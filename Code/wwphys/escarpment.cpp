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
 *                     $Archive:: /Commando/Code/wwphys/escarpment.cpp                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   EscarpmentClass::Sample -- the benches at the nearest station                             *
 *   EscarpmentClass::Does_Segment_Cross -- 2D crossing plus a height                          *
 *   EscarpmentSystem::Conform_To_Terrain -- read the benches out of the heightfield           *
 *   EscarpmentSystem::Build_Escarpment_Geometry -- the face strip, and its collision          *
 *   EscarpmentSystem::Is_Segment_Blocked -- what a route check asks                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "escarpment.h"

#include "aabox.h"
#include "coltype.h"
#include "dynamicmeshbuilder.h"
#include "matrix3d.h"
#include "pscene.h"
#include "rendobj.h"
#include "shader.h"
#include "shadermgr.h"
#include "staticphys.h"
#include "terrainmaterial.h"
#include "terraintexturesystem.h"
#include "texture.h"
#include "vertmaterial.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


/*
**	A cliff face is a wall.  It stops what a wall stops, which is the same set a bridge pier
**	stops -- there is no case where you want to walk through the middle of a cliff.
*/
static const int ESCARPMENT_COLLISION_BITS =
	COLLISION_TYPE_PHYSICAL | COLLISION_TYPE_PROJECTILE |
	COLLISION_TYPE_VEHICLE | COLLISION_TYPE_CAMERA;

/*
**	Faces shorter than this are not worth building: a step of a few centimetres is what the
**	heightfield is for, and a strip of degenerate quads down a map is a lot of draw calls for
**	something nobody can see.
*/
static const float MIN_FACE_HEIGHT = 0.25f;

/*
**	How far the face is pushed out from the polyline, away from the high side.  The heightfield
**	behind a cliff is a run of near-vertical cells, and two near-coincident surfaces fight.  A
**	few centimetres of separation is invisible and settles it.
*/
static const float FACE_OFFSET = 0.05f;

/*
**	Fallback tiling when no cliff layer names a scale.  Ten metres is TerrainTextureSystem's
**	own default, so a face and the ground beside it tile at the same rate by default.
*/
static const float DEFAULT_METERS_PER_TILE = 10.0f;


DynamicVectorClass<EscarpmentClass>	EscarpmentSystem::Escarpments;
DynamicVectorClass<Vector3>			EscarpmentSystem::PortalCutCenters;
DynamicVectorClass<float>				EscarpmentSystem::PortalCutRadii;
bool	EscarpmentSystem::GeometryBuilt	= false;
int	EscarpmentSystem::UntexturedFaces	= 0;

static EscarpmentPointClass	_EmptyPoint;


