/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderpowerpanel.h                 $*
 *                                                                                             *
 *	Roadmap Section 42's per-building power control: the panel a Commander sees after clicking   *
 *	one of their own buildings (`CommanderSelection::Get_Selected_Building`).                    *
 *                                                                                             *
 *	`CommanderPowerPanel` is the model -- what the selected building's power looks like right     *
 *	now, and what request each button would send.  It reads, never writes: a click becomes a      *
 *	`cCommanderPowerEvent` (Commando), the server applies it, and the answer comes back through   *
 *	`CommanderStateClass`'s mirror, so the panel shows the server's truth rather than the click.  *
 *                                                                                             *
 *	`CommanderPowerPanelRenderer` draws it where the command grid sits.  The two are never open  *
 *	together: a building selection and a unit selection exclude each other.                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPOWERPANEL_H
#define COMMANDERPOWERPANEL_H

#include "always.h"
#include "rendercomponent.h"
#include "wwstring.h"

enum CommanderPowerPanelButtonType
{
	POWER_PANEL_TOGGLE = 0,			// switch the building on or off by hand
	POWER_PANEL_PRIORITY_DOWN,
	POWER_PANEL_PRIORITY_UP,
	POWER_PANEL_PRIORITY_RESET,	// back to the descriptor's priority
	POWER_PANEL_BUTTON_COUNT
};

struct CommanderPowerPanelState
{
	CommanderPowerPanelState(void) : Valid(false),BuildingId(0),Team(-1),HasDescriptor(false),
												RequiresPower(false),Generation(0.0f),Consumption(0.0f),
												DescriptorPriority(0),ManualDisabled(false),HasPriorityOverride(false),
												EffectivePriority(0),Powered(false),TeamGeneration(0.0f),
												TeamRequested(0.0f),TeamAllocated(0.0f) { }

	bool			Valid;
	int			BuildingId;
	int			Team;
	StringClass	Name;

	//	From the building's descriptor.  A building with none is not on the Commander grid at all
	//	(BaseControllerClass::Power_Base still owns it), so the panel shows it and offers nothing.
	bool			HasDescriptor;
	bool			RequiresPower;
	float			Generation;
	float			Consumption;
	int			DescriptorPriority;

	//	The Commander's overrides, as the server holds them.
	bool			ManualDisabled;
	bool			HasPriorityOverride;
	int			EffectivePriority;

	bool			Powered;				// the building's replicated power state

	float			TeamGeneration;
	float			TeamRequested;
	float			TeamAllocated;
};

class CommanderPowerPanel
{
	public:

		enum { PRIORITY_STEP = 10 };

		//	The selected building's panel, or an invalid state when nothing (live, own-team) is
		//	selected.
		static bool		Build_State(int building_id,CommanderPowerPanelState * out);

		/*
		**	What a button asks the server for, as a (cCommanderPowerEvent action, value) pair --
		**	0 = set enabled, 1 = set priority, 2 = clear priority, the event's own order.  False
		**	when the button means nothing for this building (no descriptor, or reset with no
		**	override to clear).  Pure: the state is the whole input.
		*/
		static bool		Get_Request(CommanderPowerPanelButtonType button,const CommanderPowerPanelState & state,
											 int * out_action,int * out_value);

		static const char *	Get_Button_Label(CommanderPowerPanelButtonType button,
														  const CommanderPowerPanelState & state);
};

class CommanderPowerPanelRenderer
{
	public:

		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		static void		Update(void);
		static void		Render(void);

		//	This panel as Section 29's RTS overlay component (D-001 P26); the HUD draws it
		//	through the interface.
		static IRTSOverlayComponent &	Get_Overlay_Component(void);

		//	Same contract as CommanderCommandPaletteRenderer::Hit_Test: a point inside the panel
		//	but between buttons reports inside and no button, and must still swallow the click.
		static bool		Hit_Test(float x,float y,CommanderPowerPanelButtonType * out_button,
										bool * out_inside_panel);

		static bool		Is_Open(void);
};

#endif // COMMANDERPOWERPANEL_H
