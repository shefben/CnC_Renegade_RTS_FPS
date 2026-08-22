#include "general.h"

#include "HAnimClass.h"
#include "WW3DAssetManager.h"
#include "DynamicAnimPhysClass.h"
#include "AnimControlClass.h"
#include "ScriptCommands.h"
#include "PhysicalGameObj.h"
#include "CinematicGameObj.h"
#include "SoldierGameObj.h"
#include "HUDClass.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "RadarManager.h"
#include "BuildingGameObj.h"
#include "PhysicalGameObj.h"
#include "CombatManager.h"
#include "CCameraClass.h"
#include "VehicleGameObj.h"
#include "ArmorWarheadManager.h"
#include "OffenseObjectClass.h"
#include "SmartGameObj.h"
void Set_Animation ( GameObject * obj, const char * anim_name, bool looping, const char * sub_obj_name, float start_frame, float end_frame, bool is_blended)
{
	if (!obj)
		return;
	
	PhysicalGameObj* obj1 = ((BaseGameObj*)obj)->As_PhysicalGameObj();
	if (!obj1)
		return;
	
	if (sub_obj_name && sub_obj_name[0])
	{
		RenderObjClass* model = obj1->Peek_Physical_Object()->Peek_Model();
		RenderObjClass* subModel = model->Get_Sub_Object_By_Name(sub_obj_name);
		if (!subModel)
			return;
			
		HAnimClass* hAnim = WW3DAssetManager::TheInstance->Get_HAnim(anim_name);
		
		if (hAnim)
		{

			subModel->Set_Animation(hAnim, start_frame, (looping ? 1 : 2));
			hAnim->Release_Ref();
		}
		subModel->Release_Ref();
	}
	else
	{
		CinematicGameObj* obj3 = obj1->As_CinematicGameObj();
		if (obj3)
		{
			DynamicAnimPhysClass* animClass = obj3->Peek_Physical_Object()->As_DynamicAnimPhysClass();
			if (!animClass)
				return;

			if (anim_name)
			{
				animClass->animMgr.Set_Animation(anim_name);
			}

			animClass->animMgr.Set_Current_Frame(start_frame);

			if (looping)
			{
				animClass->animMgr.Set_Animation_Mode((AnimCollisionManagerClass::AnimModeType)0);
			}
			else
			{
				animClass->animMgr.Set_Animation_Mode((AnimCollisionManagerClass::AnimModeType)1);
				animClass->animMgr.Set_Target_Frame_End();
			}

			if (end_frame >= 0)
			{
				animClass->animMgr.Set_Target_Frame(end_frame);
			}
			obj3->Set_Object_Dirty_Bit(DB_RARE, true);
		}
		else
		{
			SoldierGameObj* obj4 = obj1->As_SoldierGameObj();

			if (obj4 && (obj4->Get_State() == HumanStateClass::DEATH || obj4->Get_State() == HumanStateClass::DESTROY))
				return;

			if (obj4 && is_blended)
			{
				obj4->Set_Blended_Animation(anim_name, looping, start_frame, false);
			}
			else
			{
				obj1->Set_Animation(anim_name, looping, start_frame);
			}

			AnimControlClass* animControl = obj1->Get_Anim_Control();
			
			if (animControl)
			{
				animControl->Set_Mode((looping ? ANIM_MODE_LOOP : ANIM_MODE_ONCE), start_frame);
				if (end_frame >= 0)
				{
					animControl->Set_Mode(ANIM_MODE_TARGET, start_frame);
					animControl->Set_Target_Frame(end_frame);
				}
			}
		}
	}
	return;
}

void Control_Enable(GameObject *obj,bool Enable)
{
	if (obj)
	{
		SoldierGameObj *o = obj->As_SoldierGameObj();
		if (o)
		{
			o->Control_Enable(Enable);
		}
	}
}

void Display_Health_Bar(GameObject *Obj,bool Display)
{
	if (Obj)
	{
		if (Obj->As_DamageableGameObj())
		{
			Obj->As_DamageableGameObj()->Set_Is_Health_Bar_Displayed(Display);
		}
	}
}

void Set_Obj_Radar_Blip_Shape(GameObject *Obj,int Shape)
{
	if (Obj)
	{
		PhysicalGameObj *phys = Obj->As_PhysicalGameObj();
		if (phys)
		{
			if (Shape >= 0)
			{
				phys->Set_Radar_Blip_Shape_Type(Shape);
			}
			else
			{
				phys->Reset_Radar_Blip_Shape_Type();
			}
		}
	}
}

void Set_Player_Type(GameObject *Obj,int PlayerType)
{
	if (Obj)
	{
		if (Obj->As_PhysicalGameObj())
		{
			Obj->As_PhysicalGameObj()->Set_Player_Type(PlayerType);
		}
	}
}

void Enable_HUD(bool Enable)
{
	HUDClass::Enable(Enable);
}

void Select_Weapon(GameObject *Obj,const char *Weapon)
{
	if (Obj)
	{
		PhysicalGameObj *pobj = Obj->As_PhysicalGameObj();
		if (pobj)
		{
			if (pobj->As_ArmedGameObj())
			{
				pobj->As_ArmedGameObj()->Get_Weapon_Bag()->Select_Weapon_Name(Weapon);
			}
		}
	}
}

