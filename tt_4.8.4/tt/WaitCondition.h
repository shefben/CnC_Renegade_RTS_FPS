#ifndef TT_INCLUDE__WAITCONDITION_H
#define TT_INCLUDE__WAITCONDITION_H



#include "RefCounted.h"



class CallbackHook;
class WideStringClass;



class WaitCondition :
	public RefCounted
{

public:

	enum WaitResult
	{
		WAIT_RESULT_PENDING, // 0
		WAIT_RESULT_FINISHED, // 1
		WAIT_RESULT_CANCELLED, // 2
		WAIT_RESULT_TIMEDOUT, // 3
		WAIT_RESULT_ERROR, // 4
	};

	virtual ~WaitCondition() {};
	virtual WaitCondition::WaitResult WaitFor(CallbackHook& callbackHook, uint32 timeout);
	virtual void WaitBeginning() = 0;
	virtual WaitResult GetResult() = 0;
	virtual void EndWait(WaitResult _result, const wchar_t* _resultText) = 0;
	virtual const WideStringClass& GetResultText() = 0;
	virtual const WideStringClass* GetWaitText() = 0;
	virtual uint32 GetTimeout() = 0;


};



#endif