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
#include "Swap.h"
#include "gmgame.h"
#include "Iterator.h"

#define GDI 1
#define NOD 0
#define GREEN "104,234,40"

bool IsRTC = false;
int idrtc = 0;

void PPage(int ID, const char* rgb_colour, const char *Format, ...)
{
	if(ID < 1 || ID > 128)
	{
		return;
	}

	if (!Get_GameObj(ID))
	{
		return;
	}

	char buffer[256];
	va_list va;
	_crt_va_start(va, Format);
	vsnprintf(buffer, 256, Format, va);
	va_end(va);

	float Version = Get_Client_Version(ID);

	if(Version < 2.9)
	{
		Console("ppage %d %s",ID, buffer);
                return;
	}
	else
	{
		Console("cmsgp %d %s %s", ID, rgb_colour, buffer);
	}
}

void Console(const char *Format, ...)
{
	char buffer[256];
	va_list va;
	_crt_va_start(va, Format);
	vsnprintf(buffer, 256, Format, va);
	va_end(va);
	Console_Input(buffer);
}

DynamicVectorClass<StringClass> SwappedPlayers;  

bool SwapCheck(int ID) 
{
	const char *str = Get_Player_Name_By_ID(ID);
	for (int i = 0; i < SwappedPlayers.Count(); i++) 
	{
		if (SwappedPlayers[i] == str) 
		{
			delete[] str;
			return true;
		}
	}
	delete[] str;
	return false;
}

void SwapAddPlayer(int ID) 
{
	if (SwapCheck(ID) == false) 
	{
		const char *str = Get_Player_Name_By_ID(ID);
		SwappedPlayers.Add(str);
		delete[] str;
	}
}

void SwapClearPlayers() 
{
	SwappedPlayers.Delete_All();
}



class SWAP : public Plugin
{
public:
	int SwapGameTimeLimit;
	int SwapExpireLimit;
	StringClass PPageColour;
	DynamicVectorClass<StringClass> PresetNames;

