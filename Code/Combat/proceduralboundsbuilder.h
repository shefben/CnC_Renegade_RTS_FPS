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
 *                     $Archive:: /Commando/Code/Combat/proceduralboundsbuilder.h             $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 2 step 2 -- "generate/load one closed clockwise boundary         *
 *	polygon or spline."                                                                          *
 *                                                                                             *
 *	The split this file exists for: `WorldBoundsService` (Code/wwphys) owns the boundary's data  *
 *	and every query against it, because the movement layer that has to enforce it lives below    *
 *	Combat.  Deciding *what* the boundary is needs the Phase 1 profile and seed services, which  *
 *	live in Combat.  So generation is here and enforcement is there, and the two meet at one     *
 *	`WorldBoundsService::Configure` call -- not two boundary representations, one boundary       *
 *	handed across a layer edge, the same way `CommanderModeController::Set_World_Bounds` already  *
 *	worked before v2.                                                                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALBOUNDSBUILDER_H
#define PROCEDURALBOUNDSBUILDER_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"

#include "proceduralworldprofile.h"


class ProceduralBoundsBuilder
{
	public:

		/*
		**	Section 5.1's shape tags.  "Avoid a perfectly straight square wall unless the world
		**	profile explicitly requests it" -- so `irregular_ring` is the default and the
		**	rectangle is the opt-in, not the other way round.  An unrecognised tag is a
		**	validation failure rather than a silent fallback: a profile asking for a shape this
		**	generator does not have should say so, not quietly produce a different world than
		**	the author asked for.
		*/
		static const char * const SHAPE_IRREGULAR_RING;
		static const char * const SHAPE_ROUNDED_RING;
		static const char * const SHAPE_RECTANGLE;

		/*
		**	Builds the ring for a profile, deterministically: identical (seed, profile) always
		**	produces an identical vertex list, on any platform, because the only randomness is
		**	`Random4Class` seeded from `WorldSeedService::Derive_Subsystem_Seed(seed, "boundary")`
		**	and every vertex is a pure function of its own index (Section 3B.3 -- no shared
		**	mutable stream, no dependence on evaluation order).
		**
		**	The world is centred on the origin: X spans [-width/2, +width/2] and Y spans
		**	[-height/2, +height/2].  Nothing in the roadmap fixes an origin convention, and
		**	centring keeps the ring symmetric about the axes, which is what makes the derived
		**	band rings in `WorldBoundsService::Get_Zone_Ring` well behaved.
		*/
		static bool		Build_Boundary_Polygon(const ProceduralWorldProfile & profile,
															DynamicVectorClass<Vector2> * out_vertices,
															StringClass * error_out);

		/*
		**	Builds the polygon and installs it, so a caller creating a world has one call rather
		**	than a build/validate/configure dance it could get half right.  Returns false --
		**	leaving any previously configured boundary untouched -- if either the build or
		**	`WorldBoundsService::Configure`'s own validation refuses.
		*/
		static bool		Configure_World_Bounds(const ProceduralWorldProfile & profile,
															StringClass * error_out);
};

#endif // PROCEDURALBOUNDSBUILDER_H
