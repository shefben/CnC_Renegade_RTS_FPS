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
*     Toolkit.cpp
*
* DESCRIPTION
*     Designer Toolkit for Mission Construction - Debug and Notation Subset
*
* PROGRAMMER
*     Design Team
*
* VERSION INFO
*     $Author: Dan_e $
*     $Revision: 185 $
*     $Modtime: 3/15/02 12:53p $
*     $Archive: /Commando/Code/Scripts/Toolkit.cpp $
*
******************************************************************************/

#include <cstdint>
#include "Toolkit.h"
#include "Mission1.h"

//
// M00_Advanced_Guard_Tower reads the tower's facing and height off its MCT: the
// building controller itself is not a phys object and carries neither.
//
#include "building.h"
#include "buildingaggregate.h"

/* GENERAL TOOLKIT INFORMATION

  1. All toolkit systems are listed in categories of system type: Actions, Explosions, etc.
  2. All toolkit systems are sub-categorized in the order of functions, then interfaces.
  3. The following Parameter Standardization is used for all scripts, and is not repeated.
  Each script has the appropriate parameters from this list, always in this order:

  SCRIPT PARAMETER	DESCRIPTION									SCRIPT TYPES

  Start_Now			Set this to 1 to activate upon creation.	Triggers and Controllers
  Receive_Type		Custom expected for activation/deactivation	Triggers and Controllers
  Receive_Param_On	Custom Parameter to activate this script	Triggers and Controllers
  Receive_Param_Off	Custom Parameter to deactivate this script	Triggers and Controllers

  Target_ID			The ID of the object to receive a custom.	Triggers Only
  Send_Type			The Custom Type to send to the object.		Triggers Only
  Send_Param		The Custom Paramter to send to the object.	Triggers Only
  Min_Delay			The minimum wait time for the custom.		Triggers Only
  Max_Delay			The maximum wait time for the custom.		Triggers Only

  Action_Priority	Priority level of the action applied.		Action Controllers Only
  Action_ID			The ID of the action applied.				Action Controllers Only

  (varies)			Various required parameters for the script	Triggers and Controllers

  Debug_Mode		Set this to 1 for debug information.		Triggers and Controllers
*/

/******************************************************************************
*
*	Toolkit Script Debuggers
*
******************************************************************************/

DECLARE_SCRIPT(M00_Debug_Text_File_RMV, "Description=Object:string, Filename=DebugLog.txt:string")
{
	const char *filename, *desc;
	FILE *file;
	time_t start_time, current_time;

	const char* Reason_Lookup(ActionCompleteReason reason)
	{
		static constexpr const char* reasons[] = {
			"ACTION_COMPLETE_NORMAL",
			"ACTION_COMPLETE_LOW_PRIORITY",
			"ACTION_COMPLETE_PATH_BAD_START",
			"ACTION_COMPLETE_PATH_BAD_DEST",
			"ACTION_COMPLETE_MOVE_NO_PROGRESS_MADE",
			"ACTION_COMPLETE_ATTACK_OUT_OF_RANGE"
		};
		return reasons[(int)reason];
	}

	void Created(GameObject * obj) override
	{
		current_time = time(nullptr);
		desc = Get_Parameter("Description");
		filename = Get_Parameter("Filename");
		file = fopen(filename, "wt");

		fprintf(file, "%s [ID %d] created.\n", desc, ScriptEngine::Get_ID(obj));
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		current_time = time(nullptr);
		fprintf(file, "%s [ID %d] received custom event of type %d and param %d.  Sender was object %d.   %3.1f sec.\n", desc, ScriptEngine::Get_ID(obj), type, (int)param, ScriptEngine::Get_ID(sender), difftime(current_time, start_time));
	}

	void Damaged(GameObject * obj, GameObject * damager, float /*amount*/) override
	{
		current_time = time(nullptr);
		fprintf(file, "%s [ID %d] damaged by object %d.   %3.1f sec.\n", desc, ScriptEngine::Get_ID(obj), ScriptEngine::Get_ID(damager), difftime(current_time, start_time));
	}

	void Sound_Heard( GameObject * /*obj*/, const CombatSound & /*sound*/ ) override
	{
	//	fprintf(file, "%s [ID %d] heard a sound.   %3.1f sec.\n", desc, ScriptEngine::Get_ID(obj), current_time);
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy) override
	{
		current_time = time(nullptr);
		fprintf(file, "%s [ID %d] saw enemy: object %d.   %3.1f sec.\n", desc, ScriptEngine::Get_ID(obj), ScriptEngine::Get_ID(enemy), difftime(current_time, start_time));
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		current_time = time(nullptr);
		fprintf(file, "Action %d complete on %s [ID %d] -- Reason: %s.   %3.1f sec.\n", action_id, desc, ScriptEngine::Get_ID(obj), Reason_Lookup(reason), difftime(current_time, start_time));
	}

	void Killed(GameObject * obj, GameObject * killer) override
	{
		current_time = time(nullptr);
		fprintf(file, "%s [ID %d] killed by object %d.   %3.1f sec.\n", desc, ScriptEngine::Get_ID(obj), ScriptEngine::Get_ID(killer), difftime(current_time, start_time));
	}

	void Destroyed(GameObject * /*obj*/) override
	{
		fclose(file);
	}
};

/******************************************************************************
*
*	Other Scripts
*
******************************************************************************/

#if 0

DECLARE_SCRIPT(M00_Activate_Weapon_At_Object_On_Frame_RMV, "Frame:int, Primary_Weapon=1:int, Duration:float, Target_ID:int, Start_Timer_ID:int, Stop_Timer_ID")
{
	float frame;
	bool primary;
	float duration;
	int start_id;
	int stop_id;
	int target_id;
	GameObject *target;

	void Created(GameObject * obj) override
	{
		frame = (float)Get_Int_Parameter("Frame");
		primary = (Get_Int_Parameter("Primary_Weapon") == 1) ? true : false;
		duration = Get_Float_Parameter("Duration");
		start_id = Get_Int_Parameter("Start_Timer_ID");
		stop_id = Get_Int_Parameter("Stop_Timer_ID");
		target_id = Get_Int_Parameter("Target_ID");
		target = ScriptEngine::Get_A_Star(Vector3(0.0f,0.0f,0.0f));
		if (target_id > 0)
			target = ScriptEngine::Find_Object(target_id);
		ScriptEngine::Start_Timer(obj, this, frame/30, start_id);
	}

	void Timer_Expired(GameObject * obj, int timer_id)
	{
		if (timer_id == start_id)
		{
			Vector3 position = ScriptEngine::Get_Position(target);
			ScriptEngine::Trigger_Weapon(obj, true, position, primary);
			ScriptEngine::Start_Timer(obj, this, duration, stop_id);
		}
		if (timer_id == stop_id)
		{
			Vector3 position = ScriptEngine::Get_Position(target);
			ScriptEngine::Trigger_Weapon(obj, false, position, primary);
		}
	}
};

DECLARE_SCRIPT(M00_Activate_Weapon_At_Object_On_Timer_RMV, "Delay:float, Primary_Weapon=1:int, Duration:float, Target_ID:int, Start_Timer_ID:int, Stop_Timer_ID:int")
{
	float time;
	bool primary;
	float duration;
	int start_id;
	int stop_id;
	int target_id;
	GameObject *target;

	void Created(GameObject * obj) override
	{
		time = Get_Float_Parameter("Delay");
		primary = (Get_Int_Parameter("Primary_Weapon") == 1) ? true : false;
		duration = Get_Float_Parameter("Duration");
		start_id = Get_Int_Parameter("Start_Timer_ID");
		stop_id = Get_Int_Parameter("Stop_Timer_ID");
		target_id = Get_Int_Parameter("Target_ID");
		target = ScriptEngine::Get_A_Star(Vector3(0.0f,0.0f,0.0f));
		if (target_id > 0)
			target = ScriptEngine::Find_Object(target_id);
		ScriptEngine::Start_Timer(obj, this, time, start_id);
	}

	void Timer_Expired(GameObject * obj, int timer_id)
	{
		if (timer_id == start_id)
		{
			Vector3 position = ScriptEngine::Get_Position(target);
			ScriptEngine::Trigger_Weapon(obj, true, position, primary);
			ScriptEngine::Start_Timer(obj, this, duration, stop_id);
		}
		if (timer_id == stop_id)
		{
			Vector3 position = ScriptEngine::Get_Position(target);
			ScriptEngine::Trigger_Weapon(obj, false, position, primary);
		}
	}
};

