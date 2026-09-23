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
 *                     $Archive:: /Commando/Code/Combat/worldoperationjournal.h               $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 18 -- the world revision owner and the ordered operation journal. *
 *                                                                                             *
 *	One object owns three things that are really one thing: the monotonic world revision, the      *
 *	ordered list of operations that produced it, and the base terrain they were applied to.         *
 *	Splitting them would allow a revision that no list explains, or a list that no base can be       *
 *	replayed from, and either of those makes the whole phase's promise -- "save/replay produces       *
 *	the same checksum" -- unverifiable.                                                               *
 *                                                                                             *
 *	The base snapshot is the design decision worth naming.  `Begin` copies the heightfield as it      *
 *	stands, once, and every later question about the world is answered as "the base, plus these      *
 *	operations, in this order".  That gives RESTORE_REGION something real to restore from without    *
 *	an undo blob per operation, gives replay a defined starting point, and keeps replication          *
 *	compact -- a peer receives operations, never heights.                                              *
 *                                                                                             *
 *	Why the revision advances after the dependencies, not before.  Step 12 says a new revision may     *
 *	be committed "only after authoritative dependencies reach required commit point", and the          *
 *	reason is visible the moment you try it the other way: a peer that sees revision N+1 and asks      *
 *	the pathfinder a question is entitled to an answer about revision N+1's ground.  So the           *
 *	revision is the last thing to move, and a result that says `Committed` false says exactly which    *
 *	dependency did not reach its commit point.                                                          *
 *                                                                                             *
 *	What this file does not own: the ground.  Every mutation goes through `WorldTerrainSystem`, the   *
 *	same canonical owner Phase 17's grading service already defers to, and nothing here writes a       *
 *	height sample outside the base snapshot it took itself.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDOPERATIONJOURNAL_H
#define WORLDOPERATIONJOURNAL_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"
#include "worldoperation.h"


/*
**	Step 15's bounded crater rules, as one struct so that a profile, a console command and a test
**	all set the same four numbers.  Defaults are deliberately conservative: a crater is a weapon
**	effect, and the cost of one too small is cosmetic where the cost of one too large is a hole
**	through the map's topology.
*/
struct WorldOperationCraterLimits
{
	WorldOperationCraterLimits(void) :
		MaxRadius(8.0f),MaxDepth(2.5f),
		MaxCratersPerWindow(6),RateWindowSeconds(10.0f) { }

	float	MaxRadius;					// world units, > 0
	float	MaxDepth;					// world units, > 0
	int	MaxCratersPerWindow;		// > 0 -- how many craters may land inside one window
	float	RateWindowSeconds;		// > 0 -- the window, measured on the operation's own timestamp
};


/*
**	What happened to one submitted operation.  Reported whether it was applied or refused, and
**	carrying which dependencies actually reached their commit point -- because "nav was rebuilt"
**	is a claim, and a bit that the rebuild set is evidence.
*/
struct WorldOperationApplyResult
{
	WorldOperationApplyResult(void) :
		Result(WORLD_OP_RESULT_APPLIED),Committed(false),
		OperationId(0),Revision(0),
		DependenciesRequired(0),DependenciesSatisfied(0),
		AffectedChunkCount(0),OperationChecksum(0),TerrainChecksum(0),Detail("") { }

	bool	Is_Applied(void) const	{ return Result == WORLD_OP_RESULT_APPLIED; }

	WorldOperationResultType	Result;
	bool				Committed;					// the world revision advanced
	unsigned int	OperationId;
	unsigned int	Revision;					// the revision after this operation, or before it if refused
	unsigned int	DependenciesRequired;	// WORLD_OP_DEPENDS_* bits
	unsigned int	DependenciesSatisfied;
	int				AffectedChunkCount;
	unsigned int	OperationChecksum;
	unsigned int	TerrainChecksum;			// after the operation; 0 when nothing was applied
	StringClass		Detail;						// which protected feature, which dependency, etc.
};


