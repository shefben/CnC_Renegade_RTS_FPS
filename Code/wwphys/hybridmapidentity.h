/*
**	OpenW3D
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
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/hybridmapidentity.h                   $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I3 -- runtime Phase 2, MapIdentity                      *
 * (Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md Section 8 Phase 2, Sections 4, 5,  *
 * 6.1 and 6.10).                                                                              *
 *                                                                                             *
 * Phase 2 is the first runtime phase: it decides what kind of map this seed is, and nothing    *
 * else.  In the plan's exact order -- size class, symmetry, archetype, strategic axis, biome   *
 * mode, primary biome, secondary biome, island slot bits, ArchetypeIntentSet -- it resolves     *
 * one immutable record pair:                                                                   *
 *                                                                                             *
 *   HybridMapIdentity         what this map is, plus the strategic (u,v) frame every later     *
 *                            phase measures its guides in (Section 0.4)                        *
 *   HybridArchetypeIntentSet  the Section-6.10 intent records later phases consume instead of  *
 *                            re-reading the archetype name, including the frozen guided-river  *
 *                            centerline and the mixed-biome divider parameters                 *
 *                                                                                             *
 * No terrain is touched here and no geometry is realized.  Every draw is a keyed Hash01 from    *
 * the existing map_plan / hydrology / biomes seed domains; there is no stateful RNG.  The       *
 * resolver is pure over HybridMapIdentityParams, which ProceduralWorldManager fills from the    *
 * profile and the frozen preflight metric cache -- the same split MapPlanGenerator already      *
 * uses, so the self-checks drive Phase 2 with hand-filled params and no runtime content.        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef HYBRIDMAPIDENTITY_H
#define HYBRIDMAPIDENTITY_H

#include "always.h"
#include "vector.h"
#include "vector2.h"
#include "wwstring.h"
#include "hybridplanrecords.h"
#include "hybridplanning.h"


/*
**	Section 6.1 -- the profile's symmetry as an enum.  The profile stores the plan's own spelling;
**	Hybrid_Symmetry_From_Name accepts exactly `none`, `rotational` and `mirror_x`.
*/
enum HybridSymmetry
{
	HYBRID_SYMMETRY_NONE = 0,
	HYBRID_SYMMETRY_ROTATIONAL,
	HYBRID_SYMMETRY_MIRROR_X,
	HYBRID_SYMMETRY_COUNT
};

const char *	Hybrid_Symmetry_Name(HybridSymmetry symmetry);
bool				Hybrid_Symmetry_From_Name(const char * name,HybridSymmetry * out);


/*
**	Section 0.4 -- the strategic frame.  `u` runs along the seeded GDI-to-Nod axis with `u=-1` the
**	GDI side and `u=+1` the Nod side; `v` is that axis rotated +90 degrees; `(0,0)` is the playable
**	centre.  One unit of u or v is `Scale = D/2` world units, so a rotated axis on a non-square
**	world still keeps the whole |u|,|v| <= 1 square inside the configured extents.
*/
struct HybridStrategicFrame
{
	HybridStrategicFrame(void) : Center(0.0f,0.0f),Scale(1.0f),AxisAngle(0.0f),AxisU(1.0f,0.0f),AxisV(0.0f,1.0f) { }

	Vector2	Center;				// world XY of strategic (0,0)
	float		Scale;				// world units per unit of u/v -- D/2
	float		AxisAngle;			// radians, quantized; the world-XY angle of +u
	Vector2	AxisU;				// unit vector along +u
	Vector2	AxisV;				// unit vector along +v, AxisU rotated +90 degrees

	Vector2	To_World(float u,float v) const
	{
		return Vector2 (Center.X + (AxisU.X * u + AxisV.X * v) * Scale,
							 Center.Y + (AxisU.Y * u + AxisV.Y * v) * Scale);
	}
	Vector2	To_World(const Vector2 & uv) const		{ return To_World (uv.X,uv.Y); }

	Vector2	To_Strategic(const Vector2 & world) const
	{
		const float dx = world.X - Center.X;
		const float dy = world.Y - Center.Y;
		return Vector2 (((dx * AxisU.X) + (dy * AxisU.Y)) / Scale,
							 ((dx * AxisV.X) + (dy * AxisV.Y)) / Scale);
	}
};

void	Hybrid_Build_Strategic_Frame(float center_x,float center_y,float d,float axis_angle,HybridStrategicFrame * out);

/*
**	Section 6.1 -- the paired-role map.  `rotational` sends (u,v) to (-u,-v); `mirror_x` sends it to
**	(-u,+v); `none` has no pairing and returns the input unchanged, so a caller that mirrors
**	unconditionally still gets a legal answer instead of a mirrored duplicate.
*/
Vector2	Hybrid_Mirror_Strategic(HybridSymmetry symmetry,const Vector2 & uv);