DECLARE_SCRIPT(M00_Activate_Weapon_At_Location_On_Frame_RMV, "Frame:int, Primary_Weapon=1:int, Duration:float, Target:vector3, Start_Timer_ID:int, Stop_Timer_ID")
{
	float frame;
	bool primary;
	float duration;
	int start_id;
	int stop_id;
	Vector3 target;

	void Created(GameObject * obj) override
	{
		frame = (float)Get_Int_Parameter("Frame");
		primary = (Get_Int_Parameter("Primary_Weapon") == 1) ? true : false;
		duration = Get_Float_Parameter("Duration");
		start_id = Get_Int_Parameter("Start_Timer_ID");
		stop_id = Get_Int_Parameter("Stop_Timer_ID");
		target = Get_Vector3_Parameter("Target");
		ScriptEngine::Start_Timer(obj, this, frame/30, start_id);
	}

	void Timer_Expired(GameObject * obj, int timer_id)
	{
		if (timer_id == start_id)
		{
			ScriptEngine::Trigger_Weapon(obj, true, target, primary);
			ScriptEngine::Start_Timer(obj, this, duration, stop_id);
		}
		if (timer_id == stop_id)
		{
			ScriptEngine::Trigger_Weapon(obj, false, target, primary);
		}
	}
};

DECLARE_SCRIPT(M00_Activate_Weapon_At_Location_On_Timer_RMV, "Delay:float, Primary_Weapon=1:int, Duration:float, Target:vector3, Start_Timer_ID:int, Stop_Timer_ID:int")
{
	float time;
	bool primary;
	float duration;
	int start_id;
	int stop_id;
	Vector3 target;

	void Created(GameObject * obj) override
	{
		time = Get_Float_Parameter("Delay");
		primary = (Get_Int_Parameter("Primary_Weapon") == 1) ? true : false;
		duration = Get_Float_Parameter("Duration");
		start_id = Get_Int_Parameter("Start_Timer_ID");
		stop_id = Get_Int_Parameter("Stop_Timer_ID");
		target = Get_Vector3_Parameter("Target");
		ScriptEngine::Start_Timer(obj, this, time, start_id);
	}

	void Timer_Expired(GameObject * obj, int timer_id) override
	{
		if (timer_id == start_id)
		{
			ScriptEngine::Trigger_Weapon(obj, true, target, primary);
			ScriptEngine::Start_Timer(obj, this, duration, stop_id);
		}
		if (timer_id == stop_id)
		{
			ScriptEngine::Trigger_Weapon(obj, false, target, primary);
		}
	}
};

#endif

DECLARE_SCRIPT(M00_Disable_Physical_Collision_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Disable_Physical_Collisions ( obj );
	}
};

DECLARE_SCRIPT(M00_Enable_Physical_Collision_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Collisions ( obj );
	}
};

DECLARE_SCRIPT(M00_C130_Explosion, "")
{
	void Killed(GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Create_Explosion_At_Bone("Explosion_Cargo_Plane", obj, "BODYMAIN", nullptr);
		//JDG updated this script due to explosion preset name change--07/27/2001
	}
};

DECLARE_SCRIPT(M00_Monitor_Attached_Primary, "")
{

	bool object_detached;
	int attached_object_id;

	enum{OBJECT_DETACHED};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( object_detached, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		object_detached = false;
		attached_object_id = 0;
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		ActionParamsStruct params;
		if(type == M00_CUSTOM_OBJECT_ATTACHED_PRIMARY)
		{
			attached_object_id = ScriptEngine::Get_ID(sender);
			ScriptEngine::Start_Timer (obj, this, param / 30.0f, OBJECT_DETACHED);
		}

	}

	void Timer_Expired(GameObject * /*obj*/, int timer_id) override
	{
		if (timer_id == OBJECT_DETACHED)
		{
			object_detached = true;
		}

	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if(!object_detached)
		{
			ScriptEngine::Apply_Damage( ScriptEngine::Find_Object(attached_object_id), 10000.0f, "DEATH", nullptr);
		}
	}

};

/***********************************************************************************************************
The two following scripts are a controller and a script for a gerneric barrel explosion chain rection.
You only need to place the controller in your level and enter the Vector3's and barrel types as editor parameters
See Joe G for further details
****************************************************************************************************************/

