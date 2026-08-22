#include "General.h"
#include "viewerdlg.h"
#include "viewerctrl.h"
#include "resource.h"
#include "HTreeClass.h"

ModelViewerDlg::ModelViewerDlg(const char *newanim, const char *newmodel) : PopupDialogClass(IDD_MODELVIEW)
{
	animation = newanim;
	model = newmodel;
}
void ModelViewerDlg::On_Init_Dialog()
{
	Set_Title(L"Model Viewer");
	ViewerCtrlClass *v = Get_Dlg_Item(IDC_MODELVIEW)->As_ViewerCtrlClass();
	v->Set_Model(model);
	if (animation)
	{
		RenderObjClass *o = v->Peek_Model();
		const char *htreename = o->Get_HTree()->Get_Name();
		char anim[31];
		sprintf(anim,"%s.%s",htreename,animation);
		v->Set_Animation(anim);
	}
	PopupDialogClass::On_Init_Dialog();
}
void ModelViewerDlg::On_ViewerCtrl_Model_Loaded(ViewerCtrlClass *,int,RenderObjClass *)
{
	ViewerCtrlClass *v = Get_Dlg_Item(IDC_MODELVIEW)->As_ViewerCtrlClass();
	v->Set_Interface_Mode(ViewerCtrlClass::VIRTUAL_TRACKBALL,45);
}
