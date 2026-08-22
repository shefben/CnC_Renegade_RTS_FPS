#ifndef TT_INCLUDE__CPLAYERMANAGER_H
#define TT_INCLUDE__CPLAYERMANAGER_H

#include "engine_player.h"
#include "cUserOptions.h"
template<class Action, class Actor> class TypedActionPtr;
template<class T> class Notifier;
class cPlayer;
class WideStringClass;
class ChunkLoadClass;
class Render2DClass;
class ChunkSaveClass;
class ChunkLoadClass;

enum PLAYERMGR_ACTION
{
	PLAYERMGR_ACTION_ADD = 0,
	PLAYERMGR_ACTION_REMOVE = 1,
	PLAYERMGR_ACTION_2 = 2,
	PLAYERMGR_ACTION_TYPECHANGED = 3,
};



class cPlayerManager
{

	static REF_DECL2(Y_INCREMENT_FACTOR, UNK);
	static REF_DECL2(PTextRenderer, Render2DClass*);
	static REF_DECL2(PFont, UNK);
	static REF_DECL2(XPos, UNK);
	static REF_DECL2(YPos, UNK);
	//static REF_DECL2(PlayerList, SList<cPlayer>); // This is a global variable in TT.
	static REF_ARR_DECL2(Player_Array,cPlayer *,255);
	static REF_DECL2(mNotifier, TT_NOOP(Notifier<TypedActionPtr<PLAYERMGR_ACTION, cPlayer>>));

public:

	static UNK Onetime_Init();
	static UNK Onetime_Shutdown();
	static void Think();
	static void Render();
	static cPlayer* Find_Player(int playerId);
	static cPlayer* Find_Player(const WideStringClass& playerName);
	static cPlayer* Find_Inactive_Player(const WideStringClass& playerName);
	static cPlayer* Find_Team_Player(int);
	static cPlayer* Find_Random_Team_Player(int);
	static cPlayer* Find_Team_Mate(cPlayer*);
	static cPlayer* Find_Clan_Mate(cPlayer*);
	static bool Is_Player_Present(int);
	static bool Is_Player_Present(WideStringClass&);
	static UNK Get_Player_Name(int);
	static void Add(cPlayer*);
	static void Remove(cPlayer*);
	static UNK Deactivated(cPlayer*);
	static UNK Activated(cPlayer*);
	static UNK Get_Average_Ladder_Points();
	static UNK Get_Average_WOL_Points();
	static UNK Get_Average_Games_Played();
	static UNK Get_Average_Ping();
	static UNK Get_Average_FPS();
	static void Compute_Ladder_Points(int teamId);
	static void Increment_Player_Times();
	static WideStringClass Determine_Mvp_Name();
	static UNK Compute_Full_Player_List_Height();
	static void Remove_Inactive();
	static void Remove_All();
	static int Count();
	static void Reset_Players();
	static UNK Tally_Team_Size(int);
	static bool Is_Kill_Treasonous(cPlayer*, cPlayer*);
	static void Sort_Players(bool);
	static UNK Player_Compare(const void*, const void*);
	static UNK Fast_Player_Compare(const void*, const void*);
	static UNK Compute_Fast_Sort_Key(cPlayer*);
	static void Construct_Heading(WideStringClass&, bool);
	static UNK List_Print(WideStringClass&, Vector3);
	static UNK Line(float, float, int);
	static UNK Render_Player_List();
	static void Log_Player_List();
	static bool Save(ChunkSaveClass&);
	static bool Load(ChunkLoadClass&);
	
	static SList<cPlayer>& Get_Player_Object_List() { return PlayerList; }

};

#endif
