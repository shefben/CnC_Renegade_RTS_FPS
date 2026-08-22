#include "General.h"
#include "dlgconfigvideotab.h"
#include "resource.h"
#include "ww3d.h"
#include "sliderctrl.h"
REF_DEF1(Gamma, int, 0x007FB9C8);
REF_DEF1(Brightness, int, 0x0082F158);
REF_DEF1(Contrast, int, 0x007FB9CC);

DlgConfigVideoTab::DlgConfigVideoTab() : ChildDialogClass(IDD_CONFIG_VIDEO), b(true)
{
}
DlgConfigVideoTab::~DlgConfigVideoTab()
{
}
void DlgConfigVideoTab::On_Init_Dialog()
{
	WideStringClass widerenderdevice;
	widerenderdevice.Convert_From(WW3D::Get_Render_Device_Name(WW3D::Get_Render_Device()));
	bool windowed;
	int depth;
	int width;
	int height;
	WW3D::Get_Device_Resolution(width,height,depth,windowed);
	WideStringClass resolution;
	resolution.Format(L"%d x %d",width,height);
	WideStringClass bits;
	bits.Format(L"%d",depth);
	Set_Dlg_Item_Text(IDC_DISPLAY_DRIVER,widerenderdevice);
	Set_Dlg_Item_Text(IDC_RESOLUTION,resolution);
	Set_Dlg_Item_Text(IDC_BIT_DEPTH,bits);
	Enable_Dlg_Item(IDC_DISPLAY_DRIVER,false);
	Enable_Dlg_Item(IDC_RESOLUTION,false);
	Enable_Dlg_Item(IDC_BIT_DEPTH,false);
	b = false;
	((SliderCtrlClass *)Get_Dlg_Item(IDC_GAMMA_SLIDER))->Set_Range(60,210);
	((SliderCtrlClass *)Get_Dlg_Item(IDC_GAMMA_SLIDER))->Set_Pos(Gamma);
	((SliderCtrlClass *)Get_Dlg_Item(IDC_BRIGHTNESS_SLIDER))->Set_Range(-45,45);
	((SliderCtrlClass *)Get_Dlg_Item(IDC_BRIGHTNESS_SLIDER))->Set_Pos(Brightness);
	((SliderCtrlClass *)Get_Dlg_Item(IDC_CONTRAST_SLIDER))->Set_Range(50,200);
	b = true;
	((SliderCtrlClass *)Get_Dlg_Item(IDC_CONTRAST_SLIDER))->Set_Pos(Contrast);
	ChildDialogClass::On_Init_Dialog();
}
void DlgConfigVideoTab::On_Destroy()
{
	Gamma = ((SliderCtrlClass *)Get_Dlg_Item(IDC_GAMMA_SLIDER))->Get_Pos();
	Brightness = ((SliderCtrlClass *)Get_Dlg_Item(IDC_BRIGHTNESS_SLIDER))->Get_Pos();
	Contrast = ((SliderCtrlClass *)Get_Dlg_Item(IDC_CONTRAST_SLIDER))->Get_Pos();
}
void DlgConfigVideoTab::On_SliderCtrl_Pos_Changed(SliderCtrlClass *slider_ctrl,int ctrl_id,int new_pos)
{
	WideStringClass value;
	if (b)
	{
		int gamma = ((SliderCtrlClass *)Get_Dlg_Item(IDC_GAMMA_SLIDER))->Get_Pos();
		int brightness = ((SliderCtrlClass *)Get_Dlg_Item(IDC_BRIGHTNESS_SLIDER))->Get_Pos();
		int contrast = ((SliderCtrlClass *)Get_Dlg_Item(IDC_CONTRAST_SLIDER))->Get_Pos();
		DX8Wrapper::Set_Gamma(gamma * 0.0099999998f,brightness * 0.0099999998f,contrast * 0.0099999998f,true,false);
	}
	switch (ctrl_id)
	{
	case IDC_GAMMA_SLIDER:
		value.Format(L"%.2f",slider_ctrl->Get_Pos() * 0.0099999998);
		Set_Dlg_Item_Text(IDC_GAMMA_SETTING,value);
		break;
	case IDC_BRIGHTNESS_SLIDER:
		value.Format(L"%.2f",slider_ctrl->Get_Pos() * 0.0099999998);
		Set_Dlg_Item_Text(IDC_BRIGHTNESS_SETTING,value);
		break;
	case IDC_CONTRAST_SLIDER:
		value.Format(L"%.2f",slider_ctrl->Get_Pos() * 0.0099999998);
		Set_Dlg_Item_Text(IDC_CONTRAST_SETTING,value);
		break;
	}
}
