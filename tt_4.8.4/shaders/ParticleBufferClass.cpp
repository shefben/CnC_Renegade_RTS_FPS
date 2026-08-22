#include "General.h"
#include "ParticleBufferClass.h"
#include "WW3D.h"
#include "ParticleEmitterClass.h"
#include "engine_3dre.h"
#include "SceneClass.h"
#include "PointGroupClass.h"
#include "SegLineRenderer.h"
#include "LineGroupClass.h"
#include "wwmath.h"
#include "VectorProcessorClass.h"
#include "CameraClass.h"
class Random4Class {
private:
	unsigned int mt[624];
	int mti;
public:
	float Get_Float();
};
const unsigned int ParticleBufferClass::PermutationArray[16] = {
	11, 3, 7, 14, 0, 13, 1, 2, 5, 12, 15, 6, 9, 8, 4, 10
};
const static unsigned int MAX_RANDOM_ENTRIES = 32;
unsigned int ParticleBufferClass::TotalActiveCount = 0;
#define NO_MAX_SCREEN_SIZE					WWMATH_FLOAT_MAX
float ParticleBufferClass::LODMaxScreenSizes[17] = {
	NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE,
	NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE,
	NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE,
	NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE, NO_MAX_SCREEN_SIZE,
	NO_MAX_SCREEN_SIZE
};
static Random4Class rand_gen;
const float oo_intmax = 1.0f / (float)INT_MAX;
static const W3dEmitterLinePropertiesStruct _DefaultLineEmitterProps=
{ 0,0,0.0f,1.5f,1.0f,0.0f,0.0f,0,0,0,0,0,0,0,0,0 };
ParticleBufferClass::ParticleBufferClass
(
	ParticleEmitterClass *emitter, 
	unsigned int buffer_size,
	ParticlePropertyStruct<Vector3> &color, 
	ParticlePropertyStruct<float> &opacity,
	ParticlePropertyStruct<float> &size, 
	ParticlePropertyStruct<float> &rotation,
	float orient_rnd,
	ParticlePropertyStruct<float> &frame,
	ParticlePropertyStruct<float> &blurtime,
	Vector3 accel, 
	float max_age, 
	TextureClass *tex,
	ShaderClass shader, 
	bool pingpong,
	int render_mode, 
	int frame_mode,
	const W3dEmitterLinePropertiesStruct * line_props
) :
	NewParticleQueue(NULL),
	NewParticleQueueStart(0U),
	NewParticleQueueEnd(0U),
	NewParticleQueueCount(0U),
	RenderMode(render_mode),
	FrameMode(frame_mode),
	MaxAge((int)(1000.0f * max_age)),
	LastUpdateTime(WW3D::SyncTime),
	IsEmitterDead(false),
	MaxSize(0.0f),
	MaxNum(buffer_size),
	Start(0U),
	End(0U),
	NewEnd(0U),
	NonNewNum(0),
	NewNum(0),
	BoundingBox(Vector3(0,0,0),Vector3(0,0,0)),
	BoundingBoxDirty(true),
	NumColorKeyFrames(0),
	ColorKeyFrameTimes(NULL),
	ColorKeyFrameValues(NULL),
	ColorKeyFrameDeltas(NULL),
	NumAlphaKeyFrames(0),
	AlphaKeyFrameTimes(NULL),
	AlphaKeyFrameValues(NULL),
	AlphaKeyFrameDeltas(NULL),
	NumSizeKeyFrames(0),
	SizeKeyFrameTimes(NULL),
	SizeKeyFrameValues(NULL),
	SizeKeyFrameDeltas(NULL),
	NumRotationKeyFrames(0),
	RotationKeyFrameTimes(NULL),
	RotationKeyFrameValues(NULL),
	HalfRotationKeyFrameDeltas(NULL),
	OrientationKeyFrameValues(NULL),
	NumFrameKeyFrames(0),
	FrameKeyFrameTimes(NULL),
	FrameKeyFrameValues(NULL),
	FrameKeyFrameDeltas(NULL),
	NumBlurTimeKeyFrames(0),
	BlurTimeKeyFrameTimes(NULL),
	BlurTimeKeyFrameValues(NULL),
	BlurTimeKeyFrameDeltas(NULL),
	NumRandomColorEntriesMinus1(0),
	RandomColorEntries(NULL),
	NumRandomAlphaEntriesMinus1(0),
	RandomAlphaEntries(NULL),
	NumRandomSizeEntriesMinus1(0),
	RandomSizeEntries(NULL),
	ColorRandom(0, 0, 0),
	OpacityRandom(0),
	SizeRandom(0),
	RotationRandom(0),
	FrameRandom(0),
	InitialOrientationRandom(0),
	NumRandomRotationEntriesMinus1(0),
	RandomRotationEntries(NULL),
	NumRandomOrientationEntriesMinus1(0),
	RandomOrientationEntries(NULL),
	NumRandomFrameEntriesMinus1(0),
	RandomFrameEntries(NULL),
	NumRandomBlurTimeEntriesMinus1(0),
	RandomBlurTimeEntries(NULL),
	PointGroup(NULL),
	LineRenderer(NULL),
	LineGroup(NULL),
	Diffuse(NULL),
	TailDiffuse(NULL),
	Color(NULL),
	Alpha(NULL),
	Size(NULL),
	Orientation(NULL),
	Frame(NULL),
	UCoord(NULL),
	TailPosition(NULL),
	APT(NULL),
	PingPongPosition(pingpong),
	Velocity(NULL),
	TimeStamp(NULL),
	Emitter(emitter),
	DecimationThreshold(0U),
	ProjectedArea(0.0f),
	DefaultTailDiffuse(0,0,0,0)
{
	LodCount = 17;
	LodBias = 1.0f;
	Position[0] = NULL;
	Position[1] = NULL;
	Reset_Colors(color);
	Reset_Opacity(opacity);
	Reset_Size(size);
	Reset_Rotations(rotation, orient_rnd);
	Reset_Frames(frame);
	Reset_Blur_Times(blurtime);
	NewParticleQueue = new NewParticleStruct[MaxNum];
	Accel = accel;
	HasAccel = (accel.X != 0.0f) || (accel.Y != 0.0f) || (accel.Z != 0.0f);
	shader.Enable_Fog ("ParticleBufferClass");
	switch (RenderMode)	
	{
	case W3D_EMITTER_RENDER_MODE_TRI_PARTICLES:
		{
			PointGroup = new PointGroupClass();
			PointGroup->Set_Flag(PointGroupClass::TRANSFORM, true);
			PointGroup->Set_Texture(tex);			
			PointGroup->Set_Shader(shader);
			PointGroup->Set_Frame_Row_Column_Count_Log2((unsigned char)frame_mode);
			PointGroup->Set_Point_Mode(PointGroupClass::TRIS);
		}
		break;
	case W3D_EMITTER_RENDER_MODE_QUAD_PARTICLES:
		{
			PointGroup = new PointGroupClass();
			PointGroup->Set_Flag(PointGroupClass::TRANSFORM, true);
			PointGroup->Set_Texture(tex);			
			PointGroup->Set_Shader(shader);
			PointGroup->Set_Frame_Row_Column_Count_Log2((unsigned char)frame_mode);
			PointGroup->Set_Point_Mode(PointGroupClass::QUADS);
		}
		break;
	case W3D_EMITTER_RENDER_MODE_LINE:
		{			
			LineRenderer = new SegLineRendererClass;
			LineRenderer->Init(*line_props);
			LineRenderer->Set_Texture(tex);
			LineRenderer->Set_Shader(shader);
			LineRenderer->Set_Width(Get_Particle_Size());
			if (line_props != NULL) {				
				LineRenderer->Init(*line_props);
			} else {
				LineRenderer->Init(_DefaultLineEmitterProps);
			}
		}
		break;
	case W3D_EMITTER_RENDER_MODE_LINEGRP_TETRA:
		{
			LineGroup=new LineGroupClass();
			LineGroup->Set_Flag(LineGroupClass::TRANSFORM, true);
			LineGroup->Set_Texture(tex);
			LineGroup->Set_Shader(shader);
			LineGroup->Set_Line_Mode(LineGroupClass::TETRAHEDRON);
			TailPosition = new ShareBufferClass<Vector3>(MaxNum);
			Set_Force_Visible(1);
		}
		break;
	case W3D_EMITTER_RENDER_MODE_LINEGRP_PRISM:
		{
			LineGroup=new LineGroupClass();
			LineGroup->Set_Flag(LineGroupClass::TRANSFORM, true);
			LineGroup->Set_Texture(tex);
			LineGroup->Set_Shader(shader);
			LineGroup->Set_Line_Mode(LineGroupClass::PRISM);
			TailPosition = new ShareBufferClass<Vector3>(MaxNum);
			Set_Force_Visible(1);
		}
		break;
	default:
		break;
	}	
	Position[0] = new ShareBufferClass<Vector3>(MaxNum);
	if (PingPongPosition) {
		Position[1] = new ShareBufferClass<Vector3>(MaxNum);
	}
	APT = new ShareBufferClass<unsigned int>(MaxNum);
	Velocity = new Vector3[MaxNum]; 
	TimeStamp = new unsigned int[MaxNum];
	int minlod = Calculate_Cost_Value_Arrays(1.0f, Value, Cost);
	if (Get_LOD_Level() < minlod) Set_LOD_Level(minlod);
	TotalActiveCount++;
}
ParticleBufferClass::ParticleBufferClass(const ParticleBufferClass & src) :
	RenderObjClass(src),
	NewParticleQueue(NULL),
	NewParticleQueueStart(0U),
	NewParticleQueueEnd(0U),
	NewParticleQueueCount(0U),
	RenderMode(src.RenderMode),
	FrameMode(src.FrameMode),
	MaxAge(src.MaxAge),
	LastUpdateTime(WW3D::SyncTime),
	IsEmitterDead(false),
	MaxSize(src.MaxSize),
	MaxNum(src.MaxNum),
	Start(0U),
	End(0U),
	NewEnd(0U),
	NonNewNum(0),
	NewNum(0),
	BoundingBox(Vector3(0,0,0),Vector3(0,0,0)),
	BoundingBoxDirty(true),
	NumColorKeyFrames(src.NumColorKeyFrames),
	ColorKeyFrameTimes(NULL),
	ColorKeyFrameValues(NULL),
	ColorKeyFrameDeltas(NULL),
	NumAlphaKeyFrames(src.NumAlphaKeyFrames),
	AlphaKeyFrameTimes(NULL),
	AlphaKeyFrameValues(NULL),
	AlphaKeyFrameDeltas(NULL),
	NumSizeKeyFrames(src.NumSizeKeyFrames),
	SizeKeyFrameTimes(NULL),
	SizeKeyFrameValues(NULL),
	SizeKeyFrameDeltas(NULL),
	NumRotationKeyFrames(src.NumRotationKeyFrames),
	RotationKeyFrameTimes(NULL),
	RotationKeyFrameValues(NULL),
	HalfRotationKeyFrameDeltas(NULL),
	OrientationKeyFrameValues(NULL),
	NumFrameKeyFrames(src.NumFrameKeyFrames),
	FrameKeyFrameTimes(NULL),
	FrameKeyFrameValues(NULL),
	FrameKeyFrameDeltas(NULL),
	NumBlurTimeKeyFrames(src.NumBlurTimeKeyFrames),
	BlurTimeKeyFrameTimes(NULL),
	BlurTimeKeyFrameValues(NULL),
	BlurTimeKeyFrameDeltas(NULL),
	RandomColorEntries(NULL),
	RandomAlphaEntries(NULL),
	RandomSizeEntries(NULL),
	ColorRandom(src.ColorRandom),
	OpacityRandom(src.OpacityRandom),
	SizeRandom(src.SizeRandom),
	RotationRandom(src.RotationRandom),
	FrameRandom(src.FrameRandom),
	InitialOrientationRandom(src.InitialOrientationRandom),
	NumRandomRotationEntriesMinus1(0),
	RandomRotationEntries(NULL),
	NumRandomOrientationEntriesMinus1(0),
	RandomOrientationEntries(NULL),
	NumRandomFrameEntriesMinus1(0),
	RandomFrameEntries(NULL),
	NumRandomBlurTimeEntriesMinus1(0),
	RandomBlurTimeEntries(NULL),
	PointGroup(NULL),
	LineRenderer(NULL),
	LineGroup(NULL),
	Diffuse(NULL),
	TailDiffuse(NULL),
	Color(NULL),
	Alpha(NULL),
	Size(NULL),
	Orientation(NULL),
	Frame(NULL),
	UCoord(NULL),
	TailPosition(NULL),
	APT(NULL),
	PingPongPosition(src.PingPongPosition),
	Velocity(NULL),
	TimeStamp(NULL),
	Emitter(src.Emitter),
	DecimationThreshold(src.DecimationThreshold),
	ProjectedArea(0.0f),
	DefaultTailDiffuse(src.DefaultTailDiffuse)
{
	Position[0] = NULL;
	Position[1] = NULL;
	unsigned int i;
	LodCount = MIN(MaxNum, 17);
	LodBias = src.LodBias;
	NumRandomColorEntriesMinus1 = src.NumRandomColorEntriesMinus1;
	if (src.Color) {
		Color = new ShareBufferClass<Vector3>(MaxNum);
		ColorKeyFrameTimes = new unsigned int [NumColorKeyFrames];
		ColorKeyFrameValues = new Vector3 [NumColorKeyFrames];
		ColorKeyFrameDeltas = new Vector3 [NumColorKeyFrames];
		for (i = 0; i < NumColorKeyFrames; i++) {
			ColorKeyFrameTimes[i] = src.ColorKeyFrameTimes[i];
			ColorKeyFrameValues[i] = src.ColorKeyFrameValues[i];
			ColorKeyFrameDeltas[i] = src.ColorKeyFrameDeltas[i];
		}
		if (src.RandomColorEntries) {
			RandomColorEntries = new Vector3 [NumRandomColorEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomColorEntriesMinus1; j++) {
				RandomColorEntries[j] = src.RandomColorEntries[j];
			}
		}
	} else {
		ColorKeyFrameValues = new Vector3 [1];
		ColorKeyFrameValues[0] = src.ColorKeyFrameValues[0];
	}
	NumRandomAlphaEntriesMinus1 = src.NumRandomAlphaEntriesMinus1;
	if (src.Alpha) {
		Alpha = new ShareBufferClass<float>(MaxNum);
		AlphaKeyFrameTimes = new unsigned int [NumAlphaKeyFrames];
		AlphaKeyFrameValues = new float [NumAlphaKeyFrames];
		AlphaKeyFrameDeltas = new float [NumAlphaKeyFrames];
		for (i = 0; i < NumAlphaKeyFrames; i++) {
			AlphaKeyFrameTimes[i] = src.AlphaKeyFrameTimes[i];
			AlphaKeyFrameValues[i] = src.AlphaKeyFrameValues[i];
			AlphaKeyFrameDeltas[i] = src.AlphaKeyFrameDeltas[i];
		}
		if (src.RandomAlphaEntries) {
			RandomAlphaEntries = new float [NumRandomAlphaEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomAlphaEntriesMinus1; j++) {
				RandomAlphaEntries[j] = src.RandomAlphaEntries[j];
			}
		}
	} else {
		AlphaKeyFrameValues = new float [1];
		AlphaKeyFrameValues[0] = src.AlphaKeyFrameValues[0];
	}

	NumRandomSizeEntriesMinus1 = src.NumRandomSizeEntriesMinus1;
	if (src.Size) {
		Size = new ShareBufferClass<float>(MaxNum);
		SizeKeyFrameTimes = new unsigned int [NumSizeKeyFrames];
		SizeKeyFrameValues = new float [NumSizeKeyFrames];
		SizeKeyFrameDeltas = new float [NumSizeKeyFrames];
		for (i = 0; i < NumSizeKeyFrames; i++) {
			SizeKeyFrameTimes[i] = src.SizeKeyFrameTimes[i];
			SizeKeyFrameValues[i] = src.SizeKeyFrameValues[i];
			SizeKeyFrameDeltas[i] = src.SizeKeyFrameDeltas[i];
		}
		if (src.RandomSizeEntries) {
			RandomSizeEntries = new float [NumRandomSizeEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomSizeEntriesMinus1; j++) {
				RandomSizeEntries[j] = src.RandomSizeEntries[j];
			}
		}
	} else {
		SizeKeyFrameValues = new float [1];
		SizeKeyFrameValues[0] = src.SizeKeyFrameValues[0];
	}
	NumRandomRotationEntriesMinus1 = src.NumRandomRotationEntriesMinus1;
	NumRandomOrientationEntriesMinus1 = src.NumRandomOrientationEntriesMinus1;
	if (src.Orientation) {
		Orientation = new ShareBufferClass<uint8>(MaxNum);
		RotationKeyFrameTimes = new unsigned int [NumRotationKeyFrames];
		RotationKeyFrameValues = new float [NumRotationKeyFrames];
		HalfRotationKeyFrameDeltas = new float [NumRotationKeyFrames];
		OrientationKeyFrameValues = new float [NumRotationKeyFrames];
		for (i = 0; i < NumRotationKeyFrames; i++) {
			RotationKeyFrameTimes[i] = src.RotationKeyFrameTimes[i];
			RotationKeyFrameValues[i] = src.RotationKeyFrameValues[i];
			HalfRotationKeyFrameDeltas[i] = src.HalfRotationKeyFrameDeltas[i];
			OrientationKeyFrameValues[i] = src.OrientationKeyFrameValues[i];
		}
		if (src.RandomRotationEntries) {
			RandomRotationEntries = new float [NumRandomRotationEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomRotationEntriesMinus1; j++) {
				RandomRotationEntries[j] = src.RandomRotationEntries[j];
			}
		}
		if (src.RandomOrientationEntries) {
			RandomOrientationEntries = new float [NumRandomOrientationEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomOrientationEntriesMinus1; j++) {
				RandomOrientationEntries[j] = src.RandomOrientationEntries[j];
			}
		}
	} else {
	}
	NumRandomFrameEntriesMinus1 = src.NumRandomFrameEntriesMinus1;
	if (src.Frame || src.UCoord) {
		if (src.Frame) {
			Frame = new ShareBufferClass<uint8>(MaxNum);
		} else {
			UCoord = new ShareBufferClass<float>(MaxNum);
		}
		FrameKeyFrameTimes = new unsigned int [NumFrameKeyFrames];
		FrameKeyFrameValues = new float [NumFrameKeyFrames];
		FrameKeyFrameDeltas = new float [NumFrameKeyFrames];
		for (i = 0; i < NumFrameKeyFrames; i++) {
			FrameKeyFrameTimes[i] = src.FrameKeyFrameTimes[i];
			FrameKeyFrameValues[i] = src.FrameKeyFrameValues[i];
			FrameKeyFrameDeltas[i] = src.FrameKeyFrameDeltas[i];
		}
		if (src.RandomFrameEntries) {
			RandomFrameEntries = new float [NumRandomFrameEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomFrameEntriesMinus1; j++) {
				RandomFrameEntries[j] = src.RandomFrameEntries[j];
			}
		}
	} else {
		FrameKeyFrameValues = new float [1];
		FrameKeyFrameValues[0] = src.FrameKeyFrameValues[0];
	}
	NumRandomBlurTimeEntriesMinus1 = src.NumRandomBlurTimeEntriesMinus1;
	if (NumBlurTimeKeyFrames > 0) {
		BlurTimeKeyFrameTimes = new unsigned int [NumBlurTimeKeyFrames];
		BlurTimeKeyFrameValues = new float [NumBlurTimeKeyFrames];
		BlurTimeKeyFrameDeltas = new float [NumBlurTimeKeyFrames];
		for (i = 0; i < NumBlurTimeKeyFrames; i++) {
			BlurTimeKeyFrameTimes[i] = src.BlurTimeKeyFrameTimes[i];
			BlurTimeKeyFrameValues[i] = src.BlurTimeKeyFrameValues[i];
			BlurTimeKeyFrameDeltas[i] = src.BlurTimeKeyFrameDeltas[i];
		}
		if (src.RandomBlurTimeEntries) {
			RandomBlurTimeEntries = new float [NumRandomBlurTimeEntriesMinus1 + 1];
			for (unsigned int j = 0; j <= NumRandomBlurTimeEntriesMinus1; j++) {
				RandomBlurTimeEntries[j] = src.RandomBlurTimeEntries[j];
			}
		}
	} else {
		BlurTimeKeyFrameValues = new float [1];
		BlurTimeKeyFrameValues[0] = src.BlurTimeKeyFrameValues[0];
	}
	NewParticleQueue = new NewParticleStruct[MaxNum];
	Accel = src.Accel;
	HasAccel = src.HasAccel;
	switch (RenderMode)	
	{
	case W3D_EMITTER_RENDER_MODE_TRI_PARTICLES:
		{
			PointGroup = new PointGroupClass();
			PointGroup->Set_Flag(PointGroupClass::TRANSFORM, true);
			PointGroup->Set_Texture(src.PointGroup->Peek_Texture());
			PointGroup->Set_Shader(src.PointGroup->Get_Shader());
			PointGroup->Set_Point_Mode(PointGroupClass::TRIS);
			PointGroup->Set_Frame_Row_Column_Count_Log2(src.PointGroup->Get_Frame_Row_Column_Count_Log2());
		}
		break;
	case W3D_EMITTER_RENDER_MODE_QUAD_PARTICLES:
		{
			PointGroup = new PointGroupClass();
			PointGroup->Set_Flag(PointGroupClass::TRANSFORM, true);
			PointGroup->Set_Texture(src.PointGroup->Peek_Texture());
			PointGroup->Set_Shader(src.PointGroup->Get_Shader());
			PointGroup->Set_Point_Mode(PointGroupClass::QUADS);
			PointGroup->Set_Frame_Row_Column_Count_Log2(src.PointGroup->Get_Frame_Row_Column_Count_Log2());
		}
		break;
	case W3D_EMITTER_RENDER_MODE_LINE:
		{	
			LineRenderer = new SegLineRendererClass(*src.LineRenderer);
		}
		break;
	case W3D_EMITTER_RENDER_MODE_LINEGRP_TETRA:
		{
			LineGroup=new LineGroupClass();
			LineGroup->Set_Flag(LineGroupClass::TRANSFORM, true);
			LineGroup->Set_Texture(src.LineGroup->Peek_Texture());
			LineGroup->Set_Shader(src.LineGroup->Get_Shader());
			LineGroup->Set_Line_Mode(LineGroupClass::TETRAHEDRON);
			TailPosition = new ShareBufferClass<Vector3>(MaxNum);
			Set_Force_Visible(1);
		}
		break;
	case W3D_EMITTER_RENDER_MODE_LINEGRP_PRISM:
		{
			LineGroup=new LineGroupClass();
			LineGroup->Set_Flag(LineGroupClass::TRANSFORM, true);
			LineGroup->Set_Texture(src.LineGroup->Peek_Texture());
			LineGroup->Set_Shader(src.LineGroup->Get_Shader());
			LineGroup->Set_Line_Mode(LineGroupClass::PRISM);
			TailPosition = new ShareBufferClass<Vector3>(MaxNum);
			Set_Force_Visible(1);
		}
		break;
	default:
		break;
	}
	Position[0] = new ShareBufferClass<Vector3>(MaxNum);
	if (PingPongPosition) {
		Position[1] = new ShareBufferClass<Vector3>(MaxNum);
	}
	APT = new ShareBufferClass<unsigned int>(MaxNum);
	Velocity = new Vector3[MaxNum]; 
	TimeStamp = new unsigned int[MaxNum];
	int minlod = Calculate_Cost_Value_Arrays(1.0f, Value, Cost);
	if (Get_LOD_Level() < minlod) Set_LOD_Level(minlod);
	TotalActiveCount++;
}
ParticleBufferClass & ParticleBufferClass::operator = (const ParticleBufferClass & that)
{
	RenderObjClass::operator = (that);
	if (this != &that) {
	}
	return * this;
}
ParticleBufferClass::~ParticleBufferClass(void)
{
	if (NewParticleQueue)				delete [] NewParticleQueue;
	if (ColorKeyFrameTimes)				delete [] ColorKeyFrameTimes;
	if (ColorKeyFrameValues)			delete [] ColorKeyFrameValues;
	if (ColorKeyFrameDeltas)			delete [] ColorKeyFrameDeltas;
	if (AlphaKeyFrameTimes)				delete [] AlphaKeyFrameTimes;
	if (AlphaKeyFrameValues)			delete [] AlphaKeyFrameValues;
	if (AlphaKeyFrameDeltas)			delete [] AlphaKeyFrameDeltas;
	if (SizeKeyFrameTimes)				delete [] SizeKeyFrameTimes;
	if (SizeKeyFrameValues)				delete [] SizeKeyFrameValues;
	if (SizeKeyFrameDeltas)				delete [] SizeKeyFrameDeltas;
	if (RotationKeyFrameTimes)			delete [] RotationKeyFrameTimes;
	if (RotationKeyFrameValues)		delete [] RotationKeyFrameValues;
	if (HalfRotationKeyFrameDeltas)	delete [] HalfRotationKeyFrameDeltas;
	if (OrientationKeyFrameValues)	delete [] OrientationKeyFrameValues;
	if (FrameKeyFrameTimes)				delete [] FrameKeyFrameTimes;
	if (FrameKeyFrameValues)			delete [] FrameKeyFrameValues;
	if (FrameKeyFrameDeltas)			delete [] FrameKeyFrameDeltas;
	if (BlurTimeKeyFrameTimes)			delete [] BlurTimeKeyFrameTimes;
	if (BlurTimeKeyFrameValues)		delete [] BlurTimeKeyFrameValues;
	if (BlurTimeKeyFrameDeltas)		delete [] BlurTimeKeyFrameDeltas;
	if (RandomColorEntries)				delete [] RandomColorEntries;
	if (RandomAlphaEntries)				delete [] RandomAlphaEntries;
	if (RandomSizeEntries)				delete [] RandomSizeEntries;
	if (RandomRotationEntries)			delete [] RandomRotationEntries;
	if (RandomOrientationEntries)		delete [] RandomOrientationEntries;
	if (RandomFrameEntries)				delete [] RandomFrameEntries;
	if (RandomBlurTimeEntries)			delete [] RandomBlurTimeEntries;
	if (PointGroup)						delete PointGroup;
	if (LineRenderer)						delete LineRenderer;
	if (LineGroup)							delete LineGroup;
	REF_PTR_RELEASE(Position[0]);
	REF_PTR_RELEASE(Position[1]);
	REF_PTR_RELEASE(Diffuse);
	REF_PTR_RELEASE(TailDiffuse);
	REF_PTR_RELEASE(Color);
	REF_PTR_RELEASE(Alpha);
	REF_PTR_RELEASE(Size);
	REF_PTR_RELEASE(Orientation);
	REF_PTR_RELEASE(Frame);
	REF_PTR_RELEASE(UCoord);
	REF_PTR_RELEASE(TailPosition);
	REF_PTR_RELEASE(APT);
	if (Velocity)	delete [] Velocity;
	if (TimeStamp)	delete [] TimeStamp;
	if (Emitter) {
		Emitter = NULL;
	}	
	TotalActiveCount--;
}
RenderObjClass * ParticleBufferClass::Clone(void) const
{
	return new ParticleBufferClass(*this);
}
int ParticleBufferClass::Get_Num_Polys(void) const
{
	return (int)Get_Cost();
}
int ParticleBufferClass::Get_Particle_Count(void) const
{
	return NonNewNum + NewNum;
}
#define SORT_LEVEL_NONE						0
void ParticleBufferClass::Render(RenderInfoClass & rinfo)
{
	unsigned int sort_level = SORT_LEVEL_NONE;
	if (!WW3D::IsSortingEnabled)
		sort_level=Get_Shader().Guess_Sort_Level();
	if (WW3D::Are_Static_Sort_Lists_Enabled() && sort_level!=SORT_LEVEL_NONE) {		
		WW3D::Add_To_Static_Sort_List(this, sort_level);
	} else {
		Update_Kinematic_Particle_State();
		if (DecimationThreshold < LodCount - 1) {
			Update_Visual_Particle_State();
		}
		switch( RenderMode )
		{
		case W3D_EMITTER_RENDER_MODE_TRI_PARTICLES:
		case W3D_EMITTER_RENDER_MODE_QUAD_PARTICLES:
			Render_Particles(rinfo);
			break;
		case W3D_EMITTER_RENDER_MODE_LINE:
			Render_Line(rinfo);
			break;
		case W3D_EMITTER_RENDER_MODE_LINEGRP_TETRA:
		case W3D_EMITTER_RENDER_MODE_LINEGRP_PRISM:
			Render_Line_Group(rinfo);
			break;
		}
	}
}
void ParticleBufferClass::Generate_APT(ShareBufferClass <unsigned int> **apt,unsigned int &active_point_count)
{
	if (NonNewNum < (int)MaxNum || DecimationThreshold > 0) {
		unsigned int sub1_start;
		unsigned int sub1_end;
		unsigned int sub2_start;
		unsigned int sub2_end;
		unsigned int i;
		if ((Start < End) || ((Start == End) && NonNewNum == 0)) {
			sub1_start = Start;
			sub1_end = End;
			sub2_start = End;
			sub2_end = End;
		} else {
			sub1_start = 0;
			sub1_end = End;
			sub2_start = Start;
			sub2_end = MaxNum;
		}
		unsigned int *apt_ptr = APT->Get_Array();
		for (i = sub1_start; i < sub1_end; i++) {
			if (PermutationArray[i & 0xF] >= DecimationThreshold) {
				apt_ptr[active_point_count++] = i;
			}
		}
		for (i = sub2_start; i < sub2_end; i++) {
			if (PermutationArray[i & 0xF] >= DecimationThreshold) {
				apt_ptr[active_point_count++] = i;
			}
		}
		*apt = APT;
	} else {
		active_point_count = NonNewNum;
	}
}

