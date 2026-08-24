//
//	The terrain service, checked from inside the binary that has to be right.
//	See terrainselfcheck.h for why the checks live here.
//

#include "terrainselfcheck.h"

#include "bridgesection.h"
#include "bridgesystem.h"
#include "castres.h"
#include "foliagesystem.h"
#include "foliagetype.h"
#include "coltest.h"
#include "heightfield.h"
#include "lineseg.h"
#include "coltype.h"
#include "renegadeterrainpatch.h"
#include "ribbontype.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "surfacemarktype.h"
#include "surfaceribbonsystem.h"
#include "terrainmask.h"
#include "terraintexturesystem.h"
#include "watersystem.h"
#include "watertype.h"
#include "w3d_file.h"
#include "worldsurfacemarkmanager.h"
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


/***********************************************************************************************
**	roads
***********************************************************************************************/

/*
**	A field with a bump every few metres, deterministic and rough enough that a smoothed road
**	over it is visibly not the same line as the ground under it.
*/
void	Build_Bumpy (int vx = 65, float cell = 1.0f)
{
	Check (WorldTerrainSystem::Create_Terrain (vx, vx, cell, Vector3 (0.0f, 0.0f, 0.0f)),
			"a %dx%d field would not be created", vx, vx);

	float *heights = new float[vx * vx];
	for (int iy = 0; iy < vx; iy ++) {
		for (int ix = 0; ix < vx; ix ++) {
			heights[iy * vx + ix] = (float)(((ix * 7) + (iy * 3)) % 5) * 0.4f;
		}
	}

	Check (WorldTerrainSystem::Set_Heights (heights, vx * vx), "the bumpy heights were refused");
	delete [] heights;
}


int	Add_Straight_Road (float x0, float y0, float x1, float y1, float width,
								const char *material, int grade = ROAD_GRADE_NONE)
{
	RoadSplineClass road;
	Vector3 points[2];
	points[0].Set (x0, y0, 0.0f);
	points[1].Set (x1, y1, 0.0f);

	road.Set_Control_Points (points, 2);
	road.Set_Width (width);
	road.Set_Shoulder_Width (width * 0.5f);
	road.Set_Material (material);
	road.Set_Grade_Policy (grade);

	return RoadSystem::Add_Road (road);
}


/*
**	Is any point of any surface run inside a junction?  That is the whole no-overlap claim: two
**	roads crossing must not both put a triangle on the same piece of ground.
*/
bool	Any_Run_Point_In_A_Junction (void)
{
	for (int run = 0; run < RoadSystem::Get_Surface_Run_Count (); run ++) {

		int count = 0;
		RoadSystem::Get_Surface_Run (run, nullptr, &count);

		for (int n = 0; n < count; n ++) {

			const RoadPointClass *point = RoadSystem::Peek_Surface_Run_Point (run, n);
			if (point == nullptr) {
				continue;
			}

			for (int j = 0; j < RoadSystem::Get_Junction_Count (); j ++) {
				const RoadJunctionClass *junction = RoadSystem::Peek_Junction (j);
				float dx = point->Position.X - junction->Position.X;
				float dy = point->Position.Y - junction->Position.Y;
				//	A hair inside is where the boundary points land by construction, so the test
				//	is whether a point is meaningfully inside rather than exactly on the edge.
				if (((dx * dx) + (dy * dy)) < ((junction->Radius - 0.05f) * (junction->Radius - 0.05f))) {
					return true;
				}
			}
		}
	}

	return false;
}


