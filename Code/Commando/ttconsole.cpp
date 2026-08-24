/******************************************************************************
*
* FILE
*     ttconsole.cpp
*
* DESCRIPTION
*     See ttconsole.h.
*
*     4.8.4 added these by overwriting the engine's console function list from
*     a DLL.  Here they are ordinary ConsoleFunctionClass objects added to the
*     same list every other command lives in.  What was a private-message
*     channel carrying a smuggled opcode is, in each case, the script command
*     that already does the thing: a console command and a level script asking
*     for the same effect now travel the same wire.
*
******************************************************************************/

#include "ttconsole.h"

#include "colors.h"
#include "combat.h"
#include "consolefunction.h"
#include "beacongameobj.h"
#include "c4.h"
#include "cnetwork.h"
#include "gamedata.h"
#include "gameobjmanager.h"
#include "player.h"
#include "playermanager.h"
#include "playertype.h"
#include "remotescreenshot.h"
#include "scmapqueryevent.h"
#include "scscreenshotrequestevent.h"
#include "scriptcommands.h"
#include "sctextobj.h"
#include "smartgameobj.h"
#include "soldier.h"
#include "ssgmsettings.h"
#include "vehiclefactorygameobj.h"
#include "weaponmanager.h"
#include "wwstring.h"

#include <stdlib.h>
#include <string.h>


/*
**	A console command that changes the game is the server's to run, and only
**	while there is a multiplayer game to change.
*/
static bool	Server_Command (void)
{
	return CombatManager::I_Am_Server () && (The_Game () != nullptr);
}


/*
**	"12 the rest of the line" -> 12 and "the rest of the line".  Returns false
**	when there is no second half, which is what every one of these commands
**	treats as bad input.
*/
static bool	Split (const char *input, StringClass &head, StringClass &tail)
{
	if (input == nullptr) {
		return false;
	}

	const char *space = strchr (input, ' ');
	if (space == nullptr) {
		head = input;
		tail = "";
		return false;
	}

	head = StringClass (input);
	head.Erase ((int)(space - input), head.Get_Length () - (int)(space - input));
	tail = space + 1;

	return tail.Get_Length () > 0;
}


static cPlayer *	Find_Player (int player_id)
{
	if (player_id <= 0) {
		return nullptr;
	}

	cPlayer *player = cPlayerManager::Find_Player (player_id);
	if ((player == nullptr) || !player->Is_Active ()) {
		return nullptr;
	}

	return player;
}


static GameObject *	Player_Object (int player_id)
{
	if (Find_Player (player_id) == nullptr) {
		return nullptr;
	}

	return ScriptEngine::Find_Object_By_Player_ID (player_id);
}


static StringClass	Player_Name (cPlayer *player)
{
	StringClass name;
	if (player != nullptr) {
		player->Get_Name ().Convert_To (name);
	}
	return name;
}


/*
**	A colour is either three numbers or the name of one the game already has.
**	"player" and "team" only mean something to the commands that know which
**	player or team is being addressed, so they pass one in.
*/
static void	Parse_Color (const char *spec, int team, int player_id, int &red, int &green, int &blue)
{
	Vector3 color (0, 0, 0);
	bool	named	= true;

	if (spec == nullptr) {
		spec = "";
	}

	if (_stricmp (spec, "private") == 0) {
		color = Get_Private_Text_Color ();
	} else if (_stricmp (spec, "public") == 0) {
		color = Get_Public_Text_Color ();
	} else if (_stricmp (spec, "gdi") == 0) {
		color = Get_Color_For_Team (PLAYERTYPE_GDI);
	} else if (_stricmp (spec, "nod") == 0) {
		color = Get_Color_For_Team (PLAYERTYPE_NOD);
	} else if (_stricmp (spec, "team") == 0) {
		color = Get_Color_For_Team ((team == PLAYERTYPE_GDI) ? PLAYERTYPE_GDI : PLAYERTYPE_NOD);
	} else if (_stricmp (spec, "player") == 0) {
		int type = ScriptEngine::Get_Player_Type_By_ID (player_id);
		color = Get_Color_For_Team ((type == PLAYERTYPE_GDI) ? PLAYERTYPE_GDI : PLAYERTYPE_NOD);
	} else {
		named = false;
	}

	if (named) {
		red	= (int)(color.X * 255.0f);
		green	= (int)(color.Y * 255.0f);
		blue	= (int)(color.Z * 255.0f);
		return ;
	}

	red = green = blue = 0;
	sscanf (spec, "%d,%d,%d", &red, &green, &blue);
	return ;
}


/*
**	Everyone playing, or everyone on one team.  Team 2 means both, which is how
**	4.8.4 spelled "all" in a command that otherwise takes a team.
*/
typedef void	(*PlayerVisitor) (GameObject *player_object, int player_id, void *context);

