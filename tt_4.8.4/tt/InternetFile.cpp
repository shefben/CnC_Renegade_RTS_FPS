#include "General.h"
#include "InternetFile.h"



InternetFile::InternetFile(HINTERNET _handle)
{
	handle = _handle;
}



InternetFile::~InternetFile()
{
	InternetCloseHandle(handle);
}



int InternetFile::read(byte* data, uint maxDataSize)
{
	DWORD dataSize;
	InternetReadFile(handle, data, maxDataSize, &dataSize);
	return dataSize;
}



int InternetFile::getSize()
{
	LARGE_INTEGER size = {0};

	DWORD dwordSize = sizeof(size.LowPart);
	bool success = HttpQueryInfo(handle, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &size.LowPart, &dwordSize, 0) != FALSE;

	if (!success)
		size.LowPart = FtpGetFileSize(handle, (DWORD*)&size.HighPart);

	return size.LowPart;
}



int InternetFile::getHttpStatus()
{
	DWORD httpStatus;
	DWORD httpStatusSize = sizeof(httpStatus);
	bool success = HttpQueryInfo(handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &httpStatus, &httpStatusSize, 0) != FALSE;

	if (!success)
		return 0;

	return httpStatus;
}
