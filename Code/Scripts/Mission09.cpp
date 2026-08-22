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
*     Mission09.cpp
*
* DESCRIPTION
*     Mission 9 specific scripts
*
* PROGRAMMER
*     Dan Etter
*
* VERSION INFO
*     $Author: Greg_h $
*     $Revision: 106 $
*     $Modtime: 1/20/02 11:10a $
*     $Archive: /Commando/Code/Scripts/Mission09.cpp $
*
******************************************************************************/


#include "scripts.h"
#include "Toolkit.h"
#include "Mission9.h"
#include <string.h>
#include <stdio.h>
#include "wwlib/wwstring.h"


DECLARE_SCRIPT(M09_Objective_Controller, "") // Object Controller id: 2000071
{

	enum {HAVOCS_SCRIPT};
	bool objective [3];
	int spawn_loc;
	int block1, block2, block3, block4;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( spawn_loc, 1 );
		SAVE_VARIABLE( block1, 2 );
		SAVE_VARIABLE( block2, 3 );
		SAVE_VARIABLE( block3, 4 );
		SAVE_VARIABLE( block4, 5 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Set_Num_Tertiary_Objectives ( 1 );

		ScriptEngine::Start_Timer (obj, this, 1.0f, HAVOCS_SCRIPT);
		ScriptEngine::Enable_Hibernation(obj, false);

		spawn_loc = 0;

		for (int x = 0; x <= 2; x++)
		{
			objective [x] = false;
		}

		block1 = 0;
		block2 = 0;
		block3 = 0;
		block4 = 0;

	}

	void Remove_Pog(int id)
	{
		switch (id)
		{
		// New Primary Mission Objective:  Retrieve the Mobile Power Suit for Dr. Mobius
		case 901:
			{
				ScriptEngine::Set_Objective_HUD_Info(id, -1, "POG_M09_1_01.tga", IDS_POG_RETRIEVE);
			}
			break;
		// New Primary Mission Objective: Escort Dr. Mobius to the Surface
		case 902:
			{
				ScriptEngine::Set_Objective_HUD_Info(id, -1, "POG_M09_1_02.tga", IDS_POG_ESCORT);
			}
			break;
		// New Primary Mission Objective: Escort Dr. Mobius to the Rendezvous Point
		case 903:
			{
				ScriptEngine::Set_Objective_HUD_Info(id, -1, "POG_M09_1_03.tga", IDS_POG_ESCORT);
			}
			break;
		}
	}

	void Add_An_Objective(int id)
	{
		GameObject *object;

		switch (id)
		{
		// New Primary Mission Objective:  Protect Mobius

		case 900:
			{
				ScriptEngine::Add_Objective(900, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M09_04, nullptr, IDS_Enc_Obj_Primary_M09_04);

				object = ScriptEngine::Find_Object(2000010);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(900, object);
					ScriptEngine::Set_Objective_HUD_Info_Position( 900, 90, "POG_M09_1_02.tga", IDS_POG_PROTECT, ScriptEngine::Get_Position (object));
				}
			}
			break;

		// New Primary Mission Objective:  Retrieve the Mobile Power Suit for Dr. Mobius

		case 901:
			{
				ScriptEngine::Add_Objective(901, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M09_01, nullptr, IDS_Enc_Obj_Primary_M09_01);

				object = ScriptEngine::Find_Object(2002239);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(901, object);
					ScriptEngine::Set_Objective_HUD_Info_Position( 901, 99, "POG_M09_1_01.tga", IDS_POG_RETRIEVE, ScriptEngine::Get_Position (object));
				}
			}
			break;
		// New Primary Mission Objective: Escort Dr. Mobius to the Surface
		case 902:
			{
				ScriptEngine::Add_Objective(902, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M09_02, nullptr, IDS_Enc_Obj_Primary_M09_02);

				object = ScriptEngine::Find_Object(2000955);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(902, object);
					ScriptEngine::Set_Objective_HUD_Info_Position( 902, 98, "POG_M08_1_03.tga", IDS_POG_ESCORT, ScriptEngine::Get_Position (object));
				}
			}
			break;
		// New Primary Mission Objective: Escort Dr. Mobius to the Rendezvous Point
		case 903:
			{
				ScriptEngine::Add_Objective(903, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M09_03, nullptr, IDS_Enc_Obj_Primary_M09_03);

				object = ScriptEngine::Find_Object(2000969);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(903, object);
					ScriptEngine::Set_Objective_HUD_Info_Position( 903, 97, "POG_M09_1_03.tga", IDS_POG_ESCORT, ScriptEngine::Get_Position (object));
				}
			}
			break;
		// New Hidden Mission Objective: Eliminate the NOD Resistance at Checkpoint A

		case 904:
			{
				ScriptEngine::Add_Objective(904, OBJECTIVE_TYPE_TERTIARY, OBJECTIVE_STATUS_HIDDEN, IDS_Enc_ObjTitle_Hidden_M09_01, nullptr, IDS_Enc_Obj_Hidden_M09_04);			}
			break;/*
		// New Hidden Mission Objective: Comandeer the NOD Stealth Tank.
		case 905:
			{
				ScriptEngine::Add_Objective(905, OBJECTIVE_TYPE_TERTIARY, OBJECTIVE_STATUS_HIDDEN, IDS_M09_T02, nullptr);
			}
			break;
		// New Hidden Mission Objective: Comandeer the NOD Stealth Tank.
		case 906:
			{
				ScriptEngine::Add_Objective(906, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_M09_P05, nullptr);
			}
			break;*/
		case 1000:
			{
				break;
			}
		}
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == BLOCK_ON)
		{
			if ((block1 != 0) && (block2 != 0) && (block3 != 0) && (block4 != 0)) {
				ScriptEngine::Start_Timer (obj, this, 3.0f, 333);

				GameObject *rub1 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (-108.066f, 483.917f, -191.022f));
				block1 = ScriptEngine::Get_ID (rub1);
				ScriptEngine::Set_Is_Rendered( rub1, false );
				GameObject *rub2 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (-25.433f, 513.013f, -150.613f));
				block2 = ScriptEngine::Get_ID (rub2);
				ScriptEngine::Set_Is_Rendered( rub2, false );
				GameObject *rub3 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (-47.948f, 535.216f, -103.284f));
				block3 = ScriptEngine::Get_ID (rub3);
				ScriptEngine::Set_Is_Rendered( rub3, false );
				ScriptEngine::Set_Facing (rub3, -90.0);
				GameObject *rub4 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (39.231f, 515.105f, -78.137f));
				block4 = ScriptEngine::Get_ID (rub4);
				ScriptEngine::Set_Is_Rendered( rub4, false );
			}
		}

		if (type == BLOCK_OFF)
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block1));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block2));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block3));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block4));
			block1 = block2 = block3 = block4 = 0;
		}


		if (type == MOBIUS_KILLED)
		{
			ScriptEngine::Set_HUD_Help_Text ( IDS_M09DSGN_DSGN0073I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );

			ScriptEngine::Start_Timer (obj, this, 4.0f, 40);
		}

        if (type == MISSION09_CHECK && param == MISSION09_CHECK)
		{
			ScriptEngine::Send_Custom_Event (obj, sender, 901, objective [0], 0.0f);
			ScriptEngine::Send_Custom_Event (obj, sender, 902, objective [1], 0.0f);
			ScriptEngine::Send_Custom_Event (obj, sender, 903, objective [2], 0.0f);
		}

		if (type == 901 && param == 1)
		{
			objective [0] = true;
			Remove_Pog(901);
		}

		if (type == 902 && param == 1)
		{
			objective [1] = true;
			Remove_Pog(902);
		}

		if (type == 903 && param == 1)
		{
			objective [2] = true;
			Remove_Pog(903);
		}

		switch (param)
		{
			case 1:
				{
					//ScriptEngine::Clear_Radar_Marker (type);
					ScriptEngine::Set_Objective_Status(type, OBJECTIVE_STATUS_ACCOMPLISHED);
				}
				break;
			case 2:
				{
					//ScriptEngine::Clear_Radar_Marker (type);
					ScriptEngine::Set_Objective_Status(type, OBJECTIVE_STATUS_FAILED);
				}
				break;
			case 3:
					Add_An_Objective(type);
				break;
			case 4:
					ScriptEngine::Set_Objective_Status(type, OBJECTIVE_STATUS_PENDING);
				break;
			case 5:
					ScriptEngine::Set_Objective_Status(type, OBJECTIVE_STATUS_HIDDEN);
				break;
		}

		if(type == 666 && param == 666)
		{

			ScriptEngine::Start_Timer (obj, this, 0.10f, M09_DEAD_HAVOC);
		}

		if (type == SPAWN_LOC && param == INCREMENT)
		{
			spawn_loc++;
		}
	}


	void Timer_Expired(GameObject * /* obj */, int timer_id ) override
	{
		if (timer_id == 333)
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block1));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block2));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block3));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block4));
			block1 = block2 = block3 = block4 = 0;
		}

		if (timer_id == 40)
		{
			ScriptEngine::Mission_Complete(false);
		}
	}
};

DECLARE_SCRIPT (M09_Havoc_Script, "")
{
	void Created (GameObject *obj) override
	{
		/*ScriptEngine::Give_PowerUp(obj, "Shotgun Weapon 3 Clips PU", false);
		ScriptEngine::Give_PowerUp(obj, "FlameThrower_Weapon_PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "Flame_Weapon_1_Clip_PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "Flame_Weapon_1_Clip_PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "ChemSprayer_Weapon_PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "Chem_Weapon_1_Clip_PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "Grenade Launcher Weapon PowerUps", false);
		ScriptEngine::Give_PowerUp(obj, "GL_Weapon_1_Clip_PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "Mine Weapons PowerUps", false);
		ScriptEngine::Give_PowerUp(obj, "Remote Mine Weapon 1 Clip PU", false);
		ScriptEngine::Give_PowerUp(obj, "Sniper Weapon 1 Clip PU", false);
		ScriptEngine::Give_PowerUp(obj, "Sniper Weapon 1 Clip PU", false);
		ScriptEngine::Give_PowerUp(obj, "MG Weapon 1 Clip PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "MiniGun 2 Clips PU", false);
		ScriptEngine::Give_PowerUp(obj, "Armor 100 PowerUp", false);
		ScriptEngine::Give_PowerUp(obj, "RL Weapon 1 Clip PowerUp", false);*/

		ScriptEngine::Start_Timer(obj, this, 0.5f, ACTIVATE);
		//ScriptEngine::Grant_Key ( STAR, 10, true);
	}

	void Timer_Expired (GameObject * /* obj */, int timer_id) override
	{
		if (timer_id == ACTIVATE)
		{
			ScriptEngine::Grant_Key ( STAR, 10, true);
		}
	}

	void Destroyed(GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 666, 666, 0.0f);
	}
};

DECLARE_SCRIPT (M09_Mobius_Suit_Objective, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
	}

	void Custom (GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == 8888)
		{
			GameObject *mobius = ScriptEngine::Find_Object (2000010);

			Vector3 havoc_loc = ScriptEngine::Get_Position (ScriptEngine::Find_Object (1100497));
			Vector3 mobius_loc = ScriptEngine::Get_Position (ScriptEngine::Find_Object (2002239));

			ScriptEngine::Set_Position (STAR, havoc_loc );
			ScriptEngine::Set_Position (mobius, mobius_loc );
		}
	}


	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			GameObject *mobius = ScriptEngine::Find_Object (2000010);
			already_entered = true;

			//ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (1202323));

			if (mobius)
			{
				Vector3 havoc_loc = ScriptEngine::Get_Position (STAR);
				Vector3 mobius_loc = ScriptEngine::Get_Position (mobius);

				havoc_loc.Z += 7.0f;
				mobius_loc.Z += 7.0f;


				ScriptEngine::Action_Reset(mobius, 100);
				ScriptEngine::Set_Position (STAR, havoc_loc );
				ScriptEngine::Set_Position (mobius, mobius_loc );

				//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), SPAWN_LOC, INCREMENT, 0.0f);
				//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 901, 1, 0.0f);

				GameObject * powermob = ScriptEngine::Create_Object ( "Invisible_Object", Vector3(0.0f, 0.0f, 0.0f));
				ScriptEngine::Set_Facing(powermob, 0.0f);
				ScriptEngine::Attach_Script(powermob, "Test_Cinematic", "X9C_MIDTRO.txt");

				ScriptEngine::Set_Model ( mobius, "c_ag_gdi_pmob" );
				ScriptEngine::Give_PowerUp(mobius, "POW_LaserChaingun_AI", false);
				ScriptEngine::Select_Weapon (mobius, "Weapon_LaserChaingun_Ai" );

				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 901, 1, 0.0f);
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 902, 3, 0.0f);

				ScriptEngine::Set_Health (mobius, 300.0f);
				ScriptEngine::Attach_Script(mobius, "M09_Damage_Modifier", ".05");
			}
		}
	}

	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "h_a_a0a0_l26da") == 0)
		{
			GameObject *mobius = ScriptEngine::Find_Object (2000010);

			ScriptEngine::Set_Animation ( mobius, "h_a_a0a0_l26db", true, nullptr, 0.0f, -1.0f, false);

			const char *conv_name = ("IDS_M09_D11");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 901);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if(action_id == 900 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			/*GameObject *mobius = ScriptEngine::Find_Object (2000010);
			ScriptEngine::Set_Model ( mobius, "c_ag_gdi_pmob" );
			ScriptEngine::Give_PowerUp(mobius, "POW_LaserChaingun_AI", false);
			ScriptEngine::Select_Weapon (mobius, "Weapon_LaserChaingun_Ai" );*/

			//ScriptEngine::Set_Animation ( mobius, "h_a_a0a0_l26da", false, nullptr, 0.0f, -1.0f, false);
		}

		if(action_id == 901 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			GameObject *mobius = ScriptEngine::Find_Object (2000010);

			ScriptEngine::Set_Animation ( mobius, "h_a_a0a0_l26dc", false, nullptr, 0.0f, -1.0f, false);

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 902, 3, 0.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Surface_Objective, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), SPAWN_LOC, INCREMENT, 0.0f);

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 902, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 903, 3, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000955), FLYOVER, 0, 0.0f);
			//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 906, 3, 0.0f);

			ScriptEngine::Destroy_Object ( ScriptEngine::Find_Object (2000614));
			ScriptEngine::Destroy_Object ( ScriptEngine::Find_Object (2000954));
		}
	}

	void Custom (GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == ENTERED && param ==	ENTERED)
		{
			already_entered = true;
		}
	}
};