DECLARE_SCRIPT(M00_5MetalBarrels_ChainRxn_Controller_JDG, "Barrel01_Location:vector3, Barrel01_Type (1-8):int, Barrel01_Facing = 0:float, Barrel02_Location:vector3, Barrel02_Type (1-8):int, Barrel02_Facing = 0:float, Barrel03_Location:vector3, Barrel03_Type (1-8):int, Barrel03_Facing = 0:float, Barrel04_Location:vector3, Barrel04_Type (1-8):int, Barrel04_Facing = 0:float, Barrel05_Location:vector3, Barrel05_Type (1-8):int, Barrel05_Facing = 0:float")
{
	Vector3 barrel_01_location;
	Vector3 barrel_02_location;
	Vector3 barrel_03_location;
	Vector3 barrel_04_location;
	Vector3 barrel_05_location;

	int barrel_01_type;
	int barrel_02_type;
	int barrel_03_type;
	int barrel_04_type;
	int barrel_05_type;

	int barrel_01_id;
	int barrel_02_id;
	int barrel_03_id;
	int barrel_04_id;
	int barrel_05_id;

	float barrel_01_facing;
	float barrel_02_facing;
	float barrel_03_facing;
	float barrel_04_facing;
	float barrel_05_facing;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(barrel_01_location, 1);
		SAVE_VARIABLE(barrel_02_location, 2);
		SAVE_VARIABLE(barrel_03_location, 3);
		SAVE_VARIABLE(barrel_04_location, 4);
		SAVE_VARIABLE(barrel_05_location, 5);
		SAVE_VARIABLE(barrel_01_type, 6);
		SAVE_VARIABLE(barrel_02_type, 7);
		SAVE_VARIABLE(barrel_03_type, 8);
		SAVE_VARIABLE(barrel_04_type, 9);
		SAVE_VARIABLE(barrel_05_type, 10);
		SAVE_VARIABLE(barrel_01_id, 11);
		SAVE_VARIABLE(barrel_02_id, 12);
		SAVE_VARIABLE(barrel_03_id, 13);
		SAVE_VARIABLE(barrel_04_id, 14);
		SAVE_VARIABLE(barrel_05_id, 15);
		SAVE_VARIABLE(barrel_01_facing, 16);
		SAVE_VARIABLE(barrel_02_facing, 17);
		SAVE_VARIABLE(barrel_03_facing, 18);
		SAVE_VARIABLE(barrel_04_facing, 19);
		SAVE_VARIABLE(barrel_05_facing, 20);
	}

	void Created( GameObject * obj ) override
	{
		barrel_01_location = Get_Vector3_Parameter("Barrel01_Location");
		barrel_02_location = Get_Vector3_Parameter("Barrel02_Location");
		barrel_03_location = Get_Vector3_Parameter("Barrel03_Location");
		barrel_04_location = Get_Vector3_Parameter("Barrel04_Location");
		barrel_05_location = Get_Vector3_Parameter("Barrel05_Location");
		barrel_01_type = Get_Int_Parameter("Barrel01_Type (1-8)");
		barrel_01_type -= 1;
		barrel_02_type = Get_Int_Parameter("Barrel02_Type (1-8)");
		barrel_02_type -= 1;
		barrel_03_type = Get_Int_Parameter("Barrel03_Type (1-8)");
		barrel_03_type -= 1;
		barrel_04_type = Get_Int_Parameter("Barrel04_Type (1-8)");
		barrel_04_type -= 1;
		barrel_05_type = Get_Int_Parameter("Barrel05_Type (1-8)");
		barrel_05_type -= 1;
		barrel_01_facing = Get_Float_Parameter("Barrel01_Facing");
		barrel_02_facing = Get_Float_Parameter("Barrel02_Facing");
		barrel_03_facing = Get_Float_Parameter("Barrel03_Facing");
		barrel_04_facing = Get_Float_Parameter("Barrel04_Facing");
		barrel_05_facing = Get_Float_Parameter("Barrel05_Facing");

		static constexpr const char* simple_barrels[8] =
		{
			"Simple_MetalDrum_01",
			"Simple_MetalDrum_02",
			"Simple_MetalDrum_03",
			"Simple_MetalDrum_04",
			"Simple_MetalDrum_05",
			"Simple_MetalDrum_06",
			"Simple_MetalDrum_07",
			"Simple_MetalDrum_08",
		};

		static constexpr const char* barrels_animations[8] =
		{
			"DSP_METDRUM01.DSP_METDRUM01",
			"DSP_METDRUM02.DSP_METDRUM02",
			"DSP_METDRUM03.DSP_METDRUM03",
			"DSP_METDRUM04.DSP_METDRUM04",
			"DSP_METDRUM05.DSP_METDRUM05",
			"DSP_METDRUM06.DSP_METDRUM06",
			"DSP_METDRUM07.DSP_METDRUM07",
			"DSP_METDRUM08.DSP_METDRUM08",
		};

		int controller_id = ScriptEngine::Get_ID ( obj );
		char controller[16];
		sprintf (controller, "%d", controller_id);

		GameObject * barrel_01 = ScriptEngine::Create_Object ( simple_barrels[barrel_01_type], barrel_01_location );
		ScriptEngine::Set_Animation_Frame ( barrel_01, barrels_animations[barrel_01_type], 0 );
		barrel_01_id = ScriptEngine::Get_ID ( barrel_01 );
		ScriptEngine::Set_Facing ( barrel_01, barrel_01_facing );
		ScriptEngine::Attach_Script(barrel_01, "M00_ChainRxn_Barrel_JDG", controller);

		GameObject * barrel_02 = ScriptEngine::Create_Object ( simple_barrels[barrel_02_type], barrel_02_location );
		ScriptEngine::Set_Animation_Frame ( barrel_02, barrels_animations[barrel_02_type ], 0 );
		barrel_02_id = ScriptEngine::Get_ID ( barrel_02 );
		ScriptEngine::Set_Facing ( barrel_02, barrel_02_facing );
		ScriptEngine::Attach_Script(barrel_02, "M00_ChainRxn_Barrel_JDG", controller);

		GameObject * barrel_03 = ScriptEngine::Create_Object ( simple_barrels[barrel_03_type], barrel_03_location );
		ScriptEngine::Set_Animation_Frame ( barrel_03, barrels_animations[barrel_03_type], 0 );
		barrel_03_id = ScriptEngine::Get_ID ( barrel_03 );
		ScriptEngine::Set_Facing ( barrel_03, barrel_03_facing );
		ScriptEngine::Attach_Script(barrel_03, "M00_ChainRxn_Barrel_JDG", controller);

		GameObject * barrel_04 = ScriptEngine::Create_Object ( simple_barrels[barrel_04_type], barrel_04_location );
		ScriptEngine::Set_Animation_Frame ( barrel_04, barrels_animations[barrel_04_type], 0 );
		barrel_04_id = ScriptEngine::Get_ID ( barrel_04 );
		ScriptEngine::Set_Facing ( barrel_04, barrel_04_facing );
		ScriptEngine::Attach_Script(barrel_04, "M00_ChainRxn_Barrel_JDG", controller);

		GameObject * barrel_05 = ScriptEngine::Create_Object ( simple_barrels[barrel_05_type], barrel_05_location );
		ScriptEngine::Set_Animation_Frame ( barrel_05, barrels_animations[barrel_05_type], 0 );
		barrel_05_id = ScriptEngine::Get_ID ( barrel_05 );
		ScriptEngine::Set_Facing ( barrel_05, barrel_05_facing );
		ScriptEngine::Attach_Script(barrel_05, "M00_ChainRxn_Barrel_JDG", controller);
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)
			{
				GameObject * barrel_01 = ScriptEngine::Find_Object ( barrel_01_id );
				GameObject * barrel_02 = ScriptEngine::Find_Object ( barrel_02_id );
				GameObject * barrel_03 = ScriptEngine::Find_Object ( barrel_03_id );
				GameObject * barrel_04 = ScriptEngine::Find_Object ( barrel_04_id );
				GameObject * barrel_05 = ScriptEngine::Find_Object ( barrel_05_id );

				if (sender == barrel_01)//barrel 01 has been destroyed--tell #2 to self destruct
				{
					ScriptEngine::Send_Custom_Event ( obj, barrel_01, barrel_01_type, M01_MODIFY_YOUR_ACTION_JDG, 0 );

					if (barrel_02 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_02, barrel_02_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}
				}

				else if (sender == barrel_02)//barrel 02 has been destroyed--tell #1 and #3 to self destruct
				{
					ScriptEngine::Send_Custom_Event ( obj, barrel_02, barrel_02_type, M01_MODIFY_YOUR_ACTION_JDG, 0 );

					if (barrel_01 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_01, barrel_01_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}

					if (barrel_03 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_03, barrel_03_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}
				}

				else if (sender == barrel_03)//barrel 03 has been destroyed--tell #2 and #4 to self destruct
				{
					ScriptEngine::Send_Custom_Event ( obj, barrel_03, barrel_03_type, M01_MODIFY_YOUR_ACTION_JDG, 0 );

					if (barrel_02 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_02, barrel_02_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}

					if (barrel_04 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_04, barrel_04_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}
				}

				else if (sender == barrel_04)//barrel 04 has been destroyed--tell #3 and #5 to self destruct
				{
					ScriptEngine::Send_Custom_Event ( obj, barrel_04, barrel_04_type, M01_MODIFY_YOUR_ACTION_JDG, 0 );

					if (barrel_03 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_03, barrel_03_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}

					if (barrel_05 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_05, barrel_05_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}
				}

				else if (sender == barrel_05)//barrel 05 has been destroyed--tell #4 to self destruct
				{
					ScriptEngine::Send_Custom_Event ( obj, barrel_05, barrel_05_type, M01_MODIFY_YOUR_ACTION_JDG, 0 );

					if (barrel_04 != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0.25f, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, barrel_04, barrel_04_type, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}
				}
			}
		}
	}
};

DECLARE_SCRIPT(M00_ChainRxn_Barrel_JDG, "Controller_ID :int")
{
	int controller_id;
	bool deadYet;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(controller_id, 1);
		SAVE_VARIABLE(deadYet, 2);
	}

	void Created( GameObject * /*obj*/ ) override
	{
		deadYet = false;
		controller_id = Get_Int_Parameter("Controller_ID");
	}

	void Damaged( GameObject * obj, GameObject * damager, float /*amount*/ ) override
	{
		if (obj && deadYet == false && damager == STAR)
		{
			ScriptEngine::Set_Health ( obj, 0.25f );

			GameObject * controller = ScriptEngine::Find_Object ( controller_id );
			if (controller != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, controller, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
			}
		}

		else
		{
			ScriptEngine::Set_Health ( obj, 0.25f );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /*sender*/ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG && deadYet == false)//you've been ordered to blow up--DO IT!
		{
			deadYet = true;
			int number = type;
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Explosion ( "Explosion_Mine_Remote_01", myPosition, nullptr );

			static constexpr const char* barrels_animations[8] =
			{
				"DSP_METDRUM01.DSP_METDRUM01",//there's no #1 yet
				"DSP_METDRUM02.DSP_METDRUM02",
				"DSP_METDRUM03.DSP_METDRUM03",
				"DSP_METDRUM04.DSP_METDRUM04",
				"DSP_METDRUM05.DSP_METDRUM05",
				"DSP_METDRUM06.DSP_METDRUM06",//there's no #6 yet
				"DSP_METDRUM07.DSP_METDRUM07",
				"DSP_METDRUM08.DSP_METDRUM08",
			};

			int barrels_endframe[8] =
			{
				28,//1
				15,//2
				20,//3
				8,//4
				8,//5
				27,//6
				8,//7
				8,//8
			};

			ScriptEngine::Set_Animation ( obj, barrels_animations[number], false, nullptr, 0, float(barrels_endframe[number]), false );
			ScriptEngine::Set_Health ( obj, 0.25f );

			GameObject * controller = ScriptEngine::Find_Object ( controller_id );
			if (controller != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, controller, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
			}
		}
	}
};


/* M00_Advanced_Guard_Tower

  This script handles the functionality of the Advanced Guard Tower in both single and multiplayer
  environments. Attach this script to the AGT Building Controller, and center the building
  controller directly on the top of the tower roof.

  When created this script:
  - Creates four guard tower machine guns at the corners of the building
  - Attaches another script to the machine guns to allow them enemy_seen capability.
  - Creates a missile object at the top of the building.
  - Attaches another script to the missile object to allow it to respond to the guns.

  - The guns use enemy_seen to spot, and inform the missile when it is time to fire.

*/

