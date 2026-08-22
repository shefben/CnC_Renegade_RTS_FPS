#include "General.h"
#include "dlgimghint.h"
#include "resource.h"
#include "TranslateDBClass.h"
#include "ImageCtrl.h"
void DlgImgHint::DoDialog(int titleID,int textID,int textID2,int textID3,char *texture)
{
	const WCHAR *title = TranslateDBClass::Get_String(titleID);
	const WCHAR *text = TranslateDBClass::Get_String(textID);
	const WCHAR *text2 = TranslateDBClass::Get_String(textID2);
	const WCHAR *text3 = TranslateDBClass::Get_String(textID3);
	DlgImgHint *popup = new DlgImgHint();
	if (popup)
	{
		popup->Start_Dialog();
		popup->Set_Title(title);
		popup->Set_Dlg_Item_Text(IDC_MESSAGE, text);
		popup->Set_Dlg_Item_Text(IDC_MESSAGE2, text2);
		popup->Set_Dlg_Item_Text(IDC_MESSAGE3, text3);
		((ImageCtrlClass *)(popup->Get_Dlg_Item(IDC_IMAGE)))->Set_Texture(texture);
		popup->Release_Ref();
	}
}

DlgImgHint::DlgImgHint() : PopupDialogClass(IDD_HINT_IMAGE)
{
}

DlgImgHint::~DlgImgHint()
{
}

void DlgImgHint::On_Command(int ctrl, int message, DWORD param)
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
