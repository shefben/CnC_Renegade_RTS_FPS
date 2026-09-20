/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderwaypointevent.cpp          $*
 *                                                                                             *
 *	Roadmap Sections 3A and 45.  See commanderwaypointevent.h.                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderwaypointevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "combat.h"
#include "commanderwaypoint.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"


DECLARE_NETWORKOBJECT_FACTORY (cCommanderWaypointEvent,NETCLASSID_COMMANDERWAYPOINTEVENT);


cCommanderWaypointEvent::cCommanderWaypointEvent(void) :
	Team(-1),
	Kind(0),
	Position(0.0f,0.0f,0.0f),
	LifetimeSeconds(0.0f),
	Sequence(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERWAYPOINTEVENT);
}


void cCommanderWaypointEvent::Init(int team,int kind,const Vector3 & position,
											  float lifetime_seconds,int sequence)
{
	WWASSERT (CombatManager::I_Am_Server ());

	Team				= team;
	Kind				= kind;
	Position			= position;
	LifetimeSeconds	= lifetime_seconds;
	Sequence			= sequence;

	//	No explicit network id: a server-originated event takes the one NetworkObjectClass gives
	//	it, exactly as cScExplosionEvent does.  Get_New_Client_ID is for the other direction.

	//	Sent to every client, not only the team's.  Filtering here would need the server to map
	//	clients to teams at send time; `CommanderWaypointSystem` already refuses to draw another
	//	team's marker, and a per-team send list is a second place for that rule to be wrong.
	Set_Object_Dirty_Bit (BIT_CREATION,true);

	//	The server is usually also a client (a listen server), so it applies its own event the
	//	same way cScExplosionEvent does rather than relying on the round trip.
	Act ();
}


void cCommanderWaypointEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (CombatManager::I_Am_Server ());

	cNetEvent::Export_Creation (packet);

	packet.Add (Team);
	packet.Add (Kind);
	packet.Add (Position.X);
	packet.Add (Position.Y);
	packet.Add (Position.Z);
	packet.Add (LifetimeSeconds);
	packet.Add (Sequence);

	Set_Delete_Pending ();
}


void cCommanderWaypointEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	packet.Get (Team);
	packet.Get (Kind);
	packet.Get (Position.X);
	packet.Get (Position.Y);
	packet.Get (Position.Z);
	packet.Get (LifetimeSeconds);
	packet.Get (Sequence);

	Act ();
}


void cCommanderWaypointEvent::Act(void)
{
	//	Clamped rather than trusted: a lifetime a hostile packet made enormous would pin a marker
	//	on screen for the rest of the match, and a negative one is refused by the system anyway.
	float lifetime = LifetimeSeconds;
	if (lifetime < 0.0f)  lifetime = 0.0f;
	if (lifetime > 120.0f) lifetime = 120.0f;

	if ((Kind >= 0) && (Kind < WAYPOINT_KIND_COUNT)) {
		CommanderWaypointSystem::Set_Waypoint (Team,(CommanderWaypointKindType)Kind,Position,
															lifetime,Sequence);
	}

	Set_Delete_Pending ();
}
