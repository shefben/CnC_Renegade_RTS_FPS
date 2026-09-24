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
 *                     $Archive:: /Commando/Code/Combat/teampowergridservice.cpp              $*
 *                                                                                             *
 *	Roadmap Section 40, Phase 34 -- finite team power grid + strategic building capabilities.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "teampowergridservice.h"

#include "building.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "vector3.h"
#include "worldshroudmanager.h"
#include "combat.h"
#include "gameobjmanager.h"
#include "playerdata.h"
#include "soldier.h"


struct PowerGridMember
{
	PowerGridMember(void) : InUse(false),MemberId(0),Building(nullptr),Team(0),DefinitionName(""),
									 Position(0,0,0),Destroyed(false),ManualDisabled(false),
									 HasPriorityOverride(false),PriorityOverride(0),IsAllocated(false),
									 ShroudSensorHandle(0) { }
	bool	operator == (const PowerGridMember &) const	{ return false; }
	bool	operator != (const PowerGridMember &) const	{ return true; }

	bool						InUse;
	PowerGridMemberIdType	MemberId;
	BuildingGameObj *		Building;		// may be nullptr -- see the header
	int						Team;
	StringClass				DefinitionName;
	Vector3					Position;		// refreshed from Building each RecalculateTeam if non-null
	bool						Destroyed;
	bool						ManualDisabled;
	bool						HasPriorityOverride;
	int						PriorityOverride;
	bool						IsAllocated;	// this team's last recalculation's result
	int						ShroudSensorHandle;
};

struct TeamPowerState
{
	TeamPowerState(void) : Team(0),Generation(0.0f),RequestedDemand(0.0f),AllocatedDemand(0.0f),
								  IsReplicated(false),ShedCount(0),PoweredCount(0),ManualOffCount(0) { }
	bool	operator == (const TeamPowerState &) const	{ return false; }
	bool	operator != (const TeamPowerState &) const	{ return true; }

	int		Team;
	float		Generation;
	float		RequestedDemand;
	float		AllocatedDemand;

	/*
	**	Client mirror (Phase 40).  Set only by Set_Replicated_Team_State, and only on a client:
	**	a client's allocator never runs, so its members carry no IsAllocated result to count and
	**	the three counts below have to arrive from the server rather than be walked for.  A
	**	server leaves IsReplicated false for ever and keeps counting its own members.
	*/
	bool		IsReplicated;
	int		ShedCount;
	int		PoweredCount;
	int		ManualOffCount;
};

static DynamicVectorClass<PowerGridMember>	_Members;
static DynamicVectorClass<TeamPowerState>	_Teams;

/*
**	Q-014.  Stock Renegade never had a credit ceiling, so this is a new gameplay number rather
**	than one recovered from the engine.  Held in a variable rather than as a constant so it can
**	be balanced from the console without a rebuild.
*/
const float	TeamPowerGridService::DEFAULT_BASE_WALLET_CAP = 10000.0f;

static float	_BaseWalletCap		= TeamPowerGridService::DEFAULT_BASE_WALLET_CAP;
static float	_CreditsWiped [4]	= { 0.0f,0.0f,0.0f,0.0f };
static int		_WipedTeams [4]	= { 0,0,0,0 };
static int		_WipedTeamCount	= 0;

/*
**	Phase 20's re-entrancy guard.  RecalculateTeam drives Enable_Power on every member, and
**	Enable_Power calls BuildingGameObj::Update_State, which (as of Phase 20) can itself ask to
**	recalculate the same team when the building tracks a grid-facing subcomponent -- a plain
**	nested call would stack-overflow.  Safe to just skip the nested call rather than queue or
**	defer it: the in-progress outer pass reads every member's *current* effective generation/
**	radius right now, health included, so it already reflects whatever triggered the nested call
**	by the time it gets there.  Fixed-size and small on purpose -- team ids are a tiny enumerated
**	range, not something to heap-allocate for.
*/
static int		_RecalculatingTeams [8];
static int		_RecalculatingTeamCount = 0;


//	Small fixed table rather than a per-team field, because the wipe total is debug/telemetry
//	and must not change the replication shape of TeamPowerState.
static int	Find_Or_Make_Wiped_Slot(int team)
{
	for (int i = 0; i < _WipedTeamCount; i++) {
		if (_WipedTeams [i] == team) return i;
	}
	if (_WipedTeamCount >= 4) return -1;
	_WipedTeams [_WipedTeamCount] = team;
	_CreditsWiped [_WipedTeamCount] = 0.0f;
	return _WipedTeamCount++;
}


