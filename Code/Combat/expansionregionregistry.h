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
 *                     $Archive:: /Commando/Code/Combat/expansionregionregistry.h             $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 17 steps 10-13 -- construction footprint reservation, the         *
 *	post-construction world update, and forward-base/expansion region metadata.                   *
 *                                                                                             *
 *	Step 10 is the one that has to exist for any of the rest to be safe.  Between the moment a     *
 *	placement is accepted and the moment `BuildingConstructionService` finishes raising the         *
 *	structure, there is no building in the world: no collision object, nothing in the spatial        *
 *	index, nothing for the next placement's overlap query to hit.  Two Commanders -- or one          *
 *	Commander and one AI -- clicking the same hillside inside that window both get a valid           *
 *	answer, and the second building lands inside the first.  A reservation is what closes that        *
 *	window, and it closes it on the server, where the sequence of accepted placements is the one      *
 *	that matters.                                                                                     *
 *                                                                                             *
 *	Steps 12-13 then answer "what changed".  Nav is rebuilt because grading moved the ground and     *
 *	a new structure is a new obstacle; power and production capability are asked of the services     *
 *	that already own them rather than recomputed here; spatial registration is what the              *
 *	construction service's own completed building already does through `PhysicsSceneClass`, so       *
 *	this file confirms it rather than duplicating it.                                                 *
 *                                                                                             *
 *	What an expansion region is.  Not a new kind of base: a named circle around a completed          *
 *	forward structure, with the team that owns it and the structures inside it, so that the           *
 *	Commander UI and the AI can both ask "where are my forward positions" without either of them     *
 *	walking the whole object list and inventing its own clustering rule.  It carries a stable id     *
 *	built the same way every other Phase 11-17 record's is, so Phase 18's journal and Phase 19's     *
 *	snapshot can name one.                                                                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef EXPANSIONREGIONREGISTRY_H
#define EXPANSIONREGIONREGISTRY_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	One reserved construction footprint: ground an accepted placement has claimed and no later
**	placement may overlap, until the building that claimed it exists or the claim is released.
*/
class ConstructionReservation
{
public:
	ConstructionReservation(void);

	bool	operator == (const ConstructionReservation & that) const;
	bool	operator != (const ConstructionReservation & that) const	{ return !(*this == that); }

	StringClass		Id;					// stable id, Section 3B.5 -- "reservation_<ordinal>"
	StringClass		DescriptorName;
	Vector3			Position;
	float				Radius;				// the same footprint disc the validator tested
	float				RotationDegrees;
	int				Team;
	int				SequenceId;			// the placement sequence this reservation belongs to
	int				BuildingId;			// -1 until construction has been started against it
};


/*
**	A forward base or expansion: step 13's own metadata, as one record per cluster.
*/
class ExpansionRegion
{
public:
	ExpansionRegion(void);

	bool	operator == (const ExpansionRegion & that) const;
	bool	operator != (const ExpansionRegion & that) const	{ return !(*this == that); }

	StringClass		Id;					// stable id -- "expansion_<team>_<ordinal>"
	int				Team;
	Vector3			Center;				// the mean of the structures in it
	float				Radius;				// far enough to contain every one of them, plus its own footprint
	int				StructureCount;

	//	True when this region contains no structure the Commander started the match with -- what
	//	makes it a forward position rather than the main base having grown.  Decided by distance
	//	from the team's own first region, which is the only real "home" signal available here.
	bool				IsForward;
};


class ExpansionRegionRegistry
{
public:

	static void		Init(void);
	static void		Shutdown(void);
	static void		Clear(void);

	/*
	**	Step 10.  Claim the ground an accepted placement stands on.
	**
	**	Returns false, and reserves nothing, when the footprint overlaps a live reservation --
	**	which is the race this exists to lose safely.  The caller is the server, after
	**	`CommanderPlacementValidator::Validate_Server_Request` has accepted, and before
	**	`BuildingConstructionService::Begin_Construction`.
	*/
	static bool		Reserve(const char * descriptor_name,const Vector3 & position,
									float rotation_degrees,int team,int sequence_id,
									StringClass * reservation_id_out);

	//	Does a footprint of this radius here overlap a live reservation?  The same question
	//	Reserve asks itself, exposed so a placement validator can refuse before accepting.
	static bool		Is_Reserved(const Vector3 & position,float radius);

	//	Bind a started construction to its reservation, so the reservation can be released when
	//	that building exists rather than on a timer.
	static bool		Bind_Building(const char * reservation_id,int building_id);

	static bool		Release(const char * reservation_id);
	static bool		Release_By_Building(int building_id);

	static int		Get_Reservation_Count(void);
	static bool		Get_Reservation(int index,ConstructionReservation * out);

	/*
	**	Step 12.  What a completed construction changes.
	**
	**	Rebuilds navigation (the ground moved and there is a new obstacle), refreshes the
	**	owning team's power grid, and rebuilds the chokepoint set the placement rules cache.
	**	Releases the building's reservation last, so that nothing can claim the same ground
	**	between the building appearing and the world agreeing that it has.
	**
	**	`building_id` may be -1 for a completion this registry never reserved, which is a legal
	**	case: a map-authored building is not a Commander expansion.
	*/
	static bool		Note_Construction_Complete(int building_id,const char * descriptor_name,
															 const Vector3 & position,int team,
															 StringClass * report_out);

	//	How many of each update the completion sweep actually performed -- what makes "nav was
	//	rebuilt" observable rather than merely claimed.
	static int		Get_Navigation_Rebuild_Count(void);
	static int		Get_Power_Refresh_Count(void);
	static int		Get_Completion_Count(void);

	/*
	**	Step 13.  Rebuild the expansion-region metadata from the structures that exist now.
	**
	**	Clustered by proximity rather than by who built what: two structures within
	**	`cluster_radius` of each other are the same forward position, which is what a player
	**	means by one.  Deterministic in the order the registry holds completions, so a server
	**	and a client running the same sequence produce the same regions with the same ids.
	*/
	static bool		Rebuild_Regions(float cluster_radius,StringClass * error_out);

	static int		Get_Region_Count(void);
	static bool		Get_Region(int index,ExpansionRegion * out);
	static int		Get_Region_Count_For_Team(int team);
	static bool		Find_Region_At(const Vector3 & position,ExpansionRegion * out);

	static unsigned int	Get_Checksum(void);

private:

	//	One completed structure this registry knows about, which is what Rebuild_Regions
	//	clusters.  Kept rather than re-queried because a region's identity must not change
	//	because a building was destroyed and the clustering reshuffled around the gap.
	class CompletedStructure
	{
	public:
		CompletedStructure(void) : Position(0.0f,0.0f,0.0f),Radius(0.0f),Team(-1),BuildingId(-1) { }

		bool	operator == (const CompletedStructure & that) const
		{
			return (BuildingId == that.BuildingId) && (Team == that.Team) &&
					 (Position == that.Position) && (DescriptorName == that.DescriptorName);
		}
		bool	operator != (const CompletedStructure & that) const	{ return !(*this == that); }

		StringClass	DescriptorName;
		Vector3		Position;
		float			Radius;
		int			Team;
		int			BuildingId;
	};

	static int		NextReservationOrdinal;
	static int		NavigationRebuildCount;
	static int		PowerRefreshCount;
	static int		CompletionCount;
	static unsigned int	Checksum;

	static DynamicVectorClass<ConstructionReservation>	Reservations;
	static DynamicVectorClass<CompletedStructure>		Completed;
	static DynamicVectorClass<ExpansionRegion>			Regions;
};

#endif	// EXPANSIONREGIONREGISTRY_H
