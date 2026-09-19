/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderstate.cpp                    $*
 *                                                                                             *
 *	Roadmap Section 46, Phase 40 -- the server->client half of the Commander/RTS layer.         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderstate.h"

#include "apppackettypes.h"
#include "bitstream.h"
#include "buildingconstruction.h"
#include "combat.h"
#include "commanderproduction.h"
#include "gameobjmanager.h"
#include "playertype.h"
#include "teampowergridservice.h"
#include "timemgr.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


//	One instance with a fixed network id, so both ends already have the object and only its
//	contents ever travel -- the same shape PurchaseAvailabilityClass uses.
static CommanderStateClass	_TheCommanderState;

static int	_ImportCount	= 0;
static int	_ExportCount	= 0;

/*
**	The client's estimate of the server clock: the value the last packet carried, plus the
**	client's own elapsed time since it arrived.  Not a drift-corrected clock sync -- it is only
**	used to advance a construction animation between packets, and every packet re-anchors it.
*/
static float	_ServerTimeAtImport	= 0.0f;
static float	_LocalTimeAtImport	= 0.0f;
static bool		_HaveServerTime		= false;

//	Section 44's replication list is small and constructions are rare; a hard bound keeps the
//	packet bounded by construction rather than by hope.
enum { MAX_REPLICATED_CONSTRUCTIONS = 32 };

//	The two real sides.  A third, neutral slot exists in the services' own tables but nothing
//	Commander-facing ever runs on it.
static const int	TEAMS [2] = { PLAYERTYPE_NOD,PLAYERTYPE_GDI };


/*
**	What the last export carried, so Server_Update can notice a change without a caller having
**	to remember to announce one.
*/
struct TeamMirror
{
	TeamMirror(void) : Generation(-1.0f),Requested(-1.0f),Allocated(-1.0f),
							 Shed(-1),Powered(-1),ManualOff(-1),Commander(-1) { }
	float	Generation;
	float	Requested;
	float	Allocated;
	int	Shed;
	int	Powered;
	int	ManualOff;
	int	Commander;
};

static TeamMirror	_LastSent [2];
static int			_LastSentConstructionCount = -1;


CommanderStateClass::CommanderStateClass(void)
{
	Set_Network_ID (NETID_SERVER_COMMANDER_STATE);
	Set_App_Packet_Type (APPPACKETTYPE_NETCOMMANDERSTATE);
}


void CommanderStateClass::Reset(void)
{
	for (int i = 0; i < 2; i++) _LastSent [i] = TeamMirror ();
	_LastSentConstructionCount	= -1;
	_ServerTimeAtImport			= 0.0f;
	_LocalTimeAtImport			= 0.0f;
	_HaveServerTime				= false;
	Changed ();
}


void CommanderStateClass::Changed(void)
{
	if (CombatManager::I_Am_Server ()) {
		_TheCommanderState.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE,true);
	}
}


void CommanderStateClass::Server_Update(void)
{
	if (!CombatManager::I_Am_Server ()) return;

	bool changed = false;

	for (int i = 0; i < 2; i++) {

		int team = TEAMS [i];
		TeamMirror current;
		current.Generation	= TeamPowerGridService::Get_Generation (team);
		current.Requested		= TeamPowerGridService::Get_Requested_Demand (team);
		current.Allocated		= TeamPowerGridService::Get_Allocated_Demand (team);
		current.Shed			= TeamPowerGridService::Get_Shed_Count (team);
		current.Powered		= TeamPowerGridService::Get_Powered_Count (team);
		current.ManualOff		= TeamPowerGridService::Get_Manual_Off_Count (team);
		current.Commander		= CommanderProductionService::Get_Team_Commander (team);

		if ((WWMath::Fabs (current.Generation - _LastSent [i].Generation) > 0.01f) ||
			 (WWMath::Fabs (current.Requested  - _LastSent [i].Requested)  > 0.01f) ||
			 (WWMath::Fabs (current.Allocated  - _LastSent [i].Allocated)  > 0.01f) ||
			 (current.Shed      != _LastSent [i].Shed) ||
			 (current.Powered   != _LastSent [i].Powered) ||
			 (current.ManualOff != _LastSent [i].ManualOff) ||
			 (current.Commander != _LastSent [i].Commander)) {
			changed = true;
		}
		_LastSent [i] = current;
	}

	/*
	**	A construction starting or finishing changes the list; a construction *progressing* does
	**	not, because progress is derived from a start time the client already has.  That is the
	**	whole reason this packet is rare rather than frequent.
	*/
	int construction_count = BuildingConstructionService::Get_Active_Count ();
	if (construction_count != _LastSentConstructionCount) {
		_LastSentConstructionCount = construction_count;
		changed = true;
	}

	if (changed) Changed ();
}