/***********************************************************************************************
 * EscarpmentClass                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EscarpmentClass::EscarpmentClass(void) :
	ID(-1),
	Batter(0.0f),
	LayerIndex(-1),
	FaceRowCount(1),
	ToeWidth(0.0f)
{
}


EscarpmentClass::EscarpmentClass(const EscarpmentClass & that) :
	ID(-1),
	Batter(0.0f),
	LayerIndex(-1),
	FaceRowCount(1),
	ToeWidth(0.0f)
{
	*this = that;
}


EscarpmentClass::~EscarpmentClass(void)
{
	//	Faces are owned by the system, which removes them from the scene before it drops the
	//	escarpment.  A copy made into a vector must not take a second run at releasing them.
	Faces.Delete_All();
}


EscarpmentClass & EscarpmentClass::operator = (const EscarpmentClass & that)
{
	if (this == &that) {
		return *this;
	}

	ID				= that.ID;
	Name			= that.Name;
	Batter		= that.Batter;
	LayerIndex	= that.LayerIndex;
	FaceRowCount= that.FaceRowCount;
	ToeWidth		= that.ToeWidth;

	Points.Delete_All();
	for (int i = 0; i < that.Points.Count(); i++) {
		Points.Add(that.Points[i]);
	}

	//	Geometry is not copied.  It belongs to the placed escarpment, not to the value.
	Faces.Delete_All();

	return *this;
}


bool EscarpmentClass::operator == (const EscarpmentClass & that) const
{
	if ((ID != that.ID) || (Batter != that.Batter) || (LayerIndex != that.LayerIndex) ||
		 (FaceRowCount != that.FaceRowCount) || (ToeWidth != that.ToeWidth)) {
		return false;
	}
	if (Points.Count() != that.Points.Count()) {
		return false;
	}
	for (int i = 0; i < Points.Count(); i++) {
		if (Points[i] != that.Points[i]) {
			return false;
		}
	}
	return true;
}


void EscarpmentClass::Reset(void)
{
	ID = -1;
	Name = "";
	Batter = 0.0f;
	LayerIndex = -1;
	FaceRowCount = 1;
	ToeWidth = 0.0f;
	Points.Delete_All();
	Faces.Delete_All();
}


void EscarpmentClass::Set_Name(const char * name)
{
	Name = (name != nullptr) ? name : "";
}


void EscarpmentClass::Add_Point(const EscarpmentPointClass & point)
{
	Points.Add(point);
}


void EscarpmentClass::Add_Point(const Vector3 & position,float upper,float lower)
{
	EscarpmentPointClass point;
	point.Position = position;
	point.UpperHeight = upper;
	point.LowerHeight = lower;
	Points.Add(point);
}


const EscarpmentPointClass & EscarpmentClass::Get_Point(int index) const
{
	if ((index < 0) || (index >= Points.Count())) {
		return _EmptyPoint;
	}
	return Points[index];
}


EscarpmentPointClass * EscarpmentClass::Peek_Point(int index)
{
	if ((index < 0) || (index >= Points.Count())) {
		return nullptr;
	}
	return &Points[index];
}


void EscarpmentClass::Clear_Points(void)
{
	Points.Delete_All();
}


float EscarpmentClass::Get_Length(void) const
{
	float length = 0.0f;
	for (int i = 1; i < Points.Count(); i++) {
		Vector3 delta = Points[i].Position - Points[i - 1].Position;
		delta.Z = 0.0f;
		length += delta.Length();
	}
	return length;
}


float EscarpmentClass::Get_Max_Face_Height(void) const
{
	float tallest = 0.0f;
	for (int i = 0; i < Points.Count(); i++) {
		float h = Points[i].Get_Face_Height();
		if (h > tallest) {
			tallest = h;
		}
	}
	return tallest;
}


bool EscarpmentClass::Get_Bounds(AABoxClass * bounds_out) const
{
	if ((bounds_out == nullptr) || (Points.Count() < 1)) {
		return false;
	}

	Vector3 min_corner(Points[0].Position.X,Points[0].Position.Y,Points[0].LowerHeight);
	Vector3 max_corner(Points[0].Position.X,Points[0].Position.Y,Points[0].UpperHeight);

	for (int i = 1; i < Points.Count(); i++) {
		const EscarpmentPointClass & p = Points[i];
		if (p.Position.X < min_corner.X) { min_corner.X = p.Position.X; }
		if (p.Position.Y < min_corner.Y) { min_corner.Y = p.Position.Y; }
		if (p.LowerHeight < min_corner.Z) { min_corner.Z = p.LowerHeight; }
		if (p.Position.X > max_corner.X) { max_corner.X = p.Position.X; }
		if (p.Position.Y > max_corner.Y) { max_corner.Y = p.Position.Y; }
		if (p.UpperHeight > max_corner.Z) { max_corner.Z = p.UpperHeight; }
	}

	bounds_out->Center = (min_corner + max_corner) * 0.5f;
	bounds_out->Extent = (max_corner - min_corner) * 0.5f;
	return true;
}


/***********************************************************************************************
 * EscarpmentClass::Sample -- the benches at the nearest station                               *
 *                                                                                             *
 * Nearest point on the polyline in the XY plane, with the two heights interpolated along the  *
 * segment it landed on.  Distance is unsigned, because a caller asking "is there a cliff near *
 * here" does not care which side it is standing on; one asking which side uses the crossing   *
 * test instead.                                                                               *
 *=============================================================================================*/
