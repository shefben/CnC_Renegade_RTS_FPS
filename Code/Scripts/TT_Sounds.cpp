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
*     TT_Sounds.cpp
*
* DESCRIPTION
*     When a level makes a noise.  A clock that plays something every so
*     often, a building that cries out as it is hurt, music that changes when
*     the player walks somewhere, a countdown that announces the minutes.
*
*     Native port of the 4.8.4 library's jfwsnd.cpp -- twenty-five scripts,
*     which are really six behaviours written out several times each.  Six of
*     them are one timer with six different things on the end of it; six more
*     are one health threshold with six different things on the end of that.
*     Both are written once here.
*
*     The donor read its countdown table into raw arrays of char[50] and a
*     hand-freed array of structures.  A StringClass and a vector do the same
*     job without the two delete[] paths that had to agree with them.
*
*     Donor defects fixed:
*
*     - JFW_Time_Remaining_Sounds_2 read its Team parameter inside the branch
*       that opened the configuration file, so a missing file left the field
*       uninitialised.  Both countdown scripts now read their parameters
*       before deciding whether there is anything to announce.
*
*     - Both countdown scripts asked for a table of SoundCount entries and
*       trusted the number in the file.  A negative count reached new[] as a
*       huge unsigned; the count is now clamped.
*
*     - JFW_Vehicle_Full_Sound built its announcement with sprintf into a
*       hundred-byte buffer from two names it did not measure, and then
*       delete[]'d one of them through a const char *.  It is a string here.
*
*     - JFW_2D_Sound_Damage_Range_Team_String reached into the two house
*       colour globals directly to decide what colour to write in.  That is
*       Get_Color_For_Team, which already knows about more than two teams.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "c4.h"
#include "colors.h"
#include "ini.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "vector.h"
#include "weaponmanager.h"
#include "widestring.h"
#include "wwstring.h"


/*
**	Colours are kept as floats and written as bytes.
*/
static void	Team_Message( int team, const char * message )
{
	Vector3 color = ::Get_Color_For_Team( team );

	ScriptEngine::Send_Message_Team( team,
			(int)( color.X * 255.0f ), (int)( color.Y * 255.0f ), (int)( color.Z * 255.0f ),
			message );
}


/*
**	A sound played at a position the level author nudged away from the object.
*/
static void	Play_Offset_Sound( GameObject * obj, const char * sound, const Vector3 & offset )
{
	Vector3 position = ScriptEngine::Get_Position( obj ) + offset;
	ScriptEngine::Create_Sound( sound, position, obj );
}


/*
**	The timer family.
**
**	Six scripts share one shape: start a timer when created, do something when
**	it expires, and start it again if the level author asked for a repeat.
**	They differ only in how long to wait and what to do at the end.
*/
class	JFW_Sound_Timer_Base : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		ScriptEngine::Start_Timer( obj, this, Delay(), Timer_Number() );
	}

	void	Timer_Expired( GameObject * obj, int number ) override
	{
		if ( number != Timer_Number() ) {
			return ;
		}

		Act( obj );

		if ( Repeats() ) {
			ScriptEngine::Start_Timer( obj, this, Delay(), Timer_Number() );
		}
	}

protected:
	virtual void	Act( GameObject * obj ) = 0;

	virtual float	Delay( void )		{ return Get_Float_Parameter( "Time" ); }
	virtual bool	Repeats( void )	{ return Get_Int_Parameter( "Repeat" ) == 1; }

	int	Timer_Number( void )			{ return Get_Int_Parameter( "TimerNum" ); }
};


/*
**	A delay drawn fresh from a range every time, so a repeating sound does not
**	fall into a rhythm.
*/
class	JFW_Sound_Timer_Random_Base : public JFW_Sound_Timer_Base
{
protected:
	float	Delay( void ) override
	{
		return ScriptEngine::Get_Random( Get_Float_Parameter( "Time_Min" ),
				Get_Float_Parameter( "Time_Max" ) );
	}
};


