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
*     TT_Vehicles.cpp
*
* DESCRIPTION
*     What a vehicle does that the vehicle itself does not know how to do:
*     wear a turret, show the people riding in it, run an animation while it
*     is moving, refuse a driver it does not belong to, refuel on a pad, or
*     quietly delete itself once everybody has walked away from it.
*
*     Native port of the 4.8.4 library's jfwveh.cpp.  The scripts keep their
*     registered names -- including the file's long-standing "Vechicle"
*     misspellings -- so existing levels still find them.
*
*     Three families in that file were the same script written out several
*     times over, and are one script and a virtual here: the motion-driven
*     animations, the visible riders, and the spawned turret.
*
******************************************************************************/

#include "scripts.h"
#include "actionparams.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "vehicle.h"
#include "weaponmanager.h"
#include "weapons.h"
#include "wwstring.h"


/*
**	Action, timer and distance constants the scripts in this file use among
**	themselves.
*/
enum
{
	JFW_VEH_ACTION_DRIVE			= 777,
	JFW_VEH_TIMER_STEALTH			= 1,
	JFW_VEH_TIMER_LOCK				= 2
};

//
//	How close a flying vehicle has to still be to the pad when the reload
//	interval runs out for the pad to hand it another round.
//
const float	JFW_VEH_REFILL_RANGE	= 10.0f;


////////////////////////////////////////////////////////////////////////////
//
//	Animations a vehicle plays because it is moving
//
//	Three scripts in the library polled the vehicle's position on a timer,
//	compared it to where it was last time, and ran an animation while the two
//	differed.  They are one script here; what they disagree about is only what
//	to do at the two edges, and which animation loops.
//
//	One correction.  The originals acted on the stopped branch every tick, not
//	on the edge, which for JFW_Vechicle_Animation_2 meant a parked vehicle
//	restarted its "down" animation once per tick and never got past the first
//	few frames of it.  Both edges fire once here.
//
////////////////////////////////////////////////////////////////////////////

class JFW_Vehicle_Motion_Animation : public ScriptImpClass
{
public:

	bool		Moving;
	float		LastX;
	float		LastY;
	float		LastZ;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Moving, 1);
		SAVE_VARIABLE(LastX, 2);
		SAVE_VARIABLE(LastY, 3);
		SAVE_VARIABLE(LastZ, 4);
	}

	void Created(GameObject* obj) override
	{
		Moving	= false;
		LastX		= 0.0f;
		LastY		= 0.0f;
		LastZ		= 0.0f;

		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), Get_Int_Parameter("TimerNum"));
	}

	void Timer_Expired(GameObject* obj, int /*number*/) override
	{
		Vector3 position = ScriptEngine::Get_Position(obj);

		//
		//	The first tick has nothing to compare against, so it only records
		//	where the vehicle is.
		//
		if (LastX == 0.0f && LastY == 0.0f && LastZ == 0.0f) {
			LastX = position.X;
			LastY = position.Y;
			LastZ = position.Z;
		}

		if (position.X != LastX || position.Y != LastY || position.Z != LastZ) {
			LastX = position.X;
			LastY = position.Y;
			LastZ = position.Z;
			if (!Moving) {
				Moving = true;
				On_Started_Moving(obj);
			}
		} else if (Moving) {
			Moving = false;
			On_Stopped_Moving(obj);
		}

		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), Get_Int_Parameter("TimerNum"));
	}

	void Animation_Complete(GameObject* obj, const char* animation_name) override
	{
		if (animation_name != nullptr && Is_Loop_Animation(animation_name)) {
			Clear(obj);
			Play(obj, "");
		}
	}

	virtual void	On_Started_Moving(GameObject* obj) = 0;
	virtual void	On_Stopped_Moving(GameObject* obj) = 0;

	//
	//	Which animation finishing means "start it again".  The base answer is
	//	the unprefixed one; the up/down variant also loops its up animation.
	//
	virtual bool Is_Loop_Animation(const char* animation_name)
	{
		const char* animation = Get_Parameter("Animation");
		return animation != nullptr && ::stricmp(animation_name, animation) == 0;
	}

	//
	//	Play the animation group named by a prefix -- "" for the plain one,
	//	"Up" and "Down" for the two ends of a travelling part.
	//
	void Play(GameObject* obj, const char* prefix)
	{
		StringClass name;

		name.Format("%sSubobject", prefix);
		const char* subobject = Get_Parameter(name.Peek_Buffer());
		if (subobject != nullptr && ::stricmp(subobject, "0") == 0) {
			subobject = nullptr;
		}

		name.Format("%sFirstFrame", prefix);
		float first_frame = Get_Float_Parameter(name.Peek_Buffer());
		if (first_frame == -1.0f) {
			first_frame = ScriptEngine::Get_Animation_Frame(obj);
		}

		name.Format("%sLastFrame", prefix);
		float last_frame = Get_Float_Parameter(name.Peek_Buffer());

		name.Format("%sBlended", prefix);
		bool blended = Get_Bool_Parameter(name.Peek_Buffer());

		name.Format("%sAnimation", prefix);
		ScriptEngine::Set_Animation(obj, Get_Parameter(name.Peek_Buffer()), false,
			subobject, first_frame, last_frame, blended);
	}

	//
	//	4.8.4 added a Clear_Animation of its own; here the merged
	//	PhysicalGameObj::Set_Animation treats an empty name as the clear.
	//
	static void Clear(GameObject* obj)
	{
		PhysicalGameObj* physical = obj->As_PhysicalGameObj();
		if (physical != nullptr) {
			physical->Set_Animation("", false);
		}
	}
};


#define JFW_MOTION_ANIM_PARAMS	"Animation:string,Subobject:string,FirstFrame:float,"		\
										"LastFrame:float,Blended:int,Time:float,TimerNum:int"

#define JFW_MOTION_ANIM_UPDOWN	",UpAnimation:string,UpSubobject:string,UpFirstFrame:float,"	\
										"UpLastFrame:float,UpBlended:int,DownAnimation:string,"			\
										"DownSubobject:string,DownFirstFrame:float,"							\
										"DownLastFrame:float,DownBlended:int"


/*JFW_Vechicle_Animation

  Runs an animation while the vehicle is moving and clears it when it stops.

  Parameters:

  Animation			= Animation to play while moving.
  Subobject			= Sub-object it lives on, or 0 for the whole model.
  FirstFrame		= Frame to start at, or -1 for wherever it already is.
  LastFrame			= Frame to stop at.
  Blended			= Whether to blend into it.
  Time				= How often to look at whether the vehicle has moved.
  TimerNum			= Timer id to use.
*/

REGISTER_SCRIPT_TT(JFW_Vechicle_Animation, JFW_MOTION_ANIM_PARAMS)
class JFW_Vechicle_Animation : public JFW_Vehicle_Motion_Animation
{
	void On_Started_Moving(GameObject* obj) override	{ Play(obj, ""); }
	void On_Stopped_Moving(GameObject* obj) override	{ Clear(obj); }
};


/*JFW_Vechicle_Animation_2

  As JFW_Vechicle_Animation, but instead of clearing the animation when the
  vehicle stops it plays one animation as the vehicle sets off and another as
  it settles -- undercarriage, outriggers, a raised sensor.

  Parameters:

  Animation ... TimerNum	= The looping animation, as above.
  Up*							= Played once when the vehicle starts moving.
  Down*							= Played once when it stops.
*/

REGISTER_SCRIPT_TT(JFW_Vechicle_Animation_2, JFW_MOTION_ANIM_PARAMS JFW_MOTION_ANIM_UPDOWN)
class JFW_Vechicle_Animation_2 : public JFW_Vehicle_Motion_Animation
{
	void On_Started_Moving(GameObject* obj) override	{ Play(obj, "Up"); }
	void On_Stopped_Moving(GameObject* obj) override	{ Play(obj, "Down"); }

