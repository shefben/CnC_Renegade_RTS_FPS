/*
**	OpenW3D
**	Copyright 2025 Electronic Arts Inc.
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

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/worldterrainsystem.cpp                $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldterrainsystem.h"

#include "colmath.h"
#include "frustum.h"
#include "lineseg.h"
#include "wwdebug.h"
#include "wwmath.h"


HeightfieldClass *	WorldTerrainSystem::Heightfield				= nullptr;
bool						WorldTerrainSystem::ReportedNoCollision		= false;
bool						WorldTerrainSystem::ReportedNoFarTerrain	= false;


void WorldTerrainSystem::Init(void)
{
	//	Nothing to build.  A level with no heightfield is the normal case and must cost nothing,
	//	so the field is allocated by Create_Terrain and not before.
	Destroy_Terrain();
	ReportedNoCollision = false;
	ReportedNoFarTerrain = false;
}


void WorldTerrainSystem::Shutdown(void)
{
	Destroy_Terrain();
}


bool WorldTerrainSystem::Create_Terrain(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin)
{
	Destroy_Terrain();

	Heightfield = new HeightfieldClass;
	if (!Heightfield->Create(vertex_count_x,vertex_count_y,cell_size,origin)) {
		Destroy_Terrain();
		return false;
	}

	WWDEBUG_SAY(("WorldTerrainSystem: terrain %dx%d vertices, %.2f cell\r\n",
					 vertex_count_x,vertex_count_y,cell_size));
	return true;
}


bool WorldTerrainSystem::Set_Heights(const float * heights,int count)
{
	if (Heightfield == nullptr) {
		return false;
	}
	return Heightfield->Set_Heights(heights,count);
}


void WorldTerrainSystem::Destroy_Terrain(void)
{
	if (Heightfield != nullptr) {
		delete Heightfield;
		Heightfield = nullptr;
	}
}


bool WorldTerrainSystem::Has_Terrain(void)
{
	return (Heightfield != nullptr) && Heightfield->Is_Valid();
}


HeightfieldClass * WorldTerrainSystem::Peek_Heightfield(void)
{
	return Heightfield;
}


/***********************************************************************************************
 * Queries                                                                                     *
 *=============================================================================================*/
bool WorldTerrainSystem::Sample_Height(float x,float y,float * height_out)
{
	if (!Has_Terrain()) {
		return false;
	}
	return Heightfield->Sample_Height(x,y,height_out);
}


bool WorldTerrainSystem::Sample_Normal(float x,float y,Vector3 * normal_out)
{
	if (!Has_Terrain()) {
		return false;
	}
	return Heightfield->Sample_Normal(x,y,normal_out);
}


bool WorldTerrainSystem::Sample_Slope(float x,float y,float * slope_radians_out)
{
	if (!Has_Terrain()) {
		return false;
	}
	return Heightfield->Sample_Slope(x,y,slope_radians_out);
}


bool WorldTerrainSystem::Ray_Intersect_Terrain(const LineSegClass & ray,float * fraction_out,Vector3 * normal_out)
{
	if (!Has_Terrain()) {
		return false;
	}
	return Heightfield->Cast_Ray(ray,fraction_out,normal_out);
}


/***********************************************************************************************
 * WorldTerrainSystem::Get_Material -- what the ground is made of here                          *
 *                                                                                             *
 * One answer until Section 18.  The question is asked in its final form now so that the        *
 * callers written between here and there do not have to be rewritten when the masks arrive.    *
 *=============================================================================================*/
bool WorldTerrainSystem::Get_Material(float x,float y,TerrainMaterialType * material_out)
{
	float height;
	if (!Sample_Height(x,y,&height)) {
		return false;
	}

	if (material_out != nullptr) {
		*material_out = TERRAIN_MATERIAL_DEFAULT;
	}
	return true;
}


bool WorldTerrainSystem::Get_Bounds(AABoxClass * bounds_out)
{
	if (!Has_Terrain() || (bounds_out == nullptr)) {
		return false;
	}
	*bounds_out = Heightfield->Get_Bounds();
	return true;
}


/***********************************************************************************************
 * Patches                                                                                     *
 *=============================================================================================*/
bool WorldTerrainSystem::Get_Patch(int px,int py,TerrainPatchClass * patch_out)
{
	if (!Has_Terrain()) {
		return false;
	}
	return Heightfield->Get_Patch(px,py,patch_out);
}


bool WorldTerrainSystem::Get_Patch_Bounds(int px,int py,AABoxClass * bounds_out)
{
	if (!Has_Terrain()) {
		return false;
	}
	return Heightfield->Get_Patch_Bounds(px,py,bounds_out);
}


