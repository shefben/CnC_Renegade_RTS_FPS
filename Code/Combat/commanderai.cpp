/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderai.cpp                       $*
 *                                                                                             *
 *	Roadmap Section 24.1-24.4.  See commanderai.h.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderai.h"

#include "baseselectionplanner.h"
#include "building.h"
#include "buildingconstruction.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "commanderplacement.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "gameobjmanager.h"
#include "logisticsgraphservice.h"
#include "matrix3d.h"
#include "playertype.h"
#include "runtimebuildingfactory.h"
#include "strategicplanner.h"
#include "teampowergridservice.h"
#include "tiberiumfieldgenerator.h"
#include "timemgr.h"
#include "vector2.h"
#include "worldboundsservice.h"
#include "worldshroudmanager.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <stdlib.h>


//	Two-team array, the same shape `CommanderProductionService`'s own `_Commanders[]` and every
//	Section 25.1/22 team-keyed array in this project already uses -- NOD at 0, GDI at 1.
static int	Team_Index(int team)
{
	return (team == PLAYERTYPE_GDI) ? 1 : 0;
}


enum { AI_TEAM_COUNT = 2 };

static bool									_Active [AI_TEAM_COUNT];
static float								_Treasury [AI_TEAM_COUNT];
static float								_PlanningClock [AI_TEAM_COUNT];
static CommanderAIEconomyActionType	_LastDecision [AI_TEAM_COUNT];
static CommanderAIWorldSnapshot		_LastSnapshot [AI_TEAM_COUNT];
static int									_AttemptedBuilds [AI_TEAM_COUNT];
static int									_SucceededBuilds [AI_TEAM_COUNT];

enum { MAX_EXPANSION_CANDIDATES = 16 };
static CommanderAIExpansionCandidate	_ExpansionCandidates [AI_TEAM_COUNT][MAX_EXPANSION_CANDIDATES];
static int									_ExpansionCandidateCount [AI_TEAM_COUNT];

//	Section 24.8's "planning interval" in miniature -- one shared cadence, not per-difficulty
//	tuning this phase does not build.  Slower than a human could plausibly react at, which is
//	the point: an AI Commander that re-decides every frame is not "faster", it is a bug.
static const float	AI_PLANNING_INTERVAL_SECONDS = 5.0f;


void CommanderAIService::Init(void)
{
	for (int i = 0; i < AI_TEAM_COUNT; i++) {
		_Active [i]				= false;
		_Treasury [i]			= 0.0f;
		_PlanningClock [i]	= 0.0f;
		_LastDecision [i]		= AI_ECONOMY_NONE;
		_LastSnapshot [i]		= CommanderAIWorldSnapshot ();
		_AttemptedBuilds [i]	= 0;
		_SucceededBuilds [i]	= 0;
		_ExpansionCandidateCount [i]	= 0;
	}
}

void CommanderAIService::Shutdown(void)
{
	Init ();
}


bool CommanderAIService::Activate(int team)
{
	int index = Team_Index (team);
	if (_Active [index]) return true;

	//	Never displaces a real Commander -- an AI seat only ever fills a genuinely vacant one.
	if (CommanderProductionService::Get_Team_Commander (team) != 0) return false;

	CommanderProductionService::Set_Team_Commander (team,AI_COMMANDER_ID);
	_Active [index]			= true;
	_PlanningClock [index]	= 0.0f;
	return true;
}

void CommanderAIService::Deactivate(int team)
{
	int index = Team_Index (team);
	if (!_Active [index]) return;

	//	Do not take back what this AI does not hold: a Section 25.1 REMOVE vote or a direct
	//	claim may already have cleared (or replaced) the seat by the time this is called.
	if (CommanderProductionService::Get_Team_Commander (team) == AI_COMMANDER_ID) {
		CommanderProductionService::Set_Team_Commander (team,0);
	}
	_Active [index] = false;
}

