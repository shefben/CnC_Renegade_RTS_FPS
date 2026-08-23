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
*     TT_Customs.cpp
*
* DESCRIPTION
*     Custom messages: the wiring a level is built out of.  A custom message
*     is a number one object sends another, and almost every script here does
*     one of two things with it -- waits for a number and then acts, or acts
*     and then sends a number on.  Between them they let a level author build
*     a chain of consequences without writing any code.
*
*     Native port of the 4.8.4 library's jfwcust.cpp.
*
*     One defect runs through the whole donor file and is fixed everywhere
*     here.  `Auto_Save_Variable` takes a pointer, a size and an id, and most
*     of this file passed the last two the other way round: an object id was
*     saved as one byte under id 4, a bool as nine bytes under an id another
*     variable already owned.  So a level that saved and reloaded came back
*     with its wiring in a state it had never been in -- object ids above 255
*     truncated, counters and latches sharing storage.  Every save list here
*     is written the right way round.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "physicalgameobj.h"
#include "playertype.h"


/*
**	Clearing an animation.  There is no separate command for it: an empty
**	animation name is how the engine is told to stop.
*/
static void Clear_Animation (GameObject * obj)
{
	if (obj != nullptr)
	{
		ScriptEngine::Set_Animation (obj, "", false);
	}
}


/*
**	A level writes "0" in a subobject parameter to mean the whole model.  The
**	engine wants a null pointer for that.
*/
static const char * Subobject_Or_Whole (const char * name)
{
	if (name == nullptr || ::_stricmp (name, "0") == 0)
	{
		return nullptr;
	}

	return name;
}


////////////////////////////////////////////////////////////////////////////
//
//	Hearing a message and passing one on
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Reflect_Custom

  Sends whatever it is told straight back to whoever told it.
*/

DECLARE_SCRIPT_TT (JFW_Reflect_Custom, "")
{
	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		ScriptEngine::Send_Custom_Event (obj, sender, type, (int)param, 0.0f);
	}
};


/*JFW_Reflect_Custom_Delay

  The same, after a wait.
*/

DECLARE_SCRIPT_TT (JFW_Reflect_Custom_Delay, "Delay:float")
{
	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		ScriptEngine::Send_Custom_Event (obj, sender, type, (int)param,
				Get_Float_Parameter ("Delay"));
	}
};


/*JFW_Custom_Send_Custom

  Hears one message and sends another to a named object.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Send_Custom,
		"RecieveMessage:int,SendMessage:int,SendParam:int,SendID:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("RecieveMessage"))
		{
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("SendID")),
					Get_Int_Parameter ("SendMessage"), Get_Int_Parameter ("SendParam"), 0.0f);
		}
	}
};


/*JFW_Custom_Send_Random_Custom

  The same, but only some of the time.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Send_Random_Custom,
		"RecieveMessage:int,SendMessage:int,SendParam:int,SendID:int,Percentage:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("RecieveMessage"))
		{
			return;
		}

		if (ScriptEngine::Get_Random_Int (0, 99) < Get_Int_Parameter ("Percentage"))
		{
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("SendID")),
					Get_Int_Parameter ("SendMessage"), Get_Int_Parameter ("SendParam"), 0.0f);
		}
	}
};


/*JFW_Custom_Multiple_Send_Custom

  Counts a message and sends one on when it has heard it enough times.  Then
  it is done: it never fires again.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Multiple_Send_Custom,
		"RecieveMessage:int,RecieveCount:int,SendMessage:int,SendParam:int,SendID:int")
{
	int	Times_Received;
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Times_Received,	1);
		SAVE_VARIABLE (Enabled,			2);
	}

	void Created (GameObject * /*obj*/) override
	{
		Times_Received	= 0;
		Enabled			= true;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (!Enabled || type != Get_Int_Parameter ("RecieveMessage"))
		{
			return;
		}

		Times_Received ++;

		//
		//	The count was checked outside the message test, so any other
		//	message arriving after the last one would fire it a second time.
		//
		if (Times_Received >= Get_Int_Parameter ("RecieveCount"))
		{
			Enabled = false;
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("SendID")),
					Get_Int_Parameter ("SendMessage"), Get_Int_Parameter ("SendParam"), 0.0f);
		}
	}
};


/*JFW_Custom_Multiple_Send_Custom_2

  The same, except that it starts counting again and so fires every time.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Multiple_Send_Custom_2,
		"RecieveMessage:int,RecieveCount:int,SendMessage:int,SendParam:int,SendID:int")
{
	int	Times_Received;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Times_Received, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		Times_Received = 0;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("RecieveMessage"))
		{
			return;
		}

		Times_Received ++;
		if (Times_Received >= Get_Int_Parameter ("RecieveCount"))
		{
			Times_Received = 0;
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("SendID")),
					Get_Int_Parameter ("SendMessage"), Get_Int_Parameter ("SendParam"), 0.0f);
		}
	}
};


/*JFW_Customs_Send_Custom

  Waits for eight messages, in any order, and sends one on once every one of
  them has arrived.  A message left at zero counts as already heard, so a
  level that only needs three of the eight leaves the other five alone.
*/

DECLARE_SCRIPT_TT (JFW_Customs_Send_Custom,
		"Message1:int,Message2:int,Message3:int,Message4:int,Message5:int,Message6:int,"
		"Message7:int,Message8:int,Message:int,Param:int,ID:int")
{
	enum { HEARD_COUNT = 8 };

	bool	Heard[HEARD_COUNT];
	bool	Triggered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Heard,		1);
		SAVE_VARIABLE (Triggered,	2);
	}

	void Created (GameObject * /*obj*/) override
	{
		for (int index = 0; index < HEARD_COUNT; index ++)
		{
			Heard[index] = (Slot_Message (index) == 0);
		}

		Triggered = false;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (Triggered)
		{
			return;
		}

		for (int index = 0; index < HEARD_COUNT; index ++)
		{
			if (type == Slot_Message (index))
			{
				Heard[index] = true;
			}
		}

		for (int index = 0; index < HEARD_COUNT; index ++)
		{
			if (!Heard[index])
			{
				return;
			}
		}

		Triggered = true;
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), Get_Int_Parameter ("Param"), 0.0f);
	}

	int Slot_Message (int index)
	{
		static const char * const	_names[HEARD_COUNT] =
		{
			"Message1", "Message2", "Message3", "Message4",
			"Message5", "Message6", "Message7", "Message8"
		};

		return Get_Int_Parameter (_names[index]);
	}
};


/*JFW_Custom_Send_Custom_Parameter

  Turns a range of messages into one message carrying a number: hearing
  BaseMessage+3 sends BaseMessage with a parameter of 3.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Send_Custom_Parameter, "ID:int,BaseMessage:int,Limit:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		const int base = Get_Int_Parameter ("BaseMessage");
		if (type >= base && type <= Get_Int_Parameter ("Limit"))
		{
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
					base, type - base, 0.0f);
		}
	}
};


/*
**	Eight objects and eight messages, in one script.  The three scripts below
**	differ only in which of the two lists they walk and how far.
*/
#define JFW_CUSTOM_EIGHT_IDS	"ID1:int,ID2:int,ID3:int,ID4:int,ID5:int,ID6:int,ID7:int,ID8:int"
#define JFW_CUSTOM_EIGHT_MSGS	"Custom1:int,Custom2:int,Custom3:int,Custom4:int," \
								"Custom5:int,Custom6:int,Custom7:int,Custom8:int"

