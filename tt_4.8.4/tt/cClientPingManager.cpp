#include "General.h"
#include "cClientPingManager.h"



RENEGADE_FUNCTION
void cClientPingManager::Init()
AT2(0x00456720, 0x00456830);



RENEGADE_FUNCTION
void cClientPingManager::Think()
AT2(0x00456750, 0x00456860);



RENEGADE_FUNCTION
int cClientPingManager::Get_Last_Round_Trip_Ping_Ms()
AT2(0x00456840, 0x00456950);



RENEGADE_FUNCTION
int cClientPingManager::Get_Avg_Round_Trip_Ping_Ms()
AT2(0x00456850, 0x00456960);



RENEGADE_FUNCTION
void cClientPingManager::Compute_Average_Round_Trip_Ping_Ms()
AT2(0x00456860, 0x00456970);



RENEGADE_FUNCTION
void cClientPingManager::Response_Received(int)
AT2(0x004568C0, 0x004569D0);
