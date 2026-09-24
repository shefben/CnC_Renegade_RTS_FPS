# Persistent Campaign Foundation

V2 roadmap Phase 26 (Section 26). `Code/Combat/persistentcampaign.{h,cpp}`.

## Purpose

An optional strategic layer that launches ordinary deterministic procedural battles without
contaminating standard multiplayer (Section 26's own purpose line). A campaign territory battle
is not a second world-generation path: it is the same `ProceduralWorldManager::Request_World`
contract the main menu's procedural button and the `procedural_world` console command already
use, with the seed and profile chosen by territory state instead of directly by a player.

Nothing in `PersistentCampaignService` does anything meaningful before a campaign has been
created (`Is_Campaign_Active`) -- Section 26, step 10 requires campaign progression to stay
disabled/irrelevant in ordinary standalone multiplayer, and this is the same "nothing runs unless
something explicitly asked" contract `ProceduralWorldManager::Request_World` already uses.

## Schema (26.4)

`PersistentCampaignService::CURRENT_SCHEMA_VERSION = 1`. Every serialized campaign carries a
`schema_version` field. `Deserialize`/`Load_From_File` refuse a `schema_version` newer than the
running build supports rather than silently misreading fields it does not understand, and accept
anything older -- every field already defaults through `nlohmann::json::value`, which is what
reading an older, smaller-shaped save needs. `Apply_Schema_Migrations` (private) is where a real
future schema 2 would add its own `if (from_version < 2) { ... }` block; schema 1 is the first
version this codebase has ever written, so there is no real older shape to migrate from today --
the self check exercises the *mechanism* (accept-older, refuse-newer) with synthetic version
numbers rather than fabricating a historical schema that never existed.

## Territory graph (26.1)

`CampaignTerritoryDescriptor`: id, name, up to 6 adjacent territory ids, owner faction (-1 =
neutral), the territory's own procedural `ProfileHash`/`Seed`, strategic value, and optional
tech/resource modifiers. `PersistentCampaignService::Add_Territory` refuses a duplicate id;
`Find_Territory_By_Id`/`Get_Territory`/`Set_Territory_Owner` are the read/write surface.

## Faction state (26.2)

`CampaignFactionState`: campaign resources, unlocked building/unit bit masks (over the same
building/unit descriptor indices `PurchaseAvailabilityClass` already assigns -- no new identifier
scheme), and territories owned. Up to `MAX_FACTIONS` (4) slots; `Set_Faction_State`/
`Get_Faction_State` bounds-check the faction id.

## Battle launch/result contract (26.3)

`Build_Battle_Launch_Contract` is a pure function: territory + attacking faction's unlocked
options in, `CampaignBattleLaunchContract` out. Objective selection is the one real distinction
Section 26.3 draws: an unclaimed (neutral) territory produces `CAMPAIGN_OBJECTIVE_HOLD_TERRITORY`;
an owned one produces `CAMPAIGN_OBJECTIVE_DESTROY_BASE`.

`Launch_Territory_Battle` is the real walk: it hands the contract's seed/profile to
`ProceduralWorldManager::Request_World`, the same one request queue the menu's procedural button
uses. It does not itself start a level load -- the caller (menu/console flow) does that exactly as
it already does for an ordinary procedural request, so this is not a second bootstrap path.

`Apply_Battle_Result_To_Territory`/`Apply_Battle_Result_To_Faction` are pure: a real winner
(`WinningFaction >= 0`) transfers ownership and credits the winner's resources/territory count; a
draw (`-1`) changes neither. `Apply_Battle_Result` is the real walk against live campaign state:
it looks up the named territory, applies the pure functions, writes the result back, and debits
the previous owner's territory count (never below zero).

## Reproduction/logging (26, step 11)

`Format_Battle_Reproduction_String` produces `territory=<id> seed=<n> profile=<hash>
attacker=<id> defender=<id> objective=<n>` -- the same reproduction-string philosophy Section
27.5/`WorldGenerationManifestService::Format_Reproduction_String` already establishes, so a
specific campaign battle can be pointed at the same way a bad seed can.

## Console

`campaign <create <name>|territory <id> <seed> <profile_hash> <value>|owner <id> <nod|gdi>|
launch <id> <nod|gdi> <nod|gdi>|result <id> <nod|gdi|none> <resources>|save <path>|load <path>|
status>` (`Code/Commando/consolefunction.cpp`, `CampaignConsoleFunctionClass`).

## Self check

`shaderselfcheck campaign` (`Run_Persistent_Campaign`, `Code/Commando/shaderselfcheck.cpp`) covers:
territory add/find/owner including the duplicate-id and unknown-id refusals; faction state
bounds/round-trip; `Build_Battle_Launch_Contract`'s neutral-vs-owned objective selection and
seed/mask carry-through; the pure result-application functions including the draw case; the real
`Apply_Battle_Result` walk including previous-owner debit; and a full `Serialize`/`Deserialize`
round trip plus the schema-migration accept-older/refuse-newer mechanism.

**Not exercised by the self check:** `Save_To_File`/`Load_From_File` themselves. They are thin
`RawFileClass` wrappers around `Serialize`/`Deserialize` (the same `Read_File_To_String` idiom
`proceduralworldprofile.cpp` already uses) -- the self check covers the JSON encode/decode layer
they wrap directly, and no other self check in this codebase performs real disk I/O either, so
this is not a new gap this phase introduced. **Not exercised at all:** `Launch_Territory_Battle`
actually being followed by a real level load producing a real generated world -- the same
disclosed limit every AI Commander phase's `Update` walk has carried since Phase 24, because the
self-check harness has no live level.

**Next exact action** for the user: `campaign create demo`, `campaign territory 1 12345 0 1.0`,
`campaign launch 1 nod gdi`, confirm the printed reproduction string, then trigger the level load
the menu's procedural button normally starts and confirm the world that generates uses seed
`12345`. Then `campaign result 1 nod 100`, `campaign save <path>`, restart, `campaign load <path>`,
and confirm `campaign status` shows territory 1 owned by Nod with the file round-tripped.

## Launch integration (D-001)

`campaign launch` now starts the battle: `Launch_Territory_Battle` calls the launcher Commando
installs at startup, `MainMenuDialogClass::Start_Procedural_Skirmish` -- the same host-map
choice, world request (the territory's seed and profile) and skirmish start the Procedural
Skirmish button uses. It refuses while a game is running. The launched contract is kept as the
pending battle, and the match's `GameOver` resolves it once into `Apply_Battle_Result` with the
winning team. `ResourcesCaptured` is 0: no per-match figure for it exists. Checked in
`shader_campaign`; a real launch is a live check.