static int	Find_Member(PowerGridMemberIdType member_id)
{
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].InUse && _Members [i].MemberId == member_id) return i;
	}
	return -1;
}

static int	Find_Or_Make_Team(int team)
{
	for (int i = 0; i < _Teams.Count (); i++) {
		if (_Teams [i].Team == team) return i;
	}
	TeamPowerState state;
	state.Team = team;
	_Teams.Add (state);
	return _Teams.Count () - 1;
}

//	A member with no resolved descriptor is a safe no-op on the grid: it neither generates nor
//	consumes, and is never shed for lacking one -- the same "missing config never blocks engine
//	work" rule this project applies to missing art.
static const BuildingModelDescriptor *	Peek_Descriptor(const PowerGridMember & member)
{
	if (member.DefinitionName.Get_Length () == 0) return nullptr;
	return BuildingDescriptorManager::Get_Descriptor (member.DefinitionName.Peek_Buffer ());
}

static int	Effective_Priority(const PowerGridMember & member,const BuildingModelDescriptor * descriptor)
{
	if (member.HasPriorityOverride) return member.PriorityOverride;
	return (descriptor != nullptr) ? descriptor->Power.Priority : 0;
}


void TeamPowerGridService::Init(void)
{
	_Members.Delete_All ();
	_Teams.Delete_All ();
}

void TeamPowerGridService::Shutdown(void)
{
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].ShroudSensorHandle != 0) {
			WorldShroudManager::UnregisterStrategicSensorSource (_Members [i].ShroudSensorHandle);
		}
	}
	_Members.Delete_All ();
	_Teams.Delete_All ();
}


bool TeamPowerGridService::RegisterBuilding(BuildingGameObj * building,int team,const char * definition_name,
															PowerGridMemberIdType member_id)
{
	if (Find_Member (member_id) >= 0) {
		return false;		// already registered -- a caller re-registering the same id is a mistake
	}

	int slot = -1;
	for (int i = 0; i < _Members.Count (); i++) {
		if (!_Members [i].InUse) { slot = i; break; }
	}

	PowerGridMember member;
	member.InUse				= true;
	member.MemberId			= member_id;
	member.Building			= building;
	member.Team					= team;
	member.DefinitionName	= (definition_name != nullptr) ? definition_name : "";
	member.Position			= Vector3 (0,0,0);
	if (building != nullptr) {
		building->Get_Position (&member.Position);
	}

	if (slot < 0) {
		_Members.Add (member);
	} else {
		_Members [slot] = member;
	}

	Find_Or_Make_Team (team);
	return true;
}

void TeamPowerGridService::UnregisterBuilding(PowerGridMemberIdType member_id)
{
	int index = Find_Member (member_id);
	if (index < 0) return;

	if (_Members [index].ShroudSensorHandle != 0) {
		WorldShroudManager::UnregisterStrategicSensorSource (_Members [index].ShroudSensorHandle);
	}
	_Members [index] = PowerGridMember ();		// InUse false, slot free for reuse
}


void TeamPowerGridService::OnBuildingDestroyed(PowerGridMemberIdType member_id)
{
	OnBuildingOperationalStateChanged (member_id,true);
}

void TeamPowerGridService::OnBuildingRevived(PowerGridMemberIdType member_id)
{
	OnBuildingOperationalStateChanged (member_id,false);
}

void TeamPowerGridService::OnBuildingOperationalStateChanged(PowerGridMemberIdType member_id,bool destroyed)
{
	int index = Find_Member (member_id);
	if (index < 0) return;
	_Members [index].Destroyed = destroyed;
	RecalculateTeam (_Members [index].Team);
}

void TeamPowerGridService::OnDescriptorReload(const char * definition_name)
{
	if (definition_name == nullptr) return;

	bool team_touched [256];
	for (int i = 0; i < 256; i++) team_touched [i] = false;		// see the note below on the bound

	for (int i = 0; i < _Members.Count (); i++) {
		if (!_Members [i].InUse) continue;
		if (_Members [i].DefinitionName.Compare_No_Case (definition_name) != 0) continue;

		int team = _Members [i].Team;
		//	Teams are PLAYERTYPE-range values (Section 30's own convention); the -4..N range
		//	this project's PlayerType enum uses fits comfortably inside a 256-wide dedup table
		//	offset to always be non-negative.
		int bucket = team + 128;
		if (bucket < 0 || bucket >= 256) continue;
		if (team_touched [bucket]) continue;
		team_touched [bucket] = true;

		RecalculateTeam (team);
	}
}