	bool Is_Loop_Animation(const char* animation_name) override
	{
		if (JFW_Vehicle_Motion_Animation::Is_Loop_Animation(animation_name)) {
			return true;
		}

		const char* up = Get_Parameter("UpAnimation");
		return up != nullptr && ::stricmp(animation_name, up) == 0;
	}
};


/*JFW_Vechicle_Animation_Trigger

  As JFW_Vechicle_Animation, and the up and down animations are played on
  demand by two customs rather than by the vehicle moving.

  Parameters:

  Animation ... TimerNum	= The looping animation, as above.
  Up*							= Animation the UpTrigger custom plays.
  Down*							= Animation the DownTrigger custom plays.
  UpTrigger					= Custom that plays the up animation.
  DownTrigger				= Custom that plays the down animation.
*/

REGISTER_SCRIPT_TT(JFW_Vechicle_Animation_Trigger,
	JFW_MOTION_ANIM_PARAMS JFW_MOTION_ANIM_UPDOWN ",UpTrigger:int,DownTrigger:int")
class JFW_Vechicle_Animation_Trigger : public JFW_Vehicle_Motion_Animation
{
	void On_Started_Moving(GameObject* obj) override	{ Play(obj, ""); }
	void On_Stopped_Moving(GameObject* obj) override	{ Clear(obj); }

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("UpTrigger")) {
			Play(obj, "Up");
		}

		if (type == Get_Int_Parameter("DownTrigger")) {
			Play(obj, "Down");
		}
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	People you can see riding in the vehicle
//
//	A Renegade occupant is inside the hull and invisible.  These scripts put a
//	model on a bone for each rider so a transport looks occupied.  Six scripts
//	in the library did this; they differ only in how many riders they show,
//	which bone each one sits on, and who decides what the rider looks like --
//	the script's own parameters, or the rider's own preset, asked by custom.
//
//	Two corrections.  The originals matched a leaving rider to a model by
//	position in the list rather than by who left, so the wrong model could
//	vanish; and the seat-range variant indexed its array by the seat numbers a
//	level gave it without checking them against the vehicle's actual seat
//	count.  Riders are tracked by id here, and the range is clamped.
//
////////////////////////////////////////////////////////////////////////////

struct JFW_Rider_Slot
{
	JFW_Rider_Slot (void)
		: ModelID (0), RiderID (0)		{ }

	int			ModelID;
	int			RiderID;

	bool operator== (const JFW_Rider_Slot &that) const
		{ return ModelID == that.ModelID && RiderID == that.RiderID; }
	bool operator!= (const JFW_Rider_Slot &that) const
		{ return !(*this == that); }
};


class JFW_Vehicle_Riders : public ScriptImpClass
{
public:

	//
	//	Saved as one blob, so it has to be a fixed size.  No Renegade vehicle
	//	comes close to this; a level that did would simply not dress the seats
	//	past the end.
	//
	enum { RIDER_MAX_SEATS = 16 };

	JFW_Rider_Slot	Slots[RIDER_MAX_SEATS];
	int				SlotCount;
	bool				Stealthed;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Slots, 1);
		SAVE_VARIABLE(SlotCount, 2);
		SAVE_VARIABLE(Stealthed, 3);
	}

	void Created(GameObject* obj) override
	{
		SlotCount = Slot_Count(obj);
		if (SlotCount > RIDER_MAX_SEATS) {
			SlotCount = RIDER_MAX_SEATS;
		}

		Stealthed = false;

		if (Tracks_Stealth()) {
			ScriptEngine::Start_Timer(obj, this, 1.0f, JFW_VEH_TIMER_STEALTH);
		}
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			Dress_Rider(obj, sender);
		}

		if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			int rider = ScriptEngine::Get_ID(sender);
			for (int index = 0; index < SlotCount; index ++) {
				if (Slots[index].RiderID == rider) {
					Release(index);
				}
			}
		}
	}

	void Killed(GameObject* /*obj*/, GameObject* /*killer*/) override
	{
		for (int index = 0; index < SlotCount; index ++) {
			Release(index);
		}
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number != JFW_VEH_TIMER_STEALTH) {
			return ;
		}

		bool stealthed = ScriptEngine::Is_Stealth(obj);
		if (stealthed != Stealthed) {
			Stealthed = stealthed;
			for (int index = 0; index < SlotCount; index ++) {
				if (Slots[index].ModelID == 0) {
					continue;
				}

				GameObject* model = ScriptEngine::Find_Object(Slots[index].ModelID);
				if (model != nullptr) {
					ScriptEngine::Set_Is_Rendered(model, !Stealthed);
				}
			}
		}

		ScriptEngine::Start_Timer(obj, this, 1.0f, JFW_VEH_TIMER_STEALTH);
	}

protected:

	//
	//	How many riders this script can show, which slot a given rider takes
	//	(-1 for one it does not show), the bone that slot sits on, and how the
	//	rider gets its appearance.
	//
	virtual int		Slot_Count(GameObject* obj) = 0;
	virtual int		Slot_For(GameObject* obj, GameObject* rider) = 0;
	virtual bool	Bone_Name(int slot, StringClass& name) = 0;
	virtual void	Dress(GameObject* obj, GameObject* model, GameObject* rider, int slot) = 0;

	//
	//	Whether the riders disappear along with a stealthed vehicle.  The two
	//	scripts that carry the model in their own parameters never did this.
	//
	virtual bool	Tracks_Stealth(void)		{ return true; }

	void Dress_Rider(GameObject* obj, GameObject* rider)
	{
		int slot = Slot_For(obj, rider);
		if (slot < 0 || slot >= SlotCount || Slots[slot].ModelID != 0) {
			return ;
		}

		StringClass bone;
		if (!Bone_Name(slot, bone)) {
			return ;
		}

		Vector3 position = ScriptEngine::Get_Bone_Position(obj, bone.Peek_Buffer());
		GameObject* model = ScriptEngine::Create_Object("Invisible_Object", position);
		if (model == nullptr) {
			return ;
		}

		ScriptEngine::Attach_To_Object_Bone(model, obj, bone.Peek_Buffer());

		Slots[slot].ModelID	= ScriptEngine::Get_ID(model);
		Slots[slot].RiderID	= ScriptEngine::Get_ID(rider);

		Dress(obj, model, rider, slot);
		ScriptEngine::Set_Is_Rendered(model, !Stealthed);
	}

	void Release(int slot)
	{
		if (Slots[slot].ModelID != 0) {
			GameObject* model = ScriptEngine::Find_Object(Slots[slot].ModelID);
			if (model != nullptr) {
				ScriptEngine::Destroy_Object(model);
			}
		}

		Slots[slot].ModelID	= 0;
		Slots[slot].RiderID	= 0;
	}

	//
	//	Apply the model and animation named by this script's own parameters.
	//	The suffix is "" for the single-rider script and "1"/"2" for the pair.
	//
	void Dress_From_Parameters(GameObject* obj, GameObject* model, const char* suffix)
	{
		StringClass name;

		name.Format("ModelName%s", suffix);
		ScriptEngine::Set_Model(model, Get_Parameter(name.Peek_Buffer()));

		name.Format("Subobject%s", suffix);
		const char* subobject = Get_Parameter(name.Peek_Buffer());
		if (subobject != nullptr && ::stricmp(subobject, "0") == 0) {
			subobject = nullptr;
		}

		name.Format("FirstFrame%s", suffix);
		float first_frame = Get_Float_Parameter(name.Peek_Buffer());
		if (first_frame == -1.0f) {
			first_frame = ScriptEngine::Get_Animation_Frame(obj);
		}

		name.Format("LastFrame%s", suffix);
		float last_frame = Get_Float_Parameter(name.Peek_Buffer());

		name.Format("Blended%s", suffix);
		bool blended = Get_Bool_Parameter(name.Peek_Buffer());

		name.Format("Animation%s", suffix);
		ScriptEngine::Set_Animation(model, Get_Parameter(name.Peek_Buffer()), false,
			subobject, first_frame, last_frame, blended);
	}

	//
	//	Ask the rider's own preset what it should look like, handing it the id
	//	of the model object to dress.  Whatever answers is a
	//	JFW_Visible_Person_Settings on the soldier.
	//
	void Dress_By_Custom(GameObject* obj, GameObject* model, GameObject* rider, const char* message_name)
	{
		ScriptEngine::Send_Custom_Event(obj, rider, Get_Int_Parameter(message_name),
			ScriptEngine::Get_ID(model), 0);
	}
};