void	Check_Roads (void)
{
	WorldTerrainSystem::Init ();
	RoadSystem::Init ();

	//
	//	The curve passes through its control points.  A road is drawn by saying where it goes,
	//	and a curve that merely approached those places would put the road somewhere nobody
	//	asked for -- beside the bridge rather than on it.
	//
	{
		RoadSplineClass road;
		Vector3 points[3];
		points[0].Set (0.0f, 0.0f, 0.0f);
		points[1].Set (10.0f, 6.0f, 0.0f);
		points[2].Set (20.0f, 0.0f, 0.0f);
		road.Set_Control_Points (points, 3);

		Vector3 middle = road.Evaluate (0, 1.0f);
		Check (Near (middle.X, 10.0f) && Near (middle.Y, 6.0f),
				"the curve misses its own control point, at %f,%f", middle.X, middle.Y);

		//
		//	Subdivision is driven by how far the chord wanders from the curve.  A straight has
		//	nowhere to wander to, so with only the deviation test in play it stays two points;
		//	the same settings on a bend spend many.  That is the difference between adaptive
		//	tessellation and a fixed step, and it is why a long straight road is cheap.
		//
		Check (road.Tessellate (0.05f, 0.25f, 1000000.0f), "the bend would not tessellate");
		int bend_points = road.Get_Point_Count ();
		Check (bend_points > 8, "a bend tessellated to only %d points", bend_points);

		RoadSplineClass straight;
		Vector3 line[2];
		line[0].Set (0.0f, 0.0f, 0.0f);
		line[1].Set (20.0f, 0.0f, 0.0f);
		straight.Set_Control_Points (line, 2);
		Check (straight.Tessellate (0.05f, 0.25f, 1000000.0f), "the straight would not tessellate");
		Check (straight.Get_Point_Count () == 2, "a straight spent %d points on being straight",
				straight.Get_Point_Count ());

		//
		//	The other reason to subdivide is length: a straight road still needs points, because
		//	the ground under it is not straight and there is nothing to follow between two points
		//	that are twenty metres apart.
		//
		Check (straight.Tessellate (0.05f, 0.25f, 4.0f), "the straight would not re-tessellate");
		for (int i = 1; i < straight.Get_Point_Count (); i ++) {
			float span = straight.Get_Point (i).Distance - straight.Get_Point (i-1).Distance;
			Check (span <= 4.001f, "a step of %f exceeds the four metres asked for", span);
		}
		Check (Near (straight.Get_Length (), 20.0f, 0.01f), "a twenty metre road measured %f",
				straight.Get_Length ());

		//
		//	The frame.  Side is horizontal and to the left, because the width of a road is a
		//	width on the map: measured up the slope it would be narrower in plan than it is.
		//
		const RoadPointClass &point = straight.Get_Point (1);
		Check (Near (point.Tangent.X, 1.0f, 0.01f), "the tangent of an eastward road is %f east",
				point.Tangent.X);
		Check (Near (point.Side.Y, 1.0f, 0.01f), "the side of an eastward road is %f north",
				point.Side.Y);
		Check (Near (point.Side.Z, 0.0f, 0.001f), "the side vector leans %f out of the ground plane",
				point.Side.Z);

		//
		//	Trimming cuts at the distance asked for rather than at the nearest existing point.
		//	A junction and a bridge both need the road to stop exactly, or there is an overlap
		//	that fights for depth or a gap that shows the ground through.
		//
		Check (straight.Trim_Start (5.0f), "a twenty metre road would not give up five");
		Check (Near (straight.Get_Length (), 15.0f, 0.01f), "after trimming five it is %f long",
				straight.Get_Length ());
		Check (Near (straight.Get_Point (0).Position.X, 5.0f, 0.01f),
				"the trimmed road starts at x=%f, not 5", straight.Get_Point (0).Position.X);

		Check (straight.Trim_End (5.0f), "the road would not give up five more");
		Check (Near (straight.Get_Length (), 10.0f, 0.01f), "after trimming both ends it is %f long",
				straight.Get_Length ());

		//
		//	Where a point is relative to the road, measured in plan.
		//
		int index = 0;
		float lateral = 0.0f;
		float along = 0.0f;
		Check (straight.Find_Closest_Point (Vector3 (10.0f, 3.0f, 0.0f), &index, &lateral, &along),
				"the road could not say where a point beside it was");
		Check (Near (lateral, 3.0f, 0.01f), "three metres to the side measured %f", lateral);
		Check (Near (along, 5.0f, 0.01f), "halfway along the trimmed road measured %f", along);
	}

	//
	//	A crossroads.  One junction, two arms, and each road cut into two stretches with a hole
	//	between them -- neither road is "the one that ends", and a crossroads costs what a tee
	//	costs.
	//
	RoadSystem::Clear_Roads ();
	Add_Straight_Road (0.0f, 0.0f, 40.0f, 0.0f, 8.0f, "road_a");
	Add_Straight_Road (20.0f, -20.0f, 20.0f, 20.0f, 8.0f, "road_a");
	Check (RoadSystem::Build_Network (0.25f, 0.5f, 4.0f), "the crossroads network would not build");

	Check (RoadSystem::Get_Junction_Count () == 1, "a crossroads made %d junctions",
			RoadSystem::Get_Junction_Count ());

	if (RoadSystem::Get_Junction_Count () == 1) {
		const RoadJunctionClass *junction = RoadSystem::Peek_Junction (0);
		Check (junction->ArmCount == 2, "the crossroads has %d arms", junction->ArmCount);
		Check (Near (junction->Position.X, 20.0f, 0.5f) && Near (junction->Position.Y, 0.0f, 0.5f),
				"the crossroads is at %f,%f", junction->Position.X, junction->Position.Y);
		Check (junction->Radius > 4.0f, "the junction radius %f does not cover the crossing",
				junction->Radius);
	}

	RoadSystem::Collect_Surface_Runs ();
	Check (RoadSystem::Get_Surface_Run_Count () == 4,
			"a crossroads left %d stretches of surface, not four",
			RoadSystem::Get_Surface_Run_Count ());
	Check (RoadSystem::Get_Surface_Boundary_Count () == 4,
			"a crossroads has %d road ends meeting it, not four",
			RoadSystem::Get_Surface_Boundary_Count ());
	Check (Any_Run_Point_In_A_Junction () == false,
			"road surface is being laid inside a junction, where the junction also lays some");

	//
	//	A tee.  The through road is still cut in two; the road that ends contributes one
	//	stretch and one end.  Nothing about the code distinguishes this case from the last one.
	//
	RoadSystem::Clear_Roads ();
	Add_Straight_Road (0.0f, 0.0f, 40.0f, 0.0f, 8.0f, "road_a");
	Add_Straight_Road (20.0f, 0.0f, 20.0f, 20.0f, 8.0f, "road_a");
	Check (RoadSystem::Build_Network (0.25f, 0.5f, 4.0f), "the tee network would not build");

	Check (RoadSystem::Get_Junction_Count () == 1, "a tee made %d junctions",
			RoadSystem::Get_Junction_Count ());

	RoadSystem::Collect_Surface_Runs ();
	Check (RoadSystem::Get_Surface_Run_Count () == 3,
			"a tee left %d stretches of surface, not three", RoadSystem::Get_Surface_Run_Count ());
	Check (Any_Run_Point_In_A_Junction () == false, "the tee lays surface inside its own junction");

	//
	//	Three roads meeting at one place is one junction with three arms, not three junctions
	//	overlapping.  Pairwise crossings found within a disc of each other are the same crossing.
	//
	RoadSystem::Clear_Roads ();
	Add_Straight_Road (0.0f, 0.0f, 40.0f, 0.0f, 8.0f, "road_a");
	Add_Straight_Road (20.0f, -20.0f, 20.0f, 20.0f, 8.0f, "road_a");
	Add_Straight_Road (20.0f, 0.0f, 40.0f, 20.0f, 8.0f, "road_a");
	Check (RoadSystem::Build_Network (0.25f, 0.5f, 4.0f), "the three way network would not build");

	Check (RoadSystem::Get_Junction_Count () == 1, "three roads at one place made %d junctions",
			RoadSystem::Get_Junction_Count ());
	if (RoadSystem::Get_Junction_Count () == 1) {
		Check (RoadSystem::Peek_Junction (0)->ArmCount == 3, "the three way has %d arms",
				RoadSystem::Peek_Junction (0)->ArmCount);
	}

	//
	//	Two roads that never meet do not make a junction just for being on the same map.
	//
	RoadSystem::Clear_Roads ();
	Add_Straight_Road (0.0f, 0.0f, 40.0f, 0.0f, 8.0f, "road_a");
	Add_Straight_Road (0.0f, 60.0f, 40.0f, 60.0f, 8.0f, "road_a");
	Check (RoadSystem::Build_Network (0.25f, 0.5f, 4.0f), "two parallel roads would not build");
	Check (RoadSystem::Get_Junction_Count () == 0, "two roads sixty metres apart made %d junctions",
			RoadSystem::Get_Junction_Count ());

	//
	//	Navigation.  Section 19 asks for path metadata; what pathing actually wants to know is
	//	that there is a road here, which way it runs, and whether you are on it.
	//
	RoadNavInfoClass nav;
	Check (RoadSystem::Find_Nearest_Road (Vector3 (10.0f, 3.0f, 0.0f), 50.0f, &nav),
			"a point three metres off a road found no road");
	Check (nav.RoadID == 0, "the nearer road is %d", nav.RoadID);
	Check (nav.IsOnRoad, "three metres from the middle of an eight metre road is off it");
	Check (Near (nav.Tangent.X, 1.0f, 0.01f), "the road runs %f east where it runs east",
			nav.Tangent.X);

	Check (RoadSystem::Is_On_Road (Vector3 (10.0f, 30.0f, 0.0f)) == false,
			"a point thirty metres from every road is on one");
	Check (RoadSystem::Find_Nearest_Road (Vector3 (10.0f, 200.0f, 0.0f), 20.0f, &nav) == false,
			"a road was found two hundred metres away with a twenty metre limit");

	//
	//	Bridge handover.  Section 20 does not exist yet; what can be settled now is the seam.
	//	The road records where it stopped and which way it was pointing, and the bridge that
	//	arrives later reads that rather than guessing.
	//
	RoadSystem::Clear_Roads ();
	{
		int id = Add_Straight_Road (0.0f, 0.0f, 40.0f, 0.0f, 8.0f, "road_a");
		RoadSplineClass *road = RoadSystem::Peek_Road (id);
		Check (road != nullptr, "the road just added is not there");
		if (road != nullptr) {
			road->Get_End_Connection ().Type = ROAD_ENDPOINT_BRIDGE;
			Check (RoadSystem::Build_Network (0.25f, 0.5f, 4.0f), "the bridge road would not build");

			const RoadConnectionClass &end = road->Get_End_Connection ();
			Check (Near (end.Position.X, 40.0f, 0.01f), "the road hands over at x=%f, not 40",
					end.Position.X);
			Check (Near (end.Direction.X, 1.0f, 0.01f),
					"the road hands over pointing %f east, and it was going east", end.Direction.X);
			Check (end.TargetID == -1, "a bridge nobody named has id %d", end.TargetID);
		}
	}

	//
	//	On the ground.  A road that does not grade takes the ground's height exactly, which is
	//	what makes it a road on the terrain rather than a sheet floating over it.
	//
	RoadSystem::Clear_Roads ();
	Build_Bumpy ();
	Check (TerrainTextureSystem::Create_Masks (), "the masks would not be created");
	TerrainTextureSystem::Define_Default_Layers ();

	Add_Straight_Road (8.0f, 32.0f, 56.0f, 32.0f, 6.0f, "road_a");
	Check (RoadSystem::Build_Network (0.25f, 0.5f, 2.0f), "the road on terrain would not build");

	{
		RoadSplineClass *road = RoadSystem::Peek_Road (0);
		Check (road != nullptr, "the conformed road is missing");

		if (road != nullptr) {
			for (int i = 0; i < road->Get_Point_Count (); i ++) {
				const RoadPointClass &point = road->Get_Point (i);
				float ground = 0.0f;
				Check (WorldTerrainSystem::Sample_Height (point.Position.X, point.Position.Y, &ground),
						"the road ran off the field at %f,%f", point.Position.X, point.Position.Y);
				Check (Near (point.Position.Z, ground, 0.001f),
						"the road sits at %f where the ground is %f", point.Position.Z, ground);
			}
		}
	}

	//
	//	And the road told the terrain where it is.  Section 18 built the road mask with nothing
	//	to write into it; this is its writer.  The carriageway reads one, the shoulder fades,
	//	and far away reads nothing -- which is also what settles what a soldier is standing on.
	//
	{
		TerrainMaskClass *mask = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_ROAD);
		Check (mask != nullptr, "there is no road mask to have been written");

		if (mask != nullptr) {
			Check (mask->Get (32, 32) > 0.9f, "the middle of the road reads %f in the mask",
					mask->Get (32, 32));
			Check (Near (mask->Get (32, 50), 0.0f, 0.01f), "the mask reached eighteen metres away");

			float shoulder = mask->Get (32, 36);
			Check ((shoulder > 0.0f) && (shoulder < 0.9f),
					"the shoulder reads %f, and a shoulder is neither road nor not road", shoulder);
		}

		int layer = -1;
		Check (WorldTerrainSystem::Get_Material (32.0f, 32.0f, &layer),
				"the ground under the road is made of nothing");
		Check (layer == TerrainTextureSystem::Find_Layer ("road"),
				"the ground under the road is layer %d, and the road layer is %d",
				layer, TerrainTextureSystem::Find_Layer ("road"));
	}

	//
	//	Grading.  A road that grades takes the ground smoothed along its own length and the
	//	ground is then brought up to that, so the road comes out smoother than the country it
	//	crosses and there is no step at the edge of it.
	//
	RoadSystem::Clear_Roads ();
	WorldTerrainSystem::Destroy_Terrain ();
	Build_Bumpy ();

	{
		float rough = 0.0f;
		float low = 1000.0f;
		float high = -1000.0f;
		for (float x = 8.0f; x <= 56.0f; x += 1.0f) {
			float ground = 0.0f;
			if (WorldTerrainSystem::Sample_Height (x, 32.0f, &ground)) {
				if (ground < low)		low = ground;
				if (ground > high)	high = ground;
			}
		}
		rough = high - low;
		Check (rough > 1.0f, "the bumpy field is only %f from its low point to its high one", rough);

		Add_Straight_Road (8.0f, 32.0f, 56.0f, 32.0f, 6.0f, "road_a", ROAD_GRADE_FLATTEN);
		Check (RoadSystem::Build_Network (0.25f, 0.5f, 2.0f), "the graded road would not build");

		RoadSplineClass *road = RoadSystem::Peek_Road (0);
		Check (road != nullptr, "the graded road is missing");

		if (road != nullptr) {

			float road_low = 1000.0f;
			float road_high = -1000.0f;
			float worst = 0.0f;

			for (int i = 0; i < road->Get_Point_Count (); i ++) {

				const RoadPointClass &point = road->Get_Point (i);
				if (point.Position.Z < road_low)		road_low = point.Position.Z;
				if (point.Position.Z > road_high)	road_high = point.Position.Z;

				float ground = 0.0f;
				if (WorldTerrainSystem::Sample_Height (point.Position.X, point.Position.Y, &ground)) {
					float gap = WWMath::Fabs (ground - point.Position.Z);
					if (gap > worst) worst = gap;
				}
			}

			Check ((road_high - road_low) < (rough * 0.5f),
					"the graded road rises and falls %f where the ground does %f",
					road_high - road_low, rough);
			Check (worst < 0.75f, "the graded ground is still %f from the road it was graded to",
					worst);
		}
	}

	//
	//	Nothing outlives the world it described.
	//
	RoadSystem::Shutdown ();
	Check (RoadSystem::Get_Road_Count () == 0, "the roads outlived the service");
	Check (RoadSystem::Get_Junction_Count () == 0, "the junctions outlived the roads");

	WorldTerrainSystem::Destroy_Terrain ();
	WorldTerrainSystem::Shutdown ();
}


