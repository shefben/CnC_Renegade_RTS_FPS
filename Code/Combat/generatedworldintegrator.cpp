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
 *                     $Archive:: /Commando/Code/Combat/generatedworldintegrator.cpp          $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "generatedworldintegrator.h"

#include "assetmgr.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "crc.h"
#include "matrix3d.h"
#include "proceduralassetregistry.h"
#include "pscene.h"
#include "rendobj.h"
#include "settlementgenerator.h"
#include "staticphys.h"
#include "worldshroudmanager.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				GeneratedWorldIntegrator::Built			= false;
unsigned int	GeneratedWorldIntegrator::Checksum		= 0;

DynamicVectorClass<IntegratedStructureRecord>	GeneratedWorldIntegrator::Records;
DynamicVectorClass<StaticPhysClass *>				GeneratedWorldIntegrator::PhysObjects;


IntegratedStructureRecord::IntegratedStructureRecord(void) :
	StructureIndex(-1),
	HasCollision(false)
{
}


bool IntegratedStructureRecord::operator == (const IntegratedStructureRecord & that) const
{
	return (PersistenceId == that.PersistenceId) &&
			 (StructureIndex == that.StructureIndex) &&
			 (HasCollision == that.HasCollision);
}


void GeneratedWorldIntegrator::Init(void)
{
	Clear();
}


void GeneratedWorldIntegrator::Shutdown(void)
{
	Clear();
}


void GeneratedWorldIntegrator::Clear(void)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	for (int i = 0; i < PhysObjects.Count(); i++) {
		StaticPhysClass * phys = PhysObjects[i];
		if (phys == nullptr) {
			continue;
		}
		if (scene != nullptr) {
			scene->Remove_Object(phys);
		}
		phys->Release_Ref();
	}
	PhysObjects.Delete_All();
	Records.Delete_All();

	Built		= false;
	Checksum	= 0;
}