/*JFW_Visible_Person_In_Vechicle

  Shows one rider on a named bone, wearing the model and animation this script
  names.  What the rider actually is makes no difference.

  Parameters:

  BoneName			= Bone the rider sits on.
  ModelName			= Model to give it.
  Animation			= Animation to play on that model.
  SubObject			= Sub-object the animation lives on, or 0.
  FirstFrame		= Frame to start at, or -1 for wherever it already is.
  LastFrame			= Frame to stop at.
  Blended			= Whether to blend into it.
*/

REGISTER_SCRIPT_TT(JFW_Visible_Person_In_Vechicle,
	"BoneName:string,ModelName:string,Animation:string,SubObject:string,"
	"FirstFrame:float,LastFrame:float,Blended:int")
class JFW_Visible_Person_In_Vechicle : public JFW_Vehicle_Riders
{
	int  Slot_Count(GameObject* /*obj*/) override							{ return 1; }
	int  Slot_For(GameObject* /*obj*/, GameObject* /*rider*/) override	{ return 0; }
	bool Tracks_Stealth(void) override											{ return false; }

	bool Bone_Name(int /*slot*/, StringClass& name) override
	{
		name = Get_Parameter("BoneName");
		return !name.Is_Empty();
	}

	void Dress(GameObject* obj, GameObject* model, GameObject* /*rider*/, int /*slot*/) override
	{
		Dress_From_Parameters(obj, model, "");
	}
};


/*JFW_Visible_People_In_Vechicle

  As JFW_Visible_Person_In_Vechicle, for two riders on two bones.  The first
  free slot is used, so the second rider gets the second bone.

  Parameters:

  BoneName1 ... Blended1	= The first rider.
  BoneName2 ... Blended2	= The second rider.
*/

REGISTER_SCRIPT_TT(JFW_Visible_People_In_Vechicle,
	"BoneName1:string,ModelName1:string,Animation1:string,SubObject1:string,"
	"FirstFrame1:float,LastFrame1:float,Blended1:int,"
	"BoneName2:string,ModelName2:string,Animation2:string,SubObject2:string,"
	"FirstFrame2:float,LastFrame2:float,Blended2:int")
class JFW_Visible_People_In_Vechicle : public JFW_Vehicle_Riders
{
	int  Slot_Count(GameObject* /*obj*/) override		{ return 2; }
	bool Tracks_Stealth(void) override						{ return false; }

	int Slot_For(GameObject* /*obj*/, GameObject* /*rider*/) override
	{
		for (int index = 0; index < 2; index ++) {
			if (Slots[index].ModelID == 0) {
				return index;
			}
		}

		return -1;
	}

	bool Bone_Name(int slot, StringClass& name) override
	{
		StringClass parameter;
		parameter.Format("BoneName%d", slot + 1);
		name = Get_Parameter(parameter.Peek_Buffer());
		return !name.Is_Empty();
	}

	void Dress(GameObject* obj, GameObject* model, GameObject* /*rider*/, int slot) override
	{
		StringClass suffix;
		suffix.Format("%d", slot + 1);
		Dress_From_Parameters(obj, model, suffix.Peek_Buffer());
	}
};


/*JFW_Per_Preset_Visible_Person_In_Vechicle

  Shows one rider on a named bone, and asks the rider itself what it should
  look like rather than saying so here -- so an engineer and a sniper riding
  the same transport look different.  The rider answers with a
  JFW_Visible_Person_Settings of its own.  Hides the rider when the vehicle
  stealths.

  Parameters:

  BoneName			= Bone the rider sits on.
  Message			= Custom to send the rider, carrying the model's id.
*/

REGISTER_SCRIPT_TT(JFW_Per_Preset_Visible_Person_In_Vechicle, "BoneName:string,Message:int")
class JFW_Per_Preset_Visible_Person_In_Vechicle : public JFW_Vehicle_Riders
{
	int  Slot_Count(GameObject* /*obj*/) override							{ return 1; }
	int  Slot_For(GameObject* /*obj*/, GameObject* /*rider*/) override	{ return 0; }

	bool Bone_Name(int /*slot*/, StringClass& name) override
	{
		name = Get_Parameter("BoneName");
		return !name.Is_Empty();
	}

	void Dress(GameObject* obj, GameObject* model, GameObject* rider, int /*slot*/) override
	{
		Dress_By_Custom(obj, model, rider, "Message");
	}
};


/*JFW_Per_Preset_Visible_People_In_Vechicle

  As JFW_Per_Preset_Visible_Person_In_Vechicle, for the first two seats, each
  with its own bone and its own custom.

  Parameters:

  BoneName1			= Bone the driver sits on.
  BoneName2			= Bone the passenger sits on.
  Message1			= Custom to send the driver.
  Message2			= Custom to send the passenger.
*/

REGISTER_SCRIPT_TT(JFW_Per_Preset_Visible_People_In_Vechicle,
	"BoneName1:string,BoneName2:string,Message1:int,Message2:int")
class JFW_Per_Preset_Visible_People_In_Vechicle : public JFW_Vehicle_Riders
{
	int Slot_Count(GameObject* /*obj*/) override		{ return 2; }

	int Slot_For(GameObject* obj, GameObject* rider) override
	{
		return ScriptEngine::Get_Occupant_Seat(obj, rider);
	}

	bool Bone_Name(int slot, StringClass& name) override
	{
		StringClass parameter;
		parameter.Format("BoneName%d", slot + 1);
		name = Get_Parameter(parameter.Peek_Buffer());
		return !name.Is_Empty();
	}

	void Dress(GameObject* obj, GameObject* model, GameObject* rider, int slot) override
	{
		StringClass message;
		message.Format("Message%d", slot + 1);
		Dress_By_Custom(obj, model, rider, message.Peek_Buffer());
	}
};


/*JFW_Per_Preset_Visible_Multiple_People_In_Vehicle

  As JFW_Per_Preset_Visible_Person_In_Vechicle, for every seat the vehicle
  has.  The bones are named SEAT0, SEAT1 and so on.

  Parameters:

  Message			= Custom to send each rider, carrying the model's id.
*/

REGISTER_SCRIPT_TT(JFW_Per_Preset_Visible_Multiple_People_In_Vehicle, "Message:int")
class JFW_Per_Preset_Visible_Multiple_People_In_Vehicle : public JFW_Vehicle_Riders
{
	int Slot_Count(GameObject* obj) override
	{
		return ScriptEngine::Get_Vehicle_Seat_Count(obj);
	}

	int Slot_For(GameObject* obj, GameObject* rider) override
	{
		return ScriptEngine::Get_Occupant_Seat(obj, rider);
	}

	bool Bone_Name(int slot, StringClass& name) override
	{
		name.Format("SEAT%d", slot);
		return true;
	}

	void Dress(GameObject* obj, GameObject* model, GameObject* rider, int /*slot*/) override
	{
		Dress_By_Custom(obj, model, rider, "Message");
	}
};


/*JFW_Per_Preset_Visible_Multiple_People_In_Vehicle_2

  As JFW_Per_Preset_Visible_Multiple_People_In_Vehicle, but only for a range
  of seats -- for a vehicle where the driver is under armour and the rest are
  sitting out in the open.

  Parameters:

  Message			= Custom to send each rider, carrying the model's id.
  FirstSeat			= First seat to show.
  LastSeat			= Last seat to show.
*/

REGISTER_SCRIPT_TT(JFW_Per_Preset_Visible_Multiple_People_In_Vehicle_2,
	"Message:int,FirstSeat:int,LastSeat:int")
