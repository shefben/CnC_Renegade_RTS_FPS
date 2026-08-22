#ifndef TT_INCLUDE__WWMEASUREITCLASS_H
#define TT_INCLUDE__WWMEASUREITCLASS_H



class WWMeasureItClass
{

	uint64 startTime;
	float* output;

public:

	static float computeTickTime()
	{
		uint64 frequency;
		QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
		return 1.f / frequency;
	}

	static float getTickTime()
	{
		static float tickTime = computeTickTime();
		return tickTime;
	}

	WWMeasureItClass(float* _output)
	{
		output = _output;
		QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	}

	~WWMeasureItClass()
	{
		uint64 endTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
		*output = (endTime - startTime) * getTickTime();
	}

};



#endif