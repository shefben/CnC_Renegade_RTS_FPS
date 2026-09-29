/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderoverlay.h                  $*
 *                                                                                             *
 *	Commander Mode's 2D overlay, ported from the CnC_Renegade_new fork's CommanderOverlay:      *
 *	header, bottom power bar, 2x6 build sidebar, queue strip, tooltip, minimap, selection box    *
 *	the software cursor.  Layout and drawing are that fork's; the data behind them is this      *
 *	tree's own Commander layer (CommanderSidebarModel, CommanderProductionService,              *
 *	CommanderSelection, CommanderModeController, CommanderCommandPalette).  CommanderUIInput    *
 *	still owns click behavior and asks this class what is under the cursor.                    *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDEROVERLAY_H
#define COMMANDEROVERLAY_H

#include "always.h"
#include "vector2.h"
#include "vector3.h"

class RectClass;

////////////////////////////////////////////////////////////////
//
//	Layout -- the CnC_Renegade_new fork's 640x480 design space.
//
////////////////////////////////////////////////////////////////
namespace CommanderOverlayConstants
{
	enum
	{
		//
		//	The right sidebar's item grid.  Six rows of buildings, five of units:
		//	the unit list gives its last row up to the production queue strip,
		//	which is worth more than two more cameos on a page.  Five rows still
		//	fits the twenty-odd character presets in two pages.
		//
		GRID_COLUMNS		= 2,
		GRID_ROWS			= 6,
		UNIT_GRID_ROWS		= 5,

		//	The larger of the two, for anything that has to size an array.
		MAX_ITEMS_PER_PAGE	= GRID_COLUMNS * GRID_ROWS,

		//	Matches BuildingCatalogConstants::MAX_ENTRIES.
		MAX_ENTRIES			= 256,

		//	Queue rows the strip shows before collapsing the rest into a count.
		MAX_QUEUE_ROWS		= 4
	};

	//
	//	The Renegade palette.  Taken from the shipped HUD: a very dark
	//	desaturated steel for panel fill, and the amber that the game uses for
	//	every readout and frame.  Team colours are the sidebar accents.
	//
	const unsigned int	COLOR_PANEL_FILL		= 0xC0101418;	//	ARGB
	const unsigned int	COLOR_PANEL_FILL_DARK	= 0xD0080A0C;
	const unsigned int	COLOR_FRAME				= 0xFFC8A020;	//	amber
	const unsigned int	COLOR_FRAME_DIM			= 0xFF6A5518;
	const unsigned int	COLOR_TEXT				= 0xFFE8D8A0;
	const unsigned int	COLOR_TEXT_DIM			= 0xFF807860;
	const unsigned int	COLOR_TEXT_BAD			= 0xFFE05038;
	const unsigned int	COLOR_TEXT_GOOD			= 0xFF60D050;
	const unsigned int	COLOR_SLOT_FILL			= 0xB0182028;
	const unsigned int	COLOR_SLOT_HOVER		= 0xC0304860;
	const unsigned int	COLOR_SLOT_SELECTED		= 0xD0806018;
	const unsigned int	COLOR_SLOT_BLOCKED		= 0x80200C0C;
	const unsigned int	COLOR_GDI				= 0xFFF0C020;
	const unsigned int	COLOR_NOD				= 0xFFC02020;
	const unsigned int	COLOR_CURSOR			= 0xFFE8D8A0;
	const unsigned int	COLOR_GHOST_OK			= 0x8040E040;
	const unsigned int	COLOR_GHOST_BAD			= 0x80E04040;

	//
	//	Cameo tints.  A painted cameo is finished artwork -- a framed C&C plate,
	//	gold for GDI and red for Nod -- so it is drawn WHITE, meaning untinted.
	//	Multiplying it by the amber the rest of the HUD uses would wash the
	//	team colour out of the frame and make every plate look GDI.
	//
	//	The drawn silhouettes that stand in for missing art keep the amber:  they
	//	are line work, not artwork, and they should read as part of the HUD.
	//
	const unsigned int	COLOR_CAMEO				= 0xFFFFFFFF;
	const unsigned int	COLOR_CAMEO_BLOCKED		= 0xFF6A6A6A;

