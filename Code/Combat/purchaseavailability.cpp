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

#include "purchaseavailability.h"

#include "apppackettypes.h"
#include "building.h"
#include "combat.h"
#include "gameobjmanager.h"
#include "playertype.h"
#include "ssgmsettings.h"

#include <string.h>


unsigned char	PurchaseAvailabilityClass::PageFlags
		[PurchaseSettingsDefClass::TYPE_COUNT]
		[PurchaseSettingsDefClass::TEAM_COUNT]
		[PurchaseAvailabilityClass::ENTRY_COUNT] = { { { 0 } } };

unsigned char	PurchaseAvailabilityClass::EnlistedFlags
		[TeamPurchaseSettingsDefClass::TEAM_COUNT]
		[PurchaseAvailabilityClass::ENLISTED_COUNT] = { { 0 } };

int				PurchaseAvailabilityClass::TechLevel = 0;


//
//	One instance, with a fixed network ID, so both ends already have the
//	object and only its contents ever travel.
//
static PurchaseAvailabilityClass	_ThePurchaseAvailability;


PurchaseAvailabilityClass::PurchaseAvailabilityClass (void)
{
	Set_Network_ID (NETID_SERVER_PURCHASE_AVAILABILITY);
	Set_App_Packet_Type (APPPACKETTYPE_NETPURCHASEAVAILABILITY);
}


void	PurchaseAvailabilityClass::Reset (void)
{
	::memset (PageFlags, 0, sizeof (PageFlags));
	::memset (EnlistedFlags, 0, sizeof (EnlistedFlags));

	//	The match starts wherever the server layer says it starts.
	TechLevel = SSGMSettingsClass::TechLevel;

	Changed ();
}


void	PurchaseAvailabilityClass::Set_Tech_Level (int level)
{
	if ((level == TechLevel) || !CombatManager::I_Am_Server ()) {
		return ;
	}

	TechLevel = level;
	Changed ();
}


void	PurchaseAvailabilityClass::Changed (void)
{
	if (CombatManager::I_Am_Server ()) {
		_ThePurchaseAvailability.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	}
}


unsigned char	PurchaseAvailabilityClass::Get_Flags (PurchaseSettingsDefClass::TYPE type,
		PurchaseSettingsDefClass::TEAM team, int index)
{
	if ((type < 0) || (type >= PurchaseSettingsDefClass::TYPE_COUNT))	{ return 0; }
	if ((team < 0) || (team >= PurchaseSettingsDefClass::TEAM_COUNT))	{ return 0; }
	if ((index < 0) || (index >= ENTRY_COUNT))								{ return 0; }

	unsigned char flags = PageFlags[type][team][index];

	//
	//	An entry the match has not teched up to is greyed out whatever the
	//	flags say.  This is read on both sides, so both have to agree; the
	//	tech level replicates and the definition is the same everywhere.
	//
	PurchaseSettingsDefClass *page = PurchaseSettingsDefClass::Find_Definition (type, team);
	if ((page != nullptr) && (page->Get_Tech_Level (index) > TechLevel)) {
		flags |= FLAG_DISABLED;
	}

	return flags;
}


unsigned char	PurchaseAvailabilityClass::Get_Enlisted_Flags (
		TeamPurchaseSettingsDefClass::TEAM team, int index)
{
	if ((team < 0) || (team >= TeamPurchaseSettingsDefClass::TEAM_COUNT))	{ return 0; }
	if ((index < 0) || (index >= ENLISTED_COUNT))								{ return 0; }

	return EnlistedFlags[team][index];
}


//
//	Scripts name a side the way everything else in this engine does, with a
//	PLAYERTYPE.  The purchase pages number their two sides from zero.
//
PurchaseSettingsDefClass::TEAM	PurchaseAvailabilityClass::Page_Team (int player_type)
{
	return (player_type == PLAYERTYPE_NOD)
			? PurchaseSettingsDefClass::TEAM_NOD
			: PurchaseSettingsDefClass::TEAM_GDI;
}


static void	Apply (unsigned char &flags, unsigned char flag, bool on)
{
	if (on) {
		flags = (unsigned char)(flags | flag);
	} else {
		flags = (unsigned char)(flags & ~flag);
	}
}


