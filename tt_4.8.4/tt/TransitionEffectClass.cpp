#include "General.h"
#include "TransitionEffectClass.h"
#include "RenderInfoClass.h"
#include "engine_3dre.h"
TransitionEffectClass::TransitionEffectClass() : CurrentParameter(0), TargetParameter(1), ParameterVelocity(1), MinUVRate(0.5,-0.5), MaxUVRate(2.75,-3), UVRate(0,0), MaxIntensity(1), StartDelay(0), RemoveOnComplete(false), RenderBaseMaterial(false), RenderTransitionMaterial(true), IntensityScale(1), UVOffset(0,0), Mapper(0), MaterialPass(0)
{
	MaterialPass = new MaterialPassClass();
	MaterialPass->EnableOnTranslucentMeshes = false;
	VertexMaterialClass *material = new VertexMaterialClass();
	Vector3 v(0,0,0);
	Vector3 v2(1,1,1);
	material->Set_Ambient(v);
	material->Set_Diffuse(v);
	material->Set_Specular(v);
	material->Set_Emissive(v2);
	material->Set_Opacity(1);
	material->Set_Shininess(0);
	material->CRCDirty = true;
	material->UseLighting = true;
	Mapper = new MatrixMapperClass();
	Mapper->Set_Type(MatrixMapperClass::ORTHO_PROJECTION);
	Mapper->Set_Texture_Transform(Matrix4::IDENTITY,64);
	material->Set_Mapper(Mapper,0);
	MaterialPass->Set_Material(material);
	material->Release_Ref();
	MaterialPass->Set_Shader(ShaderClass::_PresetAdditiveShader);
}

TransitionEffectClass::~TransitionEffectClass()
{
	REF_PTR_RELEASE(MaterialPass);
	REF_PTR_RELEASE(Mapper);
}

RENEGADE_FUNCTION
void TransitionEffectClass::Timestep(float)
AT2(0x00783600, 0x00782EA0);

void TransitionEffectClass::Render_Push(RenderInfoClass &rinfo,PhysClass *phys)
{
	if (RenderTransitionMaterial)
	{
		Vector3 v(IntensityScale,IntensityScale,IntensityScale);
		MaterialPass->Material->Set_Emissive(v);
		Matrix4 mappertransform = Matrix4::IDENTITY;
		mappertransform[0][3] = UVOffset.X;
		mappertransform[1][3] = UVOffset.Y;
		Mapper->Set_Texture_Transform(mappertransform, 64.f);
		rinfo.Push_Material_Pass(MaterialPass);
	}

	if (!RenderBaseMaterial)
		rinfo.Push_Override_Flags(RenderInfoClass::RINFO_OVERRIDE_ADDITIONAL_PASSES_ONLY);
}

void TransitionEffectClass::Render_Pop(RenderInfoClass &rinfo)
{
	if (!RenderBaseMaterial)
	{
		 rinfo.Pop_Override_Flags();
	}
	if (RenderTransitionMaterial)
	{
		rinfo.Pop_Material_Pass();
	}
}

