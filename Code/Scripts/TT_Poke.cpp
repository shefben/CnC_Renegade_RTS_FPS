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
*     TT_Poke.cpp
*
* DESCRIPTION
*     Poking: a player walks up to a thing and presses the use key.  Every
*     script here hangs off that one event, and between them they cover the
*     four ways a level uses it -- buy something, open something, say
*     something, or tell some other object that a player was here.
*
*     Native port of the 4.8.4 library's jfwpoke.cpp.  Its fifty-six names
*     come down to seven behaviours the level author configures, so each is
*     written once and the names are sets of answers about it.
*
*     Donor defects fixed here:
*
*     -  Every group purchase PAID its contributors instead of charging them.
*        `Give_Money` adds, and the donor passed the contribution unnegated,
*        so a pot that needed filling filled itself and the poker got richer
*        each time.  It also read a parameter named "Message" that neither
*        variant registers -- the message is called "Custom" -- so every
*        completed purchase sent message 0 rather than the one asked for.
*
*     -  `JFW_Toggle_Door_Team_2` did nothing at all.  It is one of the "_2"
*        scripts, which act on the object they are attached to, but it looked
*        up an "ID" parameter it does not register and animated whatever that
*        found, which was nothing.  `JFW_Switch_Door_Team_2` opened itself and
*        then closed that same nothing.  Both act on themselves here.
*
*     -  Six of the timed doors and both timed sound players ignored the timer
*        number, so any unrelated timer on the same object slammed the door or
*        cleared the cooldown.  Every timer here checks it is its own.
*
*     -  `JFW_Poke_Send_Custom` saved its four-byte activation state as one
*        byte, and `JFW_Escort_Poke` never saved who it was following at all.
*        That escort also never forgot: dismissing it left the dismisser
*        recorded as its charge, so every later poke by the same person was
*        read as another dismissal and it could not be taken on again.  It
*        forgets its charge when it is dismissed here.
*
*     -  Buying a vehicle passed the script's Player_Type parameter to the
*        factory lookup.  That parameter is a filter, not a side: "2" means
*        anyone may poke this and "3" means only the star may, and neither is
*        a side that owns a war factory.  The vehicle is built for the side
*        the buyer is actually on.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "actionparams.h"
#include "physicalgameobj.h"
#include "playertype.h"


/*
**	Opening and closing are the same animation played forwards and then
**	backwards from wherever it had got to.
*/
static void Play_Forward (GameObject * obj, const char * animation)
{
	if (obj != nullptr)
	{
		ScriptEngine::Set_Animation (obj, animation, false, nullptr, 0.0f, -1.0f, false);
	}
}


static void Play_Backward (GameObject * obj, const char * animation)
{
	if (obj != nullptr)
	{
		ScriptEngine::Set_Animation (obj, animation, false, nullptr,
				ScriptEngine::Get_Animation_Frame (obj), 0.0f, false);
	}
}


/*
**	The noise a purchase makes when the poker cannot afford it.  It plays
**	where the poker is standing, so only the person who tried hears it close.
*/
static void Play_Denial (GameObject * poker, const char * sound)
{
	if (poker != nullptr && sound != nullptr && sound[0] != 0)
	{
		ScriptEngine::Create_Sound (sound, ScriptEngine::Get_Position (poker), poker);
	}
}


////////////////////////////////////////////////////////////////////////////
//
//	Buying things by poking them
//
//	Eighteen registered names, and the only things that separate them are
//	what the money buys, whether the shop closes for a while afterwards, and
//	whether it makes a noise at someone who cannot pay.
//
////////////////////////////////////////////////////////////////////////////

enum JFW_Buy_Goods
{
	BUY_CHARACTER,		//	turns the buyer into a different soldier
	BUY_REFILL,			//	tops the buyer's health, armour and ammunition up
	BUY_VEHICLE,		//	built at the buyer's own war factory
	BUY_POWERUP,		//	a pickup handed straight over
	BUY_WEAPON,			//	the same, under a parameter named for weapons
	BUY_PRESET			//	an object created at a fixed spot in the level
};


enum JFW_Buy_Gate
{
	BUY_GATE_NONE,		//	always open
	BUY_GATE_TIMER,	//	closes for a while after each sale
	BUY_GATE_CUSTOM	//	closes after one sale until a message reopens it
};


struct JFW_Buy_Traits
{
	JFW_Buy_Goods	Goods;
	JFW_Buy_Gate	Gate;
	bool				Denial_Sound;
};


class	JFW_Buy_Poke_Base : public ScriptImpClass
{
public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Enabled, 1);
	}