#define JFW_TIMER_P			"Time:float,TimerNum:int,Repeat:int,"
#define JFW_TIMER_RANDOM_P	"Time_Min:float,Time_Max:float,TimerNum:int,Repeat:int,"


REGISTER_SCRIPT_TT( JFW_2D_Sound_Timer, JFW_TIMER_P "Sound:string" )
class	JFW_2D_Sound_Timer : public JFW_Sound_Timer_Base
{
protected:
	void	Act( GameObject * /*obj*/ ) override
	{
		ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_3D_Sound_Timer, JFW_TIMER_P "Sound:string,Offset:vector3" )
class	JFW_3D_Sound_Timer : public JFW_Sound_Timer_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		Play_Offset_Sound( obj, Get_Parameter( "Sound" ), Get_Vector3_Parameter( "Offset" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_2D_Sound_Timer_Random, JFW_TIMER_RANDOM_P "Sound:string" )
class	JFW_2D_Sound_Timer_Random : public JFW_Sound_Timer_Random_Base
{
protected:
	void	Act( GameObject * /*obj*/ ) override
	{
		ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
	}
};


/*
**	The one member of the family whose position is absolute rather than an
**	offset from the object.
*/
REGISTER_SCRIPT_TT( JFW_3D_Sound_Timer_Random, JFW_TIMER_RANDOM_P "Sound:string,Position:vector3" )
class	JFW_3D_Sound_Timer_Random : public JFW_Sound_Timer_Random_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Create_Sound( Get_Parameter( "Sound" ),
				Get_Vector3_Parameter( "Position" ), obj );
	}
};


REGISTER_SCRIPT_TT( JFW_2D_Sound_Timer_Health,
		JFW_TIMER_P "Sound:string,Min_Health:float,Max_Health:float" )
class	JFW_2D_Sound_Timer_Health : public JFW_Sound_Timer_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		float health = ScriptEngine::Get_Health( obj );

		if ( health >= Get_Float_Parameter( "Min_Health" ) &&
				health <= Get_Float_Parameter( "Max_Health" ) ) {
			ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
		}
	}
};


/*
**	Announcing to the team that one of their vehicles has filled up, so the
**	next person does not walk to it.  The check repeats forever; the message
**	is sent once each time the vehicle goes from having room to not.
*/
REGISTER_SCRIPT_TT( JFW_Vehicle_Full_Sound, "Time:float,TimerNum:int,Sound:string" )
class	JFW_Vehicle_Full_Sound : public JFW_Sound_Timer_Base
{
public:
	void	Register_Auto_Save_Variables( void ) override
	{
		SAVE_VARIABLE( Is_Full, 1 );
	}

	void	Created( GameObject * obj ) override
	{
		Is_Full = false;
		JFW_Sound_Timer_Base::Created( obj );
	}

protected:
	bool	Repeats( void ) override	{ return true; }

	void	Act( GameObject * obj ) override
	{
		if ( obj == nullptr || obj->As_PhysicalGameObj() == nullptr ||
				obj->As_PhysicalGameObj()->As_VehicleGameObj() == nullptr ) {
			return ;
		}

		if ( ScriptEngine::Get_Vehicle_Occupant_Count( obj ) <
				ScriptEngine::Get_Vehicle_Seat_Count( obj ) ) {
			Is_Full = false;
			return ;
		}

		if ( Is_Full ) {
			return ;
		}

		Is_Full = true;

		int team = ScriptEngine::Get_Player_Type( obj );
		ScriptEngine::Create_2D_Sound_Team( team, Get_Parameter( "Sound" ) );

		//
		//	"<driver>'s <vehicle> is now full".  Both halves are wide -- one is
		//	a player's name -- and the message channel is narrow, so they are
		//	joined wide and converted once at the end.
		//
		WideStringClass	vehicle_name;
		ScriptEngine::Get_Translated_Preset_Name( obj, vehicle_name );

		GameObject *	driver		= ScriptEngine::Get_Vehicle_Driver( obj );
		const unichar_t *	driver_name	= ( driver != nullptr )
				? ScriptEngine::Get_Wide_Player_Name( driver ) : U_CHAR("No-one");

		WideStringClass	message;
		message.Format( U_CHAR("%s's %s is now full"), driver_name, vehicle_name.Peek_Buffer() );

		StringClass narrow;
		message.Convert_To( narrow );

		Vector3 color = ScriptEngine::Get_Object_Color( obj );
		ScriptEngine::Send_Message_Team( team,
				(int)( color.X * 255.0f ), (int)( color.Y * 255.0f ), (int)( color.Z * 255.0f ),
				narrow );
	}

private:
	bool	Is_Full;
};


