/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandercommandpaletterenderer.h     $*
 *                                                                                             *
 *	Roadmap Section 45 -- the drawn 3 x 3 command grid.                                         *
 *                                                                                             *
 *	Same shape and the same reasons as `CommanderSidebarRenderer`: stamped-metal bevels, amber   *
 *	readouts, geometry rebuilt in Update and replayed in Render, and a hit-test that reads back   *
 *	the rectangles Rebuild has just drawn rather than recomputing the layout.  It sits in the      *
 *	lower right, under the production sidebar, because that is the corner the original's           *
 *	command grid occupies and because the sidebar already owns the full height of the right edge.  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERCOMMANDPALETTERENDERER_H
#define COMMANDERCOMMANDPALETTERENDERER_H

#include "always.h"

#include "commandercommandpalette.h"

class CommanderCommandPaletteRenderer
{
	public:

		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		static void		Update(void);
		static void		Render(void);

		/*
		**	Which button is under a screen point.  Returns false when the palette is closed, the
		**	renderer is unavailable, or the point misses every button -- including a point inside
		**	the panel but between buttons, because a miss there must still swallow the click
		**	rather than ordering the selection to the ground behind the panel.  `out_inside_panel`
		**	is how the caller tells those two apart.
		*/
		static bool		Hit_Test(float x,float y,CommanderPaletteCommandType * out_command,
										bool * out_inside_panel);

		static int		Get_Rebuild_Count(void);
};

#endif // COMMANDERCOMMANDPALETTERENDERER_H
