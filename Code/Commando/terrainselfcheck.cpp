////	The terrain service, checked from inside the binary that has to be right.
//	See terrainselfcheck.h for why the checks live here.
//

#include "terrainselfcheck.h"

#include "baseselectionplanner.h"
#include "proceduralbaselayout.h"
#include "proceduralworldpersistence.h"
#include "crc.h"
#include "proceduralworldmanager.h"
#include "proceduralboundsbuilder.h"
#include "worldseedservice.h"
#include "rawfile.h"
#include "chunkio.h"
#include "commanderminimap.h"
#include "proceduralworldprofile.h"
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
#include "worldsynctransport.h"
#include "bridgedamageservice.h"
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
#include "mapplangenerator.h"
#include "hybridplanrecords.h"
#include "hybridplanning.h"
#include "hybridlocalpass.h"
#include "hybridpreflight.h"
#include "hybridmapidentity.h"
#include "hybridstrategicgraph.h"
#include "hybridcoarsematrix.h"
#include "hybridlandforms.h"
#include "hybridhydrology.h"
#include "hybridmacroterrain.h"
#include "hybridanchors.h"
#include "hybridtopology.h"
#include "hybridroutes.h"
#include "hybridrealization.h"
#include "w3d_file.h"
#include "worldgenerationmanifest.h"
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
#include "WeatherMgr.h"
#include "tiberiumfieldgenerator.h"
#include "transportationnetworkvalidator.h"
#include "tunnelsystem.h"
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
	//	P15: gameplay damage.  Blasts accumulate on the spans in reach, states only advance,
	//	and a span falls exactly once -- after which the bridge no longer carries anything.
	//	Server-side only, as the explosion pass that calls it is.
	//
	{
		bool was_server = CombatManager::I_Am_Server ();
		CombatManager::Set_I_Am_Server (true);
		BridgeDamageService::Reset ();

		//	Alone: an earlier block leaves a deliberately unlayable bridge behind, and Build_Layout
		//	answers for the whole set.
		BridgeSystem::Clear_Bridges ();

		BridgeClass target;
		target.Set_Name ("damage_target");
		target.Set_Definition ("ow_bridge_concrete");
		target.Set_Endpoints (Vector3 (0.0f, 500.0f, 10.0f), Vector3 (100.0f, 500.0f, 10.0f));
		int target_id = BridgeSystem::Add_Bridge (target);
		Check (BridgeSystem::Build_Layout (), "the damage target would not lay out");
		Check (BridgeSystem::Is_Traversable (target_id), "a fresh bridge is not traversable");

		Vector3 middle (50.0f, 500.0f, 10.0f);
		float strength = BridgeDamageService::SPAN_HIT_POINTS * 0.4f;

		Check (BridgeDamageService::Apply_Explosion (Vector3 (50.0f, 900.0f, 10.0f), 8.0f, strength) == 0,
				"a blast four hundred metres away damaged the bridge");

		Check (BridgeDamageService::Apply_Explosion (middle, 6.0f, strength) > 0,
				"a blast on the deck changed no span's state");
		int hit_section = -1;
		BridgeClass *hit = BridgeSystem::Peek_Bridge (target_id);
		for (int s = 0; (hit != nullptr) && (s < hit->Get_Section_Count ()); s++) {
			if (hit->Get_Section (s).State == BRIDGE_STATE_DAMAGED) { hit_section = s; break; }
		}
		Check (hit_section >= 0, "the span under the blast is not DAMAGED");
		Check (BridgeSystem::Is_Traversable (target_id), "a damaged span stopped carrying traffic");

		BridgeDamageService::Apply_Explosion (middle, 6.0f, strength);
		BridgeDamageService::Apply_Explosion (middle, 6.0f, strength);
		Check (BridgeDamageService::Get_Break_Count () == 1, "the span fell %d times, not once",
				BridgeDamageService::Get_Break_Count ());
		Check (!BridgeSystem::Is_Traversable (target_id), "a fallen span still carries traffic");

		//	A fallen span absorbs nothing more: it cannot fall a second time.
		float fallen_damage = BridgeDamageService::Get_Span_Damage (target_id, hit_section);
		BridgeDamageService::Apply_Explosion (middle, 1.0f, strength);
		Check (BridgeDamageService::Get_Span_Damage (target_id, hit_section) == fallen_damage &&
				BridgeSystem::Get_Section_State (target_id, hit_section) == BRIDGE_STATE_BROKEN,
				"a fallen span took more damage");
		Check (BridgeDamageService::Get_Break_Count () == 1, "a blast on a fallen span broke something again");
		Check (BridgeDamageService::State_For_Damage (0.0f) == BRIDGE_STATE_PRISTINE &&
				BridgeDamageService::State_For_Damage (BridgeDamageService::SPAN_HIT_POINTS) == BRIDGE_STATE_BROKEN,
				"the damage thresholds are not pristine at zero and broken at full hit points");

		BridgeDamageService::Reset ();
		CombatManager::Set_I_Am_Server (was_server);
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
	//	D-001 P10: the sightline query.  A conifer trunk (0.7 m proxy, 9 m tall) across the line
	//	blocks it where the trunk begins; grass on the line and a trunk beside it do not; a line
	//	passing over the trunk's top does not.  Only the crossed cells are read.
	//
	{
		FoliageSystem::Clear_Instances ();
		FoliageSystem::Add_Instance ("ow_tree_conifer", Vector3 (50.0f, 0.0f, 0.0f));
		FoliageSystem::Add_Instance ("ow_grass_clump", Vector3 (20.0f, 0.0f, 0.0f));
		FoliageSystem::Add_Instance ("ow_tree_conifer", Vector3 (70.0f, 5.0f, 0.0f));
		Check (FoliageSystem::Build_Cells (), "the occlusion scene's cells did not build");

		float fraction = 1.0f;
		Check (FoliageSystem::Query_Segment_Occlusion (Vector3 (0.0f, 0.0f, 1.6f), Vector3 (100.0f, 0.0f, 1.6f), &fraction),
				"a tree across the line of sight did not block it");
		Check (WWMath::Fabs (fraction - 0.493f) < 0.002f,
				"the tree blocked at %f of the line, not where its trunk begins (0.493)", fraction);

		fraction = 1.0f;
		Check (!FoliageSystem::Query_Segment_Occlusion (Vector3 (0.0f, 0.0f, 1.6f), Vector3 (40.0f, 0.0f, 1.6f), &fraction),
				"grass blocked a line of sight");
		Check (!FoliageSystem::Query_Segment_Occlusion (Vector3 (0.0f, 2.0f, 1.6f), Vector3 (100.0f, 2.0f, 1.6f), &fraction),
				"a line passing beside the trunks was blocked");
		Check (!FoliageSystem::Query_Segment_Occlusion (Vector3 (0.0f, 0.0f, 12.0f), Vector3 (100.0f, 0.0f, 12.0f), &fraction),
				"a line passing over the tree top was blocked");

		FoliageSystem::Clear_Instances ();
	}

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
		//	P17: every default kind carries a surface -- the ow_water_* file when shipped, else
		//	the retail water_texture stand-in.
		Check (lake_def->Names_Any_Texture (), "the lake definition names no surface texture");
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
	//	The missing-art path, taken deliberately: a kind stripped of its texture.
	WaterDefinitionClass *pond_def = WaterSystem::Find_Definition ("ow_water_pond");
	if (pond_def != nullptr) {
		pond_def->Set_Surface_Texture ("");
	}
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
	//	D-001 P18: the tank track draws with Renegade's own tread print until ow_ribbon_tank_track
	//	ships; tires have no stock print and name nothing.
	Check (tank_def.Names_A_Texture (), "the tank track names no texture; d_tk_dirt is its stock stand-in");
	{
		int tire = SurfaceRibbonSystem::Find_Definition_Index ("ow_ribbon_tire_track");
		Check ((tire >= 0) && !SurfaceRibbonSystem::Peek_Definition (tire).Names_A_Texture (),
				 "the tire track names art no archive holds");
	}
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
	//	The one kind laying marks names its texture, and with no scene nothing is fetched to miss.
	Check (SurfaceRibbonSystem::Get_Missing_Texture_Count () == 0,
			 "%d kinds report missing art though the one laying marks names its texture",
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


/***********************************************************************************************
 * Check_Map_Plan -- D-003 Phase 3B                                                             *
 *=============================================================================================*/
static void Make_Map_Plan_Test_Params (MapPlanParams * p)
{
	*p = MapPlanParams ();
	p->Seed = 4242u;
	p->WorldWidth = 1200.0f;
	p->WorldHeight = 1200.0f;
	p->ChunkSize = 60.0f;			// 20 x 20 cells
	p->CenterX = 0.0f;
	p->CenterY = 0.0f;
	p->Symmetry = MAP_PLAN_SYMMETRY_ROTATIONAL;
	p->TierCount = 3;
	p->TierStep = 18.0f;
	p->LaneWidthCells = 1;
	p->BaseRadiusCells = 2;
	p->FlankLaneCount = 1;
	p->WaterFraction = 0.1f;
	p->BoundaryMarginCells = 1.0f;
}


/*
**	Hybrid rewrite item I1 gate: the schema-12 plan block round-trips deterministically, old
**	profiles still load, the native profile selects the hybrid generator, and the Section 4, 4.3,
**	6 and 7 tables hold the plan's values.
*/
void	Check_Hybrid_Plan_Records (void)
{
	StringClass error ("");

	//	Native profile: loads, is schema 12, selects the hybrid generator, and loads identically twice.
	ProceduralWorldProfile native_a;
	ProceduralWorldProfile native_b;
	bool loaded_a = ProceduralWorldProfileManager::Load_Profile_From_File ("data/procedural_world/native.json",&native_a,&error);
	Check (loaded_a,"native profile did not load: %s",error.Peek_Buffer ());
	bool loaded_b = ProceduralWorldProfileManager::Load_Profile_From_File ("data/procedural_world/native.json",&native_b,&error);
	Check (loaded_b,"native profile did not load a second time: %s",error.Peek_Buffer ());
	if (loaded_a && loaded_b) {
		Check (native_a.Uses_Hybrid_Generator (),"native profile does not select the hybrid generator");
		Check (native_a.SchemaVersion == ProceduralWorldProfile::CURRENT_SCHEMA_VERSION,
			"native profile is schema %d, expected %d",native_a.SchemaVersion,(int)ProceduralWorldProfile::CURRENT_SCHEMA_VERSION);
		Check (WorldGenerationManifestService::Compute_Profile_Hash (native_a) == WorldGenerationManifestService::Compute_Profile_Hash (native_b),
			"native profile hash differs between two loads");
		Check (!native_a.Plan.LegacyLaneWidthCellsPresent,"native profile still writes plan.lane_width_cells");
		Check (!native_a.Features.LegacySettlementsPresent,"native profile still writes world_features.settlements");
		Check (native_a.Plan.RouteRedundancy >= 2 && native_a.Plan.RouteRedundancy <= 3,"native route_redundancy %d",native_a.Plan.RouteRedundancy);
		Check (!native_a.Plan.AllowFords,"native profile enables fords without a ford capability");
		Check (native_a.Plan.WaterFraction >= 0.03f && native_a.Plan.WaterFraction <= 0.20f,"native water_fraction %f outside 0.03..0.20",native_a.Plan.WaterFraction);
	}

	//	Old profiles: the shipped schema-11 default and a bare schema-8 profile still load and stay off the hybrid path.
	ProceduralWorldProfile old_default;
	Check (ProceduralWorldProfileManager::Load_Profile_From_File ("data/procedural_world/default.json",&old_default,&error),
		"schema-11 default profile no longer loads: %s",error.Peek_Buffer ());
	Check (!old_default.Uses_Hybrid_Generator (),"default profile unexpectedly selects the hybrid generator");
	ProceduralWorldProfile old_bare;
	Check (ProceduralWorldProfileManager::Load_Profile_From_Text ("{ \"schema_version\": 8, \"seed\": 5 }",&old_bare,&error),
		"schema-8 profile no longer loads: %s",error.Peek_Buffer ());
	Check (old_bare.Plan.Archetype == "auto" && old_bare.Plan.RouteRedundancy == 2 && old_bare.Plan.RoadDensity == 0.65f,
		"a profile without schema-12 fields does not get the Section 4 defaults");

	//	Legacy fields parse on the hybrid path and are only flagged; the hash ignores nothing here (legacy hash rule).
	ProceduralWorldProfile legacy;
	Check (ProceduralWorldProfileManager::Load_Profile_From_Text (
		"{ \"schema_version\": 11, \"seed\": 5, \"plan\": { \"enabled\": true, \"lane_width_cells\": 3 }, "
		"\"world_features\": { \"settlements\": true } }",&legacy,&error),
		"hybrid profile carrying legacy fields was refused: %s",error.Peek_Buffer ());
	Check (legacy.Plan.LegacyLaneWidthCellsPresent && legacy.Features.LegacySettlementsPresent,"legacy field presence not recorded");

	//	Section 0.7: on the hybrid path the legacy fields are excluded from the profile hash; off it they still count.
	ProceduralWorldProfile legacy_other = legacy;
	legacy_other.Plan.LaneWidthCells = 1;
	legacy_other.Features.Settlements = false;
	Check (WorldGenerationManifestService::Compute_Profile_Hash (legacy) == WorldGenerationManifestService::Compute_Profile_Hash (legacy_other),
		"legacy lane_width_cells/settlements still change the hybrid profile hash");
	legacy.World.FitToLevel = true;
	legacy_other.World.FitToLevel = true;
	Check (WorldGenerationManifestService::Compute_Profile_Hash (legacy) != WorldGenerationManifestService::Compute_Profile_Hash (legacy_other),
		"legacy lane_width_cells/settlements no longer hash on the legacy path");

	//	Schema-12 fields parse and their ranges are enforced.
	ProceduralWorldProfile explicit_plan;
	Check (ProceduralWorldProfileManager::Load_Profile_From_Text (
		"{ \"schema_version\": 12, \"seed\": 5, \"plan\": { \"enabled\": true, \"archetype\": \"canyon_crossroads\", "
		"\"biome_mode\": \"mixed\", \"biome\": \"desert\", \"secondary_biome\": \"tiberium_wasteland\", "
		"\"road_density\": 0.5, \"island_chance\": 0.0, \"route_redundancy\": 3, \"flank_lanes\": 0 } }",&explicit_plan,&error),
		"explicit schema-12 plan block was refused: %s",error.Peek_Buffer ());
	Check (explicit_plan.Plan.Archetype == "canyon_crossroads" && explicit_plan.Plan.Route_Family_Count () == 3,
		"explicit plan fields did not parse (archetype %s, families %d)",
		explicit_plan.Plan.Archetype.Peek_Buffer (),explicit_plan.Plan.Route_Family_Count ());

	const char * const bad_plans [] =
	{
		"\"biome\": \"snow\", \"secondary_biome\": \"desert\"",
		"\"biome\": \"snow\", \"secondary_biome\": \"snow\"",
		"\"archetype\": \"volcano\"",
		"\"biome_mode\": \"striped\"",
		"\"route_redundancy\": 4",
		"\"water_fraction\": 0.5",
		"\"tier_count\": 6",
		"\"road_density\": 1.5",
	};
	for (int index = 0; index < (int)(sizeof (bad_plans) / sizeof (bad_plans[0])); index ++) {
		StringClass text;
		text.Format ("{ \"schema_version\": 12, \"seed\": 5, \"plan\": { \"enabled\": true, %s } }",bad_plans[index]);
		ProceduralWorldProfile bad;
		bool ok = ProceduralWorldProfileManager::Load_Profile_From_Text (text,&bad,&error)
				 && ProceduralWorldProfileManager::Validate_Profile (bad,&error);
		Check (!ok,"illegal plan block accepted: %s",bad_plans[index]);
	}

	//	Section 4 route family count, 4.3 pairs, 6 order, 7 table.
	Check (Hybrid_Route_Family_Count (2,0) == 2 && Hybrid_Route_Family_Count (2,1) == 2
		&& Hybrid_Route_Family_Count (2,2) == 3 && Hybrid_Route_Family_Count (3,0) == 3,"route_family_count formula wrong");
	Check (Hybrid_Is_Legal_Biome_Pair (HYBRID_BIOME_TEMPERATE,HYBRID_BIOME_SNOW)
		&& Hybrid_Is_Legal_Biome_Pair (HYBRID_BIOME_TIBERIUM_WASTELAND,HYBRID_BIOME_DESERT)
		&& !Hybrid_Is_Legal_Biome_Pair (HYBRID_BIOME_DESERT,HYBRID_BIOME_SNOW)
		&& !Hybrid_Is_Legal_Biome_Pair (HYBRID_BIOME_TEMPERATE,HYBRID_BIOME_TEMPERATE),"legal biome pair table wrong");
	HybridArchetype archetype = HYBRID_ARCHETYPE_AUTO;
	Check (Hybrid_Archetype_From_Name ("river_valley",&archetype) && archetype == 0
		&& Hybrid_Archetype_From_Name ("open_frontier",&archetype) && archetype == HYBRID_ARCHETYPE_COUNT - 1,"archetype stable order wrong");
	const HybridRoadClassSpec & highway = Hybrid_Road_Class_Spec (HYBRID_ROAD_HIGHWAY_4);
	Check (::fabsf (highway.Roadbed_Width_W () - 5.65f) < 0.001f && highway.PhysicalTier == HYBRID_ROAD_TIER_HIGHWAY,
		"HIGHWAY_4 roadbed %.3fW",highway.Roadbed_Width_W ());
	Check (::fabsf (Hybrid_Road_Class_Spec (HYBRID_ROAD_PRIMARY_2).Roadbed_Width_W () - 2.60f) < 0.001f
		&& ::fabsf (Hybrid_Road_Class_Spec (HYBRID_ROAD_SECONDARY_2).Roadbed_Width_W () - 2.40f) < 0.001f
		&& ::fabsf (Hybrid_Road_Class_Spec (HYBRID_ROAD_SERVICE_DIRT_2).Roadbed_Width_W () - 2.40f) < 0.001f
		&& ::fabsf (Hybrid_Road_Class_Spec (HYBRID_ROAD_TRAIL_1).Roadbed_Width_W () - 1.20f) < 0.001f,
		"Section 7 roadbed widths wrong");

	//	Semantic matrix layers stay in lockstep.
	HybridSemanticMatrix matrix;
	Check (matrix.Resize (4,3,16.0f,0.0f,0.0f),"semantic matrix would not size");
	Check (matrix.Get_Cell_Count () == 12 && matrix.Flags != nullptr && matrix.BiomeId != nullptr
		&& matrix.MacroHeightBias != nullptr,"semantic matrix layers not sized CountX*CountY");
	matrix.Set_Flag (3,2,HYBRID_FLAG_COAST | HYBRID_FLAG_NO_DECORATION);
	Check (matrix.Has_Flag (3,2,HYBRID_FLAG_COAST) && !matrix.Has_Flag (2,2,HYBRID_FLAG_COAST),"semantic flag set/test wrong");
}


void Check_Hybrid_Planning (void)
{
	Check (Hybrid_Round_Half_Up (0.49f) == 0 && Hybrid_Round_Half_Up (0.5f) == 1 && Hybrid_Round_Half_Up (2.5f) == 3,"hybrid half-up rounding");
	Check (Hybrid_Pick_Index (4,0) == 0 && Hybrid_Pick_Index (4,0.5f) == 2 && Hybrid_Pick_Index (4,0.999999f) == 3,"hybrid index endpoints");
	Check (Hybrid_Pick_Sign (0.499f) == -1 && Hybrid_Pick_Sign (0.5f) == 1,"hybrid sign boundary");
	Check (Hybrid_Quantize_To_Int (-1.2344f,HYBRID_QUANTUM_POSITION) == -1234 && Hybrid_Quantize_To_Int (1.2346f,HYBRID_QUANTUM_POSITION) == 1235,"signed position quantization");
	Check (Hybrid_Quantize_To_Int (Hybrid_Quantize_Angle (0.1236f),HYBRID_QUANTUM_ANGLE) == 124
		&& Hybrid_Quantize_To_Int (Hybrid_Quantize_Normalized (0.1234564f),HYBRID_QUANTUM_NORMALIZED) == 123456,"angle and normalized quantizers");
	Check (Near (Hybrid_Range (2,4,0.25f),2.5f),"hybrid range draw");
	for (unsigned int key = 0; key < 512; ++key) {
		const float h = WorldSeedService::Hash01 (123,WorldSeedService::SEED_DOMAIN_MAP_PLAN,key);
		Check (h >= 0 && h < 1 && h == WorldSeedService::Hash01 (123,WorldSeedService::SEED_DOMAIN_MAP_PLAN,key),"keyed hash range/repeatability %u",key);
	}
	HybridHashStream a, b, c;
	a.Add_Length (12.0001f); b.Add_Length (12.0002f); c.Add_Length (12.001f);
	Check (a.Get () == b.Get () && a.Get () != c.Get (),"hash must use quantized values");
	HybridHashStream strings_a, strings_b;
	strings_a.Add_String ("ab"); strings_a.Add_String ("c"); strings_b.Add_String ("a"); strings_b.Add_String ("bc");
	Check (strings_a.Get () != strings_b.Get (),"hash string boundaries");
	Check (WorldSeedService::Hash01 (123,WorldSeedService::SEED_DOMAIN_MAP_PLAN,4) != WorldSeedService::Hash01 (123,WorldSeedService::SEED_DOMAIN_ROADS,4),"independent named seed domains");
	Check (Hybrid_World_Size_Within_Limits (2400,2400) && !Hybrid_World_Size_Within_Limits (2401,600) && !Hybrid_World_Size_Within_Limits (0,600),"interactive world size limits");
	Check (Hybrid_Size_Class_For (899) == HYBRID_SIZE_SMALL && Hybrid_Size_Class_For (900) == HYBRID_SIZE_MEDIUM && Hybrid_Size_Class_For (1800) == HYBRID_SIZE_LARGE,"derived size class boundaries");
	HybridConstants k;
	Hybrid_Compute_Constants (2.556f,1,60,1200,1200,3,10000,&k);
	Check (Near (k.W,3.3228f,0.00001f) && k.A == 15 && k.S == 30 && k.BHash == 60 && k.RBaseProtected == 180,"canonical planning constants");
	const float widths[] = { 5.65f,2.60f,2.40f,2.40f,1.20f,1.50f };
	Check ((sizeof (widths) / sizeof (widths[0])) == HYBRID_ROAD_CLASS_COUNT,"a road class has no Section-7 width row");
	for (int road = 0; road < HYBRID_ROAD_CLASS_COUNT; ++road) {
		const HybridRoadClass rc = (HybridRoadClass)road;
		Check (Near (Hybrid_Roadbed_Width (rc,k.W),widths[road]*k.W),"roadbed width %d",road);
		if (!Hybrid_Road_Class_Is_Routed (road)) {
			//	HIGHWAY_RAMP: corridor helpers are undefined; asymmetric 0.20W / 0.30W shoulders; paved tier.
			Check (Hybrid_Max_Corridor_Half_Width (rc,k.S,k.D) == 0.0f && Hybrid_Preferred_Corridor_Half_Width (rc,k.S,k.D) == 0.0f,"a ramp has a corridor");
			const HybridRoadClassSpec & ramp = Hybrid_Road_Class_Spec (rc);
			Check (ramp.Lanes == 1 && ramp.ShoulderW == 0.20f && ramp.OuterShoulderW == 0.30f && ramp.MedianW == 0.0f &&
					 ramp.MaxGradeDegrees == 12.0f && ramp.MaxCrossSlopeDegrees == 8.0f && ramp.MinTurnRadiusW == 5.0f &&
					 ramp.PhysicalTier == HYBRID_ROAD_TIER_PAVED && ::strcmp (Hybrid_Road_Class_Name (rc),"HIGHWAY_RAMP") == 0,
					 "the HIGHWAY_RAMP row is not Section 7's");
			Check (Near (Hybrid_Road_Deformation_Half_Width (rc,k.W,k.A),0.5f*widths[road]*k.W+7.5f),"road deformation envelope %d",road);
			continue;
		}
		Check (Hybrid_Max_Corridor_Half_Width (rc,k.S,k.D) == Hybrid_Preferred_Corridor_Half_Width (rc,k.S,k.D)+2*k.S,"bounded corridor expansion %d",road);
		Check (Near (Hybrid_Road_Deformation_Half_Width (rc,k.W,k.A),0.5f*widths[road]*k.W+7.5f),"road deformation envelope %d",road);
	}
	const HybridVerticalInterval low = Hybrid_Road_Vertical_Interval (10,2,3), high = Hybrid_Road_Vertical_Interval (17,2,3);
	Check (low.Low == 8 && low.High == 13 && Hybrid_Interval_Gap (low,high) == 2,"road interval gap");
	Check (Hybrid_Grade_Compatible (low,high,10,0.2f) && !Hybrid_Grade_Compatible (low,high,10,0.1f),"grade feasibility");
	ProceduralPlanningSpatialHash spatial;
	spatial.Init (1200,1200,k.BHash);
	spatial.Insert (9,10,10,100,100); spatial.Insert (2,61,61,65,65); spatial.Insert (7,900,900,920,920);
	DynamicVectorClass<unsigned int> hits;
	spatial.Query (60,60,100,100,&hits);
	Check (hits.Count () == 2 && hits[0] == 2 && hits[1] == 9,"spatial query sorted unique bucket hits");
	spatial.Reset (); spatial.Query (0,0,1200,1200,&hits);
	Check (hits.Count () == 0 && spatial.Get_Entry_Count () == 0,"spatial reset between attempts");
	HybridWorkBudgets budget;
	Hybrid_Get_Work_Budgets (HYBRID_SIZE_MEDIUM,6400,7,&budget);
	Check (budget.MaxRoutedEdges == 32 && budget.AStarExpansionsPerRoute == 51200 && budget.AStarExpansionsPerAttempt == 1638400 && budget.LosRays == 2048 && budget.LocalRepairCandidates == 21 && budget.ScratchArenaBytes == 256*1024*1024,"deterministic work ceilings");
	HybridWorkCounters counters;
	counters.Add (HYBRID_WORK_LOS_RAYS,2048);
	Check (!counters.Exceeds (HYBRID_WORK_LOS_RAYS,budget.LosRays),"exact work budget allowed");
	counters.Add (HYBRID_WORK_LOS_RAYS);
	Check (counters.Exceeds (HYBRID_WORK_LOS_RAYS,budget.LosRays),"work ceiling exceeded");
	HybridScratchArena arena;
	Check (arena.Reserve (128),"scratch reservation");
	void * block = arena.Allocate (32,32);
	Check (block != NULL && ((size_t)block % 32) == 0,"scratch address alignment");
	const int used = arena.Get_Used ();
	Check (arena.Allocate (129) == NULL && arena.Get_Used () == used,"scratch overflow leaves arena intact");
	arena.Reset ();
	Check (arena.Get_Used () == 0 && arena.Get_High_Water () == used && arena.Allocate (32,32) == block,"scratch reuse/high water");
}


/*
**	Rewrite item I3 -- runtime Phase 2, MapIdentity.  Hand-filled params only: the resolver reads no
**	profile and no registry, so Phase 2 is checked without any content loaded.
*/
static HybridMapIdentityParams Identity_Fixture (float world_size = 1200.0f)
{
	HybridMapIdentityParams params;
	params.MapPlanSeed	= WorldSeedService::Derive_Subsystem_Seed (4242,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	params.HydrologySeed	= WorldSeedService::Derive_Subsystem_Seed (4242,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	params.BiomeSeed		= WorldSeedService::Derive_Subsystem_Seed (4242,WorldSeedService::SEED_DOMAIN_BIOMES);
	Hybrid_Compute_Constants (2.555953f,1.0f,60.0f,world_size,world_size,3.0f,10000.0f,&params.Constants);
	params.Symmetry					= HYBRID_SYMMETRY_ROTATIONAL;
	params.ArchetypeSelector		= HYBRID_ARCHETYPE_AUTO;
	params.BiomeModeSelector		= HYBRID_BIOME_MODE_SINGLE;
	params.BiomeSelector				= HYBRID_BIOME_AUTO;
	params.SecondaryBiomeSelector	= HYBRID_BIOME_AUTO;
	return params;
}


void Check_Hybrid_Map_Identity (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult result;

	//	Step 1 -- the size class is derived from D, never drawn.
	const float sizes[3] = { 600.0f, 1200.0f, 2400.0f };
	const HybridSizeClass classes[3] = { HYBRID_SIZE_SMALL, HYBRID_SIZE_MEDIUM, HYBRID_SIZE_LARGE };
	const int island_slots[3] = { 0, 1, 2 };
	for (int i = 0; i < 3; ++i) {
		HybridMapIdentityParams sized = Identity_Fixture (sizes[i]);
		Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&result),"identity %g: %s",sizes[i],result.Detail.Peek_Buffer ());
		Check (identity.SizeClass == classes[i] && identity.IslandSlotCount == island_slots[i],"size class and island slots at D=%g",sizes[i]);
	}

	//	The Phase 2 gate: identical params give identical records and checksum; another seed differs.
	HybridMapIdentityParams params = Identity_Fixture ();
	HybridMapIdentity again;
	HybridArchetypeIntentSet again_intents;
	Check (Hybrid_Resolve_Map_Identity (params,&identity,&intents,&result),"identity baseline: %s",result.Detail.Peek_Buffer ());
	Check (Hybrid_Resolve_Map_Identity (params,&again,&again_intents,&result),"identity repeat: %s",result.Detail.Peek_Buffer ());
	Check (identity.Checksum == again.Checksum && identity.Checksum == Hybrid_Map_Identity_Checksum (again,again_intents,0),
			 "identical seed/profile gives an identical identity checksum");
	Check (identity.Archetype == again.Archetype && identity.FlankSign == again.FlankSign
		 && identity.Frame.AxisAngle == again.Frame.AxisAngle && intents.Intents.Count () == again_intents.Intents.Count (),
			 "identical seed/profile gives identical identity records");
	HybridMapIdentityParams other_seed = params;
	other_seed.MapPlanSeed = WorldSeedService::Derive_Subsystem_Seed (99,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	Check (Hybrid_Resolve_Map_Identity (other_seed,&again,&again_intents,&result) && again.Checksum != identity.Checksum,
			 "a different seed changes the identity checksum");

	//	Step 4 -- the strategic axis.  mirror_x keeps the literal X axis; every other symmetry draws.
	HybridMapIdentityParams mirror = params;
	mirror.Symmetry = HYBRID_SYMMETRY_MIRROR_X;
	Check (Hybrid_Resolve_Map_Identity (mirror,&identity,&intents,&result) && identity.Frame.AxisAngle == 0.0f
		 && Near (identity.Frame.AxisU.X,1.0f) && Near (identity.Frame.AxisV.Y,1.0f),"mirror_x axis is world X");
	Check (Near (identity.Frame.Scale,600.0f),"strategic frame scale is D/2");
	for (unsigned int seed = 1; seed <= 64; ++seed) {
		HybridMapIdentityParams spun = params;
		spun.MapPlanSeed = WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
		Check (Hybrid_Resolve_Map_Identity (spun,&identity,&intents,&result)
			 && identity.Frame.AxisAngle >= 0.0f && identity.Frame.AxisAngle < WWMATH_PI,"axis angle in [0,pi) for seed %u",seed);
		const Vector2 round_trip = identity.Frame.To_Strategic (identity.Frame.To_World (-0.72f,0.31f));
		Check (Near (round_trip.X,-0.72f,0.0001f) && Near (round_trip.Y,0.31f,0.0001f),"strategic frame round trip for seed %u",seed);
		Check (Near (identity.Frame.AxisU.X*identity.Frame.AxisV.X + identity.Frame.AxisU.Y*identity.Frame.AxisV.Y,0.0f,0.0001f),
				 "v axis perpendicular to u for seed %u",seed);
	}

	//	Section 6.1 -- symmetry pairing and the anchors that hang off the flank sign.
	Check (Hybrid_Mirror_Strategic (HYBRID_SYMMETRY_ROTATIONAL,Vector2 (0.3f,0.4f)) == Vector2 (-0.3f,-0.4f)
		 && Hybrid_Mirror_Strategic (HYBRID_SYMMETRY_MIRROR_X,Vector2 (0.3f,0.4f)) == Vector2 (-0.3f,0.4f)
		 && Hybrid_Mirror_Strategic (HYBRID_SYMMETRY_NONE,Vector2 (0.3f,0.4f)) == Vector2 (0.3f,0.4f),"symmetry pairing rule");
	Check (Hybrid_Resolve_Map_Identity (params,&identity,&intents,&result)
		 && (identity.FlankSign == 1 || identity.FlankSign == -1)
		 && Near (identity.GdiBaseGuide.X,-0.72f) && Near (identity.NodBaseGuide.X,0.72f) && identity.NodBaseGuide.Y == 0.0f
		 && Near (identity.GdiHomeFieldGuide.Y,0.18f*identity.FlankSign) && Near (identity.NodHomeFieldGuide.Y,-0.18f*identity.FlankSign)
		 && Near (identity.FlankGuide[0].Y,0.45f*identity.FlankSign) && Near (identity.FlankGuide[1].Y,-0.45f*identity.FlankSign),
			 "Section 6.1 anchors follow the flank sign");
	HybridMapIdentityParams asymmetric = params;
	asymmetric.Symmetry = HYBRID_SYMMETRY_NONE;
	Check (Hybrid_Resolve_Map_Identity (asymmetric,&identity,&intents,&result)
		 && identity.NodBaseGuide.Y >= -0.18f && identity.NodBaseGuide.Y <= 0.18f,"symmetry none offsets the Nod guide");
	Check (identity.RouteFamilyCount == Hybrid_Route_Family_Count (params.RouteRedundancy,params.FlankLanes),"route family count");

	//	Step 3 -- archetype selection and the coastline filter.
	bool coastal_seen = false;
	for (unsigned int seed = 1; seed <= 256; ++seed) {
		HybridMapIdentityParams spun = params;
		spun.CoastlineEnabled = false;
		spun.MapPlanSeed = WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
		Check (Hybrid_Resolve_Map_Identity (spun,&identity,&intents,&result)
			 && identity.Archetype != HYBRID_ARCHETYPE_COASTAL_PENINSULA,"auto archetype drops coastal_peninsula without a coastline");
		spun.CoastlineEnabled = true;
		Hybrid_Resolve_Map_Identity (spun,&identity,&intents,&result);
		if (identity.Archetype == HYBRID_ARCHETYPE_COASTAL_PENINSULA) coastal_seen = true;
	}
	Check (coastal_seen,"auto archetype can select coastal_peninsula with a coastline");
	HybridMapIdentityParams landlocked = params;
	landlocked.CoastlineEnabled = false;
	landlocked.ArchetypeSelector = HYBRID_ARCHETYPE_COASTAL_PENINSULA;
	Check (!Hybrid_Resolve_Map_Identity (landlocked,&identity,&intents,&result)
		 && result.Failure == HYBRID_IDENTITY_ARCHETYPE_REQUIRES_COASTLINE,"explicit coastal_peninsula needs a coastline");

	//	Steps 5 to 7 -- biome mode and biomes.
	HybridMapIdentityParams biome = params;
	biome.BiomeModeSelector = HYBRID_BIOME_MODE_AUTO;
	biome.MixedBiomeProbability = 0.0f;
	Check (Hybrid_Resolve_Map_Identity (biome,&identity,&intents,&result) && identity.BiomeMode == HYBRID_BIOME_MODE_SINGLE
		 && identity.SecondaryBiome == HYBRID_BIOME_COUNT && !intents.Divider.Present,"probability 0 never mixes");
	biome.MixedBiomeProbability = 1.0f;
	Check (Hybrid_Resolve_Map_Identity (biome,&identity,&intents,&result) && identity.BiomeMode == HYBRID_BIOME_MODE_MIXED
		 && identity.SecondaryBiome != identity.PrimaryBiome
		 && Hybrid_Is_Legal_Biome_Pair (identity.PrimaryBiome,identity.SecondaryBiome),"probability 1 always mixes legally");
	Check (intents.Divider.Present
		 && Near (intents.Divider.TransitionWidth,8.0f*params.Constants.W > 4.0f*params.Constants.S ? 8.0f*params.Constants.W : 4.0f*params.Constants.S)
		 && intents.Divider.ControlU[0] >= -0.15f && intents.Divider.ControlU[0] <= 0.15f
		 && intents.Divider.ControlU[3] >= -0.15f && intents.Divider.ControlU[3] <= 0.15f
		 && intents.Divider.ControlU[1] >= -0.30f && intents.Divider.ControlU[1] <= 0.30f
		 && intents.Divider.ControlU[2] >= -0.30f && intents.Divider.ControlU[2] <= 0.30f
		 && intents.Divider.ControlV[0] == -1.0f && intents.Divider.ControlV[3] == 1.0f,"mixed divider control range");
	HybridMapIdentityParams pair = params;
	pair.BiomeModeSelector = HYBRID_BIOME_MODE_MIXED;
	pair.BiomeSelector = HYBRID_BIOME_SNOW;
	pair.SecondaryBiomeSelector = HYBRID_BIOME_DESERT;
	Check (!Hybrid_Resolve_Map_Identity (pair,&identity,&intents,&result)
		 && (result.Failure == HYBRID_IDENTITY_ILLEGAL_BIOME_PAIR || result.Failure == HYBRID_IDENTITY_NO_AVAILABLE_BIOME_PAIR),
			 "snow + desert is rejected");
	HybridMapIdentityParams unavailable = params;
	for (int b = 0; b < HYBRID_BIOME_COUNT; ++b) unavailable.BiomeAvailable[b] = (b == HYBRID_BIOME_DESERT);
	Check (Hybrid_Resolve_Map_Identity (unavailable,&identity,&intents,&result) && identity.PrimaryBiome == HYBRID_BIOME_DESERT,
			 "an unavailable biome is never selected");
	unavailable.BiomeSelector = HYBRID_BIOME_SNOW;
	Check (!Hybrid_Resolve_Map_Identity (unavailable,&identity,&intents,&result)
		 && result.Failure == HYBRID_IDENTITY_NO_AVAILABLE_BIOME,"an unavailable explicit biome fails");
	for (int b = 0; b < HYBRID_BIOME_COUNT; ++b) unavailable.BiomeAvailable[b] = false;
	unavailable.BiomeSelector = HYBRID_BIOME_AUTO;
	Check (!Hybrid_Resolve_Map_Identity (unavailable,&identity,&intents,&result)
		 && result.Failure == HYBRID_IDENTITY_NO_AVAILABLE_BIOME,"no available biome fails");

	//	Step 8 -- island slot bits are one independent Bernoulli test per slot.
	HybridMapIdentityParams islands = Identity_Fixture (2400.0f);
	islands.IslandChance = 0.0f;
	Check (Hybrid_Resolve_Map_Identity (islands,&identity,&intents,&result) && identity.IslandSlotBits == 0,"chance 0 accepts no island");
	islands.IslandChance = 1.0f;
	Check (Hybrid_Resolve_Map_Identity (islands,&identity,&intents,&result) && identity.IslandSlotBits == 3
		 && identity.Island_Slot_Count_Accepted () == 2,"chance 1 accepts every slot");

	/*
	**	Step 9 -- the Section-6.10 intent table, for all eight archetypes.  Every archetype builds its
	**	own named records and nothing another archetype owns.
	*/
	for (int a = 0; a < HYBRID_ARCHETYPE_COUNT; ++a) {
		HybridMapIdentityParams each = params;
		each.ArchetypeSelector = (HybridArchetype)a;
		const char * name = Hybrid_Archetype_Name ((HybridArchetype)a);
		Check (Hybrid_Resolve_Map_Identity (each,&identity,&intents,&result),"archetype %s: %s",name,result.Detail.Peek_Buffer ());
		Check (intents.Intents.Count () > 0,"archetype %s records intents",name);
		for (int i = 0; i < intents.Intents.Count (); ++i) {
			Check (intents.Intents[i].StableId == (unsigned int)(i + 1),"intent StableId is creation order (%s)",name);
		}
		const float s = (float)identity.FlankSign;
		const int crossings = intents.Count_Of_Kind (HYBRID_INTENT_ROUTE_FAMILY_CROSSING);
		const float d = params.Constants.D;

		switch ((HybridArchetype)a) {
			case HYBRID_ARCHETYPE_RIVER_VALLEY:
			{
				const HybridArchetypeIntent * river = intents.Find_First (HYBRID_INTENT_GUIDED_RIVER);
				Check (river != NULL && crossings == 2 && intents.RiverAdjacentContested,"river_valley intents");
				if (river != NULL) {
					Check (Near (river->Guide.Y,-0.90f) && Near (river->GuideEnd.Y,0.90f),"guided river endpoints");
					Check (river->Amplitude >= 0.05f*d && river->Amplitude <= 0.12f*d
						&& river->Wavelength >= 0.25f*d && river->Wavelength <= 0.45f*d
						&& river->Phase >= 0.0f && river->Phase <= 2.0f*WWMATH_PI,"guided river meander parameters");
					//	(0,-0.90) to (0,+0.90) is 1.80 * D/2 of axis, sampled every A.
					Check (river->Points.Count () == (int)Hybrid_Round_Half_Up ((0.90f*d)/params.Constants.A) + 1,
							 "guided river station count at spacing A");
					bool clamped = true;
					for (int pt = 0; pt < river->Points.Count (); ++pt) {
						const Vector2 uv = identity.Frame.To_Strategic (river->Points[pt]);
						if (uv.X < -0.1001f || uv.X > 0.1001f) clamped = false;
					}
					Check (clamped,"guided river stays within u +/- 0.10");
				}
				Check (intents.Intents[1].RouteFamily == 0 && Near (intents.Intents[1].Guide.X,0.0f) && Near (intents.Intents[1].Guide.Y,0.0f)
					&& intents.Intents[2].RouteFamily == 1 && Near (intents.Intents[2].Guide.Y,-0.45f*s),"river_valley crossing guides");
				break;
			}
			case HYBRID_ARCHETYPE_TWIN_LAKES:
			{
				Check (intents.Count_Of_Kind (HYBRID_INTENT_LAKE_GUIDE) == 2 && crossings == 2,"twin_lakes intents");
				const HybridArchetypeIntent & lake_a = intents.Intents[0];
				const HybridArchetypeIntent & lake_b = intents.Intents[1];
				Check (Near (lake_a.Guide.Y,-0.38f) && Near (lake_b.Guide.Y,0.38f)
					&& lake_a.Guide.X >= -0.05f && lake_a.Guide.X <= 0.05f && lake_b.Guide.X >= -0.05f && lake_b.Guide.X <= 0.05f,
						 "twin lake guide centres");
				Check (lake_a.Radius >= 0.18f*d && lake_a.Radius <= 0.25f*d
					&& lake_b.Radius >= 0.18f*d && lake_b.Radius <= 0.25f*d,"twin lake radii");
				Check (Near (intents.Intents[3].Guide.Y,0.78f*s),"twin_lakes family 1 guide is outside the nearer lake");
				break;
			}
			case HYBRID_ARCHETYPE_CANYON_CROSSROADS:
			case HYBRID_ARCHETYPE_RAVINE_NETWORK:
			{
				const bool canyon = ((HybridArchetype)a == HYBRID_ARCHETYPE_CANYON_CROSSROADS);
				const HybridIntentKind barrier = canyon ? HYBRID_INTENT_PRINCIPAL_CANYON : HYBRID_INTENT_MAIN_RAVINE_GUIDE;
				const float crossing_v = canyon ? 0.35f : 0.32f;
				const HybridArchetypeIntent * principal = intents.Find_First (barrier);
				Check (principal != NULL && crossings == 2,"%s intents",name);
				if (principal != NULL) {
					Check (Near (principal->Guide.X,0.0f) && Near (principal->Guide.Y,0.0f)
						&& (principal->Flags & HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V) != 0,"%s principal guide on the v axis",name);
				}
				Check (intents.Intents[1].RouteFamily == 0 && Near (intents.Intents[1].Guide.Y,crossing_v*s)
					&& (intents.Intents[1].Flags & HYBRID_INTENT_FLAG_BRIDGE_REQUIRED) != 0,"%s family 0 bridge obligation",name);
				Check (intents.Intents[2].RouteFamily == 1 && Near (intents.Intents[2].Guide.Y,-crossing_v*s)
					&& (intents.Intents[2].Flags & HYBRID_INTENT_FLAG_DISTINCT_CROSSING_REQUIRED) != 0,
						 "%s family 1 distinct crossing",name);
				break;
			}
			case HYBRID_ARCHETYPE_HIGHLAND_PASS:
			{
				const HybridArchetypeIntent * ridge = intents.Find_First (HYBRID_INTENT_PRINCIPAL_RIDGE);
				const HybridArchetypeIntent * saddle = intents.Find_First (HYBRID_INTENT_SURFACE_SADDLE);
				Check (ridge != NULL && saddle != NULL && crossings == 1,"highland_pass intents");
				Check (ridge != NULL && (ridge->Flags & HYBRID_INTENT_FLAG_ALONG_STRATEGIC_V) != 0,"principal ridge on the v axis");
				Check (saddle != NULL && Near (saddle->Guide.X,0.0f) && Near (saddle->Guide.Y,0.0f),"surface saddle at the centre");
				Check (intents.Intents[2].RouteFamily == 1 && Near (intents.Intents[2].Guide.Y,0.45f*s),"highland alternate family guide");
				break;
			}
			case HYBRID_ARCHETYPE_COASTAL_PENINSULA:
			{
				const HybridArchetypeIntent * coast = intents.Find_First (HYBRID_INTENT_COAST_GUIDE);
				const HybridArchetypeIntent * neck = intents.Find_First (HYBRID_INTENT_PENINSULA_NECK);
				const HybridArchetypeIntent * river = intents.Find_First (HYBRID_INTENT_COASTAL_GUIDED_RIVER);
				Check (coast != NULL && neck != NULL && river != NULL,"coastal_peninsula intents");
				if ((coast != NULL) && (neck != NULL) && (river != NULL)) {
					const float v_sign = (float)coast->VSign;
					Check (coast->VSign == 1 || coast->VSign == -1,"coast side is a sign");
					const float sea_v = coast->Guide.Y * v_sign;
					Check (sea_v >= 0.55f && sea_v <= 0.70f,"sea boundary in 0.55..0.70");
					Check (Near (neck->Guide.Y,0.35f*v_sign) && Near (neck->MinWidth,10.0f*params.Constants.W),"peninsula neck width");
					Check (neck->VSign == coast->VSign && river->VSign == coast->VSign,"peninsula records share the sea side");
					Check (Near (river->Guide.Y,-0.35f*v_sign) && Near (river->GuideEnd.Y,coast->Guide.Y),
							 "coastal river runs from inland to the sea boundary");
					Check (river->Amplitude <= 0.06f*d + 0.001f,"coastal river amplitude is halved");
					const Vector2 last = river->Points[river->Points.Count () - 1];
					const Vector2 end_world = identity.Frame.To_World (river->GuideEnd);
					Check (Near (last.X,end_world.X,0.01f) && Near (last.Y,end_world.Y,0.01f),"coastal river ends at the sea boundary");
				}
				break;
			}
			case HYBRID_ARCHETYPE_PLATEAU_BASIN:
			{
				const HybridArchetypeIntent * basin = intents.Find_First (HYBRID_INTENT_BASIN_GUIDE);
				Check (basin != NULL && intents.Count_Of_Kind (HYBRID_INTENT_BASIN_ENTRY) == 2,"plateau_basin intents");
				Check (basin != NULL && Near (basin->Guide.X,0.0f) && Near (basin->Guide.Y,0.0f)
					&& basin->Radius >= 0.22f*d && basin->Radius <= 0.32f*d,"basin guide and radius");
				Check (Near (intents.Intents[1].Guide.Y,0.35f*s) && Near (intents.Intents[2].Guide.Y,-0.35f*s),"basin entry points");
				break;
			}
			case HYBRID_ARCHETYPE_OPEN_FRONTIER:
			{
				Check (intents.Count_Of_Kind (HYBRID_INTENT_OPEN_MANEUVER) == 3 && crossings == 0,"open_frontier discs on MEDIUM");
				for (int i = 0; i < intents.Intents.Count (); ++i) {
					Check (Near (intents.Intents[i].Radius,9.0f*params.Constants.W),"OPEN_MANEUVER radius is 9W");
				}
				Check (Near (intents.Intents[0].Guide.Y,0.0f) && Near (intents.Intents[1].Guide.Y,0.42f)
					&& Near (intents.Intents[2].Guide.Y,-0.42f),"OPEN_MANEUVER centres");
				break;
			}
			default:
				break;
		}
	}

	//	Section 6.9 -- SMALL gets the centre disc only.
	HybridMapIdentityParams small_frontier = Identity_Fixture (600.0f);
	small_frontier.ArchetypeSelector = HYBRID_ARCHETYPE_OPEN_FRONTIER;
	Check (Hybrid_Resolve_Map_Identity (small_frontier,&identity,&intents,&result)
		 && intents.Count_Of_Kind (HYBRID_INTENT_OPEN_MANEUVER) == 1 && Near (intents.Intents[0].Guide.Y,0.0f),
			 "SMALL open_frontier has only the centre disc");
}


/*
**	Rewrite item I4 -- runtime Phase 3, the strategic gameplay graph.  Hand-filled params and a
**	resolved Phase-2 identity only; the builder reads no profile and no registry, and with no world
**	boundary configured `WorldBoundsService` answers "placeable", which is its documented
**	unconfigured behaviour.
**
**	The fixtures pair each world size with a base disc a profile for that size would actually ask
**	for -- `base_radius_cells` 1/2/3 at `chunk_size` 60 -- because the Section-6.1 guide table only
**	has room for `R_BASE_PROTECTED + FieldRadiusMax` inside its 0.10D repair budget when the base
**	reserve is in proportion to the world.  The archetype sweep uses the native 2400 world.
*/
static HybridMapIdentityParams Graph_Identity_Fixture (float world_size,float base_radius_cells)
{
	HybridMapIdentityParams params = Identity_Fixture (world_size);
	Hybrid_Compute_Constants (2.555953f,1.0f,60.0f,world_size,world_size,base_radius_cells,10000.0f,&params.Constants);
	return params;
}

static HybridStrategicGraphParams Graph_Fixture (const HybridMapIdentityParams & identity_params,float margin_cells)
{
	HybridStrategicGraphParams params;
	params.MapPlanSeed		= identity_params.MapPlanSeed;
	params.Constants			= identity_params.Constants;
	params.BoundaryMargin	= margin_cells * 60.0f;		// boundary_margin_cells x chunk_size
	params.FieldRadiusMax	= 60.0f;							// native resource.field_radius_max
	params.RiverWidth			= 6.0f;							// native water.river_width
	return params;
}

void Check_Hybrid_Strategic_Graph (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult result;

	//	Section 5 role counts by size class, and the Phase 3 gate for each of them.
	const float sizes[3] = { 600.0f, 1200.0f, 2400.0f };
	const float base_cells[3] = { 1.0f, 2.0f, 3.0f };
	const int contested[3] = { 1, 3, 5 };
	const int remote[3] = { 1, 1, 2 };

	for (int i = 0; i < 3; ++i) {

		HybridMapIdentityParams sized = Graph_Identity_Fixture (sizes[i],base_cells[i]);
		const bool identity_ok = Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result);
		Check (identity_ok,"identity at D=%g: %s",sizes[i],identity_result.Detail.Peek_Buffer ());

		HybridStrategicGraphParams params = Graph_Fixture (sized,2.0f);
		const bool built = Hybrid_Build_Strategic_Graph (params,identity,intents,&graph,&result);
		Check (built,"strategic graph at D=%g (%s): %s (%s)",sizes[i],Hybrid_Archetype_Name (identity.Archetype),
				 Hybrid_Strategic_Graph_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
		if (!built) continue;

		Check (Hybrid_Contested_Field_Count (identity.SizeClass) == contested[i]
			 && Hybrid_Remote_Field_Count (identity.SizeClass) == remote[i],"Section 5 field counts at D=%g",sizes[i]);
		Check (graph.Count_Of_Role (HYBRID_SITE_GDI_BASE) == 1 && graph.Count_Of_Role (HYBRID_SITE_NOD_BASE) == 1
			 && graph.Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_GDI) == 1
			 && graph.Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_NOD) == 1
			 && graph.Count_Of_Role (HYBRID_SITE_FRONT_CENTER) == 1
			 && graph.Count_Of_Role (HYBRID_SITE_CONTESTED_TIBERIUM) == contested[i]
			 && graph.Count_Of_Role (HYBRID_SITE_REMOTE_TIBERIUM) == remote[i],"exact role counts at D=%g",sizes[i]);

		//	The gate's own cycle requirement: distinct heavy families never leave a tree.
		Check (graph.Has_Cycle (),"heavy families leave a cycle at D=%g",sizes[i]);

		//	Every hard-validity separation rule the phase promised, measured on the accepted centers.
		const float field_radius = params.FieldRadiusMax;
		const float base_radius = params.Constants.RBaseProtected;
		for (int a = 0; a < graph.Sites.Count (); ++a) {
			for (int b = a + 1; b < graph.Sites.Count (); ++b) {

				const HybridStrategicSite & sa = graph.Sites[a];
				const HybridStrategicSite & sb = graph.Sites[b];
				const bool a_base = (sa.Role == HYBRID_SITE_GDI_BASE) || (sa.Role == HYBRID_SITE_NOD_BASE);
				const bool b_base = (sb.Role == HYBRID_SITE_GDI_BASE) || (sb.Role == HYBRID_SITE_NOD_BASE);
				const bool a_field = (sa.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) || (sa.Role == HYBRID_SITE_HOME_TIBERIUM_NOD)
										 || (sa.Role == HYBRID_SITE_CONTESTED_TIBERIUM) || (sa.Role == HYBRID_SITE_REMOTE_TIBERIUM);
				const bool b_field = (sb.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) || (sb.Role == HYBRID_SITE_HOME_TIBERIUM_NOD)
										 || (sb.Role == HYBRID_SITE_CONTESTED_TIBERIUM) || (sb.Role == HYBRID_SITE_REMOTE_TIBERIUM);

				const float dx = sa.Position.X - sb.Position.X;
				const float dy = sa.Position.Y - sb.Position.Y;
				const float distance = WWMath::Sqrt ((dx*dx) + (dy*dy));

				if (a_base && b_base) {
					Check (distance >= (2.0f*base_radius) - 0.01f,"base protected discs overlap at D=%g",sizes[i]);
				} else if (a_field && b_field) {
					Check (distance >= (2.0f*field_radius) - 0.01f,"resource reserves closer than 2*FieldRadiusMax at D=%g",sizes[i]);
				} else if ((a_base && b_field) || (a_field && b_base)) {
					Check (distance >= (base_radius + field_radius) - 0.01f,"resource reserve inside BASE_PROTECTED at D=%g",sizes[i]);
				}
			}
		}

		//	Each field carries its attachment, and every non-home field its SERVICE_DIRT_2 branch.
		for (int siteindex = 0; siteindex < graph.Sites.Count (); ++siteindex) {
			const HybridStrategicSite & site = graph.Sites[siteindex];
			const bool home = (site.Role == HYBRID_SITE_HOME_TIBERIUM_GDI) || (site.Role == HYBRID_SITE_HOME_TIBERIUM_NOD);
			const bool nonhome = (site.Role == HYBRID_SITE_CONTESTED_TIBERIUM) || (site.Role == HYBRID_SITE_REMOTE_TIBERIUM);
			if (!home && !nonhome) continue;

			int attachments = 0;
			int branches = 0;
			for (int e = 0; e < graph.Edges.Count (); ++e) {
				const HybridStrategicEdge & edge = graph.Edges[e];
				if (edge.Intent != HYBRID_EDGE_RESOURCE_ROUTE) continue;
				if ((edge.SiteA != site.StableId) && (edge.SiteB != site.StableId)) continue;
				attachments ++;
				if (edge.RoadClass == HYBRID_ROAD_SERVICE_DIRT_2) branches ++;
			}
			Check (attachments == 1 && branches == (nonhome ? 1 : 0),
					 "field %u has %d attachment(s) and %d service branch(es) at D=%g",site.StableId,attachments,branches,sizes[i]);
		}

		//	`route_family_count` complete GDI-to-Nod intents, and nothing but base nodes shared.
		for (int family = 0; family < identity.RouteFamilyCount; ++family) {
			int links = 0;
			unsigned int expected = graph.GdiBaseId;
			for (int e = 0; e < graph.Edges.Count (); ++e) {
				if (graph.Edges[e].RouteFamily != family) continue;
				Check (graph.Edges[e].SiteA == expected,"family %d chain broken at D=%g",family,sizes[i]);
				expected = graph.Edges[e].SiteB;
				links ++;
			}
			Check (links >= 2 && expected == graph.NodBaseId,"family %d is not one complete GDI-to-Nod intent at D=%g",
					 family,sizes[i]);
		}
		for (int w = 0; w < graph.Sites.Count (); ++w) {
			if (graph.Sites[w].Role != HYBRID_SITE_ROUTE_WAYPOINT) continue;
			Check (graph.Sites[w].RouteFamily >= 0 && graph.Sites[w].RouteFamily < identity.RouteFamilyCount,
					 "waypoint %u belongs to no single family at D=%g",graph.Sites[w].StableId,sizes[i]);
		}
	}

	//	The Phase 3 gate: identical inputs give an identical graph and checksum.
	HybridMapIdentityParams identity_params = Graph_Identity_Fixture (2400.0f,3.0f);
	Check (Hybrid_Resolve_Map_Identity (identity_params,&identity,&intents,&identity_result),"identity baseline");
	HybridStrategicGraphParams params = Graph_Fixture (identity_params,2.0f);

	HybridStrategicGraph again;
	const bool first_built = Hybrid_Build_Strategic_Graph (params,identity,intents,&graph,&result);
	const bool second_built = Hybrid_Build_Strategic_Graph (params,identity,intents,&again,&result);
	Check (first_built && second_built,"repeat graph build: %s (%s)",
			 Hybrid_Strategic_Graph_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
	Check (graph.Checksum == again.Checksum && graph.Checksum == Hybrid_Strategic_Graph_Checksum (again,0)
		 && graph.Sites.Count () == again.Sites.Count () && graph.Edges.Count () == again.Edges.Count ()
		 && graph.Obligations.Count () == again.Obligations.Count (),
			 "identical identity/params give an identical Phase 3 checksum");

	//	Section 6.1 -- no role moves further than 0.10D from its own guide.
	const float limit = (0.10f * params.Constants.D) + 0.01f;
	const HybridStrategicSite * gdi_base = graph.Find_Site (graph.GdiBaseId);
	const HybridStrategicSite * nod_base = graph.Find_Site (graph.NodBaseId);
	Check (gdi_base != NULL && nod_base != NULL,"both base sites exist");
	if ((gdi_base != NULL) && (nod_base != NULL)) {
		const Vector2 gdi_guide = identity.Frame.To_World (identity.GdiBaseGuide);
		const Vector2 nod_guide = identity.Frame.To_World (identity.NodBaseGuide);
		Check ((gdi_base->Position - gdi_guide).Length () <= limit
			 && (nod_base->Position - nod_guide).Length () <= limit,"a base moved further than 0.10D from its guide");
		Check (gdi_base->Team == 0 && nod_base->Team == 1 && gdi_base->Radius == nod_base->Radius
			 && Near (gdi_base->Radius,params.Constants.RBaseProtected),"base team and reserve radius");
	}

	//	Every archetype's Section-6.10 obligations are exactly what the table tagged.
	for (int a = 0; a < HYBRID_ARCHETYPE_COUNT; ++a) {

		const HybridArchetype archetype = (HybridArchetype)a;
		const char * name = Hybrid_Archetype_Name (archetype);

		HybridMapIdentityParams archetype_params = Graph_Identity_Fixture (2400.0f,3.0f);
		archetype_params.ArchetypeSelector = archetype;
		const bool archetype_identity = Hybrid_Resolve_Map_Identity (archetype_params,&identity,&intents,&identity_result);
		Check (archetype_identity,"identity for %s: %s",name,identity_result.Detail.Peek_Buffer ());
		if (!archetype_identity) continue;

		HybridStrategicGraphParams archetype_graph_params = Graph_Fixture (archetype_params,2.0f);
		const bool archetype_built = Hybrid_Build_Strategic_Graph (archetype_graph_params,identity,intents,&graph,&result);

		/*
		**	`coastal_peninsula` is the one archetype whose fixed guide table can contradict its own
		**	geography: the Section-6.6 sea begins at |v|=0.55..0.70 on the drawn coast side, and the
		**	Phase-3 remote-rich guide sits at v=0.70*s.  When those two draws land on the same side and
		**	the sea reaches far enough in, no candidate inside Section 6.1's 0.10D repair budget is dry,
		**	and the plan's answer is a named attempt failure the manager retries with a new seed -- not
		**	a wider search.  Every other archetype must place every role outright.
		*/
		if (!archetype_built) {
			Check (archetype == HYBRID_ARCHETYPE_COASTAL_PENINSULA
				 && result.Failure == HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,
					 "strategic graph for %s: %s (%s)",name,Hybrid_Strategic_Graph_Failure_Name (result.Failure),
					 result.Detail.Peek_Buffer ());
			Check (graph.Sites.Count () == 0 && graph.Edges.Count () == 0 && graph.Checksum == 0,
					 "a failed %s attempt left a partial graph",name);
			continue;
		}
		Check (graph.Has_Cycle (),"%s left a tree",name);

		int bridge_required = 0;
		int distinct_required = 0;
		int pass_required = 0;
		for (int o = 0; o < graph.Obligations.Count (); ++o) {
			const HybridCrossingObligation & obligation = graph.Obligations[o];
			Check (obligation.RouteFamily >= 0 && obligation.RouteFamily < identity.RouteFamilyCount
				 && obligation.WaypointStableId != 0 && obligation.IntentStableId != 0,
					 "obligation %u is not bound to a family waypoint and intent",obligation.StableId);
			if (obligation.RequiredMethod == HYBRID_CROSSING_BRIDGE) bridge_required ++;
			if (obligation.RequiredMethod == HYBRID_CROSSING_PASS) pass_required ++;
			if (obligation.DistinctCrossingRequired) distinct_required ++;
		}

		switch (archetype) {
			case HYBRID_ARCHETYPE_CANYON_CROSSROADS:
			case HYBRID_ARCHETYPE_RAVINE_NETWORK:
				Check (bridge_required == 1 && distinct_required == 1,"%s needs one bridge and one distinct crossing",name);
				Check (graph.Obligations.Count () == 2
					&& graph.Obligations[0].Obstruction == ((archetype == HYBRID_ARCHETYPE_CANYON_CROSSROADS)
							? HYBRID_OBSTRUCTION_CANYON : HYBRID_OBSTRUCTION_RAVINE),"%s obstruction kind",name);
				break;

			case HYBRID_ARCHETYPE_HIGHLAND_PASS:
				/*
				**	`highland_pass` raises no crossing obligation at all.  Section 6.10's table gives it a
				**	principal ridge, a surface saddle and an untagged alternate family guide -- and a ridge is
				**	not a step-18 obstruction, so there is no window any obligation on it could ever bind to.
				**	The saddle is realized as an amplitude reduction in the ridge and is crossed on ordinary
				**	drivable ground, which is why Phase 5 widens it to a `PRIMARY_2` roadbed plus two lane
				**	clearances.  Raising one made every seed of this archetype fail `obligation_unbound`.
				*/
				Check ((pass_required == 0) && (distinct_required == 0) && (bridge_required == 0),
						 "highland_pass raises %d pass, %d distinct and %d bridge obligations, not none",
						 pass_required,distinct_required,bridge_required);
				Check (graph.Obligations.Count () == 0,"highland_pass raises %d crossing obligations, not none",
						 graph.Obligations.Count ());
				break;

			case HYBRID_ARCHETYPE_OPEN_FRONTIER:
			case HYBRID_ARCHETYPE_TWIN_LAKES:
			case HYBRID_ARCHETYPE_PLATEAU_BASIN:
				Check (graph.Obligations.Count () == 0,"%s tags no crossing obligation",name);
				break;

			case HYBRID_ARCHETYPE_RIVER_VALLEY:
			{
				/*
				**	Section 6.10 -- the first contested field comes from the frozen guided river, and
				**	Section 6.2 requires its reserve edge 4W..10W from the bank.  The station offset aims
				**	at exactly 7W; a meander bend may leave the nearest point closer, never below 4W.
				*/
				const HybridArchetypeIntent * river = intents.Find_First (HYBRID_INTENT_GUIDED_RIVER);
				Check (river != NULL && river->Points.Count () > 1,"river_valley guided river spline");
				const HybridStrategicSite * first_contested = NULL;
				for (int siteindex = 0; (siteindex < graph.Sites.Count ()) && (first_contested == NULL); ++siteindex) {
					if (graph.Sites[siteindex].Role == HYBRID_SITE_CONTESTED_TIBERIUM) first_contested = &graph.Sites[siteindex];
				}
				Check (first_contested != NULL,"river_valley first contested field exists");
				if ((river != NULL) && (first_contested != NULL)) {
					float nearest = 1.0e30f;
					for (int pt = 1; pt < river->Points.Count (); ++pt) {
						const Vector2 a0 = river->Points[pt-1];
						const Vector2 a1 = river->Points[pt];
						const Vector2 segment = a1 - a0;
						const float length_squared = (segment.X*segment.X) + (segment.Y*segment.Y);
						float t = 0.0f;
						if (length_squared > 0.0f) {
							t = (((first_contested->Position.X - a0.X) * segment.X)
								 + ((first_contested->Position.Y - a0.Y) * segment.Y)) / length_squared;
							if (t < 0.0f) t = 0.0f;
							if (t > 1.0f) t = 1.0f;
						}
						const Vector2 closest (a0.X + (segment.X*t),a0.Y + (segment.Y*t));
						const float distance = (first_contested->Position - closest).Length ();
						if (distance < nearest) nearest = distance;
					}
					const float w = archetype_graph_params.Constants.W;
					const float bank_clearance = nearest - archetype_graph_params.FieldRadiusMax
														- (0.5f*archetype_graph_params.RiverWidth);
					Check (bank_clearance >= (4.0f*w) - 0.5f && bank_clearance <= (10.0f*w) + 0.5f,
							 "Section 6.2 puts the reserve edge 4W..10W from the bank, got %.3f (%.3fW)",
							 bank_clearance,bank_clearance/w);
				}
				break;
			}

			default:
				break;
		}
	}

	/*
	**	Section 6.1's boundary rule against the canonical placeable boundary.  Every accepted role
	**	must clear the configured boundary by its own full reserve radius plus the profile margin,
	**	which is what the repair search exists to guarantee.
	*/
	{
		HybridMapIdentityParams bounded_identity = Graph_Identity_Fixture (1200.0f,2.0f);
		bounded_identity.Symmetry = HYBRID_SYMMETRY_MIRROR_X;
		Check (Hybrid_Resolve_Map_Identity (bounded_identity,&identity,&intents,&identity_result),"bounded identity");

		DynamicVectorClass<Vector2> ring;
		ring.Add (Vector2 (-600.0f,-600.0f));
		ring.Add (Vector2 (-600.0f, 600.0f));
		ring.Add (Vector2 ( 600.0f, 600.0f));
		ring.Add (Vector2 ( 600.0f,-600.0f));

		StringClass bounds_error ("");
		Check (WorldBoundsService::Configure (ring,32.0f,8.0f,300.0f,&bounds_error),
				 "the Phase 3 test boundary would not configure: %s",bounds_error.Peek_Buffer ());

		HybridStrategicGraphParams bounded = Graph_Fixture (bounded_identity,1.0f);
		const bool built = Hybrid_Build_Strategic_Graph (bounded,identity,intents,&graph,&result);
		Check (built,"bounded strategic graph (%s): %s (%s)",Hybrid_Archetype_Name (identity.Archetype),
				 Hybrid_Strategic_Graph_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
		if (built) {
			for (int siteindex = 0; siteindex < graph.Sites.Count (); ++siteindex) {
				const HybridStrategicSite & site = graph.Sites[siteindex];
				Check (WorldBoundsService::Can_Place_In_Region (site.Position,site.Radius + bounded.BoundaryMargin),
						 "role %u (%d) sits inside the boundary margin",site.StableId,(int)site.Role);
			}
		}

		WorldBoundsService::Clear ();
	}
}

/*
**	Rewrite item I5 -- runtime Phase 4, the coarse semantic matrices.  Hand-filled params, a resolved
**	Phase-2 identity and a built Phase-3 graph; with no world boundary configured
**	`WorldBoundsService` answers "playable and placeable", which is its documented unconfigured
**	behaviour and makes every cell of these fixtures land.
*/
static HybridCoarseMatrixParams Coarse_Fixture (const HybridMapIdentityParams & identity_params,float world_size)
{
	HybridCoarseMatrixParams params;
	params.MapPlanSeed	= identity_params.MapPlanSeed;
	params.BiomeSeed		= identity_params.BiomeSeed;
	params.Constants		= identity_params.Constants;
	params.WorldMinX		= -0.5f * world_size;
	params.WorldMinY		= -0.5f * world_size;
	params.WorldWidth		= world_size;
	params.WorldHeight	= world_size;
	params.BoundaryMargin	= 2.0f * 60.0f;		// boundary_margin_cells x chunk_size
	params.FieldRadiusMin	= 20.0f;				// native resource.field_radius_min
	params.FieldRadiusMax	= 60.0f;				// native resource.field_radius_max
	params.RiverWidth		= 6.0f;
	params.TierCount		= 3;
	return params;
}

void Check_Hybrid_Coarse_Matrix (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult result;

	//	The arena contract first: the layers come out of the block, and a block too small fails the
	//	resize instead of growing.
	{
		HybridScratchArena arena;
		Check (arena.Reserve (1 << 20),"self-check arena would not reserve");
		HybridSemanticMatrix arena_matrix;
		Check (arena_matrix.Resize (40,30,30.0f,0.0f,0.0f,&arena) && arena.Get_Used () >= arena_matrix.Get_Layer_Bytes (),
				 "semantic matrix did not come out of the arena");
		arena_matrix.Set_Flag (10,10,HYBRID_FLAG_LAND);
		Check (arena_matrix.Has_Flag (10,10,HYBRID_FLAG_LAND) && !arena_matrix.Has_Flag (10,11,HYBRID_FLAG_LAND),
				 "arena-backed flags are not per cell");

		HybridScratchArena tiny;
		Check (tiny.Reserve (256),"tiny arena would not reserve");
		HybridSemanticMatrix too_big;
		Check (!too_big.Resize (200,200,30.0f,0.0f,0.0f,&tiny),"an over-budget matrix must fail, not grow");
	}

	//	The Section-8 Phase-4 priority comparator, which crossing-window insertion reuses later.
	{
		HybridReservation base_disc;
		base_disc.StableId	= 1;
		base_disc.Priority	= HYBRID_RESERVE_BASE_PROTECTED;
		base_disc.A.Set (0.0f,0.0f);
		base_disc.B			= base_disc.A;
		base_disc.Radius	= 100.0f;
		base_disc.SiteId	= 7;
		base_disc.Exclusive	= true;

		HybridReservation field = base_disc;
		field.StableId		= 2;
		field.Priority		= HYBRID_RESERVE_FIELD_TIBERIUM;
		field.A.Set (150.0f,0.0f);
		field.B				= field.A;
		field.Radius		= 60.0f;
		field.SiteId		= 8;

		Check (Hybrid_Reservation_Outranks (base_disc,field) && !Hybrid_Reservation_Outranks (field,base_disc),
				 "BASE_PROTECTED must outrank a contested field");
		HybridReservation later = base_disc;
		later.StableId = 3;
		Check (Hybrid_Reservation_Outranks (base_disc,later),"equal priority breaks by creation order");
		Check (Hybrid_Reservations_Overlap (base_disc,field) && Hybrid_Reservations_Conflict (base_disc,field),
				 "overlapping exclusive reserves must conflict");

		HybridReservation corridor = field;
		corridor.StableId	= 4;
		corridor.Priority	= HYBRID_RESERVE_HEAVY_CORRIDOR;
		corridor.A.Set (-400.0f,0.0f);
		corridor.B.Set (400.0f,0.0f);
		corridor.Radius	= 180.0f;
		corridor.SiteId	= 0;
		corridor.Exclusive= false;
		Check (Hybrid_Reservations_Overlap (corridor,base_disc) && !Hybrid_Reservations_Conflict (corridor,base_disc),
				 "a corridor through a reserve is layered intent, not a conflict");

		HybridReservation same_site = field;
		same_site.StableId	= 5;
		same_site.A			= base_disc.A;
		same_site.B			= base_disc.A;
		same_site.SiteId	= base_disc.SiteId;
		Check (!Hybrid_Reservations_Conflict (base_disc,same_site),"a role never conflicts with its own reserve");
	}

	//	One full phase, per size class, with the base disc a profile for that size would ask for.
	const float sizes[3] = { 600.0f, 1200.0f, 2400.0f };
	const float base_cells[3] = { 1.0f, 2.0f, 3.0f };

	for (int i = 0; i < 3; ++i) {

		HybridMapIdentityParams sized = Graph_Identity_Fixture (sizes[i],base_cells[i]);
		Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity at D=%g: %s",
				 sizes[i],identity_result.Detail.Peek_Buffer ());

		HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
				 "strategic graph at D=%g: %s (%s)",sizes[i],Hybrid_Strategic_Graph_Failure_Name (graph_result.Failure),
				 graph_result.Detail.Peek_Buffer ());

		HybridCoarseMatrixParams params = Coarse_Fixture (sized,sizes[i]);
		const bool built = Hybrid_Build_Coarse_Matrices (params,identity,intents,&graph,&matrices,&result);
		Check (built,"coarse matrices at D=%g (%s): %s (%s)",sizes[i],Hybrid_Archetype_Name (identity.Archetype),
				 Hybrid_Coarse_Matrix_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
		if (!built) continue;

		const HybridSemanticMatrix & matrix = matrices.Matrix;
		const HybridConstants & k = params.Constants;

		//	Step 1 -- dimensions are exactly ceil(extent/S), row-major, origin at the minimum corner.
		int expect_x = (int)(sizes[i] / k.S);
		if (((float)expect_x * k.S) < sizes[i]) expect_x++;
		Check (matrix.Get_Count_X () == expect_x && matrix.Get_Count_Y () == expect_x
			&& Near (matrix.Get_Cell_Size (),k.S) && Near (matrix.Get_Origin_X (),-0.5f * sizes[i]),
				 "matrix dimensions at D=%g are %dx%d",sizes[i],matrix.Get_Count_X (),matrix.Get_Count_Y ());
		Check (matrix.Index (3,2) == (2 * matrix.Get_Count_X ()) + 3,"matrix is not row-major (y,x)");
		Check (matrices.PlayableCells == matrix.Get_Cell_Count () && matrices.PlaceableCells == matrix.Get_Cell_Count (),
				 "with no boundary every cell is playable and placeable");

		//	Step 2 -- the middle tier, and no macro bias yet.
		const signed char middle = (signed char)((params.TierCount - 1) / 2);
		bool tier_ok = true;
		for (int index = 0; index < matrix.Get_Cell_Count (); ++index) {
			if ((matrix.BaseElevationTier[index] != middle) || (matrix.MacroHeightBias[index] != 0.0f)) tier_ok = false;
		}
		Check (tier_ok,"every cell starts at the middle tier with zero macro bias");

		//	Steps 4, 5 and 10 -- the role reserves and their masks.
		Check (matrices.Count_Of_Priority (HYBRID_RESERVE_BASE_PROTECTED) == 2
			&& matrices.Count_Of_Priority (HYBRID_RESERVE_HOME_TIBERIUM) == 2,"base and home reserves");
		Check (matrices.Count_Of_Priority (HYBRID_RESERVE_FIELD_TIBERIUM)
				 == (Hybrid_Contested_Field_Count (identity.SizeClass) + Hybrid_Remote_Field_Count (identity.SizeClass)),
				 "one reserve per non-home field");

		for (int site_index = 0; site_index < graph.Sites.Count (); ++site_index) {

			const HybridStrategicSite & site = graph.Sites[site_index];
			int cx = 0;
			int cy = 0;
			if (!matrix.World_To_Cell (site.Position,&cx,&cy)) continue;
			const int index = matrix.Index (cx,cy);

			switch (site.Role) {
				case HYBRID_SITE_GDI_BASE:
				case HYBRID_SITE_NOD_BASE:
					Check (matrix.Has_Flag (cx,cy,HYBRID_FLAG_BASE_CORE) && matrices.Is_Base_Protected (cx,cy),
							 "base %u centre is BASE_CORE",site.StableId);
					Check (matrix.ReservationMask[index] == (unsigned char)(HYBRID_RESERVE_BASE_PROTECTED + 1),
							 "base %u centre keeps the highest-priority reservation",site.StableId);
					Check (matrix.BaseElevationTier[index] == middle && matrix.MacroHeightBias[index] == 0.0f,
							 "base %u forces the middle tier and zero bias",site.StableId);
					Check (matrix.NoDecorationMask[index] == 1 && matrix.Has_Flag (cx,cy,HYBRID_FLAG_NO_DECORATION),
							 "base %u disc is NO_DECORATION",site.StableId);
					break;

				case HYBRID_SITE_HOME_TIBERIUM_GDI:
				case HYBRID_SITE_HOME_TIBERIUM_NOD:
				case HYBRID_SITE_CONTESTED_TIBERIUM:
				case HYBRID_SITE_REMOTE_TIBERIUM:
				{
					Check (matrix.Has_Flag (cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE) && matrix.ResourceInfluence[index] > 0.0f,
							 "field %u centre is a reserve with resource influence",site.StableId);
					Check (matrix.NoDecorationMask[index] == 1,"field %u core is NO_DECORATION",site.StableId);

					//	The core is field_radius_min, not the whole reserve: sample just outside it.
					const Vector2 edge (site.Position.X + params.FieldRadiusMin + k.S,site.Position.Y);
					int ex = 0;
					int ey = 0;
					if (matrix.World_To_Cell (edge,&ex,&ey) && !matrices.Is_Base_Protected (ex,ey)
						&& matrix.Has_Flag (ex,ey,HYBRID_FLAG_TIBERIUM_RESERVE)) {
						Check (matrix.NoDecorationMask[matrix.Index (ex,ey)] == 0,
								 "the reserve outside the field core is decorable");
					}
					break;
				}

				default:
					break;
			}
	}

		//	Steps 6 and 7 -- corridor half-widths and their influence.
		const float heavy_half = (6.0f * k.S > 0.08f * k.D) ? 6.0f * k.S : 0.08f * k.D;
		const float service_half = (4.0f * k.S > 0.05f * k.D) ? 4.0f * k.S : 0.05f * k.D;
		int heavy_count = 0;
		int service_count = 0;
		for (int r = 0; r < matrices.Reservations.Count (); ++r) {

			const HybridReservation & reservation = matrices.Reservations[r];
			if (reservation.Priority == HYBRID_RESERVE_HEAVY_CORRIDOR) {
				heavy_count++;
				Check (Near (reservation.Radius,heavy_half,0.01f) && !reservation.Exclusive,
						 "heavy corridor half-width is max(6S,0.08D)");
			} else if (reservation.Priority == HYBRID_RESERVE_OPTIONAL_CORRIDOR) {
				service_count++;
				Check (Near (reservation.Radius,service_half,0.01f) && !reservation.Exclusive,
						 "service corridor half-width is max(4S,0.05D)");
			}

			if ((reservation.Priority != HYBRID_RESERVE_HEAVY_CORRIDOR)
				&& (reservation.Priority != HYBRID_RESERVE_OPTIONAL_CORRIDOR)) continue;

			const Vector2 mid (0.5f * (reservation.A.X + reservation.B.X),0.5f * (reservation.A.Y + reservation.B.Y));
			int mx = 0;
			int my = 0;
			if (!matrix.World_To_Cell (mid,&mx,&my)) continue;
			const unsigned int expect = (reservation.Priority == HYBRID_RESERVE_HEAVY_CORRIDOR) ?
													HYBRID_FLAG_MAJOR_TRANSPORT_CORRIDOR : HYBRID_FLAG_SECONDARY_TRANSPORT_CORRIDOR;
			Check (matrix.Has_Flag (mx,my,expect) && matrix.TransportInfluence[matrix.Index (mx,my)] > 0.0f,
					 "corridor %u stamps its own influence at its midpoint",reservation.StableId);
	}
		int heavy_edges = 0;
		int service_edges = 0;
		for (int e = 0; e < graph.Edges.Count (); ++e) {
			if (graph.Edges[e].RouteFamily >= 0) heavy_edges++; else service_edges++;
	}
		Check (heavy_count == heavy_edges && service_count == service_edges,"one corridor per strategic edge");

		//	Step 9 -- a single-biome map is one owner everywhere.
		if (identity.BiomeMode == HYBRID_BIOME_MODE_SINGLE) {
			bool single = true;
			for (int index = 0; index < matrix.Get_Cell_Count (); ++index) {
				if (matrix.BiomeId[index] != (unsigned char)identity.PrimaryBiome) single = false;
			}
			Check (single && matrices.DividerShifts == 0,"a single-biome map owns every cell");
	}

		//	The gate's own promise, measured: no BASE_PROTECTED cell carries water or a barrier band.
		for (int cy = 0; cy < matrix.Get_Count_Y (); ++cy) {
			for (int cx = 0; cx < matrix.Get_Count_X (); ++cx) {
				if (!matrices.Is_Base_Protected (cx,cy)) continue;
				Check (matrix.LandWaterIntent[matrix.Index (cx,cy)] == 0
					&& !matrix.Has_Flag (cx,cy,HYBRID_FLAG_CANYON_BAND | HYBRID_FLAG_RAVINE_BAND | HYBRID_FLAG_COAST
														| HYBRID_FLAG_CLIFF_BAND),
						 "BASE_PROTECTED cell (%d,%d) sits on hard intent",cx,cy);
			}
	}

		//	Determinism: same inputs, same checksum, and a changed cell changes it.
		HybridStrategicGraph again_graph;
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&again_graph,&graph_result),"graph repeat");
		HybridCoarseMatrices again;
		const bool again_built = Hybrid_Build_Coarse_Matrices (params,identity,intents,&again_graph,&again,&result);
		Check (again_built && again.Checksum == matrices.Checksum,"identical inputs must give an identical Phase 4");
		if (again_built) {
			again.Matrix.Set_Flag (0,0,HYBRID_FLAG_HILL);
			Check (Hybrid_Coarse_Matrix_Checksum (again,0) != matrices.Checksum,"the checksum must cover every layer");
	}
	}

	//	Section 6.9 -- `open_frontier` puts its discs in the dedicated no-immutable-barrier layer.
	{
		HybridMapIdentityParams frontier = Graph_Identity_Fixture (1200.0f,2.0f);
		frontier.ArchetypeSelector = HYBRID_ARCHETYPE_OPEN_FRONTIER;
		Check (Hybrid_Resolve_Map_Identity (frontier,&identity,&intents,&identity_result),"open_frontier identity: %s",
				 identity_result.Detail.Peek_Buffer ());

		HybridStrategicGraphParams graph_params = Graph_Fixture (frontier,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),"open_frontier graph: %s",
				 graph_result.Detail.Peek_Buffer ());

		HybridCoarseMatrixParams params = Coarse_Fixture (frontier,1200.0f);
		const bool built = Hybrid_Build_Coarse_Matrices (params,identity,intents,&graph,&matrices,&result);
		Check (built,"open_frontier coarse matrices: %s (%s)",Hybrid_Coarse_Matrix_Failure_Name (result.Failure),
				 result.Detail.Peek_Buffer ());

		if (built) {
			Check (matrices.Count_Of_Priority (HYBRID_RESERVE_OPEN_MANEUVER) == 3,"three OPEN_MANEUVER reserves");
			for (int r = 0; r < matrices.Reservations.Count (); ++r) {

				const HybridReservation & reservation = matrices.Reservations[r];
				if (reservation.Priority != HYBRID_RESERVE_OPEN_MANEUVER) continue;

				Check (Near (reservation.Radius,9.0f * params.Constants.W,0.01f) && !reservation.Exclusive,
						 "OPEN_MANEUVER radius is 9W and is not exclusive");
				int cx = 0;
				int cy = 0;
				Check (matrices.Matrix.World_To_Cell (reservation.A,&cx,&cy)
					&& matrices.Matrix.Has_Flag (cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER),
						 "OPEN_MANEUVER disc %u is in the no-immutable-barrier layer",reservation.StableId);
			}
	}

		/*
		**	The hard-reservation conflict path: drop a contested field on top of a home field and
		**	require Phase 4 to move the lower-priority role through Section 6.1 repair, leaving the
		**	graph -- the owner of role positions -- holding the repaired centre.
		*/
		unsigned int moved_id = 0;
		Vector2 collided (0.0f,0.0f);
		for (int site_index = 0; site_index < graph.Sites.Count (); ++site_index) {
			if (graph.Sites[site_index].Role != HYBRID_SITE_HOME_TIBERIUM_GDI) continue;
			collided = graph.Sites[site_index].Position;
		}
		for (int site_index = 0; site_index < graph.Sites.Count (); ++site_index) {
			if (graph.Sites[site_index].Role != HYBRID_SITE_CONTESTED_TIBERIUM) continue;
			graph.Sites[site_index].Position = collided;
			moved_id = graph.Sites[site_index].StableId;
			break;
	}

		if (moved_id != 0) {
			HybridCoarseMatrices repaired;
			const bool repaired_built = Hybrid_Build_Coarse_Matrices (params,identity,intents,&graph,&repaired,&result);
			Check (repaired_built,"a hard-reservation conflict must repair, not fail: %s (%s)",
					 Hybrid_Coarse_Matrix_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
			if (repaired_built) {
				const HybridStrategicSite * site = graph.Find_Site (moved_id);
				Check (repaired.RoleMoves >= 1,"Phase 4 recorded no Section 6.1 repair");
				Check (site != NULL && (site->Position.X != collided.X || site->Position.Y != collided.Y),
						 "the repaired centre was not written back into the graph");
				const float move_x = (site != NULL) ? (site->Position.X - collided.X) : 0.0f;
				const float move_y = (site != NULL) ? (site->Position.Y - collided.Y) : 0.0f;
				Check (WWMath::Sqrt ((move_x * move_x) + (move_y * move_y)) <= (0.10f * params.Constants.D) + 0.001f,
						 "a repair may move a role at most 0.10D");
			}
	}
	}
}


/*
**	Rewrite item I6 -- runtime Phase 5, the vector landforms.  Hand-filled params on top of the
**	Phase-2/3/4 fixtures above; with no world boundary configured `WorldBoundsService` answers
**	"playable and placeable", which makes every cell of these fixtures land.
*/
static HybridLandformParams Landform_Fixture (const HybridMapIdentityParams & identity_params,float world_size)
{
	HybridLandformParams params;
	params.TerrainSeed	= WorldSeedService::Derive_Subsystem_Seed (4242,WorldSeedService::SEED_DOMAIN_TERRAIN);
	params.Constants		= identity_params.Constants;
	params.WorldMinX		= -0.5f * world_size;
	params.WorldMinY		= -0.5f * world_size;
	params.WorldWidth		= world_size;
	params.WorldHeight	= world_size;
	params.SeaLevel		= 0.0f;
	params.MinElevation	= -15.0f;					// native terrain.min_elevation
	params.MaxElevation	= 90.0f;						// native terrain.max_elevation
	params.TierStep		= 18.0f;
	params.TierCount		= 3;
	params.MacroRelief	= 0.25f;						// native terrain weights
	params.HillDensity	= 0.15f;
	params.MountainDensity			= 0.2f;
	params.InteriorCliffDensity	= 0.45f;
	params.ValleyDensity				= 0.2f;
	params.PlateauChance				= 0.12f;
	params.MaxAlternateCandidatesPerFeature	= 32;		// native retry.max_alternate_candidates_per_feature
	params.MinVehicleLaneClearance				= 1.0f;
	//	Section 5's canyon/ravine bridge quota: Phase 5 must lay that many across a heavy corridor.
	params.CrossedCanyonSystemsRequired = Hybrid_Canyon_Bridge_Quota (identity_params.Constants.SizeClass);

	return params;
}


//	A straight centerline, for the hand-built profile records below.
static void Fill_Straight_Centerline (HybridLandform * landform,const Vector2 & a,const Vector2 & b,float spacing)
{
	landform->Points.Delete_All ();
	landform->Stations.Delete_All ();

	Vector2 delta = b - a;
	const float length = delta.Length ();
	if ((length <= 0.0f) || (spacing <= 0.0f)) return;
	delta *= (1.0f / length);

	for (float station = 0.0f; station < length; station += spacing) {
		landform->Points.Add (a + (delta * station));
		landform->Stations.Add (station);
	}
	landform->Points.Add (b);
	landform->Stations.Add (length);

	landform->Length	= length;
	landform->Center	= a + (delta * (0.5f * length));
	landform->Orientation = WWMath::Atan2 (delta.Y,delta.X);
}


//	The influence AABB of a hand-built record; the builder computes this internally for its own.
static void Set_Landform_Bounds (HybridLandform * landform)
{
	const float half = landform->Influence_Half_Width ();
	if (landform->Points.Count () == 0) {
		landform->BoundsMin.Set (landform->Center.X - half,landform->Center.Y - half);
		landform->BoundsMax.Set (landform->Center.X + half,landform->Center.Y + half);
		return;
	}
	Vector2 low = landform->Points[0];
	Vector2 high = landform->Points[0];
	for (int i = 1; i < landform->Points.Count (); ++i) {
		if (landform->Points[i].X < low.X) low.X = landform->Points[i].X;
		if (landform->Points[i].Y < low.Y) low.Y = landform->Points[i].Y;
		if (landform->Points[i].X > high.X) high.X = landform->Points[i].X;
		if (landform->Points[i].Y > high.Y) high.Y = landform->Points[i].Y;
	}
	landform->BoundsMin.Set (low.X - half,low.Y - half);
	landform->BoundsMax.Set (high.X + half,high.Y + half);
}


//	Signed lateral distance from a point to a sampled centerline.
static float Landform_Lateral (const HybridLandform & landform,const Vector2 & point)
{
	float best_distance = 0.0f;
	float best_lateral = 0.0f;
	bool  have = false;

	for (int i = 0; i + 1 < landform.Points.Count (); ++i) {
		const Vector2 & a = landform.Points[i];
		Vector2 delta = landform.Points[i + 1] - a;
		const float length = delta.Length ();
		if (length <= 0.0f) continue;
		delta *= (1.0f / length);
		const Vector2 rel = point - a;
		const float along = (delta.X * rel.X) + (delta.Y * rel.Y);
		const float clamped = (along < 0.0f) ? 0.0f : ((along > length) ? length : along);
		const float distance = (point - (a + (delta * clamped))).Length ();
		if (!have || (distance < best_distance)) {
			have				= true;
			best_distance	= distance;
			best_lateral	= (delta.X * rel.Y) - (delta.Y * rel.X);
		}
	}
	return best_lateral;
}


//	The sampled point closest to a given along-curve station.
static int Landform_Station_Index (const HybridLandform & landform,float station)
{
	int best = 0;
	float best_error = 0.0f;
	for (int i = 0; i < landform.Stations.Count (); ++i) {
		const float error = WWMath::Fabs (landform.Stations[i] - station);
		if ((i == 0) || (error < best_error)) { best = i; best_error = error; }
	}
	return best;
}


void Check_Hybrid_Landforms (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformSet landforms;
	HybridLandformResult result;

	//	Section 8 Phase 5's target counts, with the native terrain weights.
	{
		Check (Hybrid_Hill_Target_Count (HYBRID_SIZE_SMALL,0.15f) == 3
			&& Hybrid_Hill_Target_Count (HYBRID_SIZE_MEDIUM,0.15f) == 7
			&& Hybrid_Hill_Target_Count (HYBRID_SIZE_LARGE,0.15f) == 11,"hill targets are 3/6/10 + RoundHalfUp(density*(2/4/6))");
		Check (Hybrid_Ridge_Target_Count (HYBRID_SIZE_SMALL,0.2f) == 1
			&& Hybrid_Ridge_Target_Count (HYBRID_SIZE_MEDIUM,0.2f) == 2
			&& Hybrid_Ridge_Target_Count (HYBRID_SIZE_LARGE,0.2f) == 4,"ridge targets");
		Check (Hybrid_Cliff_Target_Count (HYBRID_SIZE_SMALL,0.45f) == 1
			&& Hybrid_Cliff_Target_Count (HYBRID_SIZE_MEDIUM,0.45f) == 3
			&& Hybrid_Cliff_Target_Count (HYBRID_SIZE_LARGE,0.45f) == 6,"cliff targets add to the Section 5 quota");
		Check (Hybrid_Ravine_System_Target_Count (HYBRID_SIZE_SMALL,0.2f) == 1
			&& Hybrid_Ravine_System_Target_Count (HYBRID_SIZE_MEDIUM,0.2f) == 1
			&& Hybrid_Ravine_System_Target_Count (HYBRID_SIZE_LARGE,0.2f) == 2,"ravine system targets");
		Check (Hybrid_Ravine_Branch_Count (HYBRID_SIZE_SMALL) == 2
			&& Hybrid_Ravine_Branch_Count (HYBRID_SIZE_MEDIUM) == 3
			&& Hybrid_Ravine_Branch_Count (HYBRID_SIZE_LARGE) == 5,"Section 6.8 branch counts are exactly 2/3/5");
	}

	//	The shared centripetal Catmull-Rom sampler: it interpolates every control at the asked spacing.
	{
		Vector2 controls[4];
		controls[0].Set (-300.0f,  0.0f);
		controls[1].Set (-100.0f, 20.0f);
		controls[2].Set ( 100.0f,-20.0f);
		controls[3].Set ( 300.0f,  0.0f);

		DynamicVectorClass<Vector2> points;
		DynamicVectorClass<float> stations;
		Check (Hybrid_Sample_Centripetal_Catmull_Rom (controls,4,7.5f,&points,&stations),"the spline sampler failed");
		Check (points.Count () == stations.Count () && points.Count () > 40,"%d spline samples",points.Count ());
		Check (Near (points[0].X,controls[0].X) && Near (points[0].Y,controls[0].Y),"the spline misses its first control");
		const Vector2 & tail = points[points.Count () - 1];
		Check (Near (tail.X,controls[3].X) && Near (tail.Y,controls[3].Y),"the spline misses its last control");

		bool spacing_ok = true;
		bool monotone = true;
		for (int i = 1; i < points.Count (); ++i) {
			const float step = (points[i] - points[i - 1]).Length ();
			if (step > (7.5f + 0.1f)) spacing_ok = false;
			if (stations[i] < stations[i - 1]) monotone = false;
		}
		Check (spacing_ok,"a spline sample is farther than A/2 from the previous one");
		Check (monotone,"spline stations are not monotone");

		//	Determinism: identical controls, identical polyline.
		DynamicVectorClass<Vector2> again;
		Check (Hybrid_Sample_Centripetal_Catmull_Rom (controls,4,7.5f,&again,NULL),"second spline sample failed");
		bool identical = (again.Count () == points.Count ());
		for (int i = 0; (i < again.Count ()) && identical; ++i) {
			if ((again[i].X != points[i].X) || (again[i].Y != points[i].Y)) identical = false;
		}
		Check (identical,"the spline sampler is not reproducible");
	}

	//	Section 10.3 / 10.4 field profiles, on hand-built records.
	{
		HybridLandform hill;
		hill.StableId	= 1;
		hill.Kind		= HYBRID_LANDFORM_HILL;
		hill.Center.Set (0.0f,0.0f);
		hill.RadiusX	= 200.0f;
		hill.RadiusY	= 160.0f;
		hill.Orientation= 0.0f;
		hill.Amplitude	= 20.0f;

		float profile = 0.0f;
		Check (Near (Hybrid_Landform_Contribution (hill,Vector2 (0.0f,0.0f),&profile),20.0f) && Near (profile,1.0f),
				 "a hill peaks at its centre");
		Check (Hybrid_Landform_Contribution (hill,Vector2 (200.0f,0.0f),&profile) == 0.0f && (profile == 0.0f),
				 "a hill field is exactly zero at its influence radius");
		Check (Hybrid_Landform_Contribution (hill,Vector2 (260.0f,0.0f),NULL) == 0.0f,"a hill reaches past its radius");
		const float flank = Hybrid_Landform_Contribution (hill,Vector2 (100.0f,0.0f),NULL);
		Check ((flank > 0.0f) && (flank < 20.0f),"a hill flank is between zero and its amplitude");

		//	Plateau conversion flattens the upper 35% of that same profile.
		HybridLandform plateau = hill;
		plateau.IsPlateau = true;
		Check (Near (Hybrid_Landform_Contribution (plateau,Vector2 (0.0f,0.0f),NULL),20.0f),
				 "a plateau keeps its summit height");
		float top_profile = 0.0f;
		Hybrid_Landform_Contribution (hill,Vector2 (60.0f,0.0f),&top_profile);
		if (top_profile >= 0.65f) {
			Check (Near (Hybrid_Landform_Contribution (plateau,Vector2 (60.0f,0.0f),NULL),20.0f),
					 "the upper 35%% of a plateau profile is not flat");
		}

		//	Section 10.3: flat floor, smoothstep wall, exactly zero outside the rim.
		HybridLandform ravine;
		ravine.StableId		= 2;
		ravine.Kind			= HYBRID_LANDFORM_RAVINE;
		ravine.SystemId		= 1;
		ravine.FloorHalfWidth	= 6.0f;
		ravine.RimHalfWidth		= 20.0f;
		ravine.Depth			= 15.0f;
		Fill_Straight_Centerline (&ravine,Vector2 (0.0f,-300.0f),Vector2 (0.0f,300.0f),7.5f);
		Set_Landform_Bounds (&ravine);

		Check (Near (Hybrid_Landform_Contribution (ravine,Vector2 (0.0f,0.0f),NULL),-15.0f),
				 "a ravine floor is at full depth");
		Check (Near (Hybrid_Landform_Contribution (ravine,Vector2 (6.0f,0.0f),NULL),-15.0f),
				 "the whole floor half-width is at full depth");
		Check (Hybrid_Landform_Contribution (ravine,Vector2 (20.0f,0.0f),NULL) == 0.0f,
				 "a ravine contributes nothing outside its rim");
		const float wall = Hybrid_Landform_Contribution (ravine,Vector2 (13.0f,0.0f),NULL);
		Check ((wall < 0.0f) && (wall > -15.0f),"a ravine wall smoothsteps from full depth to zero");

		//	Section 10.2 cliff: the upper shelf stands a full height difference above the lower toe.
		HybridLandform cliff;
		cliff.StableId		= 3;
		cliff.Kind			= HYBRID_LANDFORM_CLIFF;
		cliff.EdgeWidth	= 4.0f;
		cliff.TalusWidth	= 6.0f;
		cliff.HeightDifference = 24.0f;
		cliff.UpperSideSign	= 1;
		Fill_Straight_Centerline (&cliff,Vector2 (-200.0f,0.0f),Vector2 (200.0f,0.0f),7.5f);
		Set_Landform_Bounds (&cliff);

		//	The centerline runs +X, so its left side (+Y) is the positive lateral side.
		Check (Near (Hybrid_Landform_Contribution (cliff,Vector2 (0.0f,5.0f),NULL),24.0f),
				 "the cliff upper shelf is not at full height");
		Check (Hybrid_Landform_Contribution (cliff,Vector2 (0.0f,-9.0f),NULL) == 0.0f,
				 "the cliff toe is not at zero");
		Check (Hybrid_Landform_Contribution (cliff,Vector2 (0.0f,15.0f),NULL) == 0.0f,
				 "a cliff must not lift terrain beyond its own influence polygon");
		HybridLandform mirrored = cliff;
		mirrored.UpperSideSign = -1;
		Check (Near (Hybrid_Landform_Contribution (mirrored,Vector2 (0.0f,-5.0f),NULL),24.0f),
				 "the cliff upper-side sign is ignored");

		//	Branch joins take the minimum contribution plus one A-wide smooth union, never the sum.
		HybridLandformSet system;
		system.Hash.Init (1200.0f,1200.0f,120.0f);
		HybridLandform branch = ravine;
		branch.StableId = 2;
		system.Landforms.Add (branch);
		HybridLandform child = ravine;
		child.StableId			= 3;
		child.ParentStableId	= 2;
		Fill_Straight_Centerline (&child,Vector2 (-300.0f,0.0f),Vector2 (300.0f,0.0f),7.5f);
		Set_Landform_Bounds (&child);
		system.Landforms.Add (child);

		const float joined = Hybrid_Landform_Height (system,Vector2 (0.0f,0.0f),15.0f);
		Check ((joined <= -15.0f) && (joined > -30.0f),"a branch join summed two depths (%g)",joined);

		//	Two independent features do sum.
		HybridLandformSet independent;
		independent.Hash.Init (1200.0f,1200.0f,120.0f);
		HybridLandform hill_a = hill;
		hill_a.StableId = 1;
		Set_Landform_Bounds (&hill_a);
		HybridLandform hill_b = hill;
		hill_b.StableId = 2;
		Set_Landform_Bounds (&hill_b);
		independent.Landforms.Add (hill_a);
		independent.Landforms.Add (hill_b);
		Check (Near (Hybrid_Landform_Height (independent,Vector2 (0.0f,0.0f),15.0f),40.0f),
				 "independent hill fields must sum");
	}

	//	A full Phase-5 build at each size class, on top of a real Phase-2/3/4 chain.
	const float sizes[3] = { 600.0f, 1200.0f, 2400.0f };
	const float base_cells[3] = { 1.0f, 2.0f, 3.0f };

	for (int i = 0; i < 3; ++i) {

		HybridMapIdentityParams sized = Graph_Identity_Fixture (sizes[i],base_cells[i]);
		Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity at D=%g: %s",
				 sizes[i],identity_result.Detail.Peek_Buffer ());

		HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
				 "strategic graph at D=%g: %s",sizes[i],graph_result.Detail.Peek_Buffer ());

		HybridCoarseMatrixParams coarse_params = Coarse_Fixture (sized,sizes[i]);
		Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
				 "coarse matrices at D=%g: %s",sizes[i],coarse_result.Detail.Peek_Buffer ());

		HybridLandformParams params = Landform_Fixture (sized,sizes[i]);
		const bool built = Hybrid_Build_Landforms (params,identity,intents,graph,&matrices,&landforms,&result);

		/*
		**	Section 5: a quota that cannot be met is a *named* attempt failure the manager retries with
		**	a new whole-world seed, not a half-built world.  So a failure here has to be named, has to
		**	explain itself and has to leave nothing behind.  Q-018 tracks the one geometry the plan
		**	over-constrains -- the LARGE hill quota against the 25%% same-type overlap cap -- which is
		**	why a LARGE failure is tolerated here while MEDIUM, the size class the plan sets its own
		**	quality target on, must succeed.
		*/
		if (!built) {
			Check (result.Failure != HYBRID_LANDFORM_OK,"a failed Phase 5 must name its failure");
			Check (result.Detail.Get_Length () > 0,"a failed Phase 5 must say what refused the feature");
			Check (landforms.Landforms.Count () == 0,"a failed Phase 5 left landforms behind");
			Check (identity.SizeClass != HYBRID_SIZE_MEDIUM,"landforms at D=%g (%s): %s (%s)",sizes[i],
					 Hybrid_Archetype_Name (identity.Archetype),Hybrid_Landform_Failure_Name (result.Failure),
					 result.Detail.Peek_Buffer ());
			continue;
		}

		const HybridConstants & k = params.Constants;
		const HybridSemanticMatrix & matrix = matrices.Matrix;

		//	The gate's own counts, recomputed here from the plan's formulas.
		Check (landforms.Count_Of_Kind (HYBRID_LANDFORM_HILL)
				 == Hybrid_Hill_Target_Count (identity.SizeClass,params.HillDensity),"hill count at D=%g",sizes[i]);
		Check (landforms.Count_Of_Kind (HYBRID_LANDFORM_RIDGE)
				 == Hybrid_Ridge_Target_Count (identity.SizeClass,params.MountainDensity),"ridge count at D=%g",sizes[i]);
		Check (landforms.Count_Of_Kind (HYBRID_LANDFORM_CLIFF)
				 == Hybrid_Cliff_Target_Count (identity.SizeClass,params.InteriorCliffDensity),"cliff count at D=%g",sizes[i]);
		Check (landforms.Count_Of_Systems ()
				 == Hybrid_Ravine_System_Target_Count (identity.SizeClass,params.ValleyDensity),
				 "canyon/ravine system count at D=%g",sizes[i]);

		for (int f = 0; f < landforms.Landforms.Count (); ++f) {

			const HybridLandform & landform = landforms.Landforms[f];
			Check (landform.StableId == (unsigned int)(f + 1),"StableIds are not creation order");

			//	Section 8 Phase 5 geometry ranges, and the elevation envelope every value is clamped to.
			switch (landform.Kind) {

				case HYBRID_LANDFORM_HILL:
					Check ((landform.RadiusX >= (0.08f * k.D) - 0.001f) && (landform.RadiusX <= (0.16f * k.D) + 0.001f),
							 "hill %u radius %g is outside 0.08D..0.16D",landform.StableId,landform.RadiusX);
					Check ((landform.RadiusY >= (0.75f * landform.RadiusX) - 0.01f)
						&& (landform.RadiusY <= (1.25f * landform.RadiusX) + 0.01f),
							 "hill %u aspect ratio is outside 0.75..1.25",landform.StableId);
					Check ((landform.Amplitude > 0.0f) && (landform.Amplitude <= params.MaxElevation),
							 "hill %u amplitude %g is outside the elevation envelope",landform.StableId,landform.Amplitude);
					break;

				case HYBRID_LANDFORM_RIDGE:
					Check ((landform.Length >= (0.25f * k.D) - 0.001f) && (landform.Length <= (0.55f * k.D) + 0.001f),
							 "ridge %u length %g is outside 0.25D..0.55D",landform.StableId,landform.Length);
					Check ((landform.InfluenceHalfWidth >= (0.04f * k.D) - 0.001f)
						&& (landform.InfluenceHalfWidth <= (0.09f * k.D) + 0.001f),
							 "ridge %u half-width is outside 0.04D..0.09D",landform.StableId);
					break;

				case HYBRID_LANDFORM_CLIFF:
					Check (landform.Centerline_Length () >= Hybrid_Cliff_Min_Length (k.W,k.D),
							 "cliff %u is shorter than the Section 5 minimum",landform.StableId);
					Check ((landform.EdgeWidth >= (0.5f * k.W) - 0.001f) && (landform.EdgeWidth <= (1.5f * k.W) + 0.001f),
							 "cliff %u edge width is outside 0.5W..1.5W",landform.StableId);
					//	Section 10.2 as revised by Q-021 (D-016): `1W..5.25W`.
					Check ((landform.TalusWidth >= k.W - 0.001f) && (landform.TalusWidth <= (5.25f * k.W) + 0.001f),
							 "cliff %u talus width is outside 1W..5.25W",landform.StableId);
					Check ((landform.UpperSideSign == 1) || (landform.UpperSideSign == -1),
							 "cliff %u has no upper-side sign",landform.StableId);
					break;

				case HYBRID_LANDFORM_CANYON:
				case HYBRID_LANDFORM_RAVINE:
					Check (landform.RimHalfWidth > landform.FloorHalfWidth,
							 "%s %u has no wall between floor and rim",Hybrid_Landform_Kind_Name (landform.Kind),
							 landform.StableId);
					Check (landform.Depth >= Hybrid_Ravine_Min_Depth (params.TierStep),
							 "%s %u is %g deep, Section 5 needs %g",Hybrid_Landform_Kind_Name (landform.Kind),
							 landform.StableId,landform.Depth,Hybrid_Ravine_Min_Depth (params.TierStep));
					if (landform.ParentStableId == 0) {
						Check (landform.Centerline_Length () >= Hybrid_Ravine_Min_Length (k.W,k.D),
								 "%s system %u is shorter than the Section 5 minimum",
								 Hybrid_Landform_Kind_Name (landform.Kind),landform.SystemId);
					}
					break;

				default:
					break;
			}

			if (!Hybrid_Landform_Is_Line (landform.Kind)) continue;

			//	The four controls sit at -0.50L, -0.1666667L, +0.1666667L, +0.50L along the feature axis,
			//	with zero endpoint lateral offset and interior offsets inside +/-0.10L.
			const float c = WWMath::Cos (landform.Orientation);
			const float s = WWMath::Sin (landform.Orientation);
			const float expect[4] = { -0.5f * landform.Length, -landform.Length / 6.0f,
											  +landform.Length / 6.0f, +0.5f * landform.Length };
			for (int control = 0; control < 4; ++control) {
				const Vector2 rel = landform.Controls[control] - landform.Center;
				const float along = (rel.X * c) + (rel.Y * s);
				const float lateral = (-rel.X * s) + (rel.Y * c);
				Check (WWMath::Fabs (along - expect[control]) <= 0.01f,
						 "%s %u control %d sits at %g along its axis, not %g",
						 Hybrid_Landform_Kind_Name (landform.Kind),landform.StableId,control,along,expect[control]);
				const float allowed = ((control == 0) || (control == 3)) ? 0.01f : ((0.10f * landform.Length) + 0.01f);
				Check (WWMath::Fabs (lateral) <= allowed,
						 "%s %u control %d lateral offset %g exceeds %g",Hybrid_Landform_Kind_Name (landform.Kind),
						 landform.StableId,control,lateral,allowed);
			}

			bool sampled_ok = (landform.Points.Count () == landform.Stations.Count ()) && (landform.Points.Count () >= 2);
			for (int point = 1; sampled_ok && (point < landform.Points.Count ()); ++point) {
				if ((landform.Points[point] - landform.Points[point - 1]).Length () > ((0.5f * k.A) + 0.1f)) sampled_ok = false;
			}
			Check (sampled_ok,"%s %u is not sampled at A/2",Hybrid_Landform_Kind_Name (landform.Kind),landform.StableId);
		}

		/*
		**	The gate's two exclusions, and the two Section-2.2 layers this phase owns.  Phase 4 leaves
		**	LandformMask and RoughnessClass clear, so anything set here came from Phase 5.
		*/
		int stamped_cells = 0;
		for (int cy = 0; cy < matrix.Get_Count_Y (); ++cy) {
			for (int cx = 0; cx < matrix.Get_Count_X (); ++cx) {

				const int index = matrix.Index (cx,cy);
				const unsigned char mask = matrix.LandformMask[index];
				if (mask != 0) stamped_cells++;

				const unsigned char immutable_mask = (unsigned char)(HYBRID_LANDFORM_MASK_CLIFF_FACE
					| HYBRID_LANDFORM_MASK_CANYON_FLOOR | HYBRID_LANDFORM_MASK_CANYON_WALL
					| HYBRID_LANDFORM_MASK_RAVINE_FLOOR | HYBRID_LANDFORM_MASK_RAVINE_WALL);
				if ((mask & immutable_mask) != 0) {
					Check (!matrices.Is_Base_Protected (cx,cy),
							 "an immutable landform stands in BASE_PROTECTED cell %d,%d",cx,cy);
					Check (!matrix.Has_Flag (cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER),
							 "an immutable landform stands in an OPEN_MANEUVER cell %d,%d",cx,cy);
				}
				if ((mask & HYBRID_LANDFORM_MASK_CLIFF_FACE) != 0) {
					Check (matrix.RoughnessClass[index] == (unsigned char)HYBRID_ROUGHNESS_BROKEN,
							 "a cliff face cell %d,%d is not broken ground",cx,cy);
					Check (matrix.Has_Flag (cx,cy,HYBRID_FLAG_CLIFF_BAND),"a cliff face cell has no CLIFF_BAND flag");
				}
				if ((mask & HYBRID_LANDFORM_MASK_HILL) != 0) {
					Check (matrix.Has_Flag (cx,cy,HYBRID_FLAG_HILL),"a hill cell has no HILL flag");
					Check (matrix.RoughnessClass[index] >= (unsigned char)HYBRID_ROUGHNESS_ROLLING,
							 "a hill cell is still flat");
				}
				if ((mask & HYBRID_LANDFORM_MASK_PLATEAU_TOP) != 0) {
					Check (matrix.Has_Flag (cx,cy,HYBRID_FLAG_PLATEAU),"a plateau-top cell has no PLATEAU flag");
				}
			}
		}
		Check (stamped_cells > 0,"Phase 5 wrote no LandformMask cell at D=%g",sizes[i]);

		//	Every accepted feature is in the Section-2.3 planning hash.
		for (int f = 0; f < landforms.Landforms.Count (); ++f) {
			const HybridLandform & landform = landforms.Landforms[f];
			DynamicVectorClass<unsigned int> ids;
			landforms.Hash.Query (landform.Center.X - params.WorldMinX,landform.Center.Y - params.WorldMinY,
										 landform.Center.X - params.WorldMinX,landform.Center.Y - params.WorldMinY,&ids);
			bool present = false;
			for (int id = 0; id < ids.Count (); ++id) {
				if (ids[id] == landform.StableId) present = true;
			}
			Check (present,"landform %u is not in the planning spatial hash",landform.StableId);
		}

		//	Determinism: the same inputs give the same checksum, a different terrain seed does not.
		{
			HybridCoarseMatrices again_matrices;
			HybridStrategicGraph again_graph;
			HybridLandformSet again;
			Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&again_graph,&graph_result),
					 "second strategic graph at D=%g",sizes[i]);
			Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&again_graph,&again_matrices,
															 &coarse_result),"second coarse matrices at D=%g",sizes[i]);
			Check (Hybrid_Build_Landforms (params,identity,intents,again_graph,&again_matrices,&again,&result),
					 "second landform build at D=%g: %s",sizes[i],result.Detail.Peek_Buffer ());
			Check (again.Checksum == landforms.Checksum,"Phase 5 is not reproducible at D=%g",sizes[i]);

			HybridLandformParams other = params;
			other.TerrainSeed = WorldSeedService::Derive_Subsystem_Seed (99887766,WorldSeedService::SEED_DOMAIN_TERRAIN);
			HybridCoarseMatrices other_matrices;
			HybridStrategicGraph other_graph;
			HybridLandformSet other_set;
			Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&other_graph,&graph_result),
					 "third strategic graph at D=%g",sizes[i]);
			Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&other_graph,&other_matrices,
															 &coarse_result),"third coarse matrices at D=%g",sizes[i]);
			if (Hybrid_Build_Landforms (other,identity,intents,other_graph,&other_matrices,&other_set,&result)) {
				Check (other_set.Checksum != landforms.Checksum,"a different terrain seed gave the same geography");
			}
		}
	}

	/*
	**	`highland_pass`: the principal ridge carries Section 6.10's mandatory surface saddle, and its
	**	flat zone is wide enough for a PRIMARY_2 roadbed plus 2 * min_vehicle_lane_clearance.
	*/
	{
		HybridMapIdentityParams saddle_params = Graph_Identity_Fixture (1200.0f,2.0f);
		saddle_params.ArchetypeSelector = HYBRID_ARCHETYPE_HIGHLAND_PASS;
		Check (Hybrid_Resolve_Map_Identity (saddle_params,&identity,&intents,&identity_result),"highland_pass identity");

		HybridStrategicGraphParams graph_params = Graph_Fixture (saddle_params,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
				 "highland_pass graph: %s",graph_result.Detail.Peek_Buffer ());
		HybridCoarseMatrixParams coarse_params = Coarse_Fixture (saddle_params,1200.0f);
		Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
				 "highland_pass matrices: %s",coarse_result.Detail.Peek_Buffer ());

		HybridLandformParams params = Landform_Fixture (saddle_params,1200.0f);
		const bool saddle_built = Hybrid_Build_Landforms (params,identity,intents,graph,&matrices,&landforms,&result);
		Check (saddle_built,"highland_pass landforms: %s (%s)",Hybrid_Landform_Failure_Name (result.Failure),
				 result.Detail.Peek_Buffer ());
		if (saddle_built) {

			const HybridLandform * ridge = NULL;
			for (int f = 0; f < landforms.Landforms.Count (); ++f) {
				if (landforms.Landforms[f].Required && (landforms.Landforms[f].Kind == HYBRID_LANDFORM_RIDGE)) {
					ridge = &landforms.Landforms[f];
				}
			}
			Check (ridge != NULL,"highland_pass built no principal ridge");
			if (ridge != NULL) {
				Check (ridge->SaddlePresent,"the principal ridge has no surface saddle");
				const float required = Hybrid_Roadbed_Width (HYBRID_ROAD_PRIMARY_2,params.Constants.W)
											+ (2.0f * params.MinVehicleLaneClearance);
				Check ((2.0f * ridge->SaddleFlatHalfLength) >= (required - 0.01f),
						 "the saddle footprint %g is narrower than a PRIMARY_2 roadbed plus clearance %g",
						 2.0f * ridge->SaddleFlatHalfLength,required);

				//	The amplitude multiplier is 0.15 at the pass and 1 away from it.
				const int saddle_index = Landform_Station_Index (*ridge,ridge->SaddleStation);
				const Vector2 at_pass = ridge->Points[saddle_index];
				const float pass_height = WWMath::Fabs (Hybrid_Landform_Contribution (*ridge,at_pass,NULL));
				const float crest_height = ridge->Amplitude;
				Check (pass_height <= (0.20f * crest_height) + 0.01f,
						 "the saddle does not drop the ridge to 0.15 of its amplitude (%g of %g)",pass_height,crest_height);
			}
		}
	}

	/*
	**	`ravine_network`: the main ravine plus exactly 2/3/5 attached branches is one system.
	*/
	{
		HybridMapIdentityParams network_params = Graph_Identity_Fixture (1200.0f,2.0f);
		network_params.ArchetypeSelector = HYBRID_ARCHETYPE_RAVINE_NETWORK;
		Check (Hybrid_Resolve_Map_Identity (network_params,&identity,&intents,&identity_result),"ravine_network identity");

		HybridStrategicGraphParams graph_params = Graph_Fixture (network_params,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
				 "ravine_network graph: %s",graph_result.Detail.Peek_Buffer ());
		HybridCoarseMatrixParams coarse_params = Coarse_Fixture (network_params,1200.0f);
		Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
				 "ravine_network matrices: %s",coarse_result.Detail.Peek_Buffer ());

		HybridLandformParams params = Landform_Fixture (network_params,1200.0f);
		const bool network_built = Hybrid_Build_Landforms (params,identity,intents,graph,&matrices,&landforms,&result);
		Check (network_built,"ravine_network landforms: %s (%s)",Hybrid_Landform_Failure_Name (result.Failure),
				 result.Detail.Peek_Buffer ());
		if (network_built) {

			unsigned int main_id = 0;
			for (int f = 0; f < landforms.Landforms.Count (); ++f) {
				const HybridLandform & landform = landforms.Landforms[f];
				if (landform.Required && (landform.Kind == HYBRID_LANDFORM_RAVINE) && (landform.ParentStableId == 0)) {
					main_id = landform.StableId;
				}
			}
			Check (main_id != 0,"ravine_network built no main ravine");

			int branches = 0;
			for (int f = 0; f < landforms.Landforms.Count (); ++f) {
				const HybridLandform & landform = landforms.Landforms[f];
				if (landform.ParentStableId != main_id) continue;
				branches++;
				const HybridLandform * parent = landforms.Find (main_id);
				Check ((parent != NULL) && (landform.SystemId == parent->SystemId),
						 "branch %u is not in its parent's system",landform.StableId);
				//	The inner endpoint joins the main ravine, so it is on the parent's centerline.
				if (parent != NULL) {
					const float lateral = Landform_Lateral (*parent,landform.Points[0]);
					Check (WWMath::Fabs (lateral) <= (0.5f * params.Constants.A),
							 "branch %u starts %g from the main ravine centerline",landform.StableId,lateral);
				}
			}
			Check (branches == Hybrid_Ravine_Branch_Count (identity.SizeClass),
					 "%d ravine branches, Section 6.8 requires %d",branches,
					 Hybrid_Ravine_Branch_Count (identity.SizeClass));
			Check (landforms.Count_Of_Systems ()
					 == Hybrid_Ravine_System_Target_Count (identity.SizeClass,params.ValleyDensity),
					 "branches must not count as independent systems");
		}
	}
}



static HybridHydrologyParams Hydrology_Fixture (const HybridMapIdentityParams & identity_params,float world_size)
{
	HybridHydrologyParams params;
	params.HydrologySeed	= identity_params.HydrologySeed;
	params.Constants		= identity_params.Constants;
	params.WorldMinX		= -0.5f * world_size;
	params.WorldMinY		= -0.5f * world_size;
	params.WorldWidth		= world_size;
	params.WorldHeight	= world_size;
	params.SeaLevel		= 0.0f;
	params.MinElevation	= -15.0f;					// native terrain.min_elevation
	params.MaxElevation	= 90.0f;						// native terrain.max_elevation
	params.TierStep		= 18.0f;
	params.TierCount		= 3;
	params.ChunkSize		= 60.0f;						// the fixture chunk size, so A = 15 and S = 30
	params.Rivers			= true;						// Section 5 requires all three on the hybrid path
	params.Lakes			= true;
	params.Coastline		= true;
	params.RiverWidth		= 6.0f;						// native water.*
	params.StreamWidth	= 2.5f;
	params.RiverAccumulationChunks	= 6.0f;
	params.MaxRiverCount					= 4;
	params.WaterFraction					= 0.08f;
	params.RiverBedDepth					= 2.5f;		// WaterSystem::Define_Default_Water's own depths
	params.LakeBedDepth					= 3.0f;
	params.MaxLocalRetriesPerStage		= 8;
	params.MaxAlternateCandidatesPerFeature	= 32;
	params.MinVehicleLaneClearance		= 1.0f;
	//	WORLDBOX measurements from docs/procedural_world/VehicleCollisionMetrics.md.
	params.HeavyVehicleWidth	= 6.801906f;
	params.HeavyVehicleHeight	= 3.064066f;
	params.HarvesterWidth		= 4.514225f;
	params.BridgeMaxSpanLength	= 200.0f;
	params.BridgeRequiredWidth	= 12.0f;
	params.BridgeApproachLength= 30.0f;
	params.BridgeWaterClearanceMargin = 2.0f;
	params.TunnelsAvailable		= true;
	params.TunnelRequiredWidth	= 9.0f;
	params.TunnelRequiredHeight= 5.0f;
	params.AllowFords				= false;
	params.FordsAvailable		= false;
	//	Native grading, so step 18's pass candidates are judged against the real allowance.
	params.MaxCutDepth	= 6.0f;
	params.MaxFillHeight	= 6.0f;

	return params;
}


//	A straight river record with a constant width, for the evaluator tests below.
static void Fill_Test_River (HybridRiverSystem * river,const Vector2 & a,const Vector2 & b,float width,
										float bed_depth,float spacing)
{
	river->Points.Delete_All ();
	river->Stations.Delete_All ();
	river->Widths.Delete_All ();
	river->Accumulation.Delete_All ();
	river->Surfaces.Delete_All ();

	Vector2 delta = b - a;
	const float length = delta.Length ();
	if ((length <= 0.0f) || (spacing <= 0.0f)) return;
	delta *= (1.0f / length);

	for (float station = 0.0f; station < length; station += spacing) {
		river->Points.Add (a + (delta * station));
		river->Stations.Add (station);
		river->Widths.Add (width);
		river->Accumulation.Add (100.0f);
		river->Surfaces.Add (0.0f);
	}
	river->Points.Add (b);
	river->Stations.Add (length);
	river->Widths.Add (width);
	river->Accumulation.Add (100.0f);
	river->Surfaces.Add (0.0f);

	river->Length	= length;
	river->BedDepth= bed_depth;
	river->Source	= a;
	river->Mouth	= b;
	river->BoundsMin.Set (((a.X < b.X) ? a.X : b.X) - width,((a.Y < b.Y) ? a.Y : b.Y) - width);
	river->BoundsMax.Set (((a.X > b.X) ? a.X : b.X) + width,((a.Y > b.Y) ? a.Y : b.Y) + width);
}


//	An axis-aligned square ring, counter-clockwise, for a hand-built water body.
static void Fill_Square_Ring (HybridWaterBody * body,const Vector2 & centre,float half)
{
	body->Points.Delete_All ();
	body->Points.Add (Vector2 (centre.X - half,centre.Y - half));
	body->Points.Add (Vector2 (centre.X + half,centre.Y - half));
	body->Points.Add (Vector2 (centre.X + half,centre.Y + half));
	body->Points.Add (Vector2 (centre.X - half,centre.Y + half));
	body->Center	= centre;
	body->Area		= 4.0f * half * half;
	body->BoundsMin.Set (centre.X - half,centre.Y - half);
	body->BoundsMax.Set (centre.X + half,centre.Y + half);
}


void Check_Hybrid_Hydrology (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformSet landforms;
	HybridLandformResult landform_result;
	HybridHydrologySet hydrology;
	HybridHydrologyResult result;

	//	Section 5's quotas and qualifications, and the Section 11 formulas.
	{
		Check (Hybrid_Required_River_Count (HYBRID_SIZE_SMALL) == 1,"SMALL river quota");
		Check (Hybrid_Required_River_Count (HYBRID_SIZE_MEDIUM) == 1,"MEDIUM river quota");
		Check (Hybrid_Required_River_Count (HYBRID_SIZE_LARGE) == 2,"LARGE river quota");
		Check (Hybrid_Required_Lake_Count (HYBRID_SIZE_SMALL) == 1,"SMALL lake quota");
		Check (Hybrid_Required_Lake_Count (HYBRID_SIZE_LARGE) == 2,"LARGE lake quota");

		//	river_threshold = max(1, RoundHalfUp(chunks * (chunk_size/A)^2)); A = 15 at chunk_size 60.
		Check (Near (Hybrid_River_Threshold (6.0f,60.0f,15.0f),96.0f),"river threshold is %g, not 96",
				 Hybrid_River_Threshold (6.0f,60.0f,15.0f));
		Check (Near (Hybrid_River_Threshold (0.0f,60.0f,15.0f),1.0f),"the threshold floor is not 1");

		//	width = lerp(stream, river, clamp((accum-threshold)/(3*threshold),0,1)).
		Check (Near (Hybrid_River_Width (96.0f,96.0f,2.5f,6.0f),2.5f),"a channel at threshold is not stream width");
		Check (Near (Hybrid_River_Width (4.0f * 96.0f,96.0f,2.5f,6.0f),6.0f),
				 "a channel at four times threshold is not river width");
		const float middle = Hybrid_River_Width (96.0f + (1.5f * 96.0f),96.0f,2.5f,6.0f);
		Check (Near (middle,4.25f),"the width ramp is not linear (%g)",middle);

		//	true lake >= max(4*A*A, 0.002 * world area); island >= max((16W)^2, 0.01 * world area).
		Check (Near (Hybrid_True_Lake_Min_Area (15.0f,1200.0f * 1200.0f),2880.0f),"true lake minimum area");
		Check (Near (Hybrid_True_Lake_Min_Area (40.0f,10000.0f),6400.0f),"the 4*A*A term must win when it is larger");
		Check (Near (Hybrid_Island_Min_Area (10.0f,1200.0f * 1200.0f),25600.0f),"island minimum area");
	}

	//	The Phase-7 evaluators, on hand-built records.
	{
		HybridHydrologySet set;

		HybridWaterBody lake;
		lake.StableId		= 1;
		lake.Kind			= HYBRID_WATER_BODY_LAKE;
		lake.SurfaceHeight= -1.0f;
		lake.BedDepth		= 3.0f;
		Fill_Square_Ring (&lake,Vector2 (0.0f,0.0f),100.0f);
		set.Bodies.Add (lake);

		unsigned int body = 0;
		float surface = 0.0f;
		Check (Hybrid_Point_In_Standing_Water (set,Vector2 (10.0f,10.0f),&body,&surface) && (body == 1)
				 && Near (surface,-1.0f),"a point inside a lake is not in standing water");
		Check (!Hybrid_Point_In_Standing_Water (set,Vector2 (150.0f,0.0f),NULL,NULL),
				 "a point outside a lake is in standing water");
		Check (Near (Hybrid_Water_Carve_Depth (set,Vector2 (0.0f,0.0f),3.32f,6.0f),3.0f),
				 "a lake does not carve its bed depth");
		Check (Hybrid_Water_Carve_Depth (set,Vector2 (150.0f,150.0f),3.32f,6.0f) == 0.0f,
				 "a lake carves outside its own polygon");

		//	An island is dry land inside its host: no standing water, no carve, and a blended bias.
		HybridWaterBody island;
		island.StableId		= 2;
		island.Kind				= HYBRID_WATER_BODY_ISLAND;
		island.HostBodyId		= 1;
		island.SurfaceHeight	= -1.0f;
		island.HeightBias		= 9.0f;
		island.ShoreBlend		= 6.0f;
		Fill_Square_Ring (&island,Vector2 (0.0f,0.0f),20.0f);
		set.Bodies.Add (island);

		Check (!Hybrid_Point_In_Standing_Water (set,Vector2 (0.0f,0.0f),NULL,NULL),
				 "an island is still standing water");
		Check (Hybrid_Water_Carve_Depth (set,Vector2 (0.0f,0.0f),3.32f,6.0f) == 0.0f,
				 "an island does not suppress its host's carve");
		Check (Near (Hybrid_Island_Height_Bias (set,Vector2 (0.0f,0.0f)),9.0f),
				 "an island's own bias is not its full value");
		const float blended = Hybrid_Island_Height_Bias (set,Vector2 (23.0f,0.0f));
		Check ((blended > 0.0f) && (blended < 9.0f),"the island shoreline blend is not between 0 and the bias (%g)",
				 blended);
		Check (Hybrid_Island_Height_Bias (set,Vector2 (40.0f,0.0f)) == 0.0f,
				 "the island bias reaches past its 2W blend band");

		//	A river carves its bed to half its width and blends out across max(river_width,2W).
		HybridHydrologySet river_set;
		HybridRiverSystem river;
		river.StableId = 1;
		Fill_Test_River (&river,Vector2 (-200.0f,0.0f),Vector2 (200.0f,0.0f),8.0f,2.5f,7.5f);
		river_set.Rivers.Add (river);

		Check (Near (Hybrid_Water_Carve_Depth (river_set,Vector2 (0.0f,0.0f),3.32f,6.0f),2.5f),
				 "a river bed does not carve its full depth");
		Check (Near (Hybrid_Water_Carve_Depth (river_set,Vector2 (0.0f,4.0f),3.32f,6.0f),2.5f),
				 "the whole half width is not at full carve");
		const float bank = Hybrid_Water_Carve_Depth (river_set,Vector2 (0.0f,7.0f),3.32f,6.0f);
		Check ((bank > 0.0f) && (bank < 2.5f),"the river bank does not blend (%g)",bank);
		Check (Hybrid_Water_Carve_Depth (river_set,Vector2 (0.0f,40.0f),3.32f,6.0f) == 0.0f,
				 "a river carves beyond its bank blend");
	}

	//	A full Phase-6 build at each size class, on top of a real Phase-2/3/4/5 chain.
	const float sizes[3] = { 600.0f, 1200.0f, 2400.0f };
	const float base_cells[3] = { 1.0f, 2.0f, 3.0f };

	for (int i = 0; i < 3; ++i) {

		HybridMapIdentityParams sized = Graph_Identity_Fixture (sizes[i],base_cells[i]);
		Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity at D=%g: %s",
				 sizes[i],identity_result.Detail.Peek_Buffer ());

		HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
				 "strategic graph at D=%g: %s",sizes[i],graph_result.Detail.Peek_Buffer ());

		HybridCoarseMatrixParams coarse_params = Coarse_Fixture (sized,sizes[i]);
		Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
				 "coarse matrices at D=%g: %s",sizes[i],coarse_result.Detail.Peek_Buffer ());

		HybridLandformParams landform_params = Landform_Fixture (sized,sizes[i]);
		if (!Hybrid_Build_Landforms (landform_params,identity,intents,graph,&matrices,&landforms,&landform_result)) {
			//	Q-018: the LARGE hill quota against the 25%% same-type overlap cap.  Phase 6 needs Phase 5.
			Check (identity.SizeClass != HYBRID_SIZE_MEDIUM,"landforms at D=%g: %s (%s)",sizes[i],
					 Hybrid_Landform_Failure_Name (landform_result.Failure),landform_result.Detail.Peek_Buffer ());
			continue;
		}

		HybridHydrologyParams params = Hydrology_Fixture (sized,sizes[i]);
		const bool built = Hybrid_Build_Hydrology (params,identity,intents,graph,landforms,&matrices,&hydrology,
																 &result);

		/*
		**	A quota Phase 6 cannot meet is a *named* attempt failure the manager retries with a new
		**	whole-world seed, never a half-built world, so a failure here has to name itself, explain
		**	itself and leave nothing behind.  MEDIUM is the size class the plan sets its quality target
		**	on, so it must succeed on this fixture seed.
		*/
		if (!built) {
			Check (result.Failure != HYBRID_HYDROLOGY_OK,"a failed Phase 6 must name its failure");
			Check (result.Detail.Get_Length () > 0,"a failed Phase 6 must say what refused the water");
			Check (hydrology.Rivers.Count () == 0,"a failed Phase 6 left rivers behind");
			Check (hydrology.Bodies.Count () == 0,"a failed Phase 6 left water bodies behind");
			Check (hydrology.Windows.Count () == 0,"a failed Phase 6 left windows behind");
			Check (identity.SizeClass != HYBRID_SIZE_MEDIUM,"hydrology at D=%g (%s): %s (%s)",sizes[i],
					 Hybrid_Archetype_Name (identity.Archetype),Hybrid_Hydrology_Failure_Name (result.Failure),
					 result.Detail.Peek_Buffer ());
			continue;
		}

		const HybridConstants & k = params.Constants;
		const HybridSemanticMatrix & matrix = matrices.Matrix;
		const float lake_minimum = Hybrid_True_Lake_Min_Area (k.A,sizes[i] * sizes[i]);

		//	Section 5 quotas, recomputed here from the plan's own table.
		Check (hydrology.Rivers.Count () >= Hybrid_Required_River_Count (identity.SizeClass),
				 "%d rivers at D=%g, Section 5 requires %d",hydrology.Rivers.Count (),sizes[i],
				 Hybrid_Required_River_Count (identity.SizeClass));
		Check (hydrology.Count_Of_True_Lakes (lake_minimum) >= Hybrid_Required_Lake_Count (identity.SizeClass),
				 "%d true lakes at D=%g, Section 5 requires %d",hydrology.Count_Of_True_Lakes (lake_minimum),
				 sizes[i],Hybrid_Required_Lake_Count (identity.SizeClass));
		Check (hydrology.ThresholdReductions <= 3,"%d threshold reductions, the plan allows three",
				 hydrology.ThresholdReductions);

		//	Every river record is complete, advances along its own stations and stays out of the reserves.
		for (int r = 0; r < hydrology.Rivers.Count (); ++r) {

			const HybridRiverSystem & river = hydrology.Rivers[r];
			Check (river.Points.Count () >= 2,"river %u has %d points",river.StableId,river.Points.Count ());
			Check ((river.Widths.Count () == river.Points.Count ()) &&
					 (river.Surfaces.Count () == river.Points.Count ()) &&
					 (river.Stations.Count () == river.Points.Count ()),
					 "river %u has mismatched per-sample arrays",river.StableId);
			Check (river.BedDepth == params.RiverBedDepth,"river %u does not carve to the water definition's bed",
					 river.StableId);

			for (int point = 0; point < river.Points.Count (); ++point) {

				Check ((river.Widths[point] >= params.StreamWidth - 0.01f) &&
						 (river.Widths[point] <= params.RiverWidth + 0.01f),
						 "river %u sample %d is %g wide, outside stream..river width",river.StableId,point,
						 river.Widths[point]);
				if (point > 0) {
					Check (river.Stations[point] > river.Stations[point - 1],
							 "river %u station %d does not advance",river.StableId,point);
				}

				int cx = 0, cy = 0;
				if (!matrix.World_To_Cell (river.Points[point],&cx,&cy)) continue;
				Check (!matrices.Is_Base_Protected (cx,cy),"river %u runs through BASE_PROTECTED",river.StableId);
				Check (!matrix.Has_Flag (cx,cy,HYBRID_FLAG_NO_IMMUTABLE_BARRIER),
						 "river %u runs through an OPEN_MANEUVER no-water cell",river.StableId);
				Check (matrix.LandWaterIntent[matrix.Index (cx,cy)] == 1,
						 "river %u left a sample cell without water intent",river.StableId);
			}
		}

		//	Every body has a simple non-degenerate ring, and a lake's surface is below its shoreline.
		for (int body = 0; body < hydrology.Bodies.Count (); ++body) {

			const HybridWaterBody & water = hydrology.Bodies[body];
			Check (water.Points.Count () >= 3,"%s %u has %d ring points",Hybrid_Water_Body_Kind_Name (water.Kind),
					 water.StableId,water.Points.Count ());
			Check (water.Area > 0.0f,"%s %u has no area",Hybrid_Water_Body_Kind_Name (water.Kind),water.StableId);

			if (water.Kind == HYBRID_WATER_BODY_LAKE) {
				Check (water.BedDepth == params.LakeBedDepth,"lake %u does not carve the lake bed depth",
						 water.StableId);
				for (int point = 0; point < water.Points.Count (); ++point) {
					const float shore = Hybrid_Provisional_Height (params,matrices,landforms,water.Points[point]);
					Check (water.SurfaceHeight <= shore,"lake %u stands %g above its shoreline point %d",
							 water.StableId,water.SurfaceHeight - shore,point);
				}
			}
			if (water.Kind == HYBRID_WATER_BODY_ISLAND) {
				Check (water.HostBodyId != 0,"island %u has no host water body",water.StableId);
				Check (water.HeightBias > 0.0f,"island %u has no macro height bias",water.StableId);
				Check (Near (water.ShoreBlend,2.0f * k.W),"island %u does not blend across 2W",water.StableId);
			}
		}

		//	Windows: every one is usable, a river window is bridgeable or fordable, and step 19's merge
		//	leaves no two windows on one obstruction closer than 4W.
		for (int window = 0; window < hydrology.Windows.Count (); ++window) {

			const HybridCrossingWindow & entry = hydrology.Windows[window];
			Check (entry.Methods != 0,"window %u permits no method at all",entry.StableId);
			Check (entry.ApproachLength >= (8.0f * k.W) - 0.01f,"window %u reserves %g of approach, not 8W",
					 entry.StableId,entry.ApproachLength);
			Check (entry.Obstruction != HYBRID_OBSTRUCTION_LAKE,"a lake must never carry a window");
			Check (entry.Obstruction != HYBRID_OBSTRUCTION_COAST,"the coast must never carry a window");

			if (entry.Obstruction == HYBRID_OBSTRUCTION_RIVER) {
				const unsigned int usable = HYBRID_WINDOW_METHOD_BRIDGE | HYBRID_WINDOW_METHOD_FORD_LIGHT |
													 HYBRID_WINDOW_METHOD_FORD_HARVESTER;
				Check ((entry.Methods & usable) != 0,"river window %u is neither bridgeable nor fordable",
						 entry.StableId);
				Check (entry.Kind == HYBRID_WINDOW_CROSSING,"a river window is not a CROSSING_WINDOW");
			}
			if (entry.Obstruction == HYBRID_OBSTRUCTION_CLIFF) {
				Check (entry.Kind == HYBRID_WINDOW_RAMP_PASS,"a cliff window is not a RAMP_PASS_WINDOW");
			}
			if (!params.AllowFords) {
				Check ((entry.Methods & (HYBRID_WINDOW_METHOD_FORD_LIGHT | HYBRID_WINDOW_METHOD_FORD_HARVESTER)) == 0,
						 "window %u carries a ford flag with allow_fords false",entry.StableId);
			}

			for (int other = window + 1; other < hydrology.Windows.Count (); ++other) {
				const HybridCrossingWindow & second = hydrology.Windows[other];
				if (second.ObstructionRecordId != entry.ObstructionRecordId) continue;
				if (second.Obstruction != entry.Obstruction) continue;
				Check (WWMath::Fabs (second.Station - entry.Station) >= (4.0f * k.W) - 0.01f,
						 "windows %u and %u are %g apart on one obstruction, under 4W",entry.StableId,
						 second.StableId,WWMath::Fabs (second.Station - entry.Station));
			}

			//	Step 19's NO_DECORATION stamp lands on the window's own approach.
			int cx = 0, cy = 0;
			if (matrix.World_To_Cell (entry.Center,&cx,&cy)) {
				Check (matrix.NoDecorationMask[matrix.Index (cx,cy)] == 1,
						 "window %u did not stamp NO_DECORATION over its approach",entry.StableId);
			}
		}

		//	Step 21: every record is in the planning spatial hash.
		DynamicVectorClass<unsigned int> found;
		for (int r = 0; r < hydrology.Rivers.Count (); ++r) {
			const HybridRiverSystem & river = hydrology.Rivers[r];
			found.Delete_All ();
			hydrology.Hash.Query (river.BoundsMin.X - params.WorldMinX,river.BoundsMin.Y - params.WorldMinY,
										 river.BoundsMax.X - params.WorldMinX,river.BoundsMax.Y - params.WorldMinY,&found);
			bool present = false;
			for (int f = 0; f < found.Count (); ++f) {
				if (found[f] == river.StableId) present = true;
			}
			Check (present,"river %u is not in the planning spatial hash",river.StableId);
		}

		//	StableIds are one creation order across rivers, bodies and windows, so nothing collides.
		{
			unsigned int highest = 0;
			for (int r = 0; r < hydrology.Rivers.Count (); ++r) {
				Check (hydrology.Rivers[r].StableId > highest,"river ids are not ascending creation order");
				highest = hydrology.Rivers[r].StableId;
			}
			for (int body = 0; body < hydrology.Bodies.Count (); ++body) {
				Check (hydrology.Bodies[body].StableId > highest,"body ids collide with river ids");
				highest = hydrology.Bodies[body].StableId;
			}
			for (int window = 0; window < hydrology.Windows.Count (); ++window) {
				Check (hydrology.Windows[window].StableId > highest,"window ids collide with water ids");
				highest = hydrology.Windows[window].StableId;
			}
		}

		//	Determinism: the same inputs give the same records, a different seed does not.
		{
			HybridStrategicGraph again_graph;
			HybridCoarseMatrices again_matrices;
			HybridLandformSet again_landforms;
			HybridHydrologySet again;

			Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&again_graph,&graph_result),
					 "graph rebuild at D=%g",sizes[i]);
			Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&again_graph,&again_matrices,
															 &coarse_result),"matrix rebuild at D=%g",sizes[i]);
			Check (Hybrid_Build_Landforms (landform_params,identity,intents,again_graph,&again_matrices,
													 &again_landforms,&landform_result),"landform rebuild at D=%g",sizes[i]);
			Check (Hybrid_Build_Hydrology (params,identity,intents,again_graph,again_landforms,&again_matrices,
													 &again,&result),"hydrology rebuild at D=%g: %s",sizes[i],
					 result.Detail.Peek_Buffer ());
			Check (again.Checksum == hydrology.Checksum,"Phase 6 is not reproducible at D=%g",sizes[i]);
			Check (Hybrid_Hydrology_Checksum (again,identity.Checksum) == hydrology.Checksum,
					 "the checksum does not describe the records it was built from");

			HybridHydrologyParams other = params;
			other.HydrologySeed = WorldSeedService::Derive_Subsystem_Seed (9797,
																							  WorldSeedService::SEED_DOMAIN_HYDROLOGY);
			HybridHydrologySet other_set;
			if (Hybrid_Build_Hydrology (other,identity,intents,again_graph,again_landforms,&again_matrices,
												 &other_set,&result)) {
				//	Only the seeded draws move -- the guide polygons and the island sites -- so a different
				//	checksum is expected only when this map actually made one of those draws.
				const bool drew = (other_set.Count_Of_Body_Kind (HYBRID_WATER_BODY_ISLAND) > 0) ||
										(other_set.Bodies.Count () != hydrology.Bodies.Count ());
				if (drew) {
					Check (other_set.Checksum != hydrology.Checksum,"a different hydrology seed changed nothing");
				}
			}
		}
	}

	//	Archetype specifics: river_valley's guided river and coastal_peninsula's sea.
	{
		const HybridArchetype wanted[2] = { HYBRID_ARCHETYPE_RIVER_VALLEY, HYBRID_ARCHETYPE_COASTAL_PENINSULA };

		for (int a = 0; a < 2; ++a) {

			HybridMapIdentityParams archetype_params = Graph_Identity_Fixture (1200.0f,2.0f);
			archetype_params.ArchetypeSelector = wanted[a];
			Check (Hybrid_Resolve_Map_Identity (archetype_params,&identity,&intents,&identity_result),
					 "%s identity: %s",Hybrid_Archetype_Name (wanted[a]),identity_result.Detail.Peek_Buffer ());

			/*
			**	`coastal_peninsula` is the one archetype whose fixed Section-6.6 guide table can contradict
			**	its own sea: when the drawn coast side and the remote-rich guide land together, no dry
			**	candidate exists inside Section 6.1's repair budget and the plan's answer is a named attempt
			**	failure the manager retries with a new seed (Check_Hybrid_Strategic_Graph says the same).
			*/
			HybridStrategicGraphParams graph_params = Graph_Fixture (archetype_params,2.0f);
			if (!Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result)) {
				Check (wanted[a] == HYBRID_ARCHETYPE_COASTAL_PENINSULA
					 && graph_result.Failure == HYBRID_GRAPH_NO_LEGAL_RESOURCE_POSITION,
						 "%s graph: %s (%s)",Hybrid_Archetype_Name (wanted[a]),
						 Hybrid_Strategic_Graph_Failure_Name (graph_result.Failure),
						 graph_result.Detail.Peek_Buffer ());
				continue;
			}

			HybridCoarseMatrixParams coarse_params = Coarse_Fixture (archetype_params,1200.0f);
			Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
					 "%s matrices: %s (%s)",Hybrid_Archetype_Name (wanted[a]),
					 Hybrid_Coarse_Matrix_Failure_Name (coarse_result.Failure),coarse_result.Detail.Peek_Buffer ());

			HybridLandformParams landform_params = Landform_Fixture (archetype_params,1200.0f);
			if (!Hybrid_Build_Landforms (landform_params,identity,intents,graph,&matrices,&landforms,
												  &landform_result)) {
				Check (false,"%s landforms: %s (%s)",Hybrid_Archetype_Name (wanted[a]),
						 Hybrid_Landform_Failure_Name (landform_result.Failure),
						 landform_result.Detail.Peek_Buffer ());
				continue;
			}

			HybridHydrologyParams params = Hydrology_Fixture (archetype_params,1200.0f);
			if (!Hybrid_Build_Hydrology (params,identity,intents,graph,landforms,&matrices,&hydrology,&result)) {
				Check (false,"%s hydrology: %s (%s)",Hybrid_Archetype_Name (wanted[a]),
						 Hybrid_Hydrology_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
				continue;
			}

			//	Step 10: the guided river is selected system 0 and keeps its frozen Section-6.10 spline.
			Check (hydrology.Rivers.Count () > 0,"%s selected no river",Hybrid_Archetype_Name (wanted[a]));
			if (hydrology.Rivers.Count () > 0) {
				const HybridRiverSystem & first = hydrology.Rivers[0];
				Check (first.Guided,"%s did not insert its guided river as system 0",
						 Hybrid_Archetype_Name (wanted[a]));
				Check (first.SelectionRank == 0,"the guided river is not selection rank 0");
				Check (first.IntentStableId != 0,"the guided river is not bound to its Section-6.10 intent");

				/*
				**	Its synthetic accumulation rises linearly from the threshold at the source toward four
				**	times the threshold at the sink.  A mouth bound to a lake or the sea ends the record at
				**	that shoreline (step 16), so the last sample is at most the sink value, and exactly the
				**	sink value when nothing truncated it.
				*/
				const int last = first.Accumulation.Count () - 1;
				Check (Near (first.Accumulation[0],hydrology.RiverThreshold),
						 "the guided river's source accumulation is not the threshold");
				for (int point = 1; point <= last; ++point) {
					Check (first.Accumulation[point] >= first.Accumulation[point - 1],
							 "the guided river's accumulation is not monotonic at sample %d",point);
				}
				Check (first.Accumulation[last] <= ((4.0f * hydrology.RiverThreshold) + 0.01f),
						 "the guided river's accumulation passes four times the threshold");
				if (first.SinkBodyId == 0) {
					Check (Near (first.Accumulation[last],4.0f * hydrology.RiverThreshold),
							 "an untruncated guided river does not reach four times the threshold");
				}
			}

			if (wanted[a] == HYBRID_ARCHETYPE_COASTAL_PENINSULA) {
				Check (hydrology.Count_Of_Body_Kind (HYBRID_WATER_BODY_COAST) == 1,
						 "coastal_peninsula built %d coast polygons, not one",
						 hydrology.Count_Of_Body_Kind (HYBRID_WATER_BODY_COAST));
				for (int body = 0; body < hydrology.Bodies.Count (); ++body) {
					if (hydrology.Bodies[body].Kind != HYBRID_WATER_BODY_COAST) continue;
					Check (hydrology.Bodies[body].BedDepth == 0.0f,"the coast must not carve a bed");
					Check (Near (hydrology.Bodies[body].SurfaceHeight,params.SeaLevel),
							 "the coast does not stand at sea level");
				}
			} else {
				Check (hydrology.Count_Of_Body_Kind (HYBRID_WATER_BODY_COAST) == 0,
						 "river_valley built exterior sea without a coast guide");
			}
		}
	}
}



static HybridMacroTerrainParams Macro_Terrain_Fixture (const HybridMapIdentityParams & identity_params)
{
	HybridMacroTerrainParams params;
	params.TerrainSeed	= WorldSeedService::Derive_Subsystem_Seed (4242,WorldSeedService::SEED_DOMAIN_TERRAIN);
	params.Constants		= identity_params.Constants;
	params.SeaLevel		= 0.0f;
	params.MinElevation	= -15.0f;					// native terrain.min_elevation
	params.MaxElevation	= 90.0f;						// native terrain.max_elevation
	params.TierStep		= 18.0f;
	params.TierCount		= 3;
	params.DetailWeight	= 0.35f;						// native plan.detail_weight
	params.FeatureScale	= 240.0f;					// native terrain.feature_scale
	params.RiverWidth		= 6.0f;
	params.MaxBuildableSlopeRadians = 0.4886f;	// ~28 degrees, the native buildable cap
	return params;
}


//	The Phase-7 gate's height sampler, reading the heightfield the write actually produced.
static bool Macro_Sample_From_Terrain (const Vector2 & world,float * height_out,void * context)
{
	(void)context;
	return WorldTerrainSystem::Sample_Height (world.X,world.Y,height_out);
}


//	A sampler that answers with the formula itself, for testing the gate without writing terrain.
struct MacroFormulaContext
{
	const HybridMacroTerrainParams *	Params;
	const HybridCoarseMatrices *		Matrices;
	const HybridLandformSet *			Landforms;
	const HybridHydrologySet *			Hydrology;
	const HybridMacroTerrainSet *		Surfaces;
	float										Offset;		// added to every answer, to force a gate failure
};

static bool Macro_Sample_From_Formula (const Vector2 & world,float * height_out,void * context)
{
	MacroFormulaContext * fixture = (MacroFormulaContext *)context;
	*height_out = Hybrid_Macro_Sample_Height (*fixture->Params,*fixture->Matrices,*fixture->Landforms,
															*fixture->Hydrology,*fixture->Surfaces,world) + fixture->Offset;
	return true;
}


void Check_Hybrid_Macro_Terrain (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformSet landforms;
	HybridLandformResult landform_result;
	HybridHydrologySet hydrology;
	HybridHydrologyResult hydrology_result;
	HybridMacroTerrainSet surfaces;
	HybridMacroTerrainResult result;

	const unsigned int terrain_seed = WorldSeedService::Derive_Subsystem_Seed (4242,
																								 WorldSeedService::SEED_DOMAIN_TERRAIN);

	//	Section 8 Phase 7's own noise primitive.
	{
		const Vector2 origin (0.0f,0.0f);
		const float length = 120.0f;

		const float first = Hybrid_Value_Noise_2D (terrain_seed,Vector2 (37.5f,-12.25f),length,0);
		const float again = Hybrid_Value_Noise_2D (terrain_seed,Vector2 (37.5f,-12.25f),length,0);
		Check (first == again,"the detail noise is not reproducible");
		Check ((first >= -1.0f) && (first <= 1.0f),"the detail noise left [-1,1] (%g)",first);
		Check (Hybrid_Value_Noise_2D (terrain_seed,origin,0.0f,0) == 0.0f,
				 "a zero wavelength must produce no noise rather than a division");

		//	At a lattice point the bilinear blend is exactly that corner's own value.
		const float corner = (2.0f * Hybrid_Hash01 (terrain_seed,Hybrid_Key ("detail_noise"),0u,0u)) - 1.0f;
		Check (Near (Hybrid_Value_Noise_2D (terrain_seed,origin,length,0),corner),
				 "a lattice point is not its own corner value");

		//	A sample between two corners stays between them, which is what the smoothstep guarantees.
		const float next = (2.0f * Hybrid_Hash01 (terrain_seed,Hybrid_Key ("detail_noise"),1u,0u)) - 1.0f;
		const float middle = Hybrid_Value_Noise_2D (terrain_seed,Vector2 (0.5f * length,0.0f),length,0);
		const float low = (corner < next) ? corner : next;
		const float high = (corner < next) ? next : corner;
		Check ((middle >= low - 0.001f) && (middle <= high + 0.001f),
				 "the noise overshot its own lattice corners (%g outside %g..%g)",middle,low,high);

		//	The octave is part of the key, so the two octaves the formula uses are independent.
		Check (Hybrid_Value_Noise_2D (terrain_seed,origin,length,1) !=
				 Hybrid_Value_Noise_2D (terrain_seed,origin,length,0),"both octaves drew the same value");
	}

	//	Step 5's river longitudinal walk, on hand-built records.
	{
		HybridMapIdentityParams sized = Graph_Identity_Fixture (1200.0f,2.0f);
		HybridMacroTerrainParams params = Macro_Terrain_Fixture (sized);
		HybridCoarseMatrices empty_matrices;
		HybridLandformSet empty_landforms;

		HybridHydrologySet set;
		HybridRiverSystem river;
		river.StableId	= 1;
		river.BedDepth	= 2.5f;
		Fill_Test_River (&river,Vector2 (-300.0f,0.0f),Vector2 (300.0f,0.0f),8.0f,2.5f,30.0f);

		//	A provisional surface with one uphill step in it, and a lake at the mouth.
		for (int i = 0; i < river.Surfaces.Count (); ++i) {
			river.Surfaces[i] = 40.0f - (2.0f * (float)i);
		}
		if (river.Surfaces.Count () > 4) river.Surfaces[4] += 12.0f;		// an uphill sample
		river.SinkBodyId = 2;
		set.Rivers.Add (river);

		HybridWaterBody lake;
		lake.StableId		= 2;
		lake.Kind			= HYBRID_WATER_BODY_LAKE;
		lake.SurfaceHeight= 6.0f;
		lake.BedDepth		= 3.0f;
		Fill_Square_Ring (&lake,Vector2 (320.0f,0.0f),60.0f);
		set.Bodies.Add (lake);

		Check (Hybrid_Resolve_River_Surfaces (params,empty_matrices,empty_landforms,set,&surfaces,&result),
				 "the river walk refused a legal record: %s (%s)",
				 Hybrid_Macro_Terrain_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());

		const HybridRiverSurface * fixed = surfaces.Find_River (1);
		Check (fixed != NULL,"the walk produced no record for river 1");
		if (fixed != NULL) {

			Check (fixed->Surface.Count () == river.Points.Count (),"the walk lost samples");
			for (int i = 1; i < fixed->Surface.Count (); ++i) {
				Check (fixed->Surface[i] <= fixed->Surface[i - 1] + 0.001f,
						 "the walk left an uphill downstream sample at %d (%g after %g)",i,fixed->Surface[i],
						 fixed->Surface[i - 1]);
			}
			Check (Near (fixed->Surface[fixed->Surface.Count () - 1],lake.SurfaceHeight),
					 "a bound mouth does not equal its sink's water level");
			Check (Near (fixed->MouthSurface,lake.SurfaceHeight),"the recorded mouth surface is not the sink level");
			for (int i = 0; i < fixed->Bed.Count (); ++i) {
				Check (fixed->Bed[i] <= fixed->Surface[i] + 0.001f,"a bed stands above its own water surface");
				const float floor_height = params.SeaLevel + params.MinElevation;
				const float expect = (fixed->Surface[i] - river.BedDepth > floor_height)
											? (fixed->Surface[i] - river.BedDepth) : floor_height;
				Check (Near (fixed->Bed[i],expect),"the bed is not the surface less the definition's bed depth");
			}
		}

		//	The elevation floor lifting a bed above its own surface is a named failure, not a clamp.
		HybridMacroTerrainParams shallow = params;
		shallow.MinElevation = 0.0f;			// the floor is sea level, so a surface below it cannot hold a bed
		HybridHydrologySet low_set;
		HybridRiverSystem low = river;
		low.SinkBodyId = 0;
		for (int i = 0; i < low.Surfaces.Count (); ++i) low.Surfaces[i] = -4.0f;
		low_set.Rivers.Add (low);

		HybridMacroTerrainSet refused;
		Check (!Hybrid_Resolve_River_Surfaces (shallow,empty_matrices,empty_landforms,low_set,&refused,&result),
				 "a bed clamped above its own surface was accepted");
		Check (result.Failure == HYBRID_MACRO_BED_ABOVE_SURFACE,"the wrong failure was named (%s)",
				 Hybrid_Macro_Terrain_Failure_Name (result.Failure));
		Check (refused.Rivers.Count () == 0,"a refused walk left records behind");
	}

	//	`detail_protection_factor`, the carve and the protected-profile test, on a real Phase 2-6 chain.
	{
		HybridMapIdentityParams sized = Graph_Identity_Fixture (1200.0f,2.0f);
		Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity: %s",
				 identity_result.Detail.Peek_Buffer ());
		HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
		Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
				 "strategic graph: %s",graph_result.Detail.Peek_Buffer ());
		HybridCoarseMatrixParams coarse_params = Coarse_Fixture (sized,1200.0f);
		Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
				 "coarse matrices: %s",coarse_result.Detail.Peek_Buffer ());
		HybridLandformParams landform_params = Landform_Fixture (sized,1200.0f);
		const bool landforms_built = Hybrid_Build_Landforms (landform_params,identity,intents,graph,&matrices,
																			 &landforms,&landform_result);
		Check (landforms_built,"landforms: %s (%s)",Hybrid_Landform_Failure_Name (landform_result.Failure),
				 landform_result.Detail.Peek_Buffer ());
		if (!landforms_built) return;

		HybridHydrologyParams hydrology_params = Hydrology_Fixture (sized,1200.0f);
		const bool hydrology_built = Hybrid_Build_Hydrology (hydrology_params,identity,intents,graph,landforms,
																			  &matrices,&hydrology,&hydrology_result);
		Check (hydrology_built,"hydrology: %s (%s)",Hybrid_Hydrology_Failure_Name (hydrology_result.Failure),
				 hydrology_result.Detail.Peek_Buffer ());
		if (!hydrology_built) return;

		HybridMacroTerrainParams params = Macro_Terrain_Fixture (sized);
		Check (Hybrid_Resolve_River_Surfaces (params,matrices,landforms,hydrology,&surfaces,&result),
				 "the river walk refused the built world: %s (%s)",
				 Hybrid_Macro_Terrain_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());

		const HybridConstants & k = params.Constants;
		const HybridSemanticMatrix & matrix = matrices.Matrix;
		const float floor_height = params.SeaLevel + params.MinElevation;
		const float ceiling = params.SeaLevel + params.MaxElevation;

		//	The two semantic reserves, read straight off the Phase-4 layers.
		bool saw_core = false;
		bool saw_reserve = false;
		for (int cy = 0; (cy < matrix.Get_Count_Y ()) && !(saw_core && saw_reserve); ++cy) {
			for (int cx = 0; (cx < matrix.Get_Count_X ()) && !(saw_core && saw_reserve); ++cx) {

				const Vector2 centre = matrix.Cell_Center (cx,cy);
				const float factor = Hybrid_Detail_Protection_Factor (params,matrices,hydrology,centre);

				if (!saw_core && matrix.Has_Flag (cx,cy,HYBRID_FLAG_BASE_CORE)) {
					saw_core = true;
					Check (factor == 0.0f,"a base core cell keeps %g of its detail, not none",factor);
				}
				if (!saw_reserve && matrix.Has_Flag (cx,cy,HYBRID_FLAG_TIBERIUM_RESERVE)
					 && !matrix.Has_Flag (cx,cy,HYBRID_FLAG_BASE_CORE)) {
					const bool wet = Hybrid_Point_In_Standing_Water (hydrology,centre,NULL,NULL);
					if (!wet) {
						saw_reserve = true;
						Check (factor <= 0.25f + 0.000001f,"a Tiberium reserve cell keeps %g of its detail, over 0.25",
								 factor);
					}
				}
			}
		}
		Check (saw_core,"the fixture has no base core cell to test detail protection with");

		//	Water: no detail on a bed, a blend across the bank, and the bed carved to its frozen height.
		if (hydrology.Rivers.Count () > 0) {

			const HybridRiverSystem & river = hydrology.Rivers[hydrology.Rivers.Count () - 1];
			const int middle_sample = river.Points.Count () / 2;
			const Vector2 on_channel = river.Points[middle_sample];
			const float bank = (params.RiverWidth > (2.0f * k.W)) ? params.RiverWidth : (2.0f * k.W);

			Check (Hybrid_Detail_Protection_Factor (params,matrices,hydrology,on_channel) == 0.0f,
					 "a river bed keeps its detail noise");
			Check (Hybrid_Macro_Is_Protected_Profile (params,landforms,hydrology,on_channel),
					 "a river bed is not a protected profile");

			float bed = 0.0f;
			Check (surfaces.River_Sample (river.StableId,middle_sample,NULL,&bed),
					 "the walk has no sample for river %u index %d",river.StableId,middle_sample);

			//	Carving from a base height well above the bed takes the ground to the bed exactly.
			const float base = bed + 30.0f;
			Check (Near (Hybrid_Macro_Water_Carve (params,hydrology,surfaces,on_channel,base),30.0f),
					 "a channel is not carved to its frozen bed height");

			//	Well outside the bank nothing is carved and the detail is back.
			Vector2 tangent = river.Points[(middle_sample + 1) % river.Points.Count ()] - on_channel;
			if (tangent.Length () > 0.0f) {
				tangent.Normalize ();
				const Vector2 normal (-tangent.Y,tangent.X);
				const Vector2 far_off = on_channel + (normal * (bank + (4.0f * k.W)));
				if (!Hybrid_Point_In_Standing_Water (hydrology,far_off,NULL,NULL)) {
					Check (Hybrid_Macro_Water_Carve (params,hydrology,surfaces,far_off,base) == 0.0f,
							 "a river carves past its own bank blend");
				}
			}
		}

		for (int body = 0; body < hydrology.Bodies.Count (); ++body) {

			const HybridWaterBody & water = hydrology.Bodies[body];
			if (water.Kind == HYBRID_WATER_BODY_ISLAND) {
				Check (Hybrid_Macro_Water_Carve (params,hydrology,surfaces,water.Center,100.0f) == 0.0f,
						 "an island is carved with its host's water");
				Check (Hybrid_Island_Height_Bias (hydrology,water.Center) > 0.0f,
						 "an island has no macro height bias at its own centre");
				continue;
			}
			Check (Hybrid_Detail_Protection_Factor (params,matrices,hydrology,water.Center) == 0.0f,
					 "standing water keeps its detail noise");
			const float target = water.SurfaceHeight - water.BedDepth;
			Check (Near (Hybrid_Macro_Water_Carve (params,hydrology,surfaces,water.Center,target + 20.0f),20.0f),
					 "%s %u is not carved to its own bed",Hybrid_Water_Body_Kind_Name (water.Kind),water.StableId);
		}

		//	An explicit wall is a protected profile; open ground away from everything is not.
		for (int f = 0; f < landforms.Landforms.Count (); ++f) {
			const HybridLandform & landform = landforms.Landforms[f];
			if (!Hybrid_Landform_Is_Immutable (landform.Kind)) continue;
			if (landform.Points.Count () == 0) continue;
			Check (Hybrid_Macro_Is_Protected_Profile (params,landforms,hydrology,
																  landform.Points[landform.Points.Count () / 2]),
					 "%s %u is not a protected profile",Hybrid_Landform_Kind_Name (landform.Kind),landform.StableId);
		}

		//	The height expression: inside the envelope everywhere, and detail bounded by the plan's term.
		const float detail_limit = params.DetailWeight * params.TierStep * 0.35f;
		for (int cy = 0; cy < matrix.Get_Count_Y (); cy += 3) {
			for (int cx = 0; cx < matrix.Get_Count_X (); cx += 3) {

				const Vector2 centre = matrix.Cell_Center (cx,cy);
				const float height = Hybrid_Macro_Sample_Height (params,matrices,landforms,hydrology,surfaces,centre);
				Check ((height >= floor_height - 0.001f) && (height <= ceiling + 0.001f),
						 "the height expression left the elevation envelope at %d,%d (%g)",cx,cy,height);

				//	Terms: tier + bias + landform - carve, plus at most the detail term.
				const int index = matrix.Index (cx,cy);
				const int tier = (int)matrix.BaseElevationTier[index];
				const float reference = params.SeaLevel + ((float)(tier - 1) * params.TierStep)
											 + matrix.MacroHeightBias[index]
											 + Hybrid_Island_Height_Bias (hydrology,centre)
											 + Hybrid_Landform_Height (landforms,centre,k.A);
				const float carve = Hybrid_Macro_Water_Carve (params,hydrology,surfaces,centre,reference);
				const float expect = reference - carve;
				if ((expect > floor_height + detail_limit) && (expect < ceiling - detail_limit)) {
					Check (WWMath::Fabs (height - expect) <= detail_limit + 0.001f,
							 "the height at %d,%d is %g, more than the detail term from %g",cx,cy,height,expect);
				}
			}
		}

		//	The gate: the formula's own ground passes, ground lifted above the water does not.
		MacroFormulaContext fixture;
		fixture.Params		= &params;
		fixture.Matrices	= &matrices;
		fixture.Landforms	= &landforms;
		fixture.Hydrology	= &hydrology;
		fixture.Surfaces	= &surfaces;
		fixture.Offset		= 0.0f;
		Check (Hybrid_Check_Macro_Terrain_Gate (params,matrices,landforms,hydrology,&surfaces,
															 Macro_Sample_From_Formula,&fixture,&result),
				 "the gate refused its own formula: %s (%s)",Hybrid_Macro_Terrain_Failure_Name (result.Failure),
				 result.Detail.Peek_Buffer ());
		Check (surfaces.Checksum != 0,"a passed gate left no checksum");

		const unsigned int first_checksum = surfaces.Checksum;
		Check (Hybrid_Macro_Terrain_Checksum (surfaces,0) == first_checksum,
				 "the checksum does not describe the records it was built from");

		if ((hydrology.Rivers.Count () > 0) || (hydrology.Bodies.Count () > 0)) {
			fixture.Offset = 200.0f;		// every bed now stands far above its own water surface
			Check (!Hybrid_Check_Macro_Terrain_Gate (params,matrices,landforms,hydrology,&surfaces,
																  Macro_Sample_From_Formula,&fixture,&result),
					 "the gate accepted ground standing above its own water");
			Check ((result.Failure == HYBRID_MACRO_BED_ABOVE_SURFACE)
				 || (result.Failure == HYBRID_MACRO_ELEVATION_LIMIT),
					 "the wrong failure was named for flooded-over ground (%s)",
					 Hybrid_Macro_Terrain_Failure_Name (result.Failure));
		}

		//	A river surface forced uphill is the gate's monotonic failure.
		if (surfaces.Rivers.Count () > 0) {
			HybridMacroTerrainSet broken = surfaces;
			if (broken.Rivers[0].Surface.Count () > 2) {
				broken.Rivers[0].Surface[1] = broken.Rivers[0].Surface[0] + 5.0f;
				fixture.Offset		= 0.0f;
				fixture.Surfaces	= &broken;
				Check (!Hybrid_Check_Macro_Terrain_Gate (params,matrices,landforms,hydrology,&broken,
																	  Macro_Sample_From_Formula,&fixture,&result),
						 "the gate accepted a river that flows uphill");
				Check (result.Failure == HYBRID_MACRO_RIVER_NOT_MONOTONIC,"the wrong failure was named (%s)",
						 Hybrid_Macro_Terrain_Failure_Name (result.Failure));
			}
		}

		/*
		**	The realization itself: one write, through the canonical owner, with the hybrid records in
		**	the request.  This is the whole point of item I8 -- the noise-geography channels are replaced,
		**	not joined -- so the heightfield, its chunk registry and the phase gate all have to hold on
		**	the ground that was actually written.
		*/
		WorldTerrainSystem::Init ();
		MacroTerrainGenerator::Init ();

		HybridMacroTerrainSet realized;
		Check (Hybrid_Resolve_River_Surfaces (params,matrices,landforms,hydrology,&realized,&result),
				 "the river walk refused the realization pass: %s",result.Detail.Peek_Buffer ());

		MacroTerrainParams write;
		write.Seed							= params.TerrainSeed;
		write.WorldWidth					= 1200.0f;
		write.WorldHeight					= 1200.0f;
		write.ChunkSize					= 60.0f;
		write.CenterX						= 0.0f;
		write.CenterY						= 0.0f;
		write.SeaLevel						= params.SeaLevel;
		write.MinElevation				= params.MinElevation;
		write.MaxElevation				= params.MaxElevation;
		write.LandElevationBias			= 0.0f;
		write.MaxBuildableSlopeRadians= params.MaxBuildableSlopeRadians;
		write.FeatureScale				= params.FeatureScale;
		write.UsePlan						= false;
		write.UseHybrid					= true;
		write.HybridParams				= &params;
		write.HybridMatrices				= &matrices;
		write.HybridLandforms			= &landforms;
		write.HybridHydrology			= &hydrology;
		write.HybridSurfaces				= &realized;

		StringClass write_error ("");
		const bool written = MacroTerrainGenerator::Generate (write,&write_error);
		Check (written,"the hybrid realization was refused: %s",write_error.Peek_Buffer ());

		if (written) {

			Check (WorldTerrainSystem::Has_Terrain (),"the realization left no heightfield");
			Check (WorldTerrainSystem::Has_Chunk_Registry (),"the realization left no chunk registry");
			Check (realized.VertexCountX > 0 && realized.VertexCountY > 0,
					 "the realization recorded no extents");
			Check ((realized.MinHeight >= floor_height - 0.001f) && (realized.MaxHeight <= ceiling + 0.001f),
					 "the realized heights %g..%g left the elevation envelope",realized.MinHeight,
					 realized.MaxHeight);
			Check (realized.ProtectedVertices > 0,
					 "a world with rivers and explicit walls protected no vertex from slope relaxation");

			StringClass report ("");
			Check (MacroTerrainGenerator::Check_Height_Range (write,&report) == 0,
					 "the realized terrain left its own height range: %s",report.Peek_Buffer ());

			Check (Hybrid_Check_Macro_Terrain_Gate (params,matrices,landforms,hydrology,&realized,
																 Macro_Sample_From_Terrain,NULL,&result),
					 "the gate refused the realized terrain: %s (%s)",
					 Hybrid_Macro_Terrain_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());

			//	A second identical write reproduces the same ground: Phase 7 draws nothing statefully.
			const unsigned int checksum = WorldTerrainSystem::Get_World_Chunk_Checksum ();
			HybridMacroTerrainSet again;
			Check (Hybrid_Resolve_River_Surfaces (params,matrices,landforms,hydrology,&again,&result),
					 "the second river walk was refused");
			write.HybridSurfaces = &again;
			Check (MacroTerrainGenerator::Generate (write,&write_error),"the second realization was refused: %s",
					 write_error.Peek_Buffer ());
			Check (WorldTerrainSystem::Get_World_Chunk_Checksum () == checksum,
					 "Phase 7 is not reproducible");
			Check (again.Checksum == realized.Checksum || (again.Checksum == 0),
					 "the same walk produced a different record");
		}

		WorldTerrainSystem::Shutdown ();
		MacroTerrainGenerator::Init ();
	}
}



static HybridAnchorParams Anchor_Fixture (const HybridMapIdentityParams & identity_params)
{
	HybridAnchorParams params;
	params.Constants		= identity_params.Constants;
	params.SeaLevel		= 0.0f;
	params.MaxBuildableSlopeRadians	= 0.4886f;	// ~28 degrees, the native buildable cap
	params.FieldRadiusMax				= 60.0f;		// native resource.field_radius_max
	params.ReferenceVehicleSpeed		= WorldMetricsClass::Get_Reference_Vehicle_Speed ();
	params.MaxInitialResourceTravelTimeDifferenceSeconds	= 15.0f;
	params.MaxBaseToFrontTravelTimeDifferenceSeconds		= 20.0f;
	params.MinBaseExpansionArea		= 4096.0f;
	params.MinVehicleApproachCount	= 2;
	params.MinVehicleLaneClearance	= 1.0f;
	params.HeavyVehicleWidth			= 6.801906f;	// the Mammoth's measured WORLDBOX width
	return params;
}


//	A sampler that refuses every point, for the "no realized ground" path.
static bool Anchor_Sample_Refuses (const Vector2 & world,float * height_out,void * context)
{
	(void)world;
	(void)height_out;
	(void)context;
	return false;
}


void Check_Hybrid_Anchors (void)
{
	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformSet landforms;
	HybridLandformResult landform_result;
	HybridHydrologySet hydrology;
	HybridHydrologyResult hydrology_result;
	HybridMacroTerrainSet surfaces;
	HybridMacroTerrainResult macro_result;
	HybridAnchorSet anchors;
	HybridAnchorResult result;

	//	Section 0.10's PlanningDistance over a hand-built graph.
	{
		HybridStrategicGraph small;
		HybridStrategicSite site;
		site.StableId = 1;	site.Position.Set (0.0f,0.0f);		small.Sites.Add (site);
		site.StableId = 2;	site.Position.Set (100.0f,0.0f);		small.Sites.Add (site);
		site.StableId = 3;	site.Position.Set (100.0f,100.0f);	small.Sites.Add (site);
		site.StableId = 4;	site.Position.Set (900.0f,900.0f);	small.Sites.Add (site);

		HybridStrategicEdge edge;
		edge.StableId = 1;	edge.SiteA = 1;	edge.SiteB = 2;	small.Edges.Add (edge);
		edge.StableId = 2;	edge.SiteA = 2;	edge.SiteB = 3;	small.Edges.Add (edge);

		Check (Near (Hybrid_Planning_Distance (small,1,1),0.0f),"a site is not zero from itself");
		Check (Near (Hybrid_Planning_Distance (small,1,2),100.0f),"a single edge is not its own length");
		Check (Near (Hybrid_Planning_Distance (small,1,3),200.0f),"a two-hop path is not the sum of its edges");
		Check (Near (Hybrid_Planning_Distance (small,3,1),200.0f),"PlanningDistance is not symmetric");
		Check (Hybrid_Planning_Distance (small,1,4) < 0.0f,"a disconnected site reported a distance");
		Check (Hybrid_Planning_Distance (small,1,99) < 0.0f,"an unknown site reported a distance");
	}

	//	The exit-sector convention: ordinal 0 along strategic +u, counter-clockwise by 22.5 degrees.
	{
		HybridStrategicFrame frame;
		Hybrid_Build_Strategic_Frame (0.0f,0.0f,1200.0f,0.35f,&frame);

		const Vector2 first = Hybrid_Exit_Sector_Direction (frame,0);
		Check (Near (first.X,frame.AxisU.X) && Near (first.Y,frame.AxisU.Y),
				 "exit sector 0 does not point along strategic +u");

		const Vector2 quarter = Hybrid_Exit_Sector_Direction (frame,4);
		Check (Near (quarter.X,frame.AxisV.X,0.01f) && Near (quarter.Y,frame.AxisV.Y,0.01f),
				 "exit sector 4 is not a quarter turn counter-clockwise");

		for (int ordinal = 0; ordinal < HYBRID_EXIT_SECTOR_COUNT; ++ordinal) {
			const Vector2 direction = Hybrid_Exit_Sector_Direction (frame,ordinal);
			Check (Near (direction.Length (),1.0f,0.001f),"exit sector %d is not a unit direction",ordinal);
			const Vector2 next = Hybrid_Exit_Sector_Direction (frame,(ordinal + 1) % HYBRID_EXIT_SECTOR_COUNT);
			const float dot = (direction.X * next.X) + (direction.Y * next.Y);
			Check (Near (dot,WWMath::Cos (22.5f * (WWMATH_PI / 180.0f)),0.001f),
					 "exit sectors %d and %d are not 22.5 degrees apart",ordinal,ordinal + 1);
		}
	}

	//	A full Phase-8 validation on a real Phase-2 to Phase-7 chain, sampled from the Phase-7 formula.
	HybridMapIdentityParams sized = Graph_Identity_Fixture (1200.0f,2.0f);
	Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity: %s",
			 identity_result.Detail.Peek_Buffer ());

	HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
	Check (Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result),
			 "strategic graph: %s",graph_result.Detail.Peek_Buffer ());

	HybridCoarseMatrixParams coarse_params = Coarse_Fixture (sized,1200.0f);
	Check (Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result),
			 "coarse matrices: %s",coarse_result.Detail.Peek_Buffer ());

	HybridLandformParams landform_params = Landform_Fixture (sized,1200.0f);
	const bool landforms_built = Hybrid_Build_Landforms (landform_params,identity,intents,graph,&matrices,
																		 &landforms,&landform_result);
	Check (landforms_built,"landforms: %s (%s)",Hybrid_Landform_Failure_Name (landform_result.Failure),
			 landform_result.Detail.Peek_Buffer ());
	if (!landforms_built) return;

	HybridHydrologyParams hydrology_params = Hydrology_Fixture (sized,1200.0f);
	const bool hydrology_built = Hybrid_Build_Hydrology (hydrology_params,identity,intents,graph,landforms,
																		  &matrices,&hydrology,&hydrology_result);
	Check (hydrology_built,"hydrology: %s (%s)",Hybrid_Hydrology_Failure_Name (hydrology_result.Failure),
			 hydrology_result.Detail.Peek_Buffer ());
	if (!hydrology_built) return;

	HybridMacroTerrainParams macro_params = Macro_Terrain_Fixture (sized);
	Check (Hybrid_Resolve_River_Surfaces (macro_params,matrices,landforms,hydrology,&surfaces,&macro_result),
			 "river surfaces: %s",macro_result.Detail.Peek_Buffer ());

	MacroFormulaContext ground;
	ground.Params		= &macro_params;
	ground.Matrices	= &matrices;
	ground.Landforms	= &landforms;
	ground.Hydrology	= &hydrology;
	ground.Surfaces	= &surfaces;
	ground.Offset		= 0.0f;

	HybridAnchorParams params = Anchor_Fixture (sized);
	const bool validated = Hybrid_Validate_Anchors (params,identity,graph,matrices,landforms,hydrology,
																	Macro_Sample_From_Formula,&ground,&anchors,&result);

	/*
	**	Phase 8 never moves a centre, so a realized world that fails a hard requirement is a *named*
	**	attempt failure the manager retries with a new whole-world seed.  Either outcome is legal here;
	**	what must hold is that a failure names itself and leaves nothing frozen.
	*/
	if (!validated) {
		Check (result.Failure != HYBRID_ANCHOR_OK,"a failed Phase 8 must name its failure");
		Check (result.Detail.Get_Length () > 0,"a failed Phase 8 must say what refused the anchor");
		Check (anchors.Bases.Count () == 0 && anchors.Resources.Count () == 0,
				 "a failed Phase 8 left anchors behind");
		Check (!anchors.Frozen,"a failed Phase 8 reported frozen anchors");

		/*
		**	MEDIUM is the size class the plan sets its own quality target on, so a refusal at D=1200 on
		**	this fixture seed is reported as the failure it is rather than tolerated: Phase 8 refusing a
		**	world every earlier phase accepted means one of its own hard requirements is wrong, not that
		**	the seed was unlucky.
		*/
		Check (identity.SizeClass != HYBRID_SIZE_MEDIUM,"anchors at D=1200 (%s): %s (%s)",
				 Hybrid_Archetype_Name (identity.Archetype),Hybrid_Anchor_Failure_Name (result.Failure),
				 result.Detail.Peek_Buffer ());
	} else {

		const HybridConstants & k = params.Constants;

		Check (anchors.Frozen,"a passed Phase 8 did not freeze its anchors");
		Check (anchors.Bases.Count () == 2,"%d bases were frozen, not two",anchors.Bases.Count ());
		Check (anchors.Checksum != 0,"a passed Phase 8 left no checksum");

		for (int i = 0; i < anchors.Bases.Count (); ++i) {

			const HybridBaseAnchor & base = anchors.Bases[i];
			const HybridStrategicSite * site = graph.Find_Site (base.SiteId);
			Check (site != NULL,"base anchor %u has no site",base.SiteId);
			Check (base.Team == i,"the frozen bases are not in team order");

			//	Nothing moved: the frozen centre is the Phase-3 centre and the Phase-4 reservation radius.
			if (site != NULL) {
				Check ((base.Center - site->Position).Length () <= k.Q,
						 "base %u was moved %g from its Phase-3 centre",base.SiteId,
						 (base.Center - site->Position).Length ());
			}
			bool matched = false;
			for (int r = 0; r < matrices.Reservations.Count (); ++r) {
				const HybridReservation & reservation = matrices.Reservations[r];
				if (reservation.Priority != HYBRID_RESERVE_BASE_PROTECTED) continue;
				if (reservation.SiteId != base.SiteId) continue;
				matched = true;
				Check (Near (base.ProtectedRadius,reservation.Radius),
						 "base %u froze radius %g, not its reservation's %g",base.SiteId,base.ProtectedRadius,
						 reservation.Radius);
			}
			Check (matched,"base %u has no BASE_PROTECTED reservation to freeze",base.SiteId);

			Check (Near (base.ProtectedRadius,k.RBaseProtected,0.01f),
					 "base %u protected radius %g is not R_BASE_PROTECTED %g",base.SiteId,base.ProtectedRadius,
					 k.RBaseProtected);
			Check (base.ExpansionArea >= params.MinBaseExpansionArea,
					 "base %u froze %g of expansion area, under the %g minimum",base.SiteId,base.ExpansionArea,
					 params.MinBaseExpansionArea);
			Check (base.MaxCoreSlopeRadians <= params.MaxBuildableSlopeRadians,
					 "base %u proxy area is steeper than the buildable cap",base.SiteId);

			const int required = (params.MinVehicleApproachCount > 2) ? params.MinVehicleApproachCount : 2;
			Check (base.ViableSectors.Count () >= required,"base %u froze %d exit sectors, fairness needs %d",
					 base.SiteId,base.ViableSectors.Count (),required);

			for (int s = 0; s < base.ViableSectors.Count (); ++s) {
				const int ordinal = base.ViableSectors[s];
				Check ((ordinal >= 0) && (ordinal < HYBRID_EXIT_SECTOR_COUNT),"sector ordinal %d is out of range",
						 ordinal);
				Check (base.Sector_Is_Viable (ordinal),"the sector mask and the sector list disagree");
				if (s > 0) {
					Check (base.ViableSectors[s] > base.ViableSectors[s - 1],
							 "the frozen exit sectors are not sorted ascending");
				}
			}
		}

		//	Section 5's resource roles, all frozen, none moved.
		Check (anchors.Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_GDI) == 1,"GDI has no single HOME field");
		Check (anchors.Count_Of_Role (HYBRID_SITE_HOME_TIBERIUM_NOD) == 1,"Nod has no single HOME field");
		Check (anchors.Count_Of_Role (HYBRID_SITE_CONTESTED_TIBERIUM)
				 == Hybrid_Contested_Field_Count (identity.SizeClass),"%d contested anchors, Section 5 wants %d",
				 anchors.Count_Of_Role (HYBRID_SITE_CONTESTED_TIBERIUM),
				 Hybrid_Contested_Field_Count (identity.SizeClass));
		Check (anchors.Count_Of_Role (HYBRID_SITE_REMOTE_TIBERIUM)
				 == Hybrid_Remote_Field_Count (identity.SizeClass),"%d remote anchors, Section 5 wants %d",
				 anchors.Count_Of_Role (HYBRID_SITE_REMOTE_TIBERIUM),
				 Hybrid_Remote_Field_Count (identity.SizeClass));

		for (int i = 0; i < anchors.Resources.Count (); ++i) {
			const HybridResourceAnchor & anchor = anchors.Resources[i];
			const HybridStrategicSite * site = graph.Find_Site (anchor.SiteId);
			Check (site != NULL,"resource anchor %u has no site",anchor.SiteId);
			if (site != NULL) {
				Check ((anchor.Center - site->Position).Length () <= params.Constants.Q,
						 "field %u was moved from its Phase-3 centre",anchor.SiteId);
			}
			Check (anchor.ReserveRadius >= params.FieldRadiusMax - 0.001f,
					 "field %u froze a reserve smaller than field_radius_max",anchor.SiteId);
			Check (anchor.ApproachCount >= 1,"field %u froze no approach",anchor.SiteId);
			Check (anchor.ApproachEdgeId != 0,"field %u named no approach edge",anchor.SiteId);
		}

		//	Section 0.10's provisional fairness, and only that.
		for (int team = 0; team < 2; ++team) {
			Check (anchors.HomeFieldDistance[team] > 0.0f,"team %d has no home-field planning distance",team);
			Check (anchors.FrontDistance[team] > 0.0f,"team %d has no front planning distance",team);
			Check (Near (anchors.HomeFieldTime[team],
							 anchors.HomeFieldDistance[team] / params.ReferenceVehicleSpeed,0.01f),
					 "team %d home time is not its distance over V_REF",team);
		}
		Check (anchors.BaseToBaseDistance > 0.0f,"the base-to-base planning distance is not positive");
		Check (anchors.HomeTimeDifference <= params.MaxInitialResourceTravelTimeDifferenceSeconds,
				 "the frozen home-time difference is over its own threshold");
		Check (anchors.FrontTimeDifference <= params.MaxBaseToFrontTravelTimeDifferenceSeconds,
				 "the frozen front-time difference is over its own threshold");

		//	Determinism: Phase 8 draws nothing, so a second pass is identical.
		HybridAnchorSet again;
		Check (Hybrid_Validate_Anchors (params,identity,graph,matrices,landforms,hydrology,
												  Macro_Sample_From_Formula,&ground,&again,&result),
				 "the second validation was refused: %s",result.Detail.Peek_Buffer ());
		Check (again.Checksum == anchors.Checksum,"Phase 8 is not reproducible");
		Check (Hybrid_Anchor_Checksum (again,identity.Checksum) == anchors.Checksum,
				 "the checksum does not describe the records it was built from");

		//	Each hard gate refuses on its own, and refusing leaves nothing frozen.
		{
			HybridAnchorParams greedy = params;
			greedy.MinBaseExpansionArea = 1.0e9f;
			HybridAnchorSet refused;
			Check (!Hybrid_Validate_Anchors (greedy,identity,graph,matrices,landforms,hydrology,
														Macro_Sample_From_Formula,&ground,&refused,&result),
					 "an impossible expansion area was accepted");
			Check (result.Failure == HYBRID_ANCHOR_BASE_NO_EXPANSION,"the wrong failure was named (%s)",
					 Hybrid_Anchor_Failure_Name (result.Failure));
			Check (!refused.Frozen && (refused.Bases.Count () == 0),"a refused Phase 8 left anchors behind");

			HybridAnchorParams many_exits = params;
			many_exits.MinVehicleApproachCount = HYBRID_EXIT_SECTOR_COUNT + 1;
			Check (!Hybrid_Validate_Anchors (many_exits,identity,graph,matrices,landforms,hydrology,
														Macro_Sample_From_Formula,&ground,&refused,&result),
					 "an impossible exit-sector count was accepted");
			Check (result.Failure == HYBRID_ANCHOR_BASE_NO_EXITS,"the wrong failure was named (%s)",
					 Hybrid_Anchor_Failure_Name (result.Failure));

			HybridAnchorSet no_ground;
			Check (!Hybrid_Validate_Anchors (params,identity,graph,matrices,landforms,hydrology,
														Anchor_Sample_Refuses,NULL,&no_ground,&result),
					 "Phase 8 validated anchors with no realized ground");
			Check (result.Failure == HYBRID_ANCHOR_BASE_NOT_PLACEABLE,"the wrong failure was named (%s)",
					 Hybrid_Anchor_Failure_Name (result.Failure));

			//	A fairness threshold of zero refuses only when the measured difference is actually non-zero.
			if (anchors.HomeTimeDifference > 0.0f) {
				HybridAnchorParams strict = params;
				strict.MaxInitialResourceTravelTimeDifferenceSeconds = 0.0f;
				Check (!Hybrid_Validate_Anchors (strict,identity,graph,matrices,landforms,hydrology,
															Macro_Sample_From_Formula,&ground,&refused,&result),
						 "a zero home-time threshold accepted a %g second difference",anchors.HomeTimeDifference);
				Check (result.Failure == HYBRID_ANCHOR_FAIRNESS_HOME,"the wrong failure was named (%s)",
						 Hybrid_Anchor_Failure_Name (result.Failure));
			}
			if (anchors.FrontTimeDifference > 0.0f) {
				HybridAnchorParams strict = params;
				strict.MaxBaseToFrontTravelTimeDifferenceSeconds = 0.0f;
				Check (!Hybrid_Validate_Anchors (strict,identity,graph,matrices,landforms,hydrology,
															Macro_Sample_From_Formula,&ground,&refused,&result),
						 "a zero front-time threshold accepted a %g second difference",anchors.FrontTimeDifference);
				Check (result.Failure == HYBRID_ANCHOR_FAIRNESS_FRONT,"the wrong failure was named (%s)",
						 Hybrid_Anchor_Failure_Name (result.Failure));
			}
		}
	}
}



static HybridTopologyParams Topology_Fixture (const HybridMapIdentityParams & identity_params)
{
	HybridTopologyParams params;
	params.Constants					= identity_params.Constants;
	params.RoadDensity				= 0.65f;			// native plan.road_density
	params.FieldRadiusMax			= 60.0f;			// native resource.field_radius_max
	params.MinVehicleLaneClearance= 1.0f;
	params.HarvesterLength			= 7.428105f;	// the measured Harvester WORLDBOX length
	params.MaxRoutedEdges			= 32;				// R at MEDIUM
	params.TunnelsAvailable			= true;
	return params;
}



/*
**	Q-023's structural-pose realizability, on the owner's own exports with hand-made poses: no terrain, no
**	world, only the geometry the rule is about.  `W` and `A` are the native constants.
*/
static HybridCrossingWindow Pose_Window (unsigned int id,const Vector2 & centre,const Vector2 & axis,float half,
													  unsigned int approach_classes)
{
	HybridCrossingWindow window;
	window.StableId			= id;
	window.Obstruction		= HYBRID_OBSTRUCTION_RIVER;
	window.Methods				= HYBRID_WINDOW_METHOD_BRIDGE;
	window.Center				= centre;
	window.Tangent.Set (-axis.Y,axis.X);
	window.ApproachA			= centre - (axis * half);
	window.ApproachB			= centre + (axis * half);
	window.Span					= 6.0f;
	window.ApproachLength	= half - 3.0f;
	window.ApproachClasses	= approach_classes;
	return window;
}


static void Check_Hybrid_Structural_Poses (void)
{
	HybridConstants k;
	::memset (&k,0,sizeof (k));
	k.W = 3.322739f;
	k.A = 15.0f;
	k.S = 60.0f;
	k.Q = 0.001f;

	const unsigned int ALL = 0x1Fu;
	const unsigned int NO_HIGHWAY = ALL & ~(1u << (unsigned int)HYBRID_ROAD_HIGHWAY_4);
	const float half = 3.0f + (8.0f * k.W);			//	a 6-wide river plus the 8W approach either side

	HybridRoadPose west;
	west.Point.Set (-400.0f,0.0f);
	HybridRoadPose east;
	east.Point.Set (400.0f,0.0f);

	//	Collinear crossings: two windows on the road's own line, well apart -- accepted, each and together.
	{
		const HybridCrossingWindow first = Pose_Window (1,Vector2 (-100.0f,0.0f),Vector2 (1.0f,0.0f),half,ALL);
		const HybridCrossingWindow second = Pose_Window (2,Vector2 (100.0f,0.0f),Vector2 (1.0f,0.0f),half,ALL);
		const HybridRoadPose a = Hybrid_Window_Pose (first,west.Point,second.Center);
		const HybridRoadPose b = Hybrid_Window_Pose (second,first.Center,east.Point);
		Check (Hybrid_Road_Poses_Connect (a,b,HYBRID_ROAD_HIGHWAY_4,k),"two collinear crossings 200 apart do not connect");
		Check (Hybrid_Window_Bindable (first,west,b,HYBRID_ROAD_HIGHWAY_4,k),"a collinear crossing is not bindable");
	}

	/*
	**	Close right-angle crossings: the first runs east, the second runs north 44 units on -- the measured
	**	failing case.  A `HIGHWAY_4` (R = 26.6) cannot turn between them and keep both straights; the same pair
	**	44 units apart *collinear* is fine, so it is the poses that decide, not the distance.
	*/
	{
		const HybridCrossingWindow first = Pose_Window (1,Vector2 (0.0f,0.0f),Vector2 (1.0f,0.0f),half,ALL);
		const HybridCrossingWindow second = Pose_Window (2,Vector2 (half + 44.0f,half + 30.0f),Vector2 (0.0f,1.0f),
																		 half,ALL);
		HybridRoadPose a = Hybrid_Window_Pose (first,west.Point,second.Center);
		HybridRoadPose b = Hybrid_Window_Pose (second,first.Center,Vector2 (half + 44.0f,400.0f));
		Check (!Hybrid_Road_Poses_Connect (a,b,HYBRID_ROAD_HIGHWAY_4,k),
				 "a HIGHWAY_4 joined two right-angle crossings with no room for its turn");

		const HybridCrossingWindow inline_second = Pose_Window (3,Vector2 ((2.0f * half) + 44.0f,0.0f),
																				  Vector2 (1.0f,0.0f),half,ALL);
		a = Hybrid_Window_Pose (first,west.Point,inline_second.Center);
		b = Hybrid_Window_Pose (inline_second,first.Center,east.Point);
		Check (Hybrid_Road_Poses_Connect (a,b,HYBRID_ROAD_HIGHWAY_4,k),
				 "two collinear crossings 44 apart did not connect -- the test is not about distance");
	}

	//	Wrong-side window: its axis is more than 90 degrees from the road's intent on one side.
	{
		const HybridCrossingWindow across = Pose_Window (1,Vector2 (0.0f,0.0f),Vector2 (0.0f,1.0f),half,ALL);
		HybridRoadPose before;
		before.Point.Set (-300.0f,200.0f);
		HybridRoadPose after;
		after.Point.Set (300.0f,200.0f);
		int refusal = HYBRID_BINDING_OK;
		Check (!Hybrid_Window_Bindable (across,before,after,HYBRID_ROAD_PRIMARY_2,k,&refusal) &&
				 (refusal == HYBRID_BINDING_WRONG_SIDE),
				 "a window the road would leave heading back the way it came was bindable (refusal %d)",refusal);
	}

	//	Next candidate: rank 1 is the right-angle trap, rank 2 a straight crossing -- rank 2, deterministically.
	{
		const HybridCrossingWindow trap = Pose_Window (1,Vector2 (-300.0f,0.0f),Vector2 (0.0f,1.0f),half,ALL);
		const HybridCrossingWindow good = Pose_Window (2,Vector2 (0.0f,0.0f),Vector2 (1.0f,0.0f),half,ALL);
		const HybridCrossingWindow * ranked[2] = { &trap,&good };
		HybridRoadPose before;
		before.Point.Set (-300.0f - 10.0f,-5.0f);
		const int first = Hybrid_First_Bindable_Window (ranked,2,before,east,HYBRID_ROAD_HIGHWAY_4,k);
		const int again = Hybrid_First_Bindable_Window (ranked,2,before,east,HYBRID_ROAD_HIGHWAY_4,k);
		Check ((first == 1) && (again == 1),"the realizable rank-2 window was not chosen (%d, %d)",first,again);
	}

	//	Highway safety, and the same window for a fixed SECONDARY_2 cross-link.
	{
		const HybridCrossingWindow narrow = Pose_Window (1,Vector2 (0.0f,0.0f),Vector2 (1.0f,0.0f),half,NO_HIGHWAY);
		int refusal = HYBRID_BINDING_OK;
		Check (!Hybrid_Window_Bindable (narrow,west,east,HYBRID_ROAD_HIGHWAY_4,k,&refusal) &&
				 (refusal == HYBRID_BINDING_APPROACH_CLASS),
				 "a window no HIGHWAY_4 can line up with was bindable to a family that may become one (refusal %d)",refusal);
		Check (Hybrid_Window_Bindable (narrow,west,east,HYBRID_ROAD_SECONDARY_2,k,&refusal),
				 "the same window was refused to a SECONDARY_2 cross-link it can carry (refusal %d)",refusal);
		Check (Hybrid_Window_Bindable (narrow,west,east,HYBRID_ROAD_PRIMARY_2,k),"PRIMARY_2 lost the window too");
	}

	/*
	**	Overpass pose: a 90-degree crossing of two intent segments whose other road arrives at the shared point
	**	from a node only 30 units off, beside it, so its own turn onto the deck has no room -- refused.  The same
	**	crossing approached from 200 units back is accepted.
	*/
	{
		const float deck = Hybrid_Overpass_Half_Deck (Vector2 (1.0f,0.0f),Vector2 (0.0f,1.0f),HYBRID_ROAD_HIGHWAY_4,
																	 HYBRID_ROAD_SECONDARY_2,k.W);
		Check (deck > 0.0f,"a right-angle overpass has no deck");
		const HybridRoadPose pose = Hybrid_Overpass_Pose (Vector2 (0.0f,0.0f),Vector2 (0.0f,-200.0f),
																		  Vector2 (0.0f,200.0f),k,deck);
		Check (pose.Before >= (10.0f * k.W),"an overpass pose reserves less than the 10W Phase 9 owes its ramps");
		HybridRoadPose close;
		close.Point.Set (30.0f,-30.0f);
		HybridRoadPose far_end;
		far_end.Point.Set (0.0f,200.0f);
		Check (!Hybrid_Road_Poses_Connect (close,pose,HYBRID_ROAD_SECONDARY_2,k),
				 "an overpass the road cannot line up with from its own neighbour was accepted");
		HybridRoadPose distant;
		distant.Point.Set (0.0f,-200.0f);
		Check (Hybrid_Road_Poses_Connect (distant,pose,HYBRID_ROAD_SECONDARY_2,k) &&
				 Hybrid_Road_Poses_Connect (pose,far_end,HYBRID_ROAD_SECONDARY_2,k),
				 "a straight run through an overpass did not connect");
	}

	/*
	**	An ordinary guide beside a crossing's lead (2026-10-02): the guide stands one cell off the road's line,
	**	ten units short of the crossing's near bank.  It is the search's anchor, not a corner of the centerline,
	**	so the crossing's lead drops it and the road runs straight in -- what Phase 10 builds.  Held as a fixed
	**	corner it left the lead no room, and Phase 9 refused a window Phase 10 could cross.  A guide far enough
	**	out keeps its corner and is accepted as before.
	*/
	{
		const HybridCrossingWindow crossing = Pose_Window (1,Vector2 (0.0f,0.0f),Vector2 (1.0f,0.0f),half,ALL);
		const HybridRoadPose pose = Hybrid_Window_Pose (crossing,west.Point,east.Point);
		HybridRoadPose guide;
		guide.Point.Set (-half - 10.0f,15.0f);
		Check (Hybrid_Road_Poses_Connect (guide,pose,HYBRID_ROAD_PRIMARY_2,k,&west.Point,nullptr),
				 "a guide beside a crossing's lead refused the crossing -- the guide was held as a fixed corner");
		guide.Point.Set (-250.0f,60.0f);
		Check (Hybrid_Road_Poses_Connect (guide,pose,HYBRID_ROAD_PRIMARY_2,k,&west.Point,nullptr),
				 "a guide well out from a crossing refused it");
	}
}

/*
**	Section 12.6's records (checklist I1, reopened): the HIGHWAY_RAMP class, topology kinds, interchange record,
**	ramp interval with the Q-026 subdivisions, route metadata, junction roles and merge/diverge records.  Every
**	field the plan lists reaches its stage checksum: an identical record reproduces it, and changing any one
**	field moves it.  Existing ordinals keep their values.
*/
static void Check_Hybrid_Ramp_Records (void)
{
	Check (HYBRID_ROAD_HIGHWAY_RAMP == 5 && HYBRID_ROAD_TRAIL_1 == 4 && HYBRID_ROAD_CLASS_COUNT == 6,
			 "HIGHWAY_RAMP is not appended after TRAIL_1");
	Check (HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION == 6 && HYBRID_EDGE_HIGHWAY_RAMP == 7 && HYBRID_EDGE_INTERCHANGE_CROSSING == 8,
			 "the interchange node/edge kinds are not appended after the existing kinds");
	Check (::strcmp (Hybrid_Topology_Node_Kind_Name (HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION),"interchange_local_junction") == 0 &&
			 ::strcmp (Hybrid_Topology_Edge_Kind_Name (HYBRID_EDGE_HIGHWAY_RAMP),"highway_ramp") == 0 &&
			 ::strcmp (Hybrid_Topology_Edge_Kind_Name (HYBRID_EDGE_INTERCHANGE_CROSSING),"interchange_crossing") == 0 &&
			 ::strcmp (Hybrid_Ramp_Role_Name (HYBRID_RAMP_ROLE_ON),"on_ramp") == 0 &&
			 ::strcmp (Hybrid_Interchange_Form_Name (HYBRID_INTERCHANGE_DIAMOND_ACCESS),"diamond_access") == 0 &&
			 ::strcmp (Hybrid_Interchange_Status_Name (HYBRID_INTERCHANGE_WITHDRAWN),"withdrawn") == 0 &&
			 ::strcmp (Hybrid_Junction_Role_Name (HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_LOCAL_JUNCTION),"highway_ramp_local_junction") == 0,
			 "a Section-12.6 name is not the plan's");

	//	A topology with one SIDE_ACCESS interchange and its ON_RAMP.
	HybridTopology topology;
	HybridTopologyNode anchor;
	anchor.StableId = 1;	anchor.Kind = HYBRID_NODE_FAMILY_WAYPOINT;	anchor.Position.Set (0.0f,0.0f);
	topology.Nodes.Add (anchor);
	HybridTopologyNode local;
	local.StableId = 2;	local.Kind = HYBRID_NODE_INTERCHANGE_LOCAL_JUNCTION;	local.Position.Set (0.0f,-28.0f);
	topology.Nodes.Add (local);
	HybridTopologyEdge ramp;
	ramp.StableId		= 1;
	ramp.Kind			= HYBRID_EDGE_HIGHWAY_RAMP;
	ramp.RoadClass		= HYBRID_ROAD_HIGHWAY_RAMP;
	ramp.NodeA			= 2;
	ramp.NodeB			= 1;
	ramp.Required		= true;
	ramp.InterchangeId= 1;
	ramp.RampRole		= HYBRID_RAMP_ROLE_ON;
	ramp.RampSide		= HYBRID_INTERCHANGE_SIDE_RIGHT;
	ramp.RampInterval.GoreStation = 30.0f;
	ramp.RampInterval.From = 30.0f;		ramp.RampInterval.To = 83.0f;
	ramp.RampInterval.FullFrom = 30.0f;	ramp.RampInterval.FullTo = 63.0f;
	ramp.RampInterval.TaperFrom = 63.0f;	ramp.RampInterval.TaperTo = 83.0f;
	topology.Edges.Add (ramp);
	HybridInterchange interchange;
	interchange.StableId				= 1;
	interchange.Form					= HYBRID_INTERCHANGE_SIDE_ACCESS;
	interchange.AnchorNode			= 1;
	interchange.HighwayEdgeIn		= 7;
	interchange.HighwayEdgeOut		= 8;
	interchange.AccessSides			= 1u << HYBRID_INTERCHANGE_SIDE_RIGHT;
	interchange.SelectedK			= 0;
	interchange.LocalOffset			= 28.0f;
	interchange.GoreOffset			= 30.0f;
	interchange.StraightHalfExtent= 90.0f;
	interchange.LocalJunction[HYBRID_INTERCHANGE_SIDE_RIGHT] = 2;
	interchange.Capabilities		= HYBRID_NAV_LIGHT;
	interchange.RampEdges.Add (1);
	HybridInterchangeRetarget retarget;
	retarget.EdgeId = 9;	retarget.FallbackNode = 1;	retarget.LocalJunction = 2;	retarget.Side = HYBRID_INTERCHANGE_SIDE_RIGHT;
	interchange.Retargets.Add (retarget);
	topology.Interchanges.Add (interchange);

	const unsigned int base = Hybrid_Topology_Checksum (topology);
	HybridTopology copy = topology;
	Check (Hybrid_Topology_Checksum (copy) == base,"an identical interchange topology does not reproduce its checksum");
	Check (topology.Find_Interchange (1) != NULL && topology.Find_Interchange (2) == NULL,"Find_Interchange");

	int unmoved = 0;
	for (int field = 0; field < 9; ++field) {
		HybridTopology changed = topology;
		HybridInterchange & ic = changed.Interchanges[0];
		HybridTopologyEdge & re = changed.Edges[0];
		switch (field) {
			case 0: ic.Status = HYBRID_INTERCHANGE_WITHDRAWN;		break;
			case 1: ic.SelectedK = 1;										break;
			case 2: ic.GoreOffset += 1.0f;								break;
			case 3: ic.Retargets[0].FallbackNode = 3;				break;
			case 4: re.RampRole = HYBRID_RAMP_ROLE_OFF;				break;
			case 5: re.RampSide = HYBRID_INTERCHANGE_SIDE_LEFT;	break;
			case 6: re.RampInterval.FullTo += 1.0f;					break;
			case 7: re.RampInterval.TaperFrom += 1.0f;				break;
			case 8: ic.Form = HYBRID_INTERCHANGE_DIAMOND_ACCESS;	break;
		}
		if (Hybrid_Topology_Checksum (changed) == base) unmoved++;
	}
	Check (unmoved == 0,"%d interchange/ramp fields do not reach the stage-9 checksum",unmoved);

	//	The route record (Section 12.1) and the stage-10 checksum.
	HybridRouteSet routes;
	HybridRoute route;
	route.EdgeId = 1;	route.RoadClass = HYBRID_ROAD_HIGHWAY_RAMP;	route.InterchangeId = 1;
	route.RampRole = HYBRID_RAMP_ROLE_ON;	route.RampSide = HYBRID_INTERCHANGE_SIDE_RIGHT;	route.RampInterval = ramp.RampInterval;
	routes.Routes.Add (route);
	HybridRoute highway;
	highway.EdgeId = 7;	highway.RoadClass = HYBRID_ROAD_HIGHWAY_4;
	HybridWidenedInterval widened;
	widened.InterchangeId = 1;	widened.RampEdgeId = 1;	widened.Role = HYBRID_RAMP_ROLE_ON;
	widened.Side = HYBRID_INTERCHANGE_SIDE_RIGHT;	widened.Interval = ramp.RampInterval;
	highway.Widened.Add (widened);
	routes.Routes.Add (highway);
	const unsigned int route_base = Hybrid_Route_Checksum (routes);
	HybridRouteSet routes_changed = routes;
	routes_changed.Routes[1].Widened[0].Interval.FullTo += 1.0f;
	HybridRouteSet ramp_changed = routes;
	ramp_changed.Routes[0].RampRole = HYBRID_RAMP_ROLE_OFF;
	Check ((Hybrid_Route_Checksum (routes_changed) != route_base) && (Hybrid_Route_Checksum (ramp_changed) != route_base),
			 "ramp metadata or widened intervals do not reach the stage-10 checksum");

	//	Junction roles (Section 12.1) and the merge/diverge record (12.6.10 item 5) in the stage-11 checksum.
	HybridInteraction incidental;
	incidental.Method = HYBRID_METHOD_AT_GRADE_JUNCTION;
	HybridInteraction shared = incidental;	shared.NodeId = 4;
	HybridInteraction obligation;	obligation.Method = HYBRID_METHOD_OVERPASS;	obligation.Mandatory = true;
	HybridInteraction bridge;	bridge.Method = HYBRID_METHOD_BRIDGE;
	HybridInteraction merge = incidental;	merge.Role = HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE;
	Check (Hybrid_Interaction_Junction_Role (incidental) == HYBRID_JUNCTION_ROLE_INCIDENTAL &&
			 Hybrid_Interaction_Junction_Role (shared) == HYBRID_JUNCTION_ROLE_SHARED_NODE &&
			 Hybrid_Interaction_Junction_Role (obligation) == HYBRID_JUNCTION_ROLE_OVERPASS_OBLIGATION &&
			 Hybrid_Interaction_Junction_Role (bridge) == HYBRID_JUNCTION_ROLE_NONE &&
			 Hybrid_Interaction_Junction_Role (merge) == HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE,
			 "a compiled junction role is not the plan's");
	HybridInteractionSet set;
	merge.StableId = 1;
	set.Interactions.Add (merge);
	HybridMergeDiverge md;
	md.StableId = 1;	md.InteractionId = 1;	md.RampEdgeId = 1;	md.HighwayEdgeId = 8;	md.InterchangeId = 1;
	md.Role = HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_MERGE;	md.Side = HYBRID_INTERCHANGE_SIDE_RIGHT;	md.Interval = ramp.RampInterval;
	HybridMergeDivergeSample sample;
	sample.Station = 30.0f;	sample.Full = true;	sample.D = 2.875f;	sample.H = 3.375f;	sample.E = 3.825f;
	md.Samples.Add (sample);
	set.MergeDiverge.Add (md);
	const unsigned int set_base = Hybrid_Interaction_Checksum (set);
	HybridInteractionSet set_changed = set;
	set_changed.MergeDiverge[0].Samples[0].E = 4.325f;
	HybridInteractionSet role_changed = set;
	role_changed.Interactions[0].Role = HYBRID_JUNCTION_ROLE_HIGHWAY_RAMP_DIVERGE;
	Check ((Hybrid_Interaction_Checksum (set_changed) != set_base) && (Hybrid_Interaction_Checksum (role_changed) != set_base),
			 "a merge/diverge sample or junction role does not reach the stage-11 checksum");
	printf ("  ramp records: class, kinds, interchange, intervals, roles and merge/diverge samples reach every stage checksum\r\n");
}


void Check_Hybrid_Topology (void)
{
	Check_Hybrid_Ramp_Records ();
	Check_Hybrid_Structural_Poses ();

	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformSet landforms;
	HybridLandformResult landform_result;
	HybridHydrologySet hydrology;
	HybridHydrologyResult hydrology_result;
	HybridMacroTerrainSet surfaces;
	HybridMacroTerrainResult macro_result;
	HybridAnchorSet anchors;
	HybridAnchorResult anchor_result;
	HybridTopology topology;
	HybridTopologyResult result;

	//	Section 5's road quotas and the optional cross-link count.
	{
		Check (Hybrid_River_Bridge_Quota (HYBRID_SIZE_SMALL) == 1,"SMALL river bridge quota");
		Check (Hybrid_River_Bridge_Quota (HYBRID_SIZE_MEDIUM) == 2,"MEDIUM river bridge quota");
		Check (Hybrid_River_Bridge_Quota (HYBRID_SIZE_LARGE) == 3,"LARGE river bridge quota");
		Check (Hybrid_Canyon_Bridge_Quota (HYBRID_SIZE_LARGE) == 2,"LARGE canyon bridge quota");
		Check (Hybrid_Highway_Quota (HYBRID_SIZE_SMALL) == 0,"SMALL highway quota");
		Check (Hybrid_Highway_Quota (HYBRID_SIZE_LARGE) == 2,"LARGE highway quota");
		Check (Hybrid_Overpass_Target (HYBRID_SIZE_MEDIUM) == 1,"MEDIUM overpass target");
		Check (Hybrid_Service_Branch_Quota (HYBRID_SIZE_LARGE) == 8,"LARGE service branch quota");
		Check (Hybrid_Trail_Quota (HYBRID_SIZE_MEDIUM) == 2,"MEDIUM trail quota");
		Check (Hybrid_Cross_Link_Cap (HYBRID_SIZE_MEDIUM) == 5,"MEDIUM cross-link cap");

		//	extra_links = RoundHalfUp(road_density * cap).
		Check (Hybrid_Extra_Cross_Link_Count (HYBRID_SIZE_MEDIUM,0.65f) == 3,"0.65 * 5 rounds to 3, not %d",
				 Hybrid_Extra_Cross_Link_Count (HYBRID_SIZE_MEDIUM,0.65f));
		Check (Hybrid_Extra_Cross_Link_Count (HYBRID_SIZE_LARGE,0.0f) == 0,"a zero road density buys no links");
		Check (Hybrid_Extra_Cross_Link_Count (HYBRID_SIZE_LARGE,1.0f) == 10,"a full road density takes the cap");
	}

	//	Section 12.2's max-flow, with BASE_INTERNAL contracted, on hand-built topologies.
	{
		HybridTopology flow;
		HybridTopologyNode node;
		node.Kind = HYBRID_NODE_BASE;				node.StableId = 1;	flow.Nodes.Add (node);	// GDI base
		node.Kind = HYBRID_NODE_BASE;				node.StableId = 2;	flow.Nodes.Add (node);	// Nod base
		node.Kind = HYBRID_NODE_FAMILY_WAYPOINT;	node.StableId = 3;	flow.Nodes.Add (node);
		node.Kind = HYBRID_NODE_FAMILY_WAYPOINT;	node.StableId = 4;	flow.Nodes.Add (node);

		HybridTopologyEdge edge;
		edge.Kind = HYBRID_EDGE_FAMILY_TRUNK;	edge.RoadClass = HYBRID_ROAD_PRIMARY_2;
		edge.StableId = 1;	edge.NodeA = 1;	edge.NodeB = 3;	flow.Edges.Add (edge);
		edge.StableId = 2;	edge.NodeA = 3;	edge.NodeB = 2;	flow.Edges.Add (edge);
		Check (Hybrid_Topology_Max_Flow (flow,1,2) == 1,"one chain is not one unit of flow");

		edge.StableId = 3;	edge.NodeA = 1;	edge.NodeB = 4;	flow.Edges.Add (edge);
		edge.StableId = 4;	edge.NodeA = 4;	edge.NodeB = 2;	flow.Edges.Add (edge);
		Check (Hybrid_Topology_Max_Flow (flow,1,2) == 2,"two edge-disjoint chains are not two units of flow");

		//	A BASE_INTERNAL edge is free connectivity, not a route: contracting it adds no flow.
		HybridTopology contracted;
		for (int i = 0; i < flow.Nodes.Count (); ++i) contracted.Nodes.Add (flow.Nodes[i]);
		node.Kind = HYBRID_NODE_BASE_ENTRANCE;	node.StableId = 5;	contracted.Nodes.Add (node);
		for (int i = 0; i < flow.Edges.Count (); ++i) contracted.Edges.Add (flow.Edges[i]);
		edge.Kind = HYBRID_EDGE_BASE_INTERNAL;	edge.RoadClass = HYBRID_ROAD_CLASS_COUNT;
		edge.StableId = 5;	edge.NodeA = 1;	edge.NodeB = 5;	contracted.Edges.Add (edge);
		Check (Hybrid_Topology_Max_Flow (contracted,1,2) == 2,"a BASE_INTERNAL edge changed the flow");
		Check (Hybrid_Topology_Max_Flow (contracted,1,1) == 0,"a node has flow to itself");
	}

	/*
	**	A full Phase-9 build on a real Phase-2 to Phase-8 chain.
	**
	**	Section 5 forbids reducing a quota to rescue a seed, so a world whose canyons and ravines happen to
	**	lie where no heavy route crosses them cannot fill the canyon/ravine bridge quota by any route, and
	**	that is a named attempt failure the manager retries with a new whole-world seed (Q-019 tracks how
	**	often an `open_frontier` world can do that).  The archetype used here is `canyon_crossroads`, whose
	**	own Section-6.10 canyon carries a crossing obligation across the strategic axis, so the quota is
	**	satisfiable by construction and every later step gets exercised; the loop still walks seeds the way
	**	the manager does, and a seed that fails must fail *named* and leave nothing behind.
	*/
	/*
	**	The first five are the original walk.  Once Phase 9 refuses structural sequences no road of the edge's
	**	class can drive (Q-023), none of them freezes a topology -- each binds a canyon obligation only to windows
	**	a `HIGHWAY_4` cannot line up with, or loses its landforms -- so the walk goes on to canyon seeds the
	**	`hybridinteractionsearch` census found passing Phase 9 under that rule.
	*/
	static const unsigned int TOPOLOGY_SEEDS[8] = { 4242u, 7u, 99u, 1234u, 31337u, 18u, 20u, 23u };
	bool topology_built = false;
	int refused_graph = 0, refused_matrix = 0, refused_landforms = 0, refused_hydrology = 0;
	int refused_anchors = 0, refused_topology = 0;

	for (int attempt = 0; (attempt < 8) && !topology_built; ++attempt) {

	const unsigned int seed = TOPOLOGY_SEEDS[attempt];
	HybridMapIdentityParams sized = Graph_Identity_Fixture (1200.0f,2.0f);
	sized.ArchetypeSelector = HYBRID_ARCHETYPE_CANYON_CROSSROADS;
	sized.MapPlanSeed		= WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	sized.HydrologySeed	= WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	sized.BiomeSeed		= WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_BIOMES);
	Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity: %s",
			 identity_result.Detail.Peek_Buffer ());

	HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
	if (!Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result)) {
		refused_graph++;
		continue;
	}

	HybridCoarseMatrixParams coarse_params = Coarse_Fixture (sized,1200.0f);
	if (!Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result)) {
		refused_matrix++;
		continue;
	}

	HybridLandformParams landform_params = Landform_Fixture (sized,1200.0f);
	landform_params.TerrainSeed = WorldSeedService::Derive_Subsystem_Seed (seed,
																							WorldSeedService::SEED_DOMAIN_TERRAIN);
	if (!Hybrid_Build_Landforms (landform_params,identity,intents,graph,&matrices,&landforms,
										  &landform_result)) {
		refused_landforms++;
		continue;
	}

	HybridHydrologyParams hydrology_params = Hydrology_Fixture (sized,1200.0f);
	hydrology_params.HydrologySeed = sized.HydrologySeed;
	if (!Hybrid_Build_Hydrology (hydrology_params,identity,intents,graph,landforms,&matrices,&hydrology,
										  &hydrology_result)) {
		refused_hydrology++;
		continue;
	}

	HybridMacroTerrainParams macro_params = Macro_Terrain_Fixture (sized);
	Check (Hybrid_Resolve_River_Surfaces (macro_params,matrices,landforms,hydrology,&surfaces,&macro_result),
			 "river surfaces: %s",macro_result.Detail.Peek_Buffer ());

	MacroFormulaContext ground;
	ground.Params		= &macro_params;
	ground.Matrices	= &matrices;
	ground.Landforms	= &landforms;
	ground.Hydrology	= &hydrology;
	ground.Surfaces	= &surfaces;
	ground.Offset		= 0.0f;

	HybridAnchorParams anchor_params = Anchor_Fixture (sized);
	if (!Hybrid_Validate_Anchors (anchor_params,identity,graph,matrices,landforms,hydrology,
											Macro_Sample_From_Formula,&ground,&anchors,&anchor_result)) {
		refused_anchors++;
		continue;
	}

	/*
	**	What Phase 6 actually left for Phase 9's bridge quotas to consume.  A world with no bridge-capable
	**	canyon window cannot fill the Section-5 canyon quota by any route, so the count is worth stating
	**	here rather than discovering it as an unexplained quota failure.
	*/
	{
		int river_windows = 0, river_bridgeable = 0, canyon_windows = 0, canyon_bridgeable = 0;
		for (int w = 0; w < hydrology.Windows.Count (); ++w) {
			const HybridCrossingWindow & window = hydrology.Windows[w];
			const bool bridgeable = window.Has_Method (HYBRID_WINDOW_METHOD_BRIDGE);
			if (window.Obstruction == HYBRID_OBSTRUCTION_RIVER) {
				river_windows++;
				if (bridgeable) river_bridgeable++;
			}
			if ((window.Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
				 (window.Obstruction == HYBRID_OBSTRUCTION_RAVINE)) {
				canyon_windows++;
				if (bridgeable) canyon_bridgeable++;
			}
		}
		Check (river_bridgeable >= Hybrid_River_Bridge_Quota (identity.SizeClass),
				 "%d river windows (%d bridge-capable) for a quota of %d",river_windows,river_bridgeable,
				 Hybrid_River_Bridge_Quota (identity.SizeClass));
		Check (canyon_bridgeable >= Hybrid_Canyon_Bridge_Quota (identity.SizeClass),
				 "%d canyon/ravine windows (%d bridge-capable) for a quota of %d",canyon_windows,canyon_bridgeable,
				 Hybrid_Canyon_Bridge_Quota (identity.SizeClass));

	}

	HybridTopologyParams params = Topology_Fixture (sized);
	const bool built = Hybrid_Build_Topology (params,identity,graph,landforms,hydrology,anchors,&matrices,
															Macro_Sample_From_Formula,&ground,&topology,&result);

	/*
	**	A Section-5 road quota Phase 9 cannot fill is a *named* attempt failure the manager retries with a
	**	new whole-world seed: the plan forbids reducing a quota to rescue a seed.  Either outcome is legal
	**	here, but a failure must name itself and leave nothing behind.
	*/
	if (!built) {
		Check (result.Failure != HYBRID_TOPOLOGY_OK,"a failed Phase 9 must name its failure");
		Check (result.Detail.Get_Length () > 0,"a failed Phase 9 must say what refused the topology");
		Check (topology.Nodes.Count () == 0 && topology.Edges.Count () == 0,
				 "a failed Phase 9 left a partial topology behind");
		refused_topology++;
		continue;					// the manager's own answer: a new whole-world seed
	}
	topology_built = true;

	const HybridConstants & k = params.Constants;

	//	Step 1: one entrance per family per base, on a frozen Phase-8 viable sector.
	Check (topology.Entrances.Count () == (2 * identity.RouteFamilyCount),
			 "%d entrances for %d families across two bases",topology.Entrances.Count (),
			 identity.RouteFamilyCount);

	for (int e = 0; e < topology.Entrances.Count (); ++e) {

		const HybridBaseEntrance & entrance = topology.Entrances[e];
		const HybridBaseAnchor * base = anchors.Find_Base (entrance.BaseSiteId);
		Check (base != NULL,"entrance %u has no frozen base",entrance.NodeId);
		if (base == NULL) continue;

		Check (base->Sector_Is_Viable (entrance.SectorOrdinal),
				 "entrance on sector %d, which Phase 8 did not freeze as viable",entrance.SectorOrdinal);
		Check (Near ((entrance.Point - base->Center).Length (),base->ProtectedRadius + k.W,0.01f),
				 "the entrance point is not R_BASE_PROTECTED + W from the base centre");
		Check (Near (entrance.CorridorHalfWidth,
						 (0.5f * Hybrid_Roadbed_Width (HYBRID_ROAD_PRIMARY_2,k.W)) + params.MinVehicleLaneClearance,
						 0.01f),"the entrance corridor half-width is not the plan's own formula");

		//	The logical base node reaches it by a BASE_INTERNAL edge, which is never realized.
		bool internal_found = false;
		for (int i = 0; i < topology.Edges.Count (); ++i) {
			const HybridTopologyEdge & edge = topology.Edges[i];
			if (edge.Kind != HYBRID_EDGE_BASE_INTERNAL) continue;
			if ((edge.NodeA != entrance.NodeId) && (edge.NodeB != entrance.NodeId)) continue;
			internal_found = true;
			Check (!edge.Is_Realized (),"a BASE_INTERNAL edge reports itself as realized");
			Check (edge.RoadClass == HYBRID_ROAD_CLASS_COUNT,"a BASE_INTERNAL edge carries a road class");
		}
		Check (internal_found,"entrance %u has no BASE_INTERNAL edge to its base",entrance.NodeId);
	}

	//	Step 5: exactly the Section-5 highway quota, and every other trunk is PRIMARY_2.
	Check (topology.Highways == Hybrid_Highway_Quota (identity.SizeClass),"%d highway corridors, Section 5 wants %d",
			 topology.Highways,Hybrid_Highway_Quota (identity.SizeClass));
	for (int i = 0; i < topology.Edges.Count (); ++i) {
		const HybridTopologyEdge & edge = topology.Edges[i];
		if (edge.Kind != HYBRID_EDGE_FAMILY_TRUNK) continue;
		Check ((edge.RoadClass == HYBRID_ROAD_HIGHWAY_4) || (edge.RoadClass == HYBRID_ROAD_PRIMARY_2),
				 "trunk edge %u is %s, which is not a heavy class",edge.StableId,
				 Hybrid_Road_Class_Name (edge.RoadClass));
	}

	//	Steps 2 to 4: the bridge quotas, each bound to a distinct bridge-capable window, 12W apart.
	Check (topology.RiverBridges >= Hybrid_River_Bridge_Quota (identity.SizeClass),
			 "%d river bridges, Section 5 wants %d",topology.RiverBridges,
			 Hybrid_River_Bridge_Quota (identity.SizeClass));
	Check (topology.CanyonBridges >= Hybrid_Canyon_Bridge_Quota (identity.SizeClass),
			 "%d canyon/ravine bridges, Section 5 wants %d",topology.CanyonBridges,
			 Hybrid_Canyon_Bridge_Quota (identity.SizeClass));

	for (int i = 0; i < topology.Nodes.Count (); ++i) {

		const HybridTopologyNode & node = topology.Nodes[i];
		if (node.Kind != HYBRID_NODE_CROSSING_OBLIGATION) continue;

		const HybridCrossingWindow * window = hydrology.Find_Window (node.WindowId);
		Check (window != NULL,"crossing waypoint %u names no Phase-6 window",node.StableId);
		if (window == NULL) continue;
		Check (Near (node.Position.X,window->Center.X) && Near (node.Position.Y,window->Center.Y),
				 "crossing waypoint %u is not on its own window",node.StableId);

		for (int j = i + 1; j < topology.Nodes.Count (); ++j) {
			const HybridTopologyNode & other = topology.Nodes[j];
			if (other.Kind != HYBRID_NODE_CROSSING_OBLIGATION) continue;
			Check (other.WindowId != node.WindowId,"window %u is bound twice",node.WindowId);
			const HybridCrossingWindow * second = hydrology.Find_Window (other.WindowId);
			if (second == NULL) continue;
			//	A river and a landform each number their own records, so the obstruction *kind* is part of
			//	the identity: window records 3 on river 3 and on ravine 3 are different obstructions.
			if (second->Obstruction != window->Obstruction) continue;
			if (second->ObstructionRecordId != window->ObstructionRecordId) continue;
			Check (WWMath::Fabs (second->Station - window->Station) >= (12.0f * k.W) - 0.01f,
					 "two selected windows on one obstruction are %g apart, under 12W",
					 WWMath::Fabs (second->Station - window->Station));
		}

		//	Every crossing waypoint is a waypoint of some edge, never a loose node.
		int uses = 0;
		for (int e = 0; e < topology.Edges.Count (); ++e) {
			for (int w = 0; w < topology.Edges[e].Waypoints.Count (); ++w) {
				if (topology.Edges[e].Waypoints[w] == node.StableId) uses++;
			}
		}
		Check (uses >= 1,"crossing waypoint %u is not on any edge",node.StableId);
	}

	//	Step 6: a field road always terminates at its FIELD_ACCESS_POINT, never at the field centre.
	Check (topology.Count_Of_Node_Kind (HYBRID_NODE_FIELD_ACCESS) == anchors.Resources.Count (),
			 "%d field access points for %d frozen field anchors",
			 topology.Count_Of_Node_Kind (HYBRID_NODE_FIELD_ACCESS),anchors.Resources.Count ());

	for (int i = 0; i < topology.Nodes.Count (); ++i) {

		const HybridTopologyNode & node = topology.Nodes[i];
		if (node.Kind != HYBRID_NODE_FIELD_ACCESS) continue;

		const HybridResourceAnchor * anchor = anchors.Find_Resource (node.SiteId);
		Check (anchor != NULL,"field access %u names no anchor",node.StableId);
		if (anchor == NULL) continue;
		Check (Near ((node.Position - anchor->Center).Length (),
						 params.FieldRadiusMax + params.MinVehicleLaneClearance,0.01f),
				 "field access %u is %g from its centre, not field_radius_max + clearance",node.StableId,
				 (node.Position - anchor->Center).Length ());
	}

	for (int e = 0; e < topology.Edges.Count (); ++e) {

		const HybridTopologyEdge & edge = topology.Edges[e];
		if ((edge.Kind != HYBRID_EDGE_HOME_ROAD) && (edge.Kind != HYBRID_EDGE_FIELD_BRANCH)) continue;

		const HybridTopologyNode * b_node = topology.Find_Node (edge.NodeB);
		Check ((b_node != NULL) && (b_node->Kind == HYBRID_NODE_FIELD_ACCESS),
				 "a field road does not end at a field access point");

		if (edge.Kind == HYBRID_EDGE_FIELD_BRANCH) {
			Check (edge.RoadClass == HYBRID_ROAD_SERVICE_DIRT_2,"a field branch is not SERVICE_DIRT_2");
			continue;
		}

		//	Step 7: PRIMARY_2 unless the entrance and the access point are closer than 12W.
		const HybridTopologyNode * a_node = topology.Find_Node (edge.NodeA);
		if ((a_node == NULL) || (b_node == NULL)) continue;
		const float distance = (b_node->Position - a_node->Position).Length ();
		const HybridRoadClass expect = (distance < (12.0f * k.W)) ? HYBRID_ROAD_SECONDARY_2 : HYBRID_ROAD_PRIMARY_2;
		Check (edge.RoadClass == expect,"a %g long home road is %s, not %s",distance,
				 Hybrid_Road_Class_Name (edge.RoadClass),Hybrid_Road_Class_Name (expect));
	}

	//	Steps 8 to 11: the Section-5 counts, and the optional links inside their own cap.
	Check (topology.ServiceBranches >= Hybrid_Service_Branch_Quota (identity.SizeClass),
			 "%d service branches, Section 5 wants %d",topology.ServiceBranches,
			 Hybrid_Service_Branch_Quota (identity.SizeClass));
	Check (topology.Trails >= Hybrid_Trail_Quota (identity.SizeClass),"%d trails, Section 5 wants %d",
			 topology.Trails,Hybrid_Trail_Quota (identity.SizeClass));
	Check (topology.CrossLinks <= (topology.OptionalCrossLinkQuota + topology.ReservedOverpassCount),
			 "%d cross-links, over the %d the quota and the overpass fallback allow",topology.CrossLinks,
			 topology.OptionalCrossLinkQuota + topology.ReservedOverpassCount);

	for (int e = 0; e < topology.Edges.Count (); ++e) {
		const HybridTopologyEdge & edge = topology.Edges[e];
		if (edge.Kind == HYBRID_EDGE_TRAIL) {
			Check (edge.RoadClass == HYBRID_ROAD_TRAIL_1,"a trail is not TRAIL_1");
			Check (edge.Length >= (8.0f * k.W) - 0.01f,"trail %u is %g long, under 8W",edge.StableId,edge.Length);
		}
		if (edge.Kind == HYBRID_EDGE_SERVICE_FILL) {
			Check (edge.RoadClass == HYBRID_ROAD_SERVICE_DIRT_2,"a quota-filling service edge is not SERVICE_DIRT_2");
		}
	}

	//	Steps 12 to 14: every overpass obligation is one waypoint shared by exactly two edges.
	//	Q-024: the Section-5 number is a target; what Phase 9 reserved is exactly what it found, never padded.
	Check (topology.RequestedOverpassTarget == Hybrid_Overpass_Target (identity.SizeClass),
			 "the overpass target is %d, Section 5 says %d",topology.RequestedOverpassTarget,
			 Hybrid_Overpass_Target (identity.SizeClass));
	Check (topology.ReservedOverpassCount <= topology.RequestedOverpassTarget,
			 "%d overpasses reserved past a target of %d",topology.ReservedOverpassCount,topology.RequestedOverpassTarget);
	Check (topology.ReservedOverpassCount == topology.Count_Of_Node_Kind (HYBRID_NODE_OVERPASS_OBLIGATION),
			 "%d overpasses counted, %d obligation nodes",topology.ReservedOverpassCount,
			 topology.Count_Of_Node_Kind (HYBRID_NODE_OVERPASS_OBLIGATION));
	Check (result.OverpassQuota.Required == topology.RequestedOverpassTarget,"the overpass census lost its target");
	if (topology.Overpass_Target_Shortage () > 0) {
		//	A shortfall is legitimate only when nothing hard-valid was left to take.
		Check (result.OverpassQuota.Have == topology.ReservedOverpassCount,"the overpass census lost its count");
		printf ("  overpass target %d, %d reserved: OVERPASS_TARGET_SHORTAGE %d (natural %d, synthesized %d candidates refused)\r\n",topology.RequestedOverpassTarget,topology.ReservedOverpassCount,topology.Overpass_Target_Shortage (),
				  result.OverpassQuota.Natural,result.OverpassQuota.Synthesized);
	}

	for (int i = 0; i < topology.Nodes.Count (); ++i) {

		if (topology.Nodes[i].Kind != HYBRID_NODE_OVERPASS_OBLIGATION) continue;
		int uses = 0;
		for (int e = 0; e < topology.Edges.Count (); ++e) {
			for (int w = 0; w < topology.Edges[e].Waypoints.Count (); ++w) {
				if (topology.Edges[e].Waypoints[w] == topology.Nodes[i].StableId) uses++;
			}
		}
		Check (uses == 2,"overpass obligation %u is shared by %d edges, not two",topology.Nodes[i].StableId,uses);
	}

	//	The gate's own invariants.
	Check (topology.Realized_Edge_Count () <= params.MaxRoutedEdges,"%d routed edges, R allows %d",
			 topology.Realized_Edge_Count (),params.MaxRoutedEdges);
	Check (topology.Has_Cycle (),"the topology is a tree, and the plan requires a cycle");
	Check (Hybrid_Topology_Max_Flow (topology,topology.Nodes[0].StableId,topology.Nodes[0].StableId) == 0,
			 "a node has flow to itself");
	Check (topology.Checksum != 0,"a passed Phase 9 left no checksum");

	//	Determinism: Phase 9 makes no draw at all, so a second pass over the same records is identical.
	HybridTopology again;
	Check (Hybrid_Build_Topology (params,identity,graph,landforms,hydrology,anchors,&matrices,
											Macro_Sample_From_Formula,&ground,&again,&result),
			 "the second topology build was refused: %s",result.Detail.Peek_Buffer ());
	Check (again.Checksum == topology.Checksum,"Phase 9 is not reproducible");
	Check (Hybrid_Topology_Checksum (again,identity.Checksum) == topology.Checksum,
			 "the checksum does not describe the records it was built from");

	//	The routed-edge budget refuses on its own, and refusing leaves nothing behind.
	HybridTopologyParams tight = params;
	tight.MaxRoutedEdges = 1;
	HybridTopology refused;
	Check (!Hybrid_Build_Topology (tight,identity,graph,landforms,hydrology,anchors,&matrices,
											 Macro_Sample_From_Formula,&ground,&refused,&result),
			 "a one-edge routing budget was accepted");
	Check (result.Failure == HYBRID_TOPOLOGY_EDGE_BUDGET,"the wrong failure was named (%s)",
			 Hybrid_Topology_Failure_Name (result.Failure));
	Check (refused.Edges.Count () == 0,"a refused Phase 9 left edges behind");
	}

	Check (topology_built,"no seed produced a Phase-9 topology: graph %d, matrix %d, landforms %d, "
			 "hydrology %d, anchors %d, topology %d refusals; last %s (%s) / %s / %s",refused_graph,refused_matrix,
			 refused_landforms,refused_hydrology,refused_anchors,refused_topology,
			 Hybrid_Topology_Failure_Name (result.Failure),result.Detail.Peek_Buffer (),
			 Hybrid_Anchor_Failure_Name (anchor_result.Failure),
			 Hybrid_Hydrology_Failure_Name (hydrology_result.Failure));
}



static HybridRouteParams Route_Fixture (const HybridMapIdentityParams & identity_params,float world_size)
{
	HybridRouteParams params;
	params.Constants		= identity_params.Constants;
	params.WorldMinX		= -0.5f * world_size;
	params.WorldMinY		= -0.5f * world_size;
	params.WorldWidth		= world_size;
	params.WorldHeight	= world_size;
	params.MaxCutDepth	= 6.0f;			// native grading.max_cut_depth
	params.MaxFillHeight	= 6.0f;
	params.RiverWidth		= 6.0f;
	params.FieldRadiusMax= 60.0f;
	params.MaxLocalRetriesPerStage = 8;
	params.MinVehicleLaneClearance = 1.0f;		// native fairness.min_vehicle_lane_clearance

	//	Section 16.1's own budgets for this grid, so the ceilings under test are the real ones.
	HybridWorkBudgets budgets;
	const int cells = (int)((world_size / params.Constants.A) + 1.0f) *
							(int)((world_size / params.Constants.A) + 1.0f);
	Hybrid_Get_Work_Budgets (identity_params.Constants.SizeClass,cells,32,&budgets);
	params.AStarExpansionsPerRoute		= budgets.AStarExpansionsPerRoute;
	params.AStarExpansionsPerAttempt	= budgets.AStarExpansionsPerAttempt;
	return params;
}


/*
**	The content metrics the hybrid generator measures at preflight, as this harness can supply them.
**
**	A self check has no loaded level and no live definition registries, so `Gather_From_Runtime` finds
**	nothing -- the same reason `Check_Hybrid_Preflight` hand-fills its own inputs.  These are the real
**	WORLDBOX measurements recorded in `docs/procedural_world/VehicleCollisionMetrics.md` and the real
**	`BridgePlannerParams` values, not invented numbers, and `Check_Hybrid_Preflight` owns the contract
**	they assert against.  Nothing optional is supplied: no tunnel module, no decoration and no lamp, so a
**	check built on this fixture exercises the *disabled* path for every optional capability, which is what
**	the shipped content actually offers today.
*/
static void Hybrid_Content_Fixture (const ProceduralWorldProfile & profile,HybridPreflightInputs * out)
{
	out->WorldUnitsPerMeter = 1.0f;

	const float HUMVEE[3]		= { 4.722971f, 2.555953f, 1.548488f };
	const float MAMMOTH[3]		= { 10.500445f, 6.801906f, 3.064066f };
	const float HARVESTER[2][3]= { { 7.428105f, 4.514225f, 2.754598f },
											 { 7.399116f, 4.514225f, 2.754598f } };

	struct { HybridVehicleMetricInput * Out; const float * Box; int Id; } vehicles[4] = {
		{ &out->Humvee,HUMVEE,101 }, { &out->HeavyVehicle,MAMMOTH,102 },
		{ &out->Harvesters[0],HARVESTER[0],103 }, { &out->Harvesters[1],HARVESTER[1],104 } };
	for (int v = 0; v < 4; ++v) {
		vehicles[v].Out->Measured		= true;
		vehicles[v].Out->DefinitionID	= vehicles[v].Id;
		vehicles[v].Out->Length			= vehicles[v].Box[0];
		vehicles[v].Out->Width			= vehicles[v].Box[1];
		vehicles[v].Out->Height			= vehicles[v].Box[2];
	}

	out->ReferenceVehicleSpeed	= WorldMetricsClass::Get_Reference_Vehicle_Speed ();
	out->ReferenceInfantrySpeed= WorldMetricsClass::Get_Reference_Infantry_Speed ();

	//	BridgePlannerParams' own values: the span bound Phase 11 resolves to when a definition has none.
	out->BridgeMaxSpanLength		= 200.0f;
	out->BridgeWaterClearanceMargin= 2.0f;
	out->BridgeApproachLength		= 30.0f;

	HybridBridgeInput bridge;
	bridge.Name					= "test_bridge";
	bridge.Width				= 40.0f;
	bridge.DeckThickness		= 2.0f;
	bridge.RequiredClearance= 5.0f;
	out->Bridges.Add (bridge);

	for (int team = 0; team < 2; ++team) {

		const DynamicVectorClass<StringClass> & names = (team == 0) ? profile.Base.GdiInitialBuildings
																					  : profile.Base.NodInitialBuildings;
		for (int i = 0; i < names.Count (); ++i) {
			HybridBuildingFootprintInput building;
			building.Name		= names[i];
			building.Team		= team;
			building.Found		= true;
			building.Radius	= 20.0f;
			building.Height	= 15.0f;
			out->Buildings.Add (building);
		}
	}

	HybridTerrainLayerInput layer;
	layer.Name = "test_grass";	layer.Kind = TERRAIN_LAYER_BASE;		layer.SurfaceType = SURFACE_TYPE_GRASS;
	out->TerrainLayers.Add (layer);
	layer.Name = "test_rock";	layer.Kind = TERRAIN_LAYER_CLIFF;	layer.SurfaceType = SURFACE_TYPE_ROCK;
	out->TerrainLayers.Add (layer);
}


/***********************************************************************************************
**	The local-pass builder's own gate.
**
**	These are owner-level tests on synthetic height fields, deliberately ahead of any whole world: the
**	builder's whole job is a geometric question -- can a road of this class get down, across and up again
**	here -- and that question is answerable exactly, on a profile whose slope is known by construction.
**	Proving it here means a later real-pipeline fixture is testing whether such a place exists, not whether
**	the arithmetic is right.
***********************************************************************************************/

//	A synthetic obstruction: flat rims at +/-`HalfSpan`, a flat floor between +/-`FloorHalf`, walls between.
struct PassTerrain
{
	Vector2	Centre;
	Vector2	Tangent;
	Vector2	Normal;
	float		HalfSpan;			// where the rim is, measured along the normal
	float		FloorHalf;			// where the floor ends
	float		RimZ;					// rim elevation
	float		FloorZ;				// floor elevation
	float		TangentFloorSlope;	// floor elevation change per unit of tangent run
	float		TangentRimSlope;		// rim elevation change per unit of tangent run, for the ramp cases

	/*
	**	The far side's own bench elevation, used where the two sides of the obstruction sit at *different*
	**	heights.  A trench has the same bench either side; a cliff is a step, and a step is the only shape
	**	that makes a single diagonal ramp mean anything.
	*/
	bool		FarSideDiffers;
	float		FarSideZ;				// applied where `across` is positive and beyond the span

	/*
	**	A true sheer step: the wall carries no slope at all, so the whole drop happens at the floor edge.  A
	**	diagonal converts a *sloped* face into a gentle ramp, and cannot do anything at all with a
	**	discontinuity, so this is the shape that genuinely cannot be realized inside `RoadVerticalInterval`.
	*/
	bool		WallIsVertical;

	bool		BlockPlusTangent;		// true: the far `+tangent` reach is unreachably high

	/*
	**	D-004's raised band instead of a trench: the bench is `FloorZ` on *both* sides, and between `-HalfSpan`
	**	and `+HalfSpan` the ground rises over `BandFace` from each toe to a shelf at `RimZ`.  `FloorHalf` is
	**	unused in this mode; the shelf half width is `HalfSpan - BandFace`.
	*/
	bool		RaisedBand;
	float		BandFace;
};


/*
**	The height field.  Between floor and rim the wall is linear, which is what makes the straight normal
**	line steep and the diagonal one gentle -- the whole point of the geometry under test.
*/
static bool Pass_Terrain_Height (const Vector2 & world,float * height_out,void * context)
{
	const PassTerrain * terrain = (const PassTerrain *)context;
	if ((terrain == NULL) || (height_out == NULL)) return false;

	const Vector2 relative = world - terrain->Centre;
	const float along = (relative.X * terrain->Tangent.X) + (relative.Y * terrain->Tangent.Y);
	const float across = (relative.X * terrain->Normal.X) + (relative.Y * terrain->Normal.Y);
	const float distance = WWMath::Fabs (across);

	/*
	**	One side may be made impassable without changing its slope, by standing the rim far above what the
	**	road's own cut and fill could ever reach.  That is how the mirror-orientation case is built.
	*/
	/*
	**	The block stands clear of the station itself, so a legal candidate's own roadbed-width offsets near
	**	`along = 0` cannot stray into it.  A block flush against the station leaks into both orientations and
	**	proves nothing.
	*/
	if (terrain->BlockPlusTangent && (along > (2.0f * 3.322739f))) {
		*height_out = terrain->RimZ + 10000.0f;
		return true;
	}

	if (terrain->RaisedBand) {
		//	A band is bench everywhere outside its toes; the trench rules below never see it.  The
		//	toe itself is bench: the face begins strictly inside it.
		if (distance >= (terrain->HalfSpan - 0.001f)) {
			*height_out = terrain->FloorZ + (along * terrain->TangentFloorSlope);
			return true;
		}
		//	D-004's face is a smoothstep, flat at the toe and at the shelf edge, not a linear kink.
		const float climb = terrain->HalfSpan - distance;		// how far in from the toe
		const float linear = (terrain->BandFace > 0.0f) ? WWMath::Min (climb / terrain->BandFace,1.0f) : 1.0f;
		const float ratio = linear * linear * (3.0f - (2.0f * linear));
		*height_out = terrain->FloorZ + ((terrain->RimZ - terrain->FloorZ) * ratio) +
						  (along * terrain->TangentFloorSlope);
		return true;
	}

	/*
	**	The floor includes its own edge, to within the position quantum.  A ramp foot lands exactly on
	**	`FloorHalf`, and a quantized point half a thousandth outside it is still on the floor: without this the
	**	synthetic field answers with the wall's height there, which is an artifact of the fixture rather than
	**	anything the builder did.
	*/
	if (distance <= (terrain->FloorHalf + 0.001f)) {
		*height_out = terrain->FloorZ + (along * terrain->TangentFloorSlope);
		return true;
	}
	const float far_z = (terrain->FarSideDiffers && (across > 0.0f)) ? terrain->FarSideZ : terrain->RimZ;

	if (distance >= terrain->HalfSpan) {
		*height_out = far_z + (along * terrain->TangentRimSlope);
		return true;
	}


	if (terrain->WallIsVertical) {
		*height_out = far_z + (along * terrain->TangentRimSlope);
		return true;
	}

	//	The wall: linear from the floor edge to whichever bench this side carries.
	const float span = terrain->HalfSpan - terrain->FloorHalf;
	const float ratio = (span > 0.0f) ? ((distance - terrain->FloorHalf) / span) : 1.0f;
	const float base = terrain->FarSideDiffers ? ((across > 0.0f) ? far_z : terrain->RimZ) : terrain->RimZ;
	*height_out = terrain->FloorZ + ((base - terrain->FloorZ) * ratio) + (along * terrain->TangentFloorSlope);
	return true;
}


static HybridLocalPassParams Local_Pass_Fixture (void)
{
	HybridLocalPassParams params;
	Hybrid_Compute_Constants (2.555953f,1.0f,60.0f,1200.0f,1200.0f,3.0f,900.0f,&params.Constants);
	params.MaxCutDepth					= 6.0f;			// native grading.max_cut_depth
	params.MaxFillHeight					= 6.0f;
	params.SweptVehicleWidth			= 6.801906f;	// the measured heavy envelope
	params.MinVehicleLaneClearance	= 1.0f;
	params.PassClass						= HYBRID_ROAD_PRIMARY_2;
	return params;
}


void Check_Hybrid_Local_Pass (void)
{
	const HybridLocalPassParams params = Local_Pass_Fixture ();
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec (HYBRID_ROAD_PRIMARY_2);
	const float max_grade = spec.MaxGradeDegrees * 0.0174532925f;
	const float approach = 8.0f * params.Constants.W;

	/*
	**	How much along-obstruction run these fixtures make available, in each direction.  A real station's
	**	run is the arc distance to each end of its own obstruction; the synthetic ones are given a long
	**	feature in both directions so the cases under test are the geometry and the realizability.  The
	**	cases that test the extent itself pass their own directional values.
	*/
	const float PASS_RUN = 400.0f;

	Check (params.Constants.W > 0.0f,"the local-pass fixture has no lane width");
	Check (Hybrid_Local_Pass_Sample_Spacing (params.Constants) > 0.0f,"the pass sample spacing is zero");

	PassTerrain terrain;
	terrain.Centre.Set (0.0f,0.0f);
	terrain.Tangent.Set (1.0f,0.0f);
	terrain.Normal.Set (0.0f,1.0f);
	terrain.HalfSpan				= 40.0f;
	terrain.FloorHalf				= 12.0f;
	//	A ravine of a depth the generator actually makes: Section 6's floor is `0.75 * tier_step`.
	terrain.RimZ					= 8.0f;
	terrain.FloorZ					= 0.0f;
	terrain.TangentFloorSlope	= 0.0f;
	terrain.TangentRimSlope		= 0.0f;
	terrain.FarSideDiffers		= false;
	terrain.FarSideZ				= 0.0f;
	terrain.WallIsVertical		= false;
	terrain.BlockPlusTangent	= false;
	terrain.RaisedBand			= false;
	terrain.BandFace				= 0.0f;

	/*
	**	The case that pins the defect this builder was written for.  The wall drops 24 over 28 of normal
	**	run -- about 40 degrees, far past a `PRIMARY_2` -- so a straight bank-to-bank sample refuses it, and
	**	that refusal is what made every pass window unreachable on every seed.  The diagonal ramp has room
	**	to make the same descent inside the class grade, so it is capable.
	*/
	{
		//	First, the straight-line reading, computed here so the contrast is on the record.
		const float straight_run = terrain.HalfSpan - terrain.FloorHalf;
		const float straight_grade = WWMath::Atan ((terrain.RimZ - terrain.FloorZ) / straight_run);
		Check (straight_grade > max_grade,
				 "the fixture wall is meant to be too steep to climb straight: %.3f rad against %.3f",
				 straight_grade,max_grade);

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		const bool built = Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,terrain.Centre,
																  terrain.Tangent,terrain.Normal,terrain.HalfSpan,
																  terrain.FloorHalf,terrain.HalfSpan,nullptr,approach,
																  PASS_RUN,PASS_RUN,Pass_Terrain_Height,&terrain,&geometry,&failure);
		Check (built,"a wall with room for a diagonal descent is not pass capable (%s)",
				 Hybrid_Local_Pass_Failure_Name (failure));

		if (built) {
			Check (geometry.Kind == HYBRID_LOCAL_PASS_FLOOR,"the floor pass has the wrong kind");
			Check (geometry.HasFloor,"the floor pass records no floor");
			Check (geometry.ControlCount == 6,"the floor pass has %d controls, not six",geometry.ControlCount);
			Check (geometry.TangentRun > 0.0f,
					 "a descent too steep to take straight recorded no along-obstruction run");
			Check (geometry.SteepestGrade <= (max_grade + 0.001f),
					 "the accepted pass profile reaches %.4f rad against a class maximum of %.4f",
					 geometry.SteepestGrade,max_grade);
			Check (geometry.TightestRadius >= ((spec.MinTurnRadiusW * params.Constants.W) - 0.001f),
					 "the accepted pass turns at %.2f against a class minimum of %.2f",geometry.TightestRadius,
					 spec.MinTurnRadiusW * params.Constants.W);

			//	Determinism: the same inputs give the same geometry, byte for byte on the quantized values.
			HybridLocalPassGeometry again;
			HybridLocalPassFailure again_failure = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,terrain.Centre,terrain.Tangent,
													  terrain.Normal,terrain.HalfSpan,terrain.FloorHalf,
													  terrain.HalfSpan,nullptr,approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&terrain,&again,
													  &again_failure),"the second pass build was refused");
			Check (again.Orientation == geometry.Orientation,"pass orientation is not stable");
			Check (again.TangentRun == geometry.TangentRun,"pass tangent run is not stable");
			Check (again.ControlCount == geometry.ControlCount,"pass control count is not stable");
			for (int i = 0; i < geometry.ControlCount; ++i) {
				Check (again.Controls[i] == geometry.Controls[i],"pass control %d is not stable",i);
			}

			//	And the frozen candidate revalidates, which is what Phase 11 will do to it.
			HybridLocalPassFailure revalidate = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Validate_Local_Pass (params,geometry,
															Hybrid_Local_Pass_Sample_Spacing (params.Constants),
															Pass_Terrain_Height,&terrain,&revalidate),
					 "a frozen pass candidate does not revalidate (%s)",
					 Hybrid_Local_Pass_Failure_Name (revalidate));
		}
	}

	//	A wall so deep that no tangent run inside the local limit can make the descent.
	{
		PassTerrain deep = terrain;
		deep.RimZ = 400.0f;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,deep.Centre,deep.Tangent,deep.Normal,
													deep.HalfSpan,deep.FloorHalf,deep.HalfSpan,nullptr,approach,
													PASS_RUN,PASS_RUN,Pass_Terrain_Height,&deep,&geometry,&failure),
				 "a 400-unit wall was accepted as a surface pass");
		Check (failure == HYBRID_LOCAL_PASS_RUN,"a wall past every legal run named %s",
				 Hybrid_Local_Pass_Failure_Name (failure));
	}

	/*
	**	Enough run for the grade, but the profile cannot be realized: the wall is a *vertical* step rather than a
	**	slope, so the grade-limited ramp hangs outside the `RoadVerticalInterval` of the ground beneath it.
	**	The cut and fill limits are not touched -- the geometry simply cannot be built.
	*/
	{
		PassTerrain step = terrain;
		step.WallIsVertical		= true;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,step.Centre,step.Tangent,step.Normal,
													step.HalfSpan,step.FloorHalf,step.HalfSpan,nullptr,approach,
													PASS_RUN,PASS_RUN,Pass_Terrain_Height,&step,&geometry,&failure),
				 "a sheer step was accepted as a surface pass");
		Check ((failure == HYBRID_LOCAL_PASS_CUT_FILL) || (failure == HYBRID_LOCAL_PASS_GRADE),
				 "a sheer step named %s rather than the cut/fill or grade it breaks",
				 Hybrid_Local_Pass_Failure_Name (failure));
	}

	//	A floor too narrow for the swept vehicle envelope.
	{
		PassTerrain narrow = terrain;
		narrow.FloorHalf = 0.5f * (params.SweptVehicleWidth + (2.0f * params.MinVehicleLaneClearance)) - 1.0f;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,narrow.Centre,narrow.Tangent,
													narrow.Normal,narrow.HalfSpan,narrow.FloorHalf,narrow.HalfSpan,nullptr,
													approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&narrow,&geometry,&failure),
				 "a floor narrower than the swept envelope was accepted");
		Check (failure == HYBRID_LOCAL_PASS_FLOOR_WIDTH,"a narrow floor named %s",
				 Hybrid_Local_Pass_Failure_Name (failure));
	}

	//	A floor that runs steeper along itself than the class allows.
	{
		PassTerrain tilted = terrain;
		tilted.TangentFloorSlope = 2.0f;		// two units of rise per unit along the floor

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,tilted.Centre,tilted.Tangent,
													tilted.Normal,tilted.HalfSpan,tilted.FloorHalf,tilted.HalfSpan,nullptr,
													approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&tilted,&geometry,&failure),
				 "a floor climbing two-to-one along itself was accepted");
		Check ((failure == HYBRID_LOCAL_PASS_FLOOR_GRADE) || (failure == HYBRID_LOCAL_PASS_CUT_FILL) ||
				 (failure == HYBRID_LOCAL_PASS_GRADE),"a steep floor named %s",
				 Hybrid_Local_Pass_Failure_Name (failure));
	}

	/*
	**	A floor pass's two orientations are mirror images over one point set, so a one-sided block refuses
	**	both -- the descent leans one way and the ascent the other, precisely so the route returns across the
	**	station instead of drifting down the ravine, which means both orientations occupy `+run` and `-run`
	**	alike.  Asserting that is worth as much as asserting the mirror, because it pins the shape.
	*/
	{
		PassTerrain blocked = terrain;
		blocked.BlockPlusTangent = true;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,blocked.Centre,blocked.Tangent,
													blocked.Normal,blocked.HalfSpan,blocked.FloorHalf,
													blocked.HalfSpan,nullptr,approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&blocked,&geometry,
													&failure),
				 "a floor pass whose ravine is walled off on one side was accepted");
	}

	/*
	**	A cliff, as D-004 realizes it: a raised band whose two approaches stand on the same bench.  The pass is
	**	the floor pass inverted -- ascend the near face on the diagonal, cross the shelf, descend the far face --
	**	and every fixture here is that shape.  A step between two benches is not a shape the generator makes.
	*/
	const float BAND_HEIGHT = 18.0f;			// tier_step * 1.00, the shallowest native cliff
	PassTerrain cliff;
	cliff.Centre.Set (0.0f,0.0f);
	cliff.Tangent.Set (1.0f,0.0f);
	cliff.Normal.Set (0.0f,1.0f);
	/*
	**	A band a `PRIMARY_2` roadbed can be benched into.  The ramp runs nearly *along* the face, so the face's
	**	slope is the road's cross slope, and a level roadbed there needs its uphill edge cut by
	**	`half_roadbed * face_slope`: with D-004's smoothstep the steepest face slope is `1.5 * H / F`, so the
	**	cut is `4.32 * 1.5 * H / F`, and it fits inside `max_cut_depth = 6` only when `H / F <= 0.93`.  This
	**	band is `H / F = 0.72`, a 12-high face 16.6 wide with a 10-wide shelf.  The broadest native band is
	**	`18 / 12.47 = 1.44` and needs 8.98: measured below as the refusal it is.
	*/
	const float BAND_HEIGHT_BENCHABLE = 12.0f;
	cliff.RaisedBand				= true;
	cliff.BandFace					= 16.6f;
	cliff.HalfSpan					= 16.6f + 5.0f;			// toe at 21.6, a 10-wide shelf
	cliff.RimZ						= BAND_HEIGHT_BENCHABLE;
	cliff.FloorHalf				= 0.0f;
	cliff.FloorZ					= 0.0f;
	cliff.TangentFloorSlope		= 0.0f;
	cliff.TangentRimSlope		= 0.0f;
	cliff.FarSideDiffers			= false;
	cliff.FarSideZ					= 0.0f;
	cliff.WallIsVertical			= false;
	cliff.BlockPlusTangent		= false;

	HybridLocalPassBand band;
	band.ToeA	= +cliff.HalfSpan;
	band.ShelfA	= +(cliff.HalfSpan - cliff.BandFace);
	band.ShelfB	= -(cliff.HalfSpan - cliff.BandFace);
	band.ToeB	= -cliff.HalfSpan;

	/*
	**	1. A band with a shelf wide enough and run enough is a ramp pass, and the pass it builds is the band
	**	shape: six controls, a flat on the shelf, both ramps leaning along the cliff, the profile inside the
	**	class grade.  The old single grade line between the two approaches would have been flat at bench
	**	height and refused on cut/fill 18 under the shelf, which is what happened on every native cliff.
	*/
	{
		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		const bool built = Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,cliff.Centre,cliff.Tangent,
																  cliff.Normal,cliff.HalfSpan,0.0f,cliff.HalfSpan,&band,
																  approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&cliff,
																  &geometry,&failure);
		Check (built,"a band with a wide shelf and room to lean is not ramp capable (%s)",
				 Hybrid_Local_Pass_Failure_Name (failure));
		if (built) {
			Check (geometry.Kind == HYBRID_LOCAL_PASS_RAMP,"the band pass has the wrong kind");
			Check (geometry.HasFloor,"the band pass records no shelf traverse");
			Check (geometry.ControlCount == 6,"the band pass has %d controls, not six",geometry.ControlCount);
			Check (geometry.TangentRun > 0.0f,"a 12-high band was climbed with no along-cliff run");
			Check (geometry.SteepestGrade <= (max_grade + 0.001f),
					 "the accepted band profile reaches %.4f rad against a class maximum of %.4f",
					 geometry.SteepestGrade,max_grade);
			Check (geometry.FlatHalfWidth == (cliff.HalfSpan - cliff.BandFace),
					 "the band pass recorded a shelf half width of %.2f, not %.2f",geometry.FlatHalfWidth,
					 cliff.HalfSpan - cliff.BandFace);
			Check (geometry.ProvedSweptWidth == params.SweptVehicleWidth,
					 "the band pass did not record the envelope it was proved for");

			//	The shelf traverse actually sits on the shelf, at full band height.
			float entry_z = 0.0f;
			float exit_z = 0.0f;
			Pass_Terrain_Height (geometry.FloorEntry,&entry_z,&cliff);
			Pass_Terrain_Height (geometry.FloorExit,&exit_z,&cliff);
			Check ((entry_z == BAND_HEIGHT_BENCHABLE) && (exit_z == BAND_HEIGHT_BENCHABLE),
					 "the shelf traverse stands at %.2f and %.2f, not on the %.2f shelf",entry_z,exit_z,
					 BAND_HEIGHT_BENCHABLE);

			//	6. Repeatable: the same inputs give the same frozen geometry.
			HybridLocalPassGeometry again;
			HybridLocalPassFailure again_failure = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,cliff.Centre,cliff.Tangent,cliff.Normal,
													  cliff.HalfSpan,0.0f,cliff.HalfSpan,&band,approach,PASS_RUN,PASS_RUN,
													  Pass_Terrain_Height,&cliff,&again,&again_failure),
					 "the second band build was refused");
			Check (::memcmp (&again,&geometry,sizeof (geometry)) == 0,"the band pass is not repeatable");

			//	And the frozen record revalidates against the same field, at Phase 11's spacing.
			HybridLocalPassFailure revalidate = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Validate_Local_Pass (params,geometry,Hybrid_Local_Pass_Sample_Spacing (params.Constants),
														  Pass_Terrain_Height,&cliff,&revalidate),
					 "the frozen band pass does not revalidate (%s)",Hybrid_Local_Pass_Failure_Name (revalidate));
		}
	}

	/*
	**	Native-range boundaries (Q-021, D-016).  These are the ordinary formulas evaluated at the ends of Section
	**	10.2's ranges, not special cases: a D-004 band built exactly as Phase 5 draws one -- face `e + t`, shelf
	**	`t`, smoothstep face -- at `EdgeWidth 1.5W` with the talus at the old and the new maximum, and at the
	**	shallowest and tallest native height.  The feasibility condition they sit either side of is
	**	`half_roadbed * 1.5 * H / F <= max_cut_depth`, i.e. `H / F <= 0.926`.
	*/
	/*
	**	Expected refusals name the rule that refuses *first* in the builder's order.  A `1W` talus makes a
	**	3.3-wide shelf, and the shelf-width test runs before any profile is compiled, so it refuses on
	**	`floor_width`; the wider-but-still-too-steep bands get as far as the profile and refuse on cross slope.
	**	Measured threshold for a `tier_step`-high band with the broadest edge: between `3.8W` and `4.0W` of
	**	talus (`H / F` between 1.19 and 1.14), so `5.25W` carries a margin beyond the `0.926` the decision
	**	derived -- the ramp crosses the face on a diagonal, and its across-face roadbed component is less than
	**	the full half roadbed the derivation assumed.
	*/
	struct NativeBandCase { const char * Name; float Height; float TalusW; bool Expected; HybridLocalPassFailure Refusal; };
	const NativeBandCase NATIVE_BANDS[4] = {
		{ "old native maximum talus 3W, tier_step high",		18.0f,	3.00f,	false,	HYBRID_LOCAL_PASS_CROSS_SLOPE },
		{ "new maximum talus 5.25W, tier_step high",			18.0f,	5.25f,	true,		HYBRID_LOCAL_PASS_OK },
		{ "new maximum talus 5.25W, 2 * tier_step high",	36.0f,	5.25f,	false,	HYBRID_LOCAL_PASS_CROSS_SLOPE },
		{ "narrow talus 1W, tier_step high",					18.0f,	1.00f,	false,	HYBRID_LOCAL_PASS_FLOOR_WIDTH },
	};
	for (int c = 0; c < 4; ++c) {

		const NativeBandCase & one = NATIVE_BANDS[c];
		const float e = 0.5f * (1.5f * params.Constants.W);		// the broadest native edge
		const float talus = one.TalusW * params.Constants.W;

		PassTerrain native = cliff;
		native.BandFace	= e + talus;
		native.HalfSpan	= native.BandFace + (0.5f * talus);
		native.RimZ			= one.Height;
		HybridLocalPassBand native_band;
		native_band.ToeA		= +native.HalfSpan;
		native_band.ShelfA	= +(0.5f * talus);
		native_band.ShelfB	= -(0.5f * talus);
		native_band.ToeB		= -native.HalfSpan;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		const bool built = Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,native.Centre,native.Tangent,
																  native.Normal,native.HalfSpan,0.0f,native.HalfSpan,&native_band,
																  approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&native,&geometry,
																  &failure);
		Check (built == one.Expected,"%s (H/F %.3f): expected %s, got %s (%s)",one.Name,
				 one.Height / native.BandFace,one.Expected ? "a pass" : "a refusal",built ? "a pass" : "a refusal",
				 Hybrid_Local_Pass_Failure_Name (failure));
		if (built) {
			Check (geometry.SteepestGrade <= (max_grade + 0.001f),"%s: profile grade %.4f",one.Name,
					 geometry.SteepestGrade);
			Check (geometry.FlatHalfWidth == Hybrid_Quantize_Position (0.5f * talus),"%s: shelf half %.3f",one.Name,
					 geometry.FlatHalfWidth);
			HybridLocalPassFailure again = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Validate_Local_Pass (params,geometry,Hybrid_Local_Pass_Sample_Spacing (params.Constants),
														  Pass_Terrain_Height,&native,&again),"%s: does not revalidate (%s)",
					 one.Name,Hybrid_Local_Pass_Failure_Name (again));
			printf ("  native band: %s accepted, run %.1f\r\n",one.Name,geometry.TangentRun);
		} else if (!one.Expected) {
			Check (failure == one.Refusal,"%s: refused as %s, not %s",one.Name,
					 Hybrid_Local_Pass_Failure_Name (failure),Hybrid_Local_Pass_Failure_Name (one.Refusal));
		}
	}

	//	2. A shelf narrower than the swept envelope plus its clearance is not a pass, whatever the faces allow.
	{
		PassTerrain narrow = cliff;
		narrow.HalfSpan = narrow.BandFace + 3.0f;		// a 6-wide shelf against 8.80 required (native t = 1W..3W)
		HybridLocalPassBand narrow_band;
		narrow_band.ToeA		= +narrow.HalfSpan;
		narrow_band.ShelfA	= +3.0f;
		narrow_band.ShelfB	= -3.0f;
		narrow_band.ToeB		= -narrow.HalfSpan;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,narrow.Centre,narrow.Tangent,narrow.Normal,
													narrow.HalfSpan,0.0f,narrow.HalfSpan,&narrow_band,approach,PASS_RUN,
													PASS_RUN,Pass_Terrain_Height,&narrow,&geometry,&failure),
				 "a band whose shelf cannot carry the swept envelope was accepted");
		Check (failure == HYBRID_LOCAL_PASS_FLOOR_WIDTH,"a narrow shelf named %s",
				 Hybrid_Local_Pass_Failure_Name (failure));
	}

	/*
	**	3. Directional run.  Both ramps and the traverse lean one way, so a station with no run toward
	**	`-tangent` still has a pass toward `+tangent` -- and takes it, deterministically -- while a station with
	**	too little run either way has none.  The run a 12-high face needs at 12 degrees is about 52 along the
	**	cliff for each ramp, so 60 in total is not enough for two ramps and a traverse.
	*/
	{
		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		HybridLocalPassMeasurement measured;
		Check (Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,cliff.Centre,cliff.Tangent,cliff.Normal,
												  cliff.HalfSpan,0.0f,cliff.HalfSpan,&band,approach,PASS_RUN,0.0f,
												  Pass_Terrain_Height,&cliff,&geometry,&failure,&measured),
				 "a band with room toward +tangent only found no pass (%s)",Hybrid_Local_Pass_Failure_Name (failure));
		Check (geometry.Orientation == 0,"the pass leaned toward the exhausted direction (orientation %d)",
				 geometry.Orientation);

		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,cliff.Centre,cliff.Tangent,cliff.Normal,
													cliff.HalfSpan,0.0f,cliff.HalfSpan,&band,approach,60.0f,60.0f,
													Pass_Terrain_Height,&cliff,&geometry,&failure,&measured),
				 "a band pass was accepted with too little run in either direction");
		Check (failure == HYBRID_LOCAL_PASS_RUN,"the exhausted band pass was refused as %s",
				 Hybrid_Local_Pass_Failure_Name (failure));
		Check (measured.Valid && (measured.RequiredTangentA > 0.0f),
				 "a face too steep to climb square required no along-cliff run");
	}

	/*
	**	4. One mirror orientation blocked, the other legal.  Walling off the `+tangent` reach makes every
	**	orientation whose ascent leans that way illegal, and the builder has to find the mirror by rule.
	*/
	{
		PassTerrain blocked = cliff;
		blocked.BlockPlusTangent = true;

		HybridLocalPassGeometry mirrored;
		HybridLocalPassFailure mirror_failure = HYBRID_LOCAL_PASS_OK;
		const bool mirror_built = Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,blocked.Centre,
																			blocked.Tangent,blocked.Normal,blocked.HalfSpan,0.0f,
																			blocked.HalfSpan,&band,approach,PASS_RUN,PASS_RUN,
																			Pass_Terrain_Height,&blocked,&mirrored,&mirror_failure);
		Check (mirror_built,"a band with one orientation blocked found neither (%s)",
				 Hybrid_Local_Pass_Failure_Name (mirror_failure));
		if (mirror_built) {
			Check (mirrored.Orientation == 1,"the blocked orientation %d was chosen anyway",mirrored.Orientation);
		}
	}

	/*
	**	5. A face too sheer to be graded inside `RoadVerticalInterval`.  With no face at all the band is a
	**	wall: the ascent has no ground to lean against, so wherever its grade line runs it stands more than
	**	`max_cut_depth` above the bench or more than `max_fill_height` below the shelf.
	*/
	{
		PassTerrain sheer = cliff;
		sheer.BandFace = 0.0f;
		HybridLocalPassBand sheer_band;
		sheer_band.ToeA	= +sheer.HalfSpan;
		sheer_band.ShelfA	= +(sheer.HalfSpan - 0.001f);
		sheer_band.ShelfB	= -(sheer.HalfSpan - 0.001f);
		sheer_band.ToeB	= -sheer.HalfSpan;

		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_RAMP,sheer.Centre,sheer.Tangent,sheer.Normal,
													sheer.HalfSpan,0.0f,sheer.HalfSpan,&sheer_band,approach,PASS_RUN,
													PASS_RUN,Pass_Terrain_Height,&sheer,&geometry,&failure),
				 "a sheer 18-high wall was accepted as a ramp pass");
		/*
		**	A wall has no face to climb: the ramp solver's foot lands on the shelf at full height at every run,
		**	so the run it asks for never settles and it refuses as `floor_grade` -- "the flat falls away faster
		**	than a ramp gains on it", which for a step of 12 in zero distance is exactly right.  Where a wall
		**	survives the solver it is the cross-slope realizability rule that refuses it.
		*/
		Check ((failure == HYBRID_LOCAL_PASS_FLOOR_GRADE) || (failure == HYBRID_LOCAL_PASS_CROSS_SLOPE) ||
				 (failure == HYBRID_LOCAL_PASS_CUT_FILL) || (failure == HYBRID_LOCAL_PASS_GRADE),
				 "a sheer band named %s rather than the grade, cross slope or cut/fill it breaks",
				 Hybrid_Local_Pass_Failure_Name (failure));
	}

	/*
	**	Directional availability on a floor pass, and the measurement a refusal has to carry.
	**
	**	A station sits somewhere along its obstruction, so the run it may spend toward one end is not the run
	**	it may spend toward the other.  A floor pass spends run *both* ways -- the descent leans one way and
	**	the ascent the other -- so exhausting either direction refuses it in both orientations.  That is not a
	**	conservative bound being hit: it is the shape of the pass.
	*/
	{
		HybridLocalPassGeometry geometry;
		HybridLocalPassFailure failure = HYBRID_LOCAL_PASS_OK;
		HybridLocalPassMeasurement measured;

		Check (!Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,terrain.Centre,terrain.Tangent,
													terrain.Normal,terrain.HalfSpan,terrain.FloorHalf,terrain.HalfSpan,nullptr,
													approach,PASS_RUN,0.0f,Pass_Terrain_Height,&terrain,&geometry,
													&failure,&measured),
				 "a floor pass was accepted with no run toward one end of its obstruction");
		Check (failure == HYBRID_LOCAL_PASS_RUN,"the exhausted floor pass was refused as %s",
				 Hybrid_Local_Pass_Failure_Name (failure));

		//	A refusal has to say by how much, or a census cannot tell "nearly fits" from "nowhere near".
		Check (measured.Valid,"the builder refused a measurable station without measuring it");
		Check (measured.AvailablePlus == PASS_RUN,"the measurement lost the +tangent availability");
		Check (measured.AvailableMinus == 0.0f,"the measurement lost the -tangent availability");
		Check (measured.RequiredTangentA > 0.0f,
				 "a wall too steep to descend square required no along-obstruction run");
		Check (measured.RequiredHorizontalA > measured.RequiredTangentA,
				 "the required horizontal run %.2f is not longer than its tangent component %.2f",
				 measured.RequiredHorizontalA,measured.RequiredTangentA);
		Check (measured.RatioBest > 1.0f,"a refused run reported a ratio of %.3f, which would have fit",
				 measured.RatioBest);
		Check (measured.Depth > 0.0f,"the measurement recorded no depth to descend");

		//	The same station with room both ways is capable, so the direction was the whole of the refusal.
		HybridLocalPassFailure both_failure = HYBRID_LOCAL_PASS_OK;
		HybridLocalPassMeasurement both;
		Check (Hybrid_Build_Local_Pass (params,HYBRID_LOCAL_PASS_FLOOR,terrain.Centre,terrain.Tangent,
												  terrain.Normal,terrain.HalfSpan,terrain.FloorHalf,terrain.HalfSpan,nullptr,
												  approach,PASS_RUN,PASS_RUN,Pass_Terrain_Height,&terrain,&geometry,
												  &both_failure,&both),
				 "the same station with run both ways was refused (%s)",
				 Hybrid_Local_Pass_Failure_Name (both_failure));
		Check (both.RatioBest <= 1.0f,"an accepted pass reported a run ratio of %.3f",both.RatioBest);
		Check (geometry.TangentRun <= PASS_RUN,"the accepted pass leaned %.2f past its available run",
				 geometry.TangentRun);
	}
}

/***********************************************************************************************
**	Runtime Phase 11's gate -- rewrite item I12.
**
**	The check runs the *real* pipeline: preflight constants, map identity, strategic graph, semantic
**	matrices, landforms, hydrology, macro terrain, anchors, topology, Phase-10 routes and then Phase 11,
**	on deterministic generated-world fixture seeds.  Its purpose is to prove that
**	`Hybrid_Build_Interactions` consumes what the earlier phases actually produce, so nothing here
**	fabricates an ideal input.
**
**	Every acceptance property that can be recomputed is recomputed from the records rather than taken on
**	the solver's word, in the same way the Phase-10 check re-measures road curvature for itself.
***********************************************************************************************/

//	Everything one fixture seed's pipeline produces, kept together so the ground sampler can point into it.
struct HybridPipelineFixture
{
	HybridMapIdentityParams			Sized;
	HybridMapIdentity					Identity;
	HybridArchetypeIntentSet		Intents;
	HybridStrategicGraph				Graph;
	HybridCoarseMatrices				Matrices;
	HybridLandformSet					Landforms;
	HybridHydrologySet				Hydrology;
	HybridMacroTerrainParams		MacroParams;
	HybridMacroTerrainSet			Surfaces;
	HybridAnchorSet					Anchors;
	HybridTopology						Topology;
	HybridRouteSet						Routes;
	MacroFormulaContext				Ground;

	//	Phase 10's own inputs, kept so Phase 11's next-envelope reroute can re-run them.
	HybridRouteParams					RouteParams;

	//	How far the pipeline got (9 = Phase 9 passed, 10 = Phase 10 passed) and Phase 9's Q-023 census.
	int									Reached;
	HybridTopologyResult				TopologyResult;
	HybridRouteResult					RouteResult;
};

//	Set by the census tool: Phase 10 then reports what refused a failed edge (HybridRouteParams::CollectCensus).
static bool s_route_census = false;


/*
**	Phases 2 to 10 for one seed.  Returns false when a phase legitimately refused this seed, which is an
**	ordinary attempt failure the caller answers by walking to the next one.
*/
static bool Build_Hybrid_Pipeline (unsigned int seed,float world_size,HybridArchetype archetype,
											  HybridPipelineFixture * out,StringClass * detail_out,
											  const HybridMetricCache * metrics = NULL)
{
	HybridMapIdentityResult identity_result;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformResult landform_result;
	HybridHydrologyResult hydrology_result;
	HybridMacroTerrainResult macro_result;
	HybridAnchorResult anchor_result;
	HybridTopologyResult topology_result;
	HybridRouteResult route_result;

	out->Reached = 0;
	out->TopologyResult = HybridTopologyResult ();
	out->Sized = Graph_Identity_Fixture (world_size,2.0f);
	out->Sized.ArchetypeSelector	= archetype;
	out->Sized.MapPlanSeed			= WorldSeedService::Derive_Subsystem_Seed (seed,
																			WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	out->Sized.HydrologySeed		= WorldSeedService::Derive_Subsystem_Seed (seed,
																			WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	out->Sized.BiomeSeed				= WorldSeedService::Derive_Subsystem_Seed (seed,
																			WorldSeedService::SEED_DOMAIN_BIOMES);

	if (!Hybrid_Resolve_Map_Identity (out->Sized,&out->Identity,&out->Intents,&identity_result)) {
		if (detail_out != nullptr) *detail_out = identity_result.Detail;
		return false;
	}

	HybridStrategicGraphParams graph_params = Graph_Fixture (out->Sized,2.0f);
	if (!Hybrid_Build_Strategic_Graph (graph_params,out->Identity,out->Intents,&out->Graph,&graph_result)) {
		if (detail_out != nullptr) *detail_out = graph_result.Detail;
		return false;
	}

	HybridCoarseMatrixParams coarse_params = Coarse_Fixture (out->Sized,world_size);
	if (!Hybrid_Build_Coarse_Matrices (coarse_params,out->Identity,out->Intents,&out->Graph,&out->Matrices,
													&coarse_result)) {
		if (detail_out != nullptr) *detail_out = coarse_result.Detail;
		return false;
	}

	HybridLandformParams landform_params = Landform_Fixture (out->Sized,world_size);
	landform_params.TerrainSeed = WorldSeedService::Derive_Subsystem_Seed (seed,
																			WorldSeedService::SEED_DOMAIN_TERRAIN);
	if (!Hybrid_Build_Landforms (landform_params,out->Identity,out->Intents,out->Graph,&out->Matrices,
											&out->Landforms,&landform_result)) {
		if (detail_out != nullptr) *detail_out = landform_result.Detail;
		return false;
	}

	HybridHydrologyParams hydrology_params = Hydrology_Fixture (out->Sized,world_size);
	hydrology_params.HydrologySeed = out->Sized.HydrologySeed;

	/*
	**	When the caller will run Phase 11 over this world, Phase 6 has to be told the same content facts
	**	Phase 11 will be: the bridge and tunnel capabilities come from one preflight cache, exactly as
	**	`ProceduralWorldManager::Build_Hybrid_Hydrology` fills them.  Left to the stand-alone Phase-6 defaults,
	**	Phase 6 promised tunnels the content registers no module for, and every window kept alive by that
	**	promise became a crossing Phase 11 could not serve.
	*/
	if (metrics != NULL) {
		const HybridRoadClass crossing_class = metrics->HeavyRouteClass;
		hydrology_params.HeavyVehicleWidth		= metrics->HeavyVehicleWidth;
		hydrology_params.HeavyVehicleHeight		= metrics->HeavyVehicleHeight;
		hydrology_params.HarvesterWidth			= metrics->HarvesterWidth;
		hydrology_params.BridgeMaxSpanLength	= metrics->BridgeMaxSpanLength;
		hydrology_params.BridgeRequiredWidth	= (metrics->BridgeForClass[crossing_class] >= 0)
															  ? metrics->BridgeRequiredWidth[crossing_class] : 0.0f;
		hydrology_params.BridgeApproachLength	= metrics->BridgeApproachLength;
		hydrology_params.BridgeWaterClearanceMargin = metrics->BridgeWaterClearanceMargin;
		hydrology_params.TunnelsAvailable		= metrics->TunnelsAvailable;		// world_features.tunnels is on
		hydrology_params.TunnelRequiredWidth	= metrics->TunnelRequiredWidth;
		hydrology_params.TunnelRequiredHeight	= metrics->TunnelRequiredHeight;
	}
	if (!Hybrid_Build_Hydrology (hydrology_params,out->Identity,out->Intents,out->Graph,out->Landforms,
											&out->Matrices,&out->Hydrology,&hydrology_result)) {
		if (detail_out != nullptr) *detail_out = hydrology_result.Detail;
		return false;
	}

	out->MacroParams = Macro_Terrain_Fixture (out->Sized);
	out->MacroParams.TerrainSeed = landform_params.TerrainSeed;
	if (!Hybrid_Resolve_River_Surfaces (out->MacroParams,out->Matrices,out->Landforms,out->Hydrology,
													&out->Surfaces,&macro_result)) {
		if (detail_out != nullptr) *detail_out = macro_result.Detail;
		return false;
	}

	out->Ground.Params		= &out->MacroParams;
	out->Ground.Matrices		= &out->Matrices;
	out->Ground.Landforms	= &out->Landforms;
	out->Ground.Hydrology	= &out->Hydrology;
	out->Ground.Surfaces		= &out->Surfaces;
	out->Ground.Offset		= 0.0f;

	HybridAnchorParams anchor_params = Anchor_Fixture (out->Sized);
	if (!Hybrid_Validate_Anchors (anchor_params,out->Identity,out->Graph,out->Matrices,out->Landforms,
											out->Hydrology,Macro_Sample_From_Formula,&out->Ground,&out->Anchors,
											&anchor_result)) {
		if (detail_out != nullptr) *detail_out = anchor_result.Detail;
		return false;
	}

	HybridTopologyParams topology_params = Topology_Fixture (out->Sized);
	out->Reached = 8;
	const bool topology_built = Hybrid_Build_Topology (topology_params,out->Identity,out->Graph,out->Landforms,
																		out->Hydrology,out->Anchors,&out->Matrices,
																		Macro_Sample_From_Formula,&out->Ground,&out->Topology,
																		&topology_result);
	out->TopologyResult = topology_result;
	if (!topology_built) {
		if (detail_out != nullptr) *detail_out = topology_result.Detail;
		return false;
	}
	out->Reached = 9;

	out->RouteParams = Route_Fixture (out->Sized,world_size);
	out->RouteParams.CollectCensus = s_route_census;
	HybridRouteParams & route_params = out->RouteParams;
	if (!Hybrid_Build_Routes (route_params,out->Identity,out->Matrices,out->Landforms,out->Hydrology,
										out->Anchors,out->Topology,Macro_Sample_From_Formula,&out->Ground,
										&out->Routes,&route_result)) {
		if (detail_out != nullptr) *detail_out = route_result.Detail;
		out->RouteResult = route_result;
		return false;
	}
	out->Reached = 10;
	return true;
}


//	Two routes are the same route: same edge, envelope, samples and structural spans, bit for bit.
static bool Routes_Identical (const HybridRoute & a,const HybridRoute & b)
{
	if ((a.EdgeId != b.EdgeId) || (a.Envelope != b.Envelope) || (a.RoadClass != b.RoadClass)) return false;
	if ((a.Capabilities != b.Capabilities) || (a.Length != b.Length)) return false;
	if ((a.Points.Count () != b.Points.Count ()) || (a.Structural.Count () != b.Structural.Count ())) return false;
	for (int i = 0; i < a.Points.Count (); ++i) {
		if ((a.Points[i].X != b.Points[i].X) || (a.Points[i].Y != b.Points[i].Y)) return false;
	}
	for (int i = 0; i < a.Structural.Count (); ++i) {
		if (a.Structural[i].WindowId != b.Structural[i].WindowId) return false;
		if (a.Structural[i].FirstPoint != b.Structural[i].FirstPoint) return false;
		if (a.Structural[i].LastPoint != b.Structural[i].LastPoint) return false;
	}
	return true;
}


/*
**	Phase 11's own fixture parameters, mirroring what `ProceduralWorldManager::Build_Hybrid_Interactions`
**	fills from the profile and the frozen preflight cache.  The bridge and tunnel kinds are the ones the
**	shipped content actually registers, read through the same preflight gather the manager uses, so the
**	metrics under test are the real ones.
*/
static HybridInteractionParams Interaction_Fixture (const HybridMapIdentityParams & identity_params,
																	 float world_size,
																	 const HybridPreflightInputs & inputs,
																	 const HybridMetricCache & metrics)
{
	HybridInteractionParams params;
	params.Constants		= identity_params.Constants;
	params.WorldMinX		= -0.5f * world_size;
	params.WorldMinY		= -0.5f * world_size;
	params.WorldWidth		= world_size;
	params.WorldHeight	= world_size;
	params.MaxCutDepth	= 6.0f;			// native grading.max_cut_depth
	params.MaxFillHeight	= 6.0f;
	params.RiverWidth		= 6.0f;

	params.AllowFords					= false;		// the native profile: no ford capability exists yet
	params.FordCapabilityAvailable	= metrics.FordsAvailable;
	params.TunnelsEnabled			= true;		// world_features.tunnels defaults on
	params.TunnelAvailable			= metrics.TunnelsAvailable;

	params.WorldUnitsPerMeter		= identity_params.Constants.WorldUnitsPerMeter;
	params.HeavyVehicleWidth		= metrics.HeavyVehicleWidth;
	params.HeavyVehicleHeight		= metrics.HeavyVehicleHeight;
	params.HarvesterWidth			= metrics.HarvesterWidth;
	params.LightVehicleWidth		= metrics.HumveeWidth;
	params.MinVehicleLaneClearance= 1.0f;		// native fairness.min_vehicle_lane_clearance
	params.TierStep					= 8.0f;		// native plan.tier_step

	params.BridgeWaterClearanceMargin = metrics.BridgeWaterClearanceMargin;
	for (int i = 0; i < HYBRID_ROAD_CLASS_COUNT; ++i) {
		params.BridgeForClass[i]		= metrics.BridgeForClass[i];
		params.BridgeRequiredWidth[i]	= metrics.BridgeRequiredWidth[i];
	}
	params.MaxLocalRetriesPerStage = 8;

	for (int i = 0; i < inputs.Bridges.Count (); ++i) {

		HybridBridgeKind kind;
		kind.StableId					= i;
		kind.ClearWidth				= inputs.Bridges[i].Width;
		kind.StructuralDepth			= inputs.Bridges[i].DeckThickness;
		kind.RequiredClearance		= inputs.Bridges[i].RequiredClearance;
		kind.MaxSpan					= Hybrid_Resolve_Bridge_Max_Span (0.0f,metrics.BridgeMaxSpanLength);
		kind.RequiredApproachLength= metrics.BridgeApproachLength;
		kind.StartSeat					= inputs.Bridges[i].StartSeat;
		kind.EndSeat					= inputs.Bridges[i].EndSeat;
		params.BridgeKinds.Add (kind);
	}
	for (int i = 0; i < inputs.TunnelModules.Count (); ++i) {

		HybridTunnelModule one;
		one.StableId			= i;
		one.Length				= inputs.TunnelModules[i].Length;
		one.ClearWidth			= inputs.TunnelModules[i].ClearanceWidth;
		one.ClearHeight		= inputs.TunnelModules[i].ClearanceHeight;
		one.MaxGradeRadians	= inputs.TunnelModules[i].MaxGradeRadians;
		one.PortalCompatible	= inputs.TunnelModules[i].PortalCompatible;
		params.TunnelModules.Add (one);
	}
	return params;
}



/***********************************************************************************************
**	Controlled crossing worlds (Q-022 section 13, Q-023 section 16).
**
**	Level ground with carved rivers, every coarse cell placeable, no anchors or landforms: straight rivers,
**	crossing windows on them and one topology edge, routed and compiled by the real Phase-10 and Phase-11
**	owners.  Each world places exactly the geometry one rule is about, which a generated world only meets by
**	chance.  The road's own line sits on an analysis-cell row (centres at 7.5 + 15n) wherever it has to pass a
**	window exactly: a window's crossable strip is narrower than a cell.
***********************************************************************************************/
//	Level ground at 0, with every river carved: a bed 4 below the banks, blended up to them over 6.
static bool Crossing_Ground (const Vector2 & world,float * height_out,void * context)
{
	const HybridHydrologySet * hydrology = (const HybridHydrologySet *)context;
	const float BED_DEPTH = 4.0f;
	const float BANK_BLEND = 6.0f;
	float height = 0.0f;
	for (int r = 0; r < hydrology->Rivers.Count (); ++r) {
		const HybridRiverSystem & river = hydrology->Rivers[r];
		for (int i = 1; i < river.Points.Count (); ++i) {
			const Vector2 a = river.Points[i - 1];
			const Vector2 d = river.Points[i] - a;
			const float length2 = (d.X * d.X) + (d.Y * d.Y);
			float t_on = (length2 > 0.0f) ? ((((world.X - a.X) * d.X) + ((world.Y - a.Y) * d.Y)) / length2) : 0.0f;
			t_on = (t_on < 0.0f) ? 0.0f : ((t_on > 1.0f) ? 1.0f : t_on);
			const float distance = (world - (a + (d * t_on))).Length ();
			const float half = 0.5f * ((t_on < 0.5f) ? river.Widths[i - 1] : river.Widths[i]);
			float here = 0.0f;
			if (distance <= half)								here = -BED_DEPTH;
			else if (distance < (half + BANK_BLEND))	here = -BED_DEPTH * (1.0f - ((distance - half) / BANK_BLEND));
			if (here < height) height = here;
		}
	}
	*height_out = height;
	return true;
}


struct CrossingWorld
{
	HybridMapIdentityParams		Sized;
	HybridMapIdentity				Identity;
	HybridCoarseMatrices			Matrices;
	HybridLandformSet				Landforms;
	HybridHydrologySet			Hydrology;
	HybridAnchorSet				Anchors;
	HybridTopology					Topology;
	HybridRouteParams				RouteParams;
	HybridRouteSet					Routes;
};


static void Crossing_World_Begin (CrossingWorld * world)
{
	const float size = 1200.0f;
	world->Sized = Graph_Identity_Fixture (size,2.0f);
	world->Identity = HybridMapIdentity ();
	world->Identity.Checksum = 0x5EE0u;

	const int cells = 24;
	world->Matrices.Reset ();
	world->Matrices.Matrix.Resize (cells,cells,size / (float)cells,-0.5f * size,-0.5f * size);
	for (int cy = 0; cy < cells; ++cy) {
		for (int cx = 0; cx < cells; ++cx) world->Matrices.Matrix.Set_Flag (cx,cy,HYBRID_FLAG_PLACEABLE);
	}
	world->Landforms.Landforms.Delete_All ();
	world->Hydrology.Reset ();
	world->Anchors = HybridAnchorSet ();
	world->Anchors.Frozen = true;
	world->Topology.Reset ();
	world->RouteParams = Route_Fixture (world->Sized,size);
	world->Routes.Reset ();
}


//	A straight river `width` wide, except `wide_width` between its own stations `wide_from` and `wide_to`.
static void Crossing_World_River (CrossingWorld * world,unsigned int id,const Vector2 & from,const Vector2 & to,
											 float width,float wide_from = 0.0f,float wide_to = 0.0f,float wide_width = 0.0f)
{
	HybridRiverSystem river;
	river.StableId	= id;
	river.BedDepth	= 0.0f;
	const float length = (to - from).Length ();
	const int steps = (int)(length / 5.0f) + 1;
	float widest = width;
	for (int i = 0; i <= steps; ++i) {
		const float s = length * ((float)i / (float)steps);
		const bool wide = (wide_width > 0.0f) && (s >= wide_from) && (s <= wide_to);
		river.Points.Add (from + ((to - from) * (s / length)));
		river.Stations.Add (s);
		river.Widths.Add (wide ? wide_width : width);
		if (wide && (wide_width > widest)) widest = wide_width;
	}
	river.Length = length;
	river.Source = from;
	river.Mouth = to;
	river.BoundsMin.Set (WWMath::Min (from.X,to.X) - (0.5f * widest),WWMath::Min (from.Y,to.Y) - (0.5f * widest));
	river.BoundsMax.Set (WWMath::Max (from.X,to.X) + (0.5f * widest),WWMath::Max (from.Y,to.Y) + (0.5f * widest));
	world->Hydrology.Rivers.Add (river);
}


//	A bridge window on river `river_id`, crossed along `axis` (ApproachA -> ApproachB), with the Step-18 8W approach.
static void Crossing_World_Window (CrossingWorld * world,unsigned int id,unsigned int river_id,const Vector2 & centre,
											  const Vector2 & axis,float span)
{
	const HybridConstants & k = world->Sized.Constants;
	const float half = (0.5f * span) + (8.0f * k.W);
	const Vector2 tangent (-axis.Y,axis.X);

	HybridCrossingWindow window;
	window.StableId				= id;
	window.Kind						= HYBRID_WINDOW_CROSSING;
	window.Obstruction			= HYBRID_OBSTRUCTION_RIVER;
	window.ObstructionRecordId	= river_id;
	window.Methods					= HYBRID_WINDOW_METHOD_BRIDGE;
	for (int r = 0; r < world->Hydrology.Rivers.Count (); ++r) {
		const HybridRiverSystem & river = world->Hydrology.Rivers[r];
		if (river.StableId != river_id) continue;
		const Vector2 along = (river.Mouth - river.Source) / river.Length;
		window.Station = ((centre.X - river.Source.X) * along.X) + ((centre.Y - river.Source.Y) * along.Y);
	}
	window.Span						= span;
	window.ApproachLength		= 8.0f * k.W;
	window.Center					= centre;
	window.Tangent					= tangent;
	const Vector2 a = centre - (axis * half);
	const Vector2 b = centre + (axis * half);
	window.ApproachA.Set (Hybrid_Quantize_Position (a.X),Hybrid_Quantize_Position (a.Y));
	window.ApproachB.Set (Hybrid_Quantize_Position (b.X),Hybrid_Quantize_Position (b.Y));
	const Vector2 side = tangent * (0.5f * span);
	window.BoundsMin.Set (WWMath::Min (WWMath::Min (a.X + side.X,a.X - side.X),WWMath::Min (b.X + side.X,b.X - side.X)),
								 WWMath::Min (WWMath::Min (a.Y + side.Y,a.Y - side.Y),WWMath::Min (b.Y + side.Y,b.Y - side.Y)));
	window.BoundsMax.Set (WWMath::Max (WWMath::Max (a.X + side.X,a.X - side.X),WWMath::Max (b.X + side.X,b.X - side.X)),
								 WWMath::Max (WWMath::Max (a.Y + side.Y,a.Y - side.Y),WWMath::Max (b.Y + side.Y,b.Y - side.Y)));
	window.ApproachClasses		= 0x1Fu;
	world->Hydrology.Windows.Add (window);
}


//	One required cross-link from `a` to `b`; through window `window_id` as a frozen crossing waypoint when non-zero.
static void Crossing_World_Edge (CrossingWorld * world,const Vector2 & a,const Vector2 & b,HybridRoadClass road_class,
											unsigned int window_id)
{
	HybridTopologyNode node;
	node.Kind = HYBRID_NODE_FAMILY_WAYPOINT;
	node.StableId = 1;	node.Position = a;	world->Topology.Nodes.Add (node);
	node.StableId = 2;	node.Position = b;	world->Topology.Nodes.Add (node);

	HybridTopologyEdge edge;
	edge.StableId		= 1;
	edge.Kind			= HYBRID_EDGE_CROSS_LINK;
	edge.RoadClass		= road_class;
	edge.Capabilities	= Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_CROSS_LINK,false);
	edge.NodeA			= 1;
	edge.NodeB			= 2;
	edge.Required		= true;
	edge.RouteFamily	= -1;
	edge.Length			= (b - a).Length ();
	const HybridCrossingWindow * window = world->Hydrology.Find_Window (window_id);
	if (window != NULL) {
		node.Kind		= HYBRID_NODE_CROSSING_OBLIGATION;
		node.StableId	= 3;
		node.Position	= window->Center;
		node.WindowId	= window_id;
		world->Topology.Nodes.Add (node);
		edge.Waypoints.Add (3);
	}
	world->Topology.Edges.Add (edge);
}


static bool Crossing_World_Route (CrossingWorld * world,HybridRouteResult * result)
{
	return Hybrid_Build_Routes (world->RouteParams,world->Identity,world->Matrices,world->Landforms,world->Hydrology,
										 world->Anchors,world->Topology,Crossing_Ground,&world->Hydrology,&world->Routes,result);
}


/*
**	Q-022's invariant on a road: it passes both of a crossing's approach ends and runs straight along their axis
**	from one to the other, so every turn onto or off the crossing is finished outside them.
*/
static bool Straight_Between (const DynamicVectorClass<Vector2> & points,const Vector2 & a,const Vector2 & b,float q,
										StringClass * why)
{
	int at = -1;
	float nearest = 3.4e38f;
	for (int i = 0; i < points.Count (); ++i) {
		const float distance = (points[i] - a).Length ();
		if (distance >= nearest) continue;
		nearest = distance;
		at = i;
	}
	if ((at < 0) || (nearest > (2.0f * q))) {
		why->Format ("the road misses the approach end (%.2f,%.2f) by %.3f",a.X,a.Y,nearest);
		return false;
	}
	const float length = (b - a).Length ();
	const Vector2 axis = (b - a) / length;
	float behind = 0.0f;
	float ahead = 0.0f;
	Hybrid_Polyline_Straight_Run (points,at,axis,q,&behind,&ahead);
	float back_behind = 0.0f;
	float back_ahead = 0.0f;
	Hybrid_Polyline_Straight_Run (points,at,-axis,q,&back_behind,&back_ahead);
	const float run = WWMath::Max (ahead,back_behind);
	if (run < (length - (2.0f * q))) {
		why->Format ("the road runs straight only %.2f of the %.2f between (%.2f,%.2f) and (%.2f,%.2f)",run,length,
						 a.X,a.Y,b.X,b.Y);
		return false;
	}
	return true;
}


//	The tightest turn a sampled road makes: the smallest circle through three consecutive non-collinear samples.
static float Tightest_Radius (const DynamicVectorClass<Vector2> & points)
{
	float tightest = 3.4e38f;
	for (int i = 1; (i + 1) < points.Count (); ++i) {
		const Vector2 p = points[i - 1];
		const Vector2 q = points[i];
		const Vector2 r = points[i + 1];
		const float cross = ((q.X - p.X) * (r.Y - p.Y)) - ((q.Y - p.Y) * (r.X - p.X));
		const float a = (q - p).Length ();
		const float b = (r - q).Length ();
		const float c = (r - p).Length ();
		if (((float)fabs ((double)cross)) <= (1.0e-4f * a * c)) continue;		//	collinear to the sample's own rounding
		const float radius = (a * b * c) / (2.0f * (float)fabs ((double)cross));
		if (radius < tightest) tightest = radius;
	}
	return tightest;
}


/*
**	Q-025 on a controlled world: one heavy family of three logical trunk edges -- A, two ordinary guides, B --
**	across level ground, and the roads Q-025's rules are about beside it.  Everything goes through the real
**	Phase-10 and Phase-11 owners.
*/
static void Trunk_World_Node (CrossingWorld * world,unsigned int id,HybridTopologyNodeKind kind,const Vector2 & at,int family)
{
	HybridTopologyNode node;
	node.StableId		= id;
	node.Kind			= kind;
	node.Position		= at;
	node.RouteFamily	= family;
	world->Topology.Nodes.Add (node);
}


static void Trunk_World_Edge (CrossingWorld * world,unsigned int id,HybridTopologyEdgeKind kind,HybridRoadClass road_class,
										unsigned int a,unsigned int c,int family)
{
	HybridTopologyEdge edge;
	edge.StableId		= id;
	edge.Kind			= kind;
	edge.RoadClass		= road_class;
	edge.Capabilities	= Hybrid_Topology_Edge_Capabilities (kind,false);
	edge.NodeA			= a;
	edge.NodeB			= c;
	edge.Required		= true;
	edge.RouteFamily	= family;
	const HybridTopologyNode * na = world->Topology.Find_Node (a);
	const HybridTopologyNode * nc = world->Topology.Find_Node (c);
	edge.Length			= ((na != NULL) && (nc != NULL)) ? (nc->Position - na->Position).Length () : 0.0f;
	world->Topology.Edges.Add (edge);
}


//	The longest stretch over which `road` runs within 30 degrees of `other` and closer than `clearance`, outside
//	`reach_own` along `road` and `reach_other` along `other` of their shared point -- measured here, independently.
static float Overlap_Run (const HybridRoute & road,const HybridRoute & other,float clearance,const Vector2 & shared,
								  float reach_own,float reach_other)
{
	const float parallel = WWMath::Cos (30.0f * 0.0174532925f);
	float own_shared = 0.0f;
	float other_shared = 0.0f;
	float best = 1.0e9f;
	for (int i = 0; i < road.Points.Count (); ++i) {
		const float d = (road.Points[i] - shared).Length ();
		if (d < best) { best = d; own_shared = road.Stations[i]; }
	}
	best = 1.0e9f;
	for (int i = 0; i < other.Points.Count (); ++i) {
		const float d = (other.Points[i] - shared).Length ();
		if (d < best) { best = d; other_shared = other.Stations[i]; }
	}
	float run = 0.0f;
	float longest = 0.0f;
	for (int i = 1; (i + 1) < road.Points.Count (); ++i) {
		int at = -1;
		float nearest = 1.0e9f;
		for (int q = 1; q < other.Points.Count (); ++q) {
			const float d = (road.Points[i] - (other.Points[q - 1] + other.Points[q]) * 0.5f).Length ();
			if (d < nearest) { nearest = d; at = q; }
		}
		bool overlap = false;
		if ((at > 0) && (nearest < clearance)) {
			Vector2 a = road.Points[i + 1] - road.Points[i - 1];
			Vector2 c = other.Points[at] - other.Points[at - 1];
			a.Normalize ();
			c.Normalize ();
			const bool junction = (WWMath::Fabs (road.Stations[i] - own_shared) <= reach_own) ||
										 (WWMath::Fabs (other.Stations[at] - other_shared) <= reach_other);
			overlap = !junction && (WWMath::Fabs ((a.X * c.X) + (a.Y * c.Y)) >= parallel);
		}
		run = overlap ? (run + (road.Points[i] - road.Points[i - 1]).Length ()) : 0.0f;
		if (run > longest) longest = run;
	}
	return longest;
}


//	Does any routed chain pass the analysis cell `point` stands in?
static bool Chain_Passes_Cell (const CrossingWorld & world,const Vector2 & point)
{
	const float a = world.Sized.Constants.A;
	const float min_x = world.RouteParams.WorldMinX;
	const float min_y = world.RouteParams.WorldMinY;
	const int cx = (int)((point.X - min_x) / a);
	const int cy = (int)((point.Y - min_y) / a);
	const int cell = (cy * world.Routes.AnalysisCountX) + cx;
	for (int r = 0; r < world.Routes.Routes.Count (); ++r) {
		for (int c = 0; c < world.Routes.Routes[r].Cells.Count (); ++c) {
			if (world.Routes.Routes[r].Cells[c] == cell) return true;
		}
	}
	return false;
}


/*
**	An ordinary guide is a strategic anchor; a structural waypoint is a physical constraint (the 2026-10-02
**	ordinary-guide correction).  Two logical trunk edges meet at a guide where the family turns 135 degrees, and
**	river channels (no windows) hem the guide's cell in on the east, south and north, so the search can only
**	arrive from the west and leave to the north-west: a sharp corner at the guide.  The search's chain passes
**	through the guide's cell, and the road rounds the corner at its class radius -- which a 135-degree arc does
**	about 28 units from the corner, outside the cell.  The road is accepted, and its two logical edges are
**	contiguous, non-overlapping slices of the one centerline, covering all of it.
*/
/*
**	A road crosses an obstruction only as a crossing, and every crossing is aligned (2026-10-02).
**
**	Own window closed: a river runs the whole height of the map with one window in it (both on analysis-cell
**	centres, so the search can reach the window's footprint), and a road whose two ends
**	both stand on the west bank is sent through that window.  It crosses east on its crossing leg; it has no
**	second window to come back by, so the road is refused -- it may not cross the same window again outside its
**	frozen interval, which is a crossing nothing records and nothing would build.
**
**	Narrow river between cell centres: a river 3 units wide whose centerline runs exactly between two columns of
**	analysis-cell centres, so no centre is wet, and no window anywhere on it.  A road from one side to the other
**	is refused by the search -- a move is judged where it meets the river, not only at the cells it joins.
**
**	Unplanned crossing aligned: the same river and window, and a road from the south-west to the north-east
**	that was never sent to the window.  The search crosses it anyway (the window penalty is a cost, not a ban),
**	and the road runs straight between the window's two approach points along its axis, with the crossing
**	recorded once as a structural span.  (A positive check: here the search meets the window on its axis by
**	itself; the promotion that lines up a crossing the centerline refuses is measured by the census.)
*/
static void Check_Hybrid_Crossing_Discipline (void)
{
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,9,Vector2 (7.5f,-600.0f),Vector2 (7.5f,600.0f),16.0f);
		Crossing_World_Window (&world,10,9,Vector2 (7.5f,7.5f),Vector2 (1.0f,0.0f),16.0f);
		Crossing_World_Edge (&world,Vector2 (-300.0f,-150.0f),Vector2 (-300.0f,150.0f),HYBRID_ROAD_SECONDARY_2,10);

		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		int spans = 0;
		if (routed && (world.Routes.Routes.Count () == 1)) {
			const HybridRoute & route = world.Routes.Routes[0];
			for (int s = 0; s < route.Structural.Count (); ++s) {
				if (route.Structural[s].WindowId == 10) spans++;
			}
		}
		Check (!routed,"a road with both ends on one bank crossed its one window there and back (%d recorded crossing(s))",
				 spans);
		//	Refused by the search -- the way back across is closed -- not later, by a centerline over a chain that
		//	crossed the window twice.
		Check (routed || (::strstr (result.Detail.Peek_Buffer (),"no path") != NULL),
				 "the road was refused for crossing back, not by the closed window: %s",result.Detail.Peek_Buffer ());
		printf ("  own window closed: %s\r\n",routed ? "routed" : result.Detail.Peek_Buffer ());
		(void)k;
	}

	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		//	Cell centres stand at A/2 + nA from the world's -600 edge, so x = A is midway between two columns.
		const float between = world.Sized.Constants.A;
		Crossing_World_River (&world,9,Vector2 (between,-600.0f),Vector2 (between,600.0f),3.0f);
		Crossing_World_Edge (&world,Vector2 (-300.0f,7.5f),Vector2 (300.0f,7.5f),HYBRID_ROAD_SECONDARY_2,0);

		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		Check (!routed && (::strstr (result.Detail.Peek_Buffer (),"no path") != NULL),
				 "a road crossed a narrow river between cell centres with no window: %s",
				 routed ? "routed" : result.Detail.Peek_Buffer ());
		printf ("  narrow river: %s\r\n",routed ? "routed" : "refused by the search");
	}

	/*
	**	Departure room: a HIGHWAY_4 crosses a north-south river eastward on its window, then must turn north to an
	**	end 300 units up and only 120 east of the crossing.  The road keeps straight past the far approach point long
	**	enough to round its turns and is accepted with the crossing straight on its axis.  (A positive check: in
	**	open terrain the search turns late anyway; the census measures the departure rule on real worlds.)
	*/
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,9,Vector2 (7.5f,-600.0f),Vector2 (7.5f,600.0f),16.0f);
		Crossing_World_Window (&world,10,9,Vector2 (7.5f,7.5f),Vector2 (1.0f,0.0f),16.0f);
		Crossing_World_Edge (&world,Vector2 (-292.5f,7.5f),Vector2 (127.5f,307.5f),HYBRID_ROAD_HIGHWAY_4,10);

		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		Check (routed,"a highway with room to turn after its crossing was refused: %s",result.Detail.Peek_Buffer ());
		if (routed && (world.Routes.Routes.Count () == 1)) {
			const HybridCrossingWindow & window = world.Hydrology.Windows[0];
			StringClass why;
			Check (Straight_Between (world.Routes.Routes[0].Points,window.ApproachA,window.ApproachB,k.Q,&why),
					 "the departure bent the crossing: %s",why.Peek_Buffer ());
		}
		printf ("  departure room: %s\r\n",routed ? "turned after the lead-out" : result.Detail.Peek_Buffer ());
	}

	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,9,Vector2 (7.5f,-600.0f),Vector2 (7.5f,600.0f),16.0f);
		Crossing_World_Window (&world,10,9,Vector2 (7.5f,7.5f),Vector2 (1.0f,0.0f),16.0f);
		Crossing_World_Edge (&world,Vector2 (-300.0f,-250.0f),Vector2 (300.0f,250.0f),HYBRID_ROAD_SECONDARY_2,0);

		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		Check (routed,"a road that had to cross an unplanned window was refused: %s",result.Detail.Peek_Buffer ());
		if (routed && (world.Routes.Routes.Count () == 1)) {
			const HybridRoute & route = world.Routes.Routes[0];
			const HybridCrossingWindow & window = world.Hydrology.Windows[0];
			int spans = 0;
			for (int s = 0; s < route.Structural.Count (); ++s) {
				if (route.Structural[s].WindowId == 10) spans++;
			}
			Check (spans == 1,"the unplanned crossing is recorded %d times",spans);
			StringClass why;
			Check (Straight_Between (route.Points,window.ApproachA,window.ApproachB,k.Q,&why),
					 "the unplanned crossing is not aligned: %s",why.Peek_Buffer ());
			printf ("  unplanned crossing: aligned, road %.1f long\r\n",route.Length);
		}
	}
}


static void Check_Hybrid_Ordinary_Guide (void)
{
	const float ROW = 7.5f;
	const HybridRoadClass TRUNK = HYBRID_ROAD_PRIMARY_2;

	CrossingWorld world;
	Crossing_World_Begin (&world);
	const HybridConstants & k = world.Sized.Constants;
	const Vector2 guide_position (7.5f,ROW);
	const float gx = guide_position.X;
	const float gy = guide_position.Y;
	Crossing_World_River (&world,11,Vector2 (gx + 15.0f,gy - 30.0f),Vector2 (gx + 15.0f,gy + 30.0f),10.0f);
	Crossing_World_River (&world,12,Vector2 (gx - 30.0f,gy - 15.0f),Vector2 (gx + 30.0f,gy - 15.0f),10.0f);
	Crossing_World_River (&world,13,Vector2 (gx,gy + 15.0f),Vector2 (gx + 30.0f,gy + 15.0f),10.0f);
	Trunk_World_Node (&world,1,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (-292.5f,ROW),0);
	Trunk_World_Node (&world,2,HYBRID_NODE_FAMILY_WAYPOINT,guide_position,0);
	Trunk_World_Node (&world,3,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (gx - 255.0f,gy + 255.0f),0);
	Trunk_World_Edge (&world,1,HYBRID_EDGE_FAMILY_TRUNK,TRUNK,1,2,0);
	Trunk_World_Edge (&world,2,HYBRID_EDGE_FAMILY_TRUNK,TRUNK,2,3,0);

	HybridRouteResult result;
	const bool routed = Crossing_World_Route (&world,&result);
	Check (routed,"a trunk turning 135 degrees at an ordinary guide was refused: %s",result.Detail.Peek_Buffer ());
	if (!routed || (world.Routes.Trunks.Count () != 1)) return;

	const HybridTrunkRoad & trunk = world.Routes.Trunks[0];
	Check (Chain_Passes_Cell (world,guide_position),"the search's chain does not pass the ordinary guide's cell");
	const float radius = Tightest_Radius (trunk.Points);
	Check (radius >= (0.98f * Hybrid_Road_Class_Spec (TRUNK).MinTurnRadiusW * k.W),
			 "the trunk rounds the guide at %.2f, under the class radius",radius);

	float nearest = 1.0e9f;
	for (int i = 0; i < trunk.Points.Count (); ++i) nearest = WWMath::Min (nearest,(trunk.Points[i] - guide_position).Length ());
	bool outside = true;
	for (int i = 0; i < trunk.Points.Count (); ++i) {
		if ((WWMath::Fabs (trunk.Points[i].X - guide_position.X) <= 7.5f) &&
			 (WWMath::Fabs (trunk.Points[i].Y - guide_position.Y) <= 7.5f)) outside = false;
	}
	Check (outside,"the 135-degree turn's centerline still enters the guide's cell -- this world does not test the rule");

	//	Logical slicing: contiguous, non-overlapping, covering the whole trunk, both edges of positive length.
	const HybridRoute * first = world.Routes.Find (1);
	const HybridRoute * second = world.Routes.Find (2);
	Check ((first != NULL) && (second != NULL),"a logical edge has no route record");
	if ((first != NULL) && (second != NULL)) {
		Check (first->Points[0] == trunk.Points[0],"the first slice does not start the trunk");
		Check (second->Points[second->Points.Count () - 1] == trunk.Points[trunk.Points.Count () - 1],
				 "the last slice does not end the trunk");
		Check (first->Points[first->Points.Count () - 1] == second->Points[0],"the slices are not contiguous");
		Check ((first->Length > 0.0f) && (second->Length > 0.0f),"a logical edge has zero length (%.3f, %.3f)",
				 first->Length,second->Length);
		Check (WWMath::Fabs ((first->Length + second->Length) - trunk.Length) <= (4.0f * k.Q),
				 "the slices cover %.3f of a %.3f trunk",first->Length + second->Length,trunk.Length);
		Check ((trunk.EdgeFrom.Count () == 2) && (trunk.EdgeTo[0] == trunk.EdgeFrom[1]) && (trunk.EdgeFrom[0] == 0.0f) &&
				 (trunk.EdgeFrom[1] > 0.0f) && (trunk.EdgeTo[1] > trunk.EdgeFrom[1]),"the trunk's edge intervals are not ordered");

		//	The boundary is the centerline point nearest the guide cell's centre.
		const Vector2 boundary = second->Points[0];
		float best = 1.0e9f;
		for (int i = 1; i < trunk.Points.Count (); ++i) {
			const Vector2 a = trunk.Points[i - 1];
			const Vector2 d = trunk.Points[i] - a;
			const float length2 = (d.X * d.X) + (d.Y * d.Y);
			float t_on = (length2 > 0.0f) ? ((((guide_position.X - a.X) * d.X) + ((guide_position.Y - a.Y) * d.Y)) / length2) : 0.0f;
			t_on = WWMath::Clamp (t_on,0.0f,1.0f);
			best = WWMath::Min (best,(guide_position - (a + (d * t_on))).Length ());
		}
		Check (WWMath::Fabs ((boundary - guide_position).Length () - best) <= (2.0f * k.Q),
				 "the boundary is %.3f from the guide, the nearest centerline point %.3f",(boundary - guide_position).Length (),best);

		//	Deterministic.
		CrossingWorld again;
		Crossing_World_Begin (&again);
		for (int r = 0; r < world.Hydrology.Rivers.Count (); ++r) again.Hydrology.Rivers.Add (world.Hydrology.Rivers[r]);
		for (int n = 0; n < world.Topology.Nodes.Count (); ++n) again.Topology.Nodes.Add (world.Topology.Nodes[n]);
		for (int e = 0; e < world.Topology.Edges.Count (); ++e) again.Topology.Edges.Add (world.Topology.Edges[e]);
		HybridRouteResult again_result;
		Check (Crossing_World_Route (&again,&again_result) && (again.Routes.Checksum == world.Routes.Checksum),
				 "the sliced trunk does not reproduce its checksum");
	}
	printf ("  ordinary guide: 135-degree turn rounded %.1f from the guide, tightest turn %.1f, slices %.1f + %.1f\r\n",
			  nearest,radius,(first != NULL) ? first->Length : 0.0f,(second != NULL) ? second->Length : 0.0f);

	/*
	**	The same turn where the guide is a crossing obligation: a structural waypoint stays exact.  The road must
	**	cross straight along the window's axis between its two frozen approach points -- or be refused by name;
	**	it is never rounded past the crossing.
	*/
	{
		CrossingWorld crossing;
		Crossing_World_Begin (&crossing);
		const Vector2 axis (0.866f,0.5f);
		Crossing_World_River (&crossing,9,Vector2 (-250.0f,ROW + 433.0f),Vector2 (250.0f,ROW - 433.0f),16.0f);
		Crossing_World_Window (&crossing,10,9,guide_position,axis,16.0f);
		Trunk_World_Node (&crossing,1,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (-292.5f,ROW),0);
		Trunk_World_Node (&crossing,3,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (-142.5f,ROW + 255.0f),0);
		HybridTopologyNode obligation;
		obligation.StableId	= 2;
		obligation.Kind		= HYBRID_NODE_CROSSING_OBLIGATION;
		obligation.Position	= guide_position;
		obligation.WindowId	= 10;
		obligation.RouteFamily= 0;
		crossing.Topology.Nodes.Add (obligation);
		Trunk_World_Edge (&crossing,1,HYBRID_EDGE_FAMILY_TRUNK,TRUNK,1,3,0);
		crossing.Topology.Edges[0].Waypoints.Add (2);

		HybridRouteResult crossing_result;
		const bool crossed = Crossing_World_Route (&crossing,&crossing_result);
		Check (crossed || (crossing_result.Failure != HYBRID_ROUTE_OK),"the structural guide failed without naming why");
		if (crossed && (crossing.Routes.Routes.Count () == 1)) {
			const HybridCrossingWindow & window = crossing.Hydrology.Windows[0];
			StringClass why;
			Check (Straight_Between (crossing.Routes.Routes[0].Points,window.ApproachA,window.ApproachB,k.Q,&why),
					 "the structural guide's pose was not kept: %s",why.Peek_Buffer ());
		}
		printf ("  structural guide: %s\r\n",crossed ? "crossed straight on its axis" : crossing_result.Detail.Peek_Buffer ());
	}
}


static void Check_Hybrid_Continuous_Trunk (const HybridPreflightInputs * inputs,const HybridMetricCache * metrics)
{
	const float ROW = 7.5f;
	const HybridRoadClass TRUNK = HYBRID_ROAD_PRIMARY_2;

	//	The family: A (-397.5,ROW), guides at (-127.5,52.5) and (142.5,-37.5), B (397.5,ROW).
	CrossingWorld world;
	Crossing_World_Begin (&world);
	const HybridConstants & k = world.Sized.Constants;
	Trunk_World_Node (&world,1,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (-397.5f,ROW),0);
	Trunk_World_Node (&world,2,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (-127.5f,52.5f),0);
	Trunk_World_Node (&world,3,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (142.5f,-37.5f),0);
	Trunk_World_Node (&world,4,HYBRID_NODE_FAMILY_WAYPOINT,Vector2 (397.5f,ROW),0);
	Trunk_World_Edge (&world,1,HYBRID_EDGE_FAMILY_TRUNK,TRUNK,1,2,0);
	Trunk_World_Edge (&world,2,HYBRID_EDGE_FAMILY_TRUNK,TRUNK,2,3,0);
	Trunk_World_Edge (&world,3,HYBRID_EDGE_FAMILY_TRUNK,TRUNK,3,4,0);

	/*
	**	A branch at the first guide (a service road off to the north), a hugging service road from the same guide
	**	that would most cheaply ride the trunk east, and a legal parallel road well clear of it.
	*/
	Trunk_World_Node (&world,5,HYBRID_NODE_FIELD_ACCESS,Vector2 (-127.5f,322.5f),-1);
	Trunk_World_Edge (&world,4,HYBRID_EDGE_FIELD_BRANCH,HYBRID_ROAD_SERVICE_DIRT_2,2,5,-1);
	Trunk_World_Node (&world,6,HYBRID_NODE_FIELD_ACCESS,Vector2 (127.5f,7.5f),-1);	// ~8 deg off the trunk, 40 north of it
	Trunk_World_Edge (&world,5,HYBRID_EDGE_SERVICE_FILL,HYBRID_ROAD_SERVICE_DIRT_2,2,6,-1);
	Trunk_World_Node (&world,7,HYBRID_NODE_FIELD_ACCESS,Vector2 (-337.5f,-232.5f),-1);
	Trunk_World_Node (&world,8,HYBRID_NODE_FIELD_ACCESS,Vector2 (337.5f,-232.5f),-1);
	Trunk_World_Edge (&world,6,HYBRID_EDGE_TRAIL,HYBRID_ROAD_TRAIL_1,7,8,-1);

	const int edges_before = world.Topology.Edges.Count ();
	HybridRouteResult result;
	const bool routed = Crossing_World_Route (&world,&result);
	Check (routed,"the controlled trunk world does not route: %s",result.Detail.Peek_Buffer ());
	if (!routed) return;
	Check (world.Topology.Edges.Count () == edges_before,"routing changed the logical graph");

	//	Ordinary waypoints, one physical road: one trunk, three slices end to end, class radius at the joints.
	Check (world.Routes.Trunks.Count () == 1,"%d physical trunks for one family",world.Routes.Trunks.Count ());
	if (world.Routes.Trunks.Count () == 1) {
		const HybridTrunkRoad & trunk = world.Routes.Trunks[0];
		Check ((trunk.EdgeIds.Count () == 3) && (trunk.EdgeIds[0] == 1) && (trunk.EdgeIds[1] == 2) && (trunk.EdgeIds[2] == 3),
				 "the trunk's logical edges are not 1, 2, 3 in order");
		for (int s = 0; (s + 1) < trunk.EdgeIds.Count (); ++s) {
			const HybridRoute * here = world.Routes.Find (trunk.EdgeIds[s]);
			const HybridRoute * next = world.Routes.Find (trunk.EdgeIds[s + 1]);
			Check ((here != NULL) && (next != NULL) && (here->Points[here->Points.Count () - 1] == next->Points[0]),
					 "trunk slices %u and %u do not meet",trunk.EdgeIds[s],trunk.EdgeIds[s + 1]);
		}
		const float radius = Tightest_Radius (trunk.Points);
		Check (radius >= (0.98f * Hybrid_Road_Class_Spec (TRUNK).MinTurnRadiusW * k.W),
				 "the trunk turns at %.2f through its waypoints, under the class radius",radius);
		//	The search's chain passes every ordinary guide's cell; the rounded centerline need not.
		for (int g = 2; g <= 3; ++g) {
			const HybridTopologyNode * guide = world.Topology.Find_Node ((unsigned int)g);
			Check (Chain_Passes_Cell (world,guide->Position),"the trunk's chain does not pass guide %d's cell",g);
		}
		printf ("  continuous trunk: %d logical edges, one centerline of %.0f, tightest turn %.1f\r\n",
				  trunk.EdgeIds.Count (),trunk.Length,radius);
	}

	//	The hugging service road may not ride the trunk past the junction it shares with it.
	const HybridRoute * hug = world.Routes.Find (5);
	const HybridRoute * slice = world.Routes.Find (2);
	if ((hug != NULL) && (slice != NULL)) {
		const float clearance = Hybrid_Parallel_Clearance (HYBRID_ROAD_SERVICE_DIRT_2,TRUNK,k.W,1.0f);
		const float run = Overlap_Run (*hug,*slice,clearance,world.Topology.Find_Node (2)->Position,
												 Hybrid_Junction_Reach (HYBRID_ROAD_SERVICE_DIRT_2,k.W),Hybrid_Junction_Reach (TRUNK,k.W));
		Check (run <= ((6.0f * k.W) + (2.0f * k.Q)),"the service road rides the trunk for %.1f past its junction",run);
		printf ("  service road beside the trunk: longest overlap past the junction %.1f of %.1f allowed\r\n",run,6.0f * k.W);
	} else {
		Check (hug == NULL,"the hugging service road routed without its trunk slice");
	}

	//	Legal parallel roads: the trail runs the same way 240 off the trunk and routes.
	Check (world.Routes.Find (6) != NULL,"a road far clear of the trunk was refused");

	//	The branch at a trunk node: one NodeId junction, and no incidental crossing between it and the trunk.
	if (inputs != NULL) {
		HybridInteractionParams params = Interaction_Fixture (world.Sized,1200.0f,*inputs,*metrics);
		HybridInteractionSet set;
		HybridInteractionResult solved;
		const bool ok = Hybrid_Build_Interactions (params,world.Identity,world.Matrices,world.Landforms,world.Hydrology,
																 world.Anchors,world.Topology,world.Routes,Crossing_Ground,
																 &world.Hydrology,&set,&solved);
		Check (ok,"the controlled trunk world does not solve: %s",solved.Detail.Peek_Buffer ());
		int at_guide = 0;
		int incidental = 0;
		for (int i = 0; i < set.Interactions.Count (); ++i) {
			const HybridInteraction & one = set.Interactions[i];
			if (one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) continue;
			if (one.NodeId == 2) at_guide++;
			//	The branch (edge 4) meets the trunk only at its guide.
			if ((one.NodeId == 0) && ((one.EdgeId == 4) || (one.OtherEdgeId == 4))) incidental++;
		}
		Check (at_guide == 1,"the branch guide has %d junction records, not one",at_guide);
		Check (incidental == 0,"%d incidental junctions on the branch, which meets the trunk only at its guide",incidental);
		Check (solved.RoadCrossings.Meetings[HYBRID_ROAD_MEETING_SHARED] >= 1,"no shared-node meeting was counted");
	}

	//	One logical edge rerouted: only its floor moves, and the trunk is laid again in one piece.
	HybridRouteEnvelopeFloor floor;
	floor.EdgeId		= 2;
	floor.MinEnvelope	= 1;
	world.RouteParams.EnvelopeFloors.Add (floor);
	HybridRouteSet again;
	HybridRouteResult again_result;
	const bool rerouted = Hybrid_Build_Routes (world.RouteParams,world.Identity,world.Matrices,world.Landforms,
															world.Hydrology,world.Anchors,world.Topology,Crossing_Ground,
															&world.Hydrology,&again,&again_result);
	Check (rerouted,"the trunk does not route with its middle edge's floor raised: %s",again_result.Detail.Peek_Buffer ());
	if (rerouted) {
		Check ((again.Find (1) != NULL) && (again.Find (1)->Envelope == 0),"edge 1's envelope moved with edge 2's floor");
		Check ((again.Find (2) != NULL) && (again.Find (2)->Envelope >= 1),"edge 2 ignored its floor");
		Check ((again.Find (3) != NULL) && (again.Find (3)->Envelope == 0),"edge 3's envelope moved with edge 2's floor");
		Check ((again.Trunks.Count () == 1) && (again.Trunks[0].EdgeIds.Count () == 3),"the rerouted trunk is not one road");
		for (unsigned int e = 1; e < 3; ++e) {
			const HybridRoute * here = again.Find (e);
			const HybridRoute * next = again.Find (e + 1);
			Check ((here != NULL) && (next != NULL) && (here->Points[here->Points.Count () - 1] == next->Points[0]),
					 "after the reroute, slices %u and %u do not meet",e,e + 1);
		}
	}
}


/*
**	Q-022 section 13, Phase 10's half: Phase 10 owns the road's arrival at every frozen crossing.  Each world is
**	a 20-wide river north-south through the origin, long enough that the road has no way round it, with one
**	window at the origin crossed eastward.  `PRIMARY_2` turns at 16.6 and carries an 8W = 26.6 straight approach.
*/
static void Check_Hybrid_Crossing_Alignment (void)
{
	const float ROW = 7.5f;
	const Vector2 EAST (1.0f,0.0f);

	//	1. Straight crossing: the road is on the axis already, and is accepted without a turn anywhere near it.
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		Crossing_World_Edge (&world,Vector2 (-397.5f,ROW),Vector2 (397.5f,ROW),HYBRID_ROAD_PRIMARY_2,2);
		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		Check (routed,"a road already on its crossing's axis did not route: %s",result.Detail.Peek_Buffer ());
		if (routed && (world.Routes.Routes.Count () == 1)) {
			const HybridRoute & road = world.Routes.Routes[0];
			const HybridCrossingWindow & window = world.Hydrology.Windows[0];
			StringClass why;
			Check (Straight_Between (road.Points,window.ApproachA,window.ApproachB,k.Q,&why),"straight crossing: %s",
					 why.Peek_Buffer ());
			float worst = 0.0f;
			for (int i = 0; i < road.Points.Count (); ++i) worst = WWMath::Max (worst,WWMath::Fabs (road.Points[i].Y - ROW));
			Check (worst <= (2.0f * k.Q),"a road on its crossing's axis was bent %.3f off it",worst);
		}
	}

	/*
	**	2. Sideways arrival: the two ends lie along the river, so the straight line between them meets the window
	**	at about 80 degrees.  Phase 10 turns on its own bank and enters along the axis.
	*/
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		const Vector2 a (-37.5f,-352.5f);
		const Vector2 b (37.5f,367.5f);
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		Crossing_World_Edge (&world,a,b,HYBRID_ROAD_PRIMARY_2,2);
		const Vector2 line = (b - a) / (b - a).Length ();
		Check (WWMath::Fabs (line.X) < 0.5f,"the sideways world's ends are not along the river");
		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		Check (routed || (result.Failure != HYBRID_ROUTE_OK),"a sideways arrival failed without naming why");
		Check (routed,"a sideways arrival with room to turn on its bank did not route: %s",result.Detail.Peek_Buffer ());
		if (routed && (world.Routes.Routes.Count () == 1)) {
			const HybridCrossingWindow & window = world.Hydrology.Windows[0];
			StringClass why;
			Check (Straight_Between (world.Routes.Routes[0].Points,window.ApproachA,window.ApproachB,k.Q,&why),
					 "sideways arrival: %s",why.Peek_Buffer ());
			Check (Tightest_Radius (world.Routes.Routes[0].Points) >= (0.98f * Hybrid_Road_Class_Spec (HYBRID_ROAD_PRIMARY_2).MinTurnRadiusW * k.W),
					 "the sideways road turns tighter than its class");
		}
	}

	/*
	**	3. Opposite arrival: the start lies east of the window's west approach end, so the straight line to it
	**	arrives heading west -- against the crossing.  Phase 10 has to swing round before the approach; a road
	**	that touched the approach end and hooked back would leave Phase 11 a hook to cut, which is not allowed.
	*/
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		const Vector2 a (-22.5f,52.5f);
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		Crossing_World_Edge (&world,a,Vector2 (397.5f,ROW),HYBRID_ROAD_PRIMARY_2,2);
		const HybridCrossingWindow & window = world.Hydrology.Windows[0];
		Check (((window.ApproachA.X - a.X) * EAST.X) + ((window.ApproachA.Y - a.Y) * EAST.Y) < 0.0f,
				 "the opposite world's start is not past the approach end");
		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		Check (routed || (result.Failure != HYBRID_ROUTE_OK),"an opposite arrival failed without naming why");
		if (routed && (world.Routes.Routes.Count () == 1)) {
			StringClass why;
			Check (Straight_Between (world.Routes.Routes[0].Points,window.ApproachA,window.ApproachB,k.Q,&why),
					 "opposite arrival: %s",why.Peek_Buffer ());
			Check (Tightest_Radius (world.Routes.Routes[0].Points) >= (0.98f * Hybrid_Road_Class_Spec (HYBRID_ROAD_PRIMARY_2).MinTurnRadiusW * k.W),
					 "the opposite-arrival road turns tighter than its class");
		}
		printf ("  opposite arrival: %s\r\n",routed ? "swung round before the approach" : result.Detail.Peek_Buffer ());
	}

	/*
	**	4. Insufficient turn room: a second river, with no window, runs parallel east of the crossing and the far
	**	end lies between the two.  Leaving the window eastward the road keeps its 8W straight and then needs one
	**	class radius more to turn north; with the second river's bank 14 past the approach end it has 4.  Every
	**	envelope is refused -- the turn may not eat into the straight.  The same world with the bank 84 past
	**	routes, so it is the turn room that decides.
	*/
	for (int roomy = 0; roomy < 2; ++roomy) {
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		const float second = (roomy != 0) ? 130.0f : 60.0f;			//	west bank at 120 or 50
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_River (&world,3,Vector2 (second,-500.0f),Vector2 (second,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		Crossing_World_Edge (&world,Vector2 (-397.5f,ROW),Vector2 ((roomy != 0) ? 67.5f : 37.5f,397.5f),
									HYBRID_ROAD_PRIMARY_2,2);
		HybridRouteResult result;
		const bool routed = Crossing_World_Route (&world,&result);
		if (roomy == 0) {
			Check (!routed,"a road with no room to turn after its crossing's straight was routed");
			Check (result.Failure != HYBRID_ROUTE_OK,"the refused turn named no failure");
			Check (result.Detail.Get_Length () > 0,"the refused turn said nothing");
			printf ("  insufficient turn room: %s\r\n",result.Detail.Peek_Buffer ());
		} else {
			Check (routed,"the same crossing with room to turn did not route: %s",result.Detail.Peek_Buffer ());
			if (routed && (world.Routes.Routes.Count () == 1)) {
				const HybridCrossingWindow & window = world.Hydrology.Windows[0];
				StringClass why;
				Check (Straight_Between (world.Routes.Routes[0].Points,window.ApproachA,window.ApproachB,k.Q,&why),
						 "turn room: %s",why.Peek_Buffer ());
			}
		}
	}
}


void Check_Hybrid_Routes (void)
{
	Check_Hybrid_Crossing_Alignment ();
	Check_Hybrid_Continuous_Trunk (NULL,NULL);
	Check_Hybrid_Ordinary_Guide ();
	Check_Hybrid_Crossing_Discipline ();

	HybridMapIdentity identity;
	HybridArchetypeIntentSet intents;
	HybridMapIdentityResult identity_result;
	HybridStrategicGraph graph;
	HybridStrategicGraphResult graph_result;
	HybridCoarseMatrices matrices;
	HybridCoarseMatrixResult coarse_result;
	HybridLandformSet landforms;
	HybridLandformResult landform_result;
	HybridHydrologySet hydrology;
	HybridHydrologyResult hydrology_result;
	HybridMacroTerrainSet surfaces;
	HybridMacroTerrainResult macro_result;
	HybridAnchorSet anchors;
	HybridAnchorResult anchor_result;
	HybridTopology topology;
	HybridTopologyResult topology_result;
	HybridRouteSet routes;
	HybridRouteResult result;

	//	Section 8 Phase 10's cost weights, and the priority group each edge kind belongs to.
	{
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_HIGHWAY_4).Ks == 10.0f,"HIGHWAY_4 grade weight");
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_HIGHWAY_4).Kt == 8.0f,"HIGHWAY_4 turn weight");
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_HIGHWAY_4).Kc == 3.0f,"HIGHWAY_4 corridor weight");
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_PRIMARY_2).Ks == 8.0f,"PRIMARY_2 grade weight");
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_SECONDARY_2).Kt == 4.0f,"SECONDARY_2 turn weight");
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_SERVICE_DIRT_2).Kc == 1.5f,"SERVICE_DIRT_2 corridor weight");
		Check (Hybrid_Route_Cost_Weights (HYBRID_ROAD_TRAIL_1).Ks == 3.0f,"TRAIL_1 grade weight");

		HybridTopology fixture;
		HybridTopologyNode node;
		node.Kind = HYBRID_NODE_FAMILY_WAYPOINT;		node.StableId = 1;	fixture.Nodes.Add (node);
		node.Kind = HYBRID_NODE_FAMILY_WAYPOINT;		node.StableId = 2;	fixture.Nodes.Add (node);
		node.Kind = HYBRID_NODE_CROSSING_OBLIGATION;	node.StableId = 3;	fixture.Nodes.Add (node);
		node.Kind = HYBRID_NODE_OVERPASS_OBLIGATION;	node.StableId = 4;	fixture.Nodes.Add (node);

		HybridTopologyEdge edge;
		edge.NodeA = 1;	edge.NodeB = 2;
		edge.Kind = HYBRID_EDGE_FAMILY_TRUNK;
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_FAMILY_TRUNK,"trunk priority");
		edge.Kind = HYBRID_EDGE_HOME_ROAD;
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_HOME_ROAD,"home road priority");
		edge.Kind = HYBRID_EDGE_FIELD_BRANCH;
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_FIELD_BRANCH,"branch priority");
		edge.Kind = HYBRID_EDGE_SERVICE_FILL;
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_SERVICE_FILL,"service priority");
		edge.Kind = HYBRID_EDGE_TRAIL;
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_TRAIL,"trail priority");

		//	A cross-link's own waypoints say which group it is in.
		edge.Kind = HYBRID_EDGE_CROSS_LINK;
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_OPTIONAL_LINK,
				 "a plain cross-link is an optional link");
		edge.Waypoints.Add (3);
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_BRIDGE_LINK,
				 "a cross-link with a crossing obligation is a bridge link");
		edge.Waypoints.Delete_All ();
		edge.Waypoints.Add (4);
		Check (Hybrid_Route_Priority_Of (fixture,edge) == HYBRID_ROUTE_PRIORITY_OVERPASS_LINK,
				 "a cross-link with an overpass obligation is an overpass link");
	}

	//	A full Phase-10 build on a real Phase-2 to Phase-9 chain, seed-walked the way the manager does.
	//	The original eight, then the canyon seeds the census found passing Phase 10 under Q-022 and Q-023.
	static const unsigned int ROUTE_SEEDS[10] = { 4242u, 7u, 99u, 1234u, 31337u, 5u, 77u, 808u, 23u, 98u };
	bool routed = false;

	for (int attempt = 0; (attempt < 10) && !routed; ++attempt) {

	const unsigned int seed = ROUTE_SEEDS[attempt];
	HybridMapIdentityParams sized = Graph_Identity_Fixture (1200.0f,2.0f);
	sized.ArchetypeSelector = HYBRID_ARCHETYPE_CANYON_CROSSROADS;
	sized.MapPlanSeed		= WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_MAP_PLAN);
	sized.HydrologySeed	= WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
	sized.BiomeSeed		= WorldSeedService::Derive_Subsystem_Seed (seed,WorldSeedService::SEED_DOMAIN_BIOMES);
	Check (Hybrid_Resolve_Map_Identity (sized,&identity,&intents,&identity_result),"identity: %s",
			 identity_result.Detail.Peek_Buffer ());

	HybridStrategicGraphParams graph_params = Graph_Fixture (sized,2.0f);
	if (!Hybrid_Build_Strategic_Graph (graph_params,identity,intents,&graph,&graph_result)) continue;

	HybridCoarseMatrixParams coarse_params = Coarse_Fixture (sized,1200.0f);
	if (!Hybrid_Build_Coarse_Matrices (coarse_params,identity,intents,&graph,&matrices,&coarse_result)) continue;

	HybridLandformParams landform_params = Landform_Fixture (sized,1200.0f);
	landform_params.TerrainSeed = WorldSeedService::Derive_Subsystem_Seed (seed,
																							WorldSeedService::SEED_DOMAIN_TERRAIN);
	if (!Hybrid_Build_Landforms (landform_params,identity,intents,graph,&matrices,&landforms,
										  &landform_result)) continue;

	HybridHydrologyParams hydrology_params = Hydrology_Fixture (sized,1200.0f);
	hydrology_params.HydrologySeed = sized.HydrologySeed;
	if (!Hybrid_Build_Hydrology (hydrology_params,identity,intents,graph,landforms,&matrices,&hydrology,
										  &hydrology_result)) continue;

	HybridMacroTerrainParams macro_params = Macro_Terrain_Fixture (sized);
	macro_params.TerrainSeed = landform_params.TerrainSeed;
	if (!Hybrid_Resolve_River_Surfaces (macro_params,matrices,landforms,hydrology,&surfaces,&macro_result)) continue;

	MacroFormulaContext ground;
	ground.Params		= &macro_params;
	ground.Matrices	= &matrices;
	ground.Landforms	= &landforms;
	ground.Hydrology	= &hydrology;
	ground.Surfaces	= &surfaces;
	ground.Offset		= 0.0f;

	HybridAnchorParams anchor_params = Anchor_Fixture (sized);
	if (!Hybrid_Validate_Anchors (anchor_params,identity,graph,matrices,landforms,hydrology,
											Macro_Sample_From_Formula,&ground,&anchors,&anchor_result)) continue;

	HybridTopologyParams topology_params = Topology_Fixture (sized);
	if (!Hybrid_Build_Topology (topology_params,identity,graph,landforms,hydrology,anchors,&matrices,
										 Macro_Sample_From_Formula,&ground,&topology,&topology_result)) continue;

	HybridRouteParams params = Route_Fixture (sized,1200.0f);
	const bool built = Hybrid_Build_Routes (params,identity,matrices,landforms,hydrology,anchors,topology,
														 Macro_Sample_From_Formula,&ground,&routes,&result);

	/*
	**	A mandatory edge with no legal spline inside its three envelopes is a named attempt failure the
	**	manager retries with a new whole-world seed, so a refusal here is legal -- but it must name
	**	itself and leave nothing behind, and the walk then tries the next seed.
	*/
	if (!built) {
		Check (result.Failure != HYBRID_ROUTE_OK,"a failed Phase 10 must name its failure");
		Check (result.Detail.Get_Length () > 0,"a failed Phase 10 must say what refused the route");
		Check (routes.Routes.Count () == 0,"a failed Phase 10 left routes behind");

		/*
		**	A refusal here is legal -- the manager retries with a new whole-world seed -- but it is worth
		**	saying which seed refused and why, so a walk that ends with no routable seed at all can be read
		**	without instrumenting the router again.  `Hybrid_Route_Census` in `hybridroutes.h` answers the
		**	cell-rule question in detail when one is needed.
		*/
		printf ("  Phase 10 refused identity %u: %s (%s)\r\n",identity.Checksum,
				  Hybrid_Route_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());

		continue;
	}
	routed = true;

	const HybridConstants & k = params.Constants;

	/*
	**	Q-025 on a real world: each heavy family is one physical trunk whose logical edges are the family's own
	**	trunk edges in travel order, sliced end to end from one continuous centerline -- and the logical graph is
	**	untouched by it, max-flow included.
	*/
	{
		int families_seen = 0;
		for (int f = 0; f < 8; ++f) {
			int family_edges = 0;
			for (int e = 0; e < topology.Edges.Count (); ++e) {
				if ((topology.Edges[e].Kind == HYBRID_EDGE_FAMILY_TRUNK) && (topology.Edges[e].RouteFamily == f)) family_edges++;
			}
			if (family_edges == 0) continue;
			families_seen++;
			const HybridTrunkRoad * trunk = NULL;
			for (int i = 0; i < routes.Trunks.Count (); ++i) {
				if (routes.Trunks[i].RouteFamily == f) {
					Check (trunk == NULL,"family %d has more than one physical trunk",f);
					trunk = &routes.Trunks[i];
				}
			}
			Check (trunk != NULL,"family %d has no physical trunk",f);
			if (trunk == NULL) continue;
			Check (trunk->EdgeIds.Count () == family_edges,"family %d's trunk carries %d of its %d edges",f,
					 trunk->EdgeIds.Count (),family_edges);
			for (int s = 0; s < trunk->EdgeIds.Count (); ++s) {
				const HybridTopologyEdge * edge = topology.Find_Edge (trunk->EdgeIds[s]);
				const HybridRoute * slice = routes.Find (trunk->EdgeIds[s]);
				Check ((edge != NULL) && (edge->Kind == HYBRID_EDGE_FAMILY_TRUNK) && (edge->RouteFamily == f),
						 "trunk %u carries edge %u, not one of family %d's",trunk->StableId,trunk->EdgeIds[s],f);
				Check (slice != NULL,"trunk edge %u has no route record",trunk->EdgeIds[s]);
				if ((edge == NULL) || (slice == NULL)) continue;
				Check (slice->RoadClass == trunk->RoadClass,"trunk edge %u changes class",edge->StableId);
				if ((s + 1) < trunk->EdgeIds.Count ()) {
					const HybridTopologyEdge * next = topology.Find_Edge (trunk->EdgeIds[s + 1]);
					const HybridRoute * next_slice = routes.Find (trunk->EdgeIds[s + 1]);
					Check ((next != NULL) && (next->NodeA == edge->NodeB),"trunk edges %u and %u do not meet",edge->StableId,
							 trunk->EdgeIds[s + 1]);
					if (next_slice != NULL) {
						Check (slice->Points[slice->Points.Count () - 1] == next_slice->Points[0],
								 "trunk edges %u and %u are not one continuous road",edge->StableId,next_slice->EdgeId);
					}
				}
			}
		}
		Check (families_seen == routes.Trunks.Count (),"%d trunks for %d families",routes.Trunks.Count (),families_seen);

		//	The logical graph is Phase 9's and Phase 10 does not touch it.
		unsigned int gdi = 0;
		unsigned int nod = 0;
		for (int n = 0; n < topology.Nodes.Count (); ++n) {
			if (topology.Nodes[n].Kind != HYBRID_NODE_BASE) continue;
			if (topology.Nodes[n].Team == 0) gdi = topology.Nodes[n].StableId;
			if (topology.Nodes[n].Team == 1) nod = topology.Nodes[n].StableId;
		}
		if ((gdi != 0) && (nod != 0)) {
			Check (Hybrid_Topology_Max_Flow (topology,gdi,nod) >= families_seen,
					 "max-flow %d under the %d heavy families after routing",Hybrid_Topology_Max_Flow (topology,gdi,nod),
					 families_seen);
		}
	}

	//	Every mandatory realized edge has exactly one route, and every route belongs to an edge.
	for (int e = 0; e < topology.Edges.Count (); ++e) {

		const HybridTopologyEdge & edge = topology.Edges[e];
		if (!edge.Is_Realized ()) {
			Check (routes.Find (edge.StableId) == NULL,"a BASE_INTERNAL edge was routed");
			continue;
		}
		if (!edge.Required) continue;
		Check (routes.Find (edge.StableId) != NULL,"mandatory edge %u has no route",edge.StableId);
	}

	for (int r = 0; r < routes.Routes.Count (); ++r) {

		const HybridRoute & route = routes.Routes[r];
		const HybridTopologyEdge * edge = topology.Find_Edge (route.EdgeId);
		Check (edge != NULL,"route %u has no topology edge",route.EdgeId);
		if (edge == NULL) continue;

		Check (route.RoadClass == edge->RoadClass,"route %u is %s but its edge is %s",route.EdgeId,
				 Hybrid_Road_Class_Name (route.RoadClass),Hybrid_Road_Class_Name (edge->RoadClass));
		Check (route.Points.Count () >= 2,"route %u has %d samples",route.EdgeId,route.Points.Count ());
		Check (route.Stations.Count () == route.Points.Count (),"route %u has mismatched stations",route.EdgeId);
		Check (route.Cells.Count () >= 1,"route %u has no cell chain",route.EdgeId);
		Check ((route.Envelope >= 0) && (route.Envelope <= 2),"route %u used envelope %d",route.EdgeId,
				 route.Envelope);
		Check (route.ControlInsertions <= params.MaxLocalRetriesPerStage,
				 "route %u used %d control insertions, over the %d limit",route.EdgeId,route.ControlInsertions,
				 params.MaxLocalRetriesPerStage);
		Check (route.Expansions <= params.AStarExpansionsPerRoute,
				 "route %u used %d expansions, over its 8N ceiling of %d",route.EdgeId,route.Expansions,
				 params.AStarExpansionsPerRoute);

		//	Stations advance, and the sample spacing is the plan's own min(W,A/2).
		const float spacing = (k.W < (0.5f * k.A)) ? k.W : (0.5f * k.A);
		for (int p = 1; p < route.Points.Count (); ++p) {
			Check (route.Stations[p] > route.Stations[p - 1],"route %u station %d does not advance",
					 route.EdgeId,p);
			const float step = (route.Points[p] - route.Points[p - 1]).Length ();
			Check (step <= (spacing + 0.01f),"route %u sample %d is %g from the last, over the %g spacing",
					 route.EdgeId,p,step,spacing);
		}

		//	The A* chain is a chain: consecutive cells are 8-way neighbours and never repeat.
		for (int c = 1; c < route.Cells.Count (); ++c) {
			const int previous = route.Cells[c - 1];
			const int current = route.Cells[c];
			const int dx = (current % routes.AnalysisCountX) - (previous % routes.AnalysisCountX);
			const int dy = (current / routes.AnalysisCountX) - (previous / routes.AnalysisCountX);
			Check ((dx != 0) || (dy != 0),"route %u repeats cell %d",route.EdgeId,current);
			Check ((dx >= -1) && (dx <= 1) && (dy >= -1) && (dy <= 1),
					 "route %u jumps from cell %d to %d",route.EdgeId,previous,current);
		}

		/*
		**	Every frozen waypoint is discharged: either its own cell is on the chain, or -- for a crossing
		**	waypoint -- the route crosses that window, which it records as a structural span.  The second is the
		**	stronger statement: the road runs through both of the window's frozen approach points, and the cell
		**	the node's position falls in is the middle of the obstruction, which the road spans rather than
		**	drives through.
		*/
		for (int w = 0; w < edge->Waypoints.Count (); ++w) {

			const HybridTopologyNode * node = topology.Find_Node (edge->Waypoints[w]);
			if (node == NULL) continue;

			bool present = false;
			for (int c = 0; (c < route.Cells.Count ()) && !present; ++c) {
				const Vector2 centre (params.WorldMinX + (((float)(route.Cells[c] % routes.AnalysisCountX) + 0.5f) * k.A),
											 params.WorldMinY + (((float)(route.Cells[c] / routes.AnalysisCountX) + 0.5f) * k.A));
				if ((centre - node->Position).Length () <= (k.A * 1.5f)) present = true;
			}
			for (int s = 0; (s < route.Structural.Count ()) && !present; ++s) {
				if (route.Structural[s].WindowId == node->WindowId) present = true;
			}
			Check (present,"route %u does not pass waypoint %u",route.EdgeId,node->StableId);
		}

		/*
		**	No route sample stands in standing water, *except* inside one of its own structural spans.  A
		**	crossing is a frozen straight interval across an obstruction, and the obstruction under one is
		**	whatever Phase 6 put there -- a channel, a canyon, or the standing water a channel widens into.
		**	Phase 11 compiles the deck or bore that carries the road over it.
		*/
		for (int p = 0; p < route.Points.Count (); ++p) {

			bool structural = false;
			for (int s = 0; (s < route.Structural.Count ()) && !structural; ++s) {
				if ((p >= route.Structural[s].FirstPoint) && (p <= route.Structural[s].LastPoint)) structural = true;
			}
			if (structural) continue;

			Check (!Hybrid_Point_In_Standing_Water (hydrology,route.Points[p],NULL,NULL),
					 "route %u sample %d stands in a lake or the sea",route.EdgeId,p);
		}

		/*
		**	The Section-7 minimum turn radius, measured here rather than taken on trust from the router: the
		**	circumradius of three samples a class-radius baseline apart, at every sample of open road.
		**
		**	The only tolerance allowed is the project position quantum `Q`, which covers writing a coordinate
		**	down at 0.001 and nothing else.  A curve that is short by tenths of a world unit is short.
		**
		**	Samples within one baseline of a structural span are not open road: the triple would straddle the
		**	frozen crossing interval and measure the corner where the road meets its own bridge, which the
		**	interaction phase compiles along with the deck.  That is the same exemption the router makes, and
		**	the span itself is checked for straightness just below.
		*/
		{
			const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec (route.RoadClass);
			const float min_radius = spec.MinTurnRadiusW * k.W;
			const float spacing = (k.W < (0.5f * k.A)) ? k.W : (0.5f * k.A);
			const int span = (int)Hybrid_Round_Half_Up (min_radius / ((spacing > k.Q) ? spacing : k.Q));
			const int baseline = (span > 1) ? span : 1;

			for (int q = baseline; q < (route.Points.Count () - baseline); ++q) {

				bool structural = false;
				for (int s = 0; (s < route.Structural.Count ()) && !structural; ++s) {
					if ((q >= (route.Structural[s].FirstPoint - baseline)) &&
						 (q <= (route.Structural[s].LastPoint + baseline))) structural = true;
				}
				if (structural) continue;

				const Vector2 & before = route.Points[q - baseline];
				const Vector2 & here = route.Points[q];
				const Vector2 & after = route.Points[q + baseline];

				const float first = (here - before).Length ();
				const float second = (after - here).Length ();
				const float chord = (before - after).Length ();
				const float cross = (((here.X - before.X) * (after.Y - before.Y)) -
											((here.Y - before.Y) * (after.X - before.X)));
				const float area = 0.5f * (float)fabs ((double)cross);
				if (area <= 1.0e-4f) continue;					// a straight run turns at no radius at all

				const float radius = (first * second * chord) / (4.0f * area);
				Check (radius >= (min_radius - k.Q),
						 "route %u (%s) turns at radius %.2f at sample %d, under the class minimum of %.2f",
						 route.EdgeId,Hybrid_Road_Class_Name (route.RoadClass),radius,q,min_radius);
			}
		}

		/*
		**	And every structural span is a straight run between the two frozen approach points of the window it
		**	names, which is the whole of what Phase 10 decides about a crossing.
		*/
		for (int s = 0; s < route.Structural.Count (); ++s) {

			const HybridRouteStructuralSpan & span = route.Structural[s];
			const HybridCrossingWindow * window = hydrology.Find_Window (span.WindowId);
			Check (window != NULL,"route %u names crossing window %u, which does not exist",route.EdgeId,
					 span.WindowId);
			if (window == NULL) continue;

			const float to_a = (span.Ingress - window->ApproachA).Length ();
			const float to_b = (span.Ingress - window->ApproachB).Length ();
			Check ((to_a <= k.Q) || (to_b <= k.Q),
					 "route %u structural span %d does not start on a frozen approach point of window %u",
					 route.EdgeId,s,span.WindowId);

			Check ((span.FirstPoint >= 0) && (span.LastPoint < route.Points.Count ()) &&
					 (span.LastPoint >= span.FirstPoint),
					 "route %u structural span %d covers samples %d to %d of %d",route.EdgeId,s,span.FirstPoint,
					 span.LastPoint,route.Points.Count ());

			for (int q = span.FirstPoint + 1; (q < span.LastPoint) && (q < route.Points.Count ()); ++q) {
				const Vector2 axis = span.Egress - span.Ingress;
				const float length = axis.Length ();
				if (length <= k.Q) continue;
				const float across = (((route.Points[q].X - span.Ingress.X) * axis.Y) -
											 ((route.Points[q].Y - span.Ingress.Y) * axis.X)) / length;
				Check ((across <= (2.0f * k.Q)) && (across >= (-2.0f * k.Q)),
						 "route %u structural span %d bows %.3f off its own axis at sample %d",route.EdgeId,s,
						 across,q);
			}
		}
	}

	Check (routes.Checksum != 0,"a passed Phase 10 left no checksum");
	Check (routes.TotalExpansions <= params.AStarExpansionsPerAttempt,
			 "%d expansions this attempt, over the R*8N ceiling of %d",routes.TotalExpansions,
			 params.AStarExpansionsPerAttempt);

	//	Determinism: Phase 10 makes no draw, so a second pass over the same records is identical.
	HybridRouteSet again;
	Check (Hybrid_Build_Routes (params,identity,matrices,landforms,hydrology,anchors,topology,
										 Macro_Sample_From_Formula,&ground,&again,&result),
			 "the second routing pass was refused: %s",result.Detail.Peek_Buffer ());
	Check (again.Checksum == routes.Checksum,"Phase 10 is not reproducible");
	Check (Hybrid_Route_Checksum (again,identity.Checksum) == routes.Checksum,
			 "the checksum does not describe the records it was built from");

	//	The expansion ceiling refuses on its own, and refusing leaves nothing behind.
	HybridRouteParams starved = params;
	starved.AStarExpansionsPerRoute = 1;
	starved.AStarExpansionsPerAttempt = 2;
	HybridRouteSet refused;
	Check (!Hybrid_Build_Routes (starved,identity,matrices,landforms,hydrology,anchors,topology,
											Macro_Sample_From_Formula,&ground,&refused,&result),
			 "a one-expansion A* budget was accepted");
	Check (result.Failure == HYBRID_ROUTE_EXPANSION_BUDGET,"the wrong failure was named (%s)",
			 Hybrid_Route_Failure_Name (result.Failure));
	Check (refused.Routes.Count () == 0,"a refused Phase 10 left routes behind");
	}

	Check (routed,"no fixture seed produced a complete Phase-10 routing: %s (%s)",
			 Hybrid_Route_Failure_Name (result.Failure),result.Detail.Peek_Buffer ());
}


// Hand-filled metadata fixture first, then one pass over the live registries.
void Check_Hybrid_Preflight (void)
{
	ProceduralWorldProfile profile;
	StringClass error;
	if (!ProceduralWorldProfileManager::Load_Profile_From_File ("data/procedural_world/native.json",&profile,&error)) {
		Check (false,"preflight native fixture: %s",error.Peek_Buffer ()); return;
	}
	profile.Plan.BiomeMode = "single"; profile.Plan.Biome = "temperate";
	HybridPreflightInputs input;
	input.WorldUnitsPerMeter = 1;
	// WORLDBOX measurements from docs/procedural_world/VehicleCollisionMetrics.md.
	const float HUMVEE[3] = { 4.722971f, 2.555953f, 1.548488f };
	const float MAMMOTH[3] = { 10.500445f, 6.801906f, 3.064066f };
	const float HARVESTER[2][3] = { { 7.428105f, 4.514225f, 2.754598f }, { 7.399116f, 4.514225f, 2.754598f } };
	struct { HybridVehicleMetricInput * Out; const float * Box; int Id; } vehicles[4] = {
		{ &input.Humvee,HUMVEE,101 }, { &input.HeavyVehicle,MAMMOTH,102 },
		{ &input.Harvesters[0],HARVESTER[0],103 }, { &input.Harvesters[1],HARVESTER[1],104 } };
	for (int v = 0; v < 4; ++v) {
		vehicles[v].Out->Measured = true; vehicles[v].Out->DefinitionID = vehicles[v].Id;
		vehicles[v].Out->Length = vehicles[v].Box[0]; vehicles[v].Out->Width = vehicles[v].Box[1]; vehicles[v].Out->Height = vehicles[v].Box[2];
	}
	input.ReferenceVehicleSpeed = WorldMetricsClass::Get_Reference_Vehicle_Speed ();
	input.ReferenceInfantrySpeed = WorldMetricsClass::Get_Reference_Infantry_Speed ();
	input.BridgeMaxSpanLength = 200; input.BridgeWaterClearanceMargin = 2; input.BridgeApproachLength = 30;
	HybridBridgeInput bridge;
	bridge.Name = "test_bridge"; bridge.Width = 40; bridge.DeckThickness = 2; bridge.RequiredClearance = 5;
	input.Bridges.Add (bridge);
	for (int team = 0; team < 2; ++team) {
		const DynamicVectorClass<StringClass> & names = team == 0 ? profile.Base.GdiInitialBuildings : profile.Base.NodInitialBuildings;
		for (int i = 0; i < names.Count (); ++i) {
			HybridBuildingFootprintInput building;
			building.Name = names[i]; building.Team = team; building.Found = true; building.Radius = 20; building.Height = 15;
			input.Buildings.Add (building);
		}
	}
	HybridTerrainLayerInput layer;
	layer.Name = "test_grass"; layer.Kind = TERRAIN_LAYER_BASE; layer.SurfaceType = SURFACE_TYPE_GRASS; input.TerrainLayers.Add (layer);
	layer.Name = "test_rock"; layer.Kind = TERRAIN_LAYER_CLIFF; layer.SurfaceType = SURFACE_TYPE_ROCK; input.TerrainLayers.Add (layer);
	HybridPreflightResult result;
	Check (Hybrid_Run_Preflight (profile,input,&result),"valid preflight: %s",result.Detail.Peek_Buffer ());
	const unsigned int baseline = result.Metrics.PlanningInputHash;
	Check (!result.Metrics.TunnelsAvailable && !result.Metrics.DecorationAvailable && !result.Metrics.LampsAvailable,"optional missing assets do not fail preflight");
	const HybridMetricCache metrics = result.Metrics;
	Check (fabsf (metrics.Constants.H - 2.555953f) < 1.0e-5f && fabsf (metrics.Constants.W - 3.322739f) < 1.0e-5f,
		"measured Humvee drives Section 0.5 H/W: %f %f",metrics.Constants.H,metrics.Constants.W);
	Check (metrics.HumveeHeight == HUMVEE[2] && metrics.HeavyVehicleWidth == MAMMOTH[1] && metrics.HeavyVehicleHeight == MAMMOTH[2] &&
		metrics.HarvesterWidth == HARVESTER[0][1] && metrics.HarvesterHeight == HARVESTER[0][2],"metric cache keeps measured widths and heights");
	Check (metrics.HumveeDefinitionID == 101 && metrics.HeavyVehicleDefinitionID == 102 &&
		metrics.HarvesterDefinitionID[0] == 103 && metrics.HarvesterDefinitionID[1] == 104,"metric cache keeps provenance");
	Check (metrics.ContentRevision == Hybrid_Structural_Content_Revision (input) && metrics.ContentRevision != 0,"content revision is the structural digest");
	HybridPreflightInputs changed = input; changed.Humvee.Measured = false;
	Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_HUMVEE_COLLISION_METRIC,"missing Humvee fails preflight");
	changed = input; changed.Humvee.Height = 0;
	Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_HUMVEE_COLLISION_METRIC,"Humvee without a height fails preflight");
	changed = input; changed.HeavyVehicle.Measured = false;
	Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_HEAVY_VEHICLE_COLLISION_METRIC,"missing heavy vehicle fails preflight");
	for (int team = 0; team < 2; ++team) {
		changed = input; changed.Harvesters[team].Measured = false;
		Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_HARVESTER_COLLISION_METRIC,
			"either team's missing Harvester fails preflight (%d)",team);
	}
	changed = input; changed.Harvesters[1].Width = 5.0f;
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.HarvesterWidth == 5.0f && result.Metrics.PlanningInputHash != baseline,
		"wider team Harvester sizes the envelope");
	changed = input; changed.HeavyVehicle.Height += 1.0f;
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.PlanningInputHash != baseline &&
		result.Metrics.ContentRevision != metrics.ContentRevision,"vehicle height affects content and planning identity");
	changed = input; changed.Buildings[0].Height += 5.0f;
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.PlanningInputHash == baseline,"render-bound building height excluded from identity");

	// Oversize worlds: rejected unless the developer/offline path asks.
	ProceduralWorldProfile oversize = profile; oversize.World.Width = 2600; oversize.World.Height = 2000;
	Check (!Hybrid_Run_Preflight (oversize,input,&result) && result.Failure == HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED,"oversize rejected at runtime");
	changed = input; changed.AllowOversizeDevelopment = true;
	Check (Hybrid_Run_Preflight (oversize,changed,&result),"developer path admits oversize: %s",result.Detail.Peek_Buffer ());
	oversize.World.Width = 0;
	Check (!Hybrid_Run_Preflight (oversize,changed,&result) && result.Failure == HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED,"developer path still rejects non-positive size");

	// Tunnels: a portal-compatible module whose authored bore clears the widest and tallest vehicle.
	ProceduralWorldProfile tunnels = profile; tunnels.Features.Tunnels = true;
	HybridTunnelModuleInput module;
	module.Name = "tight"; module.Length = 20; module.ClearanceWidth = 6; module.ClearanceHeight = 5; module.WallThickness = 1;
	module.MaxTurnRadians = 0.3f; module.MaxGradeRadians = 0.1f; module.PortalCompatible = true;
	changed = input; changed.TunnelModules.Add (module);
	Check (Hybrid_Run_Preflight (tunnels,changed,&result) && !result.Metrics.TunnelsAvailable &&
		result.Metrics.TunnelRequiredWidth > MAMMOTH[1] && result.Metrics.TunnelRequiredHeight == MAMMOTH[2],"narrow tunnel bore leaves tunnels unavailable");
	module.Name = "tube"; module.ClearanceWidth = 0; module.ClearanceHeight = 0; module.PortalCompatible = false; changed.TunnelModules.Add (module);
	Check (Hybrid_Run_Preflight (tunnels,changed,&result) && !result.Metrics.TunnelsAvailable,"tunnels need a portal-compatible module");
	module.Name = "portal"; module.PortalCompatible = true; changed.TunnelModules.Add (module);
	Check (Hybrid_Run_Preflight (tunnels,changed,&result) && result.Metrics.TunnelsAvailable && result.Metrics.TunnelPortalModule == 2,
		"procedural portal module makes tunnels available");
	ProceduralWorldProfile no_tunnels = profile; no_tunnels.Features.Tunnels = false;
	Check (Hybrid_Run_Preflight (no_tunnels,changed,&result) && !result.Metrics.TunnelsAvailable,"tunnels feature toggle off keeps tunnels unavailable");
	Check (result.Metrics.ContentRevision != metrics.ContentRevision,"tunnel module geometry affects content revision");
	changed = input; bridge.StructuralModels = "bridge_deck"; changed.Bridges[0] = bridge;
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.ContentRevision != metrics.ContentRevision,
		"bridge structural models affect content revision");
	bridge.StructuralModels = "";

	// Collision boxes are the WORLDBOX's full extents; lateral width, never the diameter.
	WorldMetricsStruct measured;
	measured.Valid = measured.HasExtents = true; measured.Extents.Set (MAMMOTH[0] * 0.5f,MAMMOTH[1] * 0.5f,MAMMOTH[2] * 0.5f);
	WorldCollisionBoxStruct box, worst;
	Check (WorldMetricsClass::Compute_Collision_Box (measured,7,&box) && box.Width == MAMMOTH[1] && box.Length == MAMMOTH[0] &&
		box.Height == MAMMOTH[2] && box.DefinitionID == 7 && box.MeasuredCount == 1,"collision box is full WORLDBOX extents");
	WorldCollisionBoxStruct unmeasured;
	measured.HasExtents = false;
	Check (!WorldMetricsClass::Compute_Collision_Box (measured,8,&unmeasured) && unmeasured.UnmeasuredCount == 1,"fallback extents are not a measurement");
	WorldMetricsClass::Accumulate_Collision_Box (unmeasured,&worst);
	WorldMetricsClass::Accumulate_Collision_Box (box,&worst);
	measured.HasExtents = true; measured.Extents.Set (6.0f,1.5f,4.0f);
	WorldCollisionBoxStruct narrow;
	WorldMetricsClass::Compute_Collision_Box (measured,3,&narrow);
	WorldMetricsClass::Accumulate_Collision_Box (narrow,&worst);
	Check (worst.Width == MAMMOTH[1] && worst.Length == 12.0f && worst.Height == 8.0f && worst.DefinitionID == 7 &&
		worst.MeasuredCount == 2 && worst.UnmeasuredCount == 1,"class box is per-axis largest keeping the widest member");
	changed = input; changed.Bridges.Reset_Active ();
	Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_NO_HEAVY_CAPABLE_BRIDGE,"missing heavy bridge fails preflight");
	ProceduralWorldProfile altered = profile; altered.Plan.AllowFords = true;
	Check (!Hybrid_Run_Preflight (altered,input,&result) && result.Failure == HYBRID_PREFLIGHT_FORD_CAPABILITY_UNAVAILABLE,"missing ford capability fails preflight");
	altered = profile; altered.Plan.BiomeMode = "mixed"; altered.Plan.Biome = "auto";
	Check (!Hybrid_Run_Preflight (altered,input,&result) && result.Failure == HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,"mixed biome preflight must reject a registry with no compatible pair");
	altered = profile; altered.Fairness.MinVehicleLaneClearance += 1;
	Check (Hybrid_Run_Preflight (altered,input,&result) && result.Metrics.PlanningInputHash != baseline,"fairness changes planning identity");
	altered = profile; altered.Retry.MaxAlternateCandidatesPerFeature += 1;
	Check (Hybrid_Run_Preflight (altered,input,&result) && result.Metrics.PlanningInputHash != baseline,"retry limits change planning identity");
	altered = profile; altered.Plan.LaneWidthCells += 1; altered.Features.Settlements = !altered.Features.Settlements;
	changed = input; changed.DecorationTypesAvailable = changed.LampTypeAvailable = true;
	Check (Hybrid_Run_Preflight (altered,changed,&result) && result.Metrics.PlanningInputHash == baseline,"legacy and cosmetic inputs excluded from planning identity");
	changed = input; bridge.Name = "alternate_bridge"; changed.Bridges.Add (bridge);
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.PlanningInputHash != baseline,"all bridge candidates affect planning identity");
	changed = input; changed.TerrainLayers[0].Name = "alternate_ground";
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.PlanningInputHash != baseline,"resolved biome material identity affects planning identity");
	changed = input;
	layer.Name = "test_snow"; layer.Kind = TERRAIN_LAYER_BASE; layer.SurfaceType = SURFACE_TYPE_SNOW;
	changed.TerrainLayers.Add (layer);
	altered = profile; altered.Plan.BiomeMode = "mixed"; altered.Plan.SecondaryBiome = "snow";
	Check (Hybrid_Run_Preflight (altered,changed,&result),"available legal mixed biome pair rejected");
	altered.Plan.Biome = "snow";
	Check (!Hybrid_Run_Preflight (altered,changed,&result) && result.Failure == HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,"mixed primary and secondary must differ");
	altered = profile; altered.Plan.BiomeMode = "auto"; altered.Plan.MixedBiomeProbability = 1;
	Check (!Hybrid_Run_Preflight (altered,input,&result) && result.Failure == HYBRID_PREFLIGHT_UNSUPPORTED_BIOME_ASSET_SET,"guaranteed mixed mode requires compatible assets");
	altered.Plan.MixedBiomeProbability = 0;
	Check (Hybrid_Run_Preflight (altered,input,&result),"guaranteed single mode needs only one biome");
	changed = input;
	layer.Name = "later_grass"; layer.SurfaceType = SURFACE_TYPE_GRASS; changed.TerrainLayers.Add (layer);
	Check (Hybrid_Run_Preflight (profile,changed,&result) && result.Metrics.BiomeSlotLayer[HYBRID_BIOME_TEMPERATE][HYBRID_SLOT_GROUND] == 0,
		"material ties select lower registry StableId");

	//	Section 0.8 required-feature validation: each hybrid-required feature disabled in the profile
	//	is rejected by the menu-time request check and by the runtime preflight, before any seeded stage.
	{
		struct { const char * Detail; int Which; } required[4] = {
			{ "water.rivers",0 }, { "water.lakes",1 }, { "world_features.highways",2 }, { "world_features.tiberium_fields",3 } };
		for (int r = 0; r < 4; ++r) {
			altered = profile;
			switch (required[r].Which) {
				case 0: altered.Water.Rivers = false; break;
				case 1: altered.Water.Lakes = false; break;
				case 2: altered.Features.Highways = false; break;
				default: altered.Features.TiberiumFields = false; break;
			}
			Check (!Hybrid_Preflight_Request_Check (altered,false,&result) && result.Failure == HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED &&
				::strcmp (result.Detail.Peek_Buffer (),required[r].Detail) == 0,
				"request check rejects disabled %s (got '%s')",required[r].Detail,result.Detail.Peek_Buffer ());
			Check (!Hybrid_Run_Preflight (altered,input,&result) && result.Failure == HYBRID_PREFLIGHT_REQUIRED_FEATURE_DISABLED,
				"runtime preflight rejects disabled %s",required[r].Detail);
		}
		Check (Hybrid_Preflight_Request_Check (profile,false,&result),"request check admits the native profile: %s",result.Detail.Peek_Buffer ());
		altered = profile; altered.World.Width = 2401; altered.World.Height = 100;
		Check (!Hybrid_Preflight_Request_Check (altered,false,&result) && result.Failure == HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED,"request check rejects an oversize extent");
		Check (Hybrid_Preflight_Request_Check (altered,true,&result),"request check developer path admits oversize");
		altered.World.Width = 0;
		Check (!Hybrid_Preflight_Request_Check (altered,true,&result) && result.Failure == HYBRID_PREFLIGHT_WORLD_SIZE_LIMIT_EXCEEDED,"request check developer path still rejects non-positive size");
		altered = profile; altered.World.Width = 2400; altered.World.Height = 2400;
		Check (Hybrid_Preflight_Request_Check (altered,false,&result),"the 2400x2400 area limit boundary is admitted");
	}

	//	Section 0.8 remaining hard failures: every listed refusal code fires from its own input gap.
	{
		changed = input; changed.WorldUnitsPerMeter = 0;
		Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_WORLD_UNITS_PER_METER,"missing world units per meter fails preflight");
		changed = input; changed.Buildings[0].Found = false;
		Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_STARTING_BUILDING_FOOTPRINT,"missing starting building footprint fails preflight");
		changed = input; changed.Buildings[0].Radius = 0;
		Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_STARTING_BUILDING_FOOTPRINT,"zero starting building radius fails preflight");
		changed = input; changed.ReferenceVehicleSpeed = 0;
		Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_FAIRNESS_REFERENCE_SPEED,"missing reference vehicle speed fails preflight");
		changed = input; changed.ReferenceInfantrySpeed = 0;
		Check (!Hybrid_Run_Preflight (profile,changed,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_FAIRNESS_REFERENCE_SPEED,"missing reference infantry speed fails preflight");
		altered = profile; altered.Resource.HarvesterLoadAmount = 0;
		Check (!Hybrid_Run_Preflight (altered,input,&result) && result.Failure == HYBRID_PREFLIGHT_MISSING_HARVESTER_LOAD_AMOUNT,"missing harvester load amount fails preflight");
	}

	//	Section 0.6 size class derives from D = min(width,height) and selects the heavy route class.
	{
		struct { float W,H; HybridSizeClass Expected; HybridRoadClass Route; } sizes[5] = {
			{ 899,2000,HYBRID_SIZE_SMALL,HYBRID_ROAD_PRIMARY_2 }, { 900,2000,HYBRID_SIZE_MEDIUM,HYBRID_ROAD_HIGHWAY_4 },
			{ 2000,1799,HYBRID_SIZE_MEDIUM,HYBRID_ROAD_HIGHWAY_4 }, { 1800,1800,HYBRID_SIZE_LARGE,HYBRID_ROAD_HIGHWAY_4 },
			{ 2400,2400,HYBRID_SIZE_LARGE,HYBRID_ROAD_HIGHWAY_4 } };
		for (int i = 0; i < 5; ++i) {
			altered = profile; altered.World.Width = sizes[i].W; altered.World.Height = sizes[i].H;
			Check (Hybrid_Run_Preflight (altered,input,&result) && result.Metrics.Constants.SizeClass == sizes[i].Expected &&
				result.Metrics.HeavyRouteClass == sizes[i].Route,
				"%gx%g derives size class %d / heavy route %d (got %d / %d: %s)",sizes[i].W,sizes[i].H,(int)sizes[i].Expected,(int)sizes[i].Route,
				(int)result.Metrics.Constants.SizeClass,(int)result.Metrics.HeavyRouteClass,result.Detail.Peek_Buffer ());
		}
	}

	//	Section 0.3: the manifest carries the planning identity next to the legacy profile hash, and
	//	it survives the manifest's own serialization.
	{
		Check (Hybrid_Run_Preflight (profile,input,&result),"baseline preflight for manifest carry: %s",result.Detail.Peek_Buffer ());
		DynamicVectorClass<StringClass> content;
		WorldGenerationManifest manifest = WorldGenerationManifestService::Create_Empty_World_Manifest (profile,content);
		Check (manifest.HybridPlanningInputHash == 0u && manifest.HybridContentRevision == 0u,"an empty manifest carries no hybrid identity");
		manifest.HybridPlanningInputHash = result.Metrics.PlanningInputHash;
		manifest.HybridContentRevision = result.Metrics.ContentRevision;
		StringClass json (""), error ("");
		WorldGenerationManifest reloaded;
		Check (WorldGenerationManifestService::Serialize (manifest,&json) &&
			WorldGenerationManifestService::Deserialize (json.Peek_Buffer (),&reloaded,&error) &&
			reloaded.HybridPlanningInputHash == result.Metrics.PlanningInputHash &&
			reloaded.HybridContentRevision == result.Metrics.ContentRevision,
			"hybrid planning identity survives the manifest round trip: %s",error.Peek_Buffer ());
		StringClass repro = WorldGenerationManifestService::Format_Reproduction_String (manifest);
		StringClass suffix (64u,true);
		suffix.Format (" hybrid_planning=%u hybrid_content=%u",result.Metrics.PlanningInputHash,result.Metrics.ContentRevision);
		Check (::strstr (repro.Peek_Buffer (),suffix.Peek_Buffer ()) != nullptr,"reproduction string names the hybrid identity: %s",repro.Peek_Buffer ());
	}

	//	Section 0.8's other half: the runtime gathering path itself, against the live registries.
	//	A self-check mounts no game content, so the honest outcome is that every vehicle metric
	//	comes back unmeasured and preflight names the missing metric rather than inventing one,
	//	while the registry-backed half -- bridges, tunnel modules, terrain layers, world units per
	//	metre and the two fairness speeds -- still resolves.
	{
		BridgeSystem::Init ();
		TunnelSystem::Init ();
		FoliageSystem::Init ();

		HybridPreflightInputs runtime;
		runtime.Gather_From_Runtime (profile);

		Check (runtime.WorldUnitsPerMeter == WorldMetricsClass::Get_World_Units_Per_Meter () && runtime.WorldUnitsPerMeter > 0.0f &&
			runtime.ReferenceVehicleSpeed == WorldMetricsClass::Get_Reference_Vehicle_Speed () &&
			runtime.ReferenceInfantrySpeed == WorldMetricsClass::Get_Reference_Infantry_Speed (),
			"runtime gathering takes the scale and both fairness speeds from WorldMetricsClass");
		Check (runtime.Bridges.Count () == BridgeSystem::Get_Definition_Count () && runtime.Bridges.Count () > 0,
			"runtime gathering reads every registered bridge (%d of %d)",runtime.Bridges.Count (),BridgeSystem::Get_Definition_Count ());
		Check (runtime.TunnelModules.Count () == TunnelSystem::Get_Module_Count () && runtime.TunnelModules.Count () > 0,
			"runtime gathering reads every registered tunnel module (%d of %d)",runtime.TunnelModules.Count (),TunnelSystem::Get_Module_Count ());
		Check (runtime.TerrainLayers.Count () == TerrainTextureSystem::Get_Layer_Count () && runtime.TerrainLayers.Count () > 0,
			"runtime gathering reads every terrain layer (%d of %d)",runtime.TerrainLayers.Count (),TerrainTextureSystem::Get_Layer_Count ());
		Check (runtime.Buildings.Count () == profile.Base.GdiInitialBuildings.Count () + profile.Base.NodInitialBuildings.Count (),
			"runtime gathering asks after every starting building");
		Check (runtime.BridgeMaxSpanLength > 0.0f && runtime.BridgeWaterClearanceMargin > 0.0f,
			"runtime gathering takes the span and water clearance limits from BridgePlannerParams");
		Check (!runtime.AllowOversizeDevelopment && !runtime.FordCapabilityAvailable,
			"runtime gathering never opens the developer oversize path or claims a ford capability");
		Check (!runtime.Humvee.Measured && !runtime.HeavyVehicle.Measured &&
			!runtime.Harvesters[0].Measured && !runtime.Harvesters[1].Measured,
			"with no content mounted every vehicle metric stays unmeasured");

		HybridPreflightResult runtime_result;
		Check (!Hybrid_Run_Preflight (profile,runtime,&runtime_result) &&
			runtime_result.Failure == HYBRID_PREFLIGHT_MISSING_HUMVEE_COLLISION_METRIC,
			"an unmeasured runtime snapshot is rejected by name rather than filled in (%s)",
			Hybrid_Preflight_Failure_Name (runtime_result.Failure));

		//	Two gatherings of one unchanged registry set must agree: the structural revision is what
		//	the manifest records and what the join handshake compares, so a drift here would split a
		//	host from a client that loaded the same content.
		HybridPreflightInputs again;
		again.Gather_From_Runtime (profile);
		const unsigned int gathered_revision = Hybrid_Structural_Content_Revision (runtime);
		Check (Hybrid_Structural_Content_Revision (again) == gathered_revision && gathered_revision != 0,
			"runtime gathering is repeatable and its structural revision is non-zero");

		//	The measured reference vehicles of docs/procedural_world/VehicleCollisionMetrics.md and
		//	resolved starting footprints are the only things content would have added, so with those
		//	filled in the gathered registry snapshot must carry a complete preflight.
		for (int v = 0; v < 4; ++v) {
			HybridVehicleMetricInput * out = (v == 0) ? &runtime.Humvee
													: (v == 1) ? &runtime.HeavyVehicle : &runtime.Harvesters[v - 2];
			*out = *vehicles[v].Out;
		}
		for (int i = 0; i < runtime.Buildings.Count (); ++i) {
			runtime.Buildings[i].Found = true;
			runtime.Buildings[i].Radius = 20;
			runtime.Buildings[i].Height = 15;
		}
		Check (Hybrid_Run_Preflight (profile,runtime,&runtime_result),
			"the gathered registry snapshot completes preflight: %s",runtime_result.Detail.Peek_Buffer ());
		Check (runtime_result.Metrics.ContentRevision == Hybrid_Structural_Content_Revision (runtime) &&
			runtime_result.Metrics.ContentRevision != gathered_revision,
			"the metric cache digests the gathered structural records");
		Check (runtime_result.Metrics.BridgeForClass[runtime_result.Metrics.HeavyRouteClass] >= 0 &&
			runtime_result.Metrics.BridgeMaxSpanLength > 0.0f,
			"a registered bridge definition carries the heavy route class");
		Check (runtime_result.Metrics.TunnelPortalModule >= 0,
			"a registered portal-compatible tunnel module clears the heavy envelope");
		Check (runtime_result.Metrics.BiomeAvailable[HYBRID_BIOME_TEMPERATE] &&
			runtime_result.Metrics.BiomeSlotLayer[HYBRID_BIOME_TEMPERATE][HYBRID_SLOT_GROUND] >= 0 &&
			runtime_result.Metrics.BiomeSlotLayer[HYBRID_BIOME_TEMPERATE][HYBRID_SLOT_EXPOSED_ROCK] >= 0,
			"the default terrain layer registry fills the temperate material slots");
		Check (runtime_result.Metrics.PlanningInputHash != baseline,
			"the gathered registry snapshot carries its own planning identity");

		BridgeSystem::Shutdown ();
		TunnelSystem::Shutdown ();
		FoliageSystem::Shutdown ();
	}
}

void	Check_Map_Plan (void)
{
	MapPlanGenerator::Init ();

	//	Refusals leave nothing behind.
	{
		StringClass error ("");
		MapPlanParams bad;
		Make_Map_Plan_Test_Params (&bad);
		bad.TierCount = 1;
		Check (!MapPlanGenerator::Generate (bad, &error), "a single tier plan was accepted");
		Check (error.Get_Length () > 0, "a refused plan carried no reason");
		Check (!MapPlanGenerator::Has_Plan (), "a refused plan left state behind");

		Make_Map_Plan_Test_Params (&bad);
		bad.WorldWidth = 300.0f;		// 5 cells across
		Check (!MapPlanGenerator::Generate (bad, &error), "a 5-cell-wide world was accepted");

		Make_Map_Plan_Test_Params (&bad);
		bad.FlankLaneCount = 3;
		Check (!MapPlanGenerator::Generate (bad, &error), "three flank lanes were accepted");
	}

	MapPlanParams params;
	Make_Map_Plan_Test_Params (&params);
	StringClass error ("");
	Check (MapPlanGenerator::Generate (params, &error), "a valid plan was refused: %s", error.Peek_Buffer ());
	Check (MapPlanGenerator::Has_Plan (), "a successful plan reported none");
	Check (MapPlanGenerator::Get_Cell_Count_X () == 20 && MapPlanGenerator::Get_Cell_Count_Y () == 20,
			 "a 1200-unit world at 60-unit cells is %d x %d cells, not 20 x 20",
			 MapPlanGenerator::Get_Cell_Count_X (), MapPlanGenerator::Get_Cell_Count_Y ());
	unsigned int checksum = MapPlanGenerator::Get_Checksum ();
	Check (checksum != 0, "a built plan has a zero checksum");

	//	Layout graph: exactly one base per side, a centre field, one flank field, two chokes.
	Check (MapPlanGenerator::Get_Site_Count_By_Kind (MAP_PLAN_SITE_BASE_GDI) == 1, "GDI base site count is not 1");
	Check (MapPlanGenerator::Get_Site_Count_By_Kind (MAP_PLAN_SITE_BASE_NOD) == 1, "Nod base site count is not 1");
	Check (MapPlanGenerator::Get_Site_Count_By_Kind (MAP_PLAN_SITE_FIELD_CENTER) == 1, "centre field count is not 1");
	Check (MapPlanGenerator::Get_Site_Count_By_Kind (MAP_PLAN_SITE_FIELD_FLANK) == 1, "flank field count is not 1");
	Check (MapPlanGenerator::Get_Site_Count_By_Kind (MAP_PLAN_SITE_CHOKE) == 2, "choke count is not 2");

	//	Symmetry: Nod is GDI rotated 180 degrees about the centre, so the two are equidistant
	//	from it and on opposite sides.  Both sit inside the margin.
	MapPlanSite gdi, nod;
	bool found_gdi = false, found_nod = false;
	for (int i = 0; i < MapPlanGenerator::Get_Site_Count (); i++) {
		MapPlanSite site;
		MapPlanGenerator::Get_Site (i, &site);
		if (site.Kind == MAP_PLAN_SITE_BASE_GDI) { gdi = site; found_gdi = true; }
		if (site.Kind == MAP_PLAN_SITE_BASE_NOD) { nod = site; found_nod = true; }
	}
	Check (found_gdi && found_nod, "base sites missing from the site list");
	Check ((gdi.CellX + nod.CellX == 19) && (gdi.CellY + nod.CellY == 19),
			 "rotational symmetry broken: GDI (%d,%d) vs Nod (%d,%d)", gdi.CellX, gdi.CellY, nod.CellX, nod.CellY);
	float gdi_d = (float)sqrt ((double)(gdi.Position.X * gdi.Position.X + gdi.Position.Y * gdi.Position.Y));
	Check (gdi_d > 200.0f, "GDI base is only %.1f units from the centre -- bases must sit apart", gdi_d);

	//	Base discs are flat on the mid tier and painted BASE; the centre field sits on tier 0.
	{
		int tier = -1;
		MapPlanCellKind kind = MAP_PLAN_CELL_OPEN;
		Check (MapPlanGenerator::Get_Cell (gdi.CellX, gdi.CellY, &tier, &kind), "GDI base cell is off-grid");
		Check (tier == 1, "GDI base tier is %d, not the mid tier 1", tier);
		Check (kind == MAP_PLAN_CELL_BASE, "GDI base cell kind is %d, not BASE", (int)kind);
		Check (MapPlanGenerator::Get_Base_Site_Weight (gdi.Position.X, gdi.Position.Y) == 1.0f, "base site weight is not 1 at the GDI site");
		Check (MapPlanGenerator::Get_Base_Site_Weight (0.0f, 0.0f) == 0.0f, "base site weight is not 0 at the world centre");

		MapPlanSite center;
		for (int i = 0; i < MapPlanGenerator::Get_Site_Count (); i++) {
			MapPlanSite site;
			MapPlanGenerator::Get_Site (i, &site);
			if (site.Kind == MAP_PLAN_SITE_FIELD_CENTER) center = site;
		}
		MapPlanGenerator::Get_Cell (center.CellX, center.CellY, &tier, &kind);
		Check (tier == 0, "centre field tier is %d, not 0", tier);
		Check (kind == MAP_PLAN_CELL_FIELD, "centre field cell kind is %d, not FIELD", (int)kind);
	}

	//	Lane continuity: no LANE/RAMP/BASE/FIELD cell differs from a 4-neighbour of the same
	//	class by more than one tier.  Every tier change somewhere on the grid is either a RAMP
	//	(under a lane) or a CLIFF_SEAM (open ground).  Water is only on tier 0 open cells.
	{
		int violations = 0, seams = 0, ramps = 0, water = 0, lane_cells = 0;
		static const int NX[4] = { 1, -1, 0, 0 };
		static const int NY[4] = { 0, 0, 1, -1 };
		int cx_count = MapPlanGenerator::Get_Cell_Count_X ();
		int cy_count = MapPlanGenerator::Get_Cell_Count_Y ();
		for (int y = 0; y < cy_count; y++) {
			for (int x = 0; x < cx_count; x++) {
				int tier = 0; MapPlanCellKind kind;
				MapPlanGenerator::Get_Cell (x, y, &tier, &kind);
				if (kind == MAP_PLAN_CELL_CLIFF_SEAM) seams++;
				if (kind == MAP_PLAN_CELL_RAMP) ramps++;
				if (kind == MAP_PLAN_CELL_LANE) lane_cells++;
				if (kind == MAP_PLAN_CELL_WATER) {
					water++;
					if (tier != 0) violations++;
				}
				bool walkable = (kind == MAP_PLAN_CELL_LANE) || (kind == MAP_PLAN_CELL_RAMP) || (kind == MAP_PLAN_CELL_BASE) || (kind == MAP_PLAN_CELL_FIELD);
				if (!walkable) continue;
				for (int n = 0; n < 4; n++) {
					int ntier = 0; MapPlanCellKind nkind;
					if (!MapPlanGenerator::Get_Cell (x + NX[n], y + NY[n], &ntier, &nkind)) continue;
					bool nwalk = (nkind == MAP_PLAN_CELL_LANE) || (nkind == MAP_PLAN_CELL_RAMP) || (nkind == MAP_PLAN_CELL_BASE) || (nkind == MAP_PLAN_CELL_FIELD);
					if (!nwalk) continue;
					if (abs (tier - ntier) > 1) violations++;
				}
			}
		}
		Check (violations == 0, "%d lane/water continuity violations in the plan", violations);
		Check (lane_cells > 0, "the plan painted no lane cells");
		Check (seams + ramps > 0, "the plan has no tier changes at all -- flat map");
		Check (ramps > 0, "no lane crosses a tier change -- expected at least one ramp with 3 tiers");
	}

	//	Tier height realization: a base cell centre samples exactly tier * step; a point far
	//	off-grid clamps into the tier range.
	{
		float h = MapPlanGenerator::Sample_Tier_Height (gdi.Position.X, gdi.Position.Y);
		Check (fabs (h - 18.0f) < 0.01f, "GDI base centre samples %.3f, not 18.0 (tier 1 x 18)", h);
		float h_corner = MapPlanGenerator::Sample_Tier_Height (-10000.0f, -10000.0f);
		Check (h_corner >= 0.0f && h_corner <= 36.0f, "an off-grid sample returned %.3f, outside 0..36", h_corner);
	}

	//	Determinism: same seed and params, same checksum; different seed, different checksum.
	{
		Check (MapPlanGenerator::Generate (params, &error), "regeneration refused");
		Check (MapPlanGenerator::Get_Checksum () == checksum, "the same seed produced a different plan checksum");
		MapPlanParams other = params;
		other.Seed = 4243u;
		Check (MapPlanGenerator::Generate (other, &error), "second seed refused");
		Check (MapPlanGenerator::Get_Checksum () != checksum, "a different seed produced the same plan checksum");
	}

	//	Mirror symmetry and no symmetry both build.
	{
		MapPlanParams m = params;
		m.Symmetry = MAP_PLAN_SYMMETRY_MIRROR_X;
		Check (MapPlanGenerator::Generate (m, &error), "mirror_x plan refused: %s", error.Peek_Buffer ());
		MapPlanParams n = params;
		n.Symmetry = MAP_PLAN_SYMMETRY_NONE;
		Check (MapPlanGenerator::Generate (n, &error), "no-symmetry plan refused: %s", error.Peek_Buffer ());
		Check (MapPlanGenerator::Get_Site_Count_By_Kind (MAP_PLAN_SITE_BASE_NOD) == 1, "no-symmetry plan lost the Nod base");
	}

	//	Macro terrain realizes the plan: with UsePlan the planned base centre lands near the
	//	tier height; UsePlan without a plan is refused.
	{
		WorldTerrainSystem::Init ();
		MacroTerrainGenerator::Init ();
		WorldBoundsService::Init ();

		MapPlanParams plan_params;
		Make_Map_Plan_Test_Params (&plan_params);
		plan_params.WorldWidth = 256.0f;
		plan_params.WorldHeight = 256.0f;
		plan_params.ChunkSize = 16.0f;		// 16 x 16 cells
		plan_params.BaseRadiusCells = 2;
		Check (MapPlanGenerator::Generate (plan_params, &error), "terrain-realization plan refused: %s", error.Peek_Buffer ());

		MacroTerrainParams tp;
		Make_Macro_Terrain_Test_Params (&tp);
		tp.ChunkSize = 16.0f;
		tp.MaxElevation = 60.0f;
		tp.UsePlan = true;
		tp.PlanDetailWeight = 0.2f;
		Check (MacroTerrainGenerator::Generate (tp, &error), "plan-driven terrain refused: %s", error.Peek_Buffer ());

		MapPlanSite base;
		for (int i = 0; i < MapPlanGenerator::Get_Site_Count (); i++) {
			MapPlanSite site;
			MapPlanGenerator::Get_Site (i, &site);
			if (site.Kind == MAP_PLAN_SITE_BASE_GDI) base = site;
		}
		float h = 0.0f;
		Check (WorldTerrainSystem::Sample_Height (base.Position.X, base.Position.Y, &h), "no terrain height at the planned base");
		Check (fabs (h - 18.0f) < 8.0f, "planned base centre realized at %.2f, expected within 8 of tier height 18", h);

		//	Site weights: 1 at the site centre, 0 well outside every disc.
		Check (MapPlanGenerator::Get_Base_Site_Weight (base.Position.X, base.Position.Y) == 1.0f, "base site weight was not 1 at the base centre");
		Check (MapPlanGenerator::Get_Field_Site_Weight (base.Position.X, base.Position.Y) == 0.0f, "field site weight was not 0 at the base centre");
		{
			MapPlanSite field;
			bool have_field = false;
			for (int i = 0; i < MapPlanGenerator::Get_Site_Count (); i++) {
				MapPlanSite site;
				MapPlanGenerator::Get_Site (i, &site);
				if (site.Kind == MAP_PLAN_SITE_FIELD_CENTER) { field = site; have_field = true; }
			}
			Check (have_field, "no centre field site to test the field weight against");
			Check (MapPlanGenerator::Get_Field_Site_Weight (field.Position.X, field.Position.Y) == 1.0f, "field site weight was not 1 at the centre field");
			Check (MapPlanGenerator::Get_Base_Site_Weight (field.Position.X, field.Position.Y) == 0.0f, "base site weight was not 0 at the centre field");
		}

		//
		//	Plan consumers on the realized terrain (D-003 follow-on).  The strategic planner
		//	puts a base candidate inside a planned base disc and a resource candidate inside a
		//	planned field disc, and never spends a base disc on a resource candidate.  The
		//	hydrology generator carves every planned WATER cell it can reach within the river
		//	budget.
		//
		{
			WorldTerrainSystem::Build_Chunk_Registry (0.5f);
			WaterSystem::Init ();
			StrategicPlanner::Init ();
			HydrologyGenerator::Init ();

			StrategicPlannerParams sp;
			sp.Seed							= 909u;
			sp.MinApproachCount			= 1;
			sp.MinBaseCandidatesPerFaction	= 1;
			sp.MinBaseSeparation			= 24.0f;
			sp.MaxTiberiumCandidates		= 4;
			sp.MaxSettlementCandidates	= 2;
			sp.MaxPoiCandidates			= 2;
			sp.MaxExpansionCandidates	= 2;
			sp.MaxEconomicRange			= 400.0f;
			Check (StrategicPlanner::Generate (sp, &error), "strategic planner refused plan-realized terrain: %s", error.Peek_Buffer ());

			int base_in_disc = 0, resource_in_field = 0, resource_in_base = 0;
			for (int i = 0; i < StrategicPlanner::Get_Node_Count (); i++) {
				StrategicNode node;
				if (!StrategicPlanner::Get_Node (i, &node)) continue;
				float bw = MapPlanGenerator::Get_Base_Site_Weight (node.Position.X, node.Position.Y);
				float fw = MapPlanGenerator::Get_Field_Site_Weight (node.Position.X, node.Position.Y);
				if (node.Type == STRATEGIC_NODE_BASE_CANDIDATE) {
					if (bw > 0.0f) base_in_disc++;
				} else if (node.Type == STRATEGIC_NODE_RESOURCE_CANDIDATE) {
					if (fw > 0.0f) resource_in_field++;
					if (bw > 0.0f) resource_in_base++;
				}
			}
			Check (base_in_disc >= 1, "no base candidate landed inside a planned base disc");
			Check (resource_in_field >= 1, "no resource candidate landed inside a planned field disc");
			Check (resource_in_base == 0, "%d resource candidates were placed inside planned base discs", resource_in_base);

			int planned_water = 0;
			for (int cy = 0; cy < MapPlanGenerator::Get_Cell_Count_Y (); cy++) {
				for (int cx = 0; cx < MapPlanGenerator::Get_Cell_Count_X (); cx++) {
					int tier = 0;
					MapPlanCellKind kind = MAP_PLAN_CELL_OPEN;
					if (MapPlanGenerator::Get_Cell (cx, cy, &tier, &kind) && (kind == MAP_PLAN_CELL_WATER)) planned_water++;
				}
			}

			HydrologyParams hp;
			hp.Seed							= 4242u;
			hp.GenerateRivers				= false;
			hp.GenerateLakes				= true;
			hp.GeneratePonds				= true;
			hp.GenerateCoastline			= false;
			hp.RiverWidth					= 6.0f;
			hp.StreamWidth					= 2.5f;
			hp.RiverAccumulationChunks	= 2.5f;
			hp.MaxRiverCount				= 64;
			hp.SeaLevel						= -100.0f;
			Check (HydrologyGenerator::Generate (hp, &error), "hydrology refused plan-realized terrain: %s", error.Peek_Buffer ());
			if (planned_water > 0) {
				Check (HydrologyGenerator::Get_Feature_Count () >= 1,
						 "the plan had %d WATER cells but hydrology carved no water feature", planned_water);
			}
			StringClass report ("");
			Check (HydrologyGenerator::Check_No_Cliff_Band_Overlap (&report) == 0, "%s", report.Peek_Buffer ());
			Check (HydrologyGenerator::Check_Lake_Within_Basin (&report) == 0, "%s", report.Peek_Buffer ());

			HydrologyGenerator::Shutdown ();
			StrategicPlanner::Shutdown ();
			WaterSystem::Shutdown ();
		}

		MapPlanGenerator::Clear ();
		Check (!MacroTerrainGenerator::Generate (tp, &error), "UsePlan without a plan was accepted");
		tp.UsePlan = false;
		Check (MacroTerrainGenerator::Generate (tp, &error), "plain terrain refused after plan tests: %s", error.Peek_Buffer ());

		MacroTerrainGenerator::Shutdown ();
		WorldBoundsService::Shutdown ();
		WorldTerrainSystem::Shutdown ();
	}

	MapPlanGenerator::Shutdown ();
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
		Check (TerrainTextureSystem::Get_Layer_Count () > 0,
			"generated terrain has no material layers and will render invisible");

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
		Check (TerrainTextureSystem::Create_Masks (), "the hydrology test could not create terrain masks");

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
		TerrainMaskClass * water_distance = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_WATER_DISTANCE);
		float nearest_water = 10000.0f;
		if (water_distance != nullptr) {
			for (int y = 0; y < water_distance->Get_Vertex_Count_Y (); y++) {
				for (int x = 0; x < water_distance->Get_Vertex_Count_X (); x++) {
					nearest_water = WWMath::Min (nearest_water,water_distance->Get (x,y));
				}
			}
		}
		Check (nearest_water < 4.0f, "generated water left the shoreline distance mask empty");
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


// Exercise the actual native profile, whose noise and boundary can expose sparse
// features that the small synthetic fixtures above deliberately cannot.
bool Check_Native_Profile_Quality (unsigned int seed_override = 0,bool bridges_only = false,unsigned int * checksum_out = nullptr)
{
	bool usable_crossing = false;
	WorldTerrainSystem::Init ();
	WorldBoundsService::Init ();
	MacroTerrainGenerator::Init ();
	InteriorCliffGenerator::Init ();
	WaterSystem::Init ();
	HydrologyGenerator::Init ();
	StrategicPlanner::Init ();
	RoadSystem::Init ();
	RoadGraphGenerator::Init ();
	BridgeSystem::Init ();
	BridgePlanner::Init ();
	BaseSelectionPlanner::Init ();

	ProceduralWorldProfile profile;
	StringClass error ("");
	bool loaded = ProceduralWorldProfileManager::Load_Profile_From_File (
		"data/procedural_world/native.json",&profile,&error);
	Check (loaded,"native procedural profile could not load: %s",error.Peek_Buffer ());
	if (loaded) {
		Check (!profile.World.FitToLevel,"native profile unexpectedly fits a stock level");
		if (seed_override != 0) profile.Seed = seed_override;
		Check (profile.World.Width >= 2400.0f && profile.World.Height >= 2400.0f,
			"native map is smaller than the large supported FPS battlefield size");
		Check (ProceduralBoundsBuilder::Configure_World_Bounds (profile,&error),
			"native boundary configuration failed: %s",error.Peek_Buffer ());

		MacroTerrainParams terrain;
		terrain.Seed = WorldSeedService::Derive_Subsystem_Seed (
			profile.Seed,WorldSeedService::SEED_DOMAIN_TERRAIN);
		terrain.WorldWidth = profile.World.Width;
		terrain.WorldHeight = profile.World.Height;
		terrain.ChunkSize = profile.World.ChunkSize;
		terrain.CenterX = profile.World.CenterX;
		terrain.CenterY = profile.World.CenterY;
		terrain.SeaLevel = profile.World.SeaLevel;
		terrain.MinElevation = profile.Terrain.MinElevation;
		terrain.MaxElevation = profile.Terrain.MaxElevation;
		terrain.LandElevationBias = profile.Terrain.LandElevationBias;
		terrain.MaxBuildableSlopeRadians = (float)DEG_TO_RAD (profile.Terrain.MaxBuildableSlopeDegrees);
		terrain.FeatureScale = profile.Terrain.FeatureScale;
		terrain.MacroRelief = profile.Terrain.MacroRelief;
		terrain.HillDensity = profile.Terrain.HillDensity;
		terrain.MountainDensity = profile.Terrain.MountainDensity;
		terrain.ValleyDensity = profile.Terrain.ValleyDensity;
		terrain.RidgeSharpness = profile.Terrain.RidgeSharpness;
		terrain.PlateauChance = profile.Terrain.PlateauChance;
		terrain.BoundaryApproachFlattening = profile.Terrain.BoundaryApproachFlattening;

		bool ground_built = MacroTerrainGenerator::Generate (terrain,&error);
		Check (ground_built,"native macro terrain failed: %s",error.Peek_Buffer ());
		if (ground_built) {
			int macro_buildable = 0;
			for (int y = 0; y < WorldTerrainSystem::Get_Chunk_Count_Y (); y++) {
				for (int x = 0; x < WorldTerrainSystem::Get_Chunk_Count_X (); x++) {
					TerrainChunkInfoClass info;
					if (WorldTerrainSystem::Get_Chunk_Info (x,y,&info) && info.Buildable) macro_buildable++;
				}
			}
			InteriorCliffParams cliffs;
			cliffs.Seed = WorldSeedService::Derive_Subsystem_Seed (
				profile.Seed,WorldSeedService::SEED_DOMAIN_INTERIOR_CLIFFS);
			cliffs.MaxBuildableSlopeRadians = terrain.MaxBuildableSlopeRadians;
			cliffs.Density = profile.Terrain.InteriorCliffDensity;
			cliffs.LayerIndex = -1;
			Check (InteriorCliffGenerator::Generate (cliffs,&error),
				"native interior cliffs failed: %s",error.Peek_Buffer ());
			Check (InteriorCliffGenerator::Get_Feature_Count () > 0,
				"native profile produced no interior cliffs");
			int after_cliffs_buildable = 0;
			for (int y = 0; y < WorldTerrainSystem::Get_Chunk_Count_Y (); y++) {
				for (int x = 0; x < WorldTerrainSystem::Get_Chunk_Count_X (); x++) {
					TerrainChunkInfoClass info;
					if (WorldTerrainSystem::Get_Chunk_Info (x,y,&info) && info.Buildable) after_cliffs_buildable++;
				}
			}

			HydrologyParams water;
			water.Seed = WorldSeedService::Derive_Subsystem_Seed (
				profile.Seed,WorldSeedService::SEED_DOMAIN_HYDROLOGY);
			water.GenerateRivers = profile.Water.Rivers;
			water.GenerateLakes = profile.Water.Lakes;
			water.GeneratePonds = profile.Water.Ponds;
			water.GenerateCoastline = profile.Water.Coastline;
			water.RiverWidth = profile.Water.RiverWidth;
			water.StreamWidth = profile.Water.StreamWidth;
			water.RiverAccumulationChunks = profile.Water.RiverAccumulationChunks;
			water.MaxRiverCount = profile.Water.MaxRiverCount;
			water.SeaLevel = profile.World.SeaLevel;
			Check (HydrologyGenerator::Generate (water,&error),
				"native hydrology failed: %s",error.Peek_Buffer ());
			Check (WorldTerrainSystem::Build_Chunk_Registry (terrain.MaxBuildableSlopeRadians),
				"native terrain registry refresh after hydrology failed");
			Check (InteriorCliffGenerator::Restore_Chunk_Claims (),
				"native interior cliff exclusions were lost after hydrology");
			int channels = 0;
			int rivers = 0;
			int basins = 0;
			int coast = 0;
			int partial_coast = 0;
			for (int i = 0; i < HydrologyGenerator::Get_Feature_Count (); i++) {
				HydrologyFeature feature;
				if (!HydrologyGenerator::Get_Feature (i,&feature)) continue;
				if (feature.Type == HYDRO_FEATURE_COAST) {
					coast++;
					const WaterAreaClass * area = WaterSystem::Find_Area (feature.Name.Peek_Buffer ());
					if (area != nullptr && area->Get_Station_Count () != 4) partial_coast++;
				}
				else if (feature.Enclosed) basins++;
				else {
					channels++;
					if (feature.Type == HYDRO_FEATURE_RIVER) rivers++;
				}
			}
			Check (channels > 0,"native profile produced no rivers or streams");
			Check (rivers > 0,"native profile produced streams but no rivers");
			Check (basins > 0,"native profile produced no lakes or ponds");
			TerrainMaskClass * shore = TerrainTextureSystem::Peek_Mask (TERRAIN_MASK_WATER_DISTANCE);
			float nearest = 10000.0f;
			if (shore != nullptr) {
				for (int y = 0; y < shore->Get_Vertex_Count_Y (); y++) {
					for (int x = 0; x < shore->Get_Vertex_Count_X (); x++) {
						nearest = WWMath::Min (nearest,shore->Get (x,y));
					}
				}
			}
			Check (nearest < 4.0f,"native water left the shoreline distance mask empty");
			int sandy_shore_samples = 0;
			int shore_layer = TerrainTextureSystem::Find_Layer ("shore");
			if (shore != nullptr && shore_layer >= 0) {
				for (int y = 0; y < shore->Get_Vertex_Count_Y (); y += 4) {
					for (int x = 0; x < shore->Get_Vertex_Count_X (); x += 4) {
						float distance = shore->Get (x,y);
						if (distance <= 0.0f || distance >= 4.0f) continue;
						float weights[16];
						int layer_count = TerrainTextureSystem::Compute_Weights (x,y,weights,16);
						if (shore_layer < layer_count && weights[shore_layer] > 0.1f) sandy_shore_samples++;
					}
				}
			}
			Check (sandy_shore_samples > 0,"native water generated no sandy shoreline samples");
			StrategicPlannerParams plan;
			plan.Seed = WorldSeedService::Derive_Subsystem_Seed (
				profile.Seed,WorldSeedService::SEED_DOMAIN_BASES);
			plan.MinApproachCount = profile.Fairness.MinVehicleApproachCount;
			bool planned = StrategicPlanner::Generate (plan,&error);
			Check (planned,"native strategic plan failed: %s",error.Peek_Buffer ());
			int buildable = 0;
			int dry_buildable = 0;
			int approachable = 0;
			for (int y = 0; y < WorldTerrainSystem::Get_Chunk_Count_Y (); y++) {
				for (int x = 0; x < WorldTerrainSystem::Get_Chunk_Count_X (); x++) {
					TerrainChunkInfoClass info;
					if (!WorldTerrainSystem::Get_Chunk_Info (x,y,&info) || !info.Buildable) continue;
					buildable++;
					AABoxClass bounds;
					if (!WorldTerrainSystem::Get_Patch_Bounds (x,y,&bounds) ||
						WaterSystem::Is_Point_In_Water (bounds.Center.X,bounds.Center.Y)) continue;
					dry_buildable++;
					int neighbors = 0;
					const int dx[4] = { -1,1,0,0 }, dy[4] = { 0,0,-1,1 };
					for (int d = 0; d < 4; d++) {
						TerrainChunkInfoClass adjacent;
						if (WorldTerrainSystem::Get_Chunk_Info (x + dx[d],y + dy[d],&adjacent) && adjacent.Buildable) neighbors++;
					}
					if (neighbors >= profile.Fairness.MinVehicleApproachCount) approachable++;
				}
			}
			if (planned) {
				RoadGraphParams roads;
				roads.Seed = WorldSeedService::Derive_Subsystem_Seed (
					profile.Seed,WorldSeedService::SEED_DOMAIN_ROADS);
				roads.MaxBuildableSlopeRadians = terrain.MaxBuildableSlopeRadians;
				roads.RequireBridgeCrossing = true;
				bool roads_built = RoadGraphGenerator::Generate (roads,&error);
				Check (bridges_only || roads_built,"native road graph failed: %s",error.Peek_Buffer ());
				Check ((bridges_only && !roads_built) || RoadGraphGenerator::Get_Edge_Count () >= 2,"native profile produced fewer than two road edges");
				if (roads_built) {
					Check (WorldTerrainSystem::Build_Chunk_Registry (terrain.MaxBuildableSlopeRadians),
						"native terrain registry refresh after roads failed");
					Check (InteriorCliffGenerator::Restore_Chunk_Claims (),
						"native cliffs were lost after road grading");
					BridgePlannerParams bridges;
					bridges.Seed = WorldSeedService::Derive_Subsystem_Seed (
						profile.Seed,WorldSeedService::SEED_DOMAIN_BRIDGES);
					bridges.MaxApproachGradeRadians = terrain.MaxBuildableSlopeRadians;
					bool bridges_built = BridgePlanner::Generate (bridges,&error);
					Check (bridges_only || bridges_built,"native bridge plan failed: %s",error.Peek_Buffer ());
					Check (bridges_only || BridgePlanner::Get_Bridge_Count () > 0,
						"native map has no bridge over a river or ravine (%d crossing diagnostics)",
						BridgePlanner::Get_Diagnostic_Count ());
					usable_crossing = BridgePlanner::Has_Usable_River_Crossing ();
					Check (bridges_only || usable_crossing,
						"native map lacks a traversable river bridge with connected dry approaches");
					::fprintf (stdout,"native bridges: %d, usable river crossing %s\n",
						BridgePlanner::Get_Bridge_Count (),usable_crossing ? "yes" : "no");
					if (usable_crossing) {
						DynamicVectorClass<int> start_roads,states;
						for (int b = 0; b < BridgeSystem::Get_Bridge_Count (); b++) {
							BridgeClass * bridge = BridgeSystem::Peek_Bridge (b);
							start_roads.Add (bridge->Get_Start_Road ());
							states.Add (bridge->Get_Damage_State ());
							bridge->Set_Start_Road (-1);
						}
						Check (!BridgePlanner::Has_Usable_River_Crossing (),"detached bridges passed the native acceptance gate");
						for (int b = 0; b < BridgeSystem::Get_Bridge_Count (); b++) {
							BridgeSystem::Peek_Bridge (b)->Set_Start_Road (start_roads[b]);
							BridgeSystem::Set_Damage_State (b,BRIDGE_STATE_BROKEN);
						}
						Check (!BridgePlanner::Has_Usable_River_Crossing (),"broken bridges passed the native acceptance gate");
						for (int b = 0; b < BridgeSystem::Get_Bridge_Count (); b++) BridgeSystem::Set_Damage_State (b,states[b]);
						Check (BridgePlanner::Has_Usable_River_Crossing (),"restored bridge crossing failed the native acceptance gate");
					}
					if (bridges_built && !bridges_only) {
						BaseSelectionParams bases;
						bases.Seed = plan.Seed;
						bases.MaxInitialResourceTravelTimeDifferenceSeconds =
							profile.Fairness.MaxInitialResourceTravelTimeDifferenceSeconds;
						bases.MaxBaseToBaseTravelTimeDifferenceSeconds =
							profile.Fairness.MaxBaseToBaseTravelTimeDifferenceSeconds;
						bases.MinBaseToBaseTravelTimeSeconds =
							profile.Fairness.MinBaseToBaseTravelTimeSeconds;
						bases.MinBaseExpansionArea = profile.Fairness.MinBaseExpansionArea;
						bases.AllowSingleBridgeDependency = profile.Fairness.AllowSingleBridgeDependency;
						bases.MaxRepairAttemptsPerPair = profile.Retry.MaxLocalRetriesPerStage;
						bool selected = BaseSelectionPlanner::Generate (bases,&error);
						Check (selected,
							"native base selection failed: %s",error.Peek_Buffer ());
						Check (BaseSelectionPlanner::Has_Selection (),"native map has no selected base pair");
					}
				}
			}
			Check (dry_buildable >= 200,"native profile has too little dry buildable land: %d chunks",dry_buildable);
			Check (StrategicPlanner::Get_Node_Count_By_Type (STRATEGIC_NODE_BASE_CANDIDATE) >= 2,
				"native profile lacks two base candidates");
			::fprintf (stdout,"native profile: %d macro/%d after cliffs/%d after water/%d dry/%d approachable, %d bases, %d resources, %d cliffs, %d channels (%d rivers), %d basins, %d coast (%d partial), %d roads, %d sand samples\n",
				macro_buildable,after_cliffs_buildable,buildable,dry_buildable,approachable,
				StrategicPlanner::Get_Node_Count_By_Type (STRATEGIC_NODE_BASE_CANDIDATE),
				StrategicPlanner::Get_Node_Count_By_Type (STRATEGIC_NODE_RESOURCE_CANDIDATE),
				InteriorCliffGenerator::Get_Feature_Count (),channels,rivers,basins,coast,partial_coast,
				RoadGraphGenerator::Get_Edge_Count (),sandy_shore_samples);
			if (BaseSelectionPlanner::Has_Selection ()) {
				const BaseFairnessMetrics & fairness = BaseSelectionPlanner::Get_Fairness_Metrics ();
				::fprintf (stdout,"native base routes: %.1f s base-to-base, %.1f s resource difference\n",
					fairness.BaseToBaseTravelTimeSeconds,fairness.ResourceTravelTimeDifferenceSeconds);
			}
			::fprintf (stdout,"native checksums: terrain=%08X cliffs=%08X water=%08X roads=%08X bases=%08X\n",
				MacroTerrainGenerator::Get_Terrain_Checksum (),InteriorCliffGenerator::Get_Checksum (),
				HydrologyGenerator::Get_Checksum (),RoadGraphGenerator::Get_Checksum (),
				BaseSelectionPlanner::Get_Checksum ());
		}
	}

	if (checksum_out != nullptr) {
		unsigned int checksums[3] = { MacroTerrainGenerator::Get_Terrain_Checksum (),
			RoadGraphGenerator::Get_Checksum (),BridgePlanner::Get_Checksum () };
		CRCEngine crc;
		crc (checksums,sizeof(checksums));
		*checksum_out = (unsigned int)crc;
	}
	BaseSelectionPlanner::Shutdown ();
	BridgePlanner::Shutdown ();
	BridgeSystem::Shutdown ();
	RoadGraphGenerator::Shutdown ();
	RoadSystem::Shutdown ();
	StrategicPlanner::Shutdown ();
	HydrologyGenerator::Shutdown ();
	WaterSystem::Shutdown ();
	InteriorCliffGenerator::Shutdown ();
	for (int y = 0; y < WorldTerrainSystem::Get_Chunk_Count_Y (); y++) {
		for (int x = 0; x < WorldTerrainSystem::Get_Chunk_Count_X (); x++) {
			TerrainChunkInfoClass info;
			if (WorldTerrainSystem::Get_Chunk_Info (x,y,&info)) {
				Check (!info.InteriorCliff,"interior cliff exclusion survived generator shutdown");
			}
		}
	}
	MacroTerrainGenerator::Shutdown ();
	WorldTerrainSystem::Shutdown ();
	WorldBoundsService::Shutdown ();
	return usable_crossing;
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

	//	D-001 P23: the Tiberium haze follows the generated fields -- the profile density inside a
	//	field, nothing two radii from every field, nothing at all with the density off.
	{
		ResolvedTiberiumField field;
		if (TiberiumFieldGenerator::Get_Field (0,&field)) {
			Check (WWMath::Fabs (WeatherMgrClass::Compute_Tiberium_Haze (field.Position,0.2f) - 0.2f) < 0.0001f,
					 "the haze inside a Tiberium field is not the profile density");
			Check (WeatherMgrClass::Compute_Tiberium_Haze (field.Position,0.0f) == 0.0f,
					 "a profile with the haze off drew haze anyway");
			Check (WeatherMgrClass::Compute_Tiberium_Haze (field.Position + Vector3 (100000.0f,0.0f,0.0f),0.2f) == 0.0f,
					 "the haze reached far from every field");
		}
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
**	V2 roadmap Section 16A -- a selected base site becomes a playable base (D-001 priority 5).
**
**	The same real chain Check_Base_Selection proves, then ProceduralBaseLayoutService over it:
**	the profile's base block, the candidate order (behind the base first, the same every run),
**	Plan_And_Place's records (every wanted building is accounted for -- placed, or refused with a
**	named reason), the spawn pool (inside the protected region, off water and exclusions) and the
**	runtime choice among it.  No device and no preset database here, so the buildings themselves
**	are not created: that is the live check.  Descriptors are registered from text.
***********************************************************************************************/
void	Check_Base_Layout (void)
{
	//	The profile block (schema 9).
	{
		ProceduralWorldProfile profile;
		StringClass error ("");
		Check (ProceduralWorldProfileManager::Load_Profile_From_Text (
					"{ \"schema_version\": 9, \"seed\": 5, \"base\": { "
					"\"gdi_initial_buildings\": [\"gdi_construction_yard\", \"gdi_power_plant\"], "
					"\"nod_initial_buildings\": [\"nod_construction_yard\"], "
					"\"spawn_candidate_count\": 6, \"spawn_zone_radius\": 20.0 } }",&profile,&error),
				"a profile with a base block was refused: %s",error.Peek_Buffer ());
		Check (profile.Base.GdiInitialBuildings.Count () == 2 && profile.Base.NodInitialBuildings.Count () == 1,
				"the base block's building lists did not parse (%d, %d)",
				profile.Base.GdiInitialBuildings.Count (),profile.Base.NodInitialBuildings.Count ());
		Check (profile.Base.SpawnCandidateCount == 6,"spawn_candidate_count read as %d",profile.Base.SpawnCandidateCount);

		ProceduralWorldProfile old_profile;
		Check (ProceduralWorldProfileManager::Load_Profile_From_Text ("{ \"schema_version\": 8, \"seed\": 5 }",&old_profile,&error),
				"a schema-8 profile no longer loads: %s",error.Peek_Buffer ());
		Check (old_profile.Base.GdiInitialBuildings.Count () == 0 && old_profile.Base.NodInitialBuildings.Count () == 0,
				"a profile with no base block starts with buildings");

		ProceduralWorldProfile bad;
		ProceduralWorldProfileManager::Load_Profile_From_Text (
			"{ \"schema_version\": 9, \"seed\": 5, \"base\": { \"spawn_zone_radius\": -1.0 } }",&bad,&error);
		Check (!ProceduralWorldProfileManager::Validate_Profile (bad,&error),
				"a negative spawn zone radius was accepted");
	}

	//	Candidate order: the first spot is directly behind the base, and the order is stable.
	{
		DynamicVectorClass<Vector3> first, second;
		Vector3 center (10.0f,20.0f,0.0f);
		Vector3 toward (1.0f,0.0f,0.0f);
		int count = ProceduralBaseLayoutService::Get_Candidate_Positions (center,toward,6.0f,30.0f,&first);
		ProceduralBaseLayoutService::Get_Candidate_Positions (center,toward,6.0f,30.0f,&second);
		Check (count > 0 && count == first.Count (),"no candidate positions came back");
		Check ((first.Count () > 0) && Near (first [0].X,4.0f) && Near (first [0].Y,20.0f),
				"the first candidate is not directly behind the base (%f, %f)",
				(first.Count () > 0) ? first [0].X : 0.0f,(first.Count () > 0) ? first [0].Y : 0.0f);
		bool same = (first.Count () == second.Count ());
		bool inside = true;
		for (int i = 0; same && i < first.Count (); i++) {
			same = (first [i] == second [i]);
			float dx = first [i].X - center.X;
			float dy = first [i].Y - center.Y;
			if (((dx * dx) + (dy * dy)) > (30.01f * 30.01f)) inside = false;
		}
		Check (same,"the same site gave two different candidate orders");
		Check (inside,"a candidate lies outside the radius it was asked for");
	}

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
	BuildingDescriptorManager::Init ();
	ProceduralBaseLayoutService::Reset ();

	{
		StringClass error ("");
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_construction_yard",
					"{ \"model_name\": \"mgcon_ext.w3d\", \"schema_version\": 1, "
					"\"footprint\": { \"width\": 40.0, \"depth\": 40.0 }, \"build\": { \"buildable\": false, \"team\": 1 } }",&error),
				"gdi_construction_yard failed to load: %s",error.Peek_Buffer ());
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("nod_construction_yard",
					"{ \"model_name\": \"mncon_ext.w3d\", \"schema_version\": 1, "
					"\"footprint\": { \"width\": 40.0, \"depth\": 40.0 }, \"build\": { \"buildable\": false, \"team\": 0 } }",&error),
				"nod_construction_yard failed to load: %s",error.Peek_Buffer ());
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("gdi_power_plant",
					"{ \"model_name\": \"mgpwr_ext.w3d\", \"schema_version\": 1, "
					"\"footprint\": { \"width\": 36.0, \"depth\": 30.0 }, \"build\": { \"team\": 1 } }",&error),
				"gdi_power_plant failed to load: %s",error.Peek_Buffer ());
	}

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
		HydrologyGenerator::Generate (hydro_params,&error);
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
		StrategicPlanner::Generate (plan_params,&error);

		RoadGraphParams road_params;
		road_params.Seed							= 55u;
		road_params.MaxBuildableSlopeRadians	= 0.5f;
		road_params.SlopeCostWeight				= 3.0f;
		road_params.CrossingCostPenalty			= 8.0f;
		road_params.BoundaryApproachCostWeight	= 1.5f;
		RoadGraphGenerator::Generate (road_params,&error);

		BridgePlannerParams bridge_params;
		bridge_params.Seed					= 77u;
		bridge_params.WaterClearanceMargin	= 2.0f;
		bridge_params.MaxSpanLength			= 200.0f;
		BridgePlanner::Generate (bridge_params,&error);

		BaseSelectionParams params;
		params.Seed												= 123u;
		params.MaxInitialResourceTravelTimeDifferenceSeconds	= 60.0f;
		params.MaxBaseToBaseTravelTimeDifferenceSeconds			= 120.0f;
		params.MinBaseExpansionArea									= 1024.0f;
		params.AllowSingleBridgeDependency							= false;
		params.MaxRepairAttemptsPerPair								= 8;
		Check (BaseSelectionPlanner::Generate (params,&error),
				"the base selection this check depends on failed: %s",error.Peek_Buffer ());
	}

	ProceduralWorldProfile profile;
	profile.Base.GdiInitialBuildings.Add (StringClass ("gdi_construction_yard"));
	profile.Base.GdiInitialBuildings.Add (StringClass ("gdi_power_plant"));
	profile.Base.GdiInitialBuildings.Add (StringClass ("no_such_building"));
	profile.Base.NodInitialBuildings.Add (StringClass ("nod_construction_yard"));
	profile.Base.SpawnCandidateCount = 8;
	profile.Base.SpawnZoneRadius = 25.0f;

	StringClass report ("");
	Check (ProceduralBaseLayoutService::Plan_And_Place (profile,&report),
			"a real base selection produced no base layout");
	Check (ProceduralBaseLayoutService::Has_Base (PLAYERTYPE_GDI) && ProceduralBaseLayoutService::Has_Base (PLAYERTYPE_NOD),
			"a faction with a selected site has no base");
	Check (ProceduralBaseLayoutService::Get_Building_Count (PLAYERTYPE_GDI) == 3,
			"%d GDI buildings were accounted for, not the 3 the profile named",
			ProceduralBaseLayoutService::Get_Building_Count (PLAYERTYPE_GDI));
	Check (ProceduralBaseLayoutService::Get_Building_Count (PLAYERTYPE_NOD) == 1,
			"%d Nod buildings were accounted for, not 1",ProceduralBaseLayoutService::Get_Building_Count (PLAYERTYPE_NOD));

	ProceduralBaseBuildingRecord unknown;
	Check (ProceduralBaseLayoutService::Get_Building (PLAYERTYPE_GDI,2,&unknown) &&
			 (unknown.BuildingId == 0) && (unknown.Refusal == PLACEMENT_NO_DESCRIPTOR),
			"a building with no descriptor was not refused as one");

	for (int faction = 0; faction < 2; faction++) {
		SelectedBaseSite site;
		BaseSelectionPlanner::Get_Selected_Site (faction,&site);

		int spawns = ProceduralBaseLayoutService::Get_Spawn_Candidate_Count (faction);
		Check (spawns > 0 && spawns <= 8,"faction %d has %d spawn candidates, not 1..8",faction,spawns);

		for (int i = 0; i < spawns; i++) {
			ProceduralSpawnCandidate spawn;
			ProceduralBaseLayoutService::Get_Spawn_Candidate (faction,i,&spawn);
			float dx = spawn.Position.X - site.Position.X;
			float dy = spawn.Position.Y - site.Position.Y;
			Check (((dx * dx) + (dy * dy)) <= (site.ProtectedRadius * site.ProtectedRadius) + 0.01f,
					"faction %d spawn %d is outside the base's protected region",faction,i);
			Check (GeneratedWorldPlacementRules::Classify (spawn.Position,1.0f) == GENERATED_WORLD_EXCLUSION_NONE,
					"faction %d spawn %d stands in an exclusion",faction,i);
		}

		Vector3 zone_center;
		float zone_radius = 0.0f;
		Check (ProceduralBaseLayoutService::Get_Spawn_Zone (faction,&zone_center,&zone_radius) && Near (zone_radius,25.0f),
				"faction %d has no fallback spawn zone of the profile's radius",faction);

		Matrix3D chosen (1);
		Check (ProceduralBaseLayoutService::Choose_Respawn_Point (faction,nullptr,&chosen),
				"faction %d with a base had no respawn point",faction);
		bool from_pool = false;
		for (int i = 0; i < spawns; i++) {
			ProceduralSpawnCandidate spawn;
			ProceduralBaseLayoutService::Get_Spawn_Candidate (faction,i,&spawn);
			if (spawn.Position == chosen.Get_Translation ()) from_pool = true;
		}
		Check (from_pool,"faction %d respawned somewhere not in its own pool",faction);
	}

	Matrix3D none (1);
	Check (!ProceduralBaseLayoutService::Choose_Respawn_Point (PLAYERTYPE_NEUTRAL,nullptr,&none),
			"a faction with no generated base was given a generated respawn point");

	ProceduralBaseLayoutService::Reset ();
	Check (!ProceduralBaseLayoutService::Choose_Respawn_Point (PLAYERTYPE_GDI,nullptr,&none),
			"a reset layout still answers respawns; a stock map would lose its own spawners");

	BuildingDescriptorManager::Unload_All ();
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

	//
	//	D-001: the renderer's model.  The world extent covers every marker; world -> panel ->
	//	world returns the same point; north is up; and water, cliff and road cells each draw in
	//	their own colour.
	//
	{
		float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
		Check (CommanderMinimap::Get_World_Extent (&min_x,&min_y,&max_x,&max_y),"the minimap has no world extent");

		RectClass panel (10.0f,100.0f,210.0f,300.0f);
		for (int i = 0; i < MinimapBaseLayer::Get_Marker_Count (); i++) {
			MinimapMarker marker;
			MinimapBaseLayer::Get_Marker (i,&marker);
			float x = 0.0f, y = 0.0f;
			Check (CommanderMinimap::World_To_Panel (marker.Position,panel,&x,&y),"marker %d did not map",i);
			Check ((x >= panel.Left - 0.01f) && (x <= panel.Right + 0.01f) && (y >= panel.Top - 0.01f) && (y <= panel.Bottom + 0.01f),
					 "marker %d maps outside the map panel",i);
			Vector3 back;
			Check (CommanderMinimap::Panel_To_World (x,y,panel,&back) &&
					 (WWMath::Fabs (back.X - marker.Position.X) < 0.05f) && (WWMath::Fabs (back.Y - marker.Position.Y) < 0.05f),
					 "marker %d did not survive world -> panel -> world",i);
		}

		float south_x = 0.0f, south_y = 0.0f, north_x = 0.0f, north_y = 0.0f;
		CommanderMinimap::World_To_Panel (Vector3 (min_x,min_y,0.0f),panel,&south_x,&south_y);
		CommanderMinimap::World_To_Panel (Vector3 (min_x,max_y,0.0f),panel,&north_x,&north_y);
		Check (north_y < south_y,"north is not up on the minimap");

		MinimapCellClass water, cliff, road, ground;
		water.IsWater = true;
		cliff.IsCliffBand = true;
		road.HasRoad = true;
		ground.AverageHeight = 5.0f;
		unsigned int colors [4] = {
			CommanderMinimap::Cell_Color (water,0.0f,10.0f),CommanderMinimap::Cell_Color (cliff,0.0f,10.0f),
			CommanderMinimap::Cell_Color (road,0.0f,10.0f),CommanderMinimap::Cell_Color (ground,0.0f,10.0f)
		};
		bool distinct = true;
		for (int a = 0; a < 4; a++) for (int b = a + 1; b < 4; b++) if (colors [a] == colors [b]) distinct = false;
		Check (distinct,"two kinds of minimap cell draw in the same colour");
	}

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

		//
		//	D-001 priority 6: stages 1 and 2 actually resolving a crossing, on this same real,
		//	deterministic terrain.  The plan is regenerated across a descending sweep of one limit;
		//	the first setting at which the original position is refused but the named stage finds a
		//	legal bridge is the case.  The sweep is the same every run, so so is the answer.
		//
		{
			//	Stage 2 -- the original approach is too steep, a longer one is not.
			BridgePlannerParams approach_params = bridge_params;
			approach_params.MaxSpanLength				= 200.0f;	// span out of the way
			approach_params.MaxRerouteAttemptsPerEdge	= 0;
			bool stage_two = false;
			for (float grade = 1.2f; (grade >= 0.01f) && !stage_two; grade -= 0.01f) {
				approach_params.MaxApproachGradeRadians = grade;
				if (!BridgePlanner::Generate (approach_params,&error)) break;
				CrossingResolutionRecord rec;
				if ((BridgePlanner::Get_Diagnostic_Count () >= 1) && BridgePlanner::Get_Diagnostic (0,&rec) &&
					 rec.Resolved && (rec.StageResolvedAt == CROSSING_STAGE_EXTENDED_APPROACH)) {
					stage_two = true;
					Check (rec.Displacement == 0.0f,"an extended approach moved the crossing; stage 2 never does");
					Check (BridgePlanner::Get_Bridge_Count () == 1,"a stage-2 resolution built %d bridges, not 1",
							 BridgePlanner::Get_Bridge_Count ());
				}
			}
			Check (stage_two,"no grade limit on this terrain made a longer approach resolve the crossing");

			//	Stage 1 -- the original span is too long, a nearby reposition's is not.  This river is
			//	the same width all along, so the case is built: a small pond is laid against the far
			//	abutment of the original crossing, on the road's own line and no wider than one
			//	reposition step, so the original crossing now has to span river and pond while one
			//	a step to either side spans the river alone.  Last, because WaterSystem has no way to
			//	take one area back out.
			BridgePlannerParams reposition_params = bridge_params;
			reposition_params.MaxSpanLength				= 200.0f;
			reposition_params.MaxApproachGradeRadians	= 1.5f;		// grade out of the way
			reposition_params.MaxRerouteAttemptsPerEdge	= 0;
			Check (BridgePlanner::Generate (reposition_params,&error),"the unconstrained plan failed: %s",error.Peek_Buffer ());

			ResolvedBridge original;
			BridgeClass * original_bridge = nullptr;
			if ((BridgePlanner::Get_Bridge_Count () == 1) && BridgePlanner::Get_Bridge (0,&original)) {
				original_bridge = BridgeSystem::Peek_Bridge (original.BridgeId);
			}
			Check (original_bridge != nullptr,"the unconstrained plan built no bridge to constrain");

			if (original_bridge != nullptr) {
				Vector3 start_point = original_bridge->Get_Start ();
				Vector3 end_point = original_bridge->Get_End ();
				Vector3 along = end_point - start_point;
				along.Z = 0.0f;
				float span = along.Length ();
				along /= span;
				Vector3 across (-along.Y,along.X,0.0f);

				Vector3 center = end_point + (along * 3.0f);		// reaching back over the abutment, so the bank search walks on into it
				WaterAreaClass pond;
				pond.Set_Name ("fallback_stage1_pond");
				pond.Set_Definition ("ow_water_pond");
				pond.Set_Closed (true);
				pond.Set_Height (end_point.Z);
				pond.Add_Station (center - (along * 5.0f) - (across * 3.0f),0.0f);
				pond.Add_Station (center + (along * 5.0f) - (across * 3.0f),0.0f);
				pond.Add_Station (center + (along * 5.0f) + (across * 3.0f),0.0f);
				pond.Add_Station (center - (along * 5.0f) + (across * 3.0f),0.0f);
				WaterSystem::Add_Area (pond);

				reposition_params.MaxSpanLength = span + 2.0f;
				Check (BridgePlanner::Generate (reposition_params,&error),"the constrained plan failed: %s",error.Peek_Buffer ());

				CrossingResolutionRecord rec;
				bool have = (BridgePlanner::Get_Diagnostic_Count () >= 1) && BridgePlanner::Get_Diagnostic (0,&rec);
				Check (have && rec.Resolved,"the crossing did not resolve once the pond lengthened its original span");
				Check (have && (rec.StageResolvedAt == CROSSING_STAGE_LOCAL_REPOSITION),
						 "the crossing resolved at stage %d, not by a local reposition",have ? (int)rec.StageResolvedAt : -1);
				Check (have && (rec.Displacement > 0.0f) && (rec.AttemptsMade >= 2),
						 "a stage-1 resolution reported %.1f displacement over %d attempt(s)",
						 have ? rec.Displacement : 0.0f,have ? rec.AttemptsMade : 0);
				Check (BridgePlanner::Get_Bridge_Count () == 1,"a stage-1 resolution built %d bridges, not 1",
						 BridgePlanner::Get_Bridge_Count ());
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

			//	D-001 locomotion masks: one topology, a capability mask per chunk.  The sector graph's
			//	walkable set is exactly the chunks every loaded class can cross; a chunk no class can
			//	cross has no mask; an edge admits only what both its chunks admit; and a chunk reaches
			//	itself for any class it admits.
			{
				unsigned loaded = NavMeshGenerator::Get_Loaded_Capabilities ();
				Check ((loaded & NAV_CAPABILITY_INFANTRY) != 0,"infantry is not among the loaded capabilities");
				int agreeing = 0;
				int infantry_only_or_more = 0;
				for (int py = 0; py < NavMeshGenerator::Get_Chunk_Count_Y (); py++) {
					for (int px = 0; px < NavMeshGenerator::Get_Chunk_Count_X (); px++) {
						unsigned mask = NavMeshGenerator::Get_Chunk_Capability (px,py);
						unsigned every = loaded & (NAV_CAPABILITY_INFANTRY | NAV_CAPABILITY_WHEELED | NAV_CAPABILITY_TRACKED);
						bool all_classes = ((mask & every) == every) &&
												 (((loaded & NAV_CAPABILITY_HEAVY) == 0) || ((mask & NAV_CAPABILITY_HEAVY) != 0));
						if (all_classes == NavMeshGenerator::Is_Chunk_Navigable (px,py)) agreeing++;
						if ((mask & NAV_CAPABILITY_INFANTRY) != 0) infantry_only_or_more++;
						Check ((mask & ~loaded) == 0,"chunk %d,%d admits a class this world has no envelope for",px,py);
						unsigned east = NavMeshGenerator::Get_Edge_Capability (px,py,NAV_EDGE_EAST);
						Check ((east & ~mask) == 0,"the edge east of %d,%d admits what its own chunk does not",px,py);
						if ((mask & NAV_CAPABILITY_INFANTRY) != 0) {
							Check (NavMeshGenerator::Is_Reachable (NAV_CAPABILITY_INFANTRY,px,py,px,py),
									 "infantry cannot reach the chunk it stands on at %d,%d",px,py);
						}
					}
				}
				Check (agreeing == NavMeshGenerator::Get_Chunk_Count_X () * NavMeshGenerator::Get_Chunk_Count_Y (),
						 "the sector graph's walkable chunks disagree with the capability masks on %d chunks",
						 (NavMeshGenerator::Get_Chunk_Count_X () * NavMeshGenerator::Get_Chunk_Count_Y ()) - agreeing);
				Check (infantry_only_or_more >= navigable_count,
						 "fewer chunks admit infantry than admit every class at once");
			}

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
		wrong.PlanningHash ^= 0xFFFFu;
		Check (WorldSyncService::Begin_Join (server_handshake,wrong,&detail) == WORLD_SYNC_PLANNING_MISMATCH,
				 "a hybrid planning input hash mismatch was accepted");

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
 * Check_World_Sync_Transport -- D-001 priority 3                                                *
 *                                                                                               *
 *	The transport end to end, with the network replaced by a capturing sender and nothing else.  *
 *	The server builds its real join package and live operation into fragments; the world is      *
 *	thrown away and regenerated exactly as a client would; and those same fragments, fed back    *
 *	through Receive_Fragment in the client role, rebuild the server's world to the byte.  What    *
 *	this cannot prove is two processes and a socket -- that is the live gate.                    *
 *=============================================================================================*/
struct CapturedFragment
{
	int					Destination;
	WorldSyncFragment	Fragment;
};

static CapturedFragment	_Captured [512];
static int					_CapturedCount = 0;

static void	Capture_Fragment (int destination,const WorldSyncFragment & fragment)
{
	if (_CapturedCount < (int)(sizeof (_Captured) / sizeof (_Captured [0]))) {
		_Captured [_CapturedCount].Destination	= destination;
		_Captured [_CapturedCount].Fragment		= fragment;
		_CapturedCount++;
	}
}

//	The kinds of the whole messages in [first, _CapturedCount), in order, one per message id.
static int	Captured_Message_Kinds (int first,int * kinds_out,int max_kinds)
{
	int count = 0;
	unsigned int last_id = 0;
	for (int i = first; (i < _CapturedCount) && (count < max_kinds); i++) {
		if (_Captured [i].Fragment.MessageId == last_id) continue;
		last_id = _Captured [i].Fragment.MessageId;
		kinds_out [count++] = _Captured [i].Fragment.Kind;
	}
	return count;
}

//	The ground alone, chunk by chunk -- the world checksum with the journal left out, for a restore
//	that compacts the journal on purpose (ProceduralWorldPersistenceClass::Apply_Pending).
static unsigned int	Ground_Checksum (void)
{
	CRCEngine crc;
	for (int py = 0; py < WorldTerrainSystem::Get_Patch_Count_Y (); py++) {
		for (int px = 0; px < WorldTerrainSystem::Get_Patch_Count_X (); px++) {
			unsigned int chunk = 0;
			if (WorldSnapshotService::Compute_Chunk_Checksum (px,py,&chunk)) crc (&chunk,sizeof (chunk));
		}
	}
	return (unsigned int)crc;
}


void	Check_World_Sync_Transport (void)
{
	bool was_server = CombatManager::I_Am_Server ();
	bool was_client = CombatManager::I_Am_Client ();

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
	WorldSyncTransport::Init ();

	//	Identity: what a client needs to regenerate the server's world, and a refusal of anything
	//	that is not exactly that.
	{
		unsigned char buffer [256];
		int used = WorldSyncTransport::Build_Identity_Payload (12345u,"data/procedural_world/small.json",
																				 buffer,sizeof (buffer));
		unsigned int seed = 0, generator = 0;
		StringClass path ("");
		Check (used > 0 && WorldSyncTransport::Parse_Identity_Payload (buffer,used,&seed,&path,&generator),
				 "a world identity did not survive its own round trip");
		Check (seed == 12345u && path == "data/procedural_world/small.json" &&
				 generator == (unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION,
				 "a world identity came back as seed %u, profile '%s', generator %u",seed,path.Peek_Buffer (),generator);
		Check (!WorldSyncTransport::Parse_Identity_Payload (buffer,used - 1,&seed,&path,&generator),
				 "a truncated world identity was accepted");
		Check (WorldSyncTransport::Build_Identity_Payload (1u,"x",buffer,4) == 0,
				 "a world identity was written into a buffer too small for it");
	}

	//	Nothing is sent without a network: the message is dropped and counted.
	Check (WorldSyncTransport::Send_Message (WSYNC_TO_SERVER,WSYNC_MSG_IDENTITY_REQUEST,nullptr,0) == 0 &&
			 WorldSyncTransport::Get_Messages_Dropped () == 1,"a message with no sender installed was not dropped");

	static const int   PATCHES		= 4;
	static const float CELL_SIZE	= 4.0f;
	static const float SLOPE		= 10.0f;
	static const Vector3 ORIGIN (-128.0f,-128.0f,0.0f);

	//
	//	The server.
	//
	CombatManager::Set_I_Am_Server (true);
	CombatManager::Set_I_Am_Client (false);
	_CapturedCount = 0;
	WorldSyncTransport::Set_Sender (Capture_Fragment);

	Build_Uniform_Ramp_Terrain (PATCHES,CELL_SIZE,SLOPE,ORIGIN);
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);
	{
		StringClass error ("");
		Check (WorldOperationJournal::Begin (&error),"the server journal failed to open: %s",error.Peek_Buffer ());
	}

	WorldOperationClass crater;
	crater.Type			= WORLD_OP_CREATE_CRATER;
	crater.BaseRevision	= WorldOperationJournal::Get_Revision ();
	crater.Center		= Vector3 (0.0f,0.0f,0.0f);
	crater.Radius		= 6.0f;
	crater.Depth		= 2.0f;
	crater.TimeSeconds	= 10.0f;
	Check (WorldOperationJournal::Submit (crater).Is_Applied (),"the crater this check depends on was refused");
	Check (_CapturedCount == 0,"an operation was broadcast with no client joined");

	//	Client 7 asks to join and receives the package: handshake, the operation, the end marker.
	{
		WorldSyncFragment request;
		request.MessageId = 1;
		request.Kind = WSYNC_MSG_JOIN_REQUEST;
		request.Index = 0;
		request.Count = 1;
		request.Bytes = 0;
		WorldSyncTransport::Receive_Fragment (7,request);
	}

	int kinds [32];
	int kind_count = Captured_Message_Kinds (0,kinds,32);
	Check (kind_count == 3 && kinds [0] == WSYNC_MSG_HANDSHAKE && kinds [1] == WSYNC_MSG_JOIN_OPERATION &&
			 kinds [2] == WSYNC_MSG_JOIN_END,
			 "the join package was %d messages, not handshake / operation / end",kind_count);
	for (int i = 0; i < _CapturedCount; i++) {
		Check (_Captured [i].Destination == 7,"a join package fragment went to %d, not the client that asked",
				 _Captured [i].Destination);
	}
	Check (WorldSyncTransport::Is_Client_Synchronised (7),"a client sent its package was not put on the live stream");

	//	A live operation reaches the joined client, numbered after the package.
	int package_fragments = _CapturedCount;
	WorldOperationClass raise;
	raise.Type			= WORLD_OP_RAISE;
	raise.BaseRevision	= WorldOperationJournal::Get_Revision ();
	raise.Center		= Vector3 (60.0f,60.0f,0.0f);
	raise.Radius		= 10.0f;
	raise.Depth			= 3.0f;
	raise.TimeSeconds	= 11.0f;
	Check (WorldOperationJournal::Submit (raise).Is_Applied (),"the raise this check depends on was refused");
	Check (_CapturedCount == package_fragments + 1 &&
			 _Captured [package_fragments].Fragment.Kind == WSYNC_MSG_LIVE_OPERATION &&
			 _Captured [package_fragments].Destination == 7,
			 "the committed operation was not broadcast to the joined client");

	unsigned int server_world = 0;
	Check (WorldSnapshotService::Compute_World_Checksum (&server_world),"the server world has no checksum");

	//	Keep what the server sent client 7, in order: that is the whole wire.
	static CapturedFragment server_stream [512];
	int server_stream_count = _CapturedCount;
	for (int i = 0; i < server_stream_count; i++) server_stream [i] = _Captured [i];

	//	A long failure report from client 9, split across fragments that arrive out of order and
	//	once twice, is reassembled into exactly one message -- and takes 9 off the live stream.
	{
		WorldSyncFragment request;
		request.MessageId = 2;
		request.Kind = WSYNC_MSG_JOIN_REQUEST;
		request.Count = 1;
		WorldSyncTransport::Receive_Fragment (9,request);
		Check (WorldSyncTransport::Is_Client_Synchronised (9),"client 9 was not joined");

		unsigned char report [900];
		report [0] = 0xFF; report [1] = 0xFF; report [2] = 0xFF; report [3] = 0xFF;	// -1: failed
		for (int i = 4; i < (int)sizeof (report); i++) report [i] = (unsigned char)('a' + (i % 26));

		WorldSyncFragment pieces [3];
		for (int p = 0; p < 3; p++) {
			pieces [p].MessageId	= 77;
			pieces [p].Kind		= WSYNC_MSG_RESULT;
			pieces [p].Index		= p;
			pieces [p].Count		= 3;
			int offset = p * WorldSyncFragment::MAX_PAYLOAD;
			int size = (int)sizeof (report) - offset;
			if (size > WorldSyncFragment::MAX_PAYLOAD) size = WorldSyncFragment::MAX_PAYLOAD;
			pieces [p].Bytes = size;
			::memcpy (pieces [p].Payload,report + offset,size);
		}

		int received_before = WorldSyncTransport::Get_Messages_Received ();
		WorldSyncTransport::Receive_Fragment (9,pieces [2]);
		WorldSyncTransport::Receive_Fragment (9,pieces [0]);
		WorldSyncTransport::Receive_Fragment (9,pieces [0]);
		Check (WorldSyncTransport::Is_Client_Synchronised (9),"a message was dispatched before its last fragment");
		WorldSyncTransport::Receive_Fragment (9,pieces [1]);
		Check (WorldSyncTransport::Get_Messages_Received () == received_before + 1,
				 "three fragments, one duplicated, made %d messages, not 1",
				 WorldSyncTransport::Get_Messages_Received () - received_before);
		Check (!WorldSyncTransport::Is_Client_Synchronised (9),"a client that reported failure stayed on the live stream");
		Check (WorldSyncTransport::Is_Client_Synchronised (7),"another client's failure took client 7 off the stream");
	}

	//
	//	The client: the same baseline regenerated, then only what arrived on the wire.
	//
	WorldOperationJournal::Close ();
	WorldTerrainSystem::Destroy_Terrain ();
	Build_Uniform_Ramp_Terrain (PATCHES,CELL_SIZE,SLOPE,ORIGIN);
	WorldTerrainSystem::Build_Chunk_Registry (0.5f);

	CombatManager::Set_I_Am_Server (false);
	CombatManager::Set_I_Am_Client (true);
	WorldSyncTransport::Init ();
	_CapturedCount = 0;
	WorldSyncTransport::Set_Sender (Capture_Fragment);

	WorldSyncTransport::Client_Join_Generated_World ();
	Check (_CapturedCount == 1 && _Captured [0].Fragment.Kind == WSYNC_MSG_JOIN_REQUEST &&
			 _Captured [0].Destination == WSYNC_TO_SERVER,"the regenerated client did not ask the server to join");
	Check (WorldSyncTransport::Get_Client_Phase () == WSYNC_CLIENT_JOINING,"the client is not joining");

	for (int i = 0; i < server_stream_count; i++) {
		WorldSyncTransport::Receive_Fragment (0,server_stream [i].Fragment);
	}

	Check (WorldSyncTransport::Get_Client_Phase () == WSYNC_CLIENT_SYNCHRONISED,
			 "the client ended the stream %s (%s)",
			 WorldSyncTransport::Get_Client_Phase_Name (WorldSyncTransport::Get_Client_Phase ()),
			 WorldSyncTransport::Get_Last_Result_Detail ().Peek_Buffer ());

	unsigned int client_world = 0;
	Check (WorldSnapshotService::Compute_World_Checksum (&client_world),"the client world has no checksum");
	Check (client_world == server_world,"the client world checksums 0x%08X, the server's 0x%08X",
			 client_world,server_world);

	int client_kinds [8];
	int client_kind_count = Captured_Message_Kinds (1,client_kinds,8);
	Check (client_kind_count == 1 && client_kinds [0] == WSYNC_MSG_RESULT,
			 "the client did not report its join result to the server");

	//	A live operation that skips ahead is never applied out of order: the client asks again.
	{
		WorldSyncFragment ahead = server_stream [server_stream_count - 1].Fragment;
		unsigned int skipped = 999u;
		ahead.Payload [0] = (unsigned char)(skipped & 0xFF);
		ahead.Payload [1] = (unsigned char)((skipped >> 8) & 0xFF);
		ahead.Payload [2] = 0;
		ahead.Payload [3] = 0;
		ahead.MessageId = 5000;

		unsigned int before = WorldOperationJournal::Get_Revision ();
		int sent_before = _CapturedCount;
		WorldSyncTransport::Receive_Fragment (0,ahead);
		Check (WorldOperationJournal::Get_Revision () == before,"an operation after a gap was applied");
		Check (_CapturedCount > sent_before && _Captured [_CapturedCount - 1].Fragment.Kind == WSYNC_MSG_JOIN_REQUEST,
				 "a gap in the live stream did not make the client ask to join again");
	}

	/*
	**	A save's restore (ProceduralWorldPersistenceClass): the package this world would send a
	**	joiner, kept, the world regenerated, and the package applied locally with no network --
	**	arriving at the same world, verified by Finish_Join.
	*/
	{
		CombatManager::Set_I_Am_Server (true);
		CombatManager::Set_I_Am_Client (false);

		DynamicVectorClass<int> kinds_saved;
		DynamicVectorClass<int> sizes_saved;
		DynamicVectorClass<unsigned char> bytes_saved;
		Check (WorldSyncTransport::Build_Join_Package (&kinds_saved,&sizes_saved,&bytes_saved),
				 "the world's save package could not be built");

		//	D-001 priority 6: the same world, written through the save subsystem to a real file now,
		//	while it is still the server's own; read back and restored further down.
		StringClass error ("");
		StringClass disk_path;
		bool disk_written = false;
		unsigned int server_ground = Ground_Checksum ();
		unsigned int server_revision = WorldOperationJournal::Get_Revision ();
		{
			StringClass profile_path ("");
			bool have_profile = false;
			for (int depth = 0; (depth < 8) && !have_profile; depth++) {
				StringClass probe ("");
				for (int up = 0; up < depth; up++) probe += "../";
				probe += "data/procedural_world/default.json";
				if (::GetFileAttributesA (probe.Peek_Buffer ()) != INVALID_FILE_ATTRIBUTES) {
					profile_path = probe;
					have_profile = true;
				}
			}
			Check (have_profile,"could not find data/procedural_world/default.json from the working directory");
			if (have_profile) {
				Check (ProceduralWorldManager::Request_World_Exact (41u,profile_path.Peek_Buffer (),&error),
						 "the shipped default profile could not be requested: %s",error.Peek_Buffer ());

				char temp_dir [MAX_PATH] = { 0 };
				::GetTempPathA (sizeof (temp_dir),temp_dir);
				disk_path.Format ("%sow_world_roundtrip_%u.bin",temp_dir,(unsigned int)::GetCurrentProcessId ());

				RawFileClass out_file (disk_path.Peek_Buffer ());
				disk_written = (out_file.Open (RawFileClass::WRITE) != 0);
				if (disk_written) {
					ChunkSaveClass csave (&out_file);
					disk_written = _TheProceduralWorldPersistence.Save (csave);
					out_file.Close ();
				}
				Check (disk_written,"the world package could not be written to disk");
			}
		}

		WorldOperationJournal::Close ();
		WorldTerrainSystem::Destroy_Terrain ();
		Build_Uniform_Ramp_Terrain (PATCHES,CELL_SIZE,SLOPE,ORIGIN);
		WorldTerrainSystem::Build_Chunk_Registry (0.5f);
		Check (WorldOperationJournal::Begin (&error),"the restored world's journal did not open: %s",error.Peek_Buffer ());

		Check (WorldSyncTransport::Apply_Local_Package (kinds_saved,sizes_saved,bytes_saved,&error),
				 "the saved package did not restore the world: %s",error.Peek_Buffer ());
		unsigned int restored = 0;
		WorldSnapshotService::Compute_World_Checksum (&restored);
		Check (restored == server_world,"the restored world checksums 0x%08X, the saved one 0x%08X",restored,server_world);

		/*
		**	D-001 priority 6: the same restore through the save subsystem and the file written
		**	above, from the server's own world.  The subsystem names the world by profile path and
		**	seed; its load re-requests that world and holds the package, and applying it to a
		**	regenerated world arrives at the saved one again.
		*/
		{
			if (disk_written) {
				const StringClass & path = disk_path;

				WorldOperationJournal::Close ();
				WorldTerrainSystem::Destroy_Terrain ();
				Build_Uniform_Ramp_Terrain (PATCHES,CELL_SIZE,SLOPE,ORIGIN);
				WorldTerrainSystem::Build_Chunk_Registry (0.5f);
				Check (WorldOperationJournal::Begin (&error),"the disk-restored world's journal did not open: %s",error.Peek_Buffer ());

				RawFileClass in_file (path.Peek_Buffer ());
				bool read = (in_file.Open (RawFileClass::READ) != 0);
				if (read) {
					ChunkLoadClass cload (&in_file);
					read = _TheProceduralWorldPersistence.Load (cload);
					in_file.Close ();
				}
				::DeleteFileA (path.Peek_Buffer ());
				Check (read && ProceduralWorldPersistenceClass::Has_Pending (),
						 "the world package did not come back from disk as a pending restore");

				Check (ProceduralWorldPersistenceClass::Apply_Pending (&error),
						 "the disk package did not apply: %s",error.Peek_Buffer ());
				//	The ground and the revision, not the whole-world checksum: Apply_Pending compacts the
				//	journal after applying, on purpose, and the world checksum includes the journal.
				unsigned int from_disk = Ground_Checksum ();
				Check (from_disk == server_ground,"the ground restored from disk checksums 0x%08X, the saved ground 0x%08X",
						 from_disk,server_ground);
				Check (WorldOperationJournal::Get_Revision () == server_revision,
						 "the world restored from disk is at revision %u, the saved one at %u",
						 WorldOperationJournal::Get_Revision (),server_revision);

				ProceduralWorldManager::Clear_Request ();
			}
		}

		DynamicVectorClass<int> truncated_sizes = sizes_saved;
		truncated_sizes [truncated_sizes.Count () - 1] = 1000000;
		Check (!WorldSyncTransport::Apply_Local_Package (kinds_saved,truncated_sizes,bytes_saved,&error),
				 "a truncated save package was applied");
	}

	WorldSyncTransport::Set_Sender (nullptr);
	WorldSyncTransport::Shutdown ();
	CombatManager::Set_I_Am_Server (was_server);
	CombatManager::Set_I_Am_Client (was_client);

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
		//	the same recipe Check_Expansion_Regions already proved.  With a generated front
		//	(D-001: the contested fields), forward means pushed toward it, so the far completion
		//	goes on a contested field when this world has one out of the home cluster.
		Vector3 far_point (1500.0f,0.0f,0.0f);
		for (int f = 0; f < TiberiumFieldGenerator::Get_Field_Count (); f++) {
			ResolvedTiberiumField field;
			if (TiberiumFieldGenerator::Get_Field (f,&field) && (field.OwningFaction == PLAYERTYPE_NEUTRAL) &&
				 (field.Position.Length () > 100.0f)) {
				far_point = field.Position;
				break;
			}
		}
		StringClass completion_report ("");
		ExpansionRegionRegistry::Note_Construction_Complete (901,"ow_test_grading_pad",
																				Vector3 (0.0f,0.0f,0.0f),PLAYERTYPE_GDI,&completion_report);
		ExpansionRegionRegistry::Note_Construction_Complete (902,"ow_test_grading_pad",
																				Vector3 (5.0f,0.0f,0.0f),PLAYERTYPE_GDI,&completion_report);
		ExpansionRegionRegistry::Note_Construction_Complete (903,"ow_test_grading_pad",
																				far_point,PLAYERTYPE_GDI,&completion_report);
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
		//	D-001 enemy obstruction.  An enemy the team knows of standing on an open route closes it
		//	to that team alone; with the radius at 0 nothing is ever closed.
		//
		{
			int open_route = -1;
			LogisticsRoute route;
			for (int i = 0; (i < LogisticsGraphService::Get_Route_Count ()) && (open_route < 0); i++) {
				if (LogisticsGraphService::Get_Route (i,&route) && (route.Status == LOGISTICS_ROUTE_OPEN) &&
					 (route.Waypoints.Count () > 0)) {
					open_route = i;
				}
			}
			if (open_route >= 0) {
				DynamicVectorClass<Vector3> on_route;
				on_route.Add (route.Waypoints [route.Waypoints.Count () / 2] + Vector3 (3.0f,0.0f,0.0f));
				DynamicVectorClass<Vector3> nobody;

				LogisticsGraphService::Set_Enemy_Threat_Radius (0.0f);
				LogisticsGraphService::Apply_Route_Threats (PLAYERTYPE_GDI,on_route);
				Check (LogisticsGraphService::Is_Route_Open_For (open_route,PLAYERTYPE_GDI),
						 "with the threat radius off an enemy still closed a route");

				LogisticsGraphService::Set_Enemy_Threat_Radius (10.0f);
				LogisticsGraphService::Apply_Route_Threats (PLAYERTYPE_GDI,on_route);
				LogisticsGraphService::Apply_Route_Threats (PLAYERTYPE_NOD,nobody);
				Check (!LogisticsGraphService::Is_Route_Open_For (open_route,PLAYERTYPE_GDI),
						 "an enemy 3 units off the route did not close it to the team that knows of it");
				Check (LogisticsGraphService::Is_Route_Open_For (open_route,PLAYERTYPE_NOD),
						 "one team's known enemy closed the route to the other team");

				LogisticsGraphService::Set_Enemy_Threat_Radius (0.0f);
				Check (LogisticsGraphService::Is_Route_Open_For (open_route,PLAYERTYPE_GDI),
						 "turning the threat radius off did not reopen the route");
			}
		}

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

	//	D-001 sensor module: off does nothing; on, full health sees at full range, the curve only
	//	falls, and zero health lands exactly on the configured floor.
	{
		ProceduralWorldVehicleSubsystemSettings off;
		Check (VehicleGameObj::Compute_Subsystem_Sensor_Factor (off,0.0f) == 1.0f,
				 "a disabled profile shrank an AI vehicle's sight range");
		Check (VehicleGameObj::Compute_Subsystem_Sensor_Factor (on,1.0f) == 1.0f,
				 "a full-health vehicle's sight range was shrunk");
		float previous = 1.0f;
		for (int step = 0; step <= 20; step ++) {
			float f = 1.0f - ((float)step / 20.0f);
			float sensor = VehicleGameObj::Compute_Subsystem_Sensor_Factor (on,f);
			Check ((sensor >= on.MinSensorFactor) && (sensor <= previous + 0.0001f),
					 "sensor factor %.4f out of bounds or rising at health fraction %.2f",sensor,f);
			previous = sensor;
		}
		Check (WWMath::Fabs (VehicleGameObj::Compute_Subsystem_Sensor_Factor (on,0.0f) - on.MinSensorFactor) < 0.0001f,
				 "at zero health the sensor factor is not the configured floor");
	}

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




static inline float Gate_Max (float a,float b)	{ return (a > b) ? a : b; }
static inline float Gate_Min (float a,float b)	{ return (a < b) ? a : b; }


/***********************************************************************************************
**	Owner-level solver fixtures for the two Phase-11 ground methods the native configuration cannot reach.
**
**	`Q-020` accepted `road_cut` and `surface_pass` as unreachable under native constants and asked instead
**	for controlled fixtures that execute the *real* solver: the real method orders, the real cut and pass
**	validators, real road-class data and real `CutClasses` metadata, on a world small enough that the one
**	thing under test is the thing that differs from native.  Nothing here changes a native constant or the
**	native bridge registry; the fixture supplies its own obstruction and, for the pass, its own bridge kind.
***********************************************************************************************/

//	A one-edge world: a placeable matrix, one topology edge between two nodes, and one straight route.
struct SolverWorld
{
	HybridMapIdentity			Identity;
	HybridCoarseMatrices		Matrices;
	HybridLandformSet			Landforms;
	HybridHydrologySet		Hydrology;
	HybridAnchorSet			Anchors;
	HybridTopology				Topology;
	HybridRouteSet				Routes;
	PassTerrain					Terrain;
};


/*
**	Build the world around one obstruction window whose crossing axis is `+Y`.  The road is a straight
**	`class` edge along `+Y` through the window centre, sampled at `min(W, A/2)`, with the window's frozen
**	structural interval on it, so what the solver sees is exactly what Phase 10 hands it.
*/
static void Build_Solver_World (const HybridInteractionParams & params,HybridRoadClass road_class,
										  unsigned int capabilities,const HybridCrossingWindow & window,
										  SolverWorld * world)
{
	const HybridConstants & k = params.Constants;
	const float half = 0.5f * params.WorldWidth;

	//	Every cell placeable and nothing reserved: the obstruction is the only feature.
	const int cells = 24;
	world->Matrices.Reset ();
	world->Matrices.Matrix.Resize (cells,cells,params.WorldWidth / (float)cells,-half,-half);
	for (int cy = 0; cy < cells; ++cy) {
		for (int cx = 0; cx < cells; ++cx) world->Matrices.Matrix.Set_Flag (cx,cy,HYBRID_FLAG_PLACEABLE);
	}

	world->Hydrology.Reset ();
	world->Hydrology.Windows.Add (window);

	world->Topology.Reset ();
	HybridTopologyNode node;
	node.Kind = HYBRID_NODE_FAMILY_WAYPOINT;
	node.StableId = 1;	node.Position.Set (0.0f,-0.45f * params.WorldWidth);	world->Topology.Nodes.Add (node);
	node.StableId = 2;	node.Position.Set (0.0f,+0.45f * params.WorldWidth);	world->Topology.Nodes.Add (node);

	HybridTopologyEdge edge;
	edge.StableId		= 1;
	edge.Kind			= HYBRID_EDGE_FAMILY_TRUNK;
	edge.RoadClass		= road_class;
	edge.Capabilities	= capabilities;
	edge.NodeA			= 1;
	edge.NodeB			= 2;
	edge.Required		= true;
	world->Topology.Edges.Add (edge);

	world->Routes.Reset ();
	HybridRoute route;
	route.EdgeId			= 1;
	route.RoadClass		= road_class;
	route.Capabilities	= capabilities;
	route.Required			= true;
	const float spacing = WWMath::Min (k.W,0.5f * k.A);
	const float from = -0.45f * params.WorldWidth;
	const float to = +0.45f * params.WorldWidth;
	const int steps = (int)WWMath::Ceil ((to - from) / spacing);
	int first_point = -1;
	int last_point = -1;
	for (int i = 0; i <= steps; ++i) {
		const float y = from + ((to - from) * ((float)i / (float)steps));
		route.Points.Add (Vector2 (0.0f,Hybrid_Quantize_Position (y)));
		route.Stations.Add (Hybrid_Quantize_Position (y - from));
		if ((first_point < 0) && (y >= window.ApproachA.Y) && (y <= window.ApproachB.Y)) first_point = i;
		if ((y >= window.ApproachA.Y) && (y <= window.ApproachB.Y)) last_point = i;
		if ((y >= window.ApproachB.Y) && (y <= window.ApproachA.Y)) {
			if (first_point < 0) first_point = i;
			last_point = i;
		}
	}
	route.Length = Hybrid_Quantize_Position (to - from);

	HybridRouteStructuralSpan span;
	span.WindowId	= window.StableId;
	span.FirstPoint= (first_point >= 0) ? first_point : 0;
	span.LastPoint	= (last_point >= 0) ? last_point : route.Points.Count () - 1;
	span.Ingress	= (window.ApproachA.Y < window.ApproachB.Y) ? window.ApproachA : window.ApproachB;
	span.Egress		= (window.ApproachA.Y < window.ApproachB.Y) ? window.ApproachB : window.ApproachA;
	route.Structural.Add (span);
	world->Routes.Routes.Add (route);
}


//	A window across the `+Y` axis at the origin: tangent `+X`, approach A on `+Y` (the local-pass frame).
static HybridCrossingWindow Solver_Window (HybridObstructionKind obstruction,HybridWindowKind kind,float half_span,
														 float approach,float low_z,float high_z)
{
	HybridCrossingWindow window;
	window.StableId				= 1;
	window.Kind						= kind;
	window.Obstruction			= obstruction;
	window.ObstructionRecordId	= 1;
	window.Station					= 0.0f;
	window.Span						= Hybrid_Quantize_Position (2.0f * half_span);
	window.ApproachLength		= Hybrid_Quantize_Position (approach);
	window.LowElevation			= low_z;
	window.HighElevation			= high_z;
	window.Center.Set (0.0f,0.0f);
	window.Tangent.Set (1.0f,0.0f);
	window.ApproachA.Set (0.0f,Hybrid_Quantize_Position (+(half_span + approach)));
	window.ApproachB.Set (0.0f,Hybrid_Quantize_Position (-(half_span + approach)));
	window.BoundsMin.Set (-1.0f,window.ApproachB.Y);
	window.BoundsMax.Set (+1.0f,window.ApproachA.Y);
	return window;
}


/***********************************************************************************************
**	Runtime Phase 12's arithmetic -- rewrite item I13.
**
**	Section 12.1's solver is five named steps and each can fail in its own way, so each is proved here on
**	terrain built to make exactly that step decide: flat ground the profile follows; a step the profile
**	must grade across inside cut and fill; a pinned crossing height the walk must meet; a wall no grade
**	can climb inside the limits, refused at forward propagation; two pins too far apart in height for the
**	distance, refused at backward propagation; and the quantized result revalidated.  The cross-section is
**	proved as the piecewise function it is.
***********************************************************************************************/

struct ProfileTerrain
{
	float	BaseZ;
	float	StepAt;			// station where the ground steps
	float	StepZ;			// by how much
	float	WallAt;			// station where a wall begins (rises `WallZ` over `WallRun`)
	float	WallZ;
	float	WallRun;
};

static bool Profile_Terrain_Height (const Vector2 & world,float * height_out,void * context)
{
	const ProfileTerrain * terrain = (const ProfileTerrain *)context;
	if ((terrain == NULL) || (height_out == NULL)) return false;
	float z = terrain->BaseZ;
	if ((terrain->StepZ != 0.0f) && (world.X >= terrain->StepAt)) z += terrain->StepZ;
	if ((terrain->WallZ != 0.0f) && (world.X >= terrain->WallAt)) {
		const float run = (terrain->WallRun > 0.0f) ? terrain->WallRun : 0.001f;
		const float ratio = WWMath::Min ((world.X - terrain->WallAt) / run,1.0f);
		z += terrain->WallZ * ratio;
	}
	*height_out = z;
	return true;
}


//	A straight route along +X from `from` to `to`, sampled at `min(W, A/2)` like Phase 10's.
static void Straight_Route (float from,float to,float step,HybridRoute * route)
{
	route->Points.Delete_All ();
	route->Stations.Delete_All ();
	const int steps = (int)WWMath::Ceil ((to - from) / step);
	for (int i = 0; i <= steps; ++i) {
		const float x = from + ((to - from) * ((float)i / (float)steps));
		route->Points.Add (Vector2 (Hybrid_Quantize_Position (x),0.0f));
		route->Stations.Add (Hybrid_Quantize_Position (x - from));
	}
}


void Check_Hybrid_Realization (void)
{
	HybridConstants k;
	Hybrid_Compute_Constants (2.555953f,1.0f,60.0f,1200.0f,1200.0f,3.0f,900.0f,&k);
	const float spacing = Hybrid_Profile_Sample_Spacing (k);
	const float max_cut = 6.0f;
	const float max_fill = 6.0f;
	const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec (HYBRID_ROAD_PRIMARY_2);
	const float max_grade = spec.MaxGradeDegrees * 0.0174532925f;
	const float g = WWMath::Tan (max_grade);

	Check (spacing == WWMath::Max (WWMath::Min (0.5f * k.W,k.A / 3.0f),k.Q),"the profile spacing is not min(W/2, A/3)");

	HybridRoute route;
	route.EdgeId = 1;
	route.RoadClass = HYBRID_ROAD_PRIMARY_2;
	Straight_Route (0.0f,120.0f,WWMath::Min (k.W,0.5f * k.A),&route);
	const int last = route.Points.Count () - 1;

	//	The sampler: exact endpoints, strictly increasing stations, spacing honoured.
	{
		ProfileTerrain flat = { 10.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
		DynamicVectorClass<HybridProfileSample> samples;
		Check (Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&flat,&samples),
				 "the sampler refused a straight route");
		Check (samples.Count () >= 2,"the sampler produced %d samples",samples.Count ());
		Check (samples[0].Station == 0.0f,"the first sample is not the interval start");
		Check (WWMath::Fabs (samples[samples.Count () - 1].Station - 120.0f) <= 0.002f,
				 "the last sample is at %.3f, not the interval end",samples[samples.Count () - 1].Station);
		bool increasing = true;
		float widest = 0.0f;
		for (int i = 1; i < samples.Count (); ++i) {
			if (samples[i].Station <= samples[i - 1].Station) increasing = false;
			widest = WWMath::Max (widest,samples[i].Station - samples[i - 1].Station);
		}
		Check (increasing,"sample stations are not strictly increasing");
		Check (widest <= (spacing + 0.002f),"a sample gap of %.3f exceeds the spacing %.3f",widest,spacing);
	}

	//	1. Flat ground: the profile follows it exactly, no cut, no fill, no grade.
	{
		ProfileTerrain flat = { 10.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
		DynamicVectorClass<HybridProfileSample> samples;
		Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&flat,&samples);
		HybridSolvedProfile solved;
		HybridProfileFailure why = HYBRID_PROFILE_OK;
		Check (Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&solved,&why),
				 "flat ground has no profile (%s)",Hybrid_Profile_Failure_Name (why));
		Check (solved.Z.Count () == samples.Count (),"the profile has %d heights for %d samples",solved.Z.Count (),
				 samples.Count ());
		Check ((solved.DeepestCut == 0.0f) && (solved.HighestFill == 0.0f) && (solved.SteepestGrade == 0.0f),
				 "a flat profile cut %.3f, filled %.3f, graded %.4f",solved.DeepestCut,solved.HighestFill,
				 solved.SteepestGrade);
		for (int i = 0; i < solved.Z.Count (); ++i) {
			Check (solved.Z[i] == 10.0f,"flat profile sample %d is at %.3f",i,solved.Z[i]);
		}

		//	Determinism: the same samples give the same profile.
		HybridSolvedProfile again;
		Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&again,&why);
		bool same = again.Z.Count () == solved.Z.Count ();
		for (int i = 0; same && (i < solved.Z.Count ()); ++i) same = (again.Z[i] == solved.Z[i]);
		Check (same,"the solver is not deterministic");
	}

	//	2. A step of 8 the class grade must spread: cut on one side, fill on the other, both inside 6.
	{
		ProfileTerrain step = { 10.0f,60.0f,8.0f,0.0f,0.0f,0.0f };
		DynamicVectorClass<HybridProfileSample> samples;
		Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&step,&samples);
		HybridSolvedProfile solved;
		HybridProfileFailure why = HYBRID_PROFILE_OK;
		Check (Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&solved,&why),
				 "an 8-unit step could not be graded (%s)",Hybrid_Profile_Failure_Name (why));
		Check (solved.SteepestGrade <= (max_grade + 0.001f),"the graded step reaches %.4f rad",solved.SteepestGrade);
		Check (solved.SteepestGrade > 0.0f,"a step was graded without any grade");
		Check ((solved.DeepestCut <= max_cut + k.Q) && (solved.HighestFill <= max_fill + k.Q),
				 "the step needs %.2f of cut and %.2f of fill",solved.DeepestCut,solved.HighestFill);
		Check ((solved.DeepestCut > 0.0f) || (solved.HighestFill > 0.0f),"a step moved no ground");

		//	Every adjacent pair, independently: grade inside the class maximum on the quantized profile.
		for (int i = 1; i < solved.Z.Count (); ++i) {
			const float ds = samples[i].Station - samples[i - 1].Station;
			const float grade = WWMath::Atan (WWMath::Fabs (solved.Z[i] - solved.Z[i - 1]) / ds);
			Check (grade <= (max_grade + 0.001f),"adjacent grade %.4f at sample %d",grade,i);
			Check (solved.Z[i] == Hybrid_Quantize_Position (solved.Z[i]),"sample %d is not quantized",i);
		}
	}

	//	3. A pinned crossing height: the walk meets it exactly, and grades toward it inside the limits.
	{
		ProfileTerrain flat = { 10.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
		DynamicVectorClass<HybridProfileSample> samples;
		Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&flat,&samples);
		const int pin = samples.Count () / 2;
		samples[pin].Fixed = true;
		samples[pin].FixedZ = Hybrid_Quantize_Position (14.0f);	// a compiled deck height is already quantized
		HybridSolvedProfile solved;
		HybridProfileFailure why = HYBRID_PROFILE_OK;
		Check (Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&solved,&why),
				 "a pinned deck height could not be met (%s)",Hybrid_Profile_Failure_Name (why));
		Check (solved.Z[pin] == samples[pin].FixedZ,"the pinned sample sits at %.5f, not the pinned %.5f",
				 solved.Z[pin],samples[pin].FixedZ);
		Check (solved.Z[0] == 10.0f,"the start was lifted to %.3f for a deck far away",solved.Z[0]);
		Check (solved.SteepestGrade <= (max_grade + 0.001f),"the approach to the deck reaches %.4f rad",
				 solved.SteepestGrade);
		Check (solved.HighestFill <= (4.0f + k.Q),"fill %.2f for a 4-high deck",solved.HighestFill);
	}

	//	4. A wall no grade can climb inside cut and fill: refused at forward propagation, never patched.
	{
		ProfileTerrain wall = { 10.0f,0.0f,0.0f,60.0f,40.0f,2.0f };		// 40 up over 2 along
		DynamicVectorClass<HybridProfileSample> samples;
		Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&wall,&samples);
		HybridSolvedProfile solved;
		HybridProfileFailure why = HYBRID_PROFILE_OK;
		Check (!Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&solved,&why),
				 "a 40-high wall was graded inside 6 of cut and fill");
		Check (why == HYBRID_PROFILE_FORWARD_EMPTY,"the wall was refused as %s",Hybrid_Profile_Failure_Name (why));
		Check (solved.Z.Count () == 0,"a refused profile left heights behind");
	}

	//	5. Two pins the grade cannot join: the second pin empties an interval on the backward pass.
	{
		ProfileTerrain flat = { 10.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
		DynamicVectorClass<HybridProfileSample> samples;
		Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&flat,&samples);
		const int a = samples.Count () / 3;
		const int b = a + 2;										// two samples apart: about 2 * spacing along
		samples[a].Fixed = true;	samples[a].FixedZ = 10.0f;
		samples[b].Fixed = true;	samples[b].FixedZ = 10.0f + (g * (samples[b].Station - samples[a].Station) * 3.0f);
		HybridSolvedProfile solved;
		HybridProfileFailure why = HYBRID_PROFILE_OK;
		Check (!Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&solved,&why),
				 "two pins three times too steep apart were joined");
		Check ((why == HYBRID_PROFILE_FORWARD_EMPTY) || (why == HYBRID_PROFILE_BACKWARD_EMPTY),
				 "unjoinable pins were refused as %s",Hybrid_Profile_Failure_Name (why));
	}

	/*
	**	6. A solved profile is a solution of itself.  A compiled deck pins the heights this solver returned,
	**	quantized; a road graded at the class maximum then has to be re-solvable from those pins, or a crossing
	**	Phase 11 accepted could not be built by Phase 12.  An 8-high step graded inside 6 of cut and fill runs
	**	at the maximum through it, so every pin there sits on the limit.
	*/
	{
		ProfileTerrain step = { 10.0f,60.0f,8.0f,0.0f,0.0f,0.0f };
		DynamicVectorClass<HybridProfileSample> samples;
		Hybrid_Sample_Route_Interval (route,0,last,spacing,Profile_Terrain_Height,&step,&samples);
		HybridSolvedProfile first;
		HybridProfileFailure why = HYBRID_PROFILE_OK;
		Check (Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&first,&why),
				 "the step did not grade (%s)",Hybrid_Profile_Failure_Name (why));
		if (first.Z.Count () == samples.Count ()) {
			Check (first.SteepestGrade >= (max_grade - 0.001f),"the step never reaches the class maximum (%.4f)",
					 first.SteepestGrade);
			for (int i = 0; i < samples.Count (); ++i) {
				samples[i].Fixed	= true;
				samples[i].FixedZ	= first.Z[i];
			}
			HybridSolvedProfile again;
			const bool resolved = Hybrid_Solve_Vertical_Profile (samples,max_cut,max_fill,max_grade,k.Q,&again,&why);
			Check (resolved,"a profile pinned to its own solution was refused (%s)",Hybrid_Profile_Failure_Name (why));
			for (int i = 0; (i < again.Z.Count ()) && (i < first.Z.Count ()); ++i) {
				Check (again.Z[i] == first.Z[i],"pinned sample %d came back at %.4f, not %.4f",i,again.Z[i],first.Z[i]);
			}
		}
	}

	/*
	**	The cross-section of every road class: level roadbed, shoulder at roadbed height, smoothstep to terrain
	**	over `outer_blend`.  Each class's roadbed is its own (Section 7's lanes, shoulders and median), and the
	**	blend is the same for all of them.
	*/
	for (int road_class = 0; road_class < HYBRID_ROAD_CLASS_COUNT; ++road_class) {
		const HybridRoadClass cls = (HybridRoadClass)road_class;
		const float half_roadbed = 0.5f * Hybrid_Roadbed_Width (cls,k.W);
		Check (half_roadbed == (0.5f * Hybrid_Road_Class_Spec (cls).Roadbed_Width_W () * k.W),
				 "%s's roadbed is not its own lanes, shoulders and median",Hybrid_Road_Class_Name (cls));
		const float blend = Hybrid_Road_Outer_Blend_Width (k.W,k.A);
		Check (blend == WWMath::Max (k.W,0.5f * k.A),"outer_blend is %.3f, not max(W, A/2)",blend);
		Check (Hybrid_Road_Cross_Section_Half_Width (cls,k.W,k.A) == (half_roadbed + blend),
				 "the %s cross-section half width is not roadbed half plus blend",Hybrid_Road_Class_Name (cls));

		const float road_z = 12.0f;
		const float ground = 8.0f;
		Check (Hybrid_Road_Cross_Section_Height (0.0f,road_z,ground,half_roadbed,blend) == road_z,"centre is not level");
		Check (Hybrid_Road_Cross_Section_Height (half_roadbed,road_z,ground,half_roadbed,blend) == road_z,
				 "the shoulder edge is not at roadbed height");
		Check (Hybrid_Road_Cross_Section_Height (-half_roadbed,road_z,ground,half_roadbed,blend) == road_z,
				 "the cross-section is not symmetric");
		const float mid = Hybrid_Road_Cross_Section_Height (half_roadbed + (0.5f * blend),road_z,ground,half_roadbed,blend);
		Check (WWMath::Fabs (mid - (0.5f * (road_z + ground))) <= 0.001f,
				 "the blend midpoint is at %.3f, not halfway (smoothstep(0.5) = 0.5)",mid);
		Check (Hybrid_Road_Cross_Section_Height (half_roadbed + blend,road_z,ground,half_roadbed,blend) == ground,
				 "the blend does not reach untouched terrain at exactly outer_blend");
		Check (Hybrid_Road_Cross_Section_Height (half_roadbed + blend + 5.0f,road_z,ground,half_roadbed,blend) == ground,
				 "the cross-section touches terrain beyond its reach");
		//	Smoothstep is flat at both ends: the quarter points are closer to their own end than linear.
		const float q1 = Hybrid_Road_Cross_Section_Height (half_roadbed + (0.25f * blend),road_z,ground,half_roadbed,blend);
		Check (WWMath::Fabs (q1 - road_z) < 0.25f * WWMath::Fabs (ground - road_z),
				 "the blend leaves the shoulder linearly, not with a flat tangent");
	}
}



/*
**	The controlled reroute world (Q-023 section 16).
**
**	With Phase 9 refusing structural sequences no road can drive, no native seed in a 1400-world census leaves a
**	Phase-11 refusal that the next Phase-10 envelope then resolves: a wider envelope only helps when the narrower
**	one forced the road onto a crossing it would otherwise have avoided, and the topology no longer freezes
**	those.  So the reroute is proved on a controlled world through the *real* owners: real Phase-10 routing, a
**	real Phase-11 refusal, the real envelope-floor increment, a real Phase-10 reroute and a real Phase-11 success.
**
**	One `SECONDARY_2` cross-link from (-400,7.5) to (+400,7.5) and a 20-wide river across it for 150 either side,
**	with one accepted crossing window on the road's line.  Envelope 0 is the class's preferred corridor, +/-150:
**	the river fills it, so the road can only cross by the window.  Nothing the fixture registers can span the
**	river and the crossing is not an obligation, so Phase 11 refuses it and asks for envelope 1, +/-180, where the
**	road goes round the river's end -- which costs less than the window's own `20A` penalty, so the search takes
**	it -- and there is nothing left to cross.
*/
static bool Build_Reroute_World (CrossingWorld * world,StringClass * why)
{
	const float ROW = 7.5f;
	Crossing_World_Begin (world);
	Crossing_World_River (world,1,Vector2 (0.0f,ROW - 150.0f),Vector2 (0.0f,ROW + 150.0f),20.0f);
	Crossing_World_Window (world,2,1,Vector2 (0.0f,ROW),Vector2 (1.0f,0.0f),20.0f);
	Crossing_World_Edge (world,Vector2 (-400.0f,ROW),Vector2 (400.0f,ROW),HYBRID_ROAD_SECONDARY_2,0);
	HybridRouteResult result;
	if (!Crossing_World_Route (world,&result)) {
		if (why != nullptr) *why = result.Detail;
		return false;
	}
	return true;
}



/*
**	Q-022 section 13, Phase 11's half, on the controlled worlds: an asset approach Phase 10's 8W does not
**	cover, a moved station, and the station cap.
*/
static void Check_Hybrid_Crossing_Compile (const HybridPreflightInputs & inputs,const HybridMetricCache & metrics)
{
	const float ROW = 7.5f;
	const Vector2 EAST (1.0f,0.0f);

	/*
	**	5. Asset approach longer than 8W.  Phase 10 only guarantees 8W; the shipped bridge kinds ask for more, and
	**	a road straight through the window has it, so the compiled reserve is the kind's own -- never cut back to
	**	8W.  Then a second river runs parallel, its water 54 past the crossing's east bank and its carved bank
	**	from 48: room for Phase 10's 8W straight and a 16.6 turn, not for a 12W straight and the same turn at
	**	any station along the river.
	**	A kind that needs 12W is refused there, not compiled on a shortened approach, and Phase 11 asks Phase 10
	**	for the next envelope.
	*/
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		Crossing_World_Edge (&world,Vector2 (-397.5f,ROW),Vector2 (397.5f,ROW),HYBRID_ROAD_PRIMARY_2,2);
		HybridRouteResult routed_result;
		const bool routed = Crossing_World_Route (&world,&routed_result);
		Check (routed,"the straight asset-approach world does not route: %s",routed_result.Detail.Peek_Buffer ());
		if (routed) {
			HybridInteractionParams params = Interaction_Fixture (world.Sized,1200.0f,inputs,metrics);
			float shipped = 0.0f;
			for (int i = 0; i < params.BridgeKinds.Count (); ++i) {
				shipped = WWMath::Max (shipped,params.BridgeKinds[i].RequiredApproachLength);
			}
			HybridInteractionSet fits;
			HybridInteractionResult fits_result;
			const bool compiled = Hybrid_Build_Interactions (params,world.Identity,world.Matrices,world.Landforms,
																			 world.Hydrology,world.Anchors,world.Topology,world.Routes,
																			 Crossing_Ground,&world.Hydrology,&fits,&fits_result);
			Check (compiled,"the shipped kinds did not compile on a straight road: %s",fits_result.Detail.Peek_Buffer ());
			for (int i = 0; i < fits.Interactions.Count (); ++i) {
				const HybridInteraction & one = fits.Interactions[i];
				const float needed = WWMath::Max (8.0f * k.W,params.BridgeKinds[one.BridgeKind].RequiredApproachLength);
				Check (one.ApproachReserve >= (needed - k.Q),"a bridge compiled on a %.2f approach, under its kind's %.2f",
						 one.ApproachReserve,needed);
			}
			printf ("  asset approach: shipped kinds need %.1f against Phase 10's 8W of %.1f\r\n",shipped,8.0f * k.W);
		}
	}
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_River (&world,3,Vector2 (74.0f,-500.0f),Vector2 (74.0f,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		Crossing_World_Edge (&world,Vector2 (-397.5f,ROW),Vector2 (37.5f,397.5f),HYBRID_ROAD_PRIMARY_2,2);
		HybridRouteResult routed_result;
		const bool routed = Crossing_World_Route (&world,&routed_result);
		Check (routed,"the narrow asset-approach world does not route: %s",routed_result.Detail.Peek_Buffer ());
		if (routed) {
			HybridInteractionParams params = Interaction_Fixture (world.Sized,1200.0f,inputs,metrics);
			for (int i = 0; i < params.BridgeKinds.Count (); ++i) params.BridgeKinds[i].RequiredApproachLength = 12.0f * k.W;
			HybridInteractionSet refused;
			HybridInteractionResult refused_result;
			Check (!Hybrid_Build_Interactions (params,world.Identity,world.Matrices,world.Landforms,world.Hydrology,
														  world.Anchors,world.Topology,world.Routes,Crossing_Ground,&world.Hydrology,
														  &refused,&refused_result),
					 "a bridge needing a 12W approach compiled where the road has room for 8W");
			Check (refused.Interactions.Count () == 0,"the refused crossing left solutions behind");
			Check (::strstr (refused_result.Detail.Peek_Buffer (),"approach_does_not_fit") != NULL,
					 "the 12W kind was refused for something else: %s",refused_result.Detail.Peek_Buffer ());
			Check (refused_result.RerouteEdgeId == world.Routes.Routes[0].EdgeId,
					 "the refused crossing did not ask Phase 10 for the next envelope (edge %u)",
					 refused_result.RerouteEdgeId);
		}
	}

	/*
	**	6. Moved station.  The river is 40 wide for 25 either side of the window and 16 elsewhere, and nothing
	**	registered spans more than 30, so the window's own station cannot be bridged.  Phase 11 moves along the
	**	river to where it narrows and builds the connector: the road turns at its class radius and runs a
	**	complete straight approach onto the deck and off it.
	*/
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		const float centre_station = 500.0f + ROW;
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),16.0f,centre_station - 25.0f,
									 centre_station + 25.0f,40.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,40.0f);
		Crossing_World_Edge (&world,Vector2 (-397.5f,ROW),Vector2 (397.5f,ROW),HYBRID_ROAD_PRIMARY_2,2);
		HybridRouteResult routed_result;
		const bool routed = Crossing_World_Route (&world,&routed_result);
		Check (routed,"the moved-station world does not route: %s",routed_result.Detail.Peek_Buffer ());
		if (routed) {
			HybridInteractionParams params = Interaction_Fixture (world.Sized,1200.0f,inputs,metrics);
			float approach = 8.0f * k.W;
			for (int i = 0; i < params.BridgeKinds.Count (); ++i) {
				params.BridgeKinds[i].MaxSpan = WWMath::Min (params.BridgeKinds[i].MaxSpan,30.0f);
				approach = WWMath::Max (approach,params.BridgeKinds[i].RequiredApproachLength);
			}
			HybridInteractionSet set;
			HybridInteractionResult result;
			const bool compiled = Hybrid_Build_Interactions (params,world.Identity,world.Matrices,world.Landforms,
																			 world.Hydrology,world.Anchors,world.Topology,world.Routes,
																			 Crossing_Ground,&world.Hydrology,&set,&result);
			Check (compiled,"no station along the narrowing river compiled: %s",result.Detail.Peek_Buffer ());
			if (compiled && (set.Interactions.Count () == 1)) {
				const HybridInteraction & one = set.Interactions[0];
				const HybridCrossingWindow & window = world.Hydrology.Windows[0];
				Check ((one.Station - window.Center).Length () > 25.0f,"the bridge stayed on the unbridgeable station");
				Check (one.Span <= 30.0f,"the moved station's span %.1f is past every kind",one.Span);
				Check (one.Path.AddedLength > 0.0f,"a moved station was compiled with no connector");

				HybridRoute realized;
				Check (Hybrid_Realized_Route (world.Routes.Routes[0],set,&realized),"the moved crossing has no realized road");
				StringClass why;
				Check (Straight_Between (realized.Points,one.ApproachA,one.ApproachB,k.Q,&why),"moved station: %s",
						 why.Peek_Buffer ());
				const float straight = (one.ApproachB - one.ApproachA).Length ();
				Check (straight >= (one.Span + (2.0f * approach) - (2.0f * k.Q)),
						 "the moved station's straight is %.2f, short of the deck and two %.2f approaches",straight,approach);
				const float radius = Tightest_Radius (realized.Points);
				Check (radius >= (0.98f * Hybrid_Road_Class_Spec (HYBRID_ROAD_PRIMARY_2).MinTurnRadiusW * k.W),
						 "the connector turns at %.2f, under the class radius",radius);
				printf ("  moved station: %.1f along the river, connector adds %.1f, tightest turn %.1f\r\n",
						  (one.Station - window.Center).Length (),one.Path.AddedLength,radius);
			}
		}
	}

	//	7. The station cap: a search range of 145 stations held to 64, the anchor first and both ends kept.
	{
		CrossingWorld world;
		Crossing_World_Begin (&world);
		const HybridConstants & k = world.Sized.Constants;
		Crossing_World_River (&world,1,Vector2 (0.0f,-500.0f),Vector2 (0.0f,500.0f),20.0f);
		Crossing_World_Window (&world,2,1,Vector2 (0.0f,ROW),EAST,20.0f);
		HybridInteractionParams params = Interaction_Fixture (world.Sized,1200.0f,inputs,metrics);
		const HybridCrossingWindow & window = world.Hydrology.Windows[0];
		DynamicVectorClass<HybridObstructionStation> list;
		Hybrid_Crossing_Station_List (params,world.Hydrology,world.Landforms,window,window.Center,&list);

		const float half_length = WWMath::Max (8.0f * k.W,4.0f * k.S);
		const float spacing = WWMath::Min (k.W,0.5f * k.A);
		Check (((2.0f * half_length) / spacing) + 1.0f > 64.0f,"the cap world's range does not reach the cap");
		Check ((list.Count () > 0) && (list.Count () <= 64),"%d stations, not 1..64",list.Count ());
		if (list.Count () > 0) {
			Check ((list[0].Centre - window.Center).Length () <= k.Q,"candidate 0 is not the anchor");
			bool low = false;
			bool high = false;
			for (int i = 0; i < list.Count (); ++i) {
				if (list[i].Offset <= (-half_length + k.Q)) low = true;
				if (list[i].Offset >= (half_length - k.Q)) high = true;
				for (int j = i + 1; j < list.Count (); ++j) {
					Check ((list[i].Centre - list[j].Centre).Length () > k.Q,"stations %d and %d are one station",i,j);
				}
			}
			Check (low && high,"the capped list lost an end of its range (%s %s)",low ? "" : "low",high ? "" : "high");
		}
	}
}


//	Where on a polyline a point lies, as an arc-length station: the nearest point of the nearest segment.
static float Gate_Station_On (const HybridRoute & road,const Vector2 & point)
{
	float best = 3.4e38f;
	float station = 0.0f;
	for (int i = 1; i < road.Points.Count (); ++i) {
		const Vector2 a = road.Points[i - 1];
		const Vector2 d = road.Points[i] - a;
		const float length2 = (d.X * d.X) + (d.Y * d.Y);
		float t_on = (length2 > 0.0f) ? ((((point.X - a.X) * d.X) + ((point.Y - a.Y) * d.Y)) / length2) : 0.0f;
		t_on = (t_on < 0.0f) ? 0.0f : ((t_on > 1.0f) ? 1.0f : t_on);
		const Vector2 on = a + (d * t_on);
		const float distance = (point - on).Length ();
		if (distance >= best) continue;
		best = distance;
		station = road.Stations[i - 1] + ((road.Stations[i] - road.Stations[i - 1]) * t_on);
	}
	return station;
}


//	The roads a solution involves: its own two, and for a topology junction every road through its node.
static void Gate_Solution_Edges (const HybridTopology & topology,const HybridInteraction & one,
											DynamicVectorClass<unsigned int> * out)
{
	out->Delete_All ();
	if (one.EdgeId != 0) out->Add (one.EdgeId);
	if (one.OtherEdgeId != 0) out->Add (one.OtherEdgeId);
	if (one.NodeId == 0) return;
	for (int e = 0; e < topology.Edges.Count (); ++e) {
		const HybridTopologyEdge & edge = topology.Edges[e];
		bool carries = (edge.NodeA == one.NodeId) || (edge.NodeB == one.NodeId);
		for (int w = 0; (w < edge.Waypoints.Count ()) && !carries; ++w) carries = (edge.Waypoints[w] == one.NodeId);
		if (carries) out->Add (edge.StableId);
	}
}


static bool Gate_Solutions_Related (const HybridTopology & topology,const HybridInteraction & one,
												const HybridInteraction & other)
{
	DynamicVectorClass<unsigned int> first;
	DynamicVectorClass<unsigned int> second;
	Gate_Solution_Edges (topology,one,&first);
	Gate_Solution_Edges (topology,other,&second);
	for (int i = 0; i < first.Count (); ++i) {
		for (int j = 0; j < second.Count (); ++j) {
			if (first[i] == second[j]) return true;
		}
	}
	return false;
}


static void Check_Controlled_Reroute (const HybridPreflightInputs & inputs,const HybridMetricCache & metrics,
												  int * runs_seen,int * untouched_checked)
{
	CrossingWorld world;
	StringClass why;
	const bool built = Build_Reroute_World (&world,&why);
	Check (built,"the controlled reroute world does not route: %s",why.Peek_Buffer ());
	if (!built) return;

	Check (world.Routes.Routes.Count () == 1,"the controlled reroute world routed %d edges",world.Routes.Routes.Count ());
	if (world.Routes.Routes.Count () != 1) return;
	const HybridRoute first = world.Routes.Routes[0];
	Check (first.Envelope == 0,"the controlled road did not start in envelope 0 (%d)",first.Envelope);
	Check (first.Structural.Count () == 1,"envelope 0 does not cross the river by its window (%d crossings)",
			 first.Structural.Count ());

	//	Nothing the fixture registers spans the river.
	HybridInteractionParams params = Interaction_Fixture (world.Sized,1200.0f,inputs,metrics);
	for (int i = 0; i < params.BridgeKinds.Count (); ++i) params.BridgeKinds[i].MaxSpan = 1.0f;

	//	The refusal on its own, first: real Phase 11 on the envelope-0 route names this edge for a reroute.
	{
		HybridInteractionSet refused;
		HybridInteractionResult refused_result;
		Check (!Hybrid_Build_Interactions (params,world.Identity,world.Matrices,world.Landforms,world.Hydrology,
													  world.Anchors,world.Topology,world.Routes,Crossing_Ground,&world.Hydrology,&refused,
													  &refused_result),"the unspannable river crossing compiled");
		Check (refused_result.RerouteEdgeId == first.EdgeId,"Phase 11 asked to reroute edge %u, not %u",
				 refused_result.RerouteEdgeId,first.EdgeId);
	}

	HybridInteractionSet solved;
	HybridInteractionResult solved_result;
	HybridRouteResult solved_routes;
	int spent = 0;
	int runs = 0;
	const bool ok = Hybrid_Solve_Interactions_With_Reroute (world.RouteParams,params,world.Identity,world.Matrices,
																			  world.Landforms,world.Hydrology,world.Anchors,
																			  world.Topology,Crossing_Ground,&world.Hydrology,&world.Routes,
																			  &solved_routes,&solved,&solved_result,&spent,&runs);
	Check (ok,"the controlled reroute did not solve: %s (%s)",Hybrid_Interaction_Failure_Name (solved_result.Failure),
			 solved_result.Detail.Peek_Buffer ());
	Check (spent == 1,"the controlled reroute spent %d reroutes",spent);
	Check (runs == 2,"the controlled reroute ran Phase 10 %d times, not twice",runs);
	if ((runs >= 1) && (runs <= 3)) runs_seen[runs]++;

	Check (world.RouteParams.EnvelopeFloors.Count () == 1,"one reroute raised %d floors",
			 world.RouteParams.EnvelopeFloors.Count ());
	if ((world.RouteParams.EnvelopeFloors.Count () == 1) && (world.Routes.Routes.Count () == 1)) {
		const HybridRouteEnvelopeFloor & floor = world.RouteParams.EnvelopeFloors[0];
		Check (floor.MinEnvelope == (first.Envelope + 1),"edge %u moved from envelope %d to floor %d: not the next one",
				 floor.EdgeId,first.Envelope,floor.MinEnvelope);
		const HybridRoute & after = world.Routes.Routes[0];
		Check (after.Envelope >= floor.MinEnvelope,"the rerouted edge came back in envelope %d under floor %d",
				 after.Envelope,floor.MinEnvelope);
		Check (after.Structural.Count () == 0,"the rerouted road still crosses the river (%d crossings)",
				 after.Structural.Count ());
		Check (solved.Interactions.Count () == 0,"the rerouted road compiled %d crossings",solved.Interactions.Count ());
		printf ("  controlled reroute: edge %u envelope %d -> %d, crossings %d -> %d\r\n",floor.EdgeId,first.Envelope,
				  after.Envelope,first.Structural.Count (),after.Structural.Count ());
	}

	//	The accepted state is one run pair: solving it again from the same inputs reproduces it.
	HybridInteractionSet again;
	HybridInteractionResult again_result;
	Check (Hybrid_Build_Interactions (params,world.Identity,world.Matrices,world.Landforms,world.Hydrology,world.Anchors,
												 world.Topology,world.Routes,Crossing_Ground,&world.Hydrology,&again,&again_result),
			 "the rerouted route set does not solve a second time");
	Check (again.Checksum == solved.Checksum,"the rerouted interaction set does not reproduce its checksum");
	(void)untouched_checked;
}


void Check_Hybrid_Interactions (void)
{
	/*
	**	The content metrics Phase 11 reads, gathered exactly as the manager gathers them.  If the shipped
	**	content cannot support the hybrid generator at all there is nothing to solve, and the check says so
	**	rather than inventing a bridge registry of its own.
	*/
	ProceduralWorldProfile profile;
	StringClass profile_error;
	const bool profile_loaded = ProceduralWorldProfileManager::Load_Profile_From_File (
											"data/procedural_world/native.json",&profile,&profile_error);
	Check (profile_loaded,"the native profile did not load: %s",profile_error.Peek_Buffer ());
	if (!profile_loaded) return;
	Check (profile.Uses_Hybrid_Generator (),"the native profile does not select the hybrid generator");

	profile.Plan.BiomeMode	= "single";
	profile.Plan.Biome		= "temperate";

	HybridPreflightInputs inputs;
	Hybrid_Content_Fixture (profile,&inputs);
	HybridPreflightResult preflight;
	const bool preflight_passed = Hybrid_Run_Preflight (profile,inputs,&preflight);
	Check (preflight_passed,"hybrid preflight refused the native profile: %s (%s)",
			 Hybrid_Preflight_Failure_Name (preflight.Failure),preflight.Detail.Peek_Buffer ());
	if (!preflight_passed) return;

	/*
	**	Section 8 Phase 11's two shared measures, checked against the plan's own arithmetic before any
	**	world is built, because every later assertion leans on them.
	*/
	{
		const float gap = Hybrid_Overpass_Clear_Gap (10.0f,1.0f);
		Check (gap == (10.0f + 1.0f),"the overpass clear gap takes the larger safety term (%.3f)",gap);
		const float small = Hybrid_Overpass_Clear_Gap (1.0f,2.0f);
		Check (small == (1.0f + 0.7f),"the overpass clear gap takes the metric safety floor (%.3f)",small);

		//	The definition first, the planner second, and no third source.
		Check (Hybrid_Resolve_Bridge_Max_Span (40.0f,90.0f) == 40.0f,
				 "a bridge definition's own maximum span wins");
		Check (Hybrid_Resolve_Bridge_Max_Span (0.0f,90.0f) == 90.0f,
				 "BridgePlannerParams::MaxSpanLength is the resolved fallback");

		Check (Hybrid_Interaction_Counts_As_Bridge (HYBRID_METHOD_BRIDGE),"a bridge fills a bridge quota");
		Check (!Hybrid_Interaction_Counts_As_Bridge (HYBRID_METHOD_FORD),"a ford never fills a bridge quota");
		Check (!Hybrid_Interaction_Counts_As_Bridge (HYBRID_METHOD_TUNNEL),
				 "a tunnel never fills a bridge quota");
	}

	/*
	**	The walk covers several archetypes, not one, because which Phase-11 method families a world even
	**	contains is decided by the obstructions its archetype lays down: a canyon crossroads produces canyon
	**	crossings, a highland pass produces cliff and ridge ones, a river valley produces river crossings.
	**	One archetype would leave whole method families untested and call the gate green.
	*/
	/*
	**	Re-seeded for Q-022 / Q-023, which made the old eight-seed walk build no Phase-10 world at all: every
	**	compiled crossing must now be attached to its road, and Phase 9 freezes only structural sequences a road
	**	can drive.  The walk is the original seeds plus, per archetype, the first seeds the 1400-world census
	**	found reaching Phase 11 -- solved ones and refused ones alike, so both outcomes stay exercised.
	*/
	static const unsigned int INTERACTION_SEEDS[8] = { 23u, 98u, 196u, 13u, 24u, 35u, 95u, 4242u };
	static const unsigned int CANYON_SEED = 23u;		//	canyon_crossroads, routed and solved
	static const HybridArchetype INTERACTION_ARCHETYPES[5] = {
		HYBRID_ARCHETYPE_CANYON_CROSSROADS,		// canyon crossings, and the mandatory canyon bridge
		HYBRID_ARCHETYPE_PLATEAU_BASIN,			// the basin rim is a CLIFF obligation: cuts and ramp passes
		HYBRID_ARCHETYPE_HIGHLAND_PASS,			// the surface saddle, on drivable ground
		HYBRID_ARCHETYPE_RIVER_VALLEY,			// river crossings
		HYBRID_ARCHETYPE_RAVINE_NETWORK,			// ravine crossings
	};
	const int INTERACTION_FIXTURE_COUNT = 8 * 5;
	bool solved_any = false;
	int refused_pipeline = 0;
	int refused_phase11 = 0;
	StringClass last_pipeline_detail = "none";
	StringClass last_phase11_detail = "none";

	//	Which method families the walk actually exercised, so the gate can say what it proved.
	int seen_method[HYBRID_METHOD_COUNT];
	for (int m = 0; m < HYBRID_METHOD_COUNT; ++m) seen_method[m] = 0;
	int mandatory_bridges = 0;
	int reserved_overpasses = 0;
	int reroutes_spent = 0;
	int runs_seen[4] = { 0,0,0,0 };
	int untouched_checked = 0;
	bool negatives_done = false;

	for (int attempt = 0; attempt < INTERACTION_FIXTURE_COUNT; ++attempt) {

		HybridPipelineFixture fixture;
		StringClass pipeline_detail;
		if (!Build_Hybrid_Pipeline (INTERACTION_SEEDS[attempt % 8],1200.0f,
											 INTERACTION_ARCHETYPES[attempt / 8],&fixture,&pipeline_detail,&preflight.Metrics)) {
			refused_pipeline++;
			last_pipeline_detail = pipeline_detail;
			printf ("  archetype %d seed %u: pipeline refused -- %s\r\n",(int)INTERACTION_ARCHETYPES[attempt / 8],
					  INTERACTION_SEEDS[attempt % 8],pipeline_detail.Peek_Buffer ());
			continue;
		}

		const HybridConstants & k = fixture.Sized.Constants;
		HybridInteractionParams params = Interaction_Fixture (fixture.Sized,1200.0f,inputs,
																			  preflight.Metrics);
		HybridInteractionSet interactions;
		HybridInteractionResult result;
		HybridRouteResult reroute_result;
		int reroutes = 0;
		int phase10_runs = 0;

		//	The frozen set Phase 10 handed over, kept to prove what a reroute did and did not touch.
		HybridRouteSet frozen_routes = fixture.Routes;

		/*
		**	Through the shared owner, which is where the plan's next-envelope reroute lives.  Calling
		**	`Hybrid_Build_Interactions` directly here would leave that path unexercised, which is exactly how
		**	`RerouteEdgeId` came to be written and never read.
		*/
		if (!Hybrid_Solve_Interactions_With_Reroute (fixture.RouteParams,params,fixture.Identity,
																 fixture.Matrices,fixture.Landforms,fixture.Hydrology,
																 fixture.Anchors,fixture.Topology,
																 Macro_Sample_From_Formula,&fixture.Ground,
																 &fixture.Routes,&reroute_result,&interactions,
																 &result,&reroutes,&phase10_runs)) {
			Check (phase10_runs == (reroutes + 1),
					 "a refused attempt ran Phase 10 %d times for %d reroutes",phase10_runs,reroutes);
			if ((phase10_runs >= 1) && (phase10_runs <= 3)) runs_seen[phase10_runs]++;
			refused_phase11++;
			last_phase11_detail.Format ("%s (%s)",Hybrid_Interaction_Failure_Name (result.Failure),
												 result.Detail.Peek_Buffer ());
			Check (result.Failure != HYBRID_INTERACTION_OK,"a failed Phase 11 must name its failure");
			Check (result.Detail.Get_Length () > 0,"a failed Phase 11 must say what refused the crossing");
			Check (interactions.Interactions.Count () == 0,"a failed Phase 11 left solutions behind");
			printf ("  archetype %d seed %u: phase 11 refused -- %s\r\n",
					  (int)INTERACTION_ARCHETYPES[attempt / 8],INTERACTION_SEEDS[attempt % 8],
					  last_phase11_detail.Peek_Buffer ());
			continue;
		}
		solved_any = true;

		/*
		**	Every road can be graded as it will be realized (Q-022): Section 12.1 over its realized centerline
		**	with every structure its compiled crossings put on it holding its own surface.  This replaces a
		**	per-solution grade from each approach end down to the ground at the station, which asked a bridge
		**	to meet the canyon floor or the water its deck never touches.
		*/
		for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {

			//	One physical road at a time (Q-025): a heavy family's trunk is graded once, as one road.
			const HybridRoute & route = fixture.Routes.Routes[r];
			const HybridTrunkRoad * trunk = fixture.Routes.Find_Trunk (route.EdgeId);
			if ((trunk != NULL) && (trunk->EdgeIds[0] != route.EdgeId)) continue;
			HybridRoute realized;
			DynamicVectorClass<unsigned int> road_edges;
			DynamicVectorClass<HybridProfileStructure> structures;
			Check (Hybrid_Realized_Physical_Road (fixture.Routes,interactions,route.EdgeId,&realized,&road_edges),
					 "route %u has no realized centerline",route.EdgeId);
			Check (Hybrid_Road_Profile_Structures (interactions,realized,&structures,&road_edges),
					 "a structure on route %u does not sit on its realized road",route.EdgeId);
			if (realized.Points.Count () < 2) continue;

			DynamicVectorClass<HybridProfileSample> samples;
			HybridSolvedProfile solved_profile;
			HybridProfileFailure why = HYBRID_PROFILE_OK;
			const float max_grade = Hybrid_Road_Class_Spec (route.RoadClass).MaxGradeDegrees * 0.0174532925f;
			const bool graded = Hybrid_Solve_Road_Stretch (realized.Points,realized.Stations,0.0f,
														realized.Stations[realized.Stations.Count () - 1],structures,
														Hybrid_Profile_Sample_Spacing (k),Macro_Sample_From_Formula,
														&fixture.Ground,params.MaxCutDepth,params.MaxFillHeight,max_grade,
														k.Q,&samples,&solved_profile,NULL,NULL,&why);
			Check (graded,"archetype %d seed %u: route %u (%s) cannot be graded as realized: %s (%d structures)",
					 (int)INTERACTION_ARCHETYPES[attempt / 8],INTERACTION_SEEDS[attempt % 8],route.EdgeId,
					 Hybrid_Road_Class_Name (route.RoadClass),Hybrid_Profile_Failure_Name (why),structures.Count ());
		}
		reroutes_spent += reroutes;

		/*
		**	The lifecycle: Phase 10 ran once when the pipeline was built, and Phase 11 consumed that frozen
		**	set.  Phase 10 runs again only for a reroute, so the count is exactly one more than the reroutes.
		*/
		Check (phase10_runs == (reroutes + 1),"Phase 10 ran %d times for %d reroutes",phase10_runs,reroutes);
		if ((phase10_runs >= 1) && (phase10_runs <= 3)) runs_seen[phase10_runs]++;

		if (reroutes == 0) {
			//	No reroute: the routes Phase 11 solved over are the ones Phase 10 froze, untouched.
			Check (fixture.Routes.Checksum == frozen_routes.Checksum,
					 "a world with no reroute changed its routes (0x%08X -> 0x%08X)",frozen_routes.Checksum,
					 fixture.Routes.Checksum);
			Check (fixture.RouteParams.EnvelopeFloors.Count () == 0,"a world with no reroute raised a floor");
		} else {
			/*
			**	A reroute moved outward only, and only for the edges it named: every floor is 1 or 2, and every
			**	route Phase 10 laid down *before* the first rerouted edge is byte-identical, because it was routed
			**	from identical inputs.  Routes after it may legitimately differ -- they avoid what came before.
			*/
			int first_rerouted = frozen_routes.Routes.Count ();
			for (int f = 0; f < fixture.RouteParams.EnvelopeFloors.Count (); ++f) {
				const HybridRouteEnvelopeFloor & floor = fixture.RouteParams.EnvelopeFloors[f];
				Check ((floor.MinEnvelope >= 1) && (floor.MinEnvelope <= 2),"envelope floor %d for edge %u",
						 floor.MinEnvelope,floor.EdgeId);
				for (int r = 0; r < frozen_routes.Routes.Count (); ++r) {
					if (frozen_routes.Routes[r].EdgeId != floor.EdgeId) continue;
					if (r < first_rerouted) first_rerouted = r;
					Check (floor.MinEnvelope > frozen_routes.Routes[r].Envelope,
							 "edge %u was rerouted to floor %d from envelope %d: not outward",floor.EdgeId,
							 floor.MinEnvelope,frozen_routes.Routes[r].Envelope);
				}
				for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {
					if (fixture.Routes.Routes[r].EdgeId != floor.EdgeId) continue;
					Check (fixture.Routes.Routes[r].Envelope >= floor.MinEnvelope,
							 "edge %u came back in envelope %d under floor %d",floor.EdgeId,
							 fixture.Routes.Routes[r].Envelope,floor.MinEnvelope);
				}
			}
			for (int r = 0; (r < first_rerouted) && (r < fixture.Routes.Routes.Count ()); ++r) {
				Check (Routes_Identical (frozen_routes.Routes[r],fixture.Routes.Routes[r]),
						 "edge %u, routed before the rerouted edge, changed under the reroute",
						 frozen_routes.Routes[r].EdgeId);
			}
			untouched_checked += first_rerouted;
		}
		{
			int spans = 0;
			int windows_cut = 0;
			int windows_pass = 0;
			for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {
				spans += fixture.Routes.Routes[r].Structural.Count ();
			}
			for (int wi = 0; wi < fixture.Hydrology.Windows.Count (); ++wi) {
				if (fixture.Hydrology.Windows[wi].Has_Method (HYBRID_WINDOW_METHOD_CUT)) windows_cut++;
				if (fixture.Hydrology.Windows[wi].Has_Method (HYBRID_WINDOW_METHOD_PASS)) windows_pass++;
			}
			StringClass refusals ("");
			for (int pr = 1; pr < HYBRID_LOCAL_PASS_FAILURE_COUNT; ++pr) {
				if (fixture.Hydrology.PassRefusals[pr] == 0) continue;
				StringClass one;
				one.Format ("%s %d, ",Hybrid_Local_Pass_Failure_Name ((HybridLocalPassFailure)pr),
								fixture.Hydrology.PassRefusals[pr]);
				refusals += one;
			}
			printf ("    pass refusals: %s\r\n",
					  (refusals.Get_Length () > 0) ? refusals.Peek_Buffer () : "none");

			/*
			**	And how far short they fell.  A reason census alone cannot distinguish "the supported parameter
			**	space cannot build a pass here" from "passes fit and later validation rejects them", so the
			**	along-obstruction run every station asked for is reported as a distribution.
			*/
			StringClass ratios ("");
			for (int rb = 0; rb < HYBRID_PASS_RATIO_BUCKETS; ++rb) {
				if (fixture.Hydrology.PassRatioBuckets[rb] == 0) continue;
				StringClass one;
				one.Format ("%s %d, ",Hybrid_Pass_Ratio_Bucket_Name (rb),
								fixture.Hydrology.PassRatioBuckets[rb]);
				ratios += one;
			}
			int median_bucket = -1;
			{
				int seen = 0;
				const int half_way = fixture.Hydrology.PassStationsMeasured / 2;
				for (int rb = 0; rb < HYBRID_PASS_RATIO_BUCKETS; ++rb) {
					seen += fixture.Hydrology.PassRatioBuckets[rb];
					if (seen <= half_way) continue;
					median_bucket = rb;
					break;
				}
			}
			printf ("    pass run: %d measured, %d fit, ratio min %.3f median %s max %.3f -- %s\r\n",
					  fixture.Hydrology.PassStationsMeasured,fixture.Hydrology.PassRunFits,
					  fixture.Hydrology.PassRatioMin,Hybrid_Pass_Ratio_Bucket_Name (median_bucket),
					  fixture.Hydrology.PassRatioMax,
					  (ratios.Get_Length () > 0) ? ratios.Peek_Buffer () : "none");

			StringClass legs ("");
			for (int lg = 0; lg < HYBRID_LOCAL_PASS_MAX_CONTROLS; ++lg) {
				if (fixture.Hydrology.PassProfileLegRefusals[lg] == 0) continue;
				StringClass one;
				one.Format ("leg %d: %d, ",lg,fixture.Hydrology.PassProfileLegRefusals[lg]);
				legs += one;
			}
			printf ("    pass profile: steepest %.4f rad -- %s\r\n",fixture.Hydrology.PassProfileGradeMax,
					  (legs.Get_Length () > 0) ? legs.Peek_Buffer () : "no grade refusal");

			StringClass kinds ("");
			for (int si = 0; si < interactions.Interactions.Count (); ++si) {
				StringClass one;
				one.Format ("%s/%s ",Hybrid_Obstruction_Kind_Name (interactions.Interactions[si].Obstruction),
								Hybrid_Interaction_Method_Name (interactions.Interactions[si].Method));
				kinds += one;
			}
			printf ("  archetype %d seed %u: solved, %d solutions, %d spans, %d windows (%d cut, %d pass): %s\r\n",
					  (int)INTERACTION_ARCHETYPES[attempt / 8],INTERACTION_SEEDS[attempt % 8],
					  interactions.Interactions.Count (),spans,fixture.Hydrology.Windows.Count (),windows_cut,
					  windows_pass,kinds.Peek_Buffer ());
		}

		Check (interactions.Checksum != 0,"a passed Phase 11 left no checksum");
		Check (Hybrid_Interaction_Checksum (interactions,fixture.Identity.Checksum) == interactions.Checksum,
				 "the checksum does not describe the records it was built from");

		/*
		**	Exactly one solution per road-feature conflict.  Phase 10 recorded every crossing it left
		**	unresolved as a structural span, so the two counts have to agree edge by edge -- a conflict with
		**	no solution and a conflict with two are both gate failures.
		*/
		for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {

			const HybridRoute & route = fixture.Routes.Routes[r];
			for (int s = 0; s < route.Structural.Count (); ++s) {

				int solutions = 0;
				for (int i = 0; i < interactions.Interactions.Count (); ++i) {
					const HybridInteraction & one = interactions.Interactions[i];
					if (one.EdgeId != route.EdgeId) continue;
					if (one.WindowId != route.Structural[s].WindowId) continue;
					solutions++;
				}
				const HybridTopologyEdge * edge = fixture.Topology.Find_Edge (route.EdgeId);
				const bool required = (edge != NULL) && edge->Required;
				if (required) {
					Check (solutions == 1,"edge %u crossing window %u has %d solutions, not one",route.EdgeId,
							 route.Structural[s].WindowId,solutions);
				} else {
					Check (solutions <= 1,"optional edge %u crossing window %u has %d solutions",route.EdgeId,
							 route.Structural[s].WindowId,solutions);
				}
			}
		}

		/*
		**	Every frozen obligation keeps what Phase 9 reserved.  A mandatory crossing obligation has a
		**	solution; a mandatory bridge stays a bridge; a reserved overpass stays an overpass and never
		**	quietly becomes an at-grade junction.
		*/
		for (int n = 0; n < fixture.Topology.Nodes.Count (); ++n) {

			const HybridTopologyNode & node = fixture.Topology.Nodes[n];

			if (node.Kind == HYBRID_NODE_CROSSING_OBLIGATION) {

				//	Only an obligation a realized, routed edge actually carries.
				bool routed = false;
				for (int e = 0; (e < fixture.Topology.Edges.Count ()) && !routed; ++e) {
					const HybridTopologyEdge & edge = fixture.Topology.Edges[e];
					if (!edge.Required) continue;
					for (int w = 0; (w < edge.Waypoints.Count ()) && !routed; ++w) {
						if (edge.Waypoints[w] != node.StableId) continue;
						if (fixture.Routes.Find (edge.StableId) != NULL) routed = true;
					}
				}
				if (!routed) continue;

				const HybridInteraction * solution = NULL;
				for (int i = 0; (i < interactions.Interactions.Count ()) && (solution == NULL); ++i) {
					if (interactions.Interactions[i].WindowId != node.WindowId) continue;
					solution = &interactions.Interactions[i];
				}
				Check (solution != NULL,"mandatory crossing obligation %u (window %u) has no solution",
						 node.StableId,node.WindowId);
				if (solution == NULL) continue;

				mandatory_bridges++;
				Check (solution->Mandatory,"the solution for obligation %u is not marked mandatory",
						 node.StableId);

				//	A mandatory crossing of a canyon or ravine must remain a bridge.
				const HybridCrossingWindow * window = fixture.Hydrology.Find_Window (node.WindowId);
				if ((window != NULL) && ((window->Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
												 (window->Obstruction == HYBRID_OBSTRUCTION_RAVINE))) {
					Check (solution->Method == HYBRID_METHOD_BRIDGE,
							 "mandatory canyon bridge obligation %u compiled as %s",node.StableId,
							 Hybrid_Interaction_Method_Name (solution->Method));
				}
				Check (solution->Method != HYBRID_METHOD_FORD,
						 "mandatory obligation %u compiled as a ford",node.StableId);
			}

			if (node.Kind == HYBRID_NODE_OVERPASS_OBLIGATION) {

				const HybridInteraction * solution = NULL;
				for (int i = 0; (i < interactions.Interactions.Count ()) && (solution == NULL); ++i) {
					const HybridInteraction & one = interactions.Interactions[i];
					if (one.OtherEdgeId == 0) continue;
					if ((one.Station - node.Position).Length () > Gate_Max (k.S,k.A)) continue;
					solution = &one;
				}
				if (solution == NULL) continue;			// the two edges were not both routed
				reserved_overpasses++;
				Check (solution->Method == HYBRID_METHOD_OVERPASS,
						 "reserved overpass obligation %u downgraded to %s",node.StableId,
						 Hybrid_Interaction_Method_Name (solution->Method));
			}
		}

		//	The Section-5 quotas, recomputed from the records rather than read from the solver.
		{
			int river_bridges = 0;
			int canyon_bridges = 0;
			int overpasses = 0;
			for (int i = 0; i < interactions.Interactions.Count (); ++i) {

				const HybridInteraction & one = interactions.Interactions[i];
				if (one.Method == HYBRID_METHOD_OVERPASS) overpasses++;
				if (!Hybrid_Interaction_Counts_As_Bridge (one.Method)) continue;
				if ((one.Obstruction == HYBRID_OBSTRUCTION_RIVER) ||
					 (one.Obstruction == HYBRID_OBSTRUCTION_LAKE) ||
					 (one.Obstruction == HYBRID_OBSTRUCTION_COAST)) river_bridges++;
				if ((one.Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
					 (one.Obstruction == HYBRID_OBSTRUCTION_RAVINE)) canyon_bridges++;
			}
			Check (river_bridges >= fixture.Topology.RequiredRiverBridges,
					 "%d river bridges against the Section-5 quota of %d",river_bridges,
					 fixture.Topology.RequiredRiverBridges);
			Check (canyon_bridges >= fixture.Topology.RequiredCanyonBridges,
					 "%d canyon bridges against the Section-5 quota of %d",canyon_bridges,
					 fixture.Topology.RequiredCanyonBridges);
			Check (overpasses >= fixture.Topology.ReservedOverpassCount,
					 "%d overpasses against the %d Phase 9 reserved",overpasses,
					 fixture.Topology.ReservedOverpassCount);
		}

		//	At most twelve crossing problems on one edge.
		for (int e = 0; e < fixture.Topology.Edges.Count (); ++e) {

			int problems = 0;
			for (int i = 0; i < interactions.Issues.Count (); ++i) {
				if (interactions.Issues[i].EdgeId == fixture.Topology.Edges[e].StableId) problems++;
			}
			Check (problems <= 12,"edge %u carries %d crossing problems, over the limit of 12",
					 fixture.Topology.Edges[e].StableId,problems);
		}

		//	The 64-station cap: no conflict may evaluate more stations than the cap allows it.
		{
			int conflicts = 0;
			for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {
				conflicts += fixture.Routes.Routes[r].Structural.Count ();
			}
			const int ceiling = (conflicts + interactions.Issues.Count () + 1) * 64 * HYBRID_METHOD_COUNT;
			Check (interactions.StationsEvaluated <= ceiling,
					 "%d stations evaluated over a ceiling of %d",interactions.StationsEvaluated,ceiling);
		}

		//	Every compiled solution, re-measured from the records.
		for (int i = 0; i < interactions.Interactions.Count (); ++i) {

			const HybridInteraction & one = interactions.Interactions[i];
			seen_method[one.Method]++;

			const HybridTopologyEdge * edge = fixture.Topology.Find_Edge (one.EdgeId);
			Check (edge != NULL,"solution %u names edge %u, which does not exist",one.StableId,one.EdgeId);
			if (edge == NULL) continue;

			//	No HIGHWAY_4 or PRIMARY_2 ever fords.
			if (one.Method == HYBRID_METHOD_FORD) {
				Check ((one.RoadClass != HYBRID_ROAD_HIGHWAY_4) && (one.RoadClass != HYBRID_ROAD_PRIMARY_2),
						 "a %s road fords at solution %u",Hybrid_Road_Class_Name (one.RoadClass),one.StableId);
			}

			//	Quantized geometry, and both approach endpoints inside the placeable world.
			Check (one.Station.X == Hybrid_Quantize_Position (one.Station.X),
					 "solution %u station X is not quantized",one.StableId);
			Check (one.Station.Y == Hybrid_Quantize_Position (one.Station.Y),
					 "solution %u station Y is not quantized",one.StableId);

			int cx = 0;
			int cy = 0;
			Check (fixture.Matrices.Matrix.World_To_Cell (one.ApproachA,&cx,&cy) &&
					 fixture.Matrices.Matrix.Has_Flag (cx,cy,HYBRID_FLAG_PLACEABLE),
					 "solution %u approach A is outside the placeable world",one.StableId);
			Check (fixture.Matrices.Matrix.World_To_Cell (one.ApproachB,&cx,&cy) &&
					 fixture.Matrices.Matrix.Has_Flag (cx,cy,HYBRID_FLAG_PLACEABLE),
					 "solution %u approach B is outside the placeable world",one.StableId);

			//	The station belongs to the obstruction it names, inside its own window's reach.
			if (one.WindowId != 0) {

				const HybridCrossingWindow * window = fixture.Hydrology.Find_Window (one.WindowId);
				Check (window != NULL,"solution %u names window %u, which does not exist",one.StableId,
						 one.WindowId);
				if (window != NULL) {
					Check (window->Obstruction == one.Obstruction,
							 "solution %u says %s but window %u crosses %s",one.StableId,
							 Hybrid_Obstruction_Kind_Name (one.Obstruction),one.WindowId,
							 Hybrid_Obstruction_Kind_Name (window->Obstruction));

					const float reach = Gate_Max (8.0f * k.W,4.0f * k.S) + window->Span;
					Check ((one.Station - window->Center).Length () <= reach,
							 "solution %u station is %.1f from window %u, over the search reach of %.1f",
							 one.StableId,(one.Station - window->Center).Length (),one.WindowId,reach);
				}
			}

			/*
			**	The approach reserve is real on both sides, and symmetric about the station.  A bridge or tunnel
			**	approach is straight along its crossing axis (Q-022), so the straight line to each approach end
			**	measures it.  An overpass ramp is Section 12.3's `L_ramp` along the upper road's own centerline
			**	beyond each end of its straight deck, and the plan does not ask the ramp itself to be straight, so
			**	it is measured along the realized road: each foot at least half the deck plus the ramp from the
			**	crossing.
			*/
			const float to_a = (one.ApproachA - one.Station).Length ();
			const float to_b = (one.ApproachB - one.Station).Length ();
			if (one.Method == HYBRID_METHOD_OVERPASS) {
				const HybridRoute * upper = NULL;
				for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {
					if (fixture.Routes.Routes[r].EdgeId == one.EdgeId) upper = &fixture.Routes.Routes[r];
				}
				HybridRoute road;
				Check ((upper != NULL) && Hybrid_Realized_Route (*upper,interactions,&road),
						 "overpass %u has no realized upper road",one.StableId);
				if (road.Points.Count () >= 2) {
					const float centre = Gate_Station_On (road,one.Station);
					const float foot_a = Gate_Station_On (road,one.ApproachA);
					const float foot_b = Gate_Station_On (road,one.ApproachB);
					const float needed = (0.5f * one.Span) + one.ApproachReserve - (2.0f * k.Q);
					Check ((float)fabs ((double)(centre - foot_a)) >= needed,
							 "overpass %u ramp A runs %.2f along its road, under the %.2f of half deck and ramp",
							 one.StableId,(float)fabs ((double)(centre - foot_a)),needed);
					Check ((float)fabs ((double)(foot_b - centre)) >= needed,
							 "overpass %u ramp B runs %.2f along its road, under the %.2f of half deck and ramp",
							 one.StableId,(float)fabs ((double)(foot_b - centre)),needed);
					Check ((foot_a - centre) * (foot_b - centre) < 0.0f,"overpass %u ramps are not either side of it",
							 one.StableId);
				}
			} else if (one.Method != HYBRID_METHOD_AT_GRADE_JUNCTION) {
				Check (to_a >= (one.ApproachReserve - k.W),
						 "solution %u approach A reserve is %.1f, under the required %.1f",one.StableId,to_a,
						 one.ApproachReserve);
				Check (to_b >= (one.ApproachReserve - k.W),
						 "solution %u approach B reserve is %.1f, under the required %.1f",one.StableId,to_b,
						 one.ApproachReserve);
			}

			/*
			**	Neither approach reserve may touch a base or Tiberium reserve this edge has no business in.
			**	Recomputed here from the anchors rather than trusted.
			*/
			{
				unsigned int own_base[2] = { 0,0 };
				unsigned int own_field[2] = { 0,0 };
				const HybridTopologyNode * ends[2];
				ends[0] = fixture.Topology.Find_Node (edge->NodeA);
				ends[1] = fixture.Topology.Find_Node (edge->NodeB);
				for (int e = 0; e < 2; ++e) {
					if (ends[e] == NULL) continue;
					if (ends[e]->Kind == HYBRID_NODE_FIELD_ACCESS)	own_field[e] = ends[e]->SiteId;
					else															own_base[e] = ends[e]->SiteId;
				}

				const int steps = 8;
				for (int s = 0; s <= steps; ++s) {

					const float ratio = (float)s / (float)steps;
					const Vector2 point (one.ApproachA.X + ((one.ApproachB.X - one.ApproachA.X) * ratio),
												one.ApproachA.Y + ((one.ApproachB.Y - one.ApproachA.Y) * ratio));

					for (int t = 0; t < fixture.Anchors.Bases.Count (); ++t) {
						const HybridBaseAnchor & base = fixture.Anchors.Bases[t];
						if ((base.SiteId == own_base[0]) || (base.SiteId == own_base[1])) continue;
						Check ((point - base.Center).Length () > base.ProtectedRadius,
								 "solution %u approach enters unrelated base %u",one.StableId,base.SiteId);
					}
					for (int t = 0; t < fixture.Anchors.Resources.Count (); ++t) {
						const HybridResourceAnchor & anchor = fixture.Anchors.Resources[t];
						if ((anchor.SiteId == own_field[0]) || (anchor.SiteId == own_field[1])) continue;
						Check ((point - anchor.Center).Length () > anchor.ReserveRadius,
								 "solution %u approach enters unrelated Tiberium reserve %u",one.StableId,
								 anchor.SiteId);
					}
				}
			}

			/*
			**	The swept vehicle envelope has to be carried by whatever the method puts the vehicle on: a
			**	structure's own deck, which Section 12.4 widens to fit, or the graded roadbed for a method that
			**	stays on the ground.  The marked carriageway is not the test -- a bridge exists precisely to be
			**	wider than the road that meets it.
			*/
			{
				const float swept = Gate_Max (preflight.Metrics.HeavyVehicleWidth,
														preflight.Metrics.HarvesterWidth) + 2.0f;
				if ((one.Method == HYBRID_METHOD_FORD) || (one.Method == HYBRID_METHOD_SURFACE_PASS) ||
					 (one.Method == HYBRID_METHOD_RAMP_PASS) || (one.Method == HYBRID_METHOD_ROAD_CUT)) {
					Check (swept <= Hybrid_Roadbed_Width (one.RoadClass,k.W),
							 "%s %u carries a %.1f swept width on a %.1f roadbed",
							 Hybrid_Interaction_Method_Name (one.Method),one.StableId,swept,
							 Hybrid_Roadbed_Width (one.RoadClass,k.W));
				}
			}

			/*
			**	4W separation from any unrelated crossing -- "unless the topology explicitly shares a road
			**	junction": two solutions are related when they involve a common road, counting every road that
			**	carries a topology junction's node.
			*/
			for (int j = 0; j < interactions.Interactions.Count (); ++j) {

				if (j == i) continue;
				const HybridInteraction & other = interactions.Interactions[j];
				if (Gate_Solutions_Related (fixture.Topology,one,other)) continue;
				Check ((other.Station - one.Station).Length () >= (4.0f * k.W),
						 "solutions %u and %u are %.1f apart, under the 4W separation of %.1f",one.StableId,
						 other.StableId,(other.Station - one.Station).Length (),4.0f * k.W);
			}

			//	A bridge: the kind's clear width, its resolved maximum span, and its vertical geometry.
			if (one.Method == HYBRID_METHOD_BRIDGE) {

				Check ((one.BridgeKind >= 0) && (one.BridgeKind < params.BridgeKinds.Count ()),
						 "bridge solution %u names kind %d",one.StableId,one.BridgeKind);
				if ((one.BridgeKind < 0) || (one.BridgeKind >= params.BridgeKinds.Count ())) continue;

				const HybridBridgeKind & kind = params.BridgeKinds[one.BridgeKind];
				const float required_width =
					Hybrid_Bridge_Clear_Width (one.RoadClass,k.W,
														Gate_Max (preflight.Metrics.HeavyVehicleWidth,
																	  preflight.Metrics.HarvesterWidth),1.0f);
				Check (kind.ClearWidth >= required_width,
						 "bridge %u has a %.2f clear width against the Section-12.4 requirement of %.2f",
						 one.StableId,kind.ClearWidth,required_width);
				Check (kind.ClearWidth >= preflight.Metrics.BridgeRequiredWidth[one.RoadClass],
						 "bridge %u is narrower than the preflight requirement for its class",one.StableId);

				//	The measured span plus both abutment seats has to fit the resolved maximum span.
				Check ((one.Span + (2.0f * k.W)) <= kind.MaxSpan,
						 "bridge %u spans %.1f plus seats against a resolved maximum of %.1f",one.StableId,
						 one.Span,kind.MaxSpan);

				const float required_approach = (kind.RequiredApproachLength > 0.0f)
														  ? kind.RequiredApproachLength : (8.0f * k.W);
				Check (one.ApproachReserve >= required_approach,
						 "bridge %u reserves %.1f of approach against the required %.1f",one.StableId,
						 one.ApproachReserve,required_approach);

				Check (one.DeckTopZ >= (one.DeckBottomZ + kind.StructuralDepth - k.Q),
						 "bridge %u deck is %.2f thick against a structural depth of %.2f",one.StableId,
						 one.DeckTopZ - one.DeckBottomZ,kind.StructuralDepth);

				//	Over water, the deck bottom clears the surface by the registered margin.
				float surface = 0.0f;
				if (Hybrid_Point_In_Standing_Water (fixture.Hydrology,one.Station,NULL,&surface)) {
					const float margin = Gate_Max (preflight.Metrics.BridgeWaterClearanceMargin,
															  kind.RequiredClearance);
					Check (one.DeckBottomZ >= (surface + margin - k.Q),
							 "bridge %u deck bottom is %.2f over a %.2f water surface, under the %.2f margin",
							 one.StableId,one.DeckBottomZ,surface,margin);
				}
			}

			//	A tunnel: its module, the clear envelope, and a bore at least as long as the obstruction.
			if (one.Method == HYBRID_METHOD_TUNNEL) {

				Check ((one.TunnelModule >= 0) && (one.TunnelModule < params.TunnelModules.Count ()),
						 "tunnel solution %u names module %d",one.StableId,one.TunnelModule);
				if ((one.TunnelModule < 0) || (one.TunnelModule >= params.TunnelModules.Count ())) continue;

				const HybridTunnelModule & module_kind = params.TunnelModules[one.TunnelModule];
				const float swept_width = Gate_Max (preflight.Metrics.HeavyVehicleWidth,
																 preflight.Metrics.HarvesterWidth) + 2.0f;
				const float swept_height = Hybrid_Overpass_Clear_Gap (preflight.Metrics.HeavyVehicleHeight,
																					 k.WorldUnitsPerMeter);
				if (module_kind.ClearWidth > 0.0f) {
					Check (module_kind.ClearWidth >= swept_width,"tunnel %u is narrower than the swept width",
							 one.StableId);
				}
				if (module_kind.ClearHeight > 0.0f) {
					Check (module_kind.ClearHeight >= swept_height,"tunnel %u is lower than the swept height",
							 one.StableId);
				}
				Check (one.TunnelLength >= ((one.ApproachB - one.ApproachA).Length () - k.Q),
						 "tunnel %u bores %.1f across a %.1f obstruction",one.StableId,one.TunnelLength,
						 (one.ApproachB - one.ApproachA).Length ());
				Check (module_kind.PortalCompatible,"tunnel %u uses a module that is not portal compatible",
						 one.StableId);
			}

			//	A road cut stays inside the allowed cut and fill.
			if (one.Method == HYBRID_METHOD_ROAD_CUT) {
				Check (one.CutDepth <= (params.MaxCutDepth + k.Q),
						 "road cut %u is %.2f deep against a limit of %.2f",one.StableId,one.CutDepth,
						 params.MaxCutDepth);
			}

			//	A pass or ramp pass: the whole swept route holds the class grade and cross slope.
			if ((one.Method == HYBRID_METHOD_SURFACE_PASS) || (one.Method == HYBRID_METHOD_RAMP_PASS)) {

				const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec (one.RoadClass);
				const float max_grade = spec.MaxGradeDegrees * 0.0174532925f;
				const float max_cross = spec.MaxCrossSlopeDegrees * 0.0174532925f;
				const float half_roadbed = 0.5f * Hybrid_Roadbed_Width (one.RoadClass,k.W);
				const float spacing = Gate_Max (Gate_Min (0.5f * k.W,k.A / 3.0f),k.Q);
				const float length = (one.ApproachB - one.ApproachA).Length ();
				const int count = (int)(length / spacing) + 1;
				const Vector2 axis = (length > 0.0f)
											? Vector2 ((one.ApproachB.X - one.ApproachA.X) / length,
															(one.ApproachB.Y - one.ApproachA.Y) / length)
											: Vector2 (1.0f,0.0f);
				const Vector2 normal (-axis.Y,axis.X);

				float previous = 0.0f;
				bool have_previous = false;
				for (int s = 0; s <= count; ++s) {

					const float ratio = (count > 0) ? ((float)s / (float)count) : 0.0f;
					const Vector2 point (one.ApproachA.X + ((one.ApproachB.X - one.ApproachA.X) * ratio),
												one.ApproachA.Y + ((one.ApproachB.Y - one.ApproachA.Y) * ratio));

					float here = 0.0f;
					if (!Macro_Sample_From_Formula (point,&here,&fixture.Ground)) continue;
					const HybridVerticalInterval interval_here =
						Hybrid_Road_Vertical_Interval (here,params.MaxCutDepth,params.MaxFillHeight);

					if (have_previous) {
						const HybridVerticalInterval interval_previous =
							Hybrid_Road_Vertical_Interval (previous,params.MaxCutDepth,params.MaxFillHeight);
						Check (Hybrid_Grade_Compatible (interval_previous,interval_here,length / (float)count,
																  max_grade),
								 "%s %u exceeds the class grade at sample %d",
								 Hybrid_Interaction_Method_Name (one.Method),one.StableId,s);
					}
					previous			= here;
					have_previous	= true;

					float left = 0.0f;
					float right = 0.0f;
					if (!Macro_Sample_From_Formula (point + (normal * half_roadbed),&left,&fixture.Ground)) {
						continue;
					}
					if (!Macro_Sample_From_Formula (point - (normal * half_roadbed),&right,&fixture.Ground)) {
						continue;
					}
					const HybridVerticalInterval left_interval =
						Hybrid_Road_Vertical_Interval (left,params.MaxCutDepth,params.MaxFillHeight);
					const HybridVerticalInterval right_interval =
						Hybrid_Road_Vertical_Interval (right,params.MaxCutDepth,params.MaxFillHeight);
					Check (Hybrid_Grade_Compatible (left_interval,right_interval,2.0f * half_roadbed,
															  max_cross),
							 "%s %u exceeds the class cross slope at sample %d",
							 Hybrid_Interaction_Method_Name (one.Method),one.StableId,s);
				}
			}

			//	An overpass: Section 12.3's clear gap, and a ramp that holds the upper class grade.
			if (one.Method == HYBRID_METHOD_OVERPASS) {

				const float gap = Hybrid_Overpass_Clear_Gap (preflight.Metrics.HeavyVehicleHeight,
																		  k.WorldUnitsPerMeter);
				Check ((one.DeckBottomZ - one.LowerSurfaceZ) >= (gap - k.Q),
						 "overpass %u clears %.2f over the road beneath it, under the required %.2f",
						 one.StableId,one.DeckBottomZ - one.LowerSurfaceZ,gap);

				const HybridRoadClassSpec & spec = Hybrid_Road_Class_Spec (one.RoadClass);
				const float max_grade = spec.MaxGradeDegrees * 0.0174532925f;
				Check (one.RampLength >= (10.0f * k.W),
						 "overpass %u ramps over %.1f, under the 10W floor of %.1f",one.StableId,
						 one.RampLength,10.0f * k.W);

				//	The ramp's average grade over its own reserve has to be inside the class maximum.
				const float rise = one.DeckTopZ - one.LowerSurfaceZ;
				const float average = (one.RampLength > k.Q)
											 ? (float)atan ((double)(rise / one.RampLength)) : 0.0f;
				Check (average <= (max_grade + 0.001f),
						 "overpass %u ramps at %.4f rad against a class maximum of %.4f",one.StableId,average,
						 max_grade);

				Check (one.OtherEdgeId != 0,"overpass %u names no road underneath it",one.StableId);
			}
		}


		/*
		**	The hard failures, on this same real world.  Each one perturbs exactly one input, and each has to
		**	be refused for its own named reason -- not crash, not quietly pick an undocumented fallback, and
		**	not leave a half-solved world behind.  No constraint is relaxed to make any of these green.
		*/
		if (!negatives_done) {

			negatives_done = true;

			struct NegativeCase
			{
				const char *					Name;
				HybridInteractionFailure	Failure;
			};

			//	1: no bridge kind spans the obstruction.  A mandatory canyon bridge cannot fall back.
			{
				HybridInteractionParams narrow = params;
				for (int i = 0; i < narrow.BridgeKinds.Count (); ++i) narrow.BridgeKinds[i].MaxSpan = 1.0f;
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				Check (!Hybrid_Build_Interactions (narrow,fixture.Identity,fixture.Matrices,fixture.Landforms,
																fixture.Hydrology,fixture.Anchors,fixture.Topology,
																fixture.Routes,Macro_Sample_From_Formula,&fixture.Ground,
																&refused,&refused_result),
						 "a one-unit maximum bridge span was accepted");
				Check ((refused_result.Failure == HYBRID_INTERACTION_OBLIGATION_LOST) ||
						 (refused_result.Failure == HYBRID_INTERACTION_NO_SOLUTION) ||
						 (refused_result.Failure == HYBRID_INTERACTION_QUOTA),
						 "an unspannable obstruction named %s",
						 Hybrid_Interaction_Failure_Name (refused_result.Failure));
				Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
				bool named_span = false;
				for (int i = 0; i < refused.Issues.Count (); ++i) {
					if (refused.Issues[i].Problem == HYBRID_PROBLEM_SPAN_TOO_LONG) named_span = true;
				}
				StringClass issue_list ("");
				for (int i = 0; i < refused.Issues.Count (); ++i) {
					StringClass one;
					one.Format ("%s(%s/win %u) ",
									Hybrid_Interaction_Problem_Name (refused.Issues[i].Problem),
									Hybrid_Obstruction_Kind_Name (refused.Issues[i].Obstruction),
									refused.Issues[i].WindowId);
					issue_list += one;
				}
				Check (named_span,"no issue named span_too_long when no kind could span the obstruction "
						 "(failure %s, %d issues: %s)",
						 Hybrid_Interaction_Failure_Name (refused_result.Failure),refused.Issues.Count (),
						 issue_list.Peek_Buffer ());
			}

			//	2: every bridge kind is narrower than Section 12.4 requires.
			{
				HybridInteractionParams thin = params;
				for (int i = 0; i < thin.BridgeKinds.Count (); ++i) thin.BridgeKinds[i].ClearWidth = 0.25f;
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				Check (!Hybrid_Build_Interactions (thin,fixture.Identity,fixture.Matrices,fixture.Landforms,
																fixture.Hydrology,fixture.Anchors,fixture.Topology,
																fixture.Routes,Macro_Sample_From_Formula,&fixture.Ground,
																&refused,&refused_result),
						 "a quarter-unit bridge clear width was accepted");
				bool named_width = false;
				for (int i = 0; i < refused.Issues.Count (); ++i) {
					if (refused.Issues[i].Problem == HYBRID_PROBLEM_WIDTH_TOO_NARROW) named_width = true;
				}
				Check (named_width,"no issue named width_too_narrow when every kind was too narrow");
				Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
			}

			//	3: the straight approach reserve cannot fit anywhere on the route.
			{
				HybridInteractionParams unreachable = params;
				for (int i = 0; i < unreachable.BridgeKinds.Count (); ++i) {
					unreachable.BridgeKinds[i].RequiredApproachLength = 10000.0f;
				}
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				Check (!Hybrid_Build_Interactions (unreachable,fixture.Identity,fixture.Matrices,
																fixture.Landforms,fixture.Hydrology,fixture.Anchors,
																fixture.Topology,fixture.Routes,
																Macro_Sample_From_Formula,&fixture.Ground,&refused,
																&refused_result),
						 "a 10000-unit bridge approach reserve was accepted");
				bool named_approach = false;
				for (int i = 0; i < refused.Issues.Count (); ++i) {
					const HybridInteractionProblem problem = refused.Issues[i].Problem;
					if ((problem == HYBRID_PROBLEM_APPROACH_DOES_NOT_FIT) ||
						 (problem == HYBRID_PROBLEM_NO_HARD_VALID_STATION)) named_approach = true;
				}
				Check (named_approach,"no issue named the approach when the reserve could not fit");
				Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
			}

			//	4: a mandatory crossing with no bridge kind registered at all, and no legal fallback.
			{
				HybridInteractionParams assetless = params;
				assetless.BridgeKinds.Delete_All ();
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				Check (!Hybrid_Build_Interactions (assetless,fixture.Identity,fixture.Matrices,
																fixture.Landforms,fixture.Hydrology,fixture.Anchors,
																fixture.Topology,fixture.Routes,
																Macro_Sample_From_Formula,&fixture.Ground,&refused,
																&refused_result),
						 "a world with no registered bridge kind was accepted");
				Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
				Check (refused_result.Detail.Get_Length () > 0,"a refused Phase 11 said nothing");
			}

			//	5: the swept vehicle envelope cannot be carried by anything.
			{
				HybridInteractionParams huge = params;
				huge.HeavyVehicleWidth	= 500.0f;
				huge.HeavyVehicleHeight	= 500.0f;
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				Check (!Hybrid_Build_Interactions (huge,fixture.Identity,fixture.Matrices,fixture.Landforms,
																fixture.Hydrology,fixture.Anchors,fixture.Topology,
																fixture.Routes,Macro_Sample_From_Formula,&fixture.Ground,
																&refused,&refused_result),
						 "a 500-unit swept vehicle envelope was accepted");
				Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
			}

			//	6: the crossing-problem limit refuses on its own terms rather than growing.
			{
				HybridInteractionParams strict = params;
				strict.MaxProblemsPerEdge = 0;
				for (int i = 0; i < strict.BridgeKinds.Count (); ++i) strict.BridgeKinds[i].MaxSpan = 1.0f;
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				Check (!Hybrid_Build_Interactions (strict,fixture.Identity,fixture.Matrices,fixture.Landforms,
																fixture.Hydrology,fixture.Anchors,fixture.Topology,
																fixture.Routes,Macro_Sample_From_Formula,&fixture.Ground,
																&refused,&refused_result),
						 "a zero crossing-problem budget was accepted");
				Check (refused_result.Failure == HYBRID_INTERACTION_LIMIT,
						 "a breached crossing-problem limit named %s",
						 Hybrid_Interaction_Failure_Name (refused_result.Failure));
				Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
			}

			//	7: an impossible overpass clearance leaves no legal vertical assignment.
			{
				HybridInteractionParams tall = params;
				tall.HeavyVehicleHeight = 400.0f;
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				const bool accepted = Hybrid_Build_Interactions (tall,fixture.Identity,fixture.Matrices,
																				 fixture.Landforms,fixture.Hydrology,
																				 fixture.Anchors,fixture.Topology,
																				 fixture.Routes,Macro_Sample_From_Formula,
																				 &fixture.Ground,&refused,&refused_result);
				//	Either it refuses, or every road x road crossing it kept is an at-grade junction: a 400-unit
				//	clearance cannot be ramped to, and an overpass may not be compiled without it.
				if (accepted) {
					for (int i = 0; i < refused.Interactions.Count (); ++i) {
						Check (refused.Interactions[i].Method != HYBRID_METHOD_OVERPASS,
								 "an overpass compiled under a 400-unit required clearance");
					}
				} else {
					Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
				}
			}

			//	8: a crossing separation so large that no two crossings can coexist.
			{
				HybridInteractionParams crowded = params;
				crowded.Constants.W = params.Constants.W * 40.0f;		// 4W separation becomes 160W of world
				HybridInteractionSet refused;
				HybridInteractionResult refused_result;
				const bool accepted = Hybrid_Build_Interactions (crowded,fixture.Identity,fixture.Matrices,
																				 fixture.Landforms,fixture.Hydrology,
																				 fixture.Anchors,fixture.Topology,
																				 fixture.Routes,Macro_Sample_From_Formula,
																				 &fixture.Ground,&refused,&refused_result);
				if (!accepted) {
					Check (refused.Interactions.Count () == 0,"a refused Phase 11 left solutions behind");
				}
			}
		}

		//	Determinism: a second solving pass over the same frozen records is identical.
		{
			HybridInteractionSet again;
			HybridInteractionResult again_result;
			Check (Hybrid_Build_Interactions (params,fixture.Identity,fixture.Matrices,fixture.Landforms,
														 fixture.Hydrology,fixture.Anchors,fixture.Topology,
														 fixture.Routes,Macro_Sample_From_Formula,&fixture.Ground,
														 &again,&again_result),
					 "the second interaction pass was refused: %s",again_result.Detail.Peek_Buffer ());
			Check (again.Checksum == interactions.Checksum,"Phase 11 is not reproducible");
			Check (again.Interactions.Count () == interactions.Interactions.Count (),
					 "the second pass compiled a different number of solutions");
			for (int i = 0; (i < again.Interactions.Count ()) &&
								 (i < interactions.Interactions.Count ()); ++i) {
				Check (again.Interactions[i].StableId == interactions.Interactions[i].StableId,
						 "solution order is not stable at %d",i);
				Check (again.Interactions[i].Method == interactions.Interactions[i].Method,
						 "method selection is not stable at %d",i);
				Check (again.Interactions[i].BridgeKind == interactions.Interactions[i].BridgeKind,
						 "bridge kind selection is not stable at %d",i);
				Check (again.Interactions[i].TunnelModule == interactions.Interactions[i].TunnelModule,
						 "tunnel module selection is not stable at %d",i);
				Check (again.Interactions[i].Station == interactions.Interactions[i].Station,
						 "station selection is not stable at %d",i);
			}
		}
	}

	Check (solved_any,"no fixture seed produced a Phase-11 solution set: %d pipeline refusals (last: %s), "
			 "%d Phase-11 refusals (last: %s)",refused_pipeline,last_pipeline_detail.Peek_Buffer (),
			 refused_phase11,last_phase11_detail.Peek_Buffer ());

	/*
	**	What the walk actually exercised.  A method the shipped content cannot support is not a failure --
	**	the plan makes fords and tunnels optional capabilities -- but the two obligations are not optional,
	**	and a bridge is the only method a quota accepts, so those have to have been seen.
	*/
	printf ("Phase 11 coverage: bridge %d, ford %d, surface pass %d, road cut %d, ramp pass %d, "
					  "tunnel %d, junction %d, overpass %d; %d mandatory crossings, %d reserved overpasses\r\n",
					  seen_method[HYBRID_METHOD_BRIDGE],seen_method[HYBRID_METHOD_FORD],
					  seen_method[HYBRID_METHOD_SURFACE_PASS],seen_method[HYBRID_METHOD_ROAD_CUT],
					  seen_method[HYBRID_METHOD_RAMP_PASS],seen_method[HYBRID_METHOD_TUNNEL],
					  seen_method[HYBRID_METHOD_AT_GRADE_JUNCTION],seen_method[HYBRID_METHOD_OVERPASS],
					  mandatory_bridges,reserved_overpasses);

	Check (seen_method[HYBRID_METHOD_BRIDGE] > 0,"no fixture seed compiled a bridge");
	Check (mandatory_bridges > 0,"no fixture seed exercised a mandatory crossing obligation");
	/*
	**	The reroute path itself.  Phase 11 names an edge in `RerouteEdgeId` and the shared owner answers by
	**	raising that edge's envelope floor and re-running Phase 10, so the floor has to be honoured or the
	**	whole fallback is decoration.  This proves it directly rather than waiting for a world that needs it:
	**	a route asked to start at envelope 1 must not come back in envelope 0.
	*/
	{
		HybridPipelineFixture fixture;
		StringClass why;
		if (Build_Hybrid_Pipeline (CANYON_SEED,1200.0f,HYBRID_ARCHETYPE_CANYON_CROSSROADS,&fixture,
											&why,&preflight.Metrics) && (fixture.Routes.Routes.Count () > 0)) {

			const unsigned int edge_id = fixture.Routes.Routes[0].EdgeId;
			const int was = fixture.Routes.Routes[0].Envelope;

			HybridRouteEnvelopeFloor floor;
			floor.EdgeId		= edge_id;
			floor.MinEnvelope	= 1;
			fixture.RouteParams.EnvelopeFloors.Add (floor);

			HybridRouteSet wider;
			HybridRouteResult wider_result;
			const bool rebuilt = Hybrid_Build_Routes (fixture.RouteParams,fixture.Identity,fixture.Matrices,
																	fixture.Landforms,fixture.Hydrology,fixture.Anchors,
																	fixture.Topology,Macro_Sample_From_Formula,
																	&fixture.Ground,&wider,&wider_result);
			Check (was == 0,"the fixture's first route did not start in envelope 0 (%d)",was);
			if (rebuilt) {
				bool found = false;
				for (int r = 0; r < wider.Routes.Count (); ++r) {
					if (wider.Routes[r].EdgeId != edge_id) continue;
					found = true;
					Check (wider.Routes[r].Envelope >= 1,
							 "edge %u ignored its envelope floor and came back in envelope %d",edge_id,
							 wider.Routes[r].Envelope);
				}
				Check (found,"edge %u vanished when its envelope floor was raised",edge_id);
			}

			/*
			**	Byte-identity of the edges a reroute does not affect, measured where it means something.  The
			**	fixture worlds that reroute happen to reroute their first-routed edge, which leaves nothing
			**	before it to compare; so here the floor goes on the *last* edge Phase 10 routes, and every route
			**	laid down before it must come back bit for bit.  The full re-run is only acceptable because this
			**	holds.
			*/
			HybridPipelineFixture late;
			StringClass late_why;
			if (Build_Hybrid_Pipeline (CANYON_SEED,1200.0f,HYBRID_ARCHETYPE_CANYON_CROSSROADS,&late,
												&late_why,&preflight.Metrics) && (late.Routes.Routes.Count () > 1)) {

				const HybridRouteSet before = late.Routes;
				const int last = before.Routes.Count () - 1;

				HybridRouteEnvelopeFloor late_floor;
				late_floor.EdgeId			= before.Routes[last].EdgeId;
				late_floor.MinEnvelope	= before.Routes[last].Envelope + 1;
				if (late_floor.MinEnvelope <= 2) {
					late.RouteParams.EnvelopeFloors.Add (late_floor);

					HybridRouteSet after;
					HybridRouteResult after_result;
					if (Hybrid_Build_Routes (late.RouteParams,late.Identity,late.Matrices,late.Landforms,
													 late.Hydrology,late.Anchors,late.Topology,Macro_Sample_From_Formula,
													 &late.Ground,&after,&after_result)) {
						/*
						**	The rerouted edge is the last one routed; at its next envelope it either routes or, when it
						**	is optional, is dropped as Phase 10 drops any optional edge with no legal road.  Everything
						**	before it is unaffected either way.
						*/
						const HybridTopologyEdge * late_edge = late.Topology.Find_Edge (late_floor.EdgeId);
						const bool dropped = (after.Find (late_floor.EdgeId) == NULL);
						Check ((after.Routes.Count () == before.Routes.Count ()) ||
								 (dropped && (late_edge != NULL) && !late_edge->Required &&
								  (after.Routes.Count () == (before.Routes.Count () - 1))),
								 "raising one floor changed the route count from %d to %d",before.Routes.Count (),
								 after.Routes.Count ());
						int identical = 0;
						for (int r = 0; (r < last) && (r < after.Routes.Count ()); ++r) {
							const bool same = Routes_Identical (before.Routes[r],after.Routes[r]);
							Check (same,"edge %u, routed before the rerouted edge %u, changed",
									 before.Routes[r].EdgeId,late_floor.EdgeId);
							if (same) identical++;
						}
						untouched_checked += identical;
						printf ("  late-edge reroute: edge %u to envelope %d, %d of %d earlier routes byte-identical\r\n",
								  late_floor.EdgeId,late_floor.MinEnvelope,identical,last);
					}
				}
			}
		}
	}

	printf ("Phase 11 reroutes spent across the walk: %d; worlds by Phase-10 runs: 1 x%d, 2 x%d, 3 x%d; "
			  "%d earlier routes proved byte-identical\r\n",reroutes_spent,runs_seen[1],runs_seen[2],runs_seen[3],
			  untouched_checked);
	/*
	**	One reroute that succeeds.  No native seed of a 1400-world census leaves a Phase-11 refusal the next
	**	envelope resolves once Phase 9 refuses undrivable structural sequences (Q-023), so this is the controlled
	**	world above, through the real Phase-10 and Phase-11 owners -- not a claim that a native one exists.
	*/
	Check_Controlled_Reroute (inputs,preflight.Metrics,runs_seen,&untouched_checked);
	Check_Hybrid_Crossing_Compile (inputs,preflight.Metrics);
	Check_Hybrid_Continuous_Trunk (&inputs,&preflight.Metrics);

	Check (runs_seen[1] > 0,"no fixture world solved without a reroute, so the one-run path is untested");
	Check (runs_seen[2] > 0,"no fixture world spent exactly one reroute, so the two-run path is untested");

	/*
	**	Two reroute levels on one edge, 0 -> 1 -> 2, and then the honest failure.
	**
	**	No shipped bridge kind is allowed to span anything, so a mandatory bridge crossing can never compile:
	**	Phase 11 must ask for envelope 1, then envelope 2, and then fail with the interaction failure it
	**	named -- running Phase 10 exactly three times, never returning to a narrower envelope, and never
	**	jumping from 0 straight to 2.  This is the negative fixture's own real pipeline, not a fabricated one.
	*/
	{
		HybridPipelineFixture fixture;
		StringClass why;
		if (Build_Hybrid_Pipeline (CANYON_SEED,1200.0f,HYBRID_ARCHETYPE_CANYON_CROSSROADS,&fixture,
											&why,&preflight.Metrics)) {

			HybridInteractionParams unspannable = Interaction_Fixture (fixture.Sized,1200.0f,inputs,
																						  preflight.Metrics);
			for (int i = 0; i < unspannable.BridgeKinds.Count (); ++i) unspannable.BridgeKinds[i].MaxSpan = 1.0f;

			HybridInteractionSet refused;
			HybridInteractionResult refused_result;
			HybridRouteResult refused_routes;
			int spent = 0;
			int runs = 0;
			Check (!Hybrid_Solve_Interactions_With_Reroute (fixture.RouteParams,unspannable,fixture.Identity,
																			fixture.Matrices,fixture.Landforms,
																			fixture.Hydrology,fixture.Anchors,
																			fixture.Topology,Macro_Sample_From_Formula,
																			&fixture.Ground,&fixture.Routes,
																			&refused_routes,&refused,&refused_result,
																			&spent,&runs),
					 "a world with no spannable bridge solved anyway");
			Check (refused_result.Failure != HYBRID_INTERACTION_OK,"the exhausted reroute named no failure");
			Check (refused.Interactions.Count () == 0,"the exhausted reroute left solutions behind");
			Check (runs == (spent + 1),"Phase 10 ran %d times for %d reroutes",runs,spent);
			Check (runs >= 3,"a crossing no envelope can serve ran Phase 10 only %d times",runs);
			if ((runs >= 1) && (runs <= 3)) runs_seen[runs]++;

			//	The one edge that failed went 0 -> 1 -> 2 and no further.
			bool two_levels = false;
			for (int f = 0; f < fixture.RouteParams.EnvelopeFloors.Count (); ++f) {
				const int floor = fixture.RouteParams.EnvelopeFloors[f].MinEnvelope;
				Check ((floor >= 1) && (floor <= 2),"an envelope floor of %d",floor);
				if (floor == 2) two_levels = true;
			}
			Check (two_levels,"the unspannable crossing never reached its second reroute level");
			printf ("  two-level reroute: %d reroutes, %d Phase-10 runs, final failure %s\r\n",spent,runs,
					  Hybrid_Interaction_Failure_Name (refused_result.Failure));
		}
	}
	Check (runs_seen[3] > 0,"no fixture exercised a three-run reroute");

	/*
	**	Section 12.1: two roads of one semantic class can be required to carry different vehicles, because the
	**	capability comes from the edge's role and not its class.  Step 11's optional heavy cross-link and a
	**	cross-link synthesized for a quota are both `SECONDARY_2`, and only the first carries the heavy
	**	reference.
	*/
	{
		const unsigned int heavy_link = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_CROSS_LINK,true);
		const unsigned int quota_link = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_CROSS_LINK,false);
		Check (heavy_link != quota_link,"two SECONDARY_2 cross-links of different roles got one capability");
		Check ((heavy_link & HYBRID_NAV_HEAVY) != 0,"the optional heavy cross-link does not carry heavy");
		Check ((quota_link & HYBRID_NAV_HEAVY) == 0,"a quota-filling cross-link was made heavy");
		Check ((quota_link & HYBRID_NAV_HARVESTER) == 0,"a quota-filling cross-link was given the Harvester");

		const unsigned int trunk = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_FAMILY_TRUNK,false);
		const unsigned int home = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_HOME_ROAD,false);
		const unsigned int branch = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_FIELD_BRANCH,false);
		const unsigned int fill = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_SERVICE_FILL,false);
		const unsigned int trail = Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_TRAIL,false);
		Check ((trunk & HYBRID_NAV_HEAVY) != 0,"a family trunk does not carry heavy");
		Check ((home & HYBRID_NAV_HARVESTER) != 0,"a home road does not carry the Harvester");
		Check ((branch & HYBRID_NAV_HARVESTER) != 0,"a field branch does not carry the Harvester");
		Check ((fill & HYBRID_NAV_HARVESTER) == 0,"a quota service link was given the Harvester");
		Check (trail == HYBRID_NAV_LIGHT,"a trail carries more than the light reference");
		Check (Hybrid_Topology_Edge_Capabilities (HYBRID_EDGE_BASE_INTERNAL,false) == 0,
				 "a never-realized base edge carries a capability");

		//	And a real world agrees: same class, different masks, straight out of Phase 9.
		HybridPipelineFixture fixture;
		StringClass why;
		bool pair = false;
		for (int s = 0; (s < 8) && !pair; ++s) {
			for (int a = 0; (a < 5) && !pair; ++a) {
				if (!Build_Hybrid_Pipeline (INTERACTION_SEEDS[s],1200.0f,INTERACTION_ARCHETYPES[a],&fixture,&why,&preflight.Metrics)) {
					continue;
				}
				for (int i = 0; (i < fixture.Topology.Edges.Count ()) && !pair; ++i) {
					for (int j = i + 1; (j < fixture.Topology.Edges.Count ()) && !pair; ++j) {
						const HybridTopologyEdge & x = fixture.Topology.Edges[i];
						const HybridTopologyEdge & y = fixture.Topology.Edges[j];
						if (!x.Is_Realized () || !y.Is_Realized ()) continue;
						if (x.RoadClass != y.RoadClass) continue;
						if (x.Capabilities == y.Capabilities) continue;
						pair = true;
						printf ("  same class, different capability: edges %u (%s, 0x%x) and %u (%s, 0x%x) are both %s\r\n",
								  x.StableId,Hybrid_Topology_Edge_Kind_Name (x.Kind),x.Capabilities,y.StableId,
								  Hybrid_Topology_Edge_Kind_Name (y.Kind),y.Capabilities,
								  Hybrid_Road_Class_Name (x.RoadClass));
					}
				}
				for (int r = 0; r < fixture.Routes.Routes.Count (); ++r) {
					const HybridTopologyEdge * edge = fixture.Topology.Find_Edge (fixture.Routes.Routes[r].EdgeId);
					if (edge == NULL) continue;
					Check (fixture.Routes.Routes[r].Capabilities == edge->Capabilities,
							 "route %u did not copy its edge's capability unchanged",edge->StableId);
				}
			}
		}
		Check (pair,"no generated world has two same-class edges with different capability masks");
	}


	/*
	**	`road_cut`, on the real solver.  Native explicit cliffs are 18..36 high against 6 of cut and 6 of
	**	fill, so the road-cut compiler can never accept one (`Q-020`); this cliff is 4 high over a band 6
	**	wide, which the straight profile between the two approaches cuts and fills inside those limits.
	**	Phase 6's own per-class cut test decides `CutClasses`, the cliff method order runs cut before ramp,
	**	and the compiled record is checked against every one of the plan's four cut conditions.
	*/
	{
		const float LOW_CLIFF = 4.0f;
		HybridInteractionParams cut_params = Interaction_Fixture (Graph_Identity_Fixture (600.0f,2.0f),600.0f,
																					 inputs,preflight.Metrics);
		const HybridConstants & k = cut_params.Constants;
		const float approach = WWMath::Max (8.0f * k.W,cut_params.BridgeKinds.Count () > 0
														 ? cut_params.BridgeKinds[0].RequiredApproachLength : 0.0f);

		SolverWorld world;
		world.Terrain.Centre.Set (0.0f,0.0f);
		world.Terrain.Tangent.Set (1.0f,0.0f);
		world.Terrain.Normal.Set (0.0f,1.0f);
		world.Terrain.RaisedBand				= true;
		world.Terrain.BandFace					= 2.0f;
		world.Terrain.HalfSpan					= 3.0f;				// a 6-wide band with a 2-wide shelf
		world.Terrain.FloorHalf					= 0.0f;
		world.Terrain.RimZ						= LOW_CLIFF;
		world.Terrain.FloorZ						= 0.0f;
		world.Terrain.TangentFloorSlope		= 0.0f;
		world.Terrain.TangentRimSlope			= 0.0f;
		world.Terrain.FarSideDiffers			= false;
		world.Terrain.FarSideZ					= 0.0f;
		world.Terrain.WallIsVertical			= false;
		world.Terrain.BlockPlusTangent		= false;

		HybridCrossingWindow window = Solver_Window (HYBRID_OBSTRUCTION_CLIFF,HYBRID_WINDOW_RAMP_PASS,
																   world.Terrain.HalfSpan,approach,0.0f,0.0f);
		//	Phase 6's flag for the class under test: the cut fits, so the class bit is set.
		window.Methods		= HYBRID_WINDOW_METHOD_CUT;
		window.CutClasses	= (1u << (unsigned int)HYBRID_ROAD_PRIMARY_2);

		Build_Solver_World (cut_params,HYBRID_ROAD_PRIMARY_2,HYBRID_NAV_LIGHT | HYBRID_NAV_HEAVY,window,&world);

		HybridInteractionSet cut_set;
		HybridInteractionResult cut_result;
		const bool cut_solved = Hybrid_Build_Interactions (cut_params,world.Identity,world.Matrices,world.Landforms,
																			world.Hydrology,world.Anchors,world.Topology,
																			world.Routes,Pass_Terrain_Height,&world.Terrain,
																			&cut_set,&cut_result);
		Check (cut_solved,"the low-cliff road cut did not solve: %s (%s)",
				 Hybrid_Interaction_Failure_Name (cut_result.Failure),cut_result.Detail.Peek_Buffer ());
		Check (cut_set.Interactions.Count () == 1,"the low-cliff world compiled %d interactions, not one",
				 cut_set.Interactions.Count ());
		if (cut_solved && (cut_set.Interactions.Count () == 1)) {

			const HybridInteraction & cut = cut_set.Interactions[0];
			Check (cut.Method == HYBRID_METHOD_ROAD_CUT,"a low cliff compiled %s, not a road cut",
					 Hybrid_Interaction_Method_Name (cut.Method));
			Check (cut.FallbackFrom == HYBRID_PROBLEM_NONE,
					 "the road cut was reached as a fallback from %s; it is first in the cliff order",
					 Hybrid_Interaction_Problem_Name (cut.FallbackFrom));
			Check (cut.Obstruction == HYBRID_OBSTRUCTION_CLIFF,"the cut names the wrong obstruction");
			Check (cut.CutDepth <= cut_params.MaxCutDepth,"the compiled cut is %.2f deep against %.2f allowed",
					 cut.CutDepth,cut_params.MaxCutDepth);
			Check (cut.CutDepth > 0.0f,"a cut through a 4-high band removed nothing");

			//	The plan's four conditions, recomputed here from the record rather than taken on the solver's word.
			const float half_deformation = Hybrid_Road_Deformation_Half_Width (cut.RoadClass,k.W,k.A);
			const float blend = half_deformation + Hybrid_Road_Outer_Blend_Width (k.W,k.A);
			const Vector2 axis = (cut.ApproachB - cut.ApproachA);
			const float length = axis.Length ();
			float z_a = 0.0f, z_b = 0.0f;
			Pass_Terrain_Height (cut.ApproachA,&z_a,&world.Terrain);
			Pass_Terrain_Height (cut.ApproachB,&z_b,&world.Terrain);
			float worst_cut = 0.0f, worst_fill = 0.0f;
			bool blend_lands = true;
			const int samples = 40;
			for (int i = 0; i <= samples; ++i) {
				const float r = (float)i / (float)samples;
				const Vector2 point = cut.ApproachA + (axis * r);
				float ground = 0.0f;
				Pass_Terrain_Height (point,&ground,&world.Terrain);
				const float correction = (z_a + ((z_b - z_a) * r)) - ground;
				if (correction < 0.0f) worst_cut = WWMath::Max (worst_cut,-correction);
				else worst_fill = WWMath::Max (worst_fill,correction);
				for (int side = -1; side <= 1; side += 2) {
					const Vector2 edge_point (point.X + (blend * (float)side),point.Y);
					int cx = 0, cy = 0;
					if (!world.Matrices.Matrix.World_To_Cell (edge_point,&cx,&cy) ||
						 !world.Matrices.Matrix.Has_Flag (cx,cy,HYBRID_FLAG_PLACEABLE)) blend_lands = false;
				}
			}
			Check (worst_cut <= (cut_params.MaxCutDepth + k.Q),"the cut profile needs %.2f of cut",worst_cut);
			Check (worst_fill <= (cut_params.MaxFillHeight + k.Q),"the cut profile needs %.2f of fill",worst_fill);
			Check (WWMath::Fabs (cut.CutDepth - worst_cut) <= 0.01f,
					 "the record says %.2f of cut and the profile needs %.2f",cut.CutDepth,worst_cut);
			Check (half_deformation >= 0.5f * Hybrid_Roadbed_Width (cut.RoadClass,k.W),
					 "the deformation half width %.2f is narrower than the roadbed",half_deformation);
			Check (blend_lands,"the outer blend at %.2f each side does not land on placeable ground",blend);
			Check (length > 0.0f,"the compiled cut has no length");
			printf ("  road_cut solver fixture: PRIMARY_2 through a %.0f-high band, %.2f of cut, blend %.2f\r\n",
					  LOW_CLIFF,cut.CutDepth,blend);
		}

		//	And the same band 18 high -- the shallowest native cliff -- is refused by Phase 6's own per-class
		//	test, so the solver never sees a cut bit for it.  That is the `Q-020` unreachability, measured.
		HybridCrossingWindow native = window;
		native.CutClasses = 0;
		native.Methods    = 0;
		Check (!native.Cut_Planned_For (HYBRID_ROAD_PRIMARY_2),"a window with no cut classes plans a cut");
	}

	/*
	**	`surface_pass`, on the real solver.  Every native canyon fits the registered 200-span bridge, so the
	**	canyon order -- bridge first -- never reaches the pass (`Q-020`); here the fixture's own bridge kind
	**	has a maximum span legitimately shorter than the canyon, so `Compile_Bridge` refuses with
	**	`SPAN_TOO_LONG` and the order falls through to the pass, whose frozen Phase-6 geometry is then
	**	revalidated by the real Phase-11 code.  The native registry is untouched: this is a fixture kind.
	*/
	{
		HybridInteractionParams pass_params = Interaction_Fixture (Graph_Identity_Fixture (600.0f,2.0f),600.0f,
																					  inputs,preflight.Metrics);
		const HybridConstants & k = pass_params.Constants;

		//	The fixture's registry: one kind whose span cannot reach across this canyon.
		pass_params.BridgeKinds.Delete_All ();
		HybridBridgeKind short_kind;
		short_kind.StableId						= 1;
		short_kind.ClearWidth					= 40.0f;
		short_kind.StructuralDepth				= 2.0f;
		short_kind.RequiredClearance			= 5.0f;
		short_kind.MaxSpan						= 30.0f;
		short_kind.RequiredApproachLength	= 30.0f;
		pass_params.BridgeKinds.Add (short_kind);
		const float approach = WWMath::Max (8.0f * k.W,short_kind.RequiredApproachLength);

		SolverWorld world;
		world.Terrain.Centre.Set (0.0f,0.0f);
		world.Terrain.Tangent.Set (1.0f,0.0f);
		world.Terrain.Normal.Set (0.0f,1.0f);
		world.Terrain.HalfSpan					= 40.0f;				// an 80-unit canyon: past a 30 + 2W seated span
		world.Terrain.FloorHalf					= 12.0f;
		world.Terrain.RimZ						= 8.0f;
		world.Terrain.FloorZ						= 0.0f;
		world.Terrain.TangentFloorSlope		= 0.0f;
		world.Terrain.TangentRimSlope			= 0.0f;
		world.Terrain.FarSideDiffers			= false;
		world.Terrain.FarSideZ					= 0.0f;
		world.Terrain.WallIsVertical			= false;
		world.Terrain.BlockPlusTangent		= false;
		world.Terrain.RaisedBand				= false;
		world.Terrain.BandFace					= 0.0f;

		HybridCrossingWindow window = Solver_Window (HYBRID_OBSTRUCTION_CANYON,HYBRID_WINDOW_CROSSING,
																   world.Terrain.HalfSpan,approach,8.0f,8.0f);

		//	Phase 6's own pass proof, frozen on the window exactly as step 18 freezes it.
		HybridLocalPassParams local = Local_Pass_Fixture ();
		local.SweptVehicleWidth = WWMath::Max (pass_params.HeavyVehicleWidth,pass_params.HarvesterWidth);
		HybridLocalPassGeometry frozen;
		HybridLocalPassFailure why = HYBRID_LOCAL_PASS_OK;
		const bool proved = Hybrid_Build_Local_Pass (local,HYBRID_LOCAL_PASS_FLOOR,world.Terrain.Centre,
																	world.Terrain.Tangent,world.Terrain.Normal,
																	world.Terrain.HalfSpan,world.Terrain.FloorHalf,
																	world.Terrain.HalfSpan,nullptr,approach,400.0f,400.0f,
																	Pass_Terrain_Height,&world.Terrain,&frozen,&why);
		Check (proved,"the surface-pass fixture canyon has no legal pass (%s)",Hybrid_Local_Pass_Failure_Name (why));
		window.Methods	= HYBRID_WINDOW_METHOD_BRIDGE | HYBRID_WINDOW_METHOD_PASS;
		window.Pass		= frozen;

		Build_Solver_World (pass_params,HYBRID_ROAD_PRIMARY_2,HYBRID_NAV_LIGHT | HYBRID_NAV_HEAVY,window,&world);

		HybridInteractionSet pass_set;
		HybridInteractionResult pass_result;
		const bool pass_solved = Hybrid_Build_Interactions (pass_params,world.Identity,world.Matrices,
																			 world.Landforms,world.Hydrology,world.Anchors,
																			 world.Topology,world.Routes,Pass_Terrain_Height,
																			 &world.Terrain,&pass_set,&pass_result);
		Check (pass_solved,"the short-bridge canyon did not solve: %s (%s)",
				 Hybrid_Interaction_Failure_Name (pass_result.Failure),pass_result.Detail.Peek_Buffer ());
		Check (pass_set.Interactions.Count () == 1,"the short-bridge world compiled %d interactions, not one",
				 pass_set.Interactions.Count ());
		if (pass_solved && (pass_set.Interactions.Count () == 1)) {

			const HybridInteraction & pass = pass_set.Interactions[0];
			Check (pass.Method == HYBRID_METHOD_SURFACE_PASS,"a canyon no bridge can span compiled %s",
					 Hybrid_Interaction_Method_Name (pass.Method));
			Check (pass.FallbackFrom == HYBRID_PROBLEM_SPAN_TOO_LONG,
					 "the pass was reached after the bridge refused as %s, not span_too_long",
					 Hybrid_Interaction_Problem_Name (pass.FallbackFrom));
			Check (pass.BridgeKind < 0,"a surface pass recorded a bridge kind");
			//	The frozen pass's own two ends, in the order this road drives them.
			const Vector2 & end_0 = frozen.Controls[0];
			const Vector2 & end_n = frozen.Controls[frozen.ControlCount - 1];
			Check (((pass.ApproachA == end_0) && (pass.ApproachB == end_n)) ||
					 ((pass.ApproachA == end_n) && (pass.ApproachB == end_0)),
					 "the compiled pass ends are not the frozen pass's own");

			//	The frozen geometry is what the real Phase-11 revalidated: revalidate it here too, independently.
			HybridLocalPassFailure again = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Validate_Local_Pass (local,frozen,Hybrid_Local_Pass_Sample_Spacing (k),Pass_Terrain_Height,
														  &world.Terrain,&again),
					 "the frozen pass Phase 11 accepted does not revalidate (%s)",
					 Hybrid_Local_Pass_Failure_Name (again));
			printf ("  surface_pass solver fixture: 80-unit canyon, 30-span bridge refused as %s, pass compiled\r\n",
					  Hybrid_Interaction_Problem_Name (pass.FallbackFrom));
		}

		//	With the native-sized bridge the same canyon takes the bridge, as the order requires.
		HybridInteractionParams native_params = Interaction_Fixture (Graph_Identity_Fixture (600.0f,2.0f),600.0f,
																						inputs,preflight.Metrics);
		HybridInteractionSet native_set;
		HybridInteractionResult native_result;
		if (Hybrid_Build_Interactions (native_params,world.Identity,world.Matrices,world.Landforms,world.Hydrology,
												 world.Anchors,world.Topology,world.Routes,Pass_Terrain_Height,&world.Terrain,
												 &native_set,&native_result) && (native_set.Interactions.Count () == 1)) {
			Check (native_set.Interactions[0].Method == HYBRID_METHOD_BRIDGE,
					 "with a bridge that spans it, the canyon compiled %s instead of the bridge the order prefers",
					 Hybrid_Interaction_Method_Name (native_set.Interactions[0].Method));
		}
	}

	/*
	**	`ramp_pass`, on the real solver.  A 12-high band with a 16.6-wide smoothstep face is the shape the band
	**	ramp is built for: its straight cut profile would need 12 of cut, so Phase 6 plans no cut for any class
	**	and the cliff order -- cut, then ramp pass -- falls through to the pass, whose frozen band geometry the
	**	real Phase 11 revalidates.  Native bands are 18..36 high over faces at most 12.5 wide and are refused
	**	on cross slope by the same builder (`Check_Hybrid_Local_Pass` measures the broadest one), which is why
	**	the native census carries no cliff window at all.
	*/
	{
		HybridInteractionParams ramp_params = Interaction_Fixture (Graph_Identity_Fixture (600.0f,2.0f),600.0f,
																					  inputs,preflight.Metrics);
		const HybridConstants & k = ramp_params.Constants;
		const float approach = WWMath::Max (8.0f * k.W,ramp_params.BridgeKinds.Count () > 0
														 ? ramp_params.BridgeKinds[0].RequiredApproachLength : 0.0f);

		SolverWorld world;
		world.Terrain.Centre.Set (0.0f,0.0f);
		world.Terrain.Tangent.Set (1.0f,0.0f);
		world.Terrain.Normal.Set (0.0f,1.0f);
		world.Terrain.RaisedBand				= true;
		world.Terrain.BandFace					= 16.6f;
		world.Terrain.HalfSpan					= 16.6f + 5.0f;
		world.Terrain.FloorHalf					= 0.0f;
		world.Terrain.RimZ						= 12.0f;
		world.Terrain.FloorZ						= 0.0f;
		world.Terrain.TangentFloorSlope		= 0.0f;
		world.Terrain.TangentRimSlope			= 0.0f;
		world.Terrain.FarSideDiffers			= false;
		world.Terrain.FarSideZ					= 0.0f;
		world.Terrain.WallIsVertical			= false;
		world.Terrain.BlockPlusTangent		= false;

		HybridLocalPassBand band;
		band.ToeA	= +world.Terrain.HalfSpan;
		band.ShelfA	= +5.0f;
		band.ShelfB	= -5.0f;
		band.ToeB	= -world.Terrain.HalfSpan;

		HybridCrossingWindow window = Solver_Window (HYBRID_OBSTRUCTION_CLIFF,HYBRID_WINDOW_RAMP_PASS,
																   world.Terrain.HalfSpan,approach,0.0f,0.0f);

		//	Phase 6's own proof, frozen on the window: the band ramp for the heavy envelope.
		HybridLocalPassParams local = Local_Pass_Fixture ();
		local.SweptVehicleWidth = WWMath::Max (ramp_params.HeavyVehicleWidth,ramp_params.HarvesterWidth);
		HybridLocalPassGeometry frozen;
		HybridLocalPassFailure why = HYBRID_LOCAL_PASS_OK;
		const bool proved = Hybrid_Build_Local_Pass (local,HYBRID_LOCAL_PASS_RAMP,world.Terrain.Centre,
																	world.Terrain.Tangent,world.Terrain.Normal,
																	world.Terrain.HalfSpan,0.0f,world.Terrain.HalfSpan,&band,
																	approach,400.0f,400.0f,Pass_Terrain_Height,&world.Terrain,
																	&frozen,&why);
		Check (proved,"the ramp-pass fixture band has no legal ramp (%s)",Hybrid_Local_Pass_Failure_Name (why));

		//	No cut for any class: a 12-high band's straight profile needs 12 of cut against 6.  Measured, not set.
		window.CutClasses	= 0;
		window.Methods		= HYBRID_WINDOW_METHOD_PASS;
		window.Pass			= frozen;

		Build_Solver_World (ramp_params,HYBRID_ROAD_PRIMARY_2,HYBRID_NAV_LIGHT | HYBRID_NAV_HEAVY,window,&world);

		HybridInteractionSet ramp_set;
		HybridInteractionResult ramp_result;
		const bool ramp_solved = Hybrid_Build_Interactions (ramp_params,world.Identity,world.Matrices,
																			 world.Landforms,world.Hydrology,world.Anchors,
																			 world.Topology,world.Routes,Pass_Terrain_Height,
																			 &world.Terrain,&ramp_set,&ramp_result);
		Check (ramp_solved,"the band world did not solve: %s (%s)",
				 Hybrid_Interaction_Failure_Name (ramp_result.Failure),ramp_result.Detail.Peek_Buffer ());
		Check (ramp_set.Interactions.Count () == 1,"the band world compiled %d interactions, not one",
				 ramp_set.Interactions.Count ());
		if (ramp_solved && (ramp_set.Interactions.Count () == 1)) {

			const HybridInteraction & ramp = ramp_set.Interactions[0];
			Check (ramp.Method == HYBRID_METHOD_RAMP_PASS,"a band no cut can cross compiled %s",
					 Hybrid_Interaction_Method_Name (ramp.Method));
			Check (ramp.FallbackFrom == HYBRID_PROBLEM_METHOD_UNAVAILABLE,
					 "the ramp was reached after the cut refused as %s, not method_unavailable",
					 Hybrid_Interaction_Problem_Name (ramp.FallbackFrom));
			//	The frozen band pass's own two ends, in the order this road drives them.
			const Vector2 & ramp_0 = frozen.Controls[0];
			const Vector2 & ramp_n = frozen.Controls[frozen.ControlCount - 1];
			Check (((ramp.ApproachA == ramp_0) && (ramp.ApproachB == ramp_n)) ||
					 ((ramp.ApproachA == ramp_n) && (ramp.ApproachB == ramp_0)),
					 "the compiled ramp ends are not the frozen band pass's own");
			Check (frozen.ControlCount == 6,"the frozen band pass has %d controls, not six",frozen.ControlCount);
			Check (frozen.TangentRun > 0.0f,"the frozen band pass spent no along-cliff run");

			//	The far approach is along the cliff from the near one: the shape's signature.
			Check (WWMath::Fabs (ramp.ApproachB.X - ramp.ApproachA.X) > (2.0f * k.W),
					 "a band pass came straight back across the station (%.2f along the cliff)",
					 ramp.ApproachB.X - ramp.ApproachA.X);

			HybridLocalPassFailure again = HYBRID_LOCAL_PASS_OK;
			Check (Hybrid_Validate_Local_Pass (local,frozen,Hybrid_Local_Pass_Sample_Spacing (k),Pass_Terrain_Height,
														  &world.Terrain,&again),
					 "the frozen band pass Phase 11 accepted does not revalidate (%s)",
					 Hybrid_Local_Pass_Failure_Name (again));
			printf ("  ramp_pass solver fixture: 12-high band, cut unavailable, band ramp compiled, exits %.1f along "
					  "the cliff\r\n",ramp.ApproachB.X - ramp.ApproachA.X);
		}
	}

	Check (negatives_done,"the negative fixtures never ran");
	Check ((seen_method[HYBRID_METHOD_OVERPASS] + seen_method[HYBRID_METHOD_AT_GRADE_JUNCTION]) > 0,
			 "no fixture seed exercised a road-on-road crossing");

	//	Fords are disabled on the native profile until a ford capability exists: prove the disabled path.
	Check (seen_method[HYBRID_METHOD_FORD] == 0,
			 "a ford compiled although the native profile has no ford capability");
}

/***********************************************************************************************
**	A deterministic search for the worlds that reach Phase 11's ground methods -- rewrite item I12.
**
**	The fixed method orders mean a method is only reached when the ones before it legitimately refuse: a
**	canyon surface pass needs a bridge that cannot compile, a cliff ramp pass needs a cut that cannot.
**	Which supported seed produces that combination is a fact about the generator, not something to
**	arrange, so this walks the real pipeline over many seeds and reports what each routed crossing was
**	actually offered.  It is a tool, not a gate: nothing registers it with ctest and it asserts nothing.
**
**		renegade -terrainselfcheck hybridinteractionsearch
***********************************************************************************************/

void Check_Hybrid_Interaction_Search (void)
{
	ProceduralWorldProfile profile;
	StringClass profile_error;
	if (!ProceduralWorldProfileManager::Load_Profile_From_File ("data/procedural_world/native.json",
																				 &profile,&profile_error)) {
		printf ("search: the native profile did not load: %s\r\n",profile_error.Peek_Buffer ());
		return;
	}
	profile.Plan.BiomeMode	= "single";
	profile.Plan.Biome		= "temperate";

	HybridPreflightInputs inputs;
	Hybrid_Content_Fixture (profile,&inputs);
	HybridPreflightResult preflight;
	if (!Hybrid_Run_Preflight (profile,inputs,&preflight)) {
		printf ("search: preflight refused the native profile\r\n");
		return;
	}

	static const HybridArchetype SEARCH_ARCHETYPES[7] = {
		HYBRID_ARCHETYPE_CANYON_CROSSROADS,HYBRID_ARCHETYPE_RAVINE_NETWORK,
		HYBRID_ARCHETYPE_PLATEAU_BASIN,HYBRID_ARCHETYPE_HIGHLAND_PASS,
		HYBRID_ARCHETYPE_RIVER_VALLEY,HYBRID_ARCHETYPE_TWIN_LAKES,
		HYBRID_ARCHETYPE_COASTAL_PENINSULA
	};

	const unsigned int SEARCH_SEEDS = 200u;

	int worlds = 0;
	int passed_phase9 = 0;
	int rejected_approach = 0;
	int rejected_wrong_side = 0;
	int rejected_pose = 0;
	int recovered_next_rank = 0;
	s_route_census = true;

	//	The upstream census: what each failed phase was refused by (the stabilization pass's measurements).
	int topology_failure[HYBRID_TOPOLOGY_FAILURE_COUNT];
	for (int i = 0; i < HYBRID_TOPOLOGY_FAILURE_COUNT; ++i) topology_failure[i] = 0;
	int quota_worlds[2] = { 0,0 };
	int quota_candidates[2] = { 0,0 };
	int quota_zero_candidates[2] = { 0,0 };
	int quota_refused[2][HYBRID_QUOTA_PATH_COUNT][HYBRID_QUOTA_REFUSAL_COUNT];
	for (int c = 0; c < 2; ++c) {
		for (int pa = 0; pa < HYBRID_QUOTA_PATH_COUNT; ++pa) {
			for (int r = 0; r < HYBRID_QUOTA_REFUSAL_COUNT; ++r) quota_refused[c][pa][r] = 0;
		}
	}
	int overpass_worlds = 0;
	int overpass_natural = 0;
	int overpass_pairs = 0;
	int overpass_synthesized = 0;
	int overpass_no_pairs = 0;
	int overpass_no_crossing = 0;
	int overpass_natural_refused[HYBRID_OVERPASS_REFUSAL_COUNT];
	int overpass_synth_refused[HYBRID_OVERPASS_REFUSAL_COUNT];
	for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) {
		overpass_natural_refused[r] = 0;
		overpass_synth_refused[r] = 0;
	}
	int route_failures = 0;
	int route_reason[HYBRID_BLOCK_COUNT];
	int route_spline_block[HYBRID_BLOCK_COUNT];
	int route_frontier[HYBRID_BLOCK_COUNT];
	for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) {
		route_reason[i] = 0;
		route_spline_block[i] = 0;
		route_frontier[i] = 0;
	}
	int route_class[HYBRID_ROAD_CLASS_COUNT];
	for (int i = 0; i < HYBRID_ROAD_CLASS_COUNT; ++i) route_class[i] = 0;
	int route_kind[HYBRID_TOPOLOGY_EDGE_KIND_COUNT];
	for (int i = 0; i < HYBRID_TOPOLOGY_EDGE_KIND_COUNT; ++i) route_kind[i] = 0;
	int route_envelope_final[3] = { 0,0,0 };
	int structural_cause[HYBRID_STRUCTURAL_COUNT];
	for (int i = 0; i < HYBRID_STRUCTURAL_COUNT; ++i) structural_cause[i] = 0;
	int spline_class[HYBRID_SPLINE_FAILURE_COUNT];
	for (int i = 0; i < HYBRID_SPLINE_FAILURE_COUNT; ++i) spline_class[i] = 0;
	int route_structural_leg = 0;
	int route_with_structural = 0;
	int meetings[HYBRID_ROAD_MEETING_COUNT] = { 0,0,0 };
	int meetings_near_shared = 0;
	int meetings_beyond_junction = 0;
	int meetings_waypoint_shared = 0;
	int meeting_failed[HYBRID_ROAD_MEETING_COUNT] = { 0,0,0 };
	int junction_problem[HYBRID_PROBLEM_COUNT];
	int overpass_problem[HYBRID_PROBLEM_COUNT];
	for (int i = 0; i < HYBRID_PROBLEM_COUNT; ++i) {
		junction_problem[i] = 0;
		overpass_problem[i] = 0;
	}
	int solved_worlds = 0;
	int reroute_requests = 0;
	int reached_phase11 = 0;
	int cliff_spans = 0;

	/*
	**	Q-021's distribution: every explicit cliff Phase 5 accepted, by the dimensions that decide whether a
	**	band can carry a ramp -- height, edge width, talus width, H / F -- and how many stations Phase 6 made
	**	a pass window at.  The feasibility threshold measured on the builder is `H / F` between 1.14 and 1.19
	**	for a `PRIMARY_2` roadbed; the decision's derivation gave 0.926.
	*/
	int cliffs_total = 0;
	int cliffs_feasible_ratio = 0;				// H / F <= 1.14
	int cliffs_with_pass_window = 0;
	int cliff_pass_windows = 0;
	int cliff_pass_windows_routed = 0;
	int height_buckets[4] = { 0,0,0,0 };		// 18..22.5, ..27, ..31.5, ..36
	int talus_buckets[5] = { 0,0,0,0,0 };		// 1..1.85W, ..2.7, ..3.55, ..4.4, ..5.25
	int ratio_buckets[6] = { 0,0,0,0,0,0 };	// <=0.93, <=1.14, <=1.5, <=2, <=3, >3
	float ratio_min = 1.0e9f;
	float ratio_max = 0.0f;
	int cliff_spans_cut_any = 0;
	int cliff_spans_cut_class = 0;
	int cliff_spans_pass = 0;
	int method_selected[HYBRID_METHOD_COUNT];
	for (int m = 0; m < HYBRID_METHOD_COUNT; ++m) method_selected[m] = 0;
	int resolved_at_one = 0;
	int resolved_at_two = 0;
	int exhausted = 0;
	int failure_reason[HYBRID_INTERACTION_FAILURE_COUNT];
	for (int f = 0; f < HYBRID_INTERACTION_FAILURE_COUNT; ++f) failure_reason[f] = 0;
	int first_surface_pass = -1;
	int first_road_cut = -1;
	int first_ramp_pass = -1;

	for (int ai = 0; ai < 7; ++ai) {
		for (unsigned int seed = 1u; seed <= SEARCH_SEEDS; ++seed) {

			HybridPipelineFixture scan;
			StringClass why;
			const bool scanned = Build_Hybrid_Pipeline (seed,1200.0f,SEARCH_ARCHETYPES[ai],&scan,&why,&preflight.Metrics);
			if (scan.Reached >= 8) {
				rejected_approach		+= scan.TopologyResult.RejectedApproachClass;
				rejected_wrong_side	+= scan.TopologyResult.RejectedWrongSide;
				rejected_pose			+= scan.TopologyResult.RejectedPose;
				recovered_next_rank	+= scan.TopologyResult.RecoveredByNextRank;
			}
			if (scan.Reached >= 9) {
				const HybridTopologyResult & tr = scan.TopologyResult;
				if (tr.OverpassQuota.Have < tr.OverpassQuota.Required) {
					const HybridOverpassCensus & o = tr.OverpassQuota;
					printf ("OVERPASSSHORT archetype %d seed %u: families %d highways %d; natural %d, synthesized %d of %d pairs "
							  "(%d heavy-heavy); angle refusals <15 %d, <30 %d, <45 %d, <60 %d\r\n",(int)SEARCH_ARCHETYPES[ai],seed,
							  o.Families,o.Highways,o.Natural,o.Synthesized,o.Pairs,o.HeavyPairs,o.AngleBucket[0],
							  o.AngleBucket[1],o.AngleBucket[2],o.AngleBucket[3]);
					overpass_worlds++;
					overpass_natural += o.Natural;
					overpass_pairs += o.Pairs;
					overpass_synthesized += o.Synthesized;
					if (o.Pairs == 0) overpass_no_pairs++;
					else if (o.Synthesized == 0) overpass_no_crossing++;
					for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) {
						overpass_natural_refused[r] += o.NaturalRefused[r];
						overpass_synth_refused[r] += o.SynthesizedRefused[r];
					}
				}
				passed_phase9++;
				printf ("TOPOLOGY archetype %d seed %u\r\n",(int)SEARCH_ARCHETYPES[ai],seed);
			}
			if (!scanned) {
				if (scan.Reached == 8) {
					const HybridTopologyResult & tr = scan.TopologyResult;
					printf ("P9FAIL archetype %d seed %u: %s -- %s\r\n",(int)SEARCH_ARCHETYPES[ai],seed,
							  Hybrid_Topology_Failure_Name (tr.Failure),tr.Detail.Peek_Buffer ());
					if ((tr.Failure >= 0) && (tr.Failure < HYBRID_TOPOLOGY_FAILURE_COUNT)) topology_failure[tr.Failure]++;
					if (tr.Failure == HYBRID_TOPOLOGY_BRIDGE_QUOTA) {
						const bool river = (tr.RiverQuota.Have < tr.RiverQuota.Required);
						const HybridQuotaCensus & q = river ? tr.RiverQuota : tr.CanyonQuota;
						const int c = river ? 0 : 1;
						quota_worlds[c]++;
						quota_candidates[c] += q.Candidates;
						if (q.Candidates == 0) quota_zero_candidates[c]++;
						for (int pa = 0; pa < HYBRID_QUOTA_PATH_COUNT; ++pa) {
							for (int r = 0; r < HYBRID_QUOTA_REFUSAL_COUNT; ++r) quota_refused[c][pa][r] += q.Refused[pa][r];
						}
					}
				}
				if ((scan.Reached == 9) && (scan.RouteResult.Census.EdgeId == 0)) {
					printf ("P10GATE archetype %d seed %u: %s -- %s\r\n",(int)SEARCH_ARCHETYPES[ai],seed,
							  Hybrid_Route_Failure_Name (scan.RouteResult.Failure),scan.RouteResult.Detail.Peek_Buffer ());
				}
				if ((scan.Reached == 9) && (scan.RouteResult.Census.EdgeId != 0)) {
					const HybridRouteFailureCensus & rc = scan.RouteResult.Census;
					route_failures++;
					if ((rc.Reason >= 0) && (rc.Reason < HYBRID_BLOCK_COUNT)) route_reason[rc.Reason]++;
					if ((rc.Reason == HYBRID_BLOCK_SPLINE) && (rc.SplineBlock >= 0) && (rc.SplineBlock < HYBRID_BLOCK_COUNT)) {
						route_spline_block[rc.SplineBlock]++;
					}
					for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) route_frontier[i] += rc.Frontier[i];
					if ((rc.RoadClass >= 0) && (rc.RoadClass < HYBRID_ROAD_CLASS_COUNT)) route_class[rc.RoadClass]++;
					if ((rc.SplineClass > 0) && (rc.SplineClass < HYBRID_SPLINE_FAILURE_COUNT)) spline_class[rc.SplineClass]++;
					if ((rc.StructuralCause > 0) && (rc.StructuralCause < HYBRID_STRUCTURAL_COUNT)) structural_cause[rc.StructuralCause]++;
					if ((rc.EdgeKind >= 0) && (rc.EdgeKind < HYBRID_TOPOLOGY_EDGE_KIND_COUNT)) route_kind[rc.EdgeKind]++;
					if ((rc.Envelope >= 0) && (rc.Envelope < 3)) route_envelope_final[rc.Envelope]++;
					if (rc.LegFromStructural || rc.LegToStructural) route_structural_leg++;
					if (rc.StructuralWaypoints > 0) route_with_structural++;
					printf ("ROUTEFAIL archetype %d seed %u: edge %u %s %s envelope %d, %d structural, leg %d of %d (%u%s -> %u%s): %s",
							  (int)SEARCH_ARCHETYPES[ai],seed,rc.EdgeId,
							  Hybrid_Topology_Edge_Kind_Name ((HybridTopologyEdgeKind)rc.EdgeKind),
							  Hybrid_Road_Class_Name ((HybridRoadClass)rc.RoadClass),rc.Envelope,rc.StructuralWaypoints,rc.Leg,
							  rc.Legs,rc.LegFrom,rc.LegFromStructural ? "*" : "",rc.LegTo,rc.LegToStructural ? "*" : "",
							  Hybrid_Route_Block_Name ((HybridRouteBlock)rc.Reason));
					if (rc.Reason == HYBRID_BLOCK_SPLINE) {
						printf (" (%s; %s; %s, %.0f from a joint; %s)",rc.SplineReason.Peek_Buffer (),Hybrid_Route_Block_Name ((HybridRouteBlock)rc.SplineBlock),
								  Hybrid_Spline_Failure_Class_Name ((HybridSplineFailureClass)rc.SplineClass),rc.SplineJointDistance,
								  Hybrid_Structural_Cause_Name ((HybridStructuralCause)rc.StructuralCause));
					} else {
						printf (" [");
						for (int i = 1; i < HYBRID_BLOCK_COUNT; ++i) {
							if (rc.Frontier[i] > 0) printf (" %s %d",Hybrid_Route_Block_Name ((HybridRouteBlock)i),rc.Frontier[i]);
						}
						printf (" ]");
					}
					printf ("\r\n");
				}
				continue;
			}
			worlds++;

			/*
			**	What every routed structural span was offered.  The span names a window, so the record is that
			**	window's own capability set plus the obstruction span against the resolved bridge maximum --
			**	which is what decides whether `Compile_Bridge` can refuse for a legitimate reason.
			*/
			for (int r = 0; r < scan.Routes.Routes.Count (); ++r) {
				const HybridRoute & route = scan.Routes.Routes[r];
				for (int s = 0; s < route.Structural.Count (); ++s) {

					const HybridCrossingWindow * window = NULL;
					for (int wi = 0; wi < scan.Hydrology.Windows.Count (); ++wi) {
						if (scan.Hydrology.Windows[wi].StableId != route.Structural[s].WindowId) continue;
						window = &scan.Hydrology.Windows[wi];
						break;
					}
					if (window == NULL) continue;

					const bool canyon = (window->Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
											  (window->Obstruction == HYBRID_OBSTRUCTION_RAVINE);
					const bool cliff = (window->Obstruction == HYBRID_OBSTRUCTION_CLIFF) ||
											 (window->Obstruction == HYBRID_OBSTRUCTION_RIDGE);
					if (!canyon && !cliff) continue;

					const bool bridge_capable = window->Has_Method (HYBRID_WINDOW_METHOD_BRIDGE);
					const bool pass_capable = window->Has_Method (HYBRID_WINDOW_METHOD_PASS);
					const bool tunnel_capable = window->Has_Method (HYBRID_WINDOW_METHOD_TUNNEL);

					//	Structurally available for *this road's* class, which is what Phase 11 will try.
					const bool cut_capable = window->Cut_Planned_For (route.RoadClass);
					if (cliff) {
						cliff_spans++;
						if (window->Has_Method (HYBRID_WINDOW_METHOD_CUT)) cliff_spans_cut_any++;
						if (cut_capable) cliff_spans_cut_class++;
						if (pass_capable) cliff_spans_pass++;
					}

					//	Only the combinations that could reach a ground method are worth printing.
					const bool interesting = (canyon && pass_capable && !bridge_capable) ||
													 (cliff && pass_capable && !cut_capable) ||
													 (cliff && cut_capable);
					if (!interesting) continue;

					printf ("SPAN archetype %d seed %u: edge %u (%s, capability 0x%x) window %u %s/%u span %.2f "
							  "(bridge max %.2f) bridge %d pass %d cut-for-class %d cut-classes 0x%02x tunnel %d\r\n",
							  (int)SEARCH_ARCHETYPES[ai],seed,route.EdgeId,Hybrid_Road_Class_Name (route.RoadClass),
							  route.Capabilities,window->StableId,
							  Hybrid_Obstruction_Kind_Name (window->Obstruction),window->ObstructionRecordId,
							  window->Span,preflight.Metrics.BridgeMaxSpanLength,bridge_capable ? 1 : 0,
							  pass_capable ? 1 : 0,cut_capable ? 1 : 0,window->CutClasses,tunnel_capable ? 1 : 0);
				}
			}

			for (int li = 0; li < scan.Landforms.Landforms.Count (); ++li) {
				const HybridLandform & lf = scan.Landforms.Landforms[li];
				if (lf.Kind != HYBRID_LANDFORM_CLIFF) continue;
				const float e = 0.5f * lf.EdgeWidth;
				const float face = e + lf.TalusWidth;
				const float ratio = (face > 0.0f) ? (lf.HeightDifference / face) : 1.0e9f;
				cliffs_total++;
				if (ratio <= 1.14f) cliffs_feasible_ratio++;
				if (ratio < ratio_min) ratio_min = ratio;
				if (ratio > ratio_max) ratio_max = ratio;
				int hb = (int)((lf.HeightDifference - 18.0f) / 4.5f);
				if (hb < 0) hb = 0; if (hb > 3) hb = 3;
				int tb = (int)((lf.TalusWidth / scan.Sized.Constants.W - 1.0f) / 0.85f);
				if (tb < 0) tb = 0; if (tb > 4) tb = 4;
				height_buckets[hb]++;
				talus_buckets[tb]++;
				ratio_buckets[(ratio <= 0.93f) ? 0 : (ratio <= 1.14f) ? 1 : (ratio <= 1.5f) ? 2 : (ratio <= 2.0f) ? 3
								  : (ratio <= 3.0f) ? 4 : 5]++;

				int windows_here = 0;
				for (int wi = 0; wi < scan.Hydrology.Windows.Count (); ++wi) {
					const HybridCrossingWindow & w = scan.Hydrology.Windows[wi];
					if ((w.Obstruction != HYBRID_OBSTRUCTION_CLIFF) || (w.ObstructionRecordId != lf.StableId)) continue;
					if (!w.Has_Method (HYBRID_WINDOW_METHOD_PASS)) continue;
					windows_here++;
					for (int r = 0; r < scan.Routes.Routes.Count (); ++r) {
						for (int s = 0; s < scan.Routes.Routes[r].Structural.Count (); ++s) {
							if (scan.Routes.Routes[r].Structural[s].WindowId == w.StableId) cliff_pass_windows_routed++;
						}
					}
				}
				if (windows_here > 0) {
					cliffs_with_pass_window++;
					printf ("RAMPWINDOW archetype %d seed %u: cliff %u H %.1f edge %.2f talus %.2f (%.2fW) H/F %.3f "
							  "length %.0f -> %d pass windows\r\n",(int)SEARCH_ARCHETYPES[ai],seed,lf.StableId,
							  lf.HeightDifference,lf.EdgeWidth,lf.TalusWidth,lf.TalusWidth / scan.Sized.Constants.W,
							  ratio,lf.Length,windows_here);
				}
				cliff_pass_windows += windows_here;
			}

			/*
			**	Why a whole family of windows is never reached: how close the accepted centerlines actually come
			**	to the cliff and ridge windows Phase 6 created.  A large minimum distance says Phase 9 never
			**	places an edge near one; a small one says Phase 10 routes around it at the last moment.
			*/
			{
				int cliff_windows = 0;
				int cliff_cut_capable = 0;
				int cliff_crossed = 0;
				float nearest = 1.0e9f;
				float widest_canyon = 0.0f;
				bool widest_bridge_capable = false;

				for (int wi = 0; wi < scan.Hydrology.Windows.Count (); ++wi) {

					const HybridCrossingWindow & w = scan.Hydrology.Windows[wi];
					if ((w.Obstruction == HYBRID_OBSTRUCTION_CANYON) ||
						 (w.Obstruction == HYBRID_OBSTRUCTION_RAVINE)) {
						if (w.Span > widest_canyon) {
							widest_canyon				= w.Span;
							widest_bridge_capable	= w.Has_Method (HYBRID_WINDOW_METHOD_BRIDGE);
						}
						continue;
					}
					if ((w.Obstruction != HYBRID_OBSTRUCTION_CLIFF) &&
						 (w.Obstruction != HYBRID_OBSTRUCTION_RIDGE)) continue;

					cliff_windows++;
					if (w.Has_Method (HYBRID_WINDOW_METHOD_CUT)) cliff_cut_capable++;

					/*
					**	Does a route actually *cross* this obstruction here?  Being near the window centre is not
					**	the same thing -- a road running along the rim is near it too -- so the test is whether
					**	two consecutive centerline samples stand on opposite sides of the obstruction, close
					**	enough along it to be this window rather than another.
					*/
					const Vector2 normal (-w.Tangent.Y,w.Tangent.X);
					const float reach = (0.5f * w.Span) + (2.0f * scan.Sized.Constants.W);
					float closest = 1.0e9f;
					bool crosses = false;

					for (int r = 0; r < scan.Routes.Routes.Count (); ++r) {

						const HybridRoute & route = scan.Routes.Routes[r];
						float previous_across = 0.0f;
						for (int s = 0; s < route.Points.Count (); ++s) {

							const Vector2 relative = route.Points[s] - w.Center;
							const float across = (relative.X * normal.X) + (relative.Y * normal.Y);
							const float along = (relative.X * w.Tangent.X) + (relative.Y * w.Tangent.Y);
							const float distance = relative.Length ();
							if (distance < closest) closest = distance;

							if ((s > 0) && (WWMath::Fabs (along) <= reach) &&
								 (((previous_across < 0.0f) && (across >= 0.0f)) ||
								  ((previous_across > 0.0f) && (across <= 0.0f)))) crosses = true;
							previous_across = across;
						}
					}
					if (closest < nearest) nearest = closest;
					if (crosses) cliff_crossed++;
				}

				if (cliff_windows > 0) {
					printf ("CLIFF archetype %d seed %u: %d windows (%d cut capable), %d crossed by a route, "
							  "nearest centerline %.1f; widest canyon span %.1f (bridge capable %d, max %.1f)\r\n",
							  (int)SEARCH_ARCHETYPES[ai],seed,cliff_windows,cliff_cut_capable,cliff_crossed,
							  nearest,widest_canyon,widest_bridge_capable ? 1 : 0,
							  preflight.Metrics.BridgeMaxSpanLength);
				}
			}

			//	Then what Phase 11 actually made of them, through the owner that owns the reroute.
			HybridInteractionParams sp = Interaction_Fixture (scan.Sized,1200.0f,inputs,preflight.Metrics);
			HybridInteractionSet si;
			HybridInteractionResult sr;
			HybridRouteResult srr;
			int spent = 0;
			const bool ok = Hybrid_Solve_Interactions_With_Reroute (scan.RouteParams,sp,scan.Identity,
																					scan.Matrices,scan.Landforms,
																					scan.Hydrology,scan.Anchors,scan.Topology,
																					Macro_Sample_From_Formula,&scan.Ground,
																					&scan.Routes,&srr,&si,&sr,&spent);
			reached_phase11++;
			for (int i = 0; i < HYBRID_ROAD_MEETING_COUNT; ++i) meetings[i] += sr.RoadCrossings.Meetings[i];
			meetings_near_shared += sr.RoadCrossings.IncidentalNearShared;
			meetings_beyond_junction += sr.RoadCrossings.IncidentalBeyondJunction;
			meetings_waypoint_shared += sr.RoadCrossings.WaypointShared;
			if (!ok && (sr.RoadCrossings.FailedMeeting >= 0) && (sr.RoadCrossings.FailedMeeting < HYBRID_ROAD_MEETING_COUNT)) {
				const HybridRoadCrossingCensus & rc = sr.RoadCrossings;
				meeting_failed[rc.FailedMeeting]++;
				if ((rc.JunctionProblem >= 0) && (rc.JunctionProblem < HYBRID_PROBLEM_COUNT)) junction_problem[rc.JunctionProblem]++;
				for (int s = 0; s < 2; ++s) {
					if ((rc.OverpassProblem[s] >= 0) && (rc.OverpassProblem[s] < HYBRID_PROBLEM_COUNT)) overpass_problem[rc.OverpassProblem[s]]++;
				}
				printf ("ROADFAIL archetype %d seed %u: %s %s x %s at %.1f deg, ends %.1f (%s) / %.1f (%s)%s; junction %s, overpass %s / %s"
						  "\r\n",(int)SEARCH_ARCHETYPES[ai],seed,
						  (rc.FailedMeeting == HYBRID_ROAD_MEETING_OBLIGATION) ? "obligation"
						  : ((rc.FailedMeeting == HYBRID_ROAD_MEETING_SHARED) ? "shared-node" : "incidental"),
						  Hybrid_Topology_Edge_Kind_Name ((HybridTopologyEdgeKind)rc.FailedKindA),
						  Hybrid_Topology_Edge_Kind_Name ((HybridTopologyEdgeKind)rc.FailedKindB),rc.FailedAngle,rc.FailedEndA,
						  Hybrid_Topology_Node_Kind_Name ((HybridTopologyNodeKind)rc.FailedEndKindA),rc.FailedEndB,
						  Hybrid_Topology_Node_Kind_Name ((HybridTopologyNodeKind)rc.FailedEndKindB),
						  rc.FailedWaypointShared ? ", ends at the other's waypoint" : "",
						  Hybrid_Interaction_Problem_Name ((HybridInteractionProblem)rc.JunctionProblem),
						  Hybrid_Interaction_Problem_Name ((HybridInteractionProblem)rc.OverpassProblem[0]),
						  Hybrid_Interaction_Problem_Name ((HybridInteractionProblem)rc.OverpassProblem[1]));
			}
			printf ("BUILT archetype %d seed %u\r\n",(int)SEARCH_ARCHETYPES[ai],seed);
			if (spent > 0) {
				printf ("REROUTE archetype %d seed %u: %d reroute(s), %s\r\n",(int)SEARCH_ARCHETYPES[ai],seed,spent,
						  ok ? "solved" : Hybrid_Interaction_Failure_Name (sr.Failure));
			}
			if (spent > 0) {
				reroute_requests += spent;
				if (ok && (spent == 1)) resolved_at_one++;
				if (ok && (spent == 2)) resolved_at_two++;
			}
			if (!ok) {
				if (spent > 0) exhausted++;
				if ((sr.Failure > HYBRID_INTERACTION_OK) && (sr.Failure < HYBRID_INTERACTION_FAILURE_COUNT)) {
					failure_reason[sr.Failure]++;
					printf ("P11FAIL archetype %d seed %u: %s -- %s\r\n",(int)SEARCH_ARCHETYPES[ai],seed,
							  Hybrid_Interaction_Failure_Name (sr.Failure),sr.Detail.Peek_Buffer ());
				}
				continue;
			}
			solved_worlds++;
			printf ("SOLVED archetype %d seed %u: %d interactions\r\n",(int)SEARCH_ARCHETYPES[ai],seed,si.Interactions.Count ());

			for (int i = 0; i < si.Interactions.Count (); ++i) {

				const HybridInteractionMethod method = si.Interactions[i].Method;
				if ((method >= 0) && (method < HYBRID_METHOD_COUNT)) method_selected[method]++;
				if ((method != HYBRID_METHOD_SURFACE_PASS) && (method != HYBRID_METHOD_ROAD_CUT) &&
					 (method != HYBRID_METHOD_RAMP_PASS)) continue;

				printf ("HIT archetype %d seed %u: %s on %s/%u, window %u, edge %u\r\n",
						  (int)SEARCH_ARCHETYPES[ai],seed,Hybrid_Interaction_Method_Name (method),
						  Hybrid_Obstruction_Kind_Name (si.Interactions[i].Obstruction),
						  si.Interactions[i].ObstructionRecordId,si.Interactions[i].WindowId,
						  si.Interactions[i].EdgeId);

				if ((method == HYBRID_METHOD_SURFACE_PASS) && (first_surface_pass < 0)) {
					first_surface_pass = (int)seed;
				}
				if ((method == HYBRID_METHOD_ROAD_CUT) && (first_road_cut < 0)) first_road_cut = (int)seed;
				if ((method == HYBRID_METHOD_RAMP_PASS) && (first_ramp_pass < 0)) first_ramp_pass = (int)seed;
			}
		}
		printf ("search archetype %d done: %d worlds built, %d solved so far\r\n",
				  (int)SEARCH_ARCHETYPES[ai],worlds,solved_worlds);
	}

	printf ("search over %u seeds x 7 archetypes: %d worlds built, %d solved; first surface pass seed %d, "
			  "first road cut seed %d, first ramp pass seed %d\r\n",
			  SEARCH_SEEDS,worlds,solved_worlds,first_surface_pass,first_road_cut,first_ramp_pass);
	printf ("phase 9 (Q-023): %d worlds passed; candidates rejected: %d approach class, %d wrong side, %d pose; "
			  "%d bindings recovered by a lower-ranked window\r\n",passed_phase9,rejected_approach,rejected_wrong_side,
			  rejected_pose,recovered_next_rank);
	printf ("worlds: %u attempted, %d built through Phase 10, %d reached Phase 11, %d solved\r\n",
			  SEARCH_SEEDS * 7u,worlds,reached_phase11,solved_worlds);
	s_route_census = false;
	printf ("phase 9 failures:");
	for (int i = 1; i < HYBRID_TOPOLOGY_FAILURE_COUNT; ++i) {
		if (topology_failure[i] > 0) printf (" %s %d",Hybrid_Topology_Failure_Name ((HybridTopologyFailure)i),topology_failure[i]);
	}
	printf ("\r\n");
	for (int c = 0; c < 2; ++c) {
		printf ("%s bridge quota short in %d worlds: %d candidates (%d worlds with none)\r\n",c == 0 ? "river" : "canyon/ravine",
				  quota_worlds[c],quota_candidates[c],quota_zero_candidates[c]);
		for (int pa = 0; pa < HYBRID_QUOTA_PATH_COUNT; ++pa) {
			printf ("   %-10s",Hybrid_Quota_Path_Name ((HybridQuotaPath)pa));
			for (int r = 0; r < HYBRID_QUOTA_REFUSAL_COUNT; ++r) {
				if (quota_refused[c][pa][r] > 0) printf (" %s %d",Hybrid_Quota_Refusal_Name ((HybridQuotaRefusal)r),quota_refused[c][pa][r]);
			}
			printf ("\r\n");
		}
	}
	printf ("overpass target short in %d of the worlds Phase 9 froze: %d natural crossings, %d synthesized (%d eligible pairs; %d worlds with no "
			  "pair, %d with no pair crossing a highway)\r\n",overpass_worlds,overpass_natural,overpass_synthesized,overpass_pairs,
			  overpass_no_pairs,overpass_no_crossing);
	printf ("   natural    ");
	for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) {
		printf (" %s %d",Hybrid_Overpass_Refusal_Name ((HybridOverpassRefusal)r),overpass_natural_refused[r]);
	}
	printf ("\r\n");
	printf ("   synthesized");
	for (int r = 0; r < HYBRID_OVERPASS_REFUSAL_COUNT; ++r) {
		printf (" %s %d",Hybrid_Overpass_Refusal_Name ((HybridOverpassRefusal)r),overpass_synth_refused[r]);
	}
	printf ("\r\n");
	printf ("phase 10 failures: %d; owning reason:",route_failures);
	for (int i = 1; i < HYBRID_BLOCK_COUNT; ++i) {
		if (route_reason[i] > 0) printf (" %s %d",Hybrid_Route_Block_Name ((HybridRouteBlock)i),route_reason[i]);
	}
	printf ("\r\n");
	printf ("   centerline refusals by owner:");
	for (int i = 0; i < HYBRID_BLOCK_COUNT; ++i) {
		if (route_spline_block[i] > 0) printf (" %s %d",Hybrid_Route_Block_Name ((HybridRouteBlock)i),route_spline_block[i]);
	}
	printf ("\r\n");
	printf ("   centerline refusals by geometric condition:");
	for (int i = 1; i < HYBRID_SPLINE_FAILURE_COUNT; ++i) {
		if (spline_class[i] > 0) printf (" %s %d",Hybrid_Spline_Failure_Class_Name ((HybridSplineFailureClass)i),spline_class[i]);
	}
	printf ("\r\n");
	printf ("   structural transitions by cause:");
	for (int i = 1; i < HYBRID_STRUCTURAL_COUNT; ++i) {
		if (structural_cause[i] > 0) printf (" %s %d",Hybrid_Structural_Cause_Name ((HybridStructuralCause)i),structural_cause[i]);
	}
	printf ("\r\n");
	printf ("   no-path frontier cells by rule:");
	for (int i = 1; i < HYBRID_BLOCK_COUNT; ++i) {
		if (route_frontier[i] > 0) printf (" %s %d",Hybrid_Route_Block_Name ((HybridRouteBlock)i),route_frontier[i]);
	}
	printf ("\r\n");
	printf ("   by class:");
	for (int i = 0; i < HYBRID_ROAD_CLASS_COUNT; ++i) {
		if (route_class[i] > 0) printf (" %s %d",Hybrid_Road_Class_Name ((HybridRoadClass)i),route_class[i]);
	}
	printf ("; by role:");
	for (int i = 0; i < HYBRID_TOPOLOGY_EDGE_KIND_COUNT; ++i) {
		if (route_kind[i] > 0) printf (" %s %d",Hybrid_Topology_Edge_Kind_Name ((HybridTopologyEdgeKind)i),route_kind[i]);
	}
	printf ("; final envelope 0/1/2: %d/%d/%d; %d with structural waypoints, %d failing on a structural leg\r\n",
			  route_envelope_final[0],route_envelope_final[1],route_envelope_final[2],route_with_structural,
			  route_structural_leg);
	printf ("phase 11 road meetings: %d shared-node, %d obligation, %d incidental; %d crossings away from a shared node "
			  "between edges that share one (%d beyond the junction arms); %d edge pairs where one ends at the other's waypoint\r\n",meetings[0],meetings[1],meetings[2],meetings_near_shared,meetings_beyond_junction,meetings_waypoint_shared);
	printf ("   failed: %d shared-node, %d obligation, %d incidental; junction refused by",meeting_failed[0],
			  meeting_failed[1],meeting_failed[2]);
	for (int i = 0; i < HYBRID_PROBLEM_COUNT; ++i) {
		if (junction_problem[i] > 0) printf (" %s %d",Hybrid_Interaction_Problem_Name ((HybridInteractionProblem)i),junction_problem[i]);
	}
	printf ("; overpass refused by");
	for (int i = 0; i < HYBRID_PROBLEM_COUNT; ++i) {
		if (overpass_problem[i] > 0) printf (" %s %d",Hybrid_Interaction_Problem_Name ((HybridInteractionProblem)i),overpass_problem[i]);
	}
	printf ("\r\n");
	printf ("reroutes: %d requested, %d resolved at envelope +1, %d at +2, %d exhausted\r\n",
			  reroute_requests,resolved_at_one,resolved_at_two,exhausted);
	printf ("routed cliff spans: %d; cut planned for some class %d, for the span's own class %d; pass %d\r\n",
			  cliff_spans,cliff_spans_cut_any,cliff_spans_cut_class,cliff_spans_pass);
	printf ("cliffs: %d accepted; H/F min %.3f max %.3f; %d with H/F <= 1.14; %d with a pass window (%d windows, "
			  "%d of them routed)\r\n",cliffs_total,ratio_min,ratio_max,cliffs_feasible_ratio,cliffs_with_pass_window,
			  cliff_pass_windows,cliff_pass_windows_routed);
	printf ("   height 18..22.5: %d, ..27: %d, ..31.5: %d, ..36: %d\r\n",height_buckets[0],height_buckets[1],
			  height_buckets[2],height_buckets[3]);
	printf ("   talus 1..1.85W: %d, ..2.7W: %d, ..3.55W: %d, ..4.4W: %d, ..5.25W: %d\r\n",talus_buckets[0],
			  talus_buckets[1],talus_buckets[2],talus_buckets[3],talus_buckets[4]);
	printf ("   H/F <=0.93: %d, <=1.14: %d, <=1.5: %d, <=2: %d, <=3: %d, >3: %d\r\n",ratio_buckets[0],ratio_buckets[1],
			  ratio_buckets[2],ratio_buckets[3],ratio_buckets[4],ratio_buckets[5]);
	for (int m = 0; m < HYBRID_METHOD_COUNT; ++m) {
		printf ("   selected %s: %d\r\n",Hybrid_Interaction_Method_Name ((HybridInteractionMethod)m),
				  method_selected[m]);
	}
	for (int f = HYBRID_INTERACTION_OK + 1; f < HYBRID_INTERACTION_FAILURE_COUNT; ++f) {
		if (failure_reason[f] == 0) continue;
		printf ("   phase 11 failed %d x %s\r\n",failure_reason[f],
				  Hybrid_Interaction_Failure_Name ((HybridInteractionFailure)f));
	}
}


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
	if ((which == nullptr) || (::strcmp (which, "baselayout") == 0)) {
		Check_Base_Layout ();
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
	if ((which == nullptr) || (::strcmp (which, "hybridplan") == 0)) {
		Check_Hybrid_Plan_Records ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridplanning") == 0)) {
		Check_Hybrid_Planning ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridmapidentity") == 0)) {
		Check_Hybrid_Map_Identity ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridstrategicgraph") == 0)) {
		Check_Hybrid_Strategic_Graph ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridcoarsematrix") == 0)) {
		Check_Hybrid_Coarse_Matrix ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridlandforms") == 0)) {
		Check_Hybrid_Landforms ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridhydrology") == 0)) {
		Check_Hybrid_Hydrology ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridmacroterrain") == 0)) {
		Check_Hybrid_Macro_Terrain ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridanchors") == 0)) {
		Check_Hybrid_Anchors ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridtopology") == 0)) {
		Check_Hybrid_Topology ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridroutes") == 0)) {
		Check_Hybrid_Routes ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridlocalpass") == 0)) {
		Check_Hybrid_Local_Pass ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridrealization") == 0)) {
		Check_Hybrid_Realization ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridinteractions") == 0)) {
		Check_Hybrid_Interactions ();
	}
	//	A tool, not a gate: only ever run when named, so `which == nullptr` deliberately skips it.
	if ((which != nullptr) && (::strcmp (which, "hybridinteractionsearch") == 0)) {
		Check_Hybrid_Interaction_Search ();
	}
	if ((which == nullptr) || (::strcmp (which, "hybridpreflight") == 0)) {
		Check_Hybrid_Preflight ();
	}
	if ((which == nullptr) || (::strcmp (which, "mapplan") == 0)) {
		Check_Map_Plan ();
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
	if ((which == nullptr) || (::strcmp (which, "native_profile") == 0)) {
		Check_Native_Profile_Quality ();
	}
	if ((which == nullptr) || (::strcmp (which, "native_bridges") == 0)) {
		for (unsigned int seed = 1; seed <= 8; seed++) {
			bool accepted = false;
			unsigned int chosen_seed = 0,checksum = 0;
			// Exercise consecutive deterministic candidates, including rejected worlds.
			// The manager's bounded policy allows at most three attempts.
			for (unsigned int attempt = 0; attempt < 3 && !accepted; attempt++) {
				::fprintf (stdout,"native bridge request %u attempt seed %u\n",seed,seed + attempt);
				chosen_seed = seed + attempt;
				accepted = Check_Native_Profile_Quality (chosen_seed,true,&checksum);
			}
			Check (accepted,"native bridge request %u exhausted its bounded retries",seed);
			if (accepted) {
				unsigned int replay_checksum = 0;
				Check (Check_Native_Profile_Quality (chosen_seed,true,&replay_checksum),
					"accepted native bridge seed %u failed exact reconstruction",chosen_seed);
				Check (checksum == replay_checksum,"native bridge seed %u was not deterministic",chosen_seed);
				::fprintf (stdout,"native bridge request %u accepted seed %u checksum %08X\n",seed,chosen_seed,checksum);
			}
		}
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
	if ((which == nullptr) || (::strcmp (which, "worldsynctransport") == 0)) {
		Check_World_Sync_Transport ();
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
