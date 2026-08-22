#ifndef TT_INCLUDE__PARTICLEEMITTERDEFCLASS_H
#define TT_INCLUDE__PARTICLEEMITTERDEFCLASS_H
#include "engine_3dre.h"
#include "engine_io.h"
#include "random3class.h"
typedef IOVector3Struct			W3dVectorStruct;
class Vector3Randomizer {
private:
	float OOIntMax;
	float OOUIntMax;
	Random3Class Randomizer;
public:
	virtual ~Vector3Randomizer();
	virtual unsigned int Class_ID() = 0;
	virtual void Get_Vector(Vector3&) = 0;
	virtual float Get_Maximum_Extent() = 0;
	virtual void Scale(float) = 0;
	virtual Vector3Randomizer *Clone() = 0;
};
struct W3dRGBAStruct
{
	uint8			R;
	uint8			G;
	uint8			B;
	uint8			A;
};
struct W3dEmitterInfoStruct
{
	char					TextureFilename[260];
	float				StartSize;
	float				EndSize;
	float				Lifetime;
	float				EmissionRate;
	float				MaxEmissions;
	float				VelocityRandom;
	float				PositionRandom;
	float				FadeTime;
	float				Gravity;
	float				Elasticity;
	W3dVectorStruct	Velocity;
	W3dVectorStruct	Acceleration;
	W3dRGBAStruct		StartColor;
	W3dRGBAStruct		EndColor;
};
struct W3dVolumeRandomizerStruct
{
	uint32				ClassID;
	float				Value1;
	float				Value2;
	float				Value3;
	uint32				reserved[4];
};
struct W3dShaderStruct
{
	W3dShaderStruct(void) {}
	uint8						DepthCompare;
	uint8						DepthMask;
	uint8						ColorMask;		// now obsolete and ignored
	uint8						DestBlend;
	uint8						FogFunc;			// now obsolete and ignored
	uint8						PriGradient;
	uint8						SecGradient;
	uint8						SrcBlend;
	uint8						Texturing;
	uint8						DetailColorFunc;
	uint8						DetailAlphaFunc;
	uint8						ShaderPreset;	// now obsolete and ignored
	uint8						AlphaTest;
	uint8						PostDetailColorFunc;
	uint8						PostDetailAlphaFunc;
	uint8						pad[1];
};
struct W3dEmitterInfoStructV2
{
	uint32							BurstSize;
	W3dVolumeRandomizerStruct	CreationVolume;
	W3dVolumeRandomizerStruct	VelRandom;
	float							OutwardVel;
	float							VelInherit;
	W3dShaderStruct				Shader;
	uint32							RenderMode;		// render as particles or lines?
	uint32							FrameMode;		// chop the texture into a grid of smaller squares?
	uint32							reserved[6];
};
struct W3dEmitterLinePropertiesStruct
{
	uint32							Flags;
	uint32							SubdivisionLevel;	
	float							NoiseAmplitude;
	float							MergeAbortFactor;
	float							TextureTileFactor;
	float							UPerSec;
	float							VPerSec;
	uint32							Reserved[9];
};
template <typename T> struct ParticlePropertyStruct {
public:
	T Start;
	T Rand;
	unsigned int NumKeyFrames;
	float *KeyTimes;
	T *Values;
};
class W3dUtilityClass {
public:
	static void Convert_Shader(W3dShaderStruct const &,ShaderClass *);
	static void Convert_Shader(ShaderClass const &,W3dShaderStruct *);
};
#define W3D_ELINE_MERGE_INTERSECTIONS 				0x00000001	// Merge intersections
#define W3D_ELINE_FREEZE_RANDOM						0x00000002	// Freeze random (note: offsets are in camera space)
#define W3D_ELINE_DISABLE_SORTING					0x00000004	// Disable sorting (even if shader has alpha-blending)
#define W3D_ELINE_END_CAPS 							0x00000008	// Draw end caps on the line
#define W3D_ELINE_TEXTURE_MAP_MODE_MASK 			0xFF000000	// Must cover all possible TextureMapMode values
#define W3D_ELINE_TEXTURE_MAP_MODE_OFFSET 		24				// By how many bits do I need to shift the texture mapping mode?
#define W3D_ELINE_UNIFORM_WIDTH_TEXTURE_MAP		0x00000000	// Entire line uses one row of texture (constant V)
#define W3D_ELINE_UNIFORM_LENGTH_TEXTURE_MAP 	0x00000001	// Entire line uses one row of texture stretched length-wise
#define W3D_ELINE_TILED_TEXTURE_MAP					0x00000002	// Tiled continuously over line
#define W3D_ELINE_DEFAULT_BITS	(W3D_ELINE_MERGE_INTERSECTIONS | (W3D_ELINE_UNIFORM_WIDTH_TEXTURE_MAP << W3D_ELINE_TEXTURE_MAP_MODE_OFFSET))
class ParticleEmitterDefClass {
public:
	ParticleEmitterDefClass (void);
	ParticleEmitterDefClass (const ParticleEmitterDefClass &src);
	virtual ~ParticleEmitterDefClass (void);
	const ParticleEmitterDefClass &operator= (const ParticleEmitterDefClass &src);
	virtual WW3DErrorType	Load_W3D (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Save_W3D (ChunkSaveClass &chunk_save);
	const char *				Get_Name (void) const					{ return m_pName; }
	virtual void				Set_Name (const char *pname);
	unsigned int			Get_Version (void) const				{ return m_Version; }
	int						Get_Render_Mode (void) const			{ return m_InfoV2.RenderMode; }	// values defined in w3d_file.h
	int						Get_Frame_Mode (void) const			{ return m_InfoV2.FrameMode; } // values in w3d_file.h
	const char *			Get_Texture_Filename (void) const	{ return m_Info.TextureFilename; }
	float						Get_Lifetime (void) const				{ return m_Info.Lifetime; }
	float						Get_Emission_Rate (void) const		{ return m_Info.EmissionRate; }
	float						Get_Max_Emissions (void) const		{ return m_Info.MaxEmissions; }
	float						Get_Fade_Time (void) const				{ return m_Info.FadeTime; }
	float						Get_Gravity (void) const				{ return m_Info.Gravity; }
	float						Get_Elasticity (void) const			{ return m_Info.Elasticity; }
	Vector3					Get_Velocity (void) const				{ return Vector3 (m_Info.Velocity.X, m_Info.Velocity.Y, m_Info.Velocity.Z); }
	Vector3					Get_Acceleration (void) const			{ return Vector3 (m_Info.Acceleration.X, m_Info.Acceleration.Y, m_Info.Acceleration.Z); }
	unsigned int			Get_Burst_Size (void) const			{ return m_InfoV2.BurstSize; }
	float						Get_Outward_Vel (void) const			{ return m_InfoV2.OutwardVel; }
	float						Get_Vel_Inherit (void) const			{ return m_InfoV2.VelInherit; }
	virtual void			Set_Render_Mode (int mode)						{ m_InfoV2.RenderMode = mode; } // values in w3d_file.h
	virtual void			Set_Frame_Mode (int mode)						{ m_InfoV2.FrameMode = mode; } // values in w3d_file.h 
	virtual void			Set_Texture_Filename (const char *pname);
	virtual void			Set_Lifetime (float value)						{ m_Info.Lifetime = value; }
	virtual void			Set_Emission_Rate (float value)				{ m_Info.EmissionRate = value; }
	virtual void			Set_Max_Emissions (float value)				{ m_Info.MaxEmissions = value; }
	virtual void			Set_Fade_Time (float value)					{ m_Info.FadeTime = value; }
	virtual void			Set_Gravity (float value)						{ m_Info.Gravity = value; }
	virtual void			Set_Elasticity (float value)					{ m_Info.Elasticity = value; }
	virtual void			Set_Velocity (const Vector3 &value)			{ m_Info.Velocity.X = value.X; m_Info.Velocity.Y = value.Y; m_Info.Velocity.Z = value.Z; }
	virtual void			Set_Acceleration (const Vector3 &value)	{ m_Info.Acceleration.X = value.X; m_Info.Acceleration.Y = value.Y; m_Info.Acceleration.Z = value.Z; }
	virtual void			Set_Burst_Size (unsigned int count)			{ m_InfoV2.BurstSize = count; }
	virtual void			Set_Outward_Vel (float value)					{ m_InfoV2.OutwardVel = value; }
	virtual void			Set_Vel_Inherit (float value)					{ m_InfoV2.VelInherit = value; }
	void						Get_Shader (ShaderClass &shader) const		{ W3dUtilityClass::Convert_Shader (m_InfoV2.Shader, &shader); }
	virtual void			Set_Shader (const ShaderClass &shader)		{ W3dUtilityClass::Convert_Shader (shader, &m_InfoV2.Shader); }
	Vector3Randomizer *	Get_Creation_Volume (void) const								{ return m_pCreationVolume->Clone (); }
	Vector3Randomizer *	Get_Velocity_Random (void) const								{ return m_pVelocityRandomizer->Clone (); }
	virtual void			Set_Creation_Volume (Vector3Randomizer *randomizer);
	virtual void			Set_Velocity_Random (Vector3Randomizer *randomizer);
	virtual void			Get_Color_Keyframes (ParticlePropertyStruct<Vector3> &keyframes) const;
	virtual void			Get_Opacity_Keyframes (ParticlePropertyStruct<float> &keyframes) const;
	virtual void			Get_Size_Keyframes (ParticlePropertyStruct<float> &keyframes) const;
	virtual void			Get_Rotation_Keyframes (ParticlePropertyStruct<float> &rotationframes) const;
	virtual void			Get_Frame_Keyframes (ParticlePropertyStruct<float> &frameframes) const;
	virtual void			Get_Blur_Time_Keyframes (ParticlePropertyStruct<float> &blurtimeframes) const;
	virtual float			Get_Initial_Orientation_Random (void) const { return m_InitialOrientationRandom; }
	virtual void			Set_Color_Keyframes (ParticlePropertyStruct<Vector3> &keyframes);
	virtual void			Set_Opacity_Keyframes (ParticlePropertyStruct<float> &keyframes);
	virtual void			Set_Size_Keyframes (ParticlePropertyStruct<float> &keyframes);
	virtual void			Set_Rotation_Keyframes (ParticlePropertyStruct<float> &keyframes, float orient_rnd);
	virtual void			Set_Frame_Keyframes (ParticlePropertyStruct<float> &keyframes);
	virtual void			Set_Blur_Time_Keyframes (ParticlePropertyStruct<float> &keyframes);
	const char *			Get_User_String (void) const					{ return m_pUserString; }
	int						Get_User_Type (void) const						{ return m_iUserType; }
	virtual void			Set_User_String (const char *pstring);
	virtual void			Set_User_Type (int type)						{ m_iUserType = type; }
	const W3dEmitterLinePropertiesStruct * Get_Line_Properties(void) const { return &m_LineProperties; }
	int						Get_Line_Texture_Mapping_Mode(void) const	{ return (m_LineProperties.Flags & W3D_ELINE_TEXTURE_MAP_MODE_MASK) >> W3D_ELINE_TEXTURE_MAP_MODE_OFFSET; }
	int						Is_Merge_Intersections(void) const			{ return m_LineProperties.Flags & W3D_ELINE_MERGE_INTERSECTIONS; }
	int						Is_Freeze_Random(void) const					{ return m_LineProperties.Flags & W3D_ELINE_FREEZE_RANDOM; }
	int						Is_Sorting_Disabled(void) const				{ return m_LineProperties.Flags & W3D_ELINE_DISABLE_SORTING; }
	int						Are_End_Caps_Enabled(void)	const				{ return m_LineProperties.Flags & W3D_ELINE_END_CAPS; }
	int						Get_Subdivision_Level(void) const			{ return m_LineProperties.SubdivisionLevel; }
	float						Get_Noise_Amplitude(void) const				{ return m_LineProperties.NoiseAmplitude; }
	float						Get_Merge_Abort_Factor(void) const			{ return m_LineProperties.MergeAbortFactor; }
	float						Get_Texture_Tile_Factor(void) const			{ return m_LineProperties.TextureTileFactor; }
	Vector2					Get_UV_Offset_Rate(void) const				{ return Vector2(m_LineProperties.UPerSec,m_LineProperties.VPerSec); }
	virtual void			Set_Line_Texture_Mapping_Mode(int mode);
	virtual void			Set_Merge_Intersections(int onoff)			{ if (onoff) { m_LineProperties.Flags |= W3D_ELINE_MERGE_INTERSECTIONS; } else { m_LineProperties.Flags &= ~W3D_ELINE_MERGE_INTERSECTIONS; }; }
	virtual void			Set_Freeze_Random(int onoff)					{ if (onoff) { m_LineProperties.Flags |= W3D_ELINE_FREEZE_RANDOM; } else { m_LineProperties.Flags &= ~W3D_ELINE_FREEZE_RANDOM; }; }
	virtual void			Set_Disable_Sorting(int onoff)				{ if (onoff) { m_LineProperties.Flags |= W3D_ELINE_DISABLE_SORTING; } else { m_LineProperties.Flags &= ~W3D_ELINE_DISABLE_SORTING; }; }
	virtual void			Set_End_Caps(int onoff)							{ if (onoff) { m_LineProperties.Flags |= W3D_ELINE_END_CAPS; } else { m_LineProperties.Flags &= ~W3D_ELINE_END_CAPS; }; }
	virtual void			Set_Subdivision_Level(int lvl)				{ m_LineProperties.SubdivisionLevel = lvl; }
	virtual void			Set_Noise_Amplitude(float k)					{ m_LineProperties.NoiseAmplitude = k; }
	virtual void			Set_Merge_Abort_Factor(float k)				{ m_LineProperties.MergeAbortFactor = k; }
	virtual void			Set_Texture_Tile_Factor(float k)				{ m_LineProperties.TextureTileFactor = k; }
	virtual void			Set_UV_Offset_Rate(const Vector2 & rate)	{ m_LineProperties.UPerSec = rate.X; m_LineProperties.VPerSec = rate.Y; }
protected:
	virtual WW3DErrorType	Read_Header (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_User_Data (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_Info (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_InfoV2 (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_Props (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_Line_Properties (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_Rotation_Keyframes (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_Frame_Keyframes (ChunkLoadClass &chunk_load);
	virtual WW3DErrorType	Read_Blur_Time_Keyframes (ChunkLoadClass &chunk_load);
	virtual bool				Read_Color_Keyframe (ChunkLoadClass &chunk_load, float *key_time, Vector3 *value);
	virtual bool				Read_Opacity_Keyframe (ChunkLoadClass &chunk_load, float *key_time, float *value);
	virtual bool				Read_Size_Keyframe (ChunkLoadClass &chunk_load, float *key_time, float *value);
	virtual WW3DErrorType	Save_Header (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_User_Data (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_Info (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_InfoV2 (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_Props (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_Line_Properties (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_Frame_Keyframes (ChunkSaveClass & chunk_save);
	virtual WW3DErrorType	Save_Rotation_Keyframes (ChunkSaveClass & chunk_save);
	virtual WW3DErrorType	Save_Blur_Time_Keyframes (ChunkSaveClass & chunk_save);
	virtual WW3DErrorType	Save_Color_Keyframes (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_Opacity_Keyframes (ChunkSaveClass &chunk_save);
	virtual WW3DErrorType	Save_Size_Keyframes (ChunkSaveClass &chunk_save);
	virtual Vector3Randomizer *Create_Randomizer (W3dVolumeRandomizerStruct &info);
	virtual void				Initialize_Randomizer_Struct (const Vector3Randomizer &randomizer, W3dVolumeRandomizerStruct &info);
	virtual void				Initialize_To_Ver2 (void);
	virtual void				Convert_To_Ver2 (void);
	virtual void				Normalize_Filename (void);
	virtual void				Free_Props (void);
private:
		char * 									m_pName;
		char *									m_pUserString;
		int										m_iUserType;
		unsigned	int							m_Version;
		ShaderClass								m_Shader;
		W3dEmitterInfoStruct					m_Info;
		W3dEmitterInfoStructV2				m_InfoV2;
		W3dEmitterLinePropertiesStruct	m_LineProperties;
		ParticlePropertyStruct<Vector3>	m_ColorKeyframes;
		ParticlePropertyStruct<float>		m_OpacityKeyframes;
		ParticlePropertyStruct<float>		m_SizeKeyframes;
		ParticlePropertyStruct<float>		m_RotationKeyframes;
		float										m_InitialOrientationRandom;
		ParticlePropertyStruct<float>		m_FrameKeyframes;
		ParticlePropertyStruct<float>		m_BlurTimeKeyframes;
		Vector3Randomizer *					m_pCreationVolume;
		Vector3Randomizer *					m_pVelocityRandomizer;
};
inline void ParticleEmitterDefClass::Set_Line_Texture_Mapping_Mode(int mode)	
{ 
	m_LineProperties.Flags &= ~W3D_ELINE_TEXTURE_MAP_MODE_MASK;
	m_LineProperties.Flags |= ((mode << W3D_ELINE_TEXTURE_MAP_MODE_OFFSET) & W3D_ELINE_TEXTURE_MAP_MODE_MASK); 
}
#endif
