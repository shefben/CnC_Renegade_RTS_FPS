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

#include "action.h"
#include "baseselectionplanner.h"
#include "bridgeplanner.h"
#include "building.h"
#include "buildingconstruction.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "commanderguardai.h"
#include "commanderplacement.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "gameobjmanager.h"
#include "logisticsgraphservice.h"
#include "matrix3d.h"
#include "playerdata.h"
#include "playertype.h"
#include "purchaseavailability.h"
#include "purchasesettings.h"
#include "runtimebuildingfactory.h"
#include "scriptablegameobj.h"
#include "scriptcommands.h"
#include "soldier.h"
#include "strategicplanner.h"
#include "teampowergridservice.h"
#include "tiberiumfieldgenerator.h"
#include "timemgr.h"
#include "vector2.h"
#include "vehicle.h"
#include "worldboundsservice.h"
#include "worldshroudmanager.h"
#include "worldspatialindex.h"
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

//	Section 24.8's default cadence -- NORMAL difficulty's own `PlanningIntervalSeconds`
//	(`Get_Difficulty_Settings`).  Slower than a human could plausibly react at, which is the
//	point: an AI Commander that re-decides every frame is not "faster", it is a bug.  Exposed by
//	`Get_Planning_Interval_Seconds` as the difficulty-independent default the self check and any
//	caller with no per-team difficulty opinion can rely on.
static const float	AI_PLANNING_INTERVAL_SECONDS = 5.0f;

static CommanderAIDifficultyType		_Difficulty [AI_TEAM_COUNT];

static CommanderAIProductionMixType	_ProductionMixDecision [AI_TEAM_COUNT];

enum { MAX_DEFENSE_POINTS = 8 };
static CommanderAIDefensePoint	_DefensePoints [AI_TEAM_COUNT][MAX_DEFENSE_POINTS];
static int								_DefensePointCount [AI_TEAM_COUNT];

enum { MAX_DEFENDERS = 8, MAX_ATTACK_FORCE = 8 };
static int								_AssignedDefenderIds [AI_TEAM_COUNT][MAX_DEFENDERS];
static int								_AssignedDefenderCount [AI_TEAM_COUNT];

static CommanderAIAttackStageType	_AttackStage [AI_TEAM_COUNT];
static int								_AttackForceIds [AI_TEAM_COUNT][MAX_ATTACK_FORCE];
static int								_AttackForceCount [AI_TEAM_COUNT];
static int								_AttackForceSizeAtEngageStart [AI_TEAM_COUNT];
static int								_AttackTargetBuildingId [AI_TEAM_COUNT];
static Vector3							_AttackTargetPosition [AI_TEAM_COUNT];


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
		_Difficulty [i]					= AI_DIFFICULTY_NORMAL;
		_ProductionMixDecision [i]	= AI_PRODUCTION_NONE;
		_DefensePointCount [i]			= 0;
		_AssignedDefenderCount [i]	= 0;
		_AttackStage [i]					= AI_ATTACK_IDLE;
		_AttackForceCount [i]			= 0;
		_AttackForceSizeAtEngageStart [i]	= 0;
		_AttackTargetBuildingId [i]	= 0;
		_AttackTargetPosition [i]		= Vector3 (0.0f,0.0f,0.0f);
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
 * CommanderAIService::Decide_Production_Mix -- Section 24.5's pure priority function             *
 *                                                                                               *
 *	Vehicle whenever the enemy's own known vehicle count is ahead of this team's -- matching        *
 *	composition is Section 24.5's own explicit input, and it overrides the ratio below rather        *
 *	than competing with it.  Otherwise infantry until `InfantryPerVehicle` is met, then vehicle.       *
 *	NONE only once both conditions are satisfied -- genuinely nothing urgently needed, not a          *
 *	treasury or population-cap question, which is `Update`'s to ask once this has answered "what".    *
 *=============================================================================================*/
CommanderAIProductionMixType CommanderAIService::Decide_Production_Mix(
	int friendly_infantry_count,int friendly_vehicle_count,
	int known_enemy_vehicle_count,const CommanderAIProductionPolicy & policy)
{
	if (friendly_vehicle_count < known_enemy_vehicle_count) return AI_PRODUCTION_VEHICLE;

	int ratio = (policy.InfantryPerVehicle > 0) ? policy.InfantryPerVehicle : 1;
	int target_infantry = friendly_vehicle_count * ratio;
	if (friendly_infantry_count < target_infantry) return AI_PRODUCTION_INFANTRY;

	return AI_PRODUCTION_NONE;
}


/***********************************************************************************************
 * CommanderAIService::Score_Defense_Point -- Section 24.6's pure weighted score                  *
 *=============================================================================================*/
static float	Clamp01(float value)
{
	if (value < 0.0f) return 0.0f;
	if (value > 1.0f) return 1.0f;
	return value;
}

