/*	Renegade Scripts.dll
	Copyright 2013 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "General.h"
#include "engine_tt.h"
#include "gmplugin.h"
#include "gmgame.h"
#include "gmlog.h"
#include "weaponmgr.h"
#include "PurchaseSettingsDefClass.h"
#include "PowerupGameObj.h"
#include "PowerupGameObjDef.h"
GameObject *CTFController = 0;

char *CTFIcons[4][2] = {
	//{Model,Animation},
	{"",""},
	{"O_EM_CHEVRON", "O_EM_CHEVRON.O_EM_CHEVRON"}, //Flag on the ped
	{"O_EM_REDARR","O_EM_REDARR.O_EM_REDARR"}, //Flag carried by enemy player
	{"O_EM_GRNARR","O_EM_GRNARR.O_EM_GRNARR"} //Flag on the ground
};

char *CTFAnimations[12] = {
	"H_A_SIDEKICK","H_A_PUNCHCOMBO","H_A_A0A0_L02","H_A_J22C","H_A_A0A0_L23",
	"H_A_A0A0_L58","H_A_B0A0_L05","H_A_A0A0_L12","H_A_J14C","H_A_X33C",
	"H_A_A0A0_L22","H_A_A0A0_L24"
};

char *CTFTeamSounds[6][2] = {
	//{Nod,GDI},
	{"",""},
	{"MXXDSGN_DSGN0011I1EVAN_SND.wav","MXXDSGN_DSGN0001I1EVAG_SND.wav"}, //Steal
	{"",""}, //Drop
	{"",""}, //Pick up
	{"MXXDSGN_DSGN0044I1EVAN_SND.wav","MXXDSGN_DSGN0014I1EVAG_SND.wav"}, //Return
	{"MXXDSGN_DSGN0012I1EVAG_SND.wav","MXXDSGN_DSGN0042I1EVAN_SND.wav"}, //Capture
};

char *CTFSounds[5] = {
	"",
	"pickupflag.wav", //Steal and pick up
	"dropflag.wav", //Drop
	"levelchange.wav", //Return
	"capflag.wav" //Capture
};

class CTF : public Plugin
{
public:
	struct Configuration {
		int CTFCapLimit;
		Vector3 CTFFlagOffset;
		bool CTFRegenForHolder;
		float CTFRegenAmount;
		StringClass CTFFlagPreset;
		StringClass CTFFlagModel[2];
		StringClass CTFFlagBackPreset;
		StringClass CTFPedPreset[2];
		float CTFCapPoints;
		float CTFCapPointsTeam;
		float CTFCapMoney;
		float CTFCapMoneyTeam;
		float CTFReturnTime;
		bool CTFFriendlyFlagRadar;
		bool CTFEnemyFlagRadar;
		bool CTFEnableReverse;
		int CTFClass;
		StringClass CTFClassName;
		bool CTFReverse;
		Vector3 CTFPedPos[2];
		void setToDefaults()
		{
			CTFCapLimit = 3;
			CTFFlagOffset = Vector3(0,0,0);
			CTFRegenForHolder = false;
			CTFRegenAmount = 2.0;
			CTFFlagPreset = "POW_Data_Disc";
			CTFFlagModel[0] = "o_flag";
			CTFFlagModel[1] = "o_flag";
			CTFFlagBackPreset = "CtfFlag";
			CTFPedPreset[0] = "CtfPedestalRed";
			CTFPedPreset[1] = "CtfPedestalGold";
			CTFCapPoints = 0;
			CTFCapPointsTeam = 0;
			CTFCapMoney = 0;
			CTFCapMoneyTeam = 0;
			CTFReturnTime = 120;
			CTFFriendlyFlagRadar = false;
			CTFEnemyFlagRadar = false;
			CTFEnableReverse = false;
			CTFClass = 0;
			CTFClassName = "NULL";
			CTFReverse = false;
			CTFPedPos[0] = Vector3(0,0,0);
			CTFPedPos[1] = Vector3(0,0,0);
		}
	};
	int CTFFlagHolder[2];
	int CTFFlagAttached[2][2];
	int CTFCaps[2];
	int CTFFlagLoc[2];
	int CTFFlagIcon[2];
	Configuration globalConfiguration;
	Configuration mapConfiguration;
	CTF()
	{
		CTFFlagHolder[0] = 0;
		CTFFlagHolder[1] = 0;
		CTFFlagAttached[0][0] = 0;
		CTFFlagAttached[0][1] = 0;
		CTFFlagAttached[1][0] = 0;
		CTFFlagAttached[1][1] = 0;
		CTFCaps[0] = 0;
		CTFCaps[1] = 0;
		RegisterEvent(EVENT_CHAT_HOOK,this);
		RegisterEvent(EVENT_GAME_OVER_HOOK,this);
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_MAP_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
		RegisterEvent(EVENT_PLAYER_JOIN_HOOK,this);
	}
	~CTF()
	{
		UnregisterEvent(EVENT_CHAT_HOOK,this);
		UnregisterEvent(EVENT_GAME_OVER_HOOK,this);
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_MAP_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
		UnregisterEvent(EVENT_PLAYER_JOIN_HOOK,this);
	}
	void LoadGenericINISettings(INIClass *SSGMIni, const char* key, Configuration& configuration)
	{
		configuration.CTFCapLimit = SSGMIni->Get_Int("CTF","CaptureLimit",configuration.CTFCapLimit);
		configuration.CTFFlagOffset.X = SSGMIni->Get_Float("CTF","FlagOffsetx",configuration.CTFFlagOffset.X);
		configuration.CTFFlagOffset.Y = SSGMIni->Get_Float("CTF","FlagOffsety",configuration.CTFFlagOffset.Y);
		configuration.CTFFlagOffset.Z = SSGMIni->Get_Float("CTF","FlagOffsetz",configuration.CTFFlagOffset.Z);
		configuration.CTFRegenForHolder = SSGMIni->Get_Bool("CTF","EnableRegenForHolder",configuration.CTFRegenForHolder);
		configuration.CTFRegenAmount = SSGMIni->Get_Float("CTF","RegenAmount",configuration.CTFRegenAmount);
		SSGMIni->Get_String(configuration.CTFFlagPreset,"CTF","FlagPresetName",configuration.CTFFlagPreset);
		SSGMIni->Get_String(configuration.CTFFlagModel[0],"CTF","FlagModelName0",configuration.CTFFlagModel[0]);
		SSGMIni->Get_String(configuration.CTFFlagModel[1],"CTF","FlagModelName1",configuration.CTFFlagModel[1]);
		SSGMIni->Get_String(configuration.CTFFlagBackPreset,"CTF","CarriedFlagPresetName",configuration.CTFFlagBackPreset);
		SSGMIni->Get_String(configuration.CTFPedPreset[0],"CTF","PedestalPresetName0",configuration.CTFPedPreset[0]);
		SSGMIni->Get_String(configuration.CTFPedPreset[1],"CTF","PedestalPresetName1",configuration.CTFPedPreset[1]);
		configuration.CTFCapPoints = SSGMIni->Get_Float("CTF","CapturePoints",configuration.CTFCapPoints);
		configuration.CTFCapPointsTeam = SSGMIni->Get_Float("CTF","CapturePointsTeam",configuration.CTFCapPointsTeam);
		configuration.CTFCapMoney = SSGMIni->Get_Float("CTF","CaptureMoney",configuration.CTFCapMoney);
		configuration.CTFCapMoneyTeam = SSGMIni->Get_Float("CTF","CaptureMoneyTeam",configuration.CTFCapMoneyTeam);
		configuration.CTFReturnTime = SSGMIni->Get_Float("CTF","ReturnDropFlagTime",configuration.CTFReturnTime);
		configuration.CTFFriendlyFlagRadar = SSGMIni->Get_Bool("CTF","ShowFriendlyFlagRadar",configuration.CTFFriendlyFlagRadar);
		configuration.CTFEnemyFlagRadar = SSGMIni->Get_Bool("CTF","ShowEnemyFlagRadar",configuration.CTFEnemyFlagRadar);
		configuration.CTFEnableReverse = SSGMIni->Get_Bool("CTF","EnableReverseCTF",configuration.CTFEnableReverse);
	}
	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		globalConfiguration.setToDefaults();
		LoadGenericINISettings(SSGMIni, "CTF", globalConfiguration);
	}
	virtual void OnLoadMapINISettings(INIClass *SSGMIni)
	{
		mapConfiguration = globalConfiguration;
		const StringClass key = StringClass(The_Game()->Get_Map_Name()).AsLower() + "_CTF";
		LoadGenericINISettings(SSGMIni, key, mapConfiguration);
		int Class = SSGMIni->Get_Int(key,"ForceClass",0);
		if (!Class)
		{
			DynamicVectorClass<int> Classes;
			for (int z = 1; ; ++z) {
				StringClass Value;
				StringClass v;
				v.Format("CTF_C%02d_Name",z);
				SSGMIni->Get_String(Value,key,v,"NULL");
				if (Value == "NULL") {
					break;
				}
				bool Enabled,Reverse;
				v.Format("CTF_C%02d_Enabled",z);
				Enabled = SSGMIni->Get_Bool(key,v,true);
				v.Format("CTF_C%02d_Reverse",z);
				Reverse = SSGMIni->Get_Bool(key,v,false);
				if (Enabled) {
					if (Reverse) {
						if (mapConfiguration.CTFEnableReverse) {
							Classes.Add(z);
						}
					}
					else {
						Classes.Add(z);
					}
				}
			}

			if (!Classes.Count()) {
				SSGMGameLog::Log_Message("Critical Error! No CTF coordinates were found. Exiting FDS.","_ERROR");
				MessageBox(HWND_DESKTOP,"No CTF coordinates found!","Critical SSGM Error",MB_OK|MB_ICONEXCLAMATION);
				Console_Input("EXIT");
			}
			else
			{
				Class = Classes[Commands->Get_Random_Int(0,Classes.Count())];
			}
		}

		mapConfiguration.CTFClass = Class;
		StringClass v;
		v.Format("CTF_C%02d_Name",mapConfiguration.CTFClass);
		SSGMIni->Get_String(mapConfiguration.CTFClassName,v,mapConfiguration.CTFClassName);
		v.Format("CTF_C%02d_Reverse",mapConfiguration.CTFClass);
		mapConfiguration.CTFReverse = SSGMIni->Get_Bool(key,v,mapConfiguration.CTFReverse);
		v.Format("CTF_C%02d_CaptureLimit",mapConfiguration.CTFClass);
		mapConfiguration.CTFCapLimit = SSGMIni->Get_Int(key,v,mapConfiguration.CTFCapLimit);
		v.Format("CTF_C%02d_Team0x",mapConfiguration.CTFClass);
		mapConfiguration.CTFPedPos[0].X = SSGMIni->Get_Float(key,v,mapConfiguration.CTFPedPos[0].X);
		v.Format("CTF_C%02d_Team0y",mapConfiguration.CTFClass);
		mapConfiguration.CTFPedPos[0].Y = SSGMIni->Get_Float(key,v,mapConfiguration.CTFPedPos[0].Y);
		v.Format("CTF_C%02d_Team0z",mapConfiguration.CTFClass);
		mapConfiguration.CTFPedPos[0].Z = SSGMIni->Get_Float(key,v,mapConfiguration.CTFPedPos[0].Z);
		v.Format("CTF_C%02d_Team1x",mapConfiguration.CTFClass);
		mapConfiguration.CTFPedPos[1].X = SSGMIni->Get_Float(key,v,mapConfiguration.CTFPedPos[1].X);
		v.Format("CTF_C%02d_Team1y",mapConfiguration.CTFClass);
		mapConfiguration.CTFPedPos[1].Y = SSGMIni->Get_Float(key,v,mapConfiguration.CTFPedPos[1].Y);
		v.Format("CTF_C%02d_Team1z",mapConfiguration.CTFClass);
		mapConfiguration.CTFPedPos[1].Z = SSGMIni->Get_Float(key,v,mapConfiguration.CTFPedPos[1].Z);
	}
	void Set_Flag_Location(int Team, int Loc)
	{
		if (Team > 1 || Team < 0 || Loc > 3 || Loc < 1)
		{
			return;
		}
		CTFFlagLoc[Team] = Loc;
		GameObject *Icon = Commands->Find_Object(CTFFlagIcon[Team]);
		Commands->Set_Model(Icon,CTFIcons[Loc][0]);
		Commands->Set_Animation(Icon,CTFIcons[Loc][1],true,0,0.0f,-1.0f,false);
	}
	GameObject *Create_CTF_DropFlag(int Team, Vector3 Position)
	{
		if (Team > 1 || Team < 0)
		{
			return 0;
		}
		CTFFlagHolder[Team] = 0;
		CTFFlagAttached[Team][0] = 0;
		CTFFlagAttached[Team][1] = 0;
		Vector3 FlagLoc = Position;
		FlagLoc += mapConfiguration.CTFFlagOffset;
		GameObject *Flag = Commands->Create_Object(mapConfiguration.CTFFlagPreset,FlagLoc);
		Commands->Set_Player_Type(Flag,Team);
		Commands->Set_Model(Flag,mapConfiguration.CTFFlagModel[Team]);
		Commands->Attach_Script(Flag,"SSGM_CTF_DropFlag","");
		Commands->Attach_Script(Flag,"KAK_Prevent_Kill","");
		Set_Flag_Location(Team,3);
		return Flag;
	}
	GameObject *Create_CTF_PedFlag(int Team)
	{
		if (Team > 1 || Team < 0)
		{
			return 0;
		}
		CTFFlagHolder[Team] = 0;
		CTFFlagAttached[Team][0] = 0;
		CTFFlagAttached[Team][1] = 0;
		Vector3 FlagLoc = mapConfiguration.CTFPedPos[Team];
		FlagLoc += mapConfiguration.CTFFlagOffset;
		GameObject *Flag = Commands->Create_Object(mapConfiguration.CTFFlagPreset,FlagLoc);
		Commands->Set_Player_Type(Flag,Team);
		Commands->Set_Model(Flag,mapConfiguration.CTFFlagModel[Team]);
		Commands->Attach_Script(Flag,"SSGM_CTF_Flag","");
		Commands->Attach_Script(Flag,"KAK_Prevent_Kill","");
		Set_Flag_Location(Team,1);
		return Flag;
	}
	GameObject *Create_CTF_Pedestal(int Team)
	{
		if (Team > 1 || Team < 0)
		{
			return 0;
		}
		Vector3 PedPos = mapConfiguration.CTFPedPos[Team];
		GameObject *Ped = Commands->Create_Object(mapConfiguration.CTFPedPreset[Team],PedPos);
		Commands->Set_Player_Type(Ped,Team);
		Commands->Attach_Script(Ped,"KAK_Prevent_Kill","");
		Create_CTF_PedFlag(Team);
		PedPos.Z += mapConfiguration.CTFFlagOffset.Z;
		PedPos.Z += 1.25f;
		GameObject *Icon = Commands->Create_Object("Invisible_Object",PedPos);
		CTFFlagIcon[Team] = Commands->Get_ID(Icon);
		return Ped;
	}
	void CTF_Message(int Type, int Team, GameObject *Player1, GameObject *Player2)
	{
		if (Type == 1)
		{
			StringClass Msg;
			const char *str = Get_Team_Name(Team);
			Msg.Format("%ls has stolen the %s flag!",Get_Wide_Player_Name(Player1),str);
			delete[] str;
			SSGMGameLog::Log_Message(Msg,"_CTF");
			StringClass tmp;
			tmp.Format("msg %s",Msg.Peek_Buffer());
			Console_Input(tmp);
			Commands->Create_2D_WAV_Sound(CTFTeamSounds[1][Team]);
			Commands->Create_2D_WAV_Sound(CTFSounds[1]);
		}
		else if (Type == 2)
		{
			StringClass Msg;
			const char *str = Get_Team_Name(Team);
			Msg.Format("The %s flag, which was carried by %ls, has been dropped thanks to %ls.",str,Get_Wide_Player_Name(Player1),Get_Wide_Player_Name(Player2));
			delete[] str;
			SSGMGameLog::Log_Message(Msg,"_CTF");
			StringClass tmp;
			tmp.Format("msg %s",Msg.Peek_Buffer());
			Console_Input(tmp);
			Commands->Create_2D_WAV_Sound(CTFSounds[2]);
		}
		else if (Type == 3)
		{
			StringClass Msg;
			const char *str = Get_Team_Name(Team);
			Msg.Format("%ls has picked up the %s flag!",Get_Wide_Player_Name(Player1),str);
			delete[] str;
			SSGMGameLog::Log_Message(Msg,"_CTF");
			StringClass tmp;
			tmp.Format("msg %s",Msg.Peek_Buffer());
			Console_Input(tmp);
			Commands->Create_2D_WAV_Sound(CTFSounds[1]);
		}
		else if (Type == 4)
		{
			StringClass Msg;
			const char *str = Get_Team_Name(Team);
			Msg.Format("%ls returned the %s flag!",Get_Wide_Player_Name(Player1),str);
			delete[] str;
			SSGMGameLog::Log_Message(Msg,"_CTF");
			StringClass tmp;
			tmp.Format("msg %s",Msg.Peek_Buffer());
			Console_Input(tmp);
			Commands->Create_2D_WAV_Sound(CTFTeamSounds[4][Team]);
			Commands->Create_2D_WAV_Sound(CTFSounds[3]);
		}
		else if (Type == 5)
		{
			StringClass Msg;
			const char *str = Get_Team_Name(Team);
			Msg.Format("The %s flag has been automatically returned by the server.",str);
			delete[] str;
			SSGMGameLog::Log_Message(Msg,"_CTF");
			StringClass tmp;
			tmp.Format("msg %s",Msg.Peek_Buffer());
			Console_Input(tmp);
			Commands->Create_2D_WAV_Sound(CTFTeamSounds[4][Team]);
			Commands->Create_2D_WAV_Sound(CTFSounds[3]);
		}
		else if (Type == 6)
		{
			StringClass Msg;
			const char *strx = Get_Team_Name(Team);
			Msg.Format("%ls has captured the %s flag!",Get_Wide_Player_Name(Player1),strx);
			delete[] strx;
			SSGMGameLog::Log_Message(Msg,"_CTF");
			StringClass tmp;
			tmp.Format("msg %s",Msg.Peek_Buffer());
			Console_Input(tmp);
			if (mapConfiguration.CTFCapLimit > 0)
			{
				if ((mapConfiguration.CTFCapLimit - CTFCaps[Get_Object_Type(Player1)]) > 0) {
					const char *str = Get_Team_Name(Get_Object_Type(Player1));
					const char *str2 = Get_Team_Name(Team);
					Msg.Format("%s needs to capture the %s flag %d more time(s) to win.",str,str2,(mapConfiguration.CTFCapLimit - CTFCaps[Get_Object_Type(Player1)]));
					delete[] str;
					delete[] str2;
					SSGMGameLog::Log_Message(Msg,"_CTF");
					StringClass tmpx;
					tmpx.Format("msg %s",Msg.Peek_Buffer());
					Console_Input(tmpx);
				}
				else
				{
					const char *str = Get_Team_Name(Get_Object_Type(Player1));
					const char *str2 = Get_Team_Name(Team);
					Msg.Format("%s has won the game by capturing the %s flag %d time(s)!",str,str2,mapConfiguration.CTFCapLimit);
					delete[] str;
					delete[] str2;
					SSGMGameLog::Log_Message(Msg,"_CTF");
					StringClass tmpx;
					tmpx.Format("msg %s",Msg.Peek_Buffer());
					Console_Input(tmpx);
				}
			}
			Commands->Create_2D_WAV_Sound(CTFTeamSounds[5][Team]);
			Commands->Create_2D_WAV_Sound(CTFSounds[4]);
		}
	}
	virtual void OnPlayerJoin(int PlayerID,const char *PlayerName)
	{
		Commands->Send_Custom_Event(0,CTFController,983294,PlayerID,1.0f);
	}
	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if (Commands->Is_A_Star(obj))
		{
			if (CTFFlagHolder[PTTEAM(Get_Object_Type(obj))] == Commands->Get_ID(obj))
			{
				Attach_Script_Once(obj,"SSGM_CTF_FlagHolder","");
			}
		}
	}
	virtual void OnLoadLevel()
	{
		Commands->Attach_Script(Create_CTF_Pedestal(0),"SSGM_CTF_Announcement","");
		Create_CTF_Pedestal(1);
		Set_Flag_Location(0,1);
		Set_Flag_Location(1,1);
	}
	virtual void OnGameOver()
	{
		CTFFlagHolder[0] = 0;
		CTFFlagHolder[1] = 0;
		CTFFlagAttached[0][0] = 0;
		CTFFlagAttached[0][1] = 0;
		CTFFlagAttached[1][0] = 0;
		CTFFlagAttached[1][1] = 0;
		CTFCaps[0] = 0;
		CTFCaps[1] = 0;
	}
	virtual bool OnChat(int PlayerID,TextMessageEnum Type,const wchar_t *Message,int recieverID)
	{
		if (Message[0] == L'!')
		{
			if (wcsistr(Message,L"!capture") == Message)
			{
				StringClass tmp;
				const char *str = Get_Team_Name(0);
				const char *str2 = Get_Team_Name(1);
				tmp.Format("ppage %d %s Flag Captures: %d/%d - %s Flag Captures: %d/%d",PlayerID,str,CTFCaps[0],mapConfiguration.CTFCapLimit,str2,CTFCaps[1],mapConfiguration.CTFCapLimit);
				delete[] str;
				delete[] str2;
				Console_Input(tmp);
				return false;
			}
			if (wcsistr(Message,L"!flag") == Message)
			{
				StringClass Msg[2];
				if (CTFFlagLoc[1] == 1) {
					Msg[1] = "currently at its pedestal";
				}
				else if (CTFFlagLoc[1] == 2) {
					Msg[1].Format("in %ls's possesion",Get_Wide_Player_Name(Commands->Find_Object(CTFFlagHolder[1])));
				}
				else {
					Msg[1] = "in the field";
				}
	
				if (CTFFlagLoc[0] == 1) {
					Msg[0] = "currently at its pedestal";
				}
				else if (CTFFlagLoc[0] == 2) {
					Msg[0].Format("in %ls's possesion",Get_Wide_Player_Name(Commands->Find_Object(CTFFlagHolder[0])));
				}
				else {
					Msg[0] = "in the field";
				}
				StringClass tmp;
				const char *str = Get_Team_Name(0);
				const char *str2 = Get_Team_Name(1);
				tmp.Format("ppage %d The %s flag is %s. The %s flag is %s. The class is '%s'.",PlayerID,str,Msg[0].Peek_Buffer(),str2,Msg[1].Peek_Buffer(),mapConfiguration.CTFClassName);
				delete[] str;
				delete[] str2;
				Console_Input(tmp);
				return false;
			}
		}
		return true;
	}
};
CTF ctf;
extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &ctf;
}

class SSGM_CTF_Flag : public ScriptImpClass {
	void Created(GameObject *obj)
	{
		PickedUp = false;
		int Team = Get_Object_Type(obj);
		if (ctf.mapConfiguration.CTFFriendlyFlagRadar) {
			GameObject *AttachedFlag = Commands->Create_Object_At_Bone(obj,ctf.mapConfiguration.CTFFlagBackPreset,"Origin");
			Commands->Attach_To_Object_Bone(AttachedFlag, obj,"Origin");
			Commands->Disable_All_Collisions(AttachedFlag);
			Commands->Set_Player_Type(AttachedFlag,Team);
			Commands->Set_Model(AttachedFlag,"null");
			ctf.CTFFlagAttached[Team][0] = Commands->Get_ID(AttachedFlag);
		}
		if (ctf.mapConfiguration.CTFEnemyFlagRadar) {
			GameObject *AttachedFlag2 = Commands->Create_Object_At_Bone(obj,"Invisible_Object","Origin");
			Commands->Attach_To_Object_Bone(AttachedFlag2, obj, "Origin");
			Commands->Disable_All_Collisions(AttachedFlag2);
			Commands->Set_Player_Type(AttachedFlag2,PTTEAM(Team));
			Commands->Set_Model(AttachedFlag2,"null");
			Commands->Set_Obj_Radar_Blip_Shape(AttachedFlag2,RADAR_BLIP_SHAPE_OBJECTIVE);
			Commands->Set_Obj_Radar_Blip_Color(AttachedFlag2,Team);
			ctf.CTFFlagAttached[Team][1] = Commands->Get_ID(AttachedFlag2);
		}
		((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).Persistent = true;
		((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).AlwaysAllowGrant = true;
		((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).GrantSoundID = 0;
	}
	void Custom(GameObject *obj, int type, int param, GameObject *sender)
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED && !PickedUp && Commands->Get_Health(sender) > 0.0f)
		{
			int Team = Get_Object_Type(obj);
			if (Team != Get_Object_Type(sender))
			{
				Commands->Attach_Script(sender,"SSGM_CTF_FlagHolder","");
				ctf.CTFFlagHolder[Team] = Commands->Get_ID(sender);	
				ctf.CTF_Message(1,Team,sender,0);
				ctf.Set_Flag_Location(Team,2);
				PickedUp = true;
				Commands->Destroy_Object(obj);
			}
			else
			{
				int OtherTeam = PTTEAM(Team);
				if (ctf.CTFFlagHolder[OtherTeam] == Commands->Get_ID(sender))
				{
					Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[OtherTeam][0]));
					Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[OtherTeam][1]));
					Commands->Send_Custom_Event(obj,sender,294829,0,0);
					ctf.CTFFlagHolder[OtherTeam] = 0;
					ctf.CTFFlagAttached[OtherTeam][0] = 0;
					ctf.CTFCaps[Team]++;
					ctf.CTF_Message(6,OtherTeam,sender,0);	
					Commands->Give_Points(sender,ctf.mapConfiguration.CTFCapPoints,false);
					Commands->Give_Money(sender,(ctf.mapConfiguration.CTFCapPoints*-1.0f),false);
					Commands->Give_Points(sender,ctf.mapConfiguration.CTFCapPointsTeam,true);
					Commands->Give_Money(sender,(ctf.mapConfiguration.CTFCapPointsTeam*-1.0f),true);
					Commands->Give_Money(sender,ctf.mapConfiguration.CTFCapMoney,false);	
					Commands->Give_Money(sender,ctf.mapConfiguration.CTFCapMoneyTeam,true);
					int AnimSize = (sizeof(CTFAnimations)/sizeof(char*));
					int Animation2Play = Commands->Get_Random_Int(0,AnimSize);
					Commands->Set_Animation(sender,CTFAnimations[Animation2Play],false,0,0.0f,-1.0f,false);
					if (ctf.CTFCaps[Team] >= ctf.mapConfiguration.CTFCapLimit)
					{
						StringClass str;
						str.Format("WIN %d\n",Team);
						Console_Input(str);
					}
					else
					{
						ctf.Create_CTF_PedFlag(OtherTeam);
					}
				}
			}
		}
	}
	bool PickedUp;
};

class SSGM_CTF_DropFlag : public ScriptImpClass {
	void Created(GameObject *obj)
	{
		PickedUp = false;
		int Team = Get_Object_Type(obj);
		if (ctf.mapConfiguration.CTFFriendlyFlagRadar)
		{
			GameObject *AttachedFlag = Commands->Create_Object_At_Bone(obj,ctf.mapConfiguration.CTFFlagBackPreset,"Origin");
			Commands->Attach_To_Object_Bone(AttachedFlag, obj,"Origin");
			Commands->Disable_All_Collisions(AttachedFlag);
			Commands->Set_Player_Type(AttachedFlag,Team);
			Commands->Set_Model(AttachedFlag,"null");
			ctf.CTFFlagAttached[Team][0] = Commands->Get_ID(AttachedFlag);
		}	
		if (ctf.mapConfiguration.CTFEnemyFlagRadar)
		{
			GameObject *AttachedFlag2 = Commands->Create_Object_At_Bone(obj,"Invisible_Object","Origin");
			Commands->Attach_To_Object_Bone(AttachedFlag2, obj, "Origin");
			Commands->Disable_All_Collisions(AttachedFlag2);
			Commands->Set_Player_Type(AttachedFlag2,PTTEAM(Team));
			Commands->Set_Model(AttachedFlag2,"null");
			Commands->Set_Obj_Radar_Blip_Shape(AttachedFlag2,RADAR_BLIP_SHAPE_OBJECTIVE);
			Commands->Set_Obj_Radar_Blip_Color(AttachedFlag2,Team);
			ctf.CTFFlagAttached[Team][1] = Commands->Get_ID(AttachedFlag2);
		}
		if (ctf.mapConfiguration.CTFReturnTime > 0.0f)
		{
			Commands->Start_Timer(obj,this,ctf.mapConfiguration.CTFReturnTime,1);
		}
		((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).Persistent = true;
		((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).AlwaysAllowGrant = true;
		((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).GrantSoundID = 0;
	}
	void Custom(GameObject *obj, int type, int param, GameObject *sender)
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED && !PickedUp && Commands->Get_Health(sender) > 0.0f)
		{
			int Team = Get_Object_Type(obj);
			PickedUp = true;
			if (Team != Get_Object_Type(sender))
			{
				Commands->Attach_Script(sender,"SSGM_CTF_FlagHolder","");
				ctf.CTFFlagHolder[Team] = Commands->Get_ID(sender);	
				ctf.CTF_Message(3,Team,sender,0);
				ctf.Set_Flag_Location(Team,2);
				Commands->Destroy_Object(obj);
			}
			else
			{
				Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][0]));
				Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][1]));
				ctf.Create_CTF_PedFlag(Team);
				ctf.CTF_Message(4,Team,sender,0);
				Commands->Destroy_Object(obj);
			}
		}
	}
	void Timer_Expired(GameObject *obj, int number)
	{
		Commands->Destroy_Object(obj);
	}
	void Destroyed(GameObject *obj)
	{
		if (!PickedUp)
		{
			int Team = Get_Object_Type(obj);
			Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][0]));
			Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][1]));
			ctf.Create_CTF_PedFlag(Team);
			ctf.CTF_Message(5,Team,0,0);
		}
	}
	bool PickedUp;
};

class SSGM_CTF_FlagHolder : public ScriptImpClass {
	void Created(GameObject *obj)
	{
		Dropped = false;
		int Team = PTTEAM(Get_Object_Type(obj));
		Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][0]));
		Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][1]));
		GameObject *AttachedFlag = Commands->Create_Object_At_Bone(obj,ctf.mapConfiguration.CTFFlagBackPreset,"bone for bag");
		Commands->Attach_To_Object_Bone(AttachedFlag,obj,"bone for bag");
		Commands->Disable_All_Collisions(AttachedFlag);
		if (ctf.mapConfiguration.CTFFriendlyFlagRadar)
		{
			Commands->Set_Player_Type(AttachedFlag,Team);
		}
		else
		{
			Commands->Set_Player_Type(AttachedFlag,-2);
		}
		Commands->Set_Model(AttachedFlag,ctf.mapConfiguration.CTFFlagModel[Team]);
		ctf.CTFFlagAttached[Team][0] = Commands->Get_ID(AttachedFlag);
		if (ctf.mapConfiguration.CTFEnemyFlagRadar)
		{
			GameObject *AttachedFlag2 = Commands->Create_Object_At_Bone(obj,"Invisible_Object","bone for bag");
			Commands->Attach_To_Object_Bone(AttachedFlag2,obj,"bone for bag");
			Commands->Disable_All_Collisions(AttachedFlag2);
			Commands->Set_Player_Type(AttachedFlag2,Get_Object_Type(obj));
			Commands->Set_Model(AttachedFlag2,"null");
			Commands->Set_Obj_Radar_Blip_Shape(AttachedFlag2,RADAR_BLIP_SHAPE_OBJECTIVE);
			Commands->Set_Obj_Radar_Blip_Color(AttachedFlag2,Team);
			ctf.CTFFlagAttached[Team][1] = Commands->Get_ID(AttachedFlag2);
		}
		if (ctf.mapConfiguration.CTFRegenForHolder)
		{
			Commands->Start_Timer(obj,this,1.0f,1);
		}
		if (Is_Stealth(obj))
		{
			Commands->Start_Timer(obj,this,1.0f,2);
		}
	}
	void Killed(GameObject *obj, GameObject *killer)
	{
		if (!Dropped)
		{
			Dropped = true;
			int Team = PTTEAM(Get_Object_Type(obj));
			Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][0]));
			Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][1]));
			ctf.Create_CTF_DropFlag(Team,Commands->Get_Position(obj));
			ctf.CTF_Message(2,Team,obj,killer);
		}
	}
	void Destroyed(GameObject *obj)
	{
		if (!Dropped)
		{
			Dropped = true;
			int Team = PTTEAM(Get_Object_Type(obj));
			Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][0]));
			Commands->Destroy_Object(Commands->Find_Object(ctf.CTFFlagAttached[Team][1]));
			ctf.Create_CTF_DropFlag(Team,Commands->Get_Position(obj));
			ctf.CTF_Message(2,Team,obj,0);
		}
	}
	void Timer_Expired(GameObject *obj, int number)
	{
		if (number == 1 && Commands->Get_Health(obj) > 0.0f)
		{
			Commands->Apply_Damage(obj,(ctf.mapConfiguration.CTFRegenAmount * -1.0f),"Repair",0);
			Commands->Start_Timer(obj,this,1.0f,1);
		}
		else if (number == 2)
		{
			Commands->Apply_Damage(obj,0,"Harmless",false);
			Commands->Start_Timer(obj,this,1.0f,2);
		}
	}
	void Custom(GameObject *obj, int type, int param, GameObject *sender)
	{
		if (type == 294829)
		{
			Dropped = true;
			Destroy_Script();
		}
	}
	bool Dropped;
};

class SSGM_CTF_Announcement : public ScriptImpClass {
	void Created(GameObject *obj)
	{
		CTFController = obj;
		Commands->Start_Timer(obj,this,30.0f,1);
		Commands->Start_Timer(obj,this,90.0f,1);
	}
	void Timer_Expired(GameObject *obj, int number)
	{
		StringClass str;
		str.Format("msg Running in Capture The Flag mode. This rounds CTF class will be '%s' with a capture limit of %d.",ctf.mapConfiguration.CTFClassName.Peek_Buffer(),ctf.mapConfiguration.CTFCapLimit);
		Console_Input(str);
		if (ctf.mapConfiguration.CTFReverse)
		{
			Console_Input("msg This is a Reverse CTF game. Your objective is to take the enemy flag from your base and run it to your pedestal in the enemy base.");
			Console_Input("msg Keep in mind that returning your own flag is not always the best course of action!");
		}
	}
	void Custom(GameObject *obj, int type, int param, GameObject *sender)
	{
		if (type == 983294)
		{
			GameObject *Flag0 = Commands->Find_Object(ctf.CTFFlagAttached[0][1]);
			if (Flag0)
			{
				Commands->Set_Obj_Radar_Blip_Shape(Flag0,RADAR_BLIP_SHAPE_OBJECTIVE);
				Commands->Set_Obj_Radar_Blip_Color(Flag0,RADAR_BLIP_COLOR_NOD);
			}
			GameObject *Flag1 = Commands->Find_Object(ctf.CTFFlagAttached[1][1]);
			if (Flag1)
			{
				Commands->Set_Obj_Radar_Blip_Shape(Flag1,RADAR_BLIP_SHAPE_OBJECTIVE);
				Commands->Set_Obj_Radar_Blip_Color(Flag1,RADAR_BLIP_COLOR_GDI);
			}
		}
	}
};

ScriptRegistrant<SSGM_CTF_Flag> SSGM_CTF_Flag_Registrant("SSGM_CTF_Flag","");
ScriptRegistrant<SSGM_CTF_DropFlag> SSGM_CTF_DropFlag_Registrant("SSGM_CTF_DropFlag","");
ScriptRegistrant<SSGM_CTF_FlagHolder> SSGM_CTF_FlagHolder_Registrant("SSGM_CTF_FlagHolder","");
ScriptRegistrant<SSGM_CTF_Announcement> SSGM_CTF_Announcement_Registrant("SSGM_CTF_Announcement","");
