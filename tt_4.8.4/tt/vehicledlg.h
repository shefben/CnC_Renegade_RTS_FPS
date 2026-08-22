#include "popupdialog.h"
#include "tt.h"
#include "ReferencerClass.h"
#include "CombatManager.h"
#include "SoldierGameObj.h"
#include "VehicleGameObj.h"
#include "engine_obj2.h"
#include "engine_obj.h"
#include "engine_player.h"
class VehicleDlg : public PopupDialogClass {
private:
	GameObject *vehicle;
public:
	VehicleDlg(GameObject *obj);
	void On_Init_Dialog();
	void Select_Player(int player);
	void Update_List();
	void On_Command(int ctrl_id, int message_id, DWORD param);
	void On_ListCtrl_Sel_Change(ListCtrlClass *Control,int ID,int Start,int End);
	static void Do_Dialog()
	{
		if (CombatManager::Get_The_Star())
		{
			SoldierGameObj *obj = CombatManager::Get_The_Star();
			if (obj && obj->Get_Vehicle() && obj->Get_Vehicle()->Get_Driver() == obj)
			{
				VehicleDlg *dialog = new VehicleDlg(Get_Vehicle(obj));
				dialog->Start_Dialog();
				REF_PTR_RELEASE (dialog);
			}
		}
	}
};