/*
**	Bridges -- roadmap Section 20.
**
**	The acceptance is variable length, damage, broken spans, collision and multiplayer state,
**	and four of those five can be asked without a screen or a physics scene.  Collision cannot:
**	a check running device-less has no scene to put a static object into, so what is checked
**	here is that the geometry declines cleanly rather than that a soldier stands on it.
*/
void	Check_Bridges (void)
{
	BridgeSystem::Init ();

	Check (BridgeSystem::Get_Definition_Count () == 3,
			"there are %d default bridge definitions, not three",
			BridgeSystem::Get_Definition_Count ());

	BridgeDefinitionClass *concrete = BridgeSystem::Find_Definition ("ow_bridge_concrete");
	Check (concrete != nullptr, "the concrete bridge is not defined");
	Check (BridgeSystem::Find_Definition ("ow_bridge_nobody_made_this") == nullptr,
			"a definition nobody defined was found anyway");

	if (concrete == nullptr) {
		BridgeSystem::Shutdown ();
		return ;
	}

	//
	//	No bridge art exists, and the section that says so is the section that has to keep
	//	working without it.
	//
	Check (!concrete->Names_Any_Model (),
			"a default bridge definition names a model, which would mean art exists");

	//
	//	Variable length.  A hundred metre gap with six metre abutments and eight metre spans is
	//	eleven spans of exactly eight, and the far end of the last piece is the far bank -- not
	//	near it, and not past it.  That is what generating a bridge rather than modelling one
	//	has to mean.
	//
	BridgeClass bridge;
	bridge.Set_Name ("test_span");
	bridge.Set_Definition ("ow_bridge_concrete");
	bridge.Set_Endpoints (Vector3 (0.0f, 0.0f, 10.0f), Vector3 (100.0f, 0.0f, 10.0f));

	int id = BridgeSystem::Add_Bridge (bridge);
	Check (id == 0, "the first bridge was given id %d", id);

	Check (BridgeSystem::Build_Layout (), "the bridge would not lay out");

	BridgeClass *placed = BridgeSystem::Peek_Bridge (id);
	Check (placed != nullptr, "the bridge that was just added is not there");

	if (placed == nullptr) {
		BridgeSystem::Shutdown ();
		return ;
	}

	int caps = 0;
	int spans = 0;
	int piers = 0;
	float deck = 0.0f;

	for (int i = 0; i < placed->Get_Section_Count (); i ++) {
		const BridgeSectionClass &section = placed->Get_Section (i);
		switch (section.Kind) {
			case BRIDGE_SECTION_START_CAP:
			case BRIDGE_SECTION_END_CAP:		caps ++;  deck += section.Length;  break;
			case BRIDGE_SECTION_SPAN:			spans ++; deck += section.Length;  break;
			case BRIDGE_SECTION_SUPPORT:		piers ++; break;
			default:									break;
		}
	}

	Check (caps == 2, "the bridge has %d abutments, not two", caps);
	Check (spans == 11, "a hundred metres of eight metre spans came out as %d spans", spans);
	Check (Near (deck, 100.0f, 0.01f), "the deck is %f long over a hundred metre gap", deck);

	//	A pier every twenty-four metres of an eighty-eight metre span region is three.
	Check (piers == 3, "the bridge stands on %d piers, not three", piers);

	//
	//	The deck is continuous.  Every piece starts where the one before it stopped, which is
	//	the difference between a bridge and a row of slabs with gaps between them.
	//
	{
		bool first = true;
		Vector3 previous (0.0f, 0.0f, 0.0f);
		float worst = 0.0f;

		for (int i = 0; i < placed->Get_Section_Count (); i ++) {
			const BridgeSectionClass &section = placed->Get_Section (i);
			if ((section.Kind != BRIDGE_SECTION_START_CAP) &&
				 (section.Kind != BRIDGE_SECTION_SPAN) &&
				 (section.Kind != BRIDGE_SECTION_END_CAP)) {
				continue ;
			}
			if (!first) {
				Vector3 gap = section.Start - previous;
				if (gap.Length () > worst) worst = gap.Length ();
			}
			previous = section.End;
			first = false;
		}

		Check (worst < 0.001f, "there is a %f metre gap between two pieces of the deck", worst);
		Check ((previous - placed->Get_End ()).Length () < 0.001f,
				"the last piece of the deck does not end at the far bank");
	}

	//
	//	Collision, as far as it can be asked here.  There is no physics scene in a device-less
	//	run, so the right answer is that nothing was built and nothing crashed.
	//
	Check (!BridgeSystem::Build_Geometry (), "geometry was built with no physics scene");
	Check (BridgeSystem::Get_Instance_Count () == 0, "physics objects exist with no scene");

	//
	//	Where the deck is.  Over a bridge the deck is the ground, and that is the whole reason
	//	this question is asked of the bridge and not of the terrain.
	//
	float height = 0.0f;
	int over = -1;
	Check (BridgeSystem::Conform_Point (20.0f, 0.0f, &height, &over),
			"the middle of the deck is not on the bridge");
	Check (Near (height, 10.0f), "the deck is at %f, not the height it was built at", height);
	Check (over == id, "the point over bridge %d says it is over bridge %d", id, over);

	Check (BridgeSystem::Conform_Point (20.0f, 3.0f, &height),
			"a point inside the carriageway is off the bridge");
	Check (!BridgeSystem::Conform_Point (20.0f, 6.0f, &height),
			"a point beyond the parapet is still on the bridge");
	Check (!BridgeSystem::Conform_Point (-10.0f, 0.0f, &height),
			"a point off the near end is still on the bridge");

	//
	//	Damage, and broken spans, which is the interesting half of it.  A bridge with its
	//	middle span gone is standing at both ends and useless, and every question anybody asks
	//	about it has to give that answer.
	//
	Check (BridgeSystem::Is_Traversable (id), "a bridge nobody has touched cannot be crossed");

	const int MIDDLE = 6;			// the abutment is section 0, so this is the sixth span

	Check (BridgeSystem::Set_Section_State (id, MIDDLE, BRIDGE_STATE_BROKEN),
			"the middle span would not break");
	Check (BridgeSystem::Get_Section_State (id, MIDDLE) == BRIDGE_STATE_BROKEN,
			"the broken span is not broken");
	Check (!BridgeSystem::Is_Traversable (id), "a bridge with a hole in it can still be crossed");

	int broken_at = -1;
	Check (BridgeSystem::Find_Break (id, &broken_at), "the break was not found");
	Check (broken_at == MIDDLE, "the break was found at %d, not at %d", broken_at, MIDDLE);

	Check (placed->Get_Damage_State () == BRIDGE_STATE_BROKEN,
			"a bridge with a broken span does not report itself broken");

	//	Six metres of abutment and five whole spans, from each end.
	float from_start = 0.0f;
	float from_end = 0.0f;
	Check (BridgeSystem::Get_Traversable_Extent (id, &from_start, &from_end),
			"the bridge would not say how much of it is left");
	Check (Near (from_start, 46.0f, 0.01f), "%f metres are reachable from the near end", from_start);
	Check (Near (from_end, 46.0f, 0.01f), "%f metres are reachable from the far end", from_end);

	//	And the hole is a hole: nothing conforms over it, while the deck either side still does.
	Check (!BridgeSystem::Conform_Point (50.0f, 0.0f, &height),
			"the broken span is still a floor");
	Check (BridgeSystem::Conform_Point (20.0f, 0.0f, &height),
			"breaking one span took the rest of the deck with it");
	Check (BridgeSystem::Conform_Point (80.0f, 0.0f, &height),
			"breaking one span took the far half of the deck with it");

	//
	//	Multiplayer state.  What travels is one byte per section and nothing about the geometry,
	//	because a client with the same definition and the same two endpoints lays out the same
	//	bridge.  So the block has to be able to put a client's copy into the server's state.
	//
	{
		int expected = 2 + 4 + placed->Get_Section_Count ();
		Check (BridgeSystem::Get_State_Block_Size () == expected,
				"the state block is %d bytes, not the %d one bridge needs",
				BridgeSystem::Get_State_Block_Size (), expected);

		unsigned char block[256];
		int used = 0;
		Check (BridgeSystem::Get_State_Block (block, sizeof (block), &used),
				"the state block would not be written");
		Check (used == expected, "the state block wrote %d bytes, not %d", used, expected);

		unsigned char small_block[4];
		int small_used = -1;
		Check (!BridgeSystem::Get_State_Block (small_block, sizeof (small_block), &small_used),
				"the state block fitted in four bytes");
		Check (small_used == 0, "a refused state block reported %d bytes written", small_used);

		//	Repair everything, then hand the block back and watch the damage return.
		Check (BridgeSystem::Set_Damage_State (id, BRIDGE_STATE_PRISTINE),
				"the bridge would not be repaired");
		Check (BridgeSystem::Is_Traversable (id), "a repaired bridge cannot be crossed");

		Check (BridgeSystem::Apply_State_Block (block, used), "the state block would not apply");
		Check (BridgeSystem::Get_Section_State (id, MIDDLE) == BRIDGE_STATE_BROKEN,
				"the state block did not carry the break");
		Check (!BridgeSystem::Is_Traversable (id), "the state block did not carry the hole");

		//	Applying what the server said is not a local change to send back out again.
		BridgeSystem::Clear_State_Dirty ();
		Check (BridgeSystem::Apply_State_Block (block, used), "the state block would not reapply");
		Check (!BridgeSystem::Is_State_Dirty (),
				"applying the server's state marked it as something to send");

		//	A truncated block is refused rather than half applied.
		Check (!BridgeSystem::Apply_State_Block (block, 3), "a truncated state block was accepted");
	}

	//
	//	Debris.  Nothing draws it yet, so the system holds what came off until something does.
	//	The default definitions name none, so this is the empty case being empty on purpose.
	//
	Check (BridgeSystem::Get_Pending_Debris_Count () == 0,
			"debris was emitted by a definition that names none");

	{
		BridgeDefinitionClass rubble = *concrete;
		rubble.Set_Name ("ow_bridge_concrete_debris_check");
		rubble.Set_Debris (BRIDGE_SECTION_SPAN, BRIDGE_STATE_BROKEN, "ow_bridge_concrete_rubble", 4);
		Check (BridgeSystem::Define_Bridge (rubble), "the debris definition would not define");
		Check (BridgeSystem::Get_Definition_Count () == 4,
				"defining a fourth bridge left %d", BridgeSystem::Get_Definition_Count ());

		BridgeClass second;
		second.Set_Definition ("ow_bridge_concrete_debris_check");
		second.Set_Endpoints (Vector3 (0.0f, 60.0f, 10.0f), Vector3 (60.0f, 60.0f, 10.0f));
		int second_id = BridgeSystem::Add_Bridge (second);

		Check (BridgeSystem::Build_Layout (), "the second bridge would not lay out");
		Check (BridgeSystem::Break_Span_At (second_id, Vector3 (30.0f, 60.0f, 10.0f)),
				"no span was broken where the shell landed");
		Check (BridgeSystem::Get_Pending_Debris_Count () == 1,
				"%d pieces of debris came off one span",
				BridgeSystem::Get_Pending_Debris_Count ());

		const BridgeDebrisClass *debris = BridgeSystem::Peek_Pending_Debris (0);
		Check (debris != nullptr, "the debris that was recorded is not there");
		if (debris != nullptr) {
			Check (debris->BridgeID == second_id, "the debris belongs to bridge %d", debris->BridgeID);
			Check (debris->Count == 4, "%d pieces came off, not four", debris->Count);
			Check (WWMath::Fabs (debris->Position.X - 30.0f) < 8.0f,
					"the debris came off %f metres along, not near the middle", debris->Position.X);
		}

		BridgeSystem::Clear_Pending_Debris ();
		Check (BridgeSystem::Get_Pending_Debris_Count () == 0, "the debris was not cleared");
	}

	//
	//	A bridge naming a kind nobody defined is a map error, not a crash: it stays in the list
	//	with no sections, carries nothing, and does not take the other bridges down with it.
	//
	{
		BridgeClass orphan;
		orphan.Set_Name ("orphan");
		orphan.Set_Definition ("ow_bridge_not_a_kind");
		orphan.Set_Endpoints (Vector3 (0.0f, -60.0f, 10.0f), Vector3 (40.0f, -60.0f, 10.0f));
		int orphan_id = BridgeSystem::Add_Bridge (orphan);

		Check (!BridgeSystem::Build_Layout (),
				"a bridge with no definition laid out anyway");

		BridgeClass *placed_orphan = BridgeSystem::Peek_Bridge (orphan_id);
		Check (placed_orphan != nullptr, "the orphan bridge disappeared");
		if (placed_orphan != nullptr) {
			Check (placed_orphan->Get_Section_Count () == 0,
					"a bridge with no definition has %d sections",
					placed_orphan->Get_Section_Count ());
		}
		Check (!BridgeSystem::Is_Traversable (orphan_id), "a bridge with no sections can be crossed");
		Check (BridgeSystem::Is_Traversable (0), "the orphan took the first bridge with it");
	}

	//
	//	The join to Section 19.  A road that said it hands over to a bridge is bound to the
	//	abutment it is standing at, and both ends then know each other's id.
	//
	{
		RoadSystem::Init ();

		RoadSplineClass road;
		road.Set_Name ("approach");
		road.Add_Control_Point (Vector3 (-40.0f, 0.0f, 10.0f));
		road.Add_Control_Point (Vector3 (-20.0f, 0.0f, 10.0f));
		road.Add_Control_Point (Vector3 (0.0f, 0.0f, 10.0f));
		road.Set_Width (8.0f);
		road.Get_End_Connection ().Type = ROAD_ENDPOINT_BRIDGE;

		int road_id = RoadSystem::Add_Road (road);
		Check (RoadSystem::Build_Network (), "the approach road would not build");

		Check (BridgeSystem::Connect_Roads () == 1,
				"the road that hands over to a bridge was not bound to one");

		RoadSplineClass *bound = RoadSystem::Peek_Road (road_id);
		Check (bound != nullptr, "the approach road is not there");
		if (bound != nullptr) {
			Check (bound->Get_End_Connection ().TargetID == 0,
					"the road hands over to bridge %d, not to the one it is touching",
					bound->Get_End_Connection ().TargetID);
		}

		BridgeClass *met = BridgeSystem::Peek_Bridge (0);
		Check (met != nullptr, "the bridge the road meets is not there");
		if (met != nullptr) {
			Check (met->Get_Start_Road () == road_id,
					"the bridge says road %d arrives at it, not road %d",
					met->Get_Start_Road (), road_id);
		}

		RoadSystem::Shutdown ();
	}

	//
	//	Nothing outlives the world it described.
	//
	BridgeSystem::Shutdown ();
	Check (BridgeSystem::Get_Bridge_Count () == 0, "the bridges outlived the service");
	Check (BridgeSystem::Get_Definition_Count () == 0, "the definitions outlived the service");
	Check (BridgeSystem::Get_Pending_Debris_Count () == 0, "the debris outlived the service");
}


