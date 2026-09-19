/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/buildingconstruction.h                $*
 *                                                                                             *
 *	Roadmap Section 44, Phase 38 -- authoritative construction + procedural block build-up.     *
 *                                                                                             *
 *	The heart of Section 44 is one sentence: "Clients deterministically reconstruct blocks."     *
 *	Everything else follows from it.  If a client can rebuild the exact block layout from a      *
 *	handful of replicated numbers, then the network never carries a block transform, a late      *
 *	joiner needs nothing but the same numbers, and a save file stores the numbers rather than     *
 *	the geometry.  So the layout generator here is a pure function of                            *
 *	(model bounds, construction_visual block, seed) and contains no rand(), no clock, no          *
 *	floating-point accumulation across calls, and nothing read from a device.                    *
 *                                                                                             *
 *	The other half is the gate.  "A building under construction provides none of those            *
 *	finished-building capabilities" is not a presentation rule, it is the reason this service     *
 *	exists at all -- so Is_Under_Construction is a query anything with a capability asks, and     *
 *	Phase 34's power grid, Phase 36's sidebar and Phase 39's production service all ask it.       *
 *                                                                                             *
 *	This file owns state and arithmetic, never geometry.  It says where block 47 is, how big      *
 *	it is and how opaque it should be at progress 0.63; it does not own a mesh, and it does not   *
 *	create one.  A renderer batches what Get_Block/Get_Block_Render_State describe -- Section     *
 *	44's "one or a small bounded number of dynamic meshes", never one object per block.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BUILDINGCONSTRUCTION_H
#define BUILDINGCONSTRUCTION_H

#include "always.h"
#include "aabox.h"
#include "matrix3d.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"


enum BuildingConstructionStageType
{
	CONSTRUCTION_STAGE_FOUNDATION = 0,	// 0.00 - 0.10 by default
	CONSTRUCTION_STAGE_BLOCKS,				// 0.10 - 0.80
	CONSTRUCTION_STAGE_REVEAL,				// source_reveal_start - 1.00, overlapping the blocks
	CONSTRUCTION_STAGE_COMPLETE,
};


/*
**	One logical block.  Section 44's own required per-block data: "deterministic final center,
**	size, row, order, and progress interval".  Local to the building, not world space -- the
**	world transform is applied last, which is what makes "rotated buildings must generate
**	rotated visuals" free rather than a second code path.
*/
struct BuildingConstructionBlock
{
	BuildingConstructionBlock(void) : LocalCenter(0.0f,0.0f,0.0f),HalfExtent(0.5f,0.5f,0.5f),
												 Row(0),Order(0),ProgressStart(0.0f),ProgressEnd(0.0f) { }

	bool	operator == (const BuildingConstructionBlock &) const	{ return false; }
	bool	operator != (const BuildingConstructionBlock &) const	{ return true; }

	Vector3	LocalCenter;
	Vector3	HalfExtent;
	int		Row;
	int		Order;				// deterministic within-row order, from the seed
	float		ProgressStart;		// normalized construction progress this block starts rising at
	float		ProgressEnd;		// and has fully settled at
};


/*
**	What a renderer needs for one block this frame, all of it derived and none of it stored.
**	`Visible` false means "do not batch this block at all", which is both a block whose row has
**	not started and a block that has finished fading out.
*/
struct BuildingConstructionBlockRenderState
{
	BuildingConstructionBlockRenderState(void) : Visible(false),Alpha(0.0f),
																 Transform(1),HalfExtent(0.0f,0.0f,0.0f) { }

	bool			Visible;
	float			Alpha;
	Matrix3D		Transform;		// world space, including the rise-from-below offset
	Vector3		HalfExtent;		// the block's size *this frame* -- it scales vertically into place

	//	Kept as a separate extent rather than baked into Transform as a non-uniform scale on
	//	purpose: a batching renderer emits eight corners per block from a centre, an extent and
	//	an orientation, and a Matrix3D carrying a scale would make every consumer undo it first.
};


/*
**	Exactly what Section 44 says the network carries, and nothing else.  No block appears in
**	this struct, which is the point.
*/
struct BuildingConstructionReplicationState
{
	BuildingConstructionReplicationState(void) : BuildingId(0),Transform(1),StartTimeSeconds(0.0f),
																 DurationSeconds(0.0f),Seed(0),Active(false),
																 DescriptorName("") { }

