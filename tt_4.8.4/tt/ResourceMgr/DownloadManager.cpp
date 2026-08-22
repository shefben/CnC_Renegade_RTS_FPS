#include "General.h"
#include "DownloadManager.h"



DownloadManager::DownloadManager()
{
}



DownloadManager::~DownloadManager()
{
	TT_ASSERT(tasks.isEmpty());
}



void DownloadManager::think()
{
	for (int i = tasks.Count() - 1; i >= 0; i--)
	{
		if (tasks[i]->isRunning())
			tasks[i]->think();
		else
		{
			tasks[i]->onFinished();
			delete tasks[i];
		}
	}
}



void DownloadManager::addTask(DownloadTask& task)
{
	tasks.Add(&task);
}



bool DownloadManager::hasFinished()
{
	return tasks.isEmpty();
}