/******************************************************************************
*
* FILE
*     TT_Defenses.cpp
*
* DESCRIPTION
*     The base defences of the 4.8.4 library, and the weapon-swapping scripts
*     that came in the same file.
*
*     A defence here is not one object.  It is a building that a level places
*     and, hanging off it, one or more invisible weapon objects the building
*     creates at fixed offsets and then drives by custom message: the turret's
*     three idle-sweep markers, the obelisk's beam emitter, the Advanced Guard
*     Tower's missile pod and its four ceiling guns.  The building watches its
*     own power and health once a second and tells the weapons whether they
*     are allowed to shoot.  The weapons repair themselves on any damage, so
*     shooting the emitter does nothing and the building is the only target.
*
*     Native port of the library's jfwweap.cpp.  The two Advanced Guard Towers
*     there are the same script written out twice -- one with the offsets
*     compiled in, one reading them from parameters -- so here they are one
*     script and a virtual, and the four guns are an array rather than four
*     names repeated through every handler.
*
*     The last two of those scripts are the file's key hooks.  A key hook is a
*     key the level gives a meaning to and the engine has none for; see
*     Code/Combat/scriptkeys.h for how a press gets from a client to them.
*
*     After them comes the second donor this file carries: jfwdef.cpp, the
*     generic base defence, which the 4.8.4 library registers forty-eight
*     times.  See the note above that section for the five things the
*     forty-eight differ in.  It lives here rather than in a file of its own
*     because a file named for defences already existed, and two of them
*     spelled a letter apart would be a trap.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "actionparams.h"
#include "physicalgameobj.h"
#include "smartgameobj.h"
#include "vehicle.h"
#include "wwstring.h"


/*
**	Action ids and timer ids the scripts in this file use among themselves.
*/
enum
{
	JFW_ACTION_ATTACK				= 1,
	JFW_ACTION_PATROL				= 777
};


////////////////////////////////////////////////////////////////////////////
//
//	The Nod turret
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Nod_Turret

  The C&C-mode Nod turret.  Left alone it sweeps: three invisible markers are
  dropped around it at creation and every ten seconds it picks one at random
  and "attacks" it, which turns the barrel without firing anything, because a
  marker is not a valid target.  A real enemy interrupts that, is shot at for
  two seconds, and then the turret goes back to sweeping.  Dying leaves the
  wreck preset behind, facing the way the turret faced.
*/

DECLARE_SCRIPT_TT(JFW_Nod_Turret, "")
{
	int MarkerID[3];

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(MarkerID, 1);
	}

	void Created(GameObject* obj) override
	{
		ScriptEngine::Enable_Hibernation(obj, false);
		ScriptEngine::Innate_Enable(obj);
		ScriptEngine::Enable_Enemy_Seen(obj, true);

		//
		//	The three markers sit low and to the sides, so sweeping between
		//	them swings the barrel across the turret's front.
		//
		static const Vector3 _offsets[3] =
		{
			Vector3(-10.0f, -10.0f, 2.0f),
			Vector3( 10.0f,   0.0f, 2.0f),
			Vector3( 10.0f, -10.0f, 2.0f)
		};

		Vector3 position = ScriptEngine::Get_Position(obj);

		for (int index = 0; index < 3; index++) {
			MarkerID[index] = 0;

			GameObject* marker =
				ScriptEngine::Create_Object("Invisible_Object", position + _offsets[index]);
			if (marker != nullptr) {
				MarkerID[index] = ScriptEngine::Get_ID(marker);
			}
		}

		ScriptEngine::Start_Timer(obj, this, 10.0f, JFW_ACTION_ATTACK);
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		float facing = ScriptEngine::Get_Facing(obj);

		GameObject* wreck =
			ScriptEngine::Create_Object("Nod_Turret_Destroyed", ScriptEngine::Get_Position(obj));
		if (wreck != nullptr) {
			ScriptEngine::Set_Facing(wreck, facing);
		}
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		//
		//	Aim slightly above the enemy's origin, which is at its feet.
		//
		Vector3 target = ScriptEngine::Get_Position(enemy);
		target.Z += 0.5f;

		ActionParamsStruct params;
		params.Set_Basic(this, 100, 2);
		params.Set_Attack(target, 300.0f, 0.0f, true);
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Attack(obj, params);

		ScriptEngine::Start_Timer(obj, this, 2.0f, 2);
	}

	void Action_Complete(GameObject* obj, int action_id, ActionCompleteReason /*reason*/) override
	{
		if (action_id == 2) {
			ScriptEngine::Action_Reset(obj, 100);
		}
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		switch (timer_id) {

			case JFW_ACTION_ATTACK:
			{
				GameObject* marker =
					ScriptEngine::Find_Object(MarkerID[ScriptEngine::Get_Random_Int(0, 2)]);
				if (marker != nullptr) {
					ActionParamsStruct params;
					params.Set_Basic(this, 70, JFW_ACTION_ATTACK);
					params.Set_Attack(marker, 0.0f, 0.0f, true);
					ScriptEngine::Action_Attack(obj, params);
				}

				ScriptEngine::Start_Timer(obj, this, 10.0f, JFW_ACTION_ATTACK);
				break;
			}

			case 2:
				ScriptEngine::Action_Reset(obj, 100);
				break;
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	The obelisk
//
////////////////////////////////////////////////////////////////////////////

/*
**	What the building tells its beam, and what the beam tells itself.
*/
enum
{
	JFW_OBELISK_POWER			= 1,	// param: 1 armed, 0 shut down
	JFW_OBELISK_TARGET			= 2,	// param: the id of something worth shooting
	JFW_OBELISK_DESTROYED		= 3		// the building has been killed
};

#define	JFW_OBELISK_ANIMATION	"OBL_POWERUP.OBL_POWERUP"


/*JFW_Nod_Obelisk_CnC

  Sits on the obelisk building.  Creates the beam emitter forty-five metres up
  and tells it, once a second, whether the building still has power and health.
  Controller_ID names an object that wants to know the emitter's id -- a level's
  own logic, usually -- and gets it as custom 9033.
*/

DECLARE_SCRIPT_TT(JFW_Nod_Obelisk_CnC, "Controller_ID=0:int")
{
	int WeaponID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(WeaponID, 1);
	}

	void Created(GameObject* obj) override
	{
		WeaponID = 0;

		Vector3 position = ScriptEngine::Get_Position(obj);
		position.Z += 45.0f;

		GameObject* weapon = ScriptEngine::Create_Object("Nod_Obelisk", position);
		if (weapon == nullptr) {
			return;
		}

		ScriptEngine::Attach_Script(weapon, "JFW_Obelisk_Weapon_CnC", "0");
		WeaponID = ScriptEngine::Get_ID(weapon);

		ScriptEngine::Start_Timer(obj, this, 1.0f, 1);

		GameObject* controller = ScriptEngine::Find_Object(Get_Int_Parameter("Controller_ID"));
		if (controller != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, controller, 9033, WeaponID, 0);
		}
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		GameObject* weapon = ScriptEngine::Find_Object(WeaponID);
		if (weapon != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, weapon, JFW_OBELISK_DESTROYED, 0, 0);
		}
	}

	void Custom(GameObject* /*obj*/, int type, intptr_t param, GameObject* /*sender*/) override
	{
		//
		//	Somebody -- the level, typically -- asking for the emitter to go
		//	away rather than merely stop firing.
		//
		if (type == JFW_OBELISK_POWER && param == 1) {
			GameObject* weapon = ScriptEngine::Find_Object(WeaponID);
			if (weapon != nullptr) {
				ScriptEngine::Destroy_Object(weapon);
			}
		}
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		if (timer_id != 1) {
			return;
		}

		GameObject* weapon = ScriptEngine::Find_Object(WeaponID);
		if (weapon != nullptr) {
			bool armed = ScriptEngine::Get_Building_Power(obj)
					&& ScriptEngine::Get_Health(obj) != 0.0f;

			ScriptEngine::Send_Custom_Event(obj, weapon, JFW_OBELISK_POWER, armed ? 1 : 0, 0);
		}

		ScriptEngine::Start_Timer(obj, this, 1.0f, 1);
	}
};


/*JFW_Obelisk_Weapon_CnC

  The emitter itself: invisible, unkillable, and Nod's whatever it is standing
  on.  Seeing an enemy does not fire at it.  It charges first -- the powerup
  animation goes to its second frame, the warm-up sound plays under the target,
  and two seconds later the beam is loosed, which is what the C&C obelisk does.
  A target closer than fifteen metres in plan, or further than a hundred and
  fifty away, is not worth charging for and the charge is abandoned.

  Losing power abandons a charge in progress and stops it seeing anything.  The
  building being killed sets `Finished`, after which nothing restarts it.

  4.8.4 answered a loss of power by ordering the emitter to attack itself and
  resetting at the same priority in the next statement, which cancels it.  Only
  the reset does anything, so only the reset is here.
*/