bool CommanderAIService::Is_Active(int team)
{
	return _Active [Team_Index (team)];
}


float CommanderAIService::Get_Treasury(int team)					{ return _Treasury [Team_Index (team)]; }
void  CommanderAIService::Set_Treasury(int team,float credits)	{ _Treasury [Team_Index (team)] = credits; }

void CommanderAIService::Increment_Treasury(int team,float delta)
{
	int index = Team_Index (team);
	_Treasury [index] += delta;
	if (_Treasury [index] < 0.0f) _Treasury [index] = 0.0f;
}


float CommanderAIService::Get_Planning_Interval_Seconds(void)	{ return AI_PLANNING_INTERVAL_SECONDS; }


/***********************************************************************************************
 * CommanderAIService::Decide_Economy_Priority -- what this team's economy needs most right now  *
 *                                                                                               *
 *	Fixed priority order: power first (every other capability this team owns can brown out          *
 *	without it), then wallet headroom.  A pure function of the snapshot and the policy -- no         *
 *	descriptor, no treasury balance; "can we afford it" is Update's question, once this has            *
 *	answered "what do we need".                                                                       *
 *=============================================================================================*/
CommanderAIEconomyActionType CommanderAIService::Decide_Economy_Priority(
	const CommanderAIWorldSnapshot & snapshot,const CommanderAIEconomyPolicy & policy)
{
	bool power_short = false;
	if (snapshot.PowerGeneration <= 0.0f) {
		power_short = (snapshot.PowerAllocatedDemand > 0.0f);
	} else {
		power_short = (snapshot.PowerAllocatedDemand > (policy.PowerHeadroomFraction * snapshot.PowerGeneration));
	}
	if (power_short) return AI_ECONOMY_BUILD_POWER_PLANT;

	if (snapshot.SiloCount < policy.TargetSiloCount) return AI_ECONOMY_BUILD_SILO;

	return AI_ECONOMY_NONE;
}


/***********************************************************************************************
 * CommanderAIService::Score_Expansion_Candidate -- Section 24.4's weighted site score            *
 *                                                                                               *
 *	Every term is normalised to 0..1 before weighting.  A distance of -1 ("none known") reads as   *
 *	the worst possible score for that term (0) rather than the best -- an unmeasured resource or    *
 *	road is not evidence of a good site, it is missing information, and missing information must     *
 *	never score better than a real, measured, merely-distant one.                                    *
 *=============================================================================================*/
static float	Normalize_Closeness(float distance,float reference_distance)
{
	if (distance < 0.0f) return 0.0f;
	if (reference_distance <= 0.0f) return 0.0f;
	float t = 1.0f - (distance / reference_distance);
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	return t;
}

float CommanderAIService::Score_Expansion_Candidate(
	const CommanderAIExpansionCandidate & candidate,const CommanderAIExpansionWeights & weights)
{
	float score = 0.0f;

	score += weights.ResourceWeight * Normalize_Closeness (candidate.ResourceDistance,weights.ResourceReferenceDistance);
	score += weights.RoadWeight * Normalize_Closeness (candidate.RoadAccessDistance,weights.RoadReferenceDistance);

	float safety = 1.0f - candidate.EnemyPressure;
	if (safety < 0.0f) safety = 0.0f;
	if (safety > 1.0f) safety = 1.0f;
	score += weights.SafetyWeight * safety;

	score += weights.PowerWeight * (candidate.PowerOk ? 1.0f : 0.0f);

	//	Closer to the boundary is worse, not better -- Directive 1.3's ring is a hazard, not an
	//	asset, so this term is the complement of the others' "closer is better" convention.
	float boundary_margin = Normalize_Closeness (
		weights.BoundaryReferenceDistance - candidate.DistanceFromBoundary,weights.BoundaryReferenceDistance);
	if (candidate.DistanceFromBoundary >= weights.BoundaryReferenceDistance) boundary_margin = 1.0f;
	score += weights.BoundaryWeight * boundary_margin;

	if (!candidate.BuildableAreaOk) return 0.0f;

	return score;
}