DECLARE_SCRIPT (M00_Advanced_Guard_Tower, "")
{
	int missile_id;
	int gun_01_id;
	int gun_02_id;
	int gun_03_id;
	int gun_04_id;
	bool can_fire;

	//Adding unneeded save/load variables just to be safe -- 2/12/2002 JDG.
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(missile_id, 1);
		SAVE_VARIABLE(gun_01_id, 2);
		SAVE_VARIABLE(gun_02_id, 3);
		SAVE_VARIABLE(gun_03_id, 4);
		SAVE_VARIABLE(gun_04_id, 5);
		SAVE_VARIABLE(can_fire, 6);
	}

	void Created (GameObject* obj) override
	{
		can_fire = true;
		ScriptEngine::Enable_Hibernation (obj, false);
		missile_id = 0;
		gun_01_id = 0;
		gun_02_id = 0;
		gun_03_id = 0;
		gun_04_id = 0;

		// Create the four guns associated with this object.

		Vector3 my_location = ScriptEngine::Get_Position (obj);
		Vector3 missile_loc = my_location;

		missile_loc.Z += 4.0f;

		//
		// The four gun mounts are corners of the tower, so their offsets are in the
		// building's own frame and have to be rotated into the world by its facing.
		// Placing them unrotated only ever worked for a tower sitting at facing zero.
		//
		// The facing lives on the MCT, not on the controller: BuildingGameObj is a
		// DamageableGameObj, so it has no transform of its own.
		//
		float facing = 0.0f;

		BuildingGameObj * building = obj->As_BuildingGameObj ();
		BuildingAggregateClass * mct = (building != nullptr) ? building->Find_MCT () : nullptr;
		if (mct != nullptr)
		{
			const Matrix3D & mct_tm = mct->Get_Transform ();
			facing = mct_tm.Get_Z_Rotation ();

			//
			// A controller placed level with the roof instead of above it puts the guns
			// through the floor. Measure from the MCT when that has happened.
			//
			const float mct_height = mct_tm.Get_Translation ().Z + 8.0f;
			if (my_location.Z - mct_height < 11.0f)
			{
				my_location.Z = mct_height + 9.2f;
				missile_loc = my_location;
				missile_loc.Z += 4.0f;
			}
		}

		const float cos_facing = cosf (facing);
		const float sin_facing = sinf (facing);

		static const float GUN_OFFSET[4][2] = {
			{  5.019f, -3.409f },
			{  5.011f,  3.672f },
			{ -5.011f,  3.657f },
			{ -5.013f, -3.406f },
		};

		Vector3 gun_pos[4];
		for (int gun_index = 0; gun_index < 4; gun_index++)
		{
			const float local_x = GUN_OFFSET[gun_index][0];
			const float local_y = GUN_OFFSET[gun_index][1];

			gun_pos[gun_index].X = my_location.X + (local_x * cos_facing) - (local_y * sin_facing);
			gun_pos[gun_index].Y = my_location.Y + (local_x * sin_facing) + (local_y * cos_facing);
			gun_pos[gun_index].Z = my_location.Z - 9.0f;
		}

		Vector3 gun_01_pos = gun_pos[0];
		Vector3 gun_02_pos = gun_pos[1];
		Vector3 gun_03_pos = gun_pos[2];
		Vector3 gun_04_pos = gun_pos[3];

		GameObject * missile_object = ScriptEngine::Create_Object ("GDI_AGT", missile_loc);
		if (missile_object)
		{
			ScriptEngine::Attach_Script (missile_object, "M00_Advanced_Guard_Tower_Missile", "");
			missile_id = ScriptEngine::Get_ID (missile_object);
		}
		GameObject * gun_01 = ScriptEngine::Create_Object ("GDI_Ceiling_Gun_AGT", gun_01_pos);
		if (gun_01)
		{
			ScriptEngine::Attach_Script (gun_01, "M00_Advanced_Guard_Tower_Gun", "");
			gun_01_id = ScriptEngine::Get_ID (gun_01);
		}
		GameObject * gun_02 = ScriptEngine::Create_Object ("GDI_Ceiling_Gun_AGT", gun_02_pos);
		if (gun_02)
		{
			ScriptEngine::Attach_Script (gun_02, "M00_Advanced_Guard_Tower_Gun", "");
			gun_02_id = ScriptEngine::Get_ID (gun_02);
		}
		GameObject * gun_03 = ScriptEngine::Create_Object ("GDI_Ceiling_Gun_AGT", gun_03_pos);
		if (gun_03)
		{
			ScriptEngine::Attach_Script (gun_03, "M00_Advanced_Guard_Tower_Gun", "");
			gun_03_id = ScriptEngine::Get_ID (gun_03);
		}
		GameObject * gun_04 = ScriptEngine::Create_Object ("GDI_Ceiling_Gun_AGT", gun_04_pos);
		if (gun_04)
		{
			ScriptEngine::Attach_Script (gun_04, "M00_Advanced_Guard_Tower_Gun", "");
			gun_04_id = ScriptEngine::Get_ID (gun_04);
		}
		ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
		ScriptEngine::Start_Timer (obj, this, 1.0f, 2);
	}

	void Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		//telling AGT guns that AGT has been killed (2/12/2002 JDG)
		GameObject * gun_01 = ScriptEngine::Find_Object (gun_01_id);
		GameObject * gun_02 = ScriptEngine::Find_Object (gun_02_id);
		GameObject * gun_03 = ScriptEngine::Find_Object (gun_03_id);
		GameObject * gun_04 = ScriptEngine::Find_Object (gun_04_id);

		if (gun_01 != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, gun_01, 3, 0, 0);//tells gun 01 to reset action and lets him know AGT is dead -- JDG 2/12/02
		}

		if (gun_02 != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, gun_02, 3, 0, 0);//tells gun 02 to reset action and lets him know AGT is dead -- JDG 2/12/02
		}

		if (gun_03 != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, gun_03, 3, 0, 0);//tells gun 03 to reset action and lets him know AGT is dead -- JDG 2/12/02
		}

		if (gun_04 != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, gun_04, 3, 0, 0);//tells gun 04 to reset action and lets him know AGT is dead -- JDG 2/12/02
		}

		//
		// Telling them the tower is dead is not enough -- nothing else ever removed them,
		// so a destroyed tower left four guns and a missile emitter standing in the air.
		//
		ScriptEngine::Destroy_Object (gun_01);
		ScriptEngine::Destroy_Object (gun_02);
		ScriptEngine::Destroy_Object (gun_03);
		ScriptEngine::Destroy_Object (gun_04);
		ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (missile_id));
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		GameObject * gun_01 = ScriptEngine::Find_Object (gun_01_id);
		GameObject * gun_02 = ScriptEngine::Find_Object (gun_02_id);
		GameObject * gun_03 = ScriptEngine::Find_Object (gun_03_id);
		GameObject * gun_04 = ScriptEngine::Find_Object (gun_04_id);
		if (timer_id == 1)
		{
			if (gun_01)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_01, 1, missile_id, 0.0f);
			}
			if (gun_02)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_02, 1, missile_id, 0.0f);
			}
			if (gun_03)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_03, 1, missile_id, 0.0f);
			}
			if (gun_04)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_04, 1, missile_id, 0.0f);
			}
		}
		else if (timer_id == 2)
		{
			float health = ScriptEngine::Get_Health (obj);
			bool power = ScriptEngine::Get_Building_Power (obj);
			if ((!health) || (!power))
			{
				if (gun_01)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_01, 2, 0, 0.0f);
				}
				if (gun_02)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_02, 2, 0, 0.0f);
				}
				if (gun_03)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_03, 2, 0, 0.0f);
				}
				if (gun_04)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_04, 2, 0, 0.0f);
				}
			}
			else if ((power) && (can_fire))
			{
				if (gun_01)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_01, 2, 1, 0.0f);
				}
				if (gun_02)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_02, 2, 1, 0.0f);
				}
				if (gun_03)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_03, 2, 1, 0.0f);
				}
				if (gun_04)
				{
					ScriptEngine::Send_Custom_Event (obj, gun_04, 2, 1, 0.0f);
				}
			}
			ScriptEngine::Start_Timer (obj, this, 1.0f, 2);
		}
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		GameObject * gun_01 = ScriptEngine::Find_Object (gun_01_id);
		GameObject * gun_02 = ScriptEngine::Find_Object (gun_02_id);
		GameObject * gun_03 = ScriptEngine::Find_Object (gun_03_id);
		GameObject * gun_04 = ScriptEngine::Find_Object (gun_04_id);
		if (type == 1)
		{
			can_fire = true;
			if (gun_01)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_01, 2, 1, 0.0f);
			}
			if (gun_02)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_02, 2, 1, 0.0f);
			}
			if (gun_03)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_03, 2, 1, 0.0f);
			}
			if (gun_04)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_04, 2, 1, 0.0f);
			}
		}
		else if (!type)
		{
			can_fire = false;
			if (gun_01)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_01, 2, 0, 0.0f);
			}
			if (gun_02)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_02, 2, 0, 0.0f);
			}
			if (gun_03)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_03, 2, 0, 0.0f);
			}
			if (gun_04)
			{
				ScriptEngine::Send_Custom_Event (obj, gun_04, 2, 0, 0.0f);
			}
		}
	}
};


