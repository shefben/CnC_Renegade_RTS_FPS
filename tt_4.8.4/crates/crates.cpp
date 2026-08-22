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
#include "TeamPurchaseSettingsDefClass.h"
#include "PurchaseSettingsDefClass.h"
#include "PowerupGameObj.h"
#include "PowerupGameObjDef.h"

class Crate : public Plugin
{
public:
	struct Configuration
	{
		bool EnableCrates;
		bool EnableNewCrates;
		bool VehCrate;
		Vector3	CrateVehSpawnPos[2];
		Vector3	CrateVehInfSpawnPos[2];
		int	CrateWeapon;
		int	CrateMoney;
		int	CratePoints;
		int	CrateVehicle;
		int	CrateDeath;
		int	CrateTiberium;
		int	CrateAmmo;
		int	CrateArmor;
		int	CrateHealth;
		int	CrateCharacter;
		int	CrateButterFingers;
		int	CrateRefill;
		int	CrateBeacon;
		int	CrateSpy;
		int	CrateStealth;
		int	CrateThief;
		int CrateAmmoRegen;
		int CrateHumanSilo;
		int CrateUberWire;

		void setToDefaults()
		{
			EnableCrates = true;
			EnableNewCrates = false;
			VehCrate = false;
			CrateVehSpawnPos[0] = Vector3(0, 0, 0);
			CrateVehSpawnPos[1] = Vector3(0, 0, 0);
			CrateVehInfSpawnPos[0] = Vector3(0, 0, 0);
			CrateVehInfSpawnPos[1] = Vector3(0, 0, 0);
		}
	};

	Configuration globalConfiguration;
	Configuration mapConfiguration;
	int CrateID;
	int CrateLastPickup;
	bool CrateExists;

