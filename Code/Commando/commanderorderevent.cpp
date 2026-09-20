/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderorderevent.cpp             $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- the client->server half of Commander unit orders.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderorderevent.h"

#include "action.h"
#include "actionparams.h"
#include "apppackettypes.h"
#include "cnetwork.h"
#include "commandercommandpalette.h"
#include "commanderguardai.h"
#include "commanderproduction.h"
#include "commanderwaypoint.h"
#include "commanderwaypointevent.h"
#include "gameobjmanager.h"
#include "networkobjectfactory.h"
#include "networkobjectmgr.h"
#include "physicalgameobj.h"
#include "scriptcommands.h"
#include "smartgameobj.h"
#include "soldier.h"

#include <string.h>


DECLARE_NETWORKOBJECT_FACTORY (cCommanderOrderEvent,NETCLASSID_COMMANDERORDEREVENT);


/*
**	Section 45's own translation of an accepted order into canonical Renegade AI, so
**	"Commander-purchased infantry are normal AI SoldierGameObj paths" holds all the way
**	through: CommanderProductionService only records that an order was issued, and this is
**	the one place that turns the recorded verb into an ActionClass call, using the exact
**	same free functions Code/Combat/scriptcommands.cpp gives mission scripts.  Priority 100
**	matches every scripted command in this tree (Mission00.cpp, DrMobius.cpp, ...) -- high
**	enough to override a unit's own idle/innate behaviour, which is the entire point of a
**	Commander order.  Observer id 0 means nothing is listening for completion, which
**	ActionClass::Notify_Completed already treats as a safe no-op.
*/
static void	Dispatch_Order (PhysicalGameObj * unit,CommanderUnitOrderType order,
									  const Vector3 & target_point,int target_id)
{
	/*
	**	Above every scripted command in this tree.  Mission scripts issue at 100
	**	(Mission00.cpp, DrMobius.cpp, ...) and a unit's own innate behaviour sits below that; a
	**	Commander order has to displace both, because "the AI drops what it is doing" is the
	**	whole contract of giving one.  Paired with the Action_Reset below, which clears the
	**	current action list outright rather than relying on the new action outranking it.
	*/
	const float ORDER_PRIORITY = 200.0f;
	const float GUARD_RADIUS	 = 15.0f;

	SmartGameObj * smart = unit->As_SmartGameObj ();
	if (smart == nullptr) return;		// no AI action layer on this object -- nothing to drive

	if (order != ORDER_NONE) {
		//	Any fresh order, ORDER_GUARD included, starts from a clean slate: without this, a
		//	unit given MOVE after being told to guard would keep fighting its own leftover
		//	GuardStateControllerClass, which drives ActionClass every server tick regardless of
		//	whatever this dispatch just set.  A no-op for a unit that was not guarding.
		CommanderGuardAI::Stop_Guarding (unit->Get_ID ());

		//	Drop whatever it was doing before the new action is pushed.  Without this the unit
		//	finishes its current action first whenever the two are compatible, which is exactly
		//	the "it ignored me" a Commander must never see.
		ScriptEngine::Action_Reset (unit,ORDER_PRIORITY);
	}

	ActionParamsStruct params;
	params.Set_Basic (0,ORDER_PRIORITY,0);

	switch (order) {

		case ORDER_NONE:
			//	Not a command -- nothing to dispatch.  Leaves whatever the unit was already
			//	doing alone, rather than stopping it.
			return;

		case ORDER_MOVE:
		case ORDER_ATTACK_MOVE:
			//	Attack-move relies on this engine's own perception/threat response -- a
			//	SmartGameObj already breaks off to engage a target it perceives regardless of
			//	its current action -- rather than a second attack-move state machine this
			//	codebase has never had.
			params.Set_Movement (target_point,1.0f,1.5f,false);
			ScriptEngine::Action_Goto (unit,params);
			break;

		case ORDER_ATTACK_TARGET: {
			ScriptableGameObj * target = (target_id != 0) ? GameObjManager::Find_ScriptableGameObj (target_id) : nullptr;
			if (target != nullptr) {
				params.Set_Attack (target,40.0f,0.0f,true);
			} else {
				params.Set_Attack (target_point,40.0f,0.0f,true);
			}
			ScriptEngine::Action_Attack (unit,params);
			break;
		}

		case ORDER_FOLLOW: {
			ScriptableGameObj * target = (target_id != 0) ? GameObjManager::Find_ScriptableGameObj (target_id) : nullptr;
			if (target != nullptr) {
				params.Set_Movement (target,1.0f,3.0f,false);
				ScriptEngine::Action_Follow_Input (unit,params);
			} else {
				ScriptEngine::Action_Reset (unit,ORDER_PRIORITY);
			}
			break;
		}

		case ORDER_PATROL:
			//	The wire format this event carries (Section 45's own struct) has one point, not
			//	a waypath -- a real back-and-forth patrol needs a second point nothing here has.
			//	Walking to the one point given is strictly more than the nothing this order did
			//	before; recorded as a limit, not silently upgraded into a loop it cannot run.
			params.Set_Movement (target_point,1.0f,1.5f,false);
			ScriptEngine::Action_Goto (unit,params);
			break;

		case ORDER_REPAIR_TARGET:
		case ORDER_RETREAT_TO_REPAIR:
			//	Neither is a distinct AI action in this codebase -- RepairBayGameObj repairs by
			//	proximity once a vehicle sits inside it -- so both orders walk to the named
			//	point rather than inventing a canonical "repair" primitive that does not exist.
			params.Set_Movement (target_point,1.0f,1.5f,false);
			ScriptEngine::Action_Goto (unit,params);
			break;

		case ORDER_GUARD:
			//	CommanderGuardAI (Code/Combat/commanderguardai.h) owns the state machine from
			//	here -- Section 37's GuardStateControllerClass, activated at the clicked point,
			//	ticked once a server frame in CombatManager::Think, translating its own intent
			//	into the same ScriptEngine calls every other case here uses.  Nothing further to
			//	dispatch this instant; the first Attack/Goto/Reset call comes from that tick.
			CommanderGuardAI::Guard (unit->Get_ID (),target_point,GUARD_RADIUS);
			break;

		case ORDER_HOLD_POSITION:
		default:
			ScriptEngine::Action_Reset (unit,ORDER_PRIORITY);
			break;
	}
}