/*JFW_Custom_Send_Ordered_Customs

  Each time it hears its message it sends the next pair from the list, and
  wraps round after the eighth.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Send_Ordered_Customs,
		JFW_CUSTOM_EIGHT_IDS "," JFW_CUSTOM_EIGHT_MSGS ",WaitCustom:int")
{
	int	Current;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Current, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		Current = 0;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("WaitCustom"))
		{
			return;
		}

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter (Current)),
				Get_Int_Parameter (Current + 8), 0, 0.0f);

		Current = (Current + 1) % 8;
	}
};


/*JFW_Custom_Send_Customs

  Sends all eight at once.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Send_Customs,
		JFW_CUSTOM_EIGHT_IDS "," JFW_CUSTOM_EIGHT_MSGS ",WaitCustom:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("WaitCustom"))
		{
			return;
		}

		for (int index = 0; index < 8; index ++)
		{
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter (index)),
					Get_Int_Parameter (index + 8), 0, 0.0f);
		}
	}
};


/*JFW_Custom_Send_Custom_Multiple_IDs

  Sends the same message to all eight.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Send_Custom_Multiple_IDs,
		JFW_CUSTOM_EIGHT_IDS ",WaitCustom:int,SendCustom:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("WaitCustom"))
		{
			return;
		}

		const int message = Get_Int_Parameter ("SendCustom");
		for (int index = 0; index < 8; index ++)
		{
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter (index)), message, 0, 0.0f);
		}
	}
};


/*JFW_Delay_Custom

  Passes a message on after a wait, from whoever sent it rather than from
  here, so the eventual recipient sees the original sender.
*/

DECLARE_SCRIPT_TT (JFW_Delay_Custom, "Message:int,Delay:float,ID:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t param, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Send_Custom_Event (sender,
					ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
					type, (int)param, Get_Float_Parameter ("Delay"));
		}
	}
};


/*JFW_Startup_Custom

  Sends one message when the level starts, then removes itself.
*/

DECLARE_SCRIPT_TT (JFW_Startup_Custom, "Message:int,ID:int")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), 0, 0.0f);
		Destroy_Script();
	}
};


/*JFW_Startup_Custom_Self

  The same, to itself, optionally with a parameter and after a wait.
*/

DECLARE_SCRIPT_TT (JFW_Startup_Custom_Self, "Message:int,Param=0:int,Delay=0:int")
{
	void Created (GameObject * obj) override
	{
		//
		//	Older levels were saved with this script carrying only its
		//	message, so the other two are read only when they are there.
		//
		const bool has_extras = (Get_Parameter_Count() > 1);

		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("Message"),
				has_extras ? Get_Int_Parameter ("Param") : 0,
				has_extras ? (float)Get_Int_Parameter ("Delay") : 0.0f);
		Destroy_Script();
	}
};


/*JFW_Created_Send_Custom

  Sends a message when the object it is on appears, then removes itself.
*/

DECLARE_SCRIPT_TT (JFW_Created_Send_Custom, "ID:int,Message:int")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), 0, 0.0f);
		Destroy_Script();
	}
};


/*JFW_Damaged_Send_Custom

  Sends a message every time the object is hurt.
*/

DECLARE_SCRIPT_TT (JFW_Damaged_Send_Custom, "ID:int,Message:int")
{
	void Damaged (GameObject * obj, GameObject * /*damager*/, float /*amount*/) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), 0, 0.0f);
	}
};


/*JFW_Damaged_Send_Custom_Amount

  Sends one of two messages depending on how hard it was hit.
*/

DECLARE_SCRIPT_TT (JFW_Damaged_Send_Custom_Amount, "ID:int,Message1:int,Message2:int,Damage:float")
{
	void Damaged (GameObject * obj, GameObject * /*damager*/, float amount) override
	{
		const bool light = (amount < Get_Float_Parameter ("Damage"));

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter (light ? "Message1" : "Message2"), 0, 0.0f);
	}
};


/*JFW_Send_Custom_On_Damage

  Sends a message when health and armour together fall past a mark, and
  another when they come back above it.  It only speaks on a crossing, so a
  long fight does not flood the level with messages.
*/

DECLARE_SCRIPT_TT (JFW_Send_Custom_On_Damage,
		"Message1:int,ID1:int,Message2:int,ID2:int,Health:float")
{
	bool	Above;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Above, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		Above = true;
	}

	void Damaged (GameObject * obj, GameObject * /*damager*/, float /*amount*/) override
	{
		const float total = ScriptEngine::Get_Health (obj) + ScriptEngine::Get_Shield_Strength (obj);
		const float mark  = Get_Float_Parameter ("Health");

		if (total < mark && Above)
		{
			Above = false;
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("ID1")),
					Get_Int_Parameter ("Message1"), 0, 0.0f);
		}
		else if (total > mark && !Above)
		{
			Above = true;
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("ID2")),
					Get_Int_Parameter ("Message2"), 0, 0.0f);
		}
	}
};


/*JFW_Enemy_Seen_Send_Custom

  Sends a message naming whoever it just saw.
*/

DECLARE_SCRIPT_TT (JFW_Enemy_Seen_Send_Custom, "ID:int,Message:int")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Enemy_Seen (obj, true);
	}

	void Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), ScriptEngine::Get_ID (enemy), 0.0f);
	}
};


/*JFW_Death_Send_Custom

  Sends a message when the object dies.
*/

DECLARE_SCRIPT_TT (JFW_Death_Send_Custom, "ID:int,Message:int,Param:int")
{
	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), Get_Int_Parameter ("Param"), 0.0f);
		Destroy_Script();
	}
};


/*JFW_Death_Send_Custom_ID

  The same, carrying its own id so the recipient knows which of several died.
*/

DECLARE_SCRIPT_TT (JFW_Death_Send_Custom_ID, "ID:int,Message:int")
{
	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), ScriptEngine::Get_ID (obj), 0.0f);
		Destroy_Script();
	}
};


/*JFW_Death_Send_Custom_Driver_ID

  The same, carrying the id of whoever was driving.  Sits on a vehicle and
  remembers its first occupant until they get out.
*/

DECLARE_SCRIPT_TT (JFW_Death_Send_Custom_Driver_ID, "ID:int,Message:int")
{
	int	Driver_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Driver_ID, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		Driver_ID = 0;
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			if (Driver_ID == 0)
			{
				Driver_ID = ScriptEngine::Get_ID (sender);
			}
		}
		else if (type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			if (Driver_ID == ScriptEngine::Get_ID (sender))
			{
				Driver_ID = 0;
			}
		}
	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), Driver_ID, 0.0f);
		Destroy_Script();
	}
};


