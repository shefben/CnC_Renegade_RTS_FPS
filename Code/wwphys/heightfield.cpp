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
 *                     $Archive:: /Commando/Code/WWPhys/heightfield.cpp                       $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "heightfield.h"

#include "lineseg.h"
#include "wwdebug.h"
#include "wwmath.h"


HeightfieldClass::HeightfieldClass(void) :
	Heights(nullptr),
	VertexCountX(0),
	VertexCountY(0),
	CellSize(0.0f),
	Origin(0.0f,0.0f,0.0f),
	Bounds(Vector3(0.0f,0.0f,0.0f),Vector3(0.0f,0.0f,0.0f)),
	Patches(nullptr),
	PatchCountX(0),
	PatchCountY(0)
{
}


HeightfieldClass::~HeightfieldClass(void)
{
	Destroy();
}


/***********************************************************************************************
 * HeightfieldClass::Create -- allocate a flat field                                           *
 *=============================================================================================*/
bool HeightfieldClass::Create(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin)
{
	Destroy();

	if ((vertex_count_x < 2) || (vertex_count_y < 2)) {
		return false;
	}
	if (!(cell_size > 0.0f) || !WWMath::Is_Valid_Float(cell_size)) {
		return false;
	}

	VertexCountX = vertex_count_x;
	VertexCountY = vertex_count_y;
	CellSize = cell_size;
	Origin = origin;

	Heights = new float[VertexCountX * VertexCountY];
	for (int i = 0; i < VertexCountX * VertexCountY; i++) {
		Heights[i] = 0.0f;
	}

	//	A patch covers PATCH_CELLS cells, and the last one along each axis is short whenever the
	//	field does not divide evenly.  Rounding up rather than refusing odd sizes matters because
	//	a generated field's size comes from the world it is generating, not from this constant.
	PatchCountX = (Get_Cell_Count_X() + PATCH_CELLS - 1) / PATCH_CELLS;
	PatchCountY = (Get_Cell_Count_Y() + PATCH_CELLS - 1) / PATCH_CELLS;
	Patches = new TerrainPatchClass[PatchCountX * PatchCountY];

	Invalidate_All_Patches();
	for (int py = 0; py < PatchCountY; py++) {
		for (int px = 0; px < PatchCountX; px++) {
			Update_Patch_Extents(px,py);
		}
	}
	Update_Bounds();

	return true;
}


void HeightfieldClass::Destroy(void)
{
	if (Heights != nullptr) {
		delete [] Heights;
		Heights = nullptr;
	}
	if (Patches != nullptr) {
		delete [] Patches;
		Patches = nullptr;
	}
	VertexCountX = 0;
	VertexCountY = 0;
	PatchCountX = 0;
	PatchCountY = 0;
	CellSize = 0.0f;
}


/***********************************************************************************************
 * HeightfieldClass::Get_Height -- a height by grid index, clamped to the field                 *
 *=============================================================================================*/
float HeightfieldClass::Get_Height(int ix,int iy) const
{
	if (Heights == nullptr) {
		return 0.0f;
	}

	if (ix < 0) ix = 0;
	if (iy < 0) iy = 0;
	if (ix > VertexCountX - 1) ix = VertexCountX - 1;
	if (iy > VertexCountY - 1) iy = VertexCountY - 1;

	return Heights[iy * VertexCountX + ix];
}


void HeightfieldClass::Set_Height(int ix,int iy,float height)
{
	if ((Heights == nullptr) || !WWMath::Is_Valid_Float(height)) {
		return;
	}
	if ((ix < 0) || (iy < 0) || (ix >= VertexCountX) || (iy >= VertexCountY)) {
		return;
	}

	Heights[iy * VertexCountX + ix] = height;
	Invalidate_Patches_Touching(ix,iy);
	Update_Bounds();
}


bool HeightfieldClass::Set_Heights(const float * heights,int count)
{
	if ((Heights == nullptr) || (heights == nullptr)) {
		return false;
	}
	if (count != VertexCountX * VertexCountY) {
		return false;
	}

	for (int i = 0; i < count; i++) {
		Heights[i] = WWMath::Is_Valid_Float(heights[i]) ? heights[i] : 0.0f;
	}

	Invalidate_All_Patches();
	for (int py = 0; py < PatchCountY; py++) {
		for (int px = 0; px < PatchCountX; px++) {
			Update_Patch_Extents(px,py);
		}
	}
	Update_Bounds();
	return true;
}


