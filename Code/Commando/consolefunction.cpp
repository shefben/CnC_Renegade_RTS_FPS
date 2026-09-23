/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/consolefunction.cpp                 $*
 *                                                                                             *
 *                      $Author:: Steve_t                                                     $*
 *                                                                                             *
 *                     $Modtime:: 10/13/02 10:13p                                             $*
 *                                                                                             *
 *                    $Revision:: 339                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "consolefunction.h"
#include "ttconsole.h"
#include "console.h"
#include "textdisplay.h"
#include "combatgmode.h"
#include "combat.h"
#include "datasafe.h"
#include "pscene.h"
#include "worldterrainsystem.h"
#include "terraintexturesystem.h"
#include "roadspline.h"
#include "bridgesection.h"
#include "foliagesystem.h"
#include "foliagetype.h"
#include "watersystem.h"
#include "watertype.h"
#include "surfacemarktype.h"
#include "surfaceribbonsystem.h"
#include "worldlightmanager.h"
#include "debugrendersystem.h"
#include "buildingdescriptormanager.h"
#include "rtstelemetryservice.h"
#include "commandermodecontroller.h"
#include "commandersidebar.h"
#include "commanderplacement.h"
#include "worldoperationjournal.h"
#include "logisticsgraphservice.h"
#include "proceduralworldmanager.h"
#include "proceduralworldprofile.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "commanderghostrenderer.h"
#include "constructionrenderer.h"
#include "buildingconstruction.h"
#include "commanderplacementevent.h"
#include "commanderproductionevent.h"
#include "commandervotecallevent.h"
#include "commanderai.h"
#include "commandervotecastevent.h"
#include "commandertransferevent.h"

#include "worldparticlemanager.h"
#include "worldshadowmanager.h"
#include "worldsurfacemarkmanager.h"
#include "ribbontype.h"
#include "bridgesystem.h"
#include "roadsystem.h"
#include "playermanager.h"
#include "ccamera.h"
#include "debug.h"
#include "level.h"
#include "weaponmanager.h"
#include "cnetwork.h"
#include "ww3d.h"
#include "miscutil.h"
#include "smartgameobj.h"
#include "weapons.h"
#include "WWAudio.h"
#include "wwprofile.h"
//#include "gamesettings.h"
#include "waypoint.h"
#include "action.h"
#include "gameobjmanager.h"
#include "surfaceeffects.h"
#include "gamedata.h"
#include "damage.h"
#include "radar.h"
#include "assets.h"
#include "animobj.h"
#include "matpass.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "objlibrary.h"
#include "useroptions.h"
#include "devoptions.h"
#include "god.h"
#include "playertype.h"
#include "teampowergridservice.h"
#include "systemsettings.h"
#include "spawn.h"
#include "input.h"
#include "Pathfind.h"
#include "waypath.h"
#include "definitionclassids.h"
#include "netinterface.h"
#include "bandwidth.h"
#include "statistics.h"
#include "physaabtreecull.h"
#include "gametype.h"
#include "part_emt.h"
#include "translatedb.h"
#include "string_ids.h"
#include "dx8renderer.h"
#include "backgroundmgr.h"
#include "dynamicphys.h"
#include "humanphys.h"
#include "vehiclephys.h"
#include "objectives.h"
#include "umbrasupport.h"
#include "vehicle.h"
#include "damageablestaticphys.h"
#include "persistfactory.h"
#include "wwphysids.h"
#include "hud.h"
#include "conversationmgr.h"
#include "basecontroller.h"
#include "building.h"
#include "vendor.h"
#include "texture.h"
#include "rddesc.h"
#include "combatchunkid.h"
#include "dialogtests.h"
#include "dx8wrapper.h"
#include "sortingrenderer.h"
#include "WeatherMgr.h"
#include "weatherenvironment.h"
#include "tracerrenderer.h"
#include "debrisrenderer.h"
#include "debrissystem.h"
#include "mapmgr.h"
#include "Path.h"
#include "sctextobj.h"
#include "consolecommandevent.h"
#include "hudinfo.h"
#include "physresourcemgr.h"
#include "cstextobj.h"
#include "suicideevent.h"
#include "godmodeevent.h"
#include "serverfps.h"
#include "warpevent.h"
//#include "helptext.h"
#include "ffactory.h"
#include "dazzle.h"
#include "renegadedialogmgr.h"
#include "moneyevent.h"
#include "scoreevent.h"
#include "dlgmpingamechat.h"
#include "playerterminal.h"
#include "encyclopediamgr.h"
#include "clientbboevent.h"
#include "wheelvehicle.h"
#include "trackedvehicle.h"
#include "WOLDiags.h"
#include "packetmgr.h"
#include "requestkillevent.h"
#include "csconsolecommandevent.h"
#include "apppacketstats.h"
#include "bandwidthgraph.h"
#include "scexplosionevent.h"
#include "BWBalance.h"
#include "sbbomanager.h"
#include "_globals.h"
#include "registry.h"
#include "vipmodeevent.h"
#include "dx8rendererdebugger.h"
#include "changeteamevent.h"
#include "gamesideservercontrol.h"
#include "AutoStart.h"
#include "Except.h"
#include "slavemaster.h"
#include "donateevent.h"
#include "ConsoleMode.h"
#include "dlgcncpurchasemainmenu.h"
#include "realcrc.h"
#include "gamespyadmin.h"
#include "GameSpyBanList.h"
#include "specialbuilds.h"
#include "lightsolve.h"
#include "lightsolvecontext.h"
#include "debugbreak.h"
#include "openw3d.h"



void	ConsoleFunctionClass::Print( const char *format, ... )
{
	//
	//	The text is already formatted, so it goes through as an argument
	//	rather than as a format string of its own.
	//
	va_list arg_list;
	va_start (arg_list, format);
	StringClass string;
	string.Format_Args( format, arg_list );
	if (Get_Text_Display()) {
		WWASSERT( Get_Text_Display() );
		Get_Text_Display()->Print_System( "%s", string.Peek_Buffer() );
	}
	ConsoleBox.Print("%s", string.Peek_Buffer());
	va_end (arg_list);
}

//----------------------------------------------------------------------------
//
// DEVELOPMENT CONSOLE FUNCTIONS ONLY
// Only present in Debug and Profile builds.
//
//----------------------------------------------------------------------------

#ifdef WWDEBUG

/*
** Console Functions
*/
class BuildingStateConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "building_state"; }
	virtual	const char * Get_Alias( void ) override { return "bs"; }
	virtual	const char * Get_Help( void ) override	{ return "BUILDING_STATE <health:0,100> <power:0,1> - force all buildings to the given state"; }
	virtual	void Activate( const char * input ) override {

		int health_percentage,power;
		int cnt=sscanf(input, "%d %d", &health_percentage, &power);
		if (cnt == 2) {
			GameObjManager::Debug_Set_All_Building_States((float)health_percentage,(power==1));
			Print("Building States set to Health = %d%%, Power = %d\n",health_percentage,power);
		} else {
			GameObjManager::Debug_Set_All_Building_States(100,1);
			GameObjManager::Debug_Set_All_Building_States(0,1);
			Print("Blew up all buildings!\n");
		}
	}
};

class DSAPOResetConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "dsapo_reset"; }
	virtual	const char * Get_Help( void ) override	{ return "DSAPO_RESET - force all DSAPO's back to their initial state."; }
	virtual	void Activate( const char * input ) override {

		if (COMBAT_SCENE) {
			RefPhysListIterator	it = COMBAT_SCENE->Get_Static_Anim_Object_Iterator();
			for (it.First();!it.Is_Done();it.Next()) {

				if (it.Peek_Obj()->Get_Factory().Chunk_ID() == PHYSICS_CHUNKID_DAMAGEABLESTATICPHYS) {
					DamageableStaticPhysClass * dobj = (DamageableStaticPhysClass *)it.Peek_Obj();
					dobj->Reset_Health();
				}
			}
		}

		int health_percentage,power;
		int cnt=sscanf(input, "%d %d", &health_percentage, &power);
		if (cnt == 2) {
			GameObjManager::Debug_Set_All_Building_States((float)health_percentage,(power==1));
			Print("Building States set to Health = %d%%, Power = %d\n",health_percentage,power);
		} else {
			GameObjManager::Debug_Set_All_Building_States(100,1);
			GameObjManager::Debug_Set_All_Building_States(0,1);
			Print("Blew up all buildings!\n");
		}
	}
};

class HUDConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "hud"; }
	virtual	const char * Get_Help( void ) override	{ return "HUD - toggles the HUD."; }
	virtual	void Activate( const char * /* input */ ) override {
		HUDClass::Enable( !HUDClass::Is_Enabled() );
      Print( "HUD toggled\n" );
	}
};


class PatsDebugConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "pat"; }
	virtual	const char * Get_Help( void ) override	{ return "PAT - Pat's generic debug console function."; }
	virtual	void Activate( const char * /* input */ ) override
	{
		RenegadeDialogMgrClass::Goto_Location (RenegadeDialogMgrClass::LOC_INTERNET_MAIN);
		return ;
	}
};


class TextureMemoryCounterConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "texture_memory_counter"; }
	virtual	const char * Get_Help( void ) override	{ return "TEXTURE_MEMORY_COUNTER - 0=off 1=on 2=details"; }
	virtual	void Activate( const char * input ) override {
		Debug_Statistics::RecordTextureMode b;
		if (sscanf(input, "%d", &b) == 1) {
			Debug_Statistics::Record_Texture_Mode(b);
		}
	}
};


class MeshDebuggerEnableConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "mesh_debugger_enable"; }
	virtual	const char * Get_Help( void ) override	{ return "MESH_DEBUGGER_ENABLE - 0=off 1=on"; }
	virtual	void Activate( const char * input ) override {
		unsigned onoff;
		if (sscanf(input, "%d", &onoff) == 1) {
			DX8RendererDebugger::Enable(!!onoff);
		}
	}
};

class MeshDebuggerEnableMeshConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "mesh_debugger_enable_mesh"; }
	virtual	const char * Get_Help( void ) override	{ return "MESH_DEBUGGER_ENABLE_MESH - id0 ... id9"; }
	virtual	void Activate( const char * input ) override {
		// Parameter '*' means ALL meshes
		char string[200];
		sscanf(input, "%s", string);
		if (!stricmp("*",string)) {
			DX8RendererDebugger::Enable_All();
		}
		else {
			unsigned id[10];
			unsigned count=sscanf(input, "%d %d %d %d %d %d %d %d %d %d",
				&id[0], &id[1], &id[2], &id[3], &id[4], &id[5], &id[6], &id[7], &id[8], &id[9]);
			for (unsigned i=0;i<count;++i) {
				DX8RendererDebugger::Enable_Mesh(id[i]);
			}
		}
	}
};

class MeshDebuggerDisableMeshConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "mesh_debugger_disable_mesh"; }
	virtual	const char * Get_Help( void ) override	{ return "MESH_DEBUGGER_DISABLE_MESH - id0 ... id9"; }
	virtual	void Activate( const char * input ) override {
		// Parameter '*' means ALL meshes
		char string[200];
		sscanf(input, "%s", string);
		if (!stricmp("*",string)) {
			DX8RendererDebugger::Disable_All();
		}
		else {
			unsigned id[10];
			unsigned count=sscanf(input, "%d %d %d %d %d %d %d %d %d %d",
				&id[0], &id[1], &id[2], &id[3], &id[4], &id[5], &id[6], &id[7], &id[8], &id[9]);
			for (unsigned i=0;i<count;++i) {
				DX8RendererDebugger::Disable_Mesh(id[i]);
			}
		}
	}
};


class FlashTextureConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "flash_texture"; }
	virtual	const char * Get_Help( void ) override	{ return "FLASH_TEXTURE <ID> <0/1> - sets texture flashing"; }
	virtual	void Activate( const char * /* input */ ) override {
#ifdef WW3D_DX8
		int id,state;
		int cnt=sscanf(input, "%d %d", &id, &state);
		TextureFileClass* tfc=nullptr;
		if (cnt>=1) tfc=TextureFileClass::Get_Texture(id);
		if (tfc) {
			if (cnt==1) {
				state=!tfc->Get_Texture_Flash();
				tfc->Set_Texture_Flash(!!state);
			}
			else if (cnt==2) {
				tfc->Set_Texture_Flash(!!state);
			}
			StringClass tmp;
			if (state) {
				tmp.Format("Texture %s (id=%d) set FLASHING\n",tfc->Get_File_Name(),tfc->ID());
			}
			else {
				tmp.Format("Texture %s (id=%d) set not flashing\n",tfc->Get_File_Name(),tfc->ID());
			}
			Print(tmp);
		}
#endif //WW3D_DX8
	}
};


class FogRangeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "fog_range"; }
	virtual	const char * Get_Help( void ) override	{ return "FOG_RANGE <start [0+]> <end [0+]> [ramptime [0+]] - Sets fog range.";}
	virtual	void Activate( const char * input ) override {

		const char *feedbackstring = "Fog range changed\n";

		int	argcount;
		float	arg0, arg1, arg2;
		bool	success = false;

		argcount = sscanf (input, "%f %f %f", &arg0, &arg1, &arg2);
		switch (argcount) {

			case 2:
				success = WeatherMgrClass::Set_Fog_Range (arg0, arg1);
				break;

			case 3:
				success = WeatherMgrClass::Set_Fog_Range (arg0, arg1, arg2);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class FogToggleConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{ return "fog_toggle"; }
	virtual	const char * Get_Help (void) override	{ return "FOG_TOGGLE - toggles fog on/off."; }
	virtual	void Activate (const char */* input */) override {

		const char *_statusstring [2] = {"Fog disabled\n", "Fog enabled\n"};

		WeatherMgrClass::Set_Fog_Enable (!WeatherMgrClass::Get_Fog_Enable());
		Print (_statusstring [WeatherMgrClass::Get_Fog_Enable()]);
	}
};


class LightModeConsoleFunctionClass : public ConsoleFunctionClass {
	virtual	const char * Get_Name (void) override	{ return "light_mode"; }
	virtual	const char * Get_Help (void) override	{ return "LIGHT_MODE <0,1,2> - 0=none, 1=surrender, 2=cheap."; }
	virtual	void Activate (const char *input) override {
		int mode;
		if (sscanf(input, "%d", &mode) == 1) {
			COMBAT_SCENE->Set_Lighting_Mode(mode);
		}

		switch(COMBAT_SCENE->Get_Lighting_Mode()) {
		case PhysicsSceneClass::LIGHTING_MODE_SURRENDER:
			Print ("Lighting Mode: SURRENDER");
			break;

		case PhysicsSceneClass::LIGHTING_MODE_CHEAP:
			Print ("Lighting Mode: CHEAP");
			break;

		default:
			Print ("Lighting Mode: NONE");
			break;
		}
	}
};

class LightDebugToggleConsoleFunctionClass : public ConsoleFunctionClass {
	virtual	const char * Get_Name (void) override	{ return "light_debug"; }
	virtual	const char * Get_Help (void) override	{ return "LIGHT_DEBUG - toggles lighting debugging on/off."; }
	virtual	void Activate (const char */* input */) override {
		COMBAT_SCENE->Enable_Lighting_Debug_Display(!COMBAT_SCENE->Is_Lighting_Debug_Display_Enabled());
		if (COMBAT_SCENE->Is_Lighting_Debug_Display_Enabled()) {
			Print ("Lighting debug display ENABLED");
		} else {
			Print ("Lighting debug display DISABLED");
		}
	}
};

class LightLODConsoleFunctionClass : public ConsoleFunctionClass {
	virtual	const char * Get_Name (void) override	{ return "light_lod"; }
	virtual	const char * Get_Help (void) override	{ return "LIGHT_LOD <intensity> - sets the LOD cutoff for lighting."; }
	virtual	void Activate (const char *input) override {
		float inten;
		if (sscanf(input, "%f", &inten) == 1) {
			COMBAT_SCENE->Set_Lighting_LOD_Cutoff(inten);
		}
		Print("Lighting LOD: %d\r\n",COMBAT_SCENE->Get_Lighting_LOD_Cutoff());
	}
};

class LightCalcConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "light_calc"; }
	virtual	const char * Get_Help( void ) override	{ return "LIGHT_CALC - computes a static vertex lighting solve"; }
	virtual	void Activate( const char * /* input */ ) override {
		if (COMBAT_SCENE) {
			LightSolveContextClass context;
			LightSolveClass::Generate_Static_Light_Solve(context);
		}
	}
};



class CameraConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "camera"; }
	virtual	const char * Get_Help( void ) override	{ return "CAMERA <NAME> - switches to camera profile <name>."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_CAMERA ) {
			COMBAT_CAMERA->Use_Profile( input );
		   Print( "Profile Changed\n" );
		}
	}
};

class VisConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_toggle"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_TOGGLE - toggles vis culling."; }
	virtual	void Activate( const char * /* input */ ) override {
		COMBAT_SCENE->Enable_Vis(!COMBAT_SCENE->Is_Vis_Enabled());
		if (COMBAT_SCENE->Is_Vis_Enabled()) {
			Print("vis enabled\n");
		} else {
			Print("vis disabled\n");
		}
	}
};

class VisInvertConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_invert"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_INVERT - toggles vis invert setting."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Invert_Vis(!COMBAT_SCENE->Is_Vis_Inverted());
		if (COMBAT_SCENE->Is_Vis_Inverted()) {
			Print("vis_invert enabled\n");
		} else {
			Print("vis_invert disabled\n");
		}
	}
};

class VisGridDisplayConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_grid_display"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_GRID_DISPLAY <0,1,2,3> - 0=off, 1=boxes, 2=centers, 3=occupied."; }
	virtual	void Activate( const char * input ) override {

		int mode = 0;
		if (sscanf(input, "%d", &mode) == 1) {
			COMBAT_SCENE->Set_Vis_Grid_Display_Mode(mode);
			if (mode == 0) {
				Print("vis_grid_display off\n");
			} else if (mode == 1) {
				Print("vis_grid_display boxes\n");
			} else if (mode == 2) {
				Print("vis_grid_display centers\n");
			} else if (mode == 3) {
				Print("vis_grid_display occupied\n");
			}
		}
	}
};

class VisGridDebugConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_grid_debug"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_GRID_DEBUG <p,s,f,b,r> - p=parent,s=sibling,f=front,b=back,r=reset"; }
	virtual	void Activate( const char * input ) override {

		char mode = 0;
		if (sscanf(input, "%c", &mode) == 1) {
			mode = (char)toupper(mode);

			if (mode == 'P') {
				if (COMBAT_SCENE->Vis_Grid_Debug_Enter_Parent()) {
					Print("Entered parent node\n");
				} else {
					Print("No parent node\n");
				}
			} else if (mode == 'S') {
				if (COMBAT_SCENE->Vis_Grid_Debug_Enter_Sibling()) {
					Print("Entered sibling\n");
				} else {
					Print("No sibling\n");
				}
			} else if (mode == 'F') {
				if (COMBAT_SCENE->Vis_Grid_Debug_Enter_Front_Child()) {
					Print("Entered front child\n");
				} else {
					Print("No front child\n");
				}
			} else if (mode == 'B') {
				if (COMBAT_SCENE->Vis_Grid_Debug_Enter_Back_Child()) {
					Print("Entered back child\n");
				} else {
					Print("No back child\n");
				}
			} else if (mode == 'R') {
				COMBAT_SCENE->Vis_Grid_Debug_Reset_Node();
				Print("Reset to root\n");
			}
		}
	}
};

class VisHierarchicalConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_hierarchical"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_HIERARCHICAL - toggles hierarchical vis culling."; }
	virtual	void Activate( const char * /* input */ ) override {

		PhysAABTreeCullClass::Enable_Hierarchical_Vis_Culling(!PhysAABTreeCullClass::Is_Hierarchical_Vis_Culling_Enabled());
		if (PhysAABTreeCullClass::Is_Hierarchical_Vis_Culling_Enabled()) {
			Print("hierarchical vis culling enabled\n");
		} else {
			Print("hierarchical vis culling disabled\n");
		}
	}
};

/*
**	Generated ground, under your feet, now.
**
**	Roadmap Section 17 requires the terrain service to accept height data from an in-memory or
**	generated source, and its acceptance line is that FPS and vehicle gameplay works on terrain
**	made that way.  Nothing in a stock Renegade level generates any, so without a way to ask for
**	some there is no way to stand on it and find out.  This is that way.
**
**	The field is laid down centred on wherever you are standing, so it appears in a level that
**	knows nothing about it, alongside the W3D geometry that level is made of -- which is the
**	arrangement Section 17 describes and the thing worth seeing work.
*/
class TerrainTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "terrain_test"; }
	virtual	const char * Get_Help( void ) override	{ return "TERRAIN_TEST <cells> <cell_size> <amplitude> - generates a heightfield under you and gives it to the physics scene."; }
	virtual	void Activate( const char * input ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "terrain_test needs somewhere to put the terrain; there is no player.\n" );
			return;
		}

		int	cells		= 64;
		float	cell_size	= 2.0f;
		float	amplitude	= 4.0f;

		if ( input != nullptr ) {
			::sscanf( input, "%d %f %f", &cells, &cell_size, &amplitude );
		}

		//	A field with fewer than one cell is not a field, and one big enough to eat the
		//	address space is not a test.
		if ( cells < 1 )		cells = 1;
		if ( cells > 512 )	cells = 512;
		if ( cell_size < 0.1f )	cell_size = 0.1f;

		int vertex_count = cells + 1;

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		//	Centred on the player and a little below, so the ground appears underfoot rather
		//	than swallowing them.
		Vector3 origin( star_pos.X - (cells * cell_size * 0.5f),
							 star_pos.Y - (cells * cell_size * 0.5f),
							 star_pos.Z - 1.0f - amplitude );

		if ( !WorldTerrainSystem::Create_Terrain( vertex_count, vertex_count, cell_size, origin ) ) {
			Print( "terrain_test could not create the field.\n" );
			return;
		}

		//	Rolling ground rather than a plane: a flat field would collide correctly even if the
		//	triangulation were wrong, and would say nothing about whether walking on it works.
		float * heights = new float[ vertex_count * vertex_count ];
		for ( int iy = 0; iy < vertex_count; iy ++ ) {
			for ( int ix = 0; ix < vertex_count; ix ++ ) {
				float x = (float)ix * 0.25f;
				float y = (float)iy * 0.25f;
				heights[ iy * vertex_count + ix ] =
					amplitude * ( 0.5f + 0.5f * ( ::sinf( x ) * ::cosf( y ) ) );
			}
		}
		WorldTerrainSystem::Set_Heights( heights, vertex_count * vertex_count );
		delete [] heights;

		//	Dress it before building, because building is what fills the material passes and a
		//	field with no layers defined gets built bare.
		TerrainTextureSystem::Create_Masks();
		TerrainTextureSystem::Define_Default_Layers();

		if ( WorldTerrainSystem::Build_Collision() ) {
			Print( "terrain_test: %d x %d cells of %.1fm, %d collision patches.\n",
					 cells, cells, cell_size, WorldTerrainSystem::Get_Collision_Patch_Count() );
		} else {
			Print( "terrain_test: the field exists but its collision would not build.\n" );
		}

		//	And the background layer over the same ground, so that walking away from the field
		//	shows the thing Section 34 is for rather than the field switching off.
		if ( WorldTerrainSystem::Build_Far_Terrain_Representation() ) {
			Print( "terrain_test: far layer %d x %d blocks, %d vertices against %d near.\n",
					 WorldTerrainSystem::Get_Far_Block_Count_X(),
					 WorldTerrainSystem::Get_Far_Block_Count_Y(),
					 WorldTerrainSystem::Get_Far_Vertex_Count(),
					 WorldTerrainSystem::Get_Near_Vertex_Count() );
		}
	}
};


/*
**	What the two terrain representations are doing right now.
**
**	Roadmap Section 34's acceptance is mostly arithmetic -- fewer vertices, fewer submissions,
**	the right regions rebuilt -- and the rest of it is whether the boundary looks like anything.
**	This prints the arithmetic so that the person looking at the boundary knows which side of it
**	they are standing on.
*/
class FarTerrainConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "far_terrain"; }
	virtual	const char * Get_Help( void ) override	{ return "FAR_TERRAIN - reports the near/far terrain split, and builds the far layer if there is none."; }
	virtual	void Activate( const char * input ) override {

		(void)input;

		if ( !WorldTerrainSystem::Has_Terrain() ) {
			Print( "far_terrain: there is no heightfield; try terrain_test first.\n" );
			return;
		}

		if ( WorldTerrainSystem::Get_Far_Block_Built_Count() == 0 ) {
			if ( !WorldTerrainSystem::Build_Far_Terrain_Representation() ) {
				Print( "far_terrain: the far layer would not build.\n" );
				return;
			}
		}

		int near_vertices	= WorldTerrainSystem::Get_Near_Vertex_Count();
		int far_vertices	= WorldTerrainSystem::Get_Far_Vertex_Count();
		int blocks			= WorldTerrainSystem::Get_Far_Block_Built_Count();

		Print( "far_terrain: %d block(s) of %d vertices against %d patch(es) of %d.\n",
				 blocks, far_vertices,
				 WorldTerrainSystem::Get_Collision_Patch_Count(), near_vertices );

		if ( far_vertices > 0 ) {
			Print( "far_terrain: %.1f near vertices per far vertex, layer dropped %.3fm.\n",
					 (float)near_vertices / (float)far_vertices,
					 WorldTerrainSystem::Get_Far_Terrain_Drop() );
		}

		Print( "far_terrain: drawing %d near patch(es) and %d far block(s), %d refill(s) so far.\n",
				 WorldTerrainSystem::Get_Visible_Near_Patch_Count(),
				 WorldTerrainSystem::Get_Visible_Far_Block_Count(),
				 WorldTerrainSystem::Get_Far_Refill_Count() );
	}
};


/*
**	Re-decide what the ground is made of.
**
**	Roadmap Section 18's acceptance is that a generated heightfield can obtain coherent terrain
**	materials entirely from runtime data and masks.  This is that sentence as a command: no
**	texture was painted, no map file was read, and the rules alone decide where the rock, the
**	cliff face, the shoreline and the road go.
*/
class TerrainDressConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "terrain_dress"; }
	virtual	const char * Get_Help( void ) override	{ return "TERRAIN_DRESS - re-decides terrain materials from the default layer rules."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( !WorldTerrainSystem::Has_Terrain() ) {
			Print( "There is no generated terrain to dress.\n" );
			return;
		}

		if ( !TerrainTextureSystem::Has_Masks() ) {
			TerrainTextureSystem::Create_Masks();
		}
		TerrainTextureSystem::Update_Water_Distance();
		TerrainTextureSystem::Define_Default_Layers();

		if ( TerrainTextureSystem::Build_All_Patch_Materials() ) {
			Print( "terrain_dress: %d layers over %d patches.\n",
					 TerrainTextureSystem::Get_Layer_Count(),
					 WorldTerrainSystem::Get_Collision_Patch_Count() );
		} else {
			Print( "terrain_dress: the layers are defined but the patches would not take them.\n" );
		}
	}
};


class TerrainClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "terrain_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "TERRAIN_CLEAR - removes generated terrain and its collision from the scene."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( !WorldTerrainSystem::Has_Terrain() ) {
			Print( "There is no generated terrain to clear.\n" );
			return;
		}

		WorldTerrainSystem::Destroy_Terrain();
		Print( "Generated terrain cleared.\n" );
	}
};


/*
**	Roads made out of the lines they were drawn as, under your feet, now.
**
**	Roadmap Section 19's acceptance is that road geometry can be generated at runtime from
**	centre lines without a unique modelled mesh for every road segment.  Nothing in a stock
**	Renegade level draws a centre line, so this draws a few: an S bend, a road crossing it, and
**	a third road teeing into the first.  Between them they exercise every case the generator
**	has -- a curve, a crossing, a tee, and a junction that three roads reach.
**
**	It works on generated heightfield terrain and on a level's own W3D geometry, because the
**	roads find the ground by asking the terrain service and, when there is none, by dropping a
**	ray through the physics scene.  Laying a road across the floor of a stock Renegade level is
**	the more interesting of the two.
*/
class RoadTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "road_test"; }
	virtual	const char * Get_Help( void ) override	{ return "ROAD_TEST <length> <width> <texture> - generates a road network around you from centre lines."; }
	virtual	void Activate( const char * input ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "road_test needs somewhere to put the roads; there is no player.\n" );
			return;
		}

		float	length	= 80.0f;
		float	width		= 8.0f;
		char	texture[ 128 ];
		texture[ 0 ] = 0;

		if ( input != nullptr ) {
			::sscanf( input, "%f %f %127s", &length, &width, texture );
		}

		if ( length < 10.0f )	length = 10.0f;
		if ( length > 2000.0f )	length = 2000.0f;
		if ( width < 1.0f )		width = 1.0f;
		if ( width > 64.0f )		width = 64.0f;

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		float cx = star_pos.X;
		float cy = star_pos.Y;
		float half = length * 0.5f;
		float sway = length * 0.125f;

		RoadSystem::Clear_Roads();

		//	An S bend through four control points.  The curve passes through all of them, so the
		//	road goes where it was told rather than somewhere near there.
		{
			RoadSplineClass road;
			Vector3 points[ 4 ];
			points[ 0 ].Set( cx - half,					cy,				star_pos.Z );
			points[ 1 ].Set( cx - ( length / 6.0f ),	cy + sway,		star_pos.Z );
			points[ 2 ].Set( cx + ( length / 6.0f ),	cy - sway,		star_pos.Z );
			points[ 3 ].Set( cx + half,					cy,				star_pos.Z );
			road.Set_Control_Points( points, 4 );
			road.Set_Name( "main" );
			road.Set_Width( width );
			road.Set_Shoulder_Width( width * 0.5f );
			road.Set_Road_Class( ROAD_CLASS_STREET );
			road.Set_Material( texture );
			RoadSystem::Add_Road( road );
		}

		//	A crossing.  Neither road is the one that ends; the junction owns the middle and both
		//	of them stop at its edge.
		{
			RoadSplineClass road;
			Vector3 points[ 2 ];
			points[ 0 ].Set( cx, cy - half, star_pos.Z );
			points[ 1 ].Set( cx, cy + half, star_pos.Z );
			road.Set_Control_Points( points, 2 );
			road.Set_Name( "cross" );
			road.Set_Width( width );
			road.Set_Shoulder_Width( width * 0.5f );
			road.Set_Road_Class( ROAD_CLASS_STREET );
			road.Set_Material( texture );
			RoadSystem::Add_Road( road );
		}

		//	And a tee, arriving on the S bend away from the crossing.
		{
			RoadSplineClass road;
			Vector3 points[ 2 ];
			points[ 0 ].Set( cx + ( length / 3.0f ), cy,						star_pos.Z );
			points[ 1 ].Set( cx + ( length / 3.0f ), cy - ( half * 0.75f ),	star_pos.Z );
			road.Set_Control_Points( points, 2 );
			road.Set_Name( "spur" );
			road.Set_Width( width * 0.75f );
			road.Set_Shoulder_Width( width * 0.5f );
			road.Set_Road_Class( ROAD_CLASS_TRACK );
			road.Set_Material( texture );
			RoadSystem::Add_Road( road );
		}

		if ( !RoadSystem::Build_Network() ) {
			Print( "road_test: the network would not build.\n" );
			return;
		}

		//	The roads have written the road mask; the ground has to be re-dressed to show it.
		if ( WorldTerrainSystem::Has_Terrain() && ( TerrainTextureSystem::Get_Layer_Count() > 0 ) ) {
			TerrainTextureSystem::Build_All_Patch_Materials();
		}

		if ( RoadSystem::Build_Geometry() ) {
			Print( "road_test: %d roads, %d junctions, %d batch(es) to draw.\n",
					 RoadSystem::Get_Road_Count(), RoadSystem::Get_Junction_Count(),
					 RoadSystem::Get_Batch_Count() );
			if ( texture[ 0 ] == 0 ) {
				Print( "road_test: no texture named, so the surface is untextured geometry.\n" );
			}
		} else {
			Print( "road_test: %d roads and %d junctions, and no geometry was built.\n",
					 RoadSystem::Get_Road_Count(), RoadSystem::Get_Junction_Count() );
		}
	}
};


class RoadClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "road_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "ROAD_CLEAR - removes generated roads and their geometry from the scene."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( RoadSystem::Get_Road_Count() == 0 ) {
			Print( "There are no generated roads to clear.\n" );
			return;
		}

		RoadSystem::Clear_Roads();
		Print( "Generated roads cleared.\n" );
	}
};


/*
**	Section 20's acceptance is that bridges support variable length, damage, broken spans,
**	collision and multiplayer state, and the first four of those are things to look at rather
**	than things to read.  This drops a bridge across the ground in front of the player at
**	whatever length was asked for, so that "variable length" can be checked by asking for a
**	different number and seeing a different number of spans.
**
**	No bridge art exists, so every section will be a slab of the size the definition declares.
**	That is the point: the slab collides and carries traffic, so the bridge can be walked and
**	driven over now, and the models replace the slabs later without anything else changing.
*/
class BridgeTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "bridge_test"; }
	virtual	const char * Get_Help( void ) override	{ return "BRIDGE_TEST <length> <kind> - builds a sectional bridge across the ground in front of you."; }
	virtual	void Activate( const char * input ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "bridge_test needs somewhere to put the bridge; there is no player.\n" );
			return;
		}

		float	length	= 100.0f;
		char	kind[ 128 ];
		::strcpy( kind, "ow_bridge_concrete" );

		if ( input != nullptr ) {
			::sscanf( input, "%f %127s", &length, kind );
		}

		if ( length < 8.0f )		length = 8.0f;
		if ( length > 2000.0f )	length = 2000.0f;

		if ( BridgeSystem::Get_Definition_Count() == 0 ) {
			BridgeSystem::Init();
		}

		if ( BridgeSystem::Find_Definition( kind ) == nullptr ) {
			Print( "bridge_test: there is no bridge kind called \"%s\".  The ones there are:\n", kind );
			for ( int i = 0; i < BridgeSystem::Get_Definition_Count(); i ++ ) {
				BridgeDefinitionClass * definition = BridgeSystem::Peek_Definition( i );
				if ( definition != nullptr ) {
					Print( "  %s (%.1fm wide, %.1fm spans)\n", definition->Get_Name(),
							 definition->Get_Width(),
							 definition->Get_Section_Length( BRIDGE_SECTION_SPAN ) );
				}
			}
			return;
		}

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		float half = length * 0.5f;

		BridgeSystem::Clear_Bridges();

		BridgeClass bridge;
		bridge.Set_Name( "test" );
		bridge.Set_Definition( kind );
		bridge.Set_Endpoints( Vector3( star_pos.X - half, star_pos.Y, star_pos.Z ),
									 Vector3( star_pos.X + half, star_pos.Y, star_pos.Z ) );
		BridgeSystem::Add_Bridge( bridge );

		if ( !BridgeSystem::Build_Layout() ) {
			Print( "bridge_test: the bridge would not lay out.\n" );
			return;
		}

		BridgeClass * placed = BridgeSystem::Peek_Bridge( 0 );
		int spans = 0;
		int piers = 0;
		if ( placed != nullptr ) {
			for ( int i = 0; i < placed->Get_Section_Count(); i ++ ) {
				if ( placed->Get_Section( i ).Kind == BRIDGE_SECTION_SPAN )		spans ++;
				if ( placed->Get_Section( i ).Kind == BRIDGE_SECTION_SUPPORT )	piers ++;
			}
		}

		//	Any road that said it hands over to a bridge now has one to hand over to.
		int bound = BridgeSystem::Connect_Roads( 16.0f );

		if ( BridgeSystem::Build_Geometry() ) {
			Print( "bridge_test: %.0fm of %s -- %d span(s), %d pier(s), %d object(s) in the scene.\n",
					 length, kind, spans, piers, BridgeSystem::Get_Instance_Count() );
			if ( BridgeSystem::Get_Missing_Model_Count() > 0 ) {
				Print( "bridge_test: %d section(s) have no model, so they are slabs of the declared size.\n",
						 BridgeSystem::Get_Missing_Model_Count() );
			}
		} else {
			Print( "bridge_test: %d span(s) and %d pier(s) laid out, and no geometry was built.\n",
					 spans, piers );
		}

		if ( bound > 0 ) {
			Print( "bridge_test: %d road end(s) hand over to this bridge.\n", bound );
		}
	}
};


/*
**	The half of Section 20 that cannot be seen by building a bridge: breaking one span leaves a
**	bridge standing at both ends that nothing can cross.
*/
class BridgeBreakConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "bridge_break"; }
	virtual	const char * Get_Help( void ) override	{ return "BRIDGE_BREAK - breaks the bridge span nearest you."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "bridge_break needs to know where you are standing.\n" );
			return;
		}
		if ( BridgeSystem::Get_Bridge_Count() == 0 ) {
			Print( "There are no bridges to break.\n" );
			return;
		}

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		//	Whichever bridge is under you, or the first one if you are not on one.
		int id = BridgeSystem::Find_Bridge_At( star_pos, 8.0f );
		if ( id < 0 ) {
			id = 0;
		}

		if ( !BridgeSystem::Break_Span_At( id, star_pos ) ) {
			Print( "bridge_break: bridge %d has no span to break there.\n", id );
			return;
		}

		float from_start = 0.0f;
		float from_end = 0.0f;
		BridgeSystem::Get_Traversable_Extent( id, &from_start, &from_end );

		Print( "bridge_break: bridge %d is %s; %.0fm reachable from one end, %.0fm from the other.\n",
				 id, BridgeSystem::Is_Traversable( id ) ? "still crossable" : "cut",
				 from_start, from_end );
	}
};


class BridgeClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "bridge_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "BRIDGE_CLEAR - removes generated bridges and their geometry from the scene."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( BridgeSystem::Get_Bridge_Count() == 0 ) {
			Print( "There are no generated bridges to clear.\n" );
			return;
		}

		BridgeSystem::Clear_Bridges();
		Print( "Generated bridges cleared.\n" );
	}
};


