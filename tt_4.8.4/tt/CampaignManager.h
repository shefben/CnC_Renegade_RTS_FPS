#ifndef TT_INCLUDE__CAMPAIGNMANAGER_H
#define TT_INCLUDE__CAMPAIGNMANAGER_H



#include "engine_string.h"



class ChunkSaveClass;
class ChunkLoadClass;



class CampaignManager
{

private:

	static REF_DECL2(State, int);
	static REF_DECL2(BackdropIndex, int);

public:

	static void Init();
	static void Shutdown();
	static bool Save(ChunkSaveClass&);
	static bool Load(ChunkLoadClass&);
	static void Start_Campaign(int);
	static void Continue(bool);
	static void Reset();
	static void Replay_Level(const char*, int);
	static uint Get_Backdrop_Description_Count();
	static const char* Get_Backdrop_Description(int);
	static void Select_Backdrop_Number(int);
	static void Select_Backdrop_Number_By_MP_Type(int);

};



#endif