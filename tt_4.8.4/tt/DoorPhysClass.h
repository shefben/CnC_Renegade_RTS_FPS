#ifndef TT_INCLUDE__DOORPHYSCLASS_H
#define TT_INCLUDE__DOORPHYSCLASS_H



#include "AccessiblePhysClass.h"



class OBBoxClass;



class DoorPhysClass : public AccessiblePhysClass
{

public:

	float DoorCloseTime; //108
	float u10C; //10C
	int DoorState; //110
	bool DoorOpen; //114
	bool DoorLocked; //115

	static void Set_Precision();

	~DoorPhysClass();
	void Timestep(float);
	DoorPhysClass * As_DoorPhysClass() {return this;}
	virtual void Save_State(ChunkSaveClass &);
	virtual void Load_State(ChunkLoadClass &);
	virtual void Update_State(float);
	virtual bool Can_Open_Door(void);
	virtual bool Check_Door_Trigger(OBBoxClass const &);
	virtual bool Set_State(int);
	const PersistFactoryClass& Get_Factory  () const;
	bool Save(ChunkSaveClass &csave);
	bool Load(ChunkLoadClass &cload);

};



#endif
