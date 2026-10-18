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
 *                     $Archive:: /Commando/Code/Combat/proceduralworldmanager.h              $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The procedural world owner.  V2 roadmap Section 34, from Phase 3 onward.                     *
 *                                                                                             *
 *	Phases 0 to 2 produced services -- a profile parser, a seed deriver, a manifest, a boundary  *
 *	-- and no one thing that decides a world exists and drives them in order.  Phase 3 step 1    *
 *	asks for a cliff component "under the procedural-world/terrain owner", which is the first    *
 *	time that owner is actually needed.  This is it, and every later phase's generation stage    *
 *	hangs off Generate_World rather than finding its own way into the level load.                *
 *                                                                                             *
 *	Directive 1.10 is the whole shape of this class.  A hand-authored Renegade map must keep     *
 *	working untouched, so nothing here runs unless something explicitly asked for a procedural   *
 *	world: Request_World is that explicit ask, and with no request outstanding Generate_World    *
 *	returns immediately having configured nothing.  The boundary stays unconfigured, the cliff   *
 *	ring stays unbuilt, and the level loads exactly as it did before v2 existed.                 *
 *                                                                                             *
 *	Where the request comes from.  The main menu's procedural button, which sets the request     *
 *	before starting the game, and the `procedural_world` console command.  Both are client-side  *
 *	entry points into a server-side generation: the generation itself runs inside the level      *
 *	load, on whichever side is loading.                                                          *
 *                                                                                             *
 *	Why generation is not replicated.  Section 3B exists precisely so it does not have to be.    *
 *	Every client deriving the same world from the same seed and profile is the determinism       *
 *	contract working; sending a generated world over the wire would be sending something both    *
 *	ends can compute.  What must agree is the seed and the profile, and the manifest is what     *
 *	makes disagreement detectable rather than silent.                                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALWORLDMANAGER_H
#define PROCEDURALWORLDMANAGER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "proceduralworldprofile.h"
#include "worldgenerationmanifest.h"
#include "hybridpreflight.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridhydrology.h"
#include "hybridmacroterrain.h"
#include "hybridanchors.h"
#include "hybridtopology.h"
#include "hybridroutes.h"
#include "hybridrealization.h"
#include "hybridinteractions.h"
#include <cstdio>
struct HybridRoadRealizerInputs;


/*
**	D-033: one generation attempt as Generate_World ran it -- the effective seed, how far it got, what stopped it and
**	the identity it carried -- for the production census and the identity line a developer launch prints (a Release
**	build compiles WWDEBUG_SAY out).  Observational only: the generator never reads it back.
*/
enum ProceduralAttemptStage
{
	PROCEDURAL_STAGE_PREFLIGHT = 0,		// the request's content preflight, before any seeded work
	PROCEDURAL_STAGE_BOUNDS,
	PROCEDURAL_STAGE_PHASE2_IDENTITY,
	PROCEDURAL_STAGE_PHASE3_GRAPH,
	PROCEDURAL_STAGE_PHASE4_COARSE,
	PROCEDURAL_STAGE_PHASE5_LANDFORMS,
	PROCEDURAL_STAGE_PHASE6_HYDROLOGY,
	PROCEDURAL_STAGE_CLIFF_RING,
	PROCEDURAL_STAGE_MAP_PLAN,
	PROCEDURAL_STAGE_PHASE7_TERRAIN,
	PROCEDURAL_STAGE_PHASE8_ANCHORS,
	PROCEDURAL_STAGE_PHASE9_TOPOLOGY,
	PROCEDURAL_STAGE_PHASE10_ROUTES,
	PROCEDURAL_STAGE_PHASE11_INTERACTIONS,
	PROCEDURAL_STAGE_WORLD_WATER,			// interior cliffs and the raster hydrology Phase 12 grades over
	PROCEDURAL_STAGE_PHASE12_REALIZATION,
	PROCEDURAL_STAGE_AFTER_PHASE12,		// Phase 12 passed; the world's later stages are running
	PROCEDURAL_STAGE_COMPLETE,
	PROCEDURAL_STAGE_COUNT
};

const char *	Procedural_Attempt_Stage_Name(int stage);