DECLARE_SCRIPT_TT(JFW_Obelisk_Weapon_CnC, "")
{
	bool	Watching;			// will act on what it sees
	bool	Finished;			// the building is gone; stay dark
	int		TargetID;			// what the current charge is for
	int		EffectID;			// the powerup animation object
	float	Range;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Watching, 1);
		SAVE_VARIABLE(TargetID, 2);
		SAVE_VARIABLE(EffectID, 3);
		SAVE_VARIABLE(Range, 4);
		SAVE_VARIABLE(Finished, 5);
	}

	void Created(GameObject* obj) override
	{
		Watching	= true;
		Finished	= false;
		TargetID	= 0;
		EffectID	= 0;
		Range		= 150.0f;

		ScriptEngine::Set_Player_Type(obj, 0);
		ScriptEngine::Set_Is_Rendered(obj, false);
		ScriptEngine::Enable_Enemy_Seen(obj, true);
		ScriptEngine::Enable_Hibernation(obj, false);
		ScriptEngine::Innate_Enable(obj);

		GameObject* effect =
			ScriptEngine::Create_Object("Obelisk Effect", ScriptEngine::Get_Position(obj));
		if (effect != nullptr) {
			EffectID = ScriptEngine::Get_ID(effect);
			ScriptEngine::Set_Animation_Frame(effect, JFW_OBELISK_ANIMATION, 0);
		}
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float /*amount*/) override
	{
		//
		//	Not a target.  The building is.
		//
		ScriptEngine::Set_Health(obj, ScriptEngine::Get_Max_Health(obj));
		ScriptEngine::Set_Shield_Strength(obj, ScriptEngine::Get_Max_Shield_Strength(obj));
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		if (!Finished) {
			//
			//	Round-trip through a custom rather than acting here, so that a
			//	sighting and the building's own "here is a target" arrive down
			//	the same path.
			//
			ScriptEngine::Send_Custom_Event(obj, obj, JFW_OBELISK_TARGET,
					ScriptEngine::Get_ID(enemy), 0);
		}
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* /*sender*/) override
	{
		switch (type) {

			case JFW_OBELISK_POWER:
				if (param == 0) {
					ScriptEngine::Enable_Enemy_Seen(obj, false);
					Watching = false;
					Stand_Down(obj);
				} else {
					ScriptEngine::Enable_Enemy_Seen(obj, true);
					Watching = true;
				}
				break;

			case JFW_OBELISK_TARGET:
			{
				if (Finished || !Watching) {
					break;
				}

				GameObject* target = ScriptEngine::Find_Object((int)param);
				if (target == nullptr) {
					break;
				}

				if (!Worth_Charging_For(obj, target)) {
					Stand_Down(obj);
					break;
				}

				TargetID = (int)param;
				Watching = false;

				Set_Effect_Frame(1);
				ScriptEngine::Start_Timer(obj, this, 2.0f, 1);

				//
				//	The warm-up is heard from under the target rather than at
				//	the top of the obelisk, which is where the beam lands.
				//
				Vector3 position = ScriptEngine::Get_Position(target);
				position.Z -= 20.0f;
				ScriptEngine::Create_Sound("Obelisk_Warm_Up", position, obj);
				break;
			}

			case JFW_OBELISK_DESTROYED:
				Finished = true;
				Stand_Down(obj);
				break;
		}
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		if (Finished) {
			return;
		}

		if (timer_id == 1) {
			//
			//	The charge is done.  Fire, unless what it was charging for has
			//	moved out of range or died in the meantime.
			//
			GameObject* target = ScriptEngine::Find_Object(TargetID);

			if (target != nullptr && Worth_Charging_For(obj, target)) {
				ActionParamsStruct params;
				params.Set_Basic(this, 100, 0);
				params.Set_Attack(target, Range, 0.0f, true);
				ScriptEngine::Action_Attack(obj, params);

				TargetID = 0;
				ScriptEngine::Start_Timer(obj, this, 2.0f, 2);
			} else {
				Stand_Down(obj);
				Watching = true;
			}

		} else if (timer_id == 2) {
			//
			//	The beam has been loosed; drop the animation and start looking
			//	again.
			//
			Set_Effect_Frame(0);
			Watching = true;
		}
	}

	//
	//	Close enough that the beam would have to fire almost straight down, or
	//	further than it reaches.  Distance is measured in three dimensions
	//	against the range and in plan against the minimum, because the emitter
	//	sits forty-five metres above what it is defending.
	//
	bool Worth_Charging_For(GameObject* obj, GameObject* target)
	{
		Vector3 here	= ScriptEngine::Get_Position(obj);
		Vector3 there	= ScriptEngine::Get_Position(target);

		float distance = ScriptEngine::Get_Distance(here, there);

		here.Z	= 0.0f;
		there.Z	= 0.0f;

		return ScriptEngine::Get_Distance(here, there) > 15.0f && distance <= Range;
	}

	void Set_Effect_Frame(int frame)
	{
		GameObject* effect = ScriptEngine::Find_Object(EffectID);
		if (effect != nullptr) {
			ScriptEngine::Set_Animation_Frame(effect, JFW_OBELISK_ANIMATION, frame);
		}
	}

	void Stand_Down(GameObject* obj)
	{
		Set_Effect_Frame(0);
		ScriptEngine::Action_Reset(obj, 100);
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	The Advanced Guard Tower
//
////////////////////////////////////////////////////////////////////////////

/*
**	What the tower tells its guns.
*/
enum
{
	JFW_AGT_MISSILE_POD		= 1,	// param: the id to report sightings to
	JFW_AGT_ENABLE			= 2,	// param: 1 may shoot, 0 may not
	JFW_AGT_DESTROYED		= 3		// the tower has been killed
};


/*
**	The tower proper.  Both registered towers are this script; they differ only
**	in where they put the five weapon objects, which is the one virtual.  The
**	fixed offsets in JFW_Advanced_Guard_Tower are measured against the stock
**	GDI AGT model.
*/
class JFW_Advanced_Guard_Tower_Base : public ScriptImpClass
{
public:

	enum { GUN_COUNT = 4 };

	int		MissileID;
	int		GunID[GUN_COUNT];
	bool	Enable;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(MissileID, 1);
		SAVE_VARIABLE(GunID, 2);
		SAVE_VARIABLE(Enable, 3);
	}

	//	offsets[0] is the missile pod, offsets[1..4] the ceiling guns.
	virtual void Get_Offsets(Vector3 offsets[GUN_COUNT + 1]) = 0;

	void Created(GameObject* obj) override
	{
		Enable		= true;
		MissileID	= 0;

		ScriptEngine::Enable_Hibernation(obj, false);

		Vector3 offsets[GUN_COUNT + 1];
		Get_Offsets(offsets);

		Vector3 position = ScriptEngine::Get_Position(obj);

		GameObject* missile = ScriptEngine::Create_Object("GDI_AGT", position + offsets[0]);
		if (missile != nullptr) {
			ScriptEngine::Attach_Script(missile, "JFW_Advanced_Guard_Tower_Missile", "0");
			MissileID = ScriptEngine::Get_ID(missile);
		}

		for (int index = 0; index < GUN_COUNT; index++) {
			GunID[index] = 0;

			GameObject* gun =
				ScriptEngine::Create_Object("GDI_Ceiling_Gun_AGT", position + offsets[index + 1]);
			if (gun != nullptr) {
				ScriptEngine::Attach_Script(gun, "JFW_Advanced_Guard_Tower_Gun", "0");
				GunID[index] = ScriptEngine::Get_ID(gun);
			}
		}

		ScriptEngine::Start_Timer(obj, this, 1.0f, 1);
		ScriptEngine::Start_Timer(obj, this, 1.0f, 2);
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		Tell_Guns(obj, JFW_AGT_DESTROYED, 0);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		//
		//	The level switching the tower off and on.  Custom 0 is off, 1 is on;
		//	the answer only holds while the tower has power and health, because
		//	the once-a-second check below overrides it.
		//
		if (type == 0) {
			Enable = false;
			Tell_Guns(obj, JFW_AGT_ENABLE, 0);
		} else if (type == 1) {
			Enable = true;
			Tell_Guns(obj, JFW_AGT_ENABLE, 1);
		}
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		switch (timer_id) {

			case 1:
				//
				//	Told once, a second after creation: this is the object to
				//	report sightings to.  The guns spot for the missile pod,
				//	which has no eyes of its own.
				//
				Tell_Guns(obj, JFW_AGT_MISSILE_POD, MissileID);
				break;

			case 2:
			{
				bool live = ScriptEngine::Get_Building_Power(obj)
						&& ScriptEngine::Get_Health(obj) != 0.0f;

				if (!live) {
					Tell_Guns(obj, JFW_AGT_ENABLE, 0);
				} else if (Enable) {
					Tell_Guns(obj, JFW_AGT_ENABLE, 1);
				}

				ScriptEngine::Start_Timer(obj, this, 1.0f, 2);
				break;
			}
		}
	}

	void Tell_Guns(GameObject* obj, int type, int param)
	{
		for (int index = 0; index < GUN_COUNT; index++) {
			GameObject* gun = ScriptEngine::Find_Object(GunID[index]);
			if (gun != nullptr) {
				ScriptEngine::Send_Custom_Event(obj, gun, type, param, 0);
			}
		}
	}
};


