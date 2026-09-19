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
 *                     $Archive:: /Commando/Code/Combat/guardstatecontroller.h                $*
 *                                                                                             *
 *	Roadmap Section 37, Zero Hour / SAGE feature 24 -- AI state/path/guard-retaliation.          *
 *                                                                                             *
 *	The donor audit's own finding, not an assumption: this codebase has no engine-level guard/  *
 *	retaliation concept anywhere in Code/Combat today.  What "Guard" and "Retaliate" mean in    *
 *	stock Renegade is a per-mission scripted behaviour (Code/Scripts/Mission*.cpp), hand-       *
 *	authored per placed unit.  That is fine for a level someone wrote, and it is exactly the    *
 *	thing Phase 39's Commander-produced AI infantry/vehicles cannot rely on -- an AI unit a     *
 *	player purchases at runtime has no mission script watching it.  So Section 37's real job    *
 *	this pass is not "replace Renegade's AI", the thing its own rule forbids -- it is "give     *
 *	OpenW3D a canonical guard/retaliate capability that did not exist at the engine level at    *
 *	all", built by adapting the *behaviour* Zero Hour's AIGuard/AIGuardRetaliate/AIStateMachine  *
 *	describe (state semantics, attacker memory, leash boundary, stuck recovery) rather than by  *
 *	porting SAGE's Thing/AIPlayer machinery, which the rule explicitly forbids.                 *
 *                                                                                             *
 *	This class knows nothing about GameObj, ActionClass or the scene.  It is a pure state       *
 *	machine: feed it damage reports and a position every tick, read back an intent (attack a    *
 *	remembered attacker, return to the guard anchor, or stand), and the caller -- eventually    *
 *	Phase 39's Commander AI unit, not built this pass -- is the one that turns an intent into   *
 *	an ActionClass::Attack/Goto call.  That split is what makes the state machine itself        *
 *	checkable with no device, no scene and no live GameObj, the same discipline every other     *
 *	self-check in this project already applies.                                                 *
 *                                                                                             *
 *	Section 3A.4/3A.6 (human-player priority) are two parameters on Update, not a separate      *
 *	system: distance-to-nearest-human scales how often the expensive part of a think actually   *
 *	runs (far away: cheap and rare; near a human: frequent, because both combat responsiveness  *
 *	and not blocking that player want it), and a replan or a return-to-guard trip that would    *
 *	land on top of a nearby human is suppressed for that tick rather than issued -- this class   *
 *	never proposes a position "through" a human, which is the concrete form of "do not solve    *
 *	congestion by disabling collision or teleporting through players."                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef GUARDSTATECONTROLLER_H
#define GUARDSTATECONTROLLER_H

#include "always.h"
#include "vector3.h"


enum GuardAIStateType
{
	GUARD_STATE_GUARDING = 0,		// at (or returned to) the anchor, nothing to do
	GUARD_STATE_RETALIATING,		// engaging CurrentTargetId
	GUARD_STATE_REACQUIRING,		// target just lost; deciding what happens next
	GUARD_STATE_RETURNING,			// walking back inside the leash
};

enum GuardAIIntentType
{
	GUARD_INTENT_NONE = 0,			// nothing changed; caller keeps doing whatever it was doing
	GUARD_INTENT_STAND,				// hold position (includes yielding to a nearby human)
	GUARD_INTENT_ATTACK,				// engage Get_Current_Target_Id() at the position passed in
	GUARD_INTENT_GOTO,					// move to Get_Intent_Position()
};


class GuardStateControllerClass
{
	public:

		GuardStateControllerClass(void);

		/*
		**	Population bound -- Section 3A.6's "Commander AI population is bounded by
		**	configurable team/map/server limits", expressed as the one number every controller
		**	instance shares: how many may be Activate()'d at once.  Refuses rather than grows.
		*/
		static bool		Set_Max_Active_Controllers(int count);
		static int		Get_Max_Active_Controllers(void);
		static int		Get_Active_Controller_Count(void);
		static int		Get_Refusal_Count(void);

		/*
		**	Call once a Combat tick, before any controller's Update this frame, to reset the
		**	shared replan budget -- "expensive path replans are staggered/budgeted" is this
		**	counter refusing a replan past its per-tick share rather than letting every stuck
		**	unit in the level replan on the same frame.
		*/
		static void		Begin_Tick(void);
		static void		Set_Max_Replans_Per_Tick(int count);
		static int		Get_Replans_This_Tick(void);

		bool				Activate(const Vector3 & guard_anchor,float guard_radius);
		void				Deactivate(void);
		bool				Is_Active(void) const		{ return _Active; }

		GuardAIStateType	Get_State(void) const		{ return _State; }
		int					Get_Current_Target_Id(void) const		{ return _CurrentTargetId; }
		Vector3				Get_Intent_Position(void) const			{ return _IntentPosition; }

		/*
		**	A hostile hit landed.  Bounded attacker memory (GUARD_ATTACKER_MEMORY_MAX entries,
		**	oldest evicted first) is "attacker memory / target reacquisition"; if nothing is
		**	currently being engaged this becomes the new target immediately -- "retaliation
		**	after hostile damage."
		*/
		void				Report_Damage(int attacker_id,const Vector3 & attacker_position,float now);

		//	The current target became invalid (destroyed, despawned, out of world).  Public so
		//	an event-driven caller can report it the instant it knows, rather than waiting for
		//	Update's own target_valid check to notice on the next poll.
		void				Report_Target_Lost(float now);

		/*
		**	Advances the controller and returns what it wants done this tick.
		**
		**	current_position     the guarded object's real position; feeds stuck detection.
		**	target_valid/
		**	target_position      what the caller's own target tracking currently believes about
		**	                     Get_Current_Target_Id(); this class looks nothing up itself.
		**	nearest_human_distance/
		**	nearest_human_position   Section 3A.4/3A.6's input; pass a negative distance (or a
		**	                     null position) when no human is nearby.
		*/
		GuardAIIntentType	Update(float now,float seconds,const Vector3 & current_position,
											bool target_valid,const Vector3 & target_position,
											const Vector3 * nearest_human_position,float nearest_human_distance);

		int				Get_Stuck_Count(void) const		{ return _StuckCount; }
		int				Get_Replan_Count(void) const		{ return _ReplanCount; }
		int				Get_Yield_Count(void) const		{ return _YieldCount; }

	private:

		enum { GUARD_ATTACKER_MEMORY_MAX = 4 };

		struct AttackerMemoryEntry
		{
			AttackerMemoryEntry(void) : InUse(false),Id(0),Position(0,0,0),Time(0.0f) { }
			bool		InUse;
			int			Id;
			Vector3	Position;
			float		Time;
		};

		void		Remember_Attacker(int attacker_id,const Vector3 & position,float now);
		bool		Find_Freshest_Attacker(float now,int * out_id,Vector3 * out_position) const;
		void		Enter_Retaliating(int target_id,const Vector3 & target_position);

		bool					_Active;
		GuardAIStateType	_State;

		Vector3				_GuardAnchor;
		float					_GuardRadius;

		int					_CurrentTargetId;
		Vector3				_CurrentTargetPosition;

		AttackerMemoryEntry	_Memory [GUARD_ATTACKER_MEMORY_MAX];

		Vector3				_LastThinkPosition;
		float					_StuckTimer;
		int					_StuckCount;
		int					_ReplanCount;
		int					_YieldCount;

		float					_NextThinkTime;
		Vector3				_IntentPosition;
};

#endif // GUARDSTATECONTROLLER_H
