#include "General.h"
#include "configdlg.h"
#include "resource.h"
#include "sliderctrl.h"
#include "comboboxctrl.h"
#include "listctrl.h"
#include "tt.h"
#include "shaders.h"
#include "inputctrl.h"
#include "DX8Wrapper.h"
#include "_globals.h"
#include "input.h"
int Get_Key_ID()
{
	int vkey;
	_asm {
		mov vkey, eax
	}
	switch (vkey)
	{
	case VK_LBUTTON:
		return 0x100;
	case VK_RBUTTON:
		return 0x101;
	case VK_MBUTTON:
		return 0x102;
	case VK_XBUTTON1:
		return 0x103;
	case VK_XBUTTON2:
		return 0x104;
	case 0:
		return 0;
	case 0x100:
		return 0x7004;
	case 0x101:
		return 0x7005;
	default:
		return *((unsigned int *)0x0085E104);
	}
}

struct Keymap {
	int VirtualKey;
	int Key;
};

struct Key {
	char *LogicalKeyName;
	int KeyNumber;
	float KeyValue;
};
extern SimpleDynVecClass<Key *> KeyList;
unsigned int GetMaxMultisampleLevel()
{
	DWORD aaLevels = 0;
	HRESULT res = DX8Wrapper::D3DInterface->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,_PresentParameters.BackBufferFormat,_PresentParameters.Windowed,D3DMULTISAMPLE_NONMASKABLE,&aaLevels);
	if (SUCCEEDED(res))
	{
		return aaLevels;
	}
	return 0;
}

extern void __declspec(dllimport) ShaderDetailChanged(int detail);
extern void __declspec(dllimport) SceneShaderDetailChanged(int detail);

