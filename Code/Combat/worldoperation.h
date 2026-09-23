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
 *                     $Archive:: /Commando/Code/Combat/worldoperation.h                      $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 18 steps 1-4 -- the versioned WorldOperation envelope.           *
 *                                                                                             *
 *	Every runtime change to the ground travels as one of these and nothing else.  That is the     *
 *	whole design: a peer never receives heights, it receives the request that produced them and    *
 *	performs it itself.  A crater is thirty-odd bytes whatever its radius, and two peers that       *
 *	ran the same ordered list of these from the same base terrain have the same terrain, which      *
 *	is what makes a checksum comparison meaningful rather than hopeful.                             *
 *                                                                                             *
 *	Versioned because it is a wire format and a save format at once.  `FormatVersion` travels        *
 *	with every operation rather than once per stream, so a journal that outlives a build can be      *
 *	read back operation by operation instead of being all-or-nothing.                                 *
 *                                                                                             *
 *	This file deliberately contains no terrain code.  It defines what an operation *is*, what a       *
 *	well-formed one looks like, and how to write one down; `WorldOperationJournal` is what             *
 *	decides whether one may run, runs it, and records that it did.  Keeping the two apart is what      *
 *	lets the envelope be validated on a client that will never apply anything.                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDOPERATION_H
#define WORLDOPERATION_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


//	Step 3's own list, in the order that step names them.
enum WorldOperationType
{
	WORLD_OP_RAISE = 0,
	WORLD_OP_LOWER,
	WORLD_OP_FLATTEN,
	WORLD_OP_SMOOTH,
	WORLD_OP_GRADE_FOUNDATION,
	WORLD_OP_CARVE_ROAD,
	WORLD_OP_CARVE_CHANNEL,
	WORLD_OP_CREATE_CRATER,
	WORLD_OP_RECLAIM_SHALLOW_WATER,
	WORLD_OP_RESTORE_REGION,

	WORLD_OPERATION_TYPE_COUNT
};

const char *	World_Operation_Type_Name(WorldOperationType type);


/*
**	Step 4's "render / collision / nav / water-road dependencies", as one mask per operation type
**	rather than a chain of ifs at the apply site.  A dependency named here is one the journal must
**	drive to its commit point before the world revision may advance (step 12).
*/
enum WorldOperationDependencyType
{
	WORLD_OP_DEPENDS_RENDER		= 0x01,
	WORLD_OP_DEPENDS_COLLISION	= 0x02,
	WORLD_OP_DEPENDS_NAV			= 0x04,
	WORLD_OP_DEPENDS_WATER		= 0x08,
	WORLD_OP_DEPENDS_ROADS		= 0x10,
	WORLD_OP_DEPENDS_MATERIALS	= 0x20
};

//	Which dependencies a type invalidates.  Static, per type, and the one place the answer lives.
unsigned int	World_Operation_Dependencies(WorldOperationType type);

/*
**	Does replay order change what this type does?
**
**	Step 6 says to reject stale requests "where ordering matters", which is not every type.  RAISE,
**	LOWER, SMOOTH and CREATE_CRATER are relative: each one's result depends on the ground it found,
**	so running it against a revision other than the one the submitter measured produces a different
**	world.  FLATTEN, GRADE_FOUNDATION, CARVE_ROAD, CARVE_CHANNEL, RECLAIM_SHALLOW_WATER and
**	RESTORE_REGION drive the ground to an absolute height, so a stale revision does not change
**	their outcome in their own region -- and the journal's own order still decides what happens
**	where two of them overlap.
*/
bool	World_Operation_Ordering_Matters(WorldOperationType type);


/*
**	Why an operation was refused.  One value each, so the reason a server logs and the reason a
**	replay reports are the same word.
*/
enum WorldOperationResultType
{
	WORLD_OP_RESULT_APPLIED = 0,
	WORLD_OP_RESULT_BAD_FORMAT_VERSION,
	WORLD_OP_RESULT_BAD_TYPE,
	WORLD_OP_RESULT_BAD_PARAMETERS,
	WORLD_OP_RESULT_NO_TERRAIN,
	WORLD_OP_RESULT_NO_JOURNAL,
	WORLD_OP_RESULT_STALE_REVISION,
	WORLD_OP_RESULT_CHUNKS_RESERVED,
	WORLD_OP_RESULT_PROTECTED_REGION,
	WORLD_OP_RESULT_BOUNDARY_BAND,
	WORLD_OP_RESULT_CRATER_LIMIT,
	WORLD_OP_RESULT_RATE_LIMITED,
	WORLD_OP_RESULT_DISABLED,