static void	For_Each_Player (int team, PlayerVisitor visitor, void *context)
{
	SList<cPlayer> *list = cPlayerManager::Get_Player_Object_List ();
	if (list == nullptr) {
		return ;
	}

	for (SLNode<cPlayer> *node = list->Head (); node != nullptr; node = node->Next ()) {

		cPlayer *player = node->Data ();
		if ((player == nullptr) || !player->Is_Active ()) {
			continue;
		}

		if ((team != 2) && (player->Get_Player_Type () != team)) {
			continue;
		}

		GameObject *object = ScriptEngine::Find_Object_By_Player_ID (player->Get_Id ());
		if (object == nullptr) {
			continue;
		}

		visitor (object, player->Get_Id (), context);
	}

	return ;
}


/*
**	The C4 a player left behind.  4.8.4 kept its own list; here the one game
**	object list is walked and each charge asked who owns it.
*/
static void	Defuse_Player_C4 (int player_id, bool timed, bool proximity, bool remote)
{
	SLNode<BaseGameObj> *node = GameObjManager::Get_Game_Obj_List ()->Head ();
	SLNode<BaseGameObj> *next = nullptr;

	for (; node != nullptr; node = next) {

		next = node->Next ();

		PhysicalGameObj *phys = (node->Data () != nullptr) ? node->Data ()->As_PhysicalGameObj () : nullptr;
		C4GameObj *		c4		= (phys != nullptr) ? phys->As_C4GameObj () : nullptr;
		if (c4 == nullptr) {
			continue;
		}

		if (ScriptEngine::Get_Player_ID (c4->Get_Owner ()) != player_id) {
			continue;
		}

		const AmmoDefinitionClass *ammo = c4->Peek_Ammo_Definition ();
		if (ammo == nullptr) {
			continue;
		}

		bool wanted = false;
		switch ((int)ammo->AmmoType) {
			case AmmoDefinitionClass::AMMO_TYPE_C4_TIMED:		wanted = timed;		break;
			case AmmoDefinitionClass::AMMO_TYPE_C4_PROXIMITY:	wanted = proximity;	break;
			case AmmoDefinitionClass::AMMO_TYPE_C4_REMOTE:		wanted = remote;		break;
			default:														wanted = false;		break;
		}

		if (wanted) {
			c4->Defuse ();
		}
	}

	return ;
}


static int	Count_Team_Mines (int team)
{
	int count = 0;

	for (SLNode<BaseGameObj> *node = GameObjManager::Get_Game_Obj_List ()->Head ();
			node != nullptr; node = node->Next ()) {

		PhysicalGameObj *phys = (node->Data () != nullptr) ? node->Data ()->As_PhysicalGameObj () : nullptr;
		C4GameObj *		c4		= (phys != nullptr) ? phys->As_C4GameObj () : nullptr;
		if ((c4 == nullptr) || (c4->Get_Player_Type () != team)) {
			continue;
		}

		const AmmoDefinitionClass *ammo = c4->Peek_Ammo_Definition ();
		if ((ammo != nullptr) && ((int)ammo->AmmoType != (int)AmmoDefinitionClass::AMMO_TYPE_C4_TIMED)) {
			count ++;
		}
	}

	return count;
}


//-----------------------------------------------------------------------------
//	Who is playing
//-----------------------------------------------------------------------------

class	IdConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "id"; }
	const char *	Get_Help (void) override
	{ return "ID <string> - Prints the name and ID of all players matching <string>. No string means all players."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		SList<cPlayer> *list = cPlayerManager::Get_Player_Object_List ();
		for (SLNode<cPlayer> *node = list->Head (); node != nullptr; node = node->Next ()) {

			cPlayer *player = node->Data ();
			if ((player == nullptr) || !player->Is_Active ()) {
				continue;
			}

			StringClass name = Player_Name (player);
			if ((input != nullptr) && (input[0] != 0) && (strstr (name, input) == nullptr)) {
				continue;
			}

			Print ("   %d: %s\n", player->Get_Id (), name.Peek_Buffer ());
		}
	}
};


class	PInfoConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "pinfo"; }
	const char *	Get_Help (void) override
	{ return "PINFO - print information about the players in the game"; }

	void	Activate (const char *) override
	{
		if (!Server_Command ()) {
			return ;
		}

		Print ("Start PInfo output\n");

		SList<cPlayer> *list = cPlayerManager::Get_Player_Object_List ();
		for (SLNode<cPlayer> *node = list->Head (); node != nullptr; node = node->Next ()) {

			cPlayer *player = node->Data ();
			if ((player == nullptr) || !player->Is_Active ()) {
				continue;
			}

			StringClass name = Player_Name (player);
			Print ("%d,%s,%d,%d,%d,%d,%d,%d\n",
					player->Get_Id (), name.Peek_Buffer (),
					(int)player->Get_Score (), player->Get_Player_Type (),
					player->Get_Ping (), player->Get_Kills (), player->Get_Deaths (),
					(int)player->Get_Money ());
		}

		Print ("End PInfo output\n");
	}
};


