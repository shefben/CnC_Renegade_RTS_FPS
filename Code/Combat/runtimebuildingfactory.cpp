/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/runtimebuildingfactory.cpp            $*
 *                                                                                             *
 *	Roadmap Section 44, Phase 38 / Q-016.  See runtimebuildingfactory.h.                         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "runtimebuildingfactory.h"

#include "basecontroller.h"
#include "building.h"
#include "buildingdescriptormanager.h"
#include "combat.h"
#include "combatchunkid.h"
#include "debug.h"
#include "definition.h"
#include "definitionclassids.h"
#include "definitionmgr.h"
#include "gameobjmanager.h"
#include "rendobj.h"
#include "teampowergridservice.h"


static int	_CreatedCount	= 0;
static int	_RefusedCount	= 0;
static int	_ActivatedCount	= 0;


/***********************************************************************************************
 * Create_Constructing_Building                                                                  *
 *=============================================================================================*/
BuildingGameObj * RuntimeBuildingFactory::Create_Constructing_Building(const char * descriptor_name,
																							  const Matrix3D & transform,
																							  int team)
{
	if ((descriptor_name == nullptr) || (descriptor_name [0] == 0)) {
		_RefusedCount++;
		return nullptr;
	}

	const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name);
	if (descriptor == nullptr) {
		WWDEBUG_SAY (("RuntimeBuildingFactory: no descriptor '%s'\n",descriptor_name));
		_RefusedCount++;
		return nullptr;
	}

	/*
	**	The preset is data, not a lookup table in here.  Validate_Descriptor already refuses a
	**	buildable descriptor that names none, so reaching this with an empty name means someone
	**	asked to construct something the data says is not constructible.
	*/
	if (descriptor->GameObjectPreset.Get_Length () == 0) {
		WWDEBUG_SAY (("RuntimeBuildingFactory: descriptor '%s' names no game_object_preset\n",
						  descriptor_name));
		_RefusedCount++;
		return nullptr;
	}

	DefinitionClass * definition =
		DefinitionMgrClass::Find_Named_Definition (descriptor->GameObjectPreset.Peek_Buffer (),true);
	if (definition == nullptr) {
		//
		//	A named preset that is not in the loaded definition data is the same class of gap as a
		//	missing model: the descriptor is asking for something this installation does not have.
		//	Refused loudly and by name, never substituted.
		//
		WWDEBUG_SAY (("RuntimeBuildingFactory: descriptor '%s' names preset '%s', which no loaded "
						  "definition provides; the placement produces no building\n",
						  descriptor_name,descriptor->GameObjectPreset.Peek_Buffer ()));
		_RefusedCount++;
		return nullptr;
	}

	/*
	**	Checked by superclass rather than by exact class id, because every building preset a
	**	descriptor could legitimately name -- PowerPlant, SoldierFactory, WarFactory, ComCenter,
	**	Refinery -- is its own class id under the same superclass.  This is the same test
	**	BaseGameObj::Load uses to assert a building definition really is one.
	*/
	if (SuperClassID_From_ClassID (definition->Get_Class_ID ()) != CLASSID_BUILDINGS) {
		WWDEBUG_SAY (("RuntimeBuildingFactory: preset '%s' is not a building definition\n",
						  descriptor->GameObjectPreset.Peek_Buffer ()));
		_RefusedCount++;
		return nullptr;
	}

	BuildingGameObj * building = (BuildingGameObj *)definition->Create ();
	if (building == nullptr) {
		_RefusedCount++;
		return nullptr;
	}

	//
	//	Ownership before structure: Init_Runtime_Building registers with the team's base and the
	//	team power grid, and both of those read the player type.
	//
	building->Set_Player_Type (team);

	if (!building->Init_Runtime_Building (descriptor_name,transform)) {
		//
		//	A building with no structure is worse than no building: it would occupy a footprint,
		//	accept damage and count towards the base with nothing behind it.  Deleted through the
		//	normal pending mechanism so its observers and network registration unwind properly.
		//
		building->Set_Delete_Pending ();
		_RefusedCount++;
		return nullptr;
	}

	/*
	**	Section 44: "real model hidden" until the build-up finishes.  The physics object stays in
	**	the scene while hidden, because the ground is occupied from the moment the Commander
	**	commits to the placement -- a second building may not be dropped on top of a site that is
	**	only half built.
	*/
	building->Set_Runtime_Model_Hidden (true);

	//
	//	Replicate it.  BuildingGameObj::Export_Creation carries the descriptor name and transform,
	//	so a client reconstructs the same runtime building from the same data the server used.
	//
	if (CombatManager::I_Am_Server ()) {
		building->Set_Object_Dirty_Bit (NetworkObjectClass::BIT_CREATION,true);
	}

	building->Start_Observers ();

	_CreatedCount++;
	return building;
}


