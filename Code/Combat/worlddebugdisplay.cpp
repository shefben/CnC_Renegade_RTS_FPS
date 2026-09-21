/*
**	Command & Conquer Renegade(tm)
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
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/worlddebugdisplay.cpp                 $*
 *                                                                                             *
 *	Roadmap Section 33, Zero Hour / SAGE feature 20 -- debug rendering tools.                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worlddebugdisplay.h"

#include "aabox.h"
#include "basegameobj.h"
#include "building.h"
#include "camera.h"
#include "combat.h"
#include "gameobjmanager.h"
#include "physicalgameobj.h"
#include "rendercomponent.h"
#include "rendobj.h"
#include "rinfo.h"
#include "scriptablegameobj.h"
#include "soldier.h"
#include "worldboundsservice.h"
#include "boundarycliffgenerator.h"
#include "tacticalintelligence.h"
#include "worldshroudmanager.h"
#include "worldterrainsystem.h"


class DebugDisplaySceneObjectClass : public RenderObjClass, public IRenderComponent
{
	public:
		DebugDisplaySceneObjectClass(void)		{ Set_Transform(Matrix3D(1)); }

		virtual RenderObjClass *	Clone(void) const override		{ WWASSERT(false); return nullptr; }

		virtual void	Render(RenderInfoClass & rinfo) override
		{
			DebugRenderSystem::Render(rinfo,rinfo.Camera.Get_Position());
		}

		virtual void	Render_Component(RenderInfoClass & rinfo) override
		{
			Render(rinfo);
		}

		virtual void	Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const override
		{
			sphere.Init(Vector3(0,0,0),100000.0f);		// debug geometry can be anywhere in the level
		}

		virtual void	Get_Obj_Space_Bounding_Box(AABoxClass & box) const override
		{
			box.Init(Vector3(0,0,0),Vector3(100000.0f,100000.0f,100000.0f));
		}
};

static DebugDisplaySceneObjectClass *	_TheWorldDebugDisplay = nullptr;


static void	Draw_Building_Footprints(void)
{
	static const Vector4	FOOTPRINT_COLOR (1.0f,1.0f,0.0f,0.8f);

	SLNode<BaseGameObj> * objnode;
	for (objnode = GameObjManager::Get_Game_Obj_List()->Head(); objnode; objnode = objnode->Next()) {

		BaseGameObj * base = objnode->Data();
		if (base == nullptr) continue;

		//	Same two-step path Section 31's tactical model uses: a BuildingGameObj is not a
		//	PhysicalGameObj, and As_BuildingGameObj is ambiguous straight off a BaseGameObj*
		//	because of the CombatPhysObserverClass side of its multiple inheritance.
		ScriptableGameObj * script_obj = base->As_ScriptableGameObj();
		BuildingGameObj * building = (script_obj != nullptr) ? script_obj->As_BuildingGameObj() : nullptr;
		if (building == nullptr) continue;

		DebugRenderSystem::Add_Box(building->Get_Bounds().Center - building->Get_Bounds().Extent,
											 building->Get_Bounds().Center + building->Get_Bounds().Extent,
											 FOOTPRINT_COLOR);
	}
}


//
//	Roadmap Section 33's DEBUG_LAYER_RADAR_CONTACTS, and the first renderer Section 31's
//	Build_Strategic_Contacts has had -- a debug one, not the Commander map (Phase 36+), but a
//	real consumer that proves the shroud-gated query returns real positions.  Colour is the
//	viewer's own team relative to each contact (green = own team, red = a visible enemy, white
//	= unowned -- a resource, an objective, a marker), not a second blip-colour table; size
//	marks CONTACT_BUILDING larger than everything else so a base reads at a glance.
//
static void	Draw_Radar_Contacts(void)
{
	if (COMBAT_STAR == nullptr) {
		return;
	}
	int viewing_team = COMBAT_STAR->Get_Player_Type();

	static const Vector4	OWN_TEAM_COLOR	 (0.2f,1.0f,0.2f,0.9f);
	static const Vector4	ENEMY_COLOR		 (1.0f,0.2f,0.2f,0.9f);
	static const Vector4	NEUTRAL_COLOR	 (1.0f,1.0f,1.0f,0.9f);

	TacticalContactListClass contacts;
	TacticalIntelligenceModel::Build_Strategic_Contacts(viewing_team,contacts);

	for (int i = 0; i < contacts.Count(); i++) {
		const TacticalContactClass & contact = contacts[i];

		const Vector4 * color = &NEUTRAL_COLOR;
		if (contact.Team == viewing_team) {
			color = &OWN_TEAM_COLOR;
		} else if (contact.Team != -1) {
			color = &ENEMY_COLOR;
		}

		float size = (contact.Type == CONTACT_BUILDING) ? 8.0f : 3.0f;
		DebugRenderSystem::Add_Cross(contact.Position,size,*color);
	}
}


//
//	Roadmap Section 33's DEBUG_LAYER_SHROUD_CELLS -- Section 30 names this overlay explicitly
//	as the reason WorldShroudManager carries Get_Grid_Geometry/Get_Cell_State_By_Index at all.
//	A flat quad outline per non-UNKNOWN cell of the local player's own team grid (four lines,
//	not Add_Box's twelve -- this is a 2D fog-of-war grid, not a volume), coloured by state.
//	Bounded the same way every other layer already is: DebugRenderSystem's own line pool drops
//	and counts anything past its budget rather than growing or asserting, so a very large
//	explored grid is a silently-lower-fidelity overlay, never a crash.
//
static void	Draw_Shroud_Cells(void)
{
	if (COMBAT_STAR == nullptr) {
		return;
	}
	int viewing_team = COMBAT_STAR->Get_Player_Type();

	float cell_size;
	int cols,rows;
	Vector3 world_min;
	if (!WorldShroudManager::Get_Grid_Geometry(&cell_size,&cols,&rows,&world_min)) {
		return;
	}

	static const Vector4	VISIBLE_COLOR		(0.2f,1.0f,0.2f,0.8f);
	static const Vector4	RADAR_COLOR			(1.0f,1.0f,0.2f,0.8f);
	static const Vector4	LAST_KNOWN_COLOR	(1.0f,0.6f,0.1f,0.8f);
	static const Vector4	EXPLORED_COLOR		(0.3f,0.3f,0.5f,0.5f);

	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++) {

			ShroudStateType state = WorldShroudManager::Get_Cell_State_By_Index(viewing_team,col,row);
			if (state == SHROUD_UNKNOWN) continue;

			const Vector4 * color = &EXPLORED_COLOR;
			if (state == SHROUD_VISIBLE) color = &VISIBLE_COLOR;
			else if (state == SHROUD_RADAR_DETECTED) color = &RADAR_COLOR;
			else if (state == SHROUD_LAST_KNOWN) color = &LAST_KNOWN_COLOR;

			float x0 = world_min.X + col * cell_size;
			float y0 = world_min.Y + row * cell_size;
			float x1 = x0 + cell_size;
			float y1 = y0 + cell_size;
			float z  = world_min.Z;

			Vector3 c0 (x0,y0,z), c1 (x1,y0,z), c2 (x1,y1,z), c3 (x0,y1,z);
			DebugRenderSystem::Add_Line (c0,c1,*color);
			DebugRenderSystem::Add_Line (c1,c2,*color);
			DebugRenderSystem::Add_Line (c2,c3,*color);
			DebugRenderSystem::Add_Line (c3,c0,*color);
		}
	}
}


/***********************************************************************************************
 * Draw_World_Bounds -- V2 roadmap Section 34 Phase 2 step 18 / Section 5.10                     *
 *                                                                                             *
 *	Every protected band as its own closed ring in its own colour, so the thing a boundary bug   *
 *	actually looks like -- a band that collapsed, a ring that pinches, geometry sitting in the   *
 *	cliff band -- is visible rather than inferred from a query answering no.                     *
 *                                                                                             *
 *	Drawn at the configured air ceiling rather than on the ground: the rings are a topological   *
 *	fact about the world, there is no terrain height to follow (this layer runs before any       *
 *	terrain exists in an empty test world), and the ceiling is itself one of the values worth    *
 *	seeing.  Nothing is drawn at all when no boundary is configured, which is every legacy map.  *
 *=============================================================================================*/