class JFW_Per_Preset_Visible_Multiple_People_In_Vehicle_2 : public JFW_Vehicle_Riders
{
	int Slot_Count(GameObject* obj) override
	{
		return ScriptEngine::Get_Vehicle_Seat_Count(obj);
	}

	int Slot_For(GameObject* obj, GameObject* rider) override
	{
		int seat = ScriptEngine::Get_Occupant_Seat(obj, rider);
		if (seat < Get_Int_Parameter("FirstSeat") || seat > Get_Int_Parameter("LastSeat")) {
			return -1;
		}

		return seat;
	}

	bool Bone_Name(int slot, StringClass& name) override
	{
		name.Format("SEAT%d", slot);
		return true;
	}

	void Dress(GameObject* obj, GameObject* model, GameObject* rider, int /*slot*/) override
	{
		Dress_By_Custom(obj, model, rider, "Message");
	}
};


/*JFW_Visible_Person_Settings

  Sits on a soldier and answers the vehicle's question about what that soldier
  looks like as a passenger.

  Parameters:

  Message			= Custom the vehicle sends; the model's id arrives with it.
  ModelName			= Model to give it.
  Animation			= Animation to play on that model.
  SubObject			= Sub-object the animation lives on, or 0.
  FirstFrame		= Frame to start at, or -1 for wherever it already is.
  LastFrame			= Frame to stop at.
  Blended			= Whether to blend into it.
*/

DECLARE_SCRIPT_TT(JFW_Visible_Person_Settings,
	"Message:int,ModelName:string,Animation:string,SubObject:string,"
	"FirstFrame:float,LastFrame:float,Blended:int")
{
	void Custom(GameObject* obj, int type, intptr_t param, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return ;
		}

		GameObject* model = ScriptEngine::Find_Object((int)param);
		if (model == nullptr) {
			return ;
		}

		ScriptEngine::Set_Model(model, Get_Parameter("ModelName"));

		const char* subobject = Get_Parameter("Subobject");
		if (subobject != nullptr && ::stricmp(subobject, "0") == 0) {
			subobject = nullptr;
		}

		float first_frame = Get_Float_Parameter("FirstFrame");
		if (first_frame == -1.0f) {
			first_frame = ScriptEngine::Get_Animation_Frame(obj);
		}

		ScriptEngine::Set_Animation(model, Get_Parameter("Animation"), false, subobject,
			first_frame, Get_Float_Parameter("LastFrame"), Get_Bool_Parameter("Blended"));
	}
};


/*JFW_Visible_Person_Settings_2

  As JFW_Visible_Person_Settings, for a rider that holds a single pose rather
  than playing an animation.

  Parameters:

  Message			= Custom the vehicle sends; the model's id arrives with it.
  ModelName			= Model to give it.
  Animation			= Animation the pose is taken from.
  Frame				= Frame of it to hold.
*/

DECLARE_SCRIPT_TT(JFW_Visible_Person_Settings_2,
	"Message:int,ModelName:string,Animation:string,Frame:float")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t param, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return ;
		}

		GameObject* model = ScriptEngine::Find_Object((int)param);
		if (model == nullptr) {
			return ;
		}

		ScriptEngine::Set_Model(model, Get_Parameter("ModelName"));
		ScriptEngine::Set_Animation_Frame(model, Get_Parameter("Animation"), Get_Int_Parameter("Frame"));
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	A turret the vehicle wears
//
//	The turret is a separate object bolted to a bone, so that it can aim
//	independently of the hull.  Two scripts in the library created one; the
//	second added a driver's side, a check that the preset actually exists, and
//	a health link so that shooting the turret hurts the vehicle.
//
////////////////////////////////////////////////////////////////////////////

class JFW_Turret_Spawn : public ScriptImpClass
{
public:

	int	TurretID;
	bool	HasDriver;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(TurretID, 1);
		SAVE_VARIABLE(HasDriver, 2);
	}

	void Created(GameObject* obj) override
	{
		TurretID		= 0;
		HasDriver	= false;

		Vector3 origin(0.0f, 0.0f, 0.0f);
		GameObject* turret = ScriptEngine::Create_Object(Get_Parameter("Turret_Preset"), origin);
		if (turret == nullptr) {
			ScriptEngine::Console_Output(
				"[%d:%s:%s] Critical Error: Failed to create an instance of the preset %s. Destroying script...\n",
				ScriptEngine::Get_ID(obj), ScriptEngine::Get_Preset_Name(obj), Get_Name(),
				Get_Parameter("Turret_Preset"));
			Destroy_Script();
			return ;
		}

		ScriptEngine::Attach_To_Object_Bone(turret, obj, Get_Parameter("Bone_Name"));
		TurretID = ScriptEngine::Get_ID(turret);

		On_Turret_Created(obj, turret);
	}

	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		Vector3 position = ScriptEngine::Get_Bone_Position(obj, Get_Parameter("Explosion_Bone"));
		ScriptEngine::Create_Explosion(Get_Parameter("Explosion_Preset"), position, nullptr);
	}

protected:

	virtual void	On_Turret_Created(GameObject* /*obj*/, GameObject* /*turret*/)	{ }

	GameObject* Turret(void)
	{
		return TurretID != 0 ? ScriptEngine::Find_Object(TurretID) : nullptr;
	}
};


/*JFW_Turret_Spawn_2

  Bolts a turret preset to a bone on this vehicle, and hands the turret over
  to whichever side the driver is on.

  Parameters:

  Turret_Preset		= Preset to bolt on.
  Bone_Name			= Bone to bolt it to.
  Explosion_Preset	= Explosion to make when the vehicle dies.
  Explosion_Bone		= Bone to make it at.
*/

REGISTER_SCRIPT_TT(JFW_Turret_Spawn_2,
	"Turret_Preset:string,Bone_Name=Tur_Mount:string,Explosion_Preset:string,Explosion_Bone:string")
class JFW_Turret_Spawn_2 : public JFW_Turret_Spawn
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_VEHICLE_ENTERED) {
			return ;
		}

		GameObject* turret = Turret();
		if (turret != nullptr) {
			ScriptEngine::Set_Player_Type(turret, ScriptEngine::Get_Player_Type(sender));
		}
	}
};


/*JFW_Turret_Spawn_3

  As JFW_Turret_Spawn_2, and tells the turret when it gains and loses a driver
  so that it can raise, stow or power down.  The turret is kept out of the
  scripts list, and its health is tied to the vehicle's in both directions --
  4.8.4 reached that by attaching a script from the dp88 library, which is
  outside the scope of this port; the file's own JFW_Vehicle_Extra_Attach does
  the same job and is used instead.

  Parameters:

  Turret_Preset			= Preset to bolt on.
  Bone_Name				= Bone to bolt it to.
  Explosion_Preset		= Explosion to make when the vehicle dies.
  Explosion_Bone			= Bone to make it at.
  Driver_Entered_Custom	= Custom to send the turret when a driver gets in.
  Driver_Exited_Custom	= Custom to send it when the last one gets out.
*/

REGISTER_SCRIPT_TT(JFW_Turret_Spawn_3,
	"Turret_Preset:string,Bone_Name=Tur_Mount:string,Explosion_Preset:string,"
	"Explosion_Bone:string,Driver_Entered_Custom:int,Driver_Exited_Custom:int")
class JFW_Turret_Spawn_3 : public JFW_Turret_Spawn
{
	void On_Turret_Created(GameObject* obj, GameObject* turret) override
	{
		ScriptEngine::Send_Custom_Event(obj, turret, Get_Int_Parameter("Driver_Exited_Custom"), 0, 0);
		ScriptEngine::Attach_Script_Once_V(turret, "JFW_Vehicle_Extra_Attach", "%d", ScriptEngine::Get_ID(obj));

		PhysicalGameObj* physical = turret->As_PhysicalGameObj();
		VehicleGameObj* vehicle = physical != nullptr ? physical->As_VehicleGameObj() : nullptr;
		if (vehicle != nullptr) {
			vehicle->Set_Is_Scripts_Visible(false);
		}
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		GameObject* turret = Turret();
		if (turret == nullptr) {
			return ;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED && !HasDriver) {
			HasDriver = true;
			ScriptEngine::Set_Player_Type(turret, ScriptEngine::Get_Player_Type(sender));
			ScriptEngine::Send_Custom_Event(obj, turret, Get_Int_Parameter("Driver_Entered_Custom"), 0, 0);
		}

		if (type == CUSTOM_EVENT_VEHICLE_EXITED && HasDriver
			&& ScriptEngine::Get_Vehicle_Occupant(obj, 0) == nullptr) {
			HasDriver = false;
			ScriptEngine::Send_Custom_Event(obj, turret, Get_Int_Parameter("Driver_Exited_Custom"), 0, 0);
		}
	}

