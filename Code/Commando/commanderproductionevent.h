/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderproductionevent.h          $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- the client->server half of Commander production.            *
 *                                                                                             *
 *	Section 45: "The UI calls a server-side production service rather than emulating human       *
 *	Purchase Terminal GUI clicks."  This is that call, over the wire.  The client names a        *
 *	facility and an item; the server looks up everything else -- the team, the facility's        *
 *	descriptor, whether it is alive, built and powered, the cost, the caps -- and hands it all   *
 *	to `CommanderProductionService::Request_Production`, which is the same validation the        *
 *	client's own sidebar greyed the card out with.                                              *
 *                                                                                             *
 *	The cost is deliberately *not* a field: a client that named its own price could buy a Mammoth *
 *	for one credit.  It is read from the purchase page on the server, by definition id.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPRODUCTIONEVENT_H
#define COMMANDERPRODUCTIONEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"


class cCommanderProductionEvent : public cNetEvent
{
	public:

		cCommanderProductionEvent(void);

		//	`facility_id` is the building's network id, which is also its power-grid member id.
		void	Init(int facility_id,int definition_id);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERPRODUCTIONEVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	FacilityId;
		int	DefinitionId;
};

#endif // COMMANDERPRODUCTIONEVENT_H
