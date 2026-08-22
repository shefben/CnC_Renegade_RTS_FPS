#ifndef TT_INCLUDE__GAMEINITMGRCLASS_H
#define TT_INCLUDE__GAMEINITMGRCLASS_H



class GameInitMgrClass
{
	
	static REF_DECL2(IsClientRequired, bool);
	static REF_DECL2(IsServerRequired, bool);
	static REF_DECL2(RestoreSFX, bool);
	static REF_DECL2(RestoreMusic, bool);
	static REF_DECL2(NeedsGameExit, bool);
	static REF_DECL2(Mode, UNK);
	static REF_DECL2(WOLReturnDialog, UNK);

public:

	static REF_DECL2(NeedsGameExitAll, bool);
	static bool Is_Game_In_Progress();
	static void Start_Game(const char*, int, uint32);
	static void End_Game();
	static void Continue_Game();
	static void Display_End_Game_Menu();
	static UNK Transmit_Player_Data(int, uint32);
	static UNK Start_Client_Server();
	static UNK End_Client_Server();
	static UNK Initialize_SP();
	static UNK Shutdown_SP();
	static UNK Initialize_Skirmish();
	static UNK Shutdown_Skirmish();
	static UNK Initialize_LAN();
	static UNK Shutdown_LAN();
	static UNK Initialize_WOL();
	static UNK Shutdown_WOL();
	static UNK Shutdown();
	static UNK Think();

	static void Set_Needs_Game_Exit_All(bool value) { NeedsGameExitAll = value; }
	static void Set_Needs_Game_Exit(bool value) { NeedsGameExit = value; }

	static void setIsClientRequired(bool value) { IsClientRequired = value; }
	static void setIsServerRequired(bool value) { IsServerRequired = value; }

};



#endif