void ParticleBufferClass::Combine_Color_And_Alpha()
{
	if (Color || Alpha)
	{
		unsigned cnt = MaxNum;
		if (!Diffuse)
		{
			Diffuse = new ShareBufferClass<Vector4>(MaxNum);
		}
		if (Color && Alpha)
		{
			VectorProcessorClass::Copy(Diffuse->Get_Array(),Color->Get_Array(),Alpha->Get_Array(),cnt);
		}
		else if (Color) 
		{
			VectorProcessorClass::Copy(Diffuse->Get_Array(),Color->Get_Array(),1.0f,cnt);
		}
		else
		{
			VectorProcessorClass::Copy(Diffuse->Get_Array(),Vector3(1.0f,1.0f,1.0f),Alpha->Get_Array(),cnt);
		}
		VectorProcessorClass::Clamp(Diffuse->Get_Array(),Diffuse->Get_Array(),0.0f,1.0f,cnt);
	}
	else if (Diffuse)
	{
		Diffuse->Release_Ref();
		Diffuse=NULL;
	}
}

void ParticleBufferClass::Render_Particles(RenderInfoClass & rinfo)
{
	ShareBufferClass<unsigned int> *apt = NULL;
	unsigned int active_point_count = 0;
	Generate_APT(&apt,active_point_count);	
	if (!Color) {
		PointGroup->Set_Point_Color(ColorKeyFrameValues[0]);
	}
	if (!Alpha) {
		PointGroup->Set_Point_Alpha(AlphaKeyFrameValues[0]);
	}
	if (!Size) {
		PointGroup->Set_Point_Size(SizeKeyFrameValues[0]);
	}
	if (!Orientation) {
		PointGroup->Set_Point_Orientation(0);
	}
	if (!Frame) {
		PointGroup->Set_Point_Frame(((int)(FrameKeyFrameValues[0])) & 0xFF);
	}
	int pingpong = 0;
	if (PingPongPosition) {
		pingpong = WW3D::FrameCount & 0x1;
	}
	Combine_Color_And_Alpha();
	PointGroup->Set_Arrays(Position[pingpong], Diffuse, apt, Size, Orientation, Frame, active_point_count);
	Update_Bounding_Box();	
	PointGroup->Render(rinfo);	
}
void ParticleBufferClass::Render_Line(RenderInfoClass & rinfo)
{
	int pingpong = 0;
	if (PingPongPosition) {
		pingpong = WW3D::FrameCount & 0x1;
	}
	static SimpleDynVecClass<Vector3> tmp_points;
	Vector3 * positions = Position[pingpong]->Get_Array();
	unsigned int sub1_end;
	unsigned int sub2_start;
	unsigned int i;	
	if ((Start < End) || ((Start == End) && NonNewNum ==0)) {
		sub1_end = End;
		sub2_start = End;
	} else {
		sub1_end = MaxNum;
		sub2_start = 0;
	}
	tmp_points.Delete_All(false);
	
	for (i = Start; i < sub1_end; i++) {
		if (PermutationArray[i & 0xF] >= DecimationThreshold) {
			tmp_points.Add(positions[i]);
		}
	}
	for (i = sub2_start; i < End; i++) {
		if (PermutationArray[i & 0xF] >= DecimationThreshold) {
			tmp_points.Add(positions[i]);
		}
	}
	if (tmp_points.Count() > 0) {
		SphereClass bounding_sphere;
		Get_Obj_Space_Bounding_Sphere(bounding_sphere);
		LineRenderer->Render(rinfo,
									Transform,
									tmp_points.Count(),
									&(tmp_points[0]),
									bounding_sphere);
	}
}
void ParticleBufferClass::Render_Line_Group(RenderInfoClass & rinfo)
{
	ShareBufferClass<unsigned int> *apt = NULL;
	unsigned int active_point_count = 0;	
	Generate_APT(&apt,active_point_count);	
	if (!Color)
	{
		LineGroup->Set_Line_Color(ColorKeyFrameValues[0]);
	}
	if (!Alpha)
	{
		LineGroup->Set_Line_Alpha(AlphaKeyFrameValues[0]);
	}
	if (!Size)
	{
		LineGroup->Set_Line_Size(SizeKeyFrameValues[0]);
	}
	if (!Frame)
	{
		LineGroup->Set_Line_UCoord(FrameKeyFrameValues[0]);
	}
	int pingpong = 0;
	if (PingPongPosition)
	{
		pingpong = WW3D::FrameCount & 0x1;
	}
	Combine_Color_And_Alpha();
	TailDiffuseTypeEnum tailtype = Determine_Tail_Diffuse();
	switch (tailtype)
	{
	case BLACK:
		REF_PTR_RELEASE(TailDiffuse);
		DefaultTailDiffuse.Set(0,0,0,0);
		break;
	case WHITE:
		REF_PTR_RELEASE(TailDiffuse);
		DefaultTailDiffuse.Set(1,1,1,1);
		break;
	case SAME_AS_HEAD_ALPHA_ZERO:
		if (!Diffuse)
		{
			REF_PTR_RELEASE(TailDiffuse);
			DefaultTailDiffuse.Set(ColorKeyFrameValues[0].X,ColorKeyFrameValues[0].Y,ColorKeyFrameValues[0].Z,0);
		}
		else
		{
			if (!TailDiffuse) TailDiffuse = new ShareBufferClass<Vector4>(MaxNum);
			for (unsigned int i=0; i<MaxNum; i++)
			{
				Vector4 elt=Diffuse->Get_Element(i);				
				elt.W=0;
				TailDiffuse->Set_Element(i,elt);
			}
		}
		break;
	case SAME_AS_HEAD:
		if (!Diffuse)
		{
			REF_PTR_RELEASE(TailDiffuse);
			DefaultTailDiffuse.Set(ColorKeyFrameValues[0].X,ColorKeyFrameValues[0].Y,ColorKeyFrameValues[0].Z,AlphaKeyFrameValues[0]);
		}
		else
		{
			if (!TailDiffuse) TailDiffuse = new ShareBufferClass<Vector4>(MaxNum);
			VectorProcessorClass::Copy(TailDiffuse->Get_Array(),Diffuse->Get_Array(),MaxNum);
		}
		break;
	default:
		break;
	}
	if (!TailDiffuse)
	{
		LineGroup->Set_Tail_Diffuse(DefaultTailDiffuse);
	}
	LineGroup->Set_Arrays(Position[pingpong], TailPosition,Diffuse,TailDiffuse, apt, Size, UCoord, active_point_count);
	Update_Bounding_Box();	
	LineGroup->Render(rinfo);	
}
void ParticleBufferClass::Scale(float scale)
{
	unsigned int i;
	for (i = 0; i < NumSizeKeyFrames; i++) {
		SizeKeyFrameValues[i] *= scale;
		SizeKeyFrameDeltas[i] *= scale;
	}
	if (RandomSizeEntries) {
		for (i = 0; i <= NumRandomSizeEntriesMinus1; i++) {
			RandomSizeEntries[i] *= scale;
		}
	}
	MaxSize *= scale;
	SizeRandom *= scale;
}
void ParticleBufferClass::On_Frame_Update(void)
{
	Invalidate_Cached_Bounding_Volumes();
	if (Emitter) {
		Emitter->Emit();
	}
	if (Is_Complete()) {
		Scene->Register(this,SceneClass::RELEASE);
	}
}
void ParticleBufferClass::Notify_Added(SceneClass * scene)
{
	RenderObjClass::Notify_Added(scene);
	scene->Register(this,SceneClass::ON_FRAME_UPDATE);
}
void ParticleBufferClass::Notify_Removed(SceneClass * scene)
{
	scene->Unregister(this,SceneClass::ON_FRAME_UPDATE);
	RenderObjClass::Notify_Removed(scene);
}
void ParticleBufferClass::Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const
{
	((ParticleBufferClass *)this)->Update_Bounding_Box();
	sphere.Center = BoundingBox.Center;
	sphere.Radius = BoundingBox.Extent.Length();
}
void ParticleBufferClass::Get_Obj_Space_Bounding_Box(AABoxClass & box) const
{
	((ParticleBufferClass *)this)->Update_Bounding_Box();
	box = BoundingBox;
}
void ParticleBufferClass::Prepare_LOD(CameraClass &camera)
{
	if (Is_Not_Hidden_At_All() == false)
	{
		return;
	}
	Vector3 cam = camera.Get_Position();
	ViewportClass viewport = camera.Get_Viewport();
	Vector2 vpr_min, vpr_max;
	camera.Get_View_Plane(vpr_min, vpr_max);
	float width_factor = viewport.Width() / (vpr_max.X - vpr_min.X);
	float height_factor = viewport.Height() / (vpr_max.Y - vpr_min.Y);
	const SphereClass & sphere = Get_Bounding_Sphere();
	float dist = (sphere.Center - cam).Length();
	float bounding_sphere_projected_radius = 0.0f;
	float particle_projected_radius = 0.0f;
	if (dist)
	{
		float oo_dist = 1.0f / dist;
		bounding_sphere_projected_radius = sphere.Radius * oo_dist;
		particle_projected_radius = MaxSize * oo_dist;
	}
	float bs_rad_sq = bounding_sphere_projected_radius * bounding_sphere_projected_radius;
	float p_rad_sq = particle_projected_radius * particle_projected_radius * MaxNum;
	float proj_area = WWMATH_PI * MIN(bs_rad_sq, p_rad_sq) * width_factor * height_factor;
	ProjectedArea = 0.9f * ProjectedArea + 0.1f * proj_area;
	int minlod = Calculate_Cost_Value_Arrays(ProjectedArea, Value, Cost);
	if (Get_LOD_Level() < minlod)
	{
		Set_LOD_Level(minlod);
	}
	PredictiveLODOptimizerClass::Add_Object(this);
}
void ParticleBufferClass::Increment_LOD(void)
{
	if (DecimationThreshold > 0) DecimationThreshold--;
}
void ParticleBufferClass::Decrement_LOD(void)
{
	if (DecimationThreshold < LodCount) DecimationThreshold++;
}
float ParticleBufferClass::Get_Cost(void) const
{
	return(Cost[(LodCount - 1) - DecimationThreshold]);
}
float ParticleBufferClass::Get_Value(void) const
{
	return(Value[(LodCount - 1) - DecimationThreshold]);
}
float ParticleBufferClass::Get_Post_Increment_Value(void) const
{
	return(Value[LodCount - DecimationThreshold]);
}
template<class T> inline
T Bound(T original, T minval, T maxval)
{
	if (original < minval) return(minval);
	if (original > maxval) return(maxval);
	return(original);
};
void ParticleBufferClass::Set_LOD_Level(int lod)
{
	lod = Bound(lod, 0, (int)LodCount);
	DecimationThreshold = (LodCount - 1) - lod;
}
int ParticleBufferClass::Get_LOD_Level(void) const
{
	return((LodCount - 1) - DecimationThreshold);
}
int ParticleBufferClass::Get_LOD_Count(void) const
{
	return LodCount;
}
int ParticleBufferClass::Calculate_Cost_Value_Arrays(float screen_area, float *values, float *costs) const
{
	unsigned int lod = 0;
	float cost_factor=0.0f;
	switch (RenderMode)
	{
	case W3D_EMITTER_RENDER_MODE_TRI_PARTICLES:
		cost_factor = (float)MaxNum * 0.0625f;
		break;
	case W3D_EMITTER_RENDER_MODE_QUAD_PARTICLES:
		cost_factor = (float)MaxNum * 2.0f * 0.0625f;
		break;
	case W3D_EMITTER_RENDER_MODE_LINE:
		cost_factor = (float) (2*MaxNum-1) * 0.0625f;
		break;
	case W3D_EMITTER_RENDER_MODE_LINEGRP_TETRA:
		cost_factor = (float)MaxNum * 4.0f * 0.0625f;
		break;
	case W3D_EMITTER_RENDER_MODE_LINEGRP_PRISM:
		cost_factor = (float)MaxNum * 8.0f * 0.0625f;
		break;
	}	
	for (lod = 0; lod < LodCount; lod++) {
		costs[lod] = cost_factor * (float)lod;
		costs[lod] = (costs[lod] != 0) ? costs[lod] : 0.000001f;
	}
	for (lod = 0;  lod < LodCount && LODMaxScreenSizes[lod] < screen_area; lod++) {
		values[lod] = AT_MIN_LOD;
	}
	if (lod >= LodCount) {
		lod = LodCount - 1;
	} else {
		values[lod] = AT_MIN_LOD;
	}
	int minlod = lod;
	lod++;
	for (; lod < LodCount; lod++) {
		float polycount = costs[lod];
		float benefit_factor = (polycount > WWMATH_EPSILON) ? (1 - (0.5f / (polycount * polycount))) : 0.0f;
		values[lod] = (benefit_factor * screen_area * LodBias) / costs[lod];
	}
	values[LodCount] = AT_MAX_LOD;
	return minlod;
}
RENEGADE_FUNCTION
void ParticleBufferClass::Reset_Colors(ParticlePropertyStruct<Vector3>& new_props)
AT1(0x0055CC50);
RENEGADE_FUNCTION
void ParticleBufferClass::Reset_Opacity(ParticlePropertyStruct<float>& new_props)
AT1(0x0055D260);
RENEGADE_FUNCTION
void ParticleBufferClass::Reset_Size(ParticlePropertyStruct<float>& new_props)
AT1(0x0055D680);
RENEGADE_FUNCTION
void ParticleBufferClass::Reset_Rotations(ParticlePropertyStruct<float>& new_props,float orient_rnd)
AT1(0x0055DB60);
RENEGADE_FUNCTION
void ParticleBufferClass::Reset_Frames(ParticlePropertyStruct<float>& new_props)
AT1(0x0055E180);
RENEGADE_FUNCTION
void ParticleBufferClass::Reset_Blur_Times(ParticlePropertyStruct<float>& new_props)
AT1(0x0055E630);
void ParticleBufferClass::Emitter_Is_Dead(void)
{
	IsEmitterDead = true;
	Emitter = NULL;
}
void ParticleBufferClass::Set_Emitter(ParticleEmitterClass *emitter)
{
	if (Emitter) {
		Emitter = NULL;
	}
	Emitter = emitter;
	if (Emitter) {
	}
}
NewParticleStruct * ParticleBufferClass::Add_Uninitialized_New_Particle(void)
{
	NewParticleStruct *ptr = &(NewParticleQueue[NewParticleQueueEnd]);
   if (++NewParticleQueueEnd == MaxNum) NewParticleQueueEnd = 0;
	if (++NewParticleQueueCount == (signed)(MaxNum + 1)) {
		if (++NewParticleQueueStart == MaxNum) NewParticleQueueStart = 0;
		NewParticleQueueCount--;
	}
	return ptr;
}
void ParticleBufferClass::Update_Cached_Bounding_Volumes(void) const
{
	((ParticleBufferClass *)this)->Update_Bounding_Box();
	CachedBoundingSphere.Init(BoundingBox.Center, BoundingBox.Extent.Length());
	CachedBoundingBox = BoundingBox;
	Validate_Cached_Bounding_Volumes();
}
void ParticleBufferClass::Update_Kinematic_Particle_State(void)
{
	unsigned int elapsed = WW3D::SyncTime - LastUpdateTime;
	if (elapsed == 0U) return;
	Get_New_Particles();
	Kill_Old_Particles();
	if (NonNewNum > 0) Update_Non_New_Particles(elapsed);
	End = NewEnd;
	NonNewNum += NewNum;
	NewNum = 0;
	LastUpdateTime = WW3D::SyncTime;
	BoundingBoxDirty = true;
}
void ParticleBufferClass::Update_Visual_Particle_State(void)
{
	bool is_linegroup=( (RenderMode==W3D_EMITTER_RENDER_MODE_LINEGRP_TETRA) ||
							  (RenderMode==W3D_EMITTER_RENDER_MODE_LINEGRP_PRISM));
	if (!Color && !Alpha && !Size && !Orientation && !Frame && !UCoord && !is_linegroup) return;
	unsigned int sub1_end;
	unsigned int sub2_start;
	if ((Start < End) || ((Start == End) && NonNewNum ==0)) {
		sub1_end = End;
		sub2_start = End;
	} else {
		sub1_end = MaxNum;
		sub2_start = 0;
	}
	unsigned int current_time = WW3D::SyncTime;
	unsigned int ckey = NumColorKeyFrames - 1;
	unsigned int akey = NumAlphaKeyFrames - 1;
	unsigned int skey = NumSizeKeyFrames - 1;
	unsigned int rkey = NumRotationKeyFrames - 1;
	unsigned int fkey = NumFrameKeyFrames - 1;
	unsigned int bkey = NumBlurTimeKeyFrames -1;
	unsigned int part;
	Vector3 *color = Color ? Color->Get_Array(): NULL;
	float *alpha = Alpha ? Alpha->Get_Array(): NULL;
	float *size = Size ? Size->Get_Array(): NULL;
	uint8 *orientation = Orientation ? Orientation->Get_Array(): NULL;
	uint8 *frame = Frame ? Frame->Get_Array(): NULL;
	float *ucoord = UCoord ? UCoord->Get_Array() : NULL;
	Vector3 *tailposition = TailPosition ? TailPosition->Get_Array() : NULL;
	Vector3 *position=NULL;
	if (PingPongPosition) {
		int pingpong = WW3D::FrameCount & 0x1;
		position = Position[pingpong]->Get_Array();		
	} else {
		position = Position[0]->Get_Array();
	}
	for (part = Start; part < sub1_end; part++) {
		unsigned int part_age = current_time - TimeStamp[part];
		if (color) {
			for (; part_age < ColorKeyFrameTimes[ckey]; ckey--);
			color[part] = ColorKeyFrameValues[ckey] +
							ColorKeyFrameDeltas[ckey] * (float)(part_age - ColorKeyFrameTimes[ckey]) +
							RandomColorEntries[part & NumRandomColorEntriesMinus1];
		}
		if (alpha) {
			for (; part_age < AlphaKeyFrameTimes[akey]; akey--);
			alpha[part] = AlphaKeyFrameValues[akey] +
				AlphaKeyFrameDeltas[akey] * (float)(part_age - AlphaKeyFrameTimes[akey]) +
				RandomAlphaEntries[part & NumRandomAlphaEntriesMinus1];
		}
		if (size) {
			for (; part_age < SizeKeyFrameTimes[skey]; skey--);

			size[part] = SizeKeyFrameValues[skey] +
				SizeKeyFrameDeltas[skey] * (float)(part_age - SizeKeyFrameTimes[skey]) +
				RandomSizeEntries[part & NumRandomSizeEntriesMinus1];
			size[part] = (size[part] >= 0.0f) ? size[part] : 0.0f;
		}
		if (orientation) {
			for (; part_age < RotationKeyFrameTimes[rkey]; rkey--);
			float f_delta_t = (float)(part_age - RotationKeyFrameTimes[rkey]);
			float tmp_orient = OrientationKeyFrameValues[rkey] +
				(RotationKeyFrameValues[rkey] + HalfRotationKeyFrameDeltas[rkey] * f_delta_t) * f_delta_t +
				RandomRotationEntries[part & NumRandomRotationEntriesMinus1] * (float)part_age +
				RandomOrientationEntries[part & NumRandomOrientationEntriesMinus1];
			orientation[part] = (uint)(((int)(tmp_orient * 256.0f)) & 0xFF);
		}
		if (frame) {
			for (; part_age < FrameKeyFrameTimes[fkey]; fkey--);
			float tmp_frame = FrameKeyFrameValues[fkey] +
				FrameKeyFrameDeltas[fkey] * (float)(part_age - FrameKeyFrameTimes[fkey]) +
				RandomFrameEntries[part & NumRandomFrameEntriesMinus1];
			frame[part] = (uint)(((int)(tmp_frame)) & 0xFF);
		}
		if (ucoord) {
			for (; part_age < FrameKeyFrameTimes[fkey]; fkey--);
			ucoord[part] = FrameKeyFrameValues[fkey] +
				FrameKeyFrameDeltas[fkey] * (float)(part_age - FrameKeyFrameTimes[fkey]) +
				RandomFrameEntries[part & NumRandomFrameEntriesMinus1];		
		}
		if (tailposition) {
			float blur_time = BlurTimeKeyFrameValues[0];
			if (BlurTimeKeyFrameTimes) {
				for (; part_age < BlurTimeKeyFrameTimes[bkey]; bkey--);
				blur_time = BlurTimeKeyFrameValues[bkey] +
					BlurTimeKeyFrameDeltas[bkey] * (float)(part_age - BlurTimeKeyFrameTimes[bkey]) +
					RandomBlurTimeEntries[part & NumRandomBlurTimeEntriesMinus1];
			}
			tailposition[part]=position[part]-Velocity[part]*blur_time*1000;
		}		
	}
	for (part = sub2_start; part < End; part++) {
		unsigned int part_age = current_time - TimeStamp[part];
		if (color) {
			for (; part_age < ColorKeyFrameTimes[ckey]; ckey--);
			color[part] = 
					ColorKeyFrameValues[ckey] +
					ColorKeyFrameDeltas[ckey] * (float)(part_age - ColorKeyFrameTimes[ckey]) +
					RandomColorEntries[part & NumRandomColorEntriesMinus1];
		}
		if (alpha) {
			for (; part_age < AlphaKeyFrameTimes[akey]; akey--);
			alpha[part] = AlphaKeyFrameValues[akey] +
				AlphaKeyFrameDeltas[akey] * (float)(part_age - AlphaKeyFrameTimes[akey]) +
				RandomAlphaEntries[part & NumRandomAlphaEntriesMinus1];
		}
		if (size) {
			for (; part_age < SizeKeyFrameTimes[skey]; skey--);
			size[part] = SizeKeyFrameValues[skey] +
				SizeKeyFrameDeltas[skey] * (float)(part_age - SizeKeyFrameTimes[skey]) +
				RandomSizeEntries[part & NumRandomSizeEntriesMinus1];
			size[part] = (size[part] >= 0.0f) ? size[part] : 0.0f;
		}
		if (orientation) {
			for (; part_age < RotationKeyFrameTimes[rkey]; rkey--);
			float f_delta_t = (float)(part_age - RotationKeyFrameTimes[rkey]);
			float tmp_orient = OrientationKeyFrameValues[rkey] +
				(RotationKeyFrameValues[rkey] + HalfRotationKeyFrameDeltas[rkey] * f_delta_t) * f_delta_t +
				RandomRotationEntries[part & NumRandomRotationEntriesMinus1] * (float)part_age +
				RandomOrientationEntries[part & NumRandomOrientationEntriesMinus1];
			orientation[part] = (uint)(((int)(tmp_orient * 256.0f)) & 0xFF);
		}
		if (frame) {
			for (; part_age < FrameKeyFrameTimes[fkey]; fkey--);
			float tmp_frame = FrameKeyFrameValues[fkey] +
				FrameKeyFrameDeltas[fkey] * (float)(part_age - FrameKeyFrameTimes[fkey]) +
				RandomFrameEntries[part & NumRandomFrameEntriesMinus1];
			frame[part] = (uint)(((int)(tmp_frame)) & 0xFF);
		}
		if (ucoord) {
			for (; part_age < FrameKeyFrameTimes[fkey]; fkey--);
			ucoord[part] = FrameKeyFrameValues[fkey] +
				FrameKeyFrameDeltas[fkey] * (float)(part_age - FrameKeyFrameTimes[fkey]) +
				RandomFrameEntries[part & NumRandomFrameEntriesMinus1];			
		}
		if (tailposition) {
			float blur_time = BlurTimeKeyFrameValues[0];
			if (BlurTimeKeyFrameTimes) {
				for (; part_age < BlurTimeKeyFrameTimes[bkey]; bkey--);
				blur_time = BlurTimeKeyFrameValues[bkey] +
					BlurTimeKeyFrameDeltas[bkey] * (float)(part_age - BlurTimeKeyFrameTimes[bkey]) +
					RandomBlurTimeEntries[part & NumRandomBlurTimeEntriesMinus1];
			}
			tailposition[part]=position[part]-Velocity[part]*blur_time*1000;
		}
	}
}
RENEGADE_FUNCTION
void ParticleBufferClass::Update_Bounding_Box(void)
AT1(0x0055F970);
void ParticleBufferClass::Get_New_Particles(void)
{
	unsigned int current_time = WW3D::SyncTime;
	Vector3 *position;
	Vector3 *prev_pos;
	if (PingPongPosition) {
		int pingpong = WW3D::FrameCount & 0x1;
		position = Position[pingpong]->Get_Array();
		prev_pos = Position[pingpong ^ 0x1]->Get_Array();
	} else {
		position = Position[0]->Get_Array();
		prev_pos = NULL;
	}
	for (; NewParticleQueueCount;) {
		NewParticleStruct &new_particle = NewParticleQueue[NewParticleQueueStart];
		if (++NewParticleQueueStart == MaxNum) NewParticleQueueStart = 0U;
		NewParticleQueueCount--;
		TimeStamp[NewEnd] = new_particle.TimeStamp;
		unsigned int age = current_time - TimeStamp[NewEnd];
		if (age >= MaxAge) continue;
		float fp_age = (float)age;
		if (HasAccel) {
			position[NewEnd] = new_particle.Position +
									(new_particle.Velocity + 0.5f * Accel * fp_age) * fp_age;
			Velocity[NewEnd] = new_particle.Velocity + (Accel * fp_age);
		} else {
			position[NewEnd] =new_particle.Position +
										(new_particle.Velocity * fp_age);
			Velocity[NewEnd] = new_particle.Velocity;
		}
		if (PingPongPosition) {
			prev_pos[NewEnd] = new_particle.Position;
		}
		NewEnd++;
      if (NewEnd == MaxNum) NewEnd = 0;
      NewNum++;
      if ((NewNum + NonNewNum) == (signed)(MaxNum + 1)) {
         Start++;
         if (Start == MaxNum) Start = 0;
         NonNewNum--;
         if (NonNewNum == -1) {
            End++;
            if (End == MaxNum) End = 0;
            NonNewNum = 0;
            NewNum--;
         }
      }
	}
}
void ParticleBufferClass::Kill_Old_Particles(void)
{
	unsigned int sub1_end;
	unsigned int sub2_start;
	unsigned int i;
	if ((Start < End) || ((Start == End) && NonNewNum ==0)) {
		sub1_end = End;
		sub2_start = End;
	} else {
		sub1_end = MaxNum;
		sub2_start = 0;
	}
   unsigned int current_time = WW3D::SyncTime;
	bool broke = false;
   for (i = Start; i < sub1_end; i++) {
      if ((current_time - TimeStamp[i]) < MaxAge) {
         broke = true;
         break;
      }
      NonNewNum--;
	}
   if (!broke) {
	   for (i = sub2_start; i < End; i++) {
		   if ((current_time - TimeStamp[i]) < MaxAge) break;
         NonNewNum--;
	   }
   }
	Start = i;
}
void ParticleBufferClass::Update_Non_New_Particles(unsigned int elapsed)
{
	unsigned int sub1_end;
	unsigned int sub2_start;
	unsigned int i;
	if ((Start < End) || ((Start == End) && NonNewNum ==0)) {
		sub1_end = End;
		sub2_start = End;
	} else {
		sub1_end = MaxNum;
		sub2_start = 0;
	}
	float fp_elapsed_time = (float)elapsed;
	if (PingPongPosition) {
		int pingpong = WW3D::FrameCount & 0x1;
		Vector3 *position = Position[pingpong]->Get_Array();
		Vector3 *prev_pos = Position[pingpong ^ 0x1]->Get_Array();
		if (HasAccel) {
			Vector3 delta_v = Accel * fp_elapsed_time;
			Vector3 accel_p = Accel * (0.5f * fp_elapsed_time * fp_elapsed_time);
			for (i = Start; i < sub1_end; i++) {
				position[i] = prev_pos[i] + Velocity[i] * fp_elapsed_time + accel_p;
				Velocity[i] += delta_v;
			}
			for (i = sub2_start; i < End; i++) {
				position[i] = prev_pos[i] + Velocity[i] * fp_elapsed_time + accel_p;
				Velocity[i] += delta_v;
			}
		} else {
			for (i = Start; i < sub1_end; i++) {
				position[i] += Velocity[i] * fp_elapsed_time;
			}
			for (i = sub2_start; i < End; i++) {
				position[i] += Velocity[i] * fp_elapsed_time;
			}
		}
	} else {
		Vector3 *position = Position[0]->Get_Array();
		if (HasAccel) {
			Vector3 delta_v = Accel * fp_elapsed_time;
			Vector3 accel_p = Accel * (0.5f * fp_elapsed_time * fp_elapsed_time);
			for (i = Start; i < sub1_end; i++) {
				position[i] += Velocity[i] * fp_elapsed_time + accel_p;
				Velocity[i] += delta_v;
			}
			for (i = sub2_start; i < End; i++) {
				position[i] += Velocity[i] * fp_elapsed_time + accel_p;
				Velocity[i] += delta_v;
			}
		} else {
			for (i = Start; i < sub1_end; i++) {
				position[i] += Velocity[i] * fp_elapsed_time;
			}
			for (i = sub2_start; i < End; i++) {
				position[i] += Velocity[i] * fp_elapsed_time;
			}
		}
	}
}
void ParticleBufferClass::Get_Color_Key_Frames (ParticlePropertyStruct<Vector3> &colors) const
{
	int real_keyframe_count = (NumColorKeyFrames > 0) ? (NumColorKeyFrames - 1) : 0;
	bool create_last_keyframe = false;
	if ((ColorKeyFrameDeltas != NULL) &&
		 ((ColorKeyFrameDeltas[NumColorKeyFrames - 1].X != 0) ||
		  (ColorKeyFrameDeltas[NumColorKeyFrames - 1].Y != 0) ||
		  (ColorKeyFrameDeltas[NumColorKeyFrames - 1].Z != 0))) {
		real_keyframe_count ++;
		create_last_keyframe = true;
	}

	colors.Start = ColorKeyFrameValues[0];
	colors.Rand = ColorRandom;
	colors.NumKeyFrames = real_keyframe_count;
	colors.KeyTimes = NULL;
	colors.Values = NULL;
	if (real_keyframe_count > 0) {
		colors.KeyTimes	= new float[real_keyframe_count];
		colors.Values		= new Vector3[real_keyframe_count];
		unsigned int index;
		for (index = 1; index < NumColorKeyFrames; index ++) {
			colors.KeyTimes[index - 1]	= ((float)ColorKeyFrameTimes[index]) / 1000;
			colors.Values[index - 1]	= ColorKeyFrameValues[index];
		}
		if (create_last_keyframe) {			
			colors.KeyTimes[index - 1] = ((float)MaxAge / 1000);
			Vector3 start_color = ColorKeyFrameValues[index - 1];
			Vector3 &delta = ColorKeyFrameDeltas[NumColorKeyFrames - 1];
			float time_delta = (float)(MaxAge - ColorKeyFrameTimes[index - 1]);
			colors.Values[index - 1] = start_color + (delta * time_delta);
		}
	}

	return ;
}

