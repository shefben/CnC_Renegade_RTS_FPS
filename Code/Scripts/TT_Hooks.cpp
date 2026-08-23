/******************************************************************************
*
* FILE
*     TT_Hooks.cpp
*
* DESCRIPTION
*     The 4.8.4 library's hook scripts: the ones that are not about the object
*     they sit on, but about something happening elsewhere.
*
*     Two kinds.  Four of them listen for anything being created anywhere in
*     the level and put a script on it if it matches -- a rule about the map
*     rather than about an object.  The other ten listen for a key: the
*     jetpack, the deployable tank, the burrowing vehicle, the suicide bomber.
*
*     Native port of the library's jfwhook.cpp.  4.8.4 reached both by
*     patching the engine and fanning out to plugin DLLs; here they are
*     `ObjectCreateHookScriptClass` over `GameEventBus::ObjectCreate` and
*     `KeyHookScriptClass` over `GameEventBus::PlayerKey`.  The six jetpacks
*     are one script and a table: they differ in whether the cooldown is fixed
*     or configured, whether flying has its own budget, whether the model
*     changes, and whether the level can forbid flying outright.
*
******************************************************************************/

#include "scripts.h"
#include "combatchunkid.h"
#include "colors.h"
#include "physicalgameobj.h"
#include "wwstring.h"


////////////////////////////////////////////////////////////////////////////
//
//	Putting a script on whatever turns up
//
////////////////////////////////////////////////////////////////////////////

/*
**	The four attach-on-create scripts differ only in what they will accept.
**	Everything else -- reading the nested parameter list, the player-type
**	filter, the attach -- is the same, and was written out four times.
*/
class JFW_Attach_On_Create : public ObjectCreateHookScriptClass
{
public:

	virtual bool Accepts(GameObject* obj) = 0;

	void Object_Created(GameObject* obj) override
	{
		if (obj == nullptr || !Accepts(obj)) {
			return;
		}

		if (!Is_Player_Type(obj, Get_Int_Parameter("Player_Type"))) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);

		ScriptEngine::Attach_Script(obj, Get_Parameter("Script"), params.Peek_Buffer());
	}
};


/*JFW_Attach_Script_Preset_Created

  Everything of this preset created from now on gets the script.
*/

REGISTER_SCRIPT_TT(JFW_Attach_Script_Preset_Created,
	"Script:string,Params:string,Delim:string,Preset:string,Player_Type:int")
class JFW_Attach_Script_Preset_Created : public JFW_Attach_On_Create
{
	bool Accepts(GameObject* obj) override
	{
		return ::stricmp(ScriptEngine::Get_Preset_Name(obj), Get_Parameter("Preset")) == 0;
	}
};


/*JFW_Attach_Script_Type_Created

  The same, by class id rather than preset -- 0x3001 soldiers, 0x3010
  vehicles, and so on.  The numbers are the engine's own definition class ids,
  which is what a level was written against.
*/

REGISTER_SCRIPT_TT(JFW_Attach_Script_Type_Created,
	"Script:string,Params:string,Delim:string,Type:int,Player_Type:int")
class JFW_Attach_Script_Type_Created : public JFW_Attach_On_Create
{
	bool Accepts(GameObject* obj) override
	{
		return (int)obj->Get_Definition().Get_Class_ID() == Get_Int_Parameter("Type");
	}
};


/*JFW_Attach_Script_Player_Created

  Every player's character, as they spawn it -- which is how a level gives all
  players a behaviour without touching any preset.  Unlike the other three it
  also catches up on creation, since the players already in the level were
  created before the script was.
*/

REGISTER_SCRIPT_TT(JFW_Attach_Script_Player_Created,
	"Script:string,Params:string,Delim:string,Player_Type:int")
class JFW_Attach_Script_Player_Created : public JFW_Attach_On_Create
{
	void Created(GameObject* obj) override
	{
		ObjectCreateHookScriptClass::Created(obj);

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);

		ScriptEngine::Attach_Script_Player_Once(Get_Parameter("Script"), params.Peek_Buffer(),
			Get_Int_Parameter("Player_Type"));
	}

	bool Accepts(GameObject* obj) override
	{
		return ScriptEngine::Is_A_Star(obj) && obj->As_SoldierGameObj() != nullptr;
	}
};


