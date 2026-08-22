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
#include "Mute.h"
#include "gmgame.h"
#include "Iterator.h"
#include "CommandLineParser.h"
#include "engine_tt.h"

bool MutedPlayers[128];

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


class CommandMUTE :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "Mute"; 
	}
	const char* Get_Help() 
	{ 
		return "MUTE <clientId> - Mutes a client."; 
	}
	void Activate(const char* argumentsString)
	{
		CommandLineParser arguments(argumentsString);
		const char* clientIdentifier = arguments.getString();

		if (!clientIdentifier || clientIdentifier[0] == '\0')
			Console_Output("Please enter a client identifier.");
		else
		{
			const int clientId = getClientIdByIdentifier(clientIdentifier);
			if (!isClientId(clientId))
			{
				Console_Output("Please enter a valid client identifier.");
			}
			else
			{
				MutedPlayers[clientId] = true;
			}
		}
	}
};


class CommandUNMUTE :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "UnMute"; 
	}
	const char* Get_Help() 
	{ 
		return "UNMUTE <clientId> - Un-Mutes a client."; 
	}
	void Activate(const char* argumentsString)
	{
		CommandLineParser arguments(argumentsString);
		const char* clientIdentifier = arguments.getString();

		if (!clientIdentifier || clientIdentifier[0] == '\0')
			Console_Output("Please enter a client identifier.");
		else
		{
			const int clientId = getClientIdByIdentifier(clientIdentifier);
			if (!isClientId(clientId))
			{
				Console_Output("Please enter a valid client identifier.");
			}
			else
			{
				MutedPlayers[clientId] = false;
			}
		}
	}
};


class MUTE : public Plugin
{
public:
	MUTE()
	{
		ConsoleFunctionList.Add(new CommandMUTE);
		ConsoleFunctionList.Add(new CommandUNMUTE);
		Sort_Function_List();
		Verbose_Help_File();
		Console_Output("Loading Mute Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_CHAT_HOOK,this);
		RegisterEvent(EVENT_RADIO_HOOK,this);
		RegisterEvent(EVENT_PLAYER_LEAVE_HOOK,this);
	}
	~MUTE()
	{
		Console_Output("Un-loading Mute Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_CHAT_HOOK,this);
		UnregisterEvent(EVENT_RADIO_HOOK,this);
		UnregisterEvent(EVENT_PLAYER_LEAVE_HOOK,this);
		Delete_Console_Function("Mute");
		Delete_Console_Function("UnMute");
	}


	virtual bool OnChat(int PlayerID,TextMessageEnum Type,const wchar_t *Message,int recieverID)
	{
		if(MutedPlayers[PlayerID] == false)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual bool OnRadioCommand(int PlayerType, int PlayerID, int AnnouncementID, int IconID, AnnouncementEnum AnnouncementType)
	{
		if(MutedPlayers[PlayerID] == false)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	virtual void OnPlayerLeave(int PlayerID)
	{
		MutedPlayers[PlayerID] = false;
	}
	
};

MUTE mute;


extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &mute;
}
