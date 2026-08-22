#ifndef TT_INCLUDE_TIMEDDECORATIONPHYSCLASS_H
#define TT_INCLUDE_TIMEDDECORATIONPHYSCLASS_H
#include "DecorationPhysClass.h"

class TimedDecorationPhysClass : public DecorationPhysClass {
public:
	float LifeTime;
	virtual ~TimedDecorationPhysClass();
	virtual bool                       Needs_Timestep               ();
	virtual void                       Timestep                     (float) = 0;
	virtual TimedDecorationPhysClass*  As_TimedDecorationPhysClass  () {return this;}
	virtual bool                       Save                         (ChunkSaveClass& oSave);
	virtual bool                       Load                         (ChunkLoadClass& oLoad);
	virtual const PersistFactoryClass& Get_Factory                  () const;
	virtual void Set_Lifetime(float lifetime);
	virtual float Get_Lifetime(float lifetime);
};

#endif