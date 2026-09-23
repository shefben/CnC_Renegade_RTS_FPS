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
 *                     $Archive:: /Commando/Code/Combat/worldsnapshot.cpp                     $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldsnapshot.h"

#include "crc.h"
#include "heightfield.h"
#include "worldoperationjournal.h"
#include "worldterrainsystem.h"
#include "wwmath.h"

#include <string.h>


/*
**	Section 3B.4's millimetre grid -- with one property the other checksums in this chain do not
**	need and this one cannot do without: `Quantize(Dequantize(q)) == q`, for every q.
**
**	A snapshot stores quantized heights and a client applies them, so a client's stored height is
**	the dequantized value where the server's is the original float.  If the round trip is not
**	exact the two checksum differently while describing the same ground -- and the repair protocol,
**	which cannot tell that apart from real corruption, sends the client chasing chunks that need
**	nothing.  Truncation does exactly that: 2.999f * 1000.0f is 2998.9998 in float, so a truncating
**	quantizer turns 2999 into 2998 on the way back.  Rounding does not, and the division is done in
**	double so the value handed back is the nearest float to the millimetre rather than to whatever
**	a float multiply produced.
*/
static inline int Quantize(float value)			{ return (int)WWMath::Floor((value * 1000.0f) + 0.5f); }
static inline float Dequantize(int value)		{ return (float)((double)value / 1000.0); }


WorldSnapshotCompactionPolicy	WorldSnapshotService::Policy;
int	WorldSnapshotService::CompactionCount	= 0;

DynamicVectorClass<WorldChunkSnapshotClass>	WorldSnapshotService::Snapshots;
DynamicVectorClass<WorldSnapshotService::RollbackPointClass>	WorldSnapshotService::RollbackPoints;


/***********************************************************************************************
**	The chunk snapshot record.
*/

WorldChunkSnapshotClass::WorldChunkSnapshotClass(void) :
	SnapshotVersion(CURRENT_SNAPSHOT_VERSION),Revision(0),
	ChunkX(-1),ChunkY(-1),ChunkChecksum(0)
{
}


void WorldChunkSnapshotClass::Reset(void)
{
	SnapshotVersion	= CURRENT_SNAPSHOT_VERSION;
	Revision				= 0;
	ChunkX				= -1;
	ChunkY				= -1;
	ChunkChecksum		= 0;
	VertexIndex.Delete_All();
	QuantizedHeight.Delete_All();
}


bool WorldChunkSnapshotClass::operator == (const WorldChunkSnapshotClass & that) const
{
	if ((SnapshotVersion != that.SnapshotVersion) || (Revision != that.Revision) ||
		 (ChunkX != that.ChunkX) || (ChunkY != that.ChunkY) ||
		 (ChunkChecksum != that.ChunkChecksum)) {
		return false;
	}
	if (VertexIndex.Count() != that.VertexIndex.Count()) return false;
	for (int i = 0; i < VertexIndex.Count(); i++) {
		if (VertexIndex[i] != that.VertexIndex[i]) return false;
		if (QuantizedHeight[i] != that.QuantizedHeight[i]) return false;
	}
	return true;
}


//	SnapshotVersion, Revision, ChunkX, ChunkY, ChunkChecksum, entry count.
enum { WORLD_CHUNK_SNAPSHOT_HEADER_FIELDS = 6 };

static void Write_Int(unsigned char * at,int value)
{
	unsigned int bits = (unsigned int)value;
	at[0] = (unsigned char)( bits        & 0xFF);
	at[1] = (unsigned char)((bits >>  8) & 0xFF);
	at[2] = (unsigned char)((bits >> 16) & 0xFF);
	at[3] = (unsigned char)((bits >> 24) & 0xFF);
}

static int Read_Int(const unsigned char * at)
{
	unsigned int bits =	((unsigned int)at[0])			|
								(((unsigned int)at[1]) <<  8)	|
								(((unsigned int)at[2]) << 16)	|
								(((unsigned int)at[3]) << 24);
	return (int)bits;
}


int WorldChunkSnapshotClass::Get_Block_Size(void) const
{
	return (WORLD_CHUNK_SNAPSHOT_HEADER_FIELDS * 4) + (VertexIndex.Count() * 2 * 4);
}