/*JFW_Send_Message_Preset

  Tells every object built from a named preset when this one appears and when
  it goes away.
*/

DECLARE_SCRIPT_TT (JFW_Send_Message_Preset, "Preset:string,CreateMessage:int,DestroyMessage:int")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Send_Custom_To_Preset (obj, Get_Parameter ("Preset"),
				Get_Int_Parameter ("CreateMessage"), 0, 0.0f);
	}

	void Destroyed (GameObject * obj) override
	{
		ScriptEngine::Send_Custom_To_Preset (obj, Get_Parameter ("Preset"),
				Get_Int_Parameter ("DestroyMessage"), 0, 0.0f);
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Shouting at everybody
//
////////////////////////////////////////////////////////////////////////////

/*
**	Four scripts that broadcast a message to a side: to everything on the map
**	or only to what is nearby, started by a repeating timer or by a message.
*/
class	JFW_Broadcast_Base : public ScriptImpClass
{
protected:

	//	Whether the broadcast reaches the whole level or only what is close.
	virtual bool	Whole_Level (void) = 0;

	void Broadcast (GameObject * obj)
	{
		const int message		= Get_Int_Parameter ("Message");
		const int player_type	= Get_Int_Parameter ("Player_Type");

		if (Whole_Level())
		{
			ScriptEngine::Send_Custom_All_Objects (message, obj, player_type);
		}
		else
		{
			ScriptEngine::Send_Custom_All_Objects_Area (message,
					ScriptEngine::Get_Position (obj), Get_Float_Parameter ("Distance"),
					obj, player_type);
		}
	}
};


/*
**	Started by a timer that restarts itself.
*/
class	JFW_Broadcast_Timer : public JFW_Broadcast_Base
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id != Get_Int_Parameter ("TimerNum"))
		{
			return;
		}

		Broadcast (obj);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), timer_id);
	}
};


/*
**	Started by hearing a message.
*/
class	JFW_Broadcast_Custom : public JFW_Broadcast_Base
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("ListenMessage"))
		{
			Broadcast (obj);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Send_Custom_Distance_Objects_Timer,
		"Distance:float,Message:int,Time:float,TimerNum:int,Player_Type:int")
class	JFW_Send_Custom_Distance_Objects_Timer : public JFW_Broadcast_Timer
{
	bool Whole_Level (void) override		{ return false; }
};

REGISTER_SCRIPT_TT (JFW_Send_Custom_Distance_Objects_Custom,
		"Distance:float,Message:int,Player_Type:int,ListenMessage:int")
class	JFW_Send_Custom_Distance_Objects_Custom : public JFW_Broadcast_Custom
{
	bool Whole_Level (void) override		{ return false; }
};

REGISTER_SCRIPT_TT (JFW_Send_Custom_All_Objects_Timer,
		"Message:int,Time:float,TimerNum:int,Player_Type:int")
class	JFW_Send_Custom_All_Objects_Timer : public JFW_Broadcast_Timer
{
	bool Whole_Level (void) override		{ return true; }
};

REGISTER_SCRIPT_TT (JFW_Send_Custom_All_Objects_Custom,
		"Message:int,Player_Type:int,ListenMessage:int")
class	JFW_Send_Custom_All_Objects_Custom : public JFW_Broadcast_Custom
{
	bool Whole_Level (void) override		{ return true; }
};


////////////////////////////////////////////////////////////////////////////
//
//	Timers
//
////////////////////////////////////////////////////////////////////////////

/*
**	Three scripts that send a message on a timer.  They differ in how long the
**	wait is and in whether anything can stop it.
*/
class	JFW_Timer_Send_Base : public ScriptImpClass
{
protected:

	//	How long until the next one.  Random for one of the three.
	virtual float	Next_Interval (void)
		{ return Get_Float_Parameter ("Time"); }

	//	Whether it should keep going after this one.
	virtual bool	Keeps_Going (void)
		{ return Get_Int_Parameter ("Repeat") == 1; }

	void Start (GameObject * obj)
	{
		ScriptEngine::Start_Timer (obj, this, Next_Interval(), Get_Int_Parameter ("TimerNum"));
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id != Get_Int_Parameter ("TimerNum"))
		{
			return;
		}

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), Message_Param(), 0.0f);

		if (Keeps_Going())
		{
			Start (obj);
		}
	}

	virtual int	Message_Param (void)
		{ return Get_Int_Parameter ("Param"); }
};


/*JFW_Timer_Custom

  Sends a message every so often, from the moment the level starts.
*/

REGISTER_SCRIPT_TT (JFW_Timer_Custom, "Time:float,TimerNum:int,Repeat:int,ID:int,Message:int,Param:int")
class	JFW_Timer_Custom : public JFW_Timer_Send_Base
{
	void Created (GameObject * obj) override		{ Start (obj); }
};


/*JFW_Timer_Custom_On_Off

  The same, but it does nothing until told to start and stops when told to.
*/

REGISTER_SCRIPT_TT (JFW_Timer_Custom_On_Off,
		"Time:float,TimerNum:int,Repeat:int,ID:int,Message:int,Param:int,"
		"EnableMessage:int,DisableMessage:int")
class	JFW_Timer_Custom_On_Off : public JFW_Timer_Send_Base
{
	bool	Enabled;

public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Enabled, 1);
	}

private:

	void Created (GameObject * /*obj*/) override
	{
		Enabled = false;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("EnableMessage"))
		{
			Enabled = true;
			Start (obj);
		}
		else if (type == Get_Int_Parameter ("DisableMessage"))
		{
			Enabled = false;
		}
	}

	bool Keeps_Going (void) override
	{
		return Enabled && JFW_Timer_Send_Base::Keeps_Going();
	}
};


/*JFW_Random_Custom

  The same, at an interval picked afresh each time.
*/

REGISTER_SCRIPT_TT (JFW_Random_Custom,
		"Message:int,ID:int,TimerNum:int,Time_Min:float,Time_Max:float,Repeat:int")
class	JFW_Random_Custom : public JFW_Timer_Send_Base
{
	void Created (GameObject * obj) override		{ Start (obj); }

	float Next_Interval (void) override
	{
		return ScriptEngine::Get_Random (Get_Float_Parameter ("Time_Min"),
				Get_Float_Parameter ("Time_Max"));
	}

	//	This one has no Param of its own.
	int Message_Param (void) override				{ return 0; }
};


////////////////////////////////////////////////////////////////////////////
//
//	Making and unmaking objects
//
////////////////////////////////////////////////////////////////////////////

/*
**	Six scripts that put an object into the world on one message and take it
**	away again on another.  They differ in whether the new object is hung off
**	a bone of this one, whether it is animated as it appears, whether taking
**	it away plays an animation first, and whether this object dying takes it
**	with it.
**
**	All six saved the id of what they had made as a single byte, so anything
**	with an id above 255 came back from a save unreachable and was never
**	cleaned up.  It is a whole id here.
*/
class	JFW_Create_Destroy_Base : public ScriptImpClass
{
public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Object_ID, 1);
	}

