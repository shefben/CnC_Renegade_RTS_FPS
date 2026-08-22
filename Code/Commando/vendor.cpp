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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commmando																	  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/vendor.cpp       $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/28/02 11:17a                                               $*
 *                                                                                             *
 *                    $Revision:: 23                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "vendor.h"
#include "ttsettings.h"
#include "gameeventbus.h"
#include "gameobjmanager.h"
#include "playerdata.h"
#include "soldier.h"
#include "playertype.h"
#include "combat.h"
#include "powerup.h"
#include "refinerygameobj.h"
#include "vehiclefactorygameobj.h"
#include "definitionmgr.h"
#include "vehicle.h"
#include "combatchunkid.h"
#include "cnetwork.h"
#include "purchaserequestevent.h"
#include "purchasesettings.h"
#include "teampurchasesettings.h"
#include "weaponbag.h"
#include "weapons.h"
#include "purchaseresponseevent.h"


////////////////////////////////////////////////////////////////
//	Namespaces
////////////////////////////////////////////////////////////////
using namespace BuildingConstants;


////////////////////////////////////////////////////////////////
//
//	Purchase_Category_Of
//
////////////////////////////////////////////////////////////////
static PurchaseCategoryEnum
Purchase_Category_Of (VendorClass::PURCHASE_TYPE type)
{
	if (type == VendorClass::TYPE_VEHICLE || type == VendorClass::TYPE_SECRET_VEHICLE) {
		return PURCHASE_CATEGORY_VEHICLE;
	}

	if (type == VendorClass::TYPE_BEACON) {
		return PURCHASE_CATEGORY_POWERUP;
	}

	//
	//	Characters, enlisted characters, secret characters and supply refills
	//	all come out of the soldier factory.
	//
	return PURCHASE_CATEGORY_CHARACTER;
}


////////////////////////////////////////////////////////////////
//
//	Purchase_Status_To_Error
//
// The purchase event lets a subscriber answer with the refusal the player
// should be shown.  Those refusal values were derived from PURCHASE_ERROR and
// are numerically identical to it; converting rather than casting keeps that
// a stated fact instead of an assumption.
//
////////////////////////////////////////////////////////////////
static VendorClass::PURCHASE_ERROR
Purchase_Status_To_Error (PurchaseStatusEnum status)
{
	switch (status) {
		case PURCHASE_STATUS_GRANTED:					return VendorClass::PERR_SUCCESS;
		case PURCHASE_STATUS_PENDING:					return VendorClass::PERR_OPERATION_PENDING;
		case PURCHASE_STATUS_INSUFFICIENT_FUNDS:	return VendorClass::PERR_NO_FUNDS;
		case PURCHASE_STATUS_FACTORY_UNAVAILABLE:	return VendorClass::PERR_NO_FACTORY;
		case PURCHASE_STATUS_OUT_OF_STOCK:			return VendorClass::PERR_NOT_IN_STOCK;
		default:												return VendorClass::PERR_UNKNOWN;
	}
}


////////////////////////////////////////////////////////////////
//
//	Purchase_Vehicle
//
////////////////////////////////////////////////////////////////
VendorClass::PURCHASE_ERROR
VendorClass::Purchase_Vehicle
(
	BaseControllerClass *	base,
	SoldierGameObj *			player,
	int							cost,
	int							vehicle_id
)
{
	PURCHASE_ERROR retval = PERR_NOT_IN_STOCK;

	//
	//	Give anything watching purchases the chance to refuse this one, or to
	//	let it through free or without a vehicle coming out of the factory.
	//
	PurchaseStatusEnum status = GameEventBus::Raise_Purchase (PURCHASE_CATEGORY_VEHICLE,
			base, player, (unsigned int)cost, (unsigned int)vehicle_id);

	if (status >= PURCHASE_STATUS_GRANTED) {
		return Purchase_Status_To_Error (status);
	}

	//
	//	Check to see if this vehicle is available to build
	//
	VehicleGameObjDef *definition = nullptr;
	definition = (VehicleGameObjDef *)DefinitionMgrClass::Find_Definition (vehicle_id);
	if (definition != nullptr) {

		PlayerDataClass *player_data	= nullptr;
		bool has_funds						= true;

		//
		//	Check to see if the player has sufficient funds to purchase the vehicle
		//
		if (player != nullptr && player->Get_Player_Data () != nullptr) {
			player_data	= player->Get_Player_Data ();
			has_funds	= (player_data->Get_Money () >= cost);
		}

		if (has_funds) {
			retval = PERR_NO_FACTORY;

			//
			//	Find our vehicle factory
			//
			BuildingGameObj *building = base->Find_Building (TYPE_VEHICLE_FACTORY);
			if (building != nullptr) {
				VehicleFactoryGameObj *factory = building->As_VehicleFactoryGameObj ();

				//
				//	If we have a vehicle factory that isn't busy, then start building the vehicle
				//
				if (factory->Is_Available ()) {

					//
					//	The purchase succeeds either way; a no-spawn purchase
					//	simply does not queue anything on the factory.
					//
					if (status != PURCHASE_STATUS_ALLOW_NO_SPAWN) {

						//
						//	The base's operation time factor is a flat 2 while
						//	the power plant is down.  BuildTimeDelay is that
						//	same multiplier, made a server setting; it defaults
						//	to 2.
						//
						float time = 5.0F;
						if (base->Is_Base_Powered () == false) {
							time *= TTSettingsClass::BuildTimeDelay;
						}

						factory->Request_Vehicle (vehicle_id, time, player);
					}

					retval = PERR_SUCCESS;

					//
					//	If a player is purcahsing the vehicle, then debit
					// the player's account
					//
					if (player_data != nullptr && status != PURCHASE_STATUS_ALLOW_FREE) {
						player_data->Purchase_Item (cost);
					}
				}
			}
		} else {
			retval = PERR_NO_FUNDS;
		}
	}

	return retval;
}


