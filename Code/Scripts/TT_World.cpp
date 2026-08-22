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
*     TT_World.cpp
*
* DESCRIPTION
*     The general-purpose objects the 4.8.4 library added to a level: money
*     trickles, sound speakers, one-shot setup scripts, the nuke and ion
*     cannon effects.  Native port of the library's jfwws.cpp; the scripts keep
*     their registered names so existing levels still find them.
*
*     Seven of the names that file registers are a second spelling of a script
*     this tree already owns, so they are registered as aliases beside the
*     canonical script rather than copied here -- see the note at the bottom of
*     this file for which, and why.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"


/*JFW_Building_Explode_No_Damage

  Shakes the camera when the object dies, without the explosion damaging
  anything.
*/

DECLARE_SCRIPT_TT(JFW_Building_Explode_No_Damage, "")
{
	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		ScriptEngine::Shake_Camera(ScriptEngine::Get_Position(obj), 25.0f, 0.1f, 4.0f);
		Destroy_Script();
	}
};


/*JFW_Viceroid_Innate

  Keeps a visceroid where it was put and plays its idle and death animations.
*/

DECLARE_SCRIPT_TT(JFW_Viceroid_Innate, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Set_Loiters_Allowed(obj, false);
		ScriptEngine::Set_Animation(obj, "C_Visceroid.C_Visceroid", true, nullptr, 0.0f, -1.0f, false);
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		ScriptEngine::Set_Animation(obj, "C_Visceroid.C_Visceroid_Die", false, nullptr, 0.0f, -1.0f, false);
	}
};


/*JFW_No_Innate

  Turns the object's own behaviour off, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_No_Innate, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Innate_Disable(obj);
		Destroy_Script();
	}
};


/*JFW_Credit_Trickle

  Pays the object's team a little at a time, forever.

  Parameters:

  Credits			= How much each time.
  Delay				= Seconds between payments.
*/

DECLARE_SCRIPT_TT(JFW_Credit_Trickle, "Credits=1:int,Delay=2.0:float")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Delay"), 667);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number == 667) {
			ScriptEngine::Give_Money(obj, (float)Get_Int_Parameter("Credits"), true);
			ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Delay"), 667);
		}
	}
};


/*JFW_Vehicle_Regen

  Heals the vehicle a little every second until it is whole again.

  The registered name carries the 4.8.4 spelling, "JFW_Vechicle_Regen".  It is
  a typo, but it is the name levels were saved against.
*/

DECLARE_SCRIPT_TT_NAMED(JFW_Vehicle_Regen, "JFW_Vechicle_Regen", "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Send_Custom_Event(obj, obj, 0, 0, 0);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != 0) {
			return;
		}

		if (ScriptEngine::Get_Health(obj) < ScriptEngine::Get_Max_Health(obj)) {
			ScriptEngine::Apply_Damage(obj, -2.0f, "RegenHealth", nullptr);
		}

		ScriptEngine::Send_Custom_Event(obj, obj, 0, 0, 1.0f);
	}
};


/*JFW_Enable_Transition

  Lets people climb in and out of the vehicle again, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_Enable_Transition, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Enable_Vehicle_Transitions(obj, true);
		Destroy_Script();
	}
};


/*JFW_Permanent_No_Falling_Damage

  Puts back whatever a fall took off.  Falling damage arrives as a Damaged with
  no amount, which is how it is told apart from being shot.
*/

DECLARE_SCRIPT_TT(JFW_Permanent_No_Falling_Damage, "")
{
	float health;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(health, 1);
	}

	void Created(GameObject* obj) override
	{
		health = ScriptEngine::Get_Health(obj);
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float amount) override
	{
		if (amount == 0.0f) {
			ScriptEngine::Set_Health(obj, health);
		}
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == CUSTOM_EVENT_FALLING_DAMAGE) {
			health = ScriptEngine::Get_Health(obj);
		}
	}
};


/*JFW_Death_Powerup

  Kills whoever picks it up.
*/

DECLARE_SCRIPT_TT(JFW_Death_Powerup, "")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED && sender != nullptr) {
			ScriptEngine::Apply_Damage(sender, 10000.0f, "Death", nullptr);
		}
	}
};


