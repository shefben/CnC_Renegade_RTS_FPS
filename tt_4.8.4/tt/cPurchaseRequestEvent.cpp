#include "general.h"
#include "cPurchaseRequestEvent.h"
#include "SoldierGameObj.h"
#include "GameObjManager.h"
#include "NetworkObjectMgrClass.h"
#include "cPurchaseResponseEvent.h"
#include "cNetwork.h"
#include "ConsoleModeClass.h"
#include "cConnection.h"
#include "engine_game.h"
uint32 Purchase_Vendor_Item(SoldierGameObj *owner, PURCHASE_TYPE type, int position, int alternate, bool sendresponse);

cPurchaseRequestEvent::cPurchaseRequestEvent()
   : clientId(0), position(0), type(PT_REFILL), alternate(-1)
{
   this->PacketType = 41;
}

void cPurchaseRequestEvent::Act()
{
	Set_Delete_Pending();

	SoldierGameObj *soldier = GameObjManager::Find_Soldier_Of_Client_ID(this->clientId);
	if (!soldier || soldier->Get_Vehicle())
		return;

	uint32 reply = 3;

	if (The_Game()->Is_Gameplay_Permitted())
		reply = Purchase_Vendor_Item(soldier, this->type, this->position, this->alternate, false);

	if (cNetwork::I_Am_Only_Server() && this->type == 1)
		ConsoleBox.Print_Maybe("%ls purchased a vehicle\n", (const wchar_t*)(FindPlayer(this->clientId))->PlayerName.Peek_Buffer());

	(new cPurchaseResponseEvent)->Init(reply, this->clientId);
}



void cPurchaseRequestEvent::Export_Creation
   (BitStreamClass& oStream)
{
   oStream.Add_Bits (this->clientId, 32);
   oStream.Add_Bits (this->type, 32);
   oStream.Add_Bits (this->position, 32);
   oStream.Add_Bits (this->alternate, 32);

   this->Set_Delete_Pending();
}



unsigned int cPurchaseRequestEvent::Get_Network_Class_ID() const
{
   return 1024;
}



void cPurchaseRequestEvent::Import_Creation
   (BitStreamClass& stream)
{
   stream.Get(this->clientId);
   stream.Get((int&)this->type);
   stream.Get(this->position);
   stream.Get(this->alternate);
   this->Act();
}



void cPurchaseRequestEvent::Init
   (PURCHASE_TYPE type, sint32 position, sint32 alternate)
{
   this->clientId = cNetwork::PClientConnection->Get_Local_Id();
   this->type      = type;
   this->position     = position;
   this->alternate    = alternate;
   this->Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID());

   if (!(cNetwork::I_Am_Server()))
      this->Set_Object_Dirty_Bit (0, DB_CREATION, true);
   else
      this->Act();
}