class WorldOperationJournal
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	/*
	**	Step 2.  Open a journal over the terrain as it stands now: snapshot it, clear the
	**	operation list, and set the world revision to 1.
	**
	**	Revision 1 rather than 0 on purpose -- 0 is "no journal", and an operation submitted
	**	against revision 0 is a submitter that never read one, which is a different mistake from
	**	a stale one and deserves a different answer.
	**
	**	Requires WorldTerrainSystem::Has_Terrain().
	*/
	static bool		Begin(StringClass * error_out);
	static void		Close(void);
	static bool		Is_Open(void)	{ return Open; }

	static unsigned int	Get_Revision(void)			{ return Revision; }
	static unsigned int	Get_Next_Operation_Id(void)	{ return NextOperationId; }

	/*
	**	Steps 5-14.  Validate, reserve, apply, invalidate, commit, append.
	**
	**	`operation`'s OperationId is assigned here and returned in the result: an id a submitter
	**	chose could collide with one the server already issued, and the server is the only thing
	**	that knows the whole order.  Everything else in the envelope is the submitter's.
	*/
	static WorldOperationApplyResult	Submit(const WorldOperationClass & operation);

	/*
	**	Validation without application, for a client previewing an operation it has not sent and
	**	for a server that wants to answer "would this be accepted" without accepting it.  Exactly
	**	the checks Submit runs before it touches anything, and nothing else.
	*/
	static WorldOperationResultType	Validate(const WorldOperationClass & operation,StringClass * detail_out);

	static int		Get_Operation_Count(void);
	static bool		Get_Operation(int index,WorldOperationClass * out);

	/*
	**	Step 5's transaction reservation, exposed rather than kept private to Submit.
	**
	**	Submit holds the affected chunks from before the mutation until after the last dependency
	**	has committed, which is the window in which a second operation must not touch them.  A
	**	caller performing a multi-operation transaction of its own reserves them for longer.
	*/
	static bool		Reserve_Chunks(const Vector3 & center,float radius,int * reserved_count_out);
	static void		Release_All_Chunks(void);
	static bool		Is_Chunk_Reserved(int px,int py);
	static int		Get_Reserved_Chunk_Count(void);

	/*
	**	Step 16's replay.  Restore the base snapshot, re-apply every journalled operation in
	**	order, and rebuild.  The world revision is left where it was: replay reproduces a state,
	**	it does not invent history.
	**
	**	This is the one function that makes "save/replay produces the same checksum" a statement
	**	about running code: take Get_Terrain_Checksum, replay, take it again.
	*/
	static bool		Replay(StringClass * report_out);

	/*
	**	Phase 19 steps 6-7 -- operation-log compaction.
	**
	**	Fold everything applied so far into the base snapshot and empty the operation list, keeping
	**	the world revision exactly where it is.  After this the world is still "base plus these
	**	operations in this order"; there are simply no operations yet, and the base is now the
	**	ground as it stands.
	**
	**	Everything or nothing, deliberately.  Folding only the first N operations would be wrong:
	**	the base would then have to be the ground as it was after operation N, and the ground as it
	**	is includes every later one, so there is nothing to copy it from without replaying.  A
	**	compaction point is a moment, not a prefix.
	*/
	static bool		Rebase(StringClass * error_out);

	/*
	**	The base snapshot, for a caller that needs to know what a chunk looked like before any
	**	operation touched it -- Phase 19's dirty-chunk detection is the one that does.  Heights are
	**	the heightfield's own stored form, relative to its origin.  False when no journal is open.
	*/
	static bool		Get_Base_Height(int ix,int iy,float * height_out);
	static bool		Get_Base_Dimensions(int * count_x_out,int * count_y_out);

	/*
	**	Phase 19 step 9-11 -- take the authority's revision as this world's own.
	**
	**	A late joiner regenerates the baseline itself and then patches it with snapshots, so its
	**	ground reaches the server's without a single operation passing through this journal.  Its
	**	revision would otherwise stay at 1 while its ground is at the server's revision N, and that
	**	is not a cosmetic disagreement: step 16 gates building and AI state on the revision, and a
	**	client that under-reports it would refuse state it is actually ready for -- or, worse,
	**	submit an operation against revision 1 that the server has long passed.
	**
	**	Deliberately not "set the revision to anything": the only legitimate caller is the late-join
	**	path, adopting a number an authority sent, and it is refused outright unless a journal is
	**	open to adopt it into.
	*/
	static bool		Adopt_Revision(unsigned int revision,StringClass * error_out);

	/*
	**	A Section 3B.4-quantized CRC over the journal itself -- every operation's own checksum, in
	**	order -- and one over the heightfield the journal produced.  The first answers "do two
	**	peers hold the same history", the second "did that history produce the same ground".
	**	Both are needed: identical journals that produced different ground is precisely the bug
	**	this phase exists to make detectable.
	*/
	static unsigned int	Get_Journal_Checksum(void);
	static unsigned int	Get_Terrain_Checksum(void);

	//	Step 15.
	static void		Set_Crater_Limits(const WorldOperationCraterLimits & limits);
	static const WorldOperationCraterLimits &	Get_Crater_Limits(void);

	/*
	**	Step 3's "RECLAIM_SHALLOW_WATER if enabled".  Off by default: reclaiming water changes a
	**	map's navigable area, which is a thing a profile opts into rather than a thing a weapon
	**	can do by accident.
	*/
	static void		Set_Shallow_Water_Reclamation_Enabled(bool enabled);
	static bool		Is_Shallow_Water_Reclamation_Enabled(void);

	//	How deep water may be and still be reclaimable.  Beyond this it is not shallow, and
	//	filling it is terraforming rather than reclamation.
	static void		Set_Max_Reclaimable_Depth(float depth);
	static float	Get_Max_Reclaimable_Depth(void);

	//	Bookkeeping a caller can observe rather than infer.
	static int		Get_Applied_Count(void);
	static int		Get_Rejected_Count(void);
	static int		Get_Navigation_Rebuild_Count(void);