float CommanderAIService::Score_Defense_Point(
	const CommanderAIDefensePoint & point,const CommanderAIDefenseWeights & weights)
{
	return (weights.ChokepointWeight * Clamp01 (point.ChokepointImportance)) +
			 (weights.ValueWeight * Clamp01 (point.ValueProximity)) +
			 (weights.PowerWeight * Clamp01 (point.PowerVulnerability));
}


/***********************************************************************************************
 * CommanderAIService::Decide_Attack_Transition -- Section 24.7's pure stage machine               *
 *                                                                                               *
 *	See the header for the full stage list and what each real signal means.  EXPLOIT returning     *
 *	to ROUTE rather than a dedicated "pick the next target" stage is deliberate -- `Update`'s own    *
 *	real code re-runs the identical target-picking step ROUTE's own entry already does, and a        *
 *	real "nothing left to exploit" is folded into `force_spent` by that same caller rather than       *
 *	adding an eighth parameter this function would otherwise never use anywhere else.                 *
 *=============================================================================================*/
CommanderAIAttackStageType CommanderAIService::Decide_Attack_Transition(
	CommanderAIAttackStageType current,
	int known_enemy_buildings,int assembled_force_size,
	int force_losses,int force_size_at_engage_start,
	bool arrived,bool target_destroyed,bool force_spent,
	const CommanderAIAttackPolicy & policy)
{
	switch (current) {

		case AI_ATTACK_IDLE:
			return AI_ATTACK_RECON;

		case AI_ATTACK_RECON:
			if (known_enemy_buildings >= policy.ReconBuildingThreshold) return AI_ATTACK_ASSEMBLE;
			return AI_ATTACK_RECON;

		case AI_ATTACK_ASSEMBLE:
			if (assembled_force_size >= policy.MinForceSize) return AI_ATTACK_ROUTE;
			return AI_ATTACK_ASSEMBLE;

		case AI_ATTACK_ROUTE:
			if (force_spent) return AI_ATTACK_RETREAT;
			if (target_destroyed) return AI_ATTACK_EXPLOIT;
			if (arrived) return AI_ATTACK_ENGAGE;
			return AI_ATTACK_ROUTE;

		case AI_ATTACK_ENGAGE: {
			if (target_destroyed) return AI_ATTACK_EXPLOIT;
			if (force_spent) return AI_ATTACK_RETREAT;
			//	Integer-ceiling of RetreatLossFraction * force_size_at_engage_start without a
			//	float ceil() dependency: adding just under one whole unit before truncating.
			int threshold = (int)((policy.RetreatLossFraction * (float)force_size_at_engage_start) + 0.999f);
			if ((threshold > 0) && (force_losses >= threshold)) return AI_ATTACK_RETREAT;
			return AI_ATTACK_ENGAGE;
		}

		case AI_ATTACK_RETREAT:
			if (force_spent) return AI_ATTACK_IDLE;
			return AI_ATTACK_RETREAT;

		case AI_ATTACK_EXPLOIT:
			if (force_spent) return AI_ATTACK_IDLE;
			return AI_ATTACK_ROUTE;

		default:
			return AI_ATTACK_IDLE;
	}
}


void CommanderAIService::Set_Difficulty(int team,CommanderAIDifficultyType difficulty)
{
	_Difficulty [Team_Index (team)] = difficulty;
}

CommanderAIDifficultyType CommanderAIService::Get_Difficulty(int team)
{
	return _Difficulty [Team_Index (team)];
}

/***********************************************************************************************
 * CommanderAIService::Get_Difficulty_Settings -- Section 24.8, three real presets                *
 *                                                                                               *
 *	Easy plans slower, wants a bigger force before committing, and holds back a quarter of its     *
 *	treasury -- cautious, not stronger.  Hard plans faster and commits with a smaller force and      *
 *	no reserve -- aggressive, not omniscient: nothing here touches AvailableCredits income,           *
 *	KnownEnemy* counts, or any validator, which is what makes "no cheats" true by construction        *
 *	rather than by promise.                                                                            *
 *=============================================================================================*/
