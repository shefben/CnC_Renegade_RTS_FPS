#ifndef TT_INCLUDE__BULLETMANAGER_H
#define TT_INCLUDE__BULLETMANAGER_H



class AmmoDefinitionClass;
class Vector3;
class ArmedGameObj;
class DamageableGameObj;
class ChunkLoadClass;
class ChunkSaveClass;



class BulletManager
{

public:

	static void Update();
	static void Create_Bullet(const AmmoDefinitionClass*, const Vector3&, const Vector3&, const ArmedGameObj*, float, const Vector3&, DamageableGameObj*);
	static void Shutdown();
	static void Init();
	static bool Load(ChunkLoadClass&);
	static bool Save(ChunkSaveClass&);

};



#endif