void ParticleBufferClass::Get_Opacity_Key_Frames (ParticlePropertyStruct<float> &opacities) const
{
	int real_keyframe_count = (NumAlphaKeyFrames > 0) ? (NumAlphaKeyFrames - 1) : 0;
	bool create_last_keyframe = false;
	if ((AlphaKeyFrameDeltas != NULL) &&
		 (AlphaKeyFrameDeltas[NumAlphaKeyFrames - 1] != 0)) {
		real_keyframe_count ++;
		create_last_keyframe = true;
	}

	opacities.Start = AlphaKeyFrameValues[0];
	opacities.Rand = OpacityRandom;
	opacities.NumKeyFrames = real_keyframe_count;
	opacities.KeyTimes = NULL;
	opacities.Values = NULL;
	if (real_keyframe_count > 0) {
		opacities.KeyTimes	= new float[real_keyframe_count];
		opacities.Values		= new float[real_keyframe_count];
		unsigned int index;
		for (index = 1; index < NumAlphaKeyFrames; index ++) {
			opacities.KeyTimes[index - 1]	= ((float)AlphaKeyFrameTimes[index]) / 1000;
			opacities.Values[index - 1]	= AlphaKeyFrameValues[index];
		}
		if (create_last_keyframe) {			
			opacities.KeyTimes[index - 1] = ((float)MaxAge / 1000);
			float start_alpha = AlphaKeyFrameValues[index - 1];
			float &delta = AlphaKeyFrameDeltas[NumAlphaKeyFrames - 1];
			float time_delta = (float)(MaxAge - AlphaKeyFrameTimes[index - 1]);
			opacities.Values[index - 1] = start_alpha + (delta * time_delta);
		}
	}

	return ;
}


