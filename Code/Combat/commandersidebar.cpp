/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandersidebar.cpp                  $*
 *                                                                                             *
 *	Roadmap Section 42, Phase 36 -- Commander right-side UI + contextual production sidebar.     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandersidebar.h"

#include "building.h"
#include "buildingconstruction.h"
#include "buildingdescriptormanager.h"
#include "commanderproduction.h"
#include "playertype.h"
#include "purchaseavailability.h"
#include "purchasesettings.h"
#include "teampowergridservice.h"
#include "wwdebug.h"

#include <string.h>


static bool										_IsOpen			= false;
static int										_Team				= PLAYERTYPE_NEUTRAL;
static CommanderSidebarModeType			_Mode				= SIDEBAR_MODE_BUILDINGS;
static int										_Page				= 0;
static int										_Credits			= 0;
static CommanderFacilityState				_Facility;
static BuildingGameObj *					_FacilityObject	= nullptr;
static DynamicVectorClass<CommanderSidebarCard>	_Cards;


/***********************************************************************************************
 * Page_Team -- a PLAYERTYPE to the purchase definitions' own two-value team enum                *
 *                                                                                             *
 *	PurchaseAvailabilityClass has a private function of the same name and the same body.  This  *
 *	is not a second implementation of a rule -- it is the same one-line mapping, which that      *
 *	class does not expose, and which the purchase definitions force on anyone who reads them.    *
 *	Anything richer (mutant pages) is content this sidebar has no mode for.                      *
 *=============================================================================================*/
static PurchaseSettingsDefClass::TEAM	Page_Team(int player_type)
{
	return (player_type == PLAYERTYPE_GDI) ? PurchaseSettingsDefClass::TEAM_GDI
													  : PurchaseSettingsDefClass::TEAM_NOD;
}


/***********************************************************************************************
 * Facility_Status -- the facility half of a unit card's status, or CARD_AVAILABLE               *
 *                                                                                             *
 *	Ordered most-fundamental-first so the reason shown is the one a player would fix first: a    *
 *	dead factory is not "unpowered", it is dead.                                                 *
 *=============================================================================================*/
static CommanderCardStatusType	Facility_Status(void)
{
	if (!_Facility.Bound)						return CARD_NO_FACILITY;
	if (!_Facility.Alive)						return CARD_FACILITY_DEAD;
	if (!_Facility.ConstructionComplete)	return CARD_FACILITY_CONSTRUCTING;
	if (!_Facility.Powered)					return CARD_FACILITY_UNPOWERED;
	return CARD_AVAILABLE;
}


/***********************************************************************************************
 * Add_Purchase_Page_Cards -- one stock purchase page, turned into cards                         *
 *=============================================================================================*/
static void	Add_Purchase_Page_Cards(PurchaseSettingsDefClass::TYPE type,int available_credits)
{
	PurchaseSettingsDefClass * page = PurchaseSettingsDefClass::Find_Definition (type,Page_Team (_Team));
	if (page == nullptr) {
		return;			// no content for this page: an empty mode, not an error
	}

	CommanderCardStatusType facility_status = Facility_Status ();

	for (int i = 0; i < PurchaseAvailabilityClass::ENTRY_COUNT; i++) {

		int definition_id = page->Get_Definition (i);
		if (definition_id == 0) continue;			// an empty slot in the page definition

		CommanderSidebarCard card;
		card.DefinitionId	= definition_id;
		card.Cost			= page->Get_Cost (i);
		card.TechLevel		= page->Get_Tech_Level (i);
		card.IconTexture	= page->Get_Texture (i);

		//	The page's name is a unicode string id resolved through the string table; the
		//	sidebar keeps the narrow name only as a fallback label, because a renderer that
		//	wants the localized one asks the page for it directly with the same index.
		card.DisplayName	= "";

		unsigned char flags = PurchaseAvailabilityClass::Get_Flags (type,Page_Team (_Team),i);

		if (facility_status != CARD_AVAILABLE) {
			card.Status = facility_status;
		} else if ((flags & PurchaseAvailabilityClass::FLAG_HIDDEN) != 0) {
			card.Status = CARD_HIDDEN;
		} else if ((flags & (PurchaseAvailabilityClass::FLAG_DISABLED | PurchaseAvailabilityClass::FLAG_BUSY)) != 0) {
			card.Status = CARD_DISABLED;
		} else if (card.TechLevel > PurchaseAvailabilityClass::Get_Tech_Level ()) {
			card.Status = CARD_TECH_LEVEL;
		} else if (CommanderProductionService::Is_Queue_Full (_Facility.MemberId,_Facility.QueueCapacity)) {
			card.Status = CARD_QUEUE_FULL;
		} else if (card.Cost > available_credits) {
			card.Status = CARD_INSUFFICIENT_CREDITS;
		} else {
			card.Status = CARD_AVAILABLE;
		}

		card.QueuedCount = CommanderProductionService::Get_Queued_Count (_Facility.MemberId,definition_id);

		_Cards.Add (card);
	}
}


