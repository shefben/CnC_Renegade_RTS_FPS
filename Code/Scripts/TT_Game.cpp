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
*     TT_Game.cpp
*
* DESCRIPTION
*     The rules a whole match plays by: who owns which point on the map, what
*     a construction yard puts back, where the ore goes, who can see a
*     stealthed tank, and what happens when somebody points a hijacker's
*     pistol at a driver.
*
*     Native port of the 4.8.4 library's jfwgame.cpp.  Every script keeps its
*     registered name and its registered parameters, so existing levels find
*     them unchanged.
*
*     Sixty-four scripts, and rather fewer ideas.  Four construction-yard
*     scripts are one repair sweep and a list of what else it touches.  Two
*     domination controllers are the same scoreboard, one of which also ends
*     the game.  Two team-deathmatch controllers count the same deaths and
*     do different things at the end of the count.  Nine message scripts are
*     one announcement and three audiences.  Two radar jammers are the same
*     proximity test.  Those are collapsed here into bases and virtuals; the
*     leaf classes carry the registered name and nothing else.
*
*     Defects in the donor, fixed:
*
*     - JFW_Carry_Control kept twenty carryall slots in twenty named members
*       and filled slot N only when slots one through N-1 were all occupied.
*       A carryall destroyed in the middle of the list therefore left a hole
*       that nothing could ever fill again, and the fleet shrank by one every
*       time a middle slot died.  The slots are an array here and the first
*       free one is used.
*
*     - JFW_Chrono_Harvester_Attached never initialised its ten crystal IDs,
*       its crystal counter or its stored facing.  With no Crystal_Preset
*       configured -- the documented way to run it -- Killed then called
*       Find_Object on ten uninitialised integers.  All of them start at
*       zero here and the destroy pass checks what it found.
*
*     - JFW_CarryAll auto-saved an `enabled` member that Created never set
*       and nothing ever read, writing an indeterminate byte into the save.
*       Dropped.
*
*     - JFW_Reaper_Web read Get_Float_Parameter("time").  Its registered
*       parameter is "Time"; the lower-case name matches nothing, so the web
*       always timed out immediately.  Fixed to the registered name.
*
*     - JFW_Resource_Collector_2 and _3 formatted their announcement into a
*       240-byte stack buffer only on three exact cash totals, and then
*       printed that buffer whatever the total had been -- so any other
*       amount announced whatever the stack happened to hold.  They also
*       `delete[]`'d the player name the engine handed them, which they did
*       not own.  Both build a string here and say nothing when there is
*       nothing to say.
*
*     - JFW_Global_Stealth_Controller::Add_Generator walked its list with
*       `while (Current->Next)`, so the duplicate check never examined the
*       last node.  Re-registering the most recently added generator
*       appended a second copy of it, and the copy was never removed.
*
*     - JFW_Assault_Powerplant_Controller started a timer, cut the power to
*       fifteen buildings when it expired, and never restarted it.  The
*       `inzone` flag it tests exists to let a team hold the zone and keep
*       the power on, which needs the poll to repeat; it repeats here.
*
*     - JFW_Team_DM_Controller applied ten thousand points of damage to five
*       objects it looked up by ID without checking that it found any of
*       them, and computed a max health it never used.
*
*     - The scripts that ask Get_Current_Weapon what a damager is holding
*       passed the answer straight to _stricmp.  An unarmed damager -- a
*       Nod turret's shell, say -- answers null.  Checked here.
*
*     - JFW_Building_Preset_Disable let its nesting count go negative when
*       an enable arrived before a disable, after which the count could
*       never return to zero and the factory stayed disabled for the match.
*
*     - JFW_Submarine's surface check had an empty `if` branch and did its
*       work in the `else`.  Written the way round it reads.
*
*     Deliberate differences from the donor:
*
*     - The ore-dump announcements went out as `cmsgt <team> <r,g,b> <text>`
*       and `sndt <team> <sound>` through the SSGM console.  There is no
*       console layer under a built-in script here, so they call
*       Send_Message_Team and Create_2D_Sound_Team, which is what those
*       console commands did at the far end.
*
*     - Ranged_Stealth_On_Team was an engine export in the donor.  It walks a
*       linked list that the script itself builds and owns, and nothing else
*       can call it, so it is a private function of the controller here
*       rather than a script command.
*
*     - JFW_Spy_Switch_Team::Detach guarded its work with `Exe != 4` -- "not
*       running inside LevelEdit".  The editor build does not tick game
*       objects, so there is nothing to guard against and the test is gone.
*
*     - JFW_Toxic_Grenade clears itself on CUSTOM_EVENT_REFILL.  That event
*       did not exist here; ScriptEngine::Grant_Refill now sends it, which
*       is where the donor's engine sent it from too.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "actionparams.h"
#include "building.h"
#include "damage.h"
#include "damageablegameobj.h"
#include "gameobjmanager.h"
#include "gameobjobserver.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "smartgameobj.h"
#include "soldier.h"
#include "vector.h"
#include "vector3.h"
#include "vehicle.h"
#include "wwmath.h"
#include "wwstring.h"

#include <math.h>
#include <string.h>


/******************************************************************************
*
*     Shared ground
*
******************************************************************************/

/*
**	The other side.  The donor wrote this as a PTTEAM macro over its own team
**	indices; every base query in this engine takes a PLAYERTYPE already.
*/
static int	Enemy_Of (int player_type)
{
	return (player_type != PLAYERTYPE_NOD) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;
}


/*
**	The vehicle an object is, rather than the vehicle an object is riding in.
*/
static VehicleGameObj *	As_Vehicle (GameObject * obj)
{
	PhysicalGameObj * physical = (obj != nullptr) ? obj->As_PhysicalGameObj() : nullptr;
	return (physical != nullptr) ? physical->As_VehicleGameObj() : nullptr;
}


/*
**	A float carried through a custom event's integer parameter.  The donor
**	cast a reference; that is undefined and, on a build where intptr_t is
**	eight bytes, reads four bytes it never wrote.
*/
static intptr_t	Float_To_Param (float value)
{
	unsigned int bits = 0;
	::memcpy (&bits, &value, sizeof (bits));
	return (intptr_t)bits;
}


static float	Param_To_Float (intptr_t param)
{
	unsigned int	bits	= (unsigned int)param;
	float				value	= 0.0f;
	::memcpy (&value, &bits, sizeof (value));
	return value;
}


/*
**	Whether an object is holding the named weapon.  Get_Current_Weapon
**	answers null for anything unarmed, which the donor handed straight to
**	_stricmp.
*/
static bool	Weapon_Is (GameObject * obj, const char * weapon_name)
{
	const char * held = ScriptEngine::Get_Current_Weapon (obj);
	return (held != nullptr) && (weapon_name != nullptr) && (::_stricmp (held, weapon_name) == 0);
}


/*
**	Send a custom to an object named by ID, if there is one.  Fifteen copies
**	of this in the donor's power-plant controller alone.
*/
static void	Send_To_ID (GameObject * from, int object_id, int type, intptr_t param = 0)
{
	GameObject * target = ScriptEngine::Find_Object (object_id);
	if (target != nullptr) {
		ScriptEngine::Send_Custom_Event (from, target, type, param, 0);
	}
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


/******************************************************************************
*
*     Team deathmatch
*
*     A zone marks the side that has to stay alive.  Anybody of that side who
*     leaves it picks up a script that reports their death to a controller,
*     and the controller counts.  What happens when the count runs out is the
*     only thing the two controllers disagree about.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Team_DM_Zone, "Player_Type:int,Controller_ID:int,Deactivate_Message:int,Death_Message:int")
{
	bool	Triggered;

	void	Created (GameObject * obj) override
	{
		Triggered = false;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Deactivate_Message")) {
			Triggered = true;
		}
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		if (!Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type"))) {
			return ;
		}

		//
		//	Ask whoever is leaving whether they are already carrying the
		//	death reporter.  If they are, their copy answers and sets the
		//	flag; if nothing answers, they are not.
		//
		Triggered = false;
		ScriptEngine::Send_Custom_Event (obj, exiter, Get_Int_Parameter ("Deactivate_Message"), 1, 0);

		if (!Triggered) {
			StringClass params;
			params.Format ("%d,%d,%d",
					Get_Int_Parameter ("Controller_ID"),
					Get_Int_Parameter ("Deactivate_Message"),
					Get_Int_Parameter ("Death_Message"));
			ScriptEngine::Attach_Script (exiter, "JFW_Team_DM_Attatched", params);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Triggered, 1);
	}
};


DECLARE_SCRIPT_TT (JFW_Team_DM_Attatched, "Controller_ID:int,Deactivate_Message:int,Death_Message:int")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Send_To_ID (obj, Get_Int_Parameter ("Controller_ID"), Get_Int_Parameter ("Death_Message"), 1);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Deactivate_Message")) {
			ScriptEngine::Send_Custom_Event (obj, sender, type, 1, 0);
		}
	}
};


/*
**	Counting the deaths.  What the count is for is the derived class's.
*/
class	JFW_Team_DM_Controller_Base : public ScriptImpClass
{
protected:
	int	Deaths;

	virtual void	Count_Reached (GameObject * obj) = 0;

	void	Created (GameObject * obj) override
	{
		Deaths = 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Death_Message")) {
			Deaths ++;
		}

		if (Deaths >= Get_Int_Parameter ("Deaths_To_Loose")) {
			Count_Reached (obj);
		}
	}

public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Deaths, 1);
	}
};


