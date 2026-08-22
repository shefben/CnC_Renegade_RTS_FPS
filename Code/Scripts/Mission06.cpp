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
*     Mission06.cpp
*
* DESCRIPTION
*     Mission 6 script
*
* PROGRAMMER
*     David Shuman
*
* VERSION INFO
*     $Author: Darren_k $
*     $Revision: 136 $
*     $Modtime: 2/05/02 4:18p $
*     $Archive: /Commando/Code/Scripts/Mission06.cpp $
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"
#include "Mission6.h"
#include <string.h>
#include <stdio.h>



// ************************* M06B
// Objective Controller
DECLARE_SCRIPT(M06_Objective_Controller, "") // 100018
{
	int mendoza_id;
	bool accomplished_609;

	enum {HAVOCS_SCRIPT, M06_INITIAL_APACHES};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( mendoza_id, 1 );
		SAVE_VARIABLE( accomplished_609, 2 );
	}

	void Created(GameObject * obj) override
	{
		// Background Music
		ScriptEngine::Set_Background_Music ("06-onyourfeet.mp3");
		// Scale their sight and hearing back
	//	ScriptEngine::Scale_AI_Awareness(.5f, .5f );

		ScriptEngine::Start_Timer (obj, this, 1.0f, HAVOCS_SCRIPT);
		ScriptEngine::Enable_Hibernation(obj, false);
		mendoza_id = 0;
		accomplished_609 = false;

		// EVA - Give me a position on the scientists.\n
		const char *conv_name = ("M06_CON059");
		int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
		ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
		ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
		ScriptEngine::Start_Conversation (conv_id, 300601);
		ScriptEngine::Monitor_Conversation (obj, conv_id);

		Add_An_Objective(605);
		Add_An_Objective(607);
		Add_An_Objective(608);
//		Add_An_Objective(609);
//		Add_An_Objective(610);

		ScriptEngine::Start_Timer (obj, this, 6.0f, M06_INITIAL_APACHES);
	}

	void Add_An_Objective(int id)
	{
		GameObject *object;

		switch (id)
		{
		// Hack War Room Computer
		case 601:
			{
				ScriptEngine::Add_Objective(601, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M06_02, nullptr, IDS_Enc_Obj_Primary_M06_02);
				object = ScriptEngine::Find_Object(106952);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(601, object);
					ScriptEngine::Set_Objective_HUD_Info_Position(601, 90.0f, "POG_M06_1_03.tga", IDS_POG_HACK, ScriptEngine::Get_Position (object));
				}
			}
			break;
		// Rescue Scientists
		case 603:
			{
				ScriptEngine::Add_Objective(603, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M06_01, nullptr, IDS_Enc_Obj_Primary_M06_01);
				object = ScriptEngine::Find_Object(101010);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(603, object);
					ScriptEngine::Set_Objective_HUD_Info_Position(603, 90.0f, "POG_M06_1_04.tga", IDS_POG_RESCUE, ScriptEngine::Get_Position (object));
				}
			}
			break;
		// Eliminate Mendoza
		case 604:
			{
				ScriptEngine::Add_Objective(604, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M06_04, nullptr, IDS_Enc_Obj_Primary_M06_04);
			//	object = ScriptEngine::Find_Object(mendoza_id);
				object = ScriptEngine::Find_Object(108275);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(604, object);
					ScriptEngine::Set_Objective_HUD_Info_Position(604, 90.0f, "POG_M06_1_01.tga", IDS_POG_ELIMINATE, ScriptEngine::Get_Position (object));
				}
			}
			break;
		// Rescue Resistance
		case 605:
			{
				ScriptEngine::Add_Objective(605, OBJECTIVE_TYPE_TERTIARY, OBJECTIVE_STATUS_HIDDEN, IDS_Enc_ObjTitle_Hidden_M06_02, nullptr, IDS_Enc_Obj_Hidden_M06_02);
				object = ScriptEngine::Find_Object(101016);

			}
			break;
		// Release GDI prisoner
		case 607:
			{
				ScriptEngine::Add_Objective(607, OBJECTIVE_TYPE_TERTIARY, OBJECTIVE_STATUS_HIDDEN, IDS_Enc_ObjTitle_Hidden_M06_03, nullptr, IDS_Enc_Obj_Hidden_M06_03);
			}
			break;
		// Plunder Raveshaw's hidden cache
		case 608:
			{
				ScriptEngine::Add_Objective(608, OBJECTIVE_TYPE_TERTIARY, OBJECTIVE_STATUS_HIDDEN, IDS_Enc_ObjTitle_Hidden_M06_04, nullptr, IDS_Enc_Obj_Hidden_M06_04);
			}
			break;
		// Deactivate alarm system
		case 609:
			{
				ScriptEngine::Add_Objective(609, OBJECTIVE_TYPE_SECONDARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Hidden_M06_01, nullptr, IDS_Enc_Obj_Hidden_M06_01);
				Vector3 star_pos = ScriptEngine::Get_Position(STAR);
				Vector3 greenhouse_alarm = ScriptEngine::Get_Position(ScriptEngine::Find_Object(101548));
				Vector3 shower_alarm = ScriptEngine::Get_Position(ScriptEngine::Find_Object(101549));
				float star_greenhouse = ScriptEngine::Get_Distance(star_pos, greenhouse_alarm);
				float star_shower = ScriptEngine::Get_Distance(star_pos, shower_alarm);
				if(star_greenhouse == (WWMath::Min(star_greenhouse, star_shower)))
				{
					object = ScriptEngine::Find_Object(101548);
					if(object)
					{
						ScriptEngine::Set_Objective_Radar_Blip_Object(609, object);
						ScriptEngine::Set_Objective_HUD_Info_Position(609, 90.0f, "POG_M08_2_02.tga", IDS_POG_DISABLE, ScriptEngine::Get_Position (object));
					}
				}
				else
				{
					object = ScriptEngine::Find_Object(101549);
					if(object)
					{
						ScriptEngine::Set_Objective_Radar_Blip_Object(609, object);
						ScriptEngine::Set_Objective_HUD_Info_Position(609, 90.0f, "POG_M08_2_02.tga", IDS_POG_DISABLE, ScriptEngine::Get_Position (object));
					}
				}



			}
			break;
		// Alarm State: Stand Down
		case 610:
			{
				ScriptEngine::Add_Objective(610, OBJECTIVE_TYPE_SECONDARY, OBJECTIVE_STATUS_HIDDEN, 1000, nullptr, 0);
			}
			break;
		// Escape Chateau with Dr. Sydney Mobius
		case 611:
			{
				ScriptEngine::Set_HUD_Help_Text ( IDS_M11DSGN_DSGN1007I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
				ScriptEngine::Add_Objective(611, OBJECTIVE_TYPE_PRIMARY, OBJECTIVE_STATUS_PENDING, IDS_Enc_ObjTitle_Primary_M06_03, nullptr, IDS_Enc_Obj_Primary_M06_03);
				object = ScriptEngine::Find_Object(101527);
				if(object)
				{
					ScriptEngine::Set_Objective_Radar_Blip_Object(611, object);
					ScriptEngine::Set_Objective_HUD_Info_Position(611, 90.0f, "POG_M06_1_05.tga", IDS_POG_ESCORT, ScriptEngine::Get_Position (object));
				}
			}
			break;
		}
	}

	void Custom(GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if(type == M06_MENDOZA_ID)
		{
			mendoza_id = param;

		}
		if(type == 666 && param == 666)
		{

			ScriptEngine::Start_Timer (obj, this, 2.0f, M06_DEAD_HAVOC);
		}
		if(type > 611)
		{
			return;
		}
		switch (param)
		{
			case 1:
				{
					if(type == 609 && accomplished_609)
					{
						return;
					}
					ScriptEngine::Clear_Radar_Marker (type);
					ScriptEngine::Set_Objective_Status(type, OBJECTIVE_STATUS_ACCOMPLISHED);
					if(type == 604)
					{
						ScriptEngine::Mission_Complete ( true );
					}
					if(type == 609)
					{
						accomplished_609 = true;
					}


				}
				break;
			case 2:
				{
					ScriptEngine::Clear_Radar_Marker (type);
					ScriptEngine::Set_Objective_Status(type, OBJECTIVE_STATUS_FAILED);
					if(type == 611)
					{
						ScriptEngine::Mission_Complete ( false );
					}
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

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if(action_id == 300601 && (reason == ACTION_COMPLETE_CONVERSATION_ENDED || reason == ACTION_COMPLETE_CONVERSATION_INTERRUPTED))
		{
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 601, 3, 0.0f);
		}
	}

	void Timer_Expired(GameObject * /*obj*/, int timer_id ) override
	{

		if(timer_id == HAVOCS_SCRIPT)
		{
	//		ScriptEngine::Attach_Script(STAR, "M06_Havoc_DLS", "");

		}
		if(timer_id == M06_DEAD_HAVOC)
		{

	//		ScriptEngine::Attach_Script(STAR, "M06_Havoc_DLS", "");

		}

		if(timer_id == M06_INITIAL_APACHES)
		{

			// Initial Apaches
			GameObject *controller1 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
			ScriptEngine::Attach_Script(controller1, "Test_Cinematic", "X6A_Apache_01.txt");

			GameObject *controller2 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
			ScriptEngine::Attach_Script(controller2, "Test_Cinematic", "X6A_Apache_02.txt");

		}
	}

};

// WarRoom Computer
DECLARE_SCRIPT(M06_WarRoom_Computer, "")
{
	bool already_poked;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_poked, 1 );

	}

	void Created (GameObject * obj) override
	{
		already_poked = false;
		ScriptEngine::Enable_HUD_Pokable_Indicator( obj, true );
	}

	void Poked(GameObject * obj, GameObject * poker) override
	{
		if (ScriptEngine::Is_A_Star(poker) && (!already_poked))
		{
			// Data decryption complete. Internal computer systems indicate Dr. Ignatio Mobius is located beneath the Chateau. Access is gained through the dining hall.\n
			const char *conv_name = ("M06_CON001");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
			ScriptEngine::Join_Conversation(nullptr, conv_id, true, true, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 300603);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			already_poked = true;
			ScriptEngine::Enable_HUD_Pokable_Indicator( obj, false );

			ScriptEngine::Create_Object ("Level_03_Keycard", Vector3 (35.263F, 16.432F, 13.382F));

		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == 300603 && (reason == ACTION_COMPLETE_CONVERSATION_ENDED || reason == ACTION_COMPLETE_CONVERSATION_INTERRUPTED))
		{
			// New Primary Mission Objective: Evacuate Doctors' Ignatio and Sydney Mobius.  Basement, NE section.  Requires level 2 security keycard.
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 601, 1, 0.0f);
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 603, 3, 1.0f);

		}
	}

};

#define	SYDNEY_MOVE_TABLE_SIZE  ( sizeof(Sydney_Move_Table) / sizeof (Sydney_Move_Table[0]) )
const int	Sydney_Move_Table[] =
{
	101567,
	101569,
	101571,
	101573,
	101575,
	101577,
	101579,
	101581,
	101582,
	101584,
	101586,
	101587,
	101588
};

DECLARE_SCRIPT(M06_Sydney_Mobius, "")
{
	int poke_id;
	bool dont_move;
	int current_move_loc;

	enum {CHECK_DISTANCE = 20, SYDNEY_MOVE_LOC, MOVE_LOC_LOW_PRIORITY};
	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poke_id, 1 );
		SAVE_VARIABLE( dont_move, 2 );
		SAVE_VARIABLE( current_move_loc, 3 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		ScriptEngine::Grant_Key (obj, 3, true);
		poke_id = 4;
		dont_move = true;

		current_move_loc = 0;

	}

	void Poked(GameObject * obj, GameObject * /*poker*/) override
	{
		ActionParamsStruct params;

		switch(poke_id)
		{
		case 1:
			{
				// \\Hold here. I'll be back.\n
				const char *conv_name = ("M06_CON002");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

				ScriptEngine::Set_Innate_Is_Stationary(obj, true);

				dont_move = true;
				poke_id = 2;
			}
			break;
		case 2:
			{
				// \\Let's keep moving.\n
				const char *conv_name = ("M06_CON003");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

				ScriptEngine::Set_Innate_Is_Stationary(obj, false);

				params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), SYDNEY_MOVE_LOC);
				params.Set_Movement (ScriptEngine::Find_Object(Sydney_Move_Table[current_move_loc]), 1.0f, 3.0f);
				ScriptEngine::Action_Goto (obj, params);

				poke_id = 1;
				dont_move = false;
			}
			break;
		}

	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		ActionParamsStruct params;

		if(type == M06_RELOCATE)
		{
			if(param == 0)
			{
				ScriptEngine::Set_Position (obj, ScriptEngine::Get_Position(ScriptEngine::Find_Object(107606)));
				// Relocate Havoc
				ScriptEngine::Send_Custom_Event(obj, STAR, M06_RELOCATE, 0, 0.0f);
				// Activate MidtroC zone
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(108285), M06_CUSTOM_ACTIVATE, 1, 0.0f);
				// Move Sydney to initial move location
				ScriptEngine::Start_Timer(obj, this, 4.0f, MOVE_LOC_LOW_PRIORITY);
				dont_move = false;
				// Start the destruction stub sequence
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 100, 100, 2.0f);
				// Accomplish mission objective to rescue scientists
				ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 603, 1, 1.0f);
				// Mission objective to escort Sydney out of Chateau
				ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 611, 3, 2.0f);
				// Clost and lock door to lab
				// Door to the lab
				ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1300001615, 0, "DOOR_0.DOOR_0");

			}
			if(param == 1)
			{
				ScriptEngine::Set_Position (obj, ScriptEngine::Get_Position(ScriptEngine::Find_Object(108277)));
				// Relocate Havoc
				ScriptEngine::Send_Custom_Event(obj, STAR, M06_RELOCATE, 1, 0.0f);
				// Accomplish Mission Objective for Sydney Escort out of Chateau
				ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 611, 1, 1.0f);
				// Relocate Mendoza
				//ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(108275), M06_RELOCATE, 0, 0.0f);

				//
				//	Play the boss music
				//
				// ScriptEngine::Set_Background_Music ("Mendoza_Battle1.mp3");

				//
				//	Create and position Mendoza
				//
				GameObject *mendoza = ScriptEngine::Create_Object ("Mendoza Boss", Vector3 (-40.572F, 28.942F, 1.605F));
				if (mendoza != nullptr) {
					ScriptEngine::Set_Facing (mendoza, -50.0F);
				}

		//		GameObject *mendoza = ScriptEngine::Create_Object ("Nod_FlameThrower_3Boss", Vector3 (-40.572F, 28.942F, 1.605F));
		//		if (mendoza != nullptr) {
		//			ScriptEngine::Set_Facing (mendoza, -50.0F);
		//			ScriptEngine::Attach_Script(mendoza, "M06_Mendoza", "");
		//		}

				// Mission objective to defeat Mendoza
				ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 604, 3, 0.0f);
				// Make Sydney indestructible
				ScriptEngine::Set_Shield_Strength (obj, 100 );
				ScriptEngine::Set_Shield_Type (obj, "Blamo");

				//
				//  Create logical sound to remove all other players in fight with Mendoza
				//
				ScriptEngine::Create_Logical_Sound(obj, M06_CLEAR_FOR_MENDOZA, ScriptEngine::Get_Position(obj), 2500.0f);

			}



		}
		if(type == M06_MOVE_SYDNEY && !dont_move)
		{
			current_move_loc = param;
			if(current_move_loc == 12)
			{
				poke_id = 4;
			}

			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), SYDNEY_MOVE_LOC);
			params.Set_Movement (ScriptEngine::Find_Object(Sydney_Move_Table[current_move_loc]), 1.0f, 3.0f);
			ScriptEngine::Action_Goto (obj, params);
		}

	}

	void Damaged( GameObject * /*obj*/, GameObject * /*damager*/, float /*amount*/ ) override
	{
	//	ScriptEngine::Set_Health(obj, ScriptEngine::Get_Max_Health(obj));
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;


		if(action_id == SYDNEY_MOVE_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 4.0f);
		}
		if((action_id == SYDNEY_MOVE_LOC) && (action_id != ScriptEngine::Get_Action_ID(obj)) && (!ScriptEngine::Is_Performing_Pathfind_Action(obj)) && (reason == ACTION_COMPLETE_LOW_PRIORITY))
		{
			ScriptEngine::Start_Timer(obj, this, 4.0f, MOVE_LOC_LOW_PRIORITY);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == MOVE_LOC_LOW_PRIORITY)
		{
			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), SYDNEY_MOVE_LOC);
			params.Set_Movement (ScriptEngine::Find_Object(Sydney_Move_Table[current_move_loc]), 1.0f, 3.0f);
			ScriptEngine::Action_Goto (obj, params);

		}
		if(timer_id == CHECK_DISTANCE)
		{
			ScriptEngine::Start_Timer (obj, this, 10.0f, CHECK_DISTANCE);
		}

		if(timer_id == CHECK_DISTANCE && poke_id == 2)
		{
			Vector3 star_pos = ScriptEngine::Get_Position(STAR);
			Vector3 sydney_pos = ScriptEngine::Get_Position(obj);
			float distance_to_star = ScriptEngine::Get_Distance(star_pos, sydney_pos);
			if(distance_to_star > 15.0f)
			{
				// Don't leave me here!\n
				const char *conv_name = ("M06_CON005");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}

		}
		else if(timer_id == CHECK_DISTANCE && poke_id == 1)
		{
			Vector3 star_pos = ScriptEngine::Get_Position(STAR);
			Vector3 sydney_pos = ScriptEngine::Get_Position(obj);
			float distance_to_star = ScriptEngine::Get_Distance(star_pos, sydney_pos);
			if(distance_to_star > 15.0f)
			{
				// Wait for me!\n
				const char *conv_name = ("M06_CON006");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

			}

		}

	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Set_HUD_Help_Text ( IDS_M11DSGN_DSGN1008I1DSGN_TXT, TEXT_COLOR_OBJECTIVE_PRIMARY );
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 611, 2, 0.0f);

	}

};

