#ifndef TT_INCLUDE__LIGHTCLASS_H
#define TT_INCLUDE__LIGHTCLASS_H
#include "RenderObjClass.h"
class LightClass : public RenderObjClass
{
public:
	enum LightType {
		POINT = 0x0,
		DIRECTIONAL = 0x1,
		SPOT = 0x2,
	};
	LightType Type; //136 160
	unsigned int Flags; //140 164
	bool CastShadows; //144	168
	float Intensity; //148 172
	Vector3 Ambient; //152 176
	Vector3 Diffuse; //164 188
	Vector3 Specular; //176 200
	float NearAttenStart; //188 212
	float NearAttenEnd; //192 216
	float FarAttenStart; //196 220
	float FarAttenEnd; //200 224
	float SpotAngle; //204 228
	float SpotAngleCos; //208 232
	float SpotExponent; //212 236
	Vector3 SpotDirection; //126 240
	virtual ~LightClass();
	virtual RenderObjClass *Clone() const;
	virtual int Class_ID() const;
	virtual void Render(RenderInfoClass&);
	virtual void Notify_Added(SceneClass*);
	virtual void Notify_Removed(SceneClass*);
	virtual void Get_Obj_Space_Bounding_Sphere(SphereClass&) const;
	virtual void Get_Obj_Space_Bounding_Box(AABoxClass&) const;
	virtual bool Is_Vertex_Processor() {return true;};
	LightClass(LightClass::LightType type);
	LightClass(const LightClass &src);
	LightClass& operator=(const LightClass& that);
};
#endif