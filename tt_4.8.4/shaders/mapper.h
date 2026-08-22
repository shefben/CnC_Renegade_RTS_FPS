#ifndef TT_INCLUDE__MAPPER_H
#define TT_INCLUDE__MAPPER_H
#include "engine_vector.h"
#include "engine_3dre.h"


class MatrixMapperClass : public TextureMapperClass
{
public:
	enum
	{
		INVERT_DEPTH_GRADIENT = 1,
	};
	enum MappingType
	{
		ORTHO_PROJECTION = 0,
		PERSPECTIVE_PROJECTION = 1,
		DEPTH_GRADIENT = 2,
		NORMAL_GRADIENT = 3,
	};
protected:
	unsigned long Flags; // 000C  0024
	MappingType Type; // 0010  0028
	Matrix4 ViewToTexture; // 0014  002C
	Matrix4 ViewToPixel; // 0054  006C
	Vector3 ViewSpaceProjectionNormal; // 0094  00AC
	float GradientUCoord; // 00A0  00B8
public:
	SHADERS_API MatrixMapperClass(int stage = 0);
	void Update_View_To_Pixel_Transform(float texsize);
	SHADERS_API void Set_Texture_Transform(const Matrix4& view_to_texture, float texsize);
	void Compute_Texture_Coordinate(const Vector3& point,Vector3* set_stq);
	~MatrixMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void SHADERS_API Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	void Set_Type(MappingType type)
	{
		Type = type;
	}
};

class UVTextureMapperClass : public TextureMapperClass
{
public:
	UVTextureMapperClass(unsigned int stage): TextureMapperClass(stage) {};
	UVTextureMapperClass(UVTextureMapperClass& src);
	UVTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~UVTextureMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class ScaleTextureMapperClass : public TextureMapperClass
{
protected:
	Vector2 Scale; // 0014  002C
public:
	ScaleTextureMapperClass(ScaleTextureMapperClass& src);
	ScaleTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~ScaleTextureMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
}; // 001C  0034

class LinearOffsetTextureMapperClass : public ScaleTextureMapperClass
{
public:
	Vector2 CurrentUVOffset; // 001C  0034
	Vector2 UVOffsetDeltaPerMS; // 0024  003C
	unsigned int LastUsedSyncTime; // 002C  0044
public:
	LinearOffsetTextureMapperClass(LinearOffsetTextureMapperClass& src);
	LinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~LinearOffsetTextureMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);

	void Set_UV_Offset_Delta(const Vector2 &per_sec)  {
		UVOffsetDeltaPerMS = per_sec;
		UVOffsetDeltaPerMS *= -0.001f;
	}
}; // 0030  0048

