#ifndef TT_INCLUDE__STATICPHYSCLASS_H
#define TT_INCLUDE__STATICPHYSCLASS_H

#include "PhysClass.h"


class StaticPhysClass : public PhysClass
{

private:

	sint32 visSectorId; // 0078  0078  0094  0090


public:

	virtual                            ~StaticPhysClass             ();
	virtual bool                       Needs_Timestep               ();
	virtual void                       Timestep                     (float);
	virtual const Matrix3D&            Get_Transform                () const;
	virtual void                       Set_Transform                (const Matrix3D&);
	virtual bool                       Cast_Ray                     (PhysRayCollisionTestClass&);
	virtual bool                       Cast_AABox                   (PhysAABoxCollisionTestClass&);
	virtual bool                       Cast_OBBox                   (PhysOBBoxCollisionTestClass&);
	virtual bool                       Intersection_Test            (PhysAABoxIntersectionTestClass&);
	virtual bool                       Intersection_Test            (PhysOBBoxIntersectionTestClass&);
	virtual bool                       Intersection_Test            (PhysMeshIntersectionTestClass&);
	virtual void                       Set_Model                    (RenderObjClass*);
	virtual void                       Render_Vis_Meshes            (RenderInfoClass&);
	virtual StaticPhysClass*           As_StaticPhysClass           () {return this;}
	virtual bool                       Save                         (ChunkSaveClass& oSave);
	virtual bool                       Load                         (ChunkLoadClass& oLoad);
	virtual bool                       Is_Simulation_Disabled       ();
	virtual bool                       Is_Rendering_Disabled        ();
	virtual const AABoxClass&          Get_Bounding_Box             () const;
	virtual bool                       Has_Dynamic_State            ();
	virtual void                       Save_State                   (ChunkSaveClass &);
	virtual void                       Load_State                   (ChunkLoadClass &);
	virtual const PersistFactoryClass&       Get_Factory            () const;
	virtual void                       On_Post_Load                 ();


	bool Is_Occluder();


}; // 007C  007C  0098  0094


#endif