bool WorldChunkSnapshotClass::Write_Block(unsigned char * buffer,int max_bytes,int * used_out) const
{
	int needed = Get_Block_Size();
	if ((buffer == nullptr) || (max_bytes < needed)) return false;

	unsigned char * at = buffer;

	Write_Int(at,(int)SnapshotVersion);	at += 4;
	Write_Int(at,(int)Revision);			at += 4;
	Write_Int(at,ChunkX);					at += 4;
	Write_Int(at,ChunkY);					at += 4;
	Write_Int(at,(int)ChunkChecksum);	at += 4;
	Write_Int(at,VertexIndex.Count());	at += 4;

	for (int i = 0; i < VertexIndex.Count(); i++) {
		Write_Int(at,VertexIndex[i]);		at += 4;
		Write_Int(at,QuantizedHeight[i]);	at += 4;
	}

	if (used_out != nullptr) *used_out = (int)(at - buffer);
	return true;
}


bool WorldChunkSnapshotClass::Read_Block(const unsigned char * buffer,int bytes,int * consumed_out)
{
	if ((buffer == nullptr) || (bytes < (WORLD_CHUNK_SNAPSHOT_HEADER_FIELDS * 4))) return false;

	const unsigned char * at = buffer;

	unsigned int version = (unsigned int)Read_Int(at);	at += 4;
	if (version != (unsigned int)CURRENT_SNAPSHOT_VERSION) return false;

	Reset();
	SnapshotVersion	= version;
	Revision				= (unsigned int)Read_Int(at);	at += 4;
	ChunkX				= Read_Int(at);						at += 4;
	ChunkY				= Read_Int(at);						at += 4;
	ChunkChecksum		= (unsigned int)Read_Int(at);		at += 4;

	int entries = Read_Int(at);	at += 4;
	if (entries < 0) return false;

	int remaining = bytes - (int)(at - buffer);
	if (remaining < (entries * 2 * 4)) return false;

	for (int i = 0; i < entries; i++) {
		int index = Read_Int(at);		at += 4;
		int height = Read_Int(at);		at += 4;
		VertexIndex.Add(index);
		QuantizedHeight.Add(height);
	}

	if (consumed_out != nullptr) *consumed_out = (int)(at - buffer);
	return true;
}


/***********************************************************************************************
**	The service.
*/

void WorldSnapshotService::Init(void)		{ Clear(); }
void WorldSnapshotService::Shutdown(void)	{ Clear(); }


void WorldSnapshotService::Clear(void)
{
	Policy = WorldSnapshotCompactionPolicy();
	CompactionCount = 0;
	Snapshots.Delete_All();
	RollbackPoints.Delete_All();
}


int	WorldSnapshotService::Get_Snapshot_Count(void)	{ return Snapshots.Count(); }
int	WorldSnapshotService::Get_Compaction_Count(void)	{ return CompactionCount; }
int	WorldSnapshotService::Get_Rollback_Point_Count(void)	{ return RollbackPoints.Count(); }

const WorldSnapshotCompactionPolicy &	WorldSnapshotService::Get_Compaction_Policy(void)	{ return Policy; }


void WorldSnapshotService::Set_Compaction_Policy(const WorldSnapshotCompactionPolicy & policy)
{
	Policy = policy;

	//	A non-positive threshold is not a stricter policy, it is one that compacts after every
	//	operation forever -- and a compaction is not free.  Clamped to the documented defaults.
	WorldSnapshotCompactionPolicy defaults;
	if (Policy.MaxOperations <= 0)					Policy.MaxOperations			= defaults.MaxOperations;
	if (!(Policy.MaxAgeSeconds > 0.0f))				Policy.MaxAgeSeconds			= defaults.MaxAgeSeconds;
	if (Policy.MaxOperationBytes <= 0)				Policy.MaxOperationBytes	= defaults.MaxOperationBytes;
}


bool WorldSnapshotService::Get_Snapshot(int index,WorldChunkSnapshotClass * out)
{
	if ((index < 0) || (index >= Snapshots.Count()) || (out == nullptr)) return false;
	*out = Snapshots[index];
	return true;
}


