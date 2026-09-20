/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandersidebarrenderer.h            $*
 *                                                                                             *
 *	Roadmap Section 42, Phase 36 -- the drawing half of the Commander production sidebar.       *
 *                                                                                             *
 *	`CommanderSidebarModel` already decided everything this file draws: which mode is showing,  *
 *	which eight cards are on this page, what each card's status and reason are, and what the    *
 *	power bars read.  This file owns pixels and nothing else.  It re-derives no rule, and in     *
 *	particular it never decides whether a card is buyable -- it colours what the model called    *
 *	buyable, so a greyed card and a refused purchase can never disagree.                         *
 *                                                                                             *
 *	It follows the HUD's own shape exactly (`Code/Combat/hud.cpp`): a set of file-static         *
 *	`Render2DClass`/`Render2DSentenceClass` renderers, geometry rebuilt in `Update` from         *
 *	`HUDClass::Think`, and geometry replayed in `Render` from `HUDClass::Render`.  Nothing here   *
 *	is a wwui dialog: Section 42 requires the sidebar to exist "in Commander Mode only" and to    *
 *	not "replace/clutter the ordinary FPS HUD", and a dialog would take input focus away from a   *
 *	Commander who is still an embodied, vulnerable soldier (Section 3A).                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERSIDEBARRENDERER_H
#define COMMANDERSIDEBARRENDERER_H

#include "always.h"

#include "commandersidebar.h"


/*
**	What the pointer is over -- roadmap Section 42's "click a card" half.
**
**	The hit-test lives here rather than in a new input file because this is the only code in the
**	tree that knows where any of the panel's boxes are: every rectangle is derived from the live
**	screen resolution inside Rebuild, so a second implementation of the layout would silently
**	drift out of alignment with the drawn one the first time either was touched.  Rebuild simply
**	records the boxes it has just drawn, and Hit_Test reads that record back.
*/
enum CommanderSidebarHitType
{
	SIDEBAR_HIT_NONE = 0,
	SIDEBAR_HIT_PANEL,			// inside the strip, but not on a control -- swallows the click
	SIDEBAR_HIT_MODE_TAB,		// index is a CommanderSidebarModeType
	SIDEBAR_HIT_CARD,				// index is a slot 0..PAGE_SIZE-1 on the current page
	SIDEBAR_HIT_PAGE_PREV,
	SIDEBAR_HIT_PAGE_NEXT,
};


class CommanderSidebarRenderer
{
	public:

		//	`render_available` is false on a dedicated server and in a device-less self check;
		//	Init then allocates nothing and every other call is a no-op, the same contract
		//	`HUDClass::Init` already has.
		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		//	Rebuilds the panel geometry from the model.  Cheap to call every frame, but it only
		//	does real work when something the panel shows has actually changed.
		static void		Update(void);

		static void		Render(void);

		//	Forces the next Update to rebuild, whatever the model says.  Called on a resolution
		//	change, since every rectangle in the panel is derived from the screen rect.
		static void		Invalidate(void);

		static int		Get_Rebuild_Count(void);
		static int		Get_Missing_Icon_Count(void);

		/*
		**	Screen point to control.  Returns SIDEBAR_HIT_NONE whenever the panel is closed, the
		**	renderer is unavailable (dedicated server, device-less self check) or the point is
		**	outside the strip entirely, which is what lets the caller pass an ordinary world click
		**	straight through without knowing anything about the layout.
		**
		**	`out_index` is only written for the hit types that carry one.
		*/
		static CommanderSidebarHitType	Hit_Test(float x,float y,int * out_index);

		//	The card the last Hit_Test found, or nullptr.  Saves the caller re-deriving the page
		//	window from a slot index and getting a different answer than the panel drew.
		static const CommanderSidebarCard *	Get_Card_At(float x,float y);
};

#endif // COMMANDERSIDEBARRENDERER_H