	WORLD_OPERATION_RESULT_COUNT
};

const char *	World_Operation_Result_Name(WorldOperationResultType result);


/*
**	One operation.  Fixed fields for every type plus a polyline for the two that need one; a type
**	that does not use a field leaves it at its default and the validator says so, rather than each
**	type getting its own struct and the journal getting a union to walk.
*/
class WorldOperationClass
{
public:

	//	Bumped when this struct's own wire layout changes.  Version 1 is this layout.
	enum { CURRENT_FORMAT_VERSION = 1 };

	//	A polyline longer than this is refused rather than truncated: an operation has to fit in a
	//	bounded packet, and a road carved in one call across a whole map is not a runtime mutation.
	enum { MAX_PATH_POINTS = 64 };

	WorldOperationClass(void);

	bool	operator == (const WorldOperationClass & that) const;
	bool	operator != (const WorldOperationClass & that) const	{ return !(*this == that); }

	unsigned int			FormatVersion;
	unsigned int			OperationId;		// assigned by the journal, monotonic, never reused
	unsigned int			BaseRevision;		// the world revision the submitter measured against
	WorldOperationType	Type;
	int						Team;					// PLAYERTYPE_*; -1 for an operation nobody owns
	float						TimeSeconds;		// server time, for the rate limits in step 15

	/*
	**	`Center` is a point, and for the three types that drive the ground to an absolute height --
	**	FLATTEN, GRADE_FOUNDATION and RECLAIM_SHALLOW_WATER -- its Z *is* that height.  One field
	**	rather than a separate `TargetHeight` that would have to be kept in step with it.
	*/
	Vector3					Center;
	float						Radius;
	float						Depth;				// RAISE/LOWER/CREATE_CRATER: the magnitude of the height
														//	change, always stated positive -- the sign belongs to
														//	the type, so a LOWER with a negative depth is malformed
														//	rather than a quiet RAISE.  CARVE_CHANNEL: channel
														//	depth.  Unused by every other type.
	float						Width;				// CARVE_ROAD/CARVE_CHANNEL: the strip's own width
	float						Blend;				// FLATTEN: 0..1 pull.  SMOOTH: 0..1 strength.
														//	CARVE_ROAD: 0..1 grade blend.
														//	GRADE_FOUNDATION: the apron radius, which is why it
														//	is validated against Radius rather than against 1.

	DynamicVectorClass<Vector3>	Path;			// CARVE_ROAD/CARVE_CHANNEL only

	/*
	**	Step 4's "checksum output".  A Section 3B.4-quantized CRC over every field above, so that
	**	an operation which travelled intact can be told from one that did not, and so that two
	**	journals holding the same operations hold the same number.  Computed, never carried in
	**	from outside: Compute_Checksum is what Get_Checksum returns.
	*/
	unsigned int	Compute_Checksum(void) const;

	/*
	**	Is this a well-formed operation of its own type?  Parameters only -- no terrain, no world
	**	revision, no protected regions, none of which this file can see.  A client with no world
	**	loaded can still reject a malformed request with this.
	*/
	WorldOperationResultType	Validate_Parameters(void) const;

	//	The plan-view circle this operation touches, whatever its type: the union of its own
	//	region and, for a polyline type, every point on the path widened by half the strip.
	//	Used for chunk reservation and protected-region tests, so it must never be optimistic.
	bool	Get_Affected_Bounds(Vector3 * center_out,float * radius_out) const;

	/*
	**	Step 14 -- compact replication.  A fixed header plus three floats per path point; nothing
	**	here scales with the size of the region an operation changes, which is the whole point.
	*/
	int		Get_Block_Size(void) const;
	bool		Write_Block(unsigned char * buffer,int max_bytes,int * used_out) const;
	bool		Read_Block(const unsigned char * buffer,int bytes,int * consumed_out);
};

#endif	// WORLDOPERATION_H
