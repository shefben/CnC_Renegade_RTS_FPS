#ifndef	TERRAINSELFCHECK_H
#define	TERRAINSELFCHECK_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

/*
**	The terrain service, checked from inside the binary that has to be right.
**
**	Roadmap Section 17 rests on one promise that is easy to state and easy to break: rendered
**	terrain and terrain collision derive from the same source data, and roads, water,
**	placement and anything generated later refer to the same canonical coordinates.  What
**	makes that true in practice is that a point sampled from the heightfield and the same
**	point hit by a ray against the heightfield's triangles agree -- if they ever disagree, a
**	soldier stands off the ground he is drawn on and every system above this one inherits it.
**
**	None of that needs a device, a level or a physics scene.  It is a grid of numbers, a
**	triangulation and some arithmetic, so the checks run before anything is initialised, the
**	same arrangement as the shader and asset checks next door.
*/

namespace	TerrainSelfCheck
{
	//
	//	The names `main` answers to:
	//
	//		sampling    heights, normals, slopes, and the field's edges
	//		rays        casts against the triangulation, and that they agree with sampling
	//		shaping     height regions, road grades, river cuts, and patch invalidation
	//		service     the service with and without terrain under it
	//		collision   the geometry handed to the physics scene, and that it agrees with sampling
	//		textures    masks, layer rules, and the material passes they fill
	//		roads       spline geometry and the runtime road system built from it
	//		bridges     sectional spans, damage, breaks, and their collision
	//		foliage     the batched tree/plant buffer and its submission ratio
	//		water       carved volumes, shoreline blending, and the water surface type tag
	//		ribbons     vehicle track marks and their bounded pools
	//		marks       batched world surface marks and their bounded pools
	//		shadows     the cached shadow slot table and its casters
	//		lights      the dynamic light grid, its bounds, and its query cost
	//		far         the far/background terrain layer and its near-field boundary
	//		shroud      the per-team strategic shroud grid and its sensor-source API
	//		macro       the procedural macro terrain generator and its chunk registry
	//		interior    the procedural interior cliff generator and its bypass candidates
	//		hydrology   the procedural hydrology generator and its crossing candidates
	//		strategic   the procedural strategic planner and its candidate node graph
	//		roadgraph   the procedural road graph generator and its realized road edges
	//		bridgeplan  the procedural bridge planner and its resolved crossings
	//
	//	Returns a process exit code: zero when everything asked for is true.
	//
	int	Run (const char *which);
}

#endif	// TERRAINSELFCHECK_H