void TeamPowerGridService::RecalculateTeam(int team)
{
	for (int i = 0; i < _RecalculatingTeamCount; i++) {
		if (_RecalculatingTeams [i] == team) return;		// see the re-entrancy note above
	}
	if (_RecalculatingTeamCount < 8) {
		_RecalculatingTeams [_RecalculatingTeamCount++] = team;
	}

	int team_index = Find_Or_Make_Team (team);

	float available = 0.0f;
	float requested = 0.0f;
	float allocated = 0.0f;

	//	Pass 1: generators -- always on while alive and not manually disabled; "requires_power
	//	= false" means they need no grid allocation of their own.
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team) continue;

		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		bool is_generator = (descriptor != nullptr) && (descriptor->Power.RequiresPower == false) &&
									(descriptor->Power.Generation > 0.0f);
		if (!is_generator) continue;

		bool operational = !member.Destroyed && !member.ManualDisabled;
		member.IsAllocated = operational;
		if (operational) {
			/*
			**	Phase 20 -- a real BuildingGameObj reports its own health-scaled generation
			**	(descriptor->Power.Generation unmodified for a member with no power_generator_module
			**	subcomponent, or for the self check's synthetic null-building members); "is this
			**	building a generator at all" above still reads the descriptor's raw, undamaged
			**	figure, deliberately -- generator classification is structural, not health-dependent.
			*/
			available += (member.Building != nullptr) ? member.Building->Get_Effective_Power_Generation ()
																	  : descriptor->Power.Generation;
		}
	}

	//	Pass 2: consumers, sorted by effective priority (descending) then MemberId (ascending)
	//	for a deterministic tie-break -- greedy allocation until the running total would exceed
	//	AvailablePower.  A plain insertion sort over an index list; team rosters are small.
	DynamicVectorClass<int>	consumer_indices;
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team) continue;

		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		bool is_consumer = (descriptor == nullptr) || (descriptor->Power.RequiresPower == true);
		if (!is_consumer) continue;			// generators were handled in pass 1

		if (member.Destroyed || member.ManualDisabled) {
			member.IsAllocated = false;		// "manual disable immediately frees demand"
			continue;								// not counted in RequestedPower either
		}

		float consumption = (descriptor != nullptr) ? descriptor->Power.Consumption : 0.0f;
		requested += consumption;
		consumer_indices.Add (i);
	}

	for (int a = 1; a < consumer_indices.Count (); a++) {
		int key = consumer_indices [a];
		const BuildingModelDescriptor * key_descriptor = Peek_Descriptor (_Members [key]);
		int key_priority = Effective_Priority (_Members [key],key_descriptor);
		int b = a - 1;
		while (b >= 0) {
			int candidate = consumer_indices [b];
			const BuildingModelDescriptor * candidate_descriptor = Peek_Descriptor (_Members [candidate]);
			int candidate_priority = Effective_Priority (_Members [candidate],candidate_descriptor);
			bool candidate_first = (candidate_priority > key_priority) ||
											((candidate_priority == key_priority) && (_Members [candidate].MemberId <= _Members [key].MemberId));
			if (candidate_first) break;
			consumer_indices [b + 1] = consumer_indices [b];
			b--;
		}
		consumer_indices [b + 1] = key;
	}

	for (int c = 0; c < consumer_indices.Count (); c++) {
		PowerGridMember & member = _Members [consumer_indices [c]];
		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		float consumption = (descriptor != nullptr) ? descriptor->Power.Consumption : 0.0f;

		if ((allocated + consumption) <= available) {
			member.IsAllocated = true;
			allocated += consumption;
		} else {
			member.IsAllocated = false;
		}
	}

	_Teams [team_index].Generation		= available;
	_Teams [team_index].RequestedDemand	= requested;
	_Teams [team_index].AllocatedDemand	= allocated;

	//	Drive the one canonical per-building flag, and communications coverage.
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team) continue;

		/*
		**	A member whose DefinitionName resolves to nothing is not this service's building.
		**	It was registered anyway -- every BuildingGameObj is, so that authoring one JSON
		**	file is the only step needed to hand a building over -- but BaseControllerClass::
		**	Power_Base still owns its Enable_Power calls, and Has_Resolved_Descriptor is what
		**	tells Power_Base's own loop which ones those are.  Skipping it here is the other
		**	half of that same rule: without it this loop would drive Enable_Power(true) on
		**	every undescribed building on the team the next time anything on the team was
		**	destroyed or revived, silently overriding whatever Power_Base had decided.
		*/
		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		if (descriptor == nullptr) continue;

		if (member.Building != nullptr) {
			member.Building->Get_Position (&member.Position);
			if (!member.Destroyed) {
				member.Building->Enable_Power (member.IsAllocated);
			}
		}

		if (!descriptor->Communications.Enabled) continue;

		bool online_eligible = !member.Destroyed &&
										(member.IsAllocated || !descriptor->Communications.RequiresBuildingOnline);

		/*
		**	Phase 20 -- "damaged communications array -> reduced radius" and "destroyed radar dish
		**	-> radar disabled while the building may remain alive" (Section 21.2) both fall straight
		**	out of scaling the two radii independently: a vision-only communications_array entry
		**	shrinks VisionRadius while RadarRadius holds, and a radar_dish entry can reach zero
		**	(radar off) well before the building itself is destroyed.  Unmodified for a member with
		**	no such subcomponent, or for the self check's synthetic null-building members.
		*/
		float vision_radius = (member.Building != nullptr) ? member.Building->Get_Effective_Vision_Radius ()
																			  : descriptor->Communications.VisionRadius;
		float radar_radius  = (member.Building != nullptr) ? member.Building->Get_Effective_Radar_Radius ()
																			  : descriptor->Communications.RadarRadius;

		if (online_eligible) {
			if (member.ShroudSensorHandle == 0) {
				member.ShroudSensorHandle = WorldShroudManager::RegisterStrategicSensorSource (
					team,member.Position,vision_radius,
					radar_radius,descriptor->Communications.LastKnownRetentionSeconds,true);
			} else {
				WorldShroudManager::UpdateStrategicSensorSource (
					member.ShroudSensorHandle,member.Position,vision_radius,
					radar_radius,true);
			}
		} else if (member.ShroudSensorHandle != 0) {
			WorldShroudManager::UnregisterStrategicSensorSource (member.ShroudSensorHandle);
			member.ShroudSensorHandle = 0;
		}
	}

	/*
	**	Q-014: the cap has just been recomputed along with everything else, so this is the one
	**	place a Silo can have been lost, revived, built or shed.  Enforcing here rather than at
	**	each of those call sites means a future way of losing a Silo cannot forget to wipe the
	**	credits it was holding.  Server-only, and a no-op when no cap was exceeded.
	*/
	Enforce_Wallet_Cap (team);

	for (int i = 0; i < _RecalculatingTeamCount; i++) {
		if (_RecalculatingTeams [i] == team) {
			_RecalculatingTeams [i] = _RecalculatingTeams [_RecalculatingTeamCount - 1];
			_RecalculatingTeamCount--;
			break;
		}
	}
}