/***********************************************************************************************
 * HeightfieldClass::Modify_Height_Region -- raise or lower a circle of ground                  *
 *                                                                                             *
 * Smoothstep falloff rather than linear.  A linear falloff leaves a crease at the radius that  *
 * shows up as a visible ring in the lighting and as a step in a slope query, and a generator   *
 * stacking hundreds of these would build a landscape out of creases.                           *
 *=============================================================================================*/
void HeightfieldClass::Modify_Height_Region(const Vector3 & center,float radius,float delta)
{
	if ((Heights == nullptr) || !(radius > 0.0f)) {
		return;
	}

	int min_x = (int)WWMath::Floor((center.X - radius - Origin.X) / CellSize);
	int min_y = (int)WWMath::Floor((center.Y - radius - Origin.Y) / CellSize);
	int max_x = (int)WWMath::Ceil((center.X + radius - Origin.X) / CellSize);
	int max_y = (int)WWMath::Ceil((center.Y + radius - Origin.Y) / CellSize);

	if (min_x < 0) min_x = 0;
	if (min_y < 0) min_y = 0;
	if (max_x > VertexCountX - 1) max_x = VertexCountX - 1;
	if (max_y > VertexCountY - 1) max_y = VertexCountY - 1;

	for (int iy = min_y; iy <= max_y; iy++) {
		for (int ix = min_x; ix <= max_x; ix++) {

			float dx = (Origin.X + ix * CellSize) - center.X;
			float dy = (Origin.Y + iy * CellSize) - center.Y;
			float dist = WWMath::Sqrt(dx*dx + dy*dy);
			if (dist >= radius) {
				continue;
			}

			float t = 1.0f - (dist / radius);
			float weight = t * t * (3.0f - 2.0f * t);
			Heights[iy * VertexCountX + ix] += delta * weight;
		}
	}

	Invalidate_Patches_Touching(min_x,min_y);
	Invalidate_Patches_Touching(max_x,max_y);

	int p_min_x = min_x / PATCH_CELLS;
	int p_min_y = min_y / PATCH_CELLS;
	int p_max_x = max_x / PATCH_CELLS;
	int p_max_y = max_y / PATCH_CELLS;
	for (int py = p_min_y; py <= p_max_y; py++) {
		for (int px = p_min_x; px <= p_max_x; px++) {
			Invalidate_Patch(px,py);
			Update_Patch_Extents(px,py);
		}
	}
	Update_Bounds();
}


/***********************************************************************************************
 * HeightfieldClass::Blend_Height_Region -- pull a circle of ground towards a height            *
 *=============================================================================================*/
void HeightfieldClass::Blend_Height_Region(const Vector3 & center,float radius,float target_height,float blend)
{
	if ((Heights == nullptr) || !(radius > 0.0f)) {
		return;
	}
	if (blend < 0.0f) blend = 0.0f;
	if (blend > 1.0f) blend = 1.0f;

	int min_x = (int)WWMath::Floor((center.X - radius - Origin.X) / CellSize);
	int min_y = (int)WWMath::Floor((center.Y - radius - Origin.Y) / CellSize);
	int max_x = (int)WWMath::Ceil((center.X + radius - Origin.X) / CellSize);
	int max_y = (int)WWMath::Ceil((center.Y + radius - Origin.Y) / CellSize);

	if (min_x < 0) min_x = 0;
	if (min_y < 0) min_y = 0;
	if (max_x > VertexCountX - 1) max_x = VertexCountX - 1;
	if (max_y > VertexCountY - 1) max_y = VertexCountY - 1;

	for (int iy = min_y; iy <= max_y; iy++) {
		for (int ix = min_x; ix <= max_x; ix++) {

			float dx = (Origin.X + ix * CellSize) - center.X;
			float dy = (Origin.Y + iy * CellSize) - center.Y;
			float dist = WWMath::Sqrt(dx*dx + dy*dy);
			if (dist >= radius) {
				continue;
			}

			float t = 1.0f - (dist / radius);
			float weight = t * t * (3.0f - 2.0f * t) * blend;

			float & height = Heights[iy * VertexCountX + ix];
			height = height + (target_height - height) * weight;
		}
	}

	int p_min_x = min_x / PATCH_CELLS;
	int p_min_y = min_y / PATCH_CELLS;
	int p_max_x = max_x / PATCH_CELLS;
	int p_max_y = max_y / PATCH_CELLS;
	for (int py = p_min_y; py <= p_max_y; py++) {
		for (int px = p_min_x; px <= p_max_x; px++) {
			Invalidate_Patch(px,py);
			Update_Patch_Extents(px,py);
		}
	}
	Update_Bounds();
}


