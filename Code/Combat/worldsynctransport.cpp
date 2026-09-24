/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/worldsynctransport.cpp                $*
 *                                                                                             *
 *	V2 Phases 18/19 over the wire.  See worldsynctransport.h.                                   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldsynctransport.h"

#include "combat.h"
#include "proceduralworldmanager.h"
#include "proceduralworldprofile.h"
#include "worldgenerationmanifest.h"
#include "worldoperation.h"
#include "worldoperationjournal.h"
#include "worldsnapshot.h"
#include "worldsyncservice.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"

#include <string.h>


//	The largest single message: a snapshot or repair block is bounded by one chunk's heights.
static const int	MAX_MESSAGE_BYTES	= 65536;
static const int	MAX_ASSEMBLIES		= 8;
static const int	MAX_CLIENTS			= 64;

struct Assembly
{
	Assembly(void) : InUse(false),Sender(0),MessageId(0),Kind(0),Count(0),Received(0) { }

	bool	operator == (const Assembly &) const	{ return false; }
	bool	operator != (const Assembly &) const	{ return true; }

	bool									InUse;
	int									Sender;
	unsigned int						MessageId;
	int									Kind;
	int									Count;
	int									Received;
	DynamicVectorClass<unsigned char>	Bytes;
	DynamicVectorClass<int>			PieceSizes;		// per fragment index, -1 until it arrives
};

static WorldSyncSendFunction	_Sender				= nullptr;
static unsigned int				_NextMessageId		= 1;
static Assembly					_Assemblies [MAX_ASSEMBLIES];

static int		_MessagesSent		= 0;
static int		_MessagesReceived	= 0;
static int		_MessagesDropped	= 0;
static int		_FragmentsSent		= 0;

//	Server: which clients have been sent a join package and so receive live operations.
static int		_JoinedClients [MAX_CLIENTS];
static int		_JoinedCount		= 0;

//	Client.
static WorldSyncClientPhase		_Phase					= WSYNC_CLIENT_IDLE;
static WorldSyncHandshakeClass	_ServerHandshake;
static unsigned int				_LastServerOperationId	= 0;
static int							_RepairsOutstanding		= 0;
static StringClass					_LastDetail;


static void	Put_U32(DynamicVectorClass<unsigned char> & out,unsigned int value)
{
	for (int i = 0; i < 4; i++) out.Add ((unsigned char)((value >> (8 * i)) & 0xFF));
}

static unsigned int	Get_U32(const unsigned char * bytes)
{
	return (unsigned int)bytes [0] | ((unsigned int)bytes [1] << 8) | ((unsigned int)bytes [2] << 16) |
			 ((unsigned int)bytes [3] << 24);
}

static bool	Is_Pure_Client(void)
{
	return CombatManager::I_Am_Client () && !CombatManager::I_Am_Server ();
}