protected:

	virtual void Traits (JFW_Buy_Traits & traits) = 0;

	bool	Enabled;

	void Created (GameObject * /*obj*/) override
	{
		Enabled = true;
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		JFW_Buy_Traits traits;
		Traits (traits);

		if (traits.Gate == BUY_GATE_CUSTOM && type == Get_Int_Parameter ("Message"))
		{
			Enabled = true;
		}
	}

	void Timer_Expired (GameObject * /*obj*/, int number) override
	{
		JFW_Buy_Traits traits;
		Traits (traits);

		if (traits.Gate == BUY_GATE_TIMER && number == Get_Int_Parameter ("TimerNum"))
		{
			Enabled = true;
		}
	}

	void Poked (GameObject * obj, GameObject * poker) override
	{
		JFW_Buy_Traits traits;
		Traits (traits);

		if (!Is_Player_Type (poker, Get_Int_Parameter ("Player_Type")))
		{
			return;
		}

		if (!Enabled)
		{
			return;
		}

		float cost = (float)Get_Int_Parameter ("Cost");

		if (cost > ScriptEngine::Get_Money (poker))
		{
			if (traits.Denial_Sound)
			{
				Play_Denial (poker, Get_Parameter ("Sound"));
			}

			return;
		}

		ScriptEngine::Give_Money (poker, -cost, false);
		Deliver (poker, traits);

		if (traits.Gate != BUY_GATE_NONE)
		{
			Enabled = false;

			if (traits.Gate == BUY_GATE_TIMER)
			{
				ScriptEngine::Start_Timer (obj, this,
						Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
			}
		}
	}

private:

	void Deliver (GameObject * poker, const JFW_Buy_Traits & traits)
	{
		switch (traits.Goods)
		{
			case BUY_CHARACTER:
				ScriptEngine::Change_Character (poker, Get_Parameter ("Preset_Name"));
				break;

			case BUY_REFILL:
				ScriptEngine::Grant_Refill (poker);
				break;

			case BUY_VEHICLE:
				//
				//	The side is the buyer's own, not the script's Player_Type:
				//	that is a filter saying who may poke this, and two of its
				//	values name no side at all.
				//
				ScriptEngine::Create_Vehicle (Get_Parameter ("Preset_Name"), 5.0f,
						poker, ScriptEngine::Get_Player_Type (poker));
				break;

			case BUY_POWERUP:
				ScriptEngine::Give_PowerUp (poker, Get_Parameter ("Preset_Name"), true);
				break;

			case BUY_WEAPON:
				ScriptEngine::Give_PowerUp (poker, Get_Parameter ("Weapon_Name"), true);
				break;

			case BUY_PRESET:
				ScriptEngine::Create_Object (Get_Parameter ("Preset_Name"),
						Get_Vector3_Parameter ("location"));
				break;
		}
	}
};


#define	JFW_BUY_ITEM		"Preset_Name:string,Cost:int,Player_Type:int"
#define	JFW_BUY_NOTHING	"Cost:int,Player_Type:int"
#define	JFW_BUY_WEAPON_P	"Weapon_Name:string,Cost:int,Player_Type:int"
#define	JFW_BUY_SPAWN		"Preset_Name:string,Cost:int,location:vector3,Player_Type:int"
#define	JFW_BUY_TIMED		",Time:float,TimerNum:int"
#define	JFW_BUY_REOPEN		",Message:int"
#define	JFW_BUY_SOUND		",Sound:string"


#define	JFW_BUY_POKE(x, params, goods, gate, sound)	\
	REGISTER_SCRIPT_TT (x, params)						\
	class x : public JFW_Buy_Poke_Base					\
	{																\
		void Traits (JFW_Buy_Traits & traits) override	\
		{															\
			traits.Goods			= goods;					\
			traits.Gate				= gate;					\
			traits.Denial_Sound	= sound;					\
		}															\
	};


/*	The six shops, always open, silent about refusals.
*/
JFW_BUY_POKE (JFW_Character_Buy_Poke,	JFW_BUY_ITEM,		BUY_CHARACTER,	BUY_GATE_NONE, false)
JFW_BUY_POKE (JFW_Refill_Buy_Poke,		JFW_BUY_NOTHING,	BUY_REFILL,		BUY_GATE_NONE, false)
JFW_BUY_POKE (JFW_Vehicle_Buy_Poke,		JFW_BUY_ITEM,		BUY_VEHICLE,	BUY_GATE_NONE, false)
JFW_BUY_POKE (JFW_Powerup_Buy_Poke,		JFW_BUY_ITEM,		BUY_POWERUP,	BUY_GATE_NONE, false)
JFW_BUY_POKE (JFW_Weapon_Buy_Poke,		JFW_BUY_WEAPON_P,	BUY_WEAPON,		BUY_GATE_NONE, false)
JFW_BUY_POKE (JFW_Preset_Buy_Poke,		JFW_BUY_SPAWN,		BUY_PRESET,		BUY_GATE_NONE, false)


