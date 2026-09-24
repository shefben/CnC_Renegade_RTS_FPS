/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tracerrenderer.h"
#include "shadermgr.h"

#include "camera.h"
#include "colmath.h"
#include "frustum.h"
#include "linegrp.h"
#include "rinfo.h"
#include "worldparticlemanager.h"
#include "wwdebug.h"

#include <string.h>


/*
**	Five hundred and twelve live tracers is far beyond anything a firefight produces --
**	stock tracer lifetimes are a fraction of a second, so the pool would have to be fed
**	several thousand shots a second to ever fill.  It exists as a hard ceiling, not a
**	tuning target; Set_Max_Tracers governs the soft one underneath it.
*/
enum { TRACER_POOL_MAX = 512 };

static bool		_Enabled = true;
static int		_MaxTracers = 256;
static float	_Clock = 0.0f;
static int		_DroppedCount = 0;
static int		_DrawnParticles = 0;

static int		_LiveCount = 0;
static Vector3	_Start [TRACER_POOL_MAX];
static Vector3	_End [TRACER_POOL_MAX];
static Vector4	_Color [TRACER_POOL_MAX];
static float	_Width [TRACER_POOL_MAX];
static float	_SpawnTime [TRACER_POOL_MAX];
static float	_Lifetime [TRACER_POOL_MAX];

static Vector3	_BoundsMin (0.0f,0.0f,0.0f);
static Vector3	_BoundsMax (0.0f,0.0f,0.0f);

//	Staging arrays for the streak submission, sized the same as the pool so every live
//	tracer can be offered in one call.
static Vector3	_StagePosition [TRACER_POOL_MAX];
static Vector3	_StageTailPosition [TRACER_POOL_MAX];
static Vector4	_StageDiffuse [TRACER_POOL_MAX];
static float	_StageSize [TRACER_POOL_MAX];


void TracerRenderer::Init(void)
{
	Reset();
}


void TracerRenderer::Shutdown(void)
{
	Reset();
}


void TracerRenderer::Reset(void)
{
	_LiveCount = 0;
	_Clock = 0.0f;
	_DroppedCount = 0;
	_DrawnParticles = 0;
	_BoundsMin.Set (0.0f,0.0f,0.0f);
	_BoundsMax.Set (0.0f,0.0f,0.0f);
}


void TracerRenderer::Set_Enabled(bool onoff)
{
	_Enabled = onoff;

	//	There is no per-tracer object left to fall back to when this is off, so off means
	//	gone, not merely undrawn.
	if (!_Enabled) {
		_LiveCount = 0;
	}
}


bool TracerRenderer::Is_Enabled(void)
{
	return (_Enabled);
}


void TracerRenderer::Set_Max_Tracers(int count)
{
	if (count < 0) count = 0;
	if (count > TRACER_POOL_MAX) count = TRACER_POOL_MAX;
	_MaxTracers = count;
}


int TracerRenderer::Get_Max_Tracers(void)
{
	return (_MaxTracers);
}


void TracerRenderer::Add_Tracer(const Vector3 & start,const Vector3 & end,float width,
											const Vector4 & color,float lifetime)
{
	if (!_Enabled) return;
	if (lifetime <= 0.0f) return;

	if (_LiveCount >= _MaxTracers) {
		//	Same direction of failure the weather layer chose for a column that could not be
		//	cast this frame: the newest arrival is the one left out, not one already on screen.
		_DroppedCount++;
		return;
	}

	int i = _LiveCount++;
	_Start [i] = start;
	_End [i] = end;
	_Color [i] = color;
	_Width [i] = width;
	_SpawnTime [i] = _Clock;
	_Lifetime [i] = lifetime;
}