/***********************************************************************************************
 * HeightfieldClass::Cell_From_World -- which cell a world point is over                        *
 *=============================================================================================*/
bool HeightfieldClass::Cell_From_World(float x,float y,int * cx_out,int * cy_out) const
{
	if (Heights == nullptr) {
		return false;
	}

	float gx = (x - Origin.X) / CellSize;
	float gy = (y - Origin.Y) / CellSize;

	int cx = (int)WWMath::Floor(gx);
	int cy = (int)WWMath::Floor(gy);

	//	The far edge belongs to the last cell rather than to a cell that does not exist.
	if (cx == Get_Cell_Count_X() && (gx - cx) < WWMATH_EPSILON) cx--;
	if (cy == Get_Cell_Count_Y() && (gy - cy) < WWMATH_EPSILON) cy--;

	if ((cx < 0) || (cy < 0) || (cx >= Get_Cell_Count_X()) || (cy >= Get_Cell_Count_Y())) {
		return false;
	}

	*cx_out = cx;
	*cy_out = cy;
	return true;
}


/***********************************************************************************************
 * HeightfieldClass::Get_Cell_Triangles -- the four corners of a cell, in world space           *
 *                                                                                             *
 * The order is the triangulation: verts 0,1,2 are the first triangle and 0,2,3 the second, so  *
 * the diagonal runs from the low corner to the high one.  Everything that turns this field     *
 * into geometry -- a renderer, a collision mesh, a ray -- reads it here, which is how the      *
 * drawn ground and the walked ground stay the same ground.                                     *
 *=============================================================================================*/
Vector3 HeightfieldClass::Compute_Vertex_Normal(int ix,int iy) const
{
	if (Heights == nullptr) {
		return Vector3(0.0f,0.0f,1.0f);
	}

	int ix0 = ix - 1;	if (ix0 < 0) ix0 = 0;
	int ix1 = ix + 1;	if (ix1 > VertexCountX - 1) ix1 = VertexCountX - 1;
	int iy0 = iy - 1;	if (iy0 < 0) iy0 = 0;
	int iy1 = iy + 1;	if (iy1 > VertexCountY - 1) iy1 = VertexCountY - 1;

	//	The span is measured rather than assumed to be two cells, because at the border one of
	//	the neighbours is the vertex itself and a gradient over the wrong distance tilts the
	//	whole edge of the field.
	float dx = (ix1 - ix0) * CellSize;
	float dy = (iy1 - iy0) * CellSize;

	Vector3 normal(0.0f,0.0f,1.0f);
	if (dx > 0.0f) {
		normal.X = (Get_Height(ix0,iy) - Get_Height(ix1,iy)) / dx;
	}
	if (dy > 0.0f) {
		normal.Y = (Get_Height(ix,iy0) - Get_Height(ix,iy1)) / dy;
	}

	normal.Normalize();
	return normal;
}


float HeightfieldClass::Compute_Curvature(int ix,int iy) const
{
	if (Heights == nullptr) {
		return 0.0f;
	}

	//	The discrete Laplacian, negated so that convex ground reads positive.  Get_Height clamps
	//	at the border, which makes the outermost ring read as flat rather than as a cliff -- the
	//	edge of the field is not a feature of the terrain.
	float centre = Get_Height(ix,iy);
	float sum = Get_Height(ix-1,iy) + Get_Height(ix+1,iy)
				 + Get_Height(ix,iy-1) + Get_Height(ix,iy+1);

	return ((4.0f * centre) - sum) / (CellSize * CellSize);
}


