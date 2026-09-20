/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderwaypointrenderer.h           $*
 *                                                                                             *
 *	Roadmap Sections 3A and 45 -- the translucent world marker a Commander's waypoint leaves.   *
 *                                                                                             *
 *	Drawn for every player on the waypoint's team, not only the Commander: the whole point of a  *
 *	waypoint is that a human who cannot be ordered can still be told, and a marker only the       *
 *	person who placed it can see tells nobody anything.  It is a world object rather than a HUD   *
 *	overlay for the same reason -- it has to sit on the ground the player is about to walk over,  *
 *	occlude correctly behind terrain, and be there when they turn around.                         *
 *                                                                                             *
 *	No art is required and none is named.  The marker is procedural: a ring of prism segments on  *
 *	the ground and a short column above it, submitted through the same translucent line path      *
 *	`DebugRenderSystem` already uses for world-space geometry.  That choice is deliberate and not  *
 *	a placeholder -- a `.w3d` would have to be authored, shipped and kept in sync with a colour    *
 *	the code picks per waypoint kind, and the shape this needs is four rings and a stick.         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERWAYPOINTRENDERER_H
#define COMMANDERWAYPOINTRENDERER_H

#include "always.h"

class CommanderWaypointRenderer
{
	public:

		//	`render_available` false (dedicated server, device-less self check) makes every call
		//	a no-op and allocates nothing, the same contract the other Commander renderers have.
		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		//	Rebuilds the marker geometry from CommanderWaypointSystem.  Once a client frame.
		static void		Update(float seconds);

		static int		Get_Drawn_Marker_Count(void);
};

#endif // COMMANDERWAYPOINTRENDERER_H