/*JFW_Attach_Script_Vehicle_Created

  Every vehicle, however it comes into being -- bought, placed, or created by
  another script.
*/

REGISTER_SCRIPT_TT(JFW_Attach_Script_Vehicle_Created,
	"Script:string,Params:string,Delim:string,Player_Type:int")
class JFW_Attach_Script_Vehicle_Created : public JFW_Attach_On_Create
{
	bool Accepts(GameObject* obj) override
	{
		PhysicalGameObj* physical = obj->As_PhysicalGameObj();
		return physical != nullptr && physical->As_VehicleGameObj() != nullptr;
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	The jetpack
//
////////////////////////////////////////////////////////////////////////////

/*
**	How long a jetpack has to wait between presses.
*/
enum JetpackPacingEnum
{
	JETPACK_PACE_FIXED,			// a tenth of a second, just to debounce the key
	JETPACK_PACE_TIMED,			// the Timer parameter, both directions
	JETPACK_PACE_BUDGETED		// separate up and down timers, and a flight time
};


/*
**	What else a variant does.  These are flags because the No_Fly variants are
**	the timed and budgeted ones with one more thing bolted on.
*/
enum
{
	JETPACK_FLAG_NONE			= 0,
	JETPACK_FLAG_MODEL			= 1,	// swaps the character's model
	JETPACK_FLAG_NO_FLY			= 2,	// the level can forbid flying outright
	JETPACK_FLAG_COMPLAINS		= 4		// says so when it refuses
};


struct JetpackVariantStruct
{
	int	Pacing;
	int	Flags;
};


/*
**	The jetpack.  Pressing the key toggles fly mode and plays an animation; the
**	variants differ in how long it makes you wait, whether it tells you it is
**	making you wait, whether it swaps your model, and whether the level can
**	ground you.
**
**	Fly mode is turned off on detach whatever the reason, because a character
**	left flying with no script to land them is a character stuck in the air.
*/
class JFW_Jetpack_Base : public KeyHookScriptClass
{
public:

	bool	Enabled;		// the level's Enable/Disable customs
	bool	Grounded;		// the level's NoFly customs
	bool	CanRise;		// the up-cooldown has expired
	bool	CanLand;		// the down-cooldown has expired

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Enabled, 1);
		SAVE_VARIABLE(Grounded, 2);
		SAVE_VARIABLE(CanRise, 3);
		SAVE_VARIABLE(CanLand, 4);
	}

	virtual const JetpackVariantStruct& Variant(void) = 0;

	void Created(GameObject* obj) override
	{
		Enabled		= true;
		Grounded	= false;
		CanRise		= true;
		CanLand		= true;

		Install_Hook("Jetpack", obj);
	}

	void Detach(GameObject* obj) override
	{
		if (obj != nullptr && ScriptEngine::Get_Fly_Mode(obj)) {
			ScriptEngine::Toggle_Fly_Mode(obj);
		}

		KeyHookScriptClass::Detach(obj);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("EnableMessage")) {
			Enabled = true;
		}

		if (type == Get_Int_Parameter("DisableMessage")) {
			Enabled = false;
		}

		if ((Variant().Flags & JETPACK_FLAG_NO_FLY) == 0) {
			return;
		}

		if (type == Get_Int_Parameter("NoFlyOff")) {
			Grounded = false;
		}