////////////////////////////////////////////////////////////////
//
//	Purchase_Powerup
//
////////////////////////////////////////////////////////////////
VendorClass::PURCHASE_ERROR
VendorClass::Purchase_Powerup
(
	BaseControllerClass *	base,
	SoldierGameObj *			player,
	int							cost,
	int							powerup_id
)
{
	PURCHASE_ERROR retval = PERR_NO_FUNDS;

	//
	//	Sanity check
	//
	if (player == nullptr) {
		return retval;
	}

	PurchaseStatusEnum status = GameEventBus::Raise_Purchase (PURCHASE_CATEGORY_POWERUP,
			base, player, (unsigned int)cost, (unsigned int)powerup_id);

	if (status >= PURCHASE_STATUS_GRANTED) {
		return Purchase_Status_To_Error (status);
	}

	//
	//	Lookup the powerup's definition
	//
	DefinitionClass *definition = DefinitionMgrClass::Find_Definition (powerup_id);
	if (definition != nullptr && definition->Get_Class_ID () == CLASSID_GAME_OBJECT_DEF_POWERUP) {
		PowerUpGameObjDef *powerup_def = reinterpret_cast<PowerUpGameObjDef *> (definition);

		//
		//	If a player is purcahsing the powerup, then debit
		// the player's account
		//
		bool has_funds = (status == PURCHASE_STATUS_ALLOW_FREE);
		PlayerDataClass *player_data = player->Get_Player_Data ();
		if (player_data != nullptr && status != PURCHASE_STATUS_ALLOW_FREE) {

			//
			//	Try to purchase the powerup
			//
			has_funds = player_data->Purchase_Item (cost);
		}

		//
		//	If the player has the cash, then grant him the powerup
		//
		if (has_funds) {
			powerup_def->Grant (player);
			retval = PERR_SUCCESS;
		}

	} else {
		retval = PERR_NOT_IN_STOCK;
	}

	return retval;
}


////////////////////////////////////////////////////////////////
//
//	Purchase_Character
//
////////////////////////////////////////////////////////////////
VendorClass::PURCHASE_ERROR
VendorClass::Purchase_Character
(
	BaseControllerClass *	base,
	SoldierGameObj *			player,
	int							cost,
	int							definition_id
)
{
	PURCHASE_ERROR retval = PERR_NO_FUNDS;

	//
	//	Sanity check
	//
	if (player == nullptr) {
		return retval;
	}

	PurchaseStatusEnum status = GameEventBus::Raise_Purchase (PURCHASE_CATEGORY_CHARACTER,
			base, player, (unsigned int)cost, (unsigned int)definition_id);

	if (status >= PURCHASE_STATUS_GRANTED) {
		return Purchase_Status_To_Error (status);
	}

	//
	//	Check to see if the player has sufficient funds to purchase the upgrade
	//
	PlayerDataClass *player_data = player->Get_Player_Data ();
	bool has_funds = (status == PURCHASE_STATUS_ALLOW_FREE) || (player_data->Get_Money () >= cost);
	if (has_funds) {
		retval = PERR_NO_FACTORY;

		//
		//	Check to see if our soldier factory is operational
		//
		BuildingGameObj *building = base->Find_Building (TYPE_SOLDIER_FACTORY);
		if ((building != nullptr && building->Is_Destroyed () == false) || cost == 0) {

			//
			//	Lookup the new definition for the soldier
			//
			DefinitionClass *definition = DefinitionMgrClass::Find_Definition (definition_id);
			if (definition != nullptr && definition->Get_Class_ID () == CLASSID_GAME_OBJECT_DEF_SOLDIER) {

				//
				//	Upgrade the player
				//
				SoldierGameObjDef *soldier_def = reinterpret_cast<SoldierGameObjDef *> (definition);
				player->Re_Init (*soldier_def);
				player->Post_Re_Init();
				retval = PERR_SUCCESS;

				//
				//	Debit the player's account
				//
				if (status != PURCHASE_STATUS_ALLOW_FREE) {
					player_data->Purchase_Item (cost);
				}
			}
		}
	}

	return retval;
}


