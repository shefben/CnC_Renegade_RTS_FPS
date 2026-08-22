#include "general.h"
#include "mapper.h"



#include "engine_3d.h"
#include "shaderstatemanager.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "engine_math.h"
MatrixMapperClass::MatrixMapperClass(int stage) : TextureMapperClass(stage)
{
	Flags = 0;
	ViewToTexture.Make_Identity();
	ViewToPixel.Make_Identity();
	GradientUCoord = 0.5;
	ViewSpaceProjectionNormal.X = 0;
	ViewSpaceProjectionNormal.Y = 0;
	ViewSpaceProjectionNormal.Z = 0;
}



void MatrixMapperClass::Update_View_To_Pixel_Transform(float texsize)
{
	float var_8 = (texsize * .5f - 1.f) / texsize;
	
	ViewToPixel[0] = (ViewToTexture[3] + ViewToTexture[0]) * var_8;
	ViewToPixel[1] = (ViewToTexture[3] - ViewToTexture[1]) * var_8;
	
	if (Flags & 1)
		ViewToPixel[2] = (ViewToTexture[3] - ViewToTexture[2]) * .5f;
	else
		ViewToPixel[2] = (ViewToTexture[3] + ViewToTexture[2]) * .5f;

	ViewToPixel[3] = ViewToTexture[3];
	ViewSpaceProjectionNormal = Normalize(-(Vector3&)ViewToTexture[2]);
}



void MatrixMapperClass::Set_Texture_Transform(const Matrix4& view_to_texture, float texsize)
{
	ViewToTexture = view_to_texture;
	Update_View_To_Pixel_Transform(texsize);
}



MatrixMapperClass::~MatrixMapperClass()
{
}

TextureMapperClass *MatrixMapperClass::Clone()
{
	return 0;
}

int MatrixMapperClass::Mapper_ID()
{
	return MAPPER_ID_MATRIX;
};

void MatrixMapperClass::Apply(int uv_array_index)
{
	Matrix4 matrix;
	switch (Type)
	{
	case ORTHO_PROJECTION:
		DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage + D3DTS_TEXTURE0),ViewToPixel);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEPOSITION);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
		break;
	case PERSPECTIVE_PROJECTION:
		matrix[0] = ViewToPixel[0];
		matrix[1] = ViewToPixel[1];
		matrix[2] = ViewToPixel[3];
		DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage + D3DTS_TEXTURE0),matrix);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEPOSITION);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT3|D3DTTFF_PROJECTED);
		break;
	case DEPTH_GRADIENT:
		matrix[1].X = ViewToPixel[2].X;
		matrix[1].Y = ViewToPixel[2].Y;
		matrix[1].Z = ViewToPixel[2].Z;
		matrix[1].W = ViewToPixel[2].W;
		matrix[0].W = GradientUCoord;
		matrix[0].X = 0;
		matrix[0].Y = 0;
		matrix[0].Z = 0;
		DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage + D3DTS_TEXTURE0),matrix);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEPOSITION);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
		break;
	case NORMAL_GRADIENT:
		matrix[0].W = GradientUCoord;
		matrix[1].X = ViewSpaceProjectionNormal.X;
		matrix[1].Y = ViewSpaceProjectionNormal.Y;
		matrix[1].Z = ViewSpaceProjectionNormal.Z;
		matrix[0].X = 0;
		matrix[0].Y = 0;
		matrix[0].Z = 0;
		matrix[1].W = 0;
		DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage + D3DTS_TEXTURE0),matrix);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACENORMAL);
		StateManager::SetTextureStageState(uv_array_index,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
		break;
	}	
}

void MatrixMapperClass::Compute_Texture_Coordinate(const Vector3& point,Vector3* set_stq)
{
	Matrix4::Transform_Vector(ViewToPixel,point,set_stq);
}

void MatrixMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	switch (Type)
	{
	case ORTHO_PROJECTION:
		mat = ViewToPixel;
		break;
	DEFAULT_UNREACHABLE;
	}
}

ScaleTextureMapperClass::ScaleTextureMapperClass(ScaleTextureMapperClass& src) : TextureMapperClass(src)
{
	Scale = src.Scale;
}

ScaleTextureMapperClass::ScaleTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	Scale.X = ini.Get_Float(section,"UScale",1.0);
	Scale.Y = ini.Get_Float(section,"VScale",1.0);
}

ScaleTextureMapperClass::~ScaleTextureMapperClass()
{
}

TextureMapperClass *ScaleTextureMapperClass::Clone()
{
	return new ScaleTextureMapperClass(*this);
}

int ScaleTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_SCALE;
}

void ScaleTextureMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	mat.Make_Identity();
	mat[0].X = Scale.X;
	mat[1].Y = Scale.Y;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void ScaleTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	mat.Make_Identity();
	mat[0].X = Scale.X;
	mat[1].Y = Scale.Y;
}

LinearOffsetTextureMapperClass::LinearOffsetTextureMapperClass(LinearOffsetTextureMapperClass& src) : ScaleTextureMapperClass(src)
{
	UVOffsetDeltaPerMS = src.UVOffsetDeltaPerMS;
	Reset();
}

LinearOffsetTextureMapperClass::LinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : ScaleTextureMapperClass(ini,section,stage)
{
	Reset();
	UVOffsetDeltaPerMS.X = (ini.Get_Float(section,"UPerSec",0) * -0.001f);
	UVOffsetDeltaPerMS.Y = (ini.Get_Float(section,"VPerSec",0) * -0.001f);
}

LinearOffsetTextureMapperClass::~LinearOffsetTextureMapperClass()
{
}

void LinearOffsetTextureMapperClass::Reset()
{
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentUVOffset.X = 0;
	CurrentUVOffset.Y = 0;
}

TextureMapperClass *LinearOffsetTextureMapperClass::Clone()
{
	return new LinearOffsetTextureMapperClass(*this);
}

int LinearOffsetTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_LINEAR_OFFSET;
}

bool LinearOffsetTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void LinearOffsetTextureMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	float offset_v;
	float offset_u;
	offset_u = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.X) + CurrentUVOffset.X;
	offset_v = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.Y) + CurrentUVOffset.Y;
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat.Make_Identity();
	mat[0].X = Scale.X;
	mat[0].Z = offset_u;
	mat[1].Y = Scale.Y;
	mat[1].Z = offset_v;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentUVOffset.X = offset_u;
	CurrentUVOffset.Y = offset_v;
}

void LinearOffsetTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	mat.Make_Identity();
	float offset_v;
	float offset_u;
	offset_u = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.X) + CurrentUVOffset.X;
	offset_v = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.Y) + CurrentUVOffset.Y;
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat[0].X = Scale.X;
	mat[0].Z = offset_u;
	mat[1].Y = Scale.Y;
	mat[1].Z = offset_v;
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentUVOffset.X = offset_u;
	CurrentUVOffset.Y = offset_v;
}

void GridTextureMapperClass::initialize(float fps, uint coordinateBitCount)
{
	LastUsedSyncTime = WW3D::SyncTime;
	
	uint maxCoordinate = 1 << coordinateBitCount;
	
	if (LastFrame == 0)
		LastFrame = maxCoordinate * maxCoordinate;
	
	GridWidthLog2 = coordinateBitCount;
	OOGridWidth = 1.f / maxCoordinate;
	Remainder = 0;
	
	if (fps == 0)
	{
		Sign = 0;
		MSPerFrame = 1;
		CurrentFrame = 0;
	}
	else
	{
		MSPerFrame = (int)(1000 / abs(fps));
		
		if (fps < 0)
		{
			Sign = -1;
			CurrentFrame = LastFrame - 1;
		}
		else
		{
			Sign = 1;
			CurrentFrame = 0;
		}
	}
}



void GridTextureMapperClass::update_temporal_state()
{
	Remainder += WW3D::SyncTime - LastUsedSyncTime;
	LastUsedSyncTime = WW3D::SyncTime;
	
	CurrentFrame += Remainder / MSPerFrame * Sign;
	CurrentFrame %= LastFrame;
	
	if (CurrentFrame < 0)
		CurrentFrame += LastFrame;
		
	Remainder = Remainder % MSPerFrame;
}



void GridTextureMapperClass::calculate_uv_offset(float* uOffset, float* vOffset)
{
	uint mask = ~(0xFFFFFFFF << GridWidthLog2);
	uint uCell = CurrentFrame & mask;
	uint vCell = (CurrentFrame >> GridWidthLog2) & mask;
	*uOffset = uCell * OOGridWidth;
	*vOffset = vCell * OOGridWidth;
}



GridTextureMapperClass::GridTextureMapperClass(GridTextureMapperClass& src) : TextureMapperClass(src)
{
	Sign = src.Sign;
	MSPerFrame = src.MSPerFrame;
	OOGridWidth = src.OOGridWidth;
	GridWidthLog2 = src.GridWidthLog2;
	LastFrame = src.LastFrame;
	Reset();
}

GridTextureMapperClass::GridTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	float fps;
	unsigned int gridwidth_log2;
	fps = ini.Get_Float(section,"FPS",1);
	gridwidth_log2 = ini.Get_Int(section,"Log2Width",1);
	LastFrame = ini.Get_Int(section,"Last",0);
	initialize(fps,gridwidth_log2);
}

void GridTextureMapperClass::Set_Frame_Per_Second(float fps)
{
	initialize(fps,GridWidthLog2);
}

GridTextureMapperClass::~GridTextureMapperClass()
{
}

void GridTextureMapperClass::Reset()
{
	Remainder = 0;
	if (Sign == -1)
	{
		CurrentFrame = ((1 << (GridWidthLog2 << 1)) - 1);
		LastUsedSyncTime = WW3D::SyncTime;
	}
	else
	{
		CurrentFrame = 0;
		LastUsedSyncTime = WW3D::SyncTime;
	}
}

TextureMapperClass *GridTextureMapperClass::Clone()
{
	return new GridTextureMapperClass(*this);
}

int GridTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_GRID;
}

bool GridTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void GridTextureMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	float u_offset,v_offset;
	update_temporal_state();
	calculate_uv_offset(&u_offset,&v_offset);
	mat.Make_Identity();
	mat[0].Z = u_offset;
	mat[1].Z = v_offset;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void GridTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	float u_offset,v_offset;
	update_temporal_state();
	calculate_uv_offset(&u_offset,&v_offset);
	mat.Make_Identity();
	mat[0].Z = u_offset;
	mat[1].Z = v_offset;
}

SineLinearOffsetTextureMapperClass::SineLinearOffsetTextureMapperClass(SineLinearOffsetTextureMapperClass& src) : TextureMapperClass(src)
{
	UAFP = src.UAFP;
	VAFP = src.VAFP;
	Reset();
}

SineLinearOffsetTextureMapperClass::SineLinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	Reset();
	UAFP.X = ini.Get_Float(section,"UAmp",1.0);
	UAFP.Y = ini.Get_Float(section,"UFreq",1.0);
	UAFP.Z = ini.Get_Float(section,"UPhaze",0);
	VAFP.X = ini.Get_Float(section,"VAmp",1.0);
	VAFP.Y = ini.Get_Float(section,"VFreq",1.0);
	VAFP.Z = ini.Get_Float(section,"VPhaze",0);
}

SineLinearOffsetTextureMapperClass::~SineLinearOffsetTextureMapperClass()
{
}

void SineLinearOffsetTextureMapperClass::Reset()
{
	CurrentAngle = 0;
	LastUsedSyncTime = WW3D::SyncTime;
}

TextureMapperClass *SineLinearOffsetTextureMapperClass::Clone()
{
	return new SineLinearOffsetTextureMapperClass(*this);
}

int SineLinearOffsetTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_SINE_LINEAR_OFFSET;
}

bool SineLinearOffsetTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void SineLinearOffsetTextureMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	int synctime = WW3D::SyncTime;
	float offset_u,offset_v;
	synctime -= LastUsedSyncTime;
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentAngle = (synctime * 0.0062831854f) + CurrentAngle;
	offset_u = (sin((CurrentAngle * UAFP.Y) + (UAFP.Z * 3.1415927f)) * UAFP.X);
	offset_v = (sin((CurrentAngle * VAFP.Y) + (VAFP.Z * 3.1415927f)) * VAFP.X);
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat.Make_Identity();
	mat[0].Z = offset_u;
	mat[1].Z = offset_v;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void SineLinearOffsetTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	int synctime = WW3D::SyncTime;
	float offset_u,offset_v;
	synctime -= LastUsedSyncTime;
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentAngle = (synctime * 0.0062831854f) + CurrentAngle;
	offset_u = (sin((CurrentAngle * UAFP.Y) + (UAFP.Z * 3.1415927f)) * UAFP.X);
	offset_v = (sin((CurrentAngle * VAFP.Y) + (VAFP.Z * 3.1415927f)) * VAFP.X);
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat.Make_Identity();
	mat[0].Z = offset_u;
	mat[1].Z = offset_v;
}

