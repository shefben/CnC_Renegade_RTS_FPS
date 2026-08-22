#include "General.h"
#include "cNetEvent.h"

#include "cNetwork.h"
#include "cAppPacketStats.h"
#include "engine_diagnostics.h"



cNetEvent::~cNetEvent()
{
}



void cNetEvent::Delete()
{
	delete this;
}



void cNetEvent::Export_Creation(BitStreamClass& oStream)
{
	// If you hit this assert, the event is probably not calling cNetEvent::Init in its ::Init function.
	TT_ASSERT(Is_Delete_Pending());
}



void cNetEvent::Import_Creation(BitStreamClass& oStream)
{
	if (!Is_Delete_Pending())
	{
		;//DebugOutputString("Event %d of type %-20s:%d was not deleted directly after creation!\n", Get_Network_ID(), cAppPacketStats::Interpret_Type(Get_App_Packet_Type()), Get_Network_Class_ID());
		Set_Delete_Pending();
	}
}



void cNetEvent::Init()
{
	Set_Delete_Pending();
}
