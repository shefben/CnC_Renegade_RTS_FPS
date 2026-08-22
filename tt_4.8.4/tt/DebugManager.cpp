#include "General.h"
#include "DebugManager.h"
#include "NetworkDiagnostics.h"
#pragma warning(disable:6262) // Function uses '' bytes of stack: exceeds /analyze:stacksize


void TTDebugManager::output(const char* file, const char* function, int line, const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	output(file, function, line, format, arguments);
	va_end(arguments);
}



void TTDebugManager::output(const char* file, const char* function, int line, const char* format, va_list arguments)
{
	char data[8192];
	char fullData[8192];

	vsnprintf(data, 8192, format, arguments);
	_snprintf(fullData, 8192, "[%d] %s:%d: '%s'\n", (uint32)DebugTimer::getMicroTime(), function, line, data);

	OutputDebugString(fullData);

#ifdef DEBUG_LOG_TO_FILE
	FILE* logFile = fopen("debug.log", "a");
	fwrite(fullData, 1, strlen(fullData), logFile);
	fclose(logFile);
#endif
}



DebugTimer::DebugTimer(const char* _file, const char* _function, int _line, const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	construct(_file, _function, _line, format, arguments);
	va_end(arguments);
}



void DebugTimer::construct(const char* _file, const char* _function, int _line, const char* format, va_list arguments)
{
	file = _file;
	function = _function;
	line = _line;

	vsnprintf(data, 8192, format, arguments);
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
}



DebugTimer::~DebugTimer()
{
	uint64 endTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
	
	float elapsedTime = (endTime - startTime) * getTickTime();

	char fullData[8192];
	_snprintf(fullData, 8192, "[TT] %s:%d (%f): '%s'\n", function, line, elapsedTime, data);
	OutputDebugString(fullData);
}