/*JFW_CnC_Crate

  Pays whoever picks it up a hundred credits.
*/

DECLARE_SCRIPT_TT(JFW_CnC_Crate, "")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED && sender != nullptr) {
			ScriptEngine::Give_Money(sender, 100.0f, false);
		}
	}
};


/*JFW_Tiberium_Refinery

  Pays the object's team while the object is still standing.

  Parameters:

  MoneyAmount		= How much each time.
  TimerLength		= Seconds between payments.
*/

DECLARE_SCRIPT_TT(JFW_Tiberium_Refinery, "MoneyAmount:int,TimerLength:int")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Start_Timer(obj, this, (float)Get_Int_Parameter("TimerLength"), 1);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number != 1) {
			return;
		}

		if (ScriptEngine::Get_Health(obj) > 0.0f) {
			ScriptEngine::Give_Money(obj, (float)Get_Int_Parameter("MoneyAmount"), true);
			ScriptEngine::Start_Timer(obj, this, (float)Get_Int_Parameter("TimerLength"), 1);
		}
	}
};


/*
**	The two prize powerups below pay a random multiple of their amount.  A
**	Randomizer of one means "pay between one and two times"; zero is read as
**	one rather than as no payment at all.
*/
static int	Prize_Multiplier(int randomizer)
{
	if (randomizer == 0) {
		randomizer = 1;
	}

	return ScriptEngine::Get_Random_Int(1, randomizer + 1);
}


/*JFW_GrantMoney_Powerup

  Pays whoever picks it up, or their whole team.

  Parameters:

  ScoreAmount		= Base amount.
  Entire_Team		= Pay the picker-up's whole team rather than just them.
  Randomizer		= Largest multiple of the base amount that can be paid.
*/

DECLARE_SCRIPT_TT(JFW_GrantMoney_Powerup, "ScoreAmount:float,Entire_Team=0:int,Randomizer=1:int")
{
	void Custom(GameObject* /*obj*/, int /*type*/, intptr_t /*param*/, GameObject* sender) override
	{
		float amount = Get_Float_Parameter("ScoreAmount")
				* (float)Prize_Multiplier(Get_Int_Parameter("Randomizer"));

		ScriptEngine::Give_Money(sender, amount, Get_Int_Parameter("Entire_Team") != 0);
	}
};


/*JFW_GrantScore_Powerup

  As above, but in points rather than credits.

  Parameters:

  ScoreAmount		= Base amount.
  Entire_Team		= Score for the picker-up's whole team rather than just them.
  Randomizer		= Largest multiple of the base amount that can be scored.
*/

DECLARE_SCRIPT_TT(JFW_GrantScore_Powerup, "ScoreAmount:float,Entire_Team=0:int,Randomizer=1:int")
{
	void Custom(GameObject* /*obj*/, int /*type*/, intptr_t /*param*/, GameObject* sender) override
	{
		float amount = Get_Float_Parameter("ScoreAmount")
				* (float)Prize_Multiplier(Get_Int_Parameter("Randomizer"));

		ScriptEngine::Give_Points(sender, amount, Get_Int_Parameter("Entire_Team") != 0);
	}
};


/*JFW_Disable_Physical_Collision

  Lets everything walk through the object, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_Disable_Physical_Collision, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Disable_Physical_Collisions(obj);
		Destroy_Script();
	}
};


/*JFW_Enable_Physical_Collision

  Puts the object back in everybody's way, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_Enable_Physical_Collision, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Enable_Collisions(obj);
		Destroy_Script();
	}
};


/*JFW_DestroyedStateObject

  Leaves a wreck behind, facing the way the original did.

  Parameters:

  OriginalModelFacing	= Which way the wreck faces.
  DestroyedModelPreset	= What the wreck is.
*/

