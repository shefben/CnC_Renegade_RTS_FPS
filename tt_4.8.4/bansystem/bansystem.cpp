/*	Renegade Scripts.dll
	Copyright 2013 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "General.h"
#include "BanSystem.h"
#include "CommandLineParser.h"
#include "ConnectionRequest.h"
#include "Iterator.h"
#include "engine_tt.h"
#include "gmgame.h"

static int getClientIdByIdentifier(const char* clientIdentifier)
{
	TT_ASSERT(clientIdentifier);
	const cPlayer* player = Find_Player(atoi(clientIdentifier));
	int result;
	if (player)
		result = player->Get_Id();
	else
		result = -1;
	return result;
}

static bool isClientId(const int id)
{
	return id > 0 && id < 128 && Find_Player(id);
}

class CommandKICK :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() { return "kick"; }
	const char* Get_Help() { return "KICK <clientId> [<reason>] - Kick a user from the game."; }
	void Activate(const char* argumentsString)
	{
		CommandLineParser arguments(argumentsString);
		const char* clientIdentifier = arguments.getString();
		const char* reason = arguments.getRemainingString();

		if (!clientIdentifier || clientIdentifier[0] == '\0')
			Console_Output("Please enter a client identifier.");
		else
		{
			const int clientId = getClientIdByIdentifier(clientIdentifier);
			if (!isClientId(clientId))
				Console_Output("Please enter a valid client identifier.");
			else
				banSystem.kick(clientId, reason ? reason : "");
		}
	}
};

class CommandBAN :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() { return "ban"; }
	const char* Get_Help() { return "BAN <clientId> [<reason>] - Permanently ban a user from this server."; }
	void Activate(const char* argumentsString)
	{
		CommandLineParser arguments(argumentsString);
		const char* clientIdentifier = arguments.getString();
		const char* reason = arguments.getRemainingString();

		if (!clientIdentifier || clientIdentifier[0] == '\0')
			Console_Output("Please enter a client identifier.");
		else
		{
			const int clientId = getClientIdByIdentifier(clientIdentifier);
			if (!isClientId(clientId))
				Console_Output("Please enter a valid client identifier.");
			else
				banSystem.ban(clientId, reason ? reason : "");
		}
	}
};

class CommandREHASH_BAN_LIST :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() { return "rehash_ban_list"; }
	const char* Get_Help() { return "REHASH_BAN_LIST - Reload the ban list."; }
	void Activate(const char* argumentsString)
	{
		banSystem.rehash();
	}
};

void BanSystem::evict(int clientId, const WideStringClass& reason)
{
	Evict_Client(clientId, reason);
}

BanSystem::BanSystem()
	: banListStore(StringClass::getFormattedString("%sbanList.tsv", Get_File_Path()))
{
	minVersion = 0;
	addConnectionAcceptanceFilter(this);
	banListStore.loadEntries(banList);

	ConsoleFunctionList.Add(new CommandKICK);
	ConsoleFunctionList.Add(new CommandBAN);
	ConsoleFunctionList.Add(new CommandREHASH_BAN_LIST);
	Sort_Function_List();
	Verbose_Help_File();
	RegisterEvent(EVENT_GLOBAL_INI,this);
}

BanSystem::~BanSystem()
{
	UnregisterEvent(EVENT_GLOBAL_INI,this);
	Delete_Console_Function("kick");
	Delete_Console_Function("ban");
	Delete_Console_Function("rehash_ban_list");
	
	removeConnectionAcceptanceFilter(this);
}

void BanSystem::rehash()
{
	banListStore.loadEntries(banList);
}

void BanSystem::kick(int clientId, const WideStringClass& reason)
{
	if (reason.Is_Empty())
		evict(clientId, L"You have been kicked");
	else
		evict(clientId, WideStringClass::getFormattedString(L"You have been kicked for %s", reason));
}

void BanSystem::ban(int clientId, const WideStringClass& reason)
{
	cPlayer* player = Find_Player(clientId);
	TT_ASSERT(player);

	const StringClass clientName = player->Get_Name();
	const IpAddress clientIp = player->Get_Ip_Address();
	const char* clientSerial = Get_Client_Serial_Hash(clientId);

	BanList::Entry banEntry(clientName, clientIp, clientSerial, StringClass(reason));
	banList.addEntry(banEntry);
	banListStore.storeEntry(banEntry);
	if (reason.Is_Empty())
		evict(clientId, L"You have been banned");
	else
		evict(clientId, WideStringClass::getFormattedString(L"You have been banned for %S", banEntry.getBanReason()));
}
	
void BanSystem::handleInitiation(const ConnectionRequest& connectionRequest)
{
}
	
void BanSystem::handleTermination(const ConnectionRequest& connectionRequest)
{
}
	
void BanSystem::handleCancellation(const ConnectionRequest& connectionRequest)
{
}
	
ConnectionAcceptanceFilter::STATUS BanSystem::getStatus(const ConnectionRequest& connectionRequest, WideStringClass& refusalMessage)
{
	STATUS status;
	if (connectionRequest.clientSerialHash.Is_Empty()) // Called when old clients have not yet sent their serial numbers.
		status = STATUS_INDETERMINATE;
	else
	{
		const BanList::Entry* banEntry = banList.getEntryForPlayer(StringClass(connectionRequest.clientName), connectionRequest.clientAddress.sin_addr, connectionRequest.clientSerialHash);
		if (!banEntry)
			status = STATUS_ACCEPTING;
		else
		{
			StringClass reason = banEntry->getBanReason();
			if (reason.Is_Empty())
				refusalMessage = L"You are banned";
			else
				refusalMessage.Format(L"You are banned for %S", reason);
			status = STATUS_REFUSING;
		}
	}
	if (connectionRequest.clientVersion < minVersion)
	{
		refusalMessage.Format(L"You need a newer version of scripts.dll to join this server. Please get the newest version from http://www.tiberiantechnologies.org/");
		status = STATUS_REFUSING;
	}
	return status;
}

void BanSystem::OnLoadGlobalINISettings(INIClass *SSGMIni)
{
	minVersion = SSGMIni->Get_Float("BanSystem","MinScriptsVersion",0);
}

BanSystem banSystem;


extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &banSystem;
}
