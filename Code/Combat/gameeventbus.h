/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef	GAMEEVENTBUS_H
#define	GAMEEVENTBUS_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

#ifndef	VECTOR_H
	#include "vector.h"
#endif

#ifndef	WWDEBUG_H
	#include "wwdebug.h"
#endif

#ifndef	COMMUNICATIONTYPES_H
	#include "communicationtypes.h"
#endif

class	BaseControllerClass;
class	BaseGameObj;
class	GameModeClass;
class	NetworkObjectClass;
class	PhysicalGameObj;
class	ScriptableGameObj;
class	WeaponClass;
class	AmmoDefinitionClass;


/*
**	The native game event bus.
**
**	This is the replacement for the closed-binary patch architecture the 4.8.4
**	scripts package used to reach engine behavior.  Every extension point that
**	package installed by overwriting executable memory is expressed here as a
**	typed event raised from the canonical OpenW3D function that owns the
**	behavior, so a subscriber names an event rather than an address.
**
**	Properties the rest of the engine may rely on:
**
**	- Deterministic.  Handlers run in registration order, on the thread that
**	  raised the event, before the raising function continues.
**	- Explicit.  An event exists only where a canonical owner raises it; there
**	  is no catch-all dispatch and no ordering by priority number.
**	- Re-entrant.  Registering or unregistering from inside a handler is legal.
**	  A handler added during dispatch does not see the event in flight; one
**	  removed during dispatch does not run again.
**	- Server safe.  Nothing here touches rendering, input or dialogs, so a
**	  dedicated server raises exactly the events a listen server does minus the
**	  ones whose owner is client-only.
*/


/*
**	Base for every event payload.
**
**	A handler that has answered definitively -- consumed the input, resolved
**	the decision -- calls Stop_Dispatch and the remaining handlers are skipped.
**	Vetoable events additionally carry their own decision field; stopping
**	dispatch is not by itself a veto.
*/
class	GameEventClass
{
public:
	GameEventClass (void)					: IsDispatchStopped (false)	{ }

	void			Stop_Dispatch (void)		{ IsDispatchStopped = true; }
	bool			Is_Dispatch_Stopped (void) const	{ return IsDispatchStopped; }

private:
	bool			IsDispatchStopped;
};


/*
**	One event family.
**
**	EVENT is the payload type; handlers take it by reference so a vetoable
**	event can be answered in place.
*/
template <class EVENT>
class	GameEventChannelClass
{
public:

	typedef void (*HandlerFunc) (EVENT &event, void *data);

	GameEventChannelClass (void)	: NextID (1), DispatchDepth (0), NeedsCompact (false)	{ }

	//
	//	Returns a token for Unregister.  Zero is never a valid token, so a
	//	subscriber may use it to mean not-subscribed.
	//
	int			Register (HandlerFunc handler, void *data = nullptr);
	void			Unregister (int token);

	//
	//	True when at least one live handler is attached.  Raising an event is
	//	cheap, but an owner that would have to do real work to fill the payload
	//	can skip it when nobody is listening.
	//
	bool			Has_Subscribers (void) const;
	int			Subscriber_Count (void) const;

	void			Dispatch (EVENT &event);

	//
	//	Drops every subscriber.  Called on engine shutdown; also what a test
	//	uses to isolate one channel from another.
	//
	void			Reset (void);

private:

	struct SubscriptionStruct
	{
		HandlerFunc	Handler;
		void *		Data;
		int			Token;

		//
		//	DynamicVectorClass compares elements when it grows and when it
		//	deletes; the token alone identifies a subscription.
		//
		bool	operator == (const SubscriptionStruct &other) const	{ return Token == other.Token; }
		bool	operator != (const SubscriptionStruct &other) const	{ return Token != other.Token; }
	};

	void			Compact (void);

	DynamicVectorClass<SubscriptionStruct>	Subscriptions;
	int			NextID;
	int			DispatchDepth;
	bool			NeedsCompact;
};


/*
**	Event payloads.
*/

//
//	Object lifetime.  Create is raised once the object is in the game object
//	list and has its network ID; Destroy is raised while the object is still
//	fully constructed, immediately before it leaves the list.
//
class	ObjectCreateEventClass : public GameEventClass
{
public:
	ObjectCreateEventClass (BaseGameObj *obj)	: Object (obj)	{ }
	BaseGameObj *	Object;
};

