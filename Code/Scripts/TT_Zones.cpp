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
*     TT_Zones.cpp
*
* DESCRIPTION
*     What a volume of space does to whoever walks into it: heal them, hurt
*     them, charge them, sell them something, teleport them, open a gate in
*     front of them, or tell some other object that they arrived.
*
*     Native port of the 4.8.4 library's jfwzone.cpp.  Every script keeps its
*     registered name and its registered parameters, so existing levels find
*     them unchanged.
*
*     The donor file is ninety-three scripts and roughly eight distinct
*     ideas.  Five repair zones are one repair zone and a test of who is
*     allowed in.  Four damage zones are one damage zone and the same test.
*     Twelve relay zones are two relays -- one that addresses an object by
*     ID, one that addresses the enterer -- and a gate in front of them.
*     Eleven grant zones are one grant zone, with or without a price.  Seven
*     spy zones come in matched pairs whose only difference is whether a spy
*     is recognised by a script attached to them or by the preset flag that
*     replaced it.  Those are collapsed here into bases and virtuals; the
*     leaf classes carry the registered name and nothing else.
*
*     Defects in the donor, fixed:
*
*     - Every repair, heal and damage zone re-fetched its target from the
*       timer ID and used it without a null check.  A vehicle destroyed while
*       standing in a repair zone therefore had Get_Position called through a
*       null pointer one second later.  The target is fetched once and
*       checked here.
*
*     - JFW_Heal_Zone_2 compared the ZONE's preset name against its
*       Preset_Name parameter instead of the enterer's -- one line copied
*       from the wrong script -- so the parameter that was meant to pick
*       which characters the zone heals only ever asked whether the zone was
*       named after itself.  It tests the enterer here, like every other
*       Preset_Name script in the file.
*
*     - JFW_Zone_Money_Preset registered its auto-saved variable with the
*       size and the ID the wrong way round, writing a four-byte int as one
*       byte under ID four.
*
*     - JFW_Apply_Damage_On_Enter_Timer auto-saved an `ID` member that
*       nothing ever read or wrote.  Dropped.
*
*     - The force-composition zones sized a raw array with one pass and
*       filled it with a second, and the vehicle version's two passes did not
*       agree: it declared eight pages, listed the same two twice, and then
*       iterated seven of them.  A vector is used here and filled once.
*
*     - The force-composition zones ran each definition name through
*       Get_Translated_Definition_Name and then `delete[]`'d the result --
*       freeing a string they did not own -- and formatted it into a
*       hundred-byte buffer with sprintf.  The engine's own call takes a
*       WideStringClass out-parameter, so neither the free nor the buffer
*       exists here.
*
*     - JFW_Resize_Zone wrote through the reference returned by
*       Get_Bounding_Box.  That accessor is const here; the script builds a
*       box and hands it to Set_Zone_Box.
*
*     - JFW_Spawn_Zone_Created read a "Position" parameter it does not
*       register.  Its registered name is "Offset", which is also what the
*       code does with it.
*
*     Deliberate differences from the donor:
*
*     - PPAGE/MSG/TMSG relayed their text through the SSGM console as
*       `ppage <id> <text>`.  There is no console layer under a built-in
*       script here, so they call Send_Message_Player, Send_Message and
*       Send_Message_Team directly, which is what those console commands did
*       at the far end.
*
*     - The force-composition zones scanned TT's TYPE_AIR and TYPE_NAVAL
*       purchase pages.  Those pages do not exist in this engine; the five
*       that do are scanned.
*
*     - JFW_Sell_Zone sent a custom to itself for compatibility with a
*       dp88 landing-zone script.  dp88 is not part of the ported set, so
*       the send is gone.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "building.h"
#include "constructionyardgameobj.h"
#include "gameobjobserver.h"
#include "matrix3.h"
#include "obbox.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "purchasesettings.h"
#include "scriptzone.h"
#include "soldier.h"
#include "teampurchasesettings.h"
#include "vector.h"
#include "vehicle.h"
#include "widestring.h"
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
**	The side a spy is working against: the one they are not on.
*/
static int	Enemy_Of (int player_type)
{
	return (player_type != PLAYERTYPE_NOD) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;
}


static VehicleGameObj *	As_Vehicle (GameObject * obj)
{
	PhysicalGameObj * physical = (obj != nullptr) ? obj->As_PhysicalGameObj() : nullptr;
	return (physical != nullptr) ? physical->As_VehicleGameObj() : nullptr;
}


/*
**	One tick of a repair or a heal: top up whatever is below its maximum by a
**	flat amount, and say whether anything was actually topped up.
*/
static bool	Restore_By_Amount (GameObject * target, float amount)
{
	bool restored = false;

	float health = ScriptEngine::Get_Health (target);
	if (health < ScriptEngine::Get_Max_Health (target)) {
		ScriptEngine::Set_Health (target, health + amount);
		restored = true;
	}

	float shield = ScriptEngine::Get_Shield_Strength (target);
	if (shield < ScriptEngine::Get_Max_Shield_Strength (target)) {
		ScriptEngine::Set_Shield_Strength (target, shield + amount);
		restored = true;
	}

	return restored;
}


/*
**	A repaired vehicle still wears its damage meshes until it is told to look
**	at itself again.
*/
static void	Refresh_Damage_Meshes (GameObject * target)
{
	VehicleGameObj * vehicle = As_Vehicle (target);
	if (vehicle != nullptr) {
		vehicle->Damage_Meshes_Update();
	}
}


/*
**	Whether a zone that lists up to eight preset names names this one.  The
**	aircraft-only and no-aircraft repair zones are the same list read two
**	ways.
*/
static bool	Named_In_Preset_List (ScriptImpClass * script, GameObject * obj)
{
	const char * preset = ScriptEngine::Get_Preset_Name (obj);
	if (preset == nullptr) { return false; }

	static const char * const	names[] =
	{
		"Preset1", "Preset2", "Preset3", "Preset4",
		"Preset5", "Preset6", "Preset7", "Preset8"
	};

	for (int i = 0; i < 8; i++) {
		const char * listed = script->Get_Parameter (names[i]);
		if ((listed != nullptr) && (::_stricmp (preset, listed) == 0)) {
			return true;
		}
	}

	return false;
}


/*
**	The five purchase pages this engine has.  TT scanned seven, two of which
**	-- air and naval -- it had added itself.
*/
static void	Collect_Purchase_Definitions (PurchaseSettingsDefClass::TYPE type,
		DynamicVectorClass<int> & ids)
{
	static const PurchaseSettingsDefClass::TEAM	teams[] =
	{
		PurchaseSettingsDefClass::TEAM_GDI,
		PurchaseSettingsDefClass::TEAM_NOD
	};

	for (int t = 0; t < 2; t++) {
		PurchaseSettingsDefClass * page = PurchaseSettingsDefClass::Find_Definition (type, teams[t]);
		if (page == nullptr) { continue; }

		for (int i = 0; i < 10; i++) {
			int id = page->Get_Definition (i);
			if (id == 0) { continue; }

			ids.Add (id);

			for (int alt = 0; alt < 3; alt++) {
				int alt_id = page->Get_Alt_Definition (i, alt);
				if (alt_id == 0) { break; }
				ids.Add (alt_id);
			}
		}
	}
}


/*
**	The four characters a team starts with, which are not on a purchase page.
*/
static void	Collect_Enlisted_Definitions (DynamicVectorClass<int> & ids)
{
	static const TeamPurchaseSettingsDefClass::TEAM	teams[] =
	{
		TeamPurchaseSettingsDefClass::TEAM_GDI,
		TeamPurchaseSettingsDefClass::TEAM_NOD
	};

	for (int t = 0; t < 2; t++) {
		TeamPurchaseSettingsDefClass * page = TeamPurchaseSettingsDefClass::Get_Definition (teams[t]);
		if (page == nullptr) { continue; }

		for (int i = 0; i < 4; i++) {
			int id = page->Get_Enlisted_Definition (i);
			if (id != 0) {
				ids.Add (id);
			}
		}
	}
}


/******************************************************************************
*
*     Repair and heal zones
*
*     One zone.  It starts a one-second timer against whoever entered, and
*     every tick tops them up and restarts the timer until they walk out of
*     range.  The five repair variants and the two heal variants differ only
*     in who they accept, which parameters carry the range and the amount,
*     and whether a repaired vehicle needs its damage meshes refreshed.
*
******************************************************************************/

class	JFW_Restore_Zone_Base : public ScriptImpClass
{
protected:
	virtual bool			Accepts (GameObject * enterer) = 0;
	virtual const char *	Distance_Parameter (void) const	{ return "Repair_Distance"; }
	virtual const char *	Amount_Parameter (void) const		{ return "Repair_Health"; }
	virtual bool			Refreshes_Meshes (void) const		{ return true; }

public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!Accepts (enterer))															{ return; }

		ScriptEngine::Start_Timer (obj, this, 1.0f, ScriptEngine::Get_ID (enterer));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		GameObject * target = ScriptEngine::Find_Object (number);
		if (target == nullptr) { return; }

		float distance = ScriptEngine::Get_Distance (ScriptEngine::Get_Position (obj),
				ScriptEngine::Get_Position (target));

		if (distance > Get_Float_Parameter (Distance_Parameter())) { return; }

		if (Restore_By_Amount (target, Get_Float_Parameter (Amount_Parameter()))
				&& Refreshes_Meshes()) {
			Refresh_Damage_Meshes (target);
		}

		ScriptEngine::Start_Timer (obj, this, 1.0f, number);
	}
};


/*
**	A vehicle whose preset says it may be repaired at all.
*/
class	JFW_Vehicle_Repair_Zone_Base : public JFW_Restore_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		VehicleGameObj * vehicle = As_Vehicle (enterer);
		return (vehicle != nullptr) && vehicle->Get_Definition().Can_Repair();
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone, "Player_Type:int,Repair_Distance:float,Repair_Health:float")

class	JFW_Repair_Zone : public JFW_Vehicle_Repair_Zone_Base
{
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_Aircraft_Only, "Player_Type:int,Repair_Distance:float,Repair_Health:float,Preset1:string,Preset2:string,Preset3:string,Preset4:string,Preset5:string,Preset6:string,Preset7:string,Preset8:string")

class	JFW_Repair_Zone_Aircraft_Only : public JFW_Vehicle_Repair_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return Named_In_Preset_List (this, enterer)
				&& JFW_Vehicle_Repair_Zone_Base::Accepts (enterer);
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_No_Aircraft, "Player_Type:int,Repair_Distance:float,Repair_Health:float,Preset1:string,Preset2:string,Preset3:string,Preset4:string,Preset5:string,Preset6:string,Preset7:string,Preset8:string")

