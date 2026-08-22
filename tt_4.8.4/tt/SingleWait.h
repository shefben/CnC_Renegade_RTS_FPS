#ifndef TT_INCLUDE__SINGLEWAIT_H
#define TT_INCLUDE__SINGLEWAIT_H



#include "WaitCondition.h"
#include "RefPtr.h"
#include "engine_string.h"



class SingleWait :
	public WaitCondition
{

protected:

	WaitResult result; // 0008
	WideStringClass resultText; // 000C
	WideStringClass waitText; // 0010
	uint32 timeout; // 0014
	UNK unk0018; // 0018

public:

	static RefPtr<SingleWait*> Create(const wchar_t* _waitText, uint32 timeout); // { return RefCount::createFromNew(new WaitCondition(_waitText, timeout)); }

	SingleWait(const wchar_t* _waitText, uint32 _timeout);

	virtual ~SingleWait() {}
	virtual void WaitBeginning() {}
	virtual WaitResult GetResult() { return result; }
	virtual void EndWait(WaitResult _result, const wchar_t* _resultText);
	virtual const WideStringClass& GetResultText() { return resultText; }
	virtual const WideStringClass* GetWaitText() { return &waitText; }
	virtual uint32 GetTimeout() { return timeout; }
	virtual void SetWaitText(const wchar_t* _waitText) { waitText = _waitText; }

}; // 001C



#endif