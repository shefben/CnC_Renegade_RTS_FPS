#ifndef TT_INCLUDE__CGOD_H
#define TT_INCLUDE__CGOD_H



class cPlayer;
class WideStringClass;
class SoldierGameObj;
class ChunkSaveClass;
class ChunkLoadClass;
class Vector3;


class cGod
{

public:

	static bool Save(ChunkSaveClass&);
	static bool Load(ChunkLoadClass&);
	static void Think();
	static cPlayer* Create_Player(int clientId, const WideStringClass& name, int preferredTeamId, uint32 wolClanId, bool god);
	static UNK Create_Ai_Player();
	static SoldierGameObj* Create_Commando(int playerId, int playerType);
	static UNK Create_Commando(cPlayer*);
	static UNK Create_Grunt(Vector3&);
	static UNK Reset();
	static UNK Exit();
	static UNK Star_Killed();
	static UNK Respawn();
	static UNK Restart();
	static UNK Load_Game();
	static UNK Mission_Failed();
	static UNK Store_Inventory(SoldierGameObj*);
	static void Restore_Inventory(SoldierGameObj*);
	static UNK Reset_Inventory();

};



#endif