////////////////////////////////////////////////////////////////
//
//	Purchase_Powerup
//
////////////////////////////////////////////////////////////////
VendorClass::PURCHASE_ERROR
VendorClass::Purchase_Item
(
	SoldierGameObj *	player,
	PURCHASE_TYPE		type,
	int					item_index,
	int					alt_skin_index,
	bool					is_from_server
)
{
	PURCHASE_ERROR retval = PERR_NO_FUNDS;

	//
	//	Sanity check
	//
	if (player == nullptr) {
		return retval;
	}

	if (CombatManager::I_Am_Server () == false) {

		//
		//	Request this purchase from the server
		//
		cPurchaseRequestEvent *purchase_request = new cPurchaseRequestEvent;
		purchase_request->Init (type, item_index, alt_skin_index);

		//
		//	Let the user know the operation is pending
		//
		retval = PERR_OPERATION_PENDING;

	} else {

		//
		//	Determine which base controller to purchase from
		//
		BaseControllerClass *base = nullptr;
		if (player->Get_Player_Type () == PLAYERTYPE_NOD) {
			base = BaseControllerClass::Find_Base ( PLAYERTYPE_NOD );
		} else {
			base = BaseControllerClass::Find_Base ( PLAYERTYPE_GDI );
		}

		//
		//	Lookup information about this purchase
		//
		int cost					= 0;
		int definition_id		= 0;
		Get_Merchandise_Information (player, type, item_index, alt_skin_index, cost, definition_id);

		//
		//	Cost is doubled if the base isn't powered
		//
		//if (base != nullptr && base->Is_Base_Powered () == false) {
		//
	//	Unpowered bases pay double, unless the server has switched that off
	//
	if (	type != TYPE_BEACON &&
			base != nullptr &&
			base->Is_Base_Powered () == false &&
			TTSettingsClass::DisableCostMultiplier == false)
	{
			cost = cost * 2;
		}

		if (type == TYPE_CHARACTER || type == TYPE_ENLISTED_CHARACTER || type == TYPE_SECRET_CHARACTER) {

			//
			//	Purchase the character
			//
			retval = Purchase_Character (base, player, cost, definition_id);
		} else if (type == TYPE_VEHICLE || type == TYPE_SECRET_VEHICLE) {

			//
			//	Purchase the vehicle
			//
			retval = Purchase_Vehicle (base, player, cost, definition_id);
		} else if (type == TYPE_BEACON) {

			//
			//	Purchase the beacon powerup
			//
			retval = Purchase_Powerup (base, player, cost, definition_id);
		} else if (type == TYPE_SUPPLY) {

			//
			//	Grant full health, armor and ammo
			//
			if (GameEventBus::Raise_Refill (player)) {
				Grant_Supplies (player);
				retval = PERR_SUCCESS;
			} else {
				retval = PERR_NOT_IN_STOCK;
			}
		}

		//
		//	Whatever the purchase path decided, anything monitoring purchases
		//	hears about it here.  This is observation only; the outcome was
		//	settled above.
		//
		GameEventBus::Raise_Purchase_Complete (Purchase_Category_Of (type), base, player,
				(unsigned int)cost, (unsigned int)definition_id, (int)retval);

		//
		//	Send the response to the server locally as necessary
		//
		if (is_from_server) {
			cPurchaseResponseEvent *event = new cPurchaseResponseEvent;
			event->Init ((int)retval, cNetwork::Get_My_Id ());
		}
	}

	return retval;
}


