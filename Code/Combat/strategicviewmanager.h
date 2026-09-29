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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/strategicviewmanager.h                $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Owns the commander view state (first person <-> overhead).  This is the *only* thing         *
 *	allowed to switch the camera mode; CommanderCamera exposes the mechanism but no policy.      *
 *                                                                                             *
 *	Everything here is client-side presentation.  Nothing in this file mutates game state or     *
 *	requires server authority -- a client may enter and leave the strategic view freely as far   *
 *	as the simulation is concerned.                                                             *
 *                                                                                             *
 *	Two seams keep this file free of dependencies that other agents are writing in parallel.     *
 *	Both default to a permissive stub so the view works standalone, and both are replaced with   *
 *	a one-line adapter at merge time:                                                           *
 *                                                                                             *
 *		IStrategicViewBounds     -> BaseBoundary          (Code/Combat/baseboundary.h, A3)       *
 *		IStrategicViewPermission -> CommanderPermissions  (Code/Combat/commanderpermissions.h, A1)*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __STRATEGICVIEWMANAGER_H
#define __STRATEGICVIEWMANAGER_H

#include "always.h"
#include "vector3.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class ChunkSaveClass;
class ChunkLoadClass;
class CommanderCamera;


////////////////////////////////////////////////////////////////
//
//	IStrategicViewBounds
//
//	"Where is this team's base, and how far may the camera stray
//	from it?"  Implemented at merge time by an adapter over
//	BaseBoundary (A3).  The default implementation is permissive:
//	it reports no known centre, which makes the clamp fall back to
//	wherever the view was opened.
//
////////////////////////////////////////////////////////////////
class IStrategicViewBounds
{
public:
	virtual ~IStrategicViewBounds (void)	{}

	//
	//	Returns false when this team has no known base, in which case the
	//	caller keeps using the position the overhead view was opened at.
	//
	virtual bool	Get_Base_Center (int player_type, Vector3 &center) const = 0;

	//
	//	Maximum distance, in metres, that the camera's look-at point may travel
	//	from the base centre.  Return a non-positive value for "unbounded".
	//
	virtual float	Get_Max_Pan_Distance (int player_type) const = 0;
};


////////////////////////////////////////////////////////////////
//
//	IStrategicViewPermission
//
//	"Is this player allowed into the overhead view at all?"
//	Implemented at merge time by an adapter over CommanderPermissions
//	(A1).  The default implementation permits everyone, so that the
//	view is usable before the commander layer lands.
//
////////////////////////////////////////////////////////////////
class IStrategicViewPermission
{
public:
	virtual ~IStrategicViewPermission (void)	{}

	virtual bool	Is_Strategic_View_Permitted (int player_type) const = 0;
};


////////////////////////////////////////////////////////////////
//
//	StrategicViewManager
//
////////////////////////////////////////////////////////////////
class StrategicViewManager
{
public:

	////////////////////////////////////////////////////////////////
	//	Public data types
	////////////////////////////////////////////////////////////////
	enum ViewModeType
	{
		VIEW_FIRST_PERSON	= 0,
		VIEW_OVERHEAD
	};

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	static void			Init (void);
	static void			Shutdown (void);

	//
	//	Per-frame client tick.  Samples the key bindings, drives pan/zoom and
	//	clamps the result.  Safe to call when no CommanderCamera exists.
	//
	static void			Think (void);

	//
	//	Mode.  Request_View_Mode is the single entry point for switching the
	//	camera; it returns false when the switch was refused (no camera, or the
	//	permission hook said no).
	//
	static ViewModeType	Get_View_Mode (void)			{ return CurrentViewMode; }
	static bool				Is_Overhead_View (void)		{ return CurrentViewMode == VIEW_OVERHEAD; }
	static bool				Request_View_Mode (ViewModeType mode);
	static bool				Toggle_View_Mode (void);

	//
	//	Which team's base bounds the camera.  Passed as an int so this header
	//	stays free of Commando includes (it is a PLAYERTYPE_* value).
	//
	static void			Set_Player_Type (int player_type)	{ PlayerType = player_type; }
	static int			Get_Player_Type (void)					{ return PlayerType; }

	//
	//	Seams -- see the interface comments above.  Passing NULL restores the
	//	built-in permissive default.
	//
	static void			Set_Bounds_Provider (IStrategicViewBounds *provider);
	static void			Set_Permission_Provider (IStrategicViewPermission *provider);

	static IStrategicViewBounds *		Get_Bounds_Provider (void);
	static IStrategicViewPermission *	Get_Permission_Provider (void);

	//
	//	Input gating.  A dialog that wants the keys for itself turns this off
	//	without leaving the overhead view.
	//
	static void			Set_Input_Enabled (bool onoff)	{ InputEnabled = onoff; }
	static bool			Is_Input_Enabled (void)				{ return InputEnabled; }

	//
	//	Tunables
	//
	static void			Set_Fallback_Max_Pan_Distance (float distance);
	static float		Get_Fallback_Max_Pan_Distance (void)	{ return FallbackMaxPanDistance; }

	static void			Set_Exit_On_Menu_Toggle (bool onoff)	{ ExitOnMenuToggle = onoff; }
	static bool			Is_Exit_On_Menu_Toggle (void)				{ return ExitOnMenuToggle; }

	//
	//	Query helpers, also used by the cam_mode console command.
	//
	static bool			Get_Camera_Position (Vector3 &position);
	static bool			Get_Look_At_Position (Vector3 &position);
	static float		Get_Camera_Height (void);
	static const char *	Get_View_Mode_Name (ViewModeType mode);

	//
	//	Persistence -- CHUNKID_COMMANDER_CAMERA.  Only the durable framing is
	//	saved (look-at point, height, heading, team).  The live mode and any
	//	in-flight transition are rebuilt, never restored: a save made while the
	//	overhead view was open loads back into first person.
	//
	static bool			Save (ChunkSaveClass &csave);
	static bool			Load (ChunkLoadClass &cload);

private:

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////
	static void			Apply_View_Presentation (bool overhead);
	static void			Update_Input (CommanderCamera *camera, float frame_seconds);
	static void			Clamp_To_Base (CommanderCamera *camera);
	static bool			Get_Anchor_Position (Vector3 &position);

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////
	static ViewModeType					CurrentViewMode;
	static int								PlayerType;
	static bool								InputEnabled;
	static bool								ExitOnMenuToggle;
	static bool								IsInitialized;

	//
	//	Whether the combat HUD was on when the overhead view was opened, so
	//	that leaving restores what the player had rather than forcing it on.
	//
	static bool								WasHUDEnabled;
	static float							FallbackMaxPanDistance;
	static Vector3							OpenPosition;

	static IStrategicViewBounds *		BoundsProvider;
	static IStrategicViewPermission *	PermissionProvider;
};


#endif //__STRATEGICVIEWMANAGER_H
