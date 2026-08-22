#include "General.h"
#include "MuzzleFlashClass.h"
#include "RenderObjClass.h"
MuzzleFlashClass::MuzzleFlashClass() :
	MuzzleA0Bone(0),
	MuzzleA1Bone(0),
	Rotation(0),
	Model(0),
	LastFlashA0(true),
	LastFlashA1(true)
{
}

MuzzleFlashClass::~MuzzleFlashClass()
{
	REF_PTR_RELEASE(Model);
}

void MuzzleFlashClass::Init(RenderObjClass* model)
{
	REF_PTR_SET(Model, model);
	if (model != NULL)
	{
		MuzzleA0Bone = model->Get_Bone_Index("muzzlea0");
		MuzzleA1Bone = model->Get_Bone_Index("muzzlea1");
		Update(false, false);
	}
}

void MuzzleFlashClass::Update(bool flashA0, bool flashA1)
{
	if (MuzzleA1Bone == 0)
	{
		flashA0 = flashA0 | flashA1;
		flashA1 = false;
	}
	if (Model != NULL)
	{
		if ((MuzzleA0Bone > 0) && (LastFlashA0 != flashA0))
		{
			LastFlashA0 = flashA0;
			for (int i = 0;i < Model->Get_Num_Sub_Objects_On_Bone(MuzzleA0Bone);i++)
			{
				RenderObjClass *robj = Model->Get_Sub_Object_On_Bone(i, MuzzleA0Bone);
				if (strstr(robj->Get_Name(),"MUZZLEFLASH") || strstr(robj->Get_Name(),"MZ"))
				{
					robj->Set_Hidden(!flashA0);
				}
				robj->Release_Ref();
			}
		}
		if ((MuzzleA1Bone > 0) && (LastFlashA1 != flashA1))
		{
			LastFlashA1 = flashA1;
			for (int i = 0;i < Model->Get_Num_Sub_Objects_On_Bone(MuzzleA1Bone);i++)
			{
				RenderObjClass *robj = Model->Get_Sub_Object_On_Bone(i, MuzzleA1Bone);
				if (strstr(robj->Get_Name(),"MUZZLEFLASH") || strstr(robj->Get_Name(),"MZ"))
				{
					robj->Set_Hidden(!flashA1);
				}
				robj->Release_Ref();
			}
		}
	}
}
