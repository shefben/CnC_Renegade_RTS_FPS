#ifndef TT_INCLUDE__CSINGLEPLAYERDATA_H
#define TT_INCLUDE__CSINGLEPLAYERDATA_H



class cPacket;
template<typename T> class SList;



enum
{
	CLIENT_LIST,
	SERVER_LIST,
};



class cSinglePlayerData
{

public:

	static REF_DECL2(IsSinglePlayer, bool);
	static SList<cPacket> InputPacketList[2];
	static bool Is_Single_Player();
	static SList<cPacket>* Get_Input_Packet_List(int listIndex);
	static void Cleanup();

};



#endif