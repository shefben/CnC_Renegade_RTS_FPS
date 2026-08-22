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
*     Mission11.cpp
*
* DESCRIPTION
*     Mission 11 scripts
*
* PROGRAMMER
*     Design Team
*
* VERSION INFO
*     $Author: Joe_g $
*     $Revision: 108 $
*     $Modtime: 2/19/02 12:09p $
*     $Archive: /Commando/Code/Scripts/Mission11.cpp $
*
******************************************************************************/

#include "scripts.h"
#include <string.h>
#include "Toolkit.h"
#include "Mission1.h"
#include "Mission11.h"


/***********************************************************************************************************************
Putting level specific character scripts here...sydney, petrova, havoc, etc...
************************************************************************************************************************/
DECLARE_SCRIPT(M11_VoltRifleGuy_Script_JDG, "")
{
	bool player_has_volt_rifle;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( player_has_volt_rifle, 1 );
	}

	void Created( GameObject * obj ) override
	{
		player_has_volt_rifle = false;

		GameObject * voltPowerupMonitor = ScriptEngine::Find_Object ( 101883 );
		if (voltPowerupMonitor != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, voltPowerupMonitor, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );//checking if player has volt rifle
		}
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)//player has volt rifle--allow powerup drop
			{
				player_has_volt_rifle = true;
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		if (player_has_volt_rifle == true)
		{
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			myPosition.Z += 0.75;

			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", myPosition );
		}
	}
};

DECLARE_SCRIPT(M11_VoltRifle_Monitor_Script_JDG, "")
{
	bool player_has_volt_rifle;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( player_has_volt_rifle, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		player_has_volt_rifle = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)//player has picked up volt rifle
			{
				player_has_volt_rifle = true;
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)//volt guy is checking on rifle status
			{
				if (player_has_volt_rifle == true)
				{
					ScriptEngine::Send_Custom_Event ( obj, sender, 0, M01_START_ACTING_JDG, 0 );
				}

				else
				{
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_VoltRifle_Script_JDG, "")
{
	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			Vector3 spawn_location01 (-2.44f, 6.189f, -38.536f);//these are in the barracks
			Vector3 spawn_location02 (-4.079f, 6.207f, -38.536f);
			Vector3 spawn_location03 (-5.857f, 6.199f, -38.536f);

			Vector3 spawn_location04 (-25.948f, 10.923f, -29.670f);//these are in seth's room
			Vector3 spawn_location05 (-24.676f, 13.208f, -29.855f);
			Vector3 spawn_location06 (-22.492f, 14.631f, -29.855f);
			Vector3 spawn_location07 (-19.753f, 14.540f, -29.855f);
			Vector3 spawn_location08 (-17.378f, 13.195f, -29.855f);
			Vector3 spawn_location09 (-17.567f, 11.051f, -29.658f);

			Vector3 spawn_location10 (20.696f, 1.720f, -29.855f);
			Vector3 spawn_location11 (20.648f, -0.227f, -29.855f);


			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location01 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location02 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location03 );

			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location04 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location05 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location06 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location07 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location08 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location09 );

			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location10 );
			ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location11 );

			GameObject * voltPowerupMonitor = ScriptEngine::Find_Object ( 101883 );
			if (voltPowerupMonitor != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, voltPowerupMonitor, 0, M01_START_ACTING_JDG, 0 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_KeyCard_01_Carrier_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Send_Custom_Event ( obj, obj, M00_CUSTOM_POWERUP_GRANT_DISABLE, 0, 0 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 spawn_location = ScriptEngine::Get_Position (obj);
		spawn_location.Z = spawn_location.Z + 0.75f;

		GameObject * powerup = ScriptEngine::Create_Object ( "POW_VoltAutoRifle_Player", spawn_location );
		ScriptEngine::Attach_Script(powerup, "M11_VoltRifle_Script_JDG", "");
	}
};

/***********************************************************************************************************************
Putting MISSION CONTROLLER here
************************************************************************************************************************/

DECLARE_SCRIPT(M11_Mission_Controller_JDG, "")
{
	int evaPingedSydneyConv;
	int missionIntroConv02;
	int missionIntroConv;
	int evaNukeConv;

	bool player_has_reached_musuem;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( evaPingedSydneyConv, 1 );
		SAVE_VARIABLE( missionIntroConv02, 2 );
		SAVE_VARIABLE( missionIntroConv, 3 );
		SAVE_VARIABLE( evaNukeConv, 4 );
		SAVE_VARIABLE( player_has_reached_musuem, 5 );
	}

	void Created( GameObject * obj ) override
	{
		player_has_reached_musuem = false;
		ScriptEngine::Show_Player_Map_Marker ( false );
		ScriptEngine::Reveal_Map ( );
		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M11_ATTACH_HAVOCS_SCRIPT_JDG, 0 );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == missionIntroConv) //introduce rescue sydney objective here
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_ADD_SECOND_OBJECTIVE_JDG, 0 );
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_PLAY_SECOND_INTRO_CONVERSATION_JDG, 3 );
					}

					else if (action_id == missionIntroConv02) //introduce rescue sydney objective here
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_ADD_FIRST_OBJECTIVE_JDG, 0 );
					}

					else if (action_id == evaPingedSydneyConv)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_ADD_SECOND_OBJECTIVE_POG_JDG, 0 );
					}

					else if (action_id == evaNukeConv)
					{
						ScriptEngine::Add_Objective( M11_FIFTH_OBJECTIVE_JDG, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M11_05, nullptr, IDS_Enc_Obj_Primary_M11_05 );

						GameObject * nukeSwitch = ScriptEngine::Find_Object ( M11_END_MISSION_SWITCH_JDG );
						if (nukeSwitch != nullptr)
						{
							ScriptEngine::Set_Objective_Radar_Blip_Object( M11_FIFTH_OBJECTIVE_JDG, nukeSwitch );
							ScriptEngine::Set_Objective_HUD_Info( M11_FIFTH_OBJECTIVE_JDG, 94, "POG_M11_1_04.tga", IDS_POG_SABOTAGE );
						}

						else
						{
							ScriptEngine::Debug_Message ( "***************************M11 mission controller cannot find sydnee to ping her on radar--add forth objective\n" );
						}
					}
				}
				break;
			default:
				break;
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
		}

		else if (type == 0)
		{
			switch (param)
			{
				case M11_ATTACH_HAVOCS_SCRIPT_JDG:
					{
						if (STAR)
						{
							//ScriptEngine::Attach_Script(STAR, "M11_Havocs_Script_JDG", "");
							missionIntroConv = ScriptEngine::Create_Conversation( "M11_Level_Intro_Conversation", 100, 1000, false);
							ScriptEngine::Join_Conversation( nullptr, missionIntroConv, false, false, true );
							ScriptEngine::Join_Conversation( STAR, missionIntroConv, false, false, true );
							//ScriptEngine::Join_Conversation( nullptr, missionIntroConv, false, false, true );
							ScriptEngine::Start_Conversation( missionIntroConv,  missionIntroConv );
							ScriptEngine::Monitor_Conversation (obj, missionIntroConv);
						}

						else
						{
							ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_ATTACH_HAVOCS_SCRIPT_JDG, 1 );
						}
					}
					break;

				case M11_PLAY_SECOND_INTRO_CONVERSATION_JDG: //play locke-havoc-cabal conv
					{
						if (STAR)
						{
							missionIntroConv02 = ScriptEngine::Create_Conversation( "M11_Level_Intro_Conversation02", 100, 1000, false);
							ScriptEngine::Join_Conversation( nullptr, missionIntroConv02, false, false, true );
							ScriptEngine::Join_Conversation( nullptr, missionIntroConv02, false, false, true );
							//ScriptEngine::Join_Conversation( STAR, missionIntroConv02, false, false, true );
							ScriptEngine::Start_Conversation( missionIntroConv02,  missionIntroConv02 );
							ScriptEngine::Monitor_Conversation (obj, missionIntroConv02);

							ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_CONTROLLER_JDG), 0, M01_START_ACTING_JDG, 5 );
						}
					}
					break;

				case M11_END_MISSION_PASS_JDG: //end mission when all objectives have been met
					{
						ScriptEngine::Mission_Complete ( true );
					}
					break;

				case M11_END_MISSION_FAIL_JDG: //end mission when critical objective failed
					{
						ScriptEngine::Mission_Complete ( false );
					}
					break;

				case M11_ADD_FIRST_OBJECTIVE_JDG: //player's first objective is to get into lower levels
					{
						if (player_has_reached_musuem == false)
						{
							ScriptEngine::Add_Objective( M11_FIRST_OBJECTIVE_JDG, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M11_02, nullptr, IDS_Enc_Obj_Primary_M11_02 );
							Vector3 musemLocation (2.072f, 16.705f, -24.531f);
							ScriptEngine::Set_Objective_HUD_Info_Position( M11_FIRST_OBJECTIVE_JDG, 91, "POG_M11_1_02.tga", IDS_POG_INFILTRATE, musemLocation );
						}
					}
					break;

				case M11_END_FIRST_OBJECTIVE_JDG: //player has reached museum area--clear first objective
					{
						player_has_reached_musuem = true;
						ScriptEngine::Set_Objective_Status( M11_FIRST_OBJECTIVE_JDG, OBJECTIVE_STATUS_ACCOMPLISHED );
						//ScriptEngine::Send_Custom_Event ( obj, obj, 0, M11_ADD_SECOND_OBJECTIVE_JDG, 5 );

						evaPingedSydneyConv = ScriptEngine::Create_Conversation( "M11_EVA_SydneyPinged_Conversation", 100, 1000, false);
						ScriptEngine::Join_Conversation( nullptr, evaPingedSydneyConv, false, false, true );;
						ScriptEngine::Start_Conversation( evaPingedSydneyConv,  evaPingedSydneyConv );
						ScriptEngine::Monitor_Conversation (obj, evaPingedSydneyConv);

					}
					break;//

				case M11_ADD_SECOND_OBJECTIVE_JDG: //player's second objective is to locate sydney mobius
					{
						Vector3 sydenysLocation (0.000f, 96.000f, -34.250f);
						//ScriptEngine::Add_Objective( M11_SECOND_OBJECTIVE_JDG, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M11_01 );
						ScriptEngine::Add_Objective( M11_SECOND_OBJECTIVE_JDG, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M11_01, nullptr, IDS_Enc_Obj_Primary_M11_01 );
						ScriptEngine::Set_Objective_HUD_Info_Position( M11_SECOND_OBJECTIVE_JDG, -1, "POG_M11_1_01.tga", IDS_POG_RESCUE, sydenysLocation );

					}
					break;

				case M11_ADD_SECOND_OBJECTIVE_POG_JDG: //player's second objective is to locate sydney mobius
					{
						GameObject * sydney = ScriptEngine::Find_Object ( M11_SIMPLE_SYDNEY_MOBIUS_JDG );
						if (sydney != nullptr)
						{
							Vector3 sydenysLocation (0.000f, 96.000f, -34.250f);
							//ScriptEngine::Set_Objective_Radar_Blip_Object( M11_SECOND_OBJECTIVE_JDG, sydney );
							ScriptEngine::Set_Objective_HUD_Info_Position( M11_SECOND_OBJECTIVE_JDG, 90, "POG_M11_1_01.tga", IDS_POG_RESCUE, sydenysLocation );
						}

						else
						{
							ScriptEngine::Debug_Message ( "***************************M11 mission controller cannot find sydnee to ping her on radar--add second objective\n" );
						}
					}
					break;

				case M11_END_SECOND_OBJECTIVE_JDG: //player has reached Sydney's cell area--clear 2nd objective
					{
						ScriptEngine::Set_Objective_Status( M11_SECOND_OBJECTIVE_JDG, OBJECTIVE_STATUS_ACCOMPLISHED );
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_ADD_FORTH_OBJECTIVE_JDG, 5 );

						//nukeIntroConv = ScriptEngine::Create_Conversation( "M11_EVA_Activate_Nuke_Conversation", 100, 1000, false);
						//ScriptEngine::Join_Conversation( nullptr, nukeIntroConv, false, false, true );
						//ScriptEngine::Start_Conversation( nukeIntroConv,  nukeIntroConv );
						//ScriptEngine::Monitor_Conversation (obj, nukeIntroConv);
					}
					break;

				case M11_ADD_THIRD_OBJECTIVE_JDG: //player's third objective is to acess the power core
					{
						ScriptEngine::Add_Objective( M11_THIRD_OBJECTIVE_JDG, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M11_03, nullptr, IDS_Enc_Obj_Primary_M11_03 );

						GameObject * entryZone = ScriptEngine::Find_Object ( 100688 );
						if (entryZone != nullptr)
						{
							Vector3 pogLocation = ScriptEngine::Get_Position ( entryZone );
							ScriptEngine::Set_Objective_Radar_Blip_Object( M11_THIRD_OBJECTIVE_JDG, entryZone );
							ScriptEngine::Set_Objective_HUD_Info_Position( M11_THIRD_OBJECTIVE_JDG, 91, "POG_M11_1_03.tga", IDS_POG_INFILTRATE, pogLocation );
						}
					}
					break;

				case M11_END_THIRD_OBJECTIVE_JDG: //player has reached power core--clear 3rd objective
					{
						ScriptEngine::Set_Objective_Status( M11_THIRD_OBJECTIVE_JDG, OBJECTIVE_STATUS_ACCOMPLISHED );

						GameObject * entryZone = ScriptEngine::Find_Object ( 100012 );
						if (entryZone != nullptr)
						{
							ScriptEngine::Destroy_Object ( entryZone );//cleaning up this zone
						}
					}
					break;

				case M11_ADD_FORTH_OBJECTIVE_JDG: //player's forth objective is to PROTECT SYDNEY
					{
						ScriptEngine::Add_Objective( M11_FORTH_OBJECTIVE_JDG, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M11_04, nullptr, IDS_Enc_Obj_Primary_M11_04 );

						GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
						if (sydney != nullptr)
						{
							ScriptEngine::Set_Objective_Radar_Blip_Object( M11_FORTH_OBJECTIVE_JDG, sydney );
							ScriptEngine::Set_Objective_HUD_Info( M11_FORTH_OBJECTIVE_JDG, 93, "POG_M07_1_04.tga", IDS_POG_PROTECT );
						}

						else
						{
							ScriptEngine::Debug_Message ( "***************************M11 mission controller cannot find sydnee to ping her on radar--add forth objective\n" );
						}
					}
					break;


				case M11_END_FORTH_OBJECTIVE_JDG: //nuke switch has been poked--end protect sydney objective then end sabotage
					{
						ScriptEngine::Set_Objective_Status( M11_FORTH_OBJECTIVE_JDG, OBJECTIVE_STATUS_ACCOMPLISHED );
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_END_FIFTH_OBJECTIVE_JDG, 3 );
					}
					break;

				case M11_ADD_FIFTH_OBJECTIVE_JDG: //player's fifth objective is to sabotage the nuclear missle
					{
						evaNukeConv = ScriptEngine::Create_Conversation( "M11_EVA_Activate_Nuke_Conversation", 100, 1000, false);
						ScriptEngine::Join_Conversation( nullptr, evaNukeConv, false, false, true );;
						ScriptEngine::Start_Conversation( evaNukeConv,  evaNukeConv );
						ScriptEngine::Monitor_Conversation (obj, evaNukeConv);
					}
					break;

				case M11_END_FIFTH_OBJECTIVE_JDG: //nuke switch has been poked--end protect sydney objective then end sabotage
					{
						ScriptEngine::Set_Objective_Status( M11_FIFTH_OBJECTIVE_JDG, OBJECTIVE_STATUS_ACCOMPLISHED );
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_END_MISSION_PASS_JDG, 3 );
					}
					break;
			}
		}
	}
};

/***********************************************************************************************************************
Putting scripts dealing with level 01 security turrets here
************************************************************************************************************************/

DECLARE_SCRIPT(M11_Floor01_SecurityTurrets_Controller_JDG, "")
{
	int deadTurrets;
	bool powerupsGranted;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( deadTurrets, 1 );
		SAVE_VARIABLE( powerupsGranted, 2 );
	}

	void Created( GameObject * /* obj */ ) override
	{
		powerupsGranted = false;
		deadTurrets = 0;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			//ScriptEngine::Create_Sound ( "M11_Cabal_InsectDetected", Vector3(0,0,0), obj );

			GameObject * turret01 = ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_TURRET_01_JDG);
			GameObject * turret03 = ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_TURRET_03_JDG);
			GameObject * turret05 = ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_TURRET_05_JDG);
			GameObject * turret08 = ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_TURRET_08_JDG);
			GameObject * turret10 = ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_TURRET_10_JDG);
			GameObject * turret12 = ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_TURRET_12_JDG);

			if (turret01 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret01, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret03 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret03, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret05 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret05, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret08 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret08, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret10 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret10, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret12 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret12, 0, M01_START_ACTING_JDG, delayTimer );
			}
		}

		else if (param == M01_YOUR_OPERATOR_IS_DEAD_JDG)//this is custom recieved when a turret gets destroyed
		{
			deadTurrets++;

			if (deadTurrets >= 6)
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
			}
		}

		else if (param == M01_SPAWNER_SPAWN_PLEASE_JDG && powerupsGranted == false)
		{
			powerupsGranted = true;
			Vector3 health100_spawnLocation (0.812f, 17.135f, 0.67f);
			Vector3 armor100_spawnLocation (-0.743f, 17.130f, 0.67f);

			ScriptEngine::Create_Object ( "POW_Health_100", health100_spawnLocation );
			ScriptEngine::Create_Object ( "POW_Armor_100", armor100_spawnLocation );
		}
	}
};

DECLARE_SCRIPT(M11_Floor01_SecurityTurret_JDG, "")
{
	bool active;
	int alarmSound;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( alarmSound, 1 );
		SAVE_VARIABLE( active, 2 );
	}

	void Created( GameObject * obj ) override
	{
		active = false;
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );

		alarmSound = ScriptEngine::Create_Sound ( "SFX.Klaxon_Alert_01", myPosition, obj );
		ScriptEngine::Stop_Sound ( alarmSound, false );

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_GOTO_IDLE_JDG, 1 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		if (active == true)
		{
			ScriptEngine::Stop_Sound ( alarmSound, true );
			Vector3 powerupSpawnLocation;

			Vector3 turret01_spawnLocation (19.065f, 22.042f, 0.67f);
			Vector3 turret03_spawnLocation (19, -3.620f, 0.67f);
			Vector3 turret05_spawnLocation (18.901f, -29.477f, 0.67f);
			Vector3 turret08_spawnLocation (-18.315f, -29.750f, 0.67f);
			Vector3 turret10_spawnLocation (-18.295f, -3.482f, 0.67f);
			Vector3 turret12_spawnLocation (-18.031f, 22.403f, 0.67f);

			int myID = ScriptEngine::Get_ID ( obj );

			if (myID == M11_FLOOR01_SECURITY_TURRET_01_JDG)
			{
				powerupSpawnLocation = turret01_spawnLocation;
			}

			else if (myID == M11_FLOOR01_SECURITY_TURRET_03_JDG)
			{
				powerupSpawnLocation = turret03_spawnLocation;
			}

			else if (myID == M11_FLOOR01_SECURITY_TURRET_05_JDG)
			{
				powerupSpawnLocation = turret05_spawnLocation;
			}

			else if (myID == M11_FLOOR01_SECURITY_TURRET_08_JDG)
			{
				powerupSpawnLocation = turret08_spawnLocation;
			}

			else if (myID == M11_FLOOR01_SECURITY_TURRET_10_JDG)
			{
				powerupSpawnLocation = turret10_spawnLocation;
			}

			else if (myID == M11_FLOOR01_SECURITY_TURRET_12_JDG)
			{
				powerupSpawnLocation = turret12_spawnLocation;
			}

			static constexpr const char *powerups[9] =
			{
				//this is a list of potential powerups to be dropped by destroyed security turrets
				"POW_Health_100",
				"POW_Armor_100",
				"POW_RocketLauncher_Player",
				"POW_SniperRifle_Player",
				"POW_TiberiumFlechetteGun_Player",
				"POW_LaserChaingun_Player",
				"POW_GrenadeLauncher_Player",
				"POW_LaserRifle_Player",
				"POW_TiberiumAutoRifle_Player",
				//more to be added later

			};
			int random = ScriptEngine::Get_Random_Int(0, 9);

			ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
		}

		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_FLOOR01_SECURITY_CONTROLLER_JDG), 0, M01_YOUR_OPERATOR_IS_DEAD_JDG, 0 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)
		{
			//int myID = ScriptEngine::Get_ID ( obj );

			/*if (obj && myID == M11_FLOOR01_SECURITY_TURRET_01_JDG || myID == M11_FLOOR01_SECURITY_TURRET_10_JDG)
			{
				Vector3 myTarget = ScriptEngine::Get_Position ( obj );
				myTarget.X += ScriptEngine::Get_Random ( -5, 5 );
				myTarget.Y += ScriptEngine::Get_Random ( -5, 5 );
				myTarget.Z += ScriptEngine::Get_Random ( -5, 0 );

				params.Set_Basic(this, 100, M01_START_ATTACKING_02_JDG);
				params.Set_Attack( myTarget, 100, 0, true );
				params.AttackCheckBlocked = false;
				ScriptEngine::Action_Attack( obj, params );

				float delayTimer = ScriptEngine::Get_Random ( 3, 5 );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
			}

			else */if (obj)
			{
				float myAccuracy = ScriptEngine::Get_Random ( 0, 1 );

				params.Set_Basic(this, 100, M01_START_ATTACKING_01_JDG);
				params.Set_Attack( STAR, 100, myAccuracy, true );
				ScriptEngine::Action_Attack( obj, params );
			}

			ScriptEngine::Start_Sound ( alarmSound  );
			active = true;
		}

		else if (obj && param == M01_YOUR_OPERATOR_IS_DEAD_JDG)
		{
			active = false;
			ScriptEngine::Apply_Damage( obj, 10000, "BlamoKiller", nullptr );
		}

		else if (obj && param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			Vector3 myTarget = ScriptEngine::Get_Position ( obj );
			myTarget.X += ScriptEngine::Get_Random ( -5, 5 );
			myTarget.Y += ScriptEngine::Get_Random ( -5, 5 );
			myTarget.Z += ScriptEngine::Get_Random ( -5, 0 );

			params.Set_Basic(this, 100, M01_START_ATTACKING_02_JDG);
			params.Set_Attack( myTarget, 100, 0, true );
			params.AttackCheckBlocked = false;
			ScriptEngine::Action_Attack( obj, params );

			float delayTimer = ScriptEngine::Get_Random ( 1, 2 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
		}
	}
};

/***********************************************************************************************************************
Putting remaining scripts dealing with first few minutes of mision here--including C130 drop off stuff
************************************************************************************************************************/
DECLARE_SCRIPT(M11_FirstFloor_Stuff_Controller_JDG, "")//M11_FIRSTFLOOR_STUFF_CONTROLLER_JDG 100593
{
	typedef enum {
		FIRST_FLOOR,
		SECOND_FLOOR
	} M11_Location;

	M11_Location playersPosition;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( playersPosition, 1 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		playersPosition = FIRST_FLOOR;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			if (playersPosition == FIRST_FLOOR)
			{
				playersPosition = SECOND_FLOOR;
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100569), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100570), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100571), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100572), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100573), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100574), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100575), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100576), 0, M01_GOTO_IDLE_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100568), 0, M01_GOTO_IDLE_JDG, 0 );//artillery controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100633), 0, M01_GOTO_IDLE_JDG, 0 );//ion cannon controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101805), 0, M01_GOTO_IDLE_JDG, 0 );//obelisk explosion controller
			}
		}

		else if (param == M01_START_ACTING_JDG)
		{
			if (playersPosition == SECOND_FLOOR)
			{
				playersPosition = FIRST_FLOOR;
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100569), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100570), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100571), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100572), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100573), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100574), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100575), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100576), 0, M01_START_ACTING_JDG, 0 );//sound controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100568), 0, M01_START_ACTING_JDG, 0 );//artillery controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (100633), 0, M01_START_ACTING_JDG, 0 );//ion cannon controller
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (101805), 0, M01_START_ACTING_JDG, 0 );//obelisk explosion controller
			}
		}
	}
};

DECLARE_SCRIPT(M11_Turn_Off_FirstFloor_Stuff_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_FIRSTFLOOR_STUFF_CONTROLLER_JDG), 0, M01_GOTO_IDLE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Turn_On_FirstFloor_Stuff_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_FIRSTFLOOR_STUFF_CONTROLLER_JDG), 0, M01_START_ACTING_JDG, 0 );
		}
	}
};

Vector3 ionCannonlocations[10] = {
	Vector3 (35.916f, -74.797f, -0.115f),
	Vector3 (30.173f, -108.692f, 9.706f),
	Vector3 (16.727f, -88.545f, 1.839f),
	Vector3 (9.679f, -97.887f, 4.033f),
	Vector3 (2.377f, -74.329f, -0.097f),
	Vector3 (3.579f, -108.123f, 9.657f),
	Vector3 (-6.437f, -107.906f, 9.731f),
	Vector3 (-17.015f, -91.3109f, 3.493f),
	Vector3 (-31.627f, -87.764f, 6.168f),
	Vector3 (-22.152f, -73.552f, -0.079f),
};

DECLARE_SCRIPT(M11_ExternalIonCannon_Controller_JDG, "")//this guys id is 100633
{
	/*int last;
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last, 1 );
		SAVE_VARIABLE( commandClearance, 2 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		commandClearance = true;
		float delayTimer = ScriptEngine::Get_Random ( 45, 60 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_01_JDG, delayTimer );

		last = 12;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			commandClearance = false;
			ScriptEngine::Enable_Hibernation( obj, true );
		}

		else if (param == M01_START_ACTING_JDG)
		{
			commandClearance = true;
			ScriptEngine::Enable_Hibernation( obj, false );
			float delayTimer = ScriptEngine::Get_Random ( 0, 5 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}

		else if (param == M01_START_ATTACKING_01_JDG)
		{
			if (commandClearance == true)
			{
				Vector3 speaker01position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker02position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker03position = ScriptEngine::Get_Position ( STAR );

				speaker01position.X += ScriptEngine::Get_Random(-10,10);
				speaker01position.Y += ScriptEngine::Get_Random(-10,10);
				speaker01position.Z += ScriptEngine::Get_Random(0,10);

				speaker02position.X += ScriptEngine::Get_Random(-10,10);
				speaker02position.Y += ScriptEngine::Get_Random(-10,10);
				speaker02position.Z += ScriptEngine::Get_Random(0,10);

				speaker03position.X += ScriptEngine::Get_Random(-10,10);
				speaker03position.Y += ScriptEngine::Get_Random(-10,10);
				speaker03position.Z += ScriptEngine::Get_Random(0,10);

				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0095I1EVAC_SND", speaker01position, obj );
				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0095I1EVAC_SND", speaker02position, obj );
				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0095I1EVAC_SND", speaker03position, obj );

				float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_02_JDG, delayTimer );
			}
		}

		else if (param == M01_START_ATTACKING_02_JDG)
		{
			if (commandClearance == true)
			{
				int random = ScriptEngine::Get_Random_Int(0, 10);

				while (random == last) {
					random = ScriptEngine::Get_Random_Int(0, 10);
				}
				last = random;

				GameObject *ionCannonGuy = ScriptEngine::Create_Object("GDI_MiniGunner_IonCannon_JDG", ionCannonlocations[random]);
				ScriptEngine::Attach_Script(ionCannonGuy, "M11_IonCannon_Guy_JDG", "");

				float delayTimer = ScriptEngine::Get_Random ( 60, 120 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_01_JDG, delayTimer );
			}
		}
	}*/
};

DECLARE_SCRIPT(M11_IonCannon_Guy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_START_ACTING_JDG );
		params.Set_Attack( myPosition, 10, 0, true );
		params.AttackCheckBlocked = false;

		ScriptEngine::Action_Attack ( obj, params );
	}
};

DECLARE_SCRIPT(M11_ExternalSound_Controller_NodTanks_JDG, "")
{
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( commandClearance, 1 );
	}

	void Created( GameObject * obj ) override
	{
		commandClearance = true;
		float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
		ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			commandClearance = false;
		}

		else if (param == M01_START_ACTING_JDG)
		{
			commandClearance = true;
			float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (commandClearance == true)
		{
			Vector3 soundLocation = ScriptEngine::Get_Position ( obj );
			soundLocation.X += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Y += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Z += ScriptEngine::Get_Random ( 0, 10 );

			ScriptEngine::Create_Sound ( "M11_Exterior_NODTank_Twiddler_JDG", soundLocation, obj );

			float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}
};

DECLARE_SCRIPT(M11_ExternalSound_Controller_GDITanks_JDG, "")
{
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( commandClearance, 1 );
	}

	void Created( GameObject * obj ) override
	{
		commandClearance = true;
		float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
		ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			commandClearance = false;
		}

		else if (param == M01_START_ACTING_JDG)
		{
			commandClearance = true;
			float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (commandClearance == true)
		{
			Vector3 soundLocation = ScriptEngine::Get_Position ( obj );
			soundLocation.X += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Y += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Z += ScriptEngine::Get_Random ( 0, 10 );

			ScriptEngine::Create_Sound ( "M11_Exterior_GDITank_Twiddler_JDG", soundLocation, obj );

			float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}
};

DECLARE_SCRIPT(M11_ExternalSound_Controller_NodTroops_JDG, "")
{
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( commandClearance, 1 );
	}

	void Created( GameObject * obj ) override
	{
		commandClearance = true;
		float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
		ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			commandClearance = false;
		}

		else if (param == M01_START_ACTING_JDG)
		{
			commandClearance = true;
			float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (commandClearance == true)
		{
			Vector3 soundLocation = ScriptEngine::Get_Position ( obj );
			soundLocation.X += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Y += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Z += ScriptEngine::Get_Random ( 0, 10 );

			ScriptEngine::Create_Sound ( "M11_Exterior_NODTroops_Twiddler_JDG", soundLocation, obj );

			float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}
};

DECLARE_SCRIPT(M11_ExternalSound_Controller_GDITroops_JDG, "")
{
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( commandClearance, 1 );
	}

	void Created( GameObject * obj ) override
	{
		commandClearance = true;
		float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
		ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			commandClearance = false;
		}

		else if (param == M01_START_ACTING_JDG)
		{
			commandClearance = true;
			float delayTimer = ScriptEngine::Get_Random ( 0, 3 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (commandClearance == true)
		{
			Vector3 soundLocation = ScriptEngine::Get_Position ( obj );
			soundLocation.X += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Y += ScriptEngine::Get_Random ( -10, 10 );
			soundLocation.Z += ScriptEngine::Get_Random ( 0, 10 );

			ScriptEngine::Create_Sound ( "M11_Exterior_GDITroops_Twiddler_JDG", soundLocation, obj );

			float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}
};

Vector3 m11Artillerylocations[35] = {
	Vector3 (3.486f, -53.794f, 3.799f),
	Vector3 (-20.923f, -31.001f, 26.833f),
	Vector3 (-23.338f, 2.514f, 26.502f),
	Vector3 (-23.097f, 32.220f, 38.927f),
	Vector3 (-12.618f, -21.520f, 12.796f),
	Vector3 (-2.253f, 5.014f, 23.738f),
	Vector3 (7.692f, 7.967f, 19.242f),
	Vector3 (0.726f, 20.706f, 24.670f),
	Vector3 (-11.148f, 50.407f, 38.031f),
	Vector3 (4.330f, 49.481f, 48.165f),
	Vector3 (9.350f, 48.324f, 40.018f),
	Vector3 (20.210f, 45.413f, 32.186f),
	Vector3 (21.162f, 31.207f, 26.841f),
	Vector3 (21.673f, 1.765f, 36.254f),
	Vector3 (19.174f, -28.685f, 21.450f),
	Vector3 (4.334f, -16.692f, 0),
	Vector3 (-3.070f, -4.271f, 0),
	Vector3 (-0.372f, -5.744f, -18.765f),
	Vector3 (43.437f, -64.747f, 51.465f),
	Vector3 (54.511f, -32.046f, 59.763f),
	Vector3 (48.525f, -14.851f, 50.524f),
	Vector3 (47.396f, 4.976f, 54.233f),
	Vector3 (-47.600f, 19.498f, 48.110f),
	Vector3 (-51.750f, -1.745f, 54.846f),
	Vector3 (-47.856f, -25.844f, 45.516f),
	Vector3 (4.559f, -123.847f, 28.179f),
	Vector3 (11.093f, -125.641f, 37.388f),
	Vector3 (8.425f, -127.718f, 45.484f),
	Vector3 (-11.490f, -165.685f, 42.821f),
	Vector3 (-35.123f, -155.089f, 34.401f),
	Vector3 (29.645f, -165.685f, 42.816f),
	Vector3 (-1.107f, -163.946f, 19.644f),
	Vector3 (-21.683f, -141.336f, 19),
	Vector3 (42.883f, -165.685f, 26.360f),
	Vector3 (-24.331f, -107.697f, 15.882f),
};

DECLARE_SCRIPT(M11_ExternalArtillery_Controller_JDG, "")
{
	Vector3 sound_location;
	int last;
	int bomb_sound;
	float delayTimerMin;
	float delayTimerMax;
	bool command_clearance;
	int bombcount;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sound_location, 1);
		SAVE_VARIABLE(last, 2);
		SAVE_VARIABLE(bomb_sound, 3);
		SAVE_VARIABLE(command_clearance, 4);
		SAVE_VARIABLE(delayTimerMin, 5);
		SAVE_VARIABLE(delayTimerMax, 6);
		SAVE_VARIABLE(bombcount, 7);
	}

	void Created( GameObject * obj ) override
	{
		last = 40;
		delayTimerMin = 0;
		delayTimerMax = 10;
		command_clearance = true;
		bombcount = 0;

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 2 );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_SOUND_ENDED  )
		{
			if (param == bomb_sound)
			{
				Vector3 explosion_location = sound_location;

				explosion_location.Z -= 10;
				ScriptEngine::Create_Explosion("Ground Explosions Twiddler", explosion_location, nullptr);

				float delayTimer = ScriptEngine::Get_Random ( delayTimerMin, delayTimerMax );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, delayTimer );
			}
		}

		switch (param)
		{
			case M01_PICK_A_NEW_LOCATION_JDG:
				{
					if (command_clearance == true && (bombcount < 10))
					{
						bombcount++;
						int random = int(ScriptEngine::Get_Random(0, 35-WWMATH_EPSILON));
						while (random == last)
						{
							random = int(ScriptEngine::Get_Random(0, 35-WWMATH_EPSILON));
						}

						last = random;

						sound_location = m11Artillerylocations[random];
						sound_location.Z += 5;

						bomb_sound = ScriptEngine::Create_Sound ( "Bomb_Falling_Sound_Twiddler", sound_location, obj );
						ScriptEngine::Monitor_Sound ( obj, bomb_sound );
					}
				}
				break;

			case M01_GOTO_IDLE_JDG:
				{
					command_clearance = false;
				}
				break;

			case M01_START_ACTING_JDG:
				{
					command_clearance = true;
					ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 2 );
				}
				break;
		}
	}
};


