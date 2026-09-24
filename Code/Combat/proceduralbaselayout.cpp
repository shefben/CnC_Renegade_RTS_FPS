/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/proceduralbaselayout.cpp              $*
 *                                                                                             *
 *	See proceduralbaselayout.h.                                                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralbaselayout.h"

#include "baseselectionplanner.h"
#include "building.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "commanderplacement.h"
#include "gameobjmanager.h"
#include "generatedworldplacementrules.h"
#include "humanphys.h"
#include "playertype.h"
#include "proceduralworldprofile.h"
#include "runtimebuildingfactory.h"
#include "soldier.h"
#include "worldboundsservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>


namespace
{
	//	Ground left between two starting buildings, beyond their footprint discs.  Enough for an
	//	infantry lane between them; a vehicle lane is the production exit's own clearance.
	const float		BUILDING_GAP			= 4.0f;

	//	The rings candidate spots are taken from, in world units.
	const float		PLACEMENT_RING_STEP	= 6.0f;
	const float		SPAWN_RING_STEP		= 5.0f;

	//	Spawn point tests: a standing player's disc, the steepest ground a player is put on, and
	//	the gap kept from any building footprint or production exit.
	const float		SPAWN_POINT_RADIUS	= 1.0f;
	const float		SPAWN_MAX_SLOPE		= DEG_TO_RADF (30.0f);
	const float		SPAWN_BUILDING_GAP	= 3.0f;

	//	The used protected radius when base selection reserved none.
	const float		FALLBACK_BASE_RADIUS	= 150.0f;

	struct FactionState
	{
		bool														HasBase;
		Vector3													Center;
		Vector3													TowardEnemy;
		float														Radius;
		DynamicVectorClass<ProceduralBaseBuildingRecord>	Buildings;
		DynamicVectorClass<ProceduralSpawnCandidate>		Spawns;
		Vector3													ZoneCenter;
		float														ZoneRadius;
	};

	FactionState	_State [ProceduralBaseLayoutService::FACTION_COUNT];

	bool	Faction_Index(int faction,int * index_out)
	{
		if ((faction != PLAYERTYPE_NOD) && (faction != PLAYERTYPE_GDI)) return false;
		*index_out = faction;
		return true;
	}

	float	Ground_Height(float x,float y,float fallback)
	{
		float height = fallback;
		if (WorldTerrainSystem::Has_Terrain () && WorldTerrainSystem::Sample_Height (x,y,&height)) {
			return height;
		}
		return fallback;
	}

	float	Horizontal_Distance(const Vector3 & a,const Vector3 & b)
	{
		float dx = a.X - b.X;
		float dy = a.Y - b.Y;
		return WWMath::Sqrt ((dx * dx) + (dy * dy));
	}

	/*
	**	Read the selected site for a faction into its state: the centre on the ground, the
	**	horizontal direction to the other faction's site, and the protected radius.
	*/
	bool	Load_Site(int faction,FactionState * state)
	{
		SelectedBaseSite site;
		SelectedBaseSite enemy;
		int enemy_faction = (faction == PLAYERTYPE_GDI) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;

		if (!BaseSelectionPlanner::Get_Selected_Site (faction,&site)) return false;

		state->Center		= site.Position;
		state->Center.Z	= Ground_Height (site.Position.X,site.Position.Y,site.Position.Z);
		state->Radius		= (site.ProtectedRadius > 0.0f) ? site.ProtectedRadius : FALLBACK_BASE_RADIUS;

		state->TowardEnemy.Set (1.0f,0.0f,0.0f);
		if (BaseSelectionPlanner::Get_Selected_Site (enemy_faction,&enemy)) {
			Vector3 toward = enemy.Position - site.Position;
			toward.Z = 0.0f;
			if (toward.Length2 () > 0.0001f) {
				toward.Normalize ();
				state->TowardEnemy = toward;
			}
		}
		return true;
	}

	float	Facing_Degrees(const Vector3 & toward)
	{
		return RAD_TO_DEGF (WWMath::Atan2 (toward.Y,toward.X));
	}