class	Kick2ConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "kick2"; }
	const char *	Get_Help (void) override
	{ return "KICK2 <playerid> - Kick a player from the game by ID rather than by name. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command () || (input == nullptr) || (input[0] == 0)) {
			return ;
		}

		int player_id = atoi (input);
		if (Find_Player (player_id) == nullptr) {
			Print ("Player with id %d not found.\n", player_id);
			return ;
		}

		cNetwork::Server_Kill_Connection (player_id);
		cNetwork::Cleanup_After_Client (player_id);
	}
};


class	EjectConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "eject"; }
	const char *	Get_Help (void) override
	{ return "EJECT <player> - Ejects that player from whatever vehicle they are in, if any"; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		GameObject *object = Player_Object (atoi (input));
		if ((object == nullptr) || (ScriptEngine::Get_Vehicle (object) == nullptr)) {
			return ;
		}

		SoldierGameObj *soldier = object->As_SoldierGameObj ();
		if (soldier != nullptr) {
			soldier->Exit_Vehicle ();
		}
	}
};


//-----------------------------------------------------------------------------
//	Teams and money
//-----------------------------------------------------------------------------

class	TeamConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "team"; }
	const char *	Get_Help (void) override
	{ return "TEAM <player> <team> - Changes a players team, taking their cash and score with it. Host only.\n0 = Nod\n1 = GDI"; }

	void	Activate (const char *input) override	{ Change_Team (input, true); }

protected:

	void	Change_Team (const char *input, bool take_cash)
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		int			player_id	= atoi (head);
		GameObject *	object		= Player_Object (player_id);
		if (object == nullptr) {
			return ;
		}

		int team = atoi (tail);
		if (team > 2) {
			team = 2;
		}

		if (team == ScriptEngine::Get_Player_Type (object)) {
			return ;
		}

		if (take_cash) {
			ScriptEngine::Give_Points (object, -ScriptEngine::Get_Points (object), false);
			ScriptEngine::Give_Money (object, -ScriptEngine::Get_Money (object), false);
		}

		Defuse_Player_C4 (player_id, true, true, true);
		ScriptEngine::Change_Team (object, team, true);
	}
};


class	Team2ConsoleFunctionClass : public TeamConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "team2"; }
	const char *	Get_Help (void) override
	{ return "TEAM2 <player> <team> - Changes a players team without taking cash/score. Host only.\n0 = Nod\n1 = GDI"; }

	void	Activate (const char *input) override	{ Change_Team (input, false); }
};


class	DonateConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "donate"; }
	const char *	Get_Help (void) override
	{ return "DONATE <from player> <to player> <amount> - Move cash from one player to another on the same team."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass from_text, rest;
		if (!Split (input, from_text, rest)) {
			return ;
		}

		StringClass to_text, amount_text;
		if (!Split (rest, to_text, amount_text)) {
			return ;
		}

		GameObject *from	= Player_Object (atoi (from_text));
		GameObject *to		= Player_Object (atoi (to_text));
		float			amount	= (float)atof (amount_text);

		if ((from == nullptr) || (to == nullptr) || (amount < 0.0f)) {
			return ;
		}

		if (ScriptEngine::Get_Money (from) < amount) {
			return ;
		}

		if (ScriptEngine::Get_Player_Type (from) != ScriptEngine::Get_Player_Type (to)) {
			return ;
		}

		ScriptEngine::Give_Money (from, -amount, false);
		ScriptEngine::Give_Money (to, amount, false);
	}
};


class	WinConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "win"; }
	const char *	Get_Help (void) override
	{ return "WIN <team> - Kills the buildings of the other team to end the game. Host only.\n0 = Nod\n1 = GDI"; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int winner = atoi (input);
		ScriptEngine::Kill_All_Buildings_By_Team ((winner == PLAYERTYPE_GDI) ? PLAYERTYPE_NOD : PLAYERTYPE_GDI);
	}
};


//-----------------------------------------------------------------------------
//	Talking to players
//-----------------------------------------------------------------------------

static void	Send_Private_Text (int player_id, const char *text, bool admin, int sender_id)
{
	WideStringClass wide (text, true);
	cScTextObj *message = new cScTextObj;
	message->Init (wide, TEXT_MESSAGE_PRIVATE, admin, sender_id, player_id);
}


class	PAMsgConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "pamsg"; }
	const char *	Get_Help (void) override
	{ return "PAMSG <player> <message> - Sends an admin message to a specific player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		int player_id = atoi (head);
		if (Find_Player (player_id) == nullptr) {
			return ;
		}

		Send_Private_Text (player_id, tail, true, HOST_TEXT_SENDER);
	}
};


class	PPageConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "ppage"; }
	const char *	Get_Help (void) override
	{ return "PPAGE <player> <message> - Sends a page to a specific player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		int player_id = atoi (head);
		if (Find_Player (player_id) == nullptr) {
			return ;
		}

		Send_Private_Text (player_id, tail, false, HOST_TEXT_SENDER);
	}
};


struct	TextVisitContext
{
	const char *	Text;
	int				SenderId;
};