/*
**	Section 6.1's bounded legality-repair search, shared by every phase that may move a plan-stage
**	position: Phase 3's roles, Phase 4's lower-priority reservations and Section 6.9's
**	`OPEN_MANEUVER` discs.  Candidates are tested in concentric rings at radii `ring_step`,
**	`2*ring_step`, ... up to `max_offset` (Section 6.1's `A` and `0.10D`), with 16 angular stations
**	per ring beginning at strategic `+u` and rotating counter-clockwise; the first accepted candidate
**	wins.  The origin itself is *not* tested here -- the caller has already tested it, and the rule
**	that applies to a guide position is not always the rule that applies once repair has moved off
**	it -- and every candidate is quantized before the test so the accepted position is the stored
**	position.
*/
typedef bool (*HybridPlacementTestFunc)(const Vector2 & candidate,void * context);

#define HYBRID_REPAIR_STATIONS_PER_RING		16

bool	Hybrid_Repair_Ring_Search(const HybridStrategicFrame & frame,float ring_step,float max_offset,
											const Vector2 & origin_world,HybridPlacementTestFunc test,void * context,
											Vector2 * out_world);


/*
**	Section 6.10 -- the immutable intent kinds.  One record per intent the table names; later
**	phases match on Kind and never on the archetype name.
*/
enum HybridIntentKind
{
	HYBRID_INTENT_GUIDED_RIVER = 0,				// river_valley principal river, frozen centerline
	HYBRID_INTENT_COASTAL_GUIDED_RIVER,			// coastal_peninsula river terminating in the sea
	HYBRID_INTENT_LAKE_GUIDE,						// twin_lakes required lake centre + target radius
	HYBRID_INTENT_PRINCIPAL_CANYON,				// canyon_crossroads principal canyon
	HYBRID_INTENT_PRINCIPAL_RIDGE,				// highland_pass principal ridge
	HYBRID_INTENT_SURFACE_SADDLE,					// highland_pass mandatory surface pass
	HYBRID_INTENT_COAST_GUIDE,						// coastal_peninsula sea side + sea boundary |v|
	HYBRID_INTENT_PENINSULA_NECK,					// coastal_peninsula neck with its minimum dry width
	HYBRID_INTENT_BASIN_GUIDE,						// plateau_basin basin centre + target radius
	HYBRID_INTENT_BASIN_ENTRY,						// plateau_basin independent basin entry point
	HYBRID_INTENT_MAIN_RAVINE_GUIDE,				// ravine_network main ravine
	HYBRID_INTENT_OPEN_MANEUVER,					// open_frontier reservation disc
	HYBRID_INTENT_ROUTE_FAMILY_CROSSING,		// per-family crossing guide
	HYBRID_INTENT_KIND_COUNT
};

const char *	Hybrid_Intent_Kind_Name(HybridIntentKind kind);


/*
**	Section 6.10 intent tags.  A crossing guide carries exactly the obligation the table gives it;
**	nothing else may be inferred from its position.
*/
enum HybridIntentFlag
{
	HYBRID_INTENT_FLAG_BRIDGE_REQUIRED				= 1 << 0,
	HYBRID_INTENT_FLAG_DISTINCT_CROSSING_REQUIRED	= 1 << 1,
	HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V				= 1 << 2,	// centreline endpoints lie on the strategic v axis
};


/*
**	One Section-6.10 intent.  Guide/Endpoint coordinates are strategic (u,v) before world rotation,
**	exactly as the table states; Points is the frozen world-space centerline a guided river needs,
**	empty for every other kind.  Radius, MinWidth, Amplitude and Wavelength are world units.
*/
struct HybridArchetypeIntent
{
	HybridArchetypeIntent(void) :
		StableId(0),Kind(HYBRID_INTENT_ROUTE_FAMILY_CROSSING),Flags(0),RouteFamily(-1),
		Guide(0.0f,0.0f),GuideEnd(0.0f,0.0f),Radius(0.0f),MinWidth(0.0f),
		Amplitude(0.0f),Wavelength(0.0f),Phase(0.0f),VSign(0) { }

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.  StableId is the key.
	bool	operator == (const HybridArchetypeIntent & that) const	{ return StableId == that.StableId; }
	bool	operator != (const HybridArchetypeIntent & that) const	{ return !(*this == that); }

	unsigned int						StableId;		// creation order, 1-based; keys every draw this intent owns
	HybridIntentKind					Kind;
	unsigned int						Flags;			// HybridIntentFlag bits
	int									RouteFamily;	// crossing guides only, else -1

