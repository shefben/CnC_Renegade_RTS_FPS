#ifndef TT_INCLUDE_BOXRENDEROBJCLASS_H
#define TT_INCLUDE_BOXRENDEROBJCLASS_H
#include "RenderObjClass.h"
class BoxRenderObjClass : public RenderObjClass
{
public:
	char Name[32];
	Vector3 Color;
	Vector3 ObjSpaceCenter;
	Vector3 ObjSpaceExtent;
	virtual const char*          Get_Name                          () const { return "UNNAMED"; }
	virtual void                 Set_Name                          (const char*) { }
	virtual int                  Get_Num_Polys                     () const { return 0; }
	virtual void update_cached_box() = 0;
	static void Init();
	static void Shutdown();
};
class OBBoxRenderObjClass : public BoxRenderObjClass
{
public:
	OBBoxClass CachedBox;
	virtual ~OBBoxRenderObjClass();
	virtual RenderObjClass*      Clone                             () const;
	virtual int                  Class_ID                          () const;
	virtual void                 Render                            (RenderInfoClass&);
	virtual void                 Special_Render                    (SpecialRenderInfoClass&);
	virtual void                 Set_Transform                     (const Matrix3D&);
	virtual void                 Set_Position                      (const Vector3&);
	virtual bool                 Cast_Ray                          (RayCollisionTestClass&);
	virtual bool                 Cast_AABox                        (AABoxCollisionTestClass&);
	virtual bool                 Cast_OBBox                        (OBBoxCollisionTestClass&);
	virtual bool                 Intersect_AABox                   (AABoxIntersectionTestClass&);
	virtual bool                 Intersect_OBBox                   (OBBoxIntersectionTestClass&);
	virtual void                 Get_Obj_Space_Bounding_Sphere     (SphereClass&) const;
	virtual void                 Get_Obj_Space_Bounding_Box        (AABoxClass&) const;
	virtual void update_cached_box();
};
#endif