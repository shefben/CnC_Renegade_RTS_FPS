#include "General.h"
#include "cMsgStatListGroup.h"



#include "cMsgStatList.h"



cMsgStatListGroup::cMsgStatListGroup() :
	statListCount(0)
{
}



cMsgStatListGroup::~cMsgStatListGroup()
{
	delete[] statLists;
}



RENEGADE_FUNCTION
void cMsgStatListGroup::Init(int, int)
AT2(0x00620590, 0x0061F350);



RENEGADE_FUNCTION
void cMsgStatListGroup::Increment_Num_Msg_Sent(int, int, int)
AT2(0x00620610, 0x0061F3D0);



RENEGADE_FUNCTION
void cMsgStatListGroup::Increment_Num_Byte_Sent(int, int, int)
AT2(0x00620650, 0x0061F410);



RENEGADE_FUNCTION
void cMsgStatListGroup::Increment_Num_Msg_Recd(int, int, int)
AT2(0x00620690, 0x0061F450);



RENEGADE_FUNCTION
void cMsgStatListGroup::Increment_Num_Byte_Recd(int, int, int)
AT2(0x006206D0, 0x0061F490);



RENEGADE_FUNCTION
int cMsgStatListGroup::Get_Num_Msg_Sent(int, int)
AT2(0x00620710, 0x0061F4D0);



RENEGADE_FUNCTION
int cMsgStatListGroup::Get_Num_Byte_Sent(int, int)
AT2(0x00620730, 0x0061F4F0);



RENEGADE_FUNCTION
int cMsgStatListGroup::Get_Num_Msg_Recd(int, int)
AT2(0x00620750, 0x0061F510);



RENEGADE_FUNCTION
int cMsgStatListGroup::Get_Num_Byte_Recd(int, int)
AT2(0x00620770, 0x0061F530);



RENEGADE_FUNCTION
void cMsgStatListGroup::Compute_Avg_Num_Byte_Sent(int, int)
AT2(0x00620790, 0x0061F550);



RENEGADE_FUNCTION
void cMsgStatListGroup::Compute_Avg_Num_Byte_Recd(int, int)
AT2(0x006207B0, 0x0061F570);



RENEGADE_FUNCTION
cMsgStatList* cMsgStatListGroup::Get_Stat_List(int)
AT2(0x006207D0, 0x0061F590);



RENEGADE_FUNCTION
void cMsgStatListGroup::Set_Name(int, const char*)
AT2(0x006207F0, 0x0061F5B0);
