/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/worldsyncevent.h                    $*
 *                                                                                             *
 *	One WorldSyncTransport fragment on Renegade's own reliable, ordered event channel, in       *
 *	either direction (D-001 priority 3).  The transport owns the protocol; this class only       *
 *	moves bytes: a client's fragment goes to the server, a server's fragment goes to one named   *
 *	client or to every client.  `Send` is the function WorldSyncTransport::Set_Sender takes.      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSYNCEVENT_H
#define WORLDSYNCEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"
#include "worldsynctransport.h"

class cWorldSyncEvent : public cNetEvent
{
	public:

		cWorldSyncEvent(void);

		static void		Send(int destination,const WorldSyncFragment & fragment);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_WORLDSYNCEVENT; }

	private:

		virtual void	Act(void) override;

		bool					FromServer;
		int					SenderId;
		WorldSyncFragment	Fragment;
};

#endif // WORLDSYNCEVENT_H
