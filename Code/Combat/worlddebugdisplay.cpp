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