bool HeightfieldClass::Get_Cell_Triangles(int cx,int cy,Vector3 * verts_out) const
{
	if ((Heights == nullptr) || (verts_out == nullptr)) {
		return false;
	}
	if ((cx < 0) || (cy < 0) || (cx >= Get_Cell_Count_X()) || (cy >= Get_Cell_Count_Y())) {
		return false;
	}

	float x0 = Origin.X + cx * CellSize;
	float y0 = Origin.Y + cy * CellSize;
	float x1 = x0 + CellSize;
	float y1 = y0 + CellSize;

	verts_out[0].Set(x0,y0,Origin.Z + Get_Height(cx,  cy  ));
	verts_out[1].Set(x1,y0,Origin.Z + Get_Height(cx+1,cy  ));
	verts_out[2].Set(x1,y1,Origin.Z + Get_Height(cx+1,cy+1));
	verts_out[3].Set(x0,y1,Origin.Z + Get_Height(cx,  cy+1));
	return true;
}


/***********************************************************************************************
 * HeightfieldClass::Sample_Cell -- height and normal of the triangle under a point             *
 *=============================================================================================*/
bool HeightfieldClass::Sample_Cell(int cx,int cy,float x,float y,float * height_out,Vector3 * normal_out) const
{
	Vector3 verts[4];
	if (!Get_Cell_Triangles(cx,cy,verts)) {
		return false;
	}

	float u = (x - verts[0].X) / CellSize;
	float v = (y - verts[0].Y) / CellSize;
	if (u < 0.0f) u = 0.0f;
	if (v < 0.0f) v = 0.0f;
	if (u > 1.0f) u = 1.0f;
	if (v > 1.0f) v = 1.0f;

	const Vector3 * a;
	const Vector3 * b;
	const Vector3 * c;

	if (u >= v) {
		//	Below the diagonal: verts 0,1,2.
		a = &verts[0]; b = &verts[1]; c = &verts[2];
		if (height_out != nullptr) {
			*height_out = verts[0].Z + (verts[1].Z - verts[0].Z) * u + (verts[2].Z - verts[1].Z) * v;
		}
	} else {
		//	Above it: verts 0,2,3.
		a = &verts[0]; b = &verts[2]; c = &verts[3];
		if (height_out != nullptr) {
			*height_out = verts[0].Z + (verts[2].Z - verts[3].Z) * u + (verts[3].Z - verts[0].Z) * v;
		}
	}

	if (normal_out != nullptr) {
		Vector3 normal;
		Vector3::Cross_Product(*b - *a,*c - *a,&normal);
		normal.Normalize();
		*normal_out = normal;
	}

	return true;
}


bool HeightfieldClass::Sample_Height(float x,float y,float * height_out) const
{
	int cx,cy;
	if (!Cell_From_World(x,y,&cx,&cy)) {
		return false;
	}
	return Sample_Cell(cx,cy,x,y,height_out,nullptr);
}


bool HeightfieldClass::Sample_Normal(float x,float y,Vector3 * normal_out) const
{
	int cx,cy;
	if (!Cell_From_World(x,y,&cx,&cy)) {
		return false;
	}
	return Sample_Cell(cx,cy,x,y,nullptr,normal_out);
}


bool HeightfieldClass::Sample_Slope(float x,float y,float * slope_radians_out) const
{
	Vector3 normal;
	if (!Sample_Normal(x,y,&normal)) {
		return false;
	}

	if (slope_radians_out != nullptr) {
		float cosine = normal.Z;
		if (cosine < -1.0f) cosine = -1.0f;
		if (cosine > 1.0f) cosine = 1.0f;
		*slope_radians_out = WWMath::Acos(cosine);
	}
	return true;
}


/***********************************************************************************************
 * Ray against one triangle -- Moller-Trumbore, parameterised on the segment                    *
 *=============================================================================================*/