/***********************************************************************************************
 * Sending and fragments                                                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int WorldSyncTransport::Send_Message(int destination,int kind,const unsigned char * bytes,int count)
{
	if ((count < 0) || (count > MAX_MESSAGE_BYTES) || ((count > 0) && (bytes == nullptr))) {
		_MessagesDropped++;
		return 0;
	}
	if (_Sender == nullptr) {
		_MessagesDropped++;
		return 0;
	}

	int fragments = (count + WorldSyncFragment::MAX_PAYLOAD - 1) / WorldSyncFragment::MAX_PAYLOAD;
	if (fragments < 1) fragments = 1;		// an empty message still has to arrive

	unsigned int message_id = _NextMessageId++;
	for (int index = 0; index < fragments; index++) {
		WorldSyncFragment fragment;
		fragment.MessageId	= message_id;
		fragment.Kind			= kind;
		fragment.Index			= index;
		fragment.Count			= fragments;

		int offset = index * WorldSyncFragment::MAX_PAYLOAD;
		int size = count - offset;
		if (size > WorldSyncFragment::MAX_PAYLOAD) size = WorldSyncFragment::MAX_PAYLOAD;
		if (size < 0) size = 0;
		fragment.Bytes = size;
		if (size > 0) ::memcpy (fragment.Payload,bytes + offset,size);

		_Sender (destination,fragment);
		_FragmentsSent++;
	}

	_MessagesSent++;
	return fragments;
}


static void	Send_Bytes(int destination,int kind,const DynamicVectorClass<unsigned char> & bytes)
{
	WorldSyncTransport::Send_Message (destination,kind,(bytes.Count () > 0) ? &bytes [0] : nullptr,bytes.Count ());
}


static void	Dispatch(int sender,int kind,const unsigned char * bytes,int count);

void WorldSyncTransport::Receive_Fragment(int sender,const WorldSyncFragment & fragment)
{
	if ((fragment.Count <= 0) || (fragment.Index < 0) || (fragment.Index >= fragment.Count) ||
		 (fragment.Bytes < 0) || (fragment.Bytes > WorldSyncFragment::MAX_PAYLOAD) ||
		 (fragment.Count * WorldSyncFragment::MAX_PAYLOAD > MAX_MESSAGE_BYTES + WorldSyncFragment::MAX_PAYLOAD)) {
		_MessagesDropped++;
		return;
	}

	if (fragment.Count == 1) {
		_MessagesReceived++;
		Dispatch (sender,fragment.Kind,fragment.Payload,fragment.Bytes);
		return;
	}

	//	Find this message's assembly, or start one in a free slot.
	Assembly * assembly = nullptr;
	for (int i = 0; i < MAX_ASSEMBLIES; i++) {
		if (_Assemblies [i].InUse && (_Assemblies [i].Sender == sender) &&
			 (_Assemblies [i].MessageId == fragment.MessageId)) {
			assembly = &_Assemblies [i];
			break;
		}
	}
	if (assembly == nullptr) {
		for (int i = 0; i < MAX_ASSEMBLIES; i++) {
			if (!_Assemblies [i].InUse) { assembly = &_Assemblies [i]; break; }
		}
		if (assembly == nullptr) {
			_MessagesDropped++;		// eight messages in flight from different senders at once
			return;
		}
		assembly->InUse		= true;
		assembly->Sender		= sender;
		assembly->MessageId	= fragment.MessageId;
		assembly->Kind			= fragment.Kind;
		assembly->Count		= fragment.Count;
		assembly->Received	= 0;
		assembly->Bytes.Delete_All ();
		assembly->PieceSizes.Delete_All ();
		for (int i = 0; i < fragment.Count * WorldSyncFragment::MAX_PAYLOAD; i++) assembly->Bytes.Add (0);
		for (int i = 0; i < fragment.Count; i++) assembly->PieceSizes.Add (-1);
	}

	if (assembly->PieceSizes [fragment.Index] >= 0) return;		// a duplicate

	::memcpy (&assembly->Bytes [fragment.Index * WorldSyncFragment::MAX_PAYLOAD],fragment.Payload,fragment.Bytes);
	assembly->PieceSizes [fragment.Index] = fragment.Bytes;
	assembly->Received++;

	if (assembly->Received < assembly->Count) return;

	//	Every piece but the last is full, so the message is the pieces laid end to end.
	int total = (assembly->Count - 1) * WorldSyncFragment::MAX_PAYLOAD + assembly->PieceSizes [assembly->Count - 1];

	int kind = assembly->Kind;
	DynamicVectorClass<unsigned char> message;
	for (int i = 0; i < total; i++) message.Add (assembly->Bytes [i]);

	assembly->InUse = false;
	assembly->Bytes.Delete_All ();
	assembly->PieceSizes.Delete_All ();

	_MessagesReceived++;
	Dispatch (sender,kind,(message.Count () > 0) ? &message [0] : nullptr,message.Count ());
}


/***********************************************************************************************
 * Identity                                                                                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int WorldSyncTransport::Build_Identity_Payload(unsigned int seed,const char * profile_path,
															  unsigned char * buffer,int max_bytes)
{
	int path_length = (profile_path != nullptr) ? (int)::strlen (profile_path) : 0;
	int needed = 4 + 4 + 4 + path_length;
	if ((buffer == nullptr) || (max_bytes < needed)) return 0;

	unsigned int values [3] = { seed,(unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION,
										  (unsigned int)path_length };
	for (int v = 0; v < 3; v++) {
		for (int i = 0; i < 4; i++) buffer [v * 4 + i] = (unsigned char)((values [v] >> (8 * i)) & 0xFF);
	}
	if (path_length > 0) ::memcpy (buffer + 12,profile_path,path_length);
	return needed;
}

bool WorldSyncTransport::Parse_Identity_Payload(const unsigned char * buffer,int bytes,unsigned int * seed_out,
																StringClass * profile_path_out,unsigned int * generator_out)
{
	if ((buffer == nullptr) || (bytes < 12)) return false;

	unsigned int seed			= Get_U32 (buffer);
	unsigned int generator	= Get_U32 (buffer + 4);
	unsigned int length		= Get_U32 (buffer + 8);
	if ((length > 1024) || ((int)(12 + length) != bytes)) return false;

	char path [1025];
	::memcpy (path,buffer + 12,length);
	path [length] = 0;

	if (seed_out != nullptr)			*seed_out = seed;
	if (generator_out != nullptr)		*generator_out = generator;
	if (profile_path_out != nullptr)	*profile_path_out = path;
	return true;
}


static void	Send_Identity(int destination)
{
	if (!ProceduralWorldManager::Is_World_Active ()) {
		WorldSyncTransport::Send_Message (destination,WSYNC_MSG_NO_WORLD,nullptr,0);
		return;
	}

	unsigned char buffer [1100];
	int used = WorldSyncTransport::Build_Identity_Payload (ProceduralWorldManager::Get_Manifest ().Seed,
																			  ProceduralWorldManager::Get_Profile_Path (),
																			  buffer,sizeof (buffer));
	if (used > 0) {
		WorldSyncTransport::Send_Message (destination,WSYNC_MSG_IDENTITY,buffer,used);
	}
}


//	The four identity numbers a handshake compares, from the world this peer generated.
static bool	Build_Local_Handshake(WorldSyncHandshakeClass * out)
{
	const WorldGenerationManifest & manifest = ProceduralWorldManager::Get_Manifest ();
	return WorldSyncService::Build_Handshake (manifest.ProfileHash,manifest.ContentManifestHash,
														  manifest.TerrainChecksum,out);
}


/***********************************************************************************************
 * Server                                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int	Find_Joined(int client_id)
{
	for (int i = 0; i < _JoinedCount; i++) {
		if (_JoinedClients [i] == client_id) return i;
	}
	return -1;
}


static void	Broadcast_Operation(const WorldOperationClass & recorded,const WorldOperationApplyResult & /* result */)
{
	if (!CombatManager::I_Am_Server ()) return;
	if (_JoinedCount == 0) return;

	unsigned char block [4096];
	int used = 0;
	if (!recorded.Write_Block (block,sizeof (block),&used)) {
		WWDEBUG_SAY (("WorldSyncTransport: operation %u would not serialise; clients will resync\r\n",
						  recorded.OperationId));
		return;
	}

	DynamicVectorClass<unsigned char> message;
	Put_U32 (message,recorded.OperationId);
	for (int i = 0; i < used; i++) message.Add (block [i]);

	for (int i = 0; i < _JoinedCount; i++) {
		Send_Bytes (_JoinedClients [i],WSYNC_MSG_LIVE_OPERATION,message);
	}
}


