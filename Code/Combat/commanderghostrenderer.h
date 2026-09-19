/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderghostrenderer.h              $*
 *                                                                                             *
 *	Roadmap Section 43, Phase 37 -- the drawing half of Commander building ghost placement.     *
 *                                                                                             *
 *	`CommanderPlacementValidator` already decides everything: where the ghost is, how it is     *
 *	rotated, and whether it is valid.  This file does exactly one thing with that -- put the    *
 *	real building model on screen, translucent, tinted by validity -- and decides nothing.      *
 *	It is a pure consumer, which is why "invalid placement ... uses the required RED            *
 *	TRANSLUCENT state" cannot drift from what the server will say: the colour is a switch on    *
 *	`Is_Ghost_Valid()`, and that is the same call the server makes.                             *
 *                                                                                             *
 *	Section 43: "no gameplay collision/network/save object".  The ghost is a bare               *
 *	`RenderObjClass` added straight to the scene -- it has no `PhysClass`, so it is in no        *
 *	collision system, no culling system that gameplay queries, and no save chunk.  Nothing in    *
 *	this file constructs a `GameObj`.                                                            *
 *                                                                                             *
 *	Translucency is applied by making the instantiated model's meshes unique and overriding      *
 *	their shader and vertex material.  "Unique" is the load-bearing word: a model instantiated   *
 *	from the asset manager shares its materials with the prototype, so tinting it in place       *
 *	without `MeshClass::Make_Unique` would turn every Power Plant in the level translucent red.  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERGHOSTRENDERER_H
#define COMMANDERGHOSTRENDERER_H

#include "always.h"


class CommanderGhostRenderer
{
	public:

		//	`render_available` is false on a dedicated server and in a device-less self check;
		//	Init then allocates nothing and Update is a no-op, the same contract HUDClass::Init
		//	already has.  A server must never instantiate a model just to not draw it.
		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		/*
		**	Called once a frame on a client that has a scene.  Creates, moves, tints and destroys
		**	the ghost to match `CommanderPlacementValidator`'s state, and does nothing at all when
		**	no ghost is active -- so a dedicated server, which never calls this, is unaffected.
		*/
		static void		Update(void);

		//	Drops the model immediately.  Called by Shutdown and by a level change; also safe to
		//	call when there is no ghost.
		static void		Release(void);

		static bool		Is_Model_Loaded(void);
		static int		Get_Missing_Model_Count(void);
};

#endif // COMMANDERGHOSTRENDERER_H
