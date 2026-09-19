#include "debrisrenderer.h"

#include "camera.h"
#include "colmath.h"
#include "frustum.h"
#include "pointgr.h"
#include "rinfo.h"
#include "worldparticlemanager.h"
#include "wwdebug.h"

#include <math.h>
#include <stdlib.h>

enum { DEBRIS_POOL_MAX = 384 };

//	Matches the magnitude of wwphys's PhysicsConstants::GravityAcceleration.Z.  Duplicated
//	rather than shared because this layer may not depend on wwphys -- the same reason
//	GAMEPLAY_DEBRIS, which does need real gravity, lives in Code/Combat instead.
static const float DEBRIS_GRAVITY = 9.8f;
static const float TWO_PI = 6.28318530718f;

static bool		_Enabled = true;
static int		_MaxDebris = 192;
static float	_Clock = 0.0f;
static int		_DroppedCount = 0;
static int		_DrawnParticles = 0;

static int		_LiveCount = 0;
static Vector3	_Position [DEBRIS_POOL_MAX];
static Vector3	_Velocity [DEBRIS_POOL_MAX];
static float	_Spin [DEBRIS_POOL_MAX];
static float	_SpinRate [DEBRIS_POOL_MAX];
static Vector4	_Color [DEBRIS_POOL_MAX];
static float	_Size [DEBRIS_POOL_MAX];
static float	_SpawnTime [DEBRIS_POOL_MAX];
static float	_Lifetime [DEBRIS_POOL_MAX];

static Vector3	_BoundsMin (0.0f,0.0f,0.0f);
static Vector3	_BoundsMax (0.0f,0.0f,0.0f);

static Vector3			_StagePosition [DEBRIS_POOL_MAX];
static Vector4			_StageDiffuse [DEBRIS_POOL_MAX];
static float			_StageSize [DEBRIS_POOL_MAX];
static unsigned char	_StageOrientation [DEBRIS_POOL_MAX];


//	A cheap [lo,hi] draw.  Debris scatter is pure decoration -- no client needs to agree with
//	any other client or with the server about where a chip lands -- so plain rand() is enough.
static float _Random_Range(float lo,float hi)
{
	float t = (float)::rand() / (float)RAND_MAX;
	return (lo + t * (hi - lo));
}

static unsigned char _Spin_To_Orientation(float radians)
{
	float wrapped = ::fmodf (radians,TWO_PI);
	if (wrapped < 0.0f) wrapped += TWO_PI;
	return (unsigned char)((wrapped / TWO_PI) * 256.0f);
}

void DebrisRenderer::Init(void)
{
	Reset();
}

void DebrisRenderer::Shutdown(void)
{
	Reset();
}

void DebrisRenderer::Reset(void)
{
	_LiveCount = 0;
	_Clock = 0.0f;
	_DroppedCount = 0;
	_DrawnParticles = 0;
	_BoundsMin.Set (0.0f,0.0f,0.0f);
	_BoundsMax.Set (0.0f,0.0f,0.0f);
}

void DebrisRenderer::Set_Enabled(bool onoff)
{
	_Enabled = onoff;

	if (!_Enabled) {
		_LiveCount = 0;
	}
}

bool DebrisRenderer::Is_Enabled(void)
{
	return (_Enabled);
}

void DebrisRenderer::Set_Max_Debris(int count)
{
	if (count < 0) count = 0;
	if (count > DEBRIS_POOL_MAX) count = DEBRIS_POOL_MAX;
	_MaxDebris = count;
}

int DebrisRenderer::Get_Max_Debris(void)
{
	return (_MaxDebris);
}

void DebrisRenderer::Spawn(const Vector3 & center,float radius,int count,
									const Vector4 & color,float lifetime)
{
	if (!_Enabled) return;
	if (lifetime <= 0.0f) return;
	if (radius < 0.0f) radius = 0.0f;

	for (int n = 0; n < count; n++) {

		if (_LiveCount >= _MaxDebris) {
			_DroppedCount++;
			continue;
		}

		int i = _LiveCount++;

		//	A random direction and a random fraction of the radius reads as an even burst; a
		//	point uniform inside the sphere would cluster too many chips near the centre.
		Vector3 offset (_Random_Range(-1.0f,1.0f),_Random_Range(-1.0f,1.0f),_Random_Range(0.0f,1.0f));
		if (offset.Length() > 0.0001f) offset.Normalize();
		float distance = _Random_Range (0.0f,radius);

		_Position [i] = center + offset * distance;
		_Velocity [i].Set (_Random_Range(-3.0f,3.0f),_Random_Range(-3.0f,3.0f),_Random_Range(2.0f,6.0f));
		_Spin [i] = _Random_Range (0.0f,TWO_PI);
		_SpinRate [i] = _Random_Range (-6.0f,6.0f);
		_Color [i] = color;
		_Size [i] = _Random_Range (0.2f,0.55f);
		_SpawnTime [i] = _Clock;
		_Lifetime [i] = lifetime;
	}
}

