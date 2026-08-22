#ifndef TT_INCLUDE_SYSTEMSETTINGENTRYSTATICLODBUDGET_H
#define TT_INCLUDE_SYSTEMSETTINGENTRYSTATICLODBUDGET_H
#include "SystemSettingEntrySlider.h"
class SystemSettingEntryStaticLODBudget :
	public SystemSettingEntrySlider
{
	virtual const char* Get_Name() { return "Static_LOD_Budget"; }
	virtual const char* Get_Help() { return "STATIC_LOD_BUDGET <n> - sets the Static LOD Budget."; }
	virtual int Get_Slider();
	virtual void Set_Slider(int _slider);
};

#endif
