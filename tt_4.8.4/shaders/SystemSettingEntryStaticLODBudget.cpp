#include "General.h"
#include "SystemSettingEntryStaticLODBudget.h"
#include "PhysicsSceneClass.h"

int SystemSettingEntryStaticLODBudget::Get_Slider()
{
	int StaticLODBudget = this->Get_Value();
	if (PhysicsSceneClass::Get_Instance())
	{
		int DynamicLODBudget;
		PhysicsSceneClass::Get_Instance()->Get_Polygon_Budgets(&StaticLODBudget,&DynamicLODBudget);
	}
	return StaticLODBudget;
}
void SystemSettingEntryStaticLODBudget::Set_Slider(int _slider)
{
	if (PhysicsSceneClass::Get_Instance())
	{
		int DynamicLODBudget;
		int StaticLODBudget;
		PhysicsSceneClass::Get_Instance()->Get_Polygon_Budgets(&StaticLODBudget,&DynamicLODBudget);
		PhysicsSceneClass::Get_Instance()->Set_Polygon_Budgets(_slider,DynamicLODBudget);
	}
}