static bool Ray_Triangle
(
	const Vector3 & origin,
	const Vector3 & dir,
	const Vector3 & a,
	const Vector3 & b,
	const Vector3 & c,
	float * fraction_out
)
{
	Vector3 edge1 = b - a;
	Vector3 edge2 = c - a;

	Vector3 pvec;
	Vector3::Cross_Product(dir,edge2,&pvec);
	float det = Vector3::Dot_Product(edge1,pvec);

	if (WWMath::Fabs(det) < WWMATH_EPSILON) {
		return false;			// parallel to the triangle
	}

	float inv_det = 1.0f / det;
	Vector3 tvec = origin - a;

	float u = Vector3::Dot_Product(tvec,pvec) * inv_det;
	if ((u < 0.0f) || (u > 1.0f)) {
		return false;
	}

	Vector3 qvec;
	Vector3::Cross_Product(tvec,edge1,&qvec);
	float v = Vector3::Dot_Product(dir,qvec) * inv_det;
	if ((v < 0.0f) || (u + v > 1.0f)) {
		return false;
	}

	float t = Vector3::Dot_Product(edge2,qvec) * inv_det;
	if ((t < 0.0f) || (t > 1.0f)) {
		return false;
	}

	*fraction_out = t;
	return true;
}


/***********************************************************************************************
 * HeightfieldClass::Cast_Ray -- walk the cells a segment crosses, nearest hit wins             *
 *                                                                                             *
 * A grid walk rather than a test against every triangle.  The cells are visited in the order   *
 * the segment reaches them, so the first triangle hit is the nearest one and the walk stops    *
 * there; a level-sized field is thousands of cells and a bullet crosses a handful of them.     *
 *=============================================================================================*/
bool HeightfieldClass::Cast_Ray(const LineSegClass & ray,float * fraction_out,Vector3 * normal_out) const
{
	if (Heights == nullptr) {
		return false;
	}

	const Vector3 & p0 = ray.Get_P0();
	const Vector3 & dp = ray.Get_DP();

	//	Clip the segment against the field's footprint first, so that a ray starting a mile away
	//	begins its walk at the edge of the terrain rather than a mile of empty cells earlier.
	float t_enter = 0.0f;
	float t_exit = 1.0f;

	float min_x = Origin.X;
	float min_y = Origin.Y;
	float max_x = Origin.X + Get_Cell_Count_X() * CellSize;
	float max_y = Origin.Y + Get_Cell_Count_Y() * CellSize;

	for (int axis = 0; axis < 2; axis++) {

		float start = (axis == 0) ? p0.X : p0.Y;
		float delta = (axis == 0) ? dp.X : dp.Y;
		float lo = (axis == 0) ? min_x : min_y;
		float hi = (axis == 0) ? max_x : max_y;

		if (WWMath::Fabs(delta) < WWMATH_EPSILON) {
			if ((start < lo) || (start > hi)) {
				return false;
			}
			continue;
		}

		float t1 = (lo - start) / delta;
		float t2 = (hi - start) / delta;
		if (t1 > t2) {
			float swap = t1; t1 = t2; t2 = swap;
		}
		if (t1 > t_enter) t_enter = t1;
		if (t2 < t_exit) t_exit = t2;
		if (t_enter > t_exit) {
			return false;
		}
	}

	Vector3 entry = p0 + t_enter * dp;

	int cx = (int)WWMath::Floor((entry.X - Origin.X) / CellSize);
	int cy = (int)WWMath::Floor((entry.Y - Origin.Y) / CellSize);
	if (cx < 0) cx = 0;
	if (cy < 0) cy = 0;
	if (cx > Get_Cell_Count_X() - 1) cx = Get_Cell_Count_X() - 1;
	if (cy > Get_Cell_Count_Y() - 1) cy = Get_Cell_Count_Y() - 1;

	int step_x = (dp.X > 0.0f) ? 1 : ((dp.X < 0.0f) ? -1 : 0);
	int step_y = (dp.Y > 0.0f) ? 1 : ((dp.Y < 0.0f) ? -1 : 0);

	//	Parameter distance from the entry point to the next cell boundary on each axis, and the
	//	parameter distance between boundaries.  A zero step means the segment never leaves its
	//	row or column, so that axis never comes up again.
	float t_max_x = 1.0f;
	float t_delta_x = 1.0f;
	if (step_x != 0) {
		float next_x = Origin.X + (cx + ((step_x > 0) ? 1 : 0)) * CellSize;
		t_max_x = (next_x - p0.X) / dp.X;
		t_delta_x = WWMath::Fabs(CellSize / dp.X);
	}

	float t_max_y = 1.0f;
	float t_delta_y = 1.0f;
	if (step_y != 0) {
		float next_y = Origin.Y + (cy + ((step_y > 0) ? 1 : 0)) * CellSize;
		t_max_y = (next_y - p0.Y) / dp.Y;
		t_delta_y = WWMath::Fabs(CellSize / dp.Y);
	}

	int max_steps = Get_Cell_Count_X() + Get_Cell_Count_Y() + 2;

	for (int step = 0; step < max_steps; step++) {

		Vector3 verts[4];
		if (Get_Cell_Triangles(cx,cy,verts)) {

			float best = 2.0f;
			int best_tri = -1;

			float hit;
			if (Ray_Triangle(p0,dp,verts[0],verts[1],verts[2],&hit) && (hit < best)) {
				best = hit;
				best_tri = 0;
			}
			if (Ray_Triangle(p0,dp,verts[0],verts[2],verts[3],&hit) && (hit < best)) {
				best = hit;
				best_tri = 1;
			}

			if (best_tri >= 0) {
				if (fraction_out != nullptr) {
					*fraction_out = best;
				}
				if (normal_out != nullptr) {
					const Vector3 & a = verts[0];
					const Vector3 & b = (best_tri == 0) ? verts[1] : verts[2];
					const Vector3 & c = (best_tri == 0) ? verts[2] : verts[3];
					Vector3 normal;
					Vector3::Cross_Product(b - a,c - a,&normal);
					normal.Normalize();
					*normal_out = normal;
				}
				return true;
			}
		}

		//	Step into the next cell the segment reaches.
		if ((step_x == 0) && (step_y == 0)) {
			break;
		}

		if ((step_y == 0) || ((step_x != 0) && (t_max_x < t_max_y))) {
			if (t_max_x > t_exit) break;
			cx += step_x;
			t_max_x += t_delta_x;
		} else {
			if (t_max_y > t_exit) break;
			cy += step_y;
			t_max_y += t_delta_y;
		}

		if ((cx < 0) || (cy < 0) || (cx >= Get_Cell_Count_X()) || (cy >= Get_Cell_Count_Y())) {
			break;
		}
	}

	return false;
}


