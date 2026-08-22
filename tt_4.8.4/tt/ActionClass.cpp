#include "general.h"

#include "ActionClass.h"
#include "RenderObjClass.h"
#include "SoldierGameObj.h"
#include "engine_io.h"
bool ActionClass::_acting = false;

void Delete_Action_Code()
{
}

void ActionClass::Act()
{
	if (!this->isPaused)
	{
		this->actCount++;
		owner->Clear_Control();
		if (this->actionCode)
		{
			this->actionCode->Act();
		}
		Delete_Action_Code();
	}
}

RENEGADE_FUNCTION
void ActionClass::Attack
   (const ActionParamsStruct& parameters)
   AT2(0x006BC9C0,0x006BC260);

void ActionClass::Begin_Hibernation()
{
	if (actionCode)
	{
		actionCode->Begin_Hibernation();
	}
}

RENEGADE_FUNCTION
void ActionClass::Dock_Vehicle
   (const ActionParamsStruct& parameters)
   AT2(0x006BF420,0x006BECC0);

void ActionClass::Done
   (sint32 reason)
{
	params.Priority = 0;
	if (actionCode)
	{
		Set_Action_Code(0);
		Notify_Completed(params.ObserverID,params.ActionID,reason);
	}
}

RENEGADE_FUNCTION
void ActionClass::Follow_Input
   (const ActionParamsStruct& oParameters)
   AT2(0x006B99B0,0x006B9250);

RENEGADE_FUNCTION
void ActionClass::Goto
   (const ActionParamsStruct& parameters)
   AT2(0x006BA7F0,0x006BA090);

bool ActionClass::Is_Acting() const
{
   return (this->actionCode != 0);
}

bool ActionClass::Reset
   (float priority)
{
   if (priority < (float)this->params.Priority)
      return false;

   this->Done (1);
   return true;
}

SafeActionParamsStruct &SafeActionParamsStruct::operator=(ActionParamsStruct *params)
{
	Priority = params->Priority;
	ActionID = params->ActionID;
	ObserverID = params->ObserverID;
	LookLocation = params->LookLocation;
	LookObjectReference = params->LookObject;
	LookDuration = params->LookDuration;
	MoveLocation = params->MoveLocation;
	MoveObjectReference = params->MoveObject;
	MoveObjectOffset = params->MoveObjectOffset;
	MoveSpeed = params->MoveSpeed;
	MoveArrivedDistance = params->MoveArrivedDistance;
	MoveBackup = params->MoveBackup;
	MoveFollow = params->MoveFollow;
	MoveCrouched = params->MoveCrouched;
	MovePathfind = params->MovePathfind;
	ShutdownEngineOnArrival = params->ShutdownEngineOnArrival;
	AttackRange = params->AttackRange;
	AttackError = params->AttackError;
	AttackErrorOverride = params->AttackErrorOverride;
	AttackPrimaryFire = params->AttackPrimaryFire;
	AttackCrouched = params->AttackCrouched;
	AttackObjectReference = params->AttackObject;
	AttackLocation = params->AttackLocation;
	AttackCheckBlocked = params->AttackCheckBlocked;
	AttackActive = params->AttackActive;
	AttackWanderAllowed = params->AttackWanderAllowed;
	AttackFaceTarget = params->AttackFaceTarget;
	AttackForceFire = params->AttackForceFire;
	ForceFacing = params->ForceFacing;
	FaceLocation = params->FaceLocation;
	FaceDuration = params->FaceDuration;
	IgnoreFacing = params->IgnoreFacing;
	WaypathID = params->WaypathID;
	WaypointStartID = params->WaypointStartID;
	WaypointEndID = params->WaypointEndID;
	WaypathSplined = params->WaypathSplined;
	AnimNameString = params->AnimationName;
	AnimationLooping = params->AnimationLooping;
	ConversationNameString = params->ConversationName;
	ActiveConversationID = params->ActiveConversationID;
	AnimNameString = params->AnimationName;
	AIState = params->AIState;
	DockLocation = params->DockLocation;
	DockEntrance = params->DockEntrance;
	return *this;
}

