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
 *                     $Archive:: /Commando/Code/Combat/persistentcampaign.cpp                $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "persistentcampaign.h"

#include "proceduralworldmanager.h"
#include "gameeventbus.h"
#include "playertype.h"
#include "wwdebug.h"
#include "rawfile.h"

#include "nlohmann/json.hpp"

using nlohmann::json;


CampaignTerritoryDescriptor::CampaignTerritoryDescriptor(void) :
	TerritoryId (-1),
	Name (""),
	AdjacentCount (0),
	OwnerFaction (-1),
	ProfileHash (0),
	Seed (1),
	StrategicValue (0.0f),
	TechModifier (0.0f),
	ResourceModifier (0.0f)
{
	for (int i = 0; i < MAX_ADJACENT; i++) {
		AdjacentTerritoryIds [i] = -1;
	}
}


CampaignFactionState::CampaignFactionState(void) :
	FactionId (-1),
	CampaignResources (0.0f),
	UnlockedBuildingMask (0),
	UnlockedUnitMask (0),
	TerritoriesOwned (0)
{
}


CampaignBattleLaunchContract::CampaignBattleLaunchContract(void) :
	TerritoryId (-1),
	Seed (1),
	ProfileHash (0),
	ProfilePath (""),
	AttackerFaction (-1),
	DefenderFaction (-1),
	AllowedBuildingMask (0),
	AllowedUnitMask (0),
	Objective (CAMPAIGN_OBJECTIVE_DESTROY_BASE)
{
}


CampaignBattleResult::CampaignBattleResult(void) :
	TerritoryId (-1),
	WinningFaction (-1),
	ResourcesCaptured (0.0f),
	TechCaptured (false)
{
}


bool								PersistentCampaignService::Active = false;
StringClass						PersistentCampaignService::CampaignName ("");
CampaignTerritoryDescriptor	PersistentCampaignService::Territories [PersistentCampaignService::MAX_TERRITORIES];
int								PersistentCampaignService::TerritoryCount = 0;
CampaignFactionState			PersistentCampaignService::Factions [PersistentCampaignService::MAX_FACTIONS];

//	The launched battle a match's end resolves (D-001), and who starts that match.
static PersistentCampaignService::BattleLauncherFunc	_BattleLauncher		= nullptr;
static CampaignBattleLaunchContract							_PendingBattle;
static bool															_HasPendingBattle	= false;
static int															_GameOverToken		= 0;

static void	On_Game_Over(GameOverEventClass & event,void * data);


void PersistentCampaignService::Init(void)
{
	Active = false;
	CampaignName = "";
	TerritoryCount = 0;
	for (int i = 0; i < MAX_TERRITORIES; i++) {
		Territories [i] = CampaignTerritoryDescriptor ();
	}
	for (int i = 0; i < MAX_FACTIONS; i++) {
		Factions [i] = CampaignFactionState ();
		Factions [i].FactionId = i;
	}
}


void PersistentCampaignService::Shutdown(void)
{
	Active = false;
	TerritoryCount = 0;
	Clear_Pending_Battle ();
	if (_GameOverToken != 0) {
		GameEventBus::GameOver.Unregister (_GameOverToken);
		_GameOverToken = 0;
	}
}


bool PersistentCampaignService::Is_Campaign_Active(void)
{
	return Active;
}


bool PersistentCampaignService::Create_Campaign(const char * campaign_name,StringClass * error_out)
{
	if ((campaign_name == nullptr) || (campaign_name [0] == '\0')) {
		if (error_out != nullptr) *error_out = "campaign name required";
		return false;
	}

	Init ();
	CampaignName = campaign_name;
	Active = true;
	return true;
}


bool PersistentCampaignService::Add_Territory(const CampaignTerritoryDescriptor & territory,StringClass * error_out)
{
	if (TerritoryCount >= MAX_TERRITORIES) {
		if (error_out != nullptr) *error_out = "territory limit reached";
		return false;
	}

	CampaignTerritoryDescriptor existing;
	if (Find_Territory_By_Id (territory.TerritoryId,&existing)) {
		if (error_out != nullptr) *error_out = "duplicate territory id";
		return false;
	}

	Territories [TerritoryCount] = territory;
	TerritoryCount++;
	return true;
}