void CommanderStateClass::Export_Rare(BitStreamClass & packet)
{
	_ExportCount++;

	//	The server clock, first, so a client can anchor everything that follows to it.
	packet.Add ((float)TimeManager::Get_Total_Seconds ());

	for (int i = 0; i < 2; i++) {
		int team = TEAMS [i];
		packet.Add ((float)TeamPowerGridService::Get_Generation (team));
		packet.Add ((float)TeamPowerGridService::Get_Requested_Demand (team));
		packet.Add ((float)TeamPowerGridService::Get_Allocated_Demand (team));
		packet.Add ((int)  TeamPowerGridService::Get_Shed_Count (team));
		packet.Add ((int)  TeamPowerGridService::Get_Powered_Count (team));
		packet.Add ((int)  TeamPowerGridService::Get_Manual_Off_Count (team));
		packet.Add ((int)  CommanderProductionService::Get_Team_Commander (team));
	}

	/*
	**	The construction list, in exactly Section 44's compact form.  The transform is sent as a
	**	position and a Z rotation rather than as twelve floats, because that is precisely what a
	**	placement produces (`CommanderPlacementValidator::Build_Placement_Transform` builds the
	**	matrix from those two), so it is both smaller and impossible to get the row order wrong.
	*/
	int active = BuildingConstructionService::Get_Active_Count ();
	if (active > MAX_REPLICATED_CONSTRUCTIONS) active = MAX_REPLICATED_CONSTRUCTIONS;
	packet.Add ((int)active);

	for (int index = 0; index < active; index++) {

		int building_id = BuildingConstructionService::Get_Active_Building (index);

		BuildingConstructionReplicationState state;
		AABoxClass bounds;
		bool ok = (building_id != 0) &&
					 BuildingConstructionService::Get_Replication_State (building_id,&state) &&
					 BuildingConstructionService::Get_Model_Bounds (building_id,&bounds);

		if (!ok) {
			//	Still write a well-formed, ignorable entry: the count is already on the wire, and
			//	a short list would desync every field after it.
			state = BuildingConstructionReplicationState ();
			bounds.Center = Vector3 (0.0f,0.0f,0.0f);
			bounds.Extent = Vector3 (0.0f,0.0f,0.0f);
		}

		Vector3 position	= state.Transform.Get_Translation ();
		Vector3 forward	= state.Transform.Get_Y_Vector ();
		float rotation		= RAD_TO_DEGF (WWMath::Atan2 (forward.X * -1.0f,forward.Y));

		packet.Add ((int)state.BuildingId);
		packet.Add_Terminated_String (state.DescriptorName.Peek_Buffer (),true);
		packet.Add ((float)position.X);
		packet.Add ((float)position.Y);
		packet.Add ((float)position.Z);
		packet.Add ((float)rotation);
		packet.Add ((float)state.StartTimeSeconds);
		packet.Add ((float)state.DurationSeconds);
		packet.Add ((int)  state.Seed);
		packet.Add ((float)bounds.Center.X);
		packet.Add ((float)bounds.Center.Y);
		packet.Add ((float)bounds.Center.Z);
		packet.Add ((float)bounds.Extent.X);
		packet.Add ((float)bounds.Extent.Y);
		packet.Add ((float)bounds.Extent.Z);
	}
}