static void	Visit_Send_Text (GameObject * /*object*/, int player_id, void *context)
{
	TextVisitContext *data = (TextVisitContext *)context;
	Send_Private_Text (player_id, data->Text, false, data->SenderId);
}


class	TPageConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "tpage"; }
	const char *	Get_Help (void) override
	{ return "TPAGE <team> <message> - Sends a page to a specific team. Host only.\n0 = Nod\n1 = GDI\n2 = both"; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		TextVisitContext context = { tail, HOST_TEXT_SENDER };
		For_Each_Player (atoi (head), Visit_Send_Text, &context);
	}
};


class	TMsgConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "tmsg"; }
	const char *	Get_Help (void) override
	{ return "TMSG <player> <message> - Sends a message to a team as though it came from <player>."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		int		player_id	= atoi (head);
		cPlayer *	player		= Find_Player (player_id);
		if (player == nullptr) {
			return ;
		}

		WideStringClass wide (tail.Peek_Buffer (), true);
		cScTextObj *message = new cScTextObj;
		message->Init (wide, TEXT_MESSAGE_TEAM, false, player_id, -1);
	}
};


//-----------------------------------------------------------------------------
//	Coloured messages in the info box
//-----------------------------------------------------------------------------

struct	ColorMessageContext
{
	const char *	Text;
	int				Red;
	int				Green;
	int				Blue;
};

static void	Visit_Color_Message (GameObject *object, int /*player_id*/, void *context)
{
	ColorMessageContext *data = (ColorMessageContext *)context;
	ScriptEngine::Send_Message_Player (object, data->Red, data->Green, data->Blue, data->Text);
}


class	CMsgConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "cmsg"; }
	const char *	Get_Help (void) override
	{ return "CMSG <red>,<green>,<blue>|private|public|gdi|nod <message> - Displays a colored message in the info box of all players. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass color_text, text;
		if (!Split (input, color_text, text)) {
			return ;
		}

		int red, green, blue;
		Parse_Color (color_text, -1, -1, red, green, blue);
		ScriptEngine::Send_Message (red, green, blue, text);
	}
};


class	CMsgPConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "cmsgp"; }
	const char *	Get_Help (void) override
	{ return "CMSGP <player> <red>,<green>,<blue>|private|public|gdi|nod|player <message> - Displays a colored message in the info box of one player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass player_text, rest;
		if (!Split (input, player_text, rest)) {
			return ;
		}

		StringClass color_text, text;
		if (!Split (rest, color_text, text)) {
			return ;
		}

		int			player_id	= atoi (player_text);
		GameObject *	object		= Player_Object (player_id);
		if (object == nullptr) {
			return ;
		}

		int red, green, blue;
		Parse_Color (color_text, -1, player_id, red, green, blue);
		ScriptEngine::Send_Message_Player (object, red, green, blue, text);
	}
};


class	CMsgTConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "cmsgt"; }
	const char *	Get_Help (void) override
	{ return "CMSGT <team> <red>,<green>,<blue>|private|public|gdi|nod|team <message> - Displays a colored message in the info box of a team. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass team_text, rest;
		if (!Split (input, team_text, rest)) {
			return ;
		}

		StringClass color_text, text;
		if (!Split (rest, color_text, text)) {
			return ;
		}

		int team = atoi (team_text);

		ColorMessageContext context;
		context.Text = text;
		Parse_Color (color_text, team, -1, context.Red, context.Green, context.Blue);

		For_Each_Player (team, Visit_Color_Message, &context);
	}
};


//-----------------------------------------------------------------------------
//	Sound and music
//-----------------------------------------------------------------------------

static StringClass	_CurrentSong;

static void	Visit_Wav_Sound (GameObject *object, int /*player_id*/, void *context)
{
	ScriptEngine::Create_2D_WAV_Sound_Player (object, (const char *)context);
}

static void	Visit_Set_Music (GameObject *object, int /*player_id*/, void *context)
{
	ScriptEngine::Set_Background_Music_Player (object, (const char *)context);
}

static void	Visit_Stop_Music (GameObject *object, int /*player_id*/, void * /*context*/)
{
	ScriptEngine::Stop_Background_Music_Player (object);
}


class	SndPConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "sndp"; }
	const char *	Get_Help (void) override
	{ return "SNDP <player> <wav file> - Plays a WAV file for a specific player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		GameObject *object = Player_Object (atoi (head));
		if (object != nullptr) {
			ScriptEngine::Create_2D_WAV_Sound_Player (object, tail);
		}
	}
};


class	SndTConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "sndt"; }
	const char *	Get_Help (void) override
	{ return "SNDT <team> <wav file> - Plays a WAV file for a specific team. Host only.\n0 = Nod\n1 = GDI\n2 = both"; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		For_Each_Player (atoi (head), Visit_Wav_Sound, (void *)tail.Peek_Buffer ());
	}
};


