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
*     TT_Damage.cpp
*
* DESCRIPTION
*     Being hurt, healed and killed.  Health and armour set or added by a
*     custom, regeneration on a timer, a wreck dropped where something died,
*     an engineer that hears a building being shot and walks over to repair
*     it, and the kill message a server writes when somebody dies.
*
*     Native port of the 4.8.4 library's jfwdmg.cpp: fifty-five registrations
*     over rather fewer behaviours.  Sixteen of them are one script -- set or
*     add, health or armour, current or maximum, to this object or to whoever
*     sent the custom -- which is one template here and sixteen names.  Six
*     more drop an object where something died and differ only over whether
*     they are allowed to.
*
*     Donor defects fixed:
*
*     - JFW_Kill_Message_Display opened its configuration file and used the
*       result without looking at it, so a missing or misspelt file was a
*       crash at level load rather than a script that did nothing.
*
*     - The same script asked four separate objects whether they were bots
*       without checking that they were soldiers, or in two cases that they
*       existed.  A vehicle killed by a turret with no gunner went through
*       As_SoldierGameObj() on a null pointer.
*
*     - It also built its message by newstr-ing four strings, and freed them
*       on one of the two paths out.  The strings here are values.
*
*     - JFW_Give_Points_On_Death had its player-type test inverted, so it
*       awarded points to exactly the teams the level author excluded.
*
*     - JFW_Engineer_Target and JFW_Engineer_Target_2 are the same script.
*       So are two-thirds of JFW_Engineer_Repair and JFW_Engineer_Repair_2:
*       the second takes its target from a parameter instead of from whoever
*       cried out.  Both pairs are one implementation here.
*
*     - JFW_Beacon_Building refused a purchase through the plugin's powerup
*       hook.  That is GameEventBus::Purchase, which is the same veto without
*       a plugin to install it.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "actionparams.h"
#include "building.h"
#include "gameeventbus.h"
#include "gameobjobserver.h"
#include "ini.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "soldier.h"
#include "vector.h"
#include "vehicle.h"
#include "widestring.h"
#include "wwstring.h"

#include <string.h>


/*
**	Two questions asked of a lot of objects here.  Neither the soldier nor the
**	vehicle is reached directly from a scriptable object: the physical object
**	is in between, and the donor went through it without checking.
*/
static SoldierGameObj *	As_Soldier( GameObject * obj )
{
	PhysicalGameObj * physical = ( obj != nullptr ) ? obj->As_PhysicalGameObj() : nullptr;
	return ( physical != nullptr ) ? physical->As_SoldierGameObj() : nullptr;
}


static VehicleGameObj *	As_Vehicle( GameObject * obj )
{
	PhysicalGameObj * physical = ( obj != nullptr ) ? obj->As_PhysicalGameObj() : nullptr;
	return ( physical != nullptr ) ? physical->As_VehicleGameObj() : nullptr;
}


/*
**	Dropping something where an object died.  Six scripts do this; they differ
**	only over whether they are allowed to and what they drop.
*/
static void	Drop_Object( GameObject * obj, const char * preset, float height )
{
	if ( preset == nullptr || preset[0] == 0 ) {
		return ;
	}

	Vector3	position	= ScriptEngine::Get_Position( obj );
	float		facing	= ScriptEngine::Get_Facing( obj );

	position.Z += height;

	GameObject * dropped = ScriptEngine::Create_Object( preset, position );
	if ( dropped != nullptr ) {
		ScriptEngine::Set_Facing( dropped, facing );
	}
}


class	JFW_Spawn_On_Death_Base : public ScriptImpClass
{
public:
	void	Killed( GameObject * obj, GameObject * killer ) override
	{
		if ( Is_Allowed( obj, killer ) ) {
			Drop_Object( obj, Preset( killer ), Get_Float_Parameter( "Drop_Height" ) );
		}

		Destroy_Script();
	}

protected:
	virtual bool			Is_Allowed( GameObject * /*obj*/, GameObject * /*killer*/ )	{ return true; }
	virtual const char *	Preset( GameObject * /*killer*/ )	{ return Get_Parameter( "Drop_Object" ); }
};


/*
**	The half of the family that a custom switches on and off.
*/
class	JFW_Spawn_On_Death_Switched_Base : public JFW_Spawn_On_Death_Base
{
public:
	void	Register_Auto_Save_Variables( void ) override	{ SAVE_VARIABLE( Is_Enabled, 1 ); }

	void	Created( GameObject * /*obj*/ ) override			{ Is_Enabled = false; }

	void	Custom( GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == Get_Int_Parameter( "EnableMessage" ) ) {
			Is_Enabled = true;
		}

		if ( type == Get_Int_Parameter( "DisableMessage" ) ) {
			Is_Enabled = false;
		}
	}

protected:
	bool	Is_Switched_On( void ) const	{ return Is_Enabled; }

	//
	//	The draw is out of a hundred, so a probability of a hundred always
	//	drops and a probability of zero drops one time in a hundred -- which
	//	is the donor's arithmetic and what any level using it was tuned to.
	//
	bool	Wins_Draw( void )
	{
		return ScriptEngine::Get_Random_Int( 0, 100 ) <= Get_Int_Parameter( "Probobility" );
	}

private:
	bool	Is_Enabled;
};


#define JFW_DROP_P			"Drop_Object:string,Drop_Height:float"
#define JFW_DROP_SWITCH_P	",EnableMessage:int,DisableMessage:int"


REGISTER_SCRIPT_TT( JFW_Spawn_Object_Death, JFW_DROP_P )
class	JFW_Spawn_Object_Death : public JFW_Spawn_On_Death_Base
{
};


REGISTER_SCRIPT_TT( JFW_Spawn_Object_Death_Random, JFW_DROP_P ",Probobility:int" )
class	JFW_Spawn_Object_Death_Random : public JFW_Spawn_On_Death_Switched_Base
{
protected:
	bool	Is_Allowed( GameObject * /*obj*/, GameObject * /*killer*/ ) override
	{
		return Wins_Draw();
	}
};


REGISTER_SCRIPT_TT( JFW_Spawn_Object_Death_Enable, JFW_DROP_P JFW_DROP_SWITCH_P )
class	JFW_Spawn_Object_Death_Enable : public JFW_Spawn_On_Death_Switched_Base
{
protected:
	bool	Is_Allowed( GameObject * /*obj*/, GameObject * /*killer*/ ) override
	{
		return Is_Switched_On();
	}
};


REGISTER_SCRIPT_TT( JFW_Spawn_Object_Death_Enable_Random,
		JFW_DROP_P JFW_DROP_SWITCH_P ",Probobility:int" )
