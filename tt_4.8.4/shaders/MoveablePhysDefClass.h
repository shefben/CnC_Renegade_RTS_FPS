#ifndef TT_INCLUDE__MOVEABLEPHYSDEFCLASS_H
#define TT_INCLUDE__MOVEABLEPHYSDEFCLASS_H

#include "DynamicPhysDefClass.h"


class MoveablePhysDefClass : public DynamicPhysDefClass
{
public:
	MoveablePhysDefClass(void);
	virtual const char *						Get_Type_Name(void)			{ return "MoveablePhysDef"; }
	virtual bool								Is_Type(const char *);
	virtual bool								Save(ChunkSaveClass &csave);
	virtual bool								Load(ChunkLoadClass &cload);
	float											Get_Mass(void)					{ return Mass; }
	float											Get_Grav_Scale(void)			{ return GravScale; }
	void											Set_Mass(float new_mass)	{ Mass = new_mass; }
	void											Set_Grav_Scale(float new_g){ GravScale = new_g; }
	DECLARE_EDITABLE(MoveablePhysDefClass,DynamicPhysDefClass);
protected:
	float				Mass;
	float				GravScale;
	float				Elasticity;
	enum { 
		CINEMATIC_COLLISION_NONE = 0, 
		CINEMATIC_COLLISION_STOP, 
		CINEMATIC_COLLISION_PUSH, 
		CINEMATIC_COLLISION_KILL 
	};
	int				CinematicCollisionMode;
	friend class MoveablePhysClass;
}; // 36

#endif
