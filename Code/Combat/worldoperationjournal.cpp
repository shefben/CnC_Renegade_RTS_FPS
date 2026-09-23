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
 *                     $Archive:: /Commando/Code/Combat/worldoperationjournal.cpp             $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldoperationjournal.h"

#include "aabox.h"
#include "bridgesystem.h"
#include "crc.h"
#include "escarpment.h"
#include "generatedworldplacementrules.h"
#include "heightfield.h"
#include "navmeshgenerator.h"
#include "roadsystem.h"
#include "vector2.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwmath.h"


bool				WorldOperationJournal::Open						= false;
unsigned int	WorldOperationJournal::Revision					= 0;
unsigned int	WorldOperationJournal::NextOperationId			= 1;

int				WorldOperationJournal::AppliedCount				= 0;
int				WorldOperationJournal::RejectedCount			= 0;
int				WorldOperationJournal::NavigationRebuildCount	= 0;

bool				WorldOperationJournal::ShallowWaterReclamationEnabled	= false;
float				WorldOperationJournal::MaxReclaimableDepth		= 2.0f;

WorldOperationCraterLimits	WorldOperationJournal::CraterLimits;

int				WorldOperationJournal::BaseVertexCountX			= 0;
int				WorldOperationJournal::BaseVertexCountY			= 0;
DynamicVectorClass<float>	WorldOperationJournal::BaseHeights;

DynamicVectorClass<WorldOperationClass>	WorldOperationJournal::Operations;

int				WorldOperationJournal::ReservedCountX			= 0;
int				WorldOperationJournal::ReservedCountY			= 0;
DynamicVectorClass<int>	WorldOperationJournal::ReservedChunks;


void WorldOperationJournal::Init(void)		{ Close(); }
void WorldOperationJournal::Shutdown(void)	{ Close(); }


void WorldOperationJournal::Close(void)
{
	Open						= false;
	Revision					= 0;
	NextOperationId		= 1;
	AppliedCount			= 0;
	RejectedCount			= 0;
	NavigationRebuildCount	= 0;

	BaseVertexCountX		= 0;
	BaseVertexCountY		= 0;
	BaseHeights.Delete_All();
	Operations.Delete_All();

	Release_All_Chunks();
}


int	WorldOperationJournal::Get_Operation_Count(void)			{ return Operations.Count(); }
int	WorldOperationJournal::Get_Applied_Count(void)				{ return AppliedCount; }
int	WorldOperationJournal::Get_Rejected_Count(void)			{ return RejectedCount; }
int	WorldOperationJournal::Get_Navigation_Rebuild_Count(void)	{ return NavigationRebuildCount; }

void	WorldOperationJournal::Set_Shallow_Water_Reclamation_Enabled(bool enabled)
																					{ ShallowWaterReclamationEnabled = enabled; }
bool	WorldOperationJournal::Is_Shallow_Water_Reclamation_Enabled(void)
																					{ return ShallowWaterReclamationEnabled; }

float	WorldOperationJournal::Get_Max_Reclaimable_Depth(void)	{ return MaxReclaimableDepth; }

void	WorldOperationJournal::Set_Max_Reclaimable_Depth(float depth)
{
	//	A non-positive reclaimable depth would make every reclamation refuse, including the ones
	//	the feature exists for, so it is a broken setting rather than a strict one.
	MaxReclaimableDepth = (depth > 0.0f) ? depth : MaxReclaimableDepth;
}

const WorldOperationCraterLimits &	WorldOperationJournal::Get_Crater_Limits(void)	{ return CraterLimits; }

void WorldOperationJournal::Set_Crater_Limits(const WorldOperationCraterLimits & limits)
{
	CraterLimits = limits;

	WorldOperationCraterLimits defaults;
	if (!(CraterLimits.MaxRadius > 0.0f))			CraterLimits.MaxRadius				= defaults.MaxRadius;
	if (!(CraterLimits.MaxDepth > 0.0f))			CraterLimits.MaxDepth				= defaults.MaxDepth;
	if (CraterLimits.MaxCratersPerWindow <= 0)	CraterLimits.MaxCratersPerWindow	= defaults.MaxCratersPerWindow;
	if (!(CraterLimits.RateWindowSeconds > 0.0f))	CraterLimits.RateWindowSeconds	= defaults.RateWindowSeconds;
}