class	SndAConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "snda"; }
	const char *	Get_Help (void) override
	{ return "SNDA <wav file> - Plays a WAV file for all players. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command () || (input == nullptr) || (input[0] == 0)) {
			return ;
		}

		For_Each_Player (2, Visit_Wav_Sound, (void *)input);
	}
};


class	MusicPConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "musicp"; }
	const char *	Get_Help (void) override
	{ return "MUSICP <player> <mp3 file> - Plays a MP3 file for a specific player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		GameObject *object = Player_Object (atoi (head));
		if (object != nullptr) {
			ScriptEngine::Set_Background_Music_Player (object, tail);
		}
	}
};


class	MusicAConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "musica"; }
	const char *	Get_Help (void) override
	{ return "MUSICA <mp3 file> - Plays a MP3 file for all players. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command () || (input == nullptr) || (input[0] == 0)) {
			return ;
		}

		_CurrentSong = input;
		For_Each_Player (2, Visit_Set_Music, (void *)input);
	}
};


class	NoMusicPConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "nomusicp"; }
	const char *	Get_Help (void) override
	{ return "NOMUSICP <player> - Stops the background music for a specific player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		GameObject *object = Player_Object (atoi (input));
		if (object != nullptr) {
			ScriptEngine::Stop_Background_Music_Player (object);
		}
	}
};


class	NoMusicAConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "nomusica"; }
	const char *	Get_Help (void) override
	{ return "NOMUSICA - Stops the background music for all players. Host only."; }

	void	Activate (const char *) override
	{
		if (!Server_Command ()) {
			return ;
		}

		_CurrentSong = "";
		For_Each_Player (2, Visit_Stop_Music, nullptr);
	}
};


class	SongConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "song"; }
	const char *	Get_Help (void) override
	{ return "SONG - Prints the name of the last song started by the MUSICA command. Host only."; }

	void	Activate (const char *) override
	{
		if (!Server_Command ()) {
			return ;
		}

		if (_CurrentSong.Is_Empty ()) {
			Print ("There is no song playing\n");
		} else {
			Print ("The current song is %s\n", _CurrentSong.Peek_Buffer ());
		}
	}
};


struct	Sound3DContext
{
	const char *	Wav;
	GameObject *	At;
};

static void	Visit_3D_Sound (GameObject *object, int /*player_id*/, void *context)
{
	Sound3DContext *data = (Sound3DContext *)context;
	ScriptEngine::Create_3D_WAV_Sound_At_Bone_Player (object, data->Wav, data->At, "ROOTTRANSFORM");
}


class	Snd3DPConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "snd3dp"; }
	const char *	Get_Help (void) override
	{ return "SND3DP <player> <wav file> - Plays a 3D WAV file for a specific player, at that player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		GameObject *object = Player_Object (atoi (head));
		if (object != nullptr) {
			ScriptEngine::Create_3D_WAV_Sound_At_Bone_Player (object, tail, object, "ROOTTRANSFORM");
		}
	}
};


class	Snd3DAConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "snd3da"; }
	const char *	Get_Help (void) override
	{ return "SND3DA <player> <wav file> - Plays a 3D WAV file for all players at the position of <player>. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		GameObject *at = Player_Object (atoi (head));
		if (at == nullptr) {
			return ;
		}

		Sound3DContext context = { tail, at };
		For_Each_Player (2, Visit_3D_Sound, &context);
	}
};


class	Snd3DTConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "snd3dt"; }
	const char *	Get_Help (void) override
	{ return "SND3DT <player> <team> <wav file> - Plays a 3D WAV file for a team at the position of <player>. Host only.\n0 = Nod\n1 = GDI\n2 = both"; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass player_text, rest;
		if (!Split (input, player_text, rest)) {
			return ;
		}

		StringClass team_text, wav;
		if (!Split (rest, team_text, wav)) {
			return ;
		}

		GameObject *at = Player_Object (atoi (player_text));
		if (at == nullptr) {
			return ;
		}

		Sound3DContext context = { wav, at };
		For_Each_Player (atoi (team_text), Visit_3D_Sound, &context);
	}
};


//-----------------------------------------------------------------------------
//	Emoticons
//-----------------------------------------------------------------------------

struct	IconContext
{
	GameObject *	Target;
	const char *	Model;
	int				Team;
	bool				Enemies;
};

static void	Visit_Icon (GameObject *object, int /*player_id*/, void *context)
{
	IconContext *data = (IconContext *)context;

	bool same_team = (ScriptEngine::Get_Player_Type (object) == data->Team);
	if (same_team == data->Enemies) {
		return ;
	}

	ScriptEngine::Set_Emot_Icon_Player (object, data->Target, data->Model, 2.0f);
}


class	IconConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "icon"; }
	const char *	Get_Help (void) override
	{ return "ICON <player> <w3d file> - Shows an emoticon over the head of a player, visible to their team. Host only."; }

	void	Activate (const char *input) override	{ Show_Icon (input, false); }