int PersistentCampaignService::Get_Territory_Count(void)
{
	return TerritoryCount;
}


bool PersistentCampaignService::Get_Territory(int index,CampaignTerritoryDescriptor * out)
{
	if ((out == nullptr) || (index < 0) || (index >= TerritoryCount)) return false;
	*out = Territories [index];
	return true;
}


bool PersistentCampaignService::Find_Territory_By_Id(int territory_id,CampaignTerritoryDescriptor * out)
{
	for (int i = 0; i < TerritoryCount; i++) {
		if (Territories [i].TerritoryId == territory_id) {
			if (out != nullptr) *out = Territories [i];
			return true;
		}
	}
	return false;
}


bool PersistentCampaignService::Set_Territory_Owner(int territory_id,int faction_id)
{
	for (int i = 0; i < TerritoryCount; i++) {
		if (Territories [i].TerritoryId == territory_id) {
			Territories [i].OwnerFaction = faction_id;
			return true;
		}
	}
	return false;
}


bool PersistentCampaignService::Get_Faction_State(int faction_id,CampaignFactionState * out)
{
	if ((out == nullptr) || (faction_id < 0) || (faction_id >= MAX_FACTIONS)) return false;
	*out = Factions [faction_id];
	return true;
}


bool PersistentCampaignService::Set_Faction_State(const CampaignFactionState & state)
{
	if ((state.FactionId < 0) || (state.FactionId >= MAX_FACTIONS)) return false;
	Factions [state.FactionId] = state;
	return true;
}


/***********************************************************************************************
 * Build_Battle_Launch_Contract -- Section 26.3.  Pure: territory + attacker state in, contract  *
 *	                              out.  No I/O, no reference to live campaign arrays, so a        *
 *	                              self check can exercise every branch with hand-built structs.   *
 *                                                                                             *
 *	Objective selection: a territory with no owner (-1) is a first claim, so the only coherent    *
 *	objective is to hold it once taken.  An owned, defended territory is fought for by trying to   *
 *	destroy its base.  This mirrors the one distinction Section 26.3 actually draws --            *
 *	"starting base configuration" implies a base exists to fight over only when one already does. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
CampaignBattleLaunchContract PersistentCampaignService::Build_Battle_Launch_Contract(
	const CampaignTerritoryDescriptor & territory,int attacker_faction,int defender_faction,
	const CampaignFactionState & attacker_state)
{
	CampaignBattleLaunchContract contract;

	contract.TerritoryId			= territory.TerritoryId;
	contract.Seed					= territory.Seed;
	contract.ProfileHash			= territory.ProfileHash;
	contract.AttackerFaction	= attacker_faction;
	contract.DefenderFaction	= defender_faction;

	contract.AllowedBuildingMask	= attacker_state.UnlockedBuildingMask;
	contract.AllowedUnitMask		= attacker_state.UnlockedUnitMask;

	contract.Objective = (territory.OwnerFaction < 0) ?
		CAMPAIGN_OBJECTIVE_HOLD_TERRITORY : CAMPAIGN_OBJECTIVE_DESTROY_BASE;

	return contract;
}


bool PersistentCampaignService::Launch_Territory_Battle(const CampaignBattleLaunchContract & contract,StringClass * error_out)
{
	if (!Active) {
		if (error_out != nullptr) *error_out = "no active campaign";
		return false;
	}

	CampaignTerritoryDescriptor territory;
	if (!Find_Territory_By_Id (contract.TerritoryId,&territory)) {
		if (error_out != nullptr) *error_out = "unknown territory id";
		return false;
	}

	const char * profile_path = contract.ProfilePath.Is_Empty () ? nullptr : (const char *)contract.ProfilePath;

	bool launched = (_BattleLauncher != nullptr)
		? _BattleLauncher (contract.Seed,profile_path,error_out)
		: ProceduralWorldManager::Request_World (contract.Seed,profile_path,error_out);

	if (launched) {
		_PendingBattle		= contract;
		_HasPendingBattle	= true;
		if (_GameOverToken == 0) {
			_GameOverToken = GameEventBus::GameOver.Register (On_Game_Over);
		}
	}
	return launched;
}


void PersistentCampaignService::Set_Battle_Launcher(BattleLauncherFunc launcher)
{
	_BattleLauncher = launcher;
}


bool PersistentCampaignService::Has_Pending_Battle(void)
{
	return _HasPendingBattle;
}


bool PersistentCampaignService::Get_Pending_Battle(CampaignBattleLaunchContract * out)
{
	if (!_HasPendingBattle) return false;
	if (out != nullptr) *out = _PendingBattle;
	return true;
}


void PersistentCampaignService::Clear_Pending_Battle(void)
{
	_HasPendingBattle = false;
	_PendingBattle = CampaignBattleLaunchContract ();
}


bool PersistentCampaignService::Resolve_Pending_Battle(int winning_team,StringClass * error_out)
{
	if (!_HasPendingBattle) {
		if (error_out != nullptr) *error_out = "no pending campaign battle";
		return false;
	}

	CampaignBattleResult result;
	result.TerritoryId			= _PendingBattle.TerritoryId;
	result.WinningFaction		= ((winning_team == PLAYERTYPE_GDI) || (winning_team == PLAYERTYPE_NOD)) ? winning_team : -1;
	result.ResourcesCaptured	= 0.0f;

	//	Cleared first, so a result that fails to apply is not applied again by a later game over.
	Clear_Pending_Battle ();
	return Apply_Battle_Result (result,error_out);
}


static void	On_Game_Over(GameOverEventClass & event,void * /* data */)
{
	if (!PersistentCampaignService::Has_Pending_Battle ()) return;

	StringClass error;
	if (!PersistentCampaignService::Resolve_Pending_Battle (event.WinnerTeam,&error)) {
		WWDEBUG_SAY (("PersistentCampaignService: the finished battle did not apply: %s\n",error.Peek_Buffer ()));
	}
}


