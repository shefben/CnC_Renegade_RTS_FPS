#include "General.h"
#include "DlgMPConnecting.h"

#include "CampaignManager.h"
#include "GameInitMgrClass.h"
#include "LanGameModeClass.h"
#include "WolGameModeClass.h"
#include "GameModeManager.h"
#include "cLanChat.h"
#include "DlgMPConnectionRefused.h"
#include "resource.h"
#include "cNetwork.h"
#include "cGameSpyAdmin.h"
#include "CombatGameModeClass.h"
#include "dialogmgr.h"
#include "MainMenuDialog.h"
//#include "ResourceMgr/ResourceManagerGameMode.h"



bool DlgMPConnecting::Create(int teamId, uint32 localPort)
{
	DlgMPConnecting* dialog = new DlgMPConnecting(teamId, localPort);
	dialog->Start_Dialog();
	dialog->Release_Ref();
	return true;
}



DlgMPConnecting::DlgMPConnecting(int _teamId, uint32 _localPort) :
	PopupDialogClass(IDD_MULTIPLAY_CONNECTING)
{
	teamId = _teamId;
	localPort = _localPort;
	gameData = NULL;
	isRefused = false;
}



void DlgMPConnecting::On_Command(int controlId, int messageId, DWORD param)
{
	if (controlId == IDCANCEL)
	{
		if (param != 1)
		{
			if (cNetwork::I_Am_Client())
				cNetwork::Cleanup_Client();

			if (cGameSpyAdmin::IsLaunchedFromGamespy)
				Stop_Main_Loop(0);

			else if (DialogMgrClass::Get_Dialog_Count() == 1)
			{
				MainMenuDialog::Do_Dialog();
				TT_UNREACHABLE;
			}
		}
	}

	PopupDialogClass::On_Command(controlId, messageId, param);
}



void DlgMPConnecting::On_Periodic()
{
	if (gameData)
	{
		Add_Ref();
		End_Dialog();

		WideStringClass string;
		if (gameData->Is_Valid_Settings(string, false))
		{
			CampaignManager::Select_Backdrop_Number_By_MP_Type(gameData->Get_Game_Type());
			GameInitMgrClass::setIsClientRequired(true);
			GameInitMgrClass::setIsServerRequired(false);
			GameInitMgrClass::Start_Game(gameData->MapName, teamId, localPort);
		}
		else
		{
			WideStringClass message;
			message.Format(L"Game data corrupt: %s", string);
			DlgMPConnectionRefused::DoDialog(message, false);
			
			LanGameModeClass* lanGameMode = (LanGameModeClass*)GameModeManager::Find("LAN");
			if (lanGameMode->Is_Active())
				lanGameMode->Get_Lan_Interface()->Refusal_Actions();
			
			WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
			if (wolGameMode->Is_Active())
				wolGameMode->Refusal_Actions();
		}

		Release_Ref();
	}
	else if (isRefused)
	{
		Add_Ref();
		End_Dialog();

		LanGameModeClass* lanGameMode = (LanGameModeClass*)GameModeManager::Find("LAN");
		if (lanGameMode->Is_Active())
			lanGameMode->Get_Lan_Interface()->Refusal_Actions();
		
		WolGameModeClass* wolGameMode = (WolGameModeClass*)GameModeManager::Find("WOL");
		if (wolGameMode->Is_Active())
			wolGameMode->Refusal_Actions();
		
		Release_Ref();
	}
}