bool WorldOperationJournal::Get_Operation(int index,WorldOperationClass * out)
{
	if ((index < 0) || (index >= Operations.Count()) || (out == nullptr)) return false;
	*out = Operations[index];
	return true;
}


/***********************************************************************************************
**	Step 2 -- open the journal over the terrain as it stands.
*/
bool WorldOperationJournal::Begin(StringClass * error_out)
{
	Close();

	if (!WorldTerrainSystem::Has_Terrain()) {
		if (error_out) *error_out = "no terrain to open a journal over";
		return false;
	}

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) {
		if (error_out) *error_out = "the terrain system reports terrain but holds no heightfield";
		return false;
	}

	int count_x = heightfield->Get_Vertex_Count_X();
	int count_y = heightfield->Get_Vertex_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) {
		if (error_out) *error_out = "the heightfield is empty";
		return false;
	}

	//	The base snapshot.  Stored heights, as the heightfield holds them -- relative to its own
	//	origin -- because that is exactly what Set_Heights takes back.  Converting to world Z here
	//	and back later would be two conversions that could disagree.
	BaseHeights.Resize(count_x * count_y);
	for (int iy = 0; iy < count_y; iy++) {
		for (int ix = 0; ix < count_x; ix++) {
			BaseHeights.Add(heightfield->Get_Height(ix,iy));
		}
	}

	BaseVertexCountX	= count_x;
	BaseVertexCountY	= count_y;

	Revision				= 1;
	NextOperationId	= 1;
	Open					= true;

	return true;
}


bool WorldOperationJournal::Restore_Base_Heights(void)
{
	if (!Open) return false;

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return false;

	//	A terrain resized under an open journal makes the snapshot meaningless.  Detected rather
	//	than papered over: writing a differently-shaped array back would silently reinterpret
	//	every row.
	if ((heightfield->Get_Vertex_Count_X() != BaseVertexCountX) ||
		 (heightfield->Get_Vertex_Count_Y() != BaseVertexCountY)) {
		return false;
	}
	if (BaseHeights.Count() != (BaseVertexCountX * BaseVertexCountY)) return false;

	return WorldTerrainSystem::Set_Heights(&BaseHeights[0],BaseHeights.Count());
}


/***********************************************************************************************
**	Step 5 -- transaction reservation of affected chunks.
**
**	The grid is the terrain's own patch grid, which is the grid the chunk registry is built on, so
**	a reservation and a chunk-registry entry mean the same square of ground.  Available whenever
**	terrain is, where the chunk registry is only available once something has built one.
*/
static bool Patch_Grid(int * count_x_out,int * count_y_out,
							  float * origin_x_out,float * origin_y_out,
							  float * size_x_out,float * size_y_out)
{
	if (!WorldTerrainSystem::Has_Terrain()) return false;

	int count_x = WorldTerrainSystem::Get_Patch_Count_X();
	int count_y = WorldTerrainSystem::Get_Patch_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	AABoxClass first;
	if (!WorldTerrainSystem::Get_Patch_Bounds(0,0,&first)) return false;

	float size_x = first.Extent.X * 2.0f;
	float size_y = first.Extent.Y * 2.0f;
	if ((size_x <= 0.0f) || (size_y <= 0.0f)) return false;

	*count_x_out	= count_x;
	*count_y_out	= count_y;
	*origin_x_out	= first.Center.X - first.Extent.X;
	*origin_y_out	= first.Center.Y - first.Extent.Y;
	*size_x_out		= size_x;
	*size_y_out		= size_y;
	return true;
}


bool WorldOperationJournal::Is_Chunk_Reserved(int px,int py)
{
	if ((px < 0) || (py < 0) || (px >= ReservedCountX) || (py >= ReservedCountY)) return false;
	return ReservedChunks[px + (py * ReservedCountX)] != 0;
}


int WorldOperationJournal::Get_Reserved_Chunk_Count(void)
{
	int count = 0;
	for (int i = 0; i < ReservedChunks.Count(); i++) {
		if (ReservedChunks[i] != 0) count++;
	}
	return count;
}


void WorldOperationJournal::Release_All_Chunks(void)
{
	ReservedCountX = 0;
	ReservedCountY = 0;
	ReservedChunks.Delete_All();
}


