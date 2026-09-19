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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/guardstatecontroller.cpp              $*
 *                                                                                             *
 *	Roadmap Section 37, Zero Hour / SAGE feature 24 -- AI state/path/guard-retaliation.          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "guardstatecontroller.h"


/*
**	How far past its guard radius a retaliating unit may chase before Section 37's "guard area
**	boundary" forces it home regardless of whether the target is still alive.
*/
static const float	GUARD_LEASH_MULTIPLIER			= 1.5f;

/*
**	How long a remembered attacker stays worth reacquiring.
*/
static const float	GUARD_ATTACKER_RETENTION		= 8.0f;

/*
**	Stuck detection: less than this much displacement over this long, while the controller has
**	proposed a GOTO, counts as stuck.  World units are Renegade's own (roughly feet); a soldier
**	walking normally covers this in a small fraction of the window.
*/
static const float	GUARD_STUCK_EPSILON				= 25.0f;
static const float	GUARD_STUCK_WINDOW				= 2.0f;

/*
**	Never propose a position this close to a human -- "do not solve congestion by disabling
**	collision or teleporting through players" made concrete as a distance this class will not
**	send anyone inside of.
*/
static const float	GUARD_HUMAN_YIELD_RADIUS		= 5.0f;

/*
**	Update-frequency throttle -- Section 3A.6.  Near a human, or while actually retaliating,
**	think every tenth of a second; otherwise once a second is enough for a guard standing
**	still watching nothing happen.
*/
static const float	GUARD_THINK_INTERVAL_NEAR		= 0.1f;
static const float	GUARD_THINK_INTERVAL_FAR		= 1.0f;
static const float	GUARD_THINK_NEAR_DISTANCE		= 20.0f;


static int		_ActiveCount				= 0;
static int		_MaxActiveControllers	= 64;
static int		_RefusalCount				= 0;

static int		_MaxReplansPerTick		= 8;
static int		_ReplansThisTick			= 0;


bool GuardStateControllerClass::Set_Max_Active_Controllers(int count)
{
	if (count < _ActiveCount) {
		return false;			// refuse to set a ceiling under what is already active
	}
	_MaxActiveControllers = count;
	return true;
}
int GuardStateControllerClass::Get_Max_Active_Controllers(void)		{ return _MaxActiveControllers; }
int GuardStateControllerClass::Get_Active_Controller_Count(void)	{ return _ActiveCount; }
int GuardStateControllerClass::Get_Refusal_Count(void)					{ return _RefusalCount; }

void GuardStateControllerClass::Begin_Tick(void)								{ _ReplansThisTick = 0; }
void GuardStateControllerClass::Set_Max_Replans_Per_Tick(int count)	{ _MaxReplansPerTick = (count < 0) ? 0 : count; }
int  GuardStateControllerClass::Get_Replans_This_Tick(void)				{ return _ReplansThisTick; }


GuardStateControllerClass::GuardStateControllerClass(void) :
	_Active(false),
	_State(GUARD_STATE_GUARDING),
	_GuardAnchor(0,0,0),
	_GuardRadius(0.0f),
	_CurrentTargetId(0),
	_CurrentTargetPosition(0,0,0),
	_LastThinkPosition(0,0,0),
	_StuckTimer(0.0f),
	_StuckCount(0),
	_ReplanCount(0),
	_YieldCount(0),
	_NextThinkTime(0.0f),
	_IntentPosition(0,0,0)
{
}


bool GuardStateControllerClass::Activate(const Vector3 & guard_anchor,float guard_radius)
{
	if (_Active) {
		return true;			// idempotent
	}
	if (_ActiveCount >= _MaxActiveControllers) {
		_RefusalCount++;
		return false;
	}

	_Active					= true;
	_State					= GUARD_STATE_GUARDING;
	_GuardAnchor			= guard_anchor;
	_GuardRadius			= (guard_radius > 0.0f) ? guard_radius : 1.0f;
	_CurrentTargetId		= 0;
	_CurrentTargetPosition = guard_anchor;
	for (int i = 0; i < GUARD_ATTACKER_MEMORY_MAX; i++) {
		_Memory [i] = AttackerMemoryEntry();
	}
	_LastThinkPosition	= guard_anchor;
	_StuckTimer				= 0.0f;
	_StuckCount				= 0;
	_ReplanCount			= 0;
	_YieldCount				= 0;
	_NextThinkTime			= 0.0f;
	_IntentPosition		= guard_anchor;

	_ActiveCount++;
	return true;
}

