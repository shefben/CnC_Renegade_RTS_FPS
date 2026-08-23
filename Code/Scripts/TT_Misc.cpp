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
*     TT_Misc.cpp
*
* DESCRIPTION
*     Everything a level needs that is not a weapon, a vehicle, a zone or a
*     rule of the match: the weather, the tech level, what the screen fades
*     to, which purchase entries a side can see, what an ion storm turns off,
*     and a long tail of one-idea scripts a mapper reaches for once.
*
*     Native port of the 4.8.4 library's jfwmisc.cpp.  Every script keeps its
*     registered name and its registered parameters, so existing levels find
*     them unchanged -- including two names the donor misspelled in its own
*     registration table, JFW_Change_Spawn_Characher and
*     JFW_Building_Zone_Controler, which are what levels were built against
*     and so are what is registered here.
*
*     A hundred and three scripts, and the same handful of shapes.  Eight
*     JFW_PT_Hide / JFW_PT_Show scripts are one call, two directions and four
*     triggers.  Nine building-disable scripts are one nesting counter and a
*     different thing to switch off.  Six game-hint scripts are two calls and
*     three triggers.  Four screen-fade scripts are one colour and four
*     triggers.  Three ion storms are one storm with progressively more of it
*     under the mapper's control.  Those are collapsed here into bases and
*     virtuals; the leaf classes carry the registered name and nothing else.
*
*     Defects in the donor, fixed:
*
*     - Every ion storm sent CUSTOM_AI_DISABLEAI to receiver zero rather than
*       to the base-defence vehicle it had just found, so the loop walked the
*       whole vehicle list and disabled nothing at all.  Sending to a null
*       receiver is how a script addresses no-one.  Fixed to send to the
*       vehicle, and the enable pass with it.
*
*     - The same loops asked Is_Script_Attached(obj, "JFW_EMP_Mine") -- obj
*       being the storm controller, not the vehicle the loop had reached --
*       and then called Destroy_Object(obj).  A storm configured to sweep the
*       mines away destroyed its own controller instead, and only if the
*       controller was itself a mine.  Both now name the vehicle.
*
*     - JFW_Object_Draw_Random destroyed its current model twice, the second
*       time without the guard, so the first custom event of the level called
*       Destroy_Object on the result of Find_Object(0).
*
*     - JFW_Change_Model_Created drew a number in [0,6) and mapped only 1..5,
*       looping until it drew a usable one.  A preset with every model
*       parameter left blank spun that loop forever.  The names actually
*       configured are collected first here and one of those is chosen; with
*       none configured the model is left alone.
*
*     - JFW_Object_Counter compared its running count against the limit on
*       every custom event, including the decrementing one, and destroyed the
*       sender each time it was at or over.  The test now happens only after
*       an increment, which is the only place it can newly become true.
*
*     - JFW_Killed_String_Sound called delete[] on the string the engine
*       handed back from Get_Translated_String, which it did not own.  The
*       canonical call fills a caller's WideStringClass and allocates nothing.
*
*     - JFW_Preset_Disable and the eight scripts that share its shape let the
*       nesting count go negative when an enable arrived without a matching
*       disable, after which the next disable moved the count from -1 to 0
*       and switched nothing off.  The count is clamped at zero.
*
*     - JFW_EMP restored a vehicle by turning its cloak on whether or not the
*       vehicle had one to begin with, which handed a permanent cloak to
*       anything an EMP had touched.  The prior state is recorded when the
*       cloak is cut and put back as it was.
*
*     - JFW_MSA read obj->As_VehicleGameObj()->Get_Occupant_Count() before
*       testing that obj was a vehicle at all.
*
*     - JFW_Debug_Text_File opened its log with fopen and kept the handle in
*       a script that outlives a level.  It writes through the engine's
*       writing file factory here, which needs no path, and closes on Detach
*       as well as on Destroyed so a removed script does not leak the handle.
*
*     - JFW_Custom_Create_Object_At_Bone checked that the object it had
*       attached still existed on the Destroyed path and not on the Killed
*       path or the timer path.
*
*     - JFW_User_Settable_Parameters read its parameter line with
*       fscanf("%99[^\n]") and did not check that the read succeeded, so an
*       empty file attached the named script with whatever the stack held.
*
*     - The ion storms and JFW_MSA walked GameObjManager vehicle and soldier
*       lists that this tree keeps as one list of smart objects.  The walk
*       asks each smart object what it is, which also removes the donor's
*       need to null-check every node.
*
*     Deliberate differences from the donor:
*
*     - JFW_Ion_Storm and JFW_Ion_Storm_2 test their IonChance parameter as
*       "if (chance <= IonChance) return", which is backwards from every
*       other use of a percentage in the library: a higher IonChance made
*       storms less likely and IonChance 100 meant never.  Kept as written,
*       because levels were tuned against the behaviour rather than against
*       the intent, and called out in the parameter comment.
*
*     - JFW_AirFactory_Disable acts on a building whose type is
*       BuildingConstants::TYPE_HELIPAD, which is this tree's air factory.
*       JFW_NavalFactory_Disable has no naval building type to name here --
*       a naval yard in this tree is a vehicle factory -- so it disables the
*       vehicle factory it is attached to.  Neither of the two purchase pages
*       the donor also disabled (TYPE_AIR, TYPE_NAVAL) exists here; the five
*       that do are reached through PurchaseAvailabilityClass::Set_By_Factory,
*       which is what the donor's Set_Page_Disabled was for.
*
*     - JFW_Refinery_Disable needs the refinery to stop asking for a
*       replacement harvester.  RefineryGameObj gained
*       Set_Harvester_Spawn_Blocked for it: the building still lives and
*       still unloads, it simply does not get another harvester when the last
*       one is lost.
*
*     - JFW_Wireframe_Mode is registered with no behaviour, as in the donor:
*       the class had no body there either, the work having lived in the
*       closed part of 4.8.4.  JFW_Scope likewise had an empty Created.  Both
*       are kept so a level that names them still loads.
*
*     - The sound attached to a translated string was checked for a
*       definition class ID of 0x5000 before playing.  Create_2D_Sound_Team
*       resolves the name through the audio system, which answers nothing for
*       a preset that is not a sound, so the ID check adds nothing and the
*       preset-validity test is kept alone.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "actionparams.h"
#include "armedgameobj.h"
#include "building.h"
#include "colors.h"
#include "combat.h"
#include "constructionyardgameobj.h"
#include "damage.h"
#include "damageablegameobj.h"
#include "encyclopediamgr.h"
#include "ffactory.h"
#include "gameinfo.h"
#include "gameobjmanager.h"
#include "gameobjobserver.h"
#include "matrix3.h"
#include "movephys.h"
#include "obbox.h"
#include "physcoltest.h"
#include "physicalgameobj.h"
#include "pscene.h"
#include "playertype.h"
#include "purchaseavailability.h"
#include "purchasesettings.h"
#include "refinerygameobj.h"
#include "smartgameobj.h"
#include "soldier.h"
#include "vector3.h"
#include "vehicle.h"
#include "vehiclefactorygameobj.h"
#include "weaponmanager.h"
#include "wwmath.h"
#include "wwstring.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/******************************************************************************
*
*     Shared ground
*
******************************************************************************/

/*
**	The dp88 library's turret-AI messages.  Those files are out of scope, but
**	the ion storms speak to base defences with two of their numbers, so the
**	two are spelled out here with the values that header gave them.
*/
static const int	CUSTOM_AI_DISABLEAI	= (int)(0xDA000000 | 0x00060000 | 0x03);
static const int	CUSTOM_AI_ENABLEAI	= (int)(0xDA000000 | 0x00060000 | 0x04);

/*
**	The two numbers the EMP mines talk to their manager with, unchanged from
**	the donor so a level that sends them by hand still works.
*/
static const int	EMP_MINE_DESTROYED	= 111111;
static const int	EMP_MINE_REQUESTED	= 222222;

/*
**	Whether an ion storm has suppressed every EMP on the map.  One flag for
**	the level, as in the donor: the storms are controllers, and a level runs
**	at most one storm at a time.
*/
static bool	_EmpDisabled = false;


/*
**	Seconds since the engine started ticking.  The debug log prints
**	differences of these; the donor asked the C library for wall-clock
**	time and printed differences of that.
*/
static float	Now_Seconds (void)
{
	return (float)ScriptEngine::Get_Sync_Time() * 0.001f;
}


/*
**	The vehicle an object is, rather than the vehicle an object is riding in.
*/
static VehicleGameObj *	As_Vehicle (GameObject * obj)
{
	PhysicalGameObj * physical = (obj != nullptr) ? obj->As_PhysicalGameObj() : nullptr;
	return (physical != nullptr) ? physical->As_VehicleGameObj() : nullptr;
}


static SoldierGameObj *	As_Soldier (GameObject * obj)
{
	PhysicalGameObj * physical = (obj != nullptr) ? obj->As_PhysicalGameObj() : nullptr;
	return (physical != nullptr) ? physical->As_SoldierGameObj() : nullptr;
}


static BuildingGameObj *	As_Building (GameObject * obj)
{
	return (obj != nullptr) ? obj->As_BuildingGameObj() : nullptr;
}


/*
**	A subobject name of "0" is the donor's way of saying "the whole model".
*/
static const char *	Subobject_Or_Null (const char * name)
{
	return ((name != nullptr) && (::_stricmp (name, "0") != 0)) ? name : nullptr;
}


/*
**	Play one animation on a freshly made cinematic object at a position.  Four
**	scripts in the donor, differing only in where the position came from.
*/
static void	Animated_Effect (ScriptImpClass * script, const Vector3 & position)
{
	GameObject * object = ScriptEngine::Create_Object ("Generic_Cinematic", position);
	if (object == nullptr) {
		return ;
	}

	ScriptEngine::Set_Model (object, script->Get_Parameter ("Model"));
	ScriptEngine::Set_Animation (object, script->Get_Parameter ("Animation"), false,
			Subobject_Or_Null (script->Get_Parameter ("Subobject")),
			script->Get_Float_Parameter ("FirstFrame"),
			script->Get_Float_Parameter ("LastFrame"),
			script->Get_Bool_Parameter ("Blended"));
}


/*
**	A position jittered inside a box.  The donor's random effect scripts.
*/
static Vector3	Jitter (const Vector3 & position, const Vector3 & offset)
{
	Vector3 result = position;
	result.X += ScriptEngine::Get_Random (-offset.X, offset.X);
	result.Y += ScriptEngine::Get_Random (-offset.Y, offset.Y);
	result.Z += ScriptEngine::Get_Random (-offset.Z, offset.Z);
	return result;
}


/*
**	Destroy an object named by ID, if there is one.
*/
static void	Destroy_By_ID (int object_id)
{
	GameObject * target = (object_id != 0) ? ScriptEngine::Find_Object (object_id) : nullptr;
	if (target != nullptr) {
		ScriptEngine::Destroy_Object (target);
	}
}


/*
**	The animation that ships inside a W3D file is named "model.model".
*/
static StringClass	Self_Animation (GameObject * obj)
{
	StringClass	name		= ScriptEngine::Get_Model (obj);
	StringClass	result	= name;
	result += ".";
	result += name;
	return result;
}


/*
**	Say a translated string to one side, and play whatever sound the
**	translation database has attached to it.
*/
static void	Say_String_To_Team (int team, int red, int green, int blue, int string_id)
{
	if ((string_id == 0) || !ScriptEngine::Is_Valid_String_ID (string_id)) {
		return ;
	}

	WideStringClass	wide;
	ScriptEngine::Get_Translated_String (string_id, wide);

	StringClass	narrow = wide.Peek_Buffer();
	ScriptEngine::Send_Message_Team (team, red, green, blue, narrow);

	int sound_id = ScriptEngine::Get_String_Sound_ID (string_id);
	if ((sound_id != 0) && ScriptEngine::Is_Valid_Preset_ID (sound_id)) {
		ScriptEngine::Create_2D_Sound_Team (team, ScriptEngine::Get_Definition_Name (sound_id));
	}
}


/******************************************************************************
*
*     Logging and setup
*
******************************************************************************/

/*JFW_Debug_Text_File

  Writes a line to a text file for every event the object sees.  A debugging
  aid: attach it to something that is misbehaving and read what happened in
  what order.

  The donor kept a FILE * and a path built from the game directory.  This
  writes through the engine's writing file factory, which resolves the name
  the same way every other file the game writes does, and closes on Detach as
  well as on Destroyed.
*/

REGISTER_SCRIPT_TT (JFW_Debug_Text_File, "Log_File:string,Description:string")
class	JFW_Debug_Text_File : public ScriptImpClass
{
	FileClass *	Log;
	float			Started;

	void	Line (const char * format, ...)
	{
		if (Log == nullptr) {
			return ;
		}

		char		text[512]	= { 0 };
		va_list	args;
		va_start (args, format);
		::_vsnprintf (text, sizeof (text) - 1, format, args);
		va_end (args);

		Log->Write (text, (int)::strlen (text));
	}

	//	Seconds since the script started, which is what the donor printed.
	float	Elapsed (void) const
	{
		return Now_Seconds() - Started;
	}

	void	Close (void)
	{
		if (Log != nullptr) {
			Log->Close();
			_TheWritingFileFactory->Return_File (Log);
			Log = nullptr;
		}
	}

	void	Created (GameObject * obj) override
	{
		Started	= Now_Seconds();
		Log		= _TheWritingFileFactory->Get_File (Get_Parameter ("Log_File"));
		if (Log != nullptr) {
			Log->Open (FileClass::WRITE);
		}

		Line ("%s [ID %d] created.\n", Get_Parameter ("Description"), ScriptEngine::Get_ID (obj));
	}

	void	Destroyed (GameObject * obj) override
	{
		Line ("%s [ID %d] shutdown.\n", Get_Parameter ("Description"), ScriptEngine::Get_ID (obj));
		Close();
	}