REGISTER_SCRIPT_TT (JFW_Team_DM_Controller, "Deaths_To_Loose:int,Death_Message:int,Building_To_Destroy1:int,Building_To_Destroy2:int,Building_To_Destroy3:int,Building_To_Destroy4:int,Building_To_Destroy5:int")
class	JFW_Team_DM_Controller : public JFW_Team_DM_Controller_Base
{
protected:
	void	Count_Reached (GameObject * obj) override
	{
		static const char * const	NAMES[] = {
			"Building_To_Destroy1", "Building_To_Destroy2", "Building_To_Destroy3",
			"Building_To_Destroy4", "Building_To_Destroy5"
		};

		for (int index = 0; index < 5; index ++) {
			GameObject * building = ScriptEngine::Find_Object (Get_Int_Parameter (NAMES[index]));
			if (building != nullptr) {
				ScriptEngine::Apply_Damage (building, 10000, "Explosive", nullptr);
			}
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Team_DM_Controller_2, "Deaths_To_Loose:int,Death_Message:int,Team:int")
class	JFW_Team_DM_Controller_2 : public JFW_Team_DM_Controller_Base
{
protected:
	void	Count_Reached (GameObject * obj) override
	{
		ScriptEngine::Kill_All_Buildings_By_Team (Get_Int_Parameter ("Team"));
	}
};


/******************************************************************************
*
*     The assault power plant
*
*     Fifteen buildings lose their power unless somebody of the named side is
*     standing in the zone when the poll comes round.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Assault_Powerplant_Controller, "Time:float,TimerNum:int,Player_Type:int,Power_Off:int,Power_On:int,Building1:int,Building2:int,Building3:int,Building4:int,Building5:int,Building6:int,Building7:int,Building8:int,Building9:int,Building10:int,Building11:int,Building12:int,Building13:int,Building14:int,Building15:int")
{
	bool	InZone;

	void	Tell_The_Buildings (GameObject * obj, int type)
	{
		for (int index = 1; index <= 15; index ++) {
			StringClass name;
			name.Format ("Building%d", index);
			Send_To_ID (obj, Get_Int_Parameter (name), type, 0);
		}
	}

	void	Created (GameObject * obj) override
	{
		InZone = false;
		Tell_The_Buildings (obj, Get_Int_Parameter ("Power_Off"));
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) {
			return ;
		}

		if (!InZone) {
			Tell_The_Buildings (obj, Get_Int_Parameter ("Power_Off"));
		}

		//
		//	The donor never restarted this.  Holding the zone is meant to keep
		//	the power on for as long as you hold it, which needs the poll.
		//
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) {
			InZone = true;
		}
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		if (Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type"))) {
			InZone = false;
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (InZone, 1);
	}
};


/******************************************************************************
*
*     Ore and tiberium
*
*     A field hands cash to whoever is parked in it, a collector carries the
*     cash, and a refinery asks the collector to hand it over.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Resource_Collector, "Collected_Mesasge:int,Refinery_Message:int,Cash_Limit:int,Cash_Type:int,Entire_Team:int")
{
	int	Cash;
	int	CarrierID;

	void	Created (GameObject * obj) override
	{
		Cash			= 0;
		CarrierID	= 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		int	cash_limit	= Get_Int_Parameter ("Cash_Limit");
		int	cash_type	= Get_Int_Parameter ("Cash_Type");

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			int old_id = CarrierID;
			CarrierID = ScriptEngine::Get_ID (sender);

			if ((cash_type == 0) || ((cash_type == 1) && (CarrierID != old_id))) {
				Cash = 0;
			}

		} else if (type == Get_Int_Parameter ("Collected_Mesasge")) {

			if (cash_type == 3) {
				Cash = 0;
			}

			Cash += (int)param;
			if ((cash_limit != 0) && (Cash > cash_limit)) {
				Cash = cash_limit;
			}

		} else if (type == Get_Int_Parameter ("Refinery_Message")) {

			GameObject * carrier = ScriptEngine::Find_Object (CarrierID);
			if (carrier != nullptr) {
				ScriptEngine::Give_Money (carrier, (float)Cash, Get_Bool_Parameter ("Entire_Team"));
			}
			Cash = 0;
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Cash, 1);
		SAVE_VARIABLE (CarrierID, 2);
	}
};


/*
**	The same collector, with an announcement.  The two derived scripts differ
**	only in what the cargo and the side are called.
*/
class	JFW_Resource_Collector_Announced_Base : public ScriptImpClass
{
protected:
	bool	HasOre;
	int	Cash;
	int	CarrierID;
	int	PlayerID;

	//	Null for an amount this collector has nothing to say about.
	virtual const char *	Cargo_Name (int cash) = 0;
	virtual const char *	Force_Name (int team) = 0;

	void	Created (GameObject * obj) override
	{
		HasOre		= false;
		Cash			= 0;
		CarrierID	= 0;
		PlayerID		= 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		int	cash_limit	= Get_Int_Parameter ("Cash_Limit");
		int	cash_type	= Get_Int_Parameter ("Cash_Type");

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {

			int	old_carrier	= CarrierID;
			int	old_player	= PlayerID;

			CarrierID	= ScriptEngine::Get_ID (sender);
			PlayerID		= ScriptEngine::Get_Player_ID (sender);

			//
			//	Cash_Type 1 keeps the load across a respawn of the same
			//	player, whose new body has a new object ID.
			//
			if ((cash_type == 0)
					|| ((cash_type == 1) && (CarrierID != old_carrier) && (PlayerID != old_player))) {
				Cash = 0;
			}

		} else if (type == Get_Int_Parameter ("Collected_Mesasge")) {

			HasOre = true;
			if (cash_type == 3) {
				Cash = 0;
			}

			Cash += (int)param;
			if ((cash_limit != 0) && (Cash > cash_limit)) {
				Cash = cash_limit;
			}

		} else if ((type == Get_Int_Parameter ("Refinery_Message")) && HasOre) {

			Deposit (obj);
		}
	}

	void	Deposit (GameObject * obj)
	{
		int				team	= ScriptEngine::Get_Player_Type (obj);
		const char *	cargo	= Cargo_Name (Cash);
		const char *	force	= Force_Name (team);

		//
		//	A load the collector has no wording for is not a load it knows
		//	how to bank.  The donor printed an uninitialised buffer here.
		//
		if ((cargo == nullptr) || (force == nullptr)) {
			return ;
		}

		GameObject * carrier = ScriptEngine::Find_Object (CarrierID);
		if (carrier != nullptr) {
			ScriptEngine::Give_Money (carrier, (float)Cash, Get_Bool_Parameter ("Entire_Team"));
		}

		StringClass		name (ScriptEngine::Get_Player_Name_By_ID (PlayerID));
		StringClass		message;
		message.Format ("%s dumped %s, funding %s with %d additional credits.",
				name.Peek_Buffer(), cargo, force, Cash);

		ScriptEngine::Create_2D_Sound_Team (team, "gsweep.wav");
		ScriptEngine::Send_Message_Team (team, 9, 193, 18, message);

		HasOre	= false;
		Cash		= 0;
	}

public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Cash, 1);
		SAVE_VARIABLE (CarrierID, 2);
		SAVE_VARIABLE (PlayerID, 3);
		SAVE_VARIABLE (HasOre, 4);
	}
};


REGISTER_SCRIPT_TT (JFW_Resource_Collector_2, "Collected_Mesasge:int,Refinery_Message:int,Cash_Limit:int,Cash_Type:int,Entire_Team:int")
class	JFW_Resource_Collector_2 : public JFW_Resource_Collector_Announced_Base
{
protected:
	const char *	Cargo_Name (int cash) override
	{
		switch (cash) {
			case 700:	return "Ore";
			case 1050:	return "Mixed Ore/Gems";
			case 1400:	return "Gems";
			default:		return nullptr;
		}
	}

	const char *	Force_Name (int team) override
	{
		switch (team) {
			case 0:	return "the Soviet Warmachine";
			case 1:	return "the Allied Forces";
			default:	return nullptr;
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Resource_Collector_3, "Collected_Mesasge:int,Refinery_Message:int,Cash_Limit:int,Cash_Type:int,Entire_Team:int")
class	JFW_Resource_Collector_3 : public JFW_Resource_Collector_Announced_Base
{
protected:
	const char *	Cargo_Name (int cash) override
	{
		switch (cash) {
			case 700:	return "Tiberium";
			case 1050:	return "Mixed Tiberium/Blue Tiberium";
			case 1400:	return "Blue Tiberium";
			default:		return nullptr;
		}
	}

	const char *	Force_Name (int team) override
	{
		switch (team) {
			case 0:	return "the Nod Warmachine";
			case 1:	return "the GDI Forces";
			default:	return nullptr;
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Resource_Refinery, "Refinery_Message:int")
{
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		ScriptEngine::Send_Custom_Event (obj, enterer, Get_Int_Parameter ("Refinery_Message"), 0, 0);
	}
};


DECLARE_SCRIPT_TT (JFW_Resource_Field, "Cash:int,Time:float,TimerNum:int,Collected_Preset:string,Collected_Preset_Distance:float,Collected_Message:int,Nod_Preset:string,GDI_Preset:string,StartHarvest:int,StopHarvest:int")
{
	int	HarvesterID;

	void	Created (GameObject * obj) override
	{
		HarvesterID = 0;
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (HarvesterID != 0) {
			return ;
		}

		const char * preset = ScriptEngine::Get_Preset_Name (enterer);
		if (preset == nullptr) {
			return ;
		}

		if ((::_stricmp (preset, Get_Parameter ("Nod_Preset")) != 0)
				&& (::_stricmp (preset, Get_Parameter ("GDI_Preset")) != 0)) {
			return ;
		}

		HarvesterID = ScriptEngine::Get_ID (enterer);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
		ScriptEngine::Send_Custom_Event (obj, enterer, Get_Int_Parameter ("StartHarvest"), 0, 0);
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		if (ScriptEngine::Get_ID (exiter) == HarvesterID) {
			HarvesterID = 0;
			ScriptEngine::Send_Custom_Event (obj, exiter, Get_Int_Parameter ("StopHarvest"), 0, 0);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		GameObject * harvester = (HarvesterID != 0) ? ScriptEngine::Find_Object (HarvesterID) : nullptr;
		if (harvester == nullptr) {
			return ;
		}

		ScriptEngine::Send_Custom_Event (obj, harvester, Get_Int_Parameter ("Collected_Message"),
				Get_Int_Parameter ("Cash"), 0);

		//	Drop the visible load just in front of whatever collected it.
		float		distance	= Get_Float_Parameter ("Collected_Preset_Distance");
		float		angle		= DEG_TO_RADF (ScriptEngine::Get_Facing (harvester));
		Vector3	position	= ScriptEngine::Get_Position (harvester);

		position.X += distance * ::cosf (angle);
		position.Y += distance * ::sinf (angle);
		ScriptEngine::Create_Object (Get_Parameter ("Collected_Preset"), position);

		ScriptEngine::Send_Custom_Event (obj, harvester, Get_Int_Parameter ("StopHarvest"), 0, 0);
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (HarvesterID, 1);
	}
};


/******************************************************************************
*
*     Domination
*
*     Zones that change hands, and a controller that pays whoever holds them.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Domination_Zone, "Controler_ID:int,NeutralMessage:int,OwnedMessage:int,Nod_Model:string,GDI_Model:string,Neutral_Model:string,Capture_Delay_Time:float,TimerNum:int")
{
	enum { UNOWNED = 2 };

	int	MarkerID;
	bool	Enabled;
	int	Owner;

	//	The flag standing in the zone, in whoever's colours it currently flies.
	void	Raise_Marker (GameObject * obj, const char * model)
	{
		Destroy_By_ID (MarkerID);
		MarkerID = 0;

		GameObject * marker = ScriptEngine::Create_Object ("Generic_Cinematic", ScriptEngine::Get_Position (obj));
		if (marker == nullptr) {
			return ;
		}

		ScriptEngine::Enable_Hibernation (marker, false);
		ScriptEngine::Add_To_Dirty_Cull_List (marker);
		ScriptEngine::Set_Model (marker, model);
		ScriptEngine::Set_Facing (marker, ScriptEngine::Get_Facing (obj));
		MarkerID = ScriptEngine::Get_ID (marker);
	}

	void	Created (GameObject * obj) override
	{
		Enabled	= true;
		Owner		= UNOWNED;
		MarkerID	= 0;
		Raise_Marker (obj, Get_Parameter ("Neutral_Model"));
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Enabled) {
			return ;
		}

		int taker = ScriptEngine::Get_Player_Type (enterer);
		if (taker == Owner) {
			return ;
		}

		//	Taking it from nobody and taking it from somebody are two events.
		int type = (Owner == UNOWNED)
				? Get_Int_Parameter ("NeutralMessage")
				: Get_Int_Parameter ("OwnedMessage");

		if (taker == PLAYERTYPE_NOD) {
			Raise_Marker (obj, Get_Parameter ("Nod_Model"));
			Owner = PLAYERTYPE_NOD;
		} else if (taker == PLAYERTYPE_GDI) {
			Raise_Marker (obj, Get_Parameter ("GDI_Model"));
			Owner = PLAYERTYPE_GDI;
		}

		Send_To_ID (obj, Get_Int_Parameter ("Controler_ID"), type, Owner);

		//	A capture delay stops the zone flickering between two people
		//	standing in it.
		float delay = Get_Float_Parameter ("Capture_Delay_Time");
		if (delay > 0.0f) {
			Enabled = false;
			ScriptEngine::Start_Timer (obj, this, delay, Get_Int_Parameter ("TimerNum"));
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		Enabled = true;
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (MarkerID, 1);
		SAVE_VARIABLE (Enabled, 2);
		SAVE_VARIABLE (Owner, 3);
	}
};


/*
**	The scoreboard.  Whether holding every zone also ends the match is the
**	derived class's business.
*/
class	JFW_Domination_Controler_Base : public ScriptImpClass
{
protected:
	int	GDIZones;
	int	NodZones;

	virtual void	Zones_Changed (GameObject * obj)		{ }

	void	Created (GameObject * obj) override
	{
		GDIZones	= 0;
		NodZones	= 0;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		int	neutral	= Get_Int_Parameter ("NeutralMessage");
		int	owned		= Get_Int_Parameter ("OwnedMessage");

		if ((type != neutral) && (type != owned)) {
			return ;
		}

		//	The parameter is the new owner, so an owned zone changing hands
		//	takes one away from whoever is not the new owner.
		if (type == owned) {
			if (param == PLAYERTYPE_NOD) {
				GDIZones --;
			} else {
				NodZones --;
			}
		}

		if (param == PLAYERTYPE_GDI) {
			GDIZones ++;
		} else {
			NodZones ++;
		}

		Zones_Changed (obj);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		GameObject *	gdi			= ScriptEngine::Find_Object (Get_Int_Parameter ("GDIObjectID"));
		GameObject *	nod			= ScriptEngine::Find_Object (Get_Int_Parameter ("NodObjectID"));
		float				to_give		= Get_Float_Parameter ("PointsToGive");
		float				to_take		= Get_Float_Parameter ("PointsToTake");

		if (GDIZones > 0) {
			if ((to_give != 0.0f) && (gdi != nullptr)) {
				ScriptEngine::Give_Points (gdi, to_give * GDIZones, true);
			}
			if ((to_take != 0.0f) && (nod != nullptr)) {
				ScriptEngine::Give_Points (nod, -(to_take * GDIZones), true);
			}
		}

		if (NodZones > 0) {
			if ((to_give != 0.0f) && (nod != nullptr)) {
				ScriptEngine::Give_Points (nod, to_give * NodZones, true);
			}
			if ((to_take != 0.0f) && (gdi != nullptr)) {
				ScriptEngine::Give_Points (gdi, -(to_take * NodZones), true);
			}
		}

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (GDIZones, 1);
		SAVE_VARIABLE (NodZones, 2);
	}
};


REGISTER_SCRIPT_TT (JFW_Domination_Controler, "NeutralMessage:int,OwnedMessage:int,Time:float,TimerNum:int,PointsToGive:float,PointsToTake:float,GDIObjectID:int,NodObjectID:int")
class	JFW_Domination_Controler : public JFW_Domination_Controler_Base
{
};


REGISTER_SCRIPT_TT (JFW_Domination_Controler_End_Game, "NeutralMessage:int,OwnedMessage:int,Time:float,TimerNum:int,PointsToGive:float,PointsToTake:float,GDIObjectID:int,NodObjectID:int,ZoneCount:int")
class	JFW_Domination_Controler_End_Game : public JFW_Domination_Controler_Base
{
protected:
	void	Zones_Changed (GameObject * obj) override
	{
		int all = Get_Int_Parameter ("ZoneCount");

		//	Holding every zone destroys the other side's base.
		if (GDIZones == all) {
			ScriptEngine::Kill_All_Buildings_By_Team (PLAYERTYPE_NOD);
		}
		if (NodZones == all) {
			ScriptEngine::Kill_All_Buildings_By_Team (PLAYERTYPE_GDI);
		}
	}
};


/******************************************************************************
*
*     Superweapons that arrive as cinematics
*
*     Both of these are a cinematic on a timer with a rendered on/off model
*     beside it.  The cinematic is a Test_Cinematic control file attached to
*     an invisible host, told which building it belongs to through a slot
*     custom.
*
******************************************************************************/

/*
**	Start one of the three animation cinematics a superweapon script names.
*/
static void	Play_Slot_Cinematic (ScriptImpClass * script, GameObject * obj, const char * which)
{
	StringClass	control_name, location_name, facing_name, slot_name;
	control_name.Format	("Animation%s", which);
	location_name.Format	("Animation%sLoc", which);
	facing_name.Format	("Animation%sFacing", which);
	slot_name.Format		("Animation%sSlot", which);

	GameObject * host = ScriptEngine::Create_Object ("Invisible_Object",
			script->Get_Vector3_Parameter (location_name));
	if (host == nullptr) {
		return ;
	}

	ScriptEngine::Set_Facing (host, script->Get_Float_Parameter (facing_name));
	ScriptEngine::Attach_Script (host, "Test_Cinematic", script->Get_Parameter (control_name));
	ScriptEngine::Send_Custom_Event (obj, host,
			M00_CUSTOM_CINEMATIC_SET_SLOT + script->Get_Int_Parameter (slot_name),
			script->Get_Int_Parameter ("BuildingID"), 0);
}


/*
**	Show one of the pair of models a superweapon uses to say whether it is
**	charged, or neither of them while a cinematic is running over the top.
*/
static void	Show_Charge_State (ScriptImpClass * script, bool on, bool off)
{
	GameObject * on_model	= ScriptEngine::Find_Object (script->Get_Int_Parameter ("OnID"));
	GameObject * off_model	= ScriptEngine::Find_Object (script->Get_Int_Parameter ("OffID"));

	if (on_model != nullptr) {
		ScriptEngine::Set_Is_Rendered (on_model, on);
	}
	if (off_model != nullptr) {
		ScriptEngine::Set_Is_Rendered (off_model, off);
	}
}


DECLARE_SCRIPT_TT (JFW_Iron_Curtain_Zone, "Time:float,TimerNum:int,Animation1:string,Animation1Loc:vector3,Animation1Facing:float,Animation1Slot:int,Animation2:string,Animation2Loc:vector3,Animation2Facing:float,Animation2Slot:int,Player_Type:int,Animation3:string,Animation3Loc:vector3,Animation3Facing:float,Animation3Slot:int,BuildingID:int,Invunlerability_Time:float,Invunlerability_TimerNum:int,Animation1Msg:int,Animation2Msg:int,Animation3Msg:int,OffID:int,OnID:int,CinematicPowerOff:int")
{
	bool	Enabled;

	void	Created (GameObject * obj) override
	{
		Enabled = false;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
		Show_Charge_State (this, false, true);
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Enabled || !Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) {
			return ;
		}

		if (As_Vehicle (enterer) == nullptr) {
			return ;
		}

		Enabled = false;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));

		if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
			Show_Charge_State (this, false, false);
		}

		Play_Slot_Cinematic (this, obj, "2");

		StringClass params;
		params.Format ("%s,%s",
				Get_Parameter ("Invunlerability_Time"),
				Get_Parameter ("Invunlerability_TimerNum"));
		ScriptEngine::Attach_Script (enterer, "JFW_Invulnerability_Timer", params);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		if (!Enabled) {
			return ;
		}

		if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
			Show_Charge_State (this, false, false);
		}

