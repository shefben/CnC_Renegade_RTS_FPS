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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/buildingdescriptormanager.h           $*
 *                                                                                             *
 *	Roadmap Section 39, Phase 33 -- building model semantic JSON + live building definitions.    *
 *                                                                                             *
 *	The canonical registry.  Its one load-bearing promise is the reload transaction: a valid     *
 *	reload atomically replaces a descriptor, an invalid one never touches what was already       *
 *	live.  JSON parsing is nlohmann::json (vendored single header, Code/wwlib/nlohmann/json.hpp    *
 *	-- see the design doc for why vendoring rather than the roadmap's suggested vcpkg dependency,   *
 *	and Q-013 in awaiting_answers.md), never a hand-rolled parser.                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BUILDINGDESCRIPTORMANAGER_H
#define BUILDINGDESCRIPTORMANAGER_H

#include "always.h"
#include "buildingdescriptor.h"
#include "wwstring.h"


class BuildingDescriptorManager
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Parses json_text directly -- what the self check uses, with no file on disk.
		**	On success, replaces any existing descriptor of the same DefinitionName; the JSON's
		**	own "definition_name" field is overridden with the definition_name argument, so a
		**	descriptor's registry identity is never accidentally whatever a copy-pasted JSON
		**	file happened to say.
		*/
		static bool		Load_Descriptor_From_Text(const char * definition_name,const char * json_text,
																 StringClass * error_out);

		//	Reads the file, then behaves exactly like Load_Descriptor_From_Text; the path is
		//	remembered so Reload_Descriptor/Reload_All_Descriptors know where to re-read from.
		static bool		Load_Descriptor_From_File(const char * definition_name,const char * file_path,
																 StringClass * error_out);

		/*
		**	The reload transaction.  Re-parses from the path the descriptor was last loaded
		**	from (Load_Descriptor_From_Text has none, and cannot be reloaded from disk -- see
		**	Reload_Descriptor's own refusal for that case).  On success the new descriptor
		**	atomically replaces the old one; on failure the old one is untouched and the error
		**	is both returned and retained in Get_Last_Error.
		*/
		static bool		Reload_Descriptor(const char * definition_name,StringClass * error_out);

		//	Reloads every descriptor that has a file path.  Returns how many succeeded; a
		//	failure for one definition never touches another's live descriptor.
		static int		Reload_All_Descriptors(void);

		static const BuildingModelDescriptor *	Get_Descriptor(const char * definition_name);
		static int		Get_Descriptor_Count(void);
		static bool		Is_Loaded(const char * definition_name);

		static int				Get_Reload_Failure_Count(void);
		static const char *	Get_Last_Error(void);

		static void		Unload_Descriptor(const char * definition_name);
		static void		Unload_All(void);
};

#endif // BUILDINGDESCRIPTORMANAGER_H