protected:

	void	Show_Icon (const char *input, bool enemies)
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		GameObject *target = Player_Object (atoi (head));
		if (target == nullptr) {
			return ;
		}

		IconContext context;
		context.Target		= target;
		context.Model		= tail;
		context.Team		= ScriptEngine::Get_Player_Type (target);
		context.Enemies	= enemies;

		For_Each_Player (2, Visit_Icon, &context);
	}
};


class	Icon2ConsoleFunctionClass : public IconConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "icon2"; }
	const char *	Get_Help (void) override
	{ return "ICON2 <player> <w3d file> - Shows an emoticon over the head of a player, visible to their enemies. Host only."; }

	void	Activate (const char *input) override	{ Show_Icon (input, true); }
};


//-----------------------------------------------------------------------------
//	C4
//-----------------------------------------------------------------------------

class	DisarmConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "disarm"; }
	const char *	Get_Help (void) override
	{ return "DISARM <player> - Disarms all C4 of a player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int player_id = atoi (input);
		if (Find_Player (player_id) != nullptr) {
			Defuse_Player_C4 (player_id, true, true, true);
		}
	}
};


class	DisarmPConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "disarmp"; }
	const char *	Get_Help (void) override
	{ return "DISARMP <player> - Disarms all proximity C4 of a player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int player_id = atoi (input);
		if (Find_Player (player_id) != nullptr) {
			Defuse_Player_C4 (player_id, false, true, false);
		}
	}
};


class	DisarmBConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "disarmb"; }
	const char *	Get_Help (void) override
	{ return "DISARMB <player> - Destroys all beacons of a player. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int player_id = atoi (input);
		if (Find_Player (player_id) == nullptr) {
			return ;
		}

		SLNode<BaseGameObj> *node = GameObjManager::Get_Game_Obj_List ()->Head ();
		SLNode<BaseGameObj> *next = nullptr;

		for (; node != nullptr; node = next) {

			next = node->Next ();

			PhysicalGameObj *phys		= (node->Data () != nullptr) ? node->Data ()->As_PhysicalGameObj () : nullptr;
			BeaconGameObj *	beacon	= (phys != nullptr) ? phys->As_BeaconGameObj () : nullptr;
			if (beacon == nullptr) {
				continue;
			}

			if (ScriptEngine::Get_Player_ID (beacon->Get_Owner ()) == player_id) {
				ScriptEngine::Destroy_Object (beacon);
			}
		}
	}
};


//-----------------------------------------------------------------------------
//	Limits and settings
//-----------------------------------------------------------------------------

class	MLimitConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mlimit"; }
	const char *	Get_Help (void) override
	{ return "MLIMIT <new limit> - Sets the mine limit. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int limit = atoi (input);
		if (limit > 0) {
			SSGMSettingsClass::MineLimit = limit;
		}
	}
};


class	MLimitDConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mlimitd"; }
	const char *	Get_Help (void) override	{ return "MLIMITD - Print the current mine limit. Host only."; }

	void	Activate (const char *) override
	{
		if (Server_Command ()) {
			Print ("The Current Mine Limit is %d\n", SSGMSettingsClass::MineLimit);
		}
	}
};


class	MinedConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mined"; }
	const char *	Get_Help (void) override
	{ return "MINED <team> - Print the current mine count for <team>. Host only.\n0 = Nod\n1 = GDI"; }

	void	Activate (const char *input) override
	{
		if (Server_Command ()) {
			Print ("The Current Mine count is %d\n", Count_Team_Mines (atoi (input)));
		}
	}
};


class	VLimitConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "vlimit"; }
	const char *	Get_Help (void) override
	{ return "VLIMIT <limit> - Changes the vehicle limit per team. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int limit = atoi (input);
		if (limit > 0) {
			VehicleFactoryGameObj::Set_Max_Vehicles_Per_Team (limit);
		}
	}
};


class	VLimitDConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "vlimitd"; }
	const char *	Get_Help (void) override	{ return "VLIMITD - Displays the current vehicle limit. Host only."; }

	void	Activate (const char *) override
	{
		if (Server_Command ()) {
			Print ("Current Vehicle Limit is %d\n", VehicleFactoryGameObj::Get_Max_Vehicles_Per_Team ());
		}
	}
};


class	PLimitConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "plimit"; }
	const char *	Get_Help (void) override	{ return "PLIMIT <new limit> - Changes the player limit."; }

	void	Activate (const char *input) override
	{
		cGameData *game = The_Game ();
		if (game == nullptr) {
			Print ("Error: game is not active.\n");
			return ;
		}

		game->Set_Max_Players (atoi (input));
		Print ("New player limit is %d\n", game->Get_Max_Players ());
	}
};


class	PLimitDConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "plimitd"; }
	const char *	Get_Help (void) override	{ return "PLIMITD - Displays the current player limit."; }

	void	Activate (const char *) override
	{
		cGameData *game = The_Game ();
		if (game == nullptr) {
			Print ("Error: game is not active.\n");
			return ;
		}

		Print ("Current player limit is %d\n", game->Get_Max_Players ());
	}
};