CommanderAIDifficultySettings CommanderAIService::Get_Difficulty_Settings(CommanderAIDifficultyType difficulty)
{
	CommanderAIDifficultySettings settings;

	switch (difficulty) {

		case AI_DIFFICULTY_EASY:
			settings.PlanningIntervalSeconds	= 10.0f;
			settings.MinForceSize				= 5;
			settings.ReserveFraction			= 0.25f;
			break;

		case AI_DIFFICULTY_HARD:
			settings.PlanningIntervalSeconds	= 2.5f;
			settings.MinForceSize				= 3;
			settings.ReserveFraction			= 0.0f;
			break;

		case AI_DIFFICULTY_NORMAL:
		default:
			settings.PlanningIntervalSeconds	= AI_PLANNING_INTERVAL_SECONDS;
			settings.MinForceSize				= 4;
			settings.ReserveFraction			= 0.1f;
			break;
	}

	return settings;
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

	/*
	**	Section 24.5's production-mix input, and Section 24.2's "known enemy composition" --
	**	both a coarse infantry-vs-vehicle proxy from the same real As_SoldierGameObj/
	**	As_VehicleGameObj split every other unit-kind test in this tree already uses, never a
	**	guessed unit taxonomy.  Friendly counts include every real unit regardless of who
	**	controls it (a human on this team is still this team's own strength, never a secret from
	**	itself); enemy counts are shroud-explored only, the identical Section 24.2 rule
	**	KnownEnemyBuildingCount already enforces.
	*/
	for (SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
		  node; node = node->Next ()) {

		SmartGameObj * smart = node->Data ();
		if ((smart == nullptr) || smart->Is_Delete_Pending ()) continue;

		bool is_infantry = (smart->As_SoldierGameObj () != nullptr);
		bool is_vehicle	= (smart->As_VehicleGameObj () != nullptr);
		if (!is_infantry && !is_vehicle) continue;

		if (smart->Get_Player_Type () == team) {
			if (is_infantry) out->FriendlyInfantryCount++;
			if (is_vehicle) out->FriendlyVehicleCount++;
		} else if (smart->Get_Player_Type () == enemy_team) {
			Vector3 unit_position;
			smart->Get_Position (&unit_position);
			if (!WorldShroudManager::Is_Explored (team,unit_position)) continue;
			if (is_infantry) out->KnownEnemyInfantryCount++;
			if (is_vehicle) out->KnownEnemyVehicleCount++;
		}
	}
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
 * Update_Economy -- Section 24.3/24.4's own building decision, extracted from Update             *
 *                                                                                               *
 *	Every early return here is a real, silent no-op for THIS planner only -- extracted into its    *
 *	own function (rather than left inline in Update, as it was before Phase 25) precisely so a      *
 *	refusal here (no descriptor loaded, no treasury, no site) cannot also skip the production/        *
 *	defense/attack planning Phase 25 adds below it; those are independent concerns and one              *
 *	planner's "nothing to do" must never look like every planner's "nothing to do".                     *
 *=============================================================================================*/
static void	Update_Economy(int team,const CommanderAIWorldSnapshot & snapshot)
{
	int index = Team_Index (team);

	CommanderAIEconomyPolicy policy;
	CommanderAIEconomyActionType decision = CommanderAIService::Decide_Economy_Priority (snapshot,policy);
	_LastDecision [index] = decision;

	if (decision == AI_ECONOMY_NONE) return;

	bool power_generating = (decision == AI_ECONOMY_BUILD_POWER_PLANT);
	const BuildingModelDescriptor * descriptor = CommanderAIService::Find_Buildable_Descriptor (power_generating,team);
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


/***********************************************************************************************
 * Find_Purchasable_Unit -- Section 24.5's own version of cCommanderProductionEvent's lookup       *
 *                                                                                               *
 *	The identical page/entry shape a human Commander's sidebar reads from -- `Find_Purchase_Entry`  *
 *	in `commanderproductionevent.cpp` is this function's own template, re-read directly rather        *
 *	than guessed at.  Picks the first available entry on the team's page for `kind` -- load order,     *
 *	same "first match wins" convention `Find_Buildable_Descriptor` already uses.  Air is never a        *
 *	`kind` this is called with; see this file's own header for why.                                     *
 *=============================================================================================*/
static bool	Find_Purchasable_Unit(int team,BuildingProductionKindType kind,int * out_definition_id,int * out_cost)
{
	PurchaseSettingsDefClass::TYPE type;
	switch (kind) {
		case BUILDING_PRODUCTION_INFANTRY:			type = PurchaseSettingsDefClass::TYPE_CLASSES;	break;
		case BUILDING_PRODUCTION_GROUND_VEHICLE:	type = PurchaseSettingsDefClass::TYPE_VEHICLES;	break;
		default:												return false;
	}

	PurchaseSettingsDefClass::TEAM page_team = (team == PLAYERTYPE_GDI)
															? PurchaseSettingsDefClass::TEAM_GDI
															: PurchaseSettingsDefClass::TEAM_NOD;

	PurchaseSettingsDefClass * page = PurchaseSettingsDefClass::Find_Definition (type,page_team);
	if (page == nullptr) return false;

	for (int i = 0; i < PurchaseAvailabilityClass::ENTRY_COUNT; i++) {
		if (!PurchaseAvailabilityClass::Is_Available (type,page_team,i)) continue;
		*out_definition_id	= page->Get_Definition (i);
		*out_cost				= page->Get_Cost (i);
		return true;
	}
	return false;
}


/***********************************************************************************************
 * Find_Team_Production_Facility -- the first real, ready facility of a kind this team owns        *
 *                                                                                               *
 *	"Ready" mirrors every check `cCommanderProductionEvent::Act` makes before calling               *
 *	`Request_Production` -- alive, construction complete, powered, exit clear -- read directly        *
 *	rather than re-derived, so a facility this function accepts is one a human's own click would        *
 *	accept too.                                                                                          *
 *=============================================================================================*/
static bool	Find_Team_Production_Facility(int team,BuildingProductionKindType kind,CommanderFacilityContext * out)
{
	for (SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
		  node; node = node->Next ()) {

		BuildingGameObj * building = node->Data ();
		if ((building == nullptr) || building->Is_Delete_Pending ()) continue;
		if (building->Get_Player_Type () != team) continue;

		const StringClass & descriptor_name = building->Get_Definition ().Get_Descriptor_Name ();
		if (descriptor_name.Get_Length () == 0) continue;
		const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name.Peek_Buffer ());
		if ((descriptor == nullptr) || (descriptor->Production.Kind != kind)) continue;

		if (building->Is_Destroyed ()) continue;
		if (BuildingConstructionService::Is_Under_Construction (building->Get_ID ())) continue;
		if (descriptor->Production.RequiresPower && !TeamPowerGridService::Is_Member_Powered (building->Get_ID ())) continue;

		Vector3 exit_point;
		float exit_radius = 0.0f;
		building->Get_Commander_Exit_Point (&exit_point,&exit_radius);
		GameObjQueryListClass occupants;
		WorldSpatialIndex::Query_Game_Objects_In_Sphere (exit_point,exit_radius,occupants,WorldSpatialIndex::QUERY_DYNAMIC);
		if (occupants.Count () != 0) continue;

		out->FacilityId				= building->Get_ID ();
		out->Team						= team;
		out->Alive						= true;
		out->ConstructionComplete	= true;
		out->Powered					= true;
		out->Kind						= descriptor->Production.Kind;
		out->QueueCapacity			= descriptor->Production.QueueCapacity;
		out->ExitClear					= true;
		return true;
	}
	return false;
}


/***********************************************************************************************
 * Update_Production -- Section 24.5's real production planner                                    *
 *=============================================================================================*/
static void	Update_Production(int team,const CommanderAIWorldSnapshot & snapshot)
{
	int index = Team_Index (team);

	CommanderAIProductionPolicy policy;
	CommanderAIProductionMixType mix = CommanderAIService::Decide_Production_Mix (
		snapshot.FriendlyInfantryCount,snapshot.FriendlyVehicleCount,snapshot.KnownEnemyVehicleCount,policy);
	_ProductionMixDecision [index] = mix;

	if (mix == AI_PRODUCTION_NONE) return;

	BuildingProductionKindType kind =
		(mix == AI_PRODUCTION_VEHICLE) ? BUILDING_PRODUCTION_GROUND_VEHICLE : BUILDING_PRODUCTION_INFANTRY;

	//	Section 24.5's own "population caps" input -- a real gate, not folded into the pure mix
	//	decision, the same split Update_Economy already uses for "can we afford it".
	if (CommanderProductionService::Get_Population (team,kind) >= CommanderProductionService::Get_Population_Cap (kind)) {
		return;
	}

	int definition_id = 0;
	int cost = 0;
	if (!Find_Purchasable_Unit (team,kind,&definition_id,&cost)) return;		// no purchase page, or nothing on offer

	CommanderAIDifficultySettings difficulty = CommanderAIService::Get_Difficulty_Settings (_Difficulty [index]);
	float spendable = _Treasury [index] - (_Treasury [index] * difficulty.ReserveFraction);
	if (spendable < (float)cost) return;

	CommanderFacilityContext facility;
	if (!Find_Team_Production_Facility (team,kind,&facility)) return;

	int ticket = 0;
	CommanderProductionResultType result = CommanderProductionService::Request_Production (
		AI_COMMANDER_ID,facility,definition_id,cost,0.0f,(int)_Treasury [index],true,&ticket);
	if (result != PRODUCTION_ACCEPTED) return;

	_Treasury [index] -= (float)cost;
	if (_Treasury [index] < 0.0f) _Treasury [index] = 0.0f;

	CommanderStateClass::Changed ();
}


/***********************************************************************************************
 * Refresh_Defense_Points -- Section 24.6's real chokepoint/high-value list                        *
 *                                                                                               *
 *	Chokepoints are `BridgePlanner::ResolvedBridge::StrategicImportance` -- Phase 9's own "no AI    *
 *	Commander consumer exists yet" field, closed here rather than left for a third phase to           *
 *	rediscover.  High-value points are this team's own Power Plants/Silos (already known from the      *
 *	snapshot's building walk) at their real position.  Power vulnerability is one real per-team          *
 *	number, not per-point, applied identically to every point this tick -- a short grid raises the         *
 *	priority of defending everything, not any one place more than another.                                 *
 *=============================================================================================*/
static void	Refresh_Defense_Points(int team,const CommanderAIWorldSnapshot & snapshot)
{
	int index = Team_Index (team);
	_DefensePointCount [index] = 0;

	float power_vulnerability = 0.0f;
	if (snapshot.PowerGeneration > 0.0f) {
		power_vulnerability = Clamp01 (snapshot.PowerAllocatedDemand / snapshot.PowerGeneration);
	} else if (snapshot.PowerAllocatedDemand > 0.0f) {
		power_vulnerability = 1.0f;
	}

	if (BridgePlanner::Has_Plan () && StrategicPlanner::Has_Plan ()) {
		int bridge_count = BridgePlanner::Get_Bridge_Count ();
		for (int i = 0; (i < bridge_count) && (_DefensePointCount [index] < MAX_DEFENSE_POINTS); i++) {

			ResolvedBridge bridge;
			if (!BridgePlanner::Get_Bridge (i,&bridge)) continue;
			if (!bridge.CountsAsConnectivity) continue;

			int node_index = -1;
			if (!StrategicPlanner::Find_Node_By_Id (bridge.CrossingNodeId,&node_index)) continue;
			StrategicNode node;
			if (!StrategicPlanner::Get_Node (node_index,&node)) continue;

			CommanderAIDefensePoint point;
			point.NodeId						= bridge.Id;
			point.Position						= node.Position;
			point.ChokepointImportance	= Clamp01 (bridge.StrategicImportance);
			point.ValueProximity				= 0.0f;
			point.PowerVulnerability		= power_vulnerability;

			_DefensePoints [index][_DefensePointCount [index]] = point;
			_DefensePointCount [index]++;
		}
	}

	for (SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
		  node && (_DefensePointCount [index] < MAX_DEFENSE_POINTS); node = node->Next ()) {

		BuildingGameObj * building = node->Data ();
		if ((building == nullptr) || building->Is_Delete_Pending ()) continue;
		if (building->Get_Player_Type () != team) continue;

		const StringClass & descriptor_name = building->Get_Definition ().Get_Descriptor_Name ();
		if (descriptor_name.Get_Length () == 0) continue;
		const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name.Peek_Buffer ());
		if (descriptor == nullptr) continue;
		if ((descriptor->Power.Generation <= 0.0f) && !descriptor->TiberiumSilo.Enabled) continue;

		Vector3 position;
		building->Get_Position (&position);

		CommanderAIDefensePoint point;
		point.NodeId						= descriptor_name;
		point.Position						= position;
		point.ChokepointImportance	= 0.0f;
		point.ValueProximity				= 1.0f;		// this team's own power/economy building, maximal value by definition
		point.PowerVulnerability		= power_vulnerability;

		_DefensePoints [index][_DefensePointCount [index]] = point;
		_DefensePointCount [index]++;
	}
}