struct ProceduralWorldAttemptRecord
{
	ProceduralWorldAttemptRecord(void) :
		Attempt(0),Seed(0),Stage(PROCEDURAL_STAGE_PREFLIGHT),Completed(false),ConnectivityOk(false),HasIdentity(false),
		Archetype(0),BiomeMode(0),PrimaryBiome(0),SecondaryBiome(0),IdentityChecksum(0),TopologyChecksum(0),RoutesChecksum(0),
		InteractionsChecksum(0),RealizationChecksum(0) { }
	bool	operator == (const ProceduralWorldAttemptRecord & that) const	{ return (Attempt == that.Attempt) && (Seed == that.Seed); }
	bool	operator != (const ProceduralWorldAttemptRecord & that) const	{ return !(*this == that); }

	int				Attempt;					// 0 is the requested seed itself
	unsigned int	Seed;						// the effective seed this attempt generated
	int				Stage;					// ProceduralAttemptStage reached (the one that stopped it, unless COMPLETE)
	bool				Completed;				// Generate_World_Attempt returned true
	bool				ConnectivityOk;		// ...and the required base routes held
	StringClass		Error;
	bool				HasIdentity;			// Phase 2 ran: the fields below are the world's
	int				Archetype;				// HybridArchetype
	int				BiomeMode;				// HybridBiomeMode
	int				PrimaryBiome;			// HybridBiome
	int				SecondaryBiome;
	unsigned int	IdentityChecksum;		// each phase's own gate checksum, 0 where the phase did not finish
	unsigned int	TopologyChecksum;
	unsigned int	RoutesChecksum;
	unsigned int	InteractionsChecksum;
	unsigned int	RealizationChecksum;
	StringClass		Reproduction;			// the manifest's reproduction string for this attempt
};


class ProceduralWorldManager
{
	public:

		//	The profile shipped with the game, used when a request names no other.
		static const char * const DEFAULT_PROFILE_PATH;

		//	A native world is generated whole -- terrain, bases, spawns, navigation -- with no
		//	level file underneath.  NATIVE_MAP_NAME is the map name such a game carries (no file
		//	by that name exists); the level loader skips the file load for it.
		static const char * const NATIVE_PROFILE_PATH;
		static const char * const NATIVE_MAP_NAME;
		static bool		Is_Native_Map(const char * map_name);

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Ask for the next level load to build a procedural world over itself.
		**
		**	`seed` of 0 means "choose one", which is what the menu button does -- the chosen
		**	value is normalized, recorded in the manifest and logged, so a world a player liked
		**	can be asked for again by number rather than being unrepeatable.
		**
		**	`profile_path` of nullptr means DEFAULT_PROFILE_PATH.
		**
		**	Returns false and requests nothing if the profile will not load or will not
		**	validate.  Failing here, before a level starts loading, is deliberate: the
		**	alternative is a player watching a level load and then finding an ordinary map,
		**	with the reason only in a log.
		*/
		static bool		Request_World(unsigned int seed,const char * profile_path,StringClass * error_out);

		/*
		**	A client's request for the world its server already built: that exact seed, one
		**	attempt, and the result accepted whatever its connectivity -- the server has already
		**	decided which attempt stands, and a retry here would pick a different seed and build a
		**	different world.  Cleared by Clear_Request like any other request.
		*/
		static bool		Request_World_Exact(unsigned int seed,const char * profile_path,StringClass * error_out);

		//	The profile the current request (and so the active world) was built from.
		static unsigned int	Get_Requested_Seed(void)	{ return RequestedSeed; }
	static const char *	Get_Profile_Path(void)	{ return RequestedProfilePath.Peek_Buffer (); }

		//	Drop any outstanding request.  The next level load is an ordinary one.
		static void		Clear_Request(void);

		static bool		Is_Requested(void)			{ return Requested; }
		//	True when the request's profile generates its own terrain (fit_to_level false), so
		//	the game should start on NATIVE_MAP_NAME rather than a host level.
		static bool		Is_Requested_Native(void)	{ return Requested && RequestedNative; }
		static bool		Is_World_Active(void)		{ return WorldActive; }

		/*
		**	Run the generation for the level that has just finished loading.  Called from
		**	CombatManager::Post_Load_Level and from nowhere else; returns immediately with no
		**	request outstanding, which is every hand-authored map.
		**
		**	It is deliberately called after the level is complete rather than before it starts,
		**	because a world fitted to its host level cannot be sized until that level's own
		**	extents exist, and because the cliff ring needs a physics scene to put itself in.
		*/
		static bool		Generate_World(StringClass * error_out);