protected:

	int	Object_ID;

	//	Hang the new object off a bone of the object this script is on.
	virtual bool	Attaches_To_Bone (void)		{ return false; }

	//	Play an animation on the new object as it appears.
	virtual bool	Animates_On_Create (void)	{ return false; }

	//	Play an animation before taking it away, and wait for a timer.
	virtual bool	Animates_On_Destroy (void)	{ return false; }

	//	Take it with us when this object dies.
	virtual bool	Dies_With_Us (void)			{ return false; }

	void Created (GameObject * /*obj*/) override
	{
		Object_ID = 0;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("CreateMessage"))
		{
			Create (obj);
		}

		if (type == Get_Int_Parameter ("DestroyMessage"))
		{
			Destroy (obj);
		}
	}

	void Timer_Expired (GameObject * /*obj*/, int timer_id) override
	{
		if (Animates_On_Destroy() && timer_id == Get_Int_Parameter ("TimerNum"))
		{
			Discard();
		}
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if (Dies_With_Us())
		{
			Discard();
		}
	}

private:

	void Create (GameObject * obj)
	{
		if (Object_ID != 0)
		{
			return;
		}

		GameObject * object = ScriptEngine::Create_Object (Get_Parameter ("Preset"),
				Get_Vector3_Parameter ("Location"));
		if (object == nullptr)
		{
			return;
		}

		ScriptEngine::Set_Facing (object, Get_Float_Parameter ("Facing"));
		Object_ID = ScriptEngine::Get_ID (object);

		if (Attaches_To_Bone())
		{
			ScriptEngine::Attach_To_Object_Bone (object, obj, Get_Parameter ("BoneName"));
		}

		if (Animates_On_Create())
		{
			Play (object, Animates_On_Destroy() ? "Create" : "");
		}
	}

	void Destroy (GameObject * obj)
	{
		if (Object_ID == 0)
		{
			return;
		}

		if (!Animates_On_Destroy())
		{
			Discard();
			return;
		}

		//
		//	Let it play itself out first; the timer clears it up.
		//
		Play (ScriptEngine::Find_Object (Object_ID), "Destroy");
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void Discard (void)
	{
		if (Object_ID != 0)
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (Object_ID));
			Object_ID = 0;
		}
	}

	//
	//	The animation parameters, whose names carry a prefix when one script
	//	has two sets of them.
	//
	void Play (GameObject * object, const char * prefix)
	{
		if (object == nullptr)
		{
			return;
		}

		StringClass name;

		name.Format ("%sAnimation", prefix);
		const char * animation = Get_Parameter (name);

		name.Format ("%sSubobject", prefix);
		const char * subobject = Subobject_Or_Whole (Get_Parameter (name));

		name.Format ("%sFirstFrame", prefix);
		float first_frame = Get_Float_Parameter (name);
		if (first_frame == -1.0f)
		{
			//
			//	Carry on from where it is.  The donor read the frame of the
			//	object the script sits on rather than the one it is about to
			//	animate, so a new object always started from its host.
			//
			first_frame = ScriptEngine::Get_Animation_Frame (object);
		}

		name.Format ("%sLastFrame", prefix);
		const float last_frame = Get_Float_Parameter (name);

		name.Format ("%sBlended", prefix);
		const bool blended = Get_Bool_Parameter (name);

		//	Only the create animation may loop; a destroy animation must end.
		bool looping = false;
		if (::_stricmp (prefix, "Destroy") != 0)
		{
			name.Format ("%sLoop", prefix);
			looping = (Get_Parameter_Index (name) >= 0) && Get_Bool_Parameter (name);
		}

		ScriptEngine::Set_Animation (object, animation, looping, subobject,
				first_frame, last_frame, blended);
	}
};


#define JFW_CREATE_DESTROY_PARAMS \
	"Location:vector3,Facing:float,Preset:string,CreateMessage:int,DestroyMessage:int"

#define JFW_CREATE_DESTROY_ANIM \
	",Animation:string,Loop:int,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int"


REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Custom, JFW_CREATE_DESTROY_PARAMS)
class	JFW_Create_Destroy_Object_On_Custom : public JFW_Create_Destroy_Base
{
};

REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Custom_Bone,
		JFW_CREATE_DESTROY_PARAMS ",BoneName:string")
class	JFW_Create_Destroy_Object_On_Custom_Bone : public JFW_Create_Destroy_Base
{
	bool Attaches_To_Bone (void) override		{ return true; }
};

REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Custom_Death, JFW_CREATE_DESTROY_PARAMS)
class	JFW_Create_Destroy_Object_On_Custom_Death : public JFW_Create_Destroy_Base
{
	bool Dies_With_Us (void) override			{ return true; }
};

REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Custom_Death_Bone,
		JFW_CREATE_DESTROY_PARAMS ",BoneName:string")
class	JFW_Create_Destroy_Object_On_Custom_Death_Bone : public JFW_Create_Destroy_Base
{
	bool Attaches_To_Bone (void) override		{ return true; }
	bool Dies_With_Us (void) override			{ return true; }
};

REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Custom_Animate,
		JFW_CREATE_DESTROY_PARAMS JFW_CREATE_DESTROY_ANIM)
class	JFW_Create_Destroy_Object_On_Custom_Animate : public JFW_Create_Destroy_Base
{
	bool Animates_On_Create (void) override		{ return true; }
};

REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Custom_Animate_2,
		JFW_CREATE_DESTROY_PARAMS
		",CreateAnimation:string,CreateLoop:int,CreateSubobject:string,CreateFirstFrame:float,"
		"CreateLastFrame:float,CreateBlended:int,DestroyAnimation:string,DestroySubobject:string,"
		"DestroyFirstFrame:float,DestroyLastFrame:float,DestroyBlended:int,Time:float,TimerNum:int")
class	JFW_Create_Destroy_Object_On_Custom_Animate_2 : public JFW_Create_Destroy_Base
{
	bool Animates_On_Create (void) override		{ return true; }
	bool Animates_On_Destroy (void) override	{ return true; }
};


/*JFW_Create_Object_Custom

  Puts an object into the world and forgets about it.
*/

DECLARE_SCRIPT_TT (JFW_Create_Object_Custom, "Object:string,Location:vector3,Facing:float,Message:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		GameObject * object = ScriptEngine::Create_Object (Get_Parameter ("Object"),
				Get_Vector3_Parameter ("Location"));
		if (object != nullptr)
		{
			ScriptEngine::Set_Facing (object, Get_Float_Parameter ("Facing"));
		}
	}
};


/*JFW_Destroy_Create_Object_On_Custom

  Replaces one object with another in the same place.
*/

