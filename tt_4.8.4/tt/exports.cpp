/*	Renegade tt.dll
	tt.dll exported functions
	Copyright 2009 Jonathan Wilson

	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"
#include "tt.h"
#include "shaders.h"
#include "sidebar.h"
#include "objdlg.h"
#include "VehicleGameObj.h"
#include "SmartGameObj.h"
#include "ExplosionManager.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "GameHintEvent.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "PopupDialog.h"
#include "engine_obj.h"
#include "engine_def.h"
#include "engine_player.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "engine_game.h"
void Display_Dialog(int dlg_res_id)
{
	PopupDialogClass *dialog = new PopupDialogClass (dlg_res_id);
	dialog->Start_Dialog ();
	REF_PTR_RELEASE (dialog);
}
extern REF_DECL2(VehicleCount,int);
extern int MineLimit;
extern "C" {
#define DLLEXPORT __declspec(dllexport)
char *CurrentSong = 0;

void DLLEXPORT New_Enable_Stealth(GameObject *obj,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	SmartGameObj* SmartObj = (SmartGameObj*)As_SmartGameObj(obj);
	if(SmartObj->Is_Stealth_Enabled() != enable)
	{
		char buf[512];
		sprintf(buf,"j\n%d\n%d\n%d\n",SS_STEALTH,Commands->Get_ID(obj),enable);
		SendTextSc(buf,PublicMessage,0,-2,-1);
		Enable_Stealth(obj,enable);
	}
}

void DLLEXPORT New_Create_Explosion(const char *explosion,const Vector3 & pos,GameObject *damager)
{
	DefinitionClass *def = Find_Named_Definition(explosion);
	if (def->Get_Class_ID() != 0xB003)
	{
		return;
	}
	unsigned int DefinitionID = def->Get_ID();
	unsigned int DamagerID = 0;
	GameObject *o = 0;
	if (damager)
	{
		o = As_SmartGameObj(damager);
	}
	if (o)
	{
		DamagerID = Commands->Get_ID(o);
	}
	ExplosionManager::Server_Explode(DefinitionID,pos,DamagerID,0);
}

void DLLEXPORT New_Create_Explosion_At_Bone(const char *explosion,GameObject *obj,const char *bone,GameObject *damager)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	Vector3 pos = Commands->Get_Bone_Position(obj,bone);
	New_Create_Explosion(explosion,pos,damager);
}

void DLLEXPORT New_Set_Fog_Enable(bool enable)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_FOGENABLE,enable);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Fog_Enable(enable);
}

void DLLEXPORT New_Set_Fog_Range(float Start_Distance,float End_Distance,float transition)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n",SS_FOGRANGE,Start_Distance,End_Distance,transition);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Fog_Range(Start_Distance,End_Distance,transition);
}

void DLLEXPORT New_Set_Fog_Enable_Player(GameObject *obj,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_FOGENABLE,enable);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Fog_Enable(enable);
	}
}

void DLLEXPORT Get_Fog_Range(float &Start_Distance,float &End_Distance)
{
	WeatherMgrClass::Get_Fog_Range(Start_Distance,End_Distance);
}

void DLLEXPORT New_Set_Fog_Range_Player(GameObject *obj,float Start_Distance,float End_Distance,float transition)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n",SS_FOGRANGE,Start_Distance,End_Distance,transition);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Fog_Range(Start_Distance,End_Distance,transition);
	}
}

void DLLEXPORT New_Set_War_Blitz(float intensity,float start_distance,float end_distance,float heading,float distribution,float transition)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n%f\n%f\n%f\n",SS_WARBLITZ,intensity,start_distance,end_distance,heading,distribution,transition);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_War_Blitz(intensity,start_distance,end_distance,heading,distribution,transition);
}

void DLLEXPORT New_Fade_Background_Music(const char *music,int unk1,int unk2)
{
	if (CurrentSong)
	{
		delete[] CurrentSong;
	}
	CurrentSong = newstr(music);
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%d\n",SS_FADEBGM,music,unk1,unk2);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Fade_Background_Music(music,unk1,unk2);
}

void DLLEXPORT New_Set_Background_Music(const char *music)
{
	if (CurrentSong)
	{
		delete[] CurrentSong;
	}
	CurrentSong = newstr(music);
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_SETBGM,music);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Background_Music(music);
}

void DLLEXPORT New_Stop_Background_Music()
{
	CurrentSong = 0;
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_STOPBGM,0);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Stop_Background_Music();
}

int DLLEXPORT New_Create_Sound(const char *soundname,const Vector3 & position,GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return 0;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%f\n%f\n%f\n%d\n",SS_CREATESOUND,soundname,position.X,position.Y,position.Z,Commands->Get_ID(obj));
	SendTextSc(buf,PublicMessage,0,-2,-1);
	return Create_Sound(soundname,position,obj);
}

int DLLEXPORT New_Create_2D_Sound(const char *soundname)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DSOUND,soundname);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	return Create_2D_Sound(soundname);
}

int DLLEXPORT New_Create_2D_WAV_Sound(const char *soundname)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DWAVSOUND,soundname);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	return Create_2D_WAV_Sound(soundname);
}

int DLLEXPORT New_Create_3D_WAV_Sound_At_Bone(const char *soundname,GameObject *obj,const char *bonename)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return 0;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DWAVSOUND,soundname,Commands->Get_ID(obj),bonename);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	return Create_3D_WAV_Sound_At_Bone(soundname,obj,bonename);
}

int DLLEXPORT New_Create_3D_Sound_At_Bone(const char *soundname,GameObject *obj,const char *bonename)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return 0;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DSOUND,soundname,Commands->Get_ID(obj),bonename);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	return Create_3D_Sound_At_Bone(soundname,obj,bonename);
}

void DLLEXPORT New_Play_Building_Announcement(GameObject *obj,int announcement)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_PLAYANNOUNCEMENT,Commands->Get_ID(obj),announcement);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Play_Building_Announcement(obj,announcement);
}

void DLLEXPORT New_Fade_Background_Music_Player(GameObject *obj,const char *music,int unk1,int unk2)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%d\n",SS_FADEBGM,music,unk1,unk2);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Fade_Background_Music(music,unk1,unk2);
	}
}

void DLLEXPORT New_Set_Background_Music_Player(GameObject *obj,const char *music)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_SETBGM,music);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Background_Music(music);
	}
}

void DLLEXPORT New_Stop_Background_Music_Player(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_STOPBGM,0);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Stop_Background_Music();
	}
}

void DLLEXPORT New_Enable_Radar_Player(GameObject *obj,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_RADAR,enable);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Enable_Radar(enable);
	}
}

void DLLEXPORT New_Display_GDI_Player_Terminal_Player(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_GDITERMINAL,0);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Display_GDI_Player_Terminal();
	}
}

void DLLEXPORT New_Display_NOD_Player_Terminal_Player(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_NODTERMINAL,0);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Display_NOD_Player_Terminal();
	}
}

void DLLEXPORT New_Set_Screen_Fade_Color_Player(GameObject *obj,float red,float green,float blue,float transition)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n%f\n",SS_SETSFC,red,green,blue,transition);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Screen_Fade_Color(red,green,blue,transition);
	}
}

void DLLEXPORT New_Set_Screen_Fade_Opacity_Player(GameObject *obj,float opacity,float transition)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n",SS_SETSFO,opacity,transition);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Screen_Fade_Opacity(opacity,transition);
	}
}

void DLLEXPORT New_Clear_Weapons(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	GameObject *o = As_VehicleGameObj(obj);
	if (o)
	{
		char buf[512];
		sprintf(buf,"j\n%d\n%d\n",SS_CLEARWEAPONS,Commands->Get_ID(obj));
		SendTextSc(buf,PublicMessage,0,-2,-1);
	}
	Clear_Weapons(obj);
}

void DLLEXPORT New_Enable_Vehicle_Transitions(GameObject *obj,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_VEHICLETRANSITIONS,Commands->Get_ID(obj),enable);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Enable_Vehicle_Transitions(obj,enable);
}

void DLLEXPORT New_Set_Screen_Fade_Color(float red,float green,float blue,float transition)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n%f\n",SS_SETSFC,red,green,blue,transition);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Screen_Fade_Color(red,green,blue,transition);
}

void DLLEXPORT New_Set_Screen_Fade_Opacity(float opacity,float transition)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n",SS_SETSFO,opacity,transition);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Screen_Fade_Opacity(opacity,transition);
}

void DLLEXPORT New_Shake_Camera(const Vector3 & pos,float radius,float intensity,float time)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n%f\n%f\n%f\n",SS_SHAKECAMERA,pos.X,pos.Y,pos.Z,radius,intensity,time);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Shake_Camera(pos,radius,intensity,time);
}

void DLLEXPORT New_Set_Display_Color(unsigned char red,unsigned char green,unsigned char blue)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n",SS_DISPLAYCOLOR,red,green,blue);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Display_Color(red,green,blue);
}

void DLLEXPORT New_Display_Text(int strnum)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_DISPLAYTEXT,strnum);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Display_Text(strnum);
}

void DLLEXPORT New_Display_Float(float num,const char *msg)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%s\n",SS_DISPLAYFLOAT,num,msg);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Display_Float(num,msg);
}

void DLLEXPORT New_Display_Int(int num,const char *msg)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%s\n",SS_DISPLAYINT,num,msg);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Display_Int(num,msg);
}

void DLLEXPORT New_Select_Weapon(GameObject *obj,const char *weapon)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	Select_Weapon(obj,weapon);
	VehicleGameObj *o = (VehicleGameObj *)As_VehicleGameObj(obj);
	if (o)
	{
		char buf[512];
		sprintf(buf,"j\n%d\n%d\n%s\n",SS_SELECTWEAPON,Commands->Get_ID(obj),weapon);
		SendTextSc(buf,PublicMessage,0,-2,-1);
		o->Init_Muzzle_Bones();
	}
}

void DLLEXPORT New_Set_Model(GameObject *obj,const char *model)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	VehicleGameObj *o = (VehicleGameObj *)As_VehicleGameObj(obj);
	if (o)
	{
		o->Release_Turret_Bones();
		o->Shutdown_Wheel_Effects();
	}
	Set_Model(obj,model);
	if (o)
	{
		o->Aquire_Turret_Bones();
		o->Init_Muzzle_Bones();
		o->Init_Wheel_Effects();
		o->Update_Damage_Meshes();
		WeaponBagClass *w = ((ArmedGameObj *)o)->Get_Weapon_Bag();
		if ((w->Get_Index()) && (w->Get_Index() < w->Get_Count()))
		{
			w->Peek_Weapon(w->Get_Index())->Clear_Firing_Sound();
		}
	}
	if (As_SoldierGameObj(obj))
	{
		Do_Soldier_Update((SoldierGameObj *)As_SoldierGameObj(obj));
	}
}

void DLLEXPORT New_Force_Camera_Look_Player(GameObject *obj,const Vector3 & look)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f",SS_FORCELOOK,look.X,look.Y,look.Z);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Force_Camera_Look(look);
	}
}

void DLLEXPORT New_Enable_HUD_Player(GameObject *obj,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_ENABLEHUD,enable);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Enable_HUD(enable);
	}
}

void DLLEXPORT New_Set_Display_Color_Player(GameObject *obj,unsigned char red,unsigned char green,unsigned char blue)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n",SS_DISPLAYCOLOR,red,green,blue);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Display_Color(red,green,blue);
	}
}

void DLLEXPORT New_Display_Text_Player(GameObject *obj,int strnum)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_DISPLAYTEXT,strnum);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Display_Text(strnum);
	}
}

void DLLEXPORT New_Display_Float_Player(GameObject *obj,float num,const char *msg)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%s\n",SS_DISPLAYFLOAT,num,msg);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Display_Float(num,msg);
	}
}

void DLLEXPORT New_Display_Int_Player(GameObject *obj,int num,const char *msg)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%s\n",SS_DISPLAYINT,num,msg);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Display_Int(num,msg);
	}
}

void DLLEXPORT New_Enable_Stealth_Player(GameObject *obj,GameObject *obj2,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1) || !Commands->Get_ID(obj2) || !obj2)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_STEALTH,Commands->Get_ID(obj2),enable);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Enable_Stealth(obj2,enable);
	}
}

void DLLEXPORT New_Create_Sound_Player(GameObject *obj,const char *soundname,const Vector3 & position,GameObject *obj2)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1) || !Commands->Get_ID(obj2) || !obj2)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%f\n%f\n%f\n%d\n",SS_CREATESOUND,soundname,position.X,position.Y,position.Z,Commands->Get_ID(obj2));
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Create_Sound(soundname,position,obj2);
	}
}

void DLLEXPORT New_Create_2D_Sound_Player(GameObject *obj,const char *soundname)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DSOUND,soundname);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Create_2D_Sound(soundname);
	}
}

int DLLEXPORT NewAddObjectCreateHook(ObjectCreateHookStruct *h)
{
	hookon = true;
	ObjectCreateHooks.Add(h);
	return ObjectCreateHooks.Count();
}

void DLLEXPORT NewRemoveObjectCreateHook(int pos)
{
	if (ObjectCreateHooks[pos - 1])
	{
		ObjectCreateHooks[pos - 1] = 0;
	}
}

void DLLEXPORT RemoveKeyHook(int pos)
{
	if (KeyHooks[pos - 1])
	{
		KeyHooks[pos - 1] = 0;
	}
}

int DLLEXPORT AddKeyHook(KeyHookStruct *h)
{
	int x = KeyHooks.Count();
	for (int i = 0;i < x;i++)
	{
		if (KeyHooks[i])
		{
			if ((KeyHooks[i]->PlayerID == h->PlayerID) && (!_stricmp(KeyHooks[i]->key,h->key)))
			{
				KeyHooks[i] = 0;
			}
		}
	}
	KeyHooks.Add(h);
	return KeyHooks.Count();
}

void DLLEXPORT AddChatHook(ChatHook h)
{
	Chat_Hook = h;
}

void DLLEXPORT AddHostHook(HostHook h)
{
	Host_Hook = h;
}

void DLLEXPORT AddPlayerJoinHook(PlayerJoin h)
{
	Player_Join_Hook = h;
}

void DLLEXPORT AddVersionHook(TTVersion h)
{
	Version_Hook = h;
}

void DLLEXPORT AddLoadLevelHook(LoadLevelHook h)
{
	Load_Level_Hook = h;
}

void DLLEXPORT AddThinkHook(LoadLevelHook h)
{
	Think_Hook = h;
}

void DLLEXPORT AddGameOverHook(LoadLevelHook h)
{
	Game_Over_Hook = h;
}

int DLLEXPORT AddPowerupPurchaseHook(PurchaseHook h,const char *data)
{
	Powerup_Purchase_Hooks_Data.Add(data);
	Powerup_Purchase_Hooks.Add(h);
	return Powerup_Purchase_Hooks.Count();
}

int DLLEXPORT AddVehiclePurchaseHook(PurchaseHook h,const char *data)
{
	Vehicle_Purchase_Hooks_Data.Add(data);
	Vehicle_Purchase_Hooks.Add(h);
	return Vehicle_Purchase_Hooks.Count();
}

int DLLEXPORT AddCharacterPurchaseHook(PurchaseHook h,const char *data)
{
	Character_Purchase_Hooks_Data.Add(data);
	Character_Purchase_Hooks.Add(h);
	return Character_Purchase_Hooks.Count();
}

int DLLEXPORT AddPowerupPurchaseMonHook(PurchaseMonHook h,const char *data)
{
	Powerup_Purchase_Monitor_Hooks_Data.Add(data);
	Powerup_Purchase_Monitor_Hooks.Add(h);
	return Powerup_Purchase_Monitor_Hooks.Count();
}

int DLLEXPORT AddVehiclePurchaseMonHook(PurchaseMonHook h,const char *data)
{
	Vehicle_Purchase_Monitor_Hooks_Data.Add(data);
	Vehicle_Purchase_Monitor_Hooks.Add(h);
	return Vehicle_Purchase_Monitor_Hooks.Count();
}

int DLLEXPORT AddCharacterPurchaseMonHook(PurchaseMonHook h,const char *data)
{
	Character_Purchase_Monitor_Hooks_Data.Add(data);
	Character_Purchase_Monitor_Hooks.Add(h);
	return Character_Purchase_Monitor_Hooks.Count();
}

void DLLEXPORT RemovePowerupPurchaseHook(int pos)
{
	if (Powerup_Purchase_Hooks[pos - 1])
	{
		Powerup_Purchase_Hooks[pos - 1] = 0;
	}
}

void DLLEXPORT RemoveVehiclePurchaseHook(int pos)
{
	if (Vehicle_Purchase_Hooks[pos - 1])
	{
		Vehicle_Purchase_Hooks[pos - 1] = 0;
	}
}

void DLLEXPORT RemoveCharacterPurchaseHook(int pos)
{
	if (Character_Purchase_Hooks[pos - 1])
	{
		Character_Purchase_Hooks[pos - 1] = 0;
	}
}

void DLLEXPORT RemovePowerupPurchaseMonHook(int pos)
{
	if (Powerup_Purchase_Monitor_Hooks[pos - 1])
	{
		Powerup_Purchase_Monitor_Hooks[pos - 1] = 0;
	}
}

void DLLEXPORT RemoveVehiclePurchaseMonHook(int pos)
{
	if (Vehicle_Purchase_Monitor_Hooks[pos - 1])
	{
		Vehicle_Purchase_Monitor_Hooks[pos - 1] = 0;
	}
}

void DLLEXPORT RemoveCharacterPurchaseMonHook(int pos)
{
	if (Character_Purchase_Monitor_Hooks[pos - 1])
	{
		Character_Purchase_Monitor_Hooks[pos - 1] = 0;
	}
}

void DLLEXPORT New_Create_2D_WAV_Sound_Player(GameObject *obj,const char *soundname)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DWAVSOUND,soundname);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Create_2D_WAV_Sound(soundname);
	}
}

void DLLEXPORT New_Create_3D_WAV_Sound_At_Bone_Player(GameObject *obj,const char *soundname,GameObject *obj2,const char *bonename)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1) || !Commands->Get_ID(obj2) || !obj2)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DWAVSOUND,soundname,Commands->Get_ID(obj2),bonename);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Create_3D_WAV_Sound_At_Bone(soundname,obj2,bonename);
	}
}

void DLLEXPORT New_Create_3D_Sound_At_Bone_Player(GameObject *obj,const char *soundname,GameObject *obj2,const char *bonename)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1) || !Commands->Get_ID(obj2) || !obj2)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DSOUND,soundname,Commands->Get_ID(obj2),bonename);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Create_3D_Sound_At_Bone(soundname,obj2,bonename);
	}
}

const char DLLEXPORT *GetCurrentMusicTrack()
{
	return newstr(CurrentSong);
}

double DLLEXPORT GetTTVersion()
{
	return TT_VERSION;
}

void DLLEXPORT New_Set_Obj_Radar_Blip_Shape(GameObject *obj,int shape)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_BLIPSHAPE,Commands->Get_ID(obj),shape);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Obj_Radar_Blip_Shape(obj,shape);
}

void DLLEXPORT New_Set_Obj_Radar_Blip_Color(GameObject *obj,int color)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_BLIPCOLOR,Commands->Get_ID(obj),color);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Obj_Radar_Blip_Color(obj,color);
}

void DLLEXPORT New_Set_Obj_Radar_Blip_Shape_Player(GameObject *obj,GameObject *obj2,int shape)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_BLIPSHAPE,Commands->Get_ID(obj2),shape);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Obj_Radar_Blip_Shape(obj2,shape);
	}
}

void DLLEXPORT New_Set_Obj_Radar_Blip_Color_Player(GameObject *obj,GameObject *obj2,int color)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_BLIPCOLOR,Commands->Get_ID(obj2),color);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Obj_Radar_Blip_Color(obj2,color);
	}
}

void DLLEXPORT Set_Info_Texture(GameObject *obj,const char *texture)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_INFOCHANGE,texture);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		SetInfoTexture(texture);
	}
}

void DLLEXPORT Clear_Info_Texture(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_INFODISABLE,0);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		ClearInfoTexture();
	}
}

void DLLEXPORT Send_Message_Player(GameObject *obj,unsigned int red,unsigned int green,unsigned int blue,const char *msg)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%s\n",SS_ADDMESSAGE,red,green,blue,msg);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		AddCombatMessage(msg,red,green,blue);
	}
}

void DLLEXPORT Send_Message(unsigned int red,unsigned int green,unsigned int blue,const char *msg)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%s\n",SS_ADDMESSAGE,red,green,blue,msg);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	AddCombatMessage(msg,red,green,blue);
}

void DLLEXPORT Set_Vehicle_Limit(unsigned int limit)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_VEHICLELIMIT,limit);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	VehicleCount = limit;
}

unsigned int DLLEXPORT Get_Vehicle_Limit()
{
	return VehicleCount;
}

void DLLEXPORT New_Display_Health_Bar(GameObject *obj,bool display)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_HEALTHBAR,Commands->Get_ID(obj),display);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Display_Health_Bar(obj,display);
}

void DLLEXPORT Set_Wireframe_Mode(unsigned int mode)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_WIREFRAME,mode);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	WireframeMode = mode;
	if (Exe)
	{
		SetWireframeMode(mode);
	}
}

void DLLEXPORT New_Disable_All_Collisions(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_DISABLEALLCOLLISIONS,Commands->Get_ID(obj));
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Disable_All_Collisions(obj);
}

void DLLEXPORT New_Disable_Physical_Collisions(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_DISABLEPHYSCOLLISIONS,Commands->Get_ID(obj));
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Disable_Physical_Collisions(obj);
}

void DLLEXPORT New_Enable_Collisions(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_ENABLECOLLISIONS,Commands->Get_ID(obj));
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Enable_Collisions(obj);
}

void DLLEXPORT Load_New_HUD_INI(GameObject *obj,const char *ini)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_HUDINI,ini);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		ReadRuntimeINI(ini);
	}
}

void DLLEXPORT Remove_Weapon(GameObject *obj,const char *weapon)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%s\n",SS_REMOVEWEAPON,Commands->Get_ID(obj),weapon);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Remove_Weap(obj,weapon);
}

void DLLEXPORT Update_PT_Data()
{
	PTDataChanged = true;
	Send_PT_Data();
}

float DLLEXPORT Get_Build_Time_Multiplier(int team)
{
	if (Is_Base_Powered(team))
	{
		return 1.0;
	}
	return BuildTimeDelay;
}

void DLLEXPORT Change_Radar_Map(float scale,float offsetx,float offsety,const char *texture)
{
	MapScale = scale;
	MapOffset.X = offsetx;
	MapOffset.Y = offsety;
	strcpy(MapTexture,texture);
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n%f\n%s\n",SS_RADARMAP,MapScale,MapOffset.X,MapOffset.Y,MapTexture);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	SendRadarData = true;
	if (!Exe)
	{
		Update_Radar_Map(scale,offsetx,offsety,texture);
	}
}

void DLLEXPORT Set_Currently_Building(bool building,int team)
{
	SetCurrentlyBuilding(building,team);
}

bool DLLEXPORT Is_Currently_Building(int team)
{
	return CurrentlyBuildingTeam[team];
}

void DLLEXPORT AddConsoleOutputHook(ConsoleOutputHook h)
{
	Console_Output_Hook = h;
}

void DLLEXPORT New_Set_Fog_Color(unsigned int red, unsigned int green, unsigned int blue)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n",SS_FOGCOLOR,red,green,blue);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Fog_Color(red,green,blue);
}

void DLLEXPORT Set_Fog_Color_Player(GameObject *obj,unsigned int red, unsigned int green, unsigned int blue)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n",SS_FOGCOLOR,red,green,blue);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Fog_Color(red,green,blue);
	}
}

void DLLEXPORT Set_Fog_Override_Player(GameObject *obj,float start, float end)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n%f\n",SS_FOGOVERRIDE,start,end);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		SetFogOverride(start,end);
	}
}

void DLLEXPORT Clear_Fog_Override_Player(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_CLEARFOGOVERRIDE,0);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		ClearFogOverride();
	}
}

void DLLEXPORT New_Set_Fog_Mode(unsigned int mode)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_FOGMODE,mode);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Fog_Mode(mode);
}

void DLLEXPORT Set_Fog_Mode_Player(GameObject *obj,unsigned int mode)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_FOGMODE,mode);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Fog_Mode(mode);
	}
}

void DLLEXPORT New_Set_Fog_Density(float density)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n",SS_FOGDENSITY,density);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Set_Fog_Density(density);
}

void DLLEXPORT Set_Fog_Density_Player(GameObject *obj,float density)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n",SS_FOGDENSITY,density);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Set_Fog_Density(density);
	}
}

void DLLEXPORT New_Change_Time_Remaining(float time)
{
	Change_Time_Remaining(time);
}

void DLLEXPORT New_Change_Time_Limit(int time)
{
	Change_Time_Limit(time);
}

void DLLEXPORT Do_Objectives_Dlg(GameObject *obj,char *file)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_TEXTFILE,file);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		ObjectivesTextDlg::Do_Dialog(file);
	}
}

void DLLEXPORT Display_GDI_Sidebar(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n",SS_GDISIDEBAR);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		SidebarDlg::Do_Dialog(0);
	}
}

void DLLEXPORT Display_NOD_Sidebar(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n",SS_NODSIDEBAR);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		SidebarDlg::Do_Dialog(1);
	}
}

void DLLEXPORT Display_Security_Dialog(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n",SS_PTSECURITY);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		Display_Dialog(230);
	}
}

void DLLEXPORT AddDataHook(DataHook h)
{
	Data_Hook = h;
}

void DLLEXPORT AddPlayerLeaveHook(PlayerLeave h)
{
	Player_Leave_Hook = h;
}

void DLLEXPORT Send_Shader_Param_Player(GameObject *obj,const char *value,const char *value2)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%s\n",SS_SENDPARAM,value,value2);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		if (!Exe)
		{
			ShaderSet(value,value2);
		}
	}
}

void DLLEXPORT Send_Shader_Param(const char *value,const char *value2)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%s\n",SS_SENDPARAM,value,value2);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	if (!Exe)
	{
		ShaderSet(value,value2);
	}
}

void DLLEXPORT SendHUDNumber(GameObject *obj,unsigned int num)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_HUDNUMBER,num);
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		if (!Exe)
		{
			Send_HUD_Number(num);
		}
	}
}

void DLLEXPORT SetExplosionObj(GameObject *obj)
{
	ExplosionObj = obj;
}

GameObject DLLEXPORT *GetExplosionObj()
{
	return ExplosionObj;
}

void DLLEXPORT New_Control_Enable(GameObject *obj,bool enable)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n",SS_CONTROLENABLE,Commands->Get_ID(obj),enable);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Control_Enable(obj,enable);
}

void DLLEXPORT Send_Shader_Param_Obj_Player(GameObject *obj,const char *value,const char *value2,GameObject *sendobj)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	if (!Commands->Get_ID(sendobj) || !sendobj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%s\n%d\n",SS_SENDPARAMOBJ,value,value2,Commands->Get_ID(sendobj));
	SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(obj));
	if ((!Exe) && (Get_Player_ID(obj) == cNetwork::PClientConnection->Get_Local_Id()))
	{
		if (!Exe)
		{
			ShaderSetObj(value,value2,sendobj);
		}
	}
}

void DLLEXPORT Send_Shader_Param_Obj(const char *value,const char *value2,GameObject *sendobj)
{
	if (!Commands->Get_ID(sendobj) || !sendobj)
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%s\n%d\n",SS_SENDPARAMOBJ,value,value2,Commands->Get_ID(sendobj));
	SendTextSc(buf,PublicMessage,0,-2,-1);
	if (!Exe)
	{
		ShaderSetObj(value,value2,sendobj);
	}
}

void DLLEXPORT RemoveShaderNotify(int pos)
{
	if (ShaderNotifies[pos - 1])
	{
		ShaderNotifies[pos - 1] = 0;
	}
}

int DLLEXPORT AddShaderNotify(ShaderNotifyStruct *h)
{
	int x = ShaderNotifies.Count();
	for (int i = 0;i < x;i++)
	{
		if (ShaderNotifies[i])
		{
			if ((ShaderNotifies[i]->PlayerID == h->PlayerID) && (ShaderNotifies[i]->ID == h->ID))
			{
				ShaderNotifies[i] = 0;
			}
		}
	}
	ShaderNotifies.Add(h);
	return ShaderNotifies.Count();
}

void DLLEXPORT Set_Player_Limit(unsigned int playerLimit)
{
	if (The_Game())
		The_Game()->Set_Max_Players(playerLimit);
}

unsigned int DLLEXPORT Get_Player_Limit()
{
	return The_Game() ? The_Game()->MaxPlayers : 0;
}

void DLLEXPORT New_Static_Anim_Phys_Goto_Frame(int ObjectID,float Frame,char *Anim)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%f\n%s\n",SS_GOTOFRAME,ObjectID,Frame,Anim);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Static_Anim_Phys_Goto_Frame(ObjectID,Frame,Anim);
}

void DLLEXPORT New_Static_Anim_Phys_Goto_Last_Frame(int ObjectID,char *Anim)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%s\n",SS_GOTOLASTFRAME,ObjectID,Anim);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Static_Anim_Phys_Goto_Last_Frame(ObjectID,Anim);
}

void DLLEXPORT Set_GDI_Soldier_Name(const char *name)
{
	delete[] GDISoldierName;
	GDISoldierName = newstr(name);
}
void DLLEXPORT Set_Nod_Soldier_Name(const char *name)
{
	delete[] NodSoldierName;
	NodSoldierName = newstr(name);
}

void DLLEXPORT Display_Game_Hint(GameObject *obj,int EventID,const char *Sound,int TitleID,int TextID,int TextID2,int TextID3)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	(new GameHintEvent)->Init(Get_Player_ID(obj),EventID,Sound,TitleID,TextID,TextID2,TextID3);
}

void DLLEXPORT Display_Game_Hint_Image(GameObject *obj,int EventID,const char *Sound,int TitleID,int TextID,int TextID2,int TextID3,const char *TextureName)
{
	if (!Commands->Get_ID(obj) || !obj || !Commands->Is_A_Star(obj) || (!Get_Player_ID(obj)) || (Get_Player_ID(obj) == -1))
	{
		return;
	}
	(new GameHintEvent)->Init(Get_Player_ID(obj),EventID,Sound,TitleID,TextID,TextID2,TextID3,TextureName);
}

void DLLEXPORT Set_Current_Bullets(GameObject *obj,int bullets)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	PhysicalGameObj *o2 = ((ScriptableGameObj *)obj)->As_PhysicalGameObj();
	if (!o2)
	{
		return;
	}
	ArmedGameObj *o3 = o2->As_ArmedGameObj();
	if (!o3)
	{
		return;
	}
	WeaponBagClass *w = o3->Get_Weapon_Bag();
	if ((w->Get_Index()) && (w->Get_Index() < w->Get_Count()))
	{
		w->Peek_Weapon(w->Get_Index())->Set_Inventory_Rounds_Client(bullets);
	}
}

void DLLEXPORT Set_Current_Clip_Bullets(GameObject *obj,int bullets)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	PhysicalGameObj *o2 = ((ScriptableGameObj *)obj)->As_PhysicalGameObj();
	if (!o2)
	{
		return;
	}
	ArmedGameObj *o3 = o2->As_ArmedGameObj();
	if (!o3)
	{
		return;
	}
	WeaponBagClass *w = o3->Get_Weapon_Bag();
	if ((w->Get_Index()) && (w->Get_Index() < w->Get_Count()))
	{
		w->Peek_Weapon(w->Get_Index())->Set_Clip_Rounds_Client(bullets);
	}
}

void DLLEXPORT Set_Position_Bullets(GameObject *obj,int position,int bullets)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	PhysicalGameObj *o2 = ((ScriptableGameObj *)obj)->As_PhysicalGameObj();
	if (!o2)
	{
		return;
	}
	ArmedGameObj *o3 = o2->As_ArmedGameObj();
	if (!o3)
	{
		return;
	}
	WeaponBagClass *w = o3->Get_Weapon_Bag();
	if ((position) && (position < w->Get_Count()))
	{
		w->Peek_Weapon(position)->Set_Inventory_Rounds_Client(bullets);
	}
}

void DLLEXPORT Set_Position_Clip_Bullets(GameObject *obj,int position,int bullets)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	PhysicalGameObj *o2 = ((ScriptableGameObj *)obj)->As_PhysicalGameObj();
	if (!o2)
	{
		return;
	}
	ArmedGameObj *o3 = o2->As_ArmedGameObj();
	if (!o3)
	{
		return;
	}
	WeaponBagClass *w = o3->Get_Weapon_Bag();
	if ((position) && (position < w->Get_Count()))
	{
		w->Peek_Weapon(position)->Set_Clip_Rounds_Client(bullets);
	}
}

void DLLEXPORT Set_Bullets(GameObject *obj,const char *weapon,int bullets)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	PhysicalGameObj *o2 = ((ScriptableGameObj *)obj)->As_PhysicalGameObj();
	if (!o2)
	{
		return;
	}
	ArmedGameObj *o3 = o2->As_ArmedGameObj();
	if (!o3)
	{
		return;
	}
	WeaponBagClass *w = o3->Get_Weapon_Bag();
	int x = w->Get_Count();
	for (int i = 0;i < x;i++)
	{
		if (w->Peek_Weapon(i))
		{
			if (!_stricmp(w->Peek_Weapon(i)->Get_Name(),weapon))
			{
				w->Peek_Weapon(i)->Set_Inventory_Rounds_Client(bullets);
			}
		}
	}
}

void DLLEXPORT Set_Clip_Bullets(GameObject *obj,const char *weapon,int bullets)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	PhysicalGameObj *o2 = ((ScriptableGameObj *)obj)->As_PhysicalGameObj();
	if (!o2)
	{
		return;
	}
	ArmedGameObj *o3 = o2->As_ArmedGameObj();
	if (!o3)
	{
		return;
	}
	WeaponBagClass *w = o3->Get_Weapon_Bag();
	int x = w->Get_Count();
	for (int i = 0;i < x;i++)
	{
		if (w->Peek_Weapon(i))
		{
			if (!_stricmp(w->Peek_Weapon(i)->Get_Name(),weapon))
			{
				w->Peek_Weapon(i)->Set_Clip_Rounds_Client(bullets);
			}
		}
	}
}

void DLLEXPORT Set_Moon_Is_Earth(bool earth)
{
	BackgroundMgrClass::Set_Moon_Is_Earth(earth);
}

int DLLEXPORT Get_Mine_Limit()
{
	return MineLimit;
}
}
