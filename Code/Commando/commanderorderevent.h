/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderorderevent.h               $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- the client->server half of Commander unit orders.           *
 *                                                                                             *
 *	Section 45: "Server validates which units the Commander can control."  The client names a    *
 *	unit, an order and a target; the server decides whether that unit is this Commander's to     *
 *	drive, through `CommanderProductionService::Can_Order_Unit` -- which refuses another team's  *
 *	unit and, more fundamentally, any unit a human is driving.                                   *
 *                                                                                             *
 *	One event carries one order for up to `MAX_UNITS` units, because a Commander ordering a      *
 *	selection is one action and splitting it into thirty packets would let a selection arrive    *
 *	half-ordered.                                                                                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERORDEREVENT_H
#define COMMANDERORDEREVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"
#include "vector3.h"


class cCommanderOrderEvent : public cNetEvent
{
	public:

		//	Bounded by construction: a packet can never carry more ids than this, so a malformed
		//	or hostile one cannot make the server read an unbounded list.
		enum { MAX_UNITS = 64 };

		cCommanderOrderEvent(void);

		//	`order` is a CommanderUnitOrderType.  `unit_ids` is the current selection.
		void	Init(int order,const Vector3 & target_point,int target_id,
					  const int * unit_ids,int unit_count);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERORDEREVENT; }

	private:

		virtual void	Act(void) override;

		int		SenderId;
		int		Order;
		Vector3	TargetPoint;
		int		TargetId;
		int		UnitCount;
		int		UnitIds [MAX_UNITS];
};

#endif // COMMANDERORDEREVENT_H
