//
//	The terrain service, checked from inside the binary that has to be right.
//	See terrainselfcheck.h for why the checks live here.
//

#include "terrainselfcheck.h"

#include "castres.h"
#include "coltest.h"
#include "heightfield.h"
#include "lineseg.h"
#include "coltype.h"
#include "renegadeterrainpatch.h"
#include "terrainmask.h"
#include "terraintexturesystem.h"
#include "worldterrainsystem.h"
#include "wwmath.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

int	_Failures = 0;

void	Check (bool condition, const char *format, ...)
{
	if (condition) {
		return ;
	}

	va_list args;
	va_start (args, format);
	::fprintf (stderr, "FAIL: ");
	::vfprintf (stderr, format, args);
	::fprintf (stderr, "\n");
	va_end (args);

	_Failures ++;
	return ;
}


bool	Near (float a, float b, float tolerance = 0.001f)
{
	return (WWMath::Fabs (a - b) <= tolerance);
}


/*
**	A field with a known shape: a plane tilted along x, so that every height, every normal and
**	every slope has an answer that can be written down rather than measured.  Height at a
**	vertex is its x index; with a cell size of one that is a forty-five degree ramp.
*/
void	Build_Ramp (HeightfieldClass &field, int vx = 33, int vy = 33, float cell = 1.0f)
{
	Check (field.Create (vx, vy, cell, Vector3 (0.0f, 0.0f, 0.0f)),
			"a %dx%d field would not be created", vx, vy);

	float *heights = new float[vx * vy];
	for (int iy = 0; iy < vy; iy ++) {
		for (int ix = 0; ix < vx; ix ++) {
			heights[iy * vx + ix] = (float)ix * cell;
		}
	}

	Check (field.Set_Heights (heights, vx * vy), "the ramp's heights were refused");
	delete [] heights;
}


/*
**	Drop a ray on a terrain patch render object and report where it landed.
*/
bool	Drop_Ray_On_Model (RenegadeTerrainPatchClass *model, float x, float y, float *z_out)
{
	CastResultStruct result;
	LineSegClass ray (Vector3 (x, y, 100.0f), Vector3 (x, y, -100.0f));
	RayCollisionTestClass raytest (ray, &result, COLLISION_TYPE_PHYSICAL);

	if (!model->Cast_Ray (raytest)) {
		return false;
	}

	*z_out = 100.0f + result.Fraction * (-200.0f);
	return true;
}


/***********************************************************************************************
**	sampling
***********************************************************************************************/
void	Check_Sampling (void)
{
	HeightfieldClass field;
	Build_Ramp (field);

	Check (field.Is_Valid (), "the field is not valid after being built");
	Check (field.Get_Cell_Count_X () == 32, "a 33 vertex field has %d cells, not 32",
			field.Get_Cell_Count_X ());

	//
	//	On a ramp the height is the x coordinate, everywhere, including between vertices and
	//	on both triangles of a cell.  A triangulation that disagreed with itself across the
	//	diagonal would show up here first.
	//
	float height = 0.0f;
	Check (field.Sample_Height (0.0f, 0.0f, &height) && Near (height, 0.0f),
			"the corner of the ramp samples %f, not 0", height);
	Check (field.Sample_Height (4.0f, 7.0f, &height) && Near (height, 4.0f),
			"a vertex on the ramp samples %f, not 4", height);
	Check (field.Sample_Height (4.25f, 7.75f, &height) && Near (height, 4.25f),
			"above the diagonal samples %f, not 4.25", height);
	Check (field.Sample_Height (4.75f, 7.25f, &height) && Near (height, 4.75f),
			"below the diagonal samples %f, not 4.75", height);

	//
	//	Off the field is not "the nearest edge".  A caller standing over the void has to be
	//	able to tell, because the alternative is a soldier walking on air at the boundary.
	//
	Check (field.Sample_Height (-1.0f, 4.0f, &height) == false, "a point west of the field sampled");
	Check (field.Sample_Height (1000.0f, 4.0f, &height) == false, "a point east of the field sampled");
	Check (field.Sample_Height (4.0f, -0.001f, &height) == false, "a point south of the field sampled");

	//
	//	The far edge belongs to the last cell.  It is exactly the kind of boundary that turns
	//	into an invisible one-cell hole in the world when it is got wrong.
	//
	Check (field.Sample_Height (32.0f, 32.0f, &height) && Near (height, 32.0f),
			"the far corner samples %f, not 32", height);

	//
	//	A forty-five degree ramp: the normal leans one to one, and the slope is a quarter turn
	//	of a half turn.
	//
	Vector3 normal;
	Check (field.Sample_Normal (4.25f, 7.75f, &normal), "the ramp has no normal above the diagonal");
	Check (Near (normal.Z, 0.7071f, 0.01f), "the ramp's normal has Z %f, not 0.707", normal.Z);
	//	The ground rises with x, so its normal leans back against the rise.
	Check (normal.X < 0.0f, "the ramp's normal leans with the rise (X %f) instead of against it", normal.X);

	float slope = 0.0f;
	Check (field.Sample_Slope (4.25f, 7.75f, &slope), "the ramp has no slope");
	Check (Near (slope, DEG_TO_RADF (45.0f), 0.01f), "the ramp's slope is %f radians, not 45 degrees", slope);

	//
	//	Bounds cover the whole footprint and the whole height range.
	//
	const AABoxClass &bounds = field.Get_Bounds ();
	Check (Near (bounds.Center.X, 16.0f) && Near (bounds.Extent.X, 16.0f),
			"the field's x bounds are %f +/- %f, not 16 +/- 16", bounds.Center.X, bounds.Extent.X);
	Check (Near (bounds.Center.Z, 16.0f) && Near (bounds.Extent.Z, 16.0f),
			"the ramp's z bounds are %f +/- %f, not 16 +/- 16", bounds.Center.Z, bounds.Extent.Z);
}


