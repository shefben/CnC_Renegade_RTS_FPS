/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors
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
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *	Roadmap Section 33, Zero Hour / SAGE feature 20 -- debug rendering tools.                    *
 *                                                                                             *
 *	Donors: W3DDebugDisplay, W3DDebugIcons.  "Every new world/Commander subsystem has enough    *
 *	visualization to diagnose placement, culling, transform, and rendering errors" is a claim    *
 *	about *capability*, not about every one of the roadmap's eighteen named overlays having a    *
 *	real producer this pass -- most of the systems those overlays would visualize (building      *
 *	semantic nodes, purchase-terminal anchors, MCT anchors, the Commander cursor) do not exist    *
 *	yet.  So this file is the one thing every future producer needs and does not yet have: a     *
 *	named layer to gate on and a batched line/box/sphere primitive to submit into, both shared   *
 *	rather than reinvented per subsystem the way stock Renegade's ad hoc debug boxes were.        *
 *                                                                                             *
 *	Immediate mode, not handle-addressed like StatusOverlayRenderer (Section 32) -- a debug      *
 *	overlay is redrawn from live state every frame it is on, so there is nothing to hold a       *
 *	handle to between frames.  Begin_Frame clears the pool; whatever called Add_* since the       *
 *	last Begin_Frame is what Render submits, batched through WorldParticleManager's shared       *
 *	streak pool exactly like Section 27's tracers and Section 32's status rings.                  *
 *                                                                                             *
 *	This class knows nothing about which subsystem a layer belongs to -- no terrain, no roads,   *
 *	no wwphys of any kind.  A caller checks Is_Layer_Enabled itself before building geometry      *
 *	(cheap, and "development-only debug controls... are not legacy/new feature selectors" reads   *
 *	here as: nothing about gameplay ever consults these layers).                                 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef DEBUGRENDERSYSTEM_H
#define DEBUGRENDERSYSTEM_H

#include "always.h"
#include "vector3.h"
#include "vector4.h"

class RenderInfoClass;

/*
**	The roadmap's own eighteen names, Section 33, as one enumeration.
*/
enum DebugLayerType
{
	DEBUG_LAYER_TERRAIN_CHUNKS = 0,
	DEBUG_LAYER_TERRAIN_MATERIALS,
	DEBUG_LAYER_ROAD_SPLINES,
	DEBUG_LAYER_ROAD_TRIANGLES,
	DEBUG_LAYER_BRIDGE_SECTIONS,
	DEBUG_LAYER_WATER_VOLUMES,
	DEBUG_LAYER_FOLIAGE_CELLS,
	DEBUG_LAYER_SPATIAL_INDEX_CELLS,
	DEBUG_LAYER_DYNAMIC_LIGHT_RADII,
	DEBUG_LAYER_SHADOW_PROJECTORS,
	DEBUG_LAYER_COLLISION_BOUNDS,
	DEBUG_LAYER_BUILDING_SEMANTIC_NODES,
	DEBUG_LAYER_BUILDING_FOOTPRINTS,
	DEBUG_LAYER_PLACEMENT_OVERLAP,
	DEBUG_LAYER_PURCHASE_TERMINAL_ANCHORS,
	DEBUG_LAYER_MCT_ANCHORS,
	DEBUG_LAYER_COMMANDER_CURSOR_RAY,
	DEBUG_LAYER_SHROUD_CELLS,
	DEBUG_LAYER_RADAR_CONTACTS,

	//	V2 roadmap Section 34 Phase 2 step 18 / Section 5.10 -- the authoritative world boundary
	//	and every protected band derived from it, so a seam or a mis-sized band can be seen
	//	rather than inferred from a query result.
	DEBUG_LAYER_WORLD_BOUNDS,

	//	V2 roadmap Section 34 Phase 3 step 16.  The generated cliff ring: its stations, its
	//	rim line and the face between them.  Separate from world_bounds because the boundary is
	//	a topological fact and the ring is geometry that is supposed to explain it -- seeing one
	//	without the other is exactly how a mismatch between them would be spotted.
	DEBUG_LAYER_BOUNDARY_CLIFFS,

	//	V2 roadmap Section 34 Phase 5 step 13.  Interior cliffs, ravines and canyon walls --
	//	separate from boundary_cliffs for the reason that layer is separate from world_bounds:
	//	two different generators feed the one shared EscarpmentSystem, and seeing which cliffs
	//	came from which is exactly how a generator writing into the wrong owner's chunks would
	//	be spotted.
	DEBUG_LAYER_INTERIOR_CLIFFS,

	//	V2 roadmap Section 34 Phase 6 step 15.  Rivers/streams as their own centre line, lakes/
	//	ponds as their own ring, and a cross at every crossing candidate.
	DEBUG_LAYER_HYDROLOGY,

	//	V2 roadmap Section 34 Phase 10 step 15.  Each bore's outline, its two mouths, its portal
	//	stations and its light anchors.
	DEBUG_LAYER_TUNNELS,

	//	D-001 P32.  Each Commander expansion region as its ring, team-coloured.
	DEBUG_LAYER_EXPANSION_REGIONS,

	DEBUG_LAYER_COUNT		// sentinel
};

enum { DEBUG_LINE_POOL_MAX = 4096 };


class DebugRenderSystem
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		static void		Set_Layer_Enabled(DebugLayerType layer,bool onoff);
		static bool		Is_Layer_Enabled(DebugLayerType layer);
		static void		Disable_All_Layers(void);
		static const char *	Get_Layer_Name(DebugLayerType layer);

		//	Drops every line submitted since the last Begin_Frame.  A caller drives this once a
		//	frame before asking any subsystem to submit; Render draws whatever is left after it.
		static void		Begin_Frame(void);

		//	Bounded -- refuses rather than grows, same discipline as every pool in this project.
		//	Add_Box/Add_Sphere/Add_Cross decompose into Add_Line calls, so a caller never has to
		//	know the pool is lines underneath.
		static void		Add_Line(const Vector3 & p0,const Vector3 & p1,const Vector4 & color);
		static void		Add_Box(const Vector3 & minimum,const Vector3 & maximum,const Vector4 & color);
		static void		Add_Sphere(const Vector3 & center,float radius,const Vector4 & color);
		static void		Add_Cross(const Vector3 & point,float size,const Vector4 & color);

		static void		Render(RenderInfoClass & rinfo,const Vector3 & camera_position);

		static int		Get_Line_Count(void);
		static int		Get_Dropped_Count(void);
		static int		Get_Drawn_Segment_Count(void);
};

#endif // DEBUGRENDERSYSTEM_H
