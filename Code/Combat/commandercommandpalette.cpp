/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandercommandpalette.cpp           $*
 *                                                                                             *
 *	Roadmap Section 45.  See commandercommandpalette.h.                                         *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandercommandpalette.h"


struct PaletteEntryStruct
{
	CommanderPaletteCommandType	Command;
	CommanderUnitOrderType			Order;
	CommanderFormationType			Formation;
	bool									NeedsTarget;
	const char *						Label;
	const char *						Hint;
};


/*
**	The grid, in reading order: three verbs, three postures, three formations.  Laid out so the
**	row a Commander wants is the row they reach for -- "where do I send them", "what do they do
**	when they get there", "how do they stand".
**
**	Every order here is one `cCommanderOrderEvent` already knows how to carry.  Nothing invents
**	a new AI primitive: PALETTE_DEFEND is ORDER_GUARD, which `CommanderGuardAI` has driven since
**	Phase 30, and PALETTE_STOP is ORDER_HOLD_POSITION, which resets the unit's action list.
*/
static const PaletteEntryStruct	_Entries [PALETTE_COMMAND_COUNT] =
{
	{ PALETTE_MOVE,			ORDER_MOVE,				FORMATION_NONE,		true,
		"MOVE",		"Walk to the point"											},
	{ PALETTE_ATTACK_MOVE,	ORDER_ATTACK_MOVE,	FORMATION_NONE,		true,
		"A-MOVE",	"Advance, engaging what is met"							},
	{ PALETTE_ATTACK,			ORDER_ATTACK_TARGET,	FORMATION_NONE,		true,
		"ATTACK",	"Attack what is under the cursor"						},

	{ PALETTE_DEFEND,			ORDER_GUARD,			FORMATION_NONE,		true,
		"DEFEND",	"Hold the point and retaliate"							},
	{ PALETTE_HOLD,			ORDER_HOLD_POSITION,	FORMATION_NONE,		false,
		"HOLD",		"Stand fast where they are"								},
	{ PALETTE_STOP,			ORDER_HOLD_POSITION,	FORMATION_NONE,		false,
		"STOP",		"Cancel what they were doing"								},

	{ PALETTE_FORM_LINE,		ORDER_MOVE,				FORMATION_LINE,		true,
		"LINE",		"Move and form abreast"										},
	{ PALETTE_FORM_WEDGE,	ORDER_MOVE,				FORMATION_WEDGE,		true,
		"WEDGE",		"Move and form an arrowhead"								},
	{ PALETTE_FORM_SCATTER,	ORDER_MOVE,				FORMATION_SCATTER,	true,
		"SCATTER",	"Move and spread out"										},
};


static bool										_IsOpen		= false;
static bool										_IsArmed		= false;
static CommanderPaletteCommandType		_Armed		= PALETTE_MOVE;
static int										_IssueCount	= 0;


static bool	Is_Valid(CommanderPaletteCommandType command)
{
	return (command >= 0) && (command < PALETTE_COMMAND_COUNT);
}


void CommanderCommandPalette::Init(void)
{
	_IsOpen		= false;
	_IsArmed		= false;
	_Armed		= PALETTE_MOVE;
	_IssueCount	= 0;
}


void CommanderCommandPalette::Shutdown(void)
{
	_IsOpen	= false;
	_IsArmed	= false;
}


void CommanderCommandPalette::Refresh(int selection_count)
{
	_IsOpen = (selection_count > 0);

	//	An armed command with nothing left to apply it to is a trap: the next click on the ground
	//	would send an order naming no units.  Disarmed with the panel rather than left set.
	if (!_IsOpen) {
		_IsArmed = false;
	}
}


bool CommanderCommandPalette::Is_Open(void)	{ return _IsOpen; }


void CommanderCommandPalette::Arm(CommanderPaletteCommandType command)
{
	if (!Is_Valid (command)) return;
	if (!_IsOpen) return;

	_Armed	= command;
	_IsArmed	= true;
}


void CommanderCommandPalette::Disarm(void)								{ _IsArmed = false; }
bool CommanderCommandPalette::Is_Armed(void)							{ return _IsArmed; }
CommanderPaletteCommandType	CommanderCommandPalette::Get_Armed(void)	{ return _Armed; }


bool CommanderCommandPalette::Needs_Target(CommanderPaletteCommandType command)
{
	if (!Is_Valid (command)) return false;
	return _Entries [command].NeedsTarget;
}


CommanderUnitOrderType	CommanderCommandPalette::Get_Order(CommanderPaletteCommandType command)
{
	if (!Is_Valid (command)) return ORDER_NONE;
	return _Entries [command].Order;
}


CommanderFormationType	CommanderCommandPalette::Get_Formation(CommanderPaletteCommandType command)
{
	if (!Is_Valid (command)) return FORMATION_NONE;
	return _Entries [command].Formation;
}


const char * CommanderCommandPalette::Get_Label(CommanderPaletteCommandType command)
{
	if (!Is_Valid (command)) return "";
	return _Entries [command].Label;
}


const char * CommanderCommandPalette::Get_Hint(CommanderPaletteCommandType command)
{
	if (!Is_Valid (command)) return "";
	return _Entries [command].Hint;
}


int	CommanderCommandPalette::Get_Issue_Count(void)	{ return _IssueCount; }
void	CommanderCommandPalette::Note_Issued(void)		{ _IssueCount++; }
