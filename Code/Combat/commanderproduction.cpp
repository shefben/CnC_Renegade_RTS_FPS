/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderproduction.cpp               $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- Commander AI infantry/ground/air production + unit control. *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderproduction.h"

#include "gameobjmanager.h"
#include "playertype.h"
#include "soldier.h"
#include "vector.h"


struct FacilityRuntimeState
{
	FacilityRuntimeState(void) : InUse(false),FacilityId(0),Alive(true),ConstructionComplete(true),
										  Powered(true),ExitClear(true) { }

	bool	operator == (const FacilityRuntimeState &) const	{ return false; }
	bool	operator != (const FacilityRuntimeState &) const	{ return true; }

	bool	InUse;
	int	FacilityId;
	bool	Alive;
	bool	ConstructionComplete;
	bool	Powered;
	bool	ExitClear;
};

struct UnitOrderRecord
{
	UnitOrderRecord(void) : InUse(false),UnitId(0),Order(ORDER_NONE),
									TargetPoint(0.0f,0.0f,0.0f),TargetId(0) { }

	bool	operator == (const UnitOrderRecord &) const	{ return false; }
	bool	operator != (const UnitOrderRecord &) const	{ return true; }

	bool						InUse;
	int						UnitId;
	CommanderUnitOrderType	Order;
	Vector3					TargetPoint;
	int						TargetId;
};

/*
**	A selected unit's id, wrapped in a struct rather than stored as a bare int.  Not stylistic:
**	DynamicVectorClass<int>::Delete is genuinely ambiguous -- Delete(T const &) and Delete(int)
**	are the same signature when T is int -- so a vector of ints is a container you can add to
**	and never remove one element from.  One member struct makes the index overload unambiguous.
*/
struct SelectedUnitEntry
{
	SelectedUnitEntry(void) : UnitId(0) { }
	explicit SelectedUnitEntry(int unit_id) : UnitId(unit_id) { }

	bool	operator == (const SelectedUnitEntry &) const	{ return false; }
	bool	operator != (const SelectedUnitEntry &) const	{ return true; }

	int	UnitId;
};

struct TeamPopulation
{
	TeamPopulation(void) : InUse(false),Team(PLAYERTYPE_NEUTRAL) { Counts [0] = Counts [1] = Counts [2] = Counts [3] = 0; }

	bool	operator == (const TeamPopulation &) const	{ return false; }
	bool	operator != (const TeamPopulation &) const	{ return true; }

	bool	InUse;
	int	Team;
	int	Counts [4];			// indexed by BuildingProductionKindType
};


static DynamicVectorClass<CommanderProductionItem>	_Queue;			// every facility's items, in request order
static DynamicVectorClass<CommanderProductionItem>	_Completed;
static DynamicVectorClass<FacilityRuntimeState>		_Facilities;
static DynamicVectorClass<UnitOrderRecord>			_Orders;
static DynamicVectorClass<TeamPopulation>				_Populations;
static DynamicVectorClass<SelectedUnitEntry>			_Selection;

static int		_Commanders [4];			// indexed by Team_Index; 0 means "this team has no Commander"

//	How long a Commander may be soldier-less before the job is vacated.  Long enough to cover a
//	respawn and a bad moment on the network, short enough that a team is not locked out of its
//	own production for a noticeable part of a match after someone quits.
static const float	COMMANDER_VACANCY_GRACE_SECONDS = 20.0f;

static float	_CommanderMissingFor [4] = { 0.0f,0.0f,0.0f,0.0f };
static int		_VacatedCount = 0;
static int		_Caps [4];
static int		_TotalBudget	= CommanderProductionService::DEFAULT_TOTAL_AI_BUDGET;
static int		_NextTicket		= 1;


/***********************************************************************************************
 * Team_Index -- PLAYERTYPE_NOD/GDI to a small array index, everything else to a shared slot     *
 *                                                                                             *
 *	The same shape TeamPowerGridService's own team table uses: two real sides, and one slot for  *
 *	everything that is not one, so a stray PLAYERTYPE_NEUTRAL cannot index out of the table.     *
 *=============================================================================================*/
