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
*     gamehint.cpp
*
* DESCRIPTION
*     See gamehint.h.
*
******************************************************************************/

#include "gamehint.h"

#include "scriptcommands.h"
#include "simplevec.h"

#include <stdio.h>


GameHintInterfaceClass *	GameHintClass::_TheHints		= nullptr;
bool								GameHintClass::_Enabled			= true;
bool								GameHintClass::_SeenLoaded		= false;


//
//	The event IDs this player has already been shown.  Small and read once, so
//	a flat list is the whole of it.
//
static SimpleDynVecClass<int>	_SeenHints;

static const char * const	HINT_FILE_NAME	= "data\\hints.cfg";


void
GameHintClass::Set_Interface (GameHintInterfaceClass *hints)
{
	_TheHints = hints;
	return ;
}


void
GameHintClass::Shutdown (void)
{
	_SeenHints.Delete_All ();
	_SeenLoaded	= false;
	_TheHints	= nullptr;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Load_Seen
//
//	4.8.4's file: one event ID per line, decimal.  A missing file means
//	nothing has been seen yet, which is not an error.
//
////////////////////////////////////////////////////////////////
void
GameHintClass::Load_Seen (void)
{
	if (_SeenLoaded) {
		return ;
	}

	_SeenLoaded = true;

	FILE *file = ::fopen (HINT_FILE_NAME, "rt");
	if (file == nullptr) {
		return ;
	}

	char line[64] = { 0 };
	while (::fgets (line, sizeof (line), file) != nullptr) {
		int event_id = ::atoi (line);
		if (event_id != 0) {
			_SeenHints.Add (event_id);
		}
	}

	::fclose (file);
	return ;
}


void
GameHintClass::Save_Seen (void)
{
	FILE *file = ::fopen (HINT_FILE_NAME, "wt");
	if (file == nullptr) {
		return ;
	}

	for (int index = 0; index < _SeenHints.Count (); index ++) {
		::fprintf (file, "%d\n", _SeenHints[index]);
	}

	::fclose (file);
	return ;
}


bool
GameHintClass::Was_Seen (int event_id)
{
	Load_Seen ();

	for (int index = 0; index < _SeenHints.Count (); index ++) {
		if (_SeenHints[index] == event_id) {
			return true;
		}
	}

	return false;
}


void
GameHintClass::Reset_Seen (void)
{
	Load_Seen ();
	_SeenHints.Delete_All ();
	Save_Seen ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Show
//
////////////////////////////////////////////////////////////////
void
GameHintClass::Show (int event_id, const char *sound, int title_id,
		int text_id, int text_id2, int text_id3, const char *texture_name)
{
	if (!_Enabled || Was_Seen (event_id)) {
		return ;
	}

	if ((sound != nullptr) && (sound[0] != 0)) {
		ScriptEngine::Create_2D_Sound (sound);
	}

	//
	//	Without a user interface there is nothing to open.  The event still
	//	counts as seen, so a tool does not accumulate them.
	//
	if (_TheHints != nullptr) {
		_TheHints->Show_Hint (title_id, text_id, text_id2, text_id3,
				((texture_name != nullptr) && (texture_name[0] != 0)) ? texture_name : nullptr);
	}

	_SeenHints.Add (event_id);
	Save_Seen ();
	return ;
}