		Play_Slot_Cinematic (this, obj, "3");
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) {
			return ;
		}

		if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
			Show_Charge_State (this, false, false);
		}

		Play_Slot_Cinematic (this, obj, "1");
		Enabled = true;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Animation1Msg")) {
			Show_Charge_State (this, true, false);
		}

		if (type == Get_Int_Parameter ("Animation2Msg")) {
			Play_Slot_Cinematic (this, obj, "3");
		}

		if (type == Get_Int_Parameter ("Animation3Msg")) {
			Show_Charge_State (this, false, true);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
	}
};


DECLARE_SCRIPT_TT (JFW_Chrono_Controler_Zone, "Spawner_Time:float,Spawner_TimerNum:int,Spawner_Preset:string,Spawner_Location:vector3,Chrono_Time:float,Chrono_TimerNum:int,Message:int,Animation1:string,Animation1Loc:vector3,Animation1Facing:float,Animation1Slot:int,Animation2:string,Animation2Loc:vector3,Animation2Facing:float,Animation2Slot:int,Animation3:string,Animation3Loc:vector3,Animation3Facing:float,Animation3Slot:int,BuildingID:int,ZAdjust:float,Animation1Msg:int,Animation2Msg:int,Animation3Msg:int,OffID:int,OnID:int,CinematicPowerOff:int,Activate_Time:float,Activate_TimerNum:int,DeathMessage:int")
{
	bool		Enabled;
	bool		Activating;
	Vector3	Destination;

	void	Created (GameObject * obj) override
	{
		Enabled		= false;
		Activating	= false;
		Destination.Set (0.0f, 0.0f, 0.0f);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Spawner_Time"),
				Get_Int_Parameter ("Spawner_TimerNum"));
		Show_Charge_State (this, false, true);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {

			//	The beacon that named the destination.
			Destination	= ScriptEngine::Get_Position (sender);
			Activating	= true;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Activate_Time"),
					Get_Int_Parameter ("Activate_TimerNum"));
		}

		if (type == Get_Int_Parameter ("DeathMessage")) {
			Activating = false;
		}

		if (type == Get_Int_Parameter ("Animation1Msg")) {
			Show_Charge_State (this, true, false);
		}

		if (type == Get_Int_Parameter ("Animation2Msg")) {
			Play_Slot_Cinematic (this, obj, "3");
		}

		if (type == Get_Int_Parameter ("Animation3Msg")) {
			Show_Charge_State (this, false, true);
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Spawner_Time"),
					Get_Int_Parameter ("Spawner_TimerNum"));
		}
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Enabled || (As_Vehicle (enterer) == nullptr)) {
			return ;
		}

		Enabled = false;
		if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
			Show_Charge_State (this, false, false);
		}

		Play_Slot_Cinematic (this, obj, "2");

		Vector3 landing = Destination;
		landing.Z += Get_Float_Parameter ("ZAdjust");
		ScriptEngine::Set_Position (enterer, landing);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if ((number == Get_Int_Parameter ("Activate_TimerNum")) && Activating) {

			if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
				Show_Charge_State (this, false, false);
			}

			Play_Slot_Cinematic (this, obj, "1");
			Enabled = true;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Chrono_Time"),
					Get_Int_Parameter ("Chrono_TimerNum"));
		}

		if (number == Get_Int_Parameter ("Spawner_TimerNum")) {
			ScriptEngine::Create_Object (Get_Parameter ("Spawner_Preset"),
					Get_Vector3_Parameter ("Spawner_Location"));
		}

		if ((number == Get_Int_Parameter ("Chrono_TimerNum")) && Enabled) {

			Enabled = false;
			if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
				Show_Charge_State (this, false, false);
			}

			Play_Slot_Cinematic (this, obj, "3");
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		if (!Enabled) {
			return ;
		}

		if (Get_Int_Parameter ("CinematicPowerOff") == 1) {
			Show_Charge_State (this, false, false);
		}

		Play_Slot_Cinematic (this, obj, "3");
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
		SAVE_VARIABLE (Destination, 2);
		SAVE_VARIABLE (Activating, 3);
	}
};


/******************************************************************************
*
*     Construction yards
*
*     One repair sweep over the team's buildings, plus whatever else the
*     particular yard also looks after.
*
******************************************************************************/

class	JFW_Conyard_Base : public ScriptImpClass
{
protected:
	bool	Disabled;

	virtual void	Do_Repairs (GameObject * obj) = 0;

	//	Two of the four donor scripts stop polling for good once disabled.
	virtual bool	Polls_While_Disabled (void)			{ return true; }

	//	Only the two that watch for a revive can come back.
	virtual bool	Listens_For_Revive (void)				{ return false; }

	void	Created (GameObject * obj) override
	{
		Disabled = false;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Disabled = true;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("DisableCustom")) {
			Disabled = true;
		} else if (Listens_For_Revive() && (type == CUSTOM_EVENT_BUILDING_REVIVED)) {
			Disabled = false;
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (!Disabled) {
			Do_Repairs (obj);
		}

		if (!Disabled || Polls_While_Disabled()) {
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
		}
	}

	//	Every one of them starts with the buildings.
	void	Repair_The_Base (GameObject * obj)
	{
		ScriptEngine::Repair_All_Buildings_By_Team (ScriptEngine::Get_Player_Type (obj),
				ScriptEngine::Get_ID (obj), Get_Float_Parameter ("Health"));
	}

public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Disabled, 1);
	}
};


REGISTER_SCRIPT_TT (JFW_Conyard, "Time:float,TimerNum:int,DisableCustom:int,Health:float")
class	JFW_Conyard : public JFW_Conyard_Base
{
protected:
	bool	Listens_For_Revive (void) override			{ return true; }

	void	Do_Repairs (GameObject * obj) override
	{
		Repair_The_Base (obj);
	}
};


REGISTER_SCRIPT_TT (JFW_Conyard_Vehicle_Buildings, "Time:float,TimerNum:int,DisableCustom:int,Health:float,Message:int")
class	JFW_Conyard_Vehicle_Buildings : public JFW_Conyard_Base
{
protected:
	bool	Polls_While_Disabled (void) override			{ return false; }

	void	Do_Repairs (GameObject * obj) override
	{
		Repair_The_Base (obj);
		ScriptEngine::Repair_All_Static_Vehicles_By_Team (ScriptEngine::Get_Player_Type (obj),
				Get_Int_Parameter ("Message"));
	}
};


REGISTER_SCRIPT_TT (JFW_Conyard_Radius, "Time:float,TimerNum:int,DisableCustom:int,Health:float,Radius:float")
class	JFW_Conyard_Radius : public JFW_Conyard_Base
{
protected:
	bool	Polls_While_Disabled (void) override			{ return false; }

	void	Do_Repairs (GameObject * obj) override
	{
		ScriptEngine::Repair_All_Buildings_By_Team_Radius (ScriptEngine::Get_Player_Type (obj),
				ScriptEngine::Get_ID (obj), Get_Float_Parameter ("Health"), Get_Float_Parameter ("Radius"));
	}
};


REGISTER_SCRIPT_TT (JFW_Conyard_Turrets, "Time:float,TimerNum:int,DisableCustom:int,Health:float")
class	JFW_Conyard_Turrets : public JFW_Conyard_Base
{
protected:
	bool	Listens_For_Revive (void) override			{ return true; }

	void	Do_Repairs (GameObject * obj) override
	{
		Repair_The_Base (obj);
		ScriptEngine::Repair_All_Turrets_By_Team (ScriptEngine::Get_Player_Type (obj),
				Get_Float_Parameter ("Health"));
	}
};


/******************************************************************************
*
*     Flying infantry
*
*     Five ways to ask for the same toggle.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Flying_Infantry, "")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Toggle_Fly_Mode (obj);
		Destroy_Script();
	}
};


DECLARE_SCRIPT_TT (JFW_Flying_Infantry_Custom, "Message:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {
			ScriptEngine::Toggle_Fly_Mode (obj);
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Flying_Infantry_Powerup, "")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			ScriptEngine::Toggle_Fly_Mode (sender);
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Flying_Infantry_Enter, "Player_Type:int")
{
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) {
			ScriptEngine::Toggle_Fly_Mode (enterer);
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Flying_Infantry_Exit, "Player_Type:int")
{
	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		if (Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type"))) {
			ScriptEngine::Toggle_Fly_Mode (exiter);
		}
	}
};


/******************************************************************************
*
*     Odds and ends of damage
*
******************************************************************************/

