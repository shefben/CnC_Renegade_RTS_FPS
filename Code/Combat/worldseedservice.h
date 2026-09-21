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
 *                     $Archive:: /Commando/Code/Combat/worldseedservice.h                    $*
 *                                                                                             *
 *	V2 roadmap Section 34 Phase 1 -- Section 3B.1's RNG contract, made concrete and versioned    *
 *	rather than left implementation-defined.  This is the whole contract every later generator   *
 *	phase (2 onward) must consume instead of inventing its own seed derivation or picking its     *
 *	own RNG class:                                                                               *
 *                                                                                             *
 *	RNG ALGORITHM.  `Random4Class` (Code/wwlib/random.h) -- a from-scratch, non-library-         *
 *	dependent Mersenne Twister (MT19937) implementation already vendored in this tree, fully      *
 *	specified by its own source rather than by whatever `std::mt19937`/`rand()` a given            *
 *	compiler/runtime happens to ship, which is exactly what Section 3B.1 forbids relying on for    *
 *	client/server/platform parity.  A v2 generator stage constructs its own `Random4Class          *
 *	(Derive_Subsystem_Seed(world_seed,domain))` and draws from it directly; this service does      *
 *	not wrap draws, only seeds.                                                                    *
 *                                                                                             *
 *	SEED WIDTH.  32 bits (`unsigned int`), matching `ProceduralWorldProfile::Seed`,               *
 *	`Random4Class`'s own constructor parameter, and `CRC::String`/`CRC::Memory`'s return type      *
 *	(Code/wwlib/crc.h) -- one width used everywhere in this contract, no truncation/widening        *
 *	conversion anywhere in the chain.                                                              *
 *                                                                                             *
 *	SEED HASHING ALGORITHM.  `Derive_Subsystem_Seed(world_seed,domain) = CRC::String(domain,        *
 *	Normalize_Seed(world_seed))` -- the same non-cryptographic CRC (`Code/wwlib/crc.h`, already      *
 *	linked and already used by `cnetwork.cpp`/`ini.cpp` elsewhere in this tree) folding the           *
 *	domain string's bytes into an accumulator that starts from the normalized world seed.  Two       *
 *	different domain strings from the same world seed produce unrelated-looking 32-bit values;        *
 *	the same (world_seed,domain) pair always produces the same one, on any platform, since             *
 *	`CRC::String`'s table-driven algorithm has no floating point and no implementation-defined         *
 *	behavior.                                                                                          *
 *                                                                                             *
 *	SEED-DOMAIN STRINGS.  The `SEED_DOMAIN_*` constants below are the only legal domain values         *
 *	for `Derive_Subsystem_Seed` -- Section 4's own list plus Section 34 Phase 1 step 6's addition       *
 *	of a twelfth, combat-space repair.  A later phase introducing a new seed-consuming subsystem        *
 *	adds a new named constant here rather than passing an ad hoc string, so the full domain set          *
 *	stays discoverable in one place.                                                                    *
 *                                                                                             *
 *	INTEGER-TO-FLOAT CONVERSION.  `Random4Class::Get_Float()` (already implemented,                     *
 *	`draw * 2.3283064370807973754314699618685e-10f`, i.e. `draw / 2^32`) is the canonical            *
 *	unsigned-32-bit-draw-to-`[0,1)`-float conversion.  A generator needing a float in `[0,1)`             *
 *	calls that, never a hand-rolled `draw / (float)UINT32_MAX` or similar -- both round               *
 *	differently at the edges and this project does not need two.                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSEEDSERVICE_H
#define WORLDSEEDSERVICE_H

#include "always.h"


class WorldSeedService
{
	public:

		//	Section 4's eleven domains plus Phase 1 step 6's twelfth (combat-space repair).
		static const char * const SEED_DOMAIN_BOUNDARY;

		//	Added with the Phase 3 cliff ring.  Deliberately its own domain rather than sharing
		//	"boundary": the ring's rim noise would otherwise be drawn from the same stream that
		//	already shaped the polygon, correlating the two so that a bump in the outline always
		//	arrived with a matching bump in the cliff height.  Separate domains is what the
		//	per-subsystem derivation exists for.
		static const char * const SEED_DOMAIN_BOUNDARY_CLIFFS;
		static const char * const SEED_DOMAIN_TERRAIN;
		static const char * const SEED_DOMAIN_HYDROLOGY;
		static const char * const SEED_DOMAIN_ROADS;
		static const char * const SEED_DOMAIN_BRIDGES;
		static const char * const SEED_DOMAIN_TUNNELS;
		static const char * const SEED_DOMAIN_SETTLEMENTS;
		static const char * const SEED_DOMAIN_TIBERIUM;
		static const char * const SEED_DOMAIN_BASES;
		static const char * const SEED_DOMAIN_BIOMES;
		static const char * const SEED_DOMAIN_DECORATION;
		static const char * const SEED_DOMAIN_COMBAT_SPACE_REPAIR;

		/*
		**	A raw world seed of exactly 0 is indistinguishable from "never set" in the many
		**	places this codebase already treats a bare 0 that way (facility/network/object ids,
		**	`RuntimeDoorService`'s own `BuildingId` convention, etc.) -- so a 0 seed is
		**	normalized to 1 rather than accepted as-is.  Every other value passes through
		**	unchanged.  Deterministic and idempotent: normalizing an already-normalized seed is
		**	a no-op.
		*/
		static unsigned int	Normalize_Seed(unsigned int raw_world_seed);

		/*
		**	See the RNG contract above.  `domain` must be one of the SEED_DOMAIN_* constants;
		**	passing a domain string not in that set still produces a deterministic result (the
		**	formula does not special-case the value) but is not part of the documented contract
		**	and should not be done by generator code.
		*/
		static unsigned int	Derive_Subsystem_Seed(unsigned int world_seed,const char * domain);
};

#endif // WORLDSEEDSERVICE_H
