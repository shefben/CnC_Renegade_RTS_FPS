#ifndef TT_INCLUDE__CNETEVENT_H
#define TT_INCLUDE__CNETEVENT_H



#include "engine_common.h"
#include "engine_string.h"
#include "NetworkObjectClass.h"

class cNetEvent :
	public NetworkObjectClass
{

public:

   virtual ~cNetEvent();
   virtual unsigned int Get_Network_Class_ID() const = 0;
   virtual void Import_Creation (BitStreamClass& oStream);
   virtual void Export_Creation (BitStreamClass& oStream);
   virtual void Delete          ();
   virtual void Act             () = 0;

   void Init();

}; // 06B4



#endif
