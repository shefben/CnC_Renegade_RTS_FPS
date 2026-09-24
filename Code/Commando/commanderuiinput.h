/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderuiinput.h                  $*
 *                                                                                             *
 *	Roadmap Sections 41-43, Phases 35-37 -- the pointer half of Commander Mode.                 *
 *                                                                                             *
 *	Everything under the Commander layer was already live and server-authoritative before this   *
 *	file existed; what it had no way to reach was a player's hand.  The panel drew cards, the     *
 *	placement validator drew ghosts, and every verb that moved either was a console command,      *
 *	because Commander Mode never showed a cursor and never stopped the mouse being a look axis.    *
 *	This is that missing phase and nothing else: it shows the cursor while Commander Mode is       *
 *	active, gates the FPS input that would otherwise fire a weapon under it, and turns a click      *
 *	into exactly the same model/net-event calls `Code/Commando/consolefunction.cpp` already makes.  *
 *                                                                                             *
 *	It lives in Commando rather than Combat for one reason: clicking a unit card has to send       *
 *	`cCommanderProductionEvent`, which is a Commando network class.  The half that genuinely       *
 *	belongs to the panel -- where its boxes are on screen -- stays in                              *
 *	`CommanderSidebarRenderer::Hit_Test`, next to the code that computed them.                     *
 *                                                                                             *
 *	It is not a wwui dialog, deliberately, for the reason `commandersidebarrenderer.h` already     *
 *	gives: registering a dialog would take input focus entirely, and Section 3A requires the        *
 *	Commander to stay an embodied, vulnerable soldier with the world live behind the panel.         *
 *	Showing the cursor and narrowing the input gate gets the pointer without giving up the body.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERUIINPUT_H
#define COMMANDERUIINPUT_H

#include "always.h"

class CommanderUIInput
{
	public:

		//	`render_available` is false on a dedicated server and in a device-less self check;
		//	Init then makes every other call a no-op, the same contract `CommanderSidebarRenderer
		//	::Init` already has.  There is no cursor to show and no click to read without a device.
		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		//	Once per client frame.  Owns the cursor/input-gate transitions as well as the clicks,
		//	so Commander Mode entered from anywhere -- the console command today, a keybind later --
		//	gets the pointer without that entry point having to know this file exists.
		static void		Update(void);

		static bool		Is_Pointer_Active(void);

		/*
		**	The `b` key, and anything else that wants to open or close Commander Mode.  Unlike
		**	calling `CommanderModeController` directly this also asks the server for the job
		**	first: entering the mode without being the team's recorded Commander gives a camera
		**	that cannot buy, place or order anything, since every authority gate in the layer
		**	refuses a sender who is not the Commander.  In a solo or skirmish match the claim is
		**	uncontested and granted the same tick, so the player is always their team's
		**	Commander there.
		*/
		static void		Toggle_Commander_Mode(void);
		static bool		Is_Local_Player_Commander(void);

		//	The entering half of the toggle, for the `commander_mode 1` console command, so the
		//	key and the command claim and enter identically.  False with no living star or when
		//	already in Commander Mode.
		static bool		Enter_Commander_Mode(void);

		//	Counters, so a self check can prove the click path without a mouse.
		static int		Get_Click_Count(void);
		static int		Get_Card_Click_Count(void);
};

#endif // COMMANDERUIINPUT_H
