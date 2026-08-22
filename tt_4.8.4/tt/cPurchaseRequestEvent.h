#ifndef TT_INCLUDE__CPURCHASEREQUESTEVENT_H
#define TT_INCLUDE__CPURCHASEREQUESTEVENT_H

#include "cNetEvent.h"
#include "tt.h"
class cPurchaseRequestEvent : public cNetEvent
{

private:

   sint32               clientId; // 1716
   PURCHASE_TYPE        type;     // 1720
   sint32               position; // 1724
   sint32               alternate;// 1728


public:

            cPurchaseRequestEvent();
   virtual unsigned int Get_Network_Class_ID () const;
   virtual void   Import_Creation      (BitStreamClass& oStream);
   virtual void   Export_Creation      (BitStreamClass& oStream);
   virtual void   Act                  ();

   void Init (PURCHASE_TYPE type, sint32 position, sint32 alternate);

};


#endif