/***********************************************************************************************
 * Add_Building_Cards -- every buildable descriptor the team may place                           *
 *=============================================================================================*/
static void	Add_Building_Cards(int available_credits)
{
	int count = BuildingDescriptorManager::Get_Descriptor_Count ();
	for (int i = 0; i < count; i++) {

		const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor_By_Index (i);
		if (descriptor == nullptr) continue;
		if (!descriptor->Build.Buildable) continue;

		CommanderSidebarCard card;
		card.DescriptorName		= descriptor->DefinitionName;
		card.Cost					= descriptor->Build.Cost;
		card.TechLevel				= descriptor->Build.TechLevel;
		card.DisplayName			= descriptor->Build.DisplayName;
		card.IconTexture			= descriptor->Build.IconTexture;
		card.PowerGeneration		= descriptor->Power.Generation;
		card.PowerConsumption	= descriptor->Power.Consumption;

		//	A descriptor with a team names the only faction that may build it; -1 is "any".
		if ((descriptor->Build.Team != -1) && (descriptor->Build.Team != _Team)) {
			card.Status = CARD_WRONG_FACTION;
		} else if (card.TechLevel > PurchaseAvailabilityClass::Get_Tech_Level ()) {
			card.Status = CARD_TECH_LEVEL;
		} else if (card.Cost > available_credits) {
			card.Status = CARD_INSUFFICIENT_CREDITS;
		} else {
			card.Status = CARD_AVAILABLE;
		}

		_Cards.Add (card);
	}
}


void CommanderSidebarModel::Init(void)
{
	_IsOpen				= false;
	_Team					= PLAYERTYPE_NEUTRAL;
	_Mode					= SIDEBAR_MODE_BUILDINGS;
	_Page					= 0;
	_Credits				= 0;
	_Facility			= CommanderFacilityState ();
	_FacilityObject	= nullptr;
	_Cards.Delete_All ();
}

void CommanderSidebarModel::Shutdown(void)
{
	Init ();
}


void CommanderSidebarModel::Open(int team)
{
	_IsOpen	= true;
	_Team		= team;
	_Mode		= SIDEBAR_MODE_BUILDINGS;		// "BUILDINGS is default"
	_Page		= 0;
	Unbind_Facility ();
	Refresh (_Credits);
}

void CommanderSidebarModel::Close(void)
{
	_IsOpen = false;
	Unbind_Facility ();
	_Cards.Delete_All ();
	_Page = 0;
}

bool	CommanderSidebarModel::Is_Open(void)	{ return _IsOpen; }
int	CommanderSidebarModel::Get_Team(void)	{ return _Team; }

CommanderSidebarModeType	CommanderSidebarModel::Get_Mode(void)	{ return _Mode; }


void CommanderSidebarModel::Set_Mode(CommanderSidebarModeType mode)
{
	if (mode < 0 || mode >= SIDEBAR_MODE_COUNT) return;
	if (mode == _Mode) return;

	_Mode	= mode;
	_Page	= 0;			// a mode change always lands on the first page of the new content
	Refresh (_Credits);
}

void CommanderSidebarModel::Return_To_Buildings(void)
{
	//	Not Set_Mode's early-out: returning to BUILDINGS while already in BUILDINGS must still
	//	drop a stale facility binding, which is the whole point of an explicit return input.
	_Mode	= SIDEBAR_MODE_BUILDINGS;
	_Page	= 0;
	Unbind_Facility ();
	Refresh (_Credits);
}


