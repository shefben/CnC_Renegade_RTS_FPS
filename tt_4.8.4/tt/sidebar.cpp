#include "General.h"
#include "sidebar.h"
#include "resource.h"
#include "merchandisectrl.h"
#include "buttonctrl.h"
#include "imagectrl.h"
#include "engine_3dre.h"
#include "ReferencerClass.h"
#include "tt.h"
#include "CombatManager.h"
#include "TimeManager.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "PurchaseSettingsDefClass.h"
#include "TeamPurchaseSettingsDefClass.h"
#include "BaseControllerClass.h"
#include "Engine_Player.h"
#include "Engine_PT.h"
#include "Engine_Game.h"
#include "GameInitMgrClass.h"

unsigned int alternateselection = 0;
extern bool AlternateSelectEnabled;
extern char *GDIAlternateSelectTexture1;
extern char *GDIAlternateSelectTexture2;
extern char *GDIAlternateSelectTexture3;
extern char *GDIAlternateSelectTexture4;
extern char *NODAlternateSelectTexture1;
extern char *NODAlternateSelectTexture2;
extern char *NODAlternateSelectTexture3;
extern char *NODAlternateSelectTexture4;
extern bool SidebarSoundsEnabled;
extern char *SidebarRefillSound;
extern char *SidebarInfantrySound;
extern char *SidebarVehicleSound;
float limit = 0;
extern float refilllimit;
extern bool ptupdate;
extern char *GDIUpArrowTexture;
extern char *GDIDownArrowTexture;
extern char *GDIBackgroundTexture1;
extern char *GDIBackgroundTexture2;
extern char *NODUpArrowTexture;
extern char *NODDownArrowTexture;
extern char *NODBackgroundTexture1;
extern char *NODBackgroundTexture2;

uint32 Purchase_Vendor_Item(SoldierGameObj* owner, PURCHASE_TYPE type, int position, int alternate, bool sendresponse);


void SidebarDlg::Do_Dialog(unsigned int newteam)
{
	SidebarDlg *newdlg = new SidebarDlg(newteam);
	newdlg->Start_Dialog();
	newdlg->Release_Ref();
}

