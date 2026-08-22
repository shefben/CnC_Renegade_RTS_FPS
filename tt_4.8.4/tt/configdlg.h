#include "menudialog.h"
#include "dialogfactory.h"
class ConfigDlg : public MenuDialogClass {
private:
	int currentkey;
public:
	void End_Dialog();
	ConfigDlg();
	void On_Init_Dialog();
	void On_Destroy();
	void On_SliderCtrl_Pos_Changed(SliderCtrlClass *control,int controlid,int pos);
	void On_Command(int ctrl_id, int message_id, DWORD param);
	void On_ComboBoxCtrl_Sel_Change(ComboBoxCtrlClass *combo,int unk1,int unk2,int unk3);
	bool On_Key_Down(uint32 key_id, uint32 key_data);
	bool On_InputCtrl_Get_Key_Info(InputCtrlClass *Control,int ID,int KeyCode,WideStringClass &KeyName,int *Key);
	void On_ListCtrl_Sel_Change(ListCtrlClass *Control,int ID,int Start,int End);
	~ConfigDlg();
	static void Do_Dialog() {ConfigDlg *dialog = new ConfigDlg; dialog->Start_Dialog (); REF_PTR_RELEASE (dialog); }
};