static void	Draw_World_Bounds(void)
{
	if (!WorldBoundsService::Is_Configured()) {
		return;
	}

	static const Vector4	HARD_BOUNDARY_COLOR	(1.0f,0.15f,0.15f,0.9f);	// the non-crossable line
	static const Vector4	CLIFF_BAND_COLOR		(1.0f,0.55f,0.1f,0.85f);	// no roads/buildings/tunnels/resources
	static const Vector4	APPROACH_COLOR			(0.9f,0.9f,0.2f,0.7f);		// terrain rises toward the perimeter
	static const Vector4	OUTER_COLOR				(0.6f,0.1f,0.6f,0.9f);		// the world edge itself

	const WorldBoundsZoneType	ZONES []	= { WORLD_ZONE_OUTSIDE_WORLD,WORLD_ZONE_HARD_BOUNDARY,
														 WORLD_ZONE_CLIFF_BAND,WORLD_ZONE_BOUNDARY_APPROACH };
	const Vector4 * const		COLORS []	= { &OUTER_COLOR,&HARD_BOUNDARY_COLOR,
														 &CLIFF_BAND_COLOR,&APPROACH_COLOR };

	float z = WorldBoundsService::Get_Air_Ceiling();

	for (int zone = 0; zone < 4; zone++) {

		DynamicVectorClass<Vector2> ring;
		if (!WorldBoundsService::Get_Zone_Ring (ZONES [zone],&ring)) {
			//	This band collapsed for this world -- worth seeing as an absence rather than
			//	papered over with an approximation.
			continue;
		}

		int count = ring.Count();
		for (int i = 0, j = count - 1; i < count; j = i++) {
			Vector3 a (ring [j].X,ring [j].Y,z);
			Vector3 b (ring [i].X,ring [i].Y,z);
			DebugRenderSystem::Add_Line (a,b,*COLORS [zone]);
		}
	}
}