/***********************************************************************************************
 * CommanderAIService::Find_Buildable_Descriptor -- the one data-driven descriptor search         *
 *                                                                                               *
 *	Load order, first match, among descriptors that are (a) Buildable and (b) either name no       *
 *	faction (Build.Team == -1) or name this one.  A descriptor for the other real faction is       *
 *	never a candidate, regardless of load order -- the same rule                                    *
 *	CommanderPlacementValidator::Validate already enforces for a human's placement.                 *
 *=============================================================================================*/
const BuildingModelDescriptor * CommanderAIService::Find_Buildable_Descriptor(bool power_generating,int team)
{
	int count = BuildingDescriptorManager::Get_Descriptor_Count ();
	for (int i = 0; i < count; i++) {
		const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor_By_Index (i);
		if (descriptor == nullptr) continue;
		if (!descriptor->Build.Buildable) continue;
		if ((descriptor->Build.Team != -1) && (descriptor->Build.Team != team)) continue;

		if (power_generating) {
			if (descriptor->Power.Generation > 0.0f) return descriptor;
		} else {
			if (descriptor->TiberiumSilo.Enabled) return descriptor;
		}
	}
	return nullptr;
}


/***********************************************************************************************
 * Build_World_Snapshot -- the one real building/unit roster walk Section 24.2 needs              *
 *                                                                                               *
 *	One pass over every real building in the world, classifying each by its own resolved           *
 *	descriptor (BuildingGameObjDef::Get_Descriptor_Name -> BuildingDescriptorManager) rather        *
 *	than a model or preset name -- the same resolution `building.cpp`'s own subcomponent code       *
 *	uses, and it works for a map-authored building exactly as it does for a Commander-built one.    *
 *	A building whose descriptor does not resolve (no back-link, or an unloaded one) still counts    *
 *	toward DamagedBuildingCount -- health is a DefenseObject property of every DamageableGameObj,   *
 *	never gated on having a resolved descriptor -- but not toward PowerPlantCount/SiloCount.         *
 *=============================================================================================*/
static void	Build_World_Snapshot(int team,CommanderAIWorldSnapshot * out)
{
	*out = CommanderAIWorldSnapshot ();

	out->AvailableCredits		= CommanderAIService::Get_Treasury (team);
	out->PowerGeneration			= TeamPowerGridService::Get_Generation (team);
	out->PowerAllocatedDemand	= TeamPowerGridService::Get_Allocated_Demand (team);

	int enemy_team = (team == PLAYERTYPE_GDI) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;

	for (SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
		  node; node = node->Next ()) {

		BuildingGameObj * building = node->Data ();
		if ((building == nullptr) || building->Is_Delete_Pending ()) continue;

		if (building->Get_Player_Type () == team) {

			if (building->Get_Defense_Object () != nullptr) {
				float max_health = building->Get_Defense_Object ()->Get_Health_Max ();
				if ((max_health > 0.0f) && (building->Get_Defense_Object ()->Get_Health () < max_health)) {
					out->DamagedBuildingCount++;
				}
			}

			const BuildingModelDescriptor * descriptor =
				BuildingDescriptorManager::Get_Descriptor (building->Get_Definition ().Get_Descriptor_Name ().Peek_Buffer ());
			if (descriptor != nullptr) {
				if (descriptor->Power.Generation > 0.0f) out->PowerPlantCount++;
				if (descriptor->TiberiumSilo.Enabled) out->SiloCount++;
			}

		} else if (building->Get_Player_Type () == enemy_team) {

			//	Section 24.2: never the enemy's own existence, only what this team's shroud has
			//	actually explored -- the identical test a human Commander's own sidebar reads.
			Vector3 enemy_position;
			building->Get_Position (&enemy_position);
			if (WorldShroudManager::Is_Explored (team,enemy_position)) {
				out->KnownEnemyBuildingCount++;
			}
		}
	}

	out->FriendlyUnitCount =
		CommanderProductionService::Get_Population (team,BUILDING_PRODUCTION_INFANTRY) +
		CommanderProductionService::Get_Population (team,BUILDING_PRODUCTION_GROUND_VEHICLE) +
		CommanderProductionService::Get_Population (team,BUILDING_PRODUCTION_AIR_VEHICLE);
}