SidebarDlg::SidebarDlg(unsigned int newteam) : MenuDialogClass(IDD_SIDEBAR)
{
	team = newteam;
	currentinfantry = 0;
	currentvehicle = 0;
}
void SidebarDlg::On_Command(int ctrl_id, int message_id, DWORD param)
{
	if (ctrl_id == IDC_INFANTRYUP)
	{
		for (int i = IDC_INFANTRY1;i <= IDC_INFANTRY3;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentinfantry > 0)
		{
			currentinfantry--;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry > 0))
		{
			currentinfantry--;
		}
	}
	if (ctrl_id == IDC_INFANTRYDOWN)
	{
		for (int i = IDC_INFANTRY1;i <= IDC_INFANTRY3;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentinfantry < 20)
		{
			currentinfantry++;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry < 20))
		{
			currentinfantry++;
		}
	}
	if (ctrl_id == IDC_VEHICLESUP)
	{
		for (int i = IDC_VEHICLES1;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentvehicle > 0)
		{
			currentvehicle--;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle > 0))
		{
			currentvehicle--;
		}
	}
	if (ctrl_id == IDC_VEHICLESDOWN)
	{
		for (int i = IDC_VEHICLES1;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentvehicle < 16)
		{
			currentvehicle++;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle < 16))
		{
			currentvehicle++;
		}
	}
	if (ctrl_id >= IDC_ALTERNATE1 && ctrl_id <= IDC_ALTERNATE4)
	{
		alternateselection = ctrl_id - IDC_ALTERNATE1;
	}
	MenuDialogClass::On_Command(ctrl_id,message_id,param);
}
void SidebarDlg::Do_Buy(int control)
{
	for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
	{
		((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
	}
	MerchandiseCtrlClass *merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(control));
	GameObject *owner;
	if (CombatManager::Get_The_Star())
	{
		owner = (GameObject *)CombatManager::Get_The_Star();
	}
	else
	{
		owner = 0;
	}
	if (!AlternateSelectEnabled)
	{
		Purchase_Vendor_Item((SoldierGameObj*)owner,(PURCHASE_TYPE)((merch->Get_User_Data() & 0xF00) / 0x100),(merch->Get_User_Data() & 0xFF),merch->Get_Selected_Alternate(),true);
	}
	else
	{
		int cost;
		int presetid;
		Get_Merchandise_Information(owner,(PURCHASE_TYPE)((merch->Get_User_Data() & 0xF00) / 0x100),(merch->Get_User_Data() & 0xFF),alternateselection - 1,cost,presetid);
		if (presetid)
		{
			Purchase_Vendor_Item((SoldierGameObj*)owner,(PURCHASE_TYPE)((merch->Get_User_Data() & 0xF00) / 0x100),(merch->Get_User_Data() & 0xFF),alternateselection - 1,true);
		}
		else
		{
			Purchase_Vendor_Item((SoldierGameObj*)owner,(PURCHASE_TYPE)((merch->Get_User_Data() & 0xF00) / 0x100),(merch->Get_User_Data() & 0xFF),(0 - 1),true);
		}
	}
	if (control == IDC_REFILL)
	{
		if (SidebarSoundsEnabled)
		{
			Create_2D_Sound(SidebarRefillSound);
		}
		limit = TimeManager::TotalSeconds;
	}
	if ((control >= IDC_INFANTRY1) && (control <= IDC_INFANTRY3) && (SidebarSoundsEnabled))
	{
		if (SidebarSoundsEnabled)
		{
			Create_2D_Sound(SidebarInfantrySound);
		}
	}
	if ((control >= IDC_VEHICLES1) && (control <= IDC_VEHICLES4) && (SidebarSoundsEnabled))
	{
		if (SidebarSoundsEnabled)
		{
			Create_2D_Sound(SidebarVehicleSound);
		}
	}
	End_Dialog();
}
void SidebarDlg::On_Init_Dialog()
{
	ptupdate = false;
	if (!AlternateSelectEnabled)
	{
		Get_Dlg_Item(IDC_ALTERNATE1)->As_ButtonCtrlClass()->Show(false);
		Get_Dlg_Item(IDC_ALTERNATE2)->As_ButtonCtrlClass()->Show(false);
		Get_Dlg_Item(IDC_ALTERNATE3)->As_ButtonCtrlClass()->Show(false);
		Get_Dlg_Item(IDC_ALTERNATE4)->As_ButtonCtrlClass()->Show(false);
	}
	else
	{
		if (team == 1)
		{
			Get_Dlg_Item(IDC_ALTERNATE1)->As_ButtonCtrlClass()->Set_Bitmap(NODAlternateSelectTexture1,0);
			Get_Dlg_Item(IDC_ALTERNATE2)->As_ButtonCtrlClass()->Set_Bitmap(NODAlternateSelectTexture2,0);
			Get_Dlg_Item(IDC_ALTERNATE3)->As_ButtonCtrlClass()->Set_Bitmap(NODAlternateSelectTexture3,0);
			Get_Dlg_Item(IDC_ALTERNATE4)->As_ButtonCtrlClass()->Set_Bitmap(NODAlternateSelectTexture4,0);
		}
		else
		{
			Get_Dlg_Item(IDC_ALTERNATE1)->As_ButtonCtrlClass()->Set_Bitmap(GDIAlternateSelectTexture1,0);
			Get_Dlg_Item(IDC_ALTERNATE2)->As_ButtonCtrlClass()->Set_Bitmap(GDIAlternateSelectTexture2,0);
			Get_Dlg_Item(IDC_ALTERNATE3)->As_ButtonCtrlClass()->Set_Bitmap(GDIAlternateSelectTexture3,0);
			Get_Dlg_Item(IDC_ALTERNATE4)->As_ButtonCtrlClass()->Set_Bitmap(GDIAlternateSelectTexture4,0);
		}
	}
	while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry < 20))
	{
		currentinfantry++;
	}
	while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle < 16))
	{
		currentvehicle++;
	}
	if (team == 1)
	{
		Get_Dlg_Item(IDC_INFANTRYUP)->As_ButtonCtrlClass()->Set_Bitmap(NODUpArrowTexture,0);
		Get_Dlg_Item(IDC_INFANTRYDOWN)->As_ButtonCtrlClass()->Set_Bitmap(NODDownArrowTexture,0);
		Get_Dlg_Item(IDC_VEHICLESUP)->As_ButtonCtrlClass()->Set_Bitmap(NODUpArrowTexture,0);
		Get_Dlg_Item(IDC_VEHICLESDOWN)->As_ButtonCtrlClass()->Set_Bitmap(NODDownArrowTexture,0);
		((ImageCtrlClass *)Get_Dlg_Item(IDC_BACKGROUND))->Set_Texture(NODBackgroundTexture1);
		((ImageCtrlClass *)Get_Dlg_Item(IDC_BACKGROUND2))->Set_Texture(NODBackgroundTexture2);
	}
	else
	{
		Get_Dlg_Item(IDC_INFANTRYUP)->As_ButtonCtrlClass()->Set_Bitmap(GDIUpArrowTexture,0);
		Get_Dlg_Item(IDC_INFANTRYDOWN)->As_ButtonCtrlClass()->Set_Bitmap(GDIDownArrowTexture,0);
		Get_Dlg_Item(IDC_VEHICLESUP)->As_ButtonCtrlClass()->Set_Bitmap(GDIUpArrowTexture,0);
		Get_Dlg_Item(IDC_VEHICLESDOWN)->As_ButtonCtrlClass()->Set_Bitmap(GDIDownArrowTexture,0);
		((ImageCtrlClass *)Get_Dlg_Item(IDC_BACKGROUND))->Set_Texture(GDIBackgroundTexture1);
		((ImageCtrlClass *)Get_Dlg_Item(IDC_BACKGROUND2))->Set_Texture(GDIBackgroundTexture2);
	}
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_REFILL))->Set_Texture(Get_Team_Purchase_Definition(team)->refilltexture);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_REFILL))->Set_Cost(0);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_REFILL))->Set_User_Data(PT_REFILL * 0x100);
	Set_Dlg_Item_Text(IDC_REFILL,L"");
	Set_Dlg_Item_Text(IDC_INFANTRY1,L"");
	Set_Dlg_Item_Text(IDC_INFANTRY2,L"");
	Set_Dlg_Item_Text(IDC_INFANTRY3,L"");
	Set_Dlg_Item_Text(IDC_VEHICLES1,L"");
	Set_Dlg_Item_Text(IDC_VEHICLES2,L"");
	Set_Dlg_Item_Text(IDC_VEHICLES3,L"");
	Set_Dlg_Item_Text(IDC_VEHICLES4,L"");
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_INFANTRY1))->Set_User_Data(0xFFFFFFFF);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_INFANTRY2))->Set_User_Data(0xFFFFFFFF);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_INFANTRY3))->Set_User_Data(0xFFFFFFFF);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES1))->Set_User_Data(0xFFFFFFFF);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES2))->Set_User_Data(0xFFFFFFFF);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES3))->Set_User_Data(0xFFFFFFFF);
	((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES4))->Set_User_Data(0xFFFFFFFF);
	MenuDialogClass::On_Init_Dialog();
}
bool SidebarDlg::On_Key_Down(uint32 key_id, uint32 key_data)
{
	if (key_id == VK_DOWN)
	{
		for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentinfantry < 20)
		{
			currentinfantry++;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry < 20))
		{
			currentinfantry++;
		}
		if (currentvehicle < 16)
		{
			currentvehicle++;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle < 16))
		{
			currentvehicle++;
		}
		return true;
	}
	if (key_id == VK_UP)
	{
		for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentinfantry > 0)
		{
			currentinfantry--;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry > 0))
		{
			currentinfantry--;
		}
		if (currentvehicle > 0)
		{
			currentvehicle--;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle > 0))
		{
			currentvehicle--;
		}
		return true;
	}
	if (key_id >= '1' && key_id <= '8')
	{
		int control = key_id - '1' + IDC_REFILL;
		if (Get_Dlg_Item(control)->Is_Enabled() && Get_Dlg_Item(control)->Is_Visible())
		{
			Do_Buy(control);
		}
		return true;
	}
	if (key_id == '9')
	{
		for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		currentinfantry -= 3;
		if (currentinfantry < 0)
		{
			currentinfantry = 0;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry > 0))
		{
			currentinfantry--;
		}
		currentvehicle -= 4;
		if (currentvehicle < 0)
		{
			currentvehicle = 0;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle > 0))
		{
			currentvehicle--;
		}
		return true;
	}
	if (key_id == '0')
	{
		for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		currentinfantry += 3;
		if (currentinfantry > 20)
		{
			currentinfantry = 20;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry < 20))
		{
			currentinfantry++;
		}
		currentvehicle += 4;
		if (currentvehicle > 16)
		{
			currentvehicle = 16;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle < 16))
		{
			currentvehicle++;
		}
		return true;
	}
	if (key_id == VK_OEM_MINUS)
	{
		currentinfantry = 0;
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry < 20))
		{
			currentinfantry++;
		}
		currentvehicle = 0;
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle < 16))
		{
			currentvehicle++;
		}
		return true;
	}
	if (key_id == VK_OEM_PLUS)
	{
		currentinfantry = 20;
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry > 0))
		{
			currentinfantry--;
		}
		currentvehicle = 16;
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle > 0))
		{
			currentvehicle--;
		}
		return true;
	}
	if (key_id >= 'A' && key_id <= 'D')
	{
		alternateselection = key_id - 'A';
		return true;
	}
	return MenuDialogClass::On_Key_Down(key_id,key_data);
}
void SidebarDlg::On_Frame_Update()
{
	int cost;
	int preset;
	int stringid;
	unsigned int data;
	const char *texture;
	const char *alt1;
	const char *alt2;
	const char *alt3;
	MerchandiseCtrlClass *merch;
	int i;
	unsigned int num = 0;
	BaseControllerClass *b = BaseControllerClass::Find_Base_For_Star();
	bool infantryb,vehiclesb;
	if (b)
	{
		infantryb = b->Can_Generate_Soldiers();
		vehiclesb = b->Can_Generate_Vehicles();
	}
	else
	{
		infantryb = true;
		vehiclesb = true;
	}
	for (i = 23;i > 0;i--)
	{
		if (Get_Infantry_Preset(i))
		{
			num++;
		}
		if (num == 3)
		{
			break;
		}
	}
	if (currentinfantry > i)
	{
		currentinfantry = i;
	}
	num = 0;
	for (i = 19;i > 0;i--)
	{
		if (Get_Vehicle_Preset(i))
		{
			num++;
		}
		if (num == 4)
		{
			break;
		}
	}
	if (currentvehicle > i)
	{
		currentvehicle = i;
	}
	int infantry = currentinfantry;
	int vehicle = currentvehicle;
	do
	{
		cost = Get_Infantry_Cost(infantry);
		preset = Get_Infantry_Preset(infantry);
		data = Get_Infantry_Data(infantry);
		texture = Get_Infantry_Texture(infantry);
		alt1 = Get_Infantry_Alternate(infantry,0);
		alt2 = Get_Infantry_Alternate(infantry,1);
		alt3 = Get_Infantry_Alternate(infantry,2);
		stringid = Get_Infantry_String_ID(infantry);
		infantry++;
	} while (((!preset) || (stringid == 12574)) && (infantry < 24));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_INFANTRY1));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!infantryb && cost))
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	do
	{
		cost = Get_Infantry_Cost(infantry);
		preset = Get_Infantry_Preset(infantry);
		data = Get_Infantry_Data(infantry);
		texture = Get_Infantry_Texture(infantry);
		alt1 = Get_Infantry_Alternate(infantry,0);
		alt2 = Get_Infantry_Alternate(infantry,1);
		alt3 = Get_Infantry_Alternate(infantry,2);
		stringid = Get_Infantry_String_ID(infantry);
		infantry++;
	} while (((!preset) || (stringid == 12574)) && (infantry < 24));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_INFANTRY2));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!infantryb && cost))
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	do
	{
		cost = Get_Infantry_Cost(infantry);
		preset = Get_Infantry_Preset(infantry);
		data = Get_Infantry_Data(infantry);
		texture = Get_Infantry_Texture(infantry);
		alt1 = Get_Infantry_Alternate(infantry,0);
		alt2 = Get_Infantry_Alternate(infantry,1);
		alt3 = Get_Infantry_Alternate(infantry,2);
		stringid = Get_Infantry_String_ID(infantry);
		infantry++;
	} while (((!preset) || (stringid == 12574)) && (infantry < 24));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_INFANTRY3));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!infantryb && cost))
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	do
	{
		cost = Get_Vehicle_Cost(vehicle);
		preset = Get_Vehicle_Preset(vehicle);
		data = Get_Vehicle_Data(vehicle);
		texture = Get_Vehicle_Texture(vehicle);
		alt1 = Get_Vehicle_Alternate(vehicle,0);
		alt2 = Get_Vehicle_Alternate(vehicle,1);
		alt3 = Get_Vehicle_Alternate(vehicle,2);
		stringid = Get_Vehicle_String_ID(vehicle);
		vehicle++;
	} while (((!preset) || (stringid == 12574)) && (vehicle < 20));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES1));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!vehiclesb) || CurrentlyBuilding)
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	do
	{
		cost = Get_Vehicle_Cost(vehicle);
		preset = Get_Vehicle_Preset(vehicle);
		data = Get_Vehicle_Data(vehicle);
		texture = Get_Vehicle_Texture(vehicle);
		alt1 = Get_Vehicle_Alternate(vehicle,0);
		alt2 = Get_Vehicle_Alternate(vehicle,1);
		alt3 = Get_Vehicle_Alternate(vehicle,2);
		stringid = Get_Vehicle_String_ID(vehicle);
		vehicle++;
	} while (((!preset) || (stringid == 12574)) && (vehicle < 20));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES2));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!vehiclesb) || CurrentlyBuilding)
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	do
	{
		cost = Get_Vehicle_Cost(vehicle);
		preset = Get_Vehicle_Preset(vehicle);
		data = Get_Vehicle_Data(vehicle);
		texture = Get_Vehicle_Texture(vehicle);
		alt1 = Get_Vehicle_Alternate(vehicle,0);
		alt2 = Get_Vehicle_Alternate(vehicle,1);
		alt3 = Get_Vehicle_Alternate(vehicle,2);
		stringid = Get_Vehicle_String_ID(vehicle);
		vehicle++;
	} while (((!preset) || (stringid == 12574)) && (vehicle < 20));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES3));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!vehiclesb) || CurrentlyBuilding)
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	do
	{
		cost = Get_Vehicle_Cost(vehicle);
		preset = Get_Vehicle_Preset(vehicle);
		data = Get_Vehicle_Data(vehicle);
		texture = Get_Vehicle_Texture(vehicle);
		alt1 = Get_Vehicle_Alternate(vehicle,0);
		alt2 = Get_Vehicle_Alternate(vehicle,1);
		alt3 = Get_Vehicle_Alternate(vehicle,2);
		stringid = Get_Vehicle_String_ID(vehicle);
		vehicle++;
	} while (((!preset) || (stringid == 12574)) && (vehicle < 20));
	merch = ((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_VEHICLES4));
	if ((stringid == 7265) || (stringid == 7263) || (stringid == 9724) || (cost > (int)Get_Money(cNetwork::PClientConnection->Get_Local_Id())) || (!vehiclesb) || CurrentlyBuilding)
	{
		merch->Enable(false);
	}
	else
	{
		merch->Enable(true);
	}
	if ((merch->Get_User_Data() != data) || (ptupdate == true))
	{
		merch->Set_Texture(texture);
		merch->Set_Cost(cost);
		merch->Set_User_Data(data);
		if (!AlternateSelectEnabled)
		{
			if ((alt1) && (alt1[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt1);
			}
			if ((alt2) && (alt2[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt2);
			}
			if ((alt3) && (alt3[0] != ' '))
			{
				merch->Add_Alternate_Texture(alt3);
			}
		}
		if (!preset)
		{
			merch->Show(false);
			merch->Set_Dirty(true);
		}
		else
		{
			merch->Show(true);
			merch->Set_Dirty(true);
		}
	}
	if ((limit) && (TimeManager::TotalSeconds-limit < refilllimit))
	{
		((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_REFILL))->Enable(false);
	}
	else
	{
		((MerchandiseCtrlClass *)Get_Dlg_Item(IDC_REFILL))->Enable(true);
		limit = 0;
	}
	Get_Dlg_Item(IDC_ALTERNATE1)->As_ButtonCtrlClass()->Enable(true);
	Get_Dlg_Item(IDC_ALTERNATE2)->As_ButtonCtrlClass()->Enable(true);
	Get_Dlg_Item(IDC_ALTERNATE3)->As_ButtonCtrlClass()->Enable(true);
	Get_Dlg_Item(IDC_ALTERNATE4)->As_ButtonCtrlClass()->Enable(true);
	Get_Dlg_Item(IDC_ALTERNATE1 + alternateselection)->As_ButtonCtrlClass()->Enable(false);
	Get_Dlg_Item(IDC_ALTERNATE1)->As_ButtonCtrlClass()->Set_Dirty(true);
	Get_Dlg_Item(IDC_ALTERNATE2)->As_ButtonCtrlClass()->Set_Dirty(true);
	Get_Dlg_Item(IDC_ALTERNATE3)->As_ButtonCtrlClass()->Set_Dirty(true);
	Get_Dlg_Item(IDC_ALTERNATE4)->As_ButtonCtrlClass()->Set_Dirty(true);
	ptupdate = false;
	MenuDialogClass::On_Frame_Update();
}
int SidebarDlg::Get_Infantry_Cost(int infantry)
{
	PurchaseSettingsDefClass *i1 = Get_Purchase_Definition(PTT_CHARACTERS,team);
	PurchaseSettingsDefClass *i2 = Get_Purchase_Definition(PTT_SECRETCHARS,team);
	if (infantry < 4)
	{
		return 0;
	}
	else if (infantry < 14)
	{
		if (i1)
		{
			if (!Is_Base_Powered(PTTEAM(team)) && !DisableCostMultiplier)
			{
				return i1->costs[infantry-4] * 2;
			}
			else
			{
				return i1->costs[infantry-4];
			}
		}
		else
		{
			return 0;
		}
	}
	else if (infantry < 24)
	{
		if (i2)
		{
			if (!Is_Base_Powered(PTTEAM(team)) && !DisableCostMultiplier)
			{
				return i2->costs[infantry-14] * 2;
			}
			else
			{
				return i2->costs[infantry-14];
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
int SidebarDlg::Get_Infantry_Preset(int infantry)
{
	TeamPurchaseSettingsDefClass *t = Get_Team_Purchase_Definition(team);
	PurchaseSettingsDefClass *i1 = Get_Purchase_Definition(PTT_CHARACTERS,team);
	PurchaseSettingsDefClass *i2 = Get_Purchase_Definition(PTT_SECRETCHARS,team);
	if (infantry < 4)
	{
		return t->presetids[infantry];
	}
	else if (infantry < 14)
	{
		if (i1)
		{
			return i1->presetids[infantry-4];
		}
		else
		{
			return 0;
		}
	}
	else if (infantry < 24)
	{
		if (i2)
		{
			return i2->presetids[infantry-14];
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
int SidebarDlg::Get_Infantry_String_ID(int infantry)
{
	TeamPurchaseSettingsDefClass *t = Get_Team_Purchase_Definition(team);
	PurchaseSettingsDefClass *i1 = Get_Purchase_Definition(PTT_CHARACTERS,team);
	PurchaseSettingsDefClass *i2 = Get_Purchase_Definition(PTT_SECRETCHARS,team);
	if (infantry < 4)
	{
		return t->stringids[infantry];
	}
	else if (infantry < 14)
	{
		if (i2)
		{
			return i1->stringids[infantry-4];
		}
		else
		{
			return 0;
		}
	}
	else if (infantry < 24)
	{
		if (i2)
		{
			return i2->stringids[infantry-14];
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
const char *SidebarDlg::Get_Infantry_Texture(int infantry)
{
	TeamPurchaseSettingsDefClass *t = Get_Team_Purchase_Definition(team);
	PurchaseSettingsDefClass *i1 = Get_Purchase_Definition(PTT_CHARACTERS,team);
	PurchaseSettingsDefClass *i2 = Get_Purchase_Definition(PTT_SECRETCHARS,team);
	if (infantry < 4)
	{
		return t->textures[infantry];
	}
	else if (infantry < 14)
	{
		if (i1)
		{
			return i1->textures[infantry-4];
		}
		else
		{
			return 0;
		}
	}
	else if (infantry < 24)
	{
		if (i2)
		{
			return i2->textures[infantry-14];
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
const char *SidebarDlg::Get_Infantry_Alternate(int infantry,int alt)
{
	PurchaseSettingsDefClass *i1 = Get_Purchase_Definition(PTT_CHARACTERS,team);
	PurchaseSettingsDefClass *i2 = Get_Purchase_Definition(PTT_SECRETCHARS,team);
	if (infantry < 4)
	{
		return NULL;
	}
	else if (infantry < 14)
	{
		if (i1)
		{
			if (i1->altpresetids[infantry-4][alt])
			{
				return i1->alttextures[infantry-4][alt];
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return 0;
		}
	}
	else if (infantry < 24)
	{
		if (i2)
		{
			if (i2->altpresetids[infantry-14][alt])
			{
				return i2->alttextures[infantry-14][alt];
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return NULL;
	}
}
int SidebarDlg::Get_Vehicle_Cost(int vehicle)
{
	PurchaseSettingsDefClass *v1 = Get_Purchase_Definition(PTT_VEHICLES,team);
	PurchaseSettingsDefClass *v2 = Get_Purchase_Definition(PTT_SECRETVEHICLES,team);
	if (vehicle < 10)
	{
		if (v1)
		{
			if (!Is_Base_Powered(PTTEAM(team)) && !DisableCostMultiplier)
			{
				return v1->costs[vehicle] * 2;
			}
			else
			{
				return v1->costs[vehicle];
			}
		}
		else
		{
			return 0;
		}
	}
	else if (vehicle < 20)
	{
		if (v2)
		{
			if (!Is_Base_Powered(PTTEAM(team)) && !DisableCostMultiplier)
			{
				return v2->costs[vehicle-10] * 2;
			}
			else
			{
				return v2->costs[vehicle-10];
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
unsigned int SidebarDlg::Get_Infantry_Data(int infantry)
{
	if (infantry < 4)
	{
		return (PT_ENLISTED * 0x100) + infantry;
	}
	else if (infantry < 14)
	{
		return (PT_CHARACTER * 0x100) + (infantry - 4);
	}
	else if (infantry < 24)
	{
		return (PT_SECRETCHAR * 0x100) + (infantry - 14);
	}
	else
	{
		return 0;
	}
}
unsigned int SidebarDlg::Get_Vehicle_Data(int vehicle)
{
	if (vehicle < 10)
	{
		return (PT_VEHICLE * 0x100) + vehicle;
	}
	else if (vehicle < 20)
	{
		return (PT_SECRETVEHICLE * 0x100) + (vehicle - 10);
	}
	else
	{
		return 0;
	}
}
SidebarDlg::~SidebarDlg()
{
	GameInitMgrClass::Continue_Game();
}
void SidebarDlg::Render()
{
	DialogBaseClass::Render();
}
void SidebarDlg::On_Merchandise_Selected(MerchandiseCtrlClass *c,int i)
{
	for (int j = IDC_REFILL;j <= IDC_VEHICLES4;j++)
	{
		((MerchandiseCtrlClass *)Get_Dlg_Item(j))->Reset_Purchase_Count();
	}
	c->Increment_Purchase_Count();
}
void SidebarDlg::On_Merchandise_DblClk(MerchandiseCtrlClass *c,int i)
{
	Do_Buy(c->Get_ID());
}
int SidebarDlg::Get_Vehicle_Preset(int vehicle)
{
	PurchaseSettingsDefClass *v1 = Get_Purchase_Definition(PTT_VEHICLES,team);
	PurchaseSettingsDefClass *v2 = Get_Purchase_Definition(PTT_SECRETVEHICLES,team);
	if (vehicle < 10)
	{
		if (v1)
		{
			return v1->presetids[vehicle];
		}
		else
		{
			return 0;
		}
	}
	else if (vehicle < 20)
	{
		if (v2)
		{
			return v2->presetids[vehicle-10];
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
int SidebarDlg::Get_Vehicle_String_ID(int vehicle)
{
	PurchaseSettingsDefClass *v1 = Get_Purchase_Definition(PTT_VEHICLES,team);
	PurchaseSettingsDefClass *v2 = Get_Purchase_Definition(PTT_SECRETVEHICLES,team);
	if (vehicle < 10)
	{
		if (v1)
		{
			return v1->stringids[vehicle];
		}
		else
		{
			return 0;
		}
	}
	else if (vehicle < 20)
	{
		if (v2)
		{
			return v2->stringids[vehicle-10];
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
const char *SidebarDlg::Get_Vehicle_Alternate(int vehicle,int alt)
{
	PurchaseSettingsDefClass *v1 = Get_Purchase_Definition(PTT_VEHICLES,team);
	PurchaseSettingsDefClass *v2 = Get_Purchase_Definition(PTT_SECRETVEHICLES,team);
	if (vehicle < 10)
	{
		if (v1)
		{
			if (v1->altpresetids[vehicle][alt])
			{
				return v1->alttextures[vehicle][alt];
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return 0;
		}
	}
	else if (vehicle < 20)
	{
		if (v2)
		{
			if (v2->altpresetids[vehicle-10][alt])
			{
				return v2->alttextures[vehicle-10][alt];
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
const char *SidebarDlg::Get_Vehicle_Texture(int vehicle)
{
	PurchaseSettingsDefClass *v1 = Get_Purchase_Definition(PTT_VEHICLES,team);
	PurchaseSettingsDefClass *v2 = Get_Purchase_Definition(PTT_SECRETVEHICLES,team);
	if (vehicle < 10)
	{
		if (v1)
		{
			return v1->textures[vehicle];
		}
		else
		{
			return 0;
		}
	}
	else if (vehicle < 20)
	{
		if (v2)
		{
			return v2->textures[vehicle-10];
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
void SidebarDlg::On_Mouse_Wheel(int direction)
{
	if (direction < 0)
	{
		for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentinfantry > 0)
		{
			currentinfantry--;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry > 0))
		{
			currentinfantry--;
		}
		if (currentvehicle > 0)
		{
			currentvehicle--;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle > 0))
		{
			currentvehicle--;
		}
	}
	if (direction > 0)
	{
		for (int i = IDC_REFILL;i <= IDC_VEHICLES4;i++)
		{
			((MerchandiseCtrlClass *)Get_Dlg_Item(i))->Reset_Purchase_Count();
		}
		if (currentinfantry < 20)
		{
			currentinfantry++;
		}
		while ((!Get_Infantry_Preset(currentinfantry)) && (currentinfantry < 20))
		{
			currentinfantry++;
		}
		if (currentvehicle < 16)
		{
			currentvehicle++;
		}
		while ((!Get_Vehicle_Preset(currentvehicle)) && (currentvehicle < 16))
		{
			currentvehicle++;
		}
	}
}