float TeamPowerGridService::Get_Generation(int team)			{ return _Teams [Find_Or_Make_Team (team)].Generation; }
float TeamPowerGridService::Get_Requested_Demand(int team)	{ return _Teams [Find_Or_Make_Team (team)].RequestedDemand; }
float TeamPowerGridService::Get_Allocated_Demand(int team)	{ return _Teams [Find_Or_Make_Team (team)].AllocatedDemand; }


bool TeamPowerGridService::Set_Manual_Disabled(PowerGridMemberIdType member_id,bool disabled)
{
	int index = Find_Member (member_id);
	if (index < 0) return false;
	_Members [index].ManualDisabled = disabled;
	RecalculateTeam (_Members [index].Team);		// "manual disable immediately frees demand"
	return true;
}

bool TeamPowerGridService::Is_Manual_Disabled(PowerGridMemberIdType member_id)
{
	int index = Find_Member (member_id);
	return (index >= 0) && _Members [index].ManualDisabled;
}

bool TeamPowerGridService::Set_Priority_Override(PowerGridMemberIdType member_id,int priority)
{
	int index = Find_Member (member_id);
	if (index < 0) return false;
	_Members [index].HasPriorityOverride	= true;
	_Members [index].PriorityOverride		= priority;
	RecalculateTeam (_Members [index].Team);		// "manual priority... control is authoritative"
	return true;
}

