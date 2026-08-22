/*	Renegade tt.dll
	Win32 specific code, addresses and patching
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "General.h"
#include "tt.h"

#include "cConnection.h"
#include "ScreenshotManager.h"
#include "Screenshot.h"
#include "engine2.h"
#include "string_ids.h"
#include "cCsTextObj.h"
#include "cNetwork.h"
#include "PhysicsSceneClass.h"
#include "WWAudioClass.h"
#include "texture.h"
#include "shaders.h"
#include "cGameSpyAdmin.h"
#include "VehicleGameObj.h"
#include "HumanStateClass.h"
#include "DynamicPhysClass.h"
#include "VehicleFactoryGameObj.h"
#include "cScTextObj.h"
#include "RadarManager.h"
#include "SaveLoadSystemClass.h"
#include "FileFactoryListClass.h"
#include "MultiHUDClass.h"
#include "IngameSendMessageDialog.h"
#include "CameraClass.h"
#include "BuildingAggregateDefClass.h"
#include "RepairBayGameObj.h"
#include "HarvesterClass.h"
#include "SpawnManager.h"
#include "NewDamageEvent.h"
#include "cPurchaseRequestEvent.h"
#include "ExplosionManager.h"
#include "SoldierGameObj.h"
#include "cGod.h"
#include "PacketManagerClass.h"
#include "DX8PolygonRendererClass.h"
#include "cPlayerManager.h"
#include "cRemoteHost.h"
#include "NetworkObjectFactoryMgrClass.h"
#include "HashMixFileFactoryClass.h"
#include "consolecommands.h"
#include "PointGroupClass.h"
#include "DazzleTypeClass.h"
#include "CombatManager.h"
#include "SortingRenderer.h"
#include "GameModeManager.h"
#include "CombatGameModeClass.h"
#include "vertexbuffer.h"
#include "ww3d.h"
#include "dialogmgr.h"
#include "optionsdlg.h"
#include "editctrl.h"
#include "dlginput.h"
#include "advancedgamelistings.h"
#include "TexProjectClass.h"
#include "hlod.h"
#include "dlgconfigvideotab.h"
#include "dlgconfigperformancetab.h"
#include "modpackageclass.h"
#include "WOLNATInterfaceClass.h"
#include "DX8TextureCategoryClass.h"
#include "MeshClass.h"
#include "WW3DAssetManager.h"
#include "ParticleEmitterPrototypeClass.h"
#include "DX8Caps.h"
#include "CClientControl.h"
#include "cScExplosionEvent.h"
#include "AirStripGameObj.h"
#include "DoorNetworkObjectClass.h"
#include "DX8Wrapper.h"
#include "stylemgr.h"
#include "CCameraClass.h"
#include "LoadingScreenClass.h"
#include "DlgHelpScreen.h"
#include "DlgCnCReference.h"
#include "SystemSettings.h"
#include "BulletDataClass.h"
#include "viewerctrl.h"
#include "CombatMaterialEffectManager.h"
#include "FileHash.h"
#include "cWinEvent.h"
#include "SurfaceEffectsManager.h"
#include "Input.h"
#include "TrackedVehicleClass.h"
#include "ResourceMgr/PackageManager.h"
#include "ModPackageMgrClass.h"
#include "RefCountPtr.h"
#include "cBioEvent.h"
#include "cSbboManager.h"
#include "SimpleFileFactoryClass.h"
#include "FileHashEvent.h"
#include "ScriptCommands.h"
#include "cCsPingRequestEvent.h"
#include "cLoadingEvent.h"
#include "CombatNetworkReceiverInstanceClass.h"
#include "cLanChat.h"
#include "DlgCnCWinScreen.h"
#include "PingProfileWait.h"
#include "DlgMPConnecting.h"
#include "ConsoleModeClass.h"
#include "ServerSettingsClass.h"
#include "cGameOptionsEvent.h"
#include "WeatherMgrClass.h"
#include "BackgroundMgrClass.h"
#include "MainMenuTransition.h"
#include "HUDClass.h"
#include "cDiagnostics.h"
#include "cGameType.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "engine_obj.h"
#include "engine_game.h"
#include "TranslateDBClass.h"
#include "C4GameObj.h"
#include "BeaconGameObj.h"
#include "PlayerMenu.h"
#include "hooksupport.h"
#include "InputConfigMgrClass.h"
#include "RefineryGameObj.h"

RENEGADE_FUNCTION
void Static_Anim_Phys_Goto_Frame(int ObjectID,float Frame,const char *Anim)
AT2(0x0072A620,0x00729EC0);
RENEGADE_FUNCTION
void Static_Anim_Phys_Goto_Last_Frame(int ObjectID,const char *Anim)
AT2(0x0072A680,0x00729F20);
RENEGADE_FUNCTION
void Set_Display_Color(unsigned char Red,unsigned char Green,unsigned char Blue)
AT2(0x00729770,0x00729010);
RENEGADE_FUNCTION
void Display_Text(int StringID)
AT2(0x007297E0,0x00729080);
RENEGADE_FUNCTION
void Display_Float(float Number,const char *Message)
AT2(0x00729870,0x00729110);
RENEGADE_FUNCTION
void Display_Int(int Number,const char *Message)
AT2(0x00729910,0x007291B0);
RENEGADE_FUNCTION
void Shake_Camera(const Vector3 &Pos,float Radius,float Intensity,float Time)
AT2(0x0072A830,0x0072A0D0);
RENEGADE_FUNCTION
void Display_GDI_Player_Terminal()
AT2(0x0072B8A0,0x0072B140);
RENEGADE_FUNCTION
void Display_NOD_Player_Terminal()
AT2(0x0072B8D0,0x0072B170);
RENEGADE_FUNCTION
void Set_Screen_Fade_Color(float Red,float Green,float Blue,float Transition)
AT2(0x0072BCC0,0x0072B560);
RENEGADE_FUNCTION
void Set_Screen_Fade_Opacity(float Opacity,float Transition)
AT2(0x0072BCE0,0x0072B580);
RENEGADE_FUNCTION
void Fade_Background_Music(const char *Music,int FadeOut,int FadeIn)
AT2(0x00729090,0x00728930);
RENEGADE_FUNCTION
void Set_Background_Music(const char *Music)
AT2(0x007290B0,0x00728950);
RENEGADE_FUNCTION
void Stop_Background_Music()
AT2(0x007290D0,0x00728970);
RENEGADE_FUNCTION
int Create_Sound(const char *Sound,const Vector3 &Position,GameObject *Obj)
AT2(0x00728C50,0x007284F0);
RENEGADE_FUNCTION
int Create_2D_Sound(const char *Sound)
AT2(0x00728D50,0x007285F0);
RENEGADE_FUNCTION
int Create_2D_WAV_Sound(const char *Sound)
AT2(0x00728DA0,0x00728640);
RENEGADE_FUNCTION
int Create_3D_WAV_Sound_At_Bone(const char *Sound,GameObject *Obj,const char *Bone)
AT2(0x00728DF0,0x00728690);
RENEGADE_FUNCTION
int Create_3D_Sound_At_Bone(const char *Sound,GameObject *Obj,const char *Bone)
AT2(0x00728E70,0x00728710);
uint32 Purchase_Vendor_Item(SoldierGameObj *owner, PURCHASE_TYPE type, int position, int alternate, bool sendresponse);
extern bool UseExtraPTPages;
extern bool NewUnpurchasableLogic;
extern bool SidebarSoundsEnabled;
extern bool ListColumnColorEnabled;
extern bool NoVehicleFlipKill;
extern float ListColumnColorRed;
extern float ListColumnColorGreen;
extern float ListColumnColorBlue;
int Get_Key_ID();
bool ClientChatLog = true;
extern bool HintsEnabled;
extern sockaddr_in *addr;
const unsigned char Code[19] = {0x6A,0x40,0xFF,0x74,0x24,0x0C,0xFF,0x74,0x24,0x0C,0xE8,0xC0,0xFF,0xFF,0xFF,0x83,0xC4,0x0C,0xC3};
unsigned long nickret;
HINSTANCE ProgramInstance;
extern char *modreg;
extern char *VersionRegistryString;
extern char *WOLUrlRegistryString;
extern bool DisableKillMessages;
float ServerVersion;

void Level_Loader_Thread();

void __declspec(naked) List_Column_Patch()
{
	_asm {
		mov eax, ListColumnColorRed
		mov [ebp+4], eax
		mov eax, ListColumnColorGreen
		mov [ebp+8], eax
		mov eax, ListColumnColorBlue
		mov [ebp+0x0C], eax
		pop ebp
		pop ecx
		retn 0x0C
	}
}





WNDPROC previousWindowHandler;
uint screenshotFrameSkips;

LRESULT CALLBACK windowHandler(HWND windowHandle, UINT messageId, WPARAM wParam, LPARAM lParam)
{
	switch (messageId)
	{
	case WM_ACTIVATEAPP:
		screenshotFrameSkips = 2;
		break;
	case WM_HOTKEY:

		if (wParam == IDHOT_SNAPDESKTOP || wParam == IDHOT_SNAPWINDOW)
		{
			Screenshot* screenshot = new Screenshot();
			screenshot->makeBuffer(windowHandle);
			screenshotManager.queueScreenshot(screenshot);
			return 0;
		}

		break;

	}

	return CallWindowProc(previousWindowHandler, windowHandle, messageId, wParam, lParam);
}



void postRenderHook()
{
	const char* url = screenshotManager.getPendingScreenshotUrl();
	if (url && GameInFocus && _HiddenFrameCount == 0 && screenshotFrameSkips-- == 0)
	{
		Make_Remote_Screen_Shot(url);
		screenshotManager.clearPendingScreenshotUrl();

		WideStringClass message;
		message.Format(L"a\n0");
		cCsTextObj* messageObj = new cCsTextObj();
		messageObj->Init(message, PrivateMessage, cNetwork::PClientConnection->Get_Local_Id(), -3);
	}
}



REF_DEF1(ExtrasEnabled,bool,0x0082F144);
void HUDInit()
{
	InitInfoTexture();
	if (UseExtraPTPages)
	{
		unsigned char jmp[1] = {0xEB};
		unsigned char ret2[3] = {0xC2,0x04,0x00};
		WriteMemory(0x0047EDA6,jmp,sizeof(jmp)); //enable secret PT pages
		WriteMemory(0x00427C80,ret2,sizeof(ret2)); //enable secret PT pages
		ExtrasEnabled = 1;
	}
	if (NewUnpurchasableLogic)
	{
		const unsigned char code1[13] = {0x8A,0x8D,0xD6,0x06,0x00,0x00,0x84,0xC9,0x8B,0xCD,0x90,0x90,0x90};
		const unsigned char code2[32] = {0x6A,0x02,0xE8,0x46,0xFE,0x26,0x00,0x85,0xC0,0x74,0x47,0x8B,0xC8,0x8B,0x01,0xFF,0x50,0x78,0x85,0xC0,0x74,0x3C,0x8A,0x88,0xB0,0x08,0x00,0x00,0x84,0xC9,0x74,0x30};
		const unsigned char code5[1] = {0xEB};
		const unsigned char code6[13] = {0x8A,0x8D,0xD5,0x06,0x00,0x00,0x84,0xC9,0x90,0x90,0x90,0x90,0x90};
		const unsigned char code9[1] = {0xEB};
		const unsigned char code10[5] = {0x33,0xC0,0x90,0x90,0x90};
		WriteMemory(0x0047F870,code1,sizeof(code1)); //new unpurchasable logic
		WriteMemory(0x0047F883,code2,sizeof(code2)); //new unpurchasable logic
		WriteNop((void *)0x0047F8D3,6); //new unpurchasable logic
		WriteNop((void *)0x0047F8DE,2); //new unpurchasable logic
		WriteMemory(0x0047F8E6,code5,sizeof(code5)); //new unpurchasable logic
		WriteMemory(0x0047F96D,code6,sizeof(code6)); //new unpurchasable logic
		WriteNop((void *)0x0047F97C,24); //new unpurchasable logic
		WriteNop((void *)0x0047F999,2); //new unpurchasable logic
		WriteMemory(0x0047F9A1,code9,sizeof(code9)); //new unpurchasable logic
		WriteMemory(0x004815B4,code10,sizeof(code10)); //new unpurchasable logic
	}
	if (SidebarSoundsEnabled)
	{
		WriteNop((void *)0x004B8F31,123); //sidebar sounds patch to disable normal sound
	}
	if (ListColumnColorEnabled)
	{
		WriteJump((void *)0x004F2C9D,List_Column_Patch); //dialog box color changes
	}
	if (VersionRegistryString)
	{
		WriteMemory(0x0042DABC,&VersionRegistryString,4); //version registry string change
	}
	if (WOLUrlRegistryString)
	{
		WriteMemory(0x004B2F57,&WOLUrlRegistryString,4); //WOL ingame URL string change
	}
	if (DisableKillMessages)
	{
		const unsigned char code[5] = {0x83,0xC4,0x10,0x90,0x90};
		WriteMemory(0x004B8895,code,sizeof(code)); //kill message disable
	}
}

void Do_Death_Sound(GameObject *obj,int soundid,Matrix3D const &pos,RefCountClass *ref,unsigned long unk1,int unk2)
{
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d",SS_POWERUPSOUND,Commands->Get_ID(obj),soundid);
	SendTextSc(buf,PublicMessage,0,-2,-1);
}

void _stdcall Do_Powerup_Sound(int soundid,Matrix3D const &pos,RefCountClass *ref,unsigned long unk1,int unk2)
{
	GameObject *obj;
	_asm {
		mov obj, esi
	}
	WWAudioClass::_theInstance->Create_Instant_Sound(soundid,pos,ref,unk1,unk2);
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d",SS_POWERUPSOUND,Commands->Get_ID(obj),soundid);
	SendTextSc(buf,PublicMessage,0,-2,-1);
}



Vector3 FogColor = Vector3(0,0,0);
void Set_Fog_Color(unsigned int red, unsigned int green, unsigned int blue)
{
	FogColor.X = red / 255.0f;
	FogColor.Y = green / 255.0f;
	FogColor.Z = blue / 255.0f;
	SetFogColor(red,green,blue);
}

void Set_Fog_Mode(unsigned int mode)
{
	SetFogMode(mode);
}

void Set_Fog_Density(float density)
{
	SetFogDensity(density);
}

void VendorGrantSupplies(SoldierGameObj *obj)
{
	WeaponBagClass *bag = obj->Get_Weapon_Bag();
	for (int i = 0;i < bag->Get_Count();i++)
	{
		WeaponClass *weapon = bag->Peek_Weapon(i);
		if (weapon)
		{
			if (weapon->Get_Definition()->CanRecieveGenericCnCAmmo)
			{
				weapon->Set_Inventory_Rounds_Client(weapon->Get_Definition()->MaxInventoryRounds);
				weapon->Set_Clip_Rounds_Client(weapon->Get_Definition()->ClipSize);
			}
		}
	}
	obj->Get_Defense_Object()->Set_Health(obj->Get_Defense_Object()->Get_Health_Max());
	obj->Get_Defense_Object()->Set_Shield_Strength(obj->Get_Defense_Object()->Get_Shield_Strength_Max());
}

void Do_Weapon_Data_Send
	(GameObject* o, int a, int b, bool c)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	GameObject *o2 = As_VehicleGameObj(o);
	if (o2)
	{
		char buf[512];
		sprintf(buf,"j\n%d\n%d\n%d\n%d\n%d\n",SS_WEAPONSEND,Commands->Get_ID(o2),a,b,c);
		SendTextSc(buf,PublicMessage,0,-2,-1);
	}
}

WeaponBagClass *bag;
GameObject *wbo;
WeaponClass *_stdcall Do_Weapon_Patch(uint32 weaponId, sint32 ammo, bool owned)
{
	Do_Weapon_Data_Send(wbo,weaponId,ammo,owned);
	return bag->Add_Weapon(weaponId,ammo,owned);
}

WeaponClass __declspec(naked) *Weapon_Patch(uint32 weaponId, sint32 ammo, bool owned)
{
	_asm {
		mov bag, ecx
		mov wbo, ebp
		jmp Do_Weapon_Patch
	}
}

void KeyboardUpdateHook()
{
	KeyboardUpdate();
	Input::Update();
}

unsigned int Calculate_CRC(const char *filename)
{
	FILE *f = fopen(filename,"rb");
	if (!f)
	{
		return 0;
	}
	long size;
	if (!fseek(f,0,SEEK_END))
	{
		size = ftell(f);
		if (!fseek(f,0,SEEK_SET))
		{
		}
	}
	else
	{
		size = 0;
	}
	unsigned char *data = new unsigned char[size];
	fread(data,size,1,f);
	unsigned int x = CRC_Memory(data,size,0);
	delete[] data;
	return x;
}

void Do_Version_Send()
{
	long ID = cNetwork::PClientConnection->Get_Local_Id();

	TT_ASSERT(cNetwork::Get_Client_Rhost());
	if (cNetwork::Get_Client_Rhost()->getVersion() < 4.0f)
	{
		char buf[255];
		sprintf(buf,"j\n%d\n%f\n",ID,TT_VERSION);
		SendTextCs(buf,PrivateMessage,ID,-3);
	}

	char val[50];
	if (modreg)
	{
		sprintf(val,"%sDataCount",modreg);
		unsigned int count = Get_Registry_Int(val,0);
		if (count)
		{
			char buf3[255];
			memset(buf3,0,sizeof(buf3));
			char ival[5];
			sprintf(ival,"d\n%d\n%d\n",ID,count);
			strcat(buf3,ival);
			for (unsigned int i = 0;i < count;i++)
			{
				sprintf(val,"%sData%d",modreg,i);
				unsigned int value = Get_Registry_Int(val,0);
				sprintf(ival,"%d\n",value);
				strcat(buf3,ival);
			}
			SendTextCs(buf3,PrivateMessage,ID,-3);
		}
	}
}

void __declspec(naked) Building_Msg_Check()
{
	_asm {
		mov cl, [eax+0x8B0]
		test cl, cl
		jnz loc1
		mov cl, CurrentlyBuilding
loc1:
		retn
	}
}

REF_DEF1(CurrentCaps, DX8Caps*, 0x008313E0);
extern REF_DECL1(_Hwnd_0, HWND);
void Release_Hook()
{
	SetWindowLong(_Hwnd_0, GWL_WNDPROC, (LONG)previousWindowHandler);
	SAFE_DELETE(CurrentCaps);
	DX8Wrapper::Shutdown();
}

bool cGameData::Set_Max_Players(int _MaxPlayers)
{
	if (_MaxPlayers > 126)
		_MaxPlayers = 126;
	
	MaxPlayers = _MaxPlayers;

	if (cNetwork::PServerConnection)
	{
		cNetwork::PServerConnection->setMaxRemoteHostCount(MaxPlayers);
		(new cGameOptionsEvent)->Init(-1);
	}

	RefPtr<WWOnline::Session> session = WWOnline::Session::GetInstance(false);
	if (session)
	{
		unsigned int playerCount;
		unsigned int maxPlayerCount = MaxPlayers;
		
		if (The_Game())
		{
			playerCount = The_Game()->CurrentPlayers;

			if (The_Game()->IsDedicated)
			{
				playerCount++;
				maxPlayerCount++;
			}
		}
		else
		{
			if (Exe == 0)
				maxPlayerCount = 2;
			else
				maxPlayerCount = 1;
		}
		// session->unk01AC->vfnr63(playerCount, maxPlayerCount);
		WWOnline::Session& sessionRef = *session;
		__asm
		{
			mov eax, sessionRef
			mov ecx, [eax+0x1AC]
			mov edx, [ecx]
			push maxPlayerCount
			push playerCount
			push ecx
			call [edx+0xFC]
		}
	}

	return true;
}



bool cGameData::Is_Map_Valid(char** _fileName)
{
	char* fileName = (ModName.Is_Empty() ? MapName : ModName).Peek_Buffer();
	
	if (_fileName)
		*_fileName = fileName;

	return File_Exists(fileName);
}



bool cGameData::Is_Valid_Settings(WideStringClass& errorString, bool a3)
{
	if (MapName.Is_Empty())
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("No initial level file selected\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_NO_LEVEL);
		return false;
	}
	
	char* mapName;
	if (!Is_Map_Valid(&mapName))
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("Map file '%s' not found\n\n", mapName);
		errorString.Format(TRANSLATE(IDS_HOPTERR_MAP_NOTFOUND), mapName);
		return false;
	}
	
	if (IsPassworded && Password.Is_Empty())
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("Blank passwords are not allowed\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_BLANK_PASS);
		return false;
	}
	
	if (IsPassworded && IsQuickMatch)
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("A password cannot be used on a Quickmatch server\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_QM_HASPASS);
		return false;
	}
	
	if (IsQuickMatch && !IsLaddered)
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("A Quickmatch server must also be laddered\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_QM_NOLADDER);
		return false;
	}
	
	if (IsQuickMatch && IsClanGame)
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("Clans are not allowed in a quickmatch server\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_QM_NOCLANS);
		return false;
	}
	
	if (IsLaddered && IsTeamChangingAllowed)
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("Team changing not allowed on a laddered server\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_NO_TEAMCHANGE);
		return false;
	}
	
	if (IsClanGame && (IsPassworded || RemixTeams))
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("Team changing or Team Remix not allowed on a clan game server.\n");
		errorString.Convert_From("Team changing or Team Remix not allowed on a clan game server.");
		return false;
	}
	
	if (MaxPlayers == 0 && ServerSettingsClass::GameMode != 2)
	{
		ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
		ConsoleBox.Print("Max player count must be greater than 0\n\n");
		errorString = TRANSLATE(IDS_HOPTERR_MAXPLAYER_0);
		return false;
	}
	
	if (a3 || ConsoleBox.Is_Exclusive())
	{
		if (MaxPlayers >= 128)
		{
			ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
			ConsoleBox.Print("Max player count must be less than 128\n\n");
			errorString.Format(TRANSLATE(IDS_HOPTERR_MAXPLAYER_TOOBIG), 128);
		}
		
		if (DoMapsLoop)
		{
			if (MapList[0].Is_Empty())
			{
				ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
				ConsoleBox.Print("Map cycling is enabled but there are no maps in the cycle\n\n");
				errorString = TRANSLATE(IDS_HOPTERR_NO_MAPCYCLE);
				return false;
			}
			
			if (DoMapsLoop && ModName.Is_Empty())
			{
				for (int mapIndex = 0; mapIndex < 100; ++mapIndex)
				{
					const StringClass& mapName2 = MapList[mapIndex];
					if (!mapName2.Is_Empty() && !File_Exists(mapName2.Peek_Buffer()))
					{
						ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
						ConsoleBox.Print("Map file '%s' not found\n\n", mapName2);
						errorString.Format(TRANSLATE(IDS_HOPTERR_MAP_NOTFOUND), mapName);
						return false;
					}
					/*
					Westwood did not check the map file; only whether the .mix file was available:
					if (mapName.Is_Empty())
					{
						StringClass mapPath;
						mapPath.Format("data/%s", mapName);
						RawFileClass file(mapPath);
						if (!file->Is_Available(0))
						{
							ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
							ConsoleBox.Print("Map file '%s' not found\n\n", mapPath);
							errorString.Format(TRANSLATE(IDS_HOPTERR_MAP_NOTFOUND), mapPath);
							return false;
						}
					}
					*/
				}
			}
		}
		
		if (!Is_Limited())
		{
			ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
			ConsoleBox.Print("Game must end by time limit or base destruction\n\n");
			errorString = TRANSLATE(IDS_HOPTERR_NO_GAMEEND);
			return false;
		}
		
		if (Motd.Get_Length() > 100)
		{
			ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
			ConsoleBox.Print("Message of the day cannot exceed %d characters\n\n", 100);
			errorString.Format(TRANSLATE(IDS_HOPTERR_MOTD_TOOBIG), 100);
			return false;
		}
		
		if (GameTitle.Get_Length() > 100)
		{
			ConsoleBox.Print("File %s - Error:\r\n\t ", INI);
			ConsoleBox.Print("Game title cannot exceed %d characters\n\n", 100);
			errorString.Format(TRANSLATE(IDS_HOPTERR_TITLE_TOOBIG), 100);
			return false;
		}
	}
	
	return true;
}



