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
*     Mission04.cpp
*
* DESCRIPTION
*     Mission 4 scripts
*
* PROGRAMMER
*     Design Team
*
* VERSION INFO
*     $Author: Joe_g $
*     $Revision: 122 $
*     $Modtime: 2/12/02 10:10a $
*     $Archive: /Commando/Code/Scripts/Mission04.cpp $
*
******************************************************************************/

#include "scripts.h"
#include <string.h>
#include "Toolkit.h"
#include "Mission1.h"
#include "Mission11.h"

DECLARE_SCRIPT(M04_Objective_Controller_JDG, "")//this guys ID number is M04_OBJECTIVE_CONTROLLER_JDG 100424
{
	int missile_01_sabotaged;
	int missile_02_sabotaged;
	int missile_03_sabotaged;
	int missile_04_sabotaged;
	int number_missiles_sabotaged;
	bool missile_primary_active;

	int torpedo_01_sabotaged;
	int torpedo_02_sabotaged;
	int number_torpedos_sabotaged;
	bool torpedo_primary_active;

	int engine_room_01_sabotaged;
	int engine_room_02_sabotaged;
	int engine_room_03_sabotaged;
	int engine_room_04_sabotaged;
	int number_engines_sabotaged;

	int announce_prisoner_objective;
	bool prisoner_primary_active;

	int announce_first_mate_objective;
	bool first_mate_primary_active;

	int announce_captain_objective;
	bool captain_primary_active;

	int announce_engine_room_objective;
	int announce_missile_room_objective;
	int announce_torpedo_room_objective;
	int announce_apache_objective;
	int announce_get_back_to_sub;

	int completed_prisoner_rescue;
	int completed_first_mate_objective;
	int completed_captain_objective;
	int completed_engine_room_objective;
	int completed_missile_room_objective;
	int completed_torpedo_room_objective;
	int completed_apache_objective;
	bool mission_started;

	int attach_havocs_script;
	int announce_prison_guard_objective;
	//int tell_where_guard_is;
	int completed_get_the_key;

	//new stuff
	int protectPOWsConv;
	int missionIntroConv;
	int missileConv;
	int engineIntroConv;
	int prisonKeyIntroConv;
	int medlab_conv;
	int firstmateConv;

	bool first_mate_primary_triggered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( missile_01_sabotaged, 1 );
		SAVE_VARIABLE( missile_02_sabotaged, 2 );
		SAVE_VARIABLE( missile_03_sabotaged, 3 );
		SAVE_VARIABLE( missile_04_sabotaged, 4 );
		SAVE_VARIABLE( number_missiles_sabotaged, 5 );
		SAVE_VARIABLE( missile_primary_active, 6 );
		SAVE_VARIABLE( torpedo_01_sabotaged, 7 );
		SAVE_VARIABLE( torpedo_02_sabotaged, 8 );
		SAVE_VARIABLE( number_torpedos_sabotaged, 9 );
		SAVE_VARIABLE( torpedo_primary_active, 10 );

		SAVE_VARIABLE( engine_room_01_sabotaged, 11 );
		SAVE_VARIABLE( engine_room_02_sabotaged, 12 );
		SAVE_VARIABLE( engine_room_03_sabotaged, 13 );
		SAVE_VARIABLE( engine_room_04_sabotaged, 14 );
		SAVE_VARIABLE( number_engines_sabotaged, 15 );
		SAVE_VARIABLE( announce_prisoner_objective, 16 );
		SAVE_VARIABLE( prisoner_primary_active, 17 );
		SAVE_VARIABLE( announce_first_mate_objective, 18 );
		SAVE_VARIABLE( first_mate_primary_active, 19 );
		SAVE_VARIABLE( announce_captain_objective, 20 );

		SAVE_VARIABLE( captain_primary_active, 21 );
		SAVE_VARIABLE( announce_engine_room_objective, 22 );
		SAVE_VARIABLE( announce_missile_room_objective, 23 );
		SAVE_VARIABLE( announce_torpedo_room_objective, 24 );
		SAVE_VARIABLE( announce_apache_objective, 25 );
		SAVE_VARIABLE( announce_get_back_to_sub, 26 );
		SAVE_VARIABLE( completed_prisoner_rescue, 27 );
		SAVE_VARIABLE( completed_first_mate_objective, 28 );
		SAVE_VARIABLE( completed_captain_objective, 29 );
		SAVE_VARIABLE( completed_engine_room_objective, 30 );

		SAVE_VARIABLE( completed_missile_room_objective, 31 );
		SAVE_VARIABLE( completed_torpedo_room_objective, 32 );
		SAVE_VARIABLE( completed_apache_objective, 33 );
		SAVE_VARIABLE( mission_started, 34 );
		SAVE_VARIABLE( attach_havocs_script, 35 );
		SAVE_VARIABLE( announce_prison_guard_objective, 36 );
		SAVE_VARIABLE( firstmateConv, 37 );
		SAVE_VARIABLE( completed_get_the_key, 38 );
		SAVE_VARIABLE( protectPOWsConv, 39 );//
		SAVE_VARIABLE( missionIntroConv, 40 );

		SAVE_VARIABLE( missileConv, 41 );
		SAVE_VARIABLE( engineIntroConv, 42 );
		SAVE_VARIABLE( prisonKeyIntroConv, 43 );
		SAVE_VARIABLE( medlab_conv, 44 );
		SAVE_VARIABLE( firstmateConv, 45 );
		SAVE_VARIABLE( first_mate_primary_triggered, 46 );
	}

	void Created( GameObject * obj ) override
	{
/**********************************************************************************************************
The following are the custom parameter number values used to control objectives in mission 04
***********************************************************************************************************/
		missile_01_sabotaged						= 100;
		missile_02_sabotaged						= 110;
		missile_03_sabotaged						= 120;
		missile_04_sabotaged						= 130;
		number_missiles_sabotaged					= 0;
		missile_primary_active						= false;

		torpedo_01_sabotaged						= 200;
		torpedo_02_sabotaged						= 210;
		number_torpedos_sabotaged					= 0;
		torpedo_primary_active						= false;

		engine_room_01_sabotaged					= 300;
		engine_room_02_sabotaged					= 310;
		engine_room_03_sabotaged					= 320;
		engine_room_04_sabotaged					= 330;
		number_engines_sabotaged					= 0;

		announce_prisoner_objective					= 400;
		announce_prison_guard_objective				= 401;
		//tell_where_guard_is							= 402;
		prisoner_primary_active						= false;

		announce_first_mate_objective				= 410;
		first_mate_primary_active					= false;
		first_mate_primary_triggered				= false;

		announce_captain_objective					= 420;
		captain_primary_active						= false;

		announce_engine_room_objective				= 430;
		announce_missile_room_objective				= 440;
		announce_torpedo_room_objective				= 450;
		announce_apache_objective					= 460;
		announce_get_back_to_sub					= 470;

		completed_prisoner_rescue					= 500;
		completed_first_mate_objective				= 510;
		completed_captain_objective					= 520;
		completed_engine_room_objective				= 530;
		completed_missile_room_objective			= 540;
		completed_torpedo_room_objective			= 550;
		completed_apache_objective					= 560;
		completed_get_the_key						= 590;
		mission_started								= false;

		attach_havocs_script						= 600;

		ScriptEngine::Send_Custom_Event( obj, obj, 0, announce_prisoner_objective, 3 );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED: //
				{
					if (action_id == missionIntroConv) //rescue prisoners here
					{
						ScriptEngine::Debug_Message ( "**********************mission start conversation over--sending custom\n" );
						ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 0 );

						ScriptEngine::Add_Objective( 100, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_01, nullptr, IDS_Enc_Obj_Primary_M04_01 );
						ScriptEngine::Add_Radar_Marker ( 101, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( M04_PRISON_PRISONER_01_JDG )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
						ScriptEngine::Add_Radar_Marker ( 102, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( M04_PRISON_PRISONER_02_JDG )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
						ScriptEngine::Add_Radar_Marker ( 103, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( M04_PRISON_PRISONER_03_JDG )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );

						Vector3 prisoner01_position = ScriptEngine::Get_Position ( ScriptEngine::Find_Object ( M04_PRISON_PRISONER_01_JDG ) );
						ScriptEngine::Set_Objective_HUD_Info_Position( 100, 90, "POG_M04_1_01.tga", IDS_POG_RESCUE, prisoner01_position );
					}

					else if (action_id == missileConv) //sabotage missile racks  M04_Add_MissileRoom_Objective_Conversation_02
					{
						//ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 2 );

						//ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);
						ScriptEngine::Add_Objective( 300, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_03, nullptr, IDS_Enc_Obj_Primary_M04_03 );

						GameObject * missile_target01 = ScriptEngine::Find_Object ( 100421 );
						GameObject * missile_target02 = ScriptEngine::Find_Object ( 100422 );
						GameObject * missile_target03 = ScriptEngine::Find_Object ( 100423 );
						GameObject * missile_target04 = ScriptEngine::Find_Object ( 100420 );
						if (missile_target01 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, missile_target01, 0, M01_START_ACTING_JDG, 0 );
						}

						if (missile_target02 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, missile_target02, 0, M01_START_ACTING_JDG, 0 );
						}

						if (missile_target03 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, missile_target03, 0, M01_START_ACTING_JDG, 0 );
						}

						if (missile_target04 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, missile_target04, 0, M01_START_ACTING_JDG, 0 );
						}

						/*int missileConv02 = ScriptEngine::Create_Conversation( "M04_Add_MissileRoom_Objective_Conversation_02", 100, 1000, false);
						ScriptEngine::Join_Conversation( nullptr, missileConv02, false, false );
						ScriptEngine::Start_Conversation( missileConv02,  missileConv02 );*/

						GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
						if (pogController  != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, pogController, M01_ADD_OBJECTIVE_POG_JDG, 300, 0 );
						}
					}

					else if (action_id == engineIntroConv) //destroy engine terminals
					{
						//ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 2 );

						ScriptEngine::Add_Objective( 200, OBJECTIVE_TYPE_SECONDARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Secondary_M04_02, nullptr, IDS_Enc_Obj_Secondary_M04_02 );

						GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
						if (pogController  != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, pogController, M01_ADD_OBJECTIVE_POG_JDG, 200, 0 );
						}

						GameObject * target01 = ScriptEngine::Find_Object ( M04_ENGINEROOM_TARGET_01_JDG );
						GameObject * target02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_TARGET_02_JDG );
						GameObject * target03 = ScriptEngine::Find_Object ( M04_ENGINEROOM_TARGET_03_JDG );
						GameObject * target04 = ScriptEngine::Find_Object ( M04_ENGINEROOM_TARGET_04_JDG );

						if (target01 != nullptr)
						{
							ScriptEngine::Set_Obj_Radar_Blip_Shape ( target01, RADAR_BLIP_SHAPE_OBJECTIVE );
							ScriptEngine::Set_Obj_Radar_Blip_Color ( target01, RADAR_BLIP_COLOR_SECONDARY_OBJECTIVE );
						}

						if (target02 != nullptr)
						{
							ScriptEngine::Set_Obj_Radar_Blip_Shape ( target02, RADAR_BLIP_SHAPE_OBJECTIVE );
							ScriptEngine::Set_Obj_Radar_Blip_Color ( target02, RADAR_BLIP_COLOR_SECONDARY_OBJECTIVE );
						}

						if (target03 != nullptr)
						{
							ScriptEngine::Set_Obj_Radar_Blip_Shape ( target03, RADAR_BLIP_SHAPE_OBJECTIVE );
							ScriptEngine::Set_Obj_Radar_Blip_Color ( target03, RADAR_BLIP_COLOR_SECONDARY_OBJECTIVE );
						}

						if (target04 != nullptr)
						{
							ScriptEngine::Set_Obj_Radar_Blip_Shape ( target04, RADAR_BLIP_SHAPE_OBJECTIVE );
							ScriptEngine::Set_Obj_Radar_Blip_Color ( target04, RADAR_BLIP_COLOR_SECONDARY_OBJECTIVE );
						}
					}

					else if (action_id == prisonKeyIntroConv) //get prison key from guard
					{
						ScriptEngine::Add_Objective( 110, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_02, nullptr, IDS_Enc_Obj_Primary_M04_02 );

						medlab_conv = ScriptEngine::Create_Conversation( "M04_Eva_Tells_Where_Guard_Is", 100, 100, false);
						ScriptEngine::Join_Conversation( nullptr, medlab_conv, false, true, true );
						ScriptEngine::Start_Conversation( medlab_conv, medlab_conv );
						ScriptEngine::Monitor_Conversation (obj, medlab_conv);
					}

					else if (action_id == medlab_conv) //get prison key from guard
					{
						GameObject * warden = ScriptEngine::Find_Object ( M04_PRISON_WARDEN_JDG );
						if (warden != nullptr)
						{
							Vector3 wardenLocation = ScriptEngine::Get_Position(warden );
							ScriptEngine::Add_Radar_Marker ( 111, wardenLocation, RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );

							ScriptEngine::Set_Objective_HUD_Info_Position( 110, 92, "POG_M04_1_02.tga", IDS_POG_ACQUIRE, wardenLocation );
						}//
					}

					else if (action_id == firstmateConv) //get level 2 key from first mate  protectPOWsConv
					{
						ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);
						ScriptEngine::Add_Objective( 600, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_07, nullptr, IDS_Enc_Obj_Primary_M04_07 );
						ScriptEngine::Add_Radar_Marker ( 601, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( M04_SHIPS_FIRST_MATE_JDG )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
						ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 2 );//this sets a small delay before havoc responds

						GameObject * firstmate = ScriptEngine::Find_Object ( M04_SHIPS_FIRST_MATE_JDG );
						if (firstmate != nullptr)
						{
							Vector3 firstmateLocation = ScriptEngine::Get_Position(firstmate);
							ScriptEngine::Set_Objective_HUD_Info_Position( 600, 93, "POG_M04_1_07.tga", IDS_POG_ACQUIRE, firstmateLocation );
						}
					}

					else if (action_id == protectPOWsConv) //give protect prisoner objective
					{
						ScriptEngine::Add_Objective(  800, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_06, nullptr, IDS_Enc_Obj_Primary_M04_06 );
						ScriptEngine::Add_Radar_Marker ( 801, Vector3 (2.55f, -5.29f, -16.96f), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );

						Vector3 subLocation (-0.401f, 3.878f, -8.850f);
						ScriptEngine::Set_Objective_HUD_Info_Position( 800, 99, "POG_M04_1_06.tga", IDS_POG_PROTECT, subLocation );
					}
				}
				break;
			default:
				break;
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
/**********************************************************************************************************
The following are params for when the individual objectives are activated and announced
***********************************************************************************************************/

		if (type == 0)
		{
			if (param == 1)//this will play a twiddler of Havoc accepting the objective
			{
				ScriptEngine::Debug_Message ( "**********************controller should be playing havoc affirmative twiddler now\n" );
				Vector3 playerPosition = ScriptEngine::Get_Position (STAR);
				ScriptEngine::Create_Sound ( "M04_Havoc_Affirmative_Twiddler", playerPosition, obj);
			}

			else if (param == 2)//this will play a twiddler of Havoc having finished the objective
			{
				//ScriptEngine::Create_Sound ( "Havoc Finished Objective Twiddler", Vector3 (0,0,0), obj);
			}

			else if (param == announce_prisoner_objective)
			{
				ScriptEngine::Debug_Message ( "**********************starting mission start conversation\n" );
				missionIntroConv = ScriptEngine::Create_Conversation( "M04_Mission_Start_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, missionIntroConv, false, false, true );
				///ScriptEngine::Join_Conversation( STAR, missionIntroConv, false, false );
				ScriptEngine::Start_Conversation( missionIntroConv,  missionIntroConv );
				ScriptEngine::Monitor_Conversation (obj, missionIntroConv);

				prisoner_primary_active = true;
				//ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);

				//ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 2 );//this sets a small delay before havoc responds
				mission_started	= true;
				//ScriptEngine::Send_Custom_Event( obj, obj, 0, attach_havocs_script, 0 );

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 0, 0 );
				}


			}

			else if (param == announce_prison_guard_objective)
			{
				prisonKeyIntroConv = ScriptEngine::Create_Conversation( "M04_Add_PrisonKey_Objective", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, prisonKeyIntroConv, false, false, true );
				ScriptEngine::Start_Conversation( prisonKeyIntroConv,  prisonKeyIntroConv );
				ScriptEngine::Monitor_Conversation (obj, prisonKeyIntroConv);

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 2, 0 );
				}


				//ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);


				ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 2 );//this sets a small delay before havoc responds
				//ScriptEngine::Send_Custom_Event( obj, obj, 0, tell_where_guard_is, 5 );
			}

			/*else if (param == attach_havocs_script)
			{
				if (STAR)
				{
					ScriptEngine::Attach_Script(STAR, "M04_Havocs_Script_JDG", "");
				}

				else
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, attach_havocs_script, 1 );
				}

			}*/

			else if (param == announce_engine_room_objective)
			{
				engineIntroConv = ScriptEngine::Create_Conversation( "M04_Add_EngineRoom_Objective_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, engineIntroConv, false, false, true );
				//ScriptEngine::Join_Conversation( STAR, engineIntroConv, false, false );
				ScriptEngine::Start_Conversation( engineIntroConv,  engineIntroConv );
				ScriptEngine::Monitor_Conversation (obj, engineIntroConv);
			}

			else if (param == announce_missile_room_objective)//
			{
				missileConv = ScriptEngine::Create_Conversation( "M04_Add_MissileRoom_Objective_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, missileConv, false, false, true );
				ScriptEngine::Start_Conversation( missileConv,  missileConv );
				ScriptEngine::Monitor_Conversation (obj, missileConv);

				missile_primary_active = true;

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 1, 0 );
				}
			}

			else if (param == announce_torpedo_room_objective)
			{
				torpedo_primary_active = true;
				//ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);
				ScriptEngine::Add_Objective( 400, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_04, nullptr, IDS_Enc_Obj_Primary_M04_04 );
				ScriptEngine::Add_Radar_Marker ( 401, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( 100409 )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				ScriptEngine::Add_Radar_Marker ( 402, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( 100410 )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 2 );//this sets a small delay before havoc responds

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 5, 0 );
				}
			}

			else if (param == announce_apache_objective)
			{
				ScriptEngine::Create_Sound ( "00-n042e", Vector3 (0,0,0), obj);

				ScriptEngine::Add_Objective( 500, OBJECTIVE_TYPE_SECONDARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Secondary_M04_03, nullptr, IDS_Enc_Obj_Secondary_M04_03 );
				ScriptEngine::Set_Objective_Radar_Blip_Object(500, sender );
				ScriptEngine::Set_Objective_HUD_Info( 500, 95, "POG_M04_2_02.tga", IDS_POG_DESTROY );
			}

			else if (param == announce_first_mate_objective && first_mate_primary_active != true && first_mate_primary_triggered == false)//
			{
				first_mate_primary_triggered = true;
				first_mate_primary_active = true;

				firstmateConv = ScriptEngine::Create_Conversation( "M04_Add_FirstMate_Objective_JDG", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, firstmateConv, false, false, true );
				ScriptEngine::Start_Conversation( firstmateConv,  firstmateConv );
				ScriptEngine::Monitor_Conversation (obj, firstmateConv);

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 3, 0 );
				}
			}

			else if (param == announce_captain_objective)//M04_Add_Captains_Key_Objective_Conversation
			{
				int captainConv = ScriptEngine::Create_Conversation( "M04_Add_Captains_Key_Objective_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, captainConv, false, false, true );
				ScriptEngine::Start_Conversation( captainConv,  captainConv );

				captain_primary_active = true;
				//ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);
				ScriptEngine::Add_Objective( 700, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M04_05, nullptr, IDS_Enc_Obj_Primary_M04_05 );
				ScriptEngine::Add_Radar_Marker ( 701, ScriptEngine::Get_Position( ScriptEngine::Find_Object ( M04_SHIPS_CAPTAIN_JDG )), RADAR_BLIP_SHAPE_OBJECTIVE, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 1, 4 );//this sets a small delay before havoc responds

				GameObject * captain = ScriptEngine::Find_Object ( M04_SHIPS_CAPTAIN_JDG );
				if (captain != nullptr)
				{
					Vector3 captainLocation = ScriptEngine::Get_Position(captain);
					ScriptEngine::Set_Objective_HUD_Info_Position( 700, 94, "POG_M04_1_05.tga", IDS_POG_ACQUIRE, captainLocation );
				}

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 4, 0 );
				}
			}

			else if (param == announce_get_back_to_sub)//
			{
				protectPOWsConv = ScriptEngine::Create_Conversation( "M04_Protect_The_Prisoners_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, protectPOWsConv, false, false, true );
				ScriptEngine::Start_Conversation( protectPOWsConv,  protectPOWsConv );
				ScriptEngine::Monitor_Conversation (obj, protectPOWsConv);

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 6, 0 );
				}

				//ScriptEngine::Create_Sound ( "00-n040e", Vector3 (0,0,0), obj);

			}
/**********************************************************************************************************
The following are params for when the individual objectives are completed.
***********************************************************************************************************/
			else if (param == completed_prisoner_rescue)
			{
				prisoner_primary_active = false;
				ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 100, OBJECTIVE_STATUS_ACCOMPLISHED );
				ScriptEngine::Clear_Radar_Marker ( 101 );
				ScriptEngine::Clear_Radar_Marker ( 102 );
				ScriptEngine::Clear_Radar_Marker ( 103 );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 2, 2 );//this sets a small delay before havoc responds

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_REMOVE_OBJECTIVE_POG_JDG, 0, 0 );
				}
			}

			else if (param == completed_get_the_key)
			{
				ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 110, OBJECTIVE_STATUS_ACCOMPLISHED );
				ScriptEngine::Clear_Radar_Marker ( 111 );

				int cargo_conv1 = ScriptEngine::Create_Conversation( "M04_NowGoFreeThePrisoners_Conversation", 100, 1000, false);
				//ScriptEngine::Join_Conversation( nullptr, cargo_conv1, false, false, false);
				ScriptEngine::Join_Conversation( STAR, cargo_conv1, false, false, false );
				ScriptEngine::Start_Conversation( cargo_conv1,  cargo_conv1 );

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_REMOVE_OBJECTIVE_POG_JDG, 2, 0 );
				}
			}

			else if (param == completed_engine_room_objective)
			{
				//ScriptEngine::Create_Sound ( "00-n050e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 200, OBJECTIVE_STATUS_ACCOMPLISHED );

				ScriptEngine::Create_Object ( "POW_Armor_100", Vector3 (0.039f, -85.758f, -14.999f));
				ScriptEngine::Create_Object ( "POW_Health_100", Vector3 (0.014f, -98.215f, -14.999f));


				ScriptEngine::Create_Object ( "POW_GrenadeLauncher_Player", Vector3 (0.020f, -89.237f, -14.999f));
				ScriptEngine::Create_Object ( "POW_GrenadeLauncher_Player", Vector3 (0.050f, -92.541f, -14.999f));
				ScriptEngine::Create_Object ( "POW_GrenadeLauncher_Player", Vector3 (0.023f, -95.280f, -14.999f));
			}

			else if (param == completed_missile_room_objective)
			{
				missile_primary_active = false;
				ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 300, OBJECTIVE_STATUS_ACCOMPLISHED );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 2, 2 );//this sets a small delay before havoc responds

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_REMOVE_OBJECTIVE_POG_JDG, 1, 0 );
				}
			}

			else if (param == completed_torpedo_room_objective)
			{
				torpedo_primary_active = false;
				ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 400, OBJECTIVE_STATUS_ACCOMPLISHED );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 2, 2 );//this sets a small delay before havoc responds

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_REMOVE_OBJECTIVE_POG_JDG, 5, 0 );
				}
			}

			else if (param == completed_apache_objective)
			{
				ScriptEngine::Create_Sound ( "00-n050e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 500, OBJECTIVE_STATUS_ACCOMPLISHED );
			}

			else if (param == completed_first_mate_objective)
			{
				first_mate_primary_triggered = true;
				first_mate_primary_active = false;
				ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 600, OBJECTIVE_STATUS_ACCOMPLISHED );
				ScriptEngine::Clear_Radar_Marker ( 601 );
				//ScriptEngine::Send_Custom_Event( obj, obj, 0, 2, 2 );//this sets a small delay before havoc responds

				GameObject * shipCaptain = ScriptEngine::Find_Object ( M04_SHIPS_CAPTAIN_JDG );
				if (shipCaptain != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, announce_captain_objective, 6 );
				}

				else if (shipCaptain == nullptr)
				{
					//ScriptEngine::Send_Custom_Event( obj, obj, 0, announce_captain_objective, 6 );
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_captain_objective, 6 );
				}

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_REMOVE_OBJECTIVE_POG_JDG, 3, 0 );
				}
			}

			else if (param == completed_captain_objective)
			{
				captain_primary_active = false;
				//ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Set_Objective_Status( 700, OBJECTIVE_STATUS_ACCOMPLISHED );
				ScriptEngine::Clear_Radar_Marker ( 701 );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 2, 2 );//this sets a small delay before havoc responds

				GameObject * objectiveReminder = ScriptEngine::Find_Object ( 105760 );
				if (objectiveReminder != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_REMOVE_OBJECTIVE_POG_JDG, 4, 0 );
				}

				if (prisoner_primary_active == false)
				{
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Rally_Round_The_Sub_Boy_Conversation", 50, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );

					objectiveReminder = ScriptEngine::Find_Object ( 105760 );
					if (objectiveReminder != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, objectiveReminder, M01_ADD_OBJECTIVE_POG_JDG, 6, 0 );
					}
				}

				else if (prisoner_primary_active == true)
				{
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Mission_Start_Conversation", 50, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

			}

			else if (param == M01_END_MISSION_PASS_JDG)
			{
				ScriptEngine::Create_Sound ( "00-n048e", Vector3 (0,0,0), obj);
				ScriptEngine::Mission_Complete ( true );
			}

			else if (param == M01_DO_END_MISSION_CHECK_JDG)
			{
				if ((mission_started == true) && (captain_primary_active == false) && (first_mate_primary_active == false) && (torpedo_primary_active == false) && (missile_primary_active == false) && (prisoner_primary_active == false))
				{
					//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101194), 0, M01_START_ACTING_JDG, 0 );
					ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_END_MISSION_PASS_JDG, 0 );
				}
			}
/**********************************************************************************************************
The following are params for when the individual missiles and whatnot get acted on or destroyed.
***********************************************************************************************************/
			else if (param == missile_01_sabotaged)//
			{
				//ScriptEngine::Clear_Radar_Marker ( 301 );
				number_missiles_sabotaged++;

				if (number_missiles_sabotaged == 1)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_01_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 2)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_02_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 3)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_03_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged >= 4)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_missile_room_objective, 0 );

					GameObject * bigSamZone = ScriptEngine::Find_Object ( 104942 );
					if (bigSamZone != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, bigSamZone, 0, M01_START_ACTING_JDG, 0 );
					}
				}
			}

			else if (param == missile_02_sabotaged)
			{
				//ScriptEngine::Clear_Radar_Marker ( 302 );
				number_missiles_sabotaged++;

				if (number_missiles_sabotaged == 1)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_01_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 2)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_02_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 3)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_03_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged >= 4)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_missile_room_objective, 0 );

					GameObject * bigSamZone = ScriptEngine::Find_Object ( 104942 );
					if (bigSamZone != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, bigSamZone, 0, M01_START_ACTING_JDG, 0 );
					}
				}
			}

			else if (param == missile_03_sabotaged)
			{
				//ScriptEngine::Clear_Radar_Marker ( 303 );
				number_missiles_sabotaged++;

				if (number_missiles_sabotaged == 1)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_01_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 2)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_02_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 3)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_03_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged >= 4)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_missile_room_objective, 0 );

					GameObject * bigSamZone = ScriptEngine::Find_Object ( 104942 );
					if (bigSamZone != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, bigSamZone, 0, M01_START_ACTING_JDG, 0 );
					}
				}
			}

			else if (param == missile_04_sabotaged)
			{
				//ScriptEngine::Clear_Radar_Marker ( 304 );
				number_missiles_sabotaged++;

				if (number_missiles_sabotaged == 1)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_01_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 2)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_02_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged == 3)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_MissileRack_03_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_missiles_sabotaged >= 4)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_missile_room_objective, 0 );

					GameObject * bigSamZone = ScriptEngine::Find_Object ( 104942 );
					if (bigSamZone != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, bigSamZone, 0, M01_START_ACTING_JDG, 0 );
					}
				}
			}

			else if (param == engine_room_01_sabotaged)
			{
				number_engines_sabotaged++;

				if (number_engines_sabotaged == 1)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0040I1EVAG_SND", Vector3 (0,0,0), obj);
				}

				else if (number_engines_sabotaged == 2)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0090I1EVAN_SND", Vector3 (0,0,0), obj);//intruder detected
				}

				else if (number_engines_sabotaged == 3)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0094I1EVAN_SND", Vector3 (0,0,0), obj);//engine failure emiment
				}

				else if (number_engines_sabotaged >= 4)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0095I1EVAN_SND", Vector3 (0,0,0), obj);//engine off line
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_engine_room_objective, 0 );
				}
			}

			else if (param == engine_room_02_sabotaged)
			{
				number_engines_sabotaged++;

				if (number_engines_sabotaged == 1)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0040I1EVAG_SND", Vector3 (0,0,0), obj);
				}

				else if (number_engines_sabotaged == 2)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0090I1EVAN_SND", Vector3 (0,0,0), obj);//intruder detected
				}

				else if (number_engines_sabotaged == 3)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0094I1EVAN_SND", Vector3 (0,0,0), obj);//engine failure emiment
				}

				else if (number_engines_sabotaged >= 4)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0095I1EVAN_SND", Vector3 (0,0,0), obj);//engine off line
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_engine_room_objective, 0 );
				}
			}

			else if (param == engine_room_03_sabotaged)
			{
				number_engines_sabotaged++;

				if (number_engines_sabotaged == 1)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0040I1EVAG_SND", Vector3 (0,0,0), obj);
				}

				else if (number_engines_sabotaged == 2)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0090I1EVAN_SND", Vector3 (0,0,0), obj);//intruder detected
				}

				else if (number_engines_sabotaged == 3)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0094I1EVAN_SND", Vector3 (0,0,0), obj);//engine failure emiment
				}

				else if (number_engines_sabotaged >= 4)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0095I1EVAN_SND", Vector3 (0,0,0), obj);//engine off line
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_engine_room_objective, 0 );
				}
			}

			else if (param == engine_room_04_sabotaged)
			{
				number_engines_sabotaged++;

				if (number_engines_sabotaged == 1)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0040I1EVAG_SND", Vector3 (0,0,0), obj);
				}

				else if (number_engines_sabotaged == 2)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0090I1EVAN_SND", Vector3 (0,0,0), obj);//intruder detected
				}

				else if (number_engines_sabotaged == 3)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0094I1EVAN_SND", Vector3 (0,0,0), obj);//engine failure emiment
				}

				else if (number_engines_sabotaged >= 4)
				{
					ScriptEngine::Create_Sound ( "M04DSGN_DSGN0095I1EVAN_SND", Vector3 (0,0,0), obj);//engine off line
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_engine_room_objective, 0 );
				}
			}

			else if (param == torpedo_01_sabotaged)
			{
				ScriptEngine::Clear_Radar_Marker ( 401 );
				number_torpedos_sabotaged++;

				if (number_torpedos_sabotaged < 2)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_Torpedo_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_torpedos_sabotaged >= 2)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_torpedo_room_objective, 0 );
				}
			}

			else if (param == torpedo_02_sabotaged)
			{
				ScriptEngine::Clear_Radar_Marker ( 402 );
				number_torpedos_sabotaged++;

				if (number_torpedos_sabotaged < 2)
				{
					int missile_conv1 = ScriptEngine::Create_Conversation( "M04_Torpedo_Sabotaged_Conversation", 100, 1000, true);
					ScriptEngine::Join_Conversation( nullptr, missile_conv1, false, false, false);
					ScriptEngine::Start_Conversation( missile_conv1,  missile_conv1 );
				}

				else if (number_torpedos_sabotaged >= 2)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, completed_torpedo_room_objective, 0 );
				}
			}
		}
	}
};