ActionClass::ActionClass (SmartGameObj* owner) : params()
{
	this->owner = owner;
}

ActionClass::~ActionClass ()
{
	Set_Action_Code(NULL);
	Delete_Action_Code();
}

void ActionClass::Dive (const ActionParamsStruct& parameters)
{
}

void ActionClass::End_Hibernation ()
{
	if (actionCode)
	{
		actionCode->End_Hibernation();
	}
}

void ActionClass::Enter_Exit (const ActionParamsStruct& parameters)
{
}

void ActionClass::Face_Location (const ActionParamsStruct& parameters)
{
}

sint32 ActionClass::Get_Act_Count ()
{
	return actCount;
}

SmartGameObj *ActionClass::Get_Action_Obj ()
{
	return owner;
}

SafeActionParamsStruct *ActionClass::Get_Parameters ()
{
	return &params;
}

void ActionClass::Have_Conversation (const ActionParamsStruct& parameters)
{
}

bool ActionClass::Is_Active ()
{
   return (this->actionCode != 0);
}

bool ActionClass::Is_Animating ()
{
	return false;
}

bool ActionClass::Is_Busy ()
{
	return false;
}

bool ActionClass::Is_Paused ()
{
	return isPaused;
}

void ActionClass::Modify (const ActionParamsStruct& parameters, bool b1, bool b2)
{
}

void ActionClass::Notify_Completed (sint32 ObserverID, sint32 ActionID, sint32 reason)
{
}

void ActionClass::Pause (bool pause)
{
	isPaused = pause;
}

void ActionClass::Play_Animation (const ActionParamsStruct& parameters)
{
}

bool ActionClass::Request_Action (ActionCodeClass* code, const ActionParamsStruct& parameters)
{
	if (parameters.Priority < this->params.Priority)
	{
		Notify_Completed(parameters.ObserverID,parameters.ActionID,1);
		delete code;
		return false;
	}
	else
	{
		bool notify = false;
		int ObserverID = 0;
		int ActionID = 0;
		if (this->actionCode)
		{
			notify = true;
			ObserverID = this->params.ObserverID;
			ActionID = this->params.ActionID;
		}
		this->params = params;
		Set_Action_Code(code);
		if (notify)
		{
			Notify_Completed(ObserverID,ActionID,1);
		}
		return true;
	}
}

bool ActionClass::Save (ChunkSaveClass& saver)
{
	return false;
}

void ActionClass::Set_Action_Code (ActionCodeClass* code)
{
	actionCode = code;
}

void ActionClass::Stand (const ActionParamsStruct& parameters)
{
}

ActionCodeClass::~ActionCodeClass()
{
}

bool ActionCodeClass::Save (ChunkSaveClass& saver)
{
	saver.Begin_Chunk(0x3D73D506);
	saver.End_Chunk();
	return true;
}

bool ActionCodeClass::Load (ChunkLoadClass& loader)
{
	while (loader.Open_Chunk())
	{
		loader.Close_Chunk();
	}
	return true;
}

void ActionCodeClass::Init (ActionClass *action)
{
	this->action = action;
	if (action->params.AIState)
	{
		if (action->owner)
		{
			SoldierGameObj *obj = action->owner->As_SoldierGameObj();
			obj->Set_AI_State(action->params.AIState);
		}
	}
}

void ActionCodeClass::Modify_Parameters (SafeActionParamsStruct const& params,bool b1,bool b2)
{
}

void ActionCodeClass::Set_Action (ActionClass *action)
{
	this->action = action;
}

void ActionCodeClass::Shutdown ()
{
	this->action = 0;
}

bool ActionCodeClass::Act ()
{
	return true;
}

bool ActionCodeClass::Is_Animating ()
{
	return false;
}

bool ActionCodeClass::Is_Busy ()
{
	return false;
}

void ActionCodeClass::Begin_Hibernation ()
{
}

void ActionCodeClass::End_Hibernation ()
{
}

RENEGADE_FUNCTION
bool ActionClass::Load(ChunkLoadClass &loader)
AT2(0x006C0DE0,0x006C0680);
