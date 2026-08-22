#include "General.h"
#include "PackageDownloadTask.h"

#include "PackageManager.h"



PackageDownloadTask::PackageDownloadTask(uint32 _id, const char* _remoteUrl) :
	DownloadTask(_remoteUrl)
{
	id = _id;
}



PackageDownloadTask::~PackageDownloadTask()
{
	//TT_ASSERT(MAIN_THREAD);
}



void PackageDownloadTask::onDataReceived(const byte* data, uint dataSize)
{
	//TT_ASSERT(WORKER_THREAD);

	file.Write((void*)data, dataSize);
}



void PackageDownloadTask::onFinished()
{
	//TT_ASSERT(MAIN_THREAD);
	//TT_ASSERT(WORKER_THREAD_TERMINATED);

	ChunkLoadClass fileLoader(&file);
	packageManager.add(fileLoader);
}