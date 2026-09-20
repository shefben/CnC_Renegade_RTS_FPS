/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderclaimevent.h               $*
 *                                                                                             *
 *	Roadmap Section 45 -- how a player becomes their team's Commander.                          *
 *                                                                                             *
 *	`CommanderProductionService` has always had `Set_Team_Commander`, and every authority gate    *
 *	in the Commander layer reads it -- production, placement and orders all refuse a request      *
 *	whose sender is not the recorded Commander for that team.  Nothing ever called the setter      *
 *	except the client-side state mirror, the save-game loader and the self checks, so on a live     *
 *	server `_Commanders[]` stayed zero for the whole match and every one of those gates failed      *
 *	closed.  The sidebar could be opened and nothing it offered could ever be bought.               *
 *                                                                                             *
 *	This is the missing half: the one client->server message that asks for the job, and the        *
 *	server-side rule that grants or refuses it.  First claimant per team wins and holds it until    *
 *	they release it, which is the least surprising rule and the only one that needs no lobby        *
 *	support.  A solo or skirmish match has exactly one claimant, so the player is always their      *
 *	team's Commander there without a special case -- that falls out of "first wins" rather than     *
 *	being written into it.                                                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERCLAIMEVENT_H
#define COMMANDERCLAIMEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"


class cCommanderClaimEvent : public cNetEvent
{
	public:

		cCommanderClaimEvent(void);

		//	`want_command` true asks for the job, false gives it up.
		void	Init(bool want_command);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERCLAIMEVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	WantCommand;
};

#endif // COMMANDERCLAIMEVENT_H
