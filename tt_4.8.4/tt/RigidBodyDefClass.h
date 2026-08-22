#ifndef TT_INCLUDE__RIGIDBODYDEFCLASS_H
#define TT_INCLUDE__RIGIDBODYDEFCLASS_H

#include "MoveablePhysDefClass.h"


class RigidBodyDefClass : public MoveablePhysDefClass
{

public:
	RigidBodyDefClass(void);
	virtual uint32								Get_Class_ID (void) const;
	virtual PersistClass *					Create(void) const;
	virtual const char *						Get_Type_Name(void)			{ return "RigidBodyDef"; }
	virtual bool								Is_Type(const char *);
	virtual const PersistFactoryClass &	Get_Factory (void) const;
	virtual bool								Save(ChunkSaveClass &csave);
	virtual bool								Load(ChunkLoadClass &cload);
	float											Get_Aerodynamic_Drag(void)	{ return AerodynamicDragCoefficient; }
	void											Set_Aerodynamic_Drag(float new_drag) { AerodynamicDragCoefficient = new_drag; }
	DECLARE_EDITABLE(RigidBodyDefClass,MoveablePhysDefClass);
protected:
	float											AerodynamicDragCoefficient;
	bool											CollisionDisabled;
	friend class RigidBodyClass;
};
#endif
