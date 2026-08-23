//
//	The 4.8.4 hard gate, checked from inside the binary that has to pass it.
//	See ttselfcheck.h for why the checks live here.
//

#include "ttselfcheck.h"

#include "combat.h"
#include "combatchunkid.h"
#include "definition.h"
#include "definitionfactory.h"
#include "definitionfactorymgr.h"
#include "gameeventbus.h"
#include "networkobject.h"
#include "phys.h"
#include "pscene.h"
#include "renegadedialog.h"
#include "saveload.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

int	_Failures = 0;

void	Fail (const char *format, ...)
{
	va_list args;
	va_start (args, format);
	::fprintf (stderr, "FAIL: ");
	::vfprintf (stderr, format, args);
	::fprintf (stderr, "\n");
	va_end (args);

	_Failures ++;
	return ;
}

void	Check (bool condition, const char *format, ...)
{
	if (condition) {
		return ;
	}

	va_list args;
	va_start (args, format);
	::fprintf (stderr, "FAIL: ");
	::vfprintf (stderr, format, args);
	::fprintf (stderr, "\n");
	va_end (args);

	_Failures ++;
	return ;
}


/***********************************************************************************************
**	objects -- the building and factory types 4.8.4 introduced
***********************************************************************************************/

struct BuildingTypeStruct
{
	const char *	Name;			// the name it is offered under in the editor
	uint32			DefClassID;	// its definition factory
	uint32			ChunkID;		// its persist factory, which is how a save finds it
};

//
//	Two of these are new classes this project added because 4.8.4 had them and
//	nothing here did.  The rest were already stock and are listed because 4.8.4
//	changed them: whatever else moved, a level that places one must still get
//	one.
//
const BuildingTypeStruct	_BuildingTypes[] =
{
	{ "Construction Yard",	CLASSID_GAME_OBJECT_DEF_CONSTRUCTION_YARD,
			CHUNKID_GAME_OBJECT_CONSTRUCTION_YARD },
	{ "Superweapon",			CLASSID_GAME_OBJECT_DEF_SUPERWEAPON,
			CHUNKID_GAME_OBJECT_SUPERWEAPON },
	{ "Soldier Factory",		CLASSID_GAME_OBJECT_DEF_SOLDIER_FACTORY,
			CHUNKID_GAME_OBJECT_SOLDIER_FACTORY },
	{ "Vehicle Factory",		CLASSID_GAME_OBJECT_DEF_VEHICLE_FACTORY,
			CHUNKID_GAME_OBJECT_VEHICLE_FACTORY },
	{ "Airstrip",				CLASSID_GAME_OBJECT_DEF_AIRSTRIP,
			CHUNKID_GAME_OBJECT_AIRSTRIP },
	{ "Refinery",				CLASSID_GAME_OBJECT_DEF_REFINERY,
			CHUNKID_GAME_OBJECT_REFINERY },
	{ "Powerplant",			CLASSID_GAME_OBJECT_DEF_POWERPLANT,
			CHUNKID_GAME_OBJECT_POWERPLANT },
	{ "Repair Bay",			CLASSID_GAME_OBJECT_DEF_REPAIR_BAY,
			CHUNKID_GAME_OBJECT_REPAIR_BAY },
	{ "Com Center",			CLASSID_GAME_OBJECT_DEF_COMCENTER,
			CHUNKID_GAME_OBJECT_COMCENTER },
};

