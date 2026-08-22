#ifndef TT_INCLUDE__CNETUTIL_H
#define TT_INCLUDE__CNETUTIL_H



#include "Socket.h"



class cPacket;



class cNetUtil
{

public:

	static const uint MULTI_SENDS = 10;
	static const uint NETSTATS_SAMPLE_TIME_MS = 2000;
	static const uint KEEPALIVE_TIMEOUT_MS = 2000;
	static const uint CLIENT_CONNECTION_LOSS_TIMEOUT = 15000;
	static const uint SERVER_CONNECTION_LOSS_TIMEOUT = 15000;
	static const uint SERVER_CONNECTION_LOSS_TIMEOUT_LOADING_ALLOWANCE = 45000;

	static REF_DECL2(DefaultResendTimeoutMs, uint);

	static bool Is_Same_Address(const Address& address1, const Address& address2);
	static void Create_Local_Address(Address& address, Port port, Ip ip = 0);
	static void Set_Socket_Buffer_Sizes(SOCKET socket, int size = 10000);
	static void Create_Unbound_Socket(SOCKET& socket);
	static void Lan_Servicing(SOCKET socket, void (*handler)(cPacket&));
	static int Broadcast(SOCKET socket, Port port, cPacket& packet);

};



#endif