bool WorldOperationJournal::Reserve_Chunks(const Vector3 & center,float radius,int * reserved_count_out)
{
	int count_x = 0, count_y = 0;
	float origin_x = 0.0f, origin_y = 0.0f, size_x = 0.0f, size_y = 0.0f;
	if (!Patch_Grid(&count_x,&count_y,&origin_x,&origin_y,&size_x,&size_y)) return false;

	if (ReservedChunks.Count() != (count_x * count_y)) {
		ReservedChunks.Delete_All();
		ReservedChunks.Resize(count_x * count_y);
		for (int i = 0; i < (count_x * count_y); i++) {
			ReservedChunks.Add(0);
		}
		ReservedCountX = count_x;
		ReservedCountY = count_y;
	}

	int min_x = (int)WWMath::Floor((center.X - radius - origin_x) / size_x);
	int min_y = (int)WWMath::Floor((center.Y - radius - origin_y) / size_y);
	int max_x = (int)WWMath::Floor((center.X + radius - origin_x) / size_x);
	int max_y = (int)WWMath::Floor((center.Y + radius - origin_y) / size_y);

	if (min_x < 0) min_x = 0;
	if (min_y < 0) min_y = 0;
	if (max_x > (count_x - 1)) max_x = count_x - 1;
	if (max_y > (count_y - 1)) max_y = count_y - 1;

	//	Nothing on the field: a legal request, and an empty reservation.  Refusing here would make
	//	an operation whose region is entirely off the map fail for the wrong reason.
	if ((min_x > max_x) || (min_y > max_y)) {
		if (reserved_count_out != nullptr) *reserved_count_out = 0;
		return true;
	}

	//	All or nothing.  A half-taken reservation is worse than a refused one: the caller would
	//	hold some of the ground it needs and none of the right to the rest.
	for (int py = min_y; py <= max_y; py++) {
		for (int px = min_x; px <= max_x; px++) {
			if (ReservedChunks[px + (py * count_x)] != 0) return false;
		}
	}

	int taken = 0;
	for (int py = min_y; py <= max_y; py++) {
		for (int px = min_x; px <= max_x; px++) {
			ReservedChunks[px + (py * count_x)] = 1;
			taken++;
		}
	}

	if (reserved_count_out != nullptr) *reserved_count_out = taken;
	return true;
}


/***********************************************************************************************
**	Step 4's protected-region rules, per type.
**
**	Which features forbid an operation depends on the operation, and the two that must not be
**	treated like the rest are the two whose whole subject is water: carving a channel is supposed
**	to move water, and reclaiming shallow water is supposed to fill it.  Everything else is
**	forbidden to every type, except that restoring ground to what it already was is always safe --
**	it cannot damage a feature it is putting back.
*/
static unsigned int Permitted_Features(WorldOperationType type)
{
	switch (type) {

		case WORLD_OP_CARVE_CHANNEL:
		case WORLD_OP_RECLAIM_SHALLOW_WATER:
			return (1u << GENERATED_WORLD_EXCLUSION_WATER);

		case WORLD_OP_RESTORE_REGION:
			//	Everything except the boundary band, which is never mutable in either direction --
			//	Section 5.9's reserved geometry, and directive 1.5's mandatory cliffs.
			return (1u << GENERATED_WORLD_EXCLUSION_WATER)
					| (1u << GENERATED_WORLD_EXCLUSION_IMMUTABLE_CLIFF)
					| (1u << GENERATED_WORLD_EXCLUSION_TUNNEL)
					| (1u << GENERATED_WORLD_EXCLUSION_BRIDGE)
					| (1u << GENERATED_WORLD_EXCLUSION_RESOURCE_ROUTE);

		default:
			return 0;
	}
}


