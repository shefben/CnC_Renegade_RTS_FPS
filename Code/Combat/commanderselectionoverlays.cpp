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
#include "rendercomponent.h"

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

static const Vector4	RING_COLOR (0.35f,1.0f,0.35f,0.9f);

static bool	Ring_Placement(int object_id,Vector3 * out_position,float * out_radius);

/*
**	D-001 P26: one selected object's ring is Section 29's selection visual.  Set_Selected is the
**	whole lifecycle -- on creates the status ring under the object, off removes it.
*/
class SelectionRingClass : public ISelectionVisualComponent
{
public:
	SelectionRingClass(void) : ObjectId(0),Handle(-1),Selected(false) { }

	virtual void	Set_Selected(bool onoff) override
	{
		if (onoff == Selected) return;
		if (onoff) {
			Vector3 position;
			float radius = 0.0f;
			if (!Ring_Placement (ObjectId,&position,&radius)) return;
			Handle = StatusOverlaySystem::Add_Overlay (OVERLAY_SELECTION,position,radius,RING_COLOR,-1,0.0f);
		} else {
			StatusOverlaySystem::Remove_Overlay (Handle);
			Handle = -1;
		}
		Selected = onoff;
	}
	virtual bool	Is_Selected(void) const override	{ return Selected; }

	//	Follow the object; false once it is gone or the ring was lost with the overlay system.
	bool	Follow(void)
	{
		Vector3 position;
		float radius = 0.0f;
		return Selected && Ring_Placement (ObjectId,&position,&radius) &&
				 StatusOverlaySystem::Update_Overlay (Handle,position,radius,RING_COLOR);
	}

	int							ObjectId;
	StatusOverlayHandleType	Handle;
	bool							Selected;
};

static SelectionRingClass	_Rings [MAX_RINGS];
static int						_RingCount = 0;
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
	_Rings [index].Set_Selected (false);
	_Rings [index] = _Rings [_RingCount - 1];
	_RingCount--;
}


/*
**	D-001 P26: the ring manager is Section 29's client update component -- a per-frame tick with
**	no gameplay authority, reached from CombatManager's frame through the interface.
*/
class SelectionOverlayUpdateClass : public IClientUpdateComponent
{
public:
	virtual void	Update_Component(float /* seconds */) override	{ CommanderSelectionOverlays::Update (); }
};

static SelectionOverlayUpdateClass	_UpdateComponent;

IClientUpdateComponent & CommanderSelectionOverlays::Get_Update_Component(void)	{ return _UpdateComponent; }


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

		if (keep && _Rings [r].Follow ()) {
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

		//	Unowned (always visible to its own viewer) and permanent -- Set_Selected (false) removes it.
		SelectionRingClass ring;
		ring.ObjectId = wanted [w];
		ring.Set_Selected (true);
		if (!ring.Is_Selected ()) continue;
		_Rings [_RingCount++] = ring;
	}
}


void CommanderSelectionOverlays::Shutdown(void)
{
	while (_RingCount > 0) {
		Remove_Ring (_RingCount - 1);
	}
}


int CommanderSelectionOverlays::Get_Ring_Count(void)	{ return _RingCount; }