bool TeamPowerGridService::Clear_Priority_Override(PowerGridMemberIdType member_id)
{
	int index = Find_Member (member_id);
	if (index < 0) return false;
	_Members [index].HasPriorityOverride = false;
	RecalculateTeam (_Members [index].Team);
	return true;
}

bool TeamPowerGridService::Is_Member_Powered(PowerGridMemberIdType member_id)
{
	int index = Find_Member (member_id);
	return (index >= 0) && _Members [index].IsAllocated;
}

bool TeamPowerGridService::Has_Resolved_Descriptor(PowerGridMemberIdType member_id)
{
	int index = Find_Member (member_id);
	if (index < 0) return false;
	return Peek_Descriptor (_Members [index]) != nullptr;
}


float TeamPowerGridService::Get_Wallet_Capacity_Bonus(int team)
{
	float bonus = 0.0f;
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team || member.Destroyed) continue;

		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		if (descriptor == nullptr || !descriptor->TiberiumSilo.Enabled) continue;

		bool online_eligible = member.IsAllocated || !descriptor->TiberiumSilo.RequiresBuildingOnline;
		if (online_eligible) {
			bonus += descriptor->TiberiumSilo.WalletCapacityBonusPerPlayer;
		}
	}
	return bonus;
}


float TeamPowerGridService::Get_Base_Wallet_Cap(void)		{ return _BaseWalletCap; }

void TeamPowerGridService::Set_Base_Wallet_Cap(float cap)
{
	if (cap < 0.0f) cap = 0.0f;
	_BaseWalletCap = cap;

	//	A lowered base takes effect on the teams that already exist, the same way losing a Silo
	//	does -- otherwise the new number would only bite the next time something was destroyed.
	for (int i = 0; i < _Teams.Count (); i++) {
		Enforce_Wallet_Cap (_Teams [i].Team);
	}
}


