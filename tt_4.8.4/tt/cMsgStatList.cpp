#include "General.h"
#include "cMsgStatList.h"



#include "cMsgStat.h"



cMsgStatList::cMsgStatList() :
	stats(NULL),
	statCount(0)
{
}



cMsgStatList::~cMsgStatList()
{
	delete[] stats;
}



RENEGADE_FUNCTION
void cMsgStatList::Init(int)
AT2(0x00620260, 0x0061FF60);



RENEGADE_FUNCTION
void cMsgStatList::Increment_Num_Msg_Sent(int, int)
AT2(0x006202C0, 0x0061FFC0);



RENEGADE_FUNCTION
void cMsgStatList::Increment_Num_Byte_Sent(int, int)
AT2(0x00620300, 0x00620000);



RENEGADE_FUNCTION
void cMsgStatList::Increment_Num_Msg_Recd(int, int)
AT2(0x00620340, 0x00620040);



RENEGADE_FUNCTION
void cMsgStatList::Increment_Num_Byte_Recd(int, int)
AT2(0x00620380, 0x00620080);



RENEGADE_FUNCTION
int cMsgStatList::Get_Num_Msg_Sent(int)
AT2(0x006203C0, 0x006200C0);



RENEGADE_FUNCTION
int cMsgStatList::Get_Num_Byte_Sent(int)
AT2(0x006203E0, 0x006200E0);



RENEGADE_FUNCTION
int cMsgStatList::Get_Num_Msg_Recd(int)
AT2(0x00620400, 0x00620100);



RENEGADE_FUNCTION
int cMsgStatList::Get_Num_Byte_Recd(int)
AT2(0x00620420, 0x00620120);



RENEGADE_FUNCTION
void cMsgStatList::Compute_Avg_Num_Byte_Sent(int)
AT2(0x00620440, 0x00620140);



RENEGADE_FUNCTION
void cMsgStatList::Compute_Avg_Num_Byte_Recd(int)
AT2(0x00620460, 0x00620160);



RENEGADE_FUNCTION
cMsgStat* cMsgStatList::Get_Stat(int)
AT2(0x00620480, 0x00620180);



RENEGADE_FUNCTION
void cMsgStatList::Set_Name(int, const char*)
AT2(0x006204A0, 0x006201A0);



RENEGADE_FUNCTION
const char* cMsgStatList::Get_Name(int)
AT2(0x006204C0, 0x006201C0);
