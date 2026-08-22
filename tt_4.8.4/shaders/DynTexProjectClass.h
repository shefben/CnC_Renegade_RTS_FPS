#ifndef TT_INCLUDE_DYNTEXPROJECTCLASS_H
#define TT_INCLUDE_DYNTEXPROJECTCLASS_H
#include "PhysTexProjectClass.h"
class DynTexProjectClass : public PhysTexProjectClass {
public:
	DynTexProjectClass(PhysClass * shadow_generator);
	virtual ~DynTexProjectClass(void);
	virtual void		Pre_Render_Update(const Matrix3D & camera);
	void					Set_Projection_Object(PhysClass * obj);
	PhysClass *			Peek_Projection_Object(void) const;
	virtual void *		Get_Projection_Object_ID(void) const			{ return ProjectionObject; }
	void					Set_Light_Source_ID(uint32 id);
	uint32				Get_Light_Source_ID(void);
	void					Set_Light_Vector(const Vector3 & vector);
	void					Get_Light_Vector(Vector3 * set_vector);
	void					Enable_Perspective(bool onoff);
	bool					Is_Perspective_Enabled(void);
	void					Set_Texture_Dirty(bool onoff = true);
	bool					Is_Texture_Dirty(void);
	void					Update_Projection(const AABoxClass & objbox,const Matrix3D & objtm,float znear = -1.0f,float zfar = -1.0f);
protected:
	PhysClass *			ProjectionObject;
	uint32				LightSourceID;
	Vector3				LightVector;
};
#endif