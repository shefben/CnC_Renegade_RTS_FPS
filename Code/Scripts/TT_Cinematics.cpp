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
*     TT_Cinematics.cpp
*
* DESCRIPTION
*     What starts a cinematic, and what an actor in one is told to do.
*
*     Native port of the twelve scripts in the 4.8.4 library's jfwcine.cpp
*     that are not the cinematic interpreter itself.  That interpreter is
*     Test_Cinematic, which the stock catalog already had and which now
*     carries 4.8.4's corrections and answers to its second name as well;
*     see Test_Cinematic.cpp.
*
*     Ten of the twelve are the same three lines -- put an invisible object
*     down, face it, hang the interpreter on it -- behind ten different
*     triggers.  They are one base and a trigger here.
*
*     Defects in the donor, fixed:
*
*     - The two random-script variants picked from five names with
*       Get_Random_Int(1,5), whose upper bound is exclusive, so Script_Name5
*       could never be chosen.
*
*     - JFW_Customs_Play_Cinematic treated an unset message number as
*       "already heard", which is what makes eight optional gates work, but
*       it read the eight parameters once in Created and never noticed a
*       message arriving with the value zero.  Unchanged in behaviour; the
*       gate array is just held rather than re-derived.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "actionparams.h"
#include "playertype.h"

#include <math.h>
#include <stdio.h>


/*
**	Every trigger in this file does the same thing when it fires: an
**	invisible object at a named place, facing a named way, with the cinematic
**	interpreter attached and the control file named as its parameter.
*/
static void	Play_Cinematic (const char * control_file, const Vector3 & position, float facing)
{
	GameObject * host = ScriptEngine::Create_Object ("Invisible_Object", position);
	if (host == nullptr) { return; }

	ScriptEngine::Set_Facing (host, facing);
	ScriptEngine::Attach_Script (host, "Test_Cinematic", control_file);
}


/*
**	The donor spelled the position parameter "location" on some of these and
**	"Location" on others.  Both spellings are what the levels carry, so both
**	are asked for.
*/
static Vector3	Cinematic_Position (ScriptImpClass * script)
{
	const char * lower = script->Get_Parameter ("location");

	return ((lower != nullptr) && (*lower != 0))
			? script->Get_Vector3_Parameter ("location")
			: script->Get_Vector3_Parameter ("Location");
}


class	JFW_Play_Cinematic_Base : public ScriptImpClass
{
protected:
	//	Which of the registered script names to play.  One name for most of
	//	them; one of five, chosen at random, for the two that say so.
	virtual const char *	Control_File (void)		{ return Get_Parameter ("Script_Name"); }

	void	Play (void)
	{
		Play_Cinematic (Control_File(), Cinematic_Position (this),
				Get_Float_Parameter ("Facing"));
	}
};


/******************************************************************************
*
*     Triggers
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Death_Play_Cinematic, "Script_Name:string,location:vector3,Facing:float")

class	JFW_Death_Play_Cinematic : public JFW_Play_Cinematic_Base
{
public:
	void	Killed (GameObject * /*obj*/, GameObject * /*killer*/) override		{ Play(); }
};


REGISTER_SCRIPT_TT (JFW_Zone_Play_Cinematic, "Script_Name:string,location:vector3,Player_Type:int,Facing:float")

class	JFW_Zone_Play_Cinematic : public JFW_Play_Cinematic_Base
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		Play();
	}
};


REGISTER_SCRIPT_TT (JFW_Custom_Play_Cinematic, "Message:int,Script_Name:string,Location:vector3,Facing:float")

class	JFW_Custom_Play_Cinematic : public JFW_Play_Cinematic_Base
{
public:
	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message")) { return; }

		Play();
	}
};


/*
**	Eight gates, all of which have to have been heard before the cinematic
**	plays, and any of which can be left unset to mean "not a gate".
*/
REGISTER_SCRIPT_TT (JFW_Customs_Play_Cinematic, "Message1:int,Message2:int,Message3:int,Message4:int,Message5:int,Message6:int,Message7:int,Message8:int,Script_Name:string,Location:vector3,Facing:float")

class	JFW_Customs_Play_Cinematic : public JFW_Play_Cinematic_Base
{
	enum { GATES = 8 };

	bool	Heard[GATES];
	bool	Played;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Heard, 1);
		SAVE_VARIABLE (Played, 2);
	}

	static void	Gate_Name (char * buffer, int size, int index)
	{
		::snprintf (buffer, size, "Message%d", index + 1);
	}

public:
	void	Created (GameObject * /*obj*/) override
	{
		Played = false;

		for (int i = 0; i < GATES; i++) {
			char name[16];
			Gate_Name (name, sizeof (name), i);

			//	An unset message number is a gate that was never asked for.
			Heard[i] = (Get_Int_Parameter (name) == 0);
		}
	}

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		for (int i = 0; i < GATES; i++) {
			char name[16];
			Gate_Name (name, sizeof (name), i);

			if (type == Get_Int_Parameter (name)) {
				Heard[i] = true;
			}
		}

		if (Played) { return; }

		for (int i = 0; i < GATES; i++) {
			if (!Heard[i]) { return; }
		}

		Played = true;
		Play();
	}
};


/*
**	A timer trigger, in four spellings: fixed or random delay, one script
**	name or one of five.
*/
class	JFW_Timer_Play_Cinematic_Base : public JFW_Play_Cinematic_Base
{
protected:
	//	The delay a fixed-time script uses; the random ones roll instead.
	virtual float	Delay (void)		{ return Get_Float_Parameter ("Time"); }

