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
*
* DESCRIPTION
*
* VERSION INFO
*     $Author: Darren_k $
*     $Revision: 25 $
*     $Modtime: 1/12/02 3:19p $
*     $Archive: /Commando/Code/Scripts/Test_DAK.cpp $
*
******************************************************************************/

#include <cstdint>
#include "Toolkit.h"

DECLARE_SCRIPT (DAK_TestScriptOne, "")
{
	void Damaged( GameObject *obj , GameObject *damager, float /*amount*/) override
	{
		if ( ScriptEngine::Is_A_Star(damager) )
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 1, 1 );
			params.Set_Movement( damager, RUN, 2 );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT (DAK_PlayerSpotted, "")
{
	void Damaged ( GameObject *obj , GameObject *damager, float /*amount*/) override
	{
		GameObject * NodSAM = ScriptEngine::Find_Object (100012);
		if (NodSAM)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 1, 1 );
			params.Set_Movement( NodSAM, RUN, 1 );
			ScriptEngine::Action_Goto( obj, params );
		}
		else
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 1, 1 );
			params.Set_Attack( damager, 20, 0, 1 );
			ScriptEngine::Action_Attack( obj, params );
		}
	}
};

DECLARE_SCRIPT ( DAK_Fire_Gas_Elec_Death_DAK, "DeathType:string" )
{
	bool firsttime; // prevents an infinante loop each time obj is damaged by DeathType.

	void Damaged ( GameObject *obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		// check to see if obj is at 25% or less of its health.
		if ( ScriptEngine::Get_Health ( obj ) <= 0.25 * ScriptEngine::Get_Max_Health ( obj ) )
		{
			// plays animation once.
			if ( firsttime == true )
			{
				firsttime = false;

				ActionParamsStruct params;
				params.Set_Basic( this, 99, 1 );

				// set animation based upon DeathType (Gas, Electric or Default to Fire)
				if ( strcmp( Get_Parameter( "DeathType" ), "Fire" ) )
				{
					params.Set_Animation( "S_A_HUMAN.H_A_FLMA",0 );	// fire death
				}
				else
					if ( strcmp( Get_Parameter( "DeathType" ), "Electic" ) )
					{
						params.Set_Animation( "S_A_HUMAN.H_A_6X05",0 ); // electric death
					}
					else params.Set_Animation( "S_A_HUMAN.H_A_FLMA",0 ); // gas death

				ScriptEngine::Action_Play_Animation( obj, params );

				// begin DeathType damage
				ScriptEngine::Apply_Damage( obj, 1.0f, Get_Parameter( "DeathType" ), nullptr );
			}
		}
		else
		{
			firsttime = true;
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if((action_id == 1) && (reason == ACTION_COMPLETE_NORMAL))
		{
			// animation is complete. kill obj.
			ScriptEngine::Apply_Damage (obj, 10000.0f, "Blamokiller", nullptr);
		}

	}
};

DECLARE_SCRIPT(DAK_Vehicle_Regen_DAK, "" )
{
	void Created ( GameObject *obj ) override
	{
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, 0, 0 );
	}

	void Custom (GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if ( type == 0 ) // regenerate health.
		{
			// check to see if health needs to be regenerated.
			if ( ScriptEngine::Get_Health ( obj ) < ScriptEngine::Get_Max_Health ( obj ) )
			{
				ScriptEngine::Apply_Damage (obj, -10, "RegenHealth", nullptr);
			}
			// restart the timer
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, 0, 5 );
		}
	}
};

DECLARE_SCRIPT(DAK_Electric_Death_DAK, "" )
{
	void Created ( GameObject *obj ) override
	{
		// TODO Why is this casting like this to round rather than calling Get_Random_Int?
		int time = int(ScriptEngine::Get_Random(1, 3));
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, 0, float(time) );
	}

	void Damaged (GameObject *obj, GameObject * /*damager*/, float /*amount*/ ) override
	{
		ScriptEngine::Send_Custom_Event ( obj, obj, 1, 0, 1 ); // wait a second before applying next ammount of damage.
	}

	void Custom ( GameObject *obj, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == 0 ) // create next soldier, attach script, kill yourself with electric damage.
		{
			Vector3 position = ScriptEngine::Get_Position (obj);
			position.X += ScriptEngine::Get_Random(-3, 3);
			position.Y += ScriptEngine::Get_Random(-3, 3);
			GameObject *new_object = ScriptEngine::Create_Object( "Nod_Minigunner_0_Def", position);

			ScriptEngine::Attach_Script (new_object, "DAK_Electric_Death_DAK", "");

			ScriptEngine::Apply_Damage (obj, 10, "Electric", nullptr);
		}

		if ( type == 1 ) // apply next ammount of electric damage.
		{
			ScriptEngine::Apply_Damage (obj, 10, "Electric", nullptr);
		}
	}
};

DECLARE_SCRIPT(DAK_PCT_Pokable_DAK, "" )
{
	void Created ( GameObject *obj ) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator( obj, true );
//		ScriptEngine::Display_Health_Bar( obj, false );
	}
};


DECLARE_SCRIPT( M00_BUILDING_EXPLODE_NO_DAMAGE_DAK, "" )
{
	void Killed( GameObject *obj, GameObject * /*killer*/ ) override
	{
		//ScriptEngine::Create_Explosion ( "Building_Explode_No_Damage", position, nullptr );
		ScriptEngine::Shake_Camera( ScriptEngine::Get_Position( obj ), 25, 0.1f, 4.0f );
	}
};
