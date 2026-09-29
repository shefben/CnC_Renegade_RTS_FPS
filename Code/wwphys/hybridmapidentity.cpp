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
 *                     $Archive:: /Commando/Code/wwphys/hybridmapidentity.cpp                 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Hybrid world generator rewrite, item I3 -- runtime Phase 2.  See hybridmapidentity.h.        *
 *                                                                                             *
 * Draw discipline here is the whole point of the phase, so it is spelled out once:             *
 *                                                                                             *
 *   - every draw is one keyed Hybrid_Hash01 over an already-derived domain seed;               *
 *   - where the plan names a key ("flank_sign", "island", "river_amp", ...) that exact spelling *
 *     is used through Hybrid_Key, so the key is the plan's own name and not a local number;     *
 *   - where it does not, Section 6's rule applies: archetype name + feature StableId +          *
 *     parameter name;                                                                          *
 *   - every range is one Hybrid_Range over one draw, never a midpoint or a tuned value;         *
 *   - normalized strategic values quantize at 1e-6, world lengths at 0.001, angles at 0.001 rad.*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "hybridmapidentity.h"
#include "wwmath.h"
#include "wwdebug.h"

#include <math.h>
#include <string.h>


static inline float Max2(float a,float b)	{ return (a > b) ? a : b; }


/*
**	Section 6.1 anchor and Section 6.10 guide constants, named once instead of repeated inline.
*/
static const float	BASE_GUIDE_U						= 0.72f;
static const float	HOME_FIELD_GUIDE_U				= 0.48f;
static const float	HOME_FIELD_GUIDE_V				= 0.18f;
static const float	FLANK_GUIDE_V						= 0.45f;
static const float	NOD_V_OFFSET_LIMIT				= 0.18f;

static const float	GUIDED_RIVER_END_V				= 0.90f;
static const float	RIVER_VALLEY_FAMILY1_V			= 0.45f;
static const float	RIVER_VALLEY_MEAN_U_LIMIT		= 0.10f;
static const float	TWIN_LAKE_V							= 0.38f;
static const float	TWIN_LAKE_U_JITTER				= 0.05f;
static const float	TWIN_LAKES_FAMILY1_V				= 0.78f;
static const float	CANYON_CROSSING_V					= 0.35f;
static const float	HIGHLAND_ALTERNATE_V				= 0.45f;
static const float	COAST_SEA_V_MIN					= 0.55f;
static const float	COAST_SEA_V_MAX					= 0.70f;
static const float	PENINSULA_NECK_V					= 0.35f;
static const float	COASTAL_RIVER_SOURCE_V			= 0.35f;
static const float	PENINSULA_NECK_WIDTH_W			= 10.0f;
static const float	BASIN_ENTRY_V						= 0.35f;
static const float	RAVINE_CROSSING_V					= 0.32f;
static const float	OPEN_MANEUVER_OUTER_V			= 0.42f;
static const float	OPEN_MANEUVER_RADIUS_W			= 9.0f;


const char * Hybrid_Symmetry_Name(HybridSymmetry symmetry)
{
	switch (symmetry) {
		case HYBRID_SYMMETRY_NONE:			return "none";
		case HYBRID_SYMMETRY_ROTATIONAL:	return "rotational";
		case HYBRID_SYMMETRY_MIRROR_X:	return "mirror_x";
		default:									return "unknown";
	}
}


bool Hybrid_Symmetry_From_Name(const char * name,HybridSymmetry * out)
{
	if ((name == nullptr) || (out == nullptr)) {
		return false;
	}
	for (int i = 0; i < HYBRID_SYMMETRY_COUNT; i++) {
		if (::strcmp (name,Hybrid_Symmetry_Name ((HybridSymmetry)i)) == 0) {
			*out = (HybridSymmetry)i;
			return true;
		}
	}
	return false;
}


const char * Hybrid_Intent_Kind_Name(HybridIntentKind kind)
{
	switch (kind) {
		case HYBRID_INTENT_GUIDED_RIVER:				return "GUIDED_RIVER";
		case HYBRID_INTENT_COASTAL_GUIDED_RIVER:	return "COASTAL_GUIDED_RIVER";
		case HYBRID_INTENT_LAKE_GUIDE:				return "LAKE_GUIDE";
		case HYBRID_INTENT_PRINCIPAL_CANYON:		return "PRINCIPAL_CANYON";
		case HYBRID_INTENT_PRINCIPAL_RIDGE:			return "PRINCIPAL_RIDGE";
		case HYBRID_INTENT_SURFACE_SADDLE:			return "SURFACE_SADDLE";
		case HYBRID_INTENT_COAST_GUIDE:				return "COAST_GUIDE";
		case HYBRID_INTENT_PENINSULA_NECK:			return "PENINSULA_NECK";
		case HYBRID_INTENT_BASIN_GUIDE:				return "BASIN_GUIDE";
		case HYBRID_INTENT_BASIN_ENTRY:				return "BASIN_ENTRY";
		case HYBRID_INTENT_MAIN_RAVINE_GUIDE:		return "MAIN_RAVINE_GUIDE";
		case HYBRID_INTENT_OPEN_MANEUVER:			return "OPEN_MANEUVER";
		case HYBRID_INTENT_ROUTE_FAMILY_CROSSING:	return "ROUTE_FAMILY_CROSSING";
		default:												return "unknown";
	}
}


