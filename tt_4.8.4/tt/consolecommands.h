/*	Renegade tt.dll
	Console commands header file
	Copyright 2009 Jonathan Wilson

	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#ifndef __consolecommands_h__
#define __consolecommands_h__

class ConsoleFunctionClass {
	public:
		virtual char *Get_Name(void)		{ return NULL; }
		virtual char *Get_Alias(void)		{ return NULL; }
		virtual char *Get_Help(void)		{ return NULL; }
		virtual void Activate(char *)		{ return; }
		virtual ~ConsoleFunctionClass()	{ return; }
};

class CommandID : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "id"; }
		char *Get_Help(void)	{ return "ID <string> - Prints the name and ID of all players matching <string>. No string means all players."; }
		void Activate(char *);
};

class CommandPAMSG : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "pamsg"; }
		char *Get_Help(void)	{ return "PAMSG <player> <message> - Sends an admin message to a specific player. Host only."; }
		void Activate(char *);
};

class CommandSNDP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "sndp"; }
		char *Get_Help(void)	{ return "SNDP <player> <wav file> - Plays a WAV file for a specific player. Host only."; }
		void Activate(char *);
};

class CommandSNDT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "sndt"; }
		char *Get_Help(void)	{ return "SNDT <team> <wav file> - Plays a WAV file for a specific team. Host only."; }
		void Activate(char *);
};

class CommandSNDA : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "snda"; }
		char *Get_Help(void)	{ return "SNDA <wav file> - Plays a WAV file for all players. Host only."; }
		void Activate(char *);
};

class CommandMUSICP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "musicp"; }
		char *Get_Help(void)	{ return "MUSICP <player> <mp3 file> - Plays a MP3 file for a specific player. Host only."; }
		void Activate(char *);
};

class CommandMUSICA : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "musica"; }
		char *Get_Help(void)	{ return "MUSICA <mp3 file> - Plays a MP3 file for all players. Host only."; }
		void Activate(char *);
};

class CommandNOMUSICP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "nomusicp"; }
		char *Get_Help(void)	{ return "NOMUSICP <player> - Stops the background music for a specific player. Host only."; }
		void Activate(char *);
};

class CommandNOMUSICA : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "nomusica"; }
		char *Get_Help(void)	{ return "NOMUSICA - Stops the background music for all players. Host only."; }
		void Activate(char *);
};

class CommandSND3DP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "snd3dp"; }
		char *Get_Help(void)	{ return "SND3DP <player> <wav file> - Plays a 3D WAV file for a specific player. Host only."; }
		void Activate(char *);
};

class CommandSND3DA : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "snd3da"; }
		char *Get_Help(void)	{ return "SND3DA <player> <wav file> - Plays a 3D WAV file for all players. The player is used to identify where to play the 3d sound. Host only."; }
		void Activate(char *);
};

class CommandPPAGE : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "ppage"; }
		char *Get_Help(void)	{ return "PPAGE <player> <message> - Sends a page to a specific player. Host only."; }
		void Activate(char *);
};

class CommandTPAGE : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "tpage"; }
		char *Get_Help(void)	{ return "TPAGE <team> <message> - Sends a page to a specific team. Host only.\n0 = Nod\n1 = GDI"; }
		void Activate(char *);
};

class CommandMSG : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "message"; }
		char *Get_Alias(void)	{ return "msg"; }
		char *Get_Help(void)	{ return "MESSAGE <message> - sends a chat message to all clients. Host only."; }
		void Activate(char *);
};

class CommandTEAM : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "team"; }
		char *Get_Help(void)	{ return "TEAM <player> <team> - Changes a players team. Host only.\n0 = Nod\n1 = GDI"; }
		void Activate(char *);
};

class CommandTEAM2 : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "team2"; }
		char *Get_Help(void)	{ return "TEAM2 <player> <team> - Changes a players team without taking cash/score Host only.\n0 = Nod\n1 = GDI"; }
		void Activate(char *);
};

class CommandDONATE : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "donate"; }
		char *Get_Help(void)	{ return "DONATE <from player> <to player> <amount> - Move cash from one player to another on the same team."; }
		void Activate(char *);
};

class CommandEXIT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "exit"; }
		char *Get_Help(void)	{ return "EXIT - Exits renegade. Renegade Client only."; }
		void Activate(char *);
};

class CommandMLIMIT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mlimit"; }
		char *Get_Help(void)	{ return "MLIMIT <new limit> - Sets the mine limit. Limit of 127. Renegade host only."; }
		void Activate(char *);
};

class CommandVERSION : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "version"; }
		char *Get_Help(void)	{ return "VERSION <player> - Get the version of tt.dll installed on a client. Host Only."; }
		void Activate(char *);
};

class CommandSVERSION : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "sversion"; }
		char *Get_Help(void)	{ return "SVERSION - Get the version of tt.dll installed on this machine."; }
		void Activate(char *);
};

class CommandRADAR : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "radar"; }
		char *Get_Help(void)	{ return "RADAR - Get the radar mode for the server. Host only."; }
		void Activate(char *);
};

class CommandMLIST : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mlist"; }
		char *Get_Help(void)	{ return "MLIST <position> - Print the name of the map in the given position in the map list. Numbers are from 0 to 99. Host only."; }
		void Activate(char *);
};

class CommandMLISTC : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mlistc"; }
		char *Get_Help(void)	{ return "MLISTC <position> <map> - Change the map at <position> in the map list. Numbers are from 0 to 99. Host only."; }
		void Activate(char *);
};

class CommandMAP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "map"; }
		char *Get_Help(void)	{ return "MAP - Print the name of the current map. Host only."; }
		void Activate(char *);
};

class CommandMOD : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mod"; }
		char *Get_Help(void)	{ return "MOD - Print the name of the current mod. Host only."; }
		void Activate(char *);
};

class CommandMAPNUM : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mapnum"; }
		char *Get_Help(void)	{ return "MAPNUM - Print the index within the map list of the current map. Host only."; }
		void Activate(char *);
};

class CommandMLIMITD : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mlimitd"; }
		char *Get_Help(void)	{ return "MLIMITD - Print the current mine limit. Host only."; }
		void Activate(char *);
};

class CommandMINED : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mined"; }
		char *Get_Help(void)	{ return "MINED <team> - Print the current mine count for <team>. Host only.\n0 = Nod\n1 = GDI"; }
		void Activate(char *);
};

class CommandEJECT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "eject"; }
		char *Get_Help(void)	{ return "EJECT <player> - Ejects that player from whatever vehicle they are in, if any"; }
		void Activate(char *);
};

class CommandSND3DT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "snd3dt"; }
		char *Get_Help(void)	{ return "SND3DT <player> <team> <wav file> - Plays a 3D sound for a given team at the location of <player>. Host only.\n0 = Nod\n1 = GDI"; }
		void Activate(char *);
};

class CommandICON : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "icon"; }
		char *Get_Help(void)	{ return "ICON <player> <w3d file> - Shows an emoticon over the head of the passed in player that is visible to their team. Host only."; }
		void Activate(char *);
};

class CommandSONG : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "song"; }
		char *Get_Help(void)	{ return "SONG - Prints the name of the last song loaded by the MUSIC command or the Set_Background_Music script command. Host only."; }
		void Activate(char *);
};

class CommandWIN : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "win"; }
		char *Get_Help(void)	{ return "WIN <team> - Kills the buildings of the other team to end the game. Host only.\n0 = Nod\n1 = GDI"; }
		void Activate(char *);
};

class CommandLOG : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "log"; }
		char *Get_Help(void)	{ return "LOG <number> - enables or disables the client chatlog, 0 = disabled, 1 = enabled"; }
		void Activate(char *);
};

class CommandLOGP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "logp"; }
		char *Get_Help(void)	{ return "LOGP - prints the status of the client chatlog"; }
		void Activate(char *);
};

class CommandVIEW : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "view"; }
		char *Get_Help(void)	{ return "VIEW <w3d filename> <animation name> - displays a w3d file in a dialog. Use to examine models for test purposes."; }
		void Activate(char *);
};

class CommandHUD : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "hud"; }
		char *Get_Help(void)	{ return "HUD - toggle HUD display"; }
		void Activate(char *);
};

class CommandTMSG : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "tmsg"; }
		char *Get_Help(void)	{ return "TMSG <player> <message> - Sends a message to a team as though it was comming from <player>."; }
		void Activate(char *);
};

class CommandGETBW : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "getbw"; }
		char *Get_Help(void)	{ return "GETBW <player> - retrives the current bandwidth for <player> (as set by sbbo)"; }
		void Activate(char *);
};

class CommandSETBW : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "setbw"; }
		char *Get_Help(void)	{ return "SETBW <player> <bandwidth> - sets the current bandwidth for <player> (as set by sbbo)"; }
		void Activate(char *);
};

class CommandPINFO : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "pinfo"; }
		char *Get_Help(void)	{ return "PINFO - print information about the players in the game"; }
		void Activate(char *);
};

class CommandICON2 : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "icon2"; }
		char *Get_Help(void)	{ return "ICON2 <player> <w3d file> - Shows an emoticon over the head of the passed in player that is visible to their enemies. Host only."; }
		void Activate(char *);
};

class CommandVLIMIT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "vlimit"; }
		char *Get_Help(void)	{ return "VLIMIT <limit> - Changes the current vehicle limit. Host only."; }
		void Activate(char *);
};

class CommandVLIMITD : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "vlimitd"; }
		char *Get_Help(void)	{ return "VLIMITD - Displays the current vehicle limit. Host only."; }
		void Activate(char *);
};

class CommandCMSG : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "cmsg"; }
		char *Get_Help(void)	{ return "CMSG <red>,<green>,<blue> <message> - Displays a colored message in the info box of all players. Host only."; }
		void Activate(char *);
};

class CommandCMSGP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "cmsgp"; }
		char *Get_Help(void)	{ return "CMSGP <player> <red>,<green>,<blue> <message> - Displays a colored message in the info box of a player. Host only."; }
		void Activate(char *);
};

class CommandCMSGT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "cmsgt"; }
		char *Get_Help(void)	{ return "CMSGT <team> <red>,<green>,<blue> <message> - Displays a colored message in the info box of a team. Host only."; }
		void Activate(char *);
};

class CommandDISARM : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "disarm"; }
		char *Get_Help(void)	{ return "DISARM <player> - Disarms all C4 of a player. Host only."; }
		void Activate(char *);
};

class CommandDISARMP : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "disarmp"; }
		char *Get_Help(void)	{ return "DISARMP <player> - Disarms all proximity C4 of a player. Host only."; }
		void Activate(char *);
};

class CommandDISARMB : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "disarmb"; }
		char *Get_Help(void)	{ return "DISARMB <player> - Disarms all beacons of a player. Host only."; }
		void Activate(char *);
};

class CommandRLMON : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "rlmon"; }
		char *Get_Help(void)	{ return "RLMON <1.2.3.4:1234> - Sets the current renlog mointor. Dedicated Servers only"; }
		void Activate(char *);
};

class CommandRLMONOFF : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "rlmonoff"; }
		char *Get_Help(void)	{ return "RLMONOFF - Clears the current renlog mointor. Dedicated Servers only"; }
		void Activate(char *);
};

class CommandPLIMIT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "plimit"; }
		char *Get_Help(void)	{ return "PLIMIT <new limit> - Changes the player limit"; }
		void Activate(char *);
};

class CommandPLIMITD : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "plimitd"; }
		char *Get_Help(void)	{ return "PLIMITD - Displays the current player limit"; }
		void Activate(char *);
};

class CommandTIME : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "time"; }
		char *Get_Help(void)	{ return "TIME <new time> - Changes the time remaining"; }
		void Activate(char *);
};

class CommandTIMED : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "timed"; }
		char *Get_Help(void)	{ return "TIMED - Displays the time remaining"; }
		void Activate(char *);
};

class CommandTIMEL : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "timel"; }
		char *Get_Help(void)	{ return "TIMEL <new limit> - Changes the time limit"; }
		void Activate(char *);
};

class CommandTIMELD : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "timeld"; }
		char *Get_Help(void)	{ return "TIMELD - Displays the time limit"; }
		void Activate(char *);
};

class CommandMAPCH : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "mapch"; }
		char *Get_Help(void)	{ return "MAPCH <player> <map> - Check if a given client has a given map. Host Only."; }
		void Activate(char *);
};

class CommandSSURL : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "ssurl"; }
		char *Get_Help(void)	{ return "SSURL <url> - Set the URL to be used for remote screenshots. Host Only."; }
		void Activate(char *);
};

class CommandSSHOT : public ConsoleFunctionClass {
	public:
		char *Get_Name(void)	{ return "sshot"; }
		char *Get_Help(void)	{ return "SSHOT <player> - Take a remote screenshot of a player. Host only."; }
		void Activate(char *);
};



class CommandTAG :
	public ConsoleFunctionClass
{

public:

	char* Get_Name(void) { return "tag"; }
	char* Get_Help(void) { return "TAG <playerid> <tag> - Sets the custom name tag for a player. Host only."; }
	void Activate(char* arguments);

};



class CommandSERIAL :
	public ConsoleFunctionClass
{

public:

	char* Get_Name(void) { return "serial"; }
	char* Get_Help(void) { return "serial <playerid> - Gets the serial hash for a player. Host only."; }
	void Activate(char* arguments);

};



class CommandKICK2 :
	public ConsoleFunctionClass
{

public:

	char* Get_Name() { return "kick2"; }
	char* Get_Help() { return "kick2 <playerid> - Kick a user from the game. Host only"; }
	void Activate(char* arguments);

};


void InitConsole(void);

#endif