	/*
	**	A discs-and-exits view of the buildings a faction has standing now.  A runtime building
	**	answers with its descriptor footprint and production exit; a map-authored one with a disc
	**	around its collection sphere, which is the only extent it carries without its components.
	*/
	struct Obstacle
	{
		bool		operator == (const Obstacle &) const	{ return false; }
		bool		operator != (const Obstacle &) const	{ return true; }
		Vector3	Center;
		float		Radius;
	};

	void	Collect_Obstacles(int faction,DynamicVectorClass<Obstacle> * out)
	{
		SList<BuildingGameObj> * list = GameObjManager::Get_Building_Game_Obj_List ();
		if (list == nullptr) return;

		for (SLNode<BuildingGameObj> * node = list->Head (); node != nullptr; node = node->Next ()) {
			BuildingGameObj * building = node->Data ();
			if ((building == nullptr) || (building->Get_Player_Type () != faction)) continue;

			Vector3 position;
			building->Get_Position (&position);

			Obstacle disc;
			disc.Center = position;
			disc.Radius = 10.0f;

			if (building->Is_Runtime_Constructed ()) {
				const char * name = building->Get_Runtime_Descriptor_Name ().Peek_Buffer ();
				CommanderPlacementValidator::Get_Footprint_Radius (name,&disc.Radius);

				const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (name);
				if ((descriptor != nullptr) && (descriptor->Placement.ExitClearance > 0.0f)) {
					Obstacle exit;
					Vector3 forward = building->Get_Runtime_Transform ().Get_Y_Vector ();
					forward.Z = 0.0f;
					if (forward.Length2 () > 0.0001f) forward.Normalize ();
					float reach = (descriptor->Footprint.Depth * 0.5f) + (descriptor->Placement.ExitClearance * 0.5f);
					exit.Center = position + (forward * reach);
					exit.Radius = WWMath::Max (descriptor->Footprint.Width * 0.5f,descriptor->Placement.ExitClearance * 0.5f);
					out->Add (exit);
				}
			}

			out->Add (disc);
		}
	}

	bool	Is_Spawn_Point_Usable(const Vector3 & point,const DynamicVectorClass<Obstacle> & obstacles,float * ground_out)
	{
		if (!WorldBoundsService::Can_Place_In_Region (Vector2 (point.X,point.Y),SPAWN_POINT_RADIUS)) return false;

		float ground = point.Z;
		if (WorldTerrainSystem::Has_Terrain ()) {
			TerrainPlacementInfoClass surface;
			if (!WorldTerrainSystem::Query_Placement_Surface (point,SPAWN_POINT_RADIUS,&surface)) return false;
			if (!surface.IsOnTerrain) return false;
			if (surface.MaxSlope > SPAWN_MAX_SLOPE) return false;
			ground = surface.AverageHeight;
		}

		//	Water, Tiberium routes, bridges, tunnel portals, cliffs and chokepoints: the generated
		//	world's own exclusion owner, the same one building placement asks.
		if (GeneratedWorldPlacementRules::Classify (point,SPAWN_POINT_RADIUS) != GENERATED_WORLD_EXCLUSION_NONE) {
			return false;
		}

		for (int i = 0; i < obstacles.Count (); i++) {
			if (Horizontal_Distance (point,obstacles [i].Center) < (obstacles [i].Radius + SPAWN_BUILDING_GAP + SPAWN_POINT_RADIUS)) {
				return false;
			}
		}

		*ground_out = ground;
		return true;
	}
}


/***********************************************************************************************
 * Candidate positions                                                                         *
 *=============================================================================================*/

int ProceduralBaseLayoutService::Get_Candidate_Positions(const Vector3 & center,const Vector3 & toward_enemy,
																			 float ring_step,float max_radius,
																			 DynamicVectorClass<Vector3> * out)
{
	if ((out == nullptr) || (ring_step <= 0.0f)) return 0;

	//	Behind the base first: the side away from the enemy is the side a starting building or a
	//	spawning player is least exposed on (16A.5), and the order is what makes the layout the
	//	same on every run with the same site.
	float behind = WWMath::Atan2 (-toward_enemy.Y,-toward_enemy.X);
	int added = 0;

	for (int ring = 1; (ring * ring_step) <= max_radius; ring++) {

		float radius = ring * ring_step;
		int count = (int)((2.0f * WWMATH_PI * radius) / ring_step);
		if (count < 8) count = 8;
		float step = (2.0f * WWMATH_PI) / (float)count;

		//	Alternate either side of "behind": 0, +1, -1, +2, -2 ...
		for (int k = 0; k < count; k++) {
			int side = (k + 1) / 2;
			float angle = behind + (((k & 1) != 0) ? (side * step) : -(side * step));
			out->Add (Vector3 (center.X + radius * WWMath::Cos (angle),
									 center.Y + radius * WWMath::Sin (angle),
									 center.Z));
			added++;
		}
	}

	return added;
}