class	JFW_Spawn_Object_Death_Enable_Random : public JFW_Spawn_On_Death_Switched_Base
{
protected:
	bool	Is_Allowed( GameObject * /*obj*/, GameObject * /*killer*/ ) override
	{
		return Wins_Draw() && Is_Switched_On();
	}
};


REGISTER_SCRIPT_TT( JFW_Spawn_Object_Death_Weapon, JFW_DROP_P ",Weapon:string" )
class	JFW_Spawn_Object_Death_Weapon : public JFW_Spawn_On_Death_Base
{
protected:
	bool	Is_Allowed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		const char * weapon = ScriptEngine::Get_Current_Weapon( obj );
		return weapon != nullptr && ::stricmp( weapon, Get_Parameter( "Weapon" ) ) == 0;
	}
};


REGISTER_SCRIPT_TT( JFW_Spawn_Object_Death_Team,
		"Drop_Object_GDI:string,Drop_Object_Nod:string,Drop_Height:float" )
class	JFW_Spawn_Object_Death_Team : public JFW_Spawn_On_Death_Base
{
protected:
	const char *	Preset( GameObject * killer ) override
	{
		return ( ScriptEngine::Get_Player_Type( killer ) == PLAYERTYPE_GDI )
				? Get_Parameter( "Drop_Object_GDI" ) : Get_Parameter( "Drop_Object_Nod" );
	}
};


/*
**	Something that happens once, a while after the object is created.
*/
class	JFW_Delayed_Base : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}

	void	Timer_Expired( GameObject * obj, int number ) override
	{
		if ( number == Get_Int_Parameter( "TimerNum" ) ) {
			Act( obj );
		}
	}

protected:
	virtual void	Act( GameObject * obj ) = 0;
};


REGISTER_SCRIPT_TT( JFW_Timer_Destroy_Building, "Time:float,TimerNum:int" )
class	JFW_Timer_Destroy_Building : public JFW_Delayed_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Apply_Damage( obj, 10000.0f, "Explosive", nullptr );
	}
};


REGISTER_SCRIPT_TT( JFW_Timer_Destroy_Object, "Time:float,TimerNum:int,Amount:float,Warhead:string" )
class	JFW_Timer_Destroy_Object : public JFW_Delayed_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Apply_Damage( obj, Get_Float_Parameter( "Amount" ),
				Get_Parameter( "Warhead" ), nullptr );
	}
};


REGISTER_SCRIPT_TT( JFW_Destroy_Self_Timer, "Time:float,TimerNum:int" )
class	JFW_Destroy_Self_Timer : public JFW_Delayed_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Destroy_Object( obj );
	}
};


/*
**	Armour that cannot be hurt for a while after the object appears, done by
**	wearing a different armour type and changing back.
*/
REGISTER_SCRIPT_TT( JFW_Invulnerable_On_Create,
		"Time:float,TimerNum:int,InvulnerableArmour:string,NormalArmour:string" )
class	JFW_Invulnerable_On_Create : public JFW_Delayed_Base
{
public:
	void	Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Shield_Type( obj, Get_Parameter( "InvulnerableArmour" ) );
		JFW_Delayed_Base::Created( obj );
	}

protected:
	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Set_Shield_Type( obj, Get_Parameter( "NormalArmour" ) );
	}
};


/*
**	A driver who does not always survive being shot down.  The check happens
**	half a second after the soldier appears, which is when a pilot ejected
**	from a wrecked vehicle is standing there.
*/
REGISTER_SCRIPT_TT( JFW_Random_DriverDeath, "Percentage:int" )
class	JFW_Random_DriverDeath : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		ScriptEngine::Start_Timer( obj, this, 0.5f, TIMER_CHECK );
	}

	void	Timer_Expired( GameObject * obj, int number ) override
	{
		if ( number != TIMER_CHECK ) {
			return ;
		}

		if ( ScriptEngine::Get_Random_Int( 0, 100 ) >= Get_Int_Parameter( "Percentage" ) ) {
			ScriptEngine::Apply_Damage( obj, 100.0f, "Death", nullptr );
		}
	}

private:
	enum { TIMER_CHECK = 12121 };
};


/*
**	Regeneration.  Health or armour creeps back up on a timer; neither will
**	bring back something that has already reached zero.
*/
class	JFW_Regen_Base : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}

	void	Timer_Expired( GameObject * obj, int number ) override
	{
		if ( number != Get_Int_Parameter( "TimerNum" ) ) {
			return ;
		}

		float current = Read( obj );
		if ( current > 0.0f ) {
			current += Get_Float_Parameter( "Points" );
		}

		float maximum = Read_Max( obj );
		if ( current > maximum ) {
			current = maximum;
		}

		Write( obj, current );

		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}

protected:
	virtual float	Read( GameObject * obj ) = 0;
	virtual float	Read_Max( GameObject * obj ) = 0;
	virtual void	Write( GameObject * obj, float value ) = 0;
};


#define JFW_REGEN_P	"Time:float,TimerNum:int,Points:float"


REGISTER_SCRIPT_TT( JFW_Armour_Regen, JFW_REGEN_P )
class	JFW_Armour_Regen : public JFW_Regen_Base
{
protected:
	float	Read( GameObject * obj ) override		{ return ScriptEngine::Get_Shield_Strength( obj ); }
	float	Read_Max( GameObject * obj ) override	{ return ScriptEngine::Get_Max_Shield_Strength( obj ); }
	void	Write( GameObject * obj, float value ) override
	{
		ScriptEngine::Set_Shield_Strength( obj, value );
	}
};


REGISTER_SCRIPT_TT( JFW_Health_Regen, JFW_REGEN_P )
class	JFW_Health_Regen : public JFW_Regen_Base
{
protected:
	float	Read( GameObject * obj ) override		{ return ScriptEngine::Get_Health( obj ); }
	float	Read_Max( GameObject * obj ) override	{ return ScriptEngine::Get_Max_Health( obj ); }
	void	Write( GameObject * obj, float value ) override
	{
		ScriptEngine::Set_Health( obj, value );
	}
};


