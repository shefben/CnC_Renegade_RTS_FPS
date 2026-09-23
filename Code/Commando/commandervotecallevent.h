/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commandervotecallevent.h            $*
 *                                                                                             *
 *	Roadmap Section 25.1 -- Phase 23 multiplayer Commander governance: opening a vote.          *
 *                                                                                             *
 *	The client->server half of `CommanderProductionService::Call_Commander_Vote`.  Same shape   *
 *	as `cCommanderClaimEvent` on purpose -- the sender's team is looked up server-side from      *
 *	their own soldier, never trusted from the packet, and the server is the only thing that      *
 *	decides whether the vote actually opens.  `IsRemoveVote` picks ELECT vs REMOVE; `CandidateId` *
 *	is only meaningful for ELECT (REMOVE always targets whoever `Get_Team_Commander` already      *
 *	names, so a client cannot choose who gets removed).                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERVOTECALLEVENT_H
#define COMMANDERVOTECALLEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"


class cCommanderVoteCallEvent : public cNetEvent
{
	public:

		cCommanderVoteCallEvent(void);

		//	`candidate_id` is ignored server-side for a remove vote.
		void	Init(bool is_remove_vote,int candidate_id);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERVOTECALLEVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	IsRemoveVote;
		int	CandidateId;
};

#endif // COMMANDERVOTECALLEVENT_H