class	ObjectDestroyEventClass : public GameEventClass
{
public:
	ObjectDestroyEventClass (BaseGameObj *obj)	: Object (obj)	{ }
	BaseGameObj *	Object;
};

//
//	Level lifecycle.  PreLoad runs before any client has been told about the
//	new level, which is the only point at which a subscriber can still change
//	what the clients will be sent.
//
class	LevelEventClass : public GameEventClass
{
public:
	LevelEventClass (const char *name)	: LevelName (name)	{ }
	const char *	LevelName;
};

//
//	Player lifecycle.
//
class	PlayerJoinEventClass : public GameEventClass
{
public:
	PlayerJoinEventClass (int id, const char *name)	: PlayerID (id), PlayerName (name)	{ }
	int				PlayerID;
	const char *	PlayerName;
};

class	PlayerLeaveEventClass : public GameEventClass
{
public:
	PlayerLeaveEventClass (int id)	: PlayerID (id)	{ }
	int				PlayerID;
};

//
//	Connection acceptance.  Raised on the server after the stock checks have
//	passed and while the request can still be refused.  A handler refuses by
//	setting RefusalCode to one of the REFUSAL_ values from connect.h, which is
//	what decides the message the client is shown.
//
class	ConnectionAcceptEventClass : public GameEventClass
{
public:
	ConnectionAcceptEventClass (const wchar_t *name, const wchar_t *password)
		:	PlayerName (name), Password (password), RefusalCode (0)	{ }

	const wchar_t *	PlayerName;
	const wchar_t *	Password;
	int					RefusalCode;
};

//
//	Chat.  Raised on the server for a message that has arrived and not yet been
//	distributed; clearing Allow drops it silently.
//
class	ChatEventClass : public GameEventClass
{
public:
	ChatEventClass (int sender, TextMessageEnum type, const wchar_t *text, int receiver)
		:	PlayerID (sender), Type (type), Message (text), ReceiverID (receiver), Allow (true)	{ }

	int					PlayerID;
	TextMessageEnum	Type;
	const wchar_t *	Message;
	int					ReceiverID;
	bool					Allow;
};

//
//	A message originating from the host/console rather than from a player.
//
class	HostMessageEventClass : public GameEventClass
{
public:
	HostMessageEventClass (int sender, TextMessageEnum type, const char *text)
		:	PlayerID (sender), Type (type), Message (text), Allow (true)	{ }

	int					PlayerID;
	TextMessageEnum	Type;
	const char *		Message;
	bool					Allow;
};

//
//	Radio commands / battlefield announcements.
//
class	RadioEventClass : public GameEventClass
{
public:
	RadioEventClass (int player_type, int player_id, int announcement_id, int icon_id, AnnouncementEnum type)
		:	PlayerType (player_type), PlayerID (player_id), AnnouncementID (announcement_id),
			IconID (icon_id), Type (type), Allow (true)	{ }

	int					PlayerType;
	int					PlayerID;
	int					AnnouncementID;
	int					IconID;
	AnnouncementEnum	Type;
	bool					Allow;
};

//
//	Host/server lifecycle.  Startup is raised once the server is accepting
//	connections; Shutdown while it still is, so a handler can still send.
//
class	ServerLifecycleEventClass : public GameEventClass
{
public:
	ServerLifecycleEventClass (bool dedicated)	: IsDedicated (dedicated)	{ }
	bool			IsDedicated;
};

//
//	Purchase.  The status a handler leaves behind is what the purchase path
//	returns, so a handler both vetoes and chooses the refusal the player sees.
//	The three allow values below are decisions, not errors: they let a handler
//	pass the purchase through while changing what it costs or spawns.
//
enum PurchaseCategoryEnum
{
	PURCHASE_CATEGORY_POWERUP,
	PURCHASE_CATEGORY_VEHICLE,
	PURCHASE_CATEGORY_CHARACTER,
};

enum PurchaseStatusEnum
{
	PURCHASE_STATUS_ALLOW_NO_SPAWN	= -3,	// allowed, but do not spawn the vehicle
	PURCHASE_STATUS_ALLOW_FREE			= -2,	// allowed, but do not charge the player
	PURCHASE_STATUS_ALLOW				= -1,	// allowed as normal

	PURCHASE_STATUS_GRANTED				= 0,
	PURCHASE_STATUS_PENDING,
	PURCHASE_STATUS_INSUFFICIENT_FUNDS,
	PURCHASE_STATUS_FACTORY_UNAVAILABLE,
	PURCHASE_STATUS_OUT_OF_STOCK,
};