const char * Hybrid_Map_Identity_Failure_Name(HybridMapIdentityFailure failure)
{
	switch (failure) {
		case HYBRID_IDENTITY_OK:									return "OK";
		case HYBRID_IDENTITY_ARCHETYPE_REQUIRES_COASTLINE:	return "ARCHETYPE_REQUIRES_COASTLINE";
		case HYBRID_IDENTITY_NO_AVAILABLE_BIOME:				return "NO_AVAILABLE_BIOME";
		case HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR:		return "NO_AVAILABLE_BIOME_PAIR";
		case HYBRID_IDENTITY_ILLEGAL_BIOME_PAIR:				return "ILLEGAL_BIOME_PAIR";
		default:															return "unknown";
	}
}


void Hybrid_Build_Strategic_Frame(float center_x,float center_y,float d,float axis_angle,HybridStrategicFrame * out)
{
	if (out == nullptr) {
		return;
	}

	out->Center.Set (Hybrid_Quantize_Position (center_x),Hybrid_Quantize_Position (center_y));
	out->Scale		= Hybrid_Quantize_Position (d * 0.5f);
	out->AxisAngle	= Hybrid_Quantize_Angle (axis_angle);

	/*
	**	The axis is rebuilt from the quantized angle, never from the caller's raw one, so a frame
	**	restored from a checksummed record is bit-identical to the frame that produced it.
	*/
	const float c = WWMath::Cos (out->AxisAngle);
	const float s = WWMath::Sin (out->AxisAngle);
	out->AxisU.Set (c,s);
	out->AxisV.Set (-s,c);			// +90 degrees, Section 0.4
}


Vector2 Hybrid_Mirror_Strategic(HybridSymmetry symmetry,const Vector2 & uv)
{
	switch (symmetry) {
		case HYBRID_SYMMETRY_ROTATIONAL:	return Vector2 (-uv.X,-uv.Y);
		case HYBRID_SYMMETRY_MIRROR_X:	return Vector2 (-uv.X, uv.Y);
		default:									return uv;		// `none` has no paired role
	}
}


/***********************************************************************************************
 * Hybrid_Repair_Ring_Search -- Section 6.1's bounded legality repair                           *
 *                                                                                             *
 * One implementation for every phase that may move a plan-stage position.  Ring radius is      *
 * `index * ring_step` rather than an accumulated sum so the ring set never drifts, and station  *
 * 0 points along strategic `+u` with increasing j rotating toward `+v`, which is                *
 * counter-clockwise in the strategic frame.                                                     *
 *=============================================================================================*/
bool Hybrid_Repair_Ring_Search(const HybridStrategicFrame & frame,float ring_step,float max_offset,
										 const Vector2 & origin_world,HybridPlacementTestFunc test,void * context,
										 Vector2 * out_world)
{
	if ((test == nullptr) || (out_world == nullptr)) {
		return false;
	}

	const float step		= (ring_step > 0.0f) ? ring_step : 1.0f;
	const float station	= (2.0f * WWMATH_PI) / (float)HYBRID_REPAIR_STATIONS_PER_RING;
	const int ring_count	= (int)(max_offset / step);

	for (int ring = 1; ring <= ring_count; ring++) {
		const float radius = step * (float)ring;
		for (int j = 0; j < HYBRID_REPAIR_STATIONS_PER_RING; j++) {

			const float angle	= station * (float)j;
			const float cos_a	= WWMath::Cos (angle);
			const float sin_a	= WWMath::Sin (angle);

			const Vector2 offset ((frame.AxisU.X * cos_a) + (frame.AxisV.X * sin_a),
										(frame.AxisU.Y * cos_a) + (frame.AxisV.Y * sin_a));

			const Vector2 candidate (Hybrid_Quantize_Position (origin_world.X + (offset.X * radius)),
											 Hybrid_Quantize_Position (origin_world.Y + (offset.Y * radius)));

			if (test (candidate,context)) {
				*out_world = candidate;
				return true;
			}
		}
	}

	return false;
}


int HybridArchetypeIntentSet::Count_Of_Kind(HybridIntentKind kind) const
{
	int count = 0;
	for (int i = 0; i < Intents.Count (); i++) {
		if (Intents[i].Kind == kind) count++;
	}
	return count;
}


const HybridArchetypeIntent * HybridArchetypeIntentSet::Find_First(HybridIntentKind kind) const
{
	for (int i = 0; i < Intents.Count (); i++) {
		if (Intents[i].Kind == kind) return &Intents[i];
	}
	return nullptr;
}


int HybridMapIdentity::Island_Slot_Count_Accepted(void) const
{
	int count = 0;
	for (int slot = 0; slot < IslandSlotCount; slot++) {
		if (Island_Slot_Accepted (slot)) count++;
	}
	return count;
}