/***********************************************************************************************
 * Layout                                                                                      *
 *=============================================================================================*/

void ProceduralBaseLayoutService::Reset(void)
{
	for (int i = 0; i < FACTION_COUNT; i++) {
		_State [i].HasBase = false;
		_State [i].Center.Set (0.0f,0.0f,0.0f);
		_State [i].TowardEnemy.Set (1.0f,0.0f,0.0f);
		_State [i].Radius = 0.0f;
		_State [i].Buildings.Delete_All ();
		_State [i].Spawns.Delete_All ();
		_State [i].ZoneCenter.Set (0.0f,0.0f,0.0f);
		_State [i].ZoneRadius = 0.0f;
	}
}


bool ProceduralBaseLayoutService::Plan_And_Place(const ProceduralWorldProfile & profile,StringClass * report_out)
{
	Reset ();

	if (!BaseSelectionPlanner::Has_Selection ()) {
		if (report_out != nullptr) *report_out = "no selected base sites; the level's own base and spawners stand";
		return false;
	}

	StringClass report;

	for (int faction = 0; faction < FACTION_COUNT; faction++) {

		FactionState & state = _State [faction];
		if (!Load_Site (faction,&state)) {
			continue;
		}
		state.HasBase = true;

		const DynamicVectorClass<StringClass> & wanted =
			(faction == PLAYERTYPE_GDI) ? profile.Base.GdiInitialBuildings : profile.Base.NodInitialBuildings;

		//	Local +Y (the production exit's side) toward the enemy.
		float rotation = Facing_Degrees (state.TowardEnemy) - 90.0f;

		for (int index = 0; index < wanted.Count (); index++) {

			ProceduralBaseBuildingRecord record;
			record.DescriptorName	= wanted [index];
			record.RotationDegrees	= rotation;

			//	A descriptor that says where its vehicle exit faces is turned so that exit, not the
			//	+Y convention, opens toward the enemy (16A.3: orient by semantic metadata, not by
			//	the bounding box).
			const BuildingModelDescriptor * descriptor =
				BuildingDescriptorManager::Get_Descriptor (record.DescriptorName.Peek_Buffer ());
			BuildingSemanticAnchor exit_anchor;
			if ((descriptor != nullptr) && descriptor->Find_Semantic_Anchor ("vehicle_exit",&exit_anchor)) {
				record.RotationDegrees = Facing_Degrees (state.TowardEnemy) - exit_anchor.LocalYawDegrees;
			}
			record.Refusal				= PLACEMENT_NO_DESCRIPTOR;

			if (!CommanderPlacementValidator::Get_Footprint_Radius (record.DescriptorName.Peek_Buffer (),&record.Radius)) {
				state.Buildings.Add (record);
				continue;
			}

			DynamicVectorClass<Vector3> candidates;
			if (index == 0) {
				candidates.Add (state.Center);		// the anchor stands on the site itself
			}
			Get_Candidate_Positions (state.Center,state.TowardEnemy,PLACEMENT_RING_STEP,
											 state.Radius - record.Radius,&candidates);

			for (int c = 0; c < candidates.Count (); c++) {

				Vector3 spot = candidates [c];
				spot.Z = Ground_Height (spot.X,spot.Y,state.Center.Z);

				bool clear = true;
				for (int placed = 0; placed < state.Buildings.Count (); placed++) {
					const ProceduralBaseBuildingRecord & other = state.Buildings [placed];
					if (other.BuildingId == 0) continue;
					if (Horizontal_Distance (spot,other.Position) < (record.Radius + other.Radius + BUILDING_GAP)) {
						clear = false;
						break;
					}
				}
				if (!clear) continue;

				CommanderPlacementRequest request;
				request.DescriptorName	= record.DescriptorName;
				request.Position			= spot;
				request.RotationDegrees	= record.RotationDegrees;
				request.Team				= faction;
				request.InitialBase		= true;

				CommanderPlacementResult result = CommanderPlacementValidator::Validate (request);
				if (!result.Is_Valid ()) {
					record.Refusal = result.Result;
					continue;
				}

				Matrix3D transform (1);
				CommanderPlacementValidator::Build_Placement_Transform (request,result,&transform);

				BuildingGameObj * building =
					RuntimeBuildingFactory::Create_Constructing_Building (record.DescriptorName.Peek_Buffer (),transform,faction);
				if (building == nullptr) {
					record.Refusal = PLACEMENT_NO_DESCRIPTOR;
					break;
				}

				//	A starting building is finished, not under construction: reveal it and open its
				//	capabilities now, which also puts it on its team's power grid.
				RuntimeBuildingFactory::Activate_Completed_Building (building->Get_ID ());

				record.Position	= transform.Get_Translation ();
				record.BuildingId	= building->Get_ID ();
				record.Refusal		= PLACEMENT_VALID;
				break;
			}

			if (record.BuildingId == 0) {
				StringClass line;
				line.Format ("%s: %s found no legal spot (last refusal: %s)\n",
								 (faction == PLAYERTYPE_GDI) ? "GDI" : "Nod",record.DescriptorName.Peek_Buffer (),
								 CommanderPlacementValidator::Get_Result_Name ((CommanderPlacementResultType)record.Refusal));
				report += line;
				WWDEBUG_SAY (("ProceduralBaseLayoutService: %s",line.Peek_Buffer ()));
			}

			state.Buildings.Add (record);
		}

		Generate_Base_Spawn_Set (faction,profile);
	}

	if (report_out != nullptr) *report_out = report;
	return _State [PLAYERTYPE_GDI].HasBase || _State [PLAYERTYPE_NOD].HasBase;
}


