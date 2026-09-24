/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/rtspersistence.cpp                    $*
 *                                                                                             *
 *	Roadmap Section 46, Phase 40 -- save/load and networking requirements for the RTS layer.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "rtspersistence.h"

#include "buildingconstruction.h"
#include "combatchunkid.h"
#include "commanderproduction.h"
#include "playertype.h"
#include "teampowergridservice.h"
#include "wwdebug.h"

#include "chunkio.h"
#include "saveloadids.h"


RTSPersistenceClass	_TheRTSPersistenceSaveSystem;


enum
{
	CHUNKID_POWER_OVERRIDES		= 0x11250800,
	CHUNKID_CONSTRUCTION,
	CHUNKID_PRODUCTION_QUEUE,
	CHUNKID_COMMANDER_ORDERS,
	CHUNKID_COMMANDERS,
};

enum
{
	VARID_MEMBER_ID = 0x01,
	VARID_MANUAL_DISABLED,
	VARID_PRIORITY_OVERRIDE,

	VARID_BUILDING_ID,
	VARID_DESCRIPTOR_NAME,
	VARID_TRANSFORM,
	VARID_START_TIME,
	VARID_DURATION,
	VARID_SEED,
	VARID_BOUNDS_CENTER,
	VARID_BOUNDS_EXTENT,

	VARID_FACILITY_ID,
	VARID_DEFINITION_ID,
	VARID_TEAM,
	VARID_COST,
	VARID_BUILD_TIME,
	VARID_ELAPSED,
	VARID_KIND,
	VARID_TICKET_ID,

	VARID_UNIT_ID,
	VARID_ORDER,
	VARID_TARGET_POINT,
	VARID_TARGET_ID,

	VARID_COMMANDER_TEAM,
	VARID_COMMANDER_PLAYER,

	VARID_REQUESTER_ID,		// appended: a save written before it restores RequesterId 0 (no refund owed)
};

static int	_LoadFailures		= 0;
static int	_LateJoinCount		= 0;


uint32 RTSPersistenceClass::Chunk_ID(void) const
{
	return CHUNKID_RTS_PERSISTENCE;
}

const char * RTSPersistenceClass::Name(void) const
{
	return "RTSPersistenceClass";
}


bool RTSPersistenceClass::Contains_Data(void) const
{
	return Has_Data ();
}

bool RTSPersistenceClass::Has_Data(void)
{
	//	A match that never entered Commander Mode writes nothing at all, so an ordinary FPS save
	//	file is byte-for-byte what it was before this phase existed.
	return (TeamPowerGridService::Get_Member_Count () > 0) ||
			 (BuildingConstructionService::Get_Active_Count () > 0) ||
			 (CommanderProductionService::Get_Item_Count () > 0) ||
			 (CommanderProductionService::Get_Order_Count () > 0);
}


