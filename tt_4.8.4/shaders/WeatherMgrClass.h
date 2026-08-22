#ifndef TT_INCLUDE__WEATHERMGRCLASS_H
#define TT_INCLUDE__WEATHERMGRCLASS_H
#include "soundenvironmentclass.h"
#include "Vector2.h"
#include "renderobjclass.h"
#include "engine_3d.h"
#include "Random2Class.h"
#include "saveloadsubsystemclass.h"
#include "NetworkObjectClass.h"
class AudibleSoundClass;
class DX8IndexBufferClass;
class WindClass
{
public:
	WindClass(float heading, float speed, float variability, SoundEnvironmentClass *soundenvironment);
	SHADERS_API ~WindClass();
	Vector2 Get_Velocity()
	{
		return Velocity;
	}
	void Set(float heading, float speed, float variability);
	bool Update();
protected:
	enum {
		OCTAVE_COUNT = 2
	};
	SoundEnvironmentClass *SoundEnvironment;
	float Heading;
	float Speed;
	float Variability;
	double Theta[OCTAVE_COUNT];
	Vector2 Velocity;
	AudibleSoundClass *Sound;
};
class WeatherSystemClass : public RenderObjClass
{
public:
	enum {
		GROWTH_STEP = 256
	};
	enum RenderModeEnum {
		RENDER_MODE_AXIS_ALIGNED,
		RENDER_MODE_CAMERA_ALIGNED,
		RENDER_MODE_SURFACE_ALIGNED
	};
	WeatherSystemClass(PhysicsSceneClass *scene,float emittersize,float emitterheight,float particledensity,float particlesperunitlength,float particlewidth,float particleheight,float particlespeed,const Vector2 &pageoffset,const Vector2 &pagesize,unsigned pagecount,bool staticpageexists,float minstatictime,float maxstatictime,RenderModeEnum rendermode,bool decayaftercollision,bool prime);
	~WeatherSystemClass();
	RenderObjClass *Clone() const
	{
		return 0;
	}
	void Set_Density(float density);
	void Render(RenderInfoClass &rinfo);
	void Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const;
	void Get_Obj_Space_Bounding_Box(AABoxClass &box) const;
	virtual bool Update(WindClass *wind, const Vector3 &cameraposition);
protected:
	enum {
		VERTICES_PER_TRIANGLE = 3,
		MAX_IB_PARTICLE_COUNT = 2048,
		MAX_AGE = 1000000
	};
	struct RayStruct : public AutoPoolClass <RayStruct, GROWTH_STEP>
	{
	public:
		RayStruct *Next;
		bool Initialized;
		bool RayCast;
		Vector3 ParticleVelocity;
		Vector2 StartPosition;
		Vector3 EndPosition;
		bool ValidSurfaceNormal;
		Vector3 SurfaceNormal;
	};
	struct ParticleStruct : public AutoPoolClass <ParticleStruct, GROWTH_STEP>
	{
	public:
		ParticleStruct *Prev;
		ParticleStruct *Next;
		float CollisionTime;
		float LifeTime;
		float ElapsedTime;
		Vector3 Velocity;
		Vector2 UnitZVelocity;
		Vector3 CollisionPosition;
		Vector3 CurrentPosition;
		Vector3 SurfaceNormal;
		unsigned char Page;
		unsigned char RenderMode;
		unsigned char Pad[2];
	};
	float Spawn_Count(float time)
	{
		return ParticleDensity * EmitterSize * EmitterSize * time;
	}
	bool Can_Spawn(const RayStruct *rayptr)
	{
		return rayptr->EndPosition.Z < EmitterPosition.Z;
	}
	bool Spawn(RayStruct *suppliedrayptr = NULL);
	void Kill(ParticleStruct *particleptr);
	PhysicsSceneClass *Scene;
	float Age;
	float EmitterSize;
	float EmitterHeight;
	Vector3 EmitterPosition;
	float ParticleDensity;
	float ParticlesPerUnitLength;
	float ParticleSpeed;
	Vector3 ParticleVelocity;
	float HalfParticleWidth;
	float HalfParticleHeight;
	RayStruct *RayHead;
	unsigned RayCount;
	RayStruct *RaySpawnPtr;
	RayStruct *RayUpdatePtr;
	float MinRayEndZ;
	float SpawnCountFraction;
	Vector3 SceneMin, SceneMax;
	Vector3 ObjectMin, ObjectMax;
	ParticleStruct *ParticleHead;
	unsigned ParticleCount;
	DX8IndexBufferClass *IndexBuffer;
	VertexMaterialClass *Material;
	ShaderClass Shader;
	TextureClass *Texture;
	Vector2 *TextureArray;
	RenderModeEnum RenderMode;
	bool DecayAfterCollision;
	unsigned PageCount;
	bool StaticPageExists;
	float MinStaticTime;
	float MaxStaticTime;
	Vector3 CameraPosition;
	bool CameraPositionValid;
	static Random2Class _RandomNumber;
	static unsigned _GlobalParticleCount;
};
class RainSystemClass : public WeatherSystemClass
{
public:
	RainSystemClass(PhysicsSceneClass *scene, float particledensity, WindClass *wind, SoundEnvironmentClass *soundenvironment, bool prime);
	virtual ~RainSystemClass();
	bool Update(WindClass *wind, const Vector3 &cameraposition);
protected:
	enum {
		PAGE_COUNT = 4
	};
	AudibleSoundClass *Sound;
	SoundEnvironmentClass *SoundEnvironment;
};
class SnowSystemClass : public WeatherSystemClass
{
public:
	SnowSystemClass(PhysicsSceneClass *scene, float particledensity, WindClass *wind, bool prime);
	bool Update(WindClass *wind, const Vector3 &cameraposition);
protected:
	enum {
		PAGE_COUNT = 4
	};
};
class AshSystemClass : public WeatherSystemClass
{
public:
	AshSystemClass(PhysicsSceneClass *scene, float particledensity, WindClass *wind, bool prime);
	bool Update(WindClass *wind, const Vector3 &cameraposition);
protected:
	enum {
		PAGE_COUNT = 4
	};
};
class WeatherParameterClass
{
public:
	SHADERS_API void Initialize();
	SHADERS_API void Set(float targetvalue, float ramptime, bool override);
	void Set(float overrideramptime)
	{
		OverrideDuration = overrideramptime;
	}
	float Value()
	{
		return (CurrentValue);
	}
	bool Update(float time, bool override);
	void Update(float &value, float &target, float &duration, float time);
private:
	float CurrentValue;
	float NormalValue;
	float NormalTarget;
	float NormalDuration;
	float OverrideTarget;
	float OverrideDuration;
	friend class WeatherMgrClass;
};
class WeatherMgrClass : public SaveLoadSubSystemClass, public NetworkObjectClass
{
public:
	enum PrecipitationEnum {
		PRECIPITATION_FIRST,
		PRECIPITATION_RAIN = PRECIPITATION_FIRST,
		PRECIPITATION_SNOW,
		PRECIPITATION_ASH,
		PRECIPITATION_COUNT
	};
	WeatherMgrClass();
	~WeatherMgrClass();
	uint32 Chunk_ID() const;
	const char *Name() const;
	void Delete();
	virtual void Set_Delete_Pending();
	bool Save(ChunkSaveClass &csave);
	bool Load(ChunkLoadClass &cload);
	bool Load_Micro_Chunks(ChunkLoadClass &cload);
	void Export_Rare(BitStreamClass &packet);
	void Import_Rare(BitStreamClass &packet);
	static void Init(SoundEnvironmentClass *soundenvironment);
	static void	Reset();
	static void Shutdown();
	static bool Save_Dynamic(ChunkSaveClass &csave);
	static bool Load_Dynamic(ChunkLoadClass &cload);
	static bool Load_Dynamic_Micro_Chunks(ChunkLoadClass &cload);
	static bool Set_Wind(float heading, float speed, float variability, float ramptime = 0.0f);
	static bool Override_Wind(float heading, float speed, float variability, float ramptime = 0.0f);
	static void Get_Wind(float &heading, float &speed, float &variability);
	static void Restore_Wind(float ramptime);
	static bool Set_Precipitation(PrecipitationEnum precipitation, float density, float ramptime = 0.0f);
	static bool Override_Precipitation(PrecipitationEnum precipitation, float density, float ramptime = 0.0f);
	static void Get_Precipitation(PrecipitationEnum precipitation, float &density);
	static void Restore_Precipitation(float ramptime);
	static void Set_Fog_Enable(bool enabled);
	static bool Get_Fog_Enable();
	static bool Set_Fog_Range(float startdistance, float enddistance, float ramptime = 0.0f);
	static void Get_Fog_Range(float &startdistance, float &enddistance);
	static void Update(PhysicsSceneClass *scene, CameraClass *camera);
	static void Render(const CameraClass *camera);
private:
	#define VARID_PARAMETER(varname) \
		VARID_ ## varname ## _CURRENT_VALUE, \
		VARID_ ## varname ## _NORMAL_VALUE,	\
		VARID_ ## varname ## _NORMAL_TARGET, \
		VARID_ ## varname ## _NORMAL_DURATION, \
		VARID_ ## varname ## _OVERRIDE_TARGET,	\
		VARID_ ## varname ## _OVERRIDE_DURATION
	enum {
		CHUNKID_MICRO_CHUNKS			 = 0x03020113,
		CHUNKID_DYNAMIC_MICRO_CHUNKS = 0x11020245
	};
	enum {
		VARID_DUMMY = 0x09,
		VARID_PARAMETER(WIND_HEADING),
		VARID_PARAMETER(WIND_SPEED),
		VARID_PARAMETER(WIND_VARIABILITY),
		VARID_PARAMETER(RAIN_DENSITY),
		VARID_PARAMETER(SNOW_DENSITY),
		VARID_PARAMETER(ASH_DENSITY),
		VARID_WIND_OVERRIDE_COUNT,
		VARID_PRECIPITATION_OVERRIDE_COUNT,
		VARID_FOG_ENABLED,
		VARID_PARAMETER(FOG_START_DISTANCE),
		VARID_PARAMETER(FOG_END_DISTANCE)
	};
	#undef VARID_PARAMETER
	enum {
		PARAMETER_WIND_HEADING,
		PARAMETER_WIND_SPEED,
		PARAMETER_WIND_VARIABILITY,
		PARAMETER_RAIN_DENSITY,
		PARAMETER_SNOW_DENSITY,
		PARAMETER_ASH_DENSITY,
		PARAMETER_FOG_START_DISTANCE,
		PARAMETER_FOG_END_DISTANCE,
		PARAMETER_COUNT
	};
	static bool Set_Wind(float heading, float speed, float variability, float ramptime, bool override);
	static bool Set_Precipitation(PrecipitationEnum precipitation, float density, float ramptime, bool override);
	static bool Is_Dirty();
	SHADERS_API static void Set_Dirty(bool dirty = true);
	SHADERS_API static SoundEnvironmentClass *_SoundEnvironment;
	SHADERS_API static WeatherParameterClass _Parameters[PARAMETER_COUNT];
	SHADERS_API static bool	_Prime;
	SHADERS_API static bool _Imported;
	SHADERS_API static unsigned	_WindOverrideCount;
	SHADERS_API static unsigned	_PrecipitationOverrideCount;
	SHADERS_API static WindClass *_Wind;
	SHADERS_API static WeatherSystemClass *_Precipitation[PRECIPITATION_COUNT];
	SHADERS_API static bool _FogEnabled;
	static bool	_Dirty;
};
#endif
