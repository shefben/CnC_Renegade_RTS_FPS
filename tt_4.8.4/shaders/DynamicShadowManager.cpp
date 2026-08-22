#include "General.h"
#include "DynamicShadowManager.h"
#include "DynTexProjectClass.h"
#include "PhysicsSceneClass.h"
#include "LightClass.h"
DynamicShadowManagerClass::DynamicShadowManagerClass(PhysClass & parent) :
	Parent(parent),
	Shadow(NULL),
	ShadowNearZ(-1.0f),
	ShadowFarZ(-1.0f),
	ForceUseBlobBox(false),
	BlobBoxProjectionScale(1,1,1)
{
}

DynamicShadowManagerClass::~DynamicShadowManagerClass(void)
{
	Release_Shadow();
}

void DynamicShadowManagerClass::Allocate_Shadow(void)
{
	if (Shadow == NULL)
	{
		PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
		Shadow = new DynTexProjectClass(&Parent);
		Shadow->Enable_Attenuation(true);
		Shadow->Enable_Depth_Gradient(true);
		Shadow->Enable_Affect_Dynamic_Objects(false);
		Shadow->Set_Intensity(scene->Get_Shadow_Normal_Intensity(),true);
		Shadow->Peek_Material_Pass()->EnableOnTranslucentMeshes = false;
		scene->Add_Dynamic_Texture_Projector(Shadow);
	}
}

void DynamicShadowManagerClass::Release_Shadow(void)
{
	if (Shadow)
	{
		if (PhysicsSceneClass::Get_Instance()->Contains(Shadow))
		{
			PhysicsSceneClass::Get_Instance()->Remove_Dynamic_Texture_Projector(Shadow);
		}
		Shadow->Release_Ref();
		Shadow = NULL;
	}
}
