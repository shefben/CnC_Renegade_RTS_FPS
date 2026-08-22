#include "general.h"
#include "ScreenshotManager.h"
#include "Screenshot.h"
#include "engine_string.h"
REF_DEF1(_Hwnd_0, HWND, 0x008304B4);



void ScreenshotManager::execute()
{
	do
	{
		CriticalSectionClass::LockClass objLock (screenshotsMutex);
		Screenshot* screenshot = screenshots[0];
		screenshots.Delete(0);
		if (!screenshot->writeToFile())
			TT_INTERRUPT; // TODO: Error message?
		delete screenshot;
	}
	while (InterlockedDecrement(&pendingTaskCount) > 0);
}



ScreenshotManager::ScreenshotManager()
{
	pendingTaskCount = 0;
	pendingScreenshotUrl = 0;
}



void ScreenshotManager::queueScreenshot(Screenshot* screenshot)
{
	{
		CriticalSectionClass::LockClass objLock (screenshotsMutex);
		screenshots.Add(screenshot);
	}
	// Check whether the screenshot writer thread needs starting (when this is the first item in the queue)
	if (InterlockedIncrement(&pendingTaskCount) == 1)
	{
		wait();
		reset();
		start();
	}
}



void ScreenshotManager::setPendingScreenshotUrl(const char* _pendingScreenshotUrl)
{
	pendingScreenshotUrl = newstr(_pendingScreenshotUrl);
}



const char* ScreenshotManager::getPendingScreenshotUrl() const
{
	return pendingScreenshotUrl;
}



void ScreenshotManager::clearPendingScreenshotUrl()
{
	delete[] pendingScreenshotUrl;
	pendingScreenshotUrl = NULL;
}



ScreenshotManager screenshotManager;



void Make_Screen_Shot
	(const char* baseFilename)
{
	Screenshot* screenshot = new Screenshot();
	screenshot->makeBuffer(_Hwnd_0);
	screenshotManager.queueScreenshot(screenshot);
}



void Make_Remote_Screen_Shot
	(const char* url)
{
	Screenshot* screenshot = new Screenshot(url);
	screenshot->makeBuffer(_Hwnd_0);
	screenshotManager.queueScreenshot(screenshot);
}