int	Run_Objects (void)
{
	const int count = (int)(sizeof (_BuildingTypes) / sizeof (_BuildingTypes[0]));

	for (int index = 0; index < count; index ++) {
		const BuildingTypeStruct &type = _BuildingTypes[index];

		//
		//	The editor offers a type by name and the level file names it by
		//	class id, so both have to reach the same factory.
		//
		DefinitionFactoryClass *by_id		= DefinitionFactoryMgrClass::Find_Factory (type.DefClassID);
		DefinitionFactoryClass *by_name	= DefinitionFactoryMgrClass::Find_Factory (type.Name);

		if (by_id == nullptr) {
			Fail ("'%s' has no definition factory registered for class id %u",
					type.Name, (unsigned int)type.DefClassID);
			continue;
		}

		Check (by_name == by_id,
				"'%s' does not reach its own factory by name", type.Name);

		//
		//	Making one is the point.  A definition that comes back saying it is
		//	something else means two types share an id, which a level file has
		//	no way to tell apart.
		//
		DefinitionClass *definition = by_id->Create ();

		if (definition == nullptr) {
			Fail ("'%s' has a factory that creates nothing", type.Name);
			continue;
		}

		Check (definition->Get_Class_ID () == type.DefClassID,
				"a '%s' says its class id is %u, not %u", type.Name,
				(unsigned int)definition->Get_Class_ID (), (unsigned int)type.DefClassID);

		delete definition;

		//
		//	And a saved game has to be able to find it again.  A persist factory
		//	that was never force-linked is the classic way a new type works
		//	until the moment somebody saves.
		//
		Check (SaveLoadSystemClass::Find_Persist_Factory (type.ChunkID) != nullptr,
				"'%s' has no persist factory for chunk id %u -- a save naming it"
				" would not load", type.Name, (unsigned int)type.ChunkID);
	}

	::printf ("objects: %d building and factory types create and persist\n", count);
	return _Failures;
}


/***********************************************************************************************
**	visibility -- per-client dirty bits, and telling one client alone
***********************************************************************************************/

//
//	NetworkObjectClass::Delete is pure virtual because a real one belongs to
//	something.  This one belongs to nobody, which is what the check wants: the
//	dirty bit bookkeeping and nothing else.
//
class	TestNetworkObjectClass : public NetworkObjectClass
{
public:
	void	Delete (void) override	{ delete this; }
};

int	Run_Visibility (void)
{
	TestNetworkObjectClass object;

	object.Clear_Object_Dirty_Bits ();

	for (int client = 1; client <= 4; client ++) {
		Check (!object.Is_Client_Dirty (client),
				"a freshly cleared object is dirty for client %d", client);
	}

	//
	//	One client, not all of them.  This is what per-client visibility is
	//	built on: an object that is some clients' business and not others'.
	//
	object.Set_Object_Dirty_Bit (2, NetworkObjectClass::BIT_RARE, true);

	Check (object.Get_Object_Dirty_Bit (2, NetworkObjectClass::BIT_RARE),
			"a bit set for client 2 did not stay set");
	Check (!object.Get_Object_Dirty_Bit (3, NetworkObjectClass::BIT_RARE),
			"a bit set for client 2 leaked to client 3");
	Check (object.Is_Client_Dirty (2), "client 2 is not dirty after setting a bit");
	Check (!object.Is_Client_Dirty (3), "client 3 went dirty on client 2's account");

	//
	//	BIT_RARE includes BIT_OCCASIONAL includes BIT_FREQUENT, so setting the
	//	coarse one sets the finer ones with it.  A receiver that is told the
	//	rare state has to be told the frequent state too or it reads a half
	//	updated object.
	//
	Check (object.Get_Object_Dirty_Bit (2, NetworkObjectClass::BIT_FREQUENT),
			"BIT_RARE did not carry BIT_FREQUENT with it");

	object.Set_Object_Dirty_Bit (2, NetworkObjectClass::BIT_RARE, false);
	Check (!object.Is_Client_Dirty (2), "clearing client 2's bit left it dirty");

	//
	//	The everyone form, which only the server may use: client status is the
	//	server's bookkeeping about who it still owes an update to, and a client
	//	has no business writing it.
	//
	const bool was_server = NetworkObjectClass::Is_Server ();

	NetworkObjectClass::Set_Is_Server (false);
	object.Clear_Object_Dirty_Bits ();
	object.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_CREATION, true);
	Check (!object.Is_Client_Dirty (1),
			"a client was allowed to dirty an object for everybody");

	NetworkObjectClass::Set_Is_Server (true);
	object.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_CREATION, true);
	Check (object.Is_Client_Dirty (1) && object.Is_Client_Dirty (5),
			"the all-clients form did not reach every client");

	//
	//	A client that joins late is told again.  The sentinel slot -- the one
	//	nobody ever connects on -- carries everything the object has raised
	//	since it was made, and copying it is how a fresh client is caught up.
	//	An object that is only some clients' business overrides this and tells
	//	the others nothing, which is why it is virtual at all.
	//
	Check (object.Get_Object_Dirty_Bits (NetworkObjectClass::MAX_CLIENT_COUNT - 1) != 0,
			"the sentinel slot was not written, so a rejoining client would be"
			" told nothing");

	object.Set_Object_Dirty_Bits (7, 0);
	object.Set_Object_Dirty_Bits (8, 0);

	object.setDirtyBitsForClient (7);
	Check (object.Is_Client_Dirty (7),
			"a rejoining client was not told about the object again");
	Check (!object.Is_Client_Dirty (8),
			"telling client 7 again told client 8 as well");

	object.Clear_Object_Dirty_Bits ();
	Check (!object.Is_Client_Dirty (1) && !object.Is_Client_Dirty (5),
			"clearing every client's bits left some set");

	NetworkObjectClass::Set_Is_Server (was_server);

	::printf ("visibility: per-client dirty bits are per client\n");
	return _Failures;
}