/***********************************************************************************************
 * Apply_Battle_Result_To_Territory -- Section 26, step 7.  Pure: ownership transfers to the      *
 *	                                  winner; a draw/inconclusive result (-1) leaves ownership     *
 *	                                  untouched rather than guessing a side.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
CampaignTerritoryDescriptor PersistentCampaignService::Apply_Battle_Result_To_Territory(
	const CampaignTerritoryDescriptor & territory,const CampaignBattleResult & result)
{
	CampaignTerritoryDescriptor updated = territory;
	if (result.WinningFaction >= 0) {
		updated.OwnerFaction = result.WinningFaction;
	}
	return updated;
}


CampaignFactionState PersistentCampaignService::Apply_Battle_Result_To_Faction(
	const CampaignFactionState & faction,const CampaignBattleResult & result,bool faction_won)
{
	CampaignFactionState updated = faction;
	if (faction_won) {
		updated.CampaignResources += result.ResourcesCaptured;
		updated.TerritoriesOwned++;
	}
	return updated;
}


bool PersistentCampaignService::Apply_Battle_Result(const CampaignBattleResult & result,StringClass * error_out)
{
	if (!Active) {
		if (error_out != nullptr) *error_out = "no active campaign";
		return false;
	}

	CampaignTerritoryDescriptor territory;
	if (!Find_Territory_By_Id (result.TerritoryId,&territory)) {
		if (error_out != nullptr) *error_out = "unknown territory id";
		return false;
	}

	int previous_owner = territory.OwnerFaction;
	CampaignTerritoryDescriptor updated_territory = Apply_Battle_Result_To_Territory (territory,result);
	Set_Territory_Owner (updated_territory.TerritoryId,updated_territory.OwnerFaction);

	if (result.WinningFaction >= 0) {
		CampaignFactionState winner;
		if (Get_Faction_State (result.WinningFaction,&winner)) {
			winner = Apply_Battle_Result_To_Faction (winner,result,true);
			Set_Faction_State (winner);
		}

		if ((previous_owner >= 0) && (previous_owner != result.WinningFaction)) {
			CampaignFactionState loser;
			if (Get_Faction_State (previous_owner,&loser)) {
				loser.TerritoriesOwned--;
				if (loser.TerritoriesOwned < 0) loser.TerritoriesOwned = 0;
				Set_Faction_State (loser);
			}
		}
	}

	return true;
}


bool PersistentCampaignService::Serialize(StringClass * out_json)
{
	if (out_json == nullptr) return false;

	json root;
	root ["schema_version"]	= CURRENT_SCHEMA_VERSION;
	root ["campaign_name"]		= (const char *)CampaignName;
	root ["active"]				= Active;

	json territories = json::array ();
	for (int i = 0; i < TerritoryCount; i++) {
		const CampaignTerritoryDescriptor & t = Territories [i];
		json jt;
		jt ["territory_id"]			= t.TerritoryId;
		jt ["name"]						= (const char *)t.Name;
		jt ["owner_faction"]			= t.OwnerFaction;
		jt ["profile_hash"]			= t.ProfileHash;
		jt ["seed"]						= t.Seed;
		jt ["strategic_value"]		= t.StrategicValue;
		jt ["tech_modifier"]			= t.TechModifier;
		jt ["resource_modifier"]	= t.ResourceModifier;

		json adjacent = json::array ();
		for (int a = 0; a < t.AdjacentCount; a++) {
			adjacent.push_back (t.AdjacentTerritoryIds [a]);
		}
		jt ["adjacent_territory_ids"] = adjacent;

		territories.push_back (jt);
	}
	root ["territories"] = territories;

	json factions = json::array ();
	for (int i = 0; i < MAX_FACTIONS; i++) {
		const CampaignFactionState & f = Factions [i];
		json jf;
		jf ["faction_id"]					= f.FactionId;
		jf ["campaign_resources"]		= f.CampaignResources;
		jf ["unlocked_building_mask"]	= f.UnlockedBuildingMask;
		jf ["unlocked_unit_mask"]		= f.UnlockedUnitMask;
		jf ["territories_owned"]		= f.TerritoriesOwned;
		factions.push_back (jf);
	}
	root ["factions"] = factions;

	*out_json = root.dump ().c_str ();
	return true;
}


/***********************************************************************************************
 * Apply_Schema_Migrations -- Section 26.4's explicit migration path.                             *
 *                                                                                             *
 *	Schema 1 is the first version this codebase has ever written, so there is no real older       *
 *	shape to migrate from today.  What is real and testable now is the mechanism itself: a         *
 *	`from_version` older than current is accepted (every field access below already defaults       *
 *	missing keys via json::value, which is what an old-schema-with-fewer-fields load needs), and    *
 *	a `from_version` newer than CURRENT_SCHEMA_VERSION is refused outright rather than silently     *
 *	misreading fields this build does not understand.  A future schema 2 adds its own              *
 *	`if (from_version < 2) { ... }` block here.                                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool PersistentCampaignService::Apply_Schema_Migrations(void * json_root_void,unsigned int from_version,StringClass * error_out)
{
	(void)json_root_void;

	if (from_version > CURRENT_SCHEMA_VERSION) {
		if (error_out != nullptr) {
			error_out->Format ("campaign save schema_version %u is newer than this build supports (%u)",
									  from_version,CURRENT_SCHEMA_VERSION);
		}
		return false;
	}

	//	from_version < CURRENT_SCHEMA_VERSION: no field-shape changes exist yet between any
	//	shipped version and schema 1, so there is nothing further to transform.

	return true;
}


bool PersistentCampaignService::Deserialize(const char * json_text,StringClass * error_out)
{
	if (json_text == nullptr) {
		if (error_out != nullptr) *error_out = "null argument";
		return false;
	}

	try {
		json root = json::parse (json_text);

		unsigned int from_version = root.value ("schema_version",CURRENT_SCHEMA_VERSION);
		if (!Apply_Schema_Migrations (&root,from_version,error_out)) {
			return false;
		}

		Init ();
		CampaignName	= root.value ("campaign_name",std::string ("")).c_str ();
		Active			= root.value ("active",false);

		if (root.contains ("territories") && root ["territories"].is_array ()) {
			for (auto & jt : root ["territories"]) {
				if (TerritoryCount >= MAX_TERRITORIES) break;

				CampaignTerritoryDescriptor t;
				t.TerritoryId			= jt.value ("territory_id",-1);
				t.Name					= jt.value ("name",std::string ("")).c_str ();
				t.OwnerFaction			= jt.value ("owner_faction",-1);
				t.ProfileHash			= jt.value ("profile_hash",0u);
				t.Seed					= jt.value ("seed",1u);
				t.StrategicValue		= jt.value ("strategic_value",0.0f);
				t.TechModifier			= jt.value ("tech_modifier",0.0f);
				t.ResourceModifier	= jt.value ("resource_modifier",0.0f);

				t.AdjacentCount = 0;
				if (jt.contains ("adjacent_territory_ids") && jt ["adjacent_territory_ids"].is_array ()) {
					for (auto & aid : jt ["adjacent_territory_ids"]) {
						if (t.AdjacentCount >= CampaignTerritoryDescriptor::MAX_ADJACENT) break;
						t.AdjacentTerritoryIds [t.AdjacentCount] = aid.get<int> ();
						t.AdjacentCount++;
					}
				}

				Territories [TerritoryCount] = t;
				TerritoryCount++;
			}
		}

		if (root.contains ("factions") && root ["factions"].is_array ()) {
			for (auto & jf : root ["factions"]) {
				int faction_id = jf.value ("faction_id",-1);
				if ((faction_id < 0) || (faction_id >= MAX_FACTIONS)) continue;

				CampaignFactionState f;
				f.FactionId				= faction_id;
				f.CampaignResources		= jf.value ("campaign_resources",0.0f);
				f.UnlockedBuildingMask	= jf.value ("unlocked_building_mask",0u);
				f.UnlockedUnitMask		= jf.value ("unlocked_unit_mask",0u);
				f.TerritoriesOwned		= jf.value ("territories_owned",0);

				Factions [faction_id] = f;
			}
		}

	} catch (json::exception & e) {
		if (error_out != nullptr) *error_out = e.what ();
		return false;
	}

	return true;
}


static bool Read_File_To_String(const char * file_path,StringClass * out)
{
	RawFileClass file (file_path);
	if (!file.Open (FileClass::READ)) {
		return false;
	}
	int size = file.Size ();
	if (size <= 0) {
		file.Close ();
		return false;
	}

	char * buffer = new char [size + 1];
	int read = file.Read (buffer,size);
	buffer [read] = '\0';
	file.Close ();

	*out = buffer;
	delete [] buffer;
	return true;
}


bool PersistentCampaignService::Save_To_File(const char * file_path,StringClass * error_out)
{
	if ((file_path == nullptr) || (file_path [0] == '\0')) {
		if (error_out != nullptr) *error_out = "null file path";
		return false;
	}

	StringClass json_text;
	if (!Serialize (&json_text)) {
		if (error_out != nullptr) *error_out = "serialize failed";
		return false;
	}

	RawFileClass file (file_path);
	if (!file.Open (FileClass::WRITE)) {
		if (error_out != nullptr) error_out->Format ("could not open '%s' for write",file_path);
		return false;
	}
	file.Write ((const char *)json_text,(int)json_text.Get_Length ());
	file.Close ();
	return true;
}


bool PersistentCampaignService::Load_From_File(const char * file_path,StringClass * error_out)
{
	if ((file_path == nullptr) || (file_path [0] == '\0')) {
		if (error_out != nullptr) *error_out = "null file path";
		return false;
	}

	StringClass json_text;
	if (!Read_File_To_String (file_path,&json_text)) {
		if (error_out != nullptr) error_out->Format ("could not read '%s'",file_path);
		return false;
	}

	return Deserialize ((const char *)json_text,error_out);
}


StringClass PersistentCampaignService::Format_Battle_Reproduction_String(const CampaignBattleLaunchContract & contract)
{
	StringClass s (160u,true);
	s.Format ("territory=%d seed=%u profile=%u attacker=%d defender=%d objective=%d",
				 contract.TerritoryId,contract.Seed,contract.ProfileHash,
				 contract.AttackerFaction,contract.DefenderFaction,(int)contract.Objective);
	return s;
}