	//	Design-space geometry.
	const float	DESIGN_WIDTH			= 640.0F;
	const float	DESIGN_HEIGHT			= 480.0F;

	const float	SIDEBAR_WIDTH			= 120.0F;	//	classic C&C sidebar, docked to the right
	const float	HEADER_HEIGHT			= 54.0F;	//	commander/credits/placement status
	const float	SIDEBAR_PADDING		= 6.0F;
	const float	SLOT_SIZE				= 52.0F;
	const float	SLOT_GAP				= 4.0F;
	const float	FRAME_WIDTH				= 1.0F;
	const float	CURSOR_SIZE				= 12.0F;
	const float	BOTTOM_BAR_TOP			= 344.0F;
	const float	COMMAND_GRID_LEFT		= 389.0F;
	const float	COMMAND_GRID_TOP		= 365.0F;
	const float	COMMAND_BUTTON_WIDTH	= 40.0F;
	const float	COMMAND_BUTTON_HEIGHT	= 34.0F;
	const float	COMMAND_BUTTON_GAP		= 2.0F;

	//	The two page arrows on the page-indicator row at the foot of the bar.
	const float	PAGE_BUTTON_WIDTH		= 14.0F;
	const float	PAGE_BUTTON_HEIGHT	= 12.0F;
	const float	PAGE_BUTTON_GAP		= 2.0F;

	//	The rally-point button, in the dead space below the last slot row.
	const float	RALLY_BUTTON_HEIGHT	= 14.0F;
	const float	RALLY_BUTTON_BOTTOM	= 446.0F;	//	design space, above the page row

	//
	//	The production queue strip.  Occupies the slot row the unit grid gives
	//	up, between the last cameo and the rally button.
	//
	const float	QUEUE_STRIP_TOP		= 368.0F;
	const float	QUEUE_HEADER_HEIGHT	= 11.0F;
	const float	QUEUE_ROW_HEIGHT		= 11.0F;
	const float	QUEUE_ROW_GAP			= 1.0F;
	const float	QUEUE_PROGRESS_HEIGHT= 2.0F;

	//	Longest unit name the strip will draw before truncating it.
	enum { QUEUE_NAME_CHARS = 11 };

	//	Cursor travel per unit of raw mouse movement.
	const float	CURSOR_SPEED			= 1.0F;

	//
	//	Design-space travel, in either axis, at which a held left button stops
	//	being a click and becomes a drag.  Four is comfortably above the jitter
	//	of a press and comfortably below a deliberate movement.
	//
	const float	DRAG_THRESHOLD			= 4.0F;

	//	Two clicks on the same object inside this select every one like it.
	const float	DOUBLE_CLICK_SECONDS	= 0.35F;

	//
	//	The rubber band.  A translucent fill so the terrain stays readable
	//	underneath, and a solid edge so the rectangle has a definite boundary --
	//	a fill alone reads as a smear at low opacity.
	//
	const unsigned int	COLOR_SELECT_BOX_FILL	= 0x2860E060;
	const unsigned int	COLOR_SELECT_BOX_EDGE	= 0xE060E060;

	//
	//	The shroud.  Unexplored ground is very nearly opaque -- not fully, so a
	//	commander can still make out the coastline of a map they have never
	//	scouted and does not feel like they are looking at a bug.  Explored but
	//	unwatched ground is dimmed rather than hidden:  the terrain is
	//	remembered, the units on it are not.
	//
	const unsigned int	COLOR_FOG_UNEXPLORED	= 0xF2000000;
	const unsigned int	COLOR_FOG_EXPLORED		= 0x66000000;

