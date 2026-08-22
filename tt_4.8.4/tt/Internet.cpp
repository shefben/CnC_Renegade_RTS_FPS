#include "General.h"
#include "Internet.h"



#include "InternetFile.h"
#include "engine_common.h"



Internet::Internet()
{
	handle = InternetOpen(TT_INTERNET_AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	TT_ASSERT(handle);
}



Internet::~Internet()
{
	InternetCloseHandle(handle);
}


	
REF_DEF1(_Hwnd, HWND, 0x00831464);
InternetFile* Internet::openFile(const char* remoteUrl)
{
	InternetGoOnline((char*)remoteUrl, _Hwnd, NULL);
	HINTERNET fileHandle = InternetOpenUrl(handle, remoteUrl, NULL, 0, INTERNET_FLAG_EXISTING_CONNECT, NULL);
	
	if (!fileHandle)
		return NULL;

	InternetFile* file = new InternetFile(fileHandle);
	
	int httpStatus = file->getHttpStatus();
	if (httpStatus != 0 && !(httpStatus >= 200 && httpStatus < 300))
	{
		delete file;
		return NULL;
	}

	return file;
}



void Internet::closeFile(InternetFile* file)
{
	delete file;
}