		/*
		**	Why the last Generate_World refused, kept after the call returns.
		**
		**	A native world has no level file under it (D-002), so a refused generation leaves the
		**	player standing in a skybox with nothing in it, and the reason only in a log.  The
		**	level loader cannot ask Generate_World for that reason -- it is called from deep
		**	inside CombatManager::Post_Load_Level -- so the refusal is recorded here for whoever
		**	owns the decision to abandon the load.
		**
		**	Cleared by Init, by a fresh request, and by a generation that succeeds.
		*/
		static bool					Did_Generation_Fail(void)		{ return GenerationFailed; }
		static const char *		Get_Generation_Error(void)		{ return LastGenerationError.Peek_Buffer (); }

		//	Called by whoever has shown the reason to the player, so it is reported once.
		static void					Clear_Generation_Failure(void)
		{
			GenerationFailed		= false;
			LastGenerationError	= "";
		}

		//	Tear the generated world down.  Called from the level unload path.
		static void		Destroy_World(void);

		//	D-033: a developer sink for one line per stage an attempt enters (with the clock), or null for none.
		static void		Set_Progress_Log(FILE * log)	{ ProgressLog = log; }

		//	D-033: the attempts the last Generate_World ran, in order (see ProceduralWorldAttemptRecord).
		static int													Get_Attempt_Count(void)		{ return AttemptLog.Count (); }
		static const ProceduralWorldAttemptRecord &		Get_Attempt(int index)		{ return AttemptLog[index]; }

		/*
		**	What was generated, for the console, the manifest file and the self check.  The
		**	profile returned is the one actually used, including any fit-to-level override, not
		**	the one as authored -- otherwise a manifest would fail to reproduce its own world.
		*/
		static const ProceduralWorldProfile &		Get_Active_Profile(void)	{ return ActiveProfile; }
		static const WorldGenerationManifest &		Get_Manifest(void)			{ return Manifest; }
		/*
		** The hybrid generator's Section 0.8 metric cache from the current request's preflight;
		** Has_Hybrid_Metrics is false for the legacy generator or before preflight passed.
		*/
		static bool											Has_Hybrid_Metrics(void)	{ return HybridMetricsValid; }
		static const HybridMetricCache &				Get_Hybrid_Metrics(void)	{ return HybridMetrics; }
		static const HybridPreflightInputs &		Get_Hybrid_Inputs(void)		{ return HybridInputs; }

		/*
		**	Phase 12 on the active native hybrid world, for the console: false when it has none; otherwise Section 12.3 run
		**	again over the runtime objects as they stand now, its verdict in `report_out`.
		*/
		static bool		Report_Hybrid_Roads(StringClass * report_out);

		/*
		** Runtime Phase 2's immutable MapIdentity and Section-6.10 intent set for the attempt
		** currently being generated (rewrite item I3).  Has_Hybrid_Identity is false for the legacy
		** generator and before Phase 2 has run for this attempt; every later hybrid phase reads
		** these records rather than re-deriving anything from the archetype name.
		*/
		static bool											Has_Hybrid_Identity(void)	{ return HybridIdentityValid; }
		static const HybridMapIdentity &				Get_Hybrid_Identity(void)	{ return HybridIdentity; }
		static const HybridArchetypeIntentSet &	Get_Hybrid_Intents(void)	{ return HybridIntents; }

		/*
		** Runtime Phase 3's strategic gameplay graph for the attempt currently being generated
		** (rewrite item I4): both bases, every Tiberium role, the front-center analysis node, the
		** heavy route-family waypoints and their route/resource/crossing intents.  False before
		** Phase 3 has run for this attempt, and for the legacy generator, which has no Phase 3.
		*/
		static bool											Has_Hybrid_Strategic_Graph(void)	{ return HybridGraphValid; }
		static const HybridStrategicGraph &			Get_Hybrid_Strategic_Graph(void)	{ return HybridGraph; }

