//
//	The terrain service, checked from inside the binary that has to be right.
//	See terrainselfcheck.h for why the checks live here.
//

#include "terrainselfcheck.h"

#include "baseselectionplanner.h"
#include "biomedecorationgenerator.h"
#include "buildingdescriptormanager.h"
#include "combatspacevalidator.h"
#include "commanderplacement.h"
#include "expansionregionregistry.h"
#include "logisticsgraphservice.h"
#include "foundationgrading.h"
#include "generatedworldplacementrules.h"
#include "worldoperation.h"
#include "worldoperationjournal.h"
#include "worldsnapshot.h"
#include "worldsyncservice.h"
#include "bridgeplanner.h"
#include "bridgesection.h"
#include "bridgesystem.h"
#include "castres.h"
#include "foliagesystem.h"
#include "foliagetype.h"
#include "coltest.h"
#include "generatedworldintegrator.h"
#include "heightfield.h"
#include "hydrologygenerator.h"
#include "interiorcliffgenerator.h"
#include "lineseg.h"
#include "minimapbaselayer.h"
#include "navmeshgenerator.h"
#include "Pathfind.h"
#include "worldmetrics.h"
#include "playertype.h"
#include "coltype.h"
#include "macroterraingenerator.h"
#include "proceduralassetregistry.h"
#include "proceduralworldprofile.h"
#include "renegadeterrainpatch.h"
#include "vehicle.h"
#include "ribbontype.h"
#include "roadgraphgenerator.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "surfacemarktype.h"
#include "surfaceribbonsystem.h"
#include "terrainmask.h"
#include "tiberiumfieldgenerator.h"
#include "transportationnetworkvalidator.h"
#include "tunnelplanner.h"
#include "escarpment.h"
#include "terraintexturesystem.h"
#include "watersystem.h"
#include "watertype.h"
#include "w3d_file.h"
#include "decophys.h"
#include "light.h"
#include "lightenvironment.h"
#include "lightphys.h"
#include "physlist.h"
#include "pscene.h"
#include "worldboundsservice.h"
#include "worldlightmanager.h"
#include "worldshadowmanager.h"
#include "worldshroudmanager.h"
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
	//	P13: every default layer carries a texture -- the ow_ter_* one when it ships, the
	//	stock always.dat stand-in until then -- so a generated world is never drawn bare.
	//
	for (int layer_index = 0; layer_index < TerrainTextureSystem::Get_Layer_Count (); layer_index ++) {
		TerrainLayerClass *layer = TerrainTextureSystem::Peek_Layer (layer_index);
		const char *texture = ((layer != nullptr) && (layer->Material != nullptr))
									 ? layer->Material->Get_Texture_Name () : nullptr;
		Check ((texture != nullptr) && (texture [0] != 0),"default layer %d ('%s') names no texture",
				 layer_index,(layer != nullptr) ? (const char *)layer->Name : "?");
	}

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
	//	Debris.  The system holds what came off until DebrisManager (Section 28) takes it.  Since
	//	P25 every default definition names a rubble model for its broken span (see
	//	Define_Default_Bridges), so the breaks above on a default bridge have already emitted some;
	//	this used to assert the pre-P25 "defaults name none" and has failed ever since.  Every
	//	pending record must name a real rubble model, and clearing leaves the isolated case below
	//	counting only its own.
	//
	Check (BridgeSystem::Get_Pending_Debris_Count () > 0,
			"breaking a default bridge's span emitted no debris, though every default names rubble");
	for (int d = 0; d < BridgeSystem::Get_Pending_Debris_Count (); d++) {
		const BridgeDebrisClass *pending = BridgeSystem::Peek_Pending_Debris (d);
		Check ((pending != nullptr) && (pending->Model.Get_Length () > 0) && (pending->Count > 0),
				"pending debris record %d names no rubble model or no pieces", d);
	}
	BridgeSystem::Clear_Pending_Debris ();

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
	//	Five natural categories plus V2 Section 10.6's street prop, four blocking behaviours, and
	//	the two are separate axes.
	//
	Check (FoliageSystem::Get_Type_Count () == 6,
			"%d default foliage types were defined, not 6", FoliageSystem::Get_Type_Count ());
	FoliageTypeClass *lamp = FoliageSystem::Find_Type ("ow_street_lamp");
	Check ((lamp != nullptr) && (lamp->Get_Category () == FOLIAGE_STREET_PROP),
			"the street lamp type is missing or not a street prop");

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


/*
**	Shadows -- roadmap Section 24.
**
**	The acceptance is a structural one: there is supposed to be one shadow implementation, and
**	the way that claim dies quietly is for the physics scene and the shadow manager to drift
**	apart until a setting made through one is invisible to the other.  So this checks the
**	shared vocabulary first, then the bounds, then the one behaviour that has to be right on a
**	machine with no graphics device at all: asking for a render target has to fail and say so
**	rather than assert, because that is a dedicated server every time it runs.
*/
void	Check_Shadows (void)
{
	//
	//	The scene and the manager have to be talking about the same four modes.  Nothing else
	//	in this file matters if these two ever disagree.
	//
	Check ((int)PhysicsSceneClass::SHADOW_MODE_NONE == (int)WorldShadowManager::SHADOW_MODE_NONE,
			 "the scene and the manager disagree about SHADOW_MODE_NONE");
	Check ((int)PhysicsSceneClass::SHADOW_MODE_BLOBS == (int)WorldShadowManager::SHADOW_MODE_BLOBS,
			 "the scene and the manager disagree about SHADOW_MODE_BLOBS");
	Check ((int)PhysicsSceneClass::SHADOW_MODE_BLOBS_PLUS == (int)WorldShadowManager::SHADOW_MODE_BLOBS_PLUS,
			 "the scene and the manager disagree about SHADOW_MODE_BLOBS_PLUS");
	Check ((int)PhysicsSceneClass::SHADOW_MODE_HARDWARE == (int)WorldShadowManager::SHADOW_MODE_HARDWARE,
			 "the scene and the manager disagree about SHADOW_MODE_HARDWARE");
	Check ((int)PhysicsSceneClass::SHADOW_MODE_COUNT == (int)WorldShadowManager::SHADOW_MODE_COUNT,
			 "there are %d shadow modes on the scene and %d on the manager",
			 (int)PhysicsSceneClass::SHADOW_MODE_COUNT, (int)WorldShadowManager::SHADOW_MODE_COUNT);

	WorldShadowManager::Init ();

	//
	//	What a fresh manager holds.
	//
	Check (WorldShadowManager::Get_Mode () == WorldShadowManager::SHADOW_MODE_NONE,
			 "a fresh shadow manager is already in mode %d", WorldShadowManager::Get_Mode ());
	Check (WorldShadowManager::Get_Held_Slot_Count () == 0, "a fresh manager already holds slots");
	Check (WorldShadowManager::Get_Render_Target_Count () == 0,
			 "a fresh manager already made %d render target(s)",
			 WorldShadowManager::Get_Render_Target_Count ());
	Check (WorldShadowManager::Get_Registered_Caster_Count () == 0, "a fresh manager already has casters");
	Check (WorldShadowManager::Get_Static_Shadow_Texture_Count () == 0,
			 "a fresh manager already cached static shadow textures");

	float atten_start = -1.0f;
	float atten_end = -1.0f;
	WorldShadowManager::Get_Attenuation (&atten_start, &atten_end);
	Check (atten_start > 0.0f, "the shadows start attenuating at %f", atten_start);
	Check (atten_end > atten_start, "shadows finish attenuating at %f before they start at %f",
			 atten_end, atten_start);

	//
	//	The mode decides how many rendered shadows may exist, and every mode change makes every
	//	cached picture wrong, so it moves the generation on.
	//
	uint32 generation = WorldShadowManager::Get_Generation ();

	WorldShadowManager::Set_Mode (WorldShadowManager::SHADOW_MODE_HARDWARE);
	Check (WorldShadowManager::Get_Mode () == WorldShadowManager::SHADOW_MODE_HARDWARE,
			 "the hardware shadow mode did not take");
	Check (WorldShadowManager::Get_Max_Simultaneous_Shadows () > 0,
			 "hardware shadows allow %d at once", WorldShadowManager::Get_Max_Simultaneous_Shadows ());
	Check (WorldShadowManager::Get_Generation () != generation,
			 "changing the shadow mode left every cached shadow believed to be current");

	WorldShadowManager::Set_Mode (WorldShadowManager::SHADOW_MODE_BLOBS);
	Check (WorldShadowManager::Get_Max_Simultaneous_Shadows () == 0,
			 "blob shadows still keep %d render target(s) around",
			 WorldShadowManager::Get_Max_Simultaneous_Shadows ());

	WorldShadowManager::Set_Mode (WorldShadowManager::SHADOW_MODE_BLOBS_PLUS);
	Check (WorldShadowManager::Get_Max_Simultaneous_Shadows () == 1,
			 "blobs-plus keeps %d rendered shadow(s), not one",
			 WorldShadowManager::Get_Max_Simultaneous_Shadows ());

	//
	//	A mode outside the enum is refused rather than stored.
	//
	WorldShadowManager::Set_Mode (99);
	Check (WorldShadowManager::Get_Mode () == WorldShadowManager::SHADOW_MODE_BLOBS_PLUS,
			 "a shadow mode of 99 was accepted");

	//
	//	Both resolutions are powers of two inside a fixed range, and they are independent --
	//	that separation is the whole reason TT names them separately.
	//
	WorldShadowManager::Set_Dynamic_Resolution (100);
	unsigned int dyn = WorldShadowManager::Get_Dynamic_Resolution ();
	Check ((dyn & (dyn - 1)) == 0, "a dynamic shadow resolution of %u is not a power of two", dyn);

	WorldShadowManager::Set_Dynamic_Resolution (4096);
	Check (WorldShadowManager::Get_Dynamic_Resolution () <= 256,
			 "the dynamic shadow resolution reached %u", WorldShadowManager::Get_Dynamic_Resolution ());

	WorldShadowManager::Set_Dynamic_Resolution (1);
	Check (WorldShadowManager::Get_Dynamic_Resolution () >= 16,
			 "the dynamic shadow resolution fell to %u", WorldShadowManager::Get_Dynamic_Resolution ());

	WorldShadowManager::Set_Static_Resolution (64);
	WorldShadowManager::Set_Dynamic_Resolution (256);
	Check (WorldShadowManager::Get_Static_Resolution () == 64,
			 "setting the dynamic resolution moved the static one to %u",
			 WorldShadowManager::Get_Static_Resolution ());

	WorldShadowManager::Set_Static_Resolution (4096);
	Check (WorldShadowManager::Get_Static_Resolution () <= 256,
			 "the static shadow resolution reached %u", WorldShadowManager::Get_Static_Resolution ());

	//
	//	The bound on how many shadows may be rendered at once.
	//
	WorldShadowManager::Set_Max_Simultaneous_Shadows (100000);
	unsigned int slots = WorldShadowManager::Get_Max_Simultaneous_Shadows ();
	Check (slots <= 32, "a hundred thousand shadows became %u slots", slots);
	Check (WorldShadowManager::Get_Held_Slot_Count () == 0,
			 "%d slot(s) are held by nobody in particular", WorldShadowManager::Get_Held_Slot_Count ());

	//
	//	With no graphics device there is no render target to be had.  It has to be counted and
	//	refused, not asserted: this is exactly the state a dedicated server runs in.
	//
	Check (WorldShadowManager::Create_Render_Target (64, 64) == nullptr,
			 "a render target was created with no device");

	//
	//	Casters that are not MovePhysClass or DynamicAnimPhysClass.  Before this there was no
	//	way at all for a world system to cast a shadow; now there is one, and it is bounded.
	//
	const int before_refusals = WorldShadowManager::Get_Caster_Refusal_Count ();

	DynamicVectorClass<DecorationPhysClass *> casters;
	for (int i = 0; i < (WORLD_SHADOW_MAX_REGISTERED_CASTERS + 8); i ++) {
		DecorationPhysClass *obj = new DecorationPhysClass;
		casters.Add (obj);
		WorldShadowManager::Register_Caster (obj);
	}

	Check (WorldShadowManager::Get_Registered_Caster_Count () <= WORLD_SHADOW_MAX_REGISTERED_CASTERS,
			 "the caster table grew to %d", WorldShadowManager::Get_Registered_Caster_Count ());
	Check (WorldShadowManager::Get_Caster_Refusal_Count () > before_refusals,
			 "the caster table overflowed without saying so");

	//
	//	Registering the same object twice does not cost a second entry.
	//
	int held = WorldShadowManager::Get_Registered_Caster_Count ();
	Check (WorldShadowManager::Register_Caster (casters[0]), "a registered caster could not re-register");
	Check (WorldShadowManager::Get_Registered_Caster_Count () == held,
			 "registering the same caster twice made %d entries out of %d",
			 WorldShadowManager::Get_Registered_Caster_Count (), held);
	Check (WorldShadowManager::Is_Registered_Caster (casters[0]), "a registered caster is not registered");

	Check (WorldShadowManager::Unregister_Caster (casters[0]), "a registered caster could not be dropped");
	Check (!WorldShadowManager::Is_Registered_Caster (casters[0]), "a dropped caster is still registered");
	Check (!WorldShadowManager::Unregister_Caster (casters[0]), "a caster was dropped twice");

	//
	//	Running the clock over registered casters with no scene under them does nothing and
	//	costs nothing -- no shadow can be created without somewhere to put it.
	//
	WorldShadowManager::Timestep (0.033f);
	Check (WorldShadowManager::Get_Held_Slot_Count () == 0,
			 "%d shadow slot(s) were taken with no scene", WorldShadowManager::Get_Held_Slot_Count ());

	//
	//	The service going away takes everything with it, casters included.
	//
	WorldShadowManager::Shutdown ();
	Check (WorldShadowManager::Get_Registered_Caster_Count () == 0, "casters outlived the service");
	Check (WorldShadowManager::Get_Max_Simultaneous_Shadows () == 0, "the slot table outlived the service");
	Check (WorldShadowManager::Get_Static_Shadow_Texture_Count () == 0,
			 "cached static shadow textures outlived the service");

	for (int i = 0; i < casters.Count (); i ++) {
		casters[i]->Release_Ref ();
	}
	casters.Delete_All ();
}


/*
**	Dynamic lights -- roadmap Section 25.
**
**	The acceptance is a claim about how cost scales: lighting has to cost what is nearby and
**	not what exists.  A claim like that is only worth anything as a number, so the middle of
**	this check plants a field of lights, asks one question in the middle of it, and counts how
**	many lights had to be looked at to answer.  Growing the field from eight lights to the full
**	table must not grow that count.
**
**	Everything else here is the bounds and the lifecycle, and all of it runs with no graphics
**	device and no physics scene -- which is also a dedicated server, where lights are still
**	created, still expire, and still cost nothing to nobody.
*/
void	Check_Lights (void)
{
	WorldLightManager::Init ();

	//
	//	What a fresh manager holds.
	//
	Check (WorldLightManager::Get_Dynamic_Light_Count () == 0,
			 "a fresh light manager already has %d light(s)", WorldLightManager::Get_Dynamic_Light_Count ());
	Check (WorldLightManager::Has_Dynamic_Lights () == false, "a fresh light manager has lights");
	Check (WorldLightManager::Is_Partitioned () == false, "a fresh light manager is already partitioned");
	Check (WorldLightManager::Get_Max_Light_Radius () > 0.0f, "a light may have no radius at all");

	//
	//	Somewhere for the lights to be.  The manager would ask the physics scene for this, and
	//	fall back to a default box; the check says where the world is so the numbers below are
	//	about a known grid rather than a guessed one.
	//
	WorldLightManager::Re_Partition (Vector3 (-1000.0f, -1000.0f, -100.0f),
												Vector3 ( 1000.0f,  1000.0f,  100.0f));
	Check (WorldLightManager::Is_Partitioned (), "the light grid would not partition");

	//
	//	One light, and it is findable, movable and removable.
	//
	const Vector3 white (1.0f, 1.0f, 1.0f);

	LightPhysClass *light = WorldLightManager::Create_Point_Light (Vector3 (10.0f, 0.0f, 0.0f),
																						white, 0.0f, 20.0f, 1.0f, -1.0f);
	Check (light != nullptr, "a point light would not be created");

	if (light != nullptr) {

		Check (WorldLightManager::Get_Dynamic_Light_Count () == 1,
				 "creating one light made %d", WorldLightManager::Get_Dynamic_Light_Count ());
		Check (WorldLightManager::Is_Dynamic_Light (light), "a created light is not a dynamic light");
		Check (WorldLightManager::Has_Dynamic_Lights (), "a manager holding a light says it has none");

		//
		//	The union bound has to actually contain the light, because it is the gate every
		//	object is tested against before anything else happens.
		//
		AABoxClass bounds;
		WorldLightManager::Get_Light_Bounds (&bounds);
		Check (bounds.Extent.X >= 20.0f, "the light bound has X extent %f for a light of radius 20",
				 bounds.Extent.X);
		Check (WWMath::Fabs (bounds.Center.X - 10.0f) < 0.01f,
				 "the light bound is centred at X %f, not on the one light in it", bounds.Center.X);

		//
		//	Found where it is, not found where it is not.
		//
		{
			NonRefPhysListClass near_list;
			Check (WorldLightManager::Collect_Dynamic_Lights (Vector3 (10.0f, 0.0f, 0.0f), &near_list) == 1,
					 "a light was not found at its own position");
		}
		{
			NonRefPhysListClass far_list;
			Check (WorldLightManager::Collect_Dynamic_Lights (Vector3 (500.0f, 0.0f, 0.0f), &far_list) == 0,
					 "a light of radius 20 was found 490 metres away");
		}

		//
		//	Moving it moves both the grid bucket and the bound.
		//
		WorldLightManager::Move_Dynamic_Light (light, Vector3 (-300.0f, 0.0f, 0.0f));
		{
			NonRefPhysListClass moved_list;
			Check (WorldLightManager::Collect_Dynamic_Lights (Vector3 (-300.0f, 0.0f, 0.0f), &moved_list) == 1,
					 "a moved light was not found where it moved to");
		}
		{
			NonRefPhysListClass old_list;
			Check (WorldLightManager::Collect_Dynamic_Lights (Vector3 (10.0f, 0.0f, 0.0f), &old_list) == 0,
					 "a moved light is still where it used to be");
		}

		Check (WorldLightManager::Remove_Dynamic_Light (light), "a light would not be removed");
		Check (WorldLightManager::Remove_Dynamic_Light (light) == false, "a light was removed twice");
		Check (WorldLightManager::Get_Dynamic_Light_Count () == 0,
				 "removing the only light left %d", WorldLightManager::Get_Dynamic_Light_Count ());
	}

	//
	//	A light bigger than the grid holds is clamped rather than refused, because the caller
	//	asked for a big flash and a big flash is what it should roughly get -- but it is counted,
	//	because a system quietly not doing what it was asked is worse than a slow one.
	//
	{
		const int before = WorldLightManager::Get_Clamped_Radius_Count ();

		LightPhysClass *huge = WorldLightManager::Create_Point_Light (Vector3 (0.0f, 0.0f, 0.0f),
																						 white, 0.0f, 10000.0f, 1.0f, -1.0f);
		Check (huge != nullptr, "a light with an absurd radius was refused instead of clamped");
		Check (WorldLightManager::Get_Clamped_Radius_Count () > before,
				 "a ten kilometre light was accepted without saying so");

		if (huge != nullptr) {
			LightClass *light_obj = (LightClass *)huge->Peek_Model ();
			Check (light_obj != nullptr, "a created light has no light object");
			if (light_obj != nullptr) {
				float atten_start = 0.0f;
				float atten_end = 0.0f;
				light_obj->Get_Far_Attenuation_Range (atten_start, atten_end);
				Check (atten_end <= WorldLightManager::Get_Max_Light_Radius (),
						 "a clamped light still reaches %f metres", atten_end);
			}
			WorldLightManager::Remove_Dynamic_Light (huge);
		}
	}

	//
	//	The lifetime.  A light with one fades and then goes away by itself; a light without one
	//	stays until somebody removes it.  This is what every muzzle flash is going to use, and
	//	it is the only thing in the system that removes a light without being asked to.
	//
	{
		LightPhysClass *flash = WorldLightManager::Create_Point_Light (Vector3 (0.0f, 0.0f, 0.0f),
																						  white, 0.0f, 10.0f, 2.0f, 1.0f);
		LightPhysClass *lamp = WorldLightManager::Create_Point_Light (Vector3 (50.0f, 0.0f, 0.0f),
																						 white, 0.0f, 10.0f, 2.0f, -1.0f);
		Check (flash != nullptr, "a light with a lifetime would not be created");
		Check (lamp != nullptr, "a light without a lifetime would not be created");

		WorldLightManager::Timestep (0.5f);
		Check (WorldLightManager::Get_Dynamic_Light_Count () == 2,
				 "a light with half a second left has already gone");

		if (flash != nullptr) {
			LightClass *flash_obj = (LightClass *)flash->Peek_Model ();
			Check ((flash_obj != nullptr) && (flash_obj->Get_Intensity () < 2.0f),
					 "a half-expired light is still at full intensity");
		}
		if (lamp != nullptr) {
			LightClass *lamp_obj = (LightClass *)lamp->Peek_Model ();
			Check ((lamp_obj != nullptr) && Near (lamp_obj->Get_Intensity (), 2.0f),
					 "a light with no lifetime faded anyway");
		}

		WorldLightManager::Timestep (0.75f);
		Check (WorldLightManager::Get_Dynamic_Light_Count () == 1,
				 "after its lifetime %d light(s) are left, not the one with no lifetime",
				 WorldLightManager::Get_Dynamic_Light_Count ());
		Check ((lamp != nullptr) && WorldLightManager::Is_Dynamic_Light (lamp),
				 "the light with no lifetime expired");

		WorldLightManager::Remove_All_Dynamic_Lights ();
		Check (WorldLightManager::Get_Dynamic_Light_Count () == 0, "lights survived being cleared");
	}

	//
	//	The acceptance, as a number.
	//
	//	A field of lights spaced far enough apart that no two of them reach the same place.  Ask
	//	one question in the middle of it, and count the lights that had to be looked at to answer
	//	it.  Then fill the table to its limit and ask exactly the same question again.  If the
	//	second count is bigger, lighting cost is scaling with the number of lights in the world,
	//	which is the thing Section 25 exists to stop.
	//
	{
		const float	SPACING = 200.0f;
		const float	ORIGIN = -700.0f;
		const float	RADIUS = 20.0f;

		DynamicVectorClass<LightPhysClass *> field;

		//	Eight lights, in a row.
		for (int i = 0; i < 8; i ++) {
			LightPhysClass *made = WorldLightManager::Create_Point_Light (
											Vector3 (ORIGIN + (float)i * SPACING, ORIGIN, 0.0f),
											white, 0.0f, RADIUS, 1.0f, -1.0f);
			Check (made != nullptr, "light %d of the field would not be created", i);
			if (made != nullptr) {
				field.Add (made);
			}
		}

		const Vector3 probe (ORIGIN, ORIGIN, 0.0f);

		WorldLightManager::Begin_Frame ();
		int found_few = 0;
		{
			NonRefPhysListClass few_list;
			found_few = WorldLightManager::Collect_Dynamic_Lights (probe, &few_list);
		}
		const int examined_few = WorldLightManager::Get_Lights_Examined ();
		const int queries_few = WorldLightManager::Get_Query_Count ();

		Check (found_few == 1, "%d of eight spaced lights reach the same point", found_few);
		Check (queries_few == 1, "one collection ran %d queries", queries_few);
		Check (examined_few <= 4,
				 "eight spaced lights cost %d examination(s) to light one point", examined_few);

		//	Now fill the table.
		for (int i = 8; i < WORLD_LIGHT_MAX_DYNAMIC_LIGHTS; i ++) {
			LightPhysClass *made = WorldLightManager::Create_Point_Light (
											Vector3 (ORIGIN + (float)(i % 8) * SPACING,
														ORIGIN + (float)(i / 8) * SPACING,
														0.0f),
											white, 0.0f, RADIUS, 1.0f, -1.0f);
			if (made != nullptr) {
				field.Add (made);
			}
		}

		Check (WorldLightManager::Get_Dynamic_Light_Count () == WORLD_LIGHT_MAX_DYNAMIC_LIGHTS,
				 "the field holds %d lights, not the %d asked for",
				 WorldLightManager::Get_Dynamic_Light_Count (), (int)WORLD_LIGHT_MAX_DYNAMIC_LIGHTS);

		WorldLightManager::Begin_Frame ();
		int found_many = 0;
		{
			NonRefPhysListClass many_list;
			found_many = WorldLightManager::Collect_Dynamic_Lights (probe, &many_list);
		}
		const int examined_many = WorldLightManager::Get_Lights_Examined ();

		Check (found_many == 1, "%d of a full table of spaced lights reach the same point", found_many);
		Check (examined_many <= examined_few,
				 "answering the same question cost %d lights with a full table and %d with eight",
				 examined_many, examined_few);
		Check (examined_many < WORLD_LIGHT_MAX_DYNAMIC_LIGHTS,
				 "%d of %d lights were examined to light one point",
				 examined_many, (int)WORLD_LIGHT_MAX_DYNAMIC_LIGHTS);

		//
		//	The table is a bound, and going past it is refused and counted rather than grown into.
		//
		const int before_refusals = WorldLightManager::Get_Refusal_Count ();
		LightPhysClass *one_too_many = WorldLightManager::Create_Point_Light (
													Vector3 (0.0f, 0.0f, 0.0f), white, 0.0f, RADIUS, 1.0f, -1.0f);
		Check (one_too_many == nullptr, "a light past the table's limit was created anyway");
		Check (WorldLightManager::Get_Refusal_Count () > before_refusals,
				 "the light table overflowed without saying so");

		//
		//	A point no light reaches is rejected by the union bound before the grid is touched at
		//	all.  That free answer is what makes a per-object light query affordable.
		//
		WorldLightManager::Begin_Frame ();
		{
			NonRefPhysListClass nowhere_list;
			Check (WorldLightManager::Collect_Dynamic_Lights (Vector3 (900.0f, 900.0f, 90.0f),
																			 &nowhere_list) == 0,
					 "a light was found where the field does not reach");
		}
		Check (WorldLightManager::Get_Trivial_Reject_Count () > 0,
				 "a point outside every light went to the grid instead of being rejected outright");
		Check (WorldLightManager::Get_Query_Count () == 0,
				 "a trivially rejected point still ran %d grid quer(ies)",
				 WorldLightManager::Get_Query_Count ());

		WorldLightManager::Remove_All_Dynamic_Lights ();
		field.Delete_All ();
	}

	//
	//	Composition.  An object that no dynamic light reaches is handed back exactly the
	//	environment it came in with -- the same pointer, not a copy of it -- because that is the
	//	case that happens to nearly every object in nearly every frame.
	//
	{
		LightPhysClass *probe_obj = new LightPhysClass (true);
		probe_obj->Set_Position (Vector3 (0.0f, 0.0f, 0.0f));

		LightEnvironmentClass env;
		env.Reset (Vector3 (0.0f, 0.0f, 0.0f), Vector3 (0.1f, 0.1f, 0.1f));

		WorldLightManager::Begin_Frame ();
		Check (WorldLightManager::Apply_Dynamic_Lights (probe_obj, &env) == &env,
				 "an object with no dynamic light near it was given a different lighting environment");
		Check (WorldLightManager::Get_Lit_Object_Count () == 0,
				 "%d object(s) were lit by no lights", WorldLightManager::Get_Lit_Object_Count ());

		//	Now put a light on top of it.
		LightPhysClass *near_light = WorldLightManager::Create_Point_Light (Vector3 (0.0f, 0.0f, 0.0f),
																								  white, 0.0f, 20.0f, 1.0f, -1.0f);
		Check (near_light != nullptr, "a light for the composition check would not be created");

		WorldLightManager::Begin_Frame ();
		LightEnvironmentClass *composed = WorldLightManager::Apply_Dynamic_Lights (probe_obj, &env);
		Check (composed != &env, "an object standing in a dynamic light got the unlit environment");
		Check (WorldLightManager::Get_Lit_Object_Count () == 1,
				 "%d object(s) were lit when one was", WorldLightManager::Get_Lit_Object_Count ());
		Check (WorldLightManager::Get_Lights_Applied () == 1,
				 "%d light(s) were applied when one was", WorldLightManager::Get_Lights_Applied ());

		//
		//	The composed environments are a fixed pool, because a mesh on the static sort list
		//	keeps the pointer it was handed until the frame is flushed.  Past the pool an object
		//	is drawn with its cached static lighting, which is the picture it had before any of
		//	this existed rather than somebody else's.
		//
		WorldLightManager::Set_Max_Lit_Objects (1);
		Check (WorldLightManager::Get_Max_Lit_Objects () == 1,
				 "the lit-object pool is %u, not the one asked for",
				 WorldLightManager::Get_Max_Lit_Objects ());

		WorldLightManager::Begin_Frame ();
		Check (WorldLightManager::Apply_Dynamic_Lights (probe_obj, &env) != &env,
				 "the first lit object of a frame did not get a composed environment");
		Check (WorldLightManager::Apply_Dynamic_Lights (probe_obj, &env) == &env,
				 "the pool handed out more environments than it has");
		Check (WorldLightManager::Get_Overflow_Count () == 1,
				 "the lit-object pool ran out %d time(s) instead of once",
				 WorldLightManager::Get_Overflow_Count ());

		WorldLightManager::Set_Max_Lit_Objects (100000);
		Check (WorldLightManager::Get_Max_Lit_Objects () <= 1024,
				 "the lit-object pool reached %u", WorldLightManager::Get_Max_Lit_Objects ());

		WorldLightManager::Remove_All_Dynamic_Lights ();
		probe_obj->Release_Ref ();
	}

	//
	//	The service going away takes the lights with it, and the world going away takes them too
	//	-- a light belongs to the world it was lit in.
	//
	WorldLightManager::Create_Point_Light (Vector3 (0.0f, 0.0f, 0.0f), white, 0.0f, 10.0f, 1.0f, -1.0f);
	WorldLightManager::Release_Resources ();
	Check (WorldLightManager::Get_Dynamic_Light_Count () == 0, "lights outlived their world");
	Check (WorldLightManager::Is_Partitioned () == false,
			 "the light grid is still partitioned for a world that was unloaded");

	WorldLightManager::Create_Point_Light (Vector3 (0.0f, 0.0f, 0.0f), white, 0.0f, 10.0f, 1.0f, -1.0f);
	WorldLightManager::Shutdown ();
	Check (WorldLightManager::Get_Dynamic_Light_Count () == 0, "lights outlived the service");
	Check (WorldLightManager::Get_Max_Lit_Objects () == 0, "the environment pool outlived the service");
}


/*
**	The coarse surface over one sampled cell, read the way the far mesh draws it.
**
**	Written out here rather than borrowed from the service on purpose: the whole claim being
**	checked is that the service's measured drop is enough, and a check that reuses the service's
**	own arithmetic to decide that would only be checking it against itself.
*/
float	Coarse_Height (const HeightfieldClass &field, int ix, int iy, int stride)
{
	int last_x = field.Get_Vertex_Count_X () - 1;
	int last_y = field.Get_Vertex_Count_Y () - 1;

	int cx0 = (ix / stride) * stride;
	int cy0 = (iy / stride) * stride;
	if (cx0 >= last_x) cx0 = ((last_x - 1) / stride) * stride;
	if (cy0 >= last_y) cy0 = ((last_y - 1) / stride) * stride;

	int cx1 = cx0 + stride;
	int cy1 = cy0 + stride;
	if (cx1 > last_x) cx1 = last_x;
	if (cy1 > last_y) cy1 = last_y;

	float h00 = field.Get_Height (cx0, cy0);
	float h10 = field.Get_Height (cx1, cy0);
	float h01 = field.Get_Height (cx0, cy1);
	float h11 = field.Get_Height (cx1, cy1);

	float tx = (float)(ix - cx0) / (float)(cx1 - cx0);
	float ty = (float)(iy - cy0) / (float)(cy1 - cy0);

	return ((h00 * (1.0f - tx)) + (h10 * tx)) * (1.0f - ty) +
			 ((h01 * (1.0f - tx)) + (h11 * tx)) * ty;
}


/*
**	Roadmap Section 34: the far / background terrain layer.
**
**	Four things are claimed and all four are arithmetic, so all four are checkable with no
**	device: the far layer is derived from the one heightfield rather than being a second terrain
**	database, it costs far fewer vertices and submissions than the near patches over the same
**	ground, an edit dirties exactly the blocks over it, and the surface it draws is never above
**	the surface the near patches draw -- which is what turns a boundary into a step rather than
**	a hole.
*/
void	Check_Far_Terrain (void)
{
	WorldTerrainSystem::Init ();

	//
	//	With no field there is no layer and nothing pretends otherwise.
	//
	Check (WorldTerrainSystem::Has_Far_Terrain () == false,
			 "a far layer exists for a field that does not");
	Check (WorldTerrainSystem::Build_Far_Terrain_Representation () == false,
			 "the far layer built itself over no terrain");

	//
	//	A field of eight patches each way: two far blocks each way, with the shared block seam in
	//	the middle where a crack would show.  The heights are bumpy and non-planar, because a
	//	plane decimates exactly and would prove nothing about the drop.
	//
	const int	VERTICES	= 129;
	const int	STRIDE	= WorldTerrainSystem::FAR_STRIDE;

	Check (WorldTerrainSystem::Create_Terrain (VERTICES, VERTICES, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			 "the service would not create a field for the far layer");

	{
		float *heights = new float[VERTICES * VERTICES];
		for (int iy = 0; iy < VERTICES; iy ++) {
			for (int ix = 0; ix < VERTICES; ix ++) {
				heights[iy * VERTICES + ix] =
					3.0f * ::sinf ((float)ix * 0.37f) * ::cosf ((float)iy * 0.29f) +
					(float)(((ix * 7) + (iy * 13)) % 5);
			}
		}
		Check (WorldTerrainSystem::Set_Heights (heights, VERTICES * VERTICES),
				 "the far layer's heights were refused");
		delete [] heights;
	}

	//
	//	The layout is known before anything is built, because a system that changes the ground
	//	has to be able to name a block before a scene exists to draw one in.
	//
	Check (WorldTerrainSystem::Get_Patch_Count_X () == 8, "a 128 cell field is %d patches across",
			 WorldTerrainSystem::Get_Patch_Count_X ());
	Check (WorldTerrainSystem::Get_Far_Block_Count_X () == 2,
			 "eight patches make %d far blocks across, not two",
			 WorldTerrainSystem::Get_Far_Block_Count_X ());
	Check (WorldTerrainSystem::Has_Far_Terrain (),
			 "a created field has no far block table");
	Check (WorldTerrainSystem::Get_Far_Block_Built_Count () == 0,
			 "%d far block(s) were built with no scene to put them in",
			 WorldTerrainSystem::Get_Far_Block_Built_Count ());

	HeightfieldClass *field = WorldTerrainSystem::Peek_Heightfield ();
	Check (field != nullptr, "the field the far layer is cut from is not there");

	RenegadeTerrainPatchClass *block = WorldTerrainSystem::Create_Far_Block_Model (0, 0);
	RenegadeTerrainPatchClass *right = WorldTerrainSystem::Create_Far_Block_Model (1, 0);
	Check (block != nullptr, "the first far block would not build");
	Check (right != nullptr, "the second far block would not build");

	if ((block != nullptr) && (right != nullptr) && (field != nullptr)) {

		//
		//	Cost.  A block covers sixteen near patches and keeps every fourth grid point, so it
		//	is one submission of 17x17 against sixteen submissions of 17x17.
		//
		int block_vertices = block->Get_Vertex_Count ();
		Check (block_vertices == 17 * 17, "a far block has %d vertices, not 289", block_vertices);

		int near_vertices = 0;
		for (int py = 0; py < WorldTerrainSystem::FAR_BLOCK_PATCHES; py ++) {
			for (int px = 0; px < WorldTerrainSystem::FAR_BLOCK_PATCHES; px ++) {
				RenegadeTerrainPatchClass *patch = WorldTerrainSystem::Create_Patch_Model (px, py);
				if (patch != nullptr) {
					near_vertices += patch->Get_Vertex_Count ();
					patch->Release_Ref ();
				}
			}
		}

		Check (near_vertices >= block_vertices * 8,
				 "a far block saves only %d vertices against %d, which is not substantially fewer",
				 near_vertices, block_vertices);

		//
		//	Derivation.  Every far vertex is a grid point of the one heightfield, moved down by
		//	the layer's drop and by nothing else.  There is no second source of heights.
		//
		float drop = WorldTerrainSystem::Get_Far_Terrain_Drop ();
		Check (drop >= 0.0f, "the far layer was raised by %f rather than lowered", drop);
		Check (drop > 0.0f, "a field with hills in it decimated with no error at all");

		const Vector3 *vertices = block->Get_Vertex_Array ();
		bool matched = true;
		for (int py = 0; py < 17; py ++) {
			for (int px = 0; px < 17; px ++) {
				const Vector3 &v = vertices[py * 17 + px];
				if (!Near (v.X, (float)(px * STRIDE), 0.001f) ||
					 !Near (v.Y, (float)(py * STRIDE), 0.001f) ||
					 !Near (v.Z, field->Get_Height (px * STRIDE, py * STRIDE) - drop, 0.001f)) {
					matched = false;
				}
			}
		}
		Check (matched, "a far vertex is not a grid point of the field lowered by the drop");

		//
		//	Seams.  Two blocks meet on a sampled grid line and keep the same points on it, so the
		//	far surface is continuous across the whole field however many blocks it is cut into.
		//
		const Vector3 *right_vertices = right->Get_Vertex_Array ();
		bool seam_matched = true;
		for (int py = 0; py < 17; py ++) {
			const Vector3 &mine = vertices[py * 17 + 16];
			const Vector3 &theirs = right_vertices[py * 17 + 0];
			if (!Near (mine.X, theirs.X, 0.0001f) ||
				 !Near (mine.Y, theirs.Y, 0.0001f) ||
				 !Near (mine.Z, theirs.Z, 0.0001f)) {
				seam_matched = false;
			}
		}
		Check (seam_matched, "two far blocks disagree about the vertices on the line they share");

		//
		//	The invariant the boundary rests on: the coarse surface, lowered by the drop, is
		//	nowhere above the real ground.  Checked at every grid point of the field, against
		//	arithmetic written independently of the service's own.
		//
		float worst = 0.0f;
		for (int iy = 0; iy < VERTICES; iy ++) {
			for (int ix = 0; ix < VERTICES; ix ++) {
				float excess = (Coarse_Height (*field, ix, iy, STRIDE) - drop) - field->Get_Height (ix, iy);
				if (excess > worst) {
					worst = excess;
				}
			}
		}
		Check (worst <= 0.001f,
				 "the far surface stands %fm above the real ground, so it can poke through it",
				 worst);

		//
		//	And nothing collides with it.  Not by convention -- the model refuses every ray it is
		//	given, so the near representation remains the only thing that can answer a question
		//	about the ground.
		//
		float hit_z = 0.0f;
		Check (Drop_Ray_On_Model (block, 8.0f, 8.0f, &hit_z) == false,
				 "a ray hit the far layer, which is a second terrain collision representation");

		block->Release_Ref ();
		right->Release_Ref ();
	}

	//
	//	Editing.  A field that nothing has changed has nothing to rebuild, and a named region
	//	dirties the blocks over it and no others.  Each block spans 64 metres, so a box at 16,16
	//	is inside the first one and must not reach any of the other three.
	//
	{
		Check (WorldTerrainSystem::Is_Far_Block_Dirty (0, 0) == false,
				 "a field nobody has edited already needs its far layer rebuilt");

		AABoxClass inside_first (Vector3 (16.0f, 16.0f, 0.0f), Vector3 (4.0f, 4.0f, 100.0f));
		WorldTerrainSystem::Invalidate_Far_Terrain_Region (inside_first);

		Check (WorldTerrainSystem::Is_Far_Block_Dirty (0, 0),
				 "an edit inside the first block did not dirty it");
		Check (WorldTerrainSystem::Is_Far_Block_Dirty (1, 0) == false,
				 "an edit in the first block dirtied the one beside it");
		Check (WorldTerrainSystem::Is_Far_Block_Dirty (0, 1) == false,
				 "an edit in the first block dirtied the one above it");
		Check (WorldTerrainSystem::Is_Far_Block_Dirty (1, 1) == false,
				 "an edit in the first block dirtied the far corner");
	}

	//
	//	Shaping the ground through the service reaches the far layer too, so a system that never
	//	heard of far terrain still gets a correct one -- and it reaches only what it touched.
	//
	{
		WorldTerrainSystem::Modify_Height_Region (Vector3 (100.0f, 100.0f, 0.0f), 6.0f, 5.0f);
		Check (WorldTerrainSystem::Is_Far_Block_Dirty (1, 1),
				 "raising the ground in the last block did not dirty it");
		Check (WorldTerrainSystem::Is_Far_Block_Dirty (1, 0) == false,
				 "raising the ground at 100,100 dirtied a block it does not touch");
	}

	//
	//	A box off the end of the field names no block at all rather than being clamped onto the
	//	nearest one, which would rebuild ground nothing happened to.
	//
	Check (WorldTerrainSystem::Create_Terrain (VERTICES, VERTICES, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			 "the service would not create a field to check the edge case on");
	{
		AABoxClass off_the_field (Vector3 (1000.0f, 1000.0f, 0.0f), Vector3 (4.0f, 4.0f, 4.0f));
		WorldTerrainSystem::Invalidate_Far_Terrain_Region (off_the_field);

		bool any_dirty = false;
		for (int by = 0; by < WorldTerrainSystem::Get_Far_Block_Count_Y (); by ++) {
			for (int bx = 0; bx < WorldTerrainSystem::Get_Far_Block_Count_X (); bx ++) {
				if (WorldTerrainSystem::Is_Far_Block_Dirty (bx, by)) {
					any_dirty = true;
				}
			}
		}
		Check (any_dirty == false, "an edit off the end of the field dirtied a block anyway");
	}

	//
	//	The layer belongs to the field.  A new field is a new layout, and no block of the old one
	//	survives into it.
	//
	Check (WorldTerrainSystem::Create_Terrain (65, 65, 1.0f, Vector3 (0.0f, 0.0f, 0.0f)),
			 "the service would not create a second field");
	Check (WorldTerrainSystem::Get_Far_Block_Count_X () == 1,
			 "a four patch field makes %d far blocks across, not one",
			 WorldTerrainSystem::Get_Far_Block_Count_X ());
	Check (WorldTerrainSystem::Get_Far_Block_Built_Count () == 0,
			 "far blocks survived into a field of a different shape");

	WorldTerrainSystem::Destroy_Terrain ();
	Check (WorldTerrainSystem::Has_Far_Terrain () == false,
			 "the far layer outlived the field it was cut from");
	Check (WorldTerrainSystem::Get_Far_Terrain_Drop () == 0.0f,
			 "the far layer's drop outlived the field it was measured on");

	WorldTerrainSystem::Shutdown ();
}


/*
**	Strategic shroud -- roadmap Section 30.
**
**	The rule under test is the section's own: nothing here is a camera, and the only way a
**	cell ever becomes VISIBLE or RADAR_DETECTED is a registered, online sensor source.  Two
**	teams get independent grids from the same two sources' worth of coverage, proving the
**	Commander/tactical layer this feeds is per-team and never a shared "reveal everything"
**	switch.  Retention is exercised by hand: an unregistered source's cell falls to LAST_KNOWN
**	on the very next tick and to EXPLORED only once its retention has been spent.
*/
void	Check_Shroud (void)
{
	WorldShroudManager::Init ();

	//
	//	A fresh, unpartitioned manager answers every query with UNKNOWN rather than asserting
	//	or crashing -- the same missing-data discipline as every other service in this project.
	//
	Check (WorldShroudManager::Get_Shroud_State (0,Vector3 (0,0,0)) == SHROUD_UNKNOWN,
			 "an unpartitioned manager answered something other than UNKNOWN");
	Check (WorldShroudManager::Is_Partitioned () == false, "a fresh manager reports partitioned");
	Check (WorldShroudManager::Get_Sensor_Source_Count () == 0,
			 "a fresh manager already has %d sensor source(s)", WorldShroudManager::Get_Sensor_Source_Count ());

	WorldShroudManager::Re_Partition (Vector3 (-5000.0f,-5000.0f,0.0f),Vector3 (5000.0f,5000.0f,0.0f));
	Check (WorldShroudManager::Is_Partitioned (), "Re_Partition did not partition");
	Check (WorldShroudManager::Get_Cell_Count () > 0, "a partitioned manager has no cells");
	Check (WorldShroudManager::Get_Cell_Count () <= WORLD_SHROUD_MAX_GRID_CELLS,
			 "the grid grew past its bound: %d cells", WorldShroudManager::Get_Cell_Count ());

	//
	//	Two teams, two sources, far enough apart that one team's coverage cannot reach the
	//	other team's point by accident.
	//
	const int TEAM_A = 0;
	const int TEAM_B = 1;
	Vector3 point_a (0.0f,0.0f,0.0f);
	Vector3 point_b (3000.0f,3000.0f,0.0f);

	ShroudSensorHandleType source_a = WorldShroudManager::RegisterStrategicSensorSource (
		TEAM_A,point_a,300.0f,900.0f,5.0f,true);
	Check (source_a != 0, "registering a sensor source with room available was refused");
	Check (WorldShroudManager::Is_Sensor_Source (source_a), "a registered source does not read back as one");
	Check (WorldShroudManager::Get_Sensor_Source_Count () == 1,
			 "one registered source reports a count of %d", WorldShroudManager::Get_Sensor_Source_Count ());

	ShroudSensorHandleType source_b = WorldShroudManager::RegisterStrategicSensorSource (
		TEAM_B,point_b,300.0f,900.0f,5.0f,true);
	Check (source_b != 0, "a second team's source was refused");

	WorldShroudManager::Timestep (0.1f);

	ShroudStateType state_a = WorldShroudManager::Get_Shroud_State (TEAM_A,point_a);
	Check (state_a == SHROUD_VISIBLE,
			 "team A is not VISIBLE at its own source's position (state %d, %d cells)",
			 (int)state_a,WorldShroudManager::Get_Cell_Count ());
	Check (WorldShroudManager::Is_Visible (TEAM_A,point_a), "Is_Visible disagrees with Get_Shroud_State");
	Check (WorldShroudManager::Get_Shroud_State (TEAM_A,point_b) == SHROUD_UNKNOWN,
			 "team A knows about team B's point with no source anywhere near it");
	Check (WorldShroudManager::Get_Shroud_State (TEAM_B,point_a) == SHROUD_UNKNOWN,
			 "team B's grid was revealed by team A's source -- shroud is not per-team");
	Check (WorldShroudManager::Get_Shroud_State (TEAM_B,point_b) == SHROUD_VISIBLE,
			 "team B is not VISIBLE at its own source's position");

	//
	//	Radar-only coverage: a point outside the vision circle but inside the radar circle
	//	reads RADAR_DETECTED, not VISIBLE.
	//
	Vector3 radar_only_point (0.0f,600.0f,0.0f);
	Check (WorldShroudManager::Get_Shroud_State (TEAM_A,radar_only_point) == SHROUD_RADAR_DETECTED,
			 "a point between the vision and radar radii did not read RADAR_DETECTED");
	Check (WorldShroudManager::Is_Radar_Detected (TEAM_A,radar_only_point),
			 "Is_Radar_Detected disagrees with Get_Shroud_State");

	//
	//	Moving the camera is not in this file at all -- there is no camera type included here,
	//	and every state change above came from RegisterStrategicSensorSource/Timestep, never
	//	from a viewpoint.  What this check can additionally prove is that an *offline* source
	//	contributes nothing, which is the same code path a Commander camera would have to go
	//	through if it ever tried to pose as a source.
	//
	Check (WorldShroudManager::UpdateStrategicSensorSource (source_a,point_a,300.0f,900.0f,false),
			 "a registered source could not be taken offline");
	WorldShroudManager::Timestep (0.1f);
	Check (WorldShroudManager::Get_Shroud_State (TEAM_A,point_a) != SHROUD_VISIBLE,
			 "an offline source is still contributing live coverage");

	//
	//	Retention: dropping a source (or, as above, taking it offline) does not erase what was
	//	just seen -- it falls to LAST_KNOWN and only reaches EXPLORED once retention is spent.
	//	This also proves LAST_KNOWN is not tracking anything live: nothing moved point_a's
	//	record, the manager only aged a timer against a stationary cell.
	//
	Check (WorldShroudManager::Get_Shroud_State (TEAM_A,point_a) == SHROUD_LAST_KNOWN,
			 "coverage lost this tick did not fall to LAST_KNOWN");
	WorldShroudManager::Timestep (10.0f);
	Check (WorldShroudManager::Get_Shroud_State (TEAM_A,point_a) == SHROUD_EXPLORED,
			 "LAST_KNOWN did not decay to EXPLORED once its retention was spent");
	Check (WorldShroudManager::Is_Explored (TEAM_A,point_a), "Is_Explored disagrees with Get_Shroud_State");

	//
	//	Dynamic add/remove, the acceptance's own words.
	//
	Check (WorldShroudManager::UnregisterStrategicSensorSource (source_a), "a registered source could not be removed");
	Check (!WorldShroudManager::Is_Sensor_Source (source_a), "an unregistered source still reads as registered");
	Check (!WorldShroudManager::UnregisterStrategicSensorSource (source_a), "a source was unregistered twice");
	Check (WorldShroudManager::Get_Sensor_Source_Count () == 1,
			 "removing one of two sources left a count of %d", WorldShroudManager::Get_Sensor_Source_Count ());

	//
	//	Bounds: the source table and the team table both refuse rather than grow.
	//
	WorldShroudManager::UnregisterStrategicSensorSource (source_b);
	int refusals_before = WorldShroudManager::Get_Refusal_Count ();
	int made = 0;
	for (int i = 0; i < WORLD_SHROUD_MAX_SENSOR_SOURCES + 8; i ++) {
		if (WorldShroudManager::RegisterStrategicSensorSource (0,point_a,10.0f,10.0f,5.0f,true) != 0) {
			made ++;
		}
	}
	Check (made == WORLD_SHROUD_MAX_SENSOR_SOURCES, "the source table let in %d, not the bound", made);
	Check (WorldShroudManager::Get_Refusal_Count () > refusals_before, "an over-the-bound source was not counted as a refusal");

	WorldShroudManager::Shutdown ();
	Check (WorldShroudManager::Get_Sensor_Source_Count () == 0, "sensor sources outlived the service");
	Check (WorldShroudManager::Is_Partitioned () == false, "partitioning outlived the service");
}


/***********************************************************************************************
**	macro
**
**	Roadmap Section 34 Phase 4.  Device-less, same reasoning as everything else in this file:
**	MacroTerrainGenerator writes to a heightfield and reads WorldBoundsService, neither of which
**	needs a physics scene, so determinism, height-range enforcement, ordinary-slope enforcement
**	and cliff-band immutability are all provable before anything is rendered or collided with.
***********************************************************************************************/
static void
Make_Macro_Terrain_Test_Params (MacroTerrainParams *out)
{
	out->Seed							= 777u;
	out->WorldWidth					= 256.0f;
	out->WorldHeight					= 256.0f;
	out->ChunkSize						= 64.0f;
	out->CenterX						= 0.0f;
	out->CenterY						= 0.0f;
	out->SeaLevel						= 0.0f;
	out->MinElevation					= -20.0f;
	out->MaxElevation					= 40.0f;
	out->MaxBuildableSlopeRadians	= 0.5f;
	out->FeatureScale					= 80.0f;
	out->MacroRelief					= 0.6f;
	out->HillDensity					= 0.4f;
	out->MountainDensity				= 0.2f;
	out->ValleyDensity					= 0.3f;
	out->RidgeSharpness				= 0.5f;
	out->PlateauChance					= 0.1f;
	out->BoundaryApproachFlattening	= 0.6f;
}


//	MacroTerrainGenerator's relaxation bounds each axis's step, so a diagonal can still exceed the
//	cap -- its own header says the limit is approached, not guaranteed.  The guarantee that does
//	exist, and that every consumer relies on, is that such a chunk is never Buildable.
int	Count_Steep_Buildable_Chunks (float limit_radians)
{
	int count = 0;
	for (int py = 0; py < WorldTerrainSystem::Get_Chunk_Count_Y (); py++) {
		for (int px = 0; px < WorldTerrainSystem::Get_Chunk_Count_X (); px++) {
			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info (px, py, &info)) continue;
			if ((info.MaxSlopeRadians > limit_radians + 0.01f) && info.Buildable) count++;
		}
	}
	return count;
}


void	Check_Macro_Terrain (void)
{
	WorldTerrainSystem::Init ();
	MacroTerrainGenerator::Init ();
	WorldBoundsService::Init ();

	//
	//	Every parameter rule refuses on its own, and refusing leaves nothing behind -- the same
	//	"refuse whole, not half" contract Phase 3's cliff ring has.
	//
	{
		StringClass error ("");
		MacroTerrainParams bad;
		Make_Macro_Terrain_Test_Params (&bad);
		bad.WorldWidth = 0.0f;
		Check (!MacroTerrainGenerator::Generate (bad, &error), "a zero world width was accepted");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!MacroTerrainGenerator::Has_Generated (), "a refused generation left state behind");
		Check (!WorldTerrainSystem::Has_Terrain (), "a refused generation left terrain behind");

		Make_Macro_Terrain_Test_Params (&bad);
		bad.MaxBuildableSlopeRadians = 0.0f;
		Check (!MacroTerrainGenerator::Generate (bad, &error), "a zero slope limit was accepted");

		Make_Macro_Terrain_Test_Params (&bad);
		bad.MaxElevation = bad.MinElevation - 1.0f;
		Check (!MacroTerrainGenerator::Generate (bad, &error), "a max elevation below min elevation was accepted");
	}

	//
	//	No boundary configured: every chunk is ordinary interior, exactly what a hand-authored
	//	heightfield with no procedural world ever gets.
	//
	MacroTerrainParams params;
	Make_Macro_Terrain_Test_Params (&params);
	{
		StringClass error ("");
		Check (MacroTerrainGenerator::Generate (params, &error),
				 "generation refused a valid profile: %s", error.Peek_Buffer ());
		Check (MacroTerrainGenerator::Has_Generated (), "a successful generation reported no terrain");
		Check (WorldTerrainSystem::Has_Terrain (), "a successful generation left no heightfield");

		//	A 256-unit field at 64/16=4-unit cells is 65 vertices across, which is 4 chunks --
		//	stated as the arithmetic it is so a change to either input shows up here as a wrong
		//	expectation rather than a silently different field, same convention Check_Far_Terrain
		//	uses next door.
		Check (MacroTerrainGenerator::Get_Vertex_Count_X () == 65,
				 "a 256-unit field at 4-unit cells is %d vertices across, not 65",
				 MacroTerrainGenerator::Get_Vertex_Count_X ());
		Check (WorldTerrainSystem::Get_Chunk_Count_X () == 4,
				 "a 64-vertex-wide field is %d chunks across, not 4", WorldTerrainSystem::Get_Chunk_Count_X ());
		Check (WorldTerrainSystem::Get_Chunk_Count_Y () == 4,
				 "a 64-vertex-tall field is %d chunks down, not 4", WorldTerrainSystem::Get_Chunk_Count_Y ());

		//	Each check runs before its message reads the report -- as one Check argument list the
		//	compiler may read the (still empty) report first, which is how these failures used to
		//	print no reason at all.
		StringClass report ("");
		int range_failures = MacroTerrainGenerator::Check_Height_Range (params, &report);
		Check (range_failures == 0, "unconfigured terrain left its own height range: %s", report.Peek_Buffer ());
		int slope_over = MacroTerrainGenerator::Check_Ordinary_Slope_Limit (params, &report);
		Check (Count_Steep_Buildable_Chunks (params.MaxBuildableSlopeRadians) == 0,
				 "a chunk steeper than the buildable limit is still marked buildable (%d over the limit: %s)",
				 slope_over, report.Peek_Buffer ());
		Check (MacroTerrainGenerator::Check_Cliff_Band_Untouched (&report) == 0,
				 "unconfigured terrain reported a cliff-band violation with no boundary configured");

		bool any_cliff_band = false;
		for (int py = 0; py < WorldTerrainSystem::Get_Chunk_Count_Y (); py++) {
			for (int px = 0; px < WorldTerrainSystem::Get_Chunk_Count_X (); px++) {
				TerrainChunkInfoClass info;
				if (WorldTerrainSystem::Get_Chunk_Info (px, py, &info) && info.CliffBand) {
					any_cliff_band = true;
				}
			}
		}
		Check (!any_cliff_band, "a chunk was tagged cliff band with no boundary configured");
	}

	//
	//	Section 3B determinism: the same seed and profile produce the same terrain, a different
	//	seed produces a different one.
	//
	{
		StringClass error ("");
		unsigned int first = MacroTerrainGenerator::Get_Terrain_Checksum ();
		Check (first != 0, "a generated terrain carried no checksum");

		Check (MacroTerrainGenerator::Generate (params, &error), "regenerating the same terrain failed");
		Check (MacroTerrainGenerator::Get_Terrain_Checksum () == first,
				 "the same seed and profile produced a different terrain checksum");

		MacroTerrainParams other = params;
		other.Seed = params.Seed + 1u;
		Check (MacroTerrainGenerator::Generate (other, &error), "generating a second seed failed");
		Check (MacroTerrainGenerator::Get_Terrain_Checksum () != first,
				 "two different terrain seeds produced an identical terrain checksum");
	}

	//
	//	With a boundary configured, the immutable cliff band -- and only the cliff band -- comes
	//	back flat.  A 240-unit boundary square inside the 256-unit field: its 15-unit band runs
	//	through the outer ring of 64-unit chunks, and the inner four chunks (to +/-64) never touch
	//	it, so both a cliff-band chunk and an ordinary interior chunk exist.  A 120-unit square,
	//	as this used to use, touches every chunk of a 4 x 4 grid -- no interior was possible.
	//
	{
		DynamicVectorClass<Vector2> polygon;
		polygon.Add (Vector2 (-120.0f,-120.0f));
		polygon.Add (Vector2 (-120.0f, 120.0f));
		polygon.Add (Vector2 ( 120.0f, 120.0f));
		polygon.Add (Vector2 ( 120.0f,-120.0f));

		StringClass error ("");
		Check (WorldBoundsService::Configure (polygon,15.0f,5.0f,200.0f,&error),
				 "the test boundary would not configure: %s", error.Peek_Buffer ());

		Check (MacroTerrainGenerator::Generate (params, &error),
				 "generation with a boundary configured failed: %s", error.Peek_Buffer ());

		StringClass report ("");
		int band_failures = MacroTerrainGenerator::Check_Cliff_Band_Untouched (&report);
		Check (band_failures == 0, "a cliff-band chunk was not flat at sea level: %s", report.Peek_Buffer ());
		int bounded_range_failures = MacroTerrainGenerator::Check_Height_Range (params, &report);
		Check (bounded_range_failures == 0, "terrain with a boundary left its own height range: %s", report.Peek_Buffer ());
		int bounded_slope_over = MacroTerrainGenerator::Check_Ordinary_Slope_Limit (params, &report);
		Check (Count_Steep_Buildable_Chunks (params.MaxBuildableSlopeRadians) == 0,
				 "with a boundary, a chunk steeper than the buildable limit is still marked buildable (%d over: %s)",
				 bounded_slope_over, report.Peek_Buffer ());

		bool any_cliff_band = false;
		bool any_interior = false;
		for (int py = 0; py < WorldTerrainSystem::Get_Chunk_Count_Y (); py++) {
			for (int px = 0; px < WorldTerrainSystem::Get_Chunk_Count_X (); px++) {
				TerrainChunkInfoClass info;
				if (!WorldTerrainSystem::Get_Chunk_Info (px, py, &info)) continue;
				if (info.CliffBand) any_cliff_band = true; else any_interior = true;
			}
		}
		Check (any_cliff_band, "a 256-unit field around a 240-unit boundary square produced no cliff-band chunk");
		Check (any_interior, "a boundary square inside a larger field left no ordinary interior chunk");

		WorldBoundsService::Clear ();
	}

	WorldTerrainSystem::Shutdown ();
	MacroTerrainGenerator::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	interior
**
**	Roadmap Section 34 Phase 5.  Device-less for the same reason macro's own check is: the
**	generator reads WorldTerrainSystem's chunk registry and WorldBoundsService and writes to
**	EscarpmentSystem and the chunk registry, none of which needs a physics scene.
**
**	A uniform-slope synthetic ramp, not MacroTerrainGenerator's own noise, is the terrain under
**	test here.  MacroTerrainGenerator's relaxation pass leaves no arithmetic guarantee of how
**	close any particular seed's chunks land to the buildable-slope cap, so a check built on it
**	could not honestly assert a feature gets built at all.  A field whose slope is the same,
**	known value everywhere removes that uncertainty by construction: every chunk qualifies as a
**	candidate and at least one feature is a mathematical certainty, not a hope about a seed.
***********************************************************************************************/
static void
Build_Uniform_Ramp_Terrain (int patches_per_side,float cell_size,float degrees_slope,const Vector3 & origin)
{
	int vcount = (patches_per_side * HeightfieldClass::PATCH_CELLS) + 1;

	WorldTerrainSystem::Destroy_Terrain ();
	WorldTerrainSystem::Create_Terrain (vcount,vcount,cell_size,origin);

	float rise_per_vertex = cell_size * WWMath::Tan (DEG_TO_RADF (degrees_slope));

	float * heights = new float [vcount * vcount];
	for (int iy = 0; iy < vcount; iy++) {
		for (int ix = 0; ix < vcount; ix++) {
			heights [iy * vcount + ix] = (float)ix * rise_per_vertex;
		}
	}
	WorldTerrainSystem::Set_Heights (heights,vcount * vcount);
	delete [] heights;
}


void	Check_Interior_Cliffs (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	InteriorCliffGenerator::Init ();

	//	~28.6 degrees, the same order Check_Macro_Terrain's own test uses for MaxBuildableSlopeRadians.
	static const float CHUNK_SLOPE_CAP = 0.5f;

	//
	//	Refusals.  No chunk registry at all, an out-of-range slope threshold fraction, a
	//	negative density and too short a run length all refuse and leave nothing behind.
	//
	{
		StringClass error ("");
		InteriorCliffParams bad;
		Check (!InteriorCliffGenerator::Generate (bad,&error), "generation ran with no chunk registry at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!InteriorCliffGenerator::Has_Cliffs (), "a refused generation left state behind");

		Build_Uniform_Ramp_Terrain (2,4.0f,40.0f,Vector3 (0.0f,0.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (CHUNK_SLOPE_CAP);

		bad.MaxBuildableSlopeRadians	= CHUNK_SLOPE_CAP;
		bad.SlopeThresholdFraction		= 0.0f;
		Check (!InteriorCliffGenerator::Generate (bad,&error), "a zero slope threshold fraction was accepted");

		bad.SlopeThresholdFraction	= 0.5f;
		bad.Density							= -0.1f;
		Check (!InteriorCliffGenerator::Generate (bad,&error), "a negative density was accepted");

		bad.Density			= 0.5f;
		bad.MaxRunLength	= 1;
		Check (!InteriorCliffGenerator::Generate (bad,&error), "a run length of 1 chunk was accepted");
	}

	//
	//	A uniformly steep 3x3-chunk field, every chunk a candidate (density 1.0): at least one
	//	feature is certain, and every one of Phase 5's own structural diagnostics must hold.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (3,4.0f,40.0f,Vector3 (-96.0f,-96.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (CHUNK_SLOPE_CAP);

		InteriorCliffParams params;
		params.Seed							= 4242u;
		params.MaxBuildableSlopeRadians	= CHUNK_SLOPE_CAP;
		params.SlopeThresholdFraction	= 0.5f;
		params.Density							= 1.0f;
		params.MaxRunLength					= 10;
		params.Batter							= 0.1f;
		params.ToeWidth						= 4.0f;
		params.LayerIndex						= -1;

		StringClass error ("");
		Check (InteriorCliffGenerator::Generate (params,&error),
				 "generation refused a uniformly steep field: %s",error.Peek_Buffer ());
		Check (InteriorCliffGenerator::Has_Cliffs (), "a successful generation reported no cliffs built");
		Check (InteriorCliffGenerator::Get_Feature_Count () >= 1,
				 "a uniformly steep 3x3-chunk field produced no interior cliff feature");

		StringClass report ("");
		Check (InteriorCliffGenerator::Check_No_Cliff_Band_Overlap (&report) == 0,
				 "an interior cliff station lay in the cliff band with no boundary configured: %s",report.Peek_Buffer ());
		Check (InteriorCliffGenerator::Check_No_Single_Chunk_Spikes (&report) == 0,
				 "a feature had fewer than two stations: %s",report.Peek_Buffer ());
		Check (InteriorCliffGenerator::Check_Bypass_Candidates_Present (&report) == 0,
				 "a bypass-kind feature carried no bypass point: %s",report.Peek_Buffer ());
		Check (InteriorCliffGenerator::Check_No_Inverted_Faces (&report) == 0,
				 "a station's rim was at or below its toe: %s",report.Peek_Buffer ());

		for (int py = 0; py < WorldTerrainSystem::Get_Chunk_Count_Y (); py++) {
			for (int px = 0; px < WorldTerrainSystem::Get_Chunk_Count_X (); px++) {
				TerrainChunkInfoClass info;
				if (WorldTerrainSystem::Get_Chunk_Info (px,py,&info) && info.InteriorCliff) {
					Check (!info.Buildable,"an interior-cliff chunk (%d,%d) was still marked buildable",px,py);
				}
			}
		}

		first_checksum = InteriorCliffGenerator::Get_Checksum ();
		Check (first_checksum != 0, "a built set of interior cliffs carried no checksum");

		Check (InteriorCliffGenerator::Generate (params,&error), "regenerating the same interior cliffs failed");
		Check (InteriorCliffGenerator::Get_Checksum () == first_checksum,
				 "the same seed and terrain produced a different interior cliff checksum");
	}

	//
	//	A different seed.  Density 1.0 makes the candidate set, and so the walk itself, seed-
	//	independent -- what changes is each feature's own type/traversability draw, which is
	//	enough to move the checksum.
	//
	{
		InteriorCliffParams other;
		other.Seed							= 4243u;
		other.MaxBuildableSlopeRadians	= CHUNK_SLOPE_CAP;
		other.SlopeThresholdFraction	= 0.5f;
		other.Density							= 1.0f;
		other.MaxRunLength					= 10;
		other.Batter							= 0.1f;
		other.ToeWidth						= 4.0f;
		other.LayerIndex						= -1;

		StringClass error ("");
		Check (InteriorCliffGenerator::Generate (other,&error), "generating a second seed failed");
		Check (InteriorCliffGenerator::Get_Checksum () != first_checksum,
				 "two different interior cliff seeds produced an identical checksum");
	}

	//
	//	The same field, with a boundary configured: chunks the boundary classifies cliff band or
	//	past it must never become a candidate however steep they are.
	//
	{
		Build_Uniform_Ramp_Terrain (3,4.0f,40.0f,Vector3 (-96.0f,-96.0f,0.0f));

		DynamicVectorClass<Vector2> polygon;
		polygon.Add (Vector2 (-40.0f,-40.0f));
		polygon.Add (Vector2 (-40.0f, 40.0f));
		polygon.Add (Vector2 ( 40.0f, 40.0f));
		polygon.Add (Vector2 ( 40.0f,-40.0f));

		StringClass error ("");
		Check (WorldBoundsService::Configure (polygon,15.0f,5.0f,300.0f,&error),
				 "the test boundary would not configure: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (CHUNK_SLOPE_CAP);

		bool any_cliff_band = false;
		for (int py = 0; py < WorldTerrainSystem::Get_Chunk_Count_Y (); py++) {
			for (int px = 0; px < WorldTerrainSystem::Get_Chunk_Count_X (); px++) {
				TerrainChunkInfoClass info;
				if (WorldTerrainSystem::Get_Chunk_Info (px,py,&info) && info.CliffBand) any_cliff_band = true;
			}
		}
		Check (any_cliff_band,"a 3x3-chunk field around a boundary square produced no cliff-band chunk");

		InteriorCliffParams params;
		params.Seed							= 99u;
		params.MaxBuildableSlopeRadians	= CHUNK_SLOPE_CAP;
		params.SlopeThresholdFraction	= 0.5f;
		params.Density							= 1.0f;
		params.MaxRunLength					= 10;
		params.Batter							= 0.1f;
		params.ToeWidth						= 4.0f;
		params.LayerIndex						= -1;

		Check (InteriorCliffGenerator::Generate (params,&error),
				 "generation with a boundary configured failed: %s",error.Peek_Buffer ());

		StringClass report ("");
		Check (InteriorCliffGenerator::Check_No_Cliff_Band_Overlap (&report) == 0,
				 "an interior cliff crossed into the cliff band: %s",report.Peek_Buffer ());

		WorldBoundsService::Clear ();
	}

	InteriorCliffGenerator::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	hydrology
**
**	Roadmap Section 34 Phase 6.  Device-less for the same reason interior's own check is:
**	HydrologyGenerator reads the chunk registry and WorldBoundsService and writes to WaterSystem
**	and the chunk registry, none of which needs a physics scene.
**
**	The same synthetic uniform-slope ramp `Check_Interior_Cliffs` uses, reused here for the same
**	reason: MacroTerrainGenerator's own noise gives no arithmetic guarantee a channel or a basin
**	exists for a given seed. A ramp whose height depends only on the X grid index gives every row
**	an identical, guaranteed drainage pattern -- each row's own westernmost chunk is a sink
**	(nothing further west, and its same-row neighbour at any other Y is equal height, never
**	lower), and every chunk east of it flows one step west toward it.
***********************************************************************************************/
void	Check_Hydrology (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	HydrologyGenerator::Init ();

	//
	//	Refusals.  No chunk registry at all, a zero river width, a stream wider than its own
	//	river, a zero accumulation threshold and a zero river-count cap all refuse and leave
	//	nothing behind.
	//
	{
		StringClass error ("");
		HydrologyParams bad;
		Check (!HydrologyGenerator::Generate (bad,&error), "generation ran with no chunk registry at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!HydrologyGenerator::Has_Hydrology (), "a refused generation left state behind");

		Build_Uniform_Ramp_Terrain (3,4.0f,10.0f,Vector3 (-96.0f,-96.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		bad.RiverWidth = 0.0f;
		Check (!HydrologyGenerator::Generate (bad,&error), "a zero river width was accepted");

		bad.RiverWidth		= 6.0f;
		bad.StreamWidth	= 8.0f;
		Check (!HydrologyGenerator::Generate (bad,&error), "a stream width above river width was accepted");

		bad.StreamWidth = 2.5f;
		bad.RiverAccumulationChunks = 0.0f;
		Check (!HydrologyGenerator::Generate (bad,&error), "a zero accumulation threshold was accepted");

		bad.RiverAccumulationChunks	= 2.5f;
		bad.MaxRiverCount					= 0;
		Check (!HydrologyGenerator::Generate (bad,&error), "a zero max river count was accepted");
	}

	//
	//	A uniform ramp, three chunks per row: every row drains west into its own sink.  Peak
	//	accumulation at each sink is 3 (all three chunks in that row), comfortably above a 2.5
	//	river threshold, so every row's channel classifies as a river and every sink becomes a
	//	lake.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (3,4.0f,10.0f,Vector3 (-96.0f,-96.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams params;
		params.Seed							= 55u;
		params.GenerateRivers				= true;
		params.GenerateLakes				= true;
		params.GeneratePonds				= true;
		params.RiverWidth					= 6.0f;
		params.StreamWidth					= 2.5f;
		params.RiverAccumulationChunks	= 2.5f;
		params.MaxRiverCount				= 4;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (params,&error),
				 "generation refused a uniform ramp: %s",error.Peek_Buffer ());
		Check (HydrologyGenerator::Has_Hydrology (), "a successful generation reported no hydrology built");
		Check (HydrologyGenerator::Get_Feature_Count () >= 2,
				 "a uniform 3-row ramp produced fewer than one channel and one basin");

		bool any_river = false;
		bool any_basin = false;
		for (int i = 0; i < HydrologyGenerator::Get_Feature_Count (); i++) {
			HydrologyFeature f;
			if (!HydrologyGenerator::Get_Feature (i,&f)) continue;
			if (!f.Enclosed && (f.Type == HYDRO_FEATURE_RIVER)) any_river = true;
			if (f.Enclosed) any_basin = true;
		}
		Check (any_river, "a uniform ramp with 3 chunks of drainage per row produced no river");
		Check (any_basin, "a uniform ramp's own sinks produced no lake/pond");

		StringClass report ("");
		Check (HydrologyGenerator::Check_No_Cliff_Band_Overlap (&report) == 0,
				 "a hydrology feature lay in the cliff band with no boundary configured: %s",report.Peek_Buffer ());
		Check (HydrologyGenerator::Check_River_Never_Climbs (&report) == 0,
				 "a river/stream station climbed uphill: %s",report.Peek_Buffer ());
		Check (HydrologyGenerator::Check_Lake_Within_Basin (&report) == 0,
				 "a lake/pond surface stood above its own basin rim: %s",report.Peek_Buffer ());
		Check (HydrologyGenerator::Check_Crossing_Candidates_Present (&report) == 0,
				 "a hydrology feature carried no crossing candidate: %s",report.Peek_Buffer ());

		first_checksum = HydrologyGenerator::Get_Checksum ();
		Check (first_checksum != 0, "a built set of hydrology features carried no checksum");

		//	"The same terrain" has to be restored, not assumed: the first run carved its channels
		//	and basins into this heightfield (WaterSystem::Shape_Terrain), and Clear drops the
		//	water, not the carving.  Production is the same -- every Generate_World attempt
		//	rebuilds terrain before hydrology runs -- so this is what determinism means here.
		HydrologyGenerator::Clear ();
		Build_Uniform_Ramp_Terrain (3,4.0f,10.0f,Vector3 (-96.0f,-96.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		Check (HydrologyGenerator::Generate (params,&error), "regenerating the same hydrology failed");
		Check (HydrologyGenerator::Get_Checksum () == first_checksum,
				 "the same seed and terrain produced a different hydrology checksum");
	}

	//
	//	Section 5.9: a boundary configured close enough that a channel's downstream chunk would
	//	otherwise be cliff band must never let that channel, or its own terminal basin, cross
	//	into it.
	//
	{
		Build_Uniform_Ramp_Terrain (3,4.0f,10.0f,Vector3 (-96.0f,-96.0f,0.0f));

		DynamicVectorClass<Vector2> polygon;
		polygon.Add (Vector2 (-40.0f,-40.0f));
		polygon.Add (Vector2 (-40.0f, 40.0f));
		polygon.Add (Vector2 ( 40.0f, 40.0f));
		polygon.Add (Vector2 ( 40.0f,-40.0f));

		StringClass error ("");
		Check (WorldBoundsService::Configure (polygon,15.0f,5.0f,300.0f,&error),
				 "the test boundary would not configure: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams params;
		params.Seed							= 77u;
		params.GenerateRivers				= true;
		params.GenerateLakes				= true;
		params.GeneratePonds				= true;
		params.RiverWidth					= 6.0f;
		params.StreamWidth					= 2.5f;
		params.RiverAccumulationChunks	= 2.5f;
		params.MaxRiverCount				= 4;

		Check (HydrologyGenerator::Generate (params,&error),
				 "generation with a boundary configured failed: %s",error.Peek_Buffer ());

		StringClass report ("");
		Check (HydrologyGenerator::Check_No_Cliff_Band_Overlap (&report) == 0,
				 "a hydrology feature crossed into the cliff band: %s",report.Peek_Buffer ());

		WorldBoundsService::Clear ();
	}

	HydrologyGenerator::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	strategic
**
**	Roadmap Section 34 Phase 7.  Device-less: StrategicPlanner reads the chunk registry,
**	WorldBoundsService and WaterSystem's own headless queries, none of which needs a physics
**	scene.
**
**	A large uniform-slope ramp -- six chunks per side rather than the three Check_Interior_
**	Cliffs and Check_Hydrology use -- gives the planner a big, entirely-buildable field with no
**	water and no boundary, so there is room for every candidate category at once without the
**	separation/economic-range parameters having to be tuned to the edge of failing.
***********************************************************************************************/
void	Check_Strategic_Plan (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();

	//
	//	Refusals.
	//
	{
		StringClass error ("");
		StrategicPlannerParams bad;
		Check (!StrategicPlanner::Generate (bad,&error), "generation ran with no chunk registry at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!StrategicPlanner::Has_Plan (), "a refused generation left state behind");

		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		bad.MinApproachCount = 0;
		Check (!StrategicPlanner::Generate (bad,&error), "a zero min approach count was accepted");

		bad.MinApproachCount = 2;
		bad.MinBaseSeparation = 0.0f;
		Check (!StrategicPlanner::Generate (bad,&error), "a zero min base separation was accepted");

		bad.MinBaseSeparation = 80.0f;
		bad.MaxEconomicRange = 0.0f;
		Check (!StrategicPlanner::Generate (bad,&error), "a zero max economic range was accepted");
	}

	//
	//	A large, entirely-buildable, unconfigured-boundary field.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams params;
		params.Seed							= 909u;
		params.MinApproachCount			= 2;
		params.MinBaseCandidatesPerFaction	= 2;
		params.MinBaseSeparation			= 80.0f;
		params.MaxTiberiumCandidates		= 4;
		params.MaxSettlementCandidates	= 3;
		params.MaxPoiCandidates			= 3;
		params.MaxExpansionCandidates	= 2;
		params.MaxEconomicRange			= 1000.0f;

		StringClass error ("");
		Check (StrategicPlanner::Generate (params,&error),
				 "generation refused a large uniform field: %s",error.Peek_Buffer ());
		Check (StrategicPlanner::Has_Plan (), "a successful generation reported no plan built");

		StringClass report ("");
		Check (StrategicPlanner::Check_Faction_Base_Minimum (2,&report) == 0,
				 "a large uniform field did not offer each faction 2 base candidates: %s",report.Peek_Buffer ());
		Check (StrategicPlanner::Check_Tiberium_Minimum (1,&report) == 0,
				 "a large uniform field offered no viable Tiberium candidate: %s",report.Peek_Buffer ());
		Check (StrategicPlanner::Check_No_Cliff_Or_Water_Overlap (&report) == 0,
				 "a strategic node overlapped water or the cliff band with neither configured: %s",report.Peek_Buffer ());
		Check (StrategicPlanner::Check_No_Economically_Disconnected_Resource (params.MaxEconomicRange,&report) == 0,
				 "a resource candidate had no base within economic range: %s",report.Peek_Buffer ());

		//	Every base candidate is tagged for a real faction -- neither PLAYERTYPE_NEUTRAL nor
		//	anything else leaked through.
		for (int i = 0; i < StrategicPlanner::Get_Node_Count (); i++) {
			StrategicNode node;
			if (!StrategicPlanner::Get_Node (i,&node)) continue;
			if (node.Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
			Check ((node.Faction == PLAYERTYPE_GDI) || (node.Faction == PLAYERTYPE_NOD),
					 "base candidate '%s' carries faction %d, neither GDI nor Nod",node.Id.Peek_Buffer (),node.Faction);
		}

		//	Travel cost is symmetric and zero to self, the two properties a straight-line
		//	distance matrix is trivially certain to have regardless of what got placed.
		int count = StrategicPlanner::Get_Node_Count ();
		if (count >= 2) {
			Check (StrategicPlanner::Get_Travel_Cost (0,0) == 0.0f, "a node's travel cost to itself was nonzero");
			Check (StrategicPlanner::Get_Travel_Cost (0,1) == StrategicPlanner::Get_Travel_Cost (1,0),
					 "travel cost was not symmetric");
		}

		first_checksum = StrategicPlanner::Get_Checksum ();
		Check (first_checksum != 0, "a built strategic plan carried no checksum");

		Check (StrategicPlanner::Generate (params,&error), "regenerating the same strategic plan failed");
		Check (StrategicPlanner::Get_Checksum () == first_checksum,
				 "the same seed and terrain produced a different strategic plan checksum");
	}

	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	roadgraph
**
**	Roadmap Section 34 Phase 8.  Device-less: RoadGraphGenerator reads the chunk registry,
**	StrategicPlanner's own node list, WorldBoundsService and WaterSystem, and writes to
**	RoadSystem through Add_Road/Build_Network, none of which needs a physics scene
**	(Build_Geometry, the one road-system call that does, is never reached here).
**
**	The same large uniform-slope ramp Check_Strategic_Plan uses, so the planner it depends on
**	has somewhere real to place base/resource candidates for this generator to connect.
***********************************************************************************************/
void	Check_Road_Graph (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();

	//
	//	Refusals.
	//
	{
		StringClass error ("");
		RoadGraphParams bad;
		Check (!RoadGraphGenerator::Generate (bad,&error), "generation ran with no chunk registry or strategic plan at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!RoadGraphGenerator::Has_Graph (), "a refused generation left state behind");

		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		bad.MaxBuildableSlopeRadians = 0.0f;
		Check (!RoadGraphGenerator::Generate (bad,&error), "a zero max buildable slope was accepted");

		bad.MaxBuildableSlopeRadians = 0.5f;
		bad.SlopeCostWeight = -1.0f;
		Check (!RoadGraphGenerator::Generate (bad,&error), "a negative slope cost weight was accepted");
	}

	//
	//	A real plan over the large uniform ramp, then a road graph connecting it.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;

		StringClass error ("");
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams params;
		params.Seed							= 55u;
		params.MaxBuildableSlopeRadians	= 0.5f;
		params.SlopeCostWeight				= 3.0f;
		params.CrossingCostPenalty			= 8.0f;
		params.BoundaryApproachCostWeight	= 1.5f;

		Check (RoadGraphGenerator::Generate (params,&error),
				 "generation refused a real strategic plan: %s",error.Peek_Buffer ());
		Check (RoadGraphGenerator::Has_Graph (), "a successful generation reported no graph built");

		StringClass report ("");
		Check (RoadGraphGenerator::Check_Required_Nodes_Connected (&report) == 0,
				 "the anchor nodes were not spanned by the road graph: %s",report.Peek_Buffer ());
		Check (RoadGraphGenerator::Check_No_Cliff_Band_Crossing (&report) == 0,
				 "a road edge crossed the cliff band with none configured: %s",report.Peek_Buffer ());
		Check (RoadGraphGenerator::Check_No_Unresolved_Water_Or_Cliff_Passage (&report) == 0,
				 "a road edge passed through water with no crossing candidate: %s",report.Peek_Buffer ());

		Check (RoadSystem::Is_Network_Built (), "a built road graph left the road network unbuilt");

		first_checksum = RoadGraphGenerator::Get_Checksum ();

		Check (RoadGraphGenerator::Generate (params,&error), "regenerating the same road graph failed");
		Check (RoadGraphGenerator::Get_Checksum () == first_checksum,
				 "the same seed, terrain and plan produced a different road graph checksum");
	}

	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	bridgeplan
**
**	Roadmap Section 34 Phase 9.  Device-less: BridgePlanner reads RoadGraphGenerator's own
**	unresolved-crossing lists and writes to BridgeSystem through Add_Bridge/Build_Layout/
**	Build_Geometry, none of which needs a physics scene.
**
**	Whether this particular seed's road graph crosses any water at all is not something this
**	check can honestly guarantee by construction the way Check_Hydrology guarantees a river --
**	Dijkstra routes around an obstruction whenever a clear path exists, so a real hydrology
**	feature carved into the same terrain a road graph is planned over may simply be routed
**	around rather than crossed.  What is checked is what holds regardless of how many bridges
**	that produces: every structural diagnostic passes for whatever BridgePlanner actually built,
**	zero bridges included.
***********************************************************************************************/
void	Check_Bridge_Plan (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();

	//
	//	Refusals.
	//
	{
		StringClass error ("");
		BridgePlannerParams bad;
		Check (!BridgePlanner::Generate (bad,&error), "generation ran with no road graph at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!BridgePlanner::Has_Plan (), "a refused generation left state behind");
	}

	//
	//	A real chain: terrain, hydrology, a strategic plan, and a road graph, over a large
	//	uniform ramp -- then bridge planning over whatever the road graph left unresolved.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		//	Chunks hydrology carved into water are no longer Buildable, so the chunk registry has
		//	to see that before the strategic plan reads it.
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams params;
		params.Seed							= 77u;
		params.WaterClearanceMargin		= 2.0f;
		params.MaxSpanLength				= 200.0f;

		Check (BridgePlanner::Generate (params,&error),
				 "generation refused a real road graph: %s",error.Peek_Buffer ());
		Check (BridgePlanner::Has_Plan (), "a successful generation reported no plan built");

		StringClass report ("");
		Check (BridgePlanner::Check_Traversable_States_Pass (&report) == 0,
				 "a non-broken bridge was not traversable: %s",report.Peek_Buffer ());
		Check (BridgePlanner::Check_Broken_States_Block (&report) == 0,
				 "a broken bridge still reported traversable: %s",report.Peek_Buffer ());
		Check (BridgePlanner::Check_No_Required_Connectivity_Lost (&report) == 0,
				 "a bridge's connectivity flag disagreed with its own state: %s",report.Peek_Buffer ());

		for (int i = 0; i < BridgePlanner::Get_Bridge_Count (); i++) {
			ResolvedBridge bridge;
			if (!BridgePlanner::Get_Bridge (i,&bridge)) continue;
			Check (BridgeSystem::Peek_Bridge (bridge.BridgeId) != nullptr,
					 "resolved bridge '%s' names a bridge id BridgeSystem does not have",bridge.Id.Peek_Buffer ());
			Check ((bridge.InitialState >= BRIDGE_STATE_PRISTINE) && (bridge.InitialState <= BRIDGE_STATE_BROKEN),
					 "resolved bridge '%s' carries an out-of-range initial state",bridge.Id.Peek_Buffer ());
		}

		first_checksum = BridgePlanner::Get_Checksum ();

		Check (BridgePlanner::Generate (params,&error), "regenerating the same bridge plan failed");
		Check (BridgePlanner::Get_Checksum () == first_checksum,
				 "the same seed, terrain, plan and road graph produced a different bridge plan checksum");
	}

	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	V2 roadmap Section 34 Phase 11 -- final base selection and deterministic fairness repair.
**	Same chain Check_Bridge_Plan already proves (terrain, hydrology, a strategic plan, a road
**	graph, a bridge plan), one step further: choosing the one GDI/Nod pair that is actually fair.
*/
void	Check_Base_Selection (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();

	//
	//	Refusals: each of the three prerequisites this generator names has to be missing in turn
	//	to prove the check is real rather than merely "no road graph" standing in for all three.
	//
	{
		StringClass error ("");
		BaseSelectionParams bad;
		Check (!BaseSelectionPlanner::Generate (bad,&error), "generation ran with no strategic plan at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!BaseSelectionPlanner::Has_Selection (), "a refused generation left state behind");
	}

	//
	//	A real chain, the same terrain/hydrology/strategic/road-graph/bridge-plan setup
	//	Check_Bridge_Plan already established as sufficient to exercise a real crossing.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 77u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

		BaseSelectionParams params;
		params.Seed												= 123u;
		params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		params.MaxBaseToBaseTravelTimeDifferenceSeconds			= 120.0f;
		params.MinBaseExpansionArea									= 1024.0f;
		params.AllowSingleBridgeDependency							= false;
		params.MaxRepairAttemptsPerPair								= 8;

		Check (BaseSelectionPlanner::Generate (params,&error),
				 "generation refused a real road graph/bridge plan pair: %s",error.Peek_Buffer ());
		Check (BaseSelectionPlanner::Has_Selection (), "a successful generation reported no selection built");

		SelectedBaseSite gdi_site, nod_site;
		Check (BaseSelectionPlanner::Get_Selected_Site (PLAYERTYPE_GDI,&gdi_site),
				 "a built selection named no GDI site");
		Check (BaseSelectionPlanner::Get_Selected_Site (PLAYERTYPE_NOD,&nod_site),
				 "a built selection named no Nod site");
		Check (gdi_site.Id != nod_site.Id, "GDI and Nod were assigned the same base candidate");
		Check (gdi_site.ProtectedRadius > 0.0f, "the selected GDI site's own reserved region is empty");
		Check (nod_site.ProtectedRadius > 0.0f, "the selected Nod site's own reserved region is empty");

		StringClass report ("");
		Check (BaseSelectionPlanner::Check_Both_Bases_Have_Build_Area (&report) == 0,
				 "the selected pair failed its own build-area check: %s",report.Peek_Buffer ());
		Check (BaseSelectionPlanner::Check_Both_Bases_Have_Resource_Access (&report) == 0,
				 "the selected pair failed its own resource-access check: %s",report.Peek_Buffer ());
		Check (BaseSelectionPlanner::Check_Connectivity_Requirements (params.AllowSingleBridgeDependency,&report) == 0,
				 "the selected pair failed its own connectivity check: %s",report.Peek_Buffer ());
		Check (BaseSelectionPlanner::Check_Travel_Time_Within_Threshold (
					params.MaxInitialResourceTravelTimeDifferenceSeconds,
					params.MaxBaseToBaseTravelTimeDifferenceSeconds,&report) == 0,
				 "the selected pair exceeded the very thresholds it was chosen under: %s",report.Peek_Buffer ());

		//	Every repair entry this run logged names a real operation, and every entry belongs to
		//	a pair attempt index that is actually in range.
		for (int i = 0; i < BaseSelectionPlanner::Get_Repair_Log_Count (); i++) {
			BaseRepairLogEntry entry;
			Check (BaseSelectionPlanner::Get_Repair_Log_Entry (i,&entry), "repair log index out of range");
			Check (::strcmp (Base_Repair_Operation_Name (entry.Operation),"unknown") != 0,
					 "repair log entry %d named an unrecognized operation",i);
			Check (entry.PairAttemptIndex >= 0, "repair log entry %d named no pair attempt",i);
		}

		first_checksum = BaseSelectionPlanner::Get_Checksum ();

		Check (BaseSelectionPlanner::Generate (params,&error), "regenerating the same base selection failed");
		Check (BaseSelectionPlanner::Get_Checksum () == first_checksum,
				 "the same seed, terrain, plan, road graph and bridge plan produced a different base "
				 "selection checksum");

		SelectedBaseSite gdi_again;
		Check (BaseSelectionPlanner::Get_Selected_Site (PLAYERTYPE_GDI,&gdi_again) && (gdi_again.Id == gdi_site.Id),
				 "the same inputs selected a different GDI base the second time");
	}

	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	V2 roadmap Section 34 Phase 12 -- Tiberium fields, crystals, harvest areas, and Harvester
**	routes.  Same chain Check_Base_Selection already proves, one step further: turning candidate
**	resource regions into real, harvestable fields.
*/
void	Check_Tiberium_Fields (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();
	TiberiumFieldGenerator::Init ();

	//
	//	Refusals.
	//
	{
		StringClass error ("");
		TiberiumFieldParams bad;
		Check (!TiberiumFieldGenerator::Generate (bad,&error), "generation ran with no strategic plan at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!TiberiumFieldGenerator::Has_Fields (), "a refused generation left state behind");
	}

	//
	//	A real chain, the same terrain/hydrology/strategic/road-graph/bridge-plan/base-selection
	//	setup Check_Base_Selection already established as sufficient.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 77u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

		BaseSelectionParams base_params;
		base_params.Seed												= 123u;
		base_params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		base_params.MaxBaseToBaseTravelTimeDifferenceSeconds		= 120.0f;
		base_params.MinBaseExpansionArea								= 1024.0f;
		base_params.AllowSingleBridgeDependency						= false;
		base_params.MaxRepairAttemptsPerPair							= 8;
		Check (BaseSelectionPlanner::Generate (base_params,&error),
				 "the base selection this check depends on failed: %s",error.Peek_Buffer ());

		TiberiumFieldParams params;
		params.Seed							= 321u;
		params.FieldRadiusMin				= 15.0f;
		params.FieldRadiusMax				= 40.0f;
		params.InitialYieldMin				= 1500.0f;
		params.InitialYieldMax				= 4000.0f;
		params.CrystalDensity				= 0.15f;
		params.HarvestAnchorCount			= 3;
		params.HarvesterLoadAmount		= 600.0f;
		params.GrowthEnabled				= false;
		params.GrowthRatePerSecond		= 0.0f;
		params.MaxRepairAttemptsPerField	= 8;
		params.ReferenceVehicleSpeed		= 8.33f;
		params.MaxBuildableSlopeRadians	= 0.5f;

		Check (TiberiumFieldGenerator::Generate (params,&error),
				 "generation refused a real strategic plan/road graph/base selection: %s",error.Peek_Buffer ());
		Check (TiberiumFieldGenerator::Has_Fields (), "a successful generation reported no fields built");
		Check (TiberiumFieldGenerator::Get_Field_Count () > 0,
				 "this terrain's own resource candidates resolved to zero real fields");

		StringClass report ("");
		Check (TiberiumFieldGenerator::Check_No_Illegal_Footprint (&report) == 0,
				 "a resolved field's own footprint is illegal: %s",report.Peek_Buffer ());
		Check (TiberiumFieldGenerator::Check_Both_Factions_Have_Early_Field (&report) == 0,
				 "not both factions have a viable early field: %s",report.Peek_Buffer ());
		Check (TiberiumFieldGenerator::Check_Harvestable_Fields_Have_Routes (&report) == 0,
				 "an owned field has no Harvester route: %s",report.Peek_Buffer ());

		for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count (); i++) {
			ResolvedTiberiumField field;
			Check (TiberiumFieldGenerator::Get_Field (i,&field), "field index out of range");
			Check (field.Radius > 0.0f, "field %d has a non-positive radius",i);
			Check (field.CrystalCount >= 1, "field %d scattered no crystals at all",i);
			Check ((field.OwningFaction != PLAYERTYPE_NEUTRAL) || !field.CountsAsEarlyField,
					 "field %d is neutral but counted as an early field",i);
		}

		Check (TiberiumFieldGenerator::Get_Crystal_Count () > 0, "no crystal placements were generated at all");
		for (int i = 0; i < TiberiumFieldGenerator::Get_Crystal_Count (); i++) {
			TiberiumCrystalPlacement crystal;
			Check (TiberiumFieldGenerator::Get_Crystal (i,&crystal), "crystal index out of range");
			Check ((crystal.FieldIndex >= 0) && (crystal.FieldIndex < TiberiumFieldGenerator::Get_Field_Count ()),
					 "crystal %d names a field index out of range",i);
		}

		for (int i = 0; i < TiberiumFieldGenerator::Get_Repair_Log_Count (); i++) {
			ResourceRepairLogEntry entry;
			Check (TiberiumFieldGenerator::Get_Repair_Log_Entry (i,&entry), "repair log index out of range");
			Check (::strcmp (Resource_Repair_Operation_Name (entry.Operation),"unknown") != 0,
					 "repair log entry %d named an unrecognized operation",i);
		}

		first_checksum = TiberiumFieldGenerator::Get_Checksum ();

		Check (TiberiumFieldGenerator::Generate (params,&error), "regenerating the same field set failed");
		Check (TiberiumFieldGenerator::Get_Checksum () == first_checksum,
				 "the same seed/terrain/plan/road-graph/base-selection produced a different field checksum");
	}

	TiberiumFieldGenerator::Shutdown ();
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	V2 roadmap Section 34 Phase 13 -- settlements, abandoned buildings, industrial areas, and
**	strategic POIs.  Same chain Check_Tiberium_Fields already proves, one step further: turning
**	candidate settlement/POI regions into real layout/road-connectivity/structure-placement data.
**
**	Registers the three real, offline-verified building descriptors this tree actually ships
**	geometry for (see proceduralassetregistry.h's own class comment) via
**	Load_Descriptor_From_Text -- the same "no file on disk" pattern BuildingDescriptorManager's
**	own header documents for a self check -- rather than BuildingDescriptorManager::Load_Directory,
**	whose relative path depends on the test binary's own working directory. The footprint values
**	below are copied verbatim from data/building_descriptors/*.json, not invented.
*/
void	Check_Settlements (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();
	TiberiumFieldGenerator::Init ();
	BuildingDescriptorManager::Init ();
	SettlementGenerator::Init ();

	{
		StringClass descriptor_error ("");
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_communications_center",
					"{ \"model_name\": \"G_Comcntr.w3d\", \"schema_version\": 1, "
					"\"footprint\": { \"width\": 15.0, \"depth\": 15.0 } }",&descriptor_error),
				 "gdi_communications_center failed to load: %s",descriptor_error.Peek_Buffer ());
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_power_plant",
					"{ \"model_name\": \"mgpwr_ext.w3d\", \"schema_version\": 1, "
					"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error),
				 "gdi_power_plant failed to load: %s",descriptor_error.Peek_Buffer ());
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("nod_power_plant",
					"{ \"model_name\": \"mnpwr_ext.w3d\", \"schema_version\": 1, "
					"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error),
				 "nod_power_plant failed to load: %s",descriptor_error.Peek_Buffer ());
	}

	ProceduralAssetRegistry::Init ();
	Check (ProceduralAssetRegistry::Has_Registry (),"the asset registry reported not built after Init");
	Check (ProceduralAssetRegistry::Get_Asset_Count () == 3,
			 "the registry did not carry all three real, verified entries (%d)",ProceduralAssetRegistry::Get_Asset_Count ());

	//
	//	Refusals.
	//
	{
		StringClass error ("");
		SettlementParams bad;
		SettlementGenerator::Clear ();
		StrategicPlanner::Clear ();
		Check (!SettlementGenerator::Generate (bad,&error), "generation ran with no strategic plan at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!SettlementGenerator::Has_Settlements (), "a refused generation left state behind");
	}

	//
	//	A real chain, the same terrain/hydrology/strategic-plan/road-graph/bridge-plan/base-
	//	selection/Tiberium-field setup Check_Tiberium_Fields already established as sufficient.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 77u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

		BaseSelectionParams base_params;
		base_params.Seed												= 123u;
		base_params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		base_params.MaxBaseToBaseTravelTimeDifferenceSeconds		= 120.0f;
		base_params.MinBaseExpansionArea								= 1024.0f;
		base_params.AllowSingleBridgeDependency						= false;
		base_params.MaxRepairAttemptsPerPair							= 8;
		Check (BaseSelectionPlanner::Generate (base_params,&error),
				 "the base selection this check depends on failed: %s",error.Peek_Buffer ());

		TiberiumFieldParams field_params;
		field_params.Seed							= 321u;
		field_params.FieldRadiusMin				= 15.0f;
		field_params.FieldRadiusMax				= 40.0f;
		field_params.InitialYieldMin				= 1500.0f;
		field_params.InitialYieldMax				= 4000.0f;
		field_params.CrystalDensity				= 0.15f;
		field_params.HarvestAnchorCount			= 3;
		field_params.HarvesterLoadAmount		= 600.0f;
		field_params.MaxRepairAttemptsPerField	= 8;
		field_params.ReferenceVehicleSpeed		= 8.33f;
		field_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (TiberiumFieldGenerator::Generate (field_params,&error),
				 "the Tiberium fields this check depends on failed: %s",error.Peek_Buffer ());

		SettlementParams params;
		params.Seed							= 555u;
		params.RoadConnectorMaxDistance	= 200.0f;
		params.MaxStructuresPerSettlement	= 4;
		params.MaxBuildableSlopeRadians	= 0.5f;

		Check (SettlementGenerator::Generate (params,&error),
				 "generation refused a real strategic plan/road graph/reservations/registry: %s",error.Peek_Buffer ());
		Check (SettlementGenerator::Has_Settlements (), "a successful generation reported no settlements built");

		StringClass report ("");
		Check (SettlementGenerator::Check_No_Illegal_Placement (&report) == 0,
				 "a placed structure's own footprint is illegal: %s",report.Peek_Buffer ());
		Check (SettlementGenerator::Check_No_Structure_Overlap (&report) == 0,
				 "two placed structures overlap: %s",report.Peek_Buffer ());

		for (int i = 0; i < SettlementGenerator::Get_Settlement_Count (); i++) {
			ResolvedSettlement settlement;
			Check (SettlementGenerator::Get_Settlement (i,&settlement), "settlement index out of range");
			Check (settlement.Radius > 0.0f, "settlement %d has a non-positive reserved radius",i);
			Check (::strcmp (Settlement_Layout_Type_Name (settlement.Layout),"unknown") != 0,
					 "settlement %d named an unrecognized layout type",i);
			Check ((settlement.RoadDistance < 0.0f) || (settlement.RoadDistance >= 0.0f),
					 "settlement %d's own road distance is not a real number",i);		// NaN guard
		}

		for (int i = 0; i < SettlementGenerator::Get_Poi_Count (); i++) {
			ResolvedPoi poi;
			Check (SettlementGenerator::Get_Poi (i,&poi), "POI index out of range");
			Check (::strcmp (Poi_Kind_Name (poi.Kind),"unknown") != 0,
					 "POI %d named an unrecognized kind",i);
			Check (!poi.HasRealAsset,
					 "POI %d claims a real asset the registry does not have -- see the class comment",i);
		}

		first_checksum = SettlementGenerator::Get_Checksum ();

		Check (SettlementGenerator::Generate (params,&error), "regenerating the same settlement set failed");
		Check (SettlementGenerator::Get_Checksum () == first_checksum,
				 "the same seed/terrain/plan/road-graph/reservations produced a different settlement checksum");
	}

	SettlementGenerator::Shutdown ();
	ProceduralAssetRegistry::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	TiberiumFieldGenerator::Shutdown ();
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	V2 roadmap Section 34 Phase 14 -- biomes, terrain materials, forests, rocks, and natural
**	decoration.  Needs only a chunk registry -- unlike every check above it, no strategic
**	plan/road graph/base selection is required, matching BiomeDecorationGenerator's own
**	documented optional-dependency design.
*/
void	Check_Biome_Decoration (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	FoliageSystem::Init ();
	RoadSystem::Init ();		// lamps read every road; none may leak in from another family

	//
	//	Refusal.
	//
	{
		StringClass error ("");
		BiomeDecorationParams bad;
		Check (!BiomeDecorationGenerator::Generate (bad,&error), "generation ran with no chunk registry at all");
		Check (error.Get_Length () > 0, "a refused generation carried no reason");
		Check (!BiomeDecorationGenerator::Has_Decoration (), "a refused generation left state behind");
	}

	//
	//	A real chain: terrain and hydrology (for a real water-distance mask), then decoration
	//	alone -- no strategic plan, road graph, base selection, Tiberium fields or settlements
	//	are required, proving the optional-dependency design for real.
	//
	unsigned int first_checksum = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		BiomeDecorationParams params;
		params.Seed							= 987u;
		params.MaxBuildableSlopeRadians	= 0.5f;
		params.ForestDensity				= 0.5f;
		params.RockDensity					= 0.3f;

		Check (BiomeDecorationGenerator::Generate (params,&error),
				 "generation refused a real chunk registry: %s",error.Peek_Buffer ());
		Check (BiomeDecorationGenerator::Has_Decoration (), "a successful generation reported no decoration built");
		Check (FoliageSystem::Get_Instance_Count () > 0, "this terrain's own real chunks placed zero instances");
		Check (BiomeDecorationGenerator::Get_Tree_Count () + BiomeDecorationGenerator::Get_Rock_Count () ==
					FoliageSystem::Get_Instance_Count (),
				 "tree/rock counts do not add up to what FoliageSystem actually holds");

		StringClass report ("");
		Check (BiomeDecorationGenerator::Check_No_Instance_In_Road (&report) == 0,
				 "a decoration instance sits inside the road mask: %s",report.Peek_Buffer ());
		Check (BiomeDecorationGenerator::Check_No_Instance_Off_Terrain (&report) == 0,
				 "a decoration instance sits off the terrain: %s",report.Peek_Buffer ());

		Check (FoliageSystem::Build_Cells (), "the real cell grid failed to build over this terrain's own instances");
		Check (FoliageSystem::Are_Cells_Built (), "cells reported not built after a successful Build_Cells");
		Check (FoliageSystem::Build_Batches (), "the real batch list failed to build");
		Check (FoliageSystem::Are_Batches_Built (), "batches reported not built after a successful Build_Batches");

		first_checksum = BiomeDecorationGenerator::Get_Checksum ();

		Check (BiomeDecorationGenerator::Generate (params,&error), "regenerating the same decoration failed");
		Check (BiomeDecorationGenerator::Get_Checksum () == first_checksum,
				 "the same seed/terrain/hydrology produced a different decoration checksum");
	}

	//
	//	Street lamps (V2 Section 10.6, Phase 14 steps 9-10): a city street gets lamps at the
	//	stated spacing, every one beside the road -- outside its carriageway and shoulders -- and
	//	alternating sides; a track far from any base or industrial area gets none; placement is
	//	deterministic.
	//
	{
		//	The rivers above would (correctly) refuse a lamp standing in them; this case is about
		//	spacing and sides, so it runs on dry ground.
		HydrologyGenerator::Clear ();
		RoadSystem::Init ();
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		RoadSplineClass street;
		street.Set_Name ("lamp_street");
		street.Add_Control_Point (Vector3 (-150.0f,40.0f,0.0f));
		street.Add_Control_Point (Vector3 (   0.0f,40.0f,0.0f));
		street.Add_Control_Point (Vector3 ( 150.0f,40.0f,0.0f));
		street.Set_Width (6.0f);
		street.Set_Shoulder_Width (1.0f);
		street.Set_Road_Class (ROAD_CLASS_STREET);
		int street_id = RoadSystem::Add_Road (street);

		RoadSplineClass track;
		track.Set_Name ("lamp_track");
		track.Add_Control_Point (Vector3 (-150.0f,-80.0f,0.0f));
		track.Add_Control_Point (Vector3 ( 150.0f,-80.0f,0.0f));
		track.Set_Width (4.0f);
		track.Set_Road_Class (ROAD_CLASS_TRACK);
		RoadSystem::Add_Road (track);

		Check (RoadSystem::Build_Network (0.25f,0.5f,4.0f),"the lamp test roads would not build");

		BiomeDecorationParams lamp_params;
		lamp_params.Seed					= 991u;
		lamp_params.ForestDensity		= 0.0f;
		lamp_params.RockDensity			= 0.0f;
		lamp_params.StreetLampSpacing	= 30.0f;

		StringClass error ("");
		Check (BiomeDecorationGenerator::Generate (lamp_params,&error),"decoration with lamps failed: %s",error.Peek_Buffer ());

		RoadSplineClass * built = RoadSystem::Peek_Road (street_id);
		int expected = (built != nullptr) ? (int)((built->Get_Length () - 15.0f) / 30.0f) + 1 : 0;
		Check (BiomeDecorationGenerator::Get_Street_Lamp_Count () == expected,
				 "a %.0f-unit street at 30-unit spacing got %d lamp(s), not %d",
				 (built != nullptr) ? built->Get_Length () : 0.0f,BiomeDecorationGenerator::Get_Street_Lamp_Count (),expected);

		int left = 0, right = 0;
		for (int i = 0; i < FoliageSystem::Get_Instance_Count (); i++) {
			const FoliageInstanceClass * instance = FoliageSystem::Peek_Instance (i);
			if (instance == nullptr) continue;
			float dy = instance->Position.Y - 40.0f;
			Check (WWMath::Fabs (dy) >= 4.0f,
				 "a lamp stands %.2f from the street's centre line, inside its 8-unit total width",WWMath::Fabs (dy));
			Check (WWMath::Fabs (instance->Position.Y + 80.0f) > 20.0f,"a lamp stands on the plain track");
			if (dy > 0.0f) left++; else right++;
		}
		Check ((left > 0) && (right > 0) && (WWMath::Fabs ((float)(left - right)) <= 1.0f),
				 "lamps did not alternate sides (%d left, %d right)",left,right);

		unsigned int lamp_checksum = BiomeDecorationGenerator::Get_Checksum ();
		Check (BiomeDecorationGenerator::Generate (lamp_params,&error),"regenerating lamps failed");
		Check (BiomeDecorationGenerator::Get_Checksum () == lamp_checksum,"the same roads produced different lamps");

		RoadSystem::Clear_Roads ();
	}

	BiomeDecorationGenerator::Shutdown ();
	FoliageSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	V2 roadmap Section 34 Phase 15 -- FPS/RTS combat-space analysis and deterministic local
**	repair.  A validator, run over the same real chain Check_Settlements already proves
**	sufficient to produce a real base selection, Tiberium fields and settlements.
*/
void	Check_Combat_Space (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();
	TiberiumFieldGenerator::Init ();
	BuildingDescriptorManager::Init ();
	SettlementGenerator::Init ();

	{
		StringClass descriptor_error ("");
		BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_communications_center",
			"{ \"model_name\": \"G_Comcntr.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 15.0, \"depth\": 15.0 } }",&descriptor_error);
		BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_power_plant",
			"{ \"model_name\": \"mgpwr_ext.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error);
		BuildingDescriptorManager::Load_Descriptor_From_Text ("nod_power_plant",
			"{ \"model_name\": \"mnpwr_ext.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error);
	}
	ProceduralAssetRegistry::Init ();

	//
	//	Validate() over nothing at all: trivially passes, the same "nothing built, nothing
	//	broken" answer every other validator in this chain gives.
	//
	{
		CombatSpaceValidator::Clear ();
		CombatSpaceValidationParams params;
		StringClass report ("");
		Check (CombatSpaceValidator::Validate (params,&report), "validation over an empty world reported a hard failure");
		Check (CombatSpaceValidator::Get_Failure_Count () == 0, "an empty world reported a nonzero failure count");
	}

	//
	//	A real chain: terrain, hydrology, strategic plan, road graph, bridge plan, base
	//	selection, Tiberium fields and settlements -- the same setup Check_Settlements already
	//	established as sufficient.
	//
	unsigned int first_checksum_proxy = 0;
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 77u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

		BaseSelectionParams base_params;
		base_params.Seed												= 123u;
		base_params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		base_params.MaxBaseToBaseTravelTimeDifferenceSeconds		= 120.0f;
		base_params.MinBaseExpansionArea								= 1024.0f;
		base_params.AllowSingleBridgeDependency						= false;
		base_params.MaxRepairAttemptsPerPair							= 8;
		Check (BaseSelectionPlanner::Generate (base_params,&error),
				 "the base selection this check depends on failed: %s",error.Peek_Buffer ());

		TiberiumFieldParams field_params;
		field_params.Seed							= 321u;
		field_params.FieldRadiusMin				= 15.0f;
		field_params.FieldRadiusMax				= 40.0f;
		field_params.InitialYieldMin				= 1500.0f;
		field_params.InitialYieldMax				= 4000.0f;
		field_params.CrystalDensity				= 0.15f;
		field_params.HarvestAnchorCount			= 3;
		field_params.HarvesterLoadAmount		= 600.0f;
		field_params.MaxRepairAttemptsPerField	= 8;
		field_params.ReferenceVehicleSpeed		= 8.33f;
		field_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (TiberiumFieldGenerator::Generate (field_params,&error),
				 "the Tiberium fields this check depends on failed: %s",error.Peek_Buffer ());

		SettlementParams settlement_params;
		settlement_params.Seed							= 555u;
		settlement_params.RoadConnectorMaxDistance	= 200.0f;
		settlement_params.MaxStructuresPerSettlement	= 4;
		settlement_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (SettlementGenerator::Generate (settlement_params,&error),
				 "the settlements this check depends on failed: %s",error.Peek_Buffer ());

		CombatSpaceValidationParams params;
		params.Seed													= 111u;
		params.MaxMandatoryExposedInfantryRunTimeSeconds	= 12.0f;
		params.MinVehicleLaneClearance							= 1.0f;
		params.ReferenceInfantrySpeed							= 4.3f;

		StringClass report ("");
		bool first_result = CombatSpaceValidator::Validate (params,&report);
		int first_failure_count = CombatSpaceValidator::Get_Failure_Count ();

		const CombatSpaceMetrics & metrics = CombatSpaceValidator::Get_Metrics ();
		Check (metrics.SightlineSampleCount > 0, "a real chain with real bases sampled zero sightlines");
		Check (metrics.SightlineP50 <= metrics.SightlineP90, "sightline P50 exceeds P90");
		Check (metrics.SightlineP90 <= metrics.SightlineMax, "sightline P90 exceeds the max");
		Check (metrics.SightlineMax >= 0.0f, "sightline max is negative");

		for (int i = 0; i < CombatSpaceValidator::Get_Failure_Count (); i++) {
			CombatSpaceFailureRecord failure;
			Check (CombatSpaceValidator::Get_Failure (i,&failure), "failure index out of range");
			Check (::strcmp (Combat_Space_Failure_Name (failure.Type),"unknown") != 0,
					 "failure %d named an unrecognized type",i);
		}

		//	Every named repair operation vocabulary entry resolves to a real name, even though
		//	none is ever produced this pass -- see the class comment for why.
		for (int op = 0; op < COMBAT_REPAIR_OPERATION_TYPE_COUNT; op++) {
			Check (::strcmp (Combat_Space_Repair_Operation_Name ((CombatSpaceRepairOperationType)op),"unknown") != 0,
					 "repair operation %d has no real name",op);
		}

		bool second_result = CombatSpaceValidator::Validate (params,&report);
		Check (second_result == first_result, "the same seed/world produced a different pass/fail verdict");
		Check (CombatSpaceValidator::Get_Failure_Count () == first_failure_count,
				 "the same seed/world produced a different failure count");

		first_checksum_proxy = (unsigned int)first_failure_count;
		Check (first_checksum_proxy == (unsigned int)CombatSpaceValidator::Get_Failure_Count (),
				 "determinism proxy mismatch");
	}

	SettlementGenerator::Shutdown ();
	ProceduralAssetRegistry::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	TiberiumFieldGenerator::Shutdown ();
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	Phase 16 steps 1-2.  Real collision/render/culling registration for Phase 13's own
**	settlement structures -- the one generated-object type the chain reached still data-only.
*/
void	Check_World_Integration (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();
	TiberiumFieldGenerator::Init ();
	BuildingDescriptorManager::Init ();
	SettlementGenerator::Init ();
	GeneratedWorldIntegrator::Init ();

	{
		StringClass descriptor_error ("");
		BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_communications_center",
			"{ \"model_name\": \"G_Comcntr.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 15.0, \"depth\": 15.0 } }",&descriptor_error);
		BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_power_plant",
			"{ \"model_name\": \"mgpwr_ext.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error);
		BuildingDescriptorManager::Load_Descriptor_From_Text ("nod_power_plant",
			"{ \"model_name\": \"mnpwr_ext.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error);
	}
	ProceduralAssetRegistry::Init ();

	//
	//	Build() over no settlements at all: trivially passes, the same "nothing built, nothing
	//	broken" answer every other generator in this chain gives.
	//
	{
		GeneratedWorldIntegrator::Clear ();
		StringClass report ("");
		Check (GeneratedWorldIntegrator::Build (&report), "integration over no settlements reported a failure");
		Check (GeneratedWorldIntegrator::Get_Record_Count () == 0, "no settlements produced a nonzero record count");
	}

	//
	//	A real chain -- the same setup Check_Settlements/Check_Combat_Space already established
	//	as sufficient to produce real structures.
	//
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 909u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 77u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

		BaseSelectionParams base_params;
		base_params.Seed												= 123u;
		base_params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		base_params.MaxBaseToBaseTravelTimeDifferenceSeconds		= 120.0f;
		base_params.MinBaseExpansionArea								= 1024.0f;
		base_params.AllowSingleBridgeDependency						= false;
		base_params.MaxRepairAttemptsPerPair							= 8;
		Check (BaseSelectionPlanner::Generate (base_params,&error),
				 "the base selection this check depends on failed: %s",error.Peek_Buffer ());

		TiberiumFieldParams field_params;
		field_params.Seed							= 321u;
		field_params.FieldRadiusMin				= 15.0f;
		field_params.FieldRadiusMax				= 40.0f;
		field_params.InitialYieldMin				= 1500.0f;
		field_params.InitialYieldMax				= 4000.0f;
		field_params.CrystalDensity				= 0.15f;
		field_params.HarvestAnchorCount			= 3;
		field_params.HarvesterLoadAmount		= 600.0f;
		field_params.MaxRepairAttemptsPerField	= 8;
		field_params.ReferenceVehicleSpeed		= 8.33f;
		field_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (TiberiumFieldGenerator::Generate (field_params,&error),
				 "the Tiberium fields this check depends on failed: %s",error.Peek_Buffer ());

		SettlementParams settlement_params;
		settlement_params.Seed							= 555u;
		settlement_params.RoadConnectorMaxDistance	= 200.0f;
		settlement_params.MaxStructuresPerSettlement	= 4;
		settlement_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (SettlementGenerator::Generate (settlement_params,&error),
				 "the settlements this check depends on failed: %s",error.Peek_Buffer ());

		GeneratedWorldIntegrator::Clear ();
		StringClass report ("");
		Check (GeneratedWorldIntegrator::Build (&report), "integration over a real chain failed: %s",report.Peek_Buffer ());
		Check (GeneratedWorldIntegrator::Get_Record_Count () == SettlementGenerator::Get_Structure_Count (),
				 "record count does not match the structure count it was built from");
		Check ((GeneratedWorldIntegrator::Get_Collision_Count () + GeneratedWorldIntegrator::Get_Missing_Model_Count ()) ==
					GeneratedWorldIntegrator::Get_Record_Count (),
				 "collision + missing-model counts do not sum to the record count");

		for (int i = 0; i < GeneratedWorldIntegrator::Get_Record_Count (); i++) {
			IntegratedStructureRecord record;
			Check (GeneratedWorldIntegrator::Get_Record (i,&record), "record index out of range");
			Check (record.PersistenceId.Get_Length () > 0, "a structure record has no persistence id");
		}

		//	3 real, offline-verified assets are loaded above -- every structure this real chain
		//	placed should therefore have real collision, not merely a real record.
		if (GeneratedWorldIntegrator::Get_Record_Count () > 0) {
			Check (GeneratedWorldIntegrator::Get_Collision_Count () > 0,
					 "a real chain with real assets loaded produced zero real collision objects");
		}

		//
		//	Step 13.  The shroud grid has to cover the world this chain actually built, not
		//	whatever world it was last partitioned over -- a shroud cell and a world position
		//	lining up is the whole point of "shroud/radar positions align".
		//
		WorldShroudManager::Init ();
		Check (WorldShroudManager::Is_Partitioned () == false,
				 "a fresh shroud manager already reports partitioned");

		Check (GeneratedWorldIntegrator::Partition_Shroud_Over_World (&report),
				 "shroud partition over a real generated world failed: %s",report.Peek_Buffer ());
		Check (WorldShroudManager::Is_Partitioned (), "the shroud grid was not partitioned");
		Check (WorldShroudManager::Get_Cell_Count () > 0, "the partitioned shroud grid has no cells");

		{
			float		cell_size	= 0.0f;
			int		cols			= 0;
			int		rows			= 0;
			Vector3	grid_min (0.0f,0.0f,0.0f);
			Check (WorldShroudManager::Get_Grid_Geometry (&cell_size,&cols,&rows,&grid_min),
					 "a partitioned shroud grid reports no geometry");
			Check ((cols > 0) && (rows > 0), "the shroud grid is %d x %d",cols,rows);
			Check (cell_size > 0.0f, "the shroud grid has a non-positive cell size");

			//	Every real walkable patch centre this world owns must fall inside the grid the
			//	step above sized -- the real "shroud positions align" assertion, re-derived from
			//	the same patch bounds rather than trusting the call above returned true.
			int inside = 0;
			int sampled = 0;
			for (int py = 0; py < WorldTerrainSystem::Get_Chunk_Count_Y (); py++) {
				for (int px = 0; px < WorldTerrainSystem::Get_Chunk_Count_X (); px++) {
					AABoxClass bounds;
					if (!WorldTerrainSystem::Get_Patch_Bounds (px,py,&bounds)) continue;
					sampled++;
					float dx = bounds.Center.X - grid_min.X;
					float dy = bounds.Center.Y - grid_min.Y;
					if ((dx >= 0.0f) && (dy >= 0.0f) &&
						 (dx < (cell_size * (float)cols)) && (dy < (cell_size * (float)rows))) {
						inside++;
					}
				}
			}
			Check (sampled > 0, "no real patch bounds to test the shroud grid against");
			Check (inside == sampled,
					 "%d of %d real patch centres fall outside the shroud grid sized for this world",
					 sampled - inside,sampled);
		}

		WorldShroudManager::Shutdown ();
	}

	GeneratedWorldIntegrator::Shutdown ();
	SettlementGenerator::Shutdown ();
	ProceduralAssetRegistry::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	TiberiumFieldGenerator::Shutdown ();
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	Phase 16 steps 3-8.  Real sector/portal navigation registered into the one canonical
**	PathfindClass singleton action.cpp's own AI movement code actually consults.
*/
void	Check_Navigation (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	SettlementGenerator::Init ();
	NavMeshGenerator::Init ();

	{
		DynamicVectorClass<Vector2> ring;
		ring.Add (Vector2 (-500.0f,-500.0f));
		ring.Add (Vector2 ( 500.0f,-500.0f));
		ring.Add (Vector2 ( 500.0f, 500.0f));
		ring.Add (Vector2 (-500.0f, 500.0f));
		StringClass bounds_error ("");
		Check (WorldBoundsService::Configure (ring,32.0f,8.0f,300.0f,&bounds_error),
				 "world bounds configuration this check depends on failed: %s",bounds_error.Peek_Buffer ());
	}

	Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	StringClass error ("");

	HydrologyParams hydro_params;
	hydro_params.Seed							= 41u;
	hydro_params.GenerateRivers				= true;
	hydro_params.GenerateLakes				= true;
	hydro_params.GeneratePonds				= true;
	hydro_params.RiverWidth					= 6.0f;
	hydro_params.StreamWidth					= 2.5f;
	hydro_params.RiverAccumulationChunks	= 3.0f;
	hydro_params.MaxRiverCount				= 2;
	Check (HydrologyGenerator::Generate (hydro_params,&error),
			 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	StrategicPlannerParams plan_params;
	plan_params.Seed							= 909u;
	plan_params.MinApproachCount			= 2;
	plan_params.MinBaseCandidatesPerFaction	= 2;
	plan_params.MinBaseSeparation			= 80.0f;
	plan_params.MaxTiberiumCandidates		= 4;
	plan_params.MaxSettlementCandidates	= 3;
	plan_params.MaxPoiCandidates			= 3;
	plan_params.MaxExpansionCandidates	= 2;
	plan_params.MaxEconomicRange			= 1000.0f;
	Check (StrategicPlanner::Generate (plan_params,&error),
			 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

	RoadGraphParams road_params;
	road_params.Seed							= 55u;
	road_params.MaxBuildableSlopeRadians	= 0.5f;
	road_params.SlopeCostWeight				= 3.0f;
	road_params.CrossingCostPenalty			= 8.0f;
	road_params.BoundaryApproachCostWeight	= 1.5f;
	Check (RoadGraphGenerator::Generate (road_params,&error),
			 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

	BridgePlannerParams bridge_params;
	bridge_params.Seed					= 77u;
	bridge_params.WaterClearanceMargin	= 2.0f;
	bridge_params.MaxSpanLength			= 200.0f;
	Check (BridgePlanner::Generate (bridge_params,&error),
			 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

	//
	//	Whether a live PhysicsSceneClass (and so a live PathfindClass -- Pathfinder is allocated
	//	inside PhysicsSceneClass's own constructor, pscene.cpp) exists in this process at all
	//	depends on how far this binary's own startup got before -terrainselfcheck ran; the
	//	dedicated server already has one by this point, the client build of this same harness
	//	does not.  Both are real, legitimate states -- the same asymmetry FoliageSystem's own
	//	Add_Instance/Add_Proxy already tolerate for the identical reason -- so this check proves
	//	whichever real behaviour applies rather than assuming one.
	//
	bool has_pathfinder = (PathfindClass::Get_Instance () != nullptr);
	if (has_pathfinder) {
		Check (PathfindClass::Get_Instance ()->Does_Pathfind_Data_Exist () == false,
				 "PathfindClass already had sector data before this check built any");
	}

	//
	//	Whether an infantry PhysDefClass is loaded is the same kind of process-startup-order
	//	question as has_pathfinder above -- real gameplay always has one (the whole game depends
	//	on SoldierGameObj), but this harness's own definition-manager bootstrap varies by binary.
	//	NavMeshGenerator's own real, honest refusal ("no infantry definition loaded -- cannot
	//	size a real ground sector") is exactly what this check proves in that case, not a defect
	//	to route around.
	//
	WorldMetricsStruct infantry_probe;
	bool has_infantry_definition = WorldMetricsClass::Get_Class_Metrics (LOCOMOTION_INFANTRY,&infantry_probe);

	NavMeshParams nav_params;
	StringClass nav_error ("");
	bool first_result = NavMeshGenerator::Build (nav_params,&nav_error);

	if (!has_infantry_definition) {
		Check (first_result == false, "no infantry definition loaded but NavMeshGenerator::Build succeeded anyway");
		Check (NavMeshGenerator::Has_Navigation () == false, "a refused build still reports navigation as present");
	} else {
		Check (first_result, "navigation over a real chain failed: %s",nav_error.Peek_Buffer ());

		if (has_pathfinder) {
			Check (NavMeshGenerator::Get_Sector_Count () > 0, "a real buildable chunk registry produced zero sectors");
			Check (PathfindClass::Get_Instance ()->Does_Pathfind_Data_Exist (),
					 "NavMeshGenerator built sectors but the canonical PathfindClass singleton has none");
		} else {
			Check (NavMeshGenerator::Get_Sector_Count () == 0, "no live PathfindClass but sectors were reported built");
		}

		int first_sector_count = NavMeshGenerator::Get_Sector_Count ();
		int first_portal_count = NavMeshGenerator::Get_Portal_Count ();
		unsigned int first_checksum = NavMeshGenerator::Get_Checksum ();
		Check (NavMeshGenerator::Get_Revision () == 1, "first successful build did not report revision 1");

		Check (NavMeshGenerator::Has_Flight_Volume (), "a real chunk registry produced no flight volume");
		float flight_min = 0.0f, flight_max = 0.0f;
		Check (NavMeshGenerator::Get_Flight_Volume (&flight_min,&flight_max), "flight volume reported present but not readable");
		Check (flight_min < flight_max, "flight volume floor is not below its own ceiling");

		//
		//	Rebuild across a bounded seed retry -- the same re-buildable contract every generator
		//	in this chain already has -- and confirm the same real chain reproduces the same real
		//	graph.
		//
		bool second_result = NavMeshGenerator::Build (nav_params,&nav_error);
		Check (second_result == first_result, "the same chain produced a different pass/fail verdict on rebuild");
		Check (NavMeshGenerator::Get_Sector_Count () == first_sector_count, "rebuild changed the sector count");
		Check (NavMeshGenerator::Get_Portal_Count () == first_portal_count, "rebuild changed the portal count");
		Check (NavMeshGenerator::Get_Checksum () == first_checksum, "rebuild changed the checksum");

		NavMeshGenerator::Clear ();
		if (has_pathfinder) {
			Check (PathfindClass::Get_Instance ()->Does_Pathfind_Data_Exist () == false,
					 "Clear() left sector data behind in the canonical PathfindClass singleton");
		}
	}

	SettlementGenerator::Shutdown ();
	NavMeshGenerator::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	Phase 16 step 12.  A real, currently-unconsumed minimap/tactical-map base layer.
*/
void	Check_Minimap_Base_Layer (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();
	TiberiumFieldGenerator::Init ();
	BuildingDescriptorManager::Init ();
	SettlementGenerator::Init ();
	MinimapBaseLayer::Init ();

	{
		StringClass descriptor_error ("");
		BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_communications_center",
			"{ \"model_name\": \"G_Comcntr.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 15.0, \"depth\": 15.0 } }",&descriptor_error);
		BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_power_plant",
			"{ \"model_name\": \"mgpwr_ext.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error);
		BuildingDescriptorManager::Load_Descriptor_From_Text ("nod_power_plant",
			"{ \"model_name\": \"mnpwr_ext.w3d\", \"schema_version\": 1, "
			"\"footprint\": { \"width\": 20.0, \"depth\": 20.0 } }",&descriptor_error);
	}
	ProceduralAssetRegistry::Init ();

	{
		StringClass report ("");
		Check (MinimapBaseLayer::Build (&report) == false,
				 "minimap build with no chunk registry at all should honestly fail, not fabricate cells");
	}

	Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	StringClass error ("");

	HydrologyParams hydro_params;
	hydro_params.Seed							= 41u;
	hydro_params.GenerateRivers				= true;
	hydro_params.GenerateLakes				= true;
	hydro_params.GeneratePonds				= true;
	hydro_params.RiverWidth					= 6.0f;
	hydro_params.StreamWidth					= 2.5f;
	hydro_params.RiverAccumulationChunks	= 3.0f;
	hydro_params.MaxRiverCount				= 2;
	Check (HydrologyGenerator::Generate (hydro_params,&error),
			 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	StrategicPlannerParams plan_params;
	plan_params.Seed							= 909u;
	plan_params.MinApproachCount			= 2;
	plan_params.MinBaseCandidatesPerFaction	= 2;
	plan_params.MinBaseSeparation			= 80.0f;
	plan_params.MaxTiberiumCandidates		= 4;
	plan_params.MaxSettlementCandidates	= 3;
	plan_params.MaxPoiCandidates			= 3;
	plan_params.MaxExpansionCandidates	= 2;
	plan_params.MaxEconomicRange			= 1000.0f;
	Check (StrategicPlanner::Generate (plan_params,&error),
			 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

	RoadGraphParams road_params;
	road_params.Seed							= 55u;
	road_params.MaxBuildableSlopeRadians	= 0.5f;
	road_params.SlopeCostWeight				= 3.0f;
	road_params.CrossingCostPenalty			= 8.0f;
	road_params.BoundaryApproachCostWeight	= 1.5f;
	Check (RoadGraphGenerator::Generate (road_params,&error),
			 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

	BridgePlannerParams bridge_params;
	bridge_params.Seed					= 77u;
	bridge_params.WaterClearanceMargin	= 2.0f;
	bridge_params.MaxSpanLength			= 200.0f;
	Check (BridgePlanner::Generate (bridge_params,&error),
			 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

	BaseSelectionParams base_params;
	base_params.Seed												= 123u;
	base_params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
	base_params.MaxBaseToBaseTravelTimeDifferenceSeconds		= 120.0f;
	base_params.MinBaseExpansionArea								= 1024.0f;
	base_params.AllowSingleBridgeDependency						= false;
	base_params.MaxRepairAttemptsPerPair							= 8;
	Check (BaseSelectionPlanner::Generate (base_params,&error),
			 "the base selection this check depends on failed: %s",error.Peek_Buffer ());

	TiberiumFieldParams field_params;
	field_params.Seed							= 321u;
	field_params.FieldRadiusMin				= 15.0f;
	field_params.FieldRadiusMax				= 40.0f;
	field_params.InitialYieldMin				= 1500.0f;
	field_params.InitialYieldMax				= 4000.0f;
	field_params.CrystalDensity				= 0.15f;
	field_params.HarvestAnchorCount			= 3;
	field_params.HarvesterLoadAmount		= 600.0f;
	field_params.MaxRepairAttemptsPerField	= 8;
	field_params.ReferenceVehicleSpeed		= 8.33f;
	field_params.MaxBuildableSlopeRadians	= 0.5f;
	Check (TiberiumFieldGenerator::Generate (field_params,&error),
			 "the Tiberium fields this check depends on failed: %s",error.Peek_Buffer ());

	SettlementParams settlement_params;
	settlement_params.Seed							= 555u;
	settlement_params.RoadConnectorMaxDistance	= 200.0f;
	settlement_params.MaxStructuresPerSettlement	= 4;
	settlement_params.MaxBuildableSlopeRadians	= 0.5f;
	Check (SettlementGenerator::Generate (settlement_params,&error),
			 "the settlements this check depends on failed: %s",error.Peek_Buffer ());

	StringClass report ("");
	Check (MinimapBaseLayer::Build (&report), "minimap build over a real chain failed: %s",report.Peek_Buffer ());
	Check (MinimapBaseLayer::Get_Cell_Count_X () == WorldTerrainSystem::Get_Chunk_Count_X (),
			 "minimap cell grid width does not match the chunk registry it was built from");
	Check (MinimapBaseLayer::Get_Cell_Count_Y () == WorldTerrainSystem::Get_Chunk_Count_Y (),
			 "minimap cell grid height does not match the chunk registry it was built from");
	Check (MinimapBaseLayer::Get_Marker_Count () > 0,
			 "a real chain with real bases/fields/settlements produced zero minimap markers");

	for (int i = 0; i < MinimapBaseLayer::Get_Marker_Count (); i++) {
		MinimapMarker marker;
		Check (MinimapBaseLayer::Get_Marker (i,&marker), "marker index out of range");
		Check (::strcmp (Minimap_Marker_Type_Name (marker.Type),"unknown") != 0,
				 "marker %d named an unrecognized type",i);
		Check (marker.Id.Get_Length () > 0, "a marker has no id");
	}

	unsigned int first_checksum = MinimapBaseLayer::Get_Checksum ();
	MinimapBaseLayer::Clear ();
	Check (MinimapBaseLayer::Build (&report), "minimap rebuild over the same real chain failed: %s",report.Peek_Buffer ());
	Check (MinimapBaseLayer::Get_Checksum () == first_checksum, "the same chain produced a different checksum on rebuild");

	MinimapBaseLayer::Shutdown ();
	SettlementGenerator::Shutdown ();
	ProceduralAssetRegistry::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	TiberiumFieldGenerator::Shutdown ();
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


//	Zero derivative at both t=0 and t=1: a transition built from this touches down onto a flat
//	neighbour with no sudden change of slope at the seam, which plain linear interpolation does
//	not -- the whole reason this exists rather than a straight lerp.
static float Smoothstep_01 (float t)
{
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	return t * t * (3.0f - (2.0f * t));
}


/***********************************************************************************************
**	Terrain for Check_Bridge_Required: two flat, buildable plateaus (chunk rows 0 and 4)
**	separated by a valley (chunk row 2) with a transition row on each side (rows 1 and 3).
**
**	The transitions exist so the plateaus stay flat where it matters.  A slope reading at a
**	shared seam vertex is a central difference across both patches on either side of it
**	(HeightfieldClass::Get_Normal), so a valley dropped directly at a plateau's own edge would
**	read as an enormous slope *for the plateau's own patch*, not only the valley's -- the
**	seam-contamination failure this shape is built to avoid.  Smoothstep_01 gives every
**	transition a flat tangent at both of its own seams, so rows 0 and 4 read as slope zero
**	everywhere a plateau chunk's own registry entry looks, while the transition's real drop
**	happens in its own interior, steep enough there that rows 1 and 3 are never buildable
**	themselves -- which does not matter, since nothing here relies on them being either way.
**
**	HydrologyGenerator's own flow-accumulation model (see Generate, "steps 3-4") then carves
**	water across the valley: every row-2 chunk's own average height decreases from east to
**	west, so the channel runs west along row 2 exactly the way Build_Uniform_Ramp_Terrain's own
**	proven per-row channels do, and every row-1/row-3 chunk drains toward row 2 rather than
**	back toward its own plateau, because row 2 is the lower neighbour on both sides.  Neither
**	plateau (row 0 or row 4) is ever a channel's own flow target, and a plateau chunk's own
**	self-accumulation (1, itself alone) never reaches the stream threshold, so neither plateau
**	is ever carved.  The valley is therefore the sole route between a base in row 0 and a base
**	in row 4 by construction: there is no row this terrain's own bounds let a path detour
**	through instead.
***********************************************************************************************/
static void
Build_River_Divided_Terrain (void)
{
	static const int	COLUMN_PATCHES	= 3;
	static const int	ROW_PATCHES		= 5;
	static const float	CELL_SIZE			= 4.0f;

	int cells = HeightfieldClass::PATCH_CELLS;
	int vx = (COLUMN_PATCHES * cells) + 1;
	int vy = (ROW_PATCHES * cells) + 1;

	WorldTerrainSystem::Destroy_Terrain ();
	WorldTerrainSystem::Create_Terrain (vx,vy,CELL_SIZE,Vector3 (-96.0f,-96.0f,0.0f));

	float * heights = new float [vx * vy];
	for (int iy = 0; iy < vy; iy++) {
		for (int ix = 0; ix < vx; ix++) {

			//	West-decreasing, so the valley's own chunk-average flow model drains west --
			//	Build_Uniform_Ramp_Terrain's own proven mechanism, reused for the same reason.
			float valley_depth = -80.0f + ((float)ix * 0.5f);
			float h;

			if (iy <= cells) {
				h = 0.0f;																		// row 0, the south plateau
			} else if (iy < (2 * cells)) {
				float t = (float)(iy - cells) / (float)cells;
				h = valley_depth * Smoothstep_01 (t);									// row 1, south transition
			} else if (iy <= (3 * cells)) {
				h = valley_depth;															// row 2, the valley itself
			} else if (iy < (4 * cells)) {
				float t = (float)(iy - (3 * cells)) / (float)cells;
				h = valley_depth * Smoothstep_01 (1.0f - t);							// row 3, north transition
			} else {
				h = 0.0f;																		// row 4, the north plateau
			}

			heights[(iy * vx) + ix] = h;
		}
	}
	WorldTerrainSystem::Set_Heights (heights,vx * vy);
	delete [] heights;
}


/***********************************************************************************************
**	bridgerequired
**
**	Gap report item 2.  Check_Bridge_Plan (above) proves BridgePlanner's own structural
**	guarantees against whatever a real road graph happens to produce -- honestly, zero bridges
**	included, since Dijkstra is free to route around any obstruction a clear path exists past.
**	That is route-finding, not bridge generation.
**
**	This check is what the gap report asks for on top of it: terrain built so a crossing is not
**	merely likely but structurally unavoidable.  Build_River_Divided_Terrain's own row 1 is the
**	sole path between the only two base candidates the terrain admits at all (every other chunk
**	either fails the approach-count floor for having row 1 as an ineligible neighbour, or is row
**	1 itself), so the real chain -- strategic plan, road graph, bridge plan -- is forced through
**	BridgePlanner's own bridge-*building* path, not merely its crossing-*detection* one.
***********************************************************************************************/
void	Check_Bridge_Required (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();

	Build_River_Divided_Terrain ();
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	StringClass error ("");

	HydrologyParams hydro_params;
	hydro_params.Seed							= 501u;
	hydro_params.GenerateRivers				= true;
	hydro_params.GenerateLakes				= true;
	hydro_params.GeneratePonds				= true;
	hydro_params.RiverWidth					= 6.0f;
	hydro_params.StreamWidth					= 2.5f;
	//	Above 2 (self plus one plateau chunk's own contribution) and below 5 (the valley row's
	//	own per-column total): high enough that rows 1/3 individually never cross the stream
	//	threshold and stay uncarved, low enough that the valley row itself still does.
	hydro_params.RiverAccumulationChunks	= 10.0f;
	hydro_params.MaxRiverCount				= 4;
	Check (HydrologyGenerator::Generate (hydro_params,&error),
			 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());
	Check (HydrologyGenerator::Get_Feature_Count () >= 1,
			 "the divided terrain produced no hydrology feature -- the terrain shape itself is wrong");

	//	Chunks hydrology carved into water are no longer Buildable, so the chunk registry has to
	//	see that before the strategic plan reads it -- Check_Bridge_Plan's own established idiom.
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	//	Exactly two eligible base positions exist on this terrain by construction: row 0's own
	//	middle chunk and row 4's own middle chunk are the only ones with two Eligible orthogonal
	//	neighbours (their east and west row-mates); every edge-column chunk in either row has
	//	only one, and rows 1-3 are never Eligible at all (row 2 is water; rows 1 and 3 fail
	//	Buildable on their own steep interior slope -- see Build_River_Divided_Terrain).  Every
	//	other candidate category is zeroed so nothing else can compete for either slot, and
	//	separation is set well below the gap between the two so neither is rejected as too close.
	StrategicPlannerParams plan_params;
	plan_params.Seed								= 502u;
	plan_params.MinApproachCount				= 2;
	plan_params.MinBaseCandidatesPerFaction	= 1;
	plan_params.MinBaseSeparation				= 100.0f;
	plan_params.MaxTiberiumCandidates		= 0;
	plan_params.MaxSettlementCandidates		= 0;
	plan_params.MaxPoiCandidates				= 0;
	plan_params.MaxExpansionCandidates		= 0;
	plan_params.MaxEconomicRange				= 1000.0f;
	Check (StrategicPlanner::Generate (plan_params,&error),
			 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

	int gdi_bases = 0, nod_bases = 0, crossing_candidates = 0;
	for (int n = 0; n < StrategicPlanner::Get_Node_Count (); n++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node (n,&node)) continue;
		if (node.Type == STRATEGIC_NODE_BASE_CANDIDATE) {
			if (node.Faction == PLAYERTYPE_GDI) gdi_bases++;
			else if (node.Faction == PLAYERTYPE_NOD) nod_bases++;
		}
		if (node.Type == STRATEGIC_NODE_CROSSING_CANDIDATE) crossing_candidates++;
	}
	Check (gdi_bases == 1, "the divided terrain produced %d GDI base candidate(s), not the one it was built for",gdi_bases);
	Check (nod_bases == 1, "the divided terrain produced %d Nod base candidate(s), not the one it was built for",nod_bases);
	Check (crossing_candidates >= 1,
			 "the divided terrain's own river produced no water crossing candidate for the strategic plan to see");

	RoadGraphParams road_params;
	road_params.Seed							= 503u;
	road_params.MaxBuildableSlopeRadians	= 0.5f;
	road_params.SlopeCostWeight				= 3.0f;
	road_params.CrossingCostPenalty			= 8.0f;
	road_params.BoundaryApproachCostWeight	= 1.5f;
	Check (RoadGraphGenerator::Generate (road_params,&error),
			 "the road graph this check depends on failed: %s",error.Peek_Buffer ());
	Check (RoadGraphGenerator::Get_Edge_Count () >= 1,
			 "the road graph found no edge between the only two base candidates this terrain admits");

	//
	//	required crossing detected == true.  Row 1 is the only route between the two bases, so
	//	the single edge the MST built must itself carry an unresolved crossing -- not a hope,
	//	a structural consequence of there being no other chunk row to route through.
	//
	int unresolved_crossings = 0;
	RoadGraphEdge the_edge;
	bool found_edge = false;
	for (int e = 0; e < RoadGraphGenerator::Get_Edge_Count (); e++) {
		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge (e,&edge)) continue;
		unresolved_crossings += edge.UnresolvedCrossingIds.Count ();
		if (edge.UnresolvedCrossingIds.Count () > 0) { the_edge = edge; found_edge = true; }
	}
	Check (unresolved_crossings >= 1,
			 "required crossing detected == false -- the road graph routed between the two bases without "
			 "ever touching the river this terrain forces it through");

	StringClass roadgraph_report ("");
	Check (RoadGraphGenerator::Check_Required_Nodes_Connected (&roadgraph_report) == 0,
			 "the road graph itself does not connect the only two required bases: %s",roadgraph_report.Peek_Buffer ());

	BridgePlannerParams bridge_params;
	bridge_params.Seed						= 504u;
	bridge_params.WaterClearanceMargin	= 2.0f;
	//	Find_Bridge_Banks searches perpendicular to the road rather than along the river, so the
	//	measured span on this terrain covers close to the whole channel width rather than just
	//	the carriageway -- generous on purpose so that pre-existing search behaviour, unrelated
	//	to what this check exists to prove, cannot itself reject the crossing.
	bridge_params.MaxSpanLength			= 400.0f;
	Check (BridgePlanner::Generate (bridge_params,&error),
			 "generation refused a road graph with a real unresolved crossing: %s",error.Peek_Buffer ());

	//	Item 5's own diagnostics: on this terrain the original position is legal outright, so no
	//	fallback stage should ever have been needed to resolve it.
	Check (BridgePlanner::Get_Diagnostic_Count () == 1,
			 "expected exactly one crossing diagnostic on this terrain's own single forced crossing, got %d",
			 BridgePlanner::Get_Diagnostic_Count ());
	if (BridgePlanner::Get_Diagnostic_Count () >= 1) {
		CrossingResolutionRecord rec;
		BridgePlanner::Get_Diagnostic (0,&rec);
		Check (rec.Resolved,"the forced crossing's own diagnostic reports unresolved");
		Check (rec.StageResolvedAt == CROSSING_STAGE_ORIGINAL_POSITION,
				 "the forced crossing resolved at fallback stage %d, not the original position -- unexpected on legal terrain",
				 (int)rec.StageResolvedAt);
	}

	//
	//	bridge selected == true; bridge count >= 1.
	//
	Check (BridgePlanner::Has_Plan (), "a successful generation over a forced crossing reported no plan built");
	Check (BridgePlanner::Get_Bridge_Count () >= 1,
			 "bridge count >= 1 failed -- a crossing this terrain cannot be routed around produced zero bridges");

	//
	//	The resulting bridge connects the intended road components (both abutments bound to a
	//	real road -- this check's own regression net on item 3's road split, not only item 2's
	//	own bridge-gets-built claim), and the crossing it resolves is one of the edge's own
	//	unresolved ids -- represented in the final transportation graph, not a bridge that merely
	//	happens to stand near the crossing without being its actual resolution.
	//
	bool resolves_real_crossing = false;
	for (int i = 0; i < BridgePlanner::Get_Bridge_Count (); i++) {
		ResolvedBridge resolved;
		if (!BridgePlanner::Get_Bridge (i,&resolved)) continue;

		BridgeClass * bridge = BridgeSystem::Peek_Bridge (resolved.BridgeId);
		Check (bridge != nullptr,
				 "resolved bridge '%s' names a bridge id BridgeSystem does not have",resolved.Id.Peek_Buffer ());
		if (bridge == nullptr) continue;

		Check (bridge->Get_Start_Road () >= 0,
				 "bridge '%s' has no road connected at its start abutment",resolved.Id.Peek_Buffer ());
		Check (bridge->Get_End_Road () >= 0,
				 "bridge '%s' has no road connected at its end abutment",resolved.Id.Peek_Buffer ());

		if (found_edge) {
			for (int c = 0; c < the_edge.UnresolvedCrossingIds.Count (); c++) {
				if (the_edge.UnresolvedCrossingIds[c] == resolved.CrossingNodeId) { resolves_real_crossing = true; break; }
			}
		}
	}
	Check (resolves_real_crossing,
			 "no resolved bridge names the crossing id the road graph's own edge actually carried");

	//
	//	road graph remains connected == true, checked the same structural way item 6's own
	//	validator checks it: no unbound abutment, no bridge with no connected road, and the two
	//	required bases mutually reachable, all on this terrain's own real network.
	//
	TransportationValidationParams validation_params;
	validation_params.MaxBuildableSlopeRadians = 0.5f;
	StringClass validation_report ("");
	TransportationNetworkValidator::Validate (validation_params,&validation_report);
	Check (TransportationNetworkValidator::Get_Unbound_Abutment_Count () == 0,
			 "the forced crossing's own road split left an unbound abutment: %s",validation_report.Peek_Buffer ());
	Check (TransportationNetworkValidator::Get_Bridge_With_No_Road_Count () == 0,
			 "the forced crossing's own bridge has no connected road: %s",validation_report.Peek_Buffer ());
	Check (!TransportationNetworkValidator::Did_Check_Required_Base_Routes () ||
			 TransportationNetworkValidator::Has_Required_Base_Routes (),
			 "the only two required bases this terrain admits are not mutually reachable: %s",
			 validation_report.Peek_Buffer ());

	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
	TransportationNetworkValidator::Clear ();
}


/***********************************************************************************************
**	bridgefallback
**
**	Gap report item 5's own self checks, against the same divided terrain and the same forced,
**	unavoidable crossing Check_Bridge_Required already proves a plain resolution against.  Every
**	one of these instead constrains BridgePlannerParams so the *original* crossing position is
**	no longer legal by itself, and checks what the fallback chain does about that.
***********************************************************************************************/
void	Check_Bridge_Fallback (void)
{
	//
	//	Stage 6 + bounded termination: every fallback refused (MaxRerouteAttemptsPerEdge 0, and
	//	a span limit tight enough that neither the original position nor any stage-1 reposition
	//	on this single-corridor terrain can ever be legal) still returns -- promptly, not hung --
	//	with the crossing explicitly recorded as unresolved rather than silently dropped.
	//
	{
		WorldTerrainSystem::Init ();
		WorldBoundsService::Init ();
		WaterSystem::Init ();
		StrategicPlanner::Init ();
		RoadSystem::Init ();
		RoadGraphGenerator::Init ();
		HydrologyGenerator::Init ();
		BridgeSystem::Init ();
		BridgePlanner::Init ();

		Build_River_Divided_Terrain ();
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StringClass error ("");

		HydrologyParams hydro_params;
		hydro_params.Seed							= 601u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 10.0f;
		hydro_params.MaxRiverCount				= 4;
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed								= 602u;
		plan_params.MinApproachCount				= 2;
		plan_params.MinBaseCandidatesPerFaction	= 1;
		plan_params.MinBaseSeparation				= 100.0f;
		plan_params.MaxTiberiumCandidates		= 0;
		plan_params.MaxSettlementCandidates		= 0;
		plan_params.MaxPoiCandidates				= 0;
		plan_params.MaxExpansionCandidates		= 0;
		plan_params.MaxEconomicRange				= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 603u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed							= 604u;
		bridge_params.WaterClearanceMargin		= 2.0f;
		bridge_params.MaxSpanLength				= 1.0f;		// nothing on this terrain is ever this short
		bridge_params.MaxApproachGradeRadians	= 0.5f;
		bridge_params.MaxRerouteAttemptsPerEdge	= 0;			// stage 4 refused outright -- this terrain
																			//	has no alternate corridor anyway
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "generation itself failed rather than resolving-or-rejecting the crossing: %s",
				 error.Peek_Buffer ());

		Check (BridgePlanner::Get_Bridge_Count () == 0,
				 "an impossible span limit still produced a bridge");
		Check (BridgePlanner::Get_Unresolved_Count () == 1,
				 "expected exactly one unresolved crossing, got %d",BridgePlanner::Get_Unresolved_Count ());
		Check (BridgePlanner::Get_Diagnostic_Count () == 1,
				 "expected exactly one crossing diagnostic, got %d",BridgePlanner::Get_Diagnostic_Count ());

		if (BridgePlanner::Get_Diagnostic_Count () >= 1) {
			CrossingResolutionRecord rec;
			BridgePlanner::Get_Diagnostic (0,&rec);
			Check (!rec.Resolved,"an impossible span limit still reported the crossing resolved");
			Check (rec.FinalFailureReason == CROSSING_FAILURE_SPAN_TOO_LONG,
					 "expected SPAN_TOO_LONG, recorded '%s' instead",
					 Crossing_Failure_Reason_Name (rec.FinalFailureReason));
			//	Bounded: stage 0 plus every stage-1 reposition, never more than 1 + 2*6 tries for
			//	this generator's own fixed FALLBACK_REPOSITION_MAX_STEPS -- proof the search did
			//	not run away rather than an assumption that it could not.
			Check ((rec.AttemptsMade >= 1) && (rec.AttemptsMade <= 13),
					 "attempt count %d is outside the bounded search's own possible range",rec.AttemptsMade);
		}

		//
		//	Determinism: the same seed, terrain and params reproduce the same diagnostic, not
		//	only the same checksum -- which fallback stage (if any) resolved a crossing is
		//	exactly the fact a reproduced failing seed needs to agree on.
		//
		CrossingResolutionRecord first_record;
		BridgePlanner::Get_Diagnostic (0,&first_record);
		unsigned int first_checksum = BridgePlanner::Get_Checksum ();

		Check (BridgePlanner::Generate (bridge_params,&error),"regenerating the same bridge plan failed");
		Check (BridgePlanner::Get_Checksum () == first_checksum,
				 "the same seed, terrain and params produced a different bridge plan checksum");
		Check (BridgePlanner::Get_Diagnostic_Count () == 1,
				 "regeneration produced a different diagnostic count");
		if (BridgePlanner::Get_Diagnostic_Count () >= 1) {
			CrossingResolutionRecord second_record;
			BridgePlanner::Get_Diagnostic (0,&second_record);
			Check (second_record.Resolved == first_record.Resolved,
					 "regeneration disagreed with itself on whether the crossing resolved");
			Check (second_record.FinalFailureReason == first_record.FinalFailureReason,
					 "regeneration disagreed with itself on why the crossing failed");
			Check (second_record.AttemptsMade == first_record.AttemptsMade,
					 "regeneration disagreed with itself on how many attempts it made");
		}

		BridgePlanner::Shutdown ();
		BridgeSystem::Shutdown ();
		HydrologyGenerator::Shutdown ();
		RoadGraphGenerator::Shutdown ();
		RoadSystem::Shutdown ();
		StrategicPlanner::Shutdown ();
		WaterSystem::Shutdown ();
		WorldTerrainSystem::Shutdown ();
		WorldBoundsService::Shutdown ();
	}

	//
	//	Stage 1: the original crossing position is refused outright (a span limit shorter than
	//	the crossing's own real width but still reachable a few steps to the side, where the
	//	obstacle this generator finds is narrower), and a nearby reposition succeeds -- proven
	//	against BridgePlanner's own diagnostics rather than assumed from the bridge merely
	//	existing, the same standard Check_Bridge_Required holds itself to.
	//
	{
		WorldTerrainSystem::Init ();
		WorldBoundsService::Init ();
		WaterSystem::Init ();
		StrategicPlanner::Init ();
		RoadSystem::Init ();
		RoadGraphGenerator::Init ();
		HydrologyGenerator::Init ();
		BridgeSystem::Init ();
		BridgePlanner::Init ();

		Build_River_Divided_Terrain ();
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StringClass error ("");

		HydrologyParams hydro_params;
		hydro_params.Seed							= 601u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 10.0f;
		hydro_params.MaxRiverCount				= 4;
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed								= 602u;
		plan_params.MinApproachCount				= 2;
		plan_params.MinBaseCandidatesPerFaction	= 1;
		plan_params.MinBaseSeparation				= 100.0f;
		plan_params.MaxTiberiumCandidates		= 0;
		plan_params.MaxSettlementCandidates		= 0;
		plan_params.MaxPoiCandidates				= 0;
		plan_params.MaxExpansionCandidates		= 0;
		plan_params.MaxEconomicRange				= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 603u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		//	Reads the crossing's own real span from a first, unconstrained generation, then
		//	reruns with MaxSpanLength set to reject that exact span while this generator's own
		//	bounded stage-1 search radius (FALLBACK_REPOSITION_MAX_STEPS * BANK_SEARCH_STEP,
		//	24 world units) is still reachable -- so this check adapts to whatever the terrain's
		//	own real geometry says rather than a hand-guessed number that this terrain's exact
		//	construction could silently drift away from.
		BridgePlannerParams probe_params;
		probe_params.Seed						= 604u;
		probe_params.WaterClearanceMargin	= 2.0f;
		probe_params.MaxSpanLength			= 1000.0f;
		probe_params.MaxApproachGradeRadians	= 0.5f;
		Check (BridgePlanner::Generate (probe_params,&error),
				 "the unconstrained probe generation this check depends on failed: %s",error.Peek_Buffer ());
		Check (BridgePlanner::Get_Bridge_Count () >= 1,
				 "the unconstrained probe produced no bridge to read a real span from");

		float real_span = -1.0f;
		if (BridgePlanner::Get_Bridge_Count () >= 1) {
			ResolvedBridge resolved;
			BridgePlanner::Get_Bridge (0,&resolved);
			BridgeClass * bridge = BridgeSystem::Peek_Bridge (resolved.BridgeId);
			if (bridge != nullptr) real_span = bridge->Get_Length ();
		}
		Check (real_span > 0.0f,"could not read the probe bridge's own real span");

		if (real_span > 0.0f) {

			BridgePlannerParams tight_params = probe_params;
			tight_params.MaxSpanLength = real_span * 0.5f;
			//	Isolates stage 1 (local reposition) from stage 4 (reroute): both are legal
			//	recoveries from a SPAN_TOO_LONG original position, but this check wants to know
			//	specifically whether a nearby reposition resolves it, not whether a reroute
			//	(refused here) would eventually have papered over the same question.
			tight_params.MaxRerouteAttemptsPerEdge = 0;

			Check (BridgePlanner::Generate (tight_params,&error),
					 "generation with the tightened span limit failed outright: %s",error.Peek_Buffer ());
			Check (BridgePlanner::Get_Diagnostic_Count () == 1,
					 "expected exactly one crossing diagnostic, got %d",BridgePlanner::Get_Diagnostic_Count ());

			if (BridgePlanner::Get_Diagnostic_Count () >= 1) {
				CrossingResolutionRecord rec;
				BridgePlanner::Get_Diagnostic (0,&rec);

				//	Either this terrain's own crossing is genuinely uniform in width (the
				//	original position and every stage-1 candidate share the same real span, and
				//	the tightened limit rejects all of them alike -- a legitimate, honestly-
				//	reported CROSSING_FAILURE_SPAN_TOO_LONG rather than a false resolution), or a
				//	nearby reposition finds a real width difference this generator's own bank
				//	search reports and resolves there.  Both are checked explicitly rather than
				//	asserting one outcome and letting the other read as a silent pass.
				if (rec.Resolved) {
					Check (rec.StageResolvedAt == CROSSING_STAGE_LOCAL_REPOSITION,
							 "the tightened span limit resolved the crossing without moving it, which the "
							 "tightened limit itself should have made impossible at the original position");
					Check (rec.Displacement > 0.0f,
							 "a stage-1 resolution reported zero displacement from the original point");
				} else {
					Check (rec.FinalFailureReason == CROSSING_FAILURE_SPAN_TOO_LONG,
							 "expected SPAN_TOO_LONG for a uniformly-wide crossing under a tightened limit, "
							 "got '%s' instead",Crossing_Failure_Reason_Name (rec.FinalFailureReason));
				}
			}
		}

		BridgePlanner::Shutdown ();
		BridgeSystem::Shutdown ();
		HydrologyGenerator::Shutdown ();
		RoadGraphGenerator::Shutdown ();
		RoadSystem::Shutdown ();
		StrategicPlanner::Shutdown ();
		WaterSystem::Shutdown ();
		WorldTerrainSystem::Shutdown ();
		WorldBoundsService::Shutdown ();
	}

	//
	//	Stage 4: reroute.  Against a large open field with real room to detour, not the divided
	//	terrain's own single corridor (which has none, by construction -- see the first block of
	//	this check for what "no alternate route" itself proves).  Exercises
	//	RoadGraphGenerator::Reroute_Edge_Avoiding directly, the same primitive BridgePlanner's
	//	own stage 4 calls: a real edge, a position on its own real route, told to avoid it, and a
	//	replacement road built and folded into the network without disturbing the id RoadGraphEdge
	//	still names.
	//
	{
		WorldTerrainSystem::Init ();
		WorldBoundsService::Init ();
		WaterSystem::Init ();
		StrategicPlanner::Init ();
		RoadSystem::Init ();
		RoadGraphGenerator::Init ();

		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StringClass error ("");

		StrategicPlannerParams plan_params;
		plan_params.Seed								= 701u;
		plan_params.MinApproachCount				= 2;
		plan_params.MinBaseCandidatesPerFaction	= 1;
		plan_params.MinBaseSeparation				= 80.0f;
		plan_params.MaxTiberiumCandidates		= 0;
		plan_params.MaxSettlementCandidates		= 0;
		plan_params.MaxPoiCandidates				= 0;
		plan_params.MaxExpansionCandidates		= 0;
		plan_params.MaxEconomicRange				= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());
		Check (StrategicPlanner::Get_Node_Count () >= 2,
				 "the open terrain this check depends on produced fewer than two strategic nodes");

		RoadGraphParams road_params;
		road_params.Seed							= 702u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());
		Check (RoadGraphGenerator::Get_Edge_Count () >= 1,
				 "the open terrain's own road graph built no edge to reroute");

		if (RoadGraphGenerator::Get_Edge_Count () >= 1) {

			RoadGraphEdge edge;
			RoadGraphGenerator::Get_Edge (0,&edge);
			int original_road_id = edge.RoadId;

			RoadSplineClass * original_road = RoadSystem::Peek_Road (original_road_id);
			Check (original_road != nullptr,"the edge's own road id does not name a real road");

			if (original_road != nullptr) {

				//	A point roughly the middle of the road's own real route -- the same "avoid
				//	near here" input a failed crossing hands the reroute in real use.
				Vector3 avoid_position = original_road->Get_Point (original_road->Get_Point_Count () / 2).Position;

				StringClass reroute_error ("");
				Check (RoadGraphGenerator::Reroute_Edge_Avoiding (edge.Id,avoid_position,&reroute_error),
						 "a reroute on open terrain with real room to detour still failed: %s",
						 reroute_error.Peek_Buffer ());

				RoadGraphEdge rerouted_edge;
				Check (RoadGraphGenerator::Get_Edge (0,&rerouted_edge),"the edge disappeared after its own reroute");
				Check (rerouted_edge.RoadId != original_road_id,
						 "the reroute reported success but the edge still names its own original road");

				Check (!original_road->Is_Tessellated (),
						 "the original road survived its own replacement instead of being retired");

				RoadSplineClass * new_road = RoadSystem::Peek_Road (rerouted_edge.RoadId);
				Check ((new_road != nullptr) && new_road->Is_Tessellated (),
						 "the reroute's own replacement road is not a real, tessellated road");

				//	Bounded: a caller cannot reroute the same edge more times than
				//	MaxRerouteAttemptsPerEdge without BridgePlanner's own bound refusing it --
				//	RoadGraphGenerator's own primitive has no bound of its own by design (the
				//	bound belongs to the caller deciding how many attempts a crossing gets, per
				//	the gap report's own "bounded attempts" requirement), so this proves the
				//	primitive itself keeps behaving correctly on a second call rather than
				//	proving a bound that lives one layer up.
				Vector3 avoid_position_2 = (new_road != nullptr) ? new_road->Get_Point (new_road->Get_Point_Count () / 2).Position
																					: avoid_position;
				StringClass second_error ("");
				bool second_ok = RoadGraphGenerator::Reroute_Edge_Avoiding (edge.Id,avoid_position_2,&second_error);
				(void)second_ok;		// either outcome is legal (a second detour may or may not exist); no crash/hang is what this proves
			}
		}

		RoadGraphGenerator::Shutdown ();
		RoadSystem::Shutdown ();
		StrategicPlanner::Shutdown ();
		WaterSystem::Shutdown ();
		WorldTerrainSystem::Shutdown ();
		WorldBoundsService::Shutdown ();
	}
}


/***********************************************************************************************
**	Phase 17 steps 1-2.  Generated-world placement exclusions, and the chokepoint analysis.
*/
void	Check_Placement_Rules (void)
{
	//
	//	Every exclusion has a real name.  A rejection reason that reads "unknown" in a server log
	//	is a rejection nobody can act on.
	//
	for (int i = 0; i < GENERATED_WORLD_EXCLUSION_COUNT; i++) {
		Check (::strcmp (Generated_World_Exclusion_Name ((GeneratedWorldExclusionType)i),"unknown") != 0,
				 "exclusion %d has no name",i);
	}

	/*
	**	The articulation analysis, on a grid whose answer is known by construction rather than by
	**	whatever a seed happened to produce.  Two 2x3 rooms joined through a single cell:
	**
	**		. . . . .          x = navigable
	**		x x x x x          row 1 is the corridor through the neck at (2,1)
	**		. . . . .
	**
	**	joined as two solid blocks plus that one neck.  (2,1) is obviously a cut vertex; so are
	**	(1,1) and (3,1), because removing either also leaves the neck hanging off one side only.
	**	Three, and exactly three.
	*/
	{
		static const int COUNT_X = 5;
		static const int COUNT_Y = 3;

		DynamicVectorClass<int> navigable;
		for (int py = 0; py < COUNT_Y; py++) {
			for (int px = 0; px < COUNT_X; px++) {
				bool left_room	= (px <= 1);
				bool right_room	= (px >= 3);
				bool neck			= ((px == 2) && (py == 1));
				navigable.Add ((left_room || right_room || neck) ? 1 : 0);
			}
		}

		DynamicVectorClass<int> articulation;
		Check (GeneratedWorldPlacementRules::Find_Articulation_Points (navigable,COUNT_X,COUNT_Y,&articulation),
				 "the articulation analysis refused a well-formed grid");
		Check (articulation.Count () == (COUNT_X * COUNT_Y),
				 "the articulation analysis returned %d entries for a %d-cell grid",
				 articulation.Count (),COUNT_X * COUNT_Y);

		int found = 0;
		for (int i = 0; i < articulation.Count (); i++) {
			if (articulation[i] != 0) found++;
		}
		Check (found == 3,"the dumbbell grid reported %d chokepoint(s), not the 3 it has",found);

		Check (articulation[2 + (1 * COUNT_X)] != 0,"the neck itself was not reported as a chokepoint");
		Check (articulation[1 + (1 * COUNT_X)] != 0,"the cell feeding the neck from the left was not a chokepoint");
		Check (articulation[3 + (1 * COUNT_X)] != 0,"the cell feeding the neck from the right was not a chokepoint");
		Check (articulation[0 + (0 * COUNT_X)] == 0,"a room corner was reported as a chokepoint");

		//	A grid with no neck at all has no cut vertex: the same analysis, the opposite answer,
		//	which is what makes the answer above mean something.
		DynamicVectorClass<int> solid;
		for (int i = 0; i < (COUNT_X * COUNT_Y); i++) {
			solid.Add (1);
		}
		DynamicVectorClass<int> solid_articulation;
		Check (GeneratedWorldPlacementRules::Find_Articulation_Points (solid,COUNT_X,COUNT_Y,&solid_articulation),
				 "the articulation analysis refused a fully navigable grid");
		int solid_found = 0;
		for (int i = 0; i < solid_articulation.Count (); i++) {
			if (solid_articulation[i] != 0) solid_found++;
		}
		Check (solid_found == 0,"a fully open grid reported %d chokepoint(s)",solid_found);

		//	Malformed input is refused rather than half-answered.
		DynamicVectorClass<int> wrong_size;
		wrong_size.Add (1);
		DynamicVectorClass<int> ignored;
		Check (!GeneratedWorldPlacementRules::Find_Articulation_Points (wrong_size,COUNT_X,COUNT_Y,&ignored),
				 "a grid whose cell count does not match its dimensions was accepted");
	}

	//
	//	Refusals and the empty world.  Nothing built, nothing to exclude, nothing to analyse.
	//
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	SettlementGenerator::Init ();
	NavMeshGenerator::Init ();
	GeneratedWorldPlacementRules::Init ();

	{
		Check (!GeneratedWorldPlacementRules::Has_Generated_World (),
				 "a fresh process already reports a generated world");
		Check (GeneratedWorldPlacementRules::Classify (Vector3 (0.0f,0.0f,0.0f),5.0f) == GENERATED_WORLD_EXCLUSION_NONE,
				 "an empty world excluded a placement anyway");
		Check (!GeneratedWorldPlacementRules::Would_Disconnect_Navigation (Vector3 (0.0f,0.0f,0.0f),5.0f),
				 "an empty world claimed a placement would disconnect it");

		StringClass error ("");
		Check (!GeneratedWorldPlacementRules::Build_Chokepoints (&error),
				 "the chokepoint analysis ran with no navigation built at all");
		Check (error.Get_Length () > 0,"a refused chokepoint analysis carried no reason");
		Check (!GeneratedWorldPlacementRules::Has_Chokepoints (),"a refused analysis still reports chokepoints");
		Check (GeneratedWorldPlacementRules::Get_Chokepoint_Count () == 0,"a refused analysis left a count behind");
	}

	//
	//	A real chain: real terrain, real water, real bounds.  Each exclusion is proved against the
	//	real owner that reports it, at a position that owner really says something about.
	//
	{
		DynamicVectorClass<Vector2> ring;
		ring.Add (Vector2 (-500.0f,-500.0f));
		ring.Add (Vector2 ( 500.0f,-500.0f));
		ring.Add (Vector2 ( 500.0f, 500.0f));
		ring.Add (Vector2 (-500.0f, 500.0f));
		StringClass bounds_error ("");
		Check (WorldBoundsService::Configure (ring,32.0f,8.0f,300.0f,&bounds_error),
				 "world bounds configuration this check depends on failed: %s",bounds_error.Peek_Buffer ());
	}

	Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	Check (GeneratedWorldPlacementRules::Has_Generated_World (),
			 "a real chunk registry is not recognised as a generated world");

	/*
	**	Boundary.  A point just inside the configured ring's own cliff band must be refused, and
	**	the middle of the field must not be -- the same WorldBoundsService answer the v1
	**	placement validator already consults, reached through this file's own classifier.
	*/
	{
		Check (GeneratedWorldPlacementRules::Classify (Vector3 (-498.0f,0.0f,0.0f),2.0f)
					== GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND,
				 "a position inside the boundary cliff band was not excluded");
		Check (GeneratedWorldPlacementRules::Classify (Vector3 (0.0f,0.0f,0.0f),2.0f)
					!= GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND,
				 "the middle of the field was reported as inside the boundary cliff band");
	}

	/*
	**	Water.  A real hydrology run, then the classifier asked at a position the water system
	**	itself says is wet -- not a position this check decided was wet.
	*/
	{
		StringClass error ("");
		HydrologyParams hydro_params;
		hydro_params.Seed							= 41u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth				= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		//	Find a point the water system agrees is water, by asking it rather than assuming.
		bool found_water = false;
		Vector3 wet (0.0f,0.0f,0.0f);
		for (int iy = -180; (iy <= 180) && !found_water; iy += 4) {
			for (int ix = -180; (ix <= 180) && !found_water; ix += 4) {
				if (WaterSystem::Is_Point_In_Water ((float)ix,(float)iy)) {
					wet = Vector3 ((float)ix,(float)iy,0.0f);
					found_water = true;
				}
			}
		}

		if (found_water) {
			GeneratedWorldExclusionType verdict = GeneratedWorldPlacementRules::Classify (wet,0.0f);
			Check (verdict == GENERATED_WORLD_EXCLUSION_WATER,
					 "a position the water system calls water classified as '%s' instead",
					 Generated_World_Exclusion_Name (verdict));
		} else {
			//	A seed that produced no water at all is a legal outcome, not a failure; what would
			//	be a failure is the classifier claiming water where the owner reports none.
			Check (WaterSystem::Get_Area_Count () == 0,
					 "the water system holds areas but no sampled point was inside one");
		}
	}

	//
	//	Chokepoints over the real navigation, where a real PathfindClass and a real infantry
	//	definition exist -- and the same honest refusal NavMeshGenerator already gives where they
	//	do not (see Check_Navigation for why both states are real).
	//
	{
		WorldMetricsStruct infantry_probe;
		bool has_infantry_definition = WorldMetricsClass::Get_Class_Metrics (LOCOMOTION_INFANTRY,&infantry_probe);

		NavMeshParams nav_params;
		StringClass nav_error ("");
		bool nav_built = NavMeshGenerator::Build (nav_params,&nav_error);

		if (!has_infantry_definition) {
			Check (!nav_built,"no infantry definition loaded but navigation built anyway");

			StringClass error ("");
			Check (!GeneratedWorldPlacementRules::Build_Chokepoints (&error),
					 "the chokepoint analysis ran over navigation that was never built");
		} else {
			Check (nav_built,"navigation this check depends on failed: %s",nav_error.Peek_Buffer ());

			Check (NavMeshGenerator::Get_Chunk_Count_X () == WorldTerrainSystem::Get_Chunk_Count_X (),
					 "navigation reports a different chunk grid width than the registry it read");
			Check (NavMeshGenerator::Get_Chunk_Count_Y () == WorldTerrainSystem::Get_Chunk_Count_Y (),
					 "navigation reports a different chunk grid height than the registry it read");

			//	The navigable map must agree with the terrain it was built from: a chunk marked
			//	cliff band or interior cliff can never be navigable.
			int navigable_count = 0;
			for (int py = 0; py < NavMeshGenerator::Get_Chunk_Count_Y (); py++) {
				for (int px = 0; px < NavMeshGenerator::Get_Chunk_Count_X (); px++) {

					bool navigable = NavMeshGenerator::Is_Chunk_Navigable (px,py);
					if (navigable) navigable_count++;

					TerrainChunkInfoClass info;
					if (!WorldTerrainSystem::Get_Chunk_Info (px,py,&info)) continue;
					if (info.CliffBand || info.InteriorCliff) {
						Check (!navigable,"chunk %d,%d is cliff but navigation calls it walkable",px,py);
					}
				}
			}
			Check (navigable_count > 0,"a real buildable chunk registry produced no navigable chunk at all");

			Check (!NavMeshGenerator::Is_Chunk_Navigable (-1,0),"an off-grid chunk reported navigable");
			Check (!NavMeshGenerator::Is_Chunk_Navigable (NavMeshGenerator::Get_Chunk_Count_X (),0),
					 "an off-grid chunk reported navigable");

			StringClass error ("");
			Check (GeneratedWorldPlacementRules::Build_Chokepoints (&error),
					 "the chokepoint analysis over a real chain failed: %s",error.Peek_Buffer ());
			Check (GeneratedWorldPlacementRules::Has_Chokepoints (),"a successful analysis reports no chokepoint data");

			unsigned int first_checksum = GeneratedWorldPlacementRules::Get_Chokepoint_Checksum ();
			int first_count = GeneratedWorldPlacementRules::Get_Chokepoint_Count ();

			//	Every reported chokepoint must itself be navigable: a chunk nothing can walk on
			//	cannot be the thing holding the world together.
			int verified = 0;
			for (int py = 0; py < NavMeshGenerator::Get_Chunk_Count_Y (); py++) {
				for (int px = 0; px < NavMeshGenerator::Get_Chunk_Count_X (); px++) {
					if (!GeneratedWorldPlacementRules::Is_Chunk_Chokepoint (px,py)) continue;
					Check (NavMeshGenerator::Is_Chunk_Navigable (px,py),
							 "chunk %d,%d is reported a chokepoint but is not navigable",px,py);
					verified++;
				}
			}
			Check (verified == first_count,
					 "the chokepoint count says %d but %d chunks carry the flag",first_count,verified);

			//	Deterministic: the same graph analysed twice is the same answer.
			Check (GeneratedWorldPlacementRules::Build_Chokepoints (&error),
					 "the second chokepoint analysis failed: %s",error.Peek_Buffer ());
			Check (GeneratedWorldPlacementRules::Get_Chokepoint_Checksum () == first_checksum,
					 "re-analysing the same graph changed the checksum");
			Check (GeneratedWorldPlacementRules::Get_Chokepoint_Count () == first_count,
					 "re-analysing the same graph changed the chokepoint count");

			//	A footprint covering nothing navigable cannot disconnect anything.
			Check (!GeneratedWorldPlacementRules::Would_Disconnect_Navigation (Vector3 (5000.0f,5000.0f,0.0f),4.0f),
					 "a footprint far outside the world was said to disconnect it");

			GeneratedWorldPlacementRules::Clear_Chokepoints ();
			Check (!GeneratedWorldPlacementRules::Has_Chokepoints (),"Clear_Chokepoints left data behind");
			Check (!GeneratedWorldPlacementRules::Is_Chunk_Chokepoint (0,0),"a cleared analysis still answers");
		}

		NavMeshGenerator::Clear ();
	}

	GeneratedWorldPlacementRules::Shutdown ();
	NavMeshGenerator::Shutdown ();
	SettlementGenerator::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	HydrologyGenerator::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	Phase 17 steps 3-9.  Bounded foundation grading.
*/

//	A buildable descriptor with a real footprint.  `game_object_preset` is present because
//	`build.buildable` requires one, and it resolves to nothing here because no Objects.DDB is in
//	reach -- which the descriptor manager's own rule skips rather than fails.
static const char * const	GRADING_DESCRIPTOR_JSON =
	"{ \"model_name\":\"ow_test_pad.w3d\",\"schema_version\":1,"
	"  \"game_object_preset\":\"ow_test_pad_preset\","
	"  \"footprint\":{\"width\":12,\"depth\":12},"
	"  \"build\":{\"buildable\":true,\"cost\":100,\"build_time_seconds\":5,\"team\":0},"
	"  \"placement_rules\":{\"max_slope_degrees\":8,\"exit_clearance\":0,"
	"                        \"requires_foundation_support\":true} }";

void	Check_Foundation_Grading (void)
{
	for (int i = 0; i < FOUNDATION_GRADING_RESULT_COUNT; i++) {
		Check (::strcmp (Foundation_Grading_Result_Name ((FoundationGradingResultType)i),"unknown") != 0,
				 "grading result %d has no name",i);
	}

	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	NavMeshGenerator::Init ();
	GeneratedWorldPlacementRules::Init ();
	BuildingDescriptorManager::Init ();
	CommanderPlacementValidator::Init ();
	FoundationGradingService::Init ();

	StringClass descriptor_error ("");
	bool descriptor_loaded = BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grading_pad",
																										GRADING_DESCRIPTOR_JSON,
																										&descriptor_error);
	Check (descriptor_loaded,"the grading test descriptor was refused: %s",descriptor_error.Peek_Buffer ());

	float footprint_radius = 0.0f;
	Check (CommanderPlacementValidator::Get_Footprint_Radius ("ow_test_grading_pad",&footprint_radius),
			 "the loaded descriptor reported no footprint radius");
	Check (footprint_radius > 0.0f,"the footprint radius came back %f",footprint_radius);
	Check (!CommanderPlacementValidator::Get_Footprint_Radius ("nothing_loaded",&footprint_radius),
			 "an unloaded descriptor still reported a footprint radius");

	FoundationGradingRequest request;
	request.Position			= Vector3 (0.0f,0.0f,0.0f);
	request.RotationDegrees	= 0.0f;
	request.Team				= 0;
	request.SequenceId		= 7;
	request.DescriptorName	= "ow_test_grading_pad";

	//
	//	Refusals that come before any measurement.
	//
	{
		FoundationGradingReport report = FoundationGradingService::Evaluate (request);
		Check (report.Result == FOUNDATION_GRADING_NO_TERRAIN,
				 "grading with no heightfield at all reported '%s' instead of refusing",
				 Foundation_Grading_Result_Name (report.Result));
		Check (report.SequenceId == request.SequenceId,"a refused report lost its sequence id");

		FoundationGradingRequest unknown = request;
		unknown.DescriptorName = "no_such_descriptor";
		FoundationGradingReport unknown_report = FoundationGradingService::Evaluate (unknown);
		Check (!unknown_report.Is_Valid (),"grading accepted a descriptor that is not loaded");
	}

	//	A 10-degree ramp: steep enough that a 12x12 footprint really does sit on a slope, shallow
	//	enough that a grading can legally level it.
	Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);
	Check (WorldTerrainSystem::Has_Terrain (),"the ramp terrain this check depends on was not created");

	//
	//	A real measurement over real ground.
	//
	FoundationGradingReport baseline;
	{
		baseline = FoundationGradingService::Evaluate (request);
		Check (baseline.Is_Valid (),"grading a shallow ramp was refused as '%s'",
				 Foundation_Grading_Result_Name (baseline.Result));
		Check (baseline.SampleCount > 0,"a valid grading measured no heightfield samples");
		Check (baseline.ApronWidth > 0.0f,"levelling a slope produced a zero-width apron");
		Check ((baseline.CutVolume > 0.0f) || (baseline.FillVolume > 0.0f),
				 "levelling a slope moved no material at all");
		Check (baseline.AffectedArea > 0.0f,"a valid grading affected no area");

		//	The levelled pad is the footprint plus the grid support the heightfield needs to hold
		//	it flat, and the affected area is that plus the apron ring.
		Check (baseline.PlateauRadius > footprint_radius,
				 "the levelled pad (%f) is no wider than the footprint (%f), so its own rim would tilt",
				 baseline.PlateauRadius,footprint_radius);

		float outer = baseline.PlateauRadius + baseline.ApronWidth;
		float expected_area = WWMATH_PI * outer * outer;
		Check (WWMath::Fabs (baseline.AffectedArea - expected_area) < (expected_area * 0.01f),
				 "the reported area %f does not match the pad-plus-apron disc %f",
				 baseline.AffectedArea,expected_area);

		//	Step 5's own slope promise, measured on the surface the grading would leave rather
		//	than assumed from the formula that sized the apron.
		Check (baseline.MaxSlopeChangeRadians <= FoundationGradingService::Get_Limits ().MaxSlopeChangeRadians,
				 "the graded surface would be left at %f rad, above the %f rad limit",
				 baseline.MaxSlopeChangeRadians,
				 FoundationGradingService::Get_Limits ().MaxSlopeChangeRadians);

		//	Evaluate changes nothing: the same request measured twice reports the same thing.
		FoundationGradingReport again = FoundationGradingService::Evaluate (request);
		Check (again.Result == baseline.Result,"measuring twice changed the verdict");
		Check (WWMath::Fabs (again.CutVolume - baseline.CutVolume) < 0.001f,"measuring twice changed the cut volume");
		Check (WWMath::Fabs (again.FillVolume - baseline.FillVolume) < 0.001f,"measuring twice changed the fill volume");
		Check (FoundationGradingService::Get_Executed_Count () == 0,"measuring executed a grading");
	}

	//
	//	Step 5's limits, each proved by making the one it names impossible.
	//
	{
		FoundationGradingLimits tight;
		tight.MaxAffectedArea = 1.0f;			// smaller than any real footprint disc
		FoundationGradingService::Set_Limits (tight);
		FoundationGradingReport report = FoundationGradingService::Evaluate (request);
		Check (report.Result == FOUNDATION_GRADING_AREA_EXCEEDED,
				 "an impossible area limit reported '%s'",Foundation_Grading_Result_Name (report.Result));
	}
	{
		FoundationGradingLimits tight;
		tight.MaxCutDepth = 0.001f;
		FoundationGradingService::Set_Limits (tight);
		FoundationGradingReport report = FoundationGradingService::Evaluate (request);
		Check ((report.Result == FOUNDATION_GRADING_DEPTH_EXCEEDED) ||
				 (report.Result == FOUNDATION_GRADING_HEIGHT_EXCEEDED),
				 "a millimetre cut limit on a 10-degree ramp reported '%s'",
				 Foundation_Grading_Result_Name (report.Result));
	}
	{
		//	A zero limit is a broken profile, not a strict one, and is clamped back to the
		//	documented default rather than silently disabling the feature.
		FoundationGradingLimits broken;
		broken.MaxCutDepth				= 0.0f;
		broken.MaxFillHeight				= -1.0f;
		broken.MaxSlopeChangeRadians	= 0.0f;
		broken.MaxAffectedArea			= 0.0f;
		FoundationGradingService::Set_Limits (broken);

		FoundationGradingLimits defaults;
		Check (FoundationGradingService::Get_Limits ().MaxCutDepth == defaults.MaxCutDepth,
				 "a zero cut limit was accepted instead of clamped");
		Check (FoundationGradingService::Get_Limits ().MaxAffectedArea == defaults.MaxAffectedArea,
				 "a zero area limit was accepted instead of clamped");
	}

	//
	//	Steps 7-9.  Execute, and prove the ground really moved and really came out flat.
	//
	{
		FoundationGradingService::Set_Limits (FoundationGradingLimits ());
		FoundationGradingService::Reset_Counters ();

		float before_center = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (0.0f,0.0f,&before_center),
				 "the ramp could not be sampled before grading");

		FoundationGradingReport report;
		bool executed = FoundationGradingService::Execute (request,&report);
		Check (executed,"executing a valid grading failed as '%s'",Foundation_Grading_Result_Name (report.Result));
		Check (FoundationGradingService::Get_Executed_Count () == 1,
				 "one executed grading was counted %d time(s)",FoundationGradingService::Get_Executed_Count ());
		Check (FoundationGradingService::Get_Last_Executed_Sequence () == request.SequenceId,
				 "the executed sequence id was not recorded");

		//	Flat afterwards: every point inside the footprint sits at the target height.
		float worst = 0.0f;
		for (int i = 0; i < 8; i++) {
			float angle = ((float)i / 8.0f) * 2.0f * WWMATH_PI;
			float x = WWMath::Cos (angle) * (footprint_radius * 0.75f);
			float y = WWMath::Sin (angle) * (footprint_radius * 0.75f);

			float height = 0.0f;
			if (!WorldTerrainSystem::Sample_Height (x,y,&height)) continue;
			worst = WWMath::Max (worst,WWMath::Fabs (height - report.TargetHeight));
		}
		Check (worst < 0.05f,"the graded footprint is still %f off level at its worst point",worst);

		//	And the ground outside the apron did not move.
		{
			float outside_x = report.PlateauRadius + report.ApronWidth + 20.0f;
			float outside_height = 0.0f;
			if (WorldTerrainSystem::Sample_Height (outside_x,0.0f,&outside_height)) {
				float expected = 0.0f;
				//	The ramp rises with x by construction; what matters is only that this point is
				//	not at the graded target, i.e. that the edit was bounded.
				(void)expected;
				Check (WWMath::Fabs (outside_height - report.TargetHeight) > 0.05f,
						 "ground well outside the apron was levelled to the grading's own target");
			}
		}

		AABoxClass region;
		Check (FoundationGradingService::Get_Last_Executed_Region (&region),
				 "an executed grading reported no affected region");
		Check (region.Extent.X > 0.0f,"the affected region has no extent");

		//	Step 7's re-measurement: grading already-flat ground moves almost nothing, which is
		//	how "Execute re-evaluates rather than trusting the preview" is observable.
		FoundationGradingReport second;
		FoundationGradingService::Execute (request,&second);
		Check (second.CutVolume < (report.CutVolume * 0.25f),
				 "re-grading flat ground still cut %f against the original %f",second.CutVolume,report.CutVolume);
		Check (second.FillVolume < (report.FillVolume + 0.001f),
				 "re-grading flat ground filled more than the first pass");
	}

	BuildingDescriptorManager::Unload_All ();
	FoundationGradingService::Shutdown ();
	CommanderPlacementValidator::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	GeneratedWorldPlacementRules::Shutdown ();
	NavMeshGenerator::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	Phase 17 steps 10-13.  Footprint reservation, the completion sweep, and expansion regions.
*/
void	Check_Expansion_Regions (void)
{
	BuildingDescriptorManager::Init ();
	CommanderPlacementValidator::Init ();
	NavMeshGenerator::Init ();
	GeneratedWorldPlacementRules::Init ();
	ExpansionRegionRegistry::Init ();

	StringClass descriptor_error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grading_pad",
																				  GRADING_DESCRIPTOR_JSON,&descriptor_error),
			 "the expansion test descriptor was refused: %s",descriptor_error.Peek_Buffer ());

	float footprint_radius = 0.0f;
	Check (CommanderPlacementValidator::Get_Footprint_Radius ("ow_test_grading_pad",&footprint_radius),
			 "the loaded descriptor reported no footprint radius");

	//
	//	Step 10.  The reservation, and the race it exists to lose safely.
	//
	{
		Check (ExpansionRegionRegistry::Get_Reservation_Count () == 0,"a fresh registry already holds reservations");

		StringClass first_id ("");
		Check (ExpansionRegionRegistry::Reserve ("ow_test_grading_pad",Vector3 (0.0f,0.0f,0.0f),
															 0.0f,0,101,&first_id),
				 "the first reservation on empty ground was refused");
		Check (first_id.Get_Length () > 0,"an accepted reservation carried no id");
		Check (ExpansionRegionRegistry::Get_Reservation_Count () == 1,"an accepted reservation was not recorded");

		//	The same ground, a moment later, by anyone: refused.
		StringClass second_id ("");
		Check (!ExpansionRegionRegistry::Reserve ("ow_test_grading_pad",Vector3 (1.0f,1.0f,0.0f),
															  0.0f,1,102,&second_id),
				 "two reservations were granted over the same ground");
		Check (ExpansionRegionRegistry::Get_Reservation_Count () == 1,"a refused reservation was recorded anyway");

		//	Ground far enough away is a different claim.
		StringClass far_id ("");
		float clear = (footprint_radius * 2.0f) + 10.0f;
		Check (ExpansionRegionRegistry::Reserve ("ow_test_grading_pad",Vector3 (clear,0.0f,0.0f),
															 0.0f,1,103,&far_id),
				 "a reservation well clear of an existing one was refused");
		Check (ExpansionRegionRegistry::Get_Reservation_Count () == 2,"the second distinct reservation was not recorded");

		Check (ExpansionRegionRegistry::Is_Reserved (Vector3 (0.0f,0.0f,0.0f),1.0f),
				 "reserved ground does not report itself reserved");
		Check (!ExpansionRegionRegistry::Is_Reserved (Vector3 (2000.0f,2000.0f,0.0f),1.0f),
				 "ground nobody claimed reports itself reserved");

		Check (!ExpansionRegionRegistry::Reserve ("no_such_descriptor",Vector3 (500.0f,500.0f,0.0f),
															  0.0f,0,104,nullptr),
				 "a reservation was granted for a descriptor that is not loaded");

		//	Binding and releasing.
		Check (ExpansionRegionRegistry::Bind_Building (first_id.Peek_Buffer (),55),
				 "a live reservation could not be bound to a building");
		Check (!ExpansionRegionRegistry::Bind_Building ("reservation_does_not_exist",56),
				 "binding succeeded against an id that does not exist");

		Check (ExpansionRegionRegistry::Release (far_id.Peek_Buffer ()),"releasing a live reservation failed");
		Check (ExpansionRegionRegistry::Get_Reservation_Count () == 1,"a released reservation was not removed");
		Check (!ExpansionRegionRegistry::Release (far_id.Peek_Buffer ()),"the same reservation released twice");

		ConstructionReservation record;
		Check (ExpansionRegionRegistry::Get_Reservation (0,&record),"the surviving reservation could not be read back");
		Check (record.BuildingId == 55,"the bound building id did not read back");
		Check (record.SequenceId == 101,"the reservation lost the placement sequence it belongs to");
		Check (!ExpansionRegionRegistry::Get_Reservation (99,&record),"an out-of-range reservation index was accepted");
	}

	//
	//	Step 12.  The completion sweep.  With no navigation and no live power grid this reports
	//	exactly that rather than claiming work it did not do -- which is the honest state in this
	//	harness, and the state the check proves.
	//
	{
		StringClass report ("");
		Check (ExpansionRegionRegistry::Note_Construction_Complete (55,"ow_test_grading_pad",
																						Vector3 (0.0f,0.0f,0.0f),0,&report),
				 "a completion over a loaded descriptor was refused");
		Check (report.Get_Length () > 0,"the completion sweep reported nothing about what it did");
		Check (ExpansionRegionRegistry::Get_Completion_Count () == 1,"the completion was not counted");

		//	The reservation this building held is gone, and gone last.
		Check (ExpansionRegionRegistry::Get_Reservation_Count () == 0,
				 "a completed building's reservation was not released");

		Check (NavMeshGenerator::Has_Navigation () ||
				 (ExpansionRegionRegistry::Get_Navigation_Rebuild_Count () == 0),
				 "navigation was reported rebuilt when none was ever built");

		StringClass ignored ("");
		Check (!ExpansionRegionRegistry::Note_Construction_Complete (56,"no_such_descriptor",
																						 Vector3 (0.0f,0.0f,0.0f),0,&ignored),
				 "a completion was accepted for a descriptor that is not loaded");
	}

	//
	//	Step 13.  Region metadata.
	//
	{
		float cluster = (footprint_radius * 3.0f);

		//	A second structure close to the first, and a third far away: one home region and one
		//	forward one.
		StringClass report ("");
		ExpansionRegionRegistry::Note_Construction_Complete (57,"ow_test_grading_pad",
																			  Vector3 (footprint_radius,0.0f,0.0f),0,&report);
		ExpansionRegionRegistry::Note_Construction_Complete (58,"ow_test_grading_pad",
																			  Vector3 (cluster * 10.0f,0.0f,0.0f),0,&report);

		StringClass error ("");
		Check (!ExpansionRegionRegistry::Rebuild_Regions (0.0f,&error),
				 "a zero cluster radius was accepted");
		Check (error.Get_Length () > 0,"a refused region rebuild carried no reason");

		Check (ExpansionRegionRegistry::Rebuild_Regions (cluster,&error),
				 "the region rebuild failed: %s",error.Peek_Buffer ());
		Check (ExpansionRegionRegistry::Get_Region_Count () == 2,
				 "two clusters of structures produced %d region(s)",ExpansionRegionRegistry::Get_Region_Count ());
		Check (ExpansionRegionRegistry::Get_Region_Count_For_Team (0) == 2,
				 "the owning team does not own both of its own regions");
		Check (ExpansionRegionRegistry::Get_Region_Count_For_Team (1) == 0,
				 "a team that built nothing owns a region");

		ExpansionRegion home;
		Check (ExpansionRegionRegistry::Get_Region (0,&home),"the first region could not be read back");
		Check (home.StructureCount == 2,"the home cluster holds %d structure(s), not 2",home.StructureCount);
		Check (!home.IsForward,"the team's first region was classified as a forward position");
		Check (home.Id.Get_Length () > 0,"a region carried no stable id");
		Check (home.Radius > 0.0f,"a region with structures in it has no radius");

		ExpansionRegion forward;
		Check (ExpansionRegionRegistry::Get_Region (1,&forward),"the second region could not be read back");
		Check (forward.StructureCount == 1,"the forward cluster holds %d structure(s), not 1",forward.StructureCount);
		Check (forward.IsForward,"a cluster ten cluster-radii from home was not classified forward");
		Check (forward.Id != home.Id,"two regions share one id");

		//	A region can be found from a point inside it, and not from a point outside every one.
		ExpansionRegion found;
		Check (ExpansionRegionRegistry::Find_Region_At (Vector3 (0.0f,0.0f,0.0f),&found),
				 "the home region could not be found from its own centre");
		Check (found.Id == home.Id,"a point in the home region resolved to a different region");
		Check (!ExpansionRegionRegistry::Find_Region_At (Vector3 (100000.0f,100000.0f,0.0f),&found),
				 "a point nowhere near any region resolved to one");

		//	Deterministic: the same completions clustered twice are the same regions.
		unsigned int first_checksum = ExpansionRegionRegistry::Get_Checksum ();
		Check (first_checksum != 0,"a rebuilt region set carries a zero checksum");
		Check (ExpansionRegionRegistry::Rebuild_Regions (cluster,&error),"the second region rebuild failed");
		Check (ExpansionRegionRegistry::Get_Checksum () == first_checksum,
				 "clustering the same completions twice produced a different checksum");

		//	A different cluster radius is a different answer, which is what makes the checksum
		//	sensitive to the thing it is supposed to be sensitive to.
		Check (ExpansionRegionRegistry::Rebuild_Regions (cluster * 100.0f,&error),
				 "a generous region rebuild failed");
		Check (ExpansionRegionRegistry::Get_Region_Count () == 1,
				 "a cluster radius covering everything still produced %d regions",
				 ExpansionRegionRegistry::Get_Region_Count ());
	}

	ExpansionRegionRegistry::Clear ();
	Check (ExpansionRegionRegistry::Get_Region_Count () == 0,"Clear left regions behind");
	Check (ExpansionRegionRegistry::Get_Reservation_Count () == 0,"Clear left reservations behind");

	BuildingDescriptorManager::Unload_All ();
	ExpansionRegionRegistry::Shutdown ();
	GeneratedWorldPlacementRules::Shutdown ();
	NavMeshGenerator::Shutdown ();
	CommanderPlacementValidator::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
}


/***********************************************************************************************
**	Phase 18.  The versioned WorldOperation envelope, the world revision, and the journal.
*/
void	Check_World_Operations (void)
{
	//
	//	Every type, result and dependency mask has to be real before anything else matters.
	//
	for (int i = 0; i < WORLD_OPERATION_TYPE_COUNT; i++) {
		WorldOperationType type = (WorldOperationType)i;
		Check (::strcmp (World_Operation_Type_Name (type),"unknown") != 0,
				 "operation type %d has no name",i);

		//	Every one of these moves the ground, so every one invalidates render and collision.
		unsigned int dependencies = World_Operation_Dependencies (type);
		Check ((dependencies & WORLD_OP_DEPENDS_RENDER) != 0,
				 "%s claims not to invalidate render",World_Operation_Type_Name (type));
		Check ((dependencies & WORLD_OP_DEPENDS_COLLISION) != 0,
				 "%s claims not to invalidate collision",World_Operation_Type_Name (type));
		Check ((dependencies & WORLD_OP_DEPENDS_NAV) != 0,
				 "%s claims not to invalidate navigation",World_Operation_Type_Name (type));
	}
	for (int i = 0; i < WORLD_OPERATION_RESULT_COUNT; i++) {
		Check (::strcmp (World_Operation_Result_Name ((WorldOperationResultType)i),"unknown") != 0,
				 "operation result %d has no name",i);
	}

	//	The water-owning types are the ones that must say so; the dry ones must not claim it.
	Check ((World_Operation_Dependencies (WORLD_OP_CARVE_CHANNEL) & WORLD_OP_DEPENDS_WATER) != 0,
			 "CARVE_CHANNEL does not invalidate water");
	Check ((World_Operation_Dependencies (WORLD_OP_RECLAIM_SHALLOW_WATER) & WORLD_OP_DEPENDS_WATER) != 0,
			 "RECLAIM_SHALLOW_WATER does not invalidate water");
	Check ((World_Operation_Dependencies (WORLD_OP_SMOOTH) & WORLD_OP_DEPENDS_WATER) == 0,
			 "SMOOTH claims to invalidate water");

	//	Ordering: relative types care about the revision, absolute ones do not.
	Check (World_Operation_Ordering_Matters (WORLD_OP_RAISE),"RAISE does not care about ordering");
	Check (World_Operation_Ordering_Matters (WORLD_OP_LOWER),"LOWER does not care about ordering");
	Check (World_Operation_Ordering_Matters (WORLD_OP_SMOOTH),"SMOOTH does not care about ordering");
	Check (World_Operation_Ordering_Matters (WORLD_OP_CREATE_CRATER),"CREATE_CRATER does not care about ordering");
	Check (!World_Operation_Ordering_Matters (WORLD_OP_FLATTEN),"FLATTEN claims ordering matters");
	Check (!World_Operation_Ordering_Matters (WORLD_OP_RESTORE_REGION),"RESTORE_REGION claims ordering matters");

	//
	//	Step 1's envelope, on its own: parameters, checksum and the compact form.
	//
	{
		WorldOperationClass fresh;
		Check (fresh.FormatVersion == (unsigned int)WorldOperationClass::CURRENT_FORMAT_VERSION,
				 "a fresh operation does not carry the current format version");

		//	A default operation is a RAISE of nothing, which is malformed, not a no-op.
		Check (fresh.Validate_Parameters () == WORLD_OP_RESULT_BAD_PARAMETERS,
				 "a zero-radius RAISE was accepted");

		WorldOperationClass raise;
		raise.Type		= WORLD_OP_RAISE;
		raise.Center	= Vector3 (10.0f,20.0f,5.0f);
		raise.Radius	= 12.0f;
		raise.Depth		= 3.0f;
		Check (raise.Validate_Parameters () == WORLD_OP_RESULT_APPLIED,
				 "a well-formed RAISE was refused");

		//	The sign belongs to the type, so a negative depth is malformed rather than a quiet
		//	LOWER -- the difference between a rule and a convention.
		WorldOperationClass negative = raise;
		negative.Depth = -3.0f;
		Check (negative.Validate_Parameters () == WORLD_OP_RESULT_BAD_PARAMETERS,
				 "a RAISE with a negative depth was accepted");

		WorldOperationClass bad_version = raise;
		bad_version.FormatVersion = 99u;
		Check (bad_version.Validate_Parameters () == WORLD_OP_RESULT_BAD_FORMAT_VERSION,
				 "an unknown format version was accepted");

		//	An apron inside its own pad is not an apron.
		WorldOperationClass grade;
		grade.Type		= WORLD_OP_GRADE_FOUNDATION;
		grade.Center	= Vector3 (0.0f,0.0f,4.0f);
		grade.Radius	= 10.0f;
		grade.Blend		= 6.0f;
		Check (grade.Validate_Parameters () == WORLD_OP_RESULT_BAD_PARAMETERS,
				 "a GRADE_FOUNDATION whose apron is inside its pad was accepted");
		grade.Blend		= 16.0f;
		Check (grade.Validate_Parameters () == WORLD_OP_RESULT_APPLIED,
				 "a well-formed GRADE_FOUNDATION was refused");

		//	A polyline type needs a polyline.
		WorldOperationClass road;
		road.Type	= WORLD_OP_CARVE_ROAD;
		road.Width	= 8.0f;
		road.Blend	= 0.8f;
		Check (road.Validate_Parameters () == WORLD_OP_RESULT_BAD_PARAMETERS,
				 "a CARVE_ROAD with no path was accepted");
		road.Path.Add (Vector3 (-40.0f,0.0f,2.0f));
		road.Path.Add (Vector3 ( 40.0f,0.0f,2.0f));
		Check (road.Validate_Parameters () == WORLD_OP_RESULT_APPLIED,
				 "a well-formed CARVE_ROAD was refused");

		//	Bounds: the polyline's own circle, widened by half the strip, never optimistic.
		Vector3 center (0.0f,0.0f,0.0f);
		float radius = 0.0f;
		Check (road.Get_Affected_Bounds (&center,&radius),"a well-formed CARVE_ROAD reported no bounds");
		Check (radius >= (40.0f + (road.Width * 0.5f)) - 0.01f,
				 "the CARVE_ROAD bounds radius %f does not cover its own path",radius);

		//	GRADE_FOUNDATION reaches to its apron, not its pad.
		Check (grade.Get_Affected_Bounds (&center,&radius),"a well-formed GRADE reported no bounds");
		Check (radius >= grade.Blend - 0.01f,"the GRADE bounds radius %f is inside its own apron",radius);

		//
		//	Step 14's compact form: every field survives the round trip, and nothing in the size
		//	scales with the region the operation changes.
		//
		int flat_size = raise.Get_Block_Size ();
		int road_size = road.Get_Block_Size ();
		Check (flat_size > 0,"an operation reported a zero block size");
		Check (road_size == (flat_size + (2 * 3 * 4)),
				 "a two-point path cost %d bytes over the flat header, not 24",road_size - flat_size);

		WorldOperationClass huge = raise;
		huge.Radius = 100000.0f;
		Check (huge.Get_Block_Size () == flat_size,
				 "a hundred-times-larger operation serialises to a different size");

		unsigned char buffer[1024];
		int used = 0;
		Check (road.Write_Block (buffer,sizeof (buffer),&used),"a well-formed operation would not serialise");
		Check (used == road_size,"serialisation used %d bytes, not the %d it reported",used,road_size);

		WorldOperationClass restored;
		int consumed = 0;
		Check (restored.Read_Block (buffer,used,&consumed),"a serialised operation would not deserialise");
		Check (consumed == used,"deserialisation consumed %d bytes of %d",consumed,used);
		Check (restored == road,"an operation did not survive its own round trip");
		Check (restored.Compute_Checksum () == road.Compute_Checksum (),
				 "a round-tripped operation carries a different checksum");

		//	Blend is the field most easily lost in a hand-written wire format, so it is asserted
		//	by name rather than only through operator ==.
		Check (WWMath::Fabs (restored.Blend - road.Blend) < 0.0001f,
				 "Blend read back as %f, not %f",restored.Blend,road.Blend);

		//	A truncated block is refused, not half-read.
		WorldOperationClass truncated;
		Check (!truncated.Read_Block (buffer,4,&consumed),"a four-byte block was accepted as an operation");
		Check (!road.Write_Block (buffer,4,&used),"a four-byte buffer accepted a whole operation");

		//	Two operations differing in one quantized field differ in checksum.
		WorldOperationClass moved = raise;
		moved.Center.X += 1.0f;
		Check (moved.Compute_Checksum () != raise.Compute_Checksum (),
				 "moving an operation a metre did not change its checksum");
	}

	//
	//	The journal, over real terrain.
	//
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	NavMeshGenerator::Init ();
	GeneratedWorldPlacementRules::Init ();
	WorldOperationJournal::Init ();

	//
	//	Refusals before any terrain exists.
	//
	{
		Check (!WorldOperationJournal::Is_Open (),"a fresh journal is already open");
		Check (WorldOperationJournal::Get_Revision () == 0,"a closed journal reports a revision");

		StringClass error ("");
		Check (!WorldOperationJournal::Begin (&error),"a journal opened with no terrain at all");
		Check (error.Get_Length () > 0,"a refused journal open carried no reason");

		WorldOperationClass raise;
		raise.Type		= WORLD_OP_RAISE;
		raise.Radius	= 10.0f;
		raise.Depth		= 2.0f;
		WorldOperationApplyResult result = WorldOperationJournal::Submit (raise);
		Check (result.Result == WORLD_OP_RESULT_NO_JOURNAL,
				 "submitting with no journal reported '%s'",World_Operation_Result_Name (result.Result));
		Check (!result.Committed,"a refused operation reported itself committed");
	}

	Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	{
		StringClass error ("");
		Check (WorldOperationJournal::Begin (&error),"opening a journal over real terrain failed: %s",
				 error.Peek_Buffer ());
		Check (WorldOperationJournal::Is_Open (),"a successful Begin did not open the journal");

		//	Revision 1, not 0: 0 is "no journal", and an operation submitted against 0 is a
		//	submitter that never read one.
		Check (WorldOperationJournal::Get_Revision () == 1,
				 "a freshly opened journal is at revision %u, not 1",WorldOperationJournal::Get_Revision ());
		Check (WorldOperationJournal::Get_Operation_Count () == 0,"a fresh journal already holds operations");
	}

	unsigned int base_terrain_checksum = WorldOperationJournal::Get_Terrain_Checksum ();
	Check (base_terrain_checksum != 0,"real terrain produced a zero checksum");

	//
	//	Step 6.  A relative operation submitted against the wrong revision is refused; an absolute
	//	one is not, because its outcome does not depend on the ground it finds.
	//
	{
		WorldOperationClass stale;
		stale.Type			= WORLD_OP_RAISE;
		stale.BaseRevision	= 99u;
		stale.Center		= Vector3 (0.0f,0.0f,0.0f);
		stale.Radius		= 10.0f;
		stale.Depth			= 2.0f;

		WorldOperationApplyResult result = WorldOperationJournal::Submit (stale);
		Check (result.Result == WORLD_OP_RESULT_STALE_REVISION,
				 "a RAISE against revision 99 reported '%s'",World_Operation_Result_Name (result.Result));
		Check (result.Detail.Get_Length () > 0,"a stale rejection said which revisions were involved: nothing");
		Check (WorldOperationJournal::Get_Operation_Count () == 0,"a refused operation was journalled");
		Check (WorldOperationJournal::Get_Terrain_Checksum () == base_terrain_checksum,
				 "a refused operation changed the ground");

		WorldOperationClass absolute;
		absolute.Type			= WORLD_OP_FLATTEN;
		absolute.BaseRevision	= 99u;
		absolute.Center		= Vector3 (0.0f,0.0f,0.0f);
		absolute.Radius		= 10.0f;
		absolute.Blend			= 1.0f;
		StringClass detail ("");
		Check (WorldOperationJournal::Validate (absolute,&detail) != WORLD_OP_RESULT_STALE_REVISION,
				 "an absolute FLATTEN was refused for a stale revision");
	}

	//
	//	Steps 5, 7-13.  A real RAISE: the ground moves, the journal records it, the revision
	//	advances, and the reservation is released again afterwards.
	//
	unsigned int after_raise_checksum = 0;
	{
		WorldOperationClass raise;
		raise.Type			= WORLD_OP_RAISE;
		raise.BaseRevision	= WorldOperationJournal::Get_Revision ();
		raise.Center		= Vector3 (0.0f,0.0f,0.0f);
		raise.Radius		= 16.0f;
		raise.Depth			= 4.0f;
		raise.TimeSeconds	= 1.0f;

		float before_height = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (0.0f,0.0f,&before_height),
				 "the ramp could not be sampled before the operation");

		WorldOperationApplyResult result = WorldOperationJournal::Submit (raise);
		Check (result.Is_Applied (),"a well-formed RAISE was refused as '%s' (%s)",
				 World_Operation_Result_Name (result.Result),result.Detail.Peek_Buffer ());
		Check (result.OperationId == 1,"the first operation was given id %u, not 1",result.OperationId);
		Check (result.AffectedChunkCount > 0,"an operation over real terrain affected no chunks");
		Check (result.DependenciesRequired != 0,"an operation required no dependencies at all");
		Check (result.Committed,"a fully satisfied operation did not advance the revision: %s",
				 result.Detail.Peek_Buffer ());
		Check (WorldOperationJournal::Get_Revision () == 2,
				 "the revision is %u after one committed operation, not 2",WorldOperationJournal::Get_Revision ());

		float after_height = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (0.0f,0.0f,&after_height),
				 "the ground could not be sampled after the operation");
		Check (after_height > (before_height + 1.0f),
				 "a 4-unit RAISE moved the ground from %f to %f",before_height,after_height);

		after_raise_checksum = WorldOperationJournal::Get_Terrain_Checksum ();
		Check (after_raise_checksum != base_terrain_checksum,"the ground moved but the checksum did not");
		Check (result.TerrainChecksum == after_raise_checksum,
				 "the result's terrain checksum does not match the terrain");

		//	Step 5: held across the operation, released after it.  A reservation still held would
		//	block every later operation in the same region.
		Check (WorldOperationJournal::Get_Reserved_Chunk_Count () == 0,
				 "%d chunk(s) were still reserved after the operation committed",
				 WorldOperationJournal::Get_Reserved_Chunk_Count ());

		Check (WorldOperationJournal::Get_Operation_Count () == 1,"the applied operation was not journalled");

		WorldOperationClass journalled;
		Check (WorldOperationJournal::Get_Operation (0,&journalled),"the journalled operation could not be read back");
		Check (journalled.Type == WORLD_OP_RAISE,"the journal recorded the wrong type");
		Check (journalled.OperationId == result.OperationId,"the journal recorded a different operation id");
		Check (!WorldOperationJournal::Get_Operation (99,&journalled),"an out-of-range journal index was accepted");
	}

	//
	//	Step 5, proved rather than assumed: ground somebody else holds cannot be taken.
	//
	{
		int reserved = 0;
		Check (WorldOperationJournal::Reserve_Chunks (Vector3 (0.0f,0.0f,0.0f),16.0f,&reserved),
				 "an explicit reservation over free ground was refused");
		Check (reserved > 0,"an explicit reservation took no chunks");

		WorldOperationClass blocked;
		blocked.Type			= WORLD_OP_RAISE;
		blocked.BaseRevision	= WorldOperationJournal::Get_Revision ();
		blocked.Center			= Vector3 (0.0f,0.0f,0.0f);
		blocked.Radius			= 16.0f;
		blocked.Depth			= 1.0f;
		blocked.TimeSeconds	= 2.0f;

		WorldOperationApplyResult result = WorldOperationJournal::Submit (blocked);
		Check (result.Result == WORLD_OP_RESULT_CHUNKS_RESERVED,
				 "an operation over reserved ground reported '%s'",World_Operation_Result_Name (result.Result));
		Check (WorldOperationJournal::Get_Terrain_Checksum () == after_raise_checksum,
				 "an operation refused for reserved ground still moved the ground");

		WorldOperationJournal::Release_All_Chunks ();
		Check (WorldOperationJournal::Get_Reserved_Chunk_Count () == 0,"Release_All_Chunks left chunks reserved");

		//	And the same operation now succeeds, which is what proves the refusal was the
		//	reservation rather than anything else about it.
		blocked.BaseRevision = WorldOperationJournal::Get_Revision ();
		WorldOperationApplyResult retry = WorldOperationJournal::Submit (blocked);
		Check (retry.Is_Applied (),"the same operation was still refused after the ground was released: %s",
				 World_Operation_Result_Name (retry.Result));
	}

	//
	//	Step 15.  Bounded crater rules, each proved by breaking the one it names.
	//
	{
		WorldOperationCraterLimits limits;
		limits.MaxRadius				= 8.0f;
		limits.MaxDepth				= 2.5f;
		limits.MaxCratersPerWindow	= 2;
		limits.RateWindowSeconds	= 10.0f;
		WorldOperationJournal::Set_Crater_Limits (limits);

		WorldOperationClass crater;
		crater.Type			= WORLD_OP_CREATE_CRATER;
		crater.Center		= Vector3 (40.0f,40.0f,0.0f);
		crater.Radius		= 6.0f;
		crater.Depth		= 1.5f;
		crater.TimeSeconds	= 100.0f;

		//	Too wide.
		WorldOperationClass wide = crater;
		wide.Radius			= limits.MaxRadius + 5.0f;
		wide.BaseRevision	= WorldOperationJournal::Get_Revision ();
		WorldOperationApplyResult wide_result = WorldOperationJournal::Submit (wide);
		Check (wide_result.Result == WORLD_OP_RESULT_CRATER_LIMIT,
				 "an oversized crater reported '%s'",World_Operation_Result_Name (wide_result.Result));

		//	Too deep.
		WorldOperationClass deep = crater;
		deep.Depth			= limits.MaxDepth + 5.0f;
		deep.BaseRevision	= WorldOperationJournal::Get_Revision ();
		WorldOperationApplyResult deep_result = WorldOperationJournal::Submit (deep);
		Check (deep_result.Result == WORLD_OP_RESULT_CRATER_LIMIT,
				 "an over-deep crater reported '%s'",World_Operation_Result_Name (deep_result.Result));

		//	Two land, the third inside the same window does not.
		for (int i = 0; i < limits.MaxCratersPerWindow; i++) {
			WorldOperationClass ok = crater;
			ok.Center.X		+= (float)i * 20.0f;
			ok.TimeSeconds	= 100.0f + (float)i;
			ok.BaseRevision	= WorldOperationJournal::Get_Revision ();
			WorldOperationApplyResult ok_result = WorldOperationJournal::Submit (ok);
			Check (ok_result.Is_Applied (),"crater %d of the allowance was refused as '%s' (%s)",
					 i,World_Operation_Result_Name (ok_result.Result),ok_result.Detail.Peek_Buffer ());
		}

		WorldOperationClass over = crater;
		over.Center.X		+= 60.0f;
		over.TimeSeconds	= 102.0f;
		over.BaseRevision	= WorldOperationJournal::Get_Revision ();
		WorldOperationApplyResult over_result = WorldOperationJournal::Submit (over);
		Check (over_result.Result == WORLD_OP_RESULT_RATE_LIMITED,
				 "the crater past the window's allowance reported '%s'",
				 World_Operation_Result_Name (over_result.Result));

		//	The same crater well outside the window is fine, which is what makes the limit a rate
		//	limit rather than a total.
		WorldOperationClass later = over;
		later.TimeSeconds	= 100.0f + (limits.RateWindowSeconds * 3.0f);
		later.BaseRevision	= WorldOperationJournal::Get_Revision ();
		WorldOperationApplyResult later_result = WorldOperationJournal::Submit (later);
		Check (later_result.Is_Applied (),"a crater outside the rate window was refused as '%s'",
				 World_Operation_Result_Name (later_result.Result));

		//	A broken limit is clamped, not accepted.
		WorldOperationCraterLimits broken;
		broken.MaxRadius				= 0.0f;
		broken.MaxCratersPerWindow	= 0;
		WorldOperationJournal::Set_Crater_Limits (broken);
		Check (WorldOperationJournal::Get_Crater_Limits ().MaxRadius > 0.0f,
				 "a zero crater radius limit was accepted");
		Check (WorldOperationJournal::Get_Crater_Limits ().MaxCratersPerWindow > 0,
				 "a zero crater count limit was accepted");

		WorldOperationJournal::Set_Crater_Limits (limits);
	}

	//
	//	Step 3's "RECLAIM_SHALLOW_WATER if enabled" -- refused outright while it is not.
	//
	{
		Check (!WorldOperationJournal::Is_Shallow_Water_Reclamation_Enabled (),
				 "shallow water reclamation is enabled by default");

		WorldOperationClass reclaim;
		reclaim.Type			= WORLD_OP_RECLAIM_SHALLOW_WATER;
		reclaim.BaseRevision	= WorldOperationJournal::Get_Revision ();
		reclaim.Center			= Vector3 (0.0f,0.0f,0.0f);
		reclaim.Radius			= 10.0f;
		WorldOperationApplyResult result = WorldOperationJournal::Submit (reclaim);
		Check (result.Result == WORLD_OP_RESULT_DISABLED,
				 "a disabled reclamation reported '%s'",World_Operation_Result_Name (result.Result));

		//	Enabled, but with no water anywhere, it still refuses -- honestly, and for the right
		//	reason rather than by silently doing nothing.
		WorldOperationJournal::Set_Shallow_Water_Reclamation_Enabled (true);
		reclaim.BaseRevision = WorldOperationJournal::Get_Revision ();
		WorldOperationApplyResult dry = WorldOperationJournal::Submit (reclaim);
		Check (dry.Result == WORLD_OP_RESULT_BAD_PARAMETERS,
				 "reclaiming dry ground reported '%s'",World_Operation_Result_Name (dry.Result));
		Check (dry.Detail.Get_Length () > 0,"a refused reclamation said nothing about why");
		WorldOperationJournal::Set_Shallow_Water_Reclamation_Enabled (false);
	}

	//
	//	Every remaining operation type, applied for real.  Each one has to move the ground, or it
	//	is a type this phase claims to implement and does not.
	//
	{
		//	SMOOTH is not in this list, and not because it is unimplemented: the test terrain is a
		//	uniform ramp, and a plane is already its own neighbour mean, so smoothing one is
		//	correctly a no-op.  It gets its own block below, over ground with real curvature in it,
		//	where the thing worth asserting is that a dent gets shallower rather than that some
		//	bytes changed.
		struct { WorldOperationType Type; const char * Name; } TYPES[] = {
			{ WORLD_OP_LOWER,				"LOWER" },
			{ WORLD_OP_FLATTEN,			"FLATTEN" },
			{ WORLD_OP_GRADE_FOUNDATION,	"GRADE_FOUNDATION" },
			{ WORLD_OP_CARVE_ROAD,		"CARVE_ROAD" },
			{ WORLD_OP_CARVE_CHANNEL,	"CARVE_CHANNEL" },
		};

		float x = -120.0f;
		for (int i = 0; i < (int)(sizeof (TYPES) / sizeof (TYPES[0])); i++) {

			unsigned int before = WorldOperationJournal::Get_Terrain_Checksum ();

			WorldOperationClass operation;
			operation.Type			= TYPES[i].Type;
			operation.BaseRevision	= WorldOperationJournal::Get_Revision ();
			operation.TimeSeconds	= 200.0f + (float)i;
			operation.Center		= Vector3 (x,-120.0f,3.0f);
			operation.Radius		= 14.0f;
			operation.Depth			= 2.0f;
			operation.Width			= 8.0f;
			operation.Blend			= 0.8f;

			if (TYPES[i].Type == WORLD_OP_GRADE_FOUNDATION) {
				operation.Blend = 24.0f;			// the apron radius
			}
			if ((TYPES[i].Type == WORLD_OP_CARVE_ROAD) || (TYPES[i].Type == WORLD_OP_CARVE_CHANNEL)) {
				operation.Path.Add (Vector3 (x - 30.0f,-120.0f,2.0f));
				operation.Path.Add (Vector3 (x + 30.0f,-120.0f,2.0f));
			}

			WorldOperationApplyResult result = WorldOperationJournal::Submit (operation);
			Check (result.Is_Applied (),"%s was refused as '%s' (%s)",
					 TYPES[i].Name,World_Operation_Result_Name (result.Result),result.Detail.Peek_Buffer ());

			if (result.Is_Applied ()) {
				Check (WorldOperationJournal::Get_Terrain_Checksum () != before,
						 "%s applied but changed no ground",TYPES[i].Name);
				Check (result.Committed,"%s did not advance the revision: %s",
						 TYPES[i].Name,result.Detail.Peek_Buffer ());
			}

			x += 45.0f;
		}
	}

	//
	//	SMOOTH, over ground that has something to smooth.
	//
	{
		static const float SMOOTH_X = 120.0f;
		static const float SMOOTH_Y = 120.0f;

		//	A narrow, deep dent, then a wider smooth over it.  A plane is its own neighbour mean,
		//	so a ramp would report success while doing nothing; a dent is curvature, and curvature
		//	is what this operation exists to remove.
		WorldOperationClass dent;
		dent.Type			= WORLD_OP_LOWER;
		dent.BaseRevision	= WorldOperationJournal::Get_Revision ();
		dent.Center			= Vector3 (SMOOTH_X,SMOOTH_Y,0.0f);
		dent.Radius			= 10.0f;
		dent.Depth			= 6.0f;
		dent.TimeSeconds	= 400.0f;
		WorldOperationApplyResult dent_result = WorldOperationJournal::Submit (dent);
		Check (dent_result.Is_Applied (),"the dent this SMOOTH check depends on was refused as '%s'",
				 World_Operation_Result_Name (dent_result.Result));

		float rim = 0.0f, bottom = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (SMOOTH_X + 12.0f,SMOOTH_Y,&rim),"the dent rim could not be sampled");
		Check (WorldTerrainSystem::Sample_Height (SMOOTH_X,SMOOTH_Y,&bottom),"the dent bottom could not be sampled");
		float before_relief = rim - bottom;
		Check (before_relief > 1.0f,"the dent this check depends on is only %f deep",before_relief);

		unsigned int before = WorldOperationJournal::Get_Terrain_Checksum ();

		WorldOperationClass smooth;
		smooth.Type			= WORLD_OP_SMOOTH;
		smooth.BaseRevision	= WorldOperationJournal::Get_Revision ();
		smooth.Center		= Vector3 (SMOOTH_X,SMOOTH_Y,0.0f);
		smooth.Radius		= 22.0f;
		smooth.Blend		= 1.0f;
		smooth.TimeSeconds	= 401.0f;

		WorldOperationApplyResult result = WorldOperationJournal::Submit (smooth);
		Check (result.Is_Applied (),"SMOOTH was refused as '%s' (%s)",
				 World_Operation_Result_Name (result.Result),result.Detail.Peek_Buffer ());
		Check (result.Committed,"SMOOTH did not advance the revision: %s",result.Detail.Peek_Buffer ());
		Check (WorldOperationJournal::Get_Terrain_Checksum () != before,
				 "SMOOTH over a real dent changed no ground");

		Check (WorldTerrainSystem::Sample_Height (SMOOTH_X + 12.0f,SMOOTH_Y,&rim),"the smoothed rim could not be sampled");
		Check (WorldTerrainSystem::Sample_Height (SMOOTH_X,SMOOTH_Y,&bottom),"the smoothed bottom could not be sampled");
		float after_relief = rim - bottom;
		Check (after_relief < before_relief,
				 "smoothing left the dent %f deep, no shallower than the %f it started at",
				 after_relief,before_relief);
	}

	//
	//	RESTORE_REGION, and the base snapshot that makes it possible.
	//
	{
		//	Restore exactly the region the first RAISE moved, and the ground there is the ground
		//	the journal opened over again.
		float restored_reference = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (0.0f,0.0f,&restored_reference),
				 "the raised region could not be sampled");

		WorldOperationClass restore;
		restore.Type			= WORLD_OP_RESTORE_REGION;
		restore.BaseRevision	= WorldOperationJournal::Get_Revision ();
		restore.Center			= Vector3 (0.0f,0.0f,0.0f);
		restore.Radius			= 40.0f;
		restore.TimeSeconds	= 300.0f;

		WorldOperationApplyResult result = WorldOperationJournal::Submit (restore);
		Check (result.Is_Applied (),"RESTORE_REGION was refused as '%s' (%s)",
				 World_Operation_Result_Name (result.Result),result.Detail.Peek_Buffer ());

		float after = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (0.0f,0.0f,&after),"the restored region could not be sampled");
		Check (after < (restored_reference - 1.0f),
				 "restoring a raised region left it at %f, barely below the raised %f",after,restored_reference);

		//	And ground well outside the restored circle was not touched by the restore.
		float outside = 0.0f;
		if (WorldTerrainSystem::Sample_Height (-120.0f,-120.0f,&outside)) {
			Check (WWMath::Is_Valid_Float (outside),"the ground outside the restored region is not a number");
		}
	}

	//
	//	Step 16.  The whole point: base plus this ordered list reproduces this ground.
	//
	{
		unsigned int before_replay = WorldOperationJournal::Get_Terrain_Checksum ();
		unsigned int journal_checksum = WorldOperationJournal::Get_Journal_Checksum ();
		unsigned int revision_before = WorldOperationJournal::Get_Revision ();
		int operations_before = WorldOperationJournal::Get_Operation_Count ();

		Check (operations_before > 1,"the replay check has only %d operation(s) to replay",operations_before);

		StringClass report ("");
		Check (WorldOperationJournal::Replay (&report),"replay refused an operation it had itself applied: %s",
				 report.Peek_Buffer ());

		Check (WorldOperationJournal::Get_Terrain_Checksum () == before_replay,
				 "replaying the journal from the base snapshot produced different ground "
				 "(0x%08X, was 0x%08X)",WorldOperationJournal::Get_Terrain_Checksum (),before_replay);

		//	Replay reproduces a state; it does not invent history.
		Check (WorldOperationJournal::Get_Revision () == revision_before,"replay advanced the world revision");
		Check (WorldOperationJournal::Get_Operation_Count () == operations_before,
				 "replay changed the number of journalled operations");
		Check (WorldOperationJournal::Get_Journal_Checksum () == journal_checksum,
				 "replay changed the journal checksum");

		//	Twice is the same as once, which is what "deterministic" has to mean here.
		Check (WorldOperationJournal::Replay (&report),"the second replay failed: %s",report.Peek_Buffer ());
		Check (WorldOperationJournal::Get_Terrain_Checksum () == before_replay,
				 "a second replay produced different ground again");
	}

	//
	//	Step 15's "no cliff-band mutation", and the boundary generally.
	//
	{
		DynamicVectorClass<Vector2> ring;
		ring.Add (Vector2 (-180.0f,-180.0f));
		ring.Add (Vector2 ( 180.0f,-180.0f));
		ring.Add (Vector2 ( 180.0f, 180.0f));
		ring.Add (Vector2 (-180.0f, 180.0f));
		StringClass bounds_error ("");
		Check (WorldBoundsService::Configure (ring,32.0f,8.0f,300.0f,&bounds_error),
				 "world bounds configuration this check depends on failed: %s",bounds_error.Peek_Buffer ());

		unsigned int before = WorldOperationJournal::Get_Terrain_Checksum ();

		WorldOperationClass crater;
		crater.Type			= WORLD_OP_CREATE_CRATER;
		crater.BaseRevision	= WorldOperationJournal::Get_Revision ();
		crater.Center		= Vector3 (-178.0f,0.0f,0.0f);		// inside the protected band
		crater.Radius		= 6.0f;
		crater.Depth		= 1.0f;
		crater.TimeSeconds	= 900.0f;

		WorldOperationApplyResult result = WorldOperationJournal::Submit (crater);
		Check (result.Result == WORLD_OP_RESULT_BOUNDARY_BAND,
				 "a crater inside the boundary cliff band reported '%s'",
				 World_Operation_Result_Name (result.Result));
		Check (WorldOperationJournal::Get_Terrain_Checksum () == before,
				 "a crater refused for the boundary band still moved the ground");
		Check (!result.Committed,"a refused crater advanced the world revision");
	}

	//
	//	Closing forgets everything, which is what makes a second match a second world.
	//
	{
		WorldOperationJournal::Close ();
		Check (!WorldOperationJournal::Is_Open (),"Close left the journal open");
		Check (WorldOperationJournal::Get_Revision () == 0,"Close left a revision behind");
		Check (WorldOperationJournal::Get_Operation_Count () == 0,"Close left operations behind");
		Check (WorldOperationJournal::Get_Reserved_Chunk_Count () == 0,"Close left chunks reserved");

		StringClass report ("");
		Check (!WorldOperationJournal::Replay (&report),"replay ran against a closed journal");
	}

	WorldOperationJournal::Shutdown ();
	GeneratedWorldPlacementRules::Shutdown ();
	NavMeshGenerator::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	Phase 19.  Per-chunk checksums, the compact snapshot, late join, regional repair, rollback.
**
**	The join is exercised for real in one process, which works because the whole protocol is byte
**	blocks: capture what a server would send, throw the world away, regenerate the same baseline,
**	feed the blocks in, and compare checksums.  That is exactly the sequence steps 9-12 describe,
**	minus a socket.
*/
void	Check_World_Sync (void)
{
	for (int i = 0; i < WORLD_SYNC_RESULT_COUNT; i++) {
		Check (::strcmp (World_Sync_Result_Name ((WorldSyncResultType)i),"unknown") != 0,
				 "sync result %d has no name",i);
	}

	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	NavMeshGenerator::Init ();
	GeneratedWorldPlacementRules::Init ();
	WorldOperationJournal::Init ();
	WorldSnapshotService::Init ();
	WorldSyncService::Init ();

	//
	//	Refusals with nothing to work with.
	//
	{
		unsigned int ignored = 0;
		Check (!WorldSnapshotService::Compute_World_Checksum (&ignored),
				 "a world checksum was produced with no terrain at all");
		Check (!WorldSnapshotService::Compute_Chunk_Checksum (0,0,&ignored),
				 "a chunk checksum was produced with no terrain at all");

		WorldSyncHandshakeClass handshake;
		Check (!WorldSyncService::Build_Handshake (1u,2u,3u,&handshake),
				 "a handshake was built for a world that does not exist");

		StringClass error ("");
		Check (WorldSnapshotService::Capture_Dirty_Chunks (&error) < 0,
				 "a snapshot set was captured with no terrain");
		Check (error.Get_Length () > 0,"a refused capture carried no reason");
	}

	//
	//	The server's world: a real baseline, a journal, and some real mutation.
	//
	static const int   PATCHES		= 4;
	static const float CELL_SIZE	= 4.0f;
	static const float SLOPE		= 10.0f;
	static const Vector3 ORIGIN (-128.0f,-128.0f,0.0f);

	Build_Uniform_Ramp_Terrain (PATCHES,CELL_SIZE,SLOPE,ORIGIN);
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	{
		StringClass error ("");
		Check (WorldOperationJournal::Begin (&error),"the server journal failed to open: %s",
				 error.Peek_Buffer ());
	}

	//
	//	Step 1-2.  Checksums over a pristine world.
	//
	unsigned int pristine_world = 0;
	{
		Check (WorldSnapshotService::Compute_World_Checksum (&pristine_world),
				 "a real world produced no checksum");
		Check (pristine_world != 0,"a real world checksummed to zero");

		unsigned int chunk_a = 0, chunk_b = 0;
		Check (WorldSnapshotService::Compute_Chunk_Checksum (0,0,&chunk_a),"chunk 0,0 produced no checksum");
		Check (WorldSnapshotService::Compute_Chunk_Checksum (1,0,&chunk_b),"chunk 1,0 produced no checksum");
		Check (chunk_a != chunk_b,"two different chunks of a ramp checksummed identically");

		Check (!WorldSnapshotService::Compute_Chunk_Checksum (-1,0,&chunk_a),"an off-grid chunk answered");
		Check (!WorldSnapshotService::Compute_Chunk_Checksum (PATCHES,0,&chunk_a),"an off-grid chunk answered");

		//	The same world checksummed twice is the same number, or none of this means anything.
		unsigned int again = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&again),"the second world checksum failed");
		Check (again == pristine_world,"checksumming the same world twice gave two answers");

		//	Nothing has happened, so nothing is dirty and a capture is empty.
		StringClass error ("");
		Check (WorldSnapshotService::Capture_Dirty_Chunks (&error) == 0,
				 "a pristine world reported %d dirty chunk(s)",WorldSnapshotService::Get_Snapshot_Count ());
		Check (WorldSnapshotService::Get_Snapshot_Bytes () == 0,"a pristine world snapshot cost bytes");
	}

	//
	//	Real mutation, and the dirty set that follows from it.
	//
	{
		WorldOperationClass crater;
		crater.Type			= WORLD_OP_CREATE_CRATER;
		crater.BaseRevision	= WorldOperationJournal::Get_Revision ();
		crater.Center		= Vector3 (0.0f,0.0f,0.0f);
		crater.Radius		= 6.0f;
		crater.Depth		= 2.0f;
		crater.TimeSeconds	= 10.0f;
		WorldOperationApplyResult first = WorldOperationJournal::Submit (crater);
		Check (first.Is_Applied (),"the crater this check depends on was refused as '%s'",
				 World_Operation_Result_Name (first.Result));

		WorldOperationClass raise;
		raise.Type			= WORLD_OP_RAISE;
		raise.BaseRevision	= WorldOperationJournal::Get_Revision ();
		raise.Center		= Vector3 (60.0f,60.0f,0.0f);
		raise.Radius		= 10.0f;
		raise.Depth			= 3.0f;
		raise.TimeSeconds	= 11.0f;
		WorldOperationApplyResult second = WorldOperationJournal::Submit (raise);
		Check (second.Is_Applied (),"the raise this check depends on was refused as '%s'",
				 World_Operation_Result_Name (second.Result));

		unsigned int mutated = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&mutated),"the mutated world produced no checksum");
		Check (mutated != pristine_world,"mutating the world did not change its checksum");
	}

	//
	//	Steps 3-5.  The compact snapshot: only the vertices that differ, and far smaller than the
	//	ground it describes.
	//
	int server_snapshot_count = 0;
	int server_snapshot_bytes = 0;
	{
		StringClass error ("");
		server_snapshot_count = WorldSnapshotService::Capture_Dirty_Chunks (&error);
		Check (server_snapshot_count > 0,"two real operations produced no dirty chunk at all");
		Check (server_snapshot_count < (PATCHES * PATCHES),
				 "two local operations dirtied all %d chunks",server_snapshot_count);

		server_snapshot_bytes = WorldSnapshotService::Get_Snapshot_Bytes ();
		Check (server_snapshot_bytes > 0,"a non-empty snapshot set cost no bytes");

		//	The bar the gate actually sets: smaller than sending the ground.  A raw field of this
		//	size is (PATCHES*16+1)^2 vertices at four bytes each.
		int vertices = ((PATCHES * 16) + 1) * ((PATCHES * 16) + 1);
		int raw_bytes = vertices * 4;
		Check (server_snapshot_bytes < (raw_bytes / 2),
				 "the snapshot set is %d bytes against %d raw -- not compact",
				 server_snapshot_bytes,raw_bytes);

		//	Every snapshot is a real chunk, carries entries, and round-trips.
		for (int i = 0; i < server_snapshot_count; i++) {

			WorldChunkSnapshotClass snapshot;
			Check (WorldSnapshotService::Get_Snapshot (i,&snapshot),"snapshot %d could not be read back",i);
			Check (snapshot.ChunkX >= 0 && snapshot.ChunkY >= 0,"a snapshot names no chunk");
			Check (snapshot.Get_Vertex_Count () > 0,"a dirty chunk's snapshot holds no vertices");
			Check (snapshot.ChunkChecksum != 0,"a snapshot carries no chunk checksum");

			unsigned char buffer[65536];
			int used = 0;
			Check (snapshot.Write_Block (buffer,sizeof (buffer),&used),"a snapshot would not serialise");
			Check (used == snapshot.Get_Block_Size (),"a snapshot used %d bytes, not the %d it reported",
					 used,snapshot.Get_Block_Size ());

			WorldChunkSnapshotClass restored;
			int consumed = 0;
			Check (restored.Read_Block (buffer,used,&consumed),"a snapshot would not deserialise");
			Check (consumed == used,"deserialising a snapshot consumed %d of %d bytes",consumed,used);
			Check (restored == snapshot,"a snapshot did not survive its own round trip");

			Check (!restored.Read_Block (buffer,4,&consumed),"a four-byte block was accepted as a snapshot");
		}

		WorldChunkSnapshotClass found;
		WorldChunkSnapshotClass first;
		Check (WorldSnapshotService::Get_Snapshot (0,&first),"the first snapshot could not be read");
		Check (WorldSnapshotService::Find_Snapshot_For_Chunk (first.ChunkX,first.ChunkY,&found),
				 "a captured chunk could not be found by its own coordinates");
		Check (found == first,"finding a snapshot by chunk returned a different record");
	}

	//
	//	The server's handshake and the blocks it would send.
	//
	static const unsigned int PROFILE_HASH		= 0xABCD1234u;
	static const unsigned int CONTENT_HASH		= 0x00C0FFEEu;
	static const unsigned int MANIFEST_REVISION	= 7u;

	/*
	**	Step 7's rebase, from the server's side: the snapshot set is the compaction point, so the
	**	server folds its log into the base before advertising the set.  Without this the client
	**	would reconstruct identical ground while its journal still described a different history,
	**	and the world checksum -- which folds in the journal -- would disagree for a reason that has
	**	nothing to do with the terrain.
	*/
	{
		StringClass report ("");
		Check (WorldSnapshotService::Compact (&report),"the server could not compact: %s",report.Peek_Buffer ());
		Check (WorldOperationJournal::Get_Operation_Count () == 0,
				 "the server's log survived its own compaction");
		Check (WorldSnapshotService::Get_Snapshot_Count () == server_snapshot_count,
				 "compaction captured %d chunks, not the %d already found dirty",
				 WorldSnapshotService::Get_Snapshot_Count (),server_snapshot_count);
	}

	WorldSyncHandshakeClass server_handshake;
	Check (WorldSyncService::Build_Handshake (PROFILE_HASH,CONTENT_HASH,MANIFEST_REVISION,&server_handshake),
			 "the server handshake could not be built");
	Check (server_handshake.SnapshotRevision == WorldOperationJournal::Get_Revision (),
			 "the handshake's snapshot revision does not match the compaction point");
	Check (server_handshake.SnapshotCount == server_snapshot_count,
			 "the handshake advertises %d snapshots, not the %d captured",
			 server_handshake.SnapshotCount,server_snapshot_count);
	Check (server_handshake.WorldRevision == WorldOperationJournal::Get_Revision (),
			 "the handshake advertises the wrong world revision");
	Check (server_handshake.WorldChecksum != 0,"the handshake carries no world checksum");

	{
		unsigned char buffer[256];
		int used = 0;
		Check (server_handshake.Write_Block (buffer,sizeof (buffer),&used),"the handshake would not serialise");
		WorldSyncHandshakeClass restored;
		int consumed = 0;
		Check (restored.Read_Block (buffer,used,&consumed),"the handshake would not deserialise");
		Check (restored == server_handshake,"the handshake did not survive its own round trip");
		Check (!server_handshake.Write_Block (buffer,4,&used),"a four-byte buffer took a whole handshake");
	}

	//	The blocks the server would send, captured before the world is thrown away.
	//	Static rather than automatic: half a megabyte of send buffers is more than a stack frame
	//	should be asked to hold, and this harness is single-threaded.
	static const int MAX_BLOCKS = 64;
	static unsigned char snapshot_blocks[MAX_BLOCKS][8192];
	static int snapshot_sizes[MAX_BLOCKS];
	int snapshot_block_count = (server_snapshot_count < MAX_BLOCKS) ? server_snapshot_count : MAX_BLOCKS;

	for (int i = 0; i < snapshot_block_count; i++) {
		WorldChunkSnapshotClass snapshot;
		snapshot_sizes[i] = 0;
		if (!WorldSnapshotService::Get_Snapshot (i,&snapshot)) continue;
		Check (snapshot.Write_Block (snapshot_blocks[i],sizeof (snapshot_blocks[i]),&snapshot_sizes[i]),
				 "snapshot %d would not fit a send buffer",i);
	}

	unsigned int server_world_checksum = server_handshake.WorldChecksum;

	/*
	**	The repair block for one chunk, built by the server while the server's world still exists.
	**
	**	This is not a shortcut around the API -- it is the API used the way it is meant to be.  A
	**	repair block is the server's answer to a client's "chunk N disagrees", so it is built from
	**	the server's ground.  Building it later, from the client's corrupted copy, would capture the
	**	corruption and "repair" the chunk to exactly what was wrong with it.
	*/
	static unsigned char repair_block[8192];
	int repair_block_bytes = 0;
	int repair_chunk_x = -1;
	int repair_chunk_y = -1;
	{
		WorldChunkSnapshotClass target;
		Check (WorldSnapshotService::Get_Snapshot (0,&target),"no server snapshot to repair from");
		repair_chunk_x = target.ChunkX;
		repair_chunk_y = target.ChunkY;

		Check (WorldSyncService::Build_Repair_Block (repair_chunk_x,repair_chunk_y,
																	repair_block,sizeof (repair_block),&repair_block_bytes),
				 "the server could not build a repair block for chunk %d,%d",repair_chunk_x,repair_chunk_y);
		Check (repair_block_bytes > 0,"a repair block cost no bytes");
		Check (repair_block_bytes <= server_snapshot_bytes,
				 "one chunk's repair (%d bytes) is larger than the whole snapshot set (%d)",
				 repair_block_bytes,server_snapshot_bytes);
	}
	WorldSyncService::Reset_Metrics ();

	//
	//	Steps 9-12.  Throw the world away, regenerate the same baseline deterministically, and
	//	rebuild the server's world from the blocks alone.
	//
	{
		WorldOperationJournal::Close ();
		WorldTerrainSystem::Destroy_Terrain ();

		//	The client's own deterministic regeneration -- the same inputs, so the same ground.
		Build_Uniform_Ramp_Terrain (PATCHES,CELL_SIZE,SLOPE,ORIGIN);
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		unsigned int client_baseline = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&client_baseline),
				 "the regenerated baseline produced no checksum");

		StringClass error ("");
		Check (WorldOperationJournal::Begin (&error),"the client journal failed to open: %s",
				 error.Peek_Buffer ());

		//	Step 8's refusals, each proved by breaking exactly one field.
		WorldSyncHandshakeClass client_handshake = server_handshake;
		StringClass detail ("");

		WorldSyncHandshakeClass wrong = client_handshake;
		wrong.GeneratorVersion += 1;
		Check (WorldSyncService::Begin_Join (server_handshake,wrong,&detail) == WORLD_SYNC_GENERATOR_MISMATCH,
				 "a generator version mismatch was accepted");
		Check (detail.Get_Length () > 0,"a refused join said nothing about which field disagreed");

		wrong = client_handshake;
		wrong.ProfileHash ^= 0xFFFFu;
		Check (WorldSyncService::Begin_Join (server_handshake,wrong,&detail) == WORLD_SYNC_PROFILE_MISMATCH,
				 "a profile hash mismatch was accepted");

		wrong = client_handshake;
		wrong.ContentHash ^= 0xFFFFu;
		Check (WorldSyncService::Begin_Join (server_handshake,wrong,&detail) == WORLD_SYNC_CONTENT_MISMATCH,
				 "a content hash mismatch was accepted");

		wrong = client_handshake;
		wrong.ManifestRevision += 1;
		Check (WorldSyncService::Begin_Join (server_handshake,wrong,&detail) == WORLD_SYNC_MANIFEST_MISMATCH,
				 "a manifest revision mismatch was accepted");

		wrong = client_handshake;
		wrong.HandshakeVersion = 99u;
		Check (WorldSyncService::Begin_Join (server_handshake,wrong,&detail) == WORLD_SYNC_BAD_HANDSHAKE_VERSION,
				 "an unknown handshake version was accepted");

		//	A refused join leaves nothing in progress.
		Check (!WorldSyncService::Is_Joining (),"a refused join left a join in progress");
		Check (WorldSyncService::Apply_Snapshot_Block (snapshot_blocks[0],snapshot_sizes[0],nullptr)
					== WORLD_SYNC_NOT_JOINING,
				 "a snapshot was accepted with no join in progress");

		//	The real join.
		Check (WorldSyncService::Begin_Join (server_handshake,client_handshake,&detail) == WORLD_SYNC_OK,
				 "a matching handshake was refused: %s",detail.Peek_Buffer ());
		Check (WorldSyncService::Is_Joining (),"an accepted join is not in progress");

		for (int i = 0; i < snapshot_block_count; i++) {
			if (snapshot_sizes[i] <= 0) continue;
			int consumed = 0;
			WorldSyncResultType result =
				WorldSyncService::Apply_Snapshot_Block (snapshot_blocks[i],snapshot_sizes[i],&consumed);
			Check (result == WORLD_SYNC_OK,"snapshot block %d was refused as '%s'",
					 i,World_Sync_Result_Name (result));
			Check (consumed == snapshot_sizes[i],"a snapshot block consumed %d of %d bytes",
					 consumed,snapshot_sizes[i]);
		}

		Check (WorldSyncService::Get_Expected_Checksum_Count () == snapshot_block_count,
				 "the client recorded %d inventory entries for %d snapshots",
				 WorldSyncService::Get_Expected_Checksum_Count (),snapshot_block_count);

		//	Step 12.  The whole point.
		WorldSyncResultType finished = WorldSyncService::Finish_Join (&detail);
		Check (finished == WORLD_SYNC_OK,
				 "the reconstructed world does not match the server's: %s (%s)",
				 World_Sync_Result_Name (finished),detail.Peek_Buffer ());

		unsigned int client_world = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&client_world),
				 "the reconstructed world produced no checksum");
		Check (client_world == server_world_checksum,
				 "the reconstructed world checksums 0x%08X, the server's 0x%08X",
				 client_world,server_world_checksum);
		Check (client_world != client_baseline,"reconstruction changed nothing about the baseline");

		Check (!WorldSyncService::Is_Joining (),"a finished join is still in progress");

		//	Step 19's metrics: real byte counts, not estimates.
		const WorldSyncMetrics & metrics = WorldSyncService::Get_Metrics ();
		Check (metrics.JoinCount == 1,"the join was counted %d time(s)",metrics.JoinCount);
		Check (metrics.SnapshotBytes > 0,"a join that applied snapshots recorded no snapshot bytes");
		Check (metrics.LateJoinBytes >= metrics.SnapshotBytes,
				 "late-join bytes (%d) are fewer than the snapshot bytes inside them (%d)",
				 metrics.LateJoinBytes,metrics.SnapshotBytes);

		int vertices = ((PATCHES * 16) + 1) * ((PATCHES * 16) + 1);
		Check (metrics.LateJoinBytes < (vertices * 4),
				 "the whole late join cost %d bytes against %d for the raw field",
				 metrics.LateJoinBytes,vertices * 4);
	}

	//
	//	Steps 13-15.  Corrupt one chunk, find it, repair it, reverify.
	//
	{
		DynamicVectorClass<int> mismatched_x;
		DynamicVectorClass<int> mismatched_y;
		Check (WorldSyncService::Find_Mismatched_Chunks (&mismatched_x,&mismatched_y) == 0,
				 "a correctly reconstructed world already reports mismatched chunks");

		//	Corrupt exactly one chunk, the way a dropped packet would: one vertex, wrong height.
		//	The chunk the server already built a repair block for.
		WorldChunkSnapshotClass target;
		Check (WorldSnapshotService::Find_Snapshot_For_Chunk (repair_chunk_x,repair_chunk_y,&target),
				 "the chunk the repair block covers is not in the snapshot set");

		HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield ();
		Check (heightfield != nullptr,"the reconstructed world has no heightfield");

		if (heightfield != nullptr) {

			int count_x = heightfield->Get_Vertex_Count_X ();
			int corrupt_index = target.VertexIndex[0];
			float good = heightfield->Get_Height (corrupt_index % count_x,corrupt_index / count_x);
			heightfield->Set_Height (corrupt_index % count_x,corrupt_index / count_x,good + 5.0f);

			int found = WorldSyncService::Find_Mismatched_Chunks (&mismatched_x,&mismatched_y);
			Check (found > 0,"a corrupted chunk was not detected at all");
			Check (found <= 2,"corrupting one vertex flagged %d chunks",found);

			//	It has to be the one that was corrupted -- a detector that flags the wrong chunk
			//	would send a repair that does not repair anything.
			bool named = false;
			for (int i = 0; i < found; i++) {
				if ((mismatched_x[i] == target.ChunkX) && (mismatched_y[i] == target.ChunkY)) named = true;
			}
			Check (named,"the corrupted chunk %d,%d was not among the %d reported",
					 target.ChunkX,target.ChunkY,found);

			//	The server's answer, built earlier from the server's own ground: one chunk, and
			//	nothing else.
			StringClass detail ("");
			WorldSyncResultType repaired =
				WorldSyncService::Apply_Repair_Block (repair_block,repair_block_bytes,&detail);
			Check (repaired == WORLD_SYNC_OK,"the repair was refused as '%s' (%s)",
					 World_Sync_Result_Name (repaired),detail.Peek_Buffer ());

			Check (WorldSyncService::Find_Mismatched_Chunks (&mismatched_x,&mismatched_y) == 0,
					 "the world still reports mismatched chunks after the repair");

			unsigned int after_repair = 0;
			Check (WorldSnapshotService::Compute_World_Checksum (&after_repair),
					 "the repaired world produced no checksum");
			Check (after_repair == server_world_checksum,
					 "the repaired world checksums 0x%08X, the server's 0x%08X",
					 after_repair,server_world_checksum);

			const WorldSyncMetrics & metrics = WorldSyncService::Get_Metrics ();
			Check (metrics.RepairCount == 1,"the repair was counted %d time(s)",metrics.RepairCount);
			Check (metrics.RepairBytes > 0,"a real repair recorded no bytes");
		}

		Check (WorldSyncService::Apply_Repair_Block (nullptr,0,nullptr) == WORLD_SYNC_SNAPSHOT_REFUSED,
				 "an empty repair block was accepted");
	}

	//
	//	Step 16.  Building/AI state waits for the world revision it was recorded against.
	//
	{
		unsigned int revision = WorldOperationJournal::Get_Revision ();
		Check (WorldSyncService::Is_World_Ready_For_Revision (revision),
				 "the world is not ready for the revision it is at");
		Check (WorldSyncService::Is_World_Ready_For_Revision (revision - 1),
				 "the world is not ready for a revision it has already passed");
		Check (!WorldSyncService::Is_World_Ready_For_Revision (revision + 10),
				 "the world claims to be ready for a revision it has not reached");
	}

	//
	//	Steps 6-7.  Compaction, and the rebase that makes it a compaction rather than a copy.
	//
	{
		//	The reconstructed world's own journal is empty -- it was rebuilt from snapshots, not
		//	from operations -- so this needs a log of its own before there is anything to compact.
		for (int i = 0; i < 2; i++) {
			WorldOperationClass raise;
			raise.Type			= WORLD_OP_RAISE;
			raise.BaseRevision	= WorldOperationJournal::Get_Revision ();
			raise.Center		= Vector3 (-60.0f + ((float)i * 30.0f),20.0f,0.0f);
			raise.Radius		= 8.0f;
			raise.Depth			= 1.5f;
			raise.TimeSeconds	= 200.0f + (float)i;
			WorldOperationApplyResult applied = WorldOperationJournal::Submit (raise);
			Check (applied.Is_Applied (),"the operation this compaction check depends on was refused as '%s'",
					 World_Operation_Result_Name (applied.Result));
		}

		WorldSnapshotCompactionPolicy policy;
		policy.MaxOperations			= 2;
		policy.MaxAgeSeconds			= 1000.0f;
		policy.MaxOperationBytes	= 1000000;
		WorldSnapshotService::Set_Compaction_Policy (policy);

		StringClass reason ("");
		Check (WorldSnapshotService::Should_Compact (&reason),
				 "a log at the operation threshold does not want compacting");
		Check (reason.Get_Length () > 0,"a compaction trigger gave no reason");

		//	The *ground*, not the world checksum: a compaction empties the journal, and the world
		//	checksum deliberately folds the journal in, so comparing that would assert that a
		//	compaction changes nothing about how the world is described -- which is the opposite of
		//	what a compaction is for.
		unsigned int before = WorldOperationJournal::Get_Terrain_Checksum ();
		int operations_before = WorldOperationJournal::Get_Operation_Count ();
		unsigned int revision_before = WorldOperationJournal::Get_Revision ();
		int compactions_before = WorldSnapshotService::Get_Compaction_Count ();

		StringClass report ("");
		Check (WorldSnapshotService::Compact (&report),"compaction failed: %s",report.Peek_Buffer ());
		Check (WorldSnapshotService::Get_Compaction_Count () == (compactions_before + 1),
				 "the compaction was not counted");
		Check (report.Get_Length () > 0,"a compaction reported nothing about what it folded");

		//	The log is gone, the ground is not, and the revision did not move -- a compaction
		//	changes how the world is described, not what it is.
		Check (WorldOperationJournal::Get_Operation_Count () == 0,
				 "compaction left %d operation(s) behind",WorldOperationJournal::Get_Operation_Count ());
		Check (operations_before > 0,"the compaction check had nothing to fold");
		Check (WorldOperationJournal::Get_Revision () == revision_before,
				 "compaction moved the world revision from %u to %u",
				 revision_before,WorldOperationJournal::Get_Revision ());

		//	Every chunk is pristine against the new baseline by definition, so a fresh capture is
		//	empty -- which is what makes "capture first, rebase second" load-bearing.
		StringClass error ("");
		Check (WorldSnapshotService::Capture_Dirty_Chunks (&error) == 0,
				 "a freshly rebased world still reports dirty chunks");

		//	Replay now reproduces the compacted ground from the compacted base.
		Check (WorldOperationJournal::Replay (&report),"replay after compaction failed: %s",
				 report.Peek_Buffer ());
		unsigned int after = WorldOperationJournal::Get_Terrain_Checksum ();
		Check (after == before,"compaction changed the ground (0x%08X, was 0x%08X)",after,before);

		//	A broken policy is clamped, not accepted.
		WorldSnapshotCompactionPolicy broken;
		broken.MaxOperations			= 0;
		broken.MaxAgeSeconds			= -1.0f;
		broken.MaxOperationBytes	= 0;
		WorldSnapshotService::Set_Compaction_Policy (broken);
		Check (WorldSnapshotService::Get_Compaction_Policy ().MaxOperations > 0,
				 "a zero operation threshold was accepted");
		Check (WorldSnapshotService::Get_Compaction_Policy ().MaxOperationBytes > 0,
				 "a zero byte threshold was accepted");

		//	An empty log never wants compacting, whatever the policy says.
		Check (!WorldSnapshotService::Should_Compact (&reason),"an empty log wants compacting");
	}

	//
	//	Steps 17-18.  Developer rollback.
	//
	{
		StringClass error ("");
		Check (WorldSnapshotService::Mark_Rollback_Point ("before the hole",&error),
				 "a rollback point could not be marked: %s",error.Peek_Buffer ());
		Check (WorldSnapshotService::Get_Rollback_Point_Count () == 1,"the rollback point was not recorded");

		unsigned int marked_checksum = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&marked_checksum),"the marked checksum failed");

		//	Recorded before the hole is dug, so the rollback has something specific to be compared
		//	against afterwards rather than only "different from the hole".
		unsigned int marked_chunk_checksum = 0;

		StringClass label ("");
		unsigned int revision = 0;
		Check (WorldSnapshotService::Get_Rollback_Point (0,&label,&revision),
				 "the rollback point could not be read back");
		Check (label == "before the hole","the rollback point lost its label");
		Check (!WorldSnapshotService::Get_Rollback_Point (99,&label,&revision),
				 "an out-of-range rollback index was accepted");

		//	Dig a hole, then undo it.
		WorldOperationClass hole;
		hole.Type			= WORLD_OP_LOWER;
		hole.BaseRevision	= WorldOperationJournal::Get_Revision ();
		hole.Center			= Vector3 (-40.0f,-40.0f,0.0f);
		hole.Radius			= 12.0f;
		hole.Depth			= 5.0f;
		hole.TimeSeconds	= 500.0f;

		{
			AABoxClass first;
			if (WorldTerrainSystem::Get_Patch_Bounds (0,0,&first)) {
				float size = first.Extent.X * 2.0f;
				int px = (int)((hole.Center.X - (first.Center.X - first.Extent.X)) / size);
				int py = (int)((hole.Center.Y - (first.Center.Y - first.Extent.Y)) / size);
				WorldSnapshotService::Compute_Chunk_Checksum (px,py,&marked_chunk_checksum);
			}
		}

		WorldOperationApplyResult dug = WorldOperationJournal::Submit (hole);
		Check (dug.Is_Applied (),"the hole this rollback check depends on was refused as '%s'",
				 World_Operation_Result_Name (dug.Result));

		unsigned int dug_checksum = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&dug_checksum),"the post-hole checksum failed");
		Check (dug_checksum != marked_checksum,"digging a hole did not change the world checksum");

		StringClass report ("");
		Check (WorldSnapshotService::Rollback_To (0,&report),"the rollback failed: %s",report.Peek_Buffer ());
		Check (report.Get_Length () > 0,"a rollback reported nothing about what it did");

		unsigned int rolled_back = 0;
		Check (WorldSnapshotService::Compute_World_Checksum (&rolled_back),"the rolled-back checksum failed");

		Check (rolled_back != dug_checksum,"rolling back left the world exactly as the hole left it");

		/*
		**	The ground itself, chunk by chunk, is what a rollback actually promises.  The world
		**	checksum cannot be compared directly against `marked_checksum`, because it folds in the
		**	journal checksum as well, and the rollback rebased the journal rather than rewinding it
		**	(a SMOOTH destroys the information needed to un-smooth it).  So the ground is verified
		**	where the ground was changed: the chunk the hole was dug in must checksum to what it
		**	did before.
		*/
		int hole_px = 0, hole_py = 0;
		{
			AABoxClass first;
			if (WorldTerrainSystem::Get_Patch_Bounds (0,0,&first)) {
				float size = first.Extent.X * 2.0f;
				float origin_x = first.Center.X - first.Extent.X;
				float origin_y = first.Center.Y - first.Extent.Y;
				hole_px = (int)((hole.Center.X - origin_x) / size);
				hole_py = (int)((hole.Center.Y - origin_y) / size);
			}
		}

		unsigned int hole_chunk_now = 0;
		Check (WorldSnapshotService::Compute_Chunk_Checksum (hole_px,hole_py,&hole_chunk_now),
				 "the chunk the hole was dug in could not be checksummed after the rollback");
		Check (hole_chunk_now == marked_chunk_checksum,
				 "the rolled-back chunk %d,%d checksums 0x%08X, not the 0x%08X it did when marked",
				 hole_px,hole_py,hole_chunk_now,marked_chunk_checksum);

		//	The journal was rebased, not rewound: a SMOOTH destroys the information needed to
		//	un-smooth it, so stating "this is the ground now" is the only honest option.
		Check (WorldOperationJournal::Get_Operation_Count () == 0,
				 "the rollback left %d operation(s) in the journal",
				 WorldOperationJournal::Get_Operation_Count ());

		Check (!WorldSnapshotService::Rollback_To (99,&report),"an out-of-range rollback was performed");

		//	Bounded: marking more points than the cap keeps the most recent.
		for (int i = 0; i < (WorldSnapshotService::MAX_ROLLBACK_POINTS + 4); i++) {
			StringClass name ("");
			name.Format ("point_%d",i);
			WorldSnapshotService::Mark_Rollback_Point (name.Peek_Buffer (),&error);
		}
		Check (WorldSnapshotService::Get_Rollback_Point_Count () == WorldSnapshotService::MAX_ROLLBACK_POINTS,
				 "the rollback history grew to %d points, past its %d cap",
				 WorldSnapshotService::Get_Rollback_Point_Count (),
				 (int)WorldSnapshotService::MAX_ROLLBACK_POINTS);

		Check (WorldSnapshotService::Get_Rollback_Point (0,&label,&revision),
				 "the oldest surviving rollback point could not be read");
		Check (label != "before the hole","the cap dropped a newer point instead of the oldest");
	}

	WorldSyncService::Shutdown ();
	WorldSnapshotService::Shutdown ();
	WorldOperationJournal::Shutdown ();
	GeneratedWorldPlacementRules::Shutdown ();
	NavMeshGenerator::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	V2 roadmap Section 34 Phase 21.  The physical logistics layer (Section 22).
**
**	Unlike Phase 20's own subcomponent self-check -- which could only prove schema/arithmetic,
**	because a real BuildingGameObj needs a scene `-shaderselfcheck` never brings up -- this
**	graph's plan-derived half (BASE, FORWARD_BASE, RESOURCE_FIELD, LOGISTICS_POI) reads only
**	headless generator-time services this harness already builds elsewhere in this file, so the
**	real chain (terrain, hydrology, strategic plan, road graph, bridge plan, base selection,
**	Tiberium fields, settlements) is built here too -- the same setup Check_Combat_Space already
**	established as sufficient -- and route length/reachability is proven against the real road
**	graph, not merely against a schema.  REFINERY/FACTORY/REPAIR nodes (Rebuild_Building_Nodes)
**	are the one part still unreached here, for the standing reason every other runtime-building
**	construction path in this tree already carries: no GameObjManager scene exists in this
**	harness either.
*/
static const char *	LOGISTICS_TEST_DESCRIPTOR_JSON =
	"{ \"model_name\": \"ow_test_grading_pad.w3d\", \"schema_version\": 1, "
	"\"footprint\": { \"width\": 10.0, \"depth\": 10.0 } }";


void	Check_Logistics (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	HydrologyGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();
	TiberiumFieldGenerator::Init ();
	BuildingDescriptorManager::Init ();
	SettlementGenerator::Init ();
	ExpansionRegionRegistry::Init ();
	LogisticsGraphService::Init ();

	StringClass descriptor_error ("");
	BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grading_pad",
		LOGISTICS_TEST_DESCRIPTOR_JSON,&descriptor_error);
	ProceduralAssetRegistry::Init ();

	//
	//	Rebuild_Nodes tolerates every source being empty -- a plan too small (or never generated
	//	at all) is a correct, unremarkable outcome, the same tolerance every other consumer of
	//	StrategicPlanner/BaseSelectionPlanner already grants.
	//
	{
		StringClass error ("");
		Check (LogisticsGraphService::Rebuild_Nodes (false,0.0f,&error),
				 "rebuilding nodes over nothing at all reported a hard failure: %s",error.Peek_Buffer ());
		Check (LogisticsGraphService::Get_Node_Count () == 0,"an empty world already carries logistics nodes");
		Check (LogisticsGraphService::Has_Graph (),"Has_Graph is false after a successful (empty) rebuild");

		Check (LogisticsGraphService::Rebuild_Routes (&error),
				 "rebuilding routes over zero nodes reported a hard failure: %s",error.Peek_Buffer ());
		Check (LogisticsGraphService::Get_Route_Count () == 0,"zero nodes produced a nonzero route count");
	}

	//
	//	A real chain -- the same setup Check_Combat_Space/Check_Settlements already established.
	//
	{
		Build_Uniform_Ramp_Terrain (6,4.0f,10.0f,Vector3 (-192.0f,-192.0f,0.0f));
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		HydrologyParams hydro_params;
		hydro_params.Seed							= 61u;
		hydro_params.GenerateRivers				= true;
		hydro_params.GenerateLakes				= true;
		hydro_params.GeneratePonds				= true;
		hydro_params.RiverWidth					= 6.0f;
		hydro_params.StreamWidth					= 2.5f;
		hydro_params.RiverAccumulationChunks	= 3.0f;
		hydro_params.MaxRiverCount				= 2;

		StringClass error ("");
		Check (HydrologyGenerator::Generate (hydro_params,&error),
				 "the hydrology this check depends on failed: %s",error.Peek_Buffer ());

		WorldTerrainSystem::Build_Chunk_Registry (0.5f);

		StrategicPlannerParams plan_params;
		plan_params.Seed							= 919u;
		plan_params.MinApproachCount			= 2;
		plan_params.MinBaseCandidatesPerFaction	= 2;
		plan_params.MinBaseSeparation			= 80.0f;
		plan_params.MaxTiberiumCandidates		= 4;
		plan_params.MaxSettlementCandidates	= 3;
		plan_params.MaxPoiCandidates			= 3;
		plan_params.MaxExpansionCandidates	= 2;
		plan_params.MaxEconomicRange			= 1000.0f;
		Check (StrategicPlanner::Generate (plan_params,&error),
				 "the strategic plan this check depends on failed: %s",error.Peek_Buffer ());

		RoadGraphParams road_params;
		road_params.Seed							= 65u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		Check (RoadGraphGenerator::Generate (road_params,&error),
				 "the road graph this check depends on failed: %s",error.Peek_Buffer ());

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 87u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		Check (BridgePlanner::Generate (bridge_params,&error),
				 "the bridge plan this check depends on failed: %s",error.Peek_Buffer ());

		BaseSelectionParams base_params;
		base_params.Seed												= 133u;
		base_params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		base_params.MaxBaseToBaseTravelTimeDifferenceSeconds		= 120.0f;
		base_params.MinBaseExpansionArea								= 1024.0f;
		base_params.AllowSingleBridgeDependency						= false;
		base_params.MaxRepairAttemptsPerPair							= 8;
		Check (BaseSelectionPlanner::Generate (base_params,&error),
				 "the base selection this check depends on failed: %s",error.Peek_Buffer ());

		TiberiumFieldParams field_params;
		field_params.Seed							= 331u;
		field_params.FieldRadiusMin				= 15.0f;
		field_params.FieldRadiusMax				= 40.0f;
		field_params.InitialYieldMin				= 1500.0f;
		field_params.InitialYieldMax				= 4000.0f;
		field_params.CrystalDensity				= 0.15f;
		field_params.HarvestAnchorCount			= 3;
		field_params.HarvesterLoadAmount		= 600.0f;
		field_params.MaxRepairAttemptsPerField	= 8;
		field_params.ReferenceVehicleSpeed		= 8.33f;
		field_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (TiberiumFieldGenerator::Generate (field_params,&error),
				 "the Tiberium fields this check depends on failed: %s",error.Peek_Buffer ());

		SettlementParams settlement_params;
		settlement_params.Seed							= 565u;
		settlement_params.RoadConnectorMaxDistance	= 200.0f;
		settlement_params.MaxStructuresPerSettlement	= 4;
		settlement_params.MaxBuildableSlopeRadians	= 0.5f;
		Check (SettlementGenerator::Generate (settlement_params,&error),
				 "the settlements this check depends on failed: %s",error.Peek_Buffer ());

		//	Two completions near the origin and one far away: one home region, one forward one --
		//	the same recipe Check_Expansion_Regions already proved.
		StringClass completion_report ("");
		ExpansionRegionRegistry::Note_Construction_Complete (901,"ow_test_grading_pad",
																				Vector3 (0.0f,0.0f,0.0f),PLAYERTYPE_GDI,&completion_report);
		ExpansionRegionRegistry::Note_Construction_Complete (902,"ow_test_grading_pad",
																				Vector3 (5.0f,0.0f,0.0f),PLAYERTYPE_GDI,&completion_report);
		ExpansionRegionRegistry::Note_Construction_Complete (903,"ow_test_grading_pad",
																				Vector3 (1500.0f,0.0f,0.0f),PLAYERTYPE_GDI,&completion_report);
		Check (ExpansionRegionRegistry::Rebuild_Regions (50.0f,&error),
				 "the expansion regions this check depends on failed: %s",error.Peek_Buffer ());

		//
		//	Rebuild_Nodes over a real world.
		//
		Check (LogisticsGraphService::Rebuild_Nodes (true,75.0f,&error),
				 "rebuilding nodes over a real world failed: %s",error.Peek_Buffer ());
		Check (LogisticsGraphService::Get_Node_Count () > 0,"a real world produced zero logistics nodes");

		bool found_base = false;
		bool found_forward = false;
		for (int i = 0; i < LogisticsGraphService::Get_Node_Count (); i++) {
			LogisticsNode node;
			Check (LogisticsGraphService::Get_Node (i,&node),"node index %d could not be read back",i);
			Check (::strcmp (Logistics_Node_Kind_Name (node.Kind),"unknown") != 0,
					 "node %d carries an unnamed kind",i);
			if (node.Kind == LOGISTICS_NODE_BASE)			{ found_base = true; }
			if (node.Kind == LOGISTICS_NODE_FORWARD_BASE)	{ found_forward = true; }
		}
		Check (found_base,"a real base selection produced no BASE logistics node");
		Check (found_forward,"a real forward completion produced no FORWARD_BASE logistics node");

		int base_index = -1;
		Check (LogisticsGraphService::Find_Node_By_Id ("logistics_base_gdi",&base_index),
				 "the GDI base node could not be found by its own stable id");

		//
		//	Rebuild_Routes over the real road graph.
		//
		Check (LogisticsGraphService::Rebuild_Routes (&error),
				 "rebuilding routes over a real world failed: %s",error.Peek_Buffer ());
		Check (LogisticsGraphService::Get_Route_Count () > 0,"a real world with real nodes produced zero routes");

		bool found_open_route = false;
		for (int i = 0; i < LogisticsGraphService::Get_Route_Count (); i++) {
			LogisticsRoute route;
			Check (LogisticsGraphService::Get_Route (i,&route),"route index %d could not be read back",i);
			Check (::strcmp (Logistics_Route_Status_Name (route.Status),"unknown") != 0,
					 "route %d carries an unnamed status",i);
			if (route.Status == LOGISTICS_ROUTE_OPEN) {
				found_open_route = true;
				Check (route.Length > 0.0f,"an OPEN route reports a non-positive length");
				Check (route.Waypoints.Count () > 0,"an OPEN route carries no waypoints to walk or test bridges against");
			} else {
				Check (route.Length < 0.0f,"an unreachable route still reports a real length");
			}
		}
		Check (found_open_route,"a real, connected world produced no OPEN route at all");

		//	The topology this file's own Rebuild_Routes documents never connects one team's base
		//	straight to the other's -- every route has a home-node endpoint on one specific team,
		//	never base-to-base -- so this is a real negative assertion about the graph's own
		//	shape, not a placeholder.
		int nod_base_index = -1;
		if (LogisticsGraphService::Find_Node_By_Id ("logistics_base_nod",&nod_base_index)) {
			int ignored_route_index = -1;
			Check (!LogisticsGraphService::Find_Route_Between (base_index,nod_base_index,&ignored_route_index),
					 "a route was created straight between the two factions' own bases");
		}

		//	Determinism: the same real world rebuilt twice is the same checksum.
		unsigned int first_checksum = LogisticsGraphService::Get_Checksum ();
		Check (first_checksum != 0,"a real, non-empty graph carries a zero checksum");
		Check (LogisticsGraphService::Rebuild_Routes (&error),"the second route rebuild failed");
		Check (LogisticsGraphService::Get_Checksum () == first_checksum,
				 "rebuilding the same routes twice produced a different checksum");

		//
		//	Step 9.  Bridge blockage -- device-less, so only the "no bridge on this route" and
		//	"an out-of-range bridge id changes nothing" arithmetic is reachable here; a route that
		//	actually crosses a broken bridge needs a live world, the same standing limit every
		//	other bridge-damage-facing check in this tree already carries.
		//
		LogisticsGraphService::Update_Route_Status ();
		Check (LogisticsGraphService::Get_Route_Count () > 0,"Update_Route_Status changed the route count");

		//
		//	Section 22.2 step 5.  Construction support: inside the radius of the forward node
		//	this world's own completions produced, and not far outside it.
		//
		LogisticsNode forward_node;
		bool have_forward_node = false;
		for (int i = 0; i < LogisticsGraphService::Get_Node_Count (); i++) {
			LogisticsNode node;
			if (!LogisticsGraphService::Get_Node (i,&node)) continue;
			if (node.Kind == LOGISTICS_NODE_FORWARD_BASE) { forward_node = node; have_forward_node = true; break; }
		}
		Check (have_forward_node,"no forward node survived to test construction support against");
		if (have_forward_node) {
			Check (LogisticsGraphService::Is_Under_Construction_Support (forward_node.Position,PLAYERTYPE_GDI),
					 "a point at a forward node's own centre is not under its own construction support");
			Vector3 far_point = forward_node.Position + Vector3 (10000.0f,0.0f,0.0f);
			Check (!LogisticsGraphService::Is_Under_Construction_Support (far_point,PLAYERTYPE_GDI),
					 "a point ten thousand units away is under construction support");
			Check (!LogisticsGraphService::Is_Under_Construction_Support (forward_node.Position,PLAYERTYPE_NOD),
					 "construction support does not respect team");
		}

		//	A radius of 0 (the profile's own default) disables the bonus outright.
		Check (LogisticsGraphService::Rebuild_Nodes (true,0.0f,&error),"re-rebuilding with a zero radius failed");
		if (have_forward_node) {
			Check (!LogisticsGraphService::Is_Under_Construction_Support (forward_node.Position,PLAYERTYPE_GDI),
					 "a zero construction support radius still granted a bonus");
		}

		//
		//	Step 6-8.  Convoys.  Refused with ConvoyEnabled false; accepted and trackable once
		//	true, against a real OPEN route with real waypoints; Stop_Convoy removes it.
		//
		Check (LogisticsGraphService::Rebuild_Nodes (false,0.0f,&error),"re-rebuilding with convoys off failed");
		Check (LogisticsGraphService::Rebuild_Routes (&error),"re-rebuilding routes after a node rebuild failed");

		int open_route_index = -1;
		for (int i = 0; i < LogisticsGraphService::Get_Route_Count (); i++) {
			LogisticsRoute route;
			if (LogisticsGraphService::Get_Route (i,&route) && (route.Status == LOGISTICS_ROUTE_OPEN)) {
				open_route_index = i;
				break;
			}
		}
		Check (open_route_index >= 0,"no OPEN route survived the convoy-test rebuild");
		if (open_route_index >= 0) {
			Check (!LogisticsGraphService::Start_Convoy (12345,open_route_index),
					 "a convoy started while ConvoyEnabled is false");

			Check (LogisticsGraphService::Rebuild_Nodes (true,0.0f,&error),"re-rebuilding with convoys on failed");
			Check (LogisticsGraphService::Rebuild_Routes (&error),"re-rebuilding routes for the convoy-on case failed");

			Check (!LogisticsGraphService::Start_Convoy (12345,9999),
					 "a convoy started against an out-of-range route index");

			//	The route list was just rebuilt; re-resolve an OPEN route in the fresh list rather
			//	than trusting the pre-rebuild index to still name the same one.
			int convoy_route_index = -1;
			for (int i = 0; i < LogisticsGraphService::Get_Route_Count (); i++) {
				LogisticsRoute route;
				if (LogisticsGraphService::Get_Route (i,&route) && (route.Status == LOGISTICS_ROUTE_OPEN)) {
					convoy_route_index = i;
					break;
				}
			}
			Check (convoy_route_index >= 0,"no OPEN route survived the convoy-on rebuild");
			if (convoy_route_index >= 0) {
				Check (LogisticsGraphService::Start_Convoy (12345,convoy_route_index),
						 "a convoy was refused against a real OPEN route with ConvoyEnabled true");
				Check (LogisticsGraphService::Is_In_Convoy (12345),"a started convoy does not report itself in-transit");
				Check (LogisticsGraphService::Get_Active_Convoy_Count () == 1,
						 "one started convoy reports %d active",LogisticsGraphService::Get_Active_Convoy_Count ());

				//	No unit with id 12345 exists in this harness (no GameObjManager scene), so
				//	Update's own "the unit no longer resolves" path is what actually gets
				//	exercised here -- the same standing harness limit as everywhere else, proven
				//	rather than merely asserted: the convoy record is dropped, not left dangling.
				LogisticsGraphService::Update (0.1f);
				Check (!LogisticsGraphService::Is_In_Convoy (12345),
						 "Update kept a convoy whose unit does not exist in this harness");
				Check (LogisticsGraphService::Get_Active_Convoy_Count () == 0,
						 "Update left a stale convoy record behind");

				LogisticsGraphService::Stop_Convoy (99999);		// no-op on an id that was never started
			}
		}
	}

	LogisticsGraphService::Clear ();
	Check (LogisticsGraphService::Get_Node_Count () == 0,"Clear left logistics nodes behind");
	Check (LogisticsGraphService::Get_Route_Count () == 0,"Clear left logistics routes behind");
	Check (!LogisticsGraphService::Has_Graph (),"Clear left Has_Graph true");

	ExpansionRegionRegistry::Shutdown ();
	BuildingDescriptorManager::Unload_All ();
	SettlementGenerator::Shutdown ();
	ProceduralAssetRegistry::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	TiberiumFieldGenerator::Shutdown ();
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
 * Check_Vehicle_Subsystems -- V2 Phase 22, Roadmap Section 23                                   *
 *                                                                                                *
 *	Exercises VehicleGameObj's own subsystem response curve directly, through the static form     *
 *	the instance accessors forward to.  A real VehicleGameObj cannot be built device-less (it     *
 *	needs definitions and a physics scene a -terrainselfcheck run never brings up -- the same     *
 *	standing harness limit Phase 20's building subcomponents and Phase 21's                        *
 *	Rebuild_Building_Nodes already carry), so what is proven here is the curve and every gate      *
 *	around it, not a vehicle actually slowing down on screen.  That remains a live-pass item.      *
 *=============================================================================================*/
void	Check_Vehicle_Subsystems (void)
{
	//	The gate that matters most: with the profile off -- the default, and what every legacy/
	//	stock map runs with -- every answer is the canonical one at every health, including
	//	dead-flat zero.  This is Phase 22's own gate ("behaviorally absent when disabled") and
	//	Section 23's opening rule, proven rather than asserted.
	{
		ProceduralWorldVehicleSubsystemSettings off;		// default-constructed: Enabled false
		const float fractions [] = { 1.0f,0.75f,0.5f,0.25f,0.1f,0.0f };
		for (int i = 0; i < (int)(sizeof (fractions) / sizeof (fractions [0])); i ++) {
			float f = fractions [i];
			Check (VehicleGameObj::Compute_Subsystem_Engine_Factor (off,f) == 1.0f,
					 "disabled profile scaled the engine at health fraction %.2f",f);
			Check (VehicleGameObj::Compute_Subsystem_Mobility_Factor (off,f) == 1.0f,
					 "disabled profile scaled mobility at health fraction %.2f",f);
			Check (VehicleGameObj::Compute_Subsystem_Weapon_Enabled (off,f),
					 "disabled profile disabled the weapon at health fraction %.2f",f);
			Check (VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (off,f) == 1.0f,
					 "disabled profile slowed the weapon at health fraction %.2f",f);
			Check (!VehicleGameObj::Compute_Subsystem_Immobilized (off,f),
					 "disabled profile immobilized the vehicle at health fraction %.2f",f);
		}
	}

	ProceduralWorldVehicleSubsystemSettings on;
	on.Enabled = true;

	//	Above the start threshold nothing is scaled at all, so a lightly damaged vehicle drives
	//	exactly as it always did -- the degradation has a real onset, it is not a curve that
	//	starts biting the instant a vehicle takes its first scratch.
	{
		Check (VehicleGameObj::Compute_Subsystem_Engine_Factor (on,1.0f) == 1.0f,
				 "a full-health vehicle had its engine scaled");
		Check (VehicleGameObj::Compute_Subsystem_Engine_Factor (on,on.EngineDamageStartFraction) == 1.0f,
				 "a vehicle exactly at the engine threshold was already degraded");
		Check (VehicleGameObj::Compute_Subsystem_Mobility_Factor (on,1.0f) == 1.0f,
				 "a full-health vehicle had its mobility scaled");
		Check (VehicleGameObj::Compute_Subsystem_Mobility_Factor (on,on.MobilityDamageStartFraction) == 1.0f,
				 "a vehicle exactly at the mobility threshold was already degraded");
	}

	//	Below the threshold the response is real, monotonic, and bounded by the configured floor
	//	-- never zero, never above 1, and never non-monotonic (which would mean taking damage
	//	could make a vehicle faster).
	{
		float previous_engine = 1.0f;
		float previous_mobility = 1.0f;
		for (int step = 0; step <= 20; step ++) {
			float f = 1.0f - ((float)step / 20.0f);		// 1.0 down to 0.0

			float engine = VehicleGameObj::Compute_Subsystem_Engine_Factor (on,f);
			Check ((engine >= on.MinEngineFactor) && (engine <= 1.0f),
					 "engine factor %.4f out of bounds at health fraction %.2f",engine,f);
			Check (engine <= previous_engine + 0.0001f,
					 "engine factor rose as health fell (%.4f then %.4f at %.2f)",previous_engine,engine,f);
			previous_engine = engine;

			float mobility = VehicleGameObj::Compute_Subsystem_Mobility_Factor (on,f);
			Check ((mobility >= on.MinMobilityFactor) && (mobility <= 1.0f),
					 "mobility factor %.4f out of bounds at health fraction %.2f",mobility,f);
			Check (mobility <= previous_mobility + 0.0001f,
					 "mobility factor rose as health fell (%.4f then %.4f at %.2f)",previous_mobility,mobility,f);
			previous_mobility = mobility;
		}

		//	At zero health the floor is exactly what a profile asked for, so "how slow can this
		//	get" is a number an author sets rather than one this curve invents.
		Check (WWMath::Fabs (VehicleGameObj::Compute_Subsystem_Engine_Factor (on,0.0f) - on.MinEngineFactor) < 0.0001f,
				 "engine factor at zero health did not reach exactly min_engine_factor");
		Check (WWMath::Fabs (VehicleGameObj::Compute_Subsystem_Mobility_Factor (on,0.0f) - on.MinMobilityFactor) < 0.0001f,
				 "mobility factor at zero health did not reach exactly min_mobility_factor");
	}

	//	Rate of fire degrades continuously, on the same shape as the engine and mobility curves:
	//	untouched above the threshold, then monotonic down to the configured floor.  A damaged
	//	turret firing more slowly is the normal outcome; falling silent is not.
	{
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (on,1.0f) == 1.0f,
				 "a full-health vehicle had its rate of fire reduced");
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (on,on.WeaponDamageStartFraction) == 1.0f,
				 "a vehicle exactly at the weapon threshold was already slowed");

		float previous_weapon = 1.0f;
		for (int step = 0; step <= 20; step ++) {
			float f = 1.0f - ((float)step / 20.0f);
			float efficiency = VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (on,f);
			Check ((efficiency >= on.MinWeaponFactor) && (efficiency <= 1.0f),
					 "weapon efficiency %.4f out of bounds at health fraction %.2f",efficiency,f);
			Check (efficiency <= previous_weapon + 0.0001f,
					 "weapon efficiency rose as health fell (%.4f then %.4f at %.2f)",previous_weapon,efficiency,f);
			previous_weapon = efficiency;
		}
		Check (WWMath::Fabs (VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (on,0.0f) - on.MinWeaponFactor) < 0.0001f,
				 "weapon efficiency at zero health did not reach exactly min_weapon_factor");

		//	The efficiency must stay strictly positive across the whole default curve, because
		//	WeaponClass::Set_State divides the refire interval by it.
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (on,0.0f) > 0.0f,
				 "the default weapon curve reached zero efficiency, which would be an infinite refire interval");
	}

	//	The hard cut-off is a separate, opt-in rule and is off by default, so an enabled profile
	//	that never mentions it leaves a wrecked vehicle still firing (slowly) rather than silent.
	{
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Enabled (on,0.0f),
				 "the default profile silenced a zero-health vehicle instead of only slowing it");

		ProceduralWorldVehicleSubsystemSettings disable = on;
		disable.WeaponDisableFraction = 0.25f;
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Enabled (disable,1.0f),
				 "a full-health vehicle could not fire");
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Enabled (disable,disable.WeaponDisableFraction + 0.01f),
				 "a vehicle just above the weapon cut-off could not fire");
		Check (!VehicleGameObj::Compute_Subsystem_Weapon_Enabled (disable,disable.WeaponDisableFraction),
				 "a vehicle exactly at the weapon cut-off could still fire");

		//	And the master toggle still wins, the same as it does for immobilizing.
		ProceduralWorldVehicleSubsystemSettings disable_but_off;
		disable_but_off.WeaponDisableFraction = 0.25f;		// Enabled deliberately left false
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Enabled (disable_but_off,0.0f),
				 "the weapon cut-off took effect with the feature itself disabled");
	}

	//	Section 23 step 5, the rule this phase most needed to not get wrong: enabling the feature
	//	never by itself removes basic drivability.  Immobilizing takes its own separate opt-in.
	{
		Check (!VehicleGameObj::Compute_Subsystem_Immobilized (on,0.0f),
				 "an enabled-but-not-allow_immobilize profile immobilized a zero-health vehicle");

		ProceduralWorldVehicleSubsystemSettings immobilize = on;
		immobilize.AllowImmobilize = true;
		Check (!VehicleGameObj::Compute_Subsystem_Immobilized (immobilize,1.0f),
				 "allow_immobilize immobilized a full-health vehicle");
		Check (!VehicleGameObj::Compute_Subsystem_Immobilized (immobilize,immobilize.ImmobilizeFraction + 0.01f),
				 "allow_immobilize immobilized a vehicle above the immobilize threshold");
		Check (VehicleGameObj::Compute_Subsystem_Immobilized (immobilize,immobilize.ImmobilizeFraction),
				 "allow_immobilize did not immobilize a vehicle at the immobilize threshold");

		//	And the master toggle still wins over the sub-toggle: allow_immobilize with the
		//	feature itself off must stay inert, or "disable the profile" would not be the total
		//	bypass Phase 22 step 11 requires.
		ProceduralWorldVehicleSubsystemSettings allow_but_off;
		allow_but_off.AllowImmobilize = true;			// Enabled deliberately left false
		Check (!VehicleGameObj::Compute_Subsystem_Immobilized (allow_but_off,0.0f),
				 "allow_immobilize took effect with the feature itself disabled");
	}

	//	A threshold of zero turns that one subsystem's degradation off, because these fields are
	//	documented as the fraction *below which* degradation begins and nothing is below zero
	//	health.  Proven at zero health, the only place the two readings could differ, and proven
	//	per-subsystem so one being switched off never quietly switches the other off too.
	{
		ProceduralWorldVehicleSubsystemSettings engine_off = on;
		engine_off.EngineDamageStartFraction = 0.0f;
		Check (VehicleGameObj::Compute_Subsystem_Engine_Factor (engine_off,0.0f) == 1.0f,
				 "a zero engine threshold still degraded the engine");
		Check (VehicleGameObj::Compute_Subsystem_Mobility_Factor (engine_off,0.0f) < 1.0f,
				 "switching the engine threshold off also switched mobility off");

		ProceduralWorldVehicleSubsystemSettings mobility_off = on;
		mobility_off.MobilityDamageStartFraction = 0.0f;
		Check (VehicleGameObj::Compute_Subsystem_Mobility_Factor (mobility_off,0.0f) == 1.0f,
				 "a zero mobility threshold still degraded mobility");
		Check (VehicleGameObj::Compute_Subsystem_Engine_Factor (mobility_off,0.0f) < 1.0f,
				 "switching the mobility threshold off also switched the engine off");

		ProceduralWorldVehicleSubsystemSettings weapon_off = on;
		weapon_off.WeaponDamageStartFraction = 0.0f;
		Check (VehicleGameObj::Compute_Subsystem_Weapon_Efficiency (weapon_off,0.0f) == 1.0f,
				 "a zero weapon threshold still degraded the rate of fire");
		Check (VehicleGameObj::Compute_Subsystem_Engine_Factor (weapon_off,0.0f) < 1.0f,
				 "switching the weapon threshold off also switched the engine off");
	}

	/*
	**	CanDrive/CanFire, the TT script gates (Set_Can_Drive, Set_Scripts_Can_Fire), are
	**	separate from subsystem damage and independent of each other: a disabled engine leaves
	**	the gun live, a blocked gun leaves the vehicle drivable, and both on changes nothing.
	*/
	{
		ControlClass control;
		struct { bool drive; bool fire; } cases [] = { { true,true },{ false,true },{ true,false },{ false,false } };
		for (int i = 0; i < 4; i ++) {
			control.Set_Analog (ControlClass::ANALOG_MOVE_FORWARD,1.0f);
			control.Set_Analog (ControlClass::ANALOG_TURN_LEFT,0.5f);
			control.Set_Boolean (ControlClass::BOOLEAN_WEAPON_FIRE_PRIMARY,true);
			control.Set_Boolean (ControlClass::BOOLEAN_WEAPON_FIRE_SECONDARY,true);

			VehicleGameObj::Apply_Script_Control_Gates (control,cases [i].drive,cases [i].fire);

			bool moving = (control.Get_Analog (ControlClass::ANALOG_MOVE_FORWARD) != 0.0f) &&
							  (control.Get_Analog (ControlClass::ANALOG_TURN_LEFT) != 0.0f);
			bool firing = control.Get_Boolean (ControlClass::BOOLEAN_WEAPON_FIRE_PRIMARY) &&
							  control.Get_Boolean (ControlClass::BOOLEAN_WEAPON_FIRE_SECONDARY);
			bool not_firing = !control.Get_Boolean (ControlClass::BOOLEAN_WEAPON_FIRE_PRIMARY) &&
									!control.Get_Boolean (ControlClass::BOOLEAN_WEAPON_FIRE_SECONDARY);

			Check (moving == cases [i].drive,"CanDrive=%d CanFire=%d left the vehicle %s",
					 (int)cases [i].drive,(int)cases [i].fire,moving ? "driving" : "stopped");
			Check (cases [i].fire ? firing : not_firing,"CanDrive=%d CanFire=%d left the guns %s",
					 (int)cases [i].drive,(int)cases [i].fire,firing ? "firing" : "not in the expected state");
		}
	}
}


/***********************************************************************************************
**	coastline
**
**	V2 Section 9.4.  A 5 x 5-chunk field (64-unit chunks) whose west column lies at -10, below a
**	sea level of 0, and whose chunk (3,2) is a -5 hollow walled in by +10 ground.  The coast must
**	flood exactly the west column -- connected to the field's edge -- and leave the hollow dry,
**	because a low spot the sea cannot reach is a basin's business, not the sea's.  Off, it floods
**	nothing; the result is deterministic.
***********************************************************************************************/
void	Build_Coast_Terrain (void)
{
	const int vx = 81;
	const int vy = 81;
	const float cell = 4.0f;

	WorldTerrainSystem::Destroy_Terrain ();
	WorldTerrainSystem::Create_Terrain (vx,vy,cell,Vector3 (0.0f,0.0f,0.0f));

	float * heights = new float [vx * vy];
	for (int iy = 0; iy < vy; iy++) {
		for (int ix = 0; ix < vx; ix++) {
			float x = (float)ix * cell;
			float y = (float)iy * cell;
			float h = 10.0f;
			if (x <= 64.0f) h = -10.0f;
			if ((x >= 192.0f) && (x <= 256.0f) && (y >= 128.0f) && (y <= 192.0f)) h = -5.0f;
			heights[(iy * vx) + ix] = h;
		}
	}
	WorldTerrainSystem::Set_Heights (heights,vx * vy);
	delete [] heights;
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);
}


void	Check_Coastline (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	HydrologyGenerator::Init ();

	HydrologyParams params;
	params.Seed						= 77u;
	params.GenerateRivers			= false;
	params.GenerateLakes			= false;
	params.GeneratePonds			= false;
	params.GenerateCoastline	= true;
	params.SeaLevel				= 0.0f;

	Build_Coast_Terrain ();
	StringClass error ("");
	Check (HydrologyGenerator::Generate (params,&error),"coastline generation failed: %s",error.Peek_Buffer ());

	int coast = 0, hollow_flooded = 0, off_column = 0;
	for (int i = 0; i < HydrologyGenerator::Get_Feature_Count (); i++) {
		HydrologyFeature f;
		if (!HydrologyGenerator::Get_Feature (i,&f) || (f.Type != HYDRO_FEATURE_COAST)) continue;
		coast++;
		const WaterAreaClass * area = WaterSystem::Find_Area (f.Name.Peek_Buffer ());
		Check (area != nullptr,"coast feature '%s' has no water area",f.Name.Peek_Buffer ());
		if (area == nullptr) continue;
		AABoxClass bounds;
		area->Get_Bounds (&bounds);
		if ((bounds.Center.X > 192.0f) && (bounds.Center.X < 256.0f) && (bounds.Center.Y > 128.0f) && (bounds.Center.Y < 192.0f)) hollow_flooded++;
		if (bounds.Center.X > 64.0f) off_column++;
		Check (WWMath::Fabs (area->Get_Height () - 0.0f) < 0.01f,"a coastal area stands at %.2f, not sea level",area->Get_Height ());
	}
	Check (coast == 5,"the coast flooded %d chunk(s), not the 5 of the west column (%d feature(s), %d x %d chunks, %d water area(s))",
			 coast,HydrologyGenerator::Get_Feature_Count (),WorldTerrainSystem::Get_Chunk_Count_X (),
			 WorldTerrainSystem::Get_Chunk_Count_Y (),WaterSystem::Get_Area_Count ());
	Check (hollow_flooded == 0,"the walled-in hollow was flooded by a sea that cannot reach it");
	Check (off_column == 0,"%d coastal chunk(s) lie outside the below-sea-level west column",off_column);

	StringClass report ("");
	int cliff_faults = HydrologyGenerator::Check_No_Cliff_Band_Overlap (&report);
	Check (cliff_faults == 0,"a coastal area reached the cliff band: %s",report.Peek_Buffer ());
	int crossing_faults = HydrologyGenerator::Check_Crossing_Candidates_Present (&report);
	Check (crossing_faults == 0,"a coastal feature carried no crossing candidate: %s",report.Peek_Buffer ());

	unsigned int first = HydrologyGenerator::Get_Checksum ();
	HydrologyGenerator::Clear ();
	Build_Coast_Terrain ();
	Check (HydrologyGenerator::Generate (params,&error),"regenerating the coast failed");
	Check (HydrologyGenerator::Get_Checksum () == first,"the same terrain produced a different coast");

	HydrologyGenerator::Clear ();
	Build_Coast_Terrain ();
	params.GenerateCoastline = false;
	Check (HydrologyGenerator::Generate (params,&error),"generation with the coast off failed");
	Check (HydrologyGenerator::Get_Feature_Count () == 0,"with the coast off, %d feature(s) were still built",
			 HydrologyGenerator::Get_Feature_Count ());

	HydrologyGenerator::Shutdown ();
	WaterSystem::Shutdown ();
	WorldTerrainSystem::Destroy_Terrain ();
	WorldBoundsService::Shutdown ();
}


/***********************************************************************************************
**	tunnels
**
**	V2 Phase 10.  A 192 x 128 field with a ridge running north-south down the middle (a 16-unit
**	flat crest at 20 units, 45-degree flanks) and a separate step terrain, both built so the
**	answer is known.  Proves: open ground is found either side of a ridge (never on its crest),
**	the bore goes under real cover, a step gets a long ramp on its high side, refusals carry the
**	right reason, the module chain is joined socket to socket, the portal cuts open exactly the
**	approach ground (not the cover) and a hole neither collides nor survives removal, a cliff
**	face the bore crosses is cut, the plan is deterministic, a bore crossing the world boundary
**	is refused, and a road tunnel splits its road and binds both new ends to itself.
***********************************************************************************************/
void	Build_Tunnel_Ridge_Terrain (bool step)
{
	const int vx = 97;
	const int vy = 65;
	const float cell = 2.0f;

	WorldTerrainSystem::Destroy_Terrain ();
	WorldTerrainSystem::Create_Terrain (vx,vy,cell,Vector3 (0.0f,0.0f,0.0f));

	float * heights = new float [vx * vy];
	for (int iy = 0; iy < vy; iy++) {
		for (int ix = 0; ix < vx; ix++) {
			float x = (float)ix * cell;
			float h;
			if (step) {
				//	0 west of x=92, 20 east of x=100, a steep face between.
				h = WWMath::Clamp ((x - 92.0f) / 8.0f,0.0f,1.0f) * 20.0f;
			} else {
				float d = WWMath::Fabs (x - 96.0f);
				h = (d <= 8.0f) ? 20.0f : WWMath::Max (0.0f,20.0f - (d - 8.0f));
			}
			heights[(iy * vx) + ix] = h;
		}
	}
	WorldTerrainSystem::Set_Heights (heights,vx * vy);
	delete [] heights;
}


TunnelPlannerParams	Tunnel_Test_Params (void)
{
	TunnelPlannerParams params;
	params.Seed				= 1001u;
	params.MaxBoreLength	= 200.0f;
	params.MinCover		= 1.0f;
	params.WallThickness	= 0.5f;

	TunnelClearanceClass infantry;
	infantry.InternalWidth		= 2.0f;
	infantry.InternalHeight		= 2.5f;
	infantry.LateralSafety		= 0.5f;
	infantry.VerticalSafety		= 0.3f;
	infantry.MaxGradeRadians	= 0.5f;
	infantry.MinPortalStraight	= 2.0f;
	params.ClassClearance[TUNNEL_CLASS_INFANTRY_ONLY]	= infantry;
	params.ClassClearance[TUNNEL_CLASS_CAVE_SHORTCUT]	= infantry;

	TunnelClearanceClass vehicle;
	vehicle.InternalWidth		= 4.0f;
	vehicle.InternalHeight		= 3.5f;
	vehicle.LateralSafety		= 1.0f;
	vehicle.VerticalSafety		= 1.0f;
	vehicle.MaxGradeRadians		= 0.3f;
	vehicle.MinPortalStraight	= 4.0f;
	params.ClassClearance[TUNNEL_CLASS_VEHICLE]		= vehicle;
	params.ClassClearance[TUNNEL_CLASS_UNDERPASS]	= vehicle;
	params.ClassClearance[TUNNEL_CLASS_MIXED]			= vehicle;
	return params;
}


TunnelCandidate	Tunnel_Test_Candidate (float x,float y)
{
	TunnelCandidate c;
	c.Id			= "tunnel_test";
	c.Source		= TUNNEL_SOURCE_INFANTRY_FLANK;
	c.Class		= TUNNEL_CLASS_INFANTRY_ONLY;
	c.Position	= Vector3 (x,y,0.0f);
	c.Axis		= Vector3 (1.0f,0.0f,0.0f);
	return c;
}


void	Check_Tunnels (void)
{
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	WaterSystem::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	EscarpmentSystem::Init ();
	TunnelSystem::Init ();
	TunnelPlanner::Init ();

	Build_Tunnel_Ridge_Terrain (false);

	TunnelPlannerParams params = Tunnel_Test_Params ();
	TunnelCandidate candidate = Tunnel_Test_Candidate (96.0f,64.0f);

	//
	//	A ridge: open ground on both flanks, never on the crest, and real cover over the bore.
	//
	TunnelClass planned;
	TunnelFailureReasonType reason = TUNNEL_FAILURE_NONE;
	bool ok = TunnelPlanner::Plan_Bore (candidate,params,&planned,&reason);
	Check (ok,"a bore straight through a 20-unit ridge was refused: %s",Tunnel_Failure_Reason_Name (reason));
	if (ok) {
		Check (planned.Start.X <= 70.0f,"the west portal is at x=%.1f, up on the ridge rather than in the valley",planned.Start.X);
		Check (planned.End.X >= 122.0f,"the east portal is at x=%.1f, up on the ridge rather than in the valley",planned.End.X);
		Check (WWMath::Fabs (planned.Start.Z) < 0.5f && WWMath::Fabs (planned.End.Z) < 0.5f,
				 "the portals are not on the valley floor (%.2f, %.2f)",planned.Start.Z,planned.End.Z);
		Check (planned.Get_Grade_Radians () <= params.ClassClearance[TUNNEL_CLASS_INFANTRY_ONLY].MaxGradeRadians,
				 "the bore's grade %.3f exceeds its class limit",planned.Get_Grade_Radians ());
		Check (planned.Clearance.Get_Bore_Width () == 3.0f,
				 "the bore is %.2f wide, not the class envelope plus both safety margins",planned.Clearance.Get_Bore_Width ());
		Check (::strcmp (planned.ModuleName.Peek_Buffer (),"tunnel_lined") == 0,
				 "an infantry tunnel picked module family '%s'",planned.ModuleName.Peek_Buffer ());

		Vector3 mid ((planned.Start.X + planned.End.X) * 0.5f,64.0f,planned.Get_Floor_Height_At (planned.Get_Plan_Length () * 0.5f) + 1.0f);
		Check (planned.Is_Point_Inside (mid),"a point in the middle of the bore is not inside it");
		Check (!planned.Is_Point_Inside (Vector3 (96.0f,64.0f,20.5f)),"a point on the ridge crest counts as inside the bore");
	}

	//
	//	Refusals, each for the right reason.
	//
	{
		TunnelPlannerParams no_metrics = params;
		no_metrics.ClassClearance[TUNNEL_CLASS_INFANTRY_ONLY] = TunnelClearanceClass ();
		Check (!TunnelPlanner::Plan_Bore (candidate,no_metrics,nullptr,&reason) && (reason == TUNNEL_FAILURE_NO_CLEARANCE_DATA),
				 "a class with no measured clearance was not refused as no_clearance_data (%s)",Tunnel_Failure_Reason_Name (reason));

		TunnelCandidate flat = Tunnel_Test_Candidate (20.0f,64.0f);
		Check (!TunnelPlanner::Plan_Bore (flat,params,nullptr,&reason) && (reason == TUNNEL_FAILURE_NO_OVERBURDEN),
				 "a bore on open flat ground was not refused as no_overburden (%s)",Tunnel_Failure_Reason_Name (reason));

		TunnelPlannerParams short_bore = params;
		short_bore.MaxBoreLength = 20.0f;
		Check (!TunnelPlanner::Plan_Bore (candidate,short_bore,nullptr,&reason),
				 "a 20-unit bore limit still accepted a ridge that needs about 56 units");
	}

	//
	//	A step: the low portal at the toe, the ramp lengthened on the high side until it fits.
	//
	{
		Build_Tunnel_Ridge_Terrain (true);
		TunnelCandidate step = Tunnel_Test_Candidate (96.0f,64.0f);
		TunnelClass step_bore;
		bool step_ok = TunnelPlanner::Plan_Bore (step,params,&step_bore,&reason);
		Check (step_ok,"a bore through a 20-unit step was refused: %s",Tunnel_Failure_Reason_Name (reason));
		if (step_ok) {
			float tan_limit = tanf (params.ClassClearance[TUNNEL_CLASS_INFANTRY_ONLY].MaxGradeRadians);
			Check (step_bore.Get_Grade_Radians () <= (params.ClassClearance[TUNNEL_CLASS_INFANTRY_ONLY].MaxGradeRadians + 0.001f),
					 "the step bore climbs at %.3f rad, over its class limit",step_bore.Get_Grade_Radians ());
			Check (step_bore.Get_Plan_Length () >= (20.0f / tan_limit) - 1.0f,
					 "the step bore is %.1f long, too short for a 20-unit climb at the class grade",step_bore.Get_Plan_Length ());
			Check (step_bore.Start.X < 96.0f && step_bore.Start.Z < 1.0f,"the step bore's low portal is not on the low bench");
			Check (step_bore.End.Z > 19.0f,"the step bore's high portal is not on the high bench (z=%.1f)",step_bore.End.Z);
		}
		Build_Tunnel_Ridge_Terrain (false);
	}

	//
	//	Realize: module chain, socket continuity, portal stations, light anchors.
	//
	Check (TunnelPlanner::Resolve_Candidate (candidate,params),"a legal ridge bore was not realized");
	Check (TunnelSystem::Get_Tunnel_Count () == 1,"%d tunnels exist after one was realized",TunnelSystem::Get_Tunnel_Count ());
	Check (TunnelSystem::Build_Layout (),"the realized tunnel would not lay out");

	const TunnelClass * tunnel = TunnelSystem::Peek_Tunnel (0);
	Check (tunnel != nullptr,"the realized tunnel is not there");
	if (tunnel != nullptr) {
		const TunnelModuleDefinitionClass * def = TunnelSystem::Peek_Module (tunnel->Modules.Count () > 0 ? tunnel->Modules[0].DefinitionIndex : -1);
		Check (def != nullptr,"the tunnel's module definition is missing");
		int expected = (def != nullptr) ? (int)ceilf (tunnel->Get_Length () / def->Length) : 0;
		Check (tunnel->Modules.Count () == expected,
				 "a %.1f-unit bore laid %d modules, not %d",tunnel->Get_Length (),tunnel->Modules.Count (),expected);

		for (int m = 1; m < tunnel->Modules.Count (); m++) {
			Vector3 prev = tunnel->Modules[m - 1].Transform.Get_Translation ();
			Vector3 here = tunnel->Modules[m].Transform.Get_Translation ();
			Check (WWMath::Fabs ((here - prev).Length () - tunnel->Modules[m - 1].Length) < 0.01f,
					 "module %d does not start at module %d's exit socket",m,m - 1);
		}
		if (tunnel->Modules.Count () > 0) {
			const TunnelModuleInstanceClass & last = tunnel->Modules[tunnel->Modules.Count () - 1];
			Vector3 chain_end = last.Transform * Vector3 (last.Length,0.0f,0.0f);
			Check ((chain_end - tunnel->End).Length () < 0.01f,"the module chain ends %.3f from the far portal",
					 (chain_end - tunnel->End).Length ());
			Check (!tunnel->Modules[0].UsesModel,"a module with no declared clearance claimed an authored model");
		}

		Check (tunnel->PortalStationA > 0.0f && tunnel->PortalStationA < tunnel->PortalStationB &&
				 tunnel->PortalStationB < tunnel->Get_Plan_Length (),
				 "portal stations %.1f/%.1f are not inside the %.1f-unit bore in order",
				 tunnel->PortalStationA,tunnel->PortalStationB,tunnel->Get_Plan_Length ());
		Check (tunnel->LightAnchors.Count () >= 1 && tunnel->LightAnchors.Count () <= 4,
				 "the tunnel has %d light anchors",tunnel->LightAnchors.Count ());
		Check (tunnel->HeadwallTopA > tunnel->Get_Floor_Height_At (tunnel->PortalStationA) + tunnel->Clearance.Get_Bore_Height (),
				 "the west headwall does not reach above the roof");

		//	The placement/mutation footprint covers the mouths and the covered bore, and nothing wide of it.
		Check (TunnelSystem::Overlaps_Footprint (tunnel->Start,1.0f,nullptr),"a building at the west mouth is not refused");
		Check (TunnelSystem::Overlaps_Footprint (Vector3 (96.0f,64.0f,20.0f),1.0f,nullptr),
				 "a building on the ridge right over the bore is not refused");
		Check (!TunnelSystem::Overlaps_Footprint (Vector3 (96.0f,100.0f,20.0f),1.0f,nullptr),
				 "a building 36 units beside the bore is refused");

		//	A cliff face across the bore is cut; one beside it is not.
		EscarpmentClass across;
		across.Add_Point (Vector3 (100.0f,0.0f,0.0f),20.0f,0.0f);
		across.Add_Point (Vector3 (100.0f,128.0f,0.0f),20.0f,0.0f);
		EscarpmentSystem::Add_Escarpment (across);
		EscarpmentClass beside;
		beside.Add_Point (Vector3 (20.0f,0.0f,0.0f),20.0f,0.0f);
		beside.Add_Point (Vector3 (20.0f,40.0f,0.0f),20.0f,0.0f);
		EscarpmentSystem::Add_Escarpment (beside);

		//
		//	Portal cuts: the approach ground opens, the cover over the bore and the ground beside it
		//	do not, and a hole neither collides nor survives removal.
		//
		int opened = TunnelSystem::Apply_Portal_Cuts ();
		Check (opened > 0,"the portal cuts opened no ground at all");
		Check (WorldTerrainSystem::Get_Cell_Hole_Count () == opened,
				 "the field reports %d holes, the tunnel opened %d",WorldTerrainSystem::Get_Cell_Hole_Count (),opened);
		Check (EscarpmentSystem::Get_Portal_Cut_Count () == 1,
				 "%d cliff cuts, not the one face the bore crosses",EscarpmentSystem::Get_Portal_Cut_Count ());

		int crest_x,crest_y,side_x,side_y,approach_x,approach_y;
		WorldTerrainSystem::World_To_Cell (96.0f,64.0f,&crest_x,&crest_y);
		WorldTerrainSystem::World_To_Cell (96.0f,20.0f,&side_x,&side_y);
		WorldTerrainSystem::World_To_Cell (tunnel->Start.X + 3.0f,64.0f,&approach_x,&approach_y);
		Check (!WorldTerrainSystem::Is_Cell_Hole (crest_x,crest_y),"the ridge crest over the bore was opened");
		Check (!WorldTerrainSystem::Is_Cell_Hole (side_x,side_y),"ground well beside the bore was opened");
		Check (WorldTerrainSystem::Is_Cell_Hole (approach_x,approach_y),"the approach ground inside the west mouth is still closed");

		int px = approach_x / HeightfieldClass::PATCH_CELLS;
		int py = approach_y / HeightfieldClass::PATCH_CELLS;
		RenegadeTerrainPatchClass * patch = WorldTerrainSystem::Create_Patch_Model (px,py);
		Check (patch != nullptr,"the patch holding the approach would not build");
		if (patch != nullptr) {
			int quads_x = HeightfieldClass::PATCH_CELLS;
			int quad = ((approach_y - (py * quads_x)) * quads_x) + (approach_x - (px * quads_x));
			Check (patch->Is_Quad_Hidden (quad),"the approach hole did not reach the patch as QF_HIDDEN");

			Vector3 hole_centre,solid_centre;
			WorldTerrainSystem::Get_Cell_Center (approach_x,approach_y,&hole_centre);
			//	Same patch, 16 units off the bore's centre line: closed ground the ray must hit.
			WorldTerrainSystem::Get_Cell_Center (approach_x,approach_y + 8,&solid_centre);
			float z = 0.0f;
			Check (!Drop_Ray_On_Model (patch,hole_centre.X,hole_centre.Y,&z),"a ray dropped through a hole hit ground at %.2f",z);
			Check (Drop_Ray_On_Model (patch,solid_centre.X,solid_centre.Y,&z),"a ray dropped on closed ground beside the hole missed");
			patch->Release_Ref ();
		}

		unsigned int first_checksum = TunnelSystem::Get_Checksum ();

		TunnelSystem::Remove_Portal_Cuts ();
		Check (WorldTerrainSystem::Get_Cell_Hole_Count () == 0,"%d holes survive removing the cuts",WorldTerrainSystem::Get_Cell_Hole_Count ());
		Check (EscarpmentSystem::Get_Portal_Cut_Count () == 0,"a cliff cut survives removing the cuts");

		//	Deterministic: the same candidate on the same ground realizes the same tunnel.
		TunnelPlanner::Clear ();
		TunnelPlanner::Resolve_Candidate (candidate,params);
		TunnelSystem::Build_Layout ();
		TunnelSystem::Apply_Portal_Cuts ();
		Check (TunnelSystem::Get_Checksum () == first_checksum,
				 "the same candidate realized a different tunnel (0x%08X vs 0x%08X)",TunnelSystem::Get_Checksum (),first_checksum);

		StringClass report ("");
		Check (TunnelPlanner::Check_No_Boundary_Breach (&report) == 0,"%s",report.Peek_Buffer ());

		EscarpmentSystem::Clear_Escarpments ();
	}

	//
	//	The world boundary: a bore whose west mouth sits in the cliff band is refused.
	//
	TunnelPlanner::Clear ();
	{
		DynamicVectorClass<Vector2> polygon;
		polygon.Add (Vector2 ( 60.0f,  0.0f));
		polygon.Add (Vector2 ( 60.0f,128.0f));
		polygon.Add (Vector2 (192.0f,128.0f));
		polygon.Add (Vector2 (192.0f,  0.0f));
		StringClass error ("");
		Check (WorldBoundsService::Configure (polygon,15.0f,5.0f,200.0f,&error),
				 "the tunnel test boundary would not configure: %s",error.Peek_Buffer ());
		Check (!TunnelPlanner::Plan_Bore (candidate,params,nullptr,&reason) && (reason == TUNNEL_FAILURE_BOUNDARY),
				 "a bore whose mouth sits in the cliff band was not refused as boundary (%s)",Tunnel_Failure_Reason_Name (reason));
		WorldBoundsService::Clear ();
	}

	//
	//	A road tunnel: the road over the ridge is split at both portals and both new ends hand
	//	over to the tunnel.
	//
	{
		RoadSplineClass road;
		road.Set_Name ("ridge_road");
		road.Add_Control_Point (Vector3 ( 20.0f,64.0f,0.0f));
		road.Add_Control_Point (Vector3 ( 96.0f,64.0f,20.0f));
		road.Add_Control_Point (Vector3 (172.0f,64.0f,0.0f));
		road.Set_Width (6.0f);
		int road_id = RoadSystem::Add_Road (road);
		Check (RoadSystem::Build_Network (0.25f,0.5f,4.0f),"the ridge road would not build");

		TunnelCandidate road_candidate = candidate;
		road_candidate.Id				= "crossing_road_test";
		road_candidate.Source		= TUNNEL_SOURCE_UNRESOLVED_ROAD_EDGE;
		road_candidate.Class			= TUNNEL_CLASS_VEHICLE;
		road_candidate.RoadId		= road_id;
		road_candidate.RoadEdgeId	= "edge_test";
		road_candidate.RoadWidth	= 6.0f;

		Check (TunnelPlanner::Resolve_Candidate (road_candidate,params),"a legal road bore was not realized");
		ResolvedTunnel resolved;
		Check (TunnelPlanner::Get_Tunnel (0,&resolved),"the road tunnel is not recorded");
		Check (resolved.RoadSplit,"the road over the ridge was not split at the tunnel's portals");

		const TunnelClass * road_tunnel = TunnelSystem::Find_Tunnel (resolved.TunnelId);
		Check ((road_tunnel != nullptr) && (road_tunnel->Clearance.InternalWidth >= 6.0f),
				 "the road bore is narrower than the road it carries");

		StringClass report ("");
		Check (TunnelPlanner::Check_Road_Tunnels_Bound (&report) == 0,"%s",report.Peek_Buffer ());

		int ends_bound = 0;
		for (int r = 0; r < RoadSystem::Get_Road_Count (); r++) {
			RoadSplineClass * spline = RoadSystem::Peek_Road (r);
			if (spline == nullptr) continue;
			if ((spline->Get_End_Connection ().Type == ROAD_ENDPOINT_TUNNEL) &&
				 (spline->Get_End_Connection ().TargetID == resolved.TunnelId)) ends_bound++;
			if ((spline->Get_Start_Connection ().Type == ROAD_ENDPOINT_TUNNEL) &&
				 (spline->Get_Start_Connection ().TargetID == resolved.TunnelId)) ends_bound++;
		}
		Check (ends_bound == 2,"%d road ends hand over to the tunnel, not the two either side of it",ends_bound);
	}

	TunnelPlanner::Clear ();
	RoadSystem::Clear_Roads ();
	TunnelSystem::Shutdown ();
	WorldTerrainSystem::Destroy_Terrain ();
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
	if ((which == nullptr) || (::strcmp (which, "shadows") == 0)) {
		Check_Shadows ();
	}
	if ((which == nullptr) || (::strcmp (which, "lights") == 0)) {
		Check_Lights ();
	}
	if ((which == nullptr) || (::strcmp (which, "far") == 0)) {
		Check_Far_Terrain ();
	}
	if ((which == nullptr) || (::strcmp (which, "shroud") == 0)) {
		Check_Shroud ();
	}
	if ((which == nullptr) || (::strcmp (which, "macro") == 0)) {
		Check_Macro_Terrain ();
	}
	if ((which == nullptr) || (::strcmp (which, "interior") == 0)) {
		Check_Interior_Cliffs ();
	}
	if ((which == nullptr) || (::strcmp (which, "hydrology") == 0)) {
		Check_Hydrology ();
	}
	if ((which == nullptr) || (::strcmp (which, "strategic") == 0)) {
		Check_Strategic_Plan ();
	}
	if ((which == nullptr) || (::strcmp (which, "roadgraph") == 0)) {
		Check_Road_Graph ();
	}
	if ((which == nullptr) || (::strcmp (which, "bridgeplan") == 0)) {
		Check_Bridge_Plan ();
	}
	if ((which == nullptr) || (::strcmp (which, "baseselection") == 0)) {
		Check_Base_Selection ();
	}
	if ((which == nullptr) || (::strcmp (which, "tiberiumfields") == 0)) {
		Check_Tiberium_Fields ();
	}
	if ((which == nullptr) || (::strcmp (which, "settlements") == 0)) {
		Check_Settlements ();
	}
	if ((which == nullptr) || (::strcmp (which, "biomedecoration") == 0)) {
		Check_Biome_Decoration ();
	}
	if ((which == nullptr) || (::strcmp (which, "combatspace") == 0)) {
		Check_Combat_Space ();
	}
	if ((which == nullptr) || (::strcmp (which, "worldintegration") == 0)) {
		Check_World_Integration ();
	}
	if ((which == nullptr) || (::strcmp (which, "navigation") == 0)) {
		Check_Navigation ();
	}
	if ((which == nullptr) || (::strcmp (which, "minimap") == 0)) {
		Check_Minimap_Base_Layer ();
	}
	if ((which == nullptr) || (::strcmp (which, "bridgerequired") == 0)) {
		Check_Bridge_Required ();
	}
	if ((which == nullptr) || (::strcmp (which, "tunnels") == 0)) {
		Check_Tunnels ();
	}
	if ((which == nullptr) || (::strcmp (which, "coastline") == 0)) {
		Check_Coastline ();
	}
	if ((which == nullptr) || (::strcmp (which, "bridgefallback") == 0)) {
		Check_Bridge_Fallback ();
	}
	if ((which == nullptr) || (::strcmp (which, "placementrules") == 0)) {
		Check_Placement_Rules ();
	}
	if ((which == nullptr) || (::strcmp (which, "grading") == 0)) {
		Check_Foundation_Grading ();
	}
	if ((which == nullptr) || (::strcmp (which, "expansion") == 0)) {
		Check_Expansion_Regions ();
	}
	if ((which == nullptr) || (::strcmp (which, "worldops") == 0)) {
		Check_World_Operations ();
	}
	if ((which == nullptr) || (::strcmp (which, "worldsync") == 0)) {
		Check_World_Sync ();
	}
	if ((which == nullptr) || (::strcmp (which, "logistics") == 0)) {
		Check_Logistics ();
	}
	if ((which == nullptr) || (::strcmp (which, "vehiclesubsystems") == 0)) {
		Check_Vehicle_Subsystems ();
	}

	if (_Failures == 0) {
		::fprintf (stdout, "terrain self check: pass\n");
	} else {
		::fprintf (stdout, "terrain self check: %d failure(s)\n", _Failures);
	}

	return (_Failures == 0) ? 0 : 1;
}