WorldOperationResultType WorldOperationJournal::Validate_Internal(const WorldOperationClass & operation,
																					  StringClass * detail_out,
																					  Vector3 * center_out,float * radius_out)
{
	if (detail_out != nullptr) *detail_out = "";

	WorldOperationResultType parameters = operation.Validate_Parameters();
	if (parameters != WORLD_OP_RESULT_APPLIED) {
		if (detail_out) *detail_out = World_Operation_Type_Name(operation.Type);
		return parameters;
	}

	if (!Open)											return WORLD_OP_RESULT_NO_JOURNAL;
	if (!WorldTerrainSystem::Has_Terrain())	return WORLD_OP_RESULT_NO_TERRAIN;

	//	Step 3's "if enabled".
	if ((operation.Type == WORLD_OP_RECLAIM_SHALLOW_WATER) && !ShallowWaterReclamationEnabled) {
		if (detail_out) *detail_out = "shallow water reclamation is not enabled for this world";
		return WORLD_OP_RESULT_DISABLED;
	}

	//	Step 6.  Only for the types whose result depends on the ground they find.
	if (World_Operation_Ordering_Matters(operation.Type) && (operation.BaseRevision != Revision)) {
		if (detail_out) {
			detail_out->Format("submitted against revision %u, world is at %u",
									 operation.BaseRevision,Revision);
		}
		return WORLD_OP_RESULT_STALE_REVISION;
	}

	Vector3 center(0.0f,0.0f,0.0f);
	float radius = 0.0f;
	if (!operation.Get_Affected_Bounds(&center,&radius)) {
		return WORLD_OP_RESULT_BAD_PARAMETERS;
	}
	if (center_out != nullptr) *center_out = center;
	if (radius_out != nullptr) *radius_out = radius;

	/*
	**	Step 15's "no cliff-band mutation", applied to every type rather than only to craters.
	**	`WorldTerrainSystem`'s own edit gate already refuses silently; refusing here first is what
	**	turns a no-op into an answer the submitter can read.
	*/
	if (WorldBoundsService::Is_Configured() &&
		 !WorldBoundsService::Can_Place_In_Region(Vector2(center.X,center.Y),radius)) {
		if (detail_out) *detail_out = "the affected region reaches the boundary cliff band";
		return WORLD_OP_RESULT_BOUNDARY_BAND;
	}

	//	Step 4's protected-region rules.
	{
		unsigned int present	= GeneratedWorldPlacementRules::Get_Static_Feature_Mask(center,radius);
		unsigned int blocking	= present & ~Permitted_Features(operation.Type);

		//	The band is never permitted, and is reported as itself rather than as "protected".
		if ((blocking & (1u << GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND)) != 0) {
			if (detail_out) *detail_out = "the affected region reaches the boundary cliff band";
			return WORLD_OP_RESULT_BOUNDARY_BAND;
		}

		if (blocking != 0) {
			for (int i = 1; i < GENERATED_WORLD_EXCLUSION_COUNT; i++) {
				if ((blocking & (1u << i)) == 0) continue;
				if (detail_out) *detail_out = Generated_World_Exclusion_Name((GeneratedWorldExclusionType)i);
				break;
			}
			return WORLD_OP_RESULT_PROTECTED_REGION;
		}
	}

	//	Step 15's bounded crater rules.
	if (operation.Type == WORLD_OP_CREATE_CRATER) {

		if (operation.Radius > CraterLimits.MaxRadius) {
			if (detail_out) {
				detail_out->Format("radius %.2f above the %.2f limit",operation.Radius,CraterLimits.MaxRadius);
			}
			return WORLD_OP_RESULT_CRATER_LIMIT;
		}
		if (operation.Depth > CraterLimits.MaxDepth) {
			if (detail_out) {
				detail_out->Format("depth %.2f above the %.2f limit",operation.Depth,CraterLimits.MaxDepth);
			}
			return WORLD_OP_RESULT_CRATER_LIMIT;
		}

		//	The rate limit reads the journal rather than a counter, because the journal is the one
		//	record that survives a replay -- and a replay that hit the rate limit differently from
		//	the original run would not be a replay.
		int recent = 0;
		for (int i = 0; i < Operations.Count(); i++) {
			if (Operations[i].Type != WORLD_OP_CREATE_CRATER) continue;
			float age = operation.TimeSeconds - Operations[i].TimeSeconds;
			if ((age >= 0.0f) && (age <= CraterLimits.RateWindowSeconds)) recent++;
		}
		if (recent >= CraterLimits.MaxCratersPerWindow) {
			if (detail_out) {
				detail_out->Format("%d crater(s) already inside the %.1fs window",
										 recent,CraterLimits.RateWindowSeconds);
			}
			return WORLD_OP_RESULT_RATE_LIMITED;
		}
	}

	/*
	**	Reclamation's own precondition: the disc has to be submerged, all of it.
	**
	**	A partly dry disc raised to the waterline would *lower* the dry part, which is not
	**	reclamation, it is levelling by another name -- and it would do it without any of
	**	FLATTEN's own checks.  Refused rather than reinterpreted.
	*/
	if (operation.Type == WORLD_OP_RECLAIM_SHALLOW_WATER) {

		float water_height = 0.0f;
		if (!WaterSystem::Get_Water_Height(operation.Center.X,operation.Center.Y,&water_height)) {
			if (detail_out) *detail_out = "no water over the centre of the region";
			return WORLD_OP_RESULT_BAD_PARAMETERS;
		}

		static const int RIM_SAMPLES = 8;
		for (int i = 0; i <= RIM_SAMPLES; i++) {

			float x = operation.Center.X;
			float y = operation.Center.Y;
			if (i < RIM_SAMPLES) {
				float angle = ((float)i / (float)RIM_SAMPLES) * 2.0f * WWMATH_PI;
				x += WWMath::Cos(angle) * operation.Radius;
				y += WWMath::Sin(angle) * operation.Radius;
			}

			float ground = 0.0f;
			if (!WorldTerrainSystem::Sample_Height(x,y,&ground)) continue;

			if (ground >= water_height) {
				if (detail_out) *detail_out = "part of the region is already dry";
				return WORLD_OP_RESULT_BAD_PARAMETERS;
			}
			if ((water_height - ground) > MaxReclaimableDepth) {
				if (detail_out) {
					detail_out->Format("%.2f deep, above the %.2f reclaimable limit",
											 water_height - ground,MaxReclaimableDepth);
				}
				return WORLD_OP_RESULT_BAD_PARAMETERS;
			}
		}
	}

	return WORLD_OP_RESULT_APPLIED;
}