bool HintsEnabled = true;
void ConfigDlg::End_Dialog()
{
	currentkey = 0;
	MenuDialogClass::End_Dialog();
}
ConfigDlg::ConfigDlg() : MenuDialogClass(IDD_EXTOPTIONS)
{
	currentkey = -1;
}
void ConfigDlg::On_Init_Dialog()
{
	Check_Dlg_Button(IDC_CHATLOG,ClientChatLog);
	Check_Dlg_Button(IDC_HINTS,HintsEnabled);
	Check_Dlg_Button(IDC_SHADOWS,Get_Registry_Int("HighQualityShadows",1));
	((SliderCtrlClass *)Get_Dlg_Item(IDC_SHADERS))->Set_Range(0,4);
	((SliderCtrlClass *)Get_Dlg_Item(IDC_SCENE))->Set_Range(0,3);
	if ((ShaderCaps::VertexShaderVersion) && (ShaderCaps::PixelShaderVersion))
	{
		((SliderCtrlClass *)Get_Dlg_Item(IDC_SHADERS))->Set_Pos(Get_Registry_Int("ShaderDetail",2),false);
		((SliderCtrlClass *)Get_Dlg_Item(IDC_SCENE))->Set_Pos(Get_Registry_Int("SceneShaderDetail",0),false);
	}
	else
	{
		((SliderCtrlClass *)Get_Dlg_Item(IDC_SHADERS))->Set_Pos(0,false);
		((SliderCtrlClass *)Get_Dlg_Item(IDC_SCENE))->Set_Pos(0,false);
		Get_Dlg_Item(IDC_SHADERS)->Enable(false);
		Get_Dlg_Item(IDC_SCENE)->Enable(false);
	}
	Check_Dlg_Button(IDC_VSYNC,RenderDeviceSettings::VSync_Enabled());

	//FIXME
	((ComboBoxCtrlClass *)Get_Dlg_Item(IDC_AA))->Add_String(L"None");
	((ComboBoxCtrlClass *)Get_Dlg_Item(IDC_AA))->Set_Curr_Sel(0);
	Enable_Dlg_Item(IDC_AA, false);

	Vector3 v;
	v.X = 1.0;
	v.Y = 1.0;
	v.Z = 1.0;
	ListCtrlClass *l = Get_Dlg_Item(IDC_KEYS)->As_ListCtrlClass();
	l->Add_Column(L"Logical Key Name",0.5,v);
	l->Add_Column(L"Key",0.5,v);
	int x = KeyList.Count();
	for (int i = 0;i < x;i++)
	{
		const wchar_t *c = CharToWideChar(KeyList[i]->LogicalKeyName);
		l->Insert_Entry(i,c);
		WideStringClass *str = new WideStringClass;
		Input::Get_Translated_Key_Name(KeyList[i]->KeyNumber,*str);
		l->Set_Entry_Text(i,1,*str);
		l->Set_Entry_Data(i,1,KeyList[i]->KeyNumber);
		delete[] c;
		delete str;
	}
	MenuDialogClass::On_Init_Dialog();
}
void ConfigDlg::On_Destroy()
{
	RenderDeviceSettings::Set_VSync_Enabled(Is_Dlg_Button_Checked(IDC_VSYNC));
	if (RenderDeviceSettings::Are_Settings_Dirty())
	{
		//FIXME
		RenderDeviceSettings::Registry_Save(Build_Registry_Location_String((char *)0x007F5274,0,"Render"));
	};
	MenuDialogClass::On_Destroy();
}
void ConfigDlg::On_SliderCtrl_Pos_Changed(SliderCtrlClass *control,int controlid,int pos)
{
	switch (controlid)
	{
	case IDC_SHADERS:
		ShaderDetailChanged(pos);
		break;
	case IDC_SCENE:
		SceneShaderDetailChanged(pos);
		break;
	}
}
extern SimpleDynVecClass<int> Hints;
void ConfigDlg::On_Command(int ctrl_id, int message_id, DWORD param)
{
	HKEY key;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,(char *)0x007F5274,0,KEY_ALL_ACCESS,&key);
	switch (ctrl_id)
	{
	case IDC_CHATLOG:
		ClientChatLog = param;
		RegSetValueEx(key,"ClientChatLog",0,REG_DWORD,(BYTE *)&param,4);
		break;
	case IDC_SHADOWS:
		RegSetValueEx(key,"HighQualityShadows",0,REG_DWORD,(BYTE *)&param,4);
		break;
	case IDC_HINTS:
		HintsEnabled = param;
		RegSetValueEx(key,"HintsEnabled",0,REG_DWORD,(BYTE *)&param,4);
		break;
	case IDC_RHINTS:
		DeleteFile("data\\hints.cfg");
		Hints.Delete_All();
		break;
	}
	RegCloseKey(key);
	MenuDialogClass::On_Command(ctrl_id,message_id,param);
}
void ConfigDlg::On_ComboBoxCtrl_Sel_Change(ComboBoxCtrlClass *combo,int unk1,int unk2,int unk3)
{
	if (combo->Get_ID() == IDC_AA)
	{
		/*
		// FIXME
		HKEY key;
		RegOpenKeyEx(HKEY_LOCAL_MACHINE,(char *)0x007F5274,0,KEY_ALL_ACCESS,&key);
		int aa = ((ComboBoxCtrlClass *)Get_Dlg_Item(IDC_AA))->Get_Curr_Sel();
		RegSetValueEx(key,"AA",0,REG_DWORD,(BYTE *)&aa,4);
		RegCloseKey(key);
		*/
	}
}
bool ConfigDlg::On_Key_Down(uint32 key_id, uint32 key_data)
{
	return MenuDialogClass::On_Key_Down(key_id,key_data);
}
bool ConfigDlg::On_InputCtrl_Get_Key_Info(InputCtrlClass *Control,int ID,int KeyCode,WideStringClass &KeyName,int *Key)
{
	int keyid;
	switch (KeyCode)
	{
	case VK_LBUTTON:
		keyid = 0x100;
		break;
	case VK_RBUTTON:
		keyid = 0x101;
		break;
	case VK_MBUTTON:
		keyid = 0x102;
		break;
	case VK_XBUTTON1:
		keyid = 0x103;
		break;
	case VK_XBUTTON2:
		keyid = 0x104;
		break;
	case 0:
		keyid = 0;
		break;
	case 0x100:
		keyid = 0x7004;
		break;
	case 0x101:
		keyid = 0x7005;
		break;
	default:
		keyid = *((unsigned int *)0x0085E104);
		break;
	}
	*Key = keyid;
	if (keyid)
	{
		Input::Get_Translated_Key_Name(keyid,KeyName);
		if (currentkey != -1)
		{
			WideStringClass *str = new WideStringClass;
			Input::Get_Translated_Key_Name(keyid,*str);
			Get_Dlg_Item(IDC_KEYS)->As_ListCtrlClass()->Set_Entry_Text(currentkey,1,*str);
			Get_Dlg_Item(IDC_KEYS)->As_ListCtrlClass()->Set_Entry_Data(currentkey,1,keyid);
		}
	}
	int x = KeyList.Count();
	FILE *f = fopen("data\\keys.cfg","wt");
	if (f)
	{
		for (int j = 0;j < x;j++)
		{
			KeyList[j]->KeyNumber = Get_Dlg_Item(IDC_KEYS)->As_ListCtrlClass()->Get_Entry_Data(j,1);
			const char *str = Input::Get_Key_Name((unsigned short)KeyList[j]->KeyNumber);
			if (str == NULL)
			{
				str = "None_Key";
			}
			fprintf(f,"%s=%s\n",KeyList[j]->LogicalKeyName,str);
		}
		fclose(f);
	}
	return true;
}
void ConfigDlg::On_ListCtrl_Sel_Change(ListCtrlClass *Control,int ID,int Start,int End)
{
	currentkey = End;
	unsigned int key = Get_Dlg_Item(IDC_KEYS)->As_ListCtrlClass()->Get_Entry_Data(currentkey,1);
	WideStringClass *str = new WideStringClass;
	Input::Get_Translated_Key_Name(key,*str);
	((InputCtrlClass *)Get_Dlg_Item(IDC_KEY))->Set_Key_Assignment(key,*str);
}
ConfigDlg::~ConfigDlg()
{
}