//	True if `unit_id` is already tracked as a defender or an attack-force member for `team` --
//	the one thing that keeps the two planners from fighting over the same idle unit.
static bool	Is_Unit_Assigned(int team,int unit_id)
{
	int index = Team_Index (team);
	for (int i = 0; i < _AssignedDefenderCount [index]; i++) {
		if (_AssignedDefenderIds [index][i] == unit_id) return true;
	}
	for (int i = 0; i < _AttackForceCount [index]; i++) {
		if (_AttackForceIds [index][i] == unit_id) return true;
	}
	return false;
}

//	Drops any id in `ids`/`*count` that no longer names a live, still-on-team unit.  Shared by
//	both trackers rather than duplicated -- the one real roster check each needs.
static void	Prune_Unit_List(int team,int * ids,int * count)
{
	int kept = 0;
	for (int i = 0; i < *count; i++) {
		ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (ids [i]);
		SmartGameObj * smart = (scriptable != nullptr) ? scriptable->As_SmartGameObj () : nullptr;
		if ((smart == nullptr) || smart->Is_Delete_Pending () || (smart->Get_Player_Type () != team)) continue;
		ids [kept] = ids [i];
		kept++;
	}
	*count = kept;
}

//	Fills `out_ids` with up to `max_count` real, unassigned, non-human units on `team`.  Returns
//	how many were found.  Human-driven units are never claimed by this AI -- Section 3A.4's own
//	rule that a human's unit refuses Commander orders, enforced here by never issuing one in the
//	first place rather than relying on the order being refused server-side.
static int	Find_Available_Units(int team,int * out_ids,int max_count)
{
	int found = 0;
	for (SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
		  node && (found < max_count); node = node->Next ()) {

		SmartGameObj * smart = node->Data ();
		if ((smart == nullptr) || smart->Is_Delete_Pending ()) continue;
		if (smart->Get_Player_Type () != team) continue;
		if (smart->Is_Human_Controlled ()) continue;
		if ((smart->As_SoldierGameObj () == nullptr) && (smart->As_VehicleGameObj () == nullptr)) continue;
		if (Is_Unit_Assigned (team,smart->Get_ID ())) continue;

		out_ids [found] = smart->Get_ID ();
		found++;
	}
	return found;
}