/*
**	A pilot who repairs the vehicle they are driving.  Health first; armour
**	only once the hull is whole.
*/
REGISTER_SCRIPT_TT( JFW_Pilot_Repair, "Time:float,TimerNum:int,Health:float" )
class	JFW_Pilot_Repair : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}

	void	Timer_Expired( GameObject * obj, int number ) override
	{
		if ( number != Get_Int_Parameter( "TimerNum" ) ) {
			return ;
		}

		GameObject * vehicle = ScriptEngine::Get_Vehicle( obj );
		if ( vehicle != nullptr && ScriptEngine::Get_Vehicle_Driver( vehicle ) == obj ) {

			float amount = Get_Float_Parameter( "Health" );

			if ( ScriptEngine::Get_Health( vehicle ) != ScriptEngine::Get_Max_Health( vehicle ) ) {
				ScriptEngine::Set_Health( vehicle, ScriptEngine::Get_Health( vehicle ) + amount );
			} else if ( ScriptEngine::Get_Shield_Strength( vehicle ) !=
					ScriptEngine::Get_Max_Shield_Strength( vehicle ) ) {
				ScriptEngine::Set_Shield_Strength( vehicle,
						ScriptEngine::Get_Shield_Strength( vehicle ) + amount );
			}
		}

		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}
};


/*
**	Sixteen scripts that are one script.
**
**	A custom arrives; something is set to a value, or has a value added to it.
**	The something is health or armour, current or maximum.  The victim is this
**	object or whoever sent the custom.  Two by four by two.
*/
struct	JFW_Health_Attribute
{
	static float	Read( GameObject * obj )		{ return ScriptEngine::Get_Health( obj ); }
	static void		Write( GameObject * obj, float value )	{ ScriptEngine::Set_Health( obj, value ); }
};

struct	JFW_Max_Health_Attribute
{
	static float	Read( GameObject * obj )		{ return ScriptEngine::Get_Max_Health( obj ); }
	static void		Write( GameObject * obj, float value )	{ ScriptEngine::Set_Max_Health( obj, value ); }
};

struct	JFW_Shield_Attribute
{
	static float	Read( GameObject * obj )		{ return ScriptEngine::Get_Shield_Strength( obj ); }
	static void		Write( GameObject * obj, float value )
	{
		ScriptEngine::Set_Shield_Strength( obj, value );
	}
};

struct	JFW_Max_Shield_Attribute
{
	static float	Read( GameObject * obj )		{ return ScriptEngine::Get_Max_Shield_Strength( obj ); }
	static void		Write( GameObject * obj, float value )
	{
		ScriptEngine::Set_Max_Shield_Strength( obj, value );
	}
};


template <typename ATTRIBUTE, bool ADDITIVE, bool ON_SENDER>
class	JFW_Attribute_On_Custom : public ScriptImpClass
{
public:
	void	Custom( GameObject * obj, int type, intptr_t /*param*/, GameObject * sender ) override
	{
		if ( type != Get_Int_Parameter( "Custom" ) ) {
			return ;
		}

		GameObject * target = ON_SENDER ? sender : obj;
		if ( target == nullptr ) {
			return ;
		}

		float value = Get_Float_Parameter( "Value" );
		if ( ADDITIVE ) {
			value += ATTRIBUTE::Read( target );
		}

		ATTRIBUTE::Write( target, value );
	}
};


#define JFW_ATTRIBUTE_P	"Custom:int,Value:float"

#define JFW_ATTRIBUTE_SCRIPT( name, attribute, additive, on_sender )	\
	REGISTER_SCRIPT_TT( name, JFW_ATTRIBUTE_P )								\
	class name : public JFW_Attribute_On_Custom<attribute, additive, on_sender> { };

JFW_ATTRIBUTE_SCRIPT( JFW_Set_Health_On_Custom,						JFW_Health_Attribute,		false,	false )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Health_On_Custom,						JFW_Health_Attribute,		true,		false )
JFW_ATTRIBUTE_SCRIPT( JFW_Set_Max_Health_On_Custom,					JFW_Max_Health_Attribute,	false,	false )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Max_Health_On_Custom,					JFW_Max_Health_Attribute,	true,		false )
JFW_ATTRIBUTE_SCRIPT( JFW_Set_Shield_Strength_On_Custom,			JFW_Shield_Attribute,		false,	false )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Shield_Strength_On_Custom,			JFW_Shield_Attribute,		true,		false )
JFW_ATTRIBUTE_SCRIPT( JFW_Set_Max_Shield_Strength_On_Custom,		JFW_Max_Shield_Attribute,	false,	false )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Max_Shield_Strength_On_Custom,		JFW_Max_Shield_Attribute,	true,		false )

JFW_ATTRIBUTE_SCRIPT( JFW_Set_Health_On_Custom_Sender,				JFW_Health_Attribute,		false,	true )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Health_On_Custom_Sender,				JFW_Health_Attribute,		true,		true )
JFW_ATTRIBUTE_SCRIPT( JFW_Set_Max_Health_On_Custom_Sender,			JFW_Max_Health_Attribute,	false,	true )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Max_Health_On_Custom_Sender,			JFW_Max_Health_Attribute,	true,		true )
JFW_ATTRIBUTE_SCRIPT( JFW_Set_Shield_Strength_On_Custom_Sender,	JFW_Shield_Attribute,		false,	true )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Shield_Strength_On_Custom_Sender,	JFW_Shield_Attribute,		true,		true )
JFW_ATTRIBUTE_SCRIPT( JFW_Set_Max_Shield_Strength_On_Custom_Sender,	JFW_Max_Shield_Attribute,	false,	true )
JFW_ATTRIBUTE_SCRIPT( JFW_Add_Max_Shield_Strength_On_Custom_Sender,	JFW_Max_Shield_Attribute,	true,		true )


/*
**	Regeneration a custom switches on, which then keeps itself going by
**	sending itself a custom rather than by holding a timer.
*/
REGISTER_SCRIPT_TT( JFW_Regenerate_Health_Conditional,
		"EnableMessage:int,DisableMessage:int,Amount:int" )
class	JFW_Regenerate_Health_Conditional : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override	{ SAVE_VARIABLE( Is_Enabled, 1 ); }

	void	Created( GameObject * /*obj*/ ) override			{ Is_Enabled = false; }

	void	Custom( GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == Get_Int_Parameter( "EnableMessage" ) ) {
			Is_Enabled = true;
			ScriptEngine::Send_Custom_Event( obj, obj, CUSTOM_TICK, 0, 0.0f );
			return ;
		}

		if ( type == Get_Int_Parameter( "DisableMessage" ) ) {
			Is_Enabled = false;
			return ;
		}

		if ( type == CUSTOM_TICK && Is_Enabled ) {
			float amount = (float)Get_Int_Parameter( "Amount" );
			ScriptEngine::Set_Health( obj, ScriptEngine::Get_Health( obj ) + amount );
			ScriptEngine::Set_Shield_Strength( obj,
					ScriptEngine::Get_Shield_Strength( obj ) + amount );
		}
	}

private:
	//
	//	The donor's own number, kept because a level may send it deliberately.
	//
	enum { CUSTOM_TICK = 100000000 };

	bool	Is_Enabled;
};


