/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderselectionoverlays.cpp        $*
 *                                                                                             *
 *	Roadmap Section 32.  See commanderselectionoverlays.h.                                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderselectionoverlays.h"

#include "building.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "commandermodecontroller.h"
#include "commanderselection.h"
#include "gameobjmanager.h"
#include "phys.h"
#include "physicalgameobj.h"
#include "rendobj.h"
#include "statusoverlaysystem.h"
#include "wwmath.h"


enum { MAX_RINGS = CommanderSelection::MAX_SELECTED + 1 };

struct SelectionRing
{
	int							ObjectId;
	StatusOverlayHandleType	Handle;
};

static SelectionRing	_Rings [MAX_RINGS];
static int				_RingCount = 0;

static const Vector4	RING_COLOR (0.35f,1.0f,0.35f,0.9f);
static const float	UNIT_RING_MIN_RADIUS			= 1.0f;
static const float	BUILDING_RING_FALLBACK		= 10.0f;


//	Where a ring goes and how wide it is, for a live unit or building; false when the object is gone.
static bool	Ring_Placement(int object_id,Vector3 * out_position,float * out_radius)
{
	ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (object_id);
	if (scriptable == nullptr) return false;

	BuildingGameObj * building = scriptable->As_BuildingGameObj ();
	if (building != nullptr) {
		building->Get_Position (out_position);

		//	The footprint's circumscribed circle when the building has a descriptor, since that
		//	is the ground it occupies; a map-authored building without one gets a fixed ring.
		*out_radius = BUILDING_RING_FALLBACK;
		const StringClass & name = building->Get_Definition ().Get_Descriptor_Name ();
		const BuildingModelDescriptor * descriptor = (name.Get_Length () > 0)
			? BuildingDescriptorManager::Get_Descriptor (name.Peek_Buffer ())
			: nullptr;
		if (descriptor != nullptr) {
			*out_radius = 0.5f * WWMath::Sqrt ((descriptor->Footprint.Width * descriptor->Footprint.Width) +
														  (descriptor->Footprint.Depth * descriptor->Footprint.Depth));
		}
		return true;
	}

	PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (object_id);
	if (unit == nullptr) return false;

	unit->Get_Position (out_position);
	*out_radius = UNIT_RING_MIN_RADIUS;

	PhysClass * phys = unit->Peek_Physical_Object ();
	RenderObjClass * model = (phys != nullptr) ? phys->Peek_Model () : nullptr;
	if (model != nullptr) {
		float radius = model->Get_Bounding_Sphere ().Radius;
		if (radius > *out_radius) *out_radius = radius;
	}
	return true;
}


static void	Remove_Ring(int index)
{
	StatusOverlaySystem::Remove_Overlay (_Rings [index].Handle);
	_Rings [index] = _Rings [_RingCount - 1];
	_RingCount--;
}


void CommanderSelectionOverlays::Update(void)
{
	int wanted [MAX_RINGS];
	int wanted_count = 0;

	if (CommanderModeController::Is_Active ()) {
		for (int i = 0; i < CommanderSelection::Get_Count (); i++) {
			int id = CommanderSelection::Get_Unit_Id (i);
			if (id != 0) wanted [wanted_count++] = id;
		}
		int building = CommanderSelection::Get_Selected_Building ();
		if ((building != 0) && (wanted_count < MAX_RINGS)) {
			wanted [wanted_count++] = building;
		}
	}

	//	Drop rings whose object left the selection or no longer exists.
	for (int r = _RingCount - 1; r >= 0; r--) {
		bool keep = false;
		for (int w = 0; w < wanted_count; w++) {
			if (wanted [w] == _Rings [r].ObjectId) { keep = true; break; }
		}

		Vector3 position;
		float radius = 0.0f;
		if (keep && Ring_Placement (_Rings [r].ObjectId,&position,&radius) &&
			 StatusOverlaySystem::Update_Overlay (_Rings [r].Handle,position,radius,RING_COLOR)) {
			continue;
		}
		Remove_Ring (r);
	}

	//	Add rings for newly selected objects.
	for (int w = 0; w < wanted_count; w++) {
		bool present = false;
		for (int r = 0; r < _RingCount; r++) {
			if (_Rings [r].ObjectId == wanted [w]) { present = true; break; }
		}
		if (present || (_RingCount >= MAX_RINGS)) continue;

		Vector3 position;
		float radius = 0.0f;
		if (!Ring_Placement (wanted [w],&position,&radius)) continue;

		//	Owned by the selection's own team, so it is never someone else's secret; a lifetime of
		//	zero is permanent -- this file removes it.
		_Rings [_RingCount].ObjectId	= wanted [w];
		_Rings [_RingCount].Handle		= StatusOverlaySystem::Add_Overlay (OVERLAY_SELECTION,position,radius,RING_COLOR,-1,0.0f);
		_RingCount++;
	}
}


void CommanderSelectionOverlays::Shutdown(void)
{
	while (_RingCount > 0) {
		Remove_Ring (_RingCount - 1);
	}
}


int CommanderSelectionOverlays::Get_Ring_Count(void)	{ return _RingCount; }