void WorldTerrainSystem::Invalidate_Patch(int px,int py)
{
	if (!Has_Terrain()) {
		return;
	}
	Heightfield->Invalidate_Patch(px,py);
}


int WorldTerrainSystem::Get_Patch_Count_X(void)
{
	return Has_Terrain() ? Heightfield->Get_Patch_Count_X() : 0;
}


int WorldTerrainSystem::Get_Patch_Count_Y(void)
{
	return Has_Terrain() ? Heightfield->Get_Patch_Count_Y() : 0;
}


/***********************************************************************************************
 * Shaping                                                                                     *
 *=============================================================================================*/
void WorldTerrainSystem::Modify_Height_Region(const Vector3 & center,float radius,float delta)
{
	if (!Has_Terrain()) {
		return;
	}
	Heightfield->Modify_Height_Region(center,radius,delta);
}


/***********************************************************************************************
 * WorldTerrainSystem::Apply_Road_Grade -- flatten the ground under a road                      *
 *                                                                                             *
 * A road is a polyline with a height of its own, and the ground has to come to meet it or the  *
 * road floats and clips.  The polyline is walked at half-width steps -- close enough that the  *
 * discs overlap, so the graded strip is continuous rather than a row of dishes -- and each     *
 * step pulls the ground towards the road's height there.                                       *
 *                                                                                             *
 * Blend rather than set: a road that stamps its height leaves a wall where it crosses a slope. *
 *=============================================================================================*/
void WorldTerrainSystem::Apply_Road_Grade(const Vector3 * points,int count,float width,float blend)
{
	if (!Has_Terrain() || (points == nullptr) || (count < 2) || !(width > 0.0f)) {
		return;
	}

	float radius = width * 0.5f;
	float step = radius;
	if (!(step > 0.0f)) {
		return;
	}

	for (int i = 0; i < count - 1; i++) {

		const Vector3 & a = points[i];
		const Vector3 & b = points[i+1];

		Vector3 span = b - a;
		float length = span.Length();
		if (length < WWMATH_EPSILON) {
			continue;
		}

		int steps = (int)WWMath::Ceil(length / step);
		if (steps < 1) steps = 1;

		for (int s = 0; s <= steps; s++) {
			float t = (float)s / (float)steps;
			Vector3 at = a + t * span;
			Heightfield->Blend_Height_Region(at,radius,at.Z - Heightfield->Get_Origin().Z,blend);
		}
	}
}


/***********************************************************************************************
 * WorldTerrainSystem::Apply_River_Cut -- carve a channel along a polyline                      *
 *=============================================================================================*/
void WorldTerrainSystem::Apply_River_Cut(const Vector3 * points,int count,float width,float depth)
{
	if (!Has_Terrain() || (points == nullptr) || (count < 2) || !(width > 0.0f)) {
		return;
	}

	float radius = width * 0.5f;
	float step = radius;

	for (int i = 0; i < count - 1; i++) {

		const Vector3 & a = points[i];
		const Vector3 & b = points[i+1];

		Vector3 span = b - a;
		float length = span.Length();
		if (length < WWMATH_EPSILON) {
			continue;
		}

		int steps = (int)WWMath::Ceil(length / step);
		if (steps < 1) steps = 1;

		for (int s = 0; s <= steps; s++) {
			float t = (float)s / (float)steps;
			Vector3 at = a + t * span;

			//	Level the channel to the water line first, then cut down from it.  Cutting alone
			//	would carry the terrain's own bumps down with it and leave a river bed that is
			//	deeper in some places than the water is.
			Heightfield->Blend_Height_Region(at,radius,at.Z - Heightfield->Get_Origin().Z,1.0f);
			Heightfield->Modify_Height_Region(at,radius,-depth);
		}
	}
}


/***********************************************************************************************
 * WorldTerrainSystem::Query_Placement_Surface -- is this footprint standable                   *
 *                                                                                             *
 * Nine samples: the centre, the four axes and the four diagonals at the radius.  A footprint   *
 * measured only at its middle is how a building ends up with one corner in the air, and a      *
 * full sweep of every vertex under it costs more than a placement preview can spend while it   *
 * follows the cursor.                                                                          *
 *=============================================================================================*/