/*
**	A custom that damages every building of one side near the sender.  Three
**	scripts, three ways of saying how much.
*/
class	JFW_Building_Damage_Base : public ScriptImpClass
{
public:
	void	Custom( GameObject * /*obj*/, int type, intptr_t param, GameObject * sender ) override
	{
		if ( type != Get_Int_Parameter( "Message" ) ) {
			return ;
		}

		Act( ScriptEngine::Get_Position( sender ), ScriptEngine::Find_Object( (int)param ) );
	}

protected:
	virtual void	Act( const Vector3 & position, GameObject * damager ) = 0;
};


#define JFW_BUILDING_DAMAGE_P	"Message:int,Player_Type:int,"
#define JFW_BUILDING_RANGE_P	"Warhead:string,Distance:float"


REGISTER_SCRIPT_TT( JFW_Building_Damage, JFW_BUILDING_DAMAGE_P "Damage:float," JFW_BUILDING_RANGE_P )
class	JFW_Building_Damage : public JFW_Building_Damage_Base
{
protected:
	void	Act( const Vector3 & position, GameObject * damager ) override
	{
		ScriptEngine::Ranged_Damage_To_Buildings_Team( Get_Int_Parameter( "Player_Type" ),
				Get_Float_Parameter( "Damage" ), Get_Parameter( "Warhead" ),
				position, Get_Float_Parameter( "Distance" ), damager );
	}
};


REGISTER_SCRIPT_TT( JFW_Building_Damage_Scale,
		JFW_BUILDING_DAMAGE_P "Damage:float," JFW_BUILDING_RANGE_P )
class	JFW_Building_Damage_Scale : public JFW_Building_Damage_Base
{
protected:
	void	Act( const Vector3 & position, GameObject * damager ) override
	{
		ScriptEngine::Ranged_Scale_Damage_To_Buildings_Team( Get_Int_Parameter( "Player_Type" ),
				Get_Float_Parameter( "Damage" ), Get_Parameter( "Warhead" ),
				position, Get_Float_Parameter( "Distance" ), damager );
	}
};


REGISTER_SCRIPT_TT( JFW_Building_Damage_Percentage,
		JFW_BUILDING_DAMAGE_P "Percentage:float," JFW_BUILDING_RANGE_P )
class	JFW_Building_Damage_Percentage : public JFW_Building_Damage_Base
{
protected:
	void	Act( const Vector3 & position, GameObject * damager ) override
	{
		ScriptEngine::Ranged_Percentage_Damage_To_Buildings_Team( Get_Int_Parameter( "Player_Type" ),
				Get_Float_Parameter( "Percentage" ), Get_Parameter( "Warhead" ),
				position, Get_Float_Parameter( "Distance" ), damager );
	}
};


/*
**	The latch.  Four scripts do something once when the health crosses a line
**	and re-arm when it comes back.
*/
class	JFW_Health_Latch_Base : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override	{ SAVE_VARIABLE( Is_Armed, 1 ); }

	void	Created( GameObject * /*obj*/ ) override			{ Is_Armed = true; }

	void	Damaged( GameObject * obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		if ( Is_Tripped( obj ) ) {
			if ( Is_Armed ) {
				Is_Armed = false;
				Act( obj );
			}
		} else {
			Is_Armed = true;
		}
	}

protected:
	virtual bool	Is_Tripped( GameObject * obj ) = 0;
	virtual void	Act( GameObject * obj ) = 0;

	bool	Is_Latched( void ) const	{ return !Is_Armed; }

private:
	bool	Is_Armed;
};


REGISTER_SCRIPT_TT( JFW_Animation_Frame_Damage,
		"Animation:string,Frame:int,MinHealth:float,MaxHealth:float" )
class	JFW_Animation_Frame_Damage : public JFW_Health_Latch_Base
{
protected:
	bool	Is_Tripped( GameObject * obj ) override
	{
		float health = ScriptEngine::Get_Health( obj );
		return health <= Get_Float_Parameter( "MaxHealth" ) &&
				health >= Get_Float_Parameter( "MinHealth" );
	}

	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Set_Animation_Frame( obj, Get_Parameter( "Animation" ),
				Get_Int_Parameter( "Frame" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_Change_Model_Health, "Model:string,Health:float" )
class	JFW_Change_Model_Health : public JFW_Health_Latch_Base
{
protected:
	bool	Is_Tripped( GameObject * obj ) override
	{
		return ScriptEngine::Get_Health( obj ) <= Get_Float_Parameter( "Health" );
	}

	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Set_Model( obj, Get_Parameter( "Model" ) );
	}
};


/*
**	The same, the other way up: the model changes when the object is healed
**	past the line rather than hurt past it.
*/
REGISTER_SCRIPT_TT( JFW_Change_Model_Health2, "Model:string,Health:float" )
class	JFW_Change_Model_Health2 : public JFW_Health_Latch_Base
{
protected:
	bool	Is_Tripped( GameObject * obj ) override
	{
		return ScriptEngine::Get_Health( obj ) > Get_Float_Parameter( "Health" );
	}

	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Set_Model( obj, Get_Parameter( "Model" ) );
	}
};


/*
**	Damage over time that starts when the object is hurt past a threshold and
**	stops when it is repaired past it.  A burning vehicle.
*/
REGISTER_SCRIPT_TT( JFW_Damage_Do_Damage,
		"Health:float,Damage:float,Warhead:string,Time:float,TimerNum:int" )
class	JFW_Damage_Do_Damage : public JFW_Health_Latch_Base
{
public:
	void	Timer_Expired( GameObject * obj, int /*number*/ ) override
	{
		if ( !Is_Latched() ) {
			return ;
		}

		ScriptEngine::Apply_Damage( obj, Get_Float_Parameter( "Damage" ),
				Get_Parameter( "Warhead" ), nullptr );
		Start( obj );
	}

protected:
	bool	Is_Tripped( GameObject * obj ) override
	{
		return ScriptEngine::Get_Health( obj ) <= Get_Float_Parameter( "Health" );
	}

	void	Act( GameObject * obj ) override	{ Start( obj ); }

private:
	void	Start( GameObject * obj )
	{
		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}
};


