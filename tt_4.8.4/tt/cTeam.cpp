#include "General.h"
#include "cTeam.h"



#include "string_ids.h"
#include "TranslateDBClass.h"
#include "cPlayer.h"
#include "Engine_Player.h"
#include "bitstream.h"

#include "CombatManager.h"
#include "Vector3.h"
#include "cTeamManager.h"



cTeam::cTeam()
{
	cTeam::Reset();
	PacketType = 7;
	cTeamManager::Add(this);
}



cTeam::~cTeam()
{
	// The original code is does not call cTeamManager::Remove. That means that apparently it assumes that the team was already removed from the list. Assert that.
	TT_ASSERT(!cTeamManager::Find_Team(id));
}



void cTeam::Init(int _id)
{
	Set_Object_Dirty_Bit(DB_CREATION, true);
	id = _id;
	Init_Team_Name();
	Set_Network_ID(2100000004 + id);
}



void cTeam::Init_Team_Name()
{
	TT_ASSERT(id >= 0 && id < 2);

	name = TRANSLATE(IDS_MP_TEAMNAME_MISSIONS_TEAM_0 + id);
}



void cTeam::Reset()
{
	kills = 0;
	deaths = 0;
	score = 0;
	unk06C4 = 0;
	Set_Object_Dirty_Bit(DB_RARE, true);
}



void cTeam::Set_Kills(int _kills)
{
	kills = _kills;
	Set_Object_Dirty_Bit(DB_RARE, true);
}



void cTeam::Set_Deaths(int _deaths)
{
	deaths = _deaths;
	Set_Object_Dirty_Bit(DB_RARE, true);
}



void cTeam::Set_Score(float _score)
{
	score = _score;
	Set_Object_Dirty_Bit(DB_OCCASIONAL, true);
}




int cTeam::Tally_Size()
{
	return Tally_Team_Size(id);
}



void cTeam::Increment_Kills()
{
	if (CombatManager::Is_Gameplay_Permitted())
	{
		kills++;
		Set_Object_Dirty_Bit(DB_RARE, true);
	}
}



void cTeam::Increment_Deaths()
{
	if (CombatManager::Is_Gameplay_Permitted())
	{
		deaths++;
		Set_Object_Dirty_Bit(DB_RARE, true);
	}
}



void cTeam::Increment_Score(float amount)
{
	if (CombatManager::Is_Gameplay_Permitted())
	{
		score += amount;
		Set_Object_Dirty_Bit(DB_OCCASIONAL, true);
	}
}



float cTeam::Get_Kill_To_Death_Ratio()
{
	return deaths == 0 ? -1.f : kills / deaths;
}


RENEGADE_FUNCTION
void cTeam::Get_Team_String(int, WideStringClass&)
AT2(0x0041C9F0,0x0041C9F0);



Vector3 cTeam::Get_Color()
{
	return Get_Color_For_Team(id);
}



int cTeam::Tally_Money()
{
	int totalMoney = 0;

	for (SLNode<cPlayer>* node = PlayerList.Head(); node; node = node->Next())
	{
		cPlayer* player = node->Data();
		TT_ASSERT(player);

		if (player->IsActive && player->PlayerType == id)
			totalMoney += (int)player->Money;
	}
	return totalMoney;
}



void cTeam::Export_Creation(BitStreamClass& bitStream)
{
	bitStream.Add(id);
}



void cTeam::Import_Creation(BitStreamClass& bitStream)
{
	bitStream.Get(id);
	Init_Team_Name();
}



void cTeam::Export_Rare(BitStreamClass& bitStream)
{
	bitStream.Add(kills);
	bitStream.Add(deaths);
}



void cTeam::Import_Rare(BitStreamClass& bitStream)
{
	bitStream.Get(kills);
	bitStream.Get(deaths);
}



void cTeam::Export_Occasional(BitStreamClass& bitStream)
{
	bitStream.Add(score);
}



void cTeam::Import_Occasional(BitStreamClass& bitStream)
{
	bitStream.Get(score);
}