HybridMapIdentityParams::HybridMapIdentityParams(void) :
	MapPlanSeed(0),HydrologySeed(0),BiomeSeed(0),
	WorldCenterX(0.0f),WorldCenterY(0.0f),
	Symmetry(HYBRID_SYMMETRY_ROTATIONAL),
	ArchetypeSelector(HYBRID_ARCHETYPE_AUTO),
	BiomeModeSelector(HYBRID_BIOME_MODE_AUTO),
	BiomeSelector(HYBRID_BIOME_AUTO),
	SecondaryBiomeSelector(HYBRID_BIOME_AUTO),
	MixedBiomeProbability(0.125f),
	IslandChance(0.20f),
	RouteRedundancy(2),
	FlankLanes(1),
	CoastlineEnabled(true)
{
	::memset (&Constants,0,sizeof (Constants));
	for (int b = 0; b < HYBRID_BIOME_COUNT; b++) {
		BiomeAvailable[b] = true;
	}
}


/***********************************************************************************************
 * Phase 2 step helpers                                                                        *
 *---------------------------------------------------------------------------------------------*/

static bool
Fail(HybridMapIdentityResult * result,HybridMapIdentityFailure failure,const char * detail)
{
	if (result != nullptr) {
		result->Failure	= failure;
		result->Detail		= (detail != nullptr) ? detail : "";
	}
	return false;
}


//	Section 5: optional island slots by size class.
static int
Island_Slot_Count(HybridSizeClass size_class)
{
	switch (size_class) {
		case HYBRID_SIZE_MEDIUM:	return 1;
		case HYBRID_SIZE_LARGE:		return 2;
		default:							return 0;		// SMALL
	}
}


/*
**	Section 6: archetype selection.  The `auto` list is the stable Section-6 order with
**	coastal_peninsula removed when there is no coastline; an explicit archetype is used as given,
**	except that explicitly asking for coastal_peninsula without a coastline is a contradiction the
**	plan resolves by that archetype simply not existing.
*/
static bool
Resolve_Archetype(const HybridMapIdentityParams & params,HybridArchetype * out,HybridMapIdentityResult * result)
{
	if (params.ArchetypeSelector != HYBRID_ARCHETYPE_AUTO) {
		if ((params.ArchetypeSelector == HYBRID_ARCHETYPE_COASTAL_PENINSULA) && !params.CoastlineEnabled) {
			return Fail (result,HYBRID_IDENTITY_ARCHETYPE_REQUIRES_COASTLINE,"coastal_peninsula needs water.coastline");
		}
		*out = params.ArchetypeSelector;
		return true;
	}

	HybridArchetype legal[HYBRID_ARCHETYPE_COUNT];
	int count = 0;
	for (int i = 0; i < HYBRID_ARCHETYPE_COUNT; i++) {
		const HybridArchetype candidate = (HybridArchetype)i;
		if ((candidate == HYBRID_ARCHETYPE_COASTAL_PENINSULA) && !params.CoastlineEnabled) {
			continue;
		}
		legal[count++] = candidate;
	}

	const float h = Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("archetype"));
	*out = legal[Hybrid_Pick_Index (count,h)];
	return true;
}


/*
**	Sections 4.1 to 4.3: biome mode, primary biome, secondary biome.  The selectable primary list is
**	exactly [temperate, desert, snow, tiberium_wasteland] in that order with unavailable entries --
**	and, on a mixed map, entries with no legal available secondary -- removed.  `auto` is never an
**	element of a draw list.
*/
static bool
Compatible_Secondary_Exists(const HybridMapIdentityParams & params,HybridBiome primary)
{
	for (int other = 0; other < HYBRID_BIOME_COUNT; other++) {
		if (!params.BiomeAvailable[other]) continue;
		if ((params.SecondaryBiomeSelector != HYBRID_BIOME_AUTO) && (params.SecondaryBiomeSelector != (HybridBiome)other)) continue;
		if (Hybrid_Is_Legal_Biome_Pair (primary,(HybridBiome)other)) return true;
	}
	return false;
}