DECLARE_SCRIPT(M06_MidtroB_Explosion_Controller, "Loc0_ID=0:int, Loc1_ID=0:int, Loc2_ID=0:int, Loc3_ID=0:int, Loc4_ID=0:int")
{
	void Custom (GameObject * /*obj*/, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if(type == M06_MIDTRO_EXPLOSION)
		{
			int explode_loc[5];
			explode_loc[0] = Get_Int_Parameter("Loc0_ID");
			explode_loc[1] = Get_Int_Parameter("Loc1_ID");
			explode_loc[2] = Get_Int_Parameter("Loc2_ID");
			explode_loc[3] = Get_Int_Parameter("Loc3_ID");
			explode_loc[4] = Get_Int_Parameter("Loc4_ID");
			ScriptEngine::Create_Explosion("Chateau_Explosions_Twiddler", ScriptEngine::Get_Position(ScriptEngine::Find_Object(explode_loc[param])), nullptr);
		}
	}
};


DECLARE_SCRIPT(M06_Activate_Midtro, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
		// Door to the lab
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1300001615, 0, "DOOR_0.DOOR_0");
		// Raveshaw/Mendoza door
		ScriptEngine::Static_Anim_Phys_Goto_Frame ( 1553454, 0, "DOOR_0.DOOR_0");
	}

	void Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			// Open door to lab
			ScriptEngine::Static_Anim_Phys_Goto_Last_Frame ( 1300001615, "DOOR_0.DOOR_0" );
			// Open Raveshaw/Mendoza Door
			ScriptEngine::Static_Anim_Phys_Goto_Last_Frame ( 1553454, "DOOR_0.DOOR_0" );
			already_entered = true;
	//		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101010), 100, 100, 0.0f);

			Vector3 havoc_loc = ScriptEngine::Get_Position (STAR);
			havoc_loc.Z += 7.0f;
			ScriptEngine::Set_Position (STAR, havoc_loc );

			GameObject * cinematic_obj = ScriptEngine::Create_Object ( "Invisible_Object", Vector3(0.0f, 0.0f, 0.0f));
			ScriptEngine::Attach_Script(cinematic_obj, "Test_Cinematic", "X6B_MIDTRO.txt");

			// Grant armor increase medal
			ScriptEngine::Create_Object ( "POW_Medal_Health", Vector3(-1.472f, 31.216f, -9.000f));
		}
	}
};

DECLARE_SCRIPT(M06_Destruction_Stub, "")
{

	enum {SHAKE_TIMER};


	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		if(type == 100 && param == 100)
		{
			// Activate destruction zones
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(104312), M06_CHATEAU_COLLAPSE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(105635), M06_CHATEAU_COLLAPSE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(105639), M06_CHATEAU_COLLAPSE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(105640), M06_CHATEAU_COLLAPSE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(105641), M06_CHATEAU_COLLAPSE, 1, 0.0f);

			// Create destruction path blocks
			GameObject *crate1 = ScriptEngine::Create_Object("X6_Rubble1", Vector3(23.453f, 16.340f, -2.000f));
			ScriptEngine::Set_Facing( crate1, -65.000f );

			GameObject *crate2 = ScriptEngine::Create_Object("X6_Rubble2", Vector3(27.086f, 28.569f, -1.454f));
			ScriptEngine::Set_Facing( crate2, 0.000f );

			GameObject *crate3 = ScriptEngine::Create_Object("X6_Rubble2", Vector3(25.614f, 33.324f, -0.043f));
			ScriptEngine::Set_Facing( crate3, 125.000f );

			GameObject *crate4 = ScriptEngine::Create_Object("X6_Rubble3", Vector3(14.597f, -30.510f, -0.033f));
			ScriptEngine::Set_Facing( crate4, 140.000f );

			GameObject *crate5 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(25.079f, 28.012f, -2.007f));
			ScriptEngine::Set_Facing( crate5, -60.000f );

			GameObject *crate6 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(26.463f, 27.382f, -2.003f));
			ScriptEngine::Set_Facing( crate6, 0.000f );

			GameObject *crate7 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(38.426f, 2.889f, 2.574f));
			ScriptEngine::Set_Facing( crate7, -35.000f );

			GameObject *crate8 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(40.258f, 3.442f, 2.798f));
			ScriptEngine::Set_Facing( crate8, -180.000f );

			GameObject *crate9 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(9.832f, -35.126f, -0.014f));
			ScriptEngine::Set_Facing( crate9, 5.000f );

			GameObject *crate10 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(4.503f, -33.694f, 3.003f));
			ScriptEngine::Set_Facing( crate10, 150.000f );

			GameObject *crate11 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(1.179f, -33.281f, -0.042f));
			ScriptEngine::Set_Facing( crate11, -160.000f );

			GameObject *crate12 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(-5.442f, -28.292f, 0.003f));
			ScriptEngine::Set_Facing( crate12, -15.000f );

			GameObject *crate13 = ScriptEngine::Create_Object("X6_Rubble4", Vector3(-5.423f, -12.960f, 0.000f));
			ScriptEngine::Set_Facing( crate13, 110.000f );

			GameObject *crate14 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(8.042f, 27.962f, -0.150f));
			ScriptEngine::Set_Facing( crate14, 150.000f );

			GameObject *crate15 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(9.271f, 28.300f, -0.001f));
			ScriptEngine::Set_Facing( crate15, -5.000f );

			GameObject *crate16 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(7.208f, 30.813f, 4.400f));
			ScriptEngine::Set_Facing( crate16, 142.684f );

			GameObject *crate17 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(5.628f, 29.865f, -0.025f));
			ScriptEngine::Set_Facing( crate17, 115.000f );

			GameObject *crate18 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(38.716f, 4.235f, 2.374f));
			ScriptEngine::Set_Facing( crate18, 50.000f );

			GameObject *crate19 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(13.124f, -30.323f, 0.000f));
			ScriptEngine::Set_Facing( crate19, 5.000f );

			GameObject *crate20 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(8.932f, -34.790f, -0.000f));
			ScriptEngine::Set_Facing( crate20, -0.000f );

			GameObject *crate21 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(6.464f, -33.466f, 2.978f));
			ScriptEngine::Set_Facing( crate21, 0.000f );

			GameObject *crate22 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(2.144f, -33.484f, 0.007f));
			ScriptEngine::Set_Facing( crate22, 0.000f );

			GameObject *crate23 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(-5.275f, -27.057f, 0.000f));
			ScriptEngine::Set_Facing( crate23, 70.000f );

			GameObject *crate24 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(-5.932f, -11.340f, 0.000f));
			ScriptEngine::Set_Facing( crate24, 25.000f );

			GameObject *crate25 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(-4.760f, -1.653f, 0.000f));
			ScriptEngine::Set_Facing( crate25, 110.000f );

			GameObject *crate26 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(-5.332f, -2.713f, 0.277f));
			ScriptEngine::Set_Facing( crate26, 0.000f );

			GameObject *crate27 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(5.699f, -32.438f, 2.563f));
			ScriptEngine::Set_Facing( crate27, -120.000f );



			float shake_timer = ScriptEngine::Get_Random(0.0f, 3.0f);
			ScriptEngine::Start_Timer (obj, this, shake_timer, SHAKE_TIMER);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		// Shake the Chateau to simulate collapse
		if(timer_id == SHAKE_TIMER)
		{



			// Explosions near Star to simulate collapse
		//	Vector3 star_pos = ScriptEngine::Get_Position(STAR);

		//	Vector3 explode_loc;
		//	explode_loc.X = star_pos.X + ScriptEngine::Get_Random(5.0f, 10.0f);
		//	explode_loc.Y = star_pos.Y + ScriptEngine::Get_Random(5.0f, 10.0f);
		//	explode_loc.Z = star_pos.Z; //+ ScriptEngine::Get_Random(5.0f, 10.0f);

			float distance = ScriptEngine::Get_Random(-12.0f, 12.0f);
			distance *= 3;
			Vector3 pos = ScriptEngine::Get_Position(STAR);
			float facing = ScriptEngine::Get_Facing(STAR);
			float a = WWMath::Cos(DEG_TO_RADF(facing)) * distance;
			float b = WWMath::Sin(DEG_TO_RADF(facing)) * distance;
			Vector3 explode_loc = pos + Vector3(a, b, 0.0f);

			ScriptEngine::Create_Explosion("Chateau_Explosions_Twiddler", explode_loc, obj);

			float shake_timer = ScriptEngine::Get_Random(1.0f, 5.0f);
			ScriptEngine::Start_Timer (obj, this, ScriptEngine::Get_Random(2.0f, 8.0f), SHAKE_TIMER);
			// Shake the Chateau to simulate collapse
			float camera_intensity = ScriptEngine::Get_Random(.01f,.09f);
			float camera_duration = shake_timer - 1.0f;
			ScriptEngine::Shake_Camera(Vector3 (-12.0f, 40.0f, -9.0f), 1000.0f, camera_intensity, camera_duration);
		}

	}
};

DECLARE_SCRIPT(M06_Mendoza, "")
{

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 604, 1, 0.0f);

	}

};

DECLARE_SCRIPT(M06_GDI_Prisoner, "")
{

	bool conversation;
	bool poked;

	enum{ESCAPE_CHATEAU};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( conversation, 1 );
		SAVE_VARIABLE( poked, 2 );
	}

	void Created (GameObject * obj) override
	{
		conversation = false;
		poked = false;
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		ScriptEngine::Set_Player_Type(obj, SCRIPT_PLAYERTYPE_NEUTRAL );
		ScriptEngine::Enable_HUD_Pokable_Indicator( obj, true );
	}


	void Poked(GameObject * obj, GameObject * poker) override
	{
		if ((ScriptEngine::Is_A_Star(poker)) && (!conversation) && (!poked))
		{
			// Thank for the release. I've got a Nod officer I've got to hunt down.\n
			const char *conv_name = ("M06_CON008");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
			ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
			ScriptEngine::Join_Conversation(poker, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 300607);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			conversation = true;
			poked = true;
			ScriptEngine::Enable_HUD_Pokable_Indicator( obj, false );
			ScriptEngine::Grant_Key (STAR, 3, true);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;


		if(action_id == 300607 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			conversation = false;
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 607, 1, 0.5f);
			ScriptEngine::Set_Innate_Is_Stationary(obj, false);
			ScriptEngine::Set_Player_Type(obj, SCRIPT_PLAYERTYPE_GDI );
			ScriptEngine::Give_PowerUp(obj, "POW_Chaingun_AI", false);
			ScriptEngine::Select_Weapon(obj, "Weapon_Chaingun_Ai" );

			ScriptEngine::Grant_Key (obj, 1, true);
			ScriptEngine::Grant_Key (obj, 2, true);
			ScriptEngine::Grant_Key (obj, 3, true);

			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), 1);
			params.Set_Movement (ScriptEngine::Find_Object(104967), RUN, 3.0f);
			ScriptEngine::Action_Goto (obj, params);

			ScriptEngine::Start_Timer (obj, this, 10.0f, ESCAPE_CHATEAU);


		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == ESCAPE_CHATEAU)
		{
			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), 1);
			params.Set_Movement (ScriptEngine::Find_Object(104967), RUN, 3.0f);
			ScriptEngine::Action_Goto (obj, params);

		}

	}
};

DECLARE_SCRIPT(M06_Activate_Secret_Door, "Secret_Door_ID=0:int")
{
	bool already_poked;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_poked, 1 );

	}


	void Created(GameObject * /*obj*/) override
	{
		already_poked = false;
	}

	void Poked(GameObject * obj, GameObject * poker) override
	{
		if (ScriptEngine::Is_A_Star(poker) && (!already_poked))
		{
			already_poked = true;



			// Looks like Raveshaw's private stash. Time for a five-finger discount.\n
			const char *conv_name = ("M06_CON061");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN + 5, 0, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 300608);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == 300608 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Static_Anim_Phys_Goto_Last_Frame ( Get_Int_Parameter("Secret_Door_ID"), "BK.BK" );
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 608, 1, 0.0f);
		}
	}
};

