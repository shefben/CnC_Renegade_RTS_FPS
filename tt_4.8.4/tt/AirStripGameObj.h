#ifndef TT_INCLUDE__AIRSTRIPGAMEOBJ_H
#define TT_INCLUDE__AIRSTRIPGAMEOBJ_H



#include "VehicleFactoryGameObj.h"



class PersistFactoryClass;
class AirStripGameObjDef;
class ChunkSaveClass;
class ChunkLoadClass;
class BaseControllerClass;



class AirStripGameObj :
	public VehicleFactoryGameObj
{

private:

	float dropoffTime; // 08CC
	float unk08D0; // 08D0
	float lengthToVehicleDisplay; // 08D4
	float lengthToDropoff; // 08D8;
	bool isCinematicPlaying; // 08DC;
	PhysicalGameObj* dropCinematic; // 08E0

public:

	AirStripGameObj();
	virtual ~AirStripGameObj();
	virtual const PersistFactoryClass& Get_Factory() const;
	virtual void Init();
	void Init(const AirStripGameObjDef&);
	const AirStripGameObjDef & Get_Definition( void ) const ;
	virtual bool Save(ChunkSaveClass&);
	virtual bool Load(ChunkLoadClass&);
	bool Load_Variables(ChunkLoadClass&);
	virtual void CnC_Initialize(BaseControllerClass*);
	virtual void Think();
	virtual void Begin_Generation();
	void Start_Cinematic();
	virtual AirStripGameObj* As_AirStripGameObj() {return this;}

}; // 08E4



#endif