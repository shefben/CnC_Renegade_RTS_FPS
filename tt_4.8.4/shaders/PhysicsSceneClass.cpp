#include "general.h"
#include "PhysicsSceneClass.h"
#include "TexProjectClass.h"
#include "PhysGridCullClass.h"
#include "DynamicPhysClass.h"
#include "physcoltest.h"
#include "StaticAABTreeCullClass.h"

REF_DEF2(PhysicsSceneClass::TheScene, PhysicsSceneClass*, 0x00855444, 0x0085462C);
REF_ARR_DEF2(PhysicsSceneClass::AllowCollisionFlags, bool, 256, 0x00854FD0, 0x008541B8);

RefMultiListIterator<PhysClass> PhysicsSceneClass::Get_Static_Object_Iterator(void)			
{ 
	return RefMultiListIterator<PhysClass>(&StaticObjList);
}

void PhysicsSceneClass::Disable_Collision_Detection(int group0,int group1)
{
	unsigned int index;
	index = group0 | (group1 << COLLISION_FLAG_SHIFT);
	AllowCollisionFlags[index] = 0;
	index = group1 | (group0 << COLLISION_FLAG_SHIFT);
	AllowCollisionFlags[index] = 0;
}

void PhysicsSceneClass::Enable_Collision_Detection(int group0,int group1)
{
	unsigned int index;
	index = group0 | (group1 << COLLISION_FLAG_SHIFT);
	AllowCollisionFlags[index] = 1;
	index = group1 | (group0 << COLLISION_FLAG_SHIFT);
	AllowCollisionFlags[index] = 1;
}

RENEGADE_FUNCTION
void PhysicsSceneClass::Add_Camera_Shake
   (const Vector3& position, float radius, float duration, float intensity)
   AT2(0x00624420,0x00623CC0);

RENEGADE_FUNCTION
int PhysicsSceneClass::Create_Decal
   (const Matrix3D &tm,const char *texture_name,float radius,bool is_permanent,bool apply_to_translucent_polys,PhysClass *only_this_obj)
   AT2(0x00632210,0x00631AB0);

RENEGADE_FUNCTION
bool PhysicsSceneClass::Cast_Ray
   (PhysRayCollisionTestClass&, bool)
   AT2(0x00631460,0x00630D00);

RENEGADE_FUNCTION
bool PhysicsSceneClass::Cast_OBBox
   (PhysOBBoxCollisionTestClass&, bool)
   AT2(0x00631600,0x00630EA0);

RENEGADE_FUNCTION
void PhysicsSceneClass::Pre_Render_Processing(CameraClass& camera)
AT1(0x00622C00);

void PhysicsSceneClass::Post_Render_Processing()
{
	VisibleDynamicObjectList.Reset_List();
	VisibleStaticObjectList.Reset_List();
	VisibleWSMeshList.Reset_List();
	Per_Frame_Statistics_Update();
}

RENEGADE_FUNCTION
void PhysicsSceneClass::Per_Frame_Statistics_Update()
AT1(0x00623D30);

RENEGADE_FUNCTION
void PhysicsSceneClass::Compute_Static_Lighting(LightEnvironmentClass *,Vector3  const&,bool,int)
AT1(0x00632700);

RENEGADE_FUNCTION
void PhysicsSceneClass::Apply_Camera_Shakes(CameraClass&)
AT1(0x00624450);

RENEGADE_FUNCTION
StaticPhysClass *PhysicsSceneClass::Get_Static_Object_By_ID(uint32 id)
AT1(0x007832B0);

RENEGADE_FUNCTION
void PhysicsSceneClass::Shatter_Mesh(MeshClass*, const Vector3&, const Vector3&, const Vector3&)
AT2(0x00623E70, 0x00623710);

RENEGADE_FUNCTION
void PhysicsSceneClass::Collect_Objects(const AABoxClass&, bool, bool, MultiListClass<PhysClass>*)
AT2(0x00631D70, 0x00631610);

RENEGADE_FUNCTION
void PhysicsSceneClass::Collect_Objects(const OBBoxClass&, bool, bool, MultiListClass<PhysClass>*)
AT2(0x00631E30, 0x006316D0);

void PhysicsSceneClass::Process_Release_List(void)
{
	while(!ReleaseList.Is_Empty())
	{
		PhysClass * obj = ReleaseList.Remove_Head();
		Remove_Object(obj);
		obj->Release_Ref();
	}	
}

void DynamicPhysClass::Update_Visibility_Status()
{
   this->DirtyVisObjectID = true;
   this->Invalidate_Static_Lighting_Cache();
}

void PhysClass::Invalidate_Static_Lighting_Cache()
{
   this->Flags |= 0x00100000;
}

RENEGADE_FUNCTION
void PhysicsSceneClass::Remove_Object(PhysClass *obj)
AT2(0x00622430, 0x00621CD0);

bool PhysicsSceneClass::Contains(PhysClass* obj)
{
	if (ObjList.Is_In_List(obj)) return true;
	if (StaticObjList.Is_In_List(obj)) return true;
	if (StaticLightList.Is_In_List(obj)) return true;
	return false;
}

RENEGADE_FUNCTION
VisTableClass* PhysicsSceneClass::Get_Vis_Table(const Vector3& position)
AT2(0x0062F580, 0x0062EE20);

void PhysicsSceneClass::Set_Polygon_Budgets(int static_count,int dynamic_count)
{
	StaticPolyBudget = static_count;
	DynamicPolyBudget = dynamic_count;
}

void PhysicsSceneClass::Get_Polygon_Budgets(int * static_count,int * dynamic_count)
{
	*static_count = StaticPolyBudget;
	*dynamic_count = DynamicPolyBudget;
}

PhysicsSceneClass::ShadowEnum
PhysicsSceneClass::Get_Shadow_Mode(void)
{
	return ShadowMode;
}

void PhysicsSceneClass::Enable_Static_Projectors(bool onoff)
{
	StaticProjectorsEnabled = onoff;
}

void PhysicsSceneClass::Enable_Dynamic_Projectors(bool onoff)
{
	DynamicProjectorsEnabled = onoff;
}
