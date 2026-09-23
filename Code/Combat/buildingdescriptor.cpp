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
 *                     $Archive:: /Commando/Code/Combat/buildingdescriptor.cpp                $*
 *                                                                                             *
 *	Roadmap Section 39, Phase 33 -- building model semantic JSON + live building definitions.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "buildingdescriptor.h"

#include "rendobj.h"


BuildingModelDescriptor::BuildingModelDescriptor(void) :
	DefinitionName(""),
	ModelName(""),
	SchemaVersion(1)
{
}


float BuildingModelDescriptor::Get_Build_Time_Seconds(void) const
{
	if (Build.BuildTimeSeconds > 0.0f) {
		return Build.BuildTimeSeconds;
	}
	return ConstructionVisual.DurationSeconds;
}

/*
**	Phase 20's canonical role table.  One place both the JSON parser and the debug status
**	report read, so the two can never disagree about what a role is called.
*/
static const char * const _SubcomponentRoleNames [SUBCOMPONENT_ROLE_COUNT] =
{
	"power_generator_module",
	"communications_array",
	"radar_dish",
	"refinery_processor",
	"tiberium_storage",
	"factory_production_module",
	"factory_exit_mechanism",
	"repair_module",
	"defense_targeting_module",
	"defense_weapon_module",
};

const char * Subcomponent_Role_Name(BuildingSubcomponentRoleType role)
{
	if ((role < 0) || (role >= SUBCOMPONENT_ROLE_COUNT)) return nullptr;
	return _SubcomponentRoleNames [role];
}

bool Subcomponent_Role_From_Name(const char * name,BuildingSubcomponentRoleType * out_role)
{
	if ((name == nullptr) || (out_role == nullptr)) return false;
	for (int i = 0; i < SUBCOMPONENT_ROLE_COUNT; i++) {
		if (::stricmp (name,_SubcomponentRoleNames [i]) == 0) {
			*out_role = (BuildingSubcomponentRoleType)i;
			return true;
		}
	}
	return false;
}


float BuildingModelDescriptor::Get_Subcomponent_Function_Fraction(BuildingSubcomponentRoleType role,
																						  float building_health_fraction) const
{
	for (int i = 0; i < Subcomponents.Count (); i++) {
		if (Subcomponents [i].Role != role) continue;

		float health = building_health_fraction;
		if (health < 0.0f) health = 0.0f;
		if (health > 1.0f) health = 1.0f;

		float full		= Subcomponents [i].FullFunctionHealthFraction;
		float disabled	= Subcomponents [i].DisabledHealthFraction;

		//	Validate_Descriptor refuses full <= disabled at load time; this is a defensive floor
		//	for a descriptor built by hand (a self check, say) rather than parsed, so it degrades
		//	to a step function instead of dividing by zero.
		if (full <= disabled) {
			return (health > disabled) ? 1.0f : 0.0f;
		}

		float t = (health - disabled) / (full - disabled);
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		return t;
	}
	return -1.0f;		// not tracked -- the caller's cue to leave its value alone
}

bool BuildingModelDescriptor::Has_Subcomponent(BuildingSubcomponentRoleType role) const
{
	for (int i = 0; i < Subcomponents.Count (); i++) {
		if (Subcomponents [i].Role == role) return true;
	}
	return false;
}

const char * BuildingModelDescriptor::Find_Subcomponent_Anchor_Bone(BuildingSubcomponentRoleType role) const
{
	for (int i = 0; i < Subcomponents.Count (); i++) {
		if (Subcomponents [i].Role != role) continue;
		if (Subcomponents [i].AnchorBoneName.Get_Length () == 0) return nullptr;
		return Subcomponents [i].AnchorBoneName.Peek_Buffer ();
	}
	return nullptr;
}


const char * BuildingModelDescriptor::Find_Anchor_Bone(const char * role_name) const
{
	if (role_name == nullptr) return nullptr;
	for (int i = 0; i < Anchors.Count (); i++) {
		if (Anchors [i].Role.Compare_No_Case (role_name) == 0) {
			return Anchors [i].BoneName.Peek_Buffer ();
		}
	}
	return nullptr;
}


bool BuildingModelDescriptor::Resolve_Anchor_World_Transform(const DynamicVectorClass<RenderObjClass *> & part_models,
																					const Matrix3D & building_world_transform,
																					const char * role_name,Matrix3D * out_transform) const
{
	if (out_transform == nullptr) {
		return false;
	}

	const char * bone_name = Find_Anchor_Bone (role_name);
	if (bone_name == nullptr || bone_name [0] == '\0') {
		return false;			// this descriptor names no anchor for that role
	}

	for (int i = 0; i < part_models.Count (); i++) {
		RenderObjClass * building_model = part_models [i];
		if (building_model == nullptr) {
			continue;
		}

		//	Get_Bone_Index answers 0 both for "root" and "no such bone" -- the same convention
		//	VehicleGameObj::Aquire_Turret_Bones and CompositeAttachmentClass::Attach already read
		//	it by, and the same refusal: a role cannot resolve to a component's root.  Not a
		//	match on this part -- try the next one rather than failing outright, which is the
		//	whole point of taking a list instead of one model.
		if (building_model->Get_Bone_Index (bone_name) == 0) {
			continue;
		}

		const Matrix3D & node_model_transform = building_model->Get_Bone_Transform (bone_name);
		*out_transform = building_world_transform * node_model_transform;
		return true;
	}

	return false;
}


/***********************************************************************************************
 * BuildingDamageVisualDefinition -- which model a given health means                            *
 *                                                                                             *
 *	The table is built already sorted (see buildingdescriptormanager.cpp), so selection is a     *
 *	single forward scan: the first entry whose threshold the building still meets wins.  A       *
 *	linear scan rather than a binary search because the table is three or four entries long and  *
 *	is read once per state change, not once per frame.                                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int BuildingDamageVisualDefinition::Find_State_Index(float normalized_health) const
{
	if (States.Count () == 0) {
		return BUILDING_DAMAGE_STATE_DESTROYED;		// nothing to choose from; caller falls back
	}

	if (normalized_health < 0.0f) normalized_health = 0.0f;
	if (normalized_health > 1.0f) normalized_health = 1.0f;

	for (int i = 0; i < States.Count (); i++) {
		if (normalized_health >= States [i].MinHealthFraction) {
			return i;
		}
	}

	//	Unreachable while the table is valid (the last threshold is always 0.0), but a descriptor
	//	that somehow got here still answers with a model rather than with nothing.
	return States.Count () - 1;
}


const char * BuildingDamageVisualDefinition::Get_State_Model(int index) const
{
	if ((index < 0) || (index >= States.Count ())) {
		return nullptr;
	}
	return States [index].ModelName.Peek_Buffer ();
}


const char * BuildingModelDescriptor::Get_Damage_Model(float normalized_health,bool destroyed) const
{
	if (destroyed && (DamageVisual.DestroyedModel.Get_Length () > 0)) {
		return DamageVisual.DestroyedModel.Peek_Buffer ();
	}

	const char * model = DamageVisual.Get_State_Model (DamageVisual.Find_State_Index (normalized_health));
	if ((model != nullptr) && (model [0] != '\0')) {
		return model;
	}

	//	No table, or a table whose chosen entry names nothing: the descriptor's own model is the
	//	honest answer, and a building that never changes model is a missing-art case, not a fault.
	return ModelName.Peek_Buffer ();
}
