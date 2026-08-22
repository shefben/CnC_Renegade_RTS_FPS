/*	Renegade tt.dll
	Global header file
	Copyright 2009 Jonathan Wilson

	This file is part of the Renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/

#ifndef TT_INCLUDE__TT_H
#define TT_INCLUDE__TT_H

#include "scripts.h"
#include "cNetEvent.h"
#include "DX8Caps.h"
#include "engine_ttdef.h"
class cPlayer;
class SoldierGameObj;
class VehicleGameObj;
// forward declarations

struct Scope {
	char *WeaponPreset;
	char *ScopeTexture;
	float MaxZoom;
	float MinZoom;
};

enum PURCHASE_TYPE {
	PT_CHARACTER = 0,
	PT_VEHICLE = 1,
	PT_ENLISTED = 2,
	PT_BEACON = 3,
	PT_REFILL = 4,
	PT_SECRETCHAR = 5,
	PT_SECRETVEHICLE = 6
};

#define SS_STEALTH 1
#define SS_FOGENABLE 2
#define SS_FOGRANGE 3
#define SS_WARBLITZ 4
#define SS_FADEBGM 5
#define SS_SETBGM 6
#define SS_STOPBGM 7
#define SS_CREATESOUND 8
#define SS_CREATE2DSOUND 9
#define SS_CREATE2DWAVSOUND 10
#define SS_CREATE3DWAVSOUND 11
#define SS_CREATE3DSOUND 12
#define SS_PLAYANNOUNCEMENT 13
#define SS_RADAR 14
#define SS_GDITERMINAL 15
#define SS_NODTERMINAL 16
#define SS_SETSFC 17
#define SS_SETSFO 18
#define SS_VERSION 19
#define SS_CLEARWEAPONS 20
#define SS_VEHICLETRANSITIONS 21
#define SS_DISPLAYCOLOR 23
#define SS_DISPLAYTEXT 24
#define SS_DISPLAYFLOAT 25
#define SS_DISPLAYINT 26
#define SS_SELECTWEAPON 27
#define SS_FORCELOOK 28
#define SS_ENABLEHUD 29
#define SS_SHAKECAMERA 30
#define SS_WEAPONSEND 31
#define SS_POWERUPSOUND 35
#define SS_ICONS 36
#define SS_BLIPSHAPE 39
#define SS_BLIPCOLOR 40
#define SS_INFOCHANGE 42
#define SS_INFODISABLE 43
#define SS_ADDMESSAGE 44
#define SS_VEHICLELIMIT 45
#define SS_HEALTHBAR 46
#define SS_WIREFRAME 47
#define SS_DISABLEALLCOLLISIONS 48
#define SS_DISABLEPHYSCOLLISIONS 49
#define SS_ENABLECOLLISIONS 50
#define SS_HUDINI 51
#define SS_REMOVEWEAPON 52
#define SS_TEAMPURCHASE 53
#define SS_PURCHASE1 54
#define SS_PURCHASE2 55
#define SS_RADARMAP 56
#define SS_CURRENTLYBUILDING 57
#define SS_FOGCOLOR 60
#define SS_FOGMODE 61
#define SS_FOGDENSITY 62
#define SS_CAMERAHOST 63
#define SS_TIMEREMAINING 64
#define SS_TIMELIMIT 65
#define SS_GDISIDEBAR 66
#define SS_NODSIDEBAR 67
#define SS_PTSECURITY 68
#define SS_HUDNUMBER 70
#define SS_MAPCHECK 71
#define SS_CONTROLENABLE 72
#define SS_TEXTFILE 74
#define SS_SERVERVERSION 75
#define SS_SCREENSHOT 76
#define SS_SENDPARAM 77
#define SS_SENDPARAMOBJ 78
#define SS_GOTOFRAME 79
#define SS_GOTOLASTFRAME 80
#define SS_FOGOVERRIDE 81
#define SS_CLEARFOGOVERRIDE 82

void SendTextSc(const char *string,TextMessageEnum mtype,bool amsg,int sender,int target);
const char *Get_Player_Name_By_Data(cPlayer *x);
int Get_Player_ID_By_Data(cPlayer *x);
cPlayer *FindPlayer(int id);
void Fade_Background_Music(const char *Music,int FadeOut,int FadeIn);
void Set_Background_Music(const char *Music);
void Stop_Background_Music();
int Create_Sound(const char *Sound,const Vector3 &Position,GameObject *Obj);
int Create_2D_WAV_Sound(const char *Sound);
int Create_2D_Sound(const char *Sound);
int Create_3D_WAV_Sound_At_Bone(const char *Sound,GameObject *Obj,const char *Bone);
int Create_3D_Sound_At_Bone(const char *Sound,GameObject *Obj,const char *Bone);
void Static_Anim_Phys_Goto_Frame(int ObjectID,float Frame,const char *Anim);
void Static_Anim_Phys_Goto_Last_Frame(int ObjectID,const char *Anim);
void Control_Enable(GameObject *Obj,bool Enable);
void Disable_All_Collisions(GameObject *Obj);
void Disable_Physical_Collisions(GameObject *Obj);
void Enable_Collisions(GameObject *Obj);
void Display_Health_Bar(GameObject *Obj,bool Display);
void Set_Obj_Radar_Blip_Shape(GameObject *Obj,int Shape);
void Set_Obj_Radar_Blip_Color(GameObject *Obj,int Color);
void Set_Model(GameObject *Obj,const char *Model);
void Set_Display_Color(unsigned char Red,unsigned char Green,unsigned char Blue);
void Display_Text(int StringID);
void Display_Float(float Number,const char *Message);
void Display_Int(int Number,const char *Message);
void Enable_Vehicle_Transitions(GameObject *Obj,bool Enable);
void Set_Player_Type(GameObject *Obj,int PlayerType);
void Shake_Camera(const Vector3 &Pos,float Radius,float Intensity,float Time);
void Force_Camera_Look(const Vector3 &Look);
void Enable_HUD(bool Enable);
void Select_Weapon(GameObject *Obj,const char *Weapon);
void Enable_Radar(bool Enable);
void Display_GDI_Player_Terminal();
void Display_NOD_Player_Terminal();
void Set_Screen_Fade_Color(float Red,float Green,float Blue,float Transition);
void Set_Screen_Fade_Opacity(float Opacity,float Transition);
void Clear_Weapons(GameObject *Obj);
void Play_Building_Announcement(GameObject *Obj,int AnnouncementID);
void Set_Fog_Enable(bool Enable);
void Set_Fog_Range(float StartDistance,float EndDistance,float Transition);
void Set_War_Blitz(float Intensity,float StartDistance,float EndDistance,float Heading,float Distribution,float Transition);
void Enable_Stealth(GameObject *Obj,bool Stealth);
void Set_Bandwidth(int PlayerID,int bandwidth);
void AddCombatMessage(const char *message, unsigned int red, unsigned int green, unsigned int blue);
void Change_Time_Remaining(float time);
void Change_Time_Limit(int time);
void Key_Is_Pressed(char *LogicalKey,int Player_ID);
void ShaderNotifyDo(int ID,int notify,int Player_ID);
void Send_PT_Data_Player(int PlayerID);
void ReadHUDIniBits(INIClass *hudini);
void ReadHUDIniBits2(INIClass *hudini);
void HUDInit();
void Do_Soldier_Update(SoldierGameObj *obj);
void ClearInfoTexture();
void SetInfoTexture(const char *texture);
void Remove_Weap(GameObject *obj,const char *weapon);
void Send_PT_Data();
void Set_Fog_Density(float density);
void SetCurrentlyBuilding(bool building,int team);
void Set_Fog_Color(unsigned int red, unsigned int green, unsigned int blue);
void Set_Fog_Mode(unsigned int mode);
void SendTextCs(const char *string,TextMessageEnum mtype,int sender,int target);
void Make_Remote_Screen_Shot(const char* url);
void InitInfoTexture();
void Do_Poke(GameObject *poked,GameObject *poker);
void DoGameOver2();
WideStringClass *CharToWSC(const char *string);
void Do_Vehicle_Observer(VehicleGameObj *obj);
void DoCombatKeyboard();
void Make_Screen_Shot(const char* baseFilename);
void KeyboardUpdate();
void HUDIniRead();
void ReadKeyboardConfig();
void ReadScopeConfig();
void FreeKeyboardConfig();
void FreeScopeConfig();
unsigned int Purchase_Powerup(BaseControllerClass *Base,GameObject *Purchaser,unsigned int cost,unsigned int preset);
void Get_Merchandise_Information(GameObject *owner, PURCHASE_TYPE type, int position, int alternate, int &cost, int &presetid);
unsigned int Purchase_Vehicle(BaseControllerClass *Base,GameObject *Purchaser,unsigned int cost,unsigned int preset);
unsigned int Purchase_Character(BaseControllerClass *Base,GameObject *Purchaser,unsigned int cost,unsigned int preset);
void Print_Hook(char* format, ...);
bool File_Exists(const char* name);
void Printf_Hook(const char *str);
void Cleanup_HUD();
void RenderGameData();
void QuickMatchDisable();
void DoUnloadLevel();
void TeamMenuFrameUpdatePatch();
void PlayersMenuFrameUpdatePatch();
void OnPtChatListUpdatePatch();
void OnPtChatListInitPatch();
void OnUnitPtKeyDownPatch();
void OnUnitPtMerchandiseItemUpdatePatch();
void DoPlayerJoin2(cPlayer *playerdata);
void DoLoadLevel2();
extern char *CurrentSong;
extern HostHook Host_Hook;
extern bool ClientChatLog;
extern char *ssurl;
extern bool hookon;
extern SimpleDynVecClass<KeyHookStruct *> KeyHooks;
extern SimpleDynVecClass<ShaderNotifyStruct *> ShaderNotifies;
extern SimpleDynVecClass<ObjectCreateHookStruct *> ObjectCreateHooks;
extern ChatHook Chat_Hook;
extern PlayerJoin Player_Join_Hook;
extern PlayerLeave Player_Leave_Hook;
extern TTVersion Version_Hook;
extern LoadLevelHook Load_Level_Hook;
extern LoadLevelHook Think_Hook;
extern LoadLevelHook Game_Over_Hook;
extern SimpleDynVecClass<PurchaseHook> Vehicle_Purchase_Hooks;
extern SimpleDynVecClass<PurchaseHook> Powerup_Purchase_Hooks;
extern SimpleDynVecClass<PurchaseHook> Character_Purchase_Hooks;
extern SimpleDynVecClass<PurchaseMonHook> Vehicle_Purchase_Monitor_Hooks;
extern SimpleDynVecClass<PurchaseMonHook> Powerup_Purchase_Monitor_Hooks;
extern SimpleDynVecClass<PurchaseMonHook> Character_Purchase_Monitor_Hooks;
extern SimpleDynVecClass<const char *> Vehicle_Purchase_Hooks_Data;
extern SimpleDynVecClass<const char *> Powerup_Purchase_Hooks_Data;
extern SimpleDynVecClass<const char *> Character_Purchase_Hooks_Data;
extern SimpleDynVecClass<const char *> Vehicle_Purchase_Monitor_Hooks_Data;
extern SimpleDynVecClass<const char *> Powerup_Purchase_Monitor_Hooks_Data;
extern SimpleDynVecClass<const char *> Character_Purchase_Monitor_Hooks_Data;
extern int WireframeMode;
extern bool PTDataChanged;
extern float BuildTimeDelay;
extern float MapScale;
extern char *MapTexture;
extern Vector2 MapOffset;
extern bool SendRadarData;
extern bool CurrentlyBuildingTeam[2];
extern ConsoleOutputHook Console_Output_Hook;
extern DataHook Data_Hook;
extern GameObject *ExplosionObj;
extern char *GDISoldierName;
extern char *NodSoldierName;
extern bool CurrentlyBuilding;
extern bool BuildingHarvester[2];
extern bool DisableCostMultiplier;
extern char *mapprefix;
extern bool VehicleOwnershipDisable;
extern float ServerVersion;
extern SimpleDynVecClass<Scope *> ScopeList;
extern REF_DECL1(CurrentCaps, DX8Caps*);
#endif