/***********************************************************************************************
**	rays
***********************************************************************************************/
void	Check_Rays (void)
{
	HeightfieldClass field;
	Build_Ramp (field);

	//
	//	Straight down onto a known point.  This is the check the whole phase rests on: the
	//	height a ray finds and the height a sample reports are the same height, because they
	//	are the same triangles.  Section 17 asks for exactly this and nothing weaker.
	//
	for (float x = 0.5f; x < 32.0f; x += 3.7f) {
		for (float y = 0.5f; y < 32.0f; y += 5.3f) {

			float sampled = 0.0f;
			Check (field.Sample_Height (x, y, &sampled), "no sample at %f,%f", x, y);

			LineSegClass ray (Vector3 (x, y, 100.0f), Vector3 (x, y, -100.0f));
			float fraction = 0.0f;
			Vector3 normal;

			bool hit = field.Cast_Ray (ray, &fraction, &normal);
			Check (hit, "a downward ray at %f,%f missed the ground", x, y);
			if (hit) {
				float hit_z = 100.0f + fraction * (-200.0f);
				Check (Near (hit_z, sampled, 0.01f),
						"at %f,%f the ray hit %f but the sample says %f", x, y, hit_z, sampled);
			}
		}
	}

	//
	//	A ray that starts under the ground and points up must not be reported as hitting it
	//	from below at the same place a downward ray hits it, and a ray that misses the field
	//	entirely must say so.
	//
	float fraction = 0.0f;
	LineSegClass miss (Vector3 (-50.0f, -50.0f, 10.0f), Vector3 (-50.0f, -50.0f, -10.0f));
	Check (field.Cast_Ray (miss, &fraction, nullptr) == false, "a ray outside the field hit something");

	LineSegClass short_of_it (Vector3 (4.0f, 4.0f, 100.0f), Vector3 (4.0f, 4.0f, 50.0f));
	Check (field.Cast_Ray (short_of_it, &fraction, nullptr) == false,
			"a segment that stops above the ground reported a hit");

	//
	//	A long shallow ray crosses many cells and has to be walked in order, so that what comes
	//	back is the first hit rather than whichever cell happened to be tested first.
	//
	LineSegClass shallow (Vector3 (0.5f, 8.0f, 20.0f), Vector3 (31.5f, 8.0f, 20.0f));
	Vector3 normal;
	bool shallow_hit = field.Cast_Ray (shallow, &fraction, &normal);
	Check (shallow_hit, "a shallow ray across the ramp missed");
	if (shallow_hit) {
		float hit_x = 0.5f + fraction * 31.0f;
		//	The ramp reaches z=20 at x=20, so that is where a level ray at that height meets it.
		Check (Near (hit_x, 20.0f, 0.2f), "the shallow ray first hit at x %f, not 20", hit_x);
	}
}


