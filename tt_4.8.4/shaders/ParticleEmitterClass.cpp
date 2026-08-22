#include "General.h"
#include "ParticleEmitterClass.h"
#include "ParticleEmitterDefClass.h"
#include "ParticleBufferClass.h"
#include "SceneClass.h"
#include "WW3DAssetManager.h"
#include "ww3d.h"
#include "WWMath.h"
Vector3 InheritedWorldSpaceEmitterVel;
bool ParticleEmitterClass::DebugDisable = false;
REF_DEF1(ParticleEmitterClass::DefaultRemoveOnComplete, bool, 0x007FF170);
RENEGADE_FUNCTION
void W3dUtilityClass::Convert_Shader(W3dShaderStruct const &,ShaderClass *)
AT1(0x005B2920);
RENEGADE_FUNCTION
void W3dUtilityClass::Convert_Shader(ShaderClass const &,W3dShaderStruct *)
AT1(0x005B29F0);
ParticleEmitterClass *
ParticleEmitterClass::Create_From_Definition (const ParticleEmitterDefClass &definition)
{
	ParticleEmitterClass *pemitter = NULL;
	const char *ptexture_filename = definition.Get_Texture_Filename ();
	TextureClass *ptexture = NULL;
	if (ptexture_filename && ptexture_filename[0]) {
		ptexture = WW3DAssetManager::TheInstance->Get_Texture(
			ptexture_filename,
			TextureClass::MIP_LEVELS_ALL,
			WW3D_FORMAT_UNKNOWN,true);
	}
	ShaderClass shader;
	definition.Get_Shader (shader);
	if (WW3DAssetManager::TheInstance->Activate_Fog_On_Load) {
		shader.Enable_Fog ("ParticleEmitterClass");
	}
	ParticlePropertyStruct<Vector3> color_keys;
	ParticlePropertyStruct<float> opacity_keys;
	ParticlePropertyStruct<float> size_keys;
	ParticlePropertyStruct<float> rotation_keys;
	ParticlePropertyStruct<float> frame_keys;
	ParticlePropertyStruct<float> blur_time_keys;
	definition.Get_Color_Keyframes (color_keys);
	definition.Get_Opacity_Keyframes (opacity_keys);
	definition.Get_Size_Keyframes (size_keys);
	definition.Get_Rotation_Keyframes (rotation_keys);
	definition.Get_Frame_Keyframes (frame_keys);
	definition.Get_Blur_Time_Keyframes (blur_time_keys);
	pemitter = new ParticleEmitterClass(definition.Get_Emission_Rate (),
																definition.Get_Burst_Size (),
																definition.Get_Creation_Volume (),
																definition.Get_Velocity (), 
																definition.Get_Velocity_Random (),
																definition.Get_Outward_Vel (),
																definition.Get_Vel_Inherit (), 
																color_keys,
																opacity_keys,
																size_keys,
																rotation_keys,
																definition.Get_Initial_Orientation_Random(),
																frame_keys,
																blur_time_keys,
																definition.Get_Acceleration (),
																definition.Get_Lifetime (),
																ptexture,
																shader,
																(int)definition.Get_Max_Emissions (),
																0,
																false,
																definition.Get_Render_Mode (),
																definition.Get_Frame_Mode (),
																definition.Get_Line_Properties ());
	if (color_keys.KeyTimes != NULL) delete [] color_keys.KeyTimes;
	if (color_keys.Values != NULL) delete [] color_keys.Values;
	if (opacity_keys.KeyTimes != NULL) delete [] opacity_keys.KeyTimes;
	if (opacity_keys.Values != NULL) delete [] opacity_keys.Values;
	if (size_keys.KeyTimes != NULL) delete [] size_keys.KeyTimes;
	if (size_keys.Values != NULL) delete [] size_keys.Values;
	if (rotation_keys.KeyTimes != NULL) delete [] rotation_keys.KeyTimes;
	if (rotation_keys.Values != NULL) delete [] rotation_keys.Values;
	if (frame_keys.KeyTimes != NULL) delete [] frame_keys.KeyTimes;
	if (frame_keys.Values != NULL) delete [] frame_keys.Values;
	if (blur_time_keys.KeyTimes != NULL) delete [] blur_time_keys.KeyTimes;
	if (blur_time_keys.Values != NULL) delete [] blur_time_keys.Values;
	pemitter->Set_Name (definition.Get_Name ());
	if (ptexture) {
		REF_PTR_RELEASE(ptexture);
		ptexture = 0;
	}
	return pemitter;
}
ParticleEmitterClass::ParticleEmitterClass(float emit_rate, unsigned int burst_size,
			Vector3Randomizer *pos_rnd, Vector3 base_vel, Vector3Randomizer *vel_rnd, float out_vel,
			float vel_inherit_factor, 
			ParticlePropertyStruct<Vector3> &color,
			ParticlePropertyStruct<float> &opacity, 
			ParticlePropertyStruct<float> &size,
			ParticlePropertyStruct<float> &rotation, float orient_rnd,
			ParticlePropertyStruct<float> &frames,
			ParticlePropertyStruct<float> &blur_times,
			Vector3 accel, float max_age, TextureClass *tex, ShaderClass shader, int max_particles,
			int max_buffer_size, bool pingpong,int render_mode,int frame_mode,
			const W3dEmitterLinePropertiesStruct * line_props
) :
	OneTimeBurstSize(1),
	OneTimeBurst(false),
	PosRand(pos_rnd),
	BaseVel(base_vel * 0.001f),
	VelRand(vel_rnd),
	OutwardVel(out_vel * 0.001f),
	VelInheritFactor(vel_inherit_factor),
	EmitRemain(0U),
	PrevQ(true),
	PrevOrig(0.0, 0.0, 0.0),
	Active(false),
	FirstTime(true),
	ParticlesLeft(max_particles),
	MaxParticles(max_particles),
	IsComplete(false),
	RemoveOnComplete(DefaultRemoveOnComplete),
	NameString(NULL),
	UserString(NULL),
	IsInScene(false)
{
	EmitRate		= emit_rate	>	0.0f ? (unsigned int)(1000.0f / emit_rate) : 1000U;
	BurstSize	= burst_size != 0	? burst_size : 1;
	max_age		= max_age	> 	0.0f ? max_age : 1.0f;
	VelRand->Scale(0.001f);
	int max_num = (int)(BurstSize * emit_rate * (max_age + 1));
	if (max_particles > 0) max_num = MIN(max_num, max_particles);
	if (max_buffer_size > 0) max_num = MIN(max_num, max_buffer_size);
	max_num = MAX(max_num, 2);
	Buffer = new ParticleBufferClass(this, max_num, color, opacity, size, rotation, orient_rnd,
		frames, blur_times, accel/1000000.0f,max_age, tex, shader, pingpong, render_mode, frame_mode,
		line_props);
	BufferSceneNeeded = true;
	NameString = newstr("ParticleEmitter");
}
ParticleEmitterClass::ParticleEmitterClass(const ParticleEmitterClass & src) :
	IsInScene(false),
	RenderObjClass(src)
{
	EmitRate = src.EmitRate;
	BurstSize = src.BurstSize;
	OneTimeBurstSize = src.OneTimeBurstSize;
	OneTimeBurst = src.OneTimeBurst;
	if (src.PosRand) {
		PosRand = src.PosRand->Clone();
	} else {
		PosRand = NULL;
	}
	BaseVel = src.BaseVel;
	if (src.VelRand) {
		VelRand = src.VelRand->Clone();
	} else {
		VelRand = NULL;
	}
	OutwardVel = src.OutwardVel;
	VelInheritFactor = src.VelInheritFactor;
	EmitRemain = src.EmitRemain;
	PrevQ = src.PrevQ;
	PrevOrig = src.PrevOrig;
	MaxParticles = src.MaxParticles;
	ParticlesLeft = src.ParticlesLeft;
	Buffer = (ParticleBufferClass *) src.Buffer->Clone();
	Buffer->Set_Emitter(this);
	BufferSceneNeeded = true;
	Active = true;
	FirstTime = true;
	IsComplete = false;
	NameString = newstr(src.NameString);
}
ParticleEmitterClass & ParticleEmitterClass::operator = (const ParticleEmitterClass & that)
{
	RenderObjClass::operator = (that);
	if (this != &that) {
	}
	return * this;
}
ParticleEmitterClass::~ParticleEmitterClass(void)
{
	Buffer->Emitter_Is_Dead();
	Buffer->Release_Ref();
	if (PosRand != NULL) {
		delete PosRand;
		PosRand = NULL;
	}
	if (VelRand != NULL) {
		delete VelRand;
		VelRand = NULL;
	}
	if (NameString != NULL) {
		delete[] (NameString);
		NameString = NULL;
	}
	return ;
}
RenderObjClass * ParticleEmitterClass::Clone(void) const
{
	return new ParticleEmitterClass(*this);
}
void ParticleEmitterClass::Restart(void)
{
	Start();
}
void ParticleEmitterClass::Notify_Added(SceneClass * scene)
{
	RenderObjClass::Notify_Added(scene);
	scene->Register(this,SceneClass::ON_FRAME_UPDATE);
	if (FirstTime == false) {
		Active = true;
	}
	IsInScene = true;
}
void ParticleEmitterClass::Notify_Removed(SceneClass * scene)
{
	scene->Unregister(this,SceneClass::ON_FRAME_UPDATE);
	RenderObjClass::Notify_Removed(scene);
	Active = false;
	IsInScene = false;
}
void ParticleEmitterClass::Scale(float scale)
{
	if (PosRand) PosRand->Scale(scale);
	BaseVel *= scale;
	if (VelRand) VelRand->Scale(scale);
	OutwardVel *= scale;
	Buffer->Scale(scale);
}
void ParticleEmitterClass::On_Frame_Update(void)
{
	if (Active && !IsComplete) {
		if (FirstTime) {
			if ( BufferSceneNeeded ) {
				if (Is_In_Scene()) {
					Buffer->Add(Scene);
					BufferSceneNeeded = false;
				} else {
					return;
				}
			}
			BufferSceneNeeded = false;
			PrevQ = Build_Quaternion(Get_Transform());
			PrevOrig = Get_Transform().Get_Translation();
			FirstTime = false;
		}
	}
	if (Is_Complete()) {
		if (Is_In_Scene() && Is_Remove_On_Complete_Enabled()) {
			Scene->Register(this,SceneClass::RELEASE);
		}
	}
}
void ParticleEmitterClass::Reset(void)
{
	Active = true;
	PrevQ = Build_Quaternion(Get_Transform());
	PrevOrig = Get_Transform().Get_Translation();
	ParticlesLeft = MaxParticles;
	EmitRemain = 0;	
	IsComplete = false;
}
void ParticleEmitterClass::Start(void)
{
	Active = true;
	PrevQ = Build_Quaternion(Get_Transform());
	PrevOrig = Get_Transform().Get_Translation();
	if (IsComplete == true) {
		ParticlesLeft = MaxParticles;
		IsComplete = false;
	}
}
void ParticleEmitterClass::Stop(void)
{
	Active = false;
}
bool ParticleEmitterClass::Is_Stopped(void)
{
	return (Active == false);
}
void ParticleEmitterClass::Set_Position_Randomizer(Vector3Randomizer *rand)
{
	if (PosRand) {
		delete PosRand;
		PosRand =NULL;
	}
	PosRand = rand;
}
void ParticleEmitterClass::Set_Velocity_Randomizer(Vector3Randomizer *rand)
{
	if (VelRand) {
		delete VelRand;
		VelRand =NULL;
	}
	VelRand = rand;
	if (VelRand) {
		VelRand->Scale(0.001f);
	}
}
Vector3Randomizer *ParticleEmitterClass::Get_Creation_Volume (void) const
{
	Vector3Randomizer *randomizer = NULL;
	if (PosRand != NULL) {
		randomizer = PosRand->Clone ();
	}
	return randomizer;
}
Vector3Randomizer *ParticleEmitterClass::Get_Velocity_Random (void) const	
{
	Vector3Randomizer *randomizer = NULL;
	if (VelRand != NULL) {
		randomizer = VelRand->Clone ();
		randomizer->Scale (1000.0F);
	}
	return randomizer;
}
void ParticleEmitterClass::Set_Base_Velocity(const Vector3& base_vel)
{
	BaseVel = base_vel * 0.001f;
}
void ParticleEmitterClass::Set_Outwards_Velocity(float out_vel)
{
	OutwardVel = out_vel * 0.001f;
}
void ParticleEmitterClass::Set_Velocity_Inheritance_Factor(float inh_factor)
{
	VelInheritFactor = inh_factor;
}
void ParticleEmitterClass::Emit(void)
{
	if (Active && !IsComplete) {
		Quaternion curr_quat;
		Vector3 curr_orig;
	   curr_quat = Build_Quaternion(Get_Transform());
	   curr_orig = Get_Transform().Get_Translation();
	   Create_New_Particles(curr_quat, curr_orig);
	   PrevQ = curr_quat;
	   PrevOrig = curr_orig;
	} else {
	   PrevQ = Build_Quaternion(Get_Transform());
	   PrevOrig = Get_Transform().Get_Translation();
	}
}
void ParticleEmitterClass::Update_Cached_Bounding_Volumes(void) const
{ 
	CachedBoundingSphere.Init(Get_Position(),0.0); 
	CachedBoundingBox.Center = Get_Position();
	CachedBoundingBox.Extent.Set(0,0,0);
	Validate_Cached_Bounding_Volumes();
}
RENEGADE_FUNCTION
void ParticleEmitterClass::Create_New_Particles(const Quaternion & curr_quat, const Vector3 & curr_orig)
AT1(0x00562870);
void ParticleEmitterClass::Initialize_Particle(NewParticleStruct * newpart,
   unsigned int timestamp, const Quaternion & quat, const Vector3 & orig)
{
	newpart->TimeStamp = timestamp;
	Vector3 rand_pos;
	if (PosRand) {
		PosRand->Get_Vector(rand_pos);
	} else {
		rand_pos.Set(0.0, 0.0, 0.0);
	}
	newpart->Position = quat.Rotate_Vector(rand_pos) + orig;
	Vector3 rand_vel;
	if (VelRand) {
		VelRand->Get_Vector(rand_vel);
	} else {
		rand_vel.Set(0.0, 0.0, 0.0);
	}
	if (OutwardVel) {
		Vector3 outwards;
		float pos_l2 = rand_pos.Length2();
		if (pos_l2) {
			outwards = rand_pos * (OutwardVel * WWMath::Inv_Sqrt(pos_l2));
		} else {
			outwards.X = OutwardVel;
			outwards.Y = 0.0f;
			outwards.Z = 0.0f;
		}
		rand_vel += outwards;
	}
	rand_vel += BaseVel;
	newpart->Velocity = InheritedWorldSpaceEmitterVel + quat.Rotate_Vector(rand_vel);
}
RENEGADE_FUNCTION
ParticleEmitterDefClass* ParticleEmitterClass::Build_Definition (void) const
AT1(0x00562EB0);
WW3DErrorType
ParticleEmitterClass::Save (ChunkSaveClass &chunk_save) const
{
	WW3DErrorType ret_val = WW3D_ERROR_SAVE_FAILED;
	ParticleEmitterDefClass *pdefinition = Build_Definition ();
	if (pdefinition != NULL) {
		ret_val = pdefinition->Save_W3D (chunk_save);
	}
	return ret_val;
}
void ParticleEmitterClass::Set_Name(const char *name)
{
	SAFE_DELETE_ARRAY(NameString);
	NameString = newstr(name);
}
void
ParticleEmitterClass::Update_On_Visibilty(void)
{
	if (Is_Not_Hidden_At_All () && Is_Stopped () && IsInScene) {
		Start ();
	} else if ((Is_Not_Hidden_At_All () == false) && (Is_Stopped () == false)) {
		Stop ();
	}
	return ;
}
void
ParticleEmitterClass::Add_Dependencies_To_List
(
	DynamicVectorClass<StringClass> &file_list,
	bool textures_only
)
{
	TextureClass *texture = Get_Texture ();
	if (texture != NULL) {
		file_list.Add (texture->FullPath);
		REF_PTR_RELEASE(texture);
	}
	RenderObjClass::Add_Dependencies_To_List (file_list, textures_only);
	return ;
}