/*	The same six, complaining out loud when the buyer is short.
*/
JFW_BUY_POKE (JFW_Character_Buy_Poke_Sound,	JFW_BUY_ITEM		JFW_BUY_SOUND, BUY_CHARACTER,	BUY_GATE_NONE, true)
JFW_BUY_POKE (JFW_Refill_Buy_Poke_Sound,		JFW_BUY_NOTHING	JFW_BUY_SOUND, BUY_REFILL,		BUY_GATE_NONE, true)
JFW_BUY_POKE (JFW_Vehicle_Buy_Poke_Sound,		JFW_BUY_ITEM		JFW_BUY_SOUND, BUY_VEHICLE,	BUY_GATE_NONE, true)
JFW_BUY_POKE (JFW_Powerup_Buy_Poke_Sound,		JFW_BUY_ITEM		JFW_BUY_SOUND, BUY_POWERUP,	BUY_GATE_NONE, true)
JFW_BUY_POKE (JFW_Weapon_Buy_Poke_Sound,		JFW_BUY_WEAPON_P	JFW_BUY_SOUND, BUY_WEAPON,		BUY_GATE_NONE, true)
JFW_BUY_POKE (JFW_Preset_Buy_Poke_Sound,		JFW_BUY_SPAWN		JFW_BUY_SOUND, BUY_PRESET,		BUY_GATE_NONE, true)


/*	Shops that shut for a while after each sale, or until told to reopen.
*/
JFW_BUY_POKE (JFW_Powerup_Buy_Poke_Timer,	JFW_BUY_ITEM	JFW_BUY_TIMED,		BUY_POWERUP,	BUY_GATE_TIMER,	false)
JFW_BUY_POKE (JFW_Preset_Buy_Poke_Timer,	JFW_BUY_SPAWN	JFW_BUY_TIMED,		BUY_PRESET,		BUY_GATE_TIMER,	false)
JFW_BUY_POKE (JFW_Preset_Buy_Poke_Custom,	JFW_BUY_SPAWN	JFW_BUY_REOPEN,	BUY_PRESET,		BUY_GATE_CUSTOM,	false)

JFW_BUY_POKE (JFW_Powerup_Buy_Poke_Timer_Sound,	JFW_BUY_ITEM	JFW_BUY_TIMED	JFW_BUY_SOUND, BUY_POWERUP,	BUY_GATE_TIMER,	true)
JFW_BUY_POKE (JFW_Preset_Buy_Poke_Timer_Sound,	JFW_BUY_SPAWN	JFW_BUY_TIMED	JFW_BUY_SOUND, BUY_PRESET,		BUY_GATE_TIMER,	true)
JFW_BUY_POKE (JFW_Preset_Buy_Poke_Custom_Sound,	JFW_BUY_SPAWN	JFW_BUY_REOPEN	JFW_BUY_SOUND, BUY_PRESET,		BUY_GATE_CUSTOM,	true)


////////////////////////////////////////////////////////////////////////////
//
//	A purchase several people have to club together for
//
////////////////////////////////////////////////////////////////////////////

class	JFW_Group_Purchase_Base : public ScriptImpClass
{
public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Pot,		1);
		SAVE_VARIABLE (Enabled,	2);
	}

protected:

	virtual bool Denial_Sound (void) = 0;

	float	Pot;
	bool	Enabled;

	void Created (GameObject * /*obj*/) override
	{
		Pot		= 0.0f;
		Enabled	= true;
	}

	void Poked (GameObject * obj, GameObject * poker) override
	{
		if (!Enabled)
		{
			return;
		}

		float share = Get_Float_Parameter ("CashPerPerson");

		if (ScriptEngine::Get_Money (poker) < share)
		{
			if (Denial_Sound ())
			{
				Play_Denial (poker, Get_Parameter ("Sound"));
			}

			return;
		}

		//
		//	Charged, not paid.  The donor handed the share over instead of
		//	taking it, so a pot that needed filling filled itself and everyone
		//	who poked walked away richer.
		//
		ScriptEngine::Give_Money (poker, -share, false);
		Pot += share;

		if (Pot >= Get_Float_Parameter ("TotalCash"))
		{
			Pot		= 0.0f;
			Enabled	= false;

			//
			//	The message is registered as "Custom"; the donor asked for one
			//	called "Message" and so sent zero every time.
			//
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
					Get_Int_Parameter ("Custom"), 0, 0.0f);
		}
	}
};