	Crate()
	{
		CrateID = 0;
		CrateExists = false;
		CrateLastPickup = -181;

		RegisterEvent(EVENT_GAME_OVER_HOOK,this);
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_MAP_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	~Crate()
	{
		UnregisterEvent(EVENT_GAME_OVER_HOOK,this);
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_MAP_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
	}
	void LoadGenericINISettings(INIClass *SSGMIni, const char* key, Configuration& configuration)
	{
		configuration.EnableCrates       = SSGMIni->Get_Bool(key, "EnableCrates",    configuration.EnableCrates);
		configuration.EnableNewCrates    = SSGMIni->Get_Bool(key, "EnableNewCrates", configuration.EnableNewCrates);
		configuration.VehCrate           = SSGMIni->Get_Bool(key, "AllowVehCrates",  configuration.VehCrate);
		configuration.CrateWeapon        = SSGMIni->Get_Int (key, "Weapon",          configuration.CrateWeapon);
		configuration.CrateMoney         = SSGMIni->Get_Int (key, "Money",           configuration.CrateMoney);
		configuration.CratePoints        = SSGMIni->Get_Int (key, "Points",          configuration.CratePoints);
		configuration.CrateVehicle       = SSGMIni->Get_Int (key, "Vehicle",         configuration.CrateVehicle);
		configuration.CrateDeath         = SSGMIni->Get_Int (key, "Death",           configuration.CrateDeath);
		configuration.CrateTiberium      = SSGMIni->Get_Int (key, "Tiberium",        configuration.CrateTiberium);
		configuration.CrateAmmo          = SSGMIni->Get_Int (key, "Ammo",            configuration.CrateAmmo);
		configuration.CrateArmor         = SSGMIni->Get_Int (key, "Armor",           configuration.CrateArmor);
		configuration.CrateHealth        = SSGMIni->Get_Int (key, "Health",          configuration.CrateHealth);
		configuration.CrateCharacter     = SSGMIni->Get_Int (key, "Character",       configuration.CrateCharacter);
		configuration.CrateButterFingers = SSGMIni->Get_Int (key, "ButterFingers",   configuration.CrateButterFingers);
		configuration.CrateRefill        = SSGMIni->Get_Int (key, "Refill",          configuration.CrateRefill);
		configuration.CrateBeacon        = SSGMIni->Get_Int (key, "Beacon",          configuration.CrateBeacon);
		configuration.CrateSpy           = SSGMIni->Get_Int (key, "Spy",             configuration.CrateSpy);
		configuration.CrateStealth       = SSGMIni->Get_Int (key, "Stealth",         configuration.CrateStealth);
		configuration.CrateThief         = SSGMIni->Get_Int (key, "Thief",           configuration.CrateThief);
		configuration.CrateAmmoRegen     = SSGMIni->Get_Int (key, "AmmoRegen",       configuration.CrateAmmoRegen);
		configuration.CrateHumanSilo     = SSGMIni->Get_Int (key, "HumanSilo",       configuration.CrateHumanSilo);
		configuration.CrateUberWire      = SSGMIni->Get_Int (key, "UberWire",        configuration.CrateUberWire);
	}
	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		globalConfiguration.setToDefaults();
		LoadGenericINISettings(SSGMIni, "Crates", globalConfiguration);
	}
	virtual void OnLoadMapINISettings(INIClass *SSGMIni)
	{
		mapConfiguration = globalConfiguration;
		const StringClass key = StringClass(The_Game()->Get_Map_Name()).AsLower() + "_Crates";
		LoadGenericINISettings(SSGMIni, key, mapConfiguration);
		mapConfiguration.CrateVehSpawnPos[0].X = SSGMIni->Get_Float(key, "NodRVCX", mapConfiguration.CrateVehSpawnPos[0].X);
		mapConfiguration.CrateVehSpawnPos[0].Y = SSGMIni->Get_Float(key, "NodRVCY", mapConfiguration.CrateVehSpawnPos[0].Y);
		mapConfiguration.CrateVehSpawnPos[0].Z = SSGMIni->Get_Float(key, "NodRVCZ", mapConfiguration.CrateVehSpawnPos[0].Z);
		mapConfiguration.CrateVehSpawnPos[1].X = SSGMIni->Get_Float(key, "GDIRVCX", mapConfiguration.CrateVehSpawnPos[1].X);
		mapConfiguration.CrateVehSpawnPos[1].Y = SSGMIni->Get_Float(key, "GDIRVCY", mapConfiguration.CrateVehSpawnPos[1].Y);
		mapConfiguration.CrateVehSpawnPos[1].Z = SSGMIni->Get_Float(key, "GDIRVCZ", mapConfiguration.CrateVehSpawnPos[1].Z);
		mapConfiguration.CrateVehInfSpawnPos[0].X = SSGMIni->Get_Float(key, "NodRVCP_X", mapConfiguration.CrateVehInfSpawnPos[0].X);
		mapConfiguration.CrateVehInfSpawnPos[0].Y = SSGMIni->Get_Float(key, "NodRVCP_Y", mapConfiguration.CrateVehInfSpawnPos[0].Y);
		mapConfiguration.CrateVehInfSpawnPos[0].Z = SSGMIni->Get_Float(key, "NodRVCP_Z", mapConfiguration.CrateVehInfSpawnPos[0].Z);
		mapConfiguration.CrateVehInfSpawnPos[1].X = SSGMIni->Get_Float(key, "GDIRVCP_X", mapConfiguration.CrateVehInfSpawnPos[1].X);
		mapConfiguration.CrateVehInfSpawnPos[1].Y = SSGMIni->Get_Float(key, "GDIRVCP_Y", mapConfiguration.CrateVehInfSpawnPos[1].Y);
		mapConfiguration.CrateVehInfSpawnPos[1].Z = SSGMIni->Get_Float(key, "GDIRVCP_Z", mapConfiguration.CrateVehInfSpawnPos[1].Z);
	}
	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if (obj->As_PhysicalGameObj() && obj->As_PhysicalGameObj()->As_PowerUpGameObj())
		{
			if (stristr(Commands->Get_Preset_Name(obj),"crate"))
			{
				Attach_Script_Once(obj,"SSGM_Crate","");
			}
		}
	}
	virtual void OnLoadLevel()
	{
		if (mapConfiguration.EnableNewCrates)
		{
			int	Total = mapConfiguration.CrateUberWire + mapConfiguration.CrateHumanSilo + mapConfiguration.CrateAmmoRegen + mapConfiguration.CrateDeath + mapConfiguration.CrateVehicle + mapConfiguration.CrateWeapon + mapConfiguration.CrateCharacter + mapConfiguration.CrateMoney + mapConfiguration.CratePoints + mapConfiguration.CrateTiberium + mapConfiguration.CrateAmmo + mapConfiguration.CrateHealth + mapConfiguration.CrateArmor + mapConfiguration.CrateRefill + mapConfiguration.CrateButterFingers + mapConfiguration.CrateSpy + mapConfiguration.CrateThief + mapConfiguration.CrateBeacon + mapConfiguration.CrateStealth;
			if (Total != 100) {
				mapConfiguration.CrateWeapon = 10;
				mapConfiguration.CrateMoney = 10;
				mapConfiguration.CratePoints = 10;
				mapConfiguration.CrateVehicle = 8;
				mapConfiguration.CrateDeath = 8;
				mapConfiguration.CrateTiberium = 6;
				mapConfiguration.CrateAmmo = 5;
				mapConfiguration.CrateArmor = 5;
				mapConfiguration.CrateHealth = 5;
				mapConfiguration.CrateCharacter = 8;
				mapConfiguration.CrateButterFingers = 3;
				mapConfiguration.CrateSpy = 3;
				mapConfiguration.CrateStealth = 3;
				mapConfiguration.CrateRefill = 6;
				mapConfiguration.CrateBeacon = 4;
				mapConfiguration.CrateThief = 2;
				mapConfiguration.CrateAmmoRegen = 2;
				mapConfiguration.CrateHumanSilo = 1;
				mapConfiguration.CrateUberWire = 1;
				StringClass msg;
				msg.Format("Total crate percentages equal %d instead of 100. Using default percentages.",Total);
				SSGMGameLog::Log_Message(msg,"_ERROR");
			}
			CrateLastPickup = -181;
			CrateExists = false;
			CrateID = 0;
		}
	}
	virtual void OnGameOver()
	{
		CrateExists = false;
	}
};
Crate crate;
extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &crate;
}

