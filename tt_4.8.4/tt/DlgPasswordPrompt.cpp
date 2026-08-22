#include "General.h"
#include "DlgPasswordPrompt.h"
#include "Resource.h"
#include "EditCtrl.h"
DlgPasswordPrompt::DlgPasswordPrompt() : PopupDialogClass(IDD_MP_JOIN_PASSWORD)
{
}

DlgPasswordPrompt::~DlgPasswordPrompt()
{
}

const WCHAR *DlgPasswordPrompt::Get_Password()
{
	return Get_Dlg_Item_Text(IDC_PASSWORD_EDIT);
}

void DlgPasswordPrompt::On_Init_Dialog()
{
	Enable_Dlg_Item(IDC_JOIN_GAME_BUTTON,false);
	Get_Dlg_Item(IDC_PASSWORD_EDIT)->Set_Focus();
	PopupDialogClass::On_Init_Dialog();
}

void DlgPasswordPrompt::On_Command(int ctrl, int message, DWORD param)
{
	if (ctrl == IDC_JOIN_GAME_BUTTON)
	{
		if (Forward)
		{
			Forward->ReceiveSignal(*this);
			End_Dialog();
		}
	}
	PopupDialogClass::On_Command(ctrl,message,param);
}

void DlgPasswordPrompt::On_EditCtrl_Change(EditCtrlClass* edit_ctrl, int ctrl_id)
{
	if (ctrl_id == IDC_PASSWORD_EDIT)
	{
		const WCHAR * text = edit_ctrl->Get_Text();
		if ((text) && (wcslen(text) > 1))
		{
			Enable_Dlg_Item(IDC_JOIN_GAME_BUTTON,true);
		}
		else
		{
			Enable_Dlg_Item(IDC_JOIN_GAME_BUTTON,true);
		}
	}
}

void DlgPasswordPrompt::On_EditCtrl_Enter_Pressed (EditCtrlClass *edit_ctrl, int ctrl_id)
{
	if (ctrl_id == IDC_PASSWORD_EDIT)
	{
		if (Is_Dlg_Item_Enabled(IDC_JOIN_GAME_BUTTON))
		{
			On_Command(IDC_JOIN_GAME_BUTTON,0,0);
		}
	}
}

