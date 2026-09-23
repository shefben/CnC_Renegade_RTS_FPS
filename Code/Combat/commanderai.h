/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderai.h                         $*
 *                                                                                             *
 *	Roadmap Section 24.1-24.4 -- Phase 24 autonomous AI Commander: roles, strategic world       *
 *	model, economy planner, base expansion.                                                     *
 *                                                                                             *
 *	"The v1 human Commander API is the only action surface the AI Commander may use.  No AI     *
 *	cheats that directly spawn buildings/units or bypass power/credits."  An AI-held seat is a   *
 *	real entry in `CommanderProductionService::_Commanders[]` (`AI_COMMANDER_ID`, a reserved      *
 *	negative id -- every real client id is `cNetwork::Get_My_Id()`, always positive), and every    *
 *	building this file creates goes through the exact same `CommanderPlacementValidator::Validate`  *
 *	and `RuntimeBuildingFactory`/`BuildingConstructionService` pipeline `cCommanderPlacementEvent`   *
 *	uses for a human -- see `Update`'s own comment for the one place they necessarily diverge         *
 *	(there is no `SoldierGameObj`/`PlayerDataClass` to charge, so an AI seat spends from its own       *
 *	treasury instead -- see `AITreasury` below for why that treasury starts at, and stays at,           *
 *	zero until a real income path exists).                                                              *
 *                                                                                             *
 *	An AI seat is not exempt from Section 25.1's own governance: `Call_Commander_Vote`'s ELECT       *
 *	path only opens against a *vacant* seat (`current == 0`), so an AI-held seat (a real nonzero        *
 *	id) is challenged by `REMOVE`, exactly like a human's -- a team is never permanently stuck with       *
 *	a bot Commander it cannot vote out.  The one place an AI seat needed its own carve-out is           *
 *	`CommanderProductionService::Update_Commander_Vacancy` (see the fix there): that poll assumes         *
 *	every seat is held by a real connected soldier and would otherwise vacate an AI's own seat for          *
 *	"no soldier found" within `COMMANDER_VACANCY_GRACE_SECONDS` of activating it.                          *
 *                                                                                             *
 *	Section 24.2's "does not inspect hidden enemy truth" is provable the same way Section 24.1's     *
 *	treasury rule is: `Build_World_Snapshot` reads `WorldShroudManager::Is_Explored` for every         *
 *	enemy building it counts, never the building's own existence directly -- an unexplored enemy        *
 *	structure is invisible to this file exactly as it would be to a human Commander's own sidebar.       *
 *                                                                                             *
 *	Every decision function below is a pure function of a caller-supplied snapshot/context and a     *
 *	tunable policy, so the actual choice logic is checkable with plain structs and no `GameObj`,        *
 *	no scene, no descriptor -- the same split `CommanderProductionService::Request_Production`            *
 *	already uses for `available_credits`.  The one real work (`Update`) is the building roster           *
 *	walk, the descriptor search and the site search that fill those structs in and act on the             *
 *	answer; it is not directly self-check-testable for the same reason `Find_Nearest_Repair_Bay`           *
 *	and `Govern_Subsystem_Speed` are not (Phase 22) -- it needs a live scene this harness has no             *
 *	way to construct.                                                                                       *
 *                                                                                             *
 *	Descriptors are matched by what they actually say about themselves, not by a guessed name:            *
 *	`descriptor->Build.Buildable`, `descriptor->Build.Team` (a `PLAYERTYPE_*` value, or -1 for            *
 *	"any faction" -- `buildingdescriptor.h`'s own field, read by `CommanderPlacementValidator`               *
 *	the identical way), and `descriptor->Power.Generation > 0.0f` / `descriptor->TiberiumSilo.Enabled`.     *
 *	Section 24.3's "more Refinery/resource infrastructure" is not decided by this planner: nothing in         *
 *	`BuildingModelDescriptor` distinguishes a Refinery from any other buildable building, and                 *
 *	Harvester/Tiberium income has no path to a Commander's wallet yet regardless (no `HarvesterClass`           *
 *	is ever spawned for a generated world -- `tiberiumfieldgenerator.h`'s own disclosed gap), so                *
 *	deciding "build a Refinery" would be a decision with no way to ever pay for itself honestly.                *
 *                                                                                             *
 *	Base-expansion site candidates reuse Phase 7's own `StrategicPlanner` node graph -- "reuse over    *
 *	rediscovery" is that file's own stated rule, and a second, invented ring-search would be exactly       *
 *	the parallel system it warns against.  `Score_Expansion_Candidate` is exposed and real, but            *
 *	`Update` does not act on it by founding a second base this phase: that is a materially larger            *
 *	behavior (protecting, powering and connecting a whole new base cluster) than "evaluate candidate          *
 *	sites" requires, and founding one badly is worse than not founding one -- a real, disclosed limit,         *
 *	not a silent one.  The economy planner's own buildings (a Power Plant, a Silo) are sited by a               *
 *	bounded search around the team's *existing* buildings instead, which is ordinary in-base                    *
 *	construction, not expansion.                                                                                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERAI_H