StepLinearOffsetTextureMapperClass::StepLinearOffsetTextureMapperClass(StepLinearOffsetTextureMapperClass& src) : TextureMapperClass(src)
{
	Step = src.Step;
	StepsPerSec = src.StepsPerSec;
	Reset();
}

StepLinearOffsetTextureMapperClass::StepLinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	Step.X = ini.Get_Float(section,"UStep",0);
	Step.Y = ini.Get_Float(section,"VStep",0);
	StepsPerSec = ini.Get_Float(section,"SPS",0) / 1000;
	Reset();
}

StepLinearOffsetTextureMapperClass::~StepLinearOffsetTextureMapperClass()
{
}

void StepLinearOffsetTextureMapperClass::Reset()
{
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentStep.X = 0;
	CurrentStep.Y = 0;
	CurrentStepsPerSec = 0;
}

TextureMapperClass *StepLinearOffsetTextureMapperClass::Clone()
{
	return new StepLinearOffsetTextureMapperClass(*this);
}

int StepLinearOffsetTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_STEP_LINEAR_OFFSET;
}

bool StepLinearOffsetTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void StepLinearOffsetTextureMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	CurrentStepsPerSec = (WW3D::SyncTime - LastUsedSyncTime) + CurrentStepsPerSec;
	LastUsedSyncTime = WW3D::SyncTime;
	int tsps = (int)(CurrentStepsPerSec * StepsPerSec);
	if (tsps)
	{
		CurrentStep.X = (tsps * Step.X) + CurrentStep.X;
		CurrentStep.Y = (tsps * Step.Y) + CurrentStep.Y;
		CurrentStepsPerSec = CurrentStepsPerSec - (tsps / StepsPerSec);
	}
	CurrentStep.X = CurrentStep.X - floor(CurrentStep.X);
	CurrentStep.Y = CurrentStep.Y - floor(CurrentStep.Y);
	mat.Make_Identity();
	mat[0].Z = CurrentStep.X;
	mat[1].Z = CurrentStep.Y;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void StepLinearOffsetTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	CurrentStepsPerSec = (WW3D::SyncTime - LastUsedSyncTime) + CurrentStepsPerSec;
	LastUsedSyncTime = WW3D::SyncTime;
	int tsps = (int)(CurrentStepsPerSec * StepsPerSec);
	if (tsps)
	{
		CurrentStep.X = (tsps * Step.X) + CurrentStep.X;
		CurrentStep.Y = (tsps * Step.Y) + CurrentStep.Y;
		CurrentStepsPerSec = CurrentStepsPerSec - (tsps / StepsPerSec);
	}
	CurrentStep.X = CurrentStep.X - floor(CurrentStep.X);
	CurrentStep.Y = CurrentStep.Y - floor(CurrentStep.Y);
	mat.Make_Identity();
	mat[0].Z = CurrentStep.X;
	mat[1].Z = CurrentStep.Y;
}

ZigZagLinearOffsetTextureMapperClass::ZigZagLinearOffsetTextureMapperClass(ZigZagLinearOffsetTextureMapperClass& src) : TextureMapperClass(src)
{
	Speed = src.Speed;
	Period = src.Period;
	Period2 = src.Period2;
	Reset();
}

ZigZagLinearOffsetTextureMapperClass::ZigZagLinearOffsetTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	Speed.X = ini.Get_Float(section,"UPerSec",0) / 1000;
	Speed.Y = ini.Get_Float(section,"VPerSec",0) / 1000;
	Period = ini.Get_Float(section,"Period",0) * 1000;
	Reset();
	if (Period < 0)
	{
		Period = -Period;
	}
	Period2 = (Period * 0.5f);
}

ZigZagLinearOffsetTextureMapperClass::~ZigZagLinearOffsetTextureMapperClass()
{
}

void ZigZagLinearOffsetTextureMapperClass::Reset()
{
	CurrentSync = 0;
	LastUsedSyncTime = WW3D::SyncTime;
}

TextureMapperClass *ZigZagLinearOffsetTextureMapperClass::Clone()
{
	return new ZigZagLinearOffsetTextureMapperClass(*this);
}

int ZigZagLinearOffsetTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_ZIGZAG_LINEAR_OFFSET;
}

bool ZigZagLinearOffsetTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void ZigZagLinearOffsetTextureMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	float offset_v;
	float offset_u;
	CurrentSync = (WW3D::SyncTime - LastUsedSyncTime) + CurrentSync;
	LastUsedSyncTime = WW3D::SyncTime;
	offset_u = 0;
	offset_v = 0;
	if (Period > 0)
	{
		int i = (int)(CurrentSync / Period);
		CurrentSync = CurrentSync - (i * Period);
		float cs = CurrentSync;
		if (cs > Period2)
		{
			cs = Period - CurrentSync;
		}
		offset_u = cs * Speed.X;
		offset_v = cs * Speed.Y;
	}
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat.Make_Identity();
	mat[0].Z = offset_u;
	mat[1].Z = offset_v;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void ZigZagLinearOffsetTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	float offset_v;
	float offset_u;
	CurrentSync = (WW3D::SyncTime - LastUsedSyncTime) + CurrentSync;
	LastUsedSyncTime = WW3D::SyncTime;
	offset_u = 0;
	offset_v = 0;
	if (Period > 0)
	{
		int i = (int)(CurrentSync / Period);
		CurrentSync = CurrentSync - (i * Period);
		float cs = CurrentSync;
		if (cs > Period2)
		{
			cs = Period - CurrentSync;
		}
		offset_u = cs * Speed.X;
		offset_v = cs * Speed.Y;
	}
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat.Make_Identity();
	mat[0].Z = offset_u;
	mat[1].Z = offset_v;
}

ClassicEnvironmentMapperClass::ClassicEnvironmentMapperClass(ClassicEnvironmentMapperClass& src) : TextureMapperClass(src)
{
}

ClassicEnvironmentMapperClass::ClassicEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
}

ClassicEnvironmentMapperClass::~ClassicEnvironmentMapperClass()
{
}

TextureMapperClass *ClassicEnvironmentMapperClass::Clone()
{
	return new ClassicEnvironmentMapperClass(*this);
}

int ClassicEnvironmentMapperClass::Mapper_ID()
{
	return MAPPER_ID_CLASSIC_ENVIRONMENT;
}

void ClassicEnvironmentMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	mat.Make_Identity();
	mat[0].X = 0.5;
	mat[0].W = 0.5;
	mat[1].Y = 0.5;
	mat[1].W = 0.5;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACENORMAL);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

bool ClassicEnvironmentMapperClass::Needs_Normals()
{
	return true;
}

void ClassicEnvironmentMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	mat.Make_Identity();
	mat[0].X = 0.5;
	mat[0].W = 0.5;
	mat[1].Y = 0.5;
	mat[1].W = 0.5;
}

EnvironmentMapperClass::EnvironmentMapperClass(EnvironmentMapperClass& src) : TextureMapperClass(src)
{
}

EnvironmentMapperClass::EnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
}

EnvironmentMapperClass::~EnvironmentMapperClass()
{
}

TextureMapperClass *EnvironmentMapperClass::Clone()
{
	return new EnvironmentMapperClass(*this);
}

int EnvironmentMapperClass::Mapper_ID()
{
	return MAPPER_ID_ENVIRONMENT;
}

void EnvironmentMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	mat.Make_Identity();
	mat[0].X = 0.25;
	mat[0].W = 0.5;
	mat[1].Y = 0.25;
	mat[1].W = 0.5;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

bool EnvironmentMapperClass::Needs_Normals()
{
	return true;
}

void EnvironmentMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	mat.Make_Identity();
	mat[0].X = 0.25;
	mat[0].W = 0.5;
	mat[1].Y = 0.25;
	mat[1].W = 0.5;
}

EdgeMapperClass::EdgeMapperClass(EdgeMapperClass& src) : TextureMapperClass(src)
{
	LastUsedSyncTime = WW3D::SyncTime;
	VSpeed = src.VSpeed;
	VOffset = src.VOffset;
	UseReflect = src.UseReflect;
}

EdgeMapperClass::EdgeMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	LastUsedSyncTime = WW3D::SyncTime;
	VSpeed = 0;
	VOffset = 0;
	UseReflect = false;
	VSpeed = ini.Get_Float(section, "VPerSec", 0.0f);
	VOffset = ini.Get_Float(section, "VStart", 0.0f);
	UseReflect = ini.Get_Bool(section, "UseReflect", false);
}

EdgeMapperClass::~EdgeMapperClass()
{
}

void EdgeMapperClass::Reset()
{
	LastUsedSyncTime = WW3D::SyncTime;
	VOffset = 0;
}

TextureMapperClass *EdgeMapperClass::Clone()
{
	return new EdgeMapperClass(*this);
}

int EdgeMapperClass::Mapper_ID()
{
	return MAPPER_ID_EDGE;
}

bool EdgeMapperClass::Is_Time_Variant()
{
	return true;
}

void EdgeMapperClass::Apply(int uv_array_index)
{
	unsigned int now = WW3D::SyncTime;
	float delta = (now - LastUsedSyncTime) * 0.001f;
	LastUsedSyncTime = now;
	VOffset += delta * VSpeed;
	VOffset -= floor(VOffset);
	Matrix3D matenv(	0.0f, 0.0f, 0.5f, 0.5f,
							0.0f, 0.0f, 0.0f, VOffset,
							0.0f, 0.0f, 1.0f, 0.0f );	
	Matrix4 mat(matenv);
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	if (UseReflect)
	{
		StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	}
	else
	{
		StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACENORMAL);
	}
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

bool EdgeMapperClass::Needs_Normals()
{
	return true;
}

void EdgeMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	unsigned int now = WW3D::SyncTime;
	float delta = (now - LastUsedSyncTime) * 0.001f;
	LastUsedSyncTime = now;
	VOffset += delta * VSpeed;
	VOffset -= floor(VOffset);
	Matrix3D matenv(	0.0f, 0.0f, 0.5f, 0.5f,
							0.0f, 0.0f, 0.0f, VOffset,
							0.0f, 0.0f, 1.0f, 0.0f );	
	mat = Matrix4(matenv);
}

WSClassicEnvironmentMapperClass::WSClassicEnvironmentMapperClass(WSClassicEnvironmentMapperClass& src) : TextureMapperClass(src)
{
}

WSClassicEnvironmentMapperClass::WSClassicEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
}

WSClassicEnvironmentMapperClass::~WSClassicEnvironmentMapperClass()
{
}

TextureMapperClass *WSClassicEnvironmentMapperClass::Clone()
{
	return new WSClassicEnvironmentMapperClass(*this);
}

int WSClassicEnvironmentMapperClass::Mapper_ID()
{
	return MAPPER_ID_WS_CLASSIC_ENVIRONMENT;
}

void WSClassicEnvironmentMapperClass::Apply(int uv_array_index)
{
	Matrix4 view = Transforms.view;
	Matrix4 mat;
	Matrix3D matenv;
	Matrix3D viewmat;
	Matrix3D out;
	matenv[0].X = 0.5;
	matenv[0].Y = 0;
	matenv[0].Z = 0;
	matenv[0].W = 0.5;
	matenv[1].X = 0;
	matenv[1].Y = 0.5;
	matenv[1].Z = 0;
	matenv[1].W = 0.5;
	matenv[2].X = 0;
	matenv[2].Y = 0;
	matenv[2].Z = 1.0;
	matenv[2].W = 0;
	viewmat[0].Y = view[0].Y;
	viewmat[0].Z = view[0].Z;
	viewmat[1].X = view[1].X;
	viewmat[1].Y = view[1].Y;
	viewmat[1].Z = view[1].Z;
	viewmat[2].X = view[2].X;
	viewmat[2].Y = view[2].Y;
	viewmat[2].Z = view[2].Z;
	viewmat[0].X = view[0].X;
	viewmat[0].W = 0;
	viewmat[1].W = 0;
	viewmat[2].W = 0;
	out = viewmat * matenv;
	mat[0].X = out[0].X;
	mat[0].Y = out[0].Y;
	mat[0].Z = out[0].Z;
	mat[0].W = out[0].W;
	mat[1].X = out[1].X;
	mat[1].Y = out[1].Y;
	mat[1].Z = out[1].Z;
	mat[1].W = out[1].W;
	mat[2].X = out[2].X;
	mat[2].Y = out[2].Y;
	mat[2].Z = out[2].Z;
	mat[2].W = out[2].W;
	mat[3].X = 0;
	mat[3].Y = 0;
	mat[3].Z = 0;
	mat[3].W = 1.0;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACENORMAL);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

