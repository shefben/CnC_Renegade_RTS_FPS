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
#include "General.h"
#include "TCPclass.h"
TCPSocket::TCPSocket()
{
   Socket = 0;
   memset((void *)Host,0,sizeof(Host));
   Port = 0;
   Connected = false;
   Mode = 0;
}

TCPSocket::TCPSocket(TCPSocket &_Socket)
{
   Socket = _Socket.Socket;
   memcpy(Host,_Socket.Host,sizeof(Host));
   Port = _Socket.Port;
   Connected = _Socket.Connected;
   Mode = _Socket.Mode;
}

TCPSocket::TCPSocket(const char *Host, unsigned short Port)
{
   Socket = 0;
   memset((void *)Host,0,sizeof(Host));
   Port = 0;
   Connected = false;
   Mode = 0;
   this->Client(Host, Port);
}

TCPSocket::TCPSocket(const char *IP, unsigned short Port, int Backlog)
{
   Socket = 0;
   memset((void *)Host,0,sizeof(Host));
   Port = 0;
   Connected = false;
   Mode = 0;
   this->Server(IP, Port, Backlog);
}

TCPSocket::TCPSocket(SOCKET &_Socket, int _Mode)
{
   Socket = 0;
   memset((void *)Host,0,sizeof(Host));
   Port = 0;
   Connected = false;
   Mode = 0;
   this->Socket = _Socket;
   this->Mode = _Mode;
   this->Connected = 1;
}

TCPSocket::~TCPSocket()
{
   shutdown(this->Socket, SD_BOTH);
   closesocket(this->Socket);
   Socket = 0;
   memset((void *)Host,0,sizeof(Host));
   Port = 0;
   Connected = false;
   Mode = 0;
}

bool TCPSocket::Client(const char *Host, unsigned short Port)
{
   hostent *he;
   if ((he = gethostbyname(Host)) == 0)
   {
      return 0;
   }
   SOCKET s_ = socket(AF_INET,SOCK_STREAM,0);
   if (s_ == INVALID_SOCKET)
   {
      return 0;
   }
   sockaddr_in addr;
   addr.sin_family = AF_INET;
   addr.sin_port = htons((u_short)Port);
   addr.sin_addr = *((in_addr *)he->h_addr);
   memset(&(addr.sin_zero), 0, 8);
   if (connect(s_, (sockaddr *) &addr, sizeof(sockaddr)))
   {
      return 0;
   }

   this->Socket = s_;
   this->Connected = 1;
   this->Mode = 0;
   this->Port = Port;
   strcpy_s(this->Host, 256, Host);
   return 1;
}

bool TCPSocket::Server(const char *IP, unsigned short Port, int Backlog)
{
   SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (server == INVALID_SOCKET)
   {
      return 0;
   }

   sockaddr_in service;
   service.sin_family = AF_INET;
   service.sin_addr.s_addr = inet_addr(IP == 0 ? "127.0.0.1" : IP);
   service.sin_port = htons(Port);

   if(bind(server, (SOCKADDR*) &service, sizeof(service)) == SOCKET_ERROR)
   {
      return 0;
   }

   if (listen(server, Backlog) == SOCKET_ERROR)
   {
      return 0;
   }
   
   this->Socket = server;
   this->Mode = 1;
   this->Connected = 1;
   return 1;
}

bool TCPSocket::Is_Connected()
{
   if(this->Mode == 1)
   {
      return 1;
   }

   fd_set fd;
   TIMEVAL tv;

   tv.tv_sec = 0;
   tv.tv_usec = 1000;

   FD_ZERO(&fd);
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
   FD_SET(this->Socket, &fd);

   if(select((int)this->Socket, 0, &fd, 0, &tv) == -1)
   {
      return 0;
   }

   if(FD_ISSET(this->Socket, &fd))
   {
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
      FD_CLR(this->Socket, &fd);
      return 1;
   }
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
   FD_CLR(this->Socket, &fd);

   return 0;
}

bool TCPSocket::Is_DataAvaliable()
{
   if(!this->Is_Connected())
   {
      return 0;
   }
   if(this->Mode != 0)
   {
      return 0;
   }
   
   fd_set fd;
   TIMEVAL tv;

   tv.tv_sec = 0;
   tv.tv_usec = 1000;

   FD_ZERO(&fd);
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
   FD_SET(this->Socket, &fd);

   if(select((int)this->Socket, &fd, 0, 0, &tv) == -1)
   {
      return 0;
   }

   if(FD_ISSET(this->Socket, &fd))
   {
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
      FD_CLR(this->Socket, &fd);
      return 1;
   }
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
   FD_CLR(this->Socket, &fd);
   return 0;
}

bool TCPSocket::Is_ConnectionWaiting()
{
   if(this->Mode != 1)
   {
      return 0;
   }
   
   fd_set fd;
   TIMEVAL tv;

   tv.tv_sec = 0;
   tv.tv_usec = 1000;

   FD_ZERO(&fd);
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
   FD_SET(this->Socket, &fd);

   if(select((int)this->Socket, &fd, 0, 0, &tv) == -1)
   {
      return 0;
   }

   if(FD_ISSET(this->Socket, &fd))
   {
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
      FD_CLR(this->Socket, &fd);
      return 1;
   }
#pragma warning (suppress: 4127) //warning c4127: conditional expression is constant
   FD_CLR(this->Socket, &fd);
   return 0;
}

bool TCPSocket::RecieveData(char *Data, int Length)
{
   bool GotSome = 0;
   for(int i = 0; i < Length; i++)
   {
      
      if(this->Is_DataAvaliable())
      {
         char c_;
         int ret = recv(this->Socket, &c_, 1, 0);
         if(ret <= 0)
         {
            this->Destroy();
            return 0;
         }
         Data[i] = c_;
         GotSome = 1;
      }
      else
      {
         Data[i] = 0;
         break;
      }
   }

   return GotSome;
}



bool TCPSocket::SendData(const char *Data, int Length)
{
   if(!this->Is_Connected())
   {
      return 0;
   }

   int Send_ = send(this->Socket, Data, Length, 0);
   if(Send_ != Length)
   {
      if(Send_ == -1)
      {
         return 0;
      }
      else
      {
         this->SendData(Data+Send_, Length-Send_);
      }
   }

   return 1;
}

bool TCPSocket::Destroy()
{
   shutdown(this->Socket, SD_BOTH);
   closesocket(this->Socket);

   Socket = 0;
   memset((void *)Host,0,sizeof(Host));
   Port = 0;
   Connected = false;
   Mode = 0;
   return 0;
}

bool TCPSocket::Accept(TCPSocket *NewConnection)
{
   if(!this->Is_ConnectionWaiting())
   {
      return 0;
   }

   sockaddr_in addr;
   int s_sa = sizeof(sockaddr);
   SOCKET client = accept(this->Socket, (sockaddr *)&addr, &s_sa);
   if(client == INVALID_SOCKET)
   {
      return 0;
   }

   NewConnection->TCPSocket::TCPSocket(client, 0);
   return 1;
}