DECLARE_SCRIPT(M11_Floor01_StealthSoldier01_JDG, "")
{
	bool attackingPlayer;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( attackingPlayer, 1 );
	}

	void Created( GameObject * obj ) override
	{
		attackingPlayer = false;
		ScriptEngine::Enable_Stealth(obj, false);
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(-6.242f, 28.340f, -10.015f), RUN, 0.25f);

		ScriptEngine::Action_Goto( obj, params );

		float delayTimer = ScriptEngine::Get_Random ( 1, 3 );
		ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (obj)
		{
			ScriptEngine::Enable_Stealth(obj, true);
		}
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if (poker == STAR && attackingPlayer == false)
		{
			attackingPlayer = true;
			ScriptEngine::Action_Reset ( obj, 100 );
			ActionParamsStruct params;
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
			params.Set_Movement( STAR, RUN, 10);
			params.Set_Attack( STAR, 10, 0, true );
			ScriptEngine::Action_Attack (obj, params);
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager == STAR && attackingPlayer == false)
		{
			attackingPlayer = true;
			ScriptEngine::Action_Reset ( obj, 100 );
			ActionParamsStruct params;
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
			params.Set_Movement( STAR, RUN, 10);
			params.Set_Attack( STAR, 10, 0, true );
			ScriptEngine::Action_Attack (obj, params);
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG && attackingPlayer == false)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
			Vector3 playersPosition = ScriptEngine::Get_Position ( STAR );

			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Face_Location( playersPosition, 1 );
			ScriptEngine::Action_Face_Location ( obj, params );
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && attackingPlayer == false)
		{
			Vector3 playersPosition = ScriptEngine::Get_Position ( STAR );
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Face_Location( playersPosition, 1 );
			ScriptEngine::Action_Face_Location ( obj, params );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (obj)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG && attackingPlayer == false)
			{
				attackingPlayer = true;
				ScriptEngine::Action_Reset ( obj, 100 );

				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( "CnC_Cloaking", myPosition, obj );

				ScriptEngine::Debug_Message ( "***************************m11 stealth guy 01 has received custom to attack player\n" );
				ActionParamsStruct params;
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
				params.Set_Attack( STAR, 10, 0, true );
				ScriptEngine::Action_Attack (obj, params);
			}
		}
	}
};

DECLARE_SCRIPT(M11_Floor01_StealthSoldier02_JDG, "")
{
	bool attackingPlayer;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( attackingPlayer, 1 );
	}

	void Created( GameObject * obj ) override
	{
		attackingPlayer = false;
		ScriptEngine::Enable_Stealth(obj, false);
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(6.014f, 27.870f, -10.015f), RUN, 0.25f);

		ScriptEngine::Action_Goto( obj, params );

		float delayTimer = ScriptEngine::Get_Random ( 1, 3 );
		ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (obj)
		{
			ScriptEngine::Enable_Stealth(obj, true);
		}
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if (poker == STAR)
		{
			ScriptEngine::Action_Reset ( obj, 100 );
			ActionParamsStruct params;
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
			params.Set_Movement( STAR, RUN, 10);
			params.Set_Attack( STAR, 10, 0, true );
			ScriptEngine::Action_Attack (obj, params);
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager == STAR)
		{
			ScriptEngine::Action_Reset ( obj, 100 );
			ActionParamsStruct params;
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
			params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
			params.Set_Movement( STAR, RUN, 10);
			params.Set_Attack( STAR, 10, 0, true );
			ScriptEngine::Action_Attack (obj, params);
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
			Vector3 playersPosition = ScriptEngine::Get_Position ( STAR );

			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Face_Location( playersPosition, 1 );
			ScriptEngine::Action_Face_Location ( obj, params );
		}

		else if (action_id == M01_DOING_ANIMATION_01_JDG && attackingPlayer == false)
		{
			Vector3 playersPosition = ScriptEngine::Get_Position ( STAR );
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Face_Location( playersPosition, 1 );
			ScriptEngine::Action_Face_Location ( obj, params );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (obj)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG && attackingPlayer == false)
			{
				attackingPlayer = true;
				ScriptEngine::Action_Reset ( obj, 100 );

				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( "CnC_Cloaking", myPosition, obj );

				ActionParamsStruct params;
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
				params.Set_Movement( STAR, RUN, 10);
				params.Set_Attack( STAR, 10, 0, true );
				ScriptEngine::Action_Attack (obj, params);
			}
		}
	}
};

DECLARE_SCRIPT(M11_Floor01_FireGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		float myHealth = ScriptEngine::Get_Health ( obj );
		float healthFudge = ScriptEngine::Get_Random ( 0.25f, 0.5f );
		myHealth = myHealth * healthFudge;

		ScriptEngine::Set_Health ( obj, myHealth );
		ScriptEngine::Apply_Damage( obj, 10, "Flamethrower", nullptr );
		ScriptEngine::Enable_Hibernation( obj, false );

		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 1 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			ScriptEngine::Apply_Damage( obj, 10, "Flamethrower", nullptr );
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 1 );
		}
	}
};

DECLARE_SCRIPT(M11_Floor01_ChaseGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		params.Set_Basic( this, 100, M01_GOING_TO_HAVOC_JDG );
		params.Set_Movement( STAR, RUN, 10);
		params.Set_Attack( STAR, 50, 1, true );
		ScriptEngine::Action_Attack (obj, params);
	}
};

DECLARE_SCRIPT(M11_C130_Dropoff_Controller_JDG, "")//M11_C130_DROPOFF_CONTROLLER_JDG 100221
{
	int last;
	int active_unit_count;
	int dropoffcount;
	int dropoffMax;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(last, 1);
		SAVE_VARIABLE(active_unit_count, 2);
		SAVE_VARIABLE(dropoffcount, 3);
		SAVE_VARIABLE(dropoffMax, 4);
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Start_Timer(obj, this, 10.0f, 0);
		active_unit_count = 0;
		last = 20;
		dropoffcount = 0;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_SPAWNER_IS_DEAD_JDG)
		{
			active_unit_count--;
			float delayTimer = ScriptEngine::Get_Random ( 0, 30 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (obj && active_unit_count <= 1)
		{
			int easy = 0;
			int medium = 1;

			int currentDifficulty = ScriptEngine::Get_Difficulty_Level( );

			if (currentDifficulty == easy)
			{
				dropoffMax = M11_C130_EASY_SPAWN_CAP_JDG;
			}

			else if (currentDifficulty == medium)
			{
				dropoffMax = M11_C130_MEDIUM_SPAWN_CAP_JDG;
			}

			else
			{
				dropoffMax = M11_C130_HARD_SPAWN_CAP_JDG;
			}

			if (dropoffcount <= dropoffMax)
			{
				dropoffcount++;
				active_unit_count += 3;
				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.2f, -25.5f, -10.0f));
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_C130Troopdrop.txt");
				ScriptEngine::Set_Facing ( controller, 90);

			}
		}
	}
};

DECLARE_SCRIPT(M11_C130_Dropoff_Dude_JDG, "")//also have M01_Hunt_The_Player_JDG script attached to these guys
{
	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "S_a_Human.H_A_X5D_ParaT_1") == 0)
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG","");
		}

		else if (stricmp(anim, "S_a_Human.H_A_X5D_ParaT_2") == 0)
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG","");
		}

		else if (stricmp(anim, "S_a_Human.H_A_X5D_ParaT_3") == 0)
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG","");
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_C130_DROPOFF_CONTROLLER_JDG ), 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
	}
};
/***********************************************************************************************************************
Putting scripts dealing with the netrunner room here
************************************************************************************************************************/
DECLARE_SCRIPT(M11_NetRunnerRoom_EntryZone_JDG, "")
{
	bool entered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(entered, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		entered = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && entered == false)
		{
			entered = true;
			GameObject * exitZone = ScriptEngine::Find_Object ( 100228 );
			if (exitZone != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, exitZone, 0, M01_START_ACTING_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};

DECLARE_SCRIPT(M11_NetRunnerRoom_ExitZone_JDG, "")
{
	bool okay_to_trigger;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(okay_to_trigger, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		okay_to_trigger = false;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			okay_to_trigger = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && okay_to_trigger == true)
		{
			GameObject * stealthSoldier01 = ScriptEngine::Find_Object ( 100225 );
			GameObject * stealthSoldier02 = ScriptEngine::Find_Object ( 100227 );

			if (stealthSoldier01 != nullptr)
			{
				float delayTimer01 = ScriptEngine::Get_Random ( 0, 1 );
				ScriptEngine::Send_Custom_Event ( obj, stealthSoldier01, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer01 );
			}

			if (stealthSoldier02 != nullptr)
			{
				float delayTimer02 = ScriptEngine::Get_Random ( 0, 1 );
				ScriptEngine::Send_Custom_Event ( obj, stealthSoldier02, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer02 );
			}
		}
	}
};

/***********************************************************************************************************************
Putting scripts dealing with the museum and the bar-area rooms here
************************************************************************************************************************/
DECLARE_SCRIPT(M11_MuseumLevel_EntryZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * museumGuard01 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_01_JDG );
			GameObject * museumGuard02 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_02_JDG );
			GameObject * museumGuard03 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_03_JDG );
			GameObject * museumGuard04 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_04_JDG );
			GameObject * museumGuard05 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_05_JDG );
			GameObject * museumGuard06 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_06_JDG );

			if (museumGuard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (museumGuard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard02, 0, M01_START_ACTING_JDG, 0 );
			}

			if (museumGuard03 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard03, 0, M01_START_ACTING_JDG, 0 );
			}

			if (museumGuard04 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard04, 0, M01_START_ACTING_JDG, 0 );
			}

			if (museumGuard05 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard05, 0, M01_START_ACTING_JDG, 0 );
			}

			if (museumGuard06 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard06, 0, M01_START_ACTING_JDG, 0 );
			}

			GameObject * zone01 = ScriptEngine::Find_Object ( 100254 );
			GameObject * zone02 = ScriptEngine::Find_Object ( 100255 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_MuseumLevel_Spawner_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Start_Timer ( obj, this, 30, M01_GOTO_IDLE_JDG );
	}

	void Timer_Expired( GameObject * obj, int timer_id ) override
	{
		if (obj)
		{
			if (timer_id == M01_GOTO_IDLE_JDG)
			{
				ScriptEngine::Enable_Hibernation( obj, true );
			}
		}
	}
};

DECLARE_SCRIPT(M11_MuseumLevel_Defender_Aggressive_JDG, "")
{
	bool okay_to_spawn;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(okay_to_spawn, 1);
	}

	void Created( GameObject * obj ) override
	{
		okay_to_spawn = true;
		ScriptEngine::Innate_Disable(obj);
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		if (okay_to_spawn == true)
		{
			GameObject * newGuy = ScriptEngine::Trigger_Spawner( M11_MUSEUM_SPAWNER_JDG );
			ScriptEngine::Attach_Script(newGuy, "M11_MuseumLevel_Spawner_JDG", "");
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
			ScriptEngine::Set_Innate_Aggressiveness ( obj, 1 );
			ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 0 );
		}

		else if (param == M01_GOTO_IDLE_JDG)
		{
			okay_to_spawn = false;
		}
	}
};

DECLARE_SCRIPT(M11_MuseumLevel_Defender_Defensive_JDG, "")
{
	bool okay_to_spawn;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(okay_to_spawn, 1);
	}

	void Created( GameObject * obj ) override
	{
		okay_to_spawn = true;
		ScriptEngine::Innate_Disable(obj);
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		if (okay_to_spawn == true)
		{
			GameObject * newGuy = ScriptEngine::Trigger_Spawner( M11_MUSEUM_SPAWNER_JDG );
			ScriptEngine::Attach_Script(newGuy, "M11_MuseumLevel_Spawner_JDG", "");
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
			ScriptEngine::Set_Innate_Aggressiveness ( obj, 1 );
			ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 1 );
		}

		else if (param == M01_GOTO_IDLE_JDG)
		{
			okay_to_spawn = false;
		}
	}
};

DECLARE_SCRIPT(M11_MuseumLevel_Disable_Spawners_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * museumGuard01 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_01_JDG );
			GameObject * museumGuard02 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_02_JDG );
			GameObject * museumGuard03 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_03_JDG );
			GameObject * museumGuard04 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_04_JDG );
			GameObject * museumGuard05 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_05_JDG );
			GameObject * museumGuard06 = ScriptEngine::Find_Object ( M11_MUSEUM_GUARD_06_JDG );

			if (museumGuard01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard01, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			if (museumGuard02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard02, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			if (museumGuard03 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard03, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			if (museumGuard04 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard04, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			if (museumGuard05 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard05, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			if (museumGuard06 != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, museumGuard06, 0, M01_GOTO_IDLE_JDG, 0 );
			}

			GameObject * zone01 = ScriptEngine::Find_Object ( 100324 );
			GameObject * zone02 = ScriptEngine::Find_Object ( 100325 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_WetBar_NeighborRoom_EntryZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * controller = ScriptEngine::Find_Object ( M11_WETBAR_SPAWNER_CONTROLLER_JDG );
			if (controller != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, controller, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_WetBar_NeighborRoom_SpawnerController_JDG, "")
{
	int activeSoldiers;
	int spawn_01_ID;
	int spawn_02_ID;
	int spawn_03_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(activeSoldiers, 1);
		SAVE_VARIABLE(spawn_01_ID, 2);
		SAVE_VARIABLE(spawn_02_ID, 3);
		SAVE_VARIABLE(spawn_03_ID, 4);
	}

	void Created( GameObject * /* obj */ ) override
	{
		activeSoldiers = 0;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_SPAWNER_SPAWN_PLEASE_JDG)
		{
			GameObject * spawnedGuy01 = ScriptEngine::Find_Object ( spawn_01_ID );
			GameObject * spawnedGuy02 = ScriptEngine::Find_Object ( spawn_02_ID );
			GameObject * spawnedGuy03 = ScriptEngine::Find_Object ( spawn_03_ID );

			if (spawnedGuy01 == nullptr)
			{
				spawnedGuy01 = ScriptEngine::Trigger_Spawner( M11_WETBAR_SPAWNER_01_JDG );
				spawn_01_ID = ScriptEngine::Get_ID ( spawnedGuy01 );
			}

			if (spawnedGuy02 == nullptr)
			{
				spawnedGuy02 = ScriptEngine::Trigger_Spawner( M11_WETBAR_SPAWNER_02_JDG );
				spawn_02_ID = ScriptEngine::Get_ID ( spawnedGuy02 );
			}

			if (spawnedGuy03 == nullptr)
			{
				spawnedGuy03 = ScriptEngine::Trigger_Spawner( M11_WETBAR_SPAWNER_03_JDG );
				spawn_03_ID = ScriptEngine::Get_ID ( spawnedGuy03 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_WetBar_MutantGuard_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 powerupSpawnLocation = ScriptEngine::Get_Position (obj);
		powerupSpawnLocation.Z += 0.75f;

		ScriptEngine::Create_Object ("POW_TiberiumAutoRifle_Player", powerupSpawnLocation );
	}
};

Vector3 m11WetBarSeachLocations[12] = {
	Vector3(-9.862f, -10.541f, -29.855f),
	Vector3(-0.757f, -12.594f, -29.855f),
	Vector3(9.187f, -12.521f, -29.855f),
	Vector3(8.002f, -2.370f, -29.855f),
	Vector3(0.163f, -2.219f, -29.855f),
	Vector3(-8.423f, -3.841f, -29.855f),
	Vector3(-9.398f, 5.363f, -29.855f),
	Vector3(0.583f, 10.380f, -29.855f),
	Vector3(9.479f, 4.651f, -29.855f),
	Vector3(12.758f, -1.367f, -24.531f),
	Vector3(-13.295f, -1.080f, -24.531f),
	Vector3(-2.069f, -15.477f, -24.531f),
};

DECLARE_SCRIPT(M11_WetBar_NeighborRoom_SpawnedDude_JDG, "")
{
	int last;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(last, 1);
	}

	void Created( GameObject * obj ) override
	{
		last = 20;

		ScriptEngine::Set_Innate_Aggressiveness ( obj, 0.3f );
		ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 1 );

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 3 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		switch (param)
		{
			case M01_PICK_A_NEW_LOCATION_JDG:
				{
					int random = ScriptEngine::Get_Random_Int(0, 12);

					while (random == last) {
						random = ScriptEngine::Get_Random_Int(0, 12);
					}

					last = random;

					Vector3 NewGotoLocation = m11WetBarSeachLocations[random];

					ActionParamsStruct params;
					params.Set_Basic( this, INNATE_PRIORITY_BULLET_HEARD - 5, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( NewGotoLocation, WALK, 1, false );
					ScriptEngine::Action_Goto( obj, params );
				}
				break;
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		switch (complete_reason)
		{
			case ACTION_COMPLETE_NORMAL:
				{
					if (action_id == M01_WALKING_WAYPATH_01_JDG)
					{
						const char* animationName = M01_Choose_Search_Animation ( );

						params.Set_Basic( this, INNATE_PRIORITY_BULLET_HEARD - 5, M01_DOING_ANIMATION_01_JDG );
						params.Set_Animation (animationName, false);
						ScriptEngine::Action_Play_Animation (obj, params);
					}

					else if (action_id == M01_DOING_ANIMATION_01_JDG)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 0 );
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_GreenRoom_EntryZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * surpriseGuy01 = ScriptEngine::Create_Object ( "Nod_RocketSoldier_2SF_Chaingun", Vector3 (-10.408f, 32.538f, -29.855f) );
			ScriptEngine::Attach_Script(surpriseGuy01, "M11_GreenRoom_SurpriseGuy_JDG", "");

			GameObject * surpriseGuy02 = ScriptEngine::Create_Object ( "Nod_RocketSoldier_2SF_Chaingun", Vector3 (10.796f, 32.476f, -29.855f) );
			ScriptEngine::Attach_Script(surpriseGuy02, "M11_GreenRoom_SurpriseGuy_JDG", "");

			ScriptEngine::Destroy_Object (obj );//one-time event--cleaning up zone
		}
	}
};

DECLARE_SCRIPT(M11_GreenRoom_SurpriseGuy_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Grant_Key( obj, 2, true );
		ScriptEngine::Grant_Key( obj, 3, true );
		ActionParamsStruct params;

		params.Set_Basic(this, 80, M01_WALKING_WAYPATH_02_JDG);
		params.Set_Movement(Vector3 (-0.471f, 23.168f, -29.855f), RUN, 2);

		ScriptEngine::Action_Goto(obj, params);
	}
};
/***********************************************************************************************************************
Putting scripts dealing with war room encounters here
************************************************************************************************************************/
DECLARE_SCRIPT(M11_WarRoom_EntryZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * engineer = ScriptEngine::Find_Object ( M11_WARROOM_ENGINEER_JDG );

			if (engineer != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, engineer, 0, M01_START_ACTING_JDG, 0 );
			}

			GameObject * turret01 = ScriptEngine::Find_Object (M11_WARROOM_SECURITY_TURRET_01_JDG);
			GameObject * turret02 = ScriptEngine::Find_Object (M11_WARROOM_SECURITY_TURRET_02_JDG);

			if (turret01 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret01, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret02 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
				ScriptEngine::Send_Custom_Event( obj, turret02, 0, M01_START_ACTING_JDG, delayTimer );
			}

			GameObject * zone01 = ScriptEngine::Find_Object ( 100628 );
			GameObject * zone02 = ScriptEngine::Find_Object ( 100629 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_WarRoom_Engineer_JDG, "")//M11_WARROOM_ENGINEER_JDG 100353
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ActionParamsStruct params;

			params.Set_Basic(this, INNATE_PRIORITY_GUNSHOT_HEARD-5, 100);
			params.Set_Animation ("H_A_CON2", true);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_WARROOM_SECURITY_CONTROLLER_JDG ), 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
	}
};

DECLARE_SCRIPT(M11_WarRoom_StationaryTech_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;

		params.Set_Basic(this, INNATE_PRIORITY_GUNSHOT_HEARD-5, 100);
		params.Set_Animation ("H_A_CON2", true);
		ScriptEngine::Action_Play_Animation (obj, params);

		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_WARROOM_SECURITY_CONTROLLER_JDG ), 0, M01_START_ACTING_JDG, 0 );
	}
};

DECLARE_SCRIPT(M11_WarRoom_SecurityTurret_JDG, "")
{
	bool active;
	int alarmSound;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(active, 1);
		SAVE_VARIABLE(alarmSound, 2);
	}

	void Created( GameObject * obj ) override
	{
		active = false;

		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		alarmSound = ScriptEngine::Create_Sound ( "SFX.Klaxon_Alert_01", myPosition, obj );
		ScriptEngine::Stop_Sound ( alarmSound, false );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		if (active == true)
		{
			ScriptEngine::Stop_Sound ( alarmSound, true );

			Vector3 powerupSpawnLocation = ScriptEngine::Get_Position (obj);
			powerupSpawnLocation.Z = -34.539f;

			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", powerupSpawnLocation );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)
		{
			if (obj)
			{
				active = true;
				ScriptEngine::Start_Sound ( alarmSound  );
				float myAccuracy = ScriptEngine::Get_Random ( 0, 1 );

				params.Set_Basic(this, 100, M01_START_ATTACKING_01_JDG);
				params.Set_Attack( STAR, 100, myAccuracy, true );
				ScriptEngine::Action_Attack( obj, params );
			}
		}
	}
};

DECLARE_SCRIPT(M11_WarRoom_BattleController_JDG, "")//M11_WARROOM_SECURITY_CONTROLLER_JDG 100361
{
	bool okay_to_spawn_warroom;
	bool okay_to_spawn_barracks;
	int active_soldier_count;
	int alarmSound;
	int spawnedSoldierCount;
	int maxSpawnedSoldierCount;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(okay_to_spawn_warroom, 1);
		SAVE_VARIABLE(okay_to_spawn_barracks, 2);
		SAVE_VARIABLE(active_soldier_count, 3);
		SAVE_VARIABLE(alarmSound, 4);
		SAVE_VARIABLE(spawnedSoldierCount, 5);
		SAVE_VARIABLE(maxSpawnedSoldierCount, 6);
	}

	void Created( GameObject * /* obj */ ) override
	{
		okay_to_spawn_warroom = false;
		okay_to_spawn_barracks = false;
		active_soldier_count = 4;
		spawnedSoldierCount = 0;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_SPAWNER_IS_DEAD_JDG)
		{
			active_soldier_count--;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}

		else if (param == M01_START_ACTING_JDG)
		{
			okay_to_spawn_warroom = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}

		else if (param == M01_SPAWNER_SPAWN_PLEASE_JDG)
		{
			int currentDifficulty = ScriptEngine::Get_Difficulty_Level();;
			int hard = 2;
			int med = 1;

			if (currentDifficulty == hard)
			{
				maxSpawnedSoldierCount = 18;
			}

			else if (currentDifficulty == med)
			{
				maxSpawnedSoldierCount = 12;
			}

			else
			{
				maxSpawnedSoldierCount = 6;
			}

			if (spawnedSoldierCount < maxSpawnedSoldierCount)
			{
				if (active_soldier_count < 8)
				{
					if (okay_to_spawn_warroom == true)//M11_WARROOM_SECURITY_SPAWNER_02_JDG
					{
						active_soldier_count++;
						spawnedSoldierCount++;
						active_soldier_count++;
						spawnedSoldierCount++;
						GameObject * newGuy = ScriptEngine::Trigger_Spawner( M11_WARROOM_SECURITY_SPAWNER_JDG );
						ScriptEngine::Attach_Script(newGuy, "M11_WarRoom_SpawnedDude_JDG", "");

						GameObject * newGuy02 = ScriptEngine::Trigger_Spawner( M11_WARROOM_SECURITY_SPAWNER_02_JDG );
						ScriptEngine::Attach_Script(newGuy02, "M11_WarRoom_SpawnedDude_JDG", "");
					}

					ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 1 );
				}
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//player is leaving room--disable warroom spawn ability
		{
			okay_to_spawn_warroom = false;
			okay_to_spawn_barracks = true;
		}
	}
};

DECLARE_SCRIPT(M11_WarRoom_SpawnedDude_JDG, "")//M11_WARROOM_SECURITY_SPAWNER_JDG 100362
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
		ScriptEngine::Enable_Hibernation( obj, false );
		ActionParamsStruct params;
		params.Set_Basic(this, 100, M01_GOING_TO_HAVOC_JDG);
		params.Set_Movement( STAR, RUN, 10 );
		params.Set_Attack( STAR, 10, 0, true );
		ScriptEngine::Action_Attack ( obj, params );

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_GOTO_IDLE_JDG, 60 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (obj && param == M01_GOTO_IDLE_JDG)
		{
			ScriptEngine::Enable_Hibernation( obj, true );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_WARROOM_SECURITY_CONTROLLER_JDG ), 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
	}
};

DECLARE_SCRIPT(M11_WarRoom_ExitZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_WARROOM_SECURITY_CONTROLLER_JDG ), 0, M01_MODIFY_YOUR_ACTION_04_JDG, 0 );

			GameObject * zone01 = ScriptEngine::Find_Object ( 100363 );
			GameObject * zone02 = ScriptEngine::Find_Object ( 100364 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};
/***********************************************************************************************************************
Putting scripts dealing with the barracks/living quarters encounters here
************************************************************************************************************************/
DECLARE_SCRIPT(M11_Barracks_SpawnerController_JDG, "")//M11_BARRACKS_SPAWNER_CONTROLLER_JDG	100430
{
	int spawn_01_ID;
	int spawn_02_ID;
	int spawn_03_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(spawn_01_ID, 1);
		SAVE_VARIABLE(spawn_02_ID, 2);
		SAVE_VARIABLE(spawn_03_ID, 3);
	}

	void Created( GameObject * /* obj */ ) override
	{
		spawn_01_ID = 0;
		spawn_02_ID = 0;
		spawn_03_ID = 0;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_SPAWNER_SPAWN_PLEASE_JDG)
		{
			GameObject * spawnedGuy01 = ScriptEngine::Find_Object ( spawn_01_ID );
			GameObject * spawnedGuy02 = ScriptEngine::Find_Object ( spawn_02_ID );
			GameObject * spawnedGuy03 = ScriptEngine::Find_Object ( spawn_03_ID );

			if (spawnedGuy01 == nullptr)
			{
				spawnedGuy01 = ScriptEngine::Trigger_Spawner( M11_BARRACKS_SPAWNER_01_JDG );
				ScriptEngine::Attach_Script(spawnedGuy01, "M11_Barracks_SpawnedDude_JDG", "");
				spawn_01_ID = ScriptEngine::Get_ID ( spawnedGuy01 );
			}

			if (spawnedGuy02 == nullptr)
			{
				spawnedGuy02 = ScriptEngine::Trigger_Spawner( M11_BARRACKS_SPAWNER_02_JDG );
				ScriptEngine::Attach_Script(spawnedGuy02, "M11_Barracks_SpawnedDude_JDG", "");
				spawn_02_ID = ScriptEngine::Get_ID ( spawnedGuy02 );
			}

			if (spawnedGuy03 == nullptr)
			{
				spawnedGuy03 = ScriptEngine::Trigger_Spawner( M11_BARRACKS_SPAWNER_03_JDG );
				ScriptEngine::Attach_Script(spawnedGuy03, "M11_Barracks_SpawnedDude_JDG", "");
				spawn_03_ID = ScriptEngine::Get_ID ( spawnedGuy03 );
			}
		}
	}
};

Vector3 m11BarracksSeachLocations[9] = {
	Vector3(-12.2f, 1.3f, -34.2f),
	Vector3(1.2f, -11.1f, -34.2f),
	Vector3(13.2f, 2.0f, -34.2f),
	Vector3(0.2f, -16.6f, -34.2f),
	Vector3(-9.8f, -9.0f, -38.2f),
	Vector3(-10, 2.5f, -38.2f),
	Vector3(0.7f, -11.0f, -38.2f),
	Vector3(10.8f, -8.9f, -38.2f),
	Vector3(10.4f, 2.4f, -38.2f),
};

DECLARE_SCRIPT(M11_Barracks_SpawnedDude_JDG, "")//
{
	int last;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(last, 1);
	}

	void Created( GameObject * obj ) override
	{
		last = 20;

		ScriptEngine::Set_Innate_Aggressiveness ( obj, 1 );
		ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 0 );

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 3 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		switch (param)
		{
			case M01_PICK_A_NEW_LOCATION_JDG:
				{
					int random = ScriptEngine::Get_Random_Int(0, 9);

					while (random == last) {
						random = ScriptEngine::Get_Random_Int(0, 9);
					}

					last = random;

					Vector3 NewGotoLocation = m11BarracksSeachLocations[random];

					ActionParamsStruct params;
					params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, M01_WALKING_WAYPATH_01_JDG );
					params.Set_Movement( NewGotoLocation, RUN, 3, false );
					ScriptEngine::Action_Goto( obj, params );
				}
				break;
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		switch (complete_reason)
		{
			case ACTION_COMPLETE_NORMAL:
				{
					if (action_id == M01_WALKING_WAYPATH_01_JDG)
					{
						const char* animationName = M01_Choose_Search_Animation ( );

						params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, M01_DOING_ANIMATION_01_JDG );
						params.Set_Animation (animationName, false);
						ScriptEngine::Action_Play_Animation (obj, params);
					}

					else if (action_id == M01_DOING_ANIMATION_01_JDG)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 0 );
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T20_EntryZone_JDG, "")//100414 entry to mutant reject w/volt ammo
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_MUTANT_REJECT_JDG 100415

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;
				GameObject * mutantReject = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANT_REJECT_JDG );
				if (mutantReject != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, mutantReject, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_MutantReject_JDG, "")//M11_BARRACKS_MUTANT_REJECT_JDG 100415
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			//this guys orders are to charge the player upon entry to this room
			ScriptEngine::Innate_Enable(obj);

			ActionParamsStruct params;
			params.Set_Basic(this, 100, M01_GOING_TO_HAVOC_JDG);
			params.Set_Movement( STAR, RUN, 1 );
			params.Set_Attack( STAR, 1, 0, true );
			ScriptEngine::Action_Attack ( obj, params );

			const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T19_EntryZone_JDG, "")//100404 entry to scientist guy
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_SCIENTIST_JDG 100405

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;
				GameObject * scientist = ScriptEngine::Find_Object ( M11_BARRACKS_SCIENTIST_JDG );
				if (scientist != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, scientist, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_Scientist_JDG, "")//M11_BARRACKS_SCIENTIST_JDG 100405
{
	int scientist_conv01;
	int scientist_conv02;
	int scientist_conv03;
	int scientist_conv04;
	bool deadYet;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(scientist_conv01, 1);
		SAVE_VARIABLE(scientist_conv02, 2);
		SAVE_VARIABLE(scientist_conv03, 3);
		SAVE_VARIABLE(scientist_conv04, 4);
		SAVE_VARIABLE(deadYet, 5);
	}

	void Created( GameObject * obj ) override
	{
		deadYet = false;

		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation ("H_A_HOST_L2B", true);
		ScriptEngine::Action_Play_Animation (obj, params);
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		deadYet = true;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_CONVERSATION_BEGAN || type == CUSTOM_EVENT_CONVERSATION_REMARK_STARTED || type == CUSTOM_EVENT_CONVERSATION_REMARK_ENDED)
		{
		}

		else if (param == M01_START_ACTING_JDG)
		{
			if (obj && STAR && deadYet == false)
			{
				scientist_conv01 = ScriptEngine::Create_Conversation( "M11_Barracks_M19_Conversation_01_JDG", 100, 30, true);
				ScriptEngine::Join_Conversation( obj, scientist_conv01, false, true, false );
				ScriptEngine::Join_Conversation( STAR, scientist_conv01, false, false, false );
				ScriptEngine::Start_Conversation( scientist_conv01, scientist_conv01 );

				ScriptEngine::Monitor_Conversation(  obj, scientist_conv01 );

				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation ("H_A_HOST_L2B", true);
				ScriptEngine::Action_Play_Animation (obj, params);
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			if (action_id == scientist_conv01)
			{
				if (obj && STAR && deadYet == false)
				{
					scientist_conv02 = ScriptEngine::Create_Conversation( "M11_Barracks_M19_Conversation_02_JDG", 100, 30, true);
					ScriptEngine::Join_Conversation( obj, scientist_conv02, false, true, false );
					ScriptEngine::Join_Conversation( STAR, scientist_conv02, false, false, false );
					ScriptEngine::Start_Conversation( scientist_conv02, scientist_conv02 );

					ScriptEngine::Monitor_Conversation(  obj, scientist_conv02 );

					params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
					params.Set_Animation ("H_A_HOST_L2B", true);
					ScriptEngine::Action_Play_Animation (obj, params);
				}
			}

			else if (action_id == scientist_conv02)
			{
				if (obj && STAR && deadYet == false)
				{
					params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );//stand up hands over head
					params.Set_Animation ("H_A_HOST_L2C", false);
					ScriptEngine::Action_Play_Animation (obj, params);
				}
			}


			else if (action_id == scientist_conv03)
			{
				if (obj && STAR && deadYet == false)
				{
					params.Set_Basic(this, 100, M01_WALKING_WAYPATH_02_JDG);
					params.Set_Movement( Vector3 (14.045f, -8.178f, -38.236f), WALK, 1 );
					ScriptEngine::Action_Goto( obj, params );
				}
			}

			else if (action_id == scientist_conv04)
			{
				if (obj && STAR && deadYet == false)
				{
					params.Set_Basic(this, 100, M01_WALKING_WAYPATH_03_JDG);
					params.Set_Movement( Vector3 (0.115f, -10.796f, -34.233f), RUN, 1 );
					ScriptEngine::Action_Goto( obj, params );
				}
			}
		}

		else if (action_id == M01_DOING_ANIMATION_02_JDG)
		{
			if (obj && STAR && deadYet == false)
			{
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_03_JDG );//stand up loop--one time
				params.Set_Animation ("H_A_HOST_L1B", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}
		}

		else if (action_id == M01_DOING_ANIMATION_03_JDG)
		{
			if (obj && STAR && deadYet == false)
			{
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_04_JDG );//stand up--put hands down
				params.Set_Animation ("H_A_HOST_L1C", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}
		}

		else if (action_id == M01_DOING_ANIMATION_04_JDG)
		{
			if (obj && STAR && deadYet == false)
			{
				scientist_conv03 = ScriptEngine::Create_Conversation( "M11_Barracks_M19_Conversation_03_JDG", 100, 30, true);
				ScriptEngine::Join_Conversation( obj, scientist_conv03, false, true, true );
				ScriptEngine::Join_Conversation( STAR, scientist_conv03, false, false, false );
				ScriptEngine::Start_Conversation( scientist_conv03, scientist_conv03 );

				ScriptEngine::Monitor_Conversation(  obj, scientist_conv03 );
			}
		}


		else if (action_id == M01_WALKING_WAYPATH_02_JDG)
		{
			if (obj && STAR && deadYet == false)
			{
				scientist_conv04 = ScriptEngine::Create_Conversation( "M11_Barracks_M19_Conversation_04_JDG", 100, 100, true);
				ScriptEngine::Join_Conversation( obj, scientist_conv04, false, true, true );
				ScriptEngine::Start_Conversation( scientist_conv04, scientist_conv04 );

				ScriptEngine::Monitor_Conversation(  obj, scientist_conv04 );
			}
		}

		else if (action_id == M01_WALKING_WAYPATH_03_JDG)
		{
			ScriptEngine::Set_Innate_Is_Stationary (obj, true);
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T18_EntryZone_JDG, "")//100401 this is room where black hand rags on tech
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_RAGSESSION_TECH_JDG 100402
	//M11_BARRACKS_RAGSESSION_BLACKHAND_JDG 100403

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;
				GameObject * blackhand = ScriptEngine::Find_Object ( M11_BARRACKS_RAGSESSION_BLACKHAND_JDG );
				GameObject * tech = ScriptEngine::Find_Object ( M11_BARRACKS_RAGSESSION_TECH_JDG );

				if (blackhand != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, blackhand, 0, M01_START_ACTING_JDG, 0 );
				}

				if (tech != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, tech, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_TechnicianConversation_Blackhand_JDG, "")//M11_BARRACKS_RAGSESSION_BLACKHAND_JDG 100403
{
	int rag_conv;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(rag_conv, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			GameObject * tech = ScriptEngine::Find_Object ( M11_BARRACKS_RAGSESSION_TECH_JDG );
			if (tech != nullptr)
			{
				rag_conv = ScriptEngine::Create_Conversation( "M11_Barracks_M18_Conversation_JDG", 100, 50, true);
				ScriptEngine::Join_Conversation( obj, rag_conv, false, true, true );
				ScriptEngine::Join_Conversation( tech, rag_conv, false, true, true );
				ScriptEngine::Start_Conversation( rag_conv, rag_conv );

				ScriptEngine::Monitor_Conversation(  obj, rag_conv );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			if (action_id == rag_conv)
			{
				GameObject * tech = ScriptEngine::Find_Object ( M11_BARRACKS_RAGSESSION_TECH_JDG );
				if (tech != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, tech, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
				}

				params.Set_Basic(this, 60, M01_GOING_TO_HAVOC_JDG);
				params.Set_Attack( STAR, 10, 0, true );
				ScriptEngine::Action_Attack ( obj, params );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_TechnicianConversation_Tech_JDG, "")//M11_BARRACKS_RAGSESSION_TECH_JDG 100402
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic(this, 60, M01_GOING_TO_HAVOC_JDG);
			params.Set_Attack( STAR, 10, 0, true );
			ScriptEngine::Action_Attack ( obj, params );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T14_EntryZone_JDG, "")//100399 this is the showers
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_STEALTHSOLDIER_JDG 100400

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;

				GameObject * stealthSoldier = ScriptEngine::Find_Object ( M11_BARRACKS_STEALTHSOLDIER_JDG );
				if (stealthSoldier != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, stealthSoldier, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_ShowerStealthSoldier_JDG, "")//M11_BARRACKS_STEALTHSOLDIER_JDG 100400
{
	bool killedYet;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(killedYet, 1);
	}

	void Created( GameObject * obj ) override
	{
		killedYet = false;
		ScriptEngine::Enable_Stealth (obj, false);
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Set_Loiters_Allowed(obj, false );
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		killedYet = true;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG && killedYet == false)
		{
			int showerConv = ScriptEngine::Create_Conversation( "M11_Shower_StealthGuy_Conversation", 90, 30, true);
			ScriptEngine::Join_Conversation( obj, showerConv, false, true, true );
			ScriptEngine::Start_Conversation( showerConv,  showerConv );

			ScriptEngine::Enable_Stealth (obj, true);
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T17_EntryZone_JDG, "")//100393 this is entrance to visceroid room
{
	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;

				GameObject * visceroid = ScriptEngine::Find_Object ( 100394 );
				if (visceroid != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, visceroid, 0, M01_START_ACTING_JDG, 1 );
				}

				GameObject * visceroid02 = ScriptEngine::Find_Object ( 101402 );
				if (visceroid02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, visceroid02, 0, M01_START_ACTING_JDG, 1 );
				}

				GameObject * visceroid03 = ScriptEngine::Find_Object ( 101403 );
				if (visceroid03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, visceroid03, 0, M01_START_ACTING_JDG, 1 );
				}

				GameObject * visceroid04 = ScriptEngine::Find_Object ( 101404 );
				if (visceroid04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, visceroid04, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};



DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T15_EntryZone_JDG, "")//this is a bedroom on left side 100377 2 nod soldiers talking about escaped mutants
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_MUTANTCONVERSATION_GUY01_JDG 100378
	//M11_BARRACKS_MUTANTCONVERSATION_GUY02_JDG 100379

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;
				GameObject * conversationGuy01 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTCONVERSATION_GUY01_JDG );
				GameObject * conversationGuy02 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTCONVERSATION_GUY02_JDG );
				if (conversationGuy01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, conversationGuy01, 0, M01_START_ACTING_JDG, 0 );
				}

				if (conversationGuy02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, conversationGuy02, 0, M01_START_ACTING_JDG, 0 );
				}

			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_MutantConversationGuy_01_JDG, "")//M11_BARRACKS_MUTANTCONVERSATION_GUY01_JDG 100378
{
	bool killedYet;
	int mutant_conv;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(killedYet, 1);
		SAVE_VARIABLE(mutant_conv, 2);
	}

	void Created( GameObject * obj ) override
	{
		killedYet = false;
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == CUSTOM_EVENT_CONVERSATION_BEGAN || type == CUSTOM_EVENT_CONVERSATION_REMARK_STARTED || type == CUSTOM_EVENT_CONVERSATION_REMARK_ENDED)
		{
		}

		else if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			GameObject * conversationGuy02 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTCONVERSATION_GUY02_JDG );
			if (obj && conversationGuy02 != nullptr)
			{
				mutant_conv = ScriptEngine::Create_Conversation( "M11_Barracks_M15_Conversation_JDG", 100, 30, true);
				ScriptEngine::Join_Conversation( obj, mutant_conv, true, true, true );
				ScriptEngine::Join_Conversation( conversationGuy02, mutant_conv, true, true, true );
				ScriptEngine::Start_Conversation( mutant_conv, mutant_conv );

				ScriptEngine::Monitor_Conversation(  obj, mutant_conv );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			if (action_id == mutant_conv)
			{
				GameObject * conversationGuy02 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTCONVERSATION_GUY02_JDG );
				if (conversationGuy02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, conversationGuy02, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
				}

				ScriptEngine::Action_Reset ( obj, 100 );
				params.Set_Basic(this, 60, M01_GOING_TO_HAVOC_JDG);
				params.Set_Movement( STAR, RUN, 1, false );
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_MutantConversationGuy_02_JDG, "")//M11_BARRACKS_MUTANTCONVERSATION_GUY02_JDG 100379
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			ScriptEngine::Action_Reset ( obj, 100 );
			params.Set_Basic(this, 60, M01_GOING_TO_HAVOC_JDG);
			params.Set_Movement( STAR, RUN, 1, false );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T13_EntryZone_JDG, "")//this is entry into mutant/toilet bathroom 100376
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_TOILET_MUTANT_01_JDG 100374
	//M11_BARRACKS_TOILET_MUTANT_02_JDG 100375

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;
				GameObject * mutant01 = ScriptEngine::Find_Object ( M11_BARRACKS_TOILET_MUTANT_01_JDG );
				GameObject * mutant02 = ScriptEngine::Find_Object ( M11_BARRACKS_TOILET_MUTANT_02_JDG );

				if (mutant01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, mutant01, 0, M01_START_ACTING_JDG, 0 );
				}

				if (mutant02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, mutant02, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_ToiletMutant01_JDG, "")//this guys ID is M11_BARRACKS_TOILET_MUTANT_01_JDG 100374
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation ("S_A_HUMAN.H_A_A0A0_L28B", true);
		ScriptEngine::Action_Play_Animation (obj, params);

	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ("M11DSGN_DSGN0052I1MEIN_SND", myPosition, obj );

			float delayTimer = ScriptEngine::Get_Random ( 1, 3 );
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation ("S_A_HUMAN.H_A_A0A0_L28C", false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_ToiletMutant02_JDG, "")//this guys ID is M11_BARRACKS_TOILET_MUTANT_02_JDG 100375
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation ("S_A_HUMAN.H_A_A0A0_L28B", true);
		ScriptEngine::Action_Play_Animation (obj, params);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			float delayTimer = ScriptEngine::Get_Random ( 0, 1 );
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ("M11DSGN_DSGN0051I1MEIN_SND", myPosition, obj );

			float delayTimer = ScriptEngine::Get_Random ( 1, 3 );
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation ("S_A_HUMAN.H_A_A0A0_L28C", false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_LivingQuarters_T16_EntryZone_JDG, "")//100385 this is where 2 mutants are attacking a blackhand guy
{
	//THESE ARE THE DUDES YOU CONTROL
	//M11_BARRACKS_MUTANTUPRISING_BLACKHAND_JDG 100387
	//M11_BARRACKS_MUTANTUPRISING_MUTANT01_JDG 100388
	//M11_BARRACKS_MUTANTUPRISING_MUTANT02_JDG 100389

	bool firstEntry;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(firstEntry, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		firstEntry = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (firstEntry == true)
			{
				firstEntry = false;

				GameObject * mutant01 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_MUTANT01_JDG );
				GameObject * mutant02 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_MUTANT02_JDG );
				GameObject * blackhand = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_BLACKHAND_JDG );

				if (mutant01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, mutant01, 0, M01_START_ACTING_JDG, 0 );
				}

				if (mutant02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, mutant02, 0, M01_START_ACTING_JDG, 0 );
				}

				if (blackhand != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, blackhand, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_BARRACKS_SPAWNER_CONTROLLER_JDG ), 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
		}
	}
};
/***********************************************************************************************************************
Putting main laboratory related scripts here
************************************************************************************************************************/
DECLARE_SCRIPT(M11_Laboratory_EntryZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * mutant01 = ScriptEngine::Find_Object ( 100450 );
			GameObject * mutant02 = ScriptEngine::Find_Object ( 100451 );
			GameObject * scientist01 = ScriptEngine::Find_Object ( 100449 );
			GameObject * scientist02 = ScriptEngine::Find_Object ( 100958 );

			if (mutant01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, mutant01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (mutant02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, mutant02, 0, M01_START_ACTING_JDG, 0 );
			}

			if (scientist01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, scientist01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (scientist02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, scientist02, 0, M01_START_ACTING_JDG, 0 );
			}

			ScriptEngine::Destroy_Object (obj);//one time only zone--cleaning up
		}
	}
};









DECLARE_SCRIPT(M11_Mutant_TrajectoryBone_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation(obj, false );
		ScriptEngine::Set_Animation ( obj, "X11E_Trajectory.X11E_Trajectory", false, nullptr, 0, 120, false );
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "X11E_Trajectory.X11E_Trajectory") == 0)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};


DECLARE_SCRIPT(M11_Mutant_ShatterGlass_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation(obj, false );
		ScriptEngine::Set_Animation ( obj, "X11E_Shatter.X11E_Shatter", false, nullptr, 0, 120, false );
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "X11E_Shatter.X11E_Shatter") == 0)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_Cryochamber_DestroyedSimple_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Shield_Type ( obj, "Blamo" );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_CRYOBIGD.DSP_CRYOBIGD", 0 );
		ScriptEngine::Set_Health ( obj, 0.25f );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIGD.DSP_CRYOBIGD", false, nullptr, 0, 5, false );
		ScriptEngine::Set_Health ( obj, 0.25f );
	}
};




/***********************************************************************************************************************
Putting Mutant crypt related scripts here...turn on spawner--turn off spawners etc...
************************************************************************************************************************/
DECLARE_SCRIPT(M11_Turn_On_Mutant_Crypt_Spawners_Zone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
			if (mutantController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};

DECLARE_SCRIPT(M11_Mutant_Crypt_Spawners_Controller_JDG, "")//this guys ID is M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG 100013
{
	int mutantSpawner01_guy_ID;
	int mutantSpawner02_guy_ID;
	int mutantSpawner03_guy_ID;
	int mutantSpawner04_guy_ID;
	int mutantSpawner05_guy_ID;
	int mutantSpawner06_guy_ID;

	bool okay_to_spawn;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(mutantSpawner01_guy_ID, 1);
		SAVE_VARIABLE(mutantSpawner02_guy_ID, 2);
		SAVE_VARIABLE(mutantSpawner03_guy_ID, 3);
		SAVE_VARIABLE(mutantSpawner04_guy_ID, 4);
		SAVE_VARIABLE(mutantSpawner05_guy_ID, 5);
		SAVE_VARIABLE(mutantSpawner06_guy_ID, 6);
		SAVE_VARIABLE(okay_to_spawn, 7);
	}

	void Created( GameObject * /* obj */ ) override
	{
		okay_to_spawn = false;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			switch (param)
			{
				case M01_SPAWNER_SPAWN_PLEASE_JDG: //M00_Select_Empty_Hands
					{
						okay_to_spawn = true;

						GameObject * mutantSpawner01_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_01_JDG );
						ScriptEngine::Attach_Script(mutantSpawner01_guy, "M11_MutantCrypt_Spawner02_Guy_JDG", "");
						mutantSpawner01_guy_ID = ScriptEngine::Get_ID ( mutantSpawner01_guy );
						ScriptEngine::Attach_Script(mutantSpawner01_guy, "M00_Select_Empty_Hands", "");

						GameObject * mutantSpawner02_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_02_JDG );
						ScriptEngine::Attach_Script(mutantSpawner02_guy, "M11_MutantCrypt_Spawner02_Guy_JDG", "");
						mutantSpawner02_guy_ID = ScriptEngine::Get_ID ( mutantSpawner02_guy );
						ScriptEngine::Attach_Script(mutantSpawner02_guy, "M00_Select_Empty_Hands", "");

						GameObject * mutantSpawner03_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_03_JDG );
						ScriptEngine::Attach_Script(mutantSpawner03_guy, "M11_MutantCrypt_Spawner03_Guy_JDG", "");
						mutantSpawner03_guy_ID = ScriptEngine::Get_ID ( mutantSpawner03_guy );
						ScriptEngine::Attach_Script(mutantSpawner03_guy, "M00_Select_Empty_Hands", "");

						GameObject * mutantSpawner04_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_04_JDG );
						ScriptEngine::Attach_Script(mutantSpawner04_guy, "M11_MutantCrypt_Spawner04_Guy_JDG", "");
						mutantSpawner04_guy_ID = ScriptEngine::Get_ID ( mutantSpawner04_guy );
						ScriptEngine::Attach_Script(mutantSpawner04_guy, "M00_Select_Empty_Hands", "");

						GameObject * mutantSpawner05_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_05_JDG );
						ScriptEngine::Attach_Script(mutantSpawner05_guy, "M11_MutantCrypt_Spawner05_Guy_JDG", "");
						mutantSpawner05_guy_ID = ScriptEngine::Get_ID ( mutantSpawner05_guy );
						ScriptEngine::Attach_Script(mutantSpawner05_guy, "M00_Select_Empty_Hands", "");

						GameObject * mutantSpawner06_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_06_JDG );
						ScriptEngine::Attach_Script(mutantSpawner06_guy, "M11_MutantCrypt_Spawner06_Guy_JDG", "");
						mutantSpawner06_guy_ID = ScriptEngine::Get_ID ( mutantSpawner06_guy );
						ScriptEngine::Attach_Script(mutantSpawner06_guy, "M00_Select_Empty_Hands", "");
					}
					break;

				case M01_QUIT_SPAWNING_PLEASE_JDG:
					{
						okay_to_spawn = false;
					}
					break;

				case M01_SPAWNER_IS_DEAD_JDG:
					{
						if (okay_to_spawn == true)
						{
							GameObject * mutantSpawner01_guy = ScriptEngine::Find_Object ( mutantSpawner01_guy_ID );
							GameObject * mutantSpawner02_guy = ScriptEngine::Find_Object ( mutantSpawner02_guy_ID );
							GameObject * mutantSpawner03_guy = ScriptEngine::Find_Object ( mutantSpawner03_guy_ID );
							GameObject * mutantSpawner04_guy = ScriptEngine::Find_Object ( mutantSpawner04_guy_ID );
							GameObject * mutantSpawner05_guy = ScriptEngine::Find_Object ( mutantSpawner05_guy_ID );
							GameObject * mutantSpawner06_guy = ScriptEngine::Find_Object ( mutantSpawner06_guy_ID );

							if (sender == mutantSpawner01_guy)
							{
								GameObject * newMutantSpawner01_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_01_JDG );
								ScriptEngine::Attach_Script(newMutantSpawner01_guy, "M11_MutantCrypt_Spawner02_Guy_JDG", "");
								mutantSpawner01_guy_ID = ScriptEngine::Get_ID ( newMutantSpawner01_guy );
								ScriptEngine::Attach_Script(mutantSpawner01_guy, "M00_Select_Empty_Hands", "");
							}

							else if (sender == mutantSpawner02_guy)
							{
								GameObject * newMutantSpawner02_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_02_JDG );
								ScriptEngine::Attach_Script(newMutantSpawner02_guy, "M11_MutantCrypt_Spawner02_Guy_JDG", "");
								mutantSpawner02_guy_ID = ScriptEngine::Get_ID ( newMutantSpawner02_guy );
								ScriptEngine::Attach_Script(mutantSpawner02_guy, "M00_Select_Empty_Hands", "");
							}

							else if (sender == mutantSpawner03_guy)
							{
								GameObject * newMutantSpawner03_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_03_JDG );
								ScriptEngine::Attach_Script(newMutantSpawner03_guy, "M11_MutantCrypt_Spawner03_Guy_JDG", "");
								mutantSpawner03_guy_ID = ScriptEngine::Get_ID ( newMutantSpawner03_guy );
								ScriptEngine::Attach_Script(mutantSpawner03_guy, "M00_Select_Empty_Hands", "");
							}

							else if (sender == mutantSpawner04_guy)
							{
								GameObject * newMutantSpawner04_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_04_JDG );
								ScriptEngine::Attach_Script(newMutantSpawner04_guy, "M11_MutantCrypt_Spawner04_Guy_JDG", "");
								mutantSpawner04_guy_ID = ScriptEngine::Get_ID ( newMutantSpawner04_guy );
								ScriptEngine::Attach_Script(mutantSpawner04_guy, "M00_Select_Empty_Hands", "");
							}

							else if (sender == mutantSpawner05_guy)
							{
								GameObject * newMutantSpawner05_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_05_JDG );
								ScriptEngine::Attach_Script(newMutantSpawner05_guy, "M11_MutantCrypt_Spawner05_Guy_JDG", "");
								mutantSpawner05_guy_ID = ScriptEngine::Get_ID ( newMutantSpawner05_guy );
								ScriptEngine::Attach_Script(mutantSpawner05_guy, "M00_Select_Empty_Hands", "");
							}

							else if (sender == mutantSpawner06_guy)
							{
								GameObject * newMutantSpawner06_guy = ScriptEngine::Trigger_Spawner( M11_MUTANT_CRYPT_SPAWNER_06_JDG );
								ScriptEngine::Attach_Script(newMutantSpawner06_guy, "M11_MutantCrypt_Spawner06_Guy_JDG", "");
								mutantSpawner06_guy_ID = ScriptEngine::Get_ID ( newMutantSpawner06_guy );
								ScriptEngine::Attach_Script(mutantSpawner06_guy, "M00_Select_Empty_Hands", "");
							}
						}
					}
					break;
			}
		}
	}
};

DECLARE_SCRIPT(M11_MutantCrypt_Spawner06_Guy_JDG, "")//this one has been fixed for wacky findpath bug
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
		ScriptEngine::Select_Weapon(obj, nullptr );
		myState = IDLE;
		float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );

		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 0 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		myState = DEAD;

		GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
		if (mutantController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (obj && enemy == STAR && myState == IDLE)//
		{
			myState = ATTACKING;
			const char* soundName = M11_Choose_Mutant_Alerted_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );

			ScriptEngine::Create_Logical_Sound (obj, M11_MUTANT_IS_NEARBY_JDG, myPosition, 50 );
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if ( static_cast<int>(sound.Type) == M11_MUTANT_IS_NEARBY_JDG )
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			if (obj && myState == IDLE)
			{
				const char* soundName = M11_Choose_Mutant_Idle_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}

			else if (obj && myState != DEAD)
			{
				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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
			params.Set_Movement( Vector3(66.505f, 47.147f, -62.686f), .1f, 1 );

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
				params.Set_Basic(this, 80, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement( Vector3(0,0,0), .1f, 1 );

				int random = ScriptEngine::Get_Random_Int(0, 3);

				if (random == 0)
				{
					params.WaypathID = 100021;
					params.WaypointStartID = 100022;
					params.WaypointEndID = 100028;
				}

				else if (random == 1)
				{
					params.WaypathID = 100021;
					params.WaypointStartID = 100028;
					params.WaypointEndID = 100022;
				}

				else
				{
					params.WaypathID = 100030;
					params.WaypointStartID = 100031;
					params.WaypointEndID = 100040;
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
				params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
				params.Set_Movement( Vector3(66.505f, 47.147f, -62.686f), .1f, 1 );

				ScriptEngine::Action_Goto( obj, params );
			}

			else if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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

DECLARE_SCRIPT(M11_MutantCrypt_Spawner05_Guy_JDG, "")
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
		ScriptEngine::Select_Weapon(obj, nullptr );
		myState = IDLE;
		float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 0 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		myState = DEAD;

		GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
		if (mutantController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (obj && enemy == STAR && myState == IDLE)//
		{
			myState = ATTACKING;
			const char* soundName = M11_Choose_Mutant_Alerted_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );

			ScriptEngine::Create_Logical_Sound (obj, M11_MUTANT_IS_NEARBY_JDG, myPosition, 50 );
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if ( static_cast<int>(sound.Type) == M11_MUTANT_IS_NEARBY_JDG )
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			if (obj && myState == IDLE)
			{
				const char* soundName = M11_Choose_Mutant_Idle_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}

			else if (obj && myState != DEAD)
			{
				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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
			params.Set_Movement( Vector3(27.367f, 18.382f, -63.748f), .1f, 1 );

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
				params.Set_Basic(this, 80, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement( Vector3(0,0,0), .1f, 1 );

				int random = ScriptEngine::Get_Random_Int(0, 4);

				if (random == 0)
				{
					params.WaypathID = 100041;
					params.WaypointStartID = 100042;
					params.WaypointEndID = 100049;
				}

				else if (random == 1)
				{
					params.WaypathID = 100041;
					params.WaypointStartID = 100049;
					params.WaypointEndID = 100042;
				}

				else if (random == 3)
				{
					params.WaypathID = 100051;
					params.WaypointStartID = 100052;
					params.WaypointEndID = 100061;
				}

				else
				{
					params.WaypathID = 100051;
					params.WaypointStartID = 100061;
					params.WaypointEndID = 100052;
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
				params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
				params.Set_Movement( Vector3(27.367f, 18.382f, -63.748f), .1f, 1 );

				ScriptEngine::Action_Goto( obj, params );
			}

			else if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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

DECLARE_SCRIPT(M11_MutantCrypt_Spawner04_Guy_JDG, "")
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
		ScriptEngine::Select_Weapon(obj, nullptr );
		myState = IDLE;
		float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );

		ActionParamsStruct params;
		params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
		params.Set_Movement( Vector3(5.531f, -10.830f, -63.749f), .1f, 1 );

		ScriptEngine::Action_Goto( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		myState = DEAD;

		GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
		if (mutantController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (obj && enemy == STAR && myState == IDLE)//
		{
			myState = ATTACKING;
			const char* soundName = M11_Choose_Mutant_Alerted_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );

			ScriptEngine::Create_Logical_Sound (obj, M11_MUTANT_IS_NEARBY_JDG, myPosition, 50 );
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if ( static_cast<int>(sound.Type) == M11_MUTANT_IS_NEARBY_JDG )
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			if (obj && myState == IDLE)
			{
				const char* soundName = M11_Choose_Mutant_Idle_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}

			else if (obj && myState != DEAD)
			{
				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 0, 10 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}
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
				params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
				params.Set_Movement( Vector3(0,0,0), .1f, 1 );
				params.WaypathID = 100063;

				ScriptEngine::Action_Goto( obj, params );
			}

			else if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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

DECLARE_SCRIPT(M11_MutantCrypt_Spawner03_Guy_JDG, "")
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
		ScriptEngine::Select_Weapon(obj, nullptr );
		myState = IDLE;
		float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_PICK_A_NEW_LOCATION_JDG, 0 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		myState = DEAD;

		GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
		if (mutantController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy )  override
	{
		if (obj && enemy == STAR && myState == IDLE)//
		{
			myState = ATTACKING;
			const char* soundName = M11_Choose_Mutant_Alerted_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );

			ScriptEngine::Create_Logical_Sound (obj, M11_MUTANT_IS_NEARBY_JDG, myPosition, 50 );
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if ( static_cast<int>(sound.Type) == M11_MUTANT_IS_NEARBY_JDG )
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			if (obj && myState == IDLE)
			{
				const char* soundName = M11_Choose_Mutant_Idle_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}

			else if (obj && myState != DEAD)
			{
				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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
				params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
				params.Set_Movement( Vector3(-15.128f, 17.965f, -63.748f), .1f, 1 );

				ScriptEngine::Action_Goto( obj, params );
			}

			else if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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

DECLARE_SCRIPT(M11_MutantCrypt_Spawner02_Guy_JDG, "")//this script goes on mutant 1 and 2--just hunting player with this guy
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
		ScriptEngine::Select_Weapon(obj, nullptr );
		myState = IDLE;
		float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );

		ActionParamsStruct params;
		params.Set_Basic(this, 80, M01_PICK_A_NEW_LOCATION_JDG);
		params.Set_Movement( STAR, .1f, 1 );

		ScriptEngine::Action_Goto( obj, params );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		myState = DEAD;

		GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
		if (mutantController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (obj && enemy == STAR && myState == IDLE)//
		{
			myState = ATTACKING;
			const char* soundName = M11_Choose_Mutant_Alerted_Sound ( );
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( soundName, myPosition, obj );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );

			ScriptEngine::Create_Logical_Sound (obj, M11_MUTANT_IS_NEARBY_JDG, myPosition, 50 );
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if ( static_cast<int>(sound.Type) == M11_MUTANT_IS_NEARBY_JDG )
		{
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			if (obj && myState == IDLE)
			{
				const char* soundName = M11_Choose_Mutant_Idle_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 10, 20 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}

			else if (obj && myState != DEAD)
			{
				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );

				float delayTimer = ScriptEngine::Get_Random ( 0, 10 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, delayTimer );
			}
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
			if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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

/***********************************************************************************************************************
Putting POWERCORE related scripts here
************************************************************************************************************************/
DECLARE_SCRIPT(M11_PowerCore_WanderingMutant_JDG, "")//
{
	void Created( GameObject * obj ) override
	{

		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		GameObject * myTarget = ScriptEngine::Find_Closest_Soldier ( myPosition, 0, 50, true );

		if (myTarget != nullptr)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, 95, M01_WALKING_WAYPATH_01_JDG );
			params.Set_Movement( myTarget, RUN, .25f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}
};

DECLARE_SCRIPT(M11_PowerCore_WanderingMutantSpawner_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 30 );
		ActionParamsStruct params;

		params.Set_Basic( this, 100, M01_HUNT_THE_PLAYER_JDG );
		params.Set_Movement( STAR, 1.0f, 1.0f );

		ScriptEngine::Action_Goto( obj, params );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (obj && param == M01_HUNT_THE_PLAYER_JDG)
		{
			if (STAR)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 100, M01_HUNT_THE_PLAYER_JDG);
				params.Set_Movement(STAR, 1.0f, 1);

				ScriptEngine::Action_Goto ( obj, params );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_HUNT_THE_PLAYER_JDG, 1 );
			}
		}

		else if (obj && param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			ScriptEngine::Enable_Hibernation( obj, true );
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (obj && action_id == M01_HUNT_THE_PLAYER_JDG)
			{
				if (STAR)
				{
					const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
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

DECLARE_SCRIPT(M11_PowerCore_StealthSoldier_Group2_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		ScriptEngine::Enable_Stealth(obj, true);
		ScriptEngine::Innate_Disable(obj);
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if (sound.Creator == STAR)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ScriptEngine::Innate_Enable(obj);
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 powerupSpawnLocation = ScriptEngine::Get_Position (obj );
		powerupSpawnLocation.Z += 0.75f;

		static constexpr const char *powerups[2] =
		{
			//this is a list of potential powerups to be dropped by destroyed security turrets
			"POW_Health_100",
			"POW_Armor_100",
		};
		int random = ScriptEngine::Get_Random_Int(0, 2);

		ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
	}
};

DECLARE_SCRIPT(M11_PowerCore_StealthSoldier_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic(this, 100, M01_HUNT_THE_PLAYER_JDG);
		params.Set_Movement(STAR, RUN, 10);

		ScriptEngine::Action_Goto ( obj, params );
	}
};

DECLARE_SCRIPT(M11_PowerCore_Initial_StealthSoldier_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Enable_Stealth(obj, false);
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (enemy == STAR)
		{
			ScriptEngine::Enable_Stealth(obj, true);
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Sound ( "CnC_Cloaking", myPosition, obj );

			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}
};

DECLARE_SCRIPT(M11_PowerCore_Powerup_Zone_JDG, "")//
{
	bool spawnClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(spawnClearance, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		spawnClearance = true;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && spawnClearance == true)
		{
			spawnClearance = false;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 15 );

			float delayTimer = ScriptEngine::Get_Random ( 0, 5 );
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, delayTimer );

		}
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			spawnClearance = true;
		}

		else if (param == M01_SPAWNER_SPAWN_PLEASE_JDG)//give powerup
		{
			Vector3 powerupSpawnLocation (-5.568f, 57.285f, -38.736f);

			static constexpr const char *powerups[9] =
			{
				//this is a list of potential powerups to be dropped by destroyed security turrets
				"POW_Health_100",
				"POW_Armor_100",
				"POW_RocketLauncher_Player",
				"POW_SniperRifle_Player",
				"POW_TiberiumFlechetteGun_Player",
				"POW_LaserChaingun_Player",
				"POW_GrenadeLauncher_Player",
				"POW_LaserRifle_Player",
				"POW_TiberiumAutoRifle_Player",
				//more to be added later

			};
			int random = ScriptEngine::Get_Random_Int(0, 9);

			ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
		}
	}
};

DECLARE_SCRIPT(M11_PowerCore_EntryZone_JDG, "")
{
	//THESE ARE THE GUYS YOU CONTROL
	//M11_POWERCORE_INITAL_STEALTHGUY01_JDG		100536
	//M11_POWERCORE_INITAL_STEALTHGUY02_JDG		100537
	//M11_POWERCORE_INITAL_STEALTHGUY03_JDG		100538

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			//deleting the two "There's Sydney!" zones once entered into powercore
			GameObject * zone01 = ScriptEngine::Find_Object ( 101327 );
			GameObject * zone02 = ScriptEngine::Find_Object ( 101328 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}

			GameObject * stealthGuy01 = ScriptEngine::Find_Object ( M11_POWERCORE_INITAL_STEALTHGUY01_JDG );
			GameObject * stealthGuy02 = ScriptEngine::Find_Object ( M11_POWERCORE_INITAL_STEALTHGUY02_JDG );
			GameObject * stealthGuy03 = ScriptEngine::Find_Object ( M11_POWERCORE_INITAL_STEALTHGUY03_JDG );

			GameObject * stealthGuy04 = ScriptEngine::Find_Object ( 100662 );
			GameObject * stealthGuy05 = ScriptEngine::Find_Object ( 100663 );
			GameObject * stealthGuy06 = ScriptEngine::Find_Object ( 100664 );
			GameObject * stealthGuy07 = ScriptEngine::Find_Object ( 100665 );
			GameObject * stealthGuy08 = ScriptEngine::Find_Object ( 100666 );
			GameObject * stealthGuy09 = ScriptEngine::Find_Object ( 100667 );

			if (stealthGuy01 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy01, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy02 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy02, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy03 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy03, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy04 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy04, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy05 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy05, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy06 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy06, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy07 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy07, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy08 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy08, 0, M01_START_ACTING_JDG, 0 );
			}

			if (stealthGuy09 != nullptr)
			{
				ScriptEngine::Send_Custom_Event( obj, stealthGuy09, 0, M01_START_ACTING_JDG, 0 );
			}

			Vector3 mutant01spot (6.339f, 64.212f, -52.114f);
			Vector3 mutant02spot (4.095f, 76.237f, -50.782f);
			Vector3 mutant03spot (-6.129f, 73.340f, -52.114f);

			GameObject * mutant01 = ScriptEngine::Create_Object ( "Mutant_0_Mutant", mutant01spot );
			ScriptEngine::Attach_Script(mutant01, "M11_PowerCore_WanderingMutant_JDG", "");

			GameObject * mutant02 = ScriptEngine::Create_Object ( "Mutant_0_Mutant", mutant02spot );
			ScriptEngine::Attach_Script(mutant02, "M11_PowerCore_WanderingMutant_JDG", "");

			GameObject * mutant03 = ScriptEngine::Create_Object ( "Mutant_0_Mutant", mutant03spot );
			ScriptEngine::Attach_Script(mutant03, "M11_PowerCore_WanderingMutant_JDG", "");

			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};
/***********************************************************************************************************************
Putting SYNDEY Escort related stuff here
************************************************************************************************************************/


DECLARE_SCRIPT(M11_NukeSilo_Protector_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 3 );
	}
};


/***********************************************************************************************************************
Putting OBJECTIVE related stuff here--zones etc.
************************************************************************************************************************/

DECLARE_SCRIPT(M11_End_First_Objective_Zone_JDG, "")//player has reached museum area--clear first objective
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_MISSION_CONTROLLER_JDG), 0, M11_END_FIRST_OBJECTIVE_JDG, 0 );//

			GameObject * cabalController = ScriptEngine::Find_Object (101204);
			if (cabalController != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
				ScriptEngine::Send_Custom_Event ( obj, cabalController, 0, M01_START_ACTING_JDG, delayTimer );
			}
			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};