void CommanderStateClass::Import_Rare(BitStreamClass & packet)
{
	_ImportCount++;

	float server_time = 0.0f;
	packet.Get (server_time);
	_ServerTimeAtImport	= server_time;
	_LocalTimeAtImport	= TimeManager::Get_Total_Seconds ();
	_HaveServerTime		= true;

	for (int i = 0; i < 2; i++) {

		float generation = 0.0f, requested = 0.0f, allocated = 0.0f;
		int shed = 0, powered = 0, manual_off = 0, commander = 0;

		packet.Get (generation);
		packet.Get (requested);
		packet.Get (allocated);
		packet.Get (shed);
		packet.Get (powered);
		packet.Get (manual_off);
		packet.Get (commander);

		/*
		**	Told, not derived.  A client's own allocator never runs, so this is the only place a
		**	client learns these numbers -- which is what keeps Section 46's "do not serialize a
		**	second independent powered/unpowered truth" true across the wire as well as in a
		**	save file.
		*/
		TeamPowerGridService::Set_Replicated_Team_State (TEAMS [i],generation,requested,allocated,
																		 shed,powered,manual_off);
		CommanderProductionService::Set_Team_Commander (TEAMS [i],commander);
	}

	int active = 0;
	packet.Get (active);
	if (active < 0) active = 0;
	if (active > MAX_REPLICATED_CONSTRUCTIONS) active = MAX_REPLICATED_CONSTRUCTIONS;

	int seen [MAX_REPLICATED_CONSTRUCTIONS];
	::memset (seen,0,sizeof (seen));

	for (int index = 0; index < active; index++) {

		char name [128] = { 0 };
		int building_id = 0, seed = 0;
		float x = 0.0f,y = 0.0f,z = 0.0f,rotation = 0.0f,start = 0.0f,duration = 0.0f;
		Vector3 center (0.0f,0.0f,0.0f), extent (0.0f,0.0f,0.0f);

		packet.Get (building_id);
		packet.Get_Terminated_String (name,sizeof (name),true);
		packet.Get (x);  packet.Get (y);  packet.Get (z);
		packet.Get (rotation);
		packet.Get (start);
		packet.Get (duration);
		packet.Get (seed);
		packet.Get (center.X); packet.Get (center.Y); packet.Get (center.Z);
		packet.Get (extent.X); packet.Get (extent.Y); packet.Get (extent.Z);

		seen [index] = building_id;

		if (building_id == 0 || duration <= 0.0f) continue;

		//	Already running locally: the numbers behind it cannot change once a construction has
		//	started, so there is nothing to update.
		if (BuildingConstructionService::Is_Under_Construction (building_id)) continue;

		BuildingConstructionReplicationState state;
		state.BuildingId			= building_id;
		state.DescriptorName		= name;
		state.StartTimeSeconds	= start;
		state.DurationSeconds	= duration;
		state.Seed					= (unsigned int)seed;
		state.Active				= true;

		Matrix3D transform (1);
		transform.Rotate_Z (DEG_TO_RADF (rotation));
		transform.Set_Translation (Vector3 (x,y,z));
		state.Transform = transform;

		AABoxClass bounds;
		bounds.Center = center;
		bounds.Extent = extent;

		/*
		**	The late-join path and the load path are this same call -- the client regenerates the
		**	identical block layout from (bounds, seed), and no block ever crossed the wire.
		*/
		if (!BuildingConstructionService::Begin_Construction_From_State (state,bounds)) {
			WWDEBUG_SAY (("CommanderStateClass: could not start replicated construction for "
							  "building %d (descriptor '%s')\n",building_id,name));
		}
	}

	/*
	**	Anything running locally that the server no longer lists was cancelled or destroyed.
	**	Dropping it here is what makes "cancellation/destruction leaves no temporary state" true
	**	on a client as well as on the server.
	*/
	int local = BuildingConstructionService::Get_Active_Count ();
	for (int index = local - 1; index >= 0; index--) {

		int building_id = BuildingConstructionService::Get_Active_Building (index);
		if (building_id == 0) continue;

		bool still_listed = false;
		for (int s = 0; s < active; s++) {
			if (seen [s] == building_id) { still_listed = true; break; }
		}
		if (!still_listed) {
			BuildingConstructionService::Cancel_Construction (building_id);
		}
	}
}


float CommanderStateClass::Get_Estimated_Server_Time(void)
{
	if (CombatManager::I_Am_Server () || !_HaveServerTime) {
		return TimeManager::Get_Total_Seconds ();
	}
	return _ServerTimeAtImport + (TimeManager::Get_Total_Seconds () - _LocalTimeAtImport);
}


int CommanderStateClass::Get_Import_Count(void)	{ return _ImportCount; }
int CommanderStateClass::Get_Export_Count(void)	{ return _ExportCount; }
