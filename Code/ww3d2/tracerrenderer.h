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
 *	Roadmap Section 27, Zero Hour / SAGE feature 12 -- tracer render module.                    *
 *                                                                                             *
 *	The ballistic-tracer half of Section 27.  What it replaced: the DisplayLaser branch of      *
 *	WeaponClass::Fire allocated a fresh Line3DClass and a fresh TimedDecorationPhysClass on      *
 *	every shot of every hitscan weapon that asked for a visible tracer, and added each one to    *
 *	the scene as its own object with its own draw call.  A firefight with ten rifles going is    *
 *	ten allocations and ten draw calls a second before anything else is counted.                *
 *                                                                                             *
 *	A tracer carries no simulation of its own.  Its start, end, width and colour are fixed at    *
 *	the instant it is fired -- WeaponClass has already done the raycast that decided where it    *
 *	ends -- so all a live tracer needs is those four things and when it dies.  This is a fixed   *
 *	pool of that record, aged by one shared clock and submitted each frame to                    *
 *	WorldParticleManager's streak pool, the same one the weather layer (Section 36) draws rain   *
 *	through.  Bullets stop paying for a RenderObjClass each; they pay for one array write.       *
 *                                                                                             *
 *	Ageing and drawing are deliberately two different calls.  Age() is expected once a           *
 *	simulation frame regardless of what is on screen, so a tracer's clock keeps moving while     *
 *	the camera is looking elsewhere and its bounding volume stays true; Render() only reads      *
 *	that state and submits it, so calling Render() twice against the same clock submits the      *
 *	same tracers, exactly as calling WeatherEnvironmentRenderer::Render() twice does.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TRACERRENDERER_H
#define TRACERRENDERER_H

#include "always.h"
#include "vector3.h"
#include "vector4.h"

class RenderInfoClass;


/*
**	A pool of world-space line segments with nothing but a clock deciding whether each is
**	still there.  Static, because there is one battlefield's worth of tracers, not one per
**	weapon or per owner -- the same reasoning WorldParticleManager itself is static for.
*/
class TracerRenderer
{
	public:

		static void	Init(void);
		static void	Shutdown(void);

		//	Clears every live tracer and the clock.  Used between self-check scenarios, so one
		//	scenario's tracers cannot outlive it into the next.
		static void	Reset(void);

		//	Off means every Add_Tracer call is dropped and nothing is drawn -- there is no
		//	per-tracer object left to fall back to, so this is the only sense in which the
		//	tracer layer can be absent.
		static void	Set_Enabled(bool onoff);
		static bool	Is_Enabled(void);

		//	How many tracers may be alive at once.  Hard-limited by the staging arrays; see
		//	TRACER_POOL_MAX in the .cpp.  A shot arriving over this is left un-drawn rather than
		//	displacing one already on screen -- the same direction of failure the weather layer
		//	chose, and for the same reason: a tracer popping out early is more wrong than one
		//	shot quietly not getting one.
		static void	Set_Max_Tracers(int count);
		static int	Get_Max_Tracers(void);

		/*
		**	Register one tracer.  'width' is the full width in world units, exactly the meaning
		**	Line3DClass gave its own width argument, so a caller carrying the old constant over
		**	draws the same size line.  'lifetime' is seconds; the tracer fades linearly to
		**	nothing over it rather than the flat cut the object it replaced ended on.
		*/
		static void	Add_Tracer(const Vector3 & start,const Vector3 & end,float width,
								 const Vector4 & color,float lifetime);

		//	Advances the shared clock and reaps whatever just expired.  Call once a simulation
		//	frame, independent of whether the tracer layer is currently on screen.
		static void	Age(float seconds);

		//	Submits everything still alive.  Pure with respect to the pool's own state: it reads
		//	what Age() last left and does not itself expire or mutate a record.
		static void	Render(RenderInfoClass & rinfo,const Vector3 & camera_position);

		static void	Get_Bounds(Vector3 & minimum,Vector3 & maximum);

		static int	Get_Live_Count(void);
		static int	Get_Drawn_Particles(void);
		static int	Get_Dropped_Count(void);
};

#endif // TRACERRENDERER_H
