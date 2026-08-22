#ifndef TT_INCLUDE__COMBATGAMEMODECLASS_H
#define TT_INCLUDE__COMBATGAMEMODECLASS_H



#include "GameModeClass.h"



class CombatGameModeClass :
	public GameModeClass
{

public:

	void Combat_Keyboard();
	void Init();
	void Shutdown();
	void Load_Level();
	void Core_Shutdown();
	void Post_Load_Id_Uniqueness_Check();
	void Post_Load_Dynamic_Object_Filtering();
	void Compute_World_Size();
	void Spawn_Point_Validation();
	void Core_Restart();
	bool Load_Registry_Keys();
	bool Save_Registry_Keys();
	void Think();
	void Render();
	void Toggle_Multi_Hud();
	void Resume();
	void Suspend();
	void Quick_Save();

	~CombatGameModeClass();

};



extern REF_DECL2(g_b_core_restart, bool);

extern void Stop_Main_Loop(int exitCode);



#endif