DECLARE_SCRIPT(M06_Civ_Prisoner, "")
{

	int poke_id;
	bool conversation;

	enum{WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poke_id, 1 );
		SAVE_VARIABLE( conversation, 2 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Set_Player_Type(obj, SCRIPT_PLAYERTYPE_NEUTRAL );
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		poke_id = 1;
		conversation = false;
		ScriptEngine::Enable_HUD_Pokable_Indicator( obj, true );

	}



	void Poked(GameObject * obj, GameObject * /*poker*/) override
	{
		if(conversation)
		{
			return;
		}

		ActionParamsStruct params;
		switch(poke_id)
		{
		case 1:
			{
				conversation = true;
				// I am in your debt.\n
				const char *conv_name = ("M06_CON009");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN + 5, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 300123);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

				Vector3 pos = ScriptEngine::Get_Position(obj);
				float facing = ScriptEngine::Get_Facing(obj);
				float a = WWMath::Cos(DEG_TO_RADF(facing)) * 1.5f;
				float b = WWMath::Sin(DEG_TO_RADF(facing)) * 1.5f;
				Vector3 powerup_loc = pos + Vector3(a, b, 0.5f);
				ScriptEngine::Create_Object("POW_Health_100", powerup_loc);
				ScriptEngine::Enable_HUD_Pokable_Indicator( obj, false );
			}
			break;
		case 2:
			{
				conversation = true;
				// I must rest.
				const char *conv_name = ("M06_CON063");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN + 5, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 300124);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
			break;

		}
	}


	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(reason == ACTION_COMPLETE_CONVERSATION_INTERRUPTED)
		{
			conversation = false;
		}
		if(action_id == 300123 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 605, 1, 0.0f);

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), RUN, 1.5f );
			params.WaypathID = 101552;
			ScriptEngine::Action_Goto( obj, params );


			ScriptEngine::Set_Player_Type(obj, SCRIPT_PLAYERTYPE_GDI );
			ScriptEngine::Set_Innate_Is_Stationary(obj, false);

		}

		if(action_id == WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 12 );
			params.Set_Animation ("S_A_HUMAN.h_a_b0c0", true);
			ScriptEngine::Action_Play_Animation (obj, params);
			poke_id = 2;
			conversation = false;

		}

	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
//		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 605, 2, 0.0f);
	}

};

DECLARE_SCRIPT(M06_Enable_Guard_Tower, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Warning: Nod Security Measures Detected.\n
	//		const char *conv_name = ("M06_CON010");
	//		int conv_id = ScriptEngine::Create_Conversation (conv_name, 100.0f, 200.0f, false);
	//		ScriptEngine::Join_Conversation(nullptr, conv_id, true, true, true);
	//		ScriptEngine::Join_Conversation(STAR, conv_id, true, true, true);
	//		ScriptEngine::Start_Conversation (conv_id, 300606);
	//		ScriptEngine::Monitor_Conversation (obj, conv_id);

			// Patrols
			ScriptEngine::Enable_Spawner(101046, true);
			ScriptEngine::Enable_Spawner(101053, true);
			// Engineer
			ScriptEngine::Enable_Spawner(101054, true);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == 300606 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 610, 1, 0.0f);
		}
	}
};

DECLARE_SCRIPT(M06_Gate_Guards, "Soldier_ID=0:int")
{

	int soldier_id;
	bool disable_tower_spawn;

	enum {M06_GATE_GUARD_WAYPATH = 105, WAYPATH, CONVERSATION};
	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( disable_tower_spawn, 1 );
	}

	void Created (GameObject * obj) override
	{
		soldier_id = Get_Int_Parameter("Soldier_ID");
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard( obj, false );
		disable_tower_spawn = false;
		ScriptEngine::Start_Timer(obj, this, 15.0f, CONVERSATION);

		ScriptEngine::Send_Custom_Event (obj, obj, M06_GATE_GUARD_WAYPATH, 1, 0.0f);
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(110908), M06_GATE_GUARD_WAYPATH, 1, 0.0f);

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;
		ScriptEngine::Debug_Message("Action_Complete on Gate Guard.  action_id %d reason %d.\n", action_id, reason);
		if(action_id == 1109081 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{

		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
	}

	void Custom(GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		ActionParamsStruct params;


		if(type == M06_GATE_GUARD_WAYPATH)
		{


			ScriptEngine::Set_Innate_Is_Stationary(obj, false);

			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = 101023;
			params.WaypointStartID = 101024;
			params.WaypointEndID = 101027;
			params.WaypathSplined = true;
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Sound_Heard(GameObject* /*obj*/, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_DISABLE_TOWER_SPAWN )
		{
			disable_tower_spawn = true;
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == WAYPATH)
		{
			ScriptEngine::Set_Innate_Is_Stationary(obj, false);

			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = 101023;
			params.WaypointStartID = 101024;
			params.WaypointEndID = 101027;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(timer_id == CONVERSATION)
		{
			switch(soldier_id)
			{
			case 1:
				{
					if(ScriptEngine::Find_Object(101021))
					{
						// The new security protocols are tight.\n
						const char *conv_name = ("M06_CON011");
						int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN - 5, 15.0f, false);
						ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
						ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(110908), conv_id, false, true, true);
						ScriptEngine::Start_Conversation (conv_id, 1109081);
						ScriptEngine::Monitor_Conversation (obj, conv_id);
					}
				}
				break;
			}
		}

	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if(ScriptEngine::Find_Object(101037) && (!disable_tower_spawn))  // Officer in tower
		{
		/*	const char * conv_name[6];
			conv_name[0] = ("M06_CON012");  // This is Tower, What's your situation brother?\n
			conv_name[1] = ("M06_CON013");  // This is Tower, please respond.\n
			conv_name[2] = ("M06_CON014");  // This is Tower, I'm sending a recon to your position.\n
			conv_name[3] = ("M06_CON015");  // This is Tower. Hold position brother, reinforcements enroute.\n
			conv_name[4] = ("M06_CON016");  // This is Tower, report in.\n
			conv_name[5] = ("M06_CON017");  // This is Tower. We've a possible man down, Bravo patrol inspect.\n

			int random = Get_Int_Random(0, 5);

			int conv_id = ScriptEngine::Create_Conversation (conv_name[random], 0, 0, true);
			ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 1);*/

			switch(soldier_id)
			{
			case 1:
				{
	//				ScriptEngine::Enable_Spawner(101035, true);
				}
				break;
			case 2:
				{
	//				ScriptEngine::Enable_Spawner(101036, true);
				}
				break;
			}
		}
	}


};

DECLARE_SCRIPT(M06_Escort_Tank, "")
{
	bool attacking;

	enum {ARRIVE_COURTYARD, START_ESCORT};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( attacking, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, 1.5f, START_ESCORT);
//		ScriptEngine::Enable_Enemy_Seen(obj, true);
		ScriptEngine::Enable_Hibernation(obj, false);
		attacking = false;
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{

		if(timer_id == START_ESCORT)
		{
			ActionParamsStruct params;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, ARRIVE_COURTYARD );
			params.Set_Movement( Vector3(0,0,0), 0.8f, 1.5f );
			params.WaypathID = 102252;
	//		params.WaypathSplined = true;
			ScriptEngine::Action_Goto( obj, params );
		}

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		ScriptEngine::Debug_Message("Action Complete on Escort Tank.  action_id %d reason %d.\n", action_id, reason);
		if(action_id == ARRIVE_COURTYARD && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Enable_Engine(obj, false);
			ScriptEngine::Enable_Hibernation(obj, true);
		}

	}


	void Enemy_Seen(GameObject * obj, GameObject * /*enemy*/ ) override
	{
		if(!attacking)
		{
			ScriptEngine::Enable_Hibernation(obj, true);
			ScriptEngine::Enable_Engine(obj, true);
			attacking = true;
			ScriptEngine::Attach_Script(obj, "M08_Mobile_Vehicle", "111422, 111423, 111424, 111425, 111426, 111427, 111428, 111429, 111430, 111431");
		}
	}

	void Damaged( GameObject * obj, GameObject * damager, float /*amount*/ ) override
	{
		ActionParamsStruct params;

		if(damager == STAR)
		{
			if(!attacking)
			{
				ScriptEngine::Enable_Hibernation(obj, true);
				ScriptEngine::Enable_Engine(obj, true);
				attacking = true;
				ScriptEngine::Attach_Script(obj, "M08_Mobile_Vehicle", "111422, 111423, 111424, 111425, 111426, 111427, 111428, 111429, 111430, 111431");
			}
		}
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{

	}

};

DECLARE_SCRIPT(M06_Alarm_Controller, "")
{
	bool alarm_sounded;
	bool triggered;
	bool stand_down;
	bool alarmed;

	enum {STAND_DOWN};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( alarm_sounded, 1 );
		SAVE_VARIABLE( triggered, 2 );
		SAVE_VARIABLE( stand_down, 3 );
		SAVE_VARIABLE( alarmed, 4 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		alarm_sounded = false;
		triggered = false;
		stand_down = false;
		alarmed = false;
		ScriptEngine::Set_Animation_Frame(obj, "mct_nod.mct_nod", 0);
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		Vector3 sender_pos = ScriptEngine::Get_Position(sender);
		ScriptEngine::Debug_Message("Custom received Alarm_Controller.  type %d param %d sender_id %d sender_pos X %d, Y %d, Z %d.\n", type, param, ScriptEngine::Get_ID(sender), sender_pos.X, sender_pos.Y, sender_pos.Z );

		ActionParamsStruct params;

		if(type == M06_ALARMED)  // Send custom to new spawns status of alarm
		{
			ScriptEngine::Send_Custom_Event (obj, sender, M06_ALARMED, alarm_sounded, 0.0f);
		}
		if(type == M06_STAND_DOWN) // && (alarm_sounded) && !stand_down)
		{
			stand_down = true;
			ScriptEngine::Start_Timer (obj, this, 0.0f, STAND_DOWN);
		}
		if(type == M06_SOUND_ALARM && (!alarm_sounded))
		{
			alarm_sounded = true;
			ScriptEngine::Create_Sound ( "M06_Alarm", Vector3 (0,0,0), obj);
			ScriptEngine::Create_Logical_Sound(obj, M06_SOUND_ALARM_ON, Vector3 (0,0,0), 2500.0f);
	//		ScriptEngine::Monitor_Sound (obj, alarm_sound);
		}

		if((type == M00_CUSTOM_CAMERA_ALARM) && (!alarm_sounded))
		{
			alarm_sounded = true;
			ScriptEngine::Create_Sound ( "M06_Alarm", Vector3 (0,0,0), obj);
	//		ScriptEngine::Monitor_Sound (obj, alarm_sound);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if(action_id == 300609 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			alarmed = true;
			// Deactivate alarm system
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 609, 3, 0.0f);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{

		if(timer_id == STAND_DOWN) // && (alarm_sounded) && (alarmed))
		{
			if(alarm_sounded)
			{
				// Stand down general alarm. Intruder alert canceled.\n
				const char *conv_name = ("M06_CON018");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Get_A_Star(Vector3(0.0f,0.0f,0.0f)), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 300000);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}

			// Stand down spawn units
			ScriptEngine::Create_Logical_Sound(obj, M06_SOUND_ALARM_OFF, Vector3 (0,0,0), 2500.0f);

			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 609, 1, 0.0f);
			alarm_sounded = true;
//			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 610, 1, 0.0f);

		}


	}
};