/***********************************************************************************************
 * Activate_Completed_Building                                                                   *
 *=============================================================================================*/
bool RuntimeBuildingFactory::Activate_Completed_Building(int building_id)
{
	//
	//	GameObjManager has no by-id building lookup -- buildings live in their own list, not in
	//	the physical-object table Find_PhysicalGameObj searches -- so the list is walked.  It
	//	holds a handful of entries and this runs once per completed construction.
	//
	BuildingGameObj * building = nullptr;
	SList<BuildingGameObj> * list = GameObjManager::Get_Building_Game_Obj_List ();
	if (list != nullptr) {
		for (SLNode<BuildingGameObj> * node = list->Head (); node != nullptr; node = node->Next ()) {
			BuildingGameObj * candidate = node->Data ();
			if ((candidate != nullptr) && (candidate->Get_ID () == building_id)) {
				building = candidate;
				break;
			}
		}
	}

	if (building == nullptr) {
		return false;
	}

	if (!building->Is_Runtime_Constructed ()) {
		return false;		// a map-authored building was never hidden and has nothing to reveal
	}

	building->Set_Runtime_Model_Hidden (false);

	/*
	**	Are_Capabilities_Enabled asks BuildingConstructionService whether this id is still
	**	building, so nothing here has to flip a capability flag: the moment the service stops
	**	saying "under construction", every capability that consults it opens by itself.  What
	**	does have to happen is the one thing that is a decision rather than a query -- the team's
	**	power allocation is now different, so it is recomputed.
	*/
	if (CombatManager::I_Am_Server ()) {
		TeamPowerGridService::RecalculateTeam (building->Get_Player_Type ());
	}

	_ActivatedCount++;
	return true;
}


/***********************************************************************************************
 * Get_Construction_Bounds                                                                       *
 *=============================================================================================*/
bool RuntimeBuildingFactory::Get_Construction_Bounds(const BuildingGameObj * building,
																	  const char * descriptor_name,
																	  AABoxClass * out_bounds)
{
	if (out_bounds == nullptr) {
		return false;
	}

	/*
	**	The real model's bounds, in the building's own local space -- the block layout is
	**	generated in model space and then transformed, so a world-space box would put the blocks
	**	somewhere else entirely for any building that is rotated.
	*/
	if (building != nullptr) {
		RenderObjClass * model = building->Peek_Runtime_Model ();
		if (model != nullptr) {
			model->Get_Obj_Space_Bounding_Box (*out_bounds);
			if ((out_bounds->Extent.X > 0.0f) && (out_bounds->Extent.Y > 0.0f) &&
				 (out_bounds->Extent.Z > 0.0f)) {
				return true;
			}
		}
	}

	//
	//	No model -- missing art, which never blocks the mechanic.  The footprint the descriptor
	//	declares is the right area, and the height is derived from it because Section 39's schema
	//	has nowhere to put one.  The build-up still runs over the correct ground.
	//
	const BuildingModelDescriptor * descriptor =
		(descriptor_name != nullptr) ? BuildingDescriptorManager::Get_Descriptor (descriptor_name) : nullptr;
	if (descriptor == nullptr) {
		return false;
	}

	out_bounds->Extent = Vector3 (descriptor->Footprint.Width * 0.5f,
											descriptor->Footprint.Depth * 0.5f,
											descriptor->Footprint.Width * 0.35f);
	out_bounds->Center = Vector3 (0.0f,0.0f,out_bounds->Extent.Z);
	return true;
}


int	RuntimeBuildingFactory::Get_Created_Count(void)		{ return _CreatedCount; }
int	RuntimeBuildingFactory::Get_Refused_Count(void)		{ return _RefusedCount; }
int	RuntimeBuildingFactory::Get_Activated_Count(void)	{ return _ActivatedCount; }

void	RuntimeBuildingFactory::Reset_Counters(void)
{
	_CreatedCount	= 0;
	_RefusedCount	= 0;
	_ActivatedCount	= 0;
}