/***********************************************************************************************
 * Dispatch_AI_Goto / Dispatch_AI_Attack -- this file's own order dispatch                         *
 *                                                                                               *
 *	See this file's own header for why these exist rather than sharing                              *
 *	`Code/Commando/commanderorderevent.cpp`'s `Dispatch_Order` (a Combat/Commando layering            *
 *	rule, not a style choice).  Same priority (200), same `Action_Reset` before every fresh order,     *
 *	same `CommanderGuardAI::Stop_Guarding` -- an attack-assigned unit that was guarding must drop        *
 *	that state exactly as a human's fresh order already forces for any unit.                             *
 *=============================================================================================*/
static const float	AI_ORDER_PRIORITY = 200.0f;

static SmartGameObj *	Find_Assigned_Unit(int unit_id)
{
	ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (unit_id);
	return (scriptable != nullptr) ? scriptable->As_SmartGameObj () : nullptr;
}

static void	Dispatch_AI_Goto(int unit_id,const Vector3 & position)
{
	SmartGameObj * unit = Find_Assigned_Unit (unit_id);
	if (unit == nullptr) return;

	CommanderGuardAI::Stop_Guarding (unit_id);
	ScriptEngine::Action_Reset (unit,AI_ORDER_PRIORITY);

	ActionParamsStruct params;
	params.Set_Basic (0,AI_ORDER_PRIORITY,0);
	params.Set_Movement (position,1.0f,3.0f,false);
	ScriptEngine::Action_Goto (unit,params);
}

