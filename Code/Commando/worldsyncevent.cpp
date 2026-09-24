/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/worldsyncevent.cpp                  $*
 *                                                                                             *
 *	D-001 priority 3.  See worldsyncevent.h.                                                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldsyncevent.h"

#include "apppackettypes.h"
#include "cnetwork.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"

DECLARE_NETWORKOBJECT_FACTORY (cWorldSyncEvent,NETCLASSID_WORLDSYNCEVENT);

cWorldSyncEvent::cWorldSyncEvent(void) :
	FromServer(false),
	SenderId(0)
{
	Set_App_Packet_Type (APPPACKETTYPE_WORLDSYNCEVENT);
}


void cWorldSyncEvent::Send(int destination,const WorldSyncFragment & fragment)
{
	cWorldSyncEvent * event = new cWorldSyncEvent;
	event->Fragment = fragment;

	if (cNetwork::I_Am_Server ()) {
		event->FromServer	= true;
		event->SenderId	= 0;

		//	A server-originated event takes the id NetworkObjectClass gives it, as
		//	cCommanderWaypointEvent does.
		if (destination == WSYNC_TO_ALL_CLIENTS) {
			event->Set_Object_Dirty_Bit (BIT_CREATION,true);
		} else {
			event->Set_Object_Dirty_Bit (destination,BIT_CREATION,true);
		}
		return;
	}

	event->FromServer	= false;
	event->SenderId	= cNetwork::Get_My_Id ();
	event->Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());
	event->Set_Object_Dirty_Bit (0,BIT_CREATION,true);
}


void cWorldSyncEvent::Export_Creation(BitStreamClass & packet)
{
	cNetEvent::Export_Creation (packet);

	packet.Add (FromServer);
	packet.Add (SenderId);
	packet.Add ((uint32_t)Fragment.MessageId);
	packet.Add (Fragment.Kind);
	packet.Add (Fragment.Index);
	packet.Add (Fragment.Count);
	packet.Add (Fragment.Bytes);
	if (Fragment.Bytes > 0) {
		packet.Add_Raw_Data ((const char *)Fragment.Payload,(uint16_t)Fragment.Bytes);
	}

	Set_Delete_Pending ();
}


void cWorldSyncEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	uint32_t message_id = 0;
	packet.Get (FromServer);
	packet.Get (SenderId);
	packet.Get (message_id);
	packet.Get (Fragment.Kind);
	packet.Get (Fragment.Index);
	packet.Get (Fragment.Count);
	packet.Get (Fragment.Bytes);
	Fragment.MessageId = message_id;

	//	Bounded before it is read, so a hostile length cannot overrun the payload: a fragment
	//	claiming more than one payload's worth is not read, and the transport refuses it.
	if ((Fragment.Bytes < 0) || (Fragment.Bytes > WorldSyncFragment::MAX_PAYLOAD)) {
		Fragment.Bytes = 0;
		Fragment.Count = 0;
	} else if (Fragment.Bytes > 0) {
		packet.Get_Raw_Data ((char *)Fragment.Payload,(uint16_t)sizeof (Fragment.Payload),(uint16_t)Fragment.Bytes);
	}

	Act ();
}


void cWorldSyncEvent::Act(void)
{
	//	A client's fragment is filed under the id it was sent with, the same sender field every
	//	Commander event carries; the server's reply goes back to that id.
	int sender = FromServer ? 0 : SenderId;

	WorldSyncTransport::Receive_Fragment (sender,Fragment);
	Set_Delete_Pending ();
}