#define	JFW_GROUP_PURCHASE(x, params, sound)	\
	REGISTER_SCRIPT_TT (x, params)				\
	class x : public JFW_Group_Purchase_Base	\
	{														\
		bool Denial_Sound (void) override		\
		{													\
			return sound;								\
		}													\
	};


JFW_GROUP_PURCHASE (JFW_Group_Purchase_Poke,
		"CashPerPerson:float,TotalCash:float,ID:int,Custom:int", false)

JFW_GROUP_PURCHASE (JFW_Group_Purchase_Poke_Sound,
		"CashPerPerson:float,TotalCash:float,ID:int,Custom:int,Sound:string", true)


////////////////////////////////////////////////////////////////////////////
//
//	Doors, gates and switches
//
//	Thirteen names for one thing: play an animation forwards when poked, and
//	play it backwards again either on a timer or on the next poke.  What
//	varies is which object animates, whether a keycard is wanted, whether the
//	poker's side is checked, and which of the two ways it closes.
//
////////////////////////////////////////////////////////////////////////////

enum JFW_Door_Mode
{
	DOOR_TIMED,		//	shuts itself after a while
	DOOR_TOGGLE		//	shuts on the next poke
};


struct JFW_Door_Traits
{
	bool				Targets_Self;
	bool				Needs_Key;
	bool				Team_Gated;
	JFW_Door_Mode	Mode;
};


class	JFW_Door_Poke_Base : public ScriptImpClass
{
public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Open, 1);
	}

protected:

	virtual void Traits (JFW_Door_Traits & traits) = 0;

	bool	Open;

	void Created (GameObject * /*obj*/) override
	{
		Open = false;
	}

	void Poked (GameObject * obj, GameObject * poker) override
	{
		JFW_Door_Traits traits;
		Traits (traits);

		if (traits.Team_Gated && !Is_Player_Type (poker, Get_Int_Parameter ("Player_Type")))
		{
			return;
		}

		if (traits.Needs_Key && !ScriptEngine::Has_Key (poker, Get_Int_Parameter ("Key")))
		{
			return;
		}

		GameObject * door = Door (obj, traits);

		if (door == nullptr)
		{
			return;
		}

		if (traits.Mode == DOOR_TOGGLE)
		{
			if (Open)
			{
				Play_Backward (door, Get_Parameter ("Animation"));
				Open = false;
			}
			else
			{
				Play_Forward (door, Get_Parameter ("Animation"));
				Open = true;
			}

			return;
		}

		Play_Forward (door, Get_Parameter ("Animation"));
		Open = true;

		ScriptEngine::Start_Timer (obj, this,
				Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
	}

	void Timer_Expired (GameObject * obj, int number) override
	{
		JFW_Door_Traits traits;
		Traits (traits);

		//
		//	Six of the donor's timed doors shut on any timer at all, so an
		//	unrelated script sharing the object slammed them.
		//
		if (traits.Mode != DOOR_TIMED || number != Get_Int_Parameter ("TimerNum"))
		{
			return;
		}

		Play_Backward (Door (obj, traits), Get_Parameter ("Animation"));
		Open = false;
	}

private:

	GameObject * Door (GameObject * obj, const JFW_Door_Traits & traits)
	{
		if (traits.Targets_Self)
		{
			return obj;
		}

		return ScriptEngine::Find_Object (Get_Int_Parameter ("ID"));
	}
};


#define	JFW_DOOR_POKE(x, params, self, key, team, mode)	\
	REGISTER_SCRIPT_TT (x, params)								\
	class x : public JFW_Door_Poke_Base							\
	{																		\
		void Traits (JFW_Door_Traits & traits) override		\
		{																	\
			traits.Targets_Self	= self;							\
			traits.Needs_Key		= key;							\
			traits.Team_Gated		= team;							\
			traits.Mode				= mode;							\
		}																	\
	};


/*	Doors somewhere else in the level, named by object id.
*/
JFW_DOOR_POKE (JFW_Switch_Door,			"ID:int,Animation:string,Time:float,TimerNum:int",						false, false, false, DOOR_TIMED)
JFW_DOOR_POKE (JFW_Switch_Lock_Door,	"ID:int,Animation:string,Time:float,TimerNum:int,Key:int",			false, true,  false, DOOR_TIMED)
JFW_DOOR_POKE (JFW_Toggle_Door,			"ID:int,Animation:string",													false, false, false, DOOR_TOGGLE)
JFW_DOOR_POKE (JFW_Toggle_Lock_Door,	"ID:int,Animation:string,Key:int",										false, true,  false, DOOR_TOGGLE)
JFW_DOOR_POKE (JFW_Switch_Door_Team,	"ID:int,Animation:string,Time:float,TimerNum:int,Player_Type:int",	false, false, true,  DOOR_TIMED)
JFW_DOOR_POKE (JFW_Toggle_Door_Team,	"ID:int,Animation:string,Player_Type:int",								false, false, true,  DOOR_TOGGLE)
JFW_DOOR_POKE (JFW_Gate_Poke,				"Player_Type:int,Time:float,TimerNum:int,ID:int,Animation:string",	false, false, true,  DOOR_TIMED)


