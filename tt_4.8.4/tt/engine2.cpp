/*	Renegade tt.dll
	cross platform tt.dll specific engine calls
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "General.h"
#include "engine2.h"

#include "tt.h"
#include "shaders.h"
#include "render2d.h"

//#include "engine_ttdef.h"
#include "cCsTextObj.h"
#include "SoldierGameObj.h"
#include "RenderObjClass.h"
#include "SoldierGameObjdef.h"
#include "cScTextObj.h"
#include "PhysicsSceneClass.h"
#include "FileFactoryListClass.h"
#include "HashMixFileFactoryClass.h"
#include "cGameSpyAdmin.h"
#include "cRemoteHost.h"
#include "cNetwork.h"
#include "VehicleFactoryGameObj.h"
#include "TextDisplayGameModeClass.h"
#include "MessageWindowClass.h"
#include "cConnection.h"
#include "TexProjectClass.h"
#include "CombatManager.h"
#include "cGameType.h"
#include "ModPackageClass.h"
#include "ConsoleModeClass.h"
#include "SimpleFileFactoryClass.h"
#include "FileHashEvent.h"
#include "cPurchaseRequestEvent.h"
#include "cPurchaseResponseEvent.h"
#include "WWOnline.h"
#include "cGameOptionsEvent.h"
#include "GameObjManager.h"
#include "PlayerDataClass.h"
#include "Engine_Obj.h"
#include "Engine_Obj2.h"
#include "Engine_Player.h"
#include "Engine_Def.h"
#include "cPlayer.h"
#include "Engine_Game.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "TeamPurchaseSettingsDefClass.h"
#include "PurchaseSettingsDefClass.h"
#include "engine_pt.h"
#include "PowerupGameObjDef.h"
#include "WeaponViewClass.h"

REF_DEF2(GameInFocus, bool, 0x0083DBC0, 0x0083CDA8);
REF_DEF2(_HiddenFrameCount, uint, 0x0081E870, 0x0081DA58);
REF_DEF2(RenegadeFileFactory, FileFactoryListClass, 0x0081ED28, 0x0081DF10);
REF_DEF2(RenegadeBaseFileFactory, SimpleFileFactoryClass, 0x0081ED60, 0x0081DF48);
REF_ARR_DEF2(GamespyNickname,char,700,0x0082D0B0,0x0082C298);
REF_DEF2(NetNickname,WideStringClass,0x0081F360,0x0081E548);
sockaddr_in rlmonaddr;
extern REF_DECL2(MessageWindow,MessageWindowClass *);
char *gb = "GUNBONE";
bool LevelRunning = false;
HashMixFileFactoryClass* _mapFileFactory = 0;
bool VehicleBuildingDisable;
bool NoVehicleFlipKill = false;
unsigned int UnsquishableArmor;
unsigned int UnsquishableArmor2;
unsigned int UnsquishableArmor3;
unsigned int UnsquishableArmor4;
bool Unsquishable;
extern bool NeutralVechiclePointsFix;
ScriptCommands *Commands;
ChatHook Chat_Hook;
HostHook Host_Hook;
PlayerJoin Player_Join_Hook;
PlayerLeave Player_Leave_Hook;
bool SendRadarData = false;
char *ssurl = 0;
float MapScale = 0;
char *MapTexture;
Vector2 MapOffset;
bool CurrentlyBuilding;
bool CurrentlyBuildingTeam[2];
SimpleDynVecClass<PurchaseHook> Vehicle_Purchase_Hooks;
SimpleDynVecClass<PurchaseHook> Powerup_Purchase_Hooks;
SimpleDynVecClass<PurchaseHook> Character_Purchase_Hooks;
SimpleDynVecClass<PurchaseMonHook> Vehicle_Purchase_Monitor_Hooks;
SimpleDynVecClass<PurchaseMonHook> Powerup_Purchase_Monitor_Hooks;
SimpleDynVecClass<PurchaseMonHook> Character_Purchase_Monitor_Hooks;
SimpleDynVecClass<const char *> Vehicle_Purchase_Hooks_Data;
SimpleDynVecClass<const char *> Powerup_Purchase_Hooks_Data;
SimpleDynVecClass<const char *> Character_Purchase_Hooks_Data;
SimpleDynVecClass<const char *> Vehicle_Purchase_Monitor_Hooks_Data;
SimpleDynVecClass<const char *> Powerup_Purchase_Monitor_Hooks_Data;
SimpleDynVecClass<const char *> Character_Purchase_Monitor_Hooks_Data;
DataHook Data_Hook;
TTVersion Version_Hook;
bool PTDataChanged;
int WireframeMode;
LoadLevelHook Load_Level_Hook;
LoadLevelHook Think_Hook;
bool BuildingHarvester[2];
LoadLevelHook Game_Over_Hook;
bool VehicleOwnershipDisable;
bool DisableCostMultiplier;
float BuildTimeDelay;
char *mapprefix;
ConsoleOutputHook Console_Output_Hook;
GameObject *ExplosionObj;
int ScriptsLastTeamTime = -1;
extern bool DisableCostMultiplier;

bool vehicleExitReloadContinued = true;

void GrantSupplies(GameObject *obj);
void SetCurrentlyBuilding(bool building,int team)
{
	CurrentlyBuildingTeam[team] = building;
	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject *o = As_SmartGameObj(x->Data());
		if (o)
		{
			if ((Get_Object_Type(o) == team) || (team == 2))
			{
				char buf[512];
				sprintf(buf,"j\n%d\n%d\n",SS_CURRENTLYBUILDING,building);
				SendTextSc(buf,PrivateMessage,0,-2,Get_Player_ID(o));
				if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == Get_Player_ID(o)))
				{
					CurrentlyBuilding = building;
				}
			}
		}
		x = x->Next();
	}
}

void Do_Poke(GameObject *poked,GameObject *poker)
{
	SimpleDynVecClass<GameObjObserverClass *> *observers = &((ScriptableGameObj *)poked)->Get_Observers();
	int x = observers->Count();
	for (int i = 0;i < x;i++)
	{
		((*observers)[i])->Poked(poked,poker);
	}
}

const wchar_t *GetNickname()
{
	if (cGameSpyAdmin::Is_Gamespy_Game())
	{
		return CharToWideChar(GamespyNickname);
	}
	else
	{
		return newwcs(NetNickname.Peek_Buffer());
	}
}

const char *GetNickname2()
{
	if (cGameSpyAdmin::Is_Gamespy_Game())
	{
		return newstr(GamespyNickname);
	}
	else
	{
		return WideCharToChar(NetNickname.Peek_Buffer());
	}
}


bool Do_Recieve_Data_Cs(cCsTextObj *Message2)
{
	bool send = false;
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return send;
	}
	if (Message2->receiverId == -2)
	{
		char *str = (char *)WideCharToChar(Message2->message);
		char *id = strtok(str,"\n");
		if (id[0] == 'j')
		{
			int pokedid = atoi(strtok(NULL,"\n"));
			int pokerid = atoi(strtok(NULL,"\n"));
			GameObject *poked = Commands->Find_Object(pokedid);
			GameObject *poker = Commands->Find_Object(pokerid);
			if ((poked) && (poker))
			{
				Do_Poke(poked,poker);
			}
		}
		delete[] str;
	}
	else if (Message2->receiverId == -3)
	{
		char *str = (char *)WideCharToChar(Message2->message);
		char *id = strtok(str,"\n");
		switch (id[0])
		{
			case 'j':
			{
				int playerId = atoi(strtok(NULL,"\n"));
				float version = (float)atof(strtok(NULL,"\n"));

				cRemoteHost* remoteHost = cNetwork::Get_Server_Rhost(playerId);
				TT_ASSERT(remoteHost);

				if (remoteHost)
				{
					TT_ASSERT(remoteHost->getVersion() == 0 || remoteHost->getVersion() == version && version < 4.0f); // Clients should not change version numbers.
					remoteHost->setVersion(version);
					
					if (Version_Hook)
						Version_Hook(playerId, version);
				}

				break;
			}
			case 'm':
			{
				int PlayerID = atoi(strtok(NULL,"\n"));
				char *map = strtok(NULL,"\n");
				Console_Output("Player %d has map %s\n",PlayerID,map);
				break;
			}
			case 'n':
			{
				int PlayerID = atoi(strtok(NULL,"\n"));
				char *map = strtok(NULL,"\n");
				Console_Output("Player %d does not have map %s\n",PlayerID,map);
				break;
			}
			case 'k':
			{
				char *key = strtok(NULL,"\n");
				Key_Is_Pressed(key,Message2->senderId);
				break;
			}
			case 'd':
			{
				int PlayerID = atoi(strtok(NULL,"\n"));
				unsigned int count = atoi(strtok(NULL,"\n"));
				unsigned int *data = new unsigned int[count];
				for (unsigned int i = 0;i < count;i++)
				{
					data[i] = atoi(strtok(NULL,"\n"));
				}
				if (Data_Hook)
				{
					Data_Hook(PlayerID,count,data);
				}
				break;
			}
			case 'f':
			{
				int ID = atoi(strtok(NULL,"\n"));
				int seat = atoi(strtok(NULL,"\n"));
				Force_Occupant_Exit(Commands->Find_Object(ID),seat);
				break;
			}
			case 'g':
			{
				int ID = atoi(strtok(NULL,"\n"));
				int seat = atoi(strtok(NULL,"\n"));
				Set_Vehicle_Gunner(Commands->Find_Object(ID),seat);
				break;
			}
			case 's':
			{
				int ID = atoi(strtok(NULL,"\n"));
				int notify = atoi(strtok(NULL,"\n"));
				ShaderNotifyDo(ID,notify,Message2->senderId);
				break;
			}

			case 'a': // Screenshot feedback message
			{
				int feedback = atoi(strtok(NULL, "\n"));
				switch (feedback)
				{
					case 0:
						Console_Output("The screenshot was made and is now being uploaded.\n");
						break;

					case 1:
						Console_Output("The screenshot has finished uploading.\n");
						break;

					case 2:
						Console_Output("The screenshot could not be made because the Renegade window\nis currently out of focus. The screenshot will be made as soon\nas Renegade regains focus.\n");
						break;

					case 3:
						Console_Output("The screenshot upload failed.\n");
						break;
				}
				break;
			}
		}
		delete[] str;
	}
	else
	{
		char temp[64];
		memset(temp,0,64);
		char buf[100];
		memset(buf,0,100);
		time_t t = time(0);
		tm *local = localtime(&t);
		local->tm_mon += 1;
		sprintf(buf,"bhs_renlog_%d-%d-%02d.txt",local->tm_mon,local->tm_mday,local->tm_year);
		FILE *f = fopen(buf,"at");
		if (f)
		{
			GetTimeFormat(LANG_SYSTEM_DEFAULT,TIME_FORCE24HOURFORMAT,0,"'['HH':'mm':'ss'] '",temp,0x40);
			send = true;
			if (Chat_Hook)
			{
				send = Chat_Hook(Message2->senderId,Message2->type,Message2->message,Message2->receiverId);
			}
			const char *name = Get_Player_Name_By_ID(Message2->senderId);
			if (!name)
			{
				name = newstr("(null)");
			}
			if (!Message2->type)
				fprintf(f,"%s%s: %S\n",temp,name,Message2->message.Peek_Buffer());
			if (Message2->type == 1)
				fprintf(f,"%s[Team] %s: %S\n",temp,name,Message2->message.Peek_Buffer());
			delete[] name;
			fclose(f);
		}
	}
	return send;
}

void Do_Soldier_Update(SoldierGameObj *obj)
{
	SoldierGameObj *o = obj->As_SoldierGameObj();
	if (!o)
	{
		return;
	}
	if (o->Get_Weapon_Render_Model())
	{
		o->Peek_Physical_Object()->Peek_Model()->Add_Sub_Object_To_Bone(o->Get_Weapon_Render_Model(),gb);
	}
	SoldierGameObjDef *d = (SoldierGameObjDef *)Get_Definition((GameObject *)o);
	o->Adjust_Skeleton(d->Get_Skeleton_Height(),d->Get_Skeleton_Width());
	o->Update_Back_Gun();
	if ((o->Get_State() == HumanStateClass::IN_VEHICLE) && (o->Peek_Physical_Object()->Peek_Model()))
	{
		o->Peek_Physical_Object()->Peek_Model()->Set_Hidden(true);
	}
	o->Prepare_Speech_Framework();
}

WideStringClass *CharToWSC(const char *string)
{
	WideStringClass *wsc = new WideStringClass;
	wsc->Convert_From(string);
	return wsc;
}

WideStringClass *CharToWSC2(const char *string)
{
	unsigned int len = strlen(string) + 2;
	if (len % 2)
	{
		len++;
	}
	WideStringClass *wsc = new WideStringClass((int)len/2,false);
	wchar_t *buf = wsc->Get_Buffer(len/2);
	memset(buf,0,len);
	memcpy(buf,string,len-2);
	return wsc;
}

void SendTextSc(const char *string,TextMessageEnum mtype,bool amsg,int sender,int target)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	cScTextObj *textobj = new cScTextObj;
	WideStringClass *wcs = CharToWSC(string);
	textobj->Init(*wcs,mtype,amsg,sender,target);
	delete wcs;
}

void SendTextSc2(const char *string,TextMessageEnum mtype,bool amsg,int sender,int target)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	cScTextObj *textobj = new cScTextObj;
	WideStringClass *wcs = CharToWSC2(string);
	textobj->Init(*wcs,mtype,amsg,sender,target);
	delete wcs;
}

char *Addr_As_String(sockaddr_in *addr2)
{
	char *temp;
	temp = new char[40];
	int ip1 = addr2->sin_addr.s_addr&0x000000FF;
	int ip2 = (addr2->sin_addr.s_addr&0x0000FF00)>>8;
	int ip3 = (addr2->sin_addr.s_addr&0x00FF0000)>>16;
	int ip4 = (addr2->sin_addr.s_addr&0xFF000000)>>24;
	int port = ntohs(addr2->sin_port);
	sprintf(temp,"%d.%d.%d.%d;%d",ip1,ip2,ip3,ip4,port);
	return temp;
}

void Set_Bandwidth(int PlayerID,int bandwidth)
{
	cRemoteHost* clientConnection = cNetwork::Get_Server_Rhost(PlayerID);
	if(clientConnection)
	{
		clientConnection->setMaxBandwidth(bandwidth);
	}
}

extern REF_DECL2(VehicleCount,int);
void DoPlayerJoin2(cPlayer *playerdata)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
		return;
	
	int PlayerID = playerdata->PlayerId;
	const char *Name = WideCharToChar(playerdata->PlayerName);
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_VEHICLELIMIT,VehicleCount);
	SendTextSc(buf,PrivateMessage,0,-2,PlayerID);
	sprintf(buf,"j\n%d\n%d\n",SS_WIREFRAME,WireframeMode);
	SendTextSc(buf,PrivateMessage,0,-2,PlayerID);
	sprintf(buf,"j\n%d\n%f\n",SS_SERVERVERSION,(float)TT_VERSION);
	SendTextSc(buf,PrivateMessage,0,-2,PlayerID);

	// BACKCOMPAT: For very old scripts.dll clients only.
	{
		cRemoteHost* remoteHost = cNetwork::Get_Server_Rhost(PlayerID);
		TT_ASSERT(remoteHost);
		if (remoteHost->getVersion() == 0)
		{
			sprintf(buf, "j\n%d\n%d\n", SS_VERSION, PlayerID);
			SendTextSc(buf, PrivateMessage, false, -2, PlayerID);
		}
	}

	if (PTDataChanged)
		Send_PT_Data_Player(PlayerID);
	
	sprintf(buf,"j\n%d\n%d\n",SS_CURRENTLYBUILDING,CurrentlyBuildingTeam[Get_Team(PlayerID)]);
	SendTextSc(buf,PrivateMessage,0,-2,PlayerID);
	if (SendRadarData)
	{
		sprintf(buf,"j\n%d\n%f\n%f\n%f\n%s\n",SS_RADARMAP,MapScale,MapOffset.X,MapOffset.Y,MapTexture);
		SendTextSc(buf,PrivateMessage,0,-2,PlayerID);
	}

	if (Player_Join_Hook)
	{
		Player_Join_Hook(PlayerID,Name);
	}
	delete[] Name;

	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject* obj = x->Data();
		SmartGameObj* o = (SmartGameObj*)As_SmartGameObj(obj);
		if(o)
		{
			if(o->Is_Stealth_Enabled() != o->Get_Definition().Is_Stealthed())
			{
				sprintf(buf,"j\n%d\n%d\n%d\n",SS_STEALTH,Commands->Get_ID(obj),o->Is_Stealth_Enabled());
				SendTextSc(buf,PublicMessage,0,-2,-1);
			}
		}
		x = x->Next();
	}
}

extern bool RenderHud;
void DoLoadLevel2()
{
	GDISoldierName = newstr("CnC_GDI_MiniGunner_0");
	NodSoldierName = newstr("CnC_Nod_Minigunner_0");
	LevelRunning = true;
	CurrentlyBuildingTeam[0] = false;
	CurrentlyBuildingTeam[1] = false;
	CurrentlyBuilding = false;
	BuildingHarvester[0] = false;
	BuildingHarvester[1] = false;
	Enable_Base_Radar(0,true);
	Enable_Base_Radar(1,true);
	SendRadarData = false;
	PTDataChanged = false;
	if (!Exe)
	{
		RenderHud = true;	
		Commands->Enable_HUD(RenderHud);
		MapLoaded();
	}
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	if (Load_Level_Hook)
	{
		Load_Level_Hook();
	}
}

class WWPhys
{
public:
	static void Shutdown();
};

RENEGADE_FUNCTION
void WWPhys::Shutdown()
AT2(0x0062E650,0x0062DEF0);

RENEGADE_FUNCTION
void WeaponViewClass::Shutdown()
AT2(0x0070E1D0,0x0070D790);

void DoUnloadLevel()
{
	delete[] GDISoldierName;
	delete[] NodSoldierName;
	WWPhys::Shutdown();
	WeaponViewClass::Shutdown();
	LevelRunning = false;
	if (_mapFileFactory)
	{
		RenegadeFileFactory.Remove_FileFactory(_mapFileFactory);
		delete _mapFileFactory;
		_mapFileFactory = 0;
	}
}

void DoGameOver2()
{
	if (Game_Over_Hook)
	{
		Game_Over_Hook();
	}
}

void AddCombatMessage(const char *message, unsigned int red, unsigned int green, unsigned int blue)
{
	Vector3 color;
	color.X = (float)(red/255.0);
	color.Y = (float)(green/255.0);
	color.Z = (float)(blue/255.0);
	WideStringClass *wcs = CharToWSC(message);
	MessageWindowClass *messagewindow = MessageWindow;
	messagewindow->Add_Message(*wcs,color,0,0);
	delete wcs;
}
extern REF_DECL2(NodHouseColor, Vector3);
extern REF_DECL2(GDIHouseColor, Vector3);
extern REF_DECL2(PrivateMessageColor, Vector3);
extern REF_DECL2(PublicMessageColor, Vector3);

//Vector3 &NodHouseColor;
//Vector3 &GDIHouseColor;
//Vector3 &PrivateMessageColor;
//Vector3 &PublicMessageColor;
extern bool NukeWeatherDisable;
extern bool IonWeatherDisable;
REF_DEF2(DrawDistance, float, 0x007FAE3C, 0x007F9FCC);
void HUDIniRead()
{
	mapprefix = new char[256];
	strcpy(mapprefix,"C&C_");
	FileClass *f = Get_Data_File("tt.ini");
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckLoad(data,size,"tt.ini",HashTTIni);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	INIClass *ttini = Get_INI("tt.ini");
	INIClass *hudini = Get_INI("hud.ini");
	if (ttini)
	{
        vehicleExitReloadContinued = ttini->Get_Bool("General","ContinueReloadOnVehicleExit", true);
		NodHouseColor.X = (float)(ttini->Get_Int("General","NodHouseRed",255)/255.0);
		NodHouseColor.Y = (float)(ttini->Get_Int("General","NodHouseGreen",0)/255.0);
		NodHouseColor.Z = (float)(ttini->Get_Int("General","NodHouseBlue",0)/255.0);
		GDIHouseColor.X = (float)(ttini->Get_Int("General","GDIHouseRed",255)/255.0);
		GDIHouseColor.Y = (float)(ttini->Get_Int("General","GDIHouseGreen",204)/255.0);
		GDIHouseColor.Z = (float)(ttini->Get_Int("General","GDIHouseBlue",0)/255.0);
		PrivateMessageColor.X = (float)(ttini->Get_Int("General","PrivateMessageRed",0)/255.0);
		PrivateMessageColor.Y = (float)(ttini->Get_Int("General","PrivateMessageGreen",0)/255.0);
		PrivateMessageColor.Z = (float)(ttini->Get_Int("General","PrivateMessageBlue",255)/255.0);
		PublicMessageColor.X = (float)(ttini->Get_Int("General","PublicMessageRed",255)/255.0);
		PublicMessageColor.Y = (float)(ttini->Get_Int("General","PublicMessageGreen",255)/255.0);
		PublicMessageColor.Z = (float)(ttini->Get_Int("General","PublicMessageBlue",255)/255.0);
		DisableCostMultiplier = ttini->Get_Bool("General","DisableCostMultiplier",false);
		BuildTimeDelay = ttini->Get_Float("General","BuildTimeDelay",2.0);
		VehicleOwnershipDisable = ttini->Get_Bool("General","VehicleOwnershipDisable",false);
		VehicleBuildingDisable = ttini->Get_Bool("General","VehicleBuildingDisable",false);
		NoVehicleFlipKill = ttini->Get_Bool("General","DisableVehicleFlipKill",false);
		Unsquishable = ttini->Get_Bool("General","Unsquishable",false);
		ttini->Get_String("General","MapPrefix","C&C_",mapprefix,256);
		NeutralVechiclePointsFix = ttini->Get_Bool("General","NeutralVechiclePointsFix",true);
		DrawDistance = ttini->Get_Float("General","DrawDistance",300);
		ScriptsLastTeamTime = ttini->Get_Int("General","ScriptsLastTeamTime",-1);
		NukeWeatherDisable = ttini->Get_Bool("General","NukeWeatherDisable",false);
		IonWeatherDisable = ttini->Get_Bool("General","IonWeatherDisable",false);
		if (Unsquishable)
		{
			UnsquishableArmor = ttini->Get_Int("General","UnsquishableArmor",0);
			UnsquishableArmor2 = ttini->Get_Int("General","UnsquishableArmor2",0);
			UnsquishableArmor3 = ttini->Get_Int("General","UnsquishableArmor3",0);
			UnsquishableArmor4 = ttini->Get_Int("General","UnsquishableArmor4",0);
		}
		if (!Exe)
		{
			ReadHUDIniBits(ttini);
		}
		Release_INI(ttini);
	}
	if (hudini)
	{
		if (!Exe)
		{
			ReadHUDIniBits2(hudini);
			ReadHUDBits(hudini);
		}
		Release_INI(hudini);
	}
	if (!Exe)
	{
		HUDInit();
	}
}


void Remove_Weap(GameObject *obj,const char *weapon)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return;
	}
	GameObject *o = As_ArmedGameObj(obj);
	if (!o)
	{
		return;
	}
	WeaponBagClass *w = ((ArmedGameObj *)o)->Get_Weapon_Bag();
	int x = w->Get_Count();
	for (int i = 0;i < x;i++)
	{
		WeaponClass *wc = w->Peek_Weapon(i);
		if (wc)
		{
			if (!_stricmp(wc->Get_Name(),weapon))
			{
				w->Remove_Weapon(i);
			}
		}
	}
}

void Send_Team_Purchase_Definition(TeamPurchaseSettingsDefClass *d,int PlayerID)
{
	char buf[2000];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%s\n%s\n%s\n%s\n%d\n%d\n%d\n%s\n%d\n%s\n",SS_TEAMPURCHASE,d->team,d->presetids[0],d->presetids[1],d->presetids[2],d->presetids[3],d->stringids[0],d->stringids[1],d->stringids[2],d->stringids[3],d->textures[0].Peek_Buffer(),d->textures[1].Peek_Buffer(),d->textures[2].Peek_Buffer(),d->textures[3].Peek_Buffer(),d->beaconcost,d->beaconpresetid,d->beaconstringid,d->beacontexture.Peek_Buffer(),d->refillstringid,d->refilltexture.Peek_Buffer());
	if (PlayerID == -1)
	{
		SendTextSc2(buf,PublicMessage,true,-3,-1);
	}
	else
	{
		SendTextSc2(buf,PrivateMessage,true,-3,PlayerID);
	}
}

void Send_Purchase_Definition(PurchaseSettingsDefClass *d,int PlayerID)
{
	char buf[2000];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",SS_PURCHASE1,d->team,d->type,d->costs[0],d->costs[1],d->costs[2],d->costs[3],d->costs[4],d->costs[5],d->costs[6],d->costs[7],d->costs[8],d->costs[9],d->presetids[0],d->presetids[1],d->presetids[2],d->presetids[3],d->presetids[4],d->presetids[5],d->presetids[6],d->presetids[7],d->presetids[8],d->presetids[9],d->stringids[0],d->stringids[1],d->stringids[2],d->stringids[3],d->stringids[4],d->stringids[5],d->stringids[6],d->stringids[7],d->stringids[8],d->stringids[9],d->textures[0].Peek_Buffer(),d->textures[1].Peek_Buffer(),d->textures[2].Peek_Buffer(),d->textures[3].Peek_Buffer(),d->textures[4].Peek_Buffer(),d->textures[5].Peek_Buffer(),d->textures[6].Peek_Buffer(),d->textures[7].Peek_Buffer(),d->textures[8].Peek_Buffer(),d->textures[9].Peek_Buffer());
	if (PlayerID == -1)
	{
		SendTextSc2(buf,PublicMessage,true,-3,-1);
	}
	else
	{
		SendTextSc2(buf,PrivateMessage,true,-3,PlayerID);
	}
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",SS_PURCHASE2,d->team,d->type,d->altpresetids[0][0],d->altpresetids[0][1],d->altpresetids[0][2],d->altpresetids[1][0],d->altpresetids[1][1],d->altpresetids[1][2],d->altpresetids[2][0],d->altpresetids[2][1],d->altpresetids[2][2],d->altpresetids[3][0],d->altpresetids[3][1],d->altpresetids[3][2],d->altpresetids[4][0],d->altpresetids[4][1],d->altpresetids[4][2],d->altpresetids[5][0],d->altpresetids[5][1],d->altpresetids[5][2],d->altpresetids[6][0],d->altpresetids[6][1],d->altpresetids[6][2],d->altpresetids[7][0],d->altpresetids[7][1],d->altpresetids[7][2],d->altpresetids[8][0],d->altpresetids[8][1],d->altpresetids[8][2],d->altpresetids[9][0],d->altpresetids[9][1],d->altpresetids[9][2],d->alttextures[0][0].Peek_Buffer(),d->alttextures[0][1].Peek_Buffer(),d->alttextures[0][2].Peek_Buffer(),d->alttextures[1][0].Peek_Buffer(),d->alttextures[1][1].Peek_Buffer(),d->alttextures[1][2].Peek_Buffer(),d->alttextures[2][0].Peek_Buffer(),d->alttextures[2][1].Peek_Buffer(),d->alttextures[2][2].Peek_Buffer(),d->alttextures[3][0].Peek_Buffer(),d->alttextures[3][1].Peek_Buffer(),d->alttextures[3][2].Peek_Buffer(),d->alttextures[4][0].Peek_Buffer(),d->alttextures[4][1].Peek_Buffer(),d->alttextures[4][2].Peek_Buffer(),d->alttextures[5][0].Peek_Buffer(),d->alttextures[5][1].Peek_Buffer(),d->alttextures[5][2].Peek_Buffer(),d->alttextures[6][0].Peek_Buffer(),d->alttextures[6][1].Peek_Buffer(),d->alttextures[6][2].Peek_Buffer(),d->alttextures[7][0].Peek_Buffer(),d->alttextures[7][1].Peek_Buffer(),d->alttextures[7][2].Peek_Buffer(),d->alttextures[8][0].Peek_Buffer(),d->alttextures[8][1].Peek_Buffer(),d->alttextures[8][2].Peek_Buffer(),d->alttextures[9][0].Peek_Buffer(),d->alttextures[9][1].Peek_Buffer(),d->alttextures[9][2].Peek_Buffer());
	if (PlayerID == -1)
	{
		SendTextSc2(buf,PublicMessage,true,-3,-1);
	}
	else
	{
		SendTextSc2(buf,PrivateMessage,true,-3,PlayerID);
	}
}

void Send_PT_Data()
{
	TeamPurchaseSettingsDefClass *d;
	d = Get_Team_Purchase_Definition(0);
	if (d)
	{
		Send_Team_Purchase_Definition(d,-1);
	}
	d = Get_Team_Purchase_Definition(1);
	if (d)
	{
		Send_Team_Purchase_Definition(d,-1);
	}
	PurchaseSettingsDefClass *d2;
	for (unsigned int i = 0;i < 5;i++)
	{
		for (unsigned int j = 0;j < 4;j++)
		{
			d2 = Get_Purchase_Definition(i,j);
			if (d2)
			{
				Send_Purchase_Definition(d2,-1);
			}
		}
	}
}

void Send_PT_Data_Player(int PlayerID)
{
	TeamPurchaseSettingsDefClass *d;
	d = Get_Team_Purchase_Definition(0);
	if (d)
	{
		Send_Team_Purchase_Definition(d,PlayerID);
	}
	d = Get_Team_Purchase_Definition(1);
	if (d)
	{
		Send_Team_Purchase_Definition(d,PlayerID);
	}
	PurchaseSettingsDefClass *d2;
	for (unsigned int i = 0;i < 5;i++)
	{
		for (unsigned int j = 0;j < 4;j++)
		{
			d2 = Get_Purchase_Definition(i,j);
			if (d2)
			{
				Send_Purchase_Definition(d2,PlayerID);
			}
		}
	}
}

unsigned int Purchase_Powerup_2(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset)
{
	int l = Powerup_Purchase_Hooks.Count();
	for (int i=0;i < l;i++)
	{
		int x;
		if (Powerup_Purchase_Hooks[i])
		{
			x = Powerup_Purchase_Hooks[i](base,purchaser,cost,preset,Powerup_Purchase_Hooks_Data[i]);
		}
		else
		{
			x = -1;
		}
		if (x != -1)
		{
			return x;
		}
	}
	if (!purchaser)
	{
		return 2;
	}
	PowerUpGameObjDef *powerup = (PowerUpGameObjDef *)Find_Definition(preset);
	if (!powerup || (powerup->Get_Class_ID() != CID_PowerUp))
	{
		return 4;
	}
	if (Purchase_Item(purchaser,cost))
	{
		powerup->Grant((SmartGameObj *)purchaser,0,true);
		return 0;
	}
	return 2;
}

unsigned int Purchase_Vehicle_2(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset)
{
	int l = Vehicle_Purchase_Hooks.Count();
	for (int i=0;i < l;i++)
	{
		int x;
		if (Vehicle_Purchase_Hooks[i])
		{
			x = Vehicle_Purchase_Hooks[i](base,purchaser,cost,preset,Vehicle_Purchase_Hooks_Data[i]);
		}
		else
		{
			x = -1;
		}
		if (x != -1)
		{
			return x;
		}
	}
	DefinitionClass *vehicle = Find_Definition(preset);
	if (!vehicle)
	{
		return 4;
	}
	float c = ((SoldierGameObj *)purchaser)->Get_Player_Data()->Money;
	if (!purchaser || cost > c)
	{
		return 2;
	}
	VehicleFactoryGameObj *vf = ((BuildingGameObj *)base->Find_Building(BT_VEHICLE_FACTORY))->As_VehicleFactoryGameObj();
	if (!vf)
	{
		return 3;
	}
	if ((vf->Is_Destroyed()) || (vf->Is_Busy()))
	{
		return 3;
	}
	float delay = 5.0;
	if (!base->Is_Base_Powered())
	{
		delay *= BuildTimeDelay;
	}
	vf->Request_Vehicle(preset,delay,(SoldierGameObj *)purchaser);
	Purchase_Item(purchaser,cost);
	return 0;
}

unsigned int Purchase_Character_2(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset)
{
	int l = Character_Purchase_Hooks.Count();
	for (int i=0;i < l;i++)
	{
		int x;
		if (Character_Purchase_Hooks[i])
		{
			x = Character_Purchase_Hooks[i](base,purchaser,cost,preset,Character_Purchase_Hooks_Data[i]);
		}
		else
		{
			x = -1;
		}
		if (x != -1)
		{
			return x;
		}
	}
	float c = ((SoldierGameObj *)purchaser)->Get_Player_Data()->Money;
	if (!purchaser || cost > c)
	{
		return 2;
	}
	BuildingGameObj *obj = (BuildingGameObj *)base->Find_Building(BT_SOLDIER_FACTORY);
	if (cost)
	{
		if (!obj || obj->Is_Destroyed())
		{
			return 3;
		}
	}
	DefinitionClass *character = Find_Definition(preset);
	if (!character || (character->Get_Class_ID() != CID_Soldier))
	{
		return 3;
	}
	SoldierGameObj *o = (SoldierGameObj *)purchaser;
	o->Re_Init(*(SoldierGameObjDef *)character);
	o->Post_Re_Init();
	Purchase_Item(purchaser,cost);
	return 0;
}

unsigned int Purchase_Powerup(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset)
{
	unsigned int x = Purchase_Powerup_2(base,purchaser,cost,preset);
	int l = Powerup_Purchase_Monitor_Hooks.Count();
	for (int i=0;i < l;i++)
	{
		if (Powerup_Purchase_Monitor_Hooks[i])
		{
			Powerup_Purchase_Monitor_Hooks[i](base,purchaser,cost,preset,x,Powerup_Purchase_Monitor_Hooks_Data[i]);
		}
	}
	return x;
}

unsigned int Purchase_Vehicle(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset)
{
	unsigned int x = Purchase_Vehicle_2(base,purchaser,cost,preset);
	int l = Vehicle_Purchase_Monitor_Hooks.Count();
	for (int i=0;i < l;i++)
	{
		if (Vehicle_Purchase_Monitor_Hooks[i])
		{
			Vehicle_Purchase_Monitor_Hooks[i](base,purchaser,cost,preset,x,Vehicle_Purchase_Monitor_Hooks_Data[i]);
		}
	}
	return x;
}

unsigned int Purchase_Character(BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset)
{
	unsigned int x = Purchase_Character_2(base,purchaser,cost,preset);
	int l = Character_Purchase_Monitor_Hooks.Count();
	for (int i=0;i < l;i++)
	{
		if (Character_Purchase_Monitor_Hooks[i])
		{
			Character_Purchase_Monitor_Hooks[i](base,purchaser,cost,preset,x,Character_Purchase_Monitor_Hooks_Data[i]);
		}
	}
	return x;
}

RENEGADE_FUNCTION
int __cdecl ren_printf(const char * _Format, ...)
AT2(0x007CB786,0x007CB026);

void Printf_Hook(const char *str)
{
	ren_printf("%s",str); //DONT REMOVE
	if (cNetwork::I_Am_Server())
	{
		if (rlmonaddr.sin_addr.s_addr)
		{
			sendto(cNetwork::PServerConnection->Get_Socket(),str,strlen(str)+1,0,(sockaddr*)&rlmonaddr,sizeof(rlmonaddr));
		}
	}
	if (Console_Output_Hook)
	{
		Console_Output_Hook(str);
	}
}

unsigned int Get_Shield_Type_Number(GameObject *obj)
{
	if (!Commands->Get_ID(obj) || !obj)
	{
		return 0;
	}
	DamageableGameObj *o = ((ScriptableGameObj *)obj)->As_DamageableGameObj();
	if (!o)
	{
		return 0;
	}
	return o->Get_Defense_Object()->Get_Shield_Type();
}

void Change_Time_Remaining(float time)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%f\n",SS_TIMEREMAINING,time);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	The_Game()->TimeRemaining_Seconds = time;
}

void Change_Time_Limit(int time)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_TIMELIMIT,time);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	The_Game()->TimeLimit_Minutes = time;
}

const char *Get_Player_Name_By_Data(cPlayer *x)
{
	if (!x)
	{
		return 0;
	}
	return WideCharToChar(x->PlayerName);
}

int Get_Player_ID_By_Data(cPlayer *x)
{
	if (!x)
	{
		return -1;
	}
	return x->PlayerId;
}

void Get_Merchandise_Information(GameObject *owner, PURCHASE_TYPE type, int position, int alternate, int &cost, int &presetid)
{
	PurchaseSettingsDefClass *def;
	TeamPurchaseSettingsDefClass *def2;
	int team = ((DamageableGameObj *)owner)->Get_Player_Type();
	switch (type)
	{
	case PT_CHARACTER:
		def = Get_Purchase_Definition(PTT_CHARACTERS,PTTEAM(team));
		cost = def->costs[position];
		if (alternate != -1)
		{
			presetid = def->altpresetids[position][alternate];
		}
		else
		{
			presetid = def->presetids[position];
		}
		break;
	case PT_VEHICLE:
		def = Get_Purchase_Definition(PTT_VEHICLES,PTTEAM(team));
		cost = def->costs[position];
		if (alternate != -1)
		{
			presetid = def->altpresetids[position][alternate];
		}
		else
		{
			presetid = def->presetids[position];
		}
		break;
	case PT_ENLISTED:
		def2 = Get_Team_Purchase_Definition(PTTEAM(team));
		cost = 0;
		presetid = def2->presetids[position];
		break;
	case PT_BEACON:
		def2 = Get_Team_Purchase_Definition(PTTEAM(team));
		cost = def2->beaconcost;
		presetid = def2->beaconpresetid;
		break;
	case PT_SECRETCHAR:
		def = Get_Purchase_Definition(PTT_SECRETCHARS,PTTEAM(team));
		cost = def->costs[position];
		if (alternate != -1)
		{
			presetid = def->altpresetids[position][alternate];
		}
		else
		{
			presetid = def->presetids[position];
		}
		break;
	case PT_SECRETVEHICLE:
		def = Get_Purchase_Definition(PTT_SECRETVEHICLES,PTTEAM(team));
		cost = def->costs[position];
		if (alternate != -1)
		{
			presetid = def->altpresetids[position][alternate];
		}
		else
		{
			presetid = def->presetids[position];
		}
		break;
	default:
		cost = 0;
		presetid = 0;
		break;
	}
}

bool File_Exists(const char* name)
{
	bool result = false;
	FileClass* file = Get_Data_File(name);
	if (file)
	{
		if (file->Is_Available(0))
			result = true;
		
		Close_Data_File(file);
	}

	return result;
}

void Print_Hook(char* formatx, ...)
{
	va_list list;
	va_start (list, formatx);
	StringClass string;
	string.Format_Args(formatx, list);
	va_end (list);
	if (TextDisplayGameModeClass::Instance)
	{
		TextDisplayGameModeClass::Instance->Print_System("%s", string);
	}
	ConsoleBox.Print("%s",string);
}

uint32 Purchase_Vendor_Item(SoldierGameObj *owner, PURCHASE_TYPE type, int position, int alternate, bool sendresponse)
{
	if (owner)
	{
		int ret = 2;
		if (!CombatManager::I_Am_Server())
		{
			(new cPurchaseRequestEvent)->Init(type,position,alternate);	
			return 1;
		}
		int pt;
		if (owner->Get_Player_Type())
		{
			pt = 1;
		}
		else
		{
			pt = 0;
		}
		BaseControllerClass *base = BaseControllerClass::Find_Base(pt);
		int cost;
		int presetid;
		Get_Merchandise_Information(owner,type,position,alternate,cost,presetid);
		if (type != PT_BEACON)
		{
			if (base)
			{
				if (!base->Is_Base_Powered() && !DisableCostMultiplier)
				{
					cost *= 2;
				}
			}
		}
		if (type == PT_REFILL)
		{
			GrantSupplies(owner);
			ret = 0;
		}
		else if (type == PT_CHARACTER || type == PT_ENLISTED || type == PT_SECRETCHAR)
		{
			ret = Purchase_Character(base,owner,cost,presetid);
		}
		else if (type == PT_VEHICLE || type == PT_SECRETVEHICLE)
		{
			ret = Purchase_Vehicle(base,owner,cost,presetid);
		}
		else if (type == PT_BEACON)
		{
			ret = Purchase_Powerup(base,owner,cost,presetid);
		}
		if (sendresponse)
		{
			(new cPurchaseResponseEvent)->Init(ret, cNetwork::Get_My_Id());
		}
		return ret;
	}
	return 2;
}
