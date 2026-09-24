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
 *                     $Archive:: /Commando/Code/Combat/persistentcampaign.h                  $*
 *                                                                                             *
 *	V2 roadmap Phase 26 -- persistent campaign foundation (Section 26).  An optional strategic  *
 *	layer that launches ordinary deterministic procedural battles without contaminating         *
 *	standard multiplayer.                                                                       *
 *                                                                                             *
 *	Battle launch reuses ProceduralWorldManager::Request_World exactly as the main menu's        *
 *	procedural button and the `procedural_world` console command already do (see                *
 *	proceduralworldmanager.h) -- a campaign territory battle is not a second world-generation     *
 *	path, it is the same one-request/one-generation contract with its seed and profile chosen     *
 *	by territory state instead of by a player picking them directly.  Section 26.2 says do not    *
 *	make ordinary multiplayer depend on campaign persistence, so nothing here runs unless a       *
 *	campaign has actually been created (Is_Campaign_Active), mirroring Request_World's own         *
 *	"nothing runs unless something explicitly asked" contract.                                    *
 *                                                                                             *
 *	Decision logic (Build_Battle_Launch_Contract, Apply_Battle_Result_To_Territory/_Faction) is    *
 *	pure functions of caller-supplied structs, the same split every Commander AI phase has used    *
 *	since Phase 24 -- self-check-testable with no live level.  Launch_Territory_Battle and          *
 *	Apply_Battle_Result are the real walk against live campaign state and, for the former, the      *
 *	live ProceduralWorldManager request queue -- untestable in the self-check harness for the       *
 *	same disclosed reason Phase 24's Update is.                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PERSISTENTCAMPAIGN_H
#define PERSISTENTCAMPAIGN_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "wwstring.h"


//	Section 26.3's "special objectives".  Kept small and closed -- new values are added only
//	when a concrete campaign design actually needs them, never speculatively.
typedef enum
{
	CAMPAIGN_OBJECTIVE_DESTROY_BASE			= 0,
	CAMPAIGN_OBJECTIVE_HOLD_TERRITORY,
	CAMPAIGN_OBJECTIVE_ELIMINATE_FORCES,
} CampaignObjectiveType;


//	Section 26.1's territory/node structure.  `OwnerFaction` of -1 means unclaimed/neutral.
//	`ProfileHash`/`Seed` are the territory's own procedural identity -- the same battle fought
//	over the same territory reproduces the same starting world, exactly as Section 26.3 asks.
struct CampaignTerritoryDescriptor
{
	CampaignTerritoryDescriptor(void);

	int				TerritoryId;
	StringClass		Name;

	enum { MAX_ADJACENT = 6 };
	int				AdjacentTerritoryIds [MAX_ADJACENT];
	int				AdjacentCount;

	int				OwnerFaction;				// -1 = neutral/unclaimed
	unsigned int	ProfileHash;
	unsigned int	Seed;

	float				StrategicValue;
	float				TechModifier;				// Section 26.1's optional tech/resource bonus
	float				ResourceModifier;
};


//	Section 26.2's persistent faction state.  `UnlockedBuildingMask`/`UnlockedUnitMask` are bit
//	masks over the same building/unit descriptor indices PurchaseAvailability already assigns --
//	no new identifier scheme, just which of the existing ones this faction has unlocked.
struct CampaignFactionState
{
	CampaignFactionState(void);

	int				FactionId;
	float				CampaignResources;
	unsigned int	UnlockedBuildingMask;
	unsigned int	UnlockedUnitMask;
	int				TerritoriesOwned;
};


//	Section 26.3's battle-launch contract.  Everything a battle needs to start, derived once
//	from campaign state and hereafter treated as an ordinary procedural-world request.
struct CampaignBattleLaunchContract
{
	CampaignBattleLaunchContract(void);

	int						TerritoryId;
	unsigned int			Seed;
	unsigned int			ProfileHash;
	StringClass				ProfilePath;			// empty = ProceduralWorldManager::DEFAULT_PROFILE_PATH

	int						AttackerFaction;
	int						DefenderFaction;

	unsigned int			AllowedBuildingMask;
	unsigned int			AllowedUnitMask;
	CampaignObjectiveType	Objective;
};


//	The one summary a completed battle produces (Section 26, step 6).
struct CampaignBattleResult
{
	CampaignBattleResult(void);

	int		TerritoryId;
	int		WinningFaction;			// -1 = draw/inconclusive
	float		ResourcesCaptured;
	bool		TechCaptured;
};


class PersistentCampaignService
{
	public:

		//	Section 26.4.  Bumped whenever a saved field's meaning changes; Deserialize/Load_From_File
		//	reject a file whose schema_version is newer than this build understands and apply
		//	Apply_Schema_Migrations for anything older.
		static const unsigned int	CURRENT_SCHEMA_VERSION = 1;

		enum { MAX_TERRITORIES = 64 };
		enum { MAX_FACTIONS = 4 };

		static void		Init(void);
		static void		Shutdown(void);