	//
	//	The other half of the health link: damage to the hull shows on the
	//	turret.  JFW_Vehicle_Extra_Attach on the turret carries it the other
	//	way.
	//
	void Damaged(GameObject* obj, GameObject* /*damager*/, float /*amount*/) override
	{
		GameObject* turret = Turret();
		if (turret == nullptr) {
			return ;
		}

		ScriptEngine::Set_Health(turret, ScriptEngine::Get_Health(obj));
		ScriptEngine::Set_Shield_Strength(turret, ScriptEngine::Get_Shield_Strength(obj));
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Bolted-on parts that are not turrets
//
////////////////////////////////////////////////////////////////////////////

class JFW_Vehicle_Extra_Base : public ScriptImpClass
{
public:

	int	ExtraID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(ExtraID, 1);
	}

	void Created(GameObject* obj) override
	{
		ExtraID = 0;

		GameObject* extra = ScriptEngine::Create_Object_At_Bone(obj,
			Get_Parameter("Extra_Preset"), Get_Parameter("Bone_Name"));
		if (extra == nullptr) {
			return ;
		}

		ScriptEngine::Attach_To_Object_Bone(extra, obj, Get_Parameter("Bone_Name"));
		ExtraID = ScriptEngine::Get_ID(extra);

		On_Extra_Created(obj, extra);
	}

	void Killed(GameObject* /*obj*/, GameObject* /*killer*/) override
	{
		GameObject* extra = Extra();
		if (extra != nullptr) {
			ScriptEngine::Destroy_Object(extra);
		}
	}

protected:

	virtual void	On_Extra_Created(GameObject* /*obj*/, GameObject* /*extra*/)	{ }

	GameObject* Extra(void)
	{
		return ExtraID != 0 ? ScriptEngine::Find_Object(ExtraID) : nullptr;
	}
};


/*JFW_Vehicle_Extra

  Bolts a second object to a bone on this vehicle and destroys it along with
  the vehicle.  Decoration: it has no side and takes no damage of its own.

  Parameters:

  Extra_Preset		= Preset to bolt on.
  Bone_Name			= Bone to bolt it to.
*/

REGISTER_SCRIPT_TT(JFW_Vehicle_Extra, "Extra_Preset:string,Bone_Name:string")
class JFW_Vehicle_Extra : public JFW_Vehicle_Extra_Base
{
};


/*JFW_Vehicle_Extra_2

  As JFW_Vehicle_Extra, but the bolted-on object is part of the vehicle: it
  shares the vehicle's health, it changes sides with the driver, and it goes
  neutral again once the vehicle is empty.  Anybody on the wrong side sitting
  in it when a driver arrives is put out.

  Parameters:

  Extra_Preset		= Preset to bolt on.
  Bone_Name			= Bone to bolt it to.
*/

REGISTER_SCRIPT_TT(JFW_Vehicle_Extra_2, "Extra_Preset:string,Bone_Name:string")
class JFW_Vehicle_Extra_2 : public JFW_Vehicle_Extra_Base
{
	void On_Extra_Created(GameObject* obj, GameObject* extra) override
	{
		ScriptEngine::Attach_Script_Once_V(extra, "JFW_Vehicle_Extra_Attach", "%d", ScriptEngine::Get_ID(obj));
		ScriptEngine::Set_Player_Type(extra, ScriptEngine::Get_Player_Type(obj));
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		GameObject* extra = Extra();
		if (extra == nullptr) {
			return ;
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			int player_type = ScriptEngine::Get_Player_Type(sender);
			ScriptEngine::Set_Player_Type(extra, player_type);
			ScriptEngine::Force_Occupants_Exit_Team(obj, player_type);
		} else if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			ScriptEngine::Set_Player_Type(extra, PLAYERTYPE_NEUTRAL);
		}
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float /*amount*/) override
	{
		GameObject* extra = Extra();
		if (extra == nullptr) {
			return ;
		}

		ScriptEngine::Set_Health(extra, ScriptEngine::Get_Health(obj));
		ScriptEngine::Set_Shield_Strength(extra, ScriptEngine::Get_Shield_Strength(obj));
	}
};


/*JFW_Vehicle_Extra_Attach

  Sits on the bolted-on part and carries its damage back to the vehicle it
  belongs to, taking the vehicle with it when it dies.

  Parameters:

  Main_Object		= Id of the object this part belongs to.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Extra_Attach, "Main_Object:int")
{
	void Killed(GameObject* /*obj*/, GameObject* /*killer*/) override
	{
		GameObject* main_object = ScriptEngine::Find_Object(Get_Int_Parameter("Main_Object"));
		if (main_object != nullptr) {
			ScriptEngine::Destroy_Object(main_object);
		}
	}

	void Damaged(GameObject* obj, GameObject* /*damager*/, float /*amount*/) override
	{
		GameObject* main_object = ScriptEngine::Find_Object(Get_Int_Parameter("Main_Object"));
		if (main_object == nullptr) {
			return ;
		}

		ScriptEngine::Set_Health(main_object, ScriptEngine::Get_Health(obj));
		ScriptEngine::Set_Shield_Strength(main_object, ScriptEngine::Get_Shield_Strength(obj));
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Who is allowed to drive
//
////////////////////////////////////////////////////////////////////////////

class JFW_Vehicle_Model_On_Entry : public ScriptImpClass
{
public:

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED && Matches(sender)) {
			ScriptEngine::Set_Model(obj, Get_Parameter("Model"));
		}
	}

protected:

	virtual bool	Matches(GameObject* sender) = 0;
};


/*JFW_Vehicle_Model_Team

  Gives the vehicle a different model when somebody from a particular side
  gets in -- markings, a different crew, a different paint job.

  Parameters:

  Model				= Model to switch to.
  Player_Type		= Side it applies to.
*/

REGISTER_SCRIPT_TT(JFW_Vehicle_Model_Team, "Model:string,Player_Type:int")
class JFW_Vehicle_Model_Team : public JFW_Vehicle_Model_On_Entry
{
	bool Matches(GameObject* sender) override
	{
		return Is_Player_Type(sender, Get_Int_Parameter("Player_Type"));
	}
};


/*JFW_Vehicle_Model_Preset

  As JFW_Vehicle_Model_Team, keyed on exactly which character got in.

  Parameters:

  Model				= Model to switch to.
  Preset				= Character preset it applies to.
*/

REGISTER_SCRIPT_TT(JFW_Vehicle_Model_Preset, "Model:string,Preset:string")
class JFW_Vehicle_Model_Preset : public JFW_Vehicle_Model_On_Entry
{
	bool Matches(GameObject* sender) override
	{
		const char* preset = ScriptEngine::Get_Preset_Name(sender);
		return preset != nullptr && ::stricmp(preset, Get_Parameter("Preset")) == 0;
	}
};


/*JFW_Vehicle_Model_Weapon

  As JFW_Vehicle_Model_Team, keyed on what the driver is carrying.

  Parameters:

  Model				= Model to switch to.
  CharWeapon		= Weapon the driver has to be holding.
*/

REGISTER_SCRIPT_TT(JFW_Vehicle_Model_Weapon, "Model:string,CharWeapon:string")
class JFW_Vehicle_Model_Weapon : public JFW_Vehicle_Model_On_Entry
{
	bool Matches(GameObject* sender) override
	{
		return ScriptEngine::Has_Weapon(sender, Get_Parameter("CharWeapon"));
	}
};