class	TimeConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "time"; }
	const char *	Get_Help (void) override	{ return "TIME <new time> - Changes the time remaining, in seconds."; }

	void	Activate (const char *input) override
	{
		cGameData *game = The_Game ();
		if (game != nullptr) {
			game->Set_Time_Remaining_Seconds ((float)atof (input));
		}
	}
};


class	TimeDConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "timed"; }
	const char *	Get_Help (void) override	{ return "TIMED - Displays the time remaining."; }

	void	Activate (const char *) override
	{
		cGameData *game = The_Game ();
		if (game != nullptr) {
			Print ("Time Remaining is %f\n", game->Get_Time_Remaining_Seconds ());
		}
	}
};


class	TimeLConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "timel"; }
	const char *	Get_Help (void) override	{ return "TIMEL <new limit> - Changes the time limit, in minutes."; }

	void	Activate (const char *input) override
	{
		cGameData *game = The_Game ();
		if (game != nullptr) {
			game->Set_Time_Limit_Minutes (atoi (input));
		}
	}
};


class	TimeLDConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "timeld"; }
	const char *	Get_Help (void) override	{ return "TIMELD - Displays the time limit."; }

	void	Activate (const char *) override
	{
		cGameData *game = The_Game ();
		if (game != nullptr) {
			Print ("Time Limit is %d\n", game->Get_Time_Limit_Minutes ());
		}
	}
};


class	RadarModeConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "radar"; }
	const char *	Get_Help (void) override	{ return "RADAR - Get the radar mode for the server. Host only."; }

	void	Activate (const char *) override
	{
		if (Server_Command ()) {
			Print ("The Current Radar Mode is %d\n", (int)The_Game ()->Get_Radar_Mode ());
		}
	}
};


//-----------------------------------------------------------------------------
//	The map cycle
//-----------------------------------------------------------------------------

class	MapConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "map"; }
	const char *	Get_Help (void) override	{ return "MAP - Print the name of the current map. Host only."; }

	void	Activate (const char *) override
	{
		if (Server_Command ()) {
			Print ("The Current Map is %s\n", The_Game ()->Get_Map_Name ().Peek_Buffer ());
		}
	}
};


class	ModConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mod"; }
	const char *	Get_Help (void) override	{ return "MOD - Print the name of the current mod. Host only."; }

	void	Activate (const char *) override
	{
		if (!Server_Command ()) {
			return ;
		}

		const StringClass &name = The_Game ()->Get_Mod_Name ();
		if (name.Is_Empty ()) {
			Print ("There is no mod loaded\n");
		} else {
			Print ("The Current Mod Package is %s\n", name.Peek_Buffer ());
		}
	}
};


class	MapNumConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mapnum"; }
	const char *	Get_Help (void) override
	{ return "MAPNUM - Print the index within the map cycle of the current map. Host only."; }

	void	Activate (const char *) override
	{
		if (Server_Command ()) {
			Print ("The Current Map Number is %d\n", The_Game ()->Get_Map_Cycle_Index ());
		}
	}
};


class	MListConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mlist"; }
	const char *	Get_Help (void) override
	{ return "MLIST <position> - Print the name of the map in the given position in the map cycle. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		int position = atoi (input);
		if ((position < 0) || (position >= cGameData::MAX_MAPS)) {
			return ;
		}

		const StringClass &name = The_Game ()->Get_Map_Cycle (position);
		if (name.Is_Empty ()) {
			Print ("There is no map in position %d of the map list\n", position);
		} else {
			Print ("The map in position %d of the map list is %s\n", position, name.Peek_Buffer ());
		}
	}
};


class	MListCConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mlistc"; }
	const char *	Get_Help (void) override
	{ return "MLISTC <position> <map> - Change the map at <position> in the map cycle. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		int position = atoi (head);
		if ((position < 0) || (position >= cGameData::MAX_MAPS)) {
			return ;
		}

		The_Game ()->Set_Map_Cycle (position, tail);
	}
};


class	SsUrlConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "ssurl"; }
	const char *	Get_Help (void) override
	{ return "SSURL <url> - Where remote screenshots are uploaded. No url turns the feature off. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		RemoteScreenshot::Set_Upload_Url (input);

		if (RemoteScreenshot::Is_Enabled ()) {
			Print ("Remote screenshots will be uploaded to %s\n", RemoteScreenshot::Get_Upload_Url ());
		} else {
			Print ("Remote screenshots are off\n");
		}
	}
};


class	SsHotConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "sshot"; }
	const char *	Get_Help (void) override
	{ return "SSHOT <player> - Ask a player for a picture of their game window. Needs SSURL first. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		//
		//	Refused rather than defaulted.  A screenshot with nowhere to go is a
		//	picture of somebody taken for no reason.
		//
		if (!RemoteScreenshot::Is_Enabled ()) {
			Print ("No upload url is set. Use SSURL first.\n");
			return ;
		}

		int player_id = atoi (input);
		if (Find_Player (player_id) == nullptr) {
			return ;
		}

		StringClass url = RemoteScreenshot::Get_Upload_Url ();

		cScScreenshotRequestEvent *request = new cScScreenshotRequestEvent;
		request->Init (player_id, url);
	}
};