bool WorldSnapshotService::Find_Snapshot_For_Chunk(int px,int py,WorldChunkSnapshotClass * out)
{
	for (int i = 0; i < Snapshots.Count(); i++) {
		if ((Snapshots[i].ChunkX != px) || (Snapshots[i].ChunkY != py)) continue;
		if (out != nullptr) *out = Snapshots[i];
		return true;
	}
	return false;
}


int WorldSnapshotService::Get_Snapshot_Bytes(void)
{
	int bytes = 0;
	for (int i = 0; i < Snapshots.Count(); i++) {
		bytes += Snapshots[i].Get_Block_Size();
	}
	return bytes;
}


/***********************************************************************************************
**	Which heightfield vertices belong to a chunk.
**
**	A chunk is a terrain patch, and a patch owns PATCH_CELLS cells each way -- so its vertices run
**	from px*PATCH_CELLS to (px+1)*PATCH_CELLS inclusive.  The shared seam vertex belongs to both
**	patches across it, and that is correct rather than a double count: a vertex on a seam really is
**	part of both chunks, and a repair that restored it for only one of them would leave a crack.
*/
static bool Chunk_Vertex_Range(int px,int py,int * min_x,int * min_y,int * max_x,int * max_y)
{
	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return false;

	int count_x = heightfield->Get_Vertex_Count_X();
	int count_y = heightfield->Get_Vertex_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	if ((px < 0) || (py < 0)) return false;
	if ((px >= WorldTerrainSystem::Get_Patch_Count_X()) ||
		 (py >= WorldTerrainSystem::Get_Patch_Count_Y())) return false;

	int lo_x = px * HeightfieldClass::PATCH_CELLS;
	int lo_y = py * HeightfieldClass::PATCH_CELLS;
	int hi_x = lo_x + HeightfieldClass::PATCH_CELLS;
	int hi_y = lo_y + HeightfieldClass::PATCH_CELLS;

	if (hi_x > (count_x - 1)) hi_x = count_x - 1;
	if (hi_y > (count_y - 1)) hi_y = count_y - 1;
	if ((lo_x > hi_x) || (lo_y > hi_y)) return false;

	*min_x = lo_x;
	*min_y = lo_y;
	*max_x = hi_x;
	*max_y = hi_y;
	return true;
}


bool WorldSnapshotService::Compute_Chunk_Checksum(int px,int py,unsigned int * checksum_out)
{
	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return false;

	int min_x = 0, min_y = 0, max_x = 0, max_y = 0;
	if (!Chunk_Vertex_Range(px,py,&min_x,&min_y,&max_x,&max_y)) return false;

	CRCEngine crc;
	crc(&px,sizeof(px));
	crc(&py,sizeof(py));

	for (int iy = min_y; iy <= max_y; iy++) {
		for (int ix = min_x; ix <= max_x; ix++) {
			int quantized = Quantize(heightfield->Get_Height(ix,iy));
			crc(&quantized,sizeof(quantized));
		}
	}

	/*
	**	Heights, and deliberately nothing else.
	**
	**	The obvious extra input is the chunk registry's own entry -- and it was in here, until a
	**	late join proved it cannot be.  That entry is *derived* from these same heights, so it adds
	**	no information a checksum over the heights does not already have; what it does add is
	**	sensitivity to sub-millimetre noise.  A client rebuilds its ground from quantized heights,
	**	so its floats land within half a millimetre of the server's; the registry then derives
	**	slopes from those floats and truncates them at 1e-3, and a value either side of a boundary
	**	truncates differently on the two machines.  The ground agrees, the entry does not, and the
	**	repair protocol -- which cannot tell that from real corruption -- sends the client chasing a
	**	chunk that needs nothing.
	**
	**	Materials, foliage and props are left out for the plainer reason: each is either
	**	regenerated deterministically from the seed or owned by a system with its own state block,
	**	and folding them in would make a checksum that disagrees with itself depending on how far a
	**	client's own generation had got.
	*/
	if (checksum_out != nullptr) *checksum_out = (unsigned int)crc;
	return true;
}