static int	Team_Index(int team)
{
	if (team == PLAYERTYPE_NOD) return 0;
	if (team == PLAYERTYPE_GDI) return 1;
	return 2;
}

static int	Kind_Index(BuildingProductionKindType kind)
{
	int index = (int)kind;
	if (index < 0 || index > 3) return 0;
	return index;
}


static FacilityRuntimeState *	Find_Facility(int facility_id)
{
	for (int i = 0; i < _Facilities.Count (); i++) {
		if (_Facilities [i].InUse && _Facilities [i].FacilityId == facility_id) return &_Facilities [i];
	}
	return nullptr;
}

static TeamPopulation *	Find_Or_Make_Population(int team)
{
	for (int i = 0; i < _Populations.Count (); i++) {
		if (_Populations [i].InUse && _Populations [i].Team == team) return &_Populations [i];
	}
	TeamPopulation entry;
	entry.InUse	= true;
	entry.Team	= team;
	_Populations.Add (entry);
	return &_Populations [_Populations.Count () - 1];
}


void CommanderProductionService::Init(void)
{
	_Queue.Delete_All ();
	_Completed.Delete_All ();
	_Facilities.Delete_All ();
	_Orders.Delete_All ();
	_Populations.Delete_All ();
	_Selection.Delete_All ();

	for (int i = 0; i < 4; i++) _Commanders [i] = 0;
	for (int i = 0; i < 4; i++) _CommanderMissingFor [i] = 0.0f;
	_VacatedCount = 0;

	_Caps [Kind_Index (BUILDING_PRODUCTION_NONE)]				= 0;
	_Caps [Kind_Index (BUILDING_PRODUCTION_INFANTRY)]			= DEFAULT_INFANTRY_CAP;
	_Caps [Kind_Index (BUILDING_PRODUCTION_GROUND_VEHICLE)]	= DEFAULT_GROUND_VEHICLE_CAP;
	_Caps [Kind_Index (BUILDING_PRODUCTION_AIR_VEHICLE)]		= DEFAULT_AIR_VEHICLE_CAP;

	_TotalBudget	= DEFAULT_TOTAL_AI_BUDGET;
	_NextTicket		= 1;
}

void CommanderProductionService::Shutdown(void)
{
	Init ();
}


void CommanderProductionService::Set_Team_Commander(int team,int player_id)
{
	int index = Team_Index (team);
	_Commanders [index] = player_id;

	//	Any change, in either direction, restarts the clock.  A resignation followed immediately
	//	by a new Commander must not inherit the old one's accumulated absence.
	_CommanderMissingFor [index] = 0.0f;
}


