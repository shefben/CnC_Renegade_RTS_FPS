#include "General.h"
#include "vehicledlg.h"
#include "buttonctrl.h"
#include "resource.h"
#include "imagectrl.h"
#include "listctrl.h"
#include "scripts.h"
#include "engine_pt.h"
#include "engine_obj2.h"
#include "engine_player.h"
#include "tt.h"
#include "cNetwork.h"
#include "cConnection.h"

VehicleDlg::VehicleDlg(GameObject *obj) : PopupDialogClass(IDD_VEHICLE)
{
	vehicle = obj;
}
void VehicleDlg::On_Init_Dialog()
{
	((ImageCtrlClass *)Get_Dlg_Item(IDC_VEHICLE))->Set_Texture(Get_Icon(Commands->Get_Preset_Name(vehicle)));
	PopupDialogClass::On_Init_Dialog();
	Vector3 v;
	v.X = 1.0;
	v.Y = 1.0;
	v.Z = 1.0;
	ListCtrlClass *l = Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass();
	l->Add_Column(L"Player Name",0.5,v);
	Update_List();
	Select_Player(0);
}
void VehicleDlg::Select_Player(int player)
{
	if (player <= Get_Vehicle_Occupant_Count(vehicle))
	{
		ListCtrlClass *l = Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass();
		l->Set_Curr_Sel(player);
		((ImageCtrlClass *)Get_Dlg_Item(IDC_PLAYER))->Set_Texture(Get_Icon(Commands->Get_Preset_Name(Get_Vehicle_Occupant(vehicle,player))));
	}
}
void VehicleDlg::Update_List()
{
	ListCtrlClass *l = Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass();
	int x = Get_Vehicle_Occupant_Count(vehicle);
	for (int i = 0;i < x;i++)
	{
		GameObject *player = Get_Vehicle_Occupant(vehicle,i);
		const wchar_t *c = Get_Wide_Player_Name(player);
		l->Insert_Entry(i,c);
	}
}
void VehicleDlg::On_Command(int ctrl_id, int message_id, DWORD param)
{
	switch (ctrl_id)
	{
	case IDC_CLOSE:
		End_Dialog();
		break;
	case IDC_KICK:
		if (Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass()->Get_Curr_Sel() > 0)
		{
			char buf[255];
			sprintf(buf,"f\n%d\n%d\n",Commands->Get_ID(vehicle),Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass()->Get_Curr_Sel());
			SendTextCs(buf,PrivateMessage,cNetwork::PClientConnection->Get_Local_Id(),-3);
			End_Dialog();
		}
		break;
	case IDC_GUNNER:
		if (Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass()->Get_Curr_Sel() > 1)
		{
			char buf[255];
			sprintf(buf,"g\n%d\n%d\n",Commands->Get_ID(vehicle),Get_Dlg_Item(IDC_CONTENTS)->As_ListCtrlClass()->Get_Curr_Sel());
			SendTextCs(buf,PrivateMessage,cNetwork::PClientConnection->Get_Local_Id(),-3);
			End_Dialog();
		}
		break;
	}
	PopupDialogClass::On_Command(ctrl_id,message_id,param);
}
void VehicleDlg::On_ListCtrl_Sel_Change(ListCtrlClass *Control,int ID,int Start,int End)
{
	Select_Player(End);
}