char *RandomWeapon[21][3] = {
	{ "Auto	Rifle",	"POW_AutoRifle_Player",	"m00pwar_aqob0004i1evag_snd.wav" },
	{ "Shotgun", "POW_Shotgun_Player", "m00pwps_aqob0004i1evag_snd.wav"	},
	{ "Flamethrower", "POW_Flamethrower_Player", "m00pwft_aqob0001i1evag_snd.wav" },
	{ "Grenade Launcher", "POW_GrenadeLauncher_Player",	"m00pwgl_aqob0004i1evag_snd.wav" },
	{ "Repair Gun(Weak)", "POW_RepairGun_Player", "m00pwrp_aqob0001i1evag_snd.wav" },
	{ "Remote C4", "CnC_POW_MineRemote_02",	"m00pacp_aqob0004i1evag_snd.wav" },
	{ "Chain Gun", "POW_Chaingun_Player", "m00pwcg_aqob0004i1evag_snd.wav" },
	{ "Rocket Launcher(Weak)", "POW_RocketLauncher_Player",	"m00pwrl_aqob0004i1evag_snd.wav" },
	{ "Chemical	Sprayer", "POW_ChemSprayer_Player",	"m00pwcs_aqob0004i1evag_snd.wav" },
	{ "Tiberium	Auto Rifle", "POW_TiberiumAutoRifle_Player", "m00pwtr_aqob0004i1evag_snd.wav" },
	{ "Sniper Rifle", "POW_SniperRifle_Player",	"m00pwsr_aqob0004i1evag_snd.wav" },
	{ "Laser Chaingun",	"POW_LaserChaingun_Player",	"m00pwlc_aqob0004i1evag_snd.wav" },
	{ "Laser Rifle", "POW_LaserRifle_Player", "m00pwlr_aqob0004i1evag_snd.wav" },
	{ "Rocket Launcher(Strong)", "CnC_POW_RocketLauncher_Player", "m00pwrl_aqob0004i1evag_snd.wav" },
	{ "Tiberium	Flechette Gun",	"POW_TiberiumFlechetteGun_Player", "m00pwtf_aqob0004i1evag_snd.wav"	},
	{ "Personal	Ion	Cannon", "POW_PersonalIonCannon_Player", "m00pwpi_aqob0004i1evag_snd.wav" },
	{ "Railgun", "POW_Railgun_Player", "m00pwrg_aqob0004i1evag_snd.wav"	},
	{ "Ramjet Rifle", "POW_RamjetRifle_Player",	"m00pwrj_aqob0004i1evag_snd.wav" },
	{ "Volt	Auto Rifle", "POW_VoltAutoRifle_Player", "m00pwvr_aqob0004i1evag_snd.wav" },
	{ "Volt	Auto Rifle", "CnC_POW_VoltAutoRifle_Player_Nod", "m00pwvr_aqob0004i1evag_snd.wav" }
};

char *RandomVehicle[12][3] = {
	{ "GDI Humvee",	"humvee", "CnC_GDI_Humm-vee" },
	{ "Nod Buggy", "buggy", "CnC_Nod_Buggy" },
	{ "GDI APC", "gdiapc", "CnC_GDI_APC" },
	{ "Nod APC", "nodapc", "CnC_Nod_APC" },
	{ "GDI MRLS", "mrls", "CnC_GDI_MRLS" },
	{ "Nod Mobile Artillery", "arty", "CnC_Nod_Mobile_Artillery"},
	{ "GDI Medium Tank", "med", "CnC_GDI_Medium_Tank" },
	{ "Nod Light Tank",	"lighttank", "CnC_Nod_Light_Tank" },
	{ "Nod Flame Tank",	"flamer", "CnC_Nod_Flame_Tank" },
	{ "Nod Stealth Tank", "stank", "CnC_Nod_Stealth_Tank" },
	{ "GDI Mammoth Tank", "mammy", "CnC_GDI_Mammoth_Tank" },
	{ "Nod Recon Bike",	"recon", "CnC_Nod_Recon_Bike"}
};

char *RandomGDIChar[10][2] = {
	{ "Officer", "CnC_GDI_MiniGunner_1Off" },
	{ "Rocket Soldier",	"CnC_GDI_RocketSoldier_1Off" },
	{ "Sydney",	"CnC_Sydney" },
	{ "Deadeye", "CnC_GDI_MiniGunner_2SF" },
	{ "Gunner",	"CnC_GDI_RocketSoldier_2SF"	},
	{ "Patch", "CnC_GDI_Grenadier_2SF" },
	{ "Havoc", "CnC_GDI_MiniGunner_3Boss" },
	{ "Prototype Sydney", "CnC_Sydney_PowerSuit" },
	{ "Mobius",	"CnC_Ignatio_Mobius" },
	{ "Hotwire", "CnC_GDI_Engineer_2SF"	}
};

char *RandomNodChar[10][2] = {
	{ "Officer", "CnC_Nod_Minigunner_1Off" },
	{ "Rocket Soldier",	"CnC_Nod_RocketSoldier_1Off" },
	{ "Chem	Warrior", "CnC_Nod_FlameThrower_1Off" },
	{ "Blackhand Sniper", "CnC_Nod_Minigunner_2SF" },
	{ "Laser Chaingunner", "CnC_Nod_RocketSoldier_2SF" },
	{ "Stealth Black Hand",	"CnC_Nod_FlameThrower_2SF" },
	{ "Sakura",	"CnC_Nod_Minigunner_3Boss" },
	{ "Raveshaw", "CnC_Nod_RocketSoldier_3Boss"	},
	{ "Mendoza", "CnC_Nod_FlameThrower_3Boss" },
	{ "Technician",	"CnC_Nod_Technician_0" }
};

