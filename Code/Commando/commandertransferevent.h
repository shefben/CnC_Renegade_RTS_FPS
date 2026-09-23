/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commandertransferevent.h            $*
 *                                                                                             *
 *	Roadmap Section 25.1 -- Phase 23 multiplayer Commander governance: a direct handoff.        *
 *                                                                                             *
 *	The client->server half of `CommanderProductionService::Transfer_Commander`.  Not a vote --  *
 *	Section 25.1 lists "commander transfer" as its own separate item from "team vote", and this   *
 *	is the sitting Commander handing the job to a named teammate outright.  The server still      *
 *	refuses it unless the sender really is the team's Commander right now.                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERTRANSFEREVENT_H
#define COMMANDERTRANSFEREVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"


class cCommanderTransferEvent : public cNetEvent
{
	public:

		cCommanderTransferEvent(void);

		void	Init(int target_client_id);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERTRANSFEREVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	TargetId;
};

#endif // COMMANDERTRANSFEREVENT_H
