#ifndef TT_INCLUDE__CLANCHAT_H
#define TT_INCLUDE__CLANCHAT_H



enum ChatLocationEnum;
class cPacket;



class cLanChat
{

	SOCKET socket; // 0000
	UNK unk0004; // 0004
	UNK unk0008; // 0008
	UNK unk000C; // 000C
	UNK unk0010; // 0010
	uint unk0014; // 0014
	int unk0018; // 0018
	int unk001C; // 001C

public:

	static REF_DECL2(LAN_BROADCAST_INTERVAL_MS, short);
	static REF_DECL2(LAN_PORT, short);

	cLanChat();
	~cLanChat();
	bool Load_Lan_Registry_Keys();
	bool Save_Lan_Registry_Keys();
	void Init_Lan_Protocol_And_Socket();
	void Accept_Actions();
	void Refusal_Actions();
	void Send_Position_Broadcast();
	void Process_Position_Broadcast(cPacket&);
	void Lan_Packet_Handler(cPacket&);
	void Go_To_Location(ChatLocationEnum);
	void Think();

};



#endif