	Vector2								Guide;			// strategic (u,v): centre, or a centerline's start
	Vector2								GuideEnd;		// strategic (u,v): a centerline's end, else == Guide
	float									Radius;			// lake/basin target effective radius, OPEN_MANEUVER disc radius
	float									MinWidth;		// PENINSULA_NECK minimum dry width
	float									Amplitude;		// guided-river meander amplitude
	float									Wavelength;		// guided-river meander wavelength
	float									Phase;			// guided-river meander phase, radians
	int									VSign;			// COAST_GUIDE / COASTAL_GUIDED_RIVER sea side, else 0

	DynamicVectorClass<Vector2>	Points;			// frozen quantized world-space centerline
};


/*
**	Section 4.3 steps 4-9 -- the mixed-biome divider, recorded for Phase 4 to rasterize.  The curve
**	is one cubic Bezier from strategic v=-1 to v=+1; ControlU holds the four control-point `u`
**	values in order (start, first control, second control, end) at v = -1, -1/3, +1/3, +1.
**	TransitionWidth is Section 4.3's exact max(4S,8W).  Phase 4 owns the 35% ownership test, the
**	six 0.05 shifts toward zero and the straight `u=0` fallback; Phase 2 only freezes the start.
*/
struct HybridBiomeDivider
{
	HybridBiomeDivider(void) : Present(false),TransitionWidth(0.0f)
	{
		ControlU[0] = ControlU[1] = ControlU[2] = ControlU[3] = 0.0f;
		ControlV[0] = -1.0f;
		ControlV[1] = -1.0f / 3.0f;
		ControlV[2] = +1.0f / 3.0f;
		ControlV[3] = +1.0f;
	}

	bool		Present;					// false on a single-biome map
	float		ControlU[4];
	float		ControlV[4];
	float		TransitionWidth;		// world units, max(4S,8W)
};


/*
**	The Section-6.10 set.  Intents are in creation order, which is their StableId order.
*/
struct HybridArchetypeIntentSet
{
	HybridArchetypeIntentSet(void) : RiverAdjacentContested(false) { }

	void	Reset(void)
	{
		Intents.Delete_All ();
		RiverAdjacentContested = false;
		Divider = HybridBiomeDivider ();
	}

	int	Count_Of_Kind(HybridIntentKind kind) const;
	const HybridArchetypeIntent *	Find_First(HybridIntentKind kind) const;

	DynamicVectorClass<HybridArchetypeIntent>	Intents;

	//	Section 6.10 river_valley flag: Phase 3 places the first CONTESTED role from the frozen
	//	guided river instead of the ordinary contested-field guide table.
	bool													RiverAdjacentContested;

	HybridBiomeDivider								Divider;
};


/*
**	Runtime Phase 2's output.  Immutable once resolved: every later phase reads it and none of them
**	re-draws any of it.
*/
struct HybridMapIdentity
{
	HybridMapIdentity(void) :
		SizeClass(HYBRID_SIZE_SMALL),Symmetry(HYBRID_SYMMETRY_ROTATIONAL),
		Archetype(HYBRID_ARCHETYPE_RIVER_VALLEY),BiomeMode(HYBRID_BIOME_MODE_SINGLE),
		PrimaryBiome(HYBRID_BIOME_TEMPERATE),SecondaryBiome(HYBRID_BIOME_COUNT),
		IslandSlotCount(0),IslandSlotBits(0),FlankSign(1),RouteFamilyCount(2),
		GdiBaseGuide(-0.72f,0.0f),NodBaseGuide(0.72f,0.0f),
		GdiHomeFieldGuide(-0.48f,0.0f),NodHomeFieldGuide(0.48f,0.0f),
		FrontCenterGuide(0.0f,0.0f),Checksum(0) { FlankGuide[0].Set (0.0f,0.0f); FlankGuide[1].Set (0.0f,0.0f); }

	HybridSizeClass			SizeClass;			// Section 0.6, derived from D; never drawn
	HybridSymmetry				Symmetry;			// the profile's own value
	HybridArchetype			Archetype;			// resolved; never HYBRID_ARCHETYPE_AUTO
	HybridBiomeMode			BiomeMode;			// resolved SINGLE or MIXED; never AUTO
	HybridBiome					PrimaryBiome;
	HybridBiome					SecondaryBiome;	// HYBRID_BIOME_COUNT on a single-biome map

	int							IslandSlotCount;	// Section 5 slots for this size class: 0 / 1 / 2
	unsigned int				IslandSlotBits;	// bit i set when slot i's Bernoulli test accepted
	int							FlankSign;			// Section 6.1 `s`, -1 or +1
	int							RouteFamilyCount;	// Section 4 clamp(max(route_redundancy,1+flank_lanes),2,3)

	HybridStrategicFrame		Frame;

