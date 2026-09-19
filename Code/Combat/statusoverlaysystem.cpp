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
 *                     $Archive:: /Commando/Code/Combat/statusoverlaysystem.cpp               $*
 *                                                                                             *
 *	Roadmap Section 32, Zero Hour / SAGE feature 17 -- waypoint / status-circle renderers.       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "statusoverlaysystem.h"

#include "camera.h"
#include "combat.h"
#include "pscene.h"
#include "rendercomponent.h"
#include "rendobj.h"
#include "rinfo.h"
#include "soldier.h"
#include "timemgr.h"
#include "vector.h"
#include "worldshroudmanager.h"
#include "wwdebug.h"


/*
**	Which team a handle belongs to, parallel to StatusOverlayRenderer's own pool and indexed
**	the same way (handle - 1) -- growable rather than a second fixed-size pool, since this
**	table only has to be as large as the highest handle ever issued.  -1 (the default for a
**	newly grown slot) reads as "unowned", which is also the value Add_Overlay below passes
**	straight through when a caller does not want a ring gated by team at all.
*/
static DynamicVectorClass<int>	_OwnerTeam;

static void	Ensure_Owner_Slot(int index)
{
	while (_OwnerTeam.Count() <= index) {
		_OwnerTeam.Add(-1);
	}
}


/*
**	One persistent scene object, added the first time an overlay is created and never
**	removed -- exactly TracerSystemClass's own shape (Code/Combat/bullet.cpp), including why:
**	the scene it is added to is not guaranteed to exist yet at static-init time.
*/
class StatusOverlaySceneObjectClass : public RenderObjClass, public IRenderComponent
{
	public:
		StatusOverlaySceneObjectClass(void)		{ Set_Transform(Matrix3D(1)); }

		virtual RenderObjClass *	Clone(void) const override		{ WWASSERT(false); return nullptr; }

		virtual void	Render(RenderInfoClass & rinfo) override
		{
			StatusOverlayRenderer::Render(rinfo,rinfo.Camera.Get_Position());
		}

		//	Section 29's IRenderComponent -- same render, reached through the shared contract.
		virtual void	Render_Component(RenderInfoClass & rinfo) override
		{
			Render(rinfo);
		}

		virtual void	Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const override
		{
			Vector3 minimum,maximum;
			StatusOverlayRenderer::Get_Bounds(minimum,maximum);
			sphere.Init((minimum + maximum) * 0.5f,((maximum - minimum) * 0.5f).Length());
		}

		virtual void	Get_Obj_Space_Bounding_Box(AABoxClass & box) const override
		{
			Vector3 minimum,maximum;
			StatusOverlayRenderer::Get_Bounds(minimum,maximum);
			box.Init((minimum + maximum) * 0.5f,(maximum - minimum) * 0.5f);
		}
};

static StatusOverlaySceneObjectClass *	_TheStatusOverlaySystem = nullptr;


StatusOverlayHandleType StatusOverlaySystem::Add_Overlay(StatusOverlayKindType kind,const Vector3 & position,
																				float radius,const Vector4 & color,int team,float lifetime)
{
	if (_TheStatusOverlaySystem == nullptr) {
		StatusOverlayRenderer::Init();
		_TheStatusOverlaySystem = NEW_REF(StatusOverlaySceneObjectClass,());
		COMBAT_SCENE->Add_Render_Object(_TheStatusOverlaySystem);
	}

	StatusOverlayHandleType handle = StatusOverlayRenderer::Add_Overlay(kind,position,radius,color,lifetime);
	if (handle != 0) {
		Set_Overlay_Team(handle,team);
	}
	return handle;
}

void StatusOverlaySystem::Set_Overlay_Team(StatusOverlayHandleType handle,int team)
{
	if (handle <= 0) {
		return;
	}
	Ensure_Owner_Slot(handle - 1);
	_OwnerTeam[handle - 1] = team;
}

bool StatusOverlaySystem::Update_Overlay(StatusOverlayHandleType handle,const Vector3 & position,
														float radius,const Vector4 & color)
{
	return StatusOverlayRenderer::Update_Overlay(handle,position,radius,color);
}

bool StatusOverlaySystem::Remove_Overlay(StatusOverlayHandleType handle)
{
	return StatusOverlayRenderer::Remove_Overlay(handle);
}

void StatusOverlaySystem::Remove_All(void)
{
	StatusOverlayRenderer::Remove_All();
}

int StatusOverlaySystem::Get_Overlay_Team(StatusOverlayHandleType handle)
{
	if (handle <= 0 || handle > _OwnerTeam.Count()) {
		return -1;
	}
	return _OwnerTeam[handle - 1];
}


bool StatusOverlaySystem::Is_Overlay_Visible_To(StatusOverlayHandleType handle,int viewing_team)
{
	if (!StatusOverlayRenderer::Is_Overlay(handle)) {
		return false;
	}

	int owner_team = Get_Overlay_Team(handle);
	if (owner_team < 0 || owner_team == viewing_team) {
		return true;
	}

	Vector3 position = StatusOverlayRenderer::Get_Overlay_Position(handle);
	ShroudStateType state = WorldShroudManager::Get_Shroud_State(viewing_team,position);
	return (state == SHROUD_VISIBLE) || (state == SHROUD_RADAR_DETECTED);
}


void StatusOverlaySystem::Update(void)
{
	StatusOverlayRenderer::Age(TimeManager::Get_Frame_Seconds());

	if (COMBAT_STAR == nullptr) {
		return;
	}
	int viewing_team = COMBAT_STAR->Get_Player_Type();

	for (int i = 0; i < _OwnerTeam.Count(); i++) {
		StatusOverlayHandleType handle = i + 1;
		if (!StatusOverlayRenderer::Is_Overlay(handle)) {
			continue;
		}
		StatusOverlayRenderer::Set_Visible(handle,Is_Overlay_Visible_To(handle,viewing_team));
	}
}


void StatusOverlaySystem::Shutdown(void)
{
	if (_TheStatusOverlaySystem != nullptr) {
		_TheStatusOverlaySystem->Remove();
		REF_PTR_RELEASE(_TheStatusOverlaySystem);
	}
	StatusOverlayRenderer::Shutdown();
	_OwnerTeam.Delete_All();
}
