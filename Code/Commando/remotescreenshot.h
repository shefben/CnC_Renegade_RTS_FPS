/******************************************************************************
*
* FILE
*     remotescreenshot.h
*
* DESCRIPTION
*     A server asking a client for a picture of its own game window.
*
*     This is 4.8.4's ssurl/sshot pair.  It was left unported through P05
*     deliberately, recorded as a policy question rather than an engineering
*     one: a picture of somebody's screen leaving their machine is not
*     something to add by quietly copying code.  The question is answered here,
*     and the answer is written into the design rather than into a comment:
*
*       - What is captured is the game window, never the desktop.  The capture
*         is WW3D::Make_Screen_Shot, which reads the front buffer clipped to
*         the game window, so nothing outside Renegade can be in the picture.
*
*       - The player is always told.  A message goes to their message window
*         naming the host the picture is being sent to, before it is sent.
*         There is no silent path; a build that removed the notice would be a
*         different feature.
*
*       - The server operator decides whether the feature exists at all: with
*         no URL set there is nothing to ask, and asking is refused.  Joining a
*         server is voluntary, and what the server can ask for is visible in
*         the moment it asks.
*
*       - The answer always comes back, including refusals and failures, so an
*         operator can tell a client that could not comply from one that did
*         not.  See cCsScreenshotResultEvent.
*
*     The picture is an uncompressed Targa, because a Targa writer is what this
*     tree has.  A receiving endpoint should expect several megabytes.
*
******************************************************************************/

#ifndef	__REMOTESCREENSHOT_H__
#define	__REMOTESCREENSHOT_H__

#ifndef	ALWAYS_H
	#include "always.h"
#endif

#include "wwstring.h"


/*
**	What became of a request, in the words an operator needs.  The distinction
**	that matters is between a client that could not and a client that would not.
*/
enum RemoteScreenshotResultType
{
	REMOTE_SCREENSHOT_UPLOADED = 0,
	REMOTE_SCREENSHOT_DEFERRED,			// window is not in focus; taken when it is
	REMOTE_SCREENSHOT_BUSY,					// one is already on its way
	REMOTE_SCREENSHOT_CAPTURE_FAILED,
	REMOTE_SCREENSHOT_UPLOAD_FAILED,
	REMOTE_SCREENSHOT_UNSUPPORTED,		// no uploader on this platform

	REMOTE_SCREENSHOT_RESULT_COUNT
};


namespace	RemoteScreenshot
{
	//
	//	Server side.  The URL a client should upload to, as set by the ssurl
	//	console command.  Empty means the feature is off, which is the default
	//	and stays the default until an operator turns it on.
	//
	void					Set_Upload_Url (const char *url);
	const char *		Get_Upload_Url (void);
	bool					Is_Enabled (void);

	//
	//	Client side.  Take a picture of the game window and send it to url.
	//	Returns at once; the answer travels back as a cCsScreenshotResultEvent
	//	when the upload finishes.
	//
	void					Request (const char *url);

	//
	//	Client side, once a frame, outside rendering.  Takes a deferred picture
	//	when the window comes back into focus and reports finished uploads.
	//
	void					Think (void);

	void					Shutdown (void);

	//
	//	For the operator-facing print.  Never null.
	//
	const char *		Get_Result_Name (int result);
}

//-----------------------------------------------------------------------------

#endif	// __REMOTESCREENSHOT_H__
