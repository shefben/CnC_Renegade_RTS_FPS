/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderminimap.h                    $*
 *                                                                                             *
 *	V2 roadmap Phase 16 step 12 / D-001 priority 5: the renderer MinimapBaseLayer was built for.   *
 *                                                                                             *
 *	MinimapBaseLayer already summarises a generated world at the chunk registry's resolution --  *
 *	height, water, cliff band, road per cell -- and names its bases, fields, settlements, POIs,  *
 *	bridges and tunnel mouths as markers.  Nothing drew it.  This is that drawing: a panel in     *
 *	Commander Mode's bottom-left corner, one quad per cell, one square per marker, and the       *
 *	camera's focus as a cross.  Clicking it moves the Commander camera there.                     *
 *                                                                                             *
 *	`CommanderMinimap` is the model -- pure mapping between the world and the panel, and the     *
 *	colour each cell and marker is drawn in -- so the self-check can pin it without a device.    *
 *	Later overlays (logistics routes, blockages, coverage; V2 Phase 21) draw into the same       *
 *	panel through World_To_Panel.                                                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERMINIMAP_H
#define COMMANDERMINIMAP_H

#include "always.h"
#include "rect.h"
#include "rendercomponent.h"
#include "vector3.h"

class MinimapCellClass;


class CommanderMinimap
{
	public:

		//	The world rectangle the layer covers, from the chunk registry's first and last patch.
		//	False with no layer or no registry.
		static bool				Get_World_Extent(float * min_x,float * min_y,float * max_x,float * max_y);

		//	World <-> panel.  North (+Y) is up on screen.  False when there is no extent.
		static bool				World_To_Panel(const Vector3 & world,const RectClass & panel,float * x,float * y);
		static bool				Panel_To_World(float x,float y,const RectClass & panel,Vector3 * world);

		//	Colours.  Water and cliff are flat; ground is shaded by height between the layer's lowest
		//	and highest cell; a road cell is drawn as road whatever its height.
		static unsigned int	Cell_Color(const MinimapCellClass & cell,float min_height,float max_height);
		static unsigned int	Marker_Color(int marker_type);
};


class CommanderMinimapRenderer
{
	public:

		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		static void		Update(void);
		static void		Render(void);

		static IRTSOverlayComponent &	Get_Overlay_Component(void);

		//	The world point under a screen point, when it is inside the map; inside_panel as the other
		//	Commander panels report it, so a click on the frame is still swallowed.
		static bool		Hit_Test(float x,float y,Vector3 * out_world,bool * out_inside_panel);

		static bool		Is_Open(void);
};

#endif // COMMANDERMINIMAP_H
