#ifndef TT_INCLUDE__WOLNATINTERFACE_H
#define TT_INCLUDE__WOLNATINTERFACE_H



class cPacket;
class SocketHandlerClass;
class IPAddressClass;



class WOLNATInterfaceClass
{

	UNK unk0000[35]; // 0000  ----
	bool unk007C; // 008C  007C
	UNK unk0080[12]; // 0090  0080
	HANDLE mutex; // 00C0  00B0
	//...

public:

	void Set_Server(bool _server);
	void Intercept_Game_Packet(cPacket& packet);
	void Service_Receive_Queue(SocketHandlerClass* socketHandler);
	BOOL Send_Game_Format_Packet_To(IPAddressClass* ipAddress, char* playerName, int, SocketHandlerClass* socketHandler);

};



extern REF_DECL2(WOLNATInterface, WOLNATInterfaceClass);



#endif