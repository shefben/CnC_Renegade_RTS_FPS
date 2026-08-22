#include "General.h"
#include "WaitCondition.h"

#include "TranslateDBClass.h"
#include "string_ids.h"
#include "SysTimeClass.h"
#include "CallbackHook.h"



WaitCondition::WaitResult WaitCondition::WaitFor(CallbackHook& callbackHook, uint32 timeout)
{
	WaitResult result;
	
	WaitBeginning();
	
	uint32 startTime = TIMEGETTIME();
	while ((result = GetResult()) != WAIT_RESULT_PENDING)
	{
		if (callbackHook.virtual0004())
		{
			EndWait(WAIT_RESULT_CANCELLED, TRANSLATE(IDS_WOL_CANCELED));
			break;
		}
		
		if (TIMEGETTIME() - startTime > timeout)
			EndWait(WAIT_RESULT_TIMEDOUT, TRANSLATE(IDS_WOL_TIMEDOUT));
	}
	
	return result;
}