static bool
Resolve_Biomes(const HybridMapIdentityParams & params,HybridMapIdentity * identity,HybridMapIdentityResult * result)
{
	//	Step 5 -- biome mode.  One draw decides `auto`.
	if (params.BiomeModeSelector == HYBRID_BIOME_MODE_AUTO) {
		const float r = Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("biome_mode"));
		identity->BiomeMode = (r < params.MixedBiomeProbability) ? HYBRID_BIOME_MODE_MIXED : HYBRID_BIOME_MODE_SINGLE;
	} else {
		identity->BiomeMode = params.BiomeModeSelector;
	}
	const bool mixed = (identity->BiomeMode == HYBRID_BIOME_MODE_MIXED);

	//	Step 6 -- primary biome.
	HybridBiome primary_list[HYBRID_BIOME_COUNT];
	int primary_count = 0;
	for (int b = 0; b < HYBRID_BIOME_COUNT; b++) {
		const HybridBiome candidate = (HybridBiome)b;
		if (!params.BiomeAvailable[b]) continue;
		if (mixed && !Compatible_Secondary_Exists (params,candidate)) continue;
		primary_list[primary_count++] = candidate;
	}
	if (primary_count == 0) {
		return Fail (result,mixed ? HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR : HYBRID_IDENTITY_NO_AVAILABLE_BIOME,
						 mixed ? "no primary biome has an available legal secondary" : "no biome resolves every material slot");
	}

	if (params.BiomeSelector != HYBRID_BIOME_AUTO) {
		bool selectable = false;
		for (int i = 0; i < primary_count; i++) {
			if (primary_list[i] == params.BiomeSelector) selectable = true;
		}
		if (!selectable) {
			return Fail (result,mixed ? HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR : HYBRID_IDENTITY_NO_AVAILABLE_BIOME,
							 Hybrid_Biome_Name (params.BiomeSelector));
		}
		identity->PrimaryBiome = params.BiomeSelector;
	} else {
		const float h = Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("primary_biome"));
		identity->PrimaryBiome = primary_list[Hybrid_Pick_Index (primary_count,h)];
	}

	//	Step 7 -- secondary biome, mixed maps only.
	identity->SecondaryBiome = HYBRID_BIOME_COUNT;
	if (!mixed) {
		return true;
	}

	if (params.SecondaryBiomeSelector != HYBRID_BIOME_AUTO) {
		const HybridBiome secondary = params.SecondaryBiomeSelector;
		if (!params.BiomeAvailable[secondary]) {
			return Fail (result,HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR,Hybrid_Biome_Name (secondary));
		}
		if (!Hybrid_Is_Legal_Biome_Pair (identity->PrimaryBiome,secondary)) {
			StringClass detail (64u,true);
			detail.Format ("%s + %s",Hybrid_Biome_Name (identity->PrimaryBiome),Hybrid_Biome_Name (secondary));
			return Fail (result,HYBRID_IDENTITY_ILLEGAL_BIOME_PAIR,detail.Peek_Buffer ());
		}
		identity->SecondaryBiome = secondary;
		return true;
	}

	HybridBiome secondary_list[HYBRID_BIOME_COUNT];
	int secondary_count = 0;
	for (int b = 0; b < HYBRID_BIOME_COUNT; b++) {
		const HybridBiome candidate = (HybridBiome)b;
		if (!params.BiomeAvailable[b]) continue;
		if (candidate == identity->PrimaryBiome) continue;
		if (!Hybrid_Is_Legal_Biome_Pair (identity->PrimaryBiome,candidate)) continue;
		secondary_list[secondary_count++] = candidate;
	}
	if (secondary_count == 0) {
		return Fail (result,HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR,Hybrid_Biome_Name (identity->PrimaryBiome));
	}

	const float h = Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("secondary_biome"));
	identity->SecondaryBiome = secondary_list[Hybrid_Pick_Index (secondary_count,h)];
	return true;
}


/*
**	Section 4.3 steps 4 to 6 and 9: freeze the mixed divider.  Endpoint `u` values come from
**	Range(-0.15,+0.15) and the two interior control `u` values from Range(-0.30,+0.30), each its own
**	keyed draw in the biomes domain.  Phase 4 owns the 35% ownership test and its repair.
*/
static void
Build_Biome_Divider(const HybridMapIdentityParams & params,const HybridMapIdentity & identity,
						  HybridBiomeDivider * divider)
{
	if (identity.BiomeMode != HYBRID_BIOME_MODE_MIXED) {
		return;
	}

	divider->Present = true;
	for (int i = 0; i < 4; i++) {
		const bool endpoint = (i == 0) || (i == 3);
		const float limit = endpoint ? 0.15f : 0.30f;
		const float h = Hybrid_Hash01 (params.BiomeSeed,Hybrid_Key ("biome_divider"),(unsigned int)i,Hybrid_Key ("u"));
		divider->ControlU[i] = Hybrid_Range (-limit,+limit,h,HYBRID_QUANTUM_NORMALIZED);
	}
	divider->TransitionWidth = Hybrid_Quantize_Position (Max2 (4.0f * params.Constants.S,8.0f * params.Constants.W));
}


