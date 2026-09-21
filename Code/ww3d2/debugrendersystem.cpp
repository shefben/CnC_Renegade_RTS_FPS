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

#include "debugrendersystem.h"

#include "camera.h"
#include "colmath.h"
#include "frustum.h"
#include "linegrp.h"
#include "rinfo.h"
#include "worldparticlemanager.h"
#include "wwmath.h"

#include <string.h>


static bool	_LayerEnabled [DEBUG_LAYER_COUNT];

static int		_LineCount			= 0;
static int		_DroppedCount		= 0;
static int		_DrawnSegments		= 0;

static Vector3	_LineStart [DEBUG_LINE_POOL_MAX];
static Vector3	_LineEnd [DEBUG_LINE_POOL_MAX];
static Vector4	_LineColor [DEBUG_LINE_POOL_MAX];

static const char * const	_LayerNames [DEBUG_LAYER_COUNT] =
{
	"terrain_chunks",
	"terrain_materials",
	"road_splines",
	"road_triangles",
	"bridge_sections",
	"water_volumes",
	"foliage_cells",
	"spatial_index_cells",
	"dynamic_light_radii",
	"shadow_projectors",
	"collision_bounds",
	"building_semantic_nodes",
	"building_footprints",
	"placement_overlap",
	"purchase_terminal_anchors",
	"mct_anchors",
	"commander_cursor_ray",
	"shroud_cells",
	"radar_contacts",
	"world_bounds",
	"boundary_cliffs",
	"interior_cliffs",
	"hydrology",
};


void DebugRenderSystem::Init(void)
{
	for (int i = 0; i < DEBUG_LAYER_COUNT; i++) {
		_LayerEnabled [i] = false;
	}
	_LineCount = 0;
	_DroppedCount = 0;
	_DrawnSegments = 0;
}

void DebugRenderSystem::Shutdown(void)
{
	Init();
}

void DebugRenderSystem::Set_Layer_Enabled(DebugLayerType layer,bool onoff)
{
	if (layer < 0 || layer >= DEBUG_LAYER_COUNT) return;
	_LayerEnabled [layer] = onoff;
}

bool DebugRenderSystem::Is_Layer_Enabled(DebugLayerType layer)
{
	if (layer < 0 || layer >= DEBUG_LAYER_COUNT) return false;
	return _LayerEnabled [layer];
}

void DebugRenderSystem::Disable_All_Layers(void)
{
	for (int i = 0; i < DEBUG_LAYER_COUNT; i++) {
		_LayerEnabled [i] = false;
	}
}

const char * DebugRenderSystem::Get_Layer_Name(DebugLayerType layer)
{
	if (layer < 0 || layer >= DEBUG_LAYER_COUNT) return "unknown";
	return _LayerNames [layer];
}


void DebugRenderSystem::Begin_Frame(void)
{
	_LineCount = 0;
	_DroppedCount = 0;
}


void DebugRenderSystem::Add_Line(const Vector3 & p0,const Vector3 & p1,const Vector4 & color)
{
	if (_LineCount >= DEBUG_LINE_POOL_MAX) {
		_DroppedCount++;
		return;
	}
	_LineStart [_LineCount] = p0;
	_LineEnd [_LineCount]	= p1;
	_LineColor [_LineCount]	= color;
	_LineCount++;
}

void DebugRenderSystem::Add_Box(const Vector3 & minimum,const Vector3 & maximum,const Vector4 & color)
{
	Vector3 corner [8] =
	{
		Vector3 (minimum.X,minimum.Y,minimum.Z), Vector3 (maximum.X,minimum.Y,minimum.Z),
		Vector3 (maximum.X,maximum.Y,minimum.Z), Vector3 (minimum.X,maximum.Y,minimum.Z),
		Vector3 (minimum.X,minimum.Y,maximum.Z), Vector3 (maximum.X,minimum.Y,maximum.Z),
		Vector3 (maximum.X,maximum.Y,maximum.Z), Vector3 (minimum.X,maximum.Y,maximum.Z),
	};

	//	Bottom face, top face, then the four verticals -- twelve edges.
	Add_Line (corner [0],corner [1],color);	Add_Line (corner [1],corner [2],color);
	Add_Line (corner [2],corner [3],color);	Add_Line (corner [3],corner [0],color);
	Add_Line (corner [4],corner [5],color);	Add_Line (corner [5],corner [6],color);
	Add_Line (corner [6],corner [7],color);	Add_Line (corner [7],corner [4],color);
	Add_Line (corner [0],corner [4],color);	Add_Line (corner [1],corner [5],color);
	Add_Line (corner [2],corner [6],color);	Add_Line (corner [3],corner [7],color);
}

