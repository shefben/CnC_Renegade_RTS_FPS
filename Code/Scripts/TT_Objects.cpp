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
*     TT_Objects.cpp
*
* DESCRIPTION
*     What an object can be told to do about itself: carry a mission
*     objective, take an order, wear a veterancy rank, deploy, jam a radar,
*     or hand a player something.
*
*     Native port of the 4.8.4 library's jfwobj.cpp.  Every script keeps its
*     registered name and its registered parameters, so existing levels find
*     them unchanged.
*
*     The file's first eleven scripts are one script written out eleven
*     times: the same six-way trigger (created / entered / killed / custom /
*     poked / timer) in front of eleven different one-line actions.  They are
*     one base and a virtual here.
*
*     Defects in the donor, fixed:
*
*     - The trigger family's timer pathway dereferenced a custom event's
*       integer parameter as a pointer to a stack structure, and the only
*       script that sends that event sent a zero.  Every timed objective
*       therefore read address zero the moment it fired.  The request now
*       travels in a file-static handed across the synchronous send, which
*       is what the donor was reaching for.
*
*     - JFW_Start_Timer never started a timer.  It filled a TimerParams on
*       the stack, dropped it, and relayed a bare custom, so its registered
*       Timer_Number, Timer_Time and Repeat did nothing at all.
*
*     - The trigger family left its timer time, number and repeat flag
*       uninitialised until a request arrived, so a repeating timer that
*       fired before one did restarted itself from whatever was on the stack.
*
*     - JFW_Star_Trigger_Zone asked whether the zone itself was the player
*       instead of whether the player had walked into it, so it never fired.
*
*     - JFW_Innate_AI_Home_Location read its vector3 as a float and passed it
*       to the cover-probability setter -- one line copied from the script
*       above it -- so it never set a home location.
*
*     - JFW_Deployable_Object tested a parameter called "Warhead" that it
*       does not register.  The warhead it means is its "Key" parameter, so
*       nothing was ever deployed.
*
*     - JFW_Medic_Beacon's "already at full strength" test compared a
*       teammate's shield with itself rather than its maximum, so a hurt
*       shield on an unhurt teammate went unhealed.
*
*     - JFW_Mech decided whether to initialise itself by reading a member it
*       had not written yet.
*
*     - JFW_Mech and JFW_Deployable_Mech chose their walk animation by
*       comparing the raw X and Y of the movement against each other, which
*       reads a sidestep as walking backwards.  Both now ask whether the
*       movement went the way the mech is pointing.
*
*     - JFW_Veteran_Custom registers six custom message numbers as strings
*       and reads them as ints.  They are ints.
*
*     - JFW_Set_Objective_Radar_Blip_Custom registers Position as an int and
*       reads it as a vector3.  It is a vector3.
*
*     - JFW_Object_Spawn_At_Bone registers "Bone:String" -- the capital S is
*       a type the parameter parser does not know.  It is a string.
*
*     Reborn_Deployable_Vehicle_Player comes with them.  It is the pilot's
*     half of a deployable vehicle -- one key press forwarded to the vehicle
*     -- and the deployable vehicle and mech here attach it by name.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"
#include "actionparams.h"
#include "armedgameobj.h"
#include "damage.h"
#include "gameobjmanager.h"
#include "movephys.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "simplegameobj.h"
#include "smartgameobj.h"
#include "soldier.h"
#include "vehicle.h"
#include "wwmath.h"
#include "wwstring.h"

#include <math.h>
#include <string.h>


/*
**	Clearing an animation.  There is no separate command for it: an empty
**	animation name is how the engine is told to stop.
*/
static void	Clear_Animation (GameObject * obj)
{
	if (obj != nullptr)
	{
		ScriptEngine::Set_Animation (obj, "", false);
	}
}


/******************************************************************************
*
*     The six-way trigger the objective scripts share
*
*     "Type" chooses which event fires the action.  "TypeVal" means whatever
*     that event needs it to mean: the player type allowed through a zone,
*     the custom message to answer, or the timer number to wait for.
*
******************************************************************************/

enum JFW_Trigger_When
{
	JFW_TRIGGER_ON_CREATE	= 0,
	JFW_TRIGGER_ON_ENTER	= 1,
	JFW_TRIGGER_ON_DEATH	= 2,
	JFW_TRIGGER_ON_CUSTOM	= 3,
	JFW_TRIGGER_ON_POKE		= 4,
	JFW_TRIGGER_ON_TIMER	= 5
};


/*
**	What JFW_Start_Timer asks the object it points at to do.  Send_Custom_Event
**	delivers on the caller's stack when the delay is zero, so the request is
**	valid for exactly the length of that call and nowhere else.
*/
struct JFW_Timer_Request
{
	int		Number;
	float	Time;
	bool	Repeat;
};

static const JFW_Timer_Request *	JFW_Pending_Timer	= nullptr;


class	JFW_Trigger_Base : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Repeat, 1);
		SAVE_VARIABLE (Timer_Time, 2);
		SAVE_VARIABLE (Timer_Number, 3);
	}

protected:
	/*
	**	The one line that tells the eleven of them apart.
	*/
	virtual void	Act (GameObject * obj) = 0;

	/*
	**	JFW_Start_Timer sends these; it does not receive them.
	*/
	virtual bool	Accepts_Timer_Request (void) const	{ return true; }

private:
	float	Timer_Time;
	int		Timer_Number;
	bool	Repeat;

	int	When (void)			{ return Get_Int_Parameter ("Type"); }
	int	When_Value (void)	{ return Get_Int_Parameter ("TypeVal"); }

	void	Created (GameObject * obj) override
	{
		Timer_Time		= 0.0f;
		Timer_Number	= 0;
		Repeat			= false;

		if (When () == JFW_TRIGGER_ON_CREATE) { Act (obj); }
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (When () != JFW_TRIGGER_ON_ENTER)			{ return; }
		if (!Is_Player_Type (enterer, When_Value ()))	{ return; }

		Act (obj);
	}

	void	Killed (GameObject * obj, GameObject * killer) override
	{
		if (When () == JFW_TRIGGER_ON_DEATH) { Act (obj); }
	}

	void	Poked (GameObject * obj, GameObject * poker) override
	{
		if (When () == JFW_TRIGGER_ON_POKE) { Act (obj); }
	}

	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (Accepts_Timer_Request () && JFW_Pending_Timer != nullptr &&
			type == Get_Int_Parameter ("Timer_Custom"))
		{
			Repeat			= JFW_Pending_Timer->Repeat;
			Timer_Time		= JFW_Pending_Timer->Time;
			Timer_Number	= JFW_Pending_Timer->Number;
			ScriptEngine::Start_Timer (obj, this, Timer_Time, Timer_Number);
		}

		if (When () != JFW_TRIGGER_ON_CUSTOM)	{ return; }
		if (When_Value () != type)				{ return; }

		Act (obj);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (When () != JFW_TRIGGER_ON_TIMER)	{ return; }
		if (When_Value () != number)			{ return; }

		if (Repeat)
		{
			ScriptEngine::Start_Timer (obj, this, Timer_Time, Timer_Number);
		}

		Act (obj);
	}
};


/*
**	Trigger parameters, shared by the eleven.  The specific ones sit between
**	these two, exactly where the donor put them.
*/
#define JFW_TRIGGER_P		"Type:int,TypeVal:int,"
#define JFW_TRIGGER_TIMER_P	"Timer_Custom:int"


REGISTER_SCRIPT_TT (JFW_Add_Objective,
	JFW_TRIGGER_P "Objective_Num:int,Objective_Type:int,Title_ID:int,Unknown:int,"
	"Sound_Name:string,Description_ID:int," JFW_TRIGGER_TIMER_P ",Trigger:int")

class	JFW_Add_Objective : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		/*
		**	"Unknown" has always been the objective's starting status; the
		**	name is kept so levels that set it keep setting it.
		*/
		ScriptEngine::Add_Objective (Get_Int_Parameter ("Objective_Num"),
			Get_Int_Parameter ("Objective_Type"), Get_Int_Parameter ("Unknown"),
			Get_Int_Parameter ("Title_ID"), Get_Parameter ("Sound_Name"),
			Get_Int_Parameter ("Description_ID"));

		int trigger = Get_Int_Parameter ("Trigger");

		if (trigger != 0)
		{
			ScriptEngine::Send_Custom_Event (obj, obj, trigger, 0, 0.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Remove_Objective,
	JFW_TRIGGER_P "Objective_Num:int," JFW_TRIGGER_TIMER_P)

class	JFW_Remove_Objective : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Remove_Objective (Get_Int_Parameter ("Objective_Num"));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_Status,
	JFW_TRIGGER_P "Objective_Num:int,Status:int," JFW_TRIGGER_TIMER_P)

class	JFW_Set_Objective_Status : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Set_Objective_Status (Get_Int_Parameter ("Objective_Num"),
			Get_Int_Parameter ("Status"));
	}
};


REGISTER_SCRIPT_TT (JFW_Change_Objective_Type,
	JFW_TRIGGER_P "Objective_Num:int,Objective_Type:int," JFW_TRIGGER_TIMER_P)

class	JFW_Change_Objective_Type : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Change_Objective_Type (Get_Int_Parameter ("Objective_Num"),
			Get_Int_Parameter ("Objective_Type"));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_Radar_Blip,
	JFW_TRIGGER_P "Objective_Num:int,Pos:vector3," JFW_TRIGGER_TIMER_P)

class	JFW_Set_Objective_Radar_Blip : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Set_Objective_Radar_Blip (Get_Int_Parameter ("Objective_Num"),
			Get_Vector3_Parameter ("Pos"));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_Radar_Blip_Object,
	JFW_TRIGGER_P "Objective_Num:int,Object:int," JFW_TRIGGER_TIMER_P)

class	JFW_Set_Objective_Radar_Blip_Object : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Set_Objective_Radar_Blip_Object (Get_Int_Parameter ("Objective_Num"),
			ScriptEngine::Find_Object (Get_Int_Parameter ("Object")));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_HUD_Info,
	JFW_TRIGGER_P "Objective_Num:int,Priority:float,Pog_File:string,Pog_Title_ID:int,"
	JFW_TRIGGER_TIMER_P)

class	JFW_Set_Objective_HUD_Info : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Set_Objective_HUD_Info (Get_Int_Parameter ("Objective_Num"),
			Get_Float_Parameter ("Priority"), Get_Parameter ("Pog_File"),
			Get_Int_Parameter ("Pog_Title_ID"));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_HUD_Info_Position,
	JFW_TRIGGER_P "Objective_Num:int,Priority:float,Pog_File:string,Pog_Title_ID:int,"
	"Position:vector3," JFW_TRIGGER_TIMER_P)

class	JFW_Set_Objective_HUD_Info_Position : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Set_Objective_HUD_Info_Position (Get_Int_Parameter ("Objective_Num"),
			Get_Float_Parameter ("Priority"), Get_Parameter ("Pog_File"),
			Get_Int_Parameter ("Pog_Title_ID"), Get_Vector3_Parameter ("Position"));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Num_Tertary_Objectives,
	JFW_TRIGGER_P "Count:int," JFW_TRIGGER_TIMER_P)

class	JFW_Set_Num_Tertary_Objectives : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Set_Num_Tertiary_Objectives (Get_Int_Parameter ("Count"));
	}
};