	//	Section 6.1 common anchors, in strategic (u,v).  Phase 3 consumes these; Phase 2 only fixes
	//	them, because the only draws they need -- the flank sign and an asymmetric Nod offset -- are
	//	map identity.
	Vector2						GdiBaseGuide;
	Vector2						NodBaseGuide;
	Vector2						GdiHomeFieldGuide;
	Vector2						NodHomeFieldGuide;
	Vector2						FrontCenterGuide;
	Vector2						FlankGuide[2];		// flank A then flank B

	unsigned int				Checksum;			// Phase 2 gate: identical seed/profile, identical value

	int	Island_Slot_Count_Accepted(void) const;
	bool	Island_Slot_Accepted(int slot) const	{ return (slot >= 0) && (slot < IslandSlotCount) && ((IslandSlotBits & (1u << slot)) != 0); }
};


/*
**	Phase 2's inputs.  Plain values only: the resolver never reads a profile, a registry or a
**	global, so it is reproducible from this struct alone.  ProceduralWorldManager fills it from the
**	validated profile and the frozen preflight metric cache.
*/
struct HybridMapIdentityParams
{
	HybridMapIdentityParams(void);

	//	Seed domains, already derived through WorldSeedService::Derive_Subsystem_Seed.
	unsigned int		MapPlanSeed;				// SEED_DOMAIN_MAP_PLAN -- identity and strategic choices
	unsigned int		HydrologySeed;				// SEED_DOMAIN_HYDROLOGY -- guided-river meander draws
	unsigned int		BiomeSeed;					// SEED_DOMAIN_BIOMES -- mixed-biome divider draws

	//	World frame and the Section 0.5 constants preflight already computed.
	float					WorldCenterX;
	float					WorldCenterY;
	HybridConstants	Constants;

	//	Active Section 4 plan fields.
	HybridSymmetry		Symmetry;
	HybridArchetype	ArchetypeSelector;		// HYBRID_ARCHETYPE_AUTO for `auto`
	HybridBiomeMode	BiomeModeSelector;		// HYBRID_BIOME_MODE_AUTO for `auto`
	HybridBiome			BiomeSelector;				// HYBRID_BIOME_AUTO for `auto`
	HybridBiome			SecondaryBiomeSelector;	// HYBRID_BIOME_AUTO for `auto`
	float					MixedBiomeProbability;
	float					IslandChance;
	int					RouteRedundancy;
	int					FlankLanes;

	//	Section 5 toggles Phase 2 depends on.
	bool					CoastlineEnabled;

	//	Section 4.2 availability, straight from the preflight metric cache.
	bool					BiomeAvailable[HYBRID_BIOME_COUNT];
};


/*
**	Phase 2 failures.  Every one is a profile/content contradiction that no seed can rescue, so the
**	manager fails the request rather than retrying a new seed.
*/
enum HybridMapIdentityFailure
{
	HYBRID_IDENTITY_OK = 0,
	HYBRID_IDENTITY_ARCHETYPE_REQUIRES_COASTLINE,	// archetype=coastal_peninsula with water.coastline=false
	HYBRID_IDENTITY_NO_AVAILABLE_BIOME,				// Section 4.2 selectable list empty
	HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR,			// Section 4.3 compatible-secondary list empty
	HYBRID_IDENTITY_ILLEGAL_BIOME_PAIR,				// explicit pair absent from the Section 4.3 table
	HYBRID_IDENTITY_FAILURE_COUNT
};

const char *	Hybrid_Map_Identity_Failure_Name(HybridMapIdentityFailure failure);

struct HybridMapIdentityResult
{
	HybridMapIdentityResult(void) : Failure(HYBRID_IDENTITY_OK) { }

	HybridMapIdentityFailure	Failure;
	StringClass						Detail;
};


/*
**	Runtime Phase 2.  Builds the identity and the intent set in the plan's exact order and computes
**	the gate checksum.  Returns false with result->Failure set on the first contradiction; nothing
**	is written to the outputs in that case beyond whatever the resolver had already resolved.
*/
bool	Hybrid_Resolve_Map_Identity(const HybridMapIdentityParams & params,HybridMapIdentity * identity,
											HybridArchetypeIntentSet * intents,HybridMapIdentityResult * result);

/*
**	The Phase 2 gate value: a quantized digest of the identity and every intent record, in stable
**	field order.  Hybrid_Resolve_Map_Identity stores it in identity->Checksum; exposed so a caller
**	can re-derive it over a received record without re-running Phase 2.
*/
unsigned int	Hybrid_Map_Identity_Checksum(const HybridMapIdentity & identity,const HybridArchetypeIntentSet & intents,
															unsigned int seed = 0);


#endif // HYBRIDMAPIDENTITY_H