DECLARE_SCRIPT(M06_Alarm_Behavior, "Alarm_Enemy_Seen=0.0:float, Alarm_Damaged=0.0:float")
{
	int alarm_switch_id[8];
	int alarm_switch_amnt;
	int closest_id;
	bool sound_alarm;
	bool alarmed;
	float alarm_enemy_seen;
	float alarm_damaged;
	bool hear_footsteps;
	int havoc_id;
	bool attacking_havoc;


	enum {HEAR_FOOTSTEPS};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( closest_id, 1 );
		SAVE_VARIABLE( sound_alarm, 2 );
		SAVE_VARIABLE( alarmed, 3 );
		SAVE_VARIABLE( hear_footsteps, 4 );
		SAVE_VARIABLE( havoc_id, 5 );
		SAVE_VARIABLE( attacking_havoc, 6 );
	}


	void Created(GameObject * obj) override
	{

		// No starting units can hear footsteps until otherwise alerted
		ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard(obj, false);
		hear_footsteps = false;

		alarm_enemy_seen = Get_Float_Parameter("Alarm_Enemy_Seen");
		alarm_damaged = Get_Float_Parameter("Alarm_Damaged");
		int difficulty = ScriptEngine::Get_Difficulty_Level();
		alarm_enemy_seen *= difficulty + 1;
		alarm_damaged *= difficulty + 1;
		closest_id = 0;
		alarm_switch_amnt = 8;
		sound_alarm = false;
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), M06_ALARMED, 1, 0.2f);
		alarmed = false;
		havoc_id = 0;
		attacking_havoc = false;

		// Hardcode all alarm switch IDs
		alarm_switch_id[0] = 100157;
		alarm_switch_id[1] = 100330;
		alarm_switch_id[2] = 100423;
		alarm_switch_id[3] = 100445;
		alarm_switch_id[4] = 101003;
		alarm_switch_id[5] = 101004;
		alarm_switch_id[6] = 101005;
		alarm_switch_id[7] = 101006;
	}

	int Nearest (GameObject * obj)  // Get nearest alarm switch to soldier
	{
		Vector3 alarm_pos;
		Vector3 soldier_pos = ScriptEngine::Get_Position(obj);
		float this_distance = 0.0f;
		float closest_distance = 0.0f;

		int i = 0;
		while (i < alarm_switch_amnt)
		{


			if(ScriptEngine::Find_Object(alarm_switch_id[i]))
			{
				alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(alarm_switch_id[i]));

				this_distance = ScriptEngine::Get_Distance(soldier_pos, alarm_pos);


				if(closest_distance == 0.0f)
				{
					closest_distance = this_distance;

				}
				if(WWMath::Min(this_distance, closest_distance) == this_distance)
				{
					closest_distance = this_distance;
					closest_id = alarm_switch_id[i];

				}
			}
			i++;
		}

		return closest_id;
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;
		if(action_id == 10 && reason == ACTION_COMPLETE_NORMAL && !alarmed)
		{
			Vector3 alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(closest_id));
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 11);
			params.Set_Face_Location( alarm_pos, 1.5f);
			ScriptEngine::Action_Face_Location ( obj, params );

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(closest_id), M06_CHECK_ALARM, ScriptEngine::Get_ID(obj), 0.0f);

		}

	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		ActionParamsStruct params;


		if(type == M06_ALARMED)  // Custom returned from alarm controller for new spawns to set alarmed status
		{
			alarmed = (param == 1) ? true : false;
		}
		if(type == M06_CHECK_ALARM && !alarmed)
		{

			if(param == 1)
			{
				// The alarm switch is destroyed, we need engineering!\n
				const char *conv_name = ("M06_CON019");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
				ScriptEngine::Monitor_Conversation (obj, conv_id);

				int i = 0;
				int curr_alarm_switch_id = ScriptEngine::Get_ID(sender);
				while (i < alarm_switch_amnt)
				{
					if(curr_alarm_switch_id == alarm_switch_id[i])
					{
						alarm_switch_id[i] = 0;
					}
					i++;
				}
				// Go to another alarm box
				closest_id = Nearest(obj);

				Vector3 alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(closest_id));
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
				params.Set_Movement( alarm_pos, RUN, 1.5f );
				ScriptEngine::Action_Goto( obj, params );
			}
			else
			{
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 12 );
				params.Set_Animation ("S_A_HUMAN.H_A_A0F0", false);
				ScriptEngine::Action_Play_Animation (obj, params);

				// Send custom to sound alarm
				ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), M06_SOUND_ALARM, 1, 0.0f);
				// Once alarm has been sounded

			}
		}

		if(type == M06_YOU_ATTACK_HAVOC)
		{
			ScriptEngine::Debug_Message("Soldier got M06_YOU_ATTACK_HAVOC custom, attacking STAR.\n");
			attacking_havoc = true;

			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN + 5), 1);
			params.Set_Attack (STAR, 50.0f, 0.0f, 1);
			ScriptEngine::Action_Attack (obj, params);
		}


	}

	void Sound_Heard(GameObject* obj, const CombatSound & sound) override
	{


		if ( static_cast<int>(sound.Type) == M06_SOUND_ALARM_ON )
		{
			alarmed = true;
			sound_alarm = false;

			Vector3 pos = ScriptEngine::Get_Position(obj);
			ScriptEngine::Innate_Force_State_Gunshots_Heard(obj, pos);


		}
		if ( static_cast<int>(sound.Type) == M06_SOUND_ALARM_OFF )
		{
			alarmed = false;
			attacking_havoc = false;
			ScriptEngine::Action_Reset ( obj, 100 );
		}
		if ( static_cast<int>(sound.Type) == M06_ATTACK_HAVOC_SOUND )
		{

			havoc_id = ScriptEngine::Get_ID(sound.Creator);
			// Let Havoc know this soldier is coming to attack
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(havoc_id), M06_ATTACK_HAVOC, 1, 0.0f);

		}

		if (((static_cast<int>(sound.Type) == SOUND_TYPE_GUNSHOT) || (static_cast<int>(sound.Type) == SOUND_TYPE_BULLET_HIT)) && (!hear_footsteps))
		{
			hear_footsteps = true;
			ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard(obj, true);
			ScriptEngine::Start_Timer (obj, this, 60.0f, HEAR_FOOTSTEPS);
		}

	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{

		if(timer_id == HEAR_FOOTSTEPS)
		{
			hear_footsteps = false;
			ScriptEngine::Innate_Soldier_Enable_Footsteps_Heard(obj, false);

		}

	}

	void Damaged (GameObject * obj, GameObject * /*damager*/, float /*amount*/) override
	{
		ActionParamsStruct params;

		if(ScriptEngine::Get_Random(0.0f, 1.0f) <= alarm_damaged)
		{
			if((!sound_alarm) && (!alarmed) && (obj))
			{
				sound_alarm = true;

				// Sound the alarm!\n
				const char *conv_name = ("M06_CON020");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);

				closest_id = Nearest(obj);

				Vector3 alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(closest_id));
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
				params.Set_Movement( alarm_pos, RUN, 1.5f );
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject * /*enemy*/ ) override
	{
		ActionParamsStruct params;

		if(ScriptEngine::Get_Random(0.0f, 1.0f) <= alarm_enemy_seen)
		{
			if((!sound_alarm) && (!alarmed))
			{
				sound_alarm = true;

				// Sound the alarm!\n
				const char *conv_name = ("M06_CON020");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);

				closest_id = Nearest(obj);

				Vector3 alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(closest_id));
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10 );
				params.Set_Movement( alarm_pos, RUN, 1.5f );
				ScriptEngine::Action_Goto( obj, params );
			}
		}
	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		if(attacking_havoc)
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(havoc_id), M06_DEAD_HAVOC_ATTACKER, 1, 0.0f);
		}
	}
};

DECLARE_SCRIPT(M06_Alarm_Switch, "")
{
	bool destroyed;

	enum{DAMAGE_ALARM};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( destroyed, 1 );
	}


	void Created(GameObject * obj) override
	{
		Vector3 alarm_pos = ScriptEngine::Get_Position(obj);
		ScriptEngine::Debug_Message("Position of alarm switch %d: X: %d, Y: %d, Z:%d", ScriptEngine::Get_ID(obj), alarm_pos.X, alarm_pos.Y, alarm_pos.Z );
		ScriptEngine::Set_Animation_Frame(obj, "dsp_doorswitch2.dsp_doorswitch2", 0);
		destroyed = false;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if(type == M06_CHECK_ALARM)
		{
			ScriptEngine::Send_Custom_Event (obj, sender, M06_CHECK_ALARM, destroyed, 0.0f);
		}


	}

	void Damaged (GameObject * obj, GameObject * /*damager*/, float /*amount*/) override
	{
		if(ScriptEngine::Get_Health(obj) > 1.0f)
		{
			ScriptEngine::Set_Animation_Frame(obj, "dsp_doorswitch2.dsp_doorswitch2", 0);
		}
		else
		{
			ScriptEngine::Set_Health(obj, 1.0f);
			if(!destroyed)
			{
				destroyed = true;
				ScriptEngine::Set_Animation_Frame(obj, "dsp_doorswitch2.dsp_doorswitch2", 60);

			}
			else
			{
				ScriptEngine::Set_Animation_Frame(obj, "dsp_doorswitch2.dsp_doorswitch2", 60);
			}
		}

	}

};

DECLARE_SCRIPT(M06_Alarm_Terminal_DLS, "")
{
	bool poked;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poked, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Animation_Frame(obj, "mct_nod.mct_nod", 0);
		ScriptEngine::Enable_HUD_Pokable_Indicator( obj, true );
		poked = false;
	}

	void Poked(GameObject * obj, GameObject * poker) override
	{
		if (ScriptEngine::Is_A_Star(poker))
		{
			poked = true;
			ScriptEngine::Enable_HUD_Pokable_Indicator( obj, false );
			// Custom to alarm controller to stand down
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), M06_STAND_DOWN, 1, 0.0f);
		}
	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		if(!poked)
		{
			// Custom to alarm controller to stand down
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), M06_STAND_DOWN, 1, 0.0f);
		}

		Vector3 myPositon = ScriptEngine::Get_Position ( obj );
		float myFacing  = ScriptEngine::Get_Facing ( obj );

		GameObject * destroyedMCT = ScriptEngine::Create_Object ( "Nod_MCT_Alarm_Destroyed", myPositon);
		ScriptEngine::Set_Facing ( destroyedMCT, myFacing );
	}

};

DECLARE_SCRIPT(M06_Tower_Patrol, "Waypath_ID=0:int, Waypath_Loc:Vector3")
{
	int waypath_id;
	Vector3 waypath_loc;
	bool disable_tower_spawn;
	bool chateau_destruction;

	enum {WAYPATH, GO_WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( waypath_id, 1 );
//		SAVE_VARIABLE( waypath_loc, 2 );
		SAVE_VARIABLE( disable_tower_spawn, 1 );
		SAVE_VARIABLE( chateau_destruction, 2 );
	}

	void Created (GameObject * obj) override
	{
		waypath_id = Get_Int_Parameter("Waypath_ID");
		waypath_loc = Get_Vector3_Parameter("Waypath_Loc");

		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
		params.Set_Movement( waypath_loc, RUN, 1.5f );
		ScriptEngine::Action_Goto( obj, params );

		disable_tower_spawn = false;
		chateau_destruction = false;

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = waypath_id;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == WAYPATH)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
			params.Set_Movement( waypath_loc, WALK, 1.5f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Sound_Heard(GameObject* /*obj*/, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_DISABLE_TOWER_SPAWN )
		{
			disable_tower_spawn = true;
		}
		if ( static_cast<int>(sound.Type) == M06_CHATEAU_DESTRUCTION )
		{
			chateau_destruction = true;
		}
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if(ScriptEngine::Find_Object(101037) && (!disable_tower_spawn) && (!chateau_destruction))  // Officer in tower
		{
			const char * conv_name[6];
			conv_name[0] = ("M06_CON012");  // This is Tower, What's your situation brother?\n
			conv_name[1] = ("M06_CON013");  // This is Tower, please respond.\n
			conv_name[2] = ("M06_CON014");  // This is Tower, I'm sending a recon to your position.\n
			conv_name[3] = ("M06_CON015");  // This is Tower. Hold position brother, reinforcements enroute.\n
			conv_name[4] = ("M06_CON016");  // This is Tower, report in.\n
			conv_name[5] = ("M06_CON017");  // This is Tower. We've a possible man down, Bravo patrol inspect.\n

			int random = Get_Int_Random(0, 10);

			if(random < 6)
			{
				int conv_id = ScriptEngine::Create_Conversation (conv_name[random], 0, 0, true);
				ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
			}
		}
	}

};

DECLARE_SCRIPT(M06_Courtyard_Patrol, "Waypath_ID=0:int, Waypath_Loc:Vector3")
{
	int waypath_id;
	Vector3 waypath_loc;
	bool disable_courtyard_spawn;
	bool chateau_destruction;


	enum {WAYPATH, GO_WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( waypath_id, 1 );
//		SAVE_VARIABLE( waypath_loc, 2 );
		SAVE_VARIABLE( disable_courtyard_spawn, 1 );
		SAVE_VARIABLE( chateau_destruction, 2 );
	}

	void Created (GameObject * obj) override
	{
		waypath_id = Get_Int_Parameter("Waypath_ID");
		waypath_loc = Get_Vector3_Parameter("Waypath_Loc");

		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
		params.Set_Movement( waypath_loc, RUN, 1.5f );
		ScriptEngine::Action_Goto( obj, params );

		disable_courtyard_spawn = false;
		chateau_destruction = false;

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = waypath_id;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == WAYPATH)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
			params.Set_Movement( waypath_loc, WALK, 1.5f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Sound_Heard(GameObject* /*obj*/, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_DISABLE_COURTYARD_SPAWN )
		{
			disable_courtyard_spawn = true;
		}
		if ( static_cast<int>(sound.Type) == M06_CHATEAU_DESTRUCTION )
		{
			chateau_destruction = true;
		}
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if((!disable_courtyard_spawn) && (!chateau_destruction))
		{
			const char * conv_name[5];
			conv_name[0] = ("M06_CON021");  // This is Courtyard, report in.\n
			conv_name[1] = ("M06_CON022");  // Courtyard here. Squad enroute.\n
			conv_name[2] = ("M06_CON023");  // This is Courtyard, get me a response team.\n
			conv_name[3] = ("M06_CON024");  // Courtyard here. What's the situation brothers?\n
			conv_name[4] = ("M06_CON025");  // Courtyard here. Intruder response to your location.\n

			int random = Get_Int_Random(0, 10);
			if(random < 5)
			{
				int conv_id = ScriptEngine::Create_Conversation (conv_name[random], 0, 0, true);
				ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
			}
		}
	}

};

DECLARE_SCRIPT(M06_Hedgemaze_Patrol, "Waypath_ID=0:int, Waypath_Loc:Vector3")
{
	int waypath_id;
	Vector3 waypath_loc;
	bool disable_hedgemaze_spawn;
	bool chateau_destruction;


	enum {WAYPATH, GO_WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( waypath_id, 1 );
//		SAVE_VARIABLE( waypath_loc, 2 );
		SAVE_VARIABLE( disable_hedgemaze_spawn, 1 );
		SAVE_VARIABLE( chateau_destruction, 2 );
	}

	void Created (GameObject * obj) override
	{
		waypath_id = Get_Int_Parameter("Waypath_ID");
		waypath_loc = Get_Vector3_Parameter("Waypath_Loc");

		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
		params.Set_Movement( waypath_loc, RUN, 1.5f );
		ScriptEngine::Action_Goto( obj, params );

		disable_hedgemaze_spawn = false;
		chateau_destruction = false;

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = waypath_id;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == WAYPATH)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
			params.Set_Movement( waypath_loc, WALK, 1.5f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Sound_Heard(GameObject* /*obj*/, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_DISABLE_HEDGEMAZE_SPAWN )
		{
			disable_hedgemaze_spawn = true;
		}
		if ( static_cast<int>(sound.Type) == M06_CHATEAU_DESTRUCTION )
		{
			chateau_destruction = true;
		}
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if((!disable_hedgemaze_spawn) && (!chateau_destruction))
		{
			const char * conv_name[4];
			conv_name[0] = ("M06_CON026");  // This is Hedge Maze, Please respond.\n
			conv_name[1] = ("M06_CON027");  // This is Hedge Maze. Get a patrol to that position.\n
			conv_name[2] = ("M06_CON028");  // Hedge Maze here, report in please.\n
			conv_name[3] = ("M06_CON029");  // Hedge Maze here, please respond.\n

			int random = Get_Int_Random(0, 10);
			if(random < 4)
			{
				int conv_id = ScriptEngine::Create_Conversation (conv_name[random], 0, 0, true);
				ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
			}
		}
	}

};

DECLARE_SCRIPT(M06_Interior_Patrol, "Waypath_ID=0:int, Waypath_Loc:Vector3")
{
	int waypath_id;
	Vector3 waypath_loc;


	enum {WAYPATH, GO_WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( waypath_id, 1 );
		SAVE_VARIABLE( waypath_loc, 2 );
	}

	void Created (GameObject * obj) override
	{
		waypath_id = Get_Int_Parameter("Waypath_ID");
		waypath_loc = Get_Vector3_Parameter("Waypath_Loc");

		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
		params.Set_Movement( waypath_loc, RUN, 1.5f );
		ScriptEngine::Action_Goto( obj, params );

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = waypath_id;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == WAYPATH)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
			params.Set_Movement( waypath_loc, WALK, 1.5f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101309), M06_DEAD_INTERIOR_PATROL, 1, 0.0f);
	}

};