REGISTER_SCRIPT_TT(JFW_Advanced_Guard_Tower, "")
class JFW_Advanced_Guard_Tower : public JFW_Advanced_Guard_Tower_Base
{
	void Get_Offsets(Vector3 offsets[GUN_COUNT + 1]) override
	{
		offsets[0] = Vector3( 0.0000000f,  0.0000000f,  6.0f);
		offsets[1] = Vector3( 5.1789999f, -4.2389998f, -9.0f);
		offsets[2] = Vector3( 5.1609998f,  3.2720001f, -9.0f);
		offsets[3] = Vector3(-4.4910002f,  3.2570000f, -9.0f);
		offsets[4] = Vector3(-5.1329999f, -4.3660002f, -9.0f);
	}
};


REGISTER_SCRIPT_TT(JFW_Advanced_Guard_Tower_2,
	"Gun1Offset:vector3,Gun2Offset:vector3,Gun3Offset:vector3,Gun4Offset:vector3,"
	"MissileOffset:vector3")
class JFW_Advanced_Guard_Tower_2 : public JFW_Advanced_Guard_Tower_Base
{
	void Get_Offsets(Vector3 offsets[GUN_COUNT + 1]) override
	{
		offsets[0] = Get_Vector3_Parameter("MissileOffset");
		offsets[1] = Get_Vector3_Parameter("Gun1Offset");
		offsets[2] = Get_Vector3_Parameter("Gun2Offset");
		offsets[3] = Get_Vector3_Parameter("Gun3Offset");
		offsets[4] = Get_Vector3_Parameter("Gun4Offset");
	}
};


/*JFW_Advanced_Guard_Tower_Missile

  The missile pod.  Blind: it fires only at what a gun tells it about, and only
  at something more than thirty metres away, which is what keeps the tower from
  missiling its own doorstep.  One missile a second at most.
*/

DECLARE_SCRIPT_TT(JFW_Advanced_Guard_Tower_Missile, "")
{
	bool Firing;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Firing, 1);
	}

	void Created(GameObject* obj) override
	{
		Firing = false;
		ScriptEngine::Set_Is_Rendered(obj, false);
		ScriptEngine::Enable_Hibernation(obj, false);
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float /*amount*/) override
	{
		ScriptEngine::Set_Health(obj, ScriptEngine::Get_Max_Health(obj));
		ScriptEngine::Set_Shield_Strength(obj, ScriptEngine::Get_Max_Shield_Strength(obj));
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* /*sender*/) override
	{
		if (type != JFW_AGT_MISSILE_POD || Firing) {
			return;
		}

		GameObject* target = ScriptEngine::Find_Object((int)param);
		if (target == nullptr) {
			return;
		}

		if (ScriptEngine::Get_Distance(ScriptEngine::Get_Position(obj),
				ScriptEngine::Get_Position(target)) <= 30.0f) {
			return;
		}

		Firing = true;

		ActionParamsStruct params;
		params.Set_Basic(this, 100, JFW_ACTION_ATTACK);
		params.Set_Attack(target, 300.0f, 0.0f, true);
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Attack(obj, params);

		ScriptEngine::Start_Timer(obj, this, 1.0f, 1);
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		if (timer_id == 1) {
			ScriptEngine::Action_Reset(obj, 100);
			Firing = false;
		}
	}
};


/*JFW_Advanced_Guard_Tower_Gun

  One of the four ceiling guns.  Wears the Blamo shield so nothing hurts it,
  reports everything it sees to the missile pod, and shoots at anything more
  than twenty metres off for ten seconds.  Its own range is not checked; the
  tower does that by switching it off.
*/

DECLARE_SCRIPT_TT(JFW_Advanced_Guard_Tower_Gun, "")
{
	int		MissileID;
	bool	Disabled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(MissileID, 1);
		SAVE_VARIABLE(Disabled, 2);
	}

	void Created(GameObject* obj) override
	{
		MissileID	= 0;
		Disabled	= false;

		ScriptEngine::Set_Shield_Type(obj, "Blamo");
		ScriptEngine::Enable_Hibernation(obj, false);
		ScriptEngine::Innate_Enable(obj);
		ScriptEngine::Enable_Enemy_Seen(obj, true);
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float /*amount*/) override
	{
		ScriptEngine::Set_Health(obj, ScriptEngine::Get_Max_Health(obj));
		ScriptEngine::Set_Shield_Strength(obj, ScriptEngine::Get_Max_Shield_Strength(obj));
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* /*sender*/) override
	{
		switch (type) {

			case JFW_AGT_MISSILE_POD:
				MissileID = (int)param;
				break;

			case JFW_AGT_ENABLE:
				ScriptEngine::Enable_Enemy_Seen(obj, param != 0);
				break;

			case JFW_AGT_DESTROYED:
				Disabled = true;
				ScriptEngine::Action_Reset(obj, 100);
				break;
		}
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		if (Disabled) {
			return;
		}

		GameObject* missile = ScriptEngine::Find_Object(MissileID);
		if (missile != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, missile, JFW_AGT_MISSILE_POD,
					ScriptEngine::Get_ID(enemy), 0);
		}

		if (ScriptEngine::Get_Distance(ScriptEngine::Get_Position(obj),
				ScriptEngine::Get_Position(enemy)) > 20.0f) {
			ActionParamsStruct params;
			params.Set_Basic(this, 100, JFW_ACTION_ATTACK);
			params.Set_Attack(enemy, 300.0f, 0.0f, true);
			params.AttackCheckBlocked = false;
			ScriptEngine::Action_Attack(obj, params);

			ScriptEngine::Start_Timer(obj, this, 10.0f, 1);
		}
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		if (timer_id == 1) {
			ScriptEngine::Action_Reset(obj, 100);
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Guard duty
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Guard_Duty_Helper

  A pair of eyes on a stick.  It is created at a bone on a guard and does
  nothing but forward what it sees, because the guard itself is walking a
  waypath with its attack switched off and would not notice.
*/

DECLARE_SCRIPT_TT(JFW_Guard_Duty_Helper, "ID:int,Helper_Msg:int")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Set_Is_Rendered(obj, false);
		ScriptEngine::Innate_Enable(obj);
		ScriptEngine::Enable_Enemy_Seen(obj, true);
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		GameObject* guard = ScriptEngine::Find_Object(Get_Int_Parameter("ID"));
		if (guard != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, guard, Get_Int_Parameter("Helper_Msg"),
					ScriptEngine::Get_ID(enemy), 0);
		}
	}
};


/*JFW_Guard_Duty

  A soldier who walks a waypath with an attack held ready but inactive, so that
  arriving somewhere never becomes a reason to stop.  The helper on his shoulder
  tells him when something is worth turning on for; he fights for Attack_Time,
  goes back to walking, and after Wait_Time the helper starts reporting again.
  Dummy_Object is a fixed thing to nominally attack -- the action needs a
  target to exist while nothing real is in front of him.

  Two corrections.  The helper's id was recorded as the guard's own, so the
  two calls that quiet the helper down during a fight and wake it up afterwards
  were both aimed at the guard, where enemy-seen was never on: the helper never
  stopped reporting.  And the helper was attached to a bone without checking it
  had been created.  What the helper is told -- the guard's id and the message
  number -- is unchanged; that part was right.
*/

