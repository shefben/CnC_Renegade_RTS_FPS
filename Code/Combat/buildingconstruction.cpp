/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/buildingconstruction.cpp              $*
 *                                                                                             *
 *	Roadmap Section 44, Phase 38 -- authoritative construction + procedural block build-up.     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "buildingconstruction.h"

#include "buildingdescriptormanager.h"
#include "combat.h"
#include "surfacemarktype.h"
#include "worldsurfacemarkmanager.h"
#include "wwmath.h"

#include <math.h>


const float	BuildingConstructionService::FOUNDATION_END	= 0.10f;
const float	BuildingConstructionService::ROWS_END			= 0.80f;


struct ConstructionRecord
{
	ConstructionRecord(void) : InUse(false),Progress(0.0f),RowCount(0),Completed(false) { }

	bool	operator == (const ConstructionRecord &) const	{ return false; }
	bool	operator != (const ConstructionRecord &) const	{ return true; }

	bool											InUse;
	BuildingConstructionReplicationState	State;
	DynamicVectorClass<BuildingConstructionBlock>	Blocks;
	float											Progress;
	int											RowCount;
	bool											Completed;

	//	Cached from the descriptor at Begin, not looked up per frame: a live reload that
	//	changes reveal timing must not make a half-built building jump, and Section 44's own
	//	replication list carries a "descriptor/config revision if required" for exactly this
	//	reason -- the construction runs on the values it started with.
	float											SourceRevealStart;
	float											BlockFadeStart;

	//	The bounds the layout was generated from.  Kept so that a save can write them and a
	//	load can regenerate an identical layout without having to have the W3D model resident
	//	-- six floats, derived from the model rather than from the renderer, which is why
	//	persisting them is not the "renderer-only implementation detail" Section 46 forbids.
	AABoxClass									ModelBounds;
};

static DynamicVectorClass<ConstructionRecord>	_Records;
static DynamicVectorClass<int>						_Completed;


static int	Find_Record(int building_id)
{
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].InUse && _Records [i].State.BuildingId == building_id) return i;
	}
	return -1;
}


/***********************************************************************************************
 * Block_Key -- the deterministic within-row ordering value                                      *
 *                                                                                             *
 *	An integer hash, not a random number generator: a generator's output depends on how many    *
 *	times it has been called, which would make a block's order depend on iteration order, and    *
 *	a client that generated rows in a different order would get a different building.  A hash    *
 *	of (seed, row, cell) depends on nothing but its arguments, so there is no ordering to get     *
 *	wrong.  This is the 32-bit finalizer from MurmurHash3, used for its mixing, not its           *
 *	cryptography.                                                                                *
 *=============================================================================================*/
static unsigned int	Block_Key(unsigned int seed,int row,int cell)
{
	unsigned int h = seed;
	h ^= (unsigned int)(row + 1) * 0x9E3779B9u;
	h ^= (unsigned int)(cell + 1) * 0x85EBCA6Bu;

	h ^= h >> 16;
	h *= 0x85EBCA6Bu;
	h ^= h >> 13;
	h *= 0xC2B2AE35u;
	h ^= h >> 16;
	return h;
}


void BuildingConstructionService::Init(void)
{
	_Records.Delete_All ();
	_Completed.Delete_All ();
}

void BuildingConstructionService::Shutdown(void)
{
	Init ();
}