class	JFW_Repair_Zone_No_Aircraft : public JFW_Vehicle_Repair_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return !Named_In_Preset_List (this, enterer)
				&& JFW_Vehicle_Repair_Zone_Base::Accepts (enterer);
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_VTOL_Only, "Player_Type:int,Repair_Distance:float,Repair_Health:float")

class	JFW_Repair_Zone_VTOL_Only : public JFW_Vehicle_Repair_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return JFW_Vehicle_Repair_Zone_Base::Accepts (enterer)
				&& (ScriptEngine::Get_Vehicle_Mode (enterer) == VEHICLE_TYPE_FLYING);
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_No_VTOL, "Player_Type:int,Repair_Distance:float,Repair_Health:float")

class	JFW_Repair_Zone_No_VTOL : public JFW_Vehicle_Repair_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return JFW_Vehicle_Repair_Zone_Base::Accepts (enterer)
				&& (ScriptEngine::Get_Vehicle_Mode (enterer) != VEHICLE_TYPE_FLYING);
	}
};


REGISTER_SCRIPT_TT (JFW_Heal_Zone, "Player_Type:int,Heal_Distance:float,Heal_Health:float")

class	JFW_Heal_Zone : public JFW_Restore_Zone_Base
{
protected:
	bool			Accepts (GameObject * enterer) override
	{
		return (enterer != nullptr) && (enterer->As_SoldierGameObj() != nullptr);
	}

	const char *	Distance_Parameter (void) const override	{ return "Heal_Distance"; }
	const char *	Amount_Parameter (void) const override		{ return "Heal_Health"; }
	bool			Refreshes_Meshes (void) const override		{ return false; }
};


/*
**	The same zone restricted to one character preset.  The donor asked the
**	zone for its own preset name; the enterer is what the parameter is for.
*/
REGISTER_SCRIPT_TT (JFW_Heal_Zone_2, "Player_Type:int,Heal_Distance:float,Heal_Health:float,Preset_Name:string")

class	JFW_Heal_Zone_2 : public JFW_Heal_Zone
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		const char * preset = ScriptEngine::Get_Preset_Name (enterer);
		return (preset != nullptr)
				&& (::_stricmp (preset, Get_Parameter ("Preset_Name")) == 0);
	}
};


/******************************************************************************
*
*     Damage zones
*
*     The mirror image of the repair zone: a timer against whoever entered
*     that applies a warhead every tick while they stay in range.
*
******************************************************************************/

class	JFW_Damage_Zone_Base : public ScriptImpClass
{
protected:
	virtual bool	Accepts (GameObject * /*enterer*/)	{ return true; }

public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!Accepts (enterer))															{ return; }

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				ScriptEngine::Get_ID (enterer));
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		GameObject * target = ScriptEngine::Find_Object (number);
		if (target == nullptr) { return; }

		float distance = ScriptEngine::Get_Distance (ScriptEngine::Get_Position (obj),
				ScriptEngine::Get_Position (target));

		if (distance > Get_Float_Parameter ("Distance")) { return; }

		ScriptEngine::Apply_Damage (target, Get_Float_Parameter ("Damage"),
				Get_Parameter ("Warhead"), nullptr);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"), number);
	}
};


REGISTER_SCRIPT_TT (JFW_Apply_Damage_On_Enter_Timer, "Warhead:string,Damage:float,Time:float,Distance:float,Player_Type:int")

class	JFW_Apply_Damage_On_Enter_Timer : public JFW_Damage_Zone_Base
{
};


REGISTER_SCRIPT_TT (JFW_Vechicle_Damage_Zone, "Warhead:string,Damage:float,Time:float,Distance:float,Player_Type:int")

class	JFW_Vechicle_Damage_Zone : public JFW_Damage_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return As_Vehicle (enterer) != nullptr;
	}
};


REGISTER_SCRIPT_TT (JFW_Heavy_Vehicle_Damage_Zone, "Warhead:string,Damage:float,Time:float,Distance:float,Player_Type:int,Mass:float")

class	JFW_Heavy_Vehicle_Damage_Zone : public JFW_Damage_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return (As_Vehicle (enterer) != nullptr)
				&& (ScriptEngine::Get_Mass (enterer) >= Get_Float_Parameter ("Mass"));
	}
};


REGISTER_SCRIPT_TT (JFW_Light_Vehicle_Damage_Zone, "Warhead:string,Damage:float,Time:float,Distance:float,Player_Type:int,Mass:float")

class	JFW_Light_Vehicle_Damage_Zone : public JFW_Damage_Zone_Base
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		return (As_Vehicle (enterer) != nullptr)
				&& (ScriptEngine::Get_Mass (enterer) <= Get_Float_Parameter ("Mass"));
	}
};


/*
**	Damage applied once, the moment somebody walks in.
*/
REGISTER_SCRIPT_TT (JFW_Apply_Damage_On_Enter, "Warhead:string,Damage:float")

class	JFW_Apply_Damage_On_Enter : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Apply_Damage (enterer, Get_Float_Parameter ("Damage"),
				Get_Parameter ("Warhead"), nullptr);
	}
};


REGISTER_SCRIPT_TT (JFW_Blow_Up_On_Enter, "Explosion:string")

class	JFW_Blow_Up_On_Enter : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Create_Explosion (Get_Parameter ("Explosion"),
				ScriptEngine::Get_Position (enterer), nullptr);
	}
};


REGISTER_SCRIPT_TT (JFW_Blow_Up_On_Enter_Random, "Explosion:string,Probobility:int")

class	JFW_Blow_Up_On_Enter_Random : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (ScriptEngine::Get_Random_Int (0, 100) > Get_Int_Parameter ("Probobility")) {
			return;
		}

		ScriptEngine::Create_Explosion (Get_Parameter ("Explosion"),
				ScriptEngine::Get_Position (enterer), nullptr);
	}
};


/*
**	The explosion goes off where the zone is, not where the player is, after
**	a delay -- a mine rather than a trap.
*/
REGISTER_SCRIPT_TT (JFW_Blow_Up_On_Enter_Delay, "Player_Type:int,Time:float,TimerNum:int,Explosion:string")

class	JFW_Blow_Up_On_Enter_Delay : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * obj, int /*number*/) override
	{
		ScriptEngine::Create_Explosion (Get_Parameter ("Explosion"),
				ScriptEngine::Get_Position (obj), nullptr);
	}
};


/******************************************************************************
*
*     Relay zones
*
*     Twelve scripts that turn "somebody crossed this line" into a custom
*     event.  They split two ways.  A relay addressed by object ID treats a
*     zero message as "say nothing" and fills a zero parameter in with the
*     ID of whoever tripped the zone; a relay addressed to the enterer sends
*     whatever it was given.  What varies on top of that is the gate: a
*     switch, a preset name, a keycard, or nothing.
*
******************************************************************************/

class	JFW_Zone_Relay_Base : public ScriptImpClass
{
protected:
	virtual bool			Accepts (GameObject * /*who*/)	{ return true; }
	virtual const char *	Type_Parameter (void) const		{ return "Player_Type"; }

	void	Relay_To_Object (GameObject * obj, GameObject * who,
			const char * message_name, const char * param_name)
	{
		if (!Is_Player_Type (who, Get_Int_Parameter (Type_Parameter())))	{ return; }
		if (!Accepts (who))																{ return; }

		int type = Get_Int_Parameter (message_name);
		if (type == 0) { return; }

		int param = Get_Int_Parameter (param_name);
		if (param == 0) { param = ScriptEngine::Get_ID (who); }

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")), type, param, 0.0f);
	}

	void	Relay_To_Enterer (GameObject * obj, GameObject * who,
			const char * message_name, const char * param_name)
	{
		if (!Is_Player_Type (who, Get_Int_Parameter (Type_Parameter())))	{ return; }
		if (!Accepts (who))																{ return; }

		ScriptEngine::Send_Custom_Event (obj, who, Get_Int_Parameter (message_name),
				Get_Int_Parameter (param_name), 0.0f);
	}
};


class	JFW_Zone_Relay_Object_Base : public JFW_Zone_Relay_Base
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		Relay_To_Object (obj, enterer, "EnterMessage", "EnterParam");
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		Relay_To_Object (obj, exiter, "ExitMessage", "ExitParam");
	}
};


class	JFW_Zone_Relay_Enterer_Base : public JFW_Zone_Relay_Base
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		Relay_To_Enterer (obj, enterer, "EnterMessage", "EnterParam");
	}

	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		Relay_To_Enterer (obj, exiter, "ExitMessage", "ExitParam");
	}
};


/*
**	Switched off until something sends it the enabling custom.
*/
REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Enable, "ID:int,EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Player_Type:int,Enable_Message:int,Disable_Message:int")

class	JFW_Zone_Send_Custom_Enable : public JFW_Zone_Relay_Object_Base
{
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
	}

protected:
	bool	Accepts (GameObject * /*who*/) override		{ return Enabled; }

public:
	void	Created (GameObject * /*obj*/) override		{ Enabled = false; }

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Enable_Message"))	{ Enabled = true; }
		if (type == Get_Int_Parameter ("Disable_Message"))	{ Enabled = false; }
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Enable_Enter, "EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Player_Type:int,Enable_Message:int,Disable_Message:int")

class	JFW_Zone_Send_Custom_Enable_Enter : public JFW_Zone_Relay_Enterer_Base
{
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
	}

protected:
	bool	Accepts (GameObject * /*who*/) override		{ return Enabled; }

public:
	void	Created (GameObject * /*obj*/) override		{ Enabled = false; }

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("Enable_Message"))	{ Enabled = true; }
		if (type == Get_Int_Parameter ("Disable_Message"))	{ Enabled = false; }
	}
};


/*
**	Gated on the preset of whoever crossed the line.
*/
static bool	Is_Named_Preset (ScriptImpClass * script, GameObject * who)
{
	const char * preset = ScriptEngine::Get_Preset_Name (who);
	const char * wanted = script->Get_Parameter ("Preset_Name");

	return (preset != nullptr) && (wanted != nullptr)
			&& (::_stricmp (preset, wanted) == 0);
}


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Preset, "ID:int,EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Player_Type:int,Preset_Name:string")

class	JFW_Zone_Send_Custom_Preset : public JFW_Zone_Relay_Object_Base
{
protected:
	bool	Accepts (GameObject * who) override		{ return Is_Named_Preset (this, who); }
};


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Not_Preset, "ID:int,EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Player_Type:int,Preset_Name:string")

class	JFW_Zone_Send_Custom_Not_Preset : public JFW_Zone_Relay_Object_Base
{
protected:
	bool	Accepts (GameObject * who) override		{ return !Is_Named_Preset (this, who); }
};


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Preset_Enter, "EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Player_Type:int,Preset_Name:string")