DECLARE_SCRIPT (M09_Evac_Point_Objective, "")
{
	bool star_in_zone;
	bool mobius_in_zone;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( star_in_zone, 1 );
		SAVE_VARIABLE( mobius_in_zone, 2 );
	}

	void Created (GameObject * /* obj */) override
	{
		star_in_zone = false;
		mobius_in_zone = false;
	}

	void Exited( GameObject * /* obj */, GameObject * exiter ) override
	{
		GameObject * mobius = ScriptEngine::Find_Object (2000010);

		if (exiter == STAR)
		{
			star_in_zone = false;
		}

		if (exiter == mobius)
		{
			mobius_in_zone = false;
		}
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		GameObject * mobius = ScriptEngine::Find_Object (2000010);

		if (enterer == STAR)
		{
			star_in_zone = true;

			if (mobius_in_zone)
			{
				ScriptEngine::Attach_Script(mobius, "M00_Damage_Modifier_DME", "0.05f, 1, 1, 0, 0");
				ScriptEngine::Start_Timer (obj, this, 1.0f, M09_MISSION_COMPLETE);
			}

			if (!mobius_in_zone)
			{
				ScriptEngine::Send_Custom_Event( obj, mobius, FOLLOW, 2000969, 0.0f );
			}
		}

		if (enterer == mobius)
		{
			mobius_in_zone = true;

			if (star_in_zone)
			{
				ScriptEngine::Attach_Script(mobius, "M00_Damage_Modifier_DME", "0.05f, 1, 1, 0, 0");
				ScriptEngine::Start_Timer (obj, this, 1.0f, M09_MISSION_COMPLETE);
			}
		}

		/*GameObject *mobius = ScriptEngine::Find_Object (2000010);
		float mobius_distance = ScriptEngine::Get_Distance(ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000969)), ScriptEngine::Get_Position (mobius));

		if (mobius_distance < 15.0f)
		{
			//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 903, 1, 0.0f);

			ScriptEngine::Start_Timer (obj, this, 5.0f, M09_MISSION_COMPLETE);

			GameObject * chin_loc = ScriptEngine::Find_Object (2000259);
			float facing = ScriptEngine::Get_Facing (chin_loc);

			GameObject * chinook_obj = ScriptEngine::Create_Object ( "Invisible_Object", ScriptEngine::Get_Position (chin_loc));
			ScriptEngine::Set_Facing(chinook_obj, facing);
			ScriptEngine::Attach_Script(chinook_obj, "Test_Cinematic", "M09_XG_EV4.txt");
		}

		else
		{
			ActionParamsStruct params;

			params.Set_Basic( this, 99, MOBIUS_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000969)), 1.2f, 0.8f );

			ScriptEngine::Action_Goto (mobius, params);

			ScriptEngine::Start_Timer (obj, this, 3.0f, TIMER_RESET);
		}*/
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		if (timer_id == M09_MISSION_COMPLETE)
		{
			ScriptEngine::Mission_Complete ( true );
		}

		if (timer_id == TIMER_RESET)
		{
			GameObject *mobius = ScriptEngine::Find_Object (2000010);
			float mobius_distance = ScriptEngine::Get_Distance(ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000969)), ScriptEngine::Get_Position (mobius));

			if (mobius_distance < 15.0f)
			{
				//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 903, 1, 0.0f);

				ScriptEngine::Start_Timer (obj, this, 1.0f, M09_MISSION_COMPLETE);

				GameObject * chin_loc = ScriptEngine::Find_Object (2000259);
				float facing = ScriptEngine::Get_Facing (chin_loc);

				GameObject * chinook_obj = ScriptEngine::Create_Object ( "Invisible_Object", ScriptEngine::Get_Position (chin_loc));
				ScriptEngine::Set_Facing(chinook_obj, facing);
				ScriptEngine::Attach_Script(chinook_obj, "Test_Cinematic", "M09_XG_EV4.txt");
			}

			else
			{
				ActionParamsStruct params;

				params.Set_Basic( this, 99, MOBIUS_GOTO );
				params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000969)), 1.2f, 0.8f );

				ScriptEngine::Action_Goto (mobius, params);

				ScriptEngine::Start_Timer (obj, this, 3.0f, TIMER_RESET);
			}
		}
	}
};

DECLARE_SCRIPT (M09_LabRoom_Zones, "Mutant_Num:int, Mutant_Goto:int")
{
	int mutant;
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
		SAVE_VARIABLE( mutant, 2 );
	}

	void Created (GameObject * obj) override
	{
		mutant = Get_Int_Parameter("Mutant_Num");
		int anim_loc = Get_Int_Parameter("Mutant_Goto");

		//already_entered = false;

		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(mutant), GOTO, anim_loc, 4.0f);

	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000279), CONVERSATION, TRIGGERED, 0.0f);

		ScriptEngine::Action_Reset ( ScriptEngine::Find_Object (mutant), 100.0f );
	}

	void Exited( GameObject * /* obj */, GameObject * /* exiter */ ) override
	{
		//ScriptEngine::Grant_Key ( STAR, 10, false);
	}

	void Custom(GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == ALREADY_ENTERED)
		{
			already_entered = true;
		}
	}
};

DECLARE_SCRIPT (M09_Animating_Mutant, "Animation=0:int")
{
	float face;

	void Custom(GameObject *obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == GOTO)
		{
			//face = ScriptEngine::Get_Facing (ScriptEngine::Find_Object (param));
			//Vector3 loc = ScriptEngine::Get_Position (ScriptEngine::Find_Object (param));

			ActionParamsStruct params;

			params.Set_Basic( this, 99, MUTANT_GOTO );
			//params.Set_Movement( loc, RUN, 0.25f );
			params.Set_Movement( Vector3(0,0,0), RUN, 0.5f);
			params.WaypathID = param;

			ScriptEngine::Action_Goto (obj, params);
		}

		if (type == ANIMATION && param == RESET)
		{
			ScriptEngine::Action_Reset ( obj, 100 );
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		int animation = Get_Int_Parameter ("Animation");

		if (action_id == MUTANT_GOTO && reason == ACTION_COMPLETE_NORMAL)
		{
			if (animation == 0)
			{
				//ScriptEngine::Set_Facing (obj, face);

				ActionParamsStruct params;
				params.Set_Basic( this, 99, DOING_ANIMATION );
				params.Set_Animation ("S_C_HUMAN.H_C_7001", true);
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			if (animation == 1)
			{
				ActionParamsStruct params;
				params.Set_Basic( this, 99, DOING_ANIMATION );
				params.Set_Animation ("S_C_HUMAN.H_C_7002", true);
				ScriptEngine::Action_Play_Animation (obj, params);
			}
		}
	}
};






DECLARE_SCRIPT (M09_LabRoom_Controller, "")
{
	bool triggered;
	bool mobius_ready;


	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( triggered, 1 );
		SAVE_VARIABLE( mobius_ready, 2 );
	}

	void Created (GameObject * /* obj */) override
	{
		triggered = false;
		mobius_ready = false;
	}

	void Custom(GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (!triggered && mobius_ready)
		{
			if (type ==	CONVERSATION && param == TRIGGERED)
			{
				triggered = true;

				GameObject *mobius = ScriptEngine::Find_Object (2000010);

				const char *conv_name = ("M09CON014");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
				ScriptEngine::Join_Conversation(mobius, conv_id, true, true, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, true, true, true);
				ScriptEngine::Start_Conversation (conv_id, 904);
				ScriptEngine::Monitor_Conversation (mobius, conv_id);
			}
		}

		if (type == CONVERSATION && param == ENDED)
		{
			mobius_ready = true;
		}
	}
};

DECLARE_SCRIPT (M09_Mobius_Initial_Conversation, "")
{
	float max_health;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( max_health, 1 );
	}

	void Created (GameObject *obj) override
	{
		ScriptEngine::Set_HUD_Help_Text ( IDS_M09DSGN_DSGN0072I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );

		ScriptEngine::Start_Timer (obj, this, 5.0f, 20);

		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 900, 3, 0.0f);

		ScriptEngine::Innate_Disable ( obj );
		max_health = ScriptEngine::Get_Health (obj);
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		if (timer_id == 20)
		{
			const char *conv_name = ("IDS_M09_D07");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(obj, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 900);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}

	/*void Damaged (GameObject *obj, GameObject *damager, float amount)
	{
		ScriptEngine::Set_Health (obj, max_health);
	}*/

	void Killed(GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), MOBIUS_KILLED, 0, 0.0f);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if(action_id == 900 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			const char *conv_name = ("IDS_M09_P01");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(obj, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 903);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}

		if(action_id == 901 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, 99, 101 );
			params.Set_Movement( Vector3(0,0,0), WALK, 0.5f);
			params.WaypathID = 2000275;

			ScriptEngine::Action_Goto (obj, params);
		}

		if (action_id == 101)
		{

			/*ActionParamsStruct params;
			params.Set_Basic( this, 99, 150 );
			params.Set_Face_Location( ScriptEngine::Get_Position (STAR), 10.0f);
			ScriptEngine::Action_Face_Location ( obj, params );
			*/
			const char *conv_name = ("IDS_M09_D07");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(obj, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 902);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}

		if(action_id == 902 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			const char *conv_name = ("IDS_M09_P01");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(obj, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 903);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}

		if(action_id == 903 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 901, 3, 0.0f);
			ScriptEngine::Innate_Enable(obj);
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000010), "M09_Mobius_Follow", "");
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000279), CONVERSATION, ENDED, 0.0f);
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), FOLLOW, 2001012, 1.0f );

			ScriptEngine::Give_PowerUp(obj, "POW_Pistol_AI", false);
			ScriptEngine::Select_Weapon (obj, "Weapon_Pistol_Ai" );
		}
	}
};