void DebrisRenderer::Age(float seconds)
{
	if (seconds < 0.0f) seconds = 0.0f;
	_Clock += seconds;

	int i = 0;
	while (i < _LiveCount) {
		float age = _Clock - _SpawnTime [i];
		if (age >= _Lifetime [i]) {
			int last = _LiveCount - 1;
			_Position [i] = _Position [last];
			_Velocity [i] = _Velocity [last];
			_Spin [i] = _Spin [last];
			_SpinRate [i] = _SpinRate [last];
			_Color [i] = _Color [last];
			_Size [i] = _Size [last];
			_SpawnTime [i] = _SpawnTime [last];
			_Lifetime [i] = _Lifetime [last];
			_LiveCount--;
		} else {
			_Velocity [i].Z -= DEBRIS_GRAVITY * seconds;
			_Position [i] += _Velocity [i] * seconds;
			_Spin [i] += _SpinRate [i] * seconds;
			i++;
		}
	}

	if (_LiveCount <= 0) {
		_BoundsMin.Set (0.0f,0.0f,0.0f);
		_BoundsMax.Set (0.0f,0.0f,0.0f);
		return;
	}

	Vector3 minimum = _Position [0];
	Vector3 maximum = _Position [0];

	for (i = 0; i < _LiveCount; i++) {
		float half = _Size [i] * 0.5f;
		const Vector3 pad (half,half,half);

		minimum.Update_Min (_Position [i] - pad);
		maximum.Update_Max (_Position [i] + pad);
	}

	_BoundsMin = minimum;
	_BoundsMax = maximum;
}

void DebrisRenderer::Render(RenderInfoClass & rinfo,const Vector3 & camera_position)
{
	_DrawnParticles = 0;

	if (!_Enabled || _LiveCount <= 0) {
		return;
	}

	const FrustumClass & frustum = rinfo.Camera.Get_Frustum();

	int staged = 0;
	Vector3 center_sum (0.0f,0.0f,0.0f);

	for (int i = 0; i < _LiveCount; i++) {

		if (CollisionMath::Overlap_Test (frustum,_Position [i]) == CollisionMath::OUTSIDE) continue;

		float age = _Clock - _SpawnTime [i];
		float remaining = _Lifetime [i] - age;
		if (remaining < 0.0f) remaining = 0.0f;
		float alpha_scale = (_Lifetime [i] > 0.0f) ? (remaining / _Lifetime [i]) : 0.0f;

		_StagePosition [staged] = _Position [i];
		_StageDiffuse [staged].Set (_Color [i].X,_Color [i].Y,_Color [i].Z,_Color [i].W * alpha_scale);
		_StageSize [staged] = _Size [i];
		_StageOrientation [staged] = _Spin_To_Orientation (_Spin [i]);

		center_sum += _Position [i];
		staged++;
	}

	if (staged <= 0) return;

	_DrawnParticles = staged;

	Vector3 batch_center = center_sum * (1.0f / (float)staged);

	ShaderClass shader = ShaderClass::_PresetAlphaShader;
	shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
	shader.Set_Cull_Mode (ShaderClass::CULL_MODE_DISABLE);
	shader.Enable_Fog ("DebrisRenderer");

	ParticleSubmissionStruct submission;
	submission.Texture = nullptr;
	submission.Shader = shader;
	submission.PointMode = PointGroupClass::QUADS;
	submission.Position = _StagePosition;
	submission.Diffuse = _StageDiffuse;
	submission.Size = _StageSize;
	submission.Orientation = _StageOrientation;
	submission.Count = staged;

	submission.Center = batch_center;
	submission.Budget = PARTICLE_BUDGET_AUTO;

	WorldParticleManager::Submit (PARTICLE_POOL_SPRITE,submission);
}

void DebrisRenderer::Get_Bounds(Vector3 & minimum,Vector3 & maximum)
{
	minimum = _BoundsMin;
	maximum = _BoundsMax;
}

int DebrisRenderer::Get_Live_Count(void)
{
	return (_LiveCount);
}

int DebrisRenderer::Get_Drawn_Particles(void)
{
	return (_DrawnParticles);
}

int DebrisRenderer::Get_Dropped_Count(void)
{
	return (_DroppedCount);
}
