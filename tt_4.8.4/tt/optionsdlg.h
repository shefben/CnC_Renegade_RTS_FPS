#include "menudialog.h"
class OptionsDlg : public MenuDialogClass {
public:
	OptionsDlg();
	void New_On_Command(int ctrl_id, int message_id, DWORD param);
	void New_Start_Dialog();
};