/***********************************************************************************************
 * CommanderProductionService::Update_Commander_Vacancy -- give up the job when nobody holds it   *
 *                                                                                               *
 *	See the header for why this polls instead of listening for a disconnect.  Two ways to fail    *
 *	the test, and both matter: no soldier at all for that client id (quit, timeout, crash, kick), *
 *	or a soldier that is no longer on the team being commanded (team change).  The second is      *
 *	immediate rather than graced -- someone who switched sides should not keep command of the     *
 *	team they left for twenty seconds, and unlike a respawn there is nothing transient about it.  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void CommanderProductionService::Update_Commander_Vacancy(float seconds)
{
	if (seconds <= 0.0f) return;

	static const int	TEAMS [2] = { PLAYERTYPE_NOD,PLAYERTYPE_GDI };

	for (int i = 0; i < 2; i++) {

		int team		= TEAMS [i];
		int index	= Team_Index (team);
		int player	= _Commanders [index];

		if (player == 0) {
			_CommanderMissingFor [index] = 0.0f;
			continue;
		}

		SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (player);

		if ((soldier != nullptr) && (soldier->Get_Player_Type () != team)) {
			//	Changed team.  Vacated at once; there is nothing to wait for.
			_Commanders [index]				= 0;
			_CommanderMissingFor [index]	= 0.0f;
			_VacatedCount++;
			continue;
		}

		if (soldier != nullptr) {
			_CommanderMissingFor [index] = 0.0f;
			continue;
		}

		_CommanderMissingFor [index] += seconds;
		if (_CommanderMissingFor [index] < COMMANDER_VACANCY_GRACE_SECONDS) continue;

		_Commanders [index]				= 0;
		_CommanderMissingFor [index]	= 0.0f;
		_VacatedCount++;
	}
}


float	CommanderProductionService::Get_Vacancy_Grace_Seconds(void)	{ return COMMANDER_VACANCY_GRACE_SECONDS; }
int	CommanderProductionService::Get_Vacated_Count(void)				{ return _VacatedCount; }

int CommanderProductionService::Get_Team_Commander(int team)
{
	return _Commanders [Team_Index (team)];
}


void CommanderProductionService::Set_Population_Cap(BuildingProductionKindType kind,int cap)
{
	if (cap < 0) cap = 0;
	_Caps [Kind_Index (kind)] = cap;
}

int CommanderProductionService::Get_Population_Cap(BuildingProductionKindType kind)
{
	return _Caps [Kind_Index (kind)];
}

void CommanderProductionService::Set_Total_AI_Budget(int budget)
{
	if (budget < 0) budget = 0;
	_TotalBudget = budget;
}


void CommanderProductionService::Set_Population(int team,BuildingProductionKindType kind,int count)
{
	if (count < 0) count = 0;
	Find_Or_Make_Population (team)->Counts [Kind_Index (kind)] = count;
}

int CommanderProductionService::Get_Population(int team,BuildingProductionKindType kind)
{
	return Find_Or_Make_Population (team)->Counts [Kind_Index (kind)];
}


/***********************************************************************************************
 * Pending_Of_Kind -- units of this kind already queued for this team                            *
 *                                                                                             *
 *	A cap that only counted *live* units would let a Commander queue thirty tanks against a cap  *
 *	of twenty and get them all, because none of them exists yet when the twenty-first is asked   *
 *	for.  Counting the queue against the cap is what makes "production is bounded" true rather   *
 *	than eventually true.                                                                        *
 *=============================================================================================*/
static int	Pending_Of_Kind(int team,BuildingProductionKindType kind)
{
	int count = 0;
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].Team == team && _Queue [i].Kind == kind) count++;
	}
	return count;
}

static int	Pending_Total(int team)
{
	int count = 0;
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].Team == team) count++;
	}
	return count;
}


CommanderProductionResultType CommanderProductionService::Validate(int requesting_player_id,
																						 const CommanderFacilityContext & facility,
																						 int cost,int available_credits,
																						 bool entry_available)
{
	//	Ordered as a player would fix it: authority, then the building, then the rules, then
	//	the money.  "Insufficient credits" last, because it is the only one that resolves by
	//	waiting.
	int commander = Get_Team_Commander (facility.Team);
	if (commander == 0 || commander != requesting_player_id) {
		return PRODUCTION_NO_AUTHORITY;
	}

	if (facility.FacilityId == 0)						return PRODUCTION_NO_FACILITY;
	if (facility.Kind == BUILDING_PRODUCTION_NONE)	return PRODUCTION_WRONG_FACILITY_KIND;
	if (!facility.Alive)									return PRODUCTION_FACILITY_DEAD;
	if (!facility.ConstructionComplete)				return PRODUCTION_FACILITY_CONSTRUCTING;
	if (!facility.Powered)								return PRODUCTION_FACILITY_UNPOWERED;

	if (!entry_available)									return PRODUCTION_UNAVAILABLE;

	int capacity = (facility.QueueCapacity > 0) ? facility.QueueCapacity : 1;
	if (Get_Queue_Length (facility.FacilityId) >= capacity) {
		return PRODUCTION_QUEUE_FULL;
	}

	int live		= Get_Population (facility.Team,facility.Kind);
	int pending	= Pending_Of_Kind (facility.Team,facility.Kind);
	if ((live + pending) >= Get_Population_Cap (facility.Kind)) {
		return PRODUCTION_POPULATION_CAP;
	}

	int live_total = Get_Population (facility.Team,BUILDING_PRODUCTION_INFANTRY) +
						  Get_Population (facility.Team,BUILDING_PRODUCTION_GROUND_VEHICLE) +
						  Get_Population (facility.Team,BUILDING_PRODUCTION_AIR_VEHICLE);
	if ((live_total + Pending_Total (facility.Team)) >= _TotalBudget) {
		return PRODUCTION_POPULATION_CAP;
	}

	if (cost > available_credits)						return PRODUCTION_INSUFFICIENT_CREDITS;

	return PRODUCTION_ACCEPTED;
}


