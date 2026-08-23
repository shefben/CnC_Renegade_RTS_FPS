/******************************************************************************
*
* FILE
*     TT_Weapons.cpp
*
* DESCRIPTION
*     The building-mounted gun the 4.8.4 library added to a level.  One script
*     sits on the building: it creates a gun object at an offset from it, hands
*     that object the matching second script, and once a second afterwards tells
*     it whether the building is still standing and still has power.  The other
*     script sits on the gun, decides whether what it can see is its business,
*     and shoots.
*
*     Native port of the library's jfwgun.cpp.  That file is those two scripts
*     written out sixty times, once for each combination of four choices: which
*     half of the pair it is, whether the gun animates and whether the animation
*     makes a noise, which targets it will take, and whether it alternates fire
*     modes.  Two of those are a branch each and two are a table lookup, so here
*     it is the two scripts and, at the bottom, the table of thirty pairs of
*     names.  The names are the ones levels were saved against.
*
******************************************************************************/

#include "scripts.h"
#include "actionparams.h"
#include "basegameobj.h"
#include "physicalgameobj.h"
#include "smartgameobj.h"
#include "wwstring.h"


/*
**	How elaborately the gun goes about firing.
*/
enum JFWGunStyleEnum
{
	JFW_GUN_STYLE_PLAIN,				// shoot on sight
	JFW_GUN_STYLE_ANIMATED,				// wind an animation up, then shoot
	JFW_GUN_STYLE_ANIMATED_SOUND		// and make a noise doing it
};


/*
**	What the gun considers its business.  The two listed filters read eight
**	preset names out of the script's parameters; the two VTOL filters ask the
**	engine whether the target flies.
*/
enum JFWGunFilterEnum
{
	JFW_GUN_FILTER_ANY,					// anything it can see
	JFW_GUN_FILTER_NOT_LISTED,			// anything but the eight named presets
	JFW_GUN_FILTER_ONLY_LISTED,			// only the eight named presets
	JFW_GUN_FILTER_NOT_VTOL,			// anything that is not flying
	JFW_GUN_FILTER_VTOL_ONLY			// only what is flying
};


struct JFWGunVariantStruct
{
	int					Style;
	int					Filter;
	bool				Alternates;		// alternate primary and secondary fire
	const char *	GunScript;		// what the building half puts on the gun it makes
};


/*
**	4.8.4 used timer id 1 for three unrelated things and id 2 for a fourth.  No
**	one script ever sees more than two of them, so the ids are kept as they were.
*/
enum
{
	JFW_GUN_TIMER_POLL			= 1,	// the building half, asking after the building
	JFW_GUN_TIMER_RESET			= 1,	// a plain gun letting go of its target
	JFW_GUN_TIMER_WIND_UP		= 1,	// an animated gun reaching its firing frame
	JFW_GUN_TIMER_WIND_DOWN		= 2		// and returning from it
};

const int JFW_GUN_PRESET_COUNT = 8;
const float JFW_GUN_ACTION_PRIORITY = 100.0f;
const int JFW_GUN_ACTION_ID = 1;


/*
**	The half that goes on the building.
**
**	It creates the gun, keeps hold of its id, and reports the building's state
**	to it once a second.
**
**	What it reports depends on the style, and the asymmetry is 4.8.4's: a plain
**	mount says both "stop" and "carry on", so its gun comes back when the power
**	does, while an animated mount only ever says "stop".  An animated gun that
**	loses power stays lost.  Preserved rather than corrected -- it is a rule a
**	level can be built around, not a defect with one right answer.
*/
class JFW_Building_Gun_Mount : public ScriptImpClass
{
public:

	virtual const JFWGunVariantStruct & Variant(void) = 0;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(GunID, 1);
	}