	//
	//	Shroud sample spacing, in design-space pixels.  Eight gives a 78 x 60
	//	sample grid over the world half of the screen -- under five thousand
	//	plane intersections a frame, which is nothing -- and is fine enough that
	//	the edge reads as a soft boundary rather than as chunky tiles.
	//
	const float	FOG_SAMPLE_STEP				= 8.0F;

	//
	//	Patrol routes.
	//
	//	Starting a route within this many metres of an existing one of the same
	//	team EXTENDS that route instead of creating a second.  Generous, because
	//	the alternative failure -- two routes stacked on top of each other, one
	//	of them invisible -- is far worse than accidentally extending one.
	//
	const float	ROUTE_EXTEND_DISTANCE		= 12.0F;

	//	Leg lines, and the marker at each authored point.
	const float	ROUTE_NODE_SIZE				= 3.0F;
	const float	ROUTE_LINE_WIDTH			= 1.5F;

	//
	//	Committed routes are amber like the rest of the HUD;  the one being
	//	drawn is green, so that "this is not stored yet" is legible without
	//	reading the status strip.
	//
	const unsigned int	COLOR_ROUTE			= 0xC0C8A020;
	const unsigned int	COLOR_ROUTE_DRAFT	= 0xE060E060;

	//
	//	The squad strip.  Left edge, stacked directly ABOVE the minimap and the
	//	same width as it, so the two share an edge instead of each finding their
	//	own.  The numbers are the minimap's own -- PANEL_LEFT and PANEL_SIZE --
	//	repeated rather than included, for the same reason the status strip
	//	repeats its two:  this header may not pull in the minimap's.
	//
	//	SQUAD_STRIP_BOTTOM is four above the minimap's top edge, which the
	//	minimap states as PANEL_BOTTOM (474) minus PANEL_SIZE (128).
	//
	const float	SQUAD_STRIP_LEFT		= 6.0F;
	const float	SQUAD_STRIP_WIDTH		= 128.0F;
	const float	SQUAD_STRIP_BOTTOM		= 342.0F;
	const float	SQUAD_HEADER_HEIGHT		= 11.0F;
	const float	SQUAD_ROW_HEIGHT		= 11.0F;
	const float	SQUAD_ROW_GAP			= 1.0F;

	enum CommanderOverlaySquadLimits
	{
		//
		//	Rows the strip draws before it stops.  Eight is what fits above the
		//	minimap, and a team that has formed more squads than that is one
		//	whose ninth squad is not the one the commander is looking for.
		//
		MAX_SQUAD_ROWS		= 8,

		//	Objective name characters before truncation.  As QUEUE_NAME_CHARS.
		SQUAD_NAME_CHARS	= 12
	};

	//
	//	A squad's objective.  Blue, because the two other things drawn in the
	//	world are amber (routes) and green (selection), and a third amber marker
	//	would be indistinguishable from a route node at a glance.
	//
	const unsigned int	COLOR_SQUAD_OBJECTIVE	= 0xC04898E0;

	const float	SQUAD_OBJECTIVE_SIZE	= 5.0F;

	//
	//	Support-power readout.  A short stack of rows at the bottom-left, just
	//	above the squad strip, each a name and a charge state.  Aligned to the
	//	minimap's left edge so the whole left column shares one.
	//
	const float	POWER_ROW_LEFT			= 6.0F;
	const float	POWER_ROW_WIDTH			= 128.0F;
	const float	POWER_ROW_HEIGHT		= 11.0F;
	const float	POWER_ROW_GAP			= 1.0F;
	const float	POWER_STACK_BOTTOM		= 300.0F;	//	above SQUAD_STRIP_BOTTOM's stack

	//	A charged power reads bright;  a charging one dim;  an aimed one green.
	const unsigned int	COLOR_POWER_READY		= 0xFFE8D8A0;
	const unsigned int	COLOR_POWER_CHARGING	= 0xFF807860;
	const unsigned int	COLOR_POWER_ARMED		= 0xFF60E060;

