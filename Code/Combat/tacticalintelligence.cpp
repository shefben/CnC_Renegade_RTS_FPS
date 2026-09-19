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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/tacticalintelligence.cpp              $*
 *                                                                                             *
 *	Roadmap Section 31, Zero Hour / SAGE feature 16 -- radar system.                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "tacticalintelligence.h"

#include "building.h"
#include "gameobjmanager.h"
#include "objectives.h"
#include "physicalgameobj.h"
#include "playertype.h"
#include "radar.h"
#include "scriptablegameobj.h"
#include "soldier.h"
#include "wwmath.h"


int	TacticalIntelligenceModel::LastFPSCount			= 0;
int	TacticalIntelligenceModel::LastStrategicCount	= 0;


/*
**	PhysicalGameObj and BuildingGameObj are both direct, independent descendants of
**	DamageableGameObj/CombatPhysObserverClass rather than one deriving from the other -- a
**	building is never a PhysicalGameObj and a PhysicalGameObj can never be a building, which
**	is also why the original RadarManager loop this file replaces (Build_FPS_Contacts) never
**	produced a building blip: its walk starts from As_PhysicalGameObj() and a building fails
**	that cast.  Build_Strategic_Contacts below classifies from the raw BaseGameObj instead, so
**	it can reach both branches of that split -- see Classify() and the building branch inline
**	in Build_Strategic_Contacts.  Reaching As_BuildingGameObj() from a raw BaseGameObj* takes
**	one extra step, As_ScriptableGameObj() first: BaseGameObj declares As_PhysicalGameObj
**	itself (so that one resolves directly), but As_BuildingGameObj is only declared lower, on
**	ScriptableGameObj -- and PhysicalGameObj/BuildingGameObj each separately inherit a second,
**	unrelated declaration of it from CombatPhysObserverClass, which is what made calling it
**	directly on either of those two types ambiguous (C2385) rather than simply absent.
**
**	A PhysicalGameObj keeps its own radar blip colour (set from Reset_Radar_Blip_Color_Type,
**	physicalgameobj.cpp); a BuildingGameObj has no such field, since nothing blipped it before.
**	This reproduces that exact mapping rather than inventing a second one.
*/
static int	Player_Type_To_Blip_Color(int player_type)
{
	switch (player_type) {
		case PLAYERTYPE_NOD:			return RadarManager::BLIP_COLOR_TYPE_NOD;
		case PLAYERTYPE_GDI:			return RadarManager::BLIP_COLOR_TYPE_GDI;
		case PLAYERTYPE_MUTANT:		return RadarManager::BLIP_COLOR_TYPE_MUTANT;
		case PLAYERTYPE_RENEGADE:	return RadarManager::BLIP_COLOR_TYPE_RENEGADE;
		default:							return RadarManager::BLIP_COLOR_TYPE_NEUTRAL;
	}
}


TacticalContactClass::TacticalContactClass(void) :
	GameObj(nullptr),
	Type(CONTACT_OTHER),
	Team(-1),
	Position(0,0,0),
	BlipShapeType(0),
	BlipColorType(0),
	Intensity(0.0f),
	Bracketed(false),
	AltitudeFade(false),
	Shroud(SHROUD_UNKNOWN)
{
}


/***********************************************************************************************
 * TacticalIntelligenceModel::Classify -- what kind of contact a PhysicalGameObj is             *
 *=============================================================================================*/
void TacticalIntelligenceModel::Classify(PhysicalGameObj * obj,TacticalContactClass * out)
{
	out->GameObj			= obj;
	obj->Get_Position(&out->Position);
	out->Team				= obj->Get_Player_Type();
	out->BlipShapeType	= obj->Get_Radar_Blip_Shape_Type();
	out->BlipColorType	= obj->Get_Radar_Blip_Color_Type();
	out->Intensity			= obj->Get_Radar_Blip_Intensity();
	out->Bracketed			= false;
	out->AltitudeFade		= false;
	out->Shroud				= SHROUD_UNKNOWN;

	if (obj->As_SoldierGameObj() != nullptr) {
		out->Type = CONTACT_INFANTRY;
	} else if (obj->As_VehicleGameObj() != nullptr) {
		out->Type = CONTACT_VEHICLE;
	} else {
		//	Not a building either -- see the note above Player_Type_To_Blip_Color: a
		//	PhysicalGameObj structurally cannot be one.
		out->Type = CONTACT_OTHER;
	}
}


/***********************************************************************************************
 * TacticalIntelligenceModel::Build_FPS_Contacts -- today's exact HUD radar contact set          *
 *=============================================================================================*/