private:

	int		GunID;

	void Created(GameObject* obj) override
	{
		GunID = 0;

		//	"Positon" is 4.8.4's spelling of the offset, and it is the spelling
		//	that is written into every level that uses this script.
		Vector3 position = ScriptEngine::Get_Position(obj) + Get_Vector3_Parameter("Positon");

		GameObject* gun = ScriptEngine::Create_Object(Get_Parameter("Weapon"), position);
		if (gun == nullptr) {
			return;
		}

		GunID = ScriptEngine::Get_ID(gun);
		ScriptEngine::Attach_Script(gun, Variant().GunScript, Gun_Parameters().Peek_Buffer());
		ScriptEngine::Start_Timer(obj, this, 1.0f, JFW_GUN_TIMER_POLL);
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		Tell_Gun(obj, 0);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number != JFW_GUN_TIMER_POLL) {
			return;
		}

		if (!ScriptEngine::Get_Building_Power(obj) || ScriptEngine::Get_Health(obj) == 0.0f) {
			Tell_Gun(obj, 0);
		} else if (Variant().Style == JFW_GUN_STYLE_PLAIN) {
			Tell_Gun(obj, 1);
		}

		ScriptEngine::Start_Timer(obj, this, 1.0f, JFW_GUN_TIMER_POLL);
	}

	void Tell_Gun(GameObject* obj, int enabled)
	{
		GameObject* gun = ScriptEngine::Find_Object(GunID);
		if (gun != nullptr) {
			ScriptEngine::Send_Custom_Event(obj, gun, Get_Int_Parameter("Disable_Custom"), enabled, 0);
		}
	}

	/*
	**	The gun half is a separate script with its own parameter list, so the
	**	parameters the two share are handed across as the text they arrived as.
	**	The order here is the order the gun half's list declares them in.
	*/
	StringClass Gun_Parameters(void)
	{
		const JFWGunVariantStruct& variant = Variant();

		StringClass params;
		params.Format("%s,%s,%s,%s", Get_Parameter("Disable_Custom"), Get_Parameter("Visible"),
				Get_Parameter("Min_Range"), Get_Parameter("Max_Range"));

		if (variant.Filter == JFW_GUN_FILTER_NOT_LISTED
				|| variant.Filter == JFW_GUN_FILTER_ONLY_LISTED) {
			for (int index = 1; index <= JFW_GUN_PRESET_COUNT; index++) {
				StringClass name;
				name.Format("Preset%d", index);
				params += ",";
				params += Get_Parameter(name.Peek_Buffer());
			}
		}

		if (variant.Style != JFW_GUN_STYLE_PLAIN) {
			params += ",";
			params += Get_Parameter("EffectPreset");
			params += ",";
			params += Get_Parameter("EffectAnimation");
			params += ",";
			params += Get_Parameter("EffectTime");
			params += ",";
			params += Get_Parameter("EffectFrame");
		}

		if (variant.Style == JFW_GUN_STYLE_ANIMATED_SOUND) {
			params += ",";
			params += Get_Parameter("Sound");
		}

		return params;
	}
};


/*
**	The half that goes on the gun the building half made.
*/
class JFW_Building_Gun_Turret : public ScriptImpClass
{
public:

	virtual const JFWGunVariantStruct & Variant(void) = 0;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(EffectID, 1);
		SAVE_VARIABLE(AttackID, 2);
		SAVE_VARIABLE(WoundUp, 3);
		SAVE_VARIABLE(Primary, 4);
	}

