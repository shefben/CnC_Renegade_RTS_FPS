#ifndef	ASSETSELFCHECK_H
#define	ASSETSELFCHECK_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

/*
**	Asset exclusion and residency, checked from inside the binary that has to be right.
**
**	Roadmap Section 14 asks for a residency service that survives repeated map load and
**	unload without invalidating what was retained and without leaking.  Two things decide
**	whether it does: whether the exclusion list reads stock Renegade asset names the way
**	the rest of the engine names them, and whether the scope arithmetic keeps exactly the
**	assets a longer-lived scope claimed plus whatever they depend on.
**
**	Both are pure decisions over names.  Neither needs a device, a window, a level or a
**	single byte of game data, so they run here before anything is initialised and the
**	process exits with the result -- the same arrangement as the script catalog check and
**	the 4.8.4 gate.  What the checks cannot cover is the release itself, which needs a
**	loaded level and is a runtime check, not a ctest one.
*/

namespace	AssetSelfCheck
{
	//
	//	The names `main` answers to:
	//
	//		exclusion   the w3d naming conventions the keep-list matches on
	//		residency   scope ordering, dependency closure and the diagnostics
	//
	//	Returns a process exit code: zero when everything asked for is true.
	//
	int	Run (const char *which);
}

#endif	// ASSETSELFCHECK_H
