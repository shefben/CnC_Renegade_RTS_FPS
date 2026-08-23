#ifndef	SCRIPTCATALOGCHECK_H
#define	SCRIPTCATALOGCHECK_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

/*
**	The built-in script catalog, checked against what the source says it is.
**
**	Every built-in script registers itself from a file-scope object that nothing
**	references.  Source-level checking -- tools/check_script_catalog.py -- proves
**	the catalog is one name to one script, but it cannot prove that a script
**	which is written down actually arrives in the registry: a linker change, a
**	build-system change, or a change to static initialisation can drop a whole
**	file's worth of scripts with every source-level check still green.
**
**	The check runs inside this program rather than in a test executable of its
**	own, because the catalog that matters is the one this binary holds.  A
**	separate program would link the scripts a second time and prove something
**	about that link instead.  `main` takes the check before it starts a game.
*/

namespace	ScriptCatalogCheck
{
	//
	//	The names `main` answers to, and what each one looks at:
	//
	//		stock       every stock script the source declares is registered
	//		tt          every 4.8.4 script is registered, and tagged as 4.8.4
	//		duplicates  no two entries answer to one name, aliases included
	//		recreate    every entry can be rebuilt by name the way a load does
	//
	//	Returns a process exit code: zero when the catalog is what the source
	//	says it is.
	//
	int	Run (const char *which);
}

#endif	// SCRIPTCATALOGCHECK_H
