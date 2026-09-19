/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderstate.h                      $*
 *                                                                                             *
 *	Roadmap Section 46, Phase 40 -- the server->client half of the Commander/RTS layer.         *
 *                                                                                             *
 *	One static network object carrying everything a client needs to *draw* the Commander layer  *
 *	and nothing it needs to decide anything.  Section 46's authority list is the spec: the       *
 *	server owns power allocation, construction completion, Commander permission and production;  *
 *	"clients render UI/ghost/procedural visuals from replicated authoritative state."             *
 *                                                                                             *
 *	Static in the same sense `PurchaseAvailabilityClass` is: one instance, a fixed network id     *
 *	both ends already have, never created or destroyed at runtime, so only its contents ever      *
 *	travel.  Dirtied on change rather than every frame -- power allocation and the construction   *
 *	list both change on events (a building dies, a placement is accepted), not continuously.      *
 *                                                                                             *
 *	What is carried:                                                                            *
 *                                                                                             *
 *	  - per team: generation, requested and allocated demand, shed/powered/manual-off counts.     *
 *	    A client cannot compute these: `RecalculateTeam` is server-gated, so a client's own grid  *
 *	    totals are meaningless.  This is a *mirror*, not a second truth -- the client's           *
 *	    TeamPowerGridService is told the answer and never derives one.                            *
 *	  - per team: the Commander's player id, so a client knows whether it is the Commander        *
 *	    without asking.                                                                          *
 *	  - the active construction list, in exactly the compact form Section 44 specified: id,       *
 *	    descriptor, transform, start time, duration, seed, model bounds.  No block, ever.         *
 *	  - the server's clock, so a client can advance those constructions against server time       *
 *	    rather than its own frame rate.                                                           *
 *                                                                                            *
 *	What is deliberately not carried: the placement ghost (transient), the strategic camera       *
 *	(Section 46 forbids it), production queue contents (the sidebar's queue badge is the          *
 *	Commander's own client, which made the request and already knows), and any block geometry.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERSTATE_H
#define COMMANDERSTATE_H

#include "always.h"
#include "networkobject.h"


class CommanderStateClass : public NetworkObjectClass
{
	public:

		CommanderStateClass(void);

		//	A static network object is never created or destroyed at runtime.
		void	Delete(void) override					{ }
		void	Set_Delete_Pending(void) override	{ }

		void	Export_Rare(BitStreamClass & packet) override;
		void	Import_Rare(BitStreamClass & packet) override;

		//	Everything starts empty when a level starts.
		static void		Reset(void);

		/*
		**	Mark the mirror stale.  Server-only, and cheap: the packet is only built when the
		**	network layer next gets round to it, so several changes in one frame cost one send.
		**	Called by anything that moves a number this object carries.
		*/
		static void		Changed(void);

		/*
		**	Called once a frame on the server.  Notices when the numbers it carries have actually
		**	moved and calls `Changed` if so -- so no caller has to remember to, and a missed call
		**	site cannot silently desync a client.  This is the belt to `Changed`'s braces.
		*/
		static void		Server_Update(void);

		/*
		**	The server's clock as this client last heard it, advanced by the client's own frame
		**	time since. What `BuildingConstructionService::Update` is given on a client, so a
		**	client's construction progress tracks server time rather than its own frame rate --
		**	Section 44's "server time/state owns completion, never client frame rate".
		**	On the server this is simply the server clock.
		*/
		static float	Get_Estimated_Server_Time(void);

		static int		Get_Import_Count(void);
		static int		Get_Export_Count(void);
};

#endif // COMMANDERSTATE_H
