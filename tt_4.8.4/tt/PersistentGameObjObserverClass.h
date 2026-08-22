#ifndef TT_INCLUDE__PERSISTENTGAMEOBJOBSERVERCLASS_H
#define TT_INCLUDE__PERSISTENTGAMEOBJOBSERVERCLASS_H



#include "scripts.h"
#include "Persist.h"

class PersistentGameObjObserverClass :
	public PersistClass,
	public GameObjObserverClass
{
public:
	PersistentGameObjObserverClass();
	~PersistentGameObjObserverClass();
	bool Save(ChunkSaveClass &csave);
	bool Load(ChunkLoadClass &cload);
}; // 0010



#endif