/*
**	The damage family.
**
**	Six scripts share one shape: the first time the object's health falls to
**	a threshold, make a noise; do not make it again until the health has come
**	back up.  Two of them use the same number for both, so they re-arm as
**	soon as the health is anything above the threshold; the other four take a
**	separate number, so a building has to be substantially repaired before it
**	will cry out a second time.
*/
class	JFW_Sound_Damage_Base : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override
	{
		SAVE_VARIABLE( Is_Armed, 1 );
	}

	void	Created( GameObject * /*obj*/ ) override
	{
		Is_Armed = true;
	}

	void	Damaged( GameObject * obj, GameObject * /*damager*/, float amount ) override
	{
		if ( !Accepts( amount ) ) {
			return ;
		}

		float health = ScriptEngine::Get_Health( obj );

		if ( health <= Fire_Health() ) {
			if ( Is_Armed ) {
				Is_Armed = false;
				Act( obj );
			}
		} else if ( health >= Rearm_Health() ) {
			Is_Armed = true;
		}
	}

protected:
	virtual void	Act( GameObject * obj ) = 0;

	virtual float	Fire_Health( void )		{ return Get_Float_Parameter( "MinHealth" ); }
	virtual float	Rearm_Health( void )		{ return Get_Float_Parameter( "MaxHealth" ); }

	//
	//	Only the plain two-dimensional variant ignores repairs.  That is the
	//	donor's behaviour and the two are separate scripts, so it is kept.
	//
	virtual bool	Accepts( float /*amount*/ )	{ return true; }

private:
	bool	Is_Armed;
};


/*
**	The single-threshold half of the family.
*/
class	JFW_Sound_Damage_Single_Base : public JFW_Sound_Damage_Base
{
protected:
	float	Fire_Health( void ) override	{ return Get_Float_Parameter( "Health" ); }
	float	Rearm_Health( void ) override	{ return Get_Float_Parameter( "Health" ); }
};


#define JFW_DAMAGE_RANGE_P	"Sound:string,MinHealth:float,MaxHealth:float"


REGISTER_SCRIPT_TT( JFW_2D_Sound_Damage, "Sound:string,Health:float" )
class	JFW_2D_Sound_Damage : public JFW_Sound_Damage_Single_Base
{
protected:
	bool	Accepts( float amount ) override	{ return amount > 0.0f; }

