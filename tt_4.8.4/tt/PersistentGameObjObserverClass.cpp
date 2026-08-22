#include "General.h"
#include "PersistentGameObjObserverClass.h"
#include "GameObjObserverManager.h"
#include "engine_vector.h"
class PersistentGameObjObserverManager {
private:
	static REF_DECL2(ObserverList, DynamicVectorClass<PersistentGameObjObserverClass *>);
public:
	static void Add(PersistentGameObjObserverClass *o)
	{
		ObserverList.Add(o);
	}
	static void Remove(PersistentGameObjObserverClass *o)
	{
		ObserverList.DeleteObj(o);
	}
};
REF_DEF2(PersistentGameObjObserverManager::ObserverList, DynamicVectorClass<PersistentGameObjObserverClass *>, 0x0085F368, 0x0085E540);
PersistentGameObjObserverClass::PersistentGameObjObserverClass()
{
	ID = GameObjObserverManager::NextID;
	GameObjObserverManager::NextID++;
	PersistentGameObjObserverManager::Add(this);
}
PersistentGameObjObserverClass::~PersistentGameObjObserverClass()
{
	PersistentGameObjObserverManager::Remove(this);
}
RENEGADE_FUNCTION
bool PersistentGameObjObserverClass::Save(ChunkSaveClass &csave)
AT2(0x00721810,0x00720DD0);
RENEGADE_FUNCTION
bool PersistentGameObjObserverClass::Load(ChunkLoadClass &cload)
AT2(0x007218A0,0x00720E60);