class	PurchaseEventClass : public GameEventClass
{
public:
	PurchaseEventClass (PurchaseCategoryEnum category, BaseControllerClass *base,
			ScriptableGameObj *purchaser, unsigned int cost, unsigned int preset_id)
		:	Category (category), Base (base), Purchaser (purchaser), Cost (cost),
			PresetID (preset_id), Status (PURCHASE_STATUS_ALLOW)	{ }

	PurchaseCategoryEnum		Category;
	BaseControllerClass *	Base;
	ScriptableGameObj *		Purchaser;
	unsigned int				Cost;
	unsigned int				PresetID;
	PurchaseStatusEnum		Status;
};

//
//	Raised after the purchase path has finished, whatever it decided.  A
//	handler here observes; it cannot change the outcome.
//
class	PurchaseCompleteEventClass : public GameEventClass
{
public:
	PurchaseCompleteEventClass (PurchaseCategoryEnum category, BaseControllerClass *base,
			ScriptableGameObj *purchaser, unsigned int cost, unsigned int preset_id, int result)
		:	Category (category), Base (base), Purchaser (purchaser), Cost (cost),
			PresetID (preset_id), Result (result)	{ }

	PurchaseCategoryEnum		Category;
	BaseControllerClass *	Base;
	ScriptableGameObj *		Purchaser;
	unsigned int				Cost;
	unsigned int				PresetID;
	int							Result;
};

class	RefillEventClass : public GameEventClass
{
public:
	RefillEventClass (ScriptableGameObj *purchaser)	: Purchaser (purchaser), Allow (true)	{ }
	ScriptableGameObj *	Purchaser;
	bool						Allow;
};

//
//	Damage.  Raised before armor and warhead scaling, so Amount is what the
//	weapon asked for rather than what the target will lose.  Ammo and Bone are
//	filled in where the damage came from a weapon and are null otherwise.
//
class	DamageEventClass : public GameEventClass
{
public:
	DamageEventClass (PhysicalGameObj *damager, PhysicalGameObj *target, float amount,
			unsigned int warhead, const AmmoDefinitionClass *ammo, const char *bone)
		:	Damager (damager), Target (target), Amount (amount), WarheadID (warhead),
			Ammo (ammo), Bone (bone), Allow (true)	{ }

	PhysicalGameObj *				Damager;
	PhysicalGameObj *				Target;
	float								Amount;
	unsigned int					WarheadID;
	const AmmoDefinitionClass *Ammo;
	const char *					Bone;
	bool								Allow;
};

class	KillEventClass : public GameEventClass
{
public:
	KillEventClass (PhysicalGameObj *victim, PhysicalGameObj *killer)
		:	Victim (victim), Killer (killer)	{ }

	PhysicalGameObj *	Victim;
	PhysicalGameObj *	Killer;
};

//
//	Weapon events.
//
class	WeaponFireEventClass : public GameEventClass
{
public:
	WeaponFireEventClass (PhysicalGameObj *shooter, WeaponClass *weapon, bool primary)
		:	Shooter (shooter), Weapon (weapon), IsPrimary (primary)	{ }

	PhysicalGameObj *	Shooter;
	WeaponClass *		Weapon;
	bool					IsPrimary;
};

class	WeaponChangedEventClass : public GameEventClass
{
public:
	WeaponChangedEventClass (PhysicalGameObj *owner, WeaponClass *weapon)
		:	Owner (owner), Weapon (weapon)	{ }

	PhysicalGameObj *	Owner;
	WeaponClass *		Weapon;
};

//
//	A logical key press forwarded from a client.  This is how a script attaches
//	behavior to a key without the client knowing what the key does.
//
class	PlayerKeyEventClass : public GameEventClass
{
public:
	PlayerKeyEventClass (int player_id, const char *key)
		:	PlayerID (player_id), KeyName (key)	{ }

	int				PlayerID;
	const char *	KeyName;
};

//
//	Dialog notifications forwarded from a client scripted dialog.
//
enum DialogMessageTypeEnum
{
	DIALOG_MESSAGE_SHOW,
	DIALOG_MESSAGE_CLOSE,
	DIALOG_MESSAGE_ESCAPE,
	DIALOG_MESSAGE_CONTROL_MOUSE_CLICK,
	DIALOG_MESSAGE_CONTROL_VALUE_CHANGE,
	DIALOG_MESSAGE_CONTROL_VALUE_CONFIRM,
	DIALOG_MESSAGE_CONTROL_FOCUSED,
};