int BuildingConstructionService::Generate_Blocks(const char * descriptor_name,const AABoxClass & model_bounds,
																  unsigned int seed,
																  DynamicVectorClass<BuildingConstructionBlock> & out)
{
	out.Delete_All ();

	if (descriptor_name == nullptr || descriptor_name [0] == '\0') return 0;

	const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name);
	if (descriptor == nullptr) return 0;

	const BuildingConstructionVisualDefinition & visual = descriptor->ConstructionVisual;

	float size_x = model_bounds.Extent.X * 2.0f;
	float size_y = model_bounds.Extent.Y * 2.0f;
	float size_z = model_bounds.Extent.Z * 2.0f;
	if (size_x <= 0.0f || size_y <= 0.0f || size_z <= 0.0f) return 0;

	/*
	**	Respect max_blocks by making the blocks bigger, never by generating fewer of them and
	**	leaving a hole.  Coarsening by a fixed factor keeps the loop deterministic and bounded:
	**	each pass multiplies the cell count down by about two, so even an absurd bounds/blocksize
	**	ratio converges in a handful of iterations.
	*/
	float block_width	= visual.BlockWidth;
	float block_depth	= visual.BlockDepth;
	float row_height	= visual.RowHeight;

	int cells_x = 1, cells_y = 1, rows = 1;

	for (int attempt = 0; attempt < 32; attempt++) {

		cells_x	= (int)::ceilf (size_x / block_width);
		cells_y	= (int)::ceilf (size_y / block_depth);
		rows		= (int)::ceilf (size_z / row_height);

		if (cells_x < 1) cells_x = 1;
		if (cells_y < 1) cells_y = 1;
		if (rows < 1)    rows    = 1;

		if ((cells_x * cells_y * rows) <= visual.MaxBlocks) break;

		block_width	*= 1.4f;
		block_depth	*= 1.4f;
		row_height	*= 1.4f;
	}

	//	Blocks tile the bounds exactly rather than being the JSON's nominal size and leaving a
	//	seam at the far edge; "keep useful approximate block proportions" is what the JSON value
	//	is for, and it is preserved to within one cell.
	float cell_w = size_x / (float)cells_x;
	float cell_d = size_y / (float)cells_y;
	float cell_h = size_z / (float)rows;

	float origin_x = model_bounds.Center.X - model_bounds.Extent.X;
	float origin_y = model_bounds.Center.Y - model_bounds.Extent.Y;
	float origin_z = model_bounds.Center.Z - model_bounds.Extent.Z;

	int cells_per_row = cells_x * cells_y;
	float row_span = (BuildingConstructionService::ROWS_END - BuildingConstructionService::FOUNDATION_END)
							/ (float)rows;

	DynamicVectorClass<int>	order_of_cell;

	for (int r = 0; r < rows; r++) {

		/*
		**	Order the cells of this row by their hash.  An insertion sort over the cell indices,
		**	with the cell index itself as the tie-break so that two cells that happen to hash
		**	equal still order identically everywhere.  Rows are small; this is not a hot path,
		**	it runs once per building.
		*/
		DynamicVectorClass<int>	sorted;
		for (int c = 0; c < cells_per_row; c++) sorted.Add (c);

		for (int a = 1; a < sorted.Count (); a++) {
			int key_cell			= sorted [a];
			unsigned int key_hash	= Block_Key (seed,r,key_cell);
			int b = a - 1;
			while (b >= 0) {
				int candidate				= sorted [b];
				unsigned int candidate_hash	= Block_Key (seed,r,candidate);
				bool candidate_first = (candidate_hash < key_hash) ||
												((candidate_hash == key_hash) && (candidate <= key_cell));
				if (candidate_first) break;
				sorted [b + 1] = sorted [b];
				b--;
			}
			sorted [b + 1] = key_cell;
		}

		order_of_cell.Delete_All ();
		for (int i = 0; i < cells_per_row; i++) order_of_cell.Add (0);
		for (int position = 0; position < sorted.Count (); position++) {
			order_of_cell [sorted [position]] = position;
		}

		float row_start = BuildingConstructionService::FOUNDATION_END + ((float)r * row_span);

		for (int cell = 0; cell < cells_per_row; cell++) {

			int cx = cell % cells_x;
			int cy = cell / cells_x;

			BuildingConstructionBlock block;
			block.LocalCenter.X	= origin_x + ((float)cx + 0.5f) * cell_w;
			block.LocalCenter.Y	= origin_y + ((float)cy + 0.5f) * cell_d;
			block.LocalCenter.Z	= origin_z + ((float)r  + 0.5f) * cell_h;
			block.HalfExtent		= Vector3 (cell_w * 0.5f,cell_d * 0.5f,cell_h * 0.5f);
			block.Row				= r;
			block.Order				= order_of_cell [cell];

			/*
			**	Every block of a row lands inside that row's own slice of the timeline, so rows
			**	activate strictly bottom-to-top however the within-row order came out.  The
			**	stagger fills the first half of the slice and each block takes the other half to
			**	rise, which is what makes a row look assembled rather than switched on.
			*/
			float stagger = (cells_per_row > 1)
									? ((float)block.Order / (float)(cells_per_row - 1)) * (row_span * 0.5f)
									: 0.0f;
			block.ProgressStart	= row_start + stagger;
			block.ProgressEnd		= block.ProgressStart + (row_span * 0.5f);

			out.Add (block);
		}
	}

	return out.Count ();
}