/***********************************************************************************************
**	hooks -- the event channels that replaced 4.8.4's patch sites
***********************************************************************************************/

int	_Calls		= 0;
int	_LastInt	= 0;

void	Count_Purchase (PurchaseEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = (int)event.Cost;
	event.Status = PURCHASE_STATUS_OUT_OF_STOCK;
}

void	Count_Refill (RefillEventClass &event, void * /*data*/)
{
	_Calls ++;
	event.Allow = false;
}

void	Count_Damage (DamageEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = (int)event.Amount;
	event.Allow = false;
}

void	Count_Kill (KillEventClass & /*event*/, void * /*data*/)
{
	_Calls ++;
}

void	Count_Chat (ChatEventClass &event, void * /*data*/)
{
	_Calls ++;
	event.Allow = false;
}

void	Count_Join (PlayerJoinEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = event.PlayerID;
}

void	Count_Leave (PlayerLeaveEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = event.PlayerID;
}

void	Count_Dialog (DialogEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = event.DialogID;
}

void	Count_Visibility (NetworkVisibilityEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = event.ClientID;
}

void	Count_Dirty (NetworkDirtyEventClass &event, void * /*data*/)
{
	_Calls ++;
	_LastInt = event.ClientID;
}


int	Run_Hooks (void)
{
	GameEventBus::Init ();

	//
	//	Purchase.  4.8.4 patched the purchase path to let a plugin refuse a
	//	buy; the channel has to carry that decision back to the caller, not
	//	merely be told about it.
	//
	{
		_Calls = 0;
		int token = GameEventBus::Purchase.Register (Count_Purchase);

		Check (GameEventBus::Purchase.Has_Subscribers (),
				"the purchase channel does not know it has a subscriber");

		PurchaseStatusEnum status = GameEventBus::Raise_Purchase (
				PURCHASE_CATEGORY_VEHICLE, nullptr, nullptr, 900, 0);

		Check (_Calls == 1, "the purchase handler ran %d times, not once", _Calls);
		Check (_LastInt == 900, "the purchase handler was told a cost of %d, not 900", _LastInt);
		Check (status == PURCHASE_STATUS_OUT_OF_STOCK,
				"a handler refused a purchase and the caller was told to allow it");

		GameEventBus::Purchase.Unregister (token);

		_Calls = 0;
		GameEventBus::Raise_Purchase (PURCHASE_CATEGORY_VEHICLE, nullptr, nullptr, 900, 0);
		Check (_Calls == 0, "an unregistered purchase handler still ran");
		Check (!GameEventBus::Purchase.Has_Subscribers (),
				"the purchase channel still claims a subscriber after unregistering");
	}

	//
	//	Refill, which is the same shape and the same decision.
	//
	{
		_Calls = 0;
		int token = GameEventBus::Refill.Register (Count_Refill);

		bool allowed = GameEventBus::Raise_Refill (nullptr);

		Check (_Calls == 1, "the refill handler ran %d times, not once", _Calls);
		Check (!allowed, "a handler refused a refill and the caller was told to allow it");

		GameEventBus::Refill.Unregister (token);
	}

	//
	//	Damage and kill.  Damage is raised before armor and warhead scaling, so
	//	what a handler sees is what the weapon asked for; refusing it is how
	//	4.8.4's damage hook made a target immune.
	//
	{
		_Calls = 0;
		int damage_token	= GameEventBus::Damage.Register (Count_Damage);
		int kill_token		= GameEventBus::Kill.Register (Count_Kill);

		bool allowed = GameEventBus::Raise_Damage (nullptr, nullptr, 40.0f, 0, nullptr, nullptr);

		Check (_Calls == 1, "the damage handler ran %d times, not once", _Calls);
		Check (_LastInt == 40, "the damage handler was told %d, not 40", _LastInt);
		Check (!allowed, "a handler refused damage and the caller was told to apply it");

		_Calls = 0;
		GameEventBus::Raise_Kill (nullptr, nullptr);
		Check (_Calls == 1, "the kill handler ran %d times, not once", _Calls);

		GameEventBus::Damage.Unregister (damage_token);
		GameEventBus::Kill.Unregister (kill_token);
	}

	//
	//	Chat and the player list.  Refusing a chat message is how a server
	//	swallows a command it has just acted on rather than repeating it to
	//	everybody.
	//
	{
		_Calls = 0;
		int chat_token		= GameEventBus::Chat.Register (Count_Chat);
		int join_token		= GameEventBus::PlayerJoin.Register (Count_Join);
		int leave_token	= GameEventBus::PlayerLeave.Register (Count_Leave);

		bool allowed = GameEventBus::Raise_Chat (3, TEXT_MESSAGE_PUBLIC, L"!hello", -1);

		Check (_Calls == 1, "the chat handler ran %d times, not once", _Calls);
		Check (!allowed, "a handler swallowed a chat message and it was sent anyway");

		_Calls = 0;
		GameEventBus::Raise_Player_Join (11, "somebody");
		Check (_Calls == 1 && _LastInt == 11, "the join handler did not see player 11");

		_Calls = 0;
		GameEventBus::Raise_Player_Leave (11);
		Check (_Calls == 1 && _LastInt == 11, "the leave handler did not see player 11");

		GameEventBus::Chat.Unregister (chat_token);
		GameEventBus::PlayerJoin.Unregister (join_token);
		GameEventBus::PlayerLeave.Unregister (leave_token);
	}

	//
	//	Dialogs, and the two network channels the per-client work is built on.
	//
	{
		_Calls = 0;
		int dialog_token		= GameEventBus::Dialog.Register (Count_Dialog);
		int visibility_token	= GameEventBus::NetworkVisibility.Register (Count_Visibility);
		int dirty_token		= GameEventBus::NetworkDirty.Register (Count_Dirty);

		GameEventBus::Raise_Dialog (2, (int)RenegadeDialogID::IDD_OBJECTIVES_TEXT, 0,
				DIALOG_MESSAGE_CONTROL_MOUSE_CLICK);
		Check (_Calls == 1
				&& _LastInt == (int)RenegadeDialogID::IDD_OBJECTIVES_TEXT,
				"the dialog handler did not see the objectives dialog");

		_Calls = 0;
		GameEventBus::Raise_Network_Visibility (nullptr, 5, false);
		Check (_Calls == 1 && _LastInt == 5,
				"the visibility handler did not see client 5");

		_Calls = 0;
		GameEventBus::Raise_Network_Dirty (nullptr, 6, NetworkObjectClass::BIT_RARE);
		Check (_Calls == 1 && _LastInt == 6, "the dirty handler did not see client 6");

		GameEventBus::Dialog.Unregister (dialog_token);
		GameEventBus::NetworkVisibility.Unregister (visibility_token);
		GameEventBus::NetworkDirty.Unregister (dirty_token);
	}

	//
	//	Nothing left listening.  A channel that kept a dead subscriber would
	//	call into freed memory the next time a game raised the event.
	//
	Check (!GameEventBus::Purchase.Has_Subscribers ()
			&& !GameEventBus::Refill.Has_Subscribers ()
			&& !GameEventBus::Damage.Has_Subscribers ()
			&& !GameEventBus::Kill.Has_Subscribers ()
			&& !GameEventBus::Chat.Has_Subscribers ()
			&& !GameEventBus::Dialog.Has_Subscribers ()
			&& !GameEventBus::NetworkVisibility.Has_Subscribers ()
			&& !GameEventBus::NetworkDirty.Has_Subscribers (),
			"a channel still has a subscriber after every token was returned");

	::printf ("hooks: purchase, refill, damage, kill, chat, player, dialog and"
			" network channels dispatch and carry their decision back\n");
	return _Failures;
}