	void	Detach (GameObject * obj) override
	{
		Close();
		ScriptImpClass::Detach (obj);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Line ("%s [ID %d] killed by object %d. %f sec.\n", Get_Parameter ("Description"),
				ScriptEngine::Get_ID (obj), ScriptEngine::Get_ID (killer), Elapsed());
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		Line ("%s [ID %d] damaged by object %d. Damage was %f %f sec.\n",
				Get_Parameter ("Description"), ScriptEngine::Get_ID (obj),
				ScriptEngine::Get_ID (damager), amount, Elapsed());
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		Line ("%s [ID %d] received custom event of type %d and param %d. Sender was object %d. %f sec.\n",
				Get_Parameter ("Description"), ScriptEngine::Get_ID (obj), type, (int)param,
				ScriptEngine::Get_ID (sender), Elapsed());
	}

	void	Sound_Heard (GameObject * obj, const CombatSound & sound) override
	{
		Line ("%s [ID %d] heard sound %d at position %f %f %f object %d. %f sec.\n",
				Get_Parameter ("Description"), ScriptEngine::Get_ID (obj), (int)sound.Type,
				sound.Position.X, sound.Position.Y, sound.Position.Z,
				ScriptEngine::Get_ID (sound.Creator), Elapsed());
	}

	void	Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{
		Line ("%s [ID %d] saw enemy: object %d. %f sec.\n", Get_Parameter ("Description"),
				ScriptEngine::Get_ID (obj), ScriptEngine::Get_ID (enemy), Elapsed());
	}

	void	Action_Complete (GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		const char * text = "ACTION_COMPLETE_NORMAL";
		switch (reason) {
			case ACTION_COMPLETE_ATTACK_OUT_OF_RANGE:
				text = "ACTION_COMPLETE_ATTACK_OUT_OF_RANGE";		break;
			case ACTION_COMPLETE_MOVE_NO_PROGRESS_MADE:
				text = "ACTION_COMPLETE_MOVE_NO_PROGRESS_MADE";	break;
			case ACTION_COMPLETE_PATH_BAD_DEST:
				text = "ACTION_COMPLETE_PATH_BAD_DEST";				break;
			case ACTION_COMPLETE_PATH_BAD_START:
				text = "ACTION_COMPLETE_PATH_BAD_START";			break;
			case ACTION_COMPLETE_LOW_PRIORITY:
				text = "ACTION_COMPLETE_LOW_PRIORITY";				break;
			default:
				break;
		}

		Line ("Action %d complete on %s [ID %d] -- Reason: %s. %f sec.\n", action_id,
				Get_Parameter ("Description"), ScriptEngine::Get_ID (obj), text, Elapsed());
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		Line ("timer %d on %s [ID %d] expired. %f sec.\n", number,
				Get_Parameter ("Description"), ScriptEngine::Get_ID (obj), Elapsed());
	}

	void	Animation_Complete (GameObject * obj, const char * animation_name) override
	{
		Line ("animation %s on %s [ID %d] complete. %f sec.\n", animation_name,
				Get_Parameter ("Description"), ScriptEngine::Get_ID (obj), Elapsed());
	}

	void	Poked (GameObject * obj, GameObject * poker) override
	{
		Line ("%s [ID %d] was poked by: object %d. %f sec.\n", Get_Parameter ("Description"),
				ScriptEngine::Get_ID (obj), ScriptEngine::Get_ID (poker), Elapsed());
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		Line ("%s [ID %d] was entered by: object %d. %f sec.\n", Get_Parameter ("Description"),
				ScriptEngine::Get_ID (obj), ScriptEngine::Get_ID (enterer), Elapsed());
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		Line ("%s [ID %d] was exited by: object %d. %f sec.\n", Get_Parameter ("Description"),
				ScriptEngine::Get_ID (obj), ScriptEngine::Get_ID (exiter), Elapsed());
	}

public:

	JFW_Debug_Text_File (void) : Log (nullptr), Started (0.0f)	{ }
	~JFW_Debug_Text_File (void)	{ Close(); }
};


/*JFW_User_Settable_Parameters

  Reads one line out of a text file and attaches a named script with that line
  as its parameters, so a server operator can retune a level without opening
  the map.

  Nothing happens when the file is missing or empty; the donor attached the
  script anyway with whatever its uninitialised buffer held.
*/

DECLARE_SCRIPT_TT (JFW_User_Settable_Parameters, "File_Name:string,Script_Name:string")
{
	void	Created (GameObject * obj) override
	{
		FileClass * file = _TheFileFactory->Get_File (Get_Parameter ("File_Name"));
		if (file == nullptr) {
			return ;
		}

		char	line[100]	= { 0 };
		int	read			= 0;

		if (file->Open (FileClass::READ)) {
			read = file->Read (line, sizeof (line) - 1);
			file->Close();
		}

		_TheFileFactory->Return_File (file);

		if (read <= 0) {
			return ;
		}

		line[read] = 0;

		//	One line only, whichever way the file ends its lines.
		char * end = ::strpbrk (line, "\r\n");
		if (end != nullptr) {
			end[0] = 0;
		}

		if (line[0] != 0) {
			ScriptEngine::Attach_Script (obj, Get_Parameter ("Script_Name"), line);
		}
	}
};


/*JFW_BHS_DLL

  Announces that the level was built against a third-party DLL that this
  engine does not load, and removes itself.
*/

DECLARE_SCRIPT_TT (JFW_BHS_DLL, "")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Console_Output ("BANDTEST.DLL is required for this map\n");
		Destroy_Script();
	}
};


/*JFW_Scope

  Registered with no behaviour, as in the donor.
*/

DECLARE_SCRIPT_TT (JFW_Scope, "Scope:int")
{
};


/*JFW_Wireframe_Mode

  Registered with no behaviour, as in the donor.
*/

DECLARE_SCRIPT_TT (JFW_Wireframe_Mode, "Mode:int")
{
};


/******************************************************************************
*
*     Movement
*
******************************************************************************/

/*JFW_Follow_Waypath

  Sends the object down a waypath the moment it is created.
*/

DECLARE_SCRIPT_TT (JFW_Follow_Waypath, "Waypathid:int,Speed:float")
{
	void	Created (GameObject * obj) override
	{
		ActionParamsStruct params;
		params.MoveArrivedDistance = 100;
		params.Set_Basic (this, 100, 777);
		params.Set_Movement ((GameObject *)nullptr, Get_Float_Parameter ("Speed"), 10);
		params.WaypathID			= Get_Int_Parameter ("Waypathid");
		params.WaypathSplined	= true;
		params.AttackActive		= false;
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Goto (obj, params);
		Destroy_Script();
	}
};


/*JFW_Follow_Waypath_Zone

  The same, for anything of a named preset that walks into the zone.
*/

DECLARE_SCRIPT_TT (JFW_Follow_Waypath_Zone, "Waypathid:int,Speed:float,Preset:string")
{
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		const char * preset = ScriptEngine::Get_Preset_Name (enterer);
		if ((preset == nullptr) || (::_stricmp (preset, Get_Parameter ("Preset")) != 0)) {
			return ;
		}

		ActionParamsStruct params;
		params.MoveArrivedDistance = 100;
		params.Set_Basic (this, 100, 777);
		params.Set_Movement ((GameObject *)nullptr, Get_Float_Parameter ("Speed"), 10);
		params.WaypathID			= Get_Int_Parameter ("Waypathid");
		params.WaypathSplined	= true;
		params.AttackActive		= false;
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Goto (enterer, params);
	}
};


/*JFW_Goto_Object_On_Startup

  Walks to an object named by ID, once, and then removes itself.
*/

DECLARE_SCRIPT_TT (JFW_Goto_Object_On_Startup, "ID:int,Speed:float,ArriveDistance:float")
{
	void	Created (GameObject * obj) override
	{
		ActionParamsStruct params;
		params.Set_Basic (this, 100, 2);
		params.Set_Movement (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Float_Parameter ("Speed"), Get_Float_Parameter ("ArriveDistance"));
		ScriptEngine::Action_Goto (obj, params);
		Destroy_Script();
	}
};


/*JFW_Goto_Player_Timer

  Walks to whichever player is nearest, every so often, forever.
*/

DECLARE_SCRIPT_TT (JFW_Goto_Player_Timer, "Time:float,TimerNum:int,Speed:float,ArriveDistance:float")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) {
			return ;
		}

		ScriptEngine::Action_Reset (obj, 100);

		ActionParamsStruct params;
		params.Set_Basic (this, 100, 2);
		params.Set_Movement (ScriptEngine::Get_A_Star (ScriptEngine::Get_Position (obj)),
				Get_Float_Parameter ("Speed"), Get_Float_Parameter ("ArriveDistance"));
		ScriptEngine::Action_Goto (obj, params);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), number);
	}

	void	Action_Complete (GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ScriptEngine::Action_Reset (obj, 100);
	}
};


/*JFW_Spawner_Delay

  Holds a spawner shut for a while after the level starts.
*/

DECLARE_SCRIPT_TT (JFW_Spawner_Delay, "ID:int,time:float")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Spawner (Get_Int_Parameter ("ID"), false);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {
			ScriptEngine::Enable_Spawner (Get_Int_Parameter ("ID"), true);
		}
	}
};


/******************************************************************************
*
*     Models and animations
*
******************************************************************************/

/*JFW_Model_Animation

  Plays the animation that ships inside the object's own model, once.
*/

DECLARE_SCRIPT_TT (JFW_Model_Animation, "")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Set_Animation (obj, Self_Animation (obj), false, nullptr, 0, -1, false);
		Destroy_Script();
	}
};


/*JFW_Model_Animation_2

  The same, with the animation named rather than derived.
*/

DECLARE_SCRIPT_TT (JFW_Model_Animation_2, "Animation:string")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), false, nullptr, 0, -1, false);
		Destroy_Script();
	}
};


/*JFW_Double_Animation

  One animation split at a frame: the first custom plays up to it, the second
  plays from there to the end.  A door that opens and closes with one clip.
*/

DECLARE_SCRIPT_TT (JFW_Double_Animation, "End_Frame:float,First_Custom:int,Second_Custom:int")
{
	void	Play_First (GameObject * obj)
	{
		ScriptEngine::Set_Animation (obj, Self_Animation (obj), false, nullptr,
				0, Get_Float_Parameter ("End_Frame"), false);
	}

	void	Created (GameObject * obj) override
	{
		Play_First (obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("First_Custom")) {
			Play_First (obj);
		}

		if (type == Get_Int_Parameter ("Second_Custom")) {
			ScriptEngine::Set_Animation (obj, Self_Animation (obj), false, nullptr,
					Get_Float_Parameter ("End_Frame") + 1, -1, false);
		}
	}
};


/*JFW_Play_Animation_Destroy_Object

  Plays an animation and destroys the object when it finishes.  A first frame
  of -1 means "carry on from wherever the object already is".
*/

DECLARE_SCRIPT_TT (JFW_Play_Animation_Destroy_Object,
		"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int")
{
	void	Created (GameObject * obj) override
	{
		float first = Get_Float_Parameter ("FirstFrame");
		if (first == -1) {
			first = ScriptEngine::Get_Animation_Frame (obj);
		}

		ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), false,
				Subobject_Or_Null (Get_Parameter ("Subobject")), first,
				Get_Float_Parameter ("LastFrame"), Get_Bool_Parameter ("Blended"));
	}

	void	Animation_Complete (GameObject * obj, const char * animation_name) override
	{
		ScriptEngine::Destroy_Object (obj);
	}
};


/******************************************************************************
*
*     Animated effects
*
*     A cinematic object made where the mapper asked, given a model and one
*     animation, and destroyed when the animation ends.  The four scripts
*     differ only in where the position comes from and whether it is jittered.
*
******************************************************************************/

class	JFW_Animated_Effect_Base : public ScriptImpClass
{
protected:

	//	Where to put the effect.  The base answers the object's own position.
	virtual Vector3	Effect_Position (GameObject * obj)
	{
		return ScriptEngine::Get_Position (obj);
	}

	//	Whether the message has to match before anything happens.
	virtual bool	Wants_Message (void) const	{ return true; }

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (Wants_Message() && (type != Get_Int_Parameter ("Message"))) {
			return ;
		}

		Animated_Effect (this, Effect_Position (obj));
	}

	void	Animation_Complete (GameObject * obj, const char * animation_name) override
	{
		ScriptEngine::Destroy_Object (obj);
	}
};


REGISTER_SCRIPT_TT (JFW_Animated_Effect,
		"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,Model:string,Message:int,Location:vector3")
class	JFW_Animated_Effect : public JFW_Animated_Effect_Base
{
	Vector3	Effect_Position (GameObject * obj) override
	{
		return Get_Vector3_Parameter ("Location");
	}
};


/*
**	The donor's _2 fired on any message at all, not on a named one.
*/
REGISTER_SCRIPT_TT (JFW_Animated_Effect_2,
		"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,Model:string,Message:int")
class	JFW_Animated_Effect_2 : public JFW_Animated_Effect_Base
{
	bool	Wants_Message (void) const override	{ return false; }
};


REGISTER_SCRIPT_TT (JFW_Random_Animated_Effect,
		"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,Model:string,Message:int,Location:vector3,Offset:vector3")
class	JFW_Random_Animated_Effect : public JFW_Animated_Effect_Base
{
	Vector3	Effect_Position (GameObject * obj) override
	{
		return Jitter (Get_Vector3_Parameter ("Location"), Get_Vector3_Parameter ("Offset"));
	}
};


REGISTER_SCRIPT_TT (JFW_Random_Animated_Effect_2,
		"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,Model:string,Message:int,Offset:vector3")
class	JFW_Random_Animated_Effect_2 : public JFW_Animated_Effect_Base
{
	bool	Wants_Message (void) const override	{ return false; }

	Vector3	Effect_Position (GameObject * obj) override
	{
		return Jitter (ScriptEngine::Get_Position (obj), Get_Vector3_Parameter ("Offset"));
	}
};


/******************************************************************************
*
*     Drawing one model out of a numbered set
*
*     A cinematic object at a fixed place whose model is swapped for the next
*     in a numbered series each time a message arrives.  Three scripts: in
*     order from one, in order from a chosen number, and at random.
*
******************************************************************************/

class	JFW_Object_Draw_Base : public ScriptImpClass
{
protected:

	int	CurrentModelID;
	int	CurrentModelNumber;

	//	Which model number to draw next, and what to leave the counter at.
	virtual int	Next_Number (int count) = 0;

	void	Created (GameObject * obj) override
	{
		CurrentModelID			= 0;
		CurrentModelNumber	= First_Number();
	}

