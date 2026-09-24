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

#include "statusoverlayrenderer.h"
#include "shadermgr.h"

#include "camera.h"
#include "colmath.h"
#include "frustum.h"
#include "linegrp.h"
#include "rinfo.h"
#include "worldparticlemanager.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


/*
**	How many rings may exist at once, and how many short line segments approximate one
**	circle.  256 rings * 12 segments = 3072 staged segments, comfortably inside what a single
**	streak submission already carries for a firefight's worth of tracers -- see
**	TRACER_POOL_MAX next door.  12 segments is enough that a selection ring at ordinary
**	gameplay scale does not read as a polygon.
*/
enum { OVERLAY_POOL_MAX = 256 };
enum { OVERLAY_SEGMENTS_PER_RING = 12 };
enum { OVERLAY_STAGE_MAX = OVERLAY_POOL_MAX * OVERLAY_SEGMENTS_PER_RING };

static bool		_Enabled = true;
static int		_MaxOverlays = OVERLAY_POOL_MAX;
static float	_Clock = 0.0f;
static int		_DroppedCount = 0;
static int		_DrawnSegments = 0;
static int		_HighWater = 0;			// one past the highest slot ever used

static bool						_InUse [OVERLAY_POOL_MAX];
static bool						_Visible [OVERLAY_POOL_MAX];
static StatusOverlayKindType	_Kind [OVERLAY_POOL_MAX];
static Vector3					_Position [OVERLAY_POOL_MAX];
static float					_Radius [OVERLAY_POOL_MAX];
static Vector4					_Color [OVERLAY_POOL_MAX];
static float					_SpawnTime [OVERLAY_POOL_MAX];
static float					_Lifetime [OVERLAY_POOL_MAX];

static Vector3	_BoundsMin (0.0f,0.0f,0.0f);
static Vector3	_BoundsMax (0.0f,0.0f,0.0f);

//	Staging arrays for the streak submission, sized for every ring in the pool at full
//	segment count so every visible ring can be offered in one call.
static Vector3	_StagePosition [OVERLAY_STAGE_MAX];
static Vector3	_StageTailPosition [OVERLAY_STAGE_MAX];
static Vector4	_StageDiffuse [OVERLAY_STAGE_MAX];
static float	_StageSize [OVERLAY_STAGE_MAX];


void StatusOverlayRenderer::Init(void)
{
	Reset();
}

void StatusOverlayRenderer::Shutdown(void)
{
	Reset();
}

void StatusOverlayRenderer::Reset(void)
{
	for (int i = 0; i < OVERLAY_POOL_MAX; i++) {
		_InUse [i] = false;
		_Visible [i] = true;
	}
	_HighWater		= 0;
	_Clock			= 0.0f;
	_DroppedCount	= 0;
	_DrawnSegments	= 0;
	_BoundsMin.Set (0.0f,0.0f,0.0f);
	_BoundsMax.Set (0.0f,0.0f,0.0f);
}

void StatusOverlayRenderer::Set_Enabled(bool onoff)			{ _Enabled = onoff; }
bool StatusOverlayRenderer::Is_Enabled(void)					{ return _Enabled; }

void StatusOverlayRenderer::Set_Max_Overlays(int count)
{
	if (count < 0) count = 0;
	if (count > OVERLAY_POOL_MAX) count = OVERLAY_POOL_MAX;
	_MaxOverlays = count;
}
int StatusOverlayRenderer::Get_Max_Overlays(void)				{ return _MaxOverlays; }


StatusOverlayHandleType StatusOverlayRenderer::Add_Overlay(StatusOverlayKindType kind,const Vector3 & position,
																				float radius,const Vector4 & color,float lifetime)
{
	int slot = -1;
	for (int i = 0; i < _MaxOverlays; i++) {
		if (!_InUse [i]) { slot = i; break; }
	}
	if (slot < 0) {
		_DroppedCount++;
		return 0;
	}

	_InUse [slot]		= true;
	_Visible [slot]	= true;
	_Kind [slot]		= kind;
	_Position [slot]	= position;
	_Radius [slot]		= (radius > 0.0f) ? radius : 0.0f;
	_Color [slot]		= color;
	_SpawnTime [slot]	= _Clock;
	_Lifetime [slot]	= lifetime;

	if (slot >= _HighWater) {
		_HighWater = slot + 1;
	}

	return slot + 1;
}

bool StatusOverlayRenderer::Update_Overlay(StatusOverlayHandleType handle,const Vector3 & position,
														  float radius,const Vector4 & color)
{
	if (handle <= 0 || handle > OVERLAY_POOL_MAX || !_InUse [handle - 1]) {
		return false;
	}
	int slot = handle - 1;
	_Position [slot]	= position;
	_Radius [slot]		= (radius > 0.0f) ? radius : 0.0f;
	_Color [slot]		= color;
	return true;
}

bool StatusOverlayRenderer::Remove_Overlay(StatusOverlayHandleType handle)
{
	if (handle <= 0 || handle > OVERLAY_POOL_MAX || !_InUse [handle - 1]) {
		return false;
	}
	_InUse [handle - 1] = false;
	return true;
}

void StatusOverlayRenderer::Remove_All(void)
{
	for (int i = 0; i < _HighWater; i++) {
		_InUse [i] = false;
	}
}

bool StatusOverlayRenderer::Is_Overlay(StatusOverlayHandleType handle)
{
	if (handle <= 0 || handle > OVERLAY_POOL_MAX) {
		return false;
	}
	return _InUse [handle - 1];
}

