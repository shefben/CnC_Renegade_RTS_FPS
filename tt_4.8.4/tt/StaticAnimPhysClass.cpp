#include "General.h"
#include "StaticAnimPhysClass.h"

#include "RenderObjClass.h"
#include "Vector2.h"



AnimCollisionManagerClass& StaticAnimPhysClass::getAnimCollisionManager()
{
	return animCollisionManager;
}



void StaticAnimPhysClass::Update_Sun_Status()
{
	AABoxClass boundingBox;
	Model->Get_Obj_Space_Bounding_Box(boundingBox);
	
	if (((Vector2&)boundingBox.Extent).Length() >= 15.f*15.f)
		Flags |= IS_IN_THE_SUN;
	else
		PhysClass::Update_Sun_Status();
}



RENEGADE_FUNCTION
void StaticAnimPhysClass::On_Post_Load()
AT2(0x00650A00, 0x006502A0);