/***********************************************************************************************
**	shaping
***********************************************************************************************/
void	Check_Shaping (void)
{
	HeightfieldClass field;
	Build_Ramp (field);

	//
	//	Patches start dirty, because nothing has drawn them yet, and stay dirty until whoever
	//	rebuilt them says so.
	//
	Check (field.Get_Patch_Count_X () == 2, "a 32 cell field has %d patches across, not 2",
			field.Get_Patch_Count_X ());
	Check (field.Is_Patch_Dirty (0, 0), "a new patch is not dirty");
	field.Validate_Patch (0, 0);
	Check (field.Is_Patch_Dirty (0, 0) == false, "a validated patch is still dirty");

	//
	//	Moving a vertex dirties the patch it is in.  A vertex on a seam belongs to the patches
	//	on both sides, and both of their edges move with it -- invalidating only one is how a
	//	crack appears down the middle of a level.
	//
	field.Validate_Patch (1, 0);
	field.Set_Height (16, 4, 99.0f);
	Check (field.Is_Patch_Dirty (0, 0), "the patch left of the seam was not dirtied");
	Check (field.Is_Patch_Dirty (1, 0), "the patch right of the seam was not dirtied");

	Check (Near (field.Get_Height (16, 4), 99.0f), "the moved vertex is %f, not 99",
			field.Get_Height (16, 4));

	//
	//	A patch knows how high it goes, which is what lets a ray skip it and a far
	//	representation stand in for it.
	//
	TerrainPatchClass patch;
	Check (field.Get_Patch (0, 0, &patch), "patch 0,0 could not be read");
	Check (Near (patch.MaxHeight, 99.0f), "the patch holding the raised vertex tops out at %f, not 99",
			patch.MaxHeight);

	//
	//	Raising a region raises its middle by the full amount and leaves the rim alone, and the
	//	falloff between them is smooth rather than a cone.
	//
	HeightfieldClass flat;
	Check (flat.Create (65, 65, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)), "a flat field would not be created");

	flat.Modify_Height_Region (Vector3 (32.0f, 32.0f, 0.0f), 8.0f, 10.0f);
	Check (Near (flat.Get_Height (32, 32), 10.0f), "the middle of the raised region is %f, not 10",
			flat.Get_Height (32, 32));
	Check (Near (flat.Get_Height (40, 32), 0.0f), "the rim of the raised region moved to %f",
			flat.Get_Height (40, 32));
	Check (flat.Get_Height (36, 32) > 0.0f && flat.Get_Height (36, 32) < 10.0f,
			"halfway out of the raised region is %f, which is not between 0 and 10",
			flat.Get_Height (36, 32));

	//
	//	A road graded across a slope brings the ground to the road rather than the other way
	//	round, and does it for the whole length rather than in dishes at the control points.
	//
	HeightfieldClass road_field;
	Build_Ramp (road_field);

	Vector3 road[2];
	road[0].Set (4.0f, 16.0f, 20.0f);
	road[1].Set (28.0f, 16.0f, 20.0f);

	//	Straight at the field, since the system's copy is what a caller would use.
	road_field.Blend_Height_Region (road[0], 3.0f, 20.0f, 1.0f);

	float height = 0.0f;
	Check (road_field.Sample_Height (4.0f, 16.0f, &height) && Near (height, 20.0f, 0.5f),
			"the graded point sits at %f, not 20", height);

	//
	//	And the ground a long way from the road is untouched, which is the difference between
	//	grading a road and flattening a level.
	//
	Check (road_field.Sample_Height (10.0f, 16.0f, &height) && Near (height, 10.0f, 0.5f),
			"a point six metres from the graded spot moved to %f, it should still be 10", height);
}