void GuardStateControllerClass::Deactivate(void)
{
	if (!_Active) {
		return;
	}
	_Active = false;
	_ActiveCount--;
}


void GuardStateControllerClass::Remember_Attacker(int attacker_id,const Vector3 & position,float now)
{
	for (int i = 0; i < GUARD_ATTACKER_MEMORY_MAX; i++) {
		if (_Memory [i].InUse && _Memory [i].Id == attacker_id) {
			_Memory [i].Position	= position;
			_Memory [i].Time		= now;
			return;
		}
	}

	int slot = -1;
	for (int i = 0; i < GUARD_ATTACKER_MEMORY_MAX; i++) {
		if (!_Memory [i].InUse) { slot = i; break; }
	}
	if (slot < 0) {
		int oldest = 0;
		for (int i = 1; i < GUARD_ATTACKER_MEMORY_MAX; i++) {
			if (_Memory [i].Time < _Memory [oldest].Time) oldest = i;
		}
		slot = oldest;
	}

	_Memory [slot].InUse		= true;
	_Memory [slot].Id			= attacker_id;
	_Memory [slot].Position	= position;
	_Memory [slot].Time		= now;
}

bool GuardStateControllerClass::Find_Freshest_Attacker(float now,int * out_id,Vector3 * out_position) const
{
	int best = -1;
	for (int i = 0; i < GUARD_ATTACKER_MEMORY_MAX; i++) {
		if (!_Memory [i].InUse) continue;
		if ((now - _Memory [i].Time) > GUARD_ATTACKER_RETENTION) continue;
		if (best < 0 || _Memory [i].Time > _Memory [best].Time) best = i;
	}
	if (best < 0) return false;

	*out_id			= _Memory [best].Id;
	*out_position	= _Memory [best].Position;
	return true;
}

void GuardStateControllerClass::Enter_Retaliating(int target_id,const Vector3 & target_position)
{
	_State						= GUARD_STATE_RETALIATING;
	_CurrentTargetId			= target_id;
	_CurrentTargetPosition	= target_position;
}


void GuardStateControllerClass::Report_Damage(int attacker_id,const Vector3 & attacker_position,float now)
{
	if (!_Active) return;

	Remember_Attacker(attacker_id,attacker_position,now);

	if (_State != GUARD_STATE_RETALIATING) {
		Enter_Retaliating(attacker_id,attacker_position);
	}
}

void GuardStateControllerClass::Report_Target_Lost(float now)
{
	if (!_Active) return;
	(void)now;

	//	The target we were just fighting is confirmed gone; forget it specifically so
	//	reacquire cannot immediately pick the same dead id back up.
	for (int i = 0; i < GUARD_ATTACKER_MEMORY_MAX; i++) {
		if (_Memory [i].InUse && _Memory [i].Id == _CurrentTargetId) {
			_Memory [i].InUse = false;
		}
	}

	_CurrentTargetId	= 0;
	_State				= GUARD_STATE_REACQUIRING;
}