	virtual int	First_Number (void)	{ return 1; }

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Custom")) {
			return ;
		}

		Destroy_By_ID (CurrentModelID);
		CurrentModelID = 0;

		int	count		= Get_Int_Parameter ("Count");
		int	number	= Next_Number (count);

		StringClass	model;
		model.Format ("%s%d", Get_Parameter ("BaseName"), number);

		GameObject * object = ScriptEngine::Create_Object ("Generic_Cinematic",
				Get_Vector3_Parameter ("Location"));
		if (object == nullptr) {
			return ;
		}

		ScriptEngine::Enable_Hibernation (object, false);
		ScriptEngine::Add_To_Dirty_Cull_List (object);
		ScriptEngine::Set_Model (object, model);
		ScriptEngine::Set_Facing (object, Get_Float_Parameter ("Facing"));

		CurrentModelID = ScriptEngine::Get_ID (object);
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (CurrentModelNumber, 1);
		SAVE_VARIABLE (CurrentModelID, 2);
	}

public:

	JFW_Object_Draw_Base (void) : CurrentModelID (0), CurrentModelNumber (1)	{ }
};


REGISTER_SCRIPT_TT (JFW_Object_Draw_In_Order,
		" Location:vector3,Custom:int,BaseName:string,Count:int,Facing:float")
class	JFW_Object_Draw_In_Order : public JFW_Object_Draw_Base
{
	int	Next_Number (int count) override
	{
		int number = CurrentModelNumber;
		CurrentModelNumber ++;
		if (CurrentModelNumber > count) {
			CurrentModelNumber = 1;
		}
		return number;
	}
};


REGISTER_SCRIPT_TT (JFW_Object_Draw_In_Order_2,
		" Location:vector3,Custom:int,BaseName:string,Count:int,Facing:float,Start_Number:int")
class	JFW_Object_Draw_In_Order_2 : public JFW_Object_Draw_Base
{
	int	First_Number (void) override	{ return Get_Int_Parameter ("Start_Number"); }

	int	Next_Number (int count) override
	{
		int number = CurrentModelNumber;
		CurrentModelNumber ++;
		if (CurrentModelNumber > count) {
			CurrentModelNumber = Get_Int_Parameter ("Start_Number");
		}
		return number;
	}
};


REGISTER_SCRIPT_TT (JFW_Object_Draw_Random,
		" Location:vector3,Custom:int,BaseName:string,Count:int,Facing:float")
class	JFW_Object_Draw_Random : public JFW_Object_Draw_Base
{
	int	Next_Number (int count) override
	{
		return ScriptEngine::Get_Random_Int (1, count);
	}
};


/******************************************************************************
*
*     Weather and sky
*
******************************************************************************/

/*JFW_Fog_Create

  Sets the fog when the object is made, when it is destroyed, or both, with an
  optional delay.
*/

DECLARE_SCRIPT_TT (JFW_Fog_Create,
		"Fog_Enable:int,Fog_Start_Distance=0.000:float,Fog_End_Distance=0.000:float,Delay=0.000:float,OnCreate=0:int,OnDestroy=0:int")
{
	void	Apply (void)
	{
		bool enable = Get_Bool_Parameter ("Fog_Enable");
		ScriptEngine::Set_Fog_Enable (enable);
		if (enable) {
			ScriptEngine::Set_Fog_Range (Get_Float_Parameter ("Fog_Start_Distance"),
					Get_Float_Parameter ("Fog_End_Distance"), 1.0f);
		}
	}

	void	Apply_Or_Wait (GameObject * obj)
	{
		float delay = Get_Float_Parameter ("Delay");
		if (delay > 0) {
			ScriptEngine::Start_Timer (obj, this, delay, 1);
		} else {
			Apply();
		}
	}

	void	Created (GameObject * obj) override
	{
		if (Get_Int_Parameter ("OnCreate") > 0) {
			Apply_Or_Wait (obj);
		}
	}

	void	Destroyed (GameObject * obj) override
	{
		if (Get_Int_Parameter ("OnDestroy") > 0) {
			Apply_Or_Wait (obj);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {
			Apply();
		}
	}
};


/*JFW_War_Blitz_Create

  The same shape, for the distant artillery flashes on the horizon.
*/

DECLARE_SCRIPT_TT (JFW_War_Blitz_Create,
		"War_Blitz_Intensity=0.000:float,Start_Distance=0.000:float,End_Distance=1.000:float,War_Blitz_Heading=0.000:float,War_Blitz_Distribution=1.000:float,Delay=0.000:float,OnCreate=0:int,OnDestroy=0:int")
{
	void	Apply (void)
	{
		ScriptEngine::Set_War_Blitz (Get_Float_Parameter ("War_Blitz_Intensity"),
				Get_Float_Parameter ("Start_Distance"), Get_Float_Parameter ("End_Distance"),
				Get_Float_Parameter ("War_Blitz_Heading"),
				Get_Float_Parameter ("War_Blitz_Distribution"), 1.0f);
	}

	void	Apply_Or_Wait (GameObject * obj)
	{
		float delay = Get_Float_Parameter ("Delay");
		if (delay > 0) {
			ScriptEngine::Start_Timer (obj, this, delay, 1);
		} else {
			Apply();
		}
	}

	void	Created (GameObject * obj) override
	{
		if (Get_Int_Parameter ("OnCreate") > 0) {
			Apply_Or_Wait (obj);
		}
	}

	void	Destroyed (GameObject * obj) override
	{
		if (Get_Int_Parameter ("OnDestroy") > 0) {
			Apply_Or_Wait (obj);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {
			Apply();
		}
	}
};


/*JFW_Moon_Is_Earth

  Draws the earth in the sky instead of the moon.
*/

DECLARE_SCRIPT_TT (JFW_Moon_Is_Earth, "")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Set_Moon_Is_Earth (true);
		Destroy_Script();
	}
};


/******************************************************************************
*
*     The screen
*
*     Four fade scripts: on entering a zone, on leaving one, on a message, and
*     on a message with a timer that clears it again.  The first three set the
*     fade for one player; the fourth sets it for everybody, as the donor did.
*
******************************************************************************/

class	JFW_Screen_Fade_Base : public ScriptImpClass
{
protected:

	void	Fade_Player (GameObject * player)
	{
		ScriptEngine::Set_Screen_Fade_Opacity_Player (player, Get_Float_Parameter ("Opacity"), 0.0f);
		ScriptEngine::Set_Screen_Fade_Color_Player (player, Get_Float_Parameter ("Red"),
				Get_Float_Parameter ("Green"), Get_Float_Parameter ("Blue"), 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Screen_Fade_On_Enter, "Red:float,Green:float,Blue:float,Opacity:float")
class	JFW_Screen_Fade_On_Enter : public JFW_Screen_Fade_Base
{
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		Fade_Player (enterer);
	}
};


REGISTER_SCRIPT_TT (JFW_Screen_Fade_On_Exit, "Red:float,Green:float,Blue:float,Opacity:float")
class	JFW_Screen_Fade_On_Exit : public JFW_Screen_Fade_Base
{
	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		Fade_Player (exiter);
	}
};


REGISTER_SCRIPT_TT (JFW_Screen_Fade_On_Custom, "Message:int,Red:float,Green:float,Blue:float,Opacity:float")
class	JFW_Screen_Fade_On_Custom : public JFW_Screen_Fade_Base
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			Fade_Player (obj);
		}
	}
};


/*JFW_Screen_Fade_Custom_Timer

  Fades everybody's screen on a message and clears it again when the timer
  runs out.  Note the registered parameter order: Red, Blue, Green.
*/

DECLARE_SCRIPT_TT (JFW_Screen_Fade_Custom_Timer,
		"Message:int,Red:float,Blue:float,Green:float,Opacity:float,Time:float,TimerNum:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
		ScriptEngine::Set_Screen_Fade_Color (Get_Float_Parameter ("Red"),
				Get_Float_Parameter ("Green"), Get_Float_Parameter ("Blue"), 0);
		ScriptEngine::Set_Screen_Fade_Opacity (Get_Float_Parameter ("Opacity"), 0);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == Get_Int_Parameter ("TimerNum")) {
			ScriptEngine::Set_Screen_Fade_Color (0, 0, 0, 0);
			ScriptEngine::Set_Screen_Fade_Opacity (0, 0);
		}
	}
};


/*JFW_Show_Info_Texture

  Covers the screen with a picture for a while, then takes it away and removes
  itself.
*/

DECLARE_SCRIPT_TT (JFW_Show_Info_Texture, "Time:float,TimerNum:int,Texture:string")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Set_Info_Texture (obj, Get_Parameter ("Texture"));
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		ScriptEngine::Clear_Info_Texture (obj);
		Destroy_Script();
	}
};


DECLARE_SCRIPT_TT (JFW_Set_Info_Texture, "Message:int,Texture:string")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Set_Info_Texture (obj, Get_Parameter ("Texture"));
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Clear_Info_Texture, "Message:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Clear_Info_Texture (obj);
		}
	}
};


/*JFW_HUD_INI

  Re-reads the HUD layout for whoever this script is attached to.
*/

DECLARE_SCRIPT_TT (JFW_HUD_INI, "HUDINI:string")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Load_New_HUD_INI (obj, Get_Parameter ("HUDINI"));
	}
};


/*JFW_Change_Radar_Map

  Swaps the radar picture and where the level sits on it.
*/

DECLARE_SCRIPT_TT (JFW_Change_Radar_Map, "Scale:float,OffsetX:float,OffsetY:float,Texture:string")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Change_Radar_Map (Get_Float_Parameter ("Scale"),
				Get_Float_Parameter ("OffsetX"), Get_Float_Parameter ("OffsetY"),
				Get_Parameter ("Texture"));
		Destroy_Script();
	}
};


/******************************************************************************
*
*     Purchase entries
*
*     Eight scripts: hide or show one preset on one side's purchase terminal,
*     triggered on creation, on death, on a message, or on a timer.
*
******************************************************************************/

class	JFW_PT_Visibility_Base : public ScriptImpClass
{
protected:

	//	True hides the entry, false shows it.
	virtual bool	Hiding (void) const = 0;

	void	Apply (void)
	{
		ScriptEngine::Hide_Preset_By_Name (Get_Int_Parameter ("Player_Type"),
				Get_Parameter ("Preset"), Hiding());
	}
};


class	JFW_PT_On_Created : public JFW_PT_Visibility_Base
{
	void	Created (GameObject * obj) override
	{
		Apply();
		Destroy_Script();
	}
};


class	JFW_PT_On_Death : public JFW_PT_Visibility_Base
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Apply();
		Destroy_Script();
	}
};


class	JFW_PT_On_Custom : public JFW_PT_Visibility_Base
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			Apply();
		}
	}
};


class	JFW_PT_On_Timer : public JFW_PT_Visibility_Base
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		Apply();
		Destroy_Script();
	}
};


REGISTER_SCRIPT_TT (JFW_PT_Hide, "Player_Type:int,Preset:string")
class	JFW_PT_Hide : public JFW_PT_On_Created
{
	bool	Hiding (void) const override	{ return true; }
};


REGISTER_SCRIPT_TT (JFW_PT_Hide_Death, "Player_Type:int,Preset:string")
class	JFW_PT_Hide_Death : public JFW_PT_On_Death
{
	bool	Hiding (void) const override	{ return true; }
};


REGISTER_SCRIPT_TT (JFW_PT_Hide_Custom, "Player_Type:int,Preset:string,Message:int")
class	JFW_PT_Hide_Custom : public JFW_PT_On_Custom
{
	bool	Hiding (void) const override	{ return true; }
};


REGISTER_SCRIPT_TT (JFW_PT_Hide_Timer, "Player_Type:int,Preset:string,Time:float")
class	JFW_PT_Hide_Timer : public JFW_PT_On_Timer
{
	bool	Hiding (void) const override	{ return true; }
};


REGISTER_SCRIPT_TT (JFW_PT_Show, "Player_Type:int,Preset:string")
class	JFW_PT_Show : public JFW_PT_On_Created
{
	bool	Hiding (void) const override	{ return false; }
};


REGISTER_SCRIPT_TT (JFW_PT_Show_Death, "Player_Type:int,Preset:string")
class	JFW_PT_Show_Death : public JFW_PT_On_Death
{
	bool	Hiding (void) const override	{ return false; }
};


REGISTER_SCRIPT_TT (JFW_PT_Show_Custom, "Player_Type:int,Preset:string,Message:int")
class	JFW_PT_Show_Custom : public JFW_PT_On_Custom
{
	bool	Hiding (void) const override	{ return false; }
};


REGISTER_SCRIPT_TT (JFW_PT_Show_Timer, "Player_Type:int,Preset:string,Time:float")
class	JFW_PT_Show_Timer : public JFW_PT_On_Timer
{
	bool	Hiding (void) const override	{ return false; }
};


/******************************************************************************
*
*     The tech level
*
*     Three scripts, one call: move the tech level on creation, on a message,
*     or on a timer with an announcement.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Tech_Level_Startup, "Tech_Level:int")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Set_Tech_Level (Get_Int_Parameter ("Tech_Level"));
		Destroy_Script();
	}
};


DECLARE_SCRIPT_TT (JFW_Tech_Level_Custom, "Message:int,Tech_Level:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Set_Tech_Level (Get_Int_Parameter ("Tech_Level"));
			Destroy_Script();
		}
	}
};


/*
**	Note the registered parameter order: Red, Blue, Green.
*/
DECLARE_SCRIPT_TT (JFW_Tech_Level_Timer,
		"Display_Message:string,Red:int,Blue:int,Green:int,Sound:string,Time:float,Tech_Level:int")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != 1) {
			return ;
		}

		ScriptEngine::Create_2D_Sound (Get_Parameter ("Sound"));
		ScriptEngine::Send_Message (Get_Int_Parameter ("Red"), Get_Int_Parameter ("Green"),
				Get_Int_Parameter ("Blue"), Get_Parameter ("Display_Message"));
		ScriptEngine::Set_Tech_Level (Get_Int_Parameter ("Tech_Level"));
		Destroy_Script();
	}
};


/******************************************************************************
*
*     Game hints
*
*     Six scripts, two calls: the popup that explains something the first time
*     a player meets it.  A soldier is told five seconds after spawning, a
*     vehicle five seconds after somebody buys it, and a zone tells whoever
*     walks in.
*
******************************************************************************/