DECLARE_SCRIPT(M11_Start_Third_Objective_Zone_JDG, "")//player's third objective is to acess the power core
{
	int addThirdObjectiveConv;
	bool entered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(addThirdObjectiveConv, 1);
		SAVE_VARIABLE(entered, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		entered = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && entered == false)
		{
			entered = true;
			addThirdObjectiveConv = ScriptEngine::Create_Conversation( "M11_Add_Third_Objective_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( nullptr, addThirdObjectiveConv, false, false, true );;
			ScriptEngine::Start_Conversation( addThirdObjectiveConv,  addThirdObjectiveConv );
			ScriptEngine::Monitor_Conversation (obj, addThirdObjectiveConv);
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == addThirdObjectiveConv) //introduce acess powercore objective here
					{
						ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_MISSION_CONTROLLER_JDG), 0, M11_ADD_THIRD_OBJECTIVE_JDG, 0 );
						ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_Start_Fifth_Objective_Zone_JDG, "")//player's fifth objective is to sabotage the nuke
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_MISSION_CONTROLLER_JDG), 0, M11_ADD_FIFTH_OBJECTIVE_JDG, 0 );

			GameObject * zoneOne = ScriptEngine::Find_Object ( 100105 );
			if (zoneOne != nullptr)
			{
				ScriptEngine::Destroy_Object ( zoneOne );//one time only zone--cleaning up
			}

			GameObject * zoneTwo = ScriptEngine::Find_Object ( 100107 );
			if (zoneTwo != nullptr)
			{
				ScriptEngine::Destroy_Object ( zoneTwo );//one time only zone--cleaning up
			}
		}
	}
};



DECLARE_SCRIPT(M11_End_Mission_Switch_JDG, "")//this guys ID is M11_END_MISSION_SWITCH_JDG 100106
{
	float myHealth;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(myHealth, 1);
	}

	void Created( GameObject * obj ) override
	{
		myHealth = ScriptEngine::Get_Health ( obj );
		ScriptEngine::Set_Shield_Type (obj, "Blamo");
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		ScriptEngine::Mission_Complete ( false );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (obj)
		{
			ScriptEngine::Set_Health ( obj, myHealth );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Set_Animation ( obj, "DSP_MINICONSEL.DSP_MINICONSEL", false, nullptr, 0, 13, false );
		}
	}
};

DECLARE_SCRIPT(M11_KanesRoom_SecurityTurret_JDG, "")
{
	int alarmSound;
	bool active;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(alarmSound, 1);
		SAVE_VARIABLE(active, 2);
	}

	void Created( GameObject * obj ) override
	{
		active = false;
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );

		alarmSound = ScriptEngine::Create_Sound ( "SFX.Klaxon_Alert_01", myPosition, obj );
		ScriptEngine::Stop_Sound ( alarmSound, false );

		ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_GOTO_IDLE_JDG, 1 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		if (active == true)
		{
			ScriptEngine::Stop_Sound ( alarmSound, true );

			Vector3 powerupSpawnLocation = ScriptEngine::Get_Position (obj);
			powerupSpawnLocation.Z = -28.705f;

			static constexpr const char* powerups[9] =
			{
				//this is a list of potential powerups to be dropped by destroyed security turrets
				"POW_Health_100",
				"POW_Armor_100",
				"POW_RocketLauncher_Player",
				"POW_SniperRifle_Player",
				"POW_TiberiumFlechetteGun_Player",
				"POW_LaserChaingun_Player",
				"POW_GrenadeLauncher_Player",
				"POW_LaserRifle_Player",
				"POW_TiberiumAutoRifle_Player",
				//more to be added later

			};
			int random = ScriptEngine::Get_Random_Int(0, 9);

			ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
		}

		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_KANESROOM_SECURITY_CONTROLLER_JDG), 0, M01_YOUR_OPERATOR_IS_DEAD_JDG, 0 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (param == M01_START_ACTING_JDG)
		{
			if (obj)
			{
				active = true;
				float myAccuracy = ScriptEngine::Get_Random ( 0, 1 );

				params.Set_Basic(this, 100, M01_START_ATTACKING_01_JDG);
				params.Set_Attack( STAR, 100, myAccuracy, true );
				ScriptEngine::Action_Attack( obj, params );

				ScriptEngine::Start_Sound ( alarmSound  );
			}
		}
	}
};

DECLARE_SCRIPT(M11_KanesRoom_SecurityZone_JDG, "")//this guys ID is M11_KANESROOM_SECURITY_CONTROLLER_JDG 100351
{
	bool entered;
	int deadTurrets;
	int deadCameras;
	bool powerupsGranted;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(entered, 1);
		SAVE_VARIABLE(deadTurrets, 2);
		SAVE_VARIABLE(deadCameras, 3);
		SAVE_VARIABLE(powerupsGranted, 4);
	}

	void Created( GameObject * /* obj */ ) override
	{
		entered = false;
		powerupsGranted = false;
		deadTurrets = 0;
		deadCameras = 0;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && entered == false)
		{
			entered = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_START_ACTING_JDG, 0 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			GameObject * turret01 = ScriptEngine::Find_Object (M11_KANESROOM_SECURITY_TURRET_01_JDG);
			GameObject * turret03 = ScriptEngine::Find_Object (M11_KANESROOM_SECURITY_TURRET_03_JDG);

			if (turret01 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 0, 2 );
				ScriptEngine::Send_Custom_Event( obj, turret01, 0, M01_START_ACTING_JDG, delayTimer );
			}

			if (turret03 != nullptr)
			{
				float delayTimer = ScriptEngine::Get_Random ( 0, 2 );
				ScriptEngine::Send_Custom_Event( obj, turret03, 0, M01_START_ACTING_JDG, delayTimer );
			}
		}

		else if (param == M01_YOUR_OPERATOR_IS_DEAD_JDG)//this is custom recieved when a turret gets destroyed
		{
			deadTurrets++;

			if (deadTurrets >= 2)
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
			}
		}

		else if (param == M01_SPAWNER_SPAWN_PLEASE_JDG && powerupsGranted == false)
		{
			powerupsGranted = true;
			Vector3 health100_spawnLocation (27.571f, 7.687f, -29.205f);
			Vector3 armor100_spawnLocation (18.841f, 7.694f, -29.205f);

			ScriptEngine::Create_Object ( "POW_Health_100", health100_spawnLocation );
			ScriptEngine::Create_Object ( "POW_Armor_100", armor100_spawnLocation );
		}
	}
};

DECLARE_SCRIPT(M00_MutantHeal_Script_JDG, "")
{
	float myLastHealth;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(myLastHealth, 1);
	}

	void Created( GameObject * obj ) override
	{
		myLastHealth = ScriptEngine::Get_Health ( obj );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (obj)
		{
			float myCurrentHealth = ScriptEngine::Get_Health ( obj );

			if (myCurrentHealth > myLastHealth)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation( "H_A_4243", false );
				ScriptEngine::Action_Play_Animation ( obj, params );

				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( "Mutant_Heal_Cry", myPosition, obj );
			}

			myLastHealth = myCurrentHealth;
		}
	}
};

DECLARE_SCRIPT(M11_Lab_Cryochamber_Switch01_JDG, "")
{
	bool poked;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(poked, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 0 );//green
		poked = false;
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if ( poker == STAR && poked == false)
		{
			ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
			poked = true;
			ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
			//ScriptEngine::Create_Sound ( "bombit1", Vector3 (0,0,0), obj );

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}
};





DECLARE_SCRIPT(M11_Laboratory_Scientist_JDG, "")
{
	bool hiding;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(hiding, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
		hiding = false;
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (obj)
		{
			if (damager == STAR)
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
			}
		}
	}

	void Sound_Heard( GameObject * obj, const CombatSound & sound ) override
	{
		if (obj)
		{
			if (sound.Creator == STAR)
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
			}
		}
	}

	void Enemy_Seen( GameObject * obj, GameObject * enemy ) override
	{
		if (obj)
		{
			if (enemy == STAR)
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ScriptEngine::Innate_Enable(obj);

				params.Set_Basic(this, 85, M01_START_ACTING_JDG);
				params.Set_Animation ("H_A_CON2", true);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG && hiding == false)
			{
				hiding = true;
				ScriptEngine::Action_Reset ( obj, 100 );

				int myIdNumber = ScriptEngine::Get_ID ( obj );

				if (myIdNumber == 100958)
				{
					params.Set_Basic(this, 100, M01_MODIFY_YOUR_ACTION_JDG);
					params.Set_Movement( Vector3 (-20.306f, 17.693f, -39.486f), RUN, 1 );
					ScriptEngine::Action_Goto (obj, params);
				}

				else if (myIdNumber == 100449)
				{
					params.Set_Basic(this, 100, M01_MODIFY_YOUR_ACTION_JDG);
					params.Set_Movement( Vector3 (-3.576f, 29.472f, -39.482f), RUN, 1 );
					ScriptEngine::Action_Goto (obj, params);
				}
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		if (action_id == M01_MODIFY_YOUR_ACTION_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic(this, 100, M01_START_ACTING_JDG);
			params.Set_Animation ("H_A_A0C0", true);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
	}
};

DECLARE_SCRIPT(M11_Laboratory_Mutant_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		Vector3 myPosition = ScriptEngine::Get_Position ( obj );
		ScriptEngine::Set_Innate_Soldier_Home_Location ( obj, myPosition, 10 );
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
	}
};

DECLARE_SCRIPT(M11_Seths_Room_Conversation_Zone_JDG, "")//101103
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			int sethConv = ScriptEngine::Create_Conversation( "M11_Kane_Regarding_Seth_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( nullptr, sethConv, false, false, true );
			ScriptEngine::Join_Conversation( STAR, sethConv, false, false, true );
			ScriptEngine::Start_Conversation( sethConv,  sethConv );

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_CeilingRepellers_Controller_JDG, "")//M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG 101125
{
	int spawned_units;
	int rope_A_id;
	int rope_B_id;
	Vector3 ropeA_v3;
	Vector3 ropeB_v3;

	bool rope_A_spawned;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(spawned_units, 1);
		SAVE_VARIABLE(rope_A_id, 2);
		SAVE_VARIABLE(rope_B_id, 3);
		SAVE_VARIABLE(ropeA_v3, 4);
		SAVE_VARIABLE(ropeB_v3, 5);
		SAVE_VARIABLE(rope_A_spawned, 6);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		rope_A_spawned = false;
		spawned_units = 0;
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 5 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, 15 );
		ropeA_v3.Set (-6.230f, 3.584f, 4.5f);
		ropeB_v3.Set (8.368f, -35.439f, 1);
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", ropeB_v3);
				ScriptEngine::Set_Facing ( controller, -10 );
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_Repel_Part1b.txt");
				spawned_units++;
			}

			else if (param == M01_START_ACTING_JDG)
			{
				rope_A_spawned = true;
				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", ropeA_v3);
				ScriptEngine::Set_Facing ( controller, 75 );
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_Repel_Part1.txt");
				spawned_units++;
			}

			else if (param == M01_ROPE_IS_SENDING_ID_JDG)
			{
				rope_A_id = ScriptEngine::Get_ID ( sender );
			}

			else if (param == M01_WAYPATH_IS_SENDING_ID_JDG)//this is actually rope-b sending id
			{
				rope_B_id = ScriptEngine::Get_ID ( sender );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				GameObject * rope_A = ScriptEngine::Find_Object ( rope_A_id );
				if (rope_A != nullptr)
				{
					GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", ropeA_v3);
					ScriptEngine::Set_Facing ( controller, 75 );
					ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_Repel_Part2.txt");
					ScriptEngine::Set_Animation ( rope_A, "X11D_Rope.X11D_Rope_L", false, nullptr, 0.0f, -1.0f, false);

					spawned_units++;

					ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_04_JDG, (95/30) );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)
			{
				GameObject * rope_B = ScriptEngine::Find_Object ( rope_B_id );
				if (rope_B != nullptr)
				{
					GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", ropeB_v3);
					ScriptEngine::Set_Facing ( controller, -10 );
					ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_Repel_Part2.txt");
					ScriptEngine::Set_Animation ( rope_B, "X11D_Rope.X11D_Rope_L", true, nullptr, 0.0f, -1.0f, false);

					spawned_units++;

					ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_05_JDG, (95/30) );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//set rope "A" to idle anim
			{
				GameObject * rope_A = ScriptEngine::Find_Object ( rope_A_id );
				if (rope_A != nullptr)
				{
					ScriptEngine::Set_Animation ( rope_A, "X11D_Rope.X11D_Rope_Z", true, nullptr, 0.0f, -1.0f, false);
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_05_JDG)//set rope "B" to idle anim
			{
				GameObject * rope_B = ScriptEngine::Find_Object ( rope_B_id );
				if (rope_B != nullptr)
				{
					ScriptEngine::Set_Animation ( rope_B, "X11D_Rope.X11D_Rope_Z", true, nullptr, 0.0f, -1.0f, false);
				}
			}

			else if (param == M01_IVE_BEEN_KILLED_JDG && spawned_units <= 8 )
			{
				int random = ScriptEngine::Get_Random_Int(0, 2);

				if (random == 0 && rope_A_spawned == true)
				{
					float delayTimer = ScriptEngine::Get_Random ( 5, 15 );
					ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}

				else
				{
					float delayTimer = ScriptEngine::Get_Random ( 5, 15 );
					ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, delayTimer );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_CeilingRepellers_RopeA_JDG, "")
{
	void Created( GameObject * obj ) override //M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		GameObject * myController = ScriptEngine::Find_Object ( M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, myController, 0, M01_ROPE_IS_SENDING_ID_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_CeilingRepellers_RopeB_JDG, "")
{
	void Created( GameObject * obj ) override //M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		GameObject * myController = ScriptEngine::Find_Object ( M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, myController, 0, M01_WAYPATH_IS_SENDING_ID_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_CeilingRepeller_JDG, "")
{
	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "S_a_Human.H_a_X11D_repel") == 0)
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG","");
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * myController = ScriptEngine::Find_Object ( M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, myController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Theres_Sydney_Zone_JDG, "")//101327 and 101328
{
	void Entered( GameObject * /* obj */, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			int theresSydneyConversation = ScriptEngine::Create_Conversation( "M11_Theres_Sydney_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( STAR, theresSydneyConversation, false, false, false );
			ScriptEngine::Join_Conversation( nullptr, theresSydneyConversation, false, false, false );
			ScriptEngine::Start_Conversation( theresSydneyConversation,  theresSydneyConversation );

			GameObject * zone01 = ScriptEngine::Find_Object ( 101327 );
			GameObject * zone02 = ScriptEngine::Find_Object ( 101328 );

			if (zone01 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone01 );
			}

			if (zone02 != nullptr)
			{
				ScriptEngine::Destroy_Object ( zone02 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_End_Third_Objective_Zone_JDG, "")//this guys ID is 100012--using to stop mutant spawners and end objective 3
{
	bool entered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(entered, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		entered = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override //
	{
		if (enterer == STAR && entered == false)
		{
			entered = true;

			int powerCoreEntry = ScriptEngine::Create_Conversation( "M11_PowerCore_Entry_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( nullptr, powerCoreEntry, false, false, false );
			ScriptEngine::Start_Conversation( powerCoreEntry,  powerCoreEntry );

			GameObject * mutantController = ScriptEngine::Find_Object ( M11_MUTANT_CRYPT_SPAWNER_CONTROLLER_JDG );
			if (mutantController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, mutantController, 0, M01_QUIT_SPAWNING_PLEASE_JDG, 0 );
			}

			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_MISSION_CONTROLLER_JDG), 0, M11_END_THIRD_OBJECTIVE_JDG, 0 );
			//one time only zone--Destroying it in mission controller to avoid potential bugs
		}
	}
};

DECLARE_SCRIPT(M11_SecurityCamera_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		Vector3 playersPosition = ScriptEngine::Get_Position ( STAR );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_START_ATTACKING_02_JDG );
		params.Set_Face_Location( playersPosition, 1 );

		ScriptEngine::Action_Face_Location ( obj, params );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_NORMAL:
				{
					if (action_id == M01_START_ATTACKING_02_JDG)
					{
						Vector3 playersPosition = ScriptEngine::Get_Position ( STAR );
						ActionParamsStruct params;
						params.Set_Basic( this, 100, M01_START_ATTACKING_02_JDG );
						params.Set_Face_Location( playersPosition, 1 );

						ScriptEngine::Action_Face_Location ( obj, params );
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_Temple_Commander_JDG, "")//101250
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );

		GameObject * hologram01 = ScriptEngine::Find_Object ( 100252 );
		if (hologram01 != nullptr)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 80, M01_START_ATTACKING_01_JDG );
			params.Set_Attack( hologram01, 0, 0, true );

			ScriptEngine::Action_Attack ( obj, params );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)//conversation is over--goto innate
			{
				ScriptEngine::Action_Reset ( obj, 100 );
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * c130Controller = ScriptEngine::Find_Object ( M11_C130_DROPOFF_CONTROLLER_JDG );
		if (c130Controller != nullptr)
		{
			ScriptEngine::Destroy_Object ( c130Controller );
		}

		GameObject * hologramController = ScriptEngine::Find_Object ( 101251 );
		if (hologramController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, hologramController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Temple_Hologram_Controller_JDG, "")//101251
{
	int kane01_ID;
	int kane02_ID;
	int kane_conversation01;
	int kane_conversation01b;
	int kane_conversation02;

	bool kane_conversation01_playing;
	bool kane_conversation02_playing;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(kane01_ID, 1);
		SAVE_VARIABLE(kane02_ID, 2);
		SAVE_VARIABLE(kane_conversation01, 3);
		SAVE_VARIABLE(kane_conversation01b, 4);
		SAVE_VARIABLE(kane_conversation02, 5);
		SAVE_VARIABLE(kane_conversation01_playing, 6);
		SAVE_VARIABLE(kane_conversation02_playing, 7);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		kane_conversation01_playing = false;
		kane_conversation02_playing = false;

		GameObject * hologram01 = ScriptEngine::Find_Object ( 100252 );
		if (hologram01 != nullptr)
		{
			GameObject * commander = ScriptEngine::Find_Object ( 101250 );
			if (commander != nullptr)
			{
				GameObject * kane01 = ScriptEngine::Create_Object_At_Bone ( hologram01, "Nod_Kane_HologramHead", "KANEBONE" );
				ScriptEngine::Disable_All_Collisions ( kane01 );
				ScriptEngine::Attach_To_Object_Bone( kane01, hologram01, "KANEBONE" );
				ScriptEngine::Set_Loiters_Allowed( kane01, false );
				ScriptEngine::Attach_Script(kane01, "M11_BattleStationAlpha_CommanderEncounter_KaneHead_JDG", "");
				kane01_ID = ScriptEngine::Get_ID ( kane01 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)//starting first kane-commander encounter
			{
				GameObject * kane01 = ScriptEngine::Find_Object ( kane01_ID );
				GameObject * commander = ScriptEngine::Find_Object ( 101250 );

				if (commander != nullptr)
				{
					kane_conversation01 = ScriptEngine::Create_Conversation( "M11_Kane_and_Blackhand_Conversation_01", 100, 1000, true);
					ScriptEngine::Join_Conversation( kane01, kane_conversation01, false, true, true );
					ScriptEngine::Join_Conversation( commander, kane_conversation01, false, true, true );
					ScriptEngine::Start_Conversation( kane_conversation01,  kane_conversation01 );

					ScriptEngine::Monitor_Conversation( obj, kane_conversation01 );

					kane_conversation01_playing = true;
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//havoc has entered first zone--start first havoc conv
			{
				if (kane_conversation01_playing == true)
				{
					kane_conversation01_playing = false;
					ScriptEngine::Stop_Conversation( kane_conversation01 );

					GameObject * commander = ScriptEngine::Find_Object ( 101250 );
					if (commander != nullptr)
					{
						ScriptEngine::Send_Custom_Event ( obj, commander, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}

					GameObject * kane01 = ScriptEngine::Find_Object ( kane01_ID );
					if (kane01 != nullptr)
					{
						kane_conversation01b = ScriptEngine::Create_Conversation( "M11_Kane_and_Havoc_Conversation_01", 90, 30, true);
						ScriptEngine::Join_Conversation( kane01, kane_conversation01b, false, true, true );
						ScriptEngine::Join_Conversation( STAR, kane_conversation01b, false, false, false );
						ScriptEngine::Start_Conversation( kane_conversation01b,  kane_conversation01b );

						ScriptEngine::Monitor_Conversation( obj, kane_conversation01b );

						kane_conversation02_playing = true;
					}
				}

				else if (kane_conversation01_playing == false)
				{
					GameObject * hologram01 = ScriptEngine::Find_Object ( 100252 );
					if (hologram01 != nullptr)
					{
						GameObject * kane02 = ScriptEngine::Create_Object_At_Bone ( hologram01, "Nod_Kane_HologramHead", "KANEBONE" );
						ScriptEngine::Disable_All_Collisions ( kane02 );
						ScriptEngine::Attach_To_Object_Bone( kane02, hologram01, "KANEBONE" );
						ScriptEngine::Set_Loiters_Allowed( kane02, false );
						ScriptEngine::Attach_Script(kane02, "M11_BattleStationAlpha_HavocEncounter_KaneHead_JDG", "");
						kane02_ID = ScriptEngine::Get_ID ( kane02 );
					}
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG && kane_conversation02_playing == false)//kane head01 is giving clearance to start havoc conv
			{
				kane_conversation02 = ScriptEngine::Create_Conversation( "M11_Kane_and_Havoc_Conversation_01", 90, 30, true);
				ScriptEngine::Join_Conversation( sender, kane_conversation02, false, true, true );
				ScriptEngine::Join_Conversation( STAR, kane_conversation02, false, false, false );
				ScriptEngine::Start_Conversation( kane_conversation02,  kane_conversation02 );

				ScriptEngine::Monitor_Conversation( obj, kane_conversation02 );

				kane_conversation02_playing = true;
			}

			else if (param == M01_IVE_BEEN_KILLED_JDG)
			{
				GameObject * commander = ScriptEngine::Find_Object ( 101250 );
				if (sender == commander)
				{
					if (kane_conversation01_playing == true)
					{
						kane_conversation01_playing = false;
						ScriptEngine::Stop_Conversation( kane_conversation01 );

						GameObject * kane01 = ScriptEngine::Find_Object ( kane01_ID );
						if (kane01 != nullptr)
						{
							ScriptEngine::Send_Custom_Event ( obj, kane01, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}
					}
				}
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == kane_conversation01 && kane_conversation01_playing == true)
					{
						kane_conversation01_playing = false;

						GameObject * kane01 = ScriptEngine::Find_Object ( kane01_ID );
						if (kane01 != nullptr)
						{
							ScriptEngine::Send_Custom_Event ( obj, kane01, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}

						GameObject * commander = ScriptEngine::Find_Object ( 101250 );
						if (commander != nullptr)
						{
							ScriptEngine::Send_Custom_Event ( obj, commander, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}
					}

					else if (action_id == kane_conversation01b && kane_conversation02_playing == true)
					{
						kane_conversation02_playing = false;

						GameObject * kane01 = ScriptEngine::Find_Object ( kane01_ID );
						if (kane01 != nullptr)
						{
							ScriptEngine::Send_Custom_Event ( obj, kane01, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}
					}

					else if (action_id == kane_conversation02 && kane_conversation02_playing == true)
					{
						kane_conversation02_playing = false;

						GameObject * kane02 = ScriptEngine::Find_Object ( kane02_ID );
						if (kane02 != nullptr)
						{
							ScriptEngine::Send_Custom_Event ( obj, kane02, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_BattleStationAlpha_CommanderEncounter_KaneHead_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		GameObject * hologramController = ScriptEngine::Find_Object ( 101251 );
		if (hologramController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, hologramController, 0, M01_MODIFY_YOUR_ACTION_JDG, 4 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)//conversation is over--go away
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_BattleStationAlpha_HavocEncounter_KaneHead_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		GameObject * hologramController = ScriptEngine::Find_Object ( 101251 );
		if (hologramController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, hologramController, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0 );
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)//conversation is over--go away
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_Temple_Hologram_01_JDG, "")//100252
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Animation (obj, "DSP_HOLO_BIG.DSP_HOLO_BIG", true, nullptr, 0.0f, -1.0f, false);
	}
};

DECLARE_SCRIPT(M11_SecondHolograph_EntryZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * hologramController = ScriptEngine::Find_Object ( 101251 );
			if (hologramController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, hologramController, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};

DECLARE_SCRIPT(M11_Floor01_InitialPatrol_Left01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), RUN, 0.25f);
		params.WaypathID = 100210;
		params.WaypointStartID = 100211;
		params.WaypointEndID = 100218;

		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			params.Set_Basic( this, 100, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(-7.989f, 16.986f, -18), RUN, 0.25f);

			ScriptEngine::Action_Goto( obj, params );
		}

		else if (action_id == M01_WALKING_WAYPATH_02_JDG)
		{
			ScriptEngine::Enable_Hibernation( obj, true );
			ScriptEngine::Set_Innate_Is_Stationary (obj, true );
		}
	}
};

DECLARE_SCRIPT(M11_Floor01_InitialPatrol_Right01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
		params.Set_Movement( Vector3(0,0,0), RUN, 0.25f);
		params.WaypathID = 100199;
		params.WaypointStartID = 100200;
		params.WaypointEndID = 100208;

		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason /* complete_reason */ ) override
	{
		ActionParamsStruct params;

		if (action_id == M01_WALKING_WAYPATH_01_JDG)
		{
			params.Set_Basic( this, 100, M01_WALKING_WAYPATH_02_JDG );
			params.Set_Movement( Vector3(1.354f, 15.273f, -18), RUN, 0.25f);

			ScriptEngine::Action_Goto( obj, params );
		}
		else if (action_id == M01_WALKING_WAYPATH_02_JDG)
		{
			ScriptEngine::Enable_Hibernation( obj, true );
			ScriptEngine::Set_Innate_Is_Stationary (obj, true );
		}
	}
};

DECLARE_SCRIPT(M11_KaneRoom_KaneEncounter_Zone_JDG, "")//
{
	int kane_ID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(kane_ID, 1);
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * holograph = ScriptEngine::Find_Object ( 101226 );
			if (holograph != nullptr)
			{
				GameObject * kane = ScriptEngine::Create_Object_At_Bone ( holograph, "Nod_Kane_HologramHead_small", "KANEBONE" );
				ScriptEngine::Disable_All_Collisions ( kane );
				ScriptEngine::Attach_To_Object_Bone( kane, holograph, "KANEBONE" );
				ScriptEngine::Set_Loiters_Allowed( kane, false );
				ScriptEngine::Attach_Script(kane, "M11_KaneRoom_KaneEncounter_Kane_JDG", "");
				kane_ID = ScriptEngine::Get_ID ( kane );

				ScriptEngine::Destroy_Object ( obj );
			}
		}
	}
};

DECLARE_SCRIPT(M11_KaneRoom_KaneEncounter_Kane_JDG, "")//M11_KanesRoom_Kane_Conversation
{
	int kane_conversation02;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(kane_conversation02, 1);
	}

	void Created( GameObject * obj ) override
	{
		if (STAR)
		{
			kane_conversation02 = ScriptEngine::Create_Conversation( "M11_KanesRoom_Kane_Conversation", 100, 1000, false);
			ScriptEngine::Join_Conversation( obj, kane_conversation02, false, true, true );
			ScriptEngine::Join_Conversation( STAR, kane_conversation02, false, false, true );
			ScriptEngine::Start_Conversation( kane_conversation02,  kane_conversation02 );

			ScriptEngine::Monitor_Conversation( obj, kane_conversation02 );
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == kane_conversation02)
					{
						ScriptEngine::Debug_Message ( "***************************kane conversation 02 is over--sending delete custom\n" );
						ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, 2 );
					}
				}
				break;
			default:
				break;
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)//conversation is over--go away
		{
			ScriptEngine::Debug_Message ( "***************************delete custom received--kane should now vanish\n" );
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_KaneRoom_KaneEncounter_Hologram_JDG, "")//101226
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Animation (obj, "DSP_HOLO.DSP_HOLO", true, nullptr, 0.0f, -1.0f, false);
	}
};

DECLARE_SCRIPT(M11_CABAL_Babble_Contoller_JDG, "")//  101204
{
	int last;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(last, 1);
	}

	void Created( GameObject * obj ) override
	{
		last = 20;
		ScriptEngine::Enable_Hibernation( obj, false);
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (obj && param == M01_START_ACTING_JDG)
			{
				Vector3 speaker01position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker02position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker03position = ScriptEngine::Get_Position ( STAR );

				speaker01position.X += ScriptEngine::Get_Random(-10,10);
				speaker01position.Y += ScriptEngine::Get_Random(-10,10);
				speaker01position.Z += ScriptEngine::Get_Random(0,10);

				speaker02position.X += ScriptEngine::Get_Random(-10,10);
				speaker02position.Y += ScriptEngine::Get_Random(-10,10);
				speaker02position.Z += ScriptEngine::Get_Random(0,10);

				speaker03position.X += ScriptEngine::Get_Random(-10,10);
				speaker03position.Y += ScriptEngine::Get_Random(-10,10);
				speaker03position.Z += ScriptEngine::Get_Random(0,10);

				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0089I1EVAC_SND", speaker01position, obj );
				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0089I1EVAC_SND", speaker02position, obj );
				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0089I1EVAC_SND", speaker03position, obj );

				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 6 );

			}

			else if (obj && param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				Vector3 speaker01position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker02position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker03position = ScriptEngine::Get_Position ( STAR );

				speaker01position.X += ScriptEngine::Get_Random(-10,10);
				speaker01position.Y += ScriptEngine::Get_Random(-10,10);
				speaker01position.Z += ScriptEngine::Get_Random(0,10);

				speaker02position.X += ScriptEngine::Get_Random(-10,10);
				speaker02position.Y += ScriptEngine::Get_Random(-10,10);
				speaker02position.Z += ScriptEngine::Get_Random(0,10);

				speaker03position.X += ScriptEngine::Get_Random(-10,10);
				speaker03position.Y += ScriptEngine::Get_Random(-10,10);
				speaker03position.Z += ScriptEngine::Get_Random(0,10);

				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0097I1EVAC_SND", speaker01position, obj );
				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0097I1EVAC_SND", speaker02position, obj );
				ScriptEngine::Create_Sound ( "M11DSGN_DSGN0097I1EVAC_SND", speaker03position, obj );

				float delayTimer = ScriptEngine::Get_Random ( 15, 60 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );

			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				Vector3 speaker01position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker02position = ScriptEngine::Get_Position ( STAR );
				Vector3 speaker03position = ScriptEngine::Get_Position ( STAR );

				speaker01position.X += ScriptEngine::Get_Random(-10,10);
				speaker01position.Y += ScriptEngine::Get_Random(-10,10);
				speaker01position.Z += ScriptEngine::Get_Random(0,10);

				speaker02position.X += ScriptEngine::Get_Random(-10,10);
				speaker02position.Y += ScriptEngine::Get_Random(-10,10);
				speaker02position.Z += ScriptEngine::Get_Random(0,10);

				speaker03position.X += ScriptEngine::Get_Random(-10,10);
				speaker03position.Y += ScriptEngine::Get_Random(-10,10);
				speaker03position.Z += ScriptEngine::Get_Random(0,10);

				static constexpr const char* cabalSounds[6] =
				{
					//this is a list of potential cabal sayings
					"M11DSGN_DSGN0088I1EVAC_SND",
					"M11DSGN_DSGN0090I1EVAC_SND",
					"M11DSGN_DSGN0091I1EVAC_SND",
					"M11DSGN_DSGN0092I1EVAC_SND",
					"M11DSGN_DSGN0094I1EVAC_SND",
					"M11DSGN_DSGN0096I1EVAC_SND",
				};
				int random = ScriptEngine::Get_Random_Int(0, 6);

				while (random == last)
				{
					random = ScriptEngine::Get_Random_Int(0, 6);
				}

				ScriptEngine::Create_Sound ( cabalSounds[random], speaker01position, obj );
				ScriptEngine::Create_Sound ( cabalSounds[random], speaker02position, obj );
				ScriptEngine::Create_Sound ( cabalSounds[random], speaker03position, obj );

				float delayTimer = ScriptEngine::Get_Random ( 15, 60 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );

				last = random;
			}
		}
	}
};

DECLARE_SCRIPT(M11_AblesTomb_TalkZone_JDG, "")//M11_Ables_Tomb_Conversation
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			int able_conv = ScriptEngine::Create_Conversation( "M11_Ables_Tomb_Conversation", 100, 2000, false);
			ScriptEngine::Join_Conversation( nullptr, able_conv, false, false, false );
			ScriptEngine::Join_Conversation( STAR, able_conv, false, false, false );
			ScriptEngine::Start_Conversation( able_conv, able_conv );

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_Visceroid_JDG, "")//100394 (for now)
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_START_ACTING_JDG );
			params.Set_Movement(STAR, RUN, 1);
			params.Set_Attack( STAR, 2, 0, true );
			params.AttackCheckBlocked = false;

			ScriptEngine::Action_Attack ( obj, params );
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_Visceroid_KeyCarrier_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		Vector3 spawnLocation = ScriptEngine::Get_Position ( obj );
		spawnLocation.Z += 0.75f;
		ScriptEngine::Create_Object ( "Level_02_Keycard", spawnLocation );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_START_ACTING_JDG );
			params.Set_Movement(STAR, RUN, 1);
			params.Set_Attack( STAR, 2, 0, true );
			params.AttackCheckBlocked = false;

			ScriptEngine::Action_Attack ( obj, params );
		}
	}
};


