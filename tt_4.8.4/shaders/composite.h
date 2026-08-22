#ifndef TT_INCLUDE__COMPOSITE_H
#define TT_INCLUDE__COMPOSITE_H
#include "AABoxClass.h"
#include "SphereClass.h"
#include "RenderObjClass.h"
class CompositeRenderObjClass : public RenderObjClass {
private:
	char* Name;
	char* BaseModelName;
	SphereClass ObjSphere;
	AABoxClass ObjBox;
public:
	~CompositeRenderObjClass();
	void Restart();
	const char *Get_Name() const;
	void Set_Name(const char* name);
	const char *Get_Base_Model_Name() const;
	void Set_Base_Model_Name(const char* name);
	int Get_Num_Polys() const;
	void Notify_Added(SceneClass* scene);
	void Notify_Removed(SceneClass* scene);
	bool Cast_Ray(RayCollisionTestClass& raytest);
	bool Cast_AABox(AABoxCollisionTestClass& boxtest);
	bool Cast_OBBox(OBBoxCollisionTestClass& boxtest);
	bool Intersect_AABox(AABoxIntersectionTestClass& boxtest);
	bool Intersect_OBBox(OBBoxIntersectionTestClass& boxtest);
	void Create_Decal(DecalGeneratorClass* generator);
	void Delete_Decal(uint32 decal_id);
	void Get_Obj_Space_Bounding_Sphere(SphereClass& sphere) const;
	void Get_Obj_Space_Bounding_Box(AABoxClass& box) const;
	void Update_Obj_Space_Bounding_Volumes();
	void Set_User_Data(void* value,bool recursive);
};
#endif
