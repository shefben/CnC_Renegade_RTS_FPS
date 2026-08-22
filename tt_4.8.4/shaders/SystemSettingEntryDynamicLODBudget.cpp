#include "General.h"
#include "SystemSettingEntryDynamicLODBudget.h"
#include "PhysicsSceneClass.h"

int SystemSettingEntryDynamicLODBudget::Get_Slider()
{
	int DynamicLODBudget = this->Get_Value();
	if (PhysicsSceneClass::Get_Instance())
	{
		int StaticLODBudget;
		PhysicsSceneClass::Get_Instance()->Get_Polygon_Budgets(&StaticLODBudget,&DynamicLODBudget);
	}
	return DynamicLODBudget;
}
void SystemSettingEntryDynamicLODBudget::Set_Slider(int _slider)
{
	if (PhysicsSceneClass::Get_Instance())
	{
		int DynamicLODBudget;
		int StaticLODBudget;
		PhysicsSceneClass::Get_Instance()->Get_Polygon_Budgets(&StaticLODBudget,&DynamicLODBudget);
		PhysicsSceneClass::Get_Instance()->Set_Polygon_Budgets(StaticLODBudget,_slider);
	}
}
