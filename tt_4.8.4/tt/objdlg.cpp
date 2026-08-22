#include "General.h"
#include "objdlg.h"
#include "listctrl.h"
#include "resource.h"
#include "GameInitMgrClass.h"
ObjectivesTextDlg::ObjectivesTextDlg(char *file) : MenuDialogClass(IDD_CHATBOX)
{
	fname = file;
}
void ObjectivesTextDlg::On_Init_Dialog()
{
	Vector3 v;
	v.X = 1.0;
	v.Y = 1.0;
	v.Z = 1.0;
	Get_Dlg_Item(IDC_CHATBOX)->As_ListCtrlClass()->Add_Column(L"",1.0,v);
	ListCtrlClass *chatbox = Get_Dlg_Item(IDC_CHATBOX)->As_ListCtrlClass();
	FileClass *f = Get_Data_File(fname);
	int i = 0;
	if ((f) && (f->Open(1)))
	{
		int fileData_size = f->Size();
		char *fileData = new char[fileData_size+1];
		f->Read(fileData,fileData_size);
		fileData[fileData_size] = 0;
		fileData[fileData_size+1] = 0;
		f->Close();
		Close_Data_File(f);
		char *c = strtok(fileData,"\n");
		while (c)
		{
			if (c[strlen(c)-1] == '\r')
			{
				c[strlen(c)-1] = 0;
			}
			chatbox->Insert_Entry(i,CharToWideChar(c));
			c = strtok(NULL,"\n");
			i++;
		}
	}
}
ObjectivesTextDlg::~ObjectivesTextDlg()
{
	GameInitMgrClass::Continue_Game();
}
