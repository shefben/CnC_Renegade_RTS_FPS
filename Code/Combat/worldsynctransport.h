/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/worldsynctransport.h                  $*
 *                                                                                             *
 *	V2 Phases 18 and 19, over a real wire (D-001 priority 3).  WorldOperationJournal and         *
 *	WorldSyncService already define every message a generated world needs -- the handshake,     *
 *	chunk snapshots, operations, repair blocks -- as byte blocks, and prove them in one process. *
 *	This is the part that moves those blocks between a server and its clients:                  *
 *                                                                                             *
 *	  - world identity: which seed and profile a client must regenerate (a client that joins a   *
 *	    procedural server otherwise has no way to know a world exists at all);                   *
 *	  - the late-join package: handshake, snapshots, the operations after them, an end marker;   *
 *	  - live operations, broadcast in journal order with the revision each one produced;        *
 *	  - regional repair: a client names the chunks that disagree, the server answers each;     *
 *	  - a result from the client, so a failed join is visible on the server.                    *
 *                                                                                             *
 *	Network-agnostic on purpose.  Every message is cut into fragments small enough for one      *
 *	packet and handed to a sender function the network layer installs (Commando's              *
 *	cWorldSyncEvent, which carries one fragment over Renegade's own reliable, ordered event     *
 *	channel -- no second socket).  Incoming fragments come back through Receive_Fragment.       *
 *	With no sender installed, a message is dropped and counted, which is what a self check and  *
 *	a single-player match both are.                                                              *
 *                                                                                             *
 *	Server-authoritative throughout: only the server's journal commits and broadcasts; a        *
 *	client submits nothing of its own, and a live operation that does not follow its revision  *
 *	is a gap that triggers a fresh join rather than being applied out of order.                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSYNCTRANSPORT_H
#define WORLDSYNCTRANSPORT_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"

enum WorldSyncMessageType
{
	WSYNC_MSG_IDENTITY_REQUEST = 1,	// client -> server: "is there a generated world?"
	WSYNC_MSG_IDENTITY,					// server -> client: seed, profile path, generator version
	WSYNC_MSG_NO_WORLD,					// server -> client: an ordinary level
	WSYNC_MSG_JOIN_REQUEST,				// client -> server: the client's own handshake block
	WSYNC_MSG_HANDSHAKE,					// server -> client
	WSYNC_MSG_SNAPSHOT,					// server -> client, one chunk snapshot block
	WSYNC_MSG_JOIN_OPERATION,			// server -> client, one operation after the snapshots
	WSYNC_MSG_JOIN_END,					// server -> client: the package is complete
	WSYNC_MSG_LIVE_OPERATION,			// server -> client: revision after, then the operation block
	WSYNC_MSG_REPAIR_REQUEST,			// client -> server: chunk coordinates that disagree
	WSYNC_MSG_REPAIR,						// server -> client, one chunk's repair block
	WSYNC_MSG_RESULT,						// client -> server: WorldSyncResultType and detail text
	WSYNC_MSG_COUNT
};

//	Special destinations for the sender: every joined client, or the server.
enum { WSYNC_TO_ALL_CLIENTS = -1, WSYNC_TO_SERVER = 0 };

struct WorldSyncFragment
{
	enum { MAX_PAYLOAD = 384 };		// comfortably inside one packet with the event's own header

	WorldSyncFragment(void) : MessageId(0),Kind(0),Index(0),Count(0),Bytes(0) { }

	bool	operator == (const WorldSyncFragment &) const	{ return false; }
	bool	operator != (const WorldSyncFragment &) const	{ return true; }

	unsigned int	MessageId;
	int				Kind;
	int				Index;
	int				Count;
	int				Bytes;
	unsigned char	Payload [MAX_PAYLOAD];
};

typedef void (*WorldSyncSendFunction)(int destination,const WorldSyncFragment & fragment);

enum WorldSyncClientPhase
{
	WSYNC_CLIENT_IDLE = 0,				// no procedural world, or not a pure client
	WSYNC_CLIENT_WAITING_IDENTITY,
	WSYNC_CLIENT_JOINING,				// identity received, world generated, package incoming
	WSYNC_CLIENT_REPAIRING,
	WSYNC_CLIENT_SYNCHRONISED,
	WSYNC_CLIENT_FAILED,
};

class WorldSyncTransport
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		static void		Set_Sender(WorldSyncSendFunction sender);

		//	Cut a message into fragments and send each.  Returns the number of fragments, or 0
		//	when nothing was sent (no sender, or an empty/oversized message).
		static int		Send_Message(int destination,int kind,const unsigned char * bytes,int count);

		//	One fragment in, from `sender` (a client id on the server, 0 on a client).  A message
		//	whose last fragment has arrived is dispatched immediately.
		static void		Receive_Fragment(int sender,const WorldSyncFragment & fragment);

		/*
		**	Server side.
		**
		**	On_World_Generated: the server opened its journal on a freshly generated world and
		**	tells every client what to regenerate.  On_Client_Left forgets a client.
		**	Operations committed to the journal are broadcast by the journal's commit listener,
		**	which Init installs -- no gameplay caller has to remember to.
		*/
		static void		On_World_Generated(void);
		static void		On_Client_Left(int client_id);
		static bool		Is_Client_Synchronised(int client_id);
		static int		Get_Joined_Client_Count(void);

		/*
		**	Client side.  On_Level_Loaded asks the server whether this level is a generated world;
		**	the rest follows from the replies.
		*/
		static void		On_Level_Loaded(void);

		/*
		**	The step after a client has regenerated the server's world: open a fresh journal on it
		**	and ask the server for the join package.  Called by the identity handler, and directly
		**	by a check whose "client" already holds the regenerated baseline.
		*/
		static void		Client_Join_Generated_World(void);
		static WorldSyncClientPhase	Get_Client_Phase(void);
		static const char *			Get_Client_Phase_Name(WorldSyncClientPhase phase);
		static const StringClass &	Get_Last_Result_Detail(void);

		//	Pure message builders, the payloads the protocol sends -- exposed for the self check.
		static int		Build_Identity_Payload(unsigned int seed,const char * profile_path,
														unsigned char * buffer,int max_bytes);
		static bool		Parse_Identity_Payload(const unsigned char * buffer,int bytes,unsigned int * seed_out,
														StringClass * profile_path_out,unsigned int * generator_out);

		//	Counters, for the console and the self check.
		static int		Get_Messages_Sent(void);
		static int		Get_Messages_Received(void);
		static int		Get_Messages_Dropped(void);
		static int		Get_Fragments_Sent(void);

		/*
		**	The server's answer to one client's join request, as the messages it would send, in
		**	order.  Separate from the handler so the package's composition is checkable without a
		**	network.  Returns false when there is no world to join.
		*/
		static bool		Build_Join_Package(DynamicVectorClass<int> * kinds_out,DynamicVectorClass<int> * sizes_out,
													 DynamicVectorClass<unsigned char> * bytes_out);

		/*
		**	The same package applied in this process, with no network: what a save load does after
		**	regenerating the saved world.  Runs the join state machine -- identity check, snapshots,
		**	operations, Finish_Join's checksum -- so a save that does not reproduce its world says so.
		*/
		static bool		Apply_Local_Package(const DynamicVectorClass<int> & kinds,const DynamicVectorClass<int> & sizes,
													  const DynamicVectorClass<unsigned char> & bytes,StringClass * error_out);
};

#endif // WORLDSYNCTRANSPORT_H