DECLARE_SCRIPT_TT(JFW_DestroyedStateObject, "OriginalModelFacing:float,DestroyedModelPreset:string")
{
	void Destroyed(GameObject* obj) override
	{
		GameObject* wreck = ScriptEngine::Create_Object(Get_Parameter("DestroyedModelPreset"),
				ScriptEngine::Get_Position(obj));

		if (wreck != nullptr) {
			ScriptEngine::Set_Facing(wreck, Get_Float_Parameter("OriginalModelFacing"));
		}
	}
};


/*JFW_Engine_Sound

  Runs a looping sound from a bone for as long as the object exists.

  Parameters:

  Preset			= Sound to run.
  Bone				= Bone to run it from.
*/

DECLARE_SCRIPT_TT(JFW_Engine_Sound, "Preset:string,Bone:string")
{
	int sound;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(sound, 1);
	}

	void Created(GameObject* obj) override
	{
		sound = ScriptEngine::Create_3D_Sound_At_Bone(Get_Parameter("Preset"), obj, Get_Parameter("Bone"));
	}

	void Destroyed(GameObject* /*obj*/) override
	{
		ScriptEngine::Stop_Sound(sound, true);
	}
};


/*
**	The two medal powerups below tell the player what they picked up.  4.8.4
**	wrote the line straight onto the local HUD because its scripts ran on the
**	client; here the script runs on the server, so the line goes to the one
**	player who picked the medal up rather than to everybody's screen.
*/
static const Vector3	MEDAL_TEXT_COLOR(0.196f, 0.882f, 0.196f);


/*JFW_HealthMedal_TextMessage

  Tells whoever picks it up what a health medal is.
*/

DECLARE_SCRIPT_TT(JFW_HealthMedal_TextMessage, "")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			ScriptEngine::Set_HUD_Help_Text_Player(sender, IDS_M00DSGN_DSGN1008I1DSGN_TXT, MEDAL_TEXT_COLOR);
		}
	}
};


/*JFW_ArmorMedal_TextMessage

  Tells whoever picks it up what an armour medal is.
*/

DECLARE_SCRIPT_TT(JFW_ArmorMedal_TextMessage, "")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED) {
			ScriptEngine::Set_HUD_Help_Text_Player(sender, IDS_M00DSGN_DSGN1009I1DSGN_TXT, MEDAL_TEXT_COLOR);
		}
	}
};


/*JFW_C130_Explosion

  Blows the cargo plane apart at its body bone when it is shot down.
*/

DECLARE_SCRIPT_TT(JFW_C130_Explosion, "")
{
	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		ScriptEngine::Create_Explosion_At_Bone("Explosion_Cargo_Plane", obj, "BODYMAIN", nullptr);
	}
};


/*JFW_Send_Object_ID

  Tells another object this one exists, then goes away.

  Parameters:

  Receiver_ID		= Object to tell.
  Param				= Value to send with the custom.
  Delay				= Seconds to wait first.
*/

DECLARE_SCRIPT_TT(JFW_Send_Object_ID, "Receiver_ID:int,Param=0:int,Delay=1.0:float")
{
	void Created(GameObject* obj) override
	{
		GameObject* receiver = ScriptEngine::Find_Object(Get_Int_Parameter("Receiver_ID"));

		if (receiver != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, receiver, M00_SEND_OBJECT_ID,
					Get_Int_Parameter("Param"), Get_Float_Parameter("Delay"));
		}

		Destroy_Script();
	}
};


/*JFW_Ion_Cannon_Sound

  Plays one of the two ion cannon sounds, then goes away.

  Parameters:

  Number			= 0 for the buildup, anything else for the beam.
*/

DECLARE_SCRIPT_TT(JFW_Ion_Cannon_Sound, "Number=0:int")
{
	void Created(GameObject* obj) override
	{
		const char* sound = (Get_Int_Parameter("Number") == 0)
				? "Ion_Cannon_Buildup" : "Ion_Cannon_Fire";

		ScriptEngine::Create_Sound(sound, ScriptEngine::Get_Position(obj), obj);
		Destroy_Script();
	}
};


/*JFW_NukeStrike_Anim

  The whole nuclear strike, from the missile coming down to the last of the
  cloud, driven off three customs it sends itself.
*/

