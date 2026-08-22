/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ssgmgamelog.h"

#include <winsock2.h>

#include "gameeventbus.h"
#include "ssgmsettings.h"
#include "wwstring.h"
#include "vector.h"
#include "wwdebug.h"

#include <stdio.h>
#include <string.h>
#include <time.h>


bool	SSGMGameLog::IsOpen	= false;


/*
**	One connected monitor.  The buffer holds whatever arrived that was not yet
**	a whole line: TCP has no idea where a command ends, so a command can arrive
**	in pieces and two commands can arrive in one read.
*/
struct	SSGMConnectionStruct
{
	SOCKET	Socket;
	char		Buffer[512];
	int		Filled;
};

static SOCKET											_ListenSocket	= INVALID_SOCKET;
static DynamicVectorClass<SSGMConnectionStruct *>	_Connections;


////////////////////////////////////////////////////////////////
//
//	Init
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Init (int port)
{
	Shutdown ();

	if (port <= 0 || port > 65535) {
		return ;
	}

	WSADATA wsa_data;
	::WSAStartup (MAKEWORD (2, 2), &wsa_data);

	_ListenSocket = ::socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_ListenSocket == INVALID_SOCKET) {
		WWDEBUG_SAY (("SSGMGameLog: could not create the log socket\n"));
		return ;
	}

	sockaddr_in address;
	::memset (&address, 0, sizeof (address));
	address.sin_family		= AF_INET;
	address.sin_addr.s_addr	= INADDR_ANY;
	address.sin_port			= ::htons ((unsigned short)port);

	//
	//	Non-blocking, because this is polled from the game loop and a server
	//	that stopped to wait for a monitor would stop for everybody.
	//
	u_long non_blocking = 1;
	::ioctlsocket (_ListenSocket, FIONBIO, &non_blocking);

	if (::bind (_ListenSocket, (const sockaddr *)&address, sizeof (address)) == SOCKET_ERROR ||
		 ::listen (_ListenSocket, 3) == SOCKET_ERROR) {

		WWDEBUG_SAY (("SSGMGameLog: port %d is not available\n", port));
		::closesocket (_ListenSocket);
		_ListenSocket = INVALID_SOCKET;
		return ;
	}

	IsOpen = true;
	WWDEBUG_SAY (("SSGMGameLog: listening on port %d\n", port));
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Shutdown
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Shutdown (void)
{
	for (int index = 0; index < _Connections.Count (); index ++) {
		::closesocket (_Connections[index]->Socket);
		delete _Connections[index];
	}
	_Connections.Delete_All ();

	if (_ListenSocket != INVALID_SOCKET) {
		::closesocket (_ListenSocket);
		_ListenSocket = INVALID_SOCKET;
	}

	IsOpen = false;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Close_Connection
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Close_Connection (int index)
{
	::closesocket (_Connections[index]->Socket);
	delete _Connections[index];
	_Connections.Delete (index);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Read_Connection
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Read_Connection (int index)
{
	SSGMConnectionStruct *connection = _Connections[index];

	int room = (int)sizeof (connection->Buffer) - 1 - connection->Filled;
	if (room <= 0) {

		//
		//	A monitor that sent half a kilobyte with no newline in it is not
		//	sending commands.  Drop what it sent rather than reading zero
		//	bytes forever and calling that a graceful close.
		//
		connection->Filled = 0;
		room = (int)sizeof (connection->Buffer) - 1;
	}

	int read = ::recv (connection->Socket, connection->Buffer + connection->Filled, room, 0);

	if (read == 0 || (read == SOCKET_ERROR && ::WSAGetLastError () != WSAEWOULDBLOCK)) {
		Close_Connection (index);
		return ;
	}

	if (read <= 0) {
		return ;
	}

	//
	//	Hand over every whole line that arrived, and keep the tail.
	//
	char *start	= connection->Buffer;
	char *end	= connection->Buffer + connection->Filled + read;

	for (;;) {

		char *line_end = (char *)::memchr (start, '\n', end - start);
		if (line_end == nullptr) {
			break;
		}

		(*line_end) = 0;

		//	A tool sending CRLF should not leave the CR on the command.
		if (line_end > start && (*(line_end - 1)) == '\r') {
			(*(line_end - 1)) = 0;
		}

		if ((*start) != 0) {
			GameEventBus::Raise_Console_Input (start);
		}

		start = line_end + 1;
	}

	connection->Filled = (int)(end - start);
	::memmove (connection->Buffer, start, connection->Filled);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Think
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Think (void)
{
	if (!IsOpen) {
		return ;
	}

	SOCKET accepted = ::accept (_ListenSocket, nullptr, nullptr);
	if (accepted != INVALID_SOCKET) {

		u_long non_blocking = 1;
		::ioctlsocket (accepted, FIONBIO, &non_blocking);

		SSGMConnectionStruct *connection = new SSGMConnectionStruct;
		connection->Socket	= accepted;
		connection->Filled	= 0;
		connection->Buffer[0]	= 0;
		_Connections.Add (connection);
	}

	//
	//	Backwards, because reading can close a connection and removing one
	//	moves everything after it down.
	//
	for (int index = _Connections.Count () - 1; index >= 0; index --) {
		Read_Connection (index);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Send
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Send (const char *text)
{
	if (!IsOpen || text == nullptr) {
		return ;
	}

	//	The terminating nul is part of the record, which is how a reader
	//	knows where one ends.
	int length = (int)::strlen (text) + 1;

	for (int index = _Connections.Count () - 1; index >= 0; index --) {

		int sent = ::send (_Connections[index]->Socket, text, length, 0);

		if (sent == SOCKET_ERROR && ::WSAGetLastError () != WSAEWOULDBLOCK) {
			Close_Connection (index);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Timestamp
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Get_Timestamp (char *buffer, int size)
{
	time_t		now	= ::time (nullptr);
	struct tm	local;

	if (::localtime_s (&local, &now) == 0) {
		::_snprintf (buffer, size, "[%02d:%02d:%02d] ", local.tm_hour, local.tm_min, local.tm_sec);
	} else {
		::_snprintf (buffer, size, "[??:??:??] ");
	}

	buffer[size - 1] = 0;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Log_Message
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Log_Message (const char *message, const char *category)
{
	if (!IsOpen) {
		return ;
	}

	char timestamp[32];
	Get_Timestamp (timestamp, sizeof (timestamp));

	StringClass record = "000";
	record += timestamp;
	record += (category != nullptr) ? category : "_GENERAL";
	record += " ";
	record += (message != nullptr) ? message : "";

	Send (record);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Log_Gamelog
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Log_Gamelog (const char *format, ...)
{
	if (!IsOpen || !SSGMSettingsClass::EnableGamelog) {
		return ;
	}

	char timestamp[32];
	Get_Timestamp (timestamp, sizeof (timestamp));

	StringClass body;
	va_list args;
	va_start (args, format);
	body.Format_Args (format, args);
	va_end (args);

	StringClass record = "001";
	record += timestamp;
	record += body;

	Send (record);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Log_Ren_Log
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Log_Ren_Log (const char *message)
{
	if (!IsOpen) {
		return ;
	}

	char timestamp[32];
	Get_Timestamp (timestamp, sizeof (timestamp));

	StringClass record = "002";
	record += timestamp;
	record += (message != nullptr) ? message : "";

	Send (record);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Send_Console
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Send_Console (const char *message)
{
	if (!IsOpen) {
		return ;
	}

	StringClass record = "003";
	record += (message != nullptr) ? message : "";

	Send (record);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Log_Custom
//
////////////////////////////////////////////////////////////////
void
SSGMGameLog::Log_Custom (int channel, const char *format, ...)
{
	if (!IsOpen) {
		return ;
	}

	StringClass body;
	va_list args;
	va_start (args, format);
	body.Format_Args (format, args);
	va_end (args);

	StringClass record;
	record.Format ("%03d\n", channel);
	record += body;

	Send (record);
	return ;
}