DECLARE_SCRIPT_TT(JFW_Guard_Duty,
	"Helper_Obj:string,Helper_Bone:string,Range:float,Dummy_Object:int,"
	"Helper_Msg:int,Waypathid:int,Attack_Time:float,Wait_Time:float")
{
	int HelperID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(HelperID, 1);
	}

	void Created(GameObject* obj) override
	{
		HelperID = 0;

		const char* bone = Get_Parameter("Helper_Bone");

		GameObject* helper =
			ScriptEngine::Create_Object_At_Bone(obj, Get_Parameter("Helper_Obj"), bone);
		if (helper != nullptr) {
			ScriptEngine::Attach_To_Object_Bone(helper, obj, bone);
			HelperID = ScriptEngine::Get_ID(helper);

			StringClass params;
			params.Format("%d,%d", ScriptEngine::Get_ID(obj), Get_Int_Parameter("Helper_Msg"));
			ScriptEngine::Attach_Script(helper, "JFW_Guard_Duty_Helper", params.Peek_Buffer());
		}

		Patrol(obj, false);
	}

	void Action_Complete(GameObject* obj, int action_id, ActionCompleteReason /*reason*/) override
	{
		if (action_id == JFW_ACTION_PATROL) {
			Patrol(obj, false);
		}
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Helper_Msg")) {
			return;
		}

		float range = Get_Float_Parameter("Range");

		ActionParamsStruct params;
		params.Set_Basic(this, 100, JFW_ACTION_PATROL);
		params.MoveArrivedDistance = 100.0f;
		params.Set_Movement((GameObject*)nullptr, 0.1f, range);
		params.Set_Attack(ScriptEngine::Find_Object((int)param), range, 0.5f, true);
		params.WaypathID = Get_Int_Parameter("Waypathid");
		params.WaypathSplined = true;
		params.AttackActive = true;
		params.AttackCheckBlocked = false;
		ScriptEngine::Modify_Action(obj, JFW_ACTION_PATROL, params, true, true);

		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Attack_Time"), 1);

		Set_Helper_Eyes(false);
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		switch (timer_id) {

			case 1:
				//
				//	Done fighting.  Back to the waypath, and start counting
				//	down to letting the helper speak again.
				//
				Patrol(obj, true);
				ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Wait_Time"), 2);
				break;

			case 2:
				Set_Helper_Eyes(true);
				break;
		}
	}

	//
	//	Walk the waypath, attack inactive.  `modify` is the difference between
	//	starting the action and steering the one already running.
	//
	void Patrol(GameObject* obj, bool modify)
	{
		float range = Get_Float_Parameter("Range");

		ActionParamsStruct params;
		params.MoveArrivedDistance = 100.0f;
		params.Set_Basic(this, 100, JFW_ACTION_PATROL);
		params.Set_Attack(ScriptEngine::Find_Object(Get_Int_Parameter("Dummy_Object")),
				range, 0.5f, true);
		params.WaypathID = Get_Int_Parameter("Waypathid");
		params.WaypathSplined = true;
		params.AttackActive = false;
		params.AttackCheckBlocked = false;

		if (modify) {
			ScriptEngine::Modify_Action(obj, JFW_ACTION_PATROL, params, true, true);
		} else {
			ScriptEngine::Action_Attack(obj, params);
		}
	}

	void Set_Helper_Eyes(bool enable)
	{
		GameObject* helper = ScriptEngine::Find_Object(HelperID);
		if (helper != nullptr) {
			ScriptEngine::Enable_Enemy_Seen(helper, enable);
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Hunting
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Hunt_Attack

  Attacks whatever it sees, from Attack_Range, and resets when the attack ends
  so that the next sighting starts clean.
*/

DECLARE_SCRIPT_TT(JFW_Hunt_Attack, "Attack_Range:float")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Innate_Enable(obj);
		ScriptEngine::Enable_Enemy_Seen(obj, true);
		ScriptEngine::Enable_Hibernation(obj, false);
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		ActionParamsStruct params;
		params.MoveArrivedDistance = 100.0f;
		params.Set_Basic(this, 100, JFW_ACTION_ATTACK);
		params.Set_Attack(enemy, Get_Float_Parameter("Attack_Range"), 0.0f, true);
		ScriptEngine::Action_Attack(obj, params);
	}

	void Action_Complete(GameObject* obj, int /*action_id*/, ActionCompleteReason /*reason*/) override
	{
		ScriptEngine::Action_Reset(obj, 100);
	}
};


/*JFW_Hunt_Blow_Up

  A suicide runner.  Follows whoever it sees at Speed until it is within
  Distance, then explodes.  The first completed action is swallowed: attaching
  the script completes whatever the object was already doing, and blowing up on
  the spot at creation is not the idea.
*/

DECLARE_SCRIPT_TT(JFW_Hunt_Blow_Up, "Speed:float,Distance:float,Explosion:string")
{
	bool FirstTime;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(FirstTime, 1);
	}

	void Created(GameObject* obj) override
	{
		FirstTime = true;
		ScriptEngine::Innate_Enable(obj);
		ScriptEngine::Enable_Enemy_Seen(obj, true);
		ScriptEngine::Enable_Hibernation(obj, false);
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		ActionParamsStruct params;
		params.MoveArrivedDistance = 100.0f;
		params.Set_Basic(this, 100, JFW_ACTION_PATROL);
		params.Set_Movement(enemy, Get_Float_Parameter("Speed"), Get_Float_Parameter("Distance"));
		params.MoveFollow = true;
		ScriptEngine::Action_Goto(obj, params);
	}

	void Action_Complete(GameObject* obj, int action_id, ActionCompleteReason /*reason*/) override
	{
		if (action_id == JFW_ACTION_PATROL && !FirstTime) {
			ScriptEngine::Action_Reset(obj, 100);
			ScriptEngine::Create_Explosion(Get_Parameter("Explosion"),
					ScriptEngine::Get_Position(obj), nullptr);
		} else {
			FirstTime = false;
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Handing out and taking away weapons
//
////////////////////////////////////////////////////////////////////////////

/*
**	Clear what is being carried, hand over up to five powerups, and select a
**	weapon.  A powerup named "0" is the level author saying "nothing here",
**	which is how a five-slot script is used to give out two things.
**
**	4.8.4 turned that "0" into a null pointer and handed it to Give_PowerUp
**	anyway; here the call is simply not made.
*/
static void	JFW_Change_Weapon (ScriptImpClass * script, GameObject * target,
		const char * const * powerup_names, int powerup_count, const char * weapon_name)
{
	if (target == nullptr) {
		return;
	}

	ScriptEngine::Clear_Weapons (target);

	for (int index = 0; index < powerup_count; index++) {
		const char * powerup = script->Get_Parameter (powerup_names[index]);

		if (powerup == nullptr || ::stricmp (powerup, "0") == 0) {
			continue;
		}

		ScriptEngine::Give_PowerUp (target, powerup, false);
	}

	ScriptEngine::Select_Weapon (target, weapon_name);
	return ;
}

static const char * const	_JFW_One_Powerup[]	= { "Powerup" };
static const char * const	_JFW_Five_Powerups[]	=
	{ "Powerup1", "Powerup2", "Powerup3", "Powerup4", "Powerup5" };


/*JFW_Clear_Weapons

  Walking into this zone leaves you empty-handed.
*/

DECLARE_SCRIPT_TT(JFW_Clear_Weapons, "")
{
	void Entered(GameObject* /*obj*/, GameObject* enterer) override
	{
		ScriptEngine::Clear_Weapons(enterer);
	}
};


/*JFW_Clear_Weapons_Create

  The same, to whatever the script is put on, the moment it exists.  Used on a
  preset to strip the weapons its definition grants.
*/

DECLARE_SCRIPT_TT(JFW_Clear_Weapons_Create, "")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Clear_Weapons(obj);
	}
};


/*JFW_Clear_Weapons_On_Custom

  The same, on a message.  Powerup and Weapon are in the parameter list because
  the three custom scripts were written from one template; this one ignores
  them.
*/

DECLARE_SCRIPT_TT(JFW_Clear_Weapons_On_Custom, "Message:int,Powerup:string,Weapon:string")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			ScriptEngine::Clear_Weapons(obj);
		}
	}
};


/*JFW_Zone_Weapon

  Walking into this zone hands out a weapon, and puts it in your hands.
  Player_Type is the usual filter: 0 Nod, 1 GDI, 2 anyone, 3 the local star.
*/

DECLARE_SCRIPT_TT(JFW_Zone_Weapon, "Weapon_Name:string,Player_Type:int")
{
	void Entered(GameObject* /*obj*/, GameObject* enterer) override
	{
		if (!Is_Player_Type(enterer, Get_Int_Parameter("Player_Type"))) {
			return;
		}

		ScriptEngine::Give_PowerUp(enterer, Get_Parameter("Weapon_Name"), true);
	}
};


