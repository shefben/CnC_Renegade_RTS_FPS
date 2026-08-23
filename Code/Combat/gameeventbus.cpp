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

#include "gameeventbus.h"


/*
**	Channel storage.
*/
GameEventChannelClass<ObjectCreateEventClass>			GameEventBus::ObjectCreate;
GameEventChannelClass<ObjectDestroyEventClass>		GameEventBus::ObjectDestroy;

GameEventChannelClass<LevelEventClass>					GameEventBus::PreLoadLevel;
GameEventChannelClass<LevelEventClass>					GameEventBus::LevelLoaded;
GameEventChannelClass<LevelEventClass>					GameEventBus::LevelUnloaded;

GameEventChannelClass<PlayerJoinEventClass>			GameEventBus::PlayerJoin;
GameEventChannelClass<PlayerLeaveEventClass>			GameEventBus::PlayerLeave;
GameEventChannelClass<ConnectionAcceptEventClass>	GameEventBus::ConnectionAccept;

GameEventChannelClass<ChatEventClass>					GameEventBus::Chat;
GameEventChannelClass<HostMessageEventClass>			GameEventBus::HostMessage;
GameEventChannelClass<RadioEventClass>					GameEventBus::Radio;

GameEventChannelClass<ServerLifecycleEventClass>	GameEventBus::ServerStartup;
GameEventChannelClass<ServerLifecycleEventClass>	GameEventBus::ServerShutdown;

GameEventChannelClass<PurchaseEventClass>				GameEventBus::Purchase;
GameEventChannelClass<PurchaseCompleteEventClass>	GameEventBus::PurchaseComplete;
GameEventChannelClass<RefillEventClass>					GameEventBus::Refill;

GameEventChannelClass<DamageEventClass>				GameEventBus::Damage;
GameEventChannelClass<KillEventClass>					GameEventBus::Kill;

GameEventChannelClass<WeaponFireEventClass>			GameEventBus::WeaponFire;
GameEventChannelClass<WeaponChangedEventClass>		GameEventBus::WeaponChanged;
GameEventChannelClass<PlayerKeyEventClass>			GameEventBus::PlayerKey;

GameEventChannelClass<DialogEventClass>				GameEventBus::Dialog;
GameEventChannelClass<ConsoleOutputEventClass>		GameEventBus::ConsoleOutput;
GameEventChannelClass<ConsoleInputEventClass>		GameEventBus::ConsoleInput;
GameEventChannelClass<ConsolePrintEventClass>		GameEventBus::ConsolePrint;
GameEventChannelClass<RenderNotifyEventClass>		GameEventBus::RenderNotify;

GameEventChannelClass<GameModeEventClass>				GameEventBus::GameModeStart;
GameEventChannelClass<GameModeEventClass>				GameEventBus::GameModeStop;
GameEventChannelClass<GameOverEventClass>				GameEventBus::GameOver;
GameEventChannelClass<ThinkEventClass>					GameEventBus::Think;

GameEventChannelClass<NetworkVisibilityEventClass>	GameEventBus::NetworkVisibility;
GameEventChannelClass<NetworkDirtyEventClass>		GameEventBus::NetworkDirty;

GameEventChannelClass<ClientQueryEventClass>		GameEventBus::ClientQuery;


/*
**	Init exists so the bus has a named place in the engine startup order even
**	though the channels are self-initializing.  Shutdown is the one that
**	matters: a subscriber whose lifetime is tied to a level must not survive
**	into the next one.
*/
void
GameEventBus::Init (void)
{
	Shutdown ();
	return ;
}


void
GameEventBus::Shutdown (void)
{
	ObjectCreate.Reset ();
	ObjectDestroy.Reset ();

	PreLoadLevel.Reset ();
	LevelLoaded.Reset ();
	LevelUnloaded.Reset ();

	PlayerJoin.Reset ();
	PlayerLeave.Reset ();
	ConnectionAccept.Reset ();

	Chat.Reset ();
	HostMessage.Reset ();
	Radio.Reset ();

	ServerStartup.Reset ();
	ServerShutdown.Reset ();

	Purchase.Reset ();
	PurchaseComplete.Reset ();
	Refill.Reset ();

	Damage.Reset ();
	Kill.Reset ();

	WeaponFire.Reset ();
	WeaponChanged.Reset ();
	PlayerKey.Reset ();

	Dialog.Reset ();
	ConsoleOutput.Reset ();
	ConsoleInput.Reset ();
	ConsolePrint.Reset ();
	RenderNotify.Reset ();

	GameModeStart.Reset ();
	GameModeStop.Reset ();
	GameOver.Reset ();
	Think.Reset ();

	NetworkVisibility.Reset ();
	NetworkDirty.Reset ();

	ClientQuery.Reset ();

	return ;
}