#define COMMANDERAI_H

#include "always.h"
#include "vector3.h"
#include "wwstring.h"

class BuildingModelDescriptor;


/*
**	Reserved Commander id for an AI-held seat.  `CommanderProductionService::_Commanders[]` uses
**	0 for "vacant" and a real client's positive id (`cNetwork::Get_My_Id()`, always > 0 --
**	`cNetwork::Get_Client_String` treats <= 0 as never a real recipient) for a human.  -1 is
**	therefore distinct from both, and every authority check already written against
**	`Get_Team_Commander(team) == requester_id` refuses an AI seat's own actions from a real
**	client for free, with no new comparison anywhere.
*/
enum { AI_COMMANDER_ID = -1 };


enum CommanderAIEconomyActionType
{
	AI_ECONOMY_NONE = 0,				// nothing is short right now
	AI_ECONOMY_BUILD_POWER_PLANT,
	AI_ECONOMY_BUILD_SILO,
	AI_ECONOMY_PAUSE,					// something is short, but the treasury cannot afford it yet
};


/*
**	Section 24.2's strategic world model.  Every field here is either this AI's own team's data
**	(never a secret from itself) or explicitly shroud-filtered enemy data -- see
**	`KnownEnemyBuildingCount`'s own comment.  Nothing here is a raw enemy truth read.
*/
struct CommanderAIWorldSnapshot
{
	CommanderAIWorldSnapshot(void) :
		AvailableCredits(0.0f),
		PowerGeneration(0.0f),PowerAllocatedDemand(0.0f),
		PowerPlantCount(0),SiloCount(0),
		DamagedBuildingCount(0),
		FriendlyUnitCount(0),
		KnownEnemyBuildingCount(0) { }

	float	AvailableCredits;			// this team's AI treasury -- see AITreasury

	float	PowerGeneration;			// TeamPowerGridService::Get_Generation(team)
	float	PowerAllocatedDemand;	// TeamPowerGridService::Get_Allocated_Demand(team)

	//	Real buildings, classified by their own resolved descriptor
	//	(`BuildingGameObjDef::Get_Descriptor_Name` -> `BuildingDescriptorManager::Get_Descriptor`,
	//	the same resolution `TeamPowerGridService::Peek_Descriptor` and `building.cpp`'s own
	//	subcomponent code already use) -- never inferred from a model or preset name.
	int	PowerPlantCount;			// Power.Generation > 0 on the resolved descriptor
	int	SiloCount;					// TiberiumSilo.Enabled on the resolved descriptor

	int	DamagedBuildingCount;	// this team's own buildings below full health -- never a secret

	int	FriendlyUnitCount;		// CommanderProductionService::Get_Population summed over kinds

	/*
	**	Buildings this team has actually explored on the enemy's territory
	**	(`WorldShroudManager::Is_Explored(this_team, enemy_building_position)`), not the enemy's
	**	real building count.  A building the shroud has never uncovered is not counted, the same
	**	as a human Commander's own sidebar would never show it.
	*/
	int	KnownEnemyBuildingCount;
};


/*
**	Section 24.3's economy planner, and Section 24.8's "planning interval" in miniature: tunable
**	targets rather than a hardcoded ladder, and a headroom fraction rather than a fixed margin.
*/
struct CommanderAIEconomyPolicy
{
	CommanderAIEconomyPolicy(void) :
		TargetSiloCount(1),
		PowerHeadroomFraction(0.85f) { }

