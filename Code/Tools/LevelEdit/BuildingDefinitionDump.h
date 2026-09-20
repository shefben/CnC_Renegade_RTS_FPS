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

/***********************************************************************************************
 *	Q-017 -- resolving real `game_object_preset` names from the installed stock data instead    *
 *	of guessing them.                                                                           *
 *                                                                                             *
 *	This is a one-shot LevelEdit command-line mode, not a UI feature.  It mounts an installed   *
 *	Renegade data directory (Always.dat / Always2.dat / Always.dbs, exactly the way real game   *
 *	startup does in `Code/Commando/init.cpp`'s `Game_Init`, minus everything else that function  *
 *	also brings up), loads `Objects.DDB` through the same `SaveGameManager::Load_Definitions`    *
 *	call the game itself uses, and writes every `CLASSID_BUILDINGS` definition -- name, id,      *
 *	class id, and every editable parameter -- to a text file.  That is enough to pick a real     *
 *	preset deliberately instead of guessing a naming convention.                                *
 *                                                                                             *
 *	Invocation:  leveledit.exe -dumpbuildingdefs="<output.txt>" -datadir="<install>\Data"        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BUILDINGDEFINITIONDUMP_H
#define BUILDINGDEFINITIONDUMP_H

class StringClass;

class BuildingDefinitionDumpClass
{
	public:

		/*
		**	Looks for `-dumpbuildingdefs=` on the raw command line.  Absent: returns false and
		**	does nothing, so normal LevelEdit startup continues.  Present: runs the dump, writes
		**	the report (or the error) to that path, and returns true -- the caller should
		**	terminate immediately rather than open the editor UI.
		*/
		static bool		Maybe_Run_From_Command_Line (const char *raw_cmd_line);

		//	The dump itself, callable directly if a caller already has both paths in hand.
		static bool		Dump_To_File (const char *data_dir,const char *output_path,StringClass *error_out);
};

#endif // BUILDINGDEFINITIONDUMP_H