const wchar_t* cGameData::Get_Team_Word()
{
	if (IsClanGame)
		return TRANSLATE(IDS_MP_CLAN);
	else
		return TRANSLATE(IDS_MP_TEAM);
}

void _stdcall VehicleOwnershipPatch(GameObject *owner)
{
	GameObject *vehicle;
	_asm {
		mov vehicle,edi
	}
	Commands->Send_Custom_Event(owner,vehicle,CUSTOM_EVENT_VEHICLE_OWNER,Commands->Get_ID(owner),0);
}

RENEGADE_FUNCTION
BaseControllerClass *BaseControllerClass::Find_Base_For_Star()
AT1(0x006EF7D0);

void Enable_Radar_Patch()
{
	BaseControllerClass* base = BaseControllerClass::Find_Base_For_Star();
	if (base)
		base->Enable_Radar(base->Is_Radar_Enabled());
}
void SendTextCs(const char *string,TextMessageEnum mtype,int sender,int target)
{
	cCsTextObj *textobj = new cCsTextObj();
	WideStringClass *wcs = CharToWSC(string);
	textobj->Init(*wcs,mtype,sender,target);
	delete wcs;
}

RENEGADE_FUNCTION
void SmartGameObj::ConInit()
AT2(0x0069E290,0x0069DB30);

void VehicleGameObj::ConInit()
{
	SmartGameObj::ConInit();
	Do_Vehicle_Observer(this);
	SCRIPTS_visible = true;
	SCRIPTS_LastTeam = -2;
	SCRIPTS_DamageMeshesUpdate = false;
}

bool VehicleGameObj::Is_Visible()
{
	if (!SCRIPTS_visible)
	{
		return false;
	}
	return true;
}

void Script_Notify(int ID,int notify)
{
	char buf[255];
	sprintf(buf,"s\n%d\n%d\n",ID,notify);
	SendTextCs(buf,PrivateMessage,cNetwork::PClientConnection->Get_Local_Id(),-3);
}

struct ObbCollisionStruct;
RENEGADE_FUNCTION
void collide_obb_obb(ObbCollisionStruct *, CastResultStruct *)
AT2(0x00604430,0x00603CD0);

void Bluehell_Fix(ObbCollisionStruct *obb, CastResultStruct *res)
{
	char *c = (char *)obb;
	c[12] = 0;
	collide_obb_obb(obb,res);
}

void BaseControllerClass::Distribute_Funds_To_Each_Teammate(int Amount)
{
	Distribute_Funds_To_Each_Teammate((float)Amount);
}

void BaseControllerClass::Distribute_Funds_To_Each_Teammate(float Amount)
{
	for (SLNode<cPlayer>* playerNode = PlayerList.Head(); playerNode; playerNode = playerNode->Next())
	{
		cPlayer* player = playerNode->Data();
		if (player->IsInGame)
		{
			int playerType = (player->Owner && player->Owner->As_SoldierGameObj()) ? player->Owner->As_SoldierGameObj()->Get_Player_Type() : player->PlayerType;
			if (playerType == Get_Team())
				player->Increment_Money(Amount);
		}
	}
}

RENEGADE_FUNCTION
void Relase_Log()
AT2(0x005D9A70,0x005D9310);

void _stdcall ResetGeometryPatch(int polycount, int vertcount)
{
	MeshModelClass *m;
	_asm {
		mov m, ecx
	}
	ResetGeometry(m,polycount,vertcount);
}
RefCountPtr<TextureClass> stealthTexture;
void Init_Stealth_Texture()
{
	char *Name = newstr("stealth_effect.tga");
	FileClass *f = Get_Data_File(Name);
	if ((!f) || (!f->Is_Available()))
	{
		int len = strlen(Name);
		Name[len - 3] = 'd';
		Name[len - 2] = 'd';
		Name[len - 1] = 's';
		f = Get_Data_File(Name);
	}
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckLoad(data,size,Name,HashStealth);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	stealthTexture = Create_Get(WW3DAssetManager::TheInstance->Get_Texture(Name,TextureClass::MIP_LEVELS_ALL,WW3D_FORMAT_UNKNOWN,true));
	delete[] Name;
}
TextureClass* Peek_Stealth_Texture()
{
	return stealthTexture.Peek();
}

void Poke_Do(GameObject *poked,GameObject *poker)
{
	if ((!CombatManager::I_Am_Server()) && (cGameType::GameType != 1))
	{
		char buf[512];
		sprintf(buf,"j\n%d\n%d\n",Commands->Get_ID(poked),Commands->Get_ID(poker));
		SendTextCs(buf,PrivateMessage,cNetwork::PClientConnection->Get_Local_Id(),-2);
	}
}

void *cDonateEvent;
void *cConsoleCommandEvent;
void *cCsConsoleCommandEvent;
void *cMoneyEvent;
void *cRequestKillEvent;
void *cScoreEvent;
void *cWarpEvent;
void *dep;
void *alwaysdep;
void *UDP1;
void *UDP2;
void *UDP4;
void *UDP5;
void *UDP6;
void *UDP7;
void *UDP8;
void *StartBugFix;
float *SlidePrecision;
void CheckGlobal(char* data, int dataLength, char* _fileName, GlobalFileType _type);
void CheckMap(char* data, int dataLength, char* _fileName, MapFileType _type);
void CheckLoad(char* data, int dataLength, char* _fileName, GlobalFileType _type);
void __declspec(dllimport) SetHashChecks(MapCheck m,GlobalCheck g,LoadCheck l);
void ReadHintFile();
extern WeatherMgrClass *_TheWeatherMgr;
extern BackgroundMgrClass *_TheBackgroundMgr;