bool WSClassicEnvironmentMapperClass::Needs_Normals()
{
	return true;
}

void WSClassicEnvironmentMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	Matrix4 view = Transforms.view;
	Matrix3D matenv;
	Matrix3D viewmat;
	Matrix3D out;
	matenv[0].X = 0.5;
	matenv[0].Y = 0;
	matenv[0].Z = 0;
	matenv[0].W = 0.5;
	matenv[1].X = 0;
	matenv[1].Y = 0.5;
	matenv[1].Z = 0;
	matenv[1].W = 0.5;
	matenv[2].X = 0;
	matenv[2].Y = 0;
	matenv[2].Z = 1.0;
	matenv[2].W = 0;
	viewmat[0].Y = view[0].Y;
	viewmat[0].Z = view[0].Z;
	viewmat[1].X = view[1].X;
	viewmat[1].Y = view[1].Y;
	viewmat[1].Z = view[1].Z;
	viewmat[2].X = view[2].X;
	viewmat[2].Y = view[2].Y;
	viewmat[2].Z = view[2].Z;
	viewmat[0].X = view[0].X;
	viewmat[0].W = 0;
	viewmat[1].W = 0;
	viewmat[2].W = 0;
	out = viewmat * matenv;
	mat[0].X = out[0].X;
	mat[0].Y = out[0].Y;
	mat[0].Z = out[0].Z;
	mat[0].W = out[0].W;
	mat[1].X = out[1].X;
	mat[1].Y = out[1].Y;
	mat[1].Z = out[1].Z;
	mat[1].W = out[1].W;
	mat[2].X = out[2].X;
	mat[2].Y = out[2].Y;
	mat[2].Z = out[2].Z;
	mat[2].W = out[2].W;
	mat[3].X = 0;
	mat[3].Y = 0;
	mat[3].Z = 0;
	mat[3].W = 1.0;
}

WSEnvironmentMapperClass::WSEnvironmentMapperClass(WSEnvironmentMapperClass& src) : TextureMapperClass(src)
{
}

WSEnvironmentMapperClass::WSEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
}

WSEnvironmentMapperClass::~WSEnvironmentMapperClass()
{
}

TextureMapperClass *WSEnvironmentMapperClass::Clone()
{
	return new WSEnvironmentMapperClass(*this);
}

int WSEnvironmentMapperClass::Mapper_ID()
{
	return MAPPER_ID_WS_ENVIRONMENT;
}

void WSEnvironmentMapperClass::Apply(int uv_array_index)
{
	Matrix4 view = Transforms.view;
	Matrix4 mat;
	Matrix3D matenv;
	Matrix3D viewmat;
	Matrix3D out;
	matenv[0].X = 0.25;
	matenv[0].Y = 0;
	matenv[0].Z = 0;
	matenv[0].W = 0.5;
	matenv[1].X = 0;
	matenv[1].Y = 0.25;
	matenv[1].Z = 0;
	matenv[1].W = 0.5;
	matenv[2].X = 0;
	matenv[2].Y = 0;
	matenv[2].Z = 1.0;
	matenv[2].W = 0;
	viewmat[0].Y = view[0].Y;
	viewmat[0].Z = view[0].Z;
	viewmat[1].X = view[1].X;
	viewmat[1].Y = view[1].Y;
	viewmat[1].Z = view[1].Z;
	viewmat[2].X = view[2].X;
	viewmat[2].Y = view[2].Y;
	viewmat[2].Z = view[2].Z;
	viewmat[0].X = view[0].X;
	viewmat[0].W = 0;
	viewmat[1].W = 0;
	viewmat[2].W = 0;
	out = viewmat * matenv;
	mat[0].X = out[0].X;
	mat[0].Y = out[0].Y;
	mat[0].Z = out[0].Z;
	mat[0].W = out[0].W;
	mat[1].X = out[1].X;
	mat[1].Y = out[1].Y;
	mat[1].Z = out[1].Z;
	mat[1].W = out[1].W;
	mat[2].X = out[2].X;
	mat[2].Y = out[2].Y;
	mat[2].Z = out[2].Z;
	mat[2].W = out[2].W;
	mat[3].X = 0;
	mat[3].Y = 0;
	mat[3].Z = 0;
	mat[3].W = 1.0;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

bool WSEnvironmentMapperClass::Needs_Normals()
{
	return true;
}

void WSEnvironmentMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	Matrix4 view = Transforms.view;
	Matrix3D matenv;
	Matrix3D viewmat;
	Matrix3D out;
	matenv[0].X = 0.25;
	matenv[0].Y = 0;
	matenv[0].Z = 0;
	matenv[0].W = 0.5;
	matenv[1].X = 0;
	matenv[1].Y = 0.25;
	matenv[1].Z = 0;
	matenv[1].W = 0.5;
	matenv[2].X = 0;
	matenv[2].Y = 0;
	matenv[2].Z = 1.0;
	matenv[2].W = 0;
	viewmat[0].Y = view[0].Y;
	viewmat[0].Z = view[0].Z;
	viewmat[1].X = view[1].X;
	viewmat[1].Y = view[1].Y;
	viewmat[1].Z = view[1].Z;
	viewmat[2].X = view[2].X;
	viewmat[2].Y = view[2].Y;
	viewmat[2].Z = view[2].Z;
	viewmat[0].X = view[0].X;
	viewmat[0].W = 0;
	viewmat[1].W = 0;
	viewmat[2].W = 0;
	out = viewmat * matenv;
	mat[0].X = out[0].X;
	mat[0].Y = out[0].Y;
	mat[0].Z = out[0].Z;
	mat[0].W = out[0].W;
	mat[1].X = out[1].X;
	mat[1].Y = out[1].Y;
	mat[1].Z = out[1].Z;
	mat[1].W = out[1].W;
	mat[2].X = out[2].X;
	mat[2].Y = out[2].Y;
	mat[2].Z = out[2].Z;
	mat[2].W = out[2].W;
	mat[3].X = 0;
	mat[3].Y = 0;
	mat[3].Z = 0;
	mat[3].W = 1.0;
}

RandomTextureMapperClass::RandomTextureMapperClass(RandomTextureMapperClass& src) : TextureMapperClass(src)
{
	FPS = src.FPS;
	Reset();
	Speed = src.Speed;
	randomize();
}

RandomTextureMapperClass::RandomTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
	Reset();
	FPS = ini.Get_Float(section,"FPS",0) / 1000;
	Speed.X = ini.Get_Float(section,"UPerSec",0) / 1000;
	Speed.Y = ini.Get_Float(section,"VPerSec",0) / 1000;
	randomize();
}

