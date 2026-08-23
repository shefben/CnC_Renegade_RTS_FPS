/******************************************************************************
*
* FILE
*     ttconsole.h
*
* DESCRIPTION
*     The server-operator console commands 4.8.4 added, natively.
*
*     They are registered into the one console function list rather than kept
*     in a second one, the same way SystemSettings adds its own.  See
*     docs/tt484/TTConsoleCommands.md for the command-by-command account,
*     including the ten that are not here and why.
*
******************************************************************************/

#ifndef	__TTCONSOLE_H
#define	__TTCONSOLE_H

#include "always.h"
#include "vector.h"

class	ConsoleFunctionClass;

namespace TTConsole
{
	void	Add_Console_Functions (DynamicVectorClass<ConsoleFunctionClass *> &list);
}

#endif	//__TTCONSOLE_H
