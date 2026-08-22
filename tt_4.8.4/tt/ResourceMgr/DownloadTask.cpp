#include "General.h"
#include "DownloadTask.h"

#include "engine_diagnostics.h"
#include "DownloadManager.h"
#include "Internet.h"
#include "InternetFile.h"
#pragma warning(disable:6262) // Function uses '' bytes of stack: exceeds /analyze:stacksize



#define DOWNLOAD_BUFFER_SIZE (32*1024)



DownloadTask::DownloadTask(const char* _remoteUrl)
{
	remoteUrl = _remoteUrl;

	downloadManager.tasks.Add(this);
}



DownloadTask::~DownloadTask()
{
	downloadManager.tasks.Delete(this);
}



void DownloadTask::start()
{
	fileSize = 0;
	receivedSize = 0;
	success = false;
	aborted = false;

	Thread::start();
}



void DownloadTask::execute()
{
	InternetFile* remoteFile = internet.openFile(remoteUrl);
	if (remoteFile)
	{
		fileSize = remoteFile->getSize();
		
		while (!aborted)
		{
			byte data[DOWNLOAD_BUFFER_SIZE];
			int dataSize = remoteFile->read(data, sizeof(data));
			if (dataSize > 0)
			{
				InterlockedExchangeAdd((volatile LONG*)&receivedSize, dataSize);
				onDataReceived(data, dataSize);
			}
			else if (dataSize <= 0)
			{
				if (dataSize == 0)
					success = true;

				break;
			}
		}

		internet.closeFile(remoteFile);
	}
}



void DownloadTask::think()
{
	if (isRunning())
	{
		if (fileSize)
			DebugOutputString("Progress: %.1f (%d/%d)\n", receivedSize * 100.f / fileSize, receivedSize, fileSize);
		else
			DebugOutputString("Progress: %.1f (%d/?)\n", receivedSize * 100.f / fileSize, receivedSize);
	}
}