DECLARE_SCRIPT (M09_Mobius_Follow, "")  //Mobius (Pre-Suit): 2000010
{
	int objective_901, objective_902, objective_903;
	int cover_id [3];
	int closest;
	int attack_path;
	int elev_num;
	float cover_distance [3];
	float current_health;
	float last_health, damage_tally;
	bool stationary;
	bool nofollow;



	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( objective_901, 1 );
		SAVE_VARIABLE( objective_902, 2 );
		SAVE_VARIABLE( objective_903, 3 );
		SAVE_VARIABLE( closest, 4 );
		SAVE_VARIABLE( current_health, 5 );
		SAVE_VARIABLE( last_health, 6 );
		SAVE_VARIABLE( damage_tally, 7 );
		SAVE_VARIABLE( stationary, 8 );
		SAVE_VARIABLE( attack_path, 9 );
		SAVE_VARIABLE( nofollow, 10 );
		SAVE_VARIABLE( elev_num, 11 );
	}

	void Follow_Function (GameObject * obj, int waypath_id)
	{
		attack_path = waypath_id;
		//Vector3 loc = ScriptEngine::Get_Position (ScriptEngine::Find_Object (waypath_id));

		ActionParamsStruct params;

		params.Set_Basic( this, 96, MOBIUS_GOTO );
		params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (waypath_id)), 1.2f, 0.8f );

		ScriptEngine::Action_Goto (obj, params);

		/*ActionParamsStruct params;

		params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), 1);
		params.Set_Movement (STAR, RUN, 0.5f);
		params.MoveFollow = true;
		ScriptEngine::Action_Goto (obj, params);*/
	}

	/*void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN +5, 10 );
		params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (attack_path)), 1.2f, 1.0f );
		//params.WaypathID = attack_path;
		params.Set_Attack(enemy, 100.0f, 1.0f, true);
		params.AttackCheckBlocked = true;
		params.AttackActive = true;

		ScriptEngine::Action_Attack (obj, params);
	}*/

	void Killed(GameObject * obj, GameObject * /* killer */ ) override
	{
        ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), MISSION09_CHECK, MISSION09_CHECK, 0.0f);

		if (objective_901 == false)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 901, 2, 0.0f);
		}

		else if (objective_902 == false)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 902, 2, 0.0f);
		}

		else if (objective_903 == false)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 903, 2, 0.0f);
		}

		ScriptEngine::Mission_Complete(false);
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == NO_FOLLOW)
		{
			if (param == ON)
			{
				nofollow = true;
			}

			if (param == OFF)
			{
				nofollow = false;
			}
		}

		if (sender == (ScriptEngine::Find_Object (2000071)))
		{
			switch (type)
			{
			case 901:
				{
					objective_901 = param;
				}
				break;

			case 902:
				{
					objective_902 = param;
				}
				break;

			case 903:
				{
					objective_903 = param;
				}
				break;
			}
		}

		if (type == WAYPATH)
		{

			ActionParamsStruct params;

			params.Set_Basic( this, 99, 951 );
			params.Set_Movement( ScriptEngine::Find_Object (param), 1.2f, 0.5f);
			ScriptEngine::Action_Goto( obj, params );
		}

		if (type == FOLLOW)
		{
			Follow_Function (obj, param);
		}

		if (type == ELEVATOR_EXIT)
		{
			attack_path = param;

			ActionParamsStruct params;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, MOBIUS_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (param)), 1.2f, 0.8f );
			params.MovePathfind = false;

			ScriptEngine::Action_Goto (obj, params);
		}

		if (type == ELEVATOR)
		{
			elev_num = ScriptEngine::Get_ID (sender);
			ScriptEngine::Action_Reset ( obj, 100 );

			ActionParamsStruct params;

			params.Set_Basic( this, 99, ELEV_WAYPOINT );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (param)), 1.2f, 1.0f);

			ScriptEngine::Action_Goto (obj, params);

		}

		if (type == ELEVATOR_DOWN)
		{
			elev_num = ScriptEngine::Get_ID (sender);
			ScriptEngine::Action_Reset ( obj, 100 );

			ActionParamsStruct params;

			params.Set_Basic( this, 99, ELEV_WAYPOINT2 );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (param)), 1.2f, 1.0f);

			ScriptEngine::Action_Goto (obj, params);

		}
	}


	void Created (GameObject *obj) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000279), CONVERSATION, ENDED, 0.0f);

		for (int x = 0; x <= 2; x++)
		{
			cover_distance [x] = 900;
		}

		cover_id [0] = 2000006;
		cover_id [1] = 2000007;
		cover_id [2] = 2000008;
		objective_901 = false;
		objective_902 = false;
		objective_903 = false;

		stationary = false;

		nofollow = false;

		closest = 0;

		last_health = ScriptEngine::Get_Max_Health(obj);
		damage_tally = 0;

		ScriptEngine::Start_Timer (obj, this, 5.0f, TOO_FAR);
	}

	/*void Damaged( GameObject * obj, GameObject * damager, float amount ) override
	{
		current_health = ScriptEngine::Get_Health (obj);
		float damage = ((last_health - current_health) + damage_tally);
		float mod_damage = (damage * .15);
		damage_tally += mod_damage;

		ScriptEngine::Set_Health (obj, (last_health - mod_damage));
		last_health = ScriptEngine::Get_Health (obj);
		current_health = ScriptEngine::Get_Health (obj);

		if (current_health <= 75)
		{
			ScriptEngine::Set_Innate_Aggressiveness ( obj, 0.0 );
			ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 100.0 );
		}
	}*/


	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{

		if((action_id == MOBIUS_GOTO) && (action_id != ScriptEngine::Get_Action_ID(obj)) && (!ScriptEngine::Is_Performing_Pathfind_Action(obj)) && (reason == ACTION_COMPLETE_LOW_PRIORITY))
		{
			ScriptEngine::Start_Timer (obj, this, 5.0f, RESEND_GOTO);
		}

		if (action_id == MOBIUS_GOTO && reason == ACTION_COMPLETE_PATH_BAD_DEST)
		{
			ScriptEngine::Start_Timer (obj, this, 0.5f, BAD_PATH);
		}

		/*if(action_id == 1 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			Follow_Function (obj);
		}

		if (action_id == 20)
		{
			ScriptEngine::Start_Timer (obj, this, 10.0f, 20);
		}

		if (action_id == 10000)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 99, 10001);
			params.Set_Animation( "H_A_J20b", true );
			ScriptEngine::Action_Play_Animation (obj, params);
		}*/

		/*if (action_id == 951)
		{
			ScriptEngine::Set_Facing (obj, ScriptEngine::Get_Facing (ScriptEngine::Find_Object (2000462)));

			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000453), VERIFY, GO, 4.0f );

			const char *conv_name = ("IDS_M09_D17");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(obj, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 952);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}*/

		if(action_id == 952)
		{
			ScriptEngine::Start_Timer (obj, this, 0.0f, 953);

		}

		if (action_id == ELEV_WAYPOINT && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (elev_num), SET_MOBIUS, 0, 0.0f);
		}

		if (action_id == ELEV_WAYPOINT2 && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (elev_num), SET_MOBIUS, 0, 0.0f);
		}
	}

	/*void Poked(GameObject * obj, GameObject * poker)
	{
		if (poker == STAR)
		{
			if (stationary == false)
			{
				stationary = true;

				ScriptEngine::Set_Innate_Is_Stationary ( obj, true );

				const char *conv_name = ("IDS_M09_D03");
				int conv_id = ScriptEngine::Create_Conversation (conv_name);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

				ActionParamsStruct params;
				params.Set_Basic( this, 99, 10000);
				params.Set_Animation( "H_A_J20a", false );
				ScriptEngine::Action_Play_Animation (obj, params);
			}

			else if (stationary == true)
			{
				stationary = false;

				ScriptEngine::Action_Reset ( obj, 100 );

				ScriptEngine::Set_Innate_Is_Stationary ( obj, false );

				const char *conv_name = ("IDS_M09_D04");
				int conv_id = ScriptEngine::Create_Conversation (conv_name);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

				ActionParamsStruct params;
				params.Set_Basic( this, 99, 10002);
				params.Set_Animation( "H_A_J20c", false );
				ScriptEngine::Action_Play_Animation (obj, params);

				//Follow_Function (GameObject * obj, int waypath_id);
			}
		}
	}*/

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		if (timer_id == RESEND_GOTO)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, MOBIUS_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (attack_path)), 1.2f, 1.0f );

			ScriptEngine::Action_Goto (obj, params);
		}

		if (timer_id == BAD_PATH)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, MOBIUS_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (attack_path)), 1.2f, 1.0f );
			params.MovePathfind = false;

			ScriptEngine::Action_Goto (obj, params);
		}

		if (timer_id == TOO_FAR)
		{
			if (!nofollow)
			{
				float distance = (ScriptEngine::Get_Distance ( ScriptEngine::Get_Position (STAR), ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000010))) );

				if (distance >= 25.0f)
				{
					ActionParamsStruct params;

					params.Set_Basic( this, 96, MOBIUS_GOTO );
					params.Set_Movement( ScriptEngine::Get_Position (STAR), 1.2f, 5.0f );
					//params.MovePathfind = false;

					ScriptEngine::Action_Goto (obj, params);
				}

				ScriptEngine::Start_Timer (obj, this, 5.0f, TOO_FAR);
			}
		}


		/*if (stationary == false)
		{
			int poke_id = ScriptEngine::Get_Random_Int(1, 3);

			ScriptEngine::Start_Timer (obj, this, 5.0f, CHECK_DISTANCE);


			// Don't leave me here
			if(timer_id == CHECK_DISTANCE && poke_id == 2)
			{
				Vector3 star_pos = ScriptEngine::Get_Position(STAR);
				Vector3 mobius_pos = ScriptEngine::Get_Position(obj);
				float distance_to_star = ScriptEngine::Get_Distance(star_pos, mobius_pos);
				if(distance_to_star > 15.0f)
				{
					const char *conv_name = ("IDS_M09_D01");
					int conv_id = ScriptEngine::Create_Conversation (conv_name);
					ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
					ScriptEngine::Start_Conversation (conv_id, 1);
					ScriptEngine::Monitor_Conversation (obj, conv_id);

					Follow_Function (obj);
				}

			}
			// Hey!  Wait for me!
			else if(timer_id == CHECK_DISTANCE && poke_id == 1)
			{
				Vector3 star_pos = ScriptEngine::Get_Position(STAR);
				Vector3 mobius_pos = ScriptEngine::Get_Position(obj);
				float distance_to_star = ScriptEngine::Get_Distance(star_pos, mobius_pos);
				if(distance_to_star > 15.0f)
				{
					const char *conv_name = ("IDS_M09_D02");
					int conv_id = ScriptEngine::Create_Conversation (conv_name);
					ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
					ScriptEngine::Start_Conversation (conv_id, 1);
					ScriptEngine::Monitor_Conversation (obj, conv_id);

					Follow_Function (obj);
				}

			}

			else if(timer_id == 20)
			{
				Follow_Function (obj);
			}
		}*/

		if(timer_id == 953)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000453), VERIFY, STOP, 3.0f );

		}

	}
};

DECLARE_SCRIPT (M09_Stationary, "Reward_override=0:int")
{
	int reward;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( reward, 1 );
	}

	void Created(GameObject *obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );

		reward = ScriptEngine::Get_Random_Int ( 1, 101 );

		if (Get_Int_Parameter("Reward_override") == 1)
		{
			ScriptEngine::Set_Health ( obj, 5 );
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject * /* enemy */ ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
	}

	/*void Killed( GameObject * obj, GameObject * killer ) override
	{
		if (Get_Int_Parameter("Reward_override") == 0)
		{
			if (reward > 25)
			{
				Vector3 powerup_loc = ScriptEngine::Get_Position (obj);
				powerup_loc.Z += 1.0f;
				ScriptEngine::Create_Object("Health 025 PowerUp", powerup_loc);
			}

			if (reward > 80)
			{
				Vector3 powerup_loc = ScriptEngine::Get_Position (obj);
				powerup_loc.Z += 1.0f;
				ScriptEngine::Create_Object("Armor 025 PowerUp", powerup_loc);
			}
		}
	}*/

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == CHANGE_PLAYERTYPE)
		{
			if (param == MUTANT)
			{
				ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_MUTANT );
			}
		}
	}

};

DECLARE_SCRIPT (M09_Stationary_Nod, "")
{
	int reward;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( reward, 1 );
	}

	void Created(GameObject *obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, true );

		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );

		reward = ScriptEngine::Get_Random_Int ( 1, 101 );
	}

	void Enemy_Seen(GameObject * obj, GameObject * /* enemy */ ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary ( obj, false );
	}

	/*void Killed( GameObject * obj, GameObject * killer ) override
	{
		if (reward > 25)
		{
			Vector3 powerup_loc = ScriptEngine::Get_Position (obj);
			powerup_loc.Z += 1.0f;
			ScriptEngine::Create_Object("Health 025 PowerUp", powerup_loc);
		}

		if (reward > 80)
		{
			Vector3 powerup_loc = ScriptEngine::Get_Position (obj);
			powerup_loc.Z += 1.0f;
			ScriptEngine::Create_Object("Armor 025 PowerUp", powerup_loc);
		}
	}*/

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == CHANGE_PLAYERTYPE)
		{
			if (param == MUTANT)
			{
				ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_MUTANT );
			}
		}
	}

};

DECLARE_SCRIPT (M09_Home_Location_10, "")
{
	void Created (GameObject *obj) override
	{
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 10.0f);
	}
};

DECLARE_SCRIPT (M09_Mutant_Ambush_Zone_01, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), SPAWN_LOC, INCREMENT, 0.0f);

			//Nod soldiers
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000126), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000126), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000127), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000127), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000128), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000128), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000129), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000129), false );

			//Mutant soldiers

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000122), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000122), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000122), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000123), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000123), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000123), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000124), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000124), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000124), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000125), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000125), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000125), "M09_Mutant_Damage_Mod_10", "");
		}
	}

};

DECLARE_SCRIPT (M09_Mutant_Damage_Mod_10, "")
{
	float last_health, current_health, damage_tally;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last_health, 1 );
		SAVE_VARIABLE( current_health, 2 );
		SAVE_VARIABLE( damage_tally, 3 );
	}

	void Created (GameObject * obj) override
	{
		last_health = ScriptEngine::Get_Max_Health(obj);
		damage_tally = 0;

	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager != STAR)
		{
			current_health = ScriptEngine::Get_Health (obj);
			float damage = ((last_health - current_health) + damage_tally);
			float mod_damage = (damage * .10f);
			damage_tally += mod_damage;

			ScriptEngine::Set_Health (obj, (last_health - mod_damage));
			last_health = ScriptEngine::Get_Health (obj);
			current_health = ScriptEngine::Get_Health (obj);
		}
	}
};

DECLARE_SCRIPT (M09_Mutant_Damage_Mod_50, "")
{
	float last_health, current_health, damage_tally;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last_health, 1 );
		SAVE_VARIABLE( current_health, 2 );
		SAVE_VARIABLE( damage_tally, 3 );
	}

	void Created (GameObject * obj) override
	{
		last_health = ScriptEngine::Get_Max_Health(obj);
		damage_tally = 0;

	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager != STAR)
		{
			current_health = ScriptEngine::Get_Health (obj);
			float damage = ((last_health - current_health) + damage_tally);
			float mod_damage = (damage * .25f);
			damage_tally += mod_damage;

			ScriptEngine::Set_Health (obj, (last_health - mod_damage));
			last_health = ScriptEngine::Get_Health (obj);
			current_health = ScriptEngine::Get_Health (obj);
		}
	}
};

DECLARE_SCRIPT (M09_Nod_Damage_Mod_10, "")
{
	float last_health, current_health, damage_tally;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last_health, 1 );
		SAVE_VARIABLE( current_health, 2 );
		SAVE_VARIABLE( damage_tally, 3 );
	}

	void Created (GameObject * obj) override
	{
		last_health = ScriptEngine::Get_Max_Health(obj);
		damage_tally = 0;

	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		if (damager != STAR)
		{
			current_health = ScriptEngine::Get_Health (obj);
			float damage = ((last_health - current_health) + damage_tally);
			float mod_damage = (damage * .08f);
			damage_tally += mod_damage;

			ScriptEngine::Set_Health (obj, (last_health - mod_damage));
			last_health = ScriptEngine::Get_Health (obj);
			current_health = ScriptEngine::Get_Health (obj);
		}
	}
};

DECLARE_SCRIPT (M09_Stealth_Tank_Pilot, "")
{
	void Created (GameObject *obj) override
	{
		ScriptEngine::Set_Facing(obj, 35.0f);
		ScriptEngine::Innate_Disable ( obj );
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		if (enemy == STAR)
		{
	//		Vector3 location = ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000091));

			ActionParamsStruct params;

			params.Set_Basic( this, 99, RUN_TO_TANK );
			params.Set_Movement( ScriptEngine::Find_Object (2000090), RUN, 4.0f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if (action_id == RUN_TO_TANK && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Destroy_Object (obj);

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000090), ATTACK, ATTACK, 0.0f);
		}
	}

};

DECLARE_SCRIPT (M09_Stationary_StealthTank, "")
{


	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == ATTACK && param == ATTACK)
		{
			Vector3 stealth_loc = ScriptEngine::Get_Position (obj);
			ScriptEngine::Destroy_Object (obj);

			GameObject *stealth_tank;
			stealth_tank = ScriptEngine::Create_Object ( "Nod_Stealth_Tank", stealth_loc );
			ScriptEngine::Attach_Script (stealth_tank, "M09_Vehicle_Attack_01", "");
			ScriptEngine::Attach_Script (stealth_tank, "M09_CheckpointA_Counter", "");
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 905, 3, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 905, 1, 0.0f);
		}
	}
};


