#ifndef TT_INCLUDE__WOLGAMEMODECLASS_H
#define TT_INCLUDE__WOLGAMEMODECLASS_H



#include "GameModeClass.h"
#include "WWOnline.h"
#include "GameData.h"
#include "notify.h"

class cGameData;
class DlgWOLWaitEvent;
class WOLPagedAction
{
};
class cPlayer;
template<typename T> class RefPtr;
template<class Action, class Actor> class TypedActionPtr;



class WolGameModeClass :
	public GameModeClass, public Signaler<WolGameModeClass>, public Observer<DlgWOLWaitEvent>, public Observer<WWOnline::GameStartEvent>, public Observer<WWOnline::GameOptionsMessage>, public Observer<WWOnline::ChannelEvent>, public Observer<WWOnline::UserEvent>, public Observer<WWOnline::LadderInfoEvent>, public Observer<TypedActionPtr<WOLPagedAction,WWOnline::PageMessage>>, public Observer<WWOnline::ConnectionStatus>, public Observer<WWOnline::ServerError>
{

public:

	WolGameModeClass();
	virtual ~WolGameModeClass();
	virtual const char* Name();
	virtual void Init();
	virtual void Shutdown();
	virtual void Render();
	virtual void Think();

	void System_Timer_Reset();
	void Create_Game(cGameData*);
	void Leave_Game();
	void Start_Game(cGameData*);
	void End_Game();
	bool Post_Game_Check();
	void Accept_Actions();
	void Refusal_Actions();
	void Evaluate_Clans(cGameData*);
	void Update_Channel_Settings(cGameData*, const RefPtr<WWOnline::ChannelData>&);
	void Init_WOL_Player(cPlayer*);
	void Get_WOL_User_Data(const wchar_t*);
	void Page_WOL_User(const wchar_t*, const wchar_t*);
	void Reply_Last_Page(const wchar_t*);
	void Locate_WOL_User(const wchar_t*);
	void Invite_WOL_User(const wchar_t*, const wchar_t*);
	void Join_WOL_User(const wchar_t*);
	bool Kick_Player(const wchar_t*);
	void Ban_Player(const wchar_t*, uint32);
	void Auto_Kick();
	bool Is_Banned(const char*, uint32);
	void Read_Kick_List();
	void HandleNotification(WWOnline::ChannelEvent&);
	void HandleNotification(WWOnline::UserEvent&);
	void HandleNotification(DlgWOLWaitEvent&);
	void HandleNotification(WWOnline::GameStartEvent&);
	void HandleNotification(WWOnline::GameOptionsMessage&);
	void HandleNotification(WWOnline::LadderInfoEvent&);
	void HandleNotification(WWOnline::ConnectionStatus&);
	void Handle_Disconnect();
	void HandleNotification(WWOnline::ServerError&);
	void Quit_And_Restart();
	void HandleNotification(TypedActionPtr<WOLPagedAction, WWOnline::PageMessage>&);
	void Game_Start_Timeout_Callback();
	void Game_Start_Timed_Out();
	bool Channel_Create_OK();
	void Set_Quiet_Mode(bool);

};



#endif