/*	The same, on the object the script is attached to.
*/
JFW_DOOR_POKE (JFW_Switch_Door_2,		"Animation:string,Time:float,TimerNum:int",					true, false, false, DOOR_TIMED)
JFW_DOOR_POKE (JFW_Switch_Lock_Door_2,	"Animation:string,Time:float,TimerNum:int,Key:int",		true, true,  false, DOOR_TIMED)
JFW_DOOR_POKE (JFW_Toggle_Door_2,		"Animation:string",													true, false, false, DOOR_TOGGLE)
JFW_DOOR_POKE (JFW_Toggle_Lock_Door_2,	"Animation:string,Key:int",										true, true,  false, DOOR_TOGGLE)
JFW_DOOR_POKE (JFW_Switch_Door_Team_2,	"Animation:string,Time:float,TimerNum:int,Player_Type:int",	true, false, true,  DOOR_TIMED)
JFW_DOOR_POKE (JFW_Toggle_Door_Team_2,	"Animation:string,Player_Type:int",								true, false, true,  DOOR_TOGGLE)


////////////////////////////////////////////////////////////////////////////
//
//	Poking something to make a noise
//
////////////////////////////////////////////////////////////////////////////

enum JFW_Poke_Audience
{
	HEARD_BY_EVERYONE,
	HEARD_BY_TEAM,		//	the side named in Player_Type
	HEARD_BY_POKER		//	only the person who poked
};


struct JFW_Poke_Sound_Traits
{
	bool						Positioned;	//	a sound in the world rather than in the ear
	JFW_Poke_Audience		Audience;
	bool						Cooldown;
};


class	JFW_Poke_Sound_Base : public ScriptImpClass
{
public:

	void Register_Auto_Save_Variables (void) override
	{
		SAVE_VARIABLE (Ready, 1);
	}

protected:

	virtual void Traits (JFW_Poke_Sound_Traits & traits) = 0;

	bool	Ready;

	void Created (GameObject * /*obj*/) override
	{
		Ready = true;
	}

	void Timer_Expired (GameObject * /*obj*/, int number) override
	{
		JFW_Poke_Sound_Traits traits;
		Traits (traits);

		//	The donor's two timed sound players cleared their own cooldown on
		//	anyone's timer.
		if (traits.Cooldown && number == Get_Int_Parameter ("TimerNum"))
		{
			Ready = true;
		}
	}

	void Poked (GameObject * obj, GameObject * poker) override
	{
		JFW_Poke_Sound_Traits traits;
		Traits (traits);

		if (!Ready)
		{
			return;
		}

		int team = Get_Int_Parameter ("Player_Type");

		if (!Is_Player_Type (poker, team))
		{
			return;
		}

		const char * sound = Get_Parameter ("Sound");

		if (traits.Positioned)
		{
			Vector3 where = Get_Vector3_Parameter ("Position");

			switch (traits.Audience)
			{
				case HEARD_BY_EVERYONE:	ScriptEngine::Create_Sound (sound, where, obj);			break;
				case HEARD_BY_TEAM:		ScriptEngine::Create_Sound_Team (team, sound, where);	break;
				case HEARD_BY_POKER:		ScriptEngine::Create_Sound_Player (poker, sound, where);	break;
			}
		}
		else
		{
			switch (traits.Audience)
			{
				case HEARD_BY_EVERYONE:	ScriptEngine::Create_2D_Sound (sound);						break;
				case HEARD_BY_TEAM:		ScriptEngine::Create_2D_Sound_Team (team, sound);		break;
				case HEARD_BY_POKER:		ScriptEngine::Create_2D_Sound_Player (poker, sound);	break;
			}
		}

		if (traits.Cooldown)
		{
			Ready = false;
			ScriptEngine::Start_Timer (obj, this,
					Get_Float_Parameter ("Time"), Get_Int_Parameter ("TimerNum"));
		}
	}
};


#define	JFW_POKE_SOUND_2D		"Player_Type:int,Sound:string"
#define	JFW_POKE_SOUND_3D		"Player_Type:int,Sound:string,Position:vector3"
#define	JFW_POKE_SOUND_WAIT	",Time:float,TimerNum:int"


