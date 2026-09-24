/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors
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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *	Roadmap Section 29, Zero Hour / SAGE feature 18 -- modular draw-component architecture.     *
 *                                                                                             *
 *	The rule this file exists to satisfy: "New rendering behavior can be attached without       *
 *	continuously expanding every GameObj subclass."  Donor concept is W3DModuleFactory /         *
 *	DrawModule -- a small set of per-behavior contracts a piece of rendering-adjacent logic      *
 *	implements, instead of every new visual feature adding another virtual to GameObj and        *
 *	another override to every subclass that wants it.  This is contracts only: nothing here      *
 *	owns storage, a registry, or a dispatch loop, because OpenW3D GameObj stays GameObj -- the    *
 *	roadmap is explicit that this must not become the SAGE Thing hierarchy.  A conforming class  *
 *	keeps whatever base class it already has and adds one of these as extra inheritance, the way *
 *	TracerSystemClass and DebrisSystemClass (Code/Combat/bullet.cpp, debrissystem.cpp) already    *
 *	do for IRenderComponent.                                                                     *
 *                                                                                             *
 *	Five interfaces, five call sites a future implementer wires into:                            *
 *                                                                                             *
 *	IRenderComponent            -- a per-frame FPS-world render, the same shape as               *
 *	                                RenderObjClass::Render, called wherever the implementer is    *
 *	                                already reached (CombatManager's scene render pass, a pooled  *
 *	                                system's own Render override, etc).                          *
 *	IClientUpdateComponent       -- a per-frame client-side tick with no gameplay authority,      *
 *	                                called from CombatManager::Think() the same way              *
 *	                                BulletManager::Update / DebrisManager::Update already are.    *
 *	IRTSOverlayComponent          -- a Commander/RTS-mode overlay draw, gated on the sidebar's     *
 *	                                current mode, layered over but never mixed with FPS render     *
 *	                                state.  No caller exists yet -- the Commander sidebar          *
 *	                                (roadmap dependency chain, later than this phase) is what      *
 *	                                will hold the mode switch this checks against.                *
 *	ISelectionVisualComponent      -- Commander-side "this is selected" visual state; the          *
 *	                                selection/status renderer named in Section 29's Initial        *
 *	                                components list implements this once it exists.                *
 *	IWorldInteractionVisualComponent -- ghost placement, waypoint/order markers and other           *
 *	                                world-space interaction feedback the Commander layer draws     *
 *	                                into the FPS world; the ghost-building and waypoint/order       *
 *	                                renderers named in the same list implement this.                *
 *                                                                                             *
 *	Implementers (D-001 P26, once each consumer existed): IClientUpdateComponent --                *
 *	CommanderSelectionOverlays' ring manager; IRTSOverlayComponent -- the command palette, the      *
 *	power panel and the governance panel, drawn by the HUD through the interface;                  *
 *	ISelectionVisualComponent -- each selected object's status ring; IWorldInteractionVisual-      *
 *	Component -- the placement ghost's valid/invalid tint.                                          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef RENDERCOMPONENT_H
#define RENDERCOMPONENT_H

class RenderInfoClass;


//	A per-frame FPS-world render.  Signature matches RenderObjClass::Render so a class already
//	rendering itself that way can implement this with a one-line forward.
class IRenderComponent
{
	public:
		virtual ~IRenderComponent(void)								{ }
		virtual void	Render_Component(RenderInfoClass & rinfo) = 0;
};


//	A per-frame client-side tick.  No return value, no gameplay authority implied -- this is for
//	the same class of work BulletManager::Update/DebrisManager::Update already do every frame.
class IClientUpdateComponent
{
	public:
		virtual ~IClientUpdateComponent(void)							{ }
		virtual void	Update_Component(float seconds) = 0;
};


//	A Commander/RTS-mode overlay.  Drawn only while the sidebar is in a mode that wants it --
//	Is_RTS_Overlay_Visible is the gate a future caller checks before Render_RTS_Overlay runs.
class IRTSOverlayComponent
{
	public:
		virtual ~IRTSOverlayComponent(void)							{ }
		virtual void	Render_RTS_Overlay(RenderInfoClass & rinfo) = 0;
		virtual bool	Is_RTS_Overlay_Visible(void) const = 0;
};


//	Commander-side "this object is selected" visual state -- a highlight, a status ring, a
//	health bar over the object.  Set_Selected is the order; Is_Selected lets a caller batch by
//	current state instead of re-deriving it.
class ISelectionVisualComponent
{
	public:
		virtual ~ISelectionVisualComponent(void)						{ }
		virtual void	Set_Selected(bool onoff) = 0;
		virtual bool	Is_Selected(void) const = 0;
};


//	World-space interaction feedback that is not the object's own body -- a ghost-placement
//	silhouette, a waypoint/order marker, a valid/invalid placement tint.  One boolean because
//	every implementer so far is a binary highlight; a component that needs more than on/off
//	is not this interface.
class IWorldInteractionVisualComponent
{
	public:
		virtual ~IWorldInteractionVisualComponent(void)				{ }
		virtual void	Set_Interaction_Highlight(bool onoff) = 0;
};

#endif // RENDERCOMPONENT_H