/*
**	Section 21's acceptance is a ratio between the number of plants in a wood and the number of
**	things the wood is drawn as, and a ratio is a thing to read rather than to look at.  This
**	plants a wood around the player and prints both numbers.
**
**	It takes a model name because there is no foliage art of our own and there are perfectly
**	good trees in a stock Renegade mix file.  Pointing the type at one is how the batching is
**	seen working on art that exists rather than only counted on art that does not.
*/
class FoliageTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "foliage_test"; }
	virtual	const char * Get_Help( void ) override	{ return "FOLIAGE_TEST <count> <radius> <model> - plants a wood around you and reports how many things it is drawn as."; }
	virtual	void Activate( const char * input ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "foliage_test needs somewhere to put the wood; there is no player.\n" );
			return;
		}

		int	count		= 500;
		float	radius	= 120.0f;
		char	model[ 128 ];
		model[ 0 ] = 0;

		if ( input != nullptr ) {
			::sscanf( input, "%d %f %127s", &count, &radius, model );
		}

		if ( count < 1 )			count = 1;
		if ( count > 20000 )		count = 20000;
		if ( radius < 4.0f )		radius = 4.0f;
		if ( radius > 2000.0f )	radius = 2000.0f;

		if ( FoliageSystem::Get_Type_Count() == 0 ) {
			FoliageSystem::Init();
		}

		FoliageTypeClass * type = FoliageSystem::Find_Type( "ow_tree_conifer" );
		if ( type == nullptr ) {
			Print( "foliage_test: the default tree is not defined.\n" );
			return;
		}

		//	A model given on the command line replaces the one the type names, so that the wood
		//	can be planted out of whatever trees this level already has.
		if ( model[ 0 ] != 0 ) {
			type->Set_Model( model );
		}

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		FoliageSystem::Clear_Instances();
		FoliageSystem::Scatter( "ow_tree_conifer", star_pos, radius, count, 1234 );

		if ( !FoliageSystem::Build_Cells( 32.0f ) ) {
			Print( "foliage_test: the wood would not lay out.\n" );
			return;
		}
		if ( !FoliageSystem::Build_Batches() ) {
			Print( "foliage_test: the wood would not batch.\n" );
			return;
		}

		bool built = FoliageSystem::Build_Geometry();

		Vector3 facing( 1.0f, 0.0f, 0.0f );
		FoliageSystem::Update_Visibility( star_pos, facing );

		Print( "foliage_test: %d plants in %d cell(s), drawn as %d batch(es) -- %d from here.\n",
				 FoliageSystem::Get_Instance_Count(),
				 FoliageSystem::Get_Cell_Count(),
				 FoliageSystem::Get_Batch_Count(),
				 FoliageSystem::Get_Visible_Batch_Count() );

		Print( "foliage_test: %d collision proxy object(s), %d object(s) in the scene.\n",
				 FoliageSystem::Get_Proxy_Count(),
				 FoliageSystem::Get_Object_Count() );

		if ( !built ) {
			Print( "foliage_test: no geometry was built.\n" );
		}
		if ( FoliageSystem::Get_Missing_Model_Count() > 0 ) {
			Print( "foliage_test: %d type(s) name no model that loads, so their batches draw nothing.\n",
					 FoliageSystem::Get_Missing_Model_Count() );
		}
		if ( FoliageSystem::Get_Multi_Material_Model_Count() > 0 ) {
			Print( "foliage_test: %d model(s) use more than one material; only the first is drawn.\n",
					 FoliageSystem::Get_Multi_Material_Model_Count() );
		}
		if ( FoliageSystem::Get_Oversized_Batch_Count() > 0 ) {
			Print( "foliage_test: %d batch(es) were too large to build.\n",
					 FoliageSystem::Get_Oversized_Batch_Count() );
		}
	}
};


class FoliageCutConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "foliage_cut"; }
	virtual	const char * Get_Help( void ) override	{ return "FOLIAGE_CUT - destroys the nearest destructible plant."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "foliage_cut needs to know where you are standing.\n" );
			return;
		}
		if ( FoliageSystem::Get_Instance_Count() == 0 ) {
			Print( "There is nothing planted to cut.\n" );
			return;
		}

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		int index = FoliageSystem::Find_Nearest_Instance( star_pos, 30.0f );
		if ( index < 0 ) {
			Print( "foliage_cut: nothing is planted within thirty metres of you.\n" );
			return;
		}

		if ( !FoliageSystem::Destroy_Instance( index ) ) {
			Print( "foliage_cut: plant %d would not come down.\n", index );
			return;
		}

		Print( "foliage_cut: plant %d is down; %d of %d still standing.\n",
				 index, FoliageSystem::Get_Live_Instance_Count(),
				 FoliageSystem::Get_Instance_Count() );
	}
};


class FoliageClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "foliage_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "FOLIAGE_CLEAR - removes generated foliage and its geometry from the scene."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( FoliageSystem::Get_Instance_Count() == 0 ) {
			Print( "There is no generated foliage to clear.\n" );
			return;
		}

		FoliageSystem::Clear_Instances();
		Print( "Generated foliage cleared.\n" );
	}
};


/*
**	Section 22's acceptance is that water works visually and physically without a competing
**	collision system, and both halves of that are things to read as much as things to look at:
**	whether the ground carved, whether the shoreline mask stamped, how many things the surface
**	is drawn as.  This places a round pond around the player and reports all three.
**
**	It takes a texture name for the same reason foliage_test takes a model name: there is no
**	water art of our own yet, and pointing the pond at a texture already in a stock mix file is
**	how the shader and the collision tagging are seen working before any of ours exists.
*/
class WaterTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "water_test"; }
	virtual	const char * Get_Help( void ) override	{ return "WATER_TEST <radius> <texture> - places a pond around you and reports how it carved, stamped and drew."; }
	virtual	void Activate( const char * input ) override {

		if ( COMBAT_STAR == nullptr ) {
			Print( "water_test needs somewhere to put the pond; there is no player.\n" );
			return;
		}

		float	radius	= 20.0f;
		char	texture[ 128 ];
		texture[ 0 ] = 0;

		if ( input != nullptr ) {
			::sscanf( input, "%f %127s", &radius, texture );
		}

		if ( radius < 4.0f )		radius = 4.0f;
		if ( radius > 200.0f )	radius = 200.0f;

		if ( WaterSystem::Get_Definition_Count() == 0 ) {
			WaterSystem::Init();
			WaterSystem::Define_Default_Water();
		}

		WaterDefinitionClass * def = WaterSystem::Find_Definition( "ow_water_pond" );
		if ( def == nullptr ) {
			Print( "water_test: the default pond is not defined.\n" );
			return;
		}

		//	A texture given on the command line replaces the one the kind names, so the pond can
		//	be drawn out of whatever texture this level already has.
		if ( texture[ 0 ] != 0 ) {
			def->Set_Surface_Texture( texture );
		}

		Vector3 star_pos;
		COMBAT_STAR->Get_Position( &star_pos );

		WaterSystem::Clear_Areas();

		WaterAreaClass pond;
		pond.Set_Name( "water_test_pond" );
		pond.Set_Definition( "ow_water_pond" );
		pond.Set_Closed( true );
		pond.Set_Height( star_pos.Z );

		const int SIDES = 16;
		for ( int i = 0; i < SIDES; i ++ ) {
			float angle = ( 2.0f * WWMATH_PI * (float)i ) / (float)SIDES;
			Vector3 point( star_pos.X + radius * WWMath::Cos( angle ),
								star_pos.Y + radius * WWMath::Sin( angle ), 0.0f );
			pond.Add_Station( point, 0.0f );
		}

		int index = WaterSystem::Add_Area( pond );

		bool shaped = WaterSystem::Shape_Terrain( index );
		bool stamped = TerrainTextureSystem::Has_Masks() && WaterSystem::Stamp_Mask( index );
		if ( stamped ) {
			TerrainTextureSystem::Update_Water_Distance();
		}
		bool built = WaterSystem::Build_Geometry( index );

		Print( "water_test: a pond of radius %.0f -- ground %s, shoreline %s, surface %s.\n",
				 radius,
				 shaped ? "carved" : "not carved (no terrain)",
				 stamped ? "stamped" : "not stamped (no terrain texture masks)",
				 built ? "drawn" : "not drawn (no texture, or no physics scene)" );

		Print( "water_test: %d object(s) in the scene.\n", WaterSystem::Get_Object_Count() );

		if ( WaterSystem::Get_Missing_Texture_Count() > 0 ) {
			Print( "water_test: %d kind(s) name no texture, so their surface draws nothing.\n",
					 WaterSystem::Get_Missing_Texture_Count() );
		}
	}
};


class WaterClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "water_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "WATER_CLEAR - removes generated water areas and their geometry from the scene."; }
	virtual	void Activate( const char * /* input */ ) override {

		if ( WaterSystem::Get_Area_Count() == 0 ) {
			Print( "There is no generated water to clear.\n" );
			return;
		}

		WaterSystem::Clear_Areas();
		Print( "Generated water cleared.\n" );
	}
};


/*
**	Marks on the ground -- roadmap Section 23.
**
**	There is nothing to place: every vehicle in the level is already laying marks off its own
**	wheel contacts, and has been since the physics library started.  What is missing is a
**	texture to draw them with, so that is the whole of what this command supplies -- point the
**	five kinds at a texture this level already has, then drive something over soft ground.
*/
class RibbonTextureConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "ribbon_texture"; }
	virtual	const char * Get_Help( void ) override	{ return "RIBBON_TEXTURE <texture> - draws every kind of ground mark with the named texture.  No argument clears it again."; }
	virtual	void Activate( const char * input ) override {

		char	texture[ 128 ];
		texture[ 0 ] = 0;

		if ( input != nullptr ) {
			::sscanf( input, "%127s", texture );
		}

		if ( SurfaceRibbonSystem::Get_Definition_Count() == 0 ) {
			SurfaceRibbonSystem::Define_Default_Ribbons();
		}

		int count = SurfaceRibbonSystem::Get_Definition_Count();
		for ( int i = 0; i < count; i ++ ) {
			SurfaceRibbonDefinitionClass def = SurfaceRibbonSystem::Peek_Definition( i );
			def.Set_Texture( texture );
			SurfaceRibbonSystem::Define_Definition( def );
		}

		//	The meshes were made against the old texture, or not made at all.  Let them go and
		//	they are made again on the next frame with whatever was just named.
		SurfaceRibbonSystem::Destroy_Geometry();

		if ( texture[ 0 ] != 0 ) {
			Print( "ribbon_texture: %d kind(s) of ground mark will draw with %s.\n", count, texture );
			Print( "ribbon_texture: drive a vehicle over dirt, sand, mud, grass or snow.\n" );
		} else {
			Print( "ribbon_texture: %d kind(s) of ground mark draw nothing again.\n", count );
		}
	}
};


class RibbonStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "ribbon_status"; }
	virtual	const char * Get_Help( void ) override	{ return "RIBBON_STATUS - reports how many ground marks exist and how many things are drawn to show them."; }
	virtual	void Activate( const char * /* input */ ) override {

		Print( "ribbon_status: %d of %d ribbon(s) in use, %d mark(s) laid.\n",
				 SurfaceRibbonSystem::Get_Active_Ribbon_Count(),
				 SurfaceRibbonSystem::Get_Pool_Size(),
				 SurfaceRibbonSystem::Get_Edge_Count() );

		//	This line is roadmap Section 23's acceptance, printed: however many marks exist, the
		//	number of things drawn to show them is at most the number of kinds of mark.
		Print( "ribbon_status: %d object(s) drawing %d polygon(s).\n",
				 SurfaceRibbonSystem::Get_Object_Count(),
				 SurfaceRibbonSystem::Get_Poly_Count() );

		if ( SurfaceRibbonSystem::Get_Missing_Texture_Count() > 0 ) {
			Print( "ribbon_status: %d kind(s) are being laid with no texture, so they draw nothing.\n",
					 SurfaceRibbonSystem::Get_Missing_Texture_Count() );
			Print( "ribbon_status: try ribbon_texture <a texture name from this level>.\n" );
		}

		if ( SurfaceRibbonSystem::Get_Bind_Failure_Count() > 0 ) {
			Print( "ribbon_status: %d time(s) the pool was full and a mark was refused rather than allocated.\n",
					 SurfaceRibbonSystem::Get_Bind_Failure_Count() );
		}
	}
};


class RibbonClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "ribbon_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "RIBBON_CLEAR - erases every ground mark without stopping anything from laying new ones."; }
	virtual	void Activate( const char * /* input */ ) override {

		SurfaceRibbonSystem::Clear_Marks();
		Print( "Ground marks cleared.\n" );
	}
};


class MarkTextureConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "mark_texture"; }
	virtual	const char * Get_Help( void ) override	{ return "MARK_TEXTURE <texture> - draws every kind of surface mark with the named texture.  No argument clears it again."; }
	virtual	void Activate( const char * input ) override {

		char	texture[ 128 ];
		texture[ 0 ] = 0;

		if ( input != nullptr ) {
			::sscanf( input, "%127s", texture );
		}

		if ( WorldSurfaceMarkManager::Get_Definition_Count() == 0 ) {
			WorldSurfaceMarkManager::Define_Default_Marks();
		}

		int count = WorldSurfaceMarkManager::Get_Definition_Count();
		for ( int i = 0; i < count; i ++ ) {
			SurfaceMarkDefinitionClass def = WorldSurfaceMarkManager::Peek_Definition( i );
			def.Set_Texture( texture );
			WorldSurfaceMarkManager::Define_Definition( def );
		}

		//	The meshes were made against the old texture, or not made at all.  Let them go and
		//	they are made again on the next frame with whatever was just named.
		WorldSurfaceMarkManager::Destroy_Geometry();

		if ( texture[ 0 ] != 0 ) {
			Print( "mark_texture: %d kind(s) of surface mark will draw with %s.\n", count, texture );
			Print( "mark_texture: try mark_test 200, or blow something up.\n" );
		} else {
			Print( "mark_texture: %d kind(s) of surface mark draw nothing again.\n", count );
		}
	}
};


class MarkTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "mark_test"; }
	virtual	const char * Get_Help( void ) override	{ return "MARK_TEST <count> [radius] - scatters that many marks on the ground around the camera."; }
	virtual	void Activate( const char * input ) override {

		int	count		= 200;
		float	radius	= 1.0f;

		if ( input != nullptr ) {
			::sscanf( input, "%d %f", &count, &radius );
		}
		if ( count < 1 )		{ count = 1; }
		if ( count > 4096 )	{ count = 4096; }
		if ( radius <= 0.0f )	{ radius = 1.0f; }

		if ( WorldSurfaceMarkManager::Get_Definition_Count() == 0 ) {
			WorldSurfaceMarkManager::Define_Default_Marks();
		}

		int definition = WorldSurfaceMarkManager::Find_Definition_By_Type( SURFACE_MARK_SCORCH );
		if ( definition < 0 ) {
			Print( "mark_test: no scorch is defined.\n" );
			return ;
		}

		if ( COMBAT_SCENE == nullptr ) {
			Print( "mark_test: no scene.\n" );
			return ;
		}

		Vector3 center = COMBAT_SCENE->Get_Last_Camera_Position();

		//	Spiral outward from where the camera is, so that however many are asked for they
		//	land on ground the player can walk to and look at.
		int placed		= 0;
		int refused		= 0;
		for ( int i = 0; i < count; i ++ ) {

			float angle	= float( i ) * 2.39996323f;			// the golden angle, so it does not band
			float dist	= 2.0f + ( 1.2f * WWMath::Sqrt( float( i ) ) );

			Vector3 spot( center.X + ( dist * WWMath::Cos( angle ) ),
							  center.Y + ( dist * WWMath::Sin( angle ) ),
							  center.Z );

			float height = spot.Z;
			if ( !WorldSurfaceMarkManager::Conform_Point( spot.X, spot.Y, spot.Z, &height ) ) {
				refused ++;
				continue;
			}
			spot.Z = height;

			if ( WorldSurfaceMarkManager::Add_Mark( definition, spot, Vector3( 0.0f, 0.0f, 1.0f ), radius ) != 0 ) {
				placed ++;
			} else {
				refused ++;
			}
		}

		Print( "mark_test: %d placed, %d refused (no ground, or the drape was too steep).\n",
				 placed, refused );
		Print( "mark_test: %d mark(s) now drawn by %d object(s).  mark_status for the rest.\n",
				 WorldSurfaceMarkManager::Get_Mark_Count(),
				 WorldSurfaceMarkManager::Get_Object_Count() );
	}
};


class MarkStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "mark_status"; }
	virtual	const char * Get_Help( void ) override	{ return "MARK_STATUS - how many marks the world holds and how many draw calls they cost."; }
	virtual	void Activate( const char * /* input */ ) override {

		int marks		= WorldSurfaceMarkManager::Get_Mark_Count();
		int batched		= WorldSurfaceMarkManager::Get_Batched_Mark_Count();
		int projected	= WorldSurfaceMarkManager::Get_Projected_Mark_Count();

		Print( "mark_status: %d mark(s) of %d in the pool -- %d batched, %d clipped (budget %d).\n",
				 marks, WorldSurfaceMarkManager::Get_Pool_Size(), batched, projected,
				 WorldSurfaceMarkManager::Get_Projected_Budget() );

		//	This is the acceptance in two numbers: marks go up, objects do not.
		Print( "mark_status: %d group(s), %d object(s), %d polygon(s).\n",
				 WorldSurfaceMarkManager::Get_Definition_Count(),
				 WorldSurfaceMarkManager::Get_Object_Count(),
				 WorldSurfaceMarkManager::Get_Poly_Count() );

		Print( "mark_status: %d eviction(s), %d group refusal(s), draw distance %.1f.\n",
				 WorldSurfaceMarkManager::Get_Eviction_Count(),
				 WorldSurfaceMarkManager::Get_Group_Refusal_Count(),
				 WorldSurfaceMarkManager::Get_Draw_Distance() );

		int missing = WorldSurfaceMarkManager::Get_Missing_Texture_Count();
		if ( missing > 0 ) {
			Print( "mark_status: %d kind(s) have marks and no texture -- see docs/assets/SurfaceMarks.md.\n",
					 missing );
		}
	}
};


class MarkClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "mark_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "MARK_CLEAR - erases every surface mark in the world."; }
	virtual	void Activate( const char * /* input */ ) override {

		WorldSurfaceMarkManager::Clear_Marks();
		Print( "Surface marks cleared.\n" );
	}
};


class ShadowStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "shadow_status"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_STATUS - how many shadows are cached and how many are being re-rendered."; }
	virtual	void Activate( const char * /* input */ ) override {

		static const char * mode_names[] = { "off", "blobs", "blobs+", "hardware" };

		int mode = WorldShadowManager::Get_Mode();
		const char * mode_name = ((mode >= 0) && (mode < 4)) ? mode_names[ mode ] : "?";

		Print( "shadow_status: mode %s, static %u, dynamic %u, %u simultaneous.\n",
				 mode_name,
				 WorldShadowManager::Get_Static_Resolution(),
				 WorldShadowManager::Get_Dynamic_Resolution(),
				 WorldShadowManager::Get_Max_Simultaneous_Shadows() );

		//	This is the section stated in two numbers: hits should dominate renders whenever
		//	the things casting shadows are holding still.
		Print( "shadow_status: %d render(s) and %d cache hit(s) last frame.\n",
				 WorldShadowManager::Get_Texture_Render_Count(),
				 WorldShadowManager::Get_Cache_Hit_Count() );

		Print( "shadow_status: %d of %d slot(s) held, %d render target(s) made, %d eviction(s).\n",
				 WorldShadowManager::Get_Held_Slot_Count(),
				 (int)WorldShadowManager::Get_Max_Simultaneous_Shadows(),
				 WorldShadowManager::Get_Render_Target_Count(),
				 WorldShadowManager::Get_Eviction_Count() );

		Print( "shadow_status: %d static shadow texture(s) shared, %d registered caster(s), %d refusal(s).\n",
				 WorldShadowManager::Get_Static_Shadow_Texture_Count(),
				 WorldShadowManager::Get_Registered_Caster_Count(),
				 WorldShadowManager::Get_Caster_Refusal_Count() );

		int missing = WorldShadowManager::Get_Missing_Target_Count();
		if ( missing > 0 ) {
			Print( "shadow_status: %d shadow(s) went without a render target -- the device refused one.\n",
					 missing );
		}
	}
};


class ShadowModeConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "shadow_mode"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_MODE <0-3> - 0 off, 1 blobs, 2 blobs plus a real one for the player, 3 rendered."; }
	virtual	void Activate( const char * input ) override {

		int mode = -1;
		if ( input != nullptr ) {
			::sscanf( input, "%d", &mode );
		}

		if ( ( mode < 0 ) || ( mode >= WorldShadowManager::SHADOW_MODE_COUNT ) ) {
			Print( "shadow_mode: the mode is %d.  0 off, 1 blobs, 2 blobs+, 3 rendered.\n",
					 WorldShadowManager::Get_Mode() );
			return ;
		}

		//	Through the scene, because that is the name every script in the tree spells and
		//	this is a good place to notice if the two ever stop agreeing.
		if ( COMBAT_SCENE != nullptr ) {
			COMBAT_SCENE->Set_Shadow_Mode( (PhysicsSceneClass::ShadowEnum)mode );
		} else {
			WorldShadowManager::Set_Mode( mode );
		}

		Print( "shadow_mode: mode %d, %u simultaneous rendered shadow(s).\n",
				 WorldShadowManager::Get_Mode(),
				 WorldShadowManager::Get_Max_Simultaneous_Shadows() );
	}
};


class ShadowInvalidateConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "shadow_invalidate"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_INVALIDATE - throws away every cached shadow picture so they are all taken again."; }
	virtual	void Activate( const char * /* input */ ) override {

		WorldShadowManager::Invalidate_All();

		if ( COMBAT_SCENE != nullptr ) {
			COMBAT_SCENE->Invalidate_Static_Shadow_Projectors();
			COMBAT_SCENE->Generate_Static_Shadow_Projectors();
		}

		Print( "Every cached shadow will be rendered again.\n" );
	}
};


/*
**	Roadmap Section 26.  The acceptance is a ratio -- emitters per draw call -- so the command
**	that reports it prints that ratio and not much else.  The counters run from whenever they
**	were last cleared, which makes the ratio meaningful over any interval and the raw totals
**	meaningful over none; `particle_status clear` starts a fresh one.
*/
class ParticleStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "particle_status"; }
	virtual	const char * Get_Help( void ) override	{ return "PARTICLE_STATUS [clear] - how many emitters the particle pools drew, and in how many draw calls."; }
	virtual	void Activate( const char * input ) override {

		if ( input != nullptr && ::strstr( input, "clear" ) != nullptr ) {
			WorldParticleManager::Reset_Statistics();
			Print( "particle_status: counters cleared.\n" );
			return;
		}

		Print( "particle_status: batching is %s.\n",
				 WorldParticleManager::Is_Enabled() ? "on" : "off" );

		for ( int p = 0; p < PARTICLE_POOL_COUNT; p ++ ) {

			ParticlePoolType pool = (ParticlePoolType)p;

			unsigned int submissions = WorldParticleManager::Get_Submission_Count( pool );
			unsigned int batches = WorldParticleManager::Get_Batch_Count( pool );
			unsigned int submitted = WorldParticleManager::Get_Submitted_Particles( pool );
			unsigned int drawn = WorldParticleManager::Get_Drawn_Particles( pool );
			unsigned int thinned = WorldParticleManager::Get_Thinned_Particles( pool );
			unsigned int refused = WorldParticleManager::Get_Refused_Submission_Count( pool );

			if ( submissions == 0 && batches == 0 ) {
				Print( "particle_status: %s pool idle, capacity %d.\n",
						 WorldParticleManager::Get_Pool_Name( pool ),
						 WorldParticleManager::Get_Pool_Capacity( pool ) );
				continue;
			}

			//	This is the number the acceptance is about.  In the stock engine it is 1.00 by
			//	construction, because every emitter was its own draw call.
			float ratio = ( batches > 0 ) ? ( (float)submissions / (float)batches ) : 0.0f;

			Print( "particle_status: %s -- %u emitter(s) in %u draw call(s), %.1f per call.\n",
					 WorldParticleManager::Get_Pool_Name( pool ),
					 submissions, batches, ratio );

			Print( "particle_status: %s -- %u of %u particle(s) drawn, %u thinned, capacity %d.\n",
					 WorldParticleManager::Get_Pool_Name( pool ),
					 drawn, submitted, thinned,
					 WorldParticleManager::Get_Pool_Capacity( pool ) );

			if ( thinned > 0 ) {
				for ( int b = 0; b < PARTICLE_BUDGET_COUNT; b ++ ) {
					ParticleBudgetType budget = (ParticleBudgetType)b;
					unsigned int charged = WorldParticleManager::Get_Budget_Particles( pool, budget );
					if ( charged > 0 ) {
						Print( "particle_status:   %s drew %u.\n",
								 WorldParticleManager::Get_Budget_Name( budget ), charged );
					}
				}
			}

			if ( refused > 0 ) {
				Print( "particle_status: %s -- %u submission(s) refused; more than %d materials at once.\n",
						 WorldParticleManager::Get_Pool_Name( pool ), refused, 64 );
			}
		}
	}
};


/*
**	The switch that makes "this changes how it is drawn, not what is drawn" checkable by eye.
**	Off, every particle buffer draws itself exactly as stock Renegade does.
*/
class ParticleBatchingConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "particle_batching"; }
	virtual	const char * Get_Help( void ) override	{ return "PARTICLE_BATCHING [on|off] - share one draw call between emitters, or let each draw itself."; }
	virtual	void Activate( const char * input ) override {

		if ( input != nullptr && ::strstr( input, "off" ) != nullptr ) {
			WorldParticleManager::Set_Enabled( false );
		} else if ( input != nullptr && ::strstr( input, "on" ) != nullptr ) {
			WorldParticleManager::Set_Enabled( true );
		}

		Print( "particle_batching: %s.\n", WorldParticleManager::Is_Enabled() ? "on" : "off" );
	}
};


/*
**	How big a pool is.  Resizing reallocates the staging arrays, so it is a thing to do once
**	while looking for the right number and not a thing to do per frame.
*/
class ParticlePoolConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "particle_pool"; }
	virtual	const char * Get_Help( void ) override	{ return "PARTICLE_POOL <sprite|point|streak> <particles> - how many particles a shared pool may draw at once."; }
	virtual	void Activate( const char * input ) override {

		char	name[ 32 ] = { 0 };
		int	capacity = 0;

		if ( input == nullptr || ::sscanf( input, "%31s %d", name, &capacity ) != 2 || capacity < 1 ) {
			for ( int p = 0; p < PARTICLE_POOL_COUNT; p ++ ) {
				Print( "particle_pool: %s holds %d.\n",
						 WorldParticleManager::Get_Pool_Name( (ParticlePoolType)p ),
						 WorldParticleManager::Get_Pool_Capacity( (ParticlePoolType)p ) );
			}
			return;
		}

		for ( int p = 0; p < PARTICLE_POOL_COUNT; p ++ ) {
			if ( ::stricmp( name, WorldParticleManager::Get_Pool_Name( (ParticlePoolType)p ) ) == 0 ) {
				WorldParticleManager::Set_Pool_Capacity( (ParticlePoolType)p, capacity );
				Print( "particle_pool: %s now holds %d.\n",
						 WorldParticleManager::Get_Pool_Name( (ParticlePoolType)p ),
						 WorldParticleManager::Get_Pool_Capacity( (ParticlePoolType)p ) );
				return;
			}
		}

		Print( "particle_pool: there is no pool called '%s'.\n", name );
	}
};


class DebugLayerConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "debug_layer"; }
	virtual	const char * Get_Help( void ) override
	{
		return "DEBUG_LAYER <name> <0|1> - roadmap Section 33; toggle one world debug overlay layer. "
				 "With no arguments, lists every layer name and whether it is currently on.";
	}
	virtual	void Activate( const char * input ) override
	{
		char	name [64] = { 0 };
		int		onoff = -1;

		if ( input == nullptr || input[0] == '\0' ||
			  ::sscanf( input,"%63s %d",name,&onoff ) < 2 ) {

			Print( "debug_layer: usage 'debug_layer <name> <0|1>'.  Layers:\n" );
			for ( int i = 0; i < DEBUG_LAYER_COUNT; i++ ) {
				DebugLayerType layer = (DebugLayerType)i;
				Print( "debug_layer:   %-28s %s\n",
						 DebugRenderSystem::Get_Layer_Name( layer ),
						 DebugRenderSystem::Is_Layer_Enabled( layer ) ? "on" : "off" );
			}
			return;
		}

		for ( int i = 0; i < DEBUG_LAYER_COUNT; i++ ) {
			DebugLayerType layer = (DebugLayerType)i;
			if ( ::strcmp( DebugRenderSystem::Get_Layer_Name( layer ),name ) == 0 ) {
				DebugRenderSystem::Set_Layer_Enabled( layer,onoff != 0 );
				Print( "debug_layer: %s is now %s.\n",name,(onoff != 0) ? "on" : "off" );
				return;
			}
		}
		Print( "debug_layer: there is no layer called '%s'.\n",name );
	}
};

class DebugStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "debug_status"; }
	virtual	const char * Get_Help( void ) override	{ return "DEBUG_STATUS - how many debug lines are queued and how many were dropped."; }
	virtual	void Activate( const char * /* input */ ) override
	{
		Print( "debug_status: %d line(s) queued this frame, %d dropped, %d drawn last submit.\n",
				 DebugRenderSystem::Get_Line_Count(),
				 DebugRenderSystem::Get_Dropped_Count(),
				 DebugRenderSystem::Get_Drawn_Segment_Count() );
	}
};

class CommanderModeConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_mode"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_MODE <0|1> - roadmap Section 41; enter/exit the strategic camera without "
				 "leaving the embodied soldier.  Refuses to enter with no living star.";
	}
	virtual	void Activate( const char * input ) override
	{
		int onoff = -1;
		if ( input == nullptr || ::sscanf( input,"%d",&onoff ) != 1 ) {
			Print( "commander_mode: usage 'commander_mode <0|1>'.  Currently %s.\n",
					 CommanderModeController::Is_Active() ? "in Commander Mode" : "in FPS mode" );
			return;
		}

		if ( onoff != 0 ) {
			if ( CommanderModeController::Enter_Commander_Mode( COMBAT_STAR ) ) {
				Print( "commander_mode: entered Commander Mode.\n" );
			} else {
				Print( "commander_mode: refused -- no living star to embody, or already active.\n" );
			}
		} else {
			CommanderModeController::Exit_Commander_Mode();
			Print( "commander_mode: returned to FPS mode.\n" );
		}
	}
};

class CommanderStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_status"; }
	virtual	const char * Get_Help( void ) override	{ return "COMMANDER_STATUS - the current Commander Mode camera state."; }
	virtual	void Activate( const char * /* input */ ) override
	{
		Print( "commander_status: %s.\n",CommanderModeController::Is_Active() ? "COMMANDER_MODE" : "FPS_MODE" );
		if ( CommanderModeController::Is_Active() ) {
			Vector3 focus = CommanderModeController::Get_Focus();
			Print( "commander_status: focus %.1f,%.1f,%.1f  zoom %.1f  tilt %.1f  rotation %.1f\n",
					 focus.X,focus.Y,focus.Z,
					 CommanderModeController::Get_Zoom(),CommanderModeController::Get_Tilt(),
					 CommanderModeController::Get_Rotation() );
		}
	}
};

class ReloadBuildingDescriptorsConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "reload_building_descriptors"; }
	virtual	const char * Get_Help( void ) override
	{
		return "RELOAD_BUILDING_DESCRIPTORS - roadmap Section 39; re-read every file-backed building "
				 "descriptor.  A descriptor that fails validation keeps its last known-good values.";
	}
	virtual	void Activate( const char * /* input */ ) override
	{
		int before = BuildingDescriptorManager::Get_Descriptor_Count ();
		int succeeded = BuildingDescriptorManager::Reload_All_Descriptors ();
		Print( "reload_building_descriptors: %d of %d descriptor(s) reloaded successfully.\n",succeeded,before );
		if ( BuildingDescriptorManager::Get_Last_Error()[0] != '\0' ) {
			Print( "reload_building_descriptors: last error: %s\n",BuildingDescriptorManager::Get_Last_Error() );
		}
	}
};

class ReloadBuildingDescriptorConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "reload_building_descriptor"; }
	virtual	const char * Get_Help( void ) override
	{
		return "RELOAD_BUILDING_DESCRIPTOR <definition> - roadmap Section 39; re-read one file-backed "
				 "building descriptor by definition name.";
	}
	virtual	void Activate( const char * input ) override
	{
		if ( input == nullptr || input[0] == '\0' ) {
			Print( "reload_building_descriptor: usage 'reload_building_descriptor <definition>'.\n" );
			return;
		}
		StringClass error;
		if ( BuildingDescriptorManager::Reload_Descriptor( input,&error ) ) {
			Print( "reload_building_descriptor: %s reloaded.\n",input );
		} else {
			Print( "reload_building_descriptor: %s was NOT reloaded -- last known-good values are still "
					 "active.  %s\n",input,error.Peek_Buffer() );
		}
	}
};

class BuildingJsonStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "building_json_status"; }
	virtual	const char * Get_Help( void ) override	{ return "BUILDING_JSON_STATUS - how many building descriptors are loaded, and the reload failure count."; }
	virtual	void Activate( const char * /* input */ ) override
	{
		Print( "building_json_status: %d descriptor(s) loaded, %d reload failure(s) total.\n",
				 BuildingDescriptorManager::Get_Descriptor_Count(),
				 BuildingDescriptorManager::Get_Reload_Failure_Count() );
	}
};

class TelemetryStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "telemetry_status"; }
	virtual	const char * Get_Help( void ) override
	{
		return "TELEMETRY_STATUS - roadmap Section 38; whether RTSTelemetryService is on, how "
				 "many records it holds, and how many events were dropped for want of a free slot.";
	}
	virtual	void Activate( const char * /* input */ ) override
	{
		DynamicVectorClass<RTSTelemetryService::SnapshotEntryClass> snapshot;
		int record_count = RTSTelemetryService::Get_Snapshot( snapshot );
		Print( "telemetry_status: %s, %d record(s), %d refusal(s) since the last reset.\n",
				 RTSTelemetryService::Is_Enabled() ? "enabled" : "disabled",
				 record_count,
				 RTSTelemetryService::Get_Refusal_Count() );
	}
};

class TelemetryDumpConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "telemetry_dump"; }
	virtual	const char * Get_Help( void ) override
	{
		return "TELEMETRY_DUMP - roadmap Section 38; print every category/subtype RTSTelemetryService "
				 "has a record for, its event count and its running total.";
	}
	virtual	void Activate( const char * /* input */ ) override
	{
		DynamicVectorClass<RTSTelemetryService::SnapshotEntryClass> snapshot;
		int record_count = RTSTelemetryService::Get_Snapshot( snapshot );
		if ( record_count == 0 ) {
			Print( "telemetry_dump: no records.\n" );
			return;
		}
		for ( int index = 0; index < snapshot.Count(); index++ ) {
			const RTSTelemetryService::SnapshotEntryClass & entry = snapshot[index];
			Print( "telemetry_dump: %s subtype=%d count=%d total=%.2f\n",
					 RTSTelemetryService::Get_Category_Name( entry.Category ),
					 entry.Subtype, entry.Count, entry.Total );
		}
	}
};


/*
**	Roadmap Sections 42-46, Phases 36-40 -- driving the Commander layer from a console.
**
**	These are not the Commander's real input: Section 42 wants a panel with clickable cards and
**	Section 35's input rerouting is a later phase.  They exist because every rule underneath is
**	live and server-authoritative now, and without a way to reach it from a running game the
**	whole layer could only ever be exercised by a self check.  Each one goes through exactly the
**	same path the eventual UI will -- the model, then a net event -- rather than reaching past it.
*/
class CommanderSidebarConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_sidebar"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_SIDEBAR [mode] - roadmap Section 42.  No argument prints the panel's "
				 "state.  mode 0-3 switches to BUILDINGS/GROUND_VEHICLES/AIR_VEHICLES/INFANTRY; "
				 "'next' and 'prev' page through the cards.";
	}
	virtual	void Activate( const char * input ) override
	{
		if ( !CommanderSidebarModel::Is_Open() ) {
			Print( "commander_sidebar: the panel is closed -- enter Commander Mode first "
					 "(commander_mode 1).\n" );
			return;
		}

		if ( input != nullptr && input[0] != '\0' ) {
			if ( ::strcmp( input, "next" ) == 0 ) {
				CommanderSidebarModel::Next_Page();
			} else if ( ::strcmp( input, "prev" ) == 0 ) {
				CommanderSidebarModel::Prev_Page();
			} else {
				int mode = ::atoi( input );
				if ( mode >= 0 && mode < SIDEBAR_MODE_COUNT ) {
					CommanderSidebarModel::Set_Mode( (CommanderSidebarModeType)mode );
				} else {
					Print( "commander_sidebar: mode must be 0-3, 'next' or 'prev'.\n" );
					return;
				}
			}
		}

		//	Credits come from the local star, the same wallet a purchase would be charged to.
		int credits = 0;
		if ( COMBAT_STAR != nullptr && COMBAT_STAR->Get_Player_Data() != nullptr ) {
			credits = (int)COMBAT_STAR->Get_Player_Data()->Get_Money();
		}
		CommanderSidebarModel::Refresh( credits );

		Print( "commander_sidebar: mode %s, page %d/%d, %d card(s), %d credit(s).\n",
				 CommanderSidebarModel::Get_Mode_Name( CommanderSidebarModel::Get_Mode() ),
				 CommanderSidebarModel::Get_Page() + 1,
				 CommanderSidebarModel::Get_Page_Count(),
				 CommanderSidebarModel::Get_Card_Count(),
				 credits );

		const CommanderSidebarCard * cards[CommanderSidebarModel::PAGE_SIZE] = { nullptr };
		int visible = CommanderSidebarModel::Get_Visible_Cards( cards, CommanderSidebarModel::PAGE_SIZE );
		for ( int i = 0; i < visible; i++ ) {
			if ( cards[i] == nullptr ) continue;
			const char * name = ( cards[i]->DescriptorName.Get_Length() > 0 )
										? cards[i]->DescriptorName.Peek_Buffer()
										: cards[i]->DisplayName.Peek_Buffer();
			Print( "  [%d] %-24s cost %-6d def %-6d %s\n", i,
					 ( name[0] != '\0' ) ? name : "(unnamed)",
					 cards[i]->Cost, cards[i]->DefinitionId,
					 CommanderSidebarModel::Get_Status_Name( cards[i]->Status ) );
		}

		CommanderPowerDisplay power;
		CommanderSidebarModel::Get_Power_Display( &power );
		Print( "  power: gen %.0f, requested %.0f, allocated %.0f, headroom %.0f, %d shed%s\n",
				 power.Generation, power.RequestedDemand, power.AllocatedDemand, power.Headroom,
				 power.ShedCount, power.Overloaded ? ", OVERLOADED" : "" );
	}
};


