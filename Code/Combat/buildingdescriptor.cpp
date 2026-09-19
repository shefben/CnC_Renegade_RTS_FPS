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


bool BuildingModelDescriptor::Resolve_Anchor_World_Transform(RenderObjClass * building_model,
																					const Matrix3D & building_world_transform,
																					const char * role_name,Matrix3D * out_transform) const
{
	if (building_model == nullptr || out_transform == nullptr) {
		return false;
	}

	const char * bone_name = Find_Anchor_Bone (role_name);
	if (bone_name == nullptr || bone_name [0] == '\0') {
		return false;			// this descriptor names no anchor for that role
	}

	//	Get_Bone_Index answers 0 both for "root" and "no such bone" -- the same convention
	//	VehicleGameObj::Aquire_Turret_Bones and CompositeAttachmentClass::Attach already read
	//	it by, and the same refusal: a role cannot resolve to a component's root.
	if (building_model->Get_Bone_Index (bone_name) == 0) {
		return false;
	}

	const Matrix3D & node_model_transform = building_model->Get_Bone_Transform (bone_name);
	*out_transform = building_world_transform * node_model_transform;
	return true;
}