DECLARE_SCRIPT (M09_Vehicle_Attack_01, "")
{
	bool attacking, charge, charging;

	Vector3 current_loc;
	Vector3 enemy_loc;

	enum{ATTACK_OVER};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( attacking, 1 );
		SAVE_VARIABLE( charge, 2 );
		SAVE_VARIABLE( charging, 3 );
		SAVE_VARIABLE( current_loc, 4 );
		SAVE_VARIABLE( enemy_loc, 5 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Enable_Enemy_Seen( obj, true);
		attacking = false;
		charge = false;
		charging = false;

		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, 10 );
		params.Set_Movement( Vector3(0,0,0), 1.0f, 0 );
		params.WaypathID = 2000095;

		ScriptEngine::Action_Attack (obj, params);
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		current_loc = ScriptEngine::Get_Position ( obj );
		enemy_loc = ScriptEngine::Get_Position ( enemy );

		if (!attacking && !charge)
		{
			if ((ScriptEngine::Get_Distance(current_loc, enemy_loc)) < 65)
			{
				attacking = true;

				ActionParamsStruct params;
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN +5, 10 );
				params.Set_Movement( Vector3(0,0,0), 1.0f, 0 );
				params.WaypathID = 2000095;
				params.Set_Attack(STAR, 100.0f, 5.0f, true);
				params.AttackCheckBlocked = false;
				params.AttackActive = true;
				ScriptEngine::Modify_Action(obj, 10, params, true, true);

				ScriptEngine::Start_Timer(obj, this, 5.0f, ATTACK_OVER);

				if (!charging)
				{
					charging = true;
					ScriptEngine::Start_Timer(obj, this, 16.0f, CHANGE_ATTACK);
				}
			}
		}

	}

	void Timer_Expired (GameObject* obj, int timer_id) override
	{
		ActionParamsStruct params;

		if (timer_id == ATTACK_OVER)
		{
			attacking = false;
		}

		else if (timer_id == CHANGE_ATTACK)
		{
			if ((ScriptEngine::Get_Distance(current_loc, enemy_loc)) < 65)
			{
				ScriptEngine::Action_Reset ( obj, 100 );
				charge = true;
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
				params.Set_Movement( STAR, 1.5f, 0 );
				params.Set_Attack(STAR, 100.0f, 5.0f, true);
				params.AttackCheckBlocked = false;
				params.AttackActive = true;
				ScriptEngine::Action_Attack (obj, params);

				ScriptEngine::Start_Timer(obj, this, 6.0f, CHARGE_COMPLETE);
			}

		}

		else if (timer_id == CHARGE_COMPLETE)
		{
			charge = false;
			charging = false;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN +5, 10 );
			params.Set_Movement( Vector3(0,0,0), 1.0f, 0 );
			params.WaypathID = 2000095;
			params.Set_Attack(STAR, 100.0f, 5.0f, true);
			params.AttackCheckBlocked = false;
			params.AttackActive = true;
			ScriptEngine::Action_Attack (obj, params);

		}

	}

};

DECLARE_SCRIPT (M09_Vehicle_Attack_02, "")
{
	bool attacking, charge, charging;

	Vector3 current_loc;
	Vector3 enemy_loc;

	enum{ATTACK_OVER};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( attacking, 1 );
		SAVE_VARIABLE( charge, 2 );
		SAVE_VARIABLE( charging, 3 );
		SAVE_VARIABLE( current_loc, 4 );
		SAVE_VARIABLE( enemy_loc, 5 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
		ScriptEngine::Enable_Enemy_Seen( obj, true);
		attacking = false;
		charge = false;
		charging = false;
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		current_loc = ScriptEngine::Get_Position ( obj );
		enemy_loc = ScriptEngine::Get_Position ( enemy );

		if (!attacking)
		{
			if ((ScriptEngine::Get_Distance(current_loc, enemy_loc)) < 200.0f)
			{
				attacking = true;

				ActionParamsStruct params;
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
				params.Set_Attack(STAR, 200.0f, 5.0f, true);
				params.AttackCheckBlocked = false;
				params.AttackActive = true;
				ScriptEngine::Action_Attack (obj, params);

				ScriptEngine::Start_Timer(obj, this, 1.0f, ATTACK_OVER);

			}
		}

	}

	void Timer_Expired (GameObject* /* obj */, int timer_id) override
	{
		ActionParamsStruct params;

		if (timer_id == ATTACK_OVER)
		{
			attacking = false;

		}

	}

};

DECLARE_SCRIPT (M09_Sam_Engineer_1, "")
{
	void Created(GameObject * obj) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 99, 10 );
		params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000102)), RUN, 1.5f );
		ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if (action_id == 10)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 99, 1);
			params.Set_Animation( "H_A_a0f0", true );
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}
};

DECLARE_SCRIPT (M09_Sam_Engineer_2, "")
{
	void Created(GameObject * obj) override
	{
		ActionParamsStruct params;
		params.Set_Basic( this, 99, 10 );
		params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000103)), RUN, 1.5f );
		ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if (action_id == 10)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 99, 1);
			params.Set_Animation( "H_A_a0f0", true );
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}
};


DECLARE_SCRIPT (M09_Excavation_Tunnel_Encounter_On, "")
{
	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if (ScriptEngine::Is_A_Star(enterer))
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000406), ACTIVATE, ACTIVATE, 0.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Excavation_Tunnel_Encounter_Off, "")
{
	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if (ScriptEngine::Is_A_Star(enterer))
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000406), DEACTIVATE, DEACTIVATE, 0.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Excavation_Tunnel_Controller, "")  //2000406
{
	void Custom(GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == ACTIVATE && param == ACTIVATE)
		{
			//nod spawners
			ScriptEngine::Enable_Spawner(2000357, true);
			ScriptEngine::Enable_Spawner(2000354, true);
			ScriptEngine::Enable_Spawner(2000356, true);

			//mutant spawners
			ScriptEngine::Enable_Spawner(2000384, true);
			ScriptEngine::Enable_Spawner(2000383, true);
			ScriptEngine::Enable_Spawner(2000382, true);
			ScriptEngine::Enable_Spawner(2000381, true);
		}

		if (type == DEACTIVATE && param == DEACTIVATE)
		{
			//nod spawners
			ScriptEngine::Enable_Spawner(2000357, false);
			ScriptEngine::Enable_Spawner(2000354, false);
			ScriptEngine::Enable_Spawner(2000356, false);

			//mutant spawners
			ScriptEngine::Enable_Spawner(2000384, false);
			ScriptEngine::Enable_Spawner(2000383, false);
			ScriptEngine::Enable_Spawner(2000382, false);
			ScriptEngine::Enable_Spawner(2000381, false);
		}
	}
};

DECLARE_SCRIPT (M09_Vehicle_Attack_03, "")
{
	bool attacking, charge, charging;

	Vector3 current_loc;
	Vector3 enemy_loc;

	enum{ATTACK_OVER};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( attacking, 1 );
		SAVE_VARIABLE( charge, 2 );
		SAVE_VARIABLE( charging, 3 );
		SAVE_VARIABLE( current_loc, 4 );
		SAVE_VARIABLE( enemy_loc, 5 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_NOD );
		ScriptEngine::Enable_Enemy_Seen( obj, true);
		attacking = false;
		charge = false;
		charging = false;
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		current_loc = ScriptEngine::Get_Position ( obj );
		enemy_loc = ScriptEngine::Get_Position ( enemy );

		if (!attacking)
		{
			if ((ScriptEngine::Get_Distance(current_loc, enemy_loc)) < 200.0f)
			{
				attacking = true;

				ActionParamsStruct params;
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
				params.Set_Attack(enemy, 200.0f, 10.0f, true);
				params.AttackCheckBlocked = false;
				params.AttackActive = true;
				ScriptEngine::Action_Attack (obj, params);

				ScriptEngine::Start_Timer(obj, this, 10.0f, ATTACK_OVER);

			}
		}

	}

	void Timer_Expired (GameObject* /* obj */, int timer_id) override
	{
		ActionParamsStruct params;

		if (timer_id == ATTACK_OVER)
		{
			attacking = false;

		}

	}

};

DECLARE_SCRIPT (M09_Tunnel_Spawner_357, "")
{
	void Created (GameObject * obj) override
	{
			ActionParamsStruct params;
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, 10 );
			params.Set_Movement( Vector3(0,0,0), RUN, 0 );
			params.WaypathID = 2000364;
			ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if(action_id == 10)
		{
			ScriptEngine::Enable_Enemy_Seen( obj, true);
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}
};

DECLARE_SCRIPT (M09_Tunnel_Spawner_354, "")
{
	void Created (GameObject * obj) override
	{
			ActionParamsStruct params;
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, 10 );
			params.Set_Movement( Vector3(0,0,0), RUN, 0 );
			params.WaypathID = 2000370;
			ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if(action_id == 10)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}
};

DECLARE_SCRIPT (M09_Tunnel_Spawner_356, "")
{
	void Created (GameObject * obj) override
	{
			ActionParamsStruct params;
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, 10 );
			params.Set_Movement( Vector3(0,0,0), RUN, 0 );
			params.WaypathID = 2000375;
			ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if(action_id == 10)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, true );
		}
	}
};

DECLARE_SCRIPT (M09_Tunnel_Spawner_384_382, "")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Aggressiveness ( obj, 1 );

		ActionParamsStruct params;
		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
		params.Set_Movement( Vector3(0,0,0), RUN, 0.25f );
		params.WaypathID = 2000385;
		ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ScriptEngine::Debug_Message("Action Complete on M09_Tunnel_Spawner_383_381 %d %d.\n", action_id, reason);
		if(action_id == 10)
		{
			ScriptEngine::Action_Reset ( obj, 100 );

			ScriptEngine::Enable_Enemy_Seen( obj, true);
		}
	}
};

DECLARE_SCRIPT (M09_Tunnel_Spawner_383_381, "")
{
	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Aggressiveness ( obj, 1 );

		ActionParamsStruct params;
		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
		params.Set_Movement( Vector3(0,0,0), RUN, .25f );
		params.WaypathID = 2000397;
		ScriptEngine::Action_Goto (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ScriptEngine::Debug_Message("Action Complete on M09_Tunnel_Spawner_383_381 %d %d.\n", action_id, reason);
		if(action_id == 10)
		{
			ScriptEngine::Action_Reset ( obj, 100 );

			ScriptEngine::Enable_Enemy_Seen( obj, true);
		}
	}
};


DECLARE_SCRIPT (M09_Excavation_Tunnel_3, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			//Nod soldiers
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000170), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000170), false );

			//Mutant soldiers
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000171), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000171), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000171), false );
		}
	}

};

DECLARE_SCRIPT (M09_Entrance_Zone, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * /* obj */, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			ScriptEngine::Enable_Spawner(2000105, true);
			ScriptEngine::Enable_Spawner(2000106, true);

			//Nod soldiers
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000234), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000234), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000235), "M09_Nod_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000235), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000236), "M09_Nod_Damage_Mod_1", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000236), false );

			//Mutant soldiers
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000216), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000216), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000217), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000217), false );
			ScriptEngine::Attach_Script(ScriptEngine::Find_Object (2000218), "M09_Mutant_Damage_Mod_10", "");
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000218), false );

			ActionParamsStruct params;
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
			params.Set_Movement( Vector3(0,0,0), 1.0f, 0 );
			params.WaypathID = 2000222;
			ScriptEngine::Action_Goto (ScriptEngine::Find_Object (2000217), params);

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 11 );
			params.Set_Movement( Vector3(0,0,0), 1.0f, 0 );
			params.WaypathID = 2000230;
			ScriptEngine::Action_Goto (ScriptEngine::Find_Object (2000216), params);

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 12 );
			params.Set_Movement( Vector3(0,0,0), 1.0f, 0 );
			params.WaypathID = 2000226;
			ScriptEngine::Action_Goto (ScriptEngine::Find_Object (2000218), params);
		}
	}

};

DECLARE_SCRIPT (M09_CheckpointA_Controller, "") //2000969
{
	int count;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( count, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		count = 0;
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == COUNTER && param == COUNTER)
		{
			if (count++ >= 12)
			{
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 904, 3, 0.0f);
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 904, 1, 0.0f);
			}
		}
	}
};



DECLARE_SCRIPT (M09_CheckpointA_Counter, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000969), COUNTER, COUNTER, 0.0f);
	}
};