bool EscarpmentClass::Sample(float x,float y,float * upper_out,float * lower_out,float * distance_out) const
{
	if (Points.Count() < 2) {
		return false;
	}

	float best_distance_sq = 0.0f;
	float best_upper = 0.0f;
	float best_lower = 0.0f;
	bool found = false;

	for (int i = 1; i < Points.Count(); i++) {

		const EscarpmentPointClass & p0 = Points[i - 1];
		const EscarpmentPointClass & p1 = Points[i];

		float ex = p1.Position.X - p0.Position.X;
		float ey = p1.Position.Y - p0.Position.Y;
		float len_sq = (ex * ex) + (ey * ey);
		if (len_sq < WWMATH_EPSILON) {
			continue;
		}

		float t = (((x - p0.Position.X) * ex) + ((y - p0.Position.Y) * ey)) / len_sq;
		if (t < 0.0f) { t = 0.0f; }
		if (t > 1.0f) { t = 1.0f; }

		float px = p0.Position.X + (ex * t);
		float py = p0.Position.Y + (ey * t);
		float dx = x - px;
		float dy = y - py;
		float distance_sq = (dx * dx) + (dy * dy);

		if (!found || (distance_sq < best_distance_sq)) {
			best_distance_sq = distance_sq;
			best_upper = p0.UpperHeight + ((p1.UpperHeight - p0.UpperHeight) * t);
			best_lower = p0.LowerHeight + ((p1.LowerHeight - p0.LowerHeight) * t);
			found = true;
		}
	}

	if (!found) {
		return false;
	}

	if (upper_out != nullptr)		{ *upper_out = best_upper; }
	if (lower_out != nullptr)		{ *lower_out = best_lower; }
	if (distance_out != nullptr)	{ *distance_out = WWMath::Sqrt(best_distance_sq); }
	return true;
}


/***********************************************************************************************
 * EscarpmentClass::Does_Segment_Cross -- 2D crossing plus a height                            *
 *                                                                                             *
 * Deliberately a segment-segment test in plan, not a trace against the face mesh.  The caller *
 * is a coarse connectivity graph deciding whether an edge exists; it wants to know a wall is  *
 * in the way and how tall it is, and a mesh trace would cost far more to answer no better.    *
 *                                                                                             *
 * Reports the tallest face crossed, since an edge is blocked by the worst thing it meets.     *
 *=============================================================================================*/
bool EscarpmentClass::Does_Segment_Cross(const Vector3 & a,const Vector3 & b,float * face_height_out) const
{
	if (Points.Count() < 2) {
		return false;
	}

	float ax = a.X, ay = a.Y;
	float bx = b.X, by = b.Y;
	float rx = bx - ax;
	float ry = by - ay;

	bool crossed = false;
	float tallest = 0.0f;

	for (int i = 1; i < Points.Count(); i++) {

		const EscarpmentPointClass & p0 = Points[i - 1];
		const EscarpmentPointClass & p1 = Points[i];

		float sx = p1.Position.X - p0.Position.X;
		float sy = p1.Position.Y - p0.Position.Y;

		float denom = (rx * sy) - (ry * sx);
		if (WWMath::Fabs(denom) < WWMATH_EPSILON) {
			//	Parallel.  A segment running along a cliff edge without crossing it is not
			//	blocked by it, which is the answer a route check wants.
			continue;
		}

		float qpx = p0.Position.X - ax;
		float qpy = p0.Position.Y - ay;

		float t = ((qpx * sy) - (qpy * sx)) / denom;
		float u = ((qpx * ry) - (qpy * rx)) / denom;

		if ((t < 0.0f) || (t > 1.0f) || (u < 0.0f) || (u > 1.0f)) {
			continue;
		}

		float height = p0.Get_Face_Height() + ((p1.Get_Face_Height() - p0.Get_Face_Height()) * u);
		if (height > tallest) {
			tallest = height;
		}
		crossed = true;
	}

	if (crossed && (face_height_out != nullptr)) {
		*face_height_out = tallest;
	}
	return crossed;
}