REGISTER_SCRIPT_TT (JFW_Mission_Complete,
	JFW_TRIGGER_P "Won:int," JFW_TRIGGER_TIMER_P)

class	JFW_Mission_Complete : public JFW_Trigger_Base
{
	void	Act (GameObject * obj) override
	{
		ScriptEngine::Mission_Complete (Get_Int_Parameter ("Won") != 0);
	}
};


REGISTER_SCRIPT_TT (JFW_Start_Timer,
	JFW_TRIGGER_P "Timer_Object:int,Timer_Number:int,Timer_Time:float,Repeat:int,"
	JFW_TRIGGER_TIMER_P)

class	JFW_Start_Timer : public JFW_Trigger_Base
{
	bool	Accepts_Timer_Request (void) const override	{ return false; }

	void	Act (GameObject * obj) override
	{
		GameObject * target = ScriptEngine::Find_Object (Get_Int_Parameter ("Timer_Object"));

		if (target == nullptr) { return; }

		JFW_Timer_Request request;
		request.Number	= Get_Int_Parameter ("Timer_Number");
		request.Time	= Get_Float_Parameter ("Timer_Time");
		request.Repeat	= Get_Bool_Parameter ("Repeat");

		JFW_Pending_Timer = &request;
		ScriptEngine::Send_Custom_Event (obj, target, Get_Int_Parameter ("Timer_Custom"),
			request.Number, 0.0f);
		JFW_Pending_Timer = nullptr;
	}
};

/******************************************************************************
*
*     The same eleven actions again, this time on a plain custom message
*
*     Where the family above answers any of six events, these answer exactly
*     one, and say so in their parameters.  Levels use both.
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Mission_Complete_Custom, "Message:int,Success:int")

class	JFW_Mission_Complete_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Mission_Complete (Get_Bool_Parameter ("Success"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_HUD_Help_Text, "Message:int,StringID:int,Red:int,Green:int,Blue:int")

class	JFW_HUD_Help_Text : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			Vector3 color ((float)Get_Int_Parameter ("Red")   / 255.0f,
						   (float)Get_Int_Parameter ("Green") / 255.0f,
						   (float)Get_Int_Parameter ("Blue")  / 255.0f);

			ScriptEngine::Set_HUD_Help_Text (Get_Int_Parameter ("StringID"), color);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Add_Objective_Startup,
	"ObjectiveID:int,ObjectiveType:int,ShortDescription:int,Sound:string,LongDescription:int")

class	JFW_Add_Objective_Startup : public ScriptImpClass
{
	void	Created (GameObject * /*obj*/) override
	{
		ScriptEngine::Add_Objective (Get_Int_Parameter ("ObjectiveID"),
			Get_Int_Parameter ("ObjectiveType"), OBJECTIVE_STATUS_HIDDEN,
			Get_Int_Parameter ("ShortDescription"), Get_Parameter ("Sound"),
			Get_Int_Parameter ("LongDescription"));
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_Status_Custom, "Message:int,ObjectiveID:int,Status:int")

class	JFW_Set_Objective_Status_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Objective_Status (Get_Int_Parameter ("ObjectiveID"),
				Get_Int_Parameter ("Status"));
		}
	}
};


/*
**	The donor registers Position as an int and then reads a vector3 out of it.
**	It is a position; it is registered as one here.
*/
REGISTER_SCRIPT_TT (JFW_Set_Objective_Radar_Blip_Custom,
	"Message:int,ObjectiveID:int,Position:vector3")

class	JFW_Set_Objective_Radar_Blip_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Objective_Radar_Blip (Get_Int_Parameter ("ObjectiveID"),
				Get_Vector3_Parameter ("Position"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_Radar_Blip_Object_Custom,
	"Message:int,ObjectiveID:int,ID:int")

class	JFW_Set_Objective_Radar_Blip_Object_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Objective_Radar_Blip_Object (Get_Int_Parameter ("ObjectiveID"),
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_HUD_Info_Custom,
	"Message:int,ObjectiveID:int,Priority:float,Texture:string,StringID:int")

class	JFW_Set_Objective_HUD_Info_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Objective_HUD_Info (Get_Int_Parameter ("ObjectiveID"),
				Get_Float_Parameter ("Priority"), Get_Parameter ("Texture"),
				Get_Int_Parameter ("StringID"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Set_Objective_HUD_Info_Position_Custom,
	"Message:int,ObjectiveID:int,Priority:float,Texture:string,StringID:int,Position:vector3")

class	JFW_Set_Objective_HUD_Info_Position_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Objective_HUD_Info_Position (Get_Int_Parameter ("ObjectiveID"),
				Get_Float_Parameter ("Priority"), Get_Parameter ("Texture"),
				Get_Int_Parameter ("StringID"), Get_Vector3_Parameter ("Position"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Add_Radar_Marker,
	"Message:int,MarkerID:int,Position:vector3,BlipShape:int,BlipColor:int")

class	JFW_Add_Radar_Marker : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Add_Radar_Marker (Get_Int_Parameter ("MarkerID"),
				Get_Vector3_Parameter ("Position"), Get_Int_Parameter ("BlipShape"),
				Get_Int_Parameter ("BlipColor"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Clear_Radar_Marker, "Message:int,MarkerID:int")

class	JFW_Clear_Radar_Marker : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Clear_Radar_Marker (Get_Int_Parameter ("MarkerID"));
		}
	}
};


/******************************************************************************
*
*     Tutorial conversations
*
*     Two scripts that differ in one line: whether the player is given a
*     speaking part in the conversation or only listens to it.
*
******************************************************************************/

#define JFW_TUTORIAL_P	"Conversation:string,ActionID:int,StartMessage:int,StopMessage:int," \
						"CompleteMessage:int,CompleteParam:int,DisableControls:int,LookID:int"

class	JFW_Tutorial_Base : public ScriptImpClass
{
protected:
	/*
	**	Who takes part, and how freely.  One form lets the speaker turn its
	**	head and face the player and gives the player a silent part; the
	**	other has the speaker hold still and talk to nobody in particular.
	*/
	virtual void	Join (GameObject * obj, int conversation) = 0;

private:
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("StartMessage"))
		{
			if (Get_Int_Parameter ("DisableControls") == 1)
			{
				ScriptEngine::Control_Enable (STAR, false);
			}

			ScriptEngine::Stop_All_Conversations();

			int conversation = ScriptEngine::Create_Conversation (Get_Parameter ("Conversation"),
					100, 300.0f, false);

			Join (obj, conversation);

			ScriptEngine::Start_Conversation (conversation, Get_Int_Parameter ("ActionID"));
			ScriptEngine::Monitor_Conversation (obj, conversation);

			int look_id = Get_Int_Parameter ("LookID");

			if (look_id != 0)
			{
				ScriptEngine::Force_Camera_Look (ScriptEngine::Get_Position (
						ScriptEngine::Find_Object (look_id)));
			}
		}
		else if (type == Get_Int_Parameter ("StopMessage"))
		{
			ScriptEngine::Stop_All_Conversations();
		}
	}

	void	Action_Complete (GameObject * obj, int action_id,
			ActionCompleteReason complete_reason) override
	{
		if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED ||
			complete_reason == ACTION_COMPLETE_CONVERSATION_INTERRUPTED ||
			complete_reason == ACTION_COMPLETE_CONVERSATION_UNABLE_TO_INIT)
		{
			if (Get_Int_Parameter ("DisableControls") == 1)
			{
				ScriptEngine::Control_Enable (STAR, true);
			}
		}

		if (action_id == Get_Int_Parameter ("ActionID") &&
			complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("CompleteMessage"),
				Get_Int_Parameter ("CompleteParam"), 0.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Tutorial_Conversation, JFW_TUTORIAL_P)

class	JFW_Tutorial_Conversation : public JFW_Tutorial_Base
{
	void	Join (GameObject * obj, int conversation) override
	{
		ScriptEngine::Join_Conversation (obj, conversation, false, true, true);
		ScriptEngine::Join_Conversation (STAR, conversation, false, false, false);
	}
};


REGISTER_SCRIPT_TT (JFW_Tutorial_Conversation_2, JFW_TUTORIAL_P)

class	JFW_Tutorial_Conversation_2 : public JFW_Tutorial_Base
{
	void	Join (GameObject * obj, int conversation) override
	{
		ScriptEngine::Join_Conversation (obj, conversation, false, false, false);
	}
};


/******************************************************************************
*
*     Orders
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Reveal_Map, "")

class	JFW_Reveal_Map : public ScriptImpClass
{
	void	Created (GameObject * /*obj*/) override
	{
		ScriptEngine::Reveal_Map();
	}
};


/*
**	Go somewhere, then say so.  The two of them differ only in whether the
**	somewhere is an object or a point.
*/
#define JFW_GOTO_P	"Message:int,ActionID:int,"
#define JFW_GOTO_REST_P	"Speed:float,ArriveDistance:float,WaypathID:int," \
						"CompleteMessage:int,CompleteParam:int"

class	JFW_Goto_Base : public ScriptImpClass
{
protected:
	virtual void	Aim (ActionParamsStruct & params) = 0;

private:
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		ActionParamsStruct params;
		params.Set_Basic (this, 100, Get_Int_Parameter ("ActionID"));
		Aim (params);
		params.WaypathID = Get_Int_Parameter ("WaypathID");
		ScriptEngine::Action_Goto (obj, params);
	}

	void	Action_Complete (GameObject * obj, int action_id,
			ActionCompleteReason /*complete_reason*/) override
	{
		if (action_id != Get_Int_Parameter ("ActionID"))	{ return; }
		if (Get_Int_Parameter ("CompleteMessage") == 0)		{ return; }

		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("CompleteMessage"),
			Get_Int_Parameter ("CompleteParam"), 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Goto_Object, JFW_GOTO_P "ID:int," JFW_GOTO_REST_P)

class	JFW_Goto_Object : public JFW_Goto_Base
{
	void	Aim (ActionParamsStruct & params) override
	{
		params.Set_Movement (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
			Get_Float_Parameter ("Speed"), Get_Float_Parameter ("ArriveDistance"));
	}
};


REGISTER_SCRIPT_TT (JFW_Goto_Location, JFW_GOTO_P "Location:vector3," JFW_GOTO_REST_P)

class	JFW_Goto_Location : public JFW_Goto_Base
{
	void	Aim (ActionParamsStruct & params) override
	{
		params.Set_Movement (Get_Vector3_Parameter ("Location"),
			Get_Float_Parameter ("Speed"), Get_Float_Parameter ("ArriveDistance"));
	}
};


/*
**	Attack something, then say so.  A speed of zero means the attacker is to
**	stand still while it does, which for a vehicle means bolting it down.
*/
#define JFW_ATTACK_REST_P	"Speed:float,ArriveDistance:float,WaypathID:int," \
							"CompleteMessage:int,CompleteParam:int,Range:float," \
							"Error:float,Primary:int"

class	JFW_Attack_Base : public ScriptImpClass
{
protected:
	virtual void	Aim (ActionParamsStruct & params) = 0;

private:
	void	Hold_Still (GameObject * obj, bool still)
	{
		if (Get_Float_Parameter ("Speed") != 0.0f) { return; }

		VehicleGameObj * vehicle = obj->As_VehicleGameObj();

		if (vehicle != nullptr) { vehicle->Set_Immovable (still); }
	}

	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		Hold_Still (obj, true);

