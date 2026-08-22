#include "General.h"
#include "cDiagnostics.h"

#include "cRemoteHost.h"
#include "cNetwork.h"
#include "PacketManagerClass.h"
#include "cServerFps.h"
#include "cDevOptions.h"
#include "engine_3d.h"




REF_DEF2(cDiagnostics::PRenderer, Render2DTextClass*, 0x0082A0F8, 0x008292E0);
REF_DEF2(cDiagnostics::PFont, Font3DInstanceClass*, 0x0082A0FC, 0x008292E4);
REF_DEF2(cDiagnostics::DiagnosticX, float, 0x0082A100, 0x008292E8);
REF_DEF2(cDiagnostics::DiagnosticY, float, 0x0082A104, 0x008292EC);



RENEGADE_FUNCTION
void cDiagnostics::Init()
AT2(0x00460010, 0x0045FAA0);



RENEGADE_FUNCTION
void cDiagnostics::Close()
AT2(0x004600D0, 0x0045FB60);



RENEGADE_FUNCTION
void cDiagnostics::Show_Object_Tally()
AT2(0x00460110, 0x0045FBA0);



RENEGADE_FUNCTION
void cDiagnostics::Add_Diagnostic(const char* format, ...)
AT2(0x00460170, 0x0045FC00);



//RENEGADE_FUNCTION
//void cDiagnostics::Render()
//AT2(0x00460220, 0x0045FCB0);
void cDiagnostics::Render()
{
	if (cDiagnostics::PFont && cDiagnostics::PRenderer)
	{
		cDiagnostics::DiagnosticY = 75.f;

		if (cDevOptions::ShowFps)
		{
			StringClass string;
			StringClass tempString;

			string.Format("FPS = %3d (worst = %3d)", cNetwork::Fps, cNetwork::worstFps);

			if (cNetwork::I_Am_Server())
			{
				tempString.Format(", KBPS = %4d", PacketManager().Get_Total_Compressed_Bandwidth_Out() / 1024);
				string += tempString;
			}
			else if (cNetwork::I_Am_Client())
			{
				tempString.Format(", SFPS = %3d", cServerFps::Get_Instance()->Get_Fps());
				string += tempString;

				cRemoteHost* serverRemoteHost = cNetwork::Get_Client_Rhost();
				if (serverRemoteHost)
				{
					tempString.Format(", PING = %4d", serverRemoteHost->Get_Ping());
					string += tempString;

					tempString.Format(", KBPS = %4d", PacketManager().Get_Compressed_Bandwidth_In(serverRemoteHost->Get_Address()) / 1024);
					string += tempString;
				}
			}

			Vector2 location;
			location.X = Render2DClass::Get_Screen_Resolution().Right - 5.f - PFont->String_Width(string);
			location.Y = Render2DClass::Get_Screen_Resolution().Top + 2.f;

			cDiagnostics::PRenderer->Reset();
			cDiagnostics::PRenderer->Set_Location(location);
			cDiagnostics::PRenderer->Draw_Text(string);
			cDiagnostics::PRenderer->Render();
		}
	}
}