void ParticleBufferClass::Get_Size_Key_Frames (ParticlePropertyStruct<float> &sizes) const
{
	int real_keyframe_count = (NumSizeKeyFrames > 0) ? (NumSizeKeyFrames - 1) : 0;
	bool create_last_keyframe = false;
	if ((SizeKeyFrameDeltas != NULL) &&
		 (SizeKeyFrameDeltas[NumSizeKeyFrames - 1] != 0)) {
		real_keyframe_count ++;
		create_last_keyframe = true;
	}

	sizes.Start				= SizeKeyFrameValues[0];
	sizes.Rand				= SizeRandom;
	sizes.NumKeyFrames	= real_keyframe_count;
	sizes.KeyTimes			= NULL;
	sizes.Values			= NULL;
	if (real_keyframe_count > 0) {
		sizes.KeyTimes	= new float[real_keyframe_count];
		sizes.Values	= new float[real_keyframe_count];
		unsigned int index;
		for (index = 1; index < NumSizeKeyFrames; index ++) {
			sizes.KeyTimes[index - 1]	= ((float)SizeKeyFrameTimes[index]) / 1000;
			sizes.Values[index - 1]	= SizeKeyFrameValues[index];
		}
		if (create_last_keyframe) {			
			sizes.KeyTimes[index - 1] = ((float)MaxAge / 1000);
			float start_size			= SizeKeyFrameValues[index - 1];
			float &delta				= SizeKeyFrameDeltas[NumSizeKeyFrames - 1];
			float time_delta			= (float)(MaxAge - SizeKeyFrameTimes[index - 1]);
			sizes.Values[index - 1]	= start_size + (delta * time_delta);
		}
	}

	return ;
}


