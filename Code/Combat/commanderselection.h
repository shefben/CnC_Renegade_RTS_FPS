/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderselection.h                  $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39 -- what the Commander currently has selected.                  *
 *                                                                                             *
 *	The order half of Section 45 has been complete for some time: `cCommanderOrderEvent` carries *
 *	a list of unit ids, the server validates every one of them, and `Dispatch_Order` turns an     *
 *	accepted order into a real `ActionClass` call.  What never existed was the list -- nothing     *
 *	in the tree could produce a selection, so the only caller was a console command naming one     *
 *	id at a time.  This is that list.                                                             *
 *                                                                                             *
 *	It is a model in the same sense `CommanderSidebarModel` is: no device, no camera, no screen   *
 *	coordinates and no input.  The caller hands it world-space work already done -- "these ids    *
 *	are inside the box" -- and it owns membership, staleness and the ordering rules.  That split  *
 *	is what lets the selection's own rules (buildings are never selectable, a dead unit leaves    *
 *	the selection the frame it dies, the list is bounded by what a packet can carry) be checked   *
 *	with no device under them.                                                                    *
 *                                                                                             *
 *	Buildings are excluded structurally rather than by a test: the only thing that ever adds to   *
 *	this list walks `GameObjManager::Get_Smart_Game_Obj_List`, and `BuildingGameObj` derives from *
 *	`DamageableGameObj`, not `SmartGameObj`, so a building cannot reach it.  Human-driven units   *
 *	*are* selectable on purpose -- a Commander must be able to see and count their own team --     *
 *	and Section 3A.4's rule that a human's unit refuses Commander orders is enforced where it      *
 *	belongs, server-side in `cCommanderOrderEvent::Act`, not by hiding the unit from the box.      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERSELECTION_H
#define COMMANDERSELECTION_H

#include "always.h"
#include "vector2.h"

class SmartGameObj;

class CommanderSelection
{
	public:

		enum
		{
			//	Matches `cCommanderOrderEvent::MAX_UNITS` exactly.  A selection that could not fit
			//	in one order packet would silently drop its tail at send time, which is a worse
			//	failure than refusing to grow: the player would see units selected that never got
			//	the order.  Kept as its own constant rather than including the network header,
			//	because this class has no business knowing a packet exists.
			MAX_SELECTED = 64,

			//	Control groups 1..9, the digit keys.  Group 0 is not used, so a group number is
			//	the digit the player pressed.
			MAX_CONTROL_GROUPS = 10,
		};

		static void		Init(void);
		static void		Shutdown(void);

		static void		Clear(void);
		static bool		Add(int unit_id);
		static bool		Contains(int unit_id);
		static int		Get_Count(void);
		static int		Get_Unit_Id(int index);

		//	Copies the live selection into a caller's buffer, dropping anything that has died
		//	since.  Returns how many were written.  This is what an order event is filled from.
		static int		Get_Unit_Ids(int * out_ids,int max_ids);

		/*
		**	Drops anything that is gone: destroyed, delete-pending, or no longer on the selecting
		**	team.  Called once a frame by whatever owns the selection, so the count the player
		**	sees is the count an order would actually reach.
		*/
		static void		Prune(void);

		//	The team the selection belongs to.  Set by the first Add; a unit on another team is
		//	refused rather than silently switching the selection's allegiance mid-drag.
		static int		Get_Team(void);
		static void		Set_Team(int team);

		/*
		**	The drag rectangle, in the same screen space `Render2DClass::Get_Screen_Resolution`
		**	reports -- held here rather than in the input file so the renderer can draw it without
		**	depending on Commando.  Begin/Update/End are the whole protocol; `Get_Drag_Rect`
		**	normalises the corners, so a box dragged up-left reads the same as one dragged
		**	down-right.
		*/
		static void		Begin_Drag(const Vector2 & anchor);
		static void		Update_Drag(const Vector2 & corner);
		static void		End_Drag(void);
		static bool		Is_Dragging(void);
		static void		Get_Drag_Rect(Vector2 * out_min,Vector2 * out_max);

		//	True once the pointer has moved far enough from the anchor for this to be a box drag
		//	rather than a click that happened to wobble.  A click and a drag must not both fire.
		static bool		Is_Drag_Significant(void);

		static int		Get_Selection_Change_Count(void);

		/*
		**	The selected building -- one at a time, and exclusive with the unit selection: a
		**	building is picked by clicking it (`CommanderUIInput`), which clears the units, and
		**	selecting units clears it.  This is what the power panel and the selection overlay
		**	read.  Held as a network id for the same reason units are.  Prune drops it once the
		**	building is gone or destroyed.
		*/
		static void		Select_Building(int building_id);
		static int		Get_Selected_Building(void);
		static void		Clear_Selected_Building(void);

		/*
		**	Control groups: local client state, never replicated -- an order issued to a recalled
		**	group travels as an ordinary order.  Assign copies the current unit selection into
		**	the group (an empty selection empties it); Recall replaces the selection with the
		**	group's members and returns how many it restored.  Prune removes dead members from
		**	every group as well as from the selection.
		*/
		static bool		Assign_Group(int group);
		static int		Recall_Group(int group);
		static int		Get_Group_Count(int group);
		static int		Get_Group_Unit_Id(int group,int index);
		static void		Clear_Groups(void);
};

#endif // COMMANDERSELECTION_H