bool WorldTerrainSystem::Query_Placement_Surface(const Vector3 & center,float radius,TerrainPlacementInfoClass * info_out)
{
	if (!Has_Terrain() || (info_out == nullptr)) {
		return false;
	}

	static const float OFFSETS[9][2] = {
		{  0.0f,  0.0f },
		{  1.0f,  0.0f }, { -1.0f,  0.0f }, {  0.0f,  1.0f }, {  0.0f, -1.0f },
		{  0.707f,  0.707f }, { -0.707f,  0.707f }, {  0.707f, -0.707f }, { -0.707f, -0.707f },
	};

	TerrainPlacementInfoClass info;
	Vector3 normal_sum(0.0f,0.0f,0.0f);
	float height_sum = 0.0f;
	int samples = 0;

	for (int i = 0; i < 9; i++) {

		float x = center.X + OFFSETS[i][0] * radius;
		float y = center.Y + OFFSETS[i][1] * radius;

		float height;
		Vector3 normal;
		if (!Heightfield->Sample_Height(x,y,&height) || !Heightfield->Sample_Normal(x,y,&normal)) {
			//	Any sample off the field means the footprint is not entirely on terrain, which is
			//	an answer rather than a failure: the caller is told, and told about the rest too.
			info.IsOnTerrain = false;
			continue;
		}

		if (samples == 0) {
			info.MinHeight = height;
			info.MaxHeight = height;
			info.IsOnTerrain = true;
		} else {
			if (height < info.MinHeight) info.MinHeight = height;
			if (height > info.MaxHeight) info.MaxHeight = height;
		}

		float cosine = normal.Z;
		if (cosine < -1.0f) cosine = -1.0f;
		if (cosine > 1.0f) cosine = 1.0f;
		float slope = WWMath::Acos(cosine);
		if (slope > info.MaxSlope) {
			info.MaxSlope = slope;
		}

		height_sum += height;
		normal_sum += normal;
		samples++;
	}

	if (samples == 0) {
		*info_out = info;
		return false;
	}

	info.AverageHeight = height_sum / (float)samples;
	normal_sum.Normalize();
	info.AverageNormal = normal_sum;

	*info_out = info;
	return true;
}


/***********************************************************************************************
 * WorldTerrainSystem::Select_Render_LOD -- how much of a patch is worth drawing                *
 *=============================================================================================*/
int WorldTerrainSystem::Select_Render_LOD(int px,int py,const Vector3 & viewer_position)
{
	AABoxClass bounds;
	if (!Get_Patch_Bounds(px,py,&bounds)) {
		return LOD_FAR;
	}

	//	Distance in patch widths, so the thresholds mean the same thing whatever the cell size
	//	is: a field of half-metre cells and a field of five-metre cells should not need
	//	different constants to look the same.
	float patch_width = Heightfield->Get_Cell_Size() * (float)HeightfieldClass::PATCH_CELLS;
	if (!(patch_width > 0.0f)) {
		return LOD_FAR;
	}

	Vector3 to_viewer = viewer_position - bounds.Center;
	float distance = to_viewer.Length() / patch_width;

	if (distance < 4.0f) {
		return LOD_NEAR;
	}
	if (distance < 12.0f) {
		return LOD_MEDIUM;
	}
	return LOD_FAR;
}


/***********************************************************************************************
 * WorldTerrainSystem::Get_Visible_Terrain_Patches -- the patches a frustum can reach           *
 *                                                                                             *
 * Indices rather than pointers, because a patch is a position in the field and not an object   *
 * with a lifetime.  The index is py * patch_count_x + px, the same order the field stores.     *
 *=============================================================================================*/
void WorldTerrainSystem::Get_Visible_Terrain_Patches(const FrustumClass & frustum,DynamicVectorClass<int> & patch_indices)
{
	if (!Has_Terrain()) {
		return;
	}

	int count_x = Heightfield->Get_Patch_Count_X();
	int count_y = Heightfield->Get_Patch_Count_Y();

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			AABoxClass bounds;
			if (!Heightfield->Get_Patch_Bounds(px,py,&bounds)) {
				continue;
			}

			if (CollisionMath::Overlap_Test(frustum,bounds) != CollisionMath::OUTSIDE) {
				patch_indices.Add(py * count_x + px);
			}
		}
	}
}


/***********************************************************************************************
 * Not yet: collision meshes and the far layer                                                  *
 *=============================================================================================*/
bool WorldTerrainSystem::Build_Collision(void)
{
	if (!ReportedNoCollision) {
		ReportedNoCollision = true;
		WWDEBUG_SAY(("WorldTerrainSystem: terrain collision is not built yet; "
						 "heightfield queries answer, the physics scene does not see the ground.\r\n"));
	}
	return false;
}


bool WorldTerrainSystem::Build_Far_Terrain_Representation(void)
{
	if (!ReportedNoFarTerrain) {
		ReportedNoFarTerrain = true;
		WWDEBUG_SAY(("WorldTerrainSystem: far terrain arrives with its own phase (roadmap Section 34).\r\n"));
	}
	return false;
}


void WorldTerrainSystem::Invalidate_Far_Terrain_Region(const AABoxClass & region)
{
	//	Nothing to invalidate until there is a far representation to hold it.  The entry point
	//	exists so that the systems that will change the ground -- roads, water, the Commander's
	//	construction -- can be written to tell it now.
	(void)region;
}