/*JFW_Vehicle_Block_Preset

  Puts one particular character straight back out again.  For a vehicle a
  given class is not supposed to be able to drive.

  Parameters:

  Preset				= Character preset that is not allowed in.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Block_Preset, "Preset:string")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_VEHICLE_ENTERED) {
			return ;
		}

		const char* preset = ScriptEngine::Get_Preset_Name(sender);
		if (preset != nullptr && ::stricmp(preset, Get_Parameter("Preset")) == 0) {
			ScriptEngine::Soldier_Transition_Vehicle(sender);
		}
	}
};


/*JFW_Vehicle_Lock

  Locks the vehicle to one player.  A custom claims it -- for whoever is
  driving, or for whichever id arrives with the custom -- and from then on
  anybody else who climbs in is put straight back out, unless they are
  carrying the script that marks them as a spy.  The claim can be made to
  lapse after a while.

  Only a lone occupant in the driver's seat is ejected: somebody who got into
  a passenger seat of a vehicle that already has a driver is a passenger, not
  a thief.

  Parameters:

  Message			= Custom that claims the vehicle.
  Spy_Script		= Script whose presence lets somebody in anyway.
  Time				= Seconds the claim lasts, or 0 to keep it.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Lock, "Message:int,Spy_Script:string,Time:float")
{
	int	OwnerID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(OwnerID, 1);
	}

	void Created(GameObject* /*obj*/) override
	{
		OwnerID = 0;
	}

	void Custom(GameObject* obj, int type, intptr_t param, GameObject* sender) override
	{
		if (type == Get_Int_Parameter("Message")) {
			if (param != 0) {
				OwnerID = (int)param;
			} else {
				OwnerID = ScriptEngine::Get_ID(ScriptEngine::Get_Vehicle_Driver(obj));
			}

			GameObject* owner = ScriptEngine::Find_Object(OwnerID);
			if (owner != nullptr) {
				ScriptEngine::Set_Player_Type(obj, ScriptEngine::Get_Player_Type(owner));
			}

			if (Get_Float_Parameter("Time") != 0.0f) {
				ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), JFW_VEH_TIMER_LOCK);
			}
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			if (OwnerID == 0 || ScriptEngine::Find_Object(OwnerID) == nullptr) {
				return ;
			}

			if (ScriptEngine::Get_ID(sender) == OwnerID
				|| ScriptEngine::Is_Script_Attached(sender, Get_Parameter("Spy_Script"))) {
				return ;
			}

			if (ScriptEngine::Get_Vehicle_Occupant_Count(obj) == 1
				&& ScriptEngine::Get_Vehicle_Occupant(obj, 0) == sender) {
				ScriptEngine::Force_Occupant_Exit(obj, 0);
			}
		}
	}

	void Timer_Expired(GameObject* /*obj*/, int number) override
	{
		if (number == JFW_VEH_TIMER_LOCK) {
			OwnerID = 0;
		}
	}
};


/*JFW_Vehicle_Effect_Animation

  Runs an animation for as long as anybody is aboard -- an idling engine, a
  lit interior, a spinning rotor.  Counts occupants, so it starts with the
  first one in and stops with the last one out.

  Parameters:

  Animation			= Animation to loop.
  Subobject			= Sub-object it lives on, or 0 for the whole model.
  FirstFrame		= Frame to start at, or -1 for wherever it already is.
  LastFrame			= Frame to stop at.
  Blended			= Whether to blend into it.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Effect_Animation,
	"Animation:string,Subobject:string,FirstFrame:float,LastFrame:float,Blended:int")
{
	bool	Running;
	int	Occupants;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Running, 1);
		SAVE_VARIABLE(Occupants, 2);
	}

	void Created(GameObject* /*obj*/) override
	{
		Running		= false;
		Occupants	= 0;
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_ENTERED) {
			Occupants ++;
			if (!Running && Occupants == 1) {
				Running = true;
				Play(obj);
			}
		}

		if (type == CUSTOM_EVENT_VEHICLE_EXITED) {
			if (Running && Occupants == 1) {
				Running = false;
				PhysicalGameObj* physical = obj->As_PhysicalGameObj();
				if (physical != nullptr) {
					physical->Set_Animation("", false);
				}
			}

			if (Occupants > 0) {
				Occupants --;
			}
		}
	}

	void Animation_Complete(GameObject* obj, const char* animation_name) override
	{
		const char* animation = Get_Parameter("Animation");
		if (animation_name == nullptr || animation == nullptr
			|| ::stricmp(animation_name, animation) != 0) {
			return ;
		}

		PhysicalGameObj* physical = obj->As_PhysicalGameObj();
		if (physical != nullptr) {
			physical->Set_Animation("", false);
		}

		Play(obj);
	}

	void Play(GameObject* obj)
	{
		const char* subobject = Get_Parameter("Subobject");
		if (subobject != nullptr && ::stricmp(subobject, "0") == 0) {
			subobject = nullptr;
		}

		float first_frame = Get_Float_Parameter("FirstFrame");
		if (first_frame == -1.0f) {
			first_frame = ScriptEngine::Get_Animation_Frame(obj);
		}

		ScriptEngine::Set_Animation(obj, Get_Parameter("Animation"), false, subobject,
			first_frame, Get_Float_Parameter("LastFrame"), Get_Bool_Parameter("Blended"));
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Repair, refuel and rearm
//
////////////////////////////////////////////////////////////////////////////

class JFW_Vehicle_Regen : public ScriptImpClass
{
public:

	void Created(GameObject* obj) override
	{
		ScriptEngine::Send_Custom_Event(obj, obj, 0, 0, 0);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != 0) {
			return ;
		}

		if (ScriptEngine::Get_Health(obj) < ScriptEngine::Get_Max_Health(obj)) {
			ScriptEngine::Apply_Damage(obj, Get_Float_Parameter("Health"), Warhead(), nullptr);
		}

		ScriptEngine::Send_Custom_Event(obj, obj, 0, 0, Get_Float_Parameter("Time"));
	}

protected:

	virtual const char *	Warhead(void) = 0;
};


/*JFW_Vechicle_Regen_2

  Repairs the vehicle a little at a time, for as long as it is damaged.  The
  repair is negative damage, so armour and skin types still decide how much
  of it lands.

  Parameters:

  Health			= How much to repair each time.
  Time				= Seconds between repairs.
*/

REGISTER_SCRIPT_TT_NAMED(JFW_Vehicle_Regen_2, "JFW_Vechicle_Regen_2", "Health:float,Time:float")
class JFW_Vehicle_Regen_2 : public JFW_Vehicle_Regen
{
	const char * Warhead(void) override		{ return "RegenHealth"; }
};


/*JFW_Vechicle_Regen_3

  As JFW_Vechicle_Regen_2, with the warhead named here rather than fixed.

  Parameters:

  Health			= How much to repair each time.
  Time				= Seconds between repairs.
  Warhead			= Warhead the repair is applied with.
*/

REGISTER_SCRIPT_TT_NAMED(JFW_Vehicle_Regen_3, "JFW_Vechicle_Regen_3", "Health:float,Time:float,Warhead:string")
class JFW_Vehicle_Regen_3 : public JFW_Vehicle_Regen
{
	const char * Warhead(void) override		{ return Get_Parameter("Warhead"); }
};


/*JFW_Aircraft_Fuel

  Blows the aircraft up a fixed time after somebody first climbs into it,
  unless it is refuelled -- which is a custom, sent by whatever counts as a
  fuel point on this map, and which starts the clock again.

  Parameters:

  Time				= Seconds of fuel.
  TimerNum			= Timer id to use.
  Explosion			= Explosion to make when it runs out.
  Refuel_Message	= Custom that refuels it.
*/