		if (type == Get_Int_Parameter("NoFlyOn")) {
			Grounded = true;
			Ground_Now(obj);
		}
	}

	void Key_Hook(void) override
	{
		GameObject* obj = Owner();
		if (obj == nullptr) {
			return;
		}

		if (Grounded) {
			Complain("NoFlyMessage");
			return;
		}

		//
		//	Not from inside a vehicle: the character is a passenger, and flying
		//	them out of their seat is not what the key is for.
		//
		if (!Enabled || ScriptEngine::Get_Vehicle(obj) != nullptr) {
			return;
		}

		bool flying = ScriptEngine::Get_Fly_Mode(obj);

		if (!(flying ? CanLand : CanRise)) {
			Complain("Message");
			return;
		}

		Set_Flying(obj, !flying);
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		switch (timer_id) {

			case 1:
				CanLand = true;
				break;

			case 2:
				CanRise = true;
				break;

			case 3:
				//
				//	The flight is over.  Land whether or not the player asked.
				//
				if (ScriptEngine::Get_Fly_Mode(obj)) {
					Set_Flying(obj, false);
				}
				break;
		}
	}

	//
	//	Go up or come down, and start whatever the variant counts.
	//
	void Set_Flying(GameObject* obj, bool flying)
	{
		if (flying) {
			ScriptEngine::Set_Animation(obj, Get_Parameter("OnAnimation"), true, nullptr, 0, -1, true);

			if (Variant().Flags & JETPACK_FLAG_MODEL) {
				ScriptEngine::Set_Model(obj, Get_Parameter("OnModel"));
			}

		} else {
			//
			//	An empty name is stop-and-clear.  4.8.4 added a Clear_Animation
			//	of its own to PhysicalGameObj because the stock Set_Animation
			//	fell through an empty name and did nothing; that is merged, so
			//	the canonical call is the clear.
			//
			PhysicalGameObj* physical = obj->As_PhysicalGameObj();
			if (physical != nullptr) {
				physical->Set_Animation("", false);
			}

			if (Variant().Flags & JETPACK_FLAG_MODEL) {
				ScriptEngine::Set_Model(obj, Get_Parameter("OffModel"));
			}
		}

		ScriptEngine::Toggle_Fly_Mode(obj);

		switch (Variant().Pacing) {

			case JETPACK_PACE_FIXED:
				CanRise = false;
				CanLand = false;
				ScriptEngine::Start_Timer(obj, this, 0.1f, 1);
				ScriptEngine::Start_Timer(obj, this, 0.1f, 2);
				break;

			case JETPACK_PACE_TIMED:
			{
				float wait = Get_Float_Parameter("Timer");
				CanRise = false;
				CanLand = false;
				ScriptEngine::Start_Timer(obj, this, wait, 1);
				ScriptEngine::Start_Timer(obj, this, wait, 2);
				break;
			}

			case JETPACK_PACE_BUDGETED:
				if (flying) {
					//
					//	Up: you may not come down for TimerOff, and you must be
					//	down by TimerUse.
					//
					CanLand = false;
					ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("TimerOff"), 1);
					ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("TimerUse"), 3);
				} else {
					CanRise = false;
					ScriptEngine::Stop_Timer(obj, this, 3);
					ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("TimerOn"), 2);
				}
				break;
		}
	}

	//
	//	The level forbidding flight while somebody is in the air.
	//
	void Ground_Now(GameObject* obj)
	{
		if (obj != nullptr && ScriptEngine::Get_Fly_Mode(obj)) {
			Set_Flying(obj, false);
		}
	}

	void Complain(const char* parameter_name)
	{
		if ((Variant().Flags & JETPACK_FLAG_COMPLAINS) == 0) {
			return;
		}

		GameObject* obj = Owner();

		//
		//	In the team's own colour, which is 0..1 here and was 0..255 in the
		//	4.8.4 call this replaces.
		//
		Vector3 color = Get_Color_For_Team(ScriptEngine::Get_Player_Type(obj));

		ScriptEngine::Send_Message_Player(obj, int(color.X * 255.0f), int(color.Y * 255.0f),
			int(color.Z * 255.0f), Get_Parameter(parameter_name));
	}
};


#define	JETPACK_PARAMS_BASE		"DisableMessage:int,EnableMessage:int,OnAnimation=null.null:string"
#define	JETPACK_PARAMS_TIMED	",timer:float,Message:string"
#define	JETPACK_PARAMS_BUDGET	",TimerOff:float,TimerOn:Float,TimerUse:float,Message:string"
#define	JETPACK_PARAMS_MODEL	",OnModel:string,OffModel:string"
#define	JETPACK_PARAMS_NO_FLY	",NoFlyOn:int,NoFlyOff:int,NoFlyMessage:string"


/*JFW_Jetpack

  Press to fly, press again to land.  Nothing limits it but a tenth of a
  second's debounce.
*/

static const JetpackVariantStruct _JFW_Jetpack_Variant =
	{ JETPACK_PACE_FIXED, JETPACK_FLAG_NONE };

REGISTER_SCRIPT_TT(JFW_Jetpack, JETPACK_PARAMS_BASE)
class JFW_Jetpack : public JFW_Jetpack_Base
{
	const JetpackVariantStruct& Variant(void) override	{ return _JFW_Jetpack_Variant; }
};


