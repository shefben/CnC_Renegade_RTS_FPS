/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderwaypointevent.h            $*
 *                                                                                             *
 *	Roadmap Sections 3A and 45 -- telling a team's humans where their Commander wants them.     *
 *                                                                                             *
 *	Server to client, the same direction and the same shape as `cScExplosionEvent`.  It carries  *
 *	a point, a team and a kind; the receiving client hands them to `CommanderWaypointSystem`,     *
 *	which is the only thing that decides whether the local player is on that team and therefore   *
 *	whether the marker is drawn at all.  Nothing about a unit is sent: an AI unit's order travels  *
 *	as an order and moves it, and this exists precisely for the players an order may not move.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERWAYPOINTEVENT_H
#define COMMANDERWAYPOINTEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"
#include "vector3.h"


class cCommanderWaypointEvent : public cNetEvent
{
	public:

		cCommanderWaypointEvent(void);

		//	`kind` is a CommanderWaypointKindType.
		void	Init(int team,int kind,const Vector3 & position,float lifetime_seconds,int sequence);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERWAYPOINTEVENT; }

	private:

		virtual void	Act(void) override;

		int		Team;
		int		Kind;
		Vector3	Position;
		float		LifetimeSeconds;
		int		Sequence;
};

#endif // COMMANDERWAYPOINTEVENT_H