private:

	int		EffectID;		// the animating model in front of the gun, if it has one
	int		AttackID;		// what it decided to shoot at while winding up
	bool	WoundUp;		// the animation is sitting at its firing frame
	bool	Primary;		// which fire mode an alternating gun uses next

	void Created(GameObject* obj) override
	{
		EffectID = 0;
		AttackID = 0;
		WoundUp = false;
		Primary = true;

		ScriptEngine::Enable_Hibernation(obj, false);
		ScriptEngine::Innate_Enable(obj);
		ScriptEngine::Enable_Enemy_Seen(obj, true);
		ScriptEngine::Set_Is_Rendered(obj, Get_Int_Parameter("Visible") != 0);

		if (Variant().Style == JFW_GUN_STYLE_PLAIN) {
			return;
		}

		GameObject* effect = ScriptEngine::Create_Object(Get_Parameter("EffectPreset"),
				ScriptEngine::Get_Position(obj));
		if (effect != nullptr) {
			EffectID = ScriptEngine::Get_ID(effect);
			ScriptEngine::Set_Animation(effect, Get_Parameter("EffectAnimation"), false,
					nullptr, 0.0f, 0.0f, false);
		}
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Disable_Custom")) {
			return;
		}

		//	An animated mount only ever sends this to say the building is gone,
		//	so its gun does not read the parameter.  See the note on the mount.
		if (Variant().Style == JFW_GUN_STYLE_PLAIN) {
			ScriptEngine::Enable_Enemy_Seen(obj, param != 0);
		} else {
			ScriptEngine::Enable_Enemy_Seen(obj, false);
		}
	}

	void Enemy_Seen(GameObject* obj, GameObject* enemy) override
	{
		//	A soldier in a vehicle is shot at as the vehicle.
		GameObject* target = ScriptEngine::Get_Vehicle(enemy);
		if (target == nullptr) {
			target = enemy;
		}

		if (!Is_Our_Business(target)) {
			return;
		}

		Vector3 there = ScriptEngine::Get_Position(target);
		if (!In_Range(ScriptEngine::Get_Position(obj), there)) {
			return;
		}

		if (Variant().Style == JFW_GUN_STYLE_PLAIN) {
			Open_Fire(obj, target);
			ScriptEngine::Start_Timer(obj, this, 1.0f, JFW_GUN_TIMER_RESET);
			return;
		}

		//	An animated gun stops looking, winds forward, and shoots when it
		//	arrives.  If it is already wound up it has a shot in flight already.
		ScriptEngine::Enable_Enemy_Seen(obj, false);
		AttackID = ScriptEngine::Get_ID(target);

		if (WoundUp) {
			return;
		}

		WoundUp = true;
		Play_Effect(0.0f, Get_Float_Parameter("EffectFrame"));

		if (Variant().Style == JFW_GUN_STYLE_ANIMATED_SOUND) {
			//	At the target, but on the ground: 4.8.4 flattens the vector for
			//	the range test and then plays the sound at what is left of it.
			Vector3 where = there;
			where.Z = 0.0f;
			ScriptEngine::Create_Sound(Get_Parameter("Sound"), where, obj);
		}

		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("EffectTime"),
				JFW_GUN_TIMER_WIND_UP);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (Variant().Style == JFW_GUN_STYLE_PLAIN) {
			if (number == JFW_GUN_TIMER_RESET) {
				ScriptEngine::Action_Reset(obj, JFW_GUN_ACTION_PRIORITY);
			}
			return;
		}

		if (number == JFW_GUN_TIMER_WIND_DOWN) {
			ScriptEngine::Enable_Enemy_Seen(obj, true);
			ScriptEngine::Action_Reset(obj, JFW_GUN_ACTION_PRIORITY);
			return;
		}

		if (number != JFW_GUN_TIMER_WIND_UP) {
			return;
		}

		if (WoundUp) {
			WoundUp = false;
			Play_Effect(Get_Float_Parameter("EffectFrame"), 0.0f);
			ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("EffectTime"),
					JFW_GUN_TIMER_WIND_DOWN);
		}

		//	The target can die during the wind-up.  4.8.4 read its position and
		//	its class through the null pointer that leaves behind.
		GameObject* target = ScriptEngine::Find_Object(AttackID);
		if (target == nullptr) {
			return;
		}

		if (!In_Range(ScriptEngine::Get_Position(obj), ScriptEngine::Get_Position(target))) {
			return;
		}

		SmartGameObj* gun = obj->As_SmartGameObj();
		PhysicalGameObj* seen = target->As_PhysicalGameObj();

		if (gun != nullptr && seen != nullptr && gun->Is_Obj_Visible(seen)) {
			Open_Fire(obj, target);
		}
	}

	/*
	**	Whether this target is the one this variant was put here to shoot.
	*/
	bool Is_Our_Business(GameObject* target)
	{
		switch (Variant().Filter)
		{
			case JFW_GUN_FILTER_NOT_LISTED:
				return !Is_Listed(target);

			case JFW_GUN_FILTER_ONLY_LISTED:
				return Is_Listed(target);

			case JFW_GUN_FILTER_NOT_VTOL:
				return !ScriptEngine::Is_VTOL(target);

			case JFW_GUN_FILTER_VTOL_ONLY:
				return ScriptEngine::Is_VTOL(target);

			default:
				return true;
		}
	}

	/*
	**	Whether the target is one of the eight presets named in the parameters.
	**	An empty slot names nothing and so matches nothing.
	*/
	bool Is_Listed(GameObject* target)
	{
		const char* preset = ScriptEngine::Get_Preset_Name(target);
		if (preset == nullptr || preset[0] == '\0') {
			return false;
		}

		for (int index = 1; index <= JFW_GUN_PRESET_COUNT; index++) {
			StringClass name;
			name.Format("Preset%d", index);

			if (::stricmp(preset, Get_Parameter(name.Peek_Buffer())) == 0) {
				return true;
			}
		}

		return false;
	}

	/*
	**	The near edge of the band is measured on the ground and the far edge in
	**	a straight line, so something directly overhead is never too close.
	*/
	bool In_Range(const Vector3& from, const Vector3& to)
	{
		float range = ScriptEngine::Get_Distance(from, to);

		Vector3 flat_from = from;
		Vector3 flat_to = to;
		flat_from.Z = 0.0f;
		flat_to.Z = 0.0f;

		return (ScriptEngine::Get_Distance(flat_from, flat_to) > Get_Float_Parameter("Min_Range")
				&& range < Get_Float_Parameter("Max_Range"));
	}

	void Open_Fire(GameObject* obj, GameObject* target)
	{
		ActionParamsStruct params;
		params.Set_Basic(this, JFW_GUN_ACTION_PRIORITY, JFW_GUN_ACTION_ID);
		params.Set_Attack(target, Get_Float_Parameter("Max_Range"), 0.0f,
				Variant().Alternates ? Primary : true);
		ScriptEngine::Action_Attack(obj, params);

		if (Variant().Alternates) {
			Primary = !Primary;
		}
	}

	void Play_Effect(float start_frame, float end_frame)
	{
		GameObject* effect = ScriptEngine::Find_Object(EffectID);
		if (effect != nullptr) {
			ScriptEngine::Set_Animation(effect, Get_Parameter("EffectAnimation"), false,
					nullptr, start_frame, end_frame, false);
		}
	}
};