bool RTSPersistenceClass::Save(ChunkSaveClass & csave)
{
	/*
	**	Power grid.  Only the two Commander-authored facts per member; everything the allocator
	**	derived is recomputed in On_Post_Load.  A member with neither a manual disable nor a
	**	priority override writes no chunk at all -- the overwhelmingly common case, so a base
	**	full of untouched buildings costs nothing.
	*/
	int member_count = TeamPowerGridService::Get_Member_Count ();
	for (int i = 0; i < member_count; i++) {

		PowerGridMemberIdType member_id = 0;
		int team = PLAYERTYPE_NEUTRAL;
		if (!TeamPowerGridService::Get_Member_By_Index (i,&member_id,&team)) continue;

		bool manual_disabled	= TeamPowerGridService::Is_Manual_Disabled (member_id);
		int priority			= 0;
		bool has_override		= TeamPowerGridService::Get_Priority_Override (member_id,&priority);

		if (!manual_disabled && !has_override) continue;

		csave.Begin_Chunk (CHUNKID_POWER_OVERRIDES);
			WRITE_MICRO_CHUNK (csave,VARID_MEMBER_ID,			member_id);
			WRITE_MICRO_CHUNK (csave,VARID_MANUAL_DISABLED,	manual_disabled);
			if (has_override) {
				WRITE_MICRO_CHUNK (csave,VARID_PRIORITY_OVERRIDE,priority);
			}
		csave.End_Chunk ();
	}

	/*
	**	Construction.  The compact replication state plus the bounds the layout came from.  No
	**	block, no mesh, no progress -- progress is (server time - start time) / duration, and
	**	writing it as well would be a second truth that could disagree with the clock.
	*/
	int construction_count = BuildingConstructionService::Get_Active_Count ();
	for (int i = 0; i < construction_count; i++) {

		int building_id = BuildingConstructionService::Get_Active_Building (i);
		if (building_id == 0) continue;

		BuildingConstructionReplicationState state;
		if (!BuildingConstructionService::Get_Replication_State (building_id,&state)) continue;

		AABoxClass bounds;
		if (!BuildingConstructionService::Get_Model_Bounds (building_id,&bounds)) continue;

		csave.Begin_Chunk (CHUNKID_CONSTRUCTION);
			WRITE_MICRO_CHUNK (csave,VARID_BUILDING_ID,		state.BuildingId);
			WRITE_MICRO_CHUNK_WWSTRING (csave,VARID_DESCRIPTOR_NAME,state.DescriptorName);
			WRITE_MICRO_CHUNK (csave,VARID_TRANSFORM,			state.Transform);
			WRITE_MICRO_CHUNK (csave,VARID_START_TIME,		state.StartTimeSeconds);
			WRITE_MICRO_CHUNK (csave,VARID_DURATION,			state.DurationSeconds);
			WRITE_MICRO_CHUNK (csave,VARID_SEED,				state.Seed);
			WRITE_MICRO_CHUNK (csave,VARID_BOUNDS_CENTER,	bounds.Center);
			WRITE_MICRO_CHUNK (csave,VARID_BOUNDS_EXTENT,	bounds.Extent);
		csave.End_Chunk ();
	}

	//	Production queues, in request order -- the order is the data, so the chunks go out in
	//	the order the items are held and come back the same way.
	int item_count = CommanderProductionService::Get_Item_Count ();
	for (int i = 0; i < item_count; i++) {

		CommanderProductionItem item;
		if (!CommanderProductionService::Get_Item (i,&item)) continue;

		csave.Begin_Chunk (CHUNKID_PRODUCTION_QUEUE);
			WRITE_MICRO_CHUNK (csave,VARID_FACILITY_ID,		item.FacilityId);
			WRITE_MICRO_CHUNK (csave,VARID_DEFINITION_ID,	item.DefinitionId);
			WRITE_MICRO_CHUNK (csave,VARID_TEAM,				item.Team);
			WRITE_MICRO_CHUNK (csave,VARID_COST,				item.Cost);
			WRITE_MICRO_CHUNK (csave,VARID_BUILD_TIME,		item.BuildTimeSeconds);
			WRITE_MICRO_CHUNK (csave,VARID_ELAPSED,			item.ElapsedSeconds);
			int kind = (int)item.Kind;
			WRITE_MICRO_CHUNK (csave,VARID_KIND,				kind);
			WRITE_MICRO_CHUNK (csave,VARID_TICKET_ID,			item.TicketId);
			WRITE_MICRO_CHUNK (csave,VARID_REQUESTER_ID,		item.RequesterId);
		csave.End_Chunk ();
	}

	//	Durable Commander orders.
	int order_count = CommanderProductionService::Get_Order_Count ();
	for (int i = 0; i < order_count; i++) {

		int unit_id = 0;
		CommanderUnitOrderType order = ORDER_NONE;
		Vector3 point (0.0f,0.0f,0.0f);
		int target_id = 0;
		if (!CommanderProductionService::Get_Order_Record (i,&unit_id,&order,&point,&target_id)) continue;

		csave.Begin_Chunk (CHUNKID_COMMANDER_ORDERS);
			WRITE_MICRO_CHUNK (csave,VARID_UNIT_ID,		unit_id);
			int order_value = (int)order;
			WRITE_MICRO_CHUNK (csave,VARID_ORDER,			order_value);
			WRITE_MICRO_CHUNK (csave,VARID_TARGET_POINT,	point);
			WRITE_MICRO_CHUNK (csave,VARID_TARGET_ID,		target_id);
		csave.End_Chunk ();
	}

	//	"Commander identity/permission where required."
	int teams [2] = { PLAYERTYPE_NOD,PLAYERTYPE_GDI };
	for (int i = 0; i < 2; i++) {
		int player = CommanderProductionService::Get_Team_Commander (teams [i]);
		if (player == 0) continue;

		csave.Begin_Chunk (CHUNKID_COMMANDERS);
			WRITE_MICRO_CHUNK (csave,VARID_COMMANDER_TEAM,	teams [i]);
			WRITE_MICRO_CHUNK (csave,VARID_COMMANDER_PLAYER,player);
		csave.End_Chunk ();
	}

	return true;
}


