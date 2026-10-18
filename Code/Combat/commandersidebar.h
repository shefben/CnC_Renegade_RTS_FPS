/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandersidebar.h                    $*
 *                                                                                             *
 *	Roadmap Section 42, Phase 36 -- Commander right-side UI + contextual production sidebar.     *
 *                                                                                             *
 *	This is the sidebar's *model*, not its pixels.  Section 42 describes a panel, but almost    *
 *	everything it actually specifies is state and rules: which mode is showing, which physical  *
 *	facility the panel is bound to, what each card says, which page of eight is visible, what   *
 *	the power bars read, and -- the part that matters most -- exactly when a card must refuse   *
 *	("Never allow stale purchases from a dead/offline facility").  All of that is decidable      *
 *	with no device, no Render2D and no scene, so all of it lives here and is checked here.       *
 *	A renderer is a consumer of this class; it owns no rule of its own.                          *
 *                                                                                             *
 *	The split is the same one StatusOverlaySystem (Section 32) already made for the same         *
 *	reason, and it is what lets "dead/offline selected facilities cannot produce" be proved by    *
 *	`renegade -shaderselfcheck sidebar` instead of by a person watching a panel grey out.        *
 *                                                                                             *
 *	Content sources, in order of how canonical they are:                                        *
 *                                                                                             *
 *	  INFANTRY        PurchaseSettingsDefClass TYPE_CLASSES   -- real stock content, real PT     *
 *	  GROUND_VEHICLES PurchaseSettingsDefClass TYPE_VEHICLES  -- icons, real costs, real tech    *
 *	                                                             levels, real availability flags *
 *	  AIR_VEHICLES    the same page shape, and stock Renegade ships no air purchase page, so     *
 *	                  this mode is empty until content exists (see docs/assets/)                 *
 *	  BUILDINGS       every loaded BuildingModelDescriptor whose `build.buildable` is true       *
 *                                                                                             *
 *	"Reuse existing Renegade Purchase Terminal images whenever available" is therefore not a     *
 *	thing this class tries to do -- it is a consequence of reading the purchase definitions      *
 *	themselves, which is where those images already are.                                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERSIDEBAR_H
#define COMMANDERSIDEBAR_H

#include "always.h"
#include "buildingdescriptor.h"
#include "wwstring.h"

class BuildingGameObj;


enum CommanderSidebarModeType
{
	SIDEBAR_MODE_BUILDINGS = 0,		// the default, and the one a return-to-BUILDINGS input restores
	SIDEBAR_MODE_GROUND_VEHICLES,
	SIDEBAR_MODE_AIR_VEHICLES,
	SIDEBAR_MODE_INFANTRY,

	SIDEBAR_MODE_COUNT
};


/*
**	Why a card cannot be bought right now.  One enumerated reason rather than a bool, because
**	Section 42 requires the panel to display a "prerequisite/disabled reason" -- a caller that
**	only got `false` back would have to re-derive the reason to draw it, which is how two
**	implementations of the same rule start.
*/
enum CommanderCardStatusType
{
	CARD_AVAILABLE = 0,
	CARD_INSUFFICIENT_CREDITS,
	CARD_TECH_LEVEL,
	CARD_HIDDEN,					// the purchase entry's own FLAG_HIDDEN
	CARD_DISABLED,					// FLAG_DISABLED / FLAG_BUSY, or a script turned the entry off
	CARD_NO_FACILITY,				// a unit mode with nothing bound to it
	CARD_FACILITY_DEAD,
	CARD_FACILITY_CONSTRUCTING,
	CARD_FACILITY_UNPOWERED,
	CARD_QUEUE_FULL,

	//	No longer produced: a descriptor another faction owns is left off the panel entirely
	//	rather than shown greyed out, because both factions author the same categories and the
	//	other side's copy read as a duplicate card.  Kept so a saved/replicated status value
	//	cannot silently become a different reason.
	CARD_WRONG_FACTION,

	CARD_STATUS_COUNT
};


struct CommanderSidebarCard
{
	CommanderSidebarCard(void) : DefinitionId(0),Cost(0),TechLevel(0),PowerGeneration(0.0f),
											PowerConsumption(0.0f),Status(CARD_AVAILABLE),QueuedCount(0),
											DisplayName(""),IconTexture(""),DescriptorName("") { }

	bool	operator == (const CommanderSidebarCard &) const	{ return false; }
	bool	operator != (const CommanderSidebarCard &) const	{ return true; }

	int							DefinitionId;			// the preset a purchase resolves to; 0 for a building card
	int							Cost;
	int							TechLevel;
	float							PowerGeneration;		// buildings only; drawn as "+N" on the card
	float							PowerConsumption;		// buildings only; drawn as "-N"
	CommanderCardStatusType	Status;
	int							QueuedCount;

	StringClass					DisplayName;
	StringClass					IconTexture;			// an existing PT image where one exists, "" otherwise
	StringClass					DescriptorName;		// building cards only: the key into the descriptor registry
};


