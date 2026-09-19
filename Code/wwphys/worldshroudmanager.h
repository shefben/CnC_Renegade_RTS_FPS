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
 *                                                                                             *
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/worldshroudmanager.h                  $*
 *                                                                                             *
 *	Roadmap Section 30, Zero Hour / SAGE feature 15 -- RTS shroud / fog of war.                  *
 *                                                                                             *
 *	The section's own rule is the shape of this file: "do not black out the physical            *
 *	first-person world because the Commander intelligence layer has not explored it."  So       *
 *	this is not a visibility system an FPS renderer or the physics scene consults -- nothing    *
 *	in this class can hide geometry, suppress a hit test or change what a soldier can shoot.     *
 *	It is a per-team strategic memory grid that only Commander/tactical callers (the radar       *
 *	model, Section 31, and later the Commander map) ever read, and that is provable here by      *
 *	construction: no physics, rendering or targeting header is included, and no query takes      *
 *	anything but a team id and a world point.                                                    *
 *                                                                                             *
 *	The five states are a per-cell, per-team state machine, not a per-object one.  UNKNOWN is    *
 *	a cell a team's sensors have never reached; EXPLORED is permanent terrain memory once a      *
 *	cell has ever been covered; VISIBLE and RADAR_DETECTED are this tick's live coverage;        *
 *	LAST_KNOWN is what a cell falls back to the instant live coverage is lost, decaying to       *
 *	EXPLORED after its retention window.  Because the record is the cell and not a handle to     *
 *	whatever was standing there, a LAST_KNOWN cell cannot be tracking a live transform -- the     *
 *	roadmap's own ban -- even by accident.                                                       *
 *                                                                                             *
 *	Coverage comes only from RegisterStrategicSensorSource and its two companions.  A camera     *
 *	has no privileged path into this file; the only way anything becomes visible is a            *
 *	registered, online sensor source with a team, a position and a radius.  A Commander camera   *
 *	that wants to reveal something would have to register itself as a source through this same   *
 *	API as any other caller, which is precisely what Section 30 forbids it from doing.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WORLDSHROUDMANAGER_H
#define WORLDSHROUDMANAGER_H

#include "always.h"
#include "vector.h"
#include "vector3.h"


/*
**	Cell state, from a team's point of view.  Ordered by how much a caller can trust it: an
**	UNKNOWN cell is a black map tile, EXPLORED is remembered terrain with nothing live on it,
**	LAST_KNOWN is remembered terrain that *did* have live coverage recently, and VISIBLE /
**	RADAR_DETECTED are this tick's truth.
*/
enum ShroudStateType
{
	SHROUD_UNKNOWN			= 0,
	SHROUD_EXPLORED,
	SHROUD_LAST_KNOWN,
	SHROUD_RADAR_DETECTED,
	SHROUD_VISIBLE,
};

/*
**	How many teams may hold independent shroud memory at once, how many sensor sources may be
**	registered at once, and how large the strategic grid may grow.  All three refuse rather
**	than grow past these -- the same discipline as WorldLightManager next door.
*/
enum
{
	WORLD_SHROUD_MAX_TEAMS				= 8,
	WORLD_SHROUD_MAX_SENSOR_SOURCES	= 256,
	WORLD_SHROUD_MAX_GRID_CELLS		= 65536,
};

/*
**	A registration handle.  Zero is never issued, so it doubles as "no source" / "refused".
*/
typedef int ShroudSensorHandleType;


class WorldShroudManager
{
public:

	static void		Init(void);
	static void		Shutdown(void);

	/*
	**	The world went away.  Every team's grid and every sensor source's bucketing is dropped;
	**	registrations themselves are not (a building that is still standing should not have to
	**	re-register itself across a map change it does not know happened), so the next Timestep
	**	re-partitions and re-stamps from the still-registered sources.
	*/
	static void		Release_Resources(void);

	static void		Re_Partition(const Vector3 & world_min,const Vector3 & world_max);
	static bool		Is_Partitioned(void);

