#ifndef TT_INCLUDE__DLGCONFIGVIDEOTAB_H
#define TT_INCLUDE__DLGCONFIGVIDEOTAB_H
#include "childdialog.h"
class DlgConfigVideoTab : public ChildDialogClass {
public:
	static ChildDialogClass *Do_Dialog() {DlgConfigVideoTab *dialog = new DlgConfigVideoTab; return dialog;}
protected:
	DlgConfigVideoTab();
	~DlgConfigVideoTab();
	bool b;
	void On_Init_Dialog();
	void On_Destroy();
	void On_SliderCtrl_Pos_Changed(SliderCtrlClass *slider_ctrl,int ctrl_id,int new_pos);
};
#endif