/*
**	A drone that keeps taking damage once the named preset has touched it, and
**	stops when something repairs it.
*/
DECLARE_SCRIPT_TT (JFW_Drone_Damage, "RepairStop:int,Preset:string,Amount:int,Custom:int")
{
	bool	Bleeding;

	void	Created (GameObject * obj) override
	{
		Bleeding = false;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if ((type != 0) || !Bleeding) {
			return ;
		}

		float amount = (float)Get_Int_Parameter ("Amount");
		ScriptEngine::Set_Health (obj, ScriptEngine::Get_Health (obj) - amount);
		ScriptEngine::Set_Shield_Strength (obj, ScriptEngine::Get_Shield_Strength (obj) - amount);
		ScriptEngine::Send_Custom_Event (obj, obj, 0, 0, 1.0f);
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		//	Whoever is riding is not the thing that hit us.
		GameObject * source = ScriptEngine::Get_Vehicle (damager);
		if (source != nullptr) {
			damager = source;
		}

		if ((amount < 0.0f) && (Get_Int_Parameter ("RepairStop") == 1)) {
			Bleeding = false;
		}

		const char * preset = (damager != nullptr) ? ScriptEngine::Get_Preset_Name (damager) : nullptr;
		if ((preset != nullptr) && (::_stricmp (Get_Parameter ("Preset"), preset) == 0)) {
			Bleeding = true;
			ScriptEngine::Send_Custom_Event (obj, damager, Get_Int_Parameter ("Custom"), 0, 0);
			ScriptEngine::Send_Custom_Event (obj, obj, 0, 0, 0);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Bleeding, 1);
	}
};


/*
**	Shooting a vehicle with the right weapon throws whoever is in it out.
*/
class	JFW_Vehicle_Thief_Base : public ScriptImpClass
{
protected:
	virtual void	Announce (GameObject * obj, GameObject * damager)		{ }

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		if (!Is_Player_Type (damager, Get_Int_Parameter ("Player_Type"))) {
			return ;
		}

		if (!Weapon_Is (damager, Get_Parameter ("Weapon"))) {
			return ;
		}

		Announce (obj, damager);
		ScriptEngine::Force_Occupants_Exit (obj);
	}
};


REGISTER_SCRIPT_TT (JFW_Vehicle_Thief, "Weapon:string,Player_Type:int")
class	JFW_Vehicle_Thief : public JFW_Vehicle_Thief_Base
{
};


REGISTER_SCRIPT_TT (JFW_Vehicle_Thief_2, "Weapon:string,Player_Type:int,Sound:string")
class	JFW_Vehicle_Thief_2 : public JFW_Vehicle_Thief_Base
{
protected:
	void	Announce (GameObject * obj, GameObject * damager) override
	{
		ScriptEngine::Create_Sound (Get_Parameter ("Sound"),
				ScriptEngine::Get_Position (damager), damager);
	}
};


/******************************************************************************
*
*     Carryalls
*
*     A carryable vehicle shouts when it is shot at; a controller passes the
*     shout on to every carryall it knows about; whichever carryall the
*     shooter is flying picks the vehicle up.
*
******************************************************************************/

enum
{
	CARRY_REQUEST_PICKUP		= 9876,	//	param = the ID of whoever asked
	CARRY_LOAD_DESTROYED		= 9875,	//	param = the ID of whoever asked
	CARRY_FLEET					= 9877	//	param = 6 joining, 7 leaving
};

enum
{
	CARRY_JOINING	= 6,
	CARRY_LEAVING	= 7
};


DECLARE_SCRIPT_TT (JFW_Carryable_Vehicle, "Only_Empty:int,Control_ID:int")
{
	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		bool only_empty = Get_Bool_Parameter ("Only_Empty");
		if (only_empty && (ScriptEngine::Get_Vehicle_Occupant_Count (obj) != 0)) {
			return ;
		}

		Send_To_ID (obj, Get_Int_Parameter ("Control_ID"), CARRY_REQUEST_PICKUP,
				ScriptEngine::Get_ID (damager));
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Send_To_ID (obj, Get_Int_Parameter ("Control_ID"), CARRY_LOAD_DESTROYED,
				ScriptEngine::Get_ID (killer));
	}
};


DECLARE_SCRIPT_TT (JFW_CarryAll, "Bone_Name:string,Cost:int,Control_ID:int")
{
	bool	Carrying;
	int	LoadID;
	int	PilotID;

	void	Created (GameObject * obj) override
	{
		PilotID	= 0;
		LoadID	= 0;
		Carrying	= false;
		Send_To_ID (obj, Get_Int_Parameter ("Control_ID"), CARRY_FLEET, CARRY_JOINING);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if ((type == CUSTOM_EVENT_VEHICLE_ENTERED) && (PilotID == 0)) {
			PilotID = ScriptEngine::Get_ID (sender);
		}

		if ((type == CUSTOM_EVENT_VEHICLE_EXITED) && (ScriptEngine::Get_ID (sender) == PilotID)) {
			PilotID = 0;
		}

		//	Somebody shot a carryable vehicle, and it was our pilot who did it.
		if ((type == CARRY_REQUEST_PICKUP) && ((int)param == PilotID) && !Carrying) {

			Carrying	= true;
			LoadID	= ScriptEngine::Get_ID (sender);

			const char * bone = Get_Parameter ("Bone_Name");
			ScriptEngine::Set_Position (sender, ScriptEngine::Get_Bone_Position (obj, bone));
			ScriptEngine::Attach_To_Object_Bone (sender, obj, bone);
		}

		//	The fleet is full, or we told it we were leaving.
		if ((type == CARRY_FLEET) && ((int)param == CARRY_LEAVING)) {

			GameObject * pilot = (PilotID != 0) ? ScriptEngine::Find_Object (PilotID) : nullptr;
			if (pilot != nullptr) {
				ScriptEngine::Give_Money (pilot, Get_Float_Parameter ("Cost"), false);
			}
			ScriptEngine::Destroy_Object (obj);
		}

		if ((type == CARRY_LOAD_DESTROYED) && Carrying && (LoadID == ScriptEngine::Get_ID (sender))) {
			Carrying = false;
		}
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		//	The pilot shooting their own load puts it down where it hangs.
		if (!Carrying || (PilotID == 0) || (damager != ScriptEngine::Find_Object (PilotID))) {
			return ;
		}

		GameObject * load = ScriptEngine::Find_Object (LoadID);
		if (load == nullptr) {
			Carrying = false;
			return ;
		}

		float				facing	= ScriptEngine::Get_Facing (load);
		float				health	= ScriptEngine::Get_Health (load);
		float				shield	= ScriptEngine::Get_Shield_Strength (load);
		const char *	preset	= ScriptEngine::Get_Preset_Name (load);

		ScriptEngine::Destroy_Object (load);

		GameObject * dropped = ScriptEngine::Create_Object (preset,
				ScriptEngine::Get_Bone_Position (obj, Get_Parameter ("Bone_Name")));
		if (dropped != nullptr) {
			ScriptEngine::Set_Health (dropped, health);
			ScriptEngine::Set_Shield_Strength (dropped, shield);
			ScriptEngine::Set_Facing (dropped, facing);
		}

		Carrying = false;
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Send_To_ID (obj, Get_Int_Parameter ("Control_ID"), CARRY_FLEET, CARRY_LEAVING);
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Carrying, 1);
		SAVE_VARIABLE (LoadID, 2);
		SAVE_VARIABLE (PilotID, 3);
	}
};


DECLARE_SCRIPT_TT (JFW_Carry_Control, "Max_Carryalls:int")
{
	enum { MAX_SLOTS = 20 };

	int	Slots[MAX_SLOTS];
	int	Count;

	void	Created (GameObject * obj) override
	{
		for (int index = 0; index < MAX_SLOTS; index ++) {
			Slots[index] = 0;
		}
		Count = 0;
	}

	void	Relay (GameObject * from, int type, intptr_t param)
	{
		for (int index = 0; index < MAX_SLOTS; index ++) {
			if (Slots[index] == 0) {
				continue;
			}

			GameObject * carryall = ScriptEngine::Find_Object (Slots[index]);
			if (carryall != nullptr) {
				ScriptEngine::Send_Custom_Event (from, carryall, type, param, 0);
			}
		}
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		int max = Get_Int_Parameter ("Max_Carryalls");
		if (max > MAX_SLOTS) {
			max = MAX_SLOTS;
		}

		if ((type == CARRY_FLEET) && ((int)param == CARRY_JOINING)) {

			Count ++;
			if (Count > max) {
				//	One too many.  Tell it to refund its buyer and go away.
				ScriptEngine::Send_Custom_Event (obj, sender, CARRY_FLEET, CARRY_LEAVING, 0);
				return ;
			}

			//
			//	The donor filled slot N only when every earlier slot was
			//	occupied, so a hole in the middle of the list stayed a hole
			//	for the rest of the match.  First free slot wins here.
			//
			for (int index = 0; index < max; index ++) {
				if (Slots[index] == 0) {
					Slots[index] = ScriptEngine::Get_ID (sender);
					return ;
				}
			}

		} else if ((type == CARRY_FLEET) && ((int)param == CARRY_LEAVING)) {

			Count --;
			if (Count < 0) {
				Count = 0;
			}

			int leaver = ScriptEngine::Get_ID (sender);
			for (int index = 0; index < MAX_SLOTS; index ++) {
				if (Slots[index] == leaver) {
					Slots[index] = 0;
				}
			}

		} else if ((type == CARRY_REQUEST_PICKUP) || (type == CARRY_LOAD_DESTROYED)) {

			//	Passed on from the carryable vehicle, so it stays the sender.
			Relay (sender, type, param);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Slots, 1);
		SAVE_VARIABLE (Count, 2);
	}
};


/******************************************************************************
*
*     Seeing what is hidden
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Sensor_Array, "Distance:float,Indicator:string,IndicatorZOffset:float,Time:float,TimerNum:int,Player_Type:int")
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) {
			return ;
		}

		Vector3 offset (0.0f, 0.0f, Get_Float_Parameter ("IndicatorZOffset"));
		ScriptEngine::Create_Effect_All_Stealthed_Objects_Area (ScriptEngine::Get_Position (obj),
				Get_Float_Parameter ("Distance"), Get_Parameter ("Indicator"), offset,
				Get_Int_Parameter ("Player_Type"));

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}
};


DECLARE_SCRIPT_TT (JFW_Sonar_Pulse, "Spy_Script:string,Sub_Preset_1:string,Sub_Preset_2:string,Indicator:string,ZAdjust:float,WaterIndicator:string,WaterZ:float,Sound:string,Time:float,ResetTime:float")
{
	bool	Charging;

	void	Created (GameObject * obj) override
	{
		Charging = false;
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		//	Somebody riding a vehicle is not who this is for.
		if (Charging || (ScriptEngine::Get_Vehicle (enterer) != nullptr)) {
			return ;
		}

		if (!ScriptEngine::Is_Script_Attached (enterer, Get_Parameter ("Spy_Script"))) {
			return ;
		}

		Charging = true;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {

			ScriptEngine::Create_2D_Sound (Get_Parameter ("Sound"));

			//	Above each submarine, and again on the surface of the water.
			for (int which = 1; which <= 2; which ++) {
				StringClass preset_name;
				preset_name.Format ("Sub_Preset_%d", which);

				const char * preset = Get_Parameter (preset_name);
				ScriptEngine::Create_Effect_All_Of_Preset (Get_Parameter ("Indicator"), preset,
						Get_Float_Parameter ("ZAdjust"), false);
				ScriptEngine::Create_Effect_All_Of_Preset (Get_Parameter ("WaterIndicator"), preset,
						Get_Float_Parameter ("WaterZ"), true);
			}

			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("ResetTime"), 2);

		} else if (number == 2) {
			Charging = false;
		}
	}
};


/******************************************************************************
*
*     Radar jammers
*
*     A unit of the named preset parked within range of the enemy's
*     communications centre.
*
******************************************************************************/

class	JFW_Radar_Jammer_Base : public ScriptImpClass
{
protected:
	virtual void	Jammed (GameObject * obj)		{ }
	virtual void	Cleared (GameObject * obj)		{ }

	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Destroy_Script();
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("DisableCustom")) {
			Destroy_Script();
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) {
			return ;
		}

		int team = ScriptEngine::Get_Player_Type (obj);
		if (ScriptEngine::Is_Base_Powered (team)) {

			//	The centre the jammer has to be near is named by definition.
			const char *	centre_preset	= ScriptEngine::Get_Definition_Name (Get_Int_Parameter ("CenterID"));
			GameObject *	centre			= (centre_preset != nullptr)
					? ScriptEngine::Find_Object_By_Preset (2, centre_preset) : nullptr;

			if (centre != nullptr) {
				bool in_range = ScriptEngine::Is_Unit_In_Range (Get_Parameter ("Preset"),
						Get_Float_Parameter ("Range"), ScriptEngine::Get_Position (centre),
						Enemy_Of (team), false);

				if (in_range) {
					Jammed (obj);
				} else {
					Cleared (obj);
				}
			}
		}

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}
};