/***********************************************************************************************
 * Find_Team_Building_Centroid -- the anchor a new economy building is searched for a site around *
 *                                                                                               *
 *	The average position of this team's own real buildings -- not `BaseSelectionPlanner`'s          *
 *	selected site, because that requires a generated world and this AI must work on a legacy         *
 *	static map exactly as well (Directive 1.10).  Returns false with no buildings to anchor on,      *
 *	a real, disclosed limit: this planner cannot found a base from nothing (no MCV-deploy logic       *
 *	exists), only build outward from one that already stands.                                        *
 *=============================================================================================*/
static bool	Find_Team_Building_Centroid(int team,Vector3 * out_position)
{
	Vector3 sum (0.0f,0.0f,0.0f);
	int count = 0;

	for (SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
		  node; node = node->Next ()) {

		BuildingGameObj * building = node->Data ();
		if ((building == nullptr) || building->Is_Delete_Pending ()) continue;
		if (building->Get_Player_Type () != team) continue;

		Vector3 position;
		building->Get_Position (&position);
		sum += position;
		count++;
	}

	if (count == 0) return false;
	*out_position = sum / (float)count;
	return true;
}


/***********************************************************************************************
 * Find_Construction_Site -- a bounded ring search around an anchor, validated for real            *
 *                                                                                               *
 *	Concentric rings outward from the anchor, `RING_STEPS` points per ring, the validator's own     *
 *	real `Validate` (not `Validate_Server_Request` -- that call's bookkeeping belongs to an actual   *
 *	client request, not an internal search sweep) -- exactly the ghost-colour path a human's own     *
 *	cursor already uses, just swept automatically instead of by a mouse.  Ground height/rotation      *
 *	are whatever the validator's own result already resolved for that point.                          *
 *=============================================================================================*/
static bool	Find_Construction_Site(int team,const Vector3 & anchor,const char * descriptor_name,
												CommanderPlacementResult * out_result,Vector3 * out_position)
{
	enum { RING_COUNT = 4, RING_STEPS = 8 };
	static const float	RING_RADII [RING_COUNT] = { 600.0f,1000.0f,1500.0f,2200.0f };

	for (int ring = 0; ring < RING_COUNT; ring++) {
		for (int step = 0; step < RING_STEPS; step++) {

			float angle = (2.0f * WWMATH_PI * (float)step) / (float)RING_STEPS;
			Vector3 position = anchor;
			position.X += RING_RADII [ring] * WWMath::Cos (angle);
			position.Y += RING_RADII [ring] * WWMath::Sin (angle);

			CommanderPlacementRequest request;
			request.DescriptorName	= descriptor_name;
			request.Position			= position;
			request.RotationDegrees	= 0.0f;
			request.Team				= team;
			request.SequenceId		= 0;

			CommanderPlacementResult result = CommanderPlacementValidator::Validate (request);
			if (result.Is_Valid ()) {
				*out_result		= result;
				*out_position	= position;
				return true;
			}
		}
	}
	return false;
}


/***********************************************************************************************
 * Nearest_Explored_Enemy_Distance -- the one real input EnemyPressure is allowed to use           *
 *                                                                                               *
 *	Section 24.2 again: only an enemy building this team's own shroud has explored may inform a   *
 *	site's safety score.  -1 with none explored, or with no enemy buildings at all.                *
 *=============================================================================================*/