////////////////////////////////////////////////////////////////
//
//	Grant_Supplies
//
////////////////////////////////////////////////////////////////
void
VendorClass::Grant_Supplies (SoldierGameObj *player)
{
	//
	//	Grant ammo
	//
	WeaponBagClass *weapon_bag = player->Get_Weapon_Bag ();
	for (int weapon_index = 0; weapon_index < weapon_bag->Get_Count (); weapon_index ++) {
		WeaponClass	*weapon = weapon_bag->Peek_Weapon (weapon_index);
		if (weapon != nullptr && weapon->Get_Definition ()->CanReceiveGenericCnCAmmo) {

			//
			//	Restore full ammo
			//
			weapon->Set_Inventory_Rounds (weapon->Get_Definition ()->MaxInventoryRounds);
			weapon->Set_Clip_Rounds (weapon->Get_Definition ()->ClipSize);
		}
	}

	//
	//	Grant full health and armor
	//
	DefenseObjectClass *defense_obj = player->Get_Defense_Object ();
	defense_obj->Set_Health (defense_obj->Get_Health_Max ());
	defense_obj->Set_Shield_Strength (defense_obj->Get_Shield_Strength_Max ());
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Merchandise_Information
//
////////////////////////////////////////////////////////////////
void
VendorClass::Get_Merchandise_Information
(
	SoldierGameObj *	player,
	PURCHASE_TYPE		type,
	int					item_index,
	int					alt_skin_index,
	int &					cost,
	int &					definition_id
)
{
	//
	//	Determine which team to use...
	//
	PurchaseSettingsDefClass::TEAM team = PurchaseSettingsDefClass::TEAM_GDI;
	if (player->Get_Player_Type () == PLAYERTYPE_NOD) {
		team = PurchaseSettingsDefClass::TEAM_NOD;
	}

	//
	//	Lookup the definition
	//
	if (type == TYPE_CHARACTER || type == TYPE_VEHICLE) {

		//
		//	Determine which purchase type to use
		//
		PurchaseSettingsDefClass::TYPE purchase_type = PurchaseSettingsDefClass::TYPE_CLASSES;
		if (type == TYPE_VEHICLE) {
			purchase_type = PurchaseSettingsDefClass::TYPE_VEHICLES;
		}

		//
		//	Lookup the information from this purchase definition
		//
		PurchaseSettingsDefClass *definition = PurchaseSettingsDefClass::Find_Definition (purchase_type, team);
		if (definition != nullptr) {
			cost = definition->Get_Cost (item_index);

			//
			//	Either get the default skin or the alternate skin
			//
			if (alt_skin_index != -1) {
				definition_id	= definition->Get_Alt_Definition (item_index, alt_skin_index);
			} else {
				definition_id	= definition->Get_Definition (item_index);
			}
		}

	} else if (type == TYPE_SECRET_CHARACTER || type == TYPE_SECRET_VEHICLE) {

		//
		// Only allow cheats in non-laddered games!
		//
		if (The_Game()->IsLaddered.Is_False()) {

			//
			//	Determine which purchase type to use
			//
			PurchaseSettingsDefClass::TYPE purchase_type = PurchaseSettingsDefClass::TYPE_SECRET_CLASSES;
			if (type == TYPE_SECRET_VEHICLE) {
				purchase_type = PurchaseSettingsDefClass::TYPE_SECRET_VEHICLES;
			}

			//
			//	Lookup the information from this purchase definition
			//
			PurchaseSettingsDefClass *definition = PurchaseSettingsDefClass::Find_Definition (purchase_type, team);
			if (definition != nullptr) {
				cost				= definition->Get_Cost (item_index);
				definition_id	= definition->Get_Definition (item_index);
			}
		}

	} else if (type == TYPE_ENLISTED_CHARACTER) {

		//
		//	For enlisted characters, lookup the team purchase definition
		//
		cost = 0;
		TeamPurchaseSettingsDefClass *definition = TeamPurchaseSettingsDefClass::Get_Definition ((TeamPurchaseSettingsDefClass::TEAM)team);
		if (definition != nullptr) {
			definition_id = definition->Get_Enlisted_Definition (item_index);
		}

	} else if (type == TYPE_BEACON) {

		//
		//	For beacons, lookup the team purchase definition
		//
		TeamPurchaseSettingsDefClass *definition = TeamPurchaseSettingsDefClass::Get_Definition ((TeamPurchaseSettingsDefClass::TEAM)team);
		if (definition != nullptr) {
			cost				= definition->Get_Beacon_Cost ();
			definition_id	= definition->Get_Beacon_Definition ();
		}
	}

	return ;
}