	int			BuildingId;
	Matrix3D		Transform;
	float			StartTimeSeconds;		// server time the construction began
	float			DurationSeconds;
	unsigned int	Seed;
	bool			Active;
	StringClass	DescriptorName;		// stands in for Section 44's "definition id ... descriptor revision"
};


class BuildingConstructionService
{
	public:

		//	Section 44's default normalized staging.  Overridable per descriptor through the
		//	construction_visual block's own reveal/fade values; the two row bounds are fixed
		//	because nothing in the roadmap's JSON exposes them.
		static const float	FOUNDATION_END;		// 0.10
		static const float	ROWS_END;				// 0.80

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	The layout generator.  Pure: same arguments in, same blocks out, on any machine, in
		**	any order, any number of times.  `model_bounds` is the real building's local bounds
		**	(a renderer has them from the prototype; the self check supplies them directly),
		**	which is Section 44's "generate from ... model bounds" rather than from the
		**	footprint, because a footprint has no height.
		**
		**	Respects ConstructionVisual.MaxBlocks by growing the block size until the count
		**	fits, never by truncating the list -- a truncated list would build a building with
		**	a missing corner, which is worse than a coarser one.
		*/
		static int		Generate_Blocks(const char * descriptor_name,const AABoxClass & model_bounds,
												  unsigned int seed,DynamicVectorClass<BuildingConstructionBlock> & out);

		/*
		**	Lifecycle.  Begin_Construction is server-authoritative; Begin_Construction_From_State
		**	is what a client or a late joiner calls with the replicated numbers, and produces an
		**	identical record.  They are separate entry points rather than one with a flag
		**	because only one of them is allowed to choose the seed.
		*/
		static bool		Begin_Construction(int building_id,const char * descriptor_name,
													  const Matrix3D & transform,const AABoxClass & model_bounds,
													  float server_time_seconds,unsigned int seed);

		static bool		Begin_Construction_From_State(const BuildingConstructionReplicationState & state,
																	 const AABoxClass & model_bounds);

		//	Cancellation and destruction.  Section 44: "no half-built object may provide
		//	power/radar/production/defense/wallet capacity", and "cancellation/destruction
		//	leaves no temporary state" -- both are this one call dropping the record, which is
		//	the only place any of that temporary state lives.
		static void		Cancel_Construction(int building_id);

		/*
		**	Advance every active construction to `server_time_seconds` and complete any that
		**	have reached 1.0.  Completions are reported through Get_Completed_Count/
		**	Get_Completed_Building rather than a callback, so that the activation order
		**	Section 44 requires is the caller's to run, in one place, in the order the roadmap
		**	lists -- not scattered across whichever subsystems registered a listener.
		*/
		static void		Update(float server_time_seconds);

		static int		Get_Completed_Count(void);
		static int		Get_Completed_Building(int index);
		static void		Clear_Completed(void);

		//	The gate every capability asks.  False for a building this service has never heard
		//	of, which is every building that was placed by a level rather than built.
		static bool		Is_Under_Construction(int building_id);

		static bool		Get_Progress(int building_id,float * out_progress);
		static BuildingConstructionStageType	Get_Stage(int building_id);

		static int		Get_Block_Count(int building_id);
		static bool		Get_Block(int building_id,int index,BuildingConstructionBlock * out);

		/*
		**	Where block `index` is and how opaque it is right now.  The rise-from-below and
		**	vertical-scale-into-place Section 44 describes are computed here, so every renderer
		**	animates identically and the animation is checkable without drawing anything.
		*/
		static bool		Get_Block_Render_State(int building_id,int index,
															BuildingConstructionBlockRenderState * out);

		//	Whole-building alpha for the real model's reveal -- Section 44's "whole-building
		//	alpha reveal is the reliable first implementation".
		static float	Get_Source_Reveal_Alpha(int building_id);

		//	The row a renderer should batch separately this frame: everything below it is
		//	settled and can live in one static batch, this one is animating.  Section 44's
		//	"completed rows batch / current animated row batch".
		static int		Get_Active_Row(int building_id);
		static int		Get_Row_Count(int building_id);

		static bool		Get_Replication_State(int building_id,BuildingConstructionReplicationState * out);

		//	The bounds this construction's layout was generated from -- what Phase 40 writes so
		//	a load can regenerate the identical layout with no model resident.
		static bool		Get_Model_Bounds(int building_id,AABoxClass * out);

		static int		Get_Active_Count(void);
		static int		Get_Active_Building(int index);

		static const char *	Get_Stage_Name(BuildingConstructionStageType stage);
};

#endif // BUILDINGCONSTRUCTION_H