/*****************************************************************************************************************
The following are the scripts for the dudes in the sub bay when the mission first starts.
*****************************************************************************************************************/
DECLARE_SCRIPT(M04_PointGuard_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (obj)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}
	}
};

DECLARE_SCRIPT(M04_A01_Sniper_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(-0.238f, 0.743f, 5.034f), WALK, .25f );

		ScriptEngine::Action_Goto( obj, params );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		ScriptEngine::Action_Reset ( obj, 100 );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );

				params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
				params.Set_Movement( Vector3(0,0,0), WALK, .25f );
				params.WaypathID = 100167;
				params.WaypointStartID = 100168;
				params.WaypointEndID = 100169;
				ScriptEngine::Action_Goto( obj, params );
			}

			else if (action_id == M01_WALKING_WAYPATH_02_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );

				params.Set_Basic( this, 45, M01_DOING_ANIMATION_02_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_02_JDG)
			{
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_03_JDG );
				params.Set_Movement( Vector3(0,0,0), WALK, .25f );
				params.WaypathID = 100171;
				params.WaypointStartID = 100172;
				params.WaypointEndID = 100175;
				ScriptEngine::Action_Goto( obj, params );
			}

			else if (action_id == M01_WALKING_WAYPATH_03_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );

				params.Set_Basic( this, 45, M01_DOING_ANIMATION_03_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_04_JDG );
				params.Set_Movement( Vector3(0,0,0), WALK, .25f );
				params.WaypathID = 100177;
				params.WaypointStartID = 100178;
				params.WaypointEndID = 100179;
				ScriptEngine::Action_Goto( obj, params );
			}

			else if (action_id == M01_WALKING_WAYPATH_04_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );

				params.Set_Basic( this, 45, M01_DOING_ANIMATION_04_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_04_JDG)
			{
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_05_JDG );
				params.Set_Movement( Vector3(0,0,0), WALK, .25f );
				params.WaypathID = 100181;
				params.WaypointStartID = 100182;
				params.WaypointEndID = 100183;
				ScriptEngine::Action_Goto( obj, params );
			}

			else if (action_id == M01_WALKING_WAYPATH_05_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );

				params.Set_Basic( this, 45, M01_DOING_ANIMATION_05_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_05_JDG)
			{
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
				params.Set_Movement( Vector3(0,0,0), WALK, .25f );
				params.WaypathID = 100167;
				params.WaypointStartID = 100168;
				params.WaypointEndID = 100169;
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}
};

DECLARE_SCRIPT(M04_A01_PatrolGuy_01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 5 );
	}
};
/*********************************************************************************************************
The following scripts all deal with the first time through the cargo and missile rooms.
*************************************************************************************************************/
DECLARE_SCRIPT(M04_MissileRoom_EnterZone_Left_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR  )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 600, 0 );//have leftside guard come in
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 200, 0 );//put missile room guys to 50% priority

			GameObject * zone02 = ScriptEngine::Find_Object (100583);//stays
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}

			GameObject * zone04 = ScriptEngine::Find_Object (100585);
			if (zone04 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone04 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_EnterZone_Right_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR  )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 500, 0 );//have rightside guard come in
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 200, 0 );//put missile room guys to 50% priority

			GameObject * zone02 = ScriptEngine::Find_Object (100583);//stays
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}

			GameObject * zone04 = ScriptEngine::Find_Object (100585);//stays
			if (zone04 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone04 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_CargoMissileRooms_Dude_Controller_JDG, "")//this guys ID number is M04_CARGOHOLD_CONTROLLER_JDG 100558
{
	int missileRoomguardOne_ID;
	int missileRoomguardTwo_ID;
	int guardFive_ID;
	int upperGuard01_ID;
	int upperGuard02_ID;

	//int cargo_conv1;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( missileRoomguardOne_ID, 1 );
		SAVE_VARIABLE( missileRoomguardTwo_ID, 2 );
		SAVE_VARIABLE( guardFive_ID, 3 );
		SAVE_VARIABLE( upperGuard01_ID, 4 );
		SAVE_VARIABLE( upperGuard02_ID, 5 );
		//SAVE_VARIABLE( cargo_conv1, 6 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)
		{
			GameObject *guardOne = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_01_JDG );
			GameObject *guardTwo = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_02_JDG );
			GameObject *guardThree = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_03_JDG );

			if ((guardOne != nullptr) && (guardTwo != nullptr) && (guardThree != nullptr))//
			{
				//cargo_conv1 = ScriptEngine::Create_Conversation( "M04_CargoHold_Conversation_01", 45, 1000, true);
				//ScriptEngine::Join_Conversation( guardOne, cargo_conv1, false, false, false);
				//ScriptEngine::Join_Conversation( guardTwo, cargo_conv1, false, false, false );
				//ScriptEngine::Start_Conversation( cargo_conv1,  cargo_conv1 );
				//ScriptEngine::Monitor_Conversation(  obj, cargo_conv1);
			}

			GameObject *guardSix = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-14.44f, -40.82f, -9.00f));
			ScriptEngine::Attach_Script(guardSix, "M04_CargoHold_Blackhand_02_JDG", "");//M01_Hunt_The_Player_JDG

			GameObject *missileRoomguardOne = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(-13.38f, -53.56f, -15.00f));
			ScriptEngine::Attach_Script(missileRoomguardOne, "M04_MissileRoom_Guard_01_JDG", "");

			GameObject *missileRoomguardTwo = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(13.37f, -61.28f, -15.00f));
			ScriptEngine::Attach_Script(missileRoomguardTwo, "M04_MissileRoom_Guard_02_JDG", "");

			GameObject *missileRoomUpperguardOne = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(9.485f, -66.975f, -9.00f));
			ScriptEngine::Attach_Script(missileRoomUpperguardOne, "M04_MissileRoom_UpperGuard_01_JDG", "");
			ScriptEngine::Set_Facing ( missileRoomUpperguardOne, 85 );
			upperGuard01_ID = ScriptEngine::Get_ID ( missileRoomUpperguardOne );

			GameObject *missileRoomUpperguardTwo = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(-9.456f, -61.862f, -9.00f));
			ScriptEngine::Attach_Script(missileRoomUpperguardTwo, "M04_MissileRoom_UpperGuard_02_JDG", "");
			ScriptEngine::Set_Facing ( missileRoomUpperguardTwo, 95 );
			upperGuard02_ID = ScriptEngine::Get_ID ( missileRoomUpperguardTwo );

			missileRoomguardOne_ID = ScriptEngine::Get_ID ( missileRoomguardOne );
			missileRoomguardTwo_ID = ScriptEngine::Get_ID ( missileRoomguardTwo );
		}

		else if (param == 200)//this sets the missile guards priority to 50 probably from 100
		{
			GameObject *missileRoomguardOne = ScriptEngine::Find_Object ( missileRoomguardOne_ID );
			GameObject *missileRoomguardTwo = ScriptEngine::Find_Object ( missileRoomguardTwo_ID );

			if (missileRoomguardOne != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, missileRoomguardOne, 200, 50, 0 );
			}

			if (missileRoomguardTwo != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, missileRoomguardTwo, 200, 50, 0 );
			}
		}

		else if (param == 300)//this is when one of the missile room guards dies
		{
			float reinforceka = ScriptEngine::Get_Random ( 0.5, 2.5 );

			if ((reinforceka >= 0.5) && (reinforceka < 1.5))
			{
				ScriptEngine::Trigger_Spawner( 100579 );
				ScriptEngine::Trigger_Spawner( 100581 );
			}

			else
			{
				ScriptEngine::Trigger_Spawner( 100581 );
			}
		}

		else if (param == 500)//user is going through the right side door into missile room
		{
			GameObject *upperGuard01 = ScriptEngine::Find_Object ( upperGuard01_ID );
			GameObject *upperGuard02 = ScriptEngine::Find_Object ( upperGuard02_ID );

			if (upperGuard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, upperGuard01, 0, M01_START_ATTACKING_01_JDG, 0 );
			}

			if (upperGuard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, upperGuard02, 0, M01_START_ACTING_JDG, 0 );
			}
		}

		else if (param == 600)//user is going through the left side door into missile room
		{
			GameObject *upperGuard01 = ScriptEngine::Find_Object ( upperGuard01_ID );
			GameObject *upperGuard02 = ScriptEngine::Find_Object ( upperGuard02_ID );

			if (upperGuard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, upperGuard01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (upperGuard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, upperGuard02, 0, M01_START_ATTACKING_01_JDG, 0 );
			}
		}

		else if (param == 800)//Officer has ragged on grunts--tell them to salute
		{
			GameObject * guardOne = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_01_JDG );
			if (guardOne != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 0, 1.5f );
				ScriptEngine::Send_Custom_Event( obj, guardOne, 0, 100, delayTimer );
			}

			GameObject * guardTwo = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_02_JDG );
			if (guardTwo != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 0, 1.5f );
				ScriptEngine::Send_Custom_Event( obj, guardTwo, 0, 100, delayTimer );
			}

			GameObject * guardThree = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_03_JDG );
			if (guardThree != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 0, 1.5f );
				ScriptEngine::Send_Custom_Event( obj, guardThree, 0, 100, delayTimer );
			}
		}
	}

	/*void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			if (action_id == cargo_conv1)
			{
				GameObject *blackHand01 = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );

				if (blackHand01 != nullptr)
				{
					ScriptEngine::Debug_Message ( "**********************cargo conv 01 is over--sending custom to blackhand guy\n" );
					ScriptEngine::Send_Custom_Event( obj, blackHand01, 0, M01_START_ACTING_JDG, 0 );
				}
			}
		}
	}*/
};