		/*
		** Runtime Phase 4's coarse semantic matrices for the attempt currently being generated
		** (rewrite item I5): the Section-2.2 layers at spacing `S` plus the reservation set later
		** phases query and extend.  False before Phase 4 has run for this attempt, and for the
		** legacy generator.  The layers live in the attempt scratch arena, so they are valid only
		** until the next attempt resets it.
		*/
		static bool											Has_Hybrid_Coarse_Matrices(void)	{ return HybridCoarseValid; }
		static const HybridCoarseMatrices &			Get_Hybrid_Coarse_Matrices(void)	{ return HybridCoarse; }

		/*
		** Runtime Phase 5's vector landforms for the attempt currently being generated (rewrite item
		** I6): the hills, ridges, explicit cliff runs and canyon/ravine systems, each with its own
		** compact-support field.  False before Phase 5 has run for this attempt, and for the legacy
		** generator.  Phase 6's provisional drainage surface and Phase 7's `landform` height term are
		** both `Hybrid_Landform_Height` over this set, so no later phase re-derives the profiles.
		*/
		static bool											Has_Hybrid_Landforms(void)			{ return HybridLandformsValid; }
		static const HybridLandformSet &				Get_Hybrid_Landforms(void)			{ return HybridLandforms; }

		/*
		** Runtime Phase 6's vector hydrology for the attempt currently being generated (rewrite item
		** I7): the selected river systems with their per-sample widths and drainage surfaces, the lake
		** and coast polygons, the decorative islands, and every crossing/ramp-pass window the road
		** phases are allowed to use.  False before Phase 6 has run for this attempt, and for the
		** legacy generator.  Phase 7's `water_carve` term is `Hybrid_Water_Carve_Depth` over this set
		** and its guided-river surface samples are the `Surfaces` array, so no later phase re-derives
		** a carve profile or a drainage height.
		*/
		static bool											Has_Hybrid_Hydrology(void)			{ return HybridHydrologyValid; }
		static const HybridHydrologySet &			Get_Hybrid_Hydrology(void)			{ return HybridHydrology; }

		/*
		** Runtime Phase 7's realization record for the attempt currently being generated (rewrite item
		** I8): the frozen river water surfaces and bed heights from the plan's longitudinal walk, plus
		** the realized heightfield extents.  The heightfield itself stays `WorldTerrainSystem`'s and
		** the write stays `MacroTerrainGenerator`'s; this is the record every later phase reads when it
		** needs a river's water level rather than sampling the ground for one.
		*/
		static bool											Has_Hybrid_Macro_Terrain(void)	{ return HybridMacroValid; }
		static const HybridMacroTerrainSet &		Get_Hybrid_Macro_Terrain(void)	{ return HybridMacro; }

		/*
		** Runtime Phase 8's frozen anchors for the attempt currently being generated (rewrite item I9):
		** each base's centre, its `BASE_PROTECTED` radius and its sorted viable heavy exit sectors, every
		** Tiberium anchor with its protected reserve, and the Section-0.10 provisional fairness numbers.
		** Phase 8 moves nothing -- these are the Phase-3 centres, validated against realized ground --
		** and later phases read this record as the authority rather than re-deriving a base footprint.
		*/
		static bool											Has_Hybrid_Anchors(void)			{ return HybridAnchorsValid; }
		static const HybridAnchorSet &				Get_Hybrid_Anchors(void)			{ return HybridAnchors; }

		/*
		** Runtime Phase 9's transportation topology for the attempt currently being generated (rewrite
		** item I10): the base entrance points, the heavy route families with their bound crossing
		** windows, the home and field roads, the quota-filled service and trail edges, the optional
		** cross-links and the shared overpass obligations.  This phase decides which roads exist and
		** which frozen points they pass through; Phase 10 routes them and may not invent a crossing this
		** record does not carry.  It replaces the native MST topology on the hybrid path.
		*/
		static bool											Has_Hybrid_Topology(void)			{ return HybridTopologyValid; }
		static const HybridTopology &					Get_Hybrid_Topology(void)			{ return HybridTopologyRecord; }

