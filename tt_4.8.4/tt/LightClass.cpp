#include "General.h"
#include "LightClass.h"
#include "SceneClass.h"
LightClass::LightClass(LightClass::LightType type) : Type(type), Flags(0), CastShadows(false), Intensity(1), Ambient(1,1,1), Diffuse(1,1,1), Specular(1,1,1), NearAttenStart(0), NearAttenEnd(0), FarAttenStart(50), FarAttenEnd(100), SpotAngle(0.78539819f), SpotAngleCos(0.70700002f), SpotExponent(1), SpotDirection(0,0,1)
{
	if (type == DIRECTIONAL)
	{
		Bits |= 8;
	}
}
LightClass::~LightClass()
{
}
LightClass::LightClass(const LightClass &src) :
	RenderObjClass(src),
	Type(src.Type),
	Flags(src.Flags),
	CastShadows(src.CastShadows),
	Intensity(src.Intensity),
	Ambient(src.Ambient),
	Diffuse(src.Diffuse),
	Specular(src.Specular),
	NearAttenStart(src.NearAttenStart),
	NearAttenEnd(src.NearAttenEnd),
	FarAttenStart(src.FarAttenStart),
	FarAttenEnd(src.FarAttenEnd),
	SpotAngle(src.SpotAngle),
	SpotAngleCos(src.SpotAngleCos),
	SpotExponent(src.SpotExponent),
	SpotDirection(src.SpotDirection)
{
}
LightClass& LightClass::operator=(const LightClass& that)
{
	if (this != &that)
	{
		RenderObjClass::operator = (that);
		Type = that.Type;
		Flags = that.Flags;
		CastShadows = that.CastShadows;
		Intensity = that.Intensity;
		Ambient = that.Ambient;
		Diffuse = that.Diffuse;
		Specular = that.Specular;
		NearAttenStart = that.NearAttenStart;
		NearAttenEnd = that.NearAttenEnd;
		FarAttenStart = that.FarAttenStart;
		FarAttenEnd = that.FarAttenEnd;
		SpotAngle = that.SpotAngle;
		SpotAngleCos = that.SpotAngleCos;
		SpotExponent = that.SpotExponent;
		SpotDirection = that.SpotDirection;
	}
	return *this;
}
RenderObjClass *LightClass::Clone() const
{
	return new LightClass(*this);
}
int LightClass::Class_ID() const
{
	return 0xE;
}
void LightClass::Render(RenderInfoClass&)
{
}
void LightClass::Notify_Added(SceneClass*scene)
{
	RenderObjClass::Notify_Added(scene);
	Scene->Register(this,SceneClass::LIGHT);
}
void LightClass::Notify_Removed(SceneClass*scene)
{
	Scene->Unregister(this,SceneClass::LIGHT);
	RenderObjClass::Notify_Removed(scene);
}
void LightClass::Get_Obj_Space_Bounding_Sphere(SphereClass&sphere) const
{
	sphere.Center = Vector3(0,0,0);
	sphere.Radius = FarAttenEnd;
}
void LightClass::Get_Obj_Space_Bounding_Box(AABoxClass&box) const
{
	box.Center = Vector3(0,0,0);
	box.Extent = Vector3(FarAttenEnd,FarAttenEnd,FarAttenEnd);
}
