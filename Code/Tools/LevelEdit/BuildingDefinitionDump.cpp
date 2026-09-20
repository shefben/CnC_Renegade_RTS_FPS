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
 *	Q-017.  See BuildingDefinitionDump.h.                                                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "StdAfx.h"
#include "BuildingDefinitionDump.h"

#include "chunkio.h"
#include "combatchunkid.h"
#include "definition.h"
#include "definitionclassids.h"
#include "definitionmgr.h"
#include "ffactory.h"
#include "ffactorylist.h"
#include "mixfile.h"
#include "parameter.h"
#include "saveload.h"
#include "saveloadids.h"
#include "simpleparameter.h"
#include "textfile.h"
#include "wwdebug.h"
#include "wwstring.h"

#include <windows.h>


/***********************************************************************************************
 * Checkpoint -- a raw, unbuffered Win32 append-write, independent of every engine subsystem     *
 * this file otherwise exercises.  Used only while diagnosing where inside Dump_To_File a crash  *
 * happens: each call is durable on disk before the next risky step runs, so a crash mid-way     *
 * still leaves a trail of exactly how far execution got.                                        *
 *=============================================================================================*/
static void
Checkpoint (const char *output_path,const char *message)
{
	if ((output_path == nullptr) || (output_path [0] == 0)) {
		return ;
	}

	HANDLE handle = ::CreateFileA (output_path,FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,
											  OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		return ;
	}

	StringClass line ("",true);
	line.Format ("CHECKPOINT: %s\r\n",message);
	DWORD written = 0;
	::WriteFile (handle,line.Peek_Buffer (),(DWORD)line.Get_Length (),&written,nullptr);
	::CloseHandle (handle);
	return ;
}


/***********************************************************************************************
 * Trace_Debug_Message -- routes WWDEBUG_SAY output (only DefinitionMgrClass::Load_Objects'      *
 * per-definition trace is enabled while this is installed -- see Verbose_Load_Diagnostics)      *
 * through the same durable Checkpoint mechanism, so a crash mid-parse still names the exact      *
 * definition it was on.                                                                         *
 *=============================================================================================*/
static const char *	_TraceOutputPath = nullptr;

static void
Trace_Debug_Message (DebugType /*type*/,const char *message)
{
	if (_TraceOutputPath == nullptr) {
		return ;
	}

	//	Strip the trailing newline WWDEBUG_SAY callers embed; Checkpoint adds its own line ending.
	StringClass trimmed (message,true);
	int length = trimmed.Get_Length ();
	while ((length > 0) && ((trimmed [length - 1] == '\n') || (trimmed [length - 1] == '\r'))) {
		trimmed.Erase (length - 1,1);
		length --;
	}

	Checkpoint (_TraceOutputPath,trimmed.Peek_Buffer ());
	return ;
}


/***********************************************************************************************
 * Extract_Flag_Value -- pull "-flag=value" (quoted or not) out of a raw command line.           *
 *=============================================================================================*/
static bool
Extract_Flag_Value (const char *cmd_line,const char *flag,StringClass *value_out)
{
	if ((cmd_line == nullptr) || (flag == nullptr) || (value_out == nullptr)) {
		return false;
	}

	const char *found = ::strstr (cmd_line,flag);
	if (found == nullptr) {
		return false;
	}

	const char *start = found + ::strlen (flag);
	bool quoted = (*start == '"');
	if (quoted) {
		start ++;
	}

	const char *end = start;
	if (quoted) {
		while ((*end != 0) && (*end != '"')) {
			end ++;
		}
	} else {
		while ((*end != 0) && (*end != ' ')) {
			end ++;
		}
	}

	value_out->Format ("%.*s",(int)(end - start),start);
	return (value_out->Get_Length () > 0);
}


/***********************************************************************************************
 * Class_ID_Name -- symbolic name for the handful of building definition class ids.              *
 *=============================================================================================*/
