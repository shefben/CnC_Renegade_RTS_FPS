#include "general.h"
#include "Screenshot.h"
#include "engine_string.h"
#include <lpng/png.h>
#include "cNetwork.h"
#include "cRemoteHost.h"
const char *GetNickname2();



Screenshot::Screenshot()
{
	url = NULL;
	bufferDevice = 0;
	bufferBitmap = 0;
}



Screenshot::Screenshot(const char* _url)
{
	url = newstr(_url);
	bufferDevice = 0;
	bufferBitmap = 0;
}



Screenshot::~Screenshot()
{
	if (url)
		delete[] url;
	
	if (bufferBitmap)
	{
		DeleteObject (bufferBitmap);
		bufferBitmap = 0;
	}
	if (bufferDevice)
	{
		DeleteDC (bufferDevice);
		bufferDevice = 0;
	}
}



void Screenshot::determineFilename()
{
	if (!PathFileExists("Screenshots"))
		CreateDirectory("Screenshots", NULL);
	
	int number = 1;
	do
	{
		sprintf(filename, "Screenshots\\Screenshot.%d.png", number++);
	}
	while (PathFileExists (filename));
}



void Screenshot::makeBuffer(HWND windowHandle)
{
	RECT clientRect;
	GetClientRect(windowHandle, &clientRect);

	bufferWidth = clientRect.right;
	bufferHeight = clientRect.bottom;
	POINT topleft = {0,0};
	ClientToScreen(windowHandle,&topleft);

	assert(bufferWidth > 0);
	assert(bufferHeight > 0);

	HDC targetDevice = GetDC (NULL);

	if (bufferDevice)
	{
		DeleteObject (bufferBitmap);
		DeleteDC (bufferDevice);
	}

	bufferDevice = CreateCompatibleDC (targetDevice);
	bufferBitmap = CreateCompatibleBitmap (targetDevice, bufferWidth, bufferHeight);
	HGDIOBJ oldObj = SelectObject (bufferDevice, bufferBitmap);
	
	BitBlt (bufferDevice, 0, 0, bufferWidth, bufferHeight, targetDevice, topleft.x, topleft.y, SRCCOPY);

	SelectObject(bufferDevice,oldObj);

	ReleaseDC (windowHandle, targetDevice);
}



bool Screenshot::writeToFile()
{
	// Get bitmap data
	RGBTRIPLE* bufferPixels = new RGBTRIPLE[bufferWidth*bufferHeight];
	BITMAPINFO bufferBitmapInfo = {{sizeof(BITMAPINFOHEADER), bufferWidth, -(signed)bufferHeight, 1, 24, BI_RGB}};
	
	if (!GetDIBits(bufferDevice, bufferBitmap, 0, bufferHeight, bufferPixels, &bufferBitmapInfo, DIB_RGB_COLORS))
	{
		delete[] bufferPixels;
		return false;
	}

	// Construct rows
	RGBTRIPLE** bufferScanlines = new RGBTRIPLE*[bufferHeight];
	RGBTRIPLE* bufferScanlineIter = bufferPixels;
	for (unsigned int scanlineIndex = 0; scanlineIndex < bufferHeight; scanlineIndex++)
	{
		bufferScanlines[scanlineIndex] = bufferScanlineIter;
		bufferScanlineIter += bufferWidth;
	}

	DeleteObject (bufferBitmap);
	bufferBitmap = 0;
	DeleteDC (bufferDevice);
	bufferDevice = 0;

	// Open file
	determineFilename();
	FILE* file = fopen (filename, "wb");
	if (!file)
	{
		delete[] bufferScanlines;
		delete[] bufferPixels;
		return false;
	}

	// Write PNG
	png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL);
	if (!png_ptr)
	{
		fclose (file);
		delete[] bufferScanlines;
		delete[] bufferPixels;
		return false;
	}

	png_infop info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct (&png_ptr, NULL);
		fclose (file);
		delete[] bufferScanlines;
		delete[] bufferPixels;
		return false;
	}

	png_init_io (png_ptr, file);
	png_set_IHDR (png_ptr, info_ptr, bufferWidth, bufferHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_invert_alpha (png_ptr);
	png_set_bgr (png_ptr);
	png_write_info (png_ptr, info_ptr);
	png_write_image (png_ptr, (png_bytepp)bufferScanlines);
	png_write_end (png_ptr, NULL);

	// Cleanup
	png_destroy_write_struct (&png_ptr, &info_ptr);
	fclose (file);

	delete[] bufferScanlines;
	delete[] bufferPixels;

	if (url)
	{
		char headers[] = "Content-Type: multipart/form-data; boundary=_-leet1337tthax-_";

		int len = strlen(url);

		URL_COMPONENTS uc = {0};
		uc.dwStructSize = sizeof(uc);
		uc.dwHostNameLength = uc.dwUrlPathLength = len;
		InternetCrackUrl(url, len, 0, &uc);
		char* host = new char[uc.dwHostNameLength + 1];
		strncpy(host, uc.lpszHostName, uc.dwHostNameLength);
		host[uc.dwHostNameLength] = 0;
		char* path = new char[uc.dwUrlPathLength + 1];
		strncpy(path, uc.lpszUrlPath, uc.dwUrlPathLength);
		path[uc.dwUrlPathLength] = 0;

		const char* accept[2] = {"*/*", NULL};
		char referrer[100];
		sockaddr_in addr = cNetwork::Get_Client_Rhost()->Get_Address();
		sprintf(referrer, "renegade://%d.%d.%d.%d:%d\n", addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2, addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4, ntohs(addr.sin_port));

		HINTERNET hOpen = InternetOpen("Renegade/1.037/4.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		HINTERNET hConnect = InternetConnect(hOpen, host, uc.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
		HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", path, NULL, referrer, accept, INTERNET_FLAG_RELOAD, NULL);


		FILE *f = fopen(filename, "rb");
		fseek(f, 0, SEEK_END);
		int flen = ftell(f);
		fseek(f, 0, SEEK_SET);

		char* req = new char[flen + 1024];
		const char *c = GetNickname2();
		int offset = sprintf(req,
			"--_-leet1337tthax-_\r\nContent-Disposition: form-data; name=\"PlayerName\"\r\n\r\n%s\r\n--_-leet1337tthax-_\r\nContent-Disposition: form-data; name=\"Screenshot\"; filename=\"screenshot.png\"\r\nContent-Type: image/png\r\n\r\n",
			c);
		
		fread(req + offset, flen, 1, f);
		fclose(f);

		char trailer[] = "\r\n--_-leet1337tthax-_--\r\n";
		memcpy(req + offset + flen, trailer, sizeof(trailer) - 1);

		//build post buffer
		#pragma warning (suppress: 6385)
		HttpSendRequest(hRequest, headers, (DWORD)-1L, req, offset + flen + sizeof(trailer) - 1);

		InternetCloseHandle(hConnect);
		InternetCloseHandle(hOpen);

		delete[] req;
		delete[] host;
		delete[] path;
		delete[] c;

		DeleteFile(filename);
	}

	return true;
}