WorldOperationResultType WorldOperationJournal::Validate(const WorldOperationClass & operation,
																			StringClass * detail_out)
{
	return Validate_Internal(operation,detail_out,nullptr,nullptr);
}


/***********************************************************************************************
**	Step 7 -- the mutation itself, through the one canonical terrain owner.
*/
bool WorldOperationJournal::Apply_Mutation(const WorldOperationClass & operation)
{
	switch (operation.Type) {

		case WORLD_OP_RAISE:
			WorldTerrainSystem::Modify_Height_Region(operation.Center,operation.Radius,operation.Depth);
			return true;

		//	LOWER and CREATE_CRATER are the same shape deliberately: a crater *is* a bounded
		//	lowering, and giving it its own dish would be a second falloff to keep in step with the
		//	first.  What makes it a crater is the rules in Validate_Internal, not the arithmetic.
		case WORLD_OP_LOWER:
		case WORLD_OP_CREATE_CRATER:
			WorldTerrainSystem::Modify_Height_Region(operation.Center,operation.Radius,-operation.Depth);
			return true;

		case WORLD_OP_FLATTEN:
			WorldTerrainSystem::Apply_Flatten(operation.Center,operation.Radius,
														 operation.Center.Z,operation.Blend);
			return true;

		case WORLD_OP_SMOOTH:
			WorldTerrainSystem::Apply_Smooth(operation.Center,operation.Radius,operation.Blend);
			return true;

		case WORLD_OP_GRADE_FOUNDATION:
			WorldTerrainSystem::Apply_Foundation_Grade(operation.Center,operation.Radius,
																	 operation.Blend,operation.Center.Z);
			return true;

		case WORLD_OP_CARVE_ROAD:
			if (operation.Path.Count() < 2) return false;
			WorldTerrainSystem::Apply_Road_Grade(&operation.Path[0],operation.Path.Count(),
															 operation.Width,operation.Blend);
			return true;

		case WORLD_OP_CARVE_CHANNEL:
			if (operation.Path.Count() < 2) return false;
			WorldTerrainSystem::Apply_River_Cut(&operation.Path[0],operation.Path.Count(),
															operation.Width,operation.Depth);
			return true;

		case WORLD_OP_RECLAIM_SHALLOW_WATER:
		{
			//	The whole disc is submerged and shallow -- Validate_Internal proved both -- so
			//	pulling it to the waterline raises every vertex in it and lowers none.
			float water_height = 0.0f;
			if (!WaterSystem::Get_Water_Height(operation.Center.X,operation.Center.Y,&water_height)) {
				return false;
			}
			WorldTerrainSystem::Apply_Flatten(operation.Center,operation.Radius,water_height,1.0f);
			return true;
		}

		case WORLD_OP_RESTORE_REGION:
			if (BaseHeights.Count() != (BaseVertexCountX * BaseVertexCountY)) return false;
			return WorldTerrainSystem::Apply_Height_Patch(operation.Center,operation.Radius,
																		 &BaseHeights[0],
																		 BaseVertexCountX,BaseVertexCountY);

		default:
			return false;
	}
}


