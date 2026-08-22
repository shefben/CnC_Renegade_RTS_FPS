/*	Renegade tt.dll
	Hook related code
	Copyright 2009 Jonathan Wilson

	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"

#include "tt.h"
#include "engine_3dre.h"
#include "shaders.h"
#include "chatdlg.h"
#include "vehicledlg.h"
#include "VehicleGameObj.h"
#include "VehicleGameObjDef.h"
#include "GameObjObserverManager.h"
#include "CombatManager.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "input.h"
#include "cGameType.h"
struct Key {
	char *LogicalKeyName;
	int KeyNumber;
	float KeyValue;
};

bool ScopeEnabled;
bool LastZoom;
bool FirstUpdate;
int ScopeNumber = -1;
bool hookon = false;
SimpleDynVecClass<ObjectCreateHookStruct *> ObjectCreateHooks;
SimpleDynVecClass<KeyHookStruct *> KeyHooks;
SimpleDynVecClass<ShaderNotifyStruct *> ShaderNotifies;
SimpleDynVecClass<Key *> KeyList;
extern int ScriptsLastTeamTime;

RENEGADE_FUNCTION
void ScriptableGameObj::Add_Observer(GameObjObserverClass*)
   AT2(0x006B6C70,0x006B6510);

RENEGADE_FUNCTION
void ScriptableGameObj::Start_Observer_Timer(int ObserverID,float time,int number)
   AT2(0x006B6EB0,0x006B6750);

class VehicleObserver : public GameObjObserverClass
{
	virtual const char *Get_Name()
	{
		return "VehicleObserver";
	}
	virtual void Attach(GameObject *newobj)
	{
	}
	virtual void Detach(GameObject *obj)
	{
		delete this;
	}
	virtual void Created(GameObject *obj)
	{
	}
	virtual void Destroyed(GameObject *obj)
	{
	}
	virtual void Killed(GameObject *obj,GameObject *killer)
	{
	}
	virtual void Damaged(GameObject *obj,GameObject *damager,float amount)
	{
	}
	virtual void Custom(GameObject *obj,int type,int param,GameObject *sender)
	{
		if (type == CUSTOM_EVENT_VEHICLE_EXITED)
		{
			VehicleGameObj* vehicleObj = ((ScriptableGameObj*)obj)->As_VehicleGameObj();
			vehicleObj->Set_Scripts_Last_Team((char)Commands->Get_Player_Type(sender));
			if (ScriptsLastTeamTime != -1)
			{
				((ScriptableGameObj *)obj)->Start_Observer_Timer(Get_ID(),(float)ScriptsLastTeamTime,1);
			}
		}
	}
	virtual void Sound_Heard(GameObject *obj,const CombatSound & sound)
	{
	}
	virtual void Enemy_Seen(GameObject *obj,GameObject *enemy)
	{
	}
	virtual void Action_Complete(GameObject *obj,int action_id,ActionCompleteReason complete_reason)
	{
	}
	virtual void Timer_Expired(GameObject *obj,int number)
	{
		if (number == 1)
		{
			VehicleGameObj* vehicleObj = ((ScriptableGameObj*)obj)->As_VehicleGameObj();
			vehicleObj->Set_Scripts_Last_Team(-2);
		}
	}
	virtual void Animation_Complete(GameObject *obj,const char *animation_name)
	{
		VehicleGameObj *o = ((ScriptableGameObj *)obj)->As_VehicleGameObj();
		if (!o)
		{
			return;
		}
		if (!o->Get_Definition().Get_Fire0_Anim().Is_Empty() && !_stricmp(o->Get_Definition().Get_Fire0_Anim(),animation_name))
		{
			obj->As_PhysicalGameObj()->Set_Animation(NULL, true, 0.f);
		}
		if (!o->Get_Definition().Get_Fire1_Anim().Is_Empty() && !_stricmp(o->Get_Definition().Get_Fire1_Anim(),animation_name))
		{
			obj->As_PhysicalGameObj()->Set_Animation(NULL, true, 0.f);
		}
	}
	virtual void Poked(GameObject *obj,GameObject *poker)
	{
	}
	virtual void Entered(GameObject *obj,GameObject *enterer)
	{
	}
	virtual void Exited(GameObject *obj,GameObject *exiter)
	{
	}
};
extern bool LevelRunning;
void Do_Vehicle_Observer(VehicleGameObj *obj)
{
	if (LevelRunning)
	{
		VehicleObserver *o = new VehicleObserver();
		o->Set_ID(GameObjObserverManager::NextID);
		GameObjObserverManager::NextID++;
		obj->Add_Observer(o);
	}
}

void Do_Object_Create(GameObject *obj)
{
	if (Commands->Is_A_Star(obj))
	{
		char buf[512];
		sprintf(buf,"j\n%d\n%d\n",SS_CURRENTLYBUILDING,CurrentlyBuildingTeam[Get_Object_Type(obj)]);
		SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
		if (cNetwork::I_Am_Client() && cNetwork::Get_My_Id() == Get_Player_ID(obj))
		{
			CurrentlyBuilding = CurrentlyBuildingTeam[Get_Object_Type(obj)];
		}
	}
	if (hookon == true)
	{
		int x = ObjectCreateHooks.Count();
		for (int i = 0;i < x;i++)
		{
			if (ObjectCreateHooks[i])
			{
				ObjectCreateHooks[i]->hook(ObjectCreateHooks[i]->data,obj);
			}
		}
	}
}

void Key_Is_Pressed(char *LogicalKey,int Player_ID)
{
	if ((!KeyHooks.Count()) || (Player_ID == -1))
	{
		return;
	}
	int x = KeyHooks.Count();
	for (int i = 0;i < x;i++)
	{
		if (KeyHooks[i])
		{
			if ((!_stricmp(KeyHooks[i]->key,LogicalKey)) && (Player_ID == KeyHooks[i]->PlayerID))
			{
				KeyHooks[i]->hook(KeyHooks[i]->data);
			}
		}
	}
}

void ShaderNotifyDo(int ID,int notify,int Player_ID)
{
	if ((!ShaderNotifies.Count()) || (Player_ID == -1))
	{
		return;
	}
	int x = ShaderNotifies.Count();
	for (int i = 0;i < x;i++)
	{
		if (ShaderNotifies[i])
		{
			if ((ShaderNotifies[i]->ID == ID) && ((Player_ID == ShaderNotifies[i]->PlayerID) || (ShaderNotifies[i]->PlayerID == -1)))
			{
				ShaderNotifies[i]->hook(ShaderNotifies[i]->data,notify);
			}
		}
	}
}

void ReadKeyboardConfig()
{
	int handle = Commands->Text_File_Open("keys.cfg");
	if ((!handle) || (Exe))
	{
		return;
	}
	char data[100];
	while (Commands->Text_File_Get_String(handle,data,100))
	{
		Key *k = new Key;
		char *a = strtok(data,"=");
		char *b = strtok(NULL,"=");
		k->LogicalKeyName = newstr(a);
		if (_stricmp(b,"None_Key"))
		{
			b[strlen(b)-2] = 0;
			k->KeyNumber = Input::Get_Key(b);
		}
		else
		{
			k->KeyNumber = 0;
		}
		k->KeyValue = 0;
		KeyList.Add(k);
	}
	Commands->Text_File_Close(handle);
}

void FreeKeyboardConfig()
{
	int x = KeyList.Count();
	for (int i = 0;i < x;i++)
	{
		SAFE_DELETE_ARRAY(KeyList[i]->LogicalKeyName);
		SAFE_DELETE(KeyList[i]);
	}
	x = ObjectCreateHooks.Count();
	for (int i = 0;i < x;i++)
	{
		SAFE_DELETE(ObjectCreateHooks[i]);
	}
	x = KeyHooks.Count();
	for (int i = 0;i < x;i++)
	{
		if (KeyHooks[i])
		{
			SAFE_DELETE_ARRAY(KeyHooks[i]->key);
			SAFE_DELETE(KeyHooks[i]);
		}
	}
	x = ShaderNotifies.Count();
	for (int i = 0;i < x;i++)
	{
		if (ShaderNotifies[i])
		{
			SAFE_DELETE(ShaderNotifies[i]);
		}
	}
}

void KeyboardUpdate()
{
	int x = KeyList.Count();
	for (int i = 0;i < x;i++)
	{
		if (KeyList[i]->KeyNumber)
		{
			float keyvalue = Input::Get_Value(0,KeyList[i]->KeyNumber,1.0);
			if (KeyList[i]->KeyValue != -1.0)
			{
				if (keyvalue > 0.0)
				{
					KeyList[i]->KeyValue = keyvalue;
				}
				else
				{
					KeyList[i]->KeyValue = 0.0;
				}
			}
			else if ((KeyList[i]->KeyValue == -1.0) && (!keyvalue))
			{
				KeyList[i]->KeyValue = -2.0;
			}
			if (ScopeEnabled)
			{
				Input::FunctionValue[INPUT_FUNCTION_NEXT_WEAPON] = 0.0;
				Input::FunctionValue[INPUT_FUNCTION_PREV_WEAPON] = 0.0;
				Input::FunctionValue[INPUT_FUNCTION_FIRST_PERSON_TOGGLE] = 0.0;
				Input::FunctionValue[INPUT_FUNCTION_ACTION] = 0.0;
			}
		}
	}
}

void DoCombatKeyboard()
{
	int x = KeyList.Count();
	for (int i = 0;i < x;i++)
	{
		if (KeyList[i]->KeyValue > 0.0)
		{
			KeyList[i]->KeyValue = -1.0;
			if ((!(Input::ConsoleMode)) && (!(Input::MenuMode)))
			{
				if (!_stricmp(KeyList[i]->LogicalKeyName,"Scope"))
				{
					if (CombatManager::Get_The_Star())
					{
						if ((ScopeNumber != -1) && (!CombatManager::Get_The_Star()->Get_Vehicle()))
						{
							LastZoom = ScopeEnabled;
							ScopeEnabled = !ScopeEnabled;
							ScopeTrigger(ScopeEnabled);
							FirstUpdate = ScopeEnabled;
						}
					}
				}
				else if (!_stricmp(KeyList[i]->LogicalKeyName,"ChatHistory"))
				{
					ChatDlg::Do_Dialog();
				}
				else if (!_stricmp(KeyList[i]->LogicalKeyName,"Vehicle"))
				{
					VehicleDlg::Do_Dialog();
				}
				else
				{
					int id = -1;
					if (!Exe)
					{
						id = cNetwork::PClientConnection->Get_Local_Id();
					}
					if ((!CombatManager::I_Am_Server()) && (cGameType::GameType != 1))
					{
						char buf[512];
						sprintf(buf,"k\n%s\n",KeyList[i]->LogicalKeyName);
						SendTextCs(buf,PrivateMessage,id,-3);
					}
					else
					{
						Key_Is_Pressed(KeyList[i]->LogicalKeyName,id);
					}
				}
			}
		}
	}
}