void ParticleBufferClass::Get_Rotation_Key_Frames (ParticlePropertyStruct<float> &rotations) const
{
	int real_keyframe_count = (NumRotationKeyFrames > 0) ? (NumRotationKeyFrames - 1) : 0;
	bool create_last_keyframe = false;
	if ((HalfRotationKeyFrameDeltas != NULL) &&
		 (HalfRotationKeyFrameDeltas[NumRotationKeyFrames - 1] != 0)) {
		real_keyframe_count ++;
		create_last_keyframe = true;
	}
	rotations.Start			= RotationKeyFrameValues ? RotationKeyFrameValues[0] * 1000.0f : 0;
	rotations.Rand				= RotationRandom * 1000.0f;
	rotations.NumKeyFrames	= real_keyframe_count;
	rotations.KeyTimes		= NULL;
	rotations.Values			= NULL;
	if (real_keyframe_count > 0) {
		rotations.KeyTimes	= new float[real_keyframe_count];
		rotations.Values		= new float[real_keyframe_count];
		unsigned int index;
		for (index = 1; index < NumRotationKeyFrames; index ++) {
			rotations.KeyTimes[index - 1]	= ((float)RotationKeyFrameTimes[index]) / 1000;
			rotations.Values[index - 1]	= RotationKeyFrameValues[index] * 1000.0f;
		}
		if (create_last_keyframe) {			
			rotations.KeyTimes[index - 1] = ((float)MaxAge / 1000);
			float start_rotation				= RotationKeyFrameValues[index - 1];
			float delta							= 2.0f * HalfRotationKeyFrameDeltas[NumRotationKeyFrames - 1];
			float time_delta					= (float)(MaxAge - RotationKeyFrameTimes[index - 1]);
			rotations.Values[index - 1]	= (start_rotation + (delta * time_delta)) * 1000.0f;
		}
	}

	return ;
}