CommanderProductionResultType CommanderProductionService::Request_Production(int requesting_player_id,
																									const CommanderFacilityContext & facility,
																									int definition_id,int cost,
																									float build_time_seconds,
																									int available_credits,
																									bool entry_available,
																									int * out_ticket)
{
	if (out_ticket != nullptr) *out_ticket = 0;

	CommanderProductionResultType result = Validate (requesting_player_id,facility,cost,
																	 available_credits,entry_available);
	if (result != PRODUCTION_ACCEPTED) {
		return result;
	}

	CommanderProductionItem item;
	item.FacilityId			= facility.FacilityId;
	item.DefinitionId			= definition_id;
	item.Team					= facility.Team;
	item.Cost					= cost;
	item.BuildTimeSeconds	= (build_time_seconds > 0.0f) ? build_time_seconds : 1.0f;
	item.ElapsedSeconds		= 0.0f;
	item.State					= PRODUCTION_STATE_QUEUED;
	item.Kind					= facility.Kind;
	item.TicketId				= _NextTicket++;

	_Queue.Add (item);

	//	Keep the facility's reported state, so Update has something to pause against even if
	//	nothing calls Set_Facility_Runtime_State again before the next tick.
	Set_Facility_Runtime_State (facility.FacilityId,facility.Alive,facility.ConstructionComplete,
										 facility.Powered,facility.ExitClear);

	if (out_ticket != nullptr) *out_ticket = item.TicketId;
	return PRODUCTION_ACCEPTED;
}


void CommanderProductionService::Set_Facility_Runtime_State(int facility_id,bool alive,
																				bool construction_complete,
																				bool powered,bool exit_clear)
{
	if (facility_id == 0) return;

	FacilityRuntimeState * state = Find_Facility (facility_id);
	if (state == nullptr) {
		FacilityRuntimeState entry;
		entry.InUse			= true;
		entry.FacilityId	= facility_id;
		_Facilities.Add (entry);
		state = &_Facilities [_Facilities.Count () - 1];
	}

	state->Alive					= alive;
	state->ConstructionComplete	= construction_complete;
	state->Powered					= powered;
	state->ExitClear				= exit_clear;
}


void CommanderProductionService::Update(float seconds)
{
	if (seconds <= 0.0f) return;

	/*
	**	One item progresses per facility: the first item in request order whose facility id
	**	matches.  Walking the shared queue rather than a per-facility list keeps request order
	**	global and exact, which is what makes a replay or a save/load restore the same queue.
	*/
	DynamicVectorClass<int>	facilities_advanced;

	for (int i = 0; i < _Queue.Count (); i++) {

		CommanderProductionItem & item = _Queue [i];

		bool already = false;
		for (int f = 0; f < facilities_advanced.Count (); f++) {
			if (facilities_advanced [f] == item.FacilityId) { already = true; break; }
		}
		if (already) {
			item.State = PRODUCTION_STATE_QUEUED;		// behind the head of its facility's line
			continue;
		}
		facilities_advanced.Add (item.FacilityId);

		const FacilityRuntimeState * state = Find_Facility (item.FacilityId);

		bool alive		= (state != nullptr) ? state->Alive					: true;
		bool complete	= (state != nullptr) ? state->ConstructionComplete	: true;
		bool powered	= (state != nullptr) ? state->Powered					: true;
		bool exit_clear	= (state != nullptr) ? state->ExitClear				: true;

		if (!alive || !complete) {
			//	A dead or reverted-to-construction facility is handled by On_Facility_Destroyed,
			//	not silently here -- an item must never complete invisibly, and it must never be
			//	dropped without its refund being reported to someone.
			item.State = PRODUCTION_STATE_QUEUED;
			continue;
		}

		if (!powered) {
			item.State = PRODUCTION_STATE_WAITING_FOR_POWER;		// pause, not cancel
			continue;
		}

		item.State = PRODUCTION_STATE_PRODUCING;
		item.ElapsedSeconds += seconds;

		if (item.ElapsedSeconds < item.BuildTimeSeconds) continue;

		/*
		**	"Do not release the next vehicle while the exit reservation remains blocked."  A
		**	finished item whose exit is occupied holds at BLOCKED_EXIT: it is done building and
		**	is not delivered, which also means the facility's line does not advance past it.
		*/
		if (!exit_clear) {
			item.ElapsedSeconds	= item.BuildTimeSeconds;
			item.State				= PRODUCTION_STATE_BLOCKED_EXIT;
			continue;
		}

		item.State = PRODUCTION_STATE_COMPLETED;
		_Completed.Add (item);
		_Queue.Delete (i);
		i--;
	}
}