/*
**	An animation played while the object is being hurt, wound back when the
**	shooting stops.
*/
REGISTER_SCRIPT_TT( JFW_Damage_Animation, "Animation:string,Time:float,TimerNum:int" )
class	JFW_Damage_Animation : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override	{ SAVE_VARIABLE( Is_Playing, 1 ); }

	void	Created( GameObject * /*obj*/ ) override			{ Is_Playing = false; }

	void	Damaged( GameObject * obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		if ( !Is_Playing ) {
			Is_Playing = true;
			ScriptEngine::Set_Animation( obj, Get_Parameter( "Animation" ), false );
		}

		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}

	void	Timer_Expired( GameObject * obj, int /*number*/ ) override
	{
		Is_Playing = false;

		//
		//	Back to the beginning from wherever it had got to.
		//
		ScriptEngine::Set_Animation( obj, Get_Parameter( "Animation" ), false, nullptr,
				ScriptEngine::Get_Animation_Frame( obj ), 0.0f, false );
	}

private:
	bool	Is_Playing;
};


/*
**	Things that happen when the object dies.
*/
REGISTER_SCRIPT_TT( JFW_Blow_Up_On_Death, "Explosion:string" )
class	JFW_Blow_Up_On_Death : public ScriptImpClass
{
public:
	void	Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		ScriptEngine::Create_Explosion( Get_Parameter( "Explosion" ),
				ScriptEngine::Get_Position( obj ), nullptr );
		Destroy_Script();
	}
};


/*
**	The same, credited to whoever was driving.  The first person into the seat
**	owns the explosion until they get out, which is how a vehicle blown up
**	after its driver leaves does not score for them.
*/
REGISTER_SCRIPT_TT( JFW_Blow_Up_On_Death_Driver, "Explosion:string" )
class	JFW_Blow_Up_On_Death_Driver : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override	{ SAVE_VARIABLE( DriverID, 1 ); }

	void	Created( GameObject * /*obj*/ ) override			{ DriverID = 0; }

	void	Custom( GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * sender ) override
	{
		if ( type == CUSTOM_EVENT_VEHICLE_ENTERED && DriverID == 0 ) {
			DriverID = ScriptEngine::Get_ID( sender );
		}

		if ( type == CUSTOM_EVENT_VEHICLE_EXITED && DriverID == ScriptEngine::Get_ID( sender ) ) {
			DriverID = 0;
		}
	}

	void	Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		ScriptEngine::Create_Explosion( Get_Parameter( "Explosion" ),
				ScriptEngine::Get_Position( obj ), ScriptEngine::Find_Object( DriverID ) );
		Destroy_Script();
	}

private:
	int	DriverID;
};


/*
**	Points for the kill, but only to the sides the level author allows.
*/
REGISTER_SCRIPT_TT( JFW_Give_Points_On_Death, "Points:float,Player_Type:int,Whole_Team:int" )
class	JFW_Give_Points_On_Death : public ScriptImpClass
{
public:
	void	Killed( GameObject * /*obj*/, GameObject * killer ) override
	{
		if ( !Is_Player_Type( killer, Get_Int_Parameter( "Player_Type" ) ) ) {
			return ;
		}

		ScriptEngine::Give_Points( killer, Get_Float_Parameter( "Points" ),
				Get_Bool_Parameter( "Whole_Team" ) );
		Destroy_Script();
	}
};


REGISTER_SCRIPT_TT( JFW_Death_Destroy_Object, "ID:int" )
class	JFW_Death_Destroy_Object : public ScriptImpClass
{
public:
	void	Killed( GameObject * /*obj*/, GameObject * /*killer*/ ) override
	{
		ScriptEngine::Destroy_Object( ScriptEngine::Find_Object( Get_Int_Parameter( "ID" ) ) );
		Destroy_Script();
	}
};


REGISTER_SCRIPT_TT( JFW_Death_Destroy_Object_Delay, "ID:int,Time:float,TimerNum:int" )
class	JFW_Death_Destroy_Object_Delay : public ScriptImpClass
{
public:
	void	Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Time" ),
				Get_Int_Parameter( "TimerNum" ) );
	}

	void	Timer_Expired( GameObject * /*obj*/, int /*number*/ ) override
	{
		ScriptEngine::Destroy_Object( ScriptEngine::Find_Object( Get_Int_Parameter( "ID" ) ) );
	}
};


REGISTER_SCRIPT_TT( JFW_Damage_Occupants_Death, "Damage:float,Warhead:string" )
class	JFW_Damage_Occupants_Death : public ScriptImpClass
{
public:
	void	Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		ScriptEngine::Damage_Occupants( obj, Get_Float_Parameter( "Damage" ),
				Get_Parameter( "Warhead" ) );
		Destroy_Script();
	}
};


/*
**	Untouchable for a moment after appearing, and topped up while it lasts.
*/
REGISTER_SCRIPT_TT( JFW_Invulnerability_Timer,
		"Invulnerability_Time:float,Invulnerability_TimerNum:int" )
class	JFW_Invulnerability_Timer : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override	{ SAVE_VARIABLE( Is_Invulnerable, 1 ); }

	void	Created( GameObject * obj ) override
	{
		Is_Invulnerable = true;

		ScriptEngine::Start_Timer( obj, this, Get_Float_Parameter( "Invulnerability_Time" ),
				Get_Int_Parameter( "Invulnerability_TimerNum" ) );

		Restore( obj );
	}

	void	Damaged( GameObject * obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		if ( Is_Invulnerable ) {
			Restore( obj );
		}
	}

	void	Timer_Expired( GameObject * /*obj*/, int number ) override
	{
		if ( number == Get_Int_Parameter( "Invulnerability_TimerNum" ) ) {
			Is_Invulnerable = false;
		}
	}

private:
	static void	Restore( GameObject * obj )
	{
		ScriptEngine::Set_Health( obj, ScriptEngine::Get_Max_Health( obj ) );
		ScriptEngine::Set_Shield_Strength( obj, ScriptEngine::Get_Max_Shield_Strength( obj ) );
	}

	bool	Is_Invulnerable;
};


/*
**	Replacing an object with another one the moment it is fully repaired.  A
**	wreck that turns back into a vehicle.
*/
REGISTER_SCRIPT_TT( JFW_Spawn_Object_Repair, "Object:string" )
class	JFW_Spawn_Object_Repair : public ScriptImpClass
{
public:
	void	Damaged( GameObject * obj, GameObject * /*damager*/, float amount ) override
	{
		if ( amount == 0.0f ||
				ScriptEngine::Get_Health( obj ) < ScriptEngine::Get_Max_Health( obj ) ) {
			return ;
		}

		Vector3	position	= ScriptEngine::Get_Position( obj );
		float		facing	= ScriptEngine::Get_Facing( obj );

		ScriptEngine::Destroy_Object( obj );

		GameObject * replacement = ScriptEngine::Create_Object( Get_Parameter( "Object" ), position );
		if ( replacement != nullptr ) {
			ScriptEngine::Set_Facing( replacement, facing );
		}
	}
};