class	DialogEventClass : public GameEventClass
{
public:
	DialogEventClass (int player_id, int dialog_id, int control_id, DialogMessageTypeEnum type)
		:	PlayerID (player_id), DialogID (dialog_id), ControlID (control_id), MessageType (type)	{ }

	int							PlayerID;
	int							DialogID;
	int							ControlID;
	DialogMessageTypeEnum	MessageType;
};

//
//	Everything printed to the console, including what a dedicated server would
//	otherwise only write to its log.
//
class	ConsoleOutputEventClass : public GameEventClass
{
public:
	ConsoleOutputEventClass (const char *text)	: Text (text)	{ }
	const char *	Text;
};

//
//	A line of console input that arrived from somewhere other than the console
//	itself -- a remote administration tool, typically.  Raised where the line
//	arrives; the game subscribes and parses it, and a tool with no console of
//	its own subscribes to nothing and the line is discarded.
//
class	ConsoleInputEventClass : public GameEventClass
{
public:
	ConsoleInputEventClass (const char *text)	: Text (text)	{ }
	const char *	Text;
};

//
//	Something below the console asking the console to print a line.  The
//	other half of ConsoleInput: input is a line arriving from somewhere that
//	is not the console, this is a line going out to it.  The console prints
//	it and then raises ConsoleOutput, so a logger still hears it once.
//
class	ConsolePrintEventClass : public GameEventClass
{
public:
	ConsolePrintEventClass (const char *text)	: Text (text)	{ }
	const char *	Text;
};

//
//	Render/shader notifications.  Raised on the client only.
//
class	RenderNotifyEventClass : public GameEventClass
{
public:
	RenderNotifyEventClass (int id, int notify)	: ID (id), Notify (notify)	{ }
	int	ID;
	int	Notify;
};

//
//	Game mode lifecycle.
//
class	GameModeEventClass : public GameEventClass
{
public:
	GameModeEventClass (GameModeClass *mode)	: Mode (mode)	{ }
	GameModeClass *	Mode;
};

class	GameOverEventClass : public GameEventClass
{
public:
	GameOverEventClass (int winner_team)	: WinnerTeam (winner_team)	{ }
	int	WinnerTeam;
};

//
//	Raised once per simulated frame, after the game object list has thought.
//	This is the only event with a per-frame cost, so its owner checks
//	Has_Subscribers first.
//
class	ThinkEventClass : public GameEventClass
{
public:
	ThinkEventClass (void)	{ }
};

//
//	Network visibility and dirty-bit traffic.  Raised on the server as the
//	replication decision is made, which is where the 4.8.4 package used to
//	patch to add its own per-client filtering.
//
class	NetworkVisibilityEventClass : public GameEventClass
{
public:
	NetworkVisibilityEventClass (NetworkObjectClass *object, int client_id, bool visible)
		:	Object (object), ClientID (client_id), IsVisible (visible)	{ }

	NetworkObjectClass *	Object;
	int						ClientID;
	bool						IsVisible;
};

class	NetworkDirtyEventClass : public GameEventClass
{
public:
	NetworkDirtyEventClass (NetworkObjectClass *object, int client_id, int bit)
		:	Object (object), ClientID (client_id), Bit (bit)	{ }

	NetworkObjectClass *	Object;
	int						ClientID;
	int						Bit;
};


/*
**	The bus itself.
**
**	Channels are public because a subscriber names the one it wants:
**
**		int token = GameEventBus::Chat.Register (My_Chat_Handler);
**		...
**		GameEventBus::Chat.Unregister (token);
**
**	Owners raise through the Raise_ helpers rather than touching the channel,
**	so the payload is built in exactly one place.
*/
//
//	Combat can address a client by id, but only the game knows who is connected
//	and which team they chose.  This is the one channel a listener is expected to
//	answer rather than observe: it fills in the client ids and sets Answered.
//	Nobody answers it in the editor, which has no clients, so a caller that gets
//	false back should do nothing rather than guess.
//
class	ClientQueryEventClass : public GameEventClass
{
public:
	ClientQueryEventClass (int team)	: Team (team), Answered (false)	{ }

	int									Team;			// -1 for every connected client
	bool									Answered;
	DynamicVectorClass<int>		ClientIDs;
};