/*JFW_Weapon_Change_On_Custom

  On the message, swap whatever the object is holding for one powerup's worth.
*/

DECLARE_SCRIPT_TT(JFW_Weapon_Change_On_Custom, "Message:int,Powerup:string,Weapon:string")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			JFW_Change_Weapon(this, obj, _JFW_One_Powerup, 1, Get_Parameter("Weapon"));
		}
	}
};


/*JFW_Weapon_Change_On_Custom_Sender

  The same, to whoever sent the message rather than to the object carrying the
  script -- so a zone or a poked terminal can re-arm the player who used it.
*/

DECLARE_SCRIPT_TT(JFW_Weapon_Change_On_Custom_Sender, "Message:int,Powerup:string,Weapon:string")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == Get_Int_Parameter("Message")) {
			JFW_Change_Weapon(this, sender, _JFW_One_Powerup, 1, Get_Parameter("Weapon"));
		}
	}
};


/*JFW_Weapon_Change_On_Custom_Multiple

  Five powerups instead of one.  Unused slots are set to "0".
*/

DECLARE_SCRIPT_TT(JFW_Weapon_Change_On_Custom_Multiple,
	"Message:int,Powerup1:string,Powerup2:string,Powerup3:string,Powerup4:string,"
	"Powerup5:string,Weapon:string")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			JFW_Change_Weapon(this, obj, _JFW_Five_Powerups, 5, Get_Parameter("Weapon"));
		}
	}
};


/*JFW_Weapon_Change_On_Custom_Multiple_Sender

  Five powerups, to the sender.
*/

DECLARE_SCRIPT_TT(JFW_Weapon_Change_On_Custom_Multiple_Sender,
	"Message:int,Powerup1:string,Powerup2:string,Powerup3:string,Powerup4:string,"
	"Powerup5:string,Weapon:string")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == Get_Int_Parameter("Message")) {
			JFW_Change_Weapon(this, sender, _JFW_Five_Powerups, 5, Get_Parameter("Weapon"));
		}
	}
};


/*JFW_Vehicle_Weapon_Preset

  Sits on a vehicle.  Whoever climbs in decides what the vehicle shoots with:
  if their preset is the one named, the vehicle re-arms.  A driver-specific
  gun without a driver-specific vehicle.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Weapon_Preset, "Powerup:string,Weapon:string,Preset:string")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_VEHICLE_ENTERED || sender == nullptr) {
			return;
		}

		if (::stricmp(ScriptEngine::Get_Preset_Name(sender), Get_Parameter("Preset")) != 0) {
			return;
		}

		JFW_Change_Weapon(this, obj, _JFW_One_Powerup, 1, Get_Parameter("Weapon"));
	}
};


/*JFW_Vehicle_Weapon_Weapon

  The same, keyed on what the driver is carrying rather than who they are.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Weapon_Weapon, "Powerup:string,Weapon:string,CharWeapon:string")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_VEHICLE_ENTERED || sender == nullptr) {
			return;
		}

		if (!ScriptEngine::Has_Weapon(sender, Get_Parameter("CharWeapon"))) {
			return;
		}

		JFW_Change_Weapon(this, obj, _JFW_One_Powerup, 1, Get_Parameter("Weapon"));
	}
};


/*JFW_Fire_Weapon_Custom

  On the message, pull the trigger at a fixed point in the world.  A gun
  emplacement a level fires by script rather than by seeing anything.
*/

DECLARE_SCRIPT_TT(JFW_Fire_Weapon_Custom, "Target:vector3,Custom:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Custom")) {
			ScriptEngine::Trigger_Weapon(obj, true, Get_Vector3_Parameter("Target"), true);
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Weapons on a key
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Vehicle_Weapon_Switcher

  A vehicle with a second weapon its driver deploys on a key.  Switching takes
  SwitchTime, during which the vehicle cannot fire at all -- that is the cost
  of deploying -- and each direction has its own sound.  The hook follows the
  driver: it is installed when somebody gets in and removed when they get out,
  so a key press by a passenger, or by the last driver after they have left,
  does nothing.

  Two corrections.  Coming back from the deployed weapon with nothing recorded
  to come back to -- which is the state a vehicle is in if the script is
  attached mid-switch -- selected a null weapon name.  And the timer restored
  the vehicle's ability to fire without checking it was still a vehicle.
*/

REGISTER_SCRIPT_TT(JFW_Vehicle_Weapon_Switcher,
	"Powerup:string,Keyhook=VDeploy:string,SwitchTime:float,OldSound:string,NewSound:string")
class JFW_Vehicle_Weapon_Switcher : public KeyHookScriptClass
{
	bool			Switching;
	bool			Deployed;
	int				PilotID;
	StringClass		StowedWeapon;

	//
	//	Nothing is auto-saved, as in 4.8.4.  Three of the four members would
	//	save cleanly and the fourth -- the weapon name to come back to -- would
	//	not, and a vehicle that restores as deployed with nothing to undeploy
	//	to is worse than one that restores stowed.
	//

	void Created(GameObject* obj) override
	{
		Switching	= false;
		Deployed	= false;
		PilotID		= 0;
		StowedWeapon = "";

		ScriptEngine::Give_PowerUp(obj, Get_Parameter("Powerup"), false);
	}

	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			if (PilotID == 0 && sender != nullptr) {
				Install_Hook(Get_Parameter("Keyhook"), sender);
				PilotID = ScriptEngine::Get_ID(sender);
			}

		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			if (sender != nullptr && PilotID == ScriptEngine::Get_ID(sender)) {
				Remove_Hook();
				PilotID = 0;
			}
		}
	}

	void Killed(GameObject* /*obj*/, GameObject* /*killer*/) override
	{
		Remove_Hook();
	}

	void Key_Hook(void) override
	{
		if (Switching) {
			return;
		}

		GameObject* obj = Owner();

		VehicleGameObj* vehicle = (obj != nullptr) ? obj->As_VehicleGameObj() : nullptr;
		if (vehicle == nullptr) {
			return;
		}

		vehicle->Set_Scripts_Can_Fire(false);
		Switching	= true;
		Deployed	= !Deployed;

		ScriptEngine::Create_Sound(Get_Parameter(Deployed ? "NewSound" : "OldSound"),
				ScriptEngine::Get_Position(obj), obj);

		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("SwitchTime"), 1);
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		if (timer_id != 1) {
			return;
		}

		Switching = false;

		VehicleGameObj* vehicle = (obj != nullptr) ? obj->As_VehicleGameObj() : nullptr;
		if (vehicle != nullptr) {
			vehicle->Set_Scripts_Can_Fire(true);
		}

		if (Deployed) {
			//
			//	Remember what it was carrying before the deployed weapon takes
			//	its place, so that undeploying has something to go back to.
			//
			StowedWeapon = ScriptEngine::Get_Current_Weapon(obj);
			ScriptEngine::Select_Weapon(obj, ScriptEngine::Get_Powerup_Weapon(Get_Parameter("Powerup")));

		} else if (StowedWeapon.Is_Empty() == false) {
			ScriptEngine::Select_Weapon(obj, StowedWeapon);
			StowedWeapon = "";
		}
	}
};


/*JFW_Char_Weapon_Switcher

  A character carrying one of two weapons and swapping between them on a key.
  Unlike the vehicle above the weapon is genuinely put away: the one being left
  behind is taken off them, and SwitchTime later the other is handed over.  A
  second press during that window is ignored, and a press within SwitchTime of
  the last completed swap is refused out loud, so the round trip costs twice
  the switch time.

  The cooldown is measured on the game clock rather than the wall clock the
  4.8.4 script read, which is the same clock the timer runs on.
*/

REGISTER_SCRIPT_TT(JFW_Char_Weapon_Switcher,
	"SwitchTime:float,WeaponPowerup1:string,WeaponPowerup2:string,Keyhook:string")
