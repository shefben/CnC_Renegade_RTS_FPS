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
 *                     $Archive:: /Commando/Code/Combat/worldsyncservice.cpp                  $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldsyncservice.h"

#include "proceduralworldprofile.h"
#include "worldoperation.h"
#include "worldoperationjournal.h"
#include "worldsnapshot.h"
#include "worldterrainsystem.h"


bool	WorldSyncService::Joining	= false;
WorldSyncHandshakeClass	WorldSyncService::ServerHandshake;
WorldSyncMetrics			WorldSyncService::Metrics;

int	WorldSyncService::ExpectedCountX	= 0;
int	WorldSyncService::ExpectedCountY	= 0;
DynamicVectorClass<unsigned int>	WorldSyncService::ExpectedChecksum;
DynamicVectorClass<int>				WorldSyncService::ExpectedValid;


const char *	World_Sync_Result_Name(WorldSyncResultType result)
{
	switch (result) {
		case WORLD_SYNC_OK:							return "ok";
		case WORLD_SYNC_BAD_HANDSHAKE_VERSION:	return "unsupported handshake version";
		case WORLD_SYNC_GENERATOR_MISMATCH:		return "generator version mismatch";
		case WORLD_SYNC_PROFILE_MISMATCH:		return "profile hash mismatch";
		case WORLD_SYNC_CONTENT_MISMATCH:		return "content hash mismatch";
		case WORLD_SYNC_MANIFEST_MISMATCH:		return "manifest revision mismatch";
		case WORLD_SYNC_NO_BASELINE:				return "no baseline generated";
		case WORLD_SYNC_SNAPSHOT_REFUSED:		return "snapshot refused";
		case WORLD_SYNC_OPERATION_REFUSED:		return "operation refused";
		case WORLD_SYNC_CHECKSUM_MISMATCH:		return "checksum mismatch";
		case WORLD_SYNC_NOT_JOINING:				return "no join in progress";
		default:											return "unknown";
	}
}


/***********************************************************************************************
**	The handshake record.
*/

WorldSyncHandshakeClass::WorldSyncHandshakeClass(void) :
	HandshakeVersion(CURRENT_HANDSHAKE_VERSION),
	GeneratorVersion(0),ProfileHash(0),ContentHash(0),ManifestRevision(0),
	WorldRevision(0),WorldChecksum(0),SnapshotRevision(0),
	SnapshotCount(0),SnapshotBytes(0),OperationCount(0)
{
}


bool WorldSyncHandshakeClass::operator == (const WorldSyncHandshakeClass & that) const
{
	return (HandshakeVersion == that.HandshakeVersion) &&
			 (GeneratorVersion == that.GeneratorVersion) &&
			 (ProfileHash == that.ProfileHash) && (ContentHash == that.ContentHash) &&
			 (ManifestRevision == that.ManifestRevision) &&
			 (WorldRevision == that.WorldRevision) && (WorldChecksum == that.WorldChecksum) &&
			 (SnapshotRevision == that.SnapshotRevision) &&
			 (SnapshotCount == that.SnapshotCount) && (SnapshotBytes == that.SnapshotBytes) &&
			 (OperationCount == that.OperationCount);
}


enum { WORLD_SYNC_HANDSHAKE_FIELDS = 11 };

static void Write_Word(unsigned char * at,unsigned int value)
{
	at[0] = (unsigned char)( value        & 0xFF);
	at[1] = (unsigned char)((value >>  8) & 0xFF);
	at[2] = (unsigned char)((value >> 16) & 0xFF);
	at[3] = (unsigned char)((value >> 24) & 0xFF);
}

static unsigned int Read_Word(const unsigned char * at)
{
	return	((unsigned int)at[0])			|
				(((unsigned int)at[1]) <<  8)	|
				(((unsigned int)at[2]) << 16)	|
				(((unsigned int)at[3]) << 24);
}


int WorldSyncHandshakeClass::Get_Block_Size(void) const
{
	return WORLD_SYNC_HANDSHAKE_FIELDS * 4;
}


bool WorldSyncHandshakeClass::Write_Block(unsigned char * buffer,int max_bytes,int * used_out) const
{
	if ((buffer == nullptr) || (max_bytes < Get_Block_Size())) return false;

	unsigned char * at = buffer;
	Write_Word(at,HandshakeVersion);			at += 4;
	Write_Word(at,GeneratorVersion);			at += 4;
	Write_Word(at,ProfileHash);				at += 4;
	Write_Word(at,ContentHash);				at += 4;
	Write_Word(at,ManifestRevision);			at += 4;
	Write_Word(at,WorldRevision);				at += 4;
	Write_Word(at,WorldChecksum);				at += 4;
	Write_Word(at,SnapshotRevision);			at += 4;
	Write_Word(at,(unsigned int)SnapshotCount);	at += 4;
	Write_Word(at,(unsigned int)SnapshotBytes);	at += 4;
	Write_Word(at,(unsigned int)OperationCount);	at += 4;

	if (used_out != nullptr) *used_out = (int)(at - buffer);
	return true;
}


