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
 *                     $Archive:: /Commando/Code/Combat/worldsnapshot.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 19 steps 1-7 -- per-chunk checksums and the compact snapshot.    *
 *                                                                                             *
 *	The gate this exists for says a late client must reconstruct the current world "without        *
 *	downloading complete raw world meshes".  So a snapshot never contains a chunk; it contains      *
 *	the difference between a chunk and the chunk a client can already regenerate for itself from     *
 *	the seed and profile.  On a pristine chunk that difference is nothing and the snapshot is a       *
 *	header.  On a chunk somebody cratered it is the handful of vertices the crater moved.              *
 *                                                                                             *
 *	That is the whole format, and it is the reason the format is worth having: "only authoritative    *
 *	mutable/generated state needed beyond baseline regeneration" is step 3's own wording, and the      *
 *	literal reading of it -- store the vertices that differ, and only those -- is both the simplest    *
 *	thing to implement and the smallest thing to send.                                                  *
 *                                                                                             *
 *	Checksums come first in this file because a snapshot is worthless without them.  A client that     *
 *	cannot tell which of its chunks is wrong has to re-request all of them, which is the full world     *
 *	download the gate forbids.  Per-chunk checksums are what make step 13's regional repair a           *
 *	request for one chunk instead of a request for everything.                                           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSNAPSHOT_H
#define WORLDSNAPSHOT_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"


/*
**	One chunk's worth of "what an operation did to this ground that regenerating it would not".
**
**	Sparse by construction: `VertexIndex` holds the offsets into the heightfield that differ from
**	the baseline, and `QuantizedHeight` holds what they should be, to the millimetre.  A pristine
**	chunk carries no entries at all.
*/
class WorldChunkSnapshotClass
{
public:

	//	Bumped when this record's own wire layout changes.  Version 1 is this layout.
	enum { CURRENT_SNAPSHOT_VERSION = 1 };

	WorldChunkSnapshotClass(void);

	bool	operator == (const WorldChunkSnapshotClass & that) const;
	bool	operator != (const WorldChunkSnapshotClass & that) const	{ return !(*this == that); }

	void	Reset(void);

	unsigned int	SnapshotVersion;
	unsigned int	Revision;			// the world revision this chunk was captured at
	int				ChunkX;
	int				ChunkY;
	unsigned int	ChunkChecksum;		// what the chunk should checksum to once applied

	DynamicVectorClass<int>	VertexIndex;		// iy * vertex_count_x + ix
	DynamicVectorClass<int>	QuantizedHeight;	// stored height * 1000, same index

	int		Get_Vertex_Count(void) const	{ return VertexIndex.Count(); }

	int		Get_Block_Size(void) const;
	bool		Write_Block(unsigned char * buffer,int max_bytes,int * used_out) const;
	bool		Read_Block(const unsigned char * buffer,int bytes,int * consumed_out);
};


/*
**	Step 6's compaction thresholds.  Three of them because an operation log can become a problem
**	three different ways: too many operations to replay quickly, a log that has been accumulating
**	for a whole match, or one that has simply grown past what a late-join packet can carry.
*/
struct WorldSnapshotCompactionPolicy
{
	WorldSnapshotCompactionPolicy(void) :
		MaxOperations(256),MaxAgeSeconds(300.0f),MaxOperationBytes(16384) { }

	int	MaxOperations;			// > 0
	float	MaxAgeSeconds;			// > 0, measured on the operations' own timestamps
	int	MaxOperationBytes;	// > 0, the serialised size of the whole log
};


class WorldSnapshotService
{
public:

	static void		Init(void);
	static void		Shutdown(void);
	static void		Clear(void);

	/*
	**	Step 1.  A chunk's authoritative checksum inputs: its own vertex heights, quantized to the
	**	millimetre (Section 3B.4), plus the chunk registry's entry when one exists.
	**
	**	Heights and nothing else.  A chunk's materials, its foliage and its props are all either
	**	regenerated deterministically from the seed or owned by a system with its own state block;
	**	folding them in here would make a checksum that disagrees with itself depending on how far
	**	a client's own generation had got.
	*/
	static bool		Compute_Chunk_Checksum(int px,int py,unsigned int * checksum_out);

	/*
	**	Step 2.  The world-level composition: every chunk checksum in grid order, then the world
	**	revision and the journal checksum.
	**
	**	Order matters and is fixed by the grid, not by iteration order, so two peers compose the
	**	same number from the same chunks.  The revision is included because two worlds with
	**	identical ground that disagree about how they got there are not the same world for
	**	ordering purposes, and a late joiner needs to find that out at the handshake rather than at
	**	the first operation it cannot place.
	*/
	static bool		Compute_World_Checksum(unsigned int * checksum_out);

	//	Is this chunk different from the baseline the journal opened over?  What makes a snapshot
	//	set "the dirty chunks" rather than "every chunk".
	static bool		Is_Chunk_Dirty(int px,int py);