/*
**	Section 21's acceptance is a ratio: a large forest has to be drawn as substantially fewer
**	things than it has plants in it.  That ratio is a question about how instances group into
**	cells and materials, not about what they look like, so it has an answer here -- with no
**	device, no scene and no art -- and the answer is the same one a running client would get.
*/
void	Check_Foliage (void)
{
	FoliageSystem::Init ();

	//
	//	Five categories, four blocking behaviours, and the two are separate axes.
	//
	Check (FoliageSystem::Get_Type_Count () == 5,
			"%d default foliage types were defined, not 5", FoliageSystem::Get_Type_Count ());

	FoliageTypeClass *conifer = FoliageSystem::Find_Type ("ow_tree_conifer");
	FoliageTypeClass *scrub = FoliageSystem::Find_Type ("ow_bush_scrub");
	FoliageTypeClass *dead = FoliageSystem::Find_Type ("ow_tree_dead");
	FoliageTypeClass *grass = FoliageSystem::Find_Type ("ow_grass_clump");
	FoliageTypeClass *boulder = FoliageSystem::Find_Type ("ow_rock_boulder");

	Check (conifer != nullptr && scrub != nullptr && dead != nullptr &&
			 grass != nullptr && boulder != nullptr, "a default foliage type is missing");
	Check (FoliageSystem::Find_Type ("ow_tree_banana") == nullptr,
			"a type nobody defined was found anyway");

	if (conifer != nullptr) {
		Check (conifer->Get_Category () == FOLIAGE_TREE, "the conifer is not a tree");
		Check (conifer->Blocks_Infantry () && conifer->Blocks_Vehicles (),
				"a live tree does not stop everything");
		Check (!conifer->Is_Destructible (), "a live tree is destructible");
		Check (!conifer->Has_Far_Lod (), "a type naming no impostor has a far level anyway");
	}
	if (grass != nullptr) {
		Check (grass->Is_Visual_Only (), "grass blocks something");
		Check (!grass->Is_Destructible (), "grass is destructible");
	}
	if (dead != nullptr) {
		Check (dead->Blocks_Infantry () && !dead->Blocks_Vehicles (),
				"a dead tree does not stop exactly a soldier");
		Check (dead->Is_Destructible (), "a dead tree does not break");
	}
	if (scrub != nullptr) {
		Check (scrub->Is_Visual_Only () && scrub->Is_Destructible (),
				"scrub is not the destructible visual-only case");
	}
	if (boulder != nullptr) {
		Check (boulder->Blocks_Infantry () && boulder->Blocks_Vehicles () &&
				 !boulder->Is_Destructible (), "a boulder is not the solid indestructible case");
	}

	Check (::strcmp (Foliage_Category_Name (FOLIAGE_GRASS_CLUMP), "GRASS_CLUMP") == 0,
			"a category has the wrong name");
	Check (::strcmp (Foliage_Lod_Name (FOLIAGE_LOD_FAR), "FAR") == 0, "a level has the wrong name");

	//
	//	A forest from a seed is the same forest twice.  This is what lets a client rebuild the
	//	server's wood rather than be sent it.
	//
	Check (FoliageSystem::Scatter ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f), 200.0f, 2000, 1234) == 2000,
			"the scatter did not plant everything it was asked for");
	Check (FoliageSystem::Get_Instance_Count () == 2000,
			"%d trees were planted, not 2000", FoliageSystem::Get_Instance_Count ());

	Vector3 first_tree (0.0f, 0.0f, 0.0f);
	Vector3 last_tree (0.0f, 0.0f, 0.0f);
	if (FoliageSystem::Peek_Instance (0) != nullptr) {
		first_tree = FoliageSystem::Peek_Instance (0)->Position;
	}
	if (FoliageSystem::Peek_Instance (1999) != nullptr) {
		last_tree = FoliageSystem::Peek_Instance (1999)->Position;
	}

	Check (first_tree != last_tree, "every tree in the scatter landed in the same place");

	FoliageSystem::Clear_Instances ();
	FoliageSystem::Scatter ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f), 200.0f, 2000, 1234);
	Check (FoliageSystem::Peek_Instance (0) != nullptr &&
			 FoliageSystem::Peek_Instance (0)->Position == first_tree,
			"the same seed grew a different forest");
	Check (FoliageSystem::Peek_Instance (1999) != nullptr &&
			 FoliageSystem::Peek_Instance (1999)->Position == last_tree,
			"the same seed grew a different forest at the far end");

	FoliageSystem::Clear_Instances ();
	FoliageSystem::Scatter ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f), 200.0f, 2000, 5678);
	Check (FoliageSystem::Peek_Instance (0) != nullptr &&
			 FoliageSystem::Peek_Instance (0)->Position != first_tree,
			"two different seeds grew the same forest");

	//
	//	Cells.  Every tree is in one, the grid finds the one a point is in, and the lists add up.
	//
	FoliageSystem::Clear_Instances ();
	FoliageSystem::Scatter ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f), 200.0f, 2000, 1234);

	Check (FoliageSystem::Build_Cells (32.0f), "the grid would not build");
	Check (FoliageSystem::Are_Cells_Built (), "the grid says it is not built");
	Check (Near (FoliageSystem::Get_Cell_Size (), 32.0f),
			"the cells came out %f across, not 32", FoliageSystem::Get_Cell_Size ());

	int cells = FoliageSystem::Get_Cell_Count ();
	Check (cells > 50 && cells < 250,
			"%d cells hold a 400 metre wood cut into 32 metre squares", cells);

	int counted = 0;
	for (int c = 0; c < cells; c ++) {
		const FoliageCellClass *cell = FoliageSystem::Peek_Cell (c);
		Check (cell != nullptr && cell->Count > 0, "cell %d holds nothing and exists anyway", c);
		if (cell != nullptr) {
			counted += cell->Count;
		}
	}
	Check (counted == 2000, "the cells hold %d trees between them, not 2000", counted);

	const FoliageInstanceClass *sample = FoliageSystem::Peek_Instance (17);
	Check (sample != nullptr && sample->Cell >= 0, "a tree is in no cell");
	if (sample != nullptr) {
		Check (FoliageSystem::Find_Cell (sample->Position.X, sample->Position.Y) == sample->Cell,
				"the grid puts a tree in a different cell than the tree does");
	}
	Check (FoliageSystem::Find_Cell (100000.0f, 100000.0f) == -1,
			"a point off the map is in a cell");

	//
	//	The acceptance.  A batch is one thing to draw; there have to be far fewer of them than
	//	there are trees.
	//
	Check (FoliageSystem::Build_Batches (), "the batches would not plan");
	int batches = FoliageSystem::Get_Batch_Count ();
	Check (batches > 0, "a wood of 2000 trees plans no batches at all");
	Check ((batches * 8) <= FoliageSystem::Get_Instance_Count (),
			"2000 trees plan %d batches, which is not substantially fewer", batches);
	Check (batches == cells,
			"%d batches for %d cells: one material at one level should be one batch a cell",
			batches, cells);

	int batched = 0;
	for (int b = 0; b < batches; b ++) {
		const FoliageBatchClass *batch = FoliageSystem::Peek_Batch (b);
		Check (batch != nullptr, "batch %d is not there", b);
		if (batch == nullptr) { continue ; }
		Check (batch->Lod == FOLIAGE_LOD_NEAR,
				"a type naming no impostor planned a far batch");
		Check (batch->Count > 0 && batch->Count <= 256,
				"batch %d holds %d instances", b, batch->Count);
		Check (Near (batch->NearDistance, 90.0f) && Near (batch->CullDistance, 400.0f),
				"a batch of conifers switches at %f and dies at %f",
				batch->NearDistance, batch->CullDistance);
		batched += batch->Count;
	}
	Check (batched == 2000, "the batches hold %d trees between them, not 2000", batched);

	//
	//	Collision proxies are per cell per blocking behaviour, not per tree.
	//
	Check (FoliageSystem::Get_Proxy_Count () == cells,
			"%d proxies for %d cells of one kind of tree",
			FoliageSystem::Get_Proxy_Count (), cells);

	if (sample != nullptr) {
		Check (FoliageSystem::Is_Blocked (sample->Position + Vector3 (0.0f, 0.0f, 1.0f),
													FOLIAGE_BLOCKS_INFANTRY),
				"a soldier walks through a tree trunk");
		Check (!FoliageSystem::Is_Blocked (sample->Position + Vector3 (0.0f, 0.0f, 100.0f),
													  FOLIAGE_BLOCKS_INFANTRY),
				"a tree blocks the sky above itself");
		Check (!FoliageSystem::Is_Blocked (sample->Position + Vector3 (0.0f, 0.0f, 1.0f),
													  FOLIAGE_BLOCKS_NOTHING),
				"something that blocks nothing was blocked");
	}

	//
	//	Visibility.  A cell is culled and takes its batches with it.
	//
	FoliageSystem::Update_Visibility (Vector3 (0.0f, 0.0f, 0.0f), Vector3 (1.0f, 0.0f, 0.0f));
	Check (FoliageSystem::Get_Visible_Cell_Count () == cells,
			"%d cells of %d are visible with nothing culling them",
			FoliageSystem::Get_Visible_Cell_Count (), cells);

	FoliageSystem::Update_Visibility (Vector3 (0.0f, 0.0f, 0.0f), Vector3 (1.0f, 0.0f, 0.0f), -1.0f, 50.0f);
	int near_cells = FoliageSystem::Get_Visible_Cell_Count ();
	Check (near_cells > 0 && near_cells < cells,
			"a fifty metre view distance leaves %d of %d cells", near_cells, cells);

	FoliageSystem::Update_Visibility (Vector3 (0.0f, 0.0f, 0.0f), Vector3 (1.0f, 0.0f, 0.0f), 0.5f, 0.0f);
	int cone_cells = FoliageSystem::Get_Visible_Cell_Count ();
	Check (cone_cells > 0 && cone_cells < cells,
			"a sixty degree cone leaves %d of %d cells", cone_cells, cells);

	//	A conifer names no impostor, so it has no far level to drop to: past its near distance
	//	its cells simply stop drawing.  From the middle of a wood twice as wide as that
	//	distance, some of it draws and some does not, and everything that draws is inside it.
	FoliageSystem::Update_Visibility (Vector3 (0.0f, 0.0f, 0.0f), Vector3 (1.0f, 0.0f, 0.0f));
	int drawn = FoliageSystem::Get_Visible_Instance_Count ();
	Check (drawn > 0 && drawn < 2000,
			"%d of 2000 trees draw from the middle of a wood wider than their near distance",
			drawn);

	for (int b = 0; b < FoliageSystem::Get_Batch_Count (); b ++) {
		const FoliageBatchClass *batch = FoliageSystem::Peek_Batch (b);
		if (batch == nullptr || !batch->Visible) { continue ; }
		const FoliageCellClass *cell = FoliageSystem::Peek_Cell (batch->Cell);
		Check (cell != nullptr && cell->Distance <= batch->NearDistance,
				"a batch with no far level is drawn past its near distance");
	}

	//	From far enough away every cell is past the cull distance and nothing is submitted.
	FoliageSystem::Update_Visibility (Vector3 (5000.0f, 0.0f, 0.0f), Vector3 (-1.0f, 0.0f, 0.0f));
	Check (FoliageSystem::Get_Visible_Batch_Count () == 0,
			"%d batches are still drawn from five kilometres away",
			FoliageSystem::Get_Visible_Batch_Count ());

	//
	//	Distance LOD.  A type with an impostor plans two batches a cell and never draws both.
	//
	{
		FoliageSystem::Clear_Instances ();

		FoliageTypeClass impostored;
		impostored.Set_Name ("ow_tree_conifer_lod");
		impostored.Set_Category (FOLIAGE_TREE);
		impostored.Set_Model ("ow_tree_conifer");
		impostored.Set_Material ("ow_tree_conifer.tga");
		impostored.Set_Impostor ("ow_tree_conifer_impostor.tga", 6.0f, 10.0f);
		impostored.Set_Distances (60.0f, 300.0f);
		Check (FoliageSystem::Define_Type (impostored), "an impostored type was refused");

		//	Wider than the near distance and inside the cull distance, so the wood has cells at
		//	both levels at once and none that have dropped out altogether.
		FoliageSystem::Scatter ("ow_tree_conifer_lod", Vector3 (0.0f, 0.0f, 0.0f), 200.0f, 400, 99);
		Check (FoliageSystem::Build_Cells (32.0f), "the impostored grid would not build");
		Check (FoliageSystem::Build_Batches (), "the impostored batches would not plan");

		int lod_cells = FoliageSystem::Get_Cell_Count ();
		Check (FoliageSystem::Get_Batch_Count () == (lod_cells * 2),
				"%d batches for %d cells that each have two levels",
				FoliageSystem::Get_Batch_Count (), lod_cells);

		//	Standing in the middle: the near cells draw their models and the outer ones their
		//	impostors, and no cell draws both.
		FoliageSystem::Update_Visibility (Vector3 (0.0f, 0.0f, 0.0f), Vector3 (1.0f, 0.0f, 0.0f));
		Check (FoliageSystem::Get_Visible_Batch_Count () == lod_cells,
				"%d of %d cells drew exactly one level",
				FoliageSystem::Get_Visible_Batch_Count (), lod_cells);

		int near_batches = 0;
		int far_batches = 0;
		for (int b = 0; b < FoliageSystem::Get_Batch_Count (); b ++) {
			const FoliageBatchClass *batch = FoliageSystem::Peek_Batch (b);
			if (batch == nullptr || !batch->Visible) { continue ; }
			if (batch->Lod == FOLIAGE_LOD_NEAR) { near_batches ++; } else { far_batches ++; }
		}
		Check (near_batches > 0, "nothing is close enough to draw its model");
		Check (far_batches > 0, "nothing is far enough to drop to an impostor");

		//	Standing off the near edge of the wood by more than the near distance: nothing is
		//	close enough to draw its model, the near half is impostors, and the far half is past
		//	the cull distance and drawn not at all.
		FoliageSystem::Update_Visibility (Vector3 (400.0f, 0.0f, 0.0f), Vector3 (-1.0f, 0.0f, 0.0f));
		int impostors = 0;
		for (int b = 0; b < FoliageSystem::Get_Batch_Count (); b ++) {
			const FoliageBatchClass *batch = FoliageSystem::Peek_Batch (b);
			if (batch == nullptr || !batch->Visible) { continue ; }
			Check (batch->Lod == FOLIAGE_LOD_FAR, "a model is still drawn at four hundred metres");
			impostors ++;
		}
		Check (impostors > 0, "nothing at all is drawn from four hundred metres");
		Check (impostors < lod_cells, "nothing was culled by distance at four hundred metres");
	}

	//
	//	A cell too crowded for one mesh becomes several batches of the same material rather
	//	than one mesh too large to draw.
	//
	{
		FoliageSystem::Clear_Instances ();
		FoliageSystem::Scatter ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f), 4.0f, 600, 7);
		Check (FoliageSystem::Build_Cells (32.0f), "the crowded grid would not build");
		Check (FoliageSystem::Build_Batches (), "the crowded batches would not plan");

		int crowded = FoliageSystem::Get_Batch_Count ();
		Check (crowded >= 3, "600 trees inside one cell planned %d batches", crowded);

		int held = 0;
		for (int b = 0; b < crowded; b ++) {
			const FoliageBatchClass *batch = FoliageSystem::Peek_Batch (b);
			if (batch == nullptr) { continue ; }
			Check (batch->Count <= 256, "a batch holds %d instances, past the cap", batch->Count);
			held += batch->Count;
		}
		Check (held == 600, "the crowded batches hold %d trees, not 600", held);
	}

	//
	//	Destruction.  Only what says it breaks breaks, and felling one thing does not disturb
	//	the count of everything else.
	//
	{
		FoliageSystem::Clear_Instances ();

		int solid = FoliageSystem::Add_Instance ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f));
		int breakable = FoliageSystem::Add_Instance ("ow_tree_dead", Vector3 (4.0f, 0.0f, 0.0f));
		Check (solid == 0 && breakable == 1, "the two test plants were not planted");
		Check (FoliageSystem::Add_Instance ("ow_tree_banana", Vector3 (0.0f, 0.0f, 0.0f)) == -1,
				"a plant of a type nobody defined was planted");

		Check (FoliageSystem::Build_Cells (32.0f), "the small grid would not build");
		Check (FoliageSystem::Build_Batches (), "the small batches would not plan");

		Check (!FoliageSystem::Apply_Damage (solid, 10000.0f),
				"a live tree was shot down and it is not destructible");
		Check (FoliageSystem::Get_Live_Instance_Count () == 2, "something died that should not");

		Check (!FoliageSystem::Apply_Damage (breakable, 100.0f),
				"a dead tree with 150 health fell to 100 damage");
		Check (FoliageSystem::Apply_Damage (breakable, 100.0f),
				"a dead tree with 50 health left survived 100 more damage");
		Check (FoliageSystem::Get_Live_Instance_Count () == 1,
				"%d plants are alive after one fell", FoliageSystem::Get_Live_Instance_Count ());
		Check (FoliageSystem::Get_Destroyed_Count () == 1,
				"%d plants are counted as destroyed", FoliageSystem::Get_Destroyed_Count ());
		Check (!FoliageSystem::Destroy_Instance (breakable), "a fallen tree fell again");
		Check (FoliageSystem::Get_Instance_Count () == 2,
				"a destroyed plant was removed from the world rather than marked");

		//	Replanning after a felling leaves the fallen one out.
		Check (FoliageSystem::Build_Batches (), "the batches would not replan");
		int left = 0;
		for (int b = 0; b < FoliageSystem::Get_Batch_Count (); b ++) {
			const FoliageBatchClass *batch = FoliageSystem::Peek_Batch (b);
			if (batch != nullptr) { left += batch->Count; }
		}
		Check (left == 1, "%d plants are still batched after one fell", left);
	}

	//
	//	Queries.
	//
	{
		FoliageSystem::Clear_Instances ();
		FoliageSystem::Add_Instance ("ow_tree_conifer", Vector3 (0.0f, 0.0f, 0.0f));
		FoliageSystem::Add_Instance ("ow_tree_conifer", Vector3 (10.0f, 0.0f, 0.0f));
		FoliageSystem::Add_Instance ("ow_tree_conifer", Vector3 (100.0f, 0.0f, 0.0f));
		FoliageSystem::Build_Cells (32.0f);

		DynamicVectorClass<int> found;
		Check (FoliageSystem::Find_Instances_Near (Vector3 (0.0f, 0.0f, 0.0f), 20.0f, found) == 2,
				"%d trees are within twenty metres of the first one, not 2", found.Count ());
		Check (FoliageSystem::Find_Nearest_Instance (Vector3 (9.0f, 0.0f, 0.0f), 20.0f) == 1,
				"the nearest tree to a point beside the second one is not the second one");
		Check (FoliageSystem::Find_Nearest_Instance (Vector3 (500.0f, 0.0f, 0.0f), 20.0f) == -1,
				"a tree was found five hundred metres from any");
	}

	//
	//	No device, no scene, no geometry -- and the batches were still planned and counted,
	//	which is the whole reason the acceptance is answerable here.
	//
	Check (!FoliageSystem::Build_Geometry (),
			"geometry was built with no physics scene to put it in");
	Check (FoliageSystem::Get_Object_Count () == 0, "there are objects in a scene that is not there");
	Check (!FoliageSystem::Has_Geometry (), "the system says it has geometry");

	//
	//	Nothing outlives the world it described.
	//
	FoliageSystem::Shutdown ();
	Check (FoliageSystem::Get_Instance_Count () == 0, "the plants outlived the service");
	Check (FoliageSystem::Get_Type_Count () == 0, "the types outlived the service");
	Check (FoliageSystem::Get_Cell_Count () == 0, "the cells outlived the service");
	Check (FoliageSystem::Get_Batch_Count () == 0, "the batches outlived the service");
}