/***********************************************************************************************
 * EscarpmentSystem                                                                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void EscarpmentSystem::Init(void)
{
	Escarpments.Delete_All();
	PortalCutCenters.Delete_All();
	PortalCutRadii.Delete_All();
	GeometryBuilt = false;
	UntexturedFaces = 0;
}


void EscarpmentSystem::Shutdown(void)
{
	Destroy_Geometry();
	Escarpments.Delete_All();
	PortalCutCenters.Delete_All();
	PortalCutRadii.Delete_All();
}


void EscarpmentSystem::Add_Portal_Cut(const Vector3 & center,float radius)
{
	if (radius <= 0.0f) {
		return;
	}
	PortalCutCenters.Add(Vector3(center.X,center.Y,0.0f));
	PortalCutRadii.Add(radius);
}


void EscarpmentSystem::Clear_Portal_Cuts(void)
{
	PortalCutCenters.Delete_All();
	PortalCutRadii.Delete_All();
}


int EscarpmentSystem::Add_Escarpment(const EscarpmentClass & escarpment)
{
	EscarpmentClass copy = escarpment;
	copy.Set_ID(Escarpments.Count());
	Escarpments.Add(copy);
	return copy.Get_ID();
}


EscarpmentClass * EscarpmentSystem::Peek_Escarpment(int id)
{
	if ((id < 0) || (id >= Escarpments.Count())) {
		return nullptr;
	}
	return &Escarpments[id];
}


int EscarpmentSystem::Get_Escarpment_Count(void)
{
	return Escarpments.Count();
}


void EscarpmentSystem::Clear_Escarpments(void)
{
	Destroy_Geometry();
	Escarpments.Delete_All();
}


/***********************************************************************************************
 * EscarpmentSystem::Conform_To_Terrain -- read the benches out of the heightfield             *
 *                                                                                             *
 * This is the rule the file header states, made mechanical: the heightfield is the authority  *
 * for ground height on both benches, so the escarpment asks it rather than being told.  The   *
 * ground is sampled one inset either side of the polyline, perpendicular to it, and the       *
 * higher sample is the top bench.                                                             *
 *                                                                                             *
 * A point the terrain cannot answer for -- off the field, or no terrain loaded at all -- is   *
 * left exactly as the caller set it, so a hand-authored cliff is not quietly flattened by a   *
 * conform pass run on a map that does not reach it.                                           *
 *=============================================================================================*/
int EscarpmentSystem::Conform_To_Terrain(float inset)
{
	if (inset <= 0.0f) {
		inset = 1.0f;
	}

	int conformed = 0;

	for (int e = 0; e < Escarpments.Count(); e++) {

		EscarpmentClass & escarpment = Escarpments[e];
		int count = escarpment.Points.Count();
		if (count < 2) {
			continue;
		}

		for (int i = 0; i < count; i++) {

			//	Direction of travel at this station: the segment ahead, the one behind at the
			//	last point, so the ends are perpendicular to the same line the face follows.
			Vector3 dir;
			if (i + 1 < count) {
				dir = escarpment.Points[i + 1].Position - escarpment.Points[i].Position;
			} else {
				dir = escarpment.Points[i].Position - escarpment.Points[i - 1].Position;
			}
			dir.Z = 0.0f;
			if (dir.Length() < WWMATH_EPSILON) {
				continue;
			}
			dir.Normalize();

			//	Right of travel is the low side, by the convention in the header.
			Vector3 right(dir.Y,-dir.X,0.0f);

			const Vector3 & at = escarpment.Points[i].Position;
			float high = 0.0f;
			float low = 0.0f;
			bool got_high = WorldTerrainSystem::Sample_Height(at.X - (right.X * inset),
																			  at.Y - (right.Y * inset),&high);
			bool got_low  = WorldTerrainSystem::Sample_Height(at.X + (right.X * inset),
																			  at.Y + (right.Y * inset),&low);

			if (!got_high || !got_low) {
				continue;
			}

			//	Whichever way round the author drew it, the upper bench is the higher ground.
			if (low > high) {
				float swap = high;
				high = low;
				low = swap;
			}

			escarpment.Points[i].UpperHeight = high;
			escarpment.Points[i].LowerHeight = low;
			conformed++;
		}
	}

	return conformed;
}


/***********************************************************************************************
 * EscarpmentSystem::Find_Cliff_Texture -- what dresses the face                               *
 *                                                                                             *
 * The escarpment's own layer index if it names one, otherwise the first layer of kind         *
 * TERRAIN_LAYER_CLIFF.  Section 18's default table already defines one, so an escarpment that *
 * says nothing still comes out as rock rather than as stretched ground.                       *
 *                                                                                             *
 * Returns false when nothing can be found, which is not an error: the face is then built      *
 * untextured, counted, and is still solid.                                                    *
 *=============================================================================================*/
