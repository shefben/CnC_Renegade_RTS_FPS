#ifndef TT_INCLUDE_PHYSTEXPROJECTCLASS_H
#define TT_INCLUDE_PHYSTEXPROJECTCLASS_H
#include "TexProjectClass.h"
class PhysTexProjectClass : public TexProjectClass {
public:
	PhysTexProjectClass(void);
	virtual ~PhysTexProjectClass(void);
	bool						Compute_Perspective_Projection(PhysClass * obj,const Vector3 & lightpos,float nearz=-1.0f,float farz=-1.0f);
	bool						Compute_Ortho_Projection(PhysClass * obj,const Vector3 & lightdir,float nearz=-1.0f,float farz=-1.0f);
	SHADERS_API bool						Compute_Texture(PhysClass * obj,bool additive_projection = false);
};
#endif