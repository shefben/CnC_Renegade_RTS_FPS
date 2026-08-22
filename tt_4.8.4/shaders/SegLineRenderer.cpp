#include "General.h"
#include "SegLineRenderer.h"
#include "engine_3dre.h"
#include "texture.h"
#include "WW3D.h"
SegLineRendererClass::SegLineRendererClass() : Texture(0), Width(0), Color(1,1,1), Opacity(1), SubdivisionLevel(0), NoiseAmplitude(0), MergeAbortFactor(1.5), TextureTileFactor(1), LastUsedSyncTime(WW3D::SyncTime), CurrentUVOffset(0,0), UVOffsetDeltaPerMS(0,0), Bits(1), Shader(ShaderClass::_PresetAdditiveSpriteShader)
{
}
SegLineRendererClass::SegLineRendererClass(const SegLineRendererClass &src) : Texture(0), Width(0), Color(1,1,1), Opacity(1), SubdivisionLevel(0), NoiseAmplitude(0), MergeAbortFactor(1.5), TextureTileFactor(1), LastUsedSyncTime(WW3D::SyncTime), CurrentUVOffset(0,0), UVOffsetDeltaPerMS(0,0), Bits(1), Shader(ShaderClass::_PresetAdditiveSpriteShader)
{
	*this = src;
}
SegLineRendererClass &SegLineRendererClass::operator=(const SegLineRendererClass& that)
{
	if (this != &that)
	{
		REF_PTR_SET(Texture,that.Texture);
		Shader = that.Shader;
		Width = that.Width;
		Color = that.Color;
		Opacity = that.Opacity;
		SubdivisionLevel = that.SubdivisionLevel;
		NoiseAmplitude = that.NoiseAmplitude;
		MergeAbortFactor = that.MergeAbortFactor;
		TextureTileFactor = that.TextureTileFactor;
		LastUsedSyncTime = that.LastUsedSyncTime;
		CurrentUVOffset = that.CurrentUVOffset;
		UVOffsetDeltaPerMS = that.UVOffsetDeltaPerMS;
		Bits = that.Bits;
	}
	return *this;
}
SegLineRendererClass::~SegLineRendererClass()
{
	REF_PTR_RELEASE(Texture);
}
RENEGADE_FUNCTION
void SegLineRendererClass::Init(const W3dEmitterLinePropertiesStruct& props)
AT1(0x005AEEE0);
void SegLineRendererClass::Set_Texture(TextureClass* texture)
{
	REF_PTR_SET(Texture,texture);
}
RENEGADE_FUNCTION
void SegLineRendererClass::Render(RenderInfoClass &rinfo, const Matrix3D &transform, unsigned int point_count, Vector3 *points, const SphereClass &obj_sphere)
AT1(0x005AF0A0);