DECLARE_SCRIPT(M09_Chinook_ParaDrop, "Preset:string")
{
	int chinook_id;
	bool dead;
	int out;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(chinook_id, 1);
		SAVE_VARIABLE(dead, 2);
		SAVE_VARIABLE(out, 3);
	}

	void Created(GameObject * obj) override
	{
		Vector3 loc = ScriptEngine::Get_Position(obj);
		float facing = ScriptEngine::Get_Facing(obj);

		GameObject *chinook_rail = ScriptEngine::Create_Object("Generic_Cinematic", loc);
		ScriptEngine::Set_Model(chinook_rail, "X5D_Chinookfly");
		ScriptEngine::Set_Facing(chinook_rail, facing);
		ScriptEngine::Set_Animation(chinook_rail, "X5D_Chinookfly.X5D_Chinookfly", false, nullptr, 0.0f, -1.0f, false);
		GameObject *chinook = ScriptEngine::Create_Object("Nod_Chinook", loc);
		ScriptEngine::Set_Facing(chinook, facing);
		ScriptEngine::Set_Animation(chinook, "v_nod_chinook.vf_nod_chinook", true, nullptr, 0.0f, -1.0f, false);
		ScriptEngine::Attach_To_Object_Bone(chinook, chinook_rail, "BN_Chinook_1");

		dead = false;
		out = 0;
		char params[12];
		sprintf(params, "%d", ScriptEngine::Get_ID(obj));
		ScriptEngine::Attach_Script(chinook, "M03_Reinforcement_Chinook", params);

		chinook_id = ScriptEngine::Get_ID(chinook);

		// Destroy Chinook
		ScriptEngine::Start_Timer(obj, this, 280.0f/30.0f, 0);
		// Parachutes
		ScriptEngine::Start_Timer(obj, this, 169.0f/30.0f, 1);
		ScriptEngine::Start_Timer(obj, this, 179.0f/30.0f, 2);
		ScriptEngine::Start_Timer(obj, this, 198.0f/30.0f, 3);
		// Soldiers
		ScriptEngine::Start_Timer(obj, this, 145.0f/30.0f, 4);
		ScriptEngine::Start_Timer(obj, this, 155.0f/30.0f, 5);
		ScriptEngine::Start_Timer(obj, this, 165.0f/30.0f, 6);


	}

	void Timer_Expired(GameObject * obj, int timer_id) override
	{
		Vector3 loc = ScriptEngine::Get_Position(obj);
		const char * preset = Get_Parameter("Preset");
		float facing = ScriptEngine::Get_Facing(obj);

		switch (timer_id)
		{
		case 0:
			GameObject *chinook;
			chinook = ScriptEngine::Find_Object(chinook_id);
			ScriptEngine::Destroy_Object(chinook);
			break;
		case 1:
			if (out >= 1)
			{
				GameObject *para1;
				para1 = ScriptEngine::Create_Object("Generic_Cinematic", loc);
				ScriptEngine::Set_Facing(para1, facing);
				ScriptEngine::Set_Model(para1, "X5D_Parachute");
				ScriptEngine::Set_Animation(para1, "X5D_Parachute.X5D_ParaC_1", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Create_3D_Sound_At_Bone("parachute_open", para1, "ROOTTRANSFORM");
				ScriptEngine::Attach_Script(para1, "M03_No_More_Parachute", "");
			}
			break;
		case 2:
			if (out >= 2)
			{
				GameObject *para2;
				para2 = ScriptEngine::Create_Object("Generic_Cinematic", loc);
				ScriptEngine::Set_Facing(para2, facing);
				ScriptEngine::Set_Model(para2, "X5D_Parachute");
				ScriptEngine::Set_Animation(para2, "X5D_Parachute.X5D_ParaC_2", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Create_3D_Sound_At_Bone("parachute_open", para2, "ROOTTRANSFORM");
				ScriptEngine::Attach_Script(para2, "M03_No_More_Parachute", "");
			}
			break;
		case 3:
			if (out == 3)
			{
				GameObject *para3;
				para3 = ScriptEngine::Create_Object("Generic_Cinematic", loc);
				ScriptEngine::Set_Facing(para3, facing);
				ScriptEngine::Set_Model(para3, "X5D_Parachute");
				ScriptEngine::Set_Animation(para3, "X5D_Parachute.X5D_ParaC_3", false, nullptr, 0.0f, -1.0f, false);
				ScriptEngine::Create_3D_Sound_At_Bone("parachute_open", para3, "ROOTTRANSFORM");
				ScriptEngine::Attach_Script(para3, "M03_No_More_Parachute", "");
			}
			break;
		case 4:
			if (!dead)
			{

			GameObject *box1 = ScriptEngine::Create_Object("Generic_Cinematic", loc);
			ScriptEngine::Set_Model(box1, "X5D_Box01");
			ScriptEngine::Set_Facing(box1, facing);
			ScriptEngine::Set_Animation(box1, "X5D_Box01.X5D_Box01", false, nullptr, 0.0f, -1.0f, false);

			GameObject *soldier1;
			soldier1 = ScriptEngine::Create_Object_At_Bone(box1, preset, "Box01");
			ScriptEngine::Set_Facing(soldier1, facing);
			ScriptEngine::Attach_Script(soldier1, "RMV_Trigger_Killed", "1144444, 1000, 1000");

			ScriptEngine::Attach_Script(soldier1, "M09_CheckpointA_Counter", "");

			ScriptEngine::Attach_To_Object_Bone( soldier1, box1, "Box01" );
			ScriptEngine::Set_Animation(soldier1, "s_a_human.H_A_X5D_ParaT_1", false, nullptr, 0.0f, -1.0f, false);
			out++;
			if ((out - 1) == DIFFICULTY)
			{
				dead = true;
			}

			}
			break;
		case 5:
			if (!dead)
			{

			GameObject *box2 = ScriptEngine::Create_Object("Generic_Cinematic", loc);
			ScriptEngine::Set_Model(box2, "X5D_Box02");
			ScriptEngine::Set_Facing(box2, facing);
			ScriptEngine::Set_Animation(box2, "X5D_Box02.X5D_Box02", false, nullptr, 0.0f, -1.0f, false);

			GameObject *soldier2;
			soldier2 = ScriptEngine::Create_Object_At_Bone(box2, preset, "Box02");
			ScriptEngine::Set_Facing(soldier2, facing);
			ScriptEngine::Attach_Script(soldier2, "RMV_Trigger_Killed", "1144444, 1000, 1000");

			ScriptEngine::Attach_Script(soldier2, "M09_CheckpointA_Counter", "");

			ScriptEngine::Set_Animation(soldier2, "s_a_human.H_A_X5D_ParaT_2", false, nullptr, 0.0f, -1.0f, false);
			ScriptEngine::Attach_To_Object_Bone( soldier2, box2, "Box02" );
			out++;
			if ((out - 1) == DIFFICULTY)
			{
				dead = true;
			}

			}
			break;
		case 6:
			if (!dead)
			{

			GameObject *box3 = ScriptEngine::Create_Object("Generic_Cinematic", loc);
			ScriptEngine::Set_Model(box3, "X5D_Box03");
			ScriptEngine::Set_Facing(box3, facing);
			ScriptEngine::Set_Animation(box3, "X5D_Box03.X5D_Box03", false, nullptr, 0.0f, -1.0f, false);

			GameObject *soldier3;
			soldier3 = ScriptEngine::Create_Object_At_Bone(box3, preset, "Box03");
			ScriptEngine::Set_Facing(soldier3, facing);
			ScriptEngine::Attach_Script(soldier3, "RMV_Trigger_Killed", "1144444, 1000, 1000");

			ScriptEngine::Attach_Script(soldier3, "M09_CheckpointA_Counter", "");

			ScriptEngine::Set_Animation(soldier3, "s_a_human.H_A_X5D_ParaT_3", false, nullptr, 0.0f, -1.0f, false);
			ScriptEngine::Attach_To_Object_Bone( soldier3, box3, "Box03" );
			out++;
			if ((out - 1) == DIFFICULTY)
			{
				dead = true;
			}

			}
			break;

		}
	}

	void Custom(GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == 23000 && param == 23000)
		{
			dead = true;
		}
	}
};

DECLARE_SCRIPT (M09_First_Mutant_Encounter_Zone, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			GameObject *mutant1 = ScriptEngine::Find_Object (2000280);

			if (mutant1)
			{

				ScriptEngine::Attach_Script (mutant1, "M09_First_Mutant_Encounter", "");
				ScriptEngine::Start_Timer (obj, this, 1.0f, 100);
			}

		}
	}

	void Timer_Expired(GameObject * /* obj */, int timer_id ) override
	{
		if (timer_id == 100)
		{
			GameObject *mutant2 = ScriptEngine::Find_Object (2000281);

			ScriptEngine::Attach_Script (mutant2, "M09_First_Mutant_Encounter", "");
		}
	}
};



DECLARE_SCRIPT (M09_First_Mutant_Encounter, "")
{

	void Created (GameObject *obj) override
	{
		ScriptEngine::Start_Timer (obj, this, 2.0f, 20);
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		if (timer_id == 20)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 99, 100);
			params.Set_Animation( "H_A_J26C", false );
			ScriptEngine::Action_Play_Animation (obj, params);

			ScriptEngine::Start_Timer (obj, this, 2.0f, 20);
		}
	}
};

DECLARE_SCRIPT (M09_Immobile, "")
{
	void Created (GameObject *obj) override
	{
		ScriptEngine::Innate_Disable ( obj );
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		if (enemy == STAR)
		{
			ScriptEngine::Innate_Enable(obj);

			ActionParamsStruct params;
			params.Set_Attack( STAR, 50.0f, 5.0f, true );
			ScriptEngine::Action_Attack( obj, params );
		}
	}
};

DECLARE_SCRIPT (M09_Mutant, "")
{
	void Created (GameObject *obj) override
	{
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_MUTANT );
	}
};

DECLARE_SCRIPT (M09_Explosion_Zone_Lab01, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			ScriptEngine::Create_Explosion("Ground Explosions Twiddler", Vector3 (-102.560f, 483.462f ,-155.543f), nullptr);
			ScriptEngine::Start_Timer (obj, this, 1.0f, 100);
		}
	}
};

DECLARE_SCRIPT (M09_Explosion_Zone_Lab02, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * /* obj */, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			ScriptEngine::Create_Explosion("Ground Explosions Twiddler", Vector3 (-6.151f, 494.846f ,-151.888f), nullptr);
			GameObject *blocker1;
			blocker1 = ScriptEngine::Create_Object ( "M08_Rubble_Stub", Vector3 (-6.151f, 494.846f ,-151.888f));
			ScriptEngine::Set_Facing ( blocker1, 60.0f );
		}
	}
};

DECLARE_SCRIPT (M09_Explosion_Zone_Tunnel01, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), SPAWN_LOC, INCREMENT, 0.0f);

			ScriptEngine::Create_Explosion("Ground Explosions Twiddler", Vector3 (-70.356f, 475.905f ,-63.233f), nullptr);
			GameObject *blocker1;
			blocker1 = ScriptEngine::Create_Object ( "M08_Rubble_Stub", Vector3 (-74.290f, 474.747f, -64.022f));
			ScriptEngine::Set_Facing(blocker1, 90.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Sam_Counter, "")
{
	void Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000331), COUNTER, COUNTER, 0.0f);
	}
};

DECLARE_SCRIPT (M09_Sam_Controller, "") //2000331
{
	int count;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( count, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		count = 0;
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == COUNTER && param == COUNTER)
		{
			count++;
		}

		if (count >= 3)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), 906, 1, 0.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Unteamed, "")
{
	void Created (GameObject *obj) override
	{
		ScriptEngine::Set_Player_Type ( obj, SCRIPT_PLAYERTYPE_SPECTATOR );
	}
};

DECLARE_SCRIPT (M09_Mutant_Excavation_Zone_01, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			//Mutant soldiers

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000158), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000158), false );
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000159), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000159), false );
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000160), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000160), false );
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000161), CHANGE_PLAYERTYPE, MUTANT, 0.0f);
			ScriptEngine::Set_Innate_Is_Stationary ( ScriptEngine::Find_Object (2000161), false );

			//noddities

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000347), ATTACK, 2000158, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000175), ATTACK, 2000159, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000346), ATTACK, 2000160, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000174), ATTACK, 2000161, 0.0f);



		}
	}

};

DECLARE_SCRIPT (M09_Custom_Attack, "")
{
	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == ATTACK)
		{
			ScriptEngine::Set_Innate_Is_Stationary ( obj, false );

			ActionParamsStruct params;
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
			params.Set_Attack(ScriptEngine::Find_Object (param), 200.0f, 5.0f, true);
			params.AttackCheckBlocked = false;
			params.AttackActive = true;
			ScriptEngine::Action_Attack (obj, params);
		}
	}
};

DECLARE_SCRIPT (M09_Flamer_Attack_Zone, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			//Flamer Customs
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000185), ATTACK, (ScriptEngine::Get_ID (STAR)), 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000186), ATTACK, (ScriptEngine::Get_ID (STAR)), 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000187), ATTACK, (ScriptEngine::Get_ID (STAR)), 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (2000188), ATTACK, (ScriptEngine::Get_ID (STAR)), 0.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Cryo_Mutant_Zone_01, "")
{
	bool already_entered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			ScriptEngine::Start_Timer (obj, this, 1.0f, 1000);
			ScriptEngine::Enable_Spawner(2000417, true);
			ScriptEngine::Enable_Spawner(2000418, true);

		}
	}

	void Timer_Expired (GameObject * /* obj */, int timer_id) override
	{
		if (timer_id == 1000)
		{
			ScriptEngine::Enable_Spawner(2000416, true);
			ScriptEngine::Enable_Spawner(2000424, true);
		}
	}
};

DECLARE_SCRIPT (M09_Mutant_Path_01, "")
{
	void Action_Complete(GameObject * obj, int /* action_id */, ActionCompleteReason /* reason */) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, 99, 101 );
		params.Set_Movement( Vector3(0,0,0), RUN, 0.5f);
		params.WaypathID = 2000425;

		ScriptEngine::Action_Goto (obj, params);
	}
};

DECLARE_SCRIPT (M09_Mutant_Path_02, "")
{
	void Created (GameObject *obj) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, 99, 101 );
		params.Set_Movement( Vector3(0,0,0), RUN, 0.5f);
		params.WaypathID = 2000443;

		ScriptEngine::Action_Goto (obj, params);
	}
};

DECLARE_SCRIPT (M09_Key_Grant, "")
{
	void Poked(GameObject * /* obj */, GameObject * poker) override
	{
		if (poker == STAR)
		{

			GameObject *key1 = ScriptEngine::Create_Object ( "Level_01_Keycard", Vector3 (-97.303f, 520.441f, -153.080f) );
			ScriptEngine::Attach_Script (key1, "M09_Containment_Key_1", "");
		}
	}
};

DECLARE_SCRIPT (M09_Lab_Key_Controller, "") //2000452
{
	int key_counter;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( key_counter, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		key_counter = 0;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * sender ) override
	{
		if ( type == KEY_COUNTER )
		{
			if (param == INCREMENT)
			{
				key_counter++;
			}
		}

		if (type == CHECK)
		{
			ScriptEngine::Send_Custom_Event( obj, sender, COUNT, key_counter, 0.0f );
		}
	}
};

DECLARE_SCRIPT (M09_Containment_Key_1, "")
{
	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), KEY_COUNTER, INCREMENT, 0.0f );

			GameObject *mobius = ScriptEngine::Find_Object (2000010);

			ActionParamsStruct params;

			params.Set_Basic( this, 99, MOBIUS_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000988)), 1.2f, 0.8f );

			ScriptEngine::Action_Goto (mobius, params);
		}
	}
};

DECLARE_SCRIPT (M09_Containment_Key_2, "")
{
	void Custom( GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), KEY_COUNTER, INCREMENT, 0.0f );

			GameObject *mobius = ScriptEngine::Find_Object (2000010);

			ActionParamsStruct params;

			params.Set_Basic( this, 99, MOBIUS_GOTO );
			params.Set_Movement( ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000988)), 1.2f, 0.8f );

			ScriptEngine::Action_Goto (mobius, params);

			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (2000699));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (2001988));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (2000457));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (2000709));
		}
	}

		/*if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), KEY_COUNTER, INCREMENT, 0.0f );
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000709), GOAL, DECREMENT, 0.0f );
		}
	}*/
};

DECLARE_SCRIPT (M09_Key_Controller_Zones, "")
{
	bool already_triggered;
	int key_count;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_triggered, 1 );
		SAVE_VARIABLE( key_count, 2);
	}

	void Created (GameObject * /* obj */) override
	{
		already_triggered = false;

	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_triggered)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), CHECK, 0, 0.0f );
		}
	}

	void Custom( GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if ( type == COUNT )
		{
			key_count = param;
		}
	}
};

DECLARE_SCRIPT (M09_Key_Box, "")
{
	bool poke;
	bool opened;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poke, 1 );
		SAVE_VARIABLE( opened, 2 );
	}

	void Created (GameObject * /* obj */) override
	{
		poke = false;
		opened = false;
	}

	void Poked(GameObject * obj, GameObject * /* poker */) override
	{
		if (poke)
		{
			opened = true;

			ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1265140, 0, "lab_spn_door08.lab_spn_door08" );
			GameObject *mobius;
			mobius = ScriptEngine::Find_Object (2000010);

			const char *conv_name = ("IDS_M09_D18");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 999);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if (action_id == 999)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), FOLLOW, 2000860, 0.0f );
		}
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if ( type == VERIFY )
		{
			if (param == GO)
			{
				poke = true;
			}

			if (param == STOP)
			{
				poke = false;

				if (!opened)
				{
					GameObject *mobius;
					mobius = ScriptEngine::Find_Object (2000010);

					const char *conv_name = ("IDS_M09_D19");
					int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
					ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
					ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
					ScriptEngine::Start_Conversation (conv_id, 900);
					ScriptEngine::Monitor_Conversation (obj, conv_id);

					ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), WAYPATH, 2000462, 0.0f );
				}
			}
		}
	}
};