private:

	static WorldOperationResultType	Validate_Internal(const WorldOperationClass & operation,
																		StringClass * detail_out,
																		Vector3 * center_out,float * radius_out);

	//	Applies the terrain mutation only.  Everything around it -- validation, reservation,
	//	dependencies, the revision, the journal -- is Submit's.  Shared with Replay, which is what
	//	guarantees a replayed operation does exactly what the original one did.
	static bool		Apply_Mutation(const WorldOperationClass & operation);

	//	Drives each required dependency to its commit point, returning the bits that got there.
	static unsigned int	Satisfy_Dependencies(unsigned int required,StringClass * detail_out);

	static bool		Restore_Base_Heights(void);

	static bool			Open;
	static unsigned int	Revision;
	static unsigned int	NextOperationId;

	static int			AppliedCount;
	static int			RejectedCount;
	static int			NavigationRebuildCount;

	static bool			ShallowWaterReclamationEnabled;
	static float		MaxReclaimableDepth;

	static WorldOperationCraterLimits	CraterLimits;

	//	The base snapshot: the heightfield's own values at Begin, with the shape it had then.  A
	//	terrain resized under an open journal invalidates it, which Restore_Base_Heights detects
	//	rather than papers over.
	static int			BaseVertexCountX;
	static int			BaseVertexCountY;
	static DynamicVectorClass<float>	BaseHeights;

	static DynamicVectorClass<WorldOperationClass>	Operations;

	//	px + py*count_x over the chunk registry, non-zero while reserved.
	static int			ReservedCountX;
	static int			ReservedCountY;
	static DynamicVectorClass<int>	ReservedChunks;
};

#endif	// WORLDOPERATIONJOURNAL_H