bool EscarpmentSystem::Find_Cliff_Texture(const EscarpmentClass & escarpment,
														TextureClass ** texture_out,float * meters_per_tile_out)
{
	if (texture_out != nullptr)			{ *texture_out = nullptr; }
	if (meters_per_tile_out != nullptr)	{ *meters_per_tile_out = DEFAULT_METERS_PER_TILE; }

	int index = escarpment.Get_Layer_Index();

	if (index < 0) {
		for (int i = 0; i < TerrainTextureSystem::Get_Layer_Count(); i++) {
			TerrainLayerClass * layer = TerrainTextureSystem::Peek_Layer(i);
			if ((layer != nullptr) && (layer->Kind == TERRAIN_LAYER_CLIFF)) {
				index = i;
				break;
			}
		}
	}

	if (index < 0) {
		return false;
	}

	TerrainLayerClass * layer = TerrainTextureSystem::Peek_Layer(index);
	if ((layer == nullptr) || (layer->Material == nullptr)) {
		return false;
	}

	if (meters_per_tile_out != nullptr) {
		float scale = layer->Material->Get_Meters_Per_Tile();
		if (scale > WWMATH_EPSILON) {
			*meters_per_tile_out = scale;
		}
	}

	TextureClass * texture = layer->Material->Peek_Texture();
	if (texture == nullptr) {
		return false;
	}

	if (texture_out != nullptr) {
		*texture_out = texture;
	}
	return true;
}


/***********************************************************************************************
 * EscarpmentSystem::Build_Escarpment_Geometry -- the face strip, and its collision            *
 *                                                                                             *
 * One quad per polyline segment, two triangles each, built in world space and given an        *
 * identity transform -- the same thing the road and bridge builders do, and for the same      *
 * reason: a polyline is not at any one place, so there is no sensible local origin for it.    *
 *=============================================================================================*/
/***********************************************************************************************
 * Face_Point -- where a station's face sits at fraction t of the way from toe to rim            *
 *                                                                                             *
 *	The straight batter line from base to top, plus this station's own inflection pushed out     *
 *	over the drop by a bulge that is zero at both ends.  With a single face row, where the only  *
 *	samples are t=0 and t=1, the inflection term vanishes entirely -- which is what makes a      *
 *	default escarpment come out exactly as it did before rows and inflection existed.            *
 *=============================================================================================*/
static void
Face_Point(float meters_per_tile,const Vector3 & right,const Vector3 & base,const Vector3 & top,
			  const EscarpmentPointClass & pt,float t,Vector3 * pos_out,float * v_out)
{
	float bulge = 4.0f * t * (1.0f - t);
	Vector3 xy = base + ((top - base) * t) + (right * (pt.FaceInflection * bulge));
	float height = pt.Get_Face_Height();

	*pos_out = Vector3 (xy.X,xy.Y,pt.LowerHeight + (height * t));
	*v_out   = (height * t) / meters_per_tile;
}