/***********************************************************************************************
 * Patches                                                                                     *
 *=============================================================================================*/
bool HeightfieldClass::Get_Patch(int px,int py,TerrainPatchClass * patch_out) const
{
	if ((Patches == nullptr) || (patch_out == nullptr)) {
		return false;
	}
	if ((px < 0) || (py < 0) || (px >= PatchCountX) || (py >= PatchCountY)) {
		return false;
	}

	*patch_out = Patches[py * PatchCountX + px];
	return true;
}


bool HeightfieldClass::Get_Patch_Bounds(int px,int py,AABoxClass * bounds_out) const
{
	if ((Patches == nullptr) || (bounds_out == nullptr)) {
		return false;
	}
	if ((px < 0) || (py < 0) || (px >= PatchCountX) || (py >= PatchCountY)) {
		return false;
	}

	int cx0 = px * PATCH_CELLS;
	int cy0 = py * PATCH_CELLS;
	int cx1 = cx0 + PATCH_CELLS;
	int cy1 = cy0 + PATCH_CELLS;
	if (cx1 > Get_Cell_Count_X()) cx1 = Get_Cell_Count_X();
	if (cy1 > Get_Cell_Count_Y()) cy1 = Get_Cell_Count_Y();

	const TerrainPatchClass & patch = Patches[py * PatchCountX + px];

	Vector3 min_corner(Origin.X + cx0 * CellSize,Origin.Y + cy0 * CellSize,Origin.Z + patch.MinHeight);
	Vector3 max_corner(Origin.X + cx1 * CellSize,Origin.Y + cy1 * CellSize,Origin.Z + patch.MaxHeight);
	bounds_out->Init_Min_Max(min_corner,max_corner);
	return true;
}