/*
**	Section 6.10 guided-river construction.  Called for both GUIDED_RIVER and COASTAL_GUIDED_RIVER:
**	the only differences are the halved amplitude and the forced final point, both of which the
**	caller passes in.
**
**	The centerline is sampled along the straight start-to-end axis "every A".  The station count is
**	RoundHalfUp(length/A) and the stations are spread evenly over that length, so spacing is A to
**	within one quantum and the endpoints stay exactly where the intent put them rather than drifting
**	by the remainder of a fixed-A walk.
*/
static void
Build_Guided_River(const HybridMapIdentityParams & params,const HybridMapIdentity & identity,
						 HybridArchetypeIntent * intent,bool halve_amplitude,bool force_end,bool clamp_mean_u)
{
	const HybridConstants & k = params.Constants;
	const unsigned int archetype_key = Hybrid_Key (Hybrid_Archetype_Name (identity.Archetype));

	float amplitude = Hybrid_Range (0.05f * k.D,0.12f * k.D,
											  Hybrid_Hash01 (params.HydrologySeed,archetype_key,Hybrid_Key ("river_amp")));
	const float wavelength = Hybrid_Range (0.25f * k.D,0.45f * k.D,
													  Hybrid_Hash01 (params.HydrologySeed,archetype_key,Hybrid_Key ("river_wave")));
	const float phase = Hybrid_Range (0.0f,2.0f * WWMATH_PI,
												 Hybrid_Hash01 (params.HydrologySeed,archetype_key,Hybrid_Key ("river_phase")),
												 HYBRID_QUANTUM_ANGLE);
	if (halve_amplitude) {
		amplitude = Hybrid_Quantize_Position (amplitude * 0.5f);
	}

	intent->Amplitude		= amplitude;
	intent->Wavelength	= wavelength;
	intent->Phase			= phase;

	const Vector2 start	= identity.Frame.To_World (intent->Guide);
	const Vector2 end		= identity.Frame.To_World (intent->GuideEnd);
	Vector2 axis (end.X - start.X,end.Y - start.Y);
	const float length = axis.Length ();
	if (length <= 0.0f) {
		intent->Points.Add (Vector2 (Hybrid_Quantize_Position (start.X),Hybrid_Quantize_Position (start.Y)));
		return;
	}
	axis.X /= length;
	axis.Y /= length;
	const Vector2 normal (-axis.Y,axis.X);

	int stations = (int)Hybrid_Round_Half_Up (length / ((k.A > 0.0f) ? k.A : 1.0f));
	if (stations < 1) stations = 1;

	for (int i = 0; i <= stations; i++) {
		const float d = (length * (float)i) / (float)stations;
		float lateral = amplitude * WWMath::Sin ((2.0f * WWMATH_PI * (d / wavelength)) + phase);

		//	COASTAL_GUIDED_RIVER's last point is the sea boundary itself, not a meandered offset.
		if (force_end && (i == stations)) {
			lateral = 0.0f;
		}

		Vector2 point (start.X + (axis.X * d) + (normal.X * lateral),
							start.Y + (axis.Y * d) + (normal.Y * lateral));

		//	Section 6.2 -- river_valley keeps its centerline `u` inside [-0.10,+0.10].
		if (clamp_mean_u) {
			Vector2 uv = identity.Frame.To_Strategic (point);
			if (uv.X < -RIVER_VALLEY_MEAN_U_LIMIT || uv.X > RIVER_VALLEY_MEAN_U_LIMIT) {
				uv.X = (uv.X < 0.0f) ? -RIVER_VALLEY_MEAN_U_LIMIT : RIVER_VALLEY_MEAN_U_LIMIT;
				point = identity.Frame.To_World (uv);
			}
		}

		intent->Points.Add (Vector2 (Hybrid_Quantize_Position (point.X),Hybrid_Quantize_Position (point.Y)));
	}
}


/*
**	Section 6.10 -- the intent table.  Records are created in the table's own left-to-right order and
**	their StableId is that creation order, 1-based, so a draw keyed by StableId is stable.
*/
static HybridArchetypeIntent *
Add_Intent(HybridArchetypeIntentSet * intents,HybridIntentKind kind)
{
	HybridArchetypeIntent intent;
	intent.StableId	= (unsigned int)intents->Intents.Count () + 1;
	intent.Kind			= kind;
	intents->Intents.Add (intent);
	return &intents->Intents[intents->Intents.Count () - 1];
}


static void
Add_Crossing(HybridArchetypeIntentSet * intents,int family,float u,float v,unsigned int flags)
{
	HybridArchetypeIntent * intent = Add_Intent (intents,HYBRID_INTENT_ROUTE_FAMILY_CROSSING);
	intent->RouteFamily	= family;
	intent->Flags			= flags;
	intent->Guide.Set (Hybrid_Quantize_Normalized (u),Hybrid_Quantize_Normalized (v));
	intent->GuideEnd = intent->Guide;
}