DECLARE_SCRIPT(M06_Lab_Patrol, "Waypath_ID=0:int, Waypath_Loc:Vector3")
{
	int waypath_id;
	Vector3 waypath_loc;


	enum {WAYPATH, GO_WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( waypath_id, 1 );
//		SAVE_VARIABLE( waypath_loc, 2 );
	}

	void Created (GameObject * obj) override
	{
		waypath_id = Get_Int_Parameter("Waypath_ID");
		waypath_loc = Get_Vector3_Parameter("Waypath_Loc");

		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
		params.Set_Movement( waypath_loc, WALK, 1.5f );
		ScriptEngine::Action_Goto( obj, params );

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = waypath_id;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(action_id == GO_WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer(obj, this, 60.0f, WAYPATH);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == WAYPATH)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, GO_WAYPATH );
			params.Set_Movement( waypath_loc, WALK, 1.5f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

};

DECLARE_SCRIPT(M06_Barracks_Patrol, "")
{
	bool disable_barracks_spawn;
	int barracks_drop[5];
	int barracks_pickup[6];
	bool chateau_destruction;

	enum {WAYPATH, PICKUP, DROP};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( disable_barracks_spawn, 1 );
		SAVE_VARIABLE( chateau_destruction, 2 );
	}


	void Created (GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);

		ActionParamsStruct params;

		barracks_drop[0] = 101534;
		barracks_drop[1] = 101535;
		barracks_drop[2] = 101536;
		barracks_drop[3] = 101537;
		barracks_drop[4] = 101538;

		barracks_pickup[0] = 101539;
		barracks_pickup[1] = 101540;
		barracks_pickup[2] = 101541;
		barracks_pickup[3] = 101542;
		barracks_pickup[4] = 101543;
		barracks_pickup[5] = 101544;

		int random = Get_Int_Random(0, 5);
		params.Set_Basic (this, (INNATE_PRIORITY_GUNSHOT_HEARD - 5), PICKUP);
		params.Set_Movement (ScriptEngine::Find_Object(barracks_pickup[random]), WALK, 1.5f);
		ScriptEngine::Action_Goto (obj, params);

		disable_barracks_spawn = false;
		chateau_destruction = false;
	}

	void Sound_Heard(GameObject* /*obj*/, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_DISABLE_BARRACKS_SPAWN )
		{
			disable_barracks_spawn = true;
		}
		if ( static_cast<int>(sound.Type) == M06_CHATEAU_DESTRUCTION )
		{
			chateau_destruction = true;
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == PICKUP && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Enable_Hibernation(obj, true);

			int random = Get_Int_Random(0, 4);
			params.Set_Basic (this, (INNATE_PRIORITY_GUNSHOT_HEARD - 5), DROP);
			params.Set_Movement (ScriptEngine::Find_Object(barracks_drop[random]), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}
		if(action_id == PICKUP && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer (obj, this, 20.0f, PICKUP);
		}
		if(action_id == DROP && reason == ACTION_COMPLETE_NORMAL)
		{
			Vector3 pos = ScriptEngine::Get_Position(obj);
			float facing = ScriptEngine::Get_Facing(obj);
			float a = WWMath::Cos(DEG_TO_RADF(facing)) * 1.5f;
			float b = WWMath::Sin(DEG_TO_RADF(facing)) * 1.5f;
			Vector3 powerup_loc = pos + Vector3(a, b, 0.5f);
			ScriptEngine::Create_Object("M06_Barracks_Powerups_Twiddler", powerup_loc);

			int random = Get_Int_Random(0, 5);
			params.Set_Basic (this, (INNATE_PRIORITY_GUNSHOT_HEARD - 5), PICKUP);
			params.Set_Movement (ScriptEngine::Find_Object(barracks_pickup[random]), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}
		if(action_id == DROP && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Start_Timer (obj, this, 20.0f, PICKUP);
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Destroy_Object(obj);
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == PICKUP)
		{
			int random = Get_Int_Random(0, 5);
			params.Set_Basic (this, (INNATE_PRIORITY_GUNSHOT_HEARD - 5), PICKUP);
			params.Set_Movement (ScriptEngine::Find_Object(barracks_pickup[random]), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}

	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		if((!disable_barracks_spawn) && (!chateau_destruction))
		{
			const char * conv_name[5];
			conv_name[0] = ("M06_CON030"); // This is Barracks, what's going on?\n
			conv_name[1] = ("M06_CON031"); // Barracks here, what's your status?\n
			conv_name[2] = ("M06_CON032"); // Barracks here. Alpha team to last known location.\n
			conv_name[3] = ("M06_CON033"); // Barracks here, report in brothers.\n
			conv_name[4] = ("M06_CON034"); // This is Barracks. You there, please respond.\n

			int random = Get_Int_Random(0, 10);

			if(random < 5)
			{
				int conv_id = ScriptEngine::Create_Conversation (conv_name[random], 0, 0, true);
				ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 1);
			}
		}
	}

};

DECLARE_SCRIPT(M06_Lab_Guard, "")
{

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
	}

	void Enemy_Seen(GameObject * obj, GameObject * /*enemy*/ ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, false);
	}
};


DECLARE_SCRIPT(M06_Nod_Tower, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( waypath_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
	}

	void Killed (GameObject * /*obj*/, GameObject * /*killer*/) override
	{
		// Disable reinforcement spawners
		ScriptEngine::Enable_Spawner(101035, false);
		ScriptEngine::Enable_Spawner(101036, false);
		ScriptEngine::Enable_Spawner(101046, false);
		ScriptEngine::Enable_Spawner(101053, false);
	}

};

DECLARE_SCRIPT(M06_Courtyard_Controller, "")
{
	int dead_courtyard_eagle;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( dead_courtyard_eagle, 1 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		dead_courtyard_eagle = 0;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		ActionParamsStruct params;
		if(type == M06_DEAD_COURTYARD_EAGLE)
		{
			dead_courtyard_eagle++;
			if(dead_courtyard_eagle == 3)
			{
				ScriptEngine::Enable_Spawner(101063, false);
				ScriptEngine::Enable_Spawner(101064, false);
				ScriptEngine::Enable_Spawner(101073, false);
				ScriptEngine::Enable_Spawner(101082, false);
				ScriptEngine::Enable_Spawner(101095, false);

				ScriptEngine::Create_Logical_Sound(obj, M06_DISABLE_COURTYARD_SPAWN, Vector3 (0,0,0), 2500.0f);

				// Create Search Apche in Courtyard
		//		GameObject * search_apache = ScriptEngine::Create_Object ( "Nod_Apache", Vector3(-99.996f, -135.659f, 37.552f));
		//		ScriptEngine::Attach_Script(search_apache, "M06_Courtyard_Apache", "");
			}


		}

	}



};

DECLARE_SCRIPT(M06_Hedgemaze_Controller, "")
{
	int dead_hedgemaze_eagle;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( dead_hedgemaze_eagle, 1 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		dead_hedgemaze_eagle = 0;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		ActionParamsStruct params;
		if(type == M06_DEAD_HEDGEMAZE_EAGLE)
		{
			dead_hedgemaze_eagle++;
			if(dead_hedgemaze_eagle == 2)
			{
				ScriptEngine::Enable_Spawner(101143, false);
				ScriptEngine::Enable_Spawner(101144, false);
				ScriptEngine::Enable_Spawner(101145, false);
				ScriptEngine::Enable_Spawner(101146, false);

				ScriptEngine::Create_Logical_Sound(obj, M06_DISABLE_HEDGEMAZE_SPAWN, Vector3 (0,0,0), 2500.0f);
			}


		}

	}



};

DECLARE_SCRIPT(M06_Barracks_Controller, "")
{
	int dead_barracks_eagle;
	int evac_soldiers;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( dead_barracks_eagle, 1 );
		SAVE_VARIABLE( evac_soldiers, 2 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		dead_barracks_eagle = 0;
		evac_soldiers = 0;

	//	GameObject * chinook_obj = ScriptEngine::Create_Object ( "Invisible_Object", Vector3(-37.824f, 30.975f, 1.605f));
	//	ScriptEngine::Set_Facing(chinook_obj, 0.000f);
	//	ScriptEngine::Attach_Script(chinook_obj, "Test_Cinematic", "M06_XG_EV4_1.txt");

	//	ScriptEngine::Start_Timer (obj, this, 30.0f, M06_NEXT_EVAC);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if(type == M06_EVAC_ARRIVED)
		{
			evac_soldiers = 0;
			ScriptEngine::Create_Logical_Sound(obj, M06_SOLDIER_TO_EVAC, Vector3 (0,0,0), 2500.0f);
		}
		if(type == M06_TO_EVAC)
		{
			evac_soldiers++;
			if(evac_soldiers < 3)
			{
				ScriptEngine::Send_Custom_Event(obj, sender, M06_TO_EVAC, 1, 0.0f);
			}
		}
		if(type == M06_DEAD_BARRACKS_EAGLE)
		{
			dead_barracks_eagle++;
			if(dead_barracks_eagle == 2)
			{
				ScriptEngine::Enable_Spawner(101531, false);
				ScriptEngine::Enable_Spawner(101532, false);
				ScriptEngine::Enable_Spawner(101533, false);

				ScriptEngine::Create_Logical_Sound(obj, M06_DISABLE_BARRACKS_SPAWN, Vector3 (0,0,0), 2500.0f);
			}


		}

	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{

		if(timer_id == M06_NEXT_EVAC)
		{
			GameObject * chinook_obj = ScriptEngine::Create_Object ( "Invisible_Object", Vector3(-37.824f, 30.975f, 1.605f));
			ScriptEngine::Set_Facing(chinook_obj, 0.000f);
			ScriptEngine::Attach_Script(chinook_obj, "Test_Cinematic", "M06_XG_EV4_1.txt");

			ScriptEngine::Start_Timer (obj, this, 30.0f, M06_NEXT_EVAC);
		}

	}



};

DECLARE_SCRIPT(M06_Interior_Controller, "")
{
	int dead_interior_patrol;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( dead_interior_patrol, 1 );
	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		dead_interior_patrol = 0;
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * sender) override
	{
		ActionParamsStruct params;
		if(type == M06_DEAD_INTERIOR_PATROL)
		{
			Vector3 dead_loc = ScriptEngine::Get_Position(sender);

			dead_interior_patrol++;
			switch(dead_interior_patrol)
			{
			case 4:
				{
					// Contact lost with patrols.  Reserve patrols to your assigned areas.  Chem units to standby.\n
					const char * conv_name = ("M06_CON035");
					int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
					ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
					ScriptEngine::Start_Conversation (conv_id, 1);

					// Disable initial interior spawns
					ScriptEngine::Enable_Spawner(101280, false);
					ScriptEngine::Enable_Spawner(101283, false);
					ScriptEngine::Enable_Spawner(101286, false);
					ScriptEngine::Enable_Spawner(101289, false);
					ScriptEngine::Enable_Spawner(101292, false);
					ScriptEngine::Enable_Spawner(101295, false);
					ScriptEngine::Enable_Spawner(101298, false);
					ScriptEngine::Enable_Spawner(101301, false);

					// Enable grade 2 patrols
					ScriptEngine::Enable_Spawner(101281, true);
					ScriptEngine::Enable_Spawner(101284, true);
					ScriptEngine::Enable_Spawner(101287, true);
					ScriptEngine::Enable_Spawner(101290, true);
					ScriptEngine::Enable_Spawner(101293, true);
					ScriptEngine::Enable_Spawner(101296, true);
					ScriptEngine::Enable_Spawner(101299, true);
					ScriptEngine::Enable_Spawner(101302, true);
				}
				break;
			case 10:
				{
					// Additional patrols lost, brothers.  Chem units to stations.\n
					const char * conv_name = ("M06_CON036");
					int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
					ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
					ScriptEngine::Start_Conversation (conv_id, 1);

					// Enable grade 3 patrols
					ScriptEngine::Enable_Spawner(101282, true);
					ScriptEngine::Enable_Spawner(101285, true);
					ScriptEngine::Enable_Spawner(101288, true);
					ScriptEngine::Enable_Spawner(101291, true);
					ScriptEngine::Enable_Spawner(101294, true);
					ScriptEngine::Enable_Spawner(101297, true);
					ScriptEngine::Enable_Spawner(101300, true);
					ScriptEngine::Enable_Spawner(101303, true);
				}
				break;
			case 26:
				{
					// Disable grade 2 patrols
					ScriptEngine::Enable_Spawner(101281, false);
					ScriptEngine::Enable_Spawner(101284, false);
					ScriptEngine::Enable_Spawner(101287, false);
					ScriptEngine::Enable_Spawner(101290, false);
					ScriptEngine::Enable_Spawner(101293, false);
					ScriptEngine::Enable_Spawner(101296, false);
					ScriptEngine::Enable_Spawner(101299, false);
					ScriptEngine::Enable_Spawner(101302, false);
				}
				break;
			case 42:
				{
					// Disable grade 3 patrols
					ScriptEngine::Enable_Spawner(101282, false);
					ScriptEngine::Enable_Spawner(101285, false);
					ScriptEngine::Enable_Spawner(101288, false);
					ScriptEngine::Enable_Spawner(101291, false);
					ScriptEngine::Enable_Spawner(101294, false);
					ScriptEngine::Enable_Spawner(101297, false);
					ScriptEngine::Enable_Spawner(101300, false);
					ScriptEngine::Enable_Spawner(101303, false);
				}
				break;
			default:
				{

					const char * conv_name;
					conv_name = nullptr;

					if((dead_loc.X < 12.0f) && (dead_loc.Y < -22.0f) && (dead_loc.Z >= 5.0f))
					{
						// Possible disturbance.  Second floor, near main library.\n
						conv_name = ("M06_CON037");
					}
					else if((dead_loc.X < 12.0f) && (dead_loc.Y < -22.0f) && (dead_loc.Z < 5.0f))
					{
						// Response team to bottom floor, main stairwell.\n
						conv_name = ("M06_CON038");
					}
					else if((dead_loc.X < 12.0f) && (dead_loc.Y >= -22.0f) && (dead_loc.Y < 4.0f) && (dead_loc.Z < 5.0f))
					{
						// Nearby patrols, investigate area on bottom floor, main gallery.\n
						conv_name = ("M06_CON039");
					}
					else if((dead_loc.X < 12.0f) && (dead_loc.Y >= -22.0f) && (dead_loc.Y < 4.0f) && (dead_loc.Z >= 5.0f))
					{
						// Fire team to top floor, main gallery.
						conv_name = ("M06_CON040");
					}
					else if((dead_loc.X < 25.0f) && (dead_loc.Y >= 5.0f) && (dead_loc.Z < 5.0f))
					{
						// Possible intruder.  Disturbance bottom floor, open air courtyard.\n
						conv_name = ("M06_CON041");
					}
					else if((dead_loc.X < 25.0f) && (dead_loc.Y >= 5.0f) && (dead_loc.Z >= 5.0f))
					{
						// Lost contact with patrol, second level of open air courtyard.  Investigate.\n
						conv_name = ("M06_CON042");
					}
					else if((dead_loc.X >= 25.0f) && (dead_loc.Y >= 5.0f) && (dead_loc.Z >= 5.0f))
					{
						// Man down.  Team to the top floor of the main dining area.\n
						conv_name = ("M06_CON043");
					}
					else if((dead_loc.X >= 25.0f) && (dead_loc.Y >= 5.0f) && (dead_loc.Z < 5.0f))
					{
						// Patrol to main dining area.  Weapons live, brothers.\n
						conv_name = ("M06_CON044");
					}

					if(conv_name != nullptr)
					{
						int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
						ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
						ScriptEngine::Start_Conversation (conv_id, 1);
					}
				}
				break;
			}

		}

	}



};

DECLARE_SCRIPT(M06_Apply_Damage, "")
{
	enum{APPLY_DAMAGE};

	void Created(GameObject * obj) override
	{
		ScriptEngine::Start_Timer (obj, this, 3.0f, APPLY_DAMAGE);
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == APPLY_DAMAGE)
		{
			ScriptEngine::Apply_Damage( obj, 10000.0f, "STEEL", nullptr);
		}
	}
};

