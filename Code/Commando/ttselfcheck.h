#ifndef	TTSELFCHECK_H
#define	TTSELFCHECK_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

/*
**	The 4.8.4 hard gate, checked from inside the binary that has to pass it.
**
**	Roadmap Section 12 asks for a short list of things to be true of the engine
**	once 4.8.4 is native: that the object types it introduced can be made, that
**	an object can be told to a single client, that the hooks it used to patch in
**	are reachable and can still refuse a purchase or a shot, that its dialogs
**	exist, and that its collision groups are the ones the scene is set up with.
**
**	None of that needs a level, a window or a network, and none of it is
**	provable by reading the source: what matters is what this program holds
**	after static initialisation.  So the checks run here and the process exits,
**	the same way the script catalog check does -- see scriptcatalogcheck.h.
*/

namespace	TTSelfCheck
{
	//
	//	The names `main` answers to:
	//
	//		objects     the building and factory types 4.8.4 introduced
	//		visibility  per-client dirty bits, and telling one client alone
	//		hooks       the event channels that replaced 4.8.4's patch sites
	//		dialogs     the dialogs 4.8.4 added, as dialog resources
	//		collision   the collision matrix CombatManager sets up
	//		modules     nothing scripts-related is loaded into the process
	//
	//	Returns a process exit code: zero when everything asked for is true.
	//
	int	Run (const char *which);
}

#endif	// TTSELFCHECK_H
