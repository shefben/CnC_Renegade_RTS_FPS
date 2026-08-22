#ifndef TT_INCLUDE__AIRSTRIPGAMEOBJDEF_H
#define TT_INCLUDE__AIRSTRIPGAMEOBJDEF_H



#include "VehicleFactoryGameObjDef.h"



class AirStripGameObjDef :
	public VehicleFactoryGameObjDef
{
			AirStripGameObjDef();
	virtual ~AirStripGameObjDef();
	virtual const PersistFactoryClass& Get_Factory  () const;
	virtual bool                       Save         (ChunkSaveClass& chunkSaver);
	virtual bool                       Load         (ChunkLoadClass& chunkLoader);
	virtual uint32                     Get_Class_ID () const;
	virtual VehicleFactoryGameObj*     Create       () const;
private:
	int dropCinematicId; // 00A8
	int dropCinematicVehicleSlot; // 00AC
	float lengthToDropoff; // 00B0
	float lengthToVehicleDisplay; // 00B4
	friend class AirStripGameObj;
}; // 00B8



#endif