bool RTSPersistenceClass::Load(ChunkLoadClass & cload)
{
	_LoadFailures	= 0;
	_LateJoinCount	= 0;

	while (cload.Open_Chunk ()) {

		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_POWER_OVERRIDES:
			{
				PowerGridMemberIdType member_id = 0;
				bool manual_disabled = false;
				int priority = 0;
				bool has_override = false;

				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {
						READ_MICRO_CHUNK (cload,VARID_MEMBER_ID,			member_id);
						READ_MICRO_CHUNK (cload,VARID_MANUAL_DISABLED,	manual_disabled);
						case VARID_PRIORITY_OVERRIDE:
							cload.Read (&priority,sizeof (priority));
							has_override = true;
							break;
					}
					cload.Close_Micro_Chunk ();
				}

				/*
				**	A member id that no longer exists is not an error: the building it named may
				**	have been destroyed by the same save's game-object state, and a manual
				**	override on a dead building is meaningless.  Counted so a diagnostic can see
				**	it, never fatal.
				*/
				if (!TeamPowerGridService::Set_Manual_Disabled (member_id,manual_disabled)) {
					_LoadFailures++;
					break;
				}
				if (has_override) {
					TeamPowerGridService::Set_Priority_Override (member_id,priority);
				}
				break;
			}

			case CHUNKID_CONSTRUCTION:
			{
				BuildingConstructionReplicationState state;
				AABoxClass bounds;
				bounds.Center = Vector3 (0.0f,0.0f,0.0f);
				bounds.Extent = Vector3 (0.0f,0.0f,0.0f);

				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {
						READ_MICRO_CHUNK (cload,VARID_BUILDING_ID,	state.BuildingId);
						READ_MICRO_CHUNK_WWSTRING (cload,VARID_DESCRIPTOR_NAME,state.DescriptorName);
						READ_MICRO_CHUNK (cload,VARID_TRANSFORM,		state.Transform);
						READ_MICRO_CHUNK (cload,VARID_START_TIME,		state.StartTimeSeconds);
						READ_MICRO_CHUNK (cload,VARID_DURATION,		state.DurationSeconds);
						READ_MICRO_CHUNK (cload,VARID_SEED,				state.Seed);
						READ_MICRO_CHUNK (cload,VARID_BOUNDS_CENTER,	bounds.Center);
						READ_MICRO_CHUNK (cload,VARID_BOUNDS_EXTENT,	bounds.Extent);
					}
					cload.Close_Micro_Chunk ();
				}

				state.Active = true;

				//	Regenerates the identical block layout from (bounds, seed) -- the same call a
				//	late joiner makes, which is what makes "reconstruct procedural block meshes
				//	locally" one code path rather than two.
				if (!BuildingConstructionService::Begin_Construction_From_State (state,bounds)) {
					//	Almost always a descriptor that no longer loads.  Section 46: "report
					//	incompatible material changes clearly."
					WWDEBUG_SAY (("RTSPersistence: could not restore construction for building %d "
									  "(descriptor '%s')\n",state.BuildingId,
									  state.DescriptorName.Peek_Buffer ()));
					_LoadFailures++;
				} else {
					_LateJoinCount++;
				}
				break;
			}

			case CHUNKID_PRODUCTION_QUEUE:
			{
				CommanderProductionItem item;
				int kind = 0;

				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {
						READ_MICRO_CHUNK (cload,VARID_FACILITY_ID,	item.FacilityId);
						READ_MICRO_CHUNK (cload,VARID_DEFINITION_ID,	item.DefinitionId);
						READ_MICRO_CHUNK (cload,VARID_TEAM,				item.Team);
						READ_MICRO_CHUNK (cload,VARID_COST,				item.Cost);
						READ_MICRO_CHUNK (cload,VARID_BUILD_TIME,		item.BuildTimeSeconds);
						READ_MICRO_CHUNK (cload,VARID_ELAPSED,			item.ElapsedSeconds);
						READ_MICRO_CHUNK (cload,VARID_KIND,				kind);
						READ_MICRO_CHUNK (cload,VARID_TICKET_ID,		item.TicketId);
						READ_MICRO_CHUNK (cload,VARID_REQUESTER_ID,	item.RequesterId);
					}
					cload.Close_Micro_Chunk ();
				}

				if (kind < 0 || kind > (int)BUILDING_PRODUCTION_AIR_VEHICLE) kind = 0;
				item.Kind	= (BuildingProductionKindType)kind;

				//	Restored as QUEUED whatever it was: PRODUCING, WAITING_FOR_POWER and
				//	BLOCKED_EXIT are all conclusions about a world that has not been rebuilt yet,
				//	and the next Update re-reaches whichever of them is true now.
				item.State	= PRODUCTION_STATE_QUEUED;

				CommanderProductionService::Restore_Item (item);
				break;
			}

			case CHUNKID_COMMANDER_ORDERS:
			{
				int unit_id = 0;
				int order_value = 0;
				Vector3 point (0.0f,0.0f,0.0f);
				int target_id = 0;

				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {
						READ_MICRO_CHUNK (cload,VARID_UNIT_ID,		unit_id);
						READ_MICRO_CHUNK (cload,VARID_ORDER,			order_value);
						READ_MICRO_CHUNK (cload,VARID_TARGET_POINT,	point);
						READ_MICRO_CHUNK (cload,VARID_TARGET_ID,		target_id);
					}
					cload.Close_Micro_Chunk ();
				}

				if (order_value < 0 || order_value >= (int)ORDER_COUNT) order_value = (int)ORDER_HOLD_POSITION;

				/*
				**	"Do not restore stale raw navigation pointers, transient reservations, or
				**	invalid path handles.  Revalidate/replan."  An order here is a destination
				**	and a target id, never a path -- there is nothing in this record that could
				**	be a stale handle, and the AI replans from the order the first time it runs.
				*/
				CommanderProductionService::Restore_Order (unit_id,(CommanderUnitOrderType)order_value,
																			point,target_id);
				break;
			}

			case CHUNKID_COMMANDERS:
			{
				int team = PLAYERTYPE_NEUTRAL;
				int player = 0;

				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {
						READ_MICRO_CHUNK (cload,VARID_COMMANDER_TEAM,	team);
						READ_MICRO_CHUNK (cload,VARID_COMMANDER_PLAYER,player);
					}
					cload.Close_Micro_Chunk ();
				}

				CommanderProductionService::Set_Team_Commander (team,player);
				break;
			}
		}

		cload.Close_Chunk ();
	}

	return true;
}


