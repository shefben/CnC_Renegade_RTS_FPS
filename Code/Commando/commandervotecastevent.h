/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commandervotecastevent.h            $*
 *                                                                                             *
 *	Roadmap Section 25.1 -- Phase 23 multiplayer Commander governance: casting a ballot.        *
 *                                                                                             *
 *	The client->server half of `CommanderProductionService::Cast_Commander_Vote`.  Same shape   *
 *	as every other Commander client->server request in this directory.                         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERVOTECASTEVENT_H
#define COMMANDERVOTECASTEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"


class cCommanderVoteCastEvent : public cNetEvent
{
	public:

		cCommanderVoteCastEvent(void);

		void	Init(bool yes_vote);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERVOTECASTEVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	YesVote;
};

#endif // COMMANDERVOTECASTEVENT_H