		ActionParamsStruct params;
		params.Set_Basic (this, 100, Get_Int_Parameter ("ActionID"));
		Aim (params);
		params.WaypathID			= Get_Int_Parameter ("WaypathID");
		params.AttackCheckBlocked	= false;
		params.AttackErrorOverride	= true;
		ScriptEngine::Action_Attack (obj, params);
	}

	void	Action_Complete (GameObject * obj, int action_id,
			ActionCompleteReason /*complete_reason*/) override
	{
		if (action_id != Get_Int_Parameter ("ActionID")) { return; }

		Hold_Still (obj, false);

		if (Get_Int_Parameter ("CompleteMessage") == 0) { return; }

		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("CompleteMessage"),
			Get_Int_Parameter ("CompleteParam"), 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Attack_Object, JFW_GOTO_P "ID:int," JFW_ATTACK_REST_P)

class	JFW_Attack_Object : public JFW_Attack_Base
{
	void	Aim (ActionParamsStruct & params) override
	{
		GameObject * target = ScriptEngine::Find_Object (Get_Int_Parameter ("ID"));

		params.Set_Movement (target, Get_Float_Parameter ("Speed"),
			Get_Float_Parameter ("ArriveDistance"));
		params.Set_Attack (target, Get_Float_Parameter ("Range"),
			Get_Float_Parameter ("Error"), Get_Bool_Parameter ("Primary"));
	}
};


REGISTER_SCRIPT_TT (JFW_Attack_Location, JFW_GOTO_P "Location:vector3," JFW_ATTACK_REST_P)

class	JFW_Attack_Location : public JFW_Attack_Base
{
	void	Aim (ActionParamsStruct & params) override
	{
		Vector3 target = Get_Vector3_Parameter ("Location");

		params.Set_Movement (target, Get_Float_Parameter ("Speed"),
			Get_Float_Parameter ("ArriveDistance"));
		params.Set_Attack (target, Get_Float_Parameter ("Range"),
			Get_Float_Parameter ("Error"), Get_Bool_Parameter ("Primary"));
	}
};


REGISTER_SCRIPT_TT (JFW_Stop_Action, "Message:int")

class	JFW_Stop_Action : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Destroy_Object, "Message:int,ID:int")

class	JFW_Destroy_Object : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")));
		}
	}
};


/******************************************************************************
*
*     Innate AI
*
*     The flags script takes -1 for "leave this one alone", which is why each
*     one is tested for being exactly 0 or 1 before it is applied.
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Innate_AI_Flags,
	"Message:int,HibernateEnable:int,EnemySeenEnable:int,InnateEnable:int,"
	"LoitersAllowed:int,InnateIsStationary:int")

class	JFW_Innate_AI_Flags : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		int hibernate = Get_Int_Parameter ("HibernateEnable");

		if (hibernate == 0 || hibernate == 1)
		{
			ScriptEngine::Enable_Hibernation (obj, hibernate != 0);
		}

		int seen = Get_Int_Parameter ("EnemySeenEnable");

		if (seen == 0 || seen == 1)
		{
			ScriptEngine::Enable_Enemy_Seen (obj, seen != 0);
		}

		int innate = Get_Int_Parameter ("InnateEnable");

		if (innate == 0)		{ ScriptEngine::Innate_Disable (obj); }
		else if (innate == 1)	{ ScriptEngine::Innate_Enable (obj); }

		int loiters = Get_Int_Parameter ("LoitersAllowed");

		if (loiters == 0 || loiters == 1)
		{
			ScriptEngine::Set_Loiters_Allowed (obj, loiters != 0);
		}

		int stationary = Get_Int_Parameter ("InnateIsStationary");

		if (stationary == 0 || stationary == 1)
		{
			ScriptEngine::Set_Innate_Is_Stationary (obj, stationary != 0);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Innate_AI_Aggressiveness, "Message:int,Aggressiveness:float")

class	JFW_Innate_AI_Aggressiveness : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Innate_Aggressiveness (obj, Get_Float_Parameter ("Aggressiveness"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Innate_AI_Cover_Probability, "Message:int,CoverProbability:float")

class	JFW_Innate_AI_Cover_Probability : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Innate_Take_Cover_Probability (obj,
				Get_Float_Parameter ("CoverProbability"));
		}
	}
};


/*
**	The donor read this script's vector3 as a float and handed it to the
**	cover-probability setter, one line copied from the script above it.  It
**	sets the home location.
*/
REGISTER_SCRIPT_TT (JFW_Innate_AI_Home_Location, "Message:int,HomeLocation:vector3")

class	JFW_Innate_AI_Home_Location : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Innate_Soldier_Home_Location (obj,
				Get_Vector3_Parameter ("HomeLocation"));
		}
	}
};


/******************************************************************************
*
*     Odds and ends on a message
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Set_Radar_Blip_State, "Message:int,BlipColor:int,BlipShape:int")

class	JFW_Set_Radar_Blip_State : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Obj_Radar_Blip_Color (obj, Get_Int_Parameter ("BlipColor"));
			ScriptEngine::Set_Obj_Radar_Blip_Shape (obj, Get_Int_Parameter ("BlipShape"));
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Give_PowerUp, "Message:int,PowerUp:string,DisplayOnHud:int,SelectWeapon:int")

class	JFW_Give_PowerUp : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		ScriptEngine::Give_PowerUp (obj, Get_Parameter ("PowerUp"),
			Get_Bool_Parameter ("DisplayOnHud"));

		if (Get_Bool_Parameter ("SelectWeapon"))
		{
			ScriptEngine::Select_Weapon (obj,
				ScriptEngine::Get_Powerup_Weapon (Get_Parameter ("PowerUp")));
		}
	}
};


/*
**	A zone that can be armed by a message and then fires the first time the
**	player walks into it.  The donor asked whether the zone was the player,
**	which it never is, so the zone never fired.
*/
REGISTER_SCRIPT_TT (JFW_Star_Trigger_Zone, "EnableMessage:int,ZoneMessage:int,ZoneParam:int")

class	JFW_Star_Trigger_Zone : public ScriptImpClass
{
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
	}

	void	Created (GameObject * /*obj*/) override
	{
		Enabled = false;
	}

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("EnableMessage"))
		{
			Enabled = true;
		}
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (Enabled && enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("ZoneMessage"),
				Get_Int_Parameter ("ZoneParam"), 0.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Encyclopedia_Reveal, "Message:int,Type:int,ID:int,DisplayUI:int")

class	JFW_Encyclopedia_Reveal : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		int		id			= Get_Int_Parameter ("ID");
		bool	revealed	= false;

		switch (Get_Int_Parameter ("Type"))
		{
			case 0:	revealed = ScriptEngine::Reveal_Encyclopedia_Character (id);	break;
			case 1:	revealed = ScriptEngine::Reveal_Encyclopedia_Weapon (id);		break;
			case 2:	revealed = ScriptEngine::Reveal_Encyclopedia_Vehicle (id);		break;
			case 3:	revealed = ScriptEngine::Reveal_Encyclopedia_Building (id);		break;
			default:																break;
		}

		if (revealed && Get_Bool_Parameter ("DisplayUI"))
		{
			ScriptEngine::Display_Encyclopedia_Event_UI();
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Poke_Send_Custom_Self, "Message:int,Type:int")

class	JFW_Poke_Send_Custom_Self : public ScriptImpClass
{
	void	Poked (GameObject * obj, GameObject * /*poker*/) override
	{
		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("Message"),
			Get_Int_Parameter ("Type"), 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Poke_Send_Custom_Poker, "Message:int,Type:int")

class	JFW_Poke_Send_Custom_Poker : public ScriptImpClass
{
	void	Poked (GameObject * obj, GameObject * poker) override
	{
		ScriptEngine::Send_Custom_Event (obj, poker, Get_Int_Parameter ("Message"),
			Get_Int_Parameter ("Type"), 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Apply_Damage_Custom, "Message:int,ID:int,Warhead:string,Amount:float")

class	JFW_Apply_Damage_Custom : public ScriptImpClass
{
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Apply_Damage (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Float_Parameter ("Amount"), Get_Parameter ("Warhead"), nullptr);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Damage_All_Objects_Area,
	"Time:float,Damage:float,Warhead:string,Distance:float,Soldiers:int,Vehicles:int,Team:int")

class	JFW_Damage_All_Objects_Area : public ScriptImpClass
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != 1) { return; }

		ScriptEngine::Damage_All_Objects_Area_By_Team (Get_Float_Parameter ("Damage"),
			Get_Parameter ("Warhead"), ScriptEngine::Get_Position (obj),
			Get_Float_Parameter ("Distance"), obj, Get_Bool_Parameter ("Soldiers"),
			Get_Bool_Parameter ("Vehicles"), Get_Int_Parameter ("Team"));

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}
};


REGISTER_SCRIPT_TT (JFW_Set_HUD_Help_Text_Player, "Message:int,String:int,Color:vector3,OnceOnly:int")

class	JFW_Set_HUD_Help_Text_Player : public ScriptImpClass
{
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		ScriptEngine::Set_HUD_Help_Text_Player (obj, Get_Int_Parameter ("String"),
			Get_Vector3_Parameter ("Color"));

		if (Get_Bool_Parameter ("OnceOnly"))
		{
			Destroy_Script();
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Set_HUD_Help_Text_Zone, "String:int,Color:vector3")

class	JFW_Set_HUD_Help_Text_Zone : public ScriptImpClass
{
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (enterer != nullptr && enterer->As_SoldierGameObj() != nullptr)
		{
			ScriptEngine::Set_HUD_Help_Text_Player (enterer, Get_Int_Parameter ("String"),
				Get_Vector3_Parameter ("Color"));
		}
	}
};


/******************************************************************************
*
*     Projectiles
*
******************************************************************************/

/*
**	Leaves a projectile behind travelling at the speed of whatever died, so a
**	shot-down shell keeps going instead of stopping in mid-air.
*/
REGISTER_SCRIPT_TT (JFW_Spawn_Projectile_Death, "Projectile:string")

class	JFW_Spawn_Projectile_Death : public ScriptImpClass
{
	void	Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		Vector3			position	= ScriptEngine::Get_Position (obj);
		GameObject *	spawned		= ScriptEngine::Create_Object (Get_Parameter ("Projectile"),
										position);

		if (spawned == nullptr)
		{
			Destroy_Script();
			return;
		}

		ScriptEngine::Set_Facing (spawned, ScriptEngine::Get_Facing (obj));

		MoveablePhysClass * from	= Moveable_Physics (obj);
		MoveablePhysClass * to		= Moveable_Physics (spawned);

		if (from != nullptr && to != nullptr)
		{
			Vector3 velocity;
			from->Get_Velocity (&velocity);
			to->Set_Velocity (velocity);
			ScriptEngine::Update_Network_Object (spawned);
		}

		Destroy_Script();
	}

	static MoveablePhysClass *	Moveable_Physics (GameObject * obj)
	{
		PhysicalGameObj * physical = (obj != nullptr) ? obj->As_PhysicalGameObj() : nullptr;

		if (physical == nullptr) { return nullptr; }

		PhysClass * physics = physical->Peek_Physical_Object();

		return (physics != nullptr) ? physics->As_MoveablePhysClass() : nullptr;
	}
};


