#include "General.h"
#include "StealthEffectClass.h"
#include "MaterialPassClass.h"
#include "mapper.h"
#include "RenderInfoClass.h"
#include "CameraClass.h"
#include "PhysClass.h"



TextureClass* Peek_Stealth_Texture();



float STEALTH_DEFAULT_FADE_DISTANCE = 25;



void StealthEffectClass::Enable_Stealth(bool enable)
{
	IsStealthEnabled = enable;
	Update_Target_Fraction();
}



void StealthEffectClass::Update_Target_Fraction()
{
	if (!IsStealthEnabled)
		TargetFraction = 0.f;
	else if (IsBroken)
		TargetFraction = 0.25f;
	else if (IsFriendly)
		TargetFraction = 0.75f;
	else
		TargetFraction = 1.f;
}



StealthEffectClass::StealthEffectClass() :
	IsStealthEnabled(false),
	IsFriendly(false),
	IsBroken(false),
	FadeDistance(STEALTH_DEFAULT_FADE_DISTANCE),
	CurrentFraction(0),
	TargetFraction(1),
	UVRate(0.5,0.5),
	RenderBaseMaterial(true),
	RenderStealthMaterial(true),
	IntensityScale(0),
	UVOffset(0,0),
	Mapper(0),
	MaterialPass(0)
{
	Vector3 v(0,0,0);
	Vector3 v2(1,1,1);

	Mapper = new MatrixMapperClass();
	Mapper->Set_Type(MatrixMapperClass::ORTHO_PROJECTION);
	Mapper->Set_Texture_Transform(Matrix4::IDENTITY, 64);

	VertexMaterialClass* material = new VertexMaterialClass();
	material->Set_Ambient(v);
	material->Set_Diffuse(v);
	material->Set_Specular(v);
	material->Set_Emissive(v2);
	material->Set_Opacity(1);
	material->Set_Shininess(0);
	material->CRCDirty = true;
	material->UseLighting = true;
	material->Set_Mapper(Mapper, 0);

	MaterialPass = new MaterialPassClass();
	MaterialPass->Set_Material(material);
	MaterialPass->Set_Shader(ShaderClass::_PresetAdditiveShader);
	MaterialPass->Set_Texture(Peek_Stealth_Texture(), 0);
	MaterialPass->EnableOnTranslucentMeshes = true;

	material->Release_Ref();
}



StealthEffectClass::~StealthEffectClass()
{
	REF_PTR_RELEASE(MaterialPass);
	REF_PTR_RELEASE(Mapper);
}



RENEGADE_FUNCTION
void StealthEffectClass::Timestep(float)
AT2(0x0077D830,0x0077D0D0);



void StealthEffectClass::Render_Push(RenderInfoClass& rinfo, PhysClass* phys)
{
	Vector3 physPosition;
	phys->Get_Position(&physPosition);

	float distance = (rinfo.Camera.Get_Position() - physPosition).Length();
	if (distance <= FadeDistance)
		IntensityScale = min(1, IntensityScale + (FadeDistance - distance) * .4f / FadeDistance);
	
	RenderStealthMaterial = IntensityScale > 0.f;
	if (RenderStealthMaterial)
	{
		Vector3 emissive(IntensityScale, IntensityScale, IntensityScale);
		MaterialPass->Material->Set_Emissive(emissive);
		Matrix4 mappertransform = Matrix4::IDENTITY;
		mappertransform[0][3] = UVOffset.X;
		mappertransform[1][3] = UVOffset.Y;
		Mapper->Set_Texture_Transform(mappertransform, 64.f);
		rinfo.Push_Material_Pass(MaterialPass);
	}

	if (!RenderBaseMaterial)
		rinfo.Push_Override_Flags(RenderInfoClass::RINFO_OVERRIDE_ADDITIONAL_PASSES_ONLY);
}



void StealthEffectClass::Render_Pop(RenderInfoClass &rinfo)
{
	if (!RenderBaseMaterial)
		rinfo.Pop_Override_Flags();
	
	if (RenderStealthMaterial)
		rinfo.Pop_Material_Pass();
}



void StealthEffectClass::Set_Friendly(bool on)
{
	IsFriendly = on;
	Update_Target_Fraction();
}



void StealthEffectClass::Set_Broken(bool on)
{
	IsBroken = on;
	Update_Target_Fraction();
}



RENEGADE_FUNCTION
bool StealthEffectClass::Load(ChunkLoadClass &loader)
AT2(0x0077DCF0,0x0077D590);

void StealthEffectClass::Damage_Occured()
{
	if (IsStealthEnabled)
	{
		if (CurrentFraction > 0.6f)
		{
			CurrentFraction = 0.6f;
		}
	}
}
