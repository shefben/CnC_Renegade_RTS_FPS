#include "General.h"
#include "menudialog.h"
#include "resource.h"
#include "childdialog.h"
#include "engine_game.h"
#include "engine_obj2.h"
#include "listctrl.h"
#include "merchandisectrl.h"
#include "SoldierFactoryGameObj.h"
#include "VehicleFactoryGameObj.h"
#include "SoldierGameObj.h"
#include "tt.h"
#include "PurchaseSettingsDefClass.h"
#include "BaseControllerClass.h"
#include "GameInitMgrClass.h"

class PlayerTerminalDialogClass : public MenuDialogClass
{
	unsigned int team;
	ChildDialogClass *ChatboxDlg;
	PurchaseSettingsDefClass *PurchaseDef;
	unsigned int ChatboxLineCount;
public:
	PlayerTerminalDialogClass() : MenuDialogClass(0)
	{
	}
	virtual ~PlayerTerminalDialogClass();
	virtual void On_Init_Dialog();
	virtual void On_Destroy();
	virtual void Render();
	virtual void On_Command(int ctrl_id, int message_id, DWORD param);
	virtual void On_Frame_Update();
	virtual bool On_Key_Down(uint32 key_id, uint32 key_data);
	void Update_Chat_List();
};

RENEGADE_FUNCTION
void PlayerTerminalDialogClass::Update_Chat_List()
AT1(0x0047FF10);

void __stdcall OnPtChatListUpdate(PlayerTerminalDialogClass *Terminal)
{
	ListCtrlClass* List = (ListCtrlClass*)Terminal->Get_Dlg_Item(IDC_CHATBOX);
	int ScrollPos = List->Get_Scroll_Pos();
	int Selection = List->Get_Curr_Sel();
	Terminal->Update_Chat_List();
	if (ScrollPos == List->Get_Scroll_Bar().Get_Max_Pos())
	{
		List->Update_Scroll_Bar_Visibility();
		ScrollPos = List->Get_Scroll_Bar().Get_Max_Pos();
	}
	List->Set_Curr_Sel(Selection);
	List->Set_Scroll_Pos(ScrollPos);
	List->Get_Scroll_Bar().Set_Pos(List->Get_Scroll_Pos(), false);
}

void __declspec(naked) OnPtChatListUpdatePatch()
{
	__asm
	{
		push ecx
		call OnPtChatListUpdate
		retn
	}
}

void __stdcall OnPtChatListInit(ListCtrlClass& List)
{
	List.Get_Scroll_Bar().Set_Range(0,0);
}

void __declspec(naked) OnPtChatListInitPatch()
{
	__asm
	{
		push ecx
		push ecx
		call OnPtChatListInit
		pop ecx
		mov edx,0x004F2B90
		jmp edx
	}


}
bool CanPurchaseInfantry()
{
	BaseControllerClass* Base = BaseControllerClass::Find_Base_For_Star();
	if (!Base) return true;

	SoldierFactoryGameObj* SoldierFactoryObj = (SoldierFactoryGameObj*)Base->Find_Building(BT_SOLDIER_FACTORY);
	if (!SoldierFactoryObj) return false;
	return !SoldierFactoryObj->Is_Destroyed();
}

extern REF_DECL2(VehicleCount,int);
bool CanPurchaseVehicles()
{
	BaseControllerClass* Base = BaseControllerClass::Find_Base_For_Star();
	if (!Base) return true;
	VehicleFactoryGameObj* VehicleFactoryObj = (VehicleFactoryGameObj*)Base->Find_Building(BT_VEHICLE_FACTORY);
	if (!VehicleFactoryObj) return false;
	if (VehicleFactoryObj->Is_Busy()) return false;
	if (VehicleFactoryObj->Is_Destroyed()) return false;
	if (Get_Team_Vehicle_Count(VehicleFactoryObj->Get_Player_Type()) >= VehicleCount) return false;
	return true;
}

class PlayerUnitTerminalDialogClass : public MenuDialogClass
{
public:
	unsigned int unk1; // 0074
	int Type; // 0078
	PurchaseSettingsDefClass* PurchaseSettings; // 007C
	DynamicVectorClass<int> SelectedControlIds; // 0080
	unsigned int unk2; // 0098
	bool unk3; // 009C
	bool unk4; // 009D
	float CostMultiplier; // 00A0
	float unk5; // 00A4
	float unk6; // 00A8
public:
	PlayerUnitTerminalDialogClass() : MenuDialogClass(0)
	{
	}
	virtual ~PlayerUnitTerminalDialogClass();
	virtual void On_Init_Dialog();
	virtual void Render();
	virtual void On_Command(int ctrl_id, int message_id, DWORD param);
	virtual void On_Frame_Update();
	virtual bool On_Key_Down(uint32 key_id, uint32 key_data);
	virtual void On_Last_Menu_Ending();
	void Buy_Selected_Items();
};

RENEGADE_FUNCTION
void PlayerUnitTerminalDialogClass::Buy_Selected_Items()
AT1(0x004814F0);

void __stdcall OnUnitPtKeyDown(PlayerUnitTerminalDialogClass* Dialog)
{
	// If no items were selected, do not hide the window.
	if (Dialog->SelectedControlIds.Count() <= 0) return;

	switch (Dialog->Type)
	{
	case 0:
	case 3:
		if (!CanPurchaseInfantry()) return;
		break;
	case 1:
	case 4:
		if (!CanPurchaseVehicles()) return;
		break;
	}

	Dialog->Buy_Selected_Items();
	GameInitMgrClass::Continue_Game();
}
__declspec(naked) void OnUnitPtKeyDownPatch()
{
	__asm
	{
		push ecx
		call OnUnitPtKeyDown
		retn
	}
}
bool __stdcall OnUnitPtMerchandiseItemUpdate(PlayerUnitTerminalDialogClass* Dialog, DialogControlClass* Control)
{
	switch (Dialog->Type)
	{
	case 0:
	case 3:
		if (!CanPurchaseInfantry()) return false;
		break;
	case 1:
	case 4:
		if (!CanPurchaseVehicles()) return false;
		break;
	}
	return true;
}
__declspec(naked) void OnUnitPtMerchandiseItemUpdatePatch()
{
	__asm
	{
		jz SkipItem

		push esi
		push ebx
		call OnUnitPtMerchandiseItemUpdate
		test al,al
		jz DisableItem

		mov eax,0x00481FFC
		jmp eax
		
	DisableItem:
		push 0
		mov eax,0x004822A9
		jmp eax

	SkipItem:
		mov eax,0x004822B0
		jmp eax
	}
}

