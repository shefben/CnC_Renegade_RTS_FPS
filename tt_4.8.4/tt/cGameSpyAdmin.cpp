#include "general.h"

#include "cGameSpyAdmin.h"


REF_DEF2(cGameSpyAdmin::IsUnderGamespyMenuing, bool, 0x0082FD91, 0x0082EF79);
REF_DEF2(cGameSpyAdmin::IsLaunchedFromGamespy, bool, 0x0082FD93, 0x0082EF7B);
REF_DEF2(cGameSpyAdmin::IsLaunchFromGamespyRequested, bool, 0x0082FD92, 0x0082EF7A);
REF_DEF2(cGameSpyAdmin::IsServerGamespyListed, bool, 0x0082FD94, 0x0082EF7C);
REF_DEF2(cGameSpyAdmin::PasswordAttempt, WideStringClass, 0x0082FD8C, 0x0082EF74);



RENEGADE_FUNCTION
void cGameSpyAdmin::Think()
AT2(0x004E3F70, 0x004E3810);



RENEGADE_FUNCTION
void cGameSpyAdmin::HandleNotification(DlgWOLWaitEvent&)
AT2(0x004E40C0, 0x004E3960);



RENEGADE_FUNCTION
void cGameSpyAdmin::Join_Server()
AT2(0x004E41E0, 0x004E3A80);



RENEGADE_FUNCTION
void cGameSpyAdmin::Reset()
AT2(0x004E4240, 0x004E3AE0);



RENEGADE_FUNCTION
void cGameSpyAdmin::Connect_To_Game_Server()
AT2(0x004E4270, 0x004E3B10);



RENEGADE_FUNCTION
void cGameSpyAdmin::Set_Game_Host_Ip(uint32)
AT2(0x004E42C0, 0x004E3B60);



RENEGADE_FUNCTION
void cGameSpyAdmin::Set_Game_Host_Port(uint16)
AT2(0x004E42D0, 0x004E3B70);



bool cGameSpyAdmin::Is_Gamespy_Game()
{
	return IsUnderGamespyMenuing || IsLaunchFromGamespyRequested || IsLaunchedFromGamespy || IsServerGamespyListed;
}



RENEGADE_FUNCTION
bool cGameSpyAdmin::Is_Nickname_Collision(WideStringClass&)
AT2(0x004E4310, 0x004E3BB0);