DECLARE_SCRIPT (M00_Advanced_Guard_Tower_Gun, "")
{
	int missile_object;
	bool agt_is_dead;

	//Adding unneeded save/load variables just to be safe -- 2/12/2002 JDG.
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(missile_object, 1);
		SAVE_VARIABLE(agt_is_dead, 2);
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Shield_Type (obj, "Blamo");
		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);
		ScriptEngine::Enable_Enemy_Seen (obj, true);
		missile_object = 0;
		agt_is_dead = false;
	}

	void Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{
		if (agt_is_dead == false)
		{
			GameObject * missile_obj = ScriptEngine::Find_Object (missile_object);
			if (missile_obj)
			{
				int enemy_id = ScriptEngine::Get_ID (enemy);
				ScriptEngine::Send_Custom_Event (obj, missile_obj, 1, enemy_id, 0.0f);
			}
			Vector3 my_loc = ScriptEngine::Get_Position (obj);
			Vector3 enemy_loc = ScriptEngine::Get_Position (enemy);
			float distance = ScriptEngine::Get_Distance (my_loc, enemy_loc);
			if (distance > 20.0f)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 100, 1);
				params.Set_Attack(enemy, 300.0f, 0.0f, true);
				params.AttackCheckBlocked = false;
				ScriptEngine::Action_Attack(obj, params);
				ScriptEngine::Start_Timer (obj, this, 10.0f, 1);
			}
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == 1)
		{
			missile_object = param;
		}
		else if (type == 2)
		{
			if (param == 0)
			{
				ScriptEngine::Enable_Enemy_Seen (obj, false);
			}
			else
			{
				ScriptEngine::Enable_Enemy_Seen (obj, true);
			}
		}

		else if (type == 3)//AGT has been killed...reset your action and prevent further firing -- 02/12/2002 JDG
		{
			agt_is_dead = true;
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}
};


DECLARE_SCRIPT (M00_Advanced_Guard_Tower_Missile, "")
{
	bool firing;

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Is_Rendered (obj, false);
		ScriptEngine::Enable_Hibernation (obj, false);
		firing = false;
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == 1)
		{
			GameObject * enemy = ScriptEngine::Find_Object (param);
			if (enemy)
			{
				Vector3 my_position = ScriptEngine::Get_Position (obj);
				Vector3 enemy_position = ScriptEngine::Get_Position (enemy);
				float distance = ScriptEngine::Get_Distance (my_position, enemy_position);
				if (distance > 30.0f)
				{
					if (!firing)
					{
						firing = true;
						ActionParamsStruct params;
						params.Set_Basic(this, 100, 1);
						params.Set_Attack(enemy, 300.0f, 0.0f, true);
						params.AttackCheckBlocked = false;
						ScriptEngine::Action_Attack(obj, params);
						ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
					}
				}
			}
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			ScriptEngine::Action_Reset (obj, 100);
			firing = false;
		}
	}
};


DECLARE_SCRIPT (M00_Purchase_Terminal_GDI, "")
{
	void Poked (GameObject * /*obj*/, GameObject * /*poker*/) override
	{
		ScriptEngine::Display_GDI_Player_Terminal ();
	}
};

DECLARE_SCRIPT (M00_Purchase_Terminal_NOD, "")
{
	void Poked (GameObject * /*obj*/, GameObject * /*poker*/) override
	{
		ScriptEngine::Display_NOD_Player_Terminal ();
	}
};


DECLARE_SCRIPT (M00_Purchase_Terminal_Mutant, "")
{
	void Poked (GameObject * /*obj*/, GameObject * /*poker*/) override
	{
		ScriptEngine::Display_Mutant_Player_Terminal ();
	}
};


DECLARE_SCRIPT (M00_Nod_Turret, "")
{
	int token_01_id;
	int token_02_id;
	int token_03_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (token_01_id, 1);
		SAVE_VARIABLE (token_02_id, 2);
		SAVE_VARIABLE (token_03_id, 3);
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);
		ScriptEngine::Enable_Enemy_Seen (obj, true);

		Vector3 my_position = ScriptEngine::Get_Position (obj);
		Vector3 token_01_pos = my_position;
		Vector3 token_02_pos = my_position;
		Vector3 token_03_pos = my_position;

		token_01_pos.X -= 10.0f;
		token_01_pos.Y -= 10.0f;
		token_01_pos.Z += 2.0f;

		token_02_pos.X += 10.0f;
		token_02_pos.Z += 2.0f;

		token_03_pos.X += 10.0f;
		token_03_pos.Y -= 10.0f;
		token_03_pos.Z += 2.0f;

		GameObject * token_01 = ScriptEngine::Create_Object ("Invisible_Object", token_01_pos);
		if (token_01)
		{
			token_01_id = ScriptEngine::Get_ID (token_01);
		}
		token_01 = ScriptEngine::Create_Object ("Invisible_Object", token_02_pos);
		if (token_01)
		{
			token_02_id = ScriptEngine::Get_ID (token_01);
		}
		token_01 = ScriptEngine::Create_Object ("Invisible_Object", token_03_pos);
		if (token_01)
		{
			token_03_id = ScriptEngine::Get_ID (token_01);
		}
		ScriptEngine::Start_Timer (obj, this, 10.0f, 1);
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			int rnd_num = Get_Int_Random(0,2);
			switch (rnd_num)
			{
			case (0):
				{
					GameObject * token_01 = ScriptEngine::Find_Object (token_01_id);
					if (token_01)
					{
						ActionParamsStruct params;
						params.Set_Basic(this, 70, 1);
						params.Set_Attack(token_01, 0.0f, 0.0f, true);
						ScriptEngine::Action_Attack(obj, params);
					}
					break;
				}
			case (1):
				{
					GameObject * token_02 = ScriptEngine::Find_Object (token_02_id);
					if (token_02)
					{
						ActionParamsStruct params;
						params.Set_Basic(this, 70, 1);
						params.Set_Attack(token_02, 0.0f, 0.0f, true);
						ScriptEngine::Action_Attack(obj, params);
					}
					break;
				}
			default:
				{
					GameObject * token_03 = ScriptEngine::Find_Object (token_03_id);
					if (token_03)
					{
						ActionParamsStruct params;
						params.Set_Basic(this, 70, 1);
						params.Set_Attack(token_03, 0.0f, 0.0f, true);
						ScriptEngine::Action_Attack(obj, params);
					}
				}
			}
			ScriptEngine::Start_Timer (obj, this, 10.0f, 1);
		}
		else if (timer_id == 2)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{
		Vector3 target_pos = ScriptEngine::Get_Position (enemy);

		target_pos.Z -= 0.5f;

		ActionParamsStruct params;
		params.Set_Basic(this, 100, 2);
		params.Set_Attack(target_pos, 300.0f, 0.0f, true);
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Attack(obj, params);
		ScriptEngine::Start_Timer (obj, this, 2.0f, 2);
	}

	void Killed(GameObject * obj, GameObject * /*killer*/) override
	{
		Vector3 myPositon = ScriptEngine::Get_Position ( obj );
		float myFacing  = ScriptEngine::Get_Facing ( obj );

		GameObject * destroyedTurret = ScriptEngine::Create_Object ( "Nod_Turret_Destroyed", myPositon);
		ScriptEngine::Set_Facing ( destroyedTurret, myFacing );
	}

	void Action_Complete (GameObject * obj, int action_id, ActionCompleteReason /*complete_reason*/) override
	{
		if (action_id == 2)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}
};