/*
**	Tells the clients where a projectile really is, at a fixed interval.
*/
REGISTER_SCRIPT_TT (JFW_Projectile_Sync, "Time:float")

class	JFW_Projectile_Sync : public ScriptImpClass
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int /*number*/) override
	{
		ScriptEngine::Update_Network_Object (obj);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}
};


/******************************************************************************
*
*     Thrown beacons
*
******************************************************************************/

/*
**	Puts a thrown object at the thrower's muzzle travelling away from it, and
**	tells the clients about it.  Both beacon layers below do this.
*/
static void	Throw_From_Muzzle (GameObject * thrower, const char * preset, float speed)
{
	if (thrower == nullptr || preset == nullptr || preset[0] == 0) { return; }

	GameObject * thrown = ScriptEngine::Create_Object (preset,
			ScriptEngine::Get_Position (thrower));

	if (thrown == nullptr) { return; }

	ScriptEngine::Set_Player_Type (thrown, ScriptEngine::Get_Player_Type (thrower));

	PhysicalGameObj * physical = thrown->As_PhysicalGameObj();

	if (physical == nullptr) { return; }

	PhysClass * physics = physical->Peek_Physical_Object();

	if (physics == nullptr) { return; }

	physics->Set_Collision_Group (TERRAIN_AND_BULLET_COLLISION_GROUP);

	/*
	**	The muzzle is the thrower's -- the donor asked the thrown object for
	**	its own, which is not a muzzle a beacon has.
	*/
	PhysicalGameObj *	physical_thrower	= thrower->As_PhysicalGameObj();
	ArmedGameObj *		armed				= (physical_thrower != nullptr)
											? physical_thrower->As_ArmedGameObj() : nullptr;

	if (armed == nullptr) { return; }

	Matrix3D muzzle = armed->Get_Muzzle();
	Matrix3D transform (true);
	transform.Rotate_Z (muzzle.Get_Z_Rotation());
	transform.Set_Translation (muzzle.Get_Translation());
	physics->Set_Transform (transform);

	MoveablePhysClass * moveable = physics->As_MoveablePhysClass();

	if (moveable != nullptr)
	{
		moveable->Set_Velocity (muzzle.Get_X_Vector() * speed);
	}

	ScriptEngine::Update_Network_Object (thrown);
}


/*
**	Whether this Damaged call is the object shooting itself with the named
**	warhead, which is how the library asks "did the holder press fire".
*/
static bool	Is_Self_Inflicted (GameObject * obj, GameObject * damager, const char * warhead_name)
{
	if (damager != obj) { return false; }

	return ScriptEngine::Get_Warhead_Type (warhead_name) == ScriptEngine::Get_Damage_Warhead();
}


REGISTER_SCRIPT_TT (JFW_Medic_Beacon_Layer, "Beacon_Preset:string,Throw_Velocity:float,Warhead:string")

class	JFW_Medic_Beacon_Layer : public ScriptImpClass
{
	void	Damaged (GameObject * obj, GameObject * damager, float /*amount*/) override
	{
		if (!Is_Self_Inflicted (obj, damager, Get_Parameter ("Warhead")))	{ return; }
		if (damager->As_SoldierGameObj() == nullptr)						{ return; }

		Throw_From_Muzzle (damager, Get_Parameter ("Beacon_Preset"),
			Get_Float_Parameter ("Throw_Velocity"));
	}
};


/*
**	Heals hurt teammates it can see, then waits out its rate of fire.  The
**	donor's "already at full strength" test compared shield strength with
**	itself, so a teammate with full health and a stripped shield was passed
**	over.
*/
REGISTER_SCRIPT_TT (JFW_Medic_Beacon, "Time:float,Amount:float,Warhead:string,ROF:float")

class	JFW_Medic_Beacon : public ScriptImpClass
{
	enum { TIMER_LIFETIME = 1, TIMER_RELOAD = 2, TIMER_LOOK = 3 };

	bool	Ready;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Ready, 1);
	}

	void	Created (GameObject * obj) override
	{
		Ready = true;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), TIMER_LIFETIME);
		ScriptEngine::Start_Timer (obj, this, 1.0f, TIMER_LOOK);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == TIMER_LIFETIME)
		{
			ScriptEngine::Destroy_Object (obj);
			return;
		}

		if (number == TIMER_RELOAD)
		{
			Ready = true;
			return;
		}

		if (number != TIMER_LOOK) { return; }

		ScriptEngine::Start_Timer (obj, this, 1.0f, TIMER_LOOK);

		if (!Ready) { return; }

		SmartGameObj * beacon = obj->As_SmartGameObj();

		if (beacon == nullptr) { return; }

		bool healed = false;

		/*
		**	Every soldier in the level: the engine keeps one list of smart
		**	objects rather than a list of soldiers.
		*/
		for (SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List()->Head();
				node != nullptr; node = node->Next())
		{
			SmartGameObj *		smart	= node->Data();
			SoldierGameObj *	soldier	= (smart != nullptr) ? smart->As_SoldierGameObj() : nullptr;

			if (soldier == nullptr) { continue; }

			DefenseObjectClass * defense = soldier->Get_Defense_Object();

			if (defense == nullptr) { continue; }

			if (defense->Get_Health() == defense->Get_Health_Max() &&
				defense->Get_Shield_Strength() == defense->Get_Shield_Strength_Max())
			{
				continue;
			}

			if (!beacon->Is_Teammate (soldier))		{ continue; }
			if (!soldier->Is_Visible())				{ continue; }
			if (!beacon->Is_Obj_Visible (soldier))	{ continue; }

			healed = true;
			ScriptEngine::Apply_Damage (soldier, Get_Float_Parameter ("Amount"),
				Get_Parameter ("Warhead"), nullptr);
		}

		if (healed)
		{
			Ready = false;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("ROF"), TIMER_RELOAD);
		}
	}
};


/*
**	The sabotage beacon is thrown only while its holder is standing in the
**	zone that arms it; anywhere else the shot is refunded.
*/
REGISTER_SCRIPT_TT (JFW_Sabotage_Beacon_Layer,
	"Enable_Custom:int,Disable_Custom:int,Beacon_Preset:string,Throw_Velocity:float,Warhead:string")

class	JFW_Sabotage_Beacon_Layer : public ScriptImpClass
{
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
	}

	void	Created (GameObject * /*obj*/) override
	{
		Enabled = false;
	}

	void	Damaged (GameObject * obj, GameObject * damager, float /*amount*/) override
	{
		if (!Is_Self_Inflicted (obj, damager, Get_Parameter ("Warhead")))
		{
			ScriptEngine::Grant_Weapon (obj, ScriptEngine::Get_Current_Weapon (obj),
				true, 999999, false);
			return;
		}

		if (!Enabled)											{ return; }
		if (damager->As_SoldierGameObj() == nullptr)			{ return; }

		Throw_From_Muzzle (damager, Get_Parameter ("Beacon_Preset"),
			Get_Float_Parameter ("Throw_Velocity"));
	}

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Enable_Custom"))		{ Enabled = true; }
		if (type == Get_Int_Parameter ("Disable_Custom"))	{ Enabled = false; }
	}
};


REGISTER_SCRIPT_TT (JFW_Sabotage_Beacon_Zone, "Enable_Custom:int,Disable_Custom:int")

class	JFW_Sabotage_Beacon_Zone : public ScriptImpClass
{
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		ScriptEngine::Send_Custom_Event (obj, enterer, Get_Int_Parameter ("Enable_Custom"),
			0, 0.0f);
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		ScriptEngine::Send_Custom_Event (obj, exiter, Get_Int_Parameter ("Disable_Custom"),
			0, 0.0f);
	}
};


/*
**	The beacon itself, telling whatever it was planted against that it is
**	there and, later, that it is gone.  The two of them differ only in how
**	they find the thing: by preset name, or as the nearest building.
*/
class	JFW_Sabotage_Beacon_Base : public ScriptImpClass
{
protected:
	virtual GameObject *	Target (const Vector3 & position) = 0;

private:
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event (obj, Target (ScriptEngine::Get_Position (obj)),
			Get_Int_Parameter ("Sabotage_On_Custom"), 0, 0.0f);
	}

	void	Destroyed (GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event (obj, Target (ScriptEngine::Get_Position (obj)),
			Get_Int_Parameter ("Sabotage_Off_Custom"), 0, 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Sabotage_Beacon,
	"Target_Preset:string,Sabotage_On_Custom:int,Sabotage_Off_Custom:int")

class	JFW_Sabotage_Beacon : public JFW_Sabotage_Beacon_Base
{
	GameObject *	Target (const Vector3 & position) override
	{
		return ScriptEngine::Find_Nearest_Preset (position, Get_Parameter ("Target_Preset"));
	}
};


REGISTER_SCRIPT_TT (JFW_Sabotage_Beacon_2, "Sabotage_On_Custom:int,Sabotage_Off_Custom:int")

class	JFW_Sabotage_Beacon_2 : public JFW_Sabotage_Beacon_Base
{
	GameObject *	Target (const Vector3 & position) override
	{
		return ScriptEngine::Find_Closest_Building (position);
	}
};


/******************************************************************************
*
*     Veterancy
*
*     A unit is promoted by a custom event carrying its new rank.  Everything
*     a rank changes -- weapon, armour, skin, model, help text -- is one of
*     three parameters named after the rank, and some of them have a second
*     set for while the unit is deployed.
*
******************************************************************************/

/*
**	Sent to a unit when it is promoted; the parameter is the new rank.  The
**	value is the 4.8.4 library's, which levels and other scripts already use.
*/
static const int	CUSTOM_VETERANCY_PROMOTED	= (int)(0xDA000000 | 0x00010000 | 0x01);

/*
**	Sent by a deployable vehicle to itself and its own scripts as its state
**	changes: 0 stowed, 1 deploying, 2 deployed, 3 stowing.
*/
static const int	CUSTOM_VEHICLE_DEPLOY		= (int)(0xDA000000 | 0x00000000 | 0x04);

/*
**	Sent to a Communications Centre to switch the enemy radar off (0) or back
**	on (1).
*/
static const int	CUSTOM_RADAR_JAM			= (int)(0xDA000000 | 0x00000000 | 0x05);


enum JFW_Rank
{
	JFW_RANK_ROOKIE		= 0,
	JFW_RANK_VETERAN	= 1,
	JFW_RANK_ELITE		= 2
};


class	JFW_Ranked_Base : public ScriptImpClass
{
protected:
	int	Rank;
	int	Deploy_State;

	void	Reset_Rank (void)
	{
		Rank			= JFW_RANK_ROOKIE;
		Deploy_State	= 0;
	}

	/*
	**	"Weapon" and "" is Weapon_Rookie; "Skin" and "_Deployed" is
	**	Skin_Elite_Deployed.  A rank the library does not know reads as a
	**	rookie rather than walking off the end of the parameter list.
	*/
	const char *	Ranked (const char * prefix, const char * suffix = "")
	{
		const char * rank_name = "_Rookie";

		if (Rank == JFW_RANK_VETERAN)	{ rank_name = "_Veteran"; }
		else if (Rank == JFW_RANK_ELITE){ rank_name = "_Elite"; }

		StringClass name;
		name.Format ("%s%s%s", prefix, rank_name, suffix);

		return Get_Parameter (name);
	}

	/*
	**	Deploy states 0 and 1 wear the stowed set; 2 and 3 the deployed one.
	*/
	const char *	Deployed_Suffix (void) const
	{
		return (Deploy_State >= 2) ? "_Deployed" : "";
	}
};


#define JFW_RANKED_WEAPON_P	"Weapon_Rookie:string,Weapon_Veteran:string,Weapon_Elite:string"

REGISTER_SCRIPT_TT (JFW_Veteran_Weapon, JFW_RANKED_WEAPON_P)

class	JFW_Veteran_Weapon : public JFW_Ranked_Base
{
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Rank, 1);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED) { return; }

		ScriptEngine::Remove_Weapon (obj, Ranked ("Weapon"));
		Rank = (int)param;
		ScriptEngine::Grant_Weapon (obj, Ranked ("Weapon"), true, 999999, false);
		ScriptEngine::Select_Weapon (obj, Ranked ("Weapon"));
	}
};


