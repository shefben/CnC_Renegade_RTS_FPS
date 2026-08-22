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
#include "teamspeak.h"
#include "TCPclass.h"
#include "gmgame.h"

TCPSocket *Client;

class TS3 : public Plugin
{
public:
	StringClass TS3ServerIP;
	unsigned short TS3Port;
	StringClass Password;
	int ServerID;
	bool Messages;
	StringClass TS3URL;
	int Channel1;
	int Channel2;
	int LobbyChan;
	TS3()
	{
		Console_Output("Loading TS3 Regulator Plugin; written by Reborn from MP-Gaming.COM\n");
		RegisterEvent(EVENT_CHAT_HOOK,this);
		RegisterEvent(EVENT_GLOBAL_INI,this);
		RegisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		RegisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
		RegisterEvent(EVENT_PLAYER_LEAVE_HOOK,this);
	}
	~TS3()
	{
		UnregisterEvent(EVENT_CHAT_HOOK,this);
		UnregisterEvent(EVENT_GLOBAL_INI,this);
		UnregisterEvent(EVENT_LOAD_LEVEL_HOOK,this);
		UnregisterEvent(EVENT_OBJECT_CREATE_HOOK,this);
		UnregisterEvent(EVENT_PLAYER_LEAVE_HOOK,this);
	}

	virtual void OnLoadGlobalINISettings(INIClass *SSGMIni)
	{
		SSGMIni->Get_String(TS3ServerIP, "TS3", "TeamSpeakServerIP", "127.0.0.1");
		TS3Port = (unsigned short)SSGMIni->Get_Int("TS3", "TeamSpeakTCPport", 10011);
		SSGMIni->Get_String(Password, "TS3", "SApassword", "Hello123");
		ServerID = SSGMIni->Get_Int("TS3", "TeamSpeakServerID", 1);
		Messages = SSGMIni->Get_Bool("TS3","EnableExplanationMessages", true);
		SSGMIni->Get_String(TS3URL, "TS3", "TeamSpeakAddress", "ts3.MP-Gaming.com");
		Channel1 = SSGMIni->Get_Int("TS3", "Channel1", 2);
		Channel2 = SSGMIni->Get_Int("TS3", "Channel2", 3);
		LobbyChan = SSGMIni->Get_Int("TS3", "LobbyChannel", 1);
		Log_Into_TS_Server();
	}


	bool IP_Matches(int Clid, int ID)
	{
		bool Matches = false;
		/*
		StringClass Msg;
		char message [256];
		char rmessage [2048];

		sprintf(message,"clientinfo clid=%i\n", Clid);
		Client->SendData(message, strlen(message));
		Client->RecieveData(rmessage,256);

		DynamicVectorClass<StringClass> str_Vector;
		char * pch;
		pch = strtok (rmessage,"	 =\n");

		while (pch != NULL)
		{
			StringClass strData = pch;
			str_Vector.Add(strData);
			pch = strtok (NULL, "	 =\n");
		}
		*/
		  /*
		  const char *PIP = Get_IP_Address(ID);
		  if(strstr(str_Vector[IPpos].Peek_Buffer(), PIP))
		  {
			  Matches = true;
			  Msg.Format("IP for Clid %i matches the player in-game with the same name.\n", Clid);
			  Console_Output(Msg);
		  }
		  else
		  {
			  Matches = false;
			  Msg.Format("IP for Clid %i does not matches the player in-game with the same name.\n", Clid);
			  Console_Output(Msg);
		  }
		  */
		Matches = true; //While the function is not complete, this should be set to true.
		return Matches;
	}


	void Log_Into_TS_Server()
	{
		char message [256];
		char rmessage [256];

		Client = new TCPSocket();
		Client->Client(TS3ServerIP, TS3Port);

		Client->RecieveData(rmessage, 256);
		sprintf(message,"login serveradmin %s\n", Password.Peek_Buffer());
		Client->SendData(message, strlen(message));
		Client->RecieveData(rmessage, 256);
		sprintf(message,"use sid=%i\n", ServerID);
		Client->SendData(message, strlen(message));
		Client->RecieveData(rmessage,256);
		sprintf(message,"sendtextmessage targetmode=3 target=%i msg=The\\sTT(SSGM)\\sTeamspeak3\\schannel\\sregulator\\splug-in\\sis\\snow\\slogged\\sin!\n", ServerID);
		Client->SendData(message, strlen(message));
		Client->RecieveData(rmessage,256);
		sprintf(message,"sendtextmessage targetmode=3 target=%i msg=Created\\sby\\sreborn\\sfrom\\sMP-Gaming.COM\n", ServerID);
		Client->SendData(message, strlen(message));
		Client->RecieveData(rmessage,256);
	}


	int Get_Clid(int ID)
	{
		int Clid = 0;
		const char *Pname = Get_Player_Name_By_ID(ID);
		StringClass Msg;
		char message [512];
		sprintf(message,"clientfind pattern=%s\n",Pname);
		Client->SendData(message, strlen(message));
		Client->RecieveData(message, 512); 
		if(strstr(message, Pname))
		{
			DynamicVectorClass<StringClass> str_Vector;
			char * pch;
			pch = strtok (message,"	 =\n");
			while (pch != NULL)
			{
				StringClass strData = pch;
				str_Vector.Add(strData);
				pch = strtok (NULL, "	 =\n");
			}

			for(int i=0;i < str_Vector.Count(); i++)
			{
				StringClass strd = str_Vector[i];
				if(strcmp(strd.Peek_Buffer(), Pname) == 0)
				{
					Clid = atoi(str_Vector[i - 2].Peek_Buffer());
				}
			}
		}
		else
		{
			Msg.Format("Player %s isn't connected to our TeamSpeak server\n",Pname);
			Console_Output(Msg);
		}
		delete[] Pname;
		return Clid;
	}