DECLARE_SCRIPT_TT (JFW_Destroy_Create_Object_On_Custom, "Message:int,ID:int,Preset:string")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		GameObject * object = ScriptEngine::Find_Object (Get_Int_Parameter ("ID"));
		if (object == nullptr)
		{
			return;
		}

		const Vector3 position = ScriptEngine::Get_Position (object);
		ScriptEngine::Destroy_Object (object);
		ScriptEngine::Create_Object (Get_Parameter ("Preset"), position);
	}
};


/*JFW_Custom_Destroy_Object

  Removes a named object.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Destroy_Object, "Message:int,ID:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")));
		}
	}
};


/*JFW_Custom_Destroy_Building

  Blows a named building up rather than deleting it, so it leaves a ruin and
  everything watching it hears about it.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Destroy_Building, "Message:int,ID:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Apply_Damage (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
					10000.0f, "Explosive", nullptr);
		}
	}
};


/*JFW_Custom_Damage_Object

  Takes a fixed amount of health off a named object, and finishes it off if
  that was all it had.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Damage_Object, "ID:int,Message:int,Damage:float")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		GameObject * object = ScriptEngine::Find_Object (Get_Int_Parameter ("ID"));
		if (object == nullptr)
		{
			return;
		}

		float health = ScriptEngine::Get_Health (object);
		health -= Get_Float_Parameter ("Damage");
		if (health < 0.0f)
		{
			health = 0.0f;
		}

		ScriptEngine::Set_Health (object, health);
		if (health == 0.0f)
		{
			ScriptEngine::Destroy_Object (object);
		}
	}
};


/*JFW_Custom_Destroy_All_Buildings

  Flattens one side.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Destroy_All_Buildings, "Player_Type:int")
{
	void Custom (GameObject * /*obj*/, int /*type*/, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		ScriptEngine::Kill_All_Buildings_By_Team (Get_Int_Parameter ("Player_Type"));
	}
};


/*JFW_Kill_All_Buildings_Customs

  The same, once it has been told to enough times.
*/

DECLARE_SCRIPT_TT (JFW_Kill_All_Buildings_Customs, "Message:int,Count:int,Player_Type:int")
{
	int	Count;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Count, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		Count = 0;
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		//
		//	The count was tested outside the message check, so once it was
		//	reached every unrelated message flattened the base again.
		//
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		Count ++;
		if (Count >= Get_Int_Parameter ("Count"))
		{
			ScriptEngine::Kill_All_Buildings_By_Team (Get_Int_Parameter ("Player_Type"));
		}
	}
};


/*JFW_Enable_Spawner_On_Custom

  Switches a spawner on or off.
*/

DECLARE_SCRIPT_TT (JFW_Enable_Spawner_On_Custom, "ID:int,Enable:int,Message:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Enable_Spawner (Get_Int_Parameter ("ID"), Get_Bool_Parameter ("Enable"));
		}
	}
};


/*JFW_Create_Vehicle_On_Custom

  Has a side build a vehicle at its own vehicle factory, the way a player
  buying one does.
*/

DECLARE_SCRIPT_TT (JFW_Create_Vehicle_On_Custom, "Preset_Name:string,Delay:float,Player_Type:int")
{
	void Custom (GameObject * /*obj*/, int /*type*/, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		ScriptEngine::Create_Vehicle (Get_Parameter ("Preset_Name"),
				Get_Float_Parameter ("Delay"), nullptr, Get_Int_Parameter ("Player_Type"));
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Noise and explosions
//
////////////////////////////////////////////////////////////////////////////

/*
**	Three scripts that set off an explosion, differing only in where.
*/
class	JFW_Explosion_Base : public ScriptImpClass
{
protected:

	virtual Vector3	Where (GameObject * obj, GameObject * sender) = 0;

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_Explosion (Get_Parameter ("Explosion"),
					Where (obj, sender), nullptr);
		}
	}
};

//	At whoever sent the message.
REGISTER_SCRIPT_TT (JFW_Blow_Up_On_Custom, "Explosion:string,Message:int")
class	JFW_Blow_Up_On_Custom : public JFW_Explosion_Base
{
	Vector3 Where (GameObject * /*obj*/, GameObject * sender) override
		{ return ScriptEngine::Get_Position (sender); }
};

//	At the object the script is on.
REGISTER_SCRIPT_TT_NAMED (JFW_Blow_Up_Obj_On_Custom, "JFW_Blow_Obj_Up_On_Custom",
		"Explosion:string,Message:int")
class	JFW_Blow_Up_Obj_On_Custom : public JFW_Explosion_Base
{
	Vector3 Where (GameObject * obj, GameObject * /*sender*/) override
		{ return ScriptEngine::Get_Position (obj); }
};

//	At a point the level names.
REGISTER_SCRIPT_TT_NAMED (JFW_Blow_Up_Location_On_Custom, "JFW_Blow_Location_Up_On_Custom",
		"Explosion:string,Message:int,Location:vector3")
class	JFW_Blow_Up_Location_On_Custom : public JFW_Explosion_Base
{
	Vector3 Where (GameObject * /*obj*/, GameObject * /*sender*/) override
		{ return Get_Vector3_Parameter ("Location"); }
};


/*JFW_2D_Sound_Custom

  A sound in the listener's head rather than out in the world.
*/

DECLARE_SCRIPT_TT (JFW_2D_Sound_Custom, "Message:int,Preset:string")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_2D_Sound (Get_Parameter ("Preset"));
		}
	}
};


/*JFW_2D_Sound_Team_Custom

  The same, for one side only.
*/

DECLARE_SCRIPT_TT (JFW_2D_Sound_Team_Custom, "Message:int,Sound:string,Team:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_2D_Sound_Team (Get_Int_Parameter ("Team"), Get_Parameter ("Sound"));
		}
	}
};


/*JFW_3D_Sound_Custom

  A sound at a point the level names.
*/

DECLARE_SCRIPT_TT (JFW_3D_Sound_Custom, "Message:int,Preset:string,Position:vector3")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_Sound (Get_Parameter ("Preset"),
					Get_Vector3_Parameter ("Position"), obj);
		}
	}
};


/*JFW_3D_Sound_Team_Custom

  The same, for one side only.
*/

DECLARE_SCRIPT_TT (JFW_3D_Sound_Team_Custom, "Message:int,Sound:string,Position:vector3,Team:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_Sound_Team (Get_Int_Parameter ("Team"), Get_Parameter ("Sound"),
					Get_Vector3_Parameter ("Position"));
		}
	}
};


/*JFW_3D_Sound_Custom_2

  A sound wherever the message came from.
*/

DECLARE_SCRIPT_TT (JFW_3D_Sound_Custom_2, "Message:int,Preset:string")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_Sound (Get_Parameter ("Preset"),
					ScriptEngine::Get_Position (sender), obj);
		}
	}
};


/*JFW_3D_Sound_Custom_Bone

  A sound at a bone of this object, which follows it if it moves.
*/

DECLARE_SCRIPT_TT (JFW_3D_Sound_Custom_Bone, "Message:int,Preset:string,Bone:string")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Create_3D_Sound_At_Bone (Get_Parameter ("Preset"), obj,
					Get_Parameter ("Bone"));
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Animation and appearance
//
////////////////////////////////////////////////////////////////////////////