REGISTER_SCRIPT_TT (JFW_Radar_Jammer, "Time:float,TimerNum:int,DisableCustom:int,Preset:string,CenterID:int,Range:float,StartSound1:string,StopSound1:string,StartSound2:string,StopSound2:string")
class	JFW_Radar_Jammer : public JFW_Radar_Jammer_Base
{
	bool	RadarOn;

protected:
	void	Created (GameObject * obj) override
	{
		RadarOn = true;
		JFW_Radar_Jammer_Base::Created (obj);
	}

	void	Jammed (GameObject * obj) override
	{
		if (!RadarOn) {
			return ;
		}

		int team = ScriptEngine::Get_Player_Type (obj);
		ScriptEngine::Create_2D_Sound_Team (team, Get_Parameter ("StartSound1"));
		ScriptEngine::Create_2D_Sound_Team (Enemy_Of (team), Get_Parameter ("StartSound2"));

		RadarOn = false;
		ScriptEngine::Enable_Base_Radar (team, false);
	}

	void	Cleared (GameObject * obj) override
	{
		if (RadarOn) {
			return ;
		}

		int team = ScriptEngine::Get_Player_Type (obj);
		ScriptEngine::Create_2D_Sound_Team (team, Get_Parameter ("StopSound1"));
		ScriptEngine::Create_2D_Sound_Team (Enemy_Of (team), Get_Parameter ("StopSound2"));

		RadarOn = true;
		ScriptEngine::Enable_Base_Radar (team, true);
	}

public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (RadarOn, 1);
	}
};


REGISTER_SCRIPT_TT (JFW_Radar_Jammer_Sound, "Time:float,TimerNum:int,DisableCustom:int,Preset:string,CenterID:int,Range:float,Sound:string")
class	JFW_Radar_Jammer_Sound : public JFW_Radar_Jammer_Base
{
protected:
	void	Jammed (GameObject * obj) override
	{
		ScriptEngine::Create_2D_Sound_Team (ScriptEngine::Get_Player_Type (obj), Get_Parameter ("Sound"));
	}
};


/******************************************************************************
*
*     The chrono harvester
*
*     Drives out along one of three waypaths, sits in the field while ten
*     crystals disappear one at a time, teleports home and docks.
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Chrono_Harvester_Logic, "Harvester_Preset:string,Explosion_Preset:string")
{
	int	HarvesterID;
	bool	Enabled;

	void	Created (GameObject * obj) override
	{
		Enabled		= true;
		HarvesterID	= 0;
		ScriptEngine::Send_Custom_Event (obj, obj, 100001, 0, 0);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if ((type == 100001) && Enabled) {

			float		facing	= ScriptEngine::Get_Facing (obj);
			Vector3	position	= ScriptEngine::Get_Position (obj);

			ScriptEngine::Create_Explosion (Get_Parameter ("Explosion_Preset"), position, nullptr);

			GameObject * harvester = ScriptEngine::Create_Object (Get_Parameter ("Harvester_Preset"), position);
			if (harvester != nullptr) {
				ScriptEngine::Set_Facing (harvester, facing);
				ScriptEngine::Send_Custom_Event (obj, harvester, 100004, Float_To_Param (facing), 0);
			}

		} else if (type == 100002) {

			//	The spawner died and is taking the harvester with it.
			GameObject * harvester = (HarvesterID != 0) ? ScriptEngine::Find_Object (HarvesterID) : nullptr;
			if (harvester != nullptr) {
				ScriptEngine::Apply_Damage (harvester, 99999, "Explosive", nullptr);
				ScriptEngine::Destroy_Object (obj);
			}

		} else if (type == 100003) {
			HarvesterID = (int)param;
		} else if (type == 100005) {
			Enabled = false;
		} else if (type == 100006) {
			HarvesterID = 0;
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (HarvesterID, 1);
		SAVE_VARIABLE (Enabled, 2);
	}
};


DECLARE_SCRIPT_TT (JFW_Chrono_Harvester_Attached, "LogicID=0:int,Number_Of_Paths=0:int,Waypath1_Dock2Field=0:int,Waypath1_Field2Dock=0:int,Waypath1_Tib_Zone:vector3,Waypath2_Dock2Field=0:int,Waypath2_Field2Dock=0:int,Waypath2_Tib_Zone:vector3,Waypath3_Dock2Field=0:int,Waypath3_Field2Dock=0:int,Waypath3_Tib_Zone:vector3,Harvest_Animation:string,Dock_Animation:string,Tiberium_Idle_Time=45.00:float,Dock_Location:vector3,Dock_Idle_Time=10.00:float,Dock_Credits=400.00:float,Harvester_Create_Idle_Time=5.00:float,Crystal_Preset:string,Chrono_Location:vector3")
{
	enum { CRYSTAL_COUNT = 10 };

	bool	Homeward;
	float	Facing;
	int	CrystalIndex;
	int	PathNumber;
	int	Crystals[CRYSTAL_COUNT];

	bool	Has_Crystals (void)
	{
		const char * preset = Get_Parameter ("Crystal_Preset");
		return (preset != nullptr) && (::_stricmp (preset, "0") != 0);
	}

	//	Which of the three tiberium patches this trip is working.
	Vector3	Field_Position (void)
	{
		StringClass name;
		name.Format ("Waypath%d_Tib_Zone", PathNumber);
		return Get_Vector3_Parameter (name);
	}

	void	Pick_A_Path (void)
	{
		int paths = Get_Int_Parameter ("Number_Of_Paths");
		if (paths > 3) {
			paths = 3;
		}
		if (paths < 1) {
			paths = 1;
		}

		PathNumber = ScriptEngine::Get_Random_Int (1, paths + 1);
	}

	//	The outward leg, along whichever waypath this trip drew.
	void	Drive_To_The_Field (GameObject * obj)
	{
		StringClass waypath_name;
		waypath_name.Format ("Waypath%d_Dock2Field", PathNumber);

		ActionParamsStruct params;
		params.MoveArrivedDistance = 100;
		params.Set_Basic (this, 100, 1);
		params.Set_Movement ((GameObject *)nullptr, 1.0f, 0.1f);
		params.WaypathID			= Get_Int_Parameter (waypath_name);
		params.WaypathSplined	= true;
		params.AttackActive		= false;
		params.AttackCheckBlocked	= false;

		ScriptEngine::Action_Goto (obj, params);
	}

	//	Ten crystals scattered around the patch, to be eaten one by one.
	void	Plant_Crystals (void)
	{
		Clear_Crystals();
		if (!Has_Crystals()) {
			return ;
		}

		Vector3 root = Field_Position();

		for (int index = 0; index < CRYSTAL_COUNT; index ++) {

			Vector3 position = root;
			int x_drift = ScriptEngine::Get_Random_Int (1, 3);
			int y_drift = ScriptEngine::Get_Random_Int (1, 3);

			if (x_drift == 1) {
				position.X += ScriptEngine::Get_Random (0.0f, 5.001f);
			} else if (x_drift == 2) {
				position.X -= ScriptEngine::Get_Random (0.0f, 5.001f);
			}

			if (y_drift == 1) {
				position.Y += ScriptEngine::Get_Random (0.0f, 5.001f);
			} else if (y_drift == 2) {
				position.Y -= ScriptEngine::Get_Random (0.0f, 5.001f);
			}

			GameObject * crystal = ScriptEngine::Create_Object (Get_Parameter ("Crystal_Preset"), position);
			if (crystal != nullptr) {
				ScriptEngine::Disable_All_Collisions (crystal);
				Crystals[index] = ScriptEngine::Get_ID (crystal);
			}
		}
	}

	void	Clear_Crystals (void)
	{
		for (int index = 0; index < CRYSTAL_COUNT; index ++) {
			Destroy_By_ID (Crystals[index]);
			Crystals[index] = 0;
		}
	}

	void	Play_Animation (GameObject * obj, const char * which, bool looping)
	{
		const char * animation = Get_Parameter (which);
		if ((animation != nullptr) && (::_stricmp (animation, "0") != 0)) {
			ScriptEngine::Set_Animation (obj, animation, looping, nullptr, 0.0f, -1.0f, false);
		}
	}

	void	Created (GameObject * obj) override
	{
		Homeward			= false;
		Facing			= 0.0f;
		CrystalIndex	= 0;
		PathNumber		= 1;

		for (int index = 0; index < CRYSTAL_COUNT; index ++) {
			Crystals[index] = 0;
		}

		Send_To_ID (obj, Get_Int_Parameter ("LogicID"), 100003, ScriptEngine::Get_ID (obj));

		Pick_A_Path();
		Plant_Crystals();
		Drive_To_The_Field (obj);
		ScriptEngine::Start_Timer (obj, this, 0.1f, 5);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		//	The dock's facing, so the harvester parks the way it was built.
		if (type == 100004) {
			Facing = Param_To_Float (param);
		}
	}

	void	Action_Complete (GameObject * obj, int action_id, ActionCompleteReason complete_reason) override
	{
		if (action_id == 1) {

			//	Arrived at the field.
			if (complete_reason == ACTION_COMPLETE_NORMAL) {
				Play_Animation (obj, "Harvest_Animation", true);
			}

			float idle = Get_Float_Parameter ("Tiberium_Idle_Time");
			ScriptEngine::Start_Timer (obj, this, idle / (float)CRYSTAL_COUNT, 1);
			ScriptEngine::Start_Timer (obj, this, idle / (float)CRYSTAL_COUNT, 4);
			ScriptEngine::Start_Timer (obj, this, idle, 2);

		} else if (action_id == 2) {

			//	Shuffled to the next spot inside the field.
			if (complete_reason == ACTION_COMPLETE_NORMAL) {
				Play_Animation (obj, "Harvest_Animation", true);
			}
			ScriptEngine::Start_Timer (obj, this, 10.0f, 1);

		} else if (action_id == 3) {

			//	Home again; back the last few metres into the dock.
			ActionParamsStruct params;
			params.MoveArrivedDistance = 100;
			params.Set_Basic (this, 100, 4);
			params.Dock_Vehicle (Get_Vector3_Parameter ("Dock_Location"), ScriptEngine::Get_Position (obj));
			params.Set_Movement (Get_Vector3_Parameter ("Dock_Location"), 1.0f, 0.5f, false);
			ScriptEngine::Action_Dock (obj, params);

		} else if (action_id == 4) {

			//	Docked.
			if (complete_reason == ACTION_COMPLETE_NORMAL) {
				ScriptEngine::Set_Facing (obj, Facing);
				Play_Animation (obj, "Dock_Animation", false);
			}
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Dock_Idle_Time"), 3);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if ((number == 1) && !Homeward) {

			//	Move a couple of metres and keep chewing.
			Play_Animation (obj, "Harvest_Animation", false);

			Vector3 spot = Field_Position();
			int x_drift = ScriptEngine::Get_Random_Int (1, 3);
			int y_drift = ScriptEngine::Get_Random_Int (1, 3);

			if (x_drift == 1) {
				spot.X += ScriptEngine::Get_Random (0.0f, 2.001f);
			} else if (x_drift == 2) {
				spot.X -= ScriptEngine::Get_Random (0.0f, 2.001f);
			}

			if (y_drift == 1) {
				spot.Y += ScriptEngine::Get_Random (0.0f, 2.001f);
			} else if (y_drift == 2) {
				spot.Y -= ScriptEngine::Get_Random (0.0f, 2.001f);
			}

			ActionParamsStruct params;
			params.MoveArrivedDistance = 100;
			params.Set_Basic (this, 100, 2);
			params.Set_Movement (spot, 1.0f, 0.1f, false);
			ScriptEngine::Action_Goto (obj, params);

		} else if (number == 2) {

			//	Full.  Teleport to the chrono pad and drive the return leg.
			Homeward = true;
			Play_Animation (obj, "Harvest_Animation", false);
			ScriptEngine::Set_Position (obj, Get_Vector3_Parameter ("Chrono_Location"));

			StringClass waypath_name;
			waypath_name.Format ("Waypath%d_Field2Dock", PathNumber);

			ActionParamsStruct params;
			params.MoveArrivedDistance = 100;
			params.Set_Basic (this, 100, 3);
			params.Set_Movement ((GameObject *)nullptr, 1.0f, 0.1f);
			params.WaypathID			= Get_Int_Parameter (waypath_name);
			params.WaypathSplined	= true;
			params.AttackActive		= false;
			params.AttackCheckBlocked	= false;

			ScriptEngine::Action_Goto (obj, params);

		} else if (number == 3) {

			//	Unloaded.  Pay the team and go out again.
			Homeward = false;
			ScriptEngine::Give_Money (obj, Get_Float_Parameter ("Dock_Credits"), true);

			Pick_A_Path();
			Plant_Crystals();
			Drive_To_The_Field (obj);

		} else if (number == 4) {

			//	One crystal per tenth of the time spent in the field.
			if (!Has_Crystals()) {
				return ;
			}

			if (CrystalIndex < CRYSTAL_COUNT) {
				Destroy_By_ID (Crystals[CrystalIndex]);
				Crystals[CrystalIndex] = 0;
				CrystalIndex ++;
			}

			if (CrystalIndex < CRYSTAL_COUNT) {
				ScriptEngine::Start_Timer (obj, this,
						Get_Float_Parameter ("Tiberium_Idle_Time") / (float)CRYSTAL_COUNT, 4);
			} else {
				CrystalIndex = 0;
			}

		} else if (number == 5) {
			CrystalIndex = 0;
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Clear_Crystals();

		Send_To_ID (obj, Get_Int_Parameter ("LogicID"), 100006, 0);

		//	And ask the spawner for a replacement after a pause.
		GameObject * logic = ScriptEngine::Find_Object (Get_Int_Parameter ("LogicID"));
		if (logic != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, logic, 100001, 0,
					Get_Float_Parameter ("Harvester_Create_Idle_Time"));
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (PathNumber, 1);
		SAVE_VARIABLE (Crystals, 2);
		SAVE_VARIABLE (Homeward, 3);
		SAVE_VARIABLE (CrystalIndex, 4);
		SAVE_VARIABLE (Facing, 5);
	}
};


DECLARE_SCRIPT_TT (JFW_Chrono_Harvester_Kill, "LogicID=0:int")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Send_To_ID (obj, Get_Int_Parameter ("LogicID"), 100002, 0);
	}
};


DECLARE_SCRIPT_TT (JFW_Chrono_Harvester_Spawn_Kill, "LogicID=0:int")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Send_To_ID (obj, Get_Int_Parameter ("LogicID"), 100005, 0);
	}
};


/******************************************************************************
*
*     Stealth generators
*
*     A controller keeps a list of the generators that are switched on, and
*     twice a second cloaks everything of the right side standing near one.
*     Anything whose preset is a stealth unit in its own right is left alone,
*     because its own cloak is not the field's to switch off.
*
******************************************************************************/

