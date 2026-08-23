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

/******************************************************************************
*
* FILE
*     gamehint.h
*
* DESCRIPTION
*     The popup a level uses to explain itself the first time a player meets
*     something new: a title, up to three lines of text, an optional picture
*     and a sound.
*
*     A hint is addressed by an event ID rather than by its text.  A player
*     sees each event once, ever -- the IDs already seen are remembered in
*     data/hints.cfg, which is the same file and the same format 4.8.4 used,
*     so a player who has been told already is not told again after moving to
*     this build.
*
*     The dialog itself belongs to the game rather than to Combat, so opening
*     it goes through an interface the game registers at startup.  In a tool
*     with no user interface nobody registers one and a hint is a sound and
*     nothing else, which is the right answer there.
*
******************************************************************************/

#ifndef	__GAMEHINT_H
#define	__GAMEHINT_H

#include "always.h"


class	GameHintInterfaceClass
{
public:
	virtual ~GameHintInterfaceClass (void)		{ }

	//
	//	`texture_name` is null for a hint with no picture.  The three text IDs
	//	may be zero, which means that line is not used.
	//
	virtual void	Show_Hint (int title_id, int text_id, int text_id2, int text_id3,
			const char *texture_name) = 0;
};


class	GameHintClass
{
public:

	static void								Set_Interface (GameHintInterfaceClass *hints);
	static GameHintInterfaceClass *	Peek_Interface (void)		{ return _TheHints; }

	//
	//	Whether hints are shown at all.  A player who does not want them turns
	//	them off once; the setting is not part of a level.
	//
	static bool		Are_Enabled (void)				{ return _Enabled; }
	static void		Enable (bool onoff)				{ _Enabled = onoff; }

	//
	//	Show one, unless hints are off or this event has been seen before.
	//	Client-side; the server asks for it over the wire.
	//
	static void		Show (int event_id, const char *sound, int title_id,
			int text_id, int text_id2, int text_id3, const char *texture_name);

	//	Forget every event, so they can all be seen again.
	static void		Reset_Seen (void);

	static void		Shutdown (void);

private:

	static void		Load_Seen (void);
	static void		Save_Seen (void);
	static bool		Was_Seen (int event_id);

	static GameHintInterfaceClass *	_TheHints;
	static bool								_Enabled;
	static bool								_SeenLoaded;
};


#endif	//__GAMEHINT_H
