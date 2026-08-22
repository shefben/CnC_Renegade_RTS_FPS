#include "General.h"
#include "TimeManager.h"



REF_DEF2(TimeManager::FrameTicks, int, 0x00857274, 0x0085645C);
REF_DEF2(TimeManager::RealFrameTicks, int, 0x00857278, 0x00856460);
REF_DEF2(TimeManager::LastTicks, int, 0x0085727C, 0x00856464);
REF_DEF2(TimeManager::TimeScale, float, 0x00811E60, 0x00811038);
REF_DEF2(TimeManager::TotalSeconds, float, 0x00857280, 0x00856468);
REF_DEF2(TimeManager::AveragedFPS, float, 0x00857284, 0x0085646C);
REF_DEF2(TimeManager::AveragedFPSTicks, int, 0x00857288, 0x00856470);
REF_DEF2(TimeManager::AveragedFPSCounter, int, 0x0085728C, 0x00856474);
REF_DEF2(TimeManager::FrameSeconds, float, 0x00857290, 0x00856478);
REF_DEF2(TimeManager::RealFrameSeconds, float, 0x00857294, 0x0085647C);


RENEGADE_FUNCTION
FrameTimeHistogramClass *TimeManager::Peek_Frame_Time_Histogram()
AT2(0x0069DC80, 0x0069D520);



RENEGADE_FUNCTION
uint32 TimeManager::SystemTicks()
AT2(0x0069DC90, 0x0069D530);



RENEGADE_FUNCTION
void TimeManager::Reset()
AT2(0x0069DCD0, 0x0069D570);



RENEGADE_FUNCTION
void TimeManager::Update_Frame_Time()
AT2(0x0069DD10, 0x0069D5B0);



RENEGADE_FUNCTION
void TimeManager::Wait_Seconds(float)
AT2(0x0069DEC0, 0x0069D760);



RENEGADE_FUNCTION
void TimeManager::Update()
AT2(0x0069DF70, 0x0069D810);
