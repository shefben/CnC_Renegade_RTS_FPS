/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/proceduralworldpersistence.h          $*
 *                                                                                             *
 *	A save file's procedural world (D-001 priority 3): which world it was -- seed and profile  *
 *	-- and what happened to it since, as the very package WorldSyncTransport sends a late        *
 *	joiner (handshake, chunk snapshots, operations).  A save restores a world the same way a    *
 *	client joins one: regenerate the baseline from the identity, then apply the package and let  *
 *	Finish_Join verify the world checksum.                                                        *
 *                                                                                             *
 *	The ordering this needs is the engine's own: SaveGameManager::Load_Game runs every save      *
 *	subsystem's Load before CombatManager::Post_Load_Level, so Load only asks for the world      *
 *	(Request_World_Exact) and keeps the package; Post_Load_Level generates the world and then    *
 *	calls Apply_Pending.  A save with no generated world writes nothing, so an ordinary level's  *
 *	save file is unchanged.                                                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALWORLDPERSISTENCE_H
#define PROCEDURALWORLDPERSISTENCE_H

#include "always.h"
#include "saveloadsubsystem.h"
#include "wwstring.h"

class ProceduralWorldPersistenceClass : public SaveLoadSubSystemClass
{
	public:

		virtual uint32			Chunk_ID(void) const override;
		virtual bool			Contains_Data(void) const override;
		virtual bool			Save(ChunkSaveClass & csave) override;
		virtual bool			Load(ChunkLoadClass & cload) override;
		virtual const char *	Name(void) const override;

		//	After the loaded save's world has been generated: apply what the save recorded, verify
		//	it, and compact so later joiners are sent snapshots rather than the whole history.
		//	A no-op with nothing pending.  False (and logged) when the world did not verify.
		static bool		Apply_Pending(StringClass * error_out);
		static bool		Has_Pending(void);
		static void		Clear_Pending(void);
};

extern ProceduralWorldPersistenceClass	_TheProceduralWorldPersistence;

#endif // PROCEDURALWORLDPERSISTENCE_H