/*
**	The parameter lists, in the groups 4.8.4 assembled its sixty lists out of.
**	A variant declares its half's own parameters plus whatever its filter and
**	its style need to read.
*/
#define JFW_GUN_PARAMS_MOUNT	"Disable_Custom:int,Positon:vector3,Weapon:string,Visible:int,Min_Range:float,Max_Range:float"
#define JFW_GUN_PARAMS_GUN		"Disable_Custom:int,Visible:int,Min_Range:float,Max_Range:float"
#define JFW_GUN_PARAMS_PRESETS	",Preset1:string,Preset2:string,Preset3:string,Preset4:string,Preset5:string,Preset6:string,Preset7:string,Preset8:string"
#define JFW_GUN_PARAMS_EFFECT	",EffectPreset:string,EffectAnimation:string,EffectTime:float,EffectFrame:float"
#define JFW_GUN_PARAMS_SOUND	",Sound:string"


/*
**	The thirty variants.  Each is a pair -- the script that goes on the building
**	and the script that goes on the gun it makes -- sharing one set of choices.
**	The class names are the registered names, which are the names levels were
**	saved against.
*/

/*
**	Shoots the moment it sees something worth shooting.
*/

//	Anything it can see, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_ANY, false, "JFW_Building_Gun_Weapon" };

class JFW_Building_Gun : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Variant; } };

class JFW_Building_Gun_Weapon : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun,
	JFW_GUN_PARAMS_MOUNT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon,
	JFW_GUN_PARAMS_GUN)