struct	StealthGeneratorStruct
{
	int	ObjectID;
	float	Range;
	int	TeamID;
	bool	Active;

	//	DynamicVectorClass compares elements when it searches and removes.
	bool	operator == (const StealthGeneratorStruct & other) const
	{
		return (ObjectID == other.ObjectID);
	}

	bool	operator != (const StealthGeneratorStruct & other) const
	{
		return (ObjectID != other.ObjectID);
	}
};


DECLARE_SCRIPT_TT (JFW_Global_Stealth_Controller, "Update_Delay=2.0:float,Timer_Number=687:int")
{
	DynamicVectorClass<StealthGeneratorStruct>	Generators;

	int	Find_Generator (int object_id)
	{
		for (int index = 0; index < Generators.Count(); index ++) {
			if (Generators[index].ObjectID == object_id) {
				return index;
			}
		}

		return -1;
	}

	void	Add_Generator (int object_id, float range, int team)
	{
		//
		//	The donor's duplicate check never looked at the last node, so
		//	re-registering the newest generator appended a second copy.
		//
		int index = Find_Generator (object_id);
		if (index < 0) {
			StealthGeneratorStruct empty = { object_id, range, team, false };
			Generators.Add (empty);
			return ;
		}

		Generators[index].Range	= range;
		Generators[index].TeamID	= team;
		Generators[index].Active	= false;
	}

	void	Remove_Generator (int object_id)
	{
		for (int index = Generators.Count() - 1; index >= 0; index --) {
			if (Generators[index].ObjectID == object_id) {
				Generators.Delete (index);
			}
		}
	}

	void	Update_Generator (int object_id, bool active)
	{
		int index = Find_Generator (object_id);
		if (index >= 0) {
			Generators[index].Active = active;
		}
	}

	void	Update_Generator (int object_id, bool active, int team)
	{
		int index = Find_Generator (object_id);
		if (index >= 0) {
			Generators[index].Active	= active;
			Generators[index].TeamID	= team;
		}
	}

	//	Whether any live generator of this object's own side covers it.
	bool	Is_Covered (SmartGameObj * unit, int team)
	{
		Vector3 unit_position;
		unit->Get_Position (&unit_position);

		for (int index = 0; index < Generators.Count(); index ++) {

			const StealthGeneratorStruct & generator = Generators[index];
			if (!generator.Active || (generator.TeamID != team)) {
				continue;
			}

			GameObject * source = ScriptEngine::Find_Object (generator.ObjectID);
			if (source == nullptr) {
				continue;
			}

			Vector3 offset = ScriptEngine::Get_Position (source) - unit_position;
			if (offset.Length2() <= (generator.Range * generator.Range)) {
				return true;
			}
		}

		return false;
	}

	/*
	**	Was an engine export in the donor.  It walks a list the script itself
	**	owns, so it lives with the script here.
	*/
	void	Ranged_Stealth_On_Team (void)
	{
		for (SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
				node != nullptr; node = node->Next()) {

			SmartGameObj * unit = node->Data();
			if (unit == nullptr) {
				continue;
			}

			//	A unit that stealths on its own is not the field's to manage.
			if (unit->Get_Definition().Is_Stealth_Unit()) {
				continue;
			}

			VehicleGameObj *	vehicle	= unit->As_VehicleGameObj();
			SoldierGameObj *	soldier	= unit->As_SoldierGameObj();
			if ((vehicle == nullptr) && (soldier == nullptr)) {
				continue;
			}

			//	A deployed vehicle answers to whoever locked it, not to the
			//	side of whoever last drove it.
			int team = unit->Get_Player_Type();
			if ((vehicle != nullptr) && (vehicle->Get_Lock_Team() != 2)) {
				team = vehicle->Get_Lock_Team();
			}

			unit->Enable_Stealth (Is_Covered (unit, team));
		}
	}

	void	Created (GameObject * obj) override
	{
		Generators.Delete_All();
		ScriptEngine::Start_Timer (obj, this, 5.0f, Get_Int_Parameter ("Timer_Number"));
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		int sender_id = ScriptEngine::Get_ID (sender);

		switch (type) {

			case 1111:
				Add_Generator (sender_id, (float)param, PLAYERTYPE_GDI);
				break;

			case 1000:
				Add_Generator (sender_id, (float)param, PLAYERTYPE_NOD);
				break;

			case 2222:
				switch ((int)param) {
					case 0:	Update_Generator (sender_id, false);	break;
					case 1:	Update_Generator (sender_id, true, ScriptEngine::Get_Player_Type (sender));	break;
					case 2:	Update_Generator (sender_id, false);	break;
					case 3:	Update_Generator (sender_id, true);		break;
				}
				break;

			case 3333:
				Remove_Generator (sender_id);
				break;
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("Timer_Number")) {
			return ;
		}

		Ranged_Stealth_On_Team();
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Update_Delay"),
				Get_Int_Parameter ("Timer_Number"));
	}

	void	Destroyed (GameObject * obj) override
	{
		Generators.Delete_All();
	}
};


/*
**	The generators themselves.  Each registers with the controller, puts a
**	visible bubble around itself while it is running, and unregisters when it
**	dies.
*/
class	JFW_Stealth_Generator_Base : public ScriptImpClass
{
protected:
	int	BubbleID;

	GameObject *	Controller (void)
	{
		return ScriptEngine::Find_Object_By_Preset (2, Get_Parameter ("Stealth_Controller"));
	}

	void	Tell_Controller (GameObject * obj, int type, intptr_t param, float delay = 0.0f)
	{
		GameObject * controller = Controller();
		if (controller != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, controller, type, param, delay);
		}
	}

	//	Register with the controller under the side we are on.
	void	Announce_Self (GameObject * obj, int team, float delay)
	{
		Tell_Controller (obj, (team == PLAYERTYPE_GDI) ? 1111 : 1000,
				Get_Int_Parameter ("Stealth_Range"), delay);
	}

	void	Raise_Bubble (GameObject * obj, const Vector3 & position, const char * bone)
	{
		if (BubbleID != 0) {
			return ;
		}

		GameObject * bubble = ScriptEngine::Create_Object (Get_Parameter ("SizePreset"), position);
		if (bubble == nullptr) {
			return ;
		}

		ScriptEngine::Set_Player_Type (bubble, ScriptEngine::Get_Player_Type (obj));
		if (bone != nullptr) {
			ScriptEngine::Attach_To_Object_Bone (bubble, obj, bone);
		}

		BubbleID = ScriptEngine::Get_ID (bubble);
	}

	void	Drop_Bubble (void)
	{
		Destroy_By_ID (BubbleID);
		BubbleID = 0;
	}

	void	Leave (GameObject * obj)
	{
		Tell_Controller (obj, 3333, 0);
		Drop_Bubble();
	}
};


REGISTER_SCRIPT_TT (JFW_Stealth_Generator_Building, "Timer_Number=876:int,Stealth_Range=50:int,Stealth_Controller:string,Team=1:int,SizePreset:string")
class	JFW_Stealth_Generator_Building : public JFW_Stealth_Generator_Base
{
	void	Created (GameObject * obj) override
	{
		BubbleID = 0;

		//	A building's side is configured, not read off the object.
		Announce_Self (obj, (Get_Int_Parameter ("Team") == 1) ? PLAYERTYPE_GDI : PLAYERTYPE_NOD, 2.0f);
		Tell_Controller (obj, 2222, 3, 3.0f);

		Raise_Bubble (obj, ScriptEngine::Get_Position (obj), nullptr);
		ScriptEngine::Start_Timer (obj, this, 2.0f, Get_Int_Parameter ("Timer_Number"));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("Timer_Number")) {
			return ;
		}

		//	No power, no field.
		if (!ScriptEngine::Get_Building_Power (obj)) {
			Tell_Controller (obj, 2222, 2);
			Drop_Bubble();
		}

		ScriptEngine::Start_Timer (obj, this, 2.0f, Get_Int_Parameter ("Timer_Number"));
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Leave (obj);
	}
};


REGISTER_SCRIPT_TT (JFW_Stealth_Generator_Vehicle, "Stealth_Range=50:int,Stealth_Controller:string,SizePreset:string")
class	JFW_Stealth_Generator_Vehicle : public JFW_Stealth_Generator_Base
{
	void	Created (GameObject * obj) override
	{
		BubbleID = 0;
		Announce_Self (obj, ScriptEngine::Get_Player_Type (obj), 0.5f);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		//	The field runs while somebody is aboard.
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			Tell_Controller (obj, 2222, 1);
			Raise_Bubble (obj, ScriptEngine::Get_Bone_Position (obj, "ROOTTRANSFORM"), "ROOTTRANSFORM");
		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			Tell_Controller (obj, 2222, 0);
			Drop_Bubble();
		}
	}

	void	Destroyed (GameObject * obj) override
	{
		Leave (obj);
	}
};


REGISTER_SCRIPT_TT (JFW_Stealth_Generator_Vehicle_Deploy, "Stealth_Range=50:int,Stealth_Controller:string,SizePreset:string")
class	JFW_Stealth_Generator_Vehicle_Deploy : public JFW_Stealth_Generator_Base
{
	bool	Deployed;

	void	Created (GameObject * obj) override
	{
		BubbleID	= 0;
		Deployed	= false;
		Announce_Self (obj, ScriptEngine::Get_Player_Type (obj), 0.5f);
		ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		//	The last one out of a deployed generator leaves it to their side.
		VehicleGameObj * vehicle = As_Vehicle (obj);
		if ((type == CUSTOM_EVENT_VEHICLE_EXITED) && Deployed
				&& (vehicle != nullptr) && (vehicle->Get_Occupant_Count() == 0)) {

			ScriptEngine::Update_Network_Object (obj);
			ScriptEngine::Set_Player_Type (obj, ScriptEngine::Get_Player_Type (sender));
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		ScriptEngine::Start_Timer (obj, this, 1.0f, 1);

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (vehicle == nullptr) {
			return ;
		}

		if (vehicle->Is_Immovable()) {

			if (!Deployed) {
				Deployed = true;
				vehicle->Set_Lock_Team (vehicle->Get_Player_Type());
				Tell_Controller (obj, 2222, 1);
				Raise_Bubble (obj, ScriptEngine::Get_Bone_Position (obj, "ROOTTRANSFORM"), "ROOTTRANSFORM");
			}

		} else if (Deployed) {

			Deployed = false;
			vehicle->Set_Lock_Team (2);
			Tell_Controller (obj, 2222, 0);
			Drop_Bubble();
		}
	}

	void	Destroyed (GameObject * obj) override
	{
		Leave (obj);
	}
};


/*
**	Something for the field to ignore.  It carries no behaviour; other scripts
**	ask whether it is attached.
*/
DECLARE_SCRIPT_TT (JFW_Stealth_Generator_Ignored, "")
{
};


/******************************************************************************
*
*     Losing power
*
******************************************************************************/

class	JFW_Low_Power_Base : public ScriptImpClass
{
protected:
	bool	LowPower;

