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
 *                     $Archive:: /Commando/Code/Combat/proceduralworldtools.h                $*
 *                                                                                             *
 *	V2 roadmap Phase 27 -- procedural-world toolchain and reproducibility tools (Section 27).    *
 *                                                                                             *
 *	Section 27.3's validation report is not re-derived: `CombatSpaceValidator` and                *
 *	`TransportationNetworkValidator` already keep their last Validate() call's metrics/failure    *
 *	counts as queryable static state (combatspacevalidator.h, transportationnetworkvalidator.h)   *
 *	-- Build_Validation_Report only reads it.                                                     *
 *                                                                                             *
 *	Section 27.1/27.2/27.4's preview/step-viewer/overlay requirement is one CPU-side top-down       *
 *	raster tool, not three separate ones: `Write_Top_Down_Preview`'s `layer_mask` both toggles       *
 *	which generated layer shows (Section 27.2's "viewing after" a given stage, since every stage's   *
 *	own data survives to the end of generation and nothing here needs to freeze mid-pipeline) and     *
 *	is the overlay inventory Section 27.4 lists, in the one form a headless build with no display      *
 *	and no existing debug-draw hook (confirmed absent before this phase) can honestly produce: a        *
 *	deterministic image file rather than a live in-engine HUD toggle.  The pixel-math primitives         *
 *	(`World_To_Pixel`, `Plot_*`, `Fill_Buffer`) are pure and self-check-covered; `Write_Top_Down_         *
 *	Preview` itself is the real walk against the live generated-world singletons (WorldTerrainSystem,     *
 *	WaterSystem, StrategicPlanner, RoadGraphGenerator, BridgePlanner, BaseSelectionPlanner,                *
 *	TiberiumFieldGenerator, SettlementGenerator) and is untestable in the self-check harness for the        *
 *	same disclosed reason every other phase's real walk against live generator state has been since         *
 *	Phase 24.                                                                                                 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALWORLDTOOLS_H
#define PROCEDURALWORLDTOOLS_H

#include <cstdio>

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "wwstring.h"

class AABoxClass;


//	Section 27.3.  Flat copy of what CombatSpaceValidator/TransportationNetworkValidator already
//	computed and cached -- no re-derivation, no new thresholds.
struct ProceduralWorldValidationReport
{
	ProceduralWorldValidationReport(void);

	unsigned int	GeneratorVersion;
	unsigned int	Seed;
	unsigned int	ProfileHash;
	unsigned int	ContentManifestHash;
	unsigned int	HybridPlanningInputHash;	// 0 for a legacy world
	unsigned int	HybridContentRevision;

	bool		CombatSpacePassed;
	int		CombatSpaceFailureCount;
	int		SightlineSampleCount;
	float		SightlineP50;
	float		SightlineP90;
	float		SightlineMax;
	float		BaseToBaseTravelTimeSeconds;
	bool		SingleBridgeDependency;
	int		OwnedFieldsWithoutRoute;
	int		SettlementsWithoutRoadConnection;

	bool		TransportPassed;
	int		UnboundAbutmentCount;
	int		BridgeNoRoadCount;
	int		BridgeNarrowerCount;
	int		DuplicateRoadCount;
	int		ExcessiveGradeCount;
	int		InsufficientClearanceCount;
	bool		RequiredBaseRoutesChecked;
	bool		RequiredBaseRoutesConnected;
	int		UnreachableCellCount;
};


//	Section 27.1/27.2/27.4.  Bit flags: which generated layer(s) to draw into the raster.
typedef enum
{
	PREVIEW_LAYER_HEIGHT			= 1 << 0,
	PREVIEW_LAYER_CLIFFS			= 1 << 1,
	PREVIEW_LAYER_WATER			= 1 << 2,
	PREVIEW_LAYER_ROADS			= 1 << 3,
	PREVIEW_LAYER_BRIDGES		= 1 << 4,
	PREVIEW_LAYER_BASES			= 1 << 5,
	PREVIEW_LAYER_TIBERIUM		= 1 << 6,
	PREVIEW_LAYER_SETTLEMENTS	= 1 << 7,
	PREVIEW_LAYER_BOUNDARY		= 1 << 8,
	PREVIEW_LAYER_ALL				= 0x1FF,
} ProceduralWorldPreviewLayerType;


//	Roadmap Phase 27's own "deterministic seed batch runner for offline validation" -- one row
//	per seed actually attempted.  Step 9: the batch stops (returns fewer than `count` rows)
//	rather than reporting a hard failure and continuing to grind through the rest.
struct ProceduralWorldSeedBatchResult
{
	ProceduralWorldSeedBatchResult(void);

	unsigned int	Seed;
	bool				GenerateSucceeded;
	bool				CombatSpacePassed;
	bool				TransportPassed;
};


class ProceduralWorldToolsService
{
	public:

		//	Section 27.3.  Reads CombatSpaceValidator/TransportationNetworkValidator's own cached
		//	static state (their last Validate() call, run by whoever generated the active world) and
		//	the active WorldGenerationManifest -- no side effects, self-check-testable once a self
		//	check has run Validate() itself with synthetic params (see shaderselfcheck.cpp).
		static ProceduralWorldValidationReport	Build_Validation_Report(void);

		static bool			Serialize_Validation_Report(const ProceduralWorldValidationReport & report,StringClass * out_json);
		static StringClass	Format_Validation_Report_Text(const ProceduralWorldValidationReport & report);

		//	Real walk: Build_Validation_Report + Serialize_Validation_Report + a file write.
		static bool			Write_Validation_Report_To_File(const char * file_path,StringClass * error_out);

		//	Section 27.5.  WorldGenerationManifestService::Format_Reproduction_String on the active
		//	world's own manifest -- the console-visible form of the string ProceduralWorldManager
		//	now also logs on every generation failure (proceduralworldmanager.cpp), not only success.
		static StringClass	Get_Reproduction_String(void);

		/*
		**	Pure pixel-math primitives.  `bounds` is the world-space rectangle the raster covers
		**	(only .Center/.Extent .X/.Y are read -- ground plane, the same (x,y) convention
		**	WorldTerrainSystem::Sample_Height already uses).  Out-of-bounds pixels are silently
		**	clipped, never written -- a marker just outside the sampled rectangle is not a crash.
		*/
		static bool			World_To_Pixel(const AABoxClass & bounds,float world_x,float world_y,
													  int width,int height,int * px_out,int * py_out);
		static void			Fill_Buffer(unsigned char * rgb,int width,int height,
												unsigned char r,unsigned char g,unsigned char b);
		static void			Plot_Point(unsigned char * rgb,int width,int height,int px,int py,
												unsigned char r,unsigned char g,unsigned char b);
		static void			Plot_Filled_Square(unsigned char * rgb,int width,int height,int px,int py,int half_size,
														  unsigned char r,unsigned char g,unsigned char b);
		static void			Plot_Line(unsigned char * rgb,int width,int height,int px0,int py0,int px1,int py1,
												unsigned char r,unsigned char g,unsigned char b);

		//	Raw binary PPM (P6) -- no image library dependency, three bytes a pixel, trivially
		//	reproducible.  Not self-check-covered (real disk I/O; see persistentcampaign.h's own
		//	Save_To_File for the same disclosed exception in this codebase).
		static bool			Write_PPM_Buffer_To_File(const char * file_path,const unsigned char * rgb,
																 int width,int height,StringClass * error_out);

		//	Real walk.  Builds the buffer from the live generated-world singletons named in this
		//	header's own comment above, honoring `layer_mask`, then calls Write_PPM_Buffer_To_File.
		//	`resolution` is clamped to [64,1024] -- large enough to be useful, small enough that a
		//	dedicated server never pays for a giant allocation by accident.
		static bool			Write_Top_Down_Preview(const char * file_path,unsigned int layer_mask,int resolution,
																  StringClass * error_out);

		//	Real walk.  Section 27's own seed batch runner: Destroy_World/Request_World/Generate_World
		//	in a loop against whichever level is already loaded (proceduralworldmanager.h's own
		//	Destroy_World never touches the level/physics scene, only what it itself built), recording
		//	one result per seed and stopping outright -- not merely skipping -- on the first hard
		//	Generate_World failure.  Untested in the self-check harness: there is no physics scene
		//	there, the same reason Launch_Territory_Battle (Phase 26) is untested too.
		enum { MAX_SEED_BATCH_RESULTS = 64 };
		static int			Run_Seed_Batch(unsigned int start_seed,int count,const char * profile_path,
													  ProceduralWorldSeedBatchResult * results_out,StringClass * error_out);

		/*
		**	D-033: the production census.  The same loop as Run_Seed_Batch -- Destroy_World, Request_World and the
		**	game's own Generate_World with its bounded retries -- over requested seeds in ascending order, but it never
		**	stops on a failed seed: it prints every attempt's identity (requested and effective seed, archetype, biome,
		**	the stage it reached and why it stopped, the manifest's reproduction string with the profile hash, generator
		**	version and content revision, and each finished phase's checksum) to `out`, then the counts.  It stops after
		**	`count` seeds, or at the first requested seed whose loaded world passed Phase 12 when `stop_at_phase12`.
		**	Needs the game's content mounted (always.dat's definitions): run it from the real client or server process.
		**	Returns the first such requested seed, or 0.
		*/
		static unsigned int	Run_Seed_Census(unsigned int first_seed,int count,const char * profile_path,bool stop_at_phase12,
														 FILE * out);

		//	One line naming the generated world a requested seed produced: requested and effective seed and identity.
		//	`out` null writes to the world log (Set_World_Log; stdout until set).
		static void			Print_World_Identity(FILE * out,const char * prefix);
		static void			Set_World_Log(FILE * log);
};

#endif	// PROCEDURALWORLDTOOLS_H
