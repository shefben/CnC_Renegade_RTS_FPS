#include "General.h"
#include "DlgCnCWinScreen.h"
#include "stylemgr.h"
#include "engine_game.h"
#include "WW3DAssetManager.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "TranslateDBClass.h"
#include "cPlayer.h"
#include "listctrl.h"
#include "CombatManager.h"
#include "cNetwork.h"
#include "TimeManager.h"
#include "cPlayerManager.h"
#include "cTeamManager.h"
#include "cTeam.h"
#include "string_ids.h"
#include "resource.h"
#include "Iterator.h"



REF_DEF1(DlgCnCWinScreen::Instance, DlgCnCWinScreen*, 0x0082F154);



DlgCnCWinScreen::DlgCnCWinScreen() : MenuDialogClass(0xF8), BackdropObj(0), Renderer(0), Backdrop(), Time(0.2f), IsLaddered(false)
{
	StyleMgrClass::Configure_Renderer(&Renderer);
	Renderer.Get_Shader()->Set_Depth_Compare(ShaderClass::PASS_ALWAYS);
	Renderer.Add_Quad(Render2DClass::Get_Screen_Resolution(),0xC8000000);
	if (The_Game()->WinnerID == 1)
	{
		BackdropObj = WW3DAssetManager::TheInstance->Create_Render_Obj("HUD_GDIWINBAN");
	}
	else
	{
		BackdropObj = WW3DAssetManager::TheInstance->Create_Render_Obj("HUD_NODWINBAN");
	}
	if (GameModeManager::Find("WOL")->Is_Active())
	{
		if (The_Game()->IsLaddered)
		{
			IsLaddered = true;
		}
	}
	Instance = this;
}
DlgCnCWinScreen::~DlgCnCWinScreen()
{
	if (BackdropObj)
	{
		BackdropObj->Remove();
		REF_PTR_RELEASE(BackdropObj);
	}
	Instance = 0;
}
void DlgCnCWinScreen::Render(void)
{
	if (ActiveMenu == this)
	{
		BackDrop->Render();
		Renderer.Render();
		Backdrop.Render();
		DialogBaseClass::Render();
	}
}
RENEGADE_FUNCTION
void DlgCnCWinScreen::On_Command(int ctrl_id, int mesage_id, DWORD param)
AT1(0x004854B0);
RENEGADE_FUNCTION
void DlgCnCWinScreen::On_Init_Dialog()
AT1(0x00484700);



void DlgCnCWinScreen::On_Frame_Update()
{
	Time -= TimeManager::FrameSeconds;
	if (Time <= 0.f)
	{
		Time = .2f;

		cPlayerManager::Sort_Players(false);

		WideStringClass nodScoreString; nodScoreString.Format(TRANSLATE(IDS_MENU_NOD_SCORE), (int)cTeamManager::Find_Team(0)->Get_Score());
		WideStringClass gdiScoreString; gdiScoreString.Format(TRANSLATE(IDS_MENU_GDI_SCORE), (int)cTeamManager::Find_Team(1)->Get_Score());

		if (The_Game()->WinnerID == 1)
		{
			Update_List(1, IDC_WIN1_LIST_CTRL);
			Update_List(0, IDC_LOSE1_LIST_CTRL);
			DialogBaseClass::Set_Dlg_Item_Text(IDC_WINNER_TEAM_SCORE_TEXT, gdiScoreString);
			DialogBaseClass::Set_Dlg_Item_Text(IDC_LOSER_TEAM_SCORE_TEXT, nodScoreString);
		}
		else
		{
			Update_List(0, IDC_WIN1_LIST_CTRL);
			Update_List(1, IDC_LOSE1_LIST_CTRL);
			DialogBaseClass::Set_Dlg_Item_Text(IDC_WINNER_TEAM_SCORE_TEXT, nodScoreString);
			DialogBaseClass::Set_Dlg_Item_Text(IDC_LOSER_TEAM_SCORE_TEXT, gdiScoreString);
		}
	}

	DialogBaseClass::On_Frame_Update();
}



RENEGADE_FUNCTION
void DlgCnCWinScreen::On_Menu_Activate(bool onoff)
AT1(0x00485500);



void DlgCnCWinScreen::Close_Dialog()
{
	if (Instance)
		Instance->End_Dialog();
}



static int reverseRungCompare(cPlayer* const& player1, cPlayer* const& player2)
{
	return player1->Rung - player2->Rung;
}



// 00485560
void DlgCnCWinScreen::Update_List(int teamId, int listId)
{
	ListCtrlClass* list = (ListCtrlClass*)Get_Dlg_Item(listId);
	TT_ASSERT(list);

	list->Delete_All_Entries();

	SimpleDynVecClass<cPlayer*> listedPlayers;

	for (Iterator<SList<cPlayer>> player(PlayerList); player; ++player)
		if (player->IsActive)
			if (player->PlayerType == teamId)
				listedPlayers.Add(player);

	if (listedPlayers.Count() > 0)
	{
		listedPlayers.qsort(&reverseRungCompare);

		for (int i = 0; i < listedPlayers.Count(); ++i)
		{
			cPlayer* player = listedPlayers[i];
			int entry = list->Insert_Entry(i, L"");

			TT_ASSERT(entry >= 0);
			if (entry >= 0)
			{
				// Data
				list->Set_Entry_Data(entry, 0, player->Rung);

				// Text
				list->Set_Entry_Int(entry, 0, player->Rung);
				list->Set_Entry_Text(entry, 1, player->PlayerName);
				list->Set_Entry_Int(entry, 2, (int)player->Score);
				list->Set_Entry_Int(entry, 3, player->Kills);
				if (IsLaddered)
					list->Set_Entry_Int(entry, 4, player->LadderPoints);

				// Color
				if (cNetwork::I_Am_Client() && player->PlayerId == cNetwork::Get_My_Id()) // Used to check for player->Owner == CombatManager::getTheStarReference()
				{
					Vector3 white(1.f, 1.f, 1.f);
					list->Add_Icon(entry, 1, "IF_LRGSTAR.TGA");
					list->Set_Entry_Color(entry, 0, white);
					list->Set_Entry_Color(entry, 1, white);
					list->Set_Entry_Color(entry, 2, white);
					list->Set_Entry_Color(entry, 3, white);
					if (IsLaddered)
						list->Set_Entry_Color(entry, 4, white);
				}
			}
		}
	}
}