//	Anything it can see, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Secondary_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_ANY, true, "JFW_Building_Gun_Weapon_Secondary" };

class JFW_Building_Gun_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Secondary_Variant; } };

class JFW_Building_Gun_Weapon_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Secondary,
	JFW_GUN_PARAMS_MOUNT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_Secondary,
	JFW_GUN_PARAMS_GUN)


//	Anything but the eight named presets, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_No_Aircraft_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_NOT_LISTED, false, "JFW_Building_Gun_Weapon_No_Aircraft" };

class JFW_Building_Gun_No_Aircraft : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_Aircraft_Variant; } };

class JFW_Building_Gun_Weapon_No_Aircraft : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_Aircraft_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_No_Aircraft,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_No_Aircraft,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS)


//	Anything but the eight named presets, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_No_Aircraft_Secondary_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_NOT_LISTED, true, "JFW_Building_Gun_Weapon_No_Aircraft_Secondary" };

class JFW_Building_Gun_No_Aircraft_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_Aircraft_Secondary_Variant; } };

class JFW_Building_Gun_Weapon_No_Aircraft_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_Aircraft_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_No_Aircraft_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_No_Aircraft_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS)


//	Only the eight named presets, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Aircraft_Only_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_ONLY_LISTED, false, "JFW_Building_Gun_Weapon_Aircraft_Only" };

class JFW_Building_Gun_Aircraft_Only : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Aircraft_Only_Variant; } };

class JFW_Building_Gun_Weapon_Aircraft_Only : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Aircraft_Only_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Aircraft_Only,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_Aircraft_Only,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS)


//	Only the eight named presets, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Aircraft_Only_Secondary_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_ONLY_LISTED, true, "JFW_Building_Gun_Weapon_Aircraft_Only_Secondary" };

class JFW_Building_Gun_Aircraft_Only_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Aircraft_Only_Secondary_Variant; } };

class JFW_Building_Gun_Weapon_Aircraft_Only_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Aircraft_Only_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Aircraft_Only_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_Aircraft_Only_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS)


//	Anything that is not flying, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_No_VTOL_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_NOT_VTOL, false, "JFW_Building_Gun_Weapon_No_VTOL" };

class JFW_Building_Gun_No_VTOL : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_VTOL_Variant; } };

class JFW_Building_Gun_Weapon_No_VTOL : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_VTOL_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_No_VTOL,
	JFW_GUN_PARAMS_MOUNT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_No_VTOL,
	JFW_GUN_PARAMS_GUN)


//	Anything that is not flying, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_No_VTOL_Secondary_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_NOT_VTOL, true, "JFW_Building_Gun_Weapon_No_VTOL_Secondary" };

class JFW_Building_Gun_No_VTOL_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_VTOL_Secondary_Variant; } };

class JFW_Building_Gun_Weapon_No_VTOL_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_No_VTOL_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_No_VTOL_Secondary,
	JFW_GUN_PARAMS_MOUNT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_No_VTOL_Secondary,
	JFW_GUN_PARAMS_GUN)


//	Only what is flying, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_VTOL_Only_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_VTOL_ONLY, false, "JFW_Building_Gun_Weapon_VTOL_Only" };

class JFW_Building_Gun_VTOL_Only : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_VTOL_Only_Variant; } };

class JFW_Building_Gun_Weapon_VTOL_Only : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_VTOL_Only_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_VTOL_Only,
	JFW_GUN_PARAMS_MOUNT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_VTOL_Only,
	JFW_GUN_PARAMS_GUN)


//	Only what is flying, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_VTOL_Only_Secondary_Variant =
	{ JFW_GUN_STYLE_PLAIN, JFW_GUN_FILTER_VTOL_ONLY, true, "JFW_Building_Gun_Weapon_VTOL_Only_Secondary" };

class JFW_Building_Gun_VTOL_Only_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_VTOL_Only_Secondary_Variant; } };

class JFW_Building_Gun_Weapon_VTOL_Only_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_VTOL_Only_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_VTOL_Only_Secondary,
	JFW_GUN_PARAMS_MOUNT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Weapon_VTOL_Only_Secondary,
	JFW_GUN_PARAMS_GUN)