void ParticleBufferClass::Get_Frame_Key_Frames (ParticlePropertyStruct<float> &frames) const
{
	int real_keyframe_count = (NumFrameKeyFrames > 0) ? (NumFrameKeyFrames - 1) : 0;
	bool create_last_keyframe = false;
	if ((FrameKeyFrameDeltas != NULL) &&
		 (FrameKeyFrameDeltas[NumFrameKeyFrames - 1] != 0)) {
		real_keyframe_count ++;
		create_last_keyframe = true;
	}

	frames.Start			= FrameKeyFrameValues[0];
	frames.Rand				= FrameRandom;
	frames.NumKeyFrames	= real_keyframe_count;
	frames.KeyTimes		= NULL;
	frames.Values			= NULL;
	if (real_keyframe_count > 0) {
		frames.KeyTimes	= new float[real_keyframe_count];
		frames.Values		= new float[real_keyframe_count];
		unsigned int index;
		for (index = 1; index < NumFrameKeyFrames; index ++) {
			frames.KeyTimes[index - 1]	= ((float)FrameKeyFrameTimes[index]) / 1000;
			frames.Values[index - 1]	= FrameKeyFrameValues[index];
		}
		if (create_last_keyframe) {			
			frames.KeyTimes[index - 1] = ((float)MaxAge / 1000);
			float start_frame			= FrameKeyFrameValues[index - 1];
			float &delta				= FrameKeyFrameDeltas[NumFrameKeyFrames - 1];
			float time_delta			= (float)(MaxAge - FrameKeyFrameTimes[index - 1]);
			frames.Values[index - 1]	= start_frame + (delta * time_delta);
		}
	}

	return ;
}

