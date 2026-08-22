#include <stdlib.h>
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include "MemoryManager\MemoryManager.h"
LARGE_INTEGER frequency;
bool ThreadExit = false;

//FastAllocatorGeneral* allocg = FastAllocatorGeneral::Get_Allocator();
DWORD WINAPI Threadg(void* param)
{
	while(!ThreadExit)
	{
		enum {
			MAX_ALLOC_SIZE = 4096 - sizeof(size_t),
			ALLOC_STEP = 2
		};

		void* allocs[MAX_ALLOC_SIZE / ALLOC_STEP] = {};
		LARGE_INTEGER time_in, time_out;

		QueryPerformanceCounter(&time_in);
		for (int i = 0; i < MAX_ALLOC_SIZE / ALLOC_STEP; ++i)
		{
			size_t alloc_size = (i + 1) * ALLOC_STEP;
			allocs[i] = new char[alloc_size];
#ifndef NDEBUG
			memset(allocs[i], 0xcc, alloc_size);
#endif
		}
		QueryPerformanceCounter(&time_out);
		printf("%s alloc: %fs\n", (char*)param, (time_out.QuadPart - time_in.QuadPart)/ double(frequency.QuadPart));

		QueryPerformanceCounter(&time_in);
		for (int i = 0; i < MAX_ALLOC_SIZE / ALLOC_STEP; ++i)
		{
			int alloc_size = (i + 1) * ALLOC_STEP;
			assert(*((unsigned char*)allocs[i]) == 0xcc);
			assert(*((unsigned char*)allocs[i] + alloc_size - 1) == 0xcc);
#ifndef NDEBUG
			memset(allocs[i], 0xdd, alloc_size);
#endif
			delete[] allocs[i];
		}
		QueryPerformanceCounter(&time_out);
		printf("%s free:  %fs\n", (char*)param, (time_out.QuadPart - time_in.QuadPart)/ double(frequency.QuadPart));
	}
	delete[] param;
	return 0;
};

int main()
{
	CheckMemoryManagerConfiguration(MEMORYMANAGER_CONFIGURATION);

#ifndef NDEBUG
	// These first 4 tests should each raise at least 1 assert
	char* mismatched_op_test = new char[1];
	delete mismatched_op_test;

	char* overflow_test = new char[1];
	overflow_test[2] = 42;
	delete[] overflow_test;

	char* underflow_test = new char[1];
	underflow_test[-1] = 42;
	delete[] underflow_test;

	char* double_delete_test = new char[1];
	delete[] double_delete_test;
	delete[] double_delete_test;

	// The following test should not raise any asserts
	void* unvalidated_alloc_test = AllocateMemory(42, AllocType_Unvalidated, __FILE__, __LINE__, __FUNCTION__);
	delete[] unvalidated_alloc_test;

	// This test should be reported as a leak when the program exits.
	char* leak_test = new char[512];
#endif

	// The following test should not leak any memory if it's not interrupted mid-test
	// Begin multithread performance/safety testing (60 seconds, 16 threads)
	QueryPerformanceFrequency(&frequency);
	const int number_threads = 16;
	HANDLE threads[number_threads] = {};
	for (int i = 0; i < number_threads; ++i)
	{
		char *threadname = new char[10];
		sprintf_s(threadname, 10, "Thread%.2dg", i + 1);
		threads[i] = CreateThread(NULL, 0, &Threadg, threadname, NULL, NULL);
		Sleep(100);
	}
	Sleep(60000);

	ThreadExit = true; 
	WaitForMultipleObjects(number_threads, threads, TRUE, INFINITE);
	// End test

	system("pause");
};