void HeightfieldClass::Invalidate_Patch(int px,int py)
{
	if (Patches == nullptr) {
		return;
	}
	if ((px < 0) || (py < 0) || (px >= PatchCountX) || (py >= PatchCountY)) {
		return;
	}
	Patches[py * PatchCountX + px].Dirty = true;
}


void HeightfieldClass::Invalidate_All_Patches(void)
{
	if (Patches == nullptr) {
		return;
	}
	for (int i = 0; i < PatchCountX * PatchCountY; i++) {
		Patches[i].Dirty = true;
	}
}


void HeightfieldClass::Validate_Patch(int px,int py)
{
	if (Patches == nullptr) {
		return;
	}
	if ((px < 0) || (py < 0) || (px >= PatchCountX) || (py >= PatchCountY)) {
		return;
	}
	Patches[py * PatchCountX + px].Dirty = false;
}


bool HeightfieldClass::Is_Patch_Dirty(int px,int py) const
{
	if (Patches == nullptr) {
		return false;
	}
	if ((px < 0) || (py < 0) || (px >= PatchCountX) || (py >= PatchCountY)) {
		return false;
	}
	return Patches[py * PatchCountX + px].Dirty;
}


void HeightfieldClass::Invalidate_Patches_Touching(int ix,int iy)
{
	//	A vertex on a patch seam belongs to the patches on both sides of it, and moving it moves
	//	the edge of each.  Invalidating only the patch the index divides into would leave a
	//	visible crack along the seam and a collision mesh that disagrees with the one next door.
	int px_lo = (ix - 1) / PATCH_CELLS;
	int py_lo = (iy - 1) / PATCH_CELLS;
	int px_hi = ix / PATCH_CELLS;
	int py_hi = iy / PATCH_CELLS;

	if (px_lo < 0) px_lo = 0;
	if (py_lo < 0) py_lo = 0;

	for (int py = py_lo; py <= py_hi; py++) {
		for (int px = px_lo; px <= px_hi; px++) {
			Invalidate_Patch(px,py);
			Update_Patch_Extents(px,py);
		}
	}
}


void HeightfieldClass::Update_Patch_Extents(int px,int py)
{
	if (Patches == nullptr) {
		return;
	}
	if ((px < 0) || (py < 0) || (px >= PatchCountX) || (py >= PatchCountY)) {
		return;
	}

	int ix0 = px * PATCH_CELLS;
	int iy0 = py * PATCH_CELLS;
	int ix1 = ix0 + PATCH_CELLS;
	int iy1 = iy0 + PATCH_CELLS;
	if (ix1 > VertexCountX - 1) ix1 = VertexCountX - 1;
	if (iy1 > VertexCountY - 1) iy1 = VertexCountY - 1;

	float min_height = Get_Height(ix0,iy0);
	float max_height = min_height;

	for (int iy = iy0; iy <= iy1; iy++) {
		for (int ix = ix0; ix <= ix1; ix++) {
			float height = Get_Height(ix,iy);
			if (height < min_height) min_height = height;
			if (height > max_height) max_height = height;
		}
	}

	TerrainPatchClass & patch = Patches[py * PatchCountX + px];
	patch.MinHeight = min_height;
	patch.MaxHeight = max_height;
}


void HeightfieldClass::Update_Bounds(void)
{
	if ((Heights == nullptr) || (Patches == nullptr)) {
		Bounds.Init_Min_Max(Vector3(0.0f,0.0f,0.0f),Vector3(0.0f,0.0f,0.0f));
		return;
	}

	float min_height = Patches[0].MinHeight;
	float max_height = Patches[0].MaxHeight;
	for (int i = 1; i < PatchCountX * PatchCountY; i++) {
		if (Patches[i].MinHeight < min_height) min_height = Patches[i].MinHeight;
		if (Patches[i].MaxHeight > max_height) max_height = Patches[i].MaxHeight;
	}

	Vector3 min_corner(Origin.X,
							 Origin.Y,
							 Origin.Z + min_height);
	Vector3 max_corner(Origin.X + Get_Cell_Count_X() * CellSize,
							 Origin.Y + Get_Cell_Count_Y() * CellSize,
							 Origin.Z + max_height);
	Bounds.Init_Min_Max(min_corner,max_corner);
}