	int	TargetSiloCount;					// how many Silos this team wants online

	/*
	**	Power is short once AllocatedDemand crosses this fraction of Generation -- not 1.0,
	**	because the whole point of building ahead of a shortfall is to never actually shed load.
	**	Zero generation with any real demand is always short, regardless of this fraction.
	*/
	float	PowerHeadroomFraction;
};


/*
**	Section 24.4's base expansion, scored as a weighted sum of normalised (0..1) terms rather
**	than a single opaque number, so a self check can move one term at a time and see the score
**	move with it.  `NodeId` names the `StrategicPlanner` node this candidate came from -- see
**	`Update`'s own comment for why founding a second base on the winner is not this phase's job.
*/
struct CommanderAIExpansionCandidate
{
	CommanderAIExpansionCandidate(void) :
		NodeId(""),Position(0.0f,0.0f,0.0f),
		ResourceDistance(-1.0f),RoadAccessDistance(-1.0f),
		EnemyPressure(0.0f),PowerOk(true),BuildableAreaOk(false),
		DistanceFromBoundary(0.0f) { }

	StringClass	NodeId;

	Vector3	Position;

	float	ResourceDistance;			// world units to the nearest known resource field; -1 = none known
	float	RoadAccessDistance;		// world units to the nearest logistics graph node; -1 = none known
	float	EnemyPressure;				// 0..1, near known enemy activity to fully clear
	bool		PowerOk;						// team power headroom can carry one more consumer
	bool		BuildableAreaOk;			// CommanderPlacementValidator::Validate passed at this position
	float	DistanceFromBoundary;	// world units to the nearest map edge (Directive 1.3's boundary)
};


struct CommanderAIExpansionWeights
{
	CommanderAIExpansionWeights(void) :
		ResourceWeight(0.35f),RoadWeight(0.20f),SafetyWeight(0.20f),
		PowerWeight(0.15f),BoundaryWeight(0.10f),
		ResourceReferenceDistance(1200.0f),RoadReferenceDistance(1200.0f),
		BoundaryReferenceDistance(600.0f) { }

	//	Sums to 1.0 across the five scored terms; not enforced, so a caller tuning one weight
	//	does not have to renormalise the rest by hand -- Score_Expansion_Candidate just weights
	//	whatever it is given.
	float	ResourceWeight;
	float	RoadWeight;
	float	SafetyWeight;
	float	PowerWeight;
	float	BoundaryWeight;

	//	Distances beyond which the corresponding term reads as fully 0 -- "close" and "far" are
	//	meaningless without a scale, and these are that scale, tunable rather than hardcoded into
	//	the scoring function itself.
	float	ResourceReferenceDistance;
	float	RoadReferenceDistance;
	float	BoundaryReferenceDistance;
};