static void
Build_Archetype_Intents(const HybridMapIdentityParams & params,const HybridMapIdentity & identity,
								HybridArchetypeIntentSet * intents)
{
	const HybridConstants & k = params.Constants;
	const float s = (float)identity.FlankSign;
	const unsigned int archetype_key = Hybrid_Key (Hybrid_Archetype_Name (identity.Archetype));

	switch (identity.Archetype) {

		case HYBRID_ARCHETYPE_RIVER_VALLEY:
		{
			HybridArchetypeIntent * river = Add_Intent (intents,HYBRID_INTENT_GUIDED_RIVER);
			river->Flags = HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V;
			river->Guide.Set (0.0f,-GUIDED_RIVER_END_V);
			river->GuideEnd.Set (0.0f,+GUIDED_RIVER_END_V);
			Build_Guided_River (params,identity,river,false,false,true);

			Add_Crossing (intents,0,0.0f,0.0f,0);
			Add_Crossing (intents,1,0.0f,-RIVER_VALLEY_FAMILY1_V * s,0);

			intents->RiverAdjacentContested = true;
			break;
		}

		case HYBRID_ARCHETYPE_TWIN_LAKES:
		{
			//	Section 6.3 -- two required lakes, each with its own independent u jitter and radius.
			const float lake_v[2] = { -TWIN_LAKE_V, +TWIN_LAKE_V };
			for (int i = 0; i < 2; i++) {
				HybridArchetypeIntent * lake = Add_Intent (intents,HYBRID_INTENT_LAKE_GUIDE);
				const float u = Hybrid_Range (-TWIN_LAKE_U_JITTER,+TWIN_LAKE_U_JITTER,
														Hybrid_Hash01 (params.MapPlanSeed,archetype_key,lake->StableId,Hybrid_Key ("lake_u_jitter")),
														HYBRID_QUANTUM_NORMALIZED);
				lake->Guide.Set (u,Hybrid_Quantize_Normalized (lake_v[i]));
				lake->GuideEnd = lake->Guide;
				lake->Radius = Hybrid_Range (0.18f * k.D,0.25f * k.D,
														Hybrid_Hash01 (params.MapPlanSeed,archetype_key,lake->StableId,Hybrid_Key ("lake_radius")));
			}

			Add_Crossing (intents,0,0.0f,0.0f,0);
			Add_Crossing (intents,1,0.0f,TWIN_LAKES_FAMILY1_V * s,0);
			break;
		}

		case HYBRID_ARCHETYPE_CANYON_CROSSROADS:
		{
			HybridArchetypeIntent * canyon = Add_Intent (intents,HYBRID_INTENT_PRINCIPAL_CANYON);
			canyon->Flags = HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V;

			Add_Crossing (intents,0,0.0f,+CANYON_CROSSING_V * s,HYBRID_INTENT_FLAG_BRIDGE_REQUIRED);
			Add_Crossing (intents,1,0.0f,-CANYON_CROSSING_V * s,HYBRID_INTENT_FLAG_DISTINCT_CROSSING_REQUIRED);
			break;
		}

		case HYBRID_ARCHETYPE_HIGHLAND_PASS:
		{
			HybridArchetypeIntent * ridge = Add_Intent (intents,HYBRID_INTENT_PRINCIPAL_RIDGE);
			ridge->Flags = HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V;

			//	The one mandatory surface saddle.  Phase 5 applies the Section-6.10 amplitude ramp
			//	around it; Phase 2 only fixes where it is.
			Add_Intent (intents,HYBRID_INTENT_SURFACE_SADDLE);

			/*
			**	The alternate family guide carries no crossing tag.  Section 6.10's table writes those tags out
			**	where it means them -- `ravine_network` tags one guide `BRIDGE_REQUIRED` and the other
			**	`DISTINCT_CROSSING_REQUIRED` in as many words -- and for `highland_pass` it lists only "alternate
			**	family guide `(0,+0.45*s)`".  Tagging it made Phase 3 raise a `RIDGE` crossing obligation, and a
			**	ridge is not a step-18 obstruction: Phase 6 creates windows for rivers, cliffs, canyons and
			**	ravines only, so the obligation could never bind and every `highland_pass` seed failed
			**	`obligation_unbound` with all 59 to 150 windows reported as the wrong obstruction.  Both families
			**	cross this ridge the way the archetype intends -- over the surface saddle, on drivable ground.
			*/
			Add_Crossing (intents,1,0.0f,+HIGHLAND_ALTERNATE_V * s,0);
			break;
		}

		case HYBRID_ARCHETYPE_COASTAL_PENINSULA:
		{
			const int v_sign = Hybrid_Pick_Sign (Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("coast_side")));

			HybridArchetypeIntent * coast = Add_Intent (intents,HYBRID_INTENT_COAST_GUIDE);
			coast->VSign = v_sign;
			const float sea_v = Hybrid_Range (COAST_SEA_V_MIN,COAST_SEA_V_MAX,
														 Hybrid_Hash01 (params.MapPlanSeed,archetype_key,coast->StableId,Hybrid_Key ("sea_boundary_v")),
														 HYBRID_QUANTUM_NORMALIZED);
			coast->Guide.Set (0.0f,sea_v * (float)v_sign);
			coast->GuideEnd = coast->Guide;

			HybridArchetypeIntent * neck = Add_Intent (intents,HYBRID_INTENT_PENINSULA_NECK);
			neck->VSign = v_sign;
			neck->Guide.Set (0.0f,Hybrid_Quantize_Normalized (PENINSULA_NECK_V * (float)v_sign));
			neck->GuideEnd = neck->Guide;
			neck->MinWidth = Hybrid_Quantize_Position (PENINSULA_NECK_WIDTH_W * k.W);

			HybridArchetypeIntent * river = Add_Intent (intents,HYBRID_INTENT_COASTAL_GUIDED_RIVER);
			river->VSign = v_sign;
			river->Guide.Set (0.0f,Hybrid_Quantize_Normalized (-COASTAL_RIVER_SOURCE_V * (float)v_sign));
			river->GuideEnd.Set (0.0f,sea_v * (float)v_sign);
			Build_Guided_River (params,identity,river,true,true,false);
			break;
		}

		case HYBRID_ARCHETYPE_PLATEAU_BASIN:
		{
			HybridArchetypeIntent * basin = Add_Intent (intents,HYBRID_INTENT_BASIN_GUIDE);
			basin->Radius = Hybrid_Range (0.22f * k.D,0.32f * k.D,
													Hybrid_Hash01 (params.MapPlanSeed,archetype_key,basin->StableId,Hybrid_Key ("basin_radius")));

			const float entry_v[2] = { +BASIN_ENTRY_V, -BASIN_ENTRY_V };
			for (int i = 0; i < 2; i++) {
				HybridArchetypeIntent * entry = Add_Intent (intents,HYBRID_INTENT_BASIN_ENTRY);
				entry->RouteFamily = i;
				entry->Guide.Set (0.0f,Hybrid_Quantize_Normalized (entry_v[i] * s));
				entry->GuideEnd = entry->Guide;
			}
			break;
		}

		case HYBRID_ARCHETYPE_RAVINE_NETWORK:
		{
			HybridArchetypeIntent * ravine = Add_Intent (intents,HYBRID_INTENT_MAIN_RAVINE_GUIDE);
			ravine->Flags = HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V;

			Add_Crossing (intents,0,0.0f,+RAVINE_CROSSING_V * s,HYBRID_INTENT_FLAG_BRIDGE_REQUIRED);
			Add_Crossing (intents,1,0.0f,-RAVINE_CROSSING_V * s,HYBRID_INTENT_FLAG_DISTINCT_CROSSING_REQUIRED);
			break;
		}

		case HYBRID_ARCHETYPE_OPEN_FRONTIER:
		{
			//	Section 6.9 -- the centre disc always, the two outer discs on MEDIUM/LARGE.  Phase 4
			//	owns the 95% placeable test and the repair search; Phase 2 fixes the centres.
			const float radius = Hybrid_Quantize_Position (OPEN_MANEUVER_RADIUS_W * k.W);
			const int disc_count = (identity.SizeClass == HYBRID_SIZE_SMALL) ? 1 : 3;
			const float disc_v[3] = { 0.0f, +OPEN_MANEUVER_OUTER_V, -OPEN_MANEUVER_OUTER_V };
			for (int i = 0; i < disc_count; i++) {
				HybridArchetypeIntent * disc = Add_Intent (intents,HYBRID_INTENT_OPEN_MANEUVER);
				disc->Guide.Set (0.0f,Hybrid_Quantize_Normalized (disc_v[i]));
				disc->GuideEnd = disc->Guide;
				disc->Radius = radius;
			}
			break;
		}

		default:
			break;
	}
}


