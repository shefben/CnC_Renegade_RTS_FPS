#include "General.h"
#include "SpawnerClass.h"

#include "MoveablePhysClass.h"
#include "Iterator.h"
#include "SoldierGameObj.h"
#include "Config.h"
#include "GameObjManager.h"


RENEGADE_FUNCTION
PhysicalGameObj* SpawnerClass::Spawn_Object(int)
AT2(0x006A1B30, 0x006A13D0);



RENEGADE_FUNCTION
bool SpawnerClass::Can_Spawn_Object(int) const
AT2(0x006A1B40, 0x006A13E0);



bool SpawnerClass::Determine_Spawn_TM(PhysicalGameObj* object)
{
	TT_ASSERT(object);

	if (config(CONFIG_USE_TT_SPAWNERS))
		return TT_Determine_Spawn_TM(object);
	else
		return Original_Determine_Spawn_TM(object);
}



bool SpawnerClass::TT_Determine_Spawn_TM(PhysicalGameObj* object)
{
	MoveablePhysClass* moveablePhys = object->Peek_Physical_Object() ? object->Peek_Physical_Object()->As_MoveablePhysClass() : NULL;

	int allowedTransformCount = 0;
	Matrix3D* allowedTransforms = new Matrix3D[alternateTransforms.Count() + 1];

	if (moveablePhys)
	{
		if (moveablePhys->Can_Teleport_And_Stand(transform, &allowedTransforms[allowedTransformCount]))
			++allowedTransformCount;

		for (Iterator<DynamicVectorClass<Matrix3D>> alternateTransform(alternateTransforms); alternateTransform; ++alternateTransform)
		{
			if (moveablePhys->Can_Teleport_And_Stand(*alternateTransform, &allowedTransforms[allowedTransformCount]))
				++allowedTransformCount;
		}
	}
	else
	{
		allowedTransforms[allowedTransformCount++] = transform;

		for (Iterator<DynamicVectorClass<Matrix3D>> alternateTransform(alternateTransforms); alternateTransform; ++alternateTransform)
			allowedTransforms[allowedTransformCount++] = *alternateTransform;
	}

	bool success = false;

	if (allowedTransformCount > 0)
	{
		spawnEffectTransform = allowedTransforms[rand() % allowedTransformCount];
		object->Set_Transform(spawnEffectTransform);
		success = true;
	}

	delete[] allowedTransforms;

	return success;
}



bool SpawnerClass::Original_Determine_Spawn_TM(PhysicalGameObj* object)
{
	TT_ASSERT(object);

	bool result = false;

	MoveablePhysClass* moveablePhys = object->Peek_Physical_Object() ? object->Peek_Physical_Object()->As_MoveablePhysClass() : NULL;

	if (alternateTransforms.Count() <= 0)
	{
		if (moveablePhys && object->As_ArmedGameObj())
		{
			Matrix3D _transform = transform;
			if (moveablePhys->Can_Teleport_And_Stand(transform, &_transform))
			{
				spawnEffectTransform = _transform;
				result = true;
			}
		}
		else
		{
			spawnEffectTransform = transform;
			result = true;
		}
	}
	else
	{
		Vector3 nearestSoldierPosition(0, 0, 0);
		float nearestSoldierDistance = 10000000;

		// Find soldier nearest to spawner (main location?)
		for (Iterator<SList<SmartGameObj>> object((SList<SmartGameObj>&)GameObjManager::SmartGameObjList); object; ++object)
		{
			SoldierGameObj* soldier = object->As_SoldierGameObj();
			if (soldier && soldier->Is_Human_Controlled())
			{
				Vector3 soldierPosition;
				soldier->Get_Position(&soldierPosition);

				float soldierDistance = (soldierPosition - transform.Get_Translation()).Length();

				if (soldierDistance < nearestSoldierDistance)
				{
					soldier->Get_Position(&nearestSoldierPosition);
					nearestSoldierDistance = soldierDistance;
				}
			}
		}

		float furthestSoldierDistance = 0.f;

		// Find alternate transform furthest from soldier nearest to main location... (wtf?)
		for (int alternateTransformIndex = 0; alternateTransformIndex < alternateTransforms.Count(); ++alternateTransformIndex)
		{
			Matrix3D alternateTransform = alternateTransforms[alternateTransformIndex];

			if (!moveablePhys || moveablePhys->Can_Teleport_And_Stand(alternateTransforms[alternateTransformIndex], &alternateTransform))
			{
				float soldierDistance = (alternateTransform.Get_Translation() - nearestSoldierPosition).Length();

				if (soldierDistance > furthestSoldierDistance)
				{
					furthestSoldierDistance = soldierDistance;
					spawnEffectTransform = alternateTransform;
					result = true;
				}
			}
		}
	}
	if (result)
		object->Set_Transform(spawnEffectTransform);
	return result;
}
