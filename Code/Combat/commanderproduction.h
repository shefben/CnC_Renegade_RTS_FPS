/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderproduction.h                 $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- Commander AI infantry/ground/air production + unit control. *
 *                                                                                             *
 *	Section 45's first requirement is the one that decides this file's whole shape: "Do not      *
 *	make bots emulate human PT GUI input."  A Commander purchase is a server-side request        *
 *	against a validated queue, and the thing that eventually creates the unit is canonical       *
 *	Renegade production -- so this service owns the queue, the validation and the ordering, and   *
 *	owns no spawning code at all.  Completed items come out of Get_Completed_Item for the         *
 *	caller that knows how to build a SoldierGameObj or a VehicleGameObj, the same "report,        *
 *	don't act" split BuildingConstructionService already uses for its completions, and for the    *
 *	same reason: the activation order is the caller's to run in one place.                        *
 *                                                                                             *
 *	"Commander-purchased infantry are normal AI SoldierGameObj paths, not fake human players"     *
 *	is therefore provable by grep on this file: there is no player-slot, scoreboard, or           *
 *	cPlayer code in it, because there is no object creation in it.                                *
 *                                                                                             *
 *	Queues are bound to a facility's power-grid member id -- the building's own network id --     *
 *	not to a team, which is Section 45's "Multiple facilities may have independent queues" and     *
 *	"Do not silently redirect to another building" as a property of the data structure rather      *
 *	than a rule someone has to remember.                                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPRODUCTION_H
#define COMMANDERPRODUCTION_H

#include "always.h"
#include "buildingdescriptor.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	Why a production request was refused.  Section 45's own server-side validation list, one
**	value each -- and the same list Section 45's "UI feedback" block asks the sidebar to show,
**	so the reason the server refused and the reason the panel displays are one value, not two
**	parallel tables that can disagree.
*/
enum CommanderProductionResultType
{
	PRODUCTION_ACCEPTED = 0,
	PRODUCTION_NO_AUTHORITY,			// the requester is not this team's Commander
	PRODUCTION_NO_FACILITY,
	PRODUCTION_FACILITY_DEAD,
	PRODUCTION_FACILITY_CONSTRUCTING,
	PRODUCTION_FACILITY_UNPOWERED,
	PRODUCTION_WRONG_FACILITY_KIND,	// an air unit asked of a barracks
	PRODUCTION_QUEUE_FULL,
	PRODUCTION_POPULATION_CAP,
	PRODUCTION_INSUFFICIENT_CREDITS,
	PRODUCTION_UNAVAILABLE,				// tech level, or a script disabled the entry

	PRODUCTION_RESULT_COUNT
};


enum CommanderProductionItemStateType
{
	PRODUCTION_STATE_QUEUED = 0,
	PRODUCTION_STATE_PRODUCING,
	PRODUCTION_STATE_WAITING_FOR_POWER,
	PRODUCTION_STATE_BLOCKED_EXIT,
	PRODUCTION_STATE_COMPLETED,
};


/*
**	Section 45's own order list.  No SAGE order model is introduced -- these names are what a
**	canonical Renegade/TT AI state is asked to do, and Phase 37's GuardStateController already
**	owns the guard/retaliation half of that.
*/
enum CommanderUnitOrderType
{
	ORDER_NONE = 0,
	ORDER_MOVE,
	ORDER_ATTACK_TARGET,
	ORDER_ATTACK_MOVE,
	ORDER_GUARD,
	ORDER_HOLD_POSITION,		// the default a newly produced unit takes at its staging point
	ORDER_PATROL,
	ORDER_FOLLOW,
	ORDER_REPAIR_TARGET,
	ORDER_RETREAT_TO_REPAIR,

	ORDER_COUNT
};


struct CommanderProductionItem
{
	CommanderProductionItem(void) : FacilityId(0),DefinitionId(0),Team(-1),Cost(0),
											  BuildTimeSeconds(0.0f),ElapsedSeconds(0.0f),
											  State(PRODUCTION_STATE_QUEUED),
											  Kind(BUILDING_PRODUCTION_NONE),TicketId(0) { }

	bool	operator == (const CommanderProductionItem &) const	{ return false; }
	bool	operator != (const CommanderProductionItem &) const	{ return true; }

	int									FacilityId;
	int									DefinitionId;
	int									Team;
	int									Cost;
	float									BuildTimeSeconds;
	float									ElapsedSeconds;
	CommanderProductionItemStateType	State;
	BuildingProductionKindType			Kind;
	int									TicketId;		// unique, so a completion names exactly one request
};


