#include "General.h"
#include "cPlayerManager.h"
#include "engine_io.h"
#include "notify.h"
#include "render2d.h"
#include "cNetwork.h"
#include "cGameType.h"
#include "cPlayer.h"

typedef Notifier<TypedActionPtr<PLAYERMGR_ACTION, cPlayer>> playerMgrActionNotifier;
REF_DEF2(cPlayerManager::mNotifier, playerMgrActionNotifier, 0x0081CFC8, 0x0081C1A8);
REF_ARR_DEF2(cPlayerManager::Player_Array,cPlayer *,255,0x0081C7CC,0x0081B9AC);


cPlayer* cPlayerManager::Find_Inactive_Player(const WideStringClass& oNickname)
{
   if (oNickname.Is_Empty())
      return 0;

   uint32 uLength = oNickname.Get_Length() + 1;

   for (SLNode<cPlayer>* oNode = PlayerList.Head(); oNode; oNode = oNode->Next())
   {
      cPlayer* oPlayer = oNode->Data();

      if (!oPlayer->IsActive && !_wcsnicmp(oPlayer->PlayerName.Peek_Buffer(), oNickname, uLength))
         return oPlayer;
   }

   return 0;
}



cPlayer* cPlayerManager::Find_Player(int clientId)
{
   for (SLNode<cPlayer>* node = PlayerList.Head(); node; node = node->Next())
   {
      cPlayer* player = node->Data();
      if (player->IsActive && player->PlayerId == clientId)
         return player;
   }

   return 0;
}



cPlayer* cPlayerManager::Find_Player(const WideStringClass& oNickname)
{
   if (oNickname.Is_Empty())
      return 0;

   uint32 uLength = oNickname.Get_Length();

   for (SLNode<cPlayer>* oNode = PlayerList.Head(); oNode; oNode = oNode->Next())
   {
      cPlayer* oPlayer = oNode->Data();

      if (oPlayer->IsActive && !_wcsnicmp (oPlayer->PlayerName.Peek_Buffer(), oNickname, uLength + 1))
         return oPlayer;
   }

   return 0;
}


bool cPlayerManager::Load(ChunkLoadClass& chunkLoader)
{
	while (chunkLoader.Open_Chunk())
	{
		if (chunkLoader.Cur_Chunk_ID() == 0x12CDD9)
		{
			while (chunkLoader.Open_Chunk())
			{
				if (chunkLoader.Cur_Chunk_ID() == 0x12CDDA)
				{
					cPlayer* player = new cPlayer();
					player->Load(chunkLoader);
					if (cGameType::GameType != 1)
						cPlayerManager::Remove(player);
				}

				chunkLoader.Close_Chunk();
			}
		}

		chunkLoader.Close_Chunk();
	}

	return true;
}



void cPlayerManager::Add(cPlayer* player)
{
	PlayerList.Add_Tail(player);
	
	TypedActionPtr<PLAYERMGR_ACTION, cPlayer> action(PLAYERMGR_ACTION_ADD, player);
	mNotifier.NotifyObservers(action);
}



void cPlayerManager::Remove(cPlayer* player)
{
	PlayerList.Remove(player);
	
	TypedActionPtr<PLAYERMGR_ACTION, cPlayer> action(PLAYERMGR_ACTION_REMOVE, player);
	mNotifier.NotifyObservers(action);
}


REF_DEF1(cPlayerManager::PTextRenderer, Render2DClass *, 0x0081DC00);
extern bool RenderHud;
extern bool HidePlayerList;
void cPlayerManager::Render()
{
	if (RenderHud && PTextRenderer && !HidePlayerList)
	{
		PTextRenderer->Render();
	}
}



void cPlayerManager::Reset_Players()
{
	Remove_Inactive();
	for (SLNode<cPlayer>* playerNode = PlayerList.Head(); playerNode; playerNode = playerNode->Next())
		playerNode->Data()->Reset_Player();
}



void cPlayerManager::Remove_Inactive()
{
	for (SLNode<cPlayer>* playerNode = PlayerList.Head(); playerNode;)
	{
		cPlayer* player = playerNode->Data();
		playerNode = playerNode->Next();

		if (!player->IsActive)
		{
			PlayerList.Remove(player);
			delete player;
		}
	}
}



RENEGADE_FUNCTION
UNK cPlayerManager::Onetime_Init()
AT2(0x004156F0, 0x004156F0);



RENEGADE_FUNCTION
UNK cPlayerManager::Onetime_Shutdown()
AT2(0x00415760, 0x00415760);



RENEGADE_FUNCTION
void cPlayerManager::Think()
AT2(0x004157B0, 0x004157B0);



RENEGADE_FUNCTION
cPlayer* cPlayerManager::Find_Team_Player(int)
AT2(0x004158B0, 0x004158B0);



RENEGADE_FUNCTION
cPlayer* cPlayerManager::Find_Random_Team_Player(int)
AT2(0x00415B40, 0x00415B40);



RENEGADE_FUNCTION
cPlayer* cPlayerManager::Find_Team_Mate(cPlayer*)
AT2(0x00415E60, 0x00415E60);