REGISTER_SCRIPT_TT (JFW_Veteran_Weapon_Deploy,
	JFW_RANKED_WEAPON_P ",Weapon_Rookie_Deployed:string,Weapon_Veteran_Deployed:string,"
	"Weapon_Elite_Deployed:string")

class	JFW_Veteran_Weapon_Deploy : public JFW_Ranked_Base
{
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Rank, 1);
		SAVE_VARIABLE (Deploy_State, 2);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED && type != CUSTOM_VEHICLE_DEPLOY) { return; }

		ScriptEngine::Remove_Weapon (obj, Ranked ("Weapon", Deployed_Suffix()));

		if (type == CUSTOM_VETERANCY_PROMOTED)	{ Rank = (int)param; }
		else									{ Deploy_State = (int)param; }

		const char * weapon = Ranked ("Weapon", Deployed_Suffix());
		ScriptEngine::Grant_Weapon (obj, weapon, true, 999999, false);
		ScriptEngine::Select_Weapon (obj, weapon);
	}
};


#define JFW_RANKED_ARMOR_P	"Shield_Rookie:string,Shield_Veteran:string,Shield_Elite:string," \
							"Skin_Rookie:string,Skin_Veteran:string,Skin_Elite:string"

REGISTER_SCRIPT_TT (JFW_Veteran_Armor, JFW_RANKED_ARMOR_P)

class	JFW_Veteran_Armor : public JFW_Ranked_Base
{
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Rank, 1);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED) { return; }

		Rank = (int)param;
		ScriptEngine::Set_Shield_Type (obj, Ranked ("Shield"));
		ScriptEngine::Set_Skin (obj, Ranked ("Skin"));
	}
};


REGISTER_SCRIPT_TT (JFW_Veteran_Armor_Deploy,
	"Shield_Rookie:string,Shield_Veteran:string,Shield_Elite:string,"
	"Shield_Rookie_Deployed:string,Shield_Veteran_Deployed:string,Shield_Elite_Deployed:string,"
	"Skin_Rookie:string,Skin_Veteran:string,Skin_Elite:string,"
	"Skin_Rookie_Deployed:string,Skin_Veteran_Deployed:string,Skin_Elite_Deployed:string")

class	JFW_Veteran_Armor_Deploy : public JFW_Ranked_Base
{
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Rank, 1);
		SAVE_VARIABLE (Deploy_State, 2);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == CUSTOM_VETERANCY_PROMOTED)	{ Rank = (int)param; }
		else if (type == CUSTOM_VEHICLE_DEPLOY)	{ Deploy_State = (int)param; }
		else									{ return; }

		ScriptEngine::Set_Shield_Type (obj, Ranked ("Shield", Deployed_Suffix()));
		ScriptEngine::Set_Skin (obj, Ranked ("Skin", Deployed_Suffix()));
	}
};


#define JFW_RANKED_MODEL_P	"Model_Rookie:string,Model_Veteran:string,Model_Elite:string"

REGISTER_SCRIPT_TT (JFW_Veteran_Model, JFW_RANKED_MODEL_P)

class	JFW_Veteran_Model : public JFW_Ranked_Base
{
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Rank, 1);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED) { return; }

		Rank = (int)param;
		ScriptEngine::Set_Model (obj, Ranked ("Model"));
	}
};


/*
**	Announces a promotion to whichever scripts the level has listening.  The
**	donor registered these six message numbers as strings.
*/
REGISTER_SCRIPT_TT (JFW_Veteran_Custom,
	"Custom_Rookie_In:int,Custom_Veteran_In:int,Custom_Elite_In:int,"
	"Custom_Rookie_Out:int,Custom_Veteran_Out:int,Custom_Elite_Out:int")

class	JFW_Veteran_Custom : public JFW_Ranked_Base
{
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Rank, 1);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED) { return; }

		ScriptEngine::Send_Custom_Event (obj, obj, Ranked_Custom ("_Out"), 0, 0.0f);
		Rank = (int)param;
		ScriptEngine::Send_Custom_Event (obj, obj, Ranked_Custom ("_In"), 0, 0.0f);
	}

	int	Ranked_Custom (const char * suffix)
	{
		const char * rank_name = "Custom_Rookie";

		if (Rank == JFW_RANK_VETERAN)		{ rank_name = "Custom_Veteran"; }
		else if (Rank == JFW_RANK_ELITE)	{ rank_name = "Custom_Elite"; }

		StringClass name;
		name.Format ("%s%s", rank_name, suffix);

		return Get_Int_Parameter (name);
	}
};


/*
**	Writes a line on the screen of whoever is driving when they are promoted.
*/
REGISTER_SCRIPT_TT (JFW_Veteran_Help_Text,
	"StringID_Rookie:int,StringID_Veteran:int,StringID_Elite:int,Red:int,Green:int,Blue:int")

class	JFW_Veteran_Help_Text : public JFW_Ranked_Base
{
	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VETERANCY_PROMOTED, 0, 0.0f);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED) { return; }

		GameObject * reader = nullptr;

		if (obj->As_SoldierGameObj() != nullptr)		{ reader = obj; }
		else if (obj->As_VehicleGameObj() != nullptr){ reader = ScriptEngine::Get_Vehicle_Driver (obj); }

		if (reader == nullptr) { return; }

		Vector3 color ((float)Get_Int_Parameter ("Red")   / 255.0f,
					   (float)Get_Int_Parameter ("Green") / 255.0f,
					   (float)Get_Int_Parameter ("Blue")  / 255.0f);

		Rank = (int)param;
		ScriptEngine::Set_HUD_Help_Text_Player (reader, Ranked_String(), color);
	}

	int	Ranked_String (void)
	{
		if (Rank == JFW_RANK_VETERAN)		{ return Get_Int_Parameter ("StringID_Veteran"); }
		if (Rank == JFW_RANK_ELITE)			{ return Get_Int_Parameter ("StringID_Elite"); }

		return Get_Int_Parameter ("StringID_Rookie");
	}
};


/*
**	Carries a weapon from the start and only draws it once its holder reaches
**	the rank that goes with it.
*/
REGISTER_SCRIPT_TT (JFW_Veteran_Weapon_NoSwitch, "Weapon:string,Model:string,VeteranLevel:int")

class	JFW_Veteran_Weapon_NoSwitch : public ScriptImpClass
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Grant_Weapon (obj, Get_Parameter ("Weapon"), true, 999999, false);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED)					{ return; }
		if ((int)param != Get_Int_Parameter ("VeteranLevel"))	{ return; }

		ScriptEngine::Select_Weapon (obj, Get_Parameter ("Weapon"));
		ScriptEngine::Set_Model (obj, Get_Parameter ("Model"));
	}
};


/*
**	Two weapons on one vehicle with a key to change between them, unlocked at
**	a given rank.  The vehicle cannot fire while it is changing over.
*/
REGISTER_SCRIPT_TT (JFW_Veteran_Weapon_Switch,
	"Weapon1:string,Weapon2:string,Keyhook:string,Model1:string,Model2:string,"
	"VeteranLevel:int,Sound1:string,Sound2:string,SwitchTime:float")

class	JFW_Veteran_Weapon_Switch : public KeyHookScriptClass
{
	enum { TIMER_SWITCH = 1 };

	bool	Switching;
	bool	Second_Weapon;
	int		PilotID;
	int		Rank;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Switching, 1);
		SAVE_VARIABLE (Second_Weapon, 2);
		SAVE_VARIABLE (PilotID, 3);
		SAVE_VARIABLE (Rank, 4);
	}

	void	Created (GameObject * obj) override
	{
		Rank			= JFW_RANK_ROOKIE;
		Switching		= false;
		Second_Weapon	= false;
		PilotID			= 0;

		ScriptEngine::Grant_Weapon (obj, Get_Parameter ("Weapon1"), true, 999999, false);
		ScriptEngine::Grant_Weapon (obj, Get_Parameter ("Weapon2"), true, 999999, false);
	}

	void	Draw (GameObject * obj)
	{
		ScriptEngine::Select_Weapon (obj, Get_Parameter (Second_Weapon ? "Weapon2" : "Weapon1"));
		ScriptEngine::Set_Model (obj, Get_Parameter (Second_Weapon ? "Model2" : "Model1"));
	}

	bool	Unlocked (void)
	{
		return !Switching && Rank == Get_Int_Parameter ("VeteranLevel");
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			if (PilotID == 0)
			{
				Install_Hook (Get_Parameter ("Keyhook"), sender);
				PilotID = ScriptEngine::Get_ID (sender);

				if (Unlocked()) { Draw (obj); }
			}
		}
		else if (type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			if (PilotID == ScriptEngine::Get_ID (sender))
			{
				Remove_Hook();
				PilotID = 0;
			}
		}
		else if (type == CUSTOM_VETERANCY_PROMOTED)
		{
			Rank = (int)param;

			if (Unlocked()) { Draw (obj); }
		}
	}

	void	Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		Remove_Hook();
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != TIMER_SWITCH) { return; }

		Switching = false;

		VehicleGameObj * vehicle = obj->As_VehicleGameObj();

		if (vehicle != nullptr) { vehicle->Set_Scripts_Can_Fire (true); }

		Draw (obj);
	}

	void	Key_Hook (void) override
	{
		if (!Unlocked()) { return; }

		GameObject * obj = Owner();

		if (obj == nullptr) { return; }

		VehicleGameObj * vehicle = obj->As_VehicleGameObj();

		if (vehicle == nullptr) { return; }

		vehicle->Set_Scripts_Can_Fire (false);
		Switching		= true;
		Second_Weapon	= !Second_Weapon;

		ScriptEngine::Create_Sound (Get_Parameter (Second_Weapon ? "Sound2" : "Sound1"),
			ScriptEngine::Get_Position (obj), obj);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("SwitchTime"), TIMER_SWITCH);
	}
};


/******************************************************************************
*
*     Deploying
*
******************************************************************************/

/*
**	The pilot's half of a deployable vehicle: it forwards one key press to
**	the vehicle and does nothing else.  The deployable vehicle and mech
**	scripts attach it by name when somebody climbs in.
*/
REGISTER_SCRIPT_TT (Reborn_Deployable_Vehicle_Player, "Key=Deploy:string,ID=0:int,Message=0:int")

class	Reborn_Deployable_Vehicle_Player : public KeyHookScriptClass
{
	void	Created (GameObject * obj) override
	{
		Install_Hook (Get_Parameter ("Key"), obj);
	}

