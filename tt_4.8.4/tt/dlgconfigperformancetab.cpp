#include "General.h"
#include "dlgconfigperformancetab.h"
#include "resource.h"
#include "dialogcontrol.h"
#include "dialogmgr.h"
#include "sliderctrl.h"
#include "RegistryClass.h"
#include "_globals.h"
#include "DX8Caps.h"
#include "PhysicsSceneClass.h"
#include "WW3D.h"
#include "string_ids.h"
#include "TranslateDBClass.h"
#include "SurfaceEffectsManager.h"
extern REF_DECL1(CurrentCaps, DX8Caps*);
extern int lodbudgetval1;
extern int lodbudgetval2;
DlgConfigPerformanceTab::DlgConfigPerformanceTab() : ChildDialogClass(232)
{
}
DlgConfigPerformanceTab::~DlgConfigPerformanceTab()
{
}
void DlgConfigPerformanceTab::On_Command(int ctrl_id, int mesage_id, DWORD param)
{
	if (ctrl_id == IDC_EXPERT_CHECK)
	{
		bool checked = Is_Dlg_Button_Checked(ctrl_id);
		for (int i = 0;i < controls.Count();i++)
		{
			controls[i]->Show(checked);
		}
	}
}
void DlgConfigPerformanceTab::Init_Controls1()
{
	Get_Dlg_Item(IDC_PERFORMANCE_SLIDER)->As_SliderCtrlClass()->Set_Range(0,3);
	Get_Dlg_Item(IDC_GEOMETRY_DETAIL_SLIDER )->As_SliderCtrlClass()->Set_Range(0,2);
	Get_Dlg_Item(IDC_CHAR_SHADOWS_SLIDER)->As_SliderCtrlClass()->Set_Range(0,3);
	Get_Dlg_Item(IDC_TEXTURE_DETAIL_SLIDER)->As_SliderCtrlClass()->Set_Range(0,2);
	Get_Dlg_Item(IDC_SURFACE_DETAIL_SLIDER)->As_SliderCtrlClass()->Set_Range(0,2);
	Get_Dlg_Item(IDC_PARTICLE_DETAIL_SLIDER)->As_SliderCtrlClass()->Set_Range(0,2);
	const wchar_t *strings[3];
	strings[0] = TranslateDBClass::Get_String(IDS_MENU_VERTEX);
	strings[1] = TranslateDBClass::Get_String(IDS_MENU_MP_LIGHTMAPS);
	strings[2] = TranslateDBClass::Get_String(IDS_MENU_MT_LIGHTMAPS);
	Set_Dlg_Item_Text(IDC_LIGHTING_MODE,strings[WW3D::PrelitMode]);
	Enable_Dlg_Item(IDC_LIGHTING_MODE,false);
	Set_Dlg_Item_Text(IDC_TEXTURE_FILTER,WW3D::Get_Texture_Filter_Name());
	Enable_Dlg_Item(IDC_TEXTURE_FILTER,false);
	if (!CurrentCaps->SupportNPatches)
	{
		Check_Dlg_Button(IDC_NPATCH_CHECK,false);
		Enable_Dlg_Item(IDC_NPATCH_CHECK,false);
	}
}
void DlgConfigPerformanceTab::Init_Controls2()
{
	SliderCtrlClass *geometrydetail = Get_Dlg_Item(IDC_GEOMETRY_DETAIL_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *charshadows = Get_Dlg_Item(IDC_CHAR_SHADOWS_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *texturedetail = Get_Dlg_Item(IDC_TEXTURE_DETAIL_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *surfacedetail = Get_Dlg_Item(IDC_SURFACE_DETAIL_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *particledetail = Get_Dlg_Item(IDC_PARTICLE_DETAIL_SLIDER)->As_SliderCtrlClass();
	RegistryClass registry(Build_Registry_Location_String((char *)0x007F5274,0,"System Settings"),true);
	if (registry.bIsValid)
	{
		bool terrainshadow = registry.Get_Int("Static_Projectors",true);
		int shadows = registry.Get_Int("Shadow_Mode",2);
		int texture = registry.Get_Int("Texture_Resolution",0);
		int particle = registry.Get_Int("Particle_Detail",0);
		int npatches = registry.Get_Int("NPatches",0);
		int polygon0;
		int polygon1;
		PhysicsSceneClass::Get_Instance()->Get_Polygon_Budgets(&polygon0,&polygon1);
		int surface = SurfaceEffectsManager::Mode;
		if (shadows >= 3)
		{
			shadows = 3;
		}
		charshadows->Set_Pos(shadows,true);
		texturedetail->Set_Pos(((2 - texture <= 0) - 1) & (2 - texture),true);
		surfacedetail->Set_Pos(surface,true);
		particledetail->Set_Pos(particle,true);
		int geometry;
		if (polygon1 >= lodbudgetval1)
		{
			if (polygon1 >= lodbudgetval2)
			{
				geometry = 2;
			}
			else
			{
				geometry = 1;
			}
		}
		else
		{
			geometry = 0;
		}
		geometrydetail->Set_Pos(geometry,true);
		Check_Dlg_Button(IDC_TERRAIN_SHADOW_CHECK,terrainshadow != 0);
		Check_Dlg_Button(IDC_NPATCH_CHECK,npatches != 0);
	}
}
RENEGADE_FUNCTION
void DlgConfigPerformanceTab::Init_Controls3()
AT1(0x00487DC0);
void DlgConfigPerformanceTab::On_Init_Dialog()
{
	Init_Controls1();
	Init_Controls2();
	Init_Controls3();
	int val = controls.Length();
	controls.Clear();
	controls.Resize(val);
	float top = Get_Dlg_Item(IDC_EXPERT_CHECK)->Get_Window_Rect().Top;
	for (int i = 0;i < ControlList.Count();i++)
	{
		if (ControlList[i]->Get_Window_Rect().Top > top)
		{
			controls.Add(ControlList[i]);
		}
	}
	for (int i = 0;i < controls.Count();i++)
	{
		controls[i]->Show(false);
	}
}
bool DlgConfigPerformanceTab::On_Apply()
{
	SliderCtrlClass *geometrydetail = Get_Dlg_Item(IDC_GEOMETRY_DETAIL_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *charshadows = Get_Dlg_Item(IDC_CHAR_SHADOWS_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *texturedetail = Get_Dlg_Item(IDC_TEXTURE_DETAIL_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *surfacedetail = Get_Dlg_Item(IDC_SURFACE_DETAIL_SLIDER)->As_SliderCtrlClass();
	SliderCtrlClass *particledetail = Get_Dlg_Item(IDC_PARTICLE_DETAIL_SLIDER)->As_SliderCtrlClass();
	RegistryClass registry(Build_Registry_Location_String((char *)0x007F5274,0,"System Settings"),true);
	if (registry.bIsValid)
	{
		int geometry = geometrydetail->Get_Pos();
		PhysicsSceneClass::ShadowEnum shadows = (PhysicsSceneClass::ShadowEnum)charshadows->Get_Pos();
		int surface = surfacedetail->Get_Pos();
		int texture = texturedetail->Get_Pos();
		int particle = particledetail->Get_Pos();
		bool terrainshadow = Is_Dlg_Button_Checked(IDC_TERRAIN_SHADOW_CHECK);
		bool npatches = Is_Dlg_Button_Checked(IDC_NPATCH_CHECK);
		int lodbudget = 0;
		if (geometry)
		{
			if (geometry == 1)
			{
				lodbudget = lodbudgetval1;
			}
			else if (geometry == 2)
			{
				lodbudget = lodbudgetval2;
			}
		}
		registry.Set_Int("Dynamic_LOD_Budget",lodbudget);
		registry.Set_Int("Static_LOD_Budget",lodbudget);
		registry.Set_Int("Dynamic_Projectors",shadows != 0);
		registry.Set_Int("Static_Projectors",terrainshadow);
		registry.Set_Int("Shadow_Mode",shadows);
		registry.Set_Int("Texture_Resolution",(2 - texture) & ((2 - texture <= 0) - 1));
		registry.Set_Int("Particle_Detail",particle);
		if (CurrentCaps->SupportNPatches)
		{
			registry.Set_Int("NPatches",npatches);
		}
		PhysicsSceneClass::Get_Instance()->Set_Polygon_Budgets(lodbudget,lodbudget);
		PhysicsSceneClass::Get_Instance()->Enable_Dynamic_Projectors(shadows != 0);
		PhysicsSceneClass::Get_Instance()->Enable_Static_Projectors(terrainshadow != 0);
		if (PhysicsSceneClass::Get_Instance()->Get_Shadow_Mode() != shadows)
		{
			WW3D::_Invalidate_Textures();
			PhysicsSceneClass::Get_Instance()->Set_Shadow_Mode(shadows);
		}
		WW3D::Set_Texture_Reduction((2 - texture) & ((2 - texture <= 0) - 1));
		SurfaceEffectsManager::Mode = surface;
	}
	return true;
}
RENEGADE_FUNCTION
void DlgConfigPerformanceTab::On_SliderCtrl_Pos_Changed(SliderCtrlClass *slider_ctrl, int ctrl_id, int new_pos)
AT1(0x00488140);