/***********************************************************************************************
 * Hybrid_Resolve_Map_Identity -- runtime Phase 2                                              *
 *---------------------------------------------------------------------------------------------*/
bool Hybrid_Resolve_Map_Identity(const HybridMapIdentityParams & params,HybridMapIdentity * identity,
											HybridArchetypeIntentSet * intents,HybridMapIdentityResult * result)
{
	if ((identity == nullptr) || (intents == nullptr)) {
		return Fail (result,HYBRID_IDENTITY_NO_AVAILABLE_BIOME,"null output record");
	}

	*identity = HybridMapIdentity ();
	intents->Reset ();
	if (result != nullptr) {
		result->Failure = HYBRID_IDENTITY_OK;
		result->Detail = "";
	}

	const HybridConstants & k = params.Constants;

	//	1 -- size class.  Derived from D by Section 0.6; never drawn.  Preflight already rejected a
	//	world outside the envelope, so the class here is always one of the three.
	identity->SizeClass = k.SizeClass;

	//	2 -- symmetry is the profile's own value.
	identity->Symmetry = params.Symmetry;

	//	3 -- archetype.
	if (!Resolve_Archetype (params,&identity->Archetype,result)) {
		return false;
	}

	//	4 -- strategic axis.  `mirror_x` keeps the literal X-axis meaning of its own name; every other
	//	symmetry draws an angle in [0,pi), because an axis and its reverse are the same axis.
	float axis_angle = 0.0f;
	if (params.Symmetry != HYBRID_SYMMETRY_MIRROR_X) {
		axis_angle = Hybrid_Range (0.0f,WWMATH_PI,
											Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("strategic_axis_angle")),
											HYBRID_QUANTUM_ANGLE);
	}
	Hybrid_Build_Strategic_Frame (params.WorldCenterX,params.WorldCenterY,k.D,axis_angle,&identity->Frame);

	//	5, 6, 7 -- biome mode, primary biome, secondary biome.
	if (!Resolve_Biomes (params,identity,result)) {
		return false;
	}

	//	8 -- island slot bits.  One independent Bernoulli test per slot this size class has.
	identity->IslandSlotCount	= Island_Slot_Count (identity->SizeClass);
	identity->IslandSlotBits	= 0;
	for (int slot = 0; slot < identity->IslandSlotCount; slot++) {
		const float h = Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("island"),(unsigned int)slot);
		if (h < params.IslandChance) {
			identity->IslandSlotBits |= (1u << slot);
		}
	}

	/*
	**	Section 6.1 -- the flank sign and the common anchors.  The anchors need no draw of their own
	**	beyond this sign and, for `none`, the Nod lateral offset, so Phase 2 fixes them here and
	**	Phase 3 places roles against them instead of re-deriving the table.
	*/
	identity->FlankSign			= Hybrid_Pick_Sign (Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("flank_sign")));
	identity->RouteFamilyCount	= Hybrid_Route_Family_Count (params.RouteRedundancy,params.FlankLanes);

	const float s = (float)identity->FlankSign;
	identity->GdiBaseGuide.Set (-BASE_GUIDE_U,0.0f);
	if (params.Symmetry == HYBRID_SYMMETRY_NONE) {
		const float nod_v = Hybrid_Range (-NOD_V_OFFSET_LIMIT,+NOD_V_OFFSET_LIMIT,
													 Hybrid_Hash01 (params.MapPlanSeed,Hybrid_Key ("nod_v_offset")),
													 HYBRID_QUANTUM_NORMALIZED);
		identity->NodBaseGuide.Set (+BASE_GUIDE_U,nod_v);
	} else {
		identity->NodBaseGuide.Set (+BASE_GUIDE_U,0.0f);
	}
	identity->GdiHomeFieldGuide.Set (-HOME_FIELD_GUIDE_U,Hybrid_Quantize_Normalized (+HOME_FIELD_GUIDE_V * s));
	identity->NodHomeFieldGuide.Set (+HOME_FIELD_GUIDE_U,Hybrid_Quantize_Normalized (-HOME_FIELD_GUIDE_V * s));
	identity->FrontCenterGuide.Set (0.0f,0.0f);
	identity->FlankGuide[0].Set (0.0f,Hybrid_Quantize_Normalized (+FLANK_GUIDE_V * s));
	identity->FlankGuide[1].Set (0.0f,Hybrid_Quantize_Normalized (-FLANK_GUIDE_V * s));

	//	9 -- the Section-6.10 intent set, plus the Section-4.3 divider the mixed path needs.
	Build_Archetype_Intents (params,*identity,intents);
	Build_Biome_Divider (params,*identity,&intents->Divider);

	identity->Checksum = Hybrid_Map_Identity_Checksum (*identity,*intents,0);

	WWDEBUG_SAY (("HybridMapIdentity: %s %s archetype=%s axis=%.3f biome=%s%s%s islands=%u/%d intents=%d checksum=%08X\n",
						Hybrid_Size_Class_Name (identity->SizeClass),Hybrid_Symmetry_Name (identity->Symmetry),
						Hybrid_Archetype_Name (identity->Archetype),identity->Frame.AxisAngle,
						Hybrid_Biome_Name (identity->PrimaryBiome),
						(identity->BiomeMode == HYBRID_BIOME_MODE_MIXED) ? " + " : "",
						(identity->BiomeMode == HYBRID_BIOME_MODE_MIXED) ? Hybrid_Biome_Name (identity->SecondaryBiome) : "",
						identity->IslandSlotBits,identity->IslandSlotCount,
						intents->Intents.Count (),identity->Checksum));
	return true;
}


