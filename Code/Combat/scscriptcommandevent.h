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

#ifndef	__SCSCRIPTCOMMANDEVENT_H__
#define	__SCSCRIPTCOMMANDEVENT_H__

#include "networkobject.h"
#include "netclassids.h"
#include "vector3.h"
#include "wwstring.h"

//-----------------------------------------------------------------------------
//
//	The set of script commands whose effect belongs to one client rather than to
//	the world.  A script runs on the server, so anything that draws, plays, or
//	fades has to be asked for over the wire before the player it is meant for
//	can see it.
//
enum ScriptClientCommandEnum
{
	SCRIPT_CLIENT_CMD_SEND_MESSAGE,				// Text, Color
	SCRIPT_CLIENT_CMD_CREATE_2D_SOUND,			// Text
	SCRIPT_CLIENT_CMD_CREATE_2D_WAV_SOUND,		// Text
	SCRIPT_CLIENT_CMD_CREATE_SOUND,				// Text, Position
	SCRIPT_CLIENT_CMD_SET_HUD_HELP_TEXT,		// IntParam1 = string id, Color
	SCRIPT_CLIENT_CMD_SET_SCREEN_FADE_COLOR,	// Color, FloatParam1 = seconds
	SCRIPT_CLIENT_CMD_SET_SCREEN_FADE_OPACITY,// FloatParam1 = opacity, FloatParam2 = seconds
	SCRIPT_CLIENT_CMD_FORCE_CAMERA_LOOK,		// Position
	SCRIPT_CLIENT_CMD_ENABLE_RADAR,				// IntParam1 = on/off
	SCRIPT_CLIENT_CMD_SET_BACKGROUND_MUSIC,	// Text
	SCRIPT_CLIENT_CMD_FADE_BACKGROUND_MUSIC,	// Text, IntParam1 = fade out, IntParam2 = fade in
	SCRIPT_CLIENT_CMD_STOP_BACKGROUND_MUSIC,	//
	SCRIPT_CLIENT_CMD_DISPLAY_GDI_TERMINAL,	//
	SCRIPT_CLIENT_CMD_DISPLAY_NOD_TERMINAL,	//
	SCRIPT_CLIENT_CMD_SET_GLOBAL_STEALTH_DISABLE,	// IntParam1 = 1 to suppress every cloak
	SCRIPT_CLIENT_CMD_SET_CAMERA_HOST,		// IntParam1 = object id, 0 to give the camera back
	SCRIPT_CLIENT_CMD_SET_SUBOBJECT_ANIMATION,	// IntParam1 = object id, IntParam2 = 1 looping | 2 blended,
												// FloatParam1/2 = first/last frame, Text = animation, Text2 = sub-object
	SCRIPT_CLIENT_CMD_SET_INFO_TEXTURE,			// Text = texture, empty to clear it
	SCRIPT_CLIENT_CMD_LOAD_HUD_INI,				// Text = ini filename
	SCRIPT_CLIENT_CMD_CHANGE_RADAR_MAP,			// Text = texture, FloatParam1 = scale,
												// Position X/Y = the map's centre pixel
	SCRIPT_CLIENT_CMD_CREATE_LIGHTNING,			// Position = one end, Color = the other,
												// FloatParam1 = width
	SCRIPT_CLIENT_CMD_DISPLAY_GAME_HINT,		// IntParam1 = event id, IntParam2 = title string id,
												// IntParam3/4/5 = the three text string ids,
												// Text = sound, Text2 = texture or empty

	SCRIPT_CLIENT_CMD_COUNT
};

//-----------------------------------------------------------------------------
//
//	A S->C event carrying one script command to one client, one team, or every
//	client.  The server never runs the command itself unless it is also playing;
//	`Act` is the client-side half and calls the ordinary local ScriptEngine
//	function, so there is exactly one implementation of each effect.
//
class	cScScriptCommandEvent : public NetworkObjectClass
{
public:

	cScScriptCommandEvent (void);

	//
	//	`client_id` addresses one client; pass -1 to reach everyone.  A team is
	//	resolved to its clients through GameEventBus::Raise_Client_Query, which
	//	only the game answers -- in the editor nobody does and the command is a
	//	no-op, which is the right answer for a tool with no clients.
	//
	void					Init (ScriptClientCommandEnum command, int client_id);
	void					Init_For_Team (ScriptClientCommandEnum command, int team);

	void					Set_Text (const char *text)			{ Text = text; }
	void					Set_Text2 (const char *text)		{ Text2 = text; }
	void					Set_Color (const Vector3 &color)		{ Color = color; }
	void					Set_Position (const Vector3 &pos)	{ Position = pos; }
	void					Set_Int_Params (int p1, int p2 = 0)	{ IntParam1 = p1; IntParam2 = p2; }
	void					Set_Int_Params2 (int p3, int p4 = 0, int p5 = 0)
								{ IntParam3 = p3; IntParam4 = p4; IntParam5 = p5; }
	void					Set_Float_Params (float p1, float p2 = 0.0f)	{ FloatParam1 = p1; FloatParam2 = p2; }

	virtual void		Delete (void) override										{ delete this; }

	virtual void		Export_Creation (BitStreamClass &packet) override;
	virtual void		Import_Creation (BitStreamClass &packet) override;
	virtual uint32		Get_Network_Class_ID (void) const override				{ return NETCLASSID_SCSCRIPTCOMMANDEVENT; }

private:

	void					Send_To (int client_id);
	void					Act (void);

	int					Command;
	int					IntParam1;
	int					IntParam2;
	int					IntParam3;
	int					IntParam4;
	int					IntParam5;
	float					FloatParam1;
	float					FloatParam2;
	Vector3				Color;
	Vector3				Position;
	StringClass			Text;
	StringClass			Text2;
};

//-----------------------------------------------------------------------------

#endif	// __SCSCRIPTCOMMANDEVENT_H__