static float	Nearest_Explored_Enemy_Distance(int team,const Vector3 & position)
{
	int enemy_team = (team == PLAYERTYPE_GDI) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;
	float nearest = -1.0f;

	for (SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
		  node; node = node->Next ()) {

		BuildingGameObj * building = node->Data ();
		if ((building == nullptr) || building->Is_Delete_Pending ()) continue;
		if (building->Get_Player_Type () != enemy_team) continue;

		Vector3 enemy_position;
		building->Get_Position (&enemy_position);
		if (!WorldShroudManager::Is_Explored (team,enemy_position)) continue;

		float distance = (enemy_position - position).Length ();
		if ((nearest < 0.0f) || (distance < nearest)) nearest = distance;
	}

	return nearest;
}


/***********************************************************************************************
 * Refresh_Expansion_Candidates -- Section 24.4, the real candidate list behind the pure score    *
 *                                                                                               *
 *	Reuses Phase 7's own `StrategicPlanner` node graph rather than inventing a second one --      *
 *	see this file's own header for why.  A BASE_CANDIDATE already claimed as this team's own       *
 *	starting base (`BaseSelectionPlanner::Get_Selected_Site`) is excluded: it is not a candidate    *
 *	for expansion, it is the base already being expanded from.  `SETTLEMENT_CANDIDATE` nodes are    *
 *	included too -- `StrategicNode::Faction` is `PLAYERTYPE_NEUTRAL` for every non-base node, so     *
 *	they are open to either faction the same way an unclaimed settlement really would be.             *
 *=============================================================================================*/
static void	Refresh_Expansion_Candidates(int team,const CommanderAIWorldSnapshot & snapshot)
{
	int index = Team_Index (team);
	_ExpansionCandidateCount [index] = 0;

	if (!StrategicPlanner::Has_Plan ()) return;		// legacy/static map -- a real, empty result

	StringClass own_base_id ("");
	if (BaseSelectionPlanner::Has_Selection ()) {
		SelectedBaseSite site;
		if (BaseSelectionPlanner::Get_Selected_Site (team,&site)) own_base_id = site.Id;
	}

	bool power_ok = (snapshot.PowerGeneration - snapshot.PowerAllocatedDemand) > 0.0f;

	const BuildingModelDescriptor * representative = CommanderAIService::Find_Buildable_Descriptor (true,team);

	int node_count = StrategicPlanner::Get_Node_Count ();
	for (int i = 0; (i < node_count) && (_ExpansionCandidateCount [index] < MAX_EXPANSION_CANDIDATES); i++) {

		StrategicNode node;
		if (!StrategicPlanner::Get_Node (i,&node)) continue;

		bool is_base_candidate = (node.Type == STRATEGIC_NODE_BASE_CANDIDATE);
		bool is_settlement_candidate = (node.Type == STRATEGIC_NODE_SETTLEMENT_CANDIDATE);
		if (!is_base_candidate && !is_settlement_candidate) continue;

		if (is_base_candidate) {
			if (node.Faction != team) continue;				// the other faction's own base site
			if (node.Id == own_base_id) continue;			// already this team's own starting base
		}

		CommanderAIExpansionCandidate candidate;
		candidate.NodeId	= node.Id;
		candidate.Position	= node.Position;

		if (TiberiumFieldGenerator::Has_Fields ()) {
			float nearest = -1.0f;
			int field_count = TiberiumFieldGenerator::Get_Field_Count ();
			for (int f = 0; f < field_count; f++) {
				ResolvedTiberiumField field;
				if (!TiberiumFieldGenerator::Get_Field (f,&field)) continue;
				float distance = (field.Position - node.Position).Length ();
				if ((nearest < 0.0f) || (distance < nearest)) nearest = distance;
			}
			candidate.ResourceDistance = nearest;
		}

		if (LogisticsGraphService::Has_Graph ()) {
			float nearest = -1.0f;
			int road_count = LogisticsGraphService::Get_Node_Count ();
			for (int r = 0; r < road_count; r++) {
				LogisticsNode road_node;
				if (!LogisticsGraphService::Get_Node (r,&road_node)) continue;
				float distance = (road_node.Position - node.Position).Length ();
				if ((nearest < 0.0f) || (distance < nearest)) nearest = distance;
			}
			candidate.RoadAccessDistance = nearest;
		}

		float enemy_distance = Nearest_Explored_Enemy_Distance (team,node.Position);
		if (enemy_distance >= 0.0f) {
			//	Same closeness normalisation Score_Expansion_Candidate uses for its other
			//	terms, over a fixed 1000-unit "danger radius" -- close enough to be a real
			//	near-term threat, not merely the same continent.
			float t = 1.0f - (enemy_distance / 1000.0f);
			candidate.EnemyPressure = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
		}

		candidate.PowerOk = power_ok;

		if (representative != nullptr) {
			CommanderPlacementRequest request;
			request.DescriptorName	= representative->DefinitionName;
			request.Position			= node.Position;
			request.RotationDegrees	= 0.0f;
			request.Team				= team;
			request.SequenceId		= 0;
			candidate.BuildableAreaOk = CommanderPlacementValidator::Validate (request).Is_Valid ();
		}

		if (WorldBoundsService::Is_Configured ()) {
			Vector2 point (node.Position.X,node.Position.Y);
			candidate.DistanceFromBoundary = WorldBoundsService::Get_Inward_Distance (point);
		} else {
			candidate.DistanceFromBoundary = 999999.0f;		// unconfigured -- never a boundary hazard
		}

		_ExpansionCandidates [index][_ExpansionCandidateCount [index]] = candidate;
		_ExpansionCandidateCount [index]++;
	}
}