unsigned int Hybrid_Map_Identity_Checksum(const HybridMapIdentity & identity,const HybridArchetypeIntentSet & intents,
														unsigned int seed)
{
	HybridHashStream h (seed);

	h.Add_Int ((int)identity.SizeClass);
	h.Add_Int ((int)identity.Symmetry);
	h.Add_Int ((int)identity.Archetype);
	h.Add_Int ((int)identity.BiomeMode);
	h.Add_Int ((int)identity.PrimaryBiome);
	h.Add_Int ((int)identity.SecondaryBiome);
	h.Add_Int (identity.IslandSlotCount);
	h.Add_Uint (identity.IslandSlotBits);
	h.Add_Int (identity.FlankSign);
	h.Add_Int (identity.RouteFamilyCount);

	h.Add_Length (identity.Frame.Center.X);
	h.Add_Length (identity.Frame.Center.Y);
	h.Add_Length (identity.Frame.Scale);
	h.Add_Angle (identity.Frame.AxisAngle);

	const Vector2 guides[7] = {
		identity.GdiBaseGuide, identity.NodBaseGuide,
		identity.GdiHomeFieldGuide, identity.NodHomeFieldGuide,
		identity.FrontCenterGuide, identity.FlankGuide[0], identity.FlankGuide[1]
	};
	for (int i = 0; i < 7; i++) {
		h.Add_Scalar (guides[i].X);
		h.Add_Scalar (guides[i].Y);
	}

	h.Add_Bool (intents.RiverAdjacentContested);
	h.Add_Bool (intents.Divider.Present);
	for (int i = 0; i < 4; i++) {
		h.Add_Scalar (intents.Divider.ControlU[i]);
		h.Add_Scalar (intents.Divider.ControlV[i]);
	}
	h.Add_Length (intents.Divider.TransitionWidth);

	h.Add_Int (intents.Intents.Count ());
	for (int i = 0; i < intents.Intents.Count (); i++) {
		const HybridArchetypeIntent & intent = intents.Intents[i];
		h.Add_Uint (intent.StableId);
		h.Add_Int ((int)intent.Kind);
		h.Add_Uint (intent.Flags);
		h.Add_Int (intent.RouteFamily);
		h.Add_Scalar (intent.Guide.X);
		h.Add_Scalar (intent.Guide.Y);
		h.Add_Scalar (intent.GuideEnd.X);
		h.Add_Scalar (intent.GuideEnd.Y);
		h.Add_Length (intent.Radius);
		h.Add_Length (intent.MinWidth);
		h.Add_Length (intent.Amplitude);
		h.Add_Length (intent.Wavelength);
		h.Add_Angle (intent.Phase);
		h.Add_Int (intent.VSign);
		h.Add_Int (intent.Points.Count ());
		for (int p = 0; p < intent.Points.Count (); p++) {
			h.Add_Length (intent.Points[p].X);
			h.Add_Length (intent.Points[p].Y);
		}
	}

	return h.Get ();
}