static const char *
Class_ID_Name (uint32 class_id)
{
	switch (class_id) {
		case CLASSID_GAME_OBJECT_DEF_BUILDING:				return "BUILDING (base, no specialization)";
		case CLASSID_GAME_OBJECT_DEF_REFINERY:				return "REFINERY";
		case CLASSID_GAME_OBJECT_DEF_POWERPLANT:				return "POWERPLANT";
		case CLASSID_GAME_OBJECT_DEF_SOLDIER_FACTORY:		return "SOLDIER_FACTORY";
		case CLASSID_GAME_OBJECT_DEF_VEHICLE_FACTORY:		return "VEHICLE_FACTORY";
		case CLASSID_GAME_OBJECT_DEF_AIRSTRIP:				return "AIRSTRIP";
		case CLASSID_GAME_OBJECT_DEF_WARFACTORY:				return "WARFACTORY";
		case CLASSID_GAME_OBJECT_DEF_COMCENTER:				return "COMCENTER";
		case CLASSID_GAME_OBJECT_DEF_REPAIR_BAY:				return "REPAIR_BAY";
		case CLASSID_GAME_OBJECT_DEF_CONSTRUCTION_YARD:	return "CONSTRUCTION_YARD";
		case CLASSID_GAME_OBJECT_DEF_SUPERWEAPON:			return "SUPERWEAPON";
		default:														return "?";
	}
}


/***********************************************************************************************
 * Append_Parameter_Value -- best-effort generic stringifier, one branch per concrete            *
 * ParameterClass this codebase actually has (see reportmgr.cpp's Add_Definition_Dependencies    *
 * for the same cast-by-Get_Type pattern this mirrors).  Anything not covered falls through to   *
 * a labeled placeholder rather than crashing or guessing.                                       *
 *=============================================================================================*/
static void
Append_Parameter_Value (ParameterClass *parameter,StringClass *out)
{
	switch (parameter->Get_Type ()) {

		case ParameterClass::TYPE_STRING:
		case ParameterClass::TYPE_FILENAME:
		case ParameterClass::TYPE_SOUND_FILENAME:
			out->Format ("%s",((StringParameterClass *)parameter)->Get_String ());
			break;

		case ParameterClass::TYPE_INT:
			out->Format ("%d",((IntParameterClass *)parameter)->Get_Value ());
			break;

		case ParameterClass::TYPE_FLOAT:
		case ParameterClass::TYPE_ANGLE:
			out->Format ("%f",((FloatParameterClass *)parameter)->Get_Value ());
			break;

		case ParameterClass::TYPE_BOOL:
			out->Format ("%s",((BoolParameterClass *)parameter)->Get_Value () ? "true" : "false");
			break;

		case ParameterClass::TYPE_ENUM: {
			EnumParameterClass *param = (EnumParameterClass *)parameter;
			int value = param->Get_Selected_Value ();
			const char *name = "?";
			for (int index = 0; index < param->Get_Count (); index ++) {
				if (param->Get_Entry_Value (index) == value) {
					name = param->Get_Entry_Name (index);
					break;
				}
			}
			out->Format ("%s (%d)",name,value);
			break;
		}

		case ParameterClass::TYPE_VECTOR3:
		case ParameterClass::TYPE_COLOR: {
			const Vector3 &value = ((Vector3ParameterClass *)parameter)->Get_Value ();
			out->Format ("(%f, %f, %f)",value.X,value.Y,value.Z);
			break;
		}

		case ParameterClass::TYPE_VECTOR2: {
			const Vector2 &value = ((Vector2ParameterClass *)parameter)->Get_Value ();
			out->Format ("(%f, %f)",value.X,value.Y);
			break;
		}

		case ParameterClass::TYPE_PHYSDEFINITIONID:
		case ParameterClass::TYPE_MODELDEFINITIONID: {
			int id = (parameter->Get_Type () == ParameterClass::TYPE_PHYSDEFINITIONID)
						? ((PhysDefParameterClass *)parameter)->Get_Value ()
						: ((ModelDefParameterClass *)parameter)->Get_Value ();
			DefinitionClass *target = DefinitionMgrClass::Find_Definition (id,false);
			out->Format ("-> %s (id 0x%08X)",(target != nullptr) ? target->Get_Name () : "<unresolved>",id);
			break;
		}

		default:
			//
			//	Every *DefParameterClass (generic / game-obj / ammo / weapon / explosion / sound)
			//	shares this one accessor via its RTTI hook, which is why they are not spelled out
			//	individually above.
			//
			if (parameter->As_DefParameterClass () != nullptr) {
				int id = parameter->As_DefParameterClass ()->Get_Value ();
				DefinitionClass *target = DefinitionMgrClass::Find_Definition (id,false);
				out->Format ("-> %s (id 0x%08X)",(target != nullptr) ? target->Get_Name () : "<unresolved>",id);
			} else {
				out->Format ("<unhandled parameter type %d>",(int)parameter->Get_Type ());
			}
			break;
	}

	return ;
}


