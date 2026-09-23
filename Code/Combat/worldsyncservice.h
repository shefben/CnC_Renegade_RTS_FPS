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
 *                     $Archive:: /Commando/Code/Combat/worldsyncservice.h                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 19 steps 8-16 and 19 -- late join, regional repair, metrics.     *
 *                                                                                             *
 *	The handshake is the interesting part, and it is interesting because of what it refuses.  A    *
 *	late joiner is told the generator version, the profile hash, the content hash, the manifest     *
 *	revision, how many chunk snapshots exist and what the current world revision is -- and if the    *
 *	first four do not match what the client has, the join is refused there, before a single           *
 *	snapshot byte moves.  A client that regenerates a different baseline and then patches it with     *
 *	the server's snapshots does not end up with the server's world; it ends up with a world that       *
 *	is wrong everywhere the snapshots did not happen to cover, and no checksum after the fact can      *
 *	tell it which parts.                                                                               *
 *                                                                                             *
 *	Regional repair is the same idea one chunk at a time.  Per-chunk checksums let a client say       *
 *	"chunk 7,3 disagrees" instead of "something disagrees", and the server answers with that one       *
 *	chunk's snapshot.  That is the difference between a repair and a reconnect.                         *
 *                                                                                             *
 *	This file is a protocol, not a transport.  Every message is a byte block a caller writes and      *
 *	reads; nothing here opens a socket or knows a packet type exists.  That is deliberate -- it        *
 *	lets the whole join/verify/repair sequence be exercised end to end without a network, which is     *
 *	how the self-check proves it.                                                                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSYNCSERVICE_H
#define WORLDSYNCSERVICE_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "wwstring.h"


/*
**	Step 8's handshake, field for field as that step lists it.
**
**	Hashes rather than the things they identify: a profile is kilobytes of JSON and a content set
**	is a whole install, and neither needs to travel for a client to discover it has the wrong one.
*/
class WorldSyncHandshakeClass
{
public:

	enum { CURRENT_HANDSHAKE_VERSION = 1 };

	WorldSyncHandshakeClass(void);

	bool	operator == (const WorldSyncHandshakeClass & that) const;
	bool	operator != (const WorldSyncHandshakeClass & that) const	{ return !(*this == that); }

	unsigned int	HandshakeVersion;
	unsigned int	GeneratorVersion;		// ProceduralWorldProfile::CURRENT_GENERATOR_VERSION
	unsigned int	ProfileHash;			// the profile that generated this world
	unsigned int	ContentHash;			// the asset/definition set the server is running
	unsigned int	ManifestRevision;		// the generation manifest's own revision
	unsigned int	WorldRevision;			// where the journal is now
	unsigned int	WorldChecksum;			// what the client should arrive at

	/*
	**	The revision the snapshot set was taken at, which is not always the current one: a server
	**	that has applied operations since its last compaction sends snapshots at revision R and
	**	the operations that followed.  A client adopts this, then replays those operations, and
	**	arrives at `WorldRevision` by the same arithmetic the server did.
	*/
	unsigned int	SnapshotRevision;

	int				SnapshotCount;			// the snapshot inventory's size
	int				SnapshotBytes;
	int				OperationCount;		// post-snapshot operations still in the log

	int		Get_Block_Size(void) const;
	bool		Write_Block(unsigned char * buffer,int max_bytes,int * used_out) const;
	bool		Read_Block(const unsigned char * buffer,int bytes,int * consumed_out);
};


//	Why a late join was refused, or how far it got.
enum WorldSyncResultType
{
	WORLD_SYNC_OK = 0,
	WORLD_SYNC_BAD_HANDSHAKE_VERSION,
	WORLD_SYNC_GENERATOR_MISMATCH,
	WORLD_SYNC_PROFILE_MISMATCH,
	WORLD_SYNC_CONTENT_MISMATCH,
	WORLD_SYNC_MANIFEST_MISMATCH,
	WORLD_SYNC_NO_BASELINE,
	WORLD_SYNC_SNAPSHOT_REFUSED,
	WORLD_SYNC_OPERATION_REFUSED,
	WORLD_SYNC_CHECKSUM_MISMATCH,
	WORLD_SYNC_NOT_JOINING,

	WORLD_SYNC_RESULT_COUNT
};

const char *	World_Sync_Result_Name(WorldSyncResultType result);


/*
**	Step 19's metrics.  Counted rather than estimated -- every number here is the size of something
**	that was actually written or read.
*/
struct WorldSyncMetrics
{
	WorldSyncMetrics(void) :
		LateJoinBytes(0),SnapshotBytes(0),OperationBytes(0),RepairBytes(0),
		GenerationTimeMilliseconds(0.0f),
		JoinCount(0),RepairCount(0),MismatchCount(0) { }

