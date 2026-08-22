#include "popupdialog.h"
class ModelViewerDlg : public PopupDialogClass {
private:
	const char *animation;
	const char *model;
public:
	ModelViewerDlg(const char *newanim, const char *newmodel);
	void On_Init_Dialog();
	void On_ViewerCtrl_Model_Loaded(ViewerCtrlClass *,int,RenderObjClass *);
	static void Do_Dialog(const char *newanim, const char *newmodel) {ModelViewerDlg *dialog = new ModelViewerDlg(newanim,newmodel); dialog->Start_Dialog (); REF_PTR_RELEASE (dialog); }
};
