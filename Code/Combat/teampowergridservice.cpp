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
	TeamPowerState(void) : Team(0),Generation(0.0f),RequestedDemand(0.0f),AllocatedDemand(0.0f) { }
	bool	operator == (const TeamPowerState &) const	{ return false; }
	bool	operator != (const TeamPowerState &) const	{ return true; }

	int		Team;
	float		Generation;
	float		RequestedDemand;
	float		AllocatedDemand;
};

static DynamicVectorClass<PowerGridMember>	_Members;
static DynamicVectorClass<TeamPowerState>	_Teams;


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
			available += descriptor->Power.Generation;
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

		if (member.Building != nullptr) {
			member.Building->Get_Position (&member.Position);
			if (!member.Destroyed) {
				member.Building->Enable_Power (member.IsAllocated);
			}
		}

		const BuildingModelDescriptor * descriptor = Peek_Descriptor (member);
		if (descriptor == nullptr || !descriptor->Communications.Enabled) continue;

		bool online_eligible = !member.Destroyed &&
										(member.IsAllocated || !descriptor->Communications.RequiresBuildingOnline);

		if (online_eligible) {
			if (member.ShroudSensorHandle == 0) {
				member.ShroudSensorHandle = WorldShroudManager::RegisterStrategicSensorSource (
					team,member.Position,descriptor->Communications.VisionRadius,
					descriptor->Communications.RadarRadius,descriptor->Communications.LastKnownRetentionSeconds,true);
			} else {
				WorldShroudManager::UpdateStrategicSensorSource (
					member.ShroudSensorHandle,member.Position,descriptor->Communications.VisionRadius,
					descriptor->Communications.RadarRadius,true);
			}
		} else if (member.ShroudSensorHandle != 0) {
			WorldShroudManager::UnregisterStrategicSensorSource (member.ShroudSensorHandle);
			member.ShroudSensorHandle = 0;
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


int TeamPowerGridService::Get_Powered_Count(int team)
{
	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].InUse && _Members [i].Team == team && !_Members [i].Destroyed && _Members [i].IsAllocated) count++;
	}
	return count;
}

int TeamPowerGridService::Get_Shed_Count(int team)
{
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

int TeamPowerGridService::Get_Member_Count(void)
{
	int count = 0;
	for (int i = 0; i < _Members.Count (); i++) {
		if (_Members [i].InUse) count++;
	}
	return count;
}
