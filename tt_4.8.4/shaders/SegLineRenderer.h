#ifndef TT_INCLUDE__SEGLINERENDERER_H
#define TT_INCLUDE__SEGLINERENDERER_H
#include "ParticleEmitterDefClass.h"
#include "texture.h"
#define MAX_SEGLINE_SUBDIV_LEVELS 7
class RenderInfoClass;
class SphereClass;
class TextureClass;
class SegLineRendererClass {
public:
	SegLineRendererClass(void);
	SegLineRendererClass(const SegLineRendererClass & that);
	SegLineRendererClass & operator = (const SegLineRendererClass & that);
	~SegLineRendererClass(void);
	enum TextureMapMode {
		UNIFORM_WIDTH_TEXTURE_MAP =	0x00000000,
		UNIFORM_LENGTH_TEXTURE_MAP =	0x00000001,
		TILED_TEXTURE_MAP =				0x00000002
	};
	void					Init(const W3dEmitterLinePropertiesStruct & props);
	TextureClass *		Get_Texture(void) const
	{
		Texture->Add_Ref();
		return Texture;
	}
	TextureClass *		Peek_Texture(void) const								{ return Texture; }
	ShaderClass			Get_Shader(void) const									{ return Shader; }
	float					Get_Width(void) const									{ return Width; }
	const Vector3 &	Get_Color(void) const 									{ return Color; }
	float					Get_Opacity(void) const									{ return Opacity; }
	float					Get_Noise_Amplitude(void) const						{ return NoiseAmplitude; }
	float					Get_Merge_Abort_Factor(void) const					{ return MergeAbortFactor; }
	unsigned int		Get_Current_Subdivision_Level(void)	const			{ return SubdivisionLevel; }
	TextureMapMode		Get_Texture_Mapping_Mode(void) const;
	float					Get_Texture_Tile_Factor(void) const					{ return TextureTileFactor; }
	Vector2				Get_UV_Offset_Rate(void) const;
	int					Is_Merge_Intersections(void) const					{ return Bits & MERGE_INTERSECTIONS; }
	int					Is_Freeze_Random(void) const							{ return Bits & FREEZE_RANDOM; }
	int					Is_Sorting_Disabled(void) const						{ return Bits & DISABLE_SORTING; }
	int					Are_End_Caps_Enabled(void)	const						{ return Bits & END_CAPS; }
	void					Set_Texture(TextureClass *texture);
	void					Set_Shader(ShaderClass shader)						{ Shader = shader; }
	void					Set_Width(float width)									{ Width = width; }
	void					Set_Color(const Vector3 &color)						{ Color = color; }
	void					Set_Opacity(float opacity)								{ Opacity = opacity; }
	void					Set_Noise_Amplitude(float amplitude)				{ NoiseAmplitude = amplitude; }
	void					Set_Merge_Abort_Factor(float factor)				{ MergeAbortFactor = factor; }
	void					Set_Current_Subdivision_Level(unsigned int lv)	{ SubdivisionLevel = lv; }
	void					Set_Texture_Mapping_Mode(TextureMapMode mode);
	void					Set_Texture_Tile_Factor(float factor);
	void					Set_Current_UV_Offset(const Vector2 & offset);
	void					Set_UV_Offset_Rate(const Vector2 &rate);
	void					Set_Merge_Intersections(int onoff)					{ if (onoff) { Bits |= MERGE_INTERSECTIONS; } else { Bits &= ~MERGE_INTERSECTIONS; }; }
	void					Set_Freeze_Random(int onoff)							{ if (onoff) { Bits |= FREEZE_RANDOM; } else { Bits &= ~FREEZE_RANDOM; }; }
	void					Set_Disable_Sorting(int onoff)						{ if (onoff) { Bits |= DISABLE_SORTING; } else { Bits &= ~DISABLE_SORTING; }; }
	void					Set_End_Caps(int onoff)									{ if (onoff) { Bits |= END_CAPS; } else { Bits &= ~END_CAPS; }; }
	void					Render(	RenderInfoClass & rinfo,
										const Matrix3D & transform,
										unsigned int point_count,
										Vector3 * points,
										const SphereClass & obj_sphere);
	void					Reset_Line(void);
private:
	void								subdivision_util(unsigned int point_cnt, const Vector3 *xformed_pts,
											const float *base_tex_v, unsigned int *p_sub_point_cnt,
											Vector3 *xformed_subdiv_pts, float *subdiv_tex_v);
	TextureClass *					Texture;
	ShaderClass						Shader;
	float								Width;
	Vector3							Color;
	float								Opacity;
	unsigned int					SubdivisionLevel;	
	float								NoiseAmplitude;
	float								MergeAbortFactor;
	float								TextureTileFactor;
	unsigned int					LastUsedSyncTime;
	Vector2							CurrentUVOffset;
	Vector2							UVOffsetDeltaPerMS;
	enum BitShiftOffsets {
		TEXTURE_MAP_MODE_OFFSET = 24
	};
	enum {
		MERGE_INTERSECTIONS =	0x00000001,
		FREEZE_RANDOM =			0x00000002,
		DISABLE_SORTING =			0x00000004,
		END_CAPS =					0x00000008,
		TEXTURE_MAP_MODE_MASK =	0xFF000000,
		DEFAULT_BITS = MERGE_INTERSECTIONS | (UNIFORM_WIDTH_TEXTURE_MAP << TEXTURE_MAP_MODE_OFFSET)
	};
	unsigned int					Bits;
	friend class SegmentedLineClass;
};
inline SegLineRendererClass::TextureMapMode SegLineRendererClass::Get_Texture_Mapping_Mode(void) const
{ 
	return (TextureMapMode)((Bits & TEXTURE_MAP_MODE_MASK) >> TEXTURE_MAP_MODE_OFFSET); 
}
inline void SegLineRendererClass::Set_Texture_Mapping_Mode(SegLineRendererClass::TextureMapMode mode)
{
	Bits &= ~TEXTURE_MAP_MODE_MASK;
	Bits |= ((mode << TEXTURE_MAP_MODE_OFFSET) & TEXTURE_MAP_MODE_MASK);
}
inline Vector2 SegLineRendererClass::Get_UV_Offset_Rate(void) const
{	
	return UVOffsetDeltaPerMS * 1000.0f;
}
inline void SegLineRendererClass::Set_UV_Offset_Rate(const Vector2 &rate)
{
	UVOffsetDeltaPerMS = rate * 0.001f;
}
#endif