DECLARE_SCRIPT(M04_MissileRoom_UpperGuard_01_JDG, "")//left side
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		//ScriptEngine::Innate_Soldier_Enable_Actions ( obj, false );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)//player has entered area--back peddle towards the engine room
		{
			Vector3 gotoSpot (4.302f, -67.224f, -9);

			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
			params.Set_Movement( gotoSpot, RUN, .25f );
			params.MoveBackup = true;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (param == M01_START_ATTACKING_01_JDG)//player has entered area--attack him
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			//ScriptEngine::Innate_Soldier_Enable_Actions ( obj, true );
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason /* complete_reason */) override
	{
		if (action_id == M01_WALKING_WAYPATH_01_JDG)//you've reached your final guard spot--goto stationary
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG", "");
			//ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_UpperGuard_02_JDG, "")//right side
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		//ScriptEngine::Innate_Soldier_Enable_Actions ( obj, false );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)//player has entered area--back peddle towards the engine room
		{
			Vector3 gotoSpot (-4.336f, -66.975f, -9);

			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
			params.Set_Movement( gotoSpot, RUN, .25f );
			params.MoveBackup = true;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (param == M01_START_ATTACKING_01_JDG)//player has entered area--attack him
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			//ScriptEngine::Innate_Soldier_Enable_Actions ( obj, true );
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason /* complete_reason */) override
	{
		if (action_id == M01_WALKING_WAYPATH_01_JDG)//you've reached your final guard spot--goto stationary
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG", "");
			//ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}
};

DECLARE_SCRIPT(M04_CargoHold_TalkGuy01_JDG, "")//M04_CARGO_TALKGUY_01_JDG  101463
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject *blackhand = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );
		if (blackhand != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, blackhand,  0, M01_START_ATTACKING_01_JDG, 0.5f );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)//you've been ragged on--salute then carry on
			{
				GameObject *blackhand = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );
				if (blackhand != nullptr)
				{
					params.Set_Basic(this, 100, M01_START_ACTING_JDG);
					params.Set_Attack( blackhand, 0, 0, true );

					ScriptEngine::Action_Attack ( obj, params );

					float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
					ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_01_JDG);
				params.Set_Animation("H_A_J19A", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;
		Vector3 myGotoSpot (-9.192f, 15.344f, -8.998f);

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_02_JDG);
				params.Set_Animation("H_A_J19S", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_02_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_03_JDG);
				params.Set_Animation("H_A_J19C", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic(this, 80, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement( myGotoSpot, WALK, 3, false );
				ScriptEngine::Action_Goto ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myGotoSpot, 10 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_CargoHold_TalkGuy02_JDG, "")//M04_CARGO_TALKGUY_02_JDG  101464
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject *blackhand = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );
		if (blackhand != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, blackhand,  0, M01_START_ATTACKING_01_JDG, 0.5f );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)//you've been ragged on--salute then carry on
			{
				GameObject *blackhand = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );
				if (blackhand != nullptr)
				{
					params.Set_Basic(this, 100, M01_START_ACTING_JDG);
					params.Set_Attack( blackhand, 0, 0, true );

					ScriptEngine::Action_Attack ( obj, params );

					float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
					ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_01_JDG);
				params.Set_Animation("H_A_J19A", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;
		Vector3 myGotoSpot (11.168f, 19.847f, -15);

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_02_JDG);
				params.Set_Animation("H_A_J19S", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_02_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_03_JDG);
				params.Set_Animation("H_A_J19C", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic(this, 80, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement( myGotoSpot, WALK, 3, false );
				ScriptEngine::Action_Goto ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myGotoSpot, 10 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_CargoHold_TalkGuy03_JDG, "")//M04_CARGO_TALKGUY_03_JDG  101465
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );

		GameObject *guardOne = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_01_JDG );
		if (guardOne != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic(this, 100, M01_START_ACTING_JDG);
			params.Set_Attack( guardOne, 0, 0, true );

			ScriptEngine::Action_Attack ( obj, params );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject *blackhand = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );
		if (blackhand != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, blackhand,  0, M01_START_ATTACKING_01_JDG, 0.5f );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)//you've been ragged on--salute then carry on
			{
				GameObject *blackhand = ScriptEngine::Find_Object ( M04_CARGO_BLACKHAND_01_JDG );
				if (blackhand != nullptr)
				{
					params.Set_Basic(this, 100, M01_START_ACTING_JDG);
					params.Set_Attack( blackhand, 0, 0, true );

					ScriptEngine::Action_Attack ( obj, params );

					float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
					ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_01_JDG);
				params.Set_Animation("H_A_J19A", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;
		Vector3 myGotoSpot (-0.099f, -71.767f, -15);

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_02_JDG);
				params.Set_Animation("H_A_J19S", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_02_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_03_JDG);
				params.Set_Animation("H_A_J19C", false );
				ScriptEngine::Action_Play_Animation ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic(this, 80, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement( myGotoSpot, WALK, 3, false );
				ScriptEngine::Action_Goto ( obj, params );
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myGotoSpot, 10 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_Guard_01_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_action_id, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
		params.WaypathID = 100555;
		params.WaypointStartID = 100556;
		params.WaypointEndID = 100557;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 300, 0 );//tell controller youre dead
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100541;
			params.WaypointStartID = 100542;
			params.WaypointEndID = 100544;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100546;
			params.WaypointStartID = 100547;
			params.WaypointEndID = 100548;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 104 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 105 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100550;
			params.WaypointStartID = 100551;
			params.WaypointEndID = 100553;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 106 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 107 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100555;
			params.WaypointStartID = 100556;
			params.WaypointEndID = 100557;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_Guard_02_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_action_id, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
		params.WaypathID = 100546;
		params.WaypointStartID = 100547;
		params.WaypointEndID = 100548;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 300, 0 );//tell controller you're dead
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100550;
			params.WaypointStartID = 100551;
			params.WaypointEndID = 100553;
			ScriptEngine::Action_Goto( obj, params );
		}

		if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 50, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100555;
			params.WaypointStartID = 100556;
			params.WaypointEndID = 100557;
			ScriptEngine::Action_Goto( obj, params );
		}

		if (action_id == 104 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 105 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100541;
			params.WaypointStartID = 100542;
			params.WaypointEndID = 100544;
			ScriptEngine::Action_Goto( obj, params );
		}

		if (action_id == 106 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 107 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 100546;
			params.WaypointStartID = 100547;
			params.WaypointEndID = 100548;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_CargoHold_Blackhand_01_JDG, "")//M04_CARGO_BLACKHAND_01_JDG 101534
{
	int cargo_conv2;
	bool idle;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( cargo_conv2, 1 );
		SAVE_VARIABLE( idle, 2 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		idle = true;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				idle = false;
				ScriptEngine::Action_Reset ( obj, 100 );
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				GameObject *guardOne = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_01_JDG );
				if (guardOne != nullptr)
				{
					params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( guardOne, WALK, 3.0f );
					ScriptEngine::Action_Goto( obj, params );
				}

				else
				{
					Vector3 myPosition = ScriptEngine::Get_Position ( obj );
					ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 20 );
				}
			}

			else if (param == M01_START_ATTACKING_01_JDG && idle == true)
			{
				idle = false;
				ScriptEngine::Action_Reset ( obj, 100 );
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic( this, 75, M01_WALKING_WAYPATH_03_JDG );
				params.Set_Movement( STAR, RUN, 1 );
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				/*GameObject *guardOne = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_01_JDG );
				if (guardOne != nullptr)
				{
					cargo_conv2 = ScriptEngine::Create_Conversation( "M04_CargoHold_Conversation_02", 50, 1000, true);
					ScriptEngine::Join_Conversation( obj, cargo_conv2, false, false );
					ScriptEngine::Join_Conversation( guardOne, cargo_conv2, false, false );
					ScriptEngine::Start_Conversation( cargo_conv2,  cargo_conv2 );
					ScriptEngine::Monitor_Conversation(  obj, cargo_conv2);
				}*/
			}

			else if (action_id == M01_WALKING_WAYPATH_02_JDG)
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 20 );
			}
		}

		else if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			if (action_id == cargo_conv2)
			{
				GameObject *guardOne = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_01_JDG );
				GameObject *guardTwo = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_02_JDG );
				GameObject *guardThree = ScriptEngine::Find_Object ( M04_CARGO_TALKGUY_03_JDG );

				if (guardOne != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
					ScriptEngine::Send_Custom_Event ( obj, guardOne, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (guardTwo != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
					ScriptEngine::Send_Custom_Event ( obj, guardTwo, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (guardThree != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
					ScriptEngine::Send_Custom_Event ( obj, guardThree, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
				params.Set_Movement( Vector3 (10.075f, -44.802f, -15), WALK, 1.5f );
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}
};

DECLARE_SCRIPT(M04_CargoHold_Blackhand_02_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_action_id, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
		params.WaypathID = 101018;
		params.WaypointStartID = 101019;
		params.WaypointEndID = 101022;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 101024;
			params.WaypointStartID = 101025;
			params.WaypointEndID = 101028;
			ScriptEngine::Action_Goto( obj, params );
		}

		if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 101024;
			params.WaypointStartID = 101028;
			params.WaypointEndID = 101025;
			ScriptEngine::Action_Goto( obj, params );
		}

		if (action_id == 104 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		if (action_id == 105 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, 1.5f );
			params.WaypathID = 101018;
			params.WaypointStartID = 101022;
			params.WaypointEndID = 101019;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};
/****************************************************************************************************************
The following scripts deal with the engine room--first time through
****************************************************************************************************************/
DECLARE_SCRIPT(M04_EngineRoom_EnterZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR  )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 430, 0 );//tell objective controller to announce objective
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 700, 0 );//tell cargo/missile contoller to turn off spawners

			GameObject * zone01 = ScriptEngine::Find_Object (100425);//stays
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject * zone04 = ScriptEngine::Find_Object (100428);//stays
			if (zone04 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone04 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_BuildingController_JDG, "")//M04_ENGINEROOM_BUILDING_CONTROLLER_JDG	150001
{
	int targetsDestroyed;
	int engineroom_sound01;
	int engineroom_sound02;
	int engineroom_sound03;
	int engineroom_sound04;
	int engineroom_sound05;
	int engineroom_sound06;
	int engineroom_sound07;
	int engineroom_sound08;
	int engineroom_sound09;
	int engineroom_sound10;
	int engineroom_sound11;
	int engineroom_sound12;
	int engineroom_sound13;
	int engineroom_sound14;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( targetsDestroyed, 1 );
		SAVE_VARIABLE( engineroom_sound01, 2 );
		SAVE_VARIABLE( engineroom_sound02, 3 );
		SAVE_VARIABLE( engineroom_sound03, 4 );
		SAVE_VARIABLE( engineroom_sound04, 5 );
		SAVE_VARIABLE( engineroom_sound05, 6 );
		SAVE_VARIABLE( engineroom_sound06, 7 );
		SAVE_VARIABLE( engineroom_sound07, 8 );
		SAVE_VARIABLE( engineroom_sound08, 9 );
		SAVE_VARIABLE( engineroom_sound09, 10 );
		SAVE_VARIABLE( engineroom_sound10, 11 );
		SAVE_VARIABLE( engineroom_sound11, 12 );
		SAVE_VARIABLE( engineroom_sound12, 13 );
		SAVE_VARIABLE( engineroom_sound13, 14 );
		SAVE_VARIABLE( engineroom_sound14, 15 );
	}

	void Created( GameObject * obj ) override
	{
		targetsDestroyed = 0;
		engineroom_sound01 = ScriptEngine::Create_Sound ( "SFX.Machine_Turbine_01", Vector3 (5.970f, -104.759f, -15.143f), obj );
		engineroom_sound02 = ScriptEngine::Create_Sound ( "SFX.Machine_Turbine_01_offset2", Vector3 (-6.021f, -104.445f, -15.157f), obj );
		engineroom_sound03 = ScriptEngine::Create_Sound ( "SFX.Ship_Amb_Engineroom", Vector3 (-0.942f, -74.017f, -11.840f), obj );
		engineroom_sound04 = ScriptEngine::Create_Sound ( "SFX.Steam_Med_Pressure_01_offset1", Vector3 (-9.838f, -76.426f, -13.498f), obj );
		engineroom_sound05 = ScriptEngine::Create_Sound ( "Ship_Engine_01", Vector3 (-10.846f, -94.205f, -13.432f), obj );
		engineroom_sound06 = ScriptEngine::Create_Sound ( "Ship_Engine_01", Vector3 (0.359f, -131.034f, 2.580f), obj );
		engineroom_sound07 = ScriptEngine::Create_Sound ( "Ship_Engine_01_offset1", Vector3 (9.045f, -96.538f, -13.366f), obj );
		engineroom_sound08 = ScriptEngine::Create_Sound ( "Ship_Engine_02", Vector3 (9.192f, -89.680f, -12.902f), obj );
		engineroom_sound09 = ScriptEngine::Create_Sound ( "Ship_Engine_02", Vector3 (3.033f, -92.739f, 16.208f), obj );
		engineroom_sound10 = ScriptEngine::Create_Sound ( "Ship_Engine_03", Vector3 (-5.870f, -84.200f, -13.423f), obj );
		engineroom_sound11 = ScriptEngine::Create_Sound ( "Ship_Engine_03", Vector3 (7.588f, -83.956f, -13.481f), obj );
		engineroom_sound12 = ScriptEngine::Create_Sound ( "Steam_Med_Pressure_01", Vector3 (9.726f, -78.736f, -14.079f), obj );
		engineroom_sound13 = ScriptEngine::Create_Sound ( "SFX.Machine_Turbine_01", Vector3 (-0.757f, -60.372f, -7.924f), obj );
		engineroom_sound14 = ScriptEngine::Create_Sound ( "Roomtone_Industrial_04_verb", Vector3 (8.568f, -67.182f, -6.454f), obj );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)
			{
				targetsDestroyed++;

				if (targetsDestroyed == 4)
				{
					ScriptEngine::Stop_Sound ( engineroom_sound01, true);
					ScriptEngine::Stop_Sound ( engineroom_sound02, true);
					ScriptEngine::Stop_Sound ( engineroom_sound03, true);
					ScriptEngine::Stop_Sound ( engineroom_sound04, true);
					ScriptEngine::Stop_Sound ( engineroom_sound05, true);
					ScriptEngine::Stop_Sound ( engineroom_sound06, true);
					ScriptEngine::Stop_Sound ( engineroom_sound07, true);
					ScriptEngine::Stop_Sound ( engineroom_sound08, true);
					ScriptEngine::Stop_Sound ( engineroom_sound09, true);
					ScriptEngine::Stop_Sound ( engineroom_sound10, true);
					ScriptEngine::Stop_Sound ( engineroom_sound11, true);
					ScriptEngine::Stop_Sound ( engineroom_sound12, true);
					ScriptEngine::Stop_Sound ( engineroom_sound13, true);
					ScriptEngine::Stop_Sound ( engineroom_sound14, true);

					engineroom_sound01 = ScriptEngine::Create_Sound ( "SFX.Klaxon_Alert_02", Vector3 (-2.309f, -94.553f, -11.050f), obj );
					engineroom_sound02 = ScriptEngine::Create_Sound ( "SFX.Klaxon_Alert_04", Vector3 (-0.715f, -88.220f, -10.745f), obj );
					engineroom_sound03 = ScriptEngine::Create_Sound ( "SFX.Ship_Engine_Broken_01", Vector3 (9.731f, -95.172f, -11.028f), obj );
					engineroom_sound04 = ScriptEngine::Create_Sound ( "SFX.Ship_Engine_Broken_01", Vector3 (-2.976f, -91.511f, -12.275f), obj );
					engineroom_sound05 = ScriptEngine::Create_Sound ( "SFX.Ship_Engine_Broken_02", Vector3 (3.918f, -84.200f, -11.625f), obj );
					engineroom_sound06 = ScriptEngine::Create_Sound ( "SFX.Ship_Engine_Broken_02", Vector3 (-9.033f, -96.617f, -12.678f), obj );
					engineroom_sound07 = ScriptEngine::Create_Sound ( "SFX.Ship_Engine_Broken_03", Vector3 (2.750f, -93.408f, -12.047f), obj );
					engineroom_sound08 = ScriptEngine::Create_Sound ( "SFX.Ship_Engine_Broken_03", Vector3 (-7.275f, -83.744f, -12.941f), obj );
					engineroom_sound09 = ScriptEngine::Create_Sound ( "Ship_Engine_01", Vector3 (6.854f, -75.227f, -14.270f), obj );
					engineroom_sound10 = ScriptEngine::Create_Sound ( "Ship_Engine_02", Vector3 (-9.464f, -76.808f, -15.745f), obj );

					ScriptEngine::Debug_Message ( "**********************all engine room targets destroyed--applying damage to engine room\n" );
					//ScriptEngine::Set_Health ( obj, 1 );
					ScriptEngine::Apply_Damage( obj, 1000, "BlamoKiller", STAR );

					ScriptEngine::Create_2D_Sound ( "SFX.Ship_Engine_Stop" );
					//ScriptEngine::Create_2D_Sound ( "M04_EngineRoom_Alarm" );

					ScriptEngine::Enable_Spawner( M04_ENGINEROOM_SPAWNER_01_JDG, true );
					ScriptEngine::Enable_Spawner( M04_ENGINEROOM_SPAWNER_02_JDG, true );

					GameObject * engineer = ScriptEngine::Find_Object ( M04_ENGINEROOM_CHIEF_ENGINEER_JDG );
					GameObject * jimmy = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_01_JDG);
					GameObject * johnny = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_02_JDG);
					GameObject * bobby = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_03_JDG);
					GameObject * williams = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_04_JDG);//M01_MODIFY_YOUR_ACTION_02_JDG

					if (engineer != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, engineer,  0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
					}

					if (jimmy != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, jimmy,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
					}

					if (johnny != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, johnny,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
					}

					if (bobby != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, bobby,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
					}

					if (williams != nullptr)
					{
						float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
						ScriptEngine::Send_Custom_Event ( obj, williams,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
					}

					GameObject* guard01 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_01_JDG );
					if (guard01 != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, guard01, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}

					GameObject* guard02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_02_JDG );
					if (guard02 != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, guard02, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}

					GameObject* hunterController = ScriptEngine::Find_Object ( M04_ENGINEROOM_HUNTING_CONTROLLER_JDG );
					if (hunterController != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, hunterController, 0, M01_START_ACTING_JDG, 0 );
					}
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_ChiefEngineer_JDG, "")// M04_ENGINEROOM_CHIEF_ENGINEER_JDG 101762
{
	typedef enum {
		IDLE,
		GOING_TO_WILLIAMS_01,
		GOING_TO_BOBBY_01,
		GOING_TO_JIMMY_01,
		GOING_TO_JOHNNY,
		GOING_TO_BOBBY_02,
		GOING_TO_JIMMY_02,
		GOING_TO_WILLIAMS_02,
		ALERTED,
		FINISHED,

	} M04EngineRoom_Location;

	M04EngineRoom_Location chiefs_location;

	bool williams_is_dead;
	bool bobby_is_dead;
	bool johnny_is_dead;
	bool jimmy_is_dead;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( williams_is_dead, 1 );
		SAVE_VARIABLE( bobby_is_dead, 2 );
		SAVE_VARIABLE( johnny_is_dead, 3 );
		SAVE_VARIABLE( jimmy_is_dead, 4 );
		SAVE_VARIABLE( chiefs_location, 5 );
	}

	void Created( GameObject * obj ) override
	{
		williams_is_dead = false;
		bobby_is_dead = false;
		johnny_is_dead = false;
		jimmy_is_dead = false;

		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, false );
		chiefs_location = IDLE;
		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 1 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)
			{
				GameObject * jimmy = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_01_JDG);
				GameObject * johnny = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_02_JDG);
				GameObject * bobby = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_03_JDG);
				GameObject * williams = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_04_JDG);

				if (sender == jimmy)
				{
					jimmy_is_dead = true;
				}

				else if (sender == johnny)
				{
					johnny_is_dead = true;
				}

				else if (sender == bobby)
				{
					bobby_is_dead = true;
				}

				else if (sender == williams)
				{
					williams_is_dead = true;
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)//someone's missing--look around then goto innate
			{
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation( "H_A_J21C", false );
				ScriptEngine::Action_Play_Animation (  obj, params );
				chiefs_location = ALERTED;
			}

			else if (param == M01_PICK_A_NEW_LOCATION_JDG)
			{
				Vector3 jimmys_location (7.6f, -107.2f, -15);
				Vector3 johnnys_location (-2.5f, -102.8f, -15);
				Vector3 bobbys_location (-0.6f, -74.4f, -18);
				Vector3 williams_location (5.8f, -82, -18);

				if (chiefs_location == IDLE)
				{
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( williams_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_WILLIAMS_01;
				}

				else if (chiefs_location == GOING_TO_WILLIAMS_01)
				{
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( bobbys_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_BOBBY_01;
				}

				else if (chiefs_location == GOING_TO_BOBBY_01)
				{
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( jimmys_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_JIMMY_01;
				}

				else if (chiefs_location == GOING_TO_JIMMY_01)
				{
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( johnnys_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_JOHNNY;
				}

				else if (chiefs_location == GOING_TO_JOHNNY)
				{
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( bobbys_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_BOBBY_02;
				}

				else if (chiefs_location == GOING_TO_BOBBY_02)
				{
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( jimmys_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_JIMMY_02;
				}

				else if (chiefs_location == GOING_TO_JIMMY_02)
				{
					params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( williams_location, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = GOING_TO_WILLIAMS_02;
				}

				else if (chiefs_location == GOING_TO_WILLIAMS_02)
				{
					Vector3 gotoPosition (-0.82f, -91.56f, -15);
					params.Set_Basic( this, 50, M01_WALKING_WAYPATH_02_JDG );
					params.Set_Movement( gotoPosition, WALK, 0.5f );
					ScriptEngine::Action_Goto( obj, params );
					chiefs_location = FINISHED;
				}
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		GameObject * jimmy = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_01_JDG);
		GameObject * johnny = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_02_JDG);
		GameObject * bobby = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_03_JDG);
		GameObject * williams = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_04_JDG);

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				Vector3 gotoPosition (-0.82f, -91.56f, -15);
				params.Set_Basic( this, 80, M01_WALKING_WAYPATH_02_JDG );
				params.Set_Movement( gotoPosition, RUN, 0.5f );
				ScriptEngine::Action_Goto( obj, params );
			}

			else if (action_id == M01_WALKING_WAYPATH_02_JDG)
			{
				Vector3 gotoPosition (-0.82f, -91.56f, -15);
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, gotoPosition, 10 );

				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( "TargetHasBeenEngaged_2", myPosition, obj );

				if (jimmy != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
					ScriptEngine::Send_Custom_Event ( obj, jimmy,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}

				if (johnny != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
					ScriptEngine::Send_Custom_Event ( obj, johnny,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}

				if (bobby != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
					ScriptEngine::Send_Custom_Event ( obj, bobby,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}

				if (williams != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
					ScriptEngine::Send_Custom_Event ( obj, williams,  0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}
			}

			else if (action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				if (chiefs_location == GOING_TO_WILLIAMS_01)
				{
					if (williams != nullptr && williams_is_dead == false)
					{
						//int williams01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Williams_01_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, williams01, false, true, true );
						//ScriptEngine::Join_Conversation( williams, williams01, false, true, true );
						//ScriptEngine::Start_Conversation( williams01,  williams01 );
						//ScriptEngine::Monitor_Conversation(  obj, williams01 );
					}

					else
					{
						chiefs_location = ALERTED;
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else if (chiefs_location == GOING_TO_BOBBY_01)
				{
					if (bobby != nullptr && bobby_is_dead == false)
					{
						//int bobby01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Bobby_01_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, bobby01, false, true, true );
						//ScriptEngine::Join_Conversation( bobby, bobby01, false, true, true );
						//ScriptEngine::Start_Conversation( bobby01,  bobby01 );
						//ScriptEngine::Monitor_Conversation(  obj, bobby01 );
					}

					else
					{
						chiefs_location = ALERTED;//
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else if (chiefs_location == GOING_TO_JIMMY_01)
				{
					if (jimmy != nullptr && jimmy_is_dead == false)
					{
						//int jimmy01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Jimmy_01_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, jimmy01, false, true, true );
						//ScriptEngine::Join_Conversation( jimmy, jimmy01, false, true, true );
						//ScriptEngine::Start_Conversation( jimmy01,  jimmy01 );
						//ScriptEngine::Monitor_Conversation(  obj, jimmy01 );
					}

					else
					{
						chiefs_location = ALERTED;//GOING_TO_JIMMY_01
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else if (chiefs_location == GOING_TO_JOHNNY)
				{
					if (johnny != nullptr && johnny_is_dead == false)
					{
						//int johnny01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Tech04_01_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, johnny01, false, true, true );
						//ScriptEngine::Join_Conversation( johnny, johnny01, false, true, true );
						//ScriptEngine::Start_Conversation( johnny01,  johnny01 );
						//ScriptEngine::Monitor_Conversation(  obj, johnny01 );
					}

					else
					{
						chiefs_location = ALERTED;//GOING_TO_JIMMY_01
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else if (chiefs_location == GOING_TO_BOBBY_02)
				{
					if (bobby != nullptr && bobby_is_dead == false)
					{
						//int bobby01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Bobby_02_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, bobby01, false, true, true );
						//ScriptEngine::Join_Conversation( bobby, bobby01, false, true, true );
						//ScriptEngine::Start_Conversation( bobby01,  bobby01 );
						//ScriptEngine::Monitor_Conversation(  obj, bobby01 );
					}

					else
					{
						chiefs_location = ALERTED;//
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else if (chiefs_location == GOING_TO_JIMMY_02)
				{
					if (jimmy != nullptr && jimmy_is_dead == false)
					{
						//int jimmy01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Jimmy_02_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, jimmy01, false, true, true );
						//ScriptEngine::Join_Conversation( jimmy, jimmy01, false, true, true );
						//ScriptEngine::Start_Conversation( jimmy01,  jimmy01 );
						//ScriptEngine::Monitor_Conversation(  obj, jimmy01 );
					}

					else
					{
						chiefs_location = ALERTED;//GOING_TO_JIMMY_01
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else if (chiefs_location == GOING_TO_WILLIAMS_02)
				{
					if (williams != nullptr && williams_is_dead == false)
					{
						//int williams01 = ScriptEngine::Create_Conversation( "M04_EngineRoom_Williams_02_Conversation", 50, 50, true);
						//ScriptEngine::Join_Conversation( obj, williams01, false, true, true );
						//ScriptEngine::Join_Conversation( williams, williams01, false, true, true );
						//ScriptEngine::Start_Conversation( williams01,  williams01 );
						//ScriptEngine::Monitor_Conversation(  obj, williams01 );
					}

					else
					{
						chiefs_location = ALERTED;
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}
			}
		}

		else if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 1 );

			if (chiefs_location == GOING_TO_WILLIAMS_01)
			{
				if (williams != nullptr && williams_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, williams, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}

			else if (chiefs_location == GOING_TO_BOBBY_01)
			{
				if (bobby != nullptr && bobby_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, bobby, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}

			else if (chiefs_location == GOING_TO_JIMMY_01)
			{
				if (jimmy != nullptr && jimmy_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, jimmy, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}

			else if (chiefs_location == GOING_TO_JOHNNY)
			{
				if (johnny != nullptr && johnny_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, johnny, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}

			else if (chiefs_location == GOING_TO_BOBBY_02)
			{
				if (bobby != nullptr && bobby_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, bobby, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}

			else if (chiefs_location == GOING_TO_JIMMY_02)
			{
				if (jimmy != nullptr && jimmy_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, jimmy, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}

			else if (chiefs_location == GOING_TO_WILLIAMS_02)
			{
				if (williams != nullptr && williams_is_dead == false)
				{
					ScriptEngine::Send_Custom_Event( obj, williams, 0, M01_START_ACTING_JDG, 0.25f );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_PrisonLift_EnterZone_JDG, "")
{
	void Entered( GameObject * /* obj */, GameObject * enterer ) override
	{
		if ( enterer == STAR  )
		{
			Vector3 spawnSpot (-0.512f, -95.434f, 0);
			GameObject * liftEngineer = ScriptEngine::Create_Object ( "Nod_Engineer_0", spawnSpot );
			ScriptEngine::Attach_Script(liftEngineer, "M04_EngineRoom_LiftEngineer_JDG", "");

			GameObject * zone01 = ScriptEngine::Find_Object (101121);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject * zone02 = ScriptEngine::Find_Object (101122);
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}

			GameObject * zone03 = ScriptEngine::Find_Object (101123);
			if (zone03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone03 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_LiftEngineer_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 100, 101 );
		params.Set_Movement( Vector3(-0.19f, -105.53f, -9), RUN, 1 );
		ScriptEngine::Action_Goto( obj, params );
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Start_Guard_Conversation_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			GameObject * prisonGuard01 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_01_JDG );
			GameObject * prisonGuard02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_02_JDG );

			if (prisonGuard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, prisonGuard01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (prisonGuard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, prisonGuard02, 0, M01_START_ACTING_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Prison_Guard_01_JDG, "")//M04_ENGINEROOM_PRISONGUARD_01_JDG 101988
{
	int haze_prisoner_conv;
	int counter;
	bool engineDestroyed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(engineDestroyed, 1);
		SAVE_VARIABLE(haze_prisoner_conv, 2);
		SAVE_VARIABLE(counter, 3);
	}

	void Created( GameObject * obj ) override
	{
		engineDestroyed = false;
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, false );
		counter = 0;

		GameObject * prisoner = ScriptEngine::Find_Object ( 100011 );
		if (prisoner != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
			params.Set_Attack( prisoner, 0, 0, true );
			ScriptEngine::Action_Attack ( obj, params );
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (obj && damager == STAR)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );

			GameObject* guard02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_02_JDG );
			if (guard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, guard02, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject* prisoner01 = ScriptEngine::Find_Object ( M04_PRISON_PRISONER_01_JDG );
		if (prisoner01 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, prisoner01, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject* guard02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_02_JDG );
		if (guard02 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, guard02, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}

	void Custom (GameObject* obj, int /* type */, intptr_t param, GameObject* /* sender */) override
	{
		if (param == M01_START_ACTING_JDG && engineDestroyed == false)//controller is telling you to start acting
		{
			if (obj)
			{
				if (counter >= 3)
				{
					counter = 0;
				}
				GameObject* guard02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_02_JDG );
				if (obj && guard02 != nullptr )
				{
					static constexpr const char* conversations[3] =
					{
						"M04_PrisonHazing_Conversation_01",
						"M04_PrisonHazing_Conversation_02",
						"M04_PrisonHazing_Conversation_03",
						//"M04_PrisonHazing_Conversation_04",
					};

					haze_prisoner_conv = ScriptEngine::Create_Conversation( conversations[counter], 65, 100, true);
					ScriptEngine::Join_Conversation( obj, haze_prisoner_conv, false, false, true );
					ScriptEngine::Join_Conversation( guard02, haze_prisoner_conv, false, false, true );
					ScriptEngine::Start_Conversation( haze_prisoner_conv, haze_prisoner_conv );
					ScriptEngine::Monitor_Conversation( obj, haze_prisoner_conv );
					counter++;
				}
			}
		}

		else if (param == M01_START_ACTING_JDG && engineDestroyed == true)//controller is telling you to start acting--engines destroyed
		{
			ScriptEngine::Action_Reset ( obj, 100 );
			ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, true );
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}

		else if (param == M01_IVE_BEEN_KILLED_JDG)//guard two has been killed/injured M01_MODIFY_YOUR_ACTION_JDG
		{
			if (obj)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );

				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
				params.Set_Attack( STAR, 10, 0, true );
				ScriptEngine::Action_Attack ( obj, params );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)//the engine has been destroyed--enable foostep hearing
		{
			engineDestroyed = true;
			ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, true );
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == haze_prisoner_conv)
					{
						if ( obj)
						{
							ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_START_ACTING_JDG, 1 );//pick a new conversation
						}
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Prison_Guard_02_JDG, "")//M04_ENGINEROOM_PRISONGUARD_02_JDG 101989
{
	bool engineDestroyed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(engineDestroyed, 1);
	}

	void Created( GameObject * obj ) override
	{
		engineDestroyed = false;
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, false );

		GameObject * prisoner = ScriptEngine::Find_Object ( M04_PRISON_PRISONER_01_JDG );
		if (prisoner != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, 999);
			params.Set_Attack( prisoner, 0, 0, true );
			ScriptEngine::Action_Attack ( obj, params );
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (obj && damager == STAR)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			GameObject* guard01 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_01_JDG );
			if (guard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, guard01, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject* prisoner01 = ScriptEngine::Find_Object ( M04_PRISON_PRISONER_01_JDG );
		if (prisoner01 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, prisoner01, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject* guard01 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_01_JDG );
		if (guard01 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, guard01, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}

	void Custom (GameObject* obj, int /* type */, intptr_t param, GameObject* /* sender */) override
	{
		if (param == M01_START_ACTING_JDG && engineDestroyed == false)//controller is telling you to start acting
		{
		}

		else if (param == M01_START_ACTING_JDG && engineDestroyed == true)//controller is telling you to start acting--engines destroyed
		{
			ScriptEngine::Action_Reset ( obj, 100 );
			ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, true );
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}

		else if (param == M01_IVE_BEEN_KILLED_JDG)//guard one has been killed/injured
		{
			if (obj)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );

				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
				params.Set_Attack( STAR, 10, 0, true );
				ScriptEngine::Action_Attack ( obj, params );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)//the engine has been destroyed--enable foostep hearing
		{
			engineDestroyed = true;
			ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, true );
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_TalkToPrisoners_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			bool playerHasLev01Card = ScriptEngine::Has_Key( STAR, 1 );

			if (playerHasLev01Card == false)
			{
				GameObject * prisoner01 = ScriptEngine::Find_Object (M04_PRISON_PRISONER_01_JDG);
				if (prisoner01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, prisoner01, 0, M01_MODIFY_YOUR_ACTION_04_JDG, 0 );//tell prisoner 3 that Havoc is close by
				}

				ScriptEngine::Destroy_Object ( obj );
			}

			else
			{
				ScriptEngine::Destroy_Object ( obj );
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Prisoner_01_JDG, "")//this guys ID number is M04_PRISON_PRISONER_01_JDG 100011
{
	bool prison_guard_01_dead;
	bool prison_guard_02_dead;
	bool firstTimeDamaged;
	bool freedYet;
	bool seenHavoc;
	int its_gdi_conv;
	int wave_counter;
	int medlab_conv;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(prison_guard_01_dead, 1);
		SAVE_VARIABLE(prison_guard_02_dead, 2);
		SAVE_VARIABLE(firstTimeDamaged, 3);
		SAVE_VARIABLE(freedYet, 4);
		SAVE_VARIABLE(seenHavoc, 5);
		SAVE_VARIABLE(its_gdi_conv, 6);
		SAVE_VARIABLE(wave_counter, 7);
		SAVE_VARIABLE(medlab_conv, 8);
	}

	void Created( GameObject * obj ) override
	{
		wave_counter = 0;
		seenHavoc = false;
		firstTimeDamaged = true;
		prison_guard_01_dead = false;
		prison_guard_02_dead = false;
		freedYet = false;
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true);
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (obj)
		{
			if (damager == STAR )
			{
				float myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
				ScriptEngine::Set_Health ( obj, myMaxHealth );

				if (firstTimeDamaged == true)
				{
					ScriptEngine::Create_Sound ( "00-N066E", Vector3 (0,0,0), obj );//you are firing on a friendly unit
					firstTimeDamaged = false;
					ActionParamsStruct params;
					params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
					params.Set_Animation( "H_A_J21C", false );
					ScriptEngine::Action_Play_Animation (  obj, params );
					//Vector3 myPosition = ScriptEngine::Get_Position ( obj );
					//ScriptEngine::Create_Sound ( "M04 PanicGuy 01 Twiddler", myPosition, obj );
				}
			}
		}
	}

	void Custom (GameObject* obj, int /* type */, intptr_t param, GameObject* sender) override
	{
		ActionParamsStruct params;

		if (param == M01_IVE_BEEN_KILLED_JDG)//prison guard one is dead
		{
			GameObject * prisonGuard01 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_01_JDG );
			GameObject * prisonGuard02 = ScriptEngine::Find_Object ( M04_ENGINEROOM_PRISONGUARD_02_JDG );

			if (sender == prisonGuard01)
			{
				prison_guard_01_dead = true;
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_DO_DAMAGE_CHECK_JDG, 1 );
			}

			if (sender == prisonGuard02)
			{
				prison_guard_02_dead = true;
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_DO_DAMAGE_CHECK_JDG, 1 );
			}
		}

		else if (param == M01_DO_DAMAGE_CHECK_JDG)//check if both are dead yet
		{
			if ((prison_guard_01_dead == true) && (prison_guard_02_dead == true) && freedYet == false && seenHavoc == false)
			{
				seenHavoc = true;
				params.Set_Basic( this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
				params.Set_Attack( STAR, 0, 0, true );
				ScriptEngine::Action_Attack ( obj, params );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 1 );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			its_gdi_conv = ScriptEngine::Create_Conversation( "M04_Is_That_a_GDI_Soldier", 100, 100, false);
			ScriptEngine::Join_Conversation( obj, its_gdi_conv, false, true, true );
			ScriptEngine::Join_Conversation( STAR, its_gdi_conv, false, true, true );
			ScriptEngine::Start_Conversation( its_gdi_conv, its_gdi_conv );
			ScriptEngine::Monitor_Conversation(  obj, its_gdi_conv );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_03_JDG && wave_counter <= 4)//
		{
			params.Set_Basic( this, 100, M01_MODIFY_YOUR_ACTION_03_JDG);
			params.Set_Animation( "H_A_J01C", false );//this is animation for hanging head down
			ScriptEngine::Action_Play_Animation (obj, params);
			wave_counter++;
		}

		else if (param == M01_MODIFY_YOUR_ACTION_04_JDG && wave_counter < 100)//playre is in first talk zone--start conversation
		{
			ScriptEngine::Action_Reset (  obj, 100 );
			wave_counter = 100;

			medlab_conv = ScriptEngine::Create_Conversation( "M04_Prisoner_Guard_is_in_Medlab_Conversation", 100, 100, false);
			ScriptEngine::Join_Conversation( obj, medlab_conv, false, true, true );
			ScriptEngine::Join_Conversation( STAR, medlab_conv, false, true, true );
			ScriptEngine::Start_Conversation( medlab_conv, medlab_conv );
			ScriptEngine::Monitor_Conversation(  obj, medlab_conv );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)//you've been freed--cheer you ungrateful bastard
		{
			freedYet = true;
			const char* animationName = M01_Choose_Cheer_Animation ( );
			ScriptEngine::Action_Reset (  obj, 100 );
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation( animationName, false );
			ScriptEngine::Action_Play_Animation (  obj, params );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( "M01_GDI_Thanks_Twiddler", myPosition, obj );
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)//prisoner one is at his hazing spot
		{
			params.Set_Basic( this, 100, 102);
			params.Set_Animation( "H_A_J06C", false );//this is animation for hanging head down
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Destroy_Object ( obj );
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			firstTimeDamaged = true;
		}

		else if (action_id == M01_MODIFY_YOUR_ACTION_03_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
			params.Set_Attack( STAR, 0, 0, true );
			ScriptEngine::Action_Attack ( obj, params );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 1 );
		}

		else if (action_id == its_gdi_conv && complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			params.Set_Basic( this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
			params.Set_Attack( STAR, 0, 0, true );
			ScriptEngine::Action_Attack ( obj, params );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 1 );
		}

		else if (action_id == medlab_conv && complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			GameObject * objectiveController = ScriptEngine::Find_Object ( M04_OBJECTIVE_CONTROLLER_JDG );
			if (objectiveController != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, objectiveController, 0, 401, 1 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Prisoner_02_JDG, "")//this guys ID number is M04_PRISON_PRISONER_02_JDG 101196
{
	bool firstTimeDamaged;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstTimeDamaged, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation( "H_A_7002", true );
		ScriptEngine::Action_Play_Animation (  obj, params );
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (obj && damager == STAR)
		{
			float myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
			ScriptEngine::Set_Health ( obj, myMaxHealth );

			if (firstTimeDamaged == true)
			{
				ScriptEngine::Create_Sound ( "00-N066E", Vector3 (0,0,0), obj );//you are firing on a friendly unit
				firstTimeDamaged = false;
			}
		}
	}
};

/*DECLARE_SCRIPT(M04_EngineRoom_Prisoner_02_JDG, "")//this guys ID number is M04_PRISON_PRISONER_02_JDG 101196
{
	bool firstTimeDamaged;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstTimeDamaged, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true);
		firstTimeDamaged = true;
	}

	void Damaged( GameObject * obj, GameObject * damager, float amount ) override
	{
		if (obj && damager == STAR)
		{
			int myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
			ScriptEngine::Set_Health ( obj, myMaxHealth );

			if (firstTimeDamaged == true)
			{
				ScriptEngine::Create_Sound ( "00-N066E", Vector3 (0,0,0), obj );//you are firing on a friendly unit
				firstTimeDamaged = false;
				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
				params.Set_Animation( "H_A_J21C", false );
				ScriptEngine::Action_Play_Animation (  obj, params );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( "M04 PanicGuy 01 Twiddler", myPosition, obj );
			}
		}
	}

	void Custom (GameObject* obj, int type, intptr_t param, GameObject* sender) override
	{
		ActionParamsStruct params;

		if (param == M01_MODIFY_YOUR_ACTION_JDG)//you've been freed--cheer you ungrateful bastard
		{
			const char* animationName = M01_Choose_Cheer_Animation ( );
			ScriptEngine::Action_Reset (  obj, 100 );
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation( animationName, false );

			ScriptEngine::Action_Play_Animation (  obj, params );

			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( "M01_GDI_Thanks_Twiddler", myPosition, obj );
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Destroy_Object ( obj );
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			firstTimeDamaged = true;
		}
	}
};*/

DECLARE_SCRIPT(M04_EngineRoom_Prisoner_03_JDG, "")//this guys ID number is M04_PRISON_PRISONER_03_JDG 100013
{
	bool firstTimeDamaged;
	bool freedYet;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(freedYet, 1);
		SAVE_VARIABLE(firstTimeDamaged, 2);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true);
		firstTimeDamaged = true;
		freedYet = false;

		ActionParamsStruct params;
		params.Set_Basic( this, 100, 102);
		params.Set_Animation( "H_A_J13B", true );//this is animation for looping pushups
		ScriptEngine::Action_Play_Animation (obj, params);
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		ActionParamsStruct params;
		if (obj && damager == STAR)
		{
			float myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
			ScriptEngine::Set_Health ( obj, myMaxHealth );

			if (firstTimeDamaged == true)
			{
				ScriptEngine::Create_Sound ( "00-N066E", Vector3 (0,0,0), obj );//you are firing on a friendly unit
				firstTimeDamaged = false;
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
				params.Set_Animation( "H_A_J21C", false );
				ScriptEngine::Action_Play_Animation (  obj, params );
				//Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				//ScriptEngine::Create_Sound ( "M04 PanicGuy 01 Twiddler", myPosition, obj );
			}
		}
	}

	void Custom (GameObject* obj, int /* type */, intptr_t param, GameObject* /* sender */) override
	{
		ActionParamsStruct params;

		if (param == 3000)
		{
			ScriptEngine::Action_Reset (  obj, 100 );
			params.Set_Basic( this, 100, M01_START_ATTACKING_01_JDG );
			params.Set_Attack( STAR, 0, 0, true);
			ScriptEngine::Action_Attack(obj, params);

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 2 );

		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//ask havco if he has the key yet
		{
			if (freedYet == false)
			{
				int key_conv = ScriptEngine::Create_Conversation( "M04_DoYouHaveTheKey_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( obj, key_conv, false, false, false);
				ScriptEngine::Join_Conversation( STAR, key_conv, false, false, false);
				ScriptEngine::Start_Conversation( key_conv,  key_conv );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)//you've been freed--cheer you ungrateful bastard
		{
			freedYet = true;
			const char* animationName = M01_Choose_Cheer_Animation ( );
			ScriptEngine::Action_Reset (  obj, 100 );
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation( animationName, false );
			ScriptEngine::Action_Play_Animation (  obj, params );
			//Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			//ScriptEngine::Create_Sound ( "M01_GDI_Thanks_Twiddler", myPosition, obj );
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_START_ATTACKING_01_JDG );
		params.Set_Attack( STAR, 0, 0, true);
		ScriptEngine::Action_Attack(obj, params);

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 1 );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Destroy_Object ( obj );
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			firstTimeDamaged = true;
		}
	}
};

DECLARE_SCRIPT(M04_Prison_CellDoor_Zone_JDG, "")
{
	void Created( GameObject * /* obj */ ) override
	{
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( M04_PRISON_DOOR_JDG, 0, "SHP_DOORP01.SHP_DOORP01" );//182085
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{

			}
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			bool playerHasPrisonKey = ScriptEngine::Has_Key( STAR, 1 );

			if (playerHasPrisonKey == true)
			{
				GameObject *prisoner01 = ScriptEngine::Find_Object (M04_PRISON_PRISONER_01_JDG);
				GameObject *prisoner02 = ScriptEngine::Find_Object (M04_PRISON_PRISONER_02_JDG);
				GameObject *prisoner03 = ScriptEngine::Find_Object (M04_PRISON_PRISONER_03_JDG);

				if (prisoner01 != nullptr)
				{
					ScriptEngine::Destroy_Object ( prisoner01 );
				}

				if (prisoner02 != nullptr)
				{
					ScriptEngine::Destroy_Object ( prisoner02 );
				}

				if (prisoner03 != nullptr)
				{
					ScriptEngine::Destroy_Object ( prisoner03 );
				}

				ScriptEngine::Set_Position ( STAR, Vector3(-7.478f, -80.231f, -0.000) );
				ScriptEngine::Set_Facing ( STAR, -125 );
				ScriptEngine::Control_Enable ( STAR, false );
				ScriptEngine::Set_Is_Rendered( STAR, false );

				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X4A_MIDTRO.txt");

				ScriptEngine::Static_Anim_Phys_Goto_Frame ( M04_PRISON_DOOR_JDG, 6, "SHP_DOORP01.SHP_DOORP01" );
				ScriptEngine::Destroy_Object ( obj );//cleaning up this zone
			}

			else
			{
				GameObject * prisoner03 = ScriptEngine::Find_Object (M04_PRISON_PRISONER_03_JDG);
				if (prisoner03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, prisoner03, 0, 3000, 0 );//tell prisoner 3 that Havoc is close by
				}
			}
		}
	}
};
/**********************************************************************************************************
The following scripts all deal with the aft deck on the way to kill the first mate.
***********************************************************************************************************/
DECLARE_SCRIPT(M04_AftDeck_InnerSanctum_Entry_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Create_Sound ( "00-n060e", ScriptEngine::Get_Position ( obj ), obj );//updating radar flags
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 4000, 0 );//tell aft deck controller that player is moving forward

			GameObject *zone02 = ScriptEngine::Find_Object ( 100648 );
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object (zone02);
			}
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_InnerSanctum_02_Entry_Zone_JDG, "")//player is almost to secondary bridge
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Create_Sound ( "00-n060e", ScriptEngine::Get_Position ( obj ), obj );//updating radar flags
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 5000, 0 );//tell aft deck controller that player is moving forward
			ScriptEngine::Destroy_Object ( obj ); //clean up the zone
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_02_Controller_JDG, "")//this guys number is M04_AFT_DECK_CONTROLLER_JDG 100624
{
	int number_of_guys_killed;
	int prisoner_reinforcements;
	int patrolGuy01_ID;
	int patrolGuy02_ID ;
	int pointGuard01_ID;
	int pointGuard02_ID;
	int BHpatrolGuy_ID;
	int prisonGuard01_ID;
	int prisonGuard02_ID;
	int lockerRoompatrolGuy01_ID;
	int lockerRoompatrolGuy02_ID;
	int leftBarracksTalkGuy01_ID;
	int leftBarracksTalkGuy02_ID;
	int leftBarracksTalkGuy03_ID;
	int rightBarrackspatrolGuy_ID;
	int blackHandPointGuard01_ID;
	int blackHandPointGuard02_ID;
	int miniGunnerGuard01_ID;
	int miniGunnerGuard02_ID;
	int miniGunnerGuard03_ID;
	int blackHandMateGuard01_ID;
	int blackHandMateGuard02_ID;
	int blackHandMateGuard03_ID;
	int blackHandMateGuard04_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( number_of_guys_killed, 1 );
		SAVE_VARIABLE( prisoner_reinforcements, 2 );
		SAVE_VARIABLE( patrolGuy01_ID, 3 );
		SAVE_VARIABLE( patrolGuy02_ID, 4 );
		SAVE_VARIABLE( pointGuard01_ID, 5 );
		SAVE_VARIABLE( pointGuard02_ID, 6 );
		SAVE_VARIABLE( BHpatrolGuy_ID, 7 );
		SAVE_VARIABLE( prisonGuard01_ID, 8 );
		SAVE_VARIABLE( prisonGuard02_ID, 9 );
		SAVE_VARIABLE( lockerRoompatrolGuy01_ID, 10 );
		SAVE_VARIABLE( lockerRoompatrolGuy02_ID, 11 );
		SAVE_VARIABLE( leftBarracksTalkGuy01_ID, 12 );
		SAVE_VARIABLE( leftBarracksTalkGuy02_ID, 13 );
		SAVE_VARIABLE( leftBarracksTalkGuy03_ID, 14 );
		SAVE_VARIABLE( rightBarrackspatrolGuy_ID, 15 );
		SAVE_VARIABLE( blackHandPointGuard01_ID, 16 );
		SAVE_VARIABLE( blackHandPointGuard02_ID, 17 );
		SAVE_VARIABLE( miniGunnerGuard01_ID, 18 );
		SAVE_VARIABLE( miniGunnerGuard02_ID, 19 );
		SAVE_VARIABLE( miniGunnerGuard03_ID, 20 );
		SAVE_VARIABLE( blackHandMateGuard01_ID, 21 );
		SAVE_VARIABLE( blackHandMateGuard02_ID, 22 );
		SAVE_VARIABLE( blackHandMateGuard03_ID, 23 );
		SAVE_VARIABLE( blackHandMateGuard04_ID, 24 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		number_of_guys_killed = 0;
		prisoner_reinforcements = 0;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//recieved custom from enter zone to start scenario
		{
			GameObject *patrolGuy01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(-.90f, -127.01f, 3.00f));
			ScriptEngine::Attach_Script(patrolGuy01, "M04_AftDeck_02_PatrolGuy_01_JDG", "");

			GameObject *patrolGuy02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(6.64f, -116.10f, 3.00f));
			ScriptEngine::Attach_Script(patrolGuy02, "M04_AftDeck_02_PatrolGuy_02_JDG", "");

			GameObject *pointGuard01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(-14.09f, -108.50f, 3.00f));
			ScriptEngine::Attach_Script(pointGuard01, "M04_AftDeck_02_Pointguard_JDG", "");

			GameObject *pointGuard02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(13.83f, -108.00f, 3.00f));
			ScriptEngine::Attach_Script(pointGuard02, "M04_AftDeck_02_Pointguard_JDG", "");

			GameObject *BHpatrolGuy = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(8.51f, -108.05f, 6.00f));
			ScriptEngine::Attach_Script(BHpatrolGuy, "M04_AftDeck_02_Blackhand_PatrolGuy_JDG", "");

			GameObject *prisonGuard01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(9.80f, -72.88f, 0));
			ScriptEngine::Attach_Script(prisonGuard01, "M04_PointGuard_JDG", "");

			GameObject *prisonGuard02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-9.39f, -72.49f, 0));
			ScriptEngine::Attach_Script(prisonGuard02, "M04_PointGuard_JDG", "");

			patrolGuy01_ID = ScriptEngine::Get_ID ( patrolGuy01 );;
			patrolGuy02_ID  = ScriptEngine::Get_ID ( patrolGuy02 );
			pointGuard01_ID = ScriptEngine::Get_ID ( pointGuard01 );
			pointGuard02_ID = ScriptEngine::Get_ID ( pointGuard02 );
			BHpatrolGuy_ID = ScriptEngine::Get_ID ( BHpatrolGuy );
			prisonGuard01_ID = ScriptEngine::Get_ID ( prisonGuard01 );
			prisonGuard02_ID = ScriptEngine::Get_ID ( prisonGuard02 );
		}

		else if (param == 200)//someone's been killed
		{
			number_of_guys_killed++;

			if (number_of_guys_killed == 2)
			{
				//ScriptEngine::Create_Sound ( "ThereHeIs", Vector3(8.51f, -108.05f, 6.00f), obj );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 2000, 2 );//play second dialog line "call reinforcements"
			}

			if ((number_of_guys_killed >= 2) && (number_of_guys_killed <= 5) )
			{
				GameObject *reinforceGuy02 = ScriptEngine::Trigger_Spawner( 100630 );
				ScriptEngine::Attach_Script(reinforceGuy02, "M04_AftDeck_Reinforcement_JDG", "");
				float motto_reinforce_ka = ScriptEngine::Get_Random ( 0.5, 2.5 );
				if ((motto_reinforce_ka >= 0.5) && (motto_reinforce_ka < 1.5) )
				{
					float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
					ScriptEngine::Send_Custom_Event( obj, obj, 0, 1000, delayTimer );//spawn 2 more guys
				}
			}
		}

		else if (param == 1000)//spawn 2 more guys after short delay
		{
			if ((number_of_guys_killed >= 2) && (number_of_guys_killed <= 5) )
			{
				GameObject *reinforceGuy01 = ScriptEngine::Trigger_Spawner( 100629 );
				GameObject *reinforceGuy02 = ScriptEngine::Trigger_Spawner( 100630 );
				ScriptEngine::Attach_Script(reinforceGuy01, "M04_AftDeck_Reinforcement_JDG", "");
				ScriptEngine::Attach_Script(reinforceGuy02, "M04_AftDeck_Reinforcement_JDG", "");
			}
		}

		else if (param == 2000)//play dialog line "call reinforcements"
		{
			//ScriptEngine::Create_Sound ( "CallReinforcements", Vector3(8.51f, -108.05f, 6.00f), obj );
		}

		else if ((param == 3000) && (prisoner_reinforcements <= 5) )//prisoners have been rescued--release the hounds
		{
			prisoner_reinforcements++;
			GameObject *reinforceGuy01 = nullptr;
			GameObject *reinforceGuy02 = nullptr;
			reinforceGuy01 = ScriptEngine::Trigger_Spawner( 100629 );
			reinforceGuy02 = ScriptEngine::Trigger_Spawner( 100630 );
			ScriptEngine::Attach_Script(reinforceGuy01, "M04_AftDeck_Reinforcement_JDG", "");
			ScriptEngine::Attach_Script(reinforceGuy02, "M04_AftDeck_Reinforcement_JDG", "");
		}

		else if (param == 4000) //player is moving up toward first mate
		{
			GameObject *lockerRoompatrolGuy01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(2.53f, -76.81f, 6.00f));
			ScriptEngine::Attach_Script(lockerRoompatrolGuy01, "M04_Aft_LockerRoom_PatrolGuy01_JDG", "");

			GameObject *lockerRoompatrolGuy02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(-1.65f, -76.81f, 6.00f));
			ScriptEngine::Attach_Script(lockerRoompatrolGuy02, "M04_Aft_LockerRoom_PatrolGuy02_JDG", "");

			GameObject *leftBarracksTalkGuy01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(11.01f, -103.69f, 6.00f));
			ScriptEngine::Attach_Script(leftBarracksTalkGuy01, "M04_Aft_LeftBarracks_TalkGuy_JDG", "");
			ScriptEngine::Set_Facing ( leftBarracksTalkGuy01, 160);

			GameObject *leftBarracksTalkGuy02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(9.25f, -100.55f, 6.00f));
			ScriptEngine::Attach_Script(leftBarracksTalkGuy02, "M04_Aft_LeftBarracks_TalkGuy_JDG", "");
			ScriptEngine::Set_Facing ( leftBarracksTalkGuy02, -75);

			GameObject *leftBarracksTalkGuy03 = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(9.33f, -103.77f, 6.00f));
			ScriptEngine::Attach_Script(leftBarracksTalkGuy03, "M04_Aft_LeftBarracks_TalkGuy_JDG", "");
			ScriptEngine::Set_Facing ( leftBarracksTalkGuy03, 60);

			GameObject *rightBarrackspatrolGuy = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-9.33f, -102.35f, 6.00f));
			ScriptEngine::Attach_Script(rightBarrackspatrolGuy, "M04_Aft_RightBarracks_PatrolGuy_JDG", "");

			lockerRoompatrolGuy01_ID = ScriptEngine::Get_ID ( lockerRoompatrolGuy01 );
			lockerRoompatrolGuy02_ID = ScriptEngine::Get_ID ( lockerRoompatrolGuy02 );
			leftBarracksTalkGuy01_ID = ScriptEngine::Get_ID ( leftBarracksTalkGuy01 );
			leftBarracksTalkGuy02_ID = ScriptEngine::Get_ID ( leftBarracksTalkGuy02 );
			leftBarracksTalkGuy03_ID = ScriptEngine::Get_ID ( leftBarracksTalkGuy03 );
			rightBarrackspatrolGuy_ID = ScriptEngine::Get_ID ( rightBarrackspatrolGuy );
		}

		else if (param == 5000) //player is almost at first mate
		{
			GameObject *blackHandPointGuard02 = ScriptEngine::Create_Object ( "Nod_FlameThrower_0", Vector3(9.13f, -86.51f, 9.00f));
			ScriptEngine::Set_Facing ( blackHandPointGuard02, -90);

			GameObject *miniGunnerGuard01 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(-9.25f, -76.39f, 9.00f));
			GameObject *miniGunnerGuard02 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(0.46f, -75.99f, 9.00f));
			GameObject *miniGunnerGuard03 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(11.11f, -76.49f, 9.00f));

			GameObject *blackHandMateGuard01 = ScriptEngine::Create_Object ( "Nod_FlameThrower_0", Vector3(3.24f, -79.21f, 12.00f));
			ScriptEngine::Attach_Script(blackHandMateGuard01, "M04_Aft_FirstMateBodyguard_JDG", "");
			ScriptEngine::Set_Facing ( blackHandMateGuard01, 0);

			GameObject *blackHandMateGuard02 = ScriptEngine::Create_Object ( "Nod_FlameThrower_0", Vector3(3.85f, -85.44f, 12.00f));
			ScriptEngine::Attach_Script(blackHandMateGuard02, "M04_Aft_FirstMateBodyguard_JDG", "");
			ScriptEngine::Set_Facing ( blackHandMateGuard02, 0);

			GameObject *blackHandMateGuard03 = ScriptEngine::Create_Object ( "Nod_FlameThrower_0", Vector3(-4.25f, -85.21f, 12.00f));
			ScriptEngine::Attach_Script(blackHandMateGuard03, "M04_Aft_FirstMateBodyguard_JDG", "");
			ScriptEngine::Set_Facing ( blackHandMateGuard03, 175);

			GameObject *blackHandMateGuard04 = ScriptEngine::Create_Object ( "Nod_FlameThrower_0", Vector3(-4.19f, -79.31f, 12.00f));
			ScriptEngine::Attach_Script(blackHandMateGuard04, "M04_Aft_FirstMateBodyguard_JDG", "");
			ScriptEngine::Set_Facing ( blackHandMateGuard04, 175);

			blackHandPointGuard02_ID = ScriptEngine::Get_ID ( blackHandPointGuard02 );
			miniGunnerGuard01_ID = ScriptEngine::Get_ID ( miniGunnerGuard01 );
			miniGunnerGuard02_ID = ScriptEngine::Get_ID ( miniGunnerGuard02 );
			miniGunnerGuard03_ID = ScriptEngine::Get_ID ( miniGunnerGuard03 );

			blackHandMateGuard01_ID = ScriptEngine::Get_ID ( blackHandMateGuard01 );
			blackHandMateGuard02_ID = ScriptEngine::Get_ID ( blackHandMateGuard02 );
			blackHandMateGuard03_ID = ScriptEngine::Get_ID ( blackHandMateGuard03 );
			blackHandMateGuard04_ID = ScriptEngine::Get_ID ( blackHandMateGuard04 );
		}

		else if (param == 5500) //tell body guards to goto innate
		{
			GameObject *blackHandMateGuard01 = ScriptEngine::Find_Object ( blackHandMateGuard01_ID );
			if (blackHandMateGuard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, blackHandMateGuard01, 0, 100, 0 );
			}

			GameObject *blackHandMateGuard02 = ScriptEngine::Find_Object ( blackHandMateGuard02_ID );
			if (blackHandMateGuard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, blackHandMateGuard02, 0, 100, 0 );
			}

			GameObject *blackHandMateGuard03 = ScriptEngine::Find_Object ( blackHandMateGuard03_ID );
			if (blackHandMateGuard03 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, blackHandMateGuard03, 0, 100, 0 );
			}

			GameObject *blackHandMateGuard04 = ScriptEngine::Find_Object ( blackHandMateGuard04_ID );
			if (blackHandMateGuard04 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, blackHandMateGuard04, 0, 100, 0 );
			}
		}

		else if (param == 6000) //player is leaving the aft deck...clean up any remaing soldiers
		{
			GameObject *patrolGuy01 = ScriptEngine::Find_Object ( patrolGuy01_ID );
			if (patrolGuy01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( patrolGuy01 );
			}

			GameObject *patrolGuy02 = ScriptEngine::Find_Object ( patrolGuy02_ID );
			if (patrolGuy02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( patrolGuy02 );
			}

			GameObject *pointGuard01 = ScriptEngine::Find_Object ( pointGuard01_ID );
			if (pointGuard01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( pointGuard01 );
			}

			GameObject *pointGuard02 = ScriptEngine::Find_Object ( pointGuard02_ID );
			if (pointGuard02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( pointGuard02 );
			}

			GameObject *BHpatrolGuy = ScriptEngine::Find_Object ( BHpatrolGuy_ID );
			if (BHpatrolGuy != nullptr)
			{
				ScriptEngine::Destroy_Object ( BHpatrolGuy );
			}

			GameObject *prisonGuard01 = ScriptEngine::Find_Object ( prisonGuard01_ID );
			if (prisonGuard01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( prisonGuard01 );
			}

			GameObject *prisonGuard02 = ScriptEngine::Find_Object ( prisonGuard02_ID );
			if (prisonGuard02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( prisonGuard02 );
			}

			GameObject *lockerRoompatrolGuy01 = ScriptEngine::Find_Object ( lockerRoompatrolGuy01_ID );
			if (lockerRoompatrolGuy01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( lockerRoompatrolGuy01 );
			}

			GameObject *lockerRoompatrolGuy02 = ScriptEngine::Find_Object ( lockerRoompatrolGuy02_ID );
			if (lockerRoompatrolGuy02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( lockerRoompatrolGuy02 );
			}

			GameObject *leftBarracksTalkGuy01 = ScriptEngine::Find_Object ( leftBarracksTalkGuy01_ID );
			if (leftBarracksTalkGuy01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( leftBarracksTalkGuy01 );
			}

			GameObject *leftBarracksTalkGuy02 = ScriptEngine::Find_Object ( leftBarracksTalkGuy02_ID );
			if (leftBarracksTalkGuy02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( leftBarracksTalkGuy02 );
			}

			GameObject *leftBarracksTalkGuy03 = ScriptEngine::Find_Object ( leftBarracksTalkGuy03_ID );
			if (leftBarracksTalkGuy03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( leftBarracksTalkGuy03 );
			}

			GameObject *rightBarrackspatrolGuy = ScriptEngine::Find_Object ( rightBarrackspatrolGuy_ID );
			if (rightBarrackspatrolGuy != nullptr)
			{
				ScriptEngine::Destroy_Object ( rightBarrackspatrolGuy );
			}

			GameObject *blackHandPointGuard01 = ScriptEngine::Find_Object ( blackHandPointGuard01_ID );
			if (blackHandPointGuard01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( blackHandPointGuard01 );
			}

			GameObject *blackHandPointGuard02 = ScriptEngine::Find_Object ( blackHandPointGuard02_ID );
			if (blackHandPointGuard02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( blackHandPointGuard02 );
			}

			GameObject *miniGunnerGuard01 = ScriptEngine::Find_Object ( miniGunnerGuard01_ID );
			if (miniGunnerGuard01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( miniGunnerGuard01 );
			}

			GameObject *miniGunnerGuard02 = ScriptEngine::Find_Object ( miniGunnerGuard02_ID );
			if (miniGunnerGuard02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( miniGunnerGuard02 );
			}

			GameObject *miniGunnerGuard03 = ScriptEngine::Find_Object ( miniGunnerGuard03_ID );
			if (miniGunnerGuard03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( miniGunnerGuard03 );
			}

			GameObject *blackHandMateGuard01 = ScriptEngine::Find_Object ( blackHandMateGuard01_ID );
			if (blackHandMateGuard01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( blackHandMateGuard01 );
			}

			GameObject *blackHandMateGuard02 = ScriptEngine::Find_Object ( blackHandMateGuard02_ID );
			if (blackHandMateGuard02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( blackHandMateGuard02 );
			}

			GameObject *blackHandMateGuard03 = ScriptEngine::Find_Object ( blackHandMateGuard03_ID );
			if (blackHandMateGuard03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( blackHandMateGuard03 );
			}

			GameObject *blackHandMateGuard04 = ScriptEngine::Find_Object ( blackHandMateGuard04_ID );
			if (blackHandMateGuard04 != nullptr)
			{
				ScriptEngine::Destroy_Object ( blackHandMateGuard04 );
			}

			GameObject *reinforceGuy01 = ScriptEngine::Trigger_Spawner( 100724 );
			GameObject *reinforceGuy02 = ScriptEngine::Trigger_Spawner( 100725 );
			GameObject *reinforceGuy03 = ScriptEngine::Trigger_Spawner( 100579 );
			GameObject *reinforceGuy04 = ScriptEngine::Trigger_Spawner( 100581 );

			ScriptEngine::Attach_Script(reinforceGuy01, "M04_AftDeck_Reinforcement_JDG", "");
			ScriptEngine::Attach_Script(reinforceGuy02, "M04_AftDeck_Reinforcement_JDG", "");
			ScriptEngine::Attach_Script(reinforceGuy03, "M04_AftDeck_Reinforcement_JDG", "");
			ScriptEngine::Attach_Script(reinforceGuy04, "M04_AftDeck_Reinforcement_JDG", "");
		}
	}
};