/*JFW_Jetpack_Model

  The same, and the character's model changes while they are in the air.
*/

static const JetpackVariantStruct _JFW_Jetpack_Model_Variant =
	{ JETPACK_PACE_FIXED, JETPACK_FLAG_MODEL };

REGISTER_SCRIPT_TT(JFW_Jetpack_Model, JETPACK_PARAMS_BASE JETPACK_PARAMS_MODEL)
class JFW_Jetpack_Model : public JFW_Jetpack_Base
{
	const JetpackVariantStruct& Variant(void) override	{ return _JFW_Jetpack_Model_Variant; }
};


/*JFW_Jetpack_Timer

  A cooldown the level sets, and a message when the answer is no.
*/

static const JetpackVariantStruct _JFW_Jetpack_Timer_Variant =
	{ JETPACK_PACE_TIMED, JETPACK_FLAG_COMPLAINS };

REGISTER_SCRIPT_TT(JFW_Jetpack_Timer, JETPACK_PARAMS_BASE JETPACK_PARAMS_TIMED)
class JFW_Jetpack_Timer : public JFW_Jetpack_Base
{
	const JetpackVariantStruct& Variant(void) override	{ return _JFW_Jetpack_Timer_Variant; }
};


/*JFW_Jetpack_Timer_No_Fly

  The same, and the level can ground everyone with a custom -- for an indoor
  section, or a no-fly zone over a base.
*/

static const JetpackVariantStruct _JFW_Jetpack_Timer_No_Fly_Variant =
	{ JETPACK_PACE_TIMED, JETPACK_FLAG_COMPLAINS | JETPACK_FLAG_NO_FLY };

REGISTER_SCRIPT_TT(JFW_Jetpack_Timer_No_Fly,
	JETPACK_PARAMS_BASE JETPACK_PARAMS_TIMED JETPACK_PARAMS_NO_FLY)
class JFW_Jetpack_Timer_No_Fly : public JFW_Jetpack_Base
{
	const JetpackVariantStruct& Variant(void) override	{ return _JFW_Jetpack_Timer_No_Fly_Variant; }
};


/*JFW_Limited_Jetpack_Timer

  Flight has a budget: you may not land for TimerOff, you are landed at
  TimerUse whether you like it or not, and you may not take off again for
  TimerOn.  A jetpack with fuel, in other words.
*/

static const JetpackVariantStruct _JFW_Limited_Jetpack_Timer_Variant =
	{ JETPACK_PACE_BUDGETED, JETPACK_FLAG_COMPLAINS };

REGISTER_SCRIPT_TT(JFW_Limited_Jetpack_Timer, JETPACK_PARAMS_BASE JETPACK_PARAMS_BUDGET)
class JFW_Limited_Jetpack_Timer : public JFW_Jetpack_Base
{
	const JetpackVariantStruct& Variant(void) override	{ return _JFW_Limited_Jetpack_Timer_Variant; }
};


/*JFW_Limited_Jetpack_Timer_No_Fly

  The budgeted jetpack, groundable by the level.
*/

static const JetpackVariantStruct _JFW_Limited_Jetpack_Timer_No_Fly_Variant =
	{ JETPACK_PACE_BUDGETED, JETPACK_FLAG_COMPLAINS | JETPACK_FLAG_NO_FLY };

REGISTER_SCRIPT_TT(JFW_Limited_Jetpack_Timer_No_Fly,
	JETPACK_PARAMS_BASE JETPACK_PARAMS_BUDGET JETPACK_PARAMS_NO_FLY)
class JFW_Limited_Jetpack_Timer_No_Fly : public JFW_Jetpack_Base
{
	const JetpackVariantStruct& Variant(void) override
		{ return _JFW_Limited_Jetpack_Timer_No_Fly_Variant; }
};


////////////////////////////////////////////////////////////////////////////
//
//	Vehicles that do something on a key
//
////////////////////////////////////////////////////////////////////////////

/*
**	Both of the vehicles below hook the key for whoever is driving: installed
**	when somebody gets in, removed when they get out, so a passenger's press
**	and a former driver's press both do nothing.
*/
class JFW_Driver_Key_Base : public KeyHookScriptClass
{
public:

	int	PilotID;

	virtual const char* Key_Name(void) = 0;

