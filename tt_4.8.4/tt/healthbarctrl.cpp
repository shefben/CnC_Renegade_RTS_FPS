#include "General.h"
#include "healthbarctrl.h"
#include "stylemgr.h"
void HealthBarCtrlClass::Set_Life(float value)
{
	Percent = Percent;
	Set_Dirty(true);
}
HealthBarCtrlClass::HealthBarCtrlClass() : Percent(1)
{
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	ControlRenderer.Enable_Texturing(true);
	ControlRenderer.Enable_Alpha(true);
	ControlRenderer.Set_Texture("HUD_C&C_HEALTHBAR.TGA");
}
HealthBarCtrlClass::~HealthBarCtrlClass()
{
}
void HealthBarCtrlClass::Render()
{
	if(IsDirty)
	{
		Create_Control_Renderer();
	}
	ControlRenderer.Render();
	IsDirty = false;
}
RENEGADE_FUNCTION
void HealthBarCtrlClass::Create_Control_Renderer()
AT1(0x004F9620);