/*
**	Object lifetime.
*/
void
GameEventBus::Raise_Object_Create (BaseGameObj *obj)
{
	if (obj == nullptr || !ObjectCreate.Has_Subscribers ()) {
		return ;
	}

	ObjectCreateEventClass event (obj);
	ObjectCreate.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Object_Destroy (BaseGameObj *obj)
{
	if (obj == nullptr || !ObjectDestroy.Has_Subscribers ()) {
		return ;
	}

	ObjectDestroyEventClass event (obj);
	ObjectDestroy.Dispatch (event);
	return ;
}


/*
**	Level lifecycle.
*/
void
GameEventBus::Raise_Pre_Load_Level (const char *level_name)
{
	LevelEventClass event (level_name);
	PreLoadLevel.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Level_Loaded (const char *level_name)
{
	LevelEventClass event (level_name);
	LevelLoaded.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Level_Unloaded (const char *level_name)
{
	LevelEventClass event (level_name);
	LevelUnloaded.Dispatch (event);
	return ;
}


/*
**	Player lifecycle.
*/
void
GameEventBus::Raise_Player_Join (int player_id, const char *player_name)
{
	PlayerJoinEventClass event (player_id, player_name);
	PlayerJoin.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Player_Leave (int player_id)
{
	PlayerLeaveEventClass event (player_id);
	PlayerLeave.Dispatch (event);
	return ;
}


int
GameEventBus::Raise_Connection_Accept (const wchar_t *player_name, const wchar_t *password)
{
	ConnectionAcceptEventClass event (player_name, password);
	ConnectionAccept.Dispatch (event);
	return event.RefusalCode;
}


/*
**	Communications.
*/
bool
GameEventBus::Raise_Chat (int player_id, TextMessageEnum type, const wchar_t *message, int receiver_id)
{
	ChatEventClass event (player_id, type, message, receiver_id);
	Chat.Dispatch (event);
	return event.Allow;
}


bool
GameEventBus::Raise_Host_Message (int player_id, TextMessageEnum type, const char *message)
{
	HostMessageEventClass event (player_id, type, message);
	HostMessage.Dispatch (event);
	return event.Allow;
}


bool
GameEventBus::Raise_Radio (int player_type, int player_id, int announcement_id, int icon_id, AnnouncementEnum type)
{
	RadioEventClass event (player_type, player_id, announcement_id, icon_id, type);
	Radio.Dispatch (event);
	return event.Allow;
}


/*
**	Host/server lifecycle.
*/
void
GameEventBus::Raise_Server_Startup (bool is_dedicated)
{
	ServerLifecycleEventClass event (is_dedicated);
	ServerStartup.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Server_Shutdown (bool is_dedicated)
{
	ServerLifecycleEventClass event (is_dedicated);
	ServerShutdown.Dispatch (event);
	return ;
}


/*
**	Purchase.
*/
PurchaseStatusEnum
GameEventBus::Raise_Purchase (PurchaseCategoryEnum category, BaseControllerClass *base,
		ScriptableGameObj *purchaser, unsigned int cost, unsigned int preset_id)
{
	if (!Purchase.Has_Subscribers ()) {
		return PURCHASE_STATUS_ALLOW;
	}

	PurchaseEventClass event (category, base, purchaser, cost, preset_id);
	Purchase.Dispatch (event);
	return event.Status;
}


void
GameEventBus::Raise_Purchase_Complete (PurchaseCategoryEnum category, BaseControllerClass *base,
		ScriptableGameObj *purchaser, unsigned int cost, unsigned int preset_id, int result)
{
	if (!PurchaseComplete.Has_Subscribers ()) {
		return ;
	}

	PurchaseCompleteEventClass event (category, base, purchaser, cost, preset_id, result);
	PurchaseComplete.Dispatch (event);
	return ;
}


bool
GameEventBus::Raise_Refill (ScriptableGameObj *purchaser)
{
	RefillEventClass event (purchaser);
	Refill.Dispatch (event);
	return event.Allow;
}


/*
**	Combat.
*/
bool
GameEventBus::Raise_Damage (PhysicalGameObj *damager, PhysicalGameObj *target, float amount,
		unsigned int warhead_id, const AmmoDefinitionClass *ammo, const char *bone)
{
	if (!Damage.Has_Subscribers ()) {
		return true;
	}

	DamageEventClass event (damager, target, amount, warhead_id, ammo, bone);
	Damage.Dispatch (event);
	return event.Allow;
}


void
GameEventBus::Raise_Kill (PhysicalGameObj *victim, PhysicalGameObj *killer)
{
	KillEventClass event (victim, killer);
	Kill.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Weapon_Fire (PhysicalGameObj *shooter, WeaponClass *weapon, bool primary)
{
	if (!WeaponFire.Has_Subscribers ()) {
		return ;
	}

	WeaponFireEventClass event (shooter, weapon, primary);
	WeaponFire.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Weapon_Changed (PhysicalGameObj *owner, WeaponClass *weapon)
{
	if (!WeaponChanged.Has_Subscribers ()) {
		return ;
	}

	WeaponChangedEventClass event (owner, weapon);
	WeaponChanged.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Player_Key (int player_id, const char *key_name)
{
	PlayerKeyEventClass event (player_id, key_name);
	PlayerKey.Dispatch (event);
	return ;
}


/*
**	Presentation and diagnostics.
*/
void
GameEventBus::Raise_Dialog (int player_id, int dialog_id, int control_id, DialogMessageTypeEnum type)
{
	DialogEventClass event (player_id, dialog_id, control_id, type);
	Dialog.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Console_Output (const char *text)
{
	//
	//	Console output is raised from inside the print path, so a handler that
	//	printed would recurse.  Keeping the no-subscriber case free means the
	//	print path pays nothing when nothing is listening.
	//
	if (text == nullptr || !ConsoleOutput.Has_Subscribers ()) {
		return ;
	}

	ConsoleOutputEventClass event (text);
	ConsoleOutput.Dispatch (event);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Raise_Console_Input
//
////////////////////////////////////////////////////////////////
void
GameEventBus::Raise_Console_Input (const char *text)
{
	if (!ConsoleInput.Has_Subscribers ()) {
		return ;
	}

	ConsoleInputEventClass event (text);
	ConsoleInput.Dispatch (event);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Raise_Console_Print
//
////////////////////////////////////////////////////////////////
void
GameEventBus::Raise_Console_Print (const char *text)
{
	if (text == nullptr || !ConsolePrint.Has_Subscribers ()) {
		return ;
	}

	ConsolePrintEventClass event (text);
	ConsolePrint.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Render_Notify (int id, int notify)
{
	if (!RenderNotify.Has_Subscribers ()) {
		return ;
	}

	RenderNotifyEventClass event (id, notify);
	RenderNotify.Dispatch (event);
	return ;
}


/*
**	Game mode lifecycle.
*/
void
GameEventBus::Raise_Game_Mode_Start (GameModeClass *mode)
{
	GameModeEventClass event (mode);
	GameModeStart.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Game_Mode_Stop (GameModeClass *mode)
{
	GameModeEventClass event (mode);
	GameModeStop.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Game_Over (int winner_team)
{
	GameOverEventClass event (winner_team);
	GameOver.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Think (void)
{
	if (!Think.Has_Subscribers ()) {
		return ;
	}

	ThinkEventClass event;
	Think.Dispatch (event);
	return ;
}


/*
**	Replication.
*/
void
GameEventBus::Raise_Network_Visibility (NetworkObjectClass *object, int client_id, bool visible)
{
	if (!NetworkVisibility.Has_Subscribers ()) {
		return ;
	}

	NetworkVisibilityEventClass event (object, client_id, visible);
	NetworkVisibility.Dispatch (event);
	return ;
}


void
GameEventBus::Raise_Network_Dirty (NetworkObjectClass *object, int client_id, int bit)
{
	if (!NetworkDirty.Has_Subscribers ()) {
		return ;
	}

	NetworkDirtyEventClass event (object, client_id, bit);
	NetworkDirty.Dispatch (event);
	return ;
}

/*
**	The one query on the bus.  Combat can address a client by id but has no
**	roster; the game answers with one, and nothing in the editor does.  A false
**	return means "nobody can tell you", which is different from "nobody is on
**	that team" -- callers must not treat it as an empty list.
*/
bool
GameEventBus::Raise_Client_Query (int team, DynamicVectorClass<int> &client_ids)
{
	if (!ClientQuery.Has_Subscribers ()) {
		return false;
	}

	ClientQueryEventClass event (team);
	ClientQuery.Dispatch (event);

	if (!event.Answered) {
		return false;
	}

	client_ids = event.ClientIDs;
	return true;
}