void EmptyFunc()
{
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL,
						DWORD ul_reason_for_call,
						LPVOID lpReserved
					 )
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		Relase_Log();
		OSVERSIONINFO osv;
		osv.dwOSVersionInfoSize = sizeof(osv);
		GetVersionEx(&osv);
		if ((osv.dwPlatformId != VER_PLATFORM_WIN32_NT) || (osv.dwMajorVersion == 4))
		{
			MessageBox(NULL,"tt.dll requires Windows 2000/XP/2003/Vista to Run","Error",MB_OK|MB_ICONEXCLAMATION|MB_TOPMOST);
			ExitProcess(1);
		}
		ProgramInstance = hinstDLL;
		WSADATA wsaData;
		WSAStartup(MAKEWORD(1,1), &wsaData);
		Chat_Hook = 0;
		Version_Hook = 0;
		Host_Hook = 0;
		WireframeMode = D3DFILL_SOLID;
		Player_Join_Hook = 0;
		Player_Leave_Hook = 0;
		Load_Level_Hook = 0;
		Think_Hook = 0;
		Game_Over_Hook = 0;
		Console_Output_Hook = 0;
		PTDataChanged = false;
		MapTexture = new char[260];
		MapOffset = Vector2(0,0);
		strcpy(MapTexture,"hud_main.tga");
		BuildingHarvester[0] = false;
		BuildingHarvester[1] = false;
		CurrentlyBuildingTeam[0] = false;
		CurrentlyBuildingTeam[1] = false;
		ExplosionObj = 0x0;
		Data_Hook = 0;
		TT_ASSERT(Exe != 6);
		unsigned char jmp[1] = {0xEB};
		switch (Exe)
		{
			case 0: //game client
			{
				Commands = (ScriptCommands *)0x0085F490;
				SlidePrecision = (float *)0x007E1880;
				StartBugFix = (void *)0x004C188C;
				UDP1 = (void *)0x0046A770;
				UDP4 = (void *)0x0061F99D;
				UDP5 = (void *)0x007725B4;
				UDP6 = (void *)0x00773845;
				UDP7 = (void *)0x007757F4;
				UDP8 = (void *)0x00775EED;
				dep = (void *)0x00813950;
				alwaysdep = (void *)0x00813958;
				cDonateEvent = (void *)0x004B6740;
				cConsoleCommandEvent = (void *)0x004B5AA0;
				cCsConsoleCommandEvent = (void *)0x004B5CD0;
				cMoneyEvent = (void *)0x004B82F0;
				cRequestKillEvent = (void *)0x004B92C0;
				cScoreEvent = (void *)0x004B94C0;
				cWarpEvent = (void *)0x004BB250;
				ShaderCaps::Initialize();
				WriteCall((void *)0x0043B905,KeyboardUpdateHook); //custom keyboard keys hook
				WriteJump(0x004015C6,DoCombatKeyboard); //custom keyboard keys hook
				WriteNop((void *)0x0043C21F,5); //nop out WWPhys::Shutdown call
				unsigned char EmotIcons[55] = {0x8B,0x44,0x24,0x0C,0x8D,0x4C,0x24,0x0C,0x6A,0x20,0x51,0x8B,0xCF,0x89,0x86,0xBC,0x06,0x00,0x00,0xE8,0x02,0x02,0x2D,0x00,0x8B,0x44,0x24,0x0C,0x8D,0x4C,0x24,0x0C,0x6A,0x20,0x51,0x8B,0xCF,0x89,0x86,0xC0,0x06,0x00,0x00,0xE8,0xEA,0x01,0x2D,0x00,0x90,0x90,0x90,0x90,0x90,0x90,0x90 };
				WriteMemory(0x004B488C,EmotIcons,sizeof(EmotIcons)); //emoticons fix
				ClientChatLog = Get_Registry_Int("ClientChatLog",0);
				HintsEnabled = Get_Registry_Int("HintsEnabled",1);
				WriteJump(0x006EF6C0,Enable_Radar_Patch); //radar fix
				WriteCall((void *)0x0047F899,Building_Msg_Check,1); //purchase terminal "building" message change
				const unsigned char ppmsgcode[16] = {0x33,0xD2,0x38,0x97,0xD4,0x06,0x00,0x00,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x75};
				WriteMemory(0x00481D1F,ppmsgcode,sizeof(ppmsgcode)); //dead powerplant 2x cost message change
				WriteVtable2(0x007D51DC,OptionsDlg::New_Start_Dialog); //options dialog hook
				WriteVtable2(0x007D5200,OptionsDlg::New_On_Command); //options dialog hook
				SetScriptNotify(Script_Notify);
				SetHashChecks(CheckMap,CheckGlobal,CheckLoad);
				WriteVtable2(0x007D513C,RenegadeUIInputClass::Is_Button_Down); //hook for dialog boxes and side mouse buttons
				WriteCall((void *)0x00489BAD,Get_Key_ID,0x46-5); //hook for side mouse buttons in the default config dialog
				unsigned char mov[2] = {0x8B,0xF8};
				WriteMemory((char *)0x00489BF3-2,mov,2);
				hookAsCall(0x0047EB7A, 0x00000000, OnPtChatListInitPatch); //PT chatbox fix
				hookAsCall(0x0047F68D, 0x00000000, OnPtChatListUpdatePatch); //PT chatbox fix
				WriteNop((void *)0x00480122,7); //PT chatbox fix
				hookAsCall2(0x00481C45, 0x00000000, OnUnitPtKeyDownPatch, 5); //PT keypress fix
				hookAsJump(0x00481FF6, 0x00000000, OnUnitPtMerchandiseItemUpdatePatch); //PT keypress fix
				WriteNop((void *)0x0047FE5F,2); //PT keypress fix
				WriteNop((void *)0x0047FE86,2); //PT keypress fix
				hookAsJump(0x0047E730,0x00000000,DlgCncBattleInfo::On_Frame_Update); //team information and battlefield information scroll fix
				WriteNop((void *)0x0047DBFC,10); //team information and battlefield information scroll fix
				hookAsJump(0x00484470,0x00000000,DlgCncTeamInfo::On_Frame_Update); //team information and battlefield information scroll fix
				WriteNop((void *)0x004835DC,10); //team information and battlefield information scroll fix
				hookAsJump(0x004834E0,0x00000000,DlgCncServerInfo::On_Frame_Update); //team information and battlefield information scroll fix
				WriteNop((void *)0x0048326C,10); //team information and battlefield information scroll fix
				hookAsJump(0x004B6340,0x00000000,cCsTextObj::Init); //chat message sent patch
				hookAsJump(0x004BA720,0x00000000,cScTextObj::Import_Creation); //chat message sent patch
				WriteJump(0x0052A440,Make_Screen_Shot); //screenshot hook
				hookAsCall(0x0043B92D,0x00000000,0x004ECAA0); //Fix issue where beam manager is shut down then objects get added to the beam manager, causing a crash on exit
				hookAsCall(0x0043B937,0x00000000,0x00437070); //Fix issue where beam manager is shut down then objects get added to the beam manager, causing a crash on exit
				WriteNop((void *)0x004263A0,46); //remove Toggle_Sorting console command
				hookAsJump(0x00476280,0x00000000,cGameData::Render); //hook so that HUD console command turns off extra stuff
				hookAsCall(0x0058DAF5,0x00000000,ResetGeometryPatch); //MeshModelClass destroy patch
				WriteVtable2(0x007D40C0,QuickMatchDisable); //disable quickmatch
				hookAsJump (0x004A38C0, 0x00000000, AdvancedGameListingsMenu::sortList); //hook to change wol sort order
				hookAsCall (0x004A23CB, 0x00000000, AdvancedGameListingsMenu::addFavorite); //hook to allow you to select a favorite server
				unsigned char s1[1] = {0x9E};
				unsigned char s2[1] = {0x86};
				WriteMemory((void *)0x004A196E,s1,1); //WOL advanced game listings dialog default sort order
				WriteMemory((void *)0x004A197A,s2,1); //WOL advanced game listings dialog default sort order
				hookAsJump(0x00525800,0x0,SaveLoadSystemClass::Load); //hooks for filehashing checks
				hookAsJump(0x00463130, 0x00000000, MultiHUDClass::Render); //multihudclass rendering improvements
				hookAsJump(0x00463140, 0x00000000, MultiHUDClass::Show_Player_Names); //multihudclass rendering improvements
				hookAsJump(0x00495950, 0x00000000, IngameSendMessageDialog::On_EditCtrl_Key_Down); //chat dialog hooks
				hookAsJump(0x004957A0, 0x00000000, IngameSendMessageDialog::On_EditCtrl_Change); //chat dialog hooks
				hookAsJump(0x00494760, 0x00000000, IngameSendMessageDialog::sendMessage); //chat dialog hooks
				hookAsJump(0x00534FC0, 0x00000000, Render2DClass::Render);	//hook Render2DClass::Render
				hookAsJump(0x0052EE10, 0x00000000, Render2DSentenceClass::Render);  //hook Render2DSentenceClass::Render
				hookAsJump(0x00555DB0, 0x00000000, CameraClass::Apply);		// hook CameraClass::Apply
				hookAsJump(0x0053D260, 0x00000000, DazzleTypeClass::Calculate_Intensities); //hook Dazzle Intensity code
				hookAsJump(0x004AFF80, 0x00000000, MainMenuTransitionClass::On_Frame_Update); //dialog box hooks
				hookAsJump(0x004EC4E0, 0x00000000, DialogMgrClass::Shutdown); //dialog box hooks
				hookAsJump(0x004EC810, 0x00000000, DialogMgrClass::UnRegister_Dialog); //dialog box hooks
				hookAsJump(0x004EC930, 0x00000000, DialogMgrClass::On_Dialog_Removed); //dialog box hooks
				hookAsJump(0x004ED0A0, 0x00000000, DialogMgrClass::Flush_Dialogs); //dialog box hooks
				hookAsJump(0x004ED160, 0x00000000, DialogMgrClass::Is_Flushing_Dialogs); //dialog box hooks
				hookAsJump(0x004ED260, 0x00000000, DialogMgrClass::Show_IME_Message); //dialog box hooks
				hookAsJump(0x004ECAA0, 0x00000000, DialogMgrClass::On_Frame_Update); //dialog box hooks
				hookAsJump(0x004ECFC0, 0x00000000, DialogMgrClass::On_Key_Down); //dialog box hooks
				hookAsJump(0x004ED000, 0x00000000, DialogMgrClass::On_Key_Up); //dialog box hooks
				hookAsJump(0x004ED030, 0x00000000, DialogMgrClass::On_Unicode_Char); //dialog box hooks
				hookAsJump(0x004EC650, 0x00000000, DialogMgrClass::Register_Dialog); //dialog box hooks
				hookAsJump(0x004EA000, 0x00000000, DialogBaseClass::On_Frame_Update); //dialog box hooks
				hookAsJump(0x004EAE30, 0x00000000, DialogBaseClass::Remove_Control); //dialog box hooks
				hookAsJump(0x004E98E0, 0x00000000, DialogBaseClass::Start_Dialog); //dialog box hooks
				hookAsJump(0x004EA080, 0x00000000, DialogBaseClass::Render); //dialog box hooks
				hookAsJump(0x004EAA40, 0x00000000, DialogBaseClass::On_Key_Down); //dialog box hooks
				hookAsJump(0x004EAB10, 0x00000000, DialogBaseClass::On_Key_Up); //dialog box hooks
				hookAsJump(0x004EAB60, 0x00000000, DialogBaseClass::On_Unicode_Char); //dialog box hooks
				hookAsJump(0x004EAFB0, 0x00000000, DialogBaseClass::Add_Child_Dialog); //dialog box hooks
				hookAsJump(0x004E9F50, 0x00000000, DialogBaseClass::End_Dialog); //dialog box hooks
				hookAsJump(0x004EB020, 0x00000000, DialogBaseClass::Remove_Child_Dialog); //dialog box hooks
				hookAsJump(0x004ED900, 0x00000000, MenuDialogClass::Start_Dialog); //dialog box hooks
				hookAsJump(0x004ED8D0, 0x00000000, MenuDialogClass::Render); //dialog box hooks
				hookAsJump(0x0059F3F0, 0x00000000, HLodClass::Render); //hook HLodClass::Render for per-subobject lighting
				hookAsCall2(0x004AB642, 0x00000000, DlgConfigVideoTab::Do_Dialog, 22); //video config dialog
				hookAsCall2(0x004AB678, 0x00000000, DlgConfigPerformanceTab::Do_Dialog, 23); //performance config dialog
				unsigned char push[2] = {0x8B,0xF0};
				WriteMemory(0x004AB65D-2,push,sizeof(push)); //video config dialog
				WriteMemory(0x004AB696-2,push,sizeof(push)); //performance config dialog
				hookAsJump(0x00577A80,0x00000000,MeshClass::Render); //hooks for AutoPoolClass bits
				hookAsJump(0x00570810,0x00000000,ParticleEmitterPrototypeClass::Create); //hook for particle emitter creation
				hookAsJump(0x00546CA0,0x00000000,Release_Hook); //hook DX8Wrapper::Shutdown
				hookAsJump(0x0063F470,0x00000000,Peek_Stealth_Texture); //hook stealth texture loading
				hookAsJump(0x004E8240,0x00000000,StyleMgrClass::Initialize_From_INI); //hook for widescreen stylemgr stuff
				hookAsJump(0x004E9040,0x00000000,StyleMgrClass::Play_Sound); //hook for widescreen stylemgr stuff
				hookAsJump(0x006DBF10, 0x0, CCameraClass::Init); //hook for cameras.ini loading
				hookAsJump(0x006DBF20, 0x0, Cleanup_HUD); //hooking Cleanup_HUD in here so that it gets called before WW3DAssetManager shuts down
				hookAsJump(0x007248C0, 0x0, CombatMaterialEffectManager::Get_Spawn_Effect); //hook for MatrixMapper stuff
				hookAsJump(0x00724960, 0x0, CombatMaterialEffectManager::Get_Death_Effect); //hook for MatrixMapper stuff
				hookAsJump(0x007249F0, 0x0, CombatMaterialEffectManager::Get_Health_Effect); //hook for MatrixMapper stuff
				hookAsJump(0x00724A80, 0x0, CombatMaterialEffectManager::Get_Electrocution_Effect); //hook for MatrixMapper stuff
				// Change window message handler and register hotkey handlers
				previousWindowHandler = (WNDPROC)SetWindowLong(_Hwnd_0, GWL_WNDPROC, (LONG)windowHandler);
				RegisterHotKey(_Hwnd_0, IDHOT_SNAPWINDOW, MOD_SHIFT, VK_SNAPSHOT);
				RegisterHotKey(_Hwnd_0, IDHOT_SNAPWINDOW, MOD_CONTROL, VK_SNAPSHOT);
				ReadWolFavorites();
				DialogMgrClass::Init2();
				WriteJump(0x0043907C, (void*)0x00439192); // Do not load all .mix files at startup
				hookAsJump(0x00490180, 0x00000000, DlgHelpScreen::On_Menu_Activate); //hook for MenuBackdropClass stuff
				hookAsJump(0x00490200, 0x00000000, DlgHelpScreen::On_Destroy); //hook for MenuBackdropClass stuff
				hookAsJump(0x00482E30, 0x00000000, DlgCnCReference::On_Menu_Activate); //hook for MenuBackdropClass stuff
				hookAsJump(0x004829F0, 0x00000000, DlgCnCReference::On_Destroy); //hook for MenuBackdropClass stuff
				hookAsJump(0x004B60C0, 0x00000000, cCsPingRequestEvent::Init); //hook cCsPingRequestEvent
				hookAsJump(0x004B8010, 0x004B77B0, cLoadingEvent::Init); //hook cLoadingEvent
				hookAsJump(0x006A96C0, 0x006A8F60, SurfaceEffectsManager::Apply_Effect); //hooks for surfaceeffects.ini
				hookAsJump(0x00485560, 0x00000000, DlgCnCWinScreen::Update_List); //hook for win screen
				hookAsJump(0x004BC4E0, 0x00000000, PingProfileWait::updatePings); //hook PingProfileWait
				hookAsJump(0x004BC570, 0x00000000, PingProfileWait::getPings); //hook PingProfileWait
				hookAsJump(0x004BC6E0, 0x00000000, PingProfileWait::Create); //hook PingProfileWait
				hookAsJump(0x004BC9B0, 0x00000000, PingProfileWait::WaitBeginning); //hook PingProfileWait
				hookAsJump(0x004BCAB0, 0x00000000, PingProfileWait::HandleNotification); //hook PingProfileWait

				//hookAsJump(0x00494040, 0x00000000, DlgMPConnecting::Create);
				//hookAsJump(0x004940D0, 0x00000000, DlgMPConnecting::DlgMPConnecting);
				//hookAsJump(0x00494160, 0x00000000, DlgMPConnecting::~DlgMPConnecting);
				//hookAsJump(0x004941A0, 0x00000000, DlgMPConnecting::On_Command);
				hookAsJump(0x00494240, 0x00000000, DlgMPConnecting::On_Periodic); //hook connecting dialog
				ReadHintFile();
				Init_Stealth_Texture(); //this is here to cause the stealth texture to be loaded globally before connection to the server
			}
			break;
			case 1: //WFDS
			{
				Commands = (ScriptCommands *)0x0085E678;
				SlidePrecision = (float *)0x007E0868;
				StartBugFix = (void *)0x004C10FC;
				UDP1 = (void *)0x0046A0A0;
				UDP4 = (void *)0x0061F23D;
				UDP5 = (void *)0x00771E54;
				UDP6 = (void *)0x007730E5;
				UDP7 = (void *)0x00775094;
				UDP8 = (void *)0x0077578D;
				dep = (void *)0x00812B28;
				alwaysdep = (void *)0x00812B30;
				cDonateEvent = (void *)0x004B5F40;
				cConsoleCommandEvent = (void *)0x004B52A0;
				cCsConsoleCommandEvent = (void *)0x004B54D0;
				cMoneyEvent = (void *)0x004B7AF0;
				cRequestKillEvent = (void *)0x004B8AC0;
				cScoreEvent = (void *)0x004B8CC0;
				cWarpEvent = (void *)0x004BAA50;
				ServerVersion = TT_VERSION;
				hookAsCall(0x0,0x00435F3D,Printf_Hook); //hook for catching console output
				unsigned char rlmonbuf[3] = {0x83,0xC4,0x04};
				WriteMemory((char *)0x00435F3D+5,rlmonbuf,sizeof(rlmonbuf)); //RenLogMon hook
				WriteNop((char *)0x00435F3D+8,5); //RenLogMon hook
				WriteNop((void *)0x00434C59,167); //stop FDS from loading at startup
				WriteJump(0x0043911C, (void*)0x00439232); // Do not load all .mix files at startup
				// Unregister all server to client only netevents on the server.
				/*NetworkObjectFactoryMgrClass::Unregister(1000); // BaseGameObj (NetworkGameObjectFactoryClass)
				NetworkObjectFactoryMgrClass::Unregister(1001); // cScTextObj
				NetworkObjectFactoryMgrClass::Unregister(1002); // cPlayerKill
				NetworkObjectFactoryMgrClass::Unregister(1003); // cWinEvent
				NetworkObjectFactoryMgrClass::Unregister(1004); // cPurchaseResponseEvent
				NetworkObjectFactoryMgrClass::Unregister(1005); // cConsoleCommandEvent
				NetworkObjectFactoryMgrClass::Unregister(1007); // cSvrGoodbyeEvent
				NetworkObjectFactoryMgrClass::Unregister(1008); // cGameOptionsEvent
				NetworkObjectFactoryMgrClass::Unregister(1009); // cEvictionEvent
				NetworkObjectFactoryMgrClass::Unregister(1010); // cTeam
				NetworkObjectFactoryMgrClass::Unregister(1011); // cPlayer
				NetworkObjectFactoryMgrClass::Unregister(1012); // cGameDataUpdateEvent
				NetworkObjectFactoryMgrClass::Unregister(1013); // cScPingResponseEvent
				NetworkObjectFactoryMgrClass::Unregister(1014); // cScExplosionEvent
				NetworkObjectFactoryMgrClass::Unregister(1016); // SCAnnouncement
				NetworkObjectFactoryMgrClass::Unregister(1017); // cGameSpyScChallengeEvent
				NetworkObjectFactoryMgrClass::Unregister(1022); // cMoneyEvent
				NetworkObjectFactoryMgrClass::Unregister(1023); // cWarpEvent
				NetworkObjectFactoryMgrClass::Unregister(1028); // cGodModeEvent
				NetworkObjectFactoryMgrClass::Unregister(1029); // cVipModeEvent
				NetworkObjectFactoryMgrClass::Unregister(1030); // cScoreEvent
				NetworkObjectFactoryMgrClass::Unregister(1035); // cRequestKillEvent
				NetworkObjectFactoryMgrClass::Unregister(1036); // cCsConsoleCommandEvent
				NetworkObjectFactoryMgrClass::Unregister(1039); // cDonateEvent*/
			}
			break;
		}
		HUDIniRead();
		if (!Exe) //has to go after the call to HUDIniRead
		{
			if (DisableCostMultiplier)
			{
				WriteMemory(0x00481D2E,jmp,sizeof(jmp)); //dead powerplant 2x cost disable
				WriteMemory(0x0047EEA6,jmp,sizeof(jmp)); //dead powerplant 2x cost disable
			}
		}
		if (NoVehicleFlipKill)
		{
			unsigned char jmp2[1] = {0xEB};
			if (!Exe)
			{
				WriteMemory(0x0062D019,jmp2,sizeof(jmp2)); //make vehicles not die when they flip over
			}
			else
			{
				WriteMemory(0x0062C8B9,jmp2,sizeof(jmp2)); //make vehicles not die when they flip over
			}
		}
		ReadKeyboardConfig();
		ReadScopeConfig();
		hookNop(0x004261E2,0x004261E2,109); //remove MSG console command
		unsigned char setdel[5] = {0x8B,0x01,0xFF,0x60,0x2C};
		unsigned char ret[1] = {0xC3};
		hookAsCall(0x00670A13,0x006702B3,DoUnloadLevel); //unload level hook
		WriteMemory(cDonateEvent,setdel,sizeof(setdel)); //disable cDonateEvent
		WriteMemory(cConsoleCommandEvent,ret,sizeof(ret)); //disable cConsoleCommandEvent
		WriteMemory(cCsConsoleCommandEvent,ret,sizeof(ret)); //disable cCsConsoleCommandEvent
		WriteMemory(cMoneyEvent,setdel,sizeof(setdel)); //disable cMoneyEvent
		WriteMemory(cRequestKillEvent,setdel,sizeof(setdel)); //disable cRequestKillEvent
		WriteMemory(cScoreEvent,setdel,sizeof(setdel)); //disable cScoreEvent
		WriteMemory(cWarpEvent,setdel,sizeof(setdel)); //disable cWarpEvent
		hookAsCall(0x006F1944,0x006F0F04,Do_Powerup_Sound); //powerup sound patch
		hookAsCall(0x006F0EDA,0x006F049A,Weapon_Patch); //send weapon powerup grant over network change
		unsigned char depbuf[4] = {0x2E,0x78,0x78,0x78};
		unsigned char alwaysdepbuf[4] = {0x78,0x78,0x78,0x78};
		WriteMemory(dep,depbuf,sizeof(depbuf)); //faster map load change
		WriteMemory(alwaysdep,alwaysdepbuf,sizeof(alwaysdepbuf)); //faster map load change
		hookNop(0x00438B01,0x00438BA6,2); //logcopy change

		hookAsJump (0x0070CE40, 0x0070C400, C4GameObj::Export_Rare); //C4 getting stuck to repair bay arc fix
		hookAsJump (0x00723E50, 0x00723410, BuildingAggregateDefClass::Load); //repair bay fixes
        hookAsJump (0x007409D0, 0x00740270, RepairBayGameObj::CnC_Initialize); //repair bay fixes
        hookAsJump (0x00740DD0, 0x00740670, RepairBayGameObj::Think); //repair bay fixes
		hookAsJump (0x006EE1A0, 0x006ED760, VehicleFactoryGameObj::Request_Vehicle); //harvester spawns invisible after nod power plant is down fix
		hookAsJump (0x006EDFB0, 0x006ED570, VehicleFactoryGameObj::Think); //harvester spawns invisible after nod power plant is down fix
		hookAsJump (0x006D9660, 0x006D8F00, DamageableGameObj::Import_Occasional); //fix for harvester under attack sounds not playing
		hookAsJump (0x006A6620, 0x006A5EC0, PhysicalGameObj::Export_Rare); //animation fixes and Set_Model fixes
		hookAsJump (0x006A6810, 0x006A60B0, PhysicalGameObj::Import_Rare); //animation fixes and Set_Model fixes
		hookAsJump (0x006A5C60, 0x006A5500, PhysicalGameObj::Set_Animation); //animation fixes and Set_Model fixes
		hookAsJump (0x0076A0C0, 0x00769960, HarvesterClass::Action_Complete); //harvester harvesting arms animation fix
		hookAsJump (0x0076A1A0, 0x00769A40, HarvesterClass::Think); //harvester harvesting arms animation fix
		hookAsJump (0x006F3620, 0x006F2BE0, WeaponBagClass::~WeaponBagClass); //weapon bag class fixes
		typedHookAsJump (0x006F3A40, 0x006F3000, (WeaponClass*(__thiscall WeaponBagClass::*)(WeaponDefinitionClass *, sint32, bool))&WeaponBagClass::Add_Weapon); //weapon bag class fixes
		hookAsJump (0x006F3990, 0x006F2F50, WeaponBagClass::Clear_Weapons); //weapon bag class fixes
		hookAsJump (0x006F4160, 0x006F3720, WeaponBagClass::Deselect); //weapon bag class fixes
		hookAsJump (0x006F4280, 0x006F3840, WeaponBagClass::Export_Weapon_List); //weapon bag class fixes
		hookAsJump (0x006F41B0, 0x006F3770, WeaponBagClass::Import_Weapon_List); //weapon bag class fixes
		hookAsJump (0x006F3930, 0x006F2EF0, WeaponBagClass::Remove_Weapon); //weapon bag class fixes
		hookAsJump (0x006F4100, 0x006F36C0, WeaponBagClass::Select_Index); //weapon bag class fixes
		hookAsJump (0x006F3CA0, 0x006F3260, WeaponBagClass::Select_Next); //weapon bag class fixes
		hookAsJump (0x006F3D30, 0x006F32F0, WeaponBagClass::Select_Prev); //weapon bag class fixes
		hookAsJump (0x006F4080, 0x006F3640, WeaponBagClass::Select_Weapon); //weapon bag class fixes
		hookAsJump (0x006F3ED0, 0x006F3490, WeaponBagClass::Select_Weapon_ID); //weapon bag class fixes
		hookAsJump (0x006F3F60, 0x006F3520, WeaponBagClass::Select_Weapon_Name); //weapon bag class fixes
		hookAsJump (0x006A2570, 0x006A1E10, SpawnManager::Get_Multiplayer_Spawn_Location); //randomize spawning points
		hookAsJump (0x0069EDD0, 0x0069E670, SmartGameObj::Export_Frequent); //send stealth state over the network
		hookAsJump (0x0069ED60, 0x0069E600, SmartGameObj::Import_Frequent); //send stealth state over the network
		hookAsCall (0x0060796A, 0x0060720A, Bluehell_Fix); //bluehell fix
		hookAsCall (0x00607B45, 0x006073E5, Bluehell_Fix); //bluehell fix
		hookAsCall (0x00607D25, 0x006075C5, Bluehell_Fix); //bluehell fix
		typedHookAsJump (0x006EF730, 0x006EECF0, (void (__thiscall BaseControllerClass::*)(int))&BaseControllerClass::Distribute_Funds_To_Each_Teammate); //fix distributing of cash
		hookAsJump (0x004BB530, 0x004BAD30, cWinEvent::Init); //join after server is gameovered fix
		hookAsJump (0x006EE210, 0x006ED7D0, VehicleFactoryGameObj::Destroy_Blocking_Objects); //weapons factory construction zone fix
		hookAsJump (0x00689EE0, 0x00689780, DefenseObjectClass::Apply_Damage); // Anti cheat: move damage calculation to server
		hookAsJump (0x004B99F0, 0x004B91F0, cScTextObj::Init); //chat message sent patch
		hookAsJump (0x004B6510, 0x004B5D10, cCsTextObj::Import_Creation); //chat message sent patch
		hookAsJump (0x004BA680, 0x004B9E80, cScTextObj::Export_Creation); //chat message sent patch
		hookAsJump (0x004B8B00, 0x004B8300, cPurchaseRequestEvent::Act); //fix you cant use a PT inside a vehicle
		hookAsJump (0x004B8B00, 0x004B8300, cPurchaseRequestEvent::Act); //fix you cant use a PT inside a vehicle
		hookAsJump (0x0070C3B0, 0x0070B970, C4GameObj::Think); //fix invisible C4 on vehicles and proximity C4 blowing up when placed next to dead enemy soldiers
		typedHookAsJump (0x00720C10, 0x007201D0, (void (*)(uint32,const Matrix3D&,ArmedGameObj*,const Vector3&,DamageableGameObj*))&ExplosionManager::Create_Explosion_At); //cli: faster speed on explosions
		hookAsJump (0x006C96D0, 0x006C8F70, SoldierGameObj::Apply_Control); //SoldierGameObj hooks
		hookAsJump (0x0069EF20, 0x0069E7C0, SmartGameObj::Apply_Control); //Stop the client from using weapons he doesnt own
		hookAsJump (0x0069EE10, 0x0069E6B0, SmartGameObj::Generate_Control); //obelisk walk
		hookAsJump (0x006A53D0, 0x006A4C70, PhysicalGameObj::Get_Bullseye_Position); //obelisk walk
		hookAsJump (0x006CE6E0, 0x006CDF80, SoldierGameObj::Get_Bullseye_Position); //obelisk walk
		hookAsJump (0x006C7100, 0x006C69A0, SoldierGameObj::Copy_Settings); //fix for humananimoverride issue
		hookAsJump (0x00406640, 0x00406640, cGod::Create_Player); //player join hook
		hookAsJump (0x006C8070, 0x006C7910, SoldierGameObj::Collision_Occurred); // unsquishable solider patch
		hookAsJump (0x0066E3F0, 0x0066DC90, File_Exists); //Fix memory leak in cMiscUtil::File_Exists 
		hookAsJump (0x0076A180, 0x00769A20, HarvesterClass::Detach); //Fix to not leak HarvesterClass objects
		hookAsJump (0x00428B50, 0x00428CD0, Print_Hook); //Fix printf issue
		hookAsJump (0x00425E20, 0x00425E20, Print_Hook); //Fix printf issue
		hookAsJump (0x006FAD80, 0x006FA340, WeaponClass::Select); //better scope code
		hookAsJump (0x006FADA0, 0x006FA360, WeaponClass::Deselect); //better scope code
		hookAsJump (0x0069E520, 0x0069DDC0, SmartGameObj::Init); //stealth code
		hookAsJump (0x0069E6F0, 0x0069DF90, SmartGameObj::Re_Init); //stealth code
		hookAsJump (0x0069F200, 0x0069EAA0, SmartGameObj::Think); //stealth code
		hookAsJump (0x0069EA20, 0x0069E2C0, SmartGameObj::Load); //stealth code
		hookAsJump (0x0069F530, 0x0069EDD0, SmartGameObj::Apply_Damage); //SmartGameObj hooks
				
		hookAsJump(0x004B6410, 0x004B5C10, cCsTextObj::Act); //chat message sent patch
		hookAsJump(0x0061C9C0, 0x0061C260, PacketManagerClass::Get_Packet); // UDP flood fix
		hookAsJump(0x006D97D0, 0x006D9070, DamageableGameObj::Set_Player_Type); // Send player type over network fix
		
		hookAsJump(0x00418D00, 0x00418D00, cPlayerManager::Load); //cPlayerManager hooks for player titles
		hookAsJump(0x00416270, 0x00416270, cPlayerManager::Add); //cPlayerManager hooks for player titles
		hookAsJump(0x00416300, 0x00416300, cPlayerManager::Remove); //cPlayerManager hooks for player titles
		
		hookAsJump(0x0061DA30, 0x0061D2D0, cRemoteHost::Init_Stats); //netcode fixups
		hookAsJump(0x0061E510, 0x0061DDB0, cRemoteHost::Adjust_Resend_Timeout); //netcode fixups

		//hookAsJump(0x00617FF0, 0x00617890, cConnection::cConnection);
		//hookAsJump(0x00618270, 0x00617B10, cConnection::~cConnection);
		hookAsJump(0x006183B0, 0x00617C50, cConnection::Init_Stats); //cConnection hooks
		hookAsJump(0x00618440, 0x00617CE0, cConnection::_Init_As_Client_1); //cConnection hooks
		hookAsJump(0x006185D0, 0x00617E70, cConnection::_Init_As_Client_2); //cConnection hooks
		hookAsJump(0x00618630, 0x00617ED0, cConnection::Init_As_Server); //cConnection hooks
		hookAsJump(0x006187D0, 0x00618070, cConnection::Bind); //cConnection hooks
		//hookAsJump(0x00618850, 0x006180F0, cConnection::Type_Translation);
		hookAsJump(0x006188C0, 0x00618160, cConnection::Sender_Id_Tests); //cConnection hooks
		hookAsJump(0x00618930, 0x006181D0, cConnection::Calculate_Packet_Bits); //cConnection hooks
		hookAsJump(0x00618960, 0x00618200, cConnection::Set_Packet_Loss); //cConnection hooks
		hookAsJump(0x00618990, 0x00618230, cConnection::Set_Packet_Duplication); //cConnection hooks
		hookAsJump(0x006189C0, 0x00618260, cConnection::Set_Packet_Latency_Range); //cConnection hooks
		hookAsJump(0x006189E0, 0x00618280, cConnection::Single_Player_recvfrom); //cConnection hooks
		hookAsJump(0x00618A60, 0x00618300, cConnection::Receive_Packet); //cConnection hooks
		hookAsJump(0x00618F80, 0x00618820, cConnection::Process_Connection_Request); //cConnection hooks
		hookAsJump(0x00619250, 0x00618AF0, cConnection::Single_Player_sendto); //cConnection hooks
		hookAsJump(0x00619350, 0x00618BF0, cConnection::Address_To_Rhostid); //cConnection hooks
		hookAsJump(0x006193B0, 0x00618C50, cConnection::Low_Level_Send_Wrapper); //cConnection hooks
		hookAsJump(0x00619510, 0x00618DB0, cConnection::_Send_Wrapper_1); //cConnection hooks
		hookAsJump(0x006196F0, 0x00618F90, cConnection::_Send_Wrapper_2); //cConnection hooks
		hookAsJump(0x006198E0, 0x00619180, cConnection::Low_Level_Receive_Wrapper); //cConnection hooks
		hookAsJump(0x00619A60, 0x00619300, cConnection::Receive_Wrapper); //cConnection hooks
		hookAsJump(0x00619B00, 0x006193A0, cConnection::Send_Packet_To_Address); //cConnection hooks
		hookAsJump(0x00619D20, 0x006195C0, cConnection::Set_R_And_U_Packet_Id); //cConnection hooks
		hookAsJump(0x00619D90, 0x00619630, cConnection::R_And_U_Send); //cConnection hooks
		hookAsJump(0x00619DD0, 0x00619670, cConnection::Send_Packet_To_Individual); //cConnection hooks
		hookAsJump(0x00619ED0, 0x00619770, cConnection::Is_Established); //cConnection hooks
		hookAsJump(0x00619F00, 0x006197A0, cConnection::Connect_Cs); //cConnection hooks
		hookAsJump(0x00619F50, 0x006197F0, cConnection::Send_Accept_Sc); //cConnection hooks
		hookAsJump(0x00619FF0, 0x00619890, cConnection::Send_Refusal_Sc); //cConnection hooks
		hookAsJump(0x0061A0A0, 0x00619940, cConnection::Send_Ack); //cConnection hooks
		hookAsJump(0x0061A190, 0x00619A30, cConnection::Destroy_Connection); //cConnection hooks
		hookAsJump(0x0061A1E0, 0x00619A80, cConnection::Send_Keepalives); //cConnection hooks
		hookAsJump(0x0061A360, 0x00619C00, cConnection::Get_Threshold_Priority); //cConnection hooks
		hookAsJump(0x0061A380, 0x00619C20, cConnection::Set_Max_Acceptable_Packetloss_Pc); //cConnection hooks
		hookAsJump(0x0061A3A0, 0x00619C40, cConnection::Demultiplex_R_Or_U_Packet); //cConnection hooks
		hookAsJump(0x0061A400, 0x00619CA0, cConnection::Service_Read); //cConnection hooks
		hookAsJump(0x0061A7B0, 0x0061A050, cConnection::Set_Bandwidth_Budget_Out); //cConnection hooks
		hookAsJump(0x0061A7F0, 0x0061A090, cConnection::Clear_Resend_Counts); //cConnection hooks
		hookAsJump(0x0061A840, 0x0061A0E0, cConnection::Get_Remote_Host); //cConnection hooks
		hookAsJump(0x0061A850, 0x0061A0F0, cConnection::Service_Send); //cConnection hooks
		hookAsJump(0x0061ACF0, 0x0061A590, cConnection::Install_Accept_Handler); //cConnection hooks
		hookAsJump(0x0061AD00, 0x0061A5A0, cConnection::Install_Refusal_Handler); //cConnection hooks
		hookAsJump(0x0061AD10, 0x0061A5B0, cConnection::Install_Server_Broken_Connection_Handler); //cConnection hooks
		hookAsJump(0x0061AD20, 0x0061A5C0, cConnection::Install_Client_Broken_Connection_Handler); //cConnection hooks
		hookAsJump(0x0061AD30, 0x0061A5D0, cConnection::Install_Eviction_Handler); //cConnection hooks
		hookAsJump(0x0061AD40, 0x0061A5E0, cConnection::Install_Conn_Handler); //cConnection hooks
		hookAsJump(0x0061AD50, 0x0061A5F0, cConnection::Install_Application_Acceptance_Handler); //cConnection hooks
		hookAsJump(0x0061AD60, 0x0061A600, cConnection::Install_Server_Packet_Handler); //cConnection hooks
		hookAsJump(0x0061AD70, 0x0061A610, cConnection::Install_Client_Packet_Handler); //cConnection hooks
		hookAsJump(0x0061AD80, 0x0061A620, cConnection::Set_Rhost_Is_In_Game); //cConnection hooks
		hookAsJump(0x0061ADE0, 0x0061A680, cConnection::Set_Rhost_Expect_Packet_Flood); //cConnection hooks
		hookAsJump(0x0061AE40, 0x0061A6E0, cConnection::Is_Time_To_Resend_Packet_To_Remote_Host); //cConnection hooks
		hookAsJump(0x0061AEF0, 0x0061A790, cConnection::Is_Packet_Too_Old); //cConnection hooks
		hookAsJump(0x0061AFA0, 0x0061A840, cConnection::Allow_Extra_Timeout_For_Loading); //cConnection hooks
		
		hookAsJump(0x0061FE60, 0x0061FA50, cPacket::Init_Encoder); //cPacket hooks
		hookAsJump(0x00456C60, 0x00456AC0, cNetwork::Cleanup_Client); //cNetwork hooks
		hookAsJump(0x00456D70, 0x00456AD0, cNetwork::Accept_Handler); //cNetwork hooks
		hookAsJump(0x00456E50, 0x00456AE0, cNetwork::Refusal_Handler); //cNetwork hooks
		//hookAsJump(0x00457450, 0x00456F00, cNetwork::Compute_Exe_Key);
		hookAsJump(0x00457700, 0x004571B0, cNetwork::Onetime_Init);
		//hookAsJump(0x00457740, 0x004571F0, cNetwork::Onetime_Shutdown);
		hookAsJump(0x00457790, 0x00457240, cNetwork::Init_Server); //cNetwork hooks
		hookAsJump(0x004579F0, 0x004574A0, cNetwork::Cleanup_Server); //cNetwork hooks
		//hookAsJump(0x00457A50, 0x00457500, cNetwork::Save);
		//hookAsJump(0x00457A80, 0x00457530, cNetwork::Load);
		hookAsJump(0x00457AC0, 0x00457570, cNetwork::Update_Fps); //cNetwork hooks
		hookAsJump(0x00457BB0, 0x00457660, cNetwork::Connection_Status_Change_Feedback); //cNetwork hooks
		hookAsJump(0x00457DB0, 0x00457860, cNetwork::Update); //cNetwork hooks
		hookAsJump(0x00457FE0, 0x00457A90, cNetwork::Client_Send_Packet); //cNetwork hooks
		hookAsJump(0x00458010, 0x00457AA0, cNetwork::Server_Send_Packet); //cNetwork hooks
		hookAsJump(0x00458080, 0x00457B10, cNetwork::Server_Send_Packet_To_All_Connected); //cNetwork hooks
		//hookAsJump(0x004580D0, 0x00457B60, cNetwork::Get_Client_Enumeration_String);
		hookAsJump(0x004581F0, 0x00457C80, cNetwork::Get_Server_Rhost); //cNetwork hooks
		hookAsJump(0x00458210, 0x00457CA0, cNetwork::Get_Client_Rhost); //cNetwork hooks
		hookAsJump(0x00458220, 0x00457CB0, cNetwork::Get_Server_Rhost_Threshold_Priority); //cNetwork hooks
		hookAsJump(0x00458240, 0x00457CD0, cNetwork::Get_Client_Rhost_Threshold_Priority); //cNetwork hooks
		hookAsJump(0x00458260, 0x00457CF0, cNetwork::Get_My_Id); //cNetwork hooks
		//hookAsJump(0x00458270, 0x00457D00, cNetwork::Get_Client_String);
		hookAsJump(0x004583F0, 0x00457E80, cNetwork::Server_Broken_Connection_Handler); //cNetwork hooks
		hookAsJump(0x00458550, 0x00457FE0, cNetwork::Client_Broken_Connection_Handler); //cNetwork hooks
		//hookAsJump(0x004585A0, 0x00458030, cNetwork::Process_Eviction_Sc);
		//hookAsJump(0x00458650, 0x004580E0, cNetwork::Eviction_Handler);
		hookAsJump(0x004586E0, 0x00458170, cNetwork::I_Am_God); //cNetwork hooks
		hookAsJump(0x00458710, 0x004581A0, cNetwork::Get_My_Player_Object); //cNetwork hooks
		hookAsJump(0x00458730, 0x004581C0, cNetwork::Get_My_Team_Number); //cNetwork hooks
		hookAsJump(0x00458990, 0x00458420, cNetwork::Get_My_Color); //cNetwork hooks
		hookAsJump(0x004589C0, 0x00458450, cNetwork::Show_Welcome_Message); //cNetwork hooks
		hookAsJump(0x00458A20, 0x004584B0, cNetwork::Get_Distance_Priority); //cNetwork hooks
		//hookAsJump(0x00458B20, 0x004585B0, cNetwork::Shell_Command);
		hookAsJump(0x00458B40, 0x004585D0, cNetwork::Application_Acceptance_Handler); //cNetwork hooks
		hookAsJump(0x00458D10, 0x004587A0, cNetwork::Connection_Handler); //cNetwork hooks
		//hookAsJump(0x00458D80, 0x00458810, cNetwork::Set_Desired_Frame_Sleep_Ms);
		//hookAsJump(0x00458D90, 0x00458820, cNetwork::Set_Simulated_Packet_Loss_Pc);
		//hookAsJump(0x00458DD0, 0x00458860, cNetwork::Set_Simulated_Packet_Duplication_Pc);
		//hookAsJump(0x00458E10, 0x004588A0, cNetwork::Set_Simulated_Latency_Range_Ms);
		//hookAsJump(0x00458E40, 0x004588D0, cNetwork::Set_Spam_Count);
		//hookAsJump(0x00458E50, 0x004588E0, cNetwork::Get_Simulated_Latency_Range_Ms);
		hookAsJump(0x00458E60, 0x004588F0, cNetwork::Flush); //cNetwork hooks
		//hookAsJump(0x00458EC0, 0x00458950, cNetwork::SwitchTeam);
		hookAsJump(0x00459150, 0x00458BE0, cNetwork::Enable_Waiting_Players); //cNetwork hooks
		//hookAsJump(0x00460750, 0x00460250, cNetwork::Tell_Client_About_Dynamic_Objects);
		hookAsJump(0x00461780, 0x00461280, cNetwork::Tell_Server_About_Dynamic_Objects); //cNetwork hooks
		hookAsJump(0x004617C0, 0x00461290, cNetwork::Tell_Client_About_Delete_Notifications); //cNetwork hooks
		hookAsJump(0x00461820, 0x004612F0, cNetwork::Send_Object_Update); //cNetwork hooks
		hookAsJump(0x00461AC0, 0x00461590, cNetwork::Intermission_Over_Processing); //cNetwork hooks
		hookAsJump(0x00461BD0, 0x004616A0, cNetwork::End_Game_Test); //cNetwork hooks
		hookAsJump(0x00461C90, 0x00461760, cNetwork::Shared_Client_And_Server_Think); //cNetwork hooks
		hookAsJump(0x00461CB0, 0x00461780, cNetwork::Client_Think); //cNetwork hooks
		//hookAsJump(0x00461E60, 0x00461790, cNetwork::Peek_Temp_Vis_Table);
		hookAsJump(0x00461F00, 0x00461830, cNetwork::Hibernation_Think); //cNetwork hooks
		hookAsJump(0x00462140, 0x00461A70, cNetwork::Server_Think); //cNetwork hooks
		hookAsJump(0x00462190, 0x00461AC0, cNetwork::Server_Kill_Connection); //cNetwork hooks
		hookAsJump(0x004621B0, 0x00461AE0, cNetwork::Delete_Player_Objects); //cNetwork hooks
		hookAsJump(0x00462200, 0x00461B30, cNetwork::Cleanup_After_Client); //cNetwork hooks
		hookAsJump(0x00462410, 0x00461D40, cNetwork::Remove_Player); //cNetwork hooks
		hookAsJump(0x00462640, 0x00461F70, cNetwork::Test_For_Team_Defaulting); //cNetwork hooks
		hookAsJump(0x0046E630, 0x0046DF60, cNetwork::Server_Packet_Handler); //cNetwork hooks
		hookAsJump(0x0046E7A0, 0x0046E0D0, cNetwork::Client_Packet_Handler); //cNetwork hooks

		hookAsJump(0x006720B0, 0x00671950, EmptyFunc); //nop out debug related functions
		hookAsJump(0x00672180, 0x00671A20, EmptyFunc); //nop out debug related functions
		hookAsJump(0x006722B0, 0x00671B50, EmptyFunc); //nop out debug related functions
		hookAsJump(0x006723E0, 0x00671C80, EmptyFunc); //nop out debug related functions
		hookAsJump(0x00672510, 0x00671DB0, EmptyFunc); //nop out debug related functions
		hookAsJump(0x00672650, 0x00671EF0, EmptyFunc); //nop out debug related functions
		hookAsJump(0x006728F0, 0x00672190, EmptyFunc); //nop out debug related functions
		hookAsJump(0x00672AB0, 0x00672350, EmptyFunc); //nop out debug related functions
		hookAsJump(0x00672B00, 0x006723A0, EmptyFunc); //nop out debug related functions
		hookAsJump(0x00777670, 0x00776F10, EmptyFunc); //nop out debug related functions
		hookAsJump(0x005E8650, 0x005E7EF0, EmptyFunc); //nop out debug related functions
		hookAsJump(0x0066E610, 0x0066DEB0, EmptyFunc); //nop out debug related functions

		if (cNetwork::Receiver)
		{
			delete cNetwork::Receiver;
			cNetwork::Receiver = cNetwork::NetworkReceiver = new CombatNetworkReceiverInstanceClass;
		}

		//hookAsJump(0x0061D840, 0x0061D0E0, cRemoteHost::cRemoteHost);
		hookAsJump(0x0061DA30, 0x0061D2D0, cRemoteHost::Init_Stats); //cRemoteHost hooks
		hookAsJump(0x0061DA70, 0x0061D310, cRemoteHost::Set_Last_Service_Count); //cRemoteHost hooks
		hookAsJump(0x0061DA80, 0x0061D320, cRemoteHost::Compute_List_Max); //cRemoteHost hooks
		hookAsJump(0x0061DAB0, 0x0061D350, cRemoteHost::Get_List_Max); //cRemoteHost hooks
		hookAsJump(0x0061DAC0, 0x0061D360, cRemoteHost::Set_List_Processing_Time); //cRemoteHost hooks
		hookAsJump(0x0061DAE0, 0x0061D380, cRemoteHost::Get_List_Processing_Time); //cRemoteHost hooks
		hookAsJump(0x0061DAF0, 0x0061D390, cRemoteHost::Get_Address); //cRemoteHost hooks
		//hookAsJump(0x0061DB00, 0x0061D3A0, cRemoteHost::~cRemoteHost);
		hookAsJump(0x0061DBD0, 0x0061D470, cRemoteHost::Add_Packet); //cRemoteHost hooks
		hookAsJump(0x0061DCD0, 0x0061D570, cRemoteHost::Remove_Packet); //cRemoteHost hooks
		hookAsJump(0x0061DE70, 0x0061D710, cRemoteHost::Toggle_Flow_Control); //cRemoteHost hooks
		hookAsJump(0x0061DEB0, 0x0061D750, cRemoteHost::Adjust_Flow_If_Necessary); //cRemoteHost hooks
		hookAsJump(0x0061E0A0, 0x0061D940, cRemoteHost::Is_Outgoing_Flooded); //cRemoteHost hooks
		hookAsJump(0x0061E1F0, 0x0061DA90, cRemoteHost::Dam_The_Flood); //cRemoteHost hooks
		hookAsJump(0x0061E3B0, 0x0061DC50, cRemoteHost::Set_Flood); //cRemoteHost hooks
		hookAsJump(0x0061E410, 0x0061DCB0, cRemoteHost::Set_Is_Loading); //cRemoteHost hooks
		hookAsJump(0x0061E490, 0x0061DD30, cRemoteHost::Was_Recently_Loading); //cRemoteHost hooks
		hookAsJump(0x0061E510, 0x0061DDB0, cRemoteHost::Adjust_Resend_Timeout); //cRemoteHost hooks

		//hookAsJump(0x0061B690, 0x0061AF30, PacketManagerClass::PacketManagerClass);
		hookAsJump(0x0061B940, 0x0061B1E0, PacketManagerClass::Set_Is_Server); //PacketMgrClass hooks
		//hookAsJump(0x0061BB30, 0x0061B3D0, PacketManagerClass::Build_Delta_Packet_Patch);
		//hookAsJump(0x0061BD90, 0x0061B630, PacketManagerClass::Reconstruct_From_Delta);
		hookAsJump(0x0061BFD0, 0x0061B870, PacketManagerClass::Get_Next_Free_Buffer_Index); //PacketMgrClass hooks
		hookAsJump(0x0061C020, 0x0061B8C0, PacketManagerClass::Take_Packet); //PacketMgrClass hooks
		hookAsJump(0x0061C130, 0x0061B9D0, PacketManagerClass::Flush); //PacketMgrClass hooks
		hookAsJump(0x0061C6D0, 0x0061BF70, PacketManagerClass::Disable_Optimizations); //PacketMgrClass hooks
		hookAsJump(0x0061C6F0, 0x0061BF90, PacketManagerClass::Break_Packet); //PacketMgrClass hooks
		//hookAsJump(0x0061C990, 0x0061C230, PacketManagerClass::Clear_Socket_Error);
		hookAsJump(0x0061C9C0, 0x0061C260, PacketManagerClass::Get_Packet); //PacketMgrClass hooks
		hookAsJump(0x0061CC20, 0x0061C4C0, PacketManagerClass::Reset_Stats); //PacketMgrClass hooks
		hookAsJump(0x0061CCB0, 0x0061C550, PacketManagerClass::Get_Stats_Index); //PacketMgrClass hooks
		hookAsJump(0x0061CD90, 0x0061C630, PacketManagerClass::Register_Packet_In); //PacketMgrClass hooks
		hookAsJump(0x0061CE80, 0x0061C720, PacketManagerClass::Register_Packet_Out); //PacketMgrClass hooks
		hookAsJump(0x0061CF70, 0x0061C810, PacketManagerClass::Update_Stats); //PacketMgrClass hooks
		hookAsJump(0x0061D190, 0x0061CA30, PacketManagerClass::Get_Total_Raw_Bandwidth_In); //PacketMgrClass hooks
		hookAsJump(0x0061D1A0, 0x0061CA40, PacketManagerClass::Get_Total_Raw_Bandwidth_Out); //PacketMgrClass hooks
		hookAsJump(0x0061D1B0, 0x0061CA50, PacketManagerClass::Get_Total_Compressed_Bandwidth_In); //PacketMgrClass hooks
		hookAsJump(0x0061D1C0, 0x0061CA60, PacketManagerClass::Get_Total_Compressed_Bandwidth_Out); //PacketMgrClass hooks
		hookAsJump(0x0061D1D0, 0x0061CA70, PacketManagerClass::Get_Raw_Bandwidth_In); //PacketMgrClass hooks
		hookAsJump(0x0061D220, 0x0061CAC0, PacketManagerClass::Get_Raw_Bandwidth_Out); //PacketMgrClass hooks
		hookAsJump(0x0061D270, 0x0061CB10, PacketManagerClass::Get_Raw_Bytes_Out); //PacketMgrClass hooks
		hookAsJump(0x0061D2C0, 0x0061CB60, PacketManagerClass::Get_Compressed_Bandwidth_In); //PacketMgrClass hooks
		hookAsJump(0x0061D310, 0x0061CBB0, PacketManagerClass::Get_Compressed_Bandwidth_Out); //PacketMgrClass hooks
		hookAsJump(0x0061D360, 0x0061CC00, PacketManagerClass::Set_Stats_Sampling_Frequency_Delay); //PacketMgrClass hooks
		hookAsJump(0x0061D400, 0x0061CCA0, PacketManagerClass::Get_Error_State); //PacketMgrClass hooks

		hookAsJump(0x0046F330, 0x0046EAA0, cSbboManager::Reset); //hook cSbboManager
		hookAsJump(0x0046F360, 0x0046EAD0, cSbboManager::Think); //hook cSbboManager
		hookAsJump(0x0046F460, 0x0046EBD0, cSbboManager::Increment_Accum_Time_S_Net_Update); //hook cSbboManager
		hookAsJump(0x0046F480, 0x0046EBF0, cSbboManager::Increment_Accum_Time_S_Combat_Think); //hook cSbboManager
		hookAsJump(0x0046F4A0, 0x0046EC10, cSbboManager::Get_Net_To_Combat_Ratio); //hook cSbboManager
		hookAsJump(0x0046F4B0, 0x0046EC20, cSbboManager::Toggle_Is_Enabled); //hook cSbboManager


		hookAsJump(0x0046D070, 0x0046C9A0, WOLNATInterfaceClass::Service_Receive_Queue); //WOL hooks
		hookAsJump(0x0046CE40, 0x0046C770, WOLNATInterfaceClass::Send_Game_Format_Packet_To); //WOL hooks

		hookAsJump(0x0061EB00, 0x0061E3A0, NetworkObjectClass::Set_Delete_Pending); //hook NetworkObjectClass
		
		hookAsJump(0x0071F7E0, 0x0071EDA0, ArmedGameObj::Export_Frequent); //turret lag fixes
		hookAsJump(0x0071F5F0, 0x0071EBB0, ArmedGameObj::Import_Frequent); //turret lag fixes
		hookAsJump(0x0071F920, 0x0071EEE0, ArmedGameObj::Export_State_Cs); //turret lag fixes
		hookAsJump(0x0071FA80, 0x0071F040, ArmedGameObj::Import_State_Cs); //turret lag fixes
		
		hookAsJump(0x00691820, 0x006910C0, DefenseObjectClass::Set_Health_Max); //send max health over the network
		hookAsJump(0x006930E0, 0x00692980, DefenseObjectClass::Set_Shield_Strength_Max); //send max health over the network
		hookAsJump(0x00693740, 0x00692FE0, DefenseObjectClass::Mark_Owner_Dirty); //send max health over the network
		
		hookAsJump(0x006C8390, 0x006C7C30, SoldierGameObj::Export_Frequent); //SoldierGameObj hooks
		hookAsJump(0x006C8570, 0x006C7E10, SoldierGameObj::Import_Frequent); //SoldierGameObj hooks
		hookAsJump(0x006C8350, 0x006C7BF0, SoldierGameObj::Export_Occasional); //SoldierGameObj hooks
		hookAsJump(0x006C8370, 0x006C7C10, SoldierGameObj::Import_Occasional); //SoldierGameObj hooks
		hookAsJump(0x006C8810, 0x006C80B0, SoldierGameObj::Import_State_Cs); //SoldierGameObj hooks
		hookAsJump(0x006C88A0, 0x006C8140, SoldierGameObj::Export_State_Cs); //SoldierGameObj hooks
		hookAsJump(0x006CFF50, 0x006CF7F0, SoldierGameObj::Is_Safe_To_Disable_Ghost_Collision); //SoldierGameObj hooks
		hookAsJump(0x006CA420, 0x006C9CC0, SoldierGameObj::Think); //SoldierGameObj hooks
		hookAsJump(0x006CD3F0, 0x006CCC90, SoldierGameObj::Apply_Damage_Extended); //SoldierGameObj hooks
		hookAsJump(0x006CBFF0, 0x006CB890, SoldierGameObj::Set_Blended_Animation); //SoldierGameObj hooks
		hookAsJump(0x006CC0E0, 0x006CB980, SoldierGameObj::Set_Animation); //SoldierGameObj hooks
		hookAsJump(0x006CD3D0, 0x006CCC70, SoldierGameObj::Apply_Damage); //SoldierGameObj hooks
		hookAsJump(0x006B6C40, 0x006B64E0, ScriptableGameObj::Start_Observers); //object create hooks
		hookAsJump(0x006B65F0, 0x006B5E90, ScriptableGameObj::Post_Re_Init); //object create hooks
		
		hookAsJump(0x0071FD00, 0x0071F2C0, ArmedGameObj::Init_Muzzle_Bones); //muzzle hook

		hookAsJump(0x0070C630, 0x0070BBF0, C4GameObj::Post_Think); //C4 hooks
		hookAsJump(0x0070B700, 0x0070ACC0, C4GameObj::Init_C4); //C4 hooks
		hookAsJump(0x0070BBA0, 0x0070B160, C4GameObj::Collision_Occurred); //C4 hooks
		hookAsJump(0x0070D6B0, 0x0070CC70, C4GameObj::Restore_Owner); //C4 hooks

		hookAsJump(0x0067A910, 0x0067A1B0, VehicleGameObj::Think); //VehicleGameObj hooks
		hookAsJump(0x0067C360, 0x0067BC00, VehicleGameObj::Apply_Damage); //VehicleGameObj hooks
		hookAsJump(0x00679690, 0x00678F30, VehicleGameObj::Import_Frequent); //VehicleGameObj hooks
		hookAsJump(0x00679970, 0x00679210, VehicleGameObj::Export_Frequent); //VehicleGameObj hooks
		hookAsJump(0x00679E50, 0x006796F0, VehicleGameObj::Update_Turret); //VehicleGameObj hooks
		hookAsJump(0x0067A060, 0x00679900, VehicleGameObj::Set_Targeting); //VehicleGameObj hooks
		hookAsJump(0x0067B460, 0x0067AD00, VehicleGameObj::Remove_Occupant); //VehicleGameObj hooks
		hookAsJump(0x0067AA30, 0x0067A2D0, VehicleGameObj::Post_Think); //VehicleGameObj hooks
		hookAsJump(0x0067C420, 0x0067BCC0, VehicleGameObj::Update_Damage_Meshes); //VehicleGameObj hooks
        typedHookAsJump(0x0067B2D0, 0x0067AB70, (void (__thiscall VehicleGameObj::*)(SoldierGameObj*,int))&VehicleGameObj::Add_Occupant); //VehicleGameObj hooks

		hookAsJump(0x00670BB0, 0x00670450, CombatManager::Think); //combat manager hooks
		hookAsJump(0x00670E30, 0x006706D0, CombatManager::Render); //combat manager hooks
		
		
		hookAsJump(0x00401680, 0x00401680, CombatGameModeClass::Load_Level); //level load hooks
		hookAsJump(0x004038D0, 0x004038D0, CombatGameModeClass::Render); //hook CombatGameModeClass::Render
		hookAsJump(0x004035D0, 0x004035D0, CombatGameModeClass::Think); //CombatGameModeClass hooks

		hookAsJump(0x004370D0, 0x00437170, GameModeManager::Render); //hook GameModeManager::Render
		hookAsJump(0x006EE0F0, 0x006ED6B0, VehicleFactoryGameObj::On_Generation_Complete); //hook for VehicleBuildingDisable stuff
		hookAsJump(0x006705E0, 0x0066FE80, Level_Loader_Thread); //hook for level loading
		hookAsJump(0x006EF460, 0x006EEA20, BaseControllerClass::Request_Harvester); //hook for VehicleBuildingDisable stuff
		hookAsJump(0x0070D160, 0x0070C720, C4GameObj::Import_Rare); //netcode hooks
		hookAsJump(0x006D9640, 0x006D8EE0, DamageableGameObj::Export_Occasional); //netcode hooks
		hookAsJump(0x0068D3B0, 0x0068CC50, DefenseObjectClass::Import); //netcode hooks
		hookAsJump(0x0068E0F0, 0x0068D990, DefenseObjectClass::Export); //netcode hooks
		hookAsJump(0x006ED560, 0x00726800, CClientControl::Import_Frequent); //netcode hooks
		hookAsJump(0x006ED4E0, 0x00726780, CClientControl::Export_Frequent); //netcode hooks
		hookAsJump(0x00684190, 0x00683A30, BuildingGameObj::Import_Rare); //netcode hooks
		hookAsJump(0x00684330, 0x00683BD0, BuildingGameObj::Export_Rare); //netcode hooks
		hookAsJump(0x007698A0, 0x00769140, cScExplosionEvent::Import_Creation); //netcode hooks
		hookAsJump(0x00769730, 0x00768FD0, cScExplosionEvent::Export_Creation); //netcode hooks
		hookAsJump(0x00679430, 0x00678CD0, VehicleGameObj::Import_Creation); //netcode hooks
		hookAsJump(0x00679380, 0x00678C20, VehicleGameObj::Export_Creation); //netcode hooks
		hookAsJump(0x0069FD30, 0x0069F5D0, SmartGameObj::Import_Creation); //netcode hooks
		hookAsJump(0x0069FCF0, 0x0069F590, SmartGameObj::Export_Creation); //netcode hooks
		hookAsJump(0x006A6460, 0x006A5D00, PhysicalGameObj::Import_Creation); //netcode hooks
		hookAsJump(0x006A62D0, 0x006A5B70, PhysicalGameObj::Export_Creation); //netcode hooks
		hookAsJump(0x006A6AE0, 0x006A6380, PhysicalGameObj::Import_Frequent); //netcode hooks
		hookAsJump(0x006A6A90, 0x006A6330, PhysicalGameObj::Export_Frequent); //netcode hooks
		hookAsJump(0x00684CF0, 0x00684590, BuildingGameObj::Import_Creation); //netcode hooks
		hookAsJump(0x006849E0, 0x00684280, BuildingGameObj::Export_Creation); //netcode hooks
		hookAsJump(0x00741C00, 0x007414A0, RepairBayGameObj::Import_Creation); //netcode hooks
		hookAsJump(0x00741940, 0x007411E0, RepairBayGameObj::Export_Creation); //netcode hooks
		hookAsJump(0x0071BCB0, 0x0071B270, DoorNetworkObjectClass::Import_Rare); //netcode hooks
		hookAsJump(0x0071BD80, 0x0071B340, DoorNetworkObjectClass::Export_Rare); //netcode hooks
		hookAsJump(0x006A5400, 0x006A4CA0, PhysicalGameObj::Apply_Damage); //PhysicalGameObj hooks

		hookAsJump(0x0073FE10, 0x0073F6B0, AirStripGameObj::Start_Cinematic); //hook for  airstrip drop off

		typedHookAsJump(0x004067D0, 0x004067D0, (SoldierGameObj* (*)(int, int))cGod::Create_Commando); //hook for player creation
		
		//hookAsJump(0x00471720, 0x00470E90, cGameData::Onetime_Init);
		//hookAsJump(0x004717A0, 0x00470F10, cGameData::Onetime_Shutdown);
		hookAsJump(0x00472120, 0x00471890, cGameData::Reset_Game); //cGameData hooks
		hookAsJump(0x004722A0, 0x00471A10, cGameData::Swap_Team_Sides); //cGameData hooks
		hookAsJump(0x004725F0, 0x00471D60, cGameData::Remix_Team_Sides); //cGameData hooks
		hookAsJump(0x00472700, 0x00471E70, cGameData::Rebalance_Team_Sides); //cGameData hooks
		//hookAsJump(0x00472860, 0x00471FD0, cGameData::Set_Ip_And_Port);
		//hookAsJump(0x004728E0, 0x004728E0, cGameData::Set_Generic_Num);
		hookAsJump(0x00472910, 0x00472080, cGameData::Set_Max_Players); //cGameData hooks
		//hookAsJump(0x00472930, 0x00472930, cGameData::Set_Time_Limit_Minutes);
		//hookAsJump(0x00472950, 0x00472950, cGameData::Set_Radar_Mode);
		//hookAsJump(0x00472960, 0x00472960, cGameData::Set_Intermission_Time_Seconds);
		//hookAsJump(0x00472970, 0x00472970, cGameData::Set_Motd);
		//hookAsJump(0x004729C0, 0x00472130, cGameData::Set_Mod_Name);
		//hookAsJump(0x00472A40, 0x004721B0, cGameData::Set_Map_Name);
		//hookAsJump(0x00472AB0, 0x00472220, cGameData::Set_Current_Players);
		//hookAsJump(0x00472AD0, 0x00472240, cGameData::Set_Owner);
		//hookAsJump(0x00472B30, 0x004722A0, cGameData::Set_Ip_Address);
		//hookAsJump(0x00472B40, 0x004722B0, cGameData::Set_Port);
		//hookAsJump(0x00472B50, 0x004722C0, cGameData::Is_Limited);
		hookAsJump(0x00472B60, 0x004722D0, cGameData::Is_Map_Valid); //cGameData hooks
		hookAsJump(0x00472BB0, 0x00472320, cGameData::Is_Valid_Settings); //cGameData hooks
		//hookAsJump(0x004734D0, 0x00472C40, cGameData::Export_Tier_1_Data);
		//hookAsJump(0x004735F0, 0x004735F0, cGameData::Import_Tier_1_Data);
		//hookAsJump(0x00473910, 0x00473910, cGameData::Does_Map_Exist);
		//hookAsJump(0x00473930, 0x00473930, cGameData::Import_Tier_1_Data);
		//hookAsJump(0x00473AF0, 0x00473260, cGameData::Export_Tier_2_Data);
		//hookAsJump(0x00473BB0, 0x00473BB0, cGameData::Import_Tier_2_Data);
		//hookAsJump(0x00473D30, 0x004734A0, cGameData::Load_From_Server_Config);
		//hookAsJump(0x004747A0, 0x004747A0, cGameData::Save_To_Server_Config);
		//hookAsJump(0x00474AA0, 0x00474170, cGameData::Soldier_Added);
		hookAsJump(0x00474B00, 0x004741D0, cGameData::Begin_Intermission); //cGameData hooks
		//hookAsJump(0x00474B80, 0x00474B80, cGameData::Set_Maximum_World_Distance);
		//hookAsJump(0x00474B90, 0x00474B90, cGameData::Set_Min_Qualifying_Time_Minutes);
		//hookAsJump(0x00474BA0, 0x00474BA0, cGameData::Get_Duration_Seconds);
		//hookAsJump(0x00474BD0, 0x00474BD0, cGameData::Get_Team_Word);
		//hookAsJump(0x00474CE0, 0x00474CE0, cGameData::Set_Time_Remaining_Seconds);
		//hookAsJump(0x00474CF0, 0x00474CF0, cGameData::Reset_Time_Remaining_Seconds);
		//hookAsJump(0x00474D10, 0x004743E0, cGameData::Get_Time_Remaining_Seconds);
		//hookAsJump(0x00474D20, 0x00474D20, cGameData::Is_Valid_Player_Type);
		//hookAsJump(0x00474D40, 0x00474D40, cGameData::Choose_Player_Type);
		//hookAsJump(0x00475030, 0x00475030, cGameData::Choose_Available_Team);
		//hookAsJump(0x00475080, 0x00474750, cGameData::Choose_Smallest_Team);
		//hookAsJump(0x00475120, 0x004747F0, cGameData::Is_Game_Over);
		//hookAsJump(0x004751B0, 0x00474880, cGameData::Has_Config_File_Changed);
		//hookAsJump(0x004751D0, 0x004748A0, cGameData::Get_Config_File_Mod_Time);
		hookAsJump(0x004752E0, 0x004749B0, cGameData::Game_Over_Processing); //cGameData hooks
		//hookAsJump(0x00475620, 0x00475620, cGameData::Is_Gameplay_Permitted);
		//hookAsJump(0x00475650, 0x00475650, cGameData::Set_Clan);
		//hookAsJump(0x00475670, 0x00475670, cGameData::Get_Clan);
		//hookAsJump(0x00475680, 0x00475680, cGameData::Clear_Clans);
		//hookAsJump(0x00475690, 0x00475690, cGameData::Find_Free_Clan_Slot);
		//hookAsJump(0x004756B0, 0x004756B0, cGameData::Is_Clan_Competing);
		//hookAsJump(0x004756E0, 0x004756E0, cGameData::Is_Clan_Game_Open);
		//hookAsJump(0x00475710, 0x004778A0, cGameData::Create_Game_Of_Type);
		//hookAsJump(0x00475730, 0x00475730, cGameData::Get_Game_Type_Name);
		//hookAsJump(0x004757D0, 0x004757D0, cGameData::Add_Bottom_Text);
		//hookAsJump(0x00475860, 0x00475860, cGameData::Show_Game_Settings_Limits);
		//hookAsJump(0x00475E80, 0x00475E80, cGameData::Get_Dedicated_Server_Label);
		//hookAsJump(0x00475F40, 0x00475F40, cGameData::Get_Gameplay_Not_Permitted_Label);
		//hookAsJump(0x00476000, 0x00476000, cGameData::Get_Time_Limit_Text);
		//hookAsJump(0x00476140, 0x00476140, cGameData::Think);
		//hookAsJump(0x00476280, 0x00476280, cGameData::Render);
		hookAsJump(0x004762A0, 0x00475970, cGameData::On_Game_Begin); //cGameData hooks
		hookAsJump(0x00476420, 0x00475BD0, cGameData::On_Game_End); //cGameData hooks
		//hookAsJump(0x00476530, 0x00475C00, cGameData::Get_Mission_Number_From_Map_Name);
		//hookAsJump(0x00476570, 0x00475C40, cGameData::Get_Map_Cycle);
		//hookAsJump(0x00476580, 0x00475C50, cGameData::Set_Map_Cycle);
		//hookAsJump(0x004765F0, 0x00475CC0, cGameData::Clear_Map_Cycle);
		//hookAsJump(0x00476620, 0x00475CF0, cGameData::Rotate_Map);
		//hookAsJump(0x004766C0, 0x004766C0, cGameData::Set_Win_Text);
		//hookAsJump(0x00476720, 0x00476720, cGameData::ReceiveSignal);
		//hookAsJump(0x00476740, 0x00476740, cGameData::Filter_Spawners);
		//hookAsJump(0x004767C0, 0x004767C0, cGameData::Set_Mvp_Name);
		//hookAsJump(0x00476820, 0x00476820, cGameData::Set_Mvp_Count);
		//hookAsJump(0x00476830, 0x00475F00, cGameData::Set_Win_Type);
		//hookAsJump(0x00476840, 0x00476840, cGameData::Set_Game_Duration_S);
		//hookAsJump(0x00476850, 0x00476850, cGameData::Get_Description);
		//hookAsJump(0x00471BC0, 0x00473D30, cGameData::Load_From_Server_Config);
		//hookAsJump(0x00471BD0, 0x004747A0, cGameData::Save_To_Server_Config);
		//hookAsJump(0x00471BE0, 0x00471BE0, cGameData::Is_Editable_Teaming);
		//hookAsJump(0x00471BF0, 0x00471BF0, cGameData::Is_Editable_Clan_Game);
		//hookAsJump(0x00471C00, 0x00471C00, cGameData::Is_Editable_Friendly_Fire);
		//hookAsJump(0x00471C10, 0x00471C10, cGameData::Is_Single_Player);
		//hookAsJump(0x00471C20, 0x00471C20, cGameData::Is_Skirmish);
		//hookAsJump(0x00471C30, 0x00471C30, cGameData::Is_Cnc);
		//hookAsJump(0x00471C40, 0x00471C40, cGameData::As_Single_Player);
		//hookAsJump(0x00471C50, 0x00471C50, cGameData::As_Skirmish);
		//hookAsJump(0x00471C60, 0x00471C60, cGameData::As_Cnc);
		//hookAsJump(0x00471C70, 0x00471C70, cGameData::Get_Min_Players);
		//hookAsJump(0x00471C80, 0x00471C80, cGameData::Remember_Inventory);
		
		hookAsJump(0x00402C00, 0x00402C00, LoadingScreenClass::Render); //hook for loadscreen
		
		hookAsJump(0x00716700, 0x00715CC0, BulletDataClass::Bullet_Collision_Occurred); //hook for bullet code
		hookAsJump(0x00701B20, 0x007010E0, WeaponClass::Update); //hook for weapon code
		hookAsJump(0x006FAE20, 0x006FA3E0, WeaponClass::Add_Rounds); //hook for weapon code
		hookAsCall(0x006F1258, 0x006F0818, WeaponClass::Add_Rounds_Client); //hook for weapon code
		hookAsJump(0x0064D900, 0x0064D1A0, TrackedVehicleClass::Render); //hook for tracked vehicle code

		hookAsJump(0x0069AF50, 0x0069A7F0, HumanStateClass::Set_State); //humanstateclass hooks
		hookAsJump(0x0069B2F0, 0x0069AB90, HumanStateClass::Update_Animation); //humanstateclass hooks
		hookAsJump(0x0069BB40, 0x0069B3E0, HumanStateClass::Update_State); //humanstateclass hooks
		hookAsJump(0x0069BF50, 0x0069B7F0, HumanStateClass::Post_Think); //humanstateclass hooks

		hookAsJump(0x006A9080, 0x006A8920, SurfaceEffectsManager::Init); //hooks for surfaceeffects.ini
		hookAsJump(0x006A95F0, 0x006A8E90, SurfaceEffectsManager::Shutdown); //hooks for surfaceeffects.ini
