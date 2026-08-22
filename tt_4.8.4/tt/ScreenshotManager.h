#ifndef TT_INCLUDE__SCREENSHOTMANAGER_H
#define TT_INCLUDE__SCREENSHOTMANAGER_H

#include "Thread.h"
#include "engine_threading.h"
#include "screenshot.h"
#include "engine_vector.h"


class Screenshot;



class ScreenshotManager :
	public Thread
{

private:
	
	volatile LONG pendingTaskCount;

	CriticalSectionClass screenshotsMutex;
	SimpleDynVecClass<Screenshot*> screenshots;

	char* pendingScreenshotUrl;


protected:

	virtual void execute();
	

public:

	ScreenshotManager();

	void queueScreenshot(Screenshot* screenshot);
	void setPendingScreenshotUrl(const char* _pendingScreenshotUrl);
	const char* getPendingScreenshotUrl() const;
	void clearPendingScreenshotUrl();

};



extern ScreenshotManager screenshotManager;
#endif
