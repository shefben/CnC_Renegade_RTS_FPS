/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/rtspersistence.h                      $*
 *                                                                                             *
 *	Roadmap Section 46, Phase 40 -- save/load and networking requirements for the RTS layer.    *
 *                                                                                             *
 *	One SaveLoadSubSystemClass for all four Commander services, not one each.  That is not a    *
 *	convenience: Section 46's rules are almost entirely about what must *not* be written, and    *
 *	several of them are cross-service ("do not serialize a second independent powered/unpowered  *
 *	truth", "rebuild ... sensor registrations from live buildings/descriptors after load").      *
 *	A single owner is the only place those rules can be enforced as code rather than as four     *
 *	separate good intentions.                                                                   *
 *                                                                                             *
 *	What is written, and why each is the authoritative rather than the derived form:             *
 *                                                                                             *
 *	  power grid    manual disable + priority override per member id.  Generation, requested     *
 *	                and allocated demand and the shed set are *not* written -- On_Post_Load      *
 *	                recalculates them, which is Section 46's own instruction and also the only   *
 *	                way the loaded state can be consistent with buildings that did not survive.  *
 *	  construction  the compact replication state Section 44 already defined, plus the model      *
 *	                bounds the layout was generated from.  No block is written; blocks are        *
 *	                regenerated from (bounds, seed), which is the same path a late joiner takes.  *
 *	  production    the queues, with their elapsed time and ticket ids.  Restored without         *
 *	                revalidation -- see CommanderProductionService::Restore_Item.                *
 *	  orders        durable Commander orders per unit id.  Not the selection set: a selection     *
 *	                is a cursor, not gameplay state, and Section 46 lists only "control groups    *
 *	                where useful" -- which this codebase does not have yet.                      *
 *                                                                                             *
 *	Deliberately absent: the placement ghost ("transient and not saved"), the strategic camera    *
 *	("do not save strategic camera position as if it were sensor coverage"), and any block        *
 *	vertex, mesh or render handle.                                                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef RTSPERSISTENCE_H
#define RTSPERSISTENCE_H

#include "always.h"
#include "saveloadsubsystem.h"


class RTSPersistenceClass : public SaveLoadSubSystemClass
{
	public:

		virtual uint32			Chunk_ID(void) const override;

		/*
		**	Late join.  Section 46: "Late joiners receive compact construction state and
		**	regenerate the current stage."  A joining client runs exactly the load path for
		**	construction -- same function, same reconstruction -- rather than a second one that
		**	could drift from it, which is why this is on the persistence class and not on a
		**	network handler.
		*/
		static int		Get_Late_Join_Construction_Count(void);

		//	Recompute everything that was deliberately not saved.  Called from On_Post_Load, and
		//	separately available so a late joiner or a descriptor reload can ask for the same
		//	reconstruction without pretending to have loaded a file.
		static void		Rebuild_Derived_State(void);

		static int		Get_Last_Load_Failure_Count(void);

		//	The public form of Contains_Data, which the SaveLoadSubSystemClass contract keeps
		//	protected.  Exposed so a check -- and a diagnostic -- can ask whether this phase
		//	would add anything to a save file at all, which for an ordinary FPS match is "no".
		static bool		Has_Data(void);

	protected:

		virtual bool			Contains_Data(void) const override;
		virtual bool			Save(ChunkSaveClass & csave) override;
		virtual bool			Load(ChunkLoadClass & cload) override;
		virtual void			On_Post_Load(void) override;
		virtual const char *	Name(void) const override;
};

extern RTSPersistenceClass	_TheRTSPersistenceSaveSystem;

#endif // RTSPERSISTENCE_H
