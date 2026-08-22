/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
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

#ifndef	COMMUNICATIONTYPES_H
#define	COMMUNICATIONTYPES_H

//
//	How a player-to-player communication is addressed.
//
//	These live here, below the network event classes that carry them, so that
//	the game event bus can describe a chat message or a radio command without
//	pulling in cScTextObj or CSAnnouncement.
//
//	Both enumerations are written into wire formats, so a new value is appended
//	and never inserted.
//


//
//	Text chat.
//
enum TextMessageEnum
{
	TEXT_MESSAGE_PUBLIC,
	TEXT_MESSAGE_TEAM,
	TEXT_MESSAGE_PRIVATE,

	//
	//	Not a routed message: the console TMSG command, which reaches the chat
	//	path without a sending player.
	//
	TEXT_MESSAGE_TMSG,
};

enum
{
	HOST_TEXT_SENDER = -1
};


//
//	Radio commands and battlefield announcements.
//
enum AnnouncementEnum
	{
	ANNOUNCEMENT_PUBLIC = 0,
	ANNOUNCEMENT_TEAM,
	ANNOUNCEMENT_PRIVATE,
	};

#endif	// COMMUNICATIONTYPES_H
