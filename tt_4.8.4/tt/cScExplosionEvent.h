#ifndef RR_INCLUDE__CSCEXPLOSIONEVENT_H
#define RR_INCLUDE__CSCEXPLOSIONEVENT_H

#include "cNetEvent.h"
#include "Vector3.h"

class cScExplosionEvent : public cNetEvent
{
private:
   int Explosion_Preset_ID; //6B4
   Vector3 position; //6B8 6BC 6C0
   int Damager_ID; //6C4
   int Damaged_ID; //6C8
public:
    cScExplosionEvent();
   ~cScExplosionEvent();
   void   Act                  ();
   void   Export_Creation      (BitStreamClass& oStream);
   unsigned int Get_Network_Class_ID () const;
   void   Import_Creation      (BitStreamClass& oStream);
   void   Init                 (sint32, Vector3&, sint32, sint32);
};


#endif