	virtual void	Power_Lost (GameObject * obj) = 0;
	virtual void	Power_Restored (GameObject * obj)		{ }

	void	Created (GameObject * obj) override
	{
		LowPower = false;
		ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != 1) {
			return ;
		}

		bool powered = ScriptEngine::Is_Base_Powered (ScriptEngine::Get_Player_Type (obj));

		if (powered && LowPower) {
			LowPower = false;
			Power_Restored (obj);
		} else if (!powered && !LowPower) {
			LowPower = true;
			Power_Lost (obj);
		}

		ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
	}
};


REGISTER_SCRIPT_TT (JFW_Low_Power_Message, "Message:string")
class	JFW_Low_Power_Message : public JFW_Low_Power_Base
{
protected:
	void	Power_Lost (GameObject * obj) override
	{
		ScriptEngine::Send_Message (255, 255, 255, Get_Parameter ("Message"));
	}
};


REGISTER_SCRIPT_TT (JFW_Low_Power_Sound, "DownSound:string,UpSound:string")
class	JFW_Low_Power_Sound : public JFW_Low_Power_Base
{
protected:
	void	Power_Lost (GameObject * obj) override
	{
		ScriptEngine::Create_Sound (Get_Parameter ("DownSound"), ScriptEngine::Get_Position (obj), obj);
	}

	void	Power_Restored (GameObject * obj) override
	{
		ScriptEngine::Create_Sound (Get_Parameter ("UpSound"), ScriptEngine::Get_Position (obj), obj);
	}
};


/******************************************************************************
*
*     Announcements
*
*     Nine scripts, one announcement, three audiences and four things that
*     set it off.
*
******************************************************************************/

enum	MessageAudienceEnum
{
	AUDIENCE_EVERYBODY,
	AUDIENCE_TEAM,
	AUDIENCE_PLAYER
};


/*
**	Say it, and play the sound that goes with it, to whoever is listening.
*/
static void	Announce_To (ScriptImpClass * script, MessageAudienceEnum audience,
		GameObject * player, int team, const char * text_parameter)
{
	const char *	sound	= script->Get_Parameter ("Sound");
	const char *	text	= script->Get_Parameter (text_parameter);
	int				red	= script->Get_Int_Parameter ("Red");
	int				green	= script->Get_Int_Parameter ("Green");
	int				blue	= script->Get_Int_Parameter ("Blue");

	switch (audience) {

		case AUDIENCE_TEAM:
			ScriptEngine::Create_2D_Sound_Team (team, sound);
			ScriptEngine::Send_Message_Team (team, red, green, blue, text);
			break;

		case AUDIENCE_PLAYER:
			if (player != nullptr) {
				ScriptEngine::Create_2D_Sound_Player (player, sound);
				ScriptEngine::Send_Message_Player (player, red, green, blue, text);
			}
			break;

		default:
			ScriptEngine::Create_2D_Sound (sound);
			ScriptEngine::Send_Message (red, green, blue, text);
			break;
	}
}


/*
**	The five that fire when somebody walks into a zone.
*/
class	JFW_Message_Send_Zone_Base : public ScriptImpClass
{
protected:
	virtual MessageAudienceEnum	Audience (void) = 0;

	//	Whether the enterer has to be riding something.
	virtual bool	Riders_Only (void)			{ return false; }

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		int team = Get_Int_Parameter ("Player_Type");
		if (!Is_Player_Type (enterer, team)) {
			return ;
		}

		if (Riders_Only() && (ScriptEngine::Get_Vehicle (enterer) == nullptr)) {
			return ;
		}

		Announce_To (this, Audience(), enterer, team, "Message");

		if (Get_Bool_Parameter ("Delete")) {
			Destroy_Script();
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Message_Send_Zone, "Player_Type:int,Message:string,Red:int,Blue:int,Green:int,Sound:string,Delete:int")
class	JFW_Message_Send_Zone : public JFW_Message_Send_Zone_Base
{
protected:
	MessageAudienceEnum	Audience (void) override		{ return AUDIENCE_EVERYBODY; }
};


REGISTER_SCRIPT_TT (JFW_Message_Send_Zone_Team, "Player_Type:int,Message:string,Red:int,Blue:int,Green:int,Sound:string,Delete:int")
class	JFW_Message_Send_Zone_Team : public JFW_Message_Send_Zone_Base
{
protected:
	MessageAudienceEnum	Audience (void) override		{ return AUDIENCE_TEAM; }
};


REGISTER_SCRIPT_TT (JFW_Message_Send_Zone_Player, "Player_Type:int,Message:string,Red:int,Blue:int,Green:int,Sound:string,Delete:int")
class	JFW_Message_Send_Zone_Player : public JFW_Message_Send_Zone_Base
{
protected:
	MessageAudienceEnum	Audience (void) override		{ return AUDIENCE_PLAYER; }
};


REGISTER_SCRIPT_TT (JFW_Message_Send_Zone_Player_Vehicle, "Player_Type:int,Message:string,Red:int,Blue:int,Green:int,Sound:string,Delete:int")
class	JFW_Message_Send_Zone_Player_Vehicle : public JFW_Message_Send_Zone_Base
{
protected:
	MessageAudienceEnum	Audience (void) override		{ return AUDIENCE_PLAYER; }
	bool						Riders_Only (void) override	{ return true; }
};


DECLARE_SCRIPT_TT (JFW_Message_Send_Death, "Message:string,Red:int,Blue:int,Green:int,Sound:string")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Announce_To (this, AUDIENCE_EVERYBODY, nullptr, 0, "Message");
		Destroy_Script();
	}
};


DECLARE_SCRIPT_TT (JFW_Message_Send_Death_Team, "Player_Type:int,Message:string,Red:int,Blue:int,Green:int,Sound:string")
{
	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Announce_To (this, AUDIENCE_TEAM, nullptr, Get_Int_Parameter ("Player_Type"), "Message");
		Destroy_Script();
	}
};


DECLARE_SCRIPT_TT (JFW_Message_Send_Custom, "Display_Message:string,Red:int,Blue:int,Green:int,Sound:string,Message:int,Delete:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		Announce_To (this, AUDIENCE_EVERYBODY, nullptr, 0, "Display_Message");

		if (Get_Bool_Parameter ("Delete")) {
			Destroy_Script();
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Message_Send_Custom_Team, "Player_Type:int,Display_Message:string,Red:int,Blue:int,Green:int,Sound:string,Message:int,Delete:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		Announce_To (this, AUDIENCE_TEAM, nullptr, Get_Int_Parameter ("Player_Type"), "Display_Message");

		if (Get_Bool_Parameter ("Delete")) {
			Destroy_Script();
		}
	}
};


DECLARE_SCRIPT_TT (JFW_Message_Send_Powerup, "Display_Message:string,Red:int,Blue:int,Green:int,Sound:string")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			Announce_To (this, AUDIENCE_PLAYER, sender, 0, "Display_Message");
		}
	}
};


/******************************************************************************
*
*     Spies
*
******************************************************************************/

/*
**	Wearing the other side's colours.  The team is put back the moment the
**	disguise comes off, however it comes off.
*/
DECLARE_SCRIPT_TT (JFW_Spy_Switch_Team, "")
{
	void	Undisguise (GameObject * obj)
	{
		ScriptEngine::Change_Team (obj, ScriptEngine::Get_Player_Type (obj), false);
	}

	void	Created (GameObject * obj) override
	{
		ScriptEngine::Change_Team (obj, Enemy_Of (ScriptEngine::Get_Player_Type (obj)), false);
		ScriptEngine::Set_Is_Visible (obj, false);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Undisguise (obj);
	}

	void	Destroyed (GameObject * obj) override
	{
		Undisguise (obj);
	}

	void	Detach (GameObject * obj) override
	{
		ScriptImpClass::Detach (obj);
		Undisguise (obj);
	}
};


/*
**	A vehicle that goes invisible while everybody aboard it is a spy.
*/
class	JFW_Spy_Vehicle_Ignore_Base : public ScriptImpClass
{
	int	SpyCount;
	int	OtherCount;

protected:
	virtual bool	Is_Spy_Rider (GameObject * rider) = 0;

	void	Created (GameObject * obj) override
	{
		SpyCount		= 0;
		OtherCount	= 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {

			if (Is_Spy_Rider (sender)) {
				SpyCount ++;
			} else {
				OtherCount ++;
			}

		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {

			if (Is_Spy_Rider (sender)) {
				SpyCount --;
			} else {
				OtherCount --;
			}

		} else {
			return ;
		}

		ScriptEngine::Set_Is_Visible (obj, (SpyCount == 0) || (OtherCount != 0));
	}

public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (SpyCount, 1);
		SAVE_VARIABLE (OtherCount, 2);
	}
};


REGISTER_SCRIPT_TT (JFW_Spy_Vehicle_Ignore, "Spy_Script:string")
class	JFW_Spy_Vehicle_Ignore : public JFW_Spy_Vehicle_Ignore_Base
{
protected:
	bool	Is_Spy_Rider (GameObject * rider) override
	{
		return ScriptEngine::Is_Script_Attached (rider, Get_Parameter ("Spy_Script"));
	}
};


REGISTER_SCRIPT_TT (JFW_Spy_Vehicle_Ignore_New, "")
class	JFW_Spy_Vehicle_Ignore_New : public JFW_Spy_Vehicle_Ignore_Base
{
protected:
	bool	Is_Spy_Rider (GameObject * rider) override
	{
		return ScriptEngine::Is_Spy (rider);
	}
};


/******************************************************************************
*
*     Under the water
*
******************************************************************************/

/*
**	Anything that wants to know where the surface is asks this, and it answers
**	with its own height.
*/
DECLARE_SCRIPT_TT (JFW_Water_Level, "Message:int")
{
	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type != Get_Int_Parameter ("Message")) {
			return ;
		}

		float level = ScriptEngine::Get_Position (obj).Z;
		ScriptEngine::Send_Custom_Event (obj, sender, type, Float_To_Param (level), 0);
	}
};