bool WorldSnapshotService::Compute_World_Checksum(unsigned int * checksum_out)
{
	if (!WorldTerrainSystem::Has_Terrain()) return false;

	int count_x = WorldTerrainSystem::Get_Patch_Count_X();
	int count_y = WorldTerrainSystem::Get_Patch_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	CRCEngine crc;
	crc(&count_x,sizeof(count_x));
	crc(&count_y,sizeof(count_y));

	//	Grid order, not iteration order, so two peers compose the same number from the same chunks.
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			unsigned int chunk = 0;
			if (!Compute_Chunk_Checksum(px,py,&chunk)) continue;
			crc(&chunk,sizeof(chunk));
		}
	}

	unsigned int revision = WorldOperationJournal::Get_Revision();
	unsigned int journal  = WorldOperationJournal::Get_Journal_Checksum();
	crc(&revision,sizeof(revision));
	crc(&journal,sizeof(journal));

	if (checksum_out != nullptr) *checksum_out = (unsigned int)crc;
	return true;
}


bool WorldSnapshotService::Is_Chunk_Dirty(int px,int py)
{
	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return false;

	int base_x = 0, base_y = 0;
	if (!WorldOperationJournal::Get_Base_Dimensions(&base_x,&base_y)) return false;
	if ((base_x != heightfield->Get_Vertex_Count_X()) ||
		 (base_y != heightfield->Get_Vertex_Count_Y())) {
		return false;
	}

	int min_x = 0, min_y = 0, max_x = 0, max_y = 0;
	if (!Chunk_Vertex_Range(px,py,&min_x,&min_y,&max_x,&max_y)) return false;

	for (int iy = min_y; iy <= max_y; iy++) {
		for (int ix = min_x; ix <= max_x; ix++) {

			float base = 0.0f;
			if (!WorldOperationJournal::Get_Base_Height(ix,iy,&base)) continue;

			//	Compared quantized, not raw.  A vertex that differs by less than a millimetre is a
			//	vertex two peers would checksum identically, so calling it dirty would send bytes
			//	that change nothing.
			if (Quantize(heightfield->Get_Height(ix,iy)) != Quantize(base)) return true;
		}
	}
	return false;
}


bool WorldSnapshotService::Capture_Chunk(int px,int py,WorldChunkSnapshotClass * out)
{
	if (out == nullptr) return false;

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return false;

	int base_x = 0, base_y = 0;
	if (!WorldOperationJournal::Get_Base_Dimensions(&base_x,&base_y)) return false;

	int count_x = heightfield->Get_Vertex_Count_X();
	if ((base_x != count_x) || (base_y != heightfield->Get_Vertex_Count_Y())) return false;

	int min_x = 0, min_y = 0, max_x = 0, max_y = 0;
	if (!Chunk_Vertex_Range(px,py,&min_x,&min_y,&max_x,&max_y)) return false;

	out->Reset();
	out->Revision	= WorldOperationJournal::Get_Revision();
	out->ChunkX		= px;
	out->ChunkY		= py;

	for (int iy = min_y; iy <= max_y; iy++) {
		for (int ix = min_x; ix <= max_x; ix++) {

			float base = 0.0f;
			if (!WorldOperationJournal::Get_Base_Height(ix,iy,&base)) continue;

			int now = Quantize(heightfield->Get_Height(ix,iy));
			if (now == Quantize(base)) continue;		// regenerating produces this already

			out->VertexIndex.Add((iy * count_x) + ix);
			out->QuantizedHeight.Add(now);
		}
	}

	Compute_Chunk_Checksum(px,py,&out->ChunkChecksum);
	return true;
}


int WorldSnapshotService::Capture_Dirty_Chunks(StringClass * error_out)
{
	Snapshots.Delete_All();

	if (!WorldTerrainSystem::Has_Terrain()) {
		if (error_out) *error_out = "no terrain to snapshot";
		return -1;
	}
	if (!WorldOperationJournal::Is_Open()) {
		if (error_out) *error_out = "no journal open -- there is no baseline to be different from";
		return -1;
	}

	/*
	**	Capturing deliberately refreshes nothing.
	**
	**	It used to refresh the chunk registry first, back when the chunk checksum read that
	**	registry -- a capture taken against a stale one recorded a number nobody could reproduce.
	**	Removing the registry from the checksum (see Compute_Chunk_Checksum) removed the reason
	**	along with the bug: every input to a capture is a height, and heights are not derived from
	**	anything.  Rebuilding the whole registry here now would be a full sweep per compaction
	**	buying nothing.
	**
	**	The registry still has to follow the ground, of course -- that is the journal's dependency
	**	pass on the server and Finish_Join/Apply_Repair_Block on a client, each of which refreshes
	**	because the *world* needs it, not because a checksum does.
	*/
	int count_x = WorldTerrainSystem::Get_Patch_Count_X();
	int count_y = WorldTerrainSystem::Get_Patch_Count_Y();

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			if (!Is_Chunk_Dirty(px,py)) continue;

			WorldChunkSnapshotClass snapshot;
			if (!Capture_Chunk(px,py,&snapshot)) continue;
			Snapshots.Add(snapshot);
		}
	}

	return Snapshots.Count();
}