DECLARE_SCRIPT (M09_MrShuman_Zone, "")
{
	bool star_in_zone;
	bool fired_off;
	bool key;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( star_in_zone, 1);
		SAVE_VARIABLE( fired_off, 2);
		SAVE_VARIABLE( key, 3);
	}

	void Created (GameObject * /* obj */) override
	{
		star_in_zone = false;
		fired_off = false;
		key = false;

	}

	void Timer_Expired(GameObject * obj, int timer_id) override
	{
		if (timer_id == DIST_CHECK && star_in_zone)
		{
			float mobius_distance = ScriptEngine::Get_Distance(ScriptEngine::Get_Position (STAR), ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000010)));

			if (mobius_distance <= 7.0 && star_in_zone && !fired_off)
			{
				fired_off = true;

				GameObject *shuman = ScriptEngine::Find_Object (2000456);
				GameObject *mobius = ScriptEngine::Find_Object (2000010);

				ScriptEngine::Send_Custom_Event ( obj, shuman, CLARK_KENT, 0, 0.0f);

				if (shuman)
				{
					const char *conv_name = ("IDS_M09_D14");
					int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
					ScriptEngine::Join_Conversation(shuman, conv_id, false, true, true);
					ScriptEngine::Join_Conversation(mobius, conv_id, false, true, true);
					ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
					ScriptEngine::Start_Conversation (conv_id, 954);
					ScriptEngine::Monitor_Conversation (obj, conv_id);
				}
			}

			else
			{
				ScriptEngine::Start_Timer (obj, this, 2.0f, DIST_CHECK);
			}
		}
	}

	void Exited( GameObject * /* obj */, GameObject * /* exiter */ ) override
	{
		star_in_zone = false;
	}

	void Custom(GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == LOCKBOX)
		{
			key = true;
		}
	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		star_in_zone = true;

		float mobius_distance = ScriptEngine::Get_Distance(ScriptEngine::Get_Position (STAR), ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000010)));

		if (mobius_distance <= 7.0 && star_in_zone && !fired_off)
		{
			fired_off = true;

			GameObject *shuman = ScriptEngine::Find_Object (2000456);
			GameObject *mobius = ScriptEngine::Find_Object (2000010);

			ScriptEngine::Send_Custom_Event ( obj, shuman, CLARK_KENT, 0, 0.0f);

			const char *conv_name = ("IDS_M09_D14");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(shuman, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 954);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}

		else
		{
			ScriptEngine::Start_Timer (obj, this, 2.0f, DIST_CHECK);
		}

	}

	void Action_Complete(GameObject * /* obj */, int action_id, ActionCompleteReason /* reason */) override
	{
		if(action_id == 954)
		{
			/*GameObject *key2 = ScriptEngine::Create_Object ( "Level_02_Keycard", Vector3 (-76.473f, 457.493f, -153.807f) );
			ScriptEngine::Attach_Script (key2, "M09_Containment_Key_2", "");*/

			GameObject *shuman = ScriptEngine::Find_Object (2000456);

			ActionParamsStruct params;

			params.Set_Basic( this, 99, 101 );
			params.Set_Movement( Vector3(0,0,0), RUN, 0.5f);
			params.WaypathID = 1200474;

			ScriptEngine::Action_Goto (shuman, params);

		}
	}
};

DECLARE_SCRIPT (M09_Invincible_MrShuman, "")
{
	bool invincible;

	void Created (GameObject * /* obj */) override
	{
		invincible = true;
	}

	void Custom (GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == CLARK_KENT)
		{
			invincible = false;
		}
	}

	void Damaged (GameObject *obj, GameObject * /* damager */, float /* amount */) override
	{
		if (invincible)
		{
			ScriptEngine::Set_Health (obj, 5.0f);
		}
	}
};



DECLARE_SCRIPT (M09_Elevator_Movement_Zone, "Direction:int, Anim_num:int, Elev_obj_num:int")
{
	void Entered (GameObject * /* obj */, GameObject * enterer) override
	{
		static constexpr const char* elevators[9] =
		{
			"l09_res_elev06.l09_res_elev06",
			"elev01.elev01",
			"res_elev04.res_elev04",
			"l09_res_elev05.l09_res_elev05",
			"res_elev11.res_elev11",
			"l09_big_elev.l09_big_elev",
			"l09_big2_elev.l09_big2_elev",
			"cent_elev01.cent_elev01",
			"lab_spn_door08.lab_spn_door08",
		};

		if (enterer == STAR)
		{
			if (Get_Int_Parameter("Direction") == 0)
			{
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( Get_Int_Parameter("Elev_obj_num"), 0, elevators [Get_Int_Parameter("Anim_num")] );
			}

			if (Get_Int_Parameter("Direction") == 1)
			{
				ScriptEngine::Static_Anim_Phys_Goto_Last_Frame ( Get_Int_Parameter("Elev_obj_num"), elevators [Get_Int_Parameter("Anim_num")] );
			}
		}
	}
};


DECLARE_SCRIPT (M09_Elevator_All_Zone, "Controller_num:int")
{
	bool star_in_zone;
	bool mobius_in_zone;
	GameObject * mobius;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( star_in_zone, 1 );
		SAVE_VARIABLE( mobius_in_zone, 2 );
	}

	void Created (GameObject * /* obj */) override
	{
		star_in_zone = false;


		mobius = ScriptEngine::Find_Object (2000010);
	}


	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if (enterer == STAR)
		{
			star_in_zone = true;
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(Get_Int_Parameter("Controller_num")), SET_STAR, ENTER, 0.0f);
		}
	}

	void Exited(GameObject * obj, GameObject * exiter) override
	{
		if (exiter == STAR)
		{
			star_in_zone = false;
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(Get_Int_Parameter("Controller_num")), SET_STAR, EXIT, 0.0f);
		}
	}
};

DECLARE_SCRIPT (M09_Elevator_All_Controller, "Waypoint_num:int, Elev_obj_num:int, Anim_num:int, Direction = 0:int, Mobius_exit_goto:int")
{
	bool star_in_zone;
	bool mobius_in_zone;
	bool transition;
	int block1, block2, block3, block4;


	GameObject *mobius;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( star_in_zone, 1 );
		SAVE_VARIABLE( mobius_in_zone, 2 );
		SAVE_VARIABLE( transition, 3 );
		SAVE_VARIABLE( block1, 4 );
		SAVE_VARIABLE( block2, 5 );
		SAVE_VARIABLE( block3, 6 );
		SAVE_VARIABLE( block4, 7 );
	}

	void Created (GameObject * /* obj */) override
	{
		star_in_zone = false;
		mobius_in_zone = false;

		block1 = block2 = block3 = block4 = 0;

		static constexpr const char *elevators[9] =
		{
			"l09_res_elev06.l09_res_elev06",
			"elev01.elev01",
			"res_elev04.res_elev04",
			"l09_res_elev05.l09_res_elev05",
			"res_elev11.res_elev11",
			"l09_big_elev.l09_big_elev",
			"l09_big2_elev.l09_big2_elev",
			"cent_elev01.cent_elev01",
			"lab_spn_door08.lab_spn_door08",
		};

		mobius = ScriptEngine::Find_Object (2000010);

		ScriptEngine::Static_Anim_Phys_Goto_Last_Frame ( Get_Int_Parameter("Elev_obj_num"), elevators [Get_Int_Parameter("Anim_num")] );


	}

	void Timer_Expired(GameObject * /* obj */, int timer_id) override
	{
		if (timer_id == 333)
		{
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block1));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block2));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block3));
			ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (block4));
		}
	}
//
	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		static constexpr const char *elevators[9] =
		{
			"l09_res_elev06.l09_res_elev06",
			"res_elev01.res_elev01",
			"res_elev04.res_elev04",
			"l09_res_elev05.l09_res_elev05",
			"res_elev11.res_elev11",
			"l09_big_elev.l09_big_elev",
			"l09_big2_elev.l09_big2_elev",
			"cent_elev01.cent_elev01",
			"lab_spn_door08.lab_spn_door08",
		};

		if (type == CHECK_STAR)
		{
			ScriptEngine::Send_Custom_Event(obj, sender, STAR_STATUS, star_in_zone, 0.0f);
		}

		if (type == CHECK_MOBIUS)
		{
			ScriptEngine::Send_Custom_Event(obj, sender, CHECK_MOBIUS, mobius_in_zone, 0.0f);
		}

		if (type == STAR_STATUS)
		{
			if (!param)
			{
				if (Get_Int_Parameter("Direction") == 0)
				{
					ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), ELEVATOR, (Get_Int_Parameter("Waypoint_num")), 0.0f);
				}

				if (Get_Int_Parameter("Direction") == 1)
				{
					ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), ELEVATOR_DOWN, (Get_Int_Parameter("Waypoint_num")), 0.0f);
				}
			}
		}

		if (type == SET_MOBIUS)
		{
			mobius_in_zone = true;

			if (star_in_zone)
			{

				if (Get_Int_Parameter("Direction") == 0)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, ACTIVATE, 0, 0.0f);
				}

				if (Get_Int_Parameter("Direction") == 1)
				{
					ScriptEngine::Send_Custom_Event( obj, obj, ACTIVATEDOWN, 0, 0.0f);
				}
			}
		}

		if (type == SET_STAR)
		{
			if (param == ENTER)
			{
				star_in_zone = true;

				if (((ScriptEngine::Get_ID (obj)) != 2000607) && ((ScriptEngine::Get_ID (obj)) != 2000599))
				{
					if (Get_Int_Parameter("Direction") == 0)
					{
						ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), ELEVATOR, (Get_Int_Parameter("Waypoint_num")), 0.0f);
					}

					if (Get_Int_Parameter("Direction") == 1)
					{
						ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), ELEVATOR_DOWN, (Get_Int_Parameter("Waypoint_num")), 0.0f);
					}
				}

				if ((ScriptEngine::Get_ID (obj)) == 2000607)
				{
					ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000599), CHECK_STAR, 0, 0.0f);
				}

				if ((ScriptEngine::Get_ID (obj)) == 2000599)
				{
					ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000607), CHECK_STAR, 0, 0.0f);
				}

			}

			if (param == EXIT)
			{
				star_in_zone = false;
				//ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), FOLLOW, Get_Int_Parameter("Mobius_exit_goto"), 0.0f);
			}
		}

		if (type == ACTIVATE)
		{
			if (star_in_zone && mobius_in_zone)
			{
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), BLOCK_ON, 0, 0.0f);

			/*	ScriptEngine::Start_Timer (obj, this, 3.0f, 333);

				GameObject *rub1 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (-108.916f, 483.904f, -191.022f));
				block1 = ScriptEngine::Get_ID (rub1);
				ScriptEngine::Set_Is_Rendered( rub1, false );
				GameObject *rub2 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (-25.433f, 513.013f, -150.613f));
				block2 = ScriptEngine::Get_ID (rub2);
				ScriptEngine::Set_Is_Rendered( rub2, false );
				GameObject *rub3 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (-47.948f, 535.216f, -103.284f));
				block3 = ScriptEngine::Get_ID (rub3);
				ScriptEngine::Set_Is_Rendered( rub3, false );
				ScriptEngine::Set_Facing (rub3, -90.0);
				GameObject *rub4 = ScriptEngine::Create_Object ("M08_Rubble_Stub", Vector3 (39.231f, 515.105f, -78.137f));
				block4 = ScriptEngine::Get_ID (rub4);
				ScriptEngine::Set_Is_Rendered( rub4, false );
*/				ScriptEngine::Static_Anim_Phys_Goto_Frame ( Get_Int_Parameter("Elev_obj_num"), 0, elevators [Get_Int_Parameter("Anim_num")] );
			}
		}

		if (type == ACTIVATEDOWN)
		{
			if (star_in_zone && mobius_in_zone)
			{
				ScriptEngine::Static_Anim_Phys_Goto_Last_Frame ( Get_Int_Parameter("Elev_obj_num"), elevators [Get_Int_Parameter("Anim_num")] );
			}
		}


	}
};

DECLARE_SCRIPT (M09_Block_Off, "")
{
	void Entered (GameObject *obj, GameObject * /* enterer */) override
	{
	 	ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000071), BLOCK_OFF, 0, 0.0f);
	}
};


DECLARE_SCRIPT (M09_Mutant_Attack, "Target_num:int")
{
	int target;

	void Created (GameObject * /* obj */) override
	{
		target = Get_Int_Parameter ("Target_num");
	}

	void Sound_Heard(GameObject* obj, const CombatSound & sound) override
	{
		target = Get_Int_Parameter ("Target_num");

		if ( sound.Type == M09_INNATE_ENABLE )
		{
			Vector3 loc = ScriptEngine::Get_Position (ScriptEngine::Find_Object (target));

			ScriptEngine::Set_Innate_Aggressiveness ( obj, 100.0f );
			ScriptEngine::Set_Innate_Take_Cover_Probability ( obj, 100.0 );

			ActionParamsStruct params;

			params.Set_Basic( this, 99, RUN_TO_TARGET );
			params.Set_Movement( loc, 1.2f, 0.75f);

			ScriptEngine::Action_Goto (obj, params);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		float face = ScriptEngine::Get_Facing (ScriptEngine::Find_Object (target));

		if (action_id == RUN_TO_TARGET)
		{
			ScriptEngine::Set_Facing (obj, face);

			ActionParamsStruct params;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, ATTACKING );
			params.Set_Attack (ScriptEngine::Find_Object (2000626), 3.0f, 0.0f, 1);

			ScriptEngine::Action_Attack( obj, params );
		}
	}
};