	void	Key_Hook (void) override
	{
		ScriptEngine::Send_Custom_Event (Owner(),
			ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
			Get_Int_Parameter ("Message"), 0, 0.0f);
	}
};


/*
**	The message the pilot's key hook sends back.  It is a fixed number in the
**	4.8.4 library and levels do not choose it.
*/
static const int	JFW_CUSTOM_DEPLOY_PRESSED	= 923572385;

static const char *	JFW_DEPLOY_PILOT_SCRIPT		= "Reborn_Deployable_Vehicle_Player";


/*
**	Deploy state, shared by the vehicle and the mech: 0 stowed, 1 deploying,
**	2 deployed, 3 stowing.
*/
enum JFW_Deploy_Mode
{
	JFW_DEPLOY_STOWED		= 0,
	JFW_DEPLOY_DEPLOYING	= 1,
	JFW_DEPLOY_DEPLOYED		= 2,
	JFW_DEPLOY_STOWING		= 3
};


/*
**	A deployable model has three names, derived from a base: the model itself,
**	"<base>_d" carrying the deploy animation, and "<base>_dd" once deployed.
*/
static StringClass	Deploying_Model (const char * base)
{
	StringClass name;
	name.Format ("%s_d", base);
	return name;
}

static StringClass	Deployed_Model (const char * base)
{
	StringClass name;
	name.Format ("%s_dd", base);
	return name;
}

static StringClass	Deploy_Animation (const char * base)
{
	StringClass name;
	name.Format ("%s_d.%s_d", base, base);
	return name;
}

static StringClass	Walk_Animation (const char * base, bool backwards)
{
	StringClass name;
	name.Format ("%s.%s_%s", base, base, backwards ? "b" : "m");
	return name;
}


/*
**	Whether the object has moved forwards or backwards since it was last
**	looked at: +1 forwards, -1 backwards, 0 not far enough to say.
**
**	The donor decided this by comparing the raw X and Y components of the
**	movement against each other under four ranges of facing, which gets the
**	answer wrong for any movement that is not along an axis -- a mech
**	stepping sideways read as walking backwards.  The question is whether
**	the movement went with the way the mech is pointing, so that is what is
**	asked here.
*/
static int	Walk_Direction (GameObject * obj, const Vector3 & from, const Vector3 & to)
{
	Vector3 travel = to - from;
	travel.Z = 0.0f;

	if (travel.Length() <= 0.1f) { return 0; }

	float facing = DEG_TO_RADF (ScriptEngine::Get_Facing (obj));

	return ((travel.X * cosf (facing) + travel.Y * sinf (facing)) >= 0.0f) ? 1 : -1;
}


/*
**	A vehicle that bolts itself down, plays a deploy animation and comes up
**	wearing a different model, then goes back again on the next key press.
*/
REGISTER_SCRIPT_TT (JFW_Deployable_Vehicle,
	JFW_RANKED_MODEL_P ",Last_Deploy_Frame:float,DeploySound:string,UndeploySound:string")

class	JFW_Deployable_Vehicle : public JFW_Ranked_Base
{
	int	PilotID;
	int	Mode;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (PilotID, 1);
		SAVE_VARIABLE (Mode, 2);
		SAVE_VARIABLE (Rank, 3);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Mode	= JFW_DEPLOY_STOWED;
		PilotID	= 0;
		ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY, JFW_DEPLOY_STOWED, 1.0f);
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_VETERANCY_PROMOTED)
		{
			Rank = (int)param;

			if (Mode == JFW_DEPLOY_STOWED)
			{
				ScriptEngine::Set_Model (obj, Ranked ("Model"));
			}
			else if (Mode == JFW_DEPLOY_DEPLOYED)
			{
				ScriptEngine::Set_Model (obj, Deployed_Model (Ranked ("Model")));
			}

			return;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			if (PilotID == 0)
			{
				PilotID = ScriptEngine::Get_ID (sender);

				StringClass params;
				params.Format ("Deploy,%d,%d", ScriptEngine::Get_ID (obj),
						JFW_CUSTOM_DEPLOY_PRESSED);
				ScriptEngine::Attach_Script (sender, JFW_DEPLOY_PILOT_SCRIPT, params);
			}

			return;
		}

		if (type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			if (PilotID == ScriptEngine::Get_ID (sender))
			{
				PilotID = 0;
				ScriptEngine::Remove_Script (sender, JFW_DEPLOY_PILOT_SCRIPT);
			}

			return;
		}

		if (type != JFW_CUSTOM_DEPLOY_PRESSED) { return; }

		VehicleGameObj * vehicle = obj->As_VehicleGameObj();

		if (vehicle == nullptr || !vehicle->Can_Drive()) { return; }

		StringClass	base		= Ranked ("Model");
		StringClass	animation	= Deploy_Animation (base);

		if (Mode == JFW_DEPLOY_STOWED)
		{
			if (vehicle->Is_Immovable()) { return; }

			vehicle->Set_Immovable (true);
			Mode = JFW_DEPLOY_DEPLOYING;
			ScriptEngine::Set_Model (obj, Deploying_Model (base));
			ScriptEngine::Set_Animation (obj, animation, false, nullptr, 0.0f, -1.0f, false);
			Begin_Transition (obj, vehicle, sender, Get_Parameter ("DeploySound"),
				JFW_DEPLOY_DEPLOYING);
		}
		else if (Mode == JFW_DEPLOY_DEPLOYED)
		{
			Mode = JFW_DEPLOY_STOWING;
			ScriptEngine::Set_Model (obj, Deploying_Model (base));
			ScriptEngine::Set_Animation (obj, animation, false, nullptr,
				Get_Float_Parameter ("Last_Deploy_Frame"), 0.0f, false);
			Begin_Transition (obj, vehicle, sender, Get_Parameter ("UndeploySound"),
				JFW_DEPLOY_STOWING);
		}
	}

	void	Begin_Transition (GameObject * obj, VehicleGameObj * vehicle, GameObject * pilot,
			const char * sound, int state)
	{
		ScriptEngine::Control_Enable (pilot, false);
		vehicle->Set_Scripts_Can_Fire (false);
		ScriptEngine::Create_Sound (sound, ScriptEngine::Get_Position (obj), obj);
		ScriptEngine::Enable_Vehicle_Transitions (obj, false);
		ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY, state, 0.0f);
	}

	void	Animation_Complete (GameObject * obj, const char * animation_name) override
	{
		StringClass base = Ranked ("Model");

		if (::_stricmp (animation_name, Deploy_Animation (base)) != 0) { return; }

		VehicleGameObj *	vehicle		= obj->As_VehicleGameObj();
		PhysicalGameObj *	physical	= obj->As_PhysicalGameObj();

		if (vehicle == nullptr || physical == nullptr) { return; }

		if (Mode == JFW_DEPLOY_DEPLOYING)
		{
			Mode = JFW_DEPLOY_DEPLOYED;
			ScriptEngine::Set_Model (obj, Deployed_Model (base));
			End_Transition (obj, vehicle, physical, false, JFW_DEPLOY_DEPLOYED);
		}
		else if (Mode == JFW_DEPLOY_STOWING)
		{
			vehicle->Set_Immovable (false);
			Mode = JFW_DEPLOY_STOWED;
			ScriptEngine::Set_Model (obj, base);
			End_Transition (obj, vehicle, physical, true, JFW_DEPLOY_STOWED);
		}
	}

	void	End_Transition (GameObject * obj, VehicleGameObj * vehicle,
			PhysicalGameObj * physical, bool engine_on, int state)
	{
		Clear_Animation (obj);
		ScriptEngine::Control_Enable (ScriptEngine::Find_Object (PilotID), true);
		vehicle->Set_Scripts_Can_Fire (true);
		ScriptEngine::Enable_Engine (obj, engine_on);
		ScriptEngine::Enable_Vehicle_Transitions (obj, true);
		ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY, state, 0.0f);
	}

	void	Destroyed (GameObject * /*obj*/) override
	{
		GameObject * pilot = (PilotID != 0) ? ScriptEngine::Find_Object (PilotID) : nullptr;

		if (pilot != nullptr)
		{
			ScriptEngine::Remove_Script (pilot, JFW_DEPLOY_PILOT_SCRIPT);
			ScriptEngine::Control_Enable (pilot, true);
		}
	}
};


/*
**	A walking vehicle: it plays a stride animation for as long as it is
**	moving, forwards or backwards, and changes model with its rank.
**
**	The donor's Created decided whether to initialise by reading a member it
**	had not written yet.
*/
REGISTER_SCRIPT_TT (JFW_Mech, JFW_RANKED_MODEL_P)

class	JFW_Mech : public JFW_Ranked_Base
{
	enum { TIMER_STRIDE = 1, TIMER_SETTLE = 4 };

	/*
	**	What the mech is doing with its legs.  The two "reversing" states are
	**	an animation being played backwards to bring the legs to rest before
	**	the other stride starts.
	*/
	enum Stride
	{
		STRIDE_NONE				= -1,
		STRIDE_FORWARD			= 1,
		STRIDE_FORWARD_ENDING	= 2,
		STRIDE_BACKWARD			= 3,
		STRIDE_BACKWARD_ENDING	= 4
	};

