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

	if (_Failures == 0) {
		::fprintf (stdout, "terrain self check: pass\n");
	} else {
		::fprintf (stdout, "terrain self check: %d failure(s)\n", _Failures);
	}

	return (_Failures == 0) ? 0 : 1;
}
