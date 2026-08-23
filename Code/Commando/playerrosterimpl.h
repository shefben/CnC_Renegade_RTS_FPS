/******************************************************************************
*
* FILE
*     playerrosterimpl.h
*
* DESCRIPTION
*     Commando's answer to Combat's PlayerRosterInterfaceClass.
*     See playerroster.h.
*
******************************************************************************/

#ifndef __PLAYERROSTERIMPL_H__
#define __PLAYERROSTERIMPL_H__

#include "playerroster.h"

class	PlayerRosterImplClass : public PlayerRosterInterfaceClass
{
public:

	static void					Register (void);

	const unichar_t *			Get_Player_Name (int player_id) override;
	int							Get_Player_Type (int player_id) override;
	int							Get_Player_Count (void) override;
	int							Get_Player_ID_By_Index (int index) override;
};

#endif	// __PLAYERROSTERIMPL_H__
