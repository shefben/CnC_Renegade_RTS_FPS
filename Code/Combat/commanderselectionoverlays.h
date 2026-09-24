/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderselectionoverlays.h          $*
 *                                                                                             *
 *	Roadmap Section 32's status rings, given the caller they were waiting for: the Commander's   *
 *	own selection.  One OVERLAY_SELECTION ring under each selected unit and under the selected   *
 *	building, moved with them every frame and removed the frame they leave the selection.        *
 *	Order destinations are not ringed here -- cCommanderOrderEvent already raises a waypoint     *
 *	marker for every order, and a second marker at the same point would be noise.                *
 *                                                                                             *
 *	Local presentation only: the selection is client state, and a dedicated server is never in  *
 *	Commander Mode, so nothing is created there.                                                 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERSELECTIONOVERLAYS_H
#define COMMANDERSELECTIONOVERLAYS_H

#include "always.h"

class CommanderSelectionOverlays
{
	public:

		static void		Update(void);
		static void		Shutdown(void);

		static int		Get_Ring_Count(void);
};

#endif // COMMANDERSELECTIONOVERLAYS_H
