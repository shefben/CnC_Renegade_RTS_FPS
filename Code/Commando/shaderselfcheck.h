#ifndef	SHADERSELFCHECK_H
#define	SHADERSELFCHECK_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

/*
**	The shader and state management layer, checked from inside the binary that has to be right.
**
**	Roadmap Section 15 asks for one layer that every pipeline goes through, and the thing
**	that makes such a layer worth having is also the thing that is easy to get wrong: exactly
**	one program owns device state at a time, and a program that stops being current gives the
**	state back exactly once.  Every pipeline added later inherits whatever this gets wrong.
**
**	None of that needs a device.  Which program is current, how many passes it wants, who is
**	reset and when, and whether a program that declines the hardware tier is dropped rather
**	than kept, are all decisions the manager makes on its own, so they run here before
**	anything is initialised -- the same arrangement as the asset checks next door.  What the
**	checks deliberately avoid is any path that ends in WWASSERT: those are caller mistakes,
**	and proving them would mean tripping an assert in the build that has them.
*/

namespace	ShaderSelfCheck
{
	//
	//	The names `main` answers to:
	//
	//		programs   registration, tiers, passes, and who owns the device state
	//
	//	Returns a process exit code: zero when everything asked for is true.
	//
	int	Run (const char *which);
}

#endif	// SHADERSELFCHECK_H