bool WorldSyncTransport::Build_Join_Package(DynamicVectorClass<int> * kinds_out,DynamicVectorClass<int> * sizes_out,
														  DynamicVectorClass<unsigned char> * bytes_out)
{
	if ((kinds_out == nullptr) || (sizes_out == nullptr) || (bytes_out == nullptr)) return false;
	kinds_out->Delete_All ();
	sizes_out->Delete_All ();
	bytes_out->Delete_All ();

	if (!WorldOperationJournal::Is_Open ()) return false;

	WorldSyncHandshakeClass handshake;
	if (!Build_Local_Handshake (&handshake)) return false;

	static unsigned char block [16384];
	int used = 0;

	if (!handshake.Write_Block (block,sizeof (block),&used)) return false;
	kinds_out->Add (WSYNC_MSG_HANDSHAKE);
	sizes_out->Add (used);
	for (int i = 0; i < used; i++) bytes_out->Add (block [i]);

	for (int s = 0; s < WorldSnapshotService::Get_Snapshot_Count (); s++) {
		WorldChunkSnapshotClass snapshot;
		if (!WorldSnapshotService::Get_Snapshot (s,&snapshot)) continue;
		if (!snapshot.Write_Block (block,sizeof (block),&used)) return false;
		kinds_out->Add (WSYNC_MSG_SNAPSHOT);
		sizes_out->Add (used);
		for (int i = 0; i < used; i++) bytes_out->Add (block [i]);
	}

	unsigned int last_operation_id = 0;
	for (int o = 0; o < WorldOperationJournal::Get_Operation_Count (); o++) {
		WorldOperationClass operation;
		if (!WorldOperationJournal::Get_Operation (o,&operation)) continue;
		if (!operation.Write_Block (block,sizeof (block),&used)) return false;
		kinds_out->Add (WSYNC_MSG_JOIN_OPERATION);
		sizes_out->Add (used);
		for (int i = 0; i < used; i++) bytes_out->Add (block [i]);
		last_operation_id = operation.OperationId;
	}

	//	The end marker carries the last operation id the package covers, so the client knows
	//	which live operation comes next.
	kinds_out->Add (WSYNC_MSG_JOIN_END);
	sizes_out->Add (4);
	for (int i = 0; i < 4; i++) bytes_out->Add ((unsigned char)((last_operation_id >> (8 * i)) & 0xFF));
	return true;
}


