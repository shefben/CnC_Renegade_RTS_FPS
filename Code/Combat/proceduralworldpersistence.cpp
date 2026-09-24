/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/proceduralworldpersistence.cpp        $*
 *                                                                                             *
 *	D-001 priority 3.  See proceduralworldpersistence.h.                                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralworldpersistence.h"

#include "chunkio.h"
#include "combatchunkid.h"
#include "proceduralworldmanager.h"
#include "proceduralworldprofile.h"
#include "vector.h"
#include "worldgenerationmanifest.h"
#include "worldoperationjournal.h"
#include "worldsnapshot.h"
#include "worldsynctransport.h"
#include "wwdebug.h"


ProceduralWorldPersistenceClass	_TheProceduralWorldPersistence;


enum
{
	CHUNKID_WORLD_IDENTITY		= 0x11260800,
	CHUNKID_WORLD_PACKAGE_KINDS,
	CHUNKID_WORLD_PACKAGE_SIZES,
	CHUNKID_WORLD_PACKAGE_BYTES,
};

enum
{
	VARID_SEED = 1,
	VARID_GENERATOR,
	VARID_PROFILE_PATH,
};


//	What a loaded save asked for, held until the world exists to apply it to.
static bool									_Pending = false;
static DynamicVectorClass<int>			_PendingKinds;
static DynamicVectorClass<int>			_PendingSizes;
static DynamicVectorClass<unsigned char>	_PendingBytes;


uint32 ProceduralWorldPersistenceClass::Chunk_ID(void) const		{ return CHUNKID_PROCEDURAL_WORLD_PERSISTENCE; }
const char * ProceduralWorldPersistenceClass::Name(void) const	{ return "ProceduralWorldPersistenceClass"; }


bool ProceduralWorldPersistenceClass::Contains_Data(void) const
{
	return ProceduralWorldManager::Is_World_Active () && WorldOperationJournal::Is_Open ();
}


bool ProceduralWorldPersistenceClass::Save(ChunkSaveClass & csave)
{
	DynamicVectorClass<int> kinds;
	DynamicVectorClass<int> sizes;
	DynamicVectorClass<unsigned char> bytes;
	if (!WorldSyncTransport::Build_Join_Package (&kinds,&sizes,&bytes)) {
		WWDEBUG_SAY (("ProceduralWorldPersistence: the world's package could not be built; the save "
						  "will regenerate the world without its changes\r\n"));
	}

	unsigned int seed			= ProceduralWorldManager::Get_Manifest ().Seed;
	if (seed == 0) {
		//	No manifest yet (nothing generated): the request is the only seed there is.
		seed = ProceduralWorldManager::Get_Requested_Seed ();
	}
	unsigned int generator	= (unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION;
	StringClass path (ProceduralWorldManager::Get_Profile_Path (),true);

	csave.Begin_Chunk (CHUNKID_WORLD_IDENTITY);
		WRITE_MICRO_CHUNK (csave,VARID_SEED,seed);
		WRITE_MICRO_CHUNK (csave,VARID_GENERATOR,generator);
		WRITE_MICRO_CHUNK_WWSTRING (csave,VARID_PROFILE_PATH,path);
	csave.End_Chunk ();

	if (kinds.Count () > 0) {
		csave.Begin_Chunk (CHUNKID_WORLD_PACKAGE_KINDS);
			csave.Write (&kinds [0],kinds.Count () * sizeof (int));
		csave.End_Chunk ();
		csave.Begin_Chunk (CHUNKID_WORLD_PACKAGE_SIZES);
			csave.Write (&sizes [0],sizes.Count () * sizeof (int));
		csave.End_Chunk ();
		if (bytes.Count () > 0) {
			csave.Begin_Chunk (CHUNKID_WORLD_PACKAGE_BYTES);
				csave.Write (&bytes [0],bytes.Count ());
			csave.End_Chunk ();
		}
	}
	return true;
}


static void	Read_Ints(ChunkLoadClass & cload,DynamicVectorClass<int> * out)
{
	int count = (int)(cload.Cur_Chunk_Length () / sizeof (int));
	for (int i = 0; i < count; i++) {
		int value = 0;
		cload.Read (&value,sizeof (value));
		out->Add (value);
	}
}


bool ProceduralWorldPersistenceClass::Load(ChunkLoadClass & cload)
{
	Clear_Pending ();

	unsigned int seed = 0, generator = 0;
	StringClass path ("");

	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_WORLD_IDENTITY:
				while (cload.Open_Micro_Chunk ()) {
					switch (cload.Cur_Micro_Chunk_ID ()) {
						READ_MICRO_CHUNK (cload,VARID_SEED,seed);
						READ_MICRO_CHUNK (cload,VARID_GENERATOR,generator);
						READ_MICRO_CHUNK_WWSTRING (cload,VARID_PROFILE_PATH,path);
					}
					cload.Close_Micro_Chunk ();
				}
				break;

			case CHUNKID_WORLD_PACKAGE_KINDS:	Read_Ints (cload,&_PendingKinds);	break;
			case CHUNKID_WORLD_PACKAGE_SIZES:	Read_Ints (cload,&_PendingSizes);	break;

			case CHUNKID_WORLD_PACKAGE_BYTES: {
				int count = (int)cload.Cur_Chunk_Length ();
				for (int i = 0; i < count; i++) {
					unsigned char value = 0;
					cload.Read (&value,1);
					_PendingBytes.Add (value);
				}
				break;
			}
		}
		cload.Close_Chunk ();
	}

	if (generator != (unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION) {
		//	Only one generator is kept; regenerating under another would build a different world
		//	and then refuse every recorded change against it.
		WWDEBUG_SAY (("ProceduralWorldPersistence: the save was made by world generator %u, this build is %u -- "
						  "the procedural world is not restored\r\n",generator,
						  (unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION));
		Clear_Pending ();
		return true;
	}

	StringClass error ("");
	if (!ProceduralWorldManager::Request_World_Exact (seed,path.Peek_Buffer (),&error)) {
		WWDEBUG_SAY (("ProceduralWorldPersistence: the saved world (seed %u, '%s') cannot be requested: %s\r\n",
						  seed,path.Peek_Buffer (),error.Peek_Buffer ()));
		Clear_Pending ();
		return true;
	}

	_Pending = (_PendingKinds.Count () > 0) && (_PendingKinds.Count () == _PendingSizes.Count ());
	return true;
}


bool ProceduralWorldPersistenceClass::Apply_Pending(StringClass * error_out)
{
	if (!_Pending) return true;

	bool ok = WorldSyncTransport::Apply_Local_Package (_PendingKinds,_PendingSizes,_PendingBytes,error_out);
	Clear_Pending ();
	if (!ok) return false;

	//	Snapshots of everything the save changed, and a rebased log, so a client joining later is
	//	sent the ground rather than a history this server no longer replays from.
	StringClass report ("");
	WorldSnapshotService::Compact (&report);
	return true;
}


bool ProceduralWorldPersistenceClass::Has_Pending(void)	{ return _Pending; }

void ProceduralWorldPersistenceClass::Clear_Pending(void)
{
	_Pending = false;
	_PendingKinds.Delete_All ();
	_PendingSizes.Delete_All ();
	_PendingBytes.Delete_All ();
}
