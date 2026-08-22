#ifndef TT_INCLUDE__SPAWNERCLASS_H
#define TT_INCLUDE__SPAWNERCLASS_H

#include "engine_vector.h"
#include "Matrix3D.h"
#include "engine_string.h"
#include "ReferencerClass.h"
#include "Persist.h"

class SpawnerDefClass;
class PhysicalGameObj;


class SpawnerClass :
	public PersistClass
{

private:

	uint32                          id;                   // 8
	Matrix3D                        transform;            // 12
	Matrix3D                        spawnEffectTransform; // 60
	const SpawnerDefClass*          definition;           // 108
	ReferencerClass                 currentObject;        // 112
	bool                            enabled;              // 128
	sint32                          spawnCount;           // 132
	float                           spawnTimeRemaining;   // 136
	DynamicVectorClass<Matrix3D>    alternateTransforms;  // 140
	DynamicVectorClass<StringClass> scriptNames;          // 164
	DynamicVectorClass<StringClass> scriptParameters;     // 188


public:

	SpawnerClass();
	virtual ~SpawnerClass();

	virtual const PersistFactoryClass& Get_Factory() const;
	virtual bool Save(ChunkSaveClass& chunkSaver);
	virtual bool Load(ChunkLoadClass& chunkLoader);

	void Add_Script(const char* name, const char* parameters);
	bool Can_Spawn_Object(sint32 id) const;
	void Check_Auto_Spawn(float);
	PhysicalGameObj* Create_Spawned_Object(sint32 id);
	bool Determine_Spawn_TM(PhysicalGameObj* object);
	bool TT_Determine_Spawn_TM(PhysicalGameObj* object);
	bool Original_Determine_Spawn_TM(PhysicalGameObj* object);
	void Enable(bool _enabled) { enabled = _enabled; }
	const SpawnerDefClass* Get_Definition() const;
	uint32 Get_ID() const;
	Matrix3D Get_TM() const;
	void Init(const SpawnerDefClass& definition);
	void Set_ID(uint32 id);
	void Set_TM(const Matrix3D& transform);
	PhysicalGameObj* Spawn(sint32 id);
	PhysicalGameObj* Spawn_Object(sint32 id);

}; // size: 212


#endif
