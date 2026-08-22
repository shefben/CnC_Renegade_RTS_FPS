#include "General.h"
#include "Socket.h"



Socket::Socket()
{
	canShutdown = true;
	handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	
	uint32 isBroadcast = true;
	setsockopt(handle, SOL_SOCKET, SO_BROADCAST, (const char*)&isBroadcast, sizeof(isBroadcast));
	
	uint32 isNonBlocking = true;
	ioctlsocket(handle, FIONBIO, (u_long*)&isNonBlocking);
}



Socket::Socket(SOCKET _handle)
{
	canShutdown = false;
	handle = _handle;
}



Socket::~Socket()
{
	if (canShutdown)
	{
		shutdown(handle, SD_BOTH);
		closesocket(handle);
	}
}



bool Socket::bind(Ip ip, Port port)
{
	Address address;
	address.sin_family = AF_INET;
	address.sin_port = ntohs(port);
	address.sin_addr.s_addr = ntohl(ip);
	memset(address.sin_zero, 0, sizeof(address.sin_zero));

	return ::bind(handle, (sockaddr*)&address, sizeof(address)) != SOCKET_ERROR;
}



bool Socket::sendTo(const byte* data, int dataSize, const Address& address, uint flags)
{
	return sendto(handle, (char*)data, dataSize, flags, (const sockaddr*)&address, sizeof(Address)) != SOCKET_ERROR;
}



int Socket::receiveFrom(byte* data, int maxDataSize, Address& address, uint flags)
{
	int addressLength = sizeof(Address);
	return recvfrom(handle, (char*)data, maxDataSize, flags, (sockaddr*)&address, &addressLength);
}



void Socket::setBufferSizes(uint bufferSizes)
{
	setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (const char*)&bufferSizes, 4);
	setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (const char*)&bufferSizes, 4);
}



int Socket::clearError()
{
	int error;
	int errorLen = sizeof(error);
	getsockopt(handle, SOL_SOCKET, SO_ERROR, (char*)&error, &errorLen);
	return error;
}