/*
**	Advance the clock, reap whatever just expired, and refresh the bounding volume from what
**	is left.  This is the half of the object that has to run every frame regardless of
**	visibility, for the same reason WeatherEnvironmentRenderer separates it from Render(): a
**	bounding volume the scene culls by has to stay true while the object is off screen, or the
**	next time it comes into view its own culling test is stale.
*/
void TracerRenderer::Age(float seconds)
{
	if (seconds < 0.0f) seconds = 0.0f;
	_Clock += seconds;

	int i = 0;
	while (i < _LiveCount) {
		float age = _Clock - _SpawnTime [i];
		if (age >= _Lifetime [i]) {
			int last = _LiveCount - 1;
			_Start [i] = _Start [last];
			_End [i] = _End [last];
			_Color [i] = _Color [last];
			_Width [i] = _Width [last];
			_SpawnTime [i] = _SpawnTime [last];
			_Lifetime [i] = _Lifetime [last];
			_LiveCount--;
		} else {
			i++;
		}
	}

	if (_LiveCount <= 0) {
		_BoundsMin.Set (0.0f,0.0f,0.0f);
		_BoundsMax.Set (0.0f,0.0f,0.0f);
		return;
	}

	Vector3 minimum = _Start [0];
	Vector3 maximum = _Start [0];

	for (i = 0; i < _LiveCount; i++) {
		float halfwidth = _Width [i] * 0.5f;
		const Vector3 pad (halfwidth,halfwidth,halfwidth);

		minimum.Update_Min (_Start [i] - pad);
		maximum.Update_Max (_Start [i] + pad);
		minimum.Update_Min (_End [i] - pad);
		maximum.Update_Max (_End [i] + pad);
	}

	_BoundsMin = minimum;
	_BoundsMax = maximum;
}


void TracerRenderer::Render(RenderInfoClass & rinfo,const Vector3 & camera_position)
{
	_DrawnParticles = 0;

	if (!_Enabled || _LiveCount <= 0) {
		return;
	}

	const FrustumClass & frustum = rinfo.Camera.Get_Frustum();

	int staged = 0;
	Vector3 center_sum (0.0f,0.0f,0.0f);

	for (int i = 0; i < _LiveCount; i++) {
		Vector3 midpoint = (_Start [i] + _End [i]) * 0.5f;

		//	Per-tracer frustum rejection, at the midpoint -- the same coarseness the weather
		//	layer accepts for a single particle, and a tracer is one short segment, not a
		//	volume worth a real intersection test.
		if (CollisionMath::Overlap_Test (frustum,midpoint) == CollisionMath::OUTSIDE) continue;

		float age = _Clock - _SpawnTime [i];
		float remaining = _Lifetime [i] - age;
		if (remaining < 0.0f) remaining = 0.0f;
		float alpha_scale = (_Lifetime [i] > 0.0f) ? (remaining / _Lifetime [i]) : 0.0f;

		_StagePosition [staged] = _Start [i];
		_StageTailPosition [staged] = _End [i];
		_StageDiffuse [staged].Set (_Color [i].X,_Color [i].Y,_Color [i].Z,_Color [i].W * alpha_scale);
		_StageSize [staged] = _Width [i] * 0.5f;

		center_sum += midpoint;
		staged++;
	}

	if (staged <= 0) return;

	_DrawnParticles = staged;

	Vector3 batch_center = center_sum * (1.0f / (float)staged);

	//	The pipeline layer's tracer program owns the blend state (D-001 P09); the built-in
	//	value below is only what a manager with nothing registered would leave undrawn.
	ShaderClass shader = ShaderClass::_PresetAlphaShader;
	if (!ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TRACER,0,&shader,nullptr)) {
		shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
		shader.Set_Cull_Mode (ShaderClass::CULL_MODE_DISABLE);
	}
	shader.Enable_Fog ("TracerRenderer");

	ParticleSubmissionStruct submission;
	submission.Texture = nullptr;
	submission.Shader = shader;
	submission.LineMode = LineGroupClass::PRISM;
	submission.Position = _StagePosition;
	submission.TailPosition = _StageTailPosition;
	submission.Diffuse = _StageDiffuse;
	submission.TailDiffuse = _StageDiffuse;
	submission.Size = _StageSize;
	submission.Count = staged;

	//	One submission covers every tracer alive this frame, near and far alike, so the
	//	budget resolution it gets is one distance for all of them: the centre of what was
	//	actually staged.  This is the same emitter-granularity every pooled submission
	//	accepts -- one world position per call, not one per particle.
	submission.Center = batch_center;
	submission.Budget = PARTICLE_BUDGET_AUTO;

	WorldParticleManager::Submit (PARTICLE_POOL_STREAK,submission);
}


void TracerRenderer::Get_Bounds(Vector3 & minimum,Vector3 & maximum)
{
	minimum = _BoundsMin;
	maximum = _BoundsMax;
}


int TracerRenderer::Get_Live_Count(void)
{
	return (_LiveCount);
}


int TracerRenderer::Get_Drawn_Particles(void)
{
	return (_DrawnParticles);
}


int TracerRenderer::Get_Dropped_Count(void)
{
	return (_DroppedCount);
}