DECLARE_SCRIPT_TT(JFW_NukeStrike_Anim, "")
{
	//
	//	Each stage swaps this object's own model and hangs an animation script
	//	on it.  The Generic_Cinematic created alongside is what 4.8.4 tested
	//	before doing so; without a cinematic object to play into, the stage is
	//	skipped.
	//
	void Play_Stage(GameObject* obj, const char* model, const char* animation)
	{
		GameObject* cinematic = ScriptEngine::Create_Object("Generic_Cinematic",
				ScriptEngine::Get_Position(obj));

		if (cinematic == nullptr) {
			return;
		}

		ScriptEngine::Set_Model(obj, model);
		ScriptEngine::Attach_Script(obj, "JFW_PlayAnimation_DestroyObject", animation);
	}

	void Created(GameObject* obj) override
	{
		ScriptEngine::Send_Custom_Event(obj, obj, 1, 0, 6.0f);
		ScriptEngine::Send_Custom_Event(obj, obj, 2, 0, 8.0f);
		ScriptEngine::Send_Custom_Event(obj, obj, 3, 0, 11.0f);

		GameObject* cinematic = ScriptEngine::Create_Object("Generic_Cinematic",
				ScriptEngine::Get_Position(obj));

		if (cinematic != nullptr) {
			ScriptEngine::Set_Model(obj, "XG_AG_Nuke");
			ScriptEngine::Attach_Script(obj, "JFW_PlayAnimation_DestroyObject", "Nuke_Missle.Nuke_Missle");
			ScriptEngine::Create_3D_Sound_At_Bone("SFX.Nuclear_Strike_Buildup", obj, "ROOTTRANSFORM");
		}
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		Vector3 pos = ScriptEngine::Get_Position(obj);

		switch (type)
		{
			case 1:
				Play_Stage(obj, "XG_AG_Nukecloud", "Nuke_cloud.Nuke_cloud");
				ScriptEngine::Shake_Camera(pos, 2.0f, 0.5f, 2.0f);
				break;

			case 2:
				ScriptEngine::Shake_Camera(pos, 2.0f, 1.0f, 3.0f);
				break;

			case 3:
				Play_Stage(obj, "XG_AG_Nukecloud", "XG_Nukecloud_02");
				break;

			default:
				break;
		}
	}
};


/*JFW_PlayAnimation_DestroyObject

  Plays one animation and takes the object away at the end of it.

  Parameters:

  AnimationName		= Animation to play.
*/

DECLARE_SCRIPT_TT(JFW_PlayAnimation_DestroyObject, "AnimationName:string")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Set_Animation(obj, Get_Parameter("AnimationName"), false, nullptr, 0.0f, -1.0f, false);
	}

	void Animation_Complete(GameObject* obj, const char* /*animation_name*/) override
	{
		ScriptEngine::Destroy_Object(obj);
	}
};


/*JFW_Play_Sound_Object_Bone

  Plays a sound from the object's root, again and again if asked.

  Parameters:

  Sound_Preset		= Sound to play.
  Frequency_Min		= Shortest gap between plays, or -1 to play once.
  Frequency_Max		= Longest gap between plays.
*/

DECLARE_SCRIPT_TT(JFW_Play_Sound_Object_Bone, "Sound_Preset:string,Frequency_Min=-1.0:float,Frequency_Max:float")
{
	void Created(GameObject* obj) override
	{
		if (Get_Int_Parameter("Frequency_Min") == -1) {
			Timer_Expired(obj, 0);
		} else {
			float time = ScriptEngine::Get_Random(Get_Float_Parameter("Frequency_Min"),
					Get_Float_Parameter("Frequency_Max"));
			ScriptEngine::Start_Timer(obj, this, time, 0);
		}
	}

	void Timer_Expired(GameObject* obj, int /*number*/) override
	{
		int sound = ScriptEngine::Create_3D_Sound_At_Bone(Get_Parameter("Sound_Preset"), obj, "ROOTTRANSFORM");
		ScriptEngine::Monitor_Sound(obj, sound);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != CUSTOM_EVENT_SOUND_ENDED) {
			return;
		}

		if (Get_Int_Parameter("Frequency_Min") != -1) {
			//
			//	One sound ending can be reported more than once, and each
			//	report started another timer, so the sound sped up every time
			//	it played until the copies were on top of each other.
			//
			if (!ScriptEngine::Has_Timer(obj, this, 0)) {
				float time = ScriptEngine::Get_Random(Get_Float_Parameter("Frequency_Min"),
						Get_Float_Parameter("Frequency_Max"));
				ScriptEngine::Start_Timer(obj, this, time, 0);
			}
		}
	}
};


