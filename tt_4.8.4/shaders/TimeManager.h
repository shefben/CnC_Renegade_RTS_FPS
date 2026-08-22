#ifndef TT_INCLUDE__TIMEMANAGER_H
#define TT_INCLUDE__TIMEMANAGER_H

class FrameTimeHistogramClass;
class TimeManager
{
public:
	static REF_DECL2(FrameTicks, int);
	static REF_DECL2(RealFrameTicks, int);
	static REF_DECL2(LastTicks, int);
	static REF_DECL2(TimeScale, float);
	static REF_DECL2(TotalSeconds, float);
	static REF_DECL2(AveragedFPS, float);
	static REF_DECL2(AveragedFPSTicks, int);
	static REF_DECL2(AveragedFPSCounter, int);
	static REF_DECL2(FrameSeconds, float);
	static REF_DECL2(RealFrameSeconds, float);

	static FrameTimeHistogramClass *Peek_Frame_Time_Histogram();
	static uint32 SystemTicks();
	static void Reset();
	static void Update_Frame_Time();
	static void Wait_Seconds(float);
	static void Update();

};


#endif