/*
**	Q-014 -- the wallet cap, inspectable and tunable.
**
**	The base is a balance number that stock Renegade never had (its economy is uncapped), so
**	it is exposed rather than compiled in: an operator can find the right ceiling without a
**	rebuild, and lowering it takes effect immediately on players who are already over it,
**	exactly as losing a Silo does.
*/
class WalletCapConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "wallet_cap"; }
	virtual	const char * Get_Help( void ) override
	{
		return "WALLET_CAP [credits] - roadmap Section 40.  No argument prints the base cap, "
				 "each team's Silo bonus and effective cap, and how many credits cap drops have "
				 "wiped.  An argument sets the base cap and enforces it at once.";
	}
	virtual	void Activate( const char * input ) override
	{
		if ( input != nullptr && input[0] != '\0' ) {
			float requested = (float)::atof( input );
			if ( requested < 0.0f ) {
				Print( "wallet_cap: the base cap cannot be negative.\n" );
				return;
			}
			TeamPowerGridService::Set_Base_Wallet_Cap( requested );
		}

		Print( "wallet_cap: base %.0f\n",TeamPowerGridService::Get_Base_Wallet_Cap() );

		static const int TEAMS[2] = { PLAYERTYPE_GDI, PLAYERTYPE_NOD };
		static const char * const NAMES[2] = { "GDI", "Nod" };

		for ( int i = 0; i < 2; i++ ) {
			Print( "  %-3s silo bonus %8.0f, effective cap %8.0f, wiped %8.0f\n",
					 NAMES[i],
					 TeamPowerGridService::Get_Wallet_Capacity_Bonus( TEAMS[i] ),
					 TeamPowerGridService::Get_Wallet_Capacity( TEAMS[i] ),
					 TeamPowerGridService::Get_Credits_Wiped( TEAMS[i] ) );
		}
	}
};


class CommanderGhostConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_ghost"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_GHOST <descriptor|off> [x y z] - roadmap Section 43.  Starts a "
				 "placement ghost for a buildable descriptor and reports whether the spot is "
				 "legal and why not.  With no position the ghost is placed at the star's feet.";
	}
	virtual	void Activate( const char * input ) override
	{
		if ( input == nullptr || input[0] == '\0' ) {
			Print( "commander_ghost: name a buildable descriptor, or 'off'.\n" );
			return;
		}

		if ( ::strcmp( input, "off" ) == 0 ) {
			CommanderPlacementValidator::Cancel_Ghost();
			Print( "commander_ghost: cancelled.\n" );
			return;
		}

		char name[128] = { 0 };
		float x = 0.0f, y = 0.0f, z = 0.0f;
		int parsed = ::sscanf( input, "%127s %f %f %f", name, &x, &y, &z );
		if ( parsed < 1 ) {
			Print( "commander_ghost: could not read a descriptor name.\n" );
			return;
		}

		int team = ( COMBAT_STAR != nullptr ) ? COMBAT_STAR->Get_Player_Type() : PLAYERTYPE_NEUTRAL;
		CommanderPlacementValidator::Begin_Ghost( name, team );

		if ( parsed < 4 ) {
			//	No position given: put it where the player is standing, which is the closest a
			//	console command can get to "at the cursor" without the input phase that owns it.
			Vector3 position( 0.0f, 0.0f, 0.0f );
			if ( COMBAT_STAR != nullptr ) {
				COMBAT_STAR->Get_Position( &position );
			}
			CommanderPlacementValidator::Move_Ghost( position );
		} else {
			CommanderPlacementValidator::Move_Ghost( Vector3( x, y, z ) );
		}

		const CommanderPlacementRequest & ghost = CommanderPlacementValidator::Get_Ghost();
		const CommanderPlacementResult & result = CommanderPlacementValidator::Get_Ghost_Result();
		Print( "commander_ghost: '%s' at %.1f,%.1f,%.1f rot %.0f -- %s\n",
				 name, ghost.Position.X, ghost.Position.Y, ghost.Position.Z, ghost.RotationDegrees,
				 CommanderPlacementValidator::Get_Result_Name( result.Result ) );
	}
};


class CommanderPlaceConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_place"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_PLACE [rotation] - roadmap Section 43.  Sends the current ghost to the "
				 "server as a placement request.  The server re-runs every check, so a ghost the "
				 "client drew red will be refused there too.";
	}
	virtual	void Activate( const char * input ) override
	{
		if ( !CommanderPlacementValidator::Is_Ghost_Active() ) {
			Print( "commander_place: no ghost -- start one with commander_ghost first.\n" );
			return;
		}

		if ( input != nullptr && input[0] != '\0' ) {
			CommanderPlacementValidator::Set_Ghost_Rotation( (float)::atof( input ) );
		}

		const CommanderPlacementRequest & ghost = CommanderPlacementValidator::Get_Ghost();

		//	Sent even when the client thinks it is invalid: "never trust client green/red state"
		//	cuts both ways, and a refusal the server prints is worth more than one the client
		//	swallows.
		cCommanderPlacementEvent * event = new cCommanderPlacementEvent;
		event->Init( ghost.DescriptorName.Peek_Buffer(), ghost.Position, ghost.RotationDegrees,
						 ghost.SequenceId );

		Print( "commander_place: sent '%s' at %.1f,%.1f,%.1f rot %.0f (client said %s).\n",
				 ghost.DescriptorName.Peek_Buffer(), ghost.Position.X, ghost.Position.Y,
				 ghost.Position.Z, ghost.RotationDegrees,
				 CommanderPlacementValidator::Get_Result_Name(
					 CommanderPlacementValidator::Get_Ghost_Result().Result ) );
	}
};


class CommanderBuildConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_build"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_BUILD <facility_id> <definition_id> - roadmap Section 45.  Asks the "
				 "server to queue a unit at a production building.  The server looks up the team, "
				 "the facility's state and the cost itself.";
	}
	virtual	void Activate( const char * input ) override
	{
		int facility_id = 0;
		int definition_id = 0;
		if ( input == nullptr || ::sscanf( input, "%d %d", &facility_id, &definition_id ) != 2 ) {
			Print( "commander_build: need a facility network id and a definition id.\n" );
			return;
		}

		cCommanderProductionEvent * event = new cCommanderProductionEvent;
		event->Init( facility_id, definition_id );

		Print( "commander_build: requested definition %d at facility %d; the server decides.\n",
				 definition_id, facility_id );
	}
};


class CommanderRTSStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_rts_status"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_RTS_STATUS - roadmap Sections 42-46.  Commander identities, live "
				 "constructions and their stage, production queues, and what the client mirror "
				 "has received.";
	}
	virtual	void Activate( const char * /* input */ ) override
	{
		Print( "commander_rts_status: Nod commander %d, GDI commander %d.\n",
				 CommanderProductionService::Get_Team_Commander( PLAYERTYPE_NOD ),
				 CommanderProductionService::Get_Team_Commander( PLAYERTYPE_GDI ) );

		//	Roadmap Section 25.1.  'commander_vote status' gives one team's own detail; this is
		//	the both-teams summary line, the same split commander_status/commander_rts_status
		//	already has for the camera layer.
		static const int GOVERNANCE_TEAMS [2] = { PLAYERTYPE_NOD,PLAYERTYPE_GDI };
		for ( int i = 0; i < 2; i++ ) {
			int team = GOVERNANCE_TEAMS [i];
			if ( !CommanderProductionService::Is_Vote_Active( team ) ) continue;
			Print( "  %s vote in progress: %s, candidate %d, %.1fs left, %d yes / %d no.\n",
					 ( team == PLAYERTYPE_NOD ) ? "Nod" : "GDI",
					 ( CommanderProductionService::Get_Vote_Type( team ) == COMMANDER_VOTE_ELECT ) ? "ELECT" : "REMOVE",
					 CommanderProductionService::Get_Vote_Candidate( team ),
					 CommanderProductionService::Get_Vote_Time_Remaining( team ),
					 CommanderProductionService::Get_Vote_Yes_Count( team ),
					 CommanderProductionService::Get_Vote_No_Count( team ) );
		}

		int active = BuildingConstructionService::Get_Active_Count();
		Print( "  %d construction(s) in progress:\n", active );
		for ( int i = 0; i < active; i++ ) {
			int id = BuildingConstructionService::Get_Active_Building( i );
			float progress = 0.0f;
			BuildingConstructionService::Get_Progress( id, &progress );
			Print( "    building %d: %.0f%% (%s), %d block(s), row %d of %d\n",
					 id, progress * 100.0f,
					 BuildingConstructionService::Get_Stage_Name(
						 BuildingConstructionService::Get_Stage( id ) ),
					 BuildingConstructionService::Get_Block_Count( id ),
					 BuildingConstructionService::Get_Active_Row( id ) + 1,
					 BuildingConstructionService::Get_Row_Count( id ) );
		}

		int items = CommanderProductionService::Get_Item_Count();
		Print( "  %d queued production item(s):\n", items );
		for ( int i = 0; i < items; i++ ) {
			CommanderProductionItem item;
			if ( !CommanderProductionService::Get_Item( i, &item ) ) continue;
			Print( "    facility %d: definition %d, %.1f/%.1fs, %s\n",
					 item.FacilityId, item.DefinitionId, item.ElapsedSeconds, item.BuildTimeSeconds,
					 CommanderProductionService::Get_Item_State_Name( item.State ) );
		}

		Print( "  %d unit(s) under Commander orders, %d selected.\n",
				 CommanderProductionService::Get_Order_Count(),
				 CommanderProductionService::Get_Selection_Count() );

		Print( "  mirror: %d packet(s) exported, %d imported; estimated server clock %.1fs.\n",
				 CommanderStateClass::Get_Export_Count(),
				 CommanderStateClass::Get_Import_Count(),
				 CommanderStateClass::Get_Estimated_Server_Time() );

		Print( "  renderers: %d construction batch(es), %d block(s) drawn, %d dropped; "
				 "ghost model %s.\n",
				 BuildingConstructionRenderer::Get_Batch_Count(),
				 BuildingConstructionRenderer::Get_Rendered_Block_Count(),
				 BuildingConstructionRenderer::Get_Dropped_Block_Count(),
				 CommanderGhostRenderer::Is_Model_Loaded() ? "loaded" : "not loaded" );
	}
};


/*
**	Roadmap Section 25.1 -- Phase 23 multiplayer Commander governance from a console, the same
**	"goes through exactly the same path the eventual UI will" discipline the block above this
**	one already documents: each of these sends the real net event and lets the server decide.
*/
class CommanderVoteConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_vote"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_VOTE <elect [client_id]|remove|yes|no|status> - roadmap Section 25.1.  "
				 "'elect' with no id nominates yourself for a vacant seat; 'remove' challenges "
				 "whoever holds it now.  'yes'/'no' cast a ballot on the team's active vote.";
	}
	virtual	void Activate( const char * input ) override
	{
		char verb[32] = { 0 };
		int candidate = 0;
		int parsed = ( input != nullptr ) ? ::sscanf( input,"%31s %d",verb,&candidate ) : 0;
		if ( parsed < 1 ) {
			Print( "commander_vote: usage 'commander_vote <elect [client_id]|remove|yes|no|status>'.\n" );
			return;
		}

		int team = ( COMBAT_STAR != nullptr ) ? COMBAT_STAR->Get_Player_Type() : PLAYERTYPE_NEUTRAL;

		if ( ::_stricmp( verb,"status" ) == 0 ) {
			if ( !CommanderProductionService::Is_Vote_Active( team ) ) {
				Print( "commander_vote: no active vote for this team.\n" );
				return;
			}
			Print( "commander_vote: %s vote, candidate %d, called by %d, %.1fs left, %d yes / %d no.\n",
					 ( CommanderProductionService::Get_Vote_Type( team ) == COMMANDER_VOTE_ELECT ) ? "ELECT" : "REMOVE",
					 CommanderProductionService::Get_Vote_Candidate( team ),
					 CommanderProductionService::Get_Vote_Initiator( team ),
					 CommanderProductionService::Get_Vote_Time_Remaining( team ),
					 CommanderProductionService::Get_Vote_Yes_Count( team ),
					 CommanderProductionService::Get_Vote_No_Count( team ) );
			return;
		}

		if ( ::_stricmp( verb,"elect" ) == 0 ) {
			int target = ( parsed >= 2 ) ? candidate : cNetwork::Get_My_Id();
			cCommanderVoteCallEvent * event = new cCommanderVoteCallEvent;
			event->Init( false,target );
			Print( "commander_vote: asked to open an ELECT vote for client %d; the server decides.\n",target );
			return;
		}

		if ( ::_stricmp( verb,"remove" ) == 0 ) {
			cCommanderVoteCallEvent * event = new cCommanderVoteCallEvent;
			event->Init( true,0 );
			Print( "commander_vote: asked to open a REMOVE vote; the server decides.\n" );
			return;
		}

		if ( ( ::_stricmp( verb,"yes" ) == 0 ) || ( ::_stricmp( verb,"no" ) == 0 ) ) {
			bool yes_vote = ( ::_stricmp( verb,"yes" ) == 0 );
			cCommanderVoteCastEvent * event = new cCommanderVoteCastEvent;
			event->Init( yes_vote );
			Print( "commander_vote: cast '%s'; the server decides whether it counted.\n",verb );
			return;
		}

		Print( "commander_vote: unrecognised '%s' -- usage 'commander_vote <elect [client_id]|remove|yes|no|status>'.\n",verb );
	}
};


class CommanderTransferConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_transfer"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_TRANSFER <client_id> - roadmap Section 25.1; the sitting Commander hands "
				 "the job directly to a named teammate, no vote.  Refused if you do not hold it.";
	}
	virtual	void Activate( const char * input ) override
	{
		int target = 0;
		if ( input == nullptr || ::sscanf( input,"%d",&target ) != 1 ) {
			Print( "commander_transfer: usage 'commander_transfer <client_id>'.\n" );
			return;
		}

		cCommanderTransferEvent * event = new cCommanderTransferEvent;
		event->Init( target );
		Print( "commander_transfer: asked to hand command to client %d; the server decides.\n",target );
	}
};


/*
**	Roadmap Section 24.1-24.4 -- Phase 24 autonomous AI Commander diagnostics and manual
**	activation, the server-side equivalent of `commander_mode` for a human.  Real calls into
**	`CommanderAIService`, not a display-only mock -- activating from here seats the same
**	`AI_COMMANDER_ID` `CommanderAIService::Update` would, and is subject to the identical
**	"never displaces a real Commander" refusal.
*/
class CommanderAIConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "commander_ai"; }
	virtual	const char * Get_Help( void ) override
	{
		return "COMMANDER_AI <activate <nod|gdi>|deactivate <nod|gdi>|difficulty <nod|gdi> "
				 "<easy|normal|hard>|status> - roadmap Section 24.1-24.9; server-side AI "
				 "Commander control and diagnostics.";
	}
	virtual	void Activate( const char * input ) override
	{
		char verb [32] = { 0 };
		char team_name [16] = { 0 };
		char third [16] = { 0 };
		int parsed = ( input != nullptr ) ? ::sscanf( input,"%31s %15s %15s",verb,team_name,third ) : 0;
		if ( parsed < 1 ) {
			Print( "commander_ai: usage 'commander_ai <activate <nod|gdi>|deactivate <nod|gdi>|"
					 "difficulty <nod|gdi> <easy|normal|hard>|status>'.\n" );
			return;
		}

		if ( ::_stricmp( verb,"status" ) == 0 ) {
			static const int TEAMS [2] = { PLAYERTYPE_NOD,PLAYERTYPE_GDI };
			for ( int i = 0; i < 2; i++ ) {
				int team = TEAMS [i];
				const char * team_label = ( team == PLAYERTYPE_GDI ) ? "GDI" : "Nod";
				if ( !CommanderAIService::Is_Active( team ) ) {
					Print( "  %s: no AI Commander seated.\n",team_label );
					continue;
				}
				const CommanderAIWorldSnapshot & snapshot = CommanderAIService::Get_Last_Snapshot( team );
				const char * difficulty_name = "normal";
				switch ( CommanderAIService::Get_Difficulty( team ) ) {
					case AI_DIFFICULTY_EASY:	difficulty_name = "easy";	break;
					case AI_DIFFICULTY_HARD:	difficulty_name = "hard";	break;
					default:							difficulty_name = "normal";	break;
				}
				Print( "  %s: AI seated (%s), treasury %.0f, last decision '%s', %d/%d builds attempted/succeeded.\n",
						 team_label,difficulty_name,CommanderAIService::Get_Treasury( team ),
						 CommanderAIService::Get_Economy_Action_Name(
							 (CommanderAIEconomyActionType)CommanderAIService::Get_Last_Decision( team ) ),
						 CommanderAIService::Get_Succeeded_Build_Count( team ),
						 CommanderAIService::Get_Attempted_Build_Count( team ) );
				Print( "      power %.0f/%.0f, %d plant(s), %d silo(s), %d damaged, %d unit(s) (%d inf/%d veh), "
						 "%d known enemy building(s) (%d inf/%d veh).\n",
						 snapshot.PowerAllocatedDemand,snapshot.PowerGeneration,
						 snapshot.PowerPlantCount,snapshot.SiloCount,snapshot.DamagedBuildingCount,
						 snapshot.FriendlyUnitCount,snapshot.FriendlyInfantryCount,snapshot.FriendlyVehicleCount,
						 snapshot.KnownEnemyBuildingCount,snapshot.KnownEnemyInfantryCount,snapshot.KnownEnemyVehicleCount );
				Print( "      %d expansion candidate(s) scored; production wants '%s'; "
						 "%d point(s) defended by %d unit(s).\n",
						 CommanderAIService::Get_Expansion_Candidate_Count( team ),
						 CommanderAIService::Get_Production_Mix_Name(
							 (CommanderAIProductionMixType)CommanderAIService::Get_Production_Mix_Decision( team ) ),
						 CommanderAIService::Get_Defense_Point_Count( team ),
						 CommanderAIService::Get_Assigned_Defender_Count( team ) );
				Vector3 attack_target = CommanderAIService::Get_Attack_Target_Position( team );
				Print( "      attack stage '%s', force size %d, target (%.0f, %.0f, %.0f).\n",
						 CommanderAIService::Get_Attack_Stage_Name(
							 (CommanderAIAttackStageType)CommanderAIService::Get_Attack_Stage( team ) ),
						 CommanderAIService::Get_Attack_Force_Size( team ),
						 attack_target.X,attack_target.Y,attack_target.Z );
			}
			return;
		}

		if ( parsed < 2 ) {
			Print( "commander_ai: '%s' needs a team -- 'commander_ai %s <nod|gdi>'.\n",verb,verb );
			return;
		}
		int team = 0;
		if ( ::_stricmp( team_name,"gdi" ) == 0 ) team = PLAYERTYPE_GDI;
		else if ( ::_stricmp( team_name,"nod" ) == 0 ) team = PLAYERTYPE_NOD;
		else {
			Print( "commander_ai: unrecognised team '%s' -- use 'nod' or 'gdi'.\n",team_name );
			return;
		}

		if ( ::_stricmp( verb,"difficulty" ) == 0 ) {
			CommanderAIDifficultyType difficulty;
			if ( ::_stricmp( third,"easy" ) == 0 ) difficulty = AI_DIFFICULTY_EASY;
			else if ( ::_stricmp( third,"hard" ) == 0 ) difficulty = AI_DIFFICULTY_HARD;
			else if ( ::_stricmp( third,"normal" ) == 0 ) difficulty = AI_DIFFICULTY_NORMAL;
			else {
				Print( "commander_ai: unrecognised difficulty '%s' -- use 'easy', 'normal' or 'hard'.\n",third );
				return;
			}
			CommanderAIService::Set_Difficulty( team,difficulty );
			Print( "commander_ai: %s difficulty set to '%s'.\n",team_name,third );
			return;
		}

		if ( ::_stricmp( verb,"activate" ) == 0 ) {
			if ( CommanderAIService::Activate( team ) ) {
				Print( "commander_ai: activated for %s.\n",team_name );
			} else {
				Print( "commander_ai: refused -- %s already has a Commander.\n",team_name );
			}
			return;
		}

		if ( ::_stricmp( verb,"deactivate" ) == 0 ) {
			CommanderAIService::Deactivate( team );
			Print( "commander_ai: deactivated for %s.\n",team_name );
			return;
		}

		Print( "commander_ai: unrecognised '%s' -- usage 'commander_ai <activate <nod|gdi>|deactivate <nod|gdi>|status>'.\n",verb );
	}
};


class LightStatusConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "light_status"; }
	virtual	const char * Get_Help( void ) override	{ return "LIGHT_STATUS - how many dynamic lights exist and how many had to be looked at."; }
	virtual	void Activate( const char * /* input */ ) override {

		int lights = WorldLightManager::Get_Dynamic_Light_Count();
		int queries = WorldLightManager::Get_Query_Count();
		int examined = WorldLightManager::Get_Lights_Examined();

		Print( "light_status: %d dynamic light(s) in the world, %u allowed, radius up to %.1f.\n",
				 lights,
				 WorldLightManager::Get_Max_Dynamic_Lights(),
				 WorldLightManager::Get_Max_Light_Radius() );

		//	This is the acceptance stated in two numbers: the lights per query has to stay flat as
		//	the first number grows, otherwise lighting is costing what exists rather than what is
		//	near.
		if ( queries > 0 ) {
			Print( "light_status: %d quer(ies) examined %d light(s) last frame -- %.2f per query.\n",
					 queries, examined, (float)examined / (float)queries );
		} else {
			Print( "light_status: no light queries ran last frame.\n" );
		}

		Print( "light_status: %d object(s) lit, %d light(s) applied, %d point(s) rejected outright.\n",
				 WorldLightManager::Get_Lit_Object_Count(),
				 WorldLightManager::Get_Lights_Applied(),
				 WorldLightManager::Get_Trivial_Reject_Count() );

		AABoxClass bounds;
		WorldLightManager::Get_Light_Bounds( &bounds );
		Print( "light_status: the lights reach %.1f x %.1f x %.1f around %.1f,%.1f,%.1f.\n",
				 bounds.Extent.X * 2.0f, bounds.Extent.Y * 2.0f, bounds.Extent.Z * 2.0f,
				 bounds.Center.X, bounds.Center.Y, bounds.Center.Z );

		int overflows = WorldLightManager::Get_Overflow_Count();
		if ( overflows > 0 ) {
			Print( "light_status: %d object(s) went unlit -- more than %u wanted a lighting environment.\n",
					 overflows, WorldLightManager::Get_Max_Lit_Objects() );
		}

		int refusals = WorldLightManager::Get_Refusal_Count();
		if ( refusals > 0 ) {
			Print( "light_status: %d light(s) were refused, %d had their radius clamped.\n",
					 refusals, WorldLightManager::Get_Clamped_Radius_Count() );
		}
	}
};


class LightTestConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "light_test"; }
	virtual	const char * Get_Help( void ) override	{ return "LIGHT_TEST <count> [radius] [seconds] - drop coloured lights around the camera."; }
	virtual	void Activate( const char * input ) override {

		int count = 8;
		float radius = 15.0f;
		float seconds = 0.0f;
		if ( input != nullptr ) {
			::sscanf( input, "%d %f %f", &count, &radius, &seconds );
		}

		if ( count < 1 ) { count = 1; }
		if ( count > (int)WorldLightManager::Get_Max_Dynamic_Lights() ) {
			count = (int)WorldLightManager::Get_Max_Dynamic_Lights();
		}
		if ( radius <= 0.0f ) { radius = 15.0f; }

		Vector3 center( 0.0f, 0.0f, 0.0f );
		if ( COMBAT_SCENE != nullptr ) {
			center = COMBAT_SCENE->Get_Last_Camera_Position();
		}

		//	A ring around the camera, one colour per light so that which light is lighting what is
		//	something the eye can answer.
		static const Vector3 colors[6] = {
			Vector3( 1.0f, 0.2f, 0.2f ),	Vector3( 0.2f, 1.0f, 0.2f ),
			Vector3( 0.2f, 0.4f, 1.0f ),	Vector3( 1.0f, 1.0f, 0.3f ),
			Vector3( 1.0f, 0.3f, 1.0f ),	Vector3( 0.3f, 1.0f, 1.0f ),
		};

		int made = 0;
		for ( int i = 0; i < count; i ++ ) {

			float angle = 2.0f * WWMATH_PI * (float)i / (float)count;
			Vector3 pos = center + Vector3( WWMath::Cos( angle ) * radius * 0.75f,
													  WWMath::Sin( angle ) * radius * 0.75f,
													  1.0f );

			if ( WorldLightManager::Create_Point_Light( pos, colors[ i % 6 ], 0.0f, radius,
																	  1.0f, seconds ) != nullptr ) {
				made ++;
			}
		}

		Print( "light_test: %d of %d light(s) placed around %.1f,%.1f,%.1f.\n",
				 made, count, center.X, center.Y, center.Z );
		if ( seconds > 0.0f ) {
			Print( "light_test: they fade out over %.1f second(s).\n", seconds );
		}
	}
};


class LightClearConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "light_clear"; }
	virtual	const char * Get_Help( void ) override	{ return "LIGHT_CLEAR - remove every dynamic light."; }
	virtual	void Activate( const char * /* input */ ) override {

		int count = WorldLightManager::Get_Dynamic_Light_Count();
		WorldLightManager::Remove_All_Dynamic_Lights();
		Print( "light_clear: %d dynamic light(s) removed.\n", count );
	}
};


class VisLockConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_lock_sample_point"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_LOCK_SAMPLE_POINT - locks/unlocks the current pvs."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Lock_Vis_Sample_Point(!COMBAT_SCENE->Is_Vis_Sample_Point_Locked());
		if (COMBAT_SCENE->Is_Vis_Sample_Point_Locked()) {
			Print("Vis sample point locked.\n");
		} else {
			Print("Vis sample point unlocked.\n");
		}
	}
};

class VisSectorDisplayConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_sector_display"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_SECTOR_DISPLAY - toggles diplay of the current vis sector."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Enable_Vis_Sector_Display(!COMBAT_SCENE->Is_Vis_Sector_Display_Enabled());
		if (COMBAT_SCENE->Is_Vis_Sector_Display_Enabled()) {
			Print("vis_sector_display enabled\n");
		} else {
			Print("vis_sector_display disabled\n");
		}
	}
};

class VisSectorHistoryConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "vis_sector_history"; }
	virtual	const char * Get_Help( void ) override	{ return "VIS_SECTOR_HISTORY - toggles diplay of the previous 3 vis sectors."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Enable_Vis_Sector_History_Display(!COMBAT_SCENE->Is_Vis_Sector_History_Display_Enabled());
		if (COMBAT_SCENE->Is_Vis_Sector_History_Display_Enabled()) {
			Print("vis_sector_history enabled\n");
		} else {
			Print("vis_sector_history disabled\n");
		}
	}
};


class SuicideConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "suicide"; }
	virtual	const char * Get_Help( void ) override	{ return "SUICIDE - kill your commando."; }
	virtual	void Activate( const char * /* input */ ) override {
		if (cNetwork::I_Am_Client()) {
         Print("Committing suicide.\n");

			cSuicideEvent * p_suicide = new cSuicideEvent;
			p_suicide->Init();

		} else {
         Print("Only clients can commit suicide.\n");
		}
	}
};


class ShadowAttenuationConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_attenuation"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_ATTENUATION <near> <far> - set the attenuation ranges for shadows."; }
	virtual	void Activate( const char * input ) override {
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		float znear,zfar;
		if (sscanf(input, "%f %f",&znear,&zfar) == 2) {
			scene->Set_Shadow_Attenuation(znear,zfar);
			scene->Get_Shadow_Attenuation(&znear,&zfar);
			Print("shadow attenuation set to %f %f\n",znear,zfar);
		}
	}
};

class ShadowBlobTextureConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_blob_texture"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_BLOB_TEXTURE <texture filename> - sets the texture to use for blob shadows."; }
	virtual	void Activate( const char * input ) override {
		if (strlen(input) > 0) {
			if (PhysResourceMgrClass::Set_Shadow_Blob_Texture(input)) {
				Print("shadow blob texture set to: %s\n",input);
			}
		}
	}
};

class ShadowElevationConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_elevation"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_ELEVATION <elevation degrees> - sets the angle of the shadow projection."; }
	virtual	void Activate( const char * input ) override
	{
		/*
		** NOTE: THIS IS JUST TEST CODE TO SEE HOW VARIOUS SHADOW ALGORITHMS
		** WORK WITH VARIOUS SUN-LIGHT ANGLES.  THE SUN IS NOT TO BE MOVED IN-GAME!!!
		*/
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();

		float elevation,rotation;
		scene->Get_Sun_Light_Orientation(&rotation,&elevation);
		if (sscanf(input, "%f",&elevation) == 1) {
			scene->Set_Sun_Light_Orientation(rotation,DEG_TO_RADF(elevation));
		}
	}
};


class ShadowIntensityConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_intensity"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_INTENSITY <intensity> - sets the darkness of dynamic shadows (0 - 1)."; }
	virtual	void Activate( const char * input ) override
	{
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		scene->Set_Shadow_Normal_Intensity(strtof(input, nullptr));
		Print("shadow intensity set to: %f\n",scene->Get_Shadow_Normal_Intensity());
	}
};

class ShadowPerPolyCullingConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_per_poly_culling"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_PER_POLY_CULLING - toggles per poly shadow culling."; }
	virtual	void Activate( const char * /* input */ ) override
	{
		MaterialPassClass::Enable_Per_Polygon_Culling(!MaterialPassClass::Is_Per_Polygon_Culling_Enabled());
		if (MaterialPassClass::Is_Per_Polygon_Culling_Enabled()) {
			Print("per-polygon shadow culling enabled!\n");
		} else {
			Print("per-polygon shadow culling disabled\n");
		}
	}
};

class ShadowResolutionConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_resolution"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_RESOLUTION <res> - sets the resolution of dynamic shadows (use a power of 2!)"; }
	virtual	void Activate( const char * input ) override
	{
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		scene->Set_Shadow_Resolution(int(strtof(input, nullptr)));
		Print("shadow resolution set to: %f\n",scene->Get_Shadow_Resolution());
	}
};

class ShadowCountConsoleFunctionClass : public ConsoleFunctionClass
{
	virtual	const char * Get_Name( void ) override	{ return "shadow_count"; }
	virtual	const char * Get_Help( void ) override	{ return "SHADOW_COUNT <count> - sets the maximum simultaneous shadows"; }
	virtual	void Activate( const char * input ) override
	{
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		scene->Set_Max_Simultaneous_Shadows(int(strtof(input, nullptr)));
		Print("simultaneous shadow count set to: %f\n",scene->Get_Max_Simultaneous_Shadows());
	}
};


class PhysicsDebugConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "physics_debug"; }
	virtual	const char * Get_Help( void ) override	{ return "PHYSICS_DEBUG - toggles physics debugging display."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Enable_Debug_Display(!COMBAT_SCENE->Is_Debug_Display_Enabled());
		if (COMBAT_SCENE->Is_Debug_Display_Enabled()) {
			Print("physics debug display enabled!\n");
		} else {
			Print("physics debug display disabled :-(\n");
		}
	}
};

class Phys3NetConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "phys3_net"; }
	virtual	const char * Get_Help( void ) override	{ return "PHYS3_NET <allowable_error> <pop_error> <corr_time> - sets the phys3 networking constants."; }
	virtual	void Activate( const char * input ) override {
		float corr_time,allow_error,pop_error;
		int params = sscanf(input, "%f %f %f",&allow_error,&pop_error,&corr_time);
		if (params == 3) {
			Phys3Class::Set_Allowable_Error(allow_error);
			Phys3Class::Set_Pop_Error(pop_error);
			Phys3Class::Set_Correction_Time(corr_time);
		}
		Print("Phys3 Network Params: allowable_error = %f\r\n",Phys3Class::Get_Allowable_Error());
		Print("Phys3 Network Params: pop_error = %f\r\n",Phys3Class::Get_Pop_Error());
		Print("Phys3 Network Params: correction_time = %f\r\n",Phys3Class::Get_Correction_Time());
	}
};

class RBodyNetConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "rbody_net"; }
	virtual	const char * Get_Help( void ) override	{ return "rbody_net <corr_time> - sets the rigid body correction time."; }
	virtual	void Activate( const char * input ) override {
		float corr_time;
		int params = sscanf(input, "%f",&corr_time );
		if (params == 3) {
			RigidBodyClass::Set_Correction_Time(corr_time);
		}
		Print("Rigid Body Network Params: correction_time = %f\r\n",RigidBodyClass::Get_Correction_Time());
	}
};

class ProjectorDebugConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "projector_debug"; }
	virtual	const char * Get_Help( void ) override	{ return "PROJECTOR_DEBUG - toggles projector debugging display."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Enable_Projector_Debug_Display(!COMBAT_SCENE->Is_Projector_Debug_Display_Enabled());
		if (COMBAT_SCENE->Is_Projector_Debug_Display_Enabled()) {
			Print("projector debug display enabled!\n");
		} else {
			Print("projector debug display disabled :-(\n");
		}
	}
};

class DirtyCullDebugConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "dirty_cull_debug"; }
	virtual	const char * Get_Help( void ) override	{ return "DIRTY_CULL_DEBUG - toggles debugging of the dirty-cull-list."; }
	virtual	void Activate( const char * /* input */ ) override {

		COMBAT_SCENE->Enable_Dirty_Cull_Debug_Display(!COMBAT_SCENE->Is_Dirty_Cull_Debug_Display_Enabled());
		if (COMBAT_SCENE->Is_Dirty_Cull_Debug_Display_Enabled()) {
			Print("dirty cull debug display enabled!\n");
		} else {
			Print("dirty cull debug display disabled.\n");
		}
	}
};

class NextRenderConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "next_render"; }
	virtual	const char * Get_Help( void ) override	{ return "NEXT_RENDER - switches to the next render device."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::Set_Next_Render_Device();
	}
};

class OneShotKillsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "one_shot_kills"; }
	virtual	const char * Get_Alias( void ) override	{ return "osk"; }
	virtual	const char * Get_Help( void ) override	{ return "ONE_SHOT_KILLS - kill or destroy with one shot. Server only."; }
	virtual	void Activate( const char * /* input */ ) override {

		if (cNetwork::I_Am_Server()) {
         bool osk = false;

#ifdef WWDEBUG
			osk = DefenseObjectClass::Toggle_One_Shot_Kills();
#endif // WWDEBUG

         Print( "One Shot Kills: %s\n", osk ? "ON" : "OFF");
		} else {
         Print(Get_Help());
		}
	}
};

class OpenConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "open"; }
	virtual	const char * Get_Help( void ) override	{ return "OPEN [regedit|logfile|ini ininame] - open specified object."; }
	virtual	void Activate( const char * input ) override {

		if (!stricmp(input, "regedit")) {
			cNetwork::Shell_Command("regedit");
		} else if (!stricmp(input, "logfile")) {
			cNetwork::Shell_Command(DebugManager::Logfile_Name());
		} else if (strstr(input, "ini")) {
			char filename[200];
			sscanf(input, "ini %s", filename);
			char open_command[200];
			sprintf(open_command, "data/%s.ini", filename);
			cNetwork::Shell_Command(open_command);
		} else {
		   Print(Get_Help());
		}
	}
};

class InvalidateMeshCacheConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "invalidate_mesh_cache"; }
	virtual	const char * Get_Help( void ) override	{ return "INVALIDATE_MESH_CACHE - invalidates all cached mesh data."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::_Invalidate_Mesh_Cache();
      Print( "Mesh cache invalidated." );
	}
};

class InvalidateTexturesConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "invalidate_textures"; }
	virtual	const char * Get_Help( void ) override	{ return "INVALIDATE_TEXTURES - invalidates all textures."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::_Invalidate_Textures();
      Print( "Textures invalidated." );
	}
};

class TexturingEnableConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "texturing_enable"; }
	virtual	const char * Get_Help( void ) override	{ return "TEXTURING_ENABLE toggle texturing."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::Enable_Texturing(!WW3D::Is_Texturing_Enabled());
		Print( "Textures switched." );
	}
};

class ExposePrelitConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "expose_prelit"; }
	virtual	const char * Get_Help( void ) override	{ return "EXPOSE_PRELIT - expose precalculated lighting on meshes."; }
	virtual	void Activate( const char * /* input */ ) override {

		static const char *_comment [2] = {
			"Unexposed prelit meshes\n",
			"Exposed	prelit meshes\n"
		};

		WW3D::Expose_Prelit (!WW3D::Expose_Prelit());
		Print (_comment [WW3D::Expose_Prelit() ? 1 : 0]);
	}
};

class PlayerPositionConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "player_position"; }
	//virtual	const char * Get_Alias( void ) override	{ return "pp"; }
	virtual	const char * Get_Help( void ) override	{ return "PLAYER_POSITION - toggles the players world position display."; }
	virtual	void Activate( const char * /* input */ ) override {
		Get_Console()->Toggle_Player_Position();
	}
};

class ToggleAssetPreloadingConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_preloading"; }
	virtual	const char * Get_Help( void ) override	{ return "TOGGLE_PRELOADING - toggles asset preloading."; }
	virtual	void Activate( const char * /* input */ ) override {
		bool is_preloading = cDevOptions::PreloadAssets.Toggle();
      Print(is_preloading ? "Assets WILL preload.\n" : "Assets will NOT preload.\n" );
	}
};

class ToggleRenderingConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_rendering"; }
	virtual	const char * Get_Help( void ) override	{ return "TOGGLE_RENDERING <static,dynamic,human,vehicles> - toggles rendering of various types."; }
	virtual	void Activate( const char * input ) override {

		/*
		** Note: I'm only comparing the first two characters so the user doesn't have to type the whole thing
		*/
		if (_strnicmp(input,"st",2) == 0) {

			StaticPhysClass::Disable_All_Rendering(!StaticPhysClass::Is_All_Rendering_Disabled());
			Print(StaticPhysClass::Is_All_Rendering_Disabled() ? "Static object rendering DISABLED" : "Static object rendering ENABLED");

		} else if (_strnicmp(input,"dy",2) == 0) {

			DynamicPhysClass::Disable_All_Rendering(!DynamicPhysClass::Is_All_Rendering_Disabled());
			Print(DynamicPhysClass::Is_All_Rendering_Disabled() ? "Dynamic object rendering DISABLED" : "Dynamic object rendering ENABLED");

		} else if (_strnicmp(input,"hu",2) == 0) {

			HumanPhysClass::Disable_All_Rendering(!HumanPhysClass::Is_All_Rendering_Disabled());
			Print(HumanPhysClass::Is_All_Rendering_Disabled() ? "Human object rendering DISABLED" : "Human object rendering ENABLED");

		} else if (_strnicmp(input,"ve",2) == 0) {

			VehiclePhysClass::Disable_All_Rendering(!VehiclePhysClass::Is_All_Rendering_Disabled());
			Print(VehiclePhysClass::Is_All_Rendering_Disabled() ? "Vehicle object rendering DISABLED" : "Vehicle object rendering ENABLED");

		}
	}
};


class ToggleSimulationConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_simulation"; }
	virtual	const char * Get_Help( void ) override	{ return "TOGGLE_SIMULATION <static,dynamic,human,vehicles> - toggles rendering of various types."; }
	virtual	void Activate( const char * input ) override {

		/*
		** Note: I'm only comparing the first two characters so the user doesn't have to type the whole thing
		*/
		if (_strnicmp(input,"st",2) == 0) {

			StaticPhysClass::Disable_All_Simulation(!StaticPhysClass::Is_All_Simulation_Disabled());
			Print(StaticPhysClass::Is_All_Simulation_Disabled() ? "Static object Simulation DISABLED" : "Static object Simulation ENABLED");

		} else if (_strnicmp(input,"dy",2) == 0) {

			DynamicPhysClass::Disable_All_Simulation(!DynamicPhysClass::Is_All_Simulation_Disabled());
			Print(DynamicPhysClass::Is_All_Simulation_Disabled() ? "Dynamic object Simulation DISABLED" : "Dynamic object Simulation ENABLED");

		} else if (_strnicmp(input,"hu",2) == 0) {

			HumanPhysClass::Disable_All_Simulation(!HumanPhysClass::Is_All_Simulation_Disabled());
			Print(HumanPhysClass::Is_All_Simulation_Disabled() ? "Human object Simulation DISABLED" : "Human object Simulation ENABLED");

		} else if (_strnicmp(input,"ve",2) == 0) {

			VehiclePhysClass::Disable_All_Simulation(!VehiclePhysClass::Is_All_Simulation_Disabled());
			Print(VehiclePhysClass::Is_All_Simulation_Disabled() ? "Vehicle object Simulation DISABLED" : "Vehicle object Simulation ENABLED");

		}
	}
};


class TomConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "tom"; }
	virtual	const char * Get_Help( void ) override	{ return "TOM - Annoyance-reduction registry tweaks customized by Tom."; }
	virtual	void Activate( const char * /* input */ ) override {

		INIClass ini(W3D_CONF_FILE);
		//
		// Enable a good level of diagnostics - all devices and types except
		// screen and netprolific
		//
      ConsoleFunctionManager::Parse_Input("debug_device on");
      ConsoleFunctionManager::Parse_Input("debug_device screen");
      ConsoleFunctionManager::Parse_Input("debug_type on");
      ConsoleFunctionManager::Parse_Input("debug_type netprolific");

		//
		// Set max scroll lines
		//
		if (Get_Text_Display()) {
			WWASSERT(Get_Text_Display() != nullptr);
			Get_Text_Display()->Set_Max_Scroll_Lines(10);
		}

		//
		// Debug flags
		//
		//cDevOptions::ShowDiagnostics.Set(true);
		cDevOptions::PreloadAssets.Set(false);
		cDevOptions::ShowFps.Set(true);
		cDevOptions::ShowGodStatus.Set(true);
		cDevOptions::CompareExeVersionOnNetwork.Set(false);
		cDevOptions::CrtDbgEnabled.Set(false);
		cDevOptions::LogDataSafe.Set(false);
		cDevOptions::FilterLevelFiles.Set(false);
		cDevOptions::ShowThumbnailPreInitDialog.Set(false);

		//
		// Release flags
		//
		cUserOptions::ShowNamesOnSoldier.Set(true);
		cUserOptions::CameraLockedToTurret.Set(true);
		cUserOptions::SkipQuitConfirmDialog.Set(true);
		cUserOptions::SkipIngameQuitConfirmDialog.Set(true);
		cUserOptions::PermitDiagLogging.Set(false);

		//
		// Set 800 x 600, windowed mode
		//
		ini.Put_Int(W3D_SECTION_RENDER, VALUE_INI_RENDER_DEVICE_WIDTH, 800);
		ini.Put_Int(W3D_SECTION_RENDER, VALUE_INI_RENDER_DEVICE_HEIGHT, 600);
		ini.Put_Bool(W3D_SECTION_RENDER, VALUE_INI_RENDER_DEVICE_WINDOWED, true);

		/*
		//
		// Stop Jani from spying on us (avoid delay on exit)
		//
		{
		RegistryClass registry(APPLICATION_SUB_KEY_NAME_DEBUG);
		WWASSERT(registry.Is_Valid());
		registry.Set_Int("SystemInfoLogDisable", true);
		registry.Set_Int("DisableLogCopying", true);
		}
		*/

		//
		// Default to third person
		//
		{
		RegistryClass registry(APPLICATION_SUB_KEY_NAME_OPTIONS);
		WWASSERT(registry.Is_Valid());
		registry.Set_Int("DefaultToFirstPerson", false);
		}

		//
		// Skip intro movies
		//
		{
		RegistryClass registry(APPLICATION_SUB_KEY_NAME_OPTIONS);
		WWASSERT(registry.Is_Valid());
		registry.Set_Int("SkipAllIntroMovies", true);
		}

		//
		// Do not require CD for movies
		//
		{
		RegistryClass registry(APPLICATION_SUB_KEY_NAME_DEBUG);
		WWASSERT(registry.Is_Valid());
		registry.Set_Int("DisableCDCheck", true);
		}

		//
		// Set gamma etc so that they don't screw up debugging
		//
		ini.Put_Int(W3D_SECTION_SYSTEM, "Gamma", 1);
		ini.Put_Int(W3D_SECTION_SYSTEM, "Brightness", 0);
		ini.Put_Int(W3D_SECTION_SYSTEM, "Contrast", 1);

		OpenW3D::Save_Config(ini);
	}
};

class RenderBoxesConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "render_boxes"; }
	virtual	const char * Get_Help( void ) override	{ return "RENDER_BOXES <mask> - enables rendering of boxes.  0=none 1=all 2=physical 4=projectile."; }
	virtual	void Activate( const char * input ) override {
		int mask = atoi(input);
		WW3D::Set_Collision_Box_Display_Mask(mask);
	}
};

class AmbientLightConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "ambient_light"; }
	virtual	const char * Get_Help( void ) override	{ return "AMBIENT_LIGHT <R> <G> <B> - sets the ambeint light color to <r> <g> <b>."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_SCENE ) {
			float r, g, b;
			if (sscanf(input, "%f %f %f", &r, &g, &b) == 3) {
		      COMBAT_SCENE->Set_Ambient_Light(Vector3(r, g, b));
		      Print( "Ambient Light changed\n" );
			}
		} else {
		   Print( "Error : no COMBAT_SCENE\n" );
		}
	}
};

class VblankSyncConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "vblank_sync"; }
	virtual	const char * Get_Help( void ) override	{ return "VBLANK_SYNC - toggles vblank syncing"; }
	virtual	void Activate( const char * /* input */ ) override {

		if (WW3D::Get_Ext_Swap_Interval() == 0) {
			WW3D::Set_Ext_Swap_Interval(1);
			Print("Vblank syncing ENABLED!\n");
		} else {
			WW3D::Set_Ext_Swap_Interval(0);
			Print("Vblank syncing DISABLED!\n");
		}
	}
};

class FrameTexturesConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "frame_textures"; }
	virtual	const char * Get_Help( void ) override	{ return "FRAME_TEXTURES - measures the amoutn of textures needed for the next frame."; }
	virtual	void Activate( const char * /* input */ ) override {
		DebugManager::Measure_Frame_Textures();
	}
};

class DebugObjectivesConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "objectives_debug"; }
	virtual	const char * Get_Help( void ) override	{ return "OBJECTIVES_DEBUG - toggles Mission Objectives debug mode."; }
	virtual	void Activate( const char * /* input */ ) override {
		bool enable_mode = !ObjectiveManager::Is_Objective_Debug_Mode_Enabled();
		ObjectiveManager::Enable_Objective_Debug_Mode( enable_mode );
		if (enable_mode) {
			Print( "Activating Mission Objectives Debug Mode.\n" );
		} else {
			Print( "Deactivating Mission Objectives Debug Mode.\n" );
		}
	}
};


class FreeWeaponsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "free_weapons"; }
	virtual	const char * Get_Help( void ) override	{ return "FREE_WEAPONS - toggle free_weapons flag (server only)."; }
	virtual	void Activate( const char * /* input */ ) override {
      if (!cNetwork::I_Am_Server()) {
			Print("Only the server can toggle free-weapons.\n");
      } else {
			WWASSERT(PTheGameData != nullptr);
			bool on = The_Game()->IsFreeWeapons.Toggle();
         Print("Free weapons is %s.\n", on ? "ON" : "OFF");

			if (on) {
				//
				// Give everyone free weapons right away
				//
				SLNode<SmartGameObj> * objnode;
				for (objnode = GameObjManager::Get_Smart_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {
					SmartGameObj * p_smart_obj = objnode->Data();
					WWASSERT(p_smart_obj != nullptr);
					if (p_smart_obj->As_SoldierGameObj() != nullptr &&
						p_smart_obj->Get_Player_Type() != PLAYERTYPE_NEUTRAL) {
						p_smart_obj->As_SoldierGameObj()->Give_All_Weapons();
					}
				}
			}
      }
	}
};


class ToggleEmotIconsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_emoticons"; }
	virtual	const char * Get_Help( void ) override	{ return "TOGGLE_EMOTICONS - Toggle the display of emot icons during conversations."; }
	virtual	void Activate( const char * /*input*/ ) override {

		bool display = !ConversationMgrClass::Are_Emot_Icons_Displayed ();
		ConversationMgrClass::Display_Emot_Icons (display);

		if (display) {
			Print( "EmotIcons are now enabled.\n" );
		} else {
			Print( "EmotIcons are now disabled.\n" );
		}
	}
};


class SoldierGhostDebugDisplayConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_ghost_debug_display"; }
	virtual	const char * Get_Help( void ) override	{ return "toggle_ghost_debug_display - Displays a box around soldiers that are in 'ghost' collision mode."; }
	virtual	void Activate( const char * /* input */ ) override {

		bool enable = !SoldierGameObj::Is_Ghost_Collision_Debug_Display_Enabled ();
		SoldierGameObj::Enable_Ghost_Collision_Debug_Display (enable);

		if (enable) {
			Print( "Ghost collision debug display enabled.\n" );
		} else {
			Print( "Ghost collision debug display disabled.\n" );
		}

		return ;
	}
};


class HUDHelpTextTestConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "hud_help_text"; }
	//virtual	const char * Get_Help( void ) override	{ return "CnC [vehicle<def_name> | powerup<def_name> |char<class>[minigunner|rocket_soldier|grenadier|engineer|flame_thrower|mutant] <rank>[enlisted|officer|special_forces|boss] ] - Purchases a team vehicle, a player powerup or upgrades the player."; }
	virtual	const char * Get_Help( void ) override	{ return "hud_help_text - Sets the current HUD help text."; }
	virtual	void Activate( const char * input ) override {

		WideStringClass wide_string;
		wide_string.Convert_From (input);
		HUDInfo::Set_HUD_Help_Text (wide_string);
		return ;
	}
};


class CnCModeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "cnc"; }
	//virtual	const char * Get_Help( void ) override	{ return "CnC [vehicle<def_name> | powerup<def_name> |char<class>[minigunner|rocket_soldier|grenadier|engineer|flame_thrower|mutant] <rank>[enlisted|officer|special_forces|boss] ] - Purchases a team vehicle, a player powerup or upgrades the player."; }
	virtual	const char * Get_Help( void ) override	{ return "CnC - Displays CnC player console (for gods only)."; }
	virtual	void Activate( const char * /* input */ ) override {

		if (cNetwork::I_Am_God()) {
			PlayerTerminalClass::Get_Instance ()->Display_Default_Terminal_For_Player (COMBAT_STAR);
		} else {
			Print(Get_Help());
		}
	}
};


class DebugEncyRevealConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "reveal_encyclopedia"; }
	virtual	const char * Get_Help( void ) override	{ return "reveal_encyclopedia - Shows all entries in the encyclopedia."; }
	virtual	void Activate( const char * /* input */ ) override {

		EncyclopediaMgrClass::Reveal_All_Objects ();
		return ;
	}
};


class DebugShowMoveVectorConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "move_vectors"; }
	virtual	const char * Get_Help( void ) override	{ return "move_vectors - Toggles the debug move vectors."; }
	virtual	void Activate( const char * /* input */ ) override {
		bool displayed = PathClass::Are_Move_Vectors_Displayed ();
		displayed = !displayed;
		PathClass::Display_Move_Vectors (displayed);
		if ( displayed ) {
			Print( "Move vectors ON.\n" );
		} else {
			Print( "Move vectors OFF.\n" );
		}

		return ;
	}
};


class MapRevealConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "map_reveal"; }
	virtual	const char * Get_Help( void ) override	{ return "map_reveal - Removes the cloud from the map."; }
	virtual	void Activate( const char * /* input */ ) override {
		MapMgrClass::Clear_All_Cloud_Cells ();
		Print( "Map is now visible.\n" );
		return ;
	}
};


class ChangeCharacterClassConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "change_character_class"; }
	virtual	const char * Get_Help( void ) override	{ return "CHANGE_CHARACTER_CLASS <preset_name> - changes your character to the specified class."; }
	virtual	void Activate( const char * input ) override {
		bool success = false;

		DefinitionClass *definition = DefinitionMgrClass::Find_Typed_Definition( input, CLASSID_GAME_OBJECT_DEF_SOLDIER );
		if (definition != nullptr) {
			SoldierGameObjDef *soldier_def = reinterpret_cast<SoldierGameObjDef *> (definition);
			COMBAT_STAR->Re_Init( *soldier_def );
			COMBAT_STAR->Post_Re_Init();
			success = true;
		}

		if (success) {
			Print( "Successfully changed character class.\n" );
		} else {
			Print( "Unable to change character class.\n" );
		}
	}
};

class RenderModeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "render_mode"; }
   virtual	const char * Get_Alias( void ) override { return "rm"; }
	virtual	const char * Get_Help( void ) override	{ return "RENDER_MODE [0 fill | 1 lines | 2 points] - set the current render mode."; }
	virtual	void Activate( const char * input ) override {
		if (COMBAT_SCENE != nullptr) {
			if ((stricmp(input,"points") == 0) || (stricmp(input,"2") == 0)) {
				COMBAT_SCENE->Set_Polygon_Mode(SceneClass::POINT);
				Print("Polygon mode set to POINT\n");
			} else if ((stricmp(input,"lines") == 0) || (stricmp(input,"1") == 0)) {
				COMBAT_SCENE->Set_Polygon_Mode(SceneClass::LINE);
				Print("Polygon mode set to LINE\n");
			} else if ((stricmp(input,"fill") == 0) || (stricmp(input,"0") == 0)) {
				COMBAT_SCENE->Set_Polygon_Mode(SceneClass::FILL);
				Print("Polygon mode set to FILL\n");
			}
		}
	}
};

class RenderModeExtraPassConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "render_mode_extra_pass"; }
   virtual	const char * Get_Alias( void ) override { return "rm"; }
	virtual	const char * Get_Help( void ) override	{ return "RENDER_MODE_EXTRA_PASS [0 disable | 1 lines | 2 zbuffered]"; }
	virtual	void Activate( const char * input ) override {
		if (COMBAT_SCENE != nullptr) {
			if ((stricmp(input,"disable") == 0) || (stricmp(input,"0") == 0)) {
				COMBAT_SCENE->Set_Extra_Pass_Polygon_Mode(SceneClass::EXTRA_PASS_DISABLE);
				Print("Extra pass polygon mode set to DISABLE\n");
			} else if ((stricmp(input,"lines") == 0) || (stricmp(input,"1") == 0)) {
				COMBAT_SCENE->Set_Extra_Pass_Polygon_Mode(SceneClass::EXTRA_PASS_LINE);
				Print("Extra pass polygon mode set to LINE\n");
			} else if ((stricmp(input,"zbuffered") == 0) || (stricmp(input,"2") == 0)) {
				COMBAT_SCENE->Set_Extra_Pass_Polygon_Mode(SceneClass::EXTRA_PASS_CLEAR_LINE);
				Print("Extra pass polygon mode set to ZBUFFERED\n");
			}
		}
	}
};

class TextureFilterModeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "texture_filter_mode"; }
	virtual	const char * Get_Help( void ) override	{ return "TEXTURE_FILTER_MODE [none|fast|best] - set texture filter and mipmap modes."; }
	virtual	void Activate( const char * input ) override {
		if (stricmp(input,"none") == 0) {
			TextureClass::_Set_Default_Min_Filter(TextureClass::FILTER_TYPE_FAST);
			TextureClass::_Set_Default_Mag_Filter(TextureClass::FILTER_TYPE_FAST);
			TextureClass::_Set_Default_Mip_Filter(TextureClass::FILTER_TYPE_NONE);
			Print("Texture filter mode changed to: none\n");
		}
		else if (stricmp(input,"fast") == 0) {
			TextureClass::_Set_Default_Min_Filter(TextureClass::FILTER_TYPE_FAST);
			TextureClass::_Set_Default_Mag_Filter(TextureClass::FILTER_TYPE_FAST);
			TextureClass::_Set_Default_Mip_Filter(TextureClass::FILTER_TYPE_FAST);
			Print("Texture filter mode changed to: fast\n");
		}
		if (stricmp(input,"best") == 0) {
			TextureClass::_Set_Default_Min_Filter(TextureClass::FILTER_TYPE_BEST);
			TextureClass::_Set_Default_Mag_Filter(TextureClass::FILTER_TYPE_BEST);
			TextureClass::_Set_Default_Mip_Filter(TextureClass::FILTER_TYPE_BEST);
			Print("Texture filter mode changed to: best\n");
		}
		else {
			Print("Valid filter modes are: none|fast|best\n");
		}
	}
};

class EnableTriangleRenderConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "enable_triangle_render"; }
	virtual	const char * Get_Help( void ) override	{ return "ENABLE_TRIANGLE_RENDER [none|opaque|sorting|all] - enable/disable triangle rendering."; }
	virtual	void Activate( const char * input ) override {
		if (stricmp(input,"none") == 0) {
			DX8Wrapper::_Enable_Triangle_Draw(false);
			SortingRendererClass::_Enable_Triangle_Draw(false);
			Print("All polygon rendering disabled\n");
		}
		else if (stricmp(input,"opaque") == 0) {
			DX8Wrapper::_Enable_Triangle_Draw(true);
			SortingRendererClass::_Enable_Triangle_Draw(false);
			Print("Only opaque polygons enabled\n");
		}
		else if (stricmp(input,"sorting") == 0) {
			DX8Wrapper::_Enable_Triangle_Draw(false);
			SortingRendererClass::_Enable_Triangle_Draw(true);
			Print("Only sorting polygons enabled\n");
		}
		else {
			DX8Wrapper::_Enable_Triangle_Draw(true);
			SortingRendererClass::_Enable_Triangle_Draw(true);
			Print("All polygons enabled\n");
		}
	}
};

class RunFastConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "run_fast"; }
	virtual	const char * Get_Help( void ) override	{ return "RUN_FAST - set options for fast framerate."; }
	virtual	void Activate( const char * /* input */ ) override {

		DebugManager::Option_Enable(DebugManager::DEBUG_DISABLE_LIGHTS, true);
		WWASSERT(DebugManager::Option_Is_Enabled(DebugManager::DEBUG_DISABLE_LIGHTS));
		Print("Lights disabled.\n");

      ConsoleFunctionManager::Parse_Input("Dynamic_LOD_Budget 100");

      ConsoleFunctionManager::Parse_Input("Static_LOD_Budget 100");

      ConsoleFunctionManager::Parse_Input("Shadow_Mode 0");

      ConsoleFunctionManager::Parse_Input("Prelit_Mode 0");

      //ConsoleFunctionManager::Parse_Input("Mesh_Draw_Mode 0");

		if (COMBAT_SCENE != nullptr) {
			COMBAT_SCENE->Enable_Dynamic_Projectors(false);
			Print("Dynamic Projectors disabled.\n");
			COMBAT_SCENE->Enable_Static_Projectors(false);
			Print("Static Projectors disabled.\n");
		}

		WW3D::Set_Texture_Reduction(1);
		Print("Texture Resolution set to 1.\n");

		ConsoleGameModeClass::Save_Registry_Keys();

		if (Get_Text_Display()) {
			WWASSERT(Get_Text_Display() != nullptr);
			Get_Text_Display()->Set_Max_Scroll_Lines(3);
			Print("Set_Max_Scroll_Lines(3)\n");
		}
	}
};

class RenderBackfacesConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "render_backfaces"; }
	virtual	const char * Get_Help( void ) override	{ return "RENDER_BACKFACES - toggles debug rendering of backfaced polys."; }
	virtual	void Activate( const char * /* input */ ) override {
		/*
		** This turns on WW3D backface rendering and also causes the Physics-Scene to
		** stop rendering non-occluders so that we can get a better idea of what VIS
		** is seeing when it renders the occluders and checks for backfaces
		*/
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		if (scene) {
			scene->Enable_Backface_Occluder_Debug(!scene->Is_Backface_Occluder_Debug_Enabled());
			if (scene->Is_Backface_Occluder_Debug_Enabled()) {
				Print("Backface Debug Display ENABLED\n");
			} else {
				Print("Backface Debug Display DISABLED\n");
			}
		}
	}
};

class AmmoConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "ammo"; }
	virtual	const char * Get_Help( void ) override	{ return "AMMO <count> - sets the players weapon's ammo count."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_STAR != nullptr ) {
			WeaponClass * weapon = COMBAT_STAR->Get_Weapon();
			if ( weapon ) {
				int amount = atoi( input );
				weapon->Set_Total_Rounds( amount );
				Print( "Ammo set to %d rounds\n", amount );
			}
		}
	}
};

extern	bool		ScriptTrace;
class ScriptTraceConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "script_trace"; }
	virtual	const char * Get_Help( void ) override	{ return "SCRIPT_TRACE - toggles debug trace of all scipt events and commands."; }
	virtual	void Activate( const char * /* input */ ) override {
      ScriptTrace = !ScriptTrace;
		Print( "ScriptTrace is %s\n", ScriptTrace ? "ON" : "OFF" );
	}
};


class DebugTypeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "debug_type"; }
   const char * Get_Alias( void ) override { return "dt"; }
	virtual	const char * Get_Help( void ) override	{ return
		"DEBUG_TYPE [type|on|off] - toggles the debug display of type (info, warning, error, script)"; }
	virtual	void Activate( const char * input ) override {
		char str[128];
		sprintf(str,"ERROR (%s)\n", input );
		if (stricmp(input,"info") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_INFORMATION );
			sprintf(str, "Information Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_INFORMATION ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"warning") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_WARNING );
			sprintf(str, "Warning Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_WARNING ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"error") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_ERROR );
			sprintf(str, "Error Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_ERROR ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"script") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_SCRIPT );
			sprintf(str, "Script Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_SCRIPT ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"netadmin") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_NETWORK_ADMIN );
			sprintf(str, "Network Admin Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_NETWORK_ADMIN ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"netbasic") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_NETWORK_BASIC );
			sprintf(str, "Network Basic Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_NETWORK_BASIC ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"netprolific") == 0) {
			DebugManager::Toggle_Type_Enabled( DebugManager::DEBUG_TYPE_NETWORK_PROLIFIC );
			sprintf(str, "Network Prolific Debug %s\n", DebugManager::Is_Type_Enabled( DebugManager::DEBUG_TYPE_NETWORK_PROLIFIC ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"net") == 0) {
         DebugManager::Disable_Type( DebugManager::DEBUG_TYPE_NETWORK_ADMIN );
         DebugManager::Disable_Type( DebugManager::DEBUG_TYPE_NETWORK_BASIC );
         DebugManager::Disable_Type( DebugManager::DEBUG_TYPE_NETWORK_PROLIFIC );
			sprintf(str, "All network debug types DISABLED\n");
		} else if (stricmp(input,"on") == 0) {
			DebugManager::Enable_Type( (DebugManager::DebugType)-1 );
			sprintf(str, "All Debug Types Enabled\n" );
		} else if (stricmp(input,"off") == 0) {
			DebugManager::Disable_Type( (DebugManager::DebugType)-1 );
			sprintf(str, "All Debug Types Disabled\n" );
		}
		Print(str);
		Debug_Say((str));
	}
};

/*
class RadarZoomConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "radar_zoom"; }
	virtual	const char * Get_Help( void ) override	{ return "RADAR_ZOOM <factor> - Set radar zoom factor."; }
	virtual	void Activate( const char * input ) override {
		float zoom_factor = atof(input);
		if (zoom_factor > WWMATH_EPSILON) {
//			RadarManager::Set_Zoom_Factor(zoom_factor);
			Print( "Radar zoomed\n" );
		} else {
			Print( "Error - zoom_factor must be small and positive.\n" );
		}
	}
};


class RadarMaxConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "radar_max"; }
	virtual	const char * Get_Help( void ) override	{ return "RADAR_MAX - zoom out the radar to world extents."; }
	virtual	void Activate( const char * input ) override {
		float world_size = The_Game()->Get_Maximum_World_Distance();
		if (world_size > WWMATH_EPSILON) {
//			RadarManager::Set_Zoom_Factor(1 / world_size);
			Print( "Radar zoomed\n" );
		} else {
			Print( "Error - world size not set.\n" );
		}
	}
};
*/

class DefectConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "defect"; }
	virtual	const char * Get_Help( void ) override	{ return "DEFECT - switch to the opposite team (gods only)."; }
	virtual	void Activate( const char * /* input */ ) override {

		cPlayer * p_player = cNetwork::Get_My_Player_Object();

		if (p_player != nullptr && p_player->Invulnerable.Is_True()) {
			cChangeTeamEvent * p_event = new cChangeTeamEvent;
			p_event->Init();
		} else {
			Print(Get_Help());
		}
	}
};


class RadarToggleConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "radar_toggle"; }
	virtual	const char * Get_Help( void ) override	{ return "RADAR_TOGGLE - toggles the Radar."; }
	virtual	void Activate( const char * /* input */ ) override {
		RadarManager::Set_Hidden( !RadarManager::Is_Hidden() );
      Print( "Radar toggled\n" );
	}
};


class ShellCommandConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "shell"; }
	virtual	const char * Get_Help( void ) override	{ return "SHELL - SHell command (server only)."; }
	virtual	void Activate( const char * input ) override {
      if (IS_SOLOPLAY || !cNetwork::I_Am_Server()) {
         Print("SH is for server only.\n");
      } else {
         cNetwork::Shell_Command(input);
      }
	}
};


/*
class RshCommandConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "rsh"; }
	virtual	const char * Get_Help( void ) override	{ return "RSH - Remote SHell command (server only)."; }
	virtual	void Activate( const char * input ) override {
      if (IS_SOLOPLAY || !cNetwork::I_Am_Server()) {
         Print("rsh is for server only.\n");
      } else {
         Print("Sending command to clients...\n");
         cNetwork::Send_Server_Rsh_Command(input);
      }
	}
};
*/


class LrshCommandConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "lrsh"; }
	virtual	const char * Get_Help( void ) override	{ return "LRSH - Local and Remote SHell command (server only)."; }

	virtual	void Activate( const char * input ) override {
      /*
		ConsoleFunctionManager::Parse_Input(input);
      char rsh_command[500];
      WWASSERT(strlen(input) < sizeof(rsh_command) - 10);
      sprintf(rsh_command, "rsh %s", input);
      ConsoleFunctionManager::Parse_Input(rsh_command);
		*/

      if (IS_SOLOPLAY || !cNetwork::I_Am_Server() || ::strlen(input) == 0) {
		   Print(Get_Help());
		} else {
			cConsoleCommandEvent * p_event = new cConsoleCommandEvent;
			p_event->Init(input);
		}
	}
};


class SrshCommandConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "srsh"; }
	virtual	const char * Get_Help( void ) override	{ return "SRSH - C->S Remote SHell command (client only)."; }

	virtual	void Activate( const char * input ) override {

		if (cNetwork::I_Am_Only_Client() && ::strlen(input) > 0) {
			cCsConsoleCommandEvent * p_event = new cCsConsoleCommandEvent;
			p_event->Init(input);
		} else {
		   Print(Get_Help());
		}
	}
};


class MainMenuConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "main_menu"; }
	virtual	const char * Get_Alias( void ) override	{ return "mm"; }
	virtual	const char * Get_Help( void ) override	{ return "MAIN_MENU - go back to the main menu."; }
	virtual	void Activate( const char * /* input */ ) override {
		cDevOptions::GoToMainMenu.Set(true);
		cDevOptions::QuickFullExit.Set(true);
	}
};


class MaxFacingPenaltyConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "max_facing_penalty"; }
	virtual	const char * Get_Alias( void ) override{ return "mfp"; }
	virtual	const char * Get_Help( void ) override	{ return "MAX_FACING_PENALTY - set the max facing penalty. Server only. Float. Use zero to disable."; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
		if (cNetwork::I_Am_Server()) {
			float f = ::strtof(input, nullptr);
			cUserOptions::MaxFacingPenalty.Set(f);
         Print( "MaxFacingPenalty set to %5.2f.", f);
		} else {
		   Print(Get_Help());
		}
	}
};


class IrrelevancePenaltyConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "irrelevance_penalty"; }
	//virtual	const char * Get_Alias( void ) override{ return ""; }
	virtual	const char * Get_Help( void ) override	{ return "IRRELEVANCE_PENALTY - set the irrelevance penalty. Server only. Float. Use zero to disable."; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
		if (cNetwork::I_Am_Server()) {
			float f = ::strtof(input, nullptr);
			cUserOptions::IrrelevancePenalty.Set(f);
         Print( "IrrelevancePenalty set to %5.2f.", f);
		} else {
		   Print(Get_Help());
		}
	}
};


class MissionConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual const char * Get_Name(void) override {return "mission";}
	virtual const char * Get_Help(void) override {return "MISSION <success/failure> end mission with given status.";}
	virtual void Activate(const char * input) override {
		if (!stricmp(input, "success")) {
			CombatManager::Mission_Complete(true);
		} else if (!stricmp(input, "failure")) {
			CombatManager::Mission_Complete(false);
		} else {
		   Print(Get_Help());
		}
	}
};


class MoneyConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual const char * Get_Name(void) override {return "money";}
	virtual const char * Get_Help(void) override {return "MONEY <amount> - money cheat, for gods only.";}

	virtual void Activate(const char * input) override {

		int amount = ::atoi(input);

		if (cNetwork::I_Am_God()) {
			cMoneyEvent * p_money = new cMoneyEvent;
			p_money->Init(amount);
		} else {
		   Print(Get_Help());
		}
	}
};


class ScoreConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual const char * Get_Name(void) override {return "score";}
	virtual const char * Get_Help(void) override {return "SCORE <increment> - score cheat, for gods only.";}

	virtual void Activate(const char * input) override {

		int amount = ::atoi(input);

		if (cNetwork::I_Am_God()) {
			cScoreEvent * p_score = new cScoreEvent;
			p_score->Init(amount);
		} else {
		   Print(Get_Help());
		}
	}
};


class MultiHudConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "multihud"; }
	virtual	const char * Get_Help( void ) override	{ return "MULTIHUD - toggle the multiplayer HUD."; }
	virtual	void Activate( const char * /* input */ ) override {
      CombatGameModeClass::Toggle_Multi_Hud();
      Print("MultiHud toggled\n");
	}
};


class SetPacketLossConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_packet_loss"; }
	virtual	const char * Get_Alias( void ) override	{ return "spl"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_PACKET_LOSS <percentage> - simulate packetloss at given percentage"; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      double percentage;
      percentage = atof(input);
      if (percentage > -WWMATH_EPSILON && percentage < 100 + WWMATH_EPSILON) {
         cNetwork::Set_Simulated_Packet_Loss_Pc((int)percentage);
			ConsoleFunctionManager::Parse_Input("clear ns"); // reset stats
      } else {
		   Print(Get_Help());
      }
   }
};


class SetBandwidthBudgetOutToggleConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "toggle_sbbo"; }
	virtual	const char * Get_Help( void ) override		{ return "TOGGLE_SBBO - toggle cSbboManager (server only)."; }

	virtual	void Activate( const char * /* input */ ) override {

		if (cNetwork::I_Am_Server()) {
			bool is_enabled = cSbboManager::Toggle_Is_Enabled();
			Print("cSbboManager %s enabled.\n", is_enabled ? "IS" : "is NOT");
		} else {
			Print(Get_Help());
		}
	}
};



class SetSendFrequencyConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "send_frequency"; }
	virtual	const char * Get_Help( void ) override		{ return "send_freqency <delay> - set delay between packet sends."; }

	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      int delay = atoi(input);
		PacketManager.Set_Flush_Frequency((unsigned int) delay);
		if (delay) {
			Print("Packets will be sent no more than %d times per second.\n", 1000 / delay);
		} else {
			Print("Packets will be sent immediately.\n");
		}
	}
};


class TogglePacketDeltasConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "packet_delta"; }
	virtual	const char * Get_Help( void ) override		{ return "packet_delta - toggle packet delta enable flag."; }

	virtual	void Activate( const char * ) override {
		bool delta = PacketManager.Toggle_Allow_Deltas();

		if (delta) {
			Print("Packets can be sent as deltas.\n");
		} else {
			Print("Packets are not allowed to be sent as deltas.\n");
		}
	}
};



class TogglePacketComboConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "packet_combo"; }
	virtual	const char * Get_Help( void ) override		{ return "packet_combo - toggle packet packing."; }

	virtual	void Activate( const char * ) override {
		bool combos = PacketManager.Toggle_Allow_Combos();

		if (combos) {
			Print("Packets of different sizes can be combined into one packet.\n");
		} else {
			Print("Packets of different sizes cannot be combined into one packet.\n");
		}
	}
};



class TogglePacketOptConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "packet_opt"; }
	virtual	const char * Get_Help( void ) override		{ return "packet_opt <state> - enable low level packet optimizations."; }

	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      int on = atoi(input);

		if (on) {
			PacketManager.Set_Flush_Frequency(1000 / 15);
			bool ok = PacketManager.Toggle_Allow_Deltas();
			if (!ok) {
				PacketManager.Toggle_Allow_Deltas();
			}
			ok = PacketManager.Toggle_Allow_Combos();
			if (!ok) {
				PacketManager.Toggle_Allow_Combos();
			}
			Print("Low level packet optioizations enabled.\n");
		} else {
			PacketManager.Set_Flush_Frequency(0);
			bool ok = PacketManager.Toggle_Allow_Deltas();
			if (ok) {
				PacketManager.Toggle_Allow_Deltas();
			}
			ok = PacketManager.Toggle_Allow_Combos();
			if (ok) {
				PacketManager.Toggle_Allow_Combos();
			}
			Print("Low level packet optioizations disabled.\n");
		}
	}
};



class SetLatencyConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "latency"; }
	virtual	const char * Get_Help( void ) override		{ return "latency <low,high> - set simulated latency on packets."; }

	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
		int low,high,current;
		int fields = sscanf(input, "%d,%d", &low, &high);
		if (fields == 1) {
			high = low;
			fields = 2;
		}
		if (fields == 2) {
			cConnection::Set_Latency(low, high);
			Print("Latency set\n");
		}

		cConnection::Get_Latency(low, high, current);
		Print("Latency set to %d,%d (current latency is %d ms)\n", low, high, current);
	}
};



class ToggleBandwidthBalancerConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "bw_balance"; }
	virtual	const char * Get_Help( void ) override		{ return "bw_balance - toggle bandwidth balancer code enable."; }

	virtual	void Activate( const char * ) override {
		if (BandwidthBalancer.IsEnabled == false) {
			BandwidthBalancer.IsEnabled = true;
			Print("Using bandwidth balancer.\n");
		} else {
			BandwidthBalancer.IsEnabled = false;
			Print("Not using bandwidth balancer.\n");
		}
	}
};


class ToggleNewClientUpdateMethodConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "new_update"; }
	virtual	const char * Get_Help( void ) override		{ return "new_update - toggle new client update method."; }

	virtual	void Activate( const char * ) override {
		if (cNetwork::SensibleUpdates == false) {
			cNetwork::SensibleUpdates = true;
			Print("Using new per object per client update scheme.\n");
		} else {
			cNetwork::SensibleUpdates = false;
			Print("Using old funky distance based update scheme.\n");
		}
	}
};



class SetDesiredSleepConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override	{ return "set_desired_sleep"; }
	virtual	const char * Get_Alias( void ) override	{ return "sds"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_DESIRED_SLEEP <sleep_ms> - sleep this many ms per frame."; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      int desired_sleep = atoi(input);
		if (desired_sleep >= 0) {
         cNetwork::Set_Desired_Frame_Sleep_Ms(desired_sleep);
         Print( "Frame sleep set to %d ms.", desired_sleep );
		} else {
		   Print(Get_Help());
		}
	}
};

class SetPacketDuplicationConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_packet_duplication"; }
	//virtual	const char * Get_Alias( void ) override	{ return "spd"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_PACKET_DUPLICATION <percentage> - simulate packet duplication at given percentage"; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      double percentage = atof(input);
      if (percentage > -WWMATH_EPSILON && percentage < 100 + WWMATH_EPSILON) {
         cNetwork::Set_Simulated_Packet_Duplication_Pc((int)percentage);
			ConsoleFunctionManager::Parse_Input("clear ns"); // reset stats
      } else {
		   Print(Get_Help());
      }
   }
};


class SetPacketLatencyRangeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_packet_latency_range"; }
	virtual	const char * Get_Alias( void ) override	{ return "latency"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_PACKET_LATENCY_RANGE <min> <max> - sets a range for simulated latency (in ms)"; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      int min_latency_ms;
      int max_latency_ms;
      int num_args = sscanf(input, "%d %d", &min_latency_ms, &max_latency_ms);

      if (num_args == 2 && min_latency_ms >= 0 && max_latency_ms >= 0 &&
         min_latency_ms <= max_latency_ms) {
         cNetwork::Set_Simulated_Latency_Range_Ms(min_latency_ms, max_latency_ms);
      } else {
		   Print(Get_Help());
      }
	}
};


class SetSpamCountConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_spam_count"; }
	virtual	const char * Get_Alias( void ) override	{ return "ssc"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_SPAM_COUNT <count> - S->C state packet is sent this many extra times."; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      int spam_count = atoi(input);
		if (spam_count >= 0) {
         cNetwork::Set_Spam_Count(spam_count);
         //Print( "Spam Count set to %d ms.", desired_sleep );
		} else {
		   Print(Get_Help());
		}
	}
};


class ClearConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "clear"; }
	virtual	const char * Get_Help( void ) override	{ return "CLEAR <object> - clear specified object."; }
	virtual	void Activate( const char * input ) override {

		if (cMiscUtil::Is_String_Same(input, "isc")) {
		   NetworkObjectMgrClass::Reset_Import_State_Counts();
		} else if (cMiscUtil::Is_String_Same(input, "netstats") ||
			cMiscUtil::Is_String_Same(input, "ns")) {
		   if (cNetwork::PClientConnection != nullptr) {
			   cNetwork::PClientConnection->Init_Stats();
			   Print("Network stats for client connection reset.\n");
		   }
		   if (cNetwork::PServerConnection != nullptr) {
			   cNetwork::PServerConnection->Init_Stats();
			   Print("Network stats for server connection reset.\n");
		   }
      } else if (cMiscUtil::Is_String_Same(input, "packet_simulation") ||
			cMiscUtil::Is_String_Same(input, "ps")) {
         ConsoleFunctionManager::Parse_Input("spl 0");
         ConsoleFunctionManager::Parse_Input("spd 0");
         ConsoleFunctionManager::Parse_Input("latency 0 0");
		} else if (cMiscUtil::Is_String_Same(input, "0")) {
			ConsoleFunctionManager::Parse_Input("clear ht");
			ConsoleFunctionManager::Parse_Input("clear isc");
			ConsoleFunctionManager::Parse_Input("clear ns");
			ConsoleFunctionManager::Parse_Input("clear ps");
		} else if (cMiscUtil::Is_String_Same(input, "")) {

         Print("Valid clear objects:\n");
         Print("     help_text - clears any help text\n");
         Print("     isc - reset the import state count of all smart game objects\n");
         Print("     netstats - reinitialize net stats\n");
         Print("     packet_simulation - turns off packet simulation (loss, duplication, latency)\n");

      } else {
			Print("Error: no such clear object.\n");
		}
	}
};


class ClientHintFactorConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "client_hint_factor"; }
	virtual	const char * Get_Alias( void ) override{ return "chf"; }
	virtual	const char * Get_Help( void ) override	{ return "CLIENT_HINT_FACTOR - set the client hint factor (worst:average). Client only. Float. Use zero to disable."; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
		if (cNetwork::I_Am_Only_Client()) {
			float chf = ::strtof(input, nullptr);
			cUserOptions::ClientHintFactor.Set(chf);
         Print( "ClientHintFactor set to %5.2f.", chf);
		} else {
		   Print(Get_Help());
		}
	}
};


class CreateBotConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "create_bot"; }
	virtual	const char * Get_Alias( void ) override	{ return "cb"; }
	virtual	const char * Get_Help( void ) override	{ return "CREATE_BOT <number> - create specified number of bots (max 20, server only)."; }
	virtual	void Activate( const char * input ) override {
      if (cNetwork::I_Am_Server()) {
			int num_bots = 1;
			if (strcmp(input, "")) {
				num_bots = atoi(input);
			}
			if (num_bots < 0) {
				num_bots = 0;
			}
			if (num_bots > 20) {
				num_bots = 20;
			}
			for (int i = 0; i < num_bots; i++) {
				WWASSERT(PTheGameData != nullptr);
				if (cPlayerManager::Count() < The_Game()->Get_Max_Players()) {
					cGod::Create_Ai_Player();
				}
			}
		} else {
		   Print(Get_Help());
		}
	}
};


class CreateGruntConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "create_grunt"; }
	virtual	const char * Get_Alias( void ) override	{ return "cg"; }
	virtual	const char * Get_Help( void ) override	{ return "CREATE_GRUNT <number> - create specified number of grunts (server only, max 20)."; }
	virtual	void Activate( const char * input ) override {
      if ( cNetwork::I_Am_Server()) {
			int num_grunts = 1;
			if (strcmp(input, "")) {
				num_grunts = atoi(input);
			}
			if (num_grunts < 0) {
				num_grunts = 0;
			}
			if (num_grunts > 20) {
				num_grunts = 20;
			}

			if (COMBAT_STAR != nullptr) {
				Vector3 star_pos;
				COMBAT_STAR->Get_Position(&star_pos);
				Vector3 star_target_pos = COMBAT_STAR->Get_Targeting_Pos();
				//star_pos.Z = 0;
				//star_target_pos.Z = 0;
				star_target_pos.Z = star_pos.Z;
				Vector3 look_vector = star_target_pos - star_pos;
				look_vector.Normalize();
				Vector3 pos = star_pos + 20 * look_vector;
				pos.Z += 0.5;

				for (int i = 0; i < num_grunts; i++) {
					cGod::Create_Grunt(pos);
				}
			}

		} else {
		   Print(Get_Help());
		}
	}
};


class CreateObjectConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "create_object"; }
	virtual	const char * Get_Alias( void ) override	{ return "co"; }
	virtual	const char * Get_Help( void ) override	{ return "CREATE_OBJECT <name> - create the named object (server only). Use with caution."; }
	virtual	void Activate( const char * input ) override {
      if (COMBAT_STAR && cNetwork::I_Am_Server()) {
         PhysicalGameObj * obj = ObjectLibraryManager::Create_Object(input);
			//if (obj != nullptr && obj->As_SmartGameObj() != nullptr) {
	      if (obj != nullptr) {
            Vector3 position;
            COMBAT_STAR->Get_Position(&position);
	         position.X += 4;
	         //position.Z += 4;
		      Matrix3D tm(1);
		      tm.Set_Translation(position);
		      obj->Set_Transform(tm);
				obj->Start_Observers();
			} else {
		      Print("Failed to create %s.", input);
         }
		} else {
		   Print(Get_Help());
		}
	}
};


class ListConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "list"; }
	//virtual	const char * Get_Alias( void ) override	{ return "l"; }
	virtual	const char * Get_Help( void ) override	{ return "LIST <object> - show list of specified object."; }
	virtual	void Activate( const char * input ) override {

		if (cMiscUtil::Is_String_Same(input, "active_game_modes") ||
			cMiscUtil::Is_String_Same(input, "agm")) {
		   GameModeManager::List_Active_Game_Modes();
		} else if (cMiscUtil::Is_String_Same(input, "definitions") ||
			cMiscUtil::Is_String_Same(input, "d")) {
		   DefinitionMgrClass::List_Available_Definitions(CLASSID_GAME_OBJECTS);
      } else if (cMiscUtil::Is_String_Same(input, "")) {

         Print("Valid list objects:\n");
         Print("     active_game_modes - list all active game modes\n");
         Print("     definitions - list all definition names\n");

      } else {
			Print("Error: no such list object.\n");
		}
	}
};


class BreakExecutionConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "break"; }
	virtual	const char * Get_Help( void ) override	{ return "BREAK - break execution. Do not use this just for fun."; }
	virtual	void Activate( const char * /* input */ ) override {
		Print("Breaking execution on demand.\n");
		debugbreak();
	}
};


class CrashExecutionConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "crash"; }
	virtual	const char * Get_Help( void ) override	{ return "CRASH - crash execution. Do not use this just for fun."; }
	virtual	void Activate( const char * /* input */ ) override {
		Print("Bang....\n");
		char *zero = nullptr;
		*zero = 0;
	}
};


class GiveWeaponsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "give_weapons"; }
	virtual	const char * Get_Help( void ) override	{ return "GIVE_WEAPONS - give the commando all the weapons (single-player only)"; }
	virtual	void Activate( const char * /* input */ ) override {

      if (cNetwork::I_Am_Server()) {
			if (IS_SOLOPLAY && COMBAT_STAR ) {
				COMBAT_STAR->Give_All_Weapons();
			} else {
				Print("Use FREE_WEAPONS instead in a multiplayer game.\n");
			}
		} else {
		   Print(Get_Help());
		}
	}
};


class GodConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "god"; }
	virtual	const char * Get_Help( void ) override	{ return "GOD - if you are a true believer, you may toggle your godlike status.\n"; }

	virtual	void Activate( const char * input ) override {

		StringClass password;
		password.Format("%s", input);

      if (cNetwork::I_Am_Client() &&
			(!password.Is_Empty() || cDevOptions::IBelieveInGod.Is_True())) {

			cGodModeEvent * p_event = new cGodModeEvent;
			p_event->Init(password);

		} else {

		   Print(Get_Help());
		}
	}
};


class IBelieveInGodConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "ibelieveingod"; }
	virtual	const char * Get_Help( void ) override	{ return "IBELIEVEINGOD - toggle philosophical standpoint on this issue.\n"; }

	virtual	void Activate( const char * /* input */ ) override {

		bool believe = cDevOptions::IBelieveInGod.Toggle();

		if (believe) {
			Print("You believe in god !\n");
		} else {
			Print("You do NOT believe in god !\n");

			if (cNetwork::I_Am_Server()) {
				//
				// Reduce any gods to mere mortals
				//
				for (
					SLNode<cPlayer> * player_node = cPlayerManager::Get_Player_Object_List()->Head();
					player_node;
					player_node = player_node->Next()) {

					cPlayer * p_player = player_node->Data();
					WWASSERT(p_player != nullptr);

					if (p_player->Invulnerable.Is_True()) {
						p_player->Invulnerable.Set(false);
						p_player->Mark_As_Modified();
					}
				}
			}
		}
	}
};


class HideStarConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "hide_star"; }
	virtual	const char * Get_Help( void ) override	{ return "HIDE_STAR - toggles display of the commando."; }
	virtual	void Activate( const char * /* input */ ) override {
		if ( COMBAT_STAR ) {
			RenderObjClass	 *ro = COMBAT_STAR->Peek_Model();
			ro->Set_Hidden( !ro->Is_Hidden() );
		}
	}
};

class FlyStarConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "fly_star"; }
	virtual	const char * Get_Help( void ) override	{ return "FLY_STAR - toggles fly mode for the commando."; }
	virtual	void Activate( const char * /* input */ ) override {
		if ( COMBAT_STAR != nullptr && (IS_SOLOPLAY || cNetwork::I_Am_God() )) {
			COMBAT_STAR->Toggle_Fly_Mode();
		}
	}
};

class WatchConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "watch"; }
	//virtual	const char * Get_Alias( void ) override	{ return "w"; }
	virtual	const char * Get_Help( void ) override	{ return "WATCH <object> - show debug output of specified type."; }
   static   void         Get_Verbose_Help(char * buffer, int bufsize) {
      WWASSERT(buffer != nullptr);
      WWASSERT(bufsize >= 1000);

      buffer[0] = 0;
      strcat(buffer, "\n");
      strcat(buffer, "Valid watch objects:\n");
      strcat(buffer, "     bandwidth               (bw)\n");
      strcat(buffer, "     bandwidth_budget_out    (bbo)\n");
//strcat(buffer, "     bounty                  \n");
      strcat(buffer, "     building_data           (bd)\n");
      strcat(buffer, "     client_fps					(client_fps)\n");
      strcat(buffer, "     client_rhost_data       (crd)\n");
      strcat(buffer, "     diagnostics             (diag)\n");
      strcat(buffer, "     door_data               (dd)\n");
      strcat(buffer, "     DSAPO_data              (DSAPO_data)\n");
      strcat(buffer, "     elevator_data           (ed)\n");
      strcat(buffer, "     fps                     (fps)\n");
      strcat(buffer, "     god                     (god)\n");
      strcat(buffer, "     import_states_count     (isc)\n");
      strcat(buffer, "     ids                     (ids)\n");
      strcat(buffer, "     ip_addresses            (ip)\n");
      strcat(buffer, "     last_contact            (lc)\n");
      strcat(buffer, "     latency                 (lat)\n");
      strcat(buffer, "     list_size               (ls)\n");
      strcat(buffer, "     list_packet_size        (lps)\n");
      strcat(buffer, "     list_time               (lt)\n");
//strcat(buffer, "     menu_stack              (ms)\n");
      strcat(buffer, "     money							(money)\n");
      strcat(buffer, "     names                   (names)\n");
      strcat(buffer, "     object_tally            (ot)\n");
      strcat(buffer, "     packets                 (p)\n");
      strcat(buffer, "     ping							(ping)\n");
      strcat(buffer, "     powerup_data				(pd)\n");
      strcat(buffer, "     priorities              (pri)\n");
      strcat(buffer, "     server_rhost_data       (srd)\n");
      strcat(buffer, "     soldier_data            (sd)\n");
      strcat(buffer, "     spawner_data            (spawn)\n");
      strcat(buffer, "     vehicle_data            (vd)\n");
      strcat(buffer, "     wol							(wol)\n");
      strcat(buffer, "     wol_location            (wl)\n");

      strcat(buffer, "     PacketsSentServer						(pss)\n");
      strcat(buffer, "     PacketsSentClient						(psc)\n");
      strcat(buffer, "     PacketsRecdServer						(prs)\n");
      strcat(buffer, "     PacketsRecdClient						(prc)\n");
      strcat(buffer, "     AvgSizePacketsSentServer			(aspss)\n");
      strcat(buffer, "     AvgSizePacketsSentClient			(aspsc)\n");
      strcat(buffer, "     AvgSizePacketsRecdServer			(asprs)\n");
      strcat(buffer, "     AvgSizePacketsRecdClient			(asprc)\n");
      strcat(buffer, "     BytesSentServer						(bss)\n");
      strcat(buffer, "     BytesSentClient						(bsc)\n");
      strcat(buffer, "     BytesRecdServer						(brs)\n");
      strcat(buffer, "     BytesRecdClient						(brc)\n");

      strcat(buffer, "     WwnetPacketsSentServer				(wpss)\n");
      strcat(buffer, "     WwnetPacketsSentClient				(wpsc)\n");
      strcat(buffer, "     WwnetPacketsRecdServer				(wprs)\n");
      strcat(buffer, "     WwnetPacketsRecdClient				(wprc)\n");
      strcat(buffer, "     WwnetAvgSizePacketsSentServer		(waspss)\n");
      strcat(buffer, "     WwnetAvgSizePacketsSentClient		(waspsc)\n");
      strcat(buffer, "     WwnetAvgSizePacketsRecdServer		(wasprs)\n");
      strcat(buffer, "     WwnetAvgSizePacketsRecdClient		(wasprc)\n");
      strcat(buffer, "     WwnetBytesSentServer					(wbss)\n");
      strcat(buffer, "     WwnetBytesSentClient					(wbsc)\n");
      strcat(buffer, "     WwnetBytesRecdServer					(wbrs)\n");
      strcat(buffer, "     WwnetBytesRecdClient					(wbrc)\n");

   }
	virtual	void Activate( const char * input ) override {

		if (cMiscUtil::Is_String_Same(input, "bandwidth") ||
			cMiscUtil::Is_String_Same(input, "bw")) {
			cDevOptions::ShowBandwidth.Toggle();
      //} else if (cMiscUtil::Is_String_Same(input, "bandwidth_budget_out") ||
		//	cMiscUtil::Is_String_Same(input, "bbo")) {
		//	cDevOptions::ShowBandwidthBudgetOut.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "client_fps")) {
			cDevOptions::ShowClientFps.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "client_rhost_data") ||
			cMiscUtil::Is_String_Same(input, "crd")) {
			cDevOptions::ShowClientRhostData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "diagnostics") ||
			cMiscUtil::Is_String_Same(input, "diag")) {
			cDevOptions::ShowDiagnostics.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "gs_auth")) {
			cDevOptions::ShowGameSpyAuthState.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "ids")) {
			cDevOptions::ShowId.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "money")) {
			cDevOptions::ShowMoney.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "ping")) {
			cDevOptions::ShowPing.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "ip_addresses") ||
			cMiscUtil::Is_String_Same(input, "ip")) {
			cDevOptions::ShowIpAddresses.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "fps")) {
			cDevOptions::ShowFps.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "god")) {
			cDevOptions::ShowGodStatus.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "import_states_count") ||
			cMiscUtil::Is_String_Same(input, "isc")) {
			bool is_shown = cDevOptions::ShowImportStates.Toggle();
			if (is_shown) {
				Print("You may use RESET_ISC to reset the counts.\n");
			}
      } else if (cMiscUtil::Is_String_Same(input, "import_states_count_soldiers_vehicles") ||
			cMiscUtil::Is_String_Same(input, "iscsv")) {
			bool is_shown = cDevOptions::ShowImportStatesSV.Toggle();
			if (is_shown) {
				Print("You may use RESET_ISC to reset the counts.\n");
			}
		} else if (cMiscUtil::Is_String_Same(input, "latency") ||
			cMiscUtil::Is_String_Same(input, "lat")) {
			ConsoleFunctionManager::Parse_Input("clear ns"); // reset stats
			cDevOptions::ShowLatency.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "last_contact") ||
			cMiscUtil::Is_String_Same(input, "lc")) {
			//ConsoleFunctionManager::Parse_Input("clear ns"); // reset stats
			cDevOptions::ShowLastContact.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "list_size") ||
			cMiscUtil::Is_String_Same(input, "ls")) {
			cDevOptions::ShowListSizes.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "list_packet_size") ||
			cMiscUtil::Is_String_Same(input, "lps")) {
			cDevOptions::ShowListPacketSizes.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "list_time") ||
			cMiscUtil::Is_String_Same(input, "lt")) {
			cDevOptions::ShowListTimes.Toggle();
		/*
		} else if (cMiscUtil::Is_String_Same(input, "menu_stack") ||
			cMiscUtil::Is_String_Same(input, "ms")) {
			cDevOptions::ShowMenuStack.Toggle();
		*/
		} else if (cMiscUtil::Is_String_Same(input, "names")) {
         if (IS_MISSION) {
            Print("Name showing is for multiplayer only.\n");
         } else {
            bool on = cUserOptions::ShowNamesOnSoldier.Toggle();
            if (on) {
               Print("Name showing is now ON\n");
            } else {
               Print("Name showing is now OFF\n");
            }
         }
		} else if (cMiscUtil::Is_String_Same(input, "object_tally") ||
			cMiscUtil::Is_String_Same(input, "ot")) {
			cDevOptions::ShowObjectTally.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "inactive")) {
			cDevOptions::ShowInactivePlayers.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "PacketsSentServer")			|| cMiscUtil::Is_String_Same(input, "pss")) {
			cDevOptions::PacketsSentServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "PacketsSentClient")			|| cMiscUtil::Is_String_Same(input, "psc")) {
			cDevOptions::PacketsSentClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "PacketsRecdServer")			|| cMiscUtil::Is_String_Same(input, "prs")) {
			cDevOptions::PacketsRecdServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "PacketsRecdClient")			|| cMiscUtil::Is_String_Same(input, "prc")) {
			cDevOptions::PacketsRecdClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "AvgSizePacketsSentServer")	|| cMiscUtil::Is_String_Same(input, "aspss")) {
			cDevOptions::AvgSizePacketsSentServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "AvgSizePacketsSentClient") || cMiscUtil::Is_String_Same(input, "aspsc")) {
			cDevOptions::AvgSizePacketsSentClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "AvgSizePacketsRecdServer") || cMiscUtil::Is_String_Same(input, "asprs")) {
			cDevOptions::AvgSizePacketsRecdServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "AvgSizePacketsRecdClient") || cMiscUtil::Is_String_Same(input, "asprc")) {
			cDevOptions::AvgSizePacketsRecdClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "BytesSentServer")				|| cMiscUtil::Is_String_Same(input, "bss")) {
			cDevOptions::BytesSentServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "BytesSentClient")				|| cMiscUtil::Is_String_Same(input, "bsc")) {
			cDevOptions::BytesSentClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "BytesRecdServer")				|| cMiscUtil::Is_String_Same(input, "brs")) {
			cDevOptions::BytesRecdServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "BytesRecdClient")				|| cMiscUtil::Is_String_Same(input, "brc")) {
			cDevOptions::BytesRecdClient.Toggle();

      } else if (cMiscUtil::Is_String_Same(input, "WwnetPacketsSentServer")			|| cMiscUtil::Is_String_Same(input, "wpss")) {
			cDevOptions::WwnetPacketsSentServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetPacketsSentClient")			|| cMiscUtil::Is_String_Same(input, "wpsc")) {
			cDevOptions::WwnetPacketsSentClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetPacketsRecdServer")			|| cMiscUtil::Is_String_Same(input, "wprs")) {
			cDevOptions::WwnetPacketsRecdServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetPacketsRecdClient")			|| cMiscUtil::Is_String_Same(input, "wprc")) {
			cDevOptions::WwnetPacketsRecdClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetAvgSizePacketsSentServer")	|| cMiscUtil::Is_String_Same(input, "waspss")) {
			cDevOptions::WwnetAvgSizePacketsSentServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetAvgSizePacketsSentClient")  || cMiscUtil::Is_String_Same(input, "waspsc")) {
			cDevOptions::WwnetAvgSizePacketsSentClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetAvgSizePacketsRecdServer")  || cMiscUtil::Is_String_Same(input, "wasprs")) {
			cDevOptions::WwnetAvgSizePacketsRecdServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetAvgSizePacketsRecdClient")  || cMiscUtil::Is_String_Same(input, "wasprc")) {
			cDevOptions::WwnetAvgSizePacketsRecdClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetBytesSentServer")				|| cMiscUtil::Is_String_Same(input, "wbss")) {
			cDevOptions::WwnetBytesSentServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetBytesSentClient")				|| cMiscUtil::Is_String_Same(input, "wbsc")) {
			cDevOptions::WwnetBytesSentClient.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetBytesRecdServer")				|| cMiscUtil::Is_String_Same(input, "wbrs")) {
			cDevOptions::WwnetBytesRecdServer.Toggle();
      } else if (cMiscUtil::Is_String_Same(input, "WwnetBytesRecdClient")				|| cMiscUtil::Is_String_Same(input, "wbrc")) {
			cDevOptions::WwnetBytesRecdClient.Toggle();

      } else if (cMiscUtil::Is_String_Same(input, "priorities") ||
			cMiscUtil::Is_String_Same(input, "pri")) {
			cDevOptions::ShowPriorities.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "server_rhost_data") ||
			cMiscUtil::Is_String_Same(input, "srd")) {
			cDevOptions::ShowServerRhostData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "soldier_data") ||
			cMiscUtil::Is_String_Same(input, "sd")) {
			cDevOptions::ShowSoldierData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "spawner_data") ||
			cMiscUtil::Is_String_Same(input, "spawn")) {
			cDevOptions::ShowSpawnerData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "vehicle_data") ||
			cMiscUtil::Is_String_Same(input, "vd")) {
			cDevOptions::ShowVehicleData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "door_data") ||
			cMiscUtil::Is_String_Same(input, "dd")) {
			cDevOptions::ShowDoorData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "elevator_data") ||
			cMiscUtil::Is_String_Same(input, "ed")) {
			cDevOptions::ShowElevatorData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "DSAPO_data") ||
			cMiscUtil::Is_String_Same(input, "DSAPO_data")) {
			cDevOptions::ShowDSAPOData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "building_data") ||
			cMiscUtil::Is_String_Same(input, "bd")) {
			cDevOptions::ShowBuildingData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "powerup_data") ||
			cMiscUtil::Is_String_Same(input, "pd")) {
			cDevOptions::ShowPowerupData.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "wol_location") ||
			cMiscUtil::Is_String_Same(input, "wl")) {
			cDevOptions::ShowWolLocation.Toggle();
		} else if (cMiscUtil::Is_String_Same(input, "0")) {
			//
			// Turn off any watch objects
			//
			cDevOptions::ShowBandwidth.Set(false);
			//cDevOptions::ShowBandwidthBudgetOut.Set(false);
			cDevOptions::ShowDiagnostics.Set(false);
         cDevOptions::ShowFps.Set(false);
         cDevOptions::ShowGodStatus.Set(false);
         //cDevOptions::ShowPacketGraphs.Set(false);
         cDevOptions::ShowPriorities.Set(false);
			cDevOptions::ShowLatency.Set(false);
			cDevOptions::ShowLastContact.Set(false);
			cDevOptions::ShowListSizes.Set(false);
			cDevOptions::ShowListTimes.Set(false);
			cDevOptions::ShowListPacketSizes.Set(false);
			cDevOptions::ShowMenuStack.Set(false);
			cDevOptions::ShowObjectTally.Set(false);
			cDevOptions::ShowInactivePlayers.Set(false);
         cDevOptions::ShowSoldierData.Set(false);
         cDevOptions::ShowSpawnerData.Set(false);
         cDevOptions::ShowVehicleData.Set(false);
         cDevOptions::ShowDoorData.Set(false);
         cDevOptions::ShowElevatorData.Set(false);
         cDevOptions::ShowDSAPOData.Set(false);
         cDevOptions::ShowBuildingData.Set(false);
         cDevOptions::ShowPowerupData.Set(false);
			cDevOptions::ShowImportStates.Set(false);
			cDevOptions::ShowImportStatesSV.Set(false);
			cDevOptions::ShowServerRhostData.Set(false);
			cDevOptions::ShowClientRhostData.Set(false);
			cDevOptions::ShowClientFps.Set(false);
			cDevOptions::ShowWolLocation.Set(false);
			cDevOptions::ShowPing.Set(false);
			cDevOptions::ShowMoney.Set(false);
			cDevOptions::ShowGameSpyAuthState.Set(false);

      } else if (cMiscUtil::Is_String_Same(input, "")) {

			cDevOptions::ShowWolLocation.Toggle();

			if (Get_Text_Display()) {
				if (cDevOptions::ShowWolLocation.Is_True())
				{
					char buffer[10000];
					Get_Verbose_Help(buffer, sizeof(buffer));
					Get_Text_Display()->Set_Verbose_Help_Text(buffer);
				}
				else
				{
					Get_Text_Display()->Set_Verbose_Help_Text("");
				}
			}

      } else {
			Print("Error: no such watch object.\n");
		}
	}
};


class SurfaceEffectsDebugConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "surface_effects_debug"; }
	virtual	const char * Get_Help( void ) override	{ return "SURFACE_EFFECTS_DEBUG - toggles display of surface effects debug info."; }
	virtual	void Activate( const char * /* input */ ) override {
		SurfaceEffectsManager::Toggle_Debug();
		Print( "Surface Effects Debug Toggled\n" );
	}
};

class SurfaceTypeOverrideConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "surface_type_override"; }
	virtual	const char * Get_Help( void ) override	{ return "SURFACE_TYPE_OVERRIDE <int> - all surfaces forced to be given type (-1 disables)."; }
	virtual	void Activate( const char * input ) override {
		int type = atoi( input );
		SurfaceEffectsManager::Set_Override_Surface_Type(type);;
		Print( "Override Surface Type Set: %d\n",type );
	}
};

class SurfacePhysicsOverrideConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "surface_physics_override"; }
	virtual	const char * Get_Help( void ) override	{ return "SURFACE_PHYSICS_OVERRIDE <friction> <drag> - force friction and drag for all surfaces (0.0 disables)."; }
	virtual	void Activate( const char * input ) override {
		float friction,drag;
		if (sscanf(input, "%f %f", &friction, &drag) == 2) {
			PhysicsConstants::Set_Override_Surface_Friction(friction);
			PhysicsConstants::Set_Override_Surface_Drag(drag);
			Print( "Friction set to: %f  Drag set to: %f\n",friction,drag );
		} 	else if (sscanf(input, "%f", &friction) == 1) {
			PhysicsConstants::Set_Override_Surface_Friction(friction);
			Print( "Friction set to: %f \n",friction);
		}
	}
};


class SwapTeamsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "swap_teams"; }
	virtual	const char * Get_Help( void ) override	{ return "SWAP_TEAMS - switch everyone to the opposite team (server only)."; }
	virtual	void Activate( const char * /* input */ ) override {
		if (cNetwork::I_Am_Server()) {
			//if (The_Game() != nullptr && The_Game()->Is_Team_Game()) {
			if (The_Game() != nullptr) {
				for (
					SLNode<cPlayer> * player_node = cPlayerManager::Get_Player_Object_List()->Head();
					player_node;
					player_node = player_node->Next()) {

					cPlayer * p_player = player_node->Data();
					WWASSERT(p_player != nullptr);

					int team = p_player->Get_Player_Type();

					WWASSERT(team == PLAYERTYPE_NOD || team == PLAYERTYPE_GDI);

					int new_team = PLAYERTYPE_NOD;
					if (team == PLAYERTYPE_NOD)
					{
						new_team = PLAYERTYPE_GDI;
					}
					else
					{
						new_team = PLAYERTYPE_NOD;
					}

					p_player->Set_Player_Type(new_team);

					SoldierGameObj * p_soldier = GameObjManager::Find_Soldier_Of_Client_ID(p_player->Get_Id());
					if (p_soldier != nullptr)
					{
						p_soldier->Set_Delete_Pending();
					}
				}
			}
		} else {
			Print(Get_Help());
		}
	}
};


/*
**
*/
class SetStarHealthConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_star_health"; }
	virtual	const char * Get_Alias( void ) override{ return "ssh"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_STAR_HEALTH <strength> - sets the strength of the star."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_STAR ) {
			float health = strtof( input, nullptr );
			DefenseObjectClass * defense = COMBAT_STAR->Get_Defense_Object();
			WWASSERT(defense != nullptr);
			if (health > defense->Get_Health_Max()) {
				defense->Set_Health_Max( health );
			}
			defense->Set_Health( health );
			Print( "Star Health set to %1.1f\n", defense->Get_Health() );
		}
	}
};

class SetStarSkinConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_star_skin"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_STAR_SKIN <type> - sets the type of the star's skin."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_STAR ) {
			DefenseObjectClass * defense = COMBAT_STAR->Get_Defense_Object();
			defense->Set_Skin( ArmorWarheadManager::Get_Armor_Type( input ) );
			Print( "Star Skin set to %s\n", ArmorWarheadManager::Get_Armor_Name( defense->Get_Skin() ) );
		}
	}
};

class SetStarShieldStrengthConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_star_shield_strength"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_STAR_SHIELD_STRENGTH <strength> - sets the strength of the star's shield."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_STAR ) {
			DefenseObjectClass * defense = COMBAT_STAR->Get_Defense_Object();
			defense->Set_Shield_Strength( strtof( input, nullptr ) );
			Print( "Star Shield Strength set to %1.1f\n", defense->Get_Shield_Strength() );
		}
	}
};

class SetStarShieldTypeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "set_star_shield_type"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_STAR_SHIELD_TYPE <type> - sets the type of the star's shield."; }
	virtual	void Activate( const char * input ) override {
		if ( COMBAT_STAR ) {
			DefenseObjectClass * defense = COMBAT_STAR->Get_Defense_Object();
			defense->Set_Shield_Type( ArmorWarheadManager::Get_Armor_Type( input ) );
			Print( "Star Sheild_Type set to %s\n", ArmorWarheadManager::Get_Armor_Name( defense->Get_Shield_Type() ) );
		}
	}
};


class BuByeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "bubye"; }
	virtual	const char * Get_Help( void ) override	{ return "BUBYE <mindist> - kills everything but the star if they are at least mindist away."; }
	virtual	void Activate( const char * input ) override {
		Vector3	star_pos(0,0,0);
		if ( COMBAT_STAR ) {
			COMBAT_STAR->Get_Position( &star_pos );
		}
		float min_dist = strtof( input, nullptr );

		int count = 0;
		SLNode<SmartGameObj> *objnode;
		for (	objnode = GameObjManager::Get_Smart_Game_Obj_List()->Head(); objnode; objnode = objnode->Next() ) {
			if ( objnode->Data() != (SmartGameObj*)COMBAT_STAR ) {
				Vector3 obj_pos;
				objnode->Data()->Get_Position( &obj_pos );
				obj_pos -= star_pos;
				if ( obj_pos.Length() > min_dist ) {
					objnode->Data()->Set_Delete_Pending();
					count++;
				}
			}
		}
		Print( "%d others say \"Bye Now!\"\n", count );

	}
};


class KillConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual LPCSTR Get_Name(void) override	{return "kill";}
	virtual LPCSTR Get_Help(void) override	{return "KILL <object types> <percent=n> <id=xxxxx> - kills % of types (vehicles,soldiers,powerups,c4s,armeds,smarts,all). Server only";}
	virtual void Activate(LPCSTR input) override {

		/*
		if (!cNetwork::I_Am_Server()) {
			Print(Get_Help());
			return;
		}
		*/

		if (strcmp(input, "")) {

			//
			// C & S can both use id=xxxxxx format
			//
			const char * p_id = ::strstr(input, "id=");
			if (p_id != nullptr) {
				int id = -1;
				::sscanf(p_id, "id=%d", &id);
				if (cNetwork::I_Am_Client()) {
					cRequestKillEvent * p_event = new cRequestKillEvent;
					p_event->Init(id);
				} else {
					NetworkObjectClass *	p_object = NetworkObjectMgrClass::Find_Object(id);
					if (p_object != nullptr) {
						p_object->Set_Delete_Pending();
					}
				}
				return;
			} else if (!cNetwork::I_Am_Server()) {
				Print(Get_Help());
				return;
			}

			//
			// Remaining functionality only for server.
			//

			const char * p_pc = strstr(input, "percent=");
			int percent = 100;
			if (p_pc != nullptr) {
				sscanf(p_pc, "percent=%d", &percent);
				if (percent < 0) {
					percent = 0;
				} else if (percent > 100) {
					percent = 100;
				}
			}

			int count = 0;
			PhysicalGameObj * p_object = nullptr;
			for (SLNode<BaseGameObj> * objnode = GameObjManager::Get_Game_Obj_List()->Head(); objnode != nullptr; objnode = objnode->Next()) {

				p_object = objnode->Data()->As_PhysicalGameObj();
				if (p_object == nullptr) {
					continue;
				}

				if (p_object->As_SmartGameObj() != nullptr && p_object == COMBAT_STAR) {
					continue;
				}

				if ((strstr(input, "vehicles")	&& p_object->As_VehicleGameObj()		!= nullptr) ||
					 (strstr(input, "soldiers")	&& p_object->As_SoldierGameObj()		!= nullptr) ||
//					 (strstr(input, "commandos")	&& p_object->As_CommandoGameObj()	!= nullptr) ||
					 (strstr(input, "powerups")	&& p_object->As_PowerUpGameObj()		!= nullptr) ||
					 (strstr(input, "c4s")			&& p_object->As_C4GameObj()			!= nullptr) ||
					 (strstr(input, "armeds")		&& p_object->As_ArmedGameObj()		!= nullptr) ||
					 (strstr(input, "smarts")		&& p_object->As_SmartGameObj()		!= nullptr) ||
					 (strstr(input, "all")) ||
					 (strstr(input, "-9"))) {

					if (rand() % 101 <= percent) {
						objnode->Data()->Set_Delete_Pending();
						count++;
					}
				}
			}

			Print("Killed a total of %d objects. Hope you're happy now.\n", count);

		} else {
			Print(Get_Help());
			Print("\n  Object types: vehicles, soldiers, commandos, powerups, c4s, armeds, smarts, all\n");
		}
	}
};


class SkeletonSliderDemoConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "skeleton_slider_demo"; }
	virtual	const char * Get_Help( void ) override	{ return "SKELETON_SLIDER_DEMO - toggles the Skeleton Slider Demo."; }
	virtual	void Activate( const char * /* input */ ) override {
		CombatManager::Toggle_Skeleton_Slider_Demo();
		Print( "Skeleton Slider Demo Toggles\n" );
	}
};

class SkeletonStandersConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "skeleton_standers"; }
	virtual	const char * Get_Help( void ) override	{ return "SKELETON_STANDERS - creates 4 skeleton demos."; }
	virtual	void Activate( const char * /* input */ ) override {

		const char * skel_names[ 4 ] = {
			"human_bt",
			"human_st",
			"human_bw",
			"human_sw",
		};

		for ( int i = 0; i < 4; i++ ) {
			PhysicalGameObjDef * def = (PhysicalGameObjDef *)DefinitionMgrClass::Find_Typed_Definition( "Commando", CLASSID_GAME_OBJECT_DEF_SOLDIER );
			WWASSERT( def );
			PhysicalGameObj * obj = (PhysicalGameObj *)def->Create();
			WWASSERT( obj );
			Matrix3D tm;
			tm.Set_Translation( Vector3( 0, i*1.2f, 0 ) );
			obj->Set_Transform( tm );
			HTreeClass *tree = WW3DAssetManager::Get_Instance()->Get_HTree( skel_names[i] );
			if ( tree ) {
				Animatable3DObjClass * robj = (Animatable3DObjClass *)obj->Peek_Model();
				robj->Set_HTree( tree );
			}
		}

		Print( "Skeleton Standers Created\n" );
	}
};

class GroupControllerTestConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "group_controller_test"; }
	virtual	const char * Get_Help( void ) override	{ return "GROUP_CONTROLLER_TEST - tests group controller code."; }
	virtual	void Activate( const char * /* input */ ) override {

#if 0
		GroupControllerClass * group = new TestGroupControllerClass();

		for ( int i = 0; i < 3; i++) {
			SoldierGameObjDef * def = (SoldierGameObjDef *)DefinitionMgrClass::Find_Definition( "Nod Minigunner" );
			WWASSERT( def );
			SoldierGameObj * obj = (SoldierGameObj *)def->Create();
			WWASSERT( obj );
			obj->Set_Control_Owner( SmartGameObj::SERVER_CONTROL_OWNER );
			obj->Set_Player_Type( PLAYERTYPE_NOD );
			Vector3 pos( 3*i, 3*i, 1 );
			obj->Set_Position( pos );

			obj->Add_Observer( group );
//			group->Add_Unit( obj );
		}

		GroupControllerManager::Add_Controller( group );

		Print( "Group Controller Test Started\n" );
#else
		Print( "Group Controller Test BROKEN\n" );
#endif
	}
};


class StarVisibilityConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "star_visibility"; }
	virtual	const char * Get_Help( void ) override	{ return "STAR_VISIBILITY - toggles the logical visibility of the star."; }
	virtual	void Activate( const char * /* input */ ) override {
		if ( COMBAT_STAR ) {
			COMBAT_STAR->Set_Is_Visible( !COMBAT_STAR->Is_Visible() );
			Print( "Star is %svisible.\n", COMBAT_STAR->Is_Visible() ? "" : "in" );
		}
	}
};


/*
class StealthConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "stealth"; }
	virtual	const char * Get_Help( void ) override	{ return "STEALTH - toggle stealth ability on star (client only)."; }
	virtual	void Activate( const char * input ) override {
		//if (COMBAT_STAR != nullptr) {
		//	COMBAT_STAR->Toggle_Stealth();
		//	Print("Star %s stealth ability\n", COMBAT_STAR->Is_Stealth_Enabled() ? "has" : "does NOT have");
		//}
      if (cNetwork::I_Am_Client()) {

			cPlayer * p_player = cNetwork::Get_My_Player_Object();

			if (p_player == nullptr) {
				Print("Error: you don't have a player object.");
			} else {

				bool i_can_stealth = p_player->StealthEnabled.Toggle();

				Print("You %s stealth ability\n", i_can_stealth ? "have" : "do NOT have");

				if (cNetwork::I_Am_Only_Client())
				{
					cStealthEvent * p_event = new cStealthEvent;
					p_event->Init(p_player->StealthEnabled.Get());
				}
				else
				{
					p_player->Mark_As_Modified();
				}
			}
		} else {
		   Print("Error: you are not a client.");
		}
	}
};
*/


class ProfileConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "profile"; }
	virtual	const char * Get_Help( void ) override	{ return "PROFILE - send a profile command (on,off,up,<#>)."; }
	virtual	void Activate( const char * input ) override {
		Get_Console()->Profile_Command( input );
	}
};


class QuickStatsConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "quick_stats"; }
	virtual	const char * Get_Alias( void ) override{ return "qs"; }
	virtual	const char * Get_Help( void ) override	{ return "QUICK_STATS - stats profile + profile on."; }
	virtual	void Activate( const char * /* input */ ) override {
      ConsoleFunctionManager::Parse_Input("stats profile");
		Get_Console()->Profile_Command( "on" );
		Get_Console()->Profile_Command( "1" );
		Get_Console()->Profile_Command( "reset" );
		if (ConsoleBox.Is_Exclusive()) {
			ConsoleBox.Set_Profile_Mode(true);
		}
	}
};


class ProfileCollectBeginConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "profile_collect_begin"; }
	virtual	const char * Get_Help( void ) override	{ return "PROFILE_COLLECT_BEGIN"; }
	virtual	void Activate( const char * /* input */ ) override {
		WWProfileManager::Begin_Collecting();
	}
};

class ProfileCollectEndConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "profile_collect_end"; }
	virtual	const char * Get_Help( void ) override	{ return "PROFILE_COLLECT_END"; }
	virtual	void Activate( const char * /* input */ ) override {
		WWProfileManager::End_Collecting("profile_log.txt");
	}
};


class SetTheStarConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "set_the_star"; }
	virtual	const char * Get_Help( void ) override	{ return "SET_THE_STAR <id> - set the star to a different object."; }
	virtual	void Activate( const char * input ) override {
		SmartGameObj * star = GameObjManager::Find_SmartGameObj( atoi( input ) );
		if ( star != nullptr && star->As_SoldierGameObj() != nullptr ) {
			CombatManager::Set_The_Star( star->As_SoldierGameObj() );
		}
	}
};


class TeleportStarConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "teleport_star"; }
	virtual	const char * Get_Help( void ) override	{ return "TELEPORT_STAR < x, y, z > - teleport the star."; }
	virtual	void Activate( const char * input ) override {
		float x,y,z;
		::sscanf( input, "%f, %f, %f", &x, &y, &z );
		if ( COMBAT_STAR ) {
			COMBAT_STAR->Set_Position( Vector3( x,y,z ) );
			Print( "Teleported Star to %f %f %f\n", x,y,z );
		}
	}
};


class ScaleTimeConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "scale_time"; }
	virtual	const char * Get_Help( void ) override	{ return "SCALE_TIME < factor > - scale time (SP only)."; }
	virtual	void Activate( const char * input ) override {
		if (IS_SOLOPLAY) {
			float scale = strtof( input, nullptr );
			if ( scale <= 0 ) {
				scale = 1;
			}
			TimeManager::Set_Time_Scale( scale );
			Print( "TimeScaling %f\n", scale );
		} else {
			Print(Get_Help());
		}
	}
};

class ParticleToggleConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "particle_toggle"; }
	virtual	const char * Get_Help( void ) override	{ return "PARTICLE_TOGGLE - toggles particle emission on and off."; }
	virtual	void Activate( const char * /* input */ ) override {
		ParticleEmitterClass::Disable_All_Emitters(!ParticleEmitterClass::Are_Emitters_Disabled());
		if (ParticleEmitterClass::Are_Emitters_Disabled()) {
			Print("Emitters disabled\n");
		} else {
			Print("Emitters enabled\n");
		}
	}
};

class TargetSteeringToggleConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "target_steering_toggle"; }
	virtual	const char * Get_Help( void ) override	{ return "TARGET_STEERING_TOGGLE - toggles vehicle targetsteering."; }
	virtual	void Activate( const char * /* input */ ) override {
		Print( "Target Steering %s\n", VehicleGameObj::Toggle_Target_Steering() ? "ON" : "OFF" );
	}
};

class DazzleReloadConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "dazzle_reload"; }
	virtual	const char * Get_Help( void ) override	{ return "DAZZLE_RELOAD - reloads the dazzle ini file (DANGEROUS!)."; }
	virtual	void Activate( const char * /* input */ ) override {

		FileClass * dazzle_ini_file = _TheFileFactory->Get_File("DAZZLE.INI");
		if (dazzle_ini_file) {
			INIClass dazzle_ini(*dazzle_ini_file);
			DazzleRenderObjClass::Init_From_INI(&dazzle_ini);
			_TheFileFactory->Return_File(dazzle_ini_file);
		}
	}
};


/*
**
** Class to print out data safe stats
**
** ST - 7/18/2001 2:06PM
*/
class DataSafeStatsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "safe_stats"; }
	virtual	const char * Get_Help( void ) override	{ return "SAFE_STATS - Show data safe statistics"; }
	virtual	void Activate( const char * /* input */ ) override {
		static char dump_buffer[1024];
		GenericDataSafeClass::Dump_Safe_Stats(dump_buffer, 1024);
		Print(dump_buffer);
	}
};





#endif // WWDEBUG, development only commands

class ScreenUVBiasConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "screen_uv_bias"; }
	//virtual	const char * Get_Alias( void ) override	{ return "sub"; }
	virtual	const char * Get_Help( void ) override	{ return "SCREEN_UV_BIAS - toggles the half pixel bias in screen text."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::Set_Screen_UV_Bias( !WW3D::Is_Screen_UV_Biased() );
      Print( "Screen UV Bias Switched\n" );
	}
};


class SetBandwidthBudgetOutConsoleFunctionClass : public ConsoleFunctionClass {
public:
   virtual	const char * Get_Name( void ) override		{ return "set_bw_budget_out"; }
	virtual	const char * Get_Alias( void ) override	{ return "sbbo"; }
	virtual	const char * Get_Help( void ) override		{ return "SET_BW_BUDGET_OUT <bps> - set total bps budget out."; }

	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
      ULONG bbo = (ULONG) ::atol(input);
		if (bbo > 0) {

      	if (cNetwork::I_Am_Server()) {
				cNetwork::PServerConnection->Set_Bandwidth_Budget_Out(bbo);
			}

      	if (cNetwork::I_Am_Client()) {
				cNetwork::PClientConnection->Set_Bandwidth_Budget_Out(bbo);

				//
				// Notify the server
				//
				cClientBboEvent * p_event = new cClientBboEvent;
				p_event->Init(bbo);
			}

			int bw_scale = (bbo * 2) / 10;
			bw_scale = (bw_scale / 1000) * 1000;
			cBandwidthGraph::Set_Scale(bw_scale);

		   if (cNetwork::PClientConnection != nullptr) {
			   cNetwork::PClientConnection->Init_Stats();
			   Print("Network stats for client connection reset.\n");
		   }
		   if (cNetwork::PServerConnection != nullptr) {
			   cNetwork::PServerConnection->Init_Stats();
			   Print("Network stats for server connection reset.\n");
		   }
			//ConsoleFunctionManager::Parse_Input("clear ns"); // reset stats
		}
	}
};



class GameOverConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "gameover"; }
	virtual	const char * Get_Help( void ) override	{ return "GAMEOVER - end current game (server only)."; }
	virtual	void Activate( const char * /* input */ ) override {

		bool print_help = false;
      if (IS_MISSION) {
			//
			// Fail the mission
			//
#ifdef WWDEBUG
	      ConsoleFunctionManager::Parse_Input("mission failure");
#else //WWDEBUG
			print_help = true;
#endif //WWDEBUG
		} else if (!cNetwork::I_Am_Server()) {
			print_help = true;
      } else {
			if (cNetwork::I_Am_Only_Server()) {
         	Print("Terminating game on demand...\n");
         	cGameData::Set_Manual_Restart(true);
			} else {
#ifdef WWDEBUG
         	cGameData::Set_Manual_Restart(true);
#else //WWDEBUG
				print_help = true;
#endif //WWDEBUG
			}
      }
		if (print_help) {
			Print("GAMEOVER is for dedicated server only\n");
		}
	}
};


class ToggleSortingConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_sorting"; }
	virtual	const char * Get_Help( void ) override	{ return "TOGGLE_SORTING - toggles WW3D sorting."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::Enable_Sorting(!WW3D::Is_Sorting_Enabled());
      Print( WW3D::Is_Sorting_Enabled() ? "Sorting Enabled\n" : "Sorting Disabled\n" );
	}
};

/*
**	Mirror console output to a log monitor watching from another machine.
*/
class RenLogMonConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "rlmon"; }
	virtual	const char * Get_Help( void ) override	{ return "RLMON <a.b.c.d:port> - mirror console output to a log monitor."; }
	virtual	void Activate( const char * input ) override {
		if ( ConsoleModeClass::Set_Log_Monitor( input ) ) {
			Print( "RenLogMon is on, sending to %s\n", input );
		} else {
			Print( "Invalid input to RLMON -- expected a.b.c.d:port\n" );
		}
	}
};

class RenLogMonOffConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "rlmonoff"; }
	virtual	const char * Get_Help( void ) override	{ return "RLMONOFF - stop mirroring console output."; }
	virtual	void Activate( const char * /* input */ ) override {
		ConsoleModeClass::Set_Log_Monitor( nullptr );
		Print( "RenLogMon is off\n" );
	}
};

class ExtrasConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "extras"; }
	virtual	const char * Get_Help( void ) override	{ return "EXTRAS <key>"; }
	virtual	void Activate( const char * /* input */ ) override {

#if 0
		//int crc = CRC_String("quantifigon");//not any more, pw changed...

		//WWDEBUG_SAY(("*E* %d\n", CRC_String(input)));

		//bool onoff = (CRC_String(input) == 0xa8452331);

		CNCPurchaseMainMenuClass::Enable_Secrets(false);  // Just to scramble code
		CNCPurchaseMainMenuClass::Enable_Secrets(false);  // Just to scramble code
		bool onoff = (CRC_String(input) == 65729409);
		CNCPurchaseMainMenuClass::Enable_Secrets(onoff);  // Just to scramble code

		if (The_Game()->IsLaddered.Is_True()) {
			onoff = false;
			CNCPurchaseMainMenuClass::Enable_Secrets(onoff);  // Just to scramble code
			Print("Not allowed in laddered games\r\n");
		}

		CNCPurchaseMainMenuClass::Enable_Secrets(onoff);
		if (CNCPurchaseMainMenuClass::Are_Secrets_Enabled()) {
			Print("extras ENABLED!\r\n");
		}
		CNCPurchaseMainMenuClass::Enable_Secrets(onoff);  // Just to scramble code
#else
		// (gth) secrets don't need a password any more...
		if (The_Game()->IsLaddered.Is_True()) {

			CNCPurchaseMainMenuClass::Enable_Secrets(false);
			Print("Not allowed in laddered games\r\n");

		} else {

			CNCPurchaseMainMenuClass::Enable_Secrets(true);
			if (CNCPurchaseMainMenuClass::Are_Secrets_Enabled()) {
				Print("extras ENABLED!\r\n");
			}
		}
#endif
	}
};

class DebugDeviceConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "debug_device"; }
   const char * Get_Alias( void ) override { return "dd"; }
	virtual	const char * Get_Help( void ) override	{ return
		"DEBUG_DEVICE [device|on|off] - toggles the debug device (screen, mono, dbwin32, log, windows)"; }
	virtual	void Activate( const char * input ) override {
		char str[128];
		sprintf(str,"ERROR (%s)\n", input );
		if (stricmp(input,"screen") == 0) {
			DebugManager::Toggle_Device_Enabled( DebugManager::DEBUG_DEVICE_SCREEN );
			sprintf(str, "Screen Debug %s\n", DebugManager::Is_Device_Enabled( DebugManager::DEBUG_DEVICE_SCREEN ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"mono") == 0) {
			DebugManager::Toggle_Device_Enabled( DebugManager::DEBUG_DEVICE_MONO );
			sprintf(str, "Mono Debug %s\n", DebugManager::Is_Device_Enabled( DebugManager::DEBUG_DEVICE_MONO ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"dbwin32") == 0) {
			DebugManager::Toggle_Device_Enabled( DebugManager::DEBUG_DEVICE_DBWIN32 );
			sprintf(str, "DBWin32 Debug %s\n", DebugManager::Is_Device_Enabled( DebugManager::DEBUG_DEVICE_DBWIN32 ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"log") == 0) {
			DebugManager::Toggle_Device_Enabled( DebugManager::DEBUG_DEVICE_LOG );
			sprintf(str, "Log File Debug %s\n", DebugManager::Is_Device_Enabled( DebugManager::DEBUG_DEVICE_LOG ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"windows") == 0) {
			DebugManager::Toggle_Device_Enabled( DebugManager::DEBUG_DEVICE_WINDOWS );
			sprintf(str, "Windows Debug %s\n", DebugManager::Is_Device_Enabled( DebugManager::DEBUG_DEVICE_WINDOWS ) ? "Enabled" : "Disabled" );
		} else if (stricmp(input,"on") == 0) {
			DebugManager::Enable_Device( (DebugManager::DebugDevice)-1 );
			sprintf(str, "All Debug Devices Enabled\n" );
		} else if (stricmp(input,"off") == 0) {
			DebugManager::Disable_Device( (DebugManager::DebugDevice)-1 );
			sprintf(str, "All Debug Devices Disabled\n" );
		}
		Print(str);
		Debug_Say((str));
	}
};

class AdminMessageConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "admin_message"; }
   const char * Get_Alias( void ) override { return "amsg"; }
	virtual	const char * Get_Help( void ) override	{ return
		"ADMIN_MESSAGE <message> - sends an admin message to all clients. Host only."; }
	virtual	void Activate( const char * input ) override {
		WWASSERT(input != nullptr);
		if (cNetwork::I_Am_Server() && ::strlen(input) > 0) {
			WideStringClass widestring;
			widestring.Convert_From(input);
			cScTextObj * p_message = new cScTextObj;
			p_message->Init(widestring, TEXT_MESSAGE_PUBLIC, true, HOST_TEXT_SENDER, -1);
		} else {
			Print(Get_Help());
		}
	}
};




class MessageConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "message"; }
   const char * Get_Alias( void ) override { return "msg"; }
	virtual	const char * Get_Help( void ) override	{ return "MESSAGE <message> - sends a chat message to all clients. Host only."; }
	virtual	void Activate( const char * input ) override {
		WWASSERT(input != nullptr);
		if (cNetwork::I_Am_Server() && ::strlen(input) > 0) {
			WideStringClass widestring(input, true);
			cScTextObj *event_obj = new cScTextObj;
			event_obj->Init(widestring, TEXT_MESSAGE_PUBLIC, false, HOST_TEXT_SENDER, -1);
		} else {
			Print(Get_Help());
		}
	}
};


#ifndef FREEDEDICATEDSERVER
class FPSConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "fps"; }
   const char * Get_Alias( void ) override { return "fps"; }
	virtual	const char * Get_Help( void ) override	{ return "FPS - toggle FPS display."; }
	virtual	void Activate( const char * ) override {
//		if (!cNetwork::I_Am_Server()) {
         cDevOptions::ShowFps.Toggle();
//		}
	}
};
#endif //FREEDEDICATEDSERVER

#if 1
class ClientPhysicsOptimizationConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "client_physics_optimization"; }
	const char * Get_Alias( void ) override { return "cpo"; }
	virtual	const char * Get_Help( void ) override	{ return "CLIENT_PHYSICS_OPTIMIZATION [0|1] - Update only visible physic objects on client."; }
	virtual	void Activate( const char * input) override {
		if (!cNetwork::I_Am_Server()) {
			int state = 0;
			int argcount=::sscanf(input, "%d", &state);
			if (argcount==1) {
				state=!!state;
			}
			else {
				state=!COMBAT_SCENE->Get_Update_Only_Visible_Objects();
			}
         COMBAT_SCENE->Set_Update_Only_Visible_Objects(state==1);

			Print( "Client physics optimization %s\n", state ? "ENABLED" : "DISABLED");
		}
	}
};
#endif

class StatsConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "stats"; }
	virtual	const char * Get_Help( void ) override	{ return "STATS <off|fps|dx8|star|sr_history|sr|gerd|collision|culling|physics|vehicle|ai|wol|profile|memory> - changes stats display."; }
	virtual	void Activate( const char * input ) override {
		StatisticsDisplayManager::Set_Display( input );
	}
};

class ResolutionConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "resolution"; }
	virtual	const char * Get_Help( void ) override	{ return "RESOLUTION [+|-]"; }
	virtual	void Activate( const char * input ) override {
		int w,h,bits;
		bool windowed;
		WW3D::Get_Device_Resolution(w,h,bits,windowed);
		const RenderDeviceDescClass& desc=WW3D::Get_Render_Device_Desc();
		const DynamicVectorClass<ResolutionDescClass> & resos=desc.Enumerate_Resolutions();
		int i;
		for (i=0;i<resos.Count();++i) {
			if (resos[i].Width==w && resos[i].Height==h && resos[i].BitDepth==bits) {
				break;
			}
		}

		unsigned char sign;
		if (sscanf (input, "%c", &sign) == 1 && (sign=='+' || sign=='-')) {

			if (sign=='+') i++;
			else i--;

			if (i>=resos.Count()) i=0;
			else if (i<0) i=resos.Count()-1;

			WW3DErrorType error=WW3D::Set_Device_Resolution(
				resos[i].Width,
				resos[i].Height,
				resos[i].BitDepth,
				-1,
				true );
			if (error==WW3D_ERROR_OK) {
				RectClass rect;
				rect.Left = 0;
				rect.Top = 0;
				rect.Right = float(resos[i].Width);
				rect.Bottom = float(resos[i].Height);

				Render2DClass::Set_Screen_Resolution(rect);
				Print( "Resolution changed to %d * %d, %d bits\n", resos[i].Width,resos[i].Height,resos[i].BitDepth);
			}
			else {
				Print( "Failed to set resolution to %d * %d, %d bits\n", resos[i].Width,resos[i].Height,resos[i].BitDepth);
				error=WW3D::Set_Device_Resolution(
					w,
					h,
					bits,
					-1,
					true );
			}
		}
		else {
			Print("Resolution is %d * %d, %d bits\n",w,h,bits);
		}
	}
};


class ToggleSurfaceEffectsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "toggle_surface_effects"; }
	virtual	const char * Get_Help( void ) override	{ return "TOGGLE_SURFACE_EFFECTS - toggles surface effects."; }
	virtual	void Activate( const char * /* input */ ) override {
		SurfaceEffectsManager::Enable_Effects(!SurfaceEffectsManager::Are_Effects_Enabled());
      Print( SurfaceEffectsManager::Are_Effects_Enabled() ? "Surface Effects Enabled\n" : "Surface Effects Disabled\n" );
	}
};


//----------------------------------------------------------------------------
//
// SHIPPING CONSOLE FUNCTIONS ONLY
// THESE MUST BE DEBUGGED!  THEY ARE PRESENT IN RELEASE BUILDS!!!
//
//----------------------------------------------------------------------------

class DisplayFindpathConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "display_findpath"; }
	virtual	const char * Get_Help( void ) override	{ return "DISPLAY_FINDPATH - Toggles the display of findpath lines."; }
	virtual	void Activate( const char * /* input */ ) override {
		Toggle_Display_Findpaths();
	}
};


class DonateConsoleFunctionClass : public ConsoleFunctionClass {
	StringClass HelpText;
public:
	virtual const char * Get_Name(void) override {return "donate";}
	virtual const char * Get_Help(void) override {
		HelpText.Format("DONATE <credits> <teammate> - donate money (minimum %d credits) to teammate.",
			cDonateEvent::Get_Minimum_Acceptable_Donation());
		return HelpText.Peek_Buffer();;
	}

	virtual void Activate(const char * input) override {

		int amount = 0;
		char recipient[1000];
		::sscanf(input, "%d %s", &amount, recipient);

		cPlayer * p_me = cNetwork::Get_My_Player_Object();
		cPlayer * p_recipient = cPlayerManager::Find_Player(recipient);

		if (p_me != nullptr &&
			 p_recipient != nullptr &&
			 amount >= cDonateEvent::Get_Minimum_Acceptable_Donation() &&
			(p_me->Get_Player_Type() == p_recipient->Get_Player_Type()) &&
			 p_me->Get_Money() >= amount) {

			cDonateEvent * p_event = new cDonateEvent;
			p_event->Init(amount, p_recipient->Get_Id());

		} else {
		   Print(Get_Help());
		}
	}
};


class DoStuffConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "do_stuff"; }
	virtual	const char * Get_Help( void ) override	{ return "DO_STUFF - Execute some task..."; }
	virtual	void Activate( const char * input ) override {

		//
		// This command is for executing some test... put whatever code
		// you want here...
		//

		/*
		int recipient_id = atoi(input);

		WideStringClass msg;
		msg.Format(U_CHAR("Hola amigo!"));

		if (cNetwork::I_Am_Client()) {
			cCsTextObj * p_message = new cCsTextObj;
			p_message->Init(msg, TEXT_MESSAGE_PRIVATE, cNetwork::Get_My_Id(), recipient_id);
		} else {
			cScTextObj * p_message = new cScTextObj;
			p_message->Init(msg, TEXT_MESSAGE_PRIVATE, HOST_TEXT_SENDER, recipient_id);
		}
		*/

		//::exit(1);

		//::Sleep(::atoi(input));
/*
#if 0
		if (cNetwork::I_Am_Server()) {
			float x, y, z;
			::sscanf(input, "%f, %f, %f", &x, &y, &z);
			Vector3 position;
			position.Set(Vector3(x, y, z));

			cScExplosionEvent * p_event = new cScExplosionEvent;
			p_event->Init(1, position);
		}
#endif
*/

		cPlayer * p_player = cPlayerManager::Find_Player(input);
		if (p_player != nullptr) {
			int id = p_player->Get_Id();
			cNetwork::Server_Kill_Connection(id);
			cNetwork::Cleanup_After_Client(id);
		}
	}
};


//
// REQUIRED for shipping dedicated server.
//
class QuitConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "quit"; }
	virtual	const char * Get_Help( void ) override	{
		if (!SlaveMaster.Am_I_Slave()) {
			return ("QUIT - End game and quit to desktop (dedicated server only).\n");
		} else {
			return ("QUIT - End game and restart slave server.\nTo exit without restarting use 'QUIT_SLAVE' on master server\n");
		}
	}
	virtual	void Activate( const char * /* input */ ) override {

		if (!IS_MISSION) {
			if (AutoRestart.Is_Active()) {
				AutoRestart.Set_Restart_Flag(false);
				Set_Exit_On_Exception(true);
         	cGameData::Set_Manual_Exit(true);
			} else {
				if (cNetwork::I_Am_Server()) {
					if (The_Game() && The_Game()->IsDedicated.Is_True()) {

						if (!SlaveMaster.Am_I_Slave()) {
         				Print("Terminating slaves on demand...\n");
							SlaveMaster.Shutdown_Slaves();
						}

         			Print("Terminating game on demand...\n");
						AutoRestart.Set_Restart_Flag(false);
						Set_Exit_On_Exception(true);
         			cGameData::Set_Manual_Exit(true);
					} else {
         			Print("QUIT is for dedicated server only.\n");
					}
				} else {
					/*
					** I_Am_Server may not report true if we just started up and we aren't a server yet.
					*/
					if (ConsoleBox.Is_Exclusive()) {
						AutoRestart.Set_Restart_Flag(false);
						Set_Exit_On_Exception(true);
         			cGameData::Set_Manual_Exit(true);
					} else {
         			Print("QUIT is for dedicated server only.\n");
					}
				}
			}
      }
	}
};




//
// REQUIRED for shipping dedicated server.
//
class QuitSlaveConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "quit_slave"; }
	virtual	const char * Get_Help( void ) override	{ return "QUIT_SLAVE slavename - Shutdown a slave server (dedicated master server only)."; }
	virtual	void Activate( const char * input ) override {

		if (!IS_MISSION) {
			if (cNetwork::I_Am_Server()) {
				if (The_Game() && The_Game()->IsDedicated.Is_True()) {

					if (!SlaveMaster.Am_I_Slave()) {
         			Print("Terminating slave %s on demand...\n", input);
						if (SlaveMaster.Shutdown_Slave((char*)input)) {
         				Print("Slave %s terminated\n", input);
						} else {
         				Print("Slave %s not found\n", input);
						}
						return;
					}
				}
			}
		}
      Print("QUIT_SLAVE is for dedicated master server only.\n");
	}
};





class NetUpdateRateConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "net_update_rate"; }
	virtual	const char * Get_Alias( void ) override{ return "nur"; }
	virtual	const char * Get_Help( void ) override	{ return "NET_UPDATE_RATE - set the max. net update think rate (times per second)."; }
	virtual	void Activate( const char * input ) override {
      WWASSERT(input != nullptr);
		int rate = ::atoi(input);
#ifdef WWDEBUG
		if (rate > 0) {
#else //WWDEBUG
		if (rate >= 5 && rate <= 30) {
#endif //WWDEBUG
			cUserOptions::NetUpdateRate.Set(rate);
         Print( "NetUpdateRate set to %d.", rate);
		} else {
		   Print(Get_Help());
		}
	}
};




class GameInfoConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "game_info"; }
	virtual	const char * Get_Help( void ) override	{ return "GAME_INFO - Print info about a game in progress to console box"; }
	virtual	void Activate(const char *) override {

		bool active = false;

		if (cNetwork::I_Am_Server()) {

			if (The_Game() && The_Game()->IsDedicated.Is_True()) {

				char upstring[256] = "?";
				char timestr[256] = "?";
				FILETIME creation;
				FILETIME exit;
				FILETIME kernel;
				FILETIME user;
				int ok = GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);
				if (ok) {
					FILETIME local;
					if (FileTimeToLocalFileTime(&creation, &local)) {
						SYSTEMTIME time;
						if (FileTimeToSystemTime(&local, &time)) {
							GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, &time, nullptr, upstring, 256);
							GetTimeFormatA(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &time, nullptr, timestr, 256);
							strcat(upstring, " - ");
							strcat(upstring, timestr);
						}
					}
				}

				GameModeClass* game = GameModeManager::Find("Combat");
				if (game && game->Is_Active()) {
					game = GameModeManager::Find("WOL");
					if (game && game->Is_Active()) {
						ConsoleBox.Print("Westwood Online mode active since %s\n", upstring);
						active = true;
					} else {
						game = GameModeManager::Find("LAN");
						if (game && game->Is_Active()) {
							if (cGameSpyAdmin::Get_Is_Server_Gamespy_Listed()) {
								ConsoleBox.Print("GameSpy mode active since %s\n", upstring);
							} else {
								ConsoleBox.Print("LAN mode active since %s\n", upstring);
							}
							active = true;
						}
					}
				}

				if (active && The_Game()) {

					unsigned int time = unsigned(The_Game()->Get_Time_Remaining_Seconds());
					unsigned int seconds = time % 60;
					unsigned int minutes = (time / 60) % 60;
					unsigned int hours = (time / (60*60));


					if (The_Game()->Is_Gameplay_Permitted()) {
						ConsoleBox.Print("Gameplay in progress\n");
					} else {
						ConsoleBox.Print("Gameplay Pending\n");
					}

					char buf[256];
					sprintf(buf, "     Map : %s\n", The_Game()->Get_Map_Name().Peek_Buffer());
					ConsoleBox.Print(buf);

					sprintf(buf, "    Time : %d.%02d.%02d\n", hours, minutes, seconds);
					ConsoleBox.Print(buf);

					sprintf(buf, "     Fps : %d\n", cNetwork::Get_Fps());
					ConsoleBox.Print(buf);

					sprintf(buf, "     GDI : %d/%d players      %d points\n",
						cPlayerManager::Tally_Team_Size(PLAYERTYPE_GDI),
						The_Game()->Get_Max_Players(),
						(int)cTeamManager::Find_Team(PLAYERTYPE_GDI)->Get_Score());
					ConsoleBox.Print(buf);

					sprintf(buf, "     NOD : %d/%d players      %d points\n",
						cPlayerManager::Tally_Team_Size(PLAYERTYPE_NOD),
						The_Game()->Get_Max_Players(),
						(int)cTeamManager::Find_Team(PLAYERTYPE_NOD)->Get_Score());
					ConsoleBox.Print(buf);

					ConsoleBox.Print("\n");
				}
			}
		}
	}
};








class PlayerInfoConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "player_info"; }
	virtual	const char * Get_Help( void ) override	{ return "PLAYER_INFO - Print info about players in the game to the console box"; }
	virtual	void Activate(const char *) override {

		if (cNetwork::I_Am_Server() && The_Game() && The_Game()->IsDedicated.Is_True() && cNetwork::PServerConnection != nullptr) {

			unsigned int time = TIMEGETTIME();
			unsigned int bw = 0;

			if (cNetwork::PServerConnection->Get_Num_RHosts() == 0) {
				ConsoleBox.Print("No players\n");
			} else {

				ConsoleBox.Print("\rId  Name           Score Side\tPing\tAddress               Kbits/s Time\n\n");

				for (int i=cNetwork::PServerConnection->Get_Min_RHost() ; i <= cNetwork::PServerConnection->Get_Max_RHost() ; i++) {

					cRemoteHost *client = cNetwork::PServerConnection->Get_Remote_Host(i);

					if (client) {
      				cPlayer * playerptr = cPlayerManager::Find_Player(i);

						if (playerptr) {
							WideStringClass name(playerptr->Get_Name(), true);
							StringClass short_name;
							name.Convert_To(short_name);
							int len = name.Get_Length();
							char temp_name[256];
							strcpy(temp_name, short_name.Peek_Buffer());
							strncat(temp_name, "                    ", 14-len);

							unsigned int dur = (time - client->Get_Creation_Time()) / 1000;

							unsigned int seconds = dur % 60;
							unsigned int minutes = (dur / 60) % 60;
							unsigned int hours = (dur / (60*60));

							bw += PacketManager.Get_Compressed_Bandwidth_Out(&client->Get_Address());

							char addr_string[128];
							sockaddr_in *addr = &client->Get_Address();
							sprintf(addr_string, "%d.%d.%d.%d;%d", 	(int)(addr->sin_addr.S_un.S_un_b.s_b1),
																					(int)(addr->sin_addr.S_un.S_un_b.s_b2),
																					(int)(addr->sin_addr.S_un.S_un_b.s_b3),
																					(int)(addr->sin_addr.S_un.S_un_b.s_b4),
																					static_cast<unsigned int>(ntohs(addr->sin_port)));
							const size_t addr_string_len = ::strlen(addr_string);
							char local_addr_string[128];
							strcpy(local_addr_string, addr_string);
							if (addr_string_len < 21) {
								const size_t padding = 21 - addr_string_len;
								strncat(local_addr_string, "                         ", padding);
							}

							Vector3 color = playerptr->Get_Color();
							char buffer[256];
							sprintf(buffer, "%3d %s%- 6d %s\t%d\t%s%- 7d  %03d.%02d.%02d\n", i, temp_name,
																								(int)playerptr->Get_Score(),
																								(playerptr->Get_Player_Type() == PLAYERTYPE_NOD) ? "NOD" : "GDI",
																								(int)client->Get_Average_Internal_Pingtime_Ms(),
																								local_addr_string,
																								(int)(PacketManager.Get_Compressed_Bandwidth_Out(&client->Get_Address()) / 1024),
																								hours, minutes, seconds);

							WideStringClass ws(buffer, true);
							ConsoleBox.Add_Message(&ws, &color, true);
						}
					}
				}
				ConsoleBox.Print("Total current bandwidth usage for players is %d kilobits per second\n", bw / 1024);
			}
   	} else {
			Print("player_info is for dedicated server only\n");
		}
	}
};

class KickConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "kick"; }
	virtual	const char * Get_Help( void ) override	{ return "KICK [<Nickname>|<Id>] - Kick a user from the game."; }
	bool KickWOLUser(const WideStringClass & user_name) {

		GameModeClass* game = GameModeManager::Find("WOL");

		if (game && game->Is_Active()) {

			if (user_name.Get_Length() > 0) {
				WolGameModeClass* wolgame = reinterpret_cast<WolGameModeClass*>(game);

				if (wolgame->Kick_Player(user_name)) {
					StringClass smallstring(true);
					user_name.Convert_To(smallstring);
					ConsoleBox.Print("%s was kicked\n", smallstring.Peek_Buffer());
					return true;
				}
			}
		}
		return false;
	}
	bool KickWOLUser(int id) {
		cPlayer *player = cPlayerManager::Find_Player(id);
		if (player) {
			return KickWOLUser(player->Get_Name());
		}
		return false;
	}
	bool KickGameSpyUser(int id) {
		if (cGameSpyAdmin::Is_Gamespy_Game()) {
			return GameSpyBanList.Kick_Player(id);
		}
		return false;
	}
	bool KickGameSpyUser(const WideStringClass & user_name) {
		if (cGameSpyAdmin::Is_Gamespy_Game()) {
			cPlayer *player = nullptr;

			for (SLNode<cPlayer> *player_node = cPlayerManager::Get_Player_Object_List ()->Head ()
				; player_node != nullptr; player_node = player_node->Next ()) {

				player = player_node->Data ();
				WWASSERT (player != nullptr);

				if (player->Get_Is_Active().Is_False() || !player->Is_Human()) {
					continue;
				}
				if (stricmp(StringClass(user_name), StringClass(player->Get_Name())) == 0) {
					KickGameSpyUser(player->Get_Id());
					return true;
				}
			}
		}
		return false;
	}
	virtual	void Activate( const char * input ) override {

		if (cNetwork::I_Am_Server()) {
			if (!input || !(*input)) return;
			bool kicked = false;
			WideStringClass user_name(input, true);

			kicked = KickWOLUser(user_name);
			if (!kicked) kicked = KickGameSpyUser(user_name);

			if (!kicked) {
				// try to kick by Id number
				// make sure this is a decimal number since atoi returns 0 on error
				// and we don't want to kick player 0 unless they really mean to
				const char *t = input;
				if (!(*t)) t = nullptr;
				while (t && *t) {
					if (!isdigit(*t)) t = nullptr;
					else t++;
				}
				if (t) {
					kicked = KickWOLUser(atoi(input));
					if (!kicked) kicked = KickGameSpyUser(atoi(input));
				}
			}

			if (!kicked) {
				ConsoleBox.Print("%s not found\n", input);
			}
		}
	}
};

class BanConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "ban"; }
	virtual	const char * Get_Help( void ) override	{ return "BAN [<Nickname>|<Id>] - Permanently ban a user from this server(GameSpy & WOL mode only)."; }
	virtual	void Activate( const char * input ) override {

		GameModeClass* wolgame = GameModeManager::Find("WOL");
		bool is_wol = false;
		if (wolgame && wolgame->Is_Active()) {
			is_wol = true;
		}
		//if (cGameSpyAdmin::Is_Gamespy_Game() && cNetwork::I_Am_Server()) {
		if ((cGameSpyAdmin::Is_Gamespy_Game() || is_wol) && cNetwork::I_Am_Server()) {

			if (!input || !(*input)) return;
			cPlayer *player = nullptr;

			SLNode<cPlayer> *player_node;
			for (player_node = cPlayerManager::Get_Player_Object_List ()->Head ()
				; player_node != nullptr; player_node = player_node->Next ()) {

				player = player_node->Data ();
				WWASSERT (player != nullptr);

				if (player->Get_Is_Active().Is_False() || !player->Is_Human()) {
					continue;
				}
				if (stricmp(StringClass(input), StringClass(player->Get_Name())) == 0) {
					if (is_wol) {
						((WolGameModeClass*)wolgame)->Ban_Player(player->Get_Name(), player->Get_Ip_Address());
					} else {
						if (player->Get_GameSpy_Hash_Id().Is_Empty()) {
							GameSpyBanList.Ban_User(input);
						} else {
							GameSpyBanList.Ban_User(input, player->Get_GameSpy_Hash_Id());
						}
					}
					break;
				}
			}
			if (player_node == nullptr) {
				// make sure this is a decimal number since atoi returns 0 on error
				// and we don't want to kick player 0 unless they really mean to
				const char *t = input;
				if (!(*t)) t = nullptr;
				while (t && *t) {
					if (!isdigit(*t)) t = nullptr;
					else t++;
				}
				if (t) {
					player = cPlayerManager::Find_Player(atoi(input));
					if (player) {
						if (is_wol) {
							((WolGameModeClass*)wolgame)->Ban_Player(player->Get_Name(), player->Get_Ip_Address());
						} else {
							if (player->Get_GameSpy_Hash_Id().Is_Empty() && !player->Get_Name().Is_Empty()) {
								GameSpyBanList.Ban_User(StringClass(player->Get_Name()));
							} else if (!player->Get_GameSpy_Hash_Id().Is_Empty()) {
								GameSpyBanList.Ban_User(StringClass(player->Get_Name()), player->Get_GameSpy_Hash_Id());
							}
						}
					}
				} else {
					ConsoleBox.Print("%s not found\n", input);
				}
			}
		}
	}
};


class AllowConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "allow"; }
	virtual	const char * Get_Help( void ) override	{ return "ALLOW [<Nickname>|<Id>] - Remove user channel ban from this server (WOL mode only)."; }
	virtual	void Activate( const char * input ) override {

		GameModeClass* game = GameModeManager::Find("WOL");

		if (!input || !(*input)) return;

		if (game && game->Is_Active()) {

			WideStringClass wname(input, true);

			if (wname.Get_Length() > 0 && wname.Get_Length() <15) {
				RefPtr<WWOnline::Session> wol_session = WWOnline::Session::GetInstance(false);
				if (wol_session.IsValid()) {
					wol_session->BanUser(wname, false);
				}
			}
		}
	}
};


class PageConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "page"; }
	virtual	const char * Get_Help( void ) override	{ return "PAGE Nickname Message - Page a Westwood Online user."; }
	virtual	void Activate( const char * input ) override {

		static unsigned int last_page = 0;

		GameModeClass* wolgame = GameModeManager::Find("WOL");
		bool is_wol = false;
		if (wolgame && wolgame->Is_Active()) {
			is_wol = true;
		}

		if (is_wol) {
			RefPtr<WWOnline::Session> wol_session = WWOnline::Session::GetInstance(false);
			if (wol_session.IsValid()) {

				unsigned int time = TIMEGETTIME();
				if (time < last_page || time - last_page > 1000) {
					last_page = time;

					char temp[1024];
					strncpy(temp, input, 1023);
					char *name = temp;
					char *message = strchr(temp, ' ');
					if (message) {
						*message++ = 0;

						if (strlen(name) && strlen(message)) {
							WideStringClass wname(name, true);
							WideStringClass wmessage(message, true);
							wol_session->PageUser(wname, wmessage);
						}
					}
				}
			}
		}
	}
};





//
// REQUIRED for shipping dedicated server.
//
class RestartConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "restart"; }
	virtual	const char * Get_Help( void ) override	{ return "RESTART - Quit to desktop and restart process (dedicated server only)."; }
	virtual	void Activate( const char * /* input */ ) override {

		if (!IS_MISSION) {
			if (AutoRestart.Is_Active()) {
				AutoRestart.Set_Restart_Flag(true);
				Set_Exit_On_Exception(true);
         	cGameData::Set_Manual_Exit(true);
			} else {
				if (cNetwork::I_Am_Server()) {
					if (The_Game() && The_Game()->IsDedicated.Is_True()) {

						if (SlaveMaster.Am_I_Slave() == false) {
         				Print("Terminating slaves on demand...\n");
							SlaveMaster.Shutdown_Slaves();
						}

         			Print("Restarting game on demand...\n");
						AutoRestart.Set_Restart_Flag(true);
						Set_Exit_On_Exception(true);
         			cGameData::Set_Manual_Exit(true);
					} else {
         			Print("RESTART is for dedicated server only.\n");
					}
				} else {
         		Print("RESTART is for dedicated server only.\n");
				}
			}
      }
	}
};




class LogMeshStatsConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "log_mesh_stats"; }
	virtual	const char * Get_Help( void ) override	{ return "LOG_MESH_STATS - log stats to debug window or file."; }
	virtual	void Activate( const char * /* input */ ) override {
		TheDX8MeshRenderer.Request_Log_Statistics();
	}
};

class LogTexturesConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	virtual	const char * Get_Name( void ) override	{ return "log_textures"; }
	virtual	const char * Get_Help( void ) override	{ return "LOG_TEXTURES - log texture statistics to debug window or file."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3DAssetManager::Get_Instance()->Log_All_Textures();
	}
};

class DeviceInfoConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "device_info"; }
	virtual	const char * Get_Help( void ) override	{ return "DEVICE_INFO - get device driver info."; }
	virtual	void Activate( const char * /* input */ ) override {
		const RenderDeviceDescClass &	rddesc = WW3D::Get_Render_Device_Desc();
		Print( "Render Device: %s\n", rddesc.Get_Device_Name() );
		Print( "Graphics Hardware: %s\n",rddesc.Get_Hardware_Name() );
	}
};


//
// TEMP DEBUG CODE
//
class NewTCADOConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "newtcado"; }
	virtual	const char * Get_Help( void ) override	{ return "newtcado - Toggle new TCADO code"; }
	virtual	void Activate( const char * /* input */ ) override {
		bool is_new_tcado = cDevOptions::UseNewTCADO.Toggle();
      Print(is_new_tcado ? "Using new TCADO.\n" : "Using old TCADO.\n" );
	}
};


class TimeOfDayConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("time_of_day");}
	virtual	const char * Get_Help (void) override	{return ("TIME_OF_DAY <hours [0..23]> <minutes [0..59]> - sets the time of day for the background.");}
	virtual	void Activate (const char *input) override {

		if (COMBAT_SCENE != nullptr) {

			unsigned hours, minutes;

			if (sscanf (input, "%d %d", &hours, &minutes) == 2) {
				if (BackgroundMgrClass::Set_Time_Of_Day (hours, minutes)) {

					const float theta = (((hours * 60.0f) + minutes) / 1440.0f) * 2.0f * WWMATH_PI;

					BackgroundMgrClass::Set_Light_Source_Type (BackgroundMgrClass::LIGHT_SOURCE_TYPE_SUN);
					COMBAT_SCENE->Set_Sun_Light_Orientation (0.0f, theta + (WWMATH_PI * 1.5f));
					Print ("Time of day changed\n");
				}
			}
		}
	}
};


class CloudsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("clouds");}
	virtual	const char * Get_Help (void) override	{return ("CLOUDS <cloudcover [0..1]> <gloominess [0..1]> [ramptime [0+]]");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Clouds changed\n";

		int	argcount;
		float cloudcover, gloominess, ramptime;
		bool	success = false;

		argcount = sscanf (input, "%f %f %f", &cloudcover, &gloominess, &ramptime);
		switch (argcount) {

			case 2:
				success = BackgroundMgrClass::Set_Clouds (cloudcover, gloominess);
				break;

			case 3:
				success = BackgroundMgrClass::Set_Clouds (cloudcover, gloominess, ramptime);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class CopyLogsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("copy_logs");}
	virtual	const char * Get_Help (void) override	{return ("COPY_LOGS <on/off> - enable/disable error logging to network.");}
	virtual	void Activate (const char *input) override {

		if (!::stricmp(input, "on")) {

			RegistryClass registry(APPLICATION_SUB_KEY_NAME_DEBUG);
			WWASSERT(registry.Is_Valid());
			registry.Set_Int("SystemInfoLogDisable", false);
			registry.Set_Int("DisableLogCopying", false);

			Print("Log copying is ON.");

		} else if (!::stricmp(input, "off")) {

			RegistryClass registry(APPLICATION_SUB_KEY_NAME_DEBUG);
			WWASSERT(registry.Is_Valid());
			registry.Set_Int("SystemInfoLogDisable", true);
			registry.Set_Int("DisableLogCopying", true);

			Print("Log copying is OFF.");

		} else {

			Print(Get_Help());
		}
	}
};


class SkyTintConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("sky_tint");}
	virtual	const char * Get_Help (void) override	{return ("SKY_TINT <tintfactor [0..1]> [ramptime [0+]]");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Sky tint changed\n";

		int	argcount;
		float skytintfactor, ramptime;
		bool	success = false;

		argcount = sscanf (input, "%f %f", &skytintfactor, &ramptime);
		switch (argcount) {

			case 1:
				success = BackgroundMgrClass::Set_Sky_Tint (skytintfactor);
				break;

			case 2:
				success = BackgroundMgrClass::Set_Sky_Tint (skytintfactor, ramptime);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class InfoDebugToggle : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "info_debug_toggle"; }
	virtual	const char * Get_Help( void ) override	{ return "INFO_DEBUG_TOGGLE - Toggles display of debug info on object info."; }
	virtual	void Activate( const char * /* input */ ) override {
		extern bool	InfoDebug;
		InfoDebug = !InfoDebug;
		Print( "Toggled Info Debug\n" );
	}
};

class MeshRenderSnapshotFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "mesh_render_snapshot"; }
	virtual	const char * Get_Help( void ) override	{ return "MESH_RENDER_SNAPSHOT - Logs one frame of mesh render system."; }
	virtual	void Activate( const char * /* input */ ) override {
		WW3D::Activate_Snapshot(true);
	}
};

class UmbraToggleFunctionClass : public ConsoleFunctionClass {
public:
	virtual  const char * Get_Name(void) override { return "umbra_toggle"; }
	virtual	const char * Get_Help(void) override { return "UMBRA_TOGGLE - toggles umbra culling (if compiled into this build)."; }
	virtual  void Activate(const char * /* input */) override {
#if (UMBRASUPPORT)
		UmbraSupport::Enable_Umbra(!UmbraSupport::Is_Umbra_Enabled());
		Print("Umbra %s\n",(UmbraSupport::Is_Umbra_Enabled() ? "enabled." : "disabled."));
#else
		Print("Umbra support not compiled into this build.\n");
#endif

	}
};

class UnusedSpawnerFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "unused_spawners"; }
	virtual	const char * Get_Help( void ) override	{ return "UNUSED_SPAWNERS - Displays the ID of spawns which have spawned nothing."; }
	virtual	void Activate( const char * /* input */ ) override {
		SpawnManager::Display_Unused_Spawners();
	}
};

class VipConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "vip"; }
	virtual	const char * Get_Help( void ) override	{ return "VIP <password> - Toggle VIP mode. Client only.\n"; }

	virtual	void Activate( const char * input ) override {

		StringClass password;
		password.Format("%s", input);

      if (cNetwork::I_Am_Client()) {// && !password.Is_Empty()) {

			cVipModeEvent * p_event = new cVipModeEvent;
			p_event->Init(password);

		} else {

		   Print(Get_Help());
		}
	}
};

class AllowKillingHibernatingSpawnFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "allow_killing_hibernating_spawn"; }
	virtual	const char * Get_Help( void ) override	{ return "ALLOW_KILLING_HIBERNATING_SPAWN - Toggles the global ability for spawners to kill hibernating spawn."; }
	virtual	void Activate( const char * /* input */ ) override {
		if (	SpawnManager::Toggle_Allow_Killing_Hibernating_Spawn() ) {
			Print("Killing Hibernating Spawn Allowed\n");
		} else {
			Print("Killing Hibernating Spawn Disallowed\n");
		}
	}
};

class DisplayHibernatingFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "display_hibernating"; }
	virtual	const char * Get_Help( void ) override	{ return "DISPLAY_HIBERNATING - Toggles displaying of objects begining and ending hibernation."; }
	virtual	void Activate( const char * /* input */ ) override {
extern bool	_DisplayHibernating;
		_DisplayHibernating = !_DisplayHibernating;
		Print("Display Hibernating now %s\n", _DisplayHibernating ? "ON" : "OFF" );
	}
};

class HideHUDPointsFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "hide_hud_points"; }
	virtual	const char * Get_Help( void ) override	{ return "HIDE_HUD_POINTS - Toggles displaying of points on the HUD."; }
	virtual	void Activate( const char * /* input */ ) override {
		Print("HUD points toggled\n" );
		HUDClass::Toggle_Hide_Points();
	}
};

class SnapShotModeToggleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "snap_shot_mode_toggle"; }
	virtual	const char * Get_Alias( void ) override{ return "ssm"; }
	virtual	const char * Get_Help( void ) override	{ return "SNAP_SHOT_MODE_TOGGLE - Toggles Snap Shot Mode."; }
	virtual	void Activate( const char * /* input */ ) override {
		if ( COMBAT_CAMERA ) {
			COMBAT_CAMERA->Set_Snap_Shot_Mode( !COMBAT_CAMERA->Is_Snap_Shot_Mode() );
			Print( "Snap Shot Mode %s\n", COMBAT_CAMERA->Is_Snap_Shot_Mode() ? "ON" : "OFF" );
		}
	}
};


class LightningConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("lightning");}
	virtual	const char * Get_Help (void) override	{return ("LIGHTNING <intensity [0..1]> [<start [0..1]> <end [0..1]> <heading [0..360]> <distribution [0..1]>] [ramptime [0+]] - Sets lightning.");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Lightning changed\n";

		int	argcount;
		float arg0, arg1, arg2, arg3, arg4, arg5;
		bool  success = false;

		argcount = sscanf (input, "%f %f %f %f %f %f", &arg0, &arg1, &arg2, &arg3, &arg4, &arg5);
		switch (argcount) {

			case 1:
				success = BackgroundMgrClass::Set_Lightning_Intensity (arg0);
				break;

			case 5:
				success = BackgroundMgrClass::Set_Lightning (arg0, arg1, arg2, arg3, arg4);
				break;

			case 6:
				success = BackgroundMgrClass::Set_Lightning (arg0, arg1, arg2, arg3, arg4, arg5);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class WarBlitzConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("warblitz");}
	virtual	const char * Get_Help (void) override	{return ("WARBLITZ <intensity [0..1]> [<start [0..1]> <end [0..1]> <heading [0..360]> <distribution [0..1]>] [ramptime [0+]] - Sets war blitz.");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "War blitz changed\n";

		int	argcount;
		float arg0, arg1, arg2, arg3, arg4, arg5;
		bool  success = false;

		argcount = sscanf (input, "%f %f %f %f %f %f", &arg0, &arg1, &arg2, &arg3, &arg4, &arg5);
		switch (argcount) {

			case 1:
				success = BackgroundMgrClass::Set_War_Blitz (arg0);
				break;

			case 5:
				success = BackgroundMgrClass::Set_War_Blitz (arg0, arg1, arg2, arg3, arg4);
				break;

			case 6:
				success = BackgroundMgrClass::Set_War_Blitz (arg0, arg1, arg2, arg3, arg4, arg5);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class WarpConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("warp");}
	virtual	const char * Get_Help (void) override	{return ("WARP <player name> - warp to first other player or named player (client only).");}
	virtual	void Activate (const char *input) override {
      if (cNetwork::I_Am_Client()) {

			WideStringClass player_name;
			player_name.Convert_From(input);

			cWarpEvent * p_warp = new cWarpEvent;
			p_warp->Init(player_name);

		} else {
		   Print(Get_Help());
		}
	}
};


class WhoIsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("whois");}
	virtual	const char * Get_Help (void) override	{return ("WHOIS <player name> - who the heck is this annoying bugger? (server only)\n");}
	virtual	void Activate (const char *input) override {
		cPlayer * p_player = cPlayerManager::Find_Player(input);

		if (!cNetwork::I_Am_Server() || p_player == nullptr) {
		   Print(Get_Help());
		} else {

			ULONG ip = p_player->Get_Ip_Address();
			HOSTENT * p_host = ::gethostbyaddr((char *) &ip, sizeof(ip), AF_INET);
			char resolved_ip[100] = "";
			if (p_host != nullptr) {
				::sprintf(resolved_ip, "(%s)", p_host->h_name);
			}

			Print("%s is at ip %s %s\n",
				input, cNetUtil::Address_To_String(ip), resolved_ip);
		}
	}
};


class WindConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("wind");}
	virtual	const char * Get_Help (void) override	{return ("WIND <heading [0..360]> <speed [0+]> <variability [0..1]> [ramptime [0+]] - Sets wind.");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Wind changed\n";

		int	argcount;
		float heading, speed, variability, ramptime;
		bool  success = false;

		argcount = sscanf (input, "%f %f %f %f", &heading, &speed, &variability, &ramptime);
		switch (argcount) {

			case 3:
				success = WeatherMgrClass::Set_Wind (heading, speed, variability);
				break;

			case 4:
				success = WeatherMgrClass::Set_Wind (heading, speed, variability, ramptime);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class RainConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("rain");}
	virtual	const char * Get_Help (void) override	{return ("RAIN <density [0+]> [ramptime [0+]] - Sets rain.");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Rain changed\n";

		int	argcount;
		float	arg0, arg1;
		bool	success = false;

		argcount = sscanf (input, "%f %f", &arg0, &arg1);
		switch (argcount) {

			case 1:
				success = WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_RAIN, arg0);
				break;

			case 2:
				success = WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_RAIN, arg0, arg1);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class SnowConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("snow");}
	virtual	const char * Get_Help (void) override	{return ("SNOW <density [0+]> [ramptime [0+]] - Sets snow.");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Snow changed\n";

		int	argcount;
		float	arg0, arg1;
		bool	success = false;

		argcount = sscanf (input, "%f %f", &arg0, &arg1);
		switch (argcount) {

			case 1:
				success = WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_SNOW, arg0);
				break;

			case 2:
				success = WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_SNOW, arg0, arg1);
				break;

 			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};


class AppPacketTypesResetConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name( void ) override	{ return "app_packet_types_reset"; }
	virtual	const char * Get_Alias( void ) override{ return "aptr"; }
	virtual	const char * Get_Help( void ) override	{ return "APP_PACKET_TYPES_RESET - reset app packet stats."; }
	virtual	void Activate( const char * /* input */ ) override {
		cAppPacketStats::Reset();
	}
};


/*
**	Roadmap Section 36.  The acceptance is that weather covers a large scene at a bounded
**	particle count, so the command that reports it prints the bound, what was drawn against it,
**	and the two numbers that say why: how far apart the fall lines ended up, and how much of the
**	ground under them the renderer has actually looked at.
*/
class WeatherStatusConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("weather_status");}
	virtual	const char * Get_Help (void) override	{return ("WEATHER_STATUS - what the weather layer drew, and what it cost.");}
	virtual	void Activate (const char * /* input */) override {

		Print ("weather_status: layer is %s, budget %d particle(s), %d cast(s) per frame.\n",
				 WeatherEnvironmentRenderer::Is_Enabled() ? "on" : "off",
				 WeatherEnvironmentRenderer::Get_Max_Particles(),
				 WeatherEnvironmentRenderer::Get_Max_Casts_Per_Frame());

		int total = 0;

		for (int p = WeatherMgrClass::PRECIPITATION_FIRST; p < WeatherMgrClass::PRECIPITATION_COUNT; p++) {

			WeatherSystemClass *system = WeatherMgrClass::Peek_Precipitation ((WeatherMgrClass::PrecipitationEnum)p);
			if (system == nullptr) continue;

			WeatherEnvironmentRenderer &renderer = system->Peek_Renderer();

			total += renderer.Get_Drawn_Particles();

			Print ("weather_status: %s -- %d particle(s) on %d column(s), %.2f apart.\n",
					 WeatherEnvironmentRenderer::Get_Mode_Name (renderer.Get_Mode()),
					 renderer.Get_Drawn_Particles(),
					 renderer.Get_Column_Count(),
					 renderer.Get_Column_Spacing());

			Print ("weather_status: %s -- %d cast(s), %d cache hit(s), %d column(s) deferred.\n",
					 WeatherEnvironmentRenderer::Get_Mode_Name (renderer.Get_Mode()),
					 renderer.Get_Casts(),
					 renderer.Get_Cache_Hits(),
					 renderer.Get_Deferred_Columns());
		}

		if (total == 0) {
			Print ("weather_status: nothing is falling.\n");
		} else {
			Print ("weather_status: %d of %d particle(s) used this frame.\n",
					 total, WeatherEnvironmentRenderer::Get_Max_Particles());
		}
	}
};


/*
**	The bound itself.  Weather spends it in mode order, so lowering it coarsens the lattice of
**	whatever is falling rather than cutting any one kind of weather off.
*/
class WeatherBudgetConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("weather_budget");}
	virtual	const char * Get_Help (void) override	{return ("WEATHER_BUDGET [particles] [casts] - how many weather particles a frame may hold.");}
	virtual	void Activate (const char *input) override {

		int	particles = 0, casts = 0;
		int	argcount = (input != nullptr) ? sscanf (input, "%d %d", &particles, &casts) : 0;

		if (argcount >= 1 && particles >= 0) {
			WeatherEnvironmentRenderer::Set_Max_Particles (particles);
		}
		if (argcount >= 2 && casts >= 0) {
			WeatherEnvironmentRenderer::Set_Max_Casts_Per_Frame (casts);
		}

		Print ("weather_budget: %d particle(s), %d cast(s) per frame.\n",
				 WeatherEnvironmentRenderer::Get_Max_Particles(),
				 WeatherEnvironmentRenderer::Get_Max_Casts_Per_Frame());
	}
};


/*
**	Off means no weather is evaluated at all, whatever the level or a script asked for.  It is
**	how you find out whether something you are looking at is the weather's fault.
*/
class WeatherLayerConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("weather_layer");}
	virtual	const char * Get_Help (void) override	{return ("WEATHER_LAYER [on|off] - draw the weather layer, or do not.");}
	virtual	void Activate (const char *input) override {

		if (input != nullptr && ::strstr (input, "off") != nullptr) {
			WeatherEnvironmentRenderer::Set_Enabled (false);
		} else if (input != nullptr && ::strstr (input, "on") != nullptr) {
			WeatherEnvironmentRenderer::Set_Enabled (true);
		}

		Print ("weather_layer: %s.\n", WeatherEnvironmentRenderer::Is_Enabled() ? "on" : "off");
	}
};


/*
**	Roadmap Section 27.  A tracer is arithmetic on a fixed pool, not a scene object, so the
**	status command reports the pool the way weather_status reports the particle lattice: the
**	bound, what is alive against it, and how many shots the bound has turned away.
*/
class TracerStatusConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("tracer_status");}
	virtual	const char * Get_Help (void) override	{return ("TRACER_STATUS - what the tracer pool holds, and what it cost.");}
	virtual	void Activate (const char * /* input */) override {

		Print ("tracer_status: layer is %s, %d of %d tracer(s) alive, %d drawn last frame, %d dropped.\n",
				 TracerRenderer::Is_Enabled() ? "on" : "off",
				 TracerRenderer::Get_Live_Count(),
				 TracerRenderer::Get_Max_Tracers(),
				 TracerRenderer::Get_Drawn_Particles(),
				 TracerRenderer::Get_Dropped_Count());
	}
};


/*
**	The ceiling itself.  A shot arriving over it is left undrawn rather than displacing one
**	already on screen -- see Code/ww3d2/tracerrenderer.h for why that is the chosen direction
**	of failure.
*/
class TracerBudgetConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("tracer_budget");}
	virtual	const char * Get_Help (void) override	{return ("TRACER_BUDGET [count] - how many tracers may be alive at once.");}
	virtual	void Activate (const char *input) override {

		int	count = 0;
		int	argcount = (input != nullptr) ? sscanf (input, "%d", &count) : 0;

		if (argcount >= 1 && count >= 0) {
			TracerRenderer::Set_Max_Tracers (count);
		}

		Print ("tracer_budget: %d tracer(s).\n", TracerRenderer::Get_Max_Tracers());
	}
};


/*
**	Off means no shot gets a tracer at all, whatever DisplayLaser says.  There is no per-shot
**	object left to fall back to, so this is the only sense in which the layer can be absent.
*/
class TracerLayerConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("tracer_layer");}
	virtual	const char * Get_Help (void) override	{return ("TRACER_LAYER [on|off] - draw ballistic tracers, or do not.");}
	virtual	void Activate (const char *input) override {

		if (input != nullptr && ::strstr (input, "off") != nullptr) {
			TracerRenderer::Set_Enabled (false);
		} else if (input != nullptr && ::strstr (input, "on") != nullptr) {
			TracerRenderer::Set_Enabled (true);
		}

		Print ("tracer_layer: %s.\n", TracerRenderer::Is_Enabled() ? "on" : "off");
	}
};


/*
**	Roadmap Section 28.  Two pools behind one status line: the visual chips
**	(DebrisRenderer, ww3d2) and the real-collision fragments (DebrisManager's own gameplay
**	pool, Code/Combat/debrissystem.h) -- see docs/zerohour/DebrisRenderModule.md for why they
**	are separate facilities.
*/
class DebrisStatusConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("debris_status");}
	virtual	const char * Get_Help (void) override	{return ("DEBRIS_STATUS - what both debris pools hold, and what they cost.");}
	virtual	void Activate (const char * /* input */) override {

		Print ("debris_status: layer is %s, visual %d/%d alive (%d drawn last frame, %d dropped), "
				 "gameplay %d/%d alive (%d dropped).\n",
				 DebrisManager::Is_Enabled() ? "on" : "off",
				 DebrisRenderer::Get_Live_Count(),
				 DebrisRenderer::Get_Max_Debris(),
				 DebrisRenderer::Get_Drawn_Particles(),
				 DebrisRenderer::Get_Dropped_Count(),
				 DebrisManager::Get_Live_Gameplay_Count(),
				 DebrisManager::Get_Max_Gameplay(),
				 DebrisManager::Get_Dropped_Gameplay_Count());
	}
};


/*
**	The visual pool's ceiling only -- the gameplay pool is small enough (see
**	GAMEPLAY_DEBRIS_MAX in debrissystem.cpp) that tuning it from the console has not come up.
*/
class DebrisBudgetConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("debris_budget");}
	virtual	const char * Get_Help (void) override	{return ("DEBRIS_BUDGET [count] - how many visual debris chips may be alive at once.");}
	virtual	void Activate (const char *input) override {

		int	count = 0;
		int	argcount = (input != nullptr) ? sscanf (input, "%d", &count) : 0;

		if (argcount >= 1 && count >= 0) {
			DebrisRenderer::Set_Max_Debris (count);
		}

		Print ("debris_budget: %d chip(s).\n", DebrisRenderer::Get_Max_Debris());
	}
};


/*
**	Off means a destroyed building or bridge span scatters nothing at all, in either tier --
**	there is no per-fragment object left to fall back to, so this is the only sense in which
**	the debris layer can be absent.
*/
class DebrisLayerConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("debris_layer");}
	virtual	const char * Get_Help (void) override	{return ("DEBRIS_LAYER [on|off] - scatter destruction debris, or do not.");}
	virtual	void Activate (const char *input) override {

		if (input != nullptr && ::strstr (input, "off") != nullptr) {
			DebrisManager::Set_Enabled (false);
		} else if (input != nullptr && ::strstr (input, "on") != nullptr) {
			DebrisManager::Set_Enabled (true);
		}

		Print ("debris_layer: %s.\n", DebrisManager::Is_Enabled() ? "on" : "off");
	}
};


/*
**	V2 roadmap Section 34 Phase 18 step 16 -- the operation debug/replay command.
**
**	One command rather than a family, because everything it does is one question about one object:
**	what does the journal hold, and does replaying it reproduce the ground it produced?  The
**	`replay` verb is the whole point -- it prints the terrain checksum before and after replaying
**	every journalled operation from the base snapshot, which is the phase's own "save/replay
**	produces same checksum" acceptance line, made runnable by a person.
*/
class WorldOperationConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("worldop");}
	virtual	const char * Get_Help (void) override	{
		return ("WORLDOP <status|begin|replay|crater X Y RADIUS DEPTH|restore X Y RADIUS> - world operation journal.");
	}
	virtual	void Activate (const char *input) override {

		char verb[32];
		verb[0] = '\0';
		float arg0 = 0.0f, arg1 = 0.0f, arg2 = 0.0f, arg3 = 0.0f;
		int argcount = sscanf (input,"%31s %f %f %f %f",verb,&arg0,&arg1,&arg2,&arg3);

		if (argcount < 1) {
			Print ("%s\n",Get_Help ());
			return;
		}

		if (::stricmp (verb,"status") == 0) {
			Print ("world operation journal: %s, revision %u, %d operation(s)\n",
					 WorldOperationJournal::Is_Open () ? "open" : "closed",
					 WorldOperationJournal::Get_Revision (),
					 WorldOperationJournal::Get_Operation_Count ());
			Print ("  applied %d, rejected %d, nav rebuilds %d\n",
					 WorldOperationJournal::Get_Applied_Count (),
					 WorldOperationJournal::Get_Rejected_Count (),
					 WorldOperationJournal::Get_Navigation_Rebuild_Count ());
			Print ("  journal checksum 0x%08X, terrain checksum 0x%08X\n",
					 WorldOperationJournal::Get_Journal_Checksum (),
					 WorldOperationJournal::Get_Terrain_Checksum ());

			//	The last few operations, so that "what just happened to the ground" is answerable
			//	without a debugger.
			int count = WorldOperationJournal::Get_Operation_Count ();
			int first = (count > 8) ? (count - 8) : 0;
			for (int i = first; i < count; i++) {
				WorldOperationClass operation;
				if (!WorldOperationJournal::Get_Operation (i,&operation)) continue;
				Print ("  #%u %s at %.1f,%.1f r=%.1f rev=%u crc=0x%08X\n",
						 operation.OperationId,World_Operation_Type_Name (operation.Type),
						 operation.Center.X,operation.Center.Y,operation.Radius,
						 operation.BaseRevision,operation.Compute_Checksum ());
			}
			return;
		}

		if (::stricmp (verb,"begin") == 0) {
			StringClass error ("");
			if (WorldOperationJournal::Begin (&error)) {
				Print ("world operation journal opened at revision %u\n",
						 WorldOperationJournal::Get_Revision ());
			} else {
				Print ("could not open a journal: %s\n",error.Peek_Buffer ());
			}
			return;
		}

		if (::stricmp (verb,"replay") == 0) {
			unsigned int before = WorldOperationJournal::Get_Terrain_Checksum ();

			StringClass report ("");
			bool ok = WorldOperationJournal::Replay (&report);

			unsigned int after = WorldOperationJournal::Get_Terrain_Checksum ();

			Print ("replay %s: %s\n",ok ? "complete" : "INCOMPLETE",report.Peek_Buffer ());
			Print ("  terrain checksum before 0x%08X, after 0x%08X -- %s\n",
					 before,after,(before == after) ? "MATCH" : "MISMATCH");
			return;
		}

		if (::stricmp (verb,"crater") == 0) {
			if (argcount < 5) { Print ("%s\n",Get_Help ()); return; }

			WorldOperationClass operation;
			operation.Type			= WORLD_OP_CREATE_CRATER;
			operation.BaseRevision	= WorldOperationJournal::Get_Revision ();
			operation.Center		= Vector3 (arg0,arg1,0.0f);
			operation.Radius		= arg2;
			operation.Depth		= arg3;
			operation.TimeSeconds	= (float)(TimeManager::Get_Total_Seconds ());

			WorldOperationApplyResult result = WorldOperationJournal::Submit (operation);
			Print ("crater: %s%s (revision %u) %s\n",
					 World_Operation_Result_Name (result.Result),
					 result.Committed ? ", committed" : "",
					 result.Revision,result.Detail.Peek_Buffer ());
			return;
		}

		if (::stricmp (verb,"restore") == 0) {
			if (argcount < 4) { Print ("%s\n",Get_Help ()); return; }

			WorldOperationClass operation;
			operation.Type			= WORLD_OP_RESTORE_REGION;
			operation.BaseRevision	= WorldOperationJournal::Get_Revision ();
			operation.Center		= Vector3 (arg0,arg1,0.0f);
			operation.Radius		= arg2;
			operation.TimeSeconds	= (float)(TimeManager::Get_Total_Seconds ());

			WorldOperationApplyResult result = WorldOperationJournal::Submit (operation);
			Print ("restore: %s%s (revision %u) %s\n",
					 World_Operation_Result_Name (result.Result),
					 result.Committed ? ", committed" : "",
					 result.Revision,result.Detail.Peek_Buffer ());
			return;
		}

		Print ("%s\n",Get_Help ());
	}
};


/*
**	Roadmap Section 21, Phase 20 step 10 -- the debug/status report for functional subcomponents.
**
**	Walks every building rather than taking an id: Commander building selection (P36's own WIP
**	entry) does not exist yet, so "the building nearest the cursor" has nothing to bind to, and a
**	report over every building with at least one declared subcomponent is small in practice --
**	Phase 20 wires two of them (the two Power Plants, one Communications Center) and nothing
**	else declares any yet.  Same GameObjManager -> ScriptableGameObj -> As_BuildingGameObj cast
**	worlddebugdisplay.cpp's building-footprints layer already uses, for the same reason its own
**	comment gives: a BuildingGameObj is not a PhysicalGameObj, and the cast is ambiguous straight
**	off a BaseGameObj* without the ScriptableGameObj step in between.
*/
class BuildingSubcomponentsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("building_subcomponents");}
	virtual	const char * Get_Help (void) override	{
		return ("BUILDING_SUBCOMPONENTS - functional subcomponent status for every building that declares one.");
	}
	virtual	void Activate (const char * /* input */) override {

		int buildings_reported = 0;

		SLNode<BaseGameObj> * objnode;
		for (objnode = GameObjManager::Get_Game_Obj_List ()->Head (); objnode; objnode = objnode->Next ()) {

			BaseGameObj * base = objnode->Data ();
			if (base == nullptr) continue;

			ScriptableGameObj * script_obj = base->As_ScriptableGameObj ();
			BuildingGameObj * building = (script_obj != nullptr) ? script_obj->As_BuildingGameObj () : nullptr;
			if (building == nullptr) continue;

			int count = building->Get_Subcomponent_Count ();
			if (count == 0) continue;

			buildings_reported++;
			Print ("building %d (%s):\n",building->Get_ID (),
					 building->Get_Definition ().Get_Descriptor_Name ().Peek_Buffer ());

			for (int i = 0; i < count; i++) {
				StringClass role_name ("");
				float function_fraction = 0.0f;
				bool anchor_resolves = false;
				if (!building->Get_Subcomponent_Status (i,&role_name,&function_fraction,&anchor_resolves)) continue;

				Print ("  %-28s function %.2f  anchor %s\n",
						 role_name.Peek_Buffer (),function_fraction,
						 anchor_resolves ? "resolved" : "not resolved");
			}
		}

		if (buildings_reported == 0) {
			Print ("no building in the world declares a functional subcomponent\n");
		}
	}
};


/*
**	Roadmap Section 22, Phase 21 -- the status report for the logistics graph.  Node and route
**	lists only, the same "no per-object selection exists yet" reasoning
**	BuildingSubcomponentsConsoleFunctionClass's own comment gives -- a report over the whole
**	graph is small in practice (bounded by the same generator candidate caps StrategicPlanner
**	already enforces).
*/
class LogisticsStatusConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("logistics_status");}
	virtual	const char * Get_Help (void) override	{
		return ("LOGISTICS_STATUS - node/route status for the Commander logistics graph.");
	}
	virtual	void Activate (const char * /* input */) override {

		if (!LogisticsGraphService::Has_Graph ()) {
			Print ("no logistics graph exists (profile.logistics.enabled is false, or no world is generated)\n");
			return;
		}

		Print ("logistics graph: %d node(s), %d route(s), checksum 0x%08X\n",
				 LogisticsGraphService::Get_Node_Count (),LogisticsGraphService::Get_Route_Count (),
				 LogisticsGraphService::Get_Checksum ());

		for (int i = 0; i < LogisticsGraphService::Get_Node_Count (); i++) {
			LogisticsNode node;
			if (!LogisticsGraphService::Get_Node (i,&node)) continue;

			Print ("  node %-3d %-15s team %-2d %s  (%.1f, %.1f, %.1f)\n",
					 i,Logistics_Node_Kind_Name (node.Kind),node.Team,
					 node.Operational ? "operational" : "not operational",
					 node.Position.X,node.Position.Y,node.Position.Z);
		}

		for (int i = 0; i < LogisticsGraphService::Get_Route_Count (); i++) {
			LogisticsRoute route;
			if (!LogisticsGraphService::Get_Route (i,&route)) continue;

			Print ("  route %-3d node %d <-> node %d  %-14s length %.1f\n",
					 i,route.FromNodeIndex,route.ToNodeIndex,
					 Logistics_Route_Status_Name (route.Status),route.Length);
		}
	}
};