/*
**	Roadmap Section 22.  The acceptance is that water works visually and physically without a
**	competing collision system, and the reason this is checkable at all without a device is the
**	same reason Check_Foliage is: shaping the ground, stamping the shoreline mask and answering
**	is-this-point-wet all come from the authored shape rather than from anything a renderer or a
**	physics scene would have to exist to provide.  Only the surface mesh itself needs a scene,
**	and it is checked the same way FoliageSystem::Build_Geometry is -- declining correctly with
**	none there.
*/
void	Check_Water (void)
{
	WaterSystem::Init ();

	//
	//	Six categories.  Ocean and coast do not carve; lake, pond, river and stream do.  River
	//	and stream flow; nothing else does.
	//
	Check (::strcmp (Water_Category_Name (WATER_RIVER), "river") == 0,
			"the river category is named '%s'", Water_Category_Name (WATER_RIVER));
	Check (Water_Category_Flows (WATER_RIVER) && Water_Category_Flows (WATER_STREAM),
			"a river or a stream does not flow");
	Check (!Water_Category_Flows (WATER_LAKE) && !Water_Category_Flows (WATER_OCEAN),
			"a lake or an ocean flows");
	Check (Water_Category_Carves_Terrain (WATER_LAKE) && Water_Category_Carves_Terrain (WATER_POND) &&
			 Water_Category_Carves_Terrain (WATER_RIVER) && Water_Category_Carves_Terrain (WATER_STREAM),
			"a bounded body of water does not carve its own bed");
	Check (!Water_Category_Carves_Terrain (WATER_OCEAN) && !Water_Category_Carves_Terrain (WATER_COAST),
			"an ocean or a coast carved a bed it has no boundary to carve along");

	//
	//	One definition per category, none of them naming any art.
	//
	WaterSystem::Define_Default_Water ();
	Check (WaterSystem::Get_Definition_Count () == 6,
			"%d default water kinds were defined, not 6", WaterSystem::Get_Definition_Count ());

	WaterDefinitionClass *lake_def = WaterSystem::Find_Definition ("ow_water_lake");
	WaterDefinitionClass *river_def = WaterSystem::Find_Definition ("ow_water_river");
	Check ((lake_def != nullptr) && (river_def != nullptr), "a default water kind is missing");
	Check (WaterSystem::Find_Definition ("ow_water_swamp") == nullptr,
			"a kind nobody defined was found anyway");

	if (lake_def != nullptr) {
		Check (lake_def->Get_Category () == WATER_LAKE, "the lake definition is not a lake");
		Check (!lake_def->Names_Any_Texture (), "the lake definition names art that does not exist");
	}

	//
	//	A pond: a closed square ring, still water at one flat height.
	//
	WaterAreaClass pond;
	pond.Set_Name ("test_pond");
	pond.Set_Definition ("ow_water_pond");
	pond.Set_Closed (true);
	pond.Set_Height (-1.0f);
	pond.Add_Station (Vector3 (10.0f, 10.0f, 0.0f), 0.0f);
	pond.Add_Station (Vector3 (20.0f, 10.0f, 0.0f), 0.0f);
	pond.Add_Station (Vector3 (20.0f, 20.0f, 0.0f), 0.0f);
	pond.Add_Station (Vector3 (10.0f, 20.0f, 0.0f), 0.0f);
	int pond_index = WaterSystem::Add_Area (pond);
	Check (WaterSystem::Get_Area_Count () == 1, "adding a pond did not add an area");
	Check (WaterSystem::Find_Area ("test_pond") != nullptr, "the pond cannot be found by name");

	Check (WaterSystem::Peek_Area (pond_index).Contains_Point (15.0f, 15.0f),
			"the middle of the pond is not in the pond");
	Check (!WaterSystem::Peek_Area (pond_index).Contains_Point (0.0f, 0.0f),
			"the origin, ten metres outside the pond, is in the pond");

	float height = 0.0f;
	bool has_height = WaterSystem::Get_Water_Height (15.0f, 15.0f, &height);
	Check (has_height && Near (height, -1.0f), "the pond surface reads %f, not -1", height);
	Check (!WaterSystem::Get_Water_Height (0.0f, 0.0f, &height),
			"there is a water height at a point with no water");

	Vector3 direction;
	float speed = -1.0f;
	Check (WaterSystem::Get_Flow (15.0f, 15.0f, &direction, &speed),
			"the pond could not answer a flow query at all");
	Check (Near (speed, 0.0f), "still water in a pond flows at %f", speed);

	//
	//	A stream: an open, sloping line, narrower at the far end.
	//
	WaterAreaClass stream;
	stream.Set_Name ("test_stream");
	stream.Set_Definition ("ow_water_stream");
	stream.Set_Closed (false);
	stream.Add_Station (Vector3 (0.0f, 0.0f, 2.0f), 4.0f);
	stream.Add_Station (Vector3 (20.0f, 0.0f, 0.0f), 2.0f);
	int stream_index = WaterSystem::Add_Area (stream);

	Check (WaterSystem::Peek_Area (stream_index).Contains_Point (10.0f, 1.0f),
			"a point one metre off a four metre wide stream's centre line is outside it");
	Check (!WaterSystem::Peek_Area (stream_index).Contains_Point (10.0f, 5.0f),
			"a point five metres off the stream is inside it");

	has_height = WaterSystem::Get_Water_Height (10.0f, 0.0f, &height);
	Check (has_height && Near (height, 1.0f, 0.01f),
			"halfway down a slope from 2 to 0 the surface reads %f, not 1", height);

	Check (WaterSystem::Get_Flow (10.0f, 0.0f, &direction, &speed),
			"the stream could not answer a flow query");
	Check (Near (direction.X, 1.0f, 0.01f) && Near (direction.Y, 0.0f, 0.01f),
			"a stream running due east flows towards %f,%f", direction.X, direction.Y);
	Check (speed > 0.0f, "a flowing stream reports zero speed");

	float depth = -1.0f;
	bool has_depth = WaterSystem::Get_Water_Depth (10.0f, 0.0f, -3.0f, &depth);
	Check (has_depth && Near (depth, 4.0f, 0.01f),
			"the ground three below a surface at one is %f deep, not 4", depth);
	has_depth = WaterSystem::Get_Water_Depth (10.0f, 0.0f, 5.0f, &depth);
	Check (has_depth && Near (depth, 0.0f),
			"a floor above the surface reads a depth of %f, not 0", depth);
	Check (WaterSystem::Is_Navigable (10.0f, 0.0f, -3.0f, 2.0f),
			"four metres of depth is not navigable to something that draws two");
	Check (!WaterSystem::Is_Navigable (10.0f, 0.0f, -3.0f, 10.0f),
			"four metres of depth is navigable to something that draws ten");

	//
	//	Shaping the ground.  A flat field, carved along the stream and under the pond, and left
	//	alone where an ocean was placed over it -- there is no boundary for an ocean to carve.
	//
	WorldTerrainSystem::Init ();
	Check (WorldTerrainSystem::Create_Terrain (33, 33, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			"a flat field would not be created");

	float *flat = new float[33 * 33];
	for (int i = 0; i < 33 * 33; i ++) { flat[i] = 0.0f; }
	Check (WorldTerrainSystem::Set_Heights (flat, 33 * 33), "the flat field's heights were refused");
	delete [] flat;

	//
	//	Near its source the stream sits at Z=2, well above the flat field it is cut into, so a
	//	levelled-and-cut bed there reads well above the original 0 -- proof the cut ran at all,
	//	not just that the ground near it happens to still read close to flat.
	//
	Check (WaterSystem::Shape_Terrain (stream_index), "the stream would not shape the ground");
	float carved = -5.0f;
	bool sampled = WorldTerrainSystem::Sample_Height (2.0f, 0.0f, &carved);
	Check (sampled && (carved > 0.4f),
			"the ground near the stream's source reads %f, not levelled up to it at all", carved);

	//	The centre is only reached by one row of the interior sweep at a partial, smoothstep-
	//	tapered weight, so it lands well short of the full -2.5 the surface and bed depth would
	//	give a fully saturated point; -1.0 is a wide, safe margin that only a real cut clears.
	Check (WaterSystem::Shape_Terrain (pond_index), "the pond would not shape the ground");
	sampled = WorldTerrainSystem::Sample_Height (15.0f, 15.0f, &carved);
	Check (sampled && (carved < -1.0f),
			"the ground under the pond reads %f, barely below the flat field it cut into", carved);

	WaterAreaClass ocean;
	ocean.Set_Name ("test_ocean");
	ocean.Set_Definition ("ow_water_ocean");
	ocean.Set_Closed (true);
	ocean.Set_Height (0.0f);
	ocean.Add_Station (Vector3 (0.0f, 25.0f, 0.0f), 0.0f);
	ocean.Add_Station (Vector3 (10.0f, 25.0f, 0.0f), 0.0f);
	ocean.Add_Station (Vector3 (10.0f, 32.0f, 0.0f), 0.0f);
	ocean.Add_Station (Vector3 (0.0f, 32.0f, 0.0f), 0.0f);
	int ocean_index = WaterSystem::Add_Area (ocean);

	Check (WaterSystem::Shape_Terrain (ocean_index), "an ocean refused to shape, which is a no-op");
	sampled = WorldTerrainSystem::Sample_Height (5.0f, 28.0f, &carved);
	Check (sampled && Near (carved, 0.0f),
			"an ocean, which does not carve, left the ground at %f instead of the flat 0 under it",
			carved);

	//
	//	The shoreline mask.  Section 18 built TERRAIN_MASK_RIVER and TERRAIN_MASK_WATER_DISTANCE
	//	with nothing to stamp them; this is that writer, for every category, not only rivers.
	//
	Check (TerrainTextureSystem::Create_Masks (), "the masks would not be created");
	int stamped = WaterSystem::Stamp_All_Masks ();
	Check (stamped == 3, "%d of 3 placed areas stamped the shoreline mask", stamped);

	TerrainMaskClass *river_mask = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_RIVER);
	TerrainMaskClass *distance_mask = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_WATER_DISTANCE);
	Check ((river_mask != nullptr) && (distance_mask != nullptr), "the shoreline masks are missing");

	if ((river_mask != nullptr) && (distance_mask != nullptr)) {
		Check (river_mask->Get (10, 0) > 0.5f, "the stream's own line barely marks the river mask");
		Check (Near (distance_mask->Get (10, 0), 0.0f, 1.5f),
				"the stream is %f from the water it is standing in", distance_mask->Get (10, 0));
	}

	//
	//	No device, no scene -- but the missing-texture count still tells a caller which of its
	//	kinds have nothing to draw, exactly as FoliageSystem's does.
	//
	Check (!WaterSystem::Build_Geometry (pond_index), "geometry was built for a kind naming no art");
	Check (WaterSystem::Get_Missing_Texture_Count () == 1,
			"%d missing textures reported after one attempt", WaterSystem::Get_Missing_Texture_Count ());

	WaterDefinitionClass *stream_def = WaterSystem::Find_Definition ("ow_water_stream");
	if (stream_def != nullptr) {
		stream_def->Set_Surface_Texture ("ow_water_stream");
		Check (!WaterSystem::Build_Geometry (stream_index),
				"geometry was built for a textured kind with no physics scene to put it in");
		Check (WaterSystem::Get_Missing_Texture_Count () == 1,
				"a textured kind was counted as missing its art");
	}

	Check (WaterSystem::Get_Object_Count () == 0, "there are objects in a scene that is not there");
	Check (!WaterSystem::Has_Geometry (pond_index), "the pond says it has geometry with no scene built");

	//
	//	Nothing outlives the world it described.
	//
	WaterSystem::Shutdown ();
	Check (WaterSystem::Get_Area_Count () == 0, "the areas outlived the service");
	Check (WaterSystem::Get_Definition_Count () == 0, "the kinds outlived the service");
}


