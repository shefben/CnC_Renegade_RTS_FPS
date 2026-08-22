#ifndef TT_INCLUDE__SIDEBAR_H
#define TT_INCLUDE__SIDEBAR_H
#include "menudialog.h"

class SidebarDlg : public MenuDialogClass {
private:
	unsigned int team;
	int currentinfantry;
	int currentvehicle;
public:
	SidebarDlg(unsigned int newteam);
	void On_Command(int ctrl_id, int message_id, DWORD param);
	void Do_Buy(int control);
	void On_Init_Dialog();
	bool On_Key_Down(uint32 key_id, uint32 key_data);
	void On_Frame_Update();
	int Get_Infantry_Cost(int infantry);
	int Get_Infantry_Preset(int infantry);
	int Get_Infantry_String_ID(int infantry);
	const char *Get_Infantry_Texture(int infantry);
	const char *Get_Infantry_Alternate(int infantry,int alt);
	int Get_Vehicle_Cost(int vehicle);
	unsigned int Get_Infantry_Data(int infantry);
	unsigned int Get_Vehicle_Data(int vehicle);
	~SidebarDlg();
	void Render();
	void On_Merchandise_Selected(MerchandiseCtrlClass *c,int i);
	void On_Merchandise_DblClk(MerchandiseCtrlClass *c,int i);
	int Get_Vehicle_Preset(int vehicle);
	int Get_Vehicle_String_ID(int vehicle);
	const char *Get_Vehicle_Alternate(int vehicle,int alt);
	const char *Get_Vehicle_Texture(int vehicle);
	static void Do_Dialog(unsigned int newteam);
	void On_Mouse_Wheel(int direction);
};

#endif