	void Move_Player(int ID)
	{
		StringClass Msg;
		char message [512];

		int clid = Get_Clid(ID);
		if(clid != -1)
		{
			if(IP_Matches(clid, ID)) //Function needs to be written, but probably over-kill tbh.
			{
				int team = Get_Team(ID);
				if(team == 0)
				{
					sprintf(message,"clientmove clid=%i cid=%i\n",clid,Channel2);
					Client->SendData(message, strlen(message));
					Client->RecieveData(message, 512); 
				}
				else if(team == 1)
				{
					sprintf(message,"clientmove clid=%i cid=%i\n",clid,Channel1);
					Client->SendData(message, strlen(message));
					Client->RecieveData(message, 512); 
				}
			}
		}
		else
		{
			Msg.Format("There was a problem getting the client ID from TeamSpeak.\n");
			Console_Output(Msg);
		}
	}


	void Remove_From_Channels(int ID)
	{
		StringClass Msg;
		char message [512];

		int clid = Get_Clid(ID);
		if(clid != -1)
		{
			if(IP_Matches(clid, ID)) //Function needs to be written, but probably over-kill tbh.
			{
				sprintf(message,"clientmove clid=%i cid=%i\n",clid,LobbyChan);
				Client->SendData(message, strlen(message));
				Client->RecieveData(message, 512); 
			}
		}
		else
		{
			Msg.Format("There was a problem getting the client ID from TeamSpeak.\n");
			Console_Output(Msg);
		}
	}



	virtual void OnObjectCreate(void *data,GameObject *obj)
	{
		if(Commands->Is_A_Star(obj))
		{
			Move_Player(Get_Player_ID(obj));
		}
	}

	virtual void OnLoadLevel()
	{
		GameObject *thing = Commands->Create_Object("Invisible_Object",Vector3(0.0f,0.0f,0.0f));
		Commands->Attach_Script(thing,"ping","");
		if(Messages == true)
		{
			Commands->Attach_Script(thing,"tsr_explanation","");
		}
	}

	virtual void OnPlayerLeave(int PlayerID)
	{
		Remove_From_Channels(PlayerID);
	}

	virtual bool OnChat(int PlayerID,TextMessageEnum Type,const wchar_t *Message,int recieverID)
	{
		if (Message[0] == L'!')
		{
			if ((wcsistr(Message,L"!ts") == Message) || (wcsistr(Message,L"!teamspeak") == Message))
			{
				StringClass Msg;
				Msg.Format("msg This server hosts a TeamSpeak3 server for you to use while playing. The hostname for the server is \"%s\".", TS3URL);
				Console_Input(Msg);

				char message [4096];
				sprintf(message,"clientlist\n");
				Client->SendData(message, strlen(message));
				Client->RecieveData(message, 4096); 

				Msg.Format("The following people are active on the TeamSpeak3 server:");

				DynamicVectorClass<StringClass> str_Vector;
				char * pch;
				pch = strtok (message,"	 =\n");
				while (pch != NULL)
				{
					StringClass strData = pch;
					str_Vector.Add(strData);
					pch = strtok (NULL, "	 =\n");
				}
				for(int i=0;i < str_Vector.Count(); i++)
				{
					StringClass strd = str_Vector[i];
					if(strcmp(strd.Peek_Buffer(), "client_nickname") == 0)
					{
						if(!strstr(str_Vector[i + 1].Peek_Buffer(), "serveradmin"))
						{
							Msg.Format("%s %s",Msg,str_Vector[i + 1].Peek_Buffer());
						}
					}
				}
				Msg.Format("msg %s", Msg);
				Console_Input(Msg);
				
				return false;
			}
		}
		return true;
	}

};

TS3 ts3;


void tsr_explanation::Created(GameObject *obj)
{
	Commands->Start_Timer(obj,this,400.0f,1);
	Commands->Start_Timer(obj,this,410.0f,2);
	Commands->Start_Timer(obj,this,420.0f,3);
}

void tsr_explanation::Timer_Expired(GameObject *obj,int number)
{
	StringClass Message;
	if(number == 1)
	{
		if(Get_Player_Count() > 0)
		{
			Message.Format("msg This server is running a TeamSpeak3 Server, the address is %s.", ts3.TS3URL);
			Console_Input(Message);
		}
	Commands->Start_Timer(obj,this,400.0f,1);
	}
	else if(number == 2)
	{
		if(Get_Player_Count() > 0)
		{
			Message.Format("msg The server is running reborn's TeamSpeak3 Regulator plug-in.");
			Console_Input(Message);
		}
		Commands->Start_Timer(obj,this,410.0f,2);
	}
	else if(number == 3)
	{
		if(Get_Player_Count() > 0)
		{
			Message.Format("msg If you join Teamspeak3 with the same in-game player name, you will be auto-teamed to the right channels, always...");
			Console_Input(Message);
		}
		Commands->Start_Timer(obj,this,420.0f,3);
	}
}


ScriptRegistrant<tsr_explanation> tsr_explanation_Registrant("tsr_explanation","");



void ping::Created(GameObject *obj){
	Commands->Start_Timer(obj,this,300.0f,1);
}

void ping::Timer_Expired(GameObject *obj,int number)
{
	if(number == 1)
	{
		char message [64];
		sprintf(message,"\n");
		Client->SendData(message, strlen(message));
		Client->RecieveData(message, 64); 
		Commands->Start_Timer(obj,this,300.0f,1);
	}
}


ScriptRegistrant<ping> ping_Registrant("ping","");



extern "C" __declspec(dllexport) Plugin* Plugin_Init()
{
	return &ts3;
}