class	JFW_Game_Hint_Base : public ScriptImpClass
{
protected:

	//	Whether this hint carries a picture.
	virtual bool	Has_Image (void) const	{ return false; }

	void	Show_To (GameObject * player)
	{
		if (player == nullptr) {
			return ;
		}

		if (Has_Image()) {
			ScriptEngine::Display_Game_Hint_Image (player, Get_Int_Parameter ("EventID"),
					Get_Parameter ("Sound"), Get_Int_Parameter ("TitleID"),
					Get_Int_Parameter ("TextID"), Get_Int_Parameter ("TextID2"),
					Get_Int_Parameter ("TextID3"), Get_Parameter ("Texture"));
		} else {
			ScriptEngine::Display_Game_Hint (player, Get_Int_Parameter ("EventID"),
					Get_Parameter ("Sound"), Get_Int_Parameter ("TitleID"),
					Get_Int_Parameter ("TextID"), Get_Int_Parameter ("TextID2"),
					Get_Int_Parameter ("TextID3"));
		}
	}
};


/*
**	The soldier the script is attached to, five seconds after it appears.
*/
class	JFW_Game_Hint_Soldier_Base : public JFW_Game_Hint_Base
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, 5, 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		Show_To (obj);
	}
};


/*
**	Whoever bought the vehicle, five seconds after they did.  The buyer's
**	object ID arrives as the parameter of the ownership event and is used as
**	the timer number, which is how the donor carried it across the wait.
*/
class	JFW_Game_Hint_Vehicle_Base : public JFW_Game_Hint_Base
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_OWNER) {
			ScriptEngine::Start_Timer (obj, this, 5, (int)param);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		Show_To (ScriptEngine::Find_Object (number));
	}
};


/*
**	Whoever of the named side walks into the zone.
*/
class	JFW_Game_Hint_Zone_Base : public JFW_Game_Hint_Base
{
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) {
			Show_To (enterer);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Game_Hint_Soldier,
		"EventID:int,Sound:string,TitleID:int,TextID:int,TextID2:int,TextID3:int")
class	JFW_Game_Hint_Soldier : public JFW_Game_Hint_Soldier_Base	{ };


REGISTER_SCRIPT_TT (JFW_Game_Hint_Vehicle,
		"EventID:int,Sound:string,TitleID:int,TextID:int,TextID2:int,TextID3:int")
class	JFW_Game_Hint_Vehicle : public JFW_Game_Hint_Vehicle_Base	{ };


REGISTER_SCRIPT_TT (JFW_Game_Hint_Zone,
		"Player_Type:int,EventID:int,Sound:string,TitleID:int,TextID:int,TextID2:int,TextID3:int")
class	JFW_Game_Hint_Zone : public JFW_Game_Hint_Zone_Base	{ };


REGISTER_SCRIPT_TT (JFW_Game_Hint_Image_Soldier,
		"EventID:int,Sound:string,TitleID:int,TextID:int,TextID2:int,TextID3:int,Texture:string")
class	JFW_Game_Hint_Image_Soldier : public JFW_Game_Hint_Soldier_Base
{
	bool	Has_Image (void) const override	{ return true; }
};


REGISTER_SCRIPT_TT (JFW_Game_Hint_Image_Vehicle,
		"EventID:int,Sound:string,TitleID:int,TextID:int,TextID2:int,TextID3:int,Texture:string")
class	JFW_Game_Hint_Image_Vehicle : public JFW_Game_Hint_Vehicle_Base
{
	bool	Has_Image (void) const override	{ return true; }
};


REGISTER_SCRIPT_TT (JFW_Game_Hint_Image_Zone,
		"Player_Type:int,EventID:int,Sound:string,TitleID:int,TextID:int,TextID2:int,TextID3:int,Texture:string")
class	JFW_Game_Hint_Image_Zone : public JFW_Game_Hint_Zone_Base
{
	bool	Has_Image (void) const override	{ return true; }
};


/******************************************************************************
*
*     A tail of one-idea scripts
*
******************************************************************************/

/*JFW_Power_Off

  Cuts or restores a building's power on two messages.
*/

DECLARE_SCRIPT_TT (JFW_Power_Off, "Message_Off:int,Message_On:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message_Off")) {
			ScriptEngine::Set_Building_Power (obj, false);
		}

		if (type == Get_Int_Parameter ("Message_On")) {
			ScriptEngine::Set_Building_Power (obj, true);
		}
	}
};


/*JFW_Stealthable_Object

  Cloaks the object for a moment on a message, and keeps it cloaked as long as
  the messages keep coming.  The two timers overlap deliberately: the first
  marks the cloak as stale, the second uncloaks unless a fresh message has
  come in between.
*/

DECLARE_SCRIPT_TT (JFW_Stealthable_Object, "Message:int")
{
	bool	Fresh;

	void	Created (GameObject * obj) override
	{
		Fresh = false;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		ScriptEngine::Enable_Stealth (obj, true);
		ScriptEngine::Start_Timer (obj, this, 2.80f, 1);
		ScriptEngine::Start_Timer (obj, this, 3.20f, 2);
		Fresh = true;
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {
			Fresh = false;
		}

		if ((number == 2) && !Fresh) {
			ScriptEngine::Enable_Stealth (obj, false);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Fresh, 1);
	}

public:

	JFW_Stealthable_Object (void) : Fresh (false)	{ }
};


/*JFW_Object_Counter

  Counts objects up and down on two messages and destroys the sender once the
  count reaches a limit.
*/

DECLARE_SCRIPT_TT (JFW_Object_Counter, "Count:int,IncrementMessage:int,DecrementMessage:int")
{
	int	Count;

	void	Created (GameObject * obj) override
	{
		Count = 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("IncrementMessage")) {
			Count ++;

			if (Count >= Get_Int_Parameter ("Count")) {
				ScriptEngine::Destroy_Object (sender);
			}
		}

		if (type == Get_Int_Parameter ("DecrementMessage")) {
			Count --;
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Count, 1);
	}

public:

	JFW_Object_Counter (void) : Count (0)	{ }
};


/*JFW_Change_Spawn_Character

  Changes what one side spawns as, from here on.  Anybody already standing
  there is changed too, but only in the first moment of the match -- once the
  game has been running, changing a live player out from under them is not
  what the mapper meant.

  Registered under the donor's own misspelling of its name.
*/

REGISTER_SCRIPT_TT_NAMED (JFW_Change_Spawn_Character, "JFW_Change_Spawn_Characher",
		"Player_Type:int,Character:string")
class	JFW_Change_Spawn_Character : public ScriptImpClass
{
	void	Created (GameObject * obj) override
	{
		int				player_type	= Get_Int_Parameter ("Player_Type");
		const char *	character	= Get_Parameter ("Character");

		if (ScriptEngine::Get_Game_Duration_Seconds() == 0) {

			SList<SoldierGameObj> *	list = GameObjManager::Get_Star_Game_Obj_List();
			for (SLNode<SoldierGameObj> * node = list->Head(); node != nullptr; node = node->Next()) {

				SoldierGameObj * soldier = node->Data();
				if ((soldier != nullptr) && (soldier->Get_Player_Type() == player_type)) {
					ScriptEngine::Change_Character (soldier, character);
				}
			}
		}

		if (player_type == PLAYERTYPE_NOD) {
			ScriptEngine::Set_Nod_Soldier_Name (character);
		} else {
			ScriptEngine::Set_GDI_Soldier_Name (character);
		}

		Destroy_Script();
	}
};


/*JFW_Change_Character_Created

  Changes whatever this is attached to into another character preset.
*/

DECLARE_SCRIPT_TT (JFW_Change_Character_Created, "Character:string")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Change_Character (obj, Get_Parameter ("Character"));
		Destroy_Script();
	}
};


/*JFW_Spawn_Object_Created

  Makes one object where this one stands, and removes itself.
*/

DECLARE_SCRIPT_TT (JFW_Spawn_Object_Created, "Object:string")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Create_Object (Get_Parameter ("Object"), ScriptEngine::Get_Position (obj));
		Destroy_Script();
	}
};


/*JFW_Change_Model_Created

  Picks one of up to five models at random.  A preset with none of the five
  filled in keeps the model it already has.
*/

DECLARE_SCRIPT_TT (JFW_Change_Model_Created, "Model1:string,Model2:string,Model3:string,Model4:string,Model5:string")
{
	void	Created (GameObject * obj) override
	{
		static const char * const	NAMES[]	= { "Model1", "Model2", "Model3", "Model4", "Model5" };

		const char *	choices[5]	= { nullptr, nullptr, nullptr, nullptr, nullptr };
		int				count			= 0;

		for (int index = 0; index < 5; index ++) {
			const char * model = Get_Parameter (NAMES[index]);
			if ((model != nullptr) && (model[0] != 0)) {
				choices[count ++] = model;
			}
		}

		if (count > 0) {
			ScriptEngine::Set_Model (obj, choices[ScriptEngine::Get_Random_Int (0, count)]);
		}

		Destroy_Script();
	}
};


/*JFW_Credit_Grant

  Hands both sides the same money every so often, forever.
*/

DECLARE_SCRIPT_TT (JFW_Credit_Grant, "Credits:int,Delay:float")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Delay"), 667);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != 667) {
			return ;
		}

		float money = (float)Get_Int_Parameter ("Credits");
		ScriptEngine::Give_Money (ScriptEngine::Find_Smart_Object_By_Team (PLAYERTYPE_NOD), money, true);
		ScriptEngine::Give_Money (ScriptEngine::Find_Smart_Object_By_Team (PLAYERTYPE_GDI), money, true);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Delay"), 667);
	}
};


/*JFW_Points_Custom

  Hands out points on a message, to the object or to its whole side.
*/

DECLARE_SCRIPT_TT (JFW_Points_Custom, "Message:int,Points:int,Team:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Give_Points (obj, (float)Get_Int_Parameter ("Points"),
					Get_Bool_Parameter ("Team"));
		}
	}
};


/*JFW_Display_String_Custom

  Writes a line on one side's screens and plays them a sound.
*/

DECLARE_SCRIPT_TT (JFW_Display_String_Custom,
		"Message:int,Team:int,String:string,Sound:string,Red:int,Green:int,Blue:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		int team = Get_Int_Parameter ("Team");
		ScriptEngine::Send_Message_Team (team, Get_Int_Parameter ("Red"),
				Get_Int_Parameter ("Green"), Get_Int_Parameter ("Blue"),
				Get_Parameter ("String"));
		ScriptEngine::Create_2D_Sound_Team (team, Get_Parameter ("Sound"));
	}
};


/*JFW_Set_Time_Custom / JFW_Set_Time_Limit_Custom

  Move the clock, or the limit the clock counts down from.
*/

DECLARE_SCRIPT_TT (JFW_Set_Time_Custom, "Message:int,Time:float")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Set_Time_Remaining_Seconds (Get_Float_Parameter ("Time"));
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Set_Time_Limit_Custom, "Message:int,TimeLimit:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Set_Time_Limit_Minutes (Get_Int_Parameter ("TimeLimit"));
		}
	}
};


/*JFW_Radar_Low_Power

  Attached to a power plant: when it dies, the side loses its radar -- but
  only if it had a communications centre to lose it from.
*/

DECLARE_SCRIPT_TT (JFW_Radar_Low_Power, "")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		int team = ScriptEngine::Get_Player_Type (obj);

		GameObject * com_center = ScriptEngine::Find_Building_By_Type (team,
				BuildingConstants::TYPE_COM_CENTER);
		if ((com_center != nullptr) && !ScriptEngine::Is_Building_Dead (com_center)) {
			ScriptEngine::Enable_Base_Radar (team, false);
		}

		Destroy_Script();
	}
};


/*JFW_Radar_Disable_Death

  The plainer form: whatever this is attached to dies, the side loses radar.
*/

DECLARE_SCRIPT_TT (JFW_Radar_Disable_Death, "")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		ScriptEngine::Enable_Base_Radar (ScriptEngine::Get_Player_Type (obj), false);
		Destroy_Script();
	}
};


/*JFW_Forward_Custom_Object

  Passes every message it receives on to another object.
*/

DECLARE_SCRIPT_TT (JFW_Forward_Custom_Object, "Object_ID:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		GameObject * target = ScriptEngine::Find_Object (Get_Int_Parameter ("Object_ID"));
		if (target != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, target, type, param, 0);
		}
	}
};


/*JFW_Death_Send_Custom_Self

  Sends itself a message when it dies, so another script on the same object
  can hear about it.
*/

DECLARE_SCRIPT_TT (JFW_Death_Send_Custom_Self, "Message:int")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("Message"), 0, 0);
		Destroy_Script();
	}
};


/*JFW_Send_Message_Preset_Death

  Tells the first object of a named preset, anywhere on the map, that this one
  has died.
*/

DECLARE_SCRIPT_TT (JFW_Send_Message_Preset_Death, "Preset:string,Message:int")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		GameObject * receiver = ScriptEngine::Find_Object_By_Preset (2, Get_Parameter ("Preset"));
		if (receiver != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, receiver, Get_Int_Parameter ("Message"), 0, 0);
		}
	}
};


/*JMG_Send_Custom_To_Self_On_Timer

  Sends itself a message on a timer.  Repeat 1 keeps going, -1 removes the
  script after the first, anything else fires once and stays.
*/

DECLARE_SCRIPT_TT (JMG_Send_Custom_To_Self_On_Timer,
		"Message:int,Param:int,Time:float,Timer_Number:int,Repeat:int")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("Timer_Number"));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("Timer_Number")) {
			return ;
		}

		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("Message"),
				Get_Int_Parameter ("Param"), 0);

		int repeat = Get_Int_Parameter ("Repeat");
		if (repeat == 1) {
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), number);
		} else if (repeat == -1) {
			Destroy_Script();
		}
	}
};


/*JFW_Warhead_Custom

  Sends itself a message when it is hit by a named warhead.
*/

DECLARE_SCRIPT_TT (JFW_Warhead_Custom, "Message:int,Warhead:string")
{
	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		unsigned int warhead = ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Warhead"));
		if (warhead == ScriptEngine::Get_Damage_Warhead()) {
			ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("Message"), 0, 0);
		}
	}
};