#ifdef HOVER_TEST
		hookAsJump(0x0064DEC0, 0x0064D760, TrackedVehicleClass::Compute_Force_And_Torque);
		hookAsJump(0x00654A50, 0x006542F0, RigidBodyClass::Compute_Derivatives);
		hookAsJump(0x00655960, 0x00655200, RigidBodyClass::Compute_Force_And_Torque);
#endif

		hookAsJump(0x004E60D0, 0x004E5970, ModPackageClass::Set_Package_Filename); //modpackage hooks
		hookAsJump(0x004E6260, 0x004E5B00, ModPackageClass::Build_Level_List); //modpackage hooks
		hookAsJump(0x004E64C0, 0x004E5D60, ModPackageClass::Find_Map_From_CRC); //modpackage hooks
		hookAsJump(0x004E65F0, 0x004E5E90, ModPackageClass::Compute_CRC); //modpackage hooks
		hookAsJump(0x004E66C0, 0x004E5F60, ModPackageClass::Get_CRC); //modpackage hooks
		hookAsJump(0x004E66E0, 0x004E5F80, ModPackageClass::Get_Map_Index); //modpackage hooks
		hookAsJump(0x00470900, 0x00470070, Purchase_Vendor_Item); //hook VendorClass::Purchase_Item
		hookAsJump(0x00470AC0, 0x00470230, VendorGrantSupplies); //hook VendorClass::Grant_Supplies

		hookAsJump(0x004E6880, 0x004E6120, ModPackageMgrClass::Initialize); //modpackage hooks
		hookAsJump(0x004E68B0, 0x004E6150, ModPackageMgrClass::Shutdown); //modpackage hooks
		hookAsJump(0x004E68E0, 0x004E6180, ModPackageMgrClass::Build_List); //modpackage hooks
		hookAsJump(0x004E6AA0, 0x004E6340, ModPackageMgrClass::Reset_List); //modpackage hooks
		typedHookAsJump(0x004E6AD0, 0x004E6370, (void (*)(const char*))&ModPackageMgrClass::Set_Current_Package); //modpackage hooks
		typedHookAsJump(0x004E6B30, 0x004E63D0, (void (*)(int))&ModPackageMgrClass::Set_Current_Package); //modpackage hooks
		hookAsJump(0x004E6BA0, 0x004E6440, ModPackageMgrClass::Find_Package); //modpackage hooks
		hookAsJump(0x004E6BF0, 0x004E6490, ModPackageMgrClass::Get_Mod_Map_Name_From_CRC_Index); //modpackage hooks
		hookAsJump(0x004E6E10, 0x004E66B0, ModPackageMgrClass::Get_Mod_Map_Name_From_CRC); //modpackage hooks
		hookAsJump(0x004E6F50, 0x004E67F0, ModPackageMgrClass::Find_Filename_From_CRC); //modpackage hooks
		hookAsJump(0x004E71A0, 0x004E6A40, ModPackageMgrClass::Find_Package_From_CRC); //modpackage hooks
		hookAsJump(0x004E7270, 0x004E6B10, ModPackageMgrClass::Load_Current_Mod); //modpackage hooks
		hookAsJump(0x004E72E0, 0x004E6B80, ModPackageMgrClass::Unload_Current_Mod); //modpackage hooks

		hookAsJump(0x00728A10, 0x007282B0, Set_Animation);//Scripts Set_Animation hook
		hookAsJump(0x0072ACC0, 0x0072A560, Apply_Damage);//Scripts Apply_Damage hook

		//hookAsJump(0x0047A4B0, 0x00479C40, cLanChat::cLanChat);
		//hookAsJump(0x0047A530, 0x00479CC0, cLanChat::~cLanChat);
		//hookAsJump(0x0047A550, 0x00479CE0, cLanChat::Load_Lan_Registry_Keys);
		//hookAsJump(0x0047A650, 0x00479DE0, cLanChat::Save_Lan_Registry_Keys);
		//hookAsJump(0x0047A720, 0x00479EB0, cLanChat::Init_Lan_Protocol_And_Socket);
		//hookAsJump(0x0047A760, 0x00479EF0, cLanChat::Accept_Actions);
		hookAsJump(0x0047A780, 0x00479F10, cLanChat::Refusal_Actions); //hook cLanChat
		hookAsJump(0x0047A790, 0x00479F20, cLanChat::Send_Position_Broadcast); //hook cLanChat
		//hookAsJump(0x0047A930, 0x0047A0C0, cLanChat::Process_Position_Broadcast);
		//hookAsJump(0x0047AAA0, 0x0047A230, cLanChat::Lan_Packet_Handler);
		//hookAsJump(0x0047AB20, 0x0047A2B0, cLanChat::Go_To_Location);
		hookAsJump(0x0047AB30, 0x0047A2C0, cLanChat::Think); //hook cLanChat

		//hookAsJump(0x00709600, 0x00708BC0, BeaconGameObj::Get_Factory);
		//hookAsJump(0x00709610, 0x00708BD0, BeaconGameObj::Init);
		//hookAsJump(0x00709640, 0x00708C00, BeaconGameObj::Init);
		hookAsJump(0x00709680, 0x00708C40, BeaconGameObj::Init_Beacon); //beacon hooks
		//hookAsJump(0x00709700, 0x00708CC0, BeaconGameObj::Save);
		//hookAsJump(0x00709850, 0x00708E10, BeaconGameObj::Load);
		//hookAsJump(0x00709960, 0x00708F20, BeaconGameObj::Load_Variables);
		//hookAsJump(0x007099F0, 0x00708FB0, BeaconGameObj::Think);
		//hookAsJump(0x00709B90, 0x00709150, BeaconGameObj::Get_Information);
		hookAsJump(0x00709BA0, 0x00709160, BeaconGameObj::Start_Cinematic); //beacon hooks
		hookAsJump(0x00709C00, 0x007091C0, BeaconGameObj::Stop_Armed_Sound); //beacon hooks
		hookAsJump(0x00709C40, 0x00709200, BeaconGameObj::Set_State); //beacon hooks
		hookAsJump(0x0070A1C0, 0x00709780, BeaconGameObj::Update_State); //beacon hooks
		hookAsJump(0x0070A4A0, 0x00709A60, BeaconGameObj::Get_Enemy_Base); //beacon hooks
		hookAsJump(0x0070A4C0, 0x00709A80, BeaconGameObj::Can_Place_Here); //beacon hooks
		hookAsJump(0x0070A4D0, 0x00709A90, BeaconGameObj::Is_In_Enemy_Base); //beacon hooks
		hookAsJump(0x0070A530, 0x00709AF0, BeaconGameObj::Stop_Current_Message_Sound); //beacon hooks
		//hookAsJump(0x0070A570, 0x00709B30, BeaconGameObj::Display_Message);
		hookAsJump(0x0070A760, 0x00709D20, BeaconGameObj::Begin_Arming); //beacon hooks
		//hookAsJump(0x0070A770, 0x00709D30, BeaconGameObj::Start_Owner_Animation);
		//hookAsJump(0x0070A850, 0x00709E10, BeaconGameObj::Stop_Owner_Animation);
		//hookAsJump(0x0070A950, 0x00709F10, BeaconGameObj::Was_Owner_Interrupted);
		hookAsJump(0x0070AA50, 0x0070A010, BeaconGameObj::Get_Owner); //beacon hooks
		hookAsJump(0x0070AA90, 0x0070A050, BeaconGameObj::Completely_Damaged); //beacon hooks
		//hookAsJump(0x0070AAC0, 0x0070A080, BeaconGameObj::Create_Explosion);
		hookAsJump(0x0070AD50, 0x0070A310, BeaconGameObj::Export_Rare); //beacon hooks
		hookAsJump(0x0070AE80, 0x0070A440, BeaconGameObj::Import_Rare); //beacon hooks
		//hookAsJump(0x0070AEE0, 0x0070A4A0, BeaconGameObj::Restore_Owner);

		hookAsJump(0x004B4B00, 0x004B4300, cBioEvent::Init); //cBioEvent hooks
		hookAsJump(0x004B4C00, 0x004B4400, cBioEvent::Act); //cBioEvent hooks
		hookAsJump(0x004B4E50, 0x004B4650, cBioEvent::Export_Creation); //cBioEvent hooks
		hookAsJump(0x004B4EF0, 0x004B46F0, cBioEvent::Import_Creation); //cBioEvent hooks
		hookAsJump(0x00713C20, 0x007131E0, WeatherMgrClass::Init); //weather hooks
		hookAsJump(0x00713DD0, 0x00713390, WeatherMgrClass::Shutdown); //weather hooks
		typedHookAsJump(0x00713E00, 0x007133C0,(bool (*) (float heading, float speed, float variability, float ramptime))&WeatherMgrClass::Set_Wind); //weather hooks
		typedHookAsJump(0x00714180, 0x00713740,(bool (*) (WeatherMgrClass::PrecipitationEnum precipitation, float density, float ramptime))&WeatherMgrClass::Set_Precipitation); //weather hooks
		hookAsJump(0x00714D70, 0x00714330, WeatherMgrClass::Save_Dynamic); //weather hooks
		hookAsJump(0x00715430, 0x007149F0, WeatherMgrClass::Load_Dynamic); //weather hooks
		hookAsJump(0x006E9150, 0x006E89F0, BackgroundMgrClass::Init); //weather hooks
		hookAsJump(0x006E92D0, 0x006E8B70, BackgroundMgrClass::Shutdown); //weather hooks
		typedHookAsJump(0x006E9340, 0x006E8BE0, (bool (*) (float cloudcover, float cloudgloominess, float ramptime))&BackgroundMgrClass::Set_Clouds); //weather hooks
		typedHookAsJump(0x006E9820, 0x006E90C0, (bool (*) (float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime))&BackgroundMgrClass::Set_Lightning); //weather hooks
		hookAsJump(0x006EAA40, 0x006EA240, BackgroundMgrClass::Save_Dynamic); //weather hooks
		hookAsJump(0x006EB530, 0x006EAD50, BackgroundMgrClass::Load_Dynamic); //weather hooks

		hookAsJump(0x00460220, 0x0045FCB0, cDiagnostics::Render);

		hookAsJump(0x007434A0, 0x00742D40, RefineryGameObj::Think);

		WIN32_FIND_DATAA data;
		HANDLE handle = ::FindFirstFileA ("data/*.mix", &data);
		if (handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!stristr(data.cFileName,mapprefix))
				{
					RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass(data.cFileName,RenegadeBaseFileFactory), data.cFileName);
				}
			}
			while (::FindNextFileA (handle, &data));
			::FindClose (handle);
		}
		RenegadeFileFactory.Add_FileFactory(new PackageManager(), "tt.vfs");
		if (PathFileExists("data/C&C_Canyon.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Canyon.mix",RenegadeBaseFileFactory),"C&C_Canyon.mix");
		if (PathFileExists("data/C&C_City.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_City.mix",RenegadeBaseFileFactory),"C&C_City.mix");
		if (PathFileExists("data/C&C_City_Flying.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_City_Flying.mix",RenegadeBaseFileFactory),"C&C_City_Flying.mix");
		if (PathFileExists("data/C&C_Complex.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Complex.mix",RenegadeBaseFileFactory),"C&C_Complex.mix");
		if (PathFileExists("data/C&C_Field.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Field.mix",RenegadeBaseFileFactory),"C&C_Field.mix");
		if (PathFileExists("data/C&C_Glacier_Flying.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Glacier_Flying.mix",RenegadeBaseFileFactory),"C&C_Glacier_Flying.mix");
		if (PathFileExists("data/C&C_Hourglass.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Hourglass.mix",RenegadeBaseFileFactory),"C&C_Hourglass.mix");
		if (PathFileExists("data/C&C_Islands.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Islands.mix",RenegadeBaseFileFactory),"C&C_Islands.mix");
		if (PathFileExists("data/C&C_Mesa.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Mesa.mix",RenegadeBaseFileFactory),"C&C_Mesa.mix");
		if (PathFileExists("data/C&C_Under.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Under.mix",RenegadeBaseFileFactory),"C&C_Under.mix");
		if (PathFileExists("data/C&C_Volcano.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Volcano.mix",RenegadeBaseFileFactory),"C&C_Volcano.mix");
		if (PathFileExists("data/C&C_Walls.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Walls.mix",RenegadeBaseFileFactory),"C&C_Walls.mix");
		if (PathFileExists("data/C&C_Walls_Flying.mix"))
			RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("C&C_Walls_Flying.mix",RenegadeBaseFileFactory),"C&C_Walls_Flying.mix");
		const unsigned char code1[3] = {0x04,0x0F,0x8C};
		const unsigned char code4[2] = {0x04,0x7D};
		const unsigned char code5[3] = {0x04,0x0F,0x8C};
		const unsigned char code6[2] = {0x04,0x7C};
		const unsigned char code7[2] = {0x04,0x7C};
		const unsigned char code8[2] = {0x90,0xE9};
		WriteMemory(UDP1,code1,sizeof(code1)); //UDP fixes
		WriteMemory(UDP4,code4,sizeof(code4)); //UDP fixes
		WriteMemory(UDP5,code5,sizeof(code5)); //UDP fixes
		WriteMemory(UDP6,code6,sizeof(code6)); //UDP fixes
		WriteMemory(UDP7,code7,sizeof(code7)); //UDP fixes
		WriteMemory(UDP8,code8,sizeof(code8)); //UDP fixes
		if (VehicleOwnershipDisable)
		{
			hookAsCall(0x0073F22D,0x0073EACD,VehicleOwnershipPatch); //vehicle ownership disable
		}
		const unsigned char sbbuf[5] = {0xE9,0xA2,0x00,0x00,0x00};
		WriteMemory(StartBugFix,sbbuf,sizeof(sbbuf)); //start button bug fix
		hookAsCall(0x00678463,0x00677D03,VehicleGameObj::ConInit); //vehicle game obj visible change
		hookVtable(0x007E2988,0x007E1970,VehicleGameObj::Is_Visible); //vehicle game obj visible change
		unsigned int sp = 0x3F800000;
		WriteMemory(SlidePrecision,&sp,4); //wall lag fix
		hookAsJump(0x006A1620,0x006A0EC0,SpawnerClass::Determine_Spawn_TM); //spawner randomness fix
		hookAsJump(0x00435B00,0x00435BA0,ConsoleModeClass::Print); //console hooks
		hookAsJump(0x00435C70,0x00435D10,ConsoleModeClass::Print_Maybe); //console hooks
		hookAsJump(0x00436550,0x004365F0,ConsoleModeClass::Add_Message); //console hooks
		hookAsJump(0x00409880,0x00409880,InputConfigMgrClass::Delete_Configuration); //input hooks
		InitConsole();
		srand((unsigned int)time(NULL) + GetCurrentProcessId());

	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		if(!Exe)
		{
			DestroyFavorites();
		}
		FreeKeyboardConfig();
		FreeScopeConfig();
		WSACleanup();
		delete[] MapTexture;
		delete[] mapprefix;
		if (ssurl)
		{
			delete[] ssurl;
		}
	}
	return TRUE;
}