/*
**	What the facility told this service about itself when the request arrived.  Passed in
**	rather than looked up, for the same reason CommanderModeController takes `star_is_alive`:
**	it makes every rule in Validate checkable with no BuildingGameObj, no power grid and no
**	scene, and it keeps this service from growing a second opinion about whether a building is
**	alive.
*/
struct CommanderFacilityContext
{
	CommanderFacilityContext(void) : FacilityId(0),Team(-1),Alive(false),ConstructionComplete(false),
												Powered(false),Kind(BUILDING_PRODUCTION_NONE),QueueCapacity(5),
												ExitClear(true) { }

	int								FacilityId;
	int								Team;
	bool								Alive;
	bool								ConstructionComplete;
	bool								Powered;
	BuildingProductionKindType	Kind;
	int								QueueCapacity;
	bool								ExitClear;		// the spawn/exit volume is free right now
};


class CommanderProductionService
{
	public:

		//	Section 45's "limits/performance" block.  Defaults, not law -- Set_Population_Cap
		//	moves each one, and a server that wants a different budget says so once at startup.
		enum
		{
			DEFAULT_INFANTRY_CAP			= 40,
			DEFAULT_GROUND_VEHICLE_CAP	= 20,
			DEFAULT_AIR_VEHICLE_CAP		= 10,
			DEFAULT_TOTAL_AI_BUDGET		= 60,
		};

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Commander authority.  One Commander per team, named by player id; 0 means the team
		**	has none and every request from it is refused.  Deliberately not a bool on a player
		**	object -- the authority check has to be answerable on the server with nothing but a
		**	team and an id, which is exactly what a network handler has.
		*/
		static void		Set_Team_Commander(int team,int player_id);
		static int		Get_Team_Commander(int team);

		/*
		**	Vacancy.  A Commander who disconnects, is dropped, changes team or otherwise stops
		**	having a soldier on the team they command must not keep the job -- the authority
		**	check is by player id, and an id nobody is using any more would lock the team out of
		**	its own production for the rest of the match with no way to elect a replacement.
		**
		**	Detected by polling rather than by a disconnect callback on purpose: this one test
		**	covers a clean quit, a timeout, a crash, a kick and a team change identically, and
		**	there is no path by which a Commander can vanish without it noticing.  The grace
		**	period is what keeps an ordinary respawn -- during which a player briefly has no
		**	soldier -- from being read as having left.
		**
		**	Server only; a client's copy is overwritten by the replicated state either way.
		*/
		static void		Update_Commander_Vacancy(float seconds);
		static float	Get_Vacancy_Grace_Seconds(void);
		static int		Get_Vacated_Count(void);

		static void		Set_Population_Cap(BuildingProductionKindType kind,int cap);
		static int		Get_Population_Cap(BuildingProductionKindType kind);
		static void		Set_Total_AI_Budget(int budget);

		//	The live count this service compares against the caps.  Maintained by the caller
		//	that actually creates and destroys the units, because this service creates none.
		static void		Set_Population(int team,BuildingProductionKindType kind,int count);
		static int		Get_Population(int team,BuildingProductionKindType kind);

		/*
		**	The one validated entry point.  Server-side: everything Section 45 lists is checked
		**	here, in the order a player would fix it, and nothing is charged or queued unless
		**	every check passes.  `available_credits` is passed in rather than read from a
		**	PlayerDataClass so that the rule is checkable and so that this service never has to
		**	decide *whose* credits a team purchase spends.
		**
		**	On acceptance `out_ticket` receives the new item's ticket id, and the caller debits
		**	the credits -- this service tracks cost but owns no wallet.
		*/
		static CommanderProductionResultType	Request_Production(int requesting_player_id,
																					const CommanderFacilityContext & facility,
																					int definition_id,int cost,
																					float build_time_seconds,
																					int available_credits,
																					bool entry_available,
																					int * out_ticket);

		//	Validation with no side effect, for a UI that wants to grey a card out.  Exactly the
		//	function Request_Production calls, so the two can never disagree.
		static CommanderProductionResultType	Validate(int requesting_player_id,
																		 const CommanderFacilityContext & facility,
																		 int cost,int available_credits,
																		 bool entry_available);