DECLARE_SCRIPT(M00_Nod_Obelisk_CNC, "Controller_ID=0:int")
{
	int obelisk_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (obelisk_id, 1);
	}

	void Created (GameObject * obj) override
	{
		obelisk_id = 0;
		Vector3 my_location = ScriptEngine::Get_Position (obj);
		my_location.Z += 45.0f;
		GameObject * obelisk = 	ScriptEngine::Create_Object ("Nod_Obelisk", my_location);
		if (obelisk)
		{
			ScriptEngine::Attach_Script (obelisk, "M00_Obelisk_Weapon_CNC", "");
			ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
			obelisk_id = ScriptEngine::Get_ID (obelisk);
			// Send Obelisk Weapon ID to controller identified through parameter Controller_ID
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(Get_Int_Parameter("Controller_ID")), M00_OBELISK_WEAPON_ID, ScriptEngine::Get_ID(obelisk), 0);
		}
	}

	void Killed( GameObject * obj, GameObject * /*killer*/ ) override
	{
		GameObject * obelisk = ScriptEngine::Find_Object(obelisk_id);
		if (obelisk != nullptr)
		{
			ScriptEngine::Send_Custom_Event (obj, obelisk, 3, 0, 0);//this custom tells the obelisk weapon that the obelisk has been destroyed -- 02/12/2002 JDG
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			float health = ScriptEngine::Get_Health (obj);
			bool power = ScriptEngine::Get_Building_Power (obj);
			if ((!health) || (!power))
			{
				GameObject * obelisk = ScriptEngine::Find_Object (obelisk_id);
				if (obelisk)
				{
					ScriptEngine::Send_Custom_Event (obj, obelisk, 1, 0, 0.0f);
				}
			}
			else if (power)
			{
				GameObject * obelisk = ScriptEngine::Find_Object (obelisk_id);
				if (obelisk)
				{
					ScriptEngine::Send_Custom_Event (obj, obelisk, 1, 1, 0.0f);
				}
			}
			ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if ((type == 1) && (param == 1))
		{
			// Custom received to destroy obelisk weapon.

			GameObject * obelisk = ScriptEngine::Find_Object (obelisk_id);
			if (obelisk)
			{
				ScriptEngine::Destroy_Object (obelisk);
			}
		}
	}
};


DECLARE_SCRIPT (M00_Obelisk_Weapon_CNC, "")
{
	bool able_to_fire;
	int current_target;
	int powerup_effect_id;
	float range;

	bool obelisk_is_dead;//adding bool trying to eliminate lingering firing bug -- 02/12/2002 JDG

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (able_to_fire, 1);
		SAVE_VARIABLE (current_target, 2);
		SAVE_VARIABLE (powerup_effect_id, 3);
		SAVE_VARIABLE (range, 4);
		SAVE_VARIABLE (obelisk_is_dead, 5);
	}

	void Created (GameObject * obj) override
	{
		obelisk_is_dead = false;
		range = 150.0f;
		powerup_effect_id = 0;
		current_target = 0;
		able_to_fire = true;
		ScriptEngine::Set_Player_Type (obj, SCRIPT_PLAYERTYPE_NOD);
		ScriptEngine::Set_Is_Rendered (obj, false);
		ScriptEngine::Enable_Enemy_Seen (obj, true);
		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);
		Vector3 my_position = ScriptEngine::Get_Position (obj);
		GameObject * effect = ScriptEngine::Create_Object ("Obelisk Effect", my_position);
		if (effect)
		{
			powerup_effect_id = ScriptEngine::Get_ID (effect);
			ScriptEngine::Set_Animation_Frame (effect, "OBL_POWERUP.OBL_POWERUP", 0);
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		if (obelisk_is_dead == false)//adding extra safety check here -- 02/12/2002 JDG
		{
			int enemy_id = ScriptEngine::Get_ID (enemy);
			ScriptEngine::Send_Custom_Event (obj, obj, 2, enemy_id, 0.0f);
		}
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == 1)
		{
			if (param == 0)
			{
				ScriptEngine::Enable_Enemy_Seen (obj, false);
				able_to_fire = false;
				ActionParamsStruct params;
				params.Set_Basic (this, 100, 0);
				params.Set_Attack (obj, 0.0f, 0.0f, true);
				ScriptEngine::Action_Attack (obj, params);
				ScriptEngine::Action_Reset (obj, 100.0f);
				Destroy_Obelisk_Effect ();
			}
			else
			{
				ScriptEngine::Enable_Enemy_Seen (obj, true);
				able_to_fire = true;
			}
		}
		else if (type == 2)
		{
			if (obelisk_is_dead == false)//adding extra safety check here -- 02/12/2002 JDG
			{
				if (able_to_fire)
				{
					GameObject * target_obj = ScriptEngine::Find_Object (param);
					if (target_obj)
					{
						Vector3 enemy_position = ScriptEngine::Get_Position (target_obj);
						Vector3 my_position = ScriptEngine::Get_Position (obj);
						float distance = ScriptEngine::Get_Distance (my_position, enemy_position);
						enemy_position.Z = 0.0f;
						my_position.Z = 0.0f;
						float difference = ScriptEngine::Get_Distance (my_position, enemy_position);
						if ((difference > 15.0f) && (distance < range))
						{
							current_target = param;
							able_to_fire = false;
							ScriptEngine::Start_Timer (obj, this, 2.0f, 1);
							GameObject * effect = ScriptEngine::Find_Object (powerup_effect_id);
							if (effect)
							{
								ScriptEngine::Set_Animation_Frame (effect, "OBL_POWERUP.OBL_POWERUP", 1);
							}
							my_position.Z -= 20.0f;
							ScriptEngine::Create_Sound ("Obelisk_Warm_Up", my_position, obj);
						}
						else
						{
							Destroy_Obelisk_Effect ();
							ScriptEngine::Action_Reset (obj, 100.0f);
						}
					}
				}
			}
		}

		else if (type == 3)//obelisk has been killed, reset action and prevent further firing -- 02/12/2002 JDG
		{
			obelisk_is_dead = true;
			Destroy_Obelisk_Effect ();
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (obelisk_is_dead == false)//adding extra safety check here -- 02/12/2002 JDG
		{
			if (timer_id == 1)
			{
				GameObject * target_obj = ScriptEngine::Find_Object (current_target);
				if (target_obj)
				{
					Vector3 my_position = ScriptEngine::Get_Position (obj);
					Vector3 enemy_position = ScriptEngine::Get_Position (target_obj);
					float distance = ScriptEngine::Get_Distance (my_position, enemy_position);
					enemy_position.Z = 0.0f;
					my_position.Z = 0.0f;
					float difference = ScriptEngine::Get_Distance (my_position, enemy_position);
					if ((difference > 15.0f) && (distance < range))
					{
						ActionParamsStruct params;
						params.Set_Basic (this, 100, 0);
						params.Set_Attack (target_obj, range, 0.0f, true);
						ScriptEngine::Action_Attack (obj, params);
						current_target = 0;
						ScriptEngine::Start_Timer (obj, this, 2.0f, 2);
					}
					else
					{
						Destroy_Obelisk_Effect ();
						ScriptEngine::Action_Reset (obj, 100.0f);
						able_to_fire = true;
					}
				}
				else
				{
					Destroy_Obelisk_Effect ();
					ScriptEngine::Action_Reset (obj, 100.0f);
					able_to_fire = true;
				}
			}
			else if (timer_id == 2)
			{
				Destroy_Obelisk_Effect ();
				able_to_fire = true;
			}
		}
	}

	void Destroy_Obelisk_Effect (void)
	{
		if (powerup_effect_id)
		{
			GameObject * effect = ScriptEngine::Find_Object (powerup_effect_id);
			if (effect)
			{
				ScriptEngine::Set_Animation_Frame (effect, "OBL_POWERUP.OBL_POWERUP", 0);
			}
		}
	}
};