bool WorldSyncHandshakeClass::Read_Block(const unsigned char * buffer,int bytes,int * consumed_out)
{
	if ((buffer == nullptr) || (bytes < (WORLD_SYNC_HANDSHAKE_FIELDS * 4))) return false;

	const unsigned char * at = buffer;
	HandshakeVersion	= Read_Word(at);	at += 4;
	GeneratorVersion	= Read_Word(at);	at += 4;
	ProfileHash			= Read_Word(at);	at += 4;
	ContentHash			= Read_Word(at);	at += 4;
	ManifestRevision	= Read_Word(at);	at += 4;
	WorldRevision		= Read_Word(at);	at += 4;
	WorldChecksum		= Read_Word(at);	at += 4;
	SnapshotRevision	= Read_Word(at);	at += 4;
	SnapshotCount		= (int)Read_Word(at);	at += 4;
	SnapshotBytes		= (int)Read_Word(at);	at += 4;
	OperationCount		= (int)Read_Word(at);	at += 4;

	if (consumed_out != nullptr) *consumed_out = (int)(at - buffer);
	return true;
}


/***********************************************************************************************
**	The service.
*/

void WorldSyncService::Init(void)
{
	Joining = false;
	ServerHandshake = WorldSyncHandshakeClass();
	Clear_Expected_Checksums();
	Reset_Metrics();
}


void WorldSyncService::Shutdown(void)	{ Init(); }

void WorldSyncService::Reset_Metrics(void)	{ Metrics = WorldSyncMetrics(); }

const WorldSyncMetrics &	WorldSyncService::Get_Metrics(void)	{ return Metrics; }

bool	WorldSyncService::Is_Joining(void)	{ return Joining; }

void	WorldSyncService::Note_Generation_Time(float milliseconds)
{
	if (milliseconds >= 0.0f) Metrics.GenerationTimeMilliseconds = milliseconds;
}


void WorldSyncService::Clear_Expected_Checksums(void)
{
	ExpectedCountX = 0;
	ExpectedCountY = 0;
	ExpectedChecksum.Delete_All();
	ExpectedValid.Delete_All();
}


int WorldSyncService::Get_Expected_Checksum_Count(void)
{
	int count = 0;
	for (int i = 0; i < ExpectedValid.Count(); i++) {
		if (ExpectedValid[i] != 0) count++;
	}
	return count;
}


static bool Ensure_Expected_Grid(int * count_x_out,int * count_y_out)
{
	if (!WorldTerrainSystem::Has_Terrain()) return false;

	int count_x = WorldTerrainSystem::Get_Patch_Count_X();
	int count_y = WorldTerrainSystem::Get_Patch_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	*count_x_out = count_x;
	*count_y_out = count_y;
	return true;
}


bool WorldSyncService::Set_Expected_Chunk_Checksum(int px,int py,unsigned int checksum)
{
	int count_x = 0, count_y = 0;
	if (!Ensure_Expected_Grid(&count_x,&count_y)) return false;
	if ((px < 0) || (py < 0) || (px >= count_x) || (py >= count_y)) return false;

	if (ExpectedChecksum.Count() != (count_x * count_y)) {
		ExpectedChecksum.Delete_All();
		ExpectedValid.Delete_All();
		ExpectedChecksum.Resize(count_x * count_y);
		ExpectedValid.Resize(count_x * count_y);
		for (int i = 0; i < (count_x * count_y); i++) {
			ExpectedChecksum.Add(0);
			ExpectedValid.Add(0);
		}
		ExpectedCountX = count_x;
		ExpectedCountY = count_y;
	}

	ExpectedChecksum[px + (py * count_x)]	= checksum;
	ExpectedValid[px + (py * count_x)]		= 1;
	return true;
}


bool WorldSyncService::Get_Expected_Chunk_Checksum(int px,int py,unsigned int * checksum_out)
{
	if ((px < 0) || (py < 0) || (px >= ExpectedCountX) || (py >= ExpectedCountY)) return false;
	if (ExpectedValid[px + (py * ExpectedCountX)] == 0) return false;
	if (checksum_out != nullptr) *checksum_out = ExpectedChecksum[px + (py * ExpectedCountX)];
	return true;
}