/*
**	Winds an animation forward first and shoots when it gets there.
*/

//	Anything it can see, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_ANY, false, "JFW_Building_Gun_Animated_Weapon" };

class JFW_Building_Gun_Animated : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Variant; } };

class JFW_Building_Gun_Animated_Weapon : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT)


//	Anything it can see, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_ANY, true, "JFW_Building_Gun_Animated_Weapon_Secondary" };

class JFW_Building_Gun_Animated_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Weapon_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT)


//	Anything but the eight named presets, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_No_Aircraft_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_NOT_LISTED, false, "JFW_Building_Gun_Animated_Weapon_No_Aircraft" };

class JFW_Building_Gun_Animated_No_Aircraft : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_Aircraft_Variant; } };

class JFW_Building_Gun_Animated_Weapon_No_Aircraft : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_Aircraft_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_No_Aircraft,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_No_Aircraft,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)


//	Anything but the eight named presets, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_No_Aircraft_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_NOT_LISTED, true, "JFW_Building_Gun_Animated_Weapon_No_Aircraft_Secondary" };

class JFW_Building_Gun_Animated_No_Aircraft_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_Aircraft_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Weapon_No_Aircraft_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_Aircraft_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_No_Aircraft_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_No_Aircraft_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)


//	Only the eight named presets, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Aircraft_Only_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_ONLY_LISTED, false, "JFW_Building_Gun_Animated_Weapon_Aircraft_Only" };

class JFW_Building_Gun_Animated_Aircraft_Only : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Aircraft_Only_Variant; } };

class JFW_Building_Gun_Animated_Weapon_Aircraft_Only : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Aircraft_Only_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Aircraft_Only,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_Aircraft_Only,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)


//	Only the eight named presets, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Aircraft_Only_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_ONLY_LISTED, true, "JFW_Building_Gun_Animated_Weapon_Aircraft_Only_Secondary" };

class JFW_Building_Gun_Animated_Aircraft_Only_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Aircraft_Only_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Weapon_Aircraft_Only_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Aircraft_Only_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Aircraft_Only_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_Aircraft_Only_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT)


//	Anything that is not flying, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_No_VTOL_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_NOT_VTOL, false, "JFW_Building_Gun_Animated_Weapon_No_VTOL" };

class JFW_Building_Gun_Animated_No_VTOL : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_VTOL_Variant; } };

class JFW_Building_Gun_Animated_Weapon_No_VTOL : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_VTOL_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_No_VTOL,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_No_VTOL,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT)


//	Anything that is not flying, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_No_VTOL_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_NOT_VTOL, true, "JFW_Building_Gun_Animated_Weapon_No_VTOL_Secondary" };

class JFW_Building_Gun_Animated_No_VTOL_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_VTOL_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Weapon_No_VTOL_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_No_VTOL_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_No_VTOL_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_No_VTOL_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT)


//	Only what is flying, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_VTOL_Only_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_VTOL_ONLY, false, "JFW_Building_Gun_Animated_Weapon_VTOL_Only" };

class JFW_Building_Gun_Animated_VTOL_Only : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_VTOL_Only_Variant; } };

class JFW_Building_Gun_Animated_Weapon_VTOL_Only : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_VTOL_Only_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_VTOL_Only,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_VTOL_Only,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT)


//	Only what is flying, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_VTOL_Only_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED, JFW_GUN_FILTER_VTOL_ONLY, true, "JFW_Building_Gun_Animated_Weapon_VTOL_Only_Secondary" };

class JFW_Building_Gun_Animated_VTOL_Only_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_VTOL_Only_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Weapon_VTOL_Only_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_VTOL_Only_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_VTOL_Only_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Weapon_VTOL_Only_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT)


/*
**	The same, and makes a noise winding up.
*/

//	Anything it can see, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_ANY, false, "JFW_Building_Gun_Animated_Sound_Weapon" };

