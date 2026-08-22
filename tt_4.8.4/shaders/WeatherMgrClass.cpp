#include "General.h"
#include "WeatherMgrClass.h"
#include "AudibleSoundClass.h"
#include "WWAudioClass.h"
#include "WW3D.h"
#include "PhysicsSceneClass.h"
#include "indexbuffer.h"
#include "WW3DAssetManager.h"
#include "physcoltest.h"
#include "CameraClass.h"
#include "RenderInfoClass.h"
#include "VertexBuffer.h"
#include "CombatManager.h"
Random2Class WeatherSystemClass::_RandomNumber (0x60486223);
unsigned WeatherSystemClass::_GlobalParticleCount = 0;
SoundEnvironmentClass *WeatherMgrClass::_SoundEnvironment;
WeatherParameterClass WeatherMgrClass::_Parameters[PARAMETER_COUNT];
bool WeatherMgrClass::_Prime;
bool WeatherMgrClass::_Imported;
unsigned WeatherMgrClass::_WindOverrideCount;
unsigned WeatherMgrClass::_PrecipitationOverrideCount;
WindClass *WeatherMgrClass::_Wind;
WeatherSystemClass *WeatherMgrClass::_Precipitation[PRECIPITATION_COUNT];
bool WeatherMgrClass::_FogEnabled;
bool WeatherMgrClass::_Dirty;
void WeatherMgrClass::Set_Dirty(bool dirty)
{
	_Dirty = dirty;
}
bool WeatherMgrClass::Is_Dirty()
{
	return _Dirty;
}
WindClass::WindClass(float heading, float speed, float variability, SoundEnvironmentClass *soundenvironment) : Velocity(0.0f, 0.0f), SoundEnvironment(soundenvironment)
{
	const char *windsamplename	= "Wind01";
	Set(heading, speed, variability);
	for (unsigned octave = 0; octave < OCTAVE_COUNT; octave++)
	{
		Theta[octave] = 0.0l;
	}
	Sound = WWAudioClass::Get_Instance()->Create_Sound(windsamplename, NULL, 0, 1);
	if (Sound != NULL)
	{
		SoundEnvironment->Add_User();
		Sound->Set_Volume(0.0f);
		Sound->Play(true);
	}
}
WindClass::~WindClass()
{
	if (Sound != NULL)
	{
		Sound->Stop(true);
		REF_PTR_RELEASE(Sound);
		SoundEnvironment->Remove_User();
	}
}
void WindClass::Set(float heading, float speed, float variability)
{
	Heading = DEG_TO_RADF(heading);
	Speed = speed;
	Variability = variability;
}
bool WindClass::Update()
{
	const double frequency[OCTAVE_COUNT] = {0.5l, 0.2l};
	const double twopi = WWMATH_PI * 2.0l;
	float h, speed;
	if (Variability > 0.0f)
	{
		float f = 0.0f;
		int	d;
		for (unsigned octave = 0; octave < OCTAVE_COUNT; octave++)
		{
			Theta[octave] += (double)(WW3D::Get_Frame_Time() * 0.001l * frequency[octave]);
			d = (unsigned int)((floorf((float)(Theta[octave] / twopi))));
			if (d >= 1) Theta[octave] -= d * twopi;
			f += sinf ((float)(Theta[octave]));
		}
		speed = Speed - (Speed * ((f + 1.0f) * 0.5f) * Variability);
	}
	else
	{
		speed = Speed;
	}
	h = Heading + (0.5f * WWMATH_PI);
	Velocity.Set(cosf(h) * speed, sinf(h) * speed);
	if (Sound != NULL)
	{
		const float maxvolumespeed	= 10.0f;
		float attenuation;
		attenuation = (0.5f * (MIN(Speed, maxvolumespeed) + MIN(speed, maxvolumespeed))) / maxvolumespeed;
		Sound->Set_Volume(SoundEnvironment->Get_Amplitude() * attenuation);
	}
	return Speed > 0.0f;
}
WeatherSystemClass::WeatherSystemClass(PhysicsSceneClass *scene,float emittersize,float emitterheight,float particledensity,float particlesperunitlength,float particlewidth,float particleheight,float particlespeed,const Vector2 &pageoffset,const Vector2 &pagesize,unsigned pagecount,bool	staticpageexists,float minstatictime,float maxstatictime,RenderModeEnum rendermode,bool decayaftercollision,bool prime) : Scene(scene),EmitterSize(emittersize),EmitterHeight(emitterheight),EmitterPosition(Vector3(0.0f, 0.0f, 0.0f)),ParticlesPerUnitLength(particlesperunitlength),ParticleSpeed(particlespeed),HalfParticleWidth(particlewidth * 0.5f),HalfParticleHeight(particleheight * 0.5f),RayHead(NULL),RayCount(0),RaySpawnPtr(NULL),RayUpdatePtr(NULL),ParticleHead(NULL),ParticleCount(0),MinRayEndZ(FLT_MAX),SpawnCountFraction(0.0f),PageCount(pagecount),StaticPageExists(staticpageexists),MinStaticTime(minstatictime),MaxStaticTime(maxstatictime),RenderMode(rendermode),DecayAfterCollision(decayaftercollision),CameraPositionValid(false)
{
	const char *texturename = "WeatherParticles.tga";
	const TextureClass::MipCountType mipcount = TextureClass::MIP_LEVELS_5;
	const float oopagecount = 1.0f / pagecount;
	Age = prime ? MAX_AGE : 0.0f;
	Set_Density(particledensity);
	scene->Get_Level_Extents(SceneMin, SceneMax);
	SceneMin.Z -= 1.0f;
	SceneMax.Z += 1.0f;
	IndexBuffer = new DX8IndexBufferClass(MAX_IB_PARTICLE_COUNT * VERTICES_PER_TRIANGLE,DX8IndexBufferClass::USAGE_DEFAULT);
	{
		SortingIndexBufferClass::WriteLockClass lock(IndexBuffer);
		unsigned short *indices = lock.Get_Index_Array();
		for (unsigned i = 0; i < MAX_IB_PARTICLE_COUNT * VERTICES_PER_TRIANGLE; i++)
		{
			*indices++ = (unsigned short)i;
		}
	}
	Material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_NODIFFUSE);
	Shader = ShaderClass::_PresetAlphaShader;
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	Shader.Enable_Fog("WeatherSystemClass");
	Texture = WW3DAssetManager::TheInstance->Get_Texture(texturename, mipcount, WW3D_FORMAT_UNKNOWN, true);
	Texture->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
	Texture->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
	Set_Translucent(true);
	TextureArray = new Vector2[pagecount * VERTICES_PER_TRIANGLE];
	for (unsigned page = 0; page < pagecount; page++)
	{
		TextureArray[page * VERTICES_PER_TRIANGLE + 0].Set(pageoffset.U + (((page + 0.5f) * oopagecount) * pagesize.U), pageoffset.V + 0.0f);
		TextureArray[page * VERTICES_PER_TRIANGLE + 1].Set(pageoffset.U + (((page + 1.0f) * oopagecount) * pagesize.U), pageoffset.V + pagesize.V);
		TextureArray[page * VERTICES_PER_TRIANGLE + 2].Set(pageoffset.U + (((page + 0.0f) * oopagecount) * pagesize.U), pageoffset.V + pagesize.V);
	}
}
WeatherSystemClass::~WeatherSystemClass()
{
	RayStruct *rayptr;
	ParticleStruct *particleptr;
	rayptr = RayHead;
	while (rayptr != NULL)
	{
		RayStruct *nextrayptr;
		nextrayptr = rayptr->Next;
		delete rayptr;
		rayptr = nextrayptr;
		RayCount--;
	}
	particleptr = ParticleHead;
	while (particleptr != NULL)
	{
		ParticleStruct *nextparticleptr = particleptr->Next;
		Kill(particleptr);
		particleptr = nextparticleptr;
	}
	REF_PTR_RELEASE(Material);
	delete[]TextureArray;
	REF_PTR_RELEASE(Texture);
	REF_PTR_RELEASE(IndexBuffer);
}
void WeatherSystemClass::Set_Density(float density)
{
	unsigned raycount;
	int signedcount, r;
	RayStruct *rayptr;
	ParticleDensity = density;
	raycount = (unsigned int)(Spawn_Count(1.0f) / (ParticlesPerUnitLength * ParticleSpeed));
	signedcount = ((int) RayCount) - ((int) raycount);
	if (signedcount < 0)
	{
		for (r = signedcount; r < 0; r++) 
		{
			rayptr = new RayStruct;
			rayptr->Next = RayHead;
			rayptr->Initialized = false;
			RayHead = rayptr;
		}
		if (RaySpawnPtr  == NULL) RaySpawnPtr  = RayHead;
		if (RayUpdatePtr == NULL) RayUpdatePtr = RayHead;
	}
	else
	{
		rayptr = RayHead;
		for (r = 0; r < signedcount; r++)
		{
			RayStruct *nextrayptr;
			if (rayptr == NULL) break;
			nextrayptr = rayptr->Next;
			if (RaySpawnPtr  == rayptr) RaySpawnPtr  = nextrayptr;
			if (RayUpdatePtr == rayptr) RayUpdatePtr = nextrayptr;
			delete rayptr;
			rayptr = nextrayptr;
		}
		RayHead = rayptr;
	}
	RayCount = raycount;
}
bool WeatherSystemClass::Update(WindClass *wind, const Vector3 &cameraposition)
{
	struct BoundingBoxStruct {
		Vector2 Min, Max;
	};
	const unsigned randomness = 10000;
	const float	oorandomness = 1.0f / randomness;
	const float overlapdelta = EmitterSize * 2.0f;
	const unsigned rayupdatecount = (unsigned int)MAX(RayCount * 0.018f, 1);
	Vector3 oldemitterposition;
	float ooz;
	Vector3 emitterdirection, emitteroffset;
	Vector3 projectedemitterposition;
	BoundingBoxStruct emitterbounds;
	float deltax, deltay;
	BoundingBoxStruct nonoverlapregions[2];
	float regionthreshold;
	Vector3 sceneminoffset, scenemaxoffset;
	Vector2 raystartoffset;
	RayStruct *rayptr;
	float spawncountfraction;
	float alpha, beta;
	Vector2 range;
	Vector3 minrayendposition;
	float l, boxoffset;
	ParticleStruct *particleptr;
	float s;
	unsigned spawncount;
	float time;
	oldemitterposition = EmitterPosition;
	if (wind != NULL)
	{
		ParticleVelocity.Set(wind->Get_Velocity().X, wind->Get_Velocity().Y, -ParticleSpeed);
	}
	else
	{
		ParticleVelocity.Set(0.0f, 0.0f, -ParticleSpeed);
	}
	ooz = 1.0f / ParticleVelocity.Z;
	emitterdirection.Set(ParticleVelocity.X * ooz, ParticleVelocity.Y * ooz, 1.0f);
	emitteroffset = EmitterHeight * emitterdirection;
	EmitterPosition = cameraposition + emitteroffset;
	emitterbounds.Min.Set(EmitterPosition.X - EmitterSize, EmitterPosition.Y - EmitterSize);
	emitterbounds.Max.Set(EmitterPosition.X + EmitterSize, EmitterPosition.Y + EmitterSize);
	projectedemitterposition = oldemitterposition + ((EmitterPosition.Z - oldemitterposition.Z) * emitterdirection);
	deltax = WWMath::Fabs(EmitterPosition.X - projectedemitterposition.X);
	deltay = WWMath::Fabs(EmitterPosition.Y - projectedemitterposition.Y);
	if ((deltax < overlapdelta) && (deltay < overlapdelta) && (EmitterPosition != projectedemitterposition))
	{
		float area0, area1;
		if (EmitterPosition.X < projectedemitterposition.X)
		{
			nonoverlapregions[0].Min.X = emitterbounds.Min.X;
			nonoverlapregions[0].Max.X = projectedemitterposition.X - EmitterSize;
			nonoverlapregions[1].Min.X = nonoverlapregions[0].Max.X;
			nonoverlapregions[1].Max.X = emitterbounds.Max.X;
		}
		else
		{
			nonoverlapregions[0].Min.X = projectedemitterposition.X + EmitterSize;
			nonoverlapregions[0].Max.X = emitterbounds.Max.X;
			nonoverlapregions[1].Min.X = emitterbounds.Min.X;
			nonoverlapregions[1].Max.X = nonoverlapregions[0].Min.X;
		}
		nonoverlapregions[0].Min.Y = emitterbounds.Min.Y;
		nonoverlapregions[0].Max.Y = emitterbounds.Max.Y;
		if (EmitterPosition.Y < projectedemitterposition.Y)
		{
			nonoverlapregions[1].Min.Y = emitterbounds.Min.Y;
			nonoverlapregions[1].Max.Y = projectedemitterposition.Y - EmitterSize;
		}
		else
		{
			nonoverlapregions[1].Min.Y = projectedemitterposition.Y + EmitterSize;
			nonoverlapregions[1].Max.Y = emitterbounds.Max.Y;
		}
		area0 = (nonoverlapregions[0].Max.X - nonoverlapregions[0].Min.X) * (nonoverlapregions[0].Max.Y - nonoverlapregions[0].Min.Y);
		area1 = (nonoverlapregions[1].Max.X - nonoverlapregions[1].Min.X) * (nonoverlapregions[1].Max.Y - nonoverlapregions[1].Min.Y);
		if (area0 == 0.0f)
		{
			regionthreshold = -FLT_MAX;
		}
		else
		{
			if (area1 == 0.0f)
			{
				regionthreshold = +FLT_MAX;
			}
			else
			{
				regionthreshold = area0 / (area0 + area1);
			}
		}
	}
	else
	{
		nonoverlapregions[0] = emitterbounds;
		regionthreshold = +FLT_MAX;
	}
	raystartoffset = (EmitterPosition.Z - oldemitterposition.Z) * Vector2(emitterdirection.X, emitterdirection.Y);
	sceneminoffset = (SceneMin.Z - EmitterPosition.Z) * emitterdirection;
	scenemaxoffset = (SceneMax.Z - EmitterPosition.Z) * emitterdirection;
	spawncountfraction = 0.0f;
	rayptr = RayHead;
	while (rayptr != NULL)
	{
	  	Vector3 raystartposition;
		if (!rayptr->Initialized)
		{
			alpha = _RandomNumber(0, randomness) * oorandomness;
			beta = _RandomNumber(0, randomness) * oorandomness;
			range = emitterbounds.Max - emitterbounds.Min;
			range.Scale(alpha, beta);
			rayptr->StartPosition = emitterbounds.Min + range;
			rayptr->Initialized = true;
			rayptr->RayCast	= true;
		}
		else
		{
			rayptr->StartPosition += raystartoffset;
			rayptr->RayCast = (rayptr->StartPosition.X < emitterbounds.Min.X) || (rayptr->StartPosition.X > emitterbounds.Max.X) || (rayptr->StartPosition.Y < emitterbounds.Min.Y) || (rayptr->StartPosition.Y > emitterbounds.Max.Y);
			if (rayptr->RayCast)
			{
				unsigned regionindex;
				if ((_RandomNumber (0, randomness) * oorandomness) < regionthreshold)
				{
					regionindex = 0;
				}
				else
				{
					regionindex = 1;
				}
				alpha = _RandomNumber(0, randomness) * oorandomness;
				beta = _RandomNumber(0, randomness) * oorandomness;
				range = nonoverlapregions[regionindex].Max - nonoverlapregions[regionindex].Min;
				range.Scale(alpha, beta);
				rayptr->StartPosition = nonoverlapregions[regionindex].Min + range;
			}
			else
			{
				rayptr = rayptr->Next;
				continue;
			}
		}
		raystartposition.Set(rayptr->StartPosition.X, rayptr->StartPosition.Y, EmitterPosition.Z);
		{
			Vector3 raycaststartpoint(raystartposition + scenemaxoffset);
			Vector3 raycastendpoint(raystartposition + sceneminoffset);
			LineSegClass raycast(raycaststartpoint, raycastendpoint);
			CastResultStruct rayresult;
			PhysRayCollisionTestClass raytest(raycast, &rayresult, TERRAIN_ONLY_COLLISION_GROUP, COLLISION_TYPE_PROJECTILE);
			Scene->Cast_Ray(raytest);
			raycast.Compute_Point(raytest.Result->Fraction, &(rayptr->EndPosition));
			if (raytest.Result->Fraction < 1.0f)
			{
				rayptr->ValidSurfaceNormal = true;
				rayptr->SurfaceNormal = raytest.Result->Normal;
			}
			else
			{
				rayptr->ValidSurfaceNormal = false;
			}
		}
		rayptr->ParticleVelocity = ParticleVelocity;
		if ((Age > 0.0f) && (Can_Spawn(rayptr)))
		{
			float s2;
			unsigned spawncount2;
			s2 = ParticlesPerUnitLength * (rayptr->EndPosition - raystartposition).Quick_Length();
			spawncount2 = (unsigned int)floor(s2);
			spawncountfraction += s2 - spawncount2;
			if (spawncountfraction >= 1.0f)
			{
				spawncountfraction -= 1.0f;
				spawncount2++;
			}
			for (unsigned p = 0; p < spawncount2; p++)
			{
				Spawn(rayptr);
			}
		}
		if (rayptr->EndPosition.Z < MinRayEndZ)
		{
			MinRayEndZ = rayptr->EndPosition.Z;
		}
		rayptr = rayptr->Next;
	}
	if (RayUpdatePtr != NULL)
	{
		for (unsigned r = 0; r < rayupdatecount; r++)
		{
			if (!RayUpdatePtr->RayCast)
			{
				alpha = _RandomNumber(0, randomness) * oorandomness;
				beta = _RandomNumber(0, randomness) * oorandomness;
				range = emitterbounds.Max - emitterbounds.Min;
				range.Scale(alpha, beta);
				RayUpdatePtr->StartPosition = emitterbounds.Min + range;
				{
					Vector3	raystartposition(RayUpdatePtr->StartPosition.X, RayUpdatePtr->StartPosition.Y, EmitterPosition.Z);
					Vector3	raycaststartpoint(raystartposition + scenemaxoffset);
					Vector3 raycastendpoint(raystartposition + sceneminoffset);
					LineSegClass raycast(raycaststartpoint, raycastendpoint);
					CastResultStruct rayresult;
					PhysRayCollisionTestClass raytest(raycast, &rayresult, TERRAIN_ONLY_COLLISION_GROUP, COLLISION_TYPE_PROJECTILE);
					Scene->Cast_Ray(raytest);
					raycast.Compute_Point(raytest.Result->Fraction, &(RayUpdatePtr->EndPosition));
					if (raytest.Result->Fraction < 1.0f)
					{
						RayUpdatePtr->ValidSurfaceNormal = true;
						RayUpdatePtr->SurfaceNormal = raytest.Result->Normal;
					}
					else
					{
						RayUpdatePtr->ValidSurfaceNormal = false;
					}
				}
				RayUpdatePtr->ParticleVelocity = ParticleVelocity;
				if (RayUpdatePtr->EndPosition.Z < MinRayEndZ)
				{
					MinRayEndZ = RayUpdatePtr->EndPosition.Z;
				}
			}
			RayUpdatePtr = RayUpdatePtr->Next;
			if (RayUpdatePtr == NULL) RayUpdatePtr = RayHead;
		}
	}
	minrayendposition = EmitterPosition + ((MinRayEndZ - EmitterPosition.Z) * emitterdirection);
	l = MAX(HalfParticleWidth, HalfParticleHeight);
	boxoffset = EmitterSize + l;
	ObjectMax.Set(MAX(EmitterPosition.X, minrayendposition.X) + boxoffset, MAX(EmitterPosition.Y, minrayendposition.Y) + boxoffset, MAX(EmitterPosition.Z, minrayendposition.Z) + l);
	ObjectMin.Set(MIN(EmitterPosition.X, minrayendposition.X) - boxoffset, MIN(EmitterPosition.Y, minrayendposition.Y) - boxoffset, MIN(EmitterPosition.Z, minrayendposition.Z) - l);
	Invalidate_Cached_Bounding_Volumes();
	time = WW3D::Get_Frame_Time() * 0.001f;
	particleptr = ParticleHead;
	while (particleptr != NULL)
	{
		Vector2 emitterposition;
		bool outside;
		ParticleStruct *nextparticleptr = particleptr->Next;
		particleptr->ElapsedTime += time;
		if (particleptr->ElapsedTime >= particleptr->LifeTime)
		{
			Kill(particleptr);
			particleptr = nextparticleptr;
			continue;
		}
		if (particleptr->ElapsedTime >= particleptr->CollisionTime)
		{
			if (particleptr->Velocity.Z != 0.0f)
			{
				particleptr->Velocity.Set(0.0f, 0.0f, 0.0f);
				particleptr->CurrentPosition = particleptr->CollisionPosition;
				if (StaticPageExists) particleptr->Page = (unsigned char)PageCount - 1;
				particleptr->RenderMode = RENDER_MODE_SURFACE_ALIGNED;
			}
		}
		else
		{
			particleptr->CurrentPosition += particleptr->Velocity * time;
		}
		emitterposition = Vector2(particleptr->CurrentPosition.X, particleptr->CurrentPosition.Y) + ((EmitterPosition.Z - particleptr->CurrentPosition.Z) * particleptr->UnitZVelocity);
		outside = (emitterposition.X < emitterbounds.Min.X) || (emitterposition.X > emitterbounds.Max.X) || (emitterposition.Y < emitterbounds.Min.Y) || (emitterposition.Y > emitterbounds.Max.Y);
		if (outside)
		{
			Kill(particleptr);
		}
		particleptr = nextparticleptr;
	}
	s = Spawn_Count(time);
	spawncount = (unsigned int)floor(s);
	SpawnCountFraction += s - spawncount;
	if (SpawnCountFraction >= 1.0f)
	{
		SpawnCountFraction -= 1.0f;
		spawncount++;
	}
	for (unsigned p = 0; p < spawncount; p++)
	{
		Spawn();
	}
	if (Age < MAX_AGE) Age += time;
	return (ParticleDensity > 0.0f) || (ParticleCount > 0);
}
bool WeatherSystemClass::Spawn(RayStruct *suppliedrayptr)
{
	const unsigned maxparticlecount = USHRT_MAX / (2 * VERTICES_PER_TRIANGLE);
	if (_GlobalParticleCount < maxparticlecount)
	{
		RayStruct *rayptr;
		if (suppliedrayptr == NULL)
		{
			if (RaySpawnPtr != NULL)
			{
				rayptr = RaySpawnPtr;
				RaySpawnPtr = RaySpawnPtr->Next;
				if (RaySpawnPtr == NULL) RaySpawnPtr = RayHead;
			}
			else
			{
				return false;
			}
		}
		else
		{
			rayptr = suppliedrayptr;
		}
		if (Can_Spawn(rayptr))
		{
			const unsigned randomness = 1000;
			const float oorandomness  = 1.0f / randomness;
			const float ooz = 1.0f / rayptr->ParticleVelocity.Z;
			const float collisiontime = (rayptr->EndPosition.Z - EmitterPosition.Z) * ooz;
			ParticleStruct *particleptr;
			particleptr = new ParticleStruct;
			if (ParticleHead != NULL)
			{
				ParticleHead->Prev = particleptr;
			}
			particleptr->Prev = NULL;
			particleptr->Next = ParticleHead;
			ParticleHead = particleptr;
			ParticleCount++;
			_GlobalParticleCount++;
			particleptr->UnitZVelocity.Set(rayptr->ParticleVelocity.X * ooz, rayptr->ParticleVelocity.Y * ooz);
			particleptr->CollisionTime = collisiontime;
			particleptr->CollisionPosition = rayptr->EndPosition;
			if (rayptr->ValidSurfaceNormal)
			{
				particleptr->LifeTime = collisiontime + WWMath::Lerp(MinStaticTime, MaxStaticTime, _RandomNumber(0, randomness) * oorandomness);
				particleptr->SurfaceNormal = rayptr->SurfaceNormal;
			}
			else
			{
				particleptr->LifeTime = collisiontime;
			}
			if (suppliedrayptr == NULL)
			{
				particleptr->ElapsedTime = 0.0f;
				particleptr->Velocity = rayptr->ParticleVelocity;
				particleptr->CurrentPosition = Vector3(rayptr->StartPosition.X, rayptr->StartPosition.Y, EmitterPosition.Z);
				particleptr->Page = (unsigned char)_RandomNumber(0, PageCount - ((StaticPageExists) ? 2 : 1));
				particleptr->RenderMode = (unsigned char)RenderMode;
			}
			else
			{
				float t;
				t = _RandomNumber(0, randomness) * oorandomness * particleptr->LifeTime;
				particleptr->ElapsedTime = MIN(t, Age);
				if (particleptr->ElapsedTime >= particleptr->CollisionTime)
				{
					particleptr->Velocity.Set(0.0f, 0.0f, 0.0f);
					particleptr->CurrentPosition = rayptr->EndPosition;
					if (StaticPageExists)
					{
						particleptr->Page = (unsigned char)(PageCount - 1);
					}
					else
					{
						particleptr->Page = (unsigned char)_RandomNumber(0, PageCount - 1);
					}
					particleptr->RenderMode = RENDER_MODE_SURFACE_ALIGNED;
				}
				else
				{
					particleptr->Velocity = rayptr->ParticleVelocity;
					particleptr->CurrentPosition = Vector3(rayptr->StartPosition.X, rayptr->StartPosition.Y, EmitterPosition.Z) + (particleptr->Velocity * particleptr->ElapsedTime);
					particleptr->Page = (unsigned char)_RandomNumber(0, PageCount - ((StaticPageExists) ? 2 : 1));
					particleptr->RenderMode = (unsigned char)RenderMode;
				}
			}
			return true;
		}
	}
	return false;
}
void WeatherSystemClass::Kill(ParticleStruct *particleptr)
{
	if (particleptr->Prev != NULL)
	{
		particleptr->Prev->Next = particleptr->Next;
	}
	else
	{
		ParticleHead = particleptr->Next;
	}
	if (particleptr->Next != NULL)
	{
		particleptr->Next->Prev = particleptr->Prev;
	}
	delete particleptr;
	ParticleCount--;
	_GlobalParticleCount--;
}
void WeatherSystemClass::Render(RenderInfoClass &rinfo)
{
	if (WW3D::Are_Static_Sort_Lists_Enabled())
	{
		const unsigned sortlevel = 31;
		WW3D::Add_To_Static_Sort_List(this, sortlevel);
	}
	else
	{
		const Vector3 zero(0.0f, 0.0f, 0.0f);
		const Vector3 white(1.0f, 1.0f, 1.0f);
		Vector3 cameravelocity;
		Vector3	color;
		unsigned dxcolor;
		Matrix4 viewmatrix(true), identitymatrix(true);
		float maxalphaheight, oomaxalphaheight, deltaheight;
		Vector3 x, y;
		float w, h;
		Vector3 offset[VERTICES_PER_TRIANGLE];
		Vector3 camerafocus;
		ParticleStruct *particleptr;
		unsigned processedparticlecount, bufferparticlecount;
		if (CameraPositionValid)
		{
			cameravelocity = ((rinfo.Camera.Get_Position() - CameraPosition) / (WW3D::Get_Frame_Time() * 0.001f));
		}
		else
		{
			cameravelocity.Set(0.0f, 0.0f, 0.0f);
			CameraPositionValid = true;
		}
		CameraPosition = rinfo.Camera.Get_Position();
		dxcolor = GetD3dColor(Vector3(1.0f, 1.0f, 1.0f), 0.0f);
		maxalphaheight = EmitterHeight * 0.2f;
		oomaxalphaheight = 1.0f / maxalphaheight;
		deltaheight = rinfo.Camera.Get_Position().Z + (EmitterHeight - maxalphaheight);
		DX8Wrapper::Set_Transform(D3DTS_WORLD, identitymatrix);
		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Set_Texture(0,Texture);
		DX8Wrapper::Set_Index_Buffer(IndexBuffer, 0);
		ApplyDepthBias(12);
 		camerafocus = rinfo.Camera.Get_Transform().Get_Z_Vector();
		particleptr = ParticleHead;
		processedparticlecount = 0;
		bufferparticlecount = MIN(MAX_IB_PARTICLE_COUNT, ParticleCount);
		while (processedparticlecount < ParticleCount)
		{
			unsigned particlecount, submittedparticlecount;
			DynamicVBAccessClass dynamicvb(2,0x252,(unsigned short)(bufferparticlecount * VERTICES_PER_TRIANGLE));
			particlecount = MIN(ParticleCount - processedparticlecount, MAX_IB_PARTICLE_COUNT);
			submittedparticlecount = 0;
			{
				DynamicVBAccessClass::WriteLockClass lock(&dynamicvb);
				VertexFormatXYZNDUV2 *vertex = lock.Get_Formatted_Vertex_Array();
				for (unsigned p = 0; p < particlecount; p++)
				{
					Vector3	position;
					position = particleptr->CurrentPosition;
					if (CollisionMath::Overlap_Test(rinfo.Camera.Get_Frustum(), position) != CollisionMath::OUTSIDE)
					{
						unsigned base;
						float alphaheight, alpha;
						switch (particleptr->RenderMode)
						{
							case RENDER_MODE_AXIS_ALIGNED:
								y = particleptr->Velocity - cameravelocity;
								y /= y.Quick_Length();
								x = Vector3::Cross_Product(camerafocus, y);
								x /= x.Quick_Length();
								w = HalfParticleWidth;
								h = HalfParticleHeight;
								break;
							case RENDER_MODE_CAMERA_ALIGNED:
								x = rinfo.Camera.Get_Transform().Get_X_Vector();
								y = rinfo.Camera.Get_Transform().Get_Y_Vector();
								w = HalfParticleWidth;
								h = HalfParticleHeight;
								break;
							case RENDER_MODE_SURFACE_ALIGNED:
								if (Vector3::Dot_Product(camerafocus, particleptr->SurfaceNormal) > 0.0f)
								{
									x = Vector3::Cross_Product(camerafocus, particleptr->SurfaceNormal);
									x /= x.Quick_Length();
									y = Vector3::Cross_Product(x, particleptr->SurfaceNormal);
									if (DecayAfterCollision)
									{
										float decaytime, totaldecaytime, s;
										decaytime = particleptr->ElapsedTime - particleptr->CollisionTime;
										totaldecaytime = particleptr->LifeTime - particleptr->CollisionTime;
										if ((decaytime >= 0.0f) && (totaldecaytime > 0.0f))
										{
											s = 1.0f - (decaytime / totaldecaytime);
											w = HalfParticleWidth  * s;
											h = HalfParticleHeight * s;
										}
										else
										{
											w = h = 0.0f;
										}
									}
									else
									{
										w = HalfParticleWidth;
										h = HalfParticleHeight;
									}
									break;
								}
								else
								{
									particleptr = particleptr->Next;
									continue;
								}
 							default:
								w = h = 0.0f;
								break;
						}
						offset[0] = -h * y;
						offset[1] = h * y + w * x;
						offset[2] = h * y - w * x;
						base = particleptr->Page * VERTICES_PER_TRIANGLE;
						alphaheight = position.Z - deltaheight;
						if (alphaheight > 0.0f)
						{
							alpha = 1.0f - (alphaheight * oomaxalphaheight);
						}
						else
						{
							alpha = 1.0f;
						}
						DX8Wrapper::Set_Alpha(alpha, dxcolor);
						vertex->x = position.X + offset[0].X;
						vertex->y = position.Y + offset[0].Y;
						vertex->z = position.Z + offset[0].Z;
						vertex->diffuse = dxcolor;
						vertex->u1 = TextureArray[base].U;
						vertex->v1 = TextureArray[base].V;
						vertex++;
						vertex->x = position.X + offset[1].X;
						vertex->y = position.Y + offset[1].Y;
						vertex->z = position.Z + offset[1].Z;
						vertex->diffuse = dxcolor;
						vertex->u1 = TextureArray[base + 1].U;
						vertex->v1 = TextureArray[base + 1].V;
						vertex++;
						vertex->x = position.X + offset[2].X;
						vertex->y = position.Y + offset[2].Y;
						vertex->z = position.Z + offset[2].Z;
						vertex->diffuse = dxcolor;
						vertex->u1 = TextureArray[base + 2].U;
						vertex->v1 = TextureArray[base + 2].V;
						vertex++;
						submittedparticlecount++;
					}
					particleptr = particleptr->Next;
				}
			}
			if (submittedparticlecount > 0)
			{
				DX8Wrapper::Set_Vertex_Buffer(dynamicvb);
				DX8Wrapper::Draw_Triangles(0, (uint16)submittedparticlecount, 0, (uint16)(submittedparticlecount * VERTICES_PER_TRIANGLE));
			}
			processedparticlecount += particlecount;
		}
		ApplyDepthBias(0);
	}
}
void WeatherSystemClass::Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const
{
	sphere.Init((ObjectMin + ObjectMax) * 0.5f,((ObjectMax - ObjectMin) * 0.5f).Length());
}
void WeatherSystemClass::Get_Obj_Space_Bounding_Box(AABoxClass &box) const
{
	box.Init((ObjectMin + ObjectMax) * 0.5f,(ObjectMax - ObjectMin) * 0.5f);
}
RainSystemClass::RainSystemClass(PhysicsSceneClass *scene, float particledensity, WindClass *wind, SoundEnvironmentClass *soundenvironment, bool prime) : WeatherSystemClass (scene, 20.0f, 20.0f, particledensity, 0.2f, 0.15f, 0.45f, 15.0f, Vector2 (0.0f, 0.0f), Vector2 (1.0f, 0.5f), PAGE_COUNT, true, 0.1f, 0.2f, WeatherSystemClass::RENDER_MODE_AXIS_ALIGNED, false, prime), SoundEnvironment (soundenvironment)
{
	const char *rainsamplename	= "Rainfall01";
	Sound = WWAudioClass::Get_Instance()->Create_Sound (rainsamplename, NULL, 0, 1);
	if (Sound != NULL)
	{
		SoundEnvironment->Add_User();
		Sound->Set_Volume(0.0f);
		Sound->Play(true);
 	}
}
RainSystemClass::~RainSystemClass()
{
	if (Sound != NULL)
	{
		Sound->Stop(true);
		REF_PTR_RELEASE(Sound);
		SoundEnvironment->Remove_User();
	}
}
bool RainSystemClass::Update(WindClass *wind, const Vector3 &cameraposition)
{
	if (Sound != NULL)
	{
		const float maxvolume = 4.0f;
		const float volumeperparticle = 0.0025f;
		float attenuation;
		attenuation = MIN(ParticleCount * volumeperparticle, maxvolume) / maxvolume;
		Sound->Set_Volume(SoundEnvironment->Get_Amplitude() * attenuation);
	}
	return WeatherSystemClass::Update(wind, cameraposition);
}
SnowSystemClass::SnowSystemClass(PhysicsSceneClass *scene, float particledensity, WindClass *wind, bool prime) : WeatherSystemClass (scene, 40.0f, 20.0f, particledensity, 0.1f, 0.32f, 0.32f, 3.5f, Vector2 (0.0f, 0.5f), Vector2 (1.0f, 0.25f), PAGE_COUNT, false, 1.0f, 2.0f, WeatherSystemClass::RENDER_MODE_CAMERA_ALIGNED, true, prime)
{
}
bool SnowSystemClass::Update(WindClass *wind, const Vector3 &cameraposition)
{
	return WeatherSystemClass::Update(wind, cameraposition);
}
AshSystemClass::AshSystemClass(PhysicsSceneClass *scene, float particledensity, WindClass *wind, bool prime) : WeatherSystemClass (scene, 40.0f, 20.0f, particledensity, 0.1f, 0.32f, 0.32f, 3.0f, Vector2 (0.0f, 0.75f), Vector2 (1.0f, 0.25f), PAGE_COUNT, false, 1.0f, 2.0f, WeatherSystemClass::RENDER_MODE_CAMERA_ALIGNED, true, prime)
{
}
bool AshSystemClass::Update(WindClass *wind, const Vector3 &cameraposition)
{
	return WeatherSystemClass::Update(wind, cameraposition);
}
void WeatherParameterClass::Initialize()
{
	CurrentValue = 0.0f;
	NormalTarget = 0.0f;
	NormalDuration = 0.0f;
	OverrideTarget = 0.0f;
	OverrideDuration = 0.0f;
}
void WeatherParameterClass::Set(float target, float ramptime, bool override)
{
	if (!override)
	{
		NormalTarget = target;
		NormalDuration = ramptime;
	}
	else
	{
		OverrideTarget = target;
		OverrideDuration = ramptime;
	}
}
bool WeatherParameterClass::Update(float time, bool override)
{
	const float previouscurrentvalue = CurrentValue;
	Update(NormalValue, NormalTarget, NormalDuration, time);
	if (override)
	{
		Update(CurrentValue, OverrideTarget, OverrideDuration, time);
	}
	else
	{
		if (OverrideDuration > 0.0f)
		{
			Update(CurrentValue, NormalValue, OverrideDuration, time);
		}
		else
		{
			CurrentValue = NormalValue;
		}
	}
	return CurrentValue != previouscurrentvalue;
}
void WeatherParameterClass::Update(float &value, float &target, float &duration, float time)
{
	if (value == target)
	{
		duration = 0.0f;
	}
	else
	{
		duration -= time;
		if (duration > 0.0f)
		{
			bool sign0, sign1;
			sign0 = value < target;
			value += ((target - value) * (time / duration));
			if (value == target)
			{
				duration = 0.0f;
			}
			else
			{
				sign1 = value < target;
				if (sign0 ^ sign1)
				{
					duration = 0.0f;
					value = target;
				}
			}
		}
		else
		{
			duration = 0.0f;
			value = target;
		}
	}
}
void WeatherMgrClass::Update(PhysicsSceneClass *scene, CameraClass *camera)
{
	float time;
	bool windmodified, fogmodified;
	time = WW3D::Get_Frame_Time() * 0.001f;
	windmodified = _Parameters[PARAMETER_WIND_HEADING].Update(time, _WindOverrideCount > 0);
	windmodified |= _Parameters[PARAMETER_WIND_SPEED].Update(time, _WindOverrideCount > 0);
	windmodified |= _Parameters[PARAMETER_WIND_VARIABILITY].Update(time, _WindOverrideCount > 0);
	if (_Wind != NULL)
	{
		if (windmodified)
		{
			_Wind->Set(_Parameters[PARAMETER_WIND_HEADING].Value(), _Parameters[PARAMETER_WIND_SPEED].Value(), _Parameters[PARAMETER_WIND_VARIABILITY].Value());
		}
		if (!_Wind->Update())
		{
			delete _Wind;
			_Wind = NULL;
		}
	}
	else
	{
		if (_Parameters[PARAMETER_WIND_SPEED].Value() > 0.0f)
		{
			_Wind = new WindClass(_Parameters[PARAMETER_WIND_HEADING].Value(), _Parameters[PARAMETER_WIND_SPEED].Value(), _Parameters[PARAMETER_WIND_VARIABILITY].Value(), _SoundEnvironment);
		}
	}
	for (int p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++)
	{
		WeatherParameterClass *parameterptr = NULL;
		bool						  modified;
		switch (p)
		{
		  	case PRECIPITATION_RAIN:
				parameterptr = &_Parameters[PARAMETER_RAIN_DENSITY];
				break;
		  	case PRECIPITATION_SNOW:
				parameterptr = &_Parameters[PARAMETER_SNOW_DENSITY];
				break;
		  	case PRECIPITATION_ASH:
				parameterptr = &_Parameters[PARAMETER_ASH_DENSITY];
				break;
			default:
				break;
		}
		modified = parameterptr->Update(time, _PrecipitationOverrideCount > 0);
	  	if (_Precipitation[p] != NULL)
		{
	  		if (modified)
			{
	  			_Precipitation[p]->Set_Density(parameterptr->Value());
	  		}
	  		if (!_Precipitation[p]->Update(_Wind, camera->Get_Position()))
			{
				_Precipitation[p]->Remove();
	  			REF_PTR_RELEASE(_Precipitation[p]);
	  		}
	  	}
		else
		{
	  	  	if (parameterptr->Value() > 0.0f)
			{
				if (!CombatManager::I_Am_Only_Server())
				{
					switch (p)
					{
	  					case PRECIPITATION_RAIN:
	  						_Precipitation[p] = new RainSystemClass(scene, parameterptr->Value(), _Wind, _SoundEnvironment, _Prime);
	  						break;
	  					case PRECIPITATION_SNOW:
	  						_Precipitation[p] = new SnowSystemClass(scene, parameterptr->Value(), _Wind, _Prime);
	  						break;
	  					case PRECIPITATION_ASH:
	  						_Precipitation[p] = new AshSystemClass(scene, parameterptr->Value(), _Wind, _Prime);
	  						break;
						default:
							break;
					}
					scene->Add_Render_Object(_Precipitation[p]);
				}
			}
		}
	}
	fogmodified  = _Parameters[PARAMETER_FOG_START_DISTANCE].Update(time, false);
	fogmodified |= _Parameters[PARAMETER_FOG_END_DISTANCE].Update(time, false);
	if (Is_Dirty() || fogmodified)
	{
		scene->Set_Fog_Enable(_FogEnabled);
		scene->Set_Fog_Range(_Parameters[PARAMETER_FOG_START_DISTANCE].Value(), _Parameters[PARAMETER_FOG_END_DISTANCE].Value());
	}
	if (CombatManager::I_Am_Server())
	{
		_Prime = false;
	}
	else
	{
		if (_Imported) _Prime = false;
	}
	Set_Dirty(false);
}