/***********************************************************************************************
 * Get_Wallet_Capacity -- Section 40's EffectivePlayerWalletCap                                  *
 *                                                                                             *
 *	Computed from the member list on both sides.  Only the server ever *enforces* it, so a       *
 *	client reading a slightly stale number while a Silo is being shed changes nothing: a         *
 *	client's wallet is whatever the server last replicated, never something it clamps itself.    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
float TeamPowerGridService::Get_Wallet_Capacity(int team)
{
	float capacity = _BaseWalletCap + Get_Wallet_Capacity_Bonus (team);
	return (capacity < 0.0f) ? 0.0f : capacity;
}


/***********************************************************************************************
 * Enforce_Wallet_Cap                                                                            *
 *                                                                                             *
 *	The sweep.  A Silo destroyed while full does not merely stop raising the ceiling -- the      *
 *	credits it was holding are gone, so a team that banked 20,000 behind two Silos and loses     *
 *	one drops to 15,000 immediately.  That is what makes a Silo worth attacking.                 *
 *                                                                                             *
 *	Players are reached through their soldiers rather than through a player manager, because     *
 *	cPlayerManager lives in Commando and this service is Combat -- the same route every other    *
 *	Commander-layer sweep in this library already takes.                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int TeamPowerGridService::Enforce_Wallet_Cap(int team)
{
	if (!CombatManager::I_Am_Server ()) {
		return 0;
	}

	float capacity = Get_Wallet_Capacity (team);
	int affected = 0;

	SList<SmartGameObj> * list = GameObjManager::Get_Smart_Game_Obj_List ();
	if (list == nullptr) {
		return 0;
	}

	for (SLNode<SmartGameObj> * node = list->Head (); node != nullptr; node = node->Next ()) {

		SmartGameObj * obj = node->Data ();
		if (obj == nullptr) continue;

		SoldierGameObj * soldier = obj->As_SoldierGameObj ();
		if (soldier == nullptr) continue;
		if (soldier->Get_Player_Type () != team) continue;

		PlayerDataClass * player = soldier->Get_Player_Data ();
		if (player == nullptr) continue;

		float money = player->Get_Money ();
		if (money <= capacity) continue;

		//	Through Set_Money rather than by writing the field, so cPlayer's override still
		//	marks the player dirty and the loss actually reaches the client.
		player->Set_Money (capacity);

		int slot = Find_Or_Make_Wiped_Slot (team);
		if (slot >= 0) {
			_CreditsWiped [slot] += (money - capacity);
		}
		affected++;
	}

	return affected;
}


float TeamPowerGridService::Get_Credits_Wiped(int team)
{
	for (int i = 0; i < _WipedTeamCount; i++) {
		if (_WipedTeams [i] == team) return _CreditsWiped [i];
	}
	return 0.0f;
}


void TeamPowerGridService::Reset_Credits_Wiped(void)
{
	for (int i = 0; i < 4; i++) {
		_CreditsWiped [i] = 0.0f;
		_WipedTeams [i] = 0;
	}
	_WipedTeamCount = 0;
}


int TeamPowerGridService::Get_Powered_Count(int team)
{
	const TeamPowerState & state = _Teams [Find_Or_Make_Team (team)];
	if (state.IsReplicated) return state.PoweredCount;

	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].InUse && _Members [i].Team == team && !_Members [i].Destroyed && _Members [i].IsAllocated) count++;
	}
	return count;
}

int TeamPowerGridService::Get_Shed_Count(int team)
{
	const TeamPowerState & state = _Teams [Find_Or_Make_Team (team)];
	if (state.IsReplicated) return state.ShedCount;

	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team || member.Destroyed || member.ManualDisabled) continue;
		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		bool is_consumer = (descriptor == nullptr) || (descriptor->Power.RequiresPower == true);
		if (is_consumer && !member.IsAllocated) count++;
	}
	return count;
}

int TeamPowerGridService::Get_Manual_Off_Count(int team)
{
	const TeamPowerState & state = _Teams [Find_Or_Make_Team (team)];
	if (state.IsReplicated) return state.ManualOffCount;

	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].InUse && _Members [i].Team == team && _Members [i].ManualDisabled) count++;
	}
	return count;
}

int TeamPowerGridService::Get_Generator_Count(int team)
{
	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team) continue;
		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		if (descriptor != nullptr && !descriptor->Power.RequiresPower && descriptor->Power.Generation > 0.0f) count++;
	}
	return count;
}

int TeamPowerGridService::Get_Consumer_Count(int team)
{
	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		PowerGridMember & member = _Members [i];
		if (!member.InUse || member.Team != team) continue;
		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		if (descriptor != nullptr && descriptor->Power.RequiresPower) count++;
	}
	return count;
}

bool TeamPowerGridService::Set_Replicated_Member_State(PowerGridMemberIdType member_id,bool manual_disabled,
																		  bool has_priority_override,int priority_override)
{
	int index = Find_Member (member_id);
	if (index < 0) return false;
	_Members [index].ManualDisabled			= manual_disabled;
	_Members [index].HasPriorityOverride	= has_priority_override;
	_Members [index].PriorityOverride		= has_priority_override ? priority_override : 0;
	return true;
}


void TeamPowerGridService::Set_Replicated_Team_State(int team,float generation,float requested,
																		  float allocated,int shed_count,int powered_count,
																		  int manual_off_count)
{
	TeamPowerState & state = _Teams [Find_Or_Make_Team (team)];
	state.Generation		= generation;
	state.RequestedDemand	= requested;
	state.AllocatedDemand	= allocated;
	state.ShedCount			= shed_count;
	state.PoweredCount		= powered_count;
	state.ManualOffCount	= manual_off_count;

	//	Latching rather than a per-call flag: once a client has been told, it must never fall
	//	back to walking its own members, or a packet-less frame would make the bars flicker to
	//	whatever a never-run allocator left behind.
	state.IsReplicated		= true;
}


bool TeamPowerGridService::Get_Member_By_Index(int index,PowerGridMemberIdType * out_member_id,int * out_team)
{
	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		if (!_Members [i].InUse) continue;
		if (count == index) {
			if (out_member_id != nullptr)	*out_member_id	= _Members [i].MemberId;
			if (out_team != nullptr)		*out_team		= _Members [i].Team;
			return true;
		}
		count++;
	}
	return false;
}

bool TeamPowerGridService::Get_Priority_Override(PowerGridMemberIdType member_id,int * out_priority)
{
	int index = Find_Member (member_id);
	if (index < 0) return false;
	if (!_Members [index].HasPriorityOverride) return false;
	if (out_priority != nullptr) *out_priority = _Members [index].PriorityOverride;
	return true;
}

int TeamPowerGridService::Get_Member_Count(void)
{
	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].InUse) count++;
	}
	return count;
}
