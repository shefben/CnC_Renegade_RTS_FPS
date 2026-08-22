#include "General.h"
#include "GameModeManager.h"



#include "DX8Wrapper.h"
#include "SortingRenderer.h"
#include "CombatManager.h"
#include "cNetwork.h"
#include "BackgroundMgrClass.h"
#include "ThreadClass.h"
#include "WW3D.h"
#include "PhysicsSceneClass.h"
#include "GameModeClass.h"
#include "CCameraClass.h"
#include "dialogmgr.h"
#include "MessageWindowClass.h"
#include "ObjectiveManager.h"
#include "ObjectivesViewerClass.h"
#include "cDiagnostics.h"
#include "textureloader.h"
#include "bink.h"
#include "SList.h"
#include "TexProjectClass.h"
#include "engine2.h"
#include "cConnection.h"
#include "cRemoteHost.h"
#include "Config.h"
#include "ConsoleModeClass.h"
#include "TextureController.h"



REF_DEF2(GameModeManager::GameModeList, SList<GameModeClass>, 0x0081E850, 0x0081DA38);
REF_DEF2(GameModeManager::BackgroundColor, Vector3, 0x0081E860, 0x0081DA48);



RENEGADE_FUNCTION
GameModeClass* GameModeManager::Find(const char*)
   AT2(0x004372B0,0x00437350);



void GameModeManager::Hide_Render_Frames(uint count)
{
	_HiddenFrameCount = count;
}


#include "engine_ttdef.h"
extern LoadLevelHook Think_Hook;
extern Vector3 FogColor;
void postRenderHook();
void GameModeManager::Render()
{
	if (Think_Hook)
	{
		Think_Hook();
	}
	if (!ConsoleBox.Is_Exclusive())
	{
		bool oldEnableTriangleDraw = DX8Wrapper::_Is_Triangle_Draw_Enabled();
		bool oldSortingEnableTriangleDraw = SortingRendererClass::_EnableTriangleDraw;

		bool isFrameShown = true;
		if (_HiddenFrameCount != 0)
		{
			DX8Wrapper::_Enable_Triangle_Draw(false);
			SortingRendererClass::_EnableTriangleDraw = false;
			isFrameShown = false;
		}
		
		//DX8RendererDebugger::Update();
		bool canRenderScene = CombatManager::Get_Scene() && (cNetwork::PClientConnection || !cNetwork::PServerConnection);
#ifndef DEBUG
		canRenderScene &= GameInFocus;
#endif
		
		if (canRenderScene && GameModeManager::Find("Combat")->Is_Active())
		{
			// TODO Find better location!
			DLListClass<RenderTargetTexture> pending = RenderTargetController::Instance()->GetPendingTextures();
			for (RenderTargetTexture* tex = pending.__Pop_Head(); tex != NULL; tex = pending.__Pop_Head())
			{
				tex->Render();
			};
			CombatManager::Get_Scene()->Pre_Render_Processing(*COMBAT_CAMERA);
		};
		Vector3 skyVector = FogColor;
		Vector3 skycolor = BackgroundMgrClass::Get_Clear_Color();
		if ((skycolor.X != 0) || (skycolor.Y != 0) || (skycolor.Z != 0))
		{
			skyVector = skycolor;
		}
		
		WW3D::Begin_Render(isFrameShown, isFrameShown, skyVector, 0);
#ifndef DEBUG
		if (GameInFocus)
#endif
		{
			for (SLNode<GameModeClass>* node = GameModeList.Head(); node; node = node->Next())
			{
				GameModeClass* gameMode = node->Data();
				if (gameMode->getState() != 1)
					gameMode->Render();
			}
		}
		
		if (CombatManager::Get_Message_Window())
			CombatManager::Get_Message_Window()->Render();
		
		ObjectiveManager::Viewer.Render();
		DialogMgrClass::Render();
		cDiagnostics::Render();

		if (config(CONFIG_NET_DIAGNOSTICS))
		{
			// Not clean (PRenderer is already rendered in cDiagnostics::Render), but it works
			if (cNetwork::I_Am_Client() && cNetwork::Get_Client_Rhost())
			{
				cDiagnostics::Add_Diagnostic("% 2d: Incoming packet loss:  %f", SERVER_HOST_ID, cNetwork::Get_Client_Rhost()->Get_Stats().Get_Pc_Packetloss_Received());
				cDiagnostics::Add_Diagnostic("% 2d: Outgoing packet loss:  %f", SERVER_HOST_ID, cNetwork::Get_Client_Rhost()->Get_Stats().Get_Pc_Packetloss_Sent());

				if (cNetwork::I_Am_Server())
				{
					for (int i = cNetwork::PServerConnection->Get_Min_RHost(); i <= cNetwork::PServerConnection->Get_Max_RHost(); i++)
					{
						cRemoteHost* remoteHost = cNetwork::PServerConnection->Get_Remote_Host(i);
						if (remoteHost)
						{
							cDiagnostics::Add_Diagnostic("");
							cDiagnostics::Add_Diagnostic("% 2d: Target bandwidth:      %d", i, remoteHost->getTargetBandwidth());
							cDiagnostics::Add_Diagnostic("% 2d: Max bandwidth:         %d", i, remoteHost->getMaxBandwidth());
							cDiagnostics::Add_Diagnostic("% 2d: Bandwidth multiplier:  %f", i, remoteHost->Get_Bandwidth_Multiplier());
							cDiagnostics::Add_Diagnostic("% 2d: Incoming packet loss:  %f", i, remoteHost->Get_Stats().Get_Pc_Packetloss_Received());
						}
					}
				}
				cDiagnostics::PRenderer->Render();
			}
		}
		
#ifndef DEBUG
		if (GameInFocus)
#endif
			BinkMovie::Render();

		WW3D::End_Render(true);
		
		if (canRenderScene)
		{
			postRenderHook();
			CombatManager::Get_Scene()->Post_Render_Processing();
		}
	
		ThreadClass::Switch_Thread();
		
		if (_HiddenFrameCount)
		{
			--_HiddenFrameCount;
			DX8Wrapper::_Enable_Triangle_Draw(oldEnableTriangleDraw);
			SortingRendererClass::_EnableTriangleDraw = oldSortingEnableTriangleDraw;
			TextureLoader::Flush_Pending_Load_Tasks();
		}
	}
}