DECLARE_SCRIPT (M09_Waypath_Run, "Waypath_num:int, Attacker_num:int, Controller_num:int")
{
	int mutant_id;
	int attackee_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( mutant_id, 1 );
		SAVE_VARIABLE( attackee_id, 2 );
	}

	void Created (GameObject *obj) override
	{
		ScriptEngine::Innate_Disable(obj);

		attackee_id = ScriptEngine::Get_ID(obj);
		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (Get_Int_Parameter("Controller_num")), MUTANT_ATTACKEE, attackee_id, 0.0f);

		ScriptEngine::Start_Timer (obj, this, 1.0f, START);

		if (Get_Int_Parameter("Waypath_num") != 0)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, 99, RUN_TO_TARGET );
			params.Set_Movement( Vector3(0,0,0), RUN, 0.5f);
			params.WaypathID = Get_Int_Parameter("Waypath_num");

			ScriptEngine::Action_Goto (obj, params);
		}
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		ScriptEngine::Action_Reset ( obj, 100 );

		ActionParamsStruct params;

		params.Set_Basic( this, 100, ANIMATION );
		params.Set_Animation ("h_a_891a", false);
		ScriptEngine::Action_Play_Animation (obj, params);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		if (action_id == ANIMATION)
		{
			if (Get_Int_Parameter("Waypath_num") != 0)
			{
				ScriptEngine::Action_Reset ( obj, 100 );

				ActionParamsStruct params;

				params.Set_Basic( this, 99, RUN_TO_TARGET );
				params.Set_Movement( Vector3(0,0,0), RUN, 0.5f);
				params.WaypathID = Get_Int_Parameter("Waypath_num");

				ScriptEngine::Action_Goto (  obj, params );
			}
		}
	}

	void Killed(GameObject * obj, GameObject * /* killer */ ) override
	{
		ScriptEngine::Send_Custom_Event(obj, (ScriptEngine::Find_Object (mutant_id)), ATTACK_STAR, 0, 0.0f);
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == ATTACKER_CHECK)
		{
			mutant_id = param;
		}

		if (type == DEAD_MUTANT)
		{
			ScriptEngine::Action_Reset ( obj, 100 );
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{

		if(timer_id == START)
		{
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (Get_Int_Parameter("Controller_num")), ATTACKER_CHECK, 0, 0.0f);
		}
	}

};

DECLARE_SCRIPT (M09_Mutant_Encounter_Controller, "")
{
	int mutant_id;
	int attackee_id;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( mutant_id, 1 );
		SAVE_VARIABLE( attackee_id, 2 );
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		if (type == MUTANT_ATTACKER)
		{
			mutant_id = param;
		}

		if (type == MUTANT_ATTACKEE)
		{
			attackee_id = param;
		}

		if (type == ATTACKER_CHECK)
		{
			ScriptEngine::Send_Custom_Event(obj, sender, ATTACKER_CHECK, mutant_id, 0.0f);
		}

		if (type == ATTACKEE_CHECK)
		{
			ScriptEngine::Send_Custom_Event(obj, sender, ATTACKEE_CHECK, attackee_id, 0.0f);
		}
	}
};


DECLARE_SCRIPT (M09_Damage_Modifier, "Damage_multiplier:float")
{
	float current_health, last_health, damage_tally;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( current_health, 1 );
		SAVE_VARIABLE( last_health, 2 );
		SAVE_VARIABLE( damage_tally, 3 );
	}

	void Created (GameObject *obj) override
	{
		last_health = ScriptEngine::Get_Health (obj);
		damage_tally = 0;
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		float damage;

		if (damager == STAR)
		{
			last_health = ScriptEngine::Get_Health (obj);
		}

		if (damager != STAR)
		{
			current_health = ScriptEngine::Get_Health (obj);
			if (current_health == 0)
			{
				damage = ((last_health - current_health) + damage_tally);
				damage_tally = 0;
			}

			else
			{
				damage = (last_health - current_health);
				damage_tally = 0;
			}
			float mod_damage = (damage * (Get_Float_Parameter("Damage_multiplier")));
			damage_tally += mod_damage;

			ScriptEngine::Set_Health (obj, (last_health - mod_damage));
			last_health = ScriptEngine::Get_Health (obj);
			current_health = ScriptEngine::Get_Health (obj);
		}
	}
};

DECLARE_SCRIPT (M09_Zone_Enabled_Mobius, "Waypath_id:int, Condition=0:int")
{
	bool already_entered;
	int goal;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
		SAVE_VARIABLE( goal, 2 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
		goal = Get_Int_Parameter("Condition");
	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (goal <= 0)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), FOLLOW, Get_Int_Parameter("Waypath_id"), 0.0f );
			goal = 1;
		}
	}

	void Custom(GameObject * /* obj */, int type, intptr_t param, GameObject * /* sender */) override
	{
		if (type == GOAL)
		{
			if (param == INCREMENT)
			{
				goal++;
			}

			if (param == DECREMENT)
			{
				goal--;
			}
		}
	}
};

DECLARE_SCRIPT (M09_No_Obj_Damage, "")
{
	float curr_health;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( curr_health, 1 );
	}

	void Created (GameObject *obj) override
	{
		curr_health = ScriptEngine::Get_Health (obj);
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		ScriptEngine::Set_Health (obj, curr_health);
	}
};

DECLARE_SCRIPT (M09_Flyover_Controller, "")
{
	int last;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		last = 0;
	}

	void Custom( GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if (type == FLYOVER)
		{
			static constexpr const char *flyovers[8] =
			{
				"X9A_Apache_00.txt",
				"X9A_Apache_01.txt",
				"X9A_Apache_02.txt",
				"X9A_Apache_03.txt",

				"X9A_Trnspt_00.txt",
				"X9A_Trnspt_01.txt",
				"X9A_Trnspt_02.txt",
				"X9A_Trnspt_03.txt",

			};
			int random = int(ScriptEngine::Get_Random(0, 8-WWMATH_EPSILON));

			while (random == last) {
				random = int(ScriptEngine::Get_Random(0, 8-WWMATH_EPSILON));
			}
			GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
			ScriptEngine::Attach_Script(controller, "Test_Cinematic", flyovers[random]);
			last = random;
		}
	}
};

DECLARE_SCRIPT (M09_Flyover_Recycle, "")
{
	void Destroyed(GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(2000955), FLYOVER, 0, 0.0f);
	}
};

DECLARE_SCRIPT (M09_Weather_On, "")
{
	void Entered (GameObject * /* obj */, GameObject * /* enterer */) override
	{
		ScriptEngine::Set_Fog_Enable ( true );
		ScriptEngine::Set_Fog_Range (0.0, 150.0, 2.0);
		ScriptEngine::Set_Lightning (0.25f, 0.0f, 0.2f, 265.0f, 0.3f, 2.0f);
		ScriptEngine::Set_Rain ( 10.0, 2.0, false);
	}
};

DECLARE_SCRIPT (M09_Weather_Off, "")
{
	void Entered (GameObject * /* obj */, GameObject * /* enterer */) override
	{
		//ScriptEngine::Set_Fog_Enable ( false );
		ScriptEngine::Set_Fog_Range (0.0, 1000.0, 2.0);
		ScriptEngine::Set_Lightning (0.0f, 0.0f, 0.2f, 265.0f, 0.3f, 2.0f);
		ScriptEngine::Set_Rain ( 0.0, 2.0, false);
	}
};

DECLARE_SCRIPT(M09_Evac_Transport, "")  //2000969
{
	bool already_entered;
	float facing;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
		SAVE_VARIABLE( facing, 2 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
		facing = 0.0f;
	}

	void Entered (GameObject * /* obj */, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			facing = ScriptEngine::Get_Facing (ScriptEngine::Find_Object (2000969));

			Vector3 loc = ScriptEngine::Get_Position (ScriptEngine::Find_Object (2000969));

			GameObject * bone = ScriptEngine::Create_Object ( "Invisible_Object", loc );
			ScriptEngine::Set_Facing(bone, facing);
			ScriptEngine::Attach_Script(bone, "M09_Evac_Bone", "");
			ScriptEngine::Set_Model ( bone, "XG_TransprtBone" );
			ScriptEngine::Set_Animation ( bone, "XG_TransprtBone.XG_EV2_PathA", false , nullptr, 0.0f, -1.0f, false);

			GameObject * chinook = ScriptEngine::Create_Object_At_Bone( bone, "GDI_Transport_Helicopter", "XG_TransprtBone" );
			GameObject * troop = ScriptEngine::Create_Object_At_Bone( bone, "GDI_RocketSoldier_2SF", "SPAWNER" );
			ScriptEngine::Attach_Script(troop, "M00_Damage_Modifier_DME", "0.05f, 1, 1, 0, 0");

			char param1[12];
			sprintf(param1, "%d", ScriptEngine::Get_ID(troop));

			ScriptEngine::Set_Facing(chinook, facing);
			ScriptEngine::Attach_Script(chinook, "M09_Evac_Helicopter", param1);
			ScriptEngine::Attach_To_Object_Bone ( chinook, bone, "BN_Trajectory" );
			ScriptEngine::Set_Animation ( chinook, "v_GDI_trnspt.XG_EV2_trnsA", false , nullptr, 0.0f, -1.0f, false);

			ScriptEngine::Attach_Script(troop, "M09_Gunner", "");
			ScriptEngine::Attach_Script(chinook, "M09_No_Obj_Damage", "");
			ScriptEngine::Attach_To_Object_Bone ( troop, chinook, "SPAWNER" );

			//ScriptEngine::Set_Facing(troop, facing - 180.0);
		}
	}
};

DECLARE_SCRIPT(M09_Evac_Bone, "")
{
	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "XG_TransprtBone.XG_EV2_PathA") == 0)
		{
			ScriptEngine::Set_Animation ( obj, "XG_TransprtBone.XG_EV2_Pathloop", true , nullptr, 0.0f, -1.0f, false);
		}

	}
};

DECLARE_SCRIPT(M09_Evac_Helicopter, "Gunner:int")
{
	void Animation_Complete(GameObject * obj, const char *anim) override
	{
		if (stricmp(anim, "v_GDI_trnspt.XG_EV2_trnsA") == 0)
		{
			int gunner_id = Get_Int_Parameter("Gunner");

			ScriptEngine::Disable_Physical_Collisions ( obj );
			ScriptEngine::Start_Timer (obj, this, 3.0f, COLLISION);

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(gunner_id), GO, 0, 0.0f);
			ScriptEngine::Set_Animation ( obj, "v_GDI_trnspt.XG_EV2_trnsloop", true , nullptr, 0.0f, -1.0f, false);
		}
	}

	void Killed(GameObject * /* obj */, GameObject * /* killer */) override
	{
		ScriptEngine::Mission_Complete(false);
	}

	void Timer_Expired (GameObject* obj, int timer_id) override
	{
		if (timer_id == COLLISION)
		{
			ScriptEngine::Enable_Collisions ( obj );
		}
	}
};

DECLARE_SCRIPT(M09_Gunner, "")
{
	float full_health;

	void Created ( GameObject *obj) override
	{
		full_health = ScriptEngine::Get_Health (obj);
	}

	void Custom(GameObject * obj, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == GO)
		{
			ScriptEngine::Attach_To_Object_Bone( obj, nullptr, nullptr );


			//ScriptEngine::Enable_Collisions ( obj );

			ActionParamsStruct params;

			params.Set_Basic( this, 99, 101 );
			params.Set_Movement( ScriptEngine::Find_Object (2000970), RUN, 1.5f);

			ScriptEngine::Action_Goto (obj, params);

			//ScriptEngine::Set_Innate_Aggressiveness ( obj, 1.0f );
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN +5, 10 );
		params.Set_Attack(enemy, 100.0f, 5.0f, true);
		params.AttackCheckBlocked = true;
		params.AttackActive = true;

		ScriptEngine::Modify_Action(obj, 10, params, true, true);
	}

	void Damaged( GameObject * obj, GameObject * /* damager */, float /* amount */ ) override
	{
		ScriptEngine::Set_Health (obj, full_health);
	}

};

DECLARE_SCRIPT(M09_Mobius_Goto, "GotoObject:int")
{
	void Entered (GameObject *obj, GameObject * /* enterer */) override
	{
		GameObject * mobius = ScriptEngine::Find_Object (2000010);

		ScriptEngine::Send_Custom_Event( obj, mobius, FOLLOW, Get_Int_Parameter("GotoObject"), 0.0f );
	}
};

DECLARE_SCRIPT(M09_Innate_Disable, "")
{
	float current_health, last_health, damage_tally;
	//GameObject * mobius;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( current_health, 1 );
		SAVE_VARIABLE( last_health, 2 );
		SAVE_VARIABLE( damage_tally, 3 );
	}

	void Created (GameObject *obj) override
	{
		ScriptEngine::Innate_Disable(obj);

		//GameObject * mobius = ScriptEngine::Find_Object (2000010);
		last_health = ScriptEngine::Get_Health (obj);
		damage_tally = 0;
	}

	void Sound_Heard(GameObject* obj, const CombatSound & sound) override
	{
		ActionParamsStruct params;

		if ( sound.Type == M09_INNATE_ENABLE )
		{
			ScriptEngine::Innate_Enable(obj);
			ScriptEngine::Set_Innate_Aggressiveness ( obj, 100.0f );
			//ScriptEngine::Innate_Force_State_Gunshots_Heard ( obj, ScriptEngine::Get_Position (obj) );

			Vector3 projector = ScriptEngine::Get_Position (obj);
		}
	}

	void Custom (GameObject *obj, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == M09_INNATE_ENABLE )
		{
			ScriptEngine::Innate_Enable(obj);
			ScriptEngine::Set_Innate_Aggressiveness ( obj, 100.0f );
			//ScriptEngine::Innate_Force_State_Gunshots_Heard ( obj, ScriptEngine::Get_Position (obj) );
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /* amount */ ) override
	{
		GameObject * mobius = ScriptEngine::Find_Object (2000010);

		if (damager == STAR  || damager == mobius)
		{
			last_health = ScriptEngine::Get_Health (obj);
		}

		if (damager != STAR && damager != mobius)
		{
			current_health = ScriptEngine::Get_Health (obj);
			float damage = ((last_health - current_health) + damage_tally);
			float mod_damage = (damage * 0.05f);
			damage_tally += mod_damage;

			ScriptEngine::Set_Health (obj, (last_health - mod_damage));
			last_health = ScriptEngine::Get_Health (obj);
			current_health = ScriptEngine::Get_Health (obj);
		}
	}
};