void	PurchaseAvailabilityClass::Set_By_Definition (int player_type, int definition_id,
		unsigned char flag, bool on)
{
	if (definition_id == 0) {
		return ;
	}

	PurchaseSettingsDefClass::TEAM		page_team	= Page_Team (player_type);
	TeamPurchaseSettingsDefClass::TEAM	team_team	= (player_type == PLAYERTYPE_NOD)
			? TeamPurchaseSettingsDefClass::TEAM_NOD
			: TeamPurchaseSettingsDefClass::TEAM_GDI;

	//
	//	The four characters a team starts with are not on a purchase page.
	//
	TeamPurchaseSettingsDefClass *enlisted = TeamPurchaseSettingsDefClass::Get_Definition (team_team);
	if (enlisted != nullptr) {
		for (int index = 0; index < ENLISTED_COUNT; index ++) {
			if (enlisted->Get_Enlisted_Definition (index) == definition_id) {
				Apply (EnlistedFlags[team_team][index], flag, on);
			}
		}
	}

	//
	//	Every page that offers it.  An alternate skin is the same entry, so
	//	naming one takes the whole entry off the menu, which is what the
	//	donor did.
	//
	for (int type = 0; type < PurchaseSettingsDefClass::TYPE_COUNT; type ++) {

		PurchaseSettingsDefClass *page = PurchaseSettingsDefClass::Find_Definition (
				(PurchaseSettingsDefClass::TYPE)type, page_team);

		if (page == nullptr) { continue; }

		for (int index = 0; index < ENTRY_COUNT; index ++) {

			bool named = (page->Get_Definition (index) == definition_id);

			for (int alt = 0; !named && (alt < 3); alt ++) {
				named = (page->Get_Alt_Definition (index, alt) == definition_id);
			}

			if (named) {
				Apply (PageFlags[type][page_team][index], flag, on);
			}
		}
	}

	Changed ();
}


void	PurchaseAvailabilityClass::Set_By_Factory (int player_type, int building_type,
		unsigned char flag, bool on)
{
	PurchaseSettingsDefClass::TEAM	page_team = Page_Team (player_type);

	for (int type = 0; type < PurchaseSettingsDefClass::TYPE_COUNT; type ++) {

		int needed = BuildingConstants::TYPE_NONE;

		switch (type) {
			case PurchaseSettingsDefClass::TYPE_CLASSES:
			case PurchaseSettingsDefClass::TYPE_SECRET_CLASSES:
				needed = BuildingConstants::TYPE_SOLDIER_FACTORY;
				break;

			case PurchaseSettingsDefClass::TYPE_VEHICLES:
			case PurchaseSettingsDefClass::TYPE_SECRET_VEHICLES:
				needed = BuildingConstants::TYPE_VEHICLE_FACTORY;
				break;

			default:
				//	Equipment comes out of the terminal itself.
				break;
		}

		if (needed != building_type) { continue; }

		PurchaseSettingsDefClass *page = PurchaseSettingsDefClass::Find_Definition (
				(PurchaseSettingsDefClass::TYPE)type, page_team);

		if (page == nullptr) { continue; }

		for (int index = 0; index < ENTRY_COUNT; index ++) {
			if (page->Get_Definition (index) != 0) {
				Apply (PageFlags[type][page_team][index], flag, on);
			}
		}
	}

	Changed ();
}


void	PurchaseAvailabilityClass::Export_Rare (BitStreamClass &packet)
{
	for (int type = 0; type < PurchaseSettingsDefClass::TYPE_COUNT; type ++) {
		for (int team = 0; team < PurchaseSettingsDefClass::TEAM_COUNT; team ++) {
			for (int index = 0; index < ENTRY_COUNT; index ++) {
				packet.Add (PageFlags[type][team][index]);
			}
		}
	}

	for (int team = 0; team < TeamPurchaseSettingsDefClass::TEAM_COUNT; team ++) {
		for (int index = 0; index < ENLISTED_COUNT; index ++) {
			packet.Add (EnlistedFlags[team][index]);
		}
	}

	packet.Add (TechLevel);
}


void	PurchaseAvailabilityClass::Import_Rare (BitStreamClass &packet)
{
	for (int type = 0; type < PurchaseSettingsDefClass::TYPE_COUNT; type ++) {
		for (int team = 0; team < PurchaseSettingsDefClass::TEAM_COUNT; team ++) {
			for (int index = 0; index < ENTRY_COUNT; index ++) {
				packet.Get (PageFlags[type][team][index]);
			}
		}
	}

	for (int team = 0; team < TeamPurchaseSettingsDefClass::TEAM_COUNT; team ++) {
		for (int index = 0; index < ENLISTED_COUNT; index ++) {
			packet.Get (EnlistedFlags[team][index]);
		}
	}

	packet.Get (TechLevel);
}