/*
**	V2 roadmap Section 23, Phase 22 -- what the optional vehicle subsystem damage feature is
**	currently doing, if anything.  Prints the active profile's own settings and then the derived
**	response at the health fraction each live vehicle actually has, so "is this on, and what is
**	it doing to that tank right now" is one command rather than a guess.
*/
class VehicleSubsystemsConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("vehicle_subsystems");}
	virtual	const char * Get_Help (void) override	{
		return ("VEHICLE_SUBSYSTEMS - optional vehicle subsystem damage settings and per-vehicle effect.");
	}
	virtual	void Activate (const char * /* input */) override {

		const ProceduralWorldVehicleSubsystemSettings & settings =
			ProceduralWorldManager::Get_Active_Profile ().VehicleSubsystems;

		if (!settings.Enabled) {
			Print ("vehicle subsystems: disabled (profile.vehicle_subsystems.enabled is false)\n");
			Print ("  canonical Renegade vehicle handling is in effect; nothing below applies\n");
			return;
		}

		Print ("vehicle subsystems: enabled, immobilize %s\n",
				 settings.AllowImmobilize ? "allowed" : "not allowed");
		Print ("  engine   degrades below %.2f health, floor %.2f\n",
				 settings.EngineDamageStartFraction,settings.MinEngineFactor);
		Print ("  mobility degrades below %.2f health, floor %.2f\n",
				 settings.MobilityDamageStartFraction,settings.MinMobilityFactor);
		Print ("  weapon   slows below %.2f health, rate floor %.2f\n",
				 settings.WeaponDamageStartFraction,settings.MinWeaponFactor);
		if (settings.WeaponDisableFraction > 0.0f) {
			Print ("  weapon cut-off at or below %.2f health\n",settings.WeaponDisableFraction);
		} else {
			Print ("  weapon cut-off off -- a damaged weapon slows but never falls silent\n");
		}
		Print ("  immobilize at or below %.2f health, AI retreats to repair at or below %.2f\n",
				 settings.ImmobilizeFraction,settings.RetreatToRepairFraction);

		int reported = 0;
		SLNode<BaseGameObj> * objnode;
		for (objnode = GameObjManager::Get_Game_Obj_List ()->Head (); objnode; objnode = objnode->Next ()) {

			BaseGameObj * base = objnode->Data ();
			if (base == nullptr) continue;

			ScriptableGameObj * scriptable = base->As_ScriptableGameObj ();
			PhysicalGameObj * physical = (scriptable != nullptr) ? scriptable->As_PhysicalGameObj () : nullptr;
			VehicleGameObj * vehicle = (physical != nullptr) ? physical->As_VehicleGameObj () : nullptr;
			if (vehicle == nullptr) continue;

			DefenseObjectClass * defense = vehicle->Get_Defense_Object ();
			float health_fraction = ((defense != nullptr) && (defense->Get_Health_Max () > 0.0f))
											 ? (defense->Get_Health () / defense->Get_Health_Max ()) : 1.0f;

			Print ("  vehicle %-6d health %.2f  engine %.2f  mobility %.2f  fire rate %.2f%s%s\n",
					 vehicle->Get_ID (),health_fraction,
					 vehicle->Get_Subsystem_Engine_Factor (),
					 vehicle->Get_Subsystem_Mobility_Factor (),
					 vehicle->Get_Subsystem_Weapon_Efficiency (),
					 vehicle->Get_Subsystem_Weapon_Enabled () ? "" : "  WEAPON CUT",
					 vehicle->Get_Subsystem_Immobilized () ? "  IMMOBILIZED" : "");
			reported ++;
		}

		if (reported == 0) {
			Print ("  no vehicles exist right now\n");
		}
	}
};


class AshConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("ash");}
	virtual	const char * Get_Help (void) override	{return ("ASH <density [0+]> [ramptime [0+]] - Sets ash.");}
	virtual	void Activate (const char *input) override {

		const char *feedbackstring = "Ash changed\n";

		int	argcount;
		float	arg0, arg1;
	  	bool	success = false;

		argcount = sscanf (input, "%f %f", &arg0, &arg1);
		switch (argcount) {

			case 1:
				success = WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_ASH, arg0);
				break;

			case 2:
				success = WeatherMgrClass::Set_Precipitation (WeatherMgrClass::PRECIPITATION_ASH, arg0, arg1);
				break;

			default:
				// Do nothing.
				break;
		}
		if (success) Print (feedbackstring);
	}
};

/*
**
*/
class CinematicFreezeConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("cinematic_freeze");}
	virtual	const char * Get_Help (void) override	{return ("CINEMATIC_FREEZE - Toggles Cinematic Freeze.");}
	virtual	void Activate (const char * /* input */) override {
		GameObjManager::Toggle_Cinematic_Freeze();
		Print( "Cinematic Freeze %s\n",
			GameObjManager::Is_Cinematic_Freeze_Active() ? "ON" : "OFF" );
	}
};



class EditVehicleConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("edit_vehicle");}
	virtual	const char * Get_Help (void) override	{return ("EDIT_VEHICLE - Edit the parameters of the currently driven vehicle.");}
	virtual	void Activate (const char * /* input */) override
	{
		// Only do this in single player or 1-player multiplay games
      if (IS_SOLOPLAY || ((PTheGameData != nullptr) && (PTheGameData->Get_Max_Players() == 1)) )
		{
			VehicleGameObj * vehicle = COMBAT_STAR->Get_Vehicle();
			if ((vehicle != nullptr) && (vehicle->Peek_Vehicle_Phys() != nullptr)) {
				VehiclePhysClass * pobj = vehicle->Peek_Vehicle_Phys();

				TrackedVehicleClass * tv = pobj->As_TrackedVehicleClass();
				WheeledVehicleClass * wv = pobj->As_WheeledVehicleClass();

				if (tv != nullptr) {

					float wheel_radius = 0.0f;
					for (int i=0; i<tv->Get_Wheel_Count(); i++) {
						WheelClass * wheel = (WheelClass*)tv->Peek_Wheel(i);
						if (wheel->Get_Flag(SuspensionElementClass::ENGINE)) {
							wheel_radius = wheel->Get_Radius();
						}
					}

					TrackedVehicleDefClass * def = (TrackedVehicleDefClass *)tv->Get_TrackedVehicleDef();

					EditTrackedVehicleDialogClass * popup = new EditTrackedVehicleDialogClass(def,wheel_radius);
					popup->Start_Dialog();
					popup->Release_Ref();

				} else if (wv != nullptr) {

					float wheel_radius = 0.0f;
					for (int i=0; i<wv->Get_Wheel_Count(); i++) {
						WheelClass * wheel = (WheelClass*)wv->Peek_Wheel(i);
						if (wheel->Get_Flag(SuspensionElementClass::ENGINE)) {
							wheel_radius = wheel->Get_Radius();
						}
					}

					WheeledVehicleDefClass * def = (WheeledVehicleDefClass *)wv->Get_WheeledVehicleDef();
					EditWheeledVehicleDialogClass * popup = new EditWheeledVehicleDialogClass(def,wheel_radius);
					popup->Start_Dialog();
					popup->Release_Ref();

				}
			}
		}
	}
};


/*
**
*/
class ToggleUseLetencyInterpretConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("toggle_latency_interpret");}
	virtual	const char * Get_Help (void) override	{return ("TOGGLE_LATENCY_INTERPRET - Toggles Latency Interpret logic.");}
	virtual	void Activate (const char * /* input */) override {
		extern	bool	_UseLatencyInterpret;
		_UseLatencyInterpret	= !_UseLatencyInterpret;
		Print( "UseLetencyInterpret %s\n",
			_UseLatencyInterpret ? "ON" : "OFF" );
	}
};

/*
**
*/
class ToggleHitReticleConsoleFunctionClass : public ConsoleFunctionClass {
public:
	virtual	const char * Get_Name (void) override	{return ("toggle_hit_reticle");}
	virtual	const char * Get_Help (void) override	{return ("TOGGLE_HIT_RETICLE - Toggles the center reticle dot.");}
	virtual	void Activate (const char * /* input */) override {
		CombatManager::Toggle_Hit_Reticle_Enabled();
		Print( "Hit Reticle Toggled\n" );
	}
};


//------------------------------------------------------------------------------------
/*
** Console Function Manager
*/
DynamicVectorClass<ConsoleFunctionClass *>	ConsoleFunctionManager::FunctionList;

void	ConsoleFunctionManager::Init( void )
{

#ifdef WWDEBUG
   //
   // DEVELOPMENT CONSOLE FUNCTIONS ONLY
	// ONLY PRESENT IN DEBUG AND PROFILE BUILDS
   //

	FunctionList.Add( new AllowKillingHibernatingSpawnFunctionClass() );
	FunctionList.Add( new AmbientLightConsoleFunctionClass() );
	FunctionList.Add( new AmmoConsoleFunctionClass() );
	FunctionList.Add( new AppPacketTypesResetConsoleFunctionClass() );
	FunctionList.Add( new AshConsoleFunctionClass() );
	FunctionList.Add( new WorldOperationConsoleFunctionClass() );
	FunctionList.Add( new BuildingSubcomponentsConsoleFunctionClass() );
	FunctionList.Add( new LogisticsStatusConsoleFunctionClass() );
	FunctionList.Add( new VehicleSubsystemsConsoleFunctionClass() );
	FunctionList.Add( new WeatherStatusConsoleFunctionClass() );
	FunctionList.Add( new WeatherBudgetConsoleFunctionClass() );
	FunctionList.Add( new WeatherLayerConsoleFunctionClass() );
	FunctionList.Add( new TracerStatusConsoleFunctionClass() );
	FunctionList.Add( new TracerBudgetConsoleFunctionClass() );
	FunctionList.Add( new TracerLayerConsoleFunctionClass() );
	FunctionList.Add( new DebrisStatusConsoleFunctionClass() );
	FunctionList.Add( new DebrisBudgetConsoleFunctionClass() );
	FunctionList.Add( new DebrisLayerConsoleFunctionClass() );
	FunctionList.Add( new BreakExecutionConsoleFunctionClass() );
	FunctionList.Add( new CrashExecutionConsoleFunctionClass() );
	FunctionList.Add( new BuByeConsoleFunctionClass() );
	FunctionList.Add( new BuildingStateConsoleFunctionClass() );
	FunctionList.Add( new CameraConsoleFunctionClass() );
	FunctionList.Add( new CinematicFreezeConsoleFunctionClass() );
	FunctionList.Add( new ClearConsoleFunctionClass() );
	FunctionList.Add( new ClientHintFactorConsoleFunctionClass() );
	FunctionList.Add( new CloudsConsoleFunctionClass() );
	FunctionList.Add( new CopyLogsConsoleFunctionClass() );
	FunctionList.Add( new SkyTintConsoleFunctionClass() );
	FunctionList.Add( new CreateBotConsoleFunctionClass() );
	FunctionList.Add( new CreateGruntConsoleFunctionClass() );
	FunctionList.Add( new CreateObjectConsoleFunctionClass() );
	FunctionList.Add( new DazzleReloadConsoleFunctionClass() );
	FunctionList.Add( new DebugTypeConsoleFunctionClass() );
	FunctionList.Add( new DefectConsoleFunctionClass() );
	FunctionList.Add( new DisplayFindpathConsoleFunctionClass() );
	FunctionList.Add( new DisplayHibernatingFunctionClass() );
	FunctionList.Add( new DirtyCullDebugConsoleFunctionClass() );
   FunctionList.Add( new DonateConsoleFunctionClass() );
	FunctionList.Add( new DoStuffConsoleFunctionClass() );
	FunctionList.Add( new TerrainTestConsoleFunctionClass() );
	FunctionList.Add( new FarTerrainConsoleFunctionClass() );
	FunctionList.Add( new TerrainDressConsoleFunctionClass() );
	FunctionList.Add( new TerrainClearConsoleFunctionClass() );
	FunctionList.Add( new RoadTestConsoleFunctionClass() );
	FunctionList.Add( new RoadClearConsoleFunctionClass() );
	FunctionList.Add( new BridgeTestConsoleFunctionClass() );
	FunctionList.Add( new BridgeBreakConsoleFunctionClass() );
	FunctionList.Add( new BridgeClearConsoleFunctionClass() );
	FunctionList.Add( new FoliageTestConsoleFunctionClass() );
	FunctionList.Add( new FoliageCutConsoleFunctionClass() );
	FunctionList.Add( new FoliageClearConsoleFunctionClass() );
	FunctionList.Add( new WaterTestConsoleFunctionClass() );
	FunctionList.Add( new WaterClearConsoleFunctionClass() );
	FunctionList.Add( new RibbonTextureConsoleFunctionClass() );
	FunctionList.Add( new RibbonStatusConsoleFunctionClass() );
	FunctionList.Add( new RibbonClearConsoleFunctionClass() );
	FunctionList.Add( new MarkTextureConsoleFunctionClass() );
	FunctionList.Add( new MarkTestConsoleFunctionClass() );
	FunctionList.Add( new MarkStatusConsoleFunctionClass() );
	FunctionList.Add( new MarkClearConsoleFunctionClass() );
	FunctionList.Add( new ShadowStatusConsoleFunctionClass() );
	FunctionList.Add( new ShadowModeConsoleFunctionClass() );
	FunctionList.Add( new ShadowInvalidateConsoleFunctionClass() );
	FunctionList.Add( new ParticleStatusConsoleFunctionClass() );
	FunctionList.Add( new ParticleBatchingConsoleFunctionClass() );
	FunctionList.Add( new ParticlePoolConsoleFunctionClass() );
	FunctionList.Add( new DebugLayerConsoleFunctionClass() );
	FunctionList.Add( new DebugStatusConsoleFunctionClass() );
	FunctionList.Add( new CommanderModeConsoleFunctionClass() );
	FunctionList.Add( new CommanderStatusConsoleFunctionClass() );
	FunctionList.Add( new ReloadBuildingDescriptorsConsoleFunctionClass() );
	FunctionList.Add( new ReloadBuildingDescriptorConsoleFunctionClass() );
	FunctionList.Add( new BuildingJsonStatusConsoleFunctionClass() );
	FunctionList.Add( new TelemetryStatusConsoleFunctionClass() );
	FunctionList.Add( new TelemetryDumpConsoleFunctionClass() );
	FunctionList.Add( new CommanderSidebarConsoleFunctionClass() );
	FunctionList.Add( new WalletCapConsoleFunctionClass() );
	FunctionList.Add( new CommanderGhostConsoleFunctionClass() );
	FunctionList.Add( new CommanderPlaceConsoleFunctionClass() );
	FunctionList.Add( new CommanderBuildConsoleFunctionClass() );
	FunctionList.Add( new CommanderRTSStatusConsoleFunctionClass() );
	FunctionList.Add( new CommanderVoteConsoleFunctionClass() );
	FunctionList.Add( new CommanderTransferConsoleFunctionClass() );
	FunctionList.Add( new CommanderAIConsoleFunctionClass() );
	FunctionList.Add( new LightStatusConsoleFunctionClass() );
	FunctionList.Add( new LightTestConsoleFunctionClass() );
	FunctionList.Add( new LightClearConsoleFunctionClass() );
	FunctionList.Add( new DSAPOResetConsoleFunctionClass() );
	FunctionList.Add( new EnableTriangleRenderConsoleFunctionClass() );
	FunctionList.Add( new ExposePrelitConsoleFunctionClass() );
	FunctionList.Add( new FlashTextureConsoleFunctionClass() );
	FunctionList.Add( new FlyStarConsoleFunctionClass() );
	FunctionList.Add( new FogRangeConsoleFunctionClass() );
	FunctionList.Add( new FogToggleConsoleFunctionClass() );
	FunctionList.Add( new FrameTexturesConsoleFunctionClass() );
	FunctionList.Add( new FreeWeaponsConsoleFunctionClass() );
	FunctionList.Add( new GiveWeaponsConsoleFunctionClass() );
	FunctionList.Add( new GodConsoleFunctionClass() );
	FunctionList.Add( new IBelieveInGodConsoleFunctionClass() );
	FunctionList.Add( new IrrelevancePenaltyConsoleFunctionClass() );
	FunctionList.Add( new GroupControllerTestConsoleFunctionClass() );
	FunctionList.Add( new HideStarConsoleFunctionClass() );
	FunctionList.Add( new HideHUDPointsFunctionClass() );
	FunctionList.Add( new InvalidateMeshCacheConsoleFunctionClass() );
	FunctionList.Add( new InvalidateTexturesConsoleFunctionClass() );
	FunctionList.Add( new InfoDebugToggle() );
	FunctionList.Add( new HUDConsoleFunctionClass() );
	FunctionList.Add( new KillConsoleFunctionClass() );
	FunctionList.Add( new ListConsoleFunctionClass() );
	FunctionList.Add( new LightModeConsoleFunctionClass() );
	FunctionList.Add( new LightDebugToggleConsoleFunctionClass() );
	FunctionList.Add( new LightCalcConsoleFunctionClass() );
	FunctionList.Add( new LightLODConsoleFunctionClass() );
	FunctionList.Add( new LightningConsoleFunctionClass() );
	FunctionList.Add( new LrshCommandConsoleFunctionClass() );
	FunctionList.Add( new LogMeshStatsConsoleFunctionClass() );
	FunctionList.Add( new LogTexturesConsoleFunctionClass() );
	FunctionList.Add( new MainMenuConsoleFunctionClass() );
	FunctionList.Add( new MaxFacingPenaltyConsoleFunctionClass() );
	FunctionList.Add( new MeshDebuggerDisableMeshConsoleFunctionClass() );
	FunctionList.Add( new MeshDebuggerEnableConsoleFunctionClass() );
	FunctionList.Add( new MeshDebuggerEnableMeshConsoleFunctionClass() );
	FunctionList.Add( new MeshRenderSnapshotFunctionClass() );
	FunctionList.Add( new MissionConsoleFunctionClass() );
	FunctionList.Add( new MoneyConsoleFunctionClass() );
	FunctionList.Add( new MultiHudConsoleFunctionClass() );
	FunctionList.Add( new NextRenderConsoleFunctionClass() );
	FunctionList.Add( new OneShotKillsConsoleFunctionClass() );
	FunctionList.Add( new OpenConsoleFunctionClass() );
	FunctionList.Add( new Phys3NetConsoleFunctionClass() );
	FunctionList.Add( new PhysicsDebugConsoleFunctionClass() );
	FunctionList.Add( new PlayerPositionConsoleFunctionClass() );
	FunctionList.Add( new ProfileCollectBeginConsoleFunctionClass() );
	FunctionList.Add( new ProfileCollectEndConsoleFunctionClass() );
	FunctionList.Add( new ProjectorDebugConsoleFunctionClass() );
	//FunctionList.Add( new RadarMaxConsoleFunctionClass() );
	FunctionList.Add( new RadarToggleConsoleFunctionClass() );
	//FunctionList.Add( new RadarZoomConsoleFunctionClass() );
	FunctionList.Add( new RBodyNetConsoleFunctionClass() );
	FunctionList.Add( new RenderBackfacesConsoleFunctionClass() );
	FunctionList.Add( new RenderBoxesConsoleFunctionClass() );
	FunctionList.Add( new RenderModeConsoleFunctionClass() );
	FunctionList.Add( new RenderModeExtraPassConsoleFunctionClass() );
	FunctionList.Add( new ResolutionConsoleFunctionClass() );
	FunctionList.Add( new RunFastConsoleFunctionClass() );
	FunctionList.Add( new ParticleToggleConsoleFunctionClass() );
	FunctionList.Add( new ProfileConsoleFunctionClass() );
	FunctionList.Add( new QuickStatsConsoleFunctionClass() );
	FunctionList.Add( new RainConsoleFunctionClass() );
	//FunctionList.Add( new RshCommandConsoleFunctionClass() );
	FunctionList.Add( new ScaleTimeConsoleFunctionClass() );
	FunctionList.Add( new ScoreConsoleFunctionClass() );
	FunctionList.Add( new SnowConsoleFunctionClass() );
	FunctionList.Add( new SrshCommandConsoleFunctionClass() );
	FunctionList.Add( new DebugObjectivesConsoleFunctionClass() );
	FunctionList.Add( new CnCModeConsoleFunctionClass() );
	FunctionList.Add( new HUDHelpTextTestConsoleFunctionClass() );
	FunctionList.Add( new DebugEncyRevealConsoleFunctionClass() );
	FunctionList.Add( new DebugShowMoveVectorConsoleFunctionClass() );
	FunctionList.Add( new ChangeCharacterClassConsoleFunctionClass() );
	FunctionList.Add( new MapRevealConsoleFunctionClass() );
	FunctionList.Add( new ScriptTraceConsoleFunctionClass() );
	FunctionList.Add( new SetBandwidthBudgetOutToggleConsoleFunctionClass() );
	FunctionList.Add( new SetDesiredSleepConsoleFunctionClass() );
	FunctionList.Add( new SetPacketDuplicationConsoleFunctionClass() );
	FunctionList.Add( new SetPacketLatencyRangeConsoleFunctionClass() );
	FunctionList.Add( new SetPacketLossConsoleFunctionClass() );
	FunctionList.Add( new SetSpamCountConsoleFunctionClass() );
	FunctionList.Add( new SetStarHealthConsoleFunctionClass() );
	FunctionList.Add( new SetStarSkinConsoleFunctionClass() );
	FunctionList.Add( new SetStarShieldStrengthConsoleFunctionClass() );
	FunctionList.Add( new SetStarShieldTypeConsoleFunctionClass() );
	FunctionList.Add( new SetTheStarConsoleFunctionClass() );
	FunctionList.Add( new ShadowAttenuationConsoleFunctionClass() );
	FunctionList.Add( new ShadowBlobTextureConsoleFunctionClass() );
	FunctionList.Add( new ShadowCountConsoleFunctionClass() );
	FunctionList.Add( new ShadowElevationConsoleFunctionClass() );
	FunctionList.Add( new ShadowIntensityConsoleFunctionClass() );
	FunctionList.Add( new ShadowPerPolyCullingConsoleFunctionClass() );
	FunctionList.Add( new ShadowResolutionConsoleFunctionClass() );
	FunctionList.Add( new ShellCommandConsoleFunctionClass() );
	FunctionList.Add( new SkeletonSliderDemoConsoleFunctionClass() );
	FunctionList.Add( new SkeletonStandersConsoleFunctionClass() );
	FunctionList.Add( new SnapShotModeToggleFunctionClass() );
	FunctionList.Add( new SoldierGhostDebugDisplayConsoleFunctionClass() );
	FunctionList.Add( new StarVisibilityConsoleFunctionClass() );
	FunctionList.Add( new StatsConsoleFunctionClass() );
	//FunctionList.Add( new StealthConsoleFunctionClass() );
	FunctionList.Add( new SuicideConsoleFunctionClass() );
	FunctionList.Add( new SurfaceEffectsDebugConsoleFunctionClass() );
	FunctionList.Add( new SurfaceTypeOverrideConsoleFunctionClass() );
	FunctionList.Add( new SurfacePhysicsOverrideConsoleFunctionClass() );
	FunctionList.Add( new SwapTeamsConsoleFunctionClass() );
	FunctionList.Add( new TargetSteeringToggleConsoleFunctionClass() );
	FunctionList.Add( new TeleportStarConsoleFunctionClass() );
	FunctionList.Add( new TextureFilterModeConsoleFunctionClass() );
	FunctionList.Add( new TextureMemoryCounterConsoleFunctionClass() );
	FunctionList.Add( new TexturingEnableConsoleFunctionClass() );
	FunctionList.Add( new TimeOfDayConsoleFunctionClass() );
	FunctionList.Add( new ToggleAssetPreloadingConsoleFunctionClass() );
	FunctionList.Add( new ToggleEmotIconsConsoleFunctionClass() );
	FunctionList.Add( new ToggleHitReticleConsoleFunctionClass() );
	FunctionList.Add( new ToggleUseLetencyInterpretConsoleFunctionClass() );
	FunctionList.Add( new ToggleSimulationConsoleFunctionClass() );
	FunctionList.Add( new ToggleRenderingConsoleFunctionClass() );
	FunctionList.Add( new TomConsoleFunctionClass() );
	FunctionList.Add( new UmbraToggleFunctionClass() );
	FunctionList.Add( new UnusedSpawnerFunctionClass() );
	FunctionList.Add( new VipConsoleFunctionClass() );
	FunctionList.Add( new VisConsoleFunctionClass() );
	FunctionList.Add( new VisInvertConsoleFunctionClass() );
	FunctionList.Add( new VisGridDisplayConsoleFunctionClass() );
	FunctionList.Add( new VisGridDebugConsoleFunctionClass() );
	FunctionList.Add( new VisHierarchicalConsoleFunctionClass() );
	FunctionList.Add( new VisLockConsoleFunctionClass() );
	FunctionList.Add( new VisSectorDisplayConsoleFunctionClass() );
	FunctionList.Add( new VisSectorHistoryConsoleFunctionClass() );
	FunctionList.Add( new VblankSyncConsoleFunctionClass() );
	FunctionList.Add( new WatchConsoleFunctionClass() );
	FunctionList.Add( new WarBlitzConsoleFunctionClass() );
	FunctionList.Add( new WarpConsoleFunctionClass() );
	FunctionList.Add( new WhoIsConsoleFunctionClass() );
	FunctionList.Add( new WindConsoleFunctionClass() );
	FunctionList.Add( new WOLConsoleFunctionClass() );
	FunctionList.Add( new DataSafeStatsConsoleFunctionClass() );
	FunctionList.Add( new PatsDebugConsoleFunctionClass() );
	FunctionList.Add( new SetSendFrequencyConsoleFunctionClass() );
	FunctionList.Add( new TogglePacketDeltasConsoleFunctionClass() );
	FunctionList.Add( new TogglePacketComboConsoleFunctionClass() );
	FunctionList.Add( new TogglePacketOptConsoleFunctionClass() );
	FunctionList.Add( new SetLatencyConsoleFunctionClass() );
	FunctionList.Add( new ToggleNewClientUpdateMethodConsoleFunctionClass() );
	FunctionList.Add( new ToggleBandwidthBalancerConsoleFunctionClass() );
	FunctionList.Add( new NewTCADOConsoleFunctionClass() );

   FunctionList.Add( new DebugDeviceConsoleFunctionClass() );
	FunctionList.Add( new StatsConsoleFunctionClass() );
	FunctionList.Add( new ToggleSurfaceEffectsConsoleFunctionClass() );

	FunctionList.Add( new DeviceInfoConsoleFunctionClass() );

#endif // WWDEBUG (development commands)

   //
   // SHIPPING CONSOLE FUNCTIONS ONLY
   //

	FunctionList.Add( new GameOverConsoleFunctionClass() );
   FunctionList.Add( new AdminMessageConsoleFunctionClass() );
	FunctionList.Add( new GameInfoConsoleFunctionClass() );
	FunctionList.Add( new KickConsoleFunctionClass() );
	FunctionList.Add( new AllowConsoleFunctionClass() );

	FunctionList.Add( new BanConsoleFunctionClass() );
   FunctionList.Add( new MessageConsoleFunctionClass() );
	FunctionList.Add( new PlayerInfoConsoleFunctionClass() );
	FunctionList.Add( new QuitConsoleFunctionClass() );
	FunctionList.Add( new QuitSlaveConsoleFunctionClass() );
	FunctionList.Add( new RestartConsoleFunctionClass() );
	FunctionList.Add( new ScreenUVBiasConsoleFunctionClass() );
	FunctionList.Add( new SetBandwidthBudgetOutConsoleFunctionClass() );
	FunctionList.Add( new ToggleSortingConsoleFunctionClass() );
	FunctionList.Add( new RenLogMonConsoleFunctionClass() );
	FunctionList.Add( new RenLogMonOffConsoleFunctionClass() );
	FunctionList.Add( new ExtrasConsoleFunctionClass() );  /// CHEATS? MAY NEED TO BE DEV ONLY!!!!
	FunctionList.Add( new EditVehicleConsoleFunctionClass() );
	FunctionList.Add( new NetUpdateRateConsoleFunctionClass() );
	FunctionList.Add( new ClientPhysicsOptimizationConsoleFunctionClass() );
#ifndef FREEDEDICATEDSERVER
	FunctionList.Add( new FPSConsoleFunctionClass() );		// Steve W wanted this.
#endif //FREEDEDICATEDSERVER
#ifdef FREEDEDICATEDSERVER
	FunctionList.Add( new PageConsoleFunctionClass() );
#endif //FREEDEDICATEDSERVER


	SystemSettings::Add_Console_Functions( FunctionList );

	//
	//	The server-operator commands 4.8.4 added.  They go into this list, not
	//	a second one of their own.
	//
	TTConsole::Add_Console_Functions( FunctionList );

	Sort_Function_List();

	//
	// Dump list to a file
	//
	Verbose_Help_File();
}

void	ConsoleFunctionManager::Shutdown( void )
{
	while ( FunctionList.Count() ) {
		delete FunctionList[ FunctionList.Count()-1 ];
		FunctionList.Delete( FunctionList.Count()-1 );
	}
}

//------------------------------------------------------------------------------------

void	ConsoleFunctionManager::Sort_Function_List( void )
{
	// ARGH!! Bubblesort!!!
	bool done = false;
	int count = FunctionList.Count();
	while ( !done ) {
		done = true;
		count--;
		for (	int index = 0; index < count; index++) {
			if ( stricmp(	FunctionList[index]->Get_Name(),
								FunctionList[index+1]->Get_Name() ) > 0 ) {
				ConsoleFunctionClass * temp = FunctionList[index];
				FunctionList[index] = FunctionList[index+1];
				FunctionList[index+1] = temp;
				done = false;
			}
		}
	}
}

//------------------------------------------------------------------------------------

void	ConsoleFunctionManager::Help(	const char * function_name )
{
	char string[120];

	// Eat leading spaces
	while ( function_name && *function_name == ' ' ) {
		function_name++;
	}

	if ( ( function_name == nullptr ) || ( function_name[0] == 0 ) ) {
		// Show name of each function...
      Print( "------ Available commands -------\n" );
		*string = 0;
		for (	int index = 0; index < FunctionList.Count(); index++) {
			ConsoleFunctionClass * function = FunctionList[index];
			if ( *string ) {
				strcat( string, ", " );
			}
			strcat( string, function->Get_Name() );
			if ( ( strlen( string ) > (sizeof( string ) / 2) ) || (index == FunctionList.Count()-1) ) {
				strcat( string, "\n" );
				Print( string );
				*string = 0;
			}
		}
	} else {
		// Show help for a certian function
		for (	int index = 0; index < FunctionList.Count(); index++) {
			ConsoleFunctionClass * function = FunctionList[index];
			if ( !stricmp( function->Get_Name(), function_name ) ) {
		      Print( function->Get_Help() );
				return;
			}
		}
		Print( "Help not found for \"%s\"\n", function_name );
	}
}

//------------------------------------------------------------------------------------
int ConsoleFunctionManager::Get_Function_Count(void)
{
	return FunctionList.Count();
}

//------------------------------------------------------------------------------------
void ConsoleFunctionManager::Verbose_Help_File(void)
{
	//
	// Log function list to a file
	//
   FILE * file = fopen("commands.txt", "w");
	if ( file ) {
		char buffer[500];
		sprintf(buffer, "RENEGADE COMMANDS as at %s\n\n", cMiscUtil::Get_Text_Time());
		fwrite(buffer, 1, strlen(buffer), file);
		for (	int index = 0; index < FunctionList.Count(); index++) {
			ConsoleFunctionClass * function = FunctionList[index];
			WWASSERT(function != nullptr);
			sprintf(buffer, "%s", function->Get_Help());
			if (function->Get_Alias() != nullptr) {
				char alias_string[100];
				sprintf(alias_string, " (%s)", function->Get_Alias());
				strcat(buffer, alias_string);
			}
			strcat(buffer, "\n");
			fwrite(buffer, 1, strlen(buffer), file);
		}

#ifdef WWDEBUG
		char watch_help_buffer[5000];
      WatchConsoleFunctionClass::Get_Verbose_Help(watch_help_buffer, sizeof(watch_help_buffer));
		fwrite(watch_help_buffer, 1, strlen(watch_help_buffer), file);
#endif

		fclose(file);
	}
}

//------------------------------------------------------------------------------------
void ConsoleFunctionManager::Next_Verbose_Help_Screen(void)
{
	//
	// Show name of each function on a separate line...
	//
	char buffer[5000];
	buffer[0] = 0;

	static int page_number = -1;
	page_number++;

	if ( page_number >= 0 ) {
		int command_number = 0;
		for (	int index = 0; index < FunctionList.Count(); index++) {
			command_number++;
			int per_page = 57;
			if (floor(command_number / per_page) == page_number) {
				ConsoleFunctionClass * function = FunctionList[index];
				WWASSERT(function != nullptr);
				strcat( buffer, function->Get_Help() );
				strcat( buffer, "\n" );

				if ( index == FunctionList.Count() - 1 ) {	// if this is the last line,
					page_number = -2;				// turn it off for next time

#ifdef WWDEBUG
					//
               // Also, show help for watch objects
               //
               strcat( buffer, "\n" );
               char watch_help_buffer[10000];
               WatchConsoleFunctionClass::Get_Verbose_Help(watch_help_buffer, sizeof(watch_help_buffer));
   				strcat( buffer, watch_help_buffer );
#endif // WWDEBUG
				}
			}
		}
	}
	if (Get_Text_Display()) {
		Get_Text_Display()->Set_Verbose_Help_Text( buffer );
	}
}

//------------------------------------------------------------------------------------
void	ConsoleFunctionManager::Parse_Input( const char * string )
{
	if (!strnicmp(string, "help", 4)) {
		Help( string + 4 );
	} else {
		for (	int index = 0; index < FunctionList.Count(); index++) {
			ConsoleFunctionClass * function = FunctionList[index];

         if ( !strnicmp( string, function->Get_Name(), strlen( function->Get_Name() ) ) ) {

				const char *s = string + strlen (function->Get_Name());

				// The next input character must be either a string terminator or space character
				// in order for the input string to match the function list string.
				if ((*s == '\0') || (*s == ' ')) {

					// Remove any trailing white space.
					while (*s == ' ') s++;

					function->Activate (s);
					return;
				}
         }

         //
         // Check short-form alias too
         // There has to be a space (or nothing) following the alias for
         // it to match
         //
         if (function->Get_Alias() != nullptr &&
            strlen(function->Get_Alias()) > 0) {
            char alias[100];
            WWASSERT(strlen(function->Get_Alias()) < sizeof(alias));
            strcpy(alias, function->Get_Alias());
            const size_t alias_len = ::strlen(alias);
            const size_t string_len = ::strlen(string);
            if (!strnicmp(string, alias, alias_len) &&
               (string_len == alias_len || string[alias_len] == ' ')) {
				   string += alias_len;
				   while ( string && *string == ' ' ) string++;
				   function->Activate( string );
				   return;
            }
         }
		}

		if (ConsoleBox.Is_Exclusive()) {
      	Print( "\"%s\" is not a command. To send a message, use \"msg\" and \"amsg\"\n", string );
		} else {
      	Print( "\"%s\" is not a command. To send a message, Use F2 and F3\n", string );
		}
      Print( "For a list of available commands, use the \"help\" command !\n" );
	}
}

bool ConsoleFunctionManager::Get_Command_Suggestion
(
	const char *	input,					// what user has typed,
	const char *	cur_suggestion,		// last suggestion we gave (move to next if not null)
	char *			set_suggestion,		// copy suggestion into this string
	char *			set_help,				// copy help into this string
	int				len						// don't copy more than this many characters.
)
{
	ConsoleFunctionClass * function = nullptr;
	int	node_index = 0;

	/*
	** If user is passing in a 'cur_suggestion', then we need to first find that
	** function in the list and search past it.  If we don't find a suggestion, we
	** will drop through and search from the head of the list.
	*/
	if (cur_suggestion != nullptr) {

		node_index = Find_Function_Node(cur_suggestion);

		/*
		** Try to find a suggestion after this node.
		*/
		if (node_index != -1) {
			function = Find_Command_Suggestion(input,node_index+1);
		}
	}

	/*
	** Find a suggestion starting at the head of the list
	*/
	if (function == nullptr) {
		function = Find_Command_Suggestion(input,0);
	}

	/*
	** If we found a suggestion, copy its command into 'set_suggestion' and its help
	** into 'set_help'
	*/
	if (function != nullptr) {
		if (set_suggestion != nullptr) {
			strncpy(set_suggestion,function->Get_Name(),len);
		}
		if (set_help != nullptr) {
			strncpy(set_help,function->Get_Help(),len);
		}
		return true;
	}
	return false;
}

int ConsoleFunctionManager::Find_Function_Node(const char * name)
{
	for (	int index = 0; index < FunctionList.Count(); index++) {
		ConsoleFunctionClass * function = FunctionList[index];
		if (stricmp(name,function->Get_Name()) == 0) {
			return index;
		}
	}
	return -1;
}

ConsoleFunctionClass * ConsoleFunctionManager::Find_Command_Suggestion
(
	const char *	input,		// incomplete function name
	int				start_index	// node to start searching from
)
{
	for (	int index = start_index; index < FunctionList.Count(); index++) {
		ConsoleFunctionClass * function = FunctionList[index];
		if (_strnicmp(input,function->Get_Name(),strlen(input)) == 0) {
			return function;
		}
	}
	return nullptr;
}

void	ConsoleFunctionManager::Print( const char *format, ... )
{
	va_list arg_list;
	va_start (arg_list, format);
	StringClass string;
	string.Format_Args( format, arg_list );
	if (Get_Text_Display()) {
		WWASSERT( Get_Text_Display() );
		Get_Text_Display()->Print_System( "%s", string.Peek_Buffer() );
	}
	ConsoleBox.Print("%s", string.Peek_Buffer());
	va_end (arg_list);
}