DECLARE_SCRIPT(M00_Nod_Obelisk, "Controller_ID=0:int")
{
	int obelisk_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (obelisk_id, 1);
	}

	void Created (GameObject * obj) override
	{
		obelisk_id = 0;
		Vector3 my_location = ScriptEngine::Get_Position (obj);
		my_location.Z += 45.0f;
		GameObject * obelisk = 	ScriptEngine::Create_Object ("Nod_Obelisk", my_location);
		if (obelisk)
		{
			ScriptEngine::Attach_Script (obelisk, "M00_Obelisk_Weapon", "");
			ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
			obelisk_id = ScriptEngine::Get_ID (obelisk);
			// Send Obelisk Weapon ID to controller identified through parameter Controller_ID
			int controller_id = Get_Int_Parameter("Controller_ID");
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(controller_id), M00_OBELISK_WEAPON_ID, ScriptEngine::Get_ID(obelisk), 4.0f);
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			float health = ScriptEngine::Get_Health (obj);
			bool power = ScriptEngine::Get_Building_Power (obj);
			if ((!health) || (!power))
			{
				GameObject * obelisk = ScriptEngine::Find_Object (obelisk_id);
				if (obelisk)
				{
					ScriptEngine::Send_Custom_Event (obj, obelisk, 1, 0, 0.0f);
				}
			}
			else if (power)
			{
				GameObject * obelisk = ScriptEngine::Find_Object (obelisk_id);
				if (obelisk)
				{
					ScriptEngine::Send_Custom_Event (obj, obelisk, 1, 1, 0.0f);
				}
			}
			ScriptEngine::Start_Timer (obj, this, 1.0f, 1);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if ((type == 1) && (param == 1))
		{
			// Custom received to destroy obelisk weapon.

			GameObject * obelisk = ScriptEngine::Find_Object (obelisk_id);
			if (obelisk)
			{
				ScriptEngine::Destroy_Object (obelisk);
			}
		}
	}
};


DECLARE_SCRIPT (M00_Obelisk_Weapon, "")
{
	bool able_to_fire;
	int current_target;
	int powerup_effect_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (able_to_fire, 1);
		SAVE_VARIABLE (current_target, 2);
		SAVE_VARIABLE (powerup_effect_id, 3);
	}

	void Created (GameObject * obj) override
	{
		powerup_effect_id = 0;
		current_target = 0;
		able_to_fire = true;
		ScriptEngine::Set_Player_Type (obj, SCRIPT_PLAYERTYPE_NOD);
		ScriptEngine::Set_Is_Rendered (obj, false);
		ScriptEngine::Enable_Enemy_Seen (obj, true);
		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);
		Vector3 my_position = ScriptEngine::Get_Position (obj);
		GameObject * effect = ScriptEngine::Create_Object ("Obelisk Effect", my_position);
		if (effect)
		{
			powerup_effect_id = ScriptEngine::Get_ID (effect);
			ScriptEngine::Set_Animation_Frame (effect, "OBL_POWERUP.OBL_POWERUP", 0);
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		int enemy_id = ScriptEngine::Get_ID (enemy);
		ScriptEngine::Send_Custom_Event (obj, obj, 2, enemy_id, 0.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == 1)
		{
			if (param == 0)
			{
				ScriptEngine::Enable_Enemy_Seen (obj, false);
				able_to_fire = false;
				ActionParamsStruct params;
				params.Set_Basic (this, 100, 0);
				params.Set_Attack (obj, 0.0f, 0.0f, true);
				ScriptEngine::Action_Attack (obj, params);
				ScriptEngine::Action_Reset (obj, 100.0f);
				Destroy_Obelisk_Effect ();
			}
			else
			{
				ScriptEngine::Enable_Enemy_Seen (obj, true);
				able_to_fire = true;
			}
		}
		else if (type == 2)
		{
			if (able_to_fire)
			{
				GameObject * target_obj = ScriptEngine::Find_Object (param);
				if (target_obj)
				{
					Vector3 enemy_position = ScriptEngine::Get_Position (target_obj);
					Vector3 my_position = ScriptEngine::Get_Position (obj);
					float distance = ScriptEngine::Get_Distance (my_position, enemy_position);
					enemy_position.Z = 0.0f;
					my_position.Z = 0.0f;
					float difference = ScriptEngine::Get_Distance (my_position, enemy_position);
					if ((difference > 15.0f) && (distance < 70.0f))
					{
						current_target = param;
						able_to_fire = false;
						ScriptEngine::Start_Timer (obj, this, 2.5f, 1);
						GameObject * effect = ScriptEngine::Find_Object (powerup_effect_id);
						if (effect)
						{
							ScriptEngine::Set_Animation_Frame (effect, "OBL_POWERUP.OBL_POWERUP", 1);
						}
						my_position.Z -= 20.0f;
						ScriptEngine::Create_Sound ("Obelisk_Warm_Up", my_position, obj);
					}
					else
					{
						Destroy_Obelisk_Effect ();
						ScriptEngine::Action_Reset (obj, 100.0f);
					}
				}
			}
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			GameObject * target_obj = ScriptEngine::Find_Object (current_target);
			if (target_obj)
			{
				Vector3 my_position = ScriptEngine::Get_Position (obj);
				Vector3 enemy_position = ScriptEngine::Get_Position (target_obj);
				float distance = ScriptEngine::Get_Distance (my_position, enemy_position);
				enemy_position.Z = 0.0f;
				my_position.Z = 0.0f;
				float difference = ScriptEngine::Get_Distance (my_position, enemy_position);
				if ((difference > 15.0f) && (distance < 70.0f))
				{
					ActionParamsStruct params;
					params.Set_Basic (this, 100, 0);
					params.Set_Attack (target_obj, 150.0f, 0.0f, true);
					ScriptEngine::Action_Attack (obj, params);
					current_target = 0;
					ScriptEngine::Start_Timer (obj, this, 2.0f, 2);
				}
				else
				{
					Destroy_Obelisk_Effect ();
					ScriptEngine::Action_Reset (obj, 100.0f);
					able_to_fire = true;
				}
			}
			else
			{
				Destroy_Obelisk_Effect ();
				ScriptEngine::Action_Reset (obj, 100.0f);
				able_to_fire = true;
			}
		}
		else if (timer_id == 2)
		{
			Destroy_Obelisk_Effect ();
			able_to_fire = true;
		}
	}

	void Destroy_Obelisk_Effect (void)
	{
		if (powerup_effect_id)
		{
			GameObject * effect = ScriptEngine::Find_Object (powerup_effect_id);
			if (effect)
			{
				ScriptEngine::Set_Animation_Frame (effect, "OBL_POWERUP.OBL_POWERUP", 0);
			}
		}
	}
};

DECLARE_SCRIPT (M00_Select_Empty_Hands, "On_Created=1:int")
{

	void Created (GameObject * obj) override
	{
		bool on_created = (Get_Int_Parameter("On_Created") == 1) ? true : false;
		if(on_created)
		{
			ScriptEngine::Select_Weapon(obj, nullptr );
		}
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M00_SELECT_EMPTY_HANDS)
		{
			ScriptEngine::Select_Weapon(obj, nullptr );
		}
	}
};

DECLARE_SCRIPT(M00_ArmorMedal_TextMessage_JDG, "")
{
	void Custom( GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Set_HUD_Help_Text ( IDS_M00DSGN_DSGN1009I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
		}
	}
};

DECLARE_SCRIPT(M00_HealthMedal_TextMessage_JDG, "")
{
	void Custom( GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Set_HUD_Help_Text ( IDS_M00DSGN_DSGN1008I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
		}
	}
};

DECLARE_SCRIPT(M00_Change_L3Mutant_RadarMarker_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_MUTANT );
	}
};

DECLARE_SCRIPT(M00_SSM_DLS, "")
{
	int ssm_missile_id;
	bool launched;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( ssm_missile_id, 1 );
		SAVE_VARIABLE( launched, 2 );
	}

	void Created (GameObject * obj) override
	{
		launched = false;

		GameObject * ssm_missile = ScriptEngine::Create_Object_At_Bone(obj, "Saveable_Invisible_Object", "V_LAUNCHER");
		ScriptEngine::Set_Model(ssm_missile, "V_AG_NOD_SSM_MS");
		ScriptEngine::Attach_To_Object_Bone(ssm_missile, obj, "V_LAUNCHER");
		ssm_missile_id = ScriptEngine::Get_ID(ssm_missile);
	}

	void Custom(GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M00_LAUNCH_SSM)
		{
			ScriptEngine::Set_Animation(obj, "V_NOD_SSM.V_NOD_SSM", false, nullptr, 0.0f, -1.0f, false);
			ScriptEngine::Set_Animation(ScriptEngine::Find_Object(ssm_missile_id), "v_nod_ssm_Missl.v_nod_ssm_Missl", false, nullptr, 0.0f, -1.0f, false);

			ScriptEngine::Start_Timer(obj, this, 7.0f, 66000);
		}
	}

	void Timer_Expired (GameObject * /*obj*/, int Timer_ID) override
	{
		if (Timer_ID == 66000)
		{
			launched = true;
		}
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if (!launched)
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (ssm_missile_id));
		}
	}

};