	void On_Driver_Left(GameObject* /*obj*/, GameObject* /*driver*/)	{ }

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (sender == nullptr) {
			return;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			if (PilotID == 0) {
				Install_Hook(Key_Name(), sender);
				PilotID = ScriptEngine::Get_ID(sender);
			}

		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			if (PilotID == ScriptEngine::Get_ID(sender)) {
				Remove_Hook();
				PilotID = 0;
			}

			On_Driver_Left(obj, sender);
		}
	}
};


/*JFW_Dplbl_Vhcls_Keyboard

  A tank that deploys into something else.  Pressing the key throws the
  occupants out, creates the deployed preset at the vehicle's origin with the
  same fraction of health and armour the vehicle had, and then damages the
  vehicle to death -- which is how the swap is made to look like one object.
  The wreck explosion is suppressed for that death and only that death.
*/

REGISTER_SCRIPT_TT(JFW_Dplbl_Vhcls_Keyboard,
	"Animation_Preset:string,oldTnk_Warhead:string,oldTnk_Dammage:float,Explosion_preset:string")
class JFW_Dplbl_Vhcls_Keyboard : public JFW_Driver_Key_Base
{
	bool	CanExplode;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(PilotID, 1);
		SAVE_VARIABLE(CanExplode, 2);
	}

	const char* Key_Name(void) override	{ return "Deploy"; }

	void Created(GameObject* /*obj*/) override
	{
		PilotID		= 0;
		CanExplode	= true;
	}

	void Key_Hook(void) override
	{
		GameObject* obj = Owner();
		if (obj == nullptr) {
			return;
		}

		ScriptEngine::Force_Occupants_Exit(obj);

		GameObject* deployed =
			ScriptEngine::Create_Object(Get_Parameter("Animation_Preset"), Vector3(0.0f, 0.0f, 0.0f));

		//
		//	Carry the damage across as a fraction rather than a number: the two
		//	presets need not have the same maximum.
		//
		if (deployed != nullptr) {
			Carry_Over(obj, deployed, false);
			Carry_Over(obj, deployed, true);

			ScriptEngine::Attach_To_Object_Bone(deployed, obj, "origin");
		}

		CanExplode = false;
		ScriptEngine::Apply_Damage(obj, Get_Float_Parameter("oldTnk_Dammage"),
			Get_Parameter("oldTnk_Warhead"), nullptr);
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		if (CanExplode) {
			ScriptEngine::Create_Explosion(Get_Parameter("Explosion_preset"),
				ScriptEngine::Get_Bone_Position(obj, "origin"), nullptr);
		}
	}

	void Carry_Over(GameObject* from, GameObject* to, bool shield)
	{
		float max_from = shield ? ScriptEngine::Get_Max_Shield_Strength(from)
								: ScriptEngine::Get_Max_Health(from);
		if (max_from == 0.0f) {
			return;
		}

		float fraction = (shield ? ScriptEngine::Get_Shield_Strength(from)
								 : ScriptEngine::Get_Health(from)) / max_from;

		if (shield) {
			ScriptEngine::Set_Shield_Strength(to,
				float(int(ScriptEngine::Get_Max_Shield_Strength(to) * fraction)));
		} else {
			ScriptEngine::Set_Health(to, float(int(ScriptEngine::Get_Max_Health(to) * fraction)));
		}
	}
};


/*JFW_Underground_Logic

  A vehicle that burrows.  Pressing the key drops it below the ground and
  fades its occupants' screens to the dig colour; pressing again brings it up.
  Being underground is not cover: getting out while buried kills the occupant,
  and the vehicle dies with them.  The level can ask, with a custom, for a
  marker to be dropped above wherever it currently is.

  One correction: the dig colour's green and blue were handed over the wrong
  way round.
*/

REGISTER_SCRIPT_TT(JFW_Underground_Logic,
	"UpZOffset:float,DownZOffset:float,DigEffectObj:string,SurfaceEffectObj:string,"
	"DisableMessage:int,EnableMessage:int,IndicatorMessage:int,IndicatorObject:string,"
	"IndicatorZOffset:float,DigRed:float,DigGreen:float,DigBlue:float,DigOpacity:float,"
	"DigSound:string")
