#ifndef TT_INCLUDE__RIGIDBODYSTATESTRUCT_H
#define TT_INCLUDE__RIGIDBODYSTATESTRUCT_H

#include "Quaternion.h"
#include "StateVectorClass.h"

struct RigidBodyStateStruct 
{
	RigidBodyStateStruct(void) {}
	RigidBodyStateStruct(const RigidBodyStateStruct & that);
	RigidBodyStateStruct & operator = (const RigidBodyStateStruct & that);
	void				Reset(void);
	void				To_Vector(StateVectorClass & vec);
	int				From_Vector(const StateVectorClass & vec,int index);
	static void		Lerp(const RigidBodyStateStruct & s0,const RigidBodyStateStruct & s1,float fraction,RigidBodyStateStruct * res);
	bool				Is_Valid(void) { return (Position.Is_Valid() && Orientation.Is_Valid() && LMomentum.Is_Valid() && AMomentum.Is_Valid()); }
	Vector3			Position;
	Quaternion		Orientation;
	Vector3			LMomentum;
	Vector3			AMomentum;
};
inline RigidBodyStateStruct::RigidBodyStateStruct(const RigidBodyStateStruct & that)
{
	*this = that;
}
inline RigidBodyStateStruct & RigidBodyStateStruct::operator = (const RigidBodyStateStruct & that)
{
	Position = that.Position;
	Orientation = that.Orientation;
	LMomentum = that.LMomentum;
	AMomentum = that.AMomentum;
	return *this;
}
inline void RigidBodyStateStruct::Reset(void)
{
	Position.Set(0,0,0);
	Orientation.Make_Identity();
	LMomentum.Set(0,0,0);
	AMomentum.Set(0,0,0);
}
#endif
