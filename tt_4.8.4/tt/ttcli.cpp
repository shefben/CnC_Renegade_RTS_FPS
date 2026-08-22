/*	Renegade tt.dll
	Win32 client message handler and related code
	Copyright 2009 Jonathan Wilson

	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"

#include "tt.h"
#include "VehicleGameObj.h"
#include "WWAudioClass.h"
#include "SoldierGameObj.h"
#include "shaders.h"
#include "sidebar.h"
#include "cCsTextObj.h"
#include "cNetwork.h"
#include "ScreenshotManager.h"
#include "cScTextObj.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "objdlg.h"
#include "engine_obj.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "engine_game.h"
#include "TeamPurchaseSettingsDefClass.h"
#include "PurchaseSettingsDefClass.h"
#include "engine_player.h"

extern bool& GameInFocus;
extern bool ScrollingMap;
bool ptupdate = false;


extern REF_DECL2(VehicleCount,int);
void Display_Dialog(int ID);
extern void *Add_Weapon;
void Do_Data_Process(char *str)
{
	char *id = strtok(str,"\n");
	if (id[0] == 'j')
	{
		int cmd = atoi(strtok(NULL,"\n"));
		switch (cmd)
		{
		case SS_STEALTH: //Enable_Stealth send over network
			{
				int ID = atoi(strtok(NULL,"\n"));
				bool enable = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Enable_Stealth(obj,enable);
				}
			}
			break;
		case SS_FOGENABLE: //Set_Fog_Enable send over network
			{
				bool enable = atoi(strtok(NULL,"\n"));
				Set_Fog_Enable(enable);
			}
			break;
		case SS_FOGRANGE: //Set_Fog_Range send over network
			{
				float start_distance = (float)atof(strtok(NULL,"\n"));
				float end_distance = (float)atof(strtok(NULL,"\n"));
				float transition = (float)atof(strtok(NULL,"\n"));
				Set_Fog_Range(start_distance,end_distance,transition);
			}
			break;
		case SS_WARBLITZ: //Set_War_Blitz send over network
			{
				float intensity = (float)atof(strtok(NULL,"\n"));
				float start_distance = (float)atof(strtok(NULL,"\n"));
				float end_distance = (float)atof(strtok(NULL,"\n"));
				float heading = (float)atof(strtok(NULL,"\n"));
				float distribution = (float)atof(strtok(NULL,"\n"));
				float transition = (float)atof(strtok(NULL,"\n"));
				Set_War_Blitz(intensity,start_distance,end_distance,heading,distribution,transition);
			}
			break;
		case SS_FADEBGM: //Fade_Background_Music send over network
			{
				char *music = strtok(NULL,"\n");
				int unk1 = atoi(strtok(NULL,"\n"));
				int unk2 = atoi(strtok(NULL,"\n"));
				Fade_Background_Music(music,unk1,unk2);
			}
			break;
		case SS_SETBGM: //Set_Background_Music send over network
			{
				char *music = strtok(NULL,"\n");
				Set_Background_Music(music);
			}
			break;
		case SS_STOPBGM: //Stop_Background_Music send over network
			{
				Stop_Background_Music();
			}
			break;
		case SS_CREATESOUND: //Create_Sound send over network
			{
				char *soundname = strtok(NULL,"\n");
				Vector3 pos;
				pos.X = (float)atof(strtok(NULL,"\n"));
				pos.Y = (float)atof(strtok(NULL,"\n"));
				pos.Z = (float)atof(strtok(NULL,"\n"));
				int ID = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Create_Sound(soundname,pos,obj);
				}
			}
			break;
		case SS_CREATE2DSOUND: //Create_2D_Sound send over network
			{
				char *soundname = strtok(NULL,"\n");
				Create_2D_Sound(soundname);
			}
			break;
		case SS_CREATE2DWAVSOUND: //Create_2D_WAV_Sound send over network
			{
				char *soundname = strtok(NULL,"\n");
				Create_2D_WAV_Sound(soundname);
			}
			break;
		case SS_CREATE3DWAVSOUND: //Create_3D_WAV_Sound_At_Bone send over network
			{
				char *soundname = strtok(NULL,"\n");
				int ID = atoi(strtok(NULL,"\n"));
				char *bonename = strtok(NULL,"\n");
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Create_3D_WAV_Sound_At_Bone(soundname,obj,bonename);
				}
			}
			break;
		case SS_CREATE3DSOUND: //Create_3D_Sound_At_Bone send over network
			{
				char *soundname = strtok(NULL,"\n");
				int ID = atoi(strtok(NULL,"\n"));
				char *bonename = strtok(NULL,"\n");
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Create_3D_Sound_At_Bone(soundname,obj,bonename);
				}
			}
			break;
		case SS_PLAYANNOUNCEMENT: //Play_Building_Announcement send over network
			{
				int ID = atoi(strtok(NULL,"\n"));
				int announcement = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Play_Building_Announcement(obj,announcement);
				}
			}
			break;
		case SS_RADAR: //Enable_Radar send over network
			{
				bool enable = atoi(strtok(NULL,"\n"));
				Enable_Radar(enable);
			}
			break;
		case SS_GDITERMINAL: //Display_GDI_Player_Terminal send over network
			{
				Display_GDI_Player_Terminal();
			}
			break;
		case SS_NODTERMINAL: //Display_NOD_Player_Terminal send over network
			{
				Display_NOD_Player_Terminal();
			}
			break;
		case SS_SETSFC: //Set_Screen_Fade_Color send over network
			{
				float red = (float)atof(strtok(NULL,"\n"));
				float green = (float)atof(strtok(NULL,"\n"));
				float blue = (float)atof(strtok(NULL,"\n"));
				float transition = (float)atof(strtok(NULL,"\n"));
				Set_Screen_Fade_Color(red,green,blue,transition);
			}
			break;
		case SS_SETSFO: //Set_Screen_Fade_Opacity send over network
			{
				float opacity = (float)atof(strtok(NULL,"\n"));
				float transition = (float)atof(strtok(NULL,"\n"));
				Set_Screen_Fade_Opacity(opacity,transition);
			}
			break;
		// BACKCOMPAT: For compatability with older scripts.dll servers only.
		case SS_VERSION: //VERSION console command
			{
				int playerId = atoi(strtok(NULL,"\n"));
				char buf[255];
				sprintf(buf, "j\n%d\n%f\n", playerId, TT_VERSION);
				SendTextCs(buf, PrivateMessage, cNetwork::PClientConnection->Get_Local_Id(), -3);
			}
			break;
		case SS_CLEARWEAPONS: //Clear_Weapons send over network
			{
				int ID = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Clear_Weapons(obj);
				}
			}
			break;
		case SS_VEHICLETRANSITIONS: //Enable_Vehicle_Transitions send over network
			{
				int ID = atoi(strtok(NULL,"\n"));
				bool enable = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Enable_Vehicle_Transitions(obj,enable);
				}
			}
			break;
		case SS_DISPLAYCOLOR: //Set_Display_Color send over network
			{
				unsigned char red = (unsigned char)atoi(strtok(NULL,"\n"));
				unsigned char green = (unsigned char)atoi(strtok(NULL,"\n"));
				unsigned char blue = (unsigned char)atoi(strtok(NULL,"\n"));
				Set_Display_Color(red,green,blue);
			}
			break;
		case SS_DISPLAYTEXT: //Display_Text send over network
			{
				int strnum = atoi(strtok(NULL,"\n"));
				Display_Text(strnum);
			}
			break;
		case SS_DISPLAYFLOAT: //Display_Float send over network
			{
				float num = (float)atof(strtok(NULL,"\n"));
				char *msg = strtok(NULL,"\n");
				Display_Float(num,msg);
			}
			break;
		case SS_DISPLAYINT: //Display_Int send over network
			{
				int num = atoi(strtok(NULL,"\n"));
				char *msg = strtok(NULL,"\n");
				Display_Int(num,msg);
			}
			break;
		case SS_SELECTWEAPON: //Select_Weapon send over network
			{
				int ID = atoi(strtok(NULL,"\n"));
				char *weapon = strtok(NULL,"\n");
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Select_Weapon(obj,weapon);
					VehicleGameObj *o = (VehicleGameObj *)As_VehicleGameObj(obj);
					if (o)
					{
						o->Init_Muzzle_Bones();
						WeaponBagClass *w = ((ArmedGameObj *)o)->Get_Weapon_Bag();
						if ((w->Get_Index()) && (w->Get_Index() < w->Get_Count()))
						{
							w->Peek_Weapon(w->Get_Index())->Clear_Firing_Sound();
						}
					}
				}
			}
			break;
		case SS_FORCELOOK: //Force_Camera_Look send over network
			{
				float x = (float)atof(strtok(NULL,"\n"));
				float y = (float)atof(strtok(NULL,"\n"));
				float z = (float)atof(strtok(NULL,"\n"));
				Vector3 v;
				v.X = x;
				v.Y = y;
				v.Z = z;
				Force_Camera_Look(v);
			}
			break;
		case SS_ENABLEHUD: //Enable_HUD send over network
			{
				bool enable = atoi(strtok(NULL,"\n"));
				Enable_HUD(enable);
			}
			break;
		case SS_SHAKECAMERA: //Shake_Camera send over network
			{
				char *c;
				float x,y,z,radius,intensity,time;
				c = strtok(NULL,"\n");
				if (c)
				{
					x = (float)atof(c);
				}
				else
				{
					break;
				}
				c = strtok(NULL,"\n");
				if (c)
				{
					y = (float)atof(c);
				}
				else
				{
					break;
				}
				c = strtok(NULL,"\n");
				if (c)
				{
					z = (float)atof(c);
				}
				else
				{
					break;
				}
				c = strtok(NULL,"\n");
				if (c)
				{
					radius = (float)atof(c);
				}
				else
				{
					break;
				}
				c = strtok(NULL,"\n");
				if (c)
				{
					intensity = (float)atof(c);
				}
				else
				{
					break;
				}
				c = strtok(NULL,"\n");
				if (c)
				{
					time = (float)atof(c);
				}
				else
				{
					break;
				}
				Vector3 v;
				v.X = x;
				v.Y = y;
				v.Z = z;
				Shake_Camera(v,radius,intensity,time);
			}
			break;
		case SS_WEAPONSEND: //Do_Weapon_Data_Send
			{
				int ID = atoi(strtok(NULL,"\n"));
				int a = atoi(strtok(NULL,"\n"));
				int b = atoi(strtok(NULL,"\n"));
				bool c = atoi(strtok(NULL,"\n"));
				GameObject *o = Commands->Find_Object(ID);
				if (o)
				{
					o->As_PhysicalGameObj()->As_ArmedGameObj()->Get_Weapon_Bag()->Add_Weapon(a,b,c);
				}
			}
			break;
		case SS_POWERUPSOUND: //Powerup/Death Sound
			{
				int ID = atoi(strtok(NULL,"\n"));
				uint32 soundid = atoi(strtok(NULL,"\n"));
				GameObject *o = Commands->Find_Object(ID);
				if (o)
				{
					const Matrix3D &m = ((PhysicalGameObj *)o)->Get_Transform();
					WWAudioClass::_theInstance->Create_Instant_Sound(soundid,m,0,0,2);
				}
			}
			break;
		case SS_ICONS: //New emoticons code
			{
				int ID = atoi(strtok(NULL,"\n"));
				char *icon = strtok(NULL,"\n");
				GameObject *o = Get_GameObj(ID);
				if (o)
				{
					((SoldierGameObj *)As_SoldierGameObj(o))->Set_Emot_Icon(icon,2.0);
				}
			}
			break;
		case SS_BLIPSHAPE: //Set_Obj_Radar_Blip_Shape
			{
				int ID = atoi(strtok(NULL,"\n"));
				int shape = atoi(strtok(NULL,"\n"));
				GameObject *o = Commands->Find_Object(ID);
				if (o)
				{
					Set_Obj_Radar_Blip_Shape(o,shape);
				}
			}
			break;
		case SS_BLIPCOLOR: //Set_Obj_Radar_Blip_Color
			{
				int ID = atoi(strtok(NULL,"\n"));
				int color = atoi(strtok(NULL,"\n"));
				GameObject *o = Commands->Find_Object(ID);
				if (o)
				{
					Set_Obj_Radar_Blip_Color(o,color);
				}
			}
			break;
		case SS_INFOCHANGE: //Info Texture change
			{
				char *texture = strtok(NULL,"\n");
				SetInfoTexture(texture);
			}
			break;
		case SS_INFODISABLE: //Info Texture Disable
			{
				ClearInfoTexture();
			}
			break;
		case SS_ADDMESSAGE: //add combat message
			{
				unsigned int red = atoi(strtok(NULL,"\n"));
				unsigned int green = atoi(strtok(NULL,"\n"));
				unsigned int blue = atoi(strtok(NULL,"\n"));
				char *msg = strtok(NULL,"\n");
				AddCombatMessage(msg,red,green,blue);
			}
			break;
		case SS_VEHICLELIMIT: //vehicle limit change
			{
				unsigned int limit = atoi(strtok(NULL,"\n"));
				VehicleCount = limit;
			}
			break;
		case SS_HEALTHBAR: //Display_Health_Bar
			{
				int ID = atoi(strtok(NULL,"\n"));
				bool display = atoi(strtok(NULL,"\n"));
				GameObject *o = Commands->Find_Object(ID);
				if (o)
				{
					Display_Health_Bar(o,display);
				}
			}
			break;
		case SS_WIREFRAME: //Wireframe mode
			{
				int mode = atoi(strtok(NULL,"\n"));
				SetWireframeMode(mode);
			}
			break;
		case SS_DISABLEALLCOLLISIONS: //Disable_All_Collisions
			{
				int ID = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Disable_All_Collisions(obj);
				}
			}
			break;
		case SS_DISABLEPHYSCOLLISIONS: //Disable_Physical_Collisions
			{
				int ID = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Disable_Physical_Collisions(obj);
				}
			}
			break;
		case SS_ENABLECOLLISIONS: //Enable_Collisions
			{
				int ID = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Enable_Collisions(obj);
				}
			}
			break;
		case SS_HUDINI: //hud.ini change stuff
			{
				char *ini = strtok(NULL,"\n");
				ReadRuntimeINI(ini);
			}
			break;
		case SS_REMOVEWEAPON: //Remove Weapon
			{
				int ID = atoi(strtok(NULL,"\n"));
				const char *weapon = strtok(NULL,"\n");
				GameObject *o = Commands->Find_Object(ID);
				if (o)
				{
					Remove_Weap(o,weapon);
				}
			}
			break;
		case SS_TEAMPURCHASE: //send TeamPurchaseSettingsDefClass over the network
			{

				ptupdate = true;
				unsigned int team = atoi(strtok(NULL,"\n"));
				TeamPurchaseSettingsDefClass *d = Get_Team_Purchase_Definition(team);
				d->presetids[0] = atoi(strtok(NULL,"\n"));
				d->presetids[1] = atoi(strtok(NULL,"\n"));
				d->presetids[2] = atoi(strtok(NULL,"\n"));
				d->presetids[3] = atoi(strtok(NULL,"\n"));
				d->stringids[0] = atoi(strtok(NULL,"\n"));
				d->stringids[1] = atoi(strtok(NULL,"\n"));
				d->stringids[2] = atoi(strtok(NULL,"\n"));
				d->stringids[3] = atoi(strtok(NULL,"\n"));
				d->textures[0] = strtok(NULL,"\n");
				d->textures[1] = strtok(NULL,"\n");
				d->textures[2] = strtok(NULL,"\n");
				d->textures[3] = strtok(NULL,"\n");
				d->beaconcost = atoi(strtok(NULL,"\n"));
				d->beaconpresetid = atoi(strtok(NULL,"\n"));
				d->beaconstringid = atoi(strtok(NULL,"\n"));
				d->beacontexture = strtok(NULL,"\n");
				d->refillstringid = atoi(strtok(NULL,"\n"));
				d->refilltexture = strtok(NULL,"\n");

			}
			break;
		case SS_PURCHASE1: //send PurchaseSettingsDefClass over the network
			{

				ptupdate = true;
				unsigned int team = atoi(strtok(NULL,"\n"));
				unsigned int type = atoi(strtok(NULL,"\n"));
				PurchaseSettingsDefClass *d = Get_Purchase_Definition(type,team);
				d->costs[0] = atoi(strtok(NULL,"\n"));
				d->costs[1] = atoi(strtok(NULL,"\n"));
				d->costs[2] = atoi(strtok(NULL,"\n"));
				d->costs[3] = atoi(strtok(NULL,"\n"));
				d->costs[4] = atoi(strtok(NULL,"\n"));
				d->costs[5] = atoi(strtok(NULL,"\n"));
				d->costs[6] = atoi(strtok(NULL,"\n"));
				d->costs[7] = atoi(strtok(NULL,"\n"));
				d->costs[8] = atoi(strtok(NULL,"\n"));
				d->costs[9] = atoi(strtok(NULL,"\n"));
				d->presetids[0] = atoi(strtok(NULL,"\n"));
				d->presetids[1] = atoi(strtok(NULL,"\n"));
				d->presetids[2] = atoi(strtok(NULL,"\n"));
				d->presetids[3] = atoi(strtok(NULL,"\n"));
				d->presetids[4] = atoi(strtok(NULL,"\n"));
				d->presetids[5] = atoi(strtok(NULL,"\n"));
				d->presetids[6] = atoi(strtok(NULL,"\n"));
				d->presetids[7] = atoi(strtok(NULL,"\n"));
				d->presetids[8] = atoi(strtok(NULL,"\n"));
				d->presetids[9] = atoi(strtok(NULL,"\n"));
				d->stringids[0] = atoi(strtok(NULL,"\n"));
				d->stringids[1] = atoi(strtok(NULL,"\n"));
				d->stringids[2] = atoi(strtok(NULL,"\n"));
				d->stringids[3] = atoi(strtok(NULL,"\n"));
				d->stringids[4] = atoi(strtok(NULL,"\n"));
				d->stringids[5] = atoi(strtok(NULL,"\n"));
				d->stringids[6] = atoi(strtok(NULL,"\n"));
				d->stringids[7] = atoi(strtok(NULL,"\n"));
				d->stringids[8] = atoi(strtok(NULL,"\n"));
				d->stringids[9] = atoi(strtok(NULL,"\n"));
				d->textures[0] = strtok(NULL,"\n");
				d->textures[1] = strtok(NULL,"\n");
				d->textures[2] = strtok(NULL,"\n");
				d->textures[3] = strtok(NULL,"\n");
				d->textures[4] = strtok(NULL,"\n");
				d->textures[5] = strtok(NULL,"\n");
				d->textures[6] = strtok(NULL,"\n");
				d->textures[7] = strtok(NULL,"\n");
				d->textures[8] = strtok(NULL,"\n");
				d->textures[9] = strtok(NULL,"\n");

			}
			break;
		case SS_PURCHASE2: //purchase settings def class part 2
			{

				ptupdate = true;
				unsigned int team = atoi(strtok(NULL,"\n"));
				unsigned int type = atoi(strtok(NULL,"\n"));
				PurchaseSettingsDefClass *d = Get_Purchase_Definition(type,team);
				d->altpresetids[0][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[0][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[0][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[1][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[1][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[1][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[2][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[2][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[2][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[3][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[3][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[3][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[4][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[4][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[4][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[5][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[5][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[5][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[6][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[6][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[6][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[7][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[7][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[7][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[8][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[8][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[8][2] = atoi(strtok(NULL,"\n"));
				d->altpresetids[9][0] = atoi(strtok(NULL,"\n"));
				d->altpresetids[9][1] = atoi(strtok(NULL,"\n"));
				d->altpresetids[9][2] = atoi(strtok(NULL,"\n"));
				d->alttextures[0][0] = strtok(NULL,"\n");
				d->alttextures[0][1] = strtok(NULL,"\n");
				d->alttextures[0][2] = strtok(NULL,"\n");
				d->alttextures[1][0] = strtok(NULL,"\n");
				d->alttextures[1][1] = strtok(NULL,"\n");
				d->alttextures[1][2] = strtok(NULL,"\n");
				d->alttextures[2][0] = strtok(NULL,"\n");
				d->alttextures[2][1] = strtok(NULL,"\n");
				d->alttextures[2][2] = strtok(NULL,"\n");
				d->alttextures[3][0] = strtok(NULL,"\n");
				d->alttextures[3][1] = strtok(NULL,"\n");
				d->alttextures[3][2] = strtok(NULL,"\n");
				d->alttextures[4][0] = strtok(NULL,"\n");
				d->alttextures[4][1] = strtok(NULL,"\n");
				d->alttextures[4][2] = strtok(NULL,"\n");
				d->alttextures[5][0] = strtok(NULL,"\n");
				d->alttextures[5][1] = strtok(NULL,"\n");
				d->alttextures[5][2] = strtok(NULL,"\n");
				d->alttextures[6][0] = strtok(NULL,"\n");
				d->alttextures[6][1] = strtok(NULL,"\n");
				d->alttextures[6][2] = strtok(NULL,"\n");
				d->alttextures[7][0] = strtok(NULL,"\n");
				d->alttextures[7][1] = strtok(NULL,"\n");
				d->alttextures[7][2] = strtok(NULL,"\n");
				d->alttextures[8][0] = strtok(NULL,"\n");
				d->alttextures[8][1] = strtok(NULL,"\n");
				d->alttextures[8][2] = strtok(NULL,"\n");
				d->alttextures[9][0] = strtok(NULL,"\n");
				d->alttextures[9][1] = strtok(NULL,"\n");
				d->alttextures[9][2] = strtok(NULL,"\n");

			}
			break;
		case SS_RADARMAP: //Send scrolling radar map data over network
			{
				if (ScrollingMap)
				{
					MapScale = (float)atof(strtok(NULL,"\n"));
					MapOffset.X = (float)atof(strtok(NULL,"\n"));
					MapOffset.Y = (float)atof(strtok(NULL,"\n"));
					strcpy(MapTexture,strtok(NULL,"\n"));
					Update_Radar_Map(MapScale,MapOffset.X,MapOffset.Y,MapTexture);
				}
			}
			break;
		case SS_CURRENTLYBUILDING: //Currently Building check
			{
				CurrentlyBuilding = atoi(strtok(NULL,"\n"));
			}
			break;
		case SS_FOGCOLOR: //fog color
			{
				unsigned int red = atoi(strtok(NULL,"\n"));
				unsigned int green = atoi(strtok(NULL,"\n"));
				unsigned int blue = atoi(strtok(NULL,"\n"));
				Set_Fog_Color(red,green,blue);
			}
			break;
		case SS_FOGMODE: //fog mode
			{
				unsigned int mode = atoi(strtok(NULL,"\n"));
				Set_Fog_Mode(mode);
			}
			break;
		case SS_FOGDENSITY: //fog density
			{
				float density = (float)atof(strtok(NULL,"\n"));
				Set_Fog_Density(density);
			}
			break;
		case SS_CAMERAHOST: //was Set_Camera_Host
			{
			}
			break;
		case SS_TIMEREMAINING: //TimeRemaining_Seconds change at runtime
			{
				float time = (float)atof(strtok(NULL,"\n"));
				The_Game()->TimeRemaining_Seconds = time;
			}
			break;
		case SS_TIMELIMIT: //TimeLimit_Minutes change at runtime
			{
				int time = atoi(strtok(NULL,"\n"));
				The_Game()->TimeLimit_Minutes = time;
			}
			break;
		case SS_GDISIDEBAR: //GDI sidebar display
			{
				SidebarDlg::Do_Dialog(0);
			}
			break;
		case SS_NODSIDEBAR: //NOD sidebar display
			{
				SidebarDlg::Do_Dialog(1);
			}
			break;
		case SS_PTSECURITY: //PT security dialog
			{
				Display_Dialog(230);
			}
			break;
		case SS_HUDNUMBER: //hud number
			{
				int num = atoi(strtok(NULL,"\n"));
				Send_HUD_Number(num);
			}
			break;
		case SS_MAPCHECK: //map check
			{
				long ID = atoi(strtok(NULL,"\n"));
				char *map = strtok(NULL,"\n");
				char filename[MAX_PATH];
				sprintf(filename,"data\\%s",map);
				if (PathFileExists(filename))
				{
					char buf[255];
					sprintf(buf,"m\n%d\n%s\n",ID,map);
					SendTextCs(buf,PrivateMessage,cNetwork::PClientConnection->Get_Local_Id(),-3);
				}
				else
				{
					char buf[255];
					sprintf(buf,"n\n%d\n%s\n",ID,map);
					SendTextCs(buf,PrivateMessage,cNetwork::PClientConnection->Get_Local_Id(),-3);
				}
			}
			break;
		case SS_CONTROLENABLE: //Control_Enable
			{
				int ID = atoi(strtok(NULL,"\n"));
				bool enable = atoi(strtok(NULL,"\n"));
				GameObject *obj = Commands->Find_Object(ID);
				if (obj)
				{
					Control_Enable(obj,enable);
				}
			}
			break;
		case SS_TEXTFILE: //text file dialog display
			{
				char *file = strtok(NULL,"\n");
				if (file)
				{
					ObjectivesTextDlg::Do_Dialog(file);
				}
			}
			break;
		case SS_SERVERVERSION: //server version
			{
				ServerVersion = (float)atof(strtok(NULL, "\n"));
			}
			break;
		case SS_SCREENSHOT: //remote screenshot
			{
				const char* url = strtok(NULL, "\n");
				if (url)
				{
					if (GameInFocus)
					{
						Make_Remote_Screen_Shot(url);

						WideStringClass message;
						message.Format(L"a\n0");
						cCsTextObj* messageObj = new cCsTextObj();
						messageObj->Init(message, PrivateMessage, cNetwork::PClientConnection->Get_Local_Id(), -3);
					}
					else
					{
						screenshotManager.setPendingScreenshotUrl(url);

						WideStringClass message;
						message.Format(L"a\n2");
						cCsTextObj* messageObj = new cCsTextObj();
						messageObj->Init(message, PrivateMessage, cNetwork::PClientConnection->Get_Local_Id(), -3);
					}
				}
			}
			break;
		case SS_SENDPARAM:
			{
				char* value = strtok(NULL, "\n");
				char* value2 = strtok(NULL, "\n");
				ShaderSet(value,value2);
			}
			break;
		case SS_SENDPARAMOBJ:
			{
				char* value = strtok(NULL, "\n");
				char* value2 = strtok(NULL, "\n");
				int ID = atoi(strtok(NULL,"\n"));
				ShaderSetObj(value,value2,Commands->Find_Object(ID));
			}
			break;
		case SS_GOTOFRAME:
			{
				int ObjectID = atoi(strtok(NULL,"\n"));
				float Frame = (float)atof(strtok(NULL,"\n"));
				char *Anim = strtok(NULL, "\n");
				Static_Anim_Phys_Goto_Frame(ObjectID,Frame,Anim);
			}
			break;
		case SS_GOTOLASTFRAME:
			{
				int ObjectID = atoi(strtok(NULL,"\n"));
				char *Anim = strtok(NULL, "\n");
				Static_Anim_Phys_Goto_Last_Frame(ObjectID,Anim);
			}
			break;
		case SS_FOGOVERRIDE:
			{
				float start_distance = (float)atof(strtok(NULL,"\n"));
				float end_distance = (float)atof(strtok(NULL,"\n"));
				SetFogOverride(start_distance,end_distance);
			}
			break;
		case SS_CLEARFOGOVERRIDE:
			{
				ClearFogOverride();
			}
			break;
		}
	}
}

void Do_Recieve_Data_Sc(cScTextObj *Message)
{
	if (Message->senderId == -2)
	{
		char *str = (char *)WideCharToChar(Message->message);
		Do_Data_Process(str);
		delete[] str;
	}
	else if (Message->senderId == -3)
	{
		char *str = (char *)(Message->message.Peek_Buffer());
		Do_Data_Process(str);
	}
	else
	{
		if (ClientChatLog == true)
		{
			char temp[64];
			memset(temp,0,64);
			char buf[100];
			memset(buf,0,100);
			time_t t = time(0);
			tm *local = localtime(&t);
			sprintf(buf,"client_%d-%d-%02d.txt",local->tm_mon+1,local->tm_mday,local->tm_year);
			FILE *f = fopen(buf,"at");
			const char *name;
			if (Message->senderId != -1)
			{
				name = Get_Player_Name_By_ID(Message->senderId);
			}
			else
			{
				name = newstr("Host");
			}
			if (f)
			{
				GetTimeFormat(LANG_SYSTEM_DEFAULT,TIME_FORCE24HOURFORMAT,0,"'['HH':'mm':'ss'] '",temp,0x40);
				if ((Message->popup == false) && (Message->receiverId >= -1))
				{
					switch(Message->type)
					{
						case PublicMessage:
						{
							fprintf(f,"%s%s: %S\n",temp,name,Message->message.Peek_Buffer());
						}
						break;
						case TeamMessage:
						{
							fprintf(f,"%s[Team] %s: %S\n",temp,name,Message->message.Peek_Buffer());
						}
						break;
						case PrivateMessage:
						{
							fprintf(f,"%s[Private] %s: %S\n",temp,name,Message->message.Peek_Buffer());
						}
						break;
					}
				}
				fclose(f);
			}
			delete[] name;
		}
	}
}
