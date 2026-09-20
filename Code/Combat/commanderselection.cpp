/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderselection.cpp                $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39.  See commanderselection.h.                                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderselection.h"

#include "damage.h"
#include "gameobjmanager.h"
#include "playertype.h"
#include "smartgameobj.h"
#include "wwmath.h"

#include <string.h>


/*
**	A drag shorter than this many pixels on both axes is a click, not a box.  Four pixels is
**	roughly the slop a hand puts into a deliberate click; anything tighter and a normal click
**	on uneven ground would sometimes clear the selection instead of ordering it.
*/
static const float	DRAG_SIGNIFICANT_PIXELS = 4.0f;


static int		_Ids [CommanderSelection::MAX_SELECTED];
static int		_Count			= 0;
static int		_Team				= PLAYERTYPE_NEUTRAL;
static int		_ChangeCount	= 0;

static bool		_Dragging		= false;
static Vector2	_DragAnchor (0.0f,0.0f);
static Vector2	_DragCorner (0.0f,0.0f);


void CommanderSelection::Init(void)
{
	Clear ();
	_ChangeCount	= 0;
	_Dragging		= false;
}


void CommanderSelection::Shutdown(void)
{
	Clear ();
	_Dragging = false;
}


void CommanderSelection::Clear(void)
{
	if (_Count != 0) {
		_ChangeCount++;
	}
	::memset (_Ids,0,sizeof (_Ids));
	_Count	= 0;
	_Team		= PLAYERTYPE_NEUTRAL;
}


bool CommanderSelection::Contains(int unit_id)
{
	for (int i = 0; i < _Count; i++) {
		if (_Ids [i] == unit_id) return true;
	}
	return false;
}


bool CommanderSelection::Add(int unit_id)
{
	if (unit_id == 0) return false;
	if (_Count >= MAX_SELECTED) return false;
	if (Contains (unit_id)) return true;			// already in: not a failure

	_Ids [_Count++] = unit_id;
	_ChangeCount++;
	return true;
}


int CommanderSelection::Get_Count(void)		{ return _Count; }


int CommanderSelection::Get_Unit_Id(int index)
{
	if ((index < 0) || (index >= _Count)) return 0;
	return _Ids [index];
}


int CommanderSelection::Get_Unit_Ids(int * out_ids,int max_ids)
{
	if ((out_ids == nullptr) || (max_ids <= 0)) return 0;

	int written = 0;
	for (int i = 0; (i < _Count) && (written < max_ids); i++) {
		if (_Ids [i] == 0) continue;
		out_ids [written++] = _Ids [i];
	}
	return written;
}


int	CommanderSelection::Get_Team(void)				{ return _Team; }
void	CommanderSelection::Set_Team(int team)		{ _Team = team; }


/***********************************************************************************************
 * CommanderSelection::Prune -- drop what is no longer there to order                            *
 *                                                                                               *
 *	Walked backwards so a removal cannot skip the next entry.  Three ways out of the selection:   *
 *	the object is gone from the manager entirely, it is queued for deletion this frame, or its    *
 *	defense object says it is dead.  The last one matters because a corpse lives for a while      *
 *	after it stops being a unit, and ordering it would be a silent no-op the player could not     *
 *	see the reason for.                                                                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void CommanderSelection::Prune(void)
{
	for (int i = _Count - 1; i >= 0; i--) {

		bool keep = false;

		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (_Ids [i]);
		if ((unit != nullptr) && !unit->Is_Delete_Pending ()) {

			DamageableGameObj * damageable = (DamageableGameObj *)unit;
			if ((damageable->Get_Defense_Object () != nullptr) &&
				 (damageable->Get_Defense_Object ()->Get_Health () > 0.0f)) {
				keep = true;
			}
		}

		if (keep) continue;

		for (int shift = i; shift < (_Count - 1); shift++) {
			_Ids [shift] = _Ids [shift + 1];
		}
		_Count--;
		_Ids [_Count] = 0;
		_ChangeCount++;
	}

	if (_Count == 0) {
		_Team = PLAYERTYPE_NEUTRAL;
	}
}


void CommanderSelection::Begin_Drag(const Vector2 & anchor)
{
	_Dragging	= true;
	_DragAnchor	= anchor;
	_DragCorner	= anchor;
}


void CommanderSelection::Update_Drag(const Vector2 & corner)
{
	if (!_Dragging) return;
	_DragCorner = corner;
}


void CommanderSelection::End_Drag(void)		{ _Dragging = false; }
bool CommanderSelection::Is_Dragging(void)	{ return _Dragging; }


void CommanderSelection::Get_Drag_Rect(Vector2 * out_min,Vector2 * out_max)
{
	if (out_min != nullptr) {
		out_min->X = WWMath::Min (_DragAnchor.X,_DragCorner.X);
		out_min->Y = WWMath::Min (_DragAnchor.Y,_DragCorner.Y);
	}
	if (out_max != nullptr) {
		out_max->X = WWMath::Max (_DragAnchor.X,_DragCorner.X);
		out_max->Y = WWMath::Max (_DragAnchor.Y,_DragCorner.Y);
	}
}


bool CommanderSelection::Is_Drag_Significant(void)
{
	if (!_Dragging) return false;

	float width  = WWMath::Fabs (_DragCorner.X - _DragAnchor.X);
	float height = WWMath::Fabs (_DragCorner.Y - _DragAnchor.Y);

	//	Either axis is enough: a long thin box down a road is a deliberate drag even though one
	//	of its sides is only a few pixels.
	return (width >= DRAG_SIGNIFICANT_PIXELS) || (height >= DRAG_SIGNIFICANT_PIXELS);
}


int CommanderSelection::Get_Selection_Change_Count(void)	{ return _ChangeCount; }
