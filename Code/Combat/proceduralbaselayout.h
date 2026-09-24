/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/proceduralbaselayout.h                $*
 *                                                                                             *
 *	V2 roadmap Section 16A -- a selected base site becomes a playable base.                      *
 *                                                                                             *
 *	BaseSelectionPlanner (Phase 11) chooses where each faction's base goes and reserves a        *
 *	protected circle there.  This service is what happens next, in the order Section 16A gives:  *
 *                                                                                             *
 *	  - Plan_And_Place lays out the profile's initial buildings (16A.2) inside that circle and   *
 *	    creates them.  Every candidate spot is judged by CommanderPlacementValidator::Validate,   *
 *	    the same function a Commander's ghost and the server's verdict use, so a starting base   *
 *	    obeys every placement rule a built one does: bounds, slope, water, cliffs, bridges,      *
 *	    tunnels, resource routes, chokepoints, overlap and production-exit clearance (16A.3).   *
 *	    Buildings face the enemy base, so a factory's +Y exit opens toward the fight.            *
 *	  - Generate_Base_Spawn_Set builds each faction's spawn pool (16A.4-16A.6) from the live      *
 *	    base -- whatever buildings stand there now, whether just placed or loaded from a save --  *
 *	    on the side away from the enemy first.                                                   *
 *	  - Choose_Respawn_Point is the runtime half (16A.6): the server scores the pool's            *
 *	    currently usable points and picks one.  SpawnManager asks it before its spawner list.    *
 *                                                                                             *
 *	Vehicles keep canonical factory production (16A.7).  Nothing here runs on a world fitted to *
 *	a hand-authored level: base selection never runs there, the level's own spawners stay the   *
 *	answer, and Choose_Respawn_Point declines.                                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALBASELAYOUT_H
#define PROCEDURALBASELAYOUT_H

#include "always.h"
#include "matrix3d.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"

class ProceduralWorldProfile;
class SoldierGameObj;


struct ProceduralBaseBuildingRecord
{
	ProceduralBaseBuildingRecord(void) : Position(0.0f,0.0f,0.0f),RotationDegrees(0.0f),
													 Radius(0.0f),BuildingId(0),Refusal(0) { }
	bool	operator == (const ProceduralBaseBuildingRecord &) const	{ return false; }
	bool	operator != (const ProceduralBaseBuildingRecord &) const	{ return true; }

	StringClass	DescriptorName;
	Vector3		Position;
	float			RotationDegrees;
	float			Radius;				// the validator's footprint disc
	int			BuildingId;			// 0 when the building could not be placed or created
	int			Refusal;				// the last CommanderPlacementResultType a candidate got, when unplaced
};


struct ProceduralSpawnCandidate
{
	ProceduralSpawnCandidate(void) : Position(0.0f,0.0f,0.0f),FacingDegrees(0.0f) { }
	bool	operator == (const ProceduralSpawnCandidate &) const	{ return false; }
	bool	operator != (const ProceduralSpawnCandidate &) const	{ return true; }

	Vector3	Position;			// on the ground
	float		FacingDegrees;		// toward the enemy base, so a spawned player faces the fight
};


class ProceduralBaseLayoutService
{
	public:

		enum { FACTION_COUNT = 2 };

		static void		Reset(void);

		/*
		**	Server: lay out and create both factions' initial buildings, then build both spawn
		**	pools.  False, with a report, when there was no selected base to lay out; a building
		**	that found no legal spot is reported and skipped, never forced.  Each faction's
		**	buildings are placed and created one at a time, so the overlap test the next one faces
		**	sees the ones before it.
		*/
		static bool		Plan_And_Place(const ProceduralWorldProfile & profile,StringClass * report_out);

		/*
		**	A faction's spawn pool, from its selected site and the buildings standing there now.
		**	Separate from placement (16A.6: generation and choice are different jobs), and callable
		**	on its own -- after a save loads its buildings back, it rebuilds the pool around them.
		*/
		static int		Generate_Base_Spawn_Set(int faction,const ProceduralWorldProfile & profile);

		/*
		**	Server, at respawn time.  False when this world has no generated base for the faction,
		**	which sends SpawnManager back to the level's own spawners.  Otherwise the pool point
		**	that is clear for this soldier and farthest from any living enemy; the fallback zone's
		**	centre when no pool point is clear.
		*/
		static bool		Choose_Respawn_Point(int faction,SoldierGameObj * soldier,Matrix3D * out_transform);

		static bool		Has_Base(int faction);
		static int		Get_Building_Count(int faction);
		static bool		Get_Building(int faction,int index,ProceduralBaseBuildingRecord * out);
		static int		Get_Spawn_Candidate_Count(int faction);
		static bool		Get_Spawn_Candidate(int faction,int index,ProceduralSpawnCandidate * out);
		static bool		Get_Spawn_Zone(int faction,Vector3 * out_center,float * out_radius);

		//	The candidate spots Plan_And_Place tries for the index-th building, in order: the site
		//	itself for the anchor, then rings outward starting behind the base.  Pure; public for
		//	the self-check.
		static int		Get_Candidate_Positions(const Vector3 & center,const Vector3 & toward_enemy,
															 float ring_step,float max_radius,
															 DynamicVectorClass<Vector3> * out);
};

#endif // PROCEDURALBASELAYOUT_H
