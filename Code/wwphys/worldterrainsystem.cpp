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

#include "worldboundsservice.h"

#include "colmath.h"
#include "frustum.h"
#include "lineseg.h"
#include "matrix3d.h"
#include "pscene.h"
#include "renegadeterrainpatch.h"
#include "staticphys.h"
#include "terraintexturesystem.h"
#include "ww3d.h"
#include "wwdebug.h"
#include "wwmath.h"


HeightfieldClass *	WorldTerrainSystem::Heightfield				= nullptr;
bool						WorldTerrainSystem::ReportedNoCollision		= false;
bool						WorldTerrainSystem::ReportedNoFarTerrain	= false;

StaticPhysClass **	WorldTerrainSystem::CollisionPatches			= nullptr;
int						WorldTerrainSystem::CollisionPatchCountX	= 0;
int						WorldTerrainSystem::CollisionPatchCountY	= 0;

StaticPhysClass **	WorldTerrainSystem::FarBlocks					= nullptr;
bool *					WorldTerrainSystem::FarBlockDirty			= nullptr;
int						WorldTerrainSystem::FarBlockCountX			= 0;
int						WorldTerrainSystem::FarBlockCountY			= 0;
float						WorldTerrainSystem::FarTerrainDrop			= 0.0f;
int						WorldTerrainSystem::FarRefillCount			= 0;

bool *					WorldTerrainSystem::FarBlockLastNear		= nullptr;
float *					WorldTerrainSystem::FarBlockOverlapTimer	= nullptr;
int						WorldTerrainSystem::VisibleNearPatchCount	= 0;
int						WorldTerrainSystem::VisibleFarBlockCount	= 0;


/*
**	The vertex range one patch covers.
**
**	Patches share the vertices along their seam -- patch p ends on the same grid line patch p+1
**	starts on -- which is why the range is inclusive at both ends, and why moving a seam vertex
**	has to dirty the patches on both sides of it.
*/
static void Patch_Vertex_Range(const HeightfieldClass & field,int px,int py,
										 int * ix0,int * iy0,int * ix1,int * iy1)
{
	*ix0 = px * HeightfieldClass::PATCH_CELLS;
	*iy0 = py * HeightfieldClass::PATCH_CELLS;
	*ix1 = *ix0 + HeightfieldClass::PATCH_CELLS;
	*iy1 = *iy0 + HeightfieldClass::PATCH_CELLS;

	if (*ix1 > field.Get_Vertex_Count_X() - 1) *ix1 = field.Get_Vertex_Count_X() - 1;
	if (*iy1 > field.Get_Vertex_Count_Y() - 1) *iy1 = field.Get_Vertex_Count_Y() - 1;
}


void WorldTerrainSystem::Init(void)
{
	//	Nothing to build.  A level with no heightfield is the normal case and must cost nothing,
	//	so the field is allocated by Create_Terrain and not before.
	Destroy_Terrain();
	TerrainTextureSystem::Init();
	ReportedNoCollision = false;
	ReportedNoFarTerrain = false;
	FarRefillCount = 0;
	VisibleNearPatchCount = 0;
	VisibleFarBlockCount = 0;
}


void WorldTerrainSystem::Shutdown(void)
{
	Destroy_Terrain();
	TerrainTextureSystem::Shutdown();
}


