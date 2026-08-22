
#include "BaseClasses.h"
#include "StdLib.h"


SHARED_INLINE CriticalSectionClass::LockClass::LockClass(CriticalSectionClass *pSection) {
	m_pCS = pSection;
	m_pCS->Enter();
}

SHARED_INLINE CriticalSectionClass::LockClass::~LockClass(void) {
	m_pCS->Leave();
}

SHARED_INLINE CriticalSectionClass::CriticalSectionClass(void) {
	InitializeCriticalSection(&m_CS);
}
SHARED_INLINE CriticalSectionClass::~CriticalSectionClass(void) {
	DeleteCriticalSection(&m_CS);
}

SHARED_INLINE void CriticalSectionClass::Enter(void) {
	EnterCriticalSection(&m_CS);
}

SHARED_INLINE void CriticalSectionClass::Leave(void) {
	LeaveCriticalSection(&m_CS);
}


SHARED_INLINE EventClass::EventClass(void) {
	m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

SHARED_INLINE EventClass::EventClass(bool bManualReset){
	m_Event = CreateEvent(NULL, (bManualReset ? TRUE : FALSE), FALSE, NULL);
}

SHARED_INLINE EventClass::~EventClass(void) {
	SetEvent(m_Event);
	CloseHandle(m_Event);
}

SHARED_INLINE void EventClass::Set(void) {
	SetEvent(m_Event);
}


SHARED_INLINE void EventClass::Reset(void) {
	ResetEvent(m_Event);
}

SHARED_INLINE bool EventClass::Wait(unsigned long msTimeout) {
	return ((WaitForSingleObject(m_Event, msTimeout) != WAIT_OBJECT_0 ? true : false));
}

/*
CFunctionTime
*/

CFunctionTime::CFunctionTime(const char* pFunction){
	m_pFunction = pFunction;
	m_Start_Time = timeGetTime();
}

CFunctionTime::~CFunctionTime(){
	DWORD time;
	DWORD ms, s, m, h, t;
	time = timeGetTime()-m_Start_Time;
	ms = time % 1000;
	t = time / 1000;
	s = t % 60;
	t /= 60;
	m = t % 60;
	h = t / 60;
	LogFormated(L"\"%S\" took %.3d:%.2d:%.2d.%.3d\n", m_pFunction, h, m, s, ms);
}