class	JFW_Zone_Send_Custom_Preset_Enter : public JFW_Zone_Relay_Enterer_Base
{
protected:
	bool	Accepts (GameObject * who) override		{ return Is_Named_Preset (this, who); }
};


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Keycard, "ID:int,EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Player_Type:int,Keycard_Number:int")

class	JFW_Zone_Send_Custom_Keycard : public JFW_Zone_Relay_Object_Base
{
protected:
	bool	Accepts (GameObject * who) override
	{
		return ScriptEngine::Has_Key (who, Get_Int_Parameter ("Keycard_Number"));
	}
};


/*
**	No gate at all, and the side is named Team_ID rather than Player_Type.
*/
REGISTER_SCRIPT_TT (JFW_Send_Custom_Zone_Enter, "EnterMessage:int,EnterParam:int,ExitMessage:int,ExitParam:int,Team_ID:int")

class	JFW_Send_Custom_Zone_Enter : public JFW_Zone_Relay_Enterer_Base
{
protected:
	const char *	Type_Parameter (void) const override		{ return "Team_ID"; }
};


/*
**	Fires once, when the Count'th body is inside at the same time.
*/
REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Multiple, "Player_Type:int,ID:int,Message:int,Count:int")

class	JFW_Zone_Send_Custom_Multiple : public ScriptImpClass
{
	int	Count;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Count, 1);
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Count = 0; }

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		Count++;

		if (Count == Get_Int_Parameter ("Count")) {
			ScriptEngine::Send_Custom_Event (obj,
					ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
					Get_Int_Parameter ("Message"), 0, 0.0f);
		}
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if (!Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type"))) { return; }

		Count--;
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Enter, "Player_Type:int,Message:int")

class	JFW_Zone_Send_Custom_Enter : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Send_Custom_Event (obj, enterer, Get_Int_Parameter ("Message"), 0, 0.0f);
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Exit, "Player_Type:int,Message:int")

class	JFW_Zone_Send_Custom_Exit : public ScriptImpClass
{
public:
	void	Exited (GameObject * obj, GameObject * exiter) override
	{
		if (!Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Send_Custom_Event (obj, exiter, Get_Int_Parameter ("Message"), 0, 0.0f);
	}
};


/*
**	A relay that charges the player for the privilege.
*/
REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Cost, "ID:int,Message:int,Param:int,Player_Type:int,Cost:int")

class	JFW_Zone_Send_Custom_Cost : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		float cost = (float)Get_Int_Parameter ("Cost");
		if (cost > ScriptEngine::Get_Money (enterer)) { return; }

		ScriptEngine::Give_Money (enterer, -cost, false);

		int param = Get_Int_Parameter ("Param");
		if (param == 0) { param = ScriptEngine::Get_ID (enterer); }

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Message"), param, 0.0f);
	}
};


/*
**	One zone, two addressees: which object hears about the crossing depends
**	on which side crossed.
*/
REGISTER_SCRIPT_TT (JFW_Zone_Send_Custom_Team, "GDIID:int,GDIEnterMessage:int,GDIEnterParam:int,GDIExitMessage:int,GDIExitParam:int,NodID:int,NodEnterMessage:int,NodEnterParam:int,NodExitMessage:int,NodExitParam:int")

class	JFW_Zone_Send_Custom_Team : public ScriptImpClass
{
	void	Relay (GameObject * obj, GameObject * who, bool entering)
	{
		bool nod = (ScriptEngine::Get_Player_Type (who) == PLAYERTYPE_NOD);

		const char * id_name = nod ? "NodID" : "GDIID";
		const char * message_name = nod
				? (entering ? "NodEnterMessage" : "NodExitMessage")
				: (entering ? "GDIEnterMessage" : "GDIExitMessage");
		const char * param_name = nod
				? (entering ? "NodEnterParam" : "NodExitParam")
				: (entering ? "GDIEnterParam" : "GDIExitParam");

		int type = Get_Int_Parameter (message_name);
		if (type == 0) { return; }

		int param = Get_Int_Parameter (param_name);
		if (param == 0) { param = ScriptEngine::Get_ID (who); }

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter (id_name)), type, param, 0.0f);
	}

public:
	void	Entered (GameObject * obj, GameObject * enterer) override	{ Relay (obj, enterer, true); }
	void	Exited (GameObject * obj, GameObject * exiter) override		{ Relay (obj, exiter, false); }
};


/*
**	Relays a vehicle's own entered/exited customs on to whoever caused them,
**	so a driver can be told they are now driving.
*/
REGISTER_SCRIPT_TT (JFW_Send_Driver_Custom_On_Enter, "EnterMessage:int,ExitMessage:int")

class	JFW_Send_Driver_Custom_On_Enter : public ScriptImpClass
{
public:
	void	Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			ScriptEngine::Send_Custom_Event (obj, sender, Get_Int_Parameter ("EnterMessage"), 0, 0.0f);
		}

		if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			ScriptEngine::Send_Custom_Event (obj, sender, Get_Int_Parameter ("ExitMessage"), 0, 0.0f);
		}
	}
};


/******************************************************************************
*
*     Grant zones
*
*     Eleven scripts that hand the player something for walking in: a
*     character, a vehicle, a powerup, a weapon, an object on the ground, or
*     a refill.  Six of them charge for it first.
*
******************************************************************************/

class	JFW_Zone_Grant_Base : public ScriptImpClass
{
protected:
	virtual void	Grant (GameObject * obj, GameObject * enterer) = 0;
	virtual bool	Accepts (GameObject * /*enterer*/)	{ return true; }
	virtual bool	Charges (void) const						{ return false; }

public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!Accepts (enterer))															{ return; }

		if (Charges()) {
			float cost = (float)Get_Int_Parameter ("Cost");
			if (cost > ScriptEngine::Get_Money (enterer)) { return; }

			ScriptEngine::Give_Money (enterer, -cost, false);
		}

		Grant (obj, enterer);
	}
};


class	JFW_Zone_Purchase_Base : public JFW_Zone_Grant_Base
{
protected:
	bool	Charges (void) const override		{ return true; }
};


REGISTER_SCRIPT_TT (JFW_Zone_Character, "Preset_Name:string,Player_Type:int")

class	JFW_Zone_Character : public JFW_Zone_Grant_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Change_Character (enterer, Get_Parameter ("Preset_Name"));
	}
};


/*
**	The same swap, but only for one character preset.
*/
REGISTER_SCRIPT_TT (JFW_Zone_Character_Swap, "Preset_Name:string,Old_Preset_Name:string,Player_Type:int")

class	JFW_Zone_Character_Swap : public JFW_Zone_Character
{
protected:
	bool	Accepts (GameObject * enterer) override
	{
		const char * preset = ScriptEngine::Get_Preset_Name (enterer);
		return (preset != nullptr)
				&& (::_stricmp (preset, Get_Parameter ("Old_Preset_Name")) == 0);
	}
};


REGISTER_SCRIPT_TT (JFW_Character_Buy, "Preset_Name:string,Cost:int,Player_Type:int")

class	JFW_Character_Buy : public JFW_Zone_Purchase_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Change_Character (enterer, Get_Parameter ("Preset_Name"));
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Vehicle, "Preset_Name:string,Player_Type:int")

class	JFW_Zone_Vehicle : public JFW_Zone_Grant_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Create_Vehicle (Get_Parameter ("Preset_Name"), 5.0f, enterer,
				Get_Int_Parameter ("Player_Type"));
	}
};


REGISTER_SCRIPT_TT (JFW_Vehicle_Buy, "Preset_Name:string,Cost:int,Player_Type:int")

class	JFW_Vehicle_Buy : public JFW_Zone_Purchase_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Create_Vehicle (Get_Parameter ("Preset_Name"), 5.0f, enterer,
				Get_Int_Parameter ("Player_Type"));
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Powerup, "Preset_Name:string,Player_Type:int")

class	JFW_Zone_Powerup : public JFW_Zone_Grant_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Give_PowerUp (enterer, Get_Parameter ("Preset_Name"), false);
	}
};


REGISTER_SCRIPT_TT (JFW_Powerup_Buy, "Preset_Name:string,Cost:int,Player_Type:int")

class	JFW_Powerup_Buy : public JFW_Zone_Purchase_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Give_PowerUp (enterer, Get_Parameter ("Preset_Name"), false);
	}
};


/*
**	The one purchase that puts its weapon on the HUD, because the player is
**	meant to notice they are now holding it.
*/
REGISTER_SCRIPT_TT (JFW_Weapon_Buy, "Weapon_Name:string,Cost:int,Player_Type:int")

class	JFW_Weapon_Buy : public JFW_Zone_Purchase_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Give_PowerUp (enterer, Get_Parameter ("Weapon_Name"), true);
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Refill, "Player_Type:int")

class	JFW_Zone_Refill : public JFW_Zone_Grant_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Grant_Refill (enterer);
	}
};


REGISTER_SCRIPT_TT (JFW_Refill_Buy, "Cost:int,Player_Type:int")

class	JFW_Refill_Buy : public JFW_Zone_Purchase_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * enterer) override
	{
		ScriptEngine::Grant_Refill (enterer);
	}
};


/*
**	Not handed to the player: dropped at a fixed spot on the map.
*/
REGISTER_SCRIPT_TT (JFW_Preset_Buy, "Preset_Name:string,Cost:int,location:vector3,Player_Type:int")

class	JFW_Preset_Buy : public JFW_Zone_Purchase_Base
{
protected:
	void	Grant (GameObject * /*obj*/, GameObject * /*enterer*/) override
	{
		ScriptEngine::Create_Object (Get_Parameter ("Preset_Name"),
				Get_Vector3_Parameter ("location"));
	}
};


/******************************************************************************
*
*     Sound zones
*
******************************************************************************/

class	JFW_Sound_Zone_Base : public ScriptImpClass
{
protected:
	virtual void	Play (GameObject * obj, GameObject * enterer, int team, const char * sound) = 0;

public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		int team = Get_Int_Parameter ("Player_Type");
		if (!Is_Player_Type (enterer, team)) { return; }

		Play (obj, enterer, team, Get_Parameter ("Sound"));
	}
};


REGISTER_SCRIPT_TT (JFW_3D_Sound_Zone, "Player_Type:int,Sound:string")

class	JFW_3D_Sound_Zone : public JFW_Sound_Zone_Base
{
protected:
	void	Play (GameObject * obj, GameObject * enterer, int /*team*/, const char * sound) override
	{
		ScriptEngine::Create_Sound (sound, ScriptEngine::Get_Position (enterer), obj);
	}
};


REGISTER_SCRIPT_TT (JFW_3D_Sound_Team_Zone, "Player_Type:int,Sound:string")

