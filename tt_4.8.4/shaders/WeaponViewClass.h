#ifndef TT_INCLUDE__WEAPONVIEWCLASS_H
#define TT_INCLUDE__WEAPONVIEWCLASS_H
#include "vector3.h"
#include "DecorationPhysClass.h"


class ChunkSaveClass;
class ChunkLoadClass;



class WeaponViewClass
{

public:

	static void Think();
	static bool Load(ChunkLoadClass&);
	static void Init();
	static void Shutdown();
	static void Reset();
	static bool Save(ChunkSaveClass&);
	static void Enable(bool);
	static Vector3 Get_Muzzle_Pos();
	static REF_DECL2(WeaponViewEnabled,bool);
	static REF_DECL1(HandsPhysObj,DecorationPhysClass*);
};



#endif