void ParticleBufferClass::Get_Blur_Time_Key_Frames (ParticlePropertyStruct<float> &blurtimes) const
{
	int real_keyframe_count = (NumBlurTimeKeyFrames > 0) ? (NumBlurTimeKeyFrames - 1) : 0;
	bool create_last_keyframe = false;
	if ((BlurTimeKeyFrameDeltas != NULL) &&
		 (BlurTimeKeyFrameDeltas[NumBlurTimeKeyFrames - 1] != 0)) {
		real_keyframe_count ++;
		create_last_keyframe = true;
	}

	blurtimes.Start			= BlurTimeKeyFrameValues[0];
	blurtimes.Rand				= BlurTimeRandom;
	blurtimes.NumKeyFrames	= real_keyframe_count;
	blurtimes.KeyTimes		= NULL;
	blurtimes.Values			= NULL;
	if (real_keyframe_count > 0) {
		blurtimes.KeyTimes	= new float[real_keyframe_count];
		blurtimes.Values		= new float[real_keyframe_count];
		unsigned int index;
		for (index = 1; index < NumBlurTimeKeyFrames; index ++) {
			blurtimes.KeyTimes[index - 1]	= ((float)BlurTimeKeyFrameTimes[index]) / 1000;
			blurtimes.Values[index - 1]	= BlurTimeKeyFrameValues[index];
		}
		if (create_last_keyframe) {			
			blurtimes.KeyTimes[index - 1] = ((float)MaxAge / 1000);
			float start_blurtime		= BlurTimeKeyFrameValues[index - 1];
			float &delta				= BlurTimeKeyFrameDeltas[NumBlurTimeKeyFrames - 1];
			float time_delta			= (float)(MaxAge - BlurTimeKeyFrameTimes[index - 1]);
			blurtimes.Values[index - 1]	= start_blurtime + (delta * time_delta);
		}
	}

	return ;
}
void ParticleBufferClass::Set_LOD_Max_Screen_Size(int lod_level,float max_screen_size)
{
	if ((lod_level <0) || (lod_level > 17)) {
		return;
	}
	LODMaxScreenSizes[lod_level] = max_screen_size;
}


