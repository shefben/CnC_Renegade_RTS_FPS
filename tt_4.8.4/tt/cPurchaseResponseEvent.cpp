#include "general.h"
#include "cPurchaseResponseEvent.h"
#include "cNetwork.h"
#include "bitstream.h"
#include "cConnection.h"


cPurchaseResponseEvent::cPurchaseResponseEvent()
   : clientId(-1), reply(-1)
{
   this->PacketType = 10;
}

RENEGADE_FUNCTION
void cPurchaseResponseEvent::Act()
   AT2(0x004B8EA0,0x004B86A0);

void cPurchaseResponseEvent::Export_Creation
   (BitStreamClass& oStream)
{
   oStream.Add(this->clientId);
   oStream.Add(this->reply);
   this->Set_Delete_Pending();
}

unsigned int cPurchaseResponseEvent::Get_Network_Class_ID() const
{
   return 1004;
}

void cPurchaseResponseEvent::Import_Creation
   (BitStreamClass& oStream)
{
   oStream.Get(this->clientId);
   oStream.Get(this->reply);
   this->Act();
}

void cPurchaseResponseEvent::Init
   (sint32 reply, sint32 clientId)
{
   this->clientId = clientId;
   this->reply           = reply;

   if (cNetwork::PClientConnection && clientId == cNetwork::PClientConnection->Get_Local_Id())
      this->Act();
   else
      this->Set_Object_Dirty_Bit(clientId, DB_CREATION,true);
}
