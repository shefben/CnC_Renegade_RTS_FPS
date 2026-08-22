#include "menudialog.h"

class ChatDlg : public MenuDialogClass {
public:
	ChatDlg();
	void Update_Chat_Window();
	void On_Init_Dialog();
	bool On_Key_Down(uint32 key_id, uint32 key_data);
	~ChatDlg();
	static void Do_Dialog() {ChatDlg *dialog = new ChatDlg; dialog->Start_Dialog (); REF_PTR_RELEASE (dialog); }
};
