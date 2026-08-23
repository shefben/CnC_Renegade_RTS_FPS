/******************************************************************************
*
* FILE
*     gameinfoimpl.h
*
* DESCRIPTION
*     Commando's answer to Combat's GameInfoInterfaceClass.  See gameinfo.h.
*
******************************************************************************/

#ifndef __GAMEINFOIMPL_H__
#define __GAMEINFOIMPL_H__

#include "gameinfo.h"


class	GameInfoImplClass : public GameInfoInterfaceClass
{
public:

	//
	//	Called once at startup, after CombatManager::Init.
	//
	static void					Register (void);

	float							Get_Time_Remaining_Seconds (void) override;
	void							Set_Time_Remaining_Seconds (float seconds) override;
	int							Get_Time_Limit_Minutes (void) override;
	void							Set_Time_Limit_Minutes (int minutes) override;
	unsigned int				Get_Duration_Seconds (void) override;
	int							Get_Win_Type (void) override;
	int							Get_Winner_ID (void) override;
	int							Get_Max_Players (void) override;
	const char *				Get_Map_Name (void) override;
	const unichar_t *			Get_Title (void) override;
	float							Get_Team_Score (int player_type) override;
	int							Get_Team_Credits (int player_type) override;
};


#endif	// __GAMEINFOIMPL_H__