class GridTextureMapperClass : public TextureMapperClass
{
protected:
	int Sign;
	unsigned int MSPerFrame;
	float OOGridWidth;
	unsigned int GridWidthLog2;
	unsigned int LastFrame;
	unsigned int Remainder;
	unsigned int CurrentFrame;
	unsigned int LastUsedSyncTime;
public:
	GridTextureMapperClass(GridTextureMapperClass& src);
	GridTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	void Set_Frame_Per_Second(float fps);
	void initialize(float fps,unsigned int gridwidth_log2);
	void update_temporal_state();
	void calculate_uv_offset(float* u_offset,float* v_offset);
	~GridTextureMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class SineLinearOffsetTextureMapperClass : public TextureMapperClass
{
protected:
	Vector3 UAFP;
	Vector3 VAFP;
	float CurrentAngle;
	unsigned int LastUsedSyncTime;
public:
	SineLinearOffsetTextureMapperClass(SineLinearOffsetTextureMapperClass& src);
	SineLinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~SineLinearOffsetTextureMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class StepLinearOffsetTextureMapperClass : public TextureMapperClass
{
protected:
	Vector2 Step;
	float StepsPerSec;
	Vector2 CurrentStep;
	float CurrentStepsPerSec;
	unsigned int LastUsedSyncTime;
public:
	StepLinearOffsetTextureMapperClass(StepLinearOffsetTextureMapperClass& src);
	StepLinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~StepLinearOffsetTextureMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class ZigZagLinearOffsetTextureMapperClass : public TextureMapperClass
{
protected:
	Vector2 Speed;
	float Period;
	float Period2;
	float CurrentSync;
	unsigned int LastUsedSyncTime;
public:
	ZigZagLinearOffsetTextureMapperClass(ZigZagLinearOffsetTextureMapperClass& src);
	ZigZagLinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~ZigZagLinearOffsetTextureMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class ClassicEnvironmentMapperClass : public TextureMapperClass
{
public:
	ClassicEnvironmentMapperClass(ClassicEnvironmentMapperClass& src);
	ClassicEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage);
	~ClassicEnvironmentMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	bool Needs_Normals();
};

class EnvironmentMapperClass : public TextureMapperClass
{
public:
	EnvironmentMapperClass(EnvironmentMapperClass& src);
	EnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage);
	~EnvironmentMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	bool Needs_Normals();
};

class EdgeMapperClass : public TextureMapperClass
{
protected:
	unsigned int LastUsedSyncTime;
	float VSpeed;
	float VOffset;
	bool UseReflect;
public:
	EdgeMapperClass(EdgeMapperClass& src);
	EdgeMapperClass(INIClass& ini,char* section,unsigned int stage);
	~EdgeMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	bool Needs_Normals();
};

class WSClassicEnvironmentMapperClass : public TextureMapperClass
{
public:
	WSClassicEnvironmentMapperClass(WSClassicEnvironmentMapperClass& src);
	WSClassicEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage);
	~WSClassicEnvironmentMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	bool Needs_Normals();
};

class WSEnvironmentMapperClass : public TextureMapperClass
{
public:
	WSEnvironmentMapperClass(WSEnvironmentMapperClass& src);
	WSEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage);
	~WSEnvironmentMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	bool Needs_Normals();
};

class RandomTextureMapperClass : public TextureMapperClass
{
protected:
	float unk40;
	float FPS;
	float CurrentAngle;
	Vector2 Center;
	Vector2 Speed;
	unsigned int LastUsedSyncTime;
public:
	RandomTextureMapperClass(RandomTextureMapperClass& src);
	RandomTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~RandomTextureMapperClass();
	void randomize();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class RotateTextureMapperClass : public ScaleTextureMapperClass
{
protected:
	float CurrentAngle;
	float RadiansPerMilliSec;
	Vector2 Center;
	unsigned int LastUsedSyncTime;
public:
	RotateTextureMapperClass(RotateTextureMapperClass& src);
	RotateTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~RotateTextureMapperClass();
	void Reset();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class BumpEnvTextureMapperClass : public LinearOffsetTextureMapperClass
{
protected:
	unsigned int LastUsedSyncTime;
	float CurrentAngle;
	float RadiansPerSecond;
	float ScaleFactor;
public:
	BumpEnvTextureMapperClass(BumpEnvTextureMapperClass& src);
	BumpEnvTextureMapperClass(INIClass& ini,char* section,unsigned int stage);
	~BumpEnvTextureMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Is_Time_Variant();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class ScreenMapperClass : public LinearOffsetTextureMapperClass
{
public:
	ScreenMapperClass(ScreenMapperClass& src);
	ScreenMapperClass(LinearOffsetTextureMapperClass& src);
	ScreenMapperClass(INIClass& ini,char* section,unsigned int stage);
	~ScreenMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class GridClassicEnvironmentMapperClass : public GridTextureMapperClass
{
public:
	GridClassicEnvironmentMapperClass(GridClassicEnvironmentMapperClass& src);
	GridClassicEnvironmentMapperClass(GridTextureMapperClass& src);
	GridClassicEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage);
	~GridClassicEnvironmentMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	bool Needs_Normals();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
};

class GridEnvironmentMapperClass : public GridTextureMapperClass
{
public:
	GridEnvironmentMapperClass(GridEnvironmentMapperClass& src);
	GridEnvironmentMapperClass(GridTextureMapperClass& src);
	GridEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage);
	~GridEnvironmentMapperClass();
	TextureMapperClass *Clone();
	int Mapper_ID();
	void Apply(int uv_array_index);
	void ApplyToMatrix(Matrix4& mat,int uv_array_index);
	bool Needs_Normals();
};
#endif
