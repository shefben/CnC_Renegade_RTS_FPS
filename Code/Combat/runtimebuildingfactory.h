/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/runtimebuildingfactory.h              $*
 *                                                                                             *
 *	Roadmap Section 44, Phase 38 / Q-016 -- creating the canonical building at accepted          *
 *	placement.                                                                                  *
 *                                                                                             *
 *	This is the half of Phase 38 that was missing.  `BuildingConstructionService` owned the      *
 *	timing and the procedural block build-up and `CommanderPlacementValidator` owned the         *
 *	verdict, but nothing ever produced a `BuildingGameObj`, so a finished construction left      *
 *	behind a hole where a building should be.                                                   *
 *                                                                                             *
 *	It does exactly three things and owns no state:                                             *
 *                                                                                             *
 *	  - resolve the descriptor's `game_object_preset` to a real building preset, so a            *
 *	    Commander-built Power Plant is a `PowerPlantGameObj` with the capabilities its class     *
 *	    carries rather than a bare controller wearing a power-plant model;                       *
 *	  - create it through `DefinitionClass::Create`, the one production primitive every other    *
 *	    runtime spawn in this codebase already shares (see                                       *
 *	    `Drain_Completed_Commander_Production` in combat.cpp);                                   *
 *	  - hand it to `BuildingGameObj::Init_Runtime_Building`, which takes the second collection   *
 *	    path and never touches the map-authored one.                                             *
 *                                                                                             *
 *	Server-only.  Clients receive the building through `BuildingGameObj::Export_Creation`, which *
 *	carries the descriptor name and transform precisely so that a client can take the same       *
 *	runtime path without ever being told which preset to instantiate.                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef RUNTIMEBUILDINGFACTORY_H
#define RUNTIMEBUILDINGFACTORY_H

#include "always.h"
#include "aabox.h"
#include "matrix3d.h"

class BuildingGameObj;

class RuntimeBuildingFactory
{
	public:

		/*
		**	Create the building in its under-construction state: real, solid, registered, owned by
		**	`team` -- and with its model hidden, because Section 44's build-up is what the player
		**	watches until the reveal.  Returns nullptr and logs rather than half-creating anything.
		*/
		static BuildingGameObj *	Create_Constructing_Building(const char * descriptor_name,
																			  const Matrix3D & transform,int team);

		//	Section 44's "real model revealed", plus the capability gate opening.  Safe to call on
		//	a building that was already activated, and on one that never had a model to reveal.
		static bool		Activate_Completed_Building(int building_id);

		/*
		**	The model bounds the procedural block layout is generated from.  Read off the real
		**	model when there is one, and derived from the descriptor's footprint when the art is
		**	missing -- the blocks still build up over the right area either way.
		*/
		static bool		Get_Construction_Bounds(const BuildingGameObj * building,
															 const char * descriptor_name,AABoxClass * out_bounds);

		//	Counters, for the self-checks and for `building_status`.
		static int		Get_Created_Count(void);
		static int		Get_Refused_Count(void);
		static int		Get_Activated_Count(void);
		static void		Reset_Counters(void);
};

#endif // RUNTIMEBUILDINGFACTORY_H