DECLARE_SCRIPT_TT(JFW_Aircraft_Fuel, "Time:float,TimerNum:int,Explosion:string,Refuel_Message:int")
{
	bool	Fuelled;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Fuelled, 1);
	}

	void Created(GameObject* /*obj*/) override
	{
		Fuelled = false;
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Refuel_Message")) {
			ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), Get_Int_Parameter("TimerNum"));
		}

		if (type == CUSTOM_EVENT_VEHICLE_ENTERED && !Fuelled) {
			ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), Get_Int_Parameter("TimerNum"));
			Fuelled = true;
		}
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number == Get_Int_Parameter("TimerNum")) {
			ScriptEngine::Create_Explosion(Get_Parameter("Explosion"), ScriptEngine::Get_Position(obj), nullptr);
		}
	}
};


/*
**	A helipad hands a flying vehicle one reload powerup every interval for as
**	long as it stays on the pad with a pilot in it.  Two scripts in the library
**	did this; the second also drove an animation on a nearby prop and refused
**	to start for an aircraft whose ammunition was already full.
*/

class JFW_Aircraft_Refill_Base : public ScriptImpClass
{
public:

	bool	Running;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE(Running, 1);
	}

	void Created(GameObject* obj) override
	{
		Running = false;
		On_Created(obj);
	}

	void Entered(GameObject* obj, GameObject* enterer) override
	{
		if (Running || ScriptEngine::Get_Player_Type(enterer) != Get_Int_Parameter("TeamSelection")) {
			return ;
		}

		GameObject* vehicle = As_Flying_Vehicle(enterer);
		if (vehicle == nullptr || !Wants_Refill(vehicle)) {
			return ;
		}

		On_Refill_Started(obj);
		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Reload_Interval"), ScriptEngine::Get_ID(vehicle));
		Running = true;
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		Running = false;

		GameObject* vehicle = ScriptEngine::Find_Object(number);
		if (vehicle == nullptr || !Still_On_Pad(obj, vehicle) || !Wants_Refill(vehicle)) {
			On_Refill_Stopped(obj);
			return ;
		}

		ScriptEngine::Give_PowerUp(vehicle, Get_Parameter("Reload_Powerup"), false);
		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Reload_Interval"), number);
		Running = true;
	}

protected:

	virtual void	On_Created(GameObject* /*obj*/)				{ }
	virtual void	On_Refill_Started(GameObject* /*obj*/)		{ }
	virtual void	On_Refill_Stopped(GameObject* /*obj*/)		{ }

	//
	//	Whether there is anything left to give this aircraft.  The plain script
	//	never asked, and handed out powerups to a full aircraft forever.
	//
	virtual bool	Wants_Refill(GameObject* /*vehicle*/)		{ return true; }

	//
	//	The zone is entered by the pilot as often as by the aircraft, so both
	//	are followed back to the vehicle itself.
	//
	static GameObject* As_Flying_Vehicle(GameObject* enterer)
	{
		GameObject* vehicle = nullptr;

		PhysicalGameObj* physical = enterer->As_PhysicalGameObj();
		if (physical != nullptr && physical->As_VehicleGameObj() != nullptr) {
			vehicle = enterer;
		}

		if (ScriptEngine::Get_Vehicle(enterer) != nullptr) {
			vehicle = ScriptEngine::Get_Vehicle(enterer);
		}

		if (vehicle == nullptr || ScriptEngine::Get_Vehicle_Mode(vehicle) != VEHICLE_TYPE_FLYING) {
			return nullptr;
		}

		return vehicle;
	}

	static bool Still_On_Pad(GameObject* obj, GameObject* vehicle)
	{
		PhysicalGameObj* physical = vehicle->As_PhysicalGameObj();
		if (physical == nullptr || physical->As_VehicleGameObj() == nullptr) {
			return false;
		}

		if (ScriptEngine::Get_Vehicle_Mode(vehicle) != VEHICLE_TYPE_FLYING
			|| ScriptEngine::Get_Vehicle_Driver(vehicle) == nullptr) {
			return false;
		}

		float distance = ScriptEngine::Get_Distance(ScriptEngine::Get_Position(obj),
			ScriptEngine::Get_Position(vehicle));
		return distance <= JFW_VEH_REFILL_RANGE;
	}

	static WeaponClass* Vehicle_Weapon(GameObject* vehicle)
	{
		PhysicalGameObj* physical = vehicle->As_PhysicalGameObj();
		VehicleGameObj* vehicle_obj = physical != nullptr ? physical->As_VehicleGameObj() : nullptr;
		return vehicle_obj != nullptr ? vehicle_obj->Get_Weapon() : nullptr;
	}
};


/*JFW_Aircraft_Refill

  A helipad.  Hands a flying vehicle a reload powerup every interval for as
  long as it sits on the pad with a pilot aboard.

  Parameters:

  TeamSelection		= Side the pad serves.
  Reload_Interval	= Seconds between reloads.
  Reload_Powerup		= Powerup to hand over.
*/

REGISTER_SCRIPT_TT(JFW_Aircraft_Refill, "TeamSelection:int,Reload_Interval:float,Reload_Powerup:string")
class JFW_Aircraft_Refill : public JFW_Aircraft_Refill_Base
{
};


/*JFW_Aircraft_Refill_Animation

  As JFW_Aircraft_Refill, and runs an animation on the nearest prop of a named
  preset while the reload is going on, so the pad visibly works.  Refuses to
  start for an aircraft whose ammunition is already full, or whose weapon
  carries no reserve at all.

  Parameters:

  TeamSelection		= Side the pad serves.
  Reload_Interval	= Seconds between reloads.
  Reload_Powerup		= Powerup to hand over.
  AnimPreset			= Preset of the prop to animate.
  Animation			= Animation to run on it.
*/

REGISTER_SCRIPT_TT(JFW_Aircraft_Refill_Animation,
	"TeamSelection:int,Reload_Interval:float,Reload_Powerup:string,AnimPreset:string,Animation:string")
class JFW_Aircraft_Refill_Animation : public JFW_Aircraft_Refill_Base
{
	void On_Created(GameObject* obj) override
	{
		GameObject* prop = Prop(obj);
		if (prop != nullptr) {
			ScriptEngine::Set_Animation_Frame(prop, Get_Parameter("Animation"), 0);
		}
	}

	void On_Refill_Started(GameObject* obj) override
	{
		GameObject* prop = Prop(obj);
		if (prop != nullptr) {
			ScriptEngine::Set_Animation(prop, Get_Parameter("Animation"), true, nullptr, 0.0f, -1.0f, false);
		}
	}

	void On_Refill_Stopped(GameObject* obj) override
	{
		GameObject* prop = Prop(obj);
		if (prop == nullptr) {
			return ;
		}

		PhysicalGameObj* physical = prop->As_PhysicalGameObj();
		if (physical != nullptr) {
			physical->Set_Animation("", false);
		}

		ScriptEngine::Set_Animation_Frame(prop, Get_Parameter("Animation"), 0);
	}

	bool Wants_Refill(GameObject* vehicle) override
	{
		if (ScriptEngine::Get_Vehicle_Driver(vehicle) == nullptr) {
			return false;
		}

		WeaponClass* weapon = Vehicle_Weapon(vehicle);
		if (weapon == nullptr || weapon->Is_Ammo_Maxed()) {
			return false;
		}

		const WeaponDefinitionClass* definition = weapon->Get_Definition();
		return definition != nullptr && (int)definition->MaxInventoryRounds != 0;
	}