DECLARE_SCRIPT(M04_Aft_FirstMateBodyguard_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable( obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//here comes the player--goto innate
		{
			if (obj)
			{
				ScriptEngine::Innate_Enable(obj);
				ScriptEngine::Set_Innate_Aggressiveness ( obj, .50f );
				ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 1.0f );
			}
		}
	}
};

DECLARE_SCRIPT(M04_SecondaryBridge_Enter_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_SHIPS_FIRST_MATE_JDG), 0, 100, 0 );//tell first mate to start hearing stuff
			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 1000, 0 );//tell objective controller to play battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 5500, 0 );//tell bodyguards to goto innate

			GameObject * zone01 = ScriptEngine::Find_Object ( 100684 );
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );//clean up the two zones
			}

			GameObject * zone02 = ScriptEngine::Find_Object ( 100686 );
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );//clean up the two zones
			}
		}
	}
};

DECLARE_SCRIPT(M04_Player_Is_Leaving_Aft_Deck_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 6000, 0 );//tell aft deck controller to clean up any remaining soldiers

			GameObject * zone01 = ScriptEngine::Find_Object ( 100726 );
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );//clean up the two zones
			}

			GameObject * zone02 = ScriptEngine::Find_Object ( 100727 );
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );//clean up the two zones
			}
		}
	}
};

DECLARE_SCRIPT(M04_Aft_RightBarracks_PatrolGuy_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_action_id, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 100667;
		params.WaypointStartID = 100668;
		params.WaypointEndID = 100669;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100671;
			params.WaypointStartID = 100672;
			params.WaypointEndID = 100673;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100667;
			params.WaypointStartID = 100668;
			params.WaypointEndID = 100669;
			ScriptEngine::Action_Goto( obj, params );;
		}
	}
};

DECLARE_SCRIPT(M04_Aft_LeftBarracks_TalkGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		Vector3 goto_spot = ScriptEngine::Get_Position( obj );
		params.Set_Basic(this, 45, 100);
		params.Set_Movement( goto_spot, .750f, .25f );
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, 100 );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}
};

DECLARE_SCRIPT(M04_Aft_LockerRoom_PatrolGuy01_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_action_id, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 100649;
		params.WaypointStartID = 100650;
		params.WaypointEndID = 100651;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100653;
			params.WaypointStartID = 100654;
			params.WaypointEndID = 100655;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100657;
			params.WaypointStartID = 100658;
			params.WaypointEndID = 100659;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_Aft_LockerRoom_PatrolGuy02_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_action_id, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 100661;
		params.WaypointStartID = 100662;
		params.WaypointEndID = 100663;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100657;
			params.WaypointStartID = 100658;
			params.WaypointEndID = 100659;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100653;
			params.WaypointStartID = 100654;
			params.WaypointEndID = 100655;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_Reinforcement_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		Vector3 player_location = ScriptEngine::Get_Position ( STAR );
		params.Set_Basic( this, 45, 100 );
		params.Set_Movement( player_location, 1.0f, 7.0f );
		ScriptEngine::Action_Goto( obj, params );

		ScriptEngine::Grant_Key( obj, 1, true );
		ScriptEngine::Grant_Key( obj, 2, true );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell aft deck controller that you've been killed
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			Vector3 player_location = ScriptEngine::Get_Position ( STAR );
			params.Set_Basic( this, 50, 100 );
			params.Set_Movement( player_location, 1.0f, 7.0f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_02_PatrolGuy_01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, 100 );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 100249;
		params.WaypointStartID = 100250;
		params.WaypointEndID = 100251;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell aft deck controller that you've been killed
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 101 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100253;
			params.WaypointStartID = 100254;
			params.WaypointEndID = 100255;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 100 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100249;
			params.WaypointStartID = 100250;
			params.WaypointEndID = 100251;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_02_PatrolGuy_02_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, 100 );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 100238;
		params.WaypointStartID = 100239;
		params.WaypointEndID = 100242;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell aft deck controller that you've been killed
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 101 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100244;
			params.WaypointStartID = 100245;
			params.WaypointEndID = 100247;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 100 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100238;
			params.WaypointStartID = 100239;
			params.WaypointEndID = 100242;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_02_Blackhand_PatrolGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
		ScriptEngine::Send_Custom_Event( obj, obj, 0, 100, delayTimer );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell aft deck controller that you've been killed
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		Vector3 look_to_spot(-0.69f, -128.52f, 4.05f);
		Vector3 present_location = ScriptEngine::Get_Position ( obj );;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 500 );
			params.Set_Movement( present_location, .20f, .25f );
			params.Set_Attack (look_to_spot, 0,0, true);
			ScriptEngine::Action_Attack( obj, params);

			float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, 101, delayTimer );
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 500 );
			params.Set_Movement( present_location, .20f, .25f );
			params.Set_Attack (look_to_spot, 0,0, true);
			ScriptEngine::Action_Attack( obj, params);
			float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, 102, delayTimer );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, 45, 500 );
			params.Set_Movement( present_location, .20f, .25f );
			params.Set_Attack (look_to_spot, 0,0, true);
			ScriptEngine::Action_Attack( obj, params);
			float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, 103, delayTimer );
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, 45, 500 );
			params.Set_Movement( present_location, .20f, .25f );
			params.Set_Attack (look_to_spot, 0,0, true);
			ScriptEngine::Action_Attack( obj, params);
			float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, 100, delayTimer );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//recieved custom to goto next waypath
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 100 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100607;
			params.WaypointStartID = 100608;
			params.WaypointEndID = 100609;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (param == 101)//recieved custom to goto next waypath
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 101 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100611;
			params.WaypointStartID = 100612;
			params.WaypointEndID = 100613;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (param == 102)//recieved custom to goto next waypath
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 102 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100615;
			params.WaypointStartID = 100616;
			params.WaypointEndID = 100617;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (param == 103)//recieved custom to goto next waypath
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 45, 103 );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100619;
			params.WaypointStartID = 100620;
			params.WaypointEndID = 100621;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_AftDeck_02_Pointguard_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (obj)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell aft deck controller that you've been killed
	}
};

DECLARE_SCRIPT(M04_AftDeck_02_Entry_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Create_Sound ( "00-n060e", ScriptEngine::Get_Position ( obj ), obj );//updating radar flags
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 100, 0 );//tell aft deck controller to start scenario

			GameObject * sniperTarget01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-9.160f, -89.962f, 9.000f));
			ScriptEngine::Attach_Script(sniperTarget01, "M04_Doorway_Enterer_JDG", "-0.091 -107.613 6");

			GameObject * sniperTarget02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(9.194f, -77.535f, 3));
			ScriptEngine::Attach_Script(sniperTarget02, "M04_Doorway_Enterer_JDG", "14.657 -94.107 3");

			GameObject * surpriseApache = ScriptEngine::Create_Object ( "Nod_Apache", Vector3(-50.075f, 8.732f, 5.534f));
			ScriptEngine::Attach_Script(surpriseApache, "M04_Surprise_Apache_JDG", "");

			ScriptEngine::Destroy_Object ( obj ); //clean up the zone
		}
	}
};

DECLARE_SCRIPT(M04_Surprise_Apache_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Disable_Physical_Collisions ( obj );
		ActionParamsStruct params;
		params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
		params.Set_Movement( Vector3(0,0,0), .15f, 10 );
		params.WaypathID = 104437;
		params.MovePathfind = false;
		//params.WaypointStartID = 104439;
		//params.WaypointEndID = 101276;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

/*************************************************************************************************************
The following are all the temp scripts for the apache hanger
*************************************************************************************************************/
DECLARE_SCRIPT(M04_Apache_Enter_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_APACHE_CONTROLLER_JDG), 0, 100, 0 );//tell apache controller to place actors

			GameObject * sniperTarget01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-10.057f, 81.803f, -15));
			ScriptEngine::Attach_Script(sniperTarget01, "M04_Doorway_Enterer_JDG", "-12.689 69.327 -15");

			GameObject * huntercontroller = ScriptEngine::Find_Object (M04_ENGINEROOM_HUNTING_CONTROLLER_JDG);
			if (huntercontroller != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, huntercontroller, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			GameObject * zone02 = ScriptEngine::Find_Object (200006);
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );//clean up these zones
			}

			GameObject * zone03 = ScriptEngine::Find_Object (200008);
			if (zone03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone03 );//clean up these zones
			}
		}
	}
};


DECLARE_SCRIPT(M04_ApacheRoom_Controller_JDG, "")//this guys ID number is M04_APACHE_CONTROLLER_JDG	200586
{
	int m04apache_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(m04apache_ID, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		m04apache_ID = 0;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//recieved custom from enter zones--here comes the player(s)
		{
			GameObject *m04apache = ScriptEngine::Create_Object ( "NOD_Apache", Vector3(1.469f, 50.814f, -14.00f));
			ScriptEngine::Attach_Script(m04apache, "M04_ApacheRoom_Apache_JDG", "");
			ScriptEngine::Set_Facing ( m04apache, -125 );

			GameObject *m04apacheSniper01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_2SF", Vector3(-13.728f, 63.244f, -9.003f));
			ScriptEngine::Attach_Script(m04apacheSniper01, "M04_ApacheRoom_Sniper01_JDG", "");

			GameObject *m04apacheSniper02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_2SF", Vector3(13.45f, 33.93f, -9.003f));
			ScriptEngine::Attach_Script(m04apacheSniper02, "M04_ApacheRoom_Sniper02_JDG", "");

			m04apache_ID = ScriptEngine::Get_ID ( m04apache );

			GameObject * tailgun01 = ScriptEngine::Find_Object (M04_ROCKET_EMPLACEMENT_01_JDG);
			GameObject * tailgun02 = ScriptEngine::Find_Object (M04_ROCKET_EMPLACEMENT_02_JDG);

			if (tailgun01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, tailgun01, M01_MODIFY_YOUR_ACTION_JDG, m04apache_ID, 0 );
			}

			if (tailgun02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, tailgun02, M01_MODIFY_YOUR_ACTION_JDG, m04apache_ID, 0 );
			}
		}

		else if (param == 200)//recieved custom from trigger zone--tell apache to take off and attack player
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (m04apache_ID), 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M04_ApacheRoom_Sniper01_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(curr_action_id, 1);
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 200592;
		params.WaypointStartID = 200593;
		params.WaypointEndID = 200594;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 200596;
			params.WaypointStartID = 200597;
			params.WaypointEndID = 200598;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 200592;
			params.WaypointStartID = 200593;
			params.WaypointEndID = 200594;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_ApacheRoom_Sniper02_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(curr_action_id, 1);
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 200601;
		params.WaypointStartID = 200602;
		params.WaypointEndID = 200603;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 200605;
			params.WaypointStartID = 200606;
			params.WaypointEndID = 200607;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 200601;
			params.WaypointStartID = 200602;
			params.WaypointEndID = 200603;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};


/***********************************************************************************************************
The following are the initial scripts for the fore deck
***********************************************************************************************************/
DECLARE_SCRIPT(M04_ForeDeck_RocketGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (obj && damager == STAR)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}
	}
};

DECLARE_SCRIPT(M04_ForeDeck_Initial_Enter_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 100, 0 );//tell ForeDeck controller that player(s) is approaching

			GameObject * zone01 = ScriptEngine::Find_Object (100784);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );//clean up these zones
			}

			GameObject * zone06 = ScriptEngine::Find_Object (100789);
			if (zone06 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone06 );//clean up these zones
			}
		}
	}
};

DECLARE_SCRIPT(M04_MedLab_Enter_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_PRISON_WARDEN_JDG), 0, 100, 0 );//tell prison warden to start acting
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_MEDLAB_TECHNICIAN_JDG), 0, 100, 0 );//tell medlab tech to start acting

			GameObject * zone01 = ScriptEngine::Find_Object (101054);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );//clean up these zones
			}

			GameObject * zone02 = ScriptEngine::Find_Object (101055);
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );//clean up these zones
			}
		}
	}
};

DECLARE_SCRIPT(M04_ForeDeck_Reinforcement_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		Vector3 players_position = ScriptEngine::Get_Position ( STAR );
		params.Set_Basic( this, 50, 100 );
		params.Set_Movement( players_position, 1.0f, .25f );
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			Vector3 players_position = ScriptEngine::Get_Position ( STAR );
			params.Set_Basic( this, 50, 100 );
			params.Set_Movement( players_position, 1.0f, .25f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell controller you've been killed
	}
};

DECLARE_SCRIPT(M04_ForeDeck_MapRoom_Guard01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 5 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 200, 0 );//tell controller you've been killed
	}
};

DECLARE_SCRIPT(M04_ForeDeck_MapRoom_Guard02_JDG, "")
{
	int curr_action_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(curr_action_id, 1);
	}

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		curr_action_id = 100;
		params.Set_Basic( this, 45, curr_action_id );
		params.Set_Movement( Vector3(0,0,0), .20f, .25f );
		params.WaypathID = 100842;
		params.WaypointStartID = 100843;
		params.WaypointEndID = 100845;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 101 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100842;
			params.WaypointStartID = 100845;
			params.WaypointEndID = 100843;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == 102 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			curr_action_id++;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == 103 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			curr_action_id = 100;
			params.Set_Basic( this, 45, curr_action_id );
			params.Set_Movement( Vector3(0,0,0), .20f, .25f );
			params.WaypathID = 100842;
			params.WaypointStartID = 100843;
			params.WaypointEndID = 100845;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_ForeDeck_TorpedoRoom_Guard_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 15 );
	}
};
/*************************************************************************************************************
The following scripts are for the foredeck.  They include both the messhalls and the captain's encounter.
*************************************************************************************************************/
DECLARE_SCRIPT(M04_ForeDeck_Controller_JDG, "")//this guys ID number is M04_FORE_DECK_CONTROLLER_JDG 100790
{
	int active_soldier_count;
	int total_mess_reinforcements;
	int total_BHmess_reinforcements;
	int torpedoRoomGuard_ID;
	int	mapRoomGuard01_ID;
	int	mapRoomGuard02_ID;
	int closetGuy01_ID;
	int BH_messhall_guy01_ID;
	int BH_messhall_guy02_ID;
	int BH_messhall_guy03_ID;
	int BH_messhall_guy04_ID;
	int BH_messhall_guy05_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(active_soldier_count, 1);
		SAVE_VARIABLE(total_mess_reinforcements, 2);
		SAVE_VARIABLE(total_BHmess_reinforcements, 3);
		SAVE_VARIABLE(torpedoRoomGuard_ID, 4);
		SAVE_VARIABLE(mapRoomGuard01_ID, 5);
		SAVE_VARIABLE(mapRoomGuard02_ID, 6);
		SAVE_VARIABLE(closetGuy01_ID, 7);
		SAVE_VARIABLE(BH_messhall_guy01_ID, 8);
		SAVE_VARIABLE(BH_messhall_guy02_ID, 9);
		SAVE_VARIABLE(BH_messhall_guy03_ID, 10);
		SAVE_VARIABLE(BH_messhall_guy04_ID, 11);
		SAVE_VARIABLE(BH_messhall_guy05_ID, 12);
	}

	void Created( GameObject * /* obj */ ) override
	{
		active_soldier_count = 0;
		total_mess_reinforcements = 0;
		total_BHmess_reinforcements = 0;
	}
	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//recieved custom from enter zones--here comes the player(s)
		{
			ScriptEngine::Create_Sound ( "00-n060e", ScriptEngine::Get_Position ( obj ), obj );

			GameObject *torpedoRoomGuard = ScriptEngine::Create_Object ( "Nod_FlameThrower_0", Vector3(-.44f, 90.39f, -15.01f));
			ScriptEngine::Attach_Script(torpedoRoomGuard, "M04_ForeDeck_TorpedoRoom_Guard_JDG", "");

			GameObject *mapRoomGuard01 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(5.99f, 91.56f, -6.0f));
			ScriptEngine::Attach_Script(mapRoomGuard01, "M04_ForeDeck_MapRoom_Guard01_JDG", "");

			GameObject *mapRoomGuard02 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(-6.46f, 92.67f, -6.0f));
			ScriptEngine::Attach_Script(mapRoomGuard02, "M04_ForeDeck_MapRoom_Guard02_JDG", "");

			active_soldier_count = 3;

			torpedoRoomGuard_ID = ScriptEngine::Get_ID ( torpedoRoomGuard );
			mapRoomGuard01_ID = ScriptEngine::Get_ID ( mapRoomGuard01 );
			mapRoomGuard02_ID = ScriptEngine::Get_ID ( mapRoomGuard02 );
		}

		else if (param == 200)//someone's been killed...spawn reinforcements if needed
		{
			active_soldier_count--;
			total_BHmess_reinforcements++;

			if (( active_soldier_count <= 6) && (total_BHmess_reinforcements < 4))
			{
				GameObject *foreDeckReinforcement01 = ScriptEngine::Trigger_Spawner(100853);
				GameObject *foreDeckReinforcement02 = ScriptEngine::Trigger_Spawner(100854);
				active_soldier_count++;
				active_soldier_count++;
				ScriptEngine::Attach_Script(foreDeckReinforcement01, "M04_ForeDeck_Reinforcement_JDG", "");
				ScriptEngine::Attach_Script(foreDeckReinforcement02, "M04_ForeDeck_Reinforcement_JDG", "");
			}
		}

		else if (param == 300)//player is going up to grunt's mess hall
		{
			active_soldier_count = active_soldier_count + 4;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, 301, 2 );
		}

		else if (param == 301)//play "proceed with caution" warning
		{
			ScriptEngine::Create_Sound ( "00-c164e", ScriptEngine::Get_Position ( obj ), obj );//proceed with caution
		}

		else if (param == 500)//player is moving toward captain from grunt mess hall
		{
			//find any guys from torpedo or map room and clean them up if needed
			GameObject * torpedoGuy_01 = ScriptEngine::Find_Object (torpedoRoomGuard_ID);
			if (torpedoGuy_01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( torpedoGuy_01 );
				active_soldier_count--;
			}

			GameObject * mapGuy_01 = ScriptEngine::Find_Object (mapRoomGuard01_ID);
			if (mapGuy_01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( mapGuy_01 );
				active_soldier_count--;
			}

			GameObject * mapGuy_02 = ScriptEngine::Find_Object (mapRoomGuard02_ID);
			if (mapGuy_02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( mapGuy_02 );
				active_soldier_count--;
			}

			//put down the closet surprise guy
			GameObject *closetGuy01 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(-3.361f, 79.633f, 3.0f));
			ScriptEngine::Attach_Script(closetGuy01, "M04_ForeDeck_ClosetSurprise_Guy_JDG", "");

			closetGuy01_ID = ScriptEngine::Get_ID ( closetGuy01 );

			//put down the 5 BH mess hall guys
			GameObject * BH_messhall_guy01 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(5.89f, 88.47f, 6.0f));
			ScriptEngine::Attach_Script(BH_messhall_guy01, "M04_BH_MessHall_Guy_JDG", "");
			ScriptEngine::Set_Facing ( BH_messhall_guy01, -135 );

			GameObject * BH_messhall_guy02 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(5.30f, 87.52f, 6.0f));
			ScriptEngine::Attach_Script(BH_messhall_guy02, "M04_BH_MessHall_Guy_JDG", "");
			ScriptEngine::Set_Facing ( BH_messhall_guy02, -155 );

			GameObject * BH_messhall_guy03 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(2.64f, 88.12f, 6.0f));
			ScriptEngine::Attach_Script(BH_messhall_guy03, "M04_BH_MessHall_Guy_JDG", "");
			ScriptEngine::Set_Facing ( BH_messhall_guy03, -135 );

			GameObject * BH_messhall_guy04 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(2.08f, 87.44f, 6.0f));
			ScriptEngine::Attach_Script(BH_messhall_guy04, "M04_BH_MessHall_Guy_JDG", "");
			ScriptEngine::Set_Facing ( BH_messhall_guy04, -140 );

			GameObject * BH_messhall_guy05 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(-0.38f, 87.29f, 6.0f));
			ScriptEngine::Attach_Script(BH_messhall_guy05, "M04_BH_MessHall_Guy_JDG", "");
			ScriptEngine::Set_Facing ( BH_messhall_guy05, -110 );

			BH_messhall_guy01_ID = ScriptEngine::Get_ID ( BH_messhall_guy01 );
			BH_messhall_guy02_ID = ScriptEngine::Get_ID ( BH_messhall_guy02 );
			BH_messhall_guy03_ID = ScriptEngine::Get_ID ( BH_messhall_guy03 );
			BH_messhall_guy04_ID = ScriptEngine::Get_ID ( BH_messhall_guy04 );
			BH_messhall_guy05_ID = ScriptEngine::Get_ID ( BH_messhall_guy05 );
		}

		else if (param == 505)//time for homey to come out of the closet
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (closetGuy01_ID), 0, 100, 0 );
		}

		else if (param == 650)//received param from BH messhall enter zone--tell BH guys its okay to react now
		{
			GameObject * BH_messhall_guy01 = ScriptEngine::Find_Object ( BH_messhall_guy01_ID);
			GameObject * BH_messhall_guy02 = ScriptEngine::Find_Object ( BH_messhall_guy02_ID);
			GameObject * BH_messhall_guy03 = ScriptEngine::Find_Object ( BH_messhall_guy03_ID);
			GameObject * BH_messhall_guy04 = ScriptEngine::Find_Object ( BH_messhall_guy04_ID);
			GameObject * BH_messhall_guy05 = ScriptEngine::Find_Object ( BH_messhall_guy05_ID);

			if (BH_messhall_guy01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, BH_messhall_guy01, 0, 600, 0 );
			}

			if (BH_messhall_guy02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, BH_messhall_guy02, 0, 600, 0 );
			}

			if (BH_messhall_guy03 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, BH_messhall_guy03, 0, 600, 0 );
			}

			if (BH_messhall_guy04 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, BH_messhall_guy04, 0, 600, 0 );
			}

			if (BH_messhall_guy05 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, BH_messhall_guy05, 0, 600, 0 );
			}
		}

		else if (param == 700)//player is approaching main bridge--put down captain's bodyguards
		{
			GameObject *captains_bodyguard_01 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(-17.79f, 85.49f, 13.13f));
			ScriptEngine::Attach_Script(captains_bodyguard_01, "M04_Captains_Bodyguard_JDG", "");
			GameObject *captains_bodyguard_02 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(-15.26f, 79.5f, 13.13f));
			ScriptEngine::Attach_Script(captains_bodyguard_02, "M04_Captains_Bodyguard_JDG", "");
			GameObject *captains_bodyguard_03 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(17.79f, 85.49f, 13.13f));
			ScriptEngine::Attach_Script(captains_bodyguard_03, "M04_Captains_Bodyguard_JDG", "");
			GameObject *captains_bodyguard_04 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(15.26f, 79.5f, 13.13f));
			ScriptEngine::Attach_Script(captains_bodyguard_04, "M04_Captains_Bodyguard_JDG", "");
			GameObject *captains_bodyguard_05 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(-8.67f, 81.66f, 13.13f));
			ScriptEngine::Attach_Script(captains_bodyguard_05, "M04_Captains_Bodyguard_JDG", "");
			GameObject *captains_bodyguard_06 = ScriptEngine::Create_Object ( "Nod_FlameThrower_1Off", Vector3(8.67f, 81.66f, 13.13f));
			ScriptEngine::Attach_Script(captains_bodyguard_06, "M04_Captains_Bodyguard_JDG", "");
		}
	}
};