/***********************************************************************************************
**	Step 8 -- the handshake, built from the systems that own each answer.
*/
bool WorldSyncService::Build_Handshake(unsigned int profile_hash,unsigned int content_hash,
													unsigned int manifest_revision,
													WorldSyncHandshakeClass * out)
{
	if (out == nullptr) return false;
	if (!WorldTerrainSystem::Has_Terrain()) return false;

	WorldSyncHandshakeClass handshake;
	handshake.GeneratorVersion	= (unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION;
	handshake.ProfileHash		= profile_hash;
	handshake.ContentHash		= content_hash;
	handshake.ManifestRevision	= manifest_revision;
	handshake.WorldRevision		= WorldOperationJournal::Get_Revision();

	unsigned int world_checksum = 0;
	if (!WorldSnapshotService::Compute_World_Checksum(&world_checksum)) return false;
	handshake.WorldChecksum		= world_checksum;

	/*
	**	The revision the snapshot set was taken at.  Read off the set itself rather than assumed to
	**	be the current one -- a server that has applied operations since its last compaction sends
	**	snapshots at the older revision and the operations that followed, and a client told the
	**	wrong starting point would arrive at the right ground with the wrong revision on it.
	*/
	handshake.SnapshotRevision	= WorldOperationJournal::Get_Revision();
	if (WorldSnapshotService::Get_Snapshot_Count() > 0) {
		WorldChunkSnapshotClass first;
		if (WorldSnapshotService::Get_Snapshot(0,&first)) {
			handshake.SnapshotRevision = first.Revision;
		}
	}

	handshake.SnapshotCount		= WorldSnapshotService::Get_Snapshot_Count();
	handshake.SnapshotBytes		= WorldSnapshotService::Get_Snapshot_Bytes();
	handshake.OperationCount	= WorldOperationJournal::Get_Operation_Count();

	*out = handshake;
	return true;
}


/***********************************************************************************************
**	Steps 9-12 -- the join.
*/
WorldSyncResultType WorldSyncService::Begin_Join(const WorldSyncHandshakeClass & server,
																 const WorldSyncHandshakeClass & client,
																 StringClass * detail_out)
{
	if (detail_out != nullptr) *detail_out = "";

	Joining = false;

	if ((server.HandshakeVersion != (unsigned int)WorldSyncHandshakeClass::CURRENT_HANDSHAKE_VERSION) ||
		 (client.HandshakeVersion != server.HandshakeVersion)) {
		return WORLD_SYNC_BAD_HANDSHAKE_VERSION;
	}

	/*
	**	The four identity fields, checked before a single snapshot byte moves.
	**
	**	A client that regenerates a different baseline and then patches it with the server's
	**	snapshots does not end up with the server's world -- it ends up with one that is wrong
	**	everywhere the snapshots did not happen to cover, and no checksum after the fact can tell
	**	it which parts.  So this refuses early and says which field disagreed.
	*/
	if (server.GeneratorVersion != client.GeneratorVersion) {
		if (detail_out) {
			detail_out->Format("server generator %u, client %u",
									 server.GeneratorVersion,client.GeneratorVersion);
		}
		return WORLD_SYNC_GENERATOR_MISMATCH;
	}
	if (server.ProfileHash != client.ProfileHash) {
		if (detail_out) {
			detail_out->Format("server profile 0x%08X, client 0x%08X",server.ProfileHash,client.ProfileHash);
		}
		return WORLD_SYNC_PROFILE_MISMATCH;
	}
	if (server.ContentHash != client.ContentHash) {
		if (detail_out) {
			detail_out->Format("server content 0x%08X, client 0x%08X",server.ContentHash,client.ContentHash);
		}
		return WORLD_SYNC_CONTENT_MISMATCH;
	}
	if (server.ManifestRevision != client.ManifestRevision) {
		if (detail_out) {
			detail_out->Format("server manifest %u, client %u",
									 server.ManifestRevision,client.ManifestRevision);
		}
		return WORLD_SYNC_MANIFEST_MISMATCH;
	}

	//	Step 9's own precondition.  This service never generates terrain; it says whether what the
	//	client generated can be patched into the server's world.
	if (!WorldTerrainSystem::Has_Terrain()) {
		if (detail_out) *detail_out = "the client has no regenerated baseline to patch";
		return WORLD_SYNC_NO_BASELINE;
	}

	/*
	**	Step 9-11's starting point.  The client's ground is about to become the server's without a
	**	single operation passing through its journal, so its revision has to come from the same
	**	authority the ground does.  Any post-snapshot operations applied after this bump it further,
	**	by exactly the arithmetic the server used.
	*/
	StringClass adopt_error ("");
	if (!WorldOperationJournal::Adopt_Revision(server.SnapshotRevision,&adopt_error)) {
		if (detail_out) *detail_out = adopt_error;
		return WORLD_SYNC_NO_BASELINE;
	}

	ServerHandshake	= server;
	Joining				= true;

	Metrics.LateJoinBytes	= server.Get_Block_Size();
	Metrics.SnapshotBytes	= 0;
	Metrics.OperationBytes	= 0;

	Clear_Expected_Checksums();
	return WORLD_SYNC_OK;
}


WorldSyncResultType WorldSyncService::Apply_Snapshot_Block(const unsigned char * buffer,int bytes,
																			  int * consumed_out)
{
	if (!Joining) return WORLD_SYNC_NOT_JOINING;

	WorldChunkSnapshotClass snapshot;
	int consumed = 0;
	if (!snapshot.Read_Block(buffer,bytes,&consumed)) return WORLD_SYNC_SNAPSHOT_REFUSED;

	if (!WorldSnapshotService::Apply_Snapshot(snapshot)) return WORLD_SYNC_SNAPSHOT_REFUSED;

	//	The inventory, recorded as it arrives: this is what step 13's mismatch search compares the
	//	client's own chunks against later.
	Set_Expected_Chunk_Checksum(snapshot.ChunkX,snapshot.ChunkY,snapshot.ChunkChecksum);

	Metrics.SnapshotBytes	+= consumed;
	Metrics.LateJoinBytes	+= consumed;

	if (consumed_out != nullptr) *consumed_out = consumed;
	return WORLD_SYNC_OK;
}


WorldSyncResultType WorldSyncService::Apply_Operation_Block(const unsigned char * buffer,int bytes,
																				int * consumed_out)
{
	if (!Joining) return WORLD_SYNC_NOT_JOINING;

	WorldOperationClass operation;
	int consumed = 0;
	if (!operation.Read_Block(buffer,bytes,&consumed)) return WORLD_SYNC_OPERATION_REFUSED;

	/*
	**	Step 11 -- "applies later operations in order".  Submitted through the journal, exactly as
	**	the server submitted it, rather than applied directly: a client that shortcut the journal
	**	would end up with the server's ground and its own idea of the revision, which is the state
	**	step 16's readiness gate exists to prevent.
	*/
	WorldOperationApplyResult result = WorldOperationJournal::Submit(operation);
	if (!result.Is_Applied()) return WORLD_SYNC_OPERATION_REFUSED;

	Metrics.OperationBytes	+= consumed;
	Metrics.LateJoinBytes	+= consumed;

	if (consumed_out != nullptr) *consumed_out = consumed;
	return WORLD_SYNC_OK;
}


WorldSyncResultType WorldSyncService::Finish_Join(StringClass * detail_out)
{
	if (detail_out != nullptr) *detail_out = "";
	if (!Joining) return WORLD_SYNC_NOT_JOINING;

	Joining = false;
	Metrics.JoinCount++;

	/*
	**	The chunk registry is derived from the heights the snapshots just replaced, and a client
	**	about to play on this ground needs it to describe this ground -- placement, navigation and
	**	the Commander's own rules all read it.
	**
	**	Not for the checksum's sake: the chunk checksum reads heights only, on purpose (see
	**	WorldSnapshotService::Compute_Chunk_Checksum).  Once here rather than once per snapshot
	**	block, because rebuilding per chunk would rebuild the whole registry for every chunk
	**	received.
	*/
	WorldSnapshotService::Refresh_Derived_State();

	unsigned int local = 0;
	if (!WorldSnapshotService::Compute_World_Checksum(&local)) {
		if (detail_out) *detail_out = "the client could not checksum its own world";
		return WORLD_SYNC_CHECKSUM_MISMATCH;
	}

	if (local != ServerHandshake.WorldChecksum) {
		Metrics.MismatchCount++;
		if (detail_out) {
			detail_out->Format("client 0x%08X, server 0x%08X",local,ServerHandshake.WorldChecksum);
		}
		return WORLD_SYNC_CHECKSUM_MISMATCH;
	}

	return WORLD_SYNC_OK;
}


/***********************************************************************************************
**	Steps 13-15 -- regional repair.
*/
int WorldSyncService::Find_Mismatched_Chunks(DynamicVectorClass<int> * chunk_x_out,
															DynamicVectorClass<int> * chunk_y_out)
{
	if (chunk_x_out != nullptr) chunk_x_out->Delete_All();
	if (chunk_y_out != nullptr) chunk_y_out->Delete_All();

	int count_x = 0, count_y = 0;
	if (!Ensure_Expected_Grid(&count_x,&count_y)) return 0;

	int found = 0;
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			unsigned int expected = 0;
			if (!Get_Expected_Chunk_Checksum(px,py,&expected)) continue;

			unsigned int local = 0;
			if (!WorldSnapshotService::Compute_Chunk_Checksum(px,py,&local)) continue;

			if (local == expected) continue;

			if (chunk_x_out != nullptr) chunk_x_out->Add(px);
			if (chunk_y_out != nullptr) chunk_y_out->Add(py);
			found++;
		}
	}

	return found;
}