/***********************************************************************************************
**	the service, with and without terrain
***********************************************************************************************/
void	Check_Service (void)
{
	WorldTerrainSystem::Init ();

	//
	//	Every Renegade level ever made has no heightfield, and on those the service answers no
	//	to everything and costs nothing.  A query that returned a height for a level with no
	//	terrain would put a floor under the whole game at z=0.
	//
	Check (WorldTerrainSystem::Has_Terrain () == false, "the service starts with terrain");

	float height = 0.0f;
	Check (WorldTerrainSystem::Sample_Height (0.0f, 0.0f, &height) == false,
			"a level with no terrain reported a height");
	Check (WorldTerrainSystem::Get_Patch_Count_X () == 0, "a level with no terrain has patches");

	AABoxClass bounds;
	Check (WorldTerrainSystem::Get_Bounds (&bounds) == false, "a level with no terrain has bounds");

	//
	//	With terrain, the service is the field.
	//
	Check (WorldTerrainSystem::Create_Terrain (17, 17, 2.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			"the service would not create terrain");
	Check (WorldTerrainSystem::Has_Terrain (), "the service created terrain and does not have it");
	Check (WorldTerrainSystem::Sample_Height (8.0f, 8.0f, &height) && Near (height, 0.0f),
			"flat terrain samples %f, not 0", height);

	//
	//	Placement over flat ground is flat, and over the edge is not on terrain at all.
	//
	TerrainPlacementInfoClass info;
	Check (WorldTerrainSystem::Query_Placement_Surface (Vector3 (16.0f, 16.0f, 0.0f), 4.0f, &info),
			"a placement query over the middle failed");
	Check (info.IsOnTerrain, "the middle of the field is not on terrain");
	Check (Near (info.MaxSlope, 0.0f, 0.01f), "flat ground has a slope of %f", info.MaxSlope);
	Check (Near (info.MaxHeight - info.MinHeight, 0.0f), "flat ground has a height range of %f",
			info.MaxHeight - info.MinHeight);

	Check (WorldTerrainSystem::Query_Placement_Surface (Vector3 (0.0f, 0.0f, 0.0f), 4.0f, &info),
			"a placement query at the corner failed outright");
	Check (info.IsOnTerrain == false, "a footprint hanging off the corner claims to be on terrain");

	//
	//	Collision is real now, but it needs somewhere to put itself.  There is no physics scene
	//	in a check that runs before the game is initialised, so the answer is no -- and it has to
	//	be a returned no rather than a crash, because that is also what a dedicated server or a
	//	tool that never made a scene will get.
	//
	Check (WorldTerrainSystem::Build_Collision () == false,
			"collision was built with no physics scene to build it in");
	Check (WorldTerrainSystem::Has_Collision () == false, "a failed build left collision behind");
	Check (WorldTerrainSystem::Peek_Collision_Patch (0, 0) == nullptr,
			"there is a collision patch with no collision");

	//
	//	The far layer is still declared and still refuses; it is Section 34's own phase.
	//
	Check (WorldTerrainSystem::Build_Far_Terrain_Representation () == false,
			"far terrain claims to be built");

	WorldTerrainSystem::Shutdown ();
	Check (WorldTerrainSystem::Has_Terrain () == false, "terrain outlived the service");
}

/***********************************************************************************************
**	collision
***********************************************************************************************/
void	Check_Collision (void)
{
	WorldTerrainSystem::Init ();

	//
	//	The whole point of building collision out of the engine's own terrain patch instead of a
	//	new mesh type is that the patch splits a cell along the same diagonal the heightfield
	//	does.  If that were ever untrue, a soldier would collide with a surface a few centimetres
	//	away from the one the sampling functions describe, and nothing would say so.
	//
	//	So: fill a patch from a field with a bump in every cell, drop rays on it, and require the
	//	geometry and the samples to agree.
	//
	Check (WorldTerrainSystem::Create_Terrain (33, 33, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			"the service would not create terrain to build collision from");

	//
	//	A different height at every corner of every cell.  The ramp cannot be used here: it is
	//	planar, and both diagonals of a planar quad give the same answer everywhere, so a field
	//	with no plane in it is what makes a wrong diagonal show up.
	//
	{
		float *heights = new float[33 * 33];
		for (int iy = 0; iy < 33; iy ++) {
			for (int ix = 0; ix < 33; ix ++) {
				heights[iy * 33 + ix] = (float)(((ix * 7) + (iy * 13)) % 5);
			}
		}
		Check (WorldTerrainSystem::Set_Heights (heights, 33 * 33), "the bumpy heights were refused");
		delete [] heights;
	}

	//
	//	Thirty-two cells is two patches of sixteen, each seventeen vertices across because the
	//	two share the grid line between them.
	//
	Check (WorldTerrainSystem::Get_Patch_Count_X () == 2, "a 32 cell field is %d patches across",
			WorldTerrainSystem::Get_Patch_Count_X ());

	RenegadeTerrainPatchClass *patch = WorldTerrainSystem::Create_Patch_Model (0, 0);
	Check (patch != nullptr, "the first patch would not build");

	if (patch != nullptr) {

		Check (patch->Get_Vertex_Count () == 17 * 17, "a 16 cell patch has %d vertices, not 289",
				patch->Get_Vertex_Count ());

		//
		//	The patch works out which cell a point falls in from where its own footprint starts,
		//	so a footprint that does not match where the vertices actually are sends every query
		//	to the wrong cell.  Only the centre is checked: the extent a terrain patch reports is
		//	stock Renegade's, and it is deliberately about twice the real size plus a metre --
		//	loose is safe for culling, and it is not this phase's to tighten.
		//
		AABoxClass box;
		patch->Get_Obj_Space_Bounding_Box (box);
		Check (Near (box.Center.X, 8.0f, 0.01f), "the first patch is centred at x=%f, not 8",
				box.Center.X);
		Check (Near (box.Center.Y, 8.0f, 0.01f), "the first patch is centred at y=%f, not 8",
				box.Center.Y);

		//
		//	Two points per cell, one either side of the diagonal, over a block of cells.
		//
		for (int cy = 0; cy < 8; cy ++) {
			for (int cx = 0; cx < 8; cx ++) {

				float points[2][2] = { { 0.7f, 0.3f }, { 0.3f, 0.7f } };

				for (int which = 0; which < 2; which ++) {

					float x = (float)cx + points[which][0];
					float y = (float)cy + points[which][1];

					float sampled = 0.0f;
					Check (WorldTerrainSystem::Sample_Height (x, y, &sampled),
							"the field has no height at %f,%f", x, y);

					float hit_z = 0.0f;
					bool hit = Drop_Ray_On_Model (patch, x, y, &hit_z);
					Check (hit, "a ray dropped at %f,%f missed the collision patch", x, y);

					if (hit) {
						Check (Near (hit_z, sampled, 0.01f),
								"at %f,%f collision says %f and sampling says %f",
								x, y, hit_z, sampled);
					}
				}
			}
		}

		patch->Release_Ref ();
	}

	//
	//	The seam.  Two patches share a grid line, and both have to put it in the same place --
	//	a disagreement there is a strip of level you fall through.
	//
	RenegadeTerrainPatchClass *left	= WorldTerrainSystem::Create_Patch_Model (0, 0);
	RenegadeTerrainPatchClass *right	= WorldTerrainSystem::Create_Patch_Model (1, 0);
	Check ((left != nullptr) && (right != nullptr), "the patches either side of the seam would not build");

	if ((left != nullptr) && (right != nullptr)) {

		for (int step = 1; step < 8; step ++) {

			float y = (float)step + 0.5f;
			float left_z = 0.0f;
			float right_z = 0.0f;

			bool left_hit		= Drop_Ray_On_Model (left, 16.0f, y, &left_z);
			bool right_hit		= Drop_Ray_On_Model (right, 16.0f, y, &right_z);

			Check (left_hit && right_hit, "the seam at y=%f was missed by one of its patches", y);
			if (left_hit && right_hit) {
				Check (Near (left_z, right_z, 0.01f),
						"the seam at y=%f is at %f on one side and %f on the other",
						y, left_z, right_z);
			}
		}

		left->Release_Ref ();
		right->Release_Ref ();
	}

	//
	//	A field that is not a whole number of patches across still covers all of itself: the
	//	last patch is short rather than missing, and rounding it up would build geometry over
	//	ground that does not exist.
	//
	WorldTerrainSystem::Destroy_Terrain ();
	Check (WorldTerrainSystem::Create_Terrain (20, 20, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			"a 20 vertex field would not be created");
	Check (WorldTerrainSystem::Get_Patch_Count_X () == 2, "a 19 cell field is %d patches across",
			WorldTerrainSystem::Get_Patch_Count_X ());

	RenegadeTerrainPatchClass *last = WorldTerrainSystem::Create_Patch_Model (1, 1);
	Check (last != nullptr, "the short last patch would not build");
	if (last != nullptr) {
		//	Vertices 16 through 19: four across, not seventeen.
		Check (last->Get_Vertex_Count () == 4 * 4, "the short last patch has %d vertices, not 16",
				last->Get_Vertex_Count ());
		last->Release_Ref ();
	}

	Check (WorldTerrainSystem::Create_Patch_Model (2, 0) == nullptr,
			"a patch past the end of the field was built anyway");

	//
	//	Vertex normals, which is what the ground will be lit by.  On a forty-five degree ramp
	//	the gradient normal is the face normal, so the two can be compared against each other.
	//
	HeightfieldClass ramp;
	Build_Ramp (ramp);

	Vector3 normal = ramp.Compute_Vertex_Normal (8, 8);
	Check (Near (normal.Z, 0.7071f, 0.01f), "the ramp's vertex normal has z=%f, not 0.707", normal.Z);
	Check (normal.X < 0.0f, "the ramp rises with x, so its normal must lean back, not to %f", normal.X);
	Check (Near (normal.Y, 0.0f, 0.01f), "a ramp along x has a vertex normal with y=%f", normal.Y);

	Vector3 sampled_normal;
	Check (ramp.Sample_Normal (8.3f, 8.3f, &sampled_normal), "the ramp has no normal at 8.3,8.3");
	Check (Near (normal.X, sampled_normal.X, 0.01f) && Near (normal.Z, sampled_normal.Z, 0.01f),
			"on a plane the vertex normal (%f,%f,%f) and the face normal (%f,%f,%f) must agree",
			normal.X, normal.Y, normal.Z,
			sampled_normal.X, sampled_normal.Y, sampled_normal.Z);

	//
	//	A vertex on the border has half its neighbours clipped away, and a gradient measured over
	//	the wrong distance would tilt the whole edge of the field.
	//
	Vector3 edge = ramp.Compute_Vertex_Normal (0, 8);
	Check (Near (edge.Z, 0.7071f, 0.01f), "the ramp's edge normal has z=%f, not 0.707", edge.Z);

	WorldTerrainSystem::Shutdown ();
	Check (WorldTerrainSystem::Has_Collision () == false, "collision outlived the service");
}

/***********************************************************************************************
**	textures
***********************************************************************************************/

/*
**	A field with a step in it: flat, then a wall, then flat again.
**
**	The two grid lines either side of the wall have the same slope beside them and opposite
**	curvature -- the lower one is the inside of a corner, the upper one is the lip.  That pair is
**	what makes curvature worth having as a rule input at all, so it is what the layer rules are
**	checked against.
*/
void	Build_Step (int size = 33, float cell = 1.0f, float step_height = 10.0f)
{
	Check (WorldTerrainSystem::Create_Terrain (size, size, cell, Vector3 (0.0f, 0.0f, 0.0f)),
			"a %dx%d stepped field would not be created", size, size);

	float *heights = new float[size * size];
	for (int iy = 0; iy < size; iy ++) {
		for (int ix = 0; ix < size; ix ++) {
			heights[iy * size + ix] = (ix >= (size / 2)) ? step_height : 0.0f;
		}
	}

	Check (WorldTerrainSystem::Set_Heights (heights, size * size), "the step's heights were refused");
	delete [] heights;
}


float	Layer_Weight (int ix, int iy, const char *name)
{
	int index = TerrainTextureSystem::Find_Layer (name);
	if (index < 0) {
		Check (false, "there is no layer called '%s'", name);
		return 0.0f;
	}

	float weights[TerrainTextureSystem::MAX_LAYERS];
	int count = TerrainTextureSystem::Compute_Weights (ix, iy, weights,
																		TerrainTextureSystem::MAX_LAYERS);
	if (index >= count) {
		return 0.0f;
	}
	return weights[index];
}


void	Check_Textures (void)
{
	WorldTerrainSystem::Init ();

	//
	//	Masks are the same shape as the field, so that a rule can ask about height and about the
	//	road at the same grid point without converting between two ideas of where a point is.
	//
	Build_Step ();
	Check (TerrainTextureSystem::Create_Masks (), "the masks would not be created");
	Check (TerrainTextureSystem::Has_Masks (), "the masks were created and are not there");

	TerrainMaskClass *road = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_ROAD);
	Check (road != nullptr, "there is no road mask");

	if (road != nullptr) {

		Check (road->Get_Vertex_Count_X () == 33, "the road mask is %d wide, the field is 33",
				road->Get_Vertex_Count_X ());
		Check (Near (road->Get (10, 10), 0.0f), "a mask nobody wrote to is not empty");

		//
		//	A road is a polyline stamped into a mask.  It has to be continuous along its length:
		//	discs dropped too far apart leave gaps, and a road with gaps in it is a dashed line.
		//
		Vector3 line[2];
		line[0].Set (4.0f, 16.0f, 0.0f);
		line[1].Set (12.0f, 16.0f, 0.0f);
		road->Stamp_Polyline (line, 2, 4.0f, 1.0f, 1.0f);

		for (int along = 4; along <= 12; along ++) {
			Check (road->Get (along, 16) > 0.9f, "the road is only %f at x=%d",
					road->Get (along, 16), along);
		}

		Check (Near (road->Get (25, 16), 0.0f, 0.01f), "the road reached x=25, which is nowhere near it");
	}

	//
	//	Water distance is derived from the river mask rather than written.  A column of river at
	//	x=8 puts every point its own distance away, and saturates rather than growing forever.
	//
	TerrainMaskClass *river = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_RIVER);
	TerrainMaskClass *water = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_WATER_DISTANCE);
	Check ((river != nullptr) && (water != nullptr), "the water masks are missing");

	if ((river != nullptr) && (water != nullptr)) {

		for (int iy = 0; iy < 33; iy ++) {
			river->Set (8, iy, 1.0f);
		}

		Check (TerrainTextureSystem::Update_Water_Distance (0.5f, 20.0f),
				"the water distance field would not build");

		Check (Near (water->Get (8, 16), 0.0f), "the river is %f from itself", water->Get (8, 16));
		Check (Near (water->Get (11, 16), 3.0f, 0.01f), "three cells from the river reads %f",
				water->Get (11, 16));
		Check (Near (water->Get (30, 16), 20.0f, 0.01f),
				"far from the river reads %f, not the 20 it saturates at", water->Get (30, 16));
	}

	//
	//	The default layer table, and the thing it is for: coherent materials with nobody having
	//	painted anything.
	//
	TerrainTextureSystem::Define_Default_Layers ();
	Check (TerrainTextureSystem::Get_Layer_Count () > 0, "the default layers defined nothing");
	Check (TerrainTextureSystem::Find_Layer ("ground") == 0, "the ground is not the first layer");

	//
	//	The base is always fully there.  If its weight fell wherever another layer was strong,
	//	two weak layers at the same point would leave a hole in the world.
	//
	Check (Near (Layer_Weight (4, 4, "ground"), 1.0f), "the base layer is %f on flat ground",
			Layer_Weight (4, 4, "ground"));
	Check (Near (Layer_Weight (16, 16, "ground"), 1.0f), "the base layer is %f on the cliff",
			Layer_Weight (16, 16, "ground"));

	//
	//	Flat ground is ground: nothing steep, nothing curved, no mask written there.
	//
	Check (Near (Layer_Weight (4, 4, "rock"), 0.0f, 0.01f), "flat ground is %f rock",
			Layer_Weight (4, 4, "rock"));
	Check (Near (Layer_Weight (4, 4, "cliff"), 0.0f, 0.01f), "flat ground is %f cliff",
			Layer_Weight (4, 4, "cliff"));

	//
	//	The pair that curvature exists for.  x=15 and x=16 sit either side of the wall and have
	//	the same slope; only their curvature differs, and only the lip is a cliff face.
	//
	Check (Layer_Weight (15, 4, "rock") > 0.9f, "the foot of the wall is only %f rock",
			Layer_Weight (15, 4, "rock"));
	Check (Layer_Weight (16, 4, "rock") > 0.9f, "the lip of the wall is only %f rock",
			Layer_Weight (16, 4, "rock"));

	Check (Near (Layer_Weight (15, 4, "cliff"), 0.0f, 0.01f),
			"the foot of the wall is %f cliff face, and it curves the other way",
			Layer_Weight (15, 4, "cliff"));
	Check (Layer_Weight (16, 4, "cliff") > 0.9f, "the lip of the wall is only %f cliff face",
			Layer_Weight (16, 4, "cliff"));

	//
	//	An exclusive layer pushes the others aside in proportion to how strongly it applies.  A
	//	road drawn over a slope is a road, not a road with a hill showing through it.
	//
	if (road != nullptr) {
		Vector3 over_the_lip[2];
		over_the_lip[0].Set (16.0f, 2.0f, 0.0f);
		over_the_lip[1].Set (16.0f, 8.0f, 0.0f);
		road->Stamp_Polyline (over_the_lip, 2, 4.0f, 1.0f, 1.0f);

		Check (Layer_Weight (16, 4, "road") > 0.9f, "the road over the lip is only %f",
				Layer_Weight (16, 4, "road"));
		Check (Near (Layer_Weight (16, 4, "rock"), 0.0f, 0.05f),
				"the road let %f of the rock through", Layer_Weight (16, 4, "rock"));
		Check (Near (Layer_Weight (16, 4, "ground"), 1.0f),
				"the road took the ground out from under itself");

		Check (TerrainTextureSystem::Find_Layer ("road") ==
				 TerrainTextureSystem::Get_Dominant_Layer (16, 4),
				"the dominant layer on the road is not the road");
	}

	//
	//	Four metres from the river is shoreline, and twenty is not.  Which is the point of a
	//	derived distance: nobody drew a beach, they drew a river, and the beach follows.
	//
	Check (TerrainTextureSystem::Find_Layer ("shore") ==
			 TerrainTextureSystem::Get_Dominant_Layer (4, 4),
			"four metres from the river is not shoreline");

	Check (TerrainTextureSystem::Get_Dominant_Layer (28, 28) == 0,
			"flat ground far from everything is not the base");

	//
	//	Determinism.  There is no random number generator below this line: variation is a hash of
	//	where you are asking and a seed, so a server and a client agree without sending anything.
	//
	Check (Near (TerrainMaskClass::Hash_Value (17, 23, 5),
					 TerrainMaskClass::Hash_Value (17, 23, 5)),
			"the same hash gave two answers");
	Check (!Near (TerrainMaskClass::Hash_Value (17, 23, 5),
					  TerrainMaskClass::Hash_Value (17, 23, 6)),
			"two seeds gave the same answer");

	float first[TerrainTextureSystem::MAX_LAYERS];
	float again[TerrainTextureSystem::MAX_LAYERS];
	int count = TerrainTextureSystem::Compute_Weights (9, 9, first, TerrainTextureSystem::MAX_LAYERS);
	TerrainTextureSystem::Compute_Weights (9, 9, again, TerrainTextureSystem::MAX_LAYERS);
	for (int index = 0; index < count; index ++) {
		Check (Near (first[index], again[index]), "layer %d weighed %f and then %f",
				index, first[index], again[index]);
	}

	//
	//	Off the field there is no answer, the same as everywhere else in this service.
	//
	int layer = -1;
	Check (TerrainTextureSystem::Get_Dominant_Layer_At (-50.0f, -50.0f, &layer) == false,
			"somewhere off the field is made of something");

	//
	//	And the passes themselves: weights become the thing the renderer reads.  The composite is
	//	stock Renegade's -- first layer with any influence draws the quad opaque, later ones draw
	//	over it -- so a patch built from rules is a patch the renderer already knows how to read.
	//
	RenegadeTerrainPatchClass *patch = WorldTerrainSystem::Create_Patch_Model (0, 0);
	Check (patch != nullptr, "a patch to dress would not build");

	if (patch != nullptr) {

		Check (TerrainTextureSystem::Build_Patch_Materials (patch, 0, 0),
				"the patch would not take its materials");
		Check (patch->Get_Material_Count () > 0, "the dressed patch has no material passes");

		RenegadeTerrainMaterialPassClass *base_pass = patch->Peek_Material_Pass (0);
		Check (base_pass != nullptr, "there is no pass for the base layer");
		if (base_pass != nullptr) {
			Check (base_pass->QuadList[RenegadeTerrainMaterialPassClass::PASS_BASE].Count () > 0,
					"the base layer draws no quads opaquely, so the ground has a hole in it");
		}

		int rock_index = TerrainTextureSystem::Find_Layer ("rock");
		if ((rock_index > 0) && (rock_index < patch->Get_Material_Count ())) {
			RenegadeTerrainMaterialPassClass *rock_pass = patch->Peek_Material_Pass (rock_index);
			Check (rock_pass->QuadList[RenegadeTerrainMaterialPassClass::PASS_ALPHA].Count () > 0,
					"the rock layer blends over nothing, and this patch has a wall in it");
		}

		patch->Release_Ref ();
	}

	//
	//	The masks belong to the field they describe, so a new field of a different size must not
	//	inherit the old one's road.
	//
	WorldTerrainSystem::Destroy_Terrain ();
	Check (TerrainTextureSystem::Has_Masks () == false, "the masks outlived the ground");

	WorldTerrainSystem::Shutdown ();
	Check (TerrainTextureSystem::Get_Layer_Count () == 0, "the layers outlived the service");
}

}	// anonymous namespace


int	TerrainSelfCheck::Run (const char *which)
{
	_Failures = 0;

	if ((which == nullptr) || (::strcmp (which, "sampling") == 0)) {
		Check_Sampling ();
	}
	if ((which == nullptr) || (::strcmp (which, "rays") == 0)) {
		Check_Rays ();
	}
	if ((which == nullptr) || (::strcmp (which, "shaping") == 0)) {
		Check_Shaping ();
	}
	if ((which == nullptr) || (::strcmp (which, "service") == 0)) {
		Check_Service ();
	}
	if ((which == nullptr) || (::strcmp (which, "collision") == 0)) {
		Check_Collision ();
	}
	if ((which == nullptr) || (::strcmp (which, "textures") == 0)) {
		Check_Textures ();
	}

	if (_Failures == 0) {
		::fprintf (stdout, "terrain self check: pass\n");
	} else {
		::fprintf (stdout, "terrain self check: %d failure(s)\n", _Failures);
	}

	return (_Failures == 0) ? 0 : 1;
}