/*JFW_Cinematic_Primary_Killed

  Tells a cinematic that the object it was watching is gone, whether it was
  killed or simply removed, and only once either way.

  Parameters:

  CallbackID		= Cinematic to tell.
*/

DECLARE_SCRIPT_TT(JFW_Cinematic_Primary_Killed, "CallbackID=0:int")
{
	bool killed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(killed, 1);
	}

	void Created(GameObject* /*obj*/) override
	{
		killed = false;
	}

	void Report(GameObject* obj)
	{
		if (killed) {
			return;
		}

		killed = true;

		GameObject* cinematic = ScriptEngine::Find_Object(Get_Int_Parameter("CallbackID"));
		if (cinematic != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, cinematic, M00_CUSTOM_CINEMATIC_PRIMARY_KILLED, 0, 0);
		}
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override		{ Report(obj); }
	void Destroyed(GameObject* obj) override							{ Report(obj); }
};


/*JFW_Disable_Loiter

  Stops the object wandering, until something tells it it may again.
*/

DECLARE_SCRIPT_TT(JFW_Disable_Loiter, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Set_Loiters_Allowed(obj, false);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == M00_LOITER_ENABLE_TOGGLE) {
			ScriptEngine::Set_Loiters_Allowed(obj, true);
		}
	}
};


/*JFW_InnateIsStationary

  Leaves the object able to fight but unable to leave its post, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_InnateIsStationary, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Set_Innate_Is_Stationary(obj, true);
		Destroy_Script();
	}
};


/*JFW_Generic_Conv

  Runs one conversation on the object and goes away.

  Parameters:

  ConvName			= Conversation to run.
*/

DECLARE_SCRIPT_TT(JFW_Generic_Conv, "ConvName:string")
{
	void Created(GameObject* obj) override
	{
		int conversation = ScriptEngine::Create_Conversation(Get_Parameter("ConvName"), 99, 2000.0f, false);
		ScriptEngine::Join_Conversation(obj, conversation, true, true, true);
		ScriptEngine::Start_Conversation(conversation, 100000);
		ScriptEngine::Monitor_Conversation(obj, conversation);
		Destroy_Script();
	}
};


/*JFW_Disable_Hibernation

  Keeps the object thinking even when nobody is near it, then goes away.
*/

DECLARE_SCRIPT_TT(JFW_Disable_Hibernation, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		Destroy_Script();
	}
};


/******************************************************************************
*
*	The seven names that are not here
*
*	jfwws.cpp registers seven names whose script this tree already owns.  Six
*	of them the 4.8.4 file registers twice itself, under both the JFW_ name and
*	the stock name it was replacing; the seventh, JFW_BuildingStateSoundSpeaker,
*	is a copy of the stock speaker that 4.8.4 could not merge because the stock
*	script lived in a DLL it did not build.
*
*	Every one of them is registered as an alias beside the script this tree
*	already has, so a level saved against the JFW_ name still finds it, and
*	there is still one implementation of each:
*
*		JFW_PCT_Pokable					-> M00_PCT_Pokable_DAK
*		JFW_Disable_Transition			-> M00_Disable_Transition
*		JFW_GrantPowerup_Created		-> M00_GrantPowerup_Created
*		JFW_Play_Sound					-> M00_Play_Sound
*		JFW_Mobius_Script				-> Dr_Mobius_Script
*		JFW_BuildingStateSoundSpeaker	-> M00_BuildingStateSoundSpeaker
*		JFW_BuildingStateSoundController-> M00_BuildingStateSoundController
*
******************************************************************************/