RENEGADE_FUNCTION
cPlayer* cPlayerManager::Find_Clan_Mate(cPlayer*)
AT2(0x00416160, 0x00416160);



RENEGADE_FUNCTION
bool cPlayerManager::Is_Player_Present(int)
AT2(0x004161A0, 0x004161A0);



RENEGADE_FUNCTION
bool cPlayerManager::Is_Player_Present(WideStringClass&)
AT2(0x004161E0, 0x004161E0);



RENEGADE_FUNCTION
UNK cPlayerManager::Get_Player_Name(int)
AT2(0x00416230, 0x00416230);



RENEGADE_FUNCTION
UNK cPlayerManager::Deactivated(cPlayer*)
AT2(0x004163D0, 0x004163D0);



RENEGADE_FUNCTION
UNK cPlayerManager::Activated(cPlayer*)
AT2(0x00416420, 0x00416420);



RENEGADE_FUNCTION
UNK cPlayerManager::Get_Average_Ladder_Points()
AT2(0x00416470, 0x00416470);



RENEGADE_FUNCTION
UNK cPlayerManager::Get_Average_WOL_Points()
AT2(0x00416720, 0x00416720);



RENEGADE_FUNCTION
UNK cPlayerManager::Get_Average_Games_Played()
AT2(0x00416770, 0x00416770);



RENEGADE_FUNCTION
UNK cPlayerManager::Get_Average_Ping()
AT2(0x004167C0, 0x004167C0);



RENEGADE_FUNCTION
UNK cPlayerManager::Get_Average_FPS()
AT2(0x00416820, 0x00416820);



RENEGADE_FUNCTION
void cPlayerManager::Compute_Ladder_Points(int)
AT2(0x00416870, 0x00416870);



RENEGADE_FUNCTION
void cPlayerManager::Increment_Player_Times()
AT2(0x00416CA0, 0x00416CA0);



RENEGADE_FUNCTION
WideStringClass cPlayerManager::Determine_Mvp_Name()
AT2(0x00416CD0, 0x00416CD0);



RENEGADE_FUNCTION
UNK cPlayerManager::Compute_Full_Player_List_Height()
AT2(0x00416E60, 0x00416E60);



RENEGADE_FUNCTION
void cPlayerManager::Remove_All()
AT2(0x00416F80, 0x00416F80);



RENEGADE_FUNCTION
int cPlayerManager::Count()
AT2(0x00417040, 0x00417040);



RENEGADE_FUNCTION
UNK cPlayerManager::Tally_Team_Size(int)
AT2(0x004170D0, 0x004170D0);



RENEGADE_FUNCTION
bool cPlayerManager::Is_Kill_Treasonous(cPlayer*, cPlayer*)
AT2(0x00417370, 0x00417370);



RENEGADE_FUNCTION
void cPlayerManager::Sort_Players(bool)
AT2(0x00417830, 0x00417830);



RENEGADE_FUNCTION
UNK cPlayerManager::Player_Compare(const void*, const void*)
AT2(0x00417960, 0x00417960);



RENEGADE_FUNCTION
UNK cPlayerManager::Fast_Player_Compare(const void*, const void*)
AT2(0x00417C30, 0x00417C30);



RENEGADE_FUNCTION
UNK cPlayerManager::Compute_Fast_Sort_Key(cPlayer*)
AT2(0x00417C5A, 0x00417C60);



RENEGADE_FUNCTION
void cPlayerManager::Construct_Heading(WideStringClass&, bool)
AT2(0x00417EB0, 0x00417EB0);



RENEGADE_FUNCTION
UNK cPlayerManager::List_Print(WideStringClass&, Vector3)
AT2(0x00418290, 0x00418290);



RENEGADE_FUNCTION
UNK cPlayerManager::Line(float, float, int)
AT2(0x00418380, 0x00418380);



RENEGADE_FUNCTION
UNK cPlayerManager::Render_Player_List()
AT2(0x004183E0, 0x004183E0);



void cPlayerManager::Log_Player_List()
{
	StringClass filename;
	filename.Format("results%d.txt",cUserOptions::ResultsLogNumber.Data);
	FILE *f = fopen(filename,"at");
	if (f)
	{
		char temp[2000];
		memset(temp,0,sizeof(temp));
		WideStringClass str(0,true);
		cPlayerManager::Construct_Heading(str,true);
		StringClass str2;
		str2.Copy_Wide(str);
		sprintf(temp,"%s\n",(const char *)str2);
		fwrite(temp,1,strlen(temp),f);
		memset(temp,0,sizeof(temp));
		for (int i = 0;i < 255;i++)
		{
			if (Player_Array[i])
			{
				Player_Array[i]->Get_Player_String(i+1,str,true);
				str2.Copy_Wide(str);
				sprintf(temp,"%s\n",(const char *)str2);
				fwrite(temp,1,strlen(temp),f);
				memset(temp,0,sizeof(temp));
			}
		}
		sprintf(temp, "\n");
		fwrite(temp,1,strlen(temp),f);
		fclose(f);
	}
}

RENEGADE_FUNCTION
bool cPlayerManager::Save(ChunkSaveClass&)
AT2(0x00418CB0, 0x00418CB0);
