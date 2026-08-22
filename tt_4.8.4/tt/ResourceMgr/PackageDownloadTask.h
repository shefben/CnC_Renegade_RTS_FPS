#ifndef TT_INCLUDE__PACKAGEDOWNLOADTASK_H
#define TT_INCLUDE__PACKAGEDOWNLOADTASK_H



#include "DownloadTask.h"
#include "MemoryFile.h"



class PackageDownloadTask :
	public DownloadTask
{

private:

	uint32 id;
	MemoryFile file;

public:

	PackageDownloadTask(uint32 _id, const char* _remoteUrl);
	virtual ~PackageDownloadTask();

	virtual void onDataReceived(const byte* data, uint dataSize); // Worker thread
	virtual void onFinished(); // Main thread

};



#endif