/***********************************************************************************************
 * Draw_Boundary_Cliffs -- V2 roadmap Section 34 Phase 3 step 16 / Section 5.10                  *
 *                                                                                             *
 *	Three things, because three different faults look different:                                 *
 *                                                                                             *
 *	  the rim line   -- closed, so a gap in it is a gap in the ring;                             *
 *	  the toe line   -- closed likewise, and drawn separately because a ring whose rim closes    *
 *	                    while its toe does not is a real and otherwise invisible fault;          *
 *	  a riser per station, toe to rim, whose colour reports whether that station's face is tall  *
 *	    enough to be a wall: green for a wall, red for a face short enough to walk up.           *
 *                                                                                             *
 *	The riser colouring is the same question Check_Traversable_Ramps answers numerically.  Having *
 *	it on screen matters because a single short station in a ring of hundreds is a way out of     *
 *	the world, and a count does not say where it is.                                              *
 *=============================================================================================*/
static void	Draw_Boundary_Cliffs(void)
{
	if (!BoundaryCliffGenerator::Has_Ring()) {
		return;
	}

	static const Vector4	RIM_COLOR		(0.55f,0.45f,0.35f,0.9f);	// rock, lit
	static const Vector4	TOE_COLOR		(0.35f,0.28f,0.22f,0.8f);	// rock, in shadow
	static const Vector4	WALL_COLOR		(0.2f,0.9f,0.3f,0.75f);		// this station is a wall
	static const Vector4	RAMP_COLOR		(1.0f,0.1f,0.1f,1.0f);		// this station can be walked up

	/*
	**	What counts as climbable.  Nothing in this tree publishes a single locomotor climb limit
	**	reachable from here, so this is a debug-only threshold for colouring risers -- it decides
	**	nothing and gates nothing.  Two metres is taller than any step a soldier takes and far
	**	shorter than the shortest cliff a valid profile can ask for, so a red riser means a real
	**	generation fault rather than a borderline judgement.
	*/
	static const float DEBUG_CLIMBABLE_HEIGHT = 2.0f;

	int count = BoundaryCliffGenerator::Get_Station_Count();

	for (int i = 0, j = count - 1; i < count; j = i++) {

		Vector3 toe_a,rim_a,toe_b,rim_b;
		if (!BoundaryCliffGenerator::Get_Ring_Station (j,&toe_a,&rim_a)) continue;
		if (!BoundaryCliffGenerator::Get_Ring_Station (i,&toe_b,&rim_b)) continue;

		DebugRenderSystem::Add_Line (rim_a,rim_b,RIM_COLOR);
		DebugRenderSystem::Add_Line (toe_a,toe_b,TOE_COLOR);

		float face = rim_b.Z - toe_b.Z;
		DebugRenderSystem::Add_Line (toe_b,rim_b,
											  (face > DEBUG_CLIMBABLE_HEIGHT) ? WALL_COLOR : RAMP_COLOR);
	}
}


