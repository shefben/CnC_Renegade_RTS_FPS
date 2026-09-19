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
 *	Roadmap Section 28, Zero Hour / SAGE feature 13 -- debris rendering.                        *
 *                                                                                             *
 *	The roadmap names two tiers.  This file is VISUAL_DEBRIS: a fixed pool of camera-facing     *
 *	chips with a simple ballistic arc and no collision, submitted through WorldParticleManager  *
 *	exactly the way TracerRenderer submits its tracers -- one array write per fragment instead  *
 *	of a RenderObjClass each.  GAMEPLAY_DEBRIS, the tier where collision matters, is real       *
 *	WWPhys fragments and lives in Code/Combat/debrissystem.h instead, because it needs the      *
 *	combat scene and the asset manager, neither of which this layer may depend on.              *
 *                                                                                             *
 *	A chip carries no identity worth keeping: where it started, how it is moving, how it is     *
 *	spinning and when it dies.  Age() integrates that arc and reaps whatever just expired, once  *
 *	a simulation frame regardless of whether the layer is on screen, for the same reason         *
 *	TracerRenderer::Age() is separate from Render() -- a bounding volume the scene culls by has  *
 *	to stay true while the object is off screen.  Render() only reads what Age() last left.      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef DEBRISRENDERER_H
#define DEBRISRENDERER_H

#include "always.h"
#include "vector3.h"
#include "vector4.h"

class RenderInfoClass;


/*
**	A pool of falling, spinning, fading chips with nothing but a clock and a ballistic arc
**	deciding where each is and whether it is still there.  Static for the same reason
**	WorldParticleManager and TracerRenderer are: there is one battlefield's worth of debris,
**	not one pool per destroyed object.
*/
class DebrisRenderer
{
	public:

		static void	Init(void);
		static void	Shutdown(void);

		//	Clears every live chip and the clock.  Used between self-check scenarios, so one
		//	scenario's debris cannot outlive it into the next.
		static void	Reset(void);

		//	Off means every Spawn call is dropped and nothing is drawn -- there is no per-chip
		//	object left to fall back to, so this is the only sense in which the layer can be
		//	absent.
		static void	Set_Enabled(bool onoff);
		static bool	Is_Enabled(void);

		//	How many chips may be alive at once across every destroyed object put together.
		//	Hard-limited by the staging arrays; see DEBRIS_POOL_MAX in the .cpp.  A scatter
		//	arriving over this is thinned rather than displacing chips already on screen -- the
		//	same direction of failure TracerRenderer and the weather layer both chose.
		static void	Set_Max_Debris(int count);
		static int	Get_Max_Debris(void);

		/*
		**	Scatters up to 'count' chips in a sphere of 'radius' around 'center', each with an
		**	independently randomised ballistic kick and in-plane spin, coloured 'color' and
		**	fading linearly to nothing over 'lifetime' seconds -- the same fade TracerRenderer
		**	gives a tracer instead of the flat cut stock content used.  Chips over the pool's cap
		**	are dropped and counted, not the chips already alive.
		*/
		static void	Spawn(const Vector3 & center,float radius,int count,
							  const Vector4 & color,float lifetime);

		//	Advances the shared clock, integrates gravity into every live chip and reaps whatever
		//	just expired.  Call once a simulation frame, independent of whether the layer is
		//	currently on screen.
		static void	Age(float seconds);

		//	Submits everything still alive.  Pure with respect to the pool's own state: it reads
		//	what Age() last left and does not itself expire or move a chip.
		static void	Render(RenderInfoClass & rinfo,const Vector3 & camera_position);

		static void	Get_Bounds(Vector3 & minimum,Vector3 & maximum);

		static int	Get_Live_Count(void);
		static int	Get_Drawn_Particles(void);
		static int	Get_Dropped_Count(void);
};

#endif // DEBRISRENDERER_H
