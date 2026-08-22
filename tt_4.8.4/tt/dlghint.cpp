#include "General.h"
#include "dlghint.h"
#include "resource.h"
#include "TranslateDBClass.h"
void DlgHint::DoDialog(int titleID,int textID,int textID2,int textID3)
{
	const WCHAR *title = TranslateDBClass::Get_String(titleID);
	const WCHAR *text = TranslateDBClass::Get_String(textID);
	const WCHAR *text2 = TranslateDBClass::Get_String(textID2);
	const WCHAR *text3 = TranslateDBClass::Get_String(textID3);
	DlgHint *popup = new DlgHint();
	if (popup)
	{
		popup->Start_Dialog();
		popup->Set_Title(title);
		popup->Set_Dlg_Item_Text(IDC_MESSAGE, text);
		popup->Set_Dlg_Item_Text(IDC_MESSAGE2, text2);
		popup->Set_Dlg_Item_Text(IDC_MESSAGE3, text3);
		popup->Release_Ref();
	}
}

DlgHint::DlgHint() : PopupDialogClass(IDD_HINT)
{
}

DlgHint::~DlgHint()
{
}

void DlgHint::On_Command(int ctrl, int message, DWORD param)
{
	switch (ctrl)
	{
		case IDOK:
			{
				End_Dialog();
			}
			break;
		default:
			PopupDialogClass::On_Command(ctrl, message, param);
			break;
	}
}
