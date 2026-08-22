#include "General.h"
#include "cMsgStat.h"



cMsgStat::cMsgStat() :
	Num_Msg_Sent(0),
	Num_Byte_Sent(0),
	Num_Msg_Recd(0),
	Num_Byte_Recd(0)
{
	strncpy(Name, "UNNAMED", sizeof(Name));
}



cMsgStat::~cMsgStat()
{
}