bool WorldTerrainSystem::Create_Terrain(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin)
{
	Destroy_Terrain();

	Heightfield = new HeightfieldClass;
	if (!Heightfield->Create(vertex_count_x,vertex_count_y,cell_size,origin)) {
		Destroy_Terrain();
		return false;
	}

	//	The far layer's block table belongs to the field, not to the renderer: something that
	//	changes the ground has to be able to say so before anything has been drawn, and a table
	//	that only appeared once a scene existed would silently drop those edits.  The blocks
	//	themselves are built later, by a caller that has a scene to put them in.
	Create_Far_Block_Table();

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
	//	The collision patches are geometry cut out of this field, so they go first, while the
	//	field they were cut from is still there to describe them.  The masks go with them: a mask
	//	is one value per grid point of *this* field, and keeping them across a new field of a
	//	different size would read the wrong ground.
	Destroy_Far_Terrain();
	Destroy_Collision();
	TerrainTextureSystem::Destroy_Masks();

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
bool WorldTerrainSystem::Get_Material(float x,float y,int * layer_index_out)
{
	//	The shape of the ground is this class's; what it is made of is Section 18's, and asking
	//	it here rather than answering here is what keeps there being one place that decides.
	return TerrainTextureSystem::Get_Dominant_Layer_At(x,y,layer_index_out);
}


bool WorldTerrainSystem::Get_Surface_Type(float x,float y,int * surface_type_out)
{
	return TerrainTextureSystem::Get_Surface_Type_At(x,y,surface_type_out);
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

	//	The far block over this patch is now describing ground that has moved.  Marking it here
	//	rather than only in Update_Far_Terrain means the far layer hears about an edit even if
	//	collision is rebuilt first and clears the field's own dirty flag on the way past.
	Invalidate_Far_Block(px / FAR_BLOCK_PATCHES,py / FAR_BLOCK_PATCHES);
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
 * Terrain_Edit_Allowed -- V2 roadmap Section 34 Phase 3 step 11, the immutable cliff band       *
 *                                                                                             *
 *	The boundary cliff band is reserved geometry.  Section 5.9 forbids a terrain brush from      *
 *	modifying it for the same reason it forbids a road or a base in it: the cliff ring's whole   *
 *	job is to be the one thing a player cannot get past, and ground that can be raised inside    *
 *	the band is a ramp waiting to be built.                                                      *
 *                                                                                             *
 *	This asks WorldBoundsService the same question building placement asks -- may something of   *
 *	this radius exist centred here -- rather than inventing a second notion of where the band    *
 *	is.  With no boundary configured, which is every hand-authored map, it always answers yes    *
 *	and terrain editing behaves exactly as it did before v2.                                     *
 *=============================================================================================*/
static bool
Terrain_Edit_Allowed(const Vector3 & center,float radius)
{
	return WorldBoundsService::Can_Place_In_Region(Vector2(center.X,center.Y),radius);
}

/***********************************************************************************************
 * Shaping                                                                                     *
 *=============================================================================================*/
void WorldTerrainSystem::Modify_Height_Region(const Vector3 & center,float radius,float delta)
{
	if (!Has_Terrain()) {
		return;
	}
	if (!Terrain_Edit_Allowed(center,radius)) {
		return;
	}
	Heightfield->Modify_Height_Region(center,radius,delta);

	//	The background layer is a picture of ground that has just moved.  Told here, where the
	//	footprint is known, rather than left for something to notice later.
	Invalidate_Far_Terrain_Region(AABoxClass(Vector3(center.X,center.Y,center.Z),
														 Vector3(radius,radius,WWMath::Fabs(delta) + radius)));
}


/*
**	The box a polyline of a given width covers, for telling the far layer what moved.
*/
static void Polyline_Region(const Vector3 * points,int count,float radius,AABoxClass * box_out)
{
	Vector3 lo = points[0];
	Vector3 hi = points[0];

	for (int i = 1; i < count; i++) {
		if (points[i].X < lo.X) lo.X = points[i].X;
		if (points[i].Y < lo.Y) lo.Y = points[i].Y;
		if (points[i].Z < lo.Z) lo.Z = points[i].Z;
		if (points[i].X > hi.X) hi.X = points[i].X;
		if (points[i].Y > hi.Y) hi.Y = points[i].Y;
		if (points[i].Z > hi.Z) hi.Z = points[i].Z;
	}

	box_out->Center = (lo + hi) * 0.5f;
	box_out->Extent = ((hi - lo) * 0.5f) + Vector3(radius,radius,radius);
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
			if (!Terrain_Edit_Allowed(at,radius)) {
				continue;
			}
			Heightfield->Blend_Height_Region(at,radius,at.Z - Heightfield->Get_Origin().Z,blend);
		}
	}

	AABoxClass region;
	Polyline_Region(points,count,radius,&region);
	Invalidate_Far_Terrain_Region(region);
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
			if (!Terrain_Edit_Allowed(at,radius)) {
				continue;
			}
			Heightfield->Blend_Height_Region(at,radius,at.Z - Heightfield->Get_Origin().Z,1.0f);
			Heightfield->Modify_Height_Region(at,radius,-depth);
		}
	}

	AABoxClass region;
	Polyline_Region(points,count,radius,&region);
	region.Extent.Z += WWMath::Fabs(depth);
	Invalidate_Far_Terrain_Region(region);
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
/***********************************************************************************************
 * Collision                                                                                   *
 *=============================================================================================*/

bool WorldTerrainSystem::Fill_Patch_Model(RenegadeTerrainPatchClass * model,int px,int py)
{
	if ((model == nullptr) || !Has_Terrain()) {
		return false;
	}
	if ((px < 0) || (py < 0) ||
		 (px >= Heightfield->Get_Patch_Count_X()) || (py >= Heightfield->Get_Patch_Count_Y())) {
		return false;
	}

	int ix0,iy0,ix1,iy1;
	Patch_Vertex_Range(*Heightfield,px,py,&ix0,&iy0,&ix1,&iy1);

	const Vector3 & origin = Heightfield->Get_Origin();
	float cell = Heightfield->Get_Cell_Size();

	/*
	**	The patch works out which cell a point is in from its own bounding box, so the box has to
	**	be right before anything asks it a question.  Its height range is taken from the field
	**	rather than left to grow from the vertices, because Set_Vertex_Pos only ever raises the
	**	ceiling -- a patch refilled after the ground was lowered would keep the old one forever.
	*/
	TerrainPatchClass extents;
	Heightfield->Get_Patch(px,py,&extents);

	model->Set_Bounding_Box_Min(Vector3(origin.X + ix0 * cell,
													origin.Y + iy0 * cell,
													origin.Z + extents.MinHeight));
	model->Set_Bounding_Box_Max(Vector3(origin.X + ix1 * cell,
													origin.Y + iy1 * cell,
													origin.Z + extents.MaxHeight));

	for (int iy = iy0; iy <= iy1; iy++) {
		for (int ix = ix0; ix <= ix1; ix++) {

			Vector3 pos(origin.X + ix * cell,
							origin.Y + iy * cell,
							origin.Z + Heightfield->Get_Height(ix,iy));

			model->Set_Vertex_Pos(ix - ix0,iy - iy0,pos);
			model->Set_Vertex_Normal(ix - ix0,iy - iy0,Heightfield->Compute_Vertex_Normal(ix,iy));
		}
	}

	//	Both of these do nothing until the patch has material passes, which is where the terrain
	//	texture system will hang.  They are called anyway so that the fill is still right once it
	//	does, rather than becoming a thing someone has to remember to add later.
	model->Update_UVs();
	model->Update_Vertex_Render_Lists();
	return true;
}