/*
**	An explosion when a particular character's C4 goes off, rather than when
**	anybody's does.
*/
REGISTER_SCRIPT_TT( JFW_C4_Explode, "Preset:string,Explosion:string" )
class	JFW_C4_Explode : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		GameObject *	planter	= ScriptEngine::Get_C4_Planter( obj );
		const char *	preset	= ( planter != nullptr )
				? ScriptEngine::Get_Preset_Name( planter ) : nullptr;

		if ( preset != nullptr && ::stricmp( preset, Get_Parameter( "Preset" ) ) == 0 ) {
			ScriptEngine::Create_Explosion( Get_Parameter( "Explosion" ),
					ScriptEngine::Get_Position( obj ), nullptr );
		}

		Destroy_Script();
	}
};


/*
**	A building that stops its own team buying beacons once it is destroyed.
**
**	The donor installed a plugin's powerup-purchase hook and took it out
**	again in a destructor guarded by which executable it was in.  The purchase
**	channel is the same veto without a plugin: a handler that answers
**	FACTORY_UNAVAILABLE stops the sale.
*/
REGISTER_SCRIPT_TT( JFW_Beacon_Building, "" )
class	JFW_Beacon_Building : public ScriptImpClass
{
public:
	JFW_Beacon_Building( void ) : Token( 0 )	{ }

	~JFW_Beacon_Building( void )
	{
		if ( Token != 0 ) {
			GameEventBus::Purchase.Unregister( Token );
			Token = 0;
		}
	}

	void	Created( GameObject * /*obj*/ ) override
	{
		if ( Token == 0 ) {
			Token = GameEventBus::Purchase.Register( Purchase_Handler, this );
		}
	}

private:
	static void	Purchase_Handler( PurchaseEventClass & event, void * data )
	{
		JFW_Beacon_Building * script = (JFW_Beacon_Building *)data;
		if ( script == nullptr || event.Category != PURCHASE_CATEGORY_POWERUP ) {
			return ;
		}

		BuildingGameObj * building = ( script->Owner() != nullptr )
				? script->Owner()->As_BuildingGameObj() : nullptr;

		if ( building == nullptr || !building->Is_Destroyed() ) {
			return ;
		}

		if ( event.Purchaser != nullptr &&
				building->Get_Player_Type() == ScriptEngine::Get_Player_Type( event.Purchaser ) ) {
			event.Status = PURCHASE_STATUS_FACTORY_UNAVAILABLE;
		}
	}

	int	Token;
};


/*
**	The engineer.
**
**	A building under fire cries out with a logical sound; an engineer that
**	hears one walks to it and repairs it, and stops when it stops crying out.
**	The two halves talk to each other with three sound numbers and one custom.
*/
enum
{
	JFW_ENGINEER_CUSTOM_CLAIM	= 40011,	// engineer to building: I am coming
	JFW_ENGINEER_SOUND_HURT		= 40012,	// building: I am being hurt
	JFW_ENGINEER_TIMER_WATCH	= 40013,	// building: am I still being hurt?
	JFW_ENGINEER_SOUND_SAFE		= 40014,	// building: stop coming
	JFW_ENGINEER_ACTION_GOTO	= 40015,
	JFW_ENGINEER_ACTION_REPAIR	= 40016,
};

static const float	JFW_ENGINEER_CALL_RADIUS	= 150.0f;
static const float	JFW_ENGINEER_WATCH_TIME		= 3.0f;


class	JFW_Engineer_Target_Base : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override
	{
		SAVE_VARIABLE( RepairerID, 1 );
		SAVE_VARIABLE( Health, 2 );
		SAVE_VARIABLE( Shield, 3 );
	}

	void	Created( GameObject * obj ) override
	{
		RepairerID	= 0;
		Health		= ScriptEngine::Get_Health( obj );
		Shield		= ScriptEngine::Get_Shield_Strength( obj );
	}

	void	Custom( GameObject * /*obj*/, int type, intptr_t param, GameObject * /*sender*/ ) override
	{
		if ( type == JFW_ENGINEER_CUSTOM_CLAIM ) {
			RepairerID = (int)param;
		}
	}

	void	Damaged( GameObject * obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		Call( obj, JFW_ENGINEER_SOUND_HURT );
		ScriptEngine::Start_Timer( obj, this, JFW_ENGINEER_WATCH_TIME, JFW_ENGINEER_TIMER_WATCH );
	}

	void	Destroyed( GameObject * obj ) override
	{
		Call( obj, JFW_ENGINEER_SOUND_SAFE );
	}

	//
	//	Nothing has happened for three seconds: either the shooting stopped,
	//	in which case the engineer is told to stand down, or it did not, in
	//	which case the call is repeated and the watch restarted.
	//
	void	Timer_Expired( GameObject * obj, int number ) override
	{
		if ( number != JFW_ENGINEER_TIMER_WATCH ) {
			return ;
		}

		float health = ScriptEngine::Get_Health( obj );
		float shield = ScriptEngine::Get_Shield_Strength( obj );

		if ( health == Health && shield == Shield ) {
			Call( obj, JFW_ENGINEER_SOUND_SAFE );
			return ;
		}

		Health = health;
		Shield = shield;

		Call( obj, JFW_ENGINEER_SOUND_HURT );
		ScriptEngine::Start_Timer( obj, this, JFW_ENGINEER_WATCH_TIME, JFW_ENGINEER_TIMER_WATCH );
	}

private:
	static void	Call( GameObject * obj, int sound )
	{
		ScriptEngine::Create_Logical_Sound( obj, sound, ScriptEngine::Get_Position( obj ),
				JFW_ENGINEER_CALL_RADIUS );
	}

	int	RepairerID;
	float	Health;
	float	Shield;
};


REGISTER_SCRIPT_TT( JFW_Engineer_Target, "" )
class	JFW_Engineer_Target : public JFW_Engineer_Target_Base
{
};


REGISTER_SCRIPT_TT( JFW_Engineer_Target_2, "" )
class	JFW_Engineer_Target_2 : public JFW_Engineer_Target_Base
{
};