bool WorldSnapshotService::Apply_Snapshot(const WorldChunkSnapshotClass & snapshot)
{
	if (snapshot.SnapshotVersion != (unsigned int)WorldChunkSnapshotClass::CURRENT_SNAPSHOT_VERSION) {
		return false;
	}
	if (snapshot.VertexIndex.Count() != snapshot.QuantizedHeight.Count()) return false;

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return false;

	int count_x = heightfield->Get_Vertex_Count_X();
	int count_y = heightfield->Get_Vertex_Count_Y();
	int total = count_x * count_y;

	//	Every index is checked before anything is written.  A snapshot from a differently-sized
	//	world would otherwise scatter heights across unrelated rows and leave a world that is
	//	wrong everywhere rather than refused once.
	for (int i = 0; i < snapshot.VertexIndex.Count(); i++) {
		int index = snapshot.VertexIndex[i];
		if ((index < 0) || (index >= total)) return false;
	}

	for (int i = 0; i < snapshot.VertexIndex.Count(); i++) {
		int index = snapshot.VertexIndex[i];
		heightfield->Set_Height(index % count_x,index / count_x,
										Dequantize(snapshot.QuantizedHeight[i]));
	}

	return true;
}


void WorldSnapshotService::Refresh_Derived_State(void)
{
	if (!WorldTerrainSystem::Has_Chunk_Registry()) return;
	WorldTerrainSystem::Build_Chunk_Registry(WorldTerrainSystem::Get_Chunk_Max_Buildable_Slope());
}


/***********************************************************************************************
**	Steps 6-7 -- compaction.
*/
bool WorldSnapshotService::Should_Compact(StringClass * reason_out)
{
	if (reason_out != nullptr) *reason_out = "";
	if (!WorldOperationJournal::Is_Open()) return false;

	int operations = WorldOperationJournal::Get_Operation_Count();
	if (operations <= 0) return false;

	if (operations >= Policy.MaxOperations) {
		if (reason_out) reason_out->Format("%d operations, threshold %d",operations,Policy.MaxOperations);
		return true;
	}

	int bytes = 0;
	float oldest = 0.0f;
	float newest = 0.0f;
	bool  timed = false;

	for (int i = 0; i < operations; i++) {
		WorldOperationClass operation;
		if (!WorldOperationJournal::Get_Operation(i,&operation)) continue;
		bytes += operation.Get_Block_Size();

		if (!timed) {
			oldest = operation.TimeSeconds;
			newest = operation.TimeSeconds;
			timed  = true;
		} else {
			oldest = WWMath::Min(oldest,operation.TimeSeconds);
			newest = WWMath::Max(newest,operation.TimeSeconds);
		}
	}

	if (bytes >= Policy.MaxOperationBytes) {
		if (reason_out) reason_out->Format("%d operation bytes, threshold %d",bytes,Policy.MaxOperationBytes);
		return true;
	}
	if (timed && ((newest - oldest) >= Policy.MaxAgeSeconds)) {
		if (reason_out) {
			reason_out->Format("log spans %.1fs, threshold %.1fs",newest - oldest,Policy.MaxAgeSeconds);
		}
		return true;
	}

	return false;
}


