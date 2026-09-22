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
 *                     $Archive:: /Commando/Code/Combat/transportationnetworkvalidator.h      $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The gap report's item 6: validate the *network*, not each generator's own success flag.     *
 *	Roads, bridges and (eventually) tunnels can each independently report success while the     *
 *	transportation graph they jointly produced is still broken -- a road ending at an abutment  *
 *	nothing bound to, a bridge with no connected road, a bridge narrower than the road it        *
 *	carries, an approach steeper than anything is allowed to climb, a required pair of bases     *
 *	with no route between them at all.  Every check here reads state that already exists        *
 *	(RoadSystem, BridgeSystem, StrategicPlanner, WorldTraversabilityGraph); nothing here is a    *
 *	second copy of what any of them already owns.                                                *
 *                                                                                             *
 *	This is the "final connectivity validation result" ProceduralWorldManager's own per-pass     *
 *	log line now reports.  It runs after Phase 9 (bridge planning) and never blocks world        *
 *	generation -- every other generator in this chain already treats its own failures as         *
 *	"logged, world still stands"; a validator that refused to let a world exist over its own      *
 *	report would be the one inconsistent piece.  What it is for is making a broken network         *
 *	*visible* rather than silently accepted because every subsystem individually returned true.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TRANSPORTATIONNETWORKVALIDATOR_H
#define TRANSPORTATIONNETWORKVALIDATOR_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "wwstring.h"


struct TransportationValidationParams
{
	TransportationValidationParams(void);

	//	The same shared cap every terrain-reading generator in this chain already uses
	//	(profile.Terrain.MaxBuildableSlopeDegrees, in radians) -- an approach steeper than this
	//	is flagged rather than the validator inventing its own bridge-specific limit.
	float	MaxBuildableSlopeRadians;

	//	How far back from each abutment the approach grade is measured, world units.  Short
	//	enough that a road's own general climb toward higher ground is not mistaken for the
	//	bridge's approach; long enough that a single interpolated trim point does not read as a
	//	cliff.
	float	ApproachSampleDistance;
};


class TransportationNetworkValidator
{
public:

	/*
	**	Runs every check below in order, logs one WWDEBUG_SAY line per finding (never a flood --
	**	each check reports a count, not one line per instance beyond the first few), and returns
	**	true only when nothing was found.  `report_out`, when given, collects the same lines the
	**	log gets, for a caller (the self check family) that wants them without parsing the log.
	*/
	static bool			Validate(const TransportationValidationParams & params,StringClass * report_out);

	static void			Clear(void);

	/*
	**	Per-item results of the last Validate() call, so a self check can assert on one finding
	**	without parsing `report_out`'s text.
	*/
	static int			Get_Unbound_Abutment_Count(void)					{ return UnboundAbutmentCount; }
	static int			Get_Bridge_With_No_Road_Count(void)				{ return BridgeNoRoadCount; }
	static int			Get_Bridge_Narrower_Than_Road_Count(void)		{ return BridgeNarrowerCount; }
	static int			Get_Duplicate_Road_Through_Bridge_Count(void)	{ return DuplicateRoadCount; }
	static int			Get_Excessive_Approach_Grade_Count(void)		{ return ExcessiveGradeCount; }
	static int			Get_Insufficient_Clearance_Count(void)			{ return InsufficientClearanceCount; }

	//	Whether a required-base connectivity question was even askable (needs both a built
	//	traversable-vehicle graph and at least one base candidate per faction) and, if so,
	//	whether every faction pair the check tried was mutually reachable.
	static bool			Did_Check_Required_Base_Routes(void)				{ return RequiredBaseRoutesChecked; }
	static bool			Has_Required_Base_Routes(void)						{ return RequiredBaseRoutesConnected; }
	static int			Get_Unreachable_Passable_Cell_Count(void)		{ return UnreachableCellCount; }

	static bool			Passed(void)												{ return LastResult; }

private:

	static int			UnboundAbutmentCount;
	static int			BridgeNoRoadCount;
	static int			BridgeNarrowerCount;
	static int			DuplicateRoadCount;
	static int			ExcessiveGradeCount;
	static int			InsufficientClearanceCount;
	static bool			RequiredBaseRoutesChecked;
	static bool			RequiredBaseRoutesConnected;
	static int			UnreachableCellCount;
	static bool			LastResult;
};

#endif	// TRANSPORTATIONNETWORKVALIDATOR_H
