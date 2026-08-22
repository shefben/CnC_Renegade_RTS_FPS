#include "general.h"
#include "cPlayer.h"

#include "SysTimeClass.h"
#include "cPlayerManager.h"
#include "bitstream.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "NetworkObjectClassId.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
#include "CombatManager.h"
#include "cTeamManager.h"
#include "cTeam.h"

cPlayer::cPlayer() :
	god(false), PlayerId(-1000), LadderPoints(0), Kills(0), Deaths(0), PlayerType(-1), Rung(0), WolRank(-1), WolPoints(0),
	DamageScaleFactor(100), Ping(0), JoinTime(0), TotalTime(0), IpAddress(0), Fps(0), LastUpdateTimeMs(0), FastSortKey(0),
	NumWolGames(0), IsWaitingForIntermission(false), IsInGame(false), IsActive(false), WolClanId(0)
{
	Set_Object_Dirty_Bit(DB_NOT_CREATED, true);

	JoinTime = SysTimeClass::SystemTime.Get();
	Set_Is_In_Game(true);
	Set_Is_Waiting_For_Intermission(false);

	GameSpyAuthState = 0;
	GameSpyAuthStateEntryTimeMs = 0;
	GameSpyKickState = 0;
	GameSpyKickStateEntryTimeMs = 0;

	PacketType = 6;

	cPlayerManager::Add(this);
}



cPlayer::~cPlayer()
{
	cPlayerManager::Remove(this);
}



void cPlayer::Delete()
{
	delete this;
}



RENEGADE_FUNCTION
bool cPlayer::Load(ChunkLoadClass& chunkLoader)
AT2(0x0040C5F0, 0x0040C5F0);



void cPlayer::Reset_Player()
{
	PlayerDataClass::Reset_Player();
	Set_Ladder_Points(0);
	Kills = 0;
	Deaths = 0;
	JoinTime = SysTimeClass::SystemTime.Get();
	TotalTime = 0;
	Set_Object_Dirty_Bit(DB_OCCASIONAL,true);
	Set_Object_Dirty_Bit(DB_RARE,true);
}



void cPlayer::Set_Score(float score)
{
	Score = score;
	Set_Object_Dirty_Bit(DB_OCCASIONAL,true);
}



void cPlayer::Set_Is_In_Game(bool b)
{
	IsInGame = b;
	Set_Object_Dirty_Bit(DB_RARE,true);
}



void cPlayer::Set_Money(float money)
{
	Money = money;
	Set_Object_Dirty_Bit(DB_OCCASIONAL,true);
}



void cPlayer::Increment_Score(float score)
{
	if (CombatManager::Is_Gameplay_Permitted())
	{
		PlayerDataClass::Increment_Score(score);
		if (Is_Team_Player())
		{
			cTeamManager::Find_Team(PlayerType)->Increment_Score(score);
		}
		Set_Object_Dirty_Bit(DB_OCCASIONAL,true);
	}
}

void cPlayer::Increment_Money(float score)
{
	if (CombatManager::Is_Gameplay_Permitted())
	{
		PlayerDataClass::Increment_Money(score);
		Set_Object_Dirty_Bit(DB_OCCASIONAL,true);
	}
}

unsigned int cPlayer::Get_Network_Class_ID() const
{
	return 1011;
}



void cPlayer::Export_Creation(BitStreamClass& stream)
{
	stream.Add_Wide_Terminated_String(PlayerName);
}



void cPlayer::Export_Occasional(BitStreamClass& stream)
{
	PlayerDataClass::Export_Occasional(stream);

	stream.Add(Kills);
	stream.Add(Deaths);
}



void cPlayer::Export_Rare(BitStreamClass& stream)
{
	stream.Add(PlayerId);
	stream.Add(LadderPoints);
	stream.Add(PlayerType);
	stream.Add(DamageScaleFactor);
	stream.Add(Get_Ping());
	stream.Add(IsInGame);
	stream.Add(god);
	stream.Add(IsActive);
	stream.Add(WolRank);
	stream.Add(NumWolGames);

	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
		stream.Add_Wide_Terminated_String(customTag);
}

int cPlayer::Get_Ping()
{
	if (cNetwork::PServerConnection)
	{
		if (PlayerId > 0)
		{
			cRemoteHost *rh = cNetwork::Get_Server_Rhost(PlayerId);
			if (rh)
			{
				Ping = rh->Get_Ping();
			}
		}
	}
	return Ping;
}


void cPlayer::Import_Creation(BitStreamClass& stream)
{
	wchar_t *buf = PlayerName.Get_Buffer(0x100);
	stream.Get_Wide_Terminated_String(buf,0x100,0);
}



void cPlayer::Import_Rare(BitStreamClass& stream)
{
	stream.Get(PlayerId);
	sint32 temp;
	stream.Get(temp);
	LadderPoints = temp;
	stream.Get(temp);
	PlayerType = temp;
	stream.Get(DamageScaleFactor);
	stream.Get(Ping);
	stream.Get(IsInGame);
	bool b;
	stream.Get(b); // god
	stream.Get(b);
	Set_Is_Active(b);
	stream.Get(temp);
	WolRank = temp;
	stream.Get(NumWolGames);

	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		wchar_t *buf = customTag.Get_Buffer(0x100);
		stream.Get_Wide_Terminated_String(buf, 0x100, 0);
	}
}



void cPlayer::Import_Occasional(BitStreamClass& stream)
{
	PlayerDataClass::Import_Occasional(stream);
	sint32 temp;
	stream.Get(temp);
	Kills = temp;
	stream.Get(temp);
	Deaths = temp;
}



void cPlayer::Set_Ip_Address(unsigned long ip)
{
	IpAddress = ip;
}

void cPlayer::Reset_Join_Time()
{
	JoinTime = SysTimeClass::SystemTime.Get();
}

void cPlayer::Increment_Total_Time()
{
	TotalTime += SysTimeClass::SystemTime.Get() - JoinTime;
}

RENEGADE_FUNCTION
void cPlayer::Get_Player_String(int id,WideStringClass &str,bool b)
AT2(0x0040E860,0x0040E860);

SimpleNetworkObjectFactoryClass<cPlayer, NET_cPlayer> cPlayerFactory;