/*JFW_Spy_Disguise_Target

  Hit this with the named warhead and the shooter takes on its appearance.
  An empty Model parameter means "whatever this object looks like now".
*/

DECLARE_SCRIPT_TT (JFW_Spy_Disguise_Target, "Model:string,Warhead:string")
{
	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		unsigned int warhead = ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Warhead"));
		if (warhead != ScriptEngine::Get_Damage_Warhead()) {
			return ;
		}

		const char * model = Get_Parameter ("Model");
		if ((model == nullptr) || (model[0] == 0)) {
			model = ScriptEngine::Get_Model (obj);
		}

		if ((damager != nullptr) && (model != nullptr) && (model[0] != 0)) {
			ScriptEngine::Set_Model (damager, model);
		}
	}
};


/*JFW_Airstrike_Cinematic

  Runs a cinematic script from an invisible object placed where this one
  stands and facing the way it faces.
*/

DECLARE_SCRIPT_TT (JFW_Airstrike_Cinematic, "Script_Name:string,Message:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		GameObject * object = ScriptEngine::Create_Object ("Invisible_Object",
				ScriptEngine::Get_Position (obj));
		if (object == nullptr) {
			return ;
		}

		ScriptEngine::Set_Facing (object, ScriptEngine::Get_Facing (obj));
		ScriptEngine::Attach_Script (object, "JFW_Cinematic", Get_Parameter ("Script_Name"));
	}
};


/*JFW_Vehicle_Crate

  A pickup that hands whoever takes it a vehicle, and puts them in it.
*/

DECLARE_SCRIPT_TT (JFW_Vehicle_Crate, "Preset:string,Offset:vector3")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != CUSTOM_EVENT_POWERUP_GRANTED) {
			return ;
		}

		Vector3 position = ScriptEngine::Get_Position (obj) + Get_Vector3_Parameter ("Offset");

		GameObject * vehicle = ScriptEngine::Create_Object (Get_Parameter ("Preset"), position);
		if (vehicle != nullptr) {
			ScriptEngine::Force_Vehicle_Entry (sender, vehicle);
		}
	}
};


/*JFW_Custom_Create_Object_At_Bone

  Attaches an object to one of this object's bones on a message, and takes it
  away again on death or after a while.  A soldier sitting in a vehicle gets
  nothing: the bone is not where they are.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Create_Object_At_Bone, "Message:int,Object:string,Bone:string,Time:float")
{
	int	AttachedID;

	void	Created (GameObject * obj) override
	{
		AttachedID = 0;
	}

	void	Remove (void)
	{
		Destroy_By_ID (AttachedID);
		AttachedID = 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if ((type != Get_Int_Parameter ("Message")) || (AttachedID != 0)) {
			return ;
		}

		SoldierGameObj * soldier = As_Soldier (obj);
		if ((soldier != nullptr) && soldier->Is_In_Vehicle()) {
			return ;
		}

		GameObject * attached = ScriptEngine::Create_Object_At_Bone (obj,
				Get_Parameter ("Object"), Get_Parameter ("Bone"));
		if (attached == nullptr) {
			return ;
		}

		ScriptEngine::Attach_To_Object_Bone (attached, obj, Get_Parameter ("Bone"));
		ScriptEngine::Set_Player_Type (attached, ScriptEngine::Get_Player_Type (obj));
		AttachedID = ScriptEngine::Get_ID (attached);

		float time = Get_Float_Parameter ("Time");
		if (time != 0) {
			ScriptEngine::Start_Timer (obj, this, time, 1);
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override		{ Remove(); }
	void	Destroyed (GameObject * obj) override							{ Remove(); }
	void	Timer_Expired (GameObject * obj, int number) override		{ Remove(); }

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (AttachedID, 1);
	}

public:

	JFW_Custom_Create_Object_At_Bone (void) : AttachedID (0)	{ }
};


/******************************************************************************
*
*     Switching a base off a piece at a time
*
*     Nine scripts, one shape: a message switches something off, another
*     switches it back on, and the two nest so that two overlapping effects do
*     not cancel one another when the first of them ends.
*
*     The donor let the count run below zero.  An enable with no matching
*     disable then left it at -1, and the next disable moved it to zero
*     without switching anything off.
*
******************************************************************************/

class	JFW_Nested_Disable_Base : public ScriptImpClass
{
	int	Depth;

protected:

	//	Switch the thing this script owns off or back on.
	virtual void	Set_Disabled (GameObject * obj, bool disabled) = 0;

	//	Whether this script has anything to act on at all.
	virtual bool	Applies (GameObject * obj)	{ return true; }

	void	Created (GameObject * obj) override
	{
		Depth = 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (!Applies (obj)) {
			return ;
		}

		if (type == Get_Int_Parameter ("Disable_Custom")) {
			if (Depth == 0) {
				Set_Disabled (obj, true);
			}
			Depth ++;
		}

		if (type == Get_Int_Parameter ("Enable_Custom")) {
			if (Depth > 0) {
				Depth --;
				if (Depth == 0) {
					Set_Disabled (obj, false);
				}
			}
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Destroy_Script();
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Depth, 1);
	}

public:

	JFW_Nested_Disable_Base (void) : Depth (0)	{ }
};


/*JFW_Preset_Disable

  Greys one purchase entry out for the side that owns this object.
*/

REGISTER_SCRIPT_TT (JFW_Preset_Disable, "Preset:string,Disable_Custom:int,Enable_Custom:int")
class	JFW_Preset_Disable : public JFW_Nested_Disable_Base
{
	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		ScriptEngine::Disable_Preset_By_Name (ScriptEngine::Get_Player_Type (obj),
				Get_Parameter ("Preset"), disabled);
	}
};


/*JFW_Power_Disable

  Cuts the power to a whole base.
*/

REGISTER_SCRIPT_TT (JFW_Power_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_Power_Disable : public JFW_Nested_Disable_Base
{
	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		ScriptEngine::Power_Base (ScriptEngine::Get_Player_Type (obj), !disabled);
	}
};


/*JFW_Radar_Disable

  Blanks a side's radar.
*/

REGISTER_SCRIPT_TT (JFW_Radar_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_Radar_Disable : public JFW_Nested_Disable_Base
{
	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		ScriptEngine::Enable_Base_Radar (ScriptEngine::Get_Player_Type (obj), !disabled);
	}
};


/*JFW_Conyard_Disable

  Marks a construction yard as spied, which is what stops it rebuilding.
*/

REGISTER_SCRIPT_TT (JFW_Conyard_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_Conyard_Disable : public JFW_Nested_Disable_Base
{
	ConstructionYardGameObj *	Conyard (GameObject * obj)
	{
		BuildingGameObj * building = As_Building (obj);
		return (building != nullptr) ? building->As_ConstructionYardGameObj() : nullptr;
	}

	bool	Applies (GameObject * obj) override	{ return Conyard (obj) != nullptr; }

	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		Conyard (obj)->Set_Spy (disabled);
	}
};


/*JFW_Refinery_Disable

  Stops a refinery asking for a replacement harvester.  The one it already has
  keeps working.
*/

REGISTER_SCRIPT_TT (JFW_Refinery_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_Refinery_Disable : public JFW_Nested_Disable_Base
{
	RefineryGameObj *	Refinery (GameObject * obj)
	{
		BuildingGameObj * building = As_Building (obj);
		return (building != nullptr) ? building->As_RefineryGameObj() : nullptr;
	}

	bool	Applies (GameObject * obj) override	{ return Refinery (obj) != nullptr; }

	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		Refinery (obj)->Set_Harvester_Spawn_Blocked (disabled);
	}
};


/*
**	The three factory scripts: shut the building itself and grey out whichever
**	purchase pages that kind of building stands behind.
*/
class	JFW_Factory_Disable_Base : public JFW_Nested_Disable_Base
{
protected:

	//	The building type whose purchase pages go with this factory.
	virtual BuildingConstants::BuildingType	Factory_Type (void) const = 0;

	VehicleFactoryGameObj *	Factory (GameObject * obj)
	{
		BuildingGameObj * building = As_Building (obj);
		if ((building == nullptr) || (building->Get_Definition().Get_Type() != Factory_Type())) {
			return nullptr;
		}
		return building->As_VehicleFactoryGameObj();
	}

	bool	Applies (GameObject * obj) override	{ return Factory (obj) != nullptr; }

	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		Factory (obj)->Set_Disabled (disabled);

		PurchaseAvailabilityClass::Set_By_Factory (ScriptEngine::Get_Player_Type (obj),
				Factory_Type(), PurchaseAvailabilityClass::FLAG_DISABLED, disabled);
	}
};


