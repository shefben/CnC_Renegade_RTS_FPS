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
#include "tacticalintelligence.h"
#include "worldshroudmanager.h"


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