class	JFW_3D_Sound_Team_Zone : public JFW_Sound_Zone_Base
{
protected:
	void	Play (GameObject * /*obj*/, GameObject * enterer, int team, const char * sound) override
	{
		ScriptEngine::Create_Sound_Team (team, sound, ScriptEngine::Get_Position (enterer));
	}
};


REGISTER_SCRIPT_TT (JFW_3D_Sound_Player_Zone, "Player_Type:int,Sound:string")

class	JFW_3D_Sound_Player_Zone : public JFW_Sound_Zone_Base
{
protected:
	void	Play (GameObject * /*obj*/, GameObject * enterer, int /*team*/, const char * sound) override
	{
		ScriptEngine::Create_Sound_Player (enterer, sound, ScriptEngine::Get_Position (enterer));
	}
};


REGISTER_SCRIPT_TT (JFW_2D_Sound_Zone, "Player_Type:int,Sound:string")

class	JFW_2D_Sound_Zone : public JFW_Sound_Zone_Base
{
protected:
	void	Play (GameObject * /*obj*/, GameObject * /*enterer*/, int /*team*/, const char * sound) override
	{
		ScriptEngine::Create_2D_Sound (sound);
	}
};


REGISTER_SCRIPT_TT (JFW_2D_Sound_Team_Zone, "Player_Type:int,Sound:string")

class	JFW_2D_Sound_Team_Zone : public JFW_Sound_Zone_Base
{
protected:
	void	Play (GameObject * /*obj*/, GameObject * /*enterer*/, int team, const char * sound) override
	{
		ScriptEngine::Create_2D_Sound_Team (team, sound);
	}
};


REGISTER_SCRIPT_TT (JFW_2D_Sound_Player_Zone, "Player_Type:int,Sound:string")

class	JFW_2D_Sound_Player_Zone : public JFW_Sound_Zone_Base
{
protected:
	void	Play (GameObject * /*obj*/, GameObject * enterer, int /*team*/, const char * sound) override
	{
		ScriptEngine::Create_2D_Sound_Player (enterer, sound);
	}
};


/*
**	The one sound zone with a delay, and with a second side named separately
**	from the side allowed to trip it: a warning the other team hears.
*/
REGISTER_SCRIPT_TT (JFW_2D_Sound_Zone_Team, "Player_Type:int,Sound:string,Time:float,TimerNum:int,Sound_Player_Type:int")

class	JFW_2D_Sound_Zone_Team : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * /*obj*/, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) { return; }

		ScriptEngine::Create_2D_Sound_Team (Get_Int_Parameter ("Sound_Player_Type"),
				Get_Parameter ("Sound"));
	}
};


/******************************************************************************
*
*     Moving the player around
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Bounce_Zone_Entry, "Player_Type:int,Amount:float")

class	JFW_Bounce_Zone_Entry : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		Vector3 position = ScriptEngine::Get_Position (enterer);
		position.Z += Get_Float_Parameter ("Amount");

		ScriptEngine::Set_Position (enterer, position);
	}
};


/*
**	The same shove, aimed along the zone's own facing rather than straight up.
*/
REGISTER_SCRIPT_TT (JFW_Bounce_Zone_Entry_All_Directions, "Player_Type:int,ZAmount:float,Amount:float")

class	JFW_Bounce_Zone_Entry_All_Directions : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		float angle = DEG_TO_RADF (ScriptEngine::Get_Facing (obj));
		float amount = Get_Float_Parameter ("Amount");

		Vector3 position = ScriptEngine::Get_Position (enterer);
		position.Z += Get_Float_Parameter ("ZAmount");
		position.X += amount * cosf (angle);
		position.Y += amount * sinf (angle);

		ScriptEngine::Set_Position (enterer, position);
	}
};


class	JFW_Teleport_Zone_Base : public ScriptImpClass
{
protected:
	virtual bool	Accepts (GameObject * /*enterer*/)	{ return true; }

public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!Accepts (enterer))															{ return; }

		int destination_id = Get_Int_Parameter ("Object_ID");
		GameObject * destination = (destination_id != 0)
				? ScriptEngine::Find_Object (destination_id) : nullptr;

		ScriptEngine::Set_Position (enterer, (destination != nullptr)
				? ScriptEngine::Get_Position (destination)
				: Get_Vector3_Parameter ("Location"));
	}
};


REGISTER_SCRIPT_TT (JFW_Teleport_Zone_Team, "Location:vector3,Object_ID=0:int,Player_Type:int")

class	JFW_Teleport_Zone_Team : public JFW_Teleport_Zone_Base
{
};


REGISTER_SCRIPT_TT (JFW_Teleport_Zone_Enable, "Location:vector3,Object_ID=0:int,Player_Type:int,EnableCustom:int,DisableCustom:int")

class	JFW_Teleport_Zone_Enable : public JFW_Teleport_Zone_Base
{
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Enabled, 1);
	}

protected:
	bool	Accepts (GameObject * /*enterer*/) override	{ return Enabled; }

public:
	void	Created (GameObject * /*obj*/) override			{ Enabled = false; }

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if (type == Get_Int_Parameter ("EnableCustom"))	{ Enabled = true; }
		if (type == Get_Int_Parameter ("DisableCustom"))	{ Enabled = false; }
	}
};


/******************************************************************************
*
*     Doors, keys and what the player can see
*
******************************************************************************/

REGISTER_SCRIPT_TT (JFW_Grant_Key_Zone_Entry, "Player_Type:int,Key:int")

class	JFW_Grant_Key_Zone_Entry : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Grant_Key (enterer, Get_Int_Parameter ("Key"), true);
	}
};


/*
**	A gate driven by a static animation phys object: run it to the end when
**	somebody arrives, snap it back when the timer says they have had long
**	enough to walk through.
*/
REGISTER_SCRIPT_TT (JFW_Gate_Zone, "Player_Type:int,Time:float,TimerNum:int,ID:int,Animation:string")

class	JFW_Gate_Zone : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Static_Anim_Phys_Goto_Last_Frame (Get_Int_Parameter ("ID"),
				Get_Parameter ("Animation"));

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * /*obj*/, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) { return; }

		ScriptEngine::Static_Anim_Phys_Goto_Frame (Get_Int_Parameter ("ID"), 0.0f,
				Get_Parameter ("Animation"));
	}
};


/*
**	The same gate on an ordinary animated object rather than a static one:
**	the door plays forward on arrival and backward when the timer expires.
*/
REGISTER_SCRIPT_TT (JFW_Gate_Zone_2, "Player_Type:int,Time:float,TimerNum:int,ID:int,Animation:string")

class	JFW_Gate_Zone_2 : public ScriptImpClass
{
public:
	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Set_Animation (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Parameter ("Animation"), false, nullptr, 0.0f, -1.0f, false);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
				Get_Int_Parameter ("TimerNum"));
	}

	void	Timer_Expired (GameObject * /*obj*/, int number) override
	{
		if (number != Get_Int_Parameter ("TimerNum")) { return; }

		GameObject * door = ScriptEngine::Find_Object (Get_Int_Parameter ("ID"));
		if (door == nullptr) { return; }

		ScriptEngine::Set_Animation (door, Get_Parameter ("Animation"), false, nullptr,
				ScriptEngine::Get_Animation_Frame (door), 0.0f, false);
	}
};


/*
**	Stealth for anyone in the zone -- except the four presets that are
**	already stealthed and would be un-stealthed on the way out.
*/
REGISTER_SCRIPT_TT (JFW_Stealth_Zone, "Player_Type:int,Stealth1:string,Stealth2:string,Stealth3:string,Stealth4:string")

class	JFW_Stealth_Zone : public ScriptImpClass
{
	bool	Is_Exempt (GameObject * who)
	{
		const char * preset = ScriptEngine::Get_Preset_Name (who);
		if (preset == nullptr) { return false; }

		static const char * const	names[] = { "Stealth1", "Stealth2", "Stealth3", "Stealth4" };

		for (int i = 0; i < 4; i++) {
			const char * listed = Get_Parameter (names[i]);
			if ((listed != nullptr) && (::_stricmp (preset, listed) == 0)) {
				return true;
			}
		}

		return false;
	}

	void	Set_Stealth (GameObject * who, bool onoff)
	{
		if (!Is_Player_Type (who, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (Is_Exempt (who))															{ return; }

		ScriptEngine::Enable_Stealth (who, onoff);
	}

public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override	{ Set_Stealth (enterer, true); }
	void	Exited (GameObject * /*obj*/, GameObject * exiter) override		{ Set_Stealth (exiter, false); }
};


REGISTER_SCRIPT_TT (JFW_Disable_Radar_Zone, "Player_Type:int")

class	JFW_Disable_Radar_Zone : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Enable_Radar_Player (enterer, false);
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if (!Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Enable_Radar_Player (exiter, true);
	}
};


/*
**	A purchase terminal you walk into rather than poke.  Player_Type 2 means
**	either side, and then the terminal shown is the one for the side the
**	player is actually on.
*/
REGISTER_SCRIPT_TT (JFW_Zone_PCT, "Player_Type:int")

class	JFW_Zone_PCT : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		int team = Get_Int_Parameter ("Player_Type");
		if (!Is_Player_Type (enterer, team)) { return; }

		if (team == 2) {
			team = ScriptEngine::Get_Player_Type (enterer);
		}

		if (team == PLAYERTYPE_NOD)	{ ScriptEngine::Display_NOD_Player_Terminal_Player (enterer); }
		if (team == PLAYERTYPE_GDI)	{ ScriptEngine::Display_GDI_Player_Terminal_Player (enterer); }
	}
};


REGISTER_SCRIPT_TT (JFW_Look_At_Location_Entry, "Player_Type:int,Location:vector3")

class	JFW_Look_At_Location_Entry : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		ScriptEngine::Force_Camera_Look_Player (enterer, Get_Vector3_Parameter ("Location"));
	}
};


REGISTER_SCRIPT_TT (JFW_Look_At_Object_Entry, "Player_Type:int,ObjectID:int")

class	JFW_Look_At_Object_Entry : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		GameObject * target = ScriptEngine::Find_Object (Get_Int_Parameter ("ObjectID"));
		if (target == nullptr) { return; }

		ScriptEngine::Force_Camera_Look_Player (enterer, ScriptEngine::Get_Position (target));
	}
};


REGISTER_SCRIPT_TT (JFW_Disable_Transitions_Zone, "")

class	JFW_Disable_Transitions_Zone : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (As_Vehicle (enterer) == nullptr) { return; }

		ScriptEngine::Enable_Vehicle_Transitions (enterer, false);
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if (As_Vehicle (exiter) == nullptr) { return; }

		ScriptEngine::Enable_Vehicle_Transitions (exiter, true);
	}
};