RenegadeTerrainPatchClass * WorldTerrainSystem::Create_Patch_Model(int px,int py)
{
	if (!Has_Terrain()) {
		return nullptr;
	}
	if ((px < 0) || (py < 0) ||
		 (px >= Heightfield->Get_Patch_Count_X()) || (py >= Heightfield->Get_Patch_Count_Y())) {
		return nullptr;
	}

	int ix0,iy0,ix1,iy1;
	Patch_Vertex_Range(*Heightfield,px,py,&ix0,&iy0,&ix1,&iy1);

	//	The last patch in a row is short whenever the field is not a whole number of patches
	//	across, so the size comes from the range rather than from PATCH_CELLS.
	RenegadeTerrainPatchClass * model = NEW_REF(RenegadeTerrainPatchClass,());
	model->Allocate(ix1 - ix0 + 1,iy1 - iy0 + 1,Heightfield->Get_Cell_Size());

	if (!Fill_Patch_Model(model,px,py)) {
		model->Release_Ref();
		return nullptr;
	}

	return model;
}


bool WorldTerrainSystem::Build_Collision_Patch(int px,int py)
{
	int index = py * CollisionPatchCountX + px;
	StaticPhysClass * phys = CollisionPatches[index];

	if (phys == nullptr) {

		RenegadeTerrainPatchClass * model = Create_Patch_Model(px,py);
		if (model == nullptr) {
			return false;
		}

		phys = new StaticPhysClass;
		phys->Set_Model(model);
		phys->Set_Transform(Matrix3D(1));
		model->Release_Ref();				// the physics object holds it now

		PhysicsSceneClass::Get_Instance()->Add_Static_Object(phys);
		CollisionPatches[index] = phys;

	} else {

		if (!Heightfield->Is_Patch_Dirty(px,py)) {
			return true;
		}

		RenegadeTerrainPatchClass * model = (RenegadeTerrainPatchClass *)phys->Peek_Model();
		if ((model == nullptr) || !Fill_Patch_Model(model,px,py)) {
			return false;
		}

		//	Same ground, coarser copy: the far block over a patch that just moved has to move too.
		Invalidate_Far_Block(px / FAR_BLOCK_PATCHES,py / FAR_BLOCK_PATCHES);
	}

	phys->Update_Cull_Box();
	Heightfield->Validate_Patch(px,py);
	return true;
}


bool WorldTerrainSystem::Build_Collision(void)
{
	if (!Has_Terrain()) {
		Destroy_Collision();
		return false;
	}

	//	Set_Model asks the scene whether it already holds the object, so there is no building
	//	collision without one.  The checks run in exactly that state and want an answer rather
	//	than a crash.
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		if (!ReportedNoCollision) {
			ReportedNoCollision = true;
			WWDEBUG_SAY(("WorldTerrainSystem: no physics scene to put terrain collision in.\r\n"));
		}
		return false;
	}

	int count_x = Heightfield->Get_Patch_Count_X();
	int count_y = Heightfield->Get_Patch_Count_Y();

	//	A field of a different shape is a different set of patches, so start over rather than try
	//	to grow the grid around what is already there.
	if ((CollisionPatches != nullptr) &&
		 ((count_x != CollisionPatchCountX) || (count_y != CollisionPatchCountY))) {
		Destroy_Collision();
	}

	bool first_build = (CollisionPatches == nullptr);
	if (first_build) {
		CollisionPatchCountX = count_x;
		CollisionPatchCountY = count_y;
		CollisionPatches = new StaticPhysClass *[count_x * count_y];
		for (int index = 0; index < count_x * count_y; index++) {
			CollisionPatches[index] = nullptr;
		}
	}

	bool built_everything = true;
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			if (!Build_Collision_Patch(px,py)) {
				built_everything = false;
			}
		}
	}

	//	Geometry that has just been rebuilt is wearing the materials the old shape called for, so
	//	dressing follows building rather than being asked for separately.  With no layers defined
	//	this does nothing, which is the right answer for terrain nobody has dressed.
	if (TerrainTextureSystem::Get_Layer_Count() > 0) {
		TerrainTextureSystem::Build_All_Patch_Materials();
	}

	if (first_build) {
		WWDEBUG_SAY(("WorldTerrainSystem: terrain collision built, %d x %d patches.\r\n",
						 count_x,count_y));
	}

	return built_everything;
}


void WorldTerrainSystem::Update_Collision(void)
{
	//	Deliberately not a build.  Something shaping the ground every frame wants the patches it
	//	moved rebuilt, not a world created underneath it because it called this too early.
	if (CollisionPatches == nullptr) {
		return;
	}
	Build_Collision();
}