bool EscarpmentSystem::Build_Escarpment_Geometry(EscarpmentClass & escarpment)
{
	Destroy_Escarpment_Geometry(escarpment);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return false;
	}

	int count = escarpment.Points.Count();
	if (count < 2) {
		return false;
	}

	TextureClass * texture = nullptr;
	float meters_per_tile = DEFAULT_METERS_PER_TILE;
	bool textured = Find_Cliff_Texture(escarpment,&texture,&meters_per_tile);
	if (meters_per_tile < WWMATH_EPSILON) {
		meters_per_tile = DEFAULT_METERS_PER_TILE;
	}

	float batter = escarpment.Get_Batter();
	int	row_count = escarpment.Get_Face_Row_Count();
	float toe_width = escarpment.Get_Toe_Width();

	//	Two passes over the same geometry: the builder counts first, then fills.
	DynamicMeshBuilderClass builder;

	for (int pass = 0; pass < 2; pass++) {

		if (pass == 0) {
			builder.Begin_Count();
		} else {
			if (!builder.Begin_Build()) {
				return false;
			}
		}

		float travelled = 0.0f;

		for (int i = 1; i < count; i++) {

			const EscarpmentPointClass & p0 = escarpment.Points[i - 1];
			const EscarpmentPointClass & p1 = escarpment.Points[i];

			Vector3 dir = p1.Position - p0.Position;
			dir.Z = 0.0f;
			float segment_length = dir.Length();
			if (segment_length < WWMATH_EPSILON) {
				continue;
			}
			dir.Normalize();

			float h0 = p0.Get_Face_Height();
			float h1 = p1.Get_Face_Height();
			if ((h0 < MIN_FACE_HEIGHT) && (h1 < MIN_FACE_HEIGHT)) {
				travelled += segment_length;
				continue;
			}

			//	Right of travel is downhill; push the face that way to clear the very steep
			//	heightfield cells usually standing right behind it.
			Vector3 right(dir.Y,-dir.X,0.0f);

			/*
			**	V2 Phase 10 portal cuts.  The stretches of this segment inside any cut disc are
			**	removed and the rest emitted as up to MAX_SPANS spans, each with its stations
			**	interpolated from the segment's own two -- so an uncut segment is exactly one span
			**	over [0,1], identical to what this loop emitted before cuts existed.
			*/
			enum { MAX_SPANS = 8 };
			float span_lo[MAX_SPANS];
			float span_hi[MAX_SPANS];
			int span_count = 1;
			span_lo[0] = 0.0f;
			span_hi[0] = 1.0f;

			for (int c = 0; c < PortalCutCenters.Count(); c++) {

				//	|p0 + t*(p1-p0) - centre|^2 = r^2 solved for t, in plan.
				Vector3 d = p1.Position - p0.Position;
				Vector3 f = p0.Position - PortalCutCenters[c];
				float qa = (d.X * d.X) + (d.Y * d.Y);
				float qb = 2.0f * ((f.X * d.X) + (f.Y * d.Y));
				float qc = (f.X * f.X) + (f.Y * f.Y) - (PortalCutRadii[c] * PortalCutRadii[c]);
				float disc = (qb * qb) - (4.0f * qa * qc);
				if ((qa <= WWMATH_EPSILON) || (disc <= 0.0f)) {
					continue;
				}
				float root = WWMath::Sqrt(disc);
				float cut_lo = (-qb - root) / (2.0f * qa);
				float cut_hi = (-qb + root) / (2.0f * qa);
				if ((cut_hi <= 0.0f) || (cut_lo >= 1.0f)) {
					continue;
				}

				float new_lo[MAX_SPANS];
				float new_hi[MAX_SPANS];
				int new_count = 0;
				for (int k = 0; k < span_count; k++) {
					if ((cut_hi <= span_lo[k]) || (cut_lo >= span_hi[k])) {
						if (new_count < MAX_SPANS) { new_lo[new_count] = span_lo[k]; new_hi[new_count] = span_hi[k]; new_count++; }
						continue;
					}
					if ((cut_lo > span_lo[k]) && (new_count < MAX_SPANS)) {
						new_lo[new_count] = span_lo[k]; new_hi[new_count] = cut_lo; new_count++;
					}
					if ((cut_hi < span_hi[k]) && (new_count < MAX_SPANS)) {
						new_lo[new_count] = cut_hi; new_hi[new_count] = span_hi[k]; new_count++;
					}
				}
				for (int k = 0; k < new_count; k++) { span_lo[k] = new_lo[k]; span_hi[k] = new_hi[k]; }
				span_count = new_count;
			}

			for (int span = 0; span < span_count; span++) {

			float t_a = span_lo[span];
			float t_b = span_hi[span];
			if (((t_b - t_a) * segment_length) < WWMATH_EPSILON) {
				continue;
			}

			EscarpmentPointClass q0 = p0;
			EscarpmentPointClass q1 = p0;
			q0.Position			= p0.Position + ((p1.Position - p0.Position) * t_a);
			q0.UpperHeight		= p0.UpperHeight + ((p1.UpperHeight - p0.UpperHeight) * t_a);
			q0.LowerHeight		= p0.LowerHeight + ((p1.LowerHeight - p0.LowerHeight) * t_a);
			q0.FaceInflection	= p0.FaceInflection + ((p1.FaceInflection - p0.FaceInflection) * t_a);
			q1.Position			= p0.Position + ((p1.Position - p0.Position) * t_b);
			q1.UpperHeight		= p0.UpperHeight + ((p1.UpperHeight - p0.UpperHeight) * t_b);
			q1.LowerHeight		= p0.LowerHeight + ((p1.LowerHeight - p0.LowerHeight) * t_b);
			q1.FaceInflection	= p0.FaceInflection + ((p1.FaceInflection - p0.FaceInflection) * t_b);

			Vector3 base0 = q0.Position + (right * FACE_OFFSET);
			Vector3 base1 = q1.Position + (right * FACE_OFFSET);

			//	The top leans back into the hill by the batter, which is away from the drop.
			Vector3 top0 = base0 - (right * (batter * q0.Get_Face_Height()));
			Vector3 top1 = base1 - (right * (batter * q1.Get_Face_Height()));

			float u0 = (travelled + (t_a * segment_length)) / meters_per_tile;
			float u1 = (travelled + (t_b * segment_length)) / meters_per_tile;

			/*
			**	The face is walked as `row_count` bands from toe to rim.  A station's position
			**	at fraction t of the way up is the straight batter line from its base to its
			**	top, plus its own inflection pushed out over the drop by a bulge that is zero
			**	at both ends -- so with one row, where the only samples are t=0 and t=1, the
			**	inflection term vanishes and the two triangles below are exactly the pair this
			**	function emitted before rows existed.
			*/

			for (int row = 0; row < row_count; row++) {

				float t_low  = (float)row / (float)row_count;
				float t_high = (float)(row + 1) / (float)row_count;

				Vector3 a0,a1,b0,b1;
				float va0,va1,vb0,vb1;
				Face_Point(meters_per_tile,right,base0,top0,q0,t_low, &a0,&va0);
				Face_Point(meters_per_tile,right,base1,top1,q1,t_low, &a1,&va1);
				Face_Point(meters_per_tile,right,base0,top0,q0,t_high,&b0,&vb0);
				Face_Point(meters_per_tile,right,base1,top1,q1,t_high,&b1,&vb1);

				//	Wound so the outward normal faces the low side, which is the side anything
				//	looking at a cliff is standing on.
				builder.Triangle(a0,a1,b0, u0,va0, u1,va1, u0,vb0);
				builder.Triangle(b0,a1,b1, u0,vb0, u1,va1, u1,vb1);
			}

			/*
			**	The scree apron, flat on the lower bench and flaring out over the drop.  It is
			**	drawn, never collided: it sits on ground that already stops everything it would
			**	stop, and a second surface a few centimetres above the first is how z-fighting
			**	and "standing on nothing" bugs start.
			*/
			if (toe_width > 0.0f) {

				Vector3 foot0(base0.X,base0.Y,q0.LowerHeight);
				Vector3 foot1(base1.X,base1.Y,q1.LowerHeight);
				Vector3 skirt0 = foot0 + (right * toe_width);
				Vector3 skirt1 = foot1 + (right * toe_width);
				skirt0.Z = q0.LowerHeight;
				skirt1.Z = q1.LowerHeight;

				float v_skirt = -(toe_width / meters_per_tile);

				builder.Triangle(skirt0,skirt1,foot0, u0,v_skirt, u1,v_skirt, u0,0.0f);
				builder.Triangle(foot0,skirt1,foot1, u0,0.0f, u1,v_skirt, u1,0.0f);
			}

			}	// span

			travelled += segment_length;
		}
	}

	if (builder.Get_Poly_Count() <= 0) {
		return false;
	}

	DynamicMeshClass * mesh = builder.Detach_Mesh();
	if (mesh == nullptr) {
		return false;
	}

	//	What terrain is drawn in, asked of the layer that owns it rather than decided here.
	ShaderClass shader;
	VertexMaterialClass * vmat = nullptr;
	if (ShaderManagerClass::Get_Pass_State(MATERIAL_PROGRAM_TERRAIN,0,&shader,&vmat)) {
		mesh->Set_Shader(shader);
		if (vmat != nullptr) {
			mesh->Set_Vertex_Material(vmat,true);
		}
	}
	REF_PTR_RELEASE(vmat);

	if (textured && (texture != nullptr)) {
		mesh->Set_Texture(texture,true);
	} else {
		UntexturedFaces++;
	}

	mesh->Set_Dirty();
	mesh->Set_Collision_Type(ESCARPMENT_COLLISION_BITS);

	StaticPhysClass * phys = new StaticPhysClass;
	phys->Set_Model(mesh);
	phys->Set_Transform(Matrix3D(1));
	mesh->Release_Ref();

	scene->Add_Static_Object(phys);
	escarpment.Faces.Add(phys);

	return true;
}


