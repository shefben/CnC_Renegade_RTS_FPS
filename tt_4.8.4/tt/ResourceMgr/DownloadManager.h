#ifndef TT_INCLUDE__DOWNLOADMANAGER_H
#define TT_INCLUDE__DOWNLOADMANAGER_H



#include "Singleton.h"
#include "CriticalSectionClass.h"
#include "DownloadTask.h"
#include "engine_string.h"
#include "engine_vector.h"



class DownloadManager
{

private:

	friend DownloadTask;
	SimpleDynVecClass<DownloadTask*> tasks;

protected:

	friend Singleton<DownloadManager>;
	DownloadManager();
	~DownloadManager();

public:

	void addTask(DownloadTask& task);
	bool hasFinished();
	void think();

};



#define downloadManager (Singleton<DownloadManager>::getInstance())



#endif