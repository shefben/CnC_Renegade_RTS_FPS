#ifndef TT_INCLUDE__LINE3DCLASS_H
#define TT_INCLUDE__LINE3DCLASS_H


#include "RenderObjClass.h"
#include "engine_3dre.h"



class RenderInfoClass;
class SphereClass;
class AABoxClass;
class Vector3;



class SHADERS_API Line3DClass :
	public RenderObjClass
{

private:

	float length; // 0088  00A0
	float thickness; // 008C  00A4
	ShaderClass shader; // 0090  00A8
	Vector3 vertices[8]; // 0094  00AC
	Vector3 color; // 00F4  010C
	float opacity; // 0100  0118

public:

	virtual Line3DClass* Clone() const { return new Line3DClass(*this); }
	virtual int Class_ID() const { return 6; }
	virtual int Get_Num_Polys() const { return 12; }
	virtual void Render(RenderInfoClass&);
	virtual void Get_Obj_Space_Bounding_Sphere(SphereClass&) const;
	virtual void Get_Obj_Space_Bounding_Box(AABoxClass&) const;
	virtual void Scale(float scale);
	virtual void Scale(float, float, float);

	Line3DClass(const Vector3& from, const Vector3& to, float _thickness, float colorR, float colorG, float colorB, float _opacity);
	void Reset(const Vector3&, const Vector3&);
	void Reset(const Vector3&, const Vector3&, float);
	void Re_Color(float, float, float);
	void Set_Opacity(float);

};



#endif