class JFW_Building_Gun_Animated_Sound : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Anything it can see, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_ANY, true, "JFW_Building_Gun_Animated_Sound_Weapon_Secondary" };

class JFW_Building_Gun_Animated_Sound_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Anything but the eight named presets, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_No_Aircraft_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_NOT_LISTED, false, "JFW_Building_Gun_Animated_Sound_Weapon_No_Aircraft" };

class JFW_Building_Gun_Animated_Sound_No_Aircraft : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_Aircraft_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_No_Aircraft : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_Aircraft_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_No_Aircraft,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_No_Aircraft,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Anything but the eight named presets, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_No_Aircraft_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_NOT_LISTED, true, "JFW_Building_Gun_Animated_Sound_Weapon_No_Aircraft_Secondary" };

class JFW_Building_Gun_Animated_Sound_No_Aircraft_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_Aircraft_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_No_Aircraft_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_Aircraft_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_No_Aircraft_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_No_Aircraft_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Only the eight named presets, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_Aircraft_Only_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_ONLY_LISTED, false, "JFW_Building_Gun_Animated_Sound_Weapon_Aircraft_Only" };

class JFW_Building_Gun_Animated_Sound_Aircraft_Only : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Aircraft_Only_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_Aircraft_Only : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Aircraft_Only_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Aircraft_Only,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_Aircraft_Only,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Only the eight named presets, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_Aircraft_Only_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_ONLY_LISTED, true, "JFW_Building_Gun_Animated_Sound_Weapon_Aircraft_Only_Secondary" };

class JFW_Building_Gun_Animated_Sound_Aircraft_Only_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Aircraft_Only_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_Aircraft_Only_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_Aircraft_Only_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Aircraft_Only_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_Aircraft_Only_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_PRESETS JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Anything that is not flying, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_No_VTOL_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_NOT_VTOL, false, "JFW_Building_Gun_Animated_Sound_Weapon_No_VTOL" };

class JFW_Building_Gun_Animated_Sound_No_VTOL : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_VTOL_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_No_VTOL : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_VTOL_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_No_VTOL,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_No_VTOL,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Anything that is not flying, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_No_VTOL_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_NOT_VTOL, true, "JFW_Building_Gun_Animated_Sound_Weapon_No_VTOL_Secondary" };

class JFW_Building_Gun_Animated_Sound_No_VTOL_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_VTOL_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_No_VTOL_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_No_VTOL_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_No_VTOL_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_No_VTOL_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Only what is flying, one fire mode.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_VTOL_Only_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_VTOL_ONLY, false, "JFW_Building_Gun_Animated_Sound_Weapon_VTOL_Only" };

class JFW_Building_Gun_Animated_Sound_VTOL_Only : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_VTOL_Only_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_VTOL_Only : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_VTOL_Only_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_VTOL_Only,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_VTOL_Only,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)


//	Only what is flying, alternating fire.
static const JFWGunVariantStruct _JFW_Building_Gun_Animated_Sound_VTOL_Only_Secondary_Variant =
	{ JFW_GUN_STYLE_ANIMATED_SOUND, JFW_GUN_FILTER_VTOL_ONLY, true, "JFW_Building_Gun_Animated_Sound_Weapon_VTOL_Only_Secondary" };

class JFW_Building_Gun_Animated_Sound_VTOL_Only_Secondary : public JFW_Building_Gun_Mount
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_VTOL_Only_Secondary_Variant; } };

class JFW_Building_Gun_Animated_Sound_Weapon_VTOL_Only_Secondary : public JFW_Building_Gun_Turret
	{ const JFWGunVariantStruct & Variant(void) override { return _JFW_Building_Gun_Animated_Sound_VTOL_Only_Secondary_Variant; } };

REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_VTOL_Only_Secondary,
	JFW_GUN_PARAMS_MOUNT JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)
REGISTER_SCRIPT_TT(JFW_Building_Gun_Animated_Sound_Weapon_VTOL_Only_Secondary,
	JFW_GUN_PARAMS_GUN JFW_GUN_PARAMS_EFFECT JFW_GUN_PARAMS_SOUND)