	void	Start (GameObject * obj)
	{
		ScriptEngine::Start_Timer (obj, this, Delay(), Get_Int_Parameter ("TimerNum"));
	}

public:
	void	Created (GameObject * obj) override		{ Start (obj); }

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) { return; }

		Play();

		if (Get_Int_Parameter ("Repeat") == 1) {
			Start (obj);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Timer_Play_Cinematic, "Time:float,TimerNum:int,Repeat:int,Script_Name:string,Location:vector3,Facing:float")

class	JFW_Timer_Play_Cinematic : public JFW_Timer_Play_Cinematic_Base
{
};


REGISTER_SCRIPT_TT (JFW_Random_Timer_Play_Cinematic, "TimeMin:float,TimeMax:float,TimerNum:int,Repeat:int,Script_Name:string,Location:vector3,Facing:float")

class	JFW_Random_Timer_Play_Cinematic : public JFW_Timer_Play_Cinematic_Base
{
protected:
	float	Delay (void) override
	{
		return ScriptEngine::Get_Random (Get_Float_Parameter ("TimeMin"),
				Get_Float_Parameter ("TimeMax"));
	}
};


REGISTER_SCRIPT_TT (JFW_Timer_Play_Random_Cinematic, "Time:float,TimerNum:int,Repeat:int,Script_Name1:string,Script_Name2:string,Script_Name3:string,Script_Name4:string,Script_Name5:string,Location:vector3,Facing:float")

class	JFW_Timer_Play_Random_Cinematic : public JFW_Timer_Play_Cinematic_Base
{
protected:
	const char *	Control_File (void) override
	{
		char name[24];
		::snprintf (name, sizeof (name), "Script_Name%d", ScriptEngine::Get_Random_Int (1, 6));

		return Get_Parameter (name);
	}
};


REGISTER_SCRIPT_TT (JFW_Random_Timer_Play_Random_Cinematic, "TimeMin:float,TimeMax:float,TimerNum:int,Repeat:int,Script_Name1:string,Script_Name2:string,Script_Name3:string,Script_Name4:string,Script_Name5:string,Location:vector3,Facing:float")

class	JFW_Random_Timer_Play_Random_Cinematic : public JFW_Timer_Play_Cinematic_Base
{
protected:
	float	Delay (void) override
	{
		return ScriptEngine::Get_Random (Get_Float_Parameter ("TimeMin"),
				Get_Float_Parameter ("TimeMax"));
	}

	const char *	Control_File (void) override
	{
		char name[24];
		::snprintf (name, sizeof (name), "Script_Name%d", ScriptEngine::Get_Random_Int (1, 6));

		return Get_Parameter (name);
	}
};


/*
**	The one trigger that plays immediately as well as on the timer, and never
**	stops.
*/
REGISTER_SCRIPT_TT (JFW_Looping_Cinematic, "Script_Name:string,Location:vector3,Facing:float,Time:float,TimerNum:int")

class	JFW_Looping_Cinematic : public JFW_Play_Cinematic_Base
{
	void	Start (GameObject * obj)
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

public:
	void	Created (GameObject * obj) override
	{
		Start (obj);
		Play();
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) { return; }

		Start (obj);
		Play();
	}
};


/******************************************************************************
*
*     Actors
*
*     Attached to an object inside a cinematic rather than to the trigger
*     that started one.
*
******************************************************************************/

/*
**	Fires at something for as long as the cinematic wants, then hands the
**	object back to whatever it was doing.
*/
class	JFW_Cinematic_Attack_Base : public ScriptImpClass
{
	enum { ACTION_ATTACK = 40016 };

protected:
	virtual void	Aim (ActionParamsStruct & params) = 0;

public:
	void	Created (GameObject * obj) override
	{
		ActionParamsStruct	params;

		params.Set_Basic (this, (float)Get_Int_Parameter ("Priority"), ACTION_ATTACK);
		Aim (params);

		ScriptEngine::Action_Attack (obj, params);
	}

	void	Action_Complete (GameObject * obj, int /*action_id*/,
			ActionCompleteReason /*reason*/) override
	{
		ScriptEngine::Action_Reset (obj, 100.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Cinematic_Attack, "Priority=96:int,ID:int,Range:float,Deviation:float,Primary:int")

class	JFW_Cinematic_Attack : public JFW_Cinematic_Attack_Base
{
protected:
	void	Aim (ActionParamsStruct & params) override
	{
		params.Set_Attack (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Float_Parameter ("Range"), Get_Float_Parameter ("Deviation"),
				Get_Bool_Parameter ("Primary"));

		//	A cinematic shot is meant to be taken whether or not the engine
		//	thinks the line of fire is clear.
		params.AttackCheckBlocked = false;
	}
};


REGISTER_SCRIPT_TT (JFW_Cinematic_Attack_Position, "Priority=96:int,Position:vector3,Range:float,Deviation:float,Primary:int")

class	JFW_Cinematic_Attack_Position : public JFW_Cinematic_Attack_Base
{
protected:
	void	Aim (ActionParamsStruct & params) override
	{
		params.Set_Attack (Get_Vector3_Parameter ("Position"),
				Get_Float_Parameter ("Range"), Get_Float_Parameter ("Deviation"),
				Get_Bool_Parameter ("Primary"));
	}
};


/*
**	Removes whatever it is attached to, without a killer and without the
**	points a kill would pay.
*/
REGISTER_SCRIPT_TT (JFW_Cinematic_Kill_Object, "")

class	JFW_Cinematic_Kill_Object : public ScriptImpClass
{
public:
	void	Created (GameObject * obj) override
	{
		ScriptEngine::Apply_Damage (obj, 10000.0f, "BlamoKiller", nullptr);

		Destroy_Script();
	}
};