class JFW_Char_Weapon_Switcher : public KeyHookScriptClass
{
	unsigned int	LastSwitch;		// sync time in milliseconds
	bool			WantSecond;
	bool			Switching;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(LastSwitch, 1);
		SAVE_VARIABLE(WantSecond, 2);
		SAVE_VARIABLE(Switching, 3);
	}

	void Created(GameObject* obj) override
	{
		LastSwitch	= 0;
		WantSecond	= true;
		Switching	= false;

		Install_Hook(Get_Parameter("Keyhook"), obj);
	}

	void Key_Hook(void) override
	{
		GameObject* obj = Owner();
		if (obj == nullptr || Switching) {
			return;
		}

		//
		//	Not while driving: the weapon being swapped is not the one in use.
		//
		if (ScriptEngine::Get_Vehicle(obj) != nullptr) {
			return;
		}

		float wait = Get_Float_Parameter("SwitchTime");

		unsigned int now = ScriptEngine::Get_Sync_Time();
		if (LastSwitch != 0 && now - LastSwitch < (unsigned int)(wait * 1000.0f)) {
			StringClass message;
			message.Format("You cannot switch weapons yet, you must wait at least "
					"%g seconds after switching to switch again.", wait);
			ScriptEngine::Send_Message_Player(obj, 153, 204, 25, message);
			return;
		}

		LastSwitch = now;

		ScriptEngine::Remove_Weapon(obj, ScriptEngine::Get_Powerup_Weapon(
				Get_Parameter(WantSecond ? "WeaponPowerup1" : "WeaponPowerup2")));

		Switching = true;
		ScriptEngine::Start_Timer(obj, this, wait, 1);
	}

	void Timer_Expired(GameObject* obj, int timer_id) override
	{
		if (timer_id != 1) {
			return;
		}

		const char* powerup = Get_Parameter(WantSecond ? "WeaponPowerup2" : "WeaponPowerup1");

		ScriptEngine::Give_PowerUp(obj, powerup, false);
		ScriptEngine::Select_Weapon(obj, ScriptEngine::Get_Powerup_Weapon(powerup));

		WantSecond	= !WantSecond;
		Switching	= false;
	}
};

////////////////////////////////////////////////////////////////////////////
//
//	The base defence, forty-eight times
//
////////////////////////////////////////////////////////////////////////////
//
//	How long the turret holds each of the three points it stares at while it
//	has nothing to shoot.
//
static const float	JFW_DEFENCE_SWEEP_TIME	= 10.0f;

//
//	Timer 1 is the sweep for a turret standing in the open and the retract for
//	one that pops up; timer 2 is the give-up for the first and the "you are all
//	the way up now, you may fire" for the second.
//
enum
{
	JFW_DEFENCE_TIMER_SWEEP		= 1,
	JFW_DEFENCE_TIMER_RETRACT	= 1,
	JFW_DEFENCE_TIMER_GIVE_UP	= 2,
	JFW_DEFENCE_TIMER_RISEN		= 2
};

//
//	Action 1 is what a popup turret attacks with, action 2 what a standing one
//	does, and action 3 the aim of the player once he is sitting in it.
//
enum
{
	JFW_DEFENCE_ACTION_POPUP	= 1,
	JFW_DEFENCE_ACTION_ATTACK	= 2,
	JFW_DEFENCE_ACTION_DRIVEN	= 3
};

enum JFW_Defence_Filter
{
	DEFENCE_FILTER_ANY,
	DEFENCE_FILTER_NOT_LISTED,			// anything except the eight PresetN
	DEFENCE_FILTER_ONLY_LISTED,			// only the eight PresetN
	DEFENCE_FILTER_NO_VTOL,
	DEFENCE_FILTER_VTOL_ONLY,
	DEFENCE_FILTER_NO_VTOL_NO_STEALTH
};

enum JFW_Defence_Fire
{
	DEFENCE_FIRE_PRIMARY,				// always the primary weapon
	DEFENCE_FIRE_ALTERNATE,				// swap every time it re-acquires
	DEFENCE_FIRE_SWAPPED				// swap when told to by a custom message
};

struct	JFW_Defence_Traits
{
	JFW_Defence_Filter	Filter;
	bool						Popup;
	bool						Sound;
	JFW_Defence_Fire		Fire;
	bool						Driveable;
};


/*
**	One base defence.
**
**	Everything the forty-eight registered names do is here; each of them says
**	only which of the five choices it made.
*/
class	JFW_Defence_Base : public ScriptImpClass
{
public:

	void Register_Auto_Save_Variables (void) override
	{
		//
		//	All of it, whichever variant this is.  The 4.8.4 file registered
		//	these per variant and got it wrong every time a popup turret was
		//	involved: the "may fire now" flag was written with a size of two or
		//	three bytes for a one-byte bool, under the same id the popup flag
		//	had already claimed, and the variants that swap weapons on a
		//	message did not save it at all.  No popup turret came back from a
		//	save in the state it went in.
		//
		SAVE_VARIABLE (Token_ID[0],	1);
		SAVE_VARIABLE (Token_ID[1],	2);
		SAVE_VARIABLE (Token_ID[2],	3);
		SAVE_VARIABLE (Player_Type,	4);
		SAVE_VARIABLE (Primary,		5);
		SAVE_VARIABLE (Popped_Up,	6);
		SAVE_VARIABLE (Can_Attack,	7);
		SAVE_VARIABLE (Driven,		8);
	}

protected:

	//
	//	Which of the forty-eight this is.  Asked wherever the answer is needed
	//	rather than cached, so that a subclass is nothing but the answer.
	//
	virtual void	Traits (JFW_Defence_Traits & traits) = 0;

	int		Token_ID[3];
	int		Player_Type;
	bool		Primary;
	bool		Popped_Up;
	bool		Can_Attack;
	bool		Driven;

