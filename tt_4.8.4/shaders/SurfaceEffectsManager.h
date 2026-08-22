#ifndef TT_INCLUDE__SURFACEEFFECTSMANAGER_H
#define TT_INCLUDE__SURFACEEFFECTSMANAGER_H

#include "multilist.h"
#include "RenderObjClass.h"
#include "TimedDecorationPhysClass.h"

class Matrix3D;
class PhysClass;
class PhysicalGameObj;
class PersistantSurfaceSoundClass;
class PersistantSurfaceEmitterClass;
class RenderObjectRecyclerClass : public CombatPhysObserverClass
{
	RefMultiListClass<RenderObjClass> ModelList;
public:
	RenderObjectRecyclerClass()
	{
	}
	~RenderObjectRecyclerClass()
	{
	}
};
class EffectRecyclerClass : public RenderObjectRecyclerClass {
	RefMultiListClass<TimedDecorationPhysClass> PhysList;
public:
	EffectRecyclerClass()
	{
	}
	~EffectRecyclerClass()
	{
		if (Exe == 0)
		{
			Reset();
		}
	}
	void Reset();
	void Spawn_Effect(char  const*, Matrix3D  const&, float);
	virtual void Object_Removed_From_Scene(PhysClass *);
};


class SurfaceEffectsManager
{

private:
	static REF_DECL2(OverrideSurfaceType, int);
public:
	static REF_DECL2(Mode, int);
	static void Init();
	static void Shutdown();
	static void Apply_Effect(int, int, const Matrix3D&, PhysClass*, PhysicalGameObj*, bool, bool);
	static PersistantSurfaceSoundClass *Create_Persistant_Sound();
	static void Destroy_Persistant_Sound(PersistantSurfaceSoundClass*);
	static void Update_Persistant_Sound(PersistantSurfaceSoundClass*, int, int, const Matrix3D&);
	static PersistantSurfaceEmitterClass *Create_Persistant_Emitter();
	static void Destroy_Persistant_Emitter(PersistantSurfaceEmitterClass*);
	static void Update_Persistant_Emitter(PersistantSurfaceEmitterClass*, int, int, const Matrix3D&);
	static bool Does_Surface_Stop_Bullets(int);
	static void Apply_Damage(int, PhysicalGameObj*);
	static bool Is_Surface_Permeable(int);
	static void Set_Override_Surface_Type(int);

};



#endif