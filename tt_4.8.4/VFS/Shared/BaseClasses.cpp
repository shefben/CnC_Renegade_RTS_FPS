
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

/*
EventClass
*/

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

SHARED_INLINE HANDLE EventClass::GetHandle(){
	return m_Event;
}

/*
EventClass
*/

SHARED_INLINE ExternEventClass::ExternEventClass() {
	m_Event = INVALID_HANDLE_VALUE;
}

SHARED_INLINE ExternEventClass::~ExternEventClass() {
	if (m_Event != INVALID_HANDLE_VALUE){
		SetEvent(m_Event);
		CloseHandle(m_Event);
		m_Event = INVALID_HANDLE_VALUE;
	}
}

SHARED_INLINE void ExternEventClass::SetHandle(HANDLE hEvent){
	m_Event = hEvent;
}

SHARED_INLINE void ExternEventClass::Set() {
	SetEvent(m_Event);
}


SHARED_INLINE void ExternEventClass::Reset() {
	ResetEvent(m_Event);
}

SHARED_INLINE bool ExternEventClass::Wait(unsigned long msTimeout) {
	return ((WaitForSingleObject(m_Event, msTimeout) != WAIT_OBJECT_0 ? true : false));
}

/*
NamedEventClass
*/

SHARED_INLINE NamedEventClass::NamedEventClass(void) {
	m_hEvent = NULL;
}

SHARED_INLINE NamedEventClass::~NamedEventClass(void) {
	if (m_hEvent){
		SetEvent(m_hEvent);
		CloseHandle(m_hEvent);
	}
}

SHARED_INLINE void NamedEventClass::Create(const char* pName, bool ManualReset){
	m_hEvent = CreateEventA(NULL, (ManualReset ? TRUE : FALSE), FALSE, pName);
}

SHARED_INLINE void NamedEventClass::CreateW(const wchar_t* pName, bool ManualReset){
	m_hEvent = CreateEventW(NULL, (ManualReset ? TRUE : FALSE), FALSE, pName);
}

SHARED_INLINE void NamedEventClass::Set(void) {
	if (m_hEvent) SetEvent(m_hEvent);
}


SHARED_INLINE void NamedEventClass::Reset(void) {
	if (m_hEvent) ResetEvent(m_hEvent);
}

SHARED_INLINE bool NamedEventClass::Wait(unsigned long msTimeout) {
	return (m_hEvent) ? ((WaitForSingleObject(m_hEvent, msTimeout) != WAIT_OBJECT_0 ? true : false)) : false;
}

SHARED_INLINE HANDLE NamedEventClass::GetHandle(){
	return m_hEvent;
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