/***********************************************************************************************
 * Formation_Offset -- where one unit of a group stands, relative to the ordered point            *
 *                                                                                               *
 *	Computed on the server and nowhere else.  A client could work out the same offsets, but then  *
 *	the packet would have to carry a destination per unit, and the server would be accepting      *
 *	positions a client chose -- which is the one thing Section 45's validation list exists to     *
 *	prevent.  Sending the shape and deriving the positions here keeps the wire format one point.  *
 *                                                                                               *
 *	`forward` is the direction the group is heading, so a line forms across the approach and a    *
 *	wedge points along it.  A degenerate forward (ordering a group onto the spot it already       *
 *	occupies) falls back to world X, which is arbitrary but stable -- the alternative is a        *
 *	formation whose orientation flickers with floating-point noise.                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static Vector3	Formation_Offset (int formation,int index,int count,const Vector3 & forward)
{
	const float SPACING = 3.0f;

	if ((formation == FORMATION_NONE) || (count <= 1)) {
		return Vector3 (0.0f,0.0f,0.0f);
	}

	Vector3 heading = forward;
	heading.Z = 0.0f;
	if (heading.Length2 () < 0.0001f) {
		heading.Set (1.0f,0.0f,0.0f);
	}
	heading.Normalize ();

	Vector3 right (heading.Y,-heading.X,0.0f);

	switch (formation) {

		case FORMATION_LINE: {
			//	Centred on the point: index 0 is the leftmost, not the middle, so the group's
			//	centre of mass ends up where the Commander actually clicked.
			float slot = (float)index - ((float)(count - 1) * 0.5f);
			return right * (slot * SPACING);
		}

		case FORMATION_WEDGE: {
			//	Alternating sides, each pair one rank further back: 0 at the point, 1 and 2
			//	behind and out, and so on.
			int rank = (index + 1) / 2;
			float side = ((index % 2) == 1) ? 1.0f : -1.0f;
			if (index == 0) { side = 0.0f; }
			return (right * (side * (float)rank * SPACING)) -
					 (heading * ((float)rank * SPACING * 0.75f));
		}

		case FORMATION_SCATTER: {
			/*
			**	A ring, one unit per slot, at a radius that grows with the group so the spacing
			**	between neighbours stays roughly constant however many there are.  The point of
			**	scatter is that no two units are close enough for one blast to reach both, so the
			**	gap is what has to be held, not the radius.
			*/
			float radius = (SPACING * 1.6f * (float)count) / (2.0f * WWMATH_PI);
			if (radius < SPACING) radius = SPACING;

			float angle = (float)index * (2.0f * WWMATH_PI / (float)count);
			return Vector3 (WWMath::Cos (angle) * radius,WWMath::Sin (angle) * radius,0.0f);
		}

		default:
			return Vector3 (0.0f,0.0f,0.0f);
	}
}