/***********************************************************************************************
 * Draw_Terrain_Chunks -- V2 roadmap Section 34 Phase 4 step 17                                  *
 *                                                                                             *
 *	One box per chunk, at the chunk's own top -- coloured by what the registry says rather than   *
 *	by anything read off the ground, so a chunk whose colour disagrees with its own visible        *
 *	terrain is the tagging/seam bug this layer exists to catch, not merely a repeat of what is      *
 *	already visible.  Cliff band is red regardless of slope -- Buildable is never true there, but   *
 *	giving it its own colour makes the immutable face's own footprint legible on its own, the        *
 *	same reason world_bounds draws the cliff band as its own ring rather than folding it into         *
 *	hard_boundary.                                                                                    *
 *=============================================================================================*/
static void	Draw_Terrain_Chunks(void)
{
	if (!WorldTerrainSystem::Has_Chunk_Registry()) {
		return;
	}

	static const Vector4	BUILDABLE_COLOR	(0.2f,0.9f,0.3f,0.6f);		// ordinary, buildable
	static const Vector4	STEEP_COLOR			(0.95f,0.85f,0.1f,0.6f);	// ordinary, too steep to build on
	static const Vector4	CLIFF_COLOR			(1.0f,0.15f,0.15f,0.7f);	// cliff band -- the immutable face

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info (px,py,&info)) continue;

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds (px,py,&bounds)) continue;

			const Vector4 & color = info.CliffBand ? CLIFF_COLOR : (info.Buildable ? BUILDABLE_COLOR : STEEP_COLOR);

			float x0 = bounds.Center.X - bounds.Extent.X;
			float x1 = bounds.Center.X + bounds.Extent.X;
			float y0 = bounds.Center.Y - bounds.Extent.Y;
			float y1 = bounds.Center.Y + bounds.Extent.Y;
			float z  = bounds.Center.Z + bounds.Extent.Z;		// the chunk's own top, so the box reads over the ground

			Vector3 a (x0,y0,z), b (x1,y0,z), c (x1,y1,z), d (x0,y1,z);
			DebugRenderSystem::Add_Line (a,b,color);
			DebugRenderSystem::Add_Line (b,c,color);
			DebugRenderSystem::Add_Line (c,d,color);
			DebugRenderSystem::Add_Line (d,a,color);
		}
	}
}


void WorldDebugDisplay::Update(void)
{
	bool anything_on = false;
	for (int i = 0; i < DEBUG_LAYER_COUNT; i++) {
		if (DebugRenderSystem::Is_Layer_Enabled((DebugLayerType)i)) {
			anything_on = true;
			break;
		}
	}
	if (!anything_on) {
		return;
	}

	if (_TheWorldDebugDisplay == nullptr) {
		DebugRenderSystem::Init();
		_TheWorldDebugDisplay = NEW_REF(DebugDisplaySceneObjectClass,());
		COMBAT_SCENE->Add_Render_Object(_TheWorldDebugDisplay);
	}

	DebugRenderSystem::Begin_Frame();

	if (DebugRenderSystem::Is_Layer_Enabled(DEBUG_LAYER_BUILDING_FOOTPRINTS)) {
		Draw_Building_Footprints();
	}

	if (DebugRenderSystem::Is_Layer_Enabled(DEBUG_LAYER_RADAR_CONTACTS)) {
		Draw_Radar_Contacts();
	}

	if (DebugRenderSystem::Is_Layer_Enabled(DEBUG_LAYER_SHROUD_CELLS)) {
		Draw_Shroud_Cells();
	}

	if (DebugRenderSystem::Is_Layer_Enabled(DEBUG_LAYER_BOUNDARY_CLIFFS)) {
		Draw_Boundary_Cliffs();
		anything_on = true;
	}

	if (DebugRenderSystem::Is_Layer_Enabled(DEBUG_LAYER_WORLD_BOUNDS)) {
		Draw_World_Bounds();
	}

	if (DebugRenderSystem::Is_Layer_Enabled(DEBUG_LAYER_TERRAIN_CHUNKS)) {
		Draw_Terrain_Chunks();
	}

	//	Every other layer is enumerated and toggleable (DebugRenderSystem::Is_Layer_Enabled
	//	answers all of them) and draws nothing yet -- see docs/zerohour/DebugRenderSystem.md
	//	for which systems this awaits.
}


void WorldDebugDisplay::Shutdown(void)
{
	if (_TheWorldDebugDisplay != nullptr) {
		_TheWorldDebugDisplay->Remove();
		REF_PTR_RELEASE(_TheWorldDebugDisplay);
	}
	DebugRenderSystem::Shutdown();
}