DECLARE_SCRIPT(M04_Captains_Bodyguard_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 2 );
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		float reinforce_ka = ScriptEngine::Get_Random ( 0.5f, 3.5f );

		if ((reinforce_ka >= 0.5) && (reinforce_ka < 1.5))
		{
			GameObject * captains_bodyguard_reinforceguy = ScriptEngine::Trigger_Spawner( 100918 );
			ScriptEngine::Attach_Script(captains_bodyguard_reinforceguy, "M04_Captains_Bodyguard_Reinforcement_JDG", "");
		}

		else if ((reinforce_ka >= 1.5) && (reinforce_ka < 2.0))
		{
			GameObject * captains_bodyguard_reinforceguy = ScriptEngine::Trigger_Spawner( 100918 );
			GameObject * captains_bodyguard_reinforceguy02 = ScriptEngine::Trigger_Spawner( 100919 );
			ScriptEngine::Attach_Script(captains_bodyguard_reinforceguy, "M04_Captains_Bodyguard_Reinforcement_JDG", "");
			ScriptEngine::Attach_Script(captains_bodyguard_reinforceguy02, "M04_Captains_Bodyguard_Reinforcement02_JDG", "");
		}
	}
};

DECLARE_SCRIPT(M04_Captains_Bodyguard_Reinforcement_JDG, "")
{
	bool engaged;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(engaged, 1);
	}

	void Created( GameObject * obj ) override
	{
		engaged = false;
		ActionParamsStruct params;
		params.Set_Basic( this, 45, 100 );
		params.Set_Movement( Vector3(0,0,0), 1.0f, .25f );
		params.WaypathID = 100927;
		params.WaypointStartID = 100928;
		params.WaypointEndID = 100932;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (enemy == STAR)
		{
			if (engaged == false)
			{
				engaged = true;
				ScriptEngine::Action_Reset ( obj, 100 );
			}
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			Vector3 players_position = ScriptEngine::Get_Position ( STAR );
			ScriptEngine::Action_Reset (  obj, 0 );
			params.Set_Basic( this, 45, 100 );
			params.Set_Movement( players_position, 1.0f, .25f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_Captains_Bodyguard_Reinforcement02_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, 100 );
		params.Set_Movement( Vector3(0,0,0), 1.0f, .25f );
		params.WaypathID = 100920;
		params.WaypointStartID = 100921;
		params.WaypointEndID = 100925;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			Vector3 players_position = ScriptEngine::Get_Position ( STAR );
			ScriptEngine::Action_Reset (  obj, 0 );
			params.Set_Basic( this, 45, 100 );
			params.Set_Movement( players_position, 1.0f, .25f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_Captains_Bridge_Enter_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 700, 0 );//tell foredeck controller that player is approaching main bridge
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_SHIPS_CAPTAIN_JDG), 0, 100, 0 );//this tells captain to turn on hearing
			ScriptEngine::Create_Sound ( "00-n060e", ScriptEngine::Get_Position ( obj ), obj );//updating radar flags

			GameObject *zone01 = ScriptEngine::Find_Object (100876);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

		}
	}
};

DECLARE_SCRIPT(M04_BH_MessHall_Trigger_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 650, 0 );//tell foredeck controller that player is entering BH messhall
			ScriptEngine::Create_Sound ( "00-c164e", ScriptEngine::Get_Position ( obj ), obj );//proceed with caution
			//clean up this zone
			GameObject *zone01 = ScriptEngine::Find_Object (100873);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_BH_MessHall_Guy_JDG, "")
{
	bool ok_to_react;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(ok_to_react, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ok_to_react = false;
		ActionParamsStruct params;//M04 Nod Alert Twiddler
		Vector3 look_position = ScriptEngine::Get_Position ( ScriptEngine::Find_Object ( 100455 ) );
		params.Set_Basic(this, 89, 100);
		params.Set_Attack(look_position, 0, 0, false);
		ScriptEngine::Action_Attack(obj, params);
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager == STAR)
		{
			if (obj)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			}
		}
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		float reinforce_ka = ScriptEngine::Get_Random ( 0.5f, 2.5f );

		if ((reinforce_ka >= 0.5) && (reinforce_ka < 1.5))
		{
			GameObject * BH_messhall_reinforceguy = ScriptEngine::Trigger_Spawner( 100875 );
			ScriptEngine::Attach_Script(BH_messhall_reinforceguy, "M04_ForeDeck_Reinforcement_03_JDG", "");
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & /* sound */ )  override
	{
		if (ok_to_react == true)
		{
			if (obj)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			}
		}
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 600)//recieved custom from controller--you can react to gunshots now
		{
			ok_to_react = true;
		}
	}
};

DECLARE_SCRIPT(M04_ForeDeck_ClosetSurprise_Guy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable( obj );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//recieved custom from enter zones--here comes the player(s)
		{
			if (obj)
			{
				ScriptEngine::Innate_Enable(obj);
				ActionParamsStruct params;
				params.Set_Basic( this, 100, 100 );
				params.Set_Movement( Vector3(0,0,0), .750f, .25f );
				params.WaypathID = 100864;
				params.WaypointStartID = 100865;
				params.WaypointEndID = 100867;
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == 100 && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			Vector3 players_position = ScriptEngine::Get_Position ( STAR );
			params.Set_Basic( this, 45, 100 );
			params.Set_Movement( players_position, 1.0f, .25f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_ForeDeck_ClosetSurprise_Trigger_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 505, 0 );//tell foredeck controller that player is leaving mess hall

			GameObject * sniperTarget01 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", Vector3(8.937f, 83.264f, 6.000f));
			ScriptEngine::Attach_Script(sniperTarget01, "M04_Doorway_Enterer_JDG", "8.149 76.787 3.000");

			GameObject *zone01 = ScriptEngine::Find_Object (100869);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject *zone02 = ScriptEngine::Find_Object (100870);
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_GruntMessHall_Exit_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 500, 0 );//tell foredeck controller that player is leaving mess hall

			GameObject *zone01 = ScriptEngine::Find_Object (100857);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_GruntMessHall_Entry_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_FORE_DECK_CONTROLLER_JDG), 0, 300, 0 );//tell foredeck controller that player is entering mess hall

			GameObject *zone01 = ScriptEngine::Find_Object (100459);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject *zone02 = ScriptEngine::Find_Object (100460);
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};
/**********************************************************************************************************
The following are the scripts and controller for the end fire fight.
**********************************************************************************************************/


DECLARE_SCRIPT(M04_Firefight_Start_Battle_Music_JDG, "")
{
	bool playerHasLevel3Keycard;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( playerHasLevel3Keycard, 1 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		playerHasLevel3Keycard = false;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 100)//level 3 keycard acquired
		{
			playerHasLevel3Keycard = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR && playerHasLevel3Keycard == true)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_END_FIREFIGHT_CONTROLLER_JDG), 0, 100, 0 );
		}
	}
};

DECLARE_SCRIPT(M04_Firefight_NodGuys, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_END_FIREFIGHT_CONTROLLER_JDG), 0, 666, 0 );//tell controller you've been killed
	}
};




DECLARE_SCRIPT(M04_PostFirstMate_SamRoom_PatrolGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), WALK, .25f );
		params.WaypathID = 101208;
		params.WaypointStartID = 101209;
		params.WaypointEndID = 101210;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101212;
			params.WaypointStartID = 101213;
			params.WaypointEndID = 101214;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_03_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101216;
			params.WaypointStartID = 101217;
			params.WaypointEndID = 101218;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_03_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_03_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_03_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_04_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101220;
			params.WaypointStartID = 101221;
			params.WaypointEndID = 101222;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_04_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_04_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_04_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101208;
			params.WaypointStartID = 101209;
			params.WaypointEndID = 101210;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_PostFirstMate_MissileRoom_PatrolGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), WALK, .25f );
		params.WaypathID = 100550;
		params.WaypointStartID = 100551;
		params.WaypointEndID = 100553;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 60, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 100555;
			params.WaypointStartID = 100556;
			params.WaypointEndID = 100557;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_03_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 100541;
			params.WaypointStartID = 100542;
			params.WaypointEndID = 100544;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_03_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_03_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_03_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_04_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 100546;
			params.WaypointStartID = 100547;
			params.WaypointEndID = 100548;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_04_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_04_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_04_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 100550;
			params.WaypointStartID = 100551;
			params.WaypointEndID = 100553;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_PostFirstMate_OfficerQuarters_PatrolGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), WALK, .25f );
		params.WaypathID = 101237;
		params.WaypointStartID = 101238;
		params.WaypointEndID = 101241;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101243;
			params.WaypointStartID = 101244;
			params.WaypointEndID = 101247;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101237;
			params.WaypointStartID = 101238;
			params.WaypointEndID = 101241;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_PostFirstMate_FrontDeck_PatrolGuy01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), WALK, .25f );
		params.WaypathID = 101250;
		params.WaypointStartID = 101251;
		params.WaypointEndID = 101255;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101250;
			params.WaypointStartID = 101255;
			params.WaypointEndID = 101251;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101250;
			params.WaypointStartID = 101251;
			params.WaypointEndID = 101255;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_PostFirstMate_FrontDeck_PatrolGuy02_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), WALK, .25f );
		params.WaypathID = 101258;
		params.WaypointStartID = 101259;
		params.WaypointEndID = 101263;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{

			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101258;
			params.WaypointStartID = 101263;
			params.WaypointEndID = 101259;
			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			const char* animationName = M01_Choose_Idle_Animation ( );
			params.Set_Basic( this, 45, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation (animationName, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( Vector3(0,0,0), WALK, .25f );
			params.WaypathID = 101258;
			params.WaypointStartID = 101259;
			params.WaypointEndID = 101263;
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M04_EnterCargoBay_BottomRight_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR  )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 100, 0 );

			GameObject * zone01 = ScriptEngine::Find_Object (100497);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject * zone03 = ScriptEngine::Find_Object (100499);
			if (zone03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone03 );
			}
		}
	}
};



DECLARE_SCRIPT(M04_EnterCargoBay_TopLeft_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR  )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_CARGOHOLD_CONTROLLER_JDG), 0, 100, 0 );

			GameObject * zone01 = ScriptEngine::Find_Object (100497);
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject * zone03 = ScriptEngine::Find_Object (100499);
			if (zone03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone03 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_RocketEmplacement_01_JDG, "")//103461
{
	int apache_id;
	bool secondary_apache_spawned;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(apache_id, 1);
		SAVE_VARIABLE(secondary_apache_spawned, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		secondary_apache_spawned = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == M01_MODIFY_YOUR_ACTION_JDG)
		{
			apache_id = param;
		}

		else if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			ScriptEngine::Debug_Message ( "***************************player has entered tailgun 01--send custom to apache\n" );
			GameObject * apache = ScriptEngine::Find_Object ( apache_id );
			if (apache != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, apache,  0, M01_MODIFY_YOUR_ACTION_05_JDG, 0 );
			}

			else if (secondary_apache_spawned == false)
			{
				secondary_apache_spawned = true;
				Vector3 spawnSpot (-5.496f, -52.198f, 22.300f);
				GameObject * secondaryapache = ScriptEngine::Create_Object ( "NOD_Apache", spawnSpot );
				ScriptEngine::Attach_Script(secondaryapache, "M04_SecondaryApache01_JDG", "");
			}
		}
	}
};

DECLARE_SCRIPT(M04_SecondaryApache01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
		params.Set_Movement(Vector3 (0,0,0), 1.0f, 5);
		params.WaypathID = 103531;

		ScriptEngine::Action_Goto(obj, params);
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		Vector3 spawnSpot (-3.732f, -59.579f, 2.704f);
		ScriptEngine::Create_Object ( "POW_Armor_100", spawnSpot );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_SecondaryApache02_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
		params.Set_Movement(Vector3 (0,0,0), 1.0f, 1);
		params.WaypathID = 103608;

		ScriptEngine::Action_Goto(obj, params);
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		Vector3 spawnSpot (-3.701f, -55.248f, 2.704f);
		ScriptEngine::Create_Object ( "POW_Health_100", spawnSpot );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_RocketEmplacement_02_JDG, "")//103462
{
	int apache_id;
	bool secondary_apache_spawned;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(apache_id, 1);
		SAVE_VARIABLE(secondary_apache_spawned, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		secondary_apache_spawned = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == M01_MODIFY_YOUR_ACTION_JDG)
		{
			apache_id = param;
		}

		else if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			ScriptEngine::Debug_Message ( "***************************player has entered tailgun 02--send custom to apache\n" );
			GameObject * apache = ScriptEngine::Find_Object ( apache_id );
			if (apache != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, apache,  0, M01_MODIFY_YOUR_ACTION_04_JDG, 0 );
			}

			else if (secondary_apache_spawned == false)
			{
				secondary_apache_spawned = true;
				Vector3 spawnSpot (13.267f, -52.984f, 22.650f);
				GameObject * secondaryapache = ScriptEngine::Create_Object ( "NOD_Apache", spawnSpot );
				ScriptEngine::Attach_Script(secondaryapache, "M04_SecondaryApache02_JDG", "");
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Stationary_Tech_JDG, "Console_ID :int")
{
	int myConsole_id;
	bool deadYet;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( deadYet, 1 );
		SAVE_VARIABLE( myConsole_id, 2 );
	}

	void Created( GameObject * obj ) override
	{
		deadYet = false;
		myConsole_id = Get_Int_Parameter("Console_ID");
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, false );

		ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_START_ACTING_JDG, 1 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		deadYet = true;
		GameObject * engineer = ScriptEngine::Find_Object ( M04_ENGINEROOM_CHIEF_ENGINEER_JDG );
		if (engineer != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, engineer,  0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0 && deadYet == false)//
		{
			if (param == M01_START_ACTING_JDG)
			{
				GameObject * myConsole = ScriptEngine::Find_Object ( myConsole_id );
				if (myConsole != nullptr)
				{
					params.Set_Basic(this, 100, M01_START_ACTING_JDG);
					params.Set_Attack( myConsole, 0, 0, true );

					ScriptEngine::Action_Attack ( obj, params );

					float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
					ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				else
				{
					ScriptEngine::Debug_Message ( "**********************startionary tech cannot find his console\n" );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				params.Set_Basic(this, 50, M01_DOING_ANIMATION_01_JDG);
				params.Set_Animation( "H_A_CON2", false );
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
			}

			else if (param == M01_IVE_BEEN_KILLED_JDG)//M04 PanicGuy 01 Twiddler
			{
				ScriptEngine::Innate_Disable(obj);
				ScriptEngine::Action_Reset ( obj, 100 );
				params.Set_Basic(this, 100, M01_FACING_SPECIFIED_DIRECTION_01_JDG);
				params.Set_Attack( STAR, 0, 0, true );
				ScriptEngine::Action_Attack (obj, params);

				ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_03_JDG, 1 );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)
			{
				//Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				//ScriptEngine::Create_Sound ( "M04 PanicGuy 01 Twiddler", myPosition, obj );
				ScriptEngine::Action_Reset ( obj, 100 );
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_03_JDG);
				params.Set_Animation ("H_A_HOST_L1A", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//stand back up then start hunting player
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_05_JDG);
				params.Set_Animation ("H_A_HOST_L2C", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );
				params.Set_Basic( this, 50, M01_DOING_ANIMATION_02_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_02_JDG)
			{
				GameObject * myConsole = ScriptEngine::Find_Object ( myConsole_id );
				if (myConsole != nullptr)
				{
					params.Set_Basic(this, 100, M01_START_ACTING_JDG);
					params.Set_Attack( myConsole, 0, 0, true );

					ScriptEngine::Action_Attack ( obj, params );

					float delayTimer = ScriptEngine::Get_Random ( 0, 0.5f );
					ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}
			}

			else if (action_id == M01_DOING_ANIMATION_03_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_04_JDG);
				params.Set_Animation ("H_A_HOST_L2A", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_04_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_04_JDG);
				params.Set_Animation ("H_A_HOST_L2B", true);
				ScriptEngine::Action_Play_Animation (obj, params);

				float delayTimer = ScriptEngine::Get_Random ( 10, 30 );
				ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_04_JDG, delayTimer );
			}

			else if (action_id == M01_DOING_ANIMATION_05_JDG)
			{
				params.Set_Basic(this, 100, M01_DOING_ANIMATION_06_JDG);
				params.Set_Animation ("H_A_HOST_L1C", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (action_id == M01_DOING_ANIMATION_06_JDG)
			{
				if (obj)
				{
					ScriptEngine::Innate_Enable(obj);
					ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG", "");
				}
			}
		}
	}
};



DECLARE_SCRIPT(M04_Catwalk_Enter_Zone_01_JDG, "")//this guy comes out level 2 door
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * sniperTarget01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-14.594f, 27.717f, -9));
			ScriptEngine::Attach_Script(sniperTarget01, "M04_Doorway_Enterer_JDG", "-9.3 8.9 -9");

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_Catwalk_Enter_Zone_02_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * sniperTarget01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(12.913f, -27.628f, -9));
			ScriptEngine::Attach_Script(sniperTarget01, "M04_Doorway_Enterer_JDG", "10.868 -6.359 -9");

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_Doorway_Enterer_JDG, "first_location:vector3")
{
	Vector3 runToPosition;
	Vector3 leavePosition;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(runToPosition, 1);
		SAVE_VARIABLE(leavePosition, 2);
	}

	void Created( GameObject * obj ) override
	{
		runToPosition = Get_Vector3_Parameter("first_location");
		leavePosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Grant_Key( obj, 1, true );
		ScriptEngine::Grant_Key( obj, 2, true );
		ScriptEngine::Grant_Key( obj, 3, true );

		ActionParamsStruct params;
		params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( runToPosition, WALK, 1);

		ScriptEngine::Action_Goto ( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 powerupSpawnLocation = ScriptEngine::Get_Position (obj );
		powerupSpawnLocation.Z += 0.75f;

		static constexpr const char *powerups[2] =
		{//this is a list of potential powerups to be dropped by sniper target guys
			"POW_Health_100",
			"POW_Armor_100",
		};
		int random = ScriptEngine::Get_Random_Int(0, 2);

		//GameObject *spawn_effect = ScriptEngine::Create_Object( "Spawner Created Special Effect", powerupSpawnLocation );

		//if (spawn_effect)
		//{
		//	ScriptEngine::Attach_Script( spawn_effect, "M00_Create_Anim_Effect_DAY", "C4_EX1A.C4_EX1A");
		//}

		ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		switch (complete_reason)
		{
			case ACTION_COMPLETE_NORMAL:
				{
					switch (action_id)
					{
						case M01_WALKING_WAYPATH_01_JDG:
							{
								const char* animationName = M01_Choose_Search_Animation ( );

								params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
								params.Set_Animation( animationName, false );
								ScriptEngine::Action_Play_Animation ( obj, params );
							}
							break;

						case M01_DOING_ANIMATION_01_JDG:
							{
								params.Set_Basic( this, 45, M01_WALKING_WAYPATH_02_JDG );
								params.Set_Movement( leavePosition, WALK, 1);

								ScriptEngine::Action_Goto ( obj, params );

							}
							break;

						case M01_WALKING_WAYPATH_02_JDG:
							{
								ScriptEngine::Enable_Hibernation( obj, true );
								Vector3 myPosition = ScriptEngine::Get_Position ( obj );
								ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 5 );
							}
							break;
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M04_Hunter_Controller_JDG, "")//M04_ENGINEROOM_HUNTING_CONTROLLER_JDG
{
	int hunter_01_id;
	int hunter_02_id;
	int hunter_03_id;
	bool messagePlayed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(hunter_01_id, 1);
		SAVE_VARIABLE(hunter_02_id, 2);
		SAVE_VARIABLE(hunter_03_id, 3);
		SAVE_VARIABLE(messagePlayed, 4);
	}

	void Created( GameObject * /* obj */ ) override
	{
		hunter_01_id = 0;
		hunter_02_id = 0;
		hunter_03_id = 0;
		messagePlayed = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				GameObject *hunter_01 = ScriptEngine::Find_Object ( hunter_01_id );
				if (hunter_01 == nullptr)
				{
					messagePlayed = false;
					Vector3 hunter01_spawnlocation (-13.523f, 48.665f, -9.003f);
					hunter_01 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", hunter01_spawnlocation );
					hunter_01_id = ScriptEngine::Get_ID ( hunter_01 );
					ScriptEngine::Attach_Script(hunter_01, "M04_Hunter_JDG", "");
				}

				GameObject *hunter_02 = ScriptEngine::Find_Object ( hunter_02_id );
				if (hunter_02 == nullptr)
				{
					Vector3 hunter02_spawnlocation (11.943f, 42.201f, -15.000f);
					hunter_02 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", hunter02_spawnlocation );
					hunter_02_id = ScriptEngine::Get_ID ( hunter_02 );
					ScriptEngine::Attach_Script(hunter_02, "M04_Hunter_JDG", "");
				}

				GameObject *hunter_03 = ScriptEngine::Find_Object ( hunter_03_id );
				if (hunter_03 == nullptr)
				{
					Vector3 hunter03_spawnlocation (3.834f, 60.196f, -15.000f);
					hunter_03 = ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_AutoRifle", hunter03_spawnlocation );
					hunter_03_id = ScriptEngine::Get_ID ( hunter_03 );
					ScriptEngine::Attach_Script(hunter_03, "M04_Hunter_JDG", "");
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				if (messagePlayed == false)
				{
					messagePlayed = true;
					int conv = ScriptEngine::Create_Conversation( "M04_Eva_WarnsAboutHunters_Conversation", 30, 50, false);
					ScriptEngine::Join_Conversation( nullptr, conv, false, false, false );
					ScriptEngine::Start_Conversation( conv,  conv );
				}
			}

			else if (param == M01_GOTO_IDLE_JDG)
			{
				ScriptEngine::Destroy_Object ( obj );
			}
		}
	}
};

DECLARE_SCRIPT(M04_Hunter_JDG, "")
{
	float myPriority;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(myPriority, 1);
	}

	void Created( GameObject * obj ) override
	{
		if (STAR)
		{
			ScriptEngine::Grant_Key( obj, 1, true );
			ScriptEngine::Grant_Key( obj, 2, true );
			myPriority = 85;
			ScriptEngine::Enable_Hibernation( obj, false );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 5 );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, myPriority, M01_WALKING_WAYPATH_01_JDG );
				params.Set_Movement( STAR, RUN, 5 );
				ScriptEngine::Action_Goto( obj, params );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 5 );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//monitor how close you are to player
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				Vector3 playerPosition = ScriptEngine::Get_Position ( STAR );
				float distanceFromStar = ScriptEngine::Get_Distance ( myPosition, playerPosition );

				if (distanceFromStar <= 40)//******play eva warning conversation here*****
				{
					myPriority = 45;

					GameObject * hunterController = ScriptEngine::Find_Object ( M04_ENGINEROOM_HUNTING_CONTROLLER_JDG );
					if (hunterController != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, hunterController, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}

				else
				{
					myPriority = 100;
				}

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 5 );
			}
		}
	}

	void Destroyed( GameObject * obj ) override
	{
		GameObject * myController = ScriptEngine::Find_Object ( M04_ENGINEROOM_HUNTING_CONTROLLER_JDG );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, myController, 0, M01_START_ACTING_JDG, 1 );
		}
	}
};

/*DECLARE_SCRIPT(M04_Hunter_JDG, "MyLeaders_ID :int")
{
	void Created( GameObject * obj ) override
	{
		int leaders_id = Get_Int_Parameter("MyLeaders_ID");

		if (leaders_id == 0)
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 5 );
		}

		GameObject * leader = ScriptEngine::Find_Object ( leaders_id );

		ScriptEngine::Grant_Key( obj, 1, true );
		ScriptEngine::Grant_Key( obj, 2, true );

		ScriptEngine::Enable_Hibernation( obj, false );
		if (leader != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( leader, RUN, 2 );
			params.AttackActive = false;
			params.MoveFollow= true;

			ScriptEngine::Action_Attack( obj, params );
		}

		else
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( STAR, RUN, 5 );
			params.AttackActive = false;

			ScriptEngine::Action_Attack( obj, params );
		}

		ScriptEngine::Start_Timer ( obj, this, 120, M01_GOTO_IDLE_JDG );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		if (obj)
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
				params.Set_Movement( STAR, RUN, 10 );
				params.Set_Attack( STAR, 50, 0, true );
				params.AttackActive = true;

				ScriptEngine::Modify_Action ( obj, M01_WALKING_WAYPATH_01_JDG, params, true, true );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//monitor how close you are to player
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				Vector3 playerPosition = ScriptEngine::Get_Position ( STAR );
				float distanceFromStar = ScriptEngine::Get_Distance ( myPosition, playerPosition );

				if (distanceFromStar <= 40)// ******play eva warning conversation here*****
				{
					int conv = ScriptEngine::Create_Conversation( "M04_Eva_WarnsAboutHunters_Conversation", 100, 50, false);
					ScriptEngine::Join_Conversation( nullptr, conv, false, false, false );
					ScriptEngine::Start_Conversation( conv,  conv );
				}

				else
				{
					ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 5 );
				}
			}
		}
	}

	void Destroyed( GameObject * obj )
	{
		GameObject * myController = ScriptEngine::Find_Object ( M04_ENGINEROOM_HUNTING_CONTROLLER_JDG );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, myController, 0, M01_START_ACTING_JDG, 1 );
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float amount ) override
	{
		if (obj && damager == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy )
	{
		if (enemy == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Timer_Expired( GameObject * obj, int timer_id )
	{
		if (timer_id == M01_GOTO_IDLE_JDG)
		{
			//ScriptEngine::Enable_Hibernation( obj, true );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			Vector3 playerPosition = ScriptEngine::Get_Position ( STAR );
			float distanceFromStar = ScriptEngine::Get_Distance ( myPosition, playerPosition );

			if (distanceFromStar >= 150)
			{
				bool visible = ScriptEngine::Is_Object_Visible( obj, STAR );

				if (visible != true)
				{
					ScriptEngine::Apply_Damage( obj, 100000, "BlamoKiller", nullptr );
				}

				else
				{
					ScriptEngine::Start_Timer ( obj, this, 20, M01_GOTO_IDLE_JDG );
				}
			}

			else
			{
				ScriptEngine::Start_Timer ( obj, this, 20, M01_GOTO_IDLE_JDG );
			}
		}
	}
};*/

DECLARE_SCRIPT(M04_Front_ofThe_Boat_Population_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 3 );
	}
};

DECLARE_SCRIPT(M04_Cooks_Script_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Set_Player_Type (obj, SCRIPT_PLAYERTYPE_NOD );
	}
};

