#ifndef TT_INCLUDE__CPURCHASERESPONSEEVENT_H
#define TT_INCLUDE__CPURCHASERESPONSEEVENT_H

#include "cNetEvent.h"


class cPurchaseResponseEvent : public cNetEvent
{

private:

   sint32 clientId;        // 1716
   sint32 reply;           // 1720


public:

   cPurchaseResponseEvent();

   virtual unsigned int Get_Network_Class_ID () const;
   virtual void   Import_Creation      (BitStreamClass& oStream);
   virtual void   Export_Creation      (BitStreamClass& oStream);
   virtual void   Act                  ();

   void Init (sint32 reply, sint32 clientId);

};


#endif