/*
**	Three scripts that start an animation on a message, one of them changing
**	the model first.
*/
class	JFW_Animate_On_Custom_Base : public ScriptImpClass
{
protected:

	virtual bool	Changes_Model (void)	{ return false; }
	virtual bool	Has_Animation (void)	{ return true; }

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		if (Changes_Model())
		{
			ScriptEngine::Set_Model (obj, Get_Parameter ("Model"));
		}

		if (!Has_Animation())
		{
			return;
		}

		float first_frame = Get_Float_Parameter ("FirstFrame");
		if (first_frame == -1.0f)
		{
			first_frame = ScriptEngine::Get_Animation_Frame (obj);
		}

		ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"),
				Get_Bool_Parameter ("Loop"), Subobject_Or_Whole (Get_Parameter ("Subobject")),
				first_frame, Get_Float_Parameter ("LastFrame"), Get_Bool_Parameter ("Blended"));
	}
};

#define JFW_ANIMATE_PARAMS \
	"Animation:string,Loop:int,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int"

REGISTER_SCRIPT_TT (JFW_Set_Animation_On_Custom, JFW_ANIMATE_PARAMS ",Message:int")
class	JFW_Set_Animation_On_Custom : public JFW_Animate_On_Custom_Base
{
};

REGISTER_SCRIPT_TT (JFW_Set_Model_Animation_On_Custom,
		JFW_ANIMATE_PARAMS ",Model:string,Message:int")
class	JFW_Set_Model_Animation_On_Custom : public JFW_Animate_On_Custom_Base
{
	bool Changes_Model (void) override		{ return true; }
};

REGISTER_SCRIPT_TT (JFW_Set_Model_On_Custom, "Model:string,Message:int")
class	JFW_Set_Model_On_Custom : public JFW_Animate_On_Custom_Base
{
	bool Changes_Model (void) override		{ return true; }
	bool Has_Animation (void) override		{ return false; }
};


/*JFW_Set_Model_On_Custom_Sender

  Changes the model of whoever sent the message rather than its own.
*/

DECLARE_SCRIPT_TT (JFW_Set_Model_On_Custom_Sender, "Message:int,Model:string")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Model (sender, Get_Parameter ("Model"));
		}
	}
};


/*JFW_Enable_Disable_Animation_On_Custom

  Runs an animation on a loop between two messages, and stops it on the
  second.  It ignores a repeat of either, so a message arriving twice does
  not restart what is already running.
*/

DECLARE_SCRIPT_TT (JFW_Enable_Disable_Animation_On_Custom,
		"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,"
		"Enable:int,Disable:int")
{
	bool	Running;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Running, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		Running = false;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Enable") && !Running)
		{
			Running = true;

			float first_frame = Get_Float_Parameter ("FirstFrame");
			if (first_frame == -1.0f)
			{
				first_frame = ScriptEngine::Get_Animation_Frame (obj);
			}

			ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), true,
					Subobject_Or_Whole (Get_Parameter ("Subobject")),
					first_frame, -1.0f, Get_Bool_Parameter ("Blended"));
		}
		else if (type == Get_Int_Parameter ("Disable") && Running)
		{
			Clear_Animation (obj);
			Running = false;
		}
	}
};


/*JFW_Custom_Toggle_Visible

  Shows or hides the object.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Toggle_Visible, "Message:int,Visible:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Is_Rendered (obj, Get_Bool_Parameter ("Visible"));
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Health, armour and being unkillable
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Repair_On_Custom

  Puts health and armour back.
*/

DECLARE_SCRIPT_TT (JFW_Repair_On_Custom, "Message:int,Health:float")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		//
		//	Nothing is repaired from dead.  The donor set armour from the
		//	health reading rather than from the armour, so repairing an
		//	object also copied its health across to its shield.
		//
		if (ScriptEngine::Get_Health (obj) <= 0.0f)
		{
			return;
		}

		const float amount = Get_Float_Parameter ("Health");
		ScriptEngine::Set_Health (obj, ScriptEngine::Get_Health (obj) + amount);
		ScriptEngine::Set_Shield_Strength (obj, ScriptEngine::Get_Shield_Strength (obj) + amount);
	}
};


/*JFW_Invulnerable_On_Custom

  Holds the object at full health for as long as it is switched on, and gives
  back what it had when it is switched off again.
*/

DECLARE_SCRIPT_TT (JFW_Invulnerable_On_Custom, "Message:int")
{
	bool	Enabled;
	float	Health;
	float	Shield;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled,	1);
		SAVE_VARIABLE (Health,	2);
		SAVE_VARIABLE (Shield,	3);
	}

	void Created (GameObject * /*obj*/) override
	{
		Enabled	= false;
		Health	= 0.0f;
		Shield	= 0.0f;
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		if (param == 1)
		{
			//
			//	Switching it on twice would otherwise record full health as
			//	what it had before, so switching it off left it healed.
			//
			if (!Enabled)
			{
				Enabled	= true;
				Health	= ScriptEngine::Get_Health (obj);
				Shield	= ScriptEngine::Get_Shield_Strength (obj);
			}

			Top_Up (obj);
		}
		else if (Enabled)
		{
			Enabled = false;
			ScriptEngine::Set_Health (obj, Health);
			ScriptEngine::Set_Shield_Strength (obj, Shield);
		}
	}

	void Damaged (GameObject * obj, GameObject * /*damager*/, float /*amount*/) override
	{
		if (Enabled)
		{
			Top_Up (obj);
		}
	}

	void Top_Up (GameObject * obj)
	{
		ScriptEngine::Set_Health (obj, ScriptEngine::Get_Max_Health (obj));
		ScriptEngine::Set_Shield_Strength (obj, ScriptEngine::Get_Max_Shield_Strength (obj));
	}
};


/*JFW_Invulnerable_On_Custom_2

  The same idea done properly: swap to an armour nothing can hurt, and back.
*/

DECLARE_SCRIPT_TT (JFW_Invulnerable_On_Custom_2,
		"Message:int,InvulnerableArmour:string,NormalArmour:string")
{
	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Shield_Type (obj,
					Get_Parameter ((param == 1) ? "InvulnerableArmour" : "NormalArmour"));
		}
	}
};


/*JFW_Invulnerable_On_Custom_3

  The same again, swapping the skin rather than the armour.
*/

DECLARE_SCRIPT_TT (JFW_Invulnerable_On_Custom_3,
		"Message:int,InvulnerableArmour:string,NormalArmour:string")
{
	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Skin (obj,
					Get_Parameter ((param == 1) ? "InvulnerableArmour" : "NormalArmour"));
		}
	}
};


/*JFW_Set_Skin_Custom

  Changes what the object is made of.
*/

DECLARE_SCRIPT_TT (JFW_Set_Skin_Custom, "Message:int,Armour:string")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Skin (obj, Get_Parameter ("Armour"));
		}
	}
};


/*JFW_Set_Armor_Custom

  Changes what it is wearing.
*/