bool WorldSyncTransport::Apply_Local_Package(const DynamicVectorClass<int> & kinds,const DynamicVectorClass<int> & sizes,
															  const DynamicVectorClass<unsigned char> & bytes,StringClass * error_out)
{
	StringClass detail ("");
	WorldSyncHandshakeClass saved;
	bool begun = false;
	int offset = 0;

	for (int m = 0; m < kinds.Count (); m++) {
		int size = (m < sizes.Count ()) ? sizes [m] : 0;
		if ((size < 0) || (offset + size > bytes.Count ())) {
			if (error_out != nullptr) *error_out = "the saved world package is truncated";
			return false;
		}
		const unsigned char * block = (size > 0) ? &bytes [offset] : nullptr;
		offset += size;

		switch (kinds [m]) {

			case WSYNC_MSG_HANDSHAKE: {
				int consumed = 0;
				WorldSyncHandshakeClass local;
				if (!saved.Read_Block (block,size,&consumed) || !Build_Local_Handshake (&local) ||
					 (WorldSyncService::Begin_Join (saved,local,&detail) != WORLD_SYNC_OK)) {
					if (error_out != nullptr) *error_out = detail.Get_Length () ? detail : StringClass ("the saved handshake was refused");
					return false;
				}
				begun = true;
				break;
			}

			case WSYNC_MSG_SNAPSHOT:
				if (!begun || (WorldSyncService::Apply_Snapshot_Block (block,size,nullptr) != WORLD_SYNC_OK)) {
					if (error_out != nullptr) *error_out = "a saved chunk snapshot was refused";
					return false;
				}
				break;

			case WSYNC_MSG_JOIN_OPERATION:
				if (!begun || (WorldSyncService::Apply_Operation_Block (block,size,nullptr) != WORLD_SYNC_OK)) {
					if (error_out != nullptr) *error_out = "a saved world operation was refused";
					return false;
				}
				break;

			case WSYNC_MSG_JOIN_END:
				if (!begun || (WorldSyncService::Finish_Join (&detail) != WORLD_SYNC_OK)) {
					if (error_out != nullptr) *error_out = detail.Get_Length () ? detail : StringClass ("the restored world does not match the save");
					return false;
				}
				return true;

			default:
				break;
		}
	}

	if (error_out != nullptr) *error_out = "the saved world package has no end marker";
	return false;
}