	Vector3	Last_Position;
	bool	Seeded;
	bool	Animating;
	int		Direction;			//	the last stride played: +1 forward, -1 back
	int		Pending;			//	0 none, +1 turn into forward, -1 turn into back
	int		State;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Last_Position, 1);
		SAVE_VARIABLE (Seeded, 2);
		SAVE_VARIABLE (Animating, 3);
		SAVE_VARIABLE (Direction, 4);
		SAVE_VARIABLE (Pending, 5);
		SAVE_VARIABLE (State, 6);
		SAVE_VARIABLE (Rank, 7);
	}

	void	Created (GameObject * obj) override
	{
		Reset_Rank();
		Last_Position	= ScriptEngine::Get_Position (obj);
		Seeded			= false;
		Animating		= false;
		Direction		= 0;
		Pending			= 0;
		State			= STRIDE_NONE;

		ScriptEngine::Start_Timer (obj, this, 0.5f, TIMER_STRIDE);
		Custom (obj, CUSTOM_VETERANCY_PROMOTED, JFW_RANK_ROOKIE, obj);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number == TIMER_SETTLE)
		{
			Animating = false;
			return;
		}

		if (number != TIMER_STRIDE) { return; }

		ScriptEngine::Start_Timer (obj, this, 0.1f, TIMER_STRIDE);

		PhysicalGameObj * physical = obj->As_PhysicalGameObj();

		if (physical == nullptr) { return; }

		/*
		**	A mech being carried is not walking.
		*/
		if (physical->Is_Attached_To_An_Object())
		{
			Clear_Animation (obj);
			return;
		}

		Vector3 position = ScriptEngine::Get_Bone_Position (obj, "Origin");

		if (!Seeded)
		{
			Last_Position	= position;
			Seeded			= true;
		}

		int direction = Walk_Direction (obj, Last_Position, position);

		if (direction == 0)
		{
			/*
			**	Standing still with the legs part-way through a stride: hold
			**	the pose rather than snapping to rest.
			*/
			if (State != STRIDE_NONE && ScriptEngine::Get_Animation_Frame (obj) != 0.0f)
			{
				float frame = ScriptEngine::Get_Animation_Frame (obj);

				ScriptEngine::Set_Animation (obj, Walk_Animation (Ranked ("Model"), Direction < 0),
					true, nullptr, frame, frame, false);

				State		= (Direction < 0) ? STRIDE_BACKWARD : STRIDE_FORWARD;
				Animating	= true;
				ScriptEngine::Start_Timer (obj, this, 0.1f, TIMER_SETTLE);
			}

			return;
		}

		Last_Position = position;

		if (Animating) { return; }

		Animating = true;

		StringClass	stride	= Walk_Animation (Ranked ("Model"), direction < 0);
		float		frame	= ScriptEngine::Get_Animation_Frame (obj);

		if (Direction == 0)
		{
			//	First step: start the stride from the beginning.
			ScriptEngine::Set_Animation (obj, stride, true, nullptr, 0.0f, -1.0f, false);
			State		= (direction < 0) ? STRIDE_BACKWARD : STRIDE_FORWARD;
			Direction	= direction;
		}
		else if (Direction != direction)
		{
			/*
			**	Changing over: run the stride already playing back to rest,
			**	and pick the other one up in Animation_Complete.
			*/
			ScriptEngine::Set_Animation (obj, Walk_Animation (Ranked ("Model"), Direction < 0),
				false, nullptr, frame, 0.0f, false);

			State	= (Direction < 0) ? STRIDE_BACKWARD_ENDING : STRIDE_FORWARD_ENDING;
			Pending	= direction;
		}
		else
		{
			//	Same way as before: carry on from where the legs are.
			ScriptEngine::Set_Animation (obj, stride, true, nullptr, frame, -1.0f, false);
			State		= (direction < 0) ? STRIDE_BACKWARD : STRIDE_FORWARD;
			Direction	= direction;
		}
	}

	void	Animation_Complete (GameObject * obj, const char * /*animation_name*/) override
	{
		if (Pending == 0)
		{
			Animating = false;
			return;
		}

		ScriptEngine::Set_Animation (obj, Walk_Animation (Ranked ("Model"), Pending < 0),
			true, nullptr, 0.0f, -1.0f, false);

		State		= (Pending < 0) ? STRIDE_BACKWARD : STRIDE_FORWARD;
		Direction	= Pending;
		Pending		= 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_VETERANCY_PROMOTED) { return; }

		float	frame		= ScriptEngine::Get_Animation_Frame (obj);
		float	target		= ScriptEngine::Get_Animation_Target_Frame (obj);

		Rank = (int)param;
		ScriptEngine::Set_Model (obj, Ranked ("Model"));

		Clear_Animation (obj);

		if (State == STRIDE_NONE) { return; }

		bool	backwards	= (State == STRIDE_BACKWARD || State == STRIDE_BACKWARD_ENDING);
		bool	looping		= (State == STRIDE_FORWARD || State == STRIDE_BACKWARD);

		ScriptEngine::Set_Animation (obj, Walk_Animation (Ranked ("Model"), backwards),
			looping, nullptr, frame, target, false);
	}
};


/*
**	A mech that also deploys.  It walks like JFW_Mech but wears one model
**	rather than three, and stops walking while it is deployed.
*/
REGISTER_SCRIPT_TT (JFW_Deployable_Mech,
	"Model_Name=none:string,Last_Deploy_Frame=0.00:float,DeploySound:string,UndeploySound:string")

class	JFW_Deployable_Mech : public ScriptImpClass
{
	enum { TIMER_STRIDE = 1 };

	Vector3	Last_Position;
	int		PilotID;
	int		Mode;
	int		Direction;
	int		Pending;
	bool	Seeded;
	bool	Animating;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Last_Position, 1);
		SAVE_VARIABLE (PilotID, 2);
		SAVE_VARIABLE (Mode, 3);
		SAVE_VARIABLE (Direction, 4);
		SAVE_VARIABLE (Pending, 5);
		SAVE_VARIABLE (Seeded, 6);
		SAVE_VARIABLE (Animating, 7);
	}

	void	Created (GameObject * obj) override
	{
		Mode			= JFW_DEPLOY_STOWED;
		PilotID			= 0;
		Last_Position	= ScriptEngine::Get_Position (obj);
		Seeded			= false;
		Animating		= false;
		Direction		= 0;
		Pending			= 0;

		ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY, JFW_DEPLOY_STOWED, 1.0f);
		ScriptEngine::Start_Timer (obj, this, 0.1f, TIMER_STRIDE);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != TIMER_STRIDE)			{ return; }
		if (Mode != JFW_DEPLOY_STOWED)		{ return; }

		ScriptEngine::Start_Timer (obj, this, 0.1f, TIMER_STRIDE);

		PhysicalGameObj * physical = obj->As_PhysicalGameObj();

		if (physical == nullptr) { return; }

		Vector3 position = ScriptEngine::Get_Bone_Position (obj, "Origin");

		if (!Seeded)
		{
			Last_Position	= position;
			Seeded			= true;
		}

		int direction = Walk_Direction (obj, Last_Position, position);

		if (direction == 0)	{ return; }

		Last_Position = position;

		if (Animating) { return; }

		Animating = true;

		const char *	base	= ScriptEngine::Get_Model (obj);
		float			frame	= ScriptEngine::Get_Animation_Frame (obj);

		Clear_Animation (obj);

		if (Direction == 0)
		{
			ScriptEngine::Set_Animation (obj, Walk_Animation (base, direction < 0),
				false, nullptr, 0.0f, -1.0f, false);
			Direction = direction;
		}
		else if (Direction != direction)
		{
			ScriptEngine::Set_Animation (obj, Walk_Animation (base, Direction < 0),
				false, nullptr, frame, 0.0f, false);
			Pending = direction;
		}
		else
		{
			ScriptEngine::Set_Animation (obj, Walk_Animation (base, direction < 0),
				false, nullptr, frame, -1.0f, false);
			Direction = direction;
		}
	}

	void	Animation_Complete (GameObject * obj, const char * animation_name) override
	{
		const char *	base		= Get_Parameter ("Model_Name");
		StringClass		deploy		= Deploy_Animation (base);

		VehicleGameObj *	vehicle		= obj->As_VehicleGameObj();
		PhysicalGameObj *	physical	= obj->As_PhysicalGameObj();

		if (vehicle == nullptr || physical == nullptr) { return; }

		if (::_stricmp (animation_name, deploy) == 0)
		{
			if (Mode == JFW_DEPLOY_DEPLOYING)
			{
				Mode = JFW_DEPLOY_DEPLOYED;
				ScriptEngine::Set_Model (obj, Deployed_Model (base));
				Clear_Animation (obj);
				ScriptEngine::Control_Enable (ScriptEngine::Find_Object (PilotID), true);
				vehicle->Set_Scripts_Can_Fire (true);
				ScriptEngine::Enable_Engine (obj, false);
				ScriptEngine::Enable_Vehicle_Transitions (obj, true);
				ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY,
					JFW_DEPLOY_DEPLOYED, 0.0f);
			}
			else if (Mode == JFW_DEPLOY_STOWING)
			{
				vehicle->Set_Immovable (false);
				Mode = JFW_DEPLOY_STOWED;
				ScriptEngine::Set_Model (obj, base);
				Clear_Animation (obj);
				ScriptEngine::Control_Enable (ScriptEngine::Find_Object (PilotID), true);
				vehicle->Set_Scripts_Can_Fire (true);
				ScriptEngine::Enable_Engine (obj, true);
				ScriptEngine::Enable_Vehicle_Transitions (obj, true);
				ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY,
					JFW_DEPLOY_STOWED, 0.0f);

				Seeded		= false;
				Animating	= false;
				Direction	= 0;
				Pending		= 0;
				ScriptEngine::Start_Timer (obj, this, 0.1f, TIMER_STRIDE);
			}

			return;
		}

		/*
		**	A stride finished.  Either take up the one that was waiting for
		**	it, or come to rest.
		*/
		if (::_stricmp (animation_name, Walk_Animation (base, false)) != 0 &&
			::_stricmp (animation_name, Walk_Animation (base, true)) != 0)
		{
			return;
		}

		if (Pending == 0)
		{
			Animating = false;
			return;
		}

		Clear_Animation (obj);
		ScriptEngine::Set_Animation (obj, Walk_Animation (ScriptEngine::Get_Model (obj),
			Pending < 0), false, nullptr, 0.0f, -1.0f, false);

		Direction	= Pending;
		Pending		= 0;
	}

	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			if (PilotID == 0)
			{
				PilotID = ScriptEngine::Get_ID (sender);

				StringClass params;
				params.Format ("Deploy,%d,%d", ScriptEngine::Get_ID (obj),
						JFW_CUSTOM_DEPLOY_PRESSED);
				ScriptEngine::Attach_Script (sender, JFW_DEPLOY_PILOT_SCRIPT, params);
			}

			return;
		}

		if (type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			if (PilotID == ScriptEngine::Get_ID (sender))
			{
				PilotID = 0;
				ScriptEngine::Remove_Script (sender, JFW_DEPLOY_PILOT_SCRIPT);
			}

			return;
		}

		if (type != JFW_CUSTOM_DEPLOY_PRESSED) { return; }

		VehicleGameObj *	vehicle		= obj->As_VehicleGameObj();
		PhysicalGameObj *	physical	= obj->As_PhysicalGameObj();

		if (vehicle == nullptr || physical == nullptr || !vehicle->Can_Drive()) { return; }

		const char *	base		= Get_Parameter ("Model_Name");
		StringClass		animation	= Deploy_Animation (base);

		if (Mode == JFW_DEPLOY_STOWED)
		{
			if (vehicle->Is_Immovable()) { return; }

			vehicle->Set_Immovable (true);
			Mode	= JFW_DEPLOY_DEPLOYING;
			Seeded	= false;
			ScriptEngine::Set_Model (obj, Deploying_Model (base));
			Clear_Animation (obj);
			ScriptEngine::Set_Animation (obj, animation, false, nullptr, 0.0f, -1.0f, false);
			Begin_Transition (obj, vehicle, sender, Get_Parameter ("DeploySound"),
				JFW_DEPLOY_DEPLOYING);
		}
		else if (Mode == JFW_DEPLOY_DEPLOYED)
		{
			Mode = JFW_DEPLOY_STOWING;
			ScriptEngine::Set_Model (obj, Deploying_Model (base));
			Clear_Animation (obj);
			ScriptEngine::Set_Animation (obj, animation, false, nullptr,
				Get_Float_Parameter ("Last_Deploy_Frame"), 0.0f, false);
			Begin_Transition (obj, vehicle, sender, Get_Parameter ("UndeploySound"),
				JFW_DEPLOY_STOWING);
		}
	}

	void	Begin_Transition (GameObject * obj, VehicleGameObj * vehicle, GameObject * pilot,
			const char * sound, int state)
	{
		ScriptEngine::Control_Enable (pilot, false);
		vehicle->Set_Scripts_Can_Fire (false);
		ScriptEngine::Create_Sound (sound, ScriptEngine::Get_Position (obj), obj);
		ScriptEngine::Enable_Vehicle_Transitions (obj, false);
		ScriptEngine::Send_Custom_Event (obj, obj, CUSTOM_VEHICLE_DEPLOY, state, 0.0f);
	}

	void	Destroyed (GameObject * /*obj*/) override
	{
		GameObject * pilot = (PilotID != 0) ? ScriptEngine::Find_Object (PilotID) : nullptr;

		if (pilot != nullptr)
		{
			ScriptEngine::Remove_Script (pilot, JFW_DEPLOY_PILOT_SCRIPT);
			ScriptEngine::Control_Enable (pilot, true);
		}
	}
};