#define	JFW_POKE_SOUND(x, params, positioned, audience, cooldown)	\
	REGISTER_SCRIPT_TT (x, params)												\
	class x : public JFW_Poke_Sound_Base									\
	{																					\
		void Traits (JFW_Poke_Sound_Traits & traits) override			\
		{																				\
			traits.Positioned	= positioned;									\
			traits.Audience	= audience;										\
			traits.Cooldown	= cooldown;										\
		}																				\
	};


JFW_POKE_SOUND (JFW_Poke_Play_2D_Sound,			JFW_POKE_SOUND_2D, false, HEARD_BY_EVERYONE,	false)
JFW_POKE_SOUND (JFW_Poke_Play_3D_Sound,			JFW_POKE_SOUND_3D, true,  HEARD_BY_EVERYONE,	false)
JFW_POKE_SOUND (JFW_Poke_Play_2D_Sound_Team,		JFW_POKE_SOUND_2D, false, HEARD_BY_TEAM,		false)
JFW_POKE_SOUND (JFW_Poke_Play_3D_Sound_Team,		JFW_POKE_SOUND_3D, true,  HEARD_BY_TEAM,		false)
JFW_POKE_SOUND (JFW_Poke_Play_2D_Sound_Player,	JFW_POKE_SOUND_2D, false, HEARD_BY_POKER,		false)
JFW_POKE_SOUND (JFW_Poke_Play_3D_Sound_Player,	JFW_POKE_SOUND_3D, true,  HEARD_BY_POKER,		false)

JFW_POKE_SOUND (JFW_Poke_Play_2D_Sound_Timer,	JFW_POKE_SOUND_2D	JFW_POKE_SOUND_WAIT, false, HEARD_BY_EVERYONE, true)
JFW_POKE_SOUND (JFW_Poke_Play_3D_Sound_Timer,	JFW_POKE_SOUND_3D	JFW_POKE_SOUND_WAIT, true,  HEARD_BY_EVERYONE, true)


/*JFW_Lock_Sound

  Says whether the poker holds a keycard, without doing anything about it.
*/

DECLARE_SCRIPT_TT (JFW_Lock_Sound, "Key:int,LockSound:string,UnlockSound:string")
{
	void Poked (GameObject * obj, GameObject * poker) override
	{
		const char * sound = ScriptEngine::Has_Key (poker, Get_Int_Parameter ("Key"))
				? Get_Parameter ("UnlockSound")
				: Get_Parameter ("LockSound");

		ScriptEngine::Create_Sound (sound, ScriptEngine::Get_Position (obj), obj);
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Poking something to tell something else
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Poke_Send_Custom

  A one-shot switch that can be armed and disarmed from elsewhere.  Poked
  while disarmed it only makes a noise; poked while armed it sends its
  message once and then will not send it again.  The pokable indicator
  follows its state, so a player can see whether it is worth walking over to.
*/

DECLARE_SCRIPT_TT (JFW_Poke_Send_Custom,
		"Sound_Name:string,Activate_Message:int,Object_Id:int,Message:int,Param:int,Delay:float")
{
	int	State;		//	0 disarmed, 1 armed, 2 already fired

	REGISTER_VARIABLES()
	{
		//	The donor saved four bytes of this as one, so a reloaded switch
		//	came back armed or spent at random.
		SAVE_VARIABLE (State, 1);
	}

	void Created (GameObject * obj) override
	{
		//	With no arming message it starts armed.
		State = (Get_Int_Parameter ("Activate_Message") == 0) ? 1 : 0;

		ScriptEngine::Enable_HUD_Pokable_Indicator (obj, true);
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type != Get_Int_Parameter ("Activate_Message"))
		{
			return;
		}

		State = (int)param;

		ScriptEngine::Enable_HUD_Pokable_Indicator (obj, State == 0 || State == 1);
	}

	void Poked (GameObject * obj, GameObject * /*poker*/) override
	{
		if (State == 0)
		{
			ScriptEngine::Create_Sound (Get_Parameter ("Sound_Name"),
					ScriptEngine::Get_Position (obj), obj);
			return;
		}

		if (State != 1)
		{
			return;
		}

		State = 2;

		ScriptEngine::Enable_HUD_Pokable_Indicator (obj, false);

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("Object_Id")),
				Get_Int_Parameter ("Message"), Get_Int_Parameter ("Param"),
				Get_Float_Parameter ("Delay"));
	}
};


/*	The four plain senders differ only in what they ask of the poker besides
**	being on the right side.
*/
enum JFW_Poke_Condition
{
	POKE_NEEDS_NOTHING,
	POKE_NEEDS_PRESET,	//	the poker must be a particular character
	POKE_NEEDS_KEY,		//	the poker must hold a keycard
	POKE_NEEDS_PAYMENT	//	the poker must pay, and does
};


class	JFW_Poke_Send_Base : public ScriptImpClass
{
protected:

	virtual JFW_Poke_Condition Condition (void) = 0;

	void Poked (GameObject * obj, GameObject * poker) override
	{
		if (!Is_Player_Type (poker, Get_Int_Parameter ("Player_Type")))
		{
			return;
		}

		switch (Condition ())
		{
			case POKE_NEEDS_NOTHING:
				break;

			case POKE_NEEDS_PRESET:
				if (::_stricmp (ScriptEngine::Get_Preset_Name (poker), Get_Parameter ("Preset")) != 0)
				{
					return;
				}
				break;

			case POKE_NEEDS_KEY:
				if (!ScriptEngine::Has_Key (poker, Get_Int_Parameter ("Key")))
				{
					return;
				}
				break;

			case POKE_NEEDS_PAYMENT:
			{
				float cost = Get_Float_Parameter ("Cost");

				if (ScriptEngine::Get_Money (poker) < cost)
				{
					return;
				}

				ScriptEngine::Give_Money (poker, -cost, false);
				break;
			}
		}

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), Get_Int_Parameter ("Param"), 0.0f);
	}
};


#define	JFW_POKE_SEND(x, params, condition)		\
	REGISTER_SCRIPT_TT (x, params)					\
	class x : public JFW_Poke_Send_Base				\
	{															\
		JFW_Poke_Condition Condition (void) override	\
		{														\
			return condition;								\
		}														\
	};


#define	JFW_POKE_SEND_P	"Player_Type:int,ID:int,Message:int,Param:int"

JFW_POKE_SEND (JFW_Poke_Send_Custom_2,			JFW_POKE_SEND_P,						POKE_NEEDS_NOTHING)
JFW_POKE_SEND (JFW_Poke_Send_Custom_Preset,	JFW_POKE_SEND_P ",Preset:string",	POKE_NEEDS_PRESET)
JFW_POKE_SEND (JFW_Poke_Send_Custom_Keycard,	JFW_POKE_SEND_P ",Key:int",		POKE_NEEDS_KEY)
JFW_POKE_SEND (JFW_Poke_Send_Custom_Cost,		JFW_POKE_SEND_P ",Cost:float",	POKE_NEEDS_PAYMENT)


/*JFW_Poke_Send_Custom_Toggle

  Alternates between two messages, so one switch can turn a thing on and off
  again.
*/

DECLARE_SCRIPT_TT (JFW_Poke_Send_Custom_Toggle, "Player_Type:int,ID:int,Message1:int,Message2:int")
{
	bool	On;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (On, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		On = false;
	}

	void Poked (GameObject * obj, GameObject * poker) override
	{
		if (!Is_Player_Type (poker, Get_Int_Parameter ("Player_Type")))
		{
			return;
		}

		On = !On;

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter (On ? "Message1" : "Message2"), 0, 0.0f);
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Poking something to be shown something
//
////////////////////////////////////////////////////////////////////////////

class	JFW_Look_At_Poke_Base : public ScriptImpClass
{
protected:

	virtual bool Where (Vector3 & target) = 0;

	void Poked (GameObject * /*obj*/, GameObject * poker) override
	{
		if (!Is_Player_Type (poker, Get_Int_Parameter ("Player_Type")))
		{
			return;
		}

		Vector3 target;

		if (Where (target))
		{
			ScriptEngine::Force_Camera_Look_Player (poker, target);
		}
	}
};


/*JFW_Look_At_Location_Poke

  Turns the poker's head towards a spot.
*/

REGISTER_SCRIPT_TT (JFW_Look_At_Location_Poke, "Player_Type:int,Location:vector3")
class	JFW_Look_At_Location_Poke : public JFW_Look_At_Poke_Base
{
	bool Where (Vector3 & target) override
	{
		target = Get_Vector3_Parameter ("Location");
		return true;
	}
};


/*JFW_Look_At_Object_Poke

  The same, towards whatever is at an object id right now.
*/

REGISTER_SCRIPT_TT (JFW_Look_At_Object_Poke, "Player_Type:int,ObjectID:int")
class	JFW_Look_At_Object_Poke : public JFW_Look_At_Poke_Base
{
	bool Where (Vector3 & target) override
	{
		GameObject * seen = ScriptEngine::Find_Object (Get_Int_Parameter ("ObjectID"));

		if (seen == nullptr)
		{
			return false;
		}

		target = ScriptEngine::Get_Position (seen);
		return true;
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	The rest
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Pokeable_Item

  Marks a thing as worth walking up to and then gets out of the way.  It does
  nothing when poked -- something else on the object handles that.
*/

DECLARE_SCRIPT_TT (JFW_Pokeable_Item, "")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator (obj, true);
		Destroy_Script ();
	}
};


/*JFW_PCT

  A purchase terminal.  Player_Type names the side whose terminal opens; the
  usual "2" means whichever side the poker is on.
*/

DECLARE_SCRIPT_TT (JFW_PCT, "Player_Type:int")
{
	void Poked (GameObject * /*obj*/, GameObject * poker) override
	{
		int team = Get_Int_Parameter ("Player_Type");

		if (!Is_Player_Type (poker, team))
		{
			return;
		}

		if (team != PLAYERTYPE_NOD && team != PLAYERTYPE_GDI)
		{
			team = ScriptEngine::Get_Player_Type (poker);
		}

		if (team == PLAYERTYPE_NOD)
		{
			ScriptEngine::Display_NOD_Player_Terminal_Player (poker);
		}
		else if (team == PLAYERTYPE_GDI)
		{
			ScriptEngine::Display_GDI_Player_Terminal_Player (poker);
		}
	}
};


/*JFW_Sidebar_PT

  A marker, and nothing else.  The 4.8.4 library registered it with no
  behaviour at all -- it was reserved for a sidebar purchase interface that
  the library never shipped.  It is kept so a level carrying it still loads,
  and so the name is not quietly handed to something else later.
*/

REGISTER_SCRIPT_TT (JFW_Sidebar_PT, "Player_Type:int")
class	JFW_Sidebar_PT : public ScriptImpClass
{
};


/*	Two slot machines: pay to poke, and win rather more than you paid one time
**	in however many the level asks for.
*/

class	JFW_Slot_Machine_Base : public ScriptImpClass
{
protected:

	virtual bool Reacts (void) = 0;

	void Poked (GameObject * obj, GameObject * poker) override
	{
		if (!Is_Player_Type (poker, Get_Int_Parameter ("Player_Type")))
		{
			return;
		}

		if (ScriptEngine::Get_Random_Int (0, 100) < Get_Int_Parameter ("Percent"))
		{
			ScriptEngine::Give_Money (poker, (float)Get_Int_Parameter ("Winnings"), false);
		}
		else
		{
			ScriptEngine::Give_Money (poker, -(float)Get_Int_Parameter ("Cost"), false);
		}

		if (Reacts ())
		{
			ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), false);
			ScriptEngine::Create_Sound (Get_Parameter ("Sound"),
					ScriptEngine::Get_Position (obj), obj);
		}
	}
};


