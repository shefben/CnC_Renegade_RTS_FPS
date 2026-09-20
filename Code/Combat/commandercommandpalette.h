/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandercommandpalette.h             $*
 *                                                                                             *
 *	Roadmap Section 45 -- the nine verbs a Commander can apply to a selection.                  *
 *                                                                                             *
 *	The model half, in the same sense `CommanderSidebarModel` is the model half of the sidebar:  *
 *	which commands exist, which of them the current selection can actually be given, and which    *
 *	one is armed waiting for a point on the ground.  No pixels, no input, no GameObj.             *
 *                                                                                             *
 *	Three of the nine are formations rather than orders.  A formation is not a different verb --   *
 *	every one of them still ends in a move to a point -- it is a rule for where each unit in the   *
 *	group stands once it gets there, which is why it rides alongside the order rather than         *
 *	replacing it.  `cCommanderOrderEvent` carries it and the server applies it, because the        *
 *	server is the only place that knows the whole selection's final positions and must not be      *
 *	handed per-unit destinations a client computed.                                                *
 *                                                                                             *
 *	Two of the nine take effect immediately with no ground click (HOLD and the STOP that clears   *
 *	the armed command); the rest arm and wait.  `Needs_Target` is what tells the input layer        *
 *	which is which, so that rule lives here with the command list rather than being re-derived     *
 *	next to the mouse.                                                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERCOMMANDPALETTE_H
#define COMMANDERCOMMANDPALETTE_H

#include "always.h"
#include "commanderproduction.h"


/*
**	How a group stands once it reaches its destination.  NONE means every unit walks at the
**	clicked point and sorts itself out, which is what an order with no formation has always done.
*/
enum CommanderFormationType
{
	FORMATION_NONE = 0,
	FORMATION_LINE,			// abreast, across the approach
	FORMATION_WEDGE,			// arrowhead, point toward the destination
	FORMATION_SCATTER,		// spread wide, so one blast cannot take the group

	FORMATION_COUNT
};


enum CommanderPaletteCommandType
{
	PALETTE_MOVE = 0,
	PALETTE_ATTACK_MOVE,
	PALETTE_ATTACK,
	PALETTE_DEFEND,
	PALETTE_HOLD,
	PALETTE_STOP,
	PALETTE_FORM_LINE,
	PALETTE_FORM_WEDGE,
	PALETTE_FORM_SCATTER,

	PALETTE_COMMAND_COUNT		// exactly nine: the 3 x 3 grid the panel draws
};


class CommanderCommandPalette
{
	public:

		enum
		{
			COLUMNS	= 3,
			ROWS		= 3,
		};

		static void		Init(void);
		static void		Shutdown(void);

		//	Open exactly when there is something to command.  Driven from the selection each
		//	frame rather than toggled, so an emptied selection cannot leave the panel behind.
		static void		Refresh(int selection_count);
		static bool		Is_Open(void);

		static void		Arm(CommanderPaletteCommandType command);
		static void		Disarm(void);
		static bool		Is_Armed(void);
		static CommanderPaletteCommandType	Get_Armed(void);

		/*
		**	True when the command cannot be carried out until the Commander names a point or a
		**	target -- i.e. arming it should leave the pointer waiting rather than sending
		**	anything.  False for the two that are complete the moment they are clicked.
		*/
		static bool		Needs_Target(CommanderPaletteCommandType command);

		//	What the command actually sends.  Split out so the input layer never maps a palette
		//	button onto a wire value itself.
		static CommanderUnitOrderType		Get_Order(CommanderPaletteCommandType command);
		static CommanderFormationType		Get_Formation(CommanderPaletteCommandType command);

		static const char *	Get_Label(CommanderPaletteCommandType command);
		static const char *	Get_Hint(CommanderPaletteCommandType command);

		static int		Get_Issue_Count(void);
		static void		Note_Issued(void);
};

#endif // COMMANDERCOMMANDPALETTE_H