	/*
	**	The sensor-source API named by Section 30, verbatim.  Communications buildings (Phase
	**	34) and every unit with a vision radius are meant to call this instead of any building-
	**	or unit-specific shroud logic being invented later.
	**
	**	last_known_retention_seconds is how long a cell this source stopped covering keeps its
	**	LAST_KNOWN state before fading to EXPLORED; <= 0 uses the manager default.
	*/
	static ShroudSensorHandleType	RegisterStrategicSensorSource(int team,const Vector3 & world_transform,
																						float vision_radius,float radar_radius,
																						float last_known_retention_seconds,bool online);
	static bool			UpdateStrategicSensorSource(ShroudSensorHandleType handle,const Vector3 & world_transform,
																	float vision_radius,float radar_radius,bool online);
	static bool			UnregisterStrategicSensorSource(ShroudSensorHandleType handle);
	static bool			Is_Sensor_Source(ShroudSensorHandleType handle);
	static int			Get_Sensor_Source_Count(void);

	/*
	**	Recomputes coverage from every online source and ages LAST_KNOWN retention.  Driven from
	**	PhysicsSceneClass::Update, the same place WorldLightManager is driven from.  Nothing
	**	about camera position is an input to this function or to anything it calls.
	*/
	static void			Timestep(float dt);

	/*
	**	Queries.  Every one is safe with no partition and no team registered: the answer is
	**	SHROUD_UNKNOWN / false, never a crash, matching every other missing-data case in this
	**	project.
	*/
	static ShroudStateType	Get_Shroud_State(int team,const Vector3 & world_point);
	static bool			Is_Visible(int team,const Vector3 & world_point);
	static bool			Is_Radar_Detected(int team,const Vector3 & world_point);
	static bool			Is_Explored(int team,const Vector3 & world_point);

	/*
	**	Accounting, for the self check and for a later debug overlay (Section 33 names "shroud
	**	cells" explicitly).
	*/
	static int			Get_Team_Count(void);
	static int			Get_Cell_Count(void);
	static int			Get_Visible_Cell_Count(int team);
	static int			Get_Explored_Cell_Count(int team);
	static int			Get_Refusal_Count(void);

private:

	enum { SHROUD_CELL_TRANSIENT_NONE = 0 };	// used inside ShroudCellClass::Transient only

	struct ShroudCellClass
	{
		ShroudCellClass(void) : Explored(false),Transient(SHROUD_CELL_TRANSIENT_NONE),
										 LastKnownRemaining(0.0f),Retention(0.0f) { }
		bool			Explored;
		unsigned char	Transient;			// 0 = none, else a ShroudStateType value for this tick
		float			LastKnownRemaining;	// counts down while Transient == SHROUD_LAST_KNOWN
		float			Retention;				// captured from whichever source covered this cell last,
														// so the retention used on drop-out does not need the
														// (by then stale) per-tick scratch buffers
	};

	struct SensorSourceRecord
	{
		SensorSourceRecord(void) : InUse(false),Team(0),Position(0,0,0),VisionRadius(0.0f),
											RadarRadius(0.0f),LastKnownRetention(0.0f),Online(false) { }
		bool		InUse;
		int			Team;
		Vector3		Position;
		float		VisionRadius;
		float		RadarRadius;
		float		LastKnownRetention;
		bool		Online;

		//	Stubs so this can live in a DynamicVectorClass; records are addressed by index
		//	(the handle), never searched for by value.
		bool	operator == (const SensorSourceRecord &) const	{ return false; }
		bool	operator != (const SensorSourceRecord &) const	{ return true; }
	};

	struct TeamGridClass
	{
		TeamGridClass(void) : Team(0),Cells(nullptr),CellCount(0) { }
		int					Team;
		ShroudCellClass *	Cells;
		int					CellCount;

		bool	operator == (const TeamGridClass &) const	{ return false; }
		bool	operator != (const TeamGridClass &) const	{ return true; }
	};

	static int		Find_Team_Slot(int team);
	static int		Make_Team_Slot(int team);
	static int		Distinct_Source_Team_Count(void);
	static bool		World_To_Cell(float x,float y,int * col,int * row);
	static void		Stamp_Circle(bool * coverage,float cx,float cy,float radius);

	static bool		Initialized;
	static bool		Partitioned;

	static Vector3	WorldMin;
	static Vector3	WorldMax;
	static float	CellSize;
	static int		Cols;
	static int		Rows;

	static float	DefaultRetention;

	static DynamicVectorClass<SensorSourceRecord>	Sources;
	static DynamicVectorClass<TeamGridClass>			Teams;

	static bool *	VisionScratch;
	static bool *	RadarScratch;
	static float *	RetentionScratch;
	static int		ScratchCellCount;

	static int		RefusalCount;
};

#endif	// WORLDSHROUDMANAGER_H