	SWAP()
	{
		Console_Output("Loading Swap Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_CHAT_HOOK,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_GLOBAL_INI,this);
	}
	~SWAP()
	{
		PresetNames.Delete_All();
		Console_Output("Un-loading Swap Plugin; written by Reborn from MP-Gaming.COM\n");
		UnregisterEvent(EVENT_CHAT_HOOK,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_GLOBAL_INI,this);
	}


	virtual bool OnChat(int PlayerID,TextMessageEnum Type,const wchar_t *Message,int recieverID)
	{
		if (Message[0] == L'!')
		{
			if ((wcsistr(Message,L"!swap") == Message) || (wcsistr(Message,L"!rtc") == Message))
			{ 
				for(int i = 0; i < PresetNames.Count(); i++)
				{
					if((strcmp(Commands->Get_Preset_Name(Get_GameObj(PlayerID)),PresetNames[i]))== 0)
					{
						PPage(PlayerID, PPageColour, "Unfortunately this preset type has been disallowed from using the command.");
						return false;
					}
				}
				if (SwapCheck(PlayerID) == false) 
				{
					RequestTeamChange(PlayerID);	
				}
				else 
				{
					PPage(PlayerID, PPageColour, "You have already swapped once. There is a limit of one successful swap per map, per player.");
				}
				return false;
			}
		}
		return true;
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni) 
	{
		SwapGameTimeLimit = SSGMIni->Get_Int("Swap", "SwapGameTimeLimit", 300);
		SwapExpireLimit = SSGMIni->Get_Int("Swap", "SwapExpireLimit", 60);
		SSGMIni->Get_String(PPageColour,"Swap", "PPageColour", GREEN);
		int count = SSGMIni->Entry_Count("ListOfPresets");
		for (int i = 0; i < count; i++)
		{
			const char* entry_name = SSGMIni->Get_Entry("ListOfPresets", i);
			StringClass entry_string; 
			SSGMIni->Get_String(entry_string, "ListOfPresets", entry_name, "42");
			PresetNames.Add(entry_string.Peek_Buffer());
		}
	}

	virtual void OnLoadLevel() 
	{
		SwapClearPlayers();
		idrtc = 0;
		IsRTC = false;
	}

	
};

SWAP swap;


void RequestTeamChange(int PlayerID) 
{
	GameObject *Player = Get_GameObj(PlayerID);
	StringClass Msg;

	if (The_Game()->Get_Game_Duration_S() < (unsigned)swap.SwapGameTimeLimit)
	{
		int NodPlayers = Get_Team_Player_Count(NOD);
		int GDIPlayers = Get_Team_Player_Count(GDI);

		if (abs(GDIPlayers-NodPlayers) > 1)
		{
			int Team = Get_Team(PlayerID);
			StringClass Msg3, Msg2;
			const char *str = Get_Team_Name(Get_Team(idrtc) ? GDI : NOD);
			const char *str2 = Get_Player_Name(Get_GameObj(PlayerID));
			Msg2.Format("msg %s has joined forces with the %s team.",str2,str);
			delete[] str;
			delete[] str2;
			if ((GDIPlayers > NodPlayers) && (Team == GDI))
			{
				Msg3.Format("team2 %d %i",PlayerID, NOD);
				Console_Input(Msg3);
				Console_Input(Msg2);
				return;
			}
			else if ((NodPlayers > GDIPlayers) && (Team == NOD))
			{
				Msg3.Format("team2 %d %i",PlayerID, GDI);
				Console_Input(Msg3);
				Console_Input(Msg2);
				return;
			}
		}
		if (!IsRTC) 
		{
			if (GDIPlayers > 0 && NodPlayers > 0)
			{
				GameObject *rtccontroller;
				rtccontroller = Commands->Create_Object("SignalFlare_Gold_Phys3",Vector3(0.0f,0.0f,20.0f));
				Commands->Set_Model(rtccontroller, "null");
				idrtc = PlayerID;
				Commands->Attach_Script(rtccontroller,"RTC_timer","");
				const char *str = Get_Team_Name(0);
				const char *str2 = Get_Team_Name(1);
				const char *str3 = Get_Player_Name(Get_GameObj(PlayerID));
				Msg.Format("msg %s on the %s team wants to swap with someone on the %s team. Type \"!rtc\" or \"!swap\" to change with them. There are %i seconds remaining before this expires.", str3, str, str2, swap.SwapExpireLimit);
				Console_Input(Msg);
				delete[] str;
				delete[] str2;
				delete[] str3;
				IsRTC = true;
			}
			else 
			{
				PPage(PlayerID, swap.PPageColour, "Unfortunately there are not enough people in the server for you to swap teams.");
			}
		}
		else if (IsRTC)
		{
			GameObject *obj = Get_GameObj(idrtc);
			if(!obj)
			{
				GameObject *rtccontroller;
				rtccontroller = Commands->Create_Object("SignalFlare_Gold_Phys3",Vector3(0.0f,0.0f,20.0f));
				Commands->Set_Model(rtccontroller, "null");
				idrtc = PlayerID;
				Commands->Attach_Script(rtccontroller,"RTC_timer","");
				const char *str = Get_Team_Name(0);
				const char *str2 = Get_Team_Name(1);
				const char *str3 = Get_Player_Name(Get_GameObj(PlayerID));
				Msg.Format("msg %s on the %s team wants to swap with someone on the %s team. Type \"!rtc\" or \"!swap\" to change with them. There are %i seconds remaining before this expires.",str3,str,str2, swap.SwapExpireLimit);
				Console_Input(Msg);
				delete[] str;
				delete[] str2;
				delete[] str3;
			}
			else 
			{
				if(Get_Team(PlayerID) != Get_Team(idrtc))
				{
						Msg.Format("team2 %d %i",PlayerID,Get_Team(idrtc));
						Console_Input(Msg);
						Msg.Format("team2 %d %i",idrtc,Commands->Get_Player_Type(Player));
						Console_Input(Msg);
						SwapAddPlayer(idrtc);
						const char *str = Get_Player_Name_By_ID(PlayerID);
						const char *str2 = Get_Player_Name_By_ID(idrtc);
						Msg.Format("msg player %s and player %s have changed teams!",str,str2);
						delete[] str;
						delete[] str2;
						Console_Input(Msg);
						GameObject *timerthing = Find_Object_With_Script("RTC_timer");
						Remove_Script(timerthing,"RTC_timer");
						Commands->Destroy_Object(timerthing);
						IsRTC = false;
						idrtc = 0;
						SwapAddPlayer(PlayerID);
				}

				else if (Get_Team(PlayerID) == Get_Team(idrtc))
				{
					const char *str = Get_Player_Name_By_ID(idrtc);
					PPage(PlayerID, swap.PPageColour, "You're on the same team, you cannot swap with %s.", str);
					delete[] str;
				}
			}
		}
	}
	else 
	{
		PPage(PlayerID, swap.PPageColour, "Unfortunately %i seconds has passed into the game. No swapping is allowed after %i seconds of map time.", The_Game()->Get_Game_Duration_S(), swap.SwapGameTimeLimit);
	}
}


void RTC_timer::Created(GameObject *obj)
{
	Commands->Start_Timer(obj, this,(float)swap.SwapExpireLimit, 1);
}


void RTC_timer::Timer_Expired(GameObject *obj, int number)
{
	if(number == 1)
	{
		IsRTC = false;
		GameObject *obj2 = Get_GameObj(idrtc);
		if(!obj2)
		{
			idrtc = 0;
			Commands->Destroy_Object(obj);
			Destroy_Script();
		}
		else
		{
			const char *str = Get_Team_Name(Get_Team(idrtc) ? GDI : NOD);
			StringClass Msg;			
			const char *str2 = Get_Player_Name_By_ID(idrtc);
			Msg.Format("msg %s's request to change to team %s has expired.",str2, str);
			delete[] str2;
			Console_Input(Msg);
			PPage(idrtc, swap.PPageColour, "Your request to change to team %s has timed out.", str);
			delete[] str;
			idrtc = 0;
			Commands->Destroy_Object(obj);
			Destroy_Script();
		}
	}
}

ScriptRegistrant<RTC_timer> RTC_timer_Registrant("RTC_timer","");

extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &swap;
}
