#pragma once



#include "Singleton.h"

#define DEBUG_ALWAYS_SHOW_OUTPUT
#define DEBUG_LOG_TO_FILE



class TTDebugManager
{

public:


	void output(const char* file, const char* function, int line, const char* format, ...);
	void output(const char* file, const char* function, int line, const char* format, va_list arguments);

};



class DebugTimer
{

	const char* function;
	const char* file;
	int line;

	char data[8192];
	uint64 startTime;

public:

	static inline uint64 computeTickFrequency()
	{
		uint64 frequency;
		QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
		return frequency;
	}

	static inline uint64 getTickFrequency()
	{
		static uint64 tickFrequency = computeTickFrequency();
		return tickFrequency;
	}

	static inline float getTickTime()
	{
		static float tickTime = 1.f / getTickFrequency();
		return tickTime;
	}

	static inline uint64 getMicroTime()
	{
		uint64 time;
		QueryPerformanceCounter((LARGE_INTEGER*)&time);
		return time * 1000000 / DebugTimer::getTickFrequency();
	}

	DebugTimer(const char* file, const char* function, int line, const char* format, ...);
	void construct(const char* file, const char* function, int line, const char* format, va_list arguments);
	~DebugTimer();

};



#define debugManager (Singleton<TTDebugManager>::getInstance())

#if defined(_DEBUG) || defined(DEBUG_ALWAYS_SHOW_OUTPUT)
#	define debugOutput(format, ...) debugManager.output(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#else
#	define debugOutput(format, ...) __noop()
#endif

#ifdef _DEBUG
#	define debugTimer(format, ...) DebugTimer __debugTimer__ ## __COUNTER__ (__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#else
#	define debugTimer(format, ...) __noop()
#endif