/*
**	Marks on the ground -- roadmap Section 23.
**
**	The acceptance is a claim about cost, not about looks, so this is mostly arithmetic: the
**	pool is a fixed size, a ribbon holds a fixed number of edges, and neither number moves no
**	matter how far anything is driven.  Everything here runs with no scene and no art, because
**	none of what is being claimed depends on either.
*/
void	Check_Ribbons (void)
{
	//
	//	The five kinds Section 23 names.
	//
	Check (::strcmp (Ribbon_Category_Name (RIBBON_TANK_TRACK), "tank track") == 0,
			 "the tank track is called %s", Ribbon_Category_Name (RIBBON_TANK_TRACK));
	Check (::strcmp (Ribbon_Category_Name (RIBBON_HARVESTER_TRACK), "harvester track") == 0,
			 "the harvester track is called %s", Ribbon_Category_Name (RIBBON_HARVESTER_TRACK));
	Check (::strcmp (Ribbon_Category_Name (RIBBON_DRAG_MARK), "drag mark") == 0,
			 "the drag mark is called %s", Ribbon_Category_Name (RIBBON_DRAG_MARK));

	//
	//	Soft ground keeps a mark, hard ground does not.  This is read off Renegade's own surface
	//	tagging, so a level that never heard of ribbons still says where tracks belong.
	//
	Check (Ribbon_Surface_Takes_Marks (SURFACE_TYPE_DIRT), "dirt keeps no track");
	Check (Ribbon_Surface_Takes_Marks (SURFACE_TYPE_SAND), "sand keeps no track");
	Check (Ribbon_Surface_Takes_Marks (SURFACE_TYPE_MUD), "mud keeps no track");
	Check (Ribbon_Surface_Takes_Marks (SURFACE_TYPE_SNOW), "snow keeps no track");
	Check (!Ribbon_Surface_Takes_Marks (SURFACE_TYPE_CONCRETE), "concrete takes a tread mark");
	Check (!Ribbon_Surface_Takes_Marks (SURFACE_TYPE_HEAVY_METAL), "steel plate takes a tread mark");
	Check (!Ribbon_Surface_Takes_Marks (SURFACE_TYPE_WATER), "water takes a tread mark");

	SurfaceRibbonSystem::Init ();

	//
	//	The pool exists before anything asks for it, and it is the only allocation there is.
	//
	const int pool = SurfaceRibbonSystem::Get_Pool_Size ();
	Check (pool > 0, "the pool is %d ribbons", pool);
	Check (SurfaceRibbonSystem::Get_Active_Ribbon_Count () == 0, "a fresh pool is already busy");
	Check (SurfaceRibbonSystem::Get_Edge_Count () == 0, "a fresh pool already has marks in it");

	SurfaceRibbonSystem::Define_Default_Ribbons ();
	Check (SurfaceRibbonSystem::Get_Definition_Count () == 5,
			 "there are %d kinds of mark, not five", SurfaceRibbonSystem::Get_Definition_Count ());

	int tank = SurfaceRibbonSystem::Find_Definition_Index ("ow_ribbon_tank_track");
	Check (tank >= 0, "the tank track is not defined");
	if (tank < 0) {
		SurfaceRibbonSystem::Shutdown ();
		return ;
	}

	const SurfaceRibbonDefinitionClass & tank_def = SurfaceRibbonSystem::Peek_Definition (tank);
	Check (tank_def.Get_Category () == RIBBON_TANK_TRACK, "the tank track is not a tank track");
	Check (!tank_def.Names_A_Texture (), "the tank track names art that does not exist");
	Check (SurfaceRibbonSystem::Find_Definition_By_Category (RIBBON_TANK_TRACK) == tank,
			 "asking for a tank track by category finds something else");

	//
	//	A ribbon cannot hold more edges than its kind allows, and no kind may exceed the buffer
	//	it lives in.  This is the cap, checked rather than trusted.
	//
	Check (tank_def.Get_Max_Edges () <= SURFACE_RIBBON_MAX_EDGES,
			 "a kind claims %d edges out of a buffer of %d",
			 tank_def.Get_Max_Edges (), SURFACE_RIBBON_MAX_EDGES);

	//
	//	Bind one, drive it in a straight line, and watch the ring fill and then stop filling.
	//
	int handle = SurfaceRibbonSystem::Bind (tank, 1234);
	Check (handle >= 0, "a fresh pool would not give out a ribbon");
	Check (SurfaceRibbonSystem::Is_Bound (handle), "the ribbon handed out is not bound");
	Check (SurfaceRibbonSystem::Find_Bound (1234) == handle, "the owner cannot find its own ribbon");

	const Vector3 forward (1.0f, 0.0f, 0.0f);
	const Vector3 up (0.0f, 0.0f, 1.0f);
	const float segment = tank_def.Get_Segment_Length ();

	bool laid = SurfaceRibbonSystem::Add_Point (handle, Vector3 (0.0f,0.0f,0.0f), forward, up, SURFACE_TYPE_DIRT);
	Check (laid, "the first contact laid no mark");
	Check (SurfaceRibbonSystem::Get_Edge_Count () == 1,
			 "the first contact laid %d marks", SurfaceRibbonSystem::Get_Edge_Count ());

	//	A vehicle that has barely moved does not lay another; this is what keeps a parked tank
	//	from spending its whole ring buffer standing still.
	laid = SurfaceRibbonSystem::Add_Point (handle, Vector3 (segment * 0.1f,0.0f,0.0f), forward, up, SURFACE_TYPE_DIRT);
	Check (!laid, "a mark was laid without the emitter moving a segment");
	Check (SurfaceRibbonSystem::Get_Edge_Count () == 1, "standing still spent the ring buffer");

	laid = SurfaceRibbonSystem::Add_Point (handle, Vector3 (segment,0.0f,0.0f), forward, up, SURFACE_TYPE_DIRT);
	Check (laid, "moving a full segment laid no mark");
	Check (SurfaceRibbonSystem::Get_Edge_Count () == 2, "moving a full segment laid the wrong number of marks");

	//	The strip is as wide as its kind says, across the direction of travel.
	const SurfaceRibbonClass * ribbon = SurfaceRibbonSystem::Peek_Ribbon (handle);
	Check (ribbon != nullptr, "a bound ribbon cannot be read");
	if (ribbon != nullptr) {
		float width = (ribbon->Peek_Edge (0).Left - ribbon->Peek_Edge (0).Right).Length ();
		Check (Near (width, tank_def.Get_Width (), 0.01f),
				 "the strip is %f wide and its kind says %f", width, tank_def.Get_Width ());
		Check (ribbon->Peek_Edge (0).StartsRun, "the first mark of a ribbon does not start its strip");
		Check (!ribbon->Peek_Edge (1).StartsRun, "the second mark started a new strip for no reason");
	}

	//	Drive a long way.  Far more contacts than the ring can hold, and the ring holds its cap.
	for (int step = 2; step < 400; step++) {
		SurfaceRibbonSystem::Add_Point (handle, Vector3 (segment * (float)step,0.0f,0.0f), forward, up, SURFACE_TYPE_DIRT);
	}
	int held = SurfaceRibbonSystem::Get_Edge_Count ();
	Check (held == tank_def.Get_Max_Edges (),
			 "four hundred contacts left %d marks in a ring of %d", held, tank_def.Get_Max_Edges ());
	Check (SurfaceRibbonSystem::Get_Pool_Size () == pool,
			 "driving grew the pool from %d to %d", pool, SurfaceRibbonSystem::Get_Pool_Size ());

	//
	//	Hard ground ends the run without ending the ribbon: the marks stay, and the next soft
	//	contact starts a fresh strip rather than being joined across the concrete.
	//
	laid = SurfaceRibbonSystem::Add_Point (handle, Vector3 (segment * 400.0f,0.0f,0.0f), forward, up, SURFACE_TYPE_CONCRETE);
	Check (!laid, "concrete took a tread mark");
	Check (SurfaceRibbonSystem::Get_Edge_Count () == held, "driving onto concrete erased the marks behind it");

	laid = SurfaceRibbonSystem::Add_Point (handle, Vector3 (segment * 401.0f,0.0f,0.0f), forward, up, SURFACE_TYPE_DIRT);
	Check (laid, "coming off concrete onto dirt laid no mark");
	ribbon = SurfaceRibbonSystem::Peek_Ribbon (handle);
	if (ribbon != nullptr) {
		Check (ribbon->Peek_Edge (ribbon->Get_Edge_Count () - 1).StartsRun,
				 "the mark after the concrete was joined to the one before it");
	}

	//
	//	A teleport is a jump larger than the kind allows, and needs nothing from the thing that
	//	jumped: the same Add_Point call decides it.
	//
	laid = SurfaceRibbonSystem::Add_Point (handle, Vector3 (5000.0f,5000.0f,0.0f), forward, up, SURFACE_TYPE_DIRT);
	Check (laid, "the mark at the far end of a teleport was never laid");
	ribbon = SurfaceRibbonSystem::Peek_Ribbon (handle);
	if (ribbon != nullptr) {
		Check (ribbon->Peek_Edge (ribbon->Get_Edge_Count () - 1).StartsRun,
				 "a teleport dragged a strip across the whole map");
	}

	//
	//	The acceptance in two numbers: many marks, and no object per mark.  There is no scene
	//	here, so nothing is drawn -- and the marks are still all there and still all bounded.
	//
	Check (SurfaceRibbonSystem::Get_Edge_Count () > 40,
			 "only %d marks exist to compare against the object count", SurfaceRibbonSystem::Get_Edge_Count ());
	Check (!SurfaceRibbonSystem::Build_Geometry (), "geometry was built with no scene to put it in");
	Check (SurfaceRibbonSystem::Get_Object_Count () == 0,
			 "%d objects exist in a scene that is not there", SurfaceRibbonSystem::Get_Object_Count ());
	Check (SurfaceRibbonSystem::Get_Missing_Texture_Count () == 1,
			 "%d kinds report missing art and one kind is laying marks",
			 SurfaceRibbonSystem::Get_Missing_Texture_Count ());

	//
	//	The pool cannot grow.  Take every slot, then ask for one more.
	//
	int failures_before = SurfaceRibbonSystem::Get_Bind_Failure_Count ();
	int taken = 0;
	for (int i = 0; i < pool + 8; i++) {
		int extra = SurfaceRibbonSystem::Bind (tank, 9000 + i);
		if (extra >= 0) {
			taken++;
		}
	}
	Check (SurfaceRibbonSystem::Get_Pool_Size () == pool,
			 "asking for %d ribbons out of a pool of %d grew it to %d",
			 pool + 8, pool, SurfaceRibbonSystem::Get_Pool_Size ());
	Check (taken <= pool, "the pool of %d handed out %d ribbons", pool, taken);
	Check (SurfaceRibbonSystem::Get_Bind_Failure_Count () > failures_before,
			 "the pool ran out and never said so");

	//
	//	Fade and lifetime.  One step past the end of a kind's life empties every ribbon of it,
	//	and an unbound ribbon that has emptied gives its slot back.
	//
	SurfaceRibbonSystem::Unbind (handle);
	Check (!SurfaceRibbonSystem::Is_Bound (handle), "an unbound ribbon is still bound");

	SurfaceRibbonSystem::Timestep (tank_def.Get_Lifetime () + 1.0f);
	Check (SurfaceRibbonSystem::Get_Edge_Count () == 0,
			 "%d marks outlived their kind", SurfaceRibbonSystem::Get_Edge_Count ());

	//
	//	Nothing outlives the world it marked.
	//
	SurfaceRibbonSystem::Shutdown ();
	Check (SurfaceRibbonSystem::Get_Pool_Size () == 0, "the pool outlived the service");
	Check (SurfaceRibbonSystem::Get_Definition_Count () == 0, "the kinds outlived the service");
	Check (SurfaceRibbonSystem::Get_Edge_Count () == 0, "the marks outlived the service");
}




