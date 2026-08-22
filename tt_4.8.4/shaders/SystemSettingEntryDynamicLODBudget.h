#ifndef TT_INCLUDE_SYSTEMSETTINGENTRYDYNAMICLODBUDGET_H
#define TT_INCLUDE_SYSTEMSETTINGENTRYDYNAMICLODBUDGET_H
#include "SystemSettingEntrySlider.h"
class SystemSettingEntryDynamicLODBudget :
	public SystemSettingEntrySlider
{
	virtual const char* Get_Name() { return "Dynamic_LOD_Budget"; }
	virtual const char* Get_Help() { return "DYNAMIC_LOD_BUDGET <n> - sets the Dynamic LOD Budget."; }
	virtual int Get_Slider();
	virtual void Set_Slider(int _slider);
};

#endif
