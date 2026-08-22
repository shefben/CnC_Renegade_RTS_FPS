#include "General.h"
#include "chatdlg.h"
#include "listctrl.h"
#include "resource.h"
#include "MessageWindowClass.h"
#include "tt.h"
#include "GameInitMgrClass.h"
ChatDlg::ChatDlg() : MenuDialogClass(IDD_CHATBOX)
{
}
REF_DEF2(MessageWindow,MessageWindowClass *,0x00855EDC,0x008550C4);
extern REF_DECL2(VehicleCount,int);
void ChatDlg::Update_Chat_Window()
{
	ListCtrlClass *chatbox = Get_Dlg_Item(IDC_CHATBOX)->As_ListCtrlClass();
	MessageWindowClass *messagewindow = MessageWindow;
	chatbox->Delete_All_Entries();
	int x = messagewindow->MessageHistory.Count();
	for (int i = 0;i < x;i++)
	{
		wchar_t *str = newwcs(messagewindow->MessageHistory[i]);
		while ((str[wcslen(str)-1] == 0x0A) || (str[wcslen(str)-1] == 0x0D))
		{
			str[wcslen(str)-1] = 0;
		}
		int entry = chatbox->Insert_Entry(i,str);
		if (entry != -1)
		{
			chatbox->Set_Entry_Color(entry,0,messagewindow->ColorHistory[i]);
		}
		delete[] str;
	}
	chatbox->Scroll_To_End();
}
void ChatDlg::On_Init_Dialog()
{
	Vector3 v;
	v.X = 1.0;
	v.Y = 1.0;
	v.Z = 1.0;
	Get_Dlg_Item(IDC_CHATBOX)->As_ListCtrlClass()->Add_Column(L"",1.0,v);
	Update_Chat_Window();
}
bool ChatDlg::On_Key_Down(uint32 key_id, uint32 key_data)
{
	if (key_id == VK_F5)
	{
		Update_Chat_Window();
	}
	MenuDialogClass::On_Key_Down(key_id,key_data);
	return true;
}
ChatDlg::~ChatDlg()
{
	GameInitMgrClass::Continue_Game();
}
