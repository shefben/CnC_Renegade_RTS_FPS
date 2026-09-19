/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/constructionrenderer.h                $*
 *                                                                                             *
 *	Roadmap Section 44, Phase 38 -- the drawing half of procedural construction build-up.       *
 *                                                                                             *
 *	`BuildingConstructionService` already says where every block is, how big it is this frame,   *
 *	and how opaque.  This file turns that into geometry and decides nothing.                    *
 *                                                                                             *
 *	Section 44: "Do not create one RenderObj or physics object per block.  Use one or a small    *
 *	bounded number of dynamic meshes, e.g. completed rows batch / current animated row batch."    *
 *	That is exactly what this is: **two** `DynamicMeshClass` objects per building under            *
 *	construction, whatever the block count. The settled batch holds every row below the one        *
 *	currently assembling and is rebuilt only when the active row advances; the active batch holds  *
 *	the row that is moving and is rebuilt each frame. No block is ever a `RenderObjClass` and no    *
 *	block is ever a `PhysClass` -- the construction's collision is one footprint blocker owned      *
 *	elsewhere, never per-block.                                                                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef CONSTRUCTIONRENDERER_H
#define CONSTRUCTIONRENDERER_H

#include "always.h"


class BuildingConstructionRenderer
{
	public:

		/*
		**	Per-batch block budget.  A bounded pool that drops and counts past its bound rather
		**	than growing or asserting -- the same discipline `DebugRenderSystem`'s line pool uses,
		**	and for the same reason: a descriptor can legally ask for `max_blocks` 2048, and a
		**	renderer that tried to honour that in one mesh would allocate 49k vertices per
		**	building. Dropping the overflow makes a very tall building look slightly sparse; the
		**	alternative makes it stutter.
		*/
		enum { MAX_BLOCKS_PER_BATCH = 512 };

		//	Same contract as HUDClass::Init: false on a dedicated server or a device-less check,
		//	and then nothing here allocates a mesh or touches the scene.
		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		//	Called once a frame on a client that has a scene.  Creates, rebuilds and retires the
		//	batches to match the service's active constructions.  A dedicated server never calls
		//	it and allocates nothing.
		static void		Update(void);

		static int		Get_Batch_Count(void);				// live DynamicMeshClass objects
		static int		Get_Rendered_Block_Count(void);	// blocks emitted on the last Update
		static int		Get_Dropped_Block_Count(void);	// blocks past the per-batch budget
};

#endif // CONSTRUCTIONRENDERER_H