class	GameEventBus
{
public:

	static void	Init (void);
	static void	Shutdown (void);

	//
	//	Channels, grouped as the integration roadmap groups them.
	//
	static GameEventChannelClass<ObjectCreateEventClass>			ObjectCreate;
	static GameEventChannelClass<ObjectDestroyEventClass>			ObjectDestroy;

	static GameEventChannelClass<LevelEventClass>					PreLoadLevel;
	static GameEventChannelClass<LevelEventClass>					LevelLoaded;
	static GameEventChannelClass<LevelEventClass>					LevelUnloaded;

	static GameEventChannelClass<PlayerJoinEventClass>				PlayerJoin;
	static GameEventChannelClass<PlayerLeaveEventClass>			PlayerLeave;
	static GameEventChannelClass<ConnectionAcceptEventClass>		ConnectionAccept;

	static GameEventChannelClass<ChatEventClass>						Chat;
	static GameEventChannelClass<HostMessageEventClass>			HostMessage;
	static GameEventChannelClass<RadioEventClass>					Radio;

	static GameEventChannelClass<ServerLifecycleEventClass>		ServerStartup;
	static GameEventChannelClass<ServerLifecycleEventClass>		ServerShutdown;

	static GameEventChannelClass<PurchaseEventClass>				Purchase;
	static GameEventChannelClass<PurchaseCompleteEventClass>		PurchaseComplete;
	static GameEventChannelClass<RefillEventClass>					Refill;

	static GameEventChannelClass<DamageEventClass>					Damage;
	static GameEventChannelClass<KillEventClass>						Kill;

	static GameEventChannelClass<WeaponFireEventClass>				WeaponFire;
	static GameEventChannelClass<WeaponChangedEventClass>			WeaponChanged;
	static GameEventChannelClass<PlayerKeyEventClass>				PlayerKey;

	static GameEventChannelClass<DialogEventClass>					Dialog;
	static GameEventChannelClass<ConsoleOutputEventClass>			ConsoleOutput;
	static GameEventChannelClass<ConsoleInputEventClass>			ConsoleInput;
	static GameEventChannelClass<ConsolePrintEventClass>			ConsolePrint;
	static GameEventChannelClass<RenderNotifyEventClass>			RenderNotify;

	static GameEventChannelClass<GameModeEventClass>				GameModeStart;
	static GameEventChannelClass<GameModeEventClass>				GameModeStop;
	static GameEventChannelClass<GameOverEventClass>				GameOver;
	static GameEventChannelClass<ThinkEventClass>					Think;

	static GameEventChannelClass<NetworkVisibilityEventClass>	NetworkVisibility;
	static GameEventChannelClass<NetworkDirtyEventClass>			NetworkDirty;

	static GameEventChannelClass<ClientQueryEventClass>			ClientQuery;

	//
	//	Raise helpers.  These are what the canonical owners call; each returns
	//	whatever decision the event carries, so the owner reads one value.
	//
	static void	Raise_Object_Create (BaseGameObj *obj);
	static void	Raise_Object_Destroy (BaseGameObj *obj);

	static void	Raise_Pre_Load_Level (const char *level_name);
	static void	Raise_Level_Loaded (const char *level_name);
	static void	Raise_Level_Unloaded (const char *level_name);

	static void	Raise_Player_Join (int player_id, const char *player_name);
	static void	Raise_Player_Leave (int player_id);
	static int	Raise_Connection_Accept (const wchar_t *player_name, const wchar_t *password);

	static bool	Raise_Chat (int player_id, TextMessageEnum type, const wchar_t *message, int receiver_id);
	static bool	Raise_Host_Message (int player_id, TextMessageEnum type, const char *message);
	static bool	Raise_Radio (int player_type, int player_id, int announcement_id, int icon_id, AnnouncementEnum type);

	static void	Raise_Server_Startup (bool is_dedicated);
	static void	Raise_Server_Shutdown (bool is_dedicated);

	static PurchaseStatusEnum	Raise_Purchase (PurchaseCategoryEnum category, BaseControllerClass *base,
										ScriptableGameObj *purchaser, unsigned int cost, unsigned int preset_id);
	static void	Raise_Purchase_Complete (PurchaseCategoryEnum category, BaseControllerClass *base,
										ScriptableGameObj *purchaser, unsigned int cost, unsigned int preset_id, int result);
	static bool	Raise_Refill (ScriptableGameObj *purchaser);

