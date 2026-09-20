/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderwaypoint.cpp                 $*
 *                                                                                             *
 *	Roadmap Sections 3A and 45.  See commanderwaypoint.h.                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderwaypoint.h"

#include "radar.h"

#include <string.h>


/*
**	Radar marker ids for the waypoints.  RadarManager keys its markers by an int the caller
**	chooses, so the Commander layer reserves a small block well clear of anything a mission
**	script would pick -- scripts use small numbers and objective ids.
*/
static const int	RADAR_MARKER_BASE = 0x0C000000;

static const int	SLOT_COUNT = CommanderWaypointSystem::MAX_TEAMS * CommanderWaypointSystem::MAX_PER_TEAM;

static CommanderWaypointStruct	_Slots [SLOT_COUNT];
static int								_SetCount		= 0;
static int								_ExpiredCount	= 0;


static const char * const	_KindNames [WAYPOINT_KIND_COUNT] =
{
	"MOVE",
	"ATTACK",
	"DEFEND",
	"RALLY",
};


//	Waypoints are drawn in the Commander layer's own yellow whatever the team, because the
//	marker says "your Commander asked for this", not "this is a Nod thing".
static int	Radar_Color_For_Kind(CommanderWaypointKindType kind)
{
	switch (kind) {
		case WAYPOINT_ATTACK:	return RadarManager::BLIP_COLOR_TYPE_PRIMARY_OBJECTIVE;
		case WAYPOINT_DEFEND:	return RadarManager::BLIP_COLOR_TYPE_SECONDARY_OBJECTIVE;
		default:						return RadarManager::BLIP_COLOR_TYPE_TERTIARY_OBJECTIVE;
	}
}


static void	Clear_Slot(int index)
{
	if (_Slots [index].Team < 0) return;

	RadarManager::Clear_Marker (RADAR_MARKER_BASE + index);
	_Slots [index] = CommanderWaypointStruct ();
}


static void	Publish_Slot(int index)
{
	RadarMarkerClass marker;
	marker.ID			= RADAR_MARKER_BASE + index;
	marker.Type			= 0;
	marker.Color		= Radar_Color_For_Kind (_Slots [index].Kind);
	marker.Position	= _Slots [index].Position;
	marker.Intensity	= 1.0f;

	//	Cleared first: Add_Marker appends without checking, so re-publishing a slot that already
	//	had a marker would leave the old one behind for the rest of the match.
	RadarManager::Clear_Marker (marker.ID);
	RadarManager::Add_Marker (marker);
}


void CommanderWaypointSystem::Init(void)
{
	for (int i = 0; i < SLOT_COUNT; i++) {
		_Slots [i] = CommanderWaypointStruct ();
	}
	_SetCount		= 0;
	_ExpiredCount	= 0;
}


void CommanderWaypointSystem::Shutdown(void)
{
	for (int i = 0; i < SLOT_COUNT; i++) {
		Clear_Slot (i);
	}
}


static int	Team_Slot_Base(int team)
{
	//	Teams are player-type constants, not 0..3, so they are folded rather than indexed
	//	directly.  A team outside the range still lands in a valid block instead of writing
	//	off the end -- a waypoint attributed to the wrong block is a cosmetic fault; an
	//	out-of-bounds write is not.
	int index = team;
	if (index < 0) index = 0;
	index = index % CommanderWaypointSystem::MAX_TEAMS;
	return index * CommanderWaypointSystem::MAX_PER_TEAM;
}


void CommanderWaypointSystem::Set_Waypoint(int team,CommanderWaypointKindType kind,
														 const Vector3 & position,float lifetime_seconds,
														 int sequence)
{
	if ((kind < 0) || (kind >= WAYPOINT_KIND_COUNT)) return;
	if (lifetime_seconds <= 0.0f) return;

	int base = Team_Slot_Base (team);

	/*
	**	Pick a slot: a free one first, otherwise the one with the least life left.  Replacing the
	**	oldest rather than refusing means the most recent order a Commander gave is always the one
	**	on screen, which is what a player looking at the marker needs it to be.
	*/
	int chosen = -1;
	float worst = 0.0f;

	for (int i = 0; i < MAX_PER_TEAM; i++) {
		int slot = base + i;
		if (_Slots [slot].Team < 0) {
			chosen = slot;
			break;
		}
		if ((chosen < 0) || (_Slots [slot].SecondsLeft < worst)) {
			chosen	= slot;
			worst		= _Slots [slot].SecondsLeft;
		}
	}

	if (chosen < 0) return;

	_Slots [chosen].Team			= team;
	_Slots [chosen].Kind			= kind;
	_Slots [chosen].Position	= position;
	_Slots [chosen].SecondsLeft	= lifetime_seconds;
	_Slots [chosen].Sequence	= sequence;

	Publish_Slot (chosen);
	_SetCount++;
}


void CommanderWaypointSystem::Clear_Team(int team)
{
	int base = Team_Slot_Base (team);
	for (int i = 0; i < MAX_PER_TEAM; i++) {
		Clear_Slot (base + i);
	}
}


void CommanderWaypointSystem::Update(float seconds)
{
	if (seconds <= 0.0f) return;

	for (int i = 0; i < SLOT_COUNT; i++) {

		if (_Slots [i].Team < 0) continue;

		_Slots [i].SecondsLeft -= seconds;
		if (_Slots [i].SecondsLeft > 0.0f) continue;

		Clear_Slot (i);
		_ExpiredCount++;
	}
}


int CommanderWaypointSystem::Get_Count(void)
{
	int count = 0;
	for (int i = 0; i < SLOT_COUNT; i++) {
		if (_Slots [i].Team >= 0) count++;
	}
	return count;
}


const CommanderWaypointStruct &	CommanderWaypointSystem::Get_Waypoint(int index)
{
	static CommanderWaypointStruct empty;
	if ((index < 0) || (index >= SLOT_COUNT)) return empty;
	return _Slots [index];
}


int CommanderWaypointSystem::Get_Team_Count(int team)
{
	int base = Team_Slot_Base (team);
	int count = 0;
	for (int i = 0; i < MAX_PER_TEAM; i++) {
		if (_Slots [base + i].Team == team) count++;
	}
	return count;
}


const char * CommanderWaypointSystem::Get_Kind_Name(CommanderWaypointKindType kind)
{
	if ((kind < 0) || (kind >= WAYPOINT_KIND_COUNT)) return "UNKNOWN";
	return _KindNames [kind];
}


int CommanderWaypointSystem::Get_Set_Count(void)		{ return _SetCount; }
int CommanderWaypointSystem::Get_Expired_Count(void)	{ return _ExpiredCount; }