class JFW_Underground_Logic : public JFW_Driver_Key_Base
{
	bool	Underground;
	bool	Enabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Underground, 1);
		SAVE_VARIABLE(Enabled, 2);
		SAVE_VARIABLE(PilotID, 3);
	}

	const char* Key_Name(void) override	{ return "Dig"; }

	void Created(GameObject* /*obj*/) override
	{
		Underground	= false;
		Enabled		= true;
		PilotID		= 0;
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* sender) override
	{
		JFW_Driver_Key_Base::Custom(obj, type, param, sender);

		if (type == Get_Int_Parameter("EnableMessage")) {
			Enabled = true;
		}

		if (type == Get_Int_Parameter("DisableMessage")) {
			Enabled = false;
		}

		//
		//	The level asking where it is.  Answered only while buried, since
		//	above ground it can be seen.
		//
		if (type == Get_Int_Parameter("IndicatorMessage") && Underground) {
			Vector3 position = ScriptEngine::Get_Position(obj);
			position.Z += Get_Float_Parameter("IndicatorZOffset");
			ScriptEngine::Create_Object(Get_Parameter("IndicatorObject"), position);
		}
	}

	//
	//	Getting out of a buried vehicle is fatal to both.
	//
	void On_Driver_Left(GameObject* obj, GameObject* driver)
	{
		if (!Underground) {
			return;
		}

		ScriptEngine::Set_Screen_Fade_Color_Player(driver, 0.0f, 0.0f, 0.0f, 0.0f);
		ScriptEngine::Set_Screen_Fade_Opacity_Player(driver, 0.0f, 0.0f);
		ScriptEngine::Apply_Damage(driver, 100.0f, "Death", nullptr);

		ScriptEngine::Apply_Damage(obj, 100.0f, "Death", nullptr);
	}

	void Key_Hook(void) override
	{
		GameObject* obj = Owner();
		if (obj == nullptr || !Enabled) {
			return;
		}

		Vector3 position = ScriptEngine::Get_Position(obj);

		if (Underground) {
			Underground = false;
			ScriptEngine::Set_Occupants_Fade(obj, 0.0f, 0.0f, 0.0f, 0.0f);

			position.Z -= Get_Float_Parameter("UpZOffset");
			ScriptEngine::Create_Sound(Get_Parameter("DigSound"), position, obj);
			ScriptEngine::Create_Object(Get_Parameter("SurfaceEffectObj"), position);
			ScriptEngine::Set_Position(obj, position);

		} else {
			Underground = true;
			ScriptEngine::Set_Occupants_Fade(obj,
				Get_Float_Parameter("DigRed"), Get_Float_Parameter("DigGreen"),
				Get_Float_Parameter("DigBlue"), Get_Float_Parameter("DigOpacity"));

			ScriptEngine::Create_Sound(Get_Parameter("DigSound"), position, obj);
			ScriptEngine::Create_Object(Get_Parameter("DigEffectObj"), position);

			position.Z += Get_Float_Parameter("DownZOffset");
			ScriptEngine::Set_Position(obj, position);
		}
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		if (Underground) {
			ScriptEngine::Set_Occupants_Fade(obj, 0.0f, 0.0f, 0.0f, 0.0f);
			ScriptEngine::Kill_Occupants(obj);
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	The rest
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Suicide_Bomber

  Press the key and take the explosion with you.
*/

REGISTER_SCRIPT_TT(JFW_Suicide_Bomber, "Explosion:string")
class JFW_Suicide_Bomber : public KeyHookScriptClass
{
	void Created(GameObject* obj) override
	{
		Install_Hook("BlowUp", obj);
	}

	void Key_Hook(void) override
	{
		GameObject* obj = Owner();
		if (obj == nullptr) {
			return;
		}

		ScriptEngine::Create_Explosion(Get_Parameter("Explosion"),
			ScriptEngine::Get_Position(obj), obj);
		ScriptEngine::Apply_Damage(obj, 99999.0f, "Death", nullptr);
	}
};


/*JFW_Sidebar_Key_2

  Registered and does nothing, which is what it did in 4.8.4: the class there
  is empty, and the parameters describe a feature that was taken out and left
  its registration behind.  It is here so that a level carrying the script
  still loads, and it is kept empty so that nothing is invented for it.
*/

DECLARE_SCRIPT_TT(JFW_Sidebar_Key_2,
	"Key=Sidebar:string,Enable_Custom=0:int,Disable_Custom=0:int,Sound:string")
{
};
