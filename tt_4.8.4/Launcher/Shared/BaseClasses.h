#ifndef _BASECLASSES_H_
#define _BASECLASSES_H_

#include <windows.h>
#include "Base.h"

#define ECS(cs) (EnterCriticalSection((cs)))
#define LCS(cs) (LeaveCriticalSection((cs)))
#define TCS(cs) (TryEnterCriticalSection((cs)))
#define KTCS(cs) if (!TryEnterCriticalSection((cs))) EnterCriticalSection((cs))
#define ICS(cs) (InitializeCriticalSection((cs)))
#define DCS(cs) (DeleteCriticalSection((cs)))

/*
CriticalSectionClass
*/

class CriticalSectionClass {
private:
	CRITICAL_SECTION m_CS;
public:
	SHARED_INLINE CriticalSectionClass(void);
	SHARED_INLINE ~CriticalSectionClass(void);
	class LockClass {
	private:
		CriticalSectionClass *m_pCS;
	public:
		SHARED_INLINE LockClass(CriticalSectionClass *pSection);
		SHARED_INLINE ~LockClass(void);
	};
	SHARED_INLINE void Enter(void);
	SHARED_INLINE void Leave(void);
};

/*
EventClass
*/

class EventClass {
private:
	HANDLE m_Event;
public:
	SHARED_INLINE EventClass(void);
	SHARED_INLINE EventClass(bool bManualReset);
	SHARED_INLINE ~EventClass(void);
	SHARED_INLINE void Set(void);
	SHARED_INLINE void Reset(void);
	SHARED_INLINE bool Wait(unsigned long msTimeout = INFINITE); //Returns true if timed out; use INFINITE to wait indefinitely
};

class CFunctionTime {
private:
	DWORD		m_Start_Time;
	const char*	m_pFunction;
public:
	SHARED_INLINE CFunctionTime(const char* pFunction);
	SHARED_INLINE ~CFunctionTime();
};

#define FUNC_TIME()	CFunctionTime _this_FuncTime_(__FUNCTION__)

#endif //_BASECLASSES_H_