/***********************************************************************************************
 * Spawn pool                                                                                  *
 *=============================================================================================*/

int ProceduralBaseLayoutService::Generate_Base_Spawn_Set(int faction,const ProceduralWorldProfile & profile)
{
	int index = 0;
	if (!Faction_Index (faction,&index)) return 0;

	FactionState & state = _State [index];
	state.Spawns.Delete_All ();

	if (!state.HasBase) {
		if (!Load_Site (faction,&state)) return 0;
		state.HasBase = true;
	}

	DynamicVectorClass<Obstacle> obstacles;
	Collect_Obstacles (faction,&obstacles);

	DynamicVectorClass<Vector3> candidates;
	Get_Candidate_Positions (state.Center,state.TowardEnemy,SPAWN_RING_STEP,state.Radius,&candidates);

	float facing = Facing_Degrees (state.TowardEnemy);
	Vector3 sum (0.0f,0.0f,0.0f);

	for (int c = 0; (c < candidates.Count ()) && (state.Spawns.Count () < profile.Base.SpawnCandidateCount); c++) {

		Vector3 point = candidates [c];
		point.Z = Ground_Height (point.X,point.Y,state.Center.Z);

		float ground = point.Z;
		if (!Is_Spawn_Point_Usable (point,obstacles,&ground)) continue;

		ProceduralSpawnCandidate spawn;
		spawn.Position.Set (point.X,point.Y,ground);
		spawn.FacingDegrees = facing;
		state.Spawns.Add (spawn);
		sum += spawn.Position;
	}

	//	Section 16A.5's fallback zone, which exists whatever became of the pool: the pool's middle,
	//	or the site itself when not one point passed.
	state.ZoneRadius = profile.Base.SpawnZoneRadius;
	state.ZoneCenter = (state.Spawns.Count () > 0) ? (sum / (float)state.Spawns.Count ()) : state.Center;

	return state.Spawns.Count ();
}