/******************************************************************************
*
*     Weather zones
*
*     Neither of these tests who walked in: the weather is a property of the
*     map, and the zone is a switch on it.
*
******************************************************************************/

class	JFW_Weather_Zone_Base : public ScriptImpClass
{
	enum { TIMER_APPLY = 1 };

protected:
	virtual void	Apply (void) = 0;

	void	Trigger (GameObject * obj, const char * when)
	{
		if (Get_Int_Parameter (when) <= 0) { return; }

		float delay = Get_Float_Parameter ("Delay");

		if (delay > 0.0f) {
			ScriptEngine::Start_Timer (obj, this, delay, TIMER_APPLY);
		} else {
			Apply();
		}
	}

public:
	void	Entered (GameObject * obj, GameObject * /*enterer*/) override	{ Trigger (obj, "OnEnter"); }
	void	Exited (GameObject * obj, GameObject * /*exiter*/) override		{ Trigger (obj, "OnExit"); }

	void	Timer_Expired (GameObject * /*obj*/, int number) override
	{
		if (number == TIMER_APPLY) { Apply(); }
	}
};


REGISTER_SCRIPT_TT (JFW_Fog_Zone, "Fog_Enable:int,Fog_Start_Distance=0.000:float,Fog_End_Distance=0.000:float,Delay=0.000:float,OnEnter=0:int,OnExit=0:int")

class	JFW_Fog_Zone : public JFW_Weather_Zone_Base
{
protected:
	void	Apply (void) override
	{
		bool enabled = Get_Bool_Parameter ("Fog_Enable");

		ScriptEngine::Set_Fog_Enable (enabled);

		if (enabled) {
			ScriptEngine::Set_Fog_Range (Get_Float_Parameter ("Fog_Start_Distance"),
					Get_Float_Parameter ("Fog_End_Distance"), 1.0f);
		}
	}
};


REGISTER_SCRIPT_TT (JFW_War_Blitz_Zone, "War_Blitz_Intensity=0.000:float,Start_Distance=0.000:float,End_Distance=1.000:float,War_Blitz_Heading=0.000:float,War_Blitz_Distribution=1.000:float,Delay=0.000:float,OnEnter=0:int,OnExit=0:int")

class	JFW_War_Blitz_Zone : public JFW_Weather_Zone_Base
{
protected:
	void	Apply (void) override
	{
		ScriptEngine::Set_War_Blitz (Get_Float_Parameter ("War_Blitz_Intensity"),
				Get_Float_Parameter ("Start_Distance"),
				Get_Float_Parameter ("End_Distance"),
				Get_Float_Parameter ("War_Blitz_Heading"),
				Get_Float_Parameter ("War_Blitz_Distribution"), 1.0f);
	}
};


/******************************************************************************
*
*     Animation zones
*
*     A door built out of a cinematic object the zone creates for itself: an
*     "Up" animation on arrival, a "Down" animation on departure, and an
*     idle "Animation" that follows whichever of them finishes.
*
******************************************************************************/

class	JFW_Animation_Zone_Base : public ScriptImpClass
{
	int	Model_ID;
	int	Occupants;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Model_ID, 1);
		SAVE_VARIABLE (Occupants, 2);
	}

protected:
	virtual bool	Accepts (GameObject * /*who*/)	{ return true; }

	//
	//	The parameter set is written three times over with a prefix: "" for
	//	the idle animation, "Up" for the one that plays on arrival, "Down"
	//	for the one that plays on departure.
	//
	void	Play (const char * prefix)
	{
		GameObject * model = ScriptEngine::Find_Object (Model_ID);
		if (model == nullptr) { return; }

		char name[64];

		::snprintf (name, sizeof (name), "%sSubobject", prefix);
		const char * subobject = Get_Parameter (name);
		if ((subobject != nullptr) && (::_stricmp (subobject, "0") == 0)) {
			subobject = nullptr;
		}

		::snprintf (name, sizeof (name), "%sFirstFrame", prefix);
		float first = Get_Float_Parameter (name);
		if (first == -1.0f) {
			first = ScriptEngine::Get_Animation_Frame (model);
		}

		::snprintf (name, sizeof (name), "%sLastFrame", prefix);
		float last = Get_Float_Parameter (name);

		::snprintf (name, sizeof (name), "%sBlended", prefix);
		bool blended = Get_Bool_Parameter (name);

		::snprintf (name, sizeof (name), "%sAnimation", prefix);

		ScriptEngine::Set_Animation (model, Get_Parameter (name), false, subobject,
				first, last, blended);
	}

public:
	void	Created (GameObject * obj) override
	{
		Occupants = 0;

		GameObject * model = ScriptEngine::Create_Object ("Generic_Cinematic",
				ScriptEngine::Get_Position (obj));

		ScriptEngine::Set_Model (model, Get_Parameter ("Model"));

		Model_ID = ScriptEngine::Get_ID (model);
	}

	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Accepts (enterer)) { return; }

		if (Occupants == 0) { Play ("Up"); }

		Occupants++;
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if (!Accepts (exiter)) { return; }

		Occupants--;

		if (Occupants <= 0) {
			Occupants = 0;
			Play ("Down");
		}
	}

	void	Animation_Complete (GameObject * /*obj*/, const char * animation_name) override
	{
		if ((::_stricmp (animation_name, Get_Parameter ("UpAnimation")) != 0)
				&& (::_stricmp (animation_name, Get_Parameter ("Animation")) != 0)) {
			return;
		}

		Play ("");
	}
};


REGISTER_SCRIPT_TT (JFW_Zone_Animation, "Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,UpAnimation:string,UpSubobject:string,UpFirstFrame:float,UpLastFrame:float,UpBlended:int,DownAnimation:string,DownSubobject:string,DownFirstFrame:float,DownLastFrame:float,DownBlended:int,Model:string")

class	JFW_Zone_Animation : public JFW_Animation_Zone_Base
{
};


REGISTER_SCRIPT_TT (JFW_Vehicle_Zone_Animation, "Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int,UpAnimation:string,UpSubobject:string,UpFirstFrame:float,UpLastFrame:float,UpBlended:int,DownAnimation:string,DownSubobject:string,DownFirstFrame:float,DownLastFrame:float,DownBlended:int,Model:string")

class	JFW_Vehicle_Zone_Animation : public JFW_Animation_Zone_Base
{
protected:
	bool	Accepts (GameObject * who) override		{ return As_Vehicle (who) != nullptr; }
};


/******************************************************************************
*
*     Counting zones
*
******************************************************************************/

/*
**	Everybody standing in the zone chips in, and when the pot is full the
**	zone tells something else about it.  The donor GAVE each contributor
**	their stake instead of taking it, and then read a "Message" parameter it
**	does not register -- its name is "Custom" -- so a full pot relayed event
**	zero.  Both are corrected here.
*/
REGISTER_SCRIPT_TT (JFW_Group_Purchase_Zone, "CashPerPerson:float,TotalCash:float,ID:int,Custom:int")

class	JFW_Group_Purchase_Zone : public ScriptImpClass
{
	float	Pot;
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Pot, 1);
		SAVE_VARIABLE (Enabled, 2);
	}

public:
	void	Created (GameObject * /*obj*/) override
	{
		Pot = 0.0f;
		Enabled = true;
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Enabled) { return; }

		float stake = Get_Float_Parameter ("CashPerPerson");

		if (ScriptEngine::Get_Money (enterer) >= stake) {
			ScriptEngine::Give_Money (enterer, -stake, false);
			Pot += stake;
		}

		if (Pot < Get_Float_Parameter ("TotalCash")) { return; }

		Pot = 0.0f;
		Enabled = false;

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Custom"), 0, 0.0f);
	}
};


/*
**	Fires once if Count bodies are still inside when the timer expires.  The
**	donor read the same unregistered "Message" parameter; the registered name
**	is "Custom".
*/
REGISTER_SCRIPT_TT (JFW_Zone_Timer, "Count:int,ID:int,Custom:int,Time:float,TimerNum:int")

class	JFW_Zone_Timer : public ScriptImpClass
{
	int	Inside;
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Inside, 1);
		SAVE_VARIABLE (Enabled, 2);
	}

public:
	void	Created (GameObject * /*obj*/) override
	{
		Inside = 0;
		Enabled = true;
	}

	void	Entered (GameObject * obj, GameObject * /*enterer*/) override
	{
		Inside++;

		if (Enabled && (Inside == 1)) {
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Time"),
					Get_Int_Parameter ("TimerNum"));
		}
	}

	void	Exited (GameObject * /*obj*/, GameObject * /*exiter*/) override
	{
		Inside--;
	}

	void	Timer_Expired (GameObject * obj, int /*number*/) override
	{
		if (Inside < Get_Int_Parameter ("Count")) { return; }

		Enabled = false;

		ScriptEngine::Send_Custom_Event (obj,
				ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Int_Parameter ("Custom"), 0, 0.0f);
	}
};


/*
**	Pays out once per body, not once per crossing: the ID of whoever is
**	standing on the plate is remembered until they step off it.
*/
REGISTER_SCRIPT_TT (JFW_Zone_Money_Preset, "Preset:string,Money:float,Player_Type:int,ID:int")

class	JFW_Zone_Money_Preset : public ScriptImpClass
{
	int	Standing_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Standing_ID, 1);
	}

	bool	Is_Named_Preset (GameObject * who)
	{
		const char * preset = ScriptEngine::Get_Preset_Name (who);
		return (preset != nullptr)
				&& (::_stricmp (preset, Get_Parameter ("Preset")) == 0);
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Standing_ID = 0; }

	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!Is_Named_Preset (enterer))												{ return; }

		if (Standing_ID == ScriptEngine::Get_ID (enterer)) { return; }

		Standing_ID = ScriptEngine::Get_ID (enterer);

		ScriptEngine::Give_Money (ScriptEngine::Find_Object (Get_Int_Parameter ("ID")),
				Get_Float_Parameter ("Money"), true);
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if (!Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!Is_Named_Preset (exiter))												{ return; }

		if (Standing_ID == ScriptEngine::Get_ID (exiter)) {
			Standing_ID = 0;
		}
	}
};


/*
**	Puts an object down while somebody is inside and takes it away again
**	when they leave.
*/
REGISTER_SCRIPT_TT (JFW_Create_Destroy_Object_On_Enter, "Location:vector3,Facing:float,Preset:string,Player_Type:int")