int CommanderProductionService::Get_Completed_Count(void)	{ return _Completed.Count (); }

bool CommanderProductionService::Get_Completed_Item(int index,CommanderProductionItem * out)
{
	if (index < 0 || index >= _Completed.Count () || out == nullptr) return false;
	*out = _Completed [index];
	return true;
}

void CommanderProductionService::Clear_Completed(void)
{
	_Completed.Delete_All ();
}


int CommanderProductionService::Get_Queue_Length(int facility_id)
{
	int count = 0;
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].FacilityId == facility_id) count++;
	}
	return count;
}

int CommanderProductionService::Get_Queued_Count(int facility_id,int definition_id)
{
	int count = 0;
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].FacilityId == facility_id && _Queue [i].DefinitionId == definition_id) count++;
	}
	return count;
}

bool CommanderProductionService::Is_Queue_Full(int facility_id,int queue_capacity)
{
	if (facility_id == 0) return false;
	if (queue_capacity <= 0) queue_capacity = 1;
	return Get_Queue_Length (facility_id) >= queue_capacity;
}

bool CommanderProductionService::Get_Queue_Item(int facility_id,int index,CommanderProductionItem * out)
{
	if (out == nullptr) return false;

	int seen = 0;
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].FacilityId != facility_id) continue;
		if (seen == index) { *out = _Queue [i]; return true; }
		seen++;
	}
	return false;
}


int CommanderProductionService::Cancel_Item(int facility_id,int ticket_id)
{
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].FacilityId != facility_id) continue;
		if (_Queue [i].TicketId != ticket_id) continue;

		int refund = _Queue [i].Cost;
		_Queue.Delete (i);
		return refund;
	}
	return 0;
}

int CommanderProductionService::Cancel_Facility_Queue(int facility_id)
{
	int refund = 0;
	for (int i = 0; i < _Queue.Count (); i++) {
		if (_Queue [i].FacilityId != facility_id) continue;
		refund += _Queue [i].Cost;
		_Queue.Delete (i);
		i--;
	}
	return refund;
}

int CommanderProductionService::On_Facility_Destroyed(int facility_id)
{
	int refund = Cancel_Facility_Queue (facility_id);

	/*
	**	A completed-but-undelivered item belonging to a dead facility is dropped too, and
	**	refunded with the rest: Section 45's "prevents invisible completion" means the unit must
	**	not appear after its factory is gone, and a caller draining the completed list has no
	**	way to know the factory died between the two calls.
	*/
	for (int i = 0; i < _Completed.Count (); i++) {
		if (_Completed [i].FacilityId != facility_id) continue;
		refund += _Completed [i].Cost;
		_Completed.Delete (i);
		i--;
	}

	FacilityRuntimeState * state = Find_Facility (facility_id);
	if (state != nullptr) {
		state->Alive = false;
	}

	return refund;
}


void CommanderProductionService::Clear_Selection(void)
{
	_Selection.Delete_All ();
}