DECLARE_SCRIPT(M06_Enable_Exterior_Courtyard, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Disable reinforcement spawners for tower
			ScriptEngine::Enable_Spawner(101035, false);
			ScriptEngine::Enable_Spawner(101036, false);
			ScriptEngine::Enable_Spawner(101046, false);
			ScriptEngine::Enable_Spawner(101053, false);

			ScriptEngine::Create_Logical_Sound(obj, M06_DISABLE_TOWER_SPAWN, Vector3 (0,0,0), 2500.0f);

			// Enable reinforcements for courtyard
			ScriptEngine::Enable_Spawner(101063, true);
			ScriptEngine::Enable_Spawner(101064, true);
			ScriptEngine::Enable_Spawner(101073, true);
			ScriptEngine::Enable_Spawner(101082, true);
			ScriptEngine::Enable_Spawner(101095, true);

		}
	}


};

DECLARE_SCRIPT(M06_Enable_Hedgemaze, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Enable reinforcements for hedgemaze
			ScriptEngine::Enable_Spawner(101143, true);
			ScriptEngine::Enable_Spawner(101144, true);
			ScriptEngine::Enable_Spawner(101145, true);
			ScriptEngine::Enable_Spawner(101146, true);

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101148), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101149), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Enable_Barracks, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Enable reinforcements for barracks
			ScriptEngine::Enable_Spawner(101531, true);
			ScriptEngine::Enable_Spawner(101532, true);
			ScriptEngine::Enable_Spawner(101533, true);

			// Disable remaining zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101012), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101212), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Enable_Interior, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Spawn initial interior patrol population
			ScriptEngine::Enable_Spawner(101280, true);
			ScriptEngine::Enable_Spawner(101283, true);
			ScriptEngine::Enable_Spawner(101286, true);
			ScriptEngine::Enable_Spawner(101289, true);
			ScriptEngine::Enable_Spawner(101292, true);
			ScriptEngine::Enable_Spawner(101295, true);
			ScriptEngine::Enable_Spawner(101298, true);
			ScriptEngine::Enable_Spawner(101301, true);

			// Shut down all enable_interior zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101304), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101305), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101306), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101307), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101308), M06_DISABLE_ZONE, 1, 0.0f);

		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Enable_Greenhouse, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Enable greenhouse units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101509), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101356), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101357), M06_CUSTOM_ACTIVATE, 1, 0.0f);


			// Shut down all greenhouse zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101350), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101351), M06_DISABLE_ZONE, 1, 0.0f);

		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Enable_Lab, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Enable units for laboratory
			ScriptEngine::Enable_Spawner(101332, true);
			ScriptEngine::Enable_Spawner(101334, true);
			ScriptEngine::Enable_Spawner(101335, true);
			ScriptEngine::Enable_Spawner(101336, true);
			ScriptEngine::Enable_Spawner(101337, true);
			ScriptEngine::Enable_Spawner(101344, true);
			ScriptEngine::Enable_Spawner(101345, true);
			ScriptEngine::Enable_Spawner(101346, true);
			ScriptEngine::Enable_Spawner(101347, true);
		}
	}


};

DECLARE_SCRIPT(M06_Alarm_Engineer, "")
{
	int broken_alarm_id;
	bool fixing_alarm;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( fixing_alarm, 1 );

	}

	void Created(GameObject * obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		broken_alarm_id = 0;
		fixing_alarm = false;
	}


	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;
		if(action_id == 10 && reason == ACTION_COMPLETE_NORMAL)
		{
			Vector3 alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(broken_alarm_id));
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, 11);
			params.Set_Face_Location( alarm_pos, 1.5f);
			ScriptEngine::Action_Face_Location ( obj, params );

		}

		if(action_id == 11 && reason == ACTION_COMPLETE_NORMAL)
		{
			GameObject * broken_alarm = ScriptEngine::Find_Object(broken_alarm_id);

			if((ScriptEngine::Get_Health(broken_alarm) / ScriptEngine::Get_Max_Health(broken_alarm)) < 1.0f)
			{
				params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, 12 );
				params.Set_Movement( ScriptEngine::Get_Position(obj), 0.0f, 2.0f );
				params.Set_Attack (broken_alarm, 5.0f, 0.0f, 1);
				ScriptEngine::Action_Attack( obj, params );
			}
			else
			{
				fixing_alarm = false;
				ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 10.0f);
			}

		}


	}



	void Sound_Heard(GameObject* obj, const CombatSound & sound) override
	{
		ActionParamsStruct params;

		if (( static_cast<int>(sound.Type) == M06_FIX_ALARM ) && (!fixing_alarm))
		{
			fixing_alarm = true;
			broken_alarm_id = ScriptEngine::Get_ID(sound.Creator);

			Vector3 alarm_pos = ScriptEngine::Get_Position(ScriptEngine::Find_Object(broken_alarm_id));
			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), 10);
			params.Set_Movement (alarm_pos, RUN, 2.0f);
			ScriptEngine::Action_Goto (obj, params);

		}

	}

};

DECLARE_SCRIPT(M06_Courtyard_Eagle, "")
{

	enum {WAYPATH};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
	//	SAVE_VARIABLE( already_seen, 1 );

	}

	void Created (GameObject * obj) override
	{
		ActionParamsStruct params;

		if(ScriptEngine::Get_ID(obj) == 101099)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = 101100;
			params.WaypointStartID = 101101;
			params.WaypointEndID = 101102;
			ScriptEngine::Action_Goto( obj, params );
		}
		else
		{
			ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		}

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_FOOTSTEPS_HEARD - 5, WAYPATH );
			params.Set_Movement( Vector3(0,0,0), WALK, 1.5f );
			params.WaypathID = 101100;
			params.WaypointStartID = 101102;
			params.WaypointEndID = 101101;
			ScriptEngine::Action_Goto( obj, params );
		}
		if(action_id == WAYPATH && reason == ACTION_COMPLETE_LOW_PRIORITY)
		{
			ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		}
	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101104), M06_DEAD_COURTYARD_EAGLE, 1, 0.0f);
	}



};

DECLARE_SCRIPT(M06_Hedgemaze_Eagle, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
	//	SAVE_VARIABLE( already_seen, 1 );

	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
	}


	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101147), M06_DEAD_HEDGEMAZE_EAGLE, 1, 0.0f);
	}

};

DECLARE_SCRIPT(M06_Barracks_Eagle, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
	//	SAVE_VARIABLE( already_seen, 1 );

	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
	}


	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101204), M06_DEAD_BARRACKS_EAGLE, 1, 0.0f);
	}

};

//Grants initial weapons
DECLARE_SCRIPT(M06_Havoc_DLS, "")
{

	bool alarmed;
	int attacking_havoc;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{

		SAVE_VARIABLE( alarmed, 1 );
		SAVE_VARIABLE( attacking_havoc, 2 );
	}

	enum {HAVOC_ALARM_SOUND};

	void Created(GameObject * obj) override
	{
		alarmed = false;
		ScriptEngine::Grant_Key (obj, 10, true);
		// Give Havoc the key to the barracks
		ScriptEngine::Grant_Key (STAR, 9, true);

		ScriptEngine::Give_PowerUp(obj, "POW_Chaingun_Player", false);
		ScriptEngine::Give_PowerUp(obj, "POW_SniperRifle_Player", false);
		ScriptEngine::Give_PowerUp(obj, "POW_MineRemote_Player", false);
		ScriptEngine::Give_PowerUp(obj, "POW_MineRemote_Player", false);
		ScriptEngine::Give_PowerUp(obj, "POW_GrenadeLauncher_Player", false);

	}

	void Destroyed(GameObject * /*obj*/) override
	{
//		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), 666, 666, 0.0f);

	}

	void Sound_Heard(GameObject* obj, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_SOUND_ALARM_ON )
		{
			if(!alarmed)
			{
				ScriptEngine::Start_Timer (obj, this, 0.0f, HAVOC_ALARM_SOUND);
				ScriptEngine::Debug_Message("Alarm heard on Havoc, play M06_ATTACK_HAVOC_SOUND.\n");
				ScriptEngine::Create_Logical_Sound(obj, M06_ATTACK_HAVOC_SOUND, Vector3 (0,0,0), 55.0f);

				alarmed = true;
			}


		}
		if ( static_cast<int>(sound.Type) == M06_SOUND_ALARM_OFF )
		{
			alarmed = false;
			attacking_havoc = 0;

		}


	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{

		if((timer_id == HAVOC_ALARM_SOUND) && (alarmed))
		{
			ScriptEngine::Create_Logical_Sound(obj, SOUND_TYPE_GUNSHOT, Vector3 (0,0,0), 30.0f);
			ScriptEngine::Start_Timer (obj, this, 8.0f, HAVOC_ALARM_SOUND);

		}

	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * sender) override
	{
		ActionParamsStruct params;
		if(type == M06_ATTACK_HAVOC)
		{
			ScriptEngine::Debug_Message("Soldier heard M06_ATTACK_HAVOC_SOUND, custom to attack Havoc.\n");
			if(attacking_havoc <= 3)
			{
				attacking_havoc++;
				ScriptEngine::Send_Custom_Event(obj, sender, M06_YOU_ATTACK_HAVOC, 1, 0.0f);


			}

		}
		if(type == M06_DEAD_HAVOC_ATTACKER)
		{
			ScriptEngine::Debug_Message("Soldier attacking Havoc killed, get new one.\n");
			attacking_havoc = 3;
			ScriptEngine::Create_Logical_Sound(obj, M06_ATTACK_HAVOC_SOUND, Vector3 (0,0,0), 55.0f);
		}
		if(type == M06_RELOCATE)
		{
			if(param == 0)
			{
				ScriptEngine::Set_Position (obj, ScriptEngine::Get_Position(ScriptEngine::Find_Object(107607)));
				ScriptEngine::Set_Facing (obj, -85.0F);
			}
			if(param == 1)
			{
				//ScriptEngine::Set_Position (obj, ScriptEngine::Get_Position(ScriptEngine::Find_Object(108278)));
				ScriptEngine::Set_Position (obj, Vector3 (-34.998F, 24.173F, 0.593F));
				ScriptEngine::Set_Facing (obj, 140.0F);
			}


		}

	}



};

DECLARE_SCRIPT(M06_Resistance_Raider_DLS, "")
{
	bool talking;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( talking, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);

		ScriptEngine::Set_Animation(obj, "S_A_HUMAN.H_A_601A", true, nullptr, 0.0f, -1.0f, false);

		ScriptEngine::Apply_Damage( obj, 25.0f, "STEEL", nullptr);
		talking = false;
		ScriptEngine::Enable_HUD_Pokable_Indicator( obj, true );

	}

	void Poked(GameObject * obj, GameObject * poker) override
	{
		ActionParamsStruct params;

		if (ScriptEngine::Is_A_Star(poker) && !talking)
		{
			talking = true;
			// Looks like you're bleedin'.\n
			const char *conv_name = ("M06_CON045");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
			ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 100823);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
			ScriptEngine::Enable_HUD_Pokable_Indicator( obj, false );

		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if(action_id == 100823 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			ScriptEngine::Apply_Damage( obj, 10000.0f, "STEEL", nullptr);

			Vector3 pos = ScriptEngine::Get_Position(obj);
			float facing = ScriptEngine::Get_Facing(obj);
			float a = WWMath::Cos(DEG_TO_RADF(facing)) * 1.5f;
			float b = WWMath::Sin(DEG_TO_RADF(facing)) * 1.5f;
			Vector3 powerup_loc = pos + Vector3(a, b, 0.5f);
			ScriptEngine::Create_Object("POW_GrenadeLauncher_Player", powerup_loc);

		}
	}

};

DECLARE_SCRIPT(M06_Assistance_Farmer_DLS, "")
{

	int poke_id;
	bool give_health;

	enum {GO_STAR, TALK_STAR, GIVE_HEALTH};
	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( poke_id, 1 );
		SAVE_VARIABLE( give_health, 2 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		give_health = false;

	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);

			ActionParamsStruct params;

			poke_id = 1;

			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, GO_STAR );
			params.Set_Movement( STAR, RUN, 1.5f );
			ScriptEngine::Action_Goto( obj, params );
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;


		if(action_id == TALK_STAR && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			Vector3 pos = ScriptEngine::Get_Position(obj);
			float facing = ScriptEngine::Get_Facing(obj);
			float a = WWMath::Cos(DEG_TO_RADF(facing)) * 1.5f;
			float b = WWMath::Sin(DEG_TO_RADF(facing)) * 1.5f;
			Vector3 powerup_loc = pos + Vector3(a, b, 0.5f);
			ScriptEngine::Create_Object("POW_GrenadeLauncher_Player", powerup_loc);

			// Send custom to M06_Trigger_A04 zone to start conversation in maintenance shaft
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101357), M06_START_CONVERSATION, 1, 0.0f);
		}

		if(action_id == GO_STAR && reason == ACTION_COMPLETE_NORMAL)
		{
			// Assume hands over head anim
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 1);
			params.Set_Animation( "H_A_HOST_L1B", true );
			ScriptEngine::Action_Play_Animation (obj, params);

			// There are two of 'em in the maintenance shaft.  Here, take this.\n
			const char *conv_name = ("M06_CON046");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
			ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, TALK_STAR);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}

	void Poked(GameObject * obj, GameObject * /*poker*/) override
	{
		if(give_health)
		{
			give_health = false;

			// \\Partner.\n
			const char *conv_name = ("M06_CON062");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
			ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 10);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			Vector3 pos = ScriptEngine::Get_Position(obj);
			float facing = ScriptEngine::Get_Facing(obj);
			float a = WWMath::Cos(DEG_TO_RADF(facing)) * 1.5f;
			float b = WWMath::Sin(DEG_TO_RADF(facing)) * 1.5f;
			Vector3 powerup_loc = pos + Vector3(a, b, 0.5f);
			ScriptEngine::Create_Object("tw_POW00_Health", powerup_loc);

			ScriptEngine::Start_Timer (obj, this, 10.0f, GIVE_HEALTH);
		}

	}

	void Timer_Expired(GameObject * /*obj*/, int timer_id ) override
	{

		if(timer_id == GIVE_HEALTH)
		{
			give_health = true;
		}

	}


};

DECLARE_SCRIPT(M06_Greenhouse_Shaft_Unit, "")
{
	int soldier_id;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101356))
			{
				// But sir, there are two&\n
				const char *conv_name = ("M06_CON047");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101356), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Floor1_Library_Unit, "")
{
	int soldier_id;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		if(type == M06_START_CONVERSATION)
		{

			if(ScriptEngine::Find_Object(101358))
			{
				// I see little reason for them to resist.\n
				const char *conv_name = ("M06_CON048");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101358), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Enable_Floor1_Library, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			// Custom to activate innate
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101358), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101360), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101360), M06_START_CONVERSATION, 1, 0.0f);

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101361), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101362), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Floor1_Bedroom_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ActionParamsStruct params;

		ScriptEngine::Innate_Disable(obj);

		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);

	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

	}


};

DECLARE_SCRIPT(M06_Enable_Floor1_Bedroom, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101363), M06_CUSTOM_ACTIVATE, 1, 0.0f);

		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Enable_Floor2_Bedroom, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101364), M06_CUSTOM_ACTIVATE, 1, 0.0f);

		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}


};

