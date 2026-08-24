/******************************************************************************
*
* FILE
*     remotescreenshot.cpp
*
* DESCRIPTION
*     See remotescreenshot.h.
*
******************************************************************************/

#include "remotescreenshot.h"

#include "cnetwork.h"
#include "combat.h"
#include "csscreenshotresultevent.h"
#include "ffactory.h"
#include "messagewindow.h"
#include "player.h"
#include "playermanager.h"
#include "thread.h"
#include "vector3.h"
#include "win.h"
#include "ww3d.h"
#include "wwdebug.h"
#include "wwfile.h"

#include <atomic>
#include <stdio.h>
#include <string.h>

#if defined(OPENW3D_WIN32)
	#include <wininet.h>
#endif


namespace {


/*
**	The host part of a URL, for telling the player where their picture is going.
**	Not a parser: everything between the scheme and the first slash, which is the
**	part a person reads.  Falls back to the whole URL when there is no scheme.
*/
StringClass	Host_Of (const char *url)
{
	StringClass	host = (url != nullptr) ? url : "";

	const char *start = ::strstr (host.Peek_Buffer (), "//");
	start = (start != nullptr) ? (start + 2) : host.Peek_Buffer ();

	StringClass	remainder = start;
	const char *slash = ::strchr (remainder.Peek_Buffer (), '/');
	if (slash != nullptr) {
		remainder.Erase ((int)(slash - remainder.Peek_Buffer ()),
			remainder.Get_Length () - (int)(slash - remainder.Peek_Buffer ()));
	}

	return remainder.Is_Empty () ? host : remainder;
}


/*
**	The upload, off the main thread.
**
**	It is given everything it needs at construction and touches no engine state
**	afterwards, which is the whole reason the picture is read into memory before
**	the thread starts rather than by the thread itself.
*/
class	UploaderClass : public ThreadClass
{
public:
	UploaderClass (const char *url, const char *player_name, char *image, int image_length)
		:	ThreadClass ("RemoteScreenshotUpload"),
			Url (url),
			PlayerName (player_name),
			Image (image),
			ImageLength (image_length),
			Result (REMOTE_SCREENSHOT_UPLOAD_FAILED),
			Finished (false)
	{
	}

	virtual ~UploaderClass (void)
	{
		delete [] Image;
		Image = nullptr;
	}

	int	Get_Result (void) const		{ return Result; }

	//
	//	Not Is_Running.  That flag is raised by the thread itself once it starts,
	//	so between Execute and the first instruction of the thread it reads false
	//	and a poller would take an unfinished upload for a finished one.
	//
	bool	Is_Finished (void) const	{ return Finished; }

protected:

	virtual void	Thread_Function (void) override
	{
		Result		= Upload ();
		Finished		= true;
	}

private:

	int	Upload (void);