		/*
		** Runtime Phase 10's routed splines for the attempt currently being generated (rewrite item I11):
		** one accepted spline per topology edge, the A* cell chain behind it, and the frozen waypoints it
		** was routed through.  Nothing is realized on the ground yet -- the crossing intervals are still
		** unresolved records for Phase 11 to compile.
		*/
		static bool											Has_Hybrid_Routes(void)				{ return HybridRoutesValid; }
		static const HybridRouteSet &					Get_Hybrid_Routes(void)				{ return HybridRoutes; }
		static const HybridInteractionSet &			Get_Hybrid_Interactions(void)		{ return HybridInteractions; }

		//	Runtime Phase 12's frozen plan, once it realized the attempt's roads and decks (I13).
		static bool											Has_Hybrid_Realization(void)		{ return HybridRealizationValid; }
		static const HybridRealizationPlan &			Get_Hybrid_Realization(void)		{ return HybridRealization; }

	private:

		//	The whole of Generate_World's work.  The public entry is a thin wrapper that records
		//	the refusal for Did_Generation_Fail/Get_Generation_Error, so there is exactly one
		//	implementation of the attempt policy and one place that remembers how it ended.
		static bool		Generate_World_Internal(StringClass * error_out);

		static bool		GenerationFailed;
		static StringClass	LastGenerationError;