DECLARE_SCRIPT(M11_Barracks_Visceroid_Morpher_JDG, "")//
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
			params.Set_Animation( "H_A_7002", true );
			ScriptEngine::Action_Play_Animation (  obj, params );

			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			ScriptEngine::Create_Object ( "Visceroid", myPosition );

			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0.5f );
		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			ScriptEngine::Apply_Damage( obj, 10000, "TiberiumRaw", nullptr );
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_Script_JDG, "")//Petrova's ID is M11_MUTANT_PETROVA_JDG 100690
{
	enum LocationEnum {
		UNDEFINED_LOCATION = -1,
		SOUTH0,
		EAST0,
		WEST0,
		EAST1,
		NORTH1,
		WEST1,
		SOUTH2,
		EAST2,
		WEST2
	};

	enum WaypathEnum {
		INITIAL_TO_SOUTH0 = 300325,
		SOUTH0_TO_EAST0	= 300117,
		SOUTH0_TO_WEST0	= 300113,
		EAST0_TO_SOUTH0	= 300149,
		EAST0_TO_WEST0		= 300153,
		EAST0_TO_WEST1		= 300121,
		WEST0_TO_SOUTH0	= 300145,
		WEST0_TO_EAST0		= 300157,
		EAST1_TO_WEST1		= 300201,
		EAST1_TO_WEST2		= 300243,
		NORTH1_TO_WEST1	= 300238,
		WEST1_TO_EAST0		= 300125,
		WEST1_TO_NORTH1	= 300233,
		WEST1_TO_EAST1		= 300193,
		SOUTH2_TO_EAST2	= 300285,
		SOUTH2_TO_WEST2	= 300299,
		EAST2_TO_SOUTH2	= 300281,
		EAST2_TO_WEST2		= 300277,
		WEST2_TO_EAST1		= 300249,
		WEST2_TO_SOUTH2	= 300294,
		WEST2_TO_EAST2		= 300273
	};

	LocationEnum CurrentLocation;
	bool midtro01_over;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (CurrentLocation, 1);
		SAVE_VARIABLE (midtro01_over, 2);
	}

	void Created (GameObject *obj) override
	{
		ScriptEngine::Select_Weapon ( obj, nullptr );
		midtro01_over = false;
		CurrentLocation = UNDEFINED_LOCATION;
		ScriptEngine::Innate_Disable (obj);
		ScriptEngine::Enable_Stealth (obj, false);

		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation( "S_A_HUMAN.H_A_CON2", true );
		ScriptEngine::Action_Play_Animation (  obj, params );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (midtro01_over == false)
		{
			float myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
			float myMaxShield = ScriptEngine::Get_Max_Shield_Strength ( obj );
			ScriptEngine::Set_Health ( obj, myMaxHealth );
			ScriptEngine::Set_Shield_Strength ( obj, myMaxShield );

			GameObject * midtro01Controller = ScriptEngine::Find_Object ( 101449 );
			if (midtro01Controller != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, midtro01Controller, 0, M01_START_ACTING_JDG, 0 );
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override //
	{
		Vector3 spawn_location (3.6f, 87.9f, -34.2f);
		spawn_location.Z = spawn_location.Z + 0.75f;

		GameObject * keycard = ScriptEngine::Create_Object ( "Level_03_Keycard", spawn_location );
		if (keycard != nullptr)
		{
			ScriptEngine::Attach_Script(keycard, "M11_Level03Key_Script_JDG", "");
		}

		GameObject * tauntController = ScriptEngine::Find_Object ( 103797 );
		if (tauntController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, tauntController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}

		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Destroy_Object ( stealthController );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		static LocationEnum _locations [][3][2] = {

		//	  Player level		  Player above	    Player below.
			{{EAST0 , WEST0},  {EAST0 , WEST0}, {EAST0,  EAST0}},	// Destinations from SOUTH0.
			{{SOUTH0, WEST0},  {SOUTH0, WEST0}, {WEST1,  WEST1}},	// Destinations from EAST0.
			{{SOUTH0, EAST0},  {SOUTH0, EAST0}, {EAST0,  EAST0}},	// Destinations from WEST0.

			{{WEST1,  WEST1},  {WEST1,  WEST1}, {WEST2,  WEST2}},	// Destinations from EAST1.
			{{WEST1,  WEST1},  {WEST1,  WEST1}, {WEST1,  WEST1}},	// Destinations from NORTH1.
			{{EAST1,  NORTH1}, {EAST0,  EAST0}, {EAST1,	EAST1}},	// Destinations from WEST1.

			{{EAST2,  WEST2},  {WEST2,  WEST2}, {EAST2,  WEST2}},	// Destinations from SOUTH2.
			{{SOUTH2, WEST2},  {WEST2,  WEST2}, {SOUTH2, WEST2}},	// Destinations from EAST2.
			{{SOUTH2, EAST2},  {EAST1,  EAST1}, {SOUTH2, EAST2}},	// Destinations from WEST2.
		};

		static WaypathEnum _waypaths [][3][2] = {

			{{SOUTH0_TO_EAST0, SOUTH0_TO_WEST0}, {SOUTH0_TO_EAST0, SOUTH0_TO_WEST0}, {SOUTH0_TO_EAST0, SOUTH0_TO_EAST0}},	// Waypaths from SOUTH0.
			{{EAST0_TO_SOUTH0, EAST0_TO_WEST0} , {EAST0_TO_SOUTH0, EAST0_TO_WEST0} , {EAST0_TO_WEST1 , EAST0_TO_WEST1}},	// Waypaths from EAST0.
			{{WEST0_TO_SOUTH0, WEST0_TO_EAST0} , {WEST0_TO_SOUTH0, WEST0_TO_EAST0} , {WEST0_TO_EAST0 , WEST0_TO_EAST0}},	// Waypaths from WEST0.

			{{EAST1_TO_WEST1,  EAST1_TO_WEST1} , {EAST1_TO_WEST1,  EAST1_TO_WEST1} , {EAST1_TO_WEST2,  EAST1_TO_WEST2}},	// Waypaths from EAST1.
			{{NORTH1_TO_WEST1, NORTH1_TO_WEST1}, {NORTH1_TO_WEST1, NORTH1_TO_WEST1}, {NORTH1_TO_WEST1, NORTH1_TO_WEST1}},	// Waypaths from NORTH1.
			{{WEST1_TO_EAST1,  WEST1_TO_NORTH1}, {WEST1_TO_EAST0,  WEST1_TO_EAST0},  {WEST1_TO_EAST1,  WEST1_TO_EAST1}},	// Waypaths from WEST1.

			{{SOUTH2_TO_EAST2, SOUTH2_TO_WEST2}, {SOUTH2_TO_WEST2, SOUTH2_TO_WEST2}, {SOUTH2_TO_EAST2, SOUTH2_TO_WEST2}},	// Waypaths from SOUTH2.
			{{EAST2_TO_SOUTH2, EAST2_TO_WEST2} , {EAST2_TO_WEST2,  EAST2_TO_WEST2} , {EAST2_TO_SOUTH2, EAST2_TO_WEST2}}, 	// Waypaths from EAST2.
			{{WEST2_TO_SOUTH2, WEST2_TO_EAST2} , {WEST2_TO_EAST1,  WEST2_TO_EAST1} , {WEST2_TO_SOUTH2, WEST2_TO_EAST2}}		// Waypaths from WEST2.
		};

		ActionParamsStruct params;

		if (type == 0)
		{
			switch (param)
			{
				case M11_PETROVA_START_ACTING_JDG:
				{
					GameObject * tauntController = ScriptEngine::Find_Object ( 103797 );
					if (tauntController != nullptr)
					{
						ScriptEngine::Send_Custom_Event ( obj, tauntController, 0, M01_START_ACTING_JDG, 0 );
					}

					Vector3 myPosition = ScriptEngine::Get_Position ( obj );
					//ScriptEngine::Create_Logical_Sound (obj, M11_STEALTHS_START_ACTING_JDG, myPosition, 150 );

					midtro01_over = true;
					ScriptEngine::Enable_Stealth (obj, true);
					ScriptEngine::Select_Weapon ( obj, "Weapon_TiberiumAutoRifle_Ai" );
					const float priority			 = 100.0f;
					const float normalizedspeed = 2.0f;		// Maximum speed.

					ScriptEngine::Debug_Message ( "***************************Petrova--received custom to start acting\n" );

					params.Set_Basic (this, priority, M11_PETROVA_PICK_NEW_LOCATION_JDG);
					params.Set_Movement (Vector3 (0.0f, 0.0f, 0.0f), normalizedspeed, 1.0f);
					params.WaypathID = INITIAL_TO_SOUTH0;
					ScriptEngine::Action_Goto (obj, params);
					CurrentLocation = SOUTH0;
					break;
				}

				case M11_PETROVA_PICK_NEW_LOCATION_JDG:
				{
					const float heightthreshold = 1.0f;
					const float priority			 = 100.0f;

					float		heightdelta;
					unsigned heightselector;
					int		binaryselector;

					heightdelta = ScriptEngine::Get_Position (STAR).Z - ScriptEngine::Get_Position (obj).Z;
					if (heightdelta < -heightthreshold) {
						heightselector = 2;		// Player is below Petrova.
					} else {
						if (heightdelta > heightthreshold) {
							heightselector = 1;	// Player is above Petrova.
						} else {
							heightselector = 0;	// Player is level with Petrova.
						}
					}

					binaryselector = ScriptEngine::Get_Random_Int (0, 2);
					params.Set_Basic (this, priority, M11_PETROVA_PICK_NEW_LOCATION_JDG);
					params.Set_Movement (Vector3 (0.0f, 0.0f, 0.0f), RUN, 1.0f);
					params.WaypathID = _waypaths [CurrentLocation][heightselector][binaryselector];
					ScriptEngine::Action_Goto (obj, params);
					CurrentLocation = _locations [CurrentLocation][heightselector][binaryselector];
					break;
				}

				case M11_PETROVA_ATTACK_PLAYER_JDG:
				{
					const float priority		= 100.0f;
					const float range			= 300.0f;
					const float	error			= 0.0f;
					const float	attackdelay	= 0.25f;
					const float	pickdelay	= 0.25f;

					params.Set_Basic (this, priority, M11_PETROVA_ATTACK_PLAYER_JDG);
					params.Set_Attack (STAR, range, error, false);
					ScriptEngine::Action_Attack (obj, params);

					if (ScriptEngine::Is_Object_Visible (obj, STAR)) {
						ScriptEngine::Send_Custom_Event (obj, obj, 0, M11_PETROVA_ATTACK_PLAYER_JDG, attackdelay);
					} else {
						ScriptEngine::Send_Custom_Event (obj, obj, 0, M11_PETROVA_PICK_NEW_LOCATION_JDG, pickdelay);
					}
					break;
				}

				default:
					break;
			}
		}
	}

	void Action_Complete (GameObject *obj, int actionid, ActionCompleteReason completereason) override
	{
		switch (completereason)	{

			case ACTION_COMPLETE_NORMAL:
			{
				if (actionid == M11_PETROVA_PICK_NEW_LOCATION_JDG) {

					const float	priority		 = 100.0f;
					const float	turnduration = 1.0f;

					ActionParamsStruct params;

					params.Set_Basic (this, priority, M11_PETROVA_FACE_PLAYER_IML);
					params.Set_Face_Location (ScriptEngine::Get_Position (STAR), turnduration);
					ScriptEngine::Action_Face_Location (obj, params);

				} else {

					if (actionid ==  M11_PETROVA_FACE_PLAYER_IML) {
						ScriptEngine::Send_Custom_Event (obj, obj, 0, M11_PETROVA_ATTACK_PLAYER_JDG, 0.0f);
					}
				}
				break;
			}

			default:
				// Do nothing.
				break;
		}
	}
};

DECLARE_SCRIPT(M11_Level03Key_Script_JDG, "")
{
	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			GameObject * midtro01Controller = ScriptEngine::Find_Object ( 101449 );
			if (midtro01Controller != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, midtro01Controller, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Okay_ToPlay_MidtroZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * midtro01Controller = ScriptEngine::Find_Object ( 101449 );
			if (midtro01Controller != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, midtro01Controller, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_Start_FirstMidtro_Zone_JDG, "")//101449
{
	bool entered;
	bool command_clearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(entered, 1);
		SAVE_VARIABLE(command_clearance, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		entered = false;
		command_clearance = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			if (entered == false && command_clearance == true)
			{
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, 0 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 0)//first midtro is over--set everyone back to visible
			{
				ScriptEngine::Set_Position ( STAR, Vector3(-2.124f, 84.915f, -34.233f) );
				ScriptEngine::Set_Facing ( STAR, 87 );
				ScriptEngine::Control_Enable ( STAR, true );
				ScriptEngine::Set_Is_Rendered( STAR, true );

				GameObject *petrova = ScriptEngine::Find_Object ( M11_MUTANT_PETROVA_JDG );
				if (petrova != nullptr)
				{
					ScriptEngine::Set_Is_Rendered( petrova, true );
					ScriptEngine::Send_Custom_Event( obj, petrova, 0, M11_PETROVA_START_ACTING_JDG, 0 );
				}



				GameObject *mutant01 = ScriptEngine::Find_Object ( 101964 );
				GameObject *mutant02 = ScriptEngine::Find_Object ( 101997 );

				//GameObject *visceroid01 = ScriptEngine::Find_Object ( 101966 );
				//GameObject *visceroid02 = ScriptEngine::Find_Object ( 101996 );
				//GameObject *visceroid03 = ScriptEngine::Find_Object ( 101965 );

				if (mutant01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, mutant01, 0, M01_START_ACTING_JDG, 0 );
				}

				if (mutant02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, mutant02, 0, M01_START_ACTING_JDG, 3 );
				}

				GameObject * visceroid01 = ScriptEngine::Find_Object (101995);
				GameObject * visceroid02 = ScriptEngine::Find_Object (101996);

				if (visceroid01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid01, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
				}

				if (visceroid02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid02, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
				}

				/*if (visceroid01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid01, 0, M01_START_ACTING_JDG, 0 );
				}

				if (visceroid02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid02, 0, M01_START_ACTING_JDG, 3 );
				}

				if (visceroid03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid03, 0, M01_START_ACTING_JDG, 5 );
				}*/
			}

			else if (param == 1)//second midtro is over--set everyone back to visible
			{
				GameObject * simpleSydney = ScriptEngine::Find_Object (M11_SIMPLE_SYDNEY_MOBIUS_JDG);
				if (simpleSydney != nullptr)
				{
					Vector3 sydnesLocation  (0.084f, 91.860f, -34.233f);

					GameObject * realSydney = ScriptEngine::Find_Object (M11_REAL_SYDNEY_MOBIUS_JDG);
					if (realSydney != nullptr)
					{
						ScriptEngine::Set_Shield_Strength ( realSydney, 100 );
						ScriptEngine::Set_Obj_Radar_Blip_Shape (realSydney, RADAR_BLIP_SHAPE_HUMAN );
						ScriptEngine::Set_Obj_Radar_Blip_Color ( realSydney, RADAR_BLIP_COLOR_GDI );
						ScriptEngine::Set_Is_Visible( realSydney, true );
						ScriptEngine::Set_Is_Rendered( realSydney, true );
						ScriptEngine::Set_Position ( realSydney, sydnesLocation );
						ScriptEngine::Send_Custom_Event( obj, realSydney, 0, M01_START_ACTING_JDG, 0.5f );
					}

					ScriptEngine::Destroy_Object ( simpleSydney );//get rid of simple version of Sydney
				}

				ScriptEngine::Set_Position ( STAR, Vector3(2.2f, 91.8f, -34.2f) );
				ScriptEngine::Set_Facing ( STAR, 127 );
				ScriptEngine::Control_Enable ( STAR, true );
				ScriptEngine::Set_Is_Rendered( STAR, true );
			}

			else if (param == M01_START_ACTING_JDG && entered == false && command_clearance == true)
			{
				entered = true;
				GameObject *petrova = ScriptEngine::Find_Object ( M11_MUTANT_PETROVA_JDG );
				if (petrova != nullptr)
				{
					ScriptEngine::Set_Is_Rendered( petrova, false );
				}

				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11M_MIDTRO.txt");

				ScriptEngine::Control_Enable ( STAR, false );
				ScriptEngine::Set_Is_Rendered( STAR, false );
				ScriptEngine::Set_Position ( STAR, Vector3(-2.124f, 84.915f, -34.233f) );
				ScriptEngine::Set_Facing ( STAR, 87 );

				ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1300001124, 6, "SHP_DOORP01.SHP_DOORP01" );
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1300001123, 6, "SHP_DOORP01.SHP_DOORP01" );

				GameObject * visceroid01 = ScriptEngine::Find_Object (101995);
				GameObject * visceroid02 = ScriptEngine::Find_Object (101996);

				if (visceroid01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid01, 0, M01_START_ACTING_JDG, 5 );
				}

				if (visceroid02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, visceroid02, 0, M01_START_ACTING_JDG, 5 );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				ScriptEngine::Control_Enable ( STAR, false );
				ScriptEngine::Set_Is_Rendered( STAR, false );

				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11N_MIDTRO.txt");
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				command_clearance = true;
			}
		}
	}
};

DECLARE_SCRIPT(M11_PowerCore_TriggerPetrova_EnterZone_JDG, "")
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR)
		{
			GameObject * petrova = ScriptEngine::Find_Object (M11_MUTANT_PETROVA_JDG);

			if (petrova != nullptr)
			{
				ScriptEngine::Debug_Message ( "***************************M11-Petrova trigger zone sending custom to petrova--jdg\n" );
				ScriptEngine::Send_Custom_Event( obj, petrova, 0, M11_PETROVA_START_ACTING_JDG, 0 );
				ScriptEngine::Destroy_Object ( obj );//one time zone--cleaning up
			}

			else
			{
				ScriptEngine::Debug_Message ( "***************************M11-Petrova trigger zone cannot find petrova--jdg\n" );
			}
		}
	}
};





DECLARE_SCRIPT(M11_Barracks_MutantUprising_BlackhandGuy_JDG, "")//M11_BARRACKS_MUTANTUPRISING_BLACKHAND_JDG 100387
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);

			GameObject * mutant01 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_MUTANT01_JDG );
			if (mutant01 != nullptr)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 45, M01_START_ATTACKING_01_JDG);
				params.Set_Attack( mutant01, 50, 0, true );
				params.AttackCrouched = true;
				ScriptEngine::Action_Attack ( obj, params );
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		switch (complete_reason)
		{
			case ACTION_COMPLETE_NORMAL:
				{
					if (action_id == M01_START_ATTACKING_01_JDG)
					{
						GameObject * mutant02 = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_MUTANT02_JDG );
						if (mutant02 != nullptr)
						{
							ActionParamsStruct params;
							params.Set_Basic(this, 45, M01_START_ATTACKING_02_JDG);
							params.Set_Attack( mutant02, 50, 0, true );
							params.AttackCrouched = true;
							ScriptEngine::Action_Attack ( obj, params );
						}
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_MutantUprising_Mutant01_JDG, "")//M11_BARRACKS_MUTANTUPRISING_MUTANT01_JDG 100388
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
			GameObject * blackhand = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_BLACKHAND_JDG );

			if (blackhand != nullptr)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 45, M01_GOING_TO_HAVOC_JDG);
				params.Set_Movement( blackhand, RUN, 0.5f, false );
				params.Set_Attack( blackhand, 0.5f, 0, true );
				ScriptEngine::Action_Attack ( obj, params );

				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Barracks_MutantUprising_Mutant02_JDG, "")//M11_BARRACKS_MUTANTUPRISING_MUTANT02_JDG 100389
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ScriptEngine::Innate_Enable(obj);
			GameObject * blackhand = ScriptEngine::Find_Object ( M11_BARRACKS_MUTANTUPRISING_BLACKHAND_JDG );

			if (blackhand != nullptr)
			{
				ActionParamsStruct params;
				params.Set_Basic(this, 45, M01_GOING_TO_HAVOC_JDG);
				params.Set_Movement( blackhand, RUN, 0.5f, false );
				params.Set_Attack( blackhand, 0.5f, 0, true );
				ScriptEngine::Action_Attack ( obj, params );

				const char* soundName = M11_Choose_Mutant_Attack_Sound ( );
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				ScriptEngine::Create_Sound ( soundName, myPosition, obj );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Flyover_Contoller_JDG, "")
{
	int last;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(last, 1);
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Start_Timer(obj, this, 5.0f, 0);
		last = 0;
	}

	void Timer_Expired(GameObject * obj, int /* timer_id */) override
	{
		if (last >= 4)
		{
			last = 0;
		}
		static constexpr const char* flyovers[4] =
		{
			//this is a low level traffic lane from nod comm to gdi ocean-- N->S
			"X11A_Flyover_01.txt",
			"X11A_Flyover_02.txt",
			"X11A_Flyover_03.txt",
			"X11A_Flyover_04.txt",
		};

		GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
		ScriptEngine::Attach_Script(controller, "Test_Cinematic", flyovers[last]);
		ScriptEngine::Start_Timer(obj, this, 10, 5);
		last++;

	}
};

DECLARE_SCRIPT(M11_Cinematic_Havoc_Script_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Select_Weapon ( obj, nullptr );
	}
};

DECLARE_SCRIPT(M11_Cinematic_Petrova_Script_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Stealth (obj, false);
		ScriptEngine::Select_Weapon ( obj, nullptr );
	}
};

DECLARE_SCRIPT(M11_Sydney_SandM_Machine_Script_JDG, "")//157366
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		ScriptEngine::Set_Animation ( obj, "DSP_TORTURE.DSP_TORTURE_01", true, nullptr, 0.0f, -1.0f, false );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 0)//DO FREE SYDNEY ANIM
			{
				ScriptEngine::Enable_Hibernation( obj, false );
				ScriptEngine::Enable_Cinematic_Freeze( obj, false );
				ScriptEngine::Set_Animation ( obj, "DSP_TORTURE.DSP_TORTURE_02", false, nullptr, 0.0f, -1.0f, false );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_SandM_Chair_Script_JDG, "")//101606
{
	void Created( GameObject * obj ) override
	{
		//ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 0)//DO FREE SYDNEY ANIM
			{
				ScriptEngine::Enable_Hibernation( obj, false );
				ScriptEngine::Enable_Cinematic_Freeze( obj, false );
				ScriptEngine::Set_Animation ( obj, "DSP_TORTURE_TBL.DSP_TORTURE_T01", false, nullptr, 0.0f, -1.0f, false );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_SandM_Wall_Script_JDG, "")//101607
{
	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 0)//wall goes away
			{
				ScriptEngine::Destroy_Object ( obj );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_SandM_Sydney_Bone_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		//ScriptEngine::Enable_Hibernation( obj, false  );
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)//play animation A
			{
				ScriptEngine::Enable_Hibernation( obj, false  );
				ScriptEngine::Debug_Message ( "**********************Sydney bone custom received....going to anim A\n" );
				ScriptEngine::Set_Animation ( obj, "X11C_BN_Sydney.X11C_SYD_A", false, nullptr, 0.0f, -1.0f, false);
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//play animation B
			{
				ScriptEngine::Enable_Hibernation( obj, false  );
				ScriptEngine::Debug_Message ( "**********************Sydney bone custom received....going to anim B\n" );
				ScriptEngine::Set_Animation ( obj, "X11C_BN_Sydney.X11C_SYD_B", false, nullptr, 0.0f, -1.0f, false);
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)//play animation c
			{
				ScriptEngine::Enable_Hibernation( obj, false  );
				ScriptEngine::Debug_Message ( "**********************Sydney bone custom received....going to anim C\n" );
				ScriptEngine::Set_Animation ( obj, "X11C_BN_Sydney.X11C_SYD_C", false, nullptr, 0.0f, -1.0f, false);
			}

			else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//play animation d
			{
				ScriptEngine::Enable_Hibernation( obj, false  );
				ScriptEngine::Debug_Message ( "**********************Sydney bone custom received....going to anim D\n" );
				ScriptEngine::Set_Animation ( obj, "X11C_BN_Sydney.X11C_SYD_D", false, nullptr, 0.0f, -1.0f, false);
			}
		}
	}
};

DECLARE_SCRIPT(M11_SimpleSydney_Script_JDG, "")//M11_SIMPLE_SYDNEY_MOBIUS_JDG	100697
{
	int bone_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(bone_id, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		GameObject * tortureMachine = ScriptEngine::Find_Object ( 157366 );
		if (tortureMachine != nullptr)
		{
			Vector3 machinePosition = ScriptEngine::Get_Position ( tortureMachine );
			float machineFacing = ScriptEngine::Get_Facing ( tortureMachine );

			GameObject * troopbone = ScriptEngine::Create_Object("Invisible_Object", machinePosition );
			ScriptEngine::Set_Model ( troopbone, "X11C_BN_Sydney" );
			ScriptEngine::Set_Facing ( troopbone, machineFacing );
			ScriptEngine::Attach_Script(troopbone, "M11_Sydney_SandM_Sydney_Bone_JDG", "");
			bone_id = ScriptEngine::Get_ID ( troopbone );

			ScriptEngine::Attach_To_Object_Bone( obj, troopbone, "BN_SYDNEY" );

			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
			ScriptEngine::Debug_Message ( "**********************bone created with ID....................%d\n", bone_id );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 0)//now do free sydney anim
			{
				GameObject * troopbone = ScriptEngine::Find_Object ( bone_id );
				if (troopbone != nullptr)
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim D sending custom to bone\n" );
					ScriptEngine::Send_Custom_Event ( obj, troopbone, 0, M01_MODIFY_YOUR_ACTION_04_JDG, 0 );
					ScriptEngine::Set_Animation ( obj, "S_B_HUMAN.H_B_X11C_SYD_D", false, nullptr, 0.0f, -1.0f, false);
				}

				else
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim D cannot find bone\n" );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)//play animation A
			{
				GameObject * troopbone = ScriptEngine::Find_Object ( bone_id );
				if (troopbone != nullptr)
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim A sending custom to bone\n" );
					ScriptEngine::Send_Custom_Event ( obj, troopbone, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					ScriptEngine::Set_Animation ( obj, "S_B_HUMAN.H_B_X11C_SYD_A", false, nullptr, 0.0f, -1.0f, false);
				}

				else
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim A cannot find bone\n" );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//play animation B
			{
				GameObject * troopbone = ScriptEngine::Find_Object ( bone_id );
				if (troopbone != nullptr)
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim B sending custom to bone\n" );
					ScriptEngine::Send_Custom_Event ( obj, troopbone, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
					ScriptEngine::Set_Animation ( obj, "S_B_HUMAN.H_B_X11C_SYD_B", false, nullptr, 0.0f, -1.0f, false);
				}

				else
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim B cannot find bone\n" );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)//play animation c
			{
				GameObject * troopbone = ScriptEngine::Find_Object ( bone_id );
				if (troopbone != nullptr)
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim C sending custom to bone\n" );
					ScriptEngine::Send_Custom_Event ( obj, troopbone, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0 );
					ScriptEngine::Set_Animation ( obj, "S_B_HUMAN.H_B_X11C_SYD_C", false, nullptr, 0.0f, -1.0f, false);
				}

				else
				{
					ScriptEngine::Debug_Message ( "**********************Sydney anim C cannot find bone\n" );
				}
			}

			//else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//play animation d
			//{

			//}
		}
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "S_B_HUMAN.H_B_X11C_SYD_A") == 0)
		{
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
		}

		else if (stricmp(anim, "S_B_HUMAN.H_B_X11C_SYD_B") == 0)
		{
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0 );
		}

		else if (stricmp(anim, "S_B_HUMAN.H_B_X11C_SYD_C") == 0)
		{
			ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}

		else if (stricmp(anim, "S_B_HUMAN.H_B_X11C_SYD_D") == 0)
		{
			//ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}
};

Vector3 m11ObeliskExplosionLocs[13] = {
	Vector3 (-10.432f, -96.805f, 4.504f),
	Vector3 (-15.732f, -106.739f, 10.572f),
	Vector3 (0.665f, -105.108f, 8.224f),
	Vector3 (14.913f, -99.565f, 4.977f),
	Vector3 (33.114f, -103.768f, 7.549f),
	Vector3 (34.822f, -114.184f, 12.776f),
	Vector3 (24.790f, -116.226f, 13.617f),
	Vector3 (12.581f, -111.634f, 11.247f),
	Vector3 (-5.196f, -116.416f, 13.689f),
	Vector3 (-9.931f, -136.410f, 18.047f),
	Vector3 (8.336f, -122.921f, 21.475f),
	Vector3 (12.898f, -123.847f, 27.640f),
	Vector3 (6.441f, -125.543f, 36.824f),
};

DECLARE_SCRIPT(M11_ObeliskBattle_Contoller_JDG, "")//101805
{
	int last;
	bool commandClearance;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last, 1 );
		SAVE_VARIABLE( commandClearance, 2 );
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		commandClearance = true;
		float delayTimer = ScriptEngine::Get_Random ( 0, 5 );
		ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_01_JDG, delayTimer );
		last = 20;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_GOTO_IDLE_JDG)
		{
			commandClearance = false;
			ScriptEngine::Enable_Hibernation( obj, true );
		}

		else if (param == M01_START_ACTING_JDG)
		{
			commandClearance = true;
			ScriptEngine::Enable_Hibernation( obj, false );
			float delayTimer = ScriptEngine::Get_Random ( 0, 5 );
			ScriptEngine::Start_Timer(obj, this, delayTimer, 0);
		}

		else if (param == M01_START_ATTACKING_01_JDG)
		{
			if (commandClearance == true)
			{
				int random = ScriptEngine::Get_Random_Int(0, 13);

				while (random == last) {
					random = ScriptEngine::Get_Random_Int(0, 13);
				}
				last = random;

				ScriptEngine::Create_Explosion ( "Ground Explosions - Harmless", m11ObeliskExplosionLocs[random], nullptr );

				float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ATTACKING_01_JDG, delayTimer );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Cell_Mutant01_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation( "S_A_HUMAN.H_A_7002", true );
		ScriptEngine::Action_Play_Animation (  obj, params );

		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
		ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_MUTANT );
	}
};

DECLARE_SCRIPT(M11_Cell_Mutant02_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Animation ("H_A_A0A0_L28B", true);
		ScriptEngine::Action_Play_Animation (obj, params);

		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
		ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_MUTANT );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
			params.Set_Animation ("H_A_A0A0_L28C", false);
			ScriptEngine::Action_Play_Animation (obj, params);

			//ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_MUTANT );
			//ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_MUTANT );
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_DOING_ANIMATION_02_JDG)
			{
				ActionParamsStruct params;

				params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement(Vector3 (0, 0, 0), RUN, 3);
				params.WaypathID = 104142;

				ScriptEngine::Action_Goto(obj, params);
			}

			else if (action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				GameObject * petrova = ScriptEngine::Find_Object ( M11_MUTANT_PETROVA_JDG );
				if (petrova != nullptr)
				{
					ActionParamsStruct params;

					params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
					params.Set_Movement(petrova, RUN, 15);
					params.Set_Attack( petrova, 20, 0, true );

					ScriptEngine::Action_Attack(obj, params);
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_Cell_Visceroid_JDG, "")
{
	bool deadYet;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( deadYet, 1 );
	}

	void Created( GameObject * obj ) override
	{
		deadYet = false;
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
		ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_MUTANT );
		//ScriptEngine::Enable_Cinematic_Freeze( obj, false );
	}

	void Killed( GameObject * /* obj */, GameObject * /* killer */ ) override
	{
		deadYet = true;
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_START_ACTING_JDG)
		{
			//ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_MUTANT );
			//ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_MUTANT );


			int myId = ScriptEngine::Get_ID ( obj );
			if (myId == 101995 && deadYet == false)
			{
				Vector3 warpSpot (-4.544f, 86.890f, -34);
				ScriptEngine::Set_Position ( obj, warpSpot );

				ActionParamsStruct params;

				params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
				//params.Set_Movement(Vector3 (-10.635f, 68.057f, -34.233f), RUN, 1);
				//making first change here...run to walk

				ScriptEngine::Action_Goto(obj, params);
				params.Set_Movement(Vector3 (-10.635f, 68.057f, -34.233f), WALK, 1);
			}

			else if (myId == 101996 && deadYet == false)
			{
				//Vector3 warpSpot (-2.457f, 84, -34);
				//change 2 going here
				Vector3 warpSpot (-7.272f, 77.365f, -34);
				ScriptEngine::Set_Position ( obj, warpSpot );

				ActionParamsStruct params;

				params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
				//params.Set_Movement(Vector3 (10.745f, 66.866f, -34.233f), RUN, 1);
				//making first change here...run to walk
				params.Set_Movement(Vector3 (10.745f, 66.866f, -34.233f), WALK, 1);

				ScriptEngine::Action_Goto(obj, params);
			}

			ScriptEngine::Enable_Cinematic_Freeze( obj, true );

		}

		else if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			GameObject * petrova = ScriptEngine::Find_Object ( M11_MUTANT_PETROVA_JDG );
			if (petrova != nullptr)
			{
				ActionParamsStruct params;

				params.Set_Basic(this, 100, M01_WALKING_WAYPATH_01_JDG);
				params.Set_Movement(petrova, RUN, 4);
				params.Set_Attack( petrova, 10, 0, true );

				ScriptEngine::Action_Attack(obj, params);
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		if (complete_reason == ACTION_COMPLETE_NORMAL)
		{
			if (action_id == M01_WALKING_WAYPATH_01_JDG)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, true );

			}
		}
	}
};