/***********************************************************************************************
 * Dump_To_File                                                                                  *
 *=============================================================================================*/
bool
BuildingDefinitionDumpClass::Dump_To_File (const char *data_dir,const char *output_path,StringClass *error_out)
{
	if (error_out != nullptr) {
		*error_out = "";
	}

	if ((data_dir == nullptr) || (data_dir [0] == 0)) {
		if (error_out != nullptr) *error_out = "no -datadir given";
		return false;
	}
	if ((output_path == nullptr) || (output_path [0] == 0)) {
		if (error_out != nullptr) *error_out = "no -dumpbuildingdefs output path given";
		return false;
	}

	/*
	**	Mount the installed data directory the same way real game startup does in
	**	Code/Commando/init.cpp's Game_Init: a plain directory factory for loose files, with
	**	Always.dat / Always2.dat / Always.dbs layered on top as mix archives.  Everything else
	**	Game_Init also brings up -- rendering, audio, networking, the game loop -- is not
	**	touched; the only thing this needs findable is Objects.DDB.
	**
	**	Deliberately never freed: this only ever runs as a one-shot CLI dump
	**	(Maybe_Run_From_Command_Line terminates the process right after), and unwinding a
	**	mounted mix archive mid-shutdown is more risk than it is worth.
	*/
	//
	//	SimpleFileFactoryClass concatenates SubDirectory and the requested filename with no
	//	separator of its own (see Code/wwlib/ffactory.cpp) -- Commando/init.cpp's own
	//	DATA_SUBDIRECTORY constant carries its trailing slash for exactly this reason, so a
	//	caller-supplied path without one is normalized here rather than silently mis-joined.
	//
	::DeleteFileA (output_path);
	Checkpoint (output_path,"begin");

	StringClass normalized_dir (data_dir,true);
	int dir_length = normalized_dir.Get_Length ();
	if ((dir_length > 0) && (normalized_dir [dir_length - 1] != '\\') && (normalized_dir [dir_length - 1] != '/')) {
		normalized_dir += "\\";
	}

	SimpleFileFactoryClass *base_factory = new SimpleFileFactoryClass;
	base_factory->Set_Sub_Directory (normalized_dir.Peek_Buffer ());
	Checkpoint (output_path,"base factory mounted");

	FileFactoryListClass *renegade_factory = new FileFactoryListClass;
	renegade_factory->Add_FileFactory (base_factory,"");
	renegade_factory->Add_FileFactory (new MixFileFactoryClass ("Always2.dat",base_factory),"Always2.dat");
	Checkpoint (output_path,"Always2.dat mix factory constructed");
	renegade_factory->Add_FileFactory (new MixFileFactoryClass ("Always.dbs",base_factory),"Always.dbs");
	Checkpoint (output_path,"Always.dbs mix factory constructed");
	renegade_factory->Add_FileFactory (new MixFileFactoryClass ("Always.dat",base_factory),"Always.dat");
	Checkpoint (output_path,"Always.dat mix factory constructed");

	FileFactoryClass *previous_factory = _TheFileFactory;
	_TheFileFactory = renegade_factory;

	/*
	**	Deliberately not SaveGameManager::Load_Definitions -- that hardcodes auto_post_load=true,
	**	which also runs On_Post_Load() across every loaded definition (soldiers, vehicles,
	**	weapons, sounds, scripts -- Objects.DDB is the whole game's definition set, buildings are
	**	a small slice of it).  Several of those post-load hooks reach into subsystems this dump
	**	never brings up (WW3D, audio, script commands -- there is no window and no game loop
	**	here), which crashed.  Everything this dump actually reads -- name, id, class id, and
	**	every parameter's own value -- is set by the plain field deserialization inside each
	**	DefinitionClass::Load(), which happens unconditionally, before post-load.  Skipping
	**	post-load only costs cross-definition fixups nothing here depends on.
	*/
	Checkpoint (output_path,"about to open Objects.DDB");
	FileClass *ddb_file = _TheFileFactory->Get_File ("Objects.DDB");
	if (ddb_file != nullptr) {
		ddb_file->Open (FileClass::READ);
		Checkpoint (output_path,"Objects.DDB opened, about to parse chunks (post-load skipped)");
		if (ddb_file->Is_Available ()) {
			ChunkLoadClass cload (ddb_file);

			//
			//	Per-definition trace for exactly this parse -- see Trace_Debug_Message /
			//	DefinitionMgrClass::Verbose_Load_Diagnostics.  Restored afterward either way.
			//
			_TraceOutputPath = output_path;
			PrintFunc previous_handler = ::WWDebug_Install_Message_Handler (Trace_Debug_Message);
			DefinitionMgrClass::Verbose_Load_Diagnostics = true;

			//
			//	Combat's own chunk-id range only.  This dump does not bring up WWAudio, WW3D or
			//	any of the other libraries whose definitions also live in Objects.DDB (terrain
			//	placements, lights, sounds, waypaths, ...), and their Load() paths assume those
			//	are already running -- WWAudio's crashed outright.  Buildings, along with every
			//	other Combat-registered definition, live entirely inside CHUNKID_COMBAT_BEGIN.
			//
			DefinitionMgrClass::Load_Chunk_Range_Begin = CHUNKID_COMBAT_BEGIN;
			DefinitionMgrClass::Load_Chunk_Range_End	= CHUNKID_COMMANDO_EDITOR_BEGIN;

			SaveLoadSystemClass::Load (cload,false);

			DefinitionMgrClass::Load_Chunk_Range_Begin = 0;
			DefinitionMgrClass::Load_Chunk_Range_End	= 0;
			DefinitionMgrClass::Verbose_Load_Diagnostics = false;
			::WWDebug_Install_Message_Handler (previous_handler);
			_TraceOutputPath = nullptr;
		}
		Checkpoint (output_path,"Objects.DDB chunk parse returned");
		ddb_file->Close ();
		_TheFileFactory->Return_File (ddb_file);
	} else {
		Checkpoint (output_path,"Objects.DDB: Get_File returned nullptr");
	}

	_TheFileFactory = previous_factory;

	int building_count = 0;
	for (	DefinitionClass *scan = DefinitionMgrClass::Get_First (CLASSID_BUILDINGS,DefinitionMgrClass::ID_SUPERCLASS);
			scan != nullptr;
			scan = DefinitionMgrClass::Get_Next (scan,CLASSID_BUILDINGS,DefinitionMgrClass::ID_SUPERCLASS))
	{
		building_count ++;
	}

	{
		StringClass msg ("",true);
		msg.Format ("counted CLASSID_BUILDINGS definitions: %d",building_count);
		Checkpoint (output_path,msg.Peek_Buffer ());
	}

	if (building_count == 0) {
		if (error_out != nullptr) {
			//
			//	Probe Objects.DDB directly through the same mounted factory, so the message says
			//	whether the mix archives even opened rather than just "zero buildings".
			//
			_TheFileFactory = renegade_factory;
			FileClass *probe = _TheFileFactory->Get_File ("Objects.DDB");
			bool probe_available = false;
			int probe_size = 0;
			if (probe != nullptr) {
				probe->Open (FileClass::READ);
				probe_available = probe->Is_Available ();
				probe_size = probe_available ? probe->Size () : 0;
				probe->Close ();
				_TheFileFactory->Return_File (probe);
			}
			_TheFileFactory = previous_factory;

			if (probe == nullptr) {
				error_out->Format ("no CLASSID_BUILDINGS definitions after loading Objects.DDB from '%s' "
										  "(normalized '%s') -- the mounted factory returned no file object at all "
										  "for 'Objects.DDB'",data_dir,normalized_dir.Peek_Buffer ());
			} else {
				error_out->Format ("no CLASSID_BUILDINGS definitions after loading Objects.DDB from '%s' "
										  "(normalized '%s') -- Objects.DDB via the mounted factory: %s, %d bytes",
										  data_dir,normalized_dir.Peek_Buffer (),
										  probe_available ? "available" : "NOT available",probe_size);
			}
		}
		return false;
	}

	TextFileClass file;
	file.Set_Name (output_path);
	if (!file.Open (FileClass::WRITE)) {
		if (error_out != nullptr) error_out->Format ("could not open '%s' for writing",output_path);
		return false;
	}

	StringClass line ("",true);
	line.Format ("%d building definition(s) found under CLASSID_BUILDINGS",building_count);
	file.Write_Line (line);

	for (	DefinitionClass *definition = DefinitionMgrClass::Get_First (CLASSID_BUILDINGS,DefinitionMgrClass::ID_SUPERCLASS);
			definition != nullptr;
			definition = DefinitionMgrClass::Get_Next (definition,CLASSID_BUILDINGS,DefinitionMgrClass::ID_SUPERCLASS))
	{
		line = "==========================================================================";
		file.Write_Line (line);

		line.Format ("name: %s",definition->Get_Name ());
		file.Write_Line (line);

		line.Format ("id: 0x%08X",definition->Get_ID ());
		file.Write_Line (line);

		line.Format ("class_id: 0x%08X (%s)",definition->Get_Class_ID (),Class_ID_Name (definition->Get_Class_ID ()));
		file.Write_Line (line);

		int param_count = definition->Get_Parameter_Count ();
		for (int index = 0; index < param_count; index ++) {
			ParameterClass *parameter = definition->Lock_Parameter (index);
			if (parameter == nullptr) {
				continue;
			}

			StringClass value ("",true);
			Append_Parameter_Value (parameter,&value);

			line.Format ("  %s (type %d) = %s",parameter->Get_Name (),(int)parameter->Get_Type (),value.Peek_Buffer ());
			file.Write_Line (line);
		}
	}

	file.Close ();
	return true;
}


/***********************************************************************************************
 * Maybe_Run_From_Command_Line                                                                   *
 *=============================================================================================*/
bool
BuildingDefinitionDumpClass::Maybe_Run_From_Command_Line (const char *raw_cmd_line)
{
	StringClass output_path ("",true);
	if (!Extract_Flag_Value (raw_cmd_line,"-dumpbuildingdefs=",&output_path)) {
		return false;
	}

	StringClass data_dir ("",true);
	Extract_Flag_Value (raw_cmd_line,"-datadir=",&data_dir);

	StringClass error ("",true);
	bool ok = Dump_To_File (data_dir.Peek_Buffer (),output_path.Peek_Buffer (),&error);

	if (!ok) {
		//
		//	Still write something at the requested path, so a scripted caller always has one
		//	artifact to look at instead of a silently missing file.
		//
		TextFileClass file;
		file.Set_Name (output_path.Peek_Buffer ());
		if (file.Open (FileClass::WRITE)) {
			StringClass line ("",true);
			line.Format ("FAILED: %s",error.Peek_Buffer ());
			file.Write_Line (line);
			file.Close ();
		}
	}

	return true;
}