static bool	Start_Record(const BuildingConstructionReplicationState & state,const AABoxClass & model_bounds)
{
	if (state.BuildingId == 0) return false;
	if (state.DurationSeconds <= 0.0f) return false;

	const BuildingModelDescriptor * descriptor =
		BuildingDescriptorManager::Get_Descriptor (state.DescriptorName.Peek_Buffer ());
	if (descriptor == nullptr) return false;

	int index = Find_Record (state.BuildingId);
	bool fresh = (index < 0);
	if (index < 0) {
		//	Reuse a freed slot before growing: construction records are created and destroyed
		//	all match long, and the id list a caller iterates stays short.
		for (int i = 0; i < _Records.Count (); i++) {
			if (!_Records [i].InUse) { index = i; break; }
		}
		if (index < 0) {
			_Records.Add (ConstructionRecord ());
			index = _Records.Count () - 1;
		}
	}

	ConstructionRecord & record = _Records [index];
	record.InUse					= true;
	record.State					= state;
	record.State.Active			= true;
	record.Progress				= 0.0f;
	record.Completed				= false;
	record.SourceRevealStart	= descriptor->ConstructionVisual.SourceRevealStart;
	record.BlockFadeStart		= descriptor->ConstructionVisual.BlockFadeStart;
	record.ModelBounds			= model_bounds;

	BuildingConstructionService::Generate_Blocks (state.DescriptorName.Peek_Buffer (),model_bounds,
																 state.Seed,record.Blocks);

	record.RowCount = 0;
	for (int i = 0; i < record.Blocks.Count (); i++) {
		if ((record.Blocks [i].Row + 1) > record.RowCount) record.RowCount = record.Blocks [i].Row + 1;
	}

	/*
	**	P19: the ground under a new building is marked as a construction pad the moment work
	**	starts -- SURFACE_MARK_CONSTRUCTION's own event, on every client including a late joiner
	**	(all three start paths come through here).  hnd_pad is the retail concrete pad texture from
	**	always.dat.  Construction marks persist rather than fade (Surface_Mark_Type rules).
	*/
	if (fresh && CombatManager::I_Am_Client ()) {
		Vector3 center = state.Transform.Get_Translation ();
		Matrix3D pad_tm;
		pad_tm.Look_At (center + Vector3 (0.0f,0.0f,2.0f),center - Vector3 (0.0f,0.0f,1.0f),0.0f);
		float radius = 0.5f * WWMath::Sqrt ((descriptor->Footprint.Width * descriptor->Footprint.Width) +
														(descriptor->Footprint.Depth * descriptor->Footprint.Depth));
		WorldSurfaceMarkManager::Create_Mark (pad_tm,"hnd_pad.tga",radius,SURFACE_MARK_CONSTRUCTION);
	}

	return true;
}


bool BuildingConstructionService::Begin_Construction(int building_id,const char * descriptor_name,
																	  const Matrix3D & transform,const AABoxClass & model_bounds,
																	  float server_time_seconds,unsigned int seed)
{
	if (descriptor_name == nullptr || descriptor_name [0] == '\0') return false;

	const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name);
	if (descriptor == nullptr) return false;

	BuildingConstructionReplicationState state;
	state.BuildingId			= building_id;
	state.Transform			= transform;
	state.StartTimeSeconds	= server_time_seconds;
	state.DurationSeconds	= descriptor->Get_Build_Time_Seconds ();
	state.Seed					= seed;
	state.Active				= true;
	state.DescriptorName		= descriptor_name;

	return Start_Record (state,model_bounds);
}


bool BuildingConstructionService::Begin_Construction_From_State(const BuildingConstructionReplicationState & state,
																					 const AABoxClass & model_bounds)
{
	return Start_Record (state,model_bounds);
}


