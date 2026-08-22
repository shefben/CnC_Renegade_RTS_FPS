#include "General.h"
#include "CampaignManager.h"



RENEGADE_FUNCTION
void CampaignManager::Init()
AT2(0x0041F2D0, 0x0041F2D0);



RENEGADE_FUNCTION
void CampaignManager::Shutdown()
AT2(0x0041F610, 0x0041F610);



RENEGADE_FUNCTION
bool CampaignManager::Save(ChunkSaveClass&)
AT2(0x0041F670, 0x0041F670);



RENEGADE_FUNCTION
bool CampaignManager::Load(ChunkLoadClass&)
AT2(0x0041F6D0, 0x0041F6D0);



RENEGADE_FUNCTION
void CampaignManager::Start_Campaign(int)
AT2(0x0041F750, 0x0041F750);



RENEGADE_FUNCTION
void CampaignManager::Continue(bool)
AT2(0x0041F7D0, 0x0041F7D0);



RENEGADE_FUNCTION
void CampaignManager::Reset()
AT2(0x0041FC80, 0x0041FC80);



RENEGADE_FUNCTION
void CampaignManager::Replay_Level(const char*, int)
AT2(0x0041FC90, 0x0041FC90);



RENEGADE_FUNCTION
uint CampaignManager::Get_Backdrop_Description_Count()
AT2(0x0041FCC0, 0x0041FCC0);



RENEGADE_FUNCTION
const char* CampaignManager::Get_Backdrop_Description(int)
AT2(0x0041FCF0, 0x0041FCF0);



RENEGADE_FUNCTION
void CampaignManager::Select_Backdrop_Number(int)
AT2(0x0041FD10, 0x0041FD10);



RENEGADE_FUNCTION
void CampaignManager::Select_Backdrop_Number_By_MP_Type(int)
AT2(0x0041FD40, 0x0041FD40);
