/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderwaypoint.h                   $*
 *                                                                                             *
 *	Roadmap Sections 3A and 45 -- the Commander's orders, made visible to the humans on the      *
 *	team who are not obliged to follow them.                                                     *
 *                                                                                             *
 *	Section 3A.4 is a non-negotiable: a human player's movement and right of way take priority   *
 *	over Commander AI work, and `cCommanderOrderEvent::Act` enforces it by refusing to drive any  *
 *	unit with a controlling player behind it.  That rule is what makes this class necessary       *
 *	rather than optional.  A Commander who selects a mixed group and issues a move has given two   *
 *	different kinds of instruction at once: to the AI it is a command, executed immediately and    *
 *	overriding whatever the unit was doing; to the humans it can only ever be a request, because   *
 *	the alternative is seizing control of a person's soldier.                                      *
 *                                                                                             *
 *	So the request is drawn instead of enforced.  A waypoint is a point, a team, a kind and a     *
 *	lifetime; the renderer puts a translucent marker on the ground there and the radar puts a      *
 *	blip on it, for every player on that team.  Nothing here touches a GameObj, and nothing here   *
 *	can move anybody -- which is the property that makes "a human is never driven by the           *
 *	Commander" checkable by reading this file rather than by testing behaviour.                    *
 *                                                                                             *
 *	Bounded by construction: a fixed number of slots per team, oldest replaced first.  A           *
 *	Commander spamming move orders cannot grow this, and a client that never hears the expiry      *
 *	still drops the marker on its own timer.                                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERWAYPOINT_H
#define COMMANDERWAYPOINT_H

#include "always.h"
#include "vector3.h"


/*
**	What the marker means.  Drawn differently per kind so a player can tell "go here" from
**	"attack here" without being told in text.
*/
enum CommanderWaypointKindType
{
	WAYPOINT_MOVE = 0,
	WAYPOINT_ATTACK,
	WAYPOINT_DEFEND,
	WAYPOINT_RALLY,

	WAYPOINT_KIND_COUNT
};


struct CommanderWaypointStruct
{
	CommanderWaypointStruct(void) : Team(-1),Kind(WAYPOINT_MOVE),Position(0.0f,0.0f,0.0f),
											  SecondsLeft(0.0f),Sequence(0) { }

	int								Team;
	CommanderWaypointKindType	Kind;
	Vector3							Position;
	float								SecondsLeft;
	int								Sequence;
};


class CommanderWaypointSystem
{
	public:

		enum
		{
			//	Per team, not in total.  Four is one per squad a Commander can plausibly be
			//	running at once; beyond that the markers stop being readable anyway.
			MAX_PER_TEAM	= 4,
			MAX_TEAMS		= 4,
		};

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Records a waypoint for a team.  Called on the server when an order is issued and on
		**	every client when the matching event arrives, with the same arguments, so both sides
		**	hold the same list without the client ever deriving one.
		*/
		static void		Set_Waypoint(int team,CommanderWaypointKindType kind,const Vector3 & position,
											  float lifetime_seconds,int sequence);

		static void		Clear_Team(int team);

		//	Ages every waypoint and drops the expired ones.  Once a frame, both sides.
		static void		Update(float seconds);

		static int		Get_Count(void);
		static const CommanderWaypointStruct &	Get_Waypoint(int index);

		//	How many of them a given team can see.  A waypoint is only ever visible to its own
		//	team: it is an order, and an order the enemy can read is intelligence handed over.
		static int		Get_Team_Count(int team);

		static const char *	Get_Kind_Name(CommanderWaypointKindType kind);

		static int		Get_Set_Count(void);
		static int		Get_Expired_Count(void);
};

#endif // COMMANDERWAYPOINT_H
