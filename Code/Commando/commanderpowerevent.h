/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderpowerevent.h               $*
 *                                                                                             *
 *	Roadmap Section 42's per-building power control, sent from the Commander's power panel.      *
 *	Client -> server only.  The server refuses it unless the sender is the team's Commander      *
 *	right now and the building is a live, registered member of that team's grid; everything     *
 *	after that is TeamPowerGridService's own Set_Manual_Disabled / Set_Priority_Override /       *
 *	Clear_Priority_Override, which recalculate the team at once.  Clients learn the result     *
 *	through CommanderStateClass's mirror, never from their own request.                         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPOWEREVENT_H
#define COMMANDERPOWEREVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"

class cCommanderPowerEvent : public cNetEvent
{
	public:

		enum
		{
			ACTION_SET_ENABLED = 0,		// Value: 1 powers the building, 0 switches it off by hand
			ACTION_SET_PRIORITY,				// Value: the new allocation priority
			ACTION_CLEAR_PRIORITY,			// back to the descriptor's own priority
			ACTION_COUNT
		};

		cCommanderPowerEvent(void);

		void	Init(int building_id,int action,int value);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERPOWEREVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	BuildingId;
		int	Action;
		int	Value;
};

#endif // COMMANDERPOWEREVENT_H