DECLARE_SCRIPT(M06_Floor2_Bedroom_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ActionParamsStruct params;

		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

	}


};

DECLARE_SCRIPT(M06_Floor2_Library_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{

		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}


		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101366))
			{
				// What could Raveshaw possibly want with references to Incan astrology?\n
				const char *conv_name = ("M06_CON049");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101366), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Enable_Floor2_Library, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101365), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101366), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101365), M06_START_CONVERSATION, 1, 0.0f);

		}
	}



};

DECLARE_SCRIPT(M06_Floor2_Conference_Unit, "")
{


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101370))
			{
				// We should simply bombard the village with long range artillery.\n
				const char *conv_name = ("M06_CON050");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101370), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Enable_Floor2_Conference, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101369), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101370), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101369), M06_START_CONVERSATION, 1, 0.0f);

		}
	}



};

// WarRoom Officer
DECLARE_SCRIPT(M06_WarRoom_Officer_DLS, "")
{
	bool has_keycard;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( has_keycard, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		has_keycard = false;
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		ActionParamsStruct params;

		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}
		if(type == M06_START_CONVERSATION)
		{
			// Do not patronize me, when are you going to move the scientists to the Research Facility?\n
		//	const char *conv_name = ("M06_CON051");
		//	int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN - 5, 0, true);
		//	ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
		//	ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
		//	ScriptEngine::Start_Conversation (conv_id, 300123);
		//	ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
		if(type == M06_WR_KCARD_ACQUIRED)
		{
			has_keycard = true;
		}

	}

	void Enemy_Seen(GameObject * obj, GameObject * /*enemy*/ ) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, false);
	}
};

DECLARE_SCRIPT(M06_WarRoom_Bodyguard_DLS, "")
{

	bool has_keycard;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( has_keycard, 1 );
	}

	void Created (GameObject * obj) override
	{
		has_keycard = false;
		ScriptEngine::Innate_Disable(obj);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}
		if(type == M06_WR_KCARD_ACQUIRED)
		{
			has_keycard = true;
		}

	}



};

DECLARE_SCRIPT(M06_Enable_Warroom_DoorGuard, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101516), M06_CUSTOM_ACTIVATE, 1, 0.0f);

		}
	}



};

DECLARE_SCRIPT(M06_WarRoom_DoorGuard, "")
{

	bool has_keycard;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( has_keycard, 1 );
	}

	void Created (GameObject * obj) override
	{
		has_keycard = false;
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}
		if(type == M06_WR_KCARD_ACQUIRED)
		{
			has_keycard = true;
		}

	}

};

DECLARE_SCRIPT(M06_Enable_WarRoom, "")
{

	bool already_entered;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Activate innate on courtyard units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101008), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101009), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
		//	ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101008), M06_START_CONVERSATION, 1, 0.0f);

			// Kane conversation with Warroom Officer
			GameObject * holograph = ScriptEngine::Find_Object(112464);
			GameObject * kane = ScriptEngine::Create_Object_At_Bone ( holograph, "Nod_Kane_HologramHead_small", "O_ARROW" );
			ScriptEngine::Attach_To_Object_Bone( kane, holograph, "O_ARROW" );
			ScriptEngine::Disable_All_Collisions ( kane );
			ScriptEngine::Set_Facing (kane, ScriptEngine::Get_Facing (holograph));
			ScriptEngine::Set_Loiters_Allowed( kane, false );
			ScriptEngine::Attach_Script(kane, "M06_KaneHead", "");
		}
	}



};

DECLARE_SCRIPT(M06_KaneHead, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * obj) override
	{
		// Do not patronize me, when are you going to move the scientists to the Research Facility?\n
		const char *conv_name = ("M06_CON051");
		int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN - 5, 0, true);
		ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
		ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101008), conv_id, false, true, true);
		ScriptEngine::Start_Conversation (conv_id, 300123);
		ScriptEngine::Monitor_Conversation (obj, conv_id);
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if((action_id == 300123 && reason == ACTION_COMPLETE_CONVERSATION_ENDED) || (action_id == 300123 && reason == ACTION_COMPLETE_CONVERSATION_INTERRUPTED))
		{
			// Kane conversation with Havoc

			const char *conv_name = ("M06_CON064");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, INNATE_PRIORITY_ENEMY_SEEN + 5, 0, true);
			ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 300124);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
		if((action_id == 300124 && reason == ACTION_COMPLETE_CONVERSATION_ENDED) || (action_id == 300124 && reason == ACTION_COMPLETE_CONVERSATION_INTERRUPTED))
		{
			ScriptEngine::Destroy_Object(obj);
		}
	}


};

DECLARE_SCRIPT(M06_Enable_Officers_Mess, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{


		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Activate innate on courtyard units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101374), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101375), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101374), M06_START_CONVERSATION, 1, 0.0f);
			// Disable zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101376), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101377), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101378), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101379), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101380), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101381), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101382), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}

};

DECLARE_SCRIPT(M06_Officers_Mess_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101375))
			{
				// It is the electromagnetic system that revolutionizes it.\n
				const char *conv_name = ("M06_CON052");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101375), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Courtyard_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101384) && ScriptEngine::Find_Object(101385))
			{
				// Both of you have proven yourself loyal, and as such I feel you are prime candidates for a very sensitive mission.\n
				const char *conv_name = ("M06_CON053");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101384), conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101385), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Enable_Courtyard, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Activate innate on courtyard units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101383), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101384), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101385), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101383), M06_START_CONVERSATION, 1, 0.0f);
			// Disable zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101386), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101387), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101388), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101389), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101390), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}

};

DECLARE_SCRIPT(M06_Enable_Bathroom, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Activate innate on north barracks units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101391), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101393), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101393), M06_START_CONVERSATION, 1, 0.0f);
			// Disable zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101394), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101395), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}

};

DECLARE_SCRIPT(M06_Enable_Barracks_Mess, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Activate innate on north barracks units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101502), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101503), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101502), M06_START_CONVERSATION, 1, 0.0f);
			// Disable zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101504), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101505), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101506), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101507), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101508), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}

};

DECLARE_SCRIPT(M06_Barracks_Mess_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}
		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101503))
			{
				// Put it over there, and don't slop it around.\n
				const char *conv_name = ("M06_CON054");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101503), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Bathroom_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}
		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101391))
			{
				// Man, this new chem suit insulation itches something awful.\n
				const char *conv_name = ("M06_CON055");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101391), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}
};

DECLARE_SCRIPT(M06_Enable_South_Barracks, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

				// Activate innate on barracks units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101399), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101400), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101403), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Send custom to activate conversation
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101400), M06_START_CONVERSATION, 1, 0.0f);
			// Disable zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101401), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101402), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}

};

DECLARE_SCRIPT(M06_South_Barracks_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);

	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

		if(type == M06_START_CONVERSATION)
		{
			if(ScriptEngine::Find_Object(101399))
			{
				// I just don't get it, we are finely trained soldiers. What do we need mercs for?\n
				const char *conv_name = ("M06_CON056");
				int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
				ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
				ScriptEngine::Join_Conversation(ScriptEngine::Find_Object(101399), conv_id, false, true, true);
				ScriptEngine::Start_Conversation (conv_id, 10);
				ScriptEngine::Monitor_Conversation (obj, conv_id);
			}
		}

	}

};

DECLARE_SCRIPT(M06_Enable_North_Barracks, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

				// Activate innate on barracks units
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101396), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101397), M06_CUSTOM_ACTIVATE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101398), M06_CUSTOM_ACTIVATE, 1, 0.0f);

			// Disable zones
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101500), M06_DISABLE_ZONE, 1, 0.0f);
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101501), M06_DISABLE_ZONE, 1, 0.0f);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_DISABLE_ZONE)
		{
			already_entered = true;
		}

	}

};

DECLARE_SCRIPT(M06_North_Barracks_Unit, "")
{

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( soldier_id, 1 );
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);
		ActionParamsStruct params;


	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			ScriptEngine::Innate_Enable(obj);
		}

	}

};



DECLARE_SCRIPT (M06_Courtyard_Apache, "")
{
	bool out_of_range;

	enum {COURTYARD};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( out_of_range, 1 );
	}

	void Created (GameObject *obj) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, 100, COURTYARD );
		params.Set_Movement( Vector3(0,0,0), 1.0f, 2.0f );
		params.WaypathID = 101428;
		params.WaypathSplined = true;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete( GameObject * obj, int action_id, ActionCompleteReason reason ) override	//this is called when the servant reaches the destination location.
	{
		ActionParamsStruct params;

		if (action_id == COURTYARD && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, 100, COURTYARD );
			params.Set_Movement( Vector3(0,0,0), 1.0f, 2.0f );
			params.Set_Attack (STAR, 150.0f, 1.0f, 1);
			params.WaypathID = 101428;
			params.WaypathSplined = true;
			ScriptEngine::Action_Attack( obj, params );
		}
		if (action_id == COURTYARD && reason == ACTION_COMPLETE_ATTACK_OUT_OF_RANGE)
		{
			out_of_range = true;
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject * /*enemy*/ ) override
	{
		if(out_of_range)
		{
			ActionParamsStruct params;

			out_of_range = false;

			params.Set_Basic( this, 100, COURTYARD );
			params.Set_Movement( Vector3(0,0,0), 1.0f, 2.0f );
			params.Set_Attack (STAR, 150.0f, 1.0f, 1);
			params.WaypathID = 101428;
			params.WaypathSplined = true;
			ScriptEngine::Action_Attack( obj, params );
		}
	}
};

DECLARE_SCRIPT(M06_Flyover_Controller, "")  // 100018
{
	int last;
	bool flyovers;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( last, 1 );
		SAVE_VARIABLE( flyovers, 2 );
	}

	void Created(GameObject * obj) override
	{
		last = 0;
		flyovers = true;
		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), M06_FLYOVER_COMPLETE, 1, 0.0f);
	}

	void Custom (GameObject * obj, int type, intptr_t param, GameObject * /*sender*/) override
	{
		ActionParamsStruct params;
		if(type == M06_FLYOVER_COMPLETE && flyovers)
		{
			last = param;

			static constexpr const char *flyover_files[11] =
			{
				"M06_XG_VehicleDrop0.txt",
				"M06_XG_VehicleDrop1.txt",
				"M06_XG_VehicleDrop2.txt",

				"X6A_Apache_03.txt",
				"X6A_Apache_04.txt",
				"X6A_Apache_05.txt",
				"X6A_Apache_06.txt",
				"X6A_Apache_07.txt",
				"X6A_Apache_08.txt",
				"X6A_Apache_09.txt",


				"M06_XG_EV2_1.txt",

			};

			int random = Get_Int_Random(0, 10);

			while(random == last)
			{
				random = Get_Int_Random(0, 10);
			}

			switch(random)
			{
			case 1:
			case 2:
			case 3:
			case 10:
				{
					GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(-44.177f, 30.547f, 1.605f));
					ScriptEngine::Set_Facing(controller, 0.000f);
					ScriptEngine::Attach_Script(controller, "Test_Cinematic", flyover_files[random]);
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				{
					GameObject *controller = ScriptEngine::Create_Object("Invisible_Object", Vector3(0,0,0));
					ScriptEngine::Attach_Script(controller, "Test_Cinematic", flyover_files[random]);
				}
				break;
			}


		}

		if(type == M06_ACTIVATE_FLYOVERS)
		{
			if(param == 1 && !flyovers)
			{
				flyovers = true;
				ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), M06_FLYOVER_COMPLETE, 1, 0.0f);
			}
			else if (param == 0 && flyovers)
			{
				flyovers = false;
			}
		}

	}
};

DECLARE_SCRIPT(M06_Activate_Flyovers, "Active=0:int")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{

	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			bool active = (Get_Int_Parameter("Active") == 1) ? true : false;
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), M06_ACTIVATE_FLYOVERS, active, 0.0f);
		}
	}
};

DECLARE_SCRIPT(M06_Flyover, "Flyover_ID=0:int")
{
	int flyover_id;

	void Created( GameObject * obj ) override
	{
		ScriptEngine::Disable_Physical_Collisions ( obj );
		flyover_id = Get_Int_Parameter("Flyover_ID");
	}

	void Destroyed(GameObject * obj) override
	{
		ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(100018), M06_FLYOVER_COMPLETE, flyover_id, 0.0f);

	}
};

DECLARE_SCRIPT(M06_Drop_Thunder_Squad, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// This is Nod Helo Delta 14.  We are hot dropping a squad of Black Hand at your doorstep, be advised they are on the ground only long enough to re-equip, skip the standard check in.  Raveshaw's orders.\n
			const char *conv_name = ("M06_CON057");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
			ScriptEngine::Join_Conversation(nullptr, conv_id, true, true, true);
			ScriptEngine::Join_Conversation(STAR, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 300603);
			ScriptEngine::Monitor_Conversation (obj, conv_id);

			GameObject * chinook_obj = ScriptEngine::Create_Object ( "Invisible_Object", Vector3(-55.305f, -87.235f, -0.767f));
			ScriptEngine::Set_Facing(chinook_obj, 85.000f);
			ScriptEngine::Attach_Script(chinook_obj, "Test_Cinematic", "X6I_TroopDrop1.txt");

		}
	}
};

DECLARE_SCRIPT(M06_Thunder_Unit, "")
{
	enum{WAYPATH};

	void Created( GameObject * obj ) override
	{
		ActionParamsStruct params;

		params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, WAYPATH );
		params.Set_Movement( Vector3(0,0,0), RUN, 1.5f );
		params.WaypathID = 101520;
		ScriptEngine::Action_Goto( obj, params );
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		if(action_id == WAYPATH && reason == ACTION_COMPLETE_NORMAL)
		{
			ScriptEngine::Set_Innate_Soldier_Home_Location(obj, ScriptEngine::Get_Position(obj), 2.0f);

		}

	}

	void Killed (GameObject * obj, GameObject * /*killer*/) override
	{
		Vector3 powerup_loc = ScriptEngine::Get_Position(obj);
		ScriptEngine::Create_Object("POW_RocketLauncher_Player", powerup_loc);
	}

};