bool WorldSyncService::Build_Repair_Block(int px,int py,unsigned char * buffer,int max_bytes,int * used_out)
{
	/*
	**	Step 14 says the server "returns only affected chunk snapshot", and the word that matters is
	**	*the* snapshot: the retained one, not a fresh capture.
	**
	**	After a compaction the journal's baseline is the current ground, so every chunk is pristine
	**	against it and a fresh capture would come back empty -- a repair block that repairs nothing.
	**	The retained set is what still holds each chunk's difference from the baseline a *client*
	**	regenerates, which is the baseline a repair has to be expressed against.  A fresh capture is
	**	the fallback for a chunk the set does not cover, which is the pre-compaction case.
	*/
	WorldChunkSnapshotClass snapshot;
	if (!WorldSnapshotService::Find_Snapshot_For_Chunk(px,py,&snapshot)) {
		if (!WorldSnapshotService::Capture_Chunk(px,py,&snapshot)) return false;
	}

	int used = 0;
	if (!snapshot.Write_Block(buffer,max_bytes,&used)) return false;

	Metrics.RepairBytes += used;
	if (used_out != nullptr) *used_out = used;
	return true;
}


WorldSyncResultType WorldSyncService::Apply_Repair_Block(const unsigned char * buffer,int bytes,
																			StringClass * detail_out)
{
	if (detail_out != nullptr) *detail_out = "";

	WorldChunkSnapshotClass snapshot;
	int consumed = 0;
	if (!snapshot.Read_Block(buffer,bytes,&consumed)) return WORLD_SYNC_SNAPSHOT_REFUSED;

	if (!WorldSnapshotService::Apply_Snapshot(snapshot)) return WORLD_SYNC_SNAPSHOT_REFUSED;

	Metrics.RepairBytes += consumed;
	Metrics.RepairCount++;

	//	Same reason as in Finish_Join: this chunk's ground just moved, so the registry describing it
	//	has to follow.  The reverify below reads heights only and does not depend on this.
	WorldSnapshotService::Refresh_Derived_State();

	//	Step 15's reverify.  The block said what this chunk should check out to; asking the chunk
	//	is the only way to know it does, and trusting the block instead would make a repair that
	//	silently failed indistinguishable from one that worked.
	unsigned int local = 0;
	if (!WorldSnapshotService::Compute_Chunk_Checksum(snapshot.ChunkX,snapshot.ChunkY,&local)) {
		if (detail_out) *detail_out = "the repaired chunk could not be checksummed";
		return WORLD_SYNC_CHECKSUM_MISMATCH;
	}

	if (local != snapshot.ChunkChecksum) {
		Metrics.MismatchCount++;
		if (detail_out) {
			detail_out->Format("chunk %d,%d repaired to 0x%08X, expected 0x%08X",
									 snapshot.ChunkX,snapshot.ChunkY,local,snapshot.ChunkChecksum);
		}
		return WORLD_SYNC_CHECKSUM_MISMATCH;
	}

	Set_Expected_Chunk_Checksum(snapshot.ChunkX,snapshot.ChunkY,snapshot.ChunkChecksum);
	return WORLD_SYNC_OK;
}


bool WorldSyncService::Is_World_Ready_For_Revision(unsigned int required_revision)
{
	//	Step 16.  Not "has the world finished loading" -- "does the world this object was recorded
	//	against exist here yet".  A building whose state was captured at revision 12 placed into a
	//	world still at revision 9 stands on ground three operations out of date.
	if (!WorldTerrainSystem::Has_Terrain())			return false;
	if (!WorldOperationJournal::Is_Open())			return false;
	if (Joining)											return false;

	return WorldOperationJournal::Get_Revision() >= required_revision;
}
