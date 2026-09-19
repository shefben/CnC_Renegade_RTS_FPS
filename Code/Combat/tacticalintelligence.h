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
 *                     $Archive:: /Commando/Code/Combat/tacticalintelligence.h                $*
 *                                                                                             *
 *	Roadmap Section 31, Zero Hour / SAGE feature 16 -- radar system.                             *
 *                                                                                             *
 *	"Implement one shared tactical intelligence model."  Renegade already has a working         *
 *	radar -- RadarManager walks every PhysicalGameObj once a frame, filters it by RadarMode     *
 *	and stealth, and draws a blip.  That loop is the "existing Renegade... radar API" Section    *
 *	31 says has to feed the model rather than be duplicated beside it, so                        *
 *	Build_FPS_Contacts is that exact loop, moved here and returned as data instead of drawn      *
 *	directly.  RadarManager::Update calls it and draws what comes back; nothing about what       *
 *	a player sees on their HUD radar changed by this move, which is the point -- Phase 35's      *
 *	own acceptance line is "FPS behavior outside Commander Mode is unchanged" and this is the    *
 *	first place that has to already be true.                                                    *
 *                                                                                             *
 *	Build_Strategic_Contacts is the other half: a per-team, shroud-aware query over the same     *
 *	world, for the Commander/tactical consumers Section 31 names that do not exist yet           *
 *	(the map/sidebar presentation is Phase 36+, out of this pass).  It is deliberately a         *
 *	second function rather than a flag on the first one, because Section 31 also says: "Do not   *
 *	expose strategic radar-only/last-known information as live FPS wallhacks."  Build_FPS_       *
 *	Contacts never calls WorldShroudManager and never produces a RADAR_ONLY or LAST_KNOWN        *
 *	contact -- those values are only reachable through Build_Strategic_Contacts, which nothing   *
 *	FPS-facing calls.  A Commander camera is not a special case of either function: it is not a  *
 *	viewer argument accepted anywhere in this file, so "Commander camera position is never       *
 *	radar coverage" holds by the same construction WorldShroudManager's camera rule does.         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TACTICALINTELLIGENCE_H
#define TACTICALINTELLIGENCE_H

#include "always.h"
#include "vector.h"
#include "vector3.h"
#include "worldshroudmanager.h"

class PhysicalGameObj;


/*
**	What a contact is.  This is the roadmap's own list from Section 31 -- infantry, vehicles,
**	buildings, resources, objectives, waypoints, Commander placement markers, radar-only
**	contacts, last-known contacts -- as one enumeration rather than one class per kind.
**	RESOURCE and WAYPOINT are named here for completeness even though nothing produces them
**	yet: there is no Tiberium-field GameObj and no waypoint-order renderer in this codebase
**	today (Section 32, Phase 29, is the latter's phase). A later producer only has to fill in
**	the value, not invent a new type for it.
*/
enum TacticalContactType
{
	CONTACT_INFANTRY = 0,
	CONTACT_VEHICLE,
	CONTACT_BUILDING,
	CONTACT_RESOURCE,
	CONTACT_OBJECTIVE,
	CONTACT_WAYPOINT,
	CONTACT_COMMANDER_MARKER,
	CONTACT_RADAR_ONLY,
	CONTACT_LAST_KNOWN,
	CONTACT_OTHER,
};


/*
**	One entry.  BlipShapeType/BlipColorType are RadarManager's own enum values, copied
**	verbatim from whatever produced this contact (PhysicalGameObj::Get_Radar_Blip_Shape_Type
**	et al, or an objective/marker's own field) rather than re-declared here -- Section 31 asks
**	for one model, not a second set of blip categories parallel to the one RadarManager
**	already has.
*/
class TacticalContactClass
{
public:
	TacticalContactClass(void);

	bool	operator == (const TacticalContactClass &) const	{ return false; }
	bool	operator != (const TacticalContactClass &) const	{ return true; }

	PhysicalGameObj *	GameObj;			// non-owning; nullptr for objective/marker/synthetic contacts
	TacticalContactType	Type;
	int						Team;				// a PLAYERTYPE_* value, or -1 for unowned/neutral
	Vector3					Position;
	int						BlipShapeType;
	int						BlipColorType;
	float						Intensity;
	bool						Bracketed;
	bool						AltitudeFade;
	ShroudStateType		Shroud;			// only meaningful from Build_Strategic_Contacts
};

typedef DynamicVectorClass<TacticalContactClass>	TacticalContactListClass;


class TacticalIntelligenceModel
{
public:

	/*
	**	Exactly what RadarManager::Update used to compute inline: every PhysicalGameObj that
	**	is not the viewer, is not a dead soldier, passes radar_mode (RadarManager::RadarModeEnum,
	**	passed as int so this header does not have to include the HUD renderer's), and is not an
	**	enemy-stealthed unit.  bracket_obj marks the one contact, if any, that should read as
	**	bracketed.  viewer may be nullptr (menus, a dedicated server with no local player);
	**	the result is then every object unfiltered by team, matching the pre-existing
	**	`COMBAT_STAR != nullptr` guard in the code this replaces.
	*/
	//	altitude_fade_z/altitude_fade_range reproduce RadarManager's own star_z/RADAR_Z_RANGE
	//	comparison; they are passed in rather than duplicated here so the fade threshold stays
	//	defined in exactly the one HUD-rendering file that owns it.
	static void		Build_FPS_Contacts(PhysicalGameObj * viewer,int radar_mode,
													const PhysicalGameObj * bracket_obj,
													float altitude_fade_z,float altitude_fade_range,
													TacticalContactListClass & out);

	/*
	**	Every PhysicalGameObj classified as INFANTRY/VEHICLE/BUILDING/OTHER, plus every pending
	**	objective (CONTACT_OBJECTIVE) and every RadarManager marker (CONTACT_COMMANDER_MARKER),
	**	filtered through WorldShroudManager for viewing_team: a contact whose position is
	**	SHROUD_VISIBLE keeps its natural type, SHROUD_RADAR_DETECTED becomes CONTACT_RADAR_ONLY,
	**	SHROUD_LAST_KNOWN becomes CONTACT_LAST_KNOWN, and SHROUD_UNKNOWN/SHROUD_EXPLORED with no
	**	live coverage is left out entirely.  Objectives and markers are included unconditionally
	**	(matching their existing always-shown FPS behaviour) rather than shroud-gated, since nothing
	**	registers them as having a world footprint smaller than "known to the team running the
	**	mission".  No renderer reads this yet -- see the design doc for what does not exist to
	**	consume it.
	*/
	static void		Build_Strategic_Contacts(int viewing_team,TacticalContactListClass & out);

	static int		Get_Last_FPS_Contact_Count(void);
	static int		Get_Last_Strategic_Contact_Count(void);

private:

	static void		Classify(PhysicalGameObj * obj,TacticalContactClass * out);

	static int		LastFPSCount;
	static int		LastStrategicCount;
};

#endif	// TACTICALINTELLIGENCE_H