class SSGM_Crate : public ScriptImpClass {
private:
	int VehBlocker;
	bool PickedUp;
public:
	void Created(GameObject *obj)
	{
		Vector3	pos	= Commands->Get_Position(obj);
		SSGMGameLog::Log_Gamelog("CREATED;CRATE;%d;%s;%d;%d;%d;%d;%d;%d;%d", Commands->Get_ID(obj), Commands->Get_Preset_Name(obj), int(pos.Y),int(pos.X),int(pos.Z), int(Commands->Get_Facing(obj)), int(Commands->Get_Max_Health(obj)),int(Commands->Get_Max_Shield_Strength(obj)), int(Commands->Get_Player_Type(obj)));
		if (!crate.mapConfiguration.EnableCrates) {
			Commands->Destroy_Object(obj);
		}
		else if (crate.mapConfiguration.EnableNewCrates) {
			if (crate.CrateExists || (The_Game()->Get_Game_Duration_S() - crate.CrateLastPickup) < 180)
			{
				Commands->Destroy_Object(obj);
			}
			else {
				PickedUp = false;
				crate.CrateExists = true;
				Commands->Set_Model(obj,"vehcol2m");
				((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).Persistent = true;
				((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).AlwaysAllowGrant = true;
				((PowerUpGameObjDef &)(obj->As_PhysicalGameObj()->As_PowerUpGameObj()->Get_Definition())).GrantSoundID = 0;
			}
		}
		else {
			Destroy_Script();
		}	
	}
	void Custom(GameObject *obj,int type,int param,GameObject *sender)
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED && !PickedUp) {
			if (crate.mapConfiguration.VehCrate || !Get_Vehicle(sender)) {
				int Team = Get_Object_Type(sender);
				if (Team > 1 || Team < 0) {
					return;
				}

				PickedUp = true;
				crate.CrateLastPickup = The_Game()->Get_Game_Duration_S();
				crate.CrateExists = false;

				StringClass message1; //Console_Input
				StringClass message2; //FDSMessage
				StringClass message3; //WriteGamelog

				CrateStart:
				int RandomIntCrate = Commands->Get_Random_Int(1,101);
				int	percent	= 0;
				if ((RandomIntCrate	<= (percent+=crate.mapConfiguration.CrateWeapon)) && (crate.mapConfiguration.CrateWeapon > 0)) {
					Reselect:
					int	Rnd	= Commands->Get_Random_Int(1,101);
					int	Index = 0;
					if (Rnd	<= 10) Index = 0;
					else if	(Rnd<=20) Index	= 1;
					else if	(Rnd<=30) Index	= 2;
					else if	(Rnd<=40) Index	= 3;
					else if	(Rnd<=50) Index	= 4;
					else if	(Rnd<=60) Index	= 5;
					else if	(Rnd<=65) Index	= 6;
					else if	(Rnd<=70) Index	= 7;
					else if	(Rnd<=75) Index	= 8;
					else if	(Rnd<=80) Index	= 9;
					else if	(Rnd<=82) Index	= 10;
					else if	(Rnd<=84) Index	= 11;
					else if	(Rnd<=86) Index	= 12;
					else if	(Rnd<=88) Index	= 13;
					else if	(Rnd<=90) Index	= 14;
					else if	(Rnd<=92) Index	= 15;
					else if	(Rnd<=94) Index	= 16;
					else if	(Rnd<=96) Index	= 17;
					else if	(Rnd<=98) Index	= 18;
					else Index = 19;
					if (IsPresetDisabled(Get_Definition_ID(RandomWeapon[Index][1]))) goto Reselect;
					Commands->Give_PowerUp(sender,RandomWeapon[Index][1],true);
					if (RandomWeapon[Index][2][0] != '\0') {
						Create_2D_WAV_Sound_Player(sender,RandomWeapon[Index][2]);
					}
					message1.Format("ppage %d [Crate] You just got a %s from the Random Weapon crate.",Get_Player_ID(sender),RandomWeapon[Index][0]);
					message2.Format("%ls picked up a Random Weapon crate.",Get_Wide_Player_Name(sender));
					Vector3 pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;WEAPON;%s;%d;%s;%f;%f;%f;%f;%f;%f;%d",RandomWeapon[Index][0],Commands->Get_ID(sender),Commands->Get_Preset_Name(sender),pos.Y,pos.X,pos.Z,Commands->Get_Facing(sender),Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateMoney))	&& (crate.mapConfiguration.CrateMoney	> 0)) {
					int	Amount = Commands->Get_Random_Int(1,1000);
					int	RndGood	= Commands->Get_Random_Int(1,2);
					if (RndGood	== 1) {
						Commands->Give_Money(sender,(float)Amount,false);
						Create_2D_WAV_Sound_Player(sender,"m00pc$$_aqob0002i1evag_snd.wav");
					}
					else {
						Commands->Give_Money(sender,(float)(Amount*-1),false);
						if (Commands->Get_Money(sender)	< 0) Commands->Give_Money(sender,(Commands->Get_Money(sender) *	-1),false);
					}
					if (RndGood == 1) {
						message1.Format("ppage %d [Crate] You just got the Money Crate, you have gained %d credits.",Get_Player_ID(sender),(int)Amount);
						message2.Format("%ls picked up a Money crate.",Get_Wide_Player_Name(sender));
						Vector3 pos = Commands->Get_Position(obj);
						message3.Format("CRATE;MONEY;%d;%d;%s;%f;%f;%f;%f;%f;%f;%d", int(Amount),	Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z,	Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
					else {
						message1.Format("ppage %d [Crate] You just got the Demoney Crate, you have lost %d credits.",Get_Player_ID(sender),(int)Amount);
						message2.Format("%ls picked up a Demoney crate.",Get_Wide_Player_Name(sender));
						Vector3 pos = Commands->Get_Position(obj);
						message3.Format("CRATE;MONEY;%d;%d;%s;%f;%f;%f;%f;%f;%f;%d", int(Amount) * -1, Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}			
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CratePoints)) &&	(crate.mapConfiguration.CratePoints >	0)) {
					int	Amount = Commands->Get_Random_Int(1,500);
					int	RndGood	= Commands->Get_Random_Int(1,2);
					if (RndGood	== 1) {
						Commands->Give_Points(sender,(float)Amount,false);
					}
					else {
						Commands->Give_Points(sender,(float)(Amount*-1),false);
					}

					if (RndGood == 1) {
						message2.Format("%ls picked up a Points crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got the Points Crate, you have gained %d points.",Get_Player_ID(sender),(int)Amount);
						StringClass msg;
						const char *str = Get_Team_Name(Team);
						msg.Format("msg Crate: Enjoy those %d points from the crate god, %s.",(int)Amount,str);
						delete[] str;
						Console_Input(msg);
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;POINTS;%d;%d;%s;%f;%f;%f;%f;%f;%f;%d",int(Amount), Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
					else {
						message2.Format("%ls picked up a Depoints crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got the Depoints Crate, you lost %d points.",Get_Player_ID(sender),(int)Amount);
						StringClass msg;
						const char *str = Get_Team_Name(Team);
						msg.Format("msg Crate: Hope you won't miss those %d points, %s.",(int)Amount,str);
						delete[] str;
						Console_Input(msg);
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;POINTS;%d;%d;%s;%f;%f;%f;%f;%f;%f;%d",int(Amount)	* -1, Commands->Get_ID(sender),	Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender),	Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}		
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateVehicle)) && (crate.mapConfiguration.CrateVehicle	> 0)) {
					if (Get_Vehicle(sender) || crate.mapConfiguration.CrateVehSpawnPos[0].X == 0.0f || crate.mapConfiguration.CrateVehSpawnPos[0].Y == 0.0f || crate.mapConfiguration.CrateVehSpawnPos[0].Z == 0.0f || crate.mapConfiguration.CrateVehSpawnPos[1].X == 0.0f || crate.mapConfiguration.CrateVehSpawnPos[1].Y == 0.0f || crate.mapConfiguration.CrateVehSpawnPos[1].Z == 0.0f) {
						goto CrateStart;
					}
					Vector3	Pos = crate.mapConfiguration.CrateVehSpawnPos[Get_Object_Type(sender)];
					Vector3	Pos2 = crate.mapConfiguration.CrateVehInfSpawnPos[Get_Object_Type(sender)];
					Damage_All_Vehicles_Area(999999.0f,"Laser_NoBuilding",Pos,2.0f,obj,0);
					Commands->Set_Position(sender,Pos2);

					GameObject *Temp = Commands->Create_Object("Invisible_Object",Pos);
					Reselect2:
					int	Rnd	= Commands->Get_Random_Int(1,201);
					int	Index = 0;
					if (Rnd	<= 25) Index = 0;
					else if	(Rnd <=	50)	Index =	1;
					else if	(Rnd <=	70)	Index =	2;
					else if	(Rnd <=	90)	Index =	3;
					else if	(Rnd <=	110) Index = 4;
					else if	(Rnd <=	130) Index = 5;
					else if	(Rnd <=	145) Index = 6;
					else if	(Rnd <=	160) Index = 7;
					else if	(Rnd <=	170) Index = 8;
					else if	(Rnd <=	180) Index = 9;
					else if	(Rnd <=	190) Index = 10;
					else Index = 11;
					if (IsPresetDisabled(Get_Definition_ID(RandomVehicle[Index][2]))) goto Reselect2;		
					message2.Format("%ls picked up a Random Vehicle crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You got a %s from the Random Vehicle crate.",Get_Player_ID(sender), RandomVehicle[Index][0]);
					StringClass cine;
					cine.Format("%scrate.txt",Commands->Get_Player_Type(sender)	== 0?"Nod":"GDI");
					Commands->Attach_Script(Temp,"Test_Cinematic",cine);
					GameObject *veh = Commands->Create_Object(RandomVehicle[Index][2],Vector3(0,0,0));
					Commands->Send_Custom_Event(Temp,Temp,CUSTOM_EVENT_CINEMATIC_SET_FIRST_SLOT+4,Commands->Get_ID(veh),0);
					StringClass msg;
					const char *str = Get_Team_Name(Team);
					msg.Format("msg Crate: Looks like %s just got a random vehicle! Go them!",str);
					delete[] str;
					Console_Input(msg);
					Vector3 pos = Commands->Get_Position(sender);
					message3.Format("CRATE;VEHICLE;%s;%d;%s;%f;%f;%f;%f;%f;%f;%d", RandomVehicle[Index][0], Commands->Get_ID(sender),	Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender),	Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateDeath))	&& (crate.mapConfiguration.CrateDeath	> 0)) {
					message2.Format("%ls picked up a Death crate.",Get_Wide_Player_Name(sender));
					Commands->Apply_Damage(Get_Vehicle(sender),99999,"Shrapnel",0);
					Commands->Apply_Damage(sender,99999,"Shrapnel",0);
					Commands->Create_Explosion("Explosion_Mine_Proximity_01",Commands->Get_Position(obj),0);
					message1.Format("ppage %d [Crate] You just got a Death Crate, you have been killed. Sorry :(",Get_Player_ID(sender));
					StringClass msg;
					const char *str = Get_Team_Name(Team);
					msg.Format("msg Crate: Some poor %s guy got pwned by the fearsome death crate!!",str);
					delete[] str;
					Console_Input(msg);
					Vector3	pos;
					pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;DEATH;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateTiberium)) && (crate.mapConfiguration.CrateTiberium >	0)) {
					message2.Format("%ls picked up a Tiberium Death crate.",Get_Wide_Player_Name(sender));
					Commands->Apply_Damage(Get_Vehicle(sender),99999,"Shrapnel",0);
					Commands->Apply_Damage(sender,99999,"TiberiumRaw",false);
					Commands->Create_Object("CnC_Visceroid",Commands->Get_Position(sender));
					message1.Format("ppage %d [Crate] You have been killed by the tiberium death crate and your remains mutated into a visceroid.",Get_Player_ID(sender));
					Commands->Create_2D_WAV_Sound("m00evag_dsgn0027i1evag_snd.wav");
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;TIBERIUMDEATH;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateAmmo)) && (crate.mapConfiguration.CrateAmmo > 0)) {
					Commands->Give_PowerUp(sender,"CnC_POW_Ammo_ClipMax",false);
					Commands->Give_PowerUp(sender,"CnC_POW_Ammo_ClipMax",false);
					Commands->Give_PowerUp(sender,"CnC_POW_Ammo_ClipMax",false);
					Commands->Give_PowerUp(sender,"CnC_POW_Ammo_ClipMax",false);
					message2.Format("%ls picked up a Full Ammo crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You just got the Full Ammo Crate, your ammo has been refilled.",Get_Player_ID(sender));
					Create_2D_WAV_Sound_Player(sender,"m00puar_aqob0002i1evag_snd.wav");
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;FULLAMMO;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z,	Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateArmor)) && (crate.mapConfiguration.CrateArmor > 0)) {
					int	RndGood	= Commands->Get_Random_Int(1,3);
					if (Commands->Get_Shield_Strength(sender) <	2) RndGood = 1;
					if (RndGood	== 1) {
						Commands->Give_PowerUp(sender,"POW_Medal_Armor",true);
						message2.Format("%ls picked up an Armor crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got the Armor Crate, your max armor has been increased!",Get_Player_ID(sender));
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;ARMORINCREASE;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
					else {
						Commands->Set_Shield_Strength(sender,1);
						message2.Format("%ls picked up a Dearmor crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got the Dearmor Crate, your armor has been set to 1.",Get_Player_ID(sender));
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;ARMORLOST;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateHealth)) &&	(crate.mapConfiguration.CrateHealth >	0)) {
					int	RndGood	= Commands->Get_Random_Int(1,3);
					if (Commands->Get_Health(sender) ==	1) RndGood = 1;
					if (RndGood	== 1) {
						Commands->Give_PowerUp(sender,"POW_Medal_Health",true);
						message2.Format("%ls picked up a Health crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got the Health Crate, your max health has been increased!",Get_Player_ID(sender));
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;HEALTHUPGRADE;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
					else {
						Commands->Set_Health(sender,1);
						message2.Format("%ls picked up a Dehealth crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got the Dehealth Crate, your health has been set to 1.",Get_Player_ID(sender));
						Create_2D_WAV_Sound_Player(sender,"m00evag_dsgn0026i1evag_snd.wav");
						Vector3 pos = Commands->Get_Position(sender);
						message3.Format("CRATE;HEALTHREDUCE;%d;%s;%f;%f;%f;%f;%f;%f;%d",	Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z,	Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateCharacter))	&& (crate.mapConfiguration.CrateCharacter	> 0)) {
					if (Get_Vehicle(sender)) {
						goto CrateStart;
					}
					Reselect3:
					int	Rnd	= Commands->Get_Random_Int(1,101);
					int	Index = 0;
					if (Rnd	<= 20) Index = 0;
					else if	(Rnd <=	40)	Index =	1;
					else if	(Rnd <=	50)	Index =	2;
					else if	(Rnd <=	60)	Index =	3;
					else if	(Rnd <=	70)	Index =	4;
					else if	(Rnd <=	80)	Index =	5;
					else if	(Rnd <=	85)	Index =	6;
					else if	(Rnd <=	90)	Index =	7;
					else if	(Rnd <=	95)	Index =	8;
					else Index = 9;
					if (Team == 1 && IsPresetDisabled(Get_Definition_ID(RandomGDIChar[Index][1]))) {
						goto Reselect3;
					}
					else if (Team == 0 && IsPresetDisabled(Get_Definition_ID(RandomNodChar[Index][1]))) {
						goto Reselect3;
					}
					message2.Format("%ls picked up a Random Character crate.",Get_Wide_Player_Name(sender));
					if (Commands->Get_Player_Type(sender) == 1) {
						Change_Character(sender,RandomGDIChar[Index][1]);
						message1.Format("ppage %d [Crate] You have been transformed into a %s by the Random Character crate.",Get_Player_ID(sender),RandomGDIChar[Index][0]);
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;CHARACTER;%s;%d;%s;%f;%f;%f;%f;%f;%f;%d", RandomGDIChar[Index][0],	Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z,	Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
					else {
						Change_Character(sender,RandomNodChar[Index][1]);
						message1.Format("ppage %d [Crate] You have been transformed into a %s by the Random Character crate.",Get_Player_ID(sender),RandomNodChar[Index][0]);
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;CHARACTER;%s;%d;%s;%f;%f;%f;%f;%f;%f;%d", RandomNodChar[Index][0],	Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z,	Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateButterFingers))	&& (crate.mapConfiguration.CrateButterFingers	> 0)) {
					Commands->Clear_Weapons(sender);
					int	Rnd	= Commands->Get_Random_Int(1,3);
					if (Rnd	== 1) {
						Commands->Give_PowerUp(sender,"POW_Pistol_Player",false);
						Commands->Select_Weapon(sender,"Weapon_Pistol_Player");
					}
					else {
						Commands->Give_PowerUp(sender,"CnC_POW_MineTimed_Player_01",false);
						Commands->Select_Weapon(sender,"CnC_Weapon_MineTimed_Player");
					}
					message2.Format("%ls picked up the Butter Fingers crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You picked up the Butter Fingers Crate, you have dropped most of your weapons.",Get_Player_ID(sender));
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;BUTTERFINGER;%d;%s;%f;%f;%f;%f;%f;%f;%d",Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z,	Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateRefill)) &&	(crate.mapConfiguration.CrateRefill >	0)) {
					Grant_Refill(sender);
					message2.Format("%ls picked up the Refill crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You just got the Refill Crate, your health, armor, and ammo have all been refilled.",Get_Player_ID(sender));
					Create_2D_WAV_Sound_Player(sender,"m00puar_aqob0002i1evag_snd.wav");
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;REFILL;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender),	Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender),	Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateBeacon)) &&	(crate.mapConfiguration.CrateBeacon >	0)) {
					if (DisableBeacons) {
						goto CrateStart;
					}
					int	RndGood	= Commands->Get_Random_Int(1,3);
					if (RndGood	== 1) {
						message2.Format("%ls picked up a Beacon crate.",Get_Wide_Player_Name(sender));
						if (Commands->Get_Player_Type(sender) == 1) {
							TeamPurchaseSettingsDefClass *PT = TeamPurchaseSettingsDefClass::Get_Definition(TeamPurchaseSettingsDefClass::TEAM_GDI);
							Commands->Give_PowerUp(sender,Get_Definition_Name(PT->Get_Beacon_Definition()),true);
							message1.Format("ppage %d [Crate] You just got the Beacon Crate, you have been given an Ion Cannon Beacon.",Get_Player_ID(sender));
							Create_2D_Sound_Player(sender,"m00evag_dsgn0070i1evag_snd.wav");
						}
						else if	(Commands->Get_Player_Type(sender) == 0) {
							TeamPurchaseSettingsDefClass *PT = TeamPurchaseSettingsDefClass::Get_Definition(TeamPurchaseSettingsDefClass::TEAM_NOD);
							Commands->Give_PowerUp(sender,Get_Definition_Name(PT->Get_Beacon_Definition()),true);
							message1.Format("ppage %d [Crate] You just got the Beacon Crate, you have been given a Nuclear Strike Beacon.",Get_Player_ID(sender));
							Create_2D_Sound_Player(sender,"m00evan_dsgn0074i1evan_snd.wav");
						}
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;BEACON;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender),	Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender),	Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
					else {
						Commands->Create_Object("Beacon_Nuke_Strike_Anim_Post",Commands->Get_Position(obj));
						Commands->Create_Explosion("Explosion_NukeBeacon",Commands->Get_Position(obj),0);
						Damage_All_Objects_Area(2500.0f,"None",Commands->Get_Position(obj),15.0f,obj,0);
						message2.Format("%ls picked up a Nuclear Bomb crate.",Get_Wide_Player_Name(sender));
						message1.Format("ppage %d [Crate] You just got a Nuclear Bomb Crate, you have been killed. Sorry :(",Get_Player_ID(sender));
						StringClass msg;
						const char *str = Get_Team_Name(Team);
						msg.Format("msg Crate: Looks like a %s player just got blown to bits by a Nuclear Bomb.",str);
						delete[] str;
						Console_Input(msg);
						Vector3	pos	= Commands->Get_Position(sender);
						message3.Format("CRATE;BEACONDEATH;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender),	pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					}
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateSpy)) && (crate.mapConfiguration.CrateSpy > 0)) {
					Change_Character(sender,"CnC_Nod_FlameThrower_2SF");
					Commands->Set_Is_Visible(sender,false);
					const char *str3 = Get_Team_Name(Team);
					message2.Format("A %s player picked up a Spy crate.",str3);
					delete[] str3;
					message1.Format("ppage %d [Crate] You got the Spy Crate, base defenses will ignore you until you die, buy a new character or destroy a building.",Get_Player_ID(sender));
					StringClass msg;
					const char *str = Get_Team_Name(Team);
					const char *str2 = Get_Team_Name(PTTEAM(Team));
					msg.Format("msg Crate: Oh no! A %s player just got a spy crate, better watch your base %s!",str,str2);
					delete[] str;
					delete[] str2;
					Console_Input(msg);
					Vector3 pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;SPY;0;0;%f;%f;%f;%f;%f;%f;%d",pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateStealth)) && (crate.mapConfiguration.CrateStealth	> 0)) {
					Commands->Enable_Stealth(sender,true);
					message2.Format("%ls picked up a Stealth crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You got the stealth crate! You have been given a stealth suit.",Get_Player_ID(sender));
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;STEALTH;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender),	pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateThief))	&& (crate.mapConfiguration.CrateThief	> 0)) {
					Commands->Give_Money(sender,(Commands->Get_Money(sender) * -1),false);
					message2.Format("%ls picked up a Thief crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You got the Thief Crate, you have lost all of your credits.",Get_Player_ID(sender));
					Create_2D_Sound_Player(sender,"m00evag_dsgn0028i1evag_snd.wav");
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;THIEF;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateAmmoRegen))	&& (crate.mapConfiguration.CrateAmmoRegen	> 0)) {
					Commands->Send_Custom_Event(obj,sender,100100,1,1);//enable regeneration of ammo on player
					message2.Format("%ls picked up a Ammo Regeneration crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You got the Ammo Reneration Crate, your ammo will be refilled every minute.",Get_Player_ID(sender));
					Create_2D_Sound_Player(sender,"m00puar_aqob0002i1evag_snd.wav");//ammo regen sound
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;AMMOREGEN;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateHumanSilo))	&& (crate.mapConfiguration.CrateHumanSilo	> 0)) {
					int team = Commands->Get_Player_Type(sender);
					Commands->Send_Custom_Event(obj,sender,100101,1,1);//enable generating of money on player
					message2.Format("%ls picked up a Human Silo crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You got the Human Silo Crate, your entire team will be granted an additional $1 credit a second until you die or change character.",Get_Player_ID(sender));
					Create_2D_WAV_Sound_Team("m00pc$$_aqob0002i1evag_snd.wav",team);//credits acquired
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;HUMANSILO;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
				}
				else if	((RandomIntCrate <=	(percent+=crate.mapConfiguration.CrateUberWire))	&& (crate.mapConfiguration.CrateUberWire	> 0)) {
					const char *str = Get_Player_Name(sender);
					char ubermsg[250]; sprintf(ubermsg,"[CRATE] %s got the UberWire crate! Beware!",str); Send_Message(255,255,255,ubermsg);
					delete[] str;
					message2.Format("%ls picked up a UberWire crate.",Get_Wide_Player_Name(sender));
					message1.Format("ppage %d [Crate] You got the UberWire crate.",Get_Player_ID(sender));
					Vector3	pos	= Commands->Get_Position(sender);
					message3.Format("CRATE;UBERWIRE;%d;%s;%f;%f;%f;%f;%f;%f;%d", Commands->Get_ID(sender), Commands->Get_Preset_Name(sender), pos.Y,pos.X,pos.Z, Commands->Get_Facing(sender), Commands->Get_Max_Health(sender),Commands->Get_Max_Shield_Strength(sender),Get_Object_Type(sender));
					Change_Character(sender,"GDI_Engineer_2SF");
				}
				else {
					goto CrateStart;
				}
				Console_Input(message1);
				SSGMGameLog::Log_Message(message2,"_CRATE");
				SSGMGameLog::Log_Gamelog(message3);
				Commands->Destroy_Object(obj);
			}
		}
	}
	void Destroyed(GameObject *obj)
	{
		Vector3	pos	= Commands->Get_Position(obj);
		SSGMGameLog::Log_Gamelog("DESTROYED;CRATE;%d;%s;%d;%d;%d",	Commands->Get_ID(obj), Commands->Get_Preset_Name(obj), int(pos.Y), int(pos.X), int(pos.Z));
	}
	void Killed(GameObject *obj,GameObject *killer)
	{
		Vector3	victimpos =	Commands->Get_Position(obj);
		Vector3	damagerpos = Commands->Get_Position(killer);
		const char *str = Get_Translated_Preset_Name_Ex(obj);
		const char *str2 = Get_Translated_Preset_Name_Ex(killer);
		SSGMGameLog::Log_Gamelog("KILLED;CRATE;%d;%s;%d;%d;%d;%d;%d;%s;%f;%f;%f;%f;%s;%s;%s",Commands->Get_ID(obj), Commands->Get_Preset_Name(obj), int(victimpos.Y), int(victimpos.X), int(victimpos.Z), int(Commands->Get_Facing(obj)), Commands->Get_ID(killer),	Commands->Get_Preset_Name(killer),	int(damagerpos.Y), int(damagerpos.X), int(damagerpos.Z), int(Commands->Get_Facing(killer)),Get_Current_Weapon(killer),str,str2);
		delete[] str;
		delete[] str2;
	}
	void Damaged(GameObject *obj,GameObject *damager,float amount)
	{
		if (amount != 0)
		{
			Vector3	victimpos =	Commands->Get_Position(obj);
			Vector3	damagerpos = Commands->Get_Position(damager);
			SSGMGameLog::Log_Gamelog("DAMAGED;CRATE;%d;%s;%d;%d;%d;%d;%d;%s;%d;%d;%d;%d;%f;%d;%d",	Commands->Get_ID(obj), Commands->Get_Preset_Name(obj), int(victimpos.Y), int(victimpos.X), int(victimpos.Z), int(Commands->Get_Facing(obj)), Commands->Get_ID(damager),	Commands->Get_Preset_Name(damager),	int(damagerpos.Y), int(damagerpos.X), int(damagerpos.Z),int(Commands->Get_Facing(damager)),	amount,	int(Commands->Get_Health(obj)),	int(Commands->Get_Shield_Strength(obj)));
		}
	}
};