/***********************************************************************************************
**	Steps 8-11 -- drive each required dependency to its commit point.
**
**	A dependency whose system is not present in this process reaches its commit point trivially --
**	there is nothing holding a stale view of the ground -- and says so in the detail rather than
**	pretending work was done.  That is a real state, not a test artefact: a dedicated server has no
**	render data, and a headless tool has no physics scene.
*/
unsigned int WorldOperationJournal::Satisfy_Dependencies(unsigned int required,StringClass * detail_out)
{
	unsigned int satisfied = 0;
	StringClass detail ("");

	if ((required & WORLD_OP_DEPENDS_RENDER) != 0) {
		//	The shaping primitives already marked their own patches dirty and invalidated the far
		//	region; this is the refill.
		WorldTerrainSystem::Update_Far_Terrain();
		satisfied |= WORLD_OP_DEPENDS_RENDER;
		detail += "render; ";
	}

	if ((required & WORLD_OP_DEPENDS_COLLISION) != 0) {
		if (WorldTerrainSystem::Has_Collision()) {
			WorldTerrainSystem::Build_Collision();
			detail += "collision; ";
		} else {
			detail += "collision (none built); ";
		}
		satisfied |= WORLD_OP_DEPENDS_COLLISION;
	}

	if ((required & WORLD_OP_DEPENDS_NAV) != 0) {

		//	The chunk registry's slope and Buildable flags were computed from ground that just
		//	moved, and the sector graph is built from the registry -- so the registry first, and
		//	against the same cap it was built with.
		if (WorldTerrainSystem::Has_Chunk_Registry()) {
			WorldTerrainSystem::Build_Chunk_Registry(WorldTerrainSystem::Get_Chunk_Max_Buildable_Slope());
		}

		if (NavMeshGenerator::Has_Navigation()) {

			NavMeshParams params;
			StringClass nav_error ("");
			if (NavMeshGenerator::Build(params,&nav_error)) {
				NavigationRebuildCount++;
				satisfied |= WORLD_OP_DEPENDS_NAV;
				detail += "nav; ";

				StringClass choke_error ("");
				GeneratedWorldPlacementRules::Build_Chokepoints(&choke_error);
			} else {
				detail += "nav NOT rebuilt (";
				detail += nav_error;
				detail += "); ";
			}
		} else {
			satisfied |= WORLD_OP_DEPENDS_NAV;
			detail += "nav (none built); ";
		}
	}

	if ((required & WORLD_OP_DEPENDS_WATER) != 0) {
		if (WaterSystem::Get_Area_Count() > 0) {
			WaterSystem::Stamp_All_Masks();
			detail += "water; ";
		} else {
			detail += "water (no areas); ";
		}
		satisfied |= WORLD_OP_DEPENDS_WATER;
	}

	if ((required & WORLD_OP_DEPENDS_ROADS) != 0) {
		//	Roads, bridge abutments and cliff benches all sit on ground that has moved.  Each is
		//	asked of its own owner; none of them is recomputed here.
		if (RoadSystem::Get_Road_Count() > 0) {
			RoadSystem::Apply_Grades();
			detail += "roads; ";
		} else {
			detail += "roads (none); ";
		}
		if (BridgeSystem::Get_Bridge_Count() > 0) {
			BridgeSystem::Conform_Abutments();
			detail += "bridge abutments; ";
		}
		if (EscarpmentSystem::Get_Escarpment_Count() > 0) {
			EscarpmentSystem::Conform_To_Terrain();
			detail += "cliff benches; ";
		}
		satisfied |= WORLD_OP_DEPENDS_ROADS;
	}

	if ((required & WORLD_OP_DEPENDS_MATERIALS) != 0) {
		if (RoadSystem::Get_Road_Count() > 0) {
			RoadSystem::Stamp_Road_Mask();
			detail += "road mask; ";
		} else {
			detail += "materials (no roads); ";
		}
		satisfied |= WORLD_OP_DEPENDS_MATERIALS;
	}

	if (detail_out != nullptr) *detail_out = detail;
	return satisfied;
}