void WorldTerrainSystem::Destroy_Collision(void)
{
	if (CollisionPatches == nullptr) {
		return;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	int count = CollisionPatchCountX * CollisionPatchCountY;

	for (int index = 0; index < count; index++) {

		StaticPhysClass * phys = CollisionPatches[index];
		if (phys == nullptr) {
			continue;
		}

		if (scene != nullptr) {
			scene->Remove_Object(phys);
		}
		phys->Release_Ref();
		CollisionPatches[index] = nullptr;
	}

	delete [] CollisionPatches;
	CollisionPatches = nullptr;
	CollisionPatchCountX = 0;
	CollisionPatchCountY = 0;
}


bool WorldTerrainSystem::Has_Collision(void)
{
	return (CollisionPatches != nullptr);
}


int WorldTerrainSystem::Get_Collision_Patch_Count(void)
{
	return CollisionPatchCountX * CollisionPatchCountY;
}


StaticPhysClass * WorldTerrainSystem::Peek_Collision_Patch(int px,int py)
{
	if (CollisionPatches == nullptr) {
		return nullptr;
	}
	if ((px < 0) || (py < 0) || (px >= CollisionPatchCountX) || (py >= CollisionPatchCountY)) {
		return nullptr;
	}
	return CollisionPatches[py * CollisionPatchCountX + px];
}


RenegadeTerrainPatchClass * WorldTerrainSystem::Peek_Patch_Model(int px,int py)
{
	StaticPhysClass * phys = Peek_Collision_Patch(px,py);
	if (phys == nullptr) {
		return nullptr;
	}
	return (RenegadeTerrainPatchClass *)phys->Peek_Model();
}


/***********************************************************************************************
 * The far / background layer -- roadmap Section 34                                             *
 *                                                                                              *
 * The donor calls this backup terrain and draws it at lower resolution behind the near work.    *
 * Here it is one decimated copy of the heightfield this class already owns, cut into blocks of  *
 * FAR_BLOCK_PATCHES near patches and sampled every FAR_STRIDE grid points.  Because FAR_STRIDE  *
 * divides PATCH_CELLS, every near-patch boundary line is also a sampled line, and because       *
 * neighbouring blocks end and start on the same sampled line they share those vertices exactly. *
 *                                                                                              *
 * It has no collision -- Set_Collision_Type(0) means its Cast_Ray refuses every ray outright -- *
 * so the near representation stays the only thing that can answer a question about the ground.  *
 *=============================================================================================*/

/*
**	How many sampled points a block has, in each direction.
**
**	The last block of a row is short whenever the field is not a whole number of blocks across.
**	Rather than give it a different spacing -- which would make its quads non-square and its
**	shared edge disagree with its neighbour -- it keeps the full point count and clamps the
**	samples that fall off the end to the last grid line.  Those repeated points make zero-area
**	quads, which draw nothing.
*/
void WorldTerrainSystem::Far_Block_Point_Counts(int bx,int by,int * points_x_out,int * points_y_out)
{
	(void)bx;
	(void)by;

	const int cells = FAR_BLOCK_PATCHES * HeightfieldClass::PATCH_CELLS;
	*points_x_out = (cells / FAR_STRIDE) + 1;
	*points_y_out = (cells / FAR_STRIDE) + 1;
}


/***********************************************************************************************
 * WorldTerrainSystem::Compute_Far_Terrain_Drop -- how far the coarse surface may ride high      *
 *                                                                                              *
 * Over one sampled cell the drawn far surface is a pair of triangles through the four sampled   *
 * corners.  Between those corners it can be above the real ground -- a hilltop sampled away is  *
 * the obvious case in reverse, a valley floor is the case that matters -- and where it is, far  *
 * terrain would poke through near terrain at the boundary.                                      *
 *                                                                                              *
 * So measure it: for every grid point, how much higher is the coarse surface than the real one. *
 * The largest answer over the whole field is what the far layer is lowered by.  Both diagonals  *
 * and the bilinear surface are tested, so the bound holds whichever way the renderer splits a   *
 * quad and whatever a later reader assumes.                                                     *
 *                                                                                              *
 * One number for the whole field rather than one per block, deliberately: a per-block drop      *
 * would move two neighbours' shared edge by different amounts and tear the far layer open       *
 * along every block seam, which is the crack this phase exists to not have.                     *
 *=============================================================================================*/
float WorldTerrainSystem::Compute_Far_Terrain_Drop(void)
{
	if (!Has_Terrain()) {
		return 0.0f;
	}

	int last_x = Heightfield->Get_Vertex_Count_X() - 1;
	int last_y = Heightfield->Get_Vertex_Count_Y() - 1;
	float max_excess = 0.0f;

	for (int cy0 = 0; cy0 < last_y; cy0 += FAR_STRIDE) {

		int cy1 = cy0 + FAR_STRIDE;
		if (cy1 > last_y) cy1 = last_y;

		for (int cx0 = 0; cx0 < last_x; cx0 += FAR_STRIDE) {

			int cx1 = cx0 + FAR_STRIDE;
			if (cx1 > last_x) cx1 = last_x;

			float h00 = Heightfield->Get_Height(cx0,cy0);
			float h10 = Heightfield->Get_Height(cx1,cy0);
			float h01 = Heightfield->Get_Height(cx0,cy1);
			float h11 = Heightfield->Get_Height(cx1,cy1);

			float inv_dx = 1.0f / (float)(cx1 - cx0);
			float inv_dy = 1.0f / (float)(cy1 - cy0);

			for (int iy = cy0; iy <= cy1; iy++) {

				float ty = (float)(iy - cy0) * inv_dy;

				for (int ix = cx0; ix <= cx1; ix++) {

					float tx = (float)(ix - cx0) * inv_dx;

					//	Bilinear, and both triangulations of the same four corners.
					float coarse = ((h00 * (1.0f - tx)) + (h10 * tx)) * (1.0f - ty) +
										((h01 * (1.0f - tx)) + (h11 * tx)) * ty;

					float diag_a = (tx >= ty) ? (h00 + tx * (h10 - h00) + ty * (h11 - h10))
														: (h00 + ty * (h01 - h00) + tx * (h11 - h01));
					if (diag_a > coarse) coarse = diag_a;

					float diag_b = ((tx + ty) <= 1.0f)
											? (h00 + tx * (h10 - h00) + ty * (h01 - h00))
											: (h11 + (1.0f - tx) * (h01 - h11) + (1.0f - ty) * (h10 - h11));
					if (diag_b > coarse) coarse = diag_b;

					float excess = coarse - Heightfield->Get_Height(ix,iy);
					if (excess > max_excess) {
						max_excess = excess;
					}
				}
			}
		}
	}

	return max_excess;
}


bool WorldTerrainSystem::Fill_Far_Block_Model(RenegadeTerrainPatchClass * model,int bx,int by)
{
	if ((model == nullptr) || !Has_Terrain()) {
		return false;
	}
	if ((bx < 0) || (by < 0) ||
		 (bx >= Get_Far_Block_Count_X()) || (by >= Get_Far_Block_Count_Y())) {
		return false;
	}

	int points_x,points_y;
	Far_Block_Point_Counts(bx,by,&points_x,&points_y);

	int ix0 = bx * FAR_BLOCK_PATCHES * HeightfieldClass::PATCH_CELLS;
	int iy0 = by * FAR_BLOCK_PATCHES * HeightfieldClass::PATCH_CELLS;
	int last_x = Heightfield->Get_Vertex_Count_X() - 1;
	int last_y = Heightfield->Get_Vertex_Count_Y() - 1;

	const Vector3 & origin = Heightfield->Get_Origin();
	float cell = Heightfield->Get_Cell_Size();

	//	The box is set before any vertex is written, because Set_Vertex_Pos only ever pushes the
	//	Z range outwards -- a block refilled after the ground fell would otherwise keep the old
	//	floor forever.  Sweep the samples once for the range rather than trusting the near
	//	patches' ranges, which cover the same ground but not the same points.
	float min_z = 0.0f;
	float max_z = 0.0f;
	bool first = true;

	for (int py_index = 0; py_index < points_y; py_index++) {

		int iy = iy0 + py_index * FAR_STRIDE;
		if (iy > last_y) iy = last_y;

		for (int px_index = 0; px_index < points_x; px_index++) {

			int ix = ix0 + px_index * FAR_STRIDE;
			if (ix > last_x) ix = last_x;

			float z = Heightfield->Get_Height(ix,iy) - FarTerrainDrop;
			if (first || (z < min_z)) min_z = z;
			if (first || (z > max_z)) max_z = z;
			first = false;
		}
	}

	int ix_last = ix0 + (points_x - 1) * FAR_STRIDE;
	int iy_last = iy0 + (points_y - 1) * FAR_STRIDE;
	if (ix_last > last_x) ix_last = last_x;
	if (iy_last > last_y) iy_last = last_y;

	model->Set_Bounding_Box_Min(Vector3(origin.X + ix0 * cell,origin.Y + iy0 * cell,origin.Z + min_z));
	model->Set_Bounding_Box_Max(Vector3(origin.X + ix_last * cell,origin.Y + iy_last * cell,origin.Z + max_z));

	for (int py_index = 0; py_index < points_y; py_index++) {

		int iy = iy0 + py_index * FAR_STRIDE;
		if (iy > last_y) iy = last_y;

		for (int px_index = 0; px_index < points_x; px_index++) {

			int ix = ix0 + px_index * FAR_STRIDE;
			if (ix > last_x) ix = last_x;

			Vector3 pos(origin.X + ix * cell,
							origin.Y + iy * cell,
							origin.Z + Heightfield->Get_Height(ix,iy) - FarTerrainDrop);

			model->Set_Vertex_Pos(px_index,py_index,pos);
			model->Set_Vertex_Normal(px_index,py_index,Heightfield->Compute_Vertex_Normal(ix,iy));
		}
	}

	//	Dressed off the same rule table the near patches read, at the same points this block
	//	sampled, so a slope does not change colour when it crosses the LOD boundary.
	if (TerrainTextureSystem::Get_Layer_Count() > 0) {
		TerrainTextureSystem::Build_Far_Block_Materials(model,ix0,iy0,FAR_STRIDE,points_x,points_y);
	}

	model->Update_UVs();
	model->Update_Vertex_Render_Lists();

	FarRefillCount++;
	return true;
}


/***********************************************************************************************
 * WorldTerrainSystem::Create_Far_Block_Model -- one block's geometry, on its own                *
 *                                                                                              *
 * Public for the same reason Create_Patch_Model is: the layer builder is not the only caller    *
 * that wants a block's mesh, and a check with no physics scene has nowhere to put one.          *
 *=============================================================================================*/
RenegadeTerrainPatchClass * WorldTerrainSystem::Create_Far_Block_Model(int bx,int by)
{
	if (!Has_Terrain()) {
		return nullptr;
	}
	if ((bx < 0) || (by < 0) ||
		 (bx >= Get_Far_Block_Count_X()) || (by >= Get_Far_Block_Count_Y())) {
		return nullptr;
	}

	//	With no block built nothing has measured the drop yet, and a block placed at the wrong
	//	height would not meet its neighbours.  Measuring it here costs one pass over the field
	//	per block, which only a caller building blocks by hand ever pays -- the layer builder
	//	measures once before it starts.
	if (Get_Far_Block_Built_Count() == 0) {
		FarTerrainDrop = Compute_Far_Terrain_Drop();
	}

	int points_x,points_y;
	Far_Block_Point_Counts(bx,by,&points_x,&points_y);

	RenegadeTerrainPatchClass * model = NEW_REF(RenegadeTerrainPatchClass,());
	model->Allocate(points_x,points_y,Heightfield->Get_Cell_Size() * (float)FAR_STRIDE);

	//	Nothing collides with the far layer.  It is a picture of ground the near patches already
	//	own, and a second surface that answers rays would be a second terrain representation --
	//	the one thing Section 34 says not to end up with.
	model->Set_Collision_Type(0);

	if (!Fill_Far_Block_Model(model,bx,by)) {
		model->Release_Ref();
		return nullptr;
	}

	return model;
}


bool WorldTerrainSystem::Build_Far_Block(int bx,int by)
{
	int index = by * FarBlockCountX + bx;
	StaticPhysClass * phys = FarBlocks[index];

	if (phys == nullptr) {

		RenegadeTerrainPatchClass * model = Create_Far_Block_Model(bx,by);
		if (model == nullptr) {
			return false;
		}

		phys = new StaticPhysClass;
		phys->Set_Model(model);
		phys->Set_Transform(Matrix3D(1));
		model->Release_Ref();				// the physics object holds it now

		PhysicsSceneClass::Get_Instance()->Add_Static_Object(phys);
		FarBlocks[index] = phys;

	} else {

		RenegadeTerrainPatchClass * model = (RenegadeTerrainPatchClass *)phys->Peek_Model();
		if ((model == nullptr) || !Fill_Far_Block_Model(model,bx,by)) {
			return false;
		}
	}

	phys->Update_Cull_Box();
	FarBlockDirty[index] = false;
	return true;
}


void WorldTerrainSystem::Create_Far_Block_Table(void)
{
	Destroy_Far_Terrain();

	int count_x = Get_Far_Block_Count_X();
	int count_y = Get_Far_Block_Count_Y();
	if ((count_x < 1) || (count_y < 1)) {
		return;
	}

	FarBlockCountX = count_x;
	FarBlockCountY = count_y;
	FarBlocks = new StaticPhysClass *[count_x * count_y];
	FarBlockDirty = new bool[count_x * count_y];
	FarBlockLastNear = new bool[count_x * count_y];
	FarBlockOverlapTimer = new float[count_x * count_y];

	//	Clean, not dirty.  Dirty means "the block that is built is out of date", and there is no
	//	block yet; the first build creates every one of them whatever the flags say.  Starting
	//	clean is what lets a caller ask which regions an edit actually touched.
	for (int index = 0; index < count_x * count_y; index++) {
		FarBlocks[index] = nullptr;
		FarBlockDirty[index] = false;
		//	True: an unbuilt block "draws near whatever the distance says" (see
		//	Update_Terrain_LOD), so starting the edge-detector at "was near" means the very
		//	first real decision only triggers an overlap window if it disagrees.
		FarBlockLastNear[index] = true;
		FarBlockOverlapTimer[index] = 0.0f;
	}
}


bool WorldTerrainSystem::Build_Far_Terrain_Representation(void)
{
	if (!Has_Terrain()) {
		Destroy_Far_Terrain();
		return false;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		if (!ReportedNoFarTerrain) {
			ReportedNoFarTerrain = true;
			WWDEBUG_SAY(("WorldTerrainSystem: no physics scene to put far terrain in.\r\n"));
		}
		return false;
	}

	if (FarBlocks == nullptr) {
		Create_Far_Block_Table();
		if (FarBlocks == nullptr) {
			return false;
		}
	}

	bool first_build = (Get_Far_Block_Built_Count() == 0);

	//	The drop is one number for the whole layer, so a change to it moves every block and
	//	every block has to be refilled.  That is the price of blocks that share their edges.
	float drop = Compute_Far_Terrain_Drop();
	if (drop != FarTerrainDrop) {
		FarTerrainDrop = drop;
		for (int index = 0; index < FarBlockCountX * FarBlockCountY; index++) {
			FarBlockDirty[index] = true;
		}
	}

	bool built_everything = true;
	for (int by = 0; by < FarBlockCountY; by++) {
		for (int bx = 0; bx < FarBlockCountX; bx++) {
			if (!Build_Far_Block(bx,by)) {
				built_everything = false;
			}
		}
	}

	if (first_build) {
		WWDEBUG_SAY(("WorldTerrainSystem: far terrain built, %d x %d blocks, %d vertices against %d near, dropped %.3fm.\r\n",
						 FarBlockCountX,FarBlockCountY,Get_Far_Vertex_Count(),
						 Get_Near_Vertex_Count(),FarTerrainDrop));
	}

	return built_everything;
}


int WorldTerrainSystem::Get_Far_Block_Built_Count(void)
{
	if (FarBlocks == nullptr) {
		return 0;
	}

	int built = 0;
	for (int index = 0; index < FarBlockCountX * FarBlockCountY; index++) {
		if (FarBlocks[index] != nullptr) {
			built++;
		}
	}
	return built;
}


/*
**	Anything the field itself marked dirty is ground the far layer is now wrong about.  Folding
**	the flags in here rather than relying on every editor to call Invalidate_Far_Terrain_Region
**	means a system that shapes the ground through the heightfield still gets a correct far layer.
*/
void WorldTerrainSystem::Fold_Dirty_Patches_Into_Far_Blocks(void)
{
	if ((FarBlocks == nullptr) || !Has_Terrain()) {
		return;
	}

	for (int py = 0; py < Heightfield->Get_Patch_Count_Y(); py++) {
		for (int px = 0; px < Heightfield->Get_Patch_Count_X(); px++) {
			if (Heightfield->Is_Patch_Dirty(px,py)) {
				Invalidate_Far_Block(px / FAR_BLOCK_PATCHES,py / FAR_BLOCK_PATCHES);
			}
		}
	}
}


void WorldTerrainSystem::Update_Far_Terrain(void)
{
	//	Deliberately not a build, for the same reason Update_Collision is not: a caller that
	//	shapes the ground every frame wants what it moved refilled, not a far layer created
	//	underneath it because it called this too early.  The block table on its own is not a
	//	layer -- a field has one from the moment it is created -- so this refills built blocks
	//	and creates none.
	if ((FarBlocks == nullptr) || (Get_Far_Block_Built_Count() == 0)) {
		return;
	}
	if (!Has_Terrain()) {
		Destroy_Far_Terrain();
		return;
	}

	Fold_Dirty_Patches_Into_Far_Blocks();

	//	A change in the drop moves the whole layer, so it is checked once and, if it moved, every
	//	block is refilled rather than leaving neighbours at two different heights.
	float drop = Compute_Far_Terrain_Drop();
	if (drop != FarTerrainDrop) {
		FarTerrainDrop = drop;
		for (int index = 0; index < FarBlockCountX * FarBlockCountY; index++) {
			FarBlockDirty[index] = true;
		}
	}

	for (int by = 0; by < FarBlockCountY; by++) {
		for (int bx = 0; bx < FarBlockCountX; bx++) {
			int index = by * FarBlockCountX + bx;
			if (FarBlockDirty[index] && (FarBlocks[index] != nullptr)) {
				Build_Far_Block(bx,by);
			}
		}
	}
}


void WorldTerrainSystem::Destroy_Far_Terrain(void)
{
	if (FarBlocks == nullptr) {
		FarTerrainDrop = 0.0f;
		return;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	int count = FarBlockCountX * FarBlockCountY;

	for (int index = 0; index < count; index++) {

		StaticPhysClass * phys = FarBlocks[index];
		if (phys == nullptr) {
			continue;
		}

		if (scene != nullptr) {
			scene->Remove_Object(phys);
		}
		phys->Release_Ref();
		FarBlocks[index] = nullptr;
	}

	delete [] FarBlocks;
	delete [] FarBlockDirty;
	delete [] FarBlockLastNear;
	delete [] FarBlockOverlapTimer;
	FarBlocks = nullptr;
	FarBlockDirty = nullptr;
	FarBlockLastNear = nullptr;
	FarBlockOverlapTimer = nullptr;
	FarBlockCountX = 0;
	FarBlockCountY = 0;
	FarTerrainDrop = 0.0f;
	VisibleFarBlockCount = 0;
}


bool WorldTerrainSystem::Has_Far_Terrain(void)
{
	return (FarBlocks != nullptr);
}


void WorldTerrainSystem::Invalidate_Far_Block(int bx,int by)
{
	if (FarBlockDirty == nullptr) {
		return;
	}
	if ((bx < 0) || (by < 0) || (bx >= FarBlockCountX) || (by >= FarBlockCountY)) {
		return;
	}
	FarBlockDirty[by * FarBlockCountX + bx] = true;
}


bool WorldTerrainSystem::Is_Far_Block_Dirty(int bx,int by)
{
	if (FarBlockDirty == nullptr) {
		return false;
	}
	if ((bx < 0) || (by < 0) || (bx >= FarBlockCountX) || (by >= FarBlockCountY)) {
		return false;
	}
	return FarBlockDirty[by * FarBlockCountX + bx];
}


/***********************************************************************************************
 * WorldTerrainSystem::Invalidate_Far_Terrain_Region -- an edit, named as a box                  *
 *                                                                                              *
 * In grid indices rather than by testing block bounding boxes, because a block's box follows    *
 * the ground and the ground is exactly what has just changed: a valley that was deepened is     *
 * outside the box it had a moment ago.  The footprint in X and Y is what is asked about.        *
 *=============================================================================================*/
void WorldTerrainSystem::Invalidate_Far_Terrain_Region(const AABoxClass & region)
{
	if ((FarBlockDirty == nullptr) || !Has_Terrain()) {
		return;
	}

	const Vector3 & origin = Heightfield->Get_Origin();
	float cell = Heightfield->Get_Cell_Size();
	if (!(cell > 0.0f)) {
		return;
	}

	float block_size = cell * (float)(FAR_BLOCK_PATCHES * HeightfieldClass::PATCH_CELLS);

	int bx0 = (int)WWMath::Floor((region.Center.X - region.Extent.X - origin.X) / block_size);
	int bx1 = (int)WWMath::Floor((region.Center.X + region.Extent.X - origin.X) / block_size);
	int by0 = (int)WWMath::Floor((region.Center.Y - region.Extent.Y - origin.Y) / block_size);
	int by1 = (int)WWMath::Floor((region.Center.Y + region.Extent.Y - origin.Y) / block_size);

	if (bx0 < 0) bx0 = 0;
	if (by0 < 0) by0 = 0;
	if (bx1 >= FarBlockCountX) bx1 = FarBlockCountX - 1;
	if (by1 >= FarBlockCountY) by1 = FarBlockCountY - 1;

	for (int by = by0; by <= by1; by++) {
		for (int bx = bx0; bx <= bx1; bx++) {
			FarBlockDirty[by * FarBlockCountX + bx] = true;
		}
	}
}


/*
**	How many blocks the field wants, not how many are built.  Derived rather than stored so that
**	a caller can ask about the layout before anything has been created -- which is the state a
**	device-less check is in, and the state Build_Far_Terrain_Representation is in on its way to
**	sizing its own array.
*/
int WorldTerrainSystem::Get_Far_Block_Count_X(void)
{
	if (!Has_Terrain()) {
		return 0;
	}
	return (Heightfield->Get_Patch_Count_X() + FAR_BLOCK_PATCHES - 1) / FAR_BLOCK_PATCHES;
}


int WorldTerrainSystem::Get_Far_Block_Count_Y(void)
{
	if (!Has_Terrain()) {
		return 0;
	}
	return (Heightfield->Get_Patch_Count_Y() + FAR_BLOCK_PATCHES - 1) / FAR_BLOCK_PATCHES;
}


bool WorldTerrainSystem::Get_Far_Block_Bounds(int bx,int by,AABoxClass * bounds_out)
{
	RenegadeTerrainPatchClass * model = Peek_Far_Block_Model(bx,by);
	if ((model == nullptr) || (bounds_out == nullptr)) {
		return false;
	}
	model->Get_Obj_Space_Bounding_Box(*bounds_out);
	return true;
}


StaticPhysClass * WorldTerrainSystem::Peek_Far_Block(int bx,int by)
{
	if (FarBlocks == nullptr) {
		return nullptr;
	}
	if ((bx < 0) || (by < 0) || (bx >= FarBlockCountX) || (by >= FarBlockCountY)) {
		return nullptr;
	}
	return FarBlocks[by * FarBlockCountX + bx];
}


RenegadeTerrainPatchClass * WorldTerrainSystem::Peek_Far_Block_Model(int bx,int by)
{
	StaticPhysClass * phys = Peek_Far_Block(bx,by);
	if (phys == nullptr) {
		return nullptr;
	}
	return (RenegadeTerrainPatchClass *)phys->Peek_Model();
}


float WorldTerrainSystem::Get_Far_Terrain_Drop(void)
{
	return FarTerrainDrop;
}


int WorldTerrainSystem::Get_Far_Vertex_Count(void)
{
	int total = 0;
	for (int by = 0; by < FarBlockCountY; by++) {
		for (int bx = 0; bx < FarBlockCountX; bx++) {
			RenegadeTerrainPatchClass * model = Peek_Far_Block_Model(bx,by);
			if (model != nullptr) {
				total += model->Get_Vertex_Count();
			}
		}
	}
	return total;
}


int WorldTerrainSystem::Get_Near_Vertex_Count(void)
{
	int total = 0;
	for (int py = 0; py < CollisionPatchCountY; py++) {
		for (int px = 0; px < CollisionPatchCountX; px++) {
			RenegadeTerrainPatchClass * model = Peek_Patch_Model(px,py);
			if (model != nullptr) {
				total += model->Get_Vertex_Count();
			}
		}
	}
	return total;
}


int WorldTerrainSystem::Get_Far_Refill_Count(void)
{
	return FarRefillCount;
}


/***********************************************************************************************
 * WorldTerrainSystem::Update_Terrain_LOD -- which representation draws which ground             *
 *=============================================================================================*/
void WorldTerrainSystem::Update_Terrain_LOD(const Vector3 & viewer_position)
{
	VisibleNearPatchCount = 0;
	VisibleFarBlockCount = 0;

	if (!Has_Terrain() || (CollisionPatches == nullptr)) {
		return;
	}

	//	With no far layer there is nothing to hand ground over to, so every near patch draws --
	//	including any that a previous far layer had hidden before it was destroyed.
	if (FarBlocks == nullptr) {
		for (int py = 0; py < CollisionPatchCountY; py++) {
			for (int px = 0; px < CollisionPatchCountX; px++) {
				RenegadeTerrainPatchClass * model = Peek_Patch_Model(px,py);
				if (model != nullptr) {
					model->Set_Hidden(false);
					VisibleNearPatchCount++;
				}
			}
		}
		return;
	}

	//	Roadmap Section 52A, Phase 47: the two layers already share exact grid vertices at the
	//	seam (see FarTerrainDrop above), so briefly drawing both together during a handover
	//	cannot open a gap -- it can only, for a bounded window, double-draw the same ground.
	//	That is the whole "dissolve": no generic per-object alpha exists on RenderObjClass in
	//	this engine to blend with instead, so an overlap window is what stands in for one, and
	//	it is what turns an instantaneous pop into a spread-out handover.
	const float OVERLAP_SECONDS = 0.35f;
	float dt = WW3D::Get_Frame_Time() * 0.001f;

	for (int by = 0; by < FarBlockCountY; by++) {
		for (int bx = 0; bx < FarBlockCountX; bx++) {

			int px0 = bx * FAR_BLOCK_PATCHES;
			int py0 = by * FAR_BLOCK_PATCHES;
			int px1 = px0 + FAR_BLOCK_PATCHES;
			int py1 = py0 + FAR_BLOCK_PATCHES;
			if (px1 > CollisionPatchCountX) px1 = CollisionPatchCountX;
			if (py1 > CollisionPatchCountY) py1 = CollisionPatchCountY;

			//	One decision for the whole block.  If any patch in it is close enough to want
			//	detail, the block draws near; a block cannot be half one thing and half the other
			//	without drawing two surfaces over the same ground.  A block with nothing built --
			//	the table exists before the geometry does -- has nothing to hand ground to, so it
			//	draws near whatever the distance says.
			RenegadeTerrainPatchClass * far_model = Peek_Far_Block_Model(bx,by);

			bool draw_near = (far_model == nullptr);
			for (int py = py0; (py < py1) && !draw_near; py++) {
				for (int px = px0; px < px1; px++) {
					if (Select_Render_LOD(px,py,viewer_position) != LOD_FAR) {
						draw_near = true;
						break;
					}
				}
			}

			int block_index = by * FarBlockCountX + bx;
			bool overlapping = false;
			if (WW3D::Get_LOD_Dissolve_Enabled()) {
				if (draw_near != FarBlockLastNear[block_index]) {
					FarBlockOverlapTimer[block_index] = OVERLAP_SECONDS;
				} else if (FarBlockOverlapTimer[block_index] > 0.0f) {
					FarBlockOverlapTimer[block_index] -= dt;
					if (FarBlockOverlapTimer[block_index] < 0.0f) FarBlockOverlapTimer[block_index] = 0.0f;
				}
				overlapping = (FarBlockOverlapTimer[block_index] > 0.0f);
			} else {
				FarBlockOverlapTimer[block_index] = 0.0f;
			}
			FarBlockLastNear[block_index] = draw_near;

			for (int py = py0; py < py1; py++) {
				for (int px = px0; px < px1; px++) {
					RenegadeTerrainPatchClass * model = Peek_Patch_Model(px,py);
					if (model != nullptr) {
						bool visible = draw_near || overlapping;
						model->Set_Hidden(!visible);
						if (visible) {
							VisibleNearPatchCount++;
						}
					}
				}
			}

			if (far_model != nullptr) {
				bool visible = !draw_near || overlapping;
				far_model->Set_Hidden(!visible);
				if (visible) {
					VisibleFarBlockCount++;
				}
			}
		}
	}
}


int WorldTerrainSystem::Get_Visible_Near_Patch_Count(void)
{
	return VisibleNearPatchCount;
}


int WorldTerrainSystem::Get_Visible_Far_Block_Count(void)
{
	return VisibleFarBlockCount;
}