/*
**	Marks on the world -- roadmap Section 35.
**
**	The acceptance is about cost and about bounds, so this is arithmetic again: the pool is a
**	fixed size, a group holds a fixed number of marks, and neither number moves however many
**	marks are thrown at it.  Everything here runs with no scene and no art, which is also how
**	the two paths through the manager get told apart -- with no world under it, a mark that
**	wants to drape onto the ground correctly refuses to be placed at all.
*/
void	Check_Marks (void)
{
	//
	//	The seven types Section 35 names.
	//
	Check (::strcmp (Surface_Mark_Type_Name (SURFACE_MARK_SCORCH), "SCORCH") == 0,
			 "the scorch is called %s", Surface_Mark_Type_Name (SURFACE_MARK_SCORCH));
	Check (::strcmp (Surface_Mark_Type_Name (SURFACE_MARK_OIL_OR_STAIN), "OIL_OR_STAIN") == 0,
			 "the stain is called %s", Surface_Mark_Type_Name (SURFACE_MARK_OIL_OR_STAIN));
	Check (::strcmp (Surface_Mark_Type_Name (SURFACE_MARK_DECORATIVE_WORLD_MARK),
							"DECORATIVE_WORLD_MARK") == 0,
			 "the world mark is called %s", Surface_Mark_Type_Name (SURFACE_MARK_DECORATIVE_WORLD_MARK));
	Check (SURFACE_MARK_TYPE_COUNT == 7, "there are %d mark types, not seven", SURFACE_MARK_TYPE_COUNT);

	//
	//	A mark that was placed outlives one that was thrown.
	//
	Check (Surface_Mark_Type_Is_Persistent (SURFACE_MARK_CONSTRUCTION),
			 "a construction mark expires like a scorch");
	Check (Surface_Mark_Type_Is_Persistent (SURFACE_MARK_DECORATIVE_WORLD_MARK),
			 "a world mark expires like a scorch");
	Check (!Surface_Mark_Type_Is_Persistent (SURFACE_MARK_SCORCH), "a scorch never expires");
	Check (!Surface_Mark_Type_Is_Persistent (SURFACE_MARK_IMPACT), "a bullet hole never expires");

	WorldSurfaceMarkManager::Init ();

	//
	//	The pool exists before anything asks for it, and it is the only allocation there is.
	//
	const int pool = WorldSurfaceMarkManager::Get_Pool_Size ();
	Check (pool > 0, "the pool is %d marks", pool);
	Check (WorldSurfaceMarkManager::Get_Mark_Count () == 0, "a fresh pool already holds marks");
	Check (WorldSurfaceMarkManager::Get_Object_Count () == 0, "a fresh pool already draws something");

	WorldSurfaceMarkManager::Define_Default_Marks ();
	Check (WorldSurfaceMarkManager::Get_Definition_Count () == 7,
			 "there are %d kinds of mark, not seven",
			 WorldSurfaceMarkManager::Get_Definition_Count ());

	int blast = WorldSurfaceMarkManager::Find_Definition_By_Type (SURFACE_MARK_BLAST);
	Check (blast >= 0, "the blast mark is not defined");
	if (blast >= 0) {
		const SurfaceMarkDefinitionClass &blast_def = WorldSurfaceMarkManager::Peek_Definition (blast);
		Check (!blast_def.Names_A_Texture (), "the blast mark names art that does not exist");
		Check (blast_def.Get_Lifetime () > 0.0f, "the blast mark never expires");
	}

	int construction = WorldSurfaceMarkManager::Find_Definition_By_Type (SURFACE_MARK_CONSTRUCTION);
	Check (construction >= 0, "the construction mark is not defined");
	if (construction >= 0) {
		Check (WorldSurfaceMarkManager::Peek_Definition (construction).Is_Persistent (),
				 "the construction mark expires");
	}

	//
	//	A kind that does not drape needs no ground under it, which is what lets the pool, the
	//	handles and the eviction policy all be checked with no world loaded.
	//
	SurfaceMarkDefinitionClass flat;
	flat.Set_Name ("ow_mark_selfcheck_flat");
	flat.Set_Type (SURFACE_MARK_SCORCH);
	flat.Set_Radius (1.0f);
	flat.Set_Lifetime (10.0f);
	flat.Set_Fade_Start (0.5f);
	flat.Set_Lift_Height (0.0f);
	flat.Set_Drape (false);

	int flat_index = WorldSurfaceMarkManager::Define_Definition (flat);
	Check (flat_index >= 0, "a flat kind could not be defined");
	if (flat_index < 0) {
		WorldSurfaceMarkManager::Shutdown ();
		return ;
	}

	//
	//	One mark.  It is a quad of the size it was asked for, and the handle finds it again.
	//
	uint32 handle = WorldSurfaceMarkManager::Add_Mark (flat_index, Vector3 (10.0f, 20.0f, 5.0f),
																		Vector3 (0.0f, 0.0f, 1.0f), 1.0f);
	Check (handle != 0, "a mark on flat ground was refused");
	Check (WorldSurfaceMarkManager::Is_Mark_Alive (handle), "the mark is not alive");
	Check (WorldSurfaceMarkManager::Get_Mark_Count () == 1,
			 "one mark placed and %d in the pool", WorldSurfaceMarkManager::Get_Mark_Count ());
	Check (WorldSurfaceMarkManager::Get_Batched_Mark_Count () == 1, "the mark did not go to the batch");
	Check (WorldSurfaceMarkManager::Get_Projected_Mark_Count () == 0, "the mark went to the clipper");

	const SurfaceMarkClass *mark = WorldSurfaceMarkManager::Peek_Mark (handle);
	Check (mark != nullptr, "the handle does not find the mark");
	if (mark != nullptr) {
		float width = (mark->Corner[1] - mark->Corner[0]).Length ();
		Check (WWMath::Fabs (width - 2.0f) < 0.01f, "the mark is %f across, not two", width);
		Check (!mark->Is_Projected (), "the mark thinks it was clipped");
	}

	//
	//	A handle to a mark that has gone stops matching, rather than addressing whatever took
	//	its slot.
	//
	Check (WorldSurfaceMarkManager::Remove_Mark (handle), "the mark could not be removed");
	Check (!WorldSurfaceMarkManager::Is_Mark_Alive (handle), "the removed mark is still alive");
	Check (!WorldSurfaceMarkManager::Remove_Mark (handle), "the mark was removed twice");
	Check (WorldSurfaceMarkManager::Peek_Mark (handle) == nullptr, "a stale handle still finds a mark");
	Check (WorldSurfaceMarkManager::Get_Mark_Count () == 0, "removing the only mark left one behind");

	//
	//	The bound.  Far more marks than any group can hold, thrown at it in one go: the group
	//	stops growing, the pool never does, and every mark that left was evicted on purpose.
	//
	const int before_evictions = WorldSurfaceMarkManager::Get_Eviction_Count ();
	for (int i = 0; i < (SURFACE_MARK_MAX_PER_GROUP * 3); i ++) {
		WorldSurfaceMarkManager::Add_Mark (flat_index,
													 Vector3 (float (i), 0.0f, 0.0f),
													 Vector3 (0.0f, 0.0f, 1.0f), 1.0f);
	}

	Check (WorldSurfaceMarkManager::Get_Group_Mark_Count (flat_index) <= SURFACE_MARK_MAX_PER_GROUP,
			 "one group holds %d marks, more than its mesh was built for",
			 WorldSurfaceMarkManager::Get_Group_Mark_Count (flat_index));
	Check (WorldSurfaceMarkManager::Get_Mark_Count () <= pool,
			 "the pool holds %d marks out of %d", WorldSurfaceMarkManager::Get_Mark_Count (), pool);
	Check (WorldSurfaceMarkManager::Get_Pool_Size () == pool, "the pool grew to %d",
			 WorldSurfaceMarkManager::Get_Pool_Size ());
	Check (WorldSurfaceMarkManager::Get_Eviction_Count () > before_evictions,
			 "nothing was evicted while the group overflowed");

	//
	//	Nothing was drawn, because nothing names a texture -- and with no scene there is nothing
	//	to draw into either.  The marks are all still there and still counted.
	//
	Check (!WorldSurfaceMarkManager::Build_Geometry (), "geometry was built with no scene");
	Check (WorldSurfaceMarkManager::Get_Object_Count () == 0, "%d objects were made with no scene",
			 WorldSurfaceMarkManager::Get_Object_Count ());
	Check (WorldSurfaceMarkManager::Get_Missing_Texture_Count () >= 1,
			 "a group with marks and no texture was not reported");
	Check (!WorldSurfaceMarkManager::Has_Geometry (flat_index), "a group with no texture has a mesh");

	//
	//	The clock.  Past the lifetime every mark of that kind is gone, and gone without the
	//	pool having grown to hold them in the meantime.
	//
	WorldSurfaceMarkManager::Timestep (11.0f);
	Check (WorldSurfaceMarkManager::Get_Group_Mark_Count (flat_index) == 0,
			 "%d marks outlived their lifetime",
			 WorldSurfaceMarkManager::Get_Group_Mark_Count (flat_index));
	Check (WorldSurfaceMarkManager::Get_Pool_Size () == pool, "the pool grew while marks expired");

	//
	//	A mark that wants to drape needs ground under it.  With no world there is none, so it is
	//	refused rather than placed somewhere invented -- and the refusal costs no slot.
	//
	SurfaceMarkDefinitionClass draped;
	draped.Set_Name ("ow_mark_selfcheck_draped");
	draped.Set_Type (SURFACE_MARK_DIRT);
	draped.Set_Radius (1.0f);
	draped.Set_Lifetime (10.0f);
	draped.Set_Drape (true);

	int draped_index = WorldSurfaceMarkManager::Define_Definition (draped);
	Check (draped_index >= 0, "a draping kind could not be defined");
	if (draped_index >= 0) {
		int held = WorldSurfaceMarkManager::Get_Mark_Count ();
		uint32 nowhere = WorldSurfaceMarkManager::Add_Mark (draped_index,
																			Vector3 (0.0f, 0.0f, 0.0f),
																			Vector3 (0.0f, 0.0f, 1.0f), 1.0f);
		Check (nowhere == 0, "a mark draped onto ground that is not there");
		Check (WorldSurfaceMarkManager::Get_Mark_Count () == held,
				 "the refused mark kept a slot anyway");
	}

	//
	//	The group table is capped too.  Content that names more decal textures than the table
	//	holds gets counted refusals, not a table that grows with the content.
	//
	const int before_refusals = WorldSurfaceMarkManager::Get_Group_Refusal_Count ();
	for (int g = 0; g < (SURFACE_MARK_MAX_GROUPS + 8); g ++) {
		StringClass name;
		name.Format ("ow_mark_selfcheck_group_%d", g);
		WorldSurfaceMarkManager::Find_Or_Define_Texture_Group (name, SURFACE_MARK_SCORCH, 1.0f);
	}
	Check (WorldSurfaceMarkManager::Get_Definition_Count () <= SURFACE_MARK_MAX_GROUPS,
			 "the group table grew to %d", WorldSurfaceMarkManager::Get_Definition_Count ());
	Check (WorldSurfaceMarkManager::Get_Group_Refusal_Count () > before_refusals,
			 "the group table overflowed without saying so");

	//
	//	The world going away takes the marks with it.
	//
	WorldSurfaceMarkManager::Clear_Marks ();
	Check (WorldSurfaceMarkManager::Get_Mark_Count () == 0, "%d marks survived the world unloading",
			 WorldSurfaceMarkManager::Get_Mark_Count ());

	WorldSurfaceMarkManager::Shutdown ();
	Check (WorldSurfaceMarkManager::Get_Pool_Size () == 0, "the pool outlived the service");
	Check (WorldSurfaceMarkManager::Get_Mark_Count () == 0, "the marks outlived the service");
	Check (WorldSurfaceMarkManager::Get_Definition_Count () == 0, "the kinds outlived the service");
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
	if ((which == nullptr) || (::strcmp (which, "roads") == 0)) {
		Check_Roads ();
	}
	if ((which == nullptr) || (::strcmp (which, "bridges") == 0)) {
		Check_Bridges ();
	}
	if ((which == nullptr) || (::strcmp (which, "foliage") == 0)) {
		Check_Foliage ();
	}
	if ((which == nullptr) || (::strcmp (which, "water") == 0)) {
		Check_Water ();
	}
	if ((which == nullptr) || (::strcmp (which, "ribbons") == 0)) {
		Check_Ribbons ();
	}
	if ((which == nullptr) || (::strcmp (which, "marks") == 0)) {
		Check_Marks ();
	}

	if (_Failures == 0) {
		::fprintf (stdout, "terrain self check: pass\n");
	} else {
		::fprintf (stdout, "terrain self check: %d failure(s)\n", _Failures);
	}

	return (_Failures == 0) ? 0 : 1;
}