/***********************************************************************************************
**	Steps 5-14 -- the whole submission.
*/
WorldOperationApplyResult WorldOperationJournal::Submit(const WorldOperationClass & operation)
{
	WorldOperationApplyResult result;
	result.Revision				= Revision;
	result.DependenciesRequired	= World_Operation_Dependencies(operation.Type);

	Vector3 center(0.0f,0.0f,0.0f);
	float radius = 0.0f;

	StringClass detail ("");
	result.Result = Validate_Internal(operation,&detail,&center,&radius);
	result.Detail = detail;

	if (result.Result != WORLD_OP_RESULT_APPLIED) {
		RejectedCount++;
		return result;
	}

	//	Step 5.  Held from here until every dependency has committed -- the window in which the
	//	ground is inconsistent with the systems that describe it.
	int reserved = 0;
	if (!Reserve_Chunks(center,radius,&reserved)) {
		result.Result = WORLD_OP_RESULT_CHUNKS_RESERVED;
		result.Detail = "another operation already holds ground this one needs";
		RejectedCount++;
		return result;
	}
	result.AffectedChunkCount = reserved;

	WorldOperationClass recorded = operation;
	recorded.OperationId = NextOperationId;

	//	Step 7.
	if (!Apply_Mutation(recorded)) {
		Release_All_Chunks();
		result.Result = WORLD_OP_RESULT_BAD_PARAMETERS;
		result.Detail = "the mutation was refused by the terrain owner";
		RejectedCount++;
		return result;
	}

	//	Steps 8-11.
	StringClass dependency_detail ("");
	result.DependenciesSatisfied = Satisfy_Dependencies(result.DependenciesRequired,&dependency_detail);

	Release_All_Chunks();

	//	Step 13.  Appended whether or not the revision advances: the operation really did change
	//	the ground, and a journal that omitted it could not replay the world.
	Operations.Add(recorded);
	NextOperationId++;
	AppliedCount++;

	result.OperationId		= recorded.OperationId;
	result.OperationChecksum	= recorded.Compute_Checksum();
	result.TerrainChecksum	= Get_Terrain_Checksum();

	/*
	**	Step 12.  The revision advances only once every required dependency has reached its commit
	**	point.  A peer that sees revision N+1 is entitled to ask the pathfinder about revision
	**	N+1's ground, so a revision published ahead of the rebuild is a promise the world cannot
	**	keep.
	*/
	if (result.DependenciesSatisfied == result.DependenciesRequired) {
		Revision++;
		result.Committed = true;
	}
	result.Revision = Revision;

	StringClass combined ("");
	combined += dependency_detail;
	if (!result.Committed) {
		combined += "revision NOT advanced -- a required dependency did not commit";
	}
	result.Detail = combined;

	return result;
}


/***********************************************************************************************
**	Step 16 -- replay.
*/
bool WorldOperationJournal::Replay(StringClass * report_out)
{
	if (!Open) {
		if (report_out) *report_out = "no journal open";
		return false;
	}

	if (!Restore_Base_Heights()) {
		if (report_out) *report_out = "the base snapshot no longer matches the heightfield";
		return false;
	}

	int replayed = 0;
	int refused = 0;

	for (int i = 0; i < Operations.Count(); i++) {
		if (Apply_Mutation(Operations[i])) {
			replayed++;
		} else {
			refused++;
		}
	}

	//	One dependency pass at the end rather than one per operation.  Replay reproduces a state,
	//	and the state every system needs to see is the one after the last operation -- rebuilding
	//	nav nine times to arrive at the same graph would only make replay slower.
	unsigned int required = 0;
	for (int i = 0; i < Operations.Count(); i++) {
		required |= World_Operation_Dependencies(Operations[i].Type);
	}

	StringClass dependency_detail ("");
	if (required != 0) {
		Satisfy_Dependencies(required,&dependency_detail);
	}

	if (report_out != nullptr) {
		report_out->Format("replayed %d of %d operation(s), %d refused; %s",
								 replayed,Operations.Count(),refused,dependency_detail.Peek_Buffer());
	}

	return refused == 0;
}