#define	JFW_SLOT_MACHINE(x, params, reacts)	\
	REGISTER_SCRIPT_TT (x, params)				\
	class x : public JFW_Slot_Machine_Base		\
	{														\
		bool Reacts (void) override				\
		{													\
			return reacts;								\
		}													\
	};


JFW_SLOT_MACHINE (JFW_Slot_Machine,		"Cost:int,Winnings:int,Percent:int,Player_Type:int",									false)
JFW_SLOT_MACHINE (JFW_Slot_Machine_2,	"Cost:int,Winnings:int,Percent:int,Player_Type:int,Animation:string,Sound:string",	true)


/*JFW_Escort_Poke

  A soldier who follows whoever poked them, and stops when that same person
  pokes them again.  Poking them while they are already following someone
  else hands them over.  Their shield changes to say which state they are in,
  but only for the side the level named.
*/

DECLARE_SCRIPT_TT (JFW_Escort_Poke, "Shield:string,Shield2:string,Player_Type:int")
{
	int	FollowerID;

	REGISTER_VARIABLES()
	{
		//	The donor never saved this, so a reloaded escort had forgotten who
		//	it was following and treated the next poke as a fresh order.
		SAVE_VARIABLE (FollowerID, 1);
	}

	void Created (GameObject * /*obj*/) override
	{
		FollowerID = 0;
	}

	void Poked (GameObject * obj, GameObject * poker) override
	{
		bool	named_side	= (ScriptEngine::Get_Player_Type (poker)
									== Get_Int_Parameter ("Player_Type"));

		if (ScriptEngine::Find_Object (FollowerID) == poker)
		{
			FollowerID = 0;

			ScriptEngine::Action_Reset (obj, 100.0f);

			if (named_side)
			{
				ScriptEngine::Set_Shield_Type (obj, Get_Parameter ("Shield2"));
			}

			return;
		}

		FollowerID = ScriptEngine::Get_ID (poker);

		ScriptEngine::Innate_Disable (obj);

		ActionParamsStruct params;
		params.Set_Basic (this, 100, 100);
		params.Set_Movement (poker, 1.0f, 1.0f);
		params.MoveFollow = true;
		ScriptEngine::Action_Goto (obj, params);

		if (named_side)
		{
			ScriptEngine::Set_Shield_Type (obj, Get_Parameter ("Shield"));
		}
	}
};