void EscarpmentSystem::Destroy_Escarpment_Geometry(EscarpmentClass & escarpment)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();

	for (int i = 0; i < escarpment.Faces.Count(); i++) {
		StaticPhysClass * phys = escarpment.Faces[i];
		if (phys == nullptr) {
			continue;
		}
		if (scene != nullptr) {
			scene->Remove_Object(phys);
		}
		phys->Release_Ref();
	}
	escarpment.Faces.Delete_All();
}


bool EscarpmentSystem::Build_Geometry(void)
{
	Destroy_Geometry();

	UntexturedFaces = 0;
	bool all = true;

	for (int e = 0; e < Escarpments.Count(); e++) {
		if (!Build_Escarpment_Geometry(Escarpments[e])) {
			all = false;
		}
	}

	GeometryBuilt = true;
	return all;
}


void EscarpmentSystem::Destroy_Geometry(void)
{
	for (int e = 0; e < Escarpments.Count(); e++) {
		Destroy_Escarpment_Geometry(Escarpments[e]);
	}
	GeometryBuilt = false;
}


bool EscarpmentSystem::Has_Geometry(void)
{
	return GeometryBuilt;
}


int EscarpmentSystem::Get_Face_Count(void)
{
	int total = 0;
	for (int e = 0; e < Escarpments.Count(); e++) {
		total += Escarpments[e].Faces.Count();
	}
	return total;
}