	//
	//	The nearest prop of the named preset.  4.8.4 used the answer without
	//	checking it, and a map with the animation preset missing crashed on
	//	load.
	//
	GameObject* Prop(GameObject* obj)
	{
		GameObject* prop = ScriptEngine::Find_Nearest_Preset(ScriptEngine::Get_Position(obj),
			Get_Parameter("AnimPreset"));
		if (prop == nullptr || prop->As_PhysicalGameObj() == nullptr) {
			return nullptr;
		}

		return prop;
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Vehicles arriving and vehicles leaving
//
////////////////////////////////////////////////////////////////////////////

/*JFW_Drive_To_Player

  Sends this vehicle after the nearest player and then gets out of the way --
  the script destroys itself once the order is given, so whatever the vehicle
  does afterwards is somebody else's business.

  Parameters:

  Speed				= How fast to drive.
  Arrive_Distance	= How close to get.
*/

DECLARE_SCRIPT_TT(JFW_Drive_To_Player, "Speed:float,Arrive_Distance:float")
{
	void Created(GameObject* obj) override
	{
		ActionParamsStruct params;
		params.MoveArrivedDistance = 100;
		params.Set_Basic(this, 100, JFW_VEH_ACTION_DRIVE);
		params.Set_Movement(ScriptEngine::Get_A_Star(ScriptEngine::Get_Position(obj)),
			Get_Float_Parameter("Speed"), Get_Float_Parameter("Arrive_Distance"));
		ScriptEngine::Action_Goto(obj, params);

		Destroy_Script();
	}
};


/*JFW_Vehicle_Reinforcement

  Waits, then creates a vehicle where this object stands and sends it along a
  waypath.  If a factory id is given and that building is dead, nothing
  arrives.

  Parameters:

  Vehicle			= Preset to create.
  Time				= Seconds to wait first.
  FactoryID			= Building that has to still be standing, or 0 for none.
  Waypathid			= Waypath to send it along.
  Speed				= How fast to drive it.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Reinforcement,
	"Vehicle:string,Time:float,FactoryID:int,Waypathid:int,Speed:float")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), 1);
	}

	void Timer_Expired(GameObject* obj, int /*number*/) override
	{
		int factory_id = Get_Int_Parameter("FactoryID");
		if (factory_id != 0) {
			GameObject* factory = ScriptEngine::Find_Object(factory_id);
			if (factory != nullptr && ScriptEngine::Is_Building_Dead(factory)) {
				return ;
			}
		}

		GameObject* vehicle = ScriptEngine::Create_Object(Get_Parameter("Vehicle"), ScriptEngine::Get_Position(obj));
		if (vehicle == nullptr) {
			return ;
		}

		ActionParamsStruct params;
		params.MoveArrivedDistance	= 100;
		params.Set_Basic(this, 100, JFW_VEH_ACTION_DRIVE);
		params.Set_Movement((GameObject*)nullptr, Get_Float_Parameter("Speed"), 10.0f);
		params.WaypathID				= Get_Int_Parameter("Waypathid");
		params.WaypathSplined		= true;
		params.AttackActive			= false;
		params.AttackCheckBlocked	= false;
		ScriptEngine::Action_Goto(vehicle, params);
	}
};


/*JFW_Empty_Vehicle_Timer

  Deletes the vehicle a while after the last person walks away from it, so an
  abandoned vehicle does not sit on the map forever.  Anybody getting back in
  before the time is up saves it, because the check is made again when the
  timer goes off.

  Parameters:

  Time				= Seconds to wait once it is empty.
  TimerNum			= Timer id to use.
*/

DECLARE_SCRIPT_TT(JFW_Empty_Vehicle_Timer, "Time:float,TimerNum:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == CUSTOM_EVENT_VEHICLE_EXITED && ScriptEngine::Get_Vehicle_Occupant_Count(obj) == 0) {
			ScriptEngine::Start_Timer(obj, this, Get_Float_Parameter("Time"), Get_Int_Parameter("TimerNum"));
		}
	}

	void Timer_Expired(GameObject* obj, int /*number*/) override
	{
		if (ScriptEngine::Get_Vehicle_Occupant_Count(obj) == 0) {
			ScriptEngine::Destroy_Object(obj);
		}
	}
};


/*JFW_Vehicle_Visible_Weapon

  Holds an animation at the frame matching how many rounds the vehicle has
  left, so the model shows its own ammunition -- a rack of missiles that
  empties as they are fired.

  Parameters:

  Animation			= Animation whose frames are the round count.
*/

DECLARE_SCRIPT_TT(JFW_Vehicle_Visible_Weapon, "Animation:string")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Start_Timer(obj, this, 0.01f, 1);
	}

	void Timer_Expired(GameObject* obj, int number) override
	{
		if (number != 1) {
			return ;
		}

		PhysicalGameObj* physical = obj->As_PhysicalGameObj();
		VehicleGameObj* vehicle = physical != nullptr ? physical->As_VehicleGameObj() : nullptr;
		if (vehicle == nullptr) {
			return ;
		}

		WeaponClass* weapon = vehicle->Get_Weapon();
		if (weapon != nullptr) {
			int rounds = weapon->Get_Total_Rounds();
			if (rounds == -1) {
				rounds = weapon->Get_Clip_Rounds();
			}

			if (rounds >= 0) {
				ScriptEngine::Set_Animation_Frame(obj, Get_Parameter("Animation"), rounds);
			}
		}

		ScriptEngine::Start_Timer(obj, this, 0.01f, 1);
	}
};


////////////////////////////////////////////////////////////////////////////
//
//	Wreckage
//
//	From the 4.8.4 library's dan.cpp, whose other script -- the crate -- is
//	nothing to do with vehicles and lives in TT_Crates.cpp.
//
////////////////////////////////////////////////////////////////////////////

/*DAN_Drop_Wreckage_On_Death

  Leaves a burnt-out hulk where the vehicle died, facing the way it was
  facing, and tells the hulk what it used to be so somebody can weld it back
  together.

  Parameters:

  Wreckage_Preset	= Object to leave behind.
*/

DECLARE_SCRIPT_TT(DAN_Drop_Wreckage_On_Death, "Wreckage_Preset:string")
{
	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		GameObject* wreck = ScriptEngine::Create_Object(Get_Parameter("Wreckage_Preset"),
				ScriptEngine::Get_Position(obj));

		if (wreck == nullptr) {
			return ;
		}

		ScriptEngine::Set_Facing(wreck, ScriptEngine::Get_Facing(obj));
		ScriptEngine::Attach_Script(wreck, "DAN_Wreckage_Rebuildable",
				ScriptEngine::Get_Preset_Name(obj));
	}
};


/*DAN_Wreckage_Rebuildable

  A hulk anybody can repair back into a working vehicle.  It belongs to
  nobody while it lies there, so either side may work on it, and it goes to
  whoever finished the job -- with almost no health, so the first shot still
  takes it out.

  Parameters:

  Vehicle_Preset	= What it turns back into.
*/

DECLARE_SCRIPT_TT(DAN_Wreckage_Rebuildable, "Vehicle_Preset:string")
{
	void Created(GameObject* obj) override
	{
		ScriptEngine::Set_Player_Type(obj, PLAYERTYPE_NEUTRAL);
		ScriptEngine::Set_Health(obj, 50.0f);
		ScriptEngine::Set_Shield_Strength(obj, 0.0f);
	}

	void Damaged(GameObject* obj, GameObject* damager, float amount) override
	{
		//	Repair, not damage.
		if (amount >= 0.0f || damager == nullptr) {
			return ;
		}

		float health = ScriptEngine::Get_Health(obj) + ScriptEngine::Get_Shield_Strength(obj);
		float whole = ScriptEngine::Get_Max_Health(obj)
				+ ScriptEngine::Get_Max_Shield_Strength(obj);

		if (health < whole) {
			return ;
		}

		Vector3 position = ScriptEngine::Get_Position(obj);

		GameObject* vehicle = ScriptEngine::Create_Object(Get_Parameter("Vehicle_Preset"),
				position);

		if (vehicle == nullptr) {
			return ;
		}

		ScriptEngine::Set_Facing(vehicle, ScriptEngine::Get_Facing(obj));
		ScriptEngine::Set_Health(vehicle, 5.0f);
		ScriptEngine::Set_Shield_Strength(vehicle, 0.0f);
		ScriptEngine::Set_Player_Type(vehicle, ScriptEngine::Get_Player_Type(damager));

		//	Clear of the wreck's own collision before it is taken away.
		position.Z += 1.0f;
		ScriptEngine::Set_Position(vehicle, position);

		ScriptEngine::Destroy_Object(obj);
	}
};