	StringClass				Url;
	StringClass				PlayerName;
	char *					Image;
	int						ImageLength;
	int						Result;
	std::atomic<bool>		Finished;
};


#if defined(OPENW3D_WIN32)

int	UploaderClass::Upload (void)
{
	//
	//	WinINet rather than a socket and a handful of headers: it is on every
	//	Windows, it follows the machine's proxy settings, and https is a flag
	//	rather than a TLS stack this tree would otherwise have to grow.
	//
	URL_COMPONENTS	components;
	char				host[256]	= { 0 };
	char				path[1024]	= { 0 };

	::memset (&components, 0, sizeof (components));
	components.dwStructSize		= sizeof (components);
	components.lpszHostName		= host;
	components.dwHostNameLength	= sizeof (host) - 1;
	components.lpszUrlPath		= path;
	components.dwUrlPathLength	= sizeof (path) - 1;

	if (!::InternetCrackUrl (Url.Peek_Buffer (), 0, 0, &components)) {
		return REMOTE_SCREENSHOT_UPLOAD_FAILED;
	}

	const char	boundary[]	= "--openw3d-remote-screenshot";
	StringClass	prologue;
	prologue.Format (
		"%s\r\n"
		"Content-Disposition: form-data; name=\"PlayerName\"\r\n"
		"\r\n"
		"%s\r\n"
		"%s\r\n"
		"Content-Disposition: form-data; name=\"Screenshot\"; filename=\"screenshot.tga\"\r\n"
		"Content-Type: image/x-targa\r\n"
		"\r\n",
		boundary, PlayerName.Peek_Buffer (), boundary);

	StringClass	epilogue;
	epilogue.Format ("\r\n%s--\r\n", boundary);

	const int	prologue_length	= prologue.Get_Length ();
	const int	epilogue_length	= epilogue.Get_Length ();
	const int	body_length			= prologue_length + ImageLength + epilogue_length;

	char *		body = new char[body_length];
	::memcpy (body, prologue.Peek_Buffer (), prologue_length);
	::memcpy (body + prologue_length, Image, ImageLength);
	::memcpy (body + prologue_length + ImageLength, epilogue.Peek_Buffer (), epilogue_length);

	StringClass	headers;
	headers.Format ("Content-Type: multipart/form-data; boundary=%s", boundary + 2);

	int	result = REMOTE_SCREENSHOT_UPLOAD_FAILED;

	HINTERNET	session = ::InternetOpen ("OpenW3D", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (session != nullptr) {

		HINTERNET	connection = ::InternetConnect (session, host, components.nPort,
									nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		if (connection != nullptr) {

			DWORD	flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
			if (components.nScheme == INTERNET_SCHEME_HTTPS) {
				flags |= INTERNET_FLAG_SECURE;
			}

			HINTERNET	request = ::HttpOpenRequest (connection, "POST", path, nullptr, nullptr, nullptr, flags, 0);
			if (request != nullptr) {

				if (::HttpSendRequest (request, headers.Peek_Buffer (), (DWORD)-1L, body, (DWORD)body_length)) {
					result = REMOTE_SCREENSHOT_UPLOADED;
				}

				::InternetCloseHandle (request);
			}

			::InternetCloseHandle (connection);
		}

		::InternetCloseHandle (session);
	}

	delete [] body;

	return result;
}

#else	// !OPENW3D_WIN32

int	UploaderClass::Upload (void)
{
	return REMOTE_SCREENSHOT_UNSUPPORTED;
}

#endif	// OPENW3D_WIN32


//-----------------------------------------------------------------------------
//	State
//-----------------------------------------------------------------------------

StringClass			_UploadUrl;			//	server side
StringClass			_PendingUrl;		//	client side, waiting for focus
UploaderClass *	_Uploader = nullptr;


const char *	_ResultNames[REMOTE_SCREENSHOT_RESULT_COUNT] =
{
	"uploaded the screenshot",
	"is not looking at the game; the screenshot will be taken when they are",
	"is already uploading a screenshot",
	"could not take a screenshot",
	"took a screenshot but could not upload it",
	"has no screenshot uploader on its platform",
};


void	Report (int result)
{
	if (!cNetwork::I_Am_Client ()) {
		return ;
	}

	cCsScreenshotResultEvent *event = new cCsScreenshotResultEvent;
	event->Init (result);

	return ;
}


void	Tell_The_Player (const char *url)
{
	MessageWindowClass *window = CombatManager::Get_Message_Window ();
	if (window == nullptr) {
		return ;
	}

	//
	//	Built here rather than translated, because a string table entry would
	//	mean editing STRINGS.TDB and a stock installation has to keep working
	//	untouched.
	//
	StringClass	narrow;
	narrow.Format ("The server has taken a picture of your game window and is sending it to %s",
		Host_Of (url).Peek_Buffer ());

	WideStringClass	message;
	message.Convert_From (narrow);

	window->Add_Message (message, Vector3 (1.0f, 0.6f, 0.2f));
	return ;
}


void	Capture (const char *url)
{
	//
	//	The picture first, then the notice, then the upload.  The notice is not
	//	a courtesy that can be skipped when the upload path is missing: a player
	//	is told their window was captured even if it never leaves the machine.
	//
	StringClass	filename;
	WW3D::Make_Screen_Shot ("remoteshot", &filename);

	if (filename.Is_Empty ()) {
		Report (REMOTE_SCREENSHOT_CAPTURE_FAILED);
		return ;
	}

	Tell_The_Player (url);

	char *	image			= nullptr;
	int		image_length	= 0;

	FileClass *file = _TheWritingFileFactory->Get_File (filename.Peek_Buffer ());
	if (file != nullptr) {

		if (file->Open (FileClass::READ)) {
			image_length = file->Size ();
			if (image_length > 0) {
				image = new char[image_length];
				if (file->Read (image, image_length) != image_length) {
					delete [] image;
					image			= nullptr;
					image_length	= 0;
				}
			}
			file->Close ();
		}

		//
		//	Gone either way.  A picture the server asked for is not something to
		//	leave lying in the game directory.
		//
		file->Delete ();
		_TheWritingFileFactory->Return_File (file);
	}

	if (image == nullptr) {
		Report (REMOTE_SCREENSHOT_CAPTURE_FAILED);
		return ;
	}

	StringClass	player_name;
	cPlayer *	player = cPlayerManager::Find_Player (cNetwork::Get_My_Id ());
	if (player != nullptr) {
		player->Get_Name ().Convert_To (player_name);
	}

	_Uploader = new UploaderClass (url, player_name.Peek_Buffer (), image, image_length);
	_Uploader->Execute ();

	return ;
}


}	// namespace


//-----------------------------------------------------------------------------
void	RemoteScreenshot::Set_Upload_Url (const char *url)
{
	_UploadUrl = (url != nullptr) ? url : "";
	return ;
}


//-----------------------------------------------------------------------------
const char *	RemoteScreenshot::Get_Upload_Url (void)
{
	return _UploadUrl.Peek_Buffer ();
}


//-----------------------------------------------------------------------------
bool	RemoteScreenshot::Is_Enabled (void)
{
	return !_UploadUrl.Is_Empty ();
}


//-----------------------------------------------------------------------------
const char *	RemoteScreenshot::Get_Result_Name (int result)
{
	if ((result < 0) || (result >= REMOTE_SCREENSHOT_RESULT_COUNT)) {
		return "sent an answer nobody understands";
	}

	return _ResultNames[result];
}


//-----------------------------------------------------------------------------
void	RemoteScreenshot::Request (const char *url)
{
	if (!cNetwork::I_Am_Client () || (url == nullptr) || (url[0] == 0)) {
		return ;
	}

	if (_Uploader != nullptr) {
		Report (REMOTE_SCREENSHOT_BUSY);
		return ;
	}

	//
	//	A window that is not on screen renders nothing worth looking at, so the
	//	request waits rather than returning a black picture.  The operator is
	//	told that it is waiting, which is itself the answer to why nothing has
	//	arrived yet.
	//
	if (!GameInFocus) {
		_PendingUrl = url;
		Report (REMOTE_SCREENSHOT_DEFERRED);
		return ;
	}

	Capture (url);
	return ;
}


//-----------------------------------------------------------------------------
void	RemoteScreenshot::Think (void)
{
	if (!cNetwork::I_Am_Client ()) {
		return ;
	}

	if (_Uploader != nullptr) {

		if (!_Uploader->Is_Finished ()) {
			return ;
		}

		Report (_Uploader->Get_Result ());

		delete _Uploader;
		_Uploader = nullptr;
		return ;
	}

	if (!_PendingUrl.Is_Empty () && GameInFocus) {

		StringClass	url = _PendingUrl;
		_PendingUrl = "";

		Capture (url.Peek_Buffer ());
	}

	return ;
}


//-----------------------------------------------------------------------------
void	RemoteScreenshot::Shutdown (void)
{
	if (_Uploader != nullptr) {
		_Uploader->Stop ();
		delete _Uploader;
		_Uploader = nullptr;
	}

	_PendingUrl	= "";
	_UploadUrl	= "";

	return ;
}