/***********************************************************************************************
 * CommanderAIService::Update -- the one real per-team planning tick                              *
 *                                                                                               *
 *	See the header for the full sequence and for why an accepted expansion candidate is not         *
 *	acted on yet.  Every early return here is a real, silent no-op for this tick -- nothing about   *
 *	"nothing to do right now" is an error.                                                           *
 *=============================================================================================*/
void CommanderAIService::Update(int team,float seconds)
{
	int index = Team_Index (team);
	if (!_Active [index]) return;

	//	The seat changed hands without going through Deactivate (a Section 25.1 REMOVE vote, or
	//	a direct claim against what this poll has not yet noticed is a contested seat) -- stand
	//	down rather than keep spending a treasury for a team this AI no longer commands.
	if (CommanderProductionService::Get_Team_Commander (team) != AI_COMMANDER_ID) {
		_Active [index] = false;
		return;
	}

	if (seconds > 0.0f) _PlanningClock [index] += seconds;
	if (_PlanningClock [index] < AI_PLANNING_INTERVAL_SECONDS) return;
	_PlanningClock [index] = 0.0f;

	CommanderAIWorldSnapshot snapshot;
	Build_World_Snapshot (team,&snapshot);
	_LastSnapshot [index] = snapshot;

	//	Section 24.9 diagnostics -- refreshed every planning tick regardless of what the economy
	//	planner decides below, so a caller can inspect "where would we expand" independently of
	//	"what are we building right now".
	Refresh_Expansion_Candidates (team,snapshot);

	CommanderAIEconomyPolicy policy;
	CommanderAIEconomyActionType decision = Decide_Economy_Priority (snapshot,policy);
	_LastDecision [index] = decision;

	if (decision == AI_ECONOMY_NONE) return;

	bool power_generating = (decision == AI_ECONOMY_BUILD_POWER_PLANT);
	const BuildingModelDescriptor * descriptor = Find_Buildable_Descriptor (power_generating,team);
	if (descriptor == nullptr) return;		// nothing loaded this team can build for this need

	if (_Treasury [index] < (float)descriptor->Build.Cost) {
		//	A real need with no descriptor problem, only an affordability one -- distinct from
		//	AI_ECONOMY_NONE for Section 24.9's diagnostics, even though Decide_Economy_Priority
		//	itself never produces this value (it has no treasury to judge affordability with).
		_LastDecision [index] = AI_ECONOMY_PAUSE;
		return;
	}

	Vector3 anchor;
	if (!Find_Team_Building_Centroid (team,&anchor)) return;		// no base to build outward from

	CommanderPlacementResult result;
	Vector3 site_position;
	_AttemptedBuilds [index]++;
	if (!Find_Construction_Site (team,anchor,descriptor->DefinitionName.Peek_Buffer (),&result,&site_position)) {
		return;
	}

	CommanderPlacementRequest request;
	request.DescriptorName	= descriptor->DefinitionName;
	request.Position			= site_position;
	request.RotationDegrees	= 0.0f;
	request.Team				= team;
	request.SequenceId		= 0;

	Matrix3D transform (1);
	CommanderPlacementValidator::Build_Placement_Transform (request,result,&transform);

	unsigned int seed = (unsigned int)((index + 1) * 2654435761u) ^
							  (unsigned int)(TimeManager::Get_Total_Seconds () * 1000.0f);

	BuildingGameObj * building =
		RuntimeBuildingFactory::Create_Constructing_Building (descriptor->DefinitionName.Peek_Buffer (),transform,team);
	if (building == nullptr) return;

	AABoxClass bounds;
	if (!RuntimeBuildingFactory::Get_Construction_Bounds (building,descriptor->DefinitionName.Peek_Buffer (),&bounds)) {
		bounds.Extent = Vector3 (descriptor->Footprint.Width * 0.5f,
										 descriptor->Footprint.Depth * 0.5f,
										 descriptor->Footprint.Width * 0.35f);
		bounds.Center = Vector3 (0.0f,0.0f,bounds.Extent.Z);
	}

	int building_id = building->Get_ID ();
	bool began = BuildingConstructionService::Begin_Construction (building_id,descriptor->DefinitionName.Peek_Buffer (),
																						transform,bounds,
																						TimeManager::Get_Total_Seconds (),seed);
	if (!began) return;

	_Treasury [index] -= (float)descriptor->Build.Cost;
	if (_Treasury [index] < 0.0f) _Treasury [index] = 0.0f;
	_SucceededBuilds [index]++;

	CommanderStateClass::Changed ();
}