/*
**	Everything Section 42's "Power display" block names, in one read.  Filled from
**	TeamPowerGridService, which is the only thing that knows any of it -- this struct exists so
**	a renderer never has to make six separate service calls and risk drawing two of them from
**	different allocation passes.
*/
struct CommanderPowerDisplay
{
	CommanderPowerDisplay(void) : Generation(0.0f),RequestedDemand(0.0f),AllocatedDemand(0.0f),
											Headroom(0.0f),Overloaded(false),ShedCount(0),PoweredCount(0),
											ManualOffCount(0) { }

	float	Generation;
	float	RequestedDemand;
	float	AllocatedDemand;
	float	Headroom;			// Generation - AllocatedDemand; negative is impossible by construction
	bool	Overloaded;			// RequestedDemand > Generation: demand the grid could not meet
	int	ShedCount;
	int	PoweredCount;
	int	ManualOffCount;
};


/*
**	The state of whatever physical building the panel is currently bound to.  A unit mode with
**	no live, complete, powered facility behind it produces nothing, and this is the one place
**	that fact is decided.
*/
struct CommanderFacilityState
{
	CommanderFacilityState(void) : Bound(false),MemberId(0),Team(-1),Alive(false),
											 ConstructionComplete(false),Powered(false),
											 Kind(BUILDING_PRODUCTION_NONE),QueueCapacity(0),DescriptorName("") { }

	bool								Bound;
	int								MemberId;					// the power-grid member id, i.e. the building's network id
	int								Team;
	bool								Alive;
	bool								ConstructionComplete;
	bool								Powered;
	BuildingProductionKindType	Kind;
	int								QueueCapacity;
	StringClass						DescriptorName;

	//	The single question every purchase path asks.  Section 42: "If selected facility is
	//	destroyed, under construction, loses power, or becomes non-operational, immediately
	//	disable/update the contextual panel."
	bool	Is_Operational(void) const
		{ return Bound && Alive && ConstructionComplete && Powered; }
};


class CommanderSidebarModel
{
	public:

		enum
		{
			//	Section 42: "two rows ... target 2 x 4 cells = 8 visible slots".  The visible
			//	page size, explicitly not a cap on how much content there may be.
			VISIBLE_ROWS		= 2,
			VISIBLE_COLUMNS	= 4,
			PAGE_SIZE			= VISIBLE_ROWS * VISIBLE_COLUMNS,
		};

		static void		Init(void);
		static void		Shutdown(void);

		//	Opening and closing the panel.  Section 42: "The sidebar exists in Commander Mode
		//	only."  Nothing else in this class does anything while it is closed.
		static void		Open(int team);
		static void		Close(void);
		static bool		Is_Open(void);
		static int		Get_Team(void);

		static void		Set_Mode(CommanderSidebarModeType mode);
		static CommanderSidebarModeType	Get_Mode(void);

		//	"Provide an obvious button/hotkey/input to return to BUILDINGS."  Returning also
		//	drops the facility binding, because a building card is never bound to a factory.
		static void		Return_To_Buildings(void);

		/*
		**	Clicking an allied building in the world.  The descriptor's production kind decides
		**	the mode -- ground factory -> GROUND_VEHICLES, Air Pad -> AIR_VEHICLES,
		**	Barracks/Hand -> INFANTRY -- which is Section 42's "use canonical building
		**	definition/capability plus descriptor semantics, not raw model filenames alone",
		**	made literal.  Returns false and changes nothing for a building that produces
		**	nothing, a building on another team, or a null one.
		**
		**	`building` may be nullptr when `member_id` is a synthetic member, which is what lets
		**	the binding rules be checked with no scene.
		*/
		static bool		Bind_Facility(BuildingGameObj * building,int member_id);
		static void		Unbind_Facility(void);
		static const CommanderFacilityState &	Get_Facility_State(void);

		//	Re-reads the facility's live state from the power grid and the game object.  Called
		//	by Refresh, and separately by anything that wants the "immediately disable" part of
		//	Section 42 to happen on a state change rather than on the next frame's refresh.
		static void		Refresh_Facility_State(void);

		/*
		**	Rebuild the card list for the current mode, team, facility and credit balance.
		**	Every Status value on every card is decided here, so a renderer draws what it is
		**	given and a production request re-asks this class rather than re-deriving anything.
		*/
		static void		Refresh(int available_credits);

		static int		Get_Card_Count(void);
		static const CommanderSidebarCard *	Get_Card(int index);

		//	Paging over the eight visible slots.  Page indices are clamped, never wrapped: a
		//	Next_Page on the last page is a no-op, which is what a held key should do.
		static int		Get_Page(void);
		static int		Get_Page_Count(void);
		static void		Set_Page(int page);
		static void		Next_Page(void);
		static void		Prev_Page(void);

		//	The cards on the current page.  `out_cards` is filled with at most PAGE_SIZE
		//	entries; the return value is how many, which is fewer than PAGE_SIZE on a partial
		//	last page.
		static int		Get_Visible_Cards(const CommanderSidebarCard ** out_cards,int max_cards);

		static void		Get_Power_Display(CommanderPowerDisplay * out);

		//	Purely so a renderer and a self check can name a reason without a second table.
		static const char *	Get_Status_Name(CommanderCardStatusType status);
		static const char *	Get_Mode_Name(CommanderSidebarModeType mode);
};

#endif // COMMANDERSIDEBAR_H