bool WorldOperationJournal::Get_Base_Dimensions(int * count_x_out,int * count_y_out)
{
	if (!Open) return false;
	if (count_x_out != nullptr) *count_x_out = BaseVertexCountX;
	if (count_y_out != nullptr) *count_y_out = BaseVertexCountY;
	return true;
}


bool WorldOperationJournal::Get_Base_Height(int ix,int iy,float * height_out)
{
	if (!Open) return false;
	if ((ix < 0) || (iy < 0) || (ix >= BaseVertexCountX) || (iy >= BaseVertexCountY)) return false;
	if (BaseHeights.Count() != (BaseVertexCountX * BaseVertexCountY)) return false;

	if (height_out != nullptr) *height_out = BaseHeights[(iy * BaseVertexCountX) + ix];
	return true;
}


bool WorldOperationJournal::Adopt_Revision(unsigned int revision,StringClass * error_out)
{
	if (!Open) {
		if (error_out) *error_out = "no journal open to adopt a revision into";
		return false;
	}
	if (revision == 0) {
		if (error_out) *error_out = "revision 0 means no journal, and cannot be adopted";
		return false;
	}

	Revision = revision;
	return true;
}


bool WorldOperationJournal::Rebase(StringClass * error_out)
{
	if (!Open) {
		if (error_out) *error_out = "no journal open";
		return false;
	}

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) {
		if (error_out) *error_out = "no heightfield to rebase onto";
		return false;
	}

	int count_x = heightfield->Get_Vertex_Count_X();
	int count_y = heightfield->Get_Vertex_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) {
		if (error_out) *error_out = "the heightfield is empty";
		return false;
	}

	BaseHeights.Delete_All();
	BaseHeights.Resize(count_x * count_y);
	for (int iy = 0; iy < count_y; iy++) {
		for (int ix = 0; ix < count_x; ix++) {
			BaseHeights.Add(heightfield->Get_Height(ix,iy));
		}
	}

	BaseVertexCountX	= count_x;
	BaseVertexCountY	= count_y;

	//	The revision does not move.  A compaction changes how the world is described, not what it
	//	is, and a peer holding revision N must not be told the world advanced because a server
	//	tidied its own log.
	Operations.Delete_All();

	return true;
}


unsigned int WorldOperationJournal::Get_Journal_Checksum(void)
{
	CRCEngine crc;

	int count = Operations.Count();
	crc(&count,sizeof(count));
	crc(&Revision,sizeof(Revision));

	for (int i = 0; i < Operations.Count(); i++) {
		unsigned int operation_checksum = Operations[i].Compute_Checksum();
		crc(&operation_checksum,sizeof(operation_checksum));
	}

	return (unsigned int)crc;
}


unsigned int WorldOperationJournal::Get_Terrain_Checksum(void)
{
	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) return 0;

	int count_x = heightfield->Get_Vertex_Count_X();
	int count_y = heightfield->Get_Vertex_Count_Y();

	CRCEngine crc;
	crc(&count_x,sizeof(count_x));
	crc(&count_y,sizeof(count_y));

	/*
	**	Section 3B.4 quantization, for the same reason every other checksum in this chain uses it:
	**	two peers whose ground agrees to the last millimetre must produce the same number.
	**
	**	Rounded rather than truncated, matching worldsnapshot.cpp.  A peer that rebuilt its ground
	**	from snapshots holds the dequantized heights where the server holds the originals, and a
	**	truncating quantizer can put those a millimetre apart -- see that file for why the round
	**	trip has to be exact.
	*/
	for (int iy = 0; iy < count_y; iy++) {
		for (int ix = 0; ix < count_x; ix++) {
			int quantized = (int)WWMath::Floor((heightfield->Get_Height(ix,iy) * 1000.0f) + 0.5f);
			crc(&quantized,sizeof(quantized));
		}
	}

	return (unsigned int)crc;
}