DECLARE_SCRIPT(M11_PowerupDifficulty_Manager_JDG, "")
{
	void Created( GameObject * /* obj */ ) override
	{
		int easy = 0;
		int medium = 1;

		int currentDifficulty = ScriptEngine::Get_Difficulty_Level( );

		if (currentDifficulty == easy)
		{
			ScriptEngine::Create_Object ( "POW_SniperRifle_Player", Vector3(17.778f, -41.716f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_SniperRifle_Player", Vector3(-17.799f, -41.177f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-11.209f, -16.475f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(10.747f, -16.773f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-3.467f, -33.332f, -10.015f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(3.651f, -33.292f, -10.015f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(1.954f, 19.875f, -18.000f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-1.897f, 20.016f, -18.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(12.768f, 5.348f, -24.531f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-13.378f, 5.055f, -24.531f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(7.661f, -17.279f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-8.177f, -17.193f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(2.911f, -32.665f, -34.789f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-1.058f, -32.588f, -34.789f) );

			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-14.646f, 40.156f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(15.466f, 40.827f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(7.671f, 23.377f, 1.594f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-8.166f, 23.366f, 1.256f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(6.072f, 13.298f, -10.015f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-6.474f, 13.163f, -10.015f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-9.831f, -17.845f, -18.000f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(9.240f, -17.551f, -18.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-3.556f, 16.234f, -24.531f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(3.966f, 15.895f, -24.530f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-10.515f, 33.298f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_SniperRifle_Player", Vector3(10.585f, 31.592f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_MineRemote_Player", Vector3(1.538f, -15.786f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_MineRemote_Player", Vector3(-0.604f, -15.760f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-13.821f, 8.995f, -34.233f) );
			//ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-13.821f, 8.995f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(14.085f, 9.259f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(19.320f, 17.879f, -39.484f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-19.301f, 17.910f, -39.484f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-12.110f, 54.177f, -39.486f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(11.853f, 54.077f, -39.486f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(96.844f, 51.430f, -62.686f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(94.033f, 43.255f, -62.686f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(27.027f, 60.014f, -55.106f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(14.259f, 58.072f, -53.022f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(3.860f, 98.276f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-3.987f, 97.681f, -34.233f) );

			ScriptEngine::Enable_Spawner( 102364, true );//lev 01 health
			ScriptEngine::Enable_Spawner( 102367, true );//lev 01 armor
			ScriptEngine::Enable_Spawner( 102370, true );//netrunner health
			ScriptEngine::Enable_Spawner( 102373, true );//netrunner armor
			ScriptEngine::Enable_Spawner( 102378, true );//wetbar armor
			ScriptEngine::Enable_Spawner( 102381, true );//wetbar health
			ScriptEngine::Enable_Spawner( 102384, true );//warroom armor
			ScriptEngine::Enable_Spawner( 102387, true );//warroom health
			ScriptEngine::Enable_Spawner( 102390, true );//mutant lab armor
			ScriptEngine::Enable_Spawner( 102393, true );//mutant lab health
			ScriptEngine::Enable_Spawner( 102396, true );//mutant crypt health01
			ScriptEngine::Enable_Spawner( 102399, true );//mutant crypt armor01
			ScriptEngine::Enable_Spawner( 102402, true );//mutant crypt health02
			ScriptEngine::Enable_Spawner( 102405, true );//mutant crypt health03
			ScriptEngine::Enable_Spawner( 102408, true );//mutant crypt armor02
			ScriptEngine::Enable_Spawner( 102411, true );//abel's tomb armor
			ScriptEngine::Enable_Spawner( 102414, true );//abel's tomb health
			ScriptEngine::Enable_Spawner( 102417, true );//powercore bottom health
			ScriptEngine::Enable_Spawner( 102420, true );//powercore bottom armor
			ScriptEngine::Enable_Spawner( 102423, true );//powercore top health
			ScriptEngine::Enable_Spawner( 102426, true );//powercore top armor
			ScriptEngine::Enable_Spawner( 102429, true );//powercore center health
			ScriptEngine::Enable_Spawner( 102432, true );//powercore center armor

		}

		else if (currentDifficulty == medium)
		{
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-14.646f, 40.156f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(15.466f, 40.827f, 0.000f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(7.671f, 23.377f, 1.594f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-8.166f, 23.366f, 1.256f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(6.072f, 13.298f, -10.015f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-6.474f, 13.163f, -10.015f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-9.831f, -17.845f, -18.000f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(9.240f, -17.551f, -18.000f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-3.556f, 16.234f, -24.531f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(3.966f, 15.895f, -24.530f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-10.515f, 33.298f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_SniperRifle_Player", Vector3(10.585f, 31.592f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_MineRemote_Player", Vector3(1.538f, -15.786f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_MineRemote_Player", Vector3(-0.604f, -15.760f, -29.855f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-13.821f, 8.995f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(-13.821f, 8.995f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(14.085f, 9.259f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(19.320f, 17.879f, -39.484f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-19.301f, 17.910f, -39.484f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-12.110f, 54.177f, -39.486f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(11.853f, 54.077f, -39.486f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(96.844f, 51.430f, -62.686f) );
			ScriptEngine::Create_Object ( "POW_RocketLauncher_Player", Vector3(94.033f, 43.255f, -62.686f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(27.027f, 60.014f, -55.106f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(14.259f, 58.072f, -53.022f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(3.860f, 98.276f, -34.233f) );
			ScriptEngine::Create_Object ( "POW_LaserChaingun_Player", Vector3(-3.987f, 97.681f, -34.233f) );

			ScriptEngine::Enable_Spawner( 102360, true );//lev 01 health
			ScriptEngine::Enable_Spawner( 102366, true );//lev 01 armor
			ScriptEngine::Enable_Spawner( 102369, true );//netrunner health
			ScriptEngine::Enable_Spawner( 102372, true );//netrunner armor
			ScriptEngine::Enable_Spawner( 102377, true );//wetbar armor
			ScriptEngine::Enable_Spawner( 102380, true );//wetbar health
			ScriptEngine::Enable_Spawner( 102383, true );//warroom armor
			ScriptEngine::Enable_Spawner( 102386, true );//warroom health
			ScriptEngine::Enable_Spawner( 102389, true );//mutant lab armor
			ScriptEngine::Enable_Spawner( 102392, true );//mutant lab health
			ScriptEngine::Enable_Spawner( 102395, true );//mutant crypt health01
			ScriptEngine::Enable_Spawner( 102398, true );//mutant crypt armor01
			ScriptEngine::Enable_Spawner( 102401, true );//mutant crypt health02
			ScriptEngine::Enable_Spawner( 102404, true );//mutant crypt health03
			ScriptEngine::Enable_Spawner( 102407, true );//mutant crypt armor02
			ScriptEngine::Enable_Spawner( 102410, true );//abel's tomb armor
			ScriptEngine::Enable_Spawner( 102413, true );//abel's tomb health
			ScriptEngine::Enable_Spawner( 102416, true );//powercore bottom health
			ScriptEngine::Enable_Spawner( 102419, true );//powercore bottom armor
			ScriptEngine::Enable_Spawner( 102422, true );//powercore top health
			ScriptEngine::Enable_Spawner( 102425, true );//powercore top armor
			ScriptEngine::Enable_Spawner( 102428, true );//powercore center health
			ScriptEngine::Enable_Spawner( 102431, true );//powercore center armor
		}

		else
		{
			ScriptEngine::Enable_Spawner( 102359, true );//lev 01 health
			ScriptEngine::Enable_Spawner( 102365, true );//lev 01 armor
			ScriptEngine::Enable_Spawner( 102368, true );//netrunner health
			ScriptEngine::Enable_Spawner( 102371, true );//netrunner armor
			ScriptEngine::Enable_Spawner( 102376, true );//wetbar armor
			ScriptEngine::Enable_Spawner( 102379, true );//wetbar health
			ScriptEngine::Enable_Spawner( 102382, true );//warroom armor
			ScriptEngine::Enable_Spawner( 102385, true );//warroom health
			ScriptEngine::Enable_Spawner( 102388, true );//mutant lab armor
			ScriptEngine::Enable_Spawner( 102391, true );//mutant lab health
			ScriptEngine::Enable_Spawner( 102394, true );//mutant crypt health01
			ScriptEngine::Enable_Spawner( 102397, true );//mutant crypt armor01
			ScriptEngine::Enable_Spawner( 102400, true );//mutant crypt health02
			ScriptEngine::Enable_Spawner( 102403, true );//mutant crypt health03
			ScriptEngine::Enable_Spawner( 102406, true );//mutant crypt armor02
			ScriptEngine::Enable_Spawner( 102409, true );//abel's tomb armor
			ScriptEngine::Enable_Spawner( 102412, true );//abel's tomb health
			ScriptEngine::Enable_Spawner( 102415, true );//powercore bottom health
			ScriptEngine::Enable_Spawner( 102418, true );//powercore bottom armor
			ScriptEngine::Enable_Spawner( 102421, true );//powercore top health
			ScriptEngine::Enable_Spawner( 102424, true );//powercore top armor
			ScriptEngine::Enable_Spawner( 102427, true );//powercore center health
			ScriptEngine::Enable_Spawner( 102430, true );//powercore center armor
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_Taunt_Controller_JDG, "")//103797
{
	bool commandClearance;
	bool PetrovaDead;
	int last;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(commandClearance, 1);
		SAVE_VARIABLE(PetrovaDead, 2);
		SAVE_VARIABLE(last, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		commandClearance = false;
		PetrovaDead = false;
		last = 20;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)
			{
				PetrovaDead = true;
			}

			else if (param == M01_START_ACTING_JDG)
			{
				commandClearance = true;
				ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG && commandClearance == true && PetrovaDead == false)
			{
				static constexpr const char* taunt_sound[9] =
				{
					"M00MBPT_ATOC0001I1MBPT_SND",
					"M00MBPT_ATOS0001I1MBPT_SND",
					"M00MBPT_DECX0001I1MBPT_SND",
					"M00MBPT_TFEA0001I1MBPT_SND",
					"M00ASQR_KILL0018I1MBPT_SND",
					"M00GSEN_KIOV0004I1MBPT_SND",
					"M00GCTK_KIOV0004I1MBPT_SND",
					"M00GSRS_KITB0004I1MBPT_SND",
					"M00GSRS_KITB0004I1MBPT_SND",
				};

				int random = ScriptEngine::Get_Random_Int(0, 9);

				while (random == last) {
					random = ScriptEngine::Get_Random_Int(0, 9);
				}

				GameObject * petrova = ScriptEngine::Find_Object ( M11_MUTANT_PETROVA_JDG );
				if (petrova != nullptr)
				{
					Vector3 petrovaLocation = ScriptEngine::Get_Position ( petrova );
					ScriptEngine::Create_Sound ( taunt_sound[random], petrovaLocation, petrova );

					float delayTimer = ScriptEngine::Get_Random ( 5, 10 );
					ScriptEngine::Send_Custom_Event ( obj, obj,  0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_Lab_Cryochamber_Switch02_JDG, "")
{
	bool poked;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(poked, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 0 );//green
		poked = false;
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if ( poker == STAR && poked == false)
		{
			ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
			poked = true;
			ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
			//ScriptEngine::Create_Sound ( "bombit1", Vector3 (0,0,0), obj );

			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
		}
	}
};

Vector3 m11cryoChamberLocs[15] = {
	Vector3(-17.385f, 34.254f, -39.486f),
	Vector3(-17.373f, 40.853f, -39.486f),
	Vector3(-17.362f, 50.281f, -39.486f),
	Vector3(-17.377f, 56.349f, -39.486f),
	Vector3(-9.668f, 59.078f, -39.486f),
	Vector3(-7.215f, 54.836f, -39.486f),
	Vector3(-5.012f, 48.971f, -39.486f),
	Vector3(-5.669f, 44.153f, -39.486f),
	Vector3(4.735f, 30.950f, -39.486f),
	Vector3(6.117f, 43.680f, -39.486f),
	Vector3(5.037f, 49.157f, -39.486f),
	Vector3(5.849f, 52.512f, -39.486f),
	Vector3(8.617f, 57.247f, -39.486f),
	Vector3(17.392f, 55.537f, -39.486f),
	Vector3(17.386f, 44.152f, -39.486f),
};

float cryoChamberFacing[15] = {
	90,
	90,
	90,
	90,
	-60,
	-60,
	-90,
	-135,
	40,
	140,
	90,
	60,
	60,
	-90,
	-90,
};

Vector3 destroyedCryoChamberLocs[3] = {
	Vector3(-4.544f, 30.811f, -39.486f),
	Vector3(17.399f, 50.261f, -39.486f),
	Vector3(17.400f, 38.322f, -39.486f),
};

float destroyedChamberFacing[3] = {
	-45,
	-90,
	-90,
};

DECLARE_SCRIPT(M11_Lab_Cryochamber_Controller_JDG, "")//100910 M11_LABORATORY_MUTANT_CONTROLLER_JDG
{
	int cryoChamber_id[15];
	int simpleMutant_id[15];
	int activeUnitCount;
	int deadMutantCount;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(activeUnitCount, 1);
		SAVE_VARIABLE(deadMutantCount, 2);
		SAVE_VARIABLE(cryoChamber_id[0], 3);
		SAVE_VARIABLE(cryoChamber_id[1], 4);
		SAVE_VARIABLE(cryoChamber_id[2], 5);
		SAVE_VARIABLE(cryoChamber_id[3], 6);
		SAVE_VARIABLE(cryoChamber_id[4], 7);
		SAVE_VARIABLE(cryoChamber_id[5], 8);
		SAVE_VARIABLE(cryoChamber_id[6], 9);
		SAVE_VARIABLE(cryoChamber_id[7], 10);
		SAVE_VARIABLE(cryoChamber_id[8], 11);
		SAVE_VARIABLE(cryoChamber_id[9], 12);
		SAVE_VARIABLE(cryoChamber_id[10], 13);
		SAVE_VARIABLE(cryoChamber_id[11], 14);
		SAVE_VARIABLE(cryoChamber_id[12], 15);
		SAVE_VARIABLE(cryoChamber_id[13], 16);
		SAVE_VARIABLE(cryoChamber_id[14], 17);
		SAVE_VARIABLE(simpleMutant_id[0], 18);
		SAVE_VARIABLE(simpleMutant_id[1], 19);
		SAVE_VARIABLE(simpleMutant_id[2], 20);
		SAVE_VARIABLE(simpleMutant_id[3], 21);
		SAVE_VARIABLE(simpleMutant_id[4], 22);
		SAVE_VARIABLE(simpleMutant_id[5], 23);
		SAVE_VARIABLE(simpleMutant_id[6], 24);
		SAVE_VARIABLE(simpleMutant_id[7], 25);
		SAVE_VARIABLE(simpleMutant_id[8], 26);
		SAVE_VARIABLE(simpleMutant_id[9], 27);
		SAVE_VARIABLE(simpleMutant_id[10], 28);
		SAVE_VARIABLE(simpleMutant_id[11], 29);
		SAVE_VARIABLE(simpleMutant_id[12], 30);
		SAVE_VARIABLE(simpleMutant_id[13], 31);
		SAVE_VARIABLE(simpleMutant_id[14], 32);
	}

	void Created( GameObject * /* obj */ ) override
	{
		GameObject * cryoChamber_00 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[0] );
		ScriptEngine::Set_Facing ( cryoChamber_00, cryoChamberFacing[0] );
		cryoChamber_id[0] = ScriptEngine::Get_ID ( cryoChamber_00 );
		ScriptEngine::Attach_Script(cryoChamber_00, "M11_Cryochamber_Simple_JDG", "0");
		GameObject * cryoChamber_01 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[1] );
		ScriptEngine::Set_Facing ( cryoChamber_01, cryoChamberFacing[1] );
		cryoChamber_id[1] = ScriptEngine::Get_ID ( cryoChamber_01 );
		ScriptEngine::Attach_Script(cryoChamber_01, "M11_Cryochamber_Simple_JDG", "1");
		GameObject * cryoChamber_02 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[2] );
		ScriptEngine::Set_Facing ( cryoChamber_02, cryoChamberFacing[2] );
		cryoChamber_id[2] = ScriptEngine::Get_ID ( cryoChamber_02 );
		ScriptEngine::Attach_Script(cryoChamber_02, "M11_Cryochamber_Simple_JDG", "2");
		GameObject * cryoChamber_03 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[3] );
		ScriptEngine::Set_Facing ( cryoChamber_03, cryoChamberFacing[3] );
		cryoChamber_id[3] = ScriptEngine::Get_ID ( cryoChamber_03 );
		ScriptEngine::Attach_Script(cryoChamber_03, "M11_Cryochamber_Simple_JDG", "3");
		GameObject * cryoChamber_04 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[4] );
		ScriptEngine::Set_Facing ( cryoChamber_04, cryoChamberFacing[4] );
		cryoChamber_id[4] = ScriptEngine::Get_ID ( cryoChamber_04 );
		ScriptEngine::Attach_Script(cryoChamber_04, "M11_Cryochamber_Simple_JDG", "4");
		GameObject * cryoChamber_05 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[5] );
		ScriptEngine::Set_Facing ( cryoChamber_05, cryoChamberFacing[5] );
		cryoChamber_id[5] = ScriptEngine::Get_ID ( cryoChamber_05 );
		ScriptEngine::Attach_Script(cryoChamber_05, "M11_Cryochamber_Simple_JDG", "5");
		GameObject * cryoChamber_06 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[6] );
		ScriptEngine::Set_Facing ( cryoChamber_06, cryoChamberFacing[6] );
		cryoChamber_id[6] = ScriptEngine::Get_ID ( cryoChamber_06 );
		ScriptEngine::Attach_Script(cryoChamber_06, "M11_Cryochamber_Simple_JDG", "6");
		GameObject * cryoChamber_07 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[7] );
		ScriptEngine::Set_Facing ( cryoChamber_07, cryoChamberFacing[7] );
		cryoChamber_id[7] = ScriptEngine::Get_ID ( cryoChamber_07 );
		ScriptEngine::Attach_Script(cryoChamber_07, "M11_Cryochamber_Simple_JDG", "7");
		GameObject * cryoChamber_08 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[8] );
		ScriptEngine::Set_Facing ( cryoChamber_08, cryoChamberFacing[8] );
		cryoChamber_id[8] = ScriptEngine::Get_ID ( cryoChamber_08 );
		ScriptEngine::Attach_Script(cryoChamber_08, "M11_Cryochamber_Simple_JDG", "8");
		GameObject * cryoChamber_09 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[9] );
		ScriptEngine::Set_Facing ( cryoChamber_09, cryoChamberFacing[9] );
		cryoChamber_id[9] = ScriptEngine::Get_ID ( cryoChamber_09 );
		ScriptEngine::Attach_Script(cryoChamber_09, "M11_Cryochamber_Simple_JDG", "9");
		GameObject * cryoChamber_10 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[10] );
		ScriptEngine::Set_Facing ( cryoChamber_10, cryoChamberFacing[10] );
		cryoChamber_id[10] = ScriptEngine::Get_ID ( cryoChamber_10 );
		ScriptEngine::Attach_Script(cryoChamber_10, "M11_Cryochamber_Simple_JDG", "10");
		GameObject * cryoChamber_11 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[11] );
		ScriptEngine::Set_Facing ( cryoChamber_11, cryoChamberFacing[11] );
		cryoChamber_id[11] = ScriptEngine::Get_ID ( cryoChamber_11 );
		ScriptEngine::Attach_Script(cryoChamber_11, "M11_Cryochamber_Simple_JDG", "11");
		GameObject * cryoChamber_12 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[12] );
		ScriptEngine::Set_Facing ( cryoChamber_12, cryoChamberFacing[12] );
		cryoChamber_id[12] = ScriptEngine::Get_ID ( cryoChamber_12 );
		ScriptEngine::Attach_Script(cryoChamber_12, "M11_Cryochamber_Simple_JDG", "12");
		GameObject * cryoChamber_13 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[13] );
		ScriptEngine::Set_Facing ( cryoChamber_13, cryoChamberFacing[13] );
		cryoChamber_id[13] = ScriptEngine::Get_ID ( cryoChamber_13 );
		ScriptEngine::Attach_Script(cryoChamber_13, "M11_Cryochamber_Simple_JDG", "13");
		GameObject * cryoChamber_14 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber", m11cryoChamberLocs[14] );
		ScriptEngine::Set_Facing ( cryoChamber_14, cryoChamberFacing[14] );
		cryoChamber_id[14] = ScriptEngine::Get_ID ( cryoChamber_14 );
		ScriptEngine::Attach_Script(cryoChamber_14, "M11_Cryochamber_Simple_JDG", "14");

		GameObject * cryoTrajectory_00 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[0] );
		ScriptEngine::Set_Model ( cryoTrajectory_00, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_00, cryoChamberFacing[0] );
		GameObject * simpleMutant_00 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_00, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_00, cryoTrajectory_00, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_00, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[0] = ScriptEngine::Get_ID ( simpleMutant_00 );
		if (cryoTrajectory_00 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_00 );
		}

		GameObject * cryoTrajectory_01 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[1] );
		ScriptEngine::Set_Model ( cryoTrajectory_01, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_01, cryoChamberFacing[1] );
		GameObject * simpleMutant_01 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_01, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_01, cryoTrajectory_01, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_01, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[1] = ScriptEngine::Get_ID ( simpleMutant_01 );
		if (cryoTrajectory_01 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_01 );
		}

		GameObject * cryoTrajectory_02 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[2] );
		ScriptEngine::Set_Model ( cryoTrajectory_02, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_02, cryoChamberFacing[2] );
		GameObject * simpleMutant_02 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_02, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_02, cryoTrajectory_02, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_02, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[2] = ScriptEngine::Get_ID ( simpleMutant_02 );
		if (cryoTrajectory_02 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_02 );
		}

		GameObject * cryoTrajectory_03 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[3] );
		ScriptEngine::Set_Model ( cryoTrajectory_03, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_03, cryoChamberFacing[3] );
		GameObject * simpleMutant_03 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_03, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_03, cryoTrajectory_03, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_03, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[3] = ScriptEngine::Get_ID ( simpleMutant_03 );
		if (cryoTrajectory_03 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_03 );
		}

		GameObject * cryoTrajectory_04 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[4] );
		ScriptEngine::Set_Model ( cryoTrajectory_04, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_04, cryoChamberFacing[4] );
		GameObject * simpleMutant_04 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_04, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_04, cryoTrajectory_04, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_04, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[4] = ScriptEngine::Get_ID ( simpleMutant_04 );
		if (cryoTrajectory_04 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_04 );
		}

		GameObject * cryoTrajectory_05 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[5] );
		ScriptEngine::Set_Model ( cryoTrajectory_05, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_05, cryoChamberFacing[5] );
		GameObject * simpleMutant_05 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_05, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_05, cryoTrajectory_05, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_05, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[5] = ScriptEngine::Get_ID ( simpleMutant_05 );
		if (cryoTrajectory_05 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_05 );
		}

		GameObject * cryoTrajectory_06 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[6] );
		ScriptEngine::Set_Model ( cryoTrajectory_06, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_06, cryoChamberFacing[6] );
		GameObject * simpleMutant_06 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_06, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_06, cryoTrajectory_06, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_06, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[6] = ScriptEngine::Get_ID ( simpleMutant_06 );
		if (cryoTrajectory_06 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_06 );
		}

		GameObject * cryoTrajectory_07 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[7] );
		ScriptEngine::Set_Model ( cryoTrajectory_07, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_07, cryoChamberFacing[7] );
		GameObject * simpleMutant_07 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_07, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_07, cryoTrajectory_07, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_07, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[7] = ScriptEngine::Get_ID ( simpleMutant_07 );
		if (cryoTrajectory_07 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_07 );
		}

		GameObject * cryoTrajectory_08 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[8] );
		ScriptEngine::Set_Model ( cryoTrajectory_08, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_08, cryoChamberFacing[8] );
		GameObject * simpleMutant_08 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_08, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_08, cryoTrajectory_08, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_08, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[8] = ScriptEngine::Get_ID ( simpleMutant_08 );
		if (cryoTrajectory_08 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_08 );
		}

		GameObject * cryoTrajectory_09 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[9] );
		ScriptEngine::Set_Model ( cryoTrajectory_09, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_09, cryoChamberFacing[9] );
		GameObject * simpleMutant_09 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_09, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_09, cryoTrajectory_09, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_09, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[9] = ScriptEngine::Get_ID ( simpleMutant_09 );
		if (cryoTrajectory_09 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_09 );
		}

		GameObject * cryoTrajectory_10 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[10] );
		ScriptEngine::Set_Model ( cryoTrajectory_10, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_10, cryoChamberFacing[10] );
		GameObject * simpleMutant_10 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_10, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_10, cryoTrajectory_10, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_10, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[10] = ScriptEngine::Get_ID ( simpleMutant_10 );
		if (cryoTrajectory_10 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_10 );
		}

		GameObject * cryoTrajectory_11 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[11] );
		ScriptEngine::Set_Model ( cryoTrajectory_11, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_11, cryoChamberFacing[11] );
		GameObject * simpleMutant_11 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_11, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_11, cryoTrajectory_11, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_11, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[11] = ScriptEngine::Get_ID ( simpleMutant_11 );
		if (cryoTrajectory_11 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_11 );
		}

		GameObject * cryoTrajectory_12 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[12] );
		ScriptEngine::Set_Model ( cryoTrajectory_12, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_12, cryoChamberFacing[12] );
		GameObject * simpleMutant_12 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_12, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_12, cryoTrajectory_12, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_12, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[12] = ScriptEngine::Get_ID ( simpleMutant_12 );
		if (cryoTrajectory_12 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_12 );
		}

		GameObject * cryoTrajectory_13 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[13] );
		ScriptEngine::Set_Model ( cryoTrajectory_13, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_13, cryoChamberFacing[13] );
		GameObject * simpleMutant_13 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_13, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_13, cryoTrajectory_13, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_13, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[13] = ScriptEngine::Get_ID ( simpleMutant_13 );
		if (cryoTrajectory_13 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_13 );
		}

		GameObject * cryoTrajectory_14 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[14] );
		ScriptEngine::Set_Model ( cryoTrajectory_14, "X11E_Trajectory" );
		ScriptEngine::Set_Facing ( cryoTrajectory_14, cryoChamberFacing[14] );
		GameObject * simpleMutant_14 = ScriptEngine::Create_Object_At_Bone ( cryoTrajectory_14, "Mutant_2SF_Templar", "BN_Trajectory" );
		ScriptEngine::Attach_To_Object_Bone( simpleMutant_14, cryoTrajectory_14, "BN_Trajectory" );
		ScriptEngine::Attach_Script(simpleMutant_14, "M11_LabMutant_Simple_JDG", "");
		simpleMutant_id[14] = ScriptEngine::Get_ID ( simpleMutant_14 );
		if (cryoTrajectory_14 != nullptr)
		{
			ScriptEngine::Destroy_Object ( cryoTrajectory_14 );
		}

		GameObject * destroyedCryoChamber_01 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", destroyedCryoChamberLocs[0] );
		ScriptEngine::Set_Facing ( destroyedCryoChamber_01, destroyedChamberFacing[0] );
		ScriptEngine::Attach_Script(destroyedCryoChamber_01, "M11_Cryochamber_DestroyedSimple_JDG", "");
		GameObject * destroyedCryoChamber_02 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", destroyedCryoChamberLocs[1] );
		ScriptEngine::Set_Facing ( destroyedCryoChamber_02, destroyedChamberFacing[1] );
		ScriptEngine::Attach_Script(destroyedCryoChamber_02, "M11_Cryochamber_DestroyedSimple_JDG", "");
		GameObject * destroyedCryoChamber_03 = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", destroyedCryoChamberLocs[2] );
		ScriptEngine::Set_Facing ( destroyedCryoChamber_03, destroyedChamberFacing[2] );
		ScriptEngine::Attach_Script(destroyedCryoChamber_03, "M11_Cryochamber_DestroyedSimple_JDG", "");

		activeUnitCount = 3;
		deadMutantCount = 0;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == M01_IVE_BEEN_KILLED_JDG)//a chamber has been destroyed--kill the hibernating mutant
		{
			if (param == 0)//cryo chamber 00 is destroyed
			{
				GameObject * cryoTrajectory_00 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[0] );
				ScriptEngine::Set_Model ( cryoTrajectory_00, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_00, "M11_Mutant_TrajectoryBone_JDG", "");//"Mutant_2SF_Templar"
				ScriptEngine::Set_Facing ( cryoTrajectory_00, cryoChamberFacing[0] );
				if (cryoTrajectory_00 != nullptr)
				{
					GameObject * simpleMutant_00 = ScriptEngine::Find_Object ( simpleMutant_id[0] );
					if (simpleMutant_00 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_00, cryoTrajectory_00, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_00, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 1)//cryo chamber 01 is destroyed
			{
				GameObject * cryoTrajectory_01 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[1] );
				ScriptEngine::Set_Model ( cryoTrajectory_01, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_01, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_01, cryoChamberFacing[1] );
				if (cryoTrajectory_01 != nullptr)
				{
					GameObject * simpleMutant_01 = ScriptEngine::Find_Object ( simpleMutant_id[1] );
					if (simpleMutant_01 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_01, cryoTrajectory_01, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_01, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 2)//cryo chamber 02 is destroyed
			{
				GameObject * cryoTrajectory_02 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[2] );
				ScriptEngine::Set_Model ( cryoTrajectory_02, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_02, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_02, cryoChamberFacing[2] );
				if (cryoTrajectory_02 != nullptr)
				{
					GameObject * simpleMutant_02 = ScriptEngine::Find_Object ( simpleMutant_id[2] );
					if (simpleMutant_02 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_02, cryoTrajectory_02, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_02, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 3)//cryo chamber 03 is destroyed
			{
				GameObject * cryoTrajectory_03 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[3] );
				ScriptEngine::Set_Model ( cryoTrajectory_03, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_03, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_03, cryoChamberFacing[3] );
				if (cryoTrajectory_03 != nullptr)
				{
					GameObject * simpleMutant_03 = ScriptEngine::Find_Object ( simpleMutant_id[3] );
					if (simpleMutant_03 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_03, cryoTrajectory_03, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_03, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 4)//cryo chamber 04 is destroyed
			{
				GameObject * cryoTrajectory_04 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[4] );
				ScriptEngine::Set_Model ( cryoTrajectory_04, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_04, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_04, cryoChamberFacing[4] );
				if (cryoTrajectory_04 != nullptr)
				{
					GameObject * simpleMutant_04 = ScriptEngine::Find_Object ( simpleMutant_id[4] );
					if (simpleMutant_04 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_04, cryoTrajectory_04, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_04, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 5)//cryo chamber 05 is destroyed
			{
				GameObject * cryoTrajectory_05 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[5] );
				ScriptEngine::Set_Model ( cryoTrajectory_05, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_05, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_05, cryoChamberFacing[5] );
				if (cryoTrajectory_05 != nullptr)
				{
					GameObject * simpleMutant_05 = ScriptEngine::Find_Object ( simpleMutant_id[5] );
					if (simpleMutant_05 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_05, cryoTrajectory_05, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_05, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 6)//cryo chamber 06 is destroyed
			{
				GameObject * cryoTrajectory_06 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[6] );
				ScriptEngine::Set_Model ( cryoTrajectory_06, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_06, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_06, cryoChamberFacing[6] );
				if (cryoTrajectory_06 != nullptr)
				{
					GameObject * simpleMutant_06 = ScriptEngine::Find_Object ( simpleMutant_id[6] );
					if (simpleMutant_06 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_06, cryoTrajectory_06, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_06, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 7)//cryo chamber 07 is destroyed
			{
				GameObject * cryoTrajectory_07 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[7] );
				ScriptEngine::Set_Model ( cryoTrajectory_07, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_07, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_07, cryoChamberFacing[7] );
				if (cryoTrajectory_07 != nullptr)
				{
					GameObject * simpleMutant_07 = ScriptEngine::Find_Object ( simpleMutant_id[7] );
					if (simpleMutant_07 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_07, cryoTrajectory_07, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_07, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 8)//cryo chamber 08 is destroyed
			{
				GameObject * cryoTrajectory_08 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[8] );
				ScriptEngine::Set_Model ( cryoTrajectory_08, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_08, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_08, cryoChamberFacing[8] );
				if (cryoTrajectory_08 != nullptr)
				{
					GameObject * simpleMutant_08 = ScriptEngine::Find_Object ( simpleMutant_id[8] );
					if (simpleMutant_08 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_08, cryoTrajectory_08, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_08, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 9)//cryo chamber 09 is destroyed
			{
				GameObject * cryoTrajectory_09 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[9] );
				ScriptEngine::Set_Model ( cryoTrajectory_09, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_09, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_09, cryoChamberFacing[9] );
				if (cryoTrajectory_09 != nullptr)
				{
					GameObject * simpleMutant_09 = ScriptEngine::Find_Object ( simpleMutant_id[9] );
					if (simpleMutant_09 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_09, cryoTrajectory_09, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_09, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 10)//cryo chamber 10 is destroyed
			{
				GameObject * cryoTrajectory_10 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[10] );
				ScriptEngine::Set_Model ( cryoTrajectory_10, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_10, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_10, cryoChamberFacing[10] );
				if (cryoTrajectory_10 != nullptr)
				{
					GameObject * simpleMutant_10 = ScriptEngine::Find_Object ( simpleMutant_id[10] );
					if (simpleMutant_10 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_10, cryoTrajectory_10, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_10, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 11)//cryo chamber 11 is destroyed
			{
				GameObject * cryoTrajectory_11 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[11] );
				ScriptEngine::Set_Model ( cryoTrajectory_11, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_11, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_11, cryoChamberFacing[11] );
				if (cryoTrajectory_11 != nullptr)
				{
					GameObject * simpleMutant_11 = ScriptEngine::Find_Object ( simpleMutant_id[11] );
					if (simpleMutant_11 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_11, cryoTrajectory_11, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_11, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 12)//cryo chamber 12 is destroyed
			{
				GameObject * cryoTrajectory_12 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[12] );
				ScriptEngine::Set_Model ( cryoTrajectory_12, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_12, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_12, cryoChamberFacing[12] );
				if (cryoTrajectory_12 != nullptr)
				{
					GameObject * simpleMutant_12 = ScriptEngine::Find_Object ( simpleMutant_id[12] );
					if (simpleMutant_12 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_12, cryoTrajectory_12, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_12, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 13)//cryo chamber 13 is destroyed
			{
				GameObject * cryoTrajectory_13 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[13] );
				ScriptEngine::Set_Model ( cryoTrajectory_13, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_13, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_13, cryoChamberFacing[13] );
				if (cryoTrajectory_13 != nullptr)
				{
					GameObject * simpleMutant_13 = ScriptEngine::Find_Object ( simpleMutant_id[13] );
					if (simpleMutant_13 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_13, cryoTrajectory_13, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_13, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}

			else if (param == 14)//cryo chamber 14 is destroyed
			{
				GameObject * cryoTrajectory_14 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[14] );
				ScriptEngine::Set_Model ( cryoTrajectory_14, "X11E_Trajectory" );
				ScriptEngine::Attach_Script(cryoTrajectory_14, "M11_Mutant_TrajectoryBone_JDG", "");
				ScriptEngine::Set_Facing ( cryoTrajectory_14, cryoChamberFacing[14] );
				if (cryoTrajectory_14 != nullptr)
				{
					GameObject * simpleMutant_14 = ScriptEngine::Find_Object ( simpleMutant_id[14] );
					if (simpleMutant_14 != nullptr)
					{
						ScriptEngine::Attach_To_Object_Bone( simpleMutant_14, cryoTrajectory_14, "BN_Trajectory" );
						deadMutantCount++;
						ScriptEngine::Send_Custom_Event ( obj, simpleMutant_14, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
					}
				}
			}
		}

		else if (type == 0)
		{
			if (param == M01_SPAWNER_IS_DEAD_JDG)
			{
				activeUnitCount--;
				deadMutantCount++;
				ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
			}

			else if (param == M01_SPAWNER_SPAWN_PLEASE_JDG)
			{
				if (activeUnitCount <= 6 && deadMutantCount < 15)
				{
					int random = ScriptEngine::Get_Random_Int(0, 15);

					GameObject * mutant = ScriptEngine::Find_Object ( simpleMutant_id[random] );
					GameObject * cryoChamber = ScriptEngine::Find_Object ( cryoChamber_id[random] );

					if (mutant == nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
					}

					else
					{
						simpleMutant_id[random] = 0;
						ScriptEngine::Send_Custom_Event( obj, mutant, M01_SPAWNER_SPAWN_PLEASE_JDG, random, 0 );
						ScriptEngine::Send_Custom_Event( obj, cryoChamber, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 1 );
						activeUnitCount++;
					}

					if (activeUnitCount <= 3)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_SPAWNER_SPAWN_PLEASE_JDG, 0 );
					}
				}

				else
				{
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)//switch 01 has been pushed--kill all related chambers
			{
				GameObject * cryoChamber_00 = ScriptEngine::Find_Object ( cryoChamber_id[0] );
				GameObject * cryoChamber_01 = ScriptEngine::Find_Object ( cryoChamber_id[1] );
				GameObject * cryoChamber_02 = ScriptEngine::Find_Object ( cryoChamber_id[2] );
				GameObject * cryoChamber_03 = ScriptEngine::Find_Object ( cryoChamber_id[3] );
				GameObject * cryoChamber_04 = ScriptEngine::Find_Object ( cryoChamber_id[4] );
				GameObject * cryoChamber_05 = ScriptEngine::Find_Object ( cryoChamber_id[5] );
				GameObject * cryoChamber_06 = ScriptEngine::Find_Object ( cryoChamber_id[6] );
				GameObject * cryoChamber_07 = ScriptEngine::Find_Object ( cryoChamber_id[7] );

				if (cryoChamber_00 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_00, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_01 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_01, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_02 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_02, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_03 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_03, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_04 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_04, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_05 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_05, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_06 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_06, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_07 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_07, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//switch 02 has been pushed--kill all related chambers
			{
				GameObject * cryoChamber_08 = ScriptEngine::Find_Object ( cryoChamber_id[8] );
				GameObject * cryoChamber_09 = ScriptEngine::Find_Object ( cryoChamber_id[9] );
				GameObject * cryoChamber_10 = ScriptEngine::Find_Object ( cryoChamber_id[10] );
				GameObject * cryoChamber_11 = ScriptEngine::Find_Object ( cryoChamber_id[11] );
				GameObject * cryoChamber_12 = ScriptEngine::Find_Object ( cryoChamber_id[12] );
				GameObject * cryoChamber_13 = ScriptEngine::Find_Object ( cryoChamber_id[13] );
				GameObject * cryoChamber_14 = ScriptEngine::Find_Object ( cryoChamber_id[14] );

				if (cryoChamber_08 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_08, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_09 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_09, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_10 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_10, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_11 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_11, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_12 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_12, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_13 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_13, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}

				if (cryoChamber_14 != nullptr)
				{
					float delayTimer = ScriptEngine::Get_Random ( 1, 3);
					ScriptEngine::Send_Custom_Event( obj, cryoChamber_14, 0, M01_MODIFY_YOUR_ACTION_JDG, delayTimer );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_Cryochamber_Simple_JDG, "chamber_number:int")
{
	int my_number;
	bool destroyed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(my_number, 1);
		SAVE_VARIABLE(destroyed, 2);
	}

	void Created( GameObject * obj ) override
	{
		my_number = Get_Int_Parameter("chamber_number");
		destroyed = false;
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", 0 );
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)//M01_SPAWNER_SPAWN_PLEASE_JDG
		{
			if (obj && destroyed == false)
			{
				destroyed = true;
				Vector3 myPosition = ScriptEngine::Get_Position ( obj );
				float myFacing = ScriptEngine::Get_Facing ( obj );
				ScriptEngine::Create_Explosion ( "Explosion_Barrel_Toxic", myPosition, nullptr );
				ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 15, false );
				ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), M01_IVE_BEEN_KILLED_JDG, my_number, 0 );
				ScriptEngine::Set_Health ( obj, 1 );

				GameObject * destroyedCryoChamber = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", myPosition );
				ScriptEngine::Set_Facing ( destroyedCryoChamber, myFacing );
				ScriptEngine::Attach_Script(destroyedCryoChamber, "M11_Cryochamber_DestroyedSimple_JDG", "");

				ScriptEngine::Destroy_Object ( obj );
			}
		}

		else if (param == M01_SPAWNER_SPAWN_PLEASE_JDG)//
		{
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			float myFacing = ScriptEngine::Get_Facing ( obj );
			GameObject * glass = ScriptEngine::Create_Object("Invisible_Object", myPosition );
			ScriptEngine::Set_Model ( glass, "X11E_Shatter" );
			ScriptEngine::Attach_Script(glass, "M11_Mutant_ShatterGlass_JDG", "");
			ScriptEngine::Set_Facing ( glass, myFacing );

			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 1 );
		}
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		float myCurrentHealth = ScriptEngine::Get_Health ( obj );
		float myMaxHeatlh = ScriptEngine::Get_Max_Health ( obj );
		float myHealthPercentage = (myCurrentHealth/myMaxHeatlh) * 100;

		if (myHealthPercentage > 25 && destroyed == false)
		{
			ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 9, false );
		}

		else if (destroyed == true)
		{
			ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 12, 15, false );
			ScriptEngine::Set_Health ( obj, 1 );
		}

		else if (destroyed == false)
		{
			destroyed = true;
			Vector3 myPosition = ScriptEngine::Get_Position ( obj );
			float myFacing = ScriptEngine::Get_Facing ( obj );
			ScriptEngine::Create_Explosion ( "Explosion_Barrel_Toxic", myPosition, nullptr );
			ScriptEngine::Set_Animation ( obj, "DSP_CRYOBIG2.DSP_CRYOBIG2", false, nullptr, 0, 15, false );
			ScriptEngine::Send_Custom_Event ( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), M01_IVE_BEEN_KILLED_JDG, my_number, 0 );
			ScriptEngine::Set_Health ( obj, 1 );

			GameObject * destroyedCryoChamber = ScriptEngine::Create_Object ( "Simple_Large_CryoChamber_Destroyed", myPosition );
			ScriptEngine::Set_Facing ( destroyedCryoChamber, myFacing );
			ScriptEngine::Attach_Script(destroyedCryoChamber, "M11_Cryochamber_DestroyedSimple_JDG", "");

			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_LabMutant_Simple_JDG, "")
{
	bool freed;
	bool sabotaged;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(freed, 1);
		SAVE_VARIABLE(sabotaged, 2);
	}

	void Created( GameObject * obj ) override
	{
		sabotaged = false;
		freed = false;
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Loiters_Allowed( obj, false );
		ScriptEngine::Select_Weapon ( obj, nullptr );
		ScriptEngine::Set_Animation_Frame ( obj, "S_A_HUMAN.H_A_822A", 15 );
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		if (freed != true)
		{
			float myMaxHealth = ScriptEngine::Get_Max_Health ( obj );
			float myMaxShield = ScriptEngine::Get_Max_Shield_Strength ( obj );
			ScriptEngine::Set_Health ( obj, myMaxHealth );
			ScriptEngine::Set_Shield_Strength ( obj, myMaxShield );
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		if (sabotaged == false)
		{
			if (freed == true)
			{
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object ( M11_LABORATORY_MUTANT_CONTROLLER_JDG ), 0, M01_SPAWNER_IS_DEAD_JDG, 0 );
			}

			Vector3 powerupSpawnLocation = ScriptEngine::Get_Position (obj );
			powerupSpawnLocation.Z += 0.75f;

			int easy = 0;
			int medium = 1;

			int currentDifficulty = ScriptEngine::Get_Difficulty_Level( );

			if (currentDifficulty == easy)
			{
				static constexpr const char *powerups[2] =
				{
					//this is a list of potential powerups to be dropped by lab mutants
					"POW_Health_100",
					"POW_Armor_100",
				};
				int random = ScriptEngine::Get_Random_Int(0, 2);
				ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
			}

			else if (currentDifficulty == medium)
			{
				static constexpr const char *powerups[2] =
				{
					//this is a list of potential powerups to be dropped by lab mutants
					"POW_Health_050",
					"POW_Armor_050",
				};
				int random = ScriptEngine::Get_Random_Int(0, 2);
				ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
			}

			else
			{
				static constexpr const char *powerups[2] =
				{
					//this is a list of potential powerups to be dropped by lab mutants
					"POW_Health_025",
					"POW_Armor_025",
				};
				int random = ScriptEngine::Get_Random_Int(0, 2);
				ScriptEngine::Create_Object ( powerups[random], powerupSpawnLocation );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == M01_SPAWNER_SPAWN_PLEASE_JDG)//you are being requested to come out and play--break out
		{
			ScriptEngine::Enable_Hibernation(obj, false );
			GameObject * cryoTrajectory_00 = ScriptEngine::Create_Object("Invisible_Object", m11cryoChamberLocs[param] );
			ScriptEngine::Set_Model ( cryoTrajectory_00, "X11E_Trajectory" );
			ScriptEngine::Attach_Script(cryoTrajectory_00, "M11_Mutant_TrajectoryBone_JDG", "");//"Mutant_2SF_Templar"
			ScriptEngine::Set_Facing ( cryoTrajectory_00, cryoChamberFacing[param] );
			if (cryoTrajectory_00 != nullptr)
			{
				if (obj)
				{
					ScriptEngine::Attach_To_Object_Bone( obj, cryoTrajectory_00, "BN_Trajectory" );
					ScriptEngine::Set_Animation ( obj, "S_C_Human.H_C_X11E_Escape", false, nullptr, 0, 120, false );
					freed = true;
				}
			}
		}

		else if (type == 0)
		{
			if (param == M01_IVE_BEEN_KILLED_JDG)//your chambers been destroyed--die please
			{
				sabotaged = true;
				freed = true;
				//ScriptEngine::Apply_Damage( obj, 10000, "BlamoKiller", nullptr );
				ScriptEngine::Set_Animation ( obj, "S_C_Human.H_C_Tubedie", false, nullptr, 0, 22, false );
			}
		}
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "S_C_Human.H_C_X11E_Escape") == 0)
		{
			ScriptEngine::Enable_Hibernation(obj, true );

			ScriptEngine::Select_Weapon ( obj, "Weapon_TiberiumAutoRifle_Ai" );
			ScriptEngine::Innate_Enable(obj);
			ActionParamsStruct params;
			params.Set_Basic( this, 85, M01_HUNT_THE_PLAYER_JDG );
			params.Set_Movement( STAR, RUN, 1 );
			params.Set_Attack( STAR, 15, 2, true );

			ScriptEngine::Action_Attack (  obj, params );
		}

		else if (stricmp(anim, "S_C_Human.H_C_Tubedie") == 0)
		{
			ScriptEngine::Destroy_Object ( obj );
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorController_JDG, "")//100698
{
	void Created( GameObject * /* obj */ ) override
	{
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_01_JDG	, 45, "MISS_ELEV1.MISS_ELEV1" );
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_02_JDG	, 45, "MISS_ELEV2.MISS_ELEV2" );
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_03_JDG	, 45, "MISS_ELEV3.MISS_ELEV3" );
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_04_JDG	, 45, "MISS_ELEV4.MISS_ELEV4" );
	}
};


DECLARE_SCRIPT(M11_Silo_ElevatorZone01_JDG, "")//100699
{
	bool sydeyIsInTheZone;
	bool havocIsInTheZone;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydeyIsInTheZone, 1);
		SAVE_VARIABLE(havocIsInTheZone, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydeyIsInTheZone = false;
		havocIsInTheZone = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			sydeyIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}

		else if (enterer == STAR)
		{
			havocIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Exited( GameObject * /* obj */, GameObject * exiter ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (exiter == sydney)
		{
			sydeyIsInTheZone = false;
		}

		else if (exiter == STAR)
		{
			havocIsInTheZone = false;
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			if ((sydeyIsInTheZone == true) && (havocIsInTheZone == true))
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( STAR );
				ScriptEngine::Create_Sound ( "Air_Compressor_01", myPosition, obj );
				ScriptEngine::Create_Sound ( "Metal_Thunk_03", myPosition, obj );
				ScriptEngine::Shake_Camera( myPosition, 25, 0.25f, 0.5f );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0.25f );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 2 );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_04_JDG	, 0, "MISS_ELEV4.MISS_ELEV4" );
			ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone02_JDG, "")//100700
{
	bool sydeyIsInTheZone;
	bool havocIsInTheZone;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydeyIsInTheZone, 1);
		SAVE_VARIABLE(havocIsInTheZone, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydeyIsInTheZone = false;
		havocIsInTheZone = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			sydeyIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}

		else if (enterer == STAR)
		{
			havocIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Exited( GameObject * /* obj */, GameObject * exiter ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (exiter == sydney)
		{
			sydeyIsInTheZone = false;
		}

		else if (exiter == STAR)
		{
			havocIsInTheZone = false;
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			if ((sydeyIsInTheZone == true) && (havocIsInTheZone == true))
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( STAR );
				ScriptEngine::Create_Sound ( "Air_Compressor_01", myPosition, obj );
				ScriptEngine::Create_Sound ( "Metal_Thunk_03", myPosition, obj );
				ScriptEngine::Shake_Camera( myPosition, 25, 0.25f, 0.5f );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0.25f );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 2 );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_03_JDG	, 0, "MISS_ELEV3.MISS_ELEV3" );
			ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone03_JDG, "")//100701
{
	bool sydeyIsInTheZone;
	bool havocIsInTheZone;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydeyIsInTheZone, 1);
		SAVE_VARIABLE(havocIsInTheZone, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydeyIsInTheZone = false;
		havocIsInTheZone = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			sydeyIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}

		else if (enterer == STAR)
		{
			havocIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Exited( GameObject * /* obj */, GameObject * exiter ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (exiter == sydney)
		{
			sydeyIsInTheZone = false;
		}

		else if (exiter == STAR)
		{
			havocIsInTheZone = false;
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			if ((sydeyIsInTheZone == true) && (havocIsInTheZone == true))
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( STAR );
				ScriptEngine::Create_Sound ( "Air_Compressor_01", myPosition, obj );
				ScriptEngine::Create_Sound ( "Metal_Thunk_03", myPosition, obj );
				ScriptEngine::Shake_Camera( myPosition, 25, 0.25f, 0.5f );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0.25f );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 2 );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_02_JDG	, 0, "MISS_ELEV2.MISS_ELEV2" );
			ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone04_JDG, "")//100702
{
	bool sydeyIsInTheZone;
	bool havocIsInTheZone;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydeyIsInTheZone, 1);
		SAVE_VARIABLE(havocIsInTheZone, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydeyIsInTheZone = false;
		havocIsInTheZone = false;
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			sydeyIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}

		else if (enterer == STAR)
		{
			havocIsInTheZone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}

	void Exited( GameObject * /* obj */, GameObject * exiter ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (exiter == sydney)
		{
			sydeyIsInTheZone = false;
		}

		else if (exiter == STAR)
		{
			havocIsInTheZone = false;
		}
	}

	void Custom( GameObject * obj, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			if ((sydeyIsInTheZone == true) && (havocIsInTheZone == true))
			{
				Vector3 myPosition = ScriptEngine::Get_Position ( STAR );
				ScriptEngine::Create_Sound ( "Air_Compressor_01", myPosition, obj );
				ScriptEngine::Create_Sound ( "Metal_Thunk_03", myPosition, obj );
				ScriptEngine::Shake_Camera( myPosition, 25, 0.25f, 0.5f );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0.25f );
			}

			else
			{
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 2 );
			}
		}

		else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
		{
			ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_01_JDG	, 0, "MISS_ELEV1.MISS_ELEV1" );
			ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_Rally_Zone_01_JDG, "")
{
	bool sydneyInPosition;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydneyInPosition, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydneyInPosition = false;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			sydneyInPosition = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && sydneyInPosition == true)
		{
			GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
			if (sydney != nullptr )
			{
				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_WALKING_WAYPATH_02_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_Rally_Zone_02_JDG, "")
{
	bool sydneyInPosition;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydneyInPosition, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydneyInPosition = false;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			sydneyInPosition = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && sydneyInPosition == true)
		{
			GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
			if (sydney != nullptr )
			{
				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_WALKING_WAYPATH_03_JDG, 0 );
			}

			ScriptEngine::Destroy_Object ( obj );//one time only zone--cleaning up
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_Rally_Zone_03_JDG, "")
{
	bool sydneyInPosition;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydneyInPosition, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydneyInPosition = false;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			sydneyInPosition = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && sydneyInPosition == true)
		{
			GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
			if (sydney != nullptr )
			{
				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_WALKING_WAYPATH_05_JDG, 0 );
			}

			GameObject * zoneOne = ScriptEngine::Find_Object ( 100133 );
			if (zoneOne != nullptr)
			{
				ScriptEngine::Destroy_Object ( zoneOne );//one time only zone--cleaning up
			}

			GameObject * zoneTwo = ScriptEngine::Find_Object ( 100134 );
			if (zoneTwo != nullptr)
			{
				ScriptEngine::Destroy_Object ( zoneTwo );//one time only zone--cleaning up
			}
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_Rally_Zone_03b_JDG, "")
{
	bool sydneyInPosition;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydneyInPosition, 1);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydneyInPosition = false;
	}

	void Custom( GameObject * /* obj */, int /* type */, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M01_MODIFY_YOUR_ACTION_JDG)
		{
			sydneyInPosition = true;
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		if (enterer == STAR && sydneyInPosition == true)
		{
			GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
			if (sydney != nullptr )
			{
				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_WALKING_WAYPATH_07_JDG, 0 );
			}

			GameObject * zoneOne = ScriptEngine::Find_Object ( 100133 );
			if (zoneOne != nullptr)
			{
				ScriptEngine::Destroy_Object ( zoneOne );//one time only zone--cleaning up
			}

			GameObject * zoneTwo = ScriptEngine::Find_Object ( 100134 );
			if (zoneTwo != nullptr)
			{
				ScriptEngine::Destroy_Object ( zoneTwo );//one time only zone--cleaning up
			}
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Controller_JDG, "")//106230
{//M11_PETROVA_STEALTHSPAWNER_01_JDG
	int initialSteatlthGuy01_id;
	int initialSteatlthGuy02_id;
	int initialSteatlthGuy03_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(initialSteatlthGuy01_id, 1);
		SAVE_VARIABLE(initialSteatlthGuy02_id, 2);
		SAVE_VARIABLE(initialSteatlthGuy03_id, 3);
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == 0)//text file is telling you to spawn initial guys
			{
				GameObject * initialSteatlthGuy01 = ScriptEngine::Trigger_Spawner( M11_PETROVA_STEALTHSPAWNER_01_JDG );
				ScriptEngine::Attach_Script(initialSteatlthGuy01, "M11_Petrova_StealthSoldier_Initial01_JDG", "");
				initialSteatlthGuy01_id = ScriptEngine::Get_ID ( initialSteatlthGuy01 );

				GameObject * initialSteatlthGuy02 = ScriptEngine::Trigger_Spawner( M11_PETROVA_STEALTHSPAWNER_02_JDG );
				ScriptEngine::Attach_Script(initialSteatlthGuy02, "M11_Petrova_StealthSoldier_Initial02_JDG", "");
				initialSteatlthGuy02_id = ScriptEngine::Get_ID ( initialSteatlthGuy02 );

				GameObject * initialSteatlthGuy03 = ScriptEngine::Trigger_Spawner( M11_PETROVA_STEALTHSPAWNER_03_JDG );
				ScriptEngine::Attach_Script(initialSteatlthGuy03, "M11_Petrova_StealthSoldier_Initial03_JDG", "");
				initialSteatlthGuy03_id = ScriptEngine::Get_ID ( initialSteatlthGuy03 );
			}

			else if (param == 1)
			{
				GameObject * initialSteatlthGuy01 = ScriptEngine::Find_Object ( initialSteatlthGuy01_id );
				if (initialSteatlthGuy01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, initialSteatlthGuy01, 0, M01_START_ACTING_JDG, 0 );
				}

				GameObject * initialSteatlthGuy02 = ScriptEngine::Find_Object ( initialSteatlthGuy02_id );
				if (initialSteatlthGuy02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, initialSteatlthGuy02, 0, M01_START_ACTING_JDG, 0 );
				}

				GameObject * initialSteatlthGuy03 = ScriptEngine::Find_Object ( initialSteatlthGuy03_id );
				if (initialSteatlthGuy03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, initialSteatlthGuy03, 0, M01_START_ACTING_JDG, 0 );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)//trigger spawner 01
			{
				GameObject * newSteatlthGuy = ScriptEngine::Trigger_Spawner( M11_PETROVA_STEALTHSPAWNER_01_JDG );
				ScriptEngine::Attach_Script(newSteatlthGuy, "M11_Petrova_StealthSoldier_Reinforcement01_JDG", "");
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//trigger spawner 02
			{
				GameObject * newSteatlthGuy = ScriptEngine::Trigger_Spawner( M11_PETROVA_STEALTHSPAWNER_02_JDG );
				ScriptEngine::Attach_Script(newSteatlthGuy, "M11_Petrova_StealthSoldier_Reinforcement02_JDG", "");
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)//trigger spawner 02
			{
				GameObject * newSteatlthGuy = ScriptEngine::Trigger_Spawner( M11_PETROVA_STEALTHSPAWNER_03_JDG );
				ScriptEngine::Attach_Script(newSteatlthGuy, "M11_Petrova_StealthSoldier_Reinforcement03_JDG", "");
			}
		}
	}
};



DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Initial01_JDG, "")//M11_PETROVA_STEALTHSPAWNER_01_JDG
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Attack (STAR, 0, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}//

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				ActionParamsStruct params;
				params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
				params.Set_Attack (STAR, 20, 0, true);
				ScriptEngine::Action_Attack (obj, params);
			}
		}
	}


	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, stealthController, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Initial02_JDG, "")//M11_PETROVA_STEALTHSPAWNER_02_JDG
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Attack (STAR, 0, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}//

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				ActionParamsStruct params;
				params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
				params.Set_Attack (STAR, 20, 0, true);
				ScriptEngine::Action_Attack (obj, params);
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, stealthController, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Initial03_JDG, "")//M11_PETROVA_STEALTHSPAWNER_03_JDG
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Attack (STAR, 0, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}//

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				ActionParamsStruct params;
				params.Set_Basic( this, 45, M01_DOING_ANIMATION_01_JDG );
				params.Set_Attack (STAR, 20, 0, true);
				ScriptEngine::Action_Attack (obj, params);
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, stealthController, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0 );
		}
	}
};


DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Reinforcement01_JDG, "")//M11_PETROVA_STEALTHSPAWNER_01_JDG
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Grant_Key( obj, 3, true );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Movement( STAR, RUN, 10 );
		params.Set_Attack (STAR, 20, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, stealthController, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Reinforcement02_JDG, "")//M11_PETROVA_STEALTHSPAWNER_02_JDG
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Grant_Key( obj, 3, true );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Movement( STAR, RUN, 10 );
		params.Set_Attack (STAR, 20, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, stealthController, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Petrova_StealthSoldier_Reinforcement03_JDG, "")//M11_PETROVA_STEALTHSPAWNER_03_JDG
{
	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		ScriptEngine::Grant_Key( obj, 3, true );
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Movement( STAR, RUN, 10 );
		params.Set_Attack (STAR, 20, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * stealthController = ScriptEngine::Find_Object ( 106230 );
		if (stealthController != nullptr)
		{
			ScriptEngine::Send_Custom_Event( obj, stealthController, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Sydney_Script_JDG, "")//M11_REAL_SYDNEY_MOBIUS_JDG 100644
{
	typedef enum {
		IDLE,
		GOING_TO_RALLY_POINT01,
		GOING_TO_RALLY_POINT02,
		GOING_TO_RALLY_POINT03,
		GOING_TO_ELEVATOR01,
		GOING_TO_ELEVATOR02,
		GOING_TO_ELEVATOR03,
		GOING_TO_ELEVATOR04,
		GOING_TO_MISSILE_SWITCH,

	} M11_Location;

	M11_Location sydneys_location;

	int sydney_conv01;
	int sydney_damaged_conv01;
	int sydney_damaged_conv02;
	int sydney_damaged_conv03;
	int missionEndConv;
	bool first_time_damaged;
	bool second_time_damaged;
	bool third_time_damaged;
	bool killedYet;
	bool sydney_damaged_conv01_playing;
	bool sydney_damaged_conv02_playing;
	bool sydney_damaged_conv03_playing;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydneys_location, 1);
		SAVE_VARIABLE(sydney_conv01, 2);
		SAVE_VARIABLE(sydney_damaged_conv01, 3);
		SAVE_VARIABLE(sydney_damaged_conv02, 4);
		SAVE_VARIABLE(sydney_damaged_conv03, 5);
		SAVE_VARIABLE(missionEndConv, 6);
		SAVE_VARIABLE(first_time_damaged, 7);
		SAVE_VARIABLE(second_time_damaged, 8);
		SAVE_VARIABLE(third_time_damaged, 9);
		SAVE_VARIABLE(killedYet, 10);
		SAVE_VARIABLE(sydney_damaged_conv01_playing, 11);
		SAVE_VARIABLE(sydney_damaged_conv02_playing, 12);
		SAVE_VARIABLE(sydney_damaged_conv03_playing, 13);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Set_Shield_Type ( obj, "Blamo" );
		sydneys_location = IDLE;
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Obj_Radar_Blip_Shape (obj, RADAR_BLIP_SHAPE_NONE );
		ScriptEngine::Set_Is_Visible( obj, false );
		ScriptEngine::Set_Is_Rendered( obj, false );

		ScriptEngine::Grant_Key( obj, 1, true );
		ScriptEngine::Grant_Key( obj, 2, true );
		ScriptEngine::Grant_Key( obj, 3, true );

		first_time_damaged = true;
		second_time_damaged = false;
		third_time_damaged = false;
		killedYet = false;
		sydney_damaged_conv01_playing = false;
		sydney_damaged_conv02_playing = false;
		sydney_damaged_conv03_playing = false;
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{	//M11_Sydney_Damaged_Conversation_01
		//M11_Sydney_Damaged_Conversation_02
		//M11_Sydney_Damaged_Conversation_03
		if (obj && killedYet == false)
		{
			if (first_time_damaged == true)
			{
				ScriptEngine::Set_HUD_Help_Text ( IDS_M11DSGN_DSGN1007I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
				first_time_damaged = false;
				sydney_damaged_conv01 = ScriptEngine::Create_Conversation( "M11_Sydney_Damaged_Conversation_01", 90, 1000, true);
				ScriptEngine::Join_Conversation( obj, sydney_damaged_conv01, false, true, true );
				ScriptEngine::Start_Conversation( sydney_damaged_conv01, sydney_damaged_conv01 );

				ScriptEngine::Monitor_Conversation( obj, sydney_damaged_conv01 );
				sydney_damaged_conv01_playing = true;
			}

			else if (second_time_damaged == true)
			{
				ScriptEngine::Set_HUD_Help_Text ( IDS_M11DSGN_DSGN1007I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
				second_time_damaged = false;
				sydney_damaged_conv02 = ScriptEngine::Create_Conversation( "M11_Sydney_Damaged_Conversation_02", 90, 1000, true);
				ScriptEngine::Join_Conversation( nullptr, sydney_damaged_conv02, false, false, false );
				ScriptEngine::Start_Conversation( sydney_damaged_conv02, sydney_damaged_conv02 );

				ScriptEngine::Monitor_Conversation( obj, sydney_damaged_conv02 );
				sydney_damaged_conv02_playing = true;

			}

			else if (third_time_damaged == true)
			{
				ScriptEngine::Set_HUD_Help_Text ( IDS_M11DSGN_DSGN1007I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
				third_time_damaged = false;
				sydney_damaged_conv03 = ScriptEngine::Create_Conversation( "M11_Sydney_Damaged_Conversation_03", 90, 1000, true);
				ScriptEngine::Join_Conversation( nullptr, sydney_damaged_conv03, false, false, false );
				ScriptEngine::Start_Conversation( sydney_damaged_conv03, sydney_damaged_conv03 );

				ScriptEngine::Monitor_Conversation( obj, sydney_damaged_conv03 );
				sydney_damaged_conv03_playing = true;
			}
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override //M11_Sydney_Dead_Conversation_01 and/or M11_Sydney_Dead_Conversation_02
	{
		ScriptEngine::Set_HUD_Help_Text ( IDS_M11DSGN_DSGN1008I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
		killedYet = true;

		if (sydney_damaged_conv01_playing == true)
		{
			ScriptEngine::Stop_Conversation(sydney_damaged_conv01);
		}

		else if (sydney_damaged_conv02_playing == true)
		{
			ScriptEngine::Stop_Conversation(sydney_damaged_conv02);
		}

		else if (sydney_damaged_conv03_playing == true)
		{
			ScriptEngine::Stop_Conversation(sydney_damaged_conv03);
		}

		int random = ScriptEngine::Get_Random_Int(0, 2);

		if (random == 0)
		{
			ScriptEngine::Create_2D_Sound ( "M11DSGN_DSGN0086I1GBRS_SND" );
		}

		else
		{
			ScriptEngine::Create_2D_Sound ( "M11DSGN_DSGN0022I1EVAG_SND" );
		}
		ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_MISSION_CONTROLLER_JDG), 0, M11_END_MISSION_FAIL_JDG, 5 );

	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		ActionParamsStruct params;

		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)//start initial conversation
			{
				if (obj && STAR)
				{
					ScriptEngine::Set_Shield_Type ( obj, "ShieldKevlar" );
					ScriptEngine::Set_Obj_Radar_Blip_Color ( obj, RADAR_BLIP_COLOR_GDI );
					//ScriptEngine::Innate_Enable(obj);
					sydney_conv01 = ScriptEngine::Create_Conversation( "M11_Initial_Sydney_Conversation_JDG", 100, 1000, false);
					ScriptEngine::Join_Conversation( obj, sydney_conv01, false, true, true );
					ScriptEngine::Join_Conversation( STAR, sydney_conv01, false, false, false );
					ScriptEngine::Join_Conversation( nullptr, sydney_conv01, false, false, false );
					ScriptEngine::Start_Conversation( sydney_conv01, sydney_conv01 );

					ScriptEngine::Monitor_Conversation( obj, sydney_conv01 );

					Vector3 siloProtector01_spot (-6.307f, 39.101f, 0);
					Vector3 siloProtector02_spot (-7.002f, 38.229f, -10.015f);
					Vector3 siloProtector03_spot (-6.596f, 38.117f, -10.015f);
					Vector3 siloProtector05_spot (7.324f, 38.026f, -18);
					Vector3 siloProtector07_spot (7.1f, 37.690f, -29.854f);
					Vector3 siloProtector08_spot (-0.197f, 44.313f, -29.854f);
					Vector3 siloProtector09_spot (-1.203f, 31.698f, -39.486f);

					GameObject * siloProtector01 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector01_spot );
					GameObject * siloProtector02 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector02_spot );
					GameObject * siloProtector03 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector03_spot );
					GameObject * siloProtector05 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector05_spot );
					GameObject * siloProtector07 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector07_spot );
					GameObject * siloProtector08 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector08_spot );
					GameObject * siloProtector09 = ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", siloProtector09_spot );

					ScriptEngine::Attach_Script(siloProtector01, "M11_NukeSilo_Protector_JDG", "");
					ScriptEngine::Attach_Script(siloProtector02, "M11_NukeSilo_Protector_JDG", "");
					ScriptEngine::Attach_Script(siloProtector03, "M11_NukeSilo_Protector_JDG", "");
					ScriptEngine::Attach_Script(siloProtector05, "M11_NukeSilo_Protector_JDG", "");
					ScriptEngine::Attach_Script(siloProtector07, "M11_NukeSilo_Protector_JDG", "");
					ScriptEngine::Attach_Script(siloProtector08, "M11_NukeSilo_Protector_JDG", "");
					ScriptEngine::Attach_Script(siloProtector09, "M11_NukeSilo_Protector_JDG", "");

					Vector3 labProtector01_spot (-0.722f, 20.642f, -34.233f);
					Vector3 labProtector02_spot (9.455f, 28.227f, -39.486f);

					ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", labProtector01_spot );
					ScriptEngine::Create_Object ( "Nod_FlameThrower_2SF", labProtector02_spot );
				}
			}

			else if (param == M01_WALKING_WAYPATH_02_JDG)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				int sydney_letsgo_conv = ScriptEngine::Create_Conversation( "M11_Sydney_WatchMyBack_Conversation", 100, 1000, false);
				ScriptEngine::Join_Conversation( obj, sydney_letsgo_conv, false, true, true );
				ScriptEngine::Start_Conversation( sydney_letsgo_conv, sydney_letsgo_conv );

				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_02_JDG );
				params.Set_Movement( Vector3 (0,0,0), RUN, 0.5f );

				int random = ScriptEngine::Get_Random_Int(0, 2);

				if (random == 0)
				{
					params.WaypathID = 100110;
					params.WaypointStartID = 100111;
					params.WaypointEndID = 100119;

				}

				else
				{
					params.WaypathID = 100121;
					params.WaypointStartID = 100122;
					params.WaypointEndID = 100130;
				}

				ScriptEngine::Action_Goto(obj, params);
			}

			else if (param == M01_WALKING_WAYPATH_03_JDG)
			{
				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				int random = ScriptEngine::Get_Random_Int(0, 2);

				if (random == 0)
				{
					params.Set_Basic( this, 100, M01_WALKING_WAYPATH_03_JDG );
					params.Set_Movement( Vector3 (0,0,0), RUN, 0.5f );
					params.WaypathID = 100135;
					params.WaypointStartID = 100136;
					params.WaypointEndID = 100149;

				}

				else
				{
					params.Set_Basic( this, 100, M01_WALKING_WAYPATH_04_JDG );
					params.Set_Movement( Vector3 (0,0,0), RUN, 0.5f );
					params.WaypathID = 100151;
					params.WaypointStartID = 100152;
					params.WaypointEndID = 100162;
				}

				ScriptEngine::Action_Goto(obj, params);
			}

			else if (param == M01_WALKING_WAYPATH_05_JDG)//now goto first silo elevator platform
			{
				sydneys_location = GOING_TO_ELEVATOR01;

				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				ScriptEngine::Debug_Message ( "***************************SYDNEY has received command to goto first elevator lift\n" );
				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_05_JDG );
				//params.Set_Movement( Vector3 (3.342f, 43.458f, -39.462f), RUN, 0.25f );
				params.Set_Movement( Vector3 (0,0,0), WALK, 0.25f );
				params.WaypathID = 105199;
				ScriptEngine::Action_Goto(obj, params);

				GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				if (elevatorDisabler != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevatorController = ScriptEngine::Find_Object ( M11_MISSILE_LIFT_CONTROLLER_JDG );
				if (elevatorController != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (param == M01_WALKING_WAYPATH_07_JDG)//now goto first silo elevator platform
			{
				sydneys_location = GOING_TO_ELEVATOR01;

				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				ScriptEngine::Debug_Message ( "***************************SYDNEY has received command to goto first elevator lift\n" );
				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_05_JDG );
				//params.Set_Movement( Vector3 (3.342f, 43.458f, -39.462f), RUN, 0.25f );
				params.Set_Movement( Vector3 (0,0,0), RUN, 0.25f );
				params.WaypathID = 105205;
				ScriptEngine::Action_Goto(obj, params);

				GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				if (elevatorDisabler != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				//ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_04_JDG	, 45, "MISS_ELEV4.MISS_ELEV4" );
				//ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_03_JDG	, 45, "MISS_ELEV3.MISS_ELEV3" );
				//ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_02_JDG	, 45, "MISS_ELEV2.MISS_ELEV2" );
				//ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_01_JDG	, 45, "MISS_ELEV1.MISS_ELEV1" );

				GameObject * elevatorController = ScriptEngine::Find_Object ( M11_MISSILE_LIFT_CONTROLLER_JDG );
				if (elevatorController != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
					ScriptEngine::Send_Custom_Event ( obj, elevatorController, 1, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_JDG)//sydney on level 2--goto next elevator
			{
				sydneys_location = GOING_TO_ELEVATOR02;

				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_05_JDG );
				//params.Set_Movement( Vector3 (-4.767f, 42.337f, -29.854f), RUN, 0.5f );
				params.Set_Movement( Vector3 (0,0,0), RUN, 0.25f );
				params.WaypathID = 104839;
				ScriptEngine::Action_Goto(obj, params);

				//GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				//if (elevatorDisabler != nullptr)
				//{
				//	ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				//}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)//sydney on level 3--goto next elevator
			{
				sydneys_location = GOING_TO_ELEVATOR03;

				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_05_JDG );
				//params.Set_Movement( Vector3 (-4.632f, 33.498f, -18), RUN, 0.5f );
				params.Set_Movement( Vector3 (0,0,0), RUN, 0.25f );
				params.WaypathID = 104845;
				ScriptEngine::Action_Goto(obj, params);

				GameObject * ropeController = ScriptEngine::Find_Object ( 106908 );
				if (ropeController != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, ropeController, 0, M01_START_ACTING_JDG, 3 );
				}

				//GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				//if (elevatorDisabler != nullptr)
				//{
				//	ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				//}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_03_JDG)//sydney on level 4--goto next elevator
			{
				sydneys_location = GOING_TO_ELEVATOR04;

				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
				params.Set_Basic( this, 100, M01_WALKING_WAYPATH_05_JDG );
				//params.Set_Movement( Vector3 (4.042f, 32.885f, -10.015f), RUN, 0.5f );
				params.Set_Movement( Vector3 (0,0,0), RUN, 0.25f );
				params.WaypathID = 104850;
				ScriptEngine::Action_Goto(obj, params);

				//GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				//if (elevatorDisabler != nullptr)
				//{
				//	ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				//}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//sydney on top level 4--goto nuke switch
			{
				sydneys_location = GOING_TO_MISSILE_SWITCH;

				GameObject * nukeSwitch = ScriptEngine::Find_Object ( M11_END_MISSION_SWITCH_JDG );
				if (nukeSwitch != nullptr)
				{
					Vector3 nukePosition  (-0.113f, 29.391f, 0.046f);
					ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
					params.Set_Basic( this, 100, M01_WALKING_WAYPATH_06_JDG );
					params.Set_Movement( nukePosition, RUN, 0.25f );
					ScriptEngine::Action_Goto(obj, params);
				}
			}

			else if (param == M01_MODIFY_YOUR_ACTION_05_JDG)//sydney should now be facing nuke console--play anim
			{
				params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
				params.Set_Animation ("S_A_HUMAN.H_A_CON2", false);
				ScriptEngine::Action_Play_Animation (obj, params);
			}
		}
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason complete_reason ) override
	{
		ActionParamsStruct params;

		switch (complete_reason)
		{
			case ACTION_COMPLETE_CONVERSATION_ENDED:
				{
					if (action_id == sydney_conv01)
					{
						ScriptEngine::Set_Innate_Is_Stationary (obj, false);
						ScriptEngine::Grant_Key( obj, 1, true );
						ScriptEngine::Grant_Key( obj, 2, true );
						ScriptEngine::Grant_Key( obj, 3, true );
						ScriptEngine::Grant_Key( obj, 4, true );
						ScriptEngine::Debug_Message ( "***************************initial SYDNEY conversation is over--going to zone 01\n" );

						params.Set_Basic( this, 100, M01_WALKING_WAYPATH_01_JDG );
						params.Set_Movement( Vector3 (0,0,0), RUN, 0.5f );
						params.WaypathID = 100541;
						params.WaypointStartID = 100542;
						params.WaypointEndID = 100544;

						ScriptEngine::Action_Goto(obj, params);

						GameObject * objectiveController = ScriptEngine::Find_Object ( M11_MISSION_CONTROLLER_JDG );
						if (objectiveController != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, objectiveController, 0, M11_END_SECOND_OBJECTIVE_JDG, 0 );
						}
					}

					else if (action_id == sydney_damaged_conv01)
					{
						second_time_damaged = true;
						sydney_damaged_conv01_playing = false;
					}

					else if (action_id == sydney_damaged_conv02)
					{
						third_time_damaged = true;
						sydney_damaged_conv02_playing = false;
					}

					else if (action_id == sydney_damaged_conv03) //missionEndConv
					{
						third_time_damaged = true;
						sydney_damaged_conv03_playing = false;
					}

					else if (action_id == missionEndConv)
					{
						params.Set_Basic( this, 100, M01_DOING_ANIMATION_02_JDG );
						params.Set_Animation ("S_A_HUMAN.H_A_CON2", false);
						ScriptEngine::Action_Play_Animation (obj, params);
					}
				}
				break;

			case ACTION_COMPLETE_PATH_BAD_START:
				{
					if (sydneys_location == GOING_TO_ELEVATOR01)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_WALKING_WAYPATH_05_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR02)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR03)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR04)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_MISSILE_SWITCH)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_04_JDG, 1 );
					}

				}
				break;

			case ACTION_COMPLETE_PATH_BAD_DEST:
				{
					if (sydneys_location == GOING_TO_ELEVATOR01)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_WALKING_WAYPATH_05_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR02)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR03)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR04)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_MISSILE_SWITCH)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_04_JDG, 1 );
					}
				}
				break;

			case ACTION_COMPLETE_MOVE_NO_PROGRESS_MADE:
				{
					if (sydneys_location == GOING_TO_ELEVATOR01)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_WALKING_WAYPATH_05_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR02)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR03)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_ELEVATOR04)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 1 );
					}

					else if (sydneys_location == GOING_TO_MISSILE_SWITCH)
					{
						ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_04_JDG, 1 );
					}
				}
				break;

			case ACTION_COMPLETE_NORMAL:
				{
					if (action_id == M01_WALKING_WAYPATH_01_JDG)
					{
						ScriptEngine::Set_Innate_Is_Stationary (obj, true);
						GameObject * rallyZone01 = ScriptEngine::Find_Object ( M11_SYDNEY_RALLY_ZONE_01_JDG );
						if (rallyZone01 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, rallyZone01, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}

						else
						{
							ScriptEngine::Debug_Message ( "**********************SYDNEY--cannot find rally zone 01\n" );
						}
					}

					else if (action_id == M01_WALKING_WAYPATH_02_JDG)
					{
						ScriptEngine::Set_Innate_Is_Stationary (obj, true);
						GameObject * rallyZone02 = ScriptEngine::Find_Object ( M11_SYDNEY_RALLY_ZONE_02_JDG );
						if (rallyZone02 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, rallyZone02, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}

						else
						{
							ScriptEngine::Debug_Message ( "**********************SYDNEY--cannot find rally zone 02\n" );
						}

						GameObject * objectiveController = ScriptEngine::Find_Object ( M11_MISSION_CONTROLLER_JDG );
						if (objectiveController != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, objectiveController, 0, M11_ADD_FIFTH_OBJECTIVE_JDG, 0 );
						}
					}

					else if (action_id == M01_WALKING_WAYPATH_03_JDG)
					{
						ScriptEngine::Set_Innate_Is_Stationary (obj, true);
						GameObject * rallyZone03 = ScriptEngine::Find_Object ( M11_SYDNEY_RALLY_ZONE_03A_JDG );
						if (rallyZone03 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, rallyZone03, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}

						else
						{
							ScriptEngine::Debug_Message ( "**********************SYDNEY--cannot find rally zone 03a\n" );
						}
					}

					else if (action_id == M01_WALKING_WAYPATH_04_JDG)
					{
						ScriptEngine::Set_Innate_Is_Stationary (obj, true);
						GameObject * rallyZone03 = ScriptEngine::Find_Object ( M11_SYDNEY_RALLY_ZONE_03B_JDG );
						if (rallyZone03 != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, rallyZone03, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
						}

						else
						{
							ScriptEngine::Debug_Message ( "**********************SYDNEY--cannot find rally zone 03b\n" );
						}
					}

					else if (action_id == M01_WALKING_WAYPATH_05_JDG)//sydneys at first elevator--goto stationary for now
					{
						ScriptEngine::Set_Innate_Is_Stationary (obj, true);

						GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
						if (elevatorDisabler != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_START_ACTING_JDG, 0 );
						}

					}

					else if (action_id == M01_WALKING_WAYPATH_06_JDG)//sydneys at missile switch-play animation
					{
						GameObject * nukeconsole = ScriptEngine::Find_Object ( 100106 );
						if (nukeconsole != nullptr)
						{
							ScriptEngine::Set_Innate_Is_Stationary (obj, true);
							params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
							params.Set_Attack (nukeconsole, 0, 0, true);
							ScriptEngine::Action_Attack (obj, params);

							ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_05_JDG, 1 );
						}

					}

					else if (action_id == M01_DOING_ANIMATION_01_JDG)//play end mission conversation
					{
						GameObject * nukeconsole = ScriptEngine::Find_Object ( 100106 );
						if (nukeconsole != nullptr)
						{
							ScriptEngine::Send_Custom_Event( obj, nukeconsole, 0, M01_START_ACTING_JDG, 0 );
						}

						missionEndConv = ScriptEngine::Create_Conversation( "M11_End_Mission_Conversation", 100, 1000, false);
						ScriptEngine::Join_Conversation( STAR, missionEndConv, false, false, true );
						ScriptEngine::Join_Conversation( obj, missionEndConv, false, false, true );
						ScriptEngine::Start_Conversation( missionEndConv,  missionEndConv );
						ScriptEngine::Monitor_Conversation (obj, missionEndConv);
					}

					else if (action_id == M01_DOING_ANIMATION_02_JDG)//end mission--success
					{
						ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (M11_MISSION_CONTROLLER_JDG), 0, M11_END_MISSION_PASS_JDG, 0 );
					}
				}
				break;
			default:
				break;
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorDeactivator_JDG, "")//105192
{
	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M11_START_ACTING_JDG)//allow switch usage
			{
				GameObject * elevator01_switch01 = ScriptEngine::Find_Object ( 101689 );
				if (elevator01_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch01, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator01_switch02 = ScriptEngine::Find_Object ( 101651 );
				if (elevator01_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch02, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator01_switch03 = ScriptEngine::Find_Object ( 101690 );
				if (elevator01_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch03, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator01_switch04 = ScriptEngine::Find_Object ( 101691 );
				if (elevator01_switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch04, 0, M11_START_ACTING_JDG, 0 );
				}

				//101692 101693 101694 101695
				GameObject * elevator02_switch01 = ScriptEngine::Find_Object ( 101692 );
				if (elevator02_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch01, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator02_switch02 = ScriptEngine::Find_Object ( 101693 );
				if (elevator02_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch02, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator02_switch03 = ScriptEngine::Find_Object ( 101694 );
				if (elevator02_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch03, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator02_switch04 = ScriptEngine::Find_Object ( 101695 );
				if (elevator02_switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch04, 0, M11_START_ACTING_JDG, 0 );
				}

				//101717 101718 101719 101720
				GameObject * elevator03_switch01 = ScriptEngine::Find_Object ( 101717 );
				if (elevator03_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch01, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator03_switch02 = ScriptEngine::Find_Object ( 101718 );
				if (elevator03_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch02, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator03_switch03 = ScriptEngine::Find_Object ( 101719 );
				if (elevator03_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch03, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator03_switch04 = ScriptEngine::Find_Object ( 101720 );
				if (elevator03_switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch04, 0, M11_START_ACTING_JDG, 0 );
				}

				//101721 101722 101723
				GameObject * elevator04_switch01 = ScriptEngine::Find_Object ( 101721 );
				if (elevator04_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator04_switch01, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator04_switch02 = ScriptEngine::Find_Object ( 101722 );
				if (elevator04_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator04_switch02, 0, M11_START_ACTING_JDG, 0 );
				}

				GameObject * elevator04_switch03 = ScriptEngine::Find_Object ( 101723 );
				if (elevator04_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator04_switch03, 0, M11_START_ACTING_JDG, 0 );
				}
			}

			if (param == M11_GOTO_IDLE_JDG)//disallow switch usage
			{
				GameObject * elevator01_switch01 = ScriptEngine::Find_Object ( 101689 );
				if (elevator01_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch01, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator01_switch02 = ScriptEngine::Find_Object ( 101651 );
				if (elevator01_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch02, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator01_switch03 = ScriptEngine::Find_Object ( 101690 );
				if (elevator01_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch03, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator01_switch04 = ScriptEngine::Find_Object ( 101691 );
				if (elevator01_switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator01_switch04, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				//101692 101693 101694 101695
				GameObject * elevator02_switch01 = ScriptEngine::Find_Object ( 101692 );
				if (elevator02_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch01, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator02_switch02 = ScriptEngine::Find_Object ( 101693 );
				if (elevator02_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch02, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator02_switch03 = ScriptEngine::Find_Object ( 101694 );
				if (elevator02_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch03, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator02_switch04 = ScriptEngine::Find_Object ( 101695 );
				if (elevator02_switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator02_switch04, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				//101717 101718 101719 101720
				GameObject * elevator03_switch01 = ScriptEngine::Find_Object ( 101717 );
				if (elevator03_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch01, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator03_switch02 = ScriptEngine::Find_Object ( 101718 );
				if (elevator03_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch02, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator03_switch03 = ScriptEngine::Find_Object ( 101719 );
				if (elevator03_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch03, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator03_switch04 = ScriptEngine::Find_Object ( 101720 );
				if (elevator03_switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator03_switch04, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				//101721 101722 101723
				GameObject * elevator04_switch01 = ScriptEngine::Find_Object ( 101721 );
				if (elevator04_switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator04_switch01, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator04_switch02 = ScriptEngine::Find_Object ( 101722 );
				if (elevator04_switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator04_switch02, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				GameObject * elevator04_switch03 = ScriptEngine::Find_Object ( 101723 );
				if (elevator04_switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevator04_switch03, 0, M11_GOTO_IDLE_JDG, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone01_Top_JDG, "")//100705
{
	bool sydney_in_zone;
	bool havoc_in_zone;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydney_in_zone, 1);
		SAVE_VARIABLE(havoc_in_zone, 2);
	}

	void Created( GameObject * /* obj */ ) override
	{
		sydney_in_zone = false;
		havoc_in_zone = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 0)
		{
			if (param == M01_MODIFY_YOUR_ACTION_JDG)
			{
				if (sydney_in_zone == true && havoc_in_zone == true)
				{
					GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
					if (elevatorDisabler != nullptr)
					{
						ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
					}

					GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
					if (sydney != nullptr )
					{
						ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
					}
				}
			}
		}
	}

	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			sydney_in_zone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0);

		}

		else if (enterer == STAR)
		{
			havoc_in_zone = true;
			ScriptEngine::Send_Custom_Event( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 0 );
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone02_Top_JDG, "")//100707
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			if (sydney != nullptr )
			{
				GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				if (elevatorDisabler != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_MODIFY_YOUR_ACTION_02_JDG, 0.25f );
				//ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
			}
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone03_Top_JDG, "")//100710
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			if (sydney != nullptr )
			{
				GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				if (elevatorDisabler != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_MODIFY_YOUR_ACTION_03_JDG, 0.25f );
				//ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
			}
		}
	}
};

DECLARE_SCRIPT(M11_Silo_ElevatorZone04_Top_JDG, "")//100712
{
	void Entered( GameObject * obj, GameObject * enterer ) override
	{
		GameObject * sydney = ScriptEngine::Find_Object ( M11_REAL_SYDNEY_MOBIUS_JDG );
		if (enterer == sydney)
		{
			if (sydney != nullptr )
			{
				GameObject * elevatorDisabler = ScriptEngine::Find_Object ( 105192 );
				if (elevatorDisabler != nullptr)
				{
					ScriptEngine::Send_Custom_Event( obj, elevatorDisabler, 0, M11_GOTO_IDLE_JDG, 0 );
				}

				ScriptEngine::Send_Custom_Event( obj, sydney, 0, M01_MODIFY_YOUR_ACTION_04_JDG, 0.25f );
				//ScriptEngine::Destroy_Object ( obj );//your job is over--clean up
			}
		}
	}
};

DECLARE_SCRIPT(M11_ForthFloor_Elevator_Switch_Script_JDG, "")//101689 101651 101690 101691
{
	bool sydney_in_motion;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydney_in_motion, 1);
	}

	void Created( GameObject * obj ) override
	{
		sydney_in_motion = false;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if (poker == STAR && sydney_in_motion == false)
		{
			GameObject * elevatorController = ScriptEngine::Find_Object ( M11_MISSILE_LIFT_CONTROLLER_JDG );
			if (elevatorController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, elevatorController, 0, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG)
		{
			if (type == 0)//elevator is moving--flash lights
			{
				ScriptEngine::Set_Animation ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", true, nullptr, 0.0f, -1.0f, false);
			}

			else if (type == 1)//elevator is at top--goto green
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 0 );//green
			}

			else if (type == 2)//elevator is at bottom--goto red
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
			}
		}

		else if (type == 0)
		{
			if (param == M11_GOTO_IDLE_JDG)
			{
				sydney_in_motion = true;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_START_ACTING_JDG, 8 );
			}

			else if (param == M11_START_ACTING_JDG)
			{
				sydney_in_motion = false;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
			}
		}
	}
};

DECLARE_SCRIPT(M11_ThirdFloor_Elevator_Switch_Script_JDG, "")//101692 101693 101694 101695
{
	bool sydney_in_motion;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydney_in_motion, 1);
	}

	void Created( GameObject * obj ) override
	{
		sydney_in_motion = false;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if (poker == STAR && sydney_in_motion == false)
		{
			GameObject * elevatorController = ScriptEngine::Find_Object ( M11_MISSILE_LIFT_CONTROLLER_JDG );
			if (elevatorController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, elevatorController, 0, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG)
		{
			if (type == 0)//elevator is moving--flash lights
			{
				ScriptEngine::Set_Animation ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", true, nullptr, 0.0f, -1.0f, false);
			}

			else if (type == 1)//elevator is at top--goto green
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 0 );//green
			}

			else if (type == 2)//elevator is at bottom--goto red
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
			}
		}

		else if (type == 0)
		{
			if (param == M11_GOTO_IDLE_JDG)
			{
				sydney_in_motion = true;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_START_ACTING_JDG, 8 );
			}

			else if (param == M11_START_ACTING_JDG)
			{
				sydney_in_motion = false;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
			}
		}
	}
};


DECLARE_SCRIPT(M11_SecondFloor_Elevator_Switch_Script_JDG, "")//101717 101718 101719 101720
{
	bool sydney_in_motion;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sydney_in_motion, 1);
	}

	void Created( GameObject * obj ) override
	{
		sydney_in_motion = false;
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if (poker == STAR && sydney_in_motion == false)
		{
			GameObject * elevatorController = ScriptEngine::Find_Object ( M11_MISSILE_LIFT_CONTROLLER_JDG );
			if (elevatorController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, elevatorController, 0, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG)
		{
			if (type == 0)//elevator is moving--flash lights
			{
				ScriptEngine::Set_Animation ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", true, nullptr, 0.0f, -1.0f, false);
			}

			else if (type == 1)//elevator is at top--goto green
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 0 );//green
			}

			else if (type == 2)//elevator is at bottom--goto red
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
			}
		}

		else if (type == 0)
		{
			if (param == M11_GOTO_IDLE_JDG)
			{
				sydney_in_motion = true;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_START_ACTING_JDG, 8 );
			}

			else if (param == M11_START_ACTING_JDG)
			{
				sydney_in_motion = false;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
			}
		}
	}
};

