#include "general.h"

#include "Thread.h"
Thread::Thread() :
	handle(NULL)
{
}



Thread::~Thread()
{
	if (handle)
		CloseHandle(handle);
}



void __cdecl Thread::ThreadMain(void* parameter)
{
	((Thread*)parameter)->execute();
}



void Thread::start()
{
	assert(!handle);

	handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&ThreadMain, this, CREATE_SUSPENDED, (LPDWORD)&id); // Start thread suspended to make sure the handle is set before the thread runs.
	if (handle)
	{
		ResumeThread(handle);
	}
}



void Thread::reset()
{
	assert(!isRunning());

	CloseHandle(handle);
	handle = NULL;
}



void Thread::wait() const
{
	if (handle)
		WaitForSingleObject(handle, INFINITE);
}



bool Thread::isRunning() const
{
	return handle &&
		WaitForSingleObject(handle, 0) != WAIT_OBJECT_0;
}
