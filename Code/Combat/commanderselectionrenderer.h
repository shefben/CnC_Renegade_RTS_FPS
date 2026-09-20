/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderselectionrenderer.h          $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- the drawn half of the Commander's selection.                *
 *                                                                                             *
 *	Two things, and nothing else: the rubber-band box while a drag is in progress, and a bracket *
 *	over each selected unit so the player can see what a drag actually caught.  Both are screen  *
 *	space, both come straight from `CommanderSelection`, and neither decides anything -- a unit   *
 *	gets a bracket because the model says it is selected, exactly as the sidebar greys a card     *
 *	because the model says it is unbuyable.                                                       *
 *                                                                                             *
 *	It follows `CommanderSidebarRenderer`'s shape (file-static `Render2DClass`, geometry rebuilt  *
 *	in Update from `HUDClass::Think`, replayed in Render from `HUDClass::Render`) and its         *
 *	`render_available` contract, so a dedicated server allocates nothing.  Unlike the sidebar it  *
 *	rebuilds every frame unconditionally: the brackets track moving units, so there is no frame    *
 *	in which last frame's geometry is still correct.                                              *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERSELECTIONRENDERER_H
#define COMMANDERSELECTIONRENDERER_H

#include "always.h"

class CommanderSelectionRenderer
{
	public:

		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		static void		Update(void);
		static void		Render(void);

		static int		Get_Bracket_Count(void);
};

#endif // COMMANDERSELECTIONRENDERER_H
