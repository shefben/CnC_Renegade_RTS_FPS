#ifndef TT_INCLUDE__DOWNLOADTASK_H
#define TT_INCLUDE__DOWNLOADTASK_H



#include "Thread.h"
#include "engine_string.h"



class InternetFile;
class DownloadManager;



class DownloadTask :
	public Thread
{

private:

	StringClass remoteUrl;

	volatile uint fileSize;
	volatile uint receivedSize;
	volatile bool success;
	volatile bool aborted;

protected:
	
	friend DownloadManager;
	virtual ~DownloadTask();

	virtual void onDataReceived(const byte* data, uint dataSize) = 0; // Worker thread
	virtual void onFinished() = 0; // Main thread

public:

	DownloadTask(const char* _remoteUrl);
	
	virtual void start();
	virtual void execute();
	virtual void think();

};



#endif