class	MapChConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "mapch"; }
	const char *	Get_Help (void) override
	{ return "MAPCH <player> <map> - Ask a player whether they have a map. The answer is printed when it arrives. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		StringClass head, tail;
		if (!Split (input, head, tail)) {
			return ;
		}

		int player_id = atoi (head);
		if (Find_Player (player_id) == nullptr) {
			return ;
		}

		//
		//	The answer arrives later, in cCsMapQueryResponseEvent::Act, and prints
		//	itself.  There is nothing to say here that would still be true by then.
		//
		cScMapQueryEvent *query = new cScMapQueryEvent;
		query->Init (player_id, tail);
	}
};


class	TagConsoleFunctionClass : public ConsoleFunctionClass
{
public:
	const char *	Get_Name (void) override	{ return "tag"; }
	const char *	Get_Help (void) override
	{ return "TAG <player> <text> - Sets a name tag shown under a player. Empty text clears it. Host only."; }

	void	Activate (const char *input) override
	{
		if (!Server_Command ()) {
			return ;
		}

		//
		//	Split returns false for a bare player id, which is how a tag is cleared:
		//	the tail is empty and that is a legal tag rather than bad input.
		//
		StringClass head, tail;
		Split (input, head, tail);

		cPlayer *player = Find_Player (atoi (head));
		if (player == nullptr) {
			return ;
		}

		WideStringClass tag;
		tag.Convert_From (tail);
		player->Set_Custom_Tag (tag);
	}
};


//-----------------------------------------------------------------------------

void	TTConsole::Add_Console_Functions (DynamicVectorClass<ConsoleFunctionClass *> &list)
{
	list.Add (new IdConsoleFunctionClass);
	list.Add (new PInfoConsoleFunctionClass);
	list.Add (new Kick2ConsoleFunctionClass);
	list.Add (new EjectConsoleFunctionClass);

	list.Add (new TeamConsoleFunctionClass);
	list.Add (new Team2ConsoleFunctionClass);
	list.Add (new DonateConsoleFunctionClass);
	list.Add (new WinConsoleFunctionClass);

	list.Add (new PAMsgConsoleFunctionClass);
	list.Add (new PPageConsoleFunctionClass);
	list.Add (new TPageConsoleFunctionClass);
	list.Add (new TMsgConsoleFunctionClass);

	list.Add (new CMsgConsoleFunctionClass);
	list.Add (new CMsgPConsoleFunctionClass);
	list.Add (new CMsgTConsoleFunctionClass);

	list.Add (new SndPConsoleFunctionClass);
	list.Add (new SndTConsoleFunctionClass);
	list.Add (new SndAConsoleFunctionClass);
	list.Add (new Snd3DPConsoleFunctionClass);
	list.Add (new Snd3DAConsoleFunctionClass);
	list.Add (new Snd3DTConsoleFunctionClass);

	list.Add (new MusicPConsoleFunctionClass);
	list.Add (new MusicAConsoleFunctionClass);
	list.Add (new NoMusicPConsoleFunctionClass);
	list.Add (new NoMusicAConsoleFunctionClass);
	list.Add (new SongConsoleFunctionClass);

	list.Add (new IconConsoleFunctionClass);
	list.Add (new Icon2ConsoleFunctionClass);

	list.Add (new DisarmConsoleFunctionClass);
	list.Add (new DisarmPConsoleFunctionClass);
	list.Add (new DisarmBConsoleFunctionClass);

	list.Add (new MLimitConsoleFunctionClass);
	list.Add (new MLimitDConsoleFunctionClass);
	list.Add (new MinedConsoleFunctionClass);
	list.Add (new VLimitConsoleFunctionClass);
	list.Add (new VLimitDConsoleFunctionClass);
	list.Add (new PLimitConsoleFunctionClass);
	list.Add (new PLimitDConsoleFunctionClass);

	list.Add (new TimeConsoleFunctionClass);
	list.Add (new TimeDConsoleFunctionClass);
	list.Add (new TimeLConsoleFunctionClass);
	list.Add (new TimeLDConsoleFunctionClass);

	list.Add (new RadarModeConsoleFunctionClass);
	list.Add (new MapConsoleFunctionClass);
	list.Add (new ModConsoleFunctionClass);
	list.Add (new MapNumConsoleFunctionClass);
	list.Add (new MListConsoleFunctionClass);
	list.Add (new MListCConsoleFunctionClass);

	list.Add (new MapChConsoleFunctionClass);
	list.Add (new SsUrlConsoleFunctionClass);
	list.Add (new SsHotConsoleFunctionClass);
	list.Add (new TagConsoleFunctionClass);

	return ;
}