/***********************************************************************************************
**	dialogs -- the dialogs 4.8.4 added, as dialog resources
***********************************************************************************************/

struct DialogStruct
{
	RenegadeDialogID	ID;
	const char *		Name;
};

const DialogStruct	_Dialogs[] =
{
	{ RenegadeDialogID::IDD_GAME_HINT,			"the hint box a level pops up" },
	{ RenegadeDialogID::IDD_OBJECTIVES_TEXT,	"the objectives text page" },
	{ RenegadeDialogID::IDD_CNC_PURCHASE_MAIN_SCREEN,	"the purchase terminal" },
	{ RenegadeDialogID::IDD_CNC_PURCHASE_ACCESS_DENIED,	"the refused purchase box" },
};

int	Run_Dialogs (void)
{
	const int count = (int)(sizeof (_Dialogs) / sizeof (_Dialogs[0]));

	for (int index = 0; index < count; index ++) {
		const DialogStruct &entry = _Dialogs[index];

		const DialogResource *resource = GetRenegadeDialog (entry.ID);

		if (resource == nullptr) {
			Fail ("%s (%d) is not in the dialog table", entry.Name, (int)entry.ID);
			continue;
		}

		Check (resource->id == (int)entry.ID,
				"%s answers to id %d, not %d", entry.Name, resource->id, (int)entry.ID);
		Check ((resource->controls != nullptr) && (resource->count_controls > 0),
				"%s has no controls, so opening it would show an empty box", entry.Name);
		Check ((resource->cx > 0) && (resource->cy > 0),
				"%s has no size", entry.Name);
	}

	::printf ("dialogs: %d dialogs are present with controls and a size\n", count);
	return _Failures;
}


