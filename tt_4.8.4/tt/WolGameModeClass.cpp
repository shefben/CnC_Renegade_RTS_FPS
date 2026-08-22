#include "General.h"
#include "WolGameModeClass.h"



RENEGADE_FUNCTION
void WolGameModeClass::Init()
AT2(0x0042DA50, 0x0042DB80);



RENEGADE_FUNCTION
void WolGameModeClass::Shutdown()
AT2(0x0042DBC0, 0x0042DCF0);



RENEGADE_FUNCTION
void WolGameModeClass::Think()
AT2(0x0042DC80, 0x0042DDB0);



RENEGADE_FUNCTION
void WolGameModeClass::System_Timer_Reset()
AT2(0x0042E460, 0x0042E590);



RENEGADE_FUNCTION
void WolGameModeClass::Create_Game(cGameData*)
AT2(0x0042E4C0, 0x0042E5F0);



RENEGADE_FUNCTION
void WolGameModeClass::Leave_Game()
AT2(0x0042E5F0, 0x0042E720);



RENEGADE_FUNCTION
void WolGameModeClass::Start_Game(cGameData*)
AT2(0x0042E730, 0x0042E860);



RENEGADE_FUNCTION
void WolGameModeClass::End_Game()
AT2(0x0042EA70, 0x0042EBA0);



RENEGADE_FUNCTION
bool WolGameModeClass::Post_Game_Check()
AT2(0x0042EB50, 0x0042EC80);



void WolGameModeClass::Accept_Actions()
{
}



RENEGADE_FUNCTION
void WolGameModeClass::Refusal_Actions()
AT2(0x0042EC70, 0x0042EDA0);



RENEGADE_FUNCTION
void WolGameModeClass::Evaluate_Clans(cGameData*)
AT2(0x0042ECC0, 0x0042EDF0);



RENEGADE_FUNCTION
void WolGameModeClass::Update_Channel_Settings(cGameData*, const RefPtr<WWOnline::ChannelData>&)
AT2(0x0042EDC0, 0x0042EEF0);



RENEGADE_FUNCTION
void WolGameModeClass::Init_WOL_Player(cPlayer*)
AT2(0x0042EEB0, 0x0042EFE0);



RENEGADE_FUNCTION
void WolGameModeClass::Get_WOL_User_Data(const wchar_t*)
AT2(0x0042EF90, 0x0042F0C0);



RENEGADE_FUNCTION
void WolGameModeClass::Page_WOL_User(const wchar_t*, const wchar_t*)
AT2(0x0042EFB0, 0x0042F0E0);



RENEGADE_FUNCTION
void WolGameModeClass::Reply_Last_Page(const wchar_t*)
AT2(0x0042F090, 0x0042F1C0);



RENEGADE_FUNCTION
void WolGameModeClass::Locate_WOL_User(const wchar_t*)
AT2(0x0042F230, 0x0042F360);



RENEGADE_FUNCTION
void WolGameModeClass::Invite_WOL_User(const wchar_t*, const wchar_t*)
AT2(0x0042F2F0, 0x0042F420);



RENEGADE_FUNCTION
void WolGameModeClass::Join_WOL_User(const wchar_t*)
AT2(0x0042F500, 0x0042F630);



RENEGADE_FUNCTION
bool WolGameModeClass::Kick_Player(const wchar_t*)
AT2(0x0042F750, 0x0042F880);



RENEGADE_FUNCTION
void WolGameModeClass::Ban_Player(const wchar_t*, uint32)
AT2(0x0042F980, 0x0042FAB0);



RENEGADE_FUNCTION
void WolGameModeClass::Auto_Kick()
AT2(0x0042FCA0, 0x0042FDD0);



RENEGADE_FUNCTION
bool WolGameModeClass::Is_Banned(const char*, uint32)
AT2(0x0042FD60, 0x0042FE90);



RENEGADE_FUNCTION
void WolGameModeClass::Read_Kick_List()
AT2(0x0042FDE0, 0x0042FF10);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::ChannelEvent&)
AT2(0x00431EA0, 0x00430140);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::UserEvent&)
AT2(0x004332E0, 0x004301D0);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(DlgWOLWaitEvent&)
AT2(0x00433690, 0x004306A0);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::GameStartEvent&)
AT2(0x004330B0, 0x004307B0);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::GameOptionsMessage&)
AT2(0x00433230, 0x004307D0);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::LadderInfoEvent&)
AT2(0x00431ED0, 0x00431020);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::ConnectionStatus&)
AT2(0x00432FD0, 0x00431230);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(WWOnline::ServerError&)
AT2(0x00432E50, 0x004314E0);



RENEGADE_FUNCTION
void WolGameModeClass::HandleNotification(TypedActionPtr<WOLPagedAction, WWOnline::PageMessage>&)
AT2(0x004335F0, 0x004315D0);



const char* WolGameModeClass::Name()
{
	return "WOL";
}



RENEGADE_FUNCTION
void WolGameModeClass::Render()
AT2(0x0042D790, 0x0042D8C0);