bool CommanderSidebarModel::Bind_Facility(BuildingGameObj * building,int member_id)
{
	if (!_IsOpen) return false;

	/*
	**	What kind of facility this is comes from the descriptor, never from the model name.
	**	A building whose descriptor is not loaded, or which produces nothing, is a legitimate
	**	thing to click -- it just is not a production facility, so the panel does not rebind.
	*/
	StringClass descriptor_name ("");
	if (building != nullptr) {
		descriptor_name = building->Get_Definition ().Get_Descriptor_Name ();
	}

	const BuildingModelDescriptor * descriptor = nullptr;
	if (descriptor_name.Get_Length () > 0) {
		descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name.Peek_Buffer ());
	}
	if (descriptor == nullptr) return false;
	if (descriptor->Production.Kind == BUILDING_PRODUCTION_NONE) return false;

	//	"Click/select an allied operational building" -- an enemy or neutral building never
	//	binds the panel, whatever it produces for its own side.
	int building_team = (building != nullptr) ? building->Get_Player_Type () : _Team;
	if (building_team != _Team) return false;

	_FacilityObject				= building;
	_Facility						= CommanderFacilityState ();
	_Facility.Bound				= true;
	_Facility.MemberId			= member_id;
	_Facility.Team					= building_team;
	_Facility.Kind					= descriptor->Production.Kind;
	_Facility.QueueCapacity		= descriptor->Production.QueueCapacity;
	_Facility.DescriptorName	= descriptor->DefinitionName;

	Refresh_Facility_State ();

	switch (descriptor->Production.Kind) {
		case BUILDING_PRODUCTION_INFANTRY:			_Mode = SIDEBAR_MODE_INFANTRY;			break;
		case BUILDING_PRODUCTION_GROUND_VEHICLE:	_Mode = SIDEBAR_MODE_GROUND_VEHICLES;	break;
		case BUILDING_PRODUCTION_AIR_VEHICLE:		_Mode = SIDEBAR_MODE_AIR_VEHICLES;		break;
		default:															break;
	}

	_Page = 0;
	Refresh (_Credits);
	return true;
}

void CommanderSidebarModel::Unbind_Facility(void)
{
	_Facility			= CommanderFacilityState ();
	_FacilityObject	= nullptr;
}

const CommanderFacilityState & CommanderSidebarModel::Get_Facility_State(void)
{
	return _Facility;
}


void CommanderSidebarModel::Refresh_Facility_State(void)
{
	if (!_Facility.Bound) return;

	/*
	**	Alive and powered are read from the two canonical owners, not cached at bind time:
	**	Section 42 requires the panel to disable itself the moment a facility dies or loses
	**	power, and a cached answer would be exactly the stale purchase it forbids.
	*/
	if (_FacilityObject != nullptr) {
		_Facility.Alive = !_FacilityObject->Is_Destroyed ();
	} else {
		//	A synthetic member with no game object behind it: the grid still knows whether it
		//	is registered and powered, which is what the rules below actually need.
		_Facility.Alive = true;
	}

	const BuildingModelDescriptor * descriptor = nullptr;
	if (_Facility.DescriptorName.Get_Length () > 0) {
		descriptor = BuildingDescriptorManager::Get_Descriptor (_Facility.DescriptorName.Peek_Buffer ());
	}

	bool requires_power = (descriptor != nullptr) ? descriptor->Production.RequiresPower : true;
	_Facility.Powered = requires_power
								? TeamPowerGridService::Is_Member_Powered (_Facility.MemberId)
								: true;

	//	Construction is Phase 38's state; a facility this service was handed without a
	//	construction record is one that was never under construction, i.e. complete.
	_Facility.ConstructionComplete = !BuildingConstructionService::Is_Under_Construction (_Facility.MemberId);
}


void CommanderSidebarModel::Refresh(int available_credits)
{
	_Credits = available_credits;
	_Cards.Delete_All ();

	if (!_IsOpen) return;

	Refresh_Facility_State ();

	switch (_Mode) {

		case SIDEBAR_MODE_BUILDINGS:
			Add_Building_Cards (available_credits);
			break;

		case SIDEBAR_MODE_INFANTRY:
			Add_Purchase_Page_Cards (PurchaseSettingsDefClass::TYPE_CLASSES,available_credits);
			break;

		case SIDEBAR_MODE_GROUND_VEHICLES:
			Add_Purchase_Page_Cards (PurchaseSettingsDefClass::TYPE_VEHICLES,available_credits);
			break;

		case SIDEBAR_MODE_AIR_VEHICLES:
			/*
			**	Stock Renegade ships no air purchase page -- there is no
			**	PurchaseSettingsDefClass::TYPE_AIRCRAFT and no content behind one.  An empty
			**	mode is the honest result: the mode exists, binds, pages and refuses
			**	purchases correctly, and fills the moment air content exists.  Recorded in
			**	docs/assets/CommanderSidebar.md rather than faked with vehicle entries.
			*/
			break;

		default:
			break;
	}

	//	Content shrank under the current page (a tech level dropped, a descriptor unloaded):
	//	clamp rather than show an empty page that paging cannot leave.
	int page_count = Get_Page_Count ();
	if (_Page >= page_count) {
		_Page = page_count - 1;
	}
	if (_Page < 0) _Page = 0;
}