void BuildingConstructionService::Cancel_Construction(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return;

	//	Everything temporary about a half-built building is this record.  Dropping it is the
	//	whole of Section 44's cancellation cleanup as far as this service is concerned; the
	//	footprint blocker and the GameObj belong to the caller that created them.
	_Records [index].InUse		= false;
	_Records [index].State		= BuildingConstructionReplicationState ();
	_Records [index].Blocks.Delete_All ();
	_Records [index].Progress	= 0.0f;
	_Records [index].RowCount	= 0;
	_Records [index].Completed	= false;
}


void BuildingConstructionService::Update(float server_time_seconds)
{
	for (int i = 0; i < _Records.Count (); i++) {

		ConstructionRecord & record = _Records [i];
		if (!record.InUse || record.Completed) continue;

		/*
		**	Progress from server time, never from accumulated frame deltas: "Server time/state
		**	owns completion, never client frame rate."  A client that hitched for two seconds
		**	arrives at the same progress as one that did not.
		*/
		float elapsed = server_time_seconds - record.State.StartTimeSeconds;
		if (elapsed < 0.0f) elapsed = 0.0f;

		float progress = elapsed / record.State.DurationSeconds;
		if (progress > 1.0f) progress = 1.0f;
		record.Progress = progress;

		if (progress >= 1.0f) {
			record.Completed		= true;
			record.State.Active	= false;
			_Completed.Add (record.State.BuildingId);
		}
	}
}


int BuildingConstructionService::Get_Completed_Count(void)			{ return _Completed.Count (); }

int BuildingConstructionService::Get_Completed_Building(int index)
{
	if (index < 0 || index >= _Completed.Count ()) return 0;
	return _Completed [index];
}

void BuildingConstructionService::Clear_Completed(void)
{
	//	The completed ids are consumed, and the records behind them go with them: a completed
	//	building is a finished building, and Is_Under_Construction must stop saying yes.
	for (int i = 0; i < _Completed.Count (); i++) {
		Cancel_Construction (_Completed [i]);
	}
	_Completed.Delete_All ();
}


bool BuildingConstructionService::Is_Under_Construction(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return false;
	return !_Records [index].Completed;
}


bool BuildingConstructionService::Get_Progress(int building_id,float * out_progress)
{
	int index = Find_Record (building_id);
	if (index < 0 || out_progress == nullptr) return false;
	*out_progress = _Records [index].Progress;
	return true;
}


BuildingConstructionStageType BuildingConstructionService::Get_Stage(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return CONSTRUCTION_STAGE_COMPLETE;

	const ConstructionRecord & record = _Records [index];
	if (record.Completed || record.Progress >= 1.0f)			return CONSTRUCTION_STAGE_COMPLETE;
	if (record.Progress < FOUNDATION_END)						return CONSTRUCTION_STAGE_FOUNDATION;
	if (record.Progress < record.SourceRevealStart)			return CONSTRUCTION_STAGE_BLOCKS;
	return CONSTRUCTION_STAGE_REVEAL;
}


int BuildingConstructionService::Get_Block_Count(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return 0;
	return _Records [index].Blocks.Count ();
}


bool BuildingConstructionService::Get_Block(int building_id,int block_index,BuildingConstructionBlock * out)
{
	int index = Find_Record (building_id);
	if (index < 0 || out == nullptr) return false;
	if (block_index < 0 || block_index >= _Records [index].Blocks.Count ()) return false;
	*out = _Records [index].Blocks [block_index];
	return true;
}


