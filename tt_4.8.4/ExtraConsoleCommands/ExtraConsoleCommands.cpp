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
#include "ExtraConsoleCommands.h"
#include "gmgame.h"
#include "Iterator.h"
#include "CommandLineParser.h"
#include "GameObjManager.h"
#include "gmlog.h"
#include "engine_tt.h"

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



class CommandTAKECREDITS :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "TakeCredits"; 
	}
	const char* Get_Help() 
	{ 
		return "TAKECREDITS <clientId> <amount> - Takes specified amount of credits (please enter a positive amount) from the client."; 
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
				int amount = arguments.getInt();
				if(!amount)
				{
					Console_Output("Please enter an amount of credits (int) to take from the client.");
				}
				else
				{
					amount = amount - (amount * 2);
					Commands->Give_Money(Get_GameObj(clientId), (float)amount, false);
					Console_Output("Credits have been taken.");
				}
			}
		}
	}
};



class CommandKILL :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "Kill"; 
	}
	const char* Get_Help() 
	{ 
		return "KILL <clientId> - Kills the player's character (if they're driving a vehicle it will kill that too)."; 
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
				GameObject *obj = Get_GameObj(clientId);
				GameObject *Veh = Get_Vehicle(obj);
				if (Veh)
				{
					if (Get_Vehicle_Driver(Veh) == obj) 
					{
						Commands->Apply_Damage(Veh,99999.0f,"Death",false);
					}
					Commands->Destroy_Object(obj);
				}
				else 
				{
					Commands->Apply_Damage(obj,99999.0f,"Death",false);
					Console_Output("Player has been killed.");
				}
			}
		}
	}
};



class CommandTAKEPOINTS :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "TakePoints"; 
	}
	const char* Get_Help() 
	{ 
		return "TAKEPOINTS <clientId> <amount> - Takes the specified amount of points from the player (please enter a positive amount)."; 
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
				int amount = arguments.getInt();
				if(!amount)
				{
					Console_Output("Please enter an amount of points (int) to remove from the client.");
				}
				else
				{
					amount = amount - (amount * 2);
					Commands->Give_Points(Get_GameObj(clientId), (float)amount, false);
					Console_Output("Points have been taken.");
				}
			}
		}
	}
};



class CommandGIVEPOW :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "GivePow"; 
	}
	const char* Get_Help() 
	{ 
		return "GIVEPOW <clientId> <PowerUp> - Gives the specified power-up to the player."; 
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
				const char* pow = arguments.getString();
				if (!pow || pow[0] == '\0')
				{
					Console_Output("Please enter a power-up string to grant the player.");
				}
				else
				{
					Commands->Give_PowerUp(Get_GameObj(clientId), pow, true);
					Console_Output("Power-up has been given.");
				}
			}
		}
	}
};



class CommandATTACHSCRIPT :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "AttachScript"; 
	}
	const char* Get_Help() 
	{ 
		return "ATTACHSCRIPT <clientId> <script> <parameters> - Attaches specified script to the player (parameters can be left out if desired)."; 
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
				const char* script = arguments.getString();
				if (!script || script[0] == '\0')
				{
					Console_Output("Please enter a script string to attach to the player.");
				}
				else
				{
					const char* params = arguments.getRemainingString();  // Vector3's are space seperated
					if (!params || params[0] == '\0')
					{
						Commands->Attach_Script(Get_GameObj(clientId), script, "");
						Console_Output("Script attached with no parameters.");
					}
					else
					{
						Commands->Attach_Script(Get_GameObj(clientId), script, params);
						Console_Output("Script attached with parameters.");
					}
				}	
			}
		}
	}
};



class CommandUNATTACHSCRIPT :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "UnAttachScript"; 
	}
	const char* Get_Help() 
	{ 
		return "UNATTACHSCRIPT <clientId> <script> - Removes the specified script from the player."; 
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
				const char* script = arguments.getString();
				if (!script || script[0] == '\0')
				{
					Console_Output("Please enter a script string to remove from the player.");
				}
				else
				{
					Remove_Script(Get_GameObj(clientId), script);
					Console_Output("Script has been removed from the player.");
				}	
			}
		}
	}
};


class CommandFREEZEPLAYER :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "FreezePlayer"; 
	}
	const char* Get_Help() 
	{ 
		return "FREEZEPLAYER <clientId> - Removes control of the character from the player."; 
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
				Commands->Control_Enable(Get_GameObj(clientId), false);
				Console_Output("Player has no control.");
			}
		}
	}
};



class CommandUNFREEZEPLAYER :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "UnFreezePlayer"; 
	}
	const char* Get_Help() 
	{ 
		return "UNFREEZEPLAYER <clientId> - Gives control of the character to the player."; 
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
				Commands->Control_Enable(Get_GameObj(clientId), true);
				Console_Output("Player has control.");
			}
		}
	}
};



class CommandLISTPLAYERSOBJECTIDS :
	public ConsoleFunctionClass
{
public:
	const char* Get_Name() 
	{ 
		return "ListPlayersObjectIDs"; 
	}
	const char* Get_Help() 
	{ 
		return "LISTPLAYERSOBJECTIDS - Sends a list of all the current players player ID's, with their object ID's to the log stream."; 
	}
	void Activate(const char* argumentsString)
	{
		SLNode<SoldierGameObj> *x = GameObjManager::StarGameObjList.Head();
		StringClass str;
		while (x)
		{
			GameObject *o = (GameObject *)x->Data();
			if (o && Commands->Is_A_Star(o))
			{
				StringClass Temp;
				Temp.Format("PlayerID:%i;GameObjectID:%i\n", Get_Player_ID(o), Commands->Get_ID(o));
				str.Format("%s%s", str, Temp);
			}
			x = x->Next();
		}
		SSGMGameLog::Log_Message(str,"_GAMEOBJECTIDS");
	}
};



class EXTRACONSOLECOMMANDS : public Plugin
{
public:
	EXTRACONSOLECOMMANDS()
	{
		ConsoleFunctionList.Add(new CommandTAKECREDITS);
		ConsoleFunctionList.Add(new CommandKILL);
		ConsoleFunctionList.Add(new CommandTAKEPOINTS);
		ConsoleFunctionList.Add(new CommandGIVEPOW);
		ConsoleFunctionList.Add(new CommandATTACHSCRIPT);
		ConsoleFunctionList.Add(new CommandUNATTACHSCRIPT);
		ConsoleFunctionList.Add(new CommandFREEZEPLAYER);
		ConsoleFunctionList.Add(new CommandUNFREEZEPLAYER);
		ConsoleFunctionList.Add(new CommandLISTPLAYERSOBJECTIDS);
		Sort_Function_List();
		Verbose_Help_File();
		Console_Output("Loading Extra Console Commands Plugin; written by Reborn from MP-Gaming.COM\n");
	}
	~EXTRACONSOLECOMMANDS()
	{
		Console_Output("Un-loading Extra Console Commands Plugin; written by Reborn from MP-Gaming.COM\n");
		Delete_Console_Function("TakeCredits");
		Delete_Console_Function("Kill");
		Delete_Console_Function("TakePoints");
		Delete_Console_Function("GivePow");
		Delete_Console_Function("AttachScript");
		Delete_Console_Function("UnAttachScript");
		Delete_Console_Function("FreezePlayer");
		Delete_Console_Function("UnFreezePlayer");
		Delete_Console_Function("ListPlayersObjectIDs");
	}
	
};

EXTRACONSOLECOMMANDS extraconsolecommands;


extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &extraconsolecommands;
}