cCommanderOrderEvent::cCommanderOrderEvent(void) :
	SenderId(0),
	Order(0),
	Formation(0),
	TargetPoint(0.0f,0.0f,0.0f),
	TargetId(0),
	UnitCount(0)
{
	::memset (UnitIds,0,sizeof (UnitIds));
	Set_App_Packet_Type (APPPACKETTYPE_COMMANDERORDEREVENT);
}


void cCommanderOrderEvent::Init(int order,const Vector3 & target_point,int target_id,
										  const int * unit_ids,int unit_count,int formation)
{
	WWASSERT (cNetwork::I_Am_Client ());

	SenderId		= cNetwork::Get_My_Id ();
	Order			= order;
	Formation	= formation;
	TargetPoint	= target_point;
	TargetId		= target_id;

	if (unit_count < 0) unit_count = 0;
	if (unit_count > MAX_UNITS) unit_count = MAX_UNITS;
	UnitCount = unit_count;

	for (int i = 0; i < UnitCount; i++) {
		UnitIds [i] = (unit_ids != nullptr) ? unit_ids [i] : 0;
	}

	Set_Network_ID (NetworkObjectMgrClass::Get_New_Client_ID ());

	if (cNetwork::I_Am_Server ()) {
		Act ();
	} else {
		Set_Object_Dirty_Bit (0,BIT_CREATION,true);
	}
}


void cCommanderOrderEvent::Export_Creation(BitStreamClass & packet)
{
	WWASSERT (cNetwork::I_Am_Client ());

	cNetEvent::Export_Creation (packet);

	packet.Add (SenderId);
	packet.Add (Order);
	packet.Add (Formation);
	packet.Add (TargetPoint.X);
	packet.Add (TargetPoint.Y);
	packet.Add (TargetPoint.Z);
	packet.Add (TargetId);
	packet.Add (UnitCount);
	for (int i = 0; i < UnitCount; i++) {
		packet.Add (UnitIds [i]);
	}

	Set_Delete_Pending ();
}


void cCommanderOrderEvent::Import_Creation(BitStreamClass & packet)
{
	cNetEvent::Import_Creation (packet);

	WWASSERT (cNetwork::I_Am_Server ());

	packet.Get (SenderId);
	packet.Get (Order);
	packet.Get (Formation);
	packet.Get (TargetPoint.X);
	packet.Get (TargetPoint.Y);
	packet.Get (TargetPoint.Z);
	packet.Get (TargetId);
	packet.Get (UnitCount);

	//	Clamped before it is used as a loop bound, not after: a hostile packet naming a hundred
	//	units must read exactly MAX_UNITS ids or the stream would desync for everything after it.
	if (UnitCount < 0) UnitCount = 0;
	if (UnitCount > MAX_UNITS) UnitCount = MAX_UNITS;

	for (int i = 0; i < UnitCount; i++) {
		packet.Get (UnitIds [i]);
	}

	Act ();
}


