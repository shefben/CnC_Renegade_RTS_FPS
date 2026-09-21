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
 *                     $Archive:: /Commando/Code/Combat/worldseedservice.cpp                  $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldseedservice.h"

#include "crc.h"


const char * const WorldSeedService::SEED_DOMAIN_BOUNDARY				= "boundary";
const char * const WorldSeedService::SEED_DOMAIN_BOUNDARY_CLIFFS	= "boundary_cliffs";
const char * const WorldSeedService::SEED_DOMAIN_TERRAIN				= "terrain";
const char * const WorldSeedService::SEED_DOMAIN_INTERIOR_CLIFFS	= "interior_cliffs";
const char * const WorldSeedService::SEED_DOMAIN_HYDROLOGY				= "hydrology";
const char * const WorldSeedService::SEED_DOMAIN_ROADS					= "roads";
const char * const WorldSeedService::SEED_DOMAIN_BRIDGES				= "bridges";
const char * const WorldSeedService::SEED_DOMAIN_TUNNELS				= "tunnels";
const char * const WorldSeedService::SEED_DOMAIN_SETTLEMENTS			= "settlements";
const char * const WorldSeedService::SEED_DOMAIN_TIBERIUM				= "tiberium";
const char * const WorldSeedService::SEED_DOMAIN_BASES					= "bases";
const char * const WorldSeedService::SEED_DOMAIN_BIOMES					= "biomes";
const char * const WorldSeedService::SEED_DOMAIN_DECORATION			= "decoration";
const char * const WorldSeedService::SEED_DOMAIN_COMBAT_SPACE_REPAIR	= "combat_space_repair";


unsigned int WorldSeedService::Normalize_Seed(unsigned int raw_world_seed)
{
	return (raw_world_seed == 0) ? 1u : raw_world_seed;
}


unsigned int WorldSeedService::Derive_Subsystem_Seed(unsigned int world_seed,const char * domain)
{
	unsigned int normalized = Normalize_Seed (world_seed);
	if ((domain == nullptr) || (domain [0] == 0)) {
		return normalized;
	}
	return CRC::String (domain,normalized);
}
