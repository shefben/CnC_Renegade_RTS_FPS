#include "general.h"
#include "cSinglePlayerData.h"
#include "slist.h"
#include "cpacket.h"

REF_DEF2(cSinglePlayerData::IsSinglePlayer, bool, 0x00854C61, 0x00853E49);
SList<cPacket> cSinglePlayerData::InputPacketList[2];


bool cSinglePlayerData::Is_Single_Player()
{
	return IsSinglePlayer;
}

void cSinglePlayerData::Cleanup()
{
	for (int i = 0;i < 1;i++)
	{
		for (SLNode<cPacket> *sl = InputPacketList[i].Head();sl;sl = sl->Next())
		{
			cPacket *p = sl->Data();
			InputPacketList[i].Remove(p);
			if (p)
			{
				delete p;
			}
		}
	}
}

SList<cPacket>* cSinglePlayerData::Get_Input_Packet_List(int listIndex)
{
	return &InputPacketList[listIndex];
}