class Random4Class {
private:
	unsigned int mt[624];
	int mti;
public:
	float Get_Float();
};

RENEGADE_FUNCTION
float Random4Class::Get_Float()
AT1(0x005E6BA0);

REF_DEF1(rand4, Random4Class, 0x0083D108);

void RandomTextureMapperClass::randomize()
{
	CurrentAngle = rand4.Get_Float() * 6.2831855f;
	Center.X = rand4.Get_Float();
	Center.Y = rand4.Get_Float();
}

RandomTextureMapperClass::~RandomTextureMapperClass()
{
}

void RandomTextureMapperClass::Reset()
{
	CurrentAngle = 0;
	LastUsedSyncTime = WW3D::SyncTime;
}

TextureMapperClass *RandomTextureMapperClass::Clone()
{
	return new RandomTextureMapperClass(*this);
}

int RandomTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_RANDOM;
}

bool RandomTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void RandomTextureMapperClass::Apply(int uv_array_index)
{
	unk40 += WW3D::SyncTime - LastUsedSyncTime;
	LastUsedSyncTime = WW3D::SyncTime;
	if (FPS != 0)
	{
		int v29 = (int)(unk40 * FPS);
		if (v29)
		{
			randomize();
			unk40 -= v29 / FPS;
		}
	}
	
	float cosAngle = cos(CurrentAngle);
	float sinAngle = sin(CurrentAngle);
	
	Matrix4 matrix1;
	Matrix4 matrix2;
	matrix1.Make_Identity();

	matrix2[0].X = cosAngle * matrix1[0].X + sinAngle * matrix1[0].Y;
	matrix2[0].Y = cosAngle * matrix1[0].Y - sinAngle * matrix1[0].X;
	matrix2[1].X = cosAngle * matrix1[1].X + sinAngle * matrix1[1].Y;
	matrix2[1].Y = cosAngle * matrix1[1].Y - sinAngle * matrix1[1].X;
	matrix2[2].X = cosAngle * matrix1[2].X + sinAngle * matrix1[2].Y;
	matrix2[2].Y = cosAngle * matrix1[2].Y - sinAngle * matrix1[2].X;
	
	matrix2[0].Z = fmod(unk40 * Speed.X + Center.X, 1);
	matrix2[1].Z = fmod(unk40 * Speed.Y + Center.Y, 1);
	
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage + D3DTS_TEXTURE0), matrix2);
	StateManager::SetTextureStageState(Stage, D3DTSS_TEXCOORDINDEX, uv_array_index);
	StateManager::SetTextureStageState(Stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
}

void RandomTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	unk40 += WW3D::SyncTime - LastUsedSyncTime;
	LastUsedSyncTime = WW3D::SyncTime;
	if (FPS != 0)
	{
		int v29 = (int)(unk40 * FPS);
		if (v29)
		{
			randomize();
			unk40 -= v29 / FPS;
		}
	}
	
	float cosAngle = cos(CurrentAngle);
	float sinAngle = sin(CurrentAngle);
	
	Matrix4 matrix1;
	matrix1.Make_Identity();
	//???
	mat[0].X = cosAngle * matrix1[0].X + sinAngle * matrix1[0].Y;
	mat[0].Y = cosAngle * matrix1[0].Y - sinAngle * matrix1[0].X;
	mat[1].X = cosAngle * matrix1[1].X + sinAngle * matrix1[1].Y;
	mat[1].Y = cosAngle * matrix1[1].Y - sinAngle * matrix1[1].X;
	mat[2].X = cosAngle * matrix1[2].X + sinAngle * matrix1[2].Y;
	mat[2].Y = cosAngle * matrix1[2].Y - sinAngle * matrix1[2].X;
	
	mat[0].Z = fmod(unk40 * Speed.X + Center.X, 1);
	mat[1].Z = fmod(unk40 * Speed.Y + Center.Y, 1);
}

RotateTextureMapperClass::RotateTextureMapperClass(RotateTextureMapperClass& src) : ScaleTextureMapperClass(src), LastUsedSyncTime(WW3D::SyncTime), RadiansPerMilliSec(src.RadiansPerMilliSec), CurrentAngle(0.0f), Center(src.Center)
{
}

RotateTextureMapperClass::RotateTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : ScaleTextureMapperClass(ini,section,stage),LastUsedSyncTime(WW3D::SyncTime),CurrentAngle(0.0f)
{
	RadiansPerMilliSec = 2*3.141592654f*ini.Get_Float(section,"Speed",0.1f)/1000.0f;
	Center.U=ini.Get_Float(section,"UCenter",0.0f);
	Center.V=ini.Get_Float(section,"VCenter",0.0f);
}

RotateTextureMapperClass::~RotateTextureMapperClass()
{
}

void RotateTextureMapperClass::Reset()
{
	CurrentAngle = 0.0f;
	LastUsedSyncTime = WW3D::SyncTime;
}

TextureMapperClass *RotateTextureMapperClass::Clone()
{
	return new RotateTextureMapperClass(*this);
}

int RotateTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_ROTATE;
}

bool RotateTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void RotateTextureMapperClass::Apply(int uv_array_index)
{
	unsigned int now = WW3D::SyncTime;
	unsigned int delta = now - LastUsedSyncTime;
	LastUsedSyncTime = now;
	CurrentAngle += RadiansPerMilliSec * delta;
	CurrentAngle = fmodf(CurrentAngle,2 * 3.141592654f);
	if (CurrentAngle < 0.0f)
	{
		CurrentAngle += 2 * 3.141592654f;
	}
	float c,s;
	c=cos(CurrentAngle);
	s=sin(CurrentAngle);
	Matrix4 m(true);
	m[0].Set(Scale.X*c,-Scale.X*s,-Scale.X*(c*Center.U-s*Center.V-Center.U),0.0f);
	m[1].Set(Scale.Y*s,Scale.Y*c,-Scale.Y*(s*Center.U+c*Center.V-Center.V),0.0f);
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE) (D3DTS_TEXTURE0+Stage),m);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_PASSTHRU | uv_array_index);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void RotateTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	unsigned int now = WW3D::SyncTime;
	unsigned int delta = now - LastUsedSyncTime;
	LastUsedSyncTime = now;
	CurrentAngle += RadiansPerMilliSec * delta;
	CurrentAngle = fmodf(CurrentAngle,2 * 3.141592654f);
	if (CurrentAngle < 0.0f)
	{
		CurrentAngle += 2 * 3.141592654f;
	}
	float c,s;
	c=cos(CurrentAngle);
	s=sin(CurrentAngle);
	mat.Make_Identity();
	mat[0].Set(Scale.X*c,-Scale.X*s,-Scale.X*(c*Center.U-s*Center.V-Center.U),0.0f);
	mat[1].Set(Scale.Y*s,Scale.Y*c,-Scale.Y*(s*Center.U+c*Center.V-Center.V),0.0f);
}