class SSGM_Prevent_Destruction_Until_Entered : public ScriptImpClass {
	void Created(GameObject *obj);
	void Damaged(GameObject *obj, GameObject *damager, float damage);
	void Custom(GameObject *obj, int message, int param, GameObject *sender);
	void Timer_Expired(GameObject *obj, int number);
};

void SSGM_Prevent_Destruction_Until_Entered::Created(GameObject *obj) {
	Commands->Set_Player_Type(obj,-2);
	Commands->Start_Timer(obj,this,30,1);
}

void SSGM_Prevent_Destruction_Until_Entered::Damaged(GameObject *obj, GameObject *damager, float damage) {
	Commands->Set_Health(obj,Commands->Get_Max_Health(obj));
	Commands->Set_Shield_Strength(obj,Commands->Get_Max_Shield_Strength(obj));
}

void SSGM_Prevent_Destruction_Until_Entered::Custom(GameObject *obj, int message, int param, GameObject *sender) {
	if (message == CUSTOM_EVENT_VEHICLE_ENTERED) {
		Destroy_Script();
	}
}

void SSGM_Prevent_Destruction_Until_Entered::Timer_Expired(GameObject *obj, int number) {
	if (number == 1) {
		Destroy_Script();
	}
}

ScriptRegistrant<SSGM_Prevent_Destruction_Until_Entered> SSGM_Prevent_Destruction_Until_Entered_Registrant("SSGM_Prevent_Destruction_Until_Entered","");
ScriptRegistrant<SSGM_Crate> SSGM_Crate_Registrant("SSGM_Crate","");


class SSGM_M00_CnC_Crate :
	public ScriptImpClass
{
	void SSGM_M00_CnC_Crate::Custom(GameObject* obj, int type, int param, GameObject* sender)
	{
		if (type == CUSTOM_EVENT_POWERUP_GRANTED && sender && !crate.mapConfiguration.EnableNewCrates)
		{
			Commands->Give_Money(sender, 100, false);
		}
	}
};
ScriptRegistrant<SSGM_M00_CnC_Crate> M00_CnC_Crate_Registrant("M00_CnC_Crate","");