		static bool		Fit_Profile_To_Level(ProceduralWorldProfile * profile);
		static bool		Build_Cliff_Ring(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Rewrite item I3.  Runtime Phase 2: resolves this attempt's MapIdentity and Section-6.10
		//	intent set from the validated profile and the frozen preflight metric cache.  `built_out`
		//	reports whether the records now exist; false with a true return means the legacy
		//	generator, which has no Phase 2 at all.
		static bool		Build_Hybrid_Map_Identity(const ProceduralWorldProfile & profile,bool * built_out,
															  StringClass * error_out);

		//	Rewrite item I4.  Runtime Phase 3: places every required gameplay role and records the
		//	route/resource/crossing intents from the frozen Phase-2 records.  `built_out` reports
		//	whether the graph now exists; false with a true return means the legacy generator.
		static bool		Build_Hybrid_Strategic_Graph(const ProceduralWorldProfile & profile,bool * built_out,
																	StringClass * error_out);

		//	Rewrite item I5.  Runtime Phase 4: the coarse semantic matrices and the reservation set,
		//	built from the frozen Phase-2 records and the Phase-3 roles.  `built_out` reports whether
		//	the matrices now exist; false with a true return means the legacy generator.
		static bool		Build_Hybrid_Coarse_Matrices(const ProceduralWorldProfile & profile,bool * built_out,
																	StringClass * error_out);

		//	Rewrite item I6.  Runtime Phase 5: the vector landforms, built from the frozen Phase-2
		//	records, the Phase-3 roles and the Phase-4 matrices, which this phase also completes by
		//	writing the LandformMask and RoughnessClass layers.  `built_out` reports whether the
		//	landform set now exists; false with a true return means the legacy generator.
		static bool		Build_Hybrid_Landforms(const ProceduralWorldProfile & profile,bool * built_out,
															StringClass * error_out);

		//	Rewrite item I7.  Runtime Phase 6: the vector hydrology, built from the frozen Phase-2
		//	records, the Phase-3 roles, the Phase-4 matrices and the Phase-5 landforms.  The matrices
		//	are handed over mutable because this phase owns the accepted water intent and the crossing
		//	window reservations Phase 4 deliberately left for it.  `built_out` reports whether the
		//	hydrology set now exists; false with a true return means the legacy generator.
		static bool		Build_Hybrid_Hydrology(const ProceduralWorldProfile & profile,bool * built_out,
															StringClass * error_out);

		//	Rewrite item I9.  Runtime Phase 8: validates the frozen Phase-3 base and Tiberium centres
		//	against the realized Phase-7 ground and freezes each base's protected disc and viable exit
		//	sectors.  Nothing is moved here; a hard requirement the realized world fails is an attempt
		//	failure.  `built_out` reports whether the anchors are frozen; false with a true return means
		//	the legacy generator, or a hybrid attempt whose Phase 7 did not run.
		static bool		Build_Hybrid_Anchors(const ProceduralWorldProfile & profile,bool * built_out,
														StringClass * error_out);

		//	Rewrite item I10.  Runtime Phase 9: the transportation topology, from the Phase-3 intents, the
		//	Phase-6 windows and the Phase-8 frozen centres and exit sectors.  `built_out` reports whether
		//	the topology now exists; false with a true return means the legacy generator, or a hybrid
		//	attempt whose Phase 8 did not run.
		static bool		Build_Hybrid_Topology(const ProceduralWorldProfile & profile,bool * built_out,
														 StringClass * error_out);

		//	Rewrite item I11.  Runtime Phase 10: the bounded heading-aware A* and its splines, over the
		//	Phase-9 topology and the realized Phase-7 ground.  `built_out` reports whether the routes now
		//	exist; false with a true return means the legacy generator, or a hybrid attempt whose Phase 9
		//	did not run.
		/*
		**	Rewrite item I12, runtime Phase 11: what each crossing Phase 10 left unresolved actually *is*.
		**	Nothing is instantiated here -- the output is one compiled solution record per conflict, which
		**	Phase 12 consumes as the whole of what it may know about a crossing.
		*/
		static bool		Build_Hybrid_Interactions(const ProceduralWorldProfile & profile,bool * built_out,
																StringClass * error_out);

		/*
		**	Rewrite item I13, runtime Phase 12: the frozen roads, bridge and overpass decks, junction surfaces and road
		**	cut/fill realized once through WorldTerrainSystem, RoadSystem and BridgeSystem (HybridRoadRealizer),
		**	verified against the plan, and published through the migration adapters the downstream systems still
		**	read.  On the native hybrid path it replaces the legacy strategic plan, road graph, bridge and tunnel
		**	plans and base selection; the native generator builds no tunnels (D-032).
		*/
		static bool		Realize_Hybrid_Roads(const ProceduralWorldProfile & profile,StringClass * error_out);
		//	Section 12.3 over the runtime objects: the record half and the physical half (swept envelopes, bridges, ramps).
		static bool		Verify_Hybrid_Roads(const ProceduralWorldProfile & profile,StringClass * error_out);
		static void		Hybrid_Realizer_Inputs(const ProceduralWorldProfile & profile,HybridRoadRealizerInputs * inputs);

		static bool		Build_Hybrid_Routes(const ProceduralWorldProfile & profile,bool * built_out,
													  StringClass * error_out);

		//	D-003 Phase 3B.  The 2D map plan built before any terrain.  `built_out` reports
		//	whether MapPlanGenerator now holds a plan; false with a true return means "correctly
		//	skipped" (plan disabled in the profile, or a `fit_to_level` world).
		static bool		Build_Map_Plan(const ProceduralWorldProfile & profile,bool * built_out,StringClass * error_out);

		//	Roadmap Section 34 Phase 4.  `built_out` is set to whether a chunk registry now exists
		//	for Phases 5-9 to read -- false and a true return together mean "correctly skipped",
		//	which only ever happens when neither generation nor fitting produced one.  A
		//	`fit_to_level` world never gets a second collidable ground (see
		//	macroterraingenerator.h's own class comment and Build_Fitted_Terrain's below), but it
		//	does get a real chunk registry now, sampled from the level itself.
		static bool		Build_Macro_Terrain(const ProceduralWorldProfile & profile,bool * built_out,StringClass * error_out);

		//	The fitted-world half of Phase 4.  Populates WorldTerrainSystem's heightfield and
		//	chunk registry by sampling the host level's own real geometry, never by generating one,
		//	and never calls Build_Collision/Build_Far_Terrain_Representation -- so the level's own
		//	ground stays the only collidable ground, and Phases 5-9 stop being permanently gated
		//	off for the one profile ("Procedural Skirmish") that ever actually requests a world.
		static bool		Build_Fitted_Terrain(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 5.  Only meaningful once macro terrain stands, so this is a
		//	no-op (not a failure) whenever Build_Macro_Terrain itself was -- a `fit_to_level`
		//	world has no chunk registry to read candidates from, same reasoning as Phase 4's own.
		static bool		Build_Interior_Cliffs(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 6.  Reads the chunk registry the same way interior cliffs
		//	does, so it shares the same "only when terrain_built" gate.
		static bool		Build_Hydrology(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 7.  Reads the same chunk registry, plus whatever interior
		//	cliff/hydrology candidates already exist (optional -- it degrades to zero of each
		//	rather than failing when either did not build).
		static bool		Build_Strategic_Plan(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 8.  Requires the strategic plan the previous step builds, so
		//	it shares that gate too, not just terrain_built.
		static bool		Build_Road_Graph(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 9.  Requires the road graph -- every candidate comes from an
		//	edge's own unresolved-crossing list -- so it shares that gate.
		static bool		Build_Bridge_Plan(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 10.  After the bridge plan: a road crossing no bridge could
		//	resolve is a tunnel candidate, and so is every cliff bypass no road passes through.
		static bool		Build_Tunnel_Plan(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 11.  Requires the road graph and the bridge plan -- Phase
		//	11's own purpose line: choose bases only once roads, bridges and terrain constraints
		//	are known.  Runs whether or not the bridge plan itself resolved every crossing, since
		//	an unresolved crossing is already visible on the road-graph tree this reads.
		static bool		Build_Base_Selection(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 12.  Requires the base selection -- step 1's own "initial
		//	per-team field" needs a selected base to be initial for -- so it shares that gate.
		static bool		Build_Tiberium_Fields(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 13.  Requires the road graph (for road-connectivity
		//	classification) and the base/Tiberium reservations, so it shares the road-graph gate
		//	but reads Phase 11/12's own state directly rather than needing a further boolean.
		static bool		Build_Settlements(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 14.  Needs only the chunk registry, so it shares
		//	terrain_built's own gate; it reads Phase 11/12/13's own reserved regions when they
		//	exist and simply excludes nothing extra when they do not.
		static bool		Build_Biome_Decoration(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 15.  A validator, the same shape as
		//	Validate_Transportation_Network below; runs after everything else and never blocks
		//	generation on its own -- its own hard-failure verdict folds into
		//	*required_connectivity_ok_out below instead, the same retry trigger item 6's own
		//	required-base-routes flag already is.
		static bool		Validate_Combat_Space(const ProceduralWorldProfile & profile);

		//	Roadmap Section 34 Phase 16 steps 1-2.  Real collision/render/culling registration for
		//	Phase 13's own settlement structures, the one generated-object type that reached this
		//	point still data-only.  Requires settlements/the asset registry; a world with no
		//	settlements built is still a real, correct 0-record pass.
		static bool		Build_World_Integration(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 16 steps 3-8.  Real infantry/ground-vehicle sector navigation
		//	plus a real flight volume, registered into the one canonical PathfindClass singleton
		//	action.cpp's own AI movement code already consults.  Requires the chunk registry, so it
		//	shares terrain_built's own gate.
		static bool		Build_Navigation(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 16 step 12.  A real, currently-unconsumed minimap/tactical-map
		//	base layer built from the same chunk registry and the same resolved generator state
		//	every earlier phase already produced.
		static bool		Build_Minimap_Layer(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 16 step 13.  WorldShroudManager is already fully world-
		//	coordinate based, but its per-team grid has to be told where the world is, and the one
		//	existing caller of WorldShroudManager::Re_Partition (PhysicsSceneClass::Re_Partition_
		//	Dynamic_Culling_System) is documented editor-only and also resets vis.  So a generated
		//	world sizes the shroud grid itself, from the chunk registry's own real patch bounds --
		//	the same bounds NavMeshGenerator and MinimapBaseLayer already read.  Shares
		//	terrain_built's own gate.
		static bool		Build_Shroud_Coverage(const ProceduralWorldProfile & profile,StringClass * error_out);

		/*
		**	Roadmap Section 34 Phase 17 steps 1-2, 5 and 10-13.  Not a generator: the Commander's
		**	expansion rules decide what may be done to this world from now on, and arming them is
		**	pushing the profile's grading limits into `FoundationGradingService`, deriving the
		**	chokepoint set from the navigation just built, and starting the expansion registry
		**	empty.
		**
		**	Runs unconditionally rather than under terrain_built, because the rules must be armed
		**	for a world whose terrain failed too -- a validator that silently keeps the previous
		**	world's limits is worse than one that reports having no chokepoint data.
		*/
		static bool		Arm_Expansion_Rules(const ProceduralWorldProfile & profile);

		//	The gap report's item 6.  Runs after every generator above has had its turn, whether
		//	or not each one individually succeeded -- a validator that only ran on a clean chain
		//	could never report the chain being broken.  Never a reason to refuse the world; see
		//	transportationnetworkvalidator.h's own comment for why.
		static bool		Validate_Transportation_Network(const ProceduralWorldProfile & profile);

		//	One full pass of everything above, over one already-fitted, already-validated
		//	profile.  Returns false only for the one failure this pipeline has always treated as
		//	fatal (the boundary itself refusing to configure); every generator failure short of
		//	that is logged and the pass still completes, exactly as it always has.
		//	`*required_connectivity_ok_out` is gap report item 5's own retry trigger: whether
		//	TransportationNetworkValidator's own required-base-routes check, if it applied at
		//	all, actually held for this attempt.
		static bool		Generate_World_Attempt(const ProceduralWorldProfile & profile,StringClass * error_out,
															 bool * required_connectivity_ok_out);

		//	Gap report item 5's own "bounded deterministic retry policy" at the level that
		//	actually owns a whole world: how many times Generate_World tries a fresh,
		//	deterministically different seed before accepting whatever the last attempt produced
		//	rather than looping forever chasing a fully-connected layout that may not exist for
		//	this profile at all.
		static const int	MAX_WORLD_GENERATION_ATTEMPTS = 3;

		static int											AttemptStage;
		static FILE *										ProgressLog;
		static void		Enter_Stage(int stage);
		static DynamicVectorClass<ProceduralWorldAttemptRecord>	AttemptLog;
		static void		Record_Attempt(const ProceduralWorldProfile & profile,int attempt,bool completed,bool connectivity_ok,
										  const StringClass & error);

		static bool		Requested;
		static bool		RequestedExact;
		static bool		RequestedNative;
		static bool		WorldActive;

		static unsigned int		RequestedSeed;
		static StringClass		RequestedProfilePath;

		static ProceduralWorldProfile		ActiveProfile;
		static WorldGenerationManifest	Manifest;
		static HybridMetricCache			HybridMetrics;
		static bool							HybridMetricsValid;
		static HybridMapIdentity			HybridIdentity;
		static HybridArchetypeIntentSet	HybridIntents;
		static bool							HybridIdentityValid;
		static HybridStrategicGraph		HybridGraph;
		static bool							HybridGraphValid;
		static HybridCoarseMatrices		HybridCoarse;
		static bool							HybridCoarseValid;
		static HybridLandformSet			HybridLandforms;
		static bool							HybridLandformsValid;
		static HybridHydrologySet			HybridHydrology;
		static bool							HybridHydrologyValid;
		static HybridMacroTerrainSet		HybridMacro;
		static bool							HybridMacroValid;
		static HybridAnchorSet				HybridAnchors;
		static bool							HybridAnchorsValid;
		static HybridTopology				HybridTopologyRecord;
		static bool							HybridTopologyValid;
		static HybridRouteSet				HybridRoutes;

	/*
	**	The Phase-10 parameters the accepted routes were built with, kept because Phase 11 may legitimately
	**	ask for one more corridor envelope and the reroute has to re-run Phase 10 with the same inputs.
	*/
	static HybridRouteParams			HybridRouteInputs;
		static HybridInteractionSet		HybridInteractions;
		static bool								HybridInteractionsValid;
		static HybridRealizationPlan		HybridRealization;
		static bool								HybridRealizationValid;

		/*
		**	The preflight inputs the metric cache indexes into.  Phase 11 needs the per-kind bridge
		**	metrics themselves, not only the cache's indices into them, and re-measuring content
		**	mid-generation is exactly what preflight exists to avoid.
		**
		**	Written once, beside `HybridMetrics` and under the same `HybridMetricsValid` latch, from the
		**	inputs the passing preflight was run on; read-only for the rest of the attempt and every retry,
		**	because nothing preflight measures changes with the effective seed.  It is a second *view* of
		**	frozen content measurements, never a second source of truth: `Get_Hybrid_Inputs` is the only way
		**	anything outside that one assignment reaches it.
		*/
		static HybridPreflightInputs		HybridInputs;
		static bool							HybridRoutesValid;

		//	Section 16.3's one per-attempt scratch arena and Section 16.1's work counters.  Reserved
		//	at the start of a hybrid attempt and reused by every phase that needs scratch.
		static HybridScratchArena		HybridArena;
		static HybridWorkCounters		HybridWork;
};

#endif	// PROCEDURALWORLDMANAGER_H