	void Created (GameObject * obj) override
	{
		JFW_Defence_Traits traits;
		Traits (traits);

		Player_Type	= ScriptEngine::Get_Player_Type (obj);
		Primary		= true;
		Popped_Up	= false;
		Can_Attack	= false;
		Driven		= false;

		Token_ID[0] = Token_ID[1] = Token_ID[2] = 0;

		if (traits.Popup)
		{
			//
			//	Sunk out of sight, watching.  It has nothing to sweep and
			//	nothing to sweep with until it comes up.
			//
			ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), false,
					nullptr, 0.0f, 0.0f, false);
			ScriptEngine::Enable_Enemy_Seen (obj, true);
			return ;
		}

		ScriptEngine::Enable_Hibernation (obj, false);
		ScriptEngine::Innate_Enable (obj);
		ScriptEngine::Enable_Enemy_Seen (obj, true);

		Create_Sweep_Points (obj);
		ScriptEngine::Start_Timer (obj, this, JFW_DEFENCE_SWEEP_TIME, JFW_DEFENCE_TIMER_SWEEP);
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * /*sender*/) override
	{
		JFW_Defence_Traits traits;
		Traits (traits);

		if (traits.Fire == DEFENCE_FIRE_SWAPPED && type == Get_Int_Parameter ("SwapMessage"))
		{
			Primary = !Primary;
			return ;
		}

		if (!traits.Driveable)
		{
			return ;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED)
		{
			//
			//	Somebody is in it.  It stops picking its own targets and
			//	points wherever he is looking.
			//
			Driven = true;

			ActionParamsStruct params;
			params.Set_Basic (this, 100.0f, JFW_DEFENCE_ACTION_DRIVEN);
			ScriptEngine::Action_Follow_Input (obj, params);
		}
		else if (type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			Driven = false;
			ScriptEngine::Set_Player_Type (obj, Player_Type);
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Enemy_Seen (GameObject * obj, GameObject * enemy) override
	{
		//
		//	Shoot the tank, not the man sitting in it.  Enemy_Seen names the
		//	soldier, and a defence firing at a driver was aiming at something
		//	it could not hit while the vehicle sat there unharmed.
		//
		GameObject * vehicle = ScriptEngine::Get_Vehicle (enemy);
		if (vehicle != nullptr)
		{
			enemy = vehicle;
		}

		JFW_Defence_Traits traits;
		Traits (traits);

		if (Driven || !Wants_Target (obj, enemy, traits))
		{
			return ;
		}

		if (traits.Popup)
		{
			Popup_Attack (obj, enemy, traits);
		}
		else
		{
			Standing_Attack (obj, enemy, traits);
		}
	}

	void Timer_Expired (GameObject * obj, int timer_id) override
	{
		JFW_Defence_Traits traits;
		Traits (traits);

		if (traits.Popup)
		{
			if (timer_id == JFW_DEFENCE_TIMER_RETRACT)
			{
				//
				//	Back down again, and blind until it next comes up.
				//
				Popped_Up	= false;
				Can_Attack	= false;
				ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), false,
						nullptr, Get_Float_Parameter ("LastFrame"), 0.0f, false);
				ScriptEngine::Action_Reset (obj, 100.0f);
			}
			else if (timer_id == JFW_DEFENCE_TIMER_RISEN)
			{
				Can_Attack = true;
			}

			return ;
		}

		if (timer_id == JFW_DEFENCE_TIMER_SWEEP)
		{
			if (!Driven)
			{
				Sweep (obj);
			}

			ScriptEngine::Start_Timer (obj, this, JFW_DEFENCE_SWEEP_TIME, JFW_DEFENCE_TIMER_SWEEP);
		}
		else if (timer_id == JFW_DEFENCE_TIMER_GIVE_UP)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Action_Complete (GameObject * obj, int action_id, ActionCompleteReason /*reason*/) override
	{
		if (action_id == JFW_DEFENCE_ACTION_ATTACK)
		{
			ScriptEngine::Action_Reset (obj, 100.0f);
		}
	}

	void Exited (GameObject * obj, GameObject * /*exiter*/) override
	{
		//
		//	Sitting in a defence hands it the side of the driver for as long
		//	as he is in it.  Give it back.
		//
		ScriptEngine::Set_Player_Type (obj, Player_Type);
	}

private:

	//
	//	The three points a standing turret looks at while idle, spaced evenly
	//	around it.  Two of the three used to sit on the same side, so it spent
	//	its idle time staring into one quarter of the map and swept the rest
	//	only on its way past.
	//
	void Create_Sweep_Points (GameObject * obj)
	{
		const Vector3 my_position = ScriptEngine::Get_Position (obj);

		Vector3 offsets[3];
		offsets[0].Set (-5.0f,  8.66025f, 2.0f);
		offsets[1].Set (10.0f,  0.0f,     2.0f);
		offsets[2].Set (-5.0f, -8.66025f, 2.0f);

		for (int index = 0; index < 3; index ++)
		{
			GameObject * token = ScriptEngine::Create_Object ("Invisible_Object",
					my_position + offsets[index]);
			if (token != nullptr)
			{
				Token_ID[index] = ScriptEngine::Get_ID (token);
			}
		}
	}

	void Sweep (GameObject * obj)
	{
		GameObject * token = ScriptEngine::Find_Object (Token_ID[Get_Int_Random (0, 2)]);
		if (token == nullptr)
		{
			return ;
		}

		ActionParamsStruct params;
		params.Set_Basic (this, 70.0f, JFW_DEFENCE_TIMER_SWEEP);
		params.Set_Attack (token, 0.0f, 0.0f, true);
		ScriptEngine::Action_Attack (obj, params);
	}

	void Standing_Attack (GameObject * obj, GameObject * enemy, const JFW_Defence_Traits & traits)
	{
		const float distance = ScriptEngine::Get_Distance (ScriptEngine::Get_Position (obj),
				ScriptEngine::Get_Position (enemy));
		if (distance < Get_Float_Parameter ("MinAttackDistance"))
		{
			return ;
		}

		ActionParamsStruct params;
		params.Set_Basic (this, 100.0f, JFW_DEFENCE_ACTION_ATTACK);
		params.Set_Attack (enemy, Get_Float_Parameter ("MaxAttackDistance"), 0.0f, Primary);
		params.AttackCheckBlocked = false;
		ScriptEngine::Action_Attack (obj, params);

		//
		//	Each sighting restarted the give-up timer without cancelling the
		//	one already running, so an older timer would fire mid-burst and
		//	reset the action against a target still in front of it.
		//
		ScriptEngine::Stop_Timer (obj, this, JFW_DEFENCE_TIMER_GIVE_UP);
		ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("AttackTimer"),
				JFW_DEFENCE_TIMER_GIVE_UP);

		if (traits.Fire == DEFENCE_FIRE_ALTERNATE)
		{
			Primary = !Primary;
		}
	}

	void Popup_Attack (GameObject * obj, GameObject * enemy, const JFW_Defence_Traits & traits)
	{
		if (!Popped_Up)
		{
			//
			//	Coming up takes PopupTime, and it cannot shoot on the way.
			//
			Popped_Up = true;
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("PopupTime"),
					JFW_DEFENCE_TIMER_RISEN);
			ScriptEngine::Set_Animation (obj, Get_Parameter ("Animation"), false,
					nullptr, 0.0f, Get_Float_Parameter ("LastFrame"), false);

			if (traits.Sound)
			{
				ScriptEngine::Create_Sound (Get_Parameter ("Sound"),
						ScriptEngine::Get_Position (enemy), obj);
			}

			ScriptEngine::Stop_Timer (obj, this, JFW_DEFENCE_TIMER_RETRACT);
			ScriptEngine::Start_Timer (obj, this, Get_Float_Parameter ("AttackTimer"),
					JFW_DEFENCE_TIMER_RETRACT);
		}
		else if (Can_Attack)
		{
			ActionParamsStruct params;
			params.Set_Basic (this, 100.0f, JFW_DEFENCE_ACTION_POPUP);
			params.Set_Attack (enemy, Get_Float_Parameter ("MaxAttackDistance"), 0.0f, Primary);
			params.AttackCheckBlocked = false;
			ScriptEngine::Action_Attack (obj, params);

			if (traits.Fire == DEFENCE_FIRE_ALTERNATE)
			{
				Primary = !Primary;
			}
		}
	}

	bool Wants_Target (GameObject * obj, GameObject * enemy, const JFW_Defence_Traits & traits)
	{
		switch (traits.Filter)
		{
			case DEFENCE_FILTER_NOT_LISTED:
				return !Is_Listed_Preset (enemy);

			case DEFENCE_FILTER_ONLY_LISTED:
				return Is_Listed_Preset (enemy);

			case DEFENCE_FILTER_NO_VTOL:
				return !ScriptEngine::Is_VTOL (enemy);

			case DEFENCE_FILTER_VTOL_ONLY:
				return ScriptEngine::Is_VTOL (enemy);

			case DEFENCE_FILTER_NO_VTOL_NO_STEALTH:
			{
				if (ScriptEngine::Is_VTOL (enemy))
				{
					return false;
				}

				//
				//	A stealthed unit is only worth shooting at from close
				//	enough that the turret could plausibly have noticed it.
				//
				SmartGameObj * smart = enemy->As_SmartGameObj ();
				if (smart == nullptr || !smart->Is_Stealthed ())
				{
					return true;
				}

				const float distance = ScriptEngine::Get_Distance (ScriptEngine::Get_Position (obj),
						ScriptEngine::Get_Position (enemy));
				return (distance <= Get_Float_Parameter ("MaxStealthAttackDistance"));
			}

			default:
				break;
		}

		return true;
	}

	//
	//	Is this one of the eight presets the level named?  The 4.8.4 file
	//	spelled this out as a sixteen-term condition in each of the twenty
	//	filtered variants, fetching the preset name of the target once per
	//	term.
	//
	bool Is_Listed_Preset (GameObject * enemy)
	{
		const char * name = ScriptEngine::Get_Preset_Name (enemy);
		if (name == nullptr || name[0] == 0)
		{
			return false;
		}

		static const char * const	_slots[8] =
		{
			"Preset1", "Preset2", "Preset3", "Preset4",
			"Preset5", "Preset6", "Preset7", "Preset8"
		};

		for (int index = 0; index < 8; index ++)
		{
			const char * listed = Get_Parameter (_slots[index]);
			if (listed != nullptr && listed[0] != 0 && ::_stricmp (name, listed) == 0)
			{
				return true;
			}
		}

		return false;
	}
};


//
//	The parameters, in the order the 4.8.4 file registered them.  A standing
//	turret has a minimum range and gives up after AttackTimer; one that pops up
//	has no minimum, and AttackTimer is instead how long it stays up.
//
#define	JFW_DEF_RANGE			"MinAttackDistance=0.0:float,MaxAttackDistance=300.0:float,AttackTimer=10.0:float"
#define	JFW_DEF_POPUP_RANGE	"MaxAttackDistance:float,AttackTimer:float"
#define	JFW_DEF_PRESETS		",Preset1:string,Preset2:string,Preset3:string,Preset4:string,Preset5:string,Preset6:string,Preset7:string,Preset8:string"
#define	JFW_DEF_ANIM			",Animation:string,LastFrame:float,PopupTime:float"
#define	JFW_DEF_SOUND			",Sound:string"
#define	JFW_DEF_SWAP			",SwapMessage:int"
#define	JFW_DEF_STEALTH		",MaxStealthAttackDistance=0.0:float"

//
//	One of the forty-eight.  The body is the five answers and nothing else.
//
#define	JFW_DEFENCE(x, params, filter, popup, sound, fire)	\
	REGISTER_SCRIPT_TT (x, params)							\
	class x : public JFW_Defence_Base							\
	{																	\
		void Traits (JFW_Defence_Traits & traits) override	\
		{																\
			traits.Filter		= filter;							\
			traits.Popup		= popup;								\
			traits.Sound		= sound;								\
			traits.Fire			= fire;								\
			traits.Driveable	= false;								\
		}																\
	};