bool CommanderProductionService::Add_To_Selection(int unit_id)
{
	if (unit_id == 0) return false;
	if (Is_Selected (unit_id)) return true;
	if (_Selection.Count () >= MAX_SELECTION) return false;
	_Selection.Add (SelectedUnitEntry (unit_id));
	return true;
}

bool CommanderProductionService::Remove_From_Selection(int unit_id)
{
	for (int i = 0; i < _Selection.Count (); i++) {
		if (_Selection [i].UnitId == unit_id) { _Selection.Delete (i); return true; }
	}
	return false;
}

bool CommanderProductionService::Is_Selected(int unit_id)
{
	for (int i = 0; i < _Selection.Count (); i++) {
		if (_Selection [i].UnitId == unit_id) return true;
	}
	return false;
}

int CommanderProductionService::Get_Selection_Count(void)	{ return _Selection.Count (); }

int CommanderProductionService::Get_Selected_Unit(int index)
{
	if (index < 0 || index >= _Selection.Count ()) return 0;
	return _Selection [index].UnitId;
}


bool CommanderProductionService::Can_Order_Unit(int requesting_player_id,int unit_team,bool is_commander_ai)
{
	//	"Human-controlled units receive priority per Section 3A.4" -- and, more fundamentally,
	//	are never the Commander's to drive.  A unit that is not Commander AI refuses every
	//	order regardless of who asked.
	if (!is_commander_ai) return false;

	int commander = Get_Team_Commander (unit_team);
	if (commander == 0) return false;
	return commander == requesting_player_id;
}


bool CommanderProductionService::Issue_Order(int requesting_player_id,int unit_id,int unit_team,
															bool is_commander_ai,CommanderUnitOrderType order,
															const Vector3 & target_point,int target_id)
{
	if (unit_id == 0) return false;
	if (order < 0 || order >= ORDER_COUNT) return false;
	if (!Can_Order_Unit (requesting_player_id,unit_team,is_commander_ai)) return false;

	for (int i = 0; i < _Orders.Count (); i++) {
		if (!_Orders [i].InUse || _Orders [i].UnitId != unit_id) continue;
		_Orders [i].Order			= order;
		_Orders [i].TargetPoint	= target_point;
		_Orders [i].TargetId		= target_id;
		return true;
	}

	UnitOrderRecord record;
	record.InUse			= true;
	record.UnitId			= unit_id;
	record.Order			= order;
	record.TargetPoint	= target_point;
	record.TargetId		= target_id;
	_Orders.Add (record);
	return true;
}


CommanderUnitOrderType CommanderProductionService::Get_Order(int unit_id)
{
	for (int i = 0; i < _Orders.Count (); i++) {
		if (_Orders [i].InUse && _Orders [i].UnitId == unit_id) return _Orders [i].Order;
	}

	//	A unit this service has never been told about defaults to HOLD_POSITION, which is
	//	exactly what Section 45 says a newly produced unit does until ordered.
	return ORDER_HOLD_POSITION;
}

bool CommanderProductionService::Get_Order_Target(int unit_id,Vector3 * out_point,int * out_target_id)
{
	for (int i = 0; i < _Orders.Count (); i++) {
		if (!_Orders [i].InUse || _Orders [i].UnitId != unit_id) continue;
		if (out_point != nullptr)		*out_point		= _Orders [i].TargetPoint;
		if (out_target_id != nullptr)	*out_target_id	= _Orders [i].TargetId;
		return true;
	}
	return false;
}

void CommanderProductionService::Forget_Unit(int unit_id)
{
	Remove_From_Selection (unit_id);
	for (int i = 0; i < _Orders.Count (); i++) {
		if (_Orders [i].InUse && _Orders [i].UnitId == unit_id) {
			_Orders.Delete (i);
			return;
		}
	}
}

int CommanderProductionService::Get_Order_Count(void)
{
	int count = 0;
	for (int i = 0; i < _Orders.Count (); i++) {
		if (_Orders [i].InUse) count++;
	}
	return count;
}


int CommanderProductionService::Get_Item_Count(void)	{ return _Queue.Count (); }