static void	Dispatch_AI_Attack(int unit_id,int target_building_id,const Vector3 & target_position)
{
	SmartGameObj * unit = Find_Assigned_Unit (unit_id);
	if (unit == nullptr) return;

	CommanderGuardAI::Stop_Guarding (unit_id);
	ScriptEngine::Action_Reset (unit,AI_ORDER_PRIORITY);

	ActionParamsStruct params;
	params.Set_Basic (0,AI_ORDER_PRIORITY,0);

	ScriptableGameObj * target = (target_building_id != 0) ? GameObjManager::Find_ScriptableGameObj (target_building_id) : nullptr;
	if (target != nullptr) {
		params.Set_Attack (target,40.0f,0.0f,true);
	} else {
		params.Set_Attack (target_position,40.0f,0.0f,true);
	}
	ScriptEngine::Action_Attack (unit,params);
}


/***********************************************************************************************
 * Update_Defense -- Section 24.6's real defense planner                                          *
 *=============================================================================================*/
static void	Update_Defense(int team,const CommanderAIWorldSnapshot & snapshot)
{
	int index = Team_Index (team);

	Prune_Unit_List (team,_AssignedDefenderIds [index],&_AssignedDefenderCount [index]);
	Refresh_Defense_Points (team,snapshot);
	if (_DefensePointCount [index] == 0) return;

	CommanderAIDefenseWeights weights;
	int best_index = 0;
	float best_score = CommanderAIService::Score_Defense_Point (_DefensePoints [index][0],weights);
	for (int i = 1; i < _DefensePointCount [index]; i++) {
		float score = CommanderAIService::Score_Defense_Point (_DefensePoints [index][i],weights);
		if (score > best_score) {
			best_score = score;
			best_index = i;
		}
	}

	CommanderAIDifficultySettings difficulty = CommanderAIService::Get_Difficulty_Settings (_Difficulty [index]);
	int target_defenders = (difficulty.MinForceSize < MAX_DEFENDERS) ? difficulty.MinForceSize : MAX_DEFENDERS;
	int need = target_defenders - _AssignedDefenderCount [index];
	if (need <= 0) return;

	int new_ids [MAX_DEFENDERS];
	int found = Find_Available_Units (team,new_ids,need);
	static const float GUARD_RADIUS = 25.0f;
	for (int i = 0; i < found; i++) {
		CommanderGuardAI::Guard (new_ids [i],_DefensePoints [index][best_index].Position,GUARD_RADIUS);
		_AssignedDefenderIds [index][_AssignedDefenderCount [index]] = new_ids [i];
		_AssignedDefenderCount [index]++;
	}
}


/***********************************************************************************************
 * Pick_Attack_Target -- Section 24.7's "choose route" made concrete                               *
 *                                                                                               *
 *	Prefers the explored enemy building reachable by the shortest real road distance              *
 *	(`LogisticsGraphService`'s own node graph) over the merely nearest straight-line one -- "choose *
 *	route" without a waypoint-following primitive this codebase has never had (every AI-issued        *
 *	order in this tree, human or bot, relies on the engine's own navigation between click and           *
 *	destination -- see this file's own header).  Falls back to straight-line distance with no road       *
 *	graph built, which is still a real, not a fabricated, choice.                                          *
 *=============================================================================================*/