/***********************************************************************************************
**	collision -- the collision matrix CombatManager sets up
***********************************************************************************************/

struct CollisionPairStruct
{
	int				Group0;
	int				Group1;
	bool				Collides;
	const char *	Why;
};

const CollisionPairStruct	_CollisionPairs[] =
{
	//
	//	The 4.8.4 ladder fix.  A ghosted soldier -- one inside a ladder or
	//	elevator coordination zone, or squeezing past a team mate -- passes
	//	through the default group as well as through other soldiers.  A vehicle
	//	parked at the foot of a ladder used to pin him there.
	//
	{ SOLDIER_GHOST_COLLISION_GROUP,	DEFAULT_COLLISION_GROUP,	false,
			"a ghosted soldier must pass through vehicles and placed objects" },
	{ SOLDIER_GHOST_COLLISION_GROUP,	SOLDIER_COLLISION_GROUP,	false,
			"a ghosted soldier must pass through other soldiers" },
	{ SOLDIER_GHOST_COLLISION_GROUP,	SOLDIER_GHOST_COLLISION_GROUP,	false,
			"two ghosted soldiers must pass through each other" },

	//
	//	And what ghosting must not cost him: he still stands on the world and
	//	is still shot at.
	//
	{ SOLDIER_GHOST_COLLISION_GROUP,	TERRAIN_COLLISION_GROUP,	true,
			"a ghosted soldier must still stand on the terrain" },
	{ SOLDIER_GHOST_COLLISION_GROUP,	BULLET_COLLISION_GROUP,	true,
			"a ghosted soldier must still be shot" },
	{ SOLDIER_GHOST_COLLISION_GROUP,	C4_COLLISION_GROUP,	true,
			"C4 must still stick to a ghosted soldier" },

	//
	//	An ordinary soldier is stopped by the things a ghosted one is not.
	//
	{ SOLDIER_COLLISION_GROUP,	DEFAULT_COLLISION_GROUP,	true,
			"an ordinary soldier must be stopped by a vehicle" },
	{ SOLDIER_COLLISION_GROUP,	SOLDIER_COLLISION_GROUP,	true,
			"two ordinary soldiers must not walk through each other" },

	//
	//	The building ghost pair, which is what lets a placed building be walked
	//	through while it is still going up.
	//
	{ PLAYER_BUILDING_COLLISION_GROUP,	PLAYER_BUILDING_GHOST_COLLISION_GROUP,	false,
			"a building and its ghost must not collide" },
};

