#ifndef TT_INCLUDE__PHYSICSCONSTANTS_H
#define TT_INCLUDE__PHYSICSCONSTANTS_H



#include "Vector3.h"



class ChunkSaveClass;
class ChunkLoadClass;



typedef float ContactFrictionCoefficientType[3][32];
extern REF_DECL2(ContactFrictionCoefficient, ContactFrictionCoefficientType);

typedef float ContactDragCoefficientType[3][32];
extern REF_DECL2(ContactDragCoefficient, ContactDragCoefficientType);



class PhysicsConstants
{

public:

	static REF_DECL2(LinearDamping, UNK);
	static const float AngularDamping;
	static REF_DECL2(RestingContactVelocity, UNK);
	static REF_DECL2(MinFrictionVelocity, UNK);
	static const float DefaultContactFriction;
	static const float DefaultContactDrag;
	static REF_DECL2(SurfaceTypeOverride, int);
	static REF_DECL2(OverrideDrag, float);
	static REF_DECL2(OverrideFriction, float);
	static const Vector3 GravityAcceleration;
	static REF_DECL2(MinFrictionVelocity2, UNK);

	static UNK Init();
	
	static void Set_Contact_Friction_Coefficient(int physicsType, int surfaceType, float value)
	{
		if ((uint)physicsType < 3 && (uint)surfaceType < 32)
			ContactFrictionCoefficient[physicsType][surfaceType] = value;
	}
	
	static float Get_Contact_Friction_Coefficient(int physicsType, int surfaceType)
	{
		if (SurfaceTypeOverride != -1)
			surfaceType = SurfaceTypeOverride;
		
		if (OverrideFriction != 0)
			return OverrideFriction;
		else if ((uint)physicsType < 3 && (uint)surfaceType < 32)
			return ContactFrictionCoefficient[physicsType][surfaceType];
		else
			return ContactFrictionCoefficient[0][0];
	}

	static void Set_Contact_Drag_Coefficient(int physicsType, int surfaceType, float value)
	{
		if ((uint)physicsType < 3 && (uint)surfaceType < 32)
			ContactDragCoefficient[physicsType][surfaceType] = value;
	}

	static float Get_Contact_Drag_Coefficient(int physicsType, int surfaceType)
	{
		TT_UNTESTED;

		if (SurfaceTypeOverride != -1)
			surfaceType = SurfaceTypeOverride;
		
		if (OverrideDrag != 0)
			return OverrideDrag;
		else if ((uint)physicsType < 3 && (uint)surfaceType < 32)
			return ContactDragCoefficient[physicsType][surfaceType];
		else
			return ContactDragCoefficient[0][0];
	}

	static UNK Set_Override_Surface_Type(int);
	static UNK Set_Override_Surface_Friction(float);
	static UNK Set_Override_Surface_Drag(float);
	static bool Save(ChunkSaveClass&);
	static bool Load(ChunkLoadClass&);

};



#endif
