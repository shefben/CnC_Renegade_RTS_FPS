#ifndef TT_INCLUDE_PROJECTOR_H
#define TT_INCLUDE_PROJECTOR_H
#include "MaterialPassClass.h"
#include "AABoxClass.h"
#include "mapper.h"
class ProjectorClass {
public:
	ProjectorClass(void);
	virtual ~ProjectorClass(void);
	virtual void					Set_Transform(const Matrix3D & tm);
	virtual const Matrix3D &	Get_Transform(void) const;
	virtual void					Set_Perspective_Projection(float hfov,float vfov,float znear,float zfar);
	virtual void					Set_Ortho_Projection(float xmin,float xmax,float ymin,float ymax,float znear,float zfar);
	const OBBoxClass &Get_Bounding_Volume(void) const { return WorldBoundingVolume; }
	SHADERS_API void								Compute_Texture_Coordinate(const Vector3 & point,Vector3 * set_stq);
protected:
	virtual void					Update_WS_Bounding_Volume(void);
	Matrix3D							Transform; // 0004
	Matrix4							Projection; // 0034
	AABoxClass						LocalBoundingVolume; // 0074
	OBBoxClass						WorldBoundingVolume; // 008C
	MatrixMapperClass *			Mapper; // 00C8
};
#endif