inline float fastSin(float value)
{
	return sin(value);

	/*

	// TODO: This was some kind of a lookup table optimized sin command.

	float value = (1024/2/PI) * ([ebp+44h] + PI/4);
	absvalue = fabs(value);
	
	uint signFill = value >> 1Fh;
	int exponent = absvalue >> 17h - 7Fh;
	int mantissa = absvalue & 7FFFFFh;
	
	maskIfExponentPositive = ~(exponent >> 1Fh);
	
	exponentBits = 1Fh - exponent
	mantissaMask = (1 << exponentBits)--;
	
	uint index = (((int)((mantissa | 800000h) << 8) >> exponentBits) & maskIfExponentPositive) ^ signFill;
	mantissa <<= 8;
	if (mantissa & mantissaMask == 0)
		index += ((absvalue - 1) >> 1Fh) ^ maskIfExponentPositive;
	
	float fraction = value - index;
	
	float value = fraction       * _FastSinTable[(index + 1) & 3FFh] +
	              (1 - fraction) * _FastSinTable[ index      & 3FFh];

	*/
}

BumpEnvTextureMapperClass::BumpEnvTextureMapperClass(BumpEnvTextureMapperClass& src) : LinearOffsetTextureMapperClass(src)
{
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentAngle = 0;
	RadiansPerSecond = src.RadiansPerSecond;
	ScaleFactor = src.ScaleFactor;
}

BumpEnvTextureMapperClass::BumpEnvTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : LinearOffsetTextureMapperClass(ini,section,stage)
{
	LastUsedSyncTime = WW3D::SyncTime;
	CurrentAngle = 0;
	RadiansPerSecond = ini.Get_Float(section,"BumpRotation",0) / 6.2831855f;
	ScaleFactor = ini.Get_Float(section,"BumpScale",1.0);
}

BumpEnvTextureMapperClass::~BumpEnvTextureMapperClass()
{
}

TextureMapperClass *BumpEnvTextureMapperClass::Clone()
{
	return new BumpEnvTextureMapperClass(*this);
}

int BumpEnvTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_BUMPENV;
}

bool BumpEnvTextureMapperClass::Is_Time_Variant()
{
	return true;
}

void BumpEnvTextureMapperClass::Apply(int uv_array_index)
{
	LinearOffsetTextureMapperClass::Apply(uv_array_index);
	
	CurrentAngle += (WW3D::SyncTime - LastUsedSyncTime) * RadiansPerSecond * .001f;
	LastUsedSyncTime = WW3D::SyncTime;
	
	CurrentAngle = fmod(CurrentAngle, (float)WWMATH_PI*2);
	
	float bumpEnv1 = ScaleFactor * fastSin(CurrentAngle + (float)WWMATH_PI/4);
	float bumpEnv3 = ScaleFactor * fastSin(CurrentAngle);
	float bumpEnv2 = -bumpEnv3;
	float bumpEnv4 = bumpEnv1;
	
	StateManager::SetTextureStageState(Stage, D3DTSS_BUMPENVMAT00, F2DW(bumpEnv1));
	StateManager::SetTextureStageState(Stage, D3DTSS_BUMPENVMAT01, F2DW(bumpEnv2));
	StateManager::SetTextureStageState(Stage, D3DTSS_BUMPENVMAT10, F2DW(bumpEnv3));
	StateManager::SetTextureStageState(Stage, D3DTSS_BUMPENVMAT11, F2DW(bumpEnv4));
}

void BumpEnvTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	LinearOffsetTextureMapperClass::ApplyToMatrix(mat,uv_array_index);
}

ScreenMapperClass::ScreenMapperClass(ScreenMapperClass& src) : LinearOffsetTextureMapperClass(src)
{
}

ScreenMapperClass::ScreenMapperClass(LinearOffsetTextureMapperClass& src) : LinearOffsetTextureMapperClass(src)
{
}

ScreenMapperClass::ScreenMapperClass(INIClass& ini,char* section,unsigned int stage) : LinearOffsetTextureMapperClass(ini,section,stage)
{
}

ScreenMapperClass::~ScreenMapperClass()
{
}

TextureMapperClass *ScreenMapperClass::Clone()
{
	return new ScreenMapperClass(*this);
}

int ScreenMapperClass::Mapper_ID()
{
	return MAPPER_ID_SCREEN;
}

void ScreenMapperClass::Apply(int uv_array_index)
{
	float offset_u;
	float offset_v;
	Vector4 last;
	Matrix4 mat = Transforms.projection;
	offset_u = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.X) + CurrentUVOffset.X;
	offset_v = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.Y) + CurrentUVOffset.Y;
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat[0].X = mat[0].X * Scale.X;
	mat[0].Y = mat[0].Y * Scale.X;
	mat[0].Z = mat[0].Z * Scale.X;
	mat[0].W = mat[0].W * Scale.X;
	mat[1].X = mat[1].X * Scale.Y;
	mat[1].Y = mat[1].Y * Scale.Y;
	mat[1].Z = mat[1].Z * Scale.Y;
	mat[1].W = mat[1].W * Scale.Y;
	mat[0].X = mat[3].X * offset_u + mat[0].X;
	mat[0].Y = mat[3].Y * offset_u + mat[0].Y;
	mat[0].Z = mat[3].Z * offset_u + mat[0].Z;
	mat[0].W = mat[3].W * offset_u + mat[0].W;
	mat[1].X = mat[3].X * offset_v + mat[1].X;
	mat[1].Y = mat[3].Y * offset_v + mat[1].Y;
	mat[1].Z = mat[3].Z * offset_v + mat[1].Z;
	mat[1].W = mat[3].W * offset_v + mat[1].W;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEPOSITION);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_PROJECTED|D3DTTFF_COUNT3);
	CurrentUVOffset.X = offset_u;
	CurrentUVOffset.Y = offset_v;
	LastUsedSyncTime = WW3D::SyncTime;
}

void ScreenMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	float offset_u;
	float offset_v;
	Vector4 last;
	offset_u = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.X) + CurrentUVOffset.X;
	offset_v = ((WW3D::SyncTime - LastUsedSyncTime) * UVOffsetDeltaPerMS.Y) + CurrentUVOffset.Y;
	offset_u = offset_u - floor(offset_u);
	offset_v = offset_v - floor(offset_v);
	mat = Transforms.projection;
	mat[0].X = mat[0].X * Scale.X;
	mat[0].Y = mat[0].Y * Scale.X;
	mat[0].Z = mat[0].Z * Scale.X;
	mat[0].W = mat[0].W * Scale.X;
	mat[1].X = mat[1].X * Scale.Y;
	mat[1].Y = mat[1].Y * Scale.Y;
	mat[1].Z = mat[1].Z * Scale.Y;
	mat[1].W = mat[1].W * Scale.Y;
	mat[0].X = mat[3].X * offset_u + mat[0].X;
	mat[0].Y = mat[3].Y * offset_u + mat[0].Y;
	mat[0].Z = mat[3].Z * offset_u + mat[0].Z;
	mat[0].W = mat[3].W * offset_u + mat[0].W;
	mat[1].X = mat[3].X * offset_v + mat[1].X;
	mat[1].Y = mat[3].Y * offset_v + mat[1].Y;
	mat[1].Z = mat[3].Z * offset_v + mat[1].Z;
	mat[1].W = mat[3].W * offset_v + mat[1].W;
	CurrentUVOffset.X = offset_u;
	CurrentUVOffset.Y = offset_v;
	LastUsedSyncTime = WW3D::SyncTime;
}

GridClassicEnvironmentMapperClass::GridClassicEnvironmentMapperClass(GridClassicEnvironmentMapperClass& src) : GridTextureMapperClass(src)
{
}

GridClassicEnvironmentMapperClass::GridClassicEnvironmentMapperClass(GridTextureMapperClass& src) : GridTextureMapperClass(src)
{
}

GridClassicEnvironmentMapperClass::GridClassicEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage) : GridTextureMapperClass(ini,section,stage)
{
}

GridClassicEnvironmentMapperClass::~GridClassicEnvironmentMapperClass()
{
}

TextureMapperClass *GridClassicEnvironmentMapperClass::Clone()
{
	return new GridClassicEnvironmentMapperClass(*this);
}

int GridClassicEnvironmentMapperClass::Mapper_ID()
{
	return MAPPER_ID_GRID_CLASSIC_ENVIRONMENT;
}

bool GridClassicEnvironmentMapperClass::Needs_Normals()
{
	return true;
}

void GridClassicEnvironmentMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	float u_offset,v_offset;
	update_temporal_state();
	calculate_uv_offset(&u_offset,&v_offset);
	mat.Make_Identity();
	float del = OOGridWidth * 0.5f;
	float vdel = del + v_offset;
	float udel = del + u_offset;
	mat.Make_Identity();
	mat[0].X = del;
	mat[1].W = vdel;
	mat[0].W = udel;
	mat[1].Y = del;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACENORMAL);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void GridClassicEnvironmentMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	float u_offset,v_offset;
	update_temporal_state();
	calculate_uv_offset(&u_offset,&v_offset);
	float del = OOGridWidth * 0.5f;
	float vdel = del + v_offset;
	float udel = del + u_offset;
	mat.Make_Identity();
	mat[0].X = del;
	mat[1].W = vdel;
	mat[0].W = udel;
	mat[1].Y = del;
}

GridEnvironmentMapperClass::GridEnvironmentMapperClass(GridEnvironmentMapperClass& src) : GridTextureMapperClass(src)
{
}

GridEnvironmentMapperClass::GridEnvironmentMapperClass(GridTextureMapperClass& src) : GridTextureMapperClass(src)
{
}

GridEnvironmentMapperClass::GridEnvironmentMapperClass(INIClass& ini,char* section,unsigned int stage) : GridTextureMapperClass(ini,section,stage)
{
}

GridEnvironmentMapperClass::~GridEnvironmentMapperClass()
{
}

TextureMapperClass *GridEnvironmentMapperClass::Clone()
{
	return new GridEnvironmentMapperClass(*this);
}

int GridEnvironmentMapperClass::Mapper_ID()
{
	return MAPPER_ID_GRID_ENVIRONMENT;
}

bool GridEnvironmentMapperClass::Needs_Normals()
{
	return true;
}

void GridEnvironmentMapperClass::Apply(int uv_array_index)
{
	Matrix4 mat;
	float u_offset,v_offset;
	update_temporal_state();
	calculate_uv_offset(&u_offset,&v_offset);
	mat.Make_Identity();
	float del = OOGridWidth * 0.5f;
	float vdel = del + v_offset;
	float udel = del + u_offset;
	mat.Make_Identity();
	mat[0].X = del;
	mat[1].W = vdel;
	mat[0].W = udel;
	mat[1].Y = del;
	DX8Wrapper::Set_Transform((D3DTRANSFORMSTATETYPE)(Stage+D3DTS_TEXTURE0),mat);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	StateManager::SetTextureStageState(Stage,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);
}

void GridEnvironmentMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	float u_offset,v_offset;
	update_temporal_state();
	calculate_uv_offset(&u_offset,&v_offset);
	float del = OOGridWidth * 0.5f;
	float vdel = del + v_offset;
	float udel = del + u_offset;
	mat.Make_Identity();
	mat[0].X = del;
	mat[1].W = vdel;
	mat[0].W = udel;
	mat[1].Y = del;
}

UVTextureMapperClass::UVTextureMapperClass(UVTextureMapperClass& src) : TextureMapperClass(src)
{
}

UVTextureMapperClass::UVTextureMapperClass(INIClass& ini,char* section,unsigned int stage) : TextureMapperClass(ini,section,stage)
{
}

UVTextureMapperClass::~UVTextureMapperClass()
{
}

TextureMapperClass *UVTextureMapperClass::Clone()
{
	return new UVTextureMapperClass(*this);
}

int UVTextureMapperClass::Mapper_ID()
{
	return MAPPER_ID_UV;
}

void UVTextureMapperClass::Apply(int uv_array_index)
{
	StateManager::SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,uv_array_index);
	StateManager::SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,0);
}

void UVTextureMapperClass::ApplyToMatrix(Matrix4& mat,int uv_array_index)
{
	mat.Make_Identity();
}



