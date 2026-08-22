#ifndef TT_INCLUDE__SCREENSHOT_H
#define TT_INCLUDE__SCREENSHOT_H

class Screenshot
{

private:

	char filename[MAX_PATH];
	char* url;
	HDC bufferDevice;
	HBITMAP bufferBitmap;
	uint bufferWidth;
	uint bufferHeight;


public:

	Screenshot();
	Screenshot(const char* _url);
	~Screenshot();

	void determineFilename();
	void makeBuffer(HWND windowHandle);
	bool writeToFile();

};
#endif