bool CommanderProductionService::Get_Item(int index,CommanderProductionItem * out)
{
	if (index < 0 || index >= _Queue.Count () || out == nullptr) return false;
	*out = _Queue [index];
	return true;
}

void CommanderProductionService::Restore_Item(const CommanderProductionItem & item)
{
	_Queue.Add (item);

	//	Keep the ticket counter ahead of everything restored, so a purchase made after a load
	//	cannot be handed a ticket id a queued item already owns.
	if (item.TicketId >= _NextTicket) {
		_NextTicket = item.TicketId + 1;
	}
}


bool CommanderProductionService::Get_Order_Record(int index,int * out_unit_id,
																  CommanderUnitOrderType * out_order,
																  Vector3 * out_point,int * out_target_id)
{
	int seen = 0;
	for (int i = 0; i < _Orders.Count (); i++) {
		if (!_Orders [i].InUse) continue;
		if (seen == index) {
			if (out_unit_id != nullptr)	*out_unit_id	= _Orders [i].UnitId;
			if (out_order != nullptr)		*out_order		= _Orders [i].Order;
			if (out_point != nullptr)		*out_point		= _Orders [i].TargetPoint;
			if (out_target_id != nullptr)	*out_target_id	= _Orders [i].TargetId;
			return true;
		}
		seen++;
	}
	return false;
}

void CommanderProductionService::Restore_Order(int unit_id,CommanderUnitOrderType order,
															  const Vector3 & target_point,int target_id)
{
	if (unit_id == 0) return;
	if (order < 0 || order >= ORDER_COUNT) return;

	UnitOrderRecord record;
	record.InUse			= true;
	record.UnitId			= unit_id;
	record.Order			= order;
	record.TargetPoint	= target_point;
	record.TargetId		= target_id;
	_Orders.Add (record);
}


const char * CommanderProductionService::Get_Result_Name(CommanderProductionResultType result)
{
	switch (result) {
		case PRODUCTION_ACCEPTED:					return "accepted";
		case PRODUCTION_NO_AUTHORITY:				return "not this team's Commander";
		case PRODUCTION_NO_FACILITY:				return "no facility selected";
		case PRODUCTION_FACILITY_DEAD:			return "facility destroyed";
		case PRODUCTION_FACILITY_CONSTRUCTING:	return "facility under construction";
		case PRODUCTION_FACILITY_UNPOWERED:		return "waiting for power";
		case PRODUCTION_WRONG_FACILITY_KIND:	return "wrong facility kind";
		case PRODUCTION_QUEUE_FULL:				return "queue full";
		case PRODUCTION_POPULATION_CAP:			return "population cap";
		case PRODUCTION_INSUFFICIENT_CREDITS:	return "insufficient credits";
		case PRODUCTION_UNAVAILABLE:				return "unavailable";
		default:											return "unknown";
	}
}

const char * CommanderProductionService::Get_Order_Name(CommanderUnitOrderType order)
{
	switch (order) {
		case ORDER_NONE:					return "none";
		case ORDER_MOVE:					return "move";
		case ORDER_ATTACK_TARGET:		return "attack target";
		case ORDER_ATTACK_MOVE:			return "attack move";
		case ORDER_GUARD:					return "guard";
		case ORDER_HOLD_POSITION:		return "hold position";
		case ORDER_PATROL:				return "patrol";
		case ORDER_FOLLOW:				return "follow";
		case ORDER_REPAIR_TARGET:		return "repair target";
		case ORDER_RETREAT_TO_REPAIR:	return "retreat to repair";
		default:								return "unknown";
	}
}

const char * CommanderProductionService::Get_Item_State_Name(CommanderProductionItemStateType state)
{
	switch (state) {
		case PRODUCTION_STATE_QUEUED:				return "queued";
		case PRODUCTION_STATE_PRODUCING:			return "producing";
		case PRODUCTION_STATE_WAITING_FOR_POWER:	return "waiting for power";
		case PRODUCTION_STATE_BLOCKED_EXIT:		return "blocked exit";
		case PRODUCTION_STATE_COMPLETED:			return "completed";
		default:											return "unknown";
	}
}