	//	The aiming reticle, drawn on the ground at the cursor while armed.
	const float	POWER_RETICLE_SIZE		= 8.0F;
	const float	POWER_RETICLE_WIDTH		= 1.5F;
};


////////////////////////////////////////////////////////////////
//
//	What lies under a screen point.
//
////////////////////////////////////////////////////////////////
enum CommanderOverlayHitType
{
	COMMANDER_OVERLAY_HIT_NONE = 0,
	COMMANDER_OVERLAY_HIT_PANEL,			// on the overlay's chrome, nothing actionable
	COMMANDER_OVERLAY_HIT_CARD,			// a sidebar slot holding a card
	COMMANDER_OVERLAY_HIT_PAGE_PREV,
	COMMANDER_OVERLAY_HIT_PAGE_NEXT,
	COMMANDER_OVERLAY_HIT_MINIMAP,		// out_world holds the ground point
	COMMANDER_OVERLAY_HIT_COMMAND,		// out_command_index holds a CommanderPaletteCommandType
};


////////////////////////////////////////////////////////////////
//
//	CommanderOverlay
//
////////////////////////////////////////////////////////////////
class CommanderOverlay
{
public:

	static void		Init (void);
	static void		Shutdown (void);

	static bool		Is_Active (void);

	//	Per frame while Commander Mode is active: moves the software cursor, refreshes hover.
	static void		Update (void);
	static void		Render (void);

	static void		Get_Cursor_Screen (Vector2 &out_screen);
	static CommanderOverlayHitType	Hit_Test (const Vector2 &screen, int *out_card_index, Vector3 *out_world,
		int *out_command_index = nullptr);
	static bool		Is_Over_Panel (const Vector2 &screen);

	static void		Change_Page (int direction);
	static void		Reset_Page (void);

private:

	static float	Scale_X (void);
	static float	Scale_Y (void);
	static Vector2	To_Screen (float x, float y);
	static RectClass	To_Screen (const RectClass &design_rect);
	static Vector2	To_Design (const Vector2 &screen);

	static int		Grid_Rows (void);
	static int		Items_Per_Page (void);
	static int		Page_Count (void);
	static bool		Slot_Rect (int slot, RectClass &rect);
	static bool		Page_Button_Rect (int direction, RectClass &rect);
	static bool		Queue_Row_Rect (int row, RectClass &rect);
	static RectClass	Minimap_Rect (void);
	static RectClass	Status_Strip_Rect (void);
	static bool	Command_Button_Rect (int command, RectClass &rect);
	static bool		Is_Design_Over_Sidebar (const Vector2 &design);
	static bool		Project_To_Design (const Vector3 &world, Vector2 &out_design);

	static void		Update_Cursor (void);
	static void		Update_Hover (void);

	static void		Render_Status_Strip (void);
	static void		Render_Command_Grid (void);
	static void		Draw_Command_Icon (int command, const RectClass &button, unsigned int color);
	static void		Render_Header (void);
	static void		Render_Build_Sidebar (void);
	static void		Render_Queue_Strip (void);
	static void		Render_Minimap (void);
	static void		Render_Selection_Box (void);
	static void		Render_Health_Bars (void);
	static bool		Draw_Health_Bar (const Vector3 &world_position, float health, float vertical_offset);
	static void		Render_Tooltip (void);
	static void		Render_Cursor (void);

	static void		Draw_Panel (const RectClass &design_rect, unsigned int fill, unsigned int frame);
	static void		Draw_Text (const char *text, float x, float y, unsigned int color);
	static bool		Draw_Texture_Icon (const char *texture_name, const RectClass &design_rect, unsigned int color);

	static bool		_IsInitialized;
	static Vector2	_CursorPos;		// design space
	static int		_HoverSlot;
	static int		_HoverPageButton;
	static int		_HoverCommand;
	static int		_Page;
	static int		_PageMode;		// sidebar mode the page belongs to
};


#endif //COMMANDEROVERLAY_H