class	JFW_Create_Destroy_Object_On_Enter : public ScriptImpClass
{
	int	Object_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Object_ID, 1);
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Object_ID = 0; }

	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (Object_ID != 0)																{ return; }

		GameObject * object = ScriptEngine::Create_Object (Get_Parameter ("Preset"),
				Get_Vector3_Parameter ("Location"));

		if (object == nullptr) { return; }

		ScriptEngine::Set_Facing (object, Get_Float_Parameter ("Facing"));

		Object_ID = ScriptEngine::Get_ID (object);
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if (!Is_Player_Type (exiter, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (Object_ID == 0)																{ return; }

		ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (Object_ID));
		Object_ID = 0;
	}
};


/******************************************************************************
*
*     Message zones
*
*     The donor built an SSGM console line -- `ppage <id> <text>` -- and fed
*     it back through the console.  There is no console under a built-in
*     script; these call the engine directly.
*
******************************************************************************/

class	JFW_Message_Zone_Base : public ScriptImpClass
{
protected:
	virtual void	Say (GameObject * player, const char * message) = 0;

public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type")))	{ return; }
		if (!ScriptEngine::Is_A_Star (enterer))									{ return; }

		Say (enterer, Get_Parameter ("Message"));
	}
};


REGISTER_SCRIPT_TT (JFW_PPAGE_Zone, "Player_Type:int,Message:string")

class	JFW_PPAGE_Zone : public JFW_Message_Zone_Base
{
protected:
	void	Say (GameObject * player, const char * message) override
	{
		ScriptEngine::Send_Message_Player (player, 255, 255, 255, message);
	}
};


REGISTER_SCRIPT_TT (JFW_MSG_Zone, "Player_Type:int,Message:string")

class	JFW_MSG_Zone : public JFW_Message_Zone_Base
{
protected:
	void	Say (GameObject * /*player*/, const char * message) override
	{
		ScriptEngine::Send_Message (255, 255, 255, message);
	}
};


REGISTER_SCRIPT_TT (JFW_TMSG_Zone, "Player_Type:int,Message:string")

class	JFW_TMSG_Zone : public JFW_Message_Zone_Base
{
protected:
	void	Say (GameObject * player, const char * message) override
	{
		ScriptEngine::Send_Message_Team (ScriptEngine::Get_Player_Type (player),
				255, 255, 255, message);
	}
};


/******************************************************************************
*
*     Service zones
*
*     A vehicle parks, its driver presses the bound key, and the zone either
*     repairs it a slice at a time for money or buys it back.  The three
*     repair variants differ only in which vehicles they accept.
*
******************************************************************************/

class	JFW_Vehicle_Repair_Service_Base : public KeyHookScriptClass
{
	int	Vehicle_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Vehicle_ID, 1);
	}

	//	The price of one full repair, halved and then spread over however
	//	many ticks the Credits parameter asks for.  A zero divisor means
	//	free, not a division by zero.
	float	Tick_Cost (GameObject * vehicle)
	{
		float divisor = Get_Float_Parameter ("Credits");
		if (divisor == 0.0f) { return 0.0f; }

		return (float)(ScriptEngine::Get_Cost (ScriptEngine::Get_Preset_Name (vehicle)) / 2)
				/ divisor;
	}

	void	Release (void)
	{
		if (Waits_For_Key()) { Remove_Hook(); }
		Vehicle_ID = 0;
	}

protected:
	virtual bool	Accepts (GameObject * /*vehicle*/)	{ return true; }
	virtual bool	Waits_For_Key (void)						{ return true; }

	void	Begin_Repairing (GameObject * obj, GameObject * vehicle)
	{
		ScriptEngine::Start_Timer (obj, this, 1.0f, Vehicle_ID);
		ScriptEngine::Create_2D_Sound_Player (ScriptEngine::Get_Vehicle_Driver (vehicle),
				Get_Parameter ("RepairSound"));
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Vehicle_ID = 0; }

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		if (ScriptEngine::Find_Object (Vehicle_ID) == nullptr) { Release(); }

		if ((Vehicle_ID != 0) && (Vehicle_ID != ScriptEngine::Get_ID (enterer))) { return; }

		VehicleGameObj * vehicle = As_Vehicle (enterer);
		if ((vehicle == nullptr) || !vehicle->Get_Definition().Can_Repair())	{ return; }

		GameObject * driver = ScriptEngine::Get_Vehicle_Driver (enterer);
		if (driver == nullptr)		{ return; }
		if (!Accepts (enterer))		{ return; }

		Vehicle_ID = ScriptEngine::Get_ID (enterer);

		if (Waits_For_Key()) {
			ScriptEngine::Create_2D_Sound_Player (driver, Get_Parameter ("Sound"));
			Install_Hook ("Repair", driver);
		} else {
			Begin_Repairing (obj, enterer);
		}
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if ((ScriptEngine::Find_Object (Vehicle_ID) == nullptr)
				|| (Vehicle_ID == ScriptEngine::Get_ID (exiter))) {
			Release();
		}
	}

	void	Key_Hook (void) override
	{
		GameObject * vehicle = ScriptEngine::Find_Object (Vehicle_ID);
		if (vehicle == nullptr) { Release(); return; }

		if (ScriptEngine::Get_Vehicle_Driver (vehicle) == nullptr) { return; }

		Begin_Repairing (Owner(), vehicle);
		Remove_Hook();
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		GameObject * vehicle = ScriptEngine::Find_Object (number);
		GameObject * driver = (vehicle != nullptr)
				? ScriptEngine::Get_Vehicle_Driver (vehicle) : nullptr;

		if (driver != nullptr) {
			float distance = ScriptEngine::Get_Distance (ScriptEngine::Get_Position (obj),
					ScriptEngine::Get_Position (vehicle));

			float cost = Tick_Cost (vehicle);

			if ((distance <= Get_Float_Parameter ("Repair_Distance"))
					&& (ScriptEngine::Get_Money (driver) >= cost)) {

				//	Health and armour are each half the bill, and only the
				//	half that was actually restored is charged for.
				float slice = Get_Float_Parameter ("Repair_Health");
				float scale = 0.0f;

				float health = ScriptEngine::Get_Health (vehicle);
				float max_health = ScriptEngine::Get_Max_Health (vehicle);
				if (health < max_health) {
					ScriptEngine::Set_Health (vehicle, health + max_health / slice);
					scale = 0.5f;
				}

				float shield = ScriptEngine::Get_Shield_Strength (vehicle);
				float max_shield = ScriptEngine::Get_Max_Shield_Strength (vehicle);
				if (shield < max_shield) {
					ScriptEngine::Set_Shield_Strength (vehicle, shield + max_shield / slice);
					scale += 0.5f;
				}

				if (scale > 0.0f) {
					Refresh_Damage_Meshes (vehicle);
					ScriptEngine::Give_Money (driver, -cost * scale, false);
					ScriptEngine::Start_Timer (obj, this, 1.0f, number);
					return;
				}
			}

			ScriptEngine::Create_2D_Sound_Player (driver, Get_Parameter ("RepairStopSound"));
		}

		Vehicle_ID = 0;
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_2, "Player_Type:int,Repair_Distance:float,Repair_Health:float,Sound:string,Credits:float,RepairSound:string,RepairStopSound:string,WaitForKeyhook=1:int")

class	JFW_Repair_Zone_2 : public JFW_Vehicle_Repair_Service_Base
{
protected:
	//	The eighth parameter is newer than the levels that use this script,
	//	so a level that does not carry it gets the original behaviour.
	bool	Waits_For_Key (void) override
	{
		return (Get_Parameter_Count() >= 8)
				? (Get_Int_Parameter ("WaitForKeyhook") == 1)
				: true;
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_No_Boats, "Player_Type:int,Repair_Distance:float,Repair_Health:float,Sound:string,Credits:float,RepairSound:string,RepairStopSound:string")

class	JFW_Repair_Zone_No_Boats : public JFW_Vehicle_Repair_Service_Base
{
protected:
	bool	Accepts (GameObject * vehicle) override
	{
		int mode = ScriptEngine::Get_Vehicle_Mode (vehicle);
		return (mode != VEHICLE_TYPE_BOAT) && (mode != VEHICLE_TYPE_SUB);
	}
};


REGISTER_SCRIPT_TT (JFW_Repair_Zone_Boats, "Player_Type:int,Repair_Distance:float,Repair_Health:float,Sound:string,Credits:float,RepairSound:string,RepairStopSound:string")

class	JFW_Repair_Zone_Boats : public JFW_Vehicle_Repair_Service_Base
{
protected:
	bool	Accepts (GameObject * vehicle) override
	{
		int mode = ScriptEngine::Get_Vehicle_Mode (vehicle);
		return (mode == VEHICLE_TYPE_BOAT) || (mode == VEHICLE_TYPE_SUB);
	}
};


/*
**	Buys the parked vehicle back for half its price, empties it, and destroys
**	it a second later so the occupants are clear.
*/
REGISTER_SCRIPT_TT (JFW_Sell_Zone, "Player_Type:int,Sound:string,SellSound:string")

class	JFW_Sell_Zone : public KeyHookScriptClass
{
	int	Vehicle_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Vehicle_ID, 1);
	}

	void	Release (void)
	{
		Remove_Hook();
		Vehicle_ID = 0;
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Vehicle_ID = 0; }

	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (!Is_Player_Type (enterer, Get_Int_Parameter ("Player_Type"))) { return; }

		if (ScriptEngine::Find_Object (Vehicle_ID) == nullptr) { Release(); }

		if ((Vehicle_ID != 0) && (Vehicle_ID != ScriptEngine::Get_ID (enterer))) { return; }

		VehicleGameObj * vehicle = As_Vehicle (enterer);
		if ((vehicle == nullptr) || !vehicle->Get_Definition().Can_Repair()) { return; }

		GameObject * driver = ScriptEngine::Get_Vehicle_Driver (enterer);
		if (driver == nullptr) { return; }

		ScriptEngine::Create_2D_Sound_Player (driver, Get_Parameter ("Sound"));
		Install_Hook ("Sell", driver);

		Vehicle_ID = ScriptEngine::Get_ID (enterer);
	}

	void	Exited (GameObject * /*obj*/, GameObject * exiter) override
	{
		if ((ScriptEngine::Find_Object (Vehicle_ID) == nullptr)
				|| (Vehicle_ID == ScriptEngine::Get_ID (exiter))) {
			Release();
		}
	}

	void	Key_Hook (void) override
	{
		GameObject * vehicle = ScriptEngine::Find_Object (Vehicle_ID);
		if ((vehicle == nullptr) || (As_Vehicle (vehicle) == nullptr)) { Release(); return; }

		GameObject * driver = ScriptEngine::Get_Vehicle_Driver (vehicle);
		if (driver == nullptr) { return; }

		ScriptEngine::Create_2D_Sound_Player (driver, Get_Parameter ("SellSound"));

		ScriptEngine::Give_Money (driver,
				(float)ScriptEngine::Get_Cost (ScriptEngine::Get_Preset_Name (vehicle)) / 2.0f,
				false);

		ScriptEngine::Force_Occupants_Exit (vehicle);
		ScriptEngine::Start_Timer (Owner(), this, 1.0f, Vehicle_ID);

		Release();
	}

	void	Timer_Expired (GameObject * /*obj*/, int number) override
	{
		ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (number));
	}
};


