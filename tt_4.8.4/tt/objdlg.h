#include "menudialog.h"

class ObjectivesTextDlg : public MenuDialogClass {
	char *fname;
public:
	ObjectivesTextDlg(char *file);
	void On_Init_Dialog();
	~ObjectivesTextDlg();
	static void Do_Dialog(char *file) {ObjectivesTextDlg *dialog = new ObjectivesTextDlg(file); dialog->Start_Dialog (); REF_PTR_RELEASE (dialog); }
};