bool GeneratedWorldIntegrator::Build(StringClass * error_out)
{
	Clear();

	//	No settlements is a real, correct outcome (an empty world, or the Settlements feature
	//	disabled) -- the same "trivially passes with zero" answer SettlementGenerator's own
	//	Generate() already documents for the same case, not a failure.
	if (!SettlementGenerator::Has_Settlements()) {
		Built = true;
		return true;
	}

	if (!ProceduralAssetRegistry::Has_Registry()) {
		if (error_out != nullptr) {
			*error_out = "GeneratedWorldIntegrator::Build -- ProceduralAssetRegistry has no registry";
		}
		return false;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();

	int structure_count = SettlementGenerator::Get_Structure_Count();
	for (int i = 0; i < structure_count; i++) {

		PlacedStructureInstance instance;
		if (!SettlementGenerator::Get_Structure(i,&instance)) {
			continue;
		}

		ResolvedSettlement settlement;
		StringClass settlement_id("unknown");
		if (SettlementGenerator::Get_Settlement(instance.SettlementIndex,&settlement)) {
			settlement_id = settlement.Id;
		}

		IntegratedStructureRecord record;
		record.StructureIndex = i;
		record.PersistenceId.Format("%s:structure:%d",settlement_id.Peek_Buffer(),i);
		record.HasCollision = false;

		//	Resolve the real, offline-verified asset -> descriptor -> model.  Any failure along
		//	this chain (unknown asset id, unloaded descriptor, no model name, model file missing
		//	on a dedicated server) leaves HasCollision false -- the same honest skip
		//	BuildingGameObj::Build_Runtime_Components already uses -- never a fabricated box.
		ProceduralAssetDescriptor asset;
		if (ProceduralAssetRegistry::Get_Asset_By_Id(instance.AssetId,&asset) && (scene != nullptr)) {

			const BuildingModelDescriptor * descriptor =
				BuildingDescriptorManager::Get_Descriptor(asset.DescriptorName.Peek_Buffer());

			if ((descriptor != nullptr) && (descriptor->ModelName.Get_Length() > 0)) {

				RenderObjClass * model =
					WW3DAssetManager::Get_Instance()->Create_Render_Obj(descriptor->ModelName.Peek_Buffer());

				if (model != nullptr) {

					Matrix3D transform(1);
					transform.Rotate_Z(instance.RotationRadians);
					transform.Set_Translation(instance.Position);

					StaticPhysClass * phys = new StaticPhysClass;
					phys->Set_Model(model);
					phys->Set_Transform(transform);
					model->Release_Ref();

					scene->Add_Static_Object(phys);

					PhysObjects.Add(phys);
					record.HasCollision = true;

				} else {
					WWDEBUG_SAY(("GeneratedWorldIntegrator: no model '%s' for structure asset '%s'; "
									 "structure %s is real but will not draw or collide\n",
									 descriptor->ModelName.Peek_Buffer(),instance.AssetId.Peek_Buffer(),
									 record.PersistenceId.Peek_Buffer()));
				}
			}
		}

		Records.Add(record);
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Records.Count(); i++) {
			const IntegratedStructureRecord & r = Records[i];
			int q_index		= r.StructureIndex;
			int q_collide	= r.HasCollision ? 1 : 0;
			crc(r.PersistenceId.Peek_Buffer(),r.PersistenceId.Get_Length());
			crc(&q_index,sizeof(q_index));
			crc(&q_collide,sizeof(q_collide));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;
	return true;
}


bool GeneratedWorldIntegrator::Partition_Shroud_Over_World(StringClass * error_out)
{
	if (!WorldTerrainSystem::Has_Chunk_Registry()) {
		if (error_out != nullptr) {
			*error_out = "Partition_Shroud_Over_World -- no chunk registry to size the shroud grid from";
		}
		return false;
	}

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();

	Vector3 world_min ( 1000000.0f, 1000000.0f, 1000000.0f);
	Vector3 world_max (-1000000.0f,-1000000.0f,-1000000.0f);
	bool any = false;

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) {
				continue;
			}
			world_min.X = WWMath::Min(world_min.X,bounds.Center.X - bounds.Extent.X);
			world_min.Y = WWMath::Min(world_min.Y,bounds.Center.Y - bounds.Extent.Y);
			world_min.Z = WWMath::Min(world_min.Z,bounds.Center.Z - bounds.Extent.Z);
			world_max.X = WWMath::Max(world_max.X,bounds.Center.X + bounds.Extent.X);
			world_max.Y = WWMath::Max(world_max.Y,bounds.Center.Y + bounds.Extent.Y);
			world_max.Z = WWMath::Max(world_max.Z,bounds.Center.Z + bounds.Extent.Z);
			any = true;
		}
	}

	if (!any) {
		if (error_out != nullptr) {
			*error_out = "Partition_Shroud_Over_World -- the chunk registry yielded no real patch bounds";
		}
		return false;
	}

	WorldShroudManager::Re_Partition(world_min,world_max);

	if (!WorldShroudManager::Is_Partitioned()) {
		if (error_out != nullptr) {
			*error_out = "Partition_Shroud_Over_World -- WorldShroudManager refused the generated world's bounds";
		}
		return false;
	}

	return true;
}


bool GeneratedWorldIntegrator::Get_Record(int index,IntegratedStructureRecord * out)
{
	if ((index < 0) || (index >= Records.Count()) || (out == nullptr)) {
		return false;
	}
	*out = Records[index];
	return true;
}


int GeneratedWorldIntegrator::Get_Collision_Count(void)
{
	int count = 0;
	for (int i = 0; i < Records.Count(); i++) {
		if (Records[i].HasCollision) {
			count++;
		}
	}
	return count;
}


int GeneratedWorldIntegrator::Get_Missing_Model_Count(void)
{
	return Records.Count() - Get_Collision_Count();
}