class CommanderAIService
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Section 24.1.  Refuses to seat an AI over a real (positive, human) Commander id -- an
		**	AI Commander only ever fills a genuinely vacant seat, never displaces a player.
		**	Deactivate only clears the seat if it is still AI-held, the same "do not take back
		**	what you do not hold" rule `cCommanderClaimEvent::Act`'s release branch already uses --
		**	a human who won a Section 25.1 REMOVE vote against this AI already cleared the seat
		**	themselves, and a second clear here would be a no-op anyway, but the guard is what
		**	makes that true by construction rather than by coincidence.
		*/
		static bool		Activate(int team);
		static void		Deactivate(int team);
		static bool		Is_Active(int team);

		/*
		**	The AI Commander's own treasury.  Starts at, and stays at, zero: this codebase's
		**	Tiberium income (`Give_Money`, scriptcommands.cpp) only ever credits a connected
		**	human's own `PlayerDataClass`, and no `HarvesterClass` is ever spawned for a generated
		**	world (`tiberiumfieldgenerator.h`'s own disclosed gap), so a bot-only team has no
		**	income path yet.  Exposed as real, settable state -- not simulated away -- so the
		**	economy planner is exercised with a genuine nonzero balance by the self check, and is
		**	already correct the day an income path exists; `Increment_Treasury` is the one entry
		**	point that path would call.
		*/
		static float	Get_Treasury(int team);
		static void		Set_Treasury(int team,float credits);
		static void		Increment_Treasury(int team,float delta);

		//	How often a per-team AI Commander re-plans -- Section 24.8's "planning interval" in
		//	miniature, one shared value rather than per-difficulty tuning this phase does not
		//	build.  Update is a correct, cheap no-op between intervals.
		static float	Get_Planning_Interval_Seconds(void);

		/*
		**	Section 24.3.  Pure: reads the snapshot and policy, decides what is most needed right
		**	now, in the fixed priority order Section 24.3 itself implies (keep the lights on
		**	first).  Never looks at a descriptor or a treasury balance -- "can we afford it" is
		**	`Update`'s question, once this has answered "what do we need".
		*/
		static CommanderAIEconomyActionType	Decide_Economy_Priority(
			const CommanderAIWorldSnapshot & snapshot,const CommanderAIEconomyPolicy & policy);

		//	Section 24.4.  Pure weighted sum of the candidate's own normalised terms; see
		//	CommanderAIExpansionWeights for what "normalised" means for each one.
		static float	Score_Expansion_Candidate(
			const CommanderAIExpansionCandidate & candidate,const CommanderAIExpansionWeights & weights);

		/*
		**	The one real function.  Server-only, a cheap no-op between planning intervals.  Per
		**	active AI-held team: builds a real `CommanderAIWorldSnapshot`, asks
		**	`Decide_Economy_Priority`, resolves that priority to a real loaded, buildable
		**	descriptor for this team's own faction by `Find_Buildable_Descriptor`, searches a
		**	bounded ring of candidate sites around the team's own existing buildings for one
		**	`CommanderPlacementValidator::Validate` accepts, and -- only if the treasury actually
		**	affords the resolved descriptor's real cost -- builds it through the same
		**	`RuntimeBuildingFactory`/`BuildingConstructionService` pipeline
		**	`cCommanderPlacementEvent::Act` uses, charging the AI treasury in place of a player's
		**	wallet.  Also refreshes `Get_Expansion_Candidate_Count`/`Get_Expansion_Candidate` from
		**	`StrategicPlanner`, purely for Section 24.9 diagnostics -- see this file's own header
		**	for why nothing acts on the winner yet.  Every other outcome (no matching descriptor
		**	loaded, no site found, no existing building to search around, insufficient treasury)
		**	is a real, silent no-op this tick, exactly as "nothing to do yet" should read.
		*/
		static void		Update(int team,float seconds);

		//	Diagnostics -- Section 24.9's own named list, the parts of it 24.1-24.4 can answer.
		static int		Get_Last_Decision(int team);						// CommanderAIEconomyActionType
		static const CommanderAIWorldSnapshot &	Get_Last_Snapshot(int team);
		static int		Get_Attempted_Build_Count(int team);
		static int		Get_Succeeded_Build_Count(int team);

		static const char *	Get_Economy_Action_Name(CommanderAIEconomyActionType action);

		/*
		**	Section 24.4 diagnostics.  Refreshed once per planning tick by `Update`, from
		**	`StrategicPlanner`'s own BASE_CANDIDATE/SETTLEMENT_CANDIDATE nodes for this team's
		**	faction, minus whichever one `BaseSelectionPlanner` already resolved as this team's
		**	own starting base.  Empty (count 0) whenever `StrategicPlanner::Has_Plan()` is false --
		**	a legacy/static map, Directive 1.10, has no candidate graph to read and this is a
		**	real, correct empty result, not a failure.  See this file's own header for why
		**	nothing in `Update` acts on these yet.
		*/
		static int		Get_Expansion_Candidate_Count(int team);
		static bool		Get_Expansion_Candidate(int team,int index,CommanderAIExpansionCandidate * out);

		/*
		**	The one real, data-driven descriptor search this file performs -- exposed so the
		**	self check can prove "no matching descriptor", "wrong faction is skipped" and "first
		**	buildable match for this faction wins" as real outcomes rather than assumed ones.
		**	power_generating selects the Power Plant signal when true, the Silo signal when
		**	false.  team is a PLAYERTYPE_* value; a descriptor whose own Build.Team names a
		**	different real faction is skipped, and Build.Team == -1 ("any faction") matches.
		*/
		static const BuildingModelDescriptor *	Find_Buildable_Descriptor(bool power_generating,int team);
};

#endif // COMMANDERAI_H