DECLARE_SCRIPT(M00_Generic_Conv_DME, "ConvName:string")
{
	void Created (GameObject *obj) override
	{
		int id = ScriptEngine::Create_Conversation(Get_Parameter( "ConvName" ), 99, 2000, false);
		ScriptEngine::Join_Conversation(obj, id, true, true, true);
		ScriptEngine::Start_Conversation(id, 100000);
		ScriptEngine::Monitor_Conversation(obj, id);
	}
};

DECLARE_SCRIPT (M00_Damage_Modifier_DME, "Damage_multiplier:float, Star_Modifier=1:int, NotStar_Modifier=1:int, Killable_By_Star=1:int, Killable_by_NotStar=1:int")
{
	float current_health, last_health, damage_tally;
	int star_killable, notStar_killable;
	int star_modifier, notStar_modifier;
	bool mod_on;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( current_health, 1 );
		SAVE_VARIABLE( last_health, 2 );
		SAVE_VARIABLE( damage_tally, 3 );
		SAVE_VARIABLE( star_killable, 4 );
		SAVE_VARIABLE( notStar_killable, 5 );
		SAVE_VARIABLE( star_modifier, 6 );
		SAVE_VARIABLE( notStar_modifier, 7 );
		SAVE_VARIABLE( mod_on, 8 );
	}

	void Created (GameObject *obj) override
	{
		mod_on = true;
		last_health = ScriptEngine::Get_Health (obj);
		damage_tally = 0;
		star_killable = Get_Int_Parameter("Killable_By_Star");
		notStar_killable = Get_Int_Parameter("Killable_ByNotStar");
		star_modifier = Get_Int_Parameter("Star_Modifier");
		notStar_modifier = Get_Int_Parameter("NotStar_Modifier");
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if (type == M00_ENABLE_DAMAGE_MOD)
		{
			if (param == 1)
			{
				mod_on = true;
			}
			if (param == 0)
			{
				mod_on = false;
			}
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /*amount*/ ) override
	{
		if (mod_on)
		{
			float damage;

			if ((!star_modifier && damager == STAR && star_killable) || (!notStar_modifier && damager != STAR && notStar_killable))
			{
				last_health = ScriptEngine::Get_Health (obj);
			}

			if ((star_modifier && damager == STAR && star_killable) || (notStar_modifier && damager != STAR && notStar_killable))
			{
				current_health = ScriptEngine::Get_Health (obj);
				if (current_health == 0)
				{
					damage = ((last_health - current_health) + damage_tally);
				}

				else
				{
					damage = (last_health - current_health);
				}
				float mod_damage = (damage * (Get_Float_Parameter("Damage_multiplier")));
				damage_tally += mod_damage;

				ScriptEngine::Set_Health (obj, (last_health - mod_damage));
				last_health = ScriptEngine::Get_Health (obj);
				current_health = ScriptEngine::Get_Health (obj);
			}

			if ((star_modifier && damager == STAR && !star_killable) || (notStar_modifier && damager != STAR && !notStar_killable))
			{
				current_health = ScriptEngine::Get_Health (obj);
				if (current_health == 0)
				{
					damage = ((last_health - current_health));
				}

				else
				{
					damage = (last_health - current_health);
				}
				float mod_damage = (damage * (Get_Float_Parameter("Damage_multiplier")));

				ScriptEngine::Set_Health (obj, (last_health - mod_damage));
				last_health = ScriptEngine::Get_Health (obj);
				current_health = ScriptEngine::Get_Health (obj);
			}
		}
	}
};


DECLARE_SCRIPT (M00_Base_Defense, "MinAttackDistance=0:int, MaxAttackDistance=300:int, AttackTimer=10:int")
{
	int token_01_id;
	int token_02_id;
	int token_03_id;
	int player_type;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (token_01_id, 1);
		SAVE_VARIABLE (token_02_id, 2);
		SAVE_VARIABLE (token_03_id, 3);
		SAVE_VARIABLE (player_type, 4);
	}

	void Created (GameObject * obj) override
	{
		// find out what my team preset is.
		player_type = ScriptEngine::Get_Player_Type( obj );
		ScriptEngine::Debug_Message( "***** Player Type Saved *****\n" );


		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);
		ScriptEngine::Enable_Enemy_Seen (obj, true);

		Vector3 my_position = ScriptEngine::Get_Position (obj);
		Vector3 token_01_pos = my_position;
		Vector3 token_02_pos = my_position;
		Vector3 token_03_pos = my_position;

		token_01_pos.X -= 10.0f;
		token_01_pos.Y -= 10.0f;
		token_01_pos.Z += 2.0f;

		token_02_pos.X += 10.0f;
		token_02_pos.Z += 2.0f;

		token_03_pos.X += 10.0f;
		token_03_pos.Y -= 10.0f;
		token_03_pos.Z += 2.0f;

		GameObject * token_01 = ScriptEngine::Create_Object ("Invisible_Object", token_01_pos);
		if (token_01)
		{
			token_01_id = ScriptEngine::Get_ID (token_01);
		}
		token_01 = ScriptEngine::Create_Object ("Invisible_Object", token_02_pos);
		if (token_01)
		{
			token_02_id = ScriptEngine::Get_ID (token_01);
		}
		token_01 = ScriptEngine::Create_Object ("Invisible_Object", token_03_pos);
		if (token_01)
		{
			token_03_id = ScriptEngine::Get_ID (token_01);
		}
		ScriptEngine::Start_Timer (obj, this, 10.0f, 1);
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		if (timer_id == 1)
		{
			int rnd_num = Get_Int_Random(0,2);
			switch (rnd_num)
			{
			case (0):
				{
					GameObject * token_01 = ScriptEngine::Find_Object (token_01_id);
					if (token_01)
					{
						ActionParamsStruct params;
						params.Set_Basic(this, 70, 1);
						params.Set_Attack(token_01, 0.0f, 0.0f, true);
						ScriptEngine::Action_Attack(obj, params);
					}
					break;
				}
			case (1):
				{
					GameObject * token_02 = ScriptEngine::Find_Object (token_02_id);
					if (token_02)
					{
						ActionParamsStruct params;
						params.Set_Basic(this, 70, 1);
						params.Set_Attack(token_02, 0.0f, 0.0f, true);
						ScriptEngine::Action_Attack(obj, params);
					}
					break;
				}
			default:
				{
					GameObject * token_03 = ScriptEngine::Find_Object (token_03_id);
					if (token_03)
					{
						ActionParamsStruct params;
						params.Set_Basic(this, 70, 1);
						params.Set_Attack(token_03, 0.0f, 0.0f, true);
						ScriptEngine::Action_Attack(obj, params);
					}
				}
			}
			ScriptEngine::Start_Timer (obj, this, 10.0f, 1);
		}
		else if (timer_id == 2)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{

		Vector3 my_loc = ScriptEngine::Get_Position (obj);
		Vector3 enemy_loc = ScriptEngine::Get_Position (enemy);
		float distance = ScriptEngine::Get_Distance (my_loc, enemy_loc);
		if (distance > Get_Int_Parameter("MinAttackDistance") )
		{
			ActionParamsStruct params;
			params.Set_Basic(this, 100, 2);
			params.Set_Attack(enemy, float(Get_Int_Parameter("MaxAttackDistance")), 0.0f, true);
			params.AttackCheckBlocked = false;
			ScriptEngine::Action_Attack(obj, params);
			ScriptEngine::Start_Timer (obj, this, float(Get_Int_Parameter( "AttackTimer")), 2);
		}
	}

	void Action_Complete (GameObject * obj, int action_id, ActionCompleteReason /*complete_reason*/) override
	{
		if (action_id == 2)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Exited (GameObject* obj, GameObject* /*exiter*/) override
	{
		// set team back to my preset.
		ScriptEngine::Set_Player_Type( obj, player_type );
		ScriptEngine::Debug_Message( "***** Player Type Exited *****\n" );
	}
};