//
//	The matrix, written somewhere it can be read back.  A physics scene needs a
//	graphics device behind it, which a check running before anything is opened
//	does not have, so CombatManager describes the matrix to this instead -- the
//	same description the game scene gets, from the same function.
//
class	TableCollisionGroupSink : public CollisionGroupSinkClass
{
public:
	TableCollisionGroupSink (void)	{ ::memset (Allowed, 0, sizeof (Allowed)); }

	void	Enable_All (int group) override
	{
		for (int other = 0; other < COLLISION_GROUP_COUNT; other ++) { Set (group, other, true); }
	}

	void	Disable_All (int group) override
	{
		for (int other = 0; other < COLLISION_GROUP_COUNT; other ++) { Set (group, other, false); }
	}

	void	Enable (int group0, int group1) override	{ Set (group0, group1, true); }
	void	Disable (int group0, int group1) override	{ Set (group0, group1, false); }

	bool	Collides (int group0, int group1) const
	{
		if (!In_Range (group0) || !In_Range (group1)) { return false; }
		return Allowed[group0][group1];
	}

private:
	static bool	In_Range (int group)
	{
		return (group >= 0) && (group < COLLISION_GROUP_COUNT);
	}

	void	Set (int group0, int group1, bool onoff)
	{
		if (!In_Range (group0) || !In_Range (group1)) { return ; }

		//	the matrix is symmetric; the scene stores it that way too
		Allowed[group0][group1]	= onoff;
		Allowed[group1][group0]	= onoff;
	}

	bool	Allowed[COLLISION_GROUP_COUNT][COLLISION_GROUP_COUNT];
};

int	Run_Collision (void)
{
	TableCollisionGroupSink	matrix;
	CombatManager::Define_Collision_Groups (matrix);

	const int count = (int)(sizeof (_CollisionPairs) / sizeof (_CollisionPairs[0]));

	for (int index = 0; index < count; index ++) {
		const CollisionPairStruct &pair = _CollisionPairs[index];

		const bool collides	= matrix.Collides (pair.Group0, pair.Group1);
		const bool reversed	= matrix.Collides (pair.Group1, pair.Group0);

		Check (collides == pair.Collides, "groups %d and %d %s: %s",
				pair.Group0, pair.Group1,
				pair.Collides ? "do not collide" : "collide", pair.Why);

		Check (collides == reversed,
				"groups %d and %d collide in one direction and not the other",
				pair.Group0, pair.Group1);
	}

	::printf ("collision: %d group pairs are set the way the scene sets them\n", count);
	return _Failures;
}

}	// namespace


int	TTSelfCheck::Run (const char *which)
{
	if (which == nullptr) {
		which = "hooks";
	}

	if (::strcmp (which, "objects") == 0) {
		Run_Objects ();
	} else if (::strcmp (which, "visibility") == 0) {
		Run_Visibility ();
	} else if (::strcmp (which, "hooks") == 0) {
		Run_Hooks ();
	} else if (::strcmp (which, "dialogs") == 0) {
		Run_Dialogs ();
	} else if (::strcmp (which, "collision") == 0) {
		Run_Collision ();
	} else {
		::fprintf (stderr, "unknown self check '%s'\n", which);
		return 2;
	}

	::fflush (stdout);
	::fflush (stderr);

	return (_Failures == 0) ? 0 : 1;
}
