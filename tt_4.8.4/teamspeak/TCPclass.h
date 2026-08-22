/*	Renegade Scripts.dll
	Copyright 2013 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#ifndef TEAMSPEAK_INCLUDE__TCPCLASS_H
#define TEAMSPEAK_INCLUDE__TCPCLASS_H
class TCPSocket
{
   SOCKET Socket;
   char Host[256];
   unsigned short Port;
   bool Connected;
   int Mode;

   TCPSocket(SOCKET &_Socket, int _Mode);

public:
   
   TCPSocket();
   TCPSocket(SOCKET &_Socket);
   TCPSocket(TCPSocket &_Socket);
   TCPSocket(const char *Host, unsigned short Port);
   TCPSocket(const char *IP, unsigned short Port, int Backlog);
   ~TCPSocket();
   
   bool Client(const char *Host, unsigned short Port);
   bool Server(const char *IP, unsigned short Port, int Backlog);
   bool Accept(TCPSocket *NewConnection);

   bool Is_Connected();
   bool Is_DataAvaliable();
   bool Is_ConnectionWaiting();

   bool RecieveData(char *Data, int Length);
   bool SendData(const char *Data, int Length);

   bool Destroy();
};
#endif