/******************************************************************************
*
*     Spy zones
*
*     Thirteen scripts and six behaviours.  Almost every one of them comes in
*     two spellings: an older one that recognises a spy by a script the level
*     attached to them, and a newer one that asks the character preset.  The
*     behaviour is written once; the macro at the bottom of each one writes
*     the two leaf classes that differ only in how they answer that question.
*
******************************************************************************/

class	JFW_Spy_Zone_Base : public ScriptImpClass
{
protected:
	virtual bool	Is_Spy_Enterer (GameObject * enterer) = 0;
};


#define JFW_SPY_SCRIPT( name, behaviour, by_preset, params )								\
	REGISTER_SCRIPT_TT( name, params )																\
	class name : public behaviour																		\
	{																											\
	protected:																								\
		bool	Is_Spy_Enterer (GameObject * enterer) override									\
		{																										\
			return (by_preset)																			\
					? ScriptEngine::Is_Spy (enterer)													\
					: ScriptEngine::Is_Script_Attached (enterer, Get_Parameter ("Spy_Script"));	\
		}																										\
	};


/*
**	Hands the spy's own team its radar back, if their Communications Centre
**	is dead and the enemy's radar is still up.  Works once.
*/
class	JFW_Radar_Spy_Base : public JFW_Spy_Zone_Base
{
	bool	Used;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Used, 1);
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Used = false; }

	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if (Used || (ScriptEngine::Get_Vehicle (enterer) != nullptr))	{ return; }
		if (!Is_Spy_Enterer (enterer))										{ return; }

		int team = ScriptEngine::Get_Player_Type (enterer);

		GameObject * centre = ScriptEngine::Find_Com_Center (team);
		if ((centre == nullptr) || !ScriptEngine::Is_Building_Dead (centre))	{ return; }
		if (!ScriptEngine::Is_Radar_Enabled (Enemy_Of (team)))						{ return; }

		ScriptEngine::Create_2D_Sound_Team (team, Get_Parameter ("Sound"));
		ScriptEngine::Enable_Base_Radar (team, true);

		Used = true;
	}
};

JFW_SPY_SCRIPT (JFW_Radar_Spy_Zone, JFW_Radar_Spy_Base, false, "Spy_Script:string,Sound:string")
JFW_SPY_SCRIPT (JFW_Radar_Spy_Zone_New, JFW_Radar_Spy_Base, true, "Sound:string")


/*
**	Reads the enemy's bank balance out to the spy's own team.
*/
class	JFW_Cash_Spy_Base : public JFW_Spy_Zone_Base
{
	enum { TIMER_RESET = 1 };

	bool	Cooling;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Cooling, 1);
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Cooling = false; }

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (Cooling || (ScriptEngine::Get_Vehicle (enterer) != nullptr))	{ return; }
		if (!Is_Spy_Enterer (enterer))										{ return; }

		int team = ScriptEngine::Get_Player_Type (enterer);

		Cooling = true;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("ResetTime"), TIMER_RESET);

		ScriptEngine::Create_2D_Sound_Team (team, Get_Parameter ("Sound"));

		StringClass	line;
		line.Format ("Current Enemy Credits: %.0f\n",
				ScriptEngine::Get_Team_Credits (Enemy_Of (team)));

		ScriptEngine::Send_Message_Team (team, 255, 255, 255, line);
	}

	void	Timer_Expired (GameObject * /*obj*/, int /*number*/) override
	{
		Cooling = false;
	}
};

JFW_SPY_SCRIPT (JFW_Cash_Spy_Zone, JFW_Cash_Spy_Base, false, "Spy_Script:string,Sound:string,ResetTime:float")
JFW_SPY_SCRIPT (JFW_Cash_Spy_Zone_New, JFW_Cash_Spy_Base, true, "Sound:string,ResetTime:float")


/*
**	Cuts the enemy's power for a while, and their radar with it if their
**	Communications Centre is still standing.
*/
class	JFW_Power_Spy_Base : public JFW_Spy_Zone_Base
{
	enum { TIMER_RESTORE = 1, TIMER_RESET = 2 };

	bool	Cooling;
	int		Spy_Team;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Cooling, 1);
		SAVE_VARIABLE (Spy_Team, 2);
	}

	static bool	Has_Live_Power_Plant (int team)
	{
		GameObject * plant = ScriptEngine::Find_Building_By_Type (team,
				BuildingConstants::TYPE_POWER_PLANT);

		return (plant != nullptr) && !ScriptEngine::Is_Building_Dead (plant);
	}

	static void	Set_Radar (int team, bool enable)
	{
		GameObject * centre = ScriptEngine::Find_Building_By_Type (team,
				BuildingConstants::TYPE_COM_CENTER);

		if ((centre != nullptr) && !ScriptEngine::Is_Building_Dead (centre)) {
			ScriptEngine::Enable_Base_Radar (team, enable);
		}
	}

public:
	void	Created (GameObject * /*obj*/) override
	{
		Cooling = false;
		Spy_Team = PLAYERTYPE_NEUTRAL;
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Spy_Enterer (enterer)) { return; }

		Spy_Team = ScriptEngine::Get_Player_Type (enterer);

		int enemy = Enemy_Of (Spy_Team);

		if (Cooling || !ScriptEngine::Is_Base_Powered (enemy))	{ return; }
		if (!Has_Live_Power_Plant (enemy))								{ return; }

		ScriptEngine::Create_2D_Sound_Team (Spy_Team, Get_Parameter ("Sound"));

		ScriptEngine::Power_Base (enemy, false);
		Set_Radar (enemy, false);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Power_Time"), TIMER_RESTORE);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		int enemy = Enemy_Of (Spy_Team);

		if ((number == TIMER_RESTORE) && Has_Live_Power_Plant (enemy)) {
			ScriptEngine::Create_2D_Sound_Team (enemy, Get_Parameter ("Sound2"));

			ScriptEngine::Power_Base (enemy, true);
			Set_Radar (enemy, true);

			Cooling = true;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Reset_Time"), TIMER_RESET);
		}

		if (number == TIMER_RESET) {
			Cooling = false;
		}
	}
};

JFW_SPY_SCRIPT (JFW_Power_Spy_Zone, JFW_Power_Spy_Base, false, "Spy_Script:string,Power_Time:float,Reset_Time:float,Sound:string,Sound2:string")
JFW_SPY_SCRIPT (JFW_Power_Spy_Zone_New, JFW_Power_Spy_Base, true, "Power_Time:float,Reset_Time:float,Sound:string,Sound2:string")


/*
**	Shuts the enemy Construction Yard down for a while by telling it so.  The
**	two custom numbers are the Construction Yard scripts' own.
*/
class	JFW_Conyard_Custom_Spy_Base : public JFW_Spy_Zone_Base
{
	enum
	{
		TIMER_RESTORE			= 1,
		TIMER_RESET				= 2,

		CUSTOM_CONYARD			= 3000,
		CONYARD_DISABLE		= 320023,
		CONYARD_ENABLE			= 230032
	};

	bool	Cooling;
	bool	Disabled;
	int		Spy_Team;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Cooling, 1);
		SAVE_VARIABLE (Disabled, 2);
		SAVE_VARIABLE (Spy_Team, 3);
	}

	static GameObject *	Live_Conyard (int team)
	{
		GameObject * yard = ScriptEngine::Find_Building_By_Type (team,
				BuildingConstants::TYPE_CONYARD);

		return ((yard != nullptr) && !ScriptEngine::Is_Building_Dead (yard)) ? yard : nullptr;
	}

public:
	void	Created (GameObject * /*obj*/) override
	{
		Cooling = false;
		Disabled = false;
		Spy_Team = PLAYERTYPE_NEUTRAL;
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Spy_Enterer (enterer)) { return; }

		Spy_Team = ScriptEngine::Get_Player_Type (enterer);

		if (Cooling || Disabled) { return; }

		GameObject * yard = Live_Conyard (Enemy_Of (Spy_Team));
		if (yard == nullptr) { return; }

		ScriptEngine::Create_2D_Sound_Team (Spy_Team, Get_Parameter ("Sound"));

		Disabled = true;
		ScriptEngine::Send_Custom_Event (obj, yard, CUSTOM_CONYARD, CONYARD_DISABLE, 0.0f);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Power_Time"), TIMER_RESTORE);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		int enemy = Enemy_Of (Spy_Team);

		if (number == TIMER_RESTORE) {
			GameObject * yard = Live_Conyard (enemy);
			if (yard == nullptr) { return; }

			ScriptEngine::Create_2D_Sound_Team (enemy, Get_Parameter ("Sound2"));

			Disabled = false;
			ScriptEngine::Send_Custom_Event (obj, yard, CUSTOM_CONYARD, CONYARD_ENABLE, 0.0f);

			Cooling = true;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Reset_Time"), TIMER_RESET);
		}

		if (number == TIMER_RESET) {
			Cooling = false;
		}
	}
};

JFW_SPY_SCRIPT (JFW_Conyard_Spy_Zone, JFW_Conyard_Custom_Spy_Base, false, "Spy_Script:string,Power_Time:float,Reset_Time:float,Sound:string,Sound2:string")


/*
**	The same idea against the engine's own Construction Yard object rather
**	than a script on it.
*/
class	JFW_Conyard_Spy_Base : public JFW_Spy_Zone_Base
{
	enum { TIMER_RESTORE = 1, TIMER_RESET = 2 };

	bool	Cooling;
	bool	Disabled;
	int		Spy_Team;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Cooling, 1);
		SAVE_VARIABLE (Disabled, 2);
		SAVE_VARIABLE (Spy_Team, 3);
	}

	static ConstructionYardGameObj *	Live_Conyard (int team)
	{
		GameObject * yard = ScriptEngine::Find_Construction_Yard (team);
		if ((yard == nullptr) || ScriptEngine::Is_Building_Dead (yard)) { return nullptr; }

		BuildingGameObj * building = yard->As_BuildingGameObj();
		return (building != nullptr) ? building->As_ConstructionYardGameObj() : nullptr;
	}