DECLARE_SCRIPT(M04_Prison_Keycard_CheckZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * prisonGuard01 = ScriptEngine::Find_Object (M04_ENGINEROOM_PRISONGUARD_01_JDG);
			GameObject * prisonGuard02 = ScriptEngine::Find_Object (M04_ENGINEROOM_PRISONGUARD_02_JDG);

			if (prisonGuard01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( prisonGuard01 );
			}

			if (prisonGuard02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( prisonGuard02 );
			}

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_TiberiumHold_EntryZone_and_Controller_JDG, "")//this guys ID number is M04_TIBERIUM_HOLD_CONTROLLER_JDG 100572
{
	bool entered;
	bool ok_to_clean_up;
	int cargo_conv1;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( entered, 1 );
		SAVE_VARIABLE( ok_to_clean_up, 2 );
		SAVE_VARIABLE( cargo_conv1, 3 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		entered = false;
		ok_to_clean_up = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR  && (ok_to_clean_up == true))
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 6000, 0 );//tell aft deck controller to clean up any remaining soldiers
		}

		if ( enterer == STAR  && (entered == false))
		{
			entered = true;

			if (STAR != nullptr)
			{
				cargo_conv1 = ScriptEngine::Create_Conversation( "M04_TiberiumHold_Entry_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, cargo_conv1, false, false, false);
				//ScriptEngine::Join_Conversation( STAR, cargo_conv1, false, false, false );
				ScriptEngine::Start_Conversation( cargo_conv1,  cargo_conv1 );
			}

			GameObject * cryochamber = ScriptEngine::Find_Object (M04_TIBERIUM_CRYOCHAMBER_JDG);
			if (cryochamber != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, cryochamber, 0, M01_START_ACTING_JDG, 0 );
			}

			GameObject *visceroidGuy01 = ScriptEngine::Find_Object (M04_TIBERIUM_HOLD_TECH01_JDG);
			if (visceroidGuy01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, visceroidGuy01, 0, M01_START_ACTING_JDG, 3 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_TibHold_MutantChamber_JDG, "")
{
	int cryoShakeCounter;
	bool released;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( cryoShakeCounter, 1 );
		SAVE_VARIABLE( released, 2 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", 0 );
		cryoShakeCounter = 0;
		released = false;
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		float myHealth = ScriptEngine::Get_Health ( obj );

		if (myHealth <= 10)
		{
			ScriptEngine::Set_Health ( obj, 0.25f );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Animation_Complete( GameObject * obj, const char * /* animation_name */ ) override
	{
		if (cryoShakeCounter < 10)
		{
			ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 9, false );
			cryoShakeCounter++;
		}

		else
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 9, false );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG && released == false)
			{
				released = true;
				ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 15, false );

				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Explosion ( "Explosion_Barrel_Toxic", myPosition, nullptr );

				Vector3 spawnLocation (-14.4f, -112.2f, -9);
				GameObject * realMutant = ScriptEngine::Create_Object ( "Mutant_0_Mutant", spawnLocation );
				ScriptEngine::Attach_Script(realMutant, "M04_TibHold_RealMutant_JDG", "");

				GameObject * simpleMutant = ScriptEngine::Find_Object (M04_TIBERIUM_SIMPLEMUTANT_JDG);
				if (simpleMutant != nullptr)
				{
					ScriptEngine::Destroy_Object ( simpleMutant );
				}

				float myFacing = ScriptEngine::Get_Facing ( obj );

				GameObject *destroyedVersion = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", myPosition );
				ScriptEngine::Set_Facing ( destroyedVersion, myFacing );
				ScriptEngine::Destroy_Object ( obj );
			}
		}
	}
};

DECLARE_SCRIPT(M04_TibHold_SimpleMutant_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Animation_Frame ( obj, "S_A_HUMAN.H_A_822A", 15 );
	}
};

/*DECLARE_SCRIPT(M11_MutantCrypt_Spawner03_Guy_JDG, "")
{
	typedef enum {
		IDLE,
		ATTACKING,
		DEAD,
	} M11_Mutant_State;

	M11_Mutant_State myState;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(myState, 1);
	}

	void Created( GameObject * obj ) override
	{
		myState = IDLE;
		float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 0 );
	}

	void Killed( GameObject * obj, GameObject * killer ) override
	{
		myState = DEAD;

		GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
		if (mutantController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy )
	{
		if (obj && enemy == STAR && myState == IDLE)//
		{
			myState = ATTACKING;
			char *soundName = M11_Choose_Mutant_Alerted_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );

			ScriptEngine::Create_Logical_Sound (obj, M11_MUTANT_IS_NEARBY_JDG, myPosition, 50 );
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound )
	{
		if ( sound.Type == M11_MUTANT_IS_NEARBY_JDG )
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			if (obj && myState == IDLE)
			{
				char *soundName = M11_Choose_Mutant_Idle_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}

			else if (obj && myState != DEAD)
			{
				char *soundName = M11_Choose_Mutant_Attack_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 0, 10 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}
		}

		else if (obj && param == M01_PICK_A_NEW_LOCATION_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
			params.Set_Movement( Vector3(-15.128f, 17.965f, -63.748f), .1f, 1 );

			ScriptEngine::Action_Goto( obj, params );
		}

		else if (obj && param == M01_HUNT_THE_PLAYER_JDG)
		{
			if (STAR)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 100, M01_HUNT_THE_PLAYER_JDG);
				params.Set_Movement(STAR, 2, 1);

				ScriptEngine::Action_Goto ( obj, params );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (obj && action_id == M01_PICK_A_NEW_LOCATION_JDG)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 80, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement( Vector3(0,0,0), .1f, 1 );

				int random = ScriptEngine::Get_Random_Int(0, 4);

				if (random == 0)
				{
					params.WaypathID = 100076;
					params.WaypointStartID = 100077;
					params.WaypointEndID = 100085;
				}

				else if (random == 1)
				{
					params.WaypathID = 100087;
					params.WaypointStartID = 100088;
					params.WaypointEndID = 100096;
				}

				else if (random == 3)
				{
					params.WaypathID = 100076;
					params.WaypointStartID = 100085;
					params.WaypointEndID = 100077;
				}

				else
				{
					params.WaypathID = 100087;
					params.WaypointStartID = 100096;
					params.WaypointEndID = 100088;
				}

				ScriptEngine::Action_Goto( obj, params );


			}

			else if (obj && action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				const char* animationName = M01_Choose_Idle_Animation ( );

				params.Set_Basic( this, 60, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation (animationName, false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (obj && action_id == M01_DOING_ANIMATION_01_JDG)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
				params.Set_Movement( Vector3(-15.128f, 17.965f, -63.748f), .1f, 1 );

				ScriptEngine::Action_Goto( obj, params );
			}

			else if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					char *soundName = M11_Choose_Mutant_Attack_Sound ( );
					Vector3 myPosition = ScriptEngine::Get_Position ( obj );
					ScriptEngine::Create_Sound ( soundName, myPosition, obj );

					const char* animationName = M11_Choose_Mutant_Attack_Animation ( );

					params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
					params.Set_Animation (animationName, false);
					ScriptEngine::Action_Play_Animation (obj, params);

					ScriptEngine::Apply_Damage( STAR, 5, "TiberiumRaw", obj );
					ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
				}
			}
		}
	}
};*/

DECLARE_SCRIPT(M04_TibHold_RealMutant_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation(obj, false );
		static constexpr const char *mutantAnimations[4] =
		{
			"S_A_HUMAN.H_A_A0A0_L02",
			"S_A_HUMAN.H_A_A0A0_L04",
			"S_A_HUMAN.H_A_J11C",
			"S_A_HUMAN.H_A_J21C",
		};
		int random = ScriptEngine::Get_Random_Int(0, 4);

		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation( mutantAnimations[random], false );
		ScriptEngine::Action_Play_Animation (  obj, params );


	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (obj && param == M01_HUNT_THE_PLAYER_JDG)
		{
			if (STAR)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 100, M01_GOING_TO_HAVOC_JDG);
				params.Set_Movement(STAR, 1, 0.5f);

				ScriptEngine::Action_Goto ( obj, params );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
				params.Set_Movement( STAR, 1.0f, 0.5f );
				ScriptEngine::Action_Goto (obj, params);
			}

			else if (action_id == M01_GOING_TO_HAVOC_JDG)
			{
				if (obj && STAR)
				{
					const char *soundName = M11_Choose_Mutant_Attack_Sound ( );
					Vector3 myPosition = ScriptEngine::Get_Position ( obj );
					ScriptEngine::Create_Sound ( soundName, myPosition, obj );

					const char* animationName = M11_Choose_Mutant_Attack_Animation ( );

					params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
					params.Set_Animation (animationName, false);
					ScriptEngine::Action_Play_Animation (obj, params);

					ScriptEngine::Apply_Damage( STAR, 5, "TiberiumRaw", obj );
					ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_TibHold_MutantChambers_Controller_JDG, "")//M04_TIBERIUM_CHAMBERCONTROLLER_JDG 104113
{
	int mutantChamber01_id;
	int mutantChamber02_id;
	int mutantChamber03_id;
	int mutantChamber04_id;

	int mutant01_id;
	int mutant02_id;
	int mutant03_id;
	int mutant04_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( mutantChamber01_id, 1 );
		SAVE_VARIABLE( mutantChamber02_id, 2 );
		SAVE_VARIABLE( mutantChamber03_id, 3 );
		SAVE_VARIABLE( mutantChamber04_id, 4 );

		SAVE_VARIABLE( mutant01_id, 5 );
		SAVE_VARIABLE( mutant02_id, 6 );
		SAVE_VARIABLE( mutant03_id, 7 );
		SAVE_VARIABLE( mutant04_id, 8 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		Vector3 mutantChamber01_Position (12.322f, -113.537f, -9);
		float mutantChamber01_Facing = 155;
		Vector3 mutantChamber02_Position (5.007f, -111.035f, -9);
		float mutantChamber02_Facing = -90;
		Vector3 mutantChamber03_Position (-5.646f, -113.485f, -9);
		float mutantChamber03_Facing = -90;
		Vector3 mutantChamber04_Position (-11.817f, -111.029f, -9);
		float mutantChamber04_Facing = -90;

		Vector3 mutant01_Position (12.330f, -113.560f, -8.224f);
		float mutant01_Facing = 65;
		Vector3 mutant02_Position (4.999f, -111.061f, -8.254f);
		float mutant02_Facing = -175;
		Vector3 mutant03_Position (-5.614f, -113.500f, -8.263f);
		float mutant03_Facing = 180;
		Vector3 mutant04_Position (-11.767f, -111.026f, -8.257f);
		float mutant04_Facing = 180;

		GameObject * mutantChamber01 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", mutantChamber01_Position );
		ScriptEngine::Set_Facing ( mutantChamber01, mutantChamber01_Facing );
		ScriptEngine::Attach_Script(mutantChamber01, "M04_TibHold_MutantChamber_NoThreat_JDG", "");
		GameObject * mutantChamber02 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", mutantChamber02_Position );
		ScriptEngine::Set_Facing ( mutantChamber02, mutantChamber02_Facing );
		ScriptEngine::Attach_Script(mutantChamber02, "M04_TibHold_MutantChamber_NoThreat_JDG", "");
		GameObject * mutantChamber03 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", mutantChamber03_Position );
		ScriptEngine::Set_Facing ( mutantChamber03, mutantChamber03_Facing );
		ScriptEngine::Attach_Script(mutantChamber03, "M04_TibHold_MutantChamber_NoThreat_JDG", "");
		GameObject * mutantChamber04 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", mutantChamber04_Position );
		ScriptEngine::Set_Facing ( mutantChamber04, mutantChamber04_Facing );
		ScriptEngine::Attach_Script(mutantChamber04, "M04_TibHold_MutantChamber_NoThreat_JDG", "");

		GameObject * mutant01 = ScriptEngine::Create_Object ( "M11_Mutant_1_Simple", mutant01_Position );
		ScriptEngine::Set_Facing ( mutant01, mutant01_Facing );
		ScriptEngine::Attach_Script(mutant01, "M04_TibHold_Mutant_NoThreat_JDG", "");
		GameObject * mutant02 = ScriptEngine::Create_Object ( "M11_Mutant_1_Simple", mutant02_Position );
		ScriptEngine::Set_Facing ( mutant02, mutant02_Facing );
		ScriptEngine::Attach_Script(mutant02, "M04_TibHold_Mutant_NoThreat_JDG", "");
		GameObject * mutant03 = ScriptEngine::Create_Object ( "M11_Mutant_1_Simple", mutant03_Position );
		ScriptEngine::Set_Facing ( mutant03, mutant03_Facing );
		ScriptEngine::Attach_Script(mutant03, "M04_TibHold_Mutant_NoThreat_JDG", "");
		GameObject * mutant04 = ScriptEngine::Create_Object ( "M11_Mutant_1_Simple", mutant04_Position );
		ScriptEngine::Set_Facing ( mutant04, mutant04_Facing );
		ScriptEngine::Attach_Script(mutant04, "M04_TibHold_Mutant_NoThreat_JDG", "");

		mutantChamber01_id = ScriptEngine::Get_ID ( mutantChamber01 );
		mutantChamber02_id = ScriptEngine::Get_ID ( mutantChamber02 );
		mutantChamber03_id = ScriptEngine::Get_ID ( mutantChamber03 );
		mutantChamber04_id = ScriptEngine::Get_ID ( mutantChamber04 );

		mutant01_id = ScriptEngine::Get_ID ( mutant01 );
		mutant02_id = ScriptEngine::Get_ID ( mutant02 );
		mutant03_id = ScriptEngine::Get_ID ( mutant03 );
		mutant04_id = ScriptEngine::Get_ID ( mutant04 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)//a chamber's been destroyed--do swap and whatnot
			{
				ScriptEngine::Debug_Message ( "***************************Mutant chamber received custom that non-destroyed version has been killed\n" );
				GameObject * mutantChamber01 = ScriptEngine::Find_Object ( mutantChamber01_id );
				GameObject * mutantChamber02 = ScriptEngine::Find_Object ( mutantChamber02_id );
				GameObject * mutantChamber03 = ScriptEngine::Find_Object ( mutantChamber03_id );
				GameObject * mutantChamber04 = ScriptEngine::Find_Object ( mutantChamber04_id );

				Vector3 chamberPosition = ScriptEngine::Get_Position ( sender );
				float chamberFacing = ScriptEngine::Get_Facing ( sender );

				GameObject *destroyedVersion = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", chamberPosition );
				ScriptEngine::Set_Facing ( destroyedVersion, chamberFacing );
				ScriptEngine::Destroy_Object ( sender );
				GameObject * simpleMutant = nullptr;

				if (sender == mutantChamber01)
				{
					simpleMutant = ScriptEngine::Find_Object ( mutant01_id );
				}

				else if (sender == mutantChamber02)
				{
					simpleMutant = ScriptEngine::Find_Object ( mutant02_id );
				}

				else if (sender == mutantChamber03)
				{
					simpleMutant = ScriptEngine::Find_Object ( mutant03_id );
				}

				else if (sender == mutantChamber04)
				{
					simpleMutant = ScriptEngine::Find_Object ( mutant04_id );
				}

				if (simpleMutant != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, simpleMutant, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_TibHold_MutantChamber_NoThreat_JDG, "")
{
	bool switched;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( switched, 1 );
	}

	void Created( GameObject * obj ) override
	{
		switched = false;
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", 0 );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		float myHealth = ScriptEngine::Get_Health ( obj );

		if (obj && switched == true)
		{
			ScriptEngine::Set_Health ( obj, 0.25f );
		}

		else if (obj && myHealth <= 10 && switched == false)
		{
			switched = true;
			ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 15, false );

			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Explosion ( "Explosion_Barrel_Toxic", myPosition, nullptr );

			ScriptEngine::Set_Health ( obj, 0.25f );
			GameObject * myController = ScriptEngine::Find_Object ( M04_TIBERIUM_CHAMBERCONTROLLER_JDG );

			if (myController != nullptr)
			{
				ScriptEngine::Debug_Message ( "***************************Mutant chamber sending custom that non-destroyed version has been killed\n" );
				ScriptEngine::Send_Custom_Event( obj, myController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
			}
		}

		else
		{
			ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 9, false );
		}
	}
};

DECLARE_SCRIPT(M04_TibHold_Mutant_NoThreat_JDG, "")
{
	float myMaxHealth;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( myMaxHealth, 1 );
	}

	void Created( GameObject * obj ) override
	{
		myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
		ScriptEngine::Set_Animation_Frame ( obj, "S_A_HUMAN.H_A_822A", 15 );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (obj)
		{
			ScriptEngine::Set_Health ( obj, myMaxHealth );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)//your chamber's been destroyed--do swap and whatnot
			{
				ScriptEngine::Set_Animation ( obj, "S_C_Human.H_C_Tubedie", false, nullptr, 0, 22, false );
			}
		}
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "S_C_Human.H_C_Tubedie") == 0)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_Visceroid_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
		params.Set_Movement( STAR, 1.0f, 1 );
		ScriptEngine::Action_Goto (obj, params);

		int cargo_conv1 = ScriptEngine::Create_Conversation( "M04_TiberiumHold_Visceroid_Conversation", 100, 1000, false);
		ScriptEngine::Join_Conversation( nullptr, cargo_conv1, false, false, false);
		//ScriptEngine::Join_Conversation( STAR, cargo_conv1, false, false, false );
		ScriptEngine::Start_Conversation( cargo_conv1,  cargo_conv1 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				GameObject * myTarget = ScriptEngine::Find_Closest_Soldier ( myPosition, 0, 10, true );

				if (myTarget != nullptr)
				{
					ActionParamsStruct params;
					params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
					params.Set_Movement( myTarget, 1.0f, 3 );
					ScriptEngine::Action_Goto (obj, params);
				}
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		if (action_id == M01_GOING_TO_HAVOC_JDG)
		{
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			GameObject * myTarget = ScriptEngine::Find_Closest_Soldier ( myPosition, 0, 10, true );

			if (myTarget != nullptr)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_START_ATTACKING_01_JDG );
				params.Set_Attack( myTarget, 5, 0, true );
				ScriptEngine::Action_Attack (obj, params);
			}

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 2 );
		}
	}
};

DECLARE_SCRIPT(M04_Visceroid_Dude_01_JDG, "")//M04_TIBERIUM_HOLD_TECH01_JDG	 102147
{
	int spill_conv1;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( spill_conv1, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, false );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				GameObject *visceroidGuy02 = ScriptEngine::Find_Object (M04_TIBERIUM_HOLD_TECH02_JDG);
				if (obj && visceroidGuy02 != nullptr)
				{
					spill_conv1 = ScriptEngine::Create_Conversation( "M04_Tiberium_Tech_Conversation", 30, 20, true);
					ScriptEngine::Join_Conversation( obj, spill_conv1, false, false, false);
					ScriptEngine::Join_Conversation( visceroidGuy02, spill_conv1, false, false, false );
					ScriptEngine::Start_Conversation( spill_conv1,  spill_conv1 );
					ScriptEngine::Monitor_Conversation(  obj, spill_conv1);
				}
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			if (action_id == spill_conv1)
			{
				GameObject *visceroidGuy02 = ScriptEngine::Find_Object (M04_TIBERIUM_HOLD_TECH02_JDG);
				if (visceroidGuy02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroidGuy02, 0, M01_START_ACTING_JDG, 2 );
				}
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );

				ActionParamsStruct params;
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
				params.Set_Movement( Vector3 (-9.622f, -5.406f, -15), WALK, 5, false );
				ScriptEngine::Action_Goto (  obj, params );
			}
		}

		else if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				ScriptEngine::Set_Innate_Soldier_Home_Location (obj, Vector3 (-9.622f, -5.406f, -15), 10 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_Visceroid_Dude_02_JDG, "")//M04_TIBERIUM_HOLD_TECH02_JDG	 102148
{
	bool first_time_damaged;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( first_time_damaged, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard ( obj, false );
		first_time_damaged = true;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation( "H_A_A0A0_L28A", false );
				ScriptEngine::Action_Play_Animation (  obj, params );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				myPosition.Z += 0.25f;
				ScriptEngine::Create_Explosion ( "Explosion_Barrel_Toxic", myPosition, nullptr );//
				GameObject * visceroid = ScriptEngine::Create_Object ( "Visceroid", myPosition );
				ScriptEngine::Attach_Script(visceroid, "M04_Visceroid_JDG", "");

				ScriptEngine::Destroy_Object ( obj );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_01_JDG)
			{
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
				params.Set_Animation( "H_A_A0A0_L28B", true );
				ScriptEngine::Action_Play_Animation (  obj, params );
			}
		}
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		ScriptEngine::Set_Health (obj, 0.25f );

		if (first_time_damaged == true)
		{
			first_time_damaged = false;
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_03_JDG );
			params.Set_Animation( "H_A_601A", false );
			ScriptEngine::Action_Play_Animation (  obj, params );

			float delayTimer = ScriptEngine::Get_Random ( 1, 2 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
		}
	}
};



DECLARE_SCRIPT(M04_Keycard_01_Script_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
	}

	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 590, 2 );
		}
	}
};

DECLARE_SCRIPT(M04_Keycard_02_Script_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
	}

	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 510, 4 );
			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 1001, 2 );//tell objective controller to go back to regular background music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_TIBERIUM_HOLD_CONTROLLER_JDG) , 0, 2000, 0);//this custom tells the tib zone that its okay clean up aft deck soldiers next time entered

			GameObject *zone02 = ScriptEngine::Find_Object (100438);//stays--subbay door
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}

			GameObject *zone04 = ScriptEngine::Find_Object (100440);//stays--subbay door
			if (zone04 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone04 );
			}

			GameObject *zone06 = ScriptEngine::Find_Object (100442);//stays--yellow door by prison
			if (zone06 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone06 );
			}

			GameObject *zone01 = ScriptEngine::Find_Object (104762);//stays-- yellow door by big sam
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_Keycard_03_Script_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
	}

	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 520, 4 );
			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 470, 8 );

			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 1001, 2 );//tell objective controller to go back to regular background music
																						   //controller to start action
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101126), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101127), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101128), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101129), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101130), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101131), 0, 100, 0 );//tells subbay zones to okay battle music

			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101132), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101133), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101134), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101135), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101136), 0, 100, 0 );//tells subbay zones to okay battle music
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101137), 0, 100, 0 );//tells subbay zones to okay battle music

			ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_Shotgun", Vector3(0.084f, 95.837f, 3) );
			ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_Shotgun", Vector3(-1.983f, 72.772f, 0) );
			ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_Shotgun", Vector3(9.840f, 89.533f, 0) );
			ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_Shotgun", Vector3(11.822f, 39.659f, -15) );
			ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_Shotgun", Vector3(-13.725f, 38.871f, -9) );
			ScriptEngine::Create_Object ( "Nod_Minigunner_2SF_Shotgun", Vector3(-10.606f, 32.021f, 0) );
		}
	}
};

DECLARE_SCRIPT(M04_Ships_Captain_JDG, "")//this guys ID number is M04_SHIPS_CAPTAIN_JDG 100401
{
#define	STATIONARY_DELAY_TIMER		0x1234553

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Start_Timer ( obj, this, 1, STATIONARY_DELAY_TIMER );
	}

	void Timer_Expired( GameObject * obj, int timer_id ) override
	{
		if (timer_id == STATIONARY_DELAY_TIMER)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 spawn_location = ScriptEngine::Get_Position ( obj );
		spawn_location.Z = spawn_location.Z + 0.25f;

		ScriptEngine::Create_Object ( "M04_L03_Keycard", spawn_location );
	}
};

DECLARE_SCRIPT(M04_Ships_FirstMate_JDG, "")//this guys ID number is M04_SHIPS_FIRST_MATE_JDG  100400
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 spawn_location = ScriptEngine::Get_Position ( obj );
		spawn_location.Z = spawn_location.Z + 0.25f;

		ScriptEngine::Create_Object ( "M04_L02_Keycard", spawn_location );

		//putting down patrol guys for trip back towards captain
		GameObject * samRoomPatrolGuy = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(11.503f, -51.722f, 2.704f));
		ScriptEngine::Attach_Script(samRoomPatrolGuy, "M04_PostFirstMate_SamRoom_PatrolGuy_JDG", "");

		GameObject * missileRoomPatrolGuy = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(13.213f, -53.008f, -15.0f));
		ScriptEngine::Attach_Script(missileRoomPatrolGuy, "M04_PostFirstMate_MissileRoom_PatrolGuy_JDG", "");

		GameObject * subBayPatrolGuy = ScriptEngine::Create_Object ( "Nod_MiniGunner_0", Vector3(12.527f, 19.447f, -15.0f));
		ScriptEngine::Attach_Script(subBayPatrolGuy, "M04_Front_ofThe_Boat_Population_JDG", "");

		GameObject * officerQuartersPatrolGuy = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(8.342f, 80.0f, 3.0f));
		ScriptEngine::Attach_Script(officerQuartersPatrolGuy, "M04_PostFirstMate_OfficerQuarters_PatrolGuy_JDG", "");

		GameObject * foreDeckPatrolGuy01 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(0.345f, 106.479f, 3.001f));
		ScriptEngine::Attach_Script(foreDeckPatrolGuy01, "M04_PostFirstMate_FrontDeck_PatrolGuy01_JDG", "");

		GameObject * foreDeckPatrolGuy02 = ScriptEngine::Create_Object ( "Nod_MiniGunner_1Off", Vector3(-13.320f, 71.885f, 0));
		ScriptEngine::Attach_Script(foreDeckPatrolGuy02, "M04_PostFirstMate_FrontDeck_PatrolGuy02_JDG", "");
	}
};

DECLARE_SCRIPT(M04_Prison_Warden_JDG, "")//this guys ID is M04_PRISON_WARDEN_JDG 100399
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		GameObject * medTech = ScriptEngine::Find_Object ( M04_MEDLAB_TECHNICIAN_JDG);
		if (medTech != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 90, M01_START_ATTACKING_01_JDG );
			params.Set_Attack( medTech, 0, 0, true);
			ScriptEngine::Action_Attack(obj, params);
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager == STAR)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}
	}

	void Custom (GameObject* obj, int /* type */, intptr_t param, GameObject* /* sender */) override
	{
		ActionParamsStruct params;

		if (param == 100)//warden has received custom to start acting
		{
			GameObject *medTech = ScriptEngine::Find_Object ( M04_MEDLAB_TECHNICIAN_JDG);
			if (obj && medTech != nullptr)
			{
				int medlabConversation = ScriptEngine::Create_Conversation( "M04_MedLab_Conversation", 80, 50, true);
				ScriptEngine::Join_Conversation( obj, medlabConversation, true, true, true );
				ScriptEngine::Join_Conversation( medTech, medlabConversation, true, true, true );
				ScriptEngine::Start_Conversation( medlabConversation, medlabConversation );
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 spawn_location = ScriptEngine::Get_Position ( obj );
		spawn_location.Z = spawn_location.Z + 0.25f;

		ScriptEngine::Create_Object ( "M04_L01_Keycard", spawn_location );
	}
};

DECLARE_SCRIPT(M04_MedLab_Tech_JDG, "")//this guys ID is M04_MEDLAB_TECHNICIAN_JDG 101202
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );

		GameObject * prisonGuard = ScriptEngine::Find_Object ( M04_PRISON_WARDEN_JDG );
		if (prisonGuard != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 90, M01_START_ATTACKING_01_JDG );
			params.Set_Attack( prisonGuard, 0, 0, true);
			ScriptEngine::Action_Attack(obj, params);
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				//ScriptEngine::Action_Reset ( obj, 100 );
				ActionParamsStruct params;
				params.Set_Basic( this, 45, M01_WALKING_WAYPATH_01_JDG );
				params.Set_Movement( Vector3 (5.3f, -83.6f, 3.000f), RUN, 1 );
				ScriptEngine::Action_Goto( obj, params );
			}

			else if (param == 100)
			{
				GameObject * prisonGuard = ScriptEngine::Find_Object ( M04_PRISON_WARDEN_JDG );
				if (prisonGuard != nullptr)
				{
					ScriptEngine::Action_Reset ( obj, 100 );
					Vector3 wardenLocation = ScriptEngine::Get_Position ( prisonGuard );
					ActionParamsStruct params;
					params.Set_Basic( this, 100, M01_START_ATTACKING_01_JDG );
					params.Set_Face_Location( wardenLocation, 2 );
					ScriptEngine::Action_Face_Location ( obj, params );
				}
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic(this, 100, M01_START_ACTING_JDG);
			params.Set_Animation ("H_A_A0C0", true);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager == STAR)
		{
			if (obj)
			{
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, 0 );
			}
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (obj && enemy == STAR)
		{
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, 0 );
		}
	}
};

/****************************************************************************************************************
The Following are all the scripts associated with the objectives...including controller
****************************************************************************************************************/

DECLARE_SCRIPT(M04_Prisoners_Rescued_Controller_JDG, "")
{
	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 1)
			{
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_END_FIREFIGHT_CONTROLLER_JDG), 0, 1, 2 );//telling firefight controller to allow end firefight
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 500, 2 );
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_AFT_DECK_CONTROLLER_JDG), 0, 3000, 2 );//tell aft deck controller that prisoners have been rescued
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 410, 6 );

				ScriptEngine::Set_Position ( STAR, Vector3(2.542f, -80.526f, 0) );
				ScriptEngine::Set_Facing ( STAR, -125 );
				ScriptEngine::Control_Enable ( STAR, true );
				ScriptEngine::Set_Is_Rendered( STAR, true );
			}
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_ObjectiveZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 440, 0 );//tell objective controller to announce missile objective

			GameObject *zone01 = ScriptEngine::Find_Object (100429);//stays
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			GameObject *zone03 = ScriptEngine::Find_Object (100431);//stays
			if (zone03 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone03 );
			}
		}
	}
};



DECLARE_SCRIPT(M04_Announce_Keycard_02_Objective_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 410, 5 );

			GameObject *zone02 = ScriptEngine::Find_Object (100438);//stays
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}

			GameObject *zone04 = ScriptEngine::Find_Object (100440);//stays
			if (zone04 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone04 );
			}

			GameObject *zone06 = ScriptEngine::Find_Object (100442);//stays also add 104762
			if (zone06 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone06 );
			}

			GameObject *zone01 = ScriptEngine::Find_Object (104762);//stays
			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}
		}
	}
};