DECLARE_SCRIPT(M11_FirstFloor_Elevator_Switch_Script_JDG, "")//101721 101722 101723
{
	bool sydney_in_motion;

	REGISTER_VARIABLES()
	{
		sydney_in_motion = false;
		SAVE_VARIABLE(sydney_in_motion, 1);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
		ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
	}

	void Poked( GameObject * obj, GameObject * poker ) override
	{
		if (poker == STAR && sydney_in_motion == false)
		{
			GameObject * elevatorController = ScriptEngine::Find_Object ( M11_MISSILE_LIFT_CONTROLLER_JDG );
			if (elevatorController != nullptr)
			{
				ScriptEngine::Send_Custom_Event ( obj, elevatorController, 0, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
			}
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (param == M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG)
		{
			if (type == 0)//elevator is moving--flash lights
			{
				ScriptEngine::Set_Animation ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", true, nullptr, 0.0f, -1.0f, false);
			}

			else if (type == 1)//elevator is at top--goto green
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 0 );//green
			}

			else if (type == 2)//elevator is at bottom--goto red
			{
				ScriptEngine::Set_Animation_Frame ( obj, "DSP_DOORSWITCH.DSP_DOORSWITCH", 1 );//red
			}
		}

		else if (type == 0)
		{
			if (param == M11_GOTO_IDLE_JDG)
			{
				sydney_in_motion = true;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, false );
				ScriptEngine::Send_Custom_Event( obj, obj, 0, M11_START_ACTING_JDG, 8 );
			}

			else if (param == M11_START_ACTING_JDG)
			{
				sydney_in_motion = false;
				ScriptEngine::Enable_HUD_Pokable_Indicator ( obj, true );
			}
		}
	}
};

DECLARE_SCRIPT(M11_Elevator_Switch_MasterControllerScript_JDG, "")//101673
{
	bool level4_lift_down;
	bool level4_lift_up;
	bool level4_lift_moving;
	bool level3_lift_down;
	bool level3_lift_up;
	bool level3_lift_moving;
	bool level2_lift_down;
	bool level2_lift_up;
	bool level2_lift_moving;
	bool level1_lift_down;
	bool level1_lift_up;
	bool level1_lift_moving;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(level4_lift_down, 1);
		SAVE_VARIABLE(level4_lift_up, 2);
		SAVE_VARIABLE(level4_lift_moving, 3);
		SAVE_VARIABLE(level3_lift_down, 4);
		SAVE_VARIABLE(level3_lift_up, 5);
		SAVE_VARIABLE(level3_lift_moving, 6);
		SAVE_VARIABLE(level2_lift_down, 7);
		SAVE_VARIABLE(level2_lift_up, 8);
		SAVE_VARIABLE(level2_lift_moving, 9);
		SAVE_VARIABLE(level1_lift_down, 10);
		SAVE_VARIABLE(level1_lift_up, 11);
		SAVE_VARIABLE(level1_lift_moving, 12);
	}

	void Created( GameObject * /* obj */ ) override
	{
		level4_lift_down = true;
		level4_lift_up = false;
		level4_lift_moving = false;
		level3_lift_down = true;
		level3_lift_up = false;
		level3_lift_moving = false;
		level2_lift_down = true;
		level2_lift_up = false;
		level2_lift_moving = false;
		level1_lift_down = true;
		level1_lift_up = false;
		level1_lift_moving = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == 1)
		{
			if (param == M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG)//sydney is coming...force elevator down
			{
				level4_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG );
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_04_JDG	, 45, "MISS_ELEV4.MISS_ELEV4" );
				level4_lift_down = true;
			}

			else if (param == M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG)//sydney is coming...force elevator down
			{
				level3_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG );
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_03_JDG	, 45, "MISS_ELEV3.MISS_ELEV3" );
				level3_lift_down = true;
			}

			else if (param == M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG)//sydney is coming...force elevator down
			{
				level2_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG );
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_02_JDG	, 45, "MISS_ELEV2.MISS_ELEV2" );
				level2_lift_down = true;
			}

			else if (param == M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG)//sydney is coming...force elevator down
			{
				level1_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG );
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_01_JDG	, 45, "MISS_ELEV1.MISS_ELEV1" );
				level1_lift_down = true;
			}
		}

		else if (type == 0)
		{
			if (param == M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG && level4_lift_moving == false)
			{
				level4_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG );

				if (level4_lift_down == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_04_JDG	, 0, "MISS_ELEV4.MISS_ELEV4" );
				}

				else if (level4_lift_up == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_04_JDG	, 45, "MISS_ELEV4.MISS_ELEV4" );
				}//101689 101651 101690 101691

				GameObject * lev4switch01 = ScriptEngine::Find_Object ( 101689 );
				GameObject * lev4switch02 = ScriptEngine::Find_Object ( 101651 );
				GameObject * lev4switch03 = ScriptEngine::Find_Object ( 101690 );
				GameObject * lev4switch04 = ScriptEngine::Find_Object ( 101691 );

				if (lev4switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch01, 0, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch02, 0, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch03, 0, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch04, 0, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (param == M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG && level3_lift_moving == false)
			{
				level3_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG );

				if (level3_lift_down == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_03_JDG	, 0, "MISS_ELEV3.MISS_ELEV3" );
				}

				else if (level3_lift_up == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_03_JDG	, 45, "MISS_ELEV3.MISS_ELEV3" );
				}//101692 101693 101694 101695

				GameObject * lev3switch01 = ScriptEngine::Find_Object ( 101692 );
				GameObject * lev3switch02 = ScriptEngine::Find_Object ( 101693 );
				GameObject * lev3switch03 = ScriptEngine::Find_Object ( 101694 );
				GameObject * lev3switch04 = ScriptEngine::Find_Object ( 101695 );

				if (lev3switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch01, 0, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch02, 0, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch03, 0, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch04, 0, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (param == M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG && level2_lift_moving == false)
			{
				level2_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG );

				if (level2_lift_down == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_02_JDG	, 0, "MISS_ELEV2.MISS_ELEV2" );
				}

				else if (level2_lift_up == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_02_JDG	, 45, "MISS_ELEV2.MISS_ELEV2" );
				}//101717 101718 101719 101720

				GameObject * lev2switch01 = ScriptEngine::Find_Object ( 101717 );
				GameObject * lev2switch02 = ScriptEngine::Find_Object ( 101718 );
				GameObject * lev2switch03 = ScriptEngine::Find_Object ( 101719 );
				GameObject * lev2switch04 = ScriptEngine::Find_Object ( 101720 );

				if (lev2switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch01, 0, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch02, 0, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch03, 0, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch04, 0, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (param == M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG && level1_lift_moving == false)
			{
				level1_lift_moving = true;
				ScriptEngine::Start_Timer ( obj, this, 3, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG );

				if (level1_lift_down == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_01_JDG	, 0, "MISS_ELEV1.MISS_ELEV1" );
				}

				else if (level1_lift_up == true)
				{
					ScriptEngine::Static_Anim_Phys_Goto_Frame ( M11_MISSILE_LIFT_01_JDG	, 45, "MISS_ELEV1.MISS_ELEV1" );
				}//101721 101722 101723

				GameObject * lev1switch01 = ScriptEngine::Find_Object ( 101721 );
				GameObject * lev1switch02 = ScriptEngine::Find_Object ( 101722 );
				GameObject * lev1switch03 = ScriptEngine::Find_Object ( 101723 );

				if (lev1switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch01, 0, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev1switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch02, 0, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev1switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch03, 0, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}
			}
		}
	}

	void Timer_Expired( GameObject * obj, int timer_id ) override
	{
		if (timer_id == M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG)
		{
			level4_lift_moving = false;

			if (level4_lift_down == true)
			{
				level4_lift_down = false;
				level4_lift_up = true;

				GameObject * lev4switch01 = ScriptEngine::Find_Object ( 101689 );
				GameObject * lev4switch02 = ScriptEngine::Find_Object ( 101651 );
				GameObject * lev4switch03 = ScriptEngine::Find_Object ( 101690 );
				GameObject * lev4switch04 = ScriptEngine::Find_Object ( 101691 );

				if (lev4switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch01, 1, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch02, 1, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch03, 1, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch04, 1, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (level4_lift_up == true)
			{
				level4_lift_up = false;
				level4_lift_down = true;

				GameObject * lev4switch01 = ScriptEngine::Find_Object ( 101689 );
				GameObject * lev4switch02 = ScriptEngine::Find_Object ( 101651 );
				GameObject * lev4switch03 = ScriptEngine::Find_Object ( 101690 );
				GameObject * lev4switch04 = ScriptEngine::Find_Object ( 101691 );

				if (lev4switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch01, 2, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch02, 2, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch03, 2, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev4switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev4switch04, 2, M11_PLAYER_HAS_POKED_LEVEL4_ELEVATOR_SWITCH_JDG, 0 );
				}
			}
		}

		else if (timer_id == M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG)
		{
			level3_lift_moving = false;

			if (level3_lift_down == true)
			{
				level3_lift_down = false;
				level3_lift_up = true;

				GameObject * lev3switch01 = ScriptEngine::Find_Object ( 101692 );
				GameObject * lev3switch02 = ScriptEngine::Find_Object ( 101693 );
				GameObject * lev3switch03 = ScriptEngine::Find_Object ( 101694 );
				GameObject * lev3switch04 = ScriptEngine::Find_Object ( 101695 );

				if (lev3switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch01, 1, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch02, 1, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch03, 1, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch04, 1, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (level3_lift_up == true)
			{
				level3_lift_up = false;
				level3_lift_down = true;

				GameObject * lev3switch01 = ScriptEngine::Find_Object ( 101692 );
				GameObject * lev3switch02 = ScriptEngine::Find_Object ( 101693 );
				GameObject * lev3switch03 = ScriptEngine::Find_Object ( 101694 );
				GameObject * lev3switch04 = ScriptEngine::Find_Object ( 101695 );

				if (lev3switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch01, 2, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch02, 2, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch03, 2, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev3switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev3switch04, 2, M11_PLAYER_HAS_POKED_LEVEL3_ELEVATOR_SWITCH_JDG, 0 );
				}
			}
		}

		else if (timer_id == M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG)
		{
			level2_lift_moving = false;

			if (level2_lift_down == true)
			{
				level2_lift_down = false;
				level2_lift_up = true;

				GameObject * lev2switch01 = ScriptEngine::Find_Object ( 101717 );
				GameObject * lev2switch02 = ScriptEngine::Find_Object ( 101718 );
				GameObject * lev2switch03 = ScriptEngine::Find_Object ( 101719 );
				GameObject * lev2switch04 = ScriptEngine::Find_Object ( 101720 );

				if (lev2switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch01, 1, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch02, 1, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch03, 1, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch04, 1, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (level2_lift_up == true)
			{
				level2_lift_up = false;
				level2_lift_down = true;

				GameObject * lev2switch01 = ScriptEngine::Find_Object ( 101717 );
				GameObject * lev2switch02 = ScriptEngine::Find_Object ( 101718 );
				GameObject * lev2switch03 = ScriptEngine::Find_Object ( 101719 );
				GameObject * lev2switch04 = ScriptEngine::Find_Object ( 101720 );

				if (lev2switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch01, 2, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch02, 2, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch03, 2, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev2switch04 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev2switch04, 2, M11_PLAYER_HAS_POKED_LEVEL2_ELEVATOR_SWITCH_JDG, 0 );
				}
			}
		}

		else if (timer_id == M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG)
		{
			level1_lift_moving = false;

			if (level1_lift_down == true)
			{
				level1_lift_down = false;
				level1_lift_up = true;

				GameObject * lev1switch01 = ScriptEngine::Find_Object ( 101721 );
				GameObject * lev1switch02 = ScriptEngine::Find_Object ( 101722 );
				GameObject * lev1switch03 = ScriptEngine::Find_Object ( 101723 );

				if (lev1switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch01, 1, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev1switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch02, 1, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev1switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch03, 1, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}
			}

			else if (level1_lift_up == true)
			{
				level1_lift_up = false;
				level1_lift_down = true;

				GameObject * lev1switch01 = ScriptEngine::Find_Object ( 101721 );
				GameObject * lev1switch02 = ScriptEngine::Find_Object ( 101722 );
				GameObject * lev1switch03 = ScriptEngine::Find_Object ( 101723 );

				if (lev1switch01 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch01, 2, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev1switch02 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch02, 2, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}

				if (lev1switch03 != nullptr)
				{
					ScriptEngine::Send_Custom_Event ( obj, lev1switch03, 2, M11_PLAYER_HAS_POKED_LEVEL1_ELEVATOR_SWITCH_JDG, 0 );
				}
			}
		}
	}
};

DECLARE_SCRIPT(M11_CeilingRepellers_Controller2_JDG, "")//106908
{
	int spawned_units;
	int rope_C_id;
	Vector3 ropeC_v3;

	bool rope_C_spawned;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(spawned_units, 1);
		SAVE_VARIABLE(rope_C_id, 2);
		SAVE_VARIABLE(ropeC_v3, 4);
		SAVE_VARIABLE(rope_C_spawned, 6);
	}

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		rope_C_spawned = false;
		spawned_units = 0;
		//ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_JDG, 5 );
		//ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_START_ACTING_JDG, 15 );
		ropeC_v3.Set (4.753f, 38.483f, 8.044f);

	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if (type == 0)
		{
			if (param == M01_START_ACTING_JDG)
			{
				rope_C_spawned = true;
				GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", ropeC_v3);
				//ScriptEngine::Set_Facing ( controller, 75 );
				ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_Repel_Part3.txt");
				spawned_units++;
			}

			else if (param == M01_ROPE_IS_SENDING_ID_JDG)
			{
				rope_C_id = ScriptEngine::Get_ID ( sender );
			}

			else if (param == M01_MODIFY_YOUR_ACTION_02_JDG)
			{
				GameObject * rope_C = ScriptEngine::Find_Object ( rope_C_id );
				if (rope_C != nullptr)
				{
					GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", ropeC_v3);
					//ScriptEngine::Set_Facing ( controller, 75 );
					ScriptEngine::Attach_Script(controller, "Test_Cinematic", "X11D_Repel_Part4.txt");
					ScriptEngine::Set_Animation ( rope_C, "X11D_Rope.X11D_Rope_L", false, nullptr, 0.0f, -1.0f, false);

					spawned_units++;

					ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_04_JDG, (95/30) );
				}
			}


			else if (param == M01_MODIFY_YOUR_ACTION_04_JDG)//set rope "C" to idle anim
			{
				GameObject * rope_C = ScriptEngine::Find_Object ( rope_C_id );
				if (rope_C != nullptr)
				{
					ScriptEngine::Set_Animation ( rope_C, "X11D_Rope.X11D_Rope_Z", true, nullptr, 0.0f, -1.0f, false);
				}
			}

			else if (param == M01_IVE_BEEN_KILLED_JDG )
			{
				if (rope_C_spawned == true)
				{
					float delayTimer = ScriptEngine::Get_Random ( 2, 5 );
					ScriptEngine::Send_Custom_Event ( obj, obj, 0, M01_MODIFY_YOUR_ACTION_02_JDG, delayTimer );
				}


			}
		}
	}
};

DECLARE_SCRIPT(M11_CeilingRepellers_RopeC_JDG, "")
{
	void Created( GameObject * obj ) override //M11_FIRSTFLOOR_REPELER_CONTROLLER_JDG
	{
		ScriptEngine::Enable_Hibernation( obj, false );
		GameObject * myController = ScriptEngine::Find_Object ( 106908 );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, myController, 0, M01_ROPE_IS_SENDING_ID_JDG, 0 );
		}
	}
};



DECLARE_SCRIPT(M11_CeilingRepeller2_JDG, "")
{
	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 100, M01_DOING_ANIMATION_01_JDG );
		params.Set_Attack (STAR, 100, 0, true);
		ScriptEngine::Action_Attack (obj, params);
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "S_a_Human.H_a_X11D_repel") == 0)
		{
			ScriptEngine::Attach_Script(obj, "M01_Hunt_The_Player_JDG","");
		}
	}

	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		GameObject * myController = ScriptEngine::Find_Object ( 106908 );
		if (myController != nullptr)
		{
			ScriptEngine::Send_Custom_Event ( obj, myController, 0, M01_IVE_BEEN_KILLED_JDG, 0 );
		}
	}
};





