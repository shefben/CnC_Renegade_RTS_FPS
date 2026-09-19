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
 *	Roadmap Section 32, Zero Hour / SAGE feature 17 -- waypoint / status-circle renderers.       *
 *                                                                                             *
 *	Donors: W3DStatusCircle, W3dWaypointBuffer.  One bounded pool of ground rings rather than    *
 *	a class per marker kind -- selection, target, move point, defend area, repair target,        *
 *	patrol point, construction footprint and placement validity are all the same geometry (a     *
 *	circle at a world point) with a different colour and a different owner, so they are one      *
 *	kind field on one record, the same shape of decision Section 35's surface-mark manager       *
 *	already made for scorch/dirt/oil.                                                           *
 *                                                                                             *
 *	This class knows nothing about who may see what -- no team, no shroud.  It has exactly the   *
 *	same shape as TracerRenderer next door: a handle-addressed pool, aged by one clock, drawn    *
 *	as line segments through WorldParticleManager's shared streak pool so a thousand rings cost  *
 *	one submission, not one draw call each.  "These visuals must be reusable by Commander Mode   *
 *	but are not permission/gameplay logic themselves" is the roadmap's own words for exactly     *
 *	this split: Set_Visible is the one hook a layer above this (Section 31's tactical model,     *
 *	via StatusOverlaySystem in Code/Combat) uses to say which rings a given viewer's shroud       *
 *	allows this frame, without this class ever knowing why.                                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef STATUSOVERLAYRENDERER_H
#define STATUSOVERLAYRENDERER_H

#include "always.h"
#include "vector3.h"
#include "vector4.h"

class RenderInfoClass;

/*
**	The roadmap's own list, Section 32, as one kind field rather than one class each.
*/
enum StatusOverlayKindType
{
	OVERLAY_SELECTION = 0,
	OVERLAY_TARGET,
	OVERLAY_MOVE_POINT,
	OVERLAY_DEFEND_AREA,
	OVERLAY_REPAIR_TARGET,
	OVERLAY_PATROL_POINT,
	OVERLAY_CONSTRUCTION_FOOTPRINT,
	OVERLAY_PLACEMENT_VALID,
	OVERLAY_PLACEMENT_INVALID,
};

typedef int StatusOverlayHandleType;


class StatusOverlayRenderer
{
	public:

		static void	Init(void);
		static void	Shutdown(void);

		//	Clears every overlay and the clock.  Used between self-check scenarios, the same
		//	reason TracerRenderer::Reset exists.
		static void	Reset(void);

		static void	Set_Enabled(bool onoff);
		static bool	Is_Enabled(void);

		//	Bounded, refuses rather than grows; see OVERLAY_POOL_MAX in the .cpp.
		static void	Set_Max_Overlays(int count);
		static int	Get_Max_Overlays(void);

		/*
		**	lifetime <= 0 means the ring stays until Remove_Overlay -- a selection ring, held
		**	for as long as a unit is selected, needs exactly that.  A positive lifetime fades
		**	and self-removes, the same convention WorldLightManager's dynamic lights use, for
		**	a transient marker like a one-shot "target acquired" flash.
		*/
		static StatusOverlayHandleType	Add_Overlay(StatusOverlayKindType kind,const Vector3 & position,
																	float radius,const Vector4 & color,float lifetime);
		static bool		Update_Overlay(StatusOverlayHandleType handle,const Vector3 & position,
												 float radius,const Vector4 & color);
		static bool		Remove_Overlay(StatusOverlayHandleType handle);
		static void		Remove_All(void);
		static bool		Is_Overlay(StatusOverlayHandleType handle);
		static StatusOverlayKindType	Get_Overlay_Kind(StatusOverlayHandleType handle);
		static Vector3	Get_Overlay_Position(StatusOverlayHandleType handle);

		//	Per-frame draw eligibility.  True the instant a handle is created; a caller with a
		//	visibility policy (team ownership, shroud) clears this before Render and never
		//	touches anything else about the slot.  This is deliberately not folded into
		//	Remove_Overlay -- a ring losing shroud coverage is not the same event as its owner
		//	deciding it no longer exists.
		static void		Set_Visible(StatusOverlayHandleType handle,bool visible);
		static bool		Is_Visible(StatusOverlayHandleType handle);

		//	Ages lifetimes and reaps whatever just expired.  Call once a simulation frame,
		//	independent of whether anything is currently on screen -- same split as
		//	TracerRenderer::Age versus Render.
		static void		Age(float seconds);

		//	Submits every InUse, enabled, currently-visible ring as batched line segments.
		//	Pure with respect to the pool: reads what Age()/Set_Visible() last left, does not
		//	itself expire or mutate a record.
		static void		Render(RenderInfoClass & rinfo,const Vector3 & camera_position);

		static void		Get_Bounds(Vector3 & minimum,Vector3 & maximum);

		static int		Get_Live_Count(void);
		static int		Get_Drawn_Segment_Count(void);
		static int		Get_Dropped_Count(void);
};

#endif // STATUSOVERLAYRENDERER_H