void cCommanderOrderEvent::Act(void)
{
	WWASSERT (cNetwork::I_Am_Server ());

	SoldierGameObj * soldier = GameObjManager::Find_Soldier_Of_Client_ID (SenderId);
	if (soldier == nullptr) return;

	int team = soldier->Get_Player_Type ();
	if (CommanderProductionService::Get_Team_Commander (team) != SenderId) {
		return;
	}

	/*
	**	The group's heading, needed before any unit is moved: a formation is oriented by where the
	**	group is coming from, and once the first unit has been given its destination the centroid
	**	is no longer the one the rest were measured against.  Averaged over the units that are
	**	still alive, so a selection half of which died in flight still forms up sensibly.
	*/
	Vector3 centroid (0.0f,0.0f,0.0f);
	int alive = 0;
	for (int i = 0; i < UnitCount; i++) {
		if (UnitIds [i] == 0) continue;
		PhysicalGameObj * probe = GameObjManager::Find_PhysicalGameObj (UnitIds [i]);
		if (probe == nullptr) continue;
		Vector3 probe_position;
		probe->Get_Position (&probe_position);
		centroid += probe_position;
		alive++;
	}
	if (alive > 0) {
		centroid *= (1.0f / (float)alive);
	}

	Vector3 group_forward = TargetPoint - centroid;

	//	Counted separately from the loop index: a formation slot belongs to a unit that actually
	//	received the order, so refused and dead ones must not leave gaps in the shape.
	int formation_slot = 0;
	bool any_human_told = false;

	for (int i = 0; i < UnitCount; i++) {

		int unit_id = UnitIds [i];
		if (unit_id == 0) continue;

		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (unit_id);
		if (unit == nullptr) continue;			// died between the click and the packet

		/*
		**	Whether this unit is Commander AI rather than a human's, looked up rather than taken
		**	from the packet.  A soldier with a controlling player behind it is a human-driven
		**	unit and refuses every Commander order -- Section 3A.4's priority rule, enforced at
		**	the one place an order can enter the server.
		**
		**	Commander-produced AI has no controlling player, which is exactly what distinguishes
		**	it; Section 45's "AI consumes no fake human player slots" is what makes that test
		**	sound rather than a heuristic.
		*/
		bool is_commander_ai = true;

		SoldierGameObj * unit_soldier = unit->As_SoldierGameObj ();
		if ((unit_soldier != nullptr) && (unit_soldier->Get_Player_Data () != nullptr)) {
			is_commander_ai = false;
		}

		bool accepted = CommanderProductionService::Issue_Order (SenderId,unit_id,unit->Get_Player_Type (),
																					is_commander_ai,
																					(CommanderUnitOrderType)Order,
																					TargetPoint,TargetId);

		/*
		**	A human-driven unit is refused here and always will be -- Section 3A.4 -- but refusing
		**	it is not the same as ignoring it.  The Commander selected that player deliberately,
		**	and the order becomes a waypoint marker they can see and choose to act on, which is
		**	the only instruction this engine may give a person.  Noted once for the whole packet;
		**	the marker is a point, not one per player.
		*/
		if (!is_commander_ai) {
			any_human_told = true;
			continue;
		}

		//	The service records the order regardless of caller; only an *accepted* one drives
		//	the actual AI -- refused orders (wrong team, wrong Commander) must change nothing
		//	about what the unit is doing, the same as if the packet had never arrived.
		if (!accepted) continue;

		Vector3 destination = TargetPoint +
									 Formation_Offset (Formation,formation_slot,(alive > 0) ? alive : 1,
																group_forward);
		formation_slot++;

		Dispatch_Order (unit,(CommanderUnitOrderType)Order,destination,TargetId);
	}

	/*
	**	The marker.  Raised whenever a human was named in the order -- that is the case it exists
	**	for -- and also for any order the Commander gave at all, because a Commander watching
	**	their own AI move wants the same confirmation of where they just sent it.  Its kind is
	**	derived from the order rather than sent, so the wire format stays one enum.
	*/
	{
		const float WAYPOINT_LIFETIME = any_human_told ? 30.0f : 12.0f;

		CommanderWaypointKindType kind = WAYPOINT_MOVE;
		switch ((CommanderUnitOrderType)Order) {
			case ORDER_ATTACK_TARGET:
			case ORDER_ATTACK_MOVE:		kind = WAYPOINT_ATTACK;	break;
			case ORDER_GUARD:
			case ORDER_HOLD_POSITION:	kind = WAYPOINT_DEFEND;	break;
			default:							kind = WAYPOINT_MOVE;	break;
		}

		if ((CommanderUnitOrderType)Order != ORDER_NONE) {
			cCommanderWaypointEvent * event = new cCommanderWaypointEvent;
			event->Init (team,(int)kind,TargetPoint,WAYPOINT_LIFETIME,Get_Network_ID ());
		}
	}
}
