#ifndef TT_INCLUDE__DLGCONFIGPERFORMANCETAB_H
#define TT_INCLUDE__DLGCONFIGPERFORMANCETAB_H
#include "childdialog.h"
class DlgConfigPerformanceTab : public ChildDialogClass {
public:
	static ChildDialogClass *Do_Dialog() {DlgConfigPerformanceTab *dialog = new DlgConfigPerformanceTab; return dialog;}
protected:
	DynamicVectorClass<DialogControlClass *> controls;
	DlgConfigPerformanceTab();
	~DlgConfigPerformanceTab();
	void On_Command(int ctrl_id, int mesage_id, DWORD param);
	void On_Init_Dialog();
	void Init_Controls1();
	void Init_Controls2();
	void Init_Controls3();
	bool On_Apply();
	void On_SliderCtrl_Pos_Changed(SliderCtrlClass *slider_ctrl, int ctrl_id, int new_pos);
};
#endif