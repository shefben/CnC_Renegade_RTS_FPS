/*	Renegade Scripts.dll
	Network related classes and engine calls
	Copyright 2009 Jonathan Wilson, Mark Sararu

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "general.h"

#include "NetworkObjectClass.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
#include "PacketManagerClass.h"

extern "C" __declspec(dllexport) void Server_Kill_Connection(int PlayerID)
{
	cNetwork::Server_Kill_Connection(PlayerID);
}

extern "C" __declspec(dllexport) void Cleanup_After_Client(int PlayerID)
{
	cNetwork::Cleanup_After_Client(PlayerID);
}


extern "C" __declspec(dllexport) const char *Get_IP_Address(int PlayerID)
{
	cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	if(clientConnection)
	{
		return inet_ntoa(clientConnection->Get_Address().sin_addr);
	}
	return "0.0.0.0";
}

extern "C" __declspec(dllexport) const char *Get_IP_Port(int PlayerID)
{
	cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	if(clientConnection)
	{
		char *temp;
		temp = new char[40];
		sprintf(temp, "%s;%u", inet_ntoa(clientConnection->Get_Address().sin_addr), clientConnection->Get_Address().sin_port);
		return temp;
	}
	return "0.0.0.0;0";
}

extern "C" __declspec(dllexport) const sockaddr_in *Get_IP(int PlayerID)
{
	 cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	 if(clientConnection)
	 {
		 return &(clientConnection->Get_Address());
	 }
	 return 0;
}

extern "C" __declspec(dllexport) int Get_Ping(int PlayerID)
{
	cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	if(clientConnection)
	{
		return clientConnection->Get_Ping();
	}
	return 0;
}

extern "C" __declspec(dllexport) unsigned long Get_Kbits(int PlayerID)
{
	cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	if(clientConnection)
	{
		return PacketManager().Get_Compressed_Bandwidth_Out(clientConnection->Get_Address()) / 1024;
	}
	return 0;
}

extern "C" __declspec(dllexport) int Get_Bandwidth(int PlayerID)
{
	cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	if(clientConnection)
	{
		return clientConnection->getMaxBandwidth();
	}
	return 0;
}