DECLARE_SCRIPT(M06_Servant_Behavior, "Loc1_ID=0:int, Loc2_ID=0:int, Loc3_ID=0:int, Animation=S_A_HUMAN.H_A_A0F0:string")
{
	bool enemy_seen;

	enum{GO_FIRST_LOC, FACING_FIRST_LOC, ANIMATE_FIRST_LOC, GO_SECOND_LOC, FACING_SECOND_LOC, ANIMATE_SECOND_LOC,
		GO_THIRD_LOC, FACING_THIRD_LOC, ANIMATE_THIRD_LOC, HANDS_OVER_HEAD, START_OVER};

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( enemy_seen, 1 );
	}

	void Created (GameObject * obj) override
	{
		ActionParamsStruct params;

		enemy_seen = false;

		params.Set_Basic (this, INNATE_PRIORITY_ENEMY_SEEN - 5, GO_FIRST_LOC);
		params.Set_Movement (ScriptEngine::Find_Object(Get_Int_Parameter("Loc1_ID")), WALK, 1.5f);
		ScriptEngine::Action_Goto (obj, params);

	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		ActionParamsStruct params;

		const char *anim = Get_Parameter("Animation");

		// First Loc
		if(action_id == GO_FIRST_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, FACING_FIRST_LOC);
			params.Set_Face_Location( ScriptEngine::Get_Position(ScriptEngine::Find_Object(Get_Int_Parameter("Loc1_ID"))), 1.5f);
			ScriptEngine::Action_Face_Location ( obj, params );
		}
		if(action_id == FACING_FIRST_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, ANIMATE_FIRST_LOC );
			params.Set_Animation (anim, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
		if(action_id == ANIMATE_FIRST_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), GO_SECOND_LOC);
			params.Set_Movement (ScriptEngine::Find_Object(Get_Int_Parameter("Loc2_ID")), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}

		// Second Loc
		if(action_id == GO_SECOND_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, FACING_SECOND_LOC);
			params.Set_Face_Location( ScriptEngine::Get_Position(ScriptEngine::Find_Object(Get_Int_Parameter("Loc2_ID"))), 1.5f);
			ScriptEngine::Action_Face_Location ( obj, params );
		}
		if(action_id == FACING_SECOND_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, ANIMATE_SECOND_LOC );
			params.Set_Animation (anim, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
		if(action_id == ANIMATE_SECOND_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), GO_THIRD_LOC);
			params.Set_Movement (ScriptEngine::Find_Object(Get_Int_Parameter("Loc3_ID")), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}

		// Third Loc

		if(action_id == GO_THIRD_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, FACING_THIRD_LOC);
			params.Set_Face_Location( ScriptEngine::Get_Position(ScriptEngine::Find_Object(Get_Int_Parameter("Loc3_ID"))), 1.5f);
			ScriptEngine::Action_Face_Location ( obj, params );
		}
		if(action_id == FACING_THIRD_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN - 5, ANIMATE_THIRD_LOC );
			params.Set_Animation (anim, false);
			ScriptEngine::Action_Play_Animation (obj, params);
		}
		if(action_id == ANIMATE_THIRD_LOC && reason == ACTION_COMPLETE_NORMAL)
		{
			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), GO_FIRST_LOC);
			params.Set_Movement (ScriptEngine::Find_Object(Get_Int_Parameter("Loc1_ID")), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}

		if(action_id == HANDS_OVER_HEAD && reason == ACTION_COMPLETE_NORMAL)
		{
			// In hands over head anim
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10);
			params.Set_Animation( "H_A_HOST_L1B", true );
			ScriptEngine::Action_Play_Animation (obj, params);
		}
	}

	void Enemy_Seen(GameObject * obj, GameObject * /*enemy*/ ) override
	{
		ActionParamsStruct params;





		ScriptEngine::Create_Logical_Sound(obj, SOUND_TYPE_GUNSHOT, ScriptEngine::Get_Position(obj), 8.0f);
		if(!enemy_seen)
		{
			// Help!  Intruder!  Help!\n
			const char *conv_name = ("M06_CON058");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 0, 0, true);
			ScriptEngine::Join_Conversation(obj, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 1010211);

			// Assume hands over head anim
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, HANDS_OVER_HEAD);
			params.Set_Animation( "H_A_HOST_L1A", false );
			ScriptEngine::Action_Play_Animation (obj, params);

			ScriptEngine::Start_Timer (obj, this, 20.0f, START_OVER);
			enemy_seen = true;
		}
	}

	void Timer_Expired(GameObject * obj, int timer_id ) override
	{
		ActionParamsStruct params;

		if(timer_id == START_OVER)
		{
			enemy_seen = false;

			// Exit hands over head anim
			params.Set_Basic( this, INNATE_PRIORITY_ENEMY_SEEN + 5, 10);
			params.Set_Animation( "H_A_HOST_L1C", false );
			ScriptEngine::Action_Play_Animation (obj, params);

			params.Set_Basic (this, (INNATE_PRIORITY_ENEMY_SEEN - 5), GO_FIRST_LOC);
			params.Set_Movement (ScriptEngine::Find_Object(Get_Int_Parameter("Loc1_ID")), WALK, 1.5f);
			ScriptEngine::Action_Goto (obj, params);
		}

	}

};

DECLARE_SCRIPT(M06_Move_Sydney, "Move_Loc=0:int")
{


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
//		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{

	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if (ScriptEngine::Is_A_Star(enterer))
		{
			int move_loc = Get_Int_Parameter("Move_Loc");
			ScriptEngine::Send_Custom_Event(obj, ScriptEngine::Find_Object(101010), M06_MOVE_SYDNEY, move_loc, 0.0f);
		}
	}

};

DECLARE_SCRIPT(M06_Collapse_Zone, "Zone_ID=0:int")
{
	bool already_entered;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = true;
	}

	void Custom(GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CHATEAU_COLLAPSE)
		{
			already_entered = false;
		}
	}

	void Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			switch(Get_Int_Parameter("Zone_ID"))
			{
			// First section of crypt, immediately past the lab door
			case 1:
				{
					GameObject * l6_falldust01 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust01, "L6_falldust01" );

					GameObject * l6_falldust02 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust02, "L6_falldust02" );

					GameObject * l6_fallrocks01 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks01, "L6_fallrocks01" );

					GameObject * l6_fallrocks07 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks07, "L6_fallrocks07" );

					GameObject * l6_fallrocks08 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks08, "L6_fallrocks08" );
				}
				break;
			// Back section of crypt, right before you exit out to the stairs up to Chateau
			case 2:
				{
					GameObject * l6_fallrocks09 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks09, "L6_fallrocks09" );

					GameObject * l6_fallrocks11 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks11, "L6_fallrocks11" );

					GameObject * l6_fallrocks10 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks10, "L6_fallrocks10" );

					GameObject * l6_fallfire01 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallfire01, "L6_fallfire01" );
					ScriptEngine::Set_Animation(l6_fallfire01, "L6_Fallfire01.L6_Fallfire01", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_floorfire01 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_floorfire01, "L6_floorfire01" );
					ScriptEngine::Set_Animation(l6_floorfire01, "L6_floorfire01.L6_floorfire01", true, nullptr, 0.0f, -1.0f, false);
				}
				break;
			// Staircase leading out of crypt and hallway immediately up that staircase
			case 3:
				{
					GameObject * l6_falldust03 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust03, "L6_falldust03" );

					GameObject * l6_fallrocks12 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks12, "L6_fallrocks12" );

					GameObject * l6_fallfire02 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallfire02, "L6_fallfire02" );
					ScriptEngine::Set_Animation(l6_fallfire02, "L6_Fallfire02.L6_Fallfire02", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_floorfire02 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_floorfire02, "L6_floorfire02" );
					ScriptEngine::Set_Animation(l6_floorfire02, "L6_floorfire02.L6_floorfire02", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_falldust04 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust04, "L6_falldust04" );
				}
				break;
			// Hallway at the top of the staircase leading from crypt, immediately before wooden beam mess hall
			case 4:
				{
					GameObject * l6_fallrocks02 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks02, "L6_fallrocks02" );

					GameObject * l6_falldust05 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust05, "L6_falldust05" );


				}
				break;
			// Large mess hall with wooden beams supporting the ceiling
			case 5:
				{
					GameObject * l6_fallwood02 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood02, "L6_fallwood02" );

					GameObject * l6_fallwood03 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood03, "L6_fallwood02" );

					GameObject * l6_fallrocks03 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks03, "L6_fallrocks03" );

					GameObject * l6_fallwood04 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood04, "L6_fallwood04" );

					GameObject * l6_falldust10 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust10, "L6_falldust10" );

					GameObject * l6_falldust06 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust06, "L6_falldust06" );

					GameObject * l6_falldust21 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust21, "L6_falldust21" );

					GameObject * l6_falldust07 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust07, "L6_falldust07" );


				}
				break;
			// Staircase leading out of mess and into main artwork area
			case 6:
				{
					GameObject * l6_falldust08 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust08, "L6_falldust08" );

					GameObject * l6_fallfire03 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallfire03, "L6_fallfire03" );
					ScriptEngine::Set_Animation(l6_fallfire03, "L6_Fallfire03.L6_Fallfire03", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_falldust09 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust09, "L6_falldust09" );

					GameObject * l6_falldust11 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust11, "L6_falldust11" );
				}
				break;
			// Columned room
			case 7:
				{
					GameObject * l6_fallwood05 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood05, "L6_fallwood05" );

					GameObject * l6_fallrocks05 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks05, "L6_fallrocks05" );

					GameObject * l6_fallwood06 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood06, "L6_fallwood06" );

					GameObject * l6_fallrocks06 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallrocks06, "L6_fallrocks06" );

					GameObject * l6_falldust17 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust17, "L6_falldust17" );

					GameObject * l6_floorfire06 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_floorfire06, "L6_floorfire06" );
					ScriptEngine::Set_Animation(l6_floorfire06, "L6_floorfire06.L6_floorfire06", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_falldust18 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust18, "L6_falldust18" );
				}
				break;
			// Master staircase
			case 8:
				{
					GameObject * l6_fallwood07 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood07, "L6_fallwood07" );

					GameObject * l6_falldust12 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust12, "L6_falldust12" );

					GameObject * l6_falldust13 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust13, "L6_falldust13" );

					GameObject * l6_falldust16 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust16, "L6_falldust16" );

					GameObject * l6_floorfire04 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_floorfire04, "L6_floorfire04" );
					ScriptEngine::Set_Animation(l6_floorfire04, "L6_floorfire04.L6_floorfire04", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_floorfire05 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_floorfire05, "L6_floorfire05" );
					ScriptEngine::Set_Animation(l6_floorfire05, "L6_floorfire04.L6_floorfire05", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_falldust15 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust15, "L6_falldust15" );

					GameObject * l6_fallfire05 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallfire05, "L6_fallfire05" );
					ScriptEngine::Set_Animation(l6_fallfire05, "L6_Fallfire05.L6_Fallfire05", true, nullptr, 0.0f, -1.0f, false);

					GameObject * l6_falldust14 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust14, "L6_falldust14" );

				}
				break;
			// Escape hall and outside room
			case 9:
				{
					GameObject * l6_falldust19 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust19, "L6_falldust19" );

					GameObject * l6_falldust20 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust20, "L6_falldust20" );

					GameObject * l6_falldust22 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust22, "L6_falldust22" );

					GameObject * l6_fallwood08 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_fallwood08, "L6_fallwood08" );

					GameObject * l6_falldust23 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust23, "L6_falldust23" );

					GameObject * l6_falldust24 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust24, "L6_falldust24" );

					GameObject * l6_falldust25 = ScriptEngine::Create_Object("Invisible_Object", Vector3(0.0f, 0.0f, 0.0f) );
					ScriptEngine::Set_Model ( l6_falldust25, "L6_falldust25" );

				}
				break;
			}
		}
	}

};

DECLARE_SCRIPT(M06_Activate_MidtroC, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = true;

	}

	void Entered (GameObject * /*obj*/, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;
	//		ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(101010), 100, 100, 0.0f);

			// Block for Mendoza arena
			GameObject *crate28 = ScriptEngine::Create_Object("X6_Rubble2", Vector3(-49.309f, 1.010f, -0.016f));
			ScriptEngine::Set_Facing( crate28, 140.000f );

			GameObject *crate29 = ScriptEngine::Create_Object("X6_Rubble2", Vector3(-50.727f, 0.514f, -0.020f));
			ScriptEngine::Set_Facing( crate29, 5.000f );

			GameObject *crate30 = ScriptEngine::Create_Object("X6_Rubble1", Vector3(-11.987f, 8.771f, 0.000f));
			ScriptEngine::Set_Facing( crate30, 90.000f );

			GameObject *crate1 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(-6.280f, 9.196f, 0.000f));
			ScriptEngine::Set_Facing( crate1, 50.000f );

			GameObject *crate2 = ScriptEngine::Create_Object("X6_Rubble5", Vector3(-5.988f, 7.571f, -0.010f));
			ScriptEngine::Set_Facing( crate2, 135.000f );

			Vector3 havoc_loc = ScriptEngine::Get_Position (STAR);
			havoc_loc.Z += 7.0f;
			ScriptEngine::Set_Position (STAR, havoc_loc );

			GameObject * sydney = ScriptEngine::Find_Object(101010);
			Vector3 sydney_loc = ScriptEngine::Get_Position (sydney);
			havoc_loc.Z += 7.0f;
			ScriptEngine::Set_Position (sydney, sydney_loc );

			GameObject * cinematic_obj = ScriptEngine::Create_Object ( "Invisible_Object", Vector3(0.0f, 0.0f, 0.0f));
			ScriptEngine::Attach_Script(cinematic_obj, "Test_Cinematic", "X6C_MIDTRO.txt");

			// Take key to the barracks away from Havoc so that Mendoza will not follow him in and get stuck
			ScriptEngine::Grant_Key (STAR, 9, false);
		}
	}

	void Custom (GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		if(type == M06_CUSTOM_ACTIVATE)
		{
			already_entered = false;
		}
	}
};

DECLARE_SCRIPT (M06_Clear_For_Mendoza, "")
{
	void Sound_Heard(GameObject* obj, const CombatSound & sound) override
	{

		if ( static_cast<int>(sound.Type) == M06_CLEAR_FOR_MENDOZA )
		{
			ScriptEngine::Destroy_Object(obj);
		}
	}
};

DECLARE_SCRIPT(M06_DataDisc_01_DLS, "")//
{
	void Custom( GameObject * /*obj*/, int type, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		if ( type == CUSTOM_EVENT_POWERUP_GRANTED )
		{
			// Reveal Mutant Templar
			bool reveal = ScriptEngine::Reveal_Encyclopedia_Character(28);
			if ( reveal == 1 )
			{
				ScriptEngine::Display_Encyclopedia_Event_UI();
			}
		}
	}
};

DECLARE_SCRIPT(M06_Enable_Alarm_Objective, "")
{

	bool already_entered;


	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( already_entered, 1 );

	}

	void Created (GameObject * /*obj*/) override
	{
		already_entered = false;
	}

	void Entered (GameObject * obj, GameObject * enterer) override
	{
		if ((ScriptEngine::Is_A_Star(enterer)) && (!already_entered))
		{
			already_entered = true;

			// Warning: Nod Security Measures Detected.
			const char *conv_name = ("M06_CON060");
			int conv_id = ScriptEngine::Create_Conversation (conv_name, 100, 200.0f, false);
			ScriptEngine::Join_Conversation(nullptr, conv_id, false, true, true);
			ScriptEngine::Start_Conversation (conv_id, 300609);
			ScriptEngine::Monitor_Conversation (obj, conv_id);
		}
	}

	void Action_Complete(GameObject * obj, int action_id, ActionCompleteReason reason) override
	{
		if(action_id == 300609 && reason == ACTION_COMPLETE_CONVERSATION_ENDED)
		{
			// Deactivate alarm system
			ScriptEngine::Send_Custom_Event (obj, ScriptEngine::Find_Object(100018), 609, 3, 0.0f);
		}
	}


};