static void	Server_Send_Join_Package(int client_id)
{
	DynamicVectorClass<int> kinds;
	DynamicVectorClass<int> sizes;
	DynamicVectorClass<unsigned char> bytes;

	if (!WorldSyncTransport::Build_Join_Package (&kinds,&sizes,&bytes)) {
		WorldSyncTransport::Send_Message (client_id,WSYNC_MSG_NO_WORLD,nullptr,0);
		return;
	}

	int offset = 0;
	for (int m = 0; m < kinds.Count (); m++) {
		WorldSyncTransport::Send_Message (client_id,kinds [m],(sizes [m] > 0) ? &bytes [offset] : nullptr,sizes [m]);
		offset += sizes [m];
	}

	//	From here the client receives every operation the server commits.  The package and the
	//	live stream share one reliable ordered channel, so nothing can overtake the end marker.
	if ((Find_Joined (client_id) < 0) && (_JoinedCount < MAX_CLIENTS)) {
		_JoinedClients [_JoinedCount++] = client_id;
	}
}


static void	Server_Send_Repairs(int client_id,const unsigned char * bytes,int count)
{
	//	Pairs of 32-bit chunk coordinates.
	static unsigned char block [16384];
	for (int offset = 0; offset + 8 <= count; offset += 8) {
		int px = (int)Get_U32 (bytes + offset);
		int py = (int)Get_U32 (bytes + offset + 4);
		int used = 0;
		if (WorldSyncService::Build_Repair_Block (px,py,block,sizeof (block),&used)) {
			WorldSyncTransport::Send_Message (client_id,WSYNC_MSG_REPAIR,block,used);
		}
	}
}


void WorldSyncTransport::On_World_Generated(void)
{
	if (!CombatManager::I_Am_Server ()) return;

	//	The journal's base snapshot is the world as generated; every later operation is measured
	//	against it and every late joiner's baseline must equal it.
	StringClass error ("");
	if (!WorldOperationJournal::Is_Open () && !WorldOperationJournal::Begin (&error)) {
		WWDEBUG_SAY (("WorldSyncTransport: the server journal did not open: %s\r\n",error.Peek_Buffer ()));
		return;
	}

	_JoinedCount = 0;

	//	Clients already connected when the level loaded asked before the world existed and were
	//	told there was none; tell them again now that there is.
	Send_Identity (WSYNC_TO_ALL_CLIENTS);
}


void WorldSyncTransport::On_Client_Left(int client_id)
{
	int index = Find_Joined (client_id);
	if (index < 0) return;
	_JoinedClients [index] = _JoinedClients [_JoinedCount - 1];
	_JoinedCount--;
}


bool	WorldSyncTransport::Is_Client_Synchronised(int client_id)	{ return Find_Joined (client_id) >= 0; }
int	WorldSyncTransport::Get_Joined_Client_Count(void)				{ return _JoinedCount; }