int CommanderSidebarModel::Get_Card_Count(void)
{
	return _Cards.Count ();
}

const CommanderSidebarCard * CommanderSidebarModel::Get_Card(int index)
{
	if (index < 0 || index >= _Cards.Count ()) return nullptr;
	return &_Cards [index];
}


int CommanderSidebarModel::Get_Page(void)	{ return _Page; }

int CommanderSidebarModel::Get_Page_Count(void)
{
	if (_Cards.Count () <= 0) return 1;			// an empty mode still has one (empty) page
	return ((_Cards.Count () - 1) / PAGE_SIZE) + 1;
}

void CommanderSidebarModel::Set_Page(int page)
{
	int page_count = Get_Page_Count ();
	if (page < 0) page = 0;
	if (page >= page_count) page = page_count - 1;
	_Page = page;
}

void CommanderSidebarModel::Next_Page(void)	{ Set_Page (_Page + 1); }
void CommanderSidebarModel::Prev_Page(void)	{ Set_Page (_Page - 1); }


int CommanderSidebarModel::Get_Visible_Cards(const CommanderSidebarCard ** out_cards,int max_cards)
{
	if (out_cards == nullptr || max_cards <= 0) return 0;

	int first	= _Page * PAGE_SIZE;
	int wanted	= (max_cards < PAGE_SIZE) ? max_cards : (int)PAGE_SIZE;

	int written = 0;
	for (int i = 0; i < wanted; i++) {
		int index = first + i;
		if (index >= _Cards.Count ()) break;
		out_cards [written++] = &_Cards [index];
	}
	return written;
}


void CommanderSidebarModel::Get_Power_Display(CommanderPowerDisplay * out)
{
	if (out == nullptr) return;

	CommanderPowerDisplay display;
	display.Generation		= TeamPowerGridService::Get_Generation (_Team);
	display.RequestedDemand	= TeamPowerGridService::Get_Requested_Demand (_Team);
	display.AllocatedDemand	= TeamPowerGridService::Get_Allocated_Demand (_Team);
	display.ShedCount			= TeamPowerGridService::Get_Shed_Count (_Team);
	display.PoweredCount		= TeamPowerGridService::Get_Powered_Count (_Team);
	display.ManualOffCount	= TeamPowerGridService::Get_Manual_Off_Count (_Team);

	/*
	**	Headroom is against *allocated* demand, which is the power actually being drawn; it is
	**	never negative, because the allocator never allocates past generation.  Overload is a
	**	separate fact -- more was asked for than exists -- and is the one Section 42 wants made
	**	"immediately obvious", since it is what load shedding is a symptom of.
	*/
	display.Headroom		= display.Generation - display.AllocatedDemand;
	if (display.Headroom < 0.0f) display.Headroom = 0.0f;
	display.Overloaded	= (display.RequestedDemand > display.Generation);

	*out = display;
}


const char * CommanderSidebarModel::Get_Status_Name(CommanderCardStatusType status)
{
	switch (status) {
		case CARD_AVAILABLE:					return "available";
		case CARD_INSUFFICIENT_CREDITS:	return "insufficient credits";
		case CARD_TECH_LEVEL:				return "tech level";
		case CARD_HIDDEN:						return "hidden";
		case CARD_DISABLED:					return "disabled";
		case CARD_NO_FACILITY:				return "no facility selected";
		case CARD_FACILITY_DEAD:			return "facility destroyed";
		case CARD_FACILITY_CONSTRUCTING:	return "facility under construction";
		case CARD_FACILITY_UNPOWERED:		return "facility unpowered";
		case CARD_QUEUE_FULL:				return "queue full";
		case CARD_WRONG_FACTION:			return "wrong faction";
		default:									return "unknown";
	}
}

const char * CommanderSidebarModel::Get_Mode_Name(CommanderSidebarModeType mode)
{
	switch (mode) {
		case SIDEBAR_MODE_BUILDINGS:			return "BUILDINGS";
		case SIDEBAR_MODE_GROUND_VEHICLES:	return "GROUND_VEHICLES";
		case SIDEBAR_MODE_AIR_VEHICLES:		return "AIR_VEHICLES";
		case SIDEBAR_MODE_INFANTRY:			return "INFANTRY";
		default:										return "unknown";
	}
}