	static bool	Raise_Damage (PhysicalGameObj *damager, PhysicalGameObj *target, float amount,
										unsigned int warhead_id, const AmmoDefinitionClass *ammo, const char *bone);
	static void	Raise_Kill (PhysicalGameObj *victim, PhysicalGameObj *killer);

	static void	Raise_Weapon_Fire (PhysicalGameObj *shooter, WeaponClass *weapon, bool primary);
	static void	Raise_Weapon_Changed (PhysicalGameObj *owner, WeaponClass *weapon);
	static void	Raise_Player_Key (int player_id, const char *key_name);

	static void	Raise_Dialog (int player_id, int dialog_id, int control_id, DialogMessageTypeEnum type);
	static void	Raise_Console_Output (const char *text);
	static void	Raise_Console_Input (const char *text);
	static void	Raise_Console_Print (const char *text);
	static void	Raise_Render_Notify (int id, int notify);

	static void	Raise_Game_Mode_Start (GameModeClass *mode);
	static void	Raise_Game_Mode_Stop (GameModeClass *mode);
	static void	Raise_Game_Over (int winner_team);
	static void	Raise_Think (void);

	static void	Raise_Network_Visibility (NetworkObjectClass *object, int client_id, bool visible);
	static void	Raise_Network_Dirty (NetworkObjectClass *object, int client_id, int bit);

	static bool	Raise_Client_Query (int team, DynamicVectorClass<int> &client_ids);
};


/*
**	Channel implementation.
*/

template <class EVENT>
int
GameEventChannelClass<EVENT>::Register (HandlerFunc handler, void *data)
{
	if (handler == nullptr) {
		return 0;
	}

	SubscriptionStruct entry;
	entry.Handler	= handler;
	entry.Data		= data;
	entry.Token		= NextID ++;

	Subscriptions.Add (entry);
	return entry.Token;
}


template <class EVENT>
void
GameEventChannelClass<EVENT>::Unregister (int token)
{
	if (token == 0) {
		return ;
	}

	for (int index = 0; index < Subscriptions.Count (); index ++) {
		if (Subscriptions[index].Token == token) {

			//
			//	Clearing the handler rather than removing the entry keeps a
			//	dispatch that is walking the vector right now from skipping
			//	its neighbour.
			//
			Subscriptions[index].Handler = nullptr;

			if (DispatchDepth == 0) {
				Subscriptions.Delete (index);
			} else {
				NeedsCompact = true;
			}

			return ;
		}
	}
}


template <class EVENT>
bool
GameEventChannelClass<EVENT>::Has_Subscribers (void) const
{
	for (int index = 0; index < Subscriptions.Count (); index ++) {
		if (Subscriptions[index].Handler != nullptr) {
			return true;
		}
	}

	return false;
}


template <class EVENT>
int
GameEventChannelClass<EVENT>::Subscriber_Count (void) const
{
	int count = 0;

	for (int index = 0; index < Subscriptions.Count (); index ++) {
		if (Subscriptions[index].Handler != nullptr) {
			count ++;
		}
	}

	return count;
}


template <class EVENT>
void
GameEventChannelClass<EVENT>::Dispatch (EVENT &event)
{
	//
	//	The count is read once: a handler that subscribes during dispatch is
	//	not shown the event already in flight.
	//
	int count = Subscriptions.Count ();

	DispatchDepth ++;

	for (int index = 0; index < count; index ++) {

		HandlerFunc handler = Subscriptions[index].Handler;
		if (handler == nullptr) {
			continue;
		}

		handler (event, Subscriptions[index].Data);

		if (event.Is_Dispatch_Stopped ()) {
			break;
		}
	}

	DispatchDepth --;

	if (DispatchDepth == 0 && NeedsCompact) {
		Compact ();
	}
}


template <class EVENT>
void
GameEventChannelClass<EVENT>::Compact (void)
{
	for (int index = Subscriptions.Count () - 1; index >= 0; index --) {
		if (Subscriptions[index].Handler == nullptr) {
			Subscriptions.Delete (index);
		}
	}

	NeedsCompact = false;
}


template <class EVENT>
void
GameEventChannelClass<EVENT>::Reset (void)
{
	WWASSERT (DispatchDepth == 0);

	Subscriptions.Delete_All ();
	NeedsCompact	= false;
	NextID			= 1;
}

#endif	// GAMEEVENTBUS_H