/******************************************************************************
*
*     Radar jamming
*
*     A jammer near enough to the enemy Communications Centre keeps telling it
*     to switch its radar off; the building listens and does.  The team a
*     jammer works against is the other one from whoever is driving it.
*
******************************************************************************/

/*
**	The side a jammer aims at: the one its driver is not on.
*/
static int	Enemy_Of (int player_type)
{
	return (player_type != PLAYERTYPE_NOD) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;
}


/*
**	Whether the jammer is close enough to the given Communications Centre to
**	be jamming it at all.
*/
static bool	Within_Range (GameObject * jammer, GameObject * centre, float range)
{
	return ScriptEngine::Get_Distance (ScriptEngine::Get_Position (centre),
			ScriptEngine::Get_Position (jammer)) < range;
}


REGISTER_SCRIPT_TT (JFW_Deployable_Jammer, "Time:float,Range:float")

class	JFW_Deployable_Jammer : public ScriptImpClass
{
	enum { TIMER_JAM = 1 };

	int		Mode;
	int		Owning_Type;
	bool	Jamming;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Mode, 1);
		SAVE_VARIABLE (Owning_Type, 2);
		SAVE_VARIABLE (Jamming, 3);
	}

	void	Created (GameObject * obj) override
	{
		Mode		= JFW_DEPLOY_STOWED;
		Jamming		= false;
		Owning_Type	= PLAYERTYPE_NEUTRAL;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), TIMER_JAM);
	}

	/*
	**	The dish turns for as long as the jammer is manned or deployed.
	*/
	void	Spin (GameObject * obj)
	{
		const char *	model = ScriptEngine::Get_Model (obj);
		StringClass		animation;
		animation.Format ("%s.%s", model, model);
		ScriptEngine::Set_Animation (obj, animation, true, nullptr, 0.0f, -1.0f, false);
	}

	void	Stop_Jamming (GameObject * obj)
	{
		if (!Jamming) { return; }

		Jamming = false;

		GameObject * centre = ScriptEngine::Find_Com_Center (Enemy_Of (Owning_Type));

		if (centre != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, centre, CUSTOM_RADAR_JAM, 0, 0.0f);
		}
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == CUSTOM_VEHICLE_DEPLOY)
		{
			if (Mode == JFW_DEPLOY_DEPLOYING || Mode == JFW_DEPLOY_STOWING)
			{
				Spin (obj);
			}

			Mode = (int)param;
			return;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			if (Mode == JFW_DEPLOY_STOWED)
			{
				Spin (obj);
			}

			int driver_type = ScriptEngine::Get_Player_Type (sender);

			if (Owning_Type != driver_type && Mode == JFW_DEPLOY_DEPLOYED)
			{
				Stop_Jamming (obj);
				ScriptEngine::Set_Player_Type (obj, driver_type);
			}

			Owning_Type = driver_type;
			return;
		}

		if (type != CUSTOM_EVENT_VEHICLE_EXITED) { return; }

		Owning_Type = ScriptEngine::Get_Player_Type (sender);

		if (Mode == JFW_DEPLOY_STOWED)
		{
			Clear_Animation (obj);
		}
		else if (Mode == JFW_DEPLOY_DEPLOYED)
		{
			ScriptEngine::Update_Network_Object (obj);
			ScriptEngine::Set_Player_Type (obj, Owning_Type);
		}
	}

	void	Destroyed (GameObject * obj) override
	{
		Stop_Jamming (obj);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != TIMER_JAM) { return; }

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), TIMER_JAM);

		GameObject * centre = ScriptEngine::Find_Com_Center (Enemy_Of (Owning_Type));

		if (centre == nullptr) { return; }

		bool active = (Mode == JFW_DEPLOY_DEPLOYED) ||
			(Mode == JFW_DEPLOY_STOWED && ScriptEngine::Get_Vehicle_Occupant_Count (obj) > 0);

		bool jamming = active && Within_Range (obj, centre, Get_Float_Parameter ("Range"));

		/*
		**	Keep saying so while it is jamming, so a second jammer leaving
		**	does not turn the radar back on under this one.
		*/
		if (Jamming != jamming || jamming)
		{
			Jamming = jamming;
			ScriptEngine::Send_Custom_Event (obj, centre, CUSTOM_RADAR_JAM, jamming ? 1 : 0, 0.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Jammer, "Time:float,Range:float")

class	JFW_Jammer : public ScriptImpClass
{
	enum { TIMER_JAM = 1 };

	int		Owning_Type;
	bool	Jamming;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Owning_Type, 1);
		SAVE_VARIABLE (Jamming, 2);
	}

	void	Created (GameObject * obj) override
	{
		Jamming		= false;
		Owning_Type	= PLAYERTYPE_NEUTRAL;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), TIMER_JAM);
	}

	void	Destroyed (GameObject * obj) override
	{
		if (!Jamming) { return; }

		Jamming = false;

		GameObject * centre = ScriptEngine::Find_Com_Center (Enemy_Of (Owning_Type));

		if (centre != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, centre, CUSTOM_RADAR_JAM, 0, 0.0f);
		}
	}

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED || type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			Owning_Type = ScriptEngine::Get_Player_Type (sender);
		}
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != TIMER_JAM) { return; }

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), TIMER_JAM);

		GameObject * centre = ScriptEngine::Find_Com_Center (Enemy_Of (Owning_Type));

		if (centre == nullptr) { return; }

		bool jamming = (ScriptEngine::Get_Vehicle_Occupant_Count (obj) > 0) &&
			Within_Range (obj, centre, Get_Float_Parameter ("Range"));

		if (Jamming != jamming || jamming)
		{
			Jamming = jamming;
			ScriptEngine::Send_Custom_Event (obj, centre, CUSTOM_RADAR_JAM, jamming ? 1 : 0, 0.0f);
		}
	}
};


/*
**	The listening half: a Communications Centre that switches its team's radar
**	off while it is being jammed, and plays one sound to its own side and
**	another to the side doing the jamming.  A building without power cannot
**	be jammed because its radar is already down.
*/
REGISTER_SCRIPT_TT (JFW_Jammer_Building,
	"StartSound1:string,StopSound1:string,StartSound2:string,StopSound2:string")

class	JFW_Jammer_Building : public ScriptImpClass
{
	bool	Radar_On;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Radar_On, 1);
	}

	void	Created (GameObject * /*obj*/) override
	{
		Radar_On = true;
	}

	void	Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		Destroy_Script();
	}

	void	Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != CUSTOM_RADAR_JAM) { return; }

		int team = ScriptEngine::Get_Player_Type (obj);

		if (!ScriptEngine::Is_Base_Powered (team)) { return; }

		bool jammed = (param != 0);

		if (jammed == !Radar_On) { return; }

		Radar_On = !jammed;

		ScriptEngine::Create_2D_Sound_Team (team,
			Get_Parameter (jammed ? "StartSound1" : "StopSound1"));
		ScriptEngine::Create_2D_Sound_Team (Enemy_Of (team),
			Get_Parameter (jammed ? "StartSound2" : "StopSound2"));

		ScriptEngine::Enable_Base_Radar (team, Radar_On);
	}
};


/******************************************************************************
*
*     Small ones
*
******************************************************************************/

/*
**	Puts a second object down in front of whoever fired the named warhead at
**	themselves, which is how a deployable item is placed.
*/
REGISTER_SCRIPT_TT (JFW_Deployable_Object, "Key:string,Z_Offset:float,Distance:float,Preset:string")

class	JFW_Deployable_Object : public ScriptImpClass
{
	void	Damaged (GameObject * obj, GameObject * damager, float /*amount*/) override
	{
		if (!Is_Self_Inflicted (obj, damager, Get_Parameter ("Key"))) { return; }

		GameObject *	owner		= Owner();
		Vector3			position	= ScriptEngine::Get_Position (owner);
		float			distance	= Get_Float_Parameter ("Distance");
		float			angle		= DEG_TO_RADF (ScriptEngine::Get_Facing (owner));

		position.Z += Get_Float_Parameter ("Z_Offset");
		position.X += cosf (angle) * distance;
		position.Y += sinf (angle) * distance;

		GameObject * placed = ScriptEngine::Create_Object (Get_Parameter ("Preset"), position);

		if (placed == nullptr) { return; }

		ScriptEngine::Set_Player_Type (placed, ScriptEngine::Get_Player_Type (owner));

		PhysicalGameObj * physical = placed->As_PhysicalGameObj();

		if (physical != nullptr && physical->Peek_Physical_Object() != nullptr)
		{
			physical->Peek_Physical_Object()->Set_Collision_Group (
					TERRAIN_AND_BULLET_COLLISION_GROUP);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Object_Spawn, "Object:string")

class	JFW_Object_Spawn : public ScriptImpClass
{
	int	SpawnedID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (SpawnedID, 1);
	}

	void	Created (GameObject * obj) override
	{
		SpawnedID = 0;

		GameObject * spawned = ScriptEngine::Create_Object (Get_Parameter ("Object"),
				ScriptEngine::Get_Position (obj));

		if (spawned != nullptr) { SpawnedID = ScriptEngine::Get_ID (spawned); }
	}

	void	Destroyed (GameObject * /*obj*/) override
	{
		GameObject * spawned = (SpawnedID != 0) ? ScriptEngine::Find_Object (SpawnedID) : nullptr;

		if (spawned != nullptr) { ScriptEngine::Destroy_Object (spawned); }
	}
};


/*
**	The donor registers Bone with a capital S on its type, which is not a
**	type the parameter parser knows.
*/
REGISTER_SCRIPT_TT (JFW_Object_Spawn_At_Bone, "Object:string,Bone:string")

class	JFW_Object_Spawn_At_Bone : public ScriptImpClass
{
	int	SpawnedID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (SpawnedID, 1);
	}

	void	Created (GameObject * obj) override
	{
		SpawnedID = 0;

		GameObject * spawned = ScriptEngine::Create_Object_At_Bone (obj,
				Get_Parameter ("Object"), Get_Parameter ("Bone"));

		if (spawned != nullptr)
		{
			ScriptEngine::Attach_To_Object_Bone (spawned, obj, Get_Parameter ("Bone"));
			SpawnedID = ScriptEngine::Get_ID (spawned);
		}
	}

	void	Destroyed (GameObject * /*obj*/) override
	{
		GameObject * spawned = (SpawnedID != 0) ? ScriptEngine::Find_Object (SpawnedID) : nullptr;

		if (spawned != nullptr) { ScriptEngine::Destroy_Object (spawned); }
	}
};


REGISTER_SCRIPT_TT (JFW_Disable_Engine, "")

class	JFW_Disable_Engine : public ScriptImpClass
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Engine (obj, false);
		Destroy_Script();
	}
};


REGISTER_SCRIPT_TT (JFW_Hide_Object, "Time:float")

class	JFW_Hide_Object : public ScriptImpClass
{
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Set_Is_Rendered (obj, false);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), 1);
	}

	void	Timer_Expired (GameObject * obj, int /*number*/) override
	{
		ScriptEngine::Set_Is_Rendered (obj, true);
		Destroy_Script();
	}
};