void DebugRenderSystem::Add_Sphere(const Vector3 & center,float radius,const Vector4 & color)
{
	if (radius <= 0.0f) return;

	enum { SPHERE_RING_SEGMENTS = 16 };

	//	Three great-circle rings (XY, XZ, YZ) is the usual cheap wire-sphere approximation --
	//	enough to judge a radius by eye without a full sphere mesh.
	for (int ring = 0; ring < 3; ring++) {
		for (int s = 0; s < SPHERE_RING_SEGMENTS; s++) {
			float a0 = (float)s			/ (float)SPHERE_RING_SEGMENTS * (2.0f * WWMATH_PI);
			float a1 = (float)(s + 1)	/ (float)SPHERE_RING_SEGMENTS * (2.0f * WWMATH_PI);

			Vector3 p0,p1;
			switch (ring) {
				case 0:
					p0.Set (center.X + WWMath::Cos (a0) * radius,center.Y + WWMath::Sin (a0) * radius,center.Z);
					p1.Set (center.X + WWMath::Cos (a1) * radius,center.Y + WWMath::Sin (a1) * radius,center.Z);
					break;
				case 1:
					p0.Set (center.X + WWMath::Cos (a0) * radius,center.Y,center.Z + WWMath::Sin (a0) * radius);
					p1.Set (center.X + WWMath::Cos (a1) * radius,center.Y,center.Z + WWMath::Sin (a1) * radius);
					break;
				default:
					p0.Set (center.X,center.Y + WWMath::Cos (a0) * radius,center.Z + WWMath::Sin (a0) * radius);
					p1.Set (center.X,center.Y + WWMath::Cos (a1) * radius,center.Z + WWMath::Sin (a1) * radius);
					break;
			}
			Add_Line (p0,p1,color);
		}
	}
}

void DebugRenderSystem::Add_Cross(const Vector3 & point,float size,const Vector4 & color)
{
	Add_Line (point - Vector3 (size,0.0f,0.0f),point + Vector3 (size,0.0f,0.0f),color);
	Add_Line (point - Vector3 (0.0f,size,0.0f),point + Vector3 (0.0f,size,0.0f),color);
	Add_Line (point - Vector3 (0.0f,0.0f,size),point + Vector3 (0.0f,0.0f,size),color);
}


void DebugRenderSystem::Render(RenderInfoClass & rinfo,const Vector3 & camera_position)
{
	_DrawnSegments = 0;

	if (_LineCount <= 0) {
		return;
	}

	const FrustumClass & frustum = rinfo.Camera.Get_Frustum();

	static Vector3	stage_position [DEBUG_LINE_POOL_MAX];
	static Vector3	stage_tail [DEBUG_LINE_POOL_MAX];
	static Vector4	stage_diffuse [DEBUG_LINE_POOL_MAX];
	static float	stage_size [DEBUG_LINE_POOL_MAX];

	int staged = 0;
	Vector3 center_sum (0.0f,0.0f,0.0f);

	for (int i = 0; i < _LineCount; i++) {
		Vector3 midpoint = (_LineStart [i] + _LineEnd [i]) * 0.5f;
		if (CollisionMath::Overlap_Test (frustum,midpoint) == CollisionMath::OUTSIDE) continue;

		stage_position [staged]	= _LineStart [i];
		stage_tail [staged]		= _LineEnd [i];
		stage_diffuse [staged]	= _LineColor [i];
		stage_size [staged]		= 0.02f;

		center_sum += midpoint;
		staged++;
	}

	if (staged <= 0) return;

	_DrawnSegments = staged;
	Vector3 batch_center = center_sum * (1.0f / (float)staged);

	ShaderClass shader = ShaderClass::_PresetAlphaShader;
	shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
	shader.Set_Cull_Mode (ShaderClass::CULL_MODE_DISABLE);
	shader.Enable_Fog ("DebugRenderSystem");

	ParticleSubmissionStruct submission;
	submission.Texture			= nullptr;
	submission.Shader				= shader;
	submission.LineMode			= LineGroupClass::PRISM;
	submission.Position			= stage_position;
	submission.TailPosition		= stage_tail;
	submission.Diffuse			= stage_diffuse;
	submission.TailDiffuse		= stage_diffuse;
	submission.Size				= stage_size;
	submission.Count				= staged;
	submission.Center				= batch_center;
	submission.Budget				= PARTICLE_BUDGET_AUTO;

	WorldParticleManager::Submit (PARTICLE_POOL_STREAK,submission);
}


int DebugRenderSystem::Get_Line_Count(void)			{ return _LineCount; }
int DebugRenderSystem::Get_Dropped_Count(void)		{ return _DroppedCount; }
int DebugRenderSystem::Get_Drawn_Segment_Count(void)	{ return _DrawnSegments; }
