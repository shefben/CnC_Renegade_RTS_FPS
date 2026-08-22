#ifndef TT_INCLUDE__IPADDRESSCLASS_H
#define TT_INCLUDE__IPADDRESSCLASS_H



#include "Socket.h"



class IPAddressClass
{

	Ip ip; // 0000
	Port port; // 0004
	bool isValid; // 0006

public:


	Ip getIp() const
	{
		TT_ASSERT(isValid);
		return ip;
	}

	Port getPort() const
	{
		TT_ASSERT(isValid);
		return port;
	}

	bool isBroadcast() const
	{
		TT_ASSERT(isValid);
		return ip == ~0;
	}

	Address toAddress() const
	{
		Address address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = ip;
		address.sin_port = htons(port);
		return address;
	}


};



#endif