bool BuildingConstructionService::Get_Block_Render_State(int building_id,int block_index,
																			BuildingConstructionBlockRenderState * out)
{
	if (out == nullptr) return false;
	*out = BuildingConstructionBlockRenderState ();

	int index = Find_Record (building_id);
	if (index < 0) return false;
	if (block_index < 0 || block_index >= _Records [index].Blocks.Count ()) return false;

	const ConstructionRecord & record = _Records [index];
	const BuildingConstructionBlock & block = record.Blocks [block_index];

	float progress = record.Progress;

	if (progress < block.ProgressStart) {
		return true;			// a real answer: this block has not started, so do not batch it
	}

	float span = block.ProgressEnd - block.ProgressStart;
	float t = (span > 0.0f) ? ((progress - block.ProgressStart) / span) : 1.0f;
	if (t > 1.0f) t = 1.0f;
	if (t < 0.0f) t = 0.0f;

	/*
	**	"Blocks rise from slightly below final location, vertically scale into place, fade in,
	**	then settle."  All three from the same t, so they cannot disagree: a block is never
	**	fully opaque while still half-sunk.
	*/
	float alpha = t;

	//	The block fade at the end of the sequence, which runs against the whole building's
	//	progress rather than the block's own -- every block fades out together.
	if (progress >= record.BlockFadeStart) {
		float fade_span = 1.0f - record.BlockFadeStart;
		float fade = (fade_span > 0.0f) ? (1.0f - ((progress - record.BlockFadeStart) / fade_span)) : 0.0f;
		if (fade < 0.0f) fade = 0.0f;
		if (fade < alpha) alpha = fade;
	}

	if (alpha <= 0.0f) {
		return true;			// finished fading: also "do not batch"
	}

	float scale_z = 0.25f + (0.75f * t);
	float rise    = (block.HalfExtent.Z * 2.0f) * (1.0f - t);

	Vector3 local_center = block.LocalCenter;
	local_center.Z -= rise;

	Matrix3D transform = record.State.Transform;
	Vector3 world_center;
	Matrix3D::Transform_Vector (transform,local_center,&world_center);

	Matrix3D placed = transform;
	placed.Set_Translation (world_center);

	out->Visible		= true;
	out->Alpha			= alpha;
	out->Transform		= placed;
	out->HalfExtent	= Vector3 (block.HalfExtent.X,block.HalfExtent.Y,block.HalfExtent.Z * scale_z);
	return true;
}


float BuildingConstructionService::Get_Source_Reveal_Alpha(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return 1.0f;			// not under construction: the real model draws normally

	const ConstructionRecord & record = _Records [index];
	if (record.Completed) return 1.0f;

	if (record.Progress < record.SourceRevealStart) return 0.0f;

	float span = 1.0f - record.SourceRevealStart;
	if (span <= 0.0f) return 1.0f;

	float alpha = (record.Progress - record.SourceRevealStart) / span;
	if (alpha > 1.0f) alpha = 1.0f;
	if (alpha < 0.0f) alpha = 0.0f;
	return alpha;
}


int BuildingConstructionService::Get_Active_Row(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return -1;

	const ConstructionRecord & record = _Records [index];
	if (record.RowCount <= 0) return -1;

	if (record.Progress < FOUNDATION_END) return -1;
	if (record.Progress >= ROWS_END) return record.RowCount - 1;

	float row_span = (ROWS_END - FOUNDATION_END) / (float)record.RowCount;
	int row = (int)((record.Progress - FOUNDATION_END) / row_span);
	if (row < 0) row = 0;
	if (row >= record.RowCount) row = record.RowCount - 1;
	return row;
}


int BuildingConstructionService::Get_Row_Count(int building_id)
{
	int index = Find_Record (building_id);
	if (index < 0) return 0;
	return _Records [index].RowCount;
}


bool BuildingConstructionService::Get_Replication_State(int building_id,
																		  BuildingConstructionReplicationState * out)
{
	int index = Find_Record (building_id);
	if (index < 0 || out == nullptr) return false;
	*out = _Records [index].State;
	return true;
}


bool BuildingConstructionService::Get_Model_Bounds(int building_id,AABoxClass * out)
{
	int index = Find_Record (building_id);
	if (index < 0 || out == nullptr) return false;
	*out = _Records [index].ModelBounds;
	return true;
}


int BuildingConstructionService::Get_Active_Count(void)
{
	int count = 0;
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].InUse && !_Records [i].Completed) count++;
	}
	return count;
}

int BuildingConstructionService::Get_Active_Building(int index)
{
	int count = 0;
	for (int i = 0; i < _Records.Count (); i++) {
		if (!_Records [i].InUse || _Records [i].Completed) continue;
		if (count == index) return _Records [i].State.BuildingId;
		count++;
	}
	return 0;
}


const char * BuildingConstructionService::Get_Stage_Name(BuildingConstructionStageType stage)
{
	switch (stage) {
		case CONSTRUCTION_STAGE_FOUNDATION:	return "foundation";
		case CONSTRUCTION_STAGE_BLOCKS:		return "blocks";
		case CONSTRUCTION_STAGE_REVEAL:		return "reveal";
		case CONSTRUCTION_STAGE_COMPLETE:	return "complete";
		default:										return "unknown";
	}
}