static bool	Pick_Attack_Target(int team,int * out_building_id,Vector3 * out_position)
{
	int enemy_team = (team == PLAYERTYPE_GDI) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI;
	bool has_graph = LogisticsGraphService::Has_Graph ();

	int best_id = 0;
	Vector3 best_position (0.0f,0.0f,0.0f);
	float best_distance = -1.0f;

	for (SLNode<BuildingGameObj> * node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
		  node; node = node->Next ()) {

		BuildingGameObj * building = node->Data ();
		if ((building == nullptr) || building->Is_Delete_Pending () || building->Is_Destroyed ()) continue;
		if (building->Get_Player_Type () != enemy_team) continue;

		Vector3 position;
		building->Get_Position (&position);
		if (!WorldShroudManager::Is_Explored (team,position)) continue;

		float distance;
		if (has_graph) {
			int nearest_road_index = -1;
			float nearest_road_distance = -1.0f;
			int road_count = LogisticsGraphService::Get_Node_Count ();
			for (int r = 0; r < road_count; r++) {
				LogisticsNode road_node;
				if (!LogisticsGraphService::Get_Node (r,&road_node)) continue;
				float road_distance = (road_node.Position - position).Length ();
				if ((nearest_road_distance < 0.0f) || (road_distance < nearest_road_distance)) {
					nearest_road_distance = road_distance;
					nearest_road_index = r;
				}
			}
			distance = (nearest_road_index >= 0) ? nearest_road_distance : (position - Vector3 (0.0f,0.0f,0.0f)).Length ();
		} else {
			distance = position.Length ();		// straight-line proxy -- no graph to measure a real road distance with
		}

		if ((best_distance < 0.0f) || (distance < best_distance)) {
			best_distance	= distance;
			best_id			= building->Get_ID ();
			best_position	= position;
		}
	}

	if (best_id == 0) return false;
	*out_building_id	= best_id;
	*out_position		= best_position;
	return true;
}


/***********************************************************************************************
 * Update_Attack -- Section 24.7's real staged attack planner                                     *
 *=============================================================================================*/
static void	Update_Attack(int team,const CommanderAIWorldSnapshot & snapshot)
{
	int index = Team_Index (team);

	Prune_Unit_List (team,_AttackForceIds [index],&_AttackForceCount [index]);

	bool target_destroyed = false;
	if (_AttackTargetBuildingId [index] != 0) {
		ScriptableGameObj * target = GameObjManager::Find_ScriptableGameObj (_AttackTargetBuildingId [index]);
		BuildingGameObj * target_building = (target != nullptr) ? target->As_BuildingGameObj () : nullptr;
		target_destroyed = (target_building == nullptr) || target_building->Is_Destroyed ();
	}

	bool arrived = false;
	static const float ARRIVAL_RADIUS = 40.0f;
	if ((_AttackStage [index] == AI_ATTACK_ROUTE) && (_AttackForceCount [index] > 0) && !target_destroyed) {
		Vector3 sum (0.0f,0.0f,0.0f);
		int counted = 0;
		for (int i = 0; i < _AttackForceCount [index]; i++) {
			SmartGameObj * unit = Find_Assigned_Unit (_AttackForceIds [index][i]);
			if (unit == nullptr) continue;
			Vector3 position;
			unit->Get_Position (&position);
			sum += position;
			counted++;
		}
		if (counted > 0) {
			Vector3 average = sum / (float)counted;
			arrived = ((average - _AttackTargetPosition [index]).Length () <= ARRIVAL_RADIUS);
		}
	}

	bool stage_needs_force = (_AttackStage [index] == AI_ATTACK_ROUTE) || (_AttackStage [index] == AI_ATTACK_ENGAGE)
									  || (_AttackStage [index] == AI_ATTACK_RETREAT) || (_AttackStage [index] == AI_ATTACK_EXPLOIT);
	bool force_spent = stage_needs_force && (_AttackForceCount [index] == 0);

	int losses = _AttackForceSizeAtEngageStart [index] - _AttackForceCount [index];
	if (losses < 0) losses = 0;

	CommanderAIAttackPolicy policy;
	CommanderAIDifficultySettings difficulty = CommanderAIService::Get_Difficulty_Settings (_Difficulty [index]);
	policy.MinForceSize = (difficulty.MinForceSize < MAX_ATTACK_FORCE) ? difficulty.MinForceSize : MAX_ATTACK_FORCE;

	CommanderAIAttackStageType previous_stage = _AttackStage [index];
	CommanderAIAttackStageType next_stage = CommanderAIService::Decide_Attack_Transition (
		previous_stage,snapshot.KnownEnemyBuildingCount,_AttackForceCount [index],
		losses,_AttackForceSizeAtEngageStart [index],arrived,target_destroyed,force_spent,policy);
	_AttackStage [index] = next_stage;
	bool stage_changed = (next_stage != previous_stage);

	switch (next_stage) {

		case AI_ATTACK_IDLE:
			_AttackForceCount [index]					= 0;
			_AttackTargetBuildingId [index]			= 0;
			_AttackForceSizeAtEngageStart [index]	= 0;
			break;

		case AI_ATTACK_RECON:
			break;		// waiting on the real KnownEnemyBuildingCount the snapshot already measured

		case AI_ATTACK_ASSEMBLE: {
			Vector3 rally;
			if (!Find_Team_Building_Centroid (team,&rally)) break;
			int need = policy.MinForceSize - _AttackForceCount [index];
			if (need <= 0) break;
			int new_ids [MAX_ATTACK_FORCE];
			int found = Find_Available_Units (team,new_ids,need);
			for (int i = 0; i < found; i++) {
				Dispatch_AI_Goto (new_ids [i],rally);
				_AttackForceIds [index][_AttackForceCount [index]] = new_ids [i];
				_AttackForceCount [index]++;
			}
			break;
		}

		case AI_ATTACK_ROUTE:
			if (stage_changed) {
				//	EXPLOIT lands here too (see Decide_Attack_Transition) -- a real "nothing
				//	left to press toward" is the one case Pick_Attack_Target itself can refuse,
				//	folded into force_spent for the very next tick's own transition rather than
				//	stalling this stage on a target that will never arrive.
				if (!Pick_Attack_Target (team,&_AttackTargetBuildingId [index],&_AttackTargetPosition [index])) {
					_AttackForceCount [index] = 0;
					break;
				}
				for (int i = 0; i < _AttackForceCount [index]; i++) {
					Dispatch_AI_Goto (_AttackForceIds [index][i],_AttackTargetPosition [index]);
				}
			}
			break;

		case AI_ATTACK_ENGAGE:
			if (stage_changed) {
				_AttackForceSizeAtEngageStart [index] = _AttackForceCount [index];
				for (int i = 0; i < _AttackForceCount [index]; i++) {
					Dispatch_AI_Attack (_AttackForceIds [index][i],_AttackTargetBuildingId [index],_AttackTargetPosition [index]);
				}
			}
			break;

		case AI_ATTACK_RETREAT:
			if (stage_changed) {
				Vector3 home;
				if (Find_Team_Building_Centroid (team,&home)) {
					for (int i = 0; i < _AttackForceCount [index]; i++) {
						Dispatch_AI_Goto (_AttackForceIds [index][i],home);
					}
				}
				_AttackForceCount [index] = 0;		// released back to the available pool -- home is the engine's problem now, not this tracker's
			}
			break;

		case AI_ATTACK_EXPLOIT:
			break;		// Update_Attack's own ROUTE case (next tick) picks the next target
	}
}


