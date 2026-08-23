/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/******************************************************************************
*
* FILE
*     purchaseavailability.h
*
* DESCRIPTION
*     Which purchase-terminal entries a team may see and buy right now.
*
*     The purchase pages themselves are definitions: a fixed list of ten
*     entries per page, loaded from the level and the same on every machine.
*     What changes during a game is whether an entry is on offer, and that
*     is not part of the definition -- it is state a script sets and every
*     client has to be told about, because the client draws its own menu.
*
*     Three independent flags per entry, which is what the 4.8.4 library
*     exposed and what its scripts set:
*
*       hidden    the entry is not drawn at all
*       disabled  the entry is drawn greyed out
*       busy      the same, but meaning "come back in a moment"
*
*     Any of the three refuses a purchase server-side as well, so a client
*     whose menu is momentarily stale cannot buy what it should not see.
*
******************************************************************************/

#ifndef	__PURCHASEAVAILABILITY_H
#define	__PURCHASEAVAILABILITY_H

#include "always.h"
#include "networkobject.h"
#include "purchasesettings.h"
#include "teampurchasesettings.h"


class	PurchaseAvailabilityClass : public NetworkObjectClass
{
public:

	enum
	{
		FLAG_HIDDEN		= 0x01,
		FLAG_DISABLED	= 0x02,
		FLAG_BUSY		= 0x04,

		//	The shape of a purchase page: ten entries, and four enlisted
		//	characters that are not on a page at all.
		ENTRY_COUNT		= 10,
		ENLISTED_COUNT	= 4
	};

	PurchaseAvailabilityClass (void);

	//	A static network object is never created or destroyed at runtime.
	void	Delete (void) override					{ }
	void	Set_Delete_Pending (void) override	{ }

	void	Export_Rare (BitStreamClass &packet) override;
	void	Import_Rare (BitStreamClass &packet) override;

	//	Everything is on offer again when a level starts.
	static void	Reset (void);

	//
	//	The tech level the match has reached.  An entry whose own tech level
	//	is higher than this reads as disabled, without anything having had
	//	to set a flag on it.  Server only for the setter; the value
	//	replicates with the flags.
	//
	static void	Set_Tech_Level (int level);
	static int	Get_Tech_Level (void)			{ return TechLevel; }

	//
	//	Asking.  An out-of-range index reads as available rather than as
	//	whatever is next in memory.
	//
	static unsigned char	Get_Flags (PurchaseSettingsDefClass::TYPE type,
			PurchaseSettingsDefClass::TEAM team, int index);

	static unsigned char	Get_Enlisted_Flags (TeamPurchaseSettingsDefClass::TEAM team, int index);

	static bool	Is_Available (PurchaseSettingsDefClass::TYPE type,
			PurchaseSettingsDefClass::TEAM team, int index)
		{ return Get_Flags (type, team, index) == 0; }

	static bool	Is_Enlisted_Available (TeamPurchaseSettingsDefClass::TEAM team, int index)
		{ return Get_Enlisted_Flags (team, index) == 0; }

	//
	//	Setting.  Server only, and `player_type` is a PLAYERTYPE, which is
	//	what every script-facing team argument in this engine is.
	//
	static void	Set_By_Definition (int player_type, int definition_id,
			unsigned char flag, bool on);

	//
	//	Everything a named building produces, at once.  The 4.8.4 library
	//	read a per-entry factory field off the purchase definition; there is
	//	no such field here, so the factory an entry needs is the one its page
	//	needs -- characters from the soldier factory, vehicles from the
	//	vehicle factory -- which is the same answer for every entry the
	//	shipped data names.
	//
	static void	Set_By_Factory (int player_type, int building_type,
			unsigned char flag, bool on);

private:

	static PurchaseSettingsDefClass::TEAM	Page_Team (int player_type);
	static void										Changed (void);

	static unsigned char	PageFlags[PurchaseSettingsDefClass::TYPE_COUNT]
										[PurchaseSettingsDefClass::TEAM_COUNT]
										[ENTRY_COUNT];

	static unsigned char	EnlistedFlags[TeamPurchaseSettingsDefClass::TEAM_COUNT]
										[ENLISTED_COUNT];

	static int				TechLevel;
};


#endif	//__PURCHASEAVAILABILITY_H