class	JFW_Engineer_Repair_Base : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override
	{
		SAVE_VARIABLE( Is_Busy, 1 );
		SAVE_VARIABLE( TargetID, 2 );
		SAVE_VARIABLE( Priority, 3 );
	}

	void	Created( GameObject * /*obj*/ ) override
	{
		Is_Busy	= false;
		TargetID	= 0;
		Priority	= Get_Int_Parameter( "Repair_Priority" );
	}

	void	Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if ( (int)sound.Type == JFW_ENGINEER_SOUND_HURT && !Is_Busy &&
				ScriptEngine::Get_Player_Type( sound.Creator ) == ScriptEngine::Get_Player_Type( obj ) ) {

			TargetID = Target_Of( sound );

			GameObject * target = ScriptEngine::Find_Object( TargetID );
			if ( target == nullptr ) {
				return ;
			}

			ScriptEngine::Send_Custom_Event( obj, sound.Creator, JFW_ENGINEER_CUSTOM_CLAIM,
					ScriptEngine::Get_ID( obj ), 0.0f );
			Is_Busy = true;

			ActionParamsStruct params;
			params.Set_Basic( this, (float)Priority, JFW_ENGINEER_ACTION_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position( target ), 0.8f, 5.0f, false );
			ScriptEngine::Action_Goto( obj, params );
			return ;
		}

		if ( (int)sound.Type == JFW_ENGINEER_SOUND_SAFE && TargetID == Target_Of( sound ) ) {
			ScriptEngine::Action_Reset( obj, 100.0f );
			Is_Busy = false;
		}
	}

	void	Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /*reason*/ ) override
	{
		if ( action_id != JFW_ENGINEER_ACTION_GOTO ) {
			return ;
		}

		ActionParamsStruct params;
		params.Set_Basic( this, (float)Priority, JFW_ENGINEER_ACTION_REPAIR );
		params.Set_Attack( ScriptEngine::Find_Object( TargetID ), 50.0f, 0.0f, false );
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Attack( obj, params );
	}

protected:
	//
	//	The two variants differ here and nowhere else: one repairs whatever
	//	cried out, the other only ever repairs the one building it was given.
	//
	virtual int	Target_Of( const CombatSound & sound ) = 0;

private:
	bool	Is_Busy;
	int	TargetID;
	int	Priority;
};


REGISTER_SCRIPT_TT( JFW_Engineer_Repair, "Repair_Priority=96:int" )
class	JFW_Engineer_Repair : public JFW_Engineer_Repair_Base
{
protected:
	int	Target_Of( const CombatSound & sound ) override
	{
		return ScriptEngine::Get_ID( sound.Creator );
	}
};


REGISTER_SCRIPT_TT( JFW_Engineer_Repair_2, "Repair_Priority=96:int,Target_ID:int" )
class	JFW_Engineer_Repair_2 : public JFW_Engineer_Repair_Base
{
protected:
	int	Target_Of( const CombatSound & /*sound*/ ) override
	{
		return Get_Int_Parameter( "Target_ID" );
	}
};


/*
**	The kill message.
**
**	One object in the level wears JFW_Kill_Message_Display and holds a table,
**	read from an ini file, of "this preset means this line of text".  Every
**	object that can die wears JFW_Kill_Message and tells the display who
**	killed it.  The display assembles four pieces -- killer's name, what the
**	killer used, victim's name, what the victim was -- and writes one line in
**	the killer's team colour.
*/
struct	JFW_Kill_Message_Entry
{
	int	PresetID;
	int	StringID;
	bool	SendDriverCustom;

	JFW_Kill_Message_Entry( void ) : PresetID( 0 ), StringID( 0 ), SendDriverCustom( false )	{ }

	bool	operator == ( const JFW_Kill_Message_Entry & other ) const
	{
		return PresetID == other.PresetID && StringID == other.StringID;
	}

	bool	operator != ( const JFW_Kill_Message_Entry & other ) const
	{
		return !( *this == other );
	}
};


REGISTER_SCRIPT_TT( JFW_Kill_Message_Display, "ConfigFile:string,Message:int" )
class	JFW_Kill_Message_Display : public ScriptImpClass
{
public:
	JFW_Kill_Message_Display( void )
		:	KillerPresetID( 0 ), KilledPresetID( 0 ),
			KillerPlayerID( 0 ), KilledPlayerID( 0 ),
			KillerTeam( PLAYERTYPE_NEUTRAL ),
			KillerIsBot( false ), KilledIsBot( false )	{ }

	//
	//	The donor used the result of Get_INI without looking at it, so a
	//	missing file was a crash rather than a table with nothing in it.
	//
	void	Created( GameObject * /*obj*/ ) override
	{
		INIClass * config = ScriptEngine::Get_INI( Get_Parameter( "ConfigFile" ) );
		if ( config == nullptr ) {
			return ;
		}

		int count = config->Get_Int( "PresetNames", "PresetNameCount", 0 );
		for ( int index = 0; index < count; index ++ ) {

			StringClass	key;
			char			preset[64] = { 0 };

			JFW_Kill_Message_Entry entry;

			key.Format( "Preset%dName", index );
			config->Get_String( "PresetNames", key, "", preset, sizeof( preset ) - 1 );
			entry.PresetID = ScriptEngine::Get_Definition_ID( preset );

			key.Format( "Preset%dStringID", index );
			entry.StringID = config->Get_Int( "PresetNames", key, 0 );

			key.Format( "Preset%dSendDriverCustom", index );
			entry.SendDriverCustom = config->Get_Bool( "PresetNames", key, false );

			Entries.Add( entry );
		}

		ScriptEngine::Release_INI( config );
	}

	void	Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		GameObject * killer = ScriptEngine::Find_Object( (int)param );
		if ( killer == nullptr || sender == nullptr ) {
			return ;
		}

		if ( type == Get_Int_Parameter( "Message" ) ) {

			Read_Victim( sender );

			if ( !Read_Killer( obj, killer ) ) {
				//
				//	The killer was an empty vehicle whose preset asks to be
				//	told who owns it.  The answer comes back as a second
				//	custom and the message is written then.
				//
				return ;
			}

		} else if ( type == CUSTOM_EVENT_VEHICLE_DRIVER_ID ) {

			SoldierGameObj * soldier = As_Soldier( killer );

			KillerIsBot		= ( soldier != nullptr ) && soldier->Is_Bot();
			KillerPlayerID	= KillerIsBot ? (int)param : ScriptEngine::Get_Player_ID( killer );
			KillerTeam		= ScriptEngine::Get_Player_Type( killer );

		} else {
			return ;
		}

		Announce();
	}