void RTSPersistenceClass::Rebuild_Derived_State(void)
{
	/*
	**	Everything Section 46 says to recompute rather than load, in one place.
	**
	**	RecalculateTeam is the single call that rebuilds generation, requested and allocated
	**	demand, the load-shed set, every building's Enable_Power flag, and -- because it owns
	**	them -- the communications sensor registrations Section 46 separately requires to be
	**	"rebuilt from live buildings/descriptors after load".  One call, because there is one
	**	allocator; a second recomputation path here would be exactly the "second independent
	**	powered/unpowered truth" the section forbids.
	**
	**	Team Silo wallet capacity is likewise not stored and not rebuilt separately: it is
	**	Get_Wallet_Capacity_Bonus, which is computed on demand from the same surviving members,
	**	so it is correct the instant this returns.  "Preserve existing player balances even
	**	when they currently exceed a reduced reconstructed cap" is free here, because nothing
	**	in this codebase clamps a balance yet (see Q-014).
	*/
	TeamPowerGridService::RecalculateTeam (PLAYERTYPE_NOD);
	TeamPowerGridService::RecalculateTeam (PLAYERTYPE_GDI);
}


void RTSPersistenceClass::On_Post_Load(void)
{
	Rebuild_Derived_State ();
}


int RTSPersistenceClass::Get_Late_Join_Construction_Count(void)	{ return _LateJoinCount; }
int RTSPersistenceClass::Get_Last_Load_Failure_Count(void)			{ return _LoadFailures; }