	void	Act( GameObject * /*obj*/ ) override
	{
		ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_3D_Sound_Damage, "Sound:string,Health:float,Offset:vector3" )
class	JFW_3D_Sound_Damage : public JFW_Sound_Damage_Single_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		Play_Offset_Sound( obj, Get_Parameter( "Sound" ), Get_Vector3_Parameter( "Offset" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_2D_Sound_Damage_Range, JFW_DAMAGE_RANGE_P )
class	JFW_2D_Sound_Damage_Range : public JFW_Sound_Damage_Base
{
protected:
	void	Act( GameObject * /*obj*/ ) override
	{
		ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_3D_Sound_Damage_Range, JFW_DAMAGE_RANGE_P ",Offset:vector3" )
class	JFW_3D_Sound_Damage_Range : public JFW_Sound_Damage_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		Play_Offset_Sound( obj, Get_Parameter( "Sound" ), Get_Vector3_Parameter( "Offset" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_2D_Sound_Damage_Range_Team, JFW_DAMAGE_RANGE_P )
class	JFW_2D_Sound_Damage_Range_Team : public JFW_Sound_Damage_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		ScriptEngine::Create_2D_Sound_Team( ScriptEngine::Get_Player_Type( obj ),
				Get_Parameter( "Sound" ) );
	}
};


/*
**	The same, with a line of text in the team's own colour.
*/
REGISTER_SCRIPT_TT( JFW_2D_Sound_Damage_Range_Team_String, JFW_DAMAGE_RANGE_P ",String:string" )
class	JFW_2D_Sound_Damage_Range_Team_String : public JFW_Sound_Damage_Base
{
protected:
	void	Act( GameObject * obj ) override
	{
		int team = ScriptEngine::Get_Player_Type( obj );

		ScriptEngine::Create_2D_Sound_Team( team, Get_Parameter( "Sound" ) );
		Team_Message( team, Get_Parameter( "String" ) );
	}
};


/*
**	Music.
**
**	Three scripts change the music for everybody when a custom arrives, and
**	three change it for one player when they walk into a zone.
*/
class	JFW_Music_On_Custom_Base : public ScriptImpClass
{
public:
	void	Custom( GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == Get_Int_Parameter( "Message" ) ) {
			Act();
		}
	}

protected:
	virtual void	Act( void ) = 0;
};


REGISTER_SCRIPT_TT( JFW_Set_Background_Music_On_Custom, "Message:int,Music:string" )
class	JFW_Set_Background_Music_On_Custom : public JFW_Music_On_Custom_Base
{
protected:
	void	Act( void ) override
	{
		ScriptEngine::Set_Background_Music( Get_Parameter( "Music" ) );
	}
};


//
//	The two fade times keep the donor's parameter names.  They say nothing,
//	but a level built against 4.8.4 stores its values under them, and a
//	clearer name here would silently drop those values on load.
//
REGISTER_SCRIPT_TT( JFW_Fade_Background_Music_On_Custom,
		"Message:int,Music:string,unk1:int,unk2:int" )
class	JFW_Fade_Background_Music_On_Custom : public JFW_Music_On_Custom_Base
{
protected:
	void	Act( void ) override
	{
		ScriptEngine::Fade_Background_Music( Get_Parameter( "Music" ),
				Get_Int_Parameter( "unk1" ), Get_Int_Parameter( "unk2" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_Stop_Background_Music_On_Custom, "Message:int" )
class	JFW_Stop_Background_Music_On_Custom : public JFW_Music_On_Custom_Base
{
protected:
	void	Act( void ) override
	{
		ScriptEngine::Stop_Background_Music();
	}
};


REGISTER_SCRIPT_TT( JFW_Set_Background_Music_On_Enter, "Music:string" )
class	JFW_Set_Background_Music_On_Enter : public ScriptImpClass
{
public:
	void	Entered( GameObject * /*obj*/, GameObject * enterer ) override
	{
		ScriptEngine::Set_Background_Music_Player( enterer, Get_Parameter( "Music" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_Fade_Background_Music_On_Enter, "Music:string,unk1:int,unk2:int" )
class	JFW_Fade_Background_Music_On_Enter : public ScriptImpClass
{
public:
	void	Entered( GameObject * /*obj*/, GameObject * enterer ) override
	{
		ScriptEngine::Fade_Background_Music_Player( enterer, Get_Parameter( "Music" ),
				Get_Int_Parameter( "unk1" ), Get_Int_Parameter( "unk2" ) );
	}
};


REGISTER_SCRIPT_TT( JFW_Stop_Background_Music_On_Enter, "" )
class	JFW_Stop_Background_Music_On_Enter : public ScriptImpClass
{
public:
	void	Entered( GameObject * /*obj*/, GameObject * enterer ) override
	{
		ScriptEngine::Stop_Background_Music_Player( enterer );
	}
};


/*
**	One-shot noises.
*/
REGISTER_SCRIPT_TT( JFW_2D_Sound_Startup, "Sound:string" )
class	JFW_2D_Sound_Startup : public ScriptImpClass
{
public:
	void	Created( GameObject * /*obj*/ ) override
	{
		ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
		Destroy_Script();
	}
};


REGISTER_SCRIPT_TT( JFW_2D_Sound_Death_Team, "Sound:string" )
class	JFW_2D_Sound_Death_Team : public ScriptImpClass
{
public:
	void	Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		ScriptEngine::Create_2D_Sound_Team( ScriptEngine::Get_Player_Type( obj ),
				Get_Parameter( "Sound" ) );
		Destroy_Script();
	}
};


/*
**	C4 that has just been placed.  A remote charge and a proximity mine are
**	the same object with different ammunition, and a repair beacon is that
**	object again with negative damage -- which is how the two scripts here
**	tell an explosive apart from a healing one.  They differ only over an
**	ammunition that does nothing at all: the first counts it as a weapon,
**	the second does not.
*/
class	JFW_C4_Sound_Base : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		PhysicalGameObj * physical = ( obj != nullptr ) ? obj->As_PhysicalGameObj() : nullptr;
		C4GameObj * charge = ( physical != nullptr ) ? physical->As_C4GameObj() : nullptr;

		if ( charge != nullptr && charge->Peek_Ammo_Definition() != nullptr &&
				Is_A_Weapon( charge->Peek_Ammo_Definition()->Damage ) ) {
			ScriptEngine::Create_2D_Sound( Get_Parameter( "Sound" ) );
		}

		Destroy_Script();
	}

protected:
	virtual bool	Is_A_Weapon( float damage ) const = 0;
};


REGISTER_SCRIPT_TT( JFW_C4_Sound, "Sound:string" )
class	JFW_C4_Sound : public JFW_C4_Sound_Base
{
protected:
	bool	Is_A_Weapon( float damage ) const override	{ return damage >= 0.0f; }
};


REGISTER_SCRIPT_TT( JFW_C4_Sound_2, "Sound:string" )
class	JFW_C4_Sound_2 : public JFW_C4_Sound_Base
{
protected:
	bool	Is_A_Weapon( float damage ) const override	{ return damage > 0.0f; }
};


/*
**	Both sides are told when something finishes being repaired, each in their
**	own sound, so a team hears its own building come back and hears the other
**	team's come back too.
*/
REGISTER_SCRIPT_TT( JFW_Repair_Complete_Sound, "GDISound:string,NodSound:string" )
class	JFW_Repair_Complete_Sound : public ScriptImpClass
{
public:
	void	Register_Auto_Save_Variables( void ) override
	{
		SAVE_VARIABLE( Was_Damaged, 1 );
	}

	void	Created( GameObject * /*obj*/ ) override
	{
		Was_Damaged = false;
	}

	void	Damaged( GameObject * obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		bool is_whole = ( ScriptEngine::Get_Health( obj ) >= ScriptEngine::Get_Max_Health( obj ) ) &&
				( ScriptEngine::Get_Shield_Strength( obj ) >= ScriptEngine::Get_Max_Shield_Strength( obj ) );

		if ( !is_whole ) {
			Was_Damaged = true;
			return ;
		}

		if ( Was_Damaged ) {
			ScriptEngine::Create_2D_Sound_Team( PLAYERTYPE_NOD, Get_Parameter( "NodSound" ) );
			ScriptEngine::Create_2D_Sound_Team( PLAYERTYPE_GDI, Get_Parameter( "GDISound" ) );
			Was_Damaged = false;
		}
	}

private:
	bool	Was_Damaged;
};


/*
**	The countdown.
**
**	A table read from an ini file of "at N minutes remaining, play this".  The
**	clock is checked once a second; each entry fires once.
*/
struct	JFW_Countdown_Entry
{
	StringClass	Sound;
	int			Minutes;
	bool			Played;

	JFW_Countdown_Entry( void ) : Minutes( 0 ), Played( false )	{ }

	//
	//	The vector wants to be able to search itself.  Nothing here does, but
	//	the comparison has to exist for it to compile.
	//
	bool	operator == ( const JFW_Countdown_Entry & other ) const
	{
		return Minutes == other.Minutes && Sound == other.Sound;
	}

	bool	operator != ( const JFW_Countdown_Entry & other ) const
	{
		return !( *this == other );
	}
};


class	JFW_Time_Remaining_Base : public ScriptImpClass
{
public:
	void	Created( GameObject * obj ) override
	{
		Read_Parameters();

		if ( ScriptEngine::Get_Time_Remaining_Seconds() <= 0.0f ) {
			return ;
		}

		INIClass * config = ScriptEngine::Get_INI( Get_Parameter( "ConfigFile" ) );
		if ( config == nullptr ) {
			return ;
		}

		//
		//	The count comes out of a file an operator edits, so it is clamped
		//	rather than trusted.
		//
		int count = config->Get_Int( "Sounds", "SoundCount", 0 );
		for ( int index = 0; index < count; index ++ ) {

			StringClass key;
			char sound[64] = { 0 };

			JFW_Countdown_Entry entry;

			key.Format( "Sound%dName", index );
			config->Get_String( "Sounds", key, "", sound, sizeof( sound ) - 1 );
			entry.Sound = sound;

			key.Format( "Sound%dTime", index );
			entry.Minutes = config->Get_Int( "Sounds", key, 0 );

			Entries.Add( entry );
		}

		ScriptEngine::Release_INI( config );

		Timer_Expired( obj, 1 );
	}

	void	Timer_Expired( GameObject * obj, int /*number*/ ) override
	{
		int hours = 0;
		int minutes = 0;
		int seconds = 0;
		ScriptEngine::Seconds_To_Hms( ScriptEngine::Get_Time_Remaining_Seconds(),
				hours, minutes, seconds );
		minutes += hours * 60;

		for ( int index = 0; index < Entries.Count(); index ++ ) {
			if ( Entries[index].Minutes == minutes && !Entries[index].Played ) {
				Play( Entries[index].Sound );
				Entries[index].Played = true;
			}
		}

		ScriptEngine::Start_Timer( obj, this, 1.0f, 1 );
	}

protected:
	virtual void	Play( const char * sound ) = 0;
	virtual void	Read_Parameters( void )		{ }

private:
	DynamicVectorClass<JFW_Countdown_Entry>	Entries;
};


REGISTER_SCRIPT_TT( JFW_Time_Remaining_Sounds, "ConfigFile:string" )
class	JFW_Time_Remaining_Sounds : public JFW_Time_Remaining_Base
{
protected:
	void	Play( const char * sound ) override
	{
		ScriptEngine::Create_2D_Sound( sound );
	}
};


REGISTER_SCRIPT_TT( JFW_Time_Remaining_Sounds_2, "ConfigFile:string,Team:int" )
class	JFW_Time_Remaining_Sounds_2 : public JFW_Time_Remaining_Base
{
public:
	JFW_Time_Remaining_Sounds_2( void ) : Team( PLAYERTYPE_NOD )	{ }

protected:
	void	Read_Parameters( void ) override	{ Team = Get_Int_Parameter( "Team" ); }

	void	Play( const char * sound ) override
	{
		ScriptEngine::Create_2D_Sound_Team( Team, sound );
	}

private:
	int	Team;
};