		/*
		**	Advance every queue by `seconds`.  Only the head item of a facility's queue
		**	progresses -- one line per building, which is what a Renegade factory is.  An item
		**	whose facility has lost power stops accumulating time and reads
		**	WAITING_FOR_POWER rather than being cancelled: Section 45 requires "one documented
		**	canonical pause/cancel policy", and pause is the one this codebase takes, because
		**	cancelling on a power flicker would refund-and-requeue a queue every time a Power
		**	Plant took a hit.
		**
		**	`Set_Facility_Runtime_State` is how the caller keeps the service told; a facility it
		**	has never been told about is treated as alive, complete, powered and clear, which is
		**	what a level-placed factory that nothing has reported on actually is.
		*/
		static void		Set_Facility_Runtime_State(int facility_id,bool alive,bool construction_complete,
																  bool powered,bool exit_clear);

		static void		Update(float seconds);

		/*
		**	Completions, reported rather than acted on.  The caller drains this list, creates
		**	the canonical unit, and clears it.  An item stays in the list until cleared, so a
		**	caller that runs less often than Update never loses one.
		*/
		static int		Get_Completed_Count(void);
		static bool		Get_Completed_Item(int index,CommanderProductionItem * out);
		static void		Clear_Completed(void);

		//	Queue inspection, which is also Section 42's "queue state" on a card.
		static int		Get_Queue_Length(int facility_id);
		static int		Get_Queued_Count(int facility_id,int definition_id);

		//	Capacity is the descriptor's, so the caller that has the descriptor passes it --
		//	this service deliberately keeps no second copy of a value that a live descriptor
		//	reload can change underneath it.
		static bool		Is_Queue_Full(int facility_id,int queue_capacity);
		static bool		Get_Queue_Item(int facility_id,int index,CommanderProductionItem * out);

		/*
		**	Cancellation and destruction.  Section 45: destruction "cancels/resolves queue,
		**	applies documented refund rules, clears stale UI binding, and prevents invisible
		**	completion".  The refund rule taken here: a cancelled or destroyed item refunds its
		**	full cost, because the unit never existed -- partial refunds would need a
		**	partially-built-unit concept this game does not have.  The refunded total is
		**	returned so the caller can credit it back to the same wallet it debited.
		*/
		static int		Cancel_Item(int facility_id,int ticket_id);
		static int		Cancel_Facility_Queue(int facility_id);
		static int		On_Facility_Destroyed(int facility_id);

		/*
		**	Commander selection and orders.  Selection is a bounded set of network object ids --
		**	ids rather than pointers, because a selected unit can die between the click and the
		**	order, and a dead id is a miss where a dangling pointer is a crash.
		*/
		enum { MAX_SELECTION = 64 };

		static void		Clear_Selection(void);
		static bool		Add_To_Selection(int unit_id);
		static bool		Remove_From_Selection(int unit_id);
		static bool		Is_Selected(int unit_id);
		static int		Get_Selection_Count(void);
		static int		Get_Selected_Unit(int index);

		/*
		**	"Server validates which units the Commander can control."  The caller supplies the
		**	unit's team and whether it is Commander-controllable AI; this refuses an order to a
		**	unit on another team or to a human-driven one, and returns the order it actually
		**	set so a caller can log a refusal.
		*/
		static bool		Can_Order_Unit(int requesting_player_id,int unit_team,bool is_commander_ai);

		static bool		Issue_Order(int requesting_player_id,int unit_id,int unit_team,bool is_commander_ai,
											 CommanderUnitOrderType order,const Vector3 & target_point,int target_id);

		static CommanderUnitOrderType	Get_Order(int unit_id);
		static bool		Get_Order_Target(int unit_id,Vector3 * out_point,int * out_target_id);
		static void		Forget_Unit(int unit_id);
		static int		Get_Order_Count(void);

		/*
		**	Enumeration and restore, for Phase 40.  Restore_Item deliberately bypasses every
		**	rule Request_Production enforces: a saved queue was already validated when it was
		**	requested, and re-validating it at load -- against a credit balance that has since
		**	been spent and a facility whose power state has not been recomputed yet -- would
		**	silently delete a legitimate queue.  Section 46's own instruction is to persist the
		**	queues and revalidate *navigation*, not purchases.
		*/
		static int		Get_Item_Count(void);
		static bool		Get_Item(int index,CommanderProductionItem * out);
		static void		Restore_Item(const CommanderProductionItem & item);

		static bool		Get_Order_Record(int index,int * out_unit_id,CommanderUnitOrderType * out_order,
													 Vector3 * out_point,int * out_target_id);
		static void		Restore_Order(int unit_id,CommanderUnitOrderType order,
												const Vector3 & target_point,int target_id);

		static const char *	Get_Result_Name(CommanderProductionResultType result);
		static const char *	Get_Order_Name(CommanderUnitOrderType order);
		static const char *	Get_Item_State_Name(CommanderProductionItemStateType state);
};

#endif // COMMANDERPRODUCTION_H