void TacticalIntelligenceModel::Build_FPS_Contacts(PhysicalGameObj * viewer,int radar_mode,
																		const PhysicalGameObj * bracket_obj,
																		float altitude_fade_z,float altitude_fade_range,
																		TacticalContactListClass & out)
{
	out.Delete_All();
	LastFPSCount = 0;

	SLNode<BaseGameObj> * objnode;
	for (objnode = GameObjManager::Get_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {

		PhysicalGameObj * obj = objnode->Data()->As_PhysicalGameObj();
		if (obj == nullptr) {
			continue;
		}
		if (obj == viewer) {
			continue;						// don't report the viewer to itself
		}
		if (obj->Peek_Physical_Object() == nullptr) {
			continue;
		}

		//	Don't report dead soldiers.
		if (obj->As_SoldierGameObj() &&
			 obj->Get_Defense_Object() &&
			 obj->Get_Defense_Object()->Get_Health() <= 0.0f) {
			continue;
		}

		//	RadarMode filtering, exactly as RadarManager::Update always applied it.
		if (viewer != nullptr && radar_mode != RADAR_ALL) {
			if (radar_mode == RADAR_NOBODY) {
				continue;
			} else {
				WWASSERT(radar_mode == RADAR_TEAMMATES);
				int other_pt = obj->Get_Player_Type();
				int my_pt = viewer->Get_Player_Type();
				if (my_pt != other_pt ||
					 (my_pt != PLAYERTYPE_NOD && my_pt != PLAYERTYPE_GDI)) {
					continue;
				}
			}
		}

		//	Enemy-stealthed units are not reported.  Reproduced verbatim, including the
		//	unguarded viewer dereference the code being replaced already had.
		SmartGameObj * smart_obj = obj->As_SmartGameObj();
		if ((smart_obj != nullptr) && (smart_obj->Is_Stealthed())) {
			int obj_pt = smart_obj->Get_Player_Type();
			int my_pt = viewer->Get_Player_Type();
			if (my_pt != obj_pt) {
				continue;
			}
		}

		TacticalContactClass contact;
		Classify(obj,&contact);
		contact.Bracketed = (obj == bracket_obj);
		if (altitude_fade_range > 0.0f) {
			float z_diff = altitude_fade_z - contact.Position.Z;
			contact.AltitudeFade = (WWMath::Fabs(z_diff) > altitude_fade_range);
		}

		out.Add(contact);
		LastFPSCount++;
	}
}


/***********************************************************************************************
 * TacticalIntelligenceModel::Build_Strategic_Contacts -- per-team, shroud-aware contact set     *
 *=============================================================================================*/
void TacticalIntelligenceModel::Build_Strategic_Contacts(int viewing_team,TacticalContactListClass & out)
{
	out.Delete_All();
	LastStrategicCount = 0;

	SLNode<BaseGameObj> * objnode;
	for (objnode = GameObjManager::Get_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {

		BaseGameObj * base = objnode->Data();
		if (base == nullptr) {
			continue;
		}

		TacticalContactClass contact;

		PhysicalGameObj * obj = base->As_PhysicalGameObj();
		if (obj != nullptr) {
			if (obj->Peek_Physical_Object() == nullptr) {
				continue;
			}
			Classify(obj,&contact);
		} else {
			ScriptableGameObj * script_obj = base->As_ScriptableGameObj();
			BuildingGameObj * building = (script_obj != nullptr) ? script_obj->As_BuildingGameObj() : nullptr;
			if (building == nullptr) {
				continue;			// neither a PhysicalGameObj nor a BuildingGameObj
			}
			contact.GameObj			= nullptr;		// no PhysicalGameObj to hand back
			contact.Type				= CONTACT_BUILDING;
			contact.Team				= building->Get_Player_Type();
			building->Get_Position(&contact.Position);
			contact.BlipShapeType	= RadarManager::BLIP_SHAPE_TYPE_STATIONARY;
			contact.BlipColorType	= Player_Type_To_Blip_Color(contact.Team);
			contact.Intensity		= 1.0f;
		}

		ShroudStateType state = WorldShroudManager::Get_Shroud_State(viewing_team,contact.Position);
		if (state == SHROUD_VISIBLE) {
			contact.Shroud = state;
		} else if (state == SHROUD_RADAR_DETECTED) {
			contact.Type	= CONTACT_RADAR_ONLY;
			contact.Shroud = state;
		} else if (state == SHROUD_LAST_KNOWN) {
			contact.Type	= CONTACT_LAST_KNOWN;
			contact.Shroud = state;
		} else {
			continue;			// SHROUD_UNKNOWN, or SHROUD_EXPLORED with nothing live there
		}

		out.Add(contact);
		LastStrategicCount++;
	}

	//	Objectives are mission-known to the whole team running the mission today (RadarManager
	//	has always drawn every pending, blip-marked objective unconditionally); this keeps that
	//	behaviour rather than inventing a shroud rule Section 31 never asked for.
	int objective_count = ObjectiveManager::Get_Objective_Count();
	for (int i = 0; i < objective_count; i++) {
		Objective * objective = ObjectiveManager::Get_Objective(i);
		if (objective->DrawBlip && objective->Status == ObjectiveManager::STATUS_IS_PENDING) {
			TacticalContactClass contact;
			contact.GameObj			= nullptr;
			contact.Type				= CONTACT_OBJECTIVE;
			contact.Team				= -1;
			contact.Position			= objective->Position;
			contact.BlipShapeType	= RadarManager::BLIP_SHAPE_TYPE_OBJECTIVE;
			contact.BlipColorType	= objective->Radar_Blip_Color_Type();
			contact.Intensity		= objective->BlipIntensity;
			contact.Shroud				= SHROUD_VISIBLE;
			out.Add(contact);
			LastStrategicCount++;
		}
	}

	//	RadarManager's existing generic markers -- the closest existing match to "Commander
	//	placement markers" the roadmap names.
	int marker_count = RadarManager::Get_Marker_Count();
	for (int i = 0; i < marker_count; i++) {
		const RadarMarkerClass & marker = RadarManager::Get_Marker(i);
		TacticalContactClass contact;
		contact.GameObj			= nullptr;
		contact.Type				= CONTACT_COMMANDER_MARKER;
		contact.Team				= -1;
		contact.Position			= marker.Position;
		contact.BlipShapeType	= marker.Type;
		contact.BlipColorType	= marker.Color;
		contact.Intensity		= marker.Intensity;
		contact.Shroud				= SHROUD_VISIBLE;
		out.Add(contact);
		LastStrategicCount++;
	}
}


int TacticalIntelligenceModel::Get_Last_FPS_Contact_Count(void)
{
	return LastFPSCount;
}

int TacticalIntelligenceModel::Get_Last_Strategic_Contact_Count(void)
{
	return LastStrategicCount;
}