	int	LateJoinBytes;			// handshake + snapshots + operations for one join
	int	SnapshotBytes;
	int	OperationBytes;
	int	RepairBytes;
	float	GenerationTimeMilliseconds;

	int	JoinCount;
	int	RepairCount;
	int	MismatchCount;
};


class WorldSyncService
{
public:

	static void		Init(void);
	static void		Shutdown(void);
	static void		Reset_Metrics(void);

	/*
	**	Server side.  What a joining client is told, built from the systems that own each answer --
	**	nothing here is cached, because a handshake describing a world the server has moved past is
	**	worse than no handshake at all.
	**
	**	`profile_hash`, `content_hash` and `manifest_revision` are supplied by the caller: this
	**	service does not own a profile or an install, and inventing a hash for either would be a
	**	number that means nothing.  `ProceduralWorldManager` and `WorldGenerationManifest` are the
	**	real owners and the real callers.
	*/
	static bool		Build_Handshake(unsigned int profile_hash,unsigned int content_hash,
											 unsigned int manifest_revision,
											 WorldSyncHandshakeClass * out);

	/*
	**	Steps 9-12, client side, as a small explicit state machine rather than one call.
	**
	**	Explicit because each step can fail for its own reason and a caller needs to know which:
	**	"the baseline did not match" and "a snapshot was refused" lead to completely different
	**	recoveries, and a single Join() returning false would hide that.
	**
	**	Begin_Join checks the four identity fields and nothing else.  The client's own baseline
	**	regeneration happens between Begin_Join and the first Apply_Snapshot_Block -- this service
	**	never generates terrain, it only says whether what the client generated can be patched into
	**	the server's world.
	*/
	static WorldSyncResultType	Begin_Join(const WorldSyncHandshakeClass & server,
														const WorldSyncHandshakeClass & client,
														StringClass * detail_out);

	static bool		Is_Joining(void);

	static WorldSyncResultType	Apply_Snapshot_Block(const unsigned char * buffer,int bytes,
																	 int * consumed_out);
	static WorldSyncResultType	Apply_Operation_Block(const unsigned char * buffer,int bytes,
																	  int * consumed_out);

	/*
	**	Step 12.  Recompute and compare against what the handshake promised.  Succeeds or names the
	**	first chunk that disagrees, which is the input to step 13.
	*/
	static WorldSyncResultType	Finish_Join(StringClass * detail_out);

	/*
	**	Steps 13-15.  Regional mismatch, request, repair, reverify.
	**
	**	`Find_Mismatched_Chunks` is the client's half: compare every local chunk checksum against
	**	the inventory the server sent and list the ones that differ.  `Build_Repair_Block` is the
	**	server's: one chunk's snapshot and nothing else.  `Apply_Repair_Block` is the client's
	**	again, and it reverifies that chunk rather than trusting the block it just applied.
	*/
	static int		Find_Mismatched_Chunks(DynamicVectorClass<int> * chunk_x_out,
													  DynamicVectorClass<int> * chunk_y_out);

	static bool		Build_Repair_Block(int px,int py,unsigned char * buffer,int max_bytes,int * used_out);
	static WorldSyncResultType	Apply_Repair_Block(const unsigned char * buffer,int bytes,
																	StringClass * detail_out);

	/*
	**	The inventory a client checks its own chunks against.  Populated from the server's
	**	snapshot set; a chunk with no entry is one the server considers pristine, and a client
	**	whose copy of it differs is a client whose baseline regeneration went wrong -- which is a
	**	repairable condition and is reported as one.
	*/
	static bool		Set_Expected_Chunk_Checksum(int px,int py,unsigned int checksum);
	static bool		Get_Expected_Chunk_Checksum(int px,int py,unsigned int * checksum_out);
	static void		Clear_Expected_Checksums(void);
	static int		Get_Expected_Checksum_Count(void);

	/*
	**	Step 16.  Dynamic building/AI state may only be loaded once the world revision those
	**	objects were recorded against exists locally.  A gate rather than a loader: this service
	**	does not own building state, it owns the answer to "is the world ready for it yet".
	*/
	static bool		Is_World_Ready_For_Revision(unsigned int required_revision);

	static const WorldSyncMetrics &	Get_Metrics(void);
	static void		Note_Generation_Time(float milliseconds);

private:

	static bool		Joining;
	static WorldSyncHandshakeClass	ServerHandshake;
	static WorldSyncMetrics			Metrics;

	//	px + py*ExpectedCountX, parallel to a validity flag so that "no entry" and "entry zero" are
	//	different answers.
	static int		ExpectedCountX;
	static int		ExpectedCountY;
	static DynamicVectorClass<unsigned int>	ExpectedChecksum;
	static DynamicVectorClass<int>				ExpectedValid;
};

#endif	// WORLDSYNCSERVICE_H