void Enable_Radar(bool Enable)
{
	RadarManager::Set_Hidden(Enable);
}

void Clear_Weapons(GameObject *Obj)
{
	if (Obj)
	{
		if (Obj->As_SmartGameObj())
		{
			Obj->As_SmartGameObj()->Get_Weapon_Bag()->Clear_Weapons();
		}
	}
}

void Play_Building_Announcement(GameObject *Obj,int AnnouncementID)
{
	if (Obj)
	{
		if (Obj->As_BuildingGameObj())
		{
			Obj->As_BuildingGameObj()->Play_Announcement(AnnouncementID,true);
		}
	}
}

void Set_Fog_Enable(bool Enable)
{
	WeatherMgrClass::Set_Fog_Enable(Enable);
}

void Set_Fog_Range(float StartDistance,float EndDistance,float Transition)
{
	WeatherMgrClass::Set_Fog_Range(StartDistance,EndDistance,Transition);
}

void Set_War_Blitz(float Intensity,float StartDistance,float EndDistance,float Heading,float Distribution,float Transition)
{
	BackgroundMgrClass::Set_War_Blitz(Intensity,StartDistance,EndDistance,Heading,Distribution,Transition);
}

void Enable_Stealth(GameObject *Obj,bool Stealth)
{
	if (Obj)
	{
		SmartGameObj *sgo = Obj->As_SmartGameObj();
		if (sgo)
		{
			sgo->Enable_Stealth(Stealth);
		}
	}
}

void Set_Obj_Radar_Blip_Color(GameObject *Obj,int Color)
{
	if (Obj)
	{
		PhysicalGameObj *pobj = Obj->As_PhysicalGameObj();
		if (pobj)
		{
			if (Color >= 0)
			{
				pobj->Set_Radar_Blip_Color_Type(Color);
			}
			else
			{
				pobj->Reset_Radar_Blip_Color_Type();
			}
		}
	}
}

void Set_Model(GameObject *Obj,const char *Model)
{
	if (Obj)
	{
		PhysicalGameObj *pobj = Obj->As_PhysicalGameObj();
		if (pobj)
		{
			if (pobj->As_SoldierGameObj())
			{
				pobj->As_SoldierGameObj()->Set_Model(Model);
			}
			else
			{
				pobj->Peek_Physical_Object()->Set_Model_By_Name(Model);
				if (pobj->Get_Anim_Control())
				{
					pobj->Get_Anim_Control()->Set_Model(pobj->Peek_Physical_Object()->Peek_Model());
				}
			}
			pobj->Set_Object_Dirty_Bit(DB_RARE,true);
			pobj->Hide_Muzzle_Flashes(true);
		}
	}
}

void Force_Camera_Look(const Vector3 &Look)
{
	if (COMBAT_CAMERA)
	{
		COMBAT_CAMERA->Force_Look(Look);
	}
}

void Enable_Collisions(GameObject *Obj)
{
	if (Obj)
	{
		PhysicalGameObj *pobj = Obj->As_PhysicalGameObj();
		if (pobj)
		{
			int group;
			if (pobj->As_SoldierGameObj())
			{
				group = 6;
			}
			else
			{
				group = 0;
			}
			pobj->Set_Collision_Group(group);
		}
	}
}

void Disable_Physical_Collisions(GameObject *Obj)
{
	if (Obj)
	{
		if (Obj->As_PhysicalGameObj())
		{
			Obj->As_PhysicalGameObj()->Set_Collision_Group(5);
		}
	}
}

void Disable_All_Collisions(GameObject *Obj)
{
	if (Obj)
	{
		if (Obj->As_PhysicalGameObj())
		{
			Obj->As_PhysicalGameObj()->Set_Collision_Group(1);
		}
	}
}

void Enable_Vehicle_Transitions(GameObject *Obj,bool Enable)
{
	if (Obj)
	{
		VehicleGameObj *vobj = Obj->As_VehicleGameObj();
		if (vobj)
		{
			vobj->Script_Enable_Transitions(Enable);
		}
	}
}

void Apply_Damage(GameObject * object, float amount, const char * warhead_name, GameObject * damager)
{
	if (object)
	{
		DamageableGameObj *dobj = object->As_DamageableGameObj();
		if (dobj)
		{
			int warhead = ArmorWarheadManager::Get_Warhead_Type(warhead_name);
			SmartGameObj *sobj = 0;
			if (damager)
			{
				sobj = damager->As_SmartGameObj();
			}
			OffenseObjectClass offense(amount,warhead,sobj);
			SmartGameObj *smobj = dobj->As_SmartGameObj();
			if (smobj)
			{
				smobj->Apply_Damage_Extended(offense,1.0f,Vector3( 0,0,0 ),NULL);
				VehicleGameObj *vobj = smobj->As_VehicleGameObj();
				if (vobj) //Fix for damage meshes when damage is done server side
				{
					vobj->Damage_Meshes_Update();
				}
			}
			else
			{
				dobj->Apply_Damage(offense,1,-1);
			}
		}
	}
}