DECLARE_SCRIPT(M04_EngineRoom_SniperScript_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		int easy = 0;

		int currentDifficulty = ScriptEngine::Get_Difficulty_Level( );

		if (currentDifficulty == easy)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_Target01_JDG, "")//100420
{
	bool poked_yet;
	Vector3 myPosition;
	bool pokable;
	int sabotageSound;
	int timesSoundPlayed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked_yet, 1 );
		SAVE_VARIABLE( myPosition, 2 );
		SAVE_VARIABLE( pokable, 3 );
		SAVE_VARIABLE( sabotageSound, 4 );
		SAVE_VARIABLE( timesSoundPlayed, 5 );
	}

	void Created( GameObject * obj ) override
	{
		timesSoundPlayed = 0;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		pokable = false;
		poked_yet = false;
		myPosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", 1 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
			if (param == sabotageSound)
			{
				timesSoundPlayed++;

				if (timesSoundPlayed >= 4)
				{
					ScriptEngine::Create_Sound ("SFX.Electric_Fire_02", myPosition, obj );
				}

				else
				{
					sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
					ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				}
			}
		}

		else if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				if (poked_yet == false)
				{
					pokable = true;
					ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, RADAR_BLIP_SHAPE_OBJECTIVE );
					ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				}
			}
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (poked_yet == false && pokable == true)
		{
			if (STAR != nullptr)
			{
				sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
				ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				poked_yet = true;
				ScriptEngine::Set_Animation ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 100, 0 );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );//104693

				ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, -1 );
				ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, -1 );

				GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
				if (pogController != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 300, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_Target02_JDG, "")//100421
{
	bool poked_yet;
	Vector3 myPosition;
	bool pokable;
	int sabotageSound;
	int timesSoundPlayed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked_yet, 1 );
		SAVE_VARIABLE( myPosition, 2 );
		SAVE_VARIABLE( pokable, 3 );
		SAVE_VARIABLE( sabotageSound, 4 );
		SAVE_VARIABLE( timesSoundPlayed, 5 );
	}

	void Created( GameObject * obj ) override
	{
		timesSoundPlayed = 0;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		pokable = false;
		poked_yet = false;
		myPosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", 1 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
			if (param == sabotageSound)
			{
				timesSoundPlayed++;

				if (timesSoundPlayed >= 4)
				{
					ScriptEngine::Create_Sound ("SFX.Electric_Fire_02", myPosition, obj );
				}

				else
				{
					sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
					ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				}
			}
		}

		else if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				if (poked_yet == false)
				{
					pokable = true;
					ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, RADAR_BLIP_SHAPE_OBJECTIVE );
					ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				}
			}
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (poked_yet == false && pokable == true)
		{
			if (STAR != nullptr)
			{
				sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
				ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				poked_yet = true;
				ScriptEngine::Set_Animation ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 110, 0 );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );

				ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, -1 );
				ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, -1 );

				GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
				if (pogController != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 300, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_Target03_JDG, "")//100422
{
	bool poked_yet;
	Vector3 myPosition;
	bool pokable;
	int sabotageSound;
	int timesSoundPlayed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked_yet, 1 );
		SAVE_VARIABLE( myPosition, 2 );
		SAVE_VARIABLE( pokable, 3 );
		SAVE_VARIABLE( sabotageSound, 4 );
		SAVE_VARIABLE( timesSoundPlayed, 5 );
	}

	void Created( GameObject * obj ) override
	{
		timesSoundPlayed = 0;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		pokable = false;
		poked_yet = false;
		myPosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", 1 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
			if (param == sabotageSound)
			{
				timesSoundPlayed++;

				if (timesSoundPlayed >= 4)
				{
					ScriptEngine::Create_Sound ("SFX.Electric_Fire_02", myPosition, obj );
				}

				else
				{
					sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
					ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				}
			}
		}

		else if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				if (poked_yet == false)
				{
					pokable = true;
					ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, RADAR_BLIP_SHAPE_OBJECTIVE );
					ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				}
			}
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (poked_yet == false && pokable == true)
		{
			if (STAR != nullptr)
			{
				sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
				ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				poked_yet = true;
				ScriptEngine::Set_Animation ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 120, 0 );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );

				ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, -1 );
				ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, -1 );

				GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
				if (pogController != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 300, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_MissileRoom_Target04_JDG, "")//100423
{
	bool poked_yet;
	Vector3 myPosition;
	bool pokable;
	int sabotageSound;
	int timesSoundPlayed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked_yet, 1 );
		SAVE_VARIABLE( myPosition, 2 );
		SAVE_VARIABLE( pokable, 3 );
		SAVE_VARIABLE( sabotageSound, 4 );
		SAVE_VARIABLE( timesSoundPlayed, 5 );
	}

	void Created( GameObject * obj ) override
	{
		timesSoundPlayed = 0;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		pokable = false;
		poked_yet = false;
		myPosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", 1 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
			if (param == sabotageSound)
			{
				timesSoundPlayed++;

				if (timesSoundPlayed >= 4)
				{
					ScriptEngine::Create_Sound ("SFX.Electric_Fire_02", myPosition, obj );
				}

				else
				{
					sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
					ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				}
			}
		}

		else if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				if (poked_yet == false)
				{
					pokable = true;
					ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, RADAR_BLIP_SHAPE_OBJECTIVE );
					ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_PRIMARY_OBJECTIVE );
				}
			}
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (poked_yet == false && pokable == true)
		{
			if (STAR != nullptr)
			{
				sabotageSound = ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
				ScriptEngine::Monitor_Sound ( obj, sabotageSound );
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				poked_yet = true;
				ScriptEngine::Set_Animation ( obj, "DSP_L4MISSRACK.DSP_L4MISSRACK", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 130, 0 );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );

				ScriptEngine::Set_Obj_Radar_Blip_Shape ( obj, -1 );
				ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, -1 );

				GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
				if (pogController != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 300, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Target01_JDG, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Create_Explosion ( "Explosion_Barrel_Fuel", myPosition, obj );
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 300, 0 );

		GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
		if (pogController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 200, 0 );
		}

		GameObject * engineBuildingController = ScriptEngine::Find_Object (M04_ENGINEROOM_BUILDING_CONTROLLER_JDG);
		if (engineBuildingController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, engineBuildingController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject * myOperator = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_04_JDG);
		if (myOperator != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, myOperator, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Target02_JDG, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Create_Explosion ( "Explosion_Barrel_Fuel", myPosition, obj );
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 310, 0 );

		GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
		if (pogController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 200, 0 );
		}

		GameObject * engineBuildingController = ScriptEngine::Find_Object (M04_ENGINEROOM_BUILDING_CONTROLLER_JDG);
		if (engineBuildingController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, engineBuildingController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject * myOperator = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_01_JDG);
		if (myOperator != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, myOperator, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Target03_JDG, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Create_Explosion ( "Explosion_Barrel_Fuel", myPosition, obj );
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 320, 0 );

		GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
		if (pogController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 200, 0 );
		}

		GameObject * engineBuildingController = ScriptEngine::Find_Object (M04_ENGINEROOM_BUILDING_CONTROLLER_JDG);
		if (engineBuildingController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, engineBuildingController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject * myOperator = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_02_JDG);
		if (myOperator != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, myOperator, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M04_EngineRoom_Target04_JDG, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Create_Explosion ( "Explosion_Barrel_Fuel", myPosition, obj );
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 330, 0 );

		GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
		if (pogController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 200, 0 );
		}

		GameObject * engineBuildingController = ScriptEngine::Find_Object (M04_ENGINEROOM_BUILDING_CONTROLLER_JDG);
		if (engineBuildingController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, engineBuildingController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject * myOperator = ScriptEngine::Find_Object (M04_ENGINEROOM_TECH_03_JDG);
		if (myOperator != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, myOperator, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M04_ApacheRoom_Second_EntryZone_TopFloor_JDG, "")//105011 105012
{
	int apache_id;
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( apache_id, 1 );
		SAVE_VARIABLE( commandClearance, 2 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		apache_id = 0;
		commandClearance = false;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_CHOPPER_IS_SENDING_ID_JDG)//apache is sending you his id
			{
				apache_id = ScriptEngine::Get_ID ( sender );
			}

			else if (param == M01_START_ACTING_JDG)
			{
				commandClearance = true;
			}
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (commandClearance == true)
			{
				GameObject * apache = ScriptEngine::Find_Object ( apache_id );
				if (apache != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, apache, 0, M01_MODIFY_YOUR_ACTION_07_JDG, 0 );
				}

				GameObject * zone01 = ScriptEngine::Find_Object ( 105011 );
				GameObject * zone02 = ScriptEngine::Find_Object ( 105012 );
				GameObject * zone03 = ScriptEngine::Find_Object ( 105081 );
				GameObject * zone04 = ScriptEngine::Find_Object ( 105082 );
				GameObject * zone05 = ScriptEngine::Find_Object ( 105083 );
				GameObject * zone06 = ScriptEngine::Find_Object ( 105084 );

				if (zone01 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone01 );
				}

				if (zone02 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone02 );
				}

				if (zone03 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone03 );
				}

				if (zone04 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone04 );
				}

				if (zone05 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone05 );
				}

				if (zone06 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone06 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_ApacheRoom_Second_EntryZone_MiddleFloor_JDG, "")//105081 105082
{
	int apache_id;
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( apache_id, 1 );
		SAVE_VARIABLE( commandClearance, 2 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		apache_id = 0;
		commandClearance = false;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_CHOPPER_IS_SENDING_ID_JDG)//apache is sending you his id
			{
				apache_id = ScriptEngine::Get_ID ( sender );
			}

			else if (param == M01_START_ACTING_JDG)
			{
				commandClearance = true;
			}
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (commandClearance == true)
			{
				GameObject * apache = ScriptEngine::Find_Object ( apache_id );
				if (apache != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, apache, 0, M01_MODIFY_YOUR_ACTION_08_JDG, 0 );
				}

				GameObject * zone01 = ScriptEngine::Find_Object ( 105011 );
				GameObject * zone02 = ScriptEngine::Find_Object ( 105012 );
				GameObject * zone03 = ScriptEngine::Find_Object ( 105081 );
				GameObject * zone04 = ScriptEngine::Find_Object ( 105082 );
				GameObject * zone05 = ScriptEngine::Find_Object ( 105083 );
				GameObject * zone06 = ScriptEngine::Find_Object ( 105084 );

				if (zone01 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone01 );
				}

				if (zone02 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone02 );
				}

				if (zone03 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone03 );
				}

				if (zone04 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone04 );
				}

				if (zone05 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone05 );
				}

				if (zone06 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone06 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_ApacheRoom_Second_EntryZone_BottomFloor_JDG, "")//105083 105084
{
	int apache_id;
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( apache_id, 1 );
		SAVE_VARIABLE( commandClearance, 2 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		apache_id = 0;
		commandClearance = false;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_CHOPPER_IS_SENDING_ID_JDG)//apache is sending you his id
			{
				apache_id = ScriptEngine::Get_ID ( sender );
			}

			else if (param == M01_START_ACTING_JDG)
			{
				commandClearance = true;
			}
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (commandClearance == true)
			{
				GameObject * apache = ScriptEngine::Find_Object ( apache_id );
				if (apache != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, apache, 0, M01_MODIFY_YOUR_ACTION_09_JDG, 0 );
				}

				GameObject * zone01 = ScriptEngine::Find_Object ( 105011 );
				GameObject * zone02 = ScriptEngine::Find_Object ( 105012 );
				GameObject * zone03 = ScriptEngine::Find_Object ( 105081 );
				GameObject * zone04 = ScriptEngine::Find_Object ( 105082 );
				GameObject * zone05 = ScriptEngine::Find_Object ( 105083 );
				GameObject * zone06 = ScriptEngine::Find_Object ( 105084 );

				if (zone01 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone01 );
				}

				if (zone02 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone02 );
				}

				if (zone03 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone03 );
				}

				if (zone04 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone04 );
				}

				if (zone05 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone05 );
				}

				if (zone06 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone06 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_Apache_GoBackToHangar_EntryZone_JDG, "")//105085
{
	int apache_id;
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( apache_id, 1 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		apache_id = 0;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_CHOPPER_IS_SENDING_ID_JDG)//apache is sending you his id
			{
				apache_id = ScriptEngine::Get_ID ( sender );
			}
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * apache = ScriptEngine::Find_Object ( apache_id );
			if (apache != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, apache, 0, M01_MODIFY_YOUR_ACTION_06_JDG, 0 );//tell helicopter to go wait above hangar
			}
		}
	}
};

DECLARE_SCRIPT(M04_ApacheRoom_Apache_JDG, "")
{
	bool objective_active;
	bool fighting_rocket_turrets;
	bool waitingInAmbush;
	bool fightingInHangar;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(objective_active, 1);
		SAVE_VARIABLE(fighting_rocket_turrets, 2);
		SAVE_VARIABLE(waitingInAmbush, 3);
		SAVE_VARIABLE(fightingInHangar, 4);
	}

	void Created( GameObject * obj ) override
	{
		fightingInHangar = false;
		waitingInAmbush = false;
		fighting_rocket_turrets = false;
		ScriptEngine::Enable_Hibernation( obj, false );
		objective_active = false;
		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 2 );

		GameObject * zone01 = ScriptEngine::Find_Object ( 105011 );
		GameObject * zone02 = ScriptEngine::Find_Object ( 105012 );
		GameObject * zone03 = ScriptEngine::Find_Object ( 105081 );
		GameObject * zone04 = ScriptEngine::Find_Object ( 105082 );
		GameObject * zone05 = ScriptEngine::Find_Object ( 105083 );
		GameObject * zone06 = ScriptEngine::Find_Object ( 105084 );
		GameObject * zone07 = ScriptEngine::Find_Object ( 105085 );

		if (zone01 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone01, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}

		if (zone02 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone02, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}

		if (zone03 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone03, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}

		if (zone04 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone04, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}

		if (zone05 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone05, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}

		if (zone06 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone06, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}

		if (zone07 != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, zone07, 0, M01_CHOPPER_IS_SENDING_ID_JDG, 0 );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 spawnSpot_01;
		Vector3 spawnSpot_02;
		Vector3 spawnSpot_03;
		Vector3 spawnSpot_04;
		Vector3 spawnSpot_05;
		Vector3 spawnSpot_06;

		if (objective_active == true)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 560, 0 );
		}

		if (fighting_rocket_turrets == true)
		{
			spawnSpot_01.Set (8.677f, -50.970f, 2.954f);
			spawnSpot_02.Set (4.673f, -51.027f, 2.954f);
			spawnSpot_03.Set (1.000f, -51.069f, 3.004f);
			//spawnSpot_04.Set (-2.019f, -51.078f, 3.104f);
			spawnSpot_05.Set (-4.850f, -51.075f, 3.104f);
			spawnSpot_06.Set (-7.130f, -51.057f, 3.204f);
		}

		else
		{
			spawnSpot_01.Set (4.535f, 92.136f, 3.001f);
			spawnSpot_02.Set (1.191f, 92.178f, 3.001f);
			spawnSpot_03.Set (-1.902f, 92.156f, 3.001f);
			//spawnSpot_04.Set (3.983f, 94.662f, 3.001f);
			spawnSpot_05.Set (1.366f, 94.766f, 3.001f);
			spawnSpot_06.Set (-1.817f, 94.600f, 3.001f);
		}

		ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", spawnSpot_01 );
		ScriptEngine::Create_Object ( "POW_GrenadeLauncher_Player", spawnSpot_02 );
		ScriptEngine::Create_Object ( "POW_ChemSprayer_Player", spawnSpot_03 );
		//ScriptEngine::Create_Object ( "POW_Shotgun_Player", spawnSpot_04 );
		ScriptEngine::Create_Object ( "POW_AutoRifle_Player", spawnSpot_05 );
		ScriptEngine::Create_Object ( "POW_Chaingun_Player", spawnSpot_06 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_MODIFY_YOUR_ACTION_JDG)//recieved custom from controller--take off and attack player
		{
			objective_active = true;
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 460, 0 );
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
			params.Set_Movement(Vector3 (0,0,0), 1.0f, 0.25f);
			params.WaypathID = 101264;
			params.WaypointStartID = 101265;
			params.WaypointEndID = 101266;
			ScriptEngine::Action_Goto(obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//now please leave the hanger
		{
			ScriptEngine::Disable_Physical_Collisions ( obj );
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_02_JDG);
			params.Set_Movement(Vector3 (0,0,0), 1.0f, 0.25f);
			params.WaypathSplined = false;
			params.WaypathID = 200611;
			params.WaypointStartID = 200612;
			params.WaypointEndID = 101324;
			ScriptEngine::Action_Goto(obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)//now please cirlce the ship
		{
			ScriptEngine::Enable_Collisions ( obj );
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_03_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			params.Set_Movement(Vector3 (0,0,0), 1.0f, 0.25f);
			params.WaypathID = 101267;
			params.WaypointStartID = 101268;
			ScriptEngine::Modify_Action ( obj, M01_WALKING_WAYPATH_03_JDG, params, false, true );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//player is in tailun 02--go fight him
		{
			fighting_rocket_turrets = true;
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_03_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			params.Set_Movement(Vector3 (-50.839f, -58.626f, 6.053f), 1.0f, 5);

			ScriptEngine::Action_Attack (obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_05_JDG)//player is in tailun 01--go fight him
		{
			fighting_rocket_turrets = true;
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_03_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			params.Set_Movement(Vector3 (50.153f, -59.665f, 1.7f), 1.0f, 5);

			ScriptEngine::Action_Attack (obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_06_JDG)//player is in blackhand area--go wait at hangar
		{
			ScriptEngine::Debug_Message ( "***************************apache has received custom to go back to hangar\n" );
			waitingInAmbush = true;
			ScriptEngine::Action_Reset ( obj, 100 );
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_04_JDG);//going to above hangar
			params.Set_Movement(Vector3 (2.310f, 50.897f, 56.500f), 1.0f, 5);
			params.MovePathfind = false;
			ScriptEngine::Action_Goto (obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_07_JDG && waitingInAmbush == true && fightingInHangar == false)//player is on top floor hangar--spring the trap
		{
			ScriptEngine::Debug_Message ( "***************************apache has received custom that player is on top floor hangar\n" );
			waitingInAmbush = false;
			fightingInHangar = true;
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_05_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			params.MovePathfind = false;
			params.Set_Movement(Vector3 (1.971f, 49.491f, 1.618f), 1.0f, 5);

			ScriptEngine::Action_Attack (obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_08_JDG && waitingInAmbush == true && fightingInHangar == false)//player is on middle floor hangar--spring the trap
		{
			ScriptEngine::Debug_Message ( "***************************apache has received custom that player is on middle floor hangar\n" );
			waitingInAmbush = false;
			fightingInHangar = true;
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_05_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			params.MovePathfind = false;
			params.Set_Movement(Vector3 (1.971f, 49.491f, -7.482f), 1.0f, 5);

			ScriptEngine::Action_Attack (obj, params);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_09_JDG && waitingInAmbush == true && fightingInHangar == false)//player is on bottom floor hangar--spring the trap
		{
			ScriptEngine::Debug_Message ( "***************************apache has received custom that player is on bottom floor hangar\n" );
			waitingInAmbush = false;
			fightingInHangar = true;
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_05_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			params.MovePathfind = false;
			params.Set_Movement(Vector3 (1.971f, 49.491f, -11.432f), 1.0f, 5);

			ScriptEngine::Action_Attack (obj, params);
		}
	}

	void Action_Complete (GameObject *obj, int action_id, ActionCompleteReason complete_reason) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic(this, 100, M01_START_ATTACKING_01_JDG);
			params.Set_Attack( STAR, 1000, 0, true );
			ScriptEngine::Action_Attack(obj, params);
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 3 );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic(this, 100, M01_WALKING_WAYPATH_03_JDG);
			params.Set_Movement(Vector3 (0,0,0), 1.0f, 0.25f);
			params.WaypathID = 101267;
			params.WaypointStartID = 101268;
			params.AttackActive = false;
			ScriptEngine::Action_Attack(obj, params);
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0 );
		}

		else if (action_id == M01_WALKING_WAYPATH_04_JDG && complete_reason == ACTION_COMPLETE_NORMAL)
		{ //you are now waiting above the hangar
			ScriptEngine::Debug_Message ( "***************************apache should now be waiting over hangar\n" );

			GameObject * zone01 = ScriptEngine::Find_Object ( 105011 );//these are the enter zones for going back through the apache hangar
			GameObject * zone02 = ScriptEngine::Find_Object ( 105012 );
			GameObject * zone03 = ScriptEngine::Find_Object ( 105081 );
			GameObject * zone04 = ScriptEngine::Find_Object ( 105082 );
			GameObject * zone05 = ScriptEngine::Find_Object ( 105083 );
			GameObject * zone06 = ScriptEngine::Find_Object ( 105084 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, zone01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, zone02, 0, M01_START_ACTING_JDG, 0 );
			}

			if (zone03 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, zone03, 0, M01_START_ACTING_JDG, 0 );
			}

			if (zone04 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, zone04, 0, M01_START_ACTING_JDG, 0 );
			}

			if (zone05 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, zone05, 0, M01_START_ACTING_JDG, 0 );
			}

			if (zone06 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, zone06, 0, M01_START_ACTING_JDG, 0 );
			}
		}
	}
};






DECLARE_SCRIPT(M04_CargoHold_Stationary_Officer_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}
};

DECLARE_SCRIPT(M04_Firefight_Prisoner, "")
{
	float last_health;
	bool rallying;
	bool pokable;
	bool warningPlayed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(last_health, 1);
		SAVE_VARIABLE(rallying, 2);
		SAVE_VARIABLE(pokable, 3);
		SAVE_VARIABLE(warningPlayed, 3);
	}

	void Created( GameObject * obj ) override
	{
		warningPlayed = false;
		pokable = false;
		rallying = false;
		Vector3 home_spot = ScriptEngine::Get_Position ( obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, home_spot, 2.0f );
		last_health = ScriptEngine::Get_Health ( obj );
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		float myCurrentHealth = ScriptEngine::Get_Health ( obj );
		float myMaxHealth = ScriptEngine::Get_Max_Health ( obj );

		if (((myCurrentHealth/myMaxHealth) * 100 ) <= 50)
		{
			if (warningPlayed == false)
			{
				ScriptEngine::Set_HUD_Help_Text ( IDS_M04DSGN_DSGN0204I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
				warningPlayed = true;
				int medlabConversation = ScriptEngine::Create_Conversation( "M04_POW_Health_Warning_Conversation", 100, 50, false);
				ScriptEngine::Join_Conversation( nullptr, medlabConversation, false, true, true );
				//ScriptEngine::Join_Conversation( medTech, medlabConversation, false, true );
				ScriptEngine::Start_Conversation( medlabConversation, medlabConversation );
			}
		}

		if (damager == STAR)
		{
			ScriptEngine::Set_Health ( obj, last_health );
		}

		else if (obj)
		{
			last_health = ScriptEngine::Get_Health ( obj );
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (rallying == false && pokable == true)
		{
			ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
			int medlabConversation = ScriptEngine::Create_Conversation( "M04_Firefight_POW_Poke_Conversation", 100, 50, false);
			ScriptEngine::Join_Conversation( STAR, medlabConversation, false, true, true );
			//ScriptEngine::Join_Conversation( medTech, medlabConversation, false, true );
			ScriptEngine::Start_Conversation( medlabConversation, medlabConversation );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, 667, 2 );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Set_HUD_Help_Text ( IDS_M04DSGN_DSGN0205I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_END_FIREFIGHT_CONTROLLER_JDG), 0, 10, 0 );//tell controller you've been killed
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 1)//you are prisoner #3--you are now pokable
			{
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
				pokable = true;
			}

			else if (param == 666)//everybody rally in front of the sub
			{
				rallying = true;
				ActionParamsStruct params;
				Vector3 rally_spot (-8.57f,-0.72f,-9.00f);
				params.Set_Basic(this, 50, 100);
				params.Set_Movement(rally_spot, 0.75f, 1.0f);
				ScriptEngine::Action_Goto (obj, params);
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, rally_spot, 1.0f );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, 666, 1 );
			}

			else if (param == 667)//everybody rally in front of the sub
			{
				rallying = true;
				ActionParamsStruct params;
				Vector3 rally_spot (-8.57f,-0.72f,-9.00f);
				params.Set_Basic(this, 100, 100);
				params.Set_Movement(rally_spot, RUN, 1.0f);
				ScriptEngine::Action_Goto (obj, params);
				ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, rally_spot, 1.0f );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, 666, 1 );
			}
		}
	}
};


DECLARE_SCRIPT(M04_Firefight_Controller_JDG, "")//this guys ID number is M04_END_FIREFIGHT_CONTROLLER_JDG 100948
{
	int spawned_nod_guys;
	int prisonerOne_ID;
	int prisonerTwo_ID;
	int prisonerThree_ID;
	int missionIntroConv;

	bool prisoners_rescued;
	bool missionIntroConv_playing;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(spawned_nod_guys, 1);
		SAVE_VARIABLE(prisonerOne_ID, 2);
		SAVE_VARIABLE(prisonerTwo_ID, 3);
		SAVE_VARIABLE(prisonerThree_ID, 4);
		SAVE_VARIABLE(prisoners_rescued, 5);
		SAVE_VARIABLE(missionIntroConv, 6);
		SAVE_VARIABLE(missionIntroConv_playing, 7);
	}

	void Created( GameObject * /* obj */ ) override
	{
		spawned_nod_guys = 0;
		prisoners_rescued = false;
		missionIntroConv_playing = false;
	}

	void Action_Complete( GameObject * /* obj */, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED: //
				{
					if (action_id == missionIntroConv) //rescue prisoners here
					{
						missionIntroConv_playing = false;
					}
				}
				break;
			default:
				break;
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == 1)//player has freed prisoners--give clearance for end firefight
		{
			prisoners_rescued = true;
		}

		else if (param == 10)//one of the prisoners has been killed...give lose message
		{
			ScriptEngine::Create_Sound ( "00-n100e", ScriptEngine::Get_Position ( obj ), obj );//your mission is a failure
			ScriptEngine::Send_Custom_Event( obj, obj, 0, 11, 3 );//send custom to fail mission
		}

		else if (param == 11)//a prisoner has been killed, fail the mission
		{
			ScriptEngine::Mission_Complete ( false );
		}

		else if (param == 20)//tell POWs to gather at sub
		{
			GameObject *prisonerOne = ScriptEngine::Find_Object (prisonerOne_ID);
			if (prisonerOne != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, prisonerOne, 0, 666, 0 );
			}

			GameObject *prisonerTwo = ScriptEngine::Find_Object (prisonerTwo_ID);
			if (prisonerTwo != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, prisonerTwo, 0, 666, 0 );
			}

			GameObject *prisonerThree = ScriptEngine::Find_Object (prisonerThree_ID);
			if (prisonerThree != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, prisonerThree, 0, 666, 0 );//
			}

			int medlabConversation = ScriptEngine::Create_Conversation( "M04_Rally_Round_The_Sub_Boy_Conversation", 100, 50, false);
			ScriptEngine::Join_Conversation( nullptr, medlabConversation, false, true, true );
			//ScriptEngine::Join_Conversation( medTech, medlabConversation, false, true );
			ScriptEngine::Start_Conversation( medlabConversation, medlabConversation );
		}

		else if (param == 100 )//player has picked up the level 3 key card...place actors and give alert
		{
			if (prisoners_rescued == true)
			{
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 470, 0 );

				GameObject * zone01 = ScriptEngine::Find_Object (101126);//stays
				if (zone01 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone01 );
				}

				GameObject * zone04 = ScriptEngine::Find_Object (101129);//stays
				if (zone04 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone04 );
				}

				GameObject * zone06 = ScriptEngine::Find_Object (101131);//stays
				if (zone06 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone06 );
				}

				GameObject * zone10 = ScriptEngine::Find_Object (101135);//stays
				if (zone10 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone10 );
				}

				GameObject * zone12 = ScriptEngine::Find_Object (101137);//stays
				if (zone12 != nullptr)
				{
					ScriptEngine::Destroy_Object ( zone12 );
				}

				int currentDifficulty = ScriptEngine::Get_Difficulty_Level( );
				int medium = 1;
				int hard = 2;
				float delayTimer;

				if (currentDifficulty == hard)
				{
					delayTimer = 90;

					//put down the 3 prisoners in their siege spots
					GameObject *prisonerOne = ScriptEngine::Create_Object ( "GDI_Prisoner_v0b", Vector3(-8.58f, 0.06f,-9.0f));
					ScriptEngine::Attach_Script(prisonerOne, "M04_Firefight_Prisoner", "");

					GameObject *prisonerTwo = ScriptEngine::Create_Object ( "GDI_Prisoner_v1b", Vector3(-14.70f, -9.84f,-9.0f));
					ScriptEngine::Attach_Script(prisonerTwo, "M04_Firefight_Prisoner", "");

					GameObject *prisonerThree = ScriptEngine::Create_Object ( "GDI_Prisoner_v2b", Vector3(8.99f, -1.80f,-9.0f));
					ScriptEngine::Attach_Script(prisonerThree, "M04_Firefight_Prisoner", "");
					ScriptEngine::Send_Custom_Event( obj, prisonerThree, 0, 1, 0 );//tell this guy he is pokable

					prisonerOne_ID = ScriptEngine::Get_ID ( prisonerOne );
					prisonerTwo_ID = ScriptEngine::Get_ID ( prisonerTwo );
					prisonerThree_ID = ScriptEngine::Get_ID ( prisonerThree );
				}

				else if (currentDifficulty == medium)
				{
					delayTimer = 45;

					//put down the 3 prisoners in their siege spots
					GameObject *prisonerOne = ScriptEngine::Create_Object ( "GDI_Prisoner_v0b", Vector3(-8.58f, 0.06f,-9.0f));
					ScriptEngine::Attach_Script(prisonerOne, "M04_Firefight_Prisoner", "");

					GameObject *prisonerTwo = ScriptEngine::Create_Object ( "GDI_Prisoner_v1b", Vector3(-14.70f, -9.84f,-9.0f));
					ScriptEngine::Attach_Script(prisonerTwo, "M04_Firefight_Prisoner", "");

					GameObject *prisonerThree = ScriptEngine::Create_Object ( "GDI_Prisoner_v2b", Vector3(8.99f, -1.80f,-9.0f));
					ScriptEngine::Attach_Script(prisonerThree, "M04_Firefight_Prisoner", "");
					ScriptEngine::Send_Custom_Event( obj, prisonerThree, 0, 1, 0 );//tell this guy he is pokable

					prisonerOne_ID = ScriptEngine::Get_ID ( prisonerOne );
					prisonerTwo_ID = ScriptEngine::Get_ID ( prisonerTwo );
					prisonerThree_ID = ScriptEngine::Get_ID ( prisonerThree );
				}

				else
				{
					delayTimer = 5;

					//put down the 3 prisoners in their siege spots
					GameObject *prisonerOne = ScriptEngine::Create_Object ( "GDI_Prisoner_v0b", Vector3(-8.58f, 0.06f,-9.0f));
					ScriptEngine::Attach_Script(prisonerOne, "M04_Firefight_Prisoner", "");

					GameObject *prisonerTwo = ScriptEngine::Create_Object ( "GDI_Prisoner_v1b", Vector3(-14.70f, -9.84f,-9.0f));
					ScriptEngine::Attach_Script(prisonerTwo, "M04_Firefight_Prisoner", "");

					GameObject *prisonerThree = ScriptEngine::Create_Object ( "GDI_Prisoner_v2b", Vector3(-12.061f, -7.909f,-9.0f));
					ScriptEngine::Attach_Script(prisonerThree, "M04_Firefight_Prisoner", "");

					prisonerOne_ID = ScriptEngine::Get_ID ( prisonerOne );
					prisonerTwo_ID = ScriptEngine::Get_ID ( prisonerTwo );
					prisonerThree_ID = ScriptEngine::Get_ID ( prisonerThree );
				}

				ScriptEngine::Send_Custom_Event( obj, obj, 0, 20, delayTimer );//tell prisoners to gather at sub
				ScriptEngine::Send_Custom_Event( obj, obj, 0, 110, 1 );//give verbal alert

				//the following 3 customs tell the end-mission zone what the prisoner's ID numbers are
				GameObject * rallyZone = ScriptEngine::Find_Object (101194);
				if (rallyZone != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, rallyZone, 1, prisonerOne_ID, 0 );
					ScriptEngine::Send_Custom_Event( obj, rallyZone, 2, prisonerTwo_ID, 0 );
					ScriptEngine::Send_Custom_Event( obj, rallyZone, 3, prisonerThree_ID, 0 );
					ScriptEngine::Send_Custom_Event( obj, rallyZone, 0, M01_START_ACTING_JDG, 0 );
				}

				else
				{
					ScriptEngine::Debug_Message ( "***************************firefight controller cannot find rally zone\n" );
				}

				//trigger the first three spawners
				GameObject * nodGuy01 = ScriptEngine::Trigger_Spawner( 100938 );
				ScriptEngine::Attach_Script(nodGuy01, "M04_Firefight_NodGuys", "");
				ScriptEngine::Grant_Key( nodGuy01, 2, true );
				ScriptEngine::Grant_Key( nodGuy01, 3, true );

				GameObject * nodGuy02 = ScriptEngine::Trigger_Spawner( 100952 );
				ScriptEngine::Attach_Script(nodGuy02, "M04_Firefight_NodGuys", "");
				ScriptEngine::Grant_Key( nodGuy02, 2, true );
				ScriptEngine::Grant_Key( nodGuy02, 3, true );

				GameObject * nodGuy03 = ScriptEngine::Trigger_Spawner( 100951 );
				ScriptEngine::Attach_Script(nodGuy03, "M04_Firefight_NodGuys", "");
				ScriptEngine::Grant_Key( nodGuy03, 2, true );
				ScriptEngine::Grant_Key( nodGuy03, 3, true );

				spawned_nod_guys = 3;
			}

			else if (missionIntroConv_playing == false)//remind player to go rescue the prisoners
			{
				missionIntroConv_playing = true;
				missionIntroConv = ScriptEngine::Create_Conversation( "M04_Mission_Start_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( nullptr, missionIntroConv, false, false, true );
				///ScriptEngine::Join_Conversation( STAR, missionIntroConv, false, false );
				ScriptEngine::Start_Conversation( missionIntroConv,  missionIntroConv );
				ScriptEngine::Monitor_Conversation (obj, missionIntroConv);
			}
		}

		else if (param == 110)//announce that our ally is under attack
		{
			//ScriptEngine::Create_Sound ( "00-d404e", ScriptEngine::Get_Position ( obj ), obj );//updating radar flags..temp placeholder
		}

		else if (param == 666)//a nod attacker has been killed
		{
			float doko_ni_reinforceka = ScriptEngine::Get_Random ( 0.5f, 3.5f );

			if ((doko_ni_reinforceka >= 0.5) && (doko_ni_reinforceka < 1.5) && (spawned_nod_guys < 15))
			{
				GameObject * nodGuy01 = ScriptEngine::Trigger_Spawner( 100938 );
				ScriptEngine::Attach_Script(nodGuy01, "M04_Firefight_NodGuys", "");
				ScriptEngine::Grant_Key( nodGuy01, 2, true );
				ScriptEngine::Grant_Key( nodGuy01, 3, true );
			}

			else if ((doko_ni_reinforceka >= 1.5) && (doko_ni_reinforceka < 2.5) && (spawned_nod_guys < 15))
			{
				GameObject * nodGuy02 = ScriptEngine::Trigger_Spawner( 100952 );
				ScriptEngine::Attach_Script(nodGuy02, "M04_Firefight_NodGuys", "");
				ScriptEngine::Grant_Key( nodGuy02, 2, true );
				ScriptEngine::Grant_Key( nodGuy02, 3, true );
			}

			else if ((doko_ni_reinforceka >= 2.5) && (doko_ni_reinforceka < 3.5) && (spawned_nod_guys < 15))
			{
				GameObject * nodGuy03 = ScriptEngine::Trigger_Spawner( 100951 );
				ScriptEngine::Attach_Script(nodGuy03, "M04_Firefight_NodGuys", "");
				ScriptEngine::Grant_Key( nodGuy03, 2, true );
				ScriptEngine::Grant_Key( nodGuy03, 3, true );
			}

			/*if (spawned_nod_guys == 8)
			{
				GameObject *prisonerOne = ScriptEngine::Find_Object (prisonerOne_ID);
				if (prisonerOne != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, prisonerOne, 0, 666, 0 );
				}

				GameObject *prisonerTwo = ScriptEngine::Find_Object (prisonerTwo_ID);
				if (prisonerTwo != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, prisonerTwo, 0, 666, 0 );
				}

				GameObject *prisonerThree = ScriptEngine::Find_Object (prisonerThree_ID);
				if (prisonerThree != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, prisonerThree, 0, 666, 0 );
				}

				ScriptEngine::Create_Sound ( "00-d406e", ScriptEngine::Get_Position ( obj ), obj );
			}*/

			spawned_nod_guys++;
		}
	}
};

