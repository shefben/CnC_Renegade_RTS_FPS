#ifndef TT_INCLUDE__GAMEMODEMANAGER_H
#define TT_INCLUDE__GAMEMODEMANAGER_H



#include "SList.h"


class GameModeClass;
class Vector3;



class GameModeManager
{

public:

	static REF_DECL2(GameModeList, SList<GameModeClass>);
	static REF_DECL2(BackgroundColor, Vector3);

	static void Destroy(GameModeClass* gameMode);
	static GameModeClass* Add(GameModeClass* gameMode) { GameModeList.Add_Tail(gameMode); return gameMode; }
	static void Remove(GameModeClass* gameMode) { GameModeList.Remove(gameMode); }
	static int Count() { GameModeList.Get_Count(); }
	static void Destroy_All();
	static void List_Active_Game_Modes();
	static void Think();
	static void Safely_Deactivate();
	static void Render();
	static void Hide_Render_Frames(uint);
	static GameModeClass* Find(const char*);

};



#endif