private:
	//
	//	Who died.  A player, a bot, or -- when a vehicle died -- whoever was
	//	driving it.  The donor asked three of these whether they were bots
	//	without first asking whether they were soldiers.
	//
	void	Read_Victim( GameObject * victim )
	{
		KilledIsBot = false;

		SoldierGameObj * soldier = As_Soldier( victim );

		if ( ScriptEngine::Is_A_Star( victim ) ) {
			KilledPresetID	= 0;
			KilledPlayerID	= ScriptEngine::Get_Player_ID( victim );
			return ;
		}

		if ( soldier != nullptr && soldier->Is_Bot() ) {
			KilledPresetID	= 0;
			KilledPlayerID	= ScriptEngine::Get_ID( victim );
			KilledIsBot		= true;
			return ;
		}

		KilledPresetID	= ScriptEngine::Get_Preset_ID( victim );
		KilledPlayerID	= 0;

		GameObject * driver = As_Vehicle( victim ) != nullptr
				? ScriptEngine::Get_Vehicle_Driver( victim ) : nullptr;
		if ( driver == nullptr ) {
			return ;
		}

		SoldierGameObj * driver_soldier = As_Soldier( driver );
		if ( driver_soldier != nullptr && driver_soldier->Is_Bot() ) {
			KilledIsBot		= true;
			KilledPlayerID	= ScriptEngine::Get_ID( driver );
		} else {
			KilledPlayerID	= ScriptEngine::Get_Player_ID( driver );
		}
	}

	//
	//	Who killed, and with what.  Returns false when the answer has been
	//	asked for rather than found -- an unmanned vehicle whose owner only
	//	the vehicle itself knows.
	//
	bool	Read_Killer( GameObject * obj, GameObject * killer )
	{
		KillerIsBot		= false;
		KillerPlayerID	= 0;

		//
		//	A soldier inside a vehicle killed with the vehicle, unless the
		//	blast came from something they placed -- a mine, C4, a beacon --
		//	in which case they killed with that.
		//
		GameObject * explosion = ScriptEngine::Get_Explosion_Object();
		if ( explosion == nullptr ||
				( As_Vehicle( explosion ) != nullptr &&
					ScriptEngine::Get_Vehicle_Mode( explosion ) != VEHICLE_TYPE_TURRET ) ) {
			GameObject * vehicle = ScriptEngine::Get_Vehicle( killer );
			if ( vehicle != nullptr ) {
				killer = vehicle;
			}
		}

		KillerTeam = ScriptEngine::Get_Player_Type( killer );

		SoldierGameObj * soldier = As_Soldier( killer );
		if ( soldier != nullptr ) {

			if ( ScriptEngine::Is_A_Star( killer ) ) {
				KillerPlayerID = ScriptEngine::Get_Player_ID( killer );
			} else if ( soldier->Is_Bot() ) {
				KillerIsBot		= true;
				KillerPlayerID	= ScriptEngine::Get_ID( killer );
			}

			KillerPresetID = ( explosion != nullptr )
					? ScriptEngine::Get_Preset_ID( explosion )
					: ScriptEngine::Get_Definition_ID( ScriptEngine::Get_Current_Weapon( killer ) );
			return true;
		}

		if ( As_Vehicle( killer ) == nullptr ) {
			KillerPresetID = ScriptEngine::Get_Preset_ID( killer );
			return true;
		}

		KillerPresetID = ( explosion != nullptr )
				? ScriptEngine::Get_Preset_ID( explosion )
				: ScriptEngine::Get_Preset_ID( killer );

		GameObject * gunner = ScriptEngine::Get_Vehicle_Gunner( killer );
		if ( gunner != nullptr ) {

			SoldierGameObj * gunner_soldier = As_Soldier( gunner );

			if ( gunner_soldier != nullptr && gunner_soldier->Is_Bot() ) {
				KillerIsBot		= true;
				KillerPlayerID	= ScriptEngine::Get_ID( gunner );
			} else {
				KillerPlayerID	= ScriptEngine::Get_Player_ID( gunner );
			}

			KillerTeam = ScriptEngine::Get_Player_Type( gunner );
			return true;
		}

		//
		//	Nobody aboard.  If the table says this preset knows its owner,
		//	ask it and write the message when it answers.
		//
		for ( int index = 0; index < Entries.Count(); index ++ ) {
			if ( Entries[index].PresetID == KillerPresetID && Entries[index].SendDriverCustom ) {
				ScriptEngine::Send_Custom_Event( obj, killer, CUSTOM_EVENT_VEHICLE_DRIVER, 0, 0.0f );
				return false;
			}
		}

		return true;
	}

	void	Announce( void )
	{
		WideStringClass	killer_text;
		WideStringClass	killed_text;

		if ( !Text_For_Preset( KillerPresetID, killer_text ) ||
				!Text_For_Preset( KilledPresetID, killed_text ) ) {
			return ;
		}

		WideStringClass	message;
		message.Format( U_CHAR("%s%s%s%s"),
				Name_Of( KillerPlayerID, KillerIsBot ),
				killer_text.Peek_Buffer(),
				Name_Of( KilledPlayerID, KilledIsBot ),
				killed_text.Peek_Buffer() );

		StringClass narrow;
		message.Convert_To( narrow );

		ScriptEngine::Send_Message_With_Team_Color( KillerTeam, narrow );
	}

	//
	//	A preset of zero means "nothing to say about this half", which is not
	//	a failure.  A preset the table does not know is: the donor bailed out
	//	rather than write half a message, and so does this.
	//
	bool	Text_For_Preset( int preset_id, WideStringClass & text )
	{
		text = U_CHAR("");

		if ( preset_id == 0 ) {
			return true;
		}

		for ( int index = 0; index < Entries.Count(); index ++ ) {
			if ( Entries[index].PresetID == preset_id ) {
				ScriptEngine::Get_Translated_String( Entries[index].StringID, text );
				return true;
			}
		}

		return false;
	}

	const unichar_t *	Name_Of( int id, bool is_bot )
	{
		if ( id == 0 ) {
			return U_CHAR("");
		}

		if ( is_bot ) {
			GameObject * bot = ScriptEngine::Find_Object( id );
			return ( bot != nullptr ) ? ScriptEngine::Get_Wide_Player_Name( bot ) : U_CHAR("");
		}

		return ScriptEngine::Get_Player_Name_By_ID( id );
	}

	DynamicVectorClass<JFW_Kill_Message_Entry>	Entries;

	int	KillerPresetID;
	int	KilledPresetID;
	int	KillerPlayerID;
	int	KilledPlayerID;
	int	KillerTeam;
	bool	KillerIsBot;
	bool	KilledIsBot;
};


REGISTER_SCRIPT_TT( JFW_Kill_Message, "Message:int" )
class	JFW_Kill_Message : public ScriptImpClass
{
public:
	void	Killed( GameObject * obj, GameObject * killer ) override
	{
		GameObject * display = ScriptEngine::Find_Object_With_Script( "JFW_Kill_Message_Display" );
		if ( display != nullptr ) {
			ScriptEngine::Send_Custom_Event( obj, display, Get_Int_Parameter( "Message" ),
					( killer != nullptr ) ? ScriptEngine::Get_ID( killer ) : 0, 0.0f );
		}

		Destroy_Script();
	}
};