REGISTER_SCRIPT_TT (JFW_VehicleFactory_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_VehicleFactory_Disable : public JFW_Factory_Disable_Base
{
	BuildingConstants::BuildingType	Factory_Type (void) const override
	{
		return BuildingConstants::TYPE_VEHICLE_FACTORY;
	}
};


/*
**	The air factory here is the helipad.  No purchase page stands behind it in
**	this tree, so Set_By_Factory finds nothing to grey out and the building
**	itself is what closes.
*/
REGISTER_SCRIPT_TT (JFW_AirFactory_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_AirFactory_Disable : public JFW_Factory_Disable_Base
{
	BuildingConstants::BuildingType	Factory_Type (void) const override
	{
		return BuildingConstants::TYPE_HELIPAD;
	}
};


/*
**	There is no naval building type here; a naval yard in this tree is a
**	vehicle factory, and that is what this closes.
*/
REGISTER_SCRIPT_TT (JFW_NavalFactory_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_NavalFactory_Disable : public JFW_Factory_Disable_Base
{
	BuildingConstants::BuildingType	Factory_Type (void) const override
	{
		return BuildingConstants::TYPE_VEHICLE_FACTORY;
	}
};


/*JFW_SoldierFactory_Disable

  Stops a barracks or hand of Nod producing anybody.
*/

REGISTER_SCRIPT_TT (JFW_SoldierFactory_Disable, "Disable_Custom:int,Enable_Custom:int")
class	JFW_SoldierFactory_Disable : public JFW_Nested_Disable_Base
{
	bool	Applies (GameObject * obj) override
	{
		BuildingGameObj * building = As_Building (obj);
		return (building != nullptr) && (building->As_SoldierFactoryGameObj() != nullptr);
	}

	void	Set_Disabled (GameObject * obj, bool disabled) override
	{
		ScriptEngine::Set_Can_Generate_Soldiers (ScriptEngine::Get_Player_Type (obj), !disabled);
	}
};


/******************************************************************************
*
*     The engineer hack
*
*     A second pool of health that only one warhead touches: a friendly
*     engineer repairs it, an enemy one wears it down, and it recovers on its
*     own between visits.  Crossing a threshold on the way down takes the
*     building offline and crossing it on the way up brings it back.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Engineer_Hack,
		"Hack_Warhead:string,Hack_Health:float,Hack_Damage:float,Hack_Repair:float,Hack_Restore_Timer:float,Hack_Restore_Amount:float,Hack_Offline_Amount:float,Hack_Offline_Custom:int,Hack_Online_Custom:int,Hack_Indicator_Preset:string")
{
	unsigned int	Warhead;
	int				IndicatorID;
	float				HackHealth;

	void	Announce (GameObject * obj, int message)
	{
		ScriptEngine::Send_Custom_Event (obj, obj, message, 0, 0);

		GameObject * indicator = ScriptEngine::Find_Object (IndicatorID);
		if (indicator != nullptr) {
			ScriptEngine::Send_Custom_Event (indicator, obj, message, 0, 0);
		}
	}

	void	Hurt (GameObject * obj, float damage)
	{
		float	before	= HackHealth;
		float	offline	= Get_Float_Parameter ("Hack_Offline_Amount");

		HackHealth -= damage;

		if (HackHealth <= 0) {
			ScriptEngine::Apply_Damage (obj, 99999, "Death", nullptr);
		}

		if ((before > offline) && (HackHealth <= offline)) {
			Announce (obj, Get_Int_Parameter ("Hack_Offline_Custom"));
		}
	}

	void	Mend (GameObject * obj, float repair)
	{
		float	before	= HackHealth;
		float	offline	= Get_Float_Parameter ("Hack_Offline_Amount");
		float	most		= Get_Float_Parameter ("Hack_Health");

		HackHealth += repair;
		if (HackHealth > most) {
			HackHealth = most;
		}

		if ((before <= offline) && (HackHealth > offline)) {
			Announce (obj, Get_Int_Parameter ("Hack_Online_Custom"));
		}
	}

	void	Created (GameObject * obj) override
	{
		Warhead		= ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Hack_Warhead"));
		HackHealth	= Get_Float_Parameter ("Hack_Health");

		GameObject * indicator = ScriptEngine::Find_Nearest_Preset (ScriptEngine::Get_Position (obj),
				Get_Parameter ("Hack_Indicator_Preset"));
		IndicatorID = (indicator != nullptr) ? ScriptEngine::Get_ID (indicator) : 0;

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Hack_Restore_Timer"), 1);
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		if (Warhead != ScriptEngine::Get_Damage_Warhead()) {
			return ;
		}

		if (ScriptEngine::Get_Player_Type (obj) == ScriptEngine::Get_Player_Type (damager)) {
			Mend (obj, Get_Float_Parameter ("Hack_Repair"));
		} else {
			Hurt (obj, Get_Float_Parameter ("Hack_Damage"));
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		Mend (obj, Get_Float_Parameter ("Hack_Restore_Amount"));
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Hack_Restore_Timer"), 1);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Destroy_Script();
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (IndicatorID, 1);
		SAVE_VARIABLE (HackHealth, 2);
	}

public:

	JFW_Engineer_Hack (void) : Warhead (0), IndicatorID (0), HackHealth (0.0f)	{ }
};


/******************************************************************************
*
*     EMP
*
*     A warhead that stops a vehicle dead for a while -- no engine, no guns,
*     no cloak -- or freezes a soldier where they stand.  Two scripts do it:
*     one plays an animation while it lasts, the other attaches a model.
*
******************************************************************************/

class	JFW_EMP_Base : public ScriptImpClass
{
protected:

	unsigned int	Warhead;
	bool				StealthWas;

	//	What to show while the vehicle is down, and what to take away after.
	virtual void	Begin_Effect (GameObject * obj)	{ }
	virtual void	End_Effect (GameObject * obj)		{ }

	void	Created (GameObject * obj) override
	{
		Warhead = ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Warhead"));
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		if ((Warhead != ScriptEngine::Get_Damage_Warhead()) || _EmpDisabled) {
			return ;
		}

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (	(vehicle != nullptr) && vehicle->Get_Is_Scripts_Visible()
				&& vehicle->Get_Scripts_Can_Fire() && vehicle->Can_Drive())
		{
			ScriptEngine::Enable_Engine (obj, false);
			vehicle->Set_Can_Drive (false);
			vehicle->Set_Scripts_Can_Fire (false);

			//
			//	Put the cloak back the way it was, rather than switching it on
			//	unconditionally as the donor did -- which handed a permanent
			//	cloak to anything an EMP had touched.
			//
			StealthWas = vehicle->Is_Stealth_Enabled();
			ScriptEngine::Enable_Stealth (obj, false);

			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
			Begin_Effect (obj);

			obj->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
		}

		SoldierGameObj * soldier = As_Soldier (obj);
		if ((soldier != nullptr) && !soldier->Is_Frozen() && soldier->Is_Visible()) {
			soldier->Set_Freeze (true);
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (vehicle != nullptr) {

			End_Effect (obj);

			obj->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
			vehicle->Set_Can_Drive (true);
			ScriptEngine::Enable_Stealth (obj, StealthWas);
			ScriptEngine::Enable_Engine (obj, true);
			vehicle->Set_Scripts_Can_Fire (true);
		}

		SoldierGameObj * soldier = As_Soldier (obj);
		if (soldier != nullptr) {
			soldier->Set_Freeze (false);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (StealthWas, 1);
	}

public:

	JFW_EMP_Base (void) : Warhead (0), StealthWas (false)	{ }
};


/*JFW_EMP

  Plays a looping animation on the vehicle while it is down, and puts the
  preset's own animation back afterwards.
*/

REGISTER_SCRIPT_TT (JFW_EMP, "Warhead:string,Time:float,Animation:string")
class	JFW_EMP : public JFW_EMP_Base
{
	void	Begin_Effect (GameObject * obj) override
	{
		const char * animation = Get_Parameter ("Animation");
		if ((animation != nullptr) && (animation[0] != 0)) {
			ScriptEngine::Set_Animation (obj, animation, true, nullptr, 0, -1, false);
		}
	}

	void	End_Effect (GameObject * obj) override
	{
		//	An empty name is how the engine is told to stop animating.
		ScriptEngine::Set_Animation (obj, "", false, nullptr, 0, -1, false);

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (vehicle != nullptr) {
			const StringClass & animation = vehicle->Get_Definition().Get_Animation();
			if (!animation.Is_Empty()) {
				ScriptEngine::Set_Animation (obj, animation, true, nullptr, 0, -1, false);
			}
		}
	}
};


/*JFW_EMP_Indicator

  Attaches a model to a bone while the vehicle is down, and destroys it after.
*/

REGISTER_SCRIPT_TT (JFW_EMP_Indicator, "Warhead:string,Time:float,Preset:string,Bone:string")
class	JFW_EMP_Indicator : public JFW_EMP_Base
{
	int	IndicatorID;

	void	Created (GameObject * obj) override
	{
		IndicatorID = 0;
		JFW_EMP_Base::Created (obj);
	}

	void	Begin_Effect (GameObject * obj) override
	{
		GameObject * indicator = ScriptEngine::Create_Object_At_Bone (obj,
				Get_Parameter ("Preset"), Get_Parameter ("Bone"));
		if (indicator == nullptr) {
			return ;
		}

		ScriptEngine::Attach_To_Object_Bone (indicator, obj, Get_Parameter ("Bone"));
		IndicatorID = ScriptEngine::Get_ID (indicator);
	}

	void	End_Effect (GameObject * obj) override
	{
		ScriptEngine::Set_Animation (obj, "", false, nullptr, 0, -1, false);
		Destroy_By_ID (IndicatorID);
		IndicatorID = 0;
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (StealthWas, 1);
		SAVE_VARIABLE (IndicatorID, 2);
	}

public:

	JFW_EMP_Indicator (void) : IndicatorID (0)	{ }
};


/*JFW_EMP_Mine

  A mine that goes off when anything an EMP could stop walks past.  It is
  blind for a moment after it is laid so that whoever laid it can get clear.
*/

DECLARE_SCRIPT_TT (JFW_EMP_Mine, "Mine_Manager_ID:int,Explosion:string,Time:float")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (vehicle != nullptr) {
			vehicle->Set_Is_Scripts_Visible (false);
		}

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{
		if (	_EmpDisabled
				|| (	!ScriptEngine::Is_Script_Attached (enemy, "JFW_EMP")
						&& !ScriptEngine::Is_Script_Attached (enemy, "JFW_EMP_Indicator")))
		{
			return ;
		}

		bool	worth_it	= false;

		VehicleGameObj * vehicle = As_Vehicle (enemy);
		if (	(vehicle != nullptr) && vehicle->Get_Is_Scripts_Visible()
				&& vehicle->Get_Scripts_Can_Fire() && vehicle->Can_Drive())
		{
			worth_it = true;
		}

		SoldierGameObj * soldier = As_Soldier (enemy);
		if ((soldier != nullptr) && !soldier->Is_Frozen() && soldier->Is_Visible()) {
			worth_it = true;
		}

		if (worth_it) {
			ScriptEngine::Create_Explosion (Get_Parameter ("Explosion"),
					ScriptEngine::Get_Position (obj), nullptr);
			ScriptEngine::Apply_Damage (obj, 99999, "Death", nullptr);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		ScriptEngine::Enable_Enemy_Seen (obj, true);
	}

	void	Destroyed (GameObject * obj) override
	{
		GameObject * manager = ScriptEngine::Find_Object (Get_Int_Parameter ("Mine_Manager_ID"));
		if (manager != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, manager, EMP_MINE_DESTROYED, 0, 0);
		}
	}
};


/*
**	Both mine managers keep the same count and answer the same two messages.
**	They differ only in where the mine ends up: one is placed on the ground in
**	front of the layer, the other is thrown out of the muzzle.
*/
class	JFW_EMP_Mine_Manager_Base : public ScriptImpClass
{
	int	Mines;

protected:

	//	Put one mine into the world for this soldier.  Null if it could not.
	virtual GameObject *	Place_Mine (GameObject * layer) = 0;

	void	Created (GameObject * obj) override
	{
		_EmpDisabled	= false;
		Mines				= 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == EMP_MINE_DESTROYED) {

			if (Mines > 0) {
				Mines --;
			}

		} else if (type == EMP_MINE_REQUESTED) {

			if (Mines >= Get_Int_Parameter ("Mine_Limit")) {
				ScriptEngine::Give_PowerUp (sender, Get_Parameter ("Mine_Reload"), false);
				return ;
			}

			if (As_Soldier (sender) == nullptr) {
				return ;
			}

			GameObject * mine = Place_Mine (sender);
			if (mine != nullptr) {
				Mines ++;
			}
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Mines, 1);
	}

public:

	JFW_EMP_Mine_Manager_Base (void) : Mines (0)	{ }
};


REGISTER_SCRIPT_TT (JFW_EMP_Mine_Manager,
		"Mine_Preset:string,Mine_Limit:int,Mine_Reload:string,Mine_Z_Offset:float,Mine_Distance:float")
class	JFW_EMP_Mine_Manager : public JFW_EMP_Mine_Manager_Base
{
	GameObject *	Place_Mine (GameObject * layer) override
	{
		Vector3	position	= ScriptEngine::Get_Position (layer);
		float		distance	= Get_Float_Parameter ("Mine_Distance");
		float		angle		= DEG_TO_RADF (ScriptEngine::Get_Facing (layer));

		position.Z += Get_Float_Parameter ("Mine_Z_Offset");
		position.X += ::cosf (angle) * distance;
		position.Y += ::sinf (angle) * distance;

		GameObject * mine = ScriptEngine::Create_Object (Get_Parameter ("Mine_Preset"), position);
		if (mine == nullptr) {
			return nullptr;
		}

		ScriptEngine::Set_Player_Type (mine, ScriptEngine::Get_Player_Type (layer));

		PhysicalGameObj * physical = mine->As_PhysicalGameObj();
		if (physical != nullptr) {
			physical->Peek_Physical_Object()->Set_Collision_Group (TERRAIN_AND_BULLET_COLLISION_GROUP);
		}

		return mine;
	}
};


REGISTER_SCRIPT_TT (JFW_EMP_Mine_Manager_2,
		"Mine_Preset:string,Mine_Limit:int,Mine_Reload:string,Throw_Velocity:float")
class	JFW_EMP_Mine_Manager_2 : public JFW_EMP_Mine_Manager_Base
{
	GameObject *	Place_Mine (GameObject * layer) override
	{
		GameObject * mine = ScriptEngine::Create_Object (Get_Parameter ("Mine_Preset"),
				ScriptEngine::Get_Position (layer));
		if (mine == nullptr) {
			return nullptr;
		}

		ScriptEngine::Set_Player_Type (mine, ScriptEngine::Get_Player_Type (layer));

		PhysicalGameObj * physical = mine->As_PhysicalGameObj();
		ArmedGameObj *		armed		= (physical != nullptr) ? physical->As_ArmedGameObj() : nullptr;
		if ((physical == nullptr) || (armed == nullptr)) {
			return mine;
		}

		physical->Peek_Physical_Object()->Set_Collision_Group (TERRAIN_AND_BULLET_COLLISION_GROUP);

		//
		//	Stand the mine up where the muzzle is, pointing the way it points,
		//	and give it the muzzle's forward speed.
		//
		Matrix3D	muzzle		= armed->Get_Muzzle();
		Matrix3D	transform	(true);
		transform.Rotate_Z (muzzle.Get_Z_Rotation());
		transform.Set_Translation (muzzle.Get_Translation());
		physical->Peek_Physical_Object()->Set_Transform (transform);

		ScriptEngine::Set_Velocity (mine,
				muzzle.Get_X_Vector() * Get_Float_Parameter ("Throw_Velocity"));

		physical->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
		return mine;
	}
};


/*JFW_EMP_Mine_Layer

  On the soldier: shooting yourself with the named warhead asks the manager
  for a mine.
*/

DECLARE_SCRIPT_TT (JFW_EMP_Mine_Layer, "Mine_Manager_ID:int,Warhead:string")
{
	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		unsigned int warhead = ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Warhead"));
		if ((warhead != ScriptEngine::Get_Damage_Warhead()) || (damager != obj)) {
			return ;
		}

		GameObject * manager = ScriptEngine::Find_Object (Get_Int_Parameter ("Mine_Manager_ID"));
		if (manager != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, manager, EMP_MINE_REQUESTED, 0, 0);
		}
	}
};


/******************************************************************************
*
*     Three vehicles that are really their drivers
*
******************************************************************************/

/*JFW_Cyborg_Reaper

  A vehicle that is one soldier standing up: getting in seats you permanently,
  getting out kills the vehicle a second later, and either way the driver dies
  with it.
*/

DECLARE_SCRIPT_TT (JFW_Cyborg_Reaper, "")
{
	int	DriverID;

	void	Created (GameObject * obj) override
	{
		DriverID = 0;
	}

	void	Kill_Driver (void)
	{
		GameObject * driver = ScriptEngine::Find_Object (DriverID);
		if (driver != nullptr) {
			ScriptEngine::Attach_Script (driver, "RA_DriverDeath", "0");
		}
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			ScriptEngine::Start_Timer (obj, this, 1, 1);
			Kill_Driver();
		} else if (type == CUSTOM_EVENT_VEHICLE_OWNER) {
			ScriptEngine::Force_Vehicle_Entry (sender, obj);
			DriverID = ScriptEngine::Get_ID (sender);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		ScriptEngine::Apply_Damage (obj, 99999, "Death", nullptr);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Kill_Driver();
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (DriverID, 1);
	}

public:

	JFW_Cyborg_Reaper (void) : DriverID (0)	{ }
};


/*JFW_Limpet_Drone

  A vehicle nobody can see in the scripts sense, whose cloak toggles on one
  particular message.
*/

DECLARE_SCRIPT_TT (JFW_Limpet_Drone, "")
{
	bool	Stealthed;

	void	Created (GameObject * obj) override
	{
		Stealthed = false;

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (vehicle != nullptr) {
			vehicle->Set_Is_Scripts_Visible (false);
		}
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == 923572385) {
			Stealthed = !Stealthed;
			ScriptEngine::Enable_Stealth (obj, Stealthed);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Stealthed, 1);
	}

public:

	JFW_Limpet_Drone (void) : Stealthed (false)	{ }
};


/*JFW_Hunter_Seeker

  A vehicle its driver aims and then leaves: press the key and the driver is
  thrown clear, five seconds later the vehicle goes off where it stands.
  Getting out any other way just destroys it.
*/

REGISTER_SCRIPT_TT (JFW_Hunter_Seeker, "Key:string,Explosion:string")
class	JFW_Hunter_Seeker : public KeyHookScriptClass
{
	int	DriverID;

	void	Created (GameObject * obj) override
	{
		DriverID = 0;
		Install_Hook (Get_Parameter ("Key"), obj);
	}

	void	Key_Hook (void) override
	{
		GameObject * obj = Owner();
		if (obj == nullptr) {
			return ;
		}

		ScriptEngine::Start_Timer (obj, this, 5, 1);

		GameObject * driver = ScriptEngine::Find_Object (DriverID);
		if (driver != nullptr) {
			ScriptEngine::Soldier_Transition_Vehicle (driver);
		}
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			ScriptEngine::Start_Timer (obj, this, 5, 2);
		} else if (type == CUSTOM_EVENT_VEHICLE_OWNER) {
			DriverID = ScriptEngine::Get_ID (sender);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {
			ScriptEngine::Apply_Damage (obj, 99999, "Death", nullptr);
			ScriptEngine::Create_Explosion (Get_Parameter ("Explosion"),
					ScriptEngine::Get_Position (obj), nullptr);
		} else if (number == 2) {
			ScriptEngine::Apply_Damage (obj, 99999, "Death", nullptr);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (DriverID, 1);
	}

public:

	JFW_Hunter_Seeker (void) : DriverID (0)	{ }
};


/******************************************************************************
*
*     Ion storms
*
*     A storm that arrives at a random moment, stays for a random while, and
*     while it lasts takes away some list of the things a base depends on.
*     Three scripts: the first hard-wires which things, the second lets the
*     mapper choose, and the third is a storm that starts with the level and
*     never stops.
*
******************************************************************************/

class	JFW_Ion_Storm_Base : public ScriptImpClass
{
protected:

	bool	Storming;

	//	What this storm takes away.  The first storm took all of it.
	virtual bool	Disables_Emp (void)					{ return Get_Int_Parameter ("DisableEmp") != 0; }
	virtual bool	Disables_Stealth (void)				{ return true; }
	virtual bool	Disables_Radar (void)				{ return true; }
	virtual bool	Disables_Power (void)				{ return true; }
	virtual bool	Disables_Base_Defenses (void)		{ return true; }
	virtual bool	Destroys_Mines (void)				{ return Get_Int_Parameter ("DestroyMines") != 0; }

	//	Which message tells the weather script to start and to stop.
	virtual int		On_Weather_Message (void)			{ return Get_Int_Parameter ("On_Weather_Custom"); }
	virtual int		Off_Weather_Message (void)			{ return Get_Int_Parameter ("Off_Weather_Custom"); }

	//
	//	Say something to each side and write a line on every screen.
	//
	void	Announce (const char * nod_sound, const char * gdi_sound, const char * text)
	{
		ScriptEngine::Create_2D_Sound_Team (PLAYERTYPE_NOD, nod_sound);
		ScriptEngine::Create_2D_Sound_Team (PLAYERTYPE_GDI, gdi_sound);
		ScriptEngine::Send_Message (Get_Int_Parameter ("Red"), Get_Int_Parameter ("Green"),
				Get_Int_Parameter ("Blue"), text);
	}

	//
	//	Tell a side's communications centre or power plant to switch off, or
	//	back on.  Nothing happens where the building is not on the map.
	//
	void	Tell_Building (GameObject * obj, int team, int building_type, int message)
	{
		GameObject * building = ScriptEngine::Find_Building_By_Type (team, building_type);
		if (building != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, building, message, 0, 0);
		}
	}

	//
	//	Every base-defence vehicle on the map, and optionally every EMP mine.
	//	The donor addressed a null receiver here and disabled nothing; it also
	//	tested and destroyed its own controller rather than the vehicle it had
	//	reached.
	//
	void	Sweep_Vehicles (GameObject * obj, int message, bool destroy_mines)
	{
		SList<SmartGameObj> *	list = GameObjManager::Get_Smart_Game_Obj_List();

		SLNode<SmartGameObj> *	node	= list->Head();
		SLNode<SmartGameObj> *	next	= nullptr;

		for (; node != nullptr; node = next) {

			//	Read the next link first: a mine may be destroyed below.
			next = node->Next();

			VehicleGameObj * vehicle = (node->Data() != nullptr)
					? node->Data()->As_VehicleGameObj() : nullptr;
			if (vehicle == nullptr) {
				continue;
			}

			if (vehicle->Get_Definition().Get_Encyclopedia_Type() == EncyclopediaMgrClass::TYPE_BUILDING) {
				ScriptEngine::Send_Custom_Event (obj, vehicle, message, 0, 0);
			}

			if (destroy_mines && ScriptEngine::Is_Script_Attached (vehicle, "JFW_EMP_Mine")) {
				ScriptEngine::Destroy_Object (vehicle);
			}
		}
	}

	void	Begin_Storm (GameObject * obj)
	{
		Storming = true;

		if (Disables_Emp()) {
			_EmpDisabled = true;
		}

		if (Disables_Stealth()) {
			ScriptEngine::Set_Global_Stealth_Disable (true);
		}

		ScriptEngine::Send_Custom_Event (obj, obj, On_Weather_Message(), 0, 0);

		if (Disables_Radar()) {
			Tell_Building (obj, PLAYERTYPE_NOD, BuildingConstants::TYPE_COM_CENTER,
					Get_Int_Parameter ("Disable_Custom"));
			Tell_Building (obj, PLAYERTYPE_GDI, BuildingConstants::TYPE_COM_CENTER,
					Get_Int_Parameter ("Disable_Custom"));
		}

		if (Disables_Power()) {
			Tell_Building (obj, PLAYERTYPE_NOD, BuildingConstants::TYPE_POWER_PLANT,
					Get_Int_Parameter ("Disable_Custom"));
			Tell_Building (obj, PLAYERTYPE_GDI, BuildingConstants::TYPE_POWER_PLANT,
					Get_Int_Parameter ("Disable_Custom"));
		}

		Sweep_Vehicles (obj, Disables_Base_Defenses() ? CUSTOM_AI_DISABLEAI : 0, Destroys_Mines());

		//	Start the crackle on the soundtrack.
		Timer_Expired (obj, 3);

		Announce (Get_Parameter ("Announcement_Sound_Nod"),
				Get_Parameter ("Announcement_Sound_GDI"), Get_Parameter ("Announcement_String"));
	}

	void	End_Storm (GameObject * obj)
	{
		Storming = false;

		ScriptEngine::Set_Global_Stealth_Disable (false);

		if (Disables_Emp()) {
			_EmpDisabled = false;
		}

		ScriptEngine::Send_Custom_Event (obj, obj, Off_Weather_Message(), 0, 0);

		Tell_Building (obj, PLAYERTYPE_NOD, BuildingConstants::TYPE_COM_CENTER,
				Get_Int_Parameter ("Enable_Custom"));
		Tell_Building (obj, PLAYERTYPE_GDI, BuildingConstants::TYPE_COM_CENTER,
				Get_Int_Parameter ("Enable_Custom"));
		Tell_Building (obj, PLAYERTYPE_NOD, BuildingConstants::TYPE_POWER_PLANT,
				Get_Int_Parameter ("Enable_Custom"));
		Tell_Building (obj, PLAYERTYPE_GDI, BuildingConstants::TYPE_POWER_PLANT,
				Get_Int_Parameter ("Enable_Custom"));

		Sweep_Vehicles (obj, CUSTOM_AI_ENABLEAI, false);

		Announce (Get_Parameter ("End_Announcement_Sound_Nod"),
				Get_Parameter ("End_Announcement_Sound_GDI"),
				Get_Parameter ("End_Announcement_String"));
	}

	//
	//	Start a timer for a random span, unless there is not enough of the
	//	match left for it, in which case the storm is over for good.
	//
	bool	Wait_Random (GameObject * obj, const char * min_name, const char * max_name, int timer)
	{
		float	remaining	= ScriptEngine::Get_Time_Remaining_Seconds();
		float	shortest		= Get_Float_Parameter (min_name);
		float	longest		= Get_Float_Parameter (max_name);

		if (longest > remaining) {
			longest = remaining;
		}

		if ((shortest > remaining) || (shortest > longest)) {
			Destroy_Script();
			return false;
		}

		ScriptEngine::Start_Timer (obj, this, ScriptEngine::Get_Random (shortest, longest), timer);
		return true;
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {

			if (Wait_Random (obj, "Min_Time", "Max_Time", 2)) {
				Begin_Storm (obj);
			}

		} else if (number == 2) {

			End_Storm (obj);
			Wait_Random (obj, "Min_Delay", "Max_Delay", 1);

		} else if (number == 3) {

			if (Storming) {
				ScriptEngine::Create_2D_Sound (Get_Parameter ("Ion_Effect_Sound"));
				ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Ion_Effect_Time"), 3);
			}
		}
	}

	//
	//	Whether this match gets storms at all.  Note the sense: the donor
	//	returned early when the roll came in at or below IonChance, so a
	//	higher IonChance means fewer storms.  Kept, because levels were tuned
	//	against the behaviour.
	//
	bool	Rolls_For_Storms (void)
	{
		return ScriptEngine::Get_Random_Int (0, 100) > Get_Int_Parameter ("IonChance");
	}

	void	Created (GameObject * obj) override
	{
		_EmpDisabled	= false;
		Storming			= false;

		if (!Rolls_For_Storms()) {
			return ;
		}

		Wait_Random (obj, "Min_Delay", "Max_Delay", 1);
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Storming, 1);
	}

public:

	JFW_Ion_Storm_Base (void) : Storming (false)	{ }
};


REGISTER_SCRIPT_TT (JFW_Ion_Storm,
		"Min_Delay:float,Max_Delay:float,Min_Time:float,Max_Time:float,Disable_Custom:int,Enable_Custom:int,Announcement_Sound_Nod:string,Announcement_Sound_GDI:string,Announcement_String:string,Red:int,Green:int,Blue:int,Ion_Effect_Sound:string,Ion_Effect_Time:float,End_Announcement_Sound_Nod:string,End_Announcement_Sound_GDI:string,End_Announcement_String:string,On_Weather_Custom:int,Off_Weather_Custom:int,DestroyMines:int,DisableEmp:int,IonChance:int")
class	JFW_Ion_Storm : public JFW_Ion_Storm_Base
{
};


/*
**	The same storm, with each of the four effects under the mapper's control.
*/
REGISTER_SCRIPT_TT (JFW_Ion_Storm_2,
		"Min_Delay:float,Max_Delay:float,Min_Time:float,Max_Time:float,Disable_Custom:int,Enable_Custom:int,Announcement_Sound_Nod:string,Announcement_Sound_GDI:string,Announcement_String:string,Red:int,Green:int,Blue:int,Ion_Effect_Sound:string,Ion_Effect_Time:float,End_Announcement_Sound_Nod:string,End_Announcement_Sound_GDI:string,End_Announcement_String:string,On_Weather_Custom:int,Off_Weather_Custom:int,DestroyMines:int,DisableEmp:int,IonChance:int,DisableStealth:int,DisableRadar:int,DisablePower:int,DisableBaseDefenses:int")
class	JFW_Ion_Storm_2 : public JFW_Ion_Storm_Base
{
	bool	Disables_Stealth (void) override			{ return Get_Int_Parameter ("DisableStealth") != 0; }
	bool	Disables_Radar (void) override			{ return Get_Int_Parameter ("DisableRadar") != 0; }
	bool	Disables_Power (void) override			{ return Get_Int_Parameter ("DisablePower") != 0; }
	bool	Disables_Base_Defenses (void) override	{ return Get_Int_Parameter ("DisableBaseDefenses") != 0; }
};


/*
**	A storm that starts with the level and never lifts.  No delays, no
**	duration, no roll: one weather message rather than two, and no ending.
*/
REGISTER_SCRIPT_TT (JFW_Ion_Storm_3,
		"Disable_Custom:int,Enable_Custom:int,Announcement_Sound_Nod:string,Announcement_Sound_GDI:string,Announcement_String:string,Red:int,Green:int,Blue:int,Ion_Effect_Sound:string,Ion_Effect_Time:float,Weather_Custom:int,DestroyMines:int,DisableEmp:int,DisableStealth:int,DisableRadar:int,DisablePower:int,DisableBaseDefenses:int")
class	JFW_Ion_Storm_3 : public JFW_Ion_Storm_Base
{
	bool	Disables_Stealth (void) override			{ return Get_Int_Parameter ("DisableStealth") != 0; }
	bool	Disables_Radar (void) override			{ return Get_Int_Parameter ("DisableRadar") != 0; }
	bool	Disables_Power (void) override			{ return Get_Int_Parameter ("DisablePower") != 0; }
	bool	Disables_Base_Defenses (void) override	{ return Get_Int_Parameter ("DisableBaseDefenses") != 0; }

	int	On_Weather_Message (void) override		{ return Get_Int_Parameter ("Weather_Custom"); }
	int	Off_Weather_Message (void) override		{ return Get_Int_Parameter ("Weather_Custom"); }

	void	Created (GameObject * obj) override
	{
		_EmpDisabled = false;
		Begin_Storm (obj);
	}

	//	Only the sound loop; this storm has no beginning or end to time.
	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 3) {
			ScriptEngine::Create_2D_Sound (Get_Parameter ("Ion_Effect_Sound"));
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Ion_Effect_Time"), 3);
		}
	}
};


/*JFW_Ion_Storm_Weather

  The sky a storm brings: lightning, cloud and a tint over the screen, set on
  one message and left that way.
*/

DECLARE_SCRIPT_TT (JFW_Ion_Storm_Weather,
		"Lightning_Intensity:float,Lightning_Start_Distance:float,Lightning_End_Distance:float,Lightning_Heading:float,Lightning_Distribution:float,Cloud_Cover:float,Cloud_Gloominess:float,Screen_Red:float,Screen_Green:float,Screen_Blue:float,Screen_Opacity:float,Message:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		ScriptEngine::Set_Lightning (Get_Float_Parameter ("Lightning_Intensity"),
				Get_Float_Parameter ("Lightning_Start_Distance"),
				Get_Float_Parameter ("Lightning_End_Distance"),
				Get_Float_Parameter ("Lightning_Heading"),
				Get_Float_Parameter ("Lightning_Distribution"), 0);
		ScriptEngine::Set_Clouds (Get_Float_Parameter ("Cloud_Cover"),
				Get_Float_Parameter ("Cloud_Gloominess"), 0);
		ScriptEngine::Set_Screen_Fade_Color (Get_Float_Parameter ("Screen_Red"),
				Get_Float_Parameter ("Screen_Green"), Get_Float_Parameter ("Screen_Blue"), 0);
		ScriptEngine::Set_Screen_Fade_Opacity (Get_Float_Parameter ("Screen_Opacity"), 0);
	}
};


/*JFW_Ion_Storm_Weather_2

  The same, remembering what the sky looked like before so it can be put back
  when the storm lifts.
*/