public:
	void	Created (GameObject * /*obj*/) override
	{
		Cooling = false;
		Disabled = false;
		Spy_Team = PLAYERTYPE_NEUTRAL;
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (!Is_Spy_Enterer (enterer)) { return; }

		Spy_Team = ScriptEngine::Get_Player_Type (enterer);

		if (Cooling || Disabled) { return; }

		ConstructionYardGameObj * yard = Live_Conyard (Enemy_Of (Spy_Team));
		if (yard == nullptr) { return; }

		ScriptEngine::Create_2D_Sound_Team (Spy_Team, Get_Parameter ("Sound"));

		Disabled = true;
		yard->Set_Spy (true);

		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Power_Time"), TIMER_RESTORE);
	}

	void	Timer_Expired (GameObject * obj, int number) override
	{
		int enemy = Enemy_Of (Spy_Team);

		if (number == TIMER_RESTORE) {
			ConstructionYardGameObj * yard = Live_Conyard (enemy);
			if (yard == nullptr) { return; }

			ScriptEngine::Create_2D_Sound_Team (enemy, Get_Parameter ("Sound2"));

			Disabled = false;
			yard->Set_Spy (false);

			Cooling = true;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("Reset_Time"), TIMER_RESET);
		}

		if (number == TIMER_RESET) {
			Cooling = false;
		}
	}
};

JFW_SPY_SCRIPT (JFW_Conyard_Spy_Zone_2, JFW_Conyard_Spy_Base, false, "Spy_Script:string,Power_Time:float,Reset_Time:float,Sound:string,Sound2:string")
JFW_SPY_SCRIPT (JFW_Conyard_Spy_Zone_2_New, JFW_Conyard_Spy_Base, true, "Power_Time:float,Reset_Time:float,Sound:string,Sound2:string")


/*
**	Reads the enemy's standing army out to the spy's own team, one line per
**	preset they still have alive.  The list of presets worth counting is
**	every character -- or every vehicle -- either team can buy, gathered once
**	when the zone is created.
*/
class	JFW_Force_Composition_Base : public JFW_Spy_Zone_Base
{
	enum { TIMER_RESET = 1 };

	DynamicVectorClass<int>	Definitions;
	bool							Cooling;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Cooling, 1);
	}

protected:
	virtual bool			Wants_Vehicles (void) const = 0;
	virtual const char *	Headline (void) const = 0;

public:
	void	Created (GameObject * /*obj*/) override
	{
		Cooling = false;

		Definitions.Delete_All();

		if (Wants_Vehicles()) {
			Collect_Purchase_Definitions (PurchaseSettingsDefClass::TYPE_VEHICLES, Definitions);
			Collect_Purchase_Definitions (PurchaseSettingsDefClass::TYPE_SECRET_VEHICLES, Definitions);
		} else {
			Collect_Enlisted_Definitions (Definitions);
			Collect_Purchase_Definitions (PurchaseSettingsDefClass::TYPE_CLASSES, Definitions);
			Collect_Purchase_Definitions (PurchaseSettingsDefClass::TYPE_SECRET_CLASSES, Definitions);
		}
	}

	void	Entered (GameObject * obj, GameObject * enterer) override
	{
		if (Cooling || (ScriptEngine::Get_Vehicle (enterer) != nullptr))	{ return; }
		if (!Is_Spy_Enterer (enterer))										{ return; }

		int team = ScriptEngine::Get_Player_Type (enterer);
		int enemy = Enemy_Of (team);

		Cooling = true;
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("ResetTime"), TIMER_RESET);

		ScriptEngine::Create_2D_Sound_Team (team, Get_Parameter ("Sound"));
		ScriptEngine::Send_Message_Team (team, 255, 255, 255, Headline());

		for (int i = 0; i < Definitions.Count(); i++) {
			const char * preset = ScriptEngine::Get_Definition_Name (Definitions[i]);
			if (preset == nullptr) { continue; }

			int count = ScriptEngine::Get_Object_Count (enemy, preset);
			if (count == 0) { continue; }

			WideStringClass	translated;
			ScriptEngine::Get_Translated_Definition_Name (Definitions[i], translated);

			StringClass	name;
			translated.Convert_To (name);

			StringClass	line;
			line.Format ("%s: %d\n", name.Peek_Buffer(), count);

			ScriptEngine::Send_Message_Team (team, 255, 255, 255, line);
		}
	}

	void	Timer_Expired (GameObject * /*obj*/, int /*number*/) override
	{
		Cooling = false;
	}
};


class	JFW_Infantry_Composition_Base : public JFW_Force_Composition_Base
{
protected:
	bool			Wants_Vehicles (void) const override	{ return false; }
	const char *	Headline (void) const override			{ return "Current Enemy Infantry Strength:\n"; }
};


class	JFW_Vehicle_Composition_Base : public JFW_Force_Composition_Base
{
protected:
	bool			Wants_Vehicles (void) const override	{ return true; }
	const char *	Headline (void) const override			{ return "Current Enemy Vehicle Strength:\n"; }
};


JFW_SPY_SCRIPT (JFW_Infantry_Force_Composition_Zone, JFW_Infantry_Composition_Base, false, "Spy_Script:string,Sound:string,ResetTime:float")
JFW_SPY_SCRIPT (JFW_Infantry_Force_Composition_Zone_New, JFW_Infantry_Composition_Base, true, "Sound:string,ResetTime:float")
JFW_SPY_SCRIPT (JFW_Vehicle_Force_Composition_Zone, JFW_Vehicle_Composition_Base, false, "Spy_Script:string,Sound:string,ResetTime:float")
JFW_SPY_SCRIPT (JFW_Vehicle_Force_Composition_Zone_New, JFW_Vehicle_Composition_Base, true, "Sound:string,ResetTime:float")


/******************************************************************************
*
*     Zones that make zones
*
******************************************************************************/

/*
**	Resizes the zone it is attached to, then removes itself.  The donor wrote
**	through the box accessor; it is const here, so the box is rebuilt and
**	handed back.
*/
REGISTER_SCRIPT_TT (JFW_Resize_Zone, "Size:vector3,ZRotate:float")

class	JFW_Resize_Zone : public ScriptImpClass
{
public:
	void	Created (GameObject * obj) override
	{
		ScriptZoneGameObj * zone = (obj != nullptr) ? obj->As_ScriptZoneGameObj() : nullptr;

		if (zone != nullptr) {
			OBBoxClass box = zone->Get_Bounding_Box();

			box.Extent = Get_Vector3_Parameter ("Size");
			box.Basis.Rotate_Z (DEG_TO_RADF (Get_Float_Parameter ("ZRotate")));

			zone->Set_Bounding_Box (box);
		}

		Destroy_Script();
	}
};


static OBBoxClass	Zone_Box (const Vector3 & position, const Vector3 & size, float z_degrees)
{
	Matrix3	rotation;
	rotation.Make_Identity();
	rotation.Rotate_Z (DEG_TO_RADF (z_degrees));

	return OBBoxClass (position, size, rotation);
}


/*
**	Puts a second zone into the world on one custom and takes it away again
**	on another.
*/
REGISTER_SCRIPT_TT (JFW_Custom_Spawn_Zone, "Position:vector3,Size:vector3,ZRotate:float,SpawnCustom:int,DestroyCustom:int,Preset:string")

class	JFW_Custom_Spawn_Zone : public ScriptImpClass
{
	int	Zone_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Zone_ID, 1);
	}

public:
	void	Created (GameObject * /*obj*/) override		{ Zone_ID = 0; }

	void	Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if ((type == Get_Int_Parameter ("SpawnCustom")) && (Zone_ID == 0)) {
			GameObject * zone = ScriptEngine::Create_Zone (Get_Parameter ("Preset"),
					Zone_Box (Get_Vector3_Parameter ("Position"),
							Get_Vector3_Parameter ("Size"),
							Get_Float_Parameter ("ZRotate")));

			Zone_ID = ScriptEngine::Get_ID (zone);
		}

		if ((type == Get_Int_Parameter ("DestroyCustom")) && (Zone_ID != 0)) {
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (Zone_ID));
			Zone_ID = 0;
		}
	}
};


/*
**	The same zone, put down beside whatever this script is attached to and
**	taken away when that thing dies.  The donor read a "Position" parameter;
**	the registered name is "Offset", which is what the code treats it as.
*/
REGISTER_SCRIPT_TT (JFW_Spawn_Zone_Created, "Offset:vector3,Size:vector3,ZRotate:float,Preset:string")

class	JFW_Spawn_Zone_Created : public ScriptImpClass
{
	int	Zone_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (Zone_ID, 1);
	}

public:
	void	Created (GameObject * obj) override
	{
		GameObject * zone = ScriptEngine::Create_Zone (Get_Parameter ("Preset"),
				Zone_Box (ScriptEngine::Get_Position (obj) + Get_Vector3_Parameter ("Offset"),
						Get_Vector3_Parameter ("Size"),
						Get_Float_Parameter ("ZRotate")));

		Zone_ID = ScriptEngine::Get_ID (zone);
	}

	void	Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (Zone_ID));
		Zone_ID = 0;
	}
};


/******************************************************************************
*
*     Markers
*
******************************************************************************/

/*
**	Carries no behaviour.  A level attaches it to say "this vehicle is a
**	boat", and other scripts ask whether it is attached.
*/
REGISTER_SCRIPT_TT (JFW_Boat, "")

class	JFW_Boat : public ScriptImpClass
{
};


/*
**	The same, spelling "do not kill this one" for the zone below.
*/
REGISTER_SCRIPT_TT (JFW_Destroy_Vehicle_Zone_Marker, "")

class	JFW_Destroy_Vehicle_Zone_Marker : public ScriptImpClass
{
};


/*
**	Kills whatever drives in.  A vehicle's occupants are marked first -- with
**	a death script normally, with the exemption marker if the vehicle is
**	being driven by remote control -- so that a remote operator sitting
**	somewhere else is not killed along with their machine.
*/
REGISTER_SCRIPT_TT (JFW_Destroy_Vehicle_Zone, "")

class	JFW_Destroy_Vehicle_Zone : public ScriptImpClass
{
public:
	void	Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if ((enterer == nullptr) || (enterer->As_SmartGameObj() == nullptr)) { return; }

		if (As_Vehicle (enterer) != nullptr) {
			ScriptEngine::Attach_Script_Occupants (enterer,
					ScriptEngine::Is_Script_Attached (enterer, "dp88_RemoteControlVehicle")
							? "JFW_Destroy_Vehicle_Zone_Marker" : "RA_DriverDeath",
					"0");
		}

		if (!ScriptEngine::Is_Script_Attached (enterer, "JFW_Destroy_Vehicle_Zone_Marker")) {
			ScriptEngine::Apply_Damage (enterer, 99999.0f, "Death", nullptr);
		}

		ScriptEngine::Remove_Script (enterer, "JFW_Destroy_Vehicle_Zone_Marker");
	}
};
