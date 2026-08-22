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

/***********************************************************************************************
 *
 *	SSGMGameLog -- the server's log and remote console, on one TCP port.
 *
 *	A server operator points a monitoring tool at `Port` in ssgm.ini and gets two things down the
 *	one connection: a running commentary on the game, and a way to type console commands into it.
 *	Every record is a three-digit channel number, a timestamp where the channel has one, the text,
 *	and a terminating nul; anything a client sends back is read a line at a time and handed to the
 *	console.
 *
 *	The channels are SSGM's own, because the tools that read them are:
 *
 *		000	general server messages
 *		001	the game log -- kills, purchases, buildings, only when EnableGamelog is set
 *		002	whatever went to the game's own log file
 *		003	whatever the console printed
 *		nnn	a custom channel, for whoever wants one
 *
 *	With no port configured nothing is opened and every entry point here is a no-op, which is the
 *	normal case for a listen server.
 *
 *	This lives in Combat rather than beside the server manager because the scripts that write to
 *	it are compiled into the level editor too, and the editor does not link Commando.  A line a
 *	remote console types therefore goes out on the event bus instead of into the console
 *	directly; the game subscribes to it, and the editor does not.
 *
 **********************************************************************************************/

#ifndef	SSGMGAMELOG_H
#define	SSGMGAMELOG_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif


class	SSGMGameLog
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Start listening.  A port of zero means the operator did not ask for a
	//	log socket, and everything below becomes a no-op.
	//
	static void		Init (int port);
	static void		Shutdown (void);

	static bool		Is_Open (void)		{ return IsOpen; }

	//
	//	Accept new monitors and read what the connected ones have typed.
	//	Called once a frame.
	//
	static void		Think (void);

	//
	//	Channel 000: something the server wants said, tagged with a category
	//	the reading tool can filter on.
	//
	static void		Log_Message (const char *message, const char *category);

	//
	//	Channel 001: the game log.  Silent unless the operator turned the
	//	gamelog on, because it is the noisy one.
	//
	static void		Log_Gamelog (const char *format, ...);

	//	Channel 002: a line that went to the game's own log file.
	static void		Log_Ren_Log (const char *message);

	//	Channel 003: a line the console printed.
	static void		Send_Console (const char *message);

	//	Any other channel, for whoever wants one.
	static void		Log_Custom (int channel, const char *format, ...);

private:

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////

	static void		Send (const char *text);
	static void		Close_Connection (int index);
	static void		Read_Connection (int index);

	//	"[HH:MM:SS] " in local time, which is what the reading tools expect.
	static void		Get_Timestamp (char *buffer, int size);

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////

	static bool		IsOpen;
};

#endif	// SSGMGAMELOG_H