int	CommanderAIService::Get_Last_Decision(int team)			{ return (int)_LastDecision [Team_Index (team)]; }
const CommanderAIWorldSnapshot & CommanderAIService::Get_Last_Snapshot(int team)
																				{ return _LastSnapshot [Team_Index (team)]; }
int	CommanderAIService::Get_Attempted_Build_Count(int team)	{ return _AttemptedBuilds [Team_Index (team)]; }
int	CommanderAIService::Get_Succeeded_Build_Count(int team)	{ return _SucceededBuilds [Team_Index (team)]; }


int CommanderAIService::Get_Expansion_Candidate_Count(int team)
{
	return _ExpansionCandidateCount [Team_Index (team)];
}

bool CommanderAIService::Get_Expansion_Candidate(int team,int index,CommanderAIExpansionCandidate * out)
{
	int team_index = Team_Index (team);
	if ((index < 0) || (index >= _ExpansionCandidateCount [team_index])) return false;
	if (out == nullptr) return false;
	*out = _ExpansionCandidates [team_index][index];
	return true;
}


const char * CommanderAIService::Get_Economy_Action_Name(CommanderAIEconomyActionType action)
{
	switch (action) {
		case AI_ECONOMY_NONE:					return "none";
		case AI_ECONOMY_BUILD_POWER_PLANT:	return "build power plant";
		case AI_ECONOMY_BUILD_SILO:			return "build silo";
		case AI_ECONOMY_PAUSE:					return "pause (cannot afford)";
		default:										return "unknown";
	}
}