int EscarpmentSystem::Get_Untextured_Face_Count(void)
{
	return UntexturedFaces;
}


/***********************************************************************************************
 * EscarpmentSystem::Sample_Face --                                                            *
 *=============================================================================================*/
bool EscarpmentSystem::Sample_Face(float x,float y,float max_distance,
											  float * upper_out,float * lower_out,int * id_out)
{
	bool found = false;
	float best_distance = 0.0f;
	float best_upper = 0.0f;
	float best_lower = 0.0f;
	int best_id = -1;

	for (int e = 0; e < Escarpments.Count(); e++) {

		float upper = 0.0f;
		float lower = 0.0f;
		float distance = 0.0f;
		if (!Escarpments[e].Sample(x,y,&upper,&lower,&distance)) {
			continue;
		}
		if ((max_distance > 0.0f) && (distance > max_distance)) {
			continue;
		}

		if (!found || (distance < best_distance)) {
			found = true;
			best_distance = distance;
			best_upper = upper;
			best_lower = lower;
			best_id = Escarpments[e].Get_ID();
		}
	}

	if (!found) {
		return false;
	}

	if (upper_out != nullptr)	{ *upper_out = best_upper; }
	if (lower_out != nullptr)	{ *lower_out = best_lower; }
	if (id_out != nullptr)		{ *id_out = best_id; }
	return true;
}


/***********************************************************************************************
 * EscarpmentSystem::Is_Segment_Blocked -- what a route check asks                             *
 *                                                                                             *
 * Blocked means: the line crosses a face, and that face is taller than the caller said it can *
 * climb.  A face a locomotor can step up is not an obstacle, which is why the height comes in *
 * as a parameter rather than being decided here -- infantry and a tank get different answers  *
 * about the same cliff, and that difference is the whole point of Phase 65's graph.           *
 *=============================================================================================*/
bool EscarpmentSystem::Is_Segment_Blocked(const Vector3 & a,const Vector3 & b,
														float max_climbable_height,int * id_out)
{
	if (max_climbable_height < 0.0f) {
		max_climbable_height = 0.0f;
	}

	for (int e = 0; e < Escarpments.Count(); e++) {

		float face_height = 0.0f;
		if (!Escarpments[e].Does_Segment_Cross(a,b,&face_height)) {
			continue;
		}
		if (face_height <= max_climbable_height) {
			continue;
		}

		if (id_out != nullptr) {
			*id_out = Escarpments[e].Get_ID();
		}
		return true;
	}

	return false;
}
