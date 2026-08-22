#include "General.h"
#include "cNetUtil.h"

#include "cPacket.h"



REF_DEF2(cNetUtil::DefaultResendTimeoutMs, uint, 0x0080EC7C, 0x0080DE54);



bool cNetUtil::Is_Same_Address(const sockaddr_in& address1, const sockaddr_in& address2)
{
	return address1.sin_addr.S_un.S_addr == address2.sin_addr.S_un.S_addr && address1.sin_port == address2.sin_port;
}



void cNetUtil::Create_Local_Address(sockaddr_in& address, Port port, Ip ip)
{
	address.sin_family = AF_INET;
	address.sin_port = ntohs(port);
	address.sin_addr.s_addr = ntohl(ip);
	memset(address.sin_zero, 0, sizeof(address.sin_zero));
}



void cNetUtil::Set_Socket_Buffer_Sizes(SOCKET socket, int size)
{
	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&size, 4);
	setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&size, 4);
}



void cNetUtil::Create_Unbound_Socket(SOCKET& socket)
{
	socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	
	uint32 isBroadcast = true;
	setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (const char*)&isBroadcast, sizeof(isBroadcast));
	
	uint32 isNonBlocking = true;
	ioctlsocket(socket, FIONBIO, (u_long*)&isNonBlocking);
}



void cNetUtil::Lan_Servicing(SOCKET socket, void (*handler)(cPacket&))
{
	cPacket packet;

#pragma warning(suppress: 4127)
	while (true)
	{
		Address address;
		int addressLength = sizeof(address);
		int packetLength = recvfrom(socket, packet.Get_Data(), packet.Get_Buffer_Size(), 0, (sockaddr*)&address, &addressLength);
		if (packetLength == -1)
			break;

		packet.Set_Bit_Write_Position(packetLength * 8);
		handler(packet);
	}
}




int cNetUtil::Broadcast(SOCKET socket, Port port, cPacket& packet)
{
	Address address;
	address.sin_family = AF_INET;
	address.sin_port = ntohs(port);
	address.sin_addr.s_addr = INADDR_BROADCAST;
	return sendto(socket, packet.Get_Data(), packet.Get_Compressed_Size_Bytes(), 0, (sockaddr*)&address, sizeof(Address));
}
