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
	//		pipelines  what each registered pipeline actually draws in
	//		particles  the shared particle pools, the batch key and the budget
	//		weather    the weather lattice, its bounded count and who pays for it
	//		tracers    the ballistic tracer pool, its ceiling, and its share of the streak pool
	//		debris     the visual debris pool, its ceiling, and its share of the sprite pool
	//		attachments  the composite attachment controller's slot bookkeeping and its
	//		           refusal of a child model that has no art yet
	//		radar        the shared tactical intelligence model, and that FPS contacts never
	//		           carry strategic radar-only/last-known information
	//		overlays     the batched status-circle pool and its shroud/per-player visibility
	//		           policy
	//		guardai      the guard/retaliate state machine's seven named scenarios, stuck/replan
	//		           budgeting, human-yield suppression, and its population bound
	//		telemetry    the RTS telemetry service's counters, running totals, snapshot, and
	//		           its enabled/disabled and record-table bounds
	//		debugrender  the world debug overlay layer toggles and the batched line pool's
	//		           bookkeeping and bound
	//
	//	Returns a process exit code: zero when everything asked for is true.
	//
	int	Run (const char *which);
}

#endif	// SHADERSELFCHECK_H