DECLARE_SCRIPT_TT (JFW_Ion_Storm_Weather_2,
		"Lightning_Intensity:float,Lightning_Start_Distance:float,Lightning_End_Distance:float,Lightning_Heading:float,Lightning_Distribution:float,Cloud_Cover:float,Cloud_Gloominess:float,Screen_Red:float,Screen_Green:float,Screen_Blue:float,Screen_Opacity:float,Enable_Message:int,Disable_Message:int")
{
	float	Intensity;
	float	StartDistance;
	float	EndDistance;
	float	Heading;
	float	Distribution;
	float	CloudCover;
	float	CloudGloominess;

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Enable_Message")) {

			ScriptEngine::Get_Lightning (Intensity, StartDistance, EndDistance,
					Heading, Distribution);
			ScriptEngine::Get_Clouds (CloudCover, CloudGloominess);

			ScriptEngine::Set_Lightning (Get_Float_Parameter ("Lightning_Intensity"),
					Get_Float_Parameter ("Lightning_Start_Distance"),
					Get_Float_Parameter ("Lightning_End_Distance"),
					Get_Float_Parameter ("Lightning_Heading"),
					Get_Float_Parameter ("Lightning_Distribution"), 0);
			ScriptEngine::Set_Clouds (Get_Float_Parameter ("Cloud_Cover"),
					Get_Float_Parameter ("Cloud_Gloominess"), 0);
			ScriptEngine::Set_Screen_Fade_Color (Get_Float_Parameter ("Screen_Red"),
					Get_Float_Parameter ("Screen_Green"), Get_Float_Parameter ("Screen_Blue"), 0);
			ScriptEngine::Set_Screen_Fade_Opacity (Get_Float_Parameter ("Screen_Opacity"), 0);

		} else if (type == Get_Int_Parameter ("Disable_Message")) {

			ScriptEngine::Set_Lightning (Intensity, StartDistance, EndDistance,
					Heading, Distribution, 0);
			ScriptEngine::Set_Clouds (CloudCover, CloudGloominess, 0);
			ScriptEngine::Set_Screen_Fade_Color (0, 0, 0, 0);
			ScriptEngine::Set_Screen_Fade_Opacity (0, 0);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Intensity, 1);
		SAVE_VARIABLE (StartDistance, 2);
		SAVE_VARIABLE (EndDistance, 3);
		SAVE_VARIABLE (Heading, 4);
		SAVE_VARIABLE (Distribution, 5);
		SAVE_VARIABLE (CloudCover, 6);
		SAVE_VARIABLE (CloudGloominess, 7);
	}

public:

	JFW_Ion_Storm_Weather_2 (void) :
		Intensity (0.0f), StartDistance (0.0f), EndDistance (0.0f), Heading (0.0f),
		Distribution (0.0f), CloudCover (0.0f), CloudGloominess (0.0f)
	{ }
};


/*JFW_Ion_Lightning

  Strikes a random point on the map every so often while a storm is running:
  the bolt, the sound the ammunition makes, its explosion, and its damage to
  whatever building it landed on.
*/

DECLARE_SCRIPT_TT (JFW_Ion_Lightning, "MinTime:float,MaxTime:float,OnMessage:int,OffMessage:int,Ammo:string")
{
	bool	Storming;

	void	Created (GameObject * obj) override
	{
		Storming = false;
	}

	void	Wait (GameObject * obj)
	{
		ScriptEngine::Start_Timer (obj, this,
				ScriptEngine::Get_Random (Get_Float_Parameter ("MinTime"),
						Get_Float_Parameter ("MaxTime")), 1);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("OnMessage")) {
			Storming = true;
			Wait (obj);
		}

		if (type == Get_Int_Parameter ("OffMessage")) {
			Storming = false;
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (!Storming || (number != 1)) {
			return ;
		}

		Strike (obj);
		Wait (obj);
	}

	void	Strike (GameObject * obj)
	{
		const AmmoDefinitionClass * ammo = WeaponManager::Find_Ammo_Definition (Get_Parameter ("Ammo"));
		if ((ammo == nullptr) || (PhysicsSceneClass::Get_Instance() == nullptr)) {
			return ;
		}

		//
		//	Drop a ray from the sky at a random point on the map and see what
		//	it lands on.
		//
		Vector3	low, high;
		PhysicsSceneClass::Get_Instance()->Get_Level_Extents (low, high);

		high.Z	+= 1;
		low.Z		-= 1;

		float	x	= ScriptEngine::Get_Random (low.X, high.X);
		float	y	= ScriptEngine::Get_Random (low.Y, high.Y);

		Vector3	top		(x, y, high.Z);
		Vector3	bottom	(x, y, low.Z);

		CastResultStruct	result;
		result.ComputeContactPoint = true;

		LineSegClass					ray (top, bottom);
		PhysRayCollisionTestClass	test (ray, &result, DEFAULT_COLLISION_GROUP);

		PhysicsSceneClass::Get_Instance()->Cast_Ray (test, false);

		if (test.CollidedRenderObj == nullptr) {
			return ;
		}

		//	A surface facing away from the sky is the underside of something.
		if (Vector3::Dot_Product (result.Normal, ray.Get_Dir()) > 0) {
			return ;
		}

		bottom.Z = result.ContactPoint.Z;

		ScriptEngine::Create_Lightning (top, bottom);
		ScriptEngine::Create_Sound (ScriptEngine::Get_Definition_Name (ammo->FireSoundDefID),
				bottom, obj);

		bottom.Z += 0.1f;
		ScriptEngine::Create_Explosion (ScriptEngine::Get_Definition_Name (ammo->ExplosionDefID),
				bottom, nullptr);

		if (test.CollidedPhysObj == nullptr) {
			return ;
		}

		CombatPhysObserverClass * observer =
				(CombatPhysObserverClass *)test.CollidedPhysObj->Get_Observer();
		if ((observer != nullptr) && (observer->As_BuildingGameObj() != nullptr)) {
			ScriptEngine::Apply_Damage (observer->As_BuildingGameObj(), ammo->Damage,
					ArmorWarheadManager::Get_Warhead_Name (ammo->Warhead), nullptr);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Storming, 1);
	}

public:

	JFW_Ion_Lightning (void) : Storming (false)	{ }
};


/******************************************************************************
*
*     Zones made at runtime
*
******************************************************************************/

/*JFW_Vehicle_Zone

  Puts a zone around one of the object's bones when it appears, and takes it
  away when the object goes.  A vehicle that carries its own trigger volume.
*/

DECLARE_SCRIPT_TT (JFW_Vehicle_Zone, "BoneName:string,ZoneSize:vector3,ZonePreset:string")
{
	int	ZoneID;

	void	Created (GameObject * obj) override
	{
		Matrix3	rotation (true);
		rotation.Rotate_Z (DEG_TO_RADF (ScriptEngine::Get_Facing (obj)));

		OBBoxClass	box (ScriptEngine::Get_Bone_Position (obj, Get_Parameter ("BoneName")),
				Get_Vector3_Parameter ("ZoneSize"), rotation);

		GameObject * zone = ScriptEngine::Create_Zone (Get_Parameter ("ZonePreset"), box);
		ZoneID = (zone != nullptr) ? ScriptEngine::Get_ID (zone) : 0;
	}

	void	Destroyed (GameObject * obj) override
	{
		Destroy_By_ID (ZoneID);
		ZoneID = 0;
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (ZoneID, 1);
	}

public:

	JFW_Vehicle_Zone (void) : ZoneID (0)	{ }
};


/*JFW_Building_Zone_Controller

  The same idea for a building, which cannot make its own zone because the
  building is placed before the thing that wants the zone knows where it is.
  A JFW_Building_Zone on the other object tells this one where to put it.

  Registered under the donor's own misspelling of its name.
*/

REGISTER_SCRIPT_TT_NAMED (JFW_Building_Zone_Controller, "JFW_Building_Zone_Controler",
		"ZoneSize:vector3,ZonePreset:string")
class	JFW_Building_Zone_Controller : public ScriptImpClass
{
	int	ZoneID;

	void	Created (GameObject * obj) override
	{
		ZoneID = 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if ((type != 0x123456) || (sender == nullptr)) {
			return ;
		}

		Matrix3	rotation (true);
		rotation.Rotate_Z (DEG_TO_RADF (ScriptEngine::Get_Facing (obj)));

		OBBoxClass	box (ScriptEngine::Get_Position (sender),
				Get_Vector3_Parameter ("ZoneSize"), rotation);

		GameObject * zone = ScriptEngine::Create_Zone (Get_Parameter ("ZonePreset"), box);
		if (zone != nullptr) {
			ZoneID = ScriptEngine::Get_ID (zone);
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Destroy_By_ID (ZoneID);
		ZoneID = 0;
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (ZoneID, 1);
	}

public:

	JFW_Building_Zone_Controller (void) : ZoneID (0)	{ }
};


/*JFW_Building_Zone

  Tells the nearest object of a named preset to put a zone here.
*/

DECLARE_SCRIPT_TT (JFW_Building_Zone, "ParentPreset:string")
{
	void	Created (GameObject * obj) override
	{
		GameObject * parent = ScriptEngine::Find_Nearest_Preset (ScriptEngine::Get_Position (obj),
				Get_Parameter ("ParentPreset"));
		if (parent != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, parent, 0x123456, 0, 0.5f);
		}
	}
};


/******************************************************************************
*
*     Two last things
*
******************************************************************************/

/*JFW_MSA

  A mobile sensor array: park it, and while it is deployed it points at every
  cloaked or buried enemy within range by sending each of them a message.  A
  size marker is attached while it is deployed so the other side can see what
  it is looking at.
*/

DECLARE_SCRIPT_TT (JFW_MSA, "Time:float,Message:int,Range:float,SizePreset:string")
{
	int	SizeID;
	bool	Deployed;

	void	Created (GameObject * obj) override
	{
		SizeID	= 0;
		Deployed	= false;

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
		ScriptEngine::Start_Timer (obj, this, 1, 2);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (	(type == CUSTOM_EVENT_VEHICLE_EXITED) && Deployed
				&& (vehicle != nullptr) && (vehicle->Get_Occupant_Count() == 0))
		{
			//	Keep it on the side of whoever last got out of it.
			ScriptEngine::Update_Network_Object (obj);
			ScriptEngine::Set_Player_Type (obj, ScriptEngine::Get_Player_Type (sender));
		}
	}

	void	Deploy (GameObject * obj, VehicleGameObj * vehicle)
	{
		Deployed = true;
		vehicle->Set_Lock_Team (vehicle->Get_Player_Type());

		if (SizeID != 0) {
			return ;
		}

		GameObject * marker = ScriptEngine::Create_Object (Get_Parameter ("SizePreset"),
				ScriptEngine::Get_Bone_Position (obj, "ROOTTRANSFORM"));
		if (marker == nullptr) {
			return ;
		}

		ScriptEngine::Set_Player_Type (marker, ScriptEngine::Get_Player_Type (obj));
		ScriptEngine::Attach_To_Object_Bone (marker, obj, "ROOTTRANSFORM");
		SizeID = ScriptEngine::Get_ID (marker);
	}

	void	Stow (VehicleGameObj * vehicle)
	{
		Deployed = false;
		vehicle->Set_Lock_Team (2);

		Destroy_By_ID (SizeID);
		SizeID = 0;
	}

	//	Everything cloaked or underground within range, on the other side.
	void	Sweep (GameObject * obj)
	{
		Vector3	here;
		obj->Get_Position (&here);

		DamageableGameObj *	self		= obj->As_DamageableGameObj();
		float						range		= Get_Float_Parameter ("Range");
		int						message	= Get_Int_Parameter ("Message");

		SList<SmartGameObj> * list = GameObjManager::Get_Smart_Game_Obj_List();
		for (SLNode<SmartGameObj> * node = list->Head(); node != nullptr; node = node->Next()) {

			SmartGameObj * other = node->Data();
			if ((other == nullptr) || (other == obj)) {
				continue;
			}

			VehicleGameObj *	vehicle	= other->As_VehicleGameObj();
			bool					hidden	= other->Is_Stealthed()
					|| ((vehicle != nullptr) && vehicle->Is_Underground());
			if (!hidden) {
				continue;
			}

			if ((self != nullptr) && other->Is_Teammate (self)) {
				continue;
			}

			Vector3	there;
			other->Get_Position (&there);

			if (ScriptEngine::Get_Distance (here, there) <= range) {
				ScriptEngine::Send_Custom_Event (obj, other, message, 0, 0);
			}
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		VehicleGameObj * vehicle = As_Vehicle (obj);

		if (number == 2) {

			ScriptEngine::Start_Timer (obj, this, 1, 2);

			if (vehicle != nullptr) {
				if (vehicle->Is_Immovable()) {
					if (!Deployed) {
						Deploy (obj, vehicle);
					}
				} else if (Deployed) {
					Stow (vehicle);
				}
			}
		}

		if ((number == 1) && Deployed) {
			Sweep (obj);
		}

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Destroyed (GameObject * obj) override
	{
		Destroy_By_ID (SizeID);
		SizeID = 0;
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (SizeID, 1);
		SAVE_VARIABLE (Deployed, 2);
	}

public:

	JFW_MSA (void) : SizeID (0), Deployed (false)	{ }
};


/*JFW_Killed_String_Sound

  Announces a death to both sides in each side's own colour, with whatever
  sound the translation database has attached to the line.  Six string IDs:
  what each side is told about a Nod death, a GDI death, and a death of
  anything belonging to neither.
*/

DECLARE_SCRIPT_TT (JFW_Killed_String_Sound,
		"GDI_String_GDI:int,Nod_String_GDI:int,Neutral_String_GDI:int,GDI_String_Nod:int,Nod_String_Nod:int,Neutral_String_Nod:int")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		int	team	= ScriptEngine::Get_Player_Type (obj);

		const char *	to_gdi	= "Neutral_String_GDI";
		const char *	to_nod	= "Neutral_String_Nod";
		Vector3			colour	= Get_Public_Text_Color();

		if (team == PLAYERTYPE_NOD) {
			to_gdi	= "Nod_String_GDI";
			to_nod	= "Nod_String_Nod";
			colour	= Get_Color_For_Team (PLAYERTYPE_NOD);
		} else if (team == PLAYERTYPE_GDI) {
			to_gdi	= "GDI_String_GDI";
			to_nod	= "GDI_String_Nod";
			colour	= Get_Color_For_Team (PLAYERTYPE_GDI);
		}

		int	red	= (int)(colour.X * 255);
		int	green	= (int)(colour.Y * 255);
		int	blue	= (int)(colour.Z * 255);

		Say_String_To_Team (PLAYERTYPE_GDI, red, green, blue, Get_Int_Parameter (to_gdi));
		Say_String_To_Team (PLAYERTYPE_NOD, red, green, blue, Get_Int_Parameter (to_nod));
	}
};