DECLARE_SCRIPT_TT (JFW_Set_Armor_Custom, "Message:int,Armour:string")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Shield_Type (obj, Get_Parameter ("Armour"));
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Teleporting
//
////////////////////////////////////////////////////////////////////////////

/*
**	Three scripts that move whoever the message names.  The message carries
**	the id of what to move, so a zone can send it on behalf of whoever walked
**	in.  They differ in whether the destination is fixed, picked at random
**	from five, or fixed but only usable once every so often.
**
**	A destination is either a point the level names or another object to stand
**	on top of; naming an object wins.
*/
class	JFW_Teleport_Base : public ScriptImpClass
{
protected:

	//	Which of the five sets of destination parameters to read; empty for
	//	the scripts that have only one.
	virtual const char *	Destination_Suffix (void)	{ return ""; }

	//	Whether this one may fire right now.
	virtual bool	Ready (void)						{ return true; }
	virtual void	Fired (GameObject * /*obj*/)		{ }

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message") || !Ready())
		{
			return;
		}

		GameObject * subject = ScriptEngine::Find_Object ((int)param);
		if (subject == nullptr)
		{
			return;
		}

		StringClass name;
		const char * suffix = Destination_Suffix();

		name.Format ("Object_ID%s", suffix);
		GameObject * marker = ScriptEngine::Find_Object (Get_Int_Parameter (name));
		if (marker != nullptr)
		{
			ScriptEngine::Set_Position (subject, ScriptEngine::Get_Position (marker));
		}
		else
		{
			name.Format ("Location%s", suffix);
			ScriptEngine::Set_Position (subject, Get_Vector3_Parameter (name));
		}

		Fired (obj);
	}
};


REGISTER_SCRIPT_TT (JFW_Teleport_Custom, "Location:vector3,Object_ID=0:int,Message:int")
class	JFW_Teleport_Custom : public JFW_Teleport_Base
{
};


REGISTER_SCRIPT_TT (JFW_Teleport_Custom_Random,
		"Location1:vector3,Object_ID1=0:int,Location2:vector3,Object_ID2=0:int,"
		"Location3:vector3,Object_ID3=0:int,Location4:vector3,Object_ID4=0:int,"
		"Location5:vector3,Object_ID5=0:int,Message:int")
class	JFW_Teleport_Custom_Random : public JFW_Teleport_Base
{
	const char * Destination_Suffix (void) override
	{
		//
		//	Picked here rather than stored, because the choice is wanted once
		//	per message and the base asks twice.
		//
		static const char * const	_suffixes[5] = { "1", "2", "3", "4", "5" };
		return _suffixes[ScriptEngine::Get_Random_Int (0, 4)];
	}
};


REGISTER_SCRIPT_TT (JFW_Teleport_Custom_Timer,
		"Location:vector3,Object_ID=0:int,Message:int,Time:float,TimerNum:int")
class	JFW_Teleport_Custom_Timer : public JFW_Teleport_Base
{
	bool	Available;

public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Available, 1);
	}