bool WorldSnapshotService::Compact(StringClass * report_out)
{
	if (!WorldOperationJournal::Is_Open()) {
		if (report_out) *report_out = "no journal open";
		return false;
	}

	//	Capture first, rebase second.  Rebasing makes every chunk pristine by definition, so a
	//	capture taken afterwards would be empty and the world's history would be gone with nothing
	//	holding it.
	StringClass capture_error ("");
	int captured = Capture_Dirty_Chunks(&capture_error);
	if (captured < 0) {
		if (report_out) *report_out = capture_error;
		return false;
	}

	int bytes = Get_Snapshot_Bytes();
	int folded = WorldOperationJournal::Get_Operation_Count();

	StringClass rebase_error ("");
	if (!WorldOperationJournal::Rebase(&rebase_error)) {
		if (report_out) *report_out = rebase_error;
		return false;
	}

	CompactionCount++;

	if (report_out != nullptr) {
		report_out->Format("folded %d operation(s) into %d chunk snapshot(s), %d byte(s)",
								 folded,captured,bytes);
	}
	return true;
}


/***********************************************************************************************
**	Steps 17-18 -- developer/admin rollback.
*/
bool WorldSnapshotService::Mark_Rollback_Point(const char * label,StringClass * error_out)
{
	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) {
		if (error_out) *error_out = "no terrain to mark";
		return false;
	}

	int count_x = heightfield->Get_Vertex_Count_X();
	int count_y = heightfield->Get_Vertex_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) {
		if (error_out) *error_out = "the heightfield is empty";
		return false;
	}

	RollbackPointClass point;
	point.Revision			= WorldOperationJournal::Get_Revision();
	point.VertexCountX	= count_x;
	point.VertexCountY	= count_y;
	point.Label				= (label != nullptr) ? label : "";

	point.Heights.Resize(count_x * count_y);
	for (int iy = 0; iy < count_y; iy++) {
		for (int ix = 0; ix < count_x; ix++) {
			point.Heights.Add(heightfield->Get_Height(ix,iy));
		}
	}

	//	Bounded: an unbounded history of a mutated world is the full-world storage this phase
	//	exists to avoid.  The oldest goes.
	while (RollbackPoints.Count() >= MAX_ROLLBACK_POINTS) {
		RollbackPoints.Delete(0);
	}
	RollbackPoints.Add(point);

	return true;
}


bool WorldSnapshotService::Get_Rollback_Point(int index,StringClass * label_out,unsigned int * revision_out)
{
	if ((index < 0) || (index >= RollbackPoints.Count())) return false;
	if (label_out != nullptr) *label_out = RollbackPoints[index].Label;
	if (revision_out != nullptr) *revision_out = RollbackPoints[index].Revision;
	return true;
}


bool WorldSnapshotService::Rollback_To(int index,StringClass * report_out)
{
	if ((index < 0) || (index >= RollbackPoints.Count())) {
		if (report_out) *report_out = "no such rollback point";
		return false;
	}

	const RollbackPointClass & point = RollbackPoints[index];

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) {
		if (report_out) *report_out = "no terrain to roll back";
		return false;
	}
	if ((heightfield->Get_Vertex_Count_X() != point.VertexCountX) ||
		 (heightfield->Get_Vertex_Count_Y() != point.VertexCountY)) {
		if (report_out) *report_out = "the terrain has been resized since this point was marked";
		return false;
	}
	if (point.Heights.Count() != (point.VertexCountX * point.VertexCountY)) {
		if (report_out) *report_out = "the rollback point is malformed";
		return false;
	}

	if (!WorldTerrainSystem::Set_Heights(&point.Heights[0],point.Heights.Count())) {
		if (report_out) *report_out = "the terrain owner refused the rollback heights";
		return false;
	}

	//	The registry was rebuilt by whatever operations are being rolled back; the ground it was
	//	derived from has just moved again.
	Refresh_Derived_State();

	/*
	**	The journal is rebased onto the rolled-back ground rather than rewound.
	**
	**	Rewinding would need every operation since the point to be individually invertible, which
	**	they are not -- a SMOOTH destroys the information needed to un-smooth it.  Rebasing states
	**	the truth instead: this is the ground now, and nothing has happened to it since.
	*/
	StringClass rebase_error ("");
	if (WorldOperationJournal::Is_Open()) {
		WorldOperationJournal::Rebase(&rebase_error);
	}

	if (report_out != nullptr) {
		report_out->Format("rolled back to '%s' (marked at revision %u); journal rebased",
								 point.Label.Peek_Buffer(),point.Revision);
	}
	return true;
}