/***********************************************************************************************
 * Client                                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void	Client_Report(int result,const char * detail)
{
	_LastDetail = (detail != nullptr) ? detail : "";

	DynamicVectorClass<unsigned char> message;
	Put_U32 (message,(unsigned int)result);
	for (int i = 0; i < (int)_LastDetail.Get_Length (); i++) message.Add ((unsigned char)_LastDetail [i]);
	Send_Bytes (WSYNC_TO_SERVER,WSYNC_MSG_RESULT,message);
}


static void	Client_Fail(const char * detail)
{
	_Phase = WSYNC_CLIENT_FAILED;
	WWDEBUG_SAY (("WorldSyncTransport: world join failed -- %s\r\n",detail));
	Client_Report (-1,detail);
}


static void	Client_Request_Join(void)
{
	WorldSyncHandshakeClass handshake;
	if (!Build_Local_Handshake (&handshake)) {
		Client_Fail ("the regenerated world has no terrain to describe");
		return;
	}

	unsigned char block [512];
	int used = 0;
	if (!handshake.Write_Block (block,sizeof (block),&used)) {
		Client_Fail ("the client handshake would not serialise");
		return;
	}
	_Phase = WSYNC_CLIENT_JOINING;
	WorldSyncTransport::Send_Message (WSYNC_TO_SERVER,WSYNC_MSG_JOIN_REQUEST,block,used);
}


static void	Client_On_Identity(const unsigned char * bytes,int count)
{
	unsigned int seed = 0, generator = 0;
	StringClass path ("");
	if (!WorldSyncTransport::Parse_Identity_Payload (bytes,count,&seed,&path,&generator)) {
		Client_Fail ("the server's world identity could not be read");
		return;
	}
	if (generator != (unsigned int)ProceduralWorldProfile::CURRENT_GENERATOR_VERSION) {
		Client_Fail ("the server runs a different world generator version");
		return;
	}

	//	The exact seed the server's world stands on, with no retries of its own: a retry here
	//	would pick another seed and build a different world.
	StringClass error ("");
	ProceduralWorldManager::Destroy_World ();
	if (!ProceduralWorldManager::Request_World_Exact (seed,path.Peek_Buffer (),&error) ||
		 !ProceduralWorldManager::Generate_World (&error)) {
		Client_Fail (error.Peek_Buffer ());
		return;
	}

	WorldSyncTransport::Client_Join_Generated_World ();
}


void WorldSyncTransport::Client_Join_Generated_World(void)
{
	StringClass error ("");
	WorldOperationJournal::Close ();
	if (!WorldOperationJournal::Begin (&error)) {
		Client_Fail (error.Peek_Buffer ());
		return;
	}
	_LastServerOperationId = 0;
	Client_Request_Join ();
}


static void	Client_Request_Repairs(void)
{
	DynamicVectorClass<int> xs;
	DynamicVectorClass<int> ys;
	int mismatched = WorldSyncService::Find_Mismatched_Chunks (&xs,&ys);
	if (mismatched <= 0) {
		Client_Fail ("the world checksum disagrees but no chunk does");
		return;
	}

	DynamicVectorClass<unsigned char> message;
	for (int i = 0; i < xs.Count (); i++) {
		Put_U32 (message,(unsigned int)xs [i]);
		Put_U32 (message,(unsigned int)ys [i]);
	}
	_RepairsOutstanding = xs.Count ();
	_Phase = WSYNC_CLIENT_REPAIRING;
	Send_Bytes (WSYNC_TO_SERVER,WSYNC_MSG_REPAIR_REQUEST,message);
}


static void	Client_Dispatch(int kind,const unsigned char * bytes,int count)
{
	StringClass detail ("");

	switch (kind) {

		case WSYNC_MSG_NO_WORLD:
			_Phase = WSYNC_CLIENT_IDLE;
			return;

		case WSYNC_MSG_IDENTITY:
			Client_On_Identity (bytes,count);
			return;

		case WSYNC_MSG_HANDSHAKE: {
			if (_Phase != WSYNC_CLIENT_JOINING) return;
			int consumed = 0;
			if (!_ServerHandshake.Read_Block (bytes,count,&consumed)) {
				Client_Fail ("the server handshake could not be read");
				return;
			}
			WorldSyncHandshakeClass local;
			Build_Local_Handshake (&local);
			WorldSyncResultType begun = WorldSyncService::Begin_Join (_ServerHandshake,local,&detail);
			if (begun != WORLD_SYNC_OK) Client_Fail (detail.Peek_Buffer ());
			return;
		}

		case WSYNC_MSG_SNAPSHOT:
			if ((_Phase == WSYNC_CLIENT_JOINING) &&
				 (WorldSyncService::Apply_Snapshot_Block (bytes,count,nullptr) != WORLD_SYNC_OK)) {
				Client_Fail ("a world snapshot was refused");
			}
			return;

		case WSYNC_MSG_JOIN_OPERATION:
			if ((_Phase == WSYNC_CLIENT_JOINING) &&
				 (WorldSyncService::Apply_Operation_Block (bytes,count,nullptr) != WORLD_SYNC_OK)) {
				Client_Fail ("a world operation in the join package was refused");
			}
			return;

		case WSYNC_MSG_JOIN_END: {
			if (_Phase != WSYNC_CLIENT_JOINING) return;
			_LastServerOperationId = (count >= 4) ? Get_U32 (bytes) : 0;

			WorldSyncResultType finished = WorldSyncService::Finish_Join (&detail);
			if (finished == WORLD_SYNC_OK) {
				_Phase = WSYNC_CLIENT_SYNCHRONISED;
				Client_Report (WORLD_SYNC_OK,"joined");
			} else {
				Client_Request_Repairs ();
			}
			return;
		}

		case WSYNC_MSG_REPAIR: {
			if (_Phase != WSYNC_CLIENT_REPAIRING) return;
			if (WorldSyncService::Apply_Repair_Block (bytes,count,&detail) != WORLD_SYNC_OK) {
				Client_Fail (detail.Peek_Buffer ());
				return;
			}
			if (--_RepairsOutstanding > 0) return;

			unsigned int local = 0;
			WorldSnapshotService::Compute_World_Checksum (&local);
			if (local == _ServerHandshake.WorldChecksum) {
				_Phase = WSYNC_CLIENT_SYNCHRONISED;
				Client_Report (WORLD_SYNC_OK,"joined after regional repair");
			} else {
				Client_Fail ("regional repair did not reach the server's world checksum");
			}
			return;
		}

		case WSYNC_MSG_LIVE_OPERATION: {
			if (_Phase != WSYNC_CLIENT_SYNCHRONISED) return;
			if (count < 4) return;

			unsigned int operation_id = Get_U32 (bytes);
			if (operation_id <= _LastServerOperationId) return;			// already have it

			if (operation_id != _LastServerOperationId + 1) {
				//	A gap: something was lost.  Never apply out of order -- ask for the world again.
				WWDEBUG_SAY (("WorldSyncTransport: operation %u arrived after %u; rejoining\r\n",
								  operation_id,_LastServerOperationId));
				Client_Request_Join ();
				return;
			}

			WorldOperationClass operation;
			int consumed = 0;
			if (!operation.Read_Block (bytes + 4,count - 4,&consumed)) {
				Client_Request_Join ();
				return;
			}
			WorldOperationApplyResult result = WorldOperationJournal::Submit (operation);
			if (!result.Is_Applied ()) {
				WWDEBUG_SAY (("WorldSyncTransport: server operation %u was refused locally (%s); rejoining\r\n",
								  operation_id,result.Detail.Peek_Buffer ()));
				Client_Request_Join ();
				return;
			}
			_LastServerOperationId = operation_id;
			return;
		}

		default:
			return;
	}
}


static void	Server_Dispatch(int client_id,int kind,const unsigned char * bytes,int count)
{
	switch (kind) {

		case WSYNC_MSG_IDENTITY_REQUEST:
			Send_Identity (client_id);
			return;

		case WSYNC_MSG_JOIN_REQUEST:
			//	The client's own handshake rides along for the log; Begin_Join on the client is
			//	what refuses a mismatch, against the handshake it is about to receive.
			Server_Send_Join_Package (client_id);
			return;

		case WSYNC_MSG_REPAIR_REQUEST:
			Server_Send_Repairs (client_id,bytes,count);
			return;

		case WSYNC_MSG_RESULT: {
			int result = (count >= 4) ? (int)Get_U32 (bytes) : -1;
			StringClass detail ("");
			for (int i = 4; i < count; i++) {
				char text [2] = { (char)bytes [i],0 };
				detail += text;
			}
			WWDEBUG_SAY (("WorldSyncTransport: client %d world join %s -- %s\r\n",client_id,
							  (result == (int)WORLD_SYNC_OK) ? "succeeded" : "FAILED",detail.Peek_Buffer ()));
			if (result != (int)WORLD_SYNC_OK) {
				WorldSyncTransport::On_Client_Left (client_id);		// no live stream to a broken world
			}
			return;
		}

		default:
			return;
	}
}


static void	Dispatch(int sender,int kind,const unsigned char * bytes,int count)
{
	if (CombatManager::I_Am_Server ()) {
		Server_Dispatch (sender,kind,bytes,count);
	} else {
		Client_Dispatch (kind,bytes,count);
	}
}


void WorldSyncTransport::On_Level_Loaded(void)
{
	if (!Is_Pure_Client ()) {
		_Phase = WSYNC_CLIENT_IDLE;
		return;
	}
	_Phase					= WSYNC_CLIENT_WAITING_IDENTITY;
	_LastServerOperationId	= 0;
	Send_Message (WSYNC_TO_SERVER,WSYNC_MSG_IDENTITY_REQUEST,nullptr,0);
}


WorldSyncClientPhase	WorldSyncTransport::Get_Client_Phase(void)			{ return _Phase; }
const StringClass &	WorldSyncTransport::Get_Last_Result_Detail(void)	{ return _LastDetail; }

const char * WorldSyncTransport::Get_Client_Phase_Name(WorldSyncClientPhase phase)
{
	switch (phase) {
		case WSYNC_CLIENT_IDLE:					return "idle";
		case WSYNC_CLIENT_WAITING_IDENTITY:	return "waiting for the world identity";
		case WSYNC_CLIENT_JOINING:				return "joining";
		case WSYNC_CLIENT_REPAIRING:			return "repairing";
		case WSYNC_CLIENT_SYNCHRONISED:		return "synchronised";
		case WSYNC_CLIENT_FAILED:				return "failed";
		default:										return "?";
	}
}


/***********************************************************************************************
 * Lifetime and counters                                                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void WorldSyncTransport::Init(void)
{
	for (int i = 0; i < MAX_ASSEMBLIES; i++) {
		_Assemblies [i].InUse = false;
		_Assemblies [i].Bytes.Delete_All ();
		_Assemblies [i].PieceSizes.Delete_All ();
	}
	_JoinedCount				= 0;
	_Phase						= WSYNC_CLIENT_IDLE;
	_LastServerOperationId	= 0;
	_RepairsOutstanding		= 0;
	_LastDetail					= "";
	_MessagesSent = _MessagesReceived = _MessagesDropped = _FragmentsSent = 0;

	WorldOperationJournal::Set_Commit_Listener (Broadcast_Operation);
}


void WorldSyncTransport::Shutdown(void)
{
	WorldOperationJournal::Set_Commit_Listener (nullptr);
	Init ();
	WorldOperationJournal::Set_Commit_Listener (nullptr);
}


void	WorldSyncTransport::Set_Sender(WorldSyncSendFunction sender)	{ _Sender = sender; }
int	WorldSyncTransport::Get_Messages_Sent(void)							{ return _MessagesSent; }
int	WorldSyncTransport::Get_Messages_Received(void)						{ return _MessagesReceived; }
int	WorldSyncTransport::Get_Messages_Dropped(void)						{ return _MessagesDropped; }
int	WorldSyncTransport::Get_Fragments_Sent(void)							{ return _FragmentsSent; }