DECLARE_SCRIPT_TT (JFW_Submarine, "Message:int,Submerge_Armor:string,Surface_Armor:string,Block_Weapon:int,Ping_Sound:string,Surface_Sound:string,Ping_Time:float,Surface_Z_Offset:float,Dive_Z_Offset:float,Powerup:string,Weapon:string")
{
	float	WaterLevel;
	bool	Submerged;
	bool	Occupied;
	int	DriverID;

	void	Allow_Firing (GameObject * obj, bool allowed)
	{
		if (!Get_Bool_Parameter ("Block_Weapon")) {
			return ;
		}

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if (vehicle != nullptr) {
			vehicle->Set_Scripts_Can_Fire (allowed);
		}
	}

	void	Created (GameObject * obj) override
	{
		WaterLevel	= 0.0f;
		Submerged	= false;
		DriverID		= 0;
		Occupied		= false;

		GameObject * water = ScriptEngine::Find_Object_With_Script ("JFW_Water_Level");
		if (water != nullptr) {
			ScriptEngine::Send_Custom_Event (obj, water, Get_Int_Parameter ("Message"), 0, 0);
		}
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message")) {

			WaterLevel = Param_To_Float (param);
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Ping_Time"), 1);
			ScriptEngine::Start_Timer (obj, this, 1.0f, 2);

		} else if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {

			Occupied = true;
			if (DriverID == 0) {
				DriverID = ScriptEngine::Get_ID (sender);
			}

		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {

			Occupied = false;

			//	Bailing out underwater floods the boat.
			if (Submerged) {
				ScriptEngine::Apply_Damage (obj, 100, "Death", nullptr);
			}

			if (DriverID == ScriptEngine::Get_ID (sender)) {
				DriverID = 0;
			}
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == 1) {

			//	The sonar ping a submerged boat gives away.
			if (Submerged) {
				Vector3 position = ScriptEngine::Get_Position (obj);
				if (position.Z < (WaterLevel + Get_Float_Parameter ("Surface_Z_Offset"))) {
					ScriptEngine::Create_Sound (Get_Parameter ("Ping_Sound"), position, obj);
				}
			}

			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Ping_Time"), 1);
			return ;
		}

		if (number != 2) {
			return ;
		}

		float height = ScriptEngine::Get_Position (obj).Z;

		if (Submerged) {

			//	Broken the surface.
			if (height >= (WaterLevel + Get_Float_Parameter ("Surface_Z_Offset"))) {

				ScriptEngine::Create_Sound (Get_Parameter ("Surface_Sound"),
						ScriptEngine::Get_Position (obj), obj);
				ScriptEngine::Set_Shield_Type (obj, Get_Parameter ("Surface_Armor"));
				ScriptEngine::Enable_Stealth (obj, false);
				Allow_Firing (obj, true);

				Send_To_ID (obj, DriverID, 987987, 0);
				Submerged = false;
			}

		} else if (height < (WaterLevel + Get_Float_Parameter ("Dive_Z_Offset"))) {

			//	Gone under.
			ScriptEngine::Set_Shield_Type (obj, Get_Parameter ("Submerge_Armor"));
			ScriptEngine::Enable_Stealth (obj, true);
			Allow_Firing (obj, false);

			if (!Occupied) {
				ScriptEngine::Apply_Damage (obj, 100, "Death", nullptr);
			}

			Send_To_ID (obj, DriverID, 789789, 0);
			Submerged = true;
		}

		ScriptEngine::Start_Timer (obj, this, 1.0f, 2);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		//	Dying underwater drowns the crew rather than ejecting them.
		if (!Submerged || (DriverID == 0)) {
			return ;
		}

		GameObject * driver = ScriptEngine::Find_Object (DriverID);
		if (driver != nullptr) {
			ScriptEngine::Attach_Script (driver, "RA_DriverDeath", "0");
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (WaterLevel, 1);
		SAVE_VARIABLE (Submerged, 2);
		SAVE_VARIABLE (Occupied, 3);
		SAVE_VARIABLE (DriverID, 4);
	}
};


/******************************************************************************
*
*     Weapons that do something other than damage
*
******************************************************************************/

/*
**	Whoever is responsible for a shot: the vehicle if the shooter is riding
**	one, otherwise the shooter.
*/
static GameObject *	Shooter_Of (GameObject * obj)
{
	if (obj == nullptr) {
		return nullptr;
	}

	GameObject * ride = ScriptEngine::Get_Vehicle (obj);
	return (ride != nullptr) ? ride : obj;
}


/*
**	A web that pins a soldier where they stand until it times out.
*/
DECLARE_SCRIPT_TT (JFW_Reaper_Web, "Warhead:string,Time:float,WebModel:string,HumanAnimation:string")
{
	int	WebID;

	void	Created (GameObject * obj) override
	{
		WebID = 0;
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		if (Shooter_Of (damager) == nullptr) {
			return ;
		}

		WarheadType warhead = ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Warhead"));
		if (warhead != ScriptEngine::Get_Damage_Warhead()) {
			return ;
		}

		SoldierGameObj * victim = obj->As_SoldierGameObj();
		if (victim == nullptr) {
			return ;
		}

		Destroy_By_ID (WebID);
		WebID = 0;

		ScriptEngine::Set_Animation (obj, Get_Parameter ("HumanAnimation"), false, nullptr, 0.0f, -1.0f, false);
		victim->Set_Freeze (true);

		GameObject * web = ScriptEngine::Create_Object ("Invisible_Object", ScriptEngine::Get_Position (obj));
		if (web != nullptr) {
			const char * model = Get_Parameter ("WebModel");
			ScriptEngine::Set_Model (web, model);
			ScriptEngine::Set_Facing (web, ScriptEngine::Get_Facing (obj));

			StringClass animation;
			animation.Format ("%s.%s", model, model);
			ScriptEngine::Set_Animation (web, animation, false, nullptr, 0.0f, -1.0f, false);

			WebID = ScriptEngine::Get_ID (web);
		}

		//	The donor read a "time" parameter it does not register.
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Destroy_By_ID (WebID);
		WebID = 0;
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (WebID == 0) {
			return ;
		}

		SoldierGameObj * victim = obj->As_SoldierGameObj();
		if (victim != nullptr) {
			victim->Set_Freeze (false);
		}

		Destroy_By_ID (WebID);
		WebID = 0;
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (WebID, 1);
	}
};


/*
**	Poison that keeps working until the victim buys a fresh loadout.
*/
DECLARE_SCRIPT_TT (JFW_Toxic_Grenade, "Weapon:string,Time:float,Warhead:string,Damage:float")
{
	bool	Poisoned;

	void	Created (GameObject * obj) override
	{
		Poisoned = false;
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		//	Being healed washes it off too.
		if (amount < 0.0f) {
			Poisoned = false;
			return ;
		}

		GameObject * shooter = Shooter_Of (damager);
		if ((shooter == nullptr) || !Weapon_Is (shooter, Get_Parameter ("Weapon"))) {
			return ;
		}

		Poisoned = true;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_REFILL) {
			Poisoned = false;
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (!Poisoned) {
			return ;
		}

		ScriptEngine::Apply_Damage (obj, Get_Float_Parameter ("Damage"), Get_Parameter ("Warhead"), nullptr);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Poisoned, 1);
	}
};


/******************************************************************************
*
*     Hijacking
*
******************************************************************************/

DECLARE_SCRIPT_TT (JFW_Hijacker_Vehicle, "Weapon:string,Warhead:string,Damage:float,Sound:string")
{
	int	HijackerID;

	void	Created (GameObject * obj) override
	{
		HijackerID = 0;
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		if ((damager == nullptr) || (HijackerID != 0)) {
			return ;
		}

		//	On foot, holding the hijacker's weapon, and not a teammate.
		if (!Weapon_Is (damager, Get_Parameter ("Weapon"))
				|| (ScriptEngine::Get_Vehicle (damager) != nullptr)) {
			return ;
		}

		DamageableGameObj * thief	= damager->As_DamageableGameObj();
		DamageableGameObj * ride	= obj->As_DamageableGameObj();
		if ((thief == nullptr) || (ride == nullptr) || thief->Is_Teammate (ride)) {
			return ;
		}

		VehicleGameObj * vehicle = As_Vehicle (obj);
		if ((ScriptEngine::Get_Vehicle_Occupant_Count (obj) != 1)
				|| (vehicle == nullptr) || vehicle->Is_Immovable()) {
			return ;
		}

		ScriptEngine::Create_Sound (Get_Parameter ("Sound"), ScriptEngine::Get_Position (damager), damager);
		ScriptEngine::Force_Occupants_Exit (obj);
		ScriptEngine::Start_Timer (obj, this, 0.5f, 1);
		HijackerID = ScriptEngine::Get_ID (damager);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (ScriptEngine::Get_Vehicle_Occupant_Count (obj) != 0) {
			return ;
		}

		GameObject * hijacker = ScriptEngine::Find_Object (HijackerID);
		if (hijacker != nullptr) {
			ScriptEngine::Soldier_Transition_Vehicle (hijacker);
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		GameObject * hijacker = (HijackerID != 0) ? ScriptEngine::Find_Object (HijackerID) : nullptr;
		if (hijacker != nullptr) {
			ScriptEngine::Apply_Damage (hijacker, Get_Float_Parameter ("Damage"),
					Get_Parameter ("Warhead"), nullptr);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (HijackerID, 1);
	}
};


DECLARE_SCRIPT_TT (JFW_Hijacker_Vehicle_2, "Warhead:string,Sound:string,Disable_Transitions:int,Single_Hijack:int,Distance:float,Weapon:string")
{
	int	HijackerID;
	bool	Jacking;

	void	Created (GameObject * obj) override
	{
		HijackerID	= 0;
		Jacking		= false;
	}

	void	Damaged (GameObject * obj, GameObject * damager, float amount) override
	{
		if ((damager == nullptr) || (HijackerID != 0) || Jacking) {
			return ;
		}

		WarheadType warhead = ArmorWarheadManager::Get_Warhead_Type (Get_Parameter ("Warhead"));
		if ((warhead != ScriptEngine::Get_Damage_Warhead())
				|| (ScriptEngine::Get_Vehicle (damager) != nullptr)) {
			return ;
		}

		DamageableGameObj * thief	= damager->As_DamageableGameObj();
		DamageableGameObj * ride	= obj->As_DamageableGameObj();
		if ((thief == nullptr) || (ride == nullptr) || thief->Is_Teammate (ride)) {
			return ;
		}

		if (ScriptEngine::Get_Vehicle_Occupant_Count (obj) != 1) {
			return ;
		}

		//	Somebody has to be at the wheel, and awake.
		GameObject *	driver		= ScriptEngine::Get_Vehicle_Driver (obj);
		SmartGameObj *	driver_smart	= (driver != nullptr) ? driver->As_SmartGameObj() : nullptr;
		if ((driver_smart == nullptr) || !driver_smart->Is_Control_Enabled()) {
			return ;
		}

		float reach = ScriptEngine::Get_Distance (ScriptEngine::Get_Position (obj),
				ScriptEngine::Get_Position (damager));
		if (reach > Get_Float_Parameter ("Distance")) {
			return ;
		}

		SoldierGameObj * hijacker = damager->As_SoldierGameObj();
		if (hijacker == nullptr) {
			return ;
		}

		ScriptEngine::Create_Sound (Get_Parameter ("Sound"), ScriptEngine::Get_Position (damager), damager);
		ScriptEngine::Force_Occupants_Exit (obj);
		ScriptEngine::Start_Timer (obj, this, 0.5f, 1);

		HijackerID = ScriptEngine::Get_ID (damager);
		hijacker->Set_Can_Drive_Vehicles (true);
		Jacking = true;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if ((HijackerID == 0) || (sender != ScriptEngine::Find_Object (HijackerID))) {
			return ;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {

			if (Get_Bool_Parameter ("Disable_Transitions")) {
				ScriptEngine::Enable_Vehicle_Transitions (obj, false);
			}

			//	A one-shot hijacker's weapon goes with the vehicle.
			if (Get_Bool_Parameter ("Single_Hijack")) {
				ScriptEngine::Remove_Weapon (sender, Get_Parameter ("Weapon"));
			}

		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {

			if (!Get_Bool_Parameter ("Single_Hijack")) {
				SoldierGameObj * hijacker = sender->As_SoldierGameObj();
				if (hijacker != nullptr) {
					hijacker->Set_Can_Drive_Vehicles (false);
				}
			}

			HijackerID = 0;
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (ScriptEngine::Get_Vehicle_Occupant_Count (obj) != 0) {
			return ;
		}

		Jacking = false;

		GameObject * hijacker = ScriptEngine::Find_Object (HijackerID);
		if (hijacker != nullptr) {
			ScriptEngine::Force_Vehicle_Entry (hijacker, obj);
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		if (Get_Bool_Parameter ("Single_Hijack")) {
			return ;
		}

		GameObject *		found		= (HijackerID != 0) ? ScriptEngine::Find_Object (HijackerID) : nullptr;
		SoldierGameObj *	hijacker	= (found != nullptr) ? found->As_SoldierGameObj() : nullptr;
		if (hijacker != nullptr) {
			hijacker->Set_Can_Drive_Vehicles (false);
		}
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (HijackerID, 1);
		SAVE_VARIABLE (Jacking, 2);
	}
};


/******************************************************************************
*
*     Taking a factory off the menu
*
******************************************************************************/

/*
**	While the building is disabled, everything it builds is greyed out on the
**	purchase terminal.  Several things can disable it at once, so the script
**	counts.
*/
DECLARE_SCRIPT_TT (JFW_Building_Preset_Disable, "Disable_Custom:int,Enable_Custom:int")
{
	int	Holds;

	void	Set_Menu (GameObject * obj, bool disable)
	{
		BuildingGameObj * building = obj->As_BuildingGameObj();
		if (building == nullptr) {
			return ;
		}

		ScriptEngine::Disable_All_Presets_By_Factory_Tech (building->Get_Definition().Get_Type(),
				building->Get_Player_Type(), disable);
	}

	void	Created (GameObject * obj) override
	{
		Holds = 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Disable_Custom")) {

			if (Holds == 0) {
				Set_Menu (obj, true);
			}
			Holds ++;

		} else if (type == Get_Int_Parameter ("Enable_Custom")) {

			//	The donor let this go negative, after which the count could
			//	never reach zero again and the factory stayed disabled.
			if (Holds == 0) {
				return ;
			}

			Holds --;
			if (Holds == 0) {
				Set_Menu (obj, false);
			}
		}
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		Destroy_Script();
	}

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Holds, 1);
	}
};
