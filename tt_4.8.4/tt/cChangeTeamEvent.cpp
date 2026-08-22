#include "General.h"
#include "cChangeTeamEvent.h"

#include "cNetwork.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "engine_game.h"

bool cChangeTeamEvent::Is_Change_Team_Possible()
{
	return
		cNetwork::I_Am_Client() &&
		GameModeManager::Find("Combat")->Is_Active() &&
		The_Game() && The_Game()->IsTeamChangingAllowed;
}