DECLARE_SCRIPT(M04_Firefight_RallyZone, "")//this guys ID number is 101194
{
	bool ok_to_end;
	int prisoner01_ID;
	int prisoner02_ID;
	int prisoner03_ID;
	bool prisoner01_at_rallypoint;
	bool prisoner02_at_rallypoint;
	bool prisoner03_at_rallypoint;
	bool player_at_rallypoint;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(ok_to_end, 1);
		SAVE_VARIABLE(prisoner01_ID, 2);
		SAVE_VARIABLE(prisoner02_ID, 3);
		SAVE_VARIABLE(prisoner03_ID, 4);
		SAVE_VARIABLE(prisoner01_at_rallypoint, 5);
		SAVE_VARIABLE(prisoner02_at_rallypoint, 6);
		SAVE_VARIABLE(prisoner03_at_rallypoint, 7);
		SAVE_VARIABLE(player_at_rallypoint, 8);
	}

	void Created( GameObject * /* obj */ ) override
	{
		ok_to_end = false;

		player_at_rallypoint = false;

		int currentDifficulty = ScriptEngine::Get_Difficulty_Level( );
		int easy = 0;

		if (currentDifficulty == easy)
		{
			prisoner01_at_rallypoint = true;
			prisoner02_at_rallypoint = true;
			prisoner03_at_rallypoint = true;
		}

		else
		{
			prisoner01_at_rallypoint = true;
			prisoner02_at_rallypoint = true;
			prisoner03_at_rallypoint = false;
		}
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 1)
		{
			prisoner01_ID = param;
		}

		else if (type == 2)
		{
			prisoner02_ID = param;
		}

		else if (type == 3)
		{
			prisoner03_ID = param;
		}

		else if (param == M01_START_ACTING_JDG)
		{
			ok_to_end = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject *prisoner01 = ScriptEngine::Find_Object ( prisoner01_ID );
		GameObject *prisoner02 = ScriptEngine::Find_Object ( prisoner02_ID );
		GameObject *prisoner03 = ScriptEngine::Find_Object ( prisoner03_ID );

		if (enterer == STAR)
		{
			//if (ok_to_end == true)
			//{
				player_at_rallypoint = true;
			//}

			//else
			//{
			//	ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, M01_DO_END_MISSION_CHECK_JDG, 0 );
			//}
		}

		else if (enterer == prisoner01)
		{
			prisoner01_at_rallypoint = true;
		}

		else if (enterer == prisoner02)
		{
			prisoner02_at_rallypoint = true;
		}

		else if (enterer == prisoner03)
		{
			prisoner03_at_rallypoint = true;
		}

		if ((player_at_rallypoint == true) && (prisoner01_at_rallypoint == true) && (prisoner02_at_rallypoint == true) && (prisoner03_at_rallypoint == true))
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, M01_DO_END_MISSION_CHECK_JDG, 0 );
		}
	}

	void Exited( GameObject * /* obj */, GameObject * exiter ) override
	{
		GameObject *prisoner01 = ScriptEngine::Find_Object ( prisoner01_ID );
		GameObject *prisoner02 = ScriptEngine::Find_Object ( prisoner01_ID );
		GameObject *prisoner03 = ScriptEngine::Find_Object ( prisoner01_ID );

		if (exiter == STAR)
		{
			player_at_rallypoint = false;
		}

		else if ( exiter == prisoner01)
		{
			prisoner01_at_rallypoint = false;
		}

		else if ( exiter == prisoner02)
		{
			prisoner02_at_rallypoint = false;
		}

		else if ( exiter == prisoner03)
		{
			prisoner03_at_rallypoint = false;
		}
	}
};

DECLARE_SCRIPT(M04_Mutant_UpdateDisc_JDG, "")
{
	void Custom( GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Reveal_Encyclopedia_Character	( 27 );
			ScriptEngine::Set_HUD_Help_Text ( IDS_M00EVAG_DSGN0104I1EVAG_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY  );
		}
	}
};

DECLARE_SCRIPT(M04_TorpedoRoom_Target01_JDG, "")//DSP_L4TORPEDO
{
	bool poked_yet;
	Vector3 myPosition;
	int animationCount;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked_yet, 1 );
		SAVE_VARIABLE( myPosition, 2 );
		SAVE_VARIABLE( animationCount, 3 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		poked_yet = false;
		animationCount = 0;
		myPosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4TORPEDO.DSP_L4TORPEDO", 0 );
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "DSP_L4TORPEDO.DSP_L4TORPEDO") == 0)
		{
			animationCount++;
			if (animationCount >= 2)
			{
				ScriptEngine::Create_Sound ("SFX.Electric_Fire_02", myPosition, obj );
			}

			else
			{
				ScriptEngine::Set_Animation ( obj, "DSP_L4TORPEDO.DSP_L4TORPEDO", false, nullptr, 0.0f, -1.0f, false);

			}
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (poked_yet == false)
		{
			if (STAR != nullptr)
			{
				ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				poked_yet = true;

				ScriptEngine::Set_Animation ( obj, "DSP_L4TORPEDO.DSP_L4TORPEDO", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 200, 0 );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );

				GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
				if (pogController  != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 400, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_TorpedoRoom_Target02_JDG, "")//DSP_L4TORPEDOR
{
	bool poked_yet;
	Vector3 myPosition;
	int animationCount;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked_yet, 1 );
		SAVE_VARIABLE( myPosition, 2 );
		SAVE_VARIABLE( animationCount, 3 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		poked_yet = false;
		animationCount = 0;
		myPosition = ScriptEngine::Get_Position ( obj );

		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4TORPEDOR.DSP_L4TORPEDOR", 0 );
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "DSP_L4TORPEDOR.DSP_L4TORPEDOR") == 0)
		{
			animationCount++;
			if (animationCount >= 2)
			{
				ScriptEngine::Create_Sound ("SFX.Electric_Fire_02", myPosition, obj );
			}

			else
			{
				ScriptEngine::Set_Animation ( obj, "DSP_L4TORPEDOR.DSP_L4TORPEDOR", false, nullptr, 0.0f, -1.0f, false);
			}
		}
	}

	void Poked( GameObject * obj, GameObject * /* poker */ ) override
	{
		if (poked_yet == false)
		{
			if (STAR != nullptr)
			{
				ScriptEngine::Create_Sound ("SFX.L04_Sabotage", myPosition, obj );
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				poked_yet = true;

				ScriptEngine::Set_Animation ( obj, "DSP_L4TORPEDOR.DSP_L4TORPEDOR", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 210, 0 );

				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );

				GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
				if (pogController  != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, pogController, M01_CHANGE_OBJECTIVE_POG_JDG, 400, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_Pog_Controller_JDG, "")//104693
{
	bool missile_target01_poked;
	bool missile_target02_poked;
	bool missile_target03_poked;
	bool missile_target04_poked;

	bool missile_target01_active;
	bool missile_target02_active;
	bool missile_target03_active;
	bool missile_target04_active;

	bool engineroom_target01_active;
	bool engineroom_target02_active;
	bool engineroom_target03_active;
	bool engineroom_target04_active;

	bool torpedo_target01_active;
	bool torpedo_target02_active;
	bool torpedo_target01_poked;
	bool torpedo_target02_poked;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( missile_target01_poked, 1 );
		SAVE_VARIABLE( missile_target02_poked, 2 );
		SAVE_VARIABLE( missile_target03_poked, 3 );
		SAVE_VARIABLE( missile_target04_poked, 4 );
		SAVE_VARIABLE( missile_target01_active, 5 );
		SAVE_VARIABLE( missile_target02_active, 6 );
		SAVE_VARIABLE( missile_target03_active, 7 );
		SAVE_VARIABLE( missile_target04_active, 8 );
		SAVE_VARIABLE( engineroom_target01_active, 9 );
		SAVE_VARIABLE( engineroom_target02_active, 10 );
		SAVE_VARIABLE( engineroom_target03_active, 11 );
		SAVE_VARIABLE( engineroom_target04_active, 12 );
		SAVE_VARIABLE( torpedo_target01_active, 13 );
		SAVE_VARIABLE( torpedo_target02_active, 14 );
		SAVE_VARIABLE( torpedo_target01_poked, 15 );
		SAVE_VARIABLE( torpedo_target02_poked, 16 );

	}

	void Created( GameObject * /* obj */ ) override
	{
		missile_target01_poked = false;
		missile_target02_poked = false;
		missile_target03_poked = false;
		missile_target04_poked = false;

		missile_target01_active = false;
		missile_target02_active = false;
		missile_target03_active = false;
		missile_target04_active = false;

		engineroom_target01_active = false;
		engineroom_target02_active = false;
		engineroom_target03_active = false;
		engineroom_target04_active = false;

		torpedo_target01_active = false;
		torpedo_target02_active = false;
		torpedo_target01_poked = false;
		torpedo_target02_poked = false;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == M01_ADD_OBJECTIVE_POG_JDG)
		{
			if (param == 200)//add first pog for engine room
			{
				GameObject * engineroom_target01 = ScriptEngine::Find_Object ( 100419 );
				GameObject * engineroom_target02 = ScriptEngine::Find_Object ( 100416 );
				GameObject * engineroom_target03 = ScriptEngine::Find_Object ( 100418 );
				GameObject * engineroom_target04 = ScriptEngine::Find_Object ( 100417 );

				if (engineroom_target01 != nullptr)
				{
					engineroom_target01_active = true;
					Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target01 );
					ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
				}

				else if (engineroom_target02 != nullptr)
				{
					engineroom_target02_active = true;
					Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target02 );
					ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
				}

				else if (engineroom_target03 != nullptr)
				{
					engineroom_target03_active = true;
					Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target03 );
					ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
				}

				else if (engineroom_target04 != nullptr)
				{
					engineroom_target04_active = true;
					Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target04 );
					ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
				}
			}

			else if (param == 300)//add first pog for missile room
			{
				if (missile_target01_poked == false)
				{
					GameObject * missile_target01 = ScriptEngine::Find_Object ( 100421 );
					if (missile_target01 != nullptr)
					{
						missile_target01_active = true;
						Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target01 );
						ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
					}
				}

				else if (missile_target02_poked == false)
				{
					GameObject * missile_target02 = ScriptEngine::Find_Object ( 100422 );
					if (missile_target02 != nullptr)
					{
						missile_target02_active = true;
						Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target02 );
						ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
					}
				}

				else if (missile_target03_poked == false)
				{
					GameObject * missile_target03 = ScriptEngine::Find_Object ( 100423 );
					if (missile_target03 != nullptr)
					{
						missile_target03_active = true;
						Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target03 );
						ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
					}
				}

				else if (missile_target04_poked == false)
				{
					GameObject * missile_target04 = ScriptEngine::Find_Object ( 100420 );
					if (missile_target04 != nullptr)
					{
						missile_target04_active = true;
						Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target04 );
						ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
					}
				}
			}

			else if (param == 400)//add first pog for torpedo room
			{
				GameObject * torpedo_target01 = ScriptEngine::Find_Object ( 100410 );
				if (torpedo_target01 != nullptr)
				{
					torpedo_target01_active = true;
					Vector3 pogLocation = ScriptEngine::Get_Position ( torpedo_target01 );
					ScriptEngine::Set_Objective_HUD_Info_Position( 400, 91, "POG_M04_1_04.tga", IDS_POG_SABOTAGE, pogLocation );
				}
			}
		}

		else if (type == M01_CHANGE_OBJECTIVE_POG_JDG)
		{
			if (param == 200)//an engineroom target has been destroyed--change pog over
			{
				GameObject * engineroom_target01 = ScriptEngine::Find_Object ( 100419 );
				GameObject * engineroom_target02 = ScriptEngine::Find_Object ( 100416 );
				GameObject * engineroom_target03 = ScriptEngine::Find_Object ( 100418 );
				GameObject * engineroom_target04 = ScriptEngine::Find_Object ( 100417 );

				if (sender == engineroom_target01)
				{
					if (engineroom_target01_active == true)
					{
						engineroom_target01_active = false;
						ScriptEngine::Set_Objective_HUD_Info( 200, -1, "POG_M04_2_01.tga", IDS_POG_DESTROY );

						if (engineroom_target02 != nullptr)
						{
							engineroom_target02_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target02 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
						}

						else if (engineroom_target03 != nullptr)
						{
							engineroom_target03_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target03 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
						}

						else if (engineroom_target04 != nullptr)
						{
							engineroom_target04_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target04 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
						}
					}
				}

				else if (sender == engineroom_target02)
				{
					if (engineroom_target02_active == true)
					{
						engineroom_target02_active = false;
						ScriptEngine::Set_Objective_HUD_Info( 200, -1, "POG_M04_2_01.tga", IDS_POG_DESTROY );

						if (engineroom_target03 != nullptr)
						{
							engineroom_target03_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target03 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
						}

						else if (engineroom_target04 != nullptr)
						{
							engineroom_target04_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target04 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
						}
					}
				}

				else if (sender == engineroom_target03)
				{
					if (engineroom_target03_active == true)
					{
						engineroom_target03_active = false;
						ScriptEngine::Set_Objective_HUD_Info( 200, -1, "POG_M04_2_01.tga", IDS_POG_DESTROY );

						if (engineroom_target04 != nullptr)
						{
							engineroom_target04_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( engineroom_target04 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 200, 92, "POG_M04_2_01.tga", IDS_POG_DESTROY, pogLocation );
						}
					}
				}

				else if (sender == engineroom_target04)
				{
					if (engineroom_target04_active == true)
					{
						engineroom_target04_active = false;
						ScriptEngine::Set_Objective_HUD_Info( 200, -1, "POG_M04_2_01.tga", IDS_POG_DESTROY );
					}
				}
			}

			else if (param == 300)//missile room target has been poked--see who it is and take action
			{
				GameObject * missile_target01 = ScriptEngine::Find_Object ( 100421 );
				GameObject * missile_target02 = ScriptEngine::Find_Object ( 100422 );
				GameObject * missile_target03 = ScriptEngine::Find_Object ( 100423 );
				GameObject * missile_target04 = ScriptEngine::Find_Object ( 100420 );

				if (sender == missile_target01)
				{
					missile_target01_poked = true;
				}

				else if (sender == missile_target02)
				{
					missile_target02_poked = true;
				}

				else if (sender == missile_target03)
				{
					missile_target03_poked = true;
				}

				else if (sender == missile_target04)
				{
					missile_target04_poked = true;
				}

				if (missile_target01_poked == true && missile_target01_active == true)//done with #1 light up next one
				{
					ScriptEngine::Set_Objective_HUD_Info( 300, -1, "POG_M04_1_03.tga", IDS_POG_SABOTAGE );

					if (missile_target02_poked == false)
					{
						missile_target02 = ScriptEngine::Find_Object ( 100422 );
						if (missile_target02 != nullptr)
						{
							missile_target02_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target02 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
						}
					}

					else if (missile_target03_poked == false)
					{
						missile_target03 = ScriptEngine::Find_Object ( 100423 );
						if (missile_target03 != nullptr)
						{
							missile_target03_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target03 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
						}
					}

					else if (missile_target04_poked == false)
					{
						missile_target04 = ScriptEngine::Find_Object ( 100420 );
						if (missile_target04 != nullptr)
						{
							missile_target04_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target04 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
						}
					}
				}

				else if (missile_target02_poked == true && missile_target02_active == true)//done with #2 light up next one
				{
					ScriptEngine::Set_Objective_HUD_Info( 300, -1, "POG_M04_1_03.tga", IDS_POG_SABOTAGE );

					if (missile_target03_poked == false)
					{
						missile_target03 = ScriptEngine::Find_Object ( 100423 );
						if (missile_target03 != nullptr)
						{
							missile_target03_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target03 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
						}
					}

					else if (missile_target04_poked == false)
					{
						missile_target04 = ScriptEngine::Find_Object ( 100420 );
						if (missile_target04 != nullptr)
						{
							missile_target04_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target04 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
						}
					}
				}

				else if (missile_target03_poked == true && missile_target03_active == true)//done with #3 light up next one
				{
					ScriptEngine::Set_Objective_HUD_Info( 300, -1, "POG_M04_1_03.tga", IDS_POG_SABOTAGE );

					if (missile_target04_poked == false)
					{
						missile_target04 = ScriptEngine::Find_Object ( 100420 );
						if (missile_target04 != nullptr)
						{
							missile_target04_active = true;
							Vector3 pogLocation = ScriptEngine::Get_Position ( missile_target04 );
							ScriptEngine::Set_Objective_HUD_Info_Position( 300, 91, "POG_M04_1_03.tga", IDS_POG_SABOTAGE, pogLocation );
						}
					}
				}
			}

			else if (param == 400)//torpedo target has been poked--see who it is and take action
			{
				GameObject * torpedo_target01 = ScriptEngine::Find_Object ( 100410 );
				GameObject * torpedo_target02 = ScriptEngine::Find_Object ( 100409 );

				if (sender == torpedo_target01)
				{
					torpedo_target01_poked = true;
				}

				if (sender == torpedo_target02)
				{
					torpedo_target02_poked = true;
				}

				if (torpedo_target01_poked == true )
				{
					torpedo_target02 = ScriptEngine::Find_Object ( 100409 );
					if (torpedo_target02 != nullptr && torpedo_target02_poked == false)
					{
						torpedo_target02_active = true;
						Vector3 pogLocation = ScriptEngine::Get_Position ( torpedo_target02 );
						ScriptEngine::Set_Objective_HUD_Info_Position( 400, 91, "POG_M04_1_04.tga", IDS_POG_SABOTAGE, pogLocation );
					}

					else
					{
						Vector3 pogLocation = ScriptEngine::Get_Position ( torpedo_target02 );
						ScriptEngine::Set_Objective_HUD_Info_Position( 400, -1, "POG_M04_1_04.tga", IDS_POG_SABOTAGE, pogLocation );
					}
				}
			}
		}
	}
};

DECLARE_SCRIPT(M04_Start_TorpedoObjective_Zone_JDG, "")//105238 105239
{
	bool conversationPlaying;
	int missionIntroConv;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( conversationPlaying, 1 );
		SAVE_VARIABLE( missionIntroConv, 2 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		conversationPlaying = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if ( enterer == STAR && conversationPlaying == false)//M04_Add_Torpedo_Objective_Conversation
		{
			conversationPlaying = true;
			missionIntroConv = ScriptEngine::Create_Conversation( "M04_Add_Torpedo_Objective_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( nullptr, missionIntroConv, false, false, true );
			ScriptEngine::Start_Conversation( missionIntroConv,  missionIntroConv );
			ScriptEngine::Monitor_Conversation (obj, missionIntroConv);
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == missionIntroConv)//conversation is over, now add objective and pogs
					{
						ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M04_OBJECTIVE_CONTROLLER_JDG), 0, 450, 0 );

						GameObject * pogController = ScriptEngine::Find_Object ( 104693 );
						if (pogController  != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, pogController, M01_ADD_OBJECTIVE_POG_JDG, 400, 0 );
						}

						GameObject * zone01 = ScriptEngine::Find_Object ( 105238 );
						if (zone01 != nullptr)
						{
							ScriptEngine::Destroy_Object ( zone01 );
						}

						GameObject * zone02 = ScriptEngine::Find_Object ( 105239 );
						if (zone02 != nullptr)
						{
							ScriptEngine::Destroy_Object ( zone02 );
						}
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M04_TorpedoRoom_EnterZone_JDG, "")// 105240
{
	void Entered( GameObject * /* obj */, GameObject * enterer ) override
	{
		if ( enterer == STAR )
		{
			int missionIntroConv = ScriptEngine::Create_Conversation( "M04_Poke_The_Torpedos_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( nullptr, missionIntroConv, false, false, true );
			ScriptEngine::Start_Conversation( missionIntroConv,  missionIntroConv );

			GameObject *zone02 = ScriptEngine::Find_Object (105240);
			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};

DECLARE_SCRIPT(M04_BigSam_EntryZone_JDG, "")//104942
{
	bool missilesSabotaged;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(missilesSabotaged, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		missilesSabotaged = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (missilesSabotaged == true)
			{
				GameObject * bigSam = ScriptEngine::Find_Object ( 104873 );
				if (bigSam != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, bigSam, 0, M01_START_ACTING_JDG, 1 );
					ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
				}
			}
		}
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				missilesSabotaged = true;
			}
		}
	}
};

DECLARE_SCRIPT(M04_BigSam_Script_JDG, "")//104873
{
	int civWarning;
	int missileStuckSound;

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_L4GIANTGUN.DSP_L4GIANTGUN", 0 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
			if (param == civWarning)
			{
				ScriptEngine::Set_Animation ( obj, "DSP_L4GIANTGUN.DSP_L4GIANTGUN", false, nullptr, 0, 119, false );

				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_01_JDG, 10 );
			}

			else if (param == missileStuckSound)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_START_ACTING_JDG );
				params.AttackActive = false;
				ScriptEngine::Action_Attack ( obj, params );

				Vector3 explosion01_spot (0.122f, -57.477f, 13.000f);//
				ScriptEngine::Create_Explosion ("Explosion_SAM_Site", explosion01_spot, nullptr );
				ScriptEngine::Set_Animation ( obj, "DSP_L4GIANTGUN.DSP_L4GIANTGUN", false, nullptr, 120, 200, false );
				ScriptEngine::Set_Health ( obj, 0.25f );
			}
		}

		else if (type == 0)
		{
			if (param == M01_START_ACTING_JDG )//missiles sabotaged/player entered room...start destruction sequence
			{
				civWarning = ScriptEngine::Create_2D_Sound ( "M04DSGN_DSGN0087I1EVAN_SND" );
				ScriptEngine::Monitor_Sound ( obj, civWarning );
			}

			else if (param == M01_START_ATTACKING_01_JDG )
			{
				Vector3 myTarget = ScriptEngine::Get_Position ( obj );
				myTarget.X += ScriptEngine::Get_Random(-30.0f, 30.0f);
				myTarget.Y += ScriptEngine::Get_Random(-30.0f, 30.0f);
				myTarget.Z += ScriptEngine::Get_Random(0, 30.0f);

				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_START_ACTING_JDG );
				params.Set_Attack( myTarget, 1000, 0, true );
				params.AttackCheckBlocked = false;
				ScriptEngine::Action_Attack ( obj, params );

				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_02_JDG, 3 );
			}

			else if (param == M01_START_ATTACKING_02_JDG )
			{
				missileStuckSound = ScriptEngine::Create_2D_Sound ( "Metal_Thunk_03" );
				ScriptEngine::Monitor_Sound ( obj, missileStuckSound );
			}
		}
	}
};

DECLARE_SCRIPT(M04_PlaySound_OnZoneEntry_OneTime_JDG, "SoundName:string")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			const char * sound_name = Get_Parameter("SoundName");

			Vector3 playersPosition = ScriptEngine::Get_Position ( obj );
			Vector3 speaker01_Position = playersPosition;
			Vector3 speaker02_Position = playersPosition;
			Vector3 speaker03_Position = playersPosition;

			speaker01_Position.X += ScriptEngine::Get_Random(-10,10);
			speaker01_Position.Y += ScriptEngine::Get_Random(-10,10);
			speaker01_Position.Z += ScriptEngine::Get_Random(-10,10);

			speaker02_Position.X += ScriptEngine::Get_Random(-10,10);
			speaker02_Position.Y += ScriptEngine::Get_Random(-10,10);
			speaker02_Position.Z += ScriptEngine::Get_Random(-10,10);

			speaker03_Position.X += ScriptEngine::Get_Random(-10,10);
			speaker03_Position.Y += ScriptEngine::Get_Random(-10,10);
			speaker03_Position.Z += ScriptEngine::Get_Random(-10,10);

			ScriptEngine::Create_Sound ( sound_name, speaker01_Position, nullptr );
			ScriptEngine::Create_Sound ( sound_name, speaker02_Position, nullptr );
			ScriptEngine::Create_Sound ( sound_name, speaker03_Position, nullptr );

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M04_Objective_Reminder_Controller_JDG, "")//105760
{
	bool prisoner_primaryActive;
	bool missile_primaryActive;
	bool l01keycard_primaryActive;
	bool l02keycard_primaryActive;
	bool l03keycard_primaryActive;
	bool torpedo_primaryActive;
	bool escape_primaryActive;

	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( prisoner_primaryActive, 1 );
		SAVE_VARIABLE( missile_primaryActive, 2 );
		SAVE_VARIABLE( l01keycard_primaryActive, 3 );
		SAVE_VARIABLE( l02keycard_primaryActive, 4 );
		SAVE_VARIABLE( l03keycard_primaryActive, 5 );
		SAVE_VARIABLE( torpedo_primaryActive, 6 );
		SAVE_VARIABLE( escape_primaryActive, 7 );
		SAVE_VARIABLE( commandClearance, 8 );
	}

	void Created( GameObject * obj ) override
	{
		prisoner_primaryActive = false;
		missile_primaryActive = false;
		l01keycard_primaryActive = false;
		l02keycard_primaryActive = false;
		l03keycard_primaryActive = false;
		torpedo_primaryActive = false;
		escape_primaryActive = false;
		commandClearance = true;

		ScriptEngine::Send_Custom_Event( obj, obj, 1, 1, 90 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == M01_ADD_OBJECTIVE_POG_JDG)
		{
			if (param == 0)//rescue the prisoner objective added
			{
				prisoner_primaryActive = true;
			}

			else if (param == 1)//sabotage the missile rack objective added
			{
				missile_primaryActive = true;
			}

			else if (param == 2)//get the level 01 keycard
			{
				l01keycard_primaryActive = true;
			}

			else if (param == 3)//get the level 02 keycard
			{
				l02keycard_primaryActive = true;
			}

			else if (param == 4)//get the level 03 keycard
			{
				l03keycard_primaryActive = true;
			}

			else if (param == 5)//sabotage the torpedo racks
			{
				torpedo_primaryActive = true;
			}

			else if (param == 6)//escape with the prisoners
			{
				escape_primaryActive = true;
			}

			commandClearance = false;
			ScriptEngine::Send_Custom_Event( obj, obj, 1, 2, 15 );
		}

		else if (type == M01_REMOVE_OBJECTIVE_POG_JDG)
		{
			if (param == 0)//rescue the prisoner objective added
			{
				prisoner_primaryActive = false;
			}

			else if (param == 1)//sabotage the missile rack objective added
			{
				missile_primaryActive = false;
			}

			else if (param == 2)//get the level 01 keycard
			{
				l01keycard_primaryActive = false;
			}

			else if (param == 3)//get the level 02 keycard
			{
				l02keycard_primaryActive = false;
			}

			else if (param == 4)//get the level 03 keycard
			{
				l03keycard_primaryActive = false;
			}

			else if (param == 5)//sabotage the missile racks
			{
				torpedo_primaryActive = false;
			}

			else if (param == 6)//escape with the prisoners
			{
				escape_primaryActive = false;
			}
		}

		else if (type == 1)
		{
			if (param == 1)//give reminder message
			{
				if (missile_primaryActive == true && commandClearance == true)
				{//M04_Add_MissileRoom_Objective_Conversation
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Add_MissileRoom_Objective_Conversation", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				else if (torpedo_primaryActive == true && commandClearance == true)
				{//M04_Add_Torpedo_Objective_Conversation
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Add_Torpedo_Objective_Conversation", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				else if (l01keycard_primaryActive == true && commandClearance == true)
				{//M04_Add_PrisonKey_Objective
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Add_PrisonKey_Objective", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				else if (prisoner_primaryActive == true && commandClearance == true)
				{//M04_Mission_Start_Conversation
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Mission_Start_Conversation", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				else if (l03keycard_primaryActive == true && commandClearance == true)
				{//M04_Add_Captains_Key_Objective_Conversation
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Add_Captains_Key_Objective_Conversation", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				else if (l02keycard_primaryActive == true && commandClearance == true)
				{//M04_Add_FirstMate_Objective_JDG
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Add_FirstMate_Objective_JDG", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				else if (escape_primaryActive == true && commandClearance == true)
				{//M04_Rally_Round_The_Sub_Boy_Conversation
					int reminderConv = ScriptEngine::Create_Conversation( "M04_Rally_Round_The_Sub_Boy_Conversation", 10, 1000, false);
					ScriptEngine::Join_Conversation( nullptr, reminderConv, false, false, true );
					ScriptEngine::Start_Conversation( reminderConv,  reminderConv );
				}

				ScriptEngine::Send_Custom_Event( obj, obj, 1, 1, 90 );
			}

			if (param == 2)//give reminder message
			{
				commandClearance = true;
			}
		}
	}
};






