StatusOverlayKindType StatusOverlayRenderer::Get_Overlay_Kind(StatusOverlayHandleType handle)
{
	WWASSERT (Is_Overlay (handle));
	return _Kind [handle - 1];
}

Vector3 StatusOverlayRenderer::Get_Overlay_Position(StatusOverlayHandleType handle)
{
	WWASSERT (Is_Overlay (handle));
	return _Position [handle - 1];
}

void StatusOverlayRenderer::Set_Visible(StatusOverlayHandleType handle,bool visible)
{
	if (handle <= 0 || handle > OVERLAY_POOL_MAX || !_InUse [handle - 1]) {
		return;
	}
	_Visible [handle - 1] = visible;
}

bool StatusOverlayRenderer::Is_Visible(StatusOverlayHandleType handle)
{
	if (handle <= 0 || handle > OVERLAY_POOL_MAX || !_InUse [handle - 1]) {
		return false;
	}
	return _Visible [handle - 1];
}


void StatusOverlayRenderer::Age(float seconds)
{
	_Clock += seconds;

	for (int i = 0; i < _HighWater; i++) {
		if (!_InUse [i]) continue;
		if (_Lifetime [i] <= 0.0f) continue;			// persistent, only Remove_Overlay ends it
		if ((_Clock - _SpawnTime [i]) >= _Lifetime [i]) {
			_InUse [i] = false;
		}
	}
}


void StatusOverlayRenderer::Render(RenderInfoClass & rinfo,const Vector3 & camera_position)
{
	_DrawnSegments = 0;

	if (!_Enabled || _HighWater <= 0) {
		return;
	}

	const FrustumClass & frustum = rinfo.Camera.Get_Frustum();

	int staged = 0;
	Vector3 center_sum (0.0f,0.0f,0.0f);
	int ring_count = 0;

	for (int i = 0; i < _HighWater && (staged + OVERLAY_SEGMENTS_PER_RING) <= OVERLAY_STAGE_MAX; i++) {
		if (!_InUse [i] || !_Visible [i]) continue;

		if (CollisionMath::Overlap_Test (frustum,_Position [i]) == CollisionMath::OUTSIDE) continue;

		float radius = _Radius [i];
		if (radius <= 0.0f) continue;

		float alpha_scale = 1.0f;
		if (_Lifetime [i] > 0.0f) {
			float age = _Clock - _SpawnTime [i];
			float remaining = _Lifetime [i] - age;
			if (remaining < 0.0f) remaining = 0.0f;
			alpha_scale = remaining / _Lifetime [i];
		}

		Vector4 diffuse (_Color [i].X,_Color [i].Y,_Color [i].Z,_Color [i].W * alpha_scale);

		for (int s = 0; s < OVERLAY_SEGMENTS_PER_RING; s++) {
			float a0 = (float)s			/ (float)OVERLAY_SEGMENTS_PER_RING * (2.0f * WWMATH_PI);
			float a1 = (float)(s + 1)	/ (float)OVERLAY_SEGMENTS_PER_RING * (2.0f * WWMATH_PI);

			Vector3 p0 = _Position [i] + Vector3 (WWMath::Cos (a0) * radius,WWMath::Sin (a0) * radius,0.0f);
			Vector3 p1 = _Position [i] + Vector3 (WWMath::Cos (a1) * radius,WWMath::Sin (a1) * radius,0.0f);

			_StagePosition [staged]		= p0;
			_StageTailPosition [staged]	= p1;
			_StageDiffuse [staged]			= diffuse;
			_StageSize [staged]				= radius * 0.02f;			// a thin ring, not a filled disc
			staged++;
		}

		center_sum += _Position [i];
		ring_count++;
	}

	if (staged <= 0) return;

	_DrawnSegments = staged;

	Vector3 batch_center = (ring_count > 0) ? (center_sum * (1.0f / (float)ring_count)) : camera_position;

	//	The pipeline layer's status-marker program owns the blend state (D-001 P09).
	ShaderClass shader = ShaderClass::_PresetAlphaShader;
	if (!ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_STATUS_MARKER,0,&shader,nullptr)) {
		shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
		shader.Set_Cull_Mode (ShaderClass::CULL_MODE_DISABLE);
	}
	shader.Enable_Fog ("StatusOverlayRenderer");

	ParticleSubmissionStruct submission;
	submission.Texture			= nullptr;
	submission.Shader				= shader;
	submission.LineMode			= LineGroupClass::PRISM;
	submission.Position			= _StagePosition;
	submission.TailPosition		= _StageTailPosition;
	submission.Diffuse			= _StageDiffuse;
	submission.TailDiffuse		= _StageDiffuse;
	submission.Size				= _StageSize;
	submission.Count				= staged;
	submission.Center				= batch_center;
	submission.Budget				= PARTICLE_BUDGET_AUTO;

	WorldParticleManager::Submit (PARTICLE_POOL_STREAK,submission);
}


void StatusOverlayRenderer::Get_Bounds(Vector3 & minimum,Vector3 & maximum)
{
	minimum = _BoundsMin;
	maximum = _BoundsMax;
}

int StatusOverlayRenderer::Get_Live_Count(void)
{
	int count = 0;
	for (int i = 0; i < _HighWater; i++) {
		if (_InUse [i]) count++;
	}
	return count;
}

int StatusOverlayRenderer::Get_Drawn_Segment_Count(void)	{ return _DrawnSegments; }
int StatusOverlayRenderer::Get_Dropped_Count(void)			{ return _DroppedCount; }
