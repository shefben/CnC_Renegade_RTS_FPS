#ifndef TT_INCLUDE_DLGPASSWORDPROMPT_H
#define TT_INCLUDE_DLGPASSWORDPROMPT_H
#include "PopupDialog.h"
#include "GameData.h"

class DlgPasswordPrompt;
class DlgPasswordPrompt : public PopupDialogClass, public Signaler<DlgPasswordPrompt> {
public:
	DlgPasswordPrompt();
	~DlgPasswordPrompt();
	const WCHAR *Get_Password();
	void On_Init_Dialog();
	void On_Command(int ctrl, int message, DWORD param);
	void On_EditCtrl_Change(EditCtrlClass* edit_ctrl, int ctrl_id);
	void On_EditCtrl_Enter_Pressed (EditCtrlClass *edit_ctrl, int ctrl_id);
	static void Do_Dialog(Signaler<DlgPasswordPrompt> *sig)
	{
		DlgPasswordPrompt *dialog = new DlgPasswordPrompt;
		dialog->Start_Dialog();
		if (sig)
		{
			if (dialog != sig)
			{
				dialog->SignalDropped(*dialog);
				dialog->Forward = sig;
				sig->Forward = dialog;
			}
		}
		REF_PTR_RELEASE(dialog);
	}
};

#endif