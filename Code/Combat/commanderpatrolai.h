/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderpatrolai.h                   $*
 *                                                                                             *
 *	Roadmap Section 45's ORDER_PATROL as a real back-and-forth: a unit walks to the ordered     *
 *	point, then back to where the patrol started, and on, until another order replaces it.      *
 *	The two points arrive on cCommanderOrderEvent (its target and its second point); this file  *
 *	keeps one small leg record per patrolling unit and, each server tick, sends the next Goto   *
 *	through the same ScriptEngine call every other Commander order uses.  Server only, like     *
 *	CommanderGuardAI, which it mirrors.                                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPATROLAI_H
#define COMMANDERPATROLAI_H

#include "always.h"
#include "vector3.h"

class CommanderPatrolAI
{
	public:

		//	A unit this close to its leg's end, in the ground plane, has arrived.
		static const float	ARRIVE_RADIUS;

		//	A leg the unit has not finished in this long is abandoned for the other one, so a unit
		//	that cannot reach a point (blocked, or ordered onto a cliff) keeps patrolling rather
		//	than standing still for the rest of the match.
		static const float	LEG_TIMEOUT_SECONDS;

		static void		Init(void);
		static void		Shutdown(void);

		//	Starts the patrol, heading for `point_b` first.  Replaces any patrol the unit had.
		static void		Patrol(int unit_id,const Vector3 & point_a,const Vector3 & point_b);
		static void		Stop_Patrolling(int unit_id);
		static bool		Is_Patrolling(int unit_id);
		static int		Get_Active_Count(void);

		//	Which leg a unit should now be on (0 = heading to A, 1 = heading to B).  Pure, so the
		//	turn-around rule is checkable without a unit.
		static int		Next_Leg(const Vector3 & position,const Vector3 & point_a,const Vector3 & point_b,
										int current_leg,float seconds_on_leg);

		static void		Update(float seconds);
};

#endif // COMMANDERPATROLAI_H
