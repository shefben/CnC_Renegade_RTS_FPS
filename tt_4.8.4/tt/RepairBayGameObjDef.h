#ifndef TT_INCLUDE__REPAIRBAYGAMEOBJDEF_H
#define TT_INCLUDE__REPAIRBAYGAMEOBJDEF_H

#include "BuildingGameObjDef.h"


class RepairBayGameObjDef : public BuildingGameObjDef
{
public:
	RepairBayGameObjDef();
	virtual ~RepairBayGameObjDef();
	virtual const PersistFactoryClass& Get_Factory  () const;
	virtual bool                       Save         (ChunkSaveClass& chunkSaver);
	virtual bool                       Load         (ChunkLoadClass& chunkLoader);
	virtual uint32                     Get_Class_ID () const;
	virtual PersistClass*              Create       () const;
	void Load_Variables (ChunkLoadClass& chunkLoader);
private:
	float repairPerSecond; // 160
	uint32 staticAnimId;    // 164
	friend class RepairBayGameObj;
};


#endif
