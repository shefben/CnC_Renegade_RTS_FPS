#ifndef TT_INCLUDE__CGAMESPYADMIN_H
#define TT_INCLUDE__CGAMESPYADMIN_H



#include "engine_string.h"
#include "notify.h"



class DlgWOLWaitEvent;



class cGameSpyAdmin :
	Observer<DlgWOLWaitEvent>
{

public:

	static REF_DECL2(DetectingBandwidth, bool);
	static REF_DECL2(IsUnderGamespyMenuing, bool);
	static REF_DECL2(IsLaunchFromGamespyRequested, bool);
	static REF_DECL2(IsLaunchedFromGamespy, bool);
	static REF_DECL2(IsServerGamespyListed, bool);
	static REF_DECL2(GameHostIp, int);
	static REF_DECL2(GameHostPort, short);
	static REF_DECL2(PasswordAttempt, WideStringClass);

	~cGameSpyAdmin();
	void HandleNotification(DlgWOLWaitEvent&);

	static void Think();
	static void Join_Server();
	static void Reset();
	static void Connect_To_Game_Server();
	static void Set_Game_Host_Ip(uint32);
	static void Set_Game_Host_Port(uint16);
	static bool Is_Gamespy_Game();
	static bool Is_Nickname_Collision(WideStringClass&);
	static void Set_Is_Under_Gamespy_Menuing(bool);
	static bool Get_Is_Under_Gamespy_Menuing();
	static void Set_Is_Launch_From_Gamespy_Requested(bool);
	static bool Get_Is_Launch_From_Gamespy_Requested();
	static void Set_Is_Launched_From_Gamespy(bool);
	static bool Get_Is_Launched_From_Gamespy();
	static void Set_Is_Server_Gamespy_Listed(bool);
	static bool Get_Is_Server_Gamespy_Listed();
	static void Set_Password_Attempt(WideStringClass&);
	static const wchar_t* Get_Password_Attempt() { return PasswordAttempt; }

};

#endif