DECLARE_SCRIPT(M09_Innate_Enable_Zone, "SoundProjector:int, Radius=7.0:float, Total_Enemies:int")
{
	bool already_entered, all_checked_in;
	int enemies;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
		enemies = 0;
	}

	/*void Sound_Heard(GameObject* obj, const CombatSound & sound)
	{
		if ( sound.Type == M09_CHECK_IN )
		{
			enemies++;
			if (enemies == Get_Int_Parameter ("Total_Enemies"))
			{
				all_checked_in = true;
			}
		}
	}*/

	void Timer_Expired (GameObject* obj, int timer_id) override
	{
		if (timer_id == INNATE_RECYCLE && !all_checked_in)
		{
			Vector3 projector = ScriptEngine::Get_Position (ScriptEngine::Find_Object (Get_Int_Parameter ("SoundProjector")));
			float rad = Get_Float_Parameter("Radius");

			ScriptEngine::Create_Logical_Sound (obj, M09_INNATE_ENABLE, projector, rad);

			//ScriptEngine::Start_Timer (obj, this, 4.0f, INNATE_RECYCLE);
		}
	}


	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			Vector3 projector = ScriptEngine::Get_Position (ScriptEngine::Find_Object (Get_Int_Parameter ("SoundProjector")));
			float rad = Get_Float_Parameter("Radius");

			ScriptEngine::Create_Logical_Sound (obj, M09_INNATE_ENABLE, projector, rad);

			ScriptEngine::Start_Timer (obj, this, 4.0f, INNATE_RECYCLE);
		}
	}
};

DECLARE_SCRIPT(M09_Zone_Destroy, "")
{
	void Entered (GameObject *obj, GameObject * /* enterer */) override
	{
		ScriptEngine::Destroy_Object(obj);
	}
};

DECLARE_SCRIPT (M09_Lab_Powerup, "Target:int")
{
	void Custom( GameObject * /* obj */, int type, intptr_t /* param */, GameObject * /* sender */ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			ScriptEngine::Grant_Key (ScriptEngine::Find_Object (Get_Int_Parameter("Target")), 10, true);
			ScriptEngine::Debug_Message("Key granted to %d.\n", Get_Int_Parameter("Target"));

			//ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object (Get_Int_Parameter("Target")), 0, 0, 0.0f);
			ScriptEngine::Action_Reset ( ScriptEngine::Find_Object (Get_Int_Parameter("Target")), 100 );
			ScriptEngine::Attach_Script( ScriptEngine::Find_Object (Get_Int_Parameter("Target")), "M03_Goto_Star", "");
		}
	}
};

DECLARE_SCRIPT (M09_Ambient_Clutter, "")
{
	int alarm;
	Vector3 sound_loc;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( alarm, 1 );
		SAVE_VARIABLE( sound_loc, 2 );
	}

	void Created (GameObject *obj) override
	{

		float delayTimer = ScriptEngine::Get_Random ( 1, 5 );
		/*int chooser = ScriptEngine::Get_Random_Int (0, 7);
		float rand_dist = ScriptEngine::Get_Random ( 15, 20 );

		sound_loc = ScriptEngine::Get_Position (STAR);

		sound_loc.X += rand_dist;
		sound_loc.Y += rand_dist;
		sound_loc.Z += rand_dist;

		//ScriptEngine::Create_Sound ( ambients [chooser], sound_loc, obj );*/

		ScriptEngine::Start_Timer(obj, this, (20.0f - delayTimer), AMB_SOUND);
		ScriptEngine::Start_Timer(obj, this, (8.0f + delayTimer), AMB_EXPLOSION);
	}

	void Timer_Expired (GameObject* obj, int timer_id) override
	{
		if (timer_id == AMB_SOUND)
		{
			static constexpr const char *ambients[7] =
				{
					"Ambient_Screams_03",
					"Ambient_Screams_04",
					"Explosion_Glass_02",
					"Explosion_Glass_03",
					"Explosion_Large_01",
					"Vehicle_Explo_Med2",
					"Vehicle_Explo_Med1",
				};

			int chooser = ScriptEngine::Get_Random_Int (0, 7);
			float delayTimer = ScriptEngine::Get_Random ( 1, 5 );
			float rand_dist = ScriptEngine::Get_Random ( 15, 30 );

			sound_loc = ScriptEngine::Get_Position (STAR);

			sound_loc.X += rand_dist;
			sound_loc.Y += rand_dist;
			sound_loc.Z += rand_dist;

			ScriptEngine::Create_Sound ( ambients [chooser], sound_loc, obj );

			ScriptEngine::Start_Timer(obj, this, (20.0f - delayTimer), AMB_SOUND);
		}

		if (timer_id == AMB_EXPLOSION)
		{
			int exp_point [5];

			exp_point [0] = 2006120;
			exp_point [1] = 2006121;
			exp_point [2] = 2006122;
			exp_point [3] = 2006123;
			exp_point [4] = 2006124;

			int exp_num = ScriptEngine::Get_Random_Int (0, 5);
			ScriptEngine::Create_Explosion("Air Explosions Twiddler", ScriptEngine::Get_Position (ScriptEngine::Find_Object (exp_point [exp_num])), nullptr);

			float delayTimer = ScriptEngine::Get_Random ( 1, 5 );
			ScriptEngine::Start_Timer(obj, this, (8.0f + delayTimer), AMB_EXPLOSION);
		}
	}
};

DECLARE_SCRIPT (M09_Scientist_Cower, "")
{
	void Created (GameObject *obj) override
	{
		float delay = ScriptEngine::Get_Random ( 1, 3 );
		ScriptEngine::Start_Timer(obj, this, (delay), ANIM_DELAY);
	}

	void Timer_Expired (GameObject* obj, int timer_id) override
	{
		if (timer_id == ANIM_DELAY)
		{
			ActionParamsStruct params;
			params.Set_Basic( this, 99, DOING_ANIMATION );
			params.Set_Animation ("S_A_HUMAN.H_A_A0A0_L51", true);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}
};

DECLARE_SCRIPT (M09_Destroy_Self_Zone, "")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		GameObject *mobius = ScriptEngine::Find_Object (2000010);

		if (!already_entered)
		{
			already_entered = true;

			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), NO_FOLLOW, ON, 0.0f);

			const char *conv_name = ("IDS_M09_D13");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(nullptr, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 900);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}
};

DECLARE_SCRIPT (M09_KeyCard_Zone, "")
{
	bool initiation;
	GameObject *mobius;
	float mobius_distance;
	bool star_in_zone;
	bool fired_off;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( initiation, 1 );
		SAVE_VARIABLE( mobius_distance, 2 );
		SAVE_VARIABLE( star_in_zone, 3 );
		SAVE_VARIABLE( fired_off, 4 );
	}

	void Created (GameObject * /* obj */) override
	{
		star_in_zone = false;
		fired_off = false;

		initiation = false;
		mobius = ScriptEngine::Find_Object (2000010);
	}

	void Timer_Expired(GameObject * obj, int timer_id) override
	{
		if (timer_id == DIST_CHECK && star_in_zone)
		{
			if (mobius_distance <= 15.0/* && !fired_off*/)
			{
				//fired_off = true;

				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), CHECK, 0, 0.0f );
			}

			else
			{
				ScriptEngine::Start_Timer (obj, this, 2.0f, DIST_CHECK);
			}
		}
	}


	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		star_in_zone = true;

		mobius = ScriptEngine::Find_Object (2000010);
		mobius_distance = ScriptEngine::Get_Distance(ScriptEngine::Get_Position (STAR), ScriptEngine::Get_Position (mobius));

		if (mobius_distance <= 15.0/* && !fired_off*/)
		{
			//fired_off = true;

			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), CHECK, 0, 0.0f );
		}

		else
		{
			ScriptEngine::Start_Timer (obj, this, 2.0f, DIST_CHECK);
		}
	}

	void Exited( GameObject * /* obj */, GameObject * /* exiter */ ) override
	{
		star_in_zone = false;
	}

	void Custom( GameObject * obj, int type, intptr_t param, GameObject * /* sender */ ) override
	{
		if (type == INITIATION)
		{
			//initiation = true;
		}

		if ( type == COUNT )
		{
			if (param == 0)
			{
				//initiation = true;

				/*const char *conv_name = ("IDS_M09_D13");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
				ScriptEngine::Join_Conversation(nullptr, conv_id, false, false, true);
				ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
				ScriptEngine::Start_Conversation (conv_id, 900);
				ScriptEngine::Monitor_Conversation (obj, conv_id);*/
			}

			if (param == 1)
			{
				//initiation = true;

				const char *conv_name = ("IDS_M09_D15");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
				ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
				ScriptEngine::Start_Conversation (conv_id, 905);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}

			if (param == 2)
			{
				ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000452), KEY_COUNTER, INCREMENT, 0.0f );

				ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1265140, 0, "lab_spn_door08.lab_spn_door08" );

				const char *conv_name = ("IDS_M09_D18");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
				ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
				ScriptEngine::Start_Conversation (conv_id, 910);
				ScriptEngine::Monitor_Conversation (obj, conv_id);


			}
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason /* reason */) override
	{
		mobius = ScriptEngine::Find_Object (2000010);

		if(action_id == 900)
		{
			const char *conv_name = ("IDS_M09_D13");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 950);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), FOLLOW, 0, 0.0f );
		}



		if(action_id == 906)
		{
			const char *conv_name = ("IDS_M09_D13");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 908);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), FOLLOW, 0, 0.0f );
		}

		if(action_id == 907)
		{
			const char *conv_name = ("IDS_M09_D15");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 909);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			//ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), FOLLOW, 0, 0.0f );
		}

		if(action_id == 908)
		{
			const char *conv_name = ("IDS_M09_D16");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 99, 200, false);
			ScriptEngine::Join_Conversation(mobius, conv_id, false, false, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, false, true);
			ScriptEngine::Start_Conversation (conv_id, 910);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}

		/*if(action_id == 910)
		{
			ScriptEngine::Send_Custom_Event( obj, ScriptEngine::Find_Object (2000010), WAYPATH, 2000462, 0.0f );

		}*/

	}
};

DECLARE_SCRIPT (M09_Innate_Activate, "Target0=0:int, Target1=0:int, Target2=0:int, Target3=0:int, Target4=0:int, Target5=0:int, Target6=0:int, Target7=0:int, Target8=0:int, Target9=0:int")
{
	int target [10];
	bool already_entered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
		SAVE_VARIABLE( target [0], 2 );
		SAVE_VARIABLE( target [1], 3 );
		SAVE_VARIABLE( target [2], 4 );
		SAVE_VARIABLE( target [3], 5 );
		SAVE_VARIABLE( target [4], 6 );
		SAVE_VARIABLE( target [5], 7 );
		SAVE_VARIABLE( target [6], 8 );
		SAVE_VARIABLE( target [7], 9 );
		SAVE_VARIABLE( target [8], 10 );
		SAVE_VARIABLE( target [9], 11 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

		target [0] = Get_Int_Parameter ("Target0");
		target [1] = Get_Int_Parameter ("Target1");
		target [2] = Get_Int_Parameter ("Target2");
		target [3] = Get_Int_Parameter ("Target3");
		target [4] = Get_Int_Parameter ("Target4");
		target [5] = Get_Int_Parameter ("Target5");
		target [6] = Get_Int_Parameter ("Target6");
		target [7] = Get_Int_Parameter ("Target7");
		target [8] = Get_Int_Parameter ("Target8");
		target [9] = Get_Int_Parameter ("Target9");
	}

	void Entered (GameObject *obj, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			for (int x = 0; x < ARRAY_SIZE(target); x++)
			{
				if (target [x] != 0)
				{
					ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(target [x]), M09_INNATE_ENABLE, 0, 0.0f);
				}
			}
		}
	}
};

DECLARE_SCRIPT (M09_Camera_Activate, "Camera0=0:int, Camera1=0:int, Camera2=0:int, Camera3=0:int, Camera4=0:int")
{
	int camera [5];
	bool already_entered;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
		SAVE_VARIABLE( camera [0], 2 );
		SAVE_VARIABLE( camera [1], 3 );
		SAVE_VARIABLE( camera [2], 4 );
		SAVE_VARIABLE( camera [3], 5 );
		SAVE_VARIABLE( camera [4], 6 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;

		camera [0] = Get_Int_Parameter ("Camera0");
		camera [1] = Get_Int_Parameter ("Camera1");
		camera [2] = Get_Int_Parameter ("Camera2");
		camera [3] = Get_Int_Parameter ("Camera3");
		camera [4] = Get_Int_Parameter ("Camera4");
	}

	void Entered (GameObject * /* obj */, GameObject * /* enterer */) override
	{
		if (!already_entered)
		{
			already_entered = true;

			for (int x = 0; x < ARRAY_SIZE(camera); x++)
			{
				if (camera [x] != 0)
				{
					ScriptEngine::Attach_Script (ScriptEngine::Find_Object (camera [x]), "RMV_Camera_Behavior", "90.0, 0, 1, 0.0");
				}
			}
		}
	}
};

DECLARE_SCRIPT(M09_Level10Key, "")
{
	void Entered (GameObject * /* obj */, GameObject * /* enterer */) override
	{
		ScriptEngine::Grant_Key ( STAR, 10, true);
	}
};

DECLARE_SCRIPT(M09_Destroy_Zone, "")
{
	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		ScriptEngine::Start_Timer (obj, this, 2.0f, 10);
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		if(timer_id == 10)
		{
			ScriptEngine::Destroy_Object (obj);
		}
	}
};

DECLARE_SCRIPT(M09_Elevator_Exit, "Mobius_exit_goto:int")
{
	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );
	}

	void Created (GameObject * /* obj */) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		//already_entered = true;

		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), ELEVATOR_EXIT, Get_Int_Parameter("Mobius_exit_goto"), 0.0f);
	}
};

DECLARE_SCRIPT(M09_Attack_Blocked_False, "")
{
	void Enemy_Seen(GameObject * obj, GameObject *enemy ) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN, 1 );
		params.Set_Attack (enemy, 200.0f, 5.0f, 1);
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Attack (obj, params);
	}
};

DECLARE_SCRIPT(M09_Mobius_OnFollow, "")
{
	void Entered (GameObject * obj, GameObject * /* enterer */) override
	{
		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object (2000010), NO_FOLLOW, OFF, 0.0f);
	}
};

DECLARE_SCRIPT(M09_PSuitAnim, "")
{
	void Created (GameObject *obj) override
	{
		/*ActionParamsStruct params;
		params.Set_Basic( this, 99, DOING_ANIMATION );
		params.Set_Animation ("S_A_HUMAN.H_A_X9C_SUIT", true);
		ScriptEngine::Action_Play_Animation (obj, params);*/

		ScriptEngine::Set_Animation ( obj, "S_A_HUMAN.H_A_X9C_SUIT", true, nullptr, 0.0f, -1.0f, false );
	}

	void Custom (GameObject *obj, int type, intptr_t /* param */, GameObject * /* sender */) override
	{
		if (type == 666)
		{
			ScriptEngine::Destroy_Object (obj);
		}
	}
};
