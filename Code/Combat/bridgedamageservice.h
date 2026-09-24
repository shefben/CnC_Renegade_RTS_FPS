/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/bridgedamageservice.h                 $*
 *                                                                                             *
 *	P15 / D-001: bridges take gameplay damage.  The engine's own explosion damage pass           *
 *	(ExplosionManager::Create_Explosion_At, server only) hands each blast here; the service      *
 *	finds the spans in reach, accumulates damage on each, and moves a span through              *
 *	BridgeSystem's own damage states -- damaged, critical, broken -- never backwards, and to     *
 *	broken exactly once.  BridgeSystem::Set_Section_State rebuilds that span's geometry, so      *
 *	collision follows.  A break then rebuilds navigation and chokepoints, re-evaluates           *
 *	logistics routes, and sends BridgeSystem's compact state block to every synchronised client *
 *	through WorldSyncTransport -- the same block a late joiner and a save receive.  No bridge    *
 *	is a replicated game object.                                                                 *
 *                                                                                             *
 *	Bullets are not counted: only explosions carry a radius and a warhead's worth of force       *
 *	into the world, which is also what brings a bridge down in every game this is modelled on.   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BRIDGEDAMAGESERVICE_H
#define BRIDGEDAMAGESERVICE_H

#include "always.h"
#include "vector3.h"

class BridgeDamageService
{
	public:

		//	Damage a span absorbs before it falls.  A span is damaged past a third of it and
		//	critical past two thirds.
		static const float	SPAN_HIT_POINTS;

		static void		Init(void);
		static void		Reset(void);

		//	One explosion.  Returns the number of spans whose state changed.  Server only; a
		//	client learns the result from the state block.
		static int		Apply_Explosion(const Vector3 & position,float radius,float strength);

		//	What a span has absorbed so far, for the console and the self check.
		static float	Get_Span_Damage(int bridge_id,int section_index);

		//	The state a span should be in after absorbing `damage`.  Pure.
		static int		State_For_Damage(float damage);

		static int		Get_Break_Count(void);
};

#endif // BRIDGEDAMAGESERVICE_H