bool ProceduralBaseLayoutService::Choose_Respawn_Point(int faction,SoldierGameObj * soldier,Matrix3D * out_transform)
{
	int index = 0;
	if ((out_transform == nullptr) || !Faction_Index (faction,&index)) return false;

	const FactionState & state = _State [index];
	if (!state.HasBase) return false;

	HumanPhysClass * phys = (soldier != nullptr) ? soldier->Peek_Human_Phys () : nullptr;

	//	Living enemy positions, once.
	DynamicVectorClass<Vector3> enemies;
	SList<SmartGameObj> * smart = GameObjManager::Get_Smart_Game_Obj_List ();
	if (smart != nullptr) {
		for (SLNode<SmartGameObj> * node = smart->Head (); node != nullptr; node = node->Next ()) {
			SmartGameObj * obj = node->Data ();
			if ((obj == nullptr) || (obj->As_SoldierGameObj () == nullptr)) continue;
			int team = obj->Get_Player_Type ();
			if ((team != PLAYERTYPE_NOD) && (team != PLAYERTYPE_GDI)) continue;
			if (team == faction) continue;
			Vector3 position;
			obj->Get_Position (&position);
			enemies.Add (position);
		}
	}

	int best = -1;
	float best_score = -1.0f;

	for (int i = 0; i < state.Spawns.Count (); i++) {

		const ProceduralSpawnCandidate & spawn = state.Spawns [i];

		Matrix3D tm (1);
		tm.Rotate_Z (DEG_TO_RADF (spawn.FacingDegrees));
		tm.Set_Translation (spawn.Position);

		if (WorldBoundsService::Is_Configured () && !WorldBoundsService::Contains_Gameplay_Point (spawn.Position)) continue;
		if ((phys != nullptr) && !phys->Can_Teleport (tm,true)) continue;

		float nearest = 100000.0f;
		for (int e = 0; e < enemies.Count (); e++) {
			float distance = Horizontal_Distance (spawn.Position,enemies [e]);
			if (distance < nearest) nearest = distance;
		}

		//	Strictly greater, so a tie goes to the earlier (more sheltered) point.
		if (nearest > best_score) {
			best_score = nearest;
			best = i;
		}
	}

	Vector3 position = state.ZoneCenter;
	float facing = Facing_Degrees (state.TowardEnemy);
	if (best >= 0) {
		position	= state.Spawns [best].Position;
		facing	= state.Spawns [best].FacingDegrees;
	} else if (state.Spawns.Count () > 0) {
		position = state.Spawns [0].Position;		// nowhere clear: the most sheltered point
	}

	Matrix3D tm (1);
	tm.Rotate_Z (DEG_TO_RADF (facing));
	tm.Set_Translation (position);
	*out_transform = tm;
	return true;
}


/***********************************************************************************************
 * Queries                                                                                     *
 *=============================================================================================*/

bool ProceduralBaseLayoutService::Has_Base(int faction)
{
	int index = 0;
	return Faction_Index (faction,&index) && _State [index].HasBase;
}

int ProceduralBaseLayoutService::Get_Building_Count(int faction)
{
	int index = 0;
	return Faction_Index (faction,&index) ? _State [index].Buildings.Count () : 0;
}

bool ProceduralBaseLayoutService::Get_Building(int faction,int i,ProceduralBaseBuildingRecord * out)
{
	int index = 0;
	if ((out == nullptr) || !Faction_Index (faction,&index)) return false;
	if ((i < 0) || (i >= _State [index].Buildings.Count ())) return false;
	*out = _State [index].Buildings [i];
	return true;
}

int ProceduralBaseLayoutService::Get_Spawn_Candidate_Count(int faction)
{
	int index = 0;
	return Faction_Index (faction,&index) ? _State [index].Spawns.Count () : 0;
}

bool ProceduralBaseLayoutService::Get_Spawn_Candidate(int faction,int i,ProceduralSpawnCandidate * out)
{
	int index = 0;
	if ((out == nullptr) || !Faction_Index (faction,&index)) return false;
	if ((i < 0) || (i >= _State [index].Spawns.Count ())) return false;
	*out = _State [index].Spawns [i];
	return true;
}

bool ProceduralBaseLayoutService::Get_Spawn_Zone(int faction,Vector3 * out_center,float * out_radius)
{
	int index = 0;
	if (!Faction_Index (faction,&index) || !_State [index].HasBase) return false;
	if (out_center != nullptr) *out_center = _State [index].ZoneCenter;
	if (out_radius != nullptr) *out_radius = _State [index].ZoneRadius;
	return true;
}
