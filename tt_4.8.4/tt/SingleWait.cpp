#include "General.h"
#include "SingleWait.h"



SingleWait::SingleWait(const wchar_t* _waitText, uint32 _timeout)
{
	result = WAIT_RESULT_PENDING;
	waitText = _waitText;
	timeout = _timeout;
}




void SingleWait::EndWait(WaitCondition::WaitResult _result, const wchar_t* _resultText)
{
	result = _result;
	resultText = _resultText;
}