GuardAIIntentType GuardStateControllerClass::Update(float now,float seconds,const Vector3 & current_position,
																	  bool target_valid,const Vector3 & target_position,
																	  const Vector3 * nearest_human_position,
																	  float nearest_human_distance)
{
	if (!_Active) {
		return GUARD_INTENT_NONE;
	}

	if (_State == GUARD_STATE_RETALIATING) {
		if (!target_valid) {
			Report_Target_Lost(now);
		} else {
			_CurrentTargetPosition = target_position;
		}
	}

	bool near_human = (nearest_human_position != nullptr) && (nearest_human_distance >= 0.0f) &&
							 (nearest_human_distance < GUARD_THINK_NEAR_DISTANCE);
	bool full_think = (now >= _NextThinkTime) || (_State == GUARD_STATE_RETALIATING);
	if (!full_think) {
		return GUARD_INTENT_NONE;
	}
	_NextThinkTime = now + ((_State == GUARD_STATE_RETALIATING || near_human) ?
										GUARD_THINK_INTERVAL_NEAR : GUARD_THINK_INTERVAL_FAR);

	GuardAIIntentType	intent = GUARD_INTENT_NONE;
	Vector3					proposed_goto (0.0f,0.0f,0.0f);
	bool						have_proposal = false;

	switch (_State) {

		case GUARD_STATE_GUARDING:
			intent = GUARD_INTENT_NONE;
			break;

		case GUARD_STATE_RETALIATING: {
			float dist_from_anchor = (current_position - _GuardAnchor).Length();
			if (dist_from_anchor > _GuardRadius * GUARD_LEASH_MULTIPLIER) {
				_State			= GUARD_STATE_RETURNING;
				proposed_goto	= _GuardAnchor;
				have_proposal	= true;
				intent			= GUARD_INTENT_GOTO;
			} else {
				intent = GUARD_INTENT_ATTACK;
			}
			break;
		}

		case GUARD_STATE_REACQUIRING: {
			int freshest_id;
			Vector3 freshest_position;
			if (Find_Freshest_Attacker(now,&freshest_id,&freshest_position)) {
				Enter_Retaliating(freshest_id,freshest_position);
				intent = GUARD_INTENT_ATTACK;
			} else if ((current_position - _GuardAnchor).Length() > _GuardRadius) {
				_State			= GUARD_STATE_RETURNING;
				proposed_goto	= _GuardAnchor;
				have_proposal	= true;
				intent			= GUARD_INTENT_GOTO;
			} else {
				_State = GUARD_STATE_GUARDING;
				intent = GUARD_INTENT_STAND;
			}
			break;
		}

		case GUARD_STATE_RETURNING: {
			if ((current_position - _GuardAnchor).Length() <= _GuardRadius * 0.25f) {
				_State = GUARD_STATE_GUARDING;
				intent = GUARD_INTENT_STAND;
			} else {
				proposed_goto	= _GuardAnchor;
				have_proposal	= true;
				intent			= GUARD_INTENT_GOTO;
			}
			break;
		}
	}

	//	Stuck detection/recovery, budgeted -- only meaningful for the GOTOs this class itself
	//	proposes (RETURNING, or the leash break-off above); a RETALIATING attack's own pathing
	//	is the caller's ActionClass::Attack, outside this class's view.
	float moved = (current_position - _LastThinkPosition).Length();
	if (have_proposal) {
		_StuckTimer = (moved < GUARD_STUCK_EPSILON) ? (_StuckTimer + seconds) : 0.0f;

		if (_StuckTimer >= GUARD_STUCK_WINDOW) {
			_StuckCount++;
			_StuckTimer = 0.0f;

			if (_ReplansThisTick < _MaxReplansPerTick) {
				//	No real path planner sits behind this class; a perpendicular offset is the
				//	honest stand-in for "try something other than the exact line that just
				//	failed" -- roadmap's own "path retry/replan improvements", at the scope
				//	this class actually owns.
				Vector3 to_goal (proposed_goto.X - current_position.X,proposed_goto.Y - current_position.Y,0.0f);
				Vector3 nudge (-to_goal.Y,to_goal.X,0.0f);
				float nudge_len = nudge.Length();
				if (nudge_len > 0.01f) {
					nudge = nudge * (1.0f / nudge_len);
				}
				proposed_goto = proposed_goto + nudge * (_GuardRadius * 0.25f);

				_ReplanCount++;
				_ReplansThisTick++;
			}
			//	else: this tick's shared replan budget is spent; try again next tick, which is
			//	the staggering the roadmap asks for -- not every stuck unit replans at once.
		}
	} else {
		_StuckTimer = 0.0f;
	}

	//	Human yield: never hand back a position within personal space of a nearby human.
	if (nearest_human_position != nullptr && nearest_human_distance >= 0.0f) {
		Vector3 check_point = have_proposal ? proposed_goto : current_position;
		if ((check_point - *nearest_human_position).Length() < GUARD_HUMAN_YIELD_RADIUS) {
			_YieldCount++;
			intent			= GUARD_INTENT_STAND;
			have_proposal	= false;
		}
	}

	_IntentPosition		= have_proposal ? proposed_goto : current_position;
	_LastThinkPosition	= current_position;

	return intent;
}