/*
**	The stock base defence, which is the all-defaults member of the family:
**	shoots anything, stands in the open, always fires its primary weapon.
**
**	The 4.8.4 file registered its own JFW_Base_Defence alongside a second
**	registration of this same stock name, the two being the same script written
**	twice.  Here there is one script, and JFW_Base_Defence is a second name for
**	it.  The registered parameters read as floats rather than the ints the
**	stock name used, so a level may now give it a fractional range; an integer
**	still parses as one, so nothing already built changes.
*/
REGISTER_SCRIPT_MERGED_ALIAS (M00_Base_Defense, JFW_DEF_RANGE, "JFW_Base_Defence")
class	M00_Base_Defense : public JFW_Defence_Base
{
	void Traits (JFW_Defence_Traits & traits) override
	{
		traits.Filter		= DEFENCE_FILTER_ANY;
		traits.Popup		= false;
		traits.Sound		= false;
		traits.Fire			= DEFENCE_FIRE_PRIMARY;
		traits.Driveable	= false;
	}
};


/*
**	The same, except that a player may climb in and aim it himself.  While
**	somebody is driving it, it stops choosing its own targets and stops
**	sweeping.
*/
REGISTER_SCRIPT_TT (JFW_User_Controllable_Base_Defence, JFW_DEF_RANGE)
class	JFW_User_Controllable_Base_Defence : public JFW_Defence_Base
{
	void Traits (JFW_Defence_Traits & traits) override
	{
		traits.Filter		= DEFENCE_FILTER_ANY;
		traits.Popup		= false;
		traits.Sound		= false;
		traits.Fire			= DEFENCE_FIRE_PRIMARY;
		traits.Driveable	= true;
	}
};


/*
**	Standing in the open, primary weapon only.
*/

JFW_DEFENCE (JFW_Base_Defence_No_Aircraft,
		JFW_DEF_RANGE JFW_DEF_PRESETS,
		DEFENCE_FILTER_NOT_LISTED, false, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Aircraft_Only,
		JFW_DEF_RANGE JFW_DEF_PRESETS,
		DEFENCE_FILTER_ONLY_LISTED, false, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_No_VTOL,
		JFW_DEF_RANGE,
		DEFENCE_FILTER_NO_VTOL, false, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_No_VTOL_No_Stealth,
		JFW_DEF_RANGE JFW_DEF_STEALTH,
		DEFENCE_FILTER_NO_VTOL_NO_STEALTH, false, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_VTOL_Only,
		JFW_DEF_RANGE,
		DEFENCE_FILTER_VTOL_ONLY, false, false, DEFENCE_FIRE_PRIMARY)


/*
**	Standing in the open, alternating primary and secondary each time it re-acquires.
*/

JFW_DEFENCE (JFW_Base_Defence_Secondary,
		JFW_DEF_RANGE,
		DEFENCE_FILTER_ANY, false, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_No_Aircraft_Secondary,
		JFW_DEF_RANGE JFW_DEF_PRESETS,
		DEFENCE_FILTER_NOT_LISTED, false, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Aircraft_Only_Secondary,
		JFW_DEF_RANGE JFW_DEF_PRESETS,
		DEFENCE_FILTER_ONLY_LISTED, false, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_No_VTOL_Secondary,
		JFW_DEF_RANGE,
		DEFENCE_FILTER_NO_VTOL, false, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_VTOL_Only_Secondary,
		JFW_DEF_RANGE,
		DEFENCE_FILTER_VTOL_ONLY, false, false, DEFENCE_FIRE_ALTERNATE)


/*
**	Standing in the open, weapon chosen by a custom message.
*/

JFW_DEFENCE (JFW_Base_Defence_Swap,
		JFW_DEF_RANGE JFW_DEF_SWAP,
		DEFENCE_FILTER_ANY, false, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_No_Aircraft_Swap,
		JFW_DEF_RANGE JFW_DEF_PRESETS JFW_DEF_SWAP,
		DEFENCE_FILTER_NOT_LISTED, false, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Aircraft_Only_Swap,
		JFW_DEF_RANGE JFW_DEF_PRESETS JFW_DEF_SWAP,
		DEFENCE_FILTER_ONLY_LISTED, false, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_No_VTOL_Swap,
		JFW_DEF_RANGE JFW_DEF_SWAP,
		DEFENCE_FILTER_NO_VTOL, false, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_VTOL_Only_Swap,
		JFW_DEF_RANGE JFW_DEF_SWAP,
		DEFENCE_FILTER_VTOL_ONLY, false, false, DEFENCE_FIRE_SWAPPED)


/*
**	Popping up out of the ground, primary weapon only.
*/

JFW_DEFENCE (JFW_Base_Defence_Animated,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM,
		DEFENCE_FILTER_ANY, true, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_No_Aircraft,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM,
		DEFENCE_FILTER_NOT_LISTED, true, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_Aircraft_Only,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM,
		DEFENCE_FILTER_ONLY_LISTED, true, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_No_VTOL,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM,
		DEFENCE_FILTER_NO_VTOL, true, false, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_VTOL_Only,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM,
		DEFENCE_FILTER_VTOL_ONLY, true, false, DEFENCE_FIRE_PRIMARY)


/*
**	Popping up, alternating primary and secondary.
*/

JFW_DEFENCE (JFW_Base_Defence_Animated_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM,
		DEFENCE_FILTER_ANY, true, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_No_Aircraft_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM,
		DEFENCE_FILTER_NOT_LISTED, true, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_Aircraft_Only_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM,
		DEFENCE_FILTER_ONLY_LISTED, true, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_No_VTOL_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM,
		DEFENCE_FILTER_NO_VTOL, true, false, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_VTOL_Only_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM,
		DEFENCE_FILTER_VTOL_ONLY, true, false, DEFENCE_FIRE_ALTERNATE)


/*
**	Popping up, weapon chosen by a custom message.
*/

JFW_DEFENCE (JFW_Base_Defence_Animated_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SWAP,
		DEFENCE_FILTER_ANY, true, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_No_Aircraft_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SWAP,
		DEFENCE_FILTER_NOT_LISTED, true, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_Aircraft_Only_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SWAP,
		DEFENCE_FILTER_ONLY_LISTED, true, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_No_VTOL_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SWAP,
		DEFENCE_FILTER_NO_VTOL, true, false, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_VTOL_Only_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SWAP,
		DEFENCE_FILTER_VTOL_ONLY, true, false, DEFENCE_FIRE_SWAPPED)


/*
**	Popping up with a sound as it rises, primary weapon only.
*/

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_ANY, true, true, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_No_Aircraft,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_NOT_LISTED, true, true, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_Aircraft_Only,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_ONLY_LISTED, true, true, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_No_VTOL,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_NO_VTOL, true, true, DEFENCE_FIRE_PRIMARY)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_VTOL_Only,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_VTOL_ONLY, true, true, DEFENCE_FIRE_PRIMARY)


/*
**	Popping up with a sound, alternating primary and secondary.
*/

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_ANY, true, true, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_No_Aircraft_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_NOT_LISTED, true, true, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_Aircraft_Only_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_ONLY_LISTED, true, true, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_No_VTOL_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_NO_VTOL, true, true, DEFENCE_FIRE_ALTERNATE)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_VTOL_Only_Secondary,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND,
		DEFENCE_FILTER_VTOL_ONLY, true, true, DEFENCE_FIRE_ALTERNATE)


/*
**	Popping up with a sound, weapon chosen by a custom message.
*/

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND JFW_DEF_SWAP,
		DEFENCE_FILTER_ANY, true, true, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_No_Aircraft_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SOUND JFW_DEF_SWAP,
		DEFENCE_FILTER_NOT_LISTED, true, true, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_Aircraft_Only_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_PRESETS JFW_DEF_ANIM JFW_DEF_SOUND JFW_DEF_SWAP,
		DEFENCE_FILTER_ONLY_LISTED, true, true, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_No_VTOL_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND JFW_DEF_SWAP,
		DEFENCE_FILTER_NO_VTOL, true, true, DEFENCE_FIRE_SWAPPED)

JFW_DEFENCE (JFW_Base_Defence_Animated_Sound_VTOL_Only_Swap,
		JFW_DEF_POPUP_RANGE JFW_DEF_ANIM JFW_DEF_SOUND JFW_DEF_SWAP,
		DEFENCE_FILTER_VTOL_ONLY, true, true, DEFENCE_FIRE_SWAPPED)