/***********************************************************************************************
 * CommanderAIService::Update -- the one real per-team planning tick                              *
 *                                                                                               *
 *	See the header for the full sequence and for why an accepted expansion candidate is not         *
 *	acted on yet.  Four independent planners -- economy, production, defense, attack -- each with   *
 *	their own real early-return "nothing to do" outcomes that never suppress one another; see        *
 *	`Update_Economy`'s own comment for why that split exists.                                         *
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
	float interval = Get_Difficulty_Settings (_Difficulty [index]).PlanningIntervalSeconds;
	if (_PlanningClock [index] < interval) return;
	_PlanningClock [index] = 0.0f;

	CommanderAIWorldSnapshot snapshot;
	Build_World_Snapshot (team,&snapshot);
	_LastSnapshot [index] = snapshot;

	//	Section 24.9 diagnostics -- refreshed every planning tick regardless of what any other
	//	planner decides, so a caller can inspect "where would we expand" independently of
	//	everything else this tick did.
	Refresh_Expansion_Candidates (team,snapshot);

	Update_Economy (team,snapshot);
	Update_Production (team,snapshot);
	Update_Defense (team,snapshot);
	Update_Attack (team,snapshot);
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


int CommanderAIService::Get_Production_Mix_Decision(int team)		{ return (int)_ProductionMixDecision [Team_Index (team)]; }

int CommanderAIService::Get_Defense_Point_Count(int team)			{ return _DefensePointCount [Team_Index (team)]; }

bool CommanderAIService::Get_Defense_Point(int team,int index,CommanderAIDefensePoint * out)
{
	int team_index = Team_Index (team);
	if ((index < 0) || (index >= _DefensePointCount [team_index])) return false;
	if (out == nullptr) return false;
	*out = _DefensePoints [team_index][index];
	return true;
}

int CommanderAIService::Get_Assigned_Defender_Count(int team)	{ return _AssignedDefenderCount [Team_Index (team)]; }

int CommanderAIService::Get_Attack_Stage(int team)					{ return (int)_AttackStage [Team_Index (team)]; }
int CommanderAIService::Get_Attack_Force_Size(int team)			{ return _AttackForceCount [Team_Index (team)]; }
Vector3 CommanderAIService::Get_Attack_Target_Position(int team)	{ return _AttackTargetPosition [Team_Index (team)]; }


const char * CommanderAIService::Get_Production_Mix_Name(CommanderAIProductionMixType mix)
{
	switch (mix) {
		case AI_PRODUCTION_NONE:			return "none";
		case AI_PRODUCTION_INFANTRY:	return "infantry";
		case AI_PRODUCTION_VEHICLE:		return "vehicle";
		default:									return "unknown";
	}
}

const char * CommanderAIService::Get_Attack_Stage_Name(CommanderAIAttackStageType stage)
{
	switch (stage) {
		case AI_ATTACK_IDLE:			return "idle";
		case AI_ATTACK_RECON:			return "recon";
		case AI_ATTACK_ASSEMBLE:	return "assemble";
		case AI_ATTACK_ROUTE:		return "route";
		case AI_ATTACK_ENGAGE:		return "engage";
		case AI_ATTACK_RETREAT:		return "retreat";
		case AI_ATTACK_EXPLOIT:		return "exploit";
		default:								return "unknown";
	}
}