float ParticleBufferClass::Get_LOD_Max_Screen_Size(int lod_level)
{
	if ((lod_level <0) || (lod_level > 17)) {
		return NO_MAX_SCREEN_SIZE;
	}
	return LODMaxScreenSizes[lod_level];
}


int ParticleBufferClass::Get_Line_Texture_Mapping_Mode(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Get_Texture_Mapping_Mode();
	} 
	return SegLineRendererClass::UNIFORM_WIDTH_TEXTURE_MAP;
}

int ParticleBufferClass::Is_Merge_Intersections(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Is_Merge_Intersections();
	} 
	return false;
}

int ParticleBufferClass::Is_Freeze_Random(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Is_Freeze_Random();
	} 
	return false;
}

int ParticleBufferClass::Is_Sorting_Disabled(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Is_Sorting_Disabled();
	} 
	return false;
}

int ParticleBufferClass::Are_End_Caps_Enabled(void)	const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Are_End_Caps_Enabled();
	} 
	return false;
}

int ParticleBufferClass::Get_Subdivision_Level(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Get_Current_Subdivision_Level();
	} 
	return 0;
}

float ParticleBufferClass::Get_Noise_Amplitude(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Get_Noise_Amplitude();
	} 
	return 0.0f;
}

float ParticleBufferClass::Get_Merge_Abort_Factor(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Get_Merge_Abort_Factor();
	} 
	return 0.0f;
}

float ParticleBufferClass::Get_Texture_Tile_Factor(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Get_Texture_Tile_Factor();
	} 
	return 1.0f;
}

Vector2 ParticleBufferClass::Get_UV_Offset_Rate(void) const
{
	if (LineRenderer != NULL) {
		return LineRenderer->Get_UV_Offset_Rate();
	} 
	return Vector2(0.0f,0.0f);
}

ParticleBufferClass::TailDiffuseTypeEnum ParticleBufferClass::Determine_Tail_Diffuse()
{
	TextureClass *tex=Get_Texture();
	if (tex)
	{
		REF_PTR_RELEASE(tex);
		return SAME_AS_HEAD;
	}

	ShaderClass shader=Get_Shader();
	if (shader.Get_Dst_Blend_Func()==ShaderClass::DSTBLEND_SRC_COLOR) return WHITE;
	else if ((shader.Get_Src_Blend_Func()==ShaderClass::SRCBLEND_ONE) && (shader.Get_Dst_Blend_Func()==ShaderClass::DSTBLEND_ONE)) return BLACK;
	else if ((shader.Get_Src_Blend_Func()==ShaderClass::SRCBLEND_ONE) && (shader.Get_Dst_Blend_Func()==ShaderClass::DSTBLEND_ONE_MINUS_SRC_COLOR)) return BLACK;
	else if ((shader.Get_Src_Blend_Func()==ShaderClass::SRCBLEND_SRC_ALPHA) && (shader.Get_Dst_Blend_Func()==ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA)) return SAME_AS_HEAD_ALPHA_ZERO;
	else if (shader.Get_Alpha_Test()==ShaderClass::ALPHATEST_ENABLE) return SAME_AS_HEAD_ALPHA_ZERO;
	return SAME_AS_HEAD;
}

TextureClass * ParticleBufferClass::Get_Texture (void) const
{
	if (PointGroup) return PointGroup->Get_Texture();
	else if (LineGroup) return LineGroup->Get_Texture();
	else if (LineRenderer) return LineRenderer->Get_Texture();
	return NULL;
}

void ParticleBufferClass::Set_Texture (TextureClass *tex)
{
	if (PointGroup) PointGroup->Set_Texture(tex);
	else if (LineGroup) LineGroup->Set_Texture(tex);
	else if (LineRenderer) LineRenderer->Set_Texture(tex);
}

ShaderClass ParticleBufferClass::Get_Shader (void) const
{
	if (PointGroup) return PointGroup->Get_Shader();
	else if (LineGroup) return LineGroup->Get_Shader();
	else if (LineRenderer) return LineRenderer->Get_Shader();
	return ShaderClass::_PresetOpaqueShader;
}
