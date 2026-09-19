/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderplacementevent.h           $*
 *                                                                                             *
 *	Roadmap Section 43, Phase 37 -- the client->server half of Commander building placement.    *
 *                                                                                             *
 *	Section 43's rule is the whole design: "Client sends: building definition/preset, position, *
 *	orientation, placement sequence id.  Server reruns placement checks.  Never trust client    *
 *	green/red state."  So this event carries exactly those four fields and no verdict -- there  *
 *	is deliberately no "the client thinks this is valid" bit for a server to be tempted by.      *
 *	`Act` re-runs `CommanderPlacementValidator::Validate_Server_Request`, which is the same      *
 *	function the client's ghost colours itself with, so an honest client is never refused and a  *
 *	lying one gains nothing.                                                                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPLACEMENTEVENT_H
#define COMMANDERPLACEMENTEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"
#include "vector3.h"
#include "wwstring.h"


class cCommanderPlacementEvent : public cNetEvent
{
	public:

		cCommanderPlacementEvent(void);

		//	Client-side.  Sends the placement; on a listen server it simply acts immediately,
		//	which is the same path every other cNetEvent in this codebase takes.
		void	Init(const char * descriptor_name,const Vector3 & position,float rotation_degrees,
					  int sequence_id);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERPLACEMENTEVENT; }

	private:

		virtual void	Act(void) override;

		int			SenderId;
		StringClass	DescriptorName;
		Vector3		Position;
		float			RotationDegrees;
		int			SequenceId;
};

#endif // COMMANDERPLACEMENTEVENT_H
