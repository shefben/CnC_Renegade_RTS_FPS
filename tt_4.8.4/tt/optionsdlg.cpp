#include "General.h"
#include "optionsdlg.h"
#include "configdlg.h"
#include "resource.h"

void OptionsDlg::New_On_Command(int ctrl_id, int message_id, DWORD param)
{
	if (ctrl_id == IDC_OPTIONS)
	{
		ConfigDlg::Do_Dialog();
	}
	else
	{
		MenuDialogClass::On_Command(ctrl_id,message_id,param);
	}
}

void OptionsDlg::New_Start_Dialog()
{
	MenuDialogClass::Start_Dialog();
}

OptionsDlg::OptionsDlg() : MenuDialogClass(IDD_MENU_OPTIONS)
{
}