private:

	void Created (GameObject * /*obj*/) override		{ Available = true; }

	bool Ready (void) override							{ return Available; }

	void Fired (GameObject * obj) override
	{
		Available = false;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void Timer_Expired (GameObject * /*obj*/, int timer_id) override
	{
		if (timer_id == Get_Int_Parameter ("TimerNum"))
		{
			Available = true;
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Weather
//
////////////////////////////////////////////////////////////////////////////

/*
**	Six scripts that change the sky on a message, each optionally after a
**	wait.  They differ only in what they change.
*/
class	JFW_Weather_Base : public ScriptImpClass
{
protected:

	enum { WEATHER_TIMER = 1 };

	virtual void	Apply (void) = 0;

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Message"))
		{
			return;
		}

		const float delay = Get_Float_Parameter ("Delay");
		if (delay > 0.0f)
		{
			ScriptEngine::Start_Timer (obj, this, delay, WEATHER_TIMER);
		}
		else
		{
			Apply();
		}
	}

	void Timer_Expired (GameObject * /*obj*/, int timer_id) override
	{
		if (timer_id == WEATHER_TIMER)
		{
			Apply();
		}
	}
};


REGISTER_SCRIPT_TT (JFW_Fog_Custom,
		"Fog_Enable:int,Fog_Start_Distance=0.000:float,Fog_End_Distance=0.000:float,"
		"Delay=0.000:float,Message:int")
class	JFW_Fog_Custom : public JFW_Weather_Base
{
	void Apply (void) override
	{
		const bool enable = Get_Bool_Parameter ("Fog_Enable");
		ScriptEngine::Set_Fog_Enable (enable);

		if (enable)
		{
			ScriptEngine::Set_Fog_Range (Get_Float_Parameter ("Fog_Start_Distance"),
					Get_Float_Parameter ("Fog_End_Distance"), 1.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_War_Blitz_Custom,
		"War_Blitz_Intensity=0.000:float,Start_Distance=0.000:float,End_Distance=1.000:float,"
		"War_Blitz_Heading=0.000:float,War_Blitz_Distribution=1.000:float,Delay=0.000:float,"
		"Message:int")
class	JFW_War_Blitz_Custom : public JFW_Weather_Base
{
	void Apply (void) override
	{
		ScriptEngine::Set_War_Blitz (Get_Float_Parameter ("War_Blitz_Intensity"),
				Get_Float_Parameter ("Start_Distance"), Get_Float_Parameter ("End_Distance"),
				Get_Float_Parameter ("War_Blitz_Heading"),
				Get_Float_Parameter ("War_Blitz_Distribution"), 1.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Lightning_Custom,
		"Lightning_Intensity=0.000:float,Start_Distance=0.000:float,End_Distance=1.000:float,"
		"Lightning_Heading=0.000:float,Lightning_Distribution=1.000:float,Delay=0.000:float,"
		"Message:int")
class	JFW_Lightning_Custom : public JFW_Weather_Base
{
	void Apply (void) override
	{
		ScriptEngine::Set_Lightning (Get_Float_Parameter ("Lightning_Intensity"),
				Get_Float_Parameter ("Start_Distance"), Get_Float_Parameter ("End_Distance"),
				Get_Float_Parameter ("Lightning_Heading"),
				Get_Float_Parameter ("Lightning_Distribution"), 1.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Wind_Custom,
		"Wind_Heading=0.000:float,Wind_Speed=0.000:float,Wind_Variability=0.000:float,"
		"Delay=0.000:float,Message:int")
class	JFW_Wind_Custom : public JFW_Weather_Base
{
	void Apply (void) override
	{
		ScriptEngine::Set_Wind (Get_Float_Parameter ("Wind_Heading"),
				Get_Float_Parameter ("Wind_Speed"), Get_Float_Parameter ("Wind_Variability"), 1.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Clouds_Custom,
		"Cloud_Cover=0.000:float,Cloud_Gloominess=0.000:float,Delay=0.000:float,Message:int")
class	JFW_Clouds_Custom : public JFW_Weather_Base
{
	void Apply (void) override
	{
		ScriptEngine::Set_Clouds (Get_Float_Parameter ("Cloud_Cover"),
				Get_Float_Parameter ("Cloud_Gloominess"), 1.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Precipitation_Custom,
		"Type=Rain:string,Density=0.000:float,Delay=0.000:float,Reset_Precipitation=0:int,"
		"Message:int")
class	JFW_Precipitation_Custom : public JFW_Weather_Base
{
	void Apply (void) override
	{
		if (Get_Int_Parameter ("Reset_Precipitation") > 0)
		{
			Clear();
		}

		const char *	type		= Get_Parameter ("Type");
		const float		density	= Get_Float_Parameter ("Density");

		if (::_stricmp (type, "Snow") == 0)
		{
			ScriptEngine::Set_Snow (density, density, true);
		}
		else if (::_stricmp (type, "Ash") == 0)
		{
			ScriptEngine::Set_Ash (density, density, true);
		}
		else if (::_stricmp (type, "Rain") == 0)
		{
			ScriptEngine::Set_Rain (density, density, true);
		}
		else
		{
			//	A type nobody recognises means clear weather.
			Clear();
		}
	}

	void Clear (void)
	{
		ScriptEngine::Set_Snow (0.0f, 0.0f, true);
		ScriptEngine::Set_Ash (0.0f, 0.0f, true);
		ScriptEngine::Set_Rain (0.0f, 0.0f, true);
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Everything else a message can do
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Custom_Toggle_Radar

  Turns the radar on or off; the message carries which.
*/

DECLARE_SCRIPT_TT (JFW_Custom_Toggle_Radar, "Message:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Enable_Radar (param != 0);
		}
	}
};


/*JFW_Enable_Team_Radar_On_Custom

  The same for one side.
*/

DECLARE_SCRIPT_TT (JFW_Enable_Team_Radar_On_Custom, "Player_Type:int,Message:int,Enable:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Enable_Radar_Team (Get_Int_Parameter ("Player_Type"),
					Get_Bool_Parameter ("Enable"));
		}
	}
};


/*JFW_Enable_Stealth_On_Custom

  Turns the stealth of this object on or off.
*/

DECLARE_SCRIPT_TT (JFW_Enable_Stealth_On_Custom, "Message:int,Enable:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Enable_Stealth (obj, Get_Bool_Parameter ("Enable"));
		}
	}
};


/*JFW_Change_Team_On_Custom

  Swaps the object between the two sides.
*/

DECLARE_SCRIPT_TT (JFW_Change_Team_On_Custom, "Message:int")
{
	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Set_Player_Type (obj,
					(ScriptEngine::Get_Player_Type (obj) == PLAYERTYPE_GDI)
							? PLAYERTYPE_NOD : PLAYERTYPE_GDI);
		}
	}
};


/*JFW_Console_Message_On_Custom

  Writes a line to the server console.
*/

DECLARE_SCRIPT_TT (JFW_Console_Message_On_Custom, "Message:int,Output:string")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			//
			//	The text is the level author's, not a format string; passing
			//	it as one would let a stray per cent sign read the stack.
			//
			ScriptEngine::Console_Output ("%s", Get_Parameter ("Output"));
		}
	}
};


/*JFW_Console_Input_On_Custom

  Runs a console command, as though it had been typed on the server.
*/

DECLARE_SCRIPT_TT (JFW_Console_Input_On_Custom, "Message:int,Input:string")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Message"))
		{
			ScriptEngine::Console_Input (Get_Parameter ("Input"));
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Keys
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Send_Self_Custom_On_Key

  Sends the object a message when its occupant presses a key, optionally only
  the first time.
*/

REGISTER_SCRIPT_TT (JFW_Send_Self_Custom_On_Key, "Key=Key:string,Message=0:int,Once=1:int")
class	JFW_Send_Self_Custom_On_Key : public KeyHookScriptClass
{
	void Created (GameObject * obj) override
	{
		Install_Hook (Get_Parameter ("Key"), obj);
	}

	void Key_Hook (void) override
	{
		GameObject * obj = Owner();
		if (obj == nullptr)
		{
			return;
		}

		ScriptEngine::Send_Custom_Event (obj, obj, Get_Int_Parameter ("Message"), 0, 0.0f);

		if (Get_Bool_Parameter ("Once"))
		{
			Destroy_Script();
		}
	}
};


/*JFW_Send_Message_On_Key

  Writes a line of text on the screen of whoever pressed the key.
*/

REGISTER_SCRIPT_TT (JFW_Send_Message_On_Key,
		"Key:string,Message:string,Red:int,Green:int,Blue:int")
class	JFW_Send_Message_On_Key : public KeyHookScriptClass
{
	void Created (GameObject * obj) override
	{
		Install_Hook (Get_Parameter ("Key"), obj);
	}

	void Key_Hook (void) override
	{
		GameObject * obj = Owner();
		if (obj != nullptr)
		{
			ScriptEngine::Send_Message_Player (obj, Get_Int_Parameter ("Red"),
					Get_Int_Parameter ("Green"), Get_Int_Parameter ("Blue"),
					Get_Parameter ("Message"));
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Markers with no behaviour
//
////////////////////////////////////////////////////////////////////////////

/*
**	These six do nothing, and did nothing in the 4.8.4 library either: their
**	classes are empty there.  They exist so that a level author can hang two
**	numbers on an object where a shader system will find them.  This tree has
**	no such system and is not getting one, so they stay what they always were
**	-- names a level may carry without the level failing to load.
*/
#define JFW_SHADER_MARKER(x, params)	\
	REGISTER_SCRIPT_TT (x, params)		\
	class x : public ScriptImpClass		\
	{												\
	};

JFW_SHADER_MARKER (JFW_Shader_Custom_Zone_Entry,	"Player_Type:int,Number:int,Number2:float")
JFW_SHADER_MARKER (JFW_Shader_Custom_Zone_Exit,		"Player_Type:int,Number:int,Number2:float")
JFW_SHADER_MARKER (JFW_Shader_Custom_Vehicle_Entry,	"Number:int,Number2:float")
JFW_SHADER_MARKER (JFW_Shader_Custom_Vehicle_Exit,	"Number:int,Number2:float")
JFW_SHADER_MARKER (JFW_Shader_Custom_Create,			"Number:int,Number2:float")
JFW_SHADER_MARKER (JFW_Shader_Custom_Poke,			"Number:int,Number2:float")


/******************************************************************************
*
*	The one name that is not here
*
*	JFW_Custom_Objectives_Dlg opens the objectives dialog on the screen of
*	whoever sent it a message, showing a named objectives file.  4.8.4 did that
*	by calling a function it resolved out of its own engine DLL by name; there
*	is no source for it here and no equivalent in this tree, because the
*	objectives viewer is client side and nothing can currently ask one client
*	to open it.  It wants the server-to-client dialog request that the HUD and
*	dialog work brings with it, and is recorded against that work rather than
*	guessed at here.
*
******************************************************************************/