	/*
	**	Steps 3-5.  Capture one chunk, or every dirty chunk.
	**
	**	Requires an open `WorldOperationJournal` -- the baseline is the journal's, because the
	**	journal is what owns "the ground before anything happened to it".
	*/
	static bool		Capture_Chunk(int px,int py,WorldChunkSnapshotClass * out);
	static int		Capture_Dirty_Chunks(StringClass * error_out);

	static int		Get_Snapshot_Count(void);
	static bool		Get_Snapshot(int index,WorldChunkSnapshotClass * out);
	static bool		Find_Snapshot_For_Chunk(int px,int py,WorldChunkSnapshotClass * out);

	//	The whole captured set's serialised size -- step 19's "snapshot bytes".
	static int		Get_Snapshot_Bytes(void);

	/*
	**	Apply one chunk snapshot to the live terrain.  Used by a late joiner reconstructing the
	**	world (step 10) and by regional repair (step 15); the same function both times, because
	**	repairing one chunk and joining are the same operation over a different number of chunks.
	*/
	static bool		Apply_Snapshot(const WorldChunkSnapshotClass & snapshot);

	/*
	**	Rebuild what is derived from the ground, after the ground has been replaced out of band.
	**
	**	A client that reconstructs its world from snapshots gets heights and nothing else, but the
	**	chunk registry's slope and buildable classification is computed from those heights, and
	**	placement, navigation and the Commander's own rules all read it.  Left stale, a client
	**	plays on the right ground with the wrong idea of what may stand on it.
	**
	**	Explicitly *not* for the checksum's sake -- the chunk checksum reads heights only, on
	**	purpose (see Compute_Chunk_Checksum).  Capturing a snapshot therefore does not call this;
	**	applying one does.
	**
	**	Called once after a batch rather than once per snapshot: rebuilding the registry per chunk
	**	during a join would rebuild the whole thing once for every chunk received.  Rebuilt against
	**	the cap it was built with, so no chunk nobody touched changes classification.
	*/
	static void		Refresh_Derived_State(void);

	/*
	**	Steps 6-7.  Is the operation log past a threshold, and the rebase that folds it away.
	**
	**	`Compact` captures the dirty chunks first and only then rebases the journal, in that order
	**	for the obvious reason: rebasing makes every chunk pristine by definition, so a capture
	**	taken afterwards would be empty and the world's history would be gone with nothing holding
	**	it.
	*/
	static void		Set_Compaction_Policy(const WorldSnapshotCompactionPolicy & policy);
	static const WorldSnapshotCompactionPolicy &	Get_Compaction_Policy(void);

	static bool		Should_Compact(StringClass * reason_out);
	static bool		Compact(StringClass * report_out);

	static int		Get_Compaction_Count(void);

	/*
	**	Steps 17-18.  Developer/admin rollback to a previous authoritative snapshot.
	**
	**	Bounded: the most recent `MAX_ROLLBACK_POINTS` are kept and the oldest is dropped, because
	**	an unbounded history of a mutated world is the full-world storage this phase exists to
	**	avoid.  Deliberately not reachable from gameplay -- step 18 says so in as many words, and
	**	the only caller in this tree is a console command behind a WWDEBUG build.
	*/
	enum { MAX_ROLLBACK_POINTS = 8 };

	static bool		Mark_Rollback_Point(const char * label,StringClass * error_out);
	static int		Get_Rollback_Point_Count(void);
	static bool		Get_Rollback_Point(int index,StringClass * label_out,unsigned int * revision_out);
	static bool		Rollback_To(int index,StringClass * report_out);

private:

	//	One saved authoritative state: a whole-field height snapshot plus the revision it was at.
	//	Whole-field rather than sparse because a rollback point is not something that travels --
	//	it never leaves the server, so the reason to make it compact does not apply.
	class RollbackPointClass
	{
	public:
		RollbackPointClass(void) : Revision(0),VertexCountX(0),VertexCountY(0),Label("") { }

		bool	operator == (const RollbackPointClass & that) const
		{
			return (Revision == that.Revision) && (Label == that.Label) &&
					 (VertexCountX == that.VertexCountX) && (VertexCountY == that.VertexCountY);
		}
		bool	operator != (const RollbackPointClass & that) const	{ return !(*this == that); }

		unsigned int	Revision;
		int				VertexCountX;
		int				VertexCountY;
		StringClass		Label;
		DynamicVectorClass<float>	Heights;
	};

	static WorldSnapshotCompactionPolicy	Policy;
	static int		CompactionCount;

	static DynamicVectorClass<WorldChunkSnapshotClass>	Snapshots;
	static DynamicVectorClass<RollbackPointClass>		RollbackPoints;
};

#endif	// WORLDSNAPSHOT_H
