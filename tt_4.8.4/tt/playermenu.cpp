#include "General.h"
#include "menudialog.h"
#include "playermenu.h"
#include "input.h"

REF_ARR_DEF1(DIKeyboardButtons,char,256,0x0085DFE8);

void DlgCncBattleInfo::On_Frame_Update()
{
	int KeyId = Input::Get_Primary_Key_For_Function(INPUT_FUNCTION_BATTLE_INFO_TOGGLE);
	if (!DIKeyboardButtons[KeyId])
	{
		End_Dialog();
		return;
	}
	DialogBaseClass::On_Frame_Update();
}

void DlgCncTeamInfo::On_Frame_Update()
{
	int KeyId = Input::Get_Primary_Key_For_Function(INPUT_FUNCTION_TEAM_INFO_TOGGLE);
	if (!DIKeyboardButtons[KeyId])
	{
		End_Dialog();
		return;
	}
	DialogBaseClass::On_Frame_Update();
}


void DlgCncServerInfo::On_Frame_Update()
{
	int KeyId = Input::Get_Primary_Key_For_Function(INPUT_FUNCTION_SERVER_INFO_TOGGLE);
	if (!DIKeyboardButtons[KeyId])
	{
		End_Dialog();
		return;
	}
	DialogBaseClass::On_Frame_Update();
}