		//	Section 26, step 10: campaign progression stays disabled/irrelevant in ordinary
		//	standalone multiplayer.  Nothing else in this class does anything meaningful before
		//	Create_Campaign (or a successful load) has run.
		static bool		Is_Campaign_Active(void);
		static bool		Create_Campaign(const char * campaign_name,StringClass * error_out);

		static bool		Add_Territory(const CampaignTerritoryDescriptor & territory,StringClass * error_out);
		static int		Get_Territory_Count(void);
		static bool		Get_Territory(int index,CampaignTerritoryDescriptor * out);
		static bool		Find_Territory_By_Id(int territory_id,CampaignTerritoryDescriptor * out);
		static bool		Set_Territory_Owner(int territory_id,int faction_id);

		static bool		Get_Faction_State(int faction_id,CampaignFactionState * out);
		static bool		Set_Faction_State(const CampaignFactionState & state);

		//	Pure.  Builds the launch contract for one territory battle from the territory's own
		//	procedural identity and the attacking faction's unlocked options -- no I/O, no global
		//	state, self-check-testable.
		static CampaignBattleLaunchContract	Build_Battle_Launch_Contract(
			const CampaignTerritoryDescriptor & territory,int attacker_faction,int defender_faction,
			const CampaignFactionState & attacker_state);

		//	Real walk.  Hands the contract's seed/profile to ProceduralWorldManager::Request_World,
		//	the same one request queue the menu's procedural button uses -- see proceduralworldmanager.h.
		//	Does not itself start a level load; the caller (menu/console flow) does that exactly as
		//	it already does for an ordinary procedural request.  Untestable in the self-check
		//	harness for the same disclosed reason CommanderAIService::Update is (Phase 24).
		static bool		Launch_Territory_Battle(const CampaignBattleLaunchContract & contract,StringClass * error_out);

		/*
		**	D-001 / V2 Phase 26: the launch reaches a match.  The launcher is Commando's Procedural
		**	Skirmish path -- the same host-map choice, world request and skirmish start the main
		**	menu button uses -- installed at startup, since Combat cannot start a game itself.  With
		**	none installed (a self-check, a dedicated tool) a launch only places the world request.
		**
		**	The launched contract is kept as the pending battle -- territory, factions, seed and
		**	profile -- so the match's end resolves it: GameOver's winning team becomes the
		**	CampaignBattleResult Apply_Battle_Result takes, once.  ResourcesCaptured is 0: no
		**	per-match figure for it exists in the engine.
		*/
		typedef bool (*BattleLauncherFunc)(unsigned int seed,const char * profile_path,StringClass * error_out);
		static void		Set_Battle_Launcher(BattleLauncherFunc launcher);
		static bool		Has_Pending_Battle(void);
		static bool		Get_Pending_Battle(CampaignBattleLaunchContract * out);
		static void		Clear_Pending_Battle(void);
		static bool		Resolve_Pending_Battle(int winning_team,StringClass * error_out);

		//	Pure.  Applies one battle result onto a territory/faction snapshot and returns the
		//	updated copy; does not touch live campaign state.
		static CampaignTerritoryDescriptor	Apply_Battle_Result_To_Territory(
			const CampaignTerritoryDescriptor & territory,const CampaignBattleResult & result);
		static CampaignFactionState	Apply_Battle_Result_To_Faction(
			const CampaignFactionState & faction,const CampaignBattleResult & result,bool faction_won);

		//	Real walk.  Looks up the live territory/factions the result names and writes the pure
		//	functions' output back into live campaign state.
		static bool		Apply_Battle_Result(const CampaignBattleResult & result,StringClass * error_out);

		static bool		Serialize(StringClass * out_json);
		static bool		Deserialize(const char * json_text,StringClass * error_out);

		static bool		Save_To_File(const char * file_path,StringClass * error_out);
		static bool		Load_From_File(const char * file_path,StringClass * error_out);

		//	Section 26, step 11: campaign reproduction/logging.  The same "generator_version=<n>
		//	seed=<n> profile=<hash> content_manifest=<hash>" shape Section 27.5/WorldGenerationManifestService::
		//	Format_Reproduction_String already defines, plus the territory/faction ids, so a
		//	specific campaign battle can be pointed at the same way a bad seed can.
		static StringClass		Format_Battle_Reproduction_String(const CampaignBattleLaunchContract & contract);

	private:

		//	Section 26.4's explicit migration path.  `root` is mutated in place so every field read
		//	afterward sees current-schema names/defaults regardless of `from_version`.  Currently a
		//	no-op chain (schema 1 is the first version this codebase has ever written) but wired so
		//	a future schema 2 adds one `if (from_version < 2)` block here rather than inventing a
		//	second migration mechanism.
		static bool		Apply_Schema_Migrations(void * json_root_void,unsigned int from_version,StringClass * error_out);

		static bool				Active;
		static StringClass		CampaignName;

		static CampaignTerritoryDescriptor	Territories [MAX_TERRITORIES];
		static int									TerritoryCount;

		static CampaignFactionState	Factions [MAX_FACTIONS];
};

#endif	// PERSISTENTCAMPAIGN_H
