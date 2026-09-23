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
 *                     $Archive:: /Commando/Code/wwphys/worldshroudmanager.cpp                $*
 *                                                                                             *
 *	Roadmap Section 30, Zero Hour / SAGE feature 15 -- RTS shroud / fog of war.                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldshroudmanager.h"

#include "wwmath.h"

#include <string.h>


/*
**	How coarse a shroud cell is.  This is a strategic-map resolution, not a gameplay one -- an
**	order of magnitude coarser than WorldLightManager's cells -- because the grid this stamps is
**	bounded at WORLD_SHROUD_MAX_GRID_CELLS regardless of map size, and a Commander map does not
**	need per-footstep granularity.
*/
static const float	WORLD_SHROUD_CELL_SIZE			= 500.0f;

/*
**	How long a cell keeps LAST_KNOWN state after every source that covered it stops covering it,
**	when the source did not say otherwise.
*/
static const float	WORLD_SHROUD_DEFAULT_RETENTION	= 10.0f;


bool				WorldShroudManager::Initialized		= false;
bool				WorldShroudManager::Partitioned		= false;

Vector3			WorldShroudManager::WorldMin(0,0,0);
Vector3			WorldShroudManager::WorldMax(0,0,0);
float				WorldShroudManager::CellSize			= WORLD_SHROUD_CELL_SIZE;
int				WorldShroudManager::Cols				= 0;
int				WorldShroudManager::Rows				= 0;

float				WorldShroudManager::DefaultRetention= WORLD_SHROUD_DEFAULT_RETENTION;

DynamicVectorClass<WorldShroudManager::SensorSourceRecord>	WorldShroudManager::Sources;
DynamicVectorClass<WorldShroudManager::TeamGridClass>			WorldShroudManager::Teams;

bool *			WorldShroudManager::VisionScratch		= nullptr;
bool *			WorldShroudManager::RadarScratch		= nullptr;
float *			WorldShroudManager::RetentionScratch	= nullptr;
int				WorldShroudManager::ScratchCellCount	= 0;

int				WorldShroudManager::RefusalCount		= 0;


/***********************************************************************************************
 * WorldShroudManager::Init -- bring the service up                                             *
 *=============================================================================================*/
void WorldShroudManager::Init(void)
{
	if (Initialized) {
		Shutdown();
	}

	Partitioned = false;
	WorldMin.Set(0,0,0);
	WorldMax.Set(0,0,0);
	CellSize = WORLD_SHROUD_CELL_SIZE;
	Cols = Rows = 0;
	DefaultRetention = WORLD_SHROUD_DEFAULT_RETENTION;

	Sources.Delete_All();
	Teams.Delete_All();

	VisionScratch = RadarScratch = nullptr;
	RetentionScratch = nullptr;
	ScratchCellCount = 0;

	RefusalCount = 0;

	Initialized = true;
}


/***********************************************************************************************
 * WorldShroudManager::Shutdown -- take the service down                                        *
 *=============================================================================================*/
void WorldShroudManager::Shutdown(void)
{
	if (!Initialized) {
		return;
	}
	Release_Resources();
	Sources.Delete_All();
	Initialized = false;
}


/***********************************************************************************************
 * WorldShroudManager::Release_Resources -- the world went away; grids go with it, sources don't *
 *=============================================================================================*/
void WorldShroudManager::Release_Resources(void)
{
	for (int i = 0; i < Teams.Count(); i++) {
		delete [] Teams[i].Cells;
	}
	Teams.Delete_All();

	delete [] VisionScratch;		VisionScratch = nullptr;
	delete [] RadarScratch;		RadarScratch = nullptr;
	delete [] RetentionScratch;	RetentionScratch = nullptr;
	ScratchCellCount = 0;

	Partitioned = false;
	Cols = Rows = 0;
}


/***********************************************************************************************
 * WorldShroudManager::Re_Partition -- say where the world is                                    *
 *=============================================================================================*/
void WorldShroudManager::Re_Partition(const Vector3 & world_min,const Vector3 & world_max)
{
	if (!Initialized) {
		return;
	}

	WorldMin = world_min;
	WorldMax = world_max;

	float extent_x = WWMath::Max(world_max.X - world_min.X,1.0f);
	float extent_y = WWMath::Max(world_max.Y - world_min.Y,1.0f);

	CellSize = WORLD_SHROUD_CELL_SIZE;
	int cols = (int)((extent_x / CellSize) + 1.0f);
	int rows = (int)((extent_y / CellSize) + 1.0f);
	cols = (cols < 1) ? 1 : cols;
	rows = (rows < 1) ? 1 : rows;

	//	The grid refuses to grow past its bound; a map big enough to need it gets coarser cells
	//	instead of unbounded memory.
	while ((cols * rows) > WORLD_SHROUD_MAX_GRID_CELLS) {
		CellSize *= 1.5f;
		cols = (int)((extent_x / CellSize) + 1.0f);
		rows = (int)((extent_y / CellSize) + 1.0f);
		cols = (cols < 1) ? 1 : cols;
		rows = (rows < 1) ? 1 : rows;
	}

	Cols = cols;
	Rows = rows;

	//	Every existing team grid was sized for the previous extent; drop them all and let
	//	Timestep rebuild them lazily from the still-registered sources.
	for (int i = 0; i < Teams.Count(); i++) {
		delete [] Teams[i].Cells;
	}
	Teams.Delete_All();

	delete [] VisionScratch;		VisionScratch = nullptr;
	delete [] RadarScratch;		RadarScratch = nullptr;
	delete [] RetentionScratch;	RetentionScratch = nullptr;
	ScratchCellCount = 0;

	Partitioned = true;
}


bool WorldShroudManager::Is_Partitioned(void)
{
	return Partitioned;
}


/***********************************************************************************************
 * WorldShroudManager::Distinct_Source_Team_Count -- how many different teams have a source      *
 *=============================================================================================*/
int WorldShroudManager::Distinct_Source_Team_Count(void)
{
	int seen[WORLD_SHROUD_MAX_TEAMS];
	int seen_count = 0;

	for (int i = 0; i < Sources.Count(); i++) {
		if (!Sources[i].InUse) {
			continue;
		}
		bool found = false;
		for (int s = 0; s < seen_count; s++) {
			if (seen[s] == Sources[i].Team) {
				found = true;
				break;
			}
		}
		if (!found && seen_count < WORLD_SHROUD_MAX_TEAMS) {
			seen[seen_count++] = Sources[i].Team;
		}
	}
	return seen_count;
}


/***********************************************************************************************
 * WorldShroudManager::RegisterStrategicSensorSource -- add a source of team visibility          *
 *=============================================================================================*/
ShroudSensorHandleType WorldShroudManager::RegisterStrategicSensorSource(int team,const Vector3 & world_transform,
																								float vision_radius,float radar_radius,
																								float last_known_retention_seconds,bool online)
{
	if (!Initialized) {
		return 0;
	}

	//	A brand-new team is only allowed in while there is room for one -- the same refuse-
	//	rather-than-grow rule as the source table and the grid itself.
	bool team_already_present = false;
	for (int i = 0; i < Sources.Count(); i++) {
		if (Sources[i].InUse && Sources[i].Team == team) {
			team_already_present = true;
			break;
		}
	}
	if (!team_already_present && Distinct_Source_Team_Count() >= WORLD_SHROUD_MAX_TEAMS) {
		RefusalCount++;
		return 0;
	}

	int slot = -1;
	for (int i = 0; i < Sources.Count(); i++) {
		if (!Sources[i].InUse) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		if (Sources.Count() >= WORLD_SHROUD_MAX_SENSOR_SOURCES) {
			RefusalCount++;
			return 0;
		}
		Sources.Add(SensorSourceRecord());
		slot = Sources.Count() - 1;
	}

	SensorSourceRecord & rec = Sources[slot];
	rec.InUse					= true;
	rec.Team						= team;
	rec.Position				= world_transform;
	rec.VisionRadius			= WWMath::Max(vision_radius,0.0f);
	rec.RadarRadius			= WWMath::Max(radar_radius,0.0f);
	rec.LastKnownRetention	= last_known_retention_seconds;
	rec.Online					= online;

	return slot + 1;
}


/***********************************************************************************************
 * WorldShroudManager::UpdateStrategicSensorSource -- move/reconfigure a registered source        *
 *=============================================================================================*/
bool WorldShroudManager::UpdateStrategicSensorSource(ShroudSensorHandleType handle,const Vector3 & world_transform,
																			float vision_radius,float radar_radius,bool online)
{
	if (!Initialized || handle <= 0 || handle > Sources.Count()) {
		return false;
	}
	SensorSourceRecord & rec = Sources[handle - 1];
	if (!rec.InUse) {
		return false;
	}
	rec.Position		= world_transform;
	rec.VisionRadius	= WWMath::Max(vision_radius,0.0f);
	rec.RadarRadius	= WWMath::Max(radar_radius,0.0f);
	rec.Online			= online;
	return true;
}


/***********************************************************************************************
 * WorldShroudManager::UnregisterStrategicSensorSource -- remove a source                        *
 *=============================================================================================*/
bool WorldShroudManager::UnregisterStrategicSensorSource(ShroudSensorHandleType handle)
{
	if (!Initialized || handle <= 0 || handle > Sources.Count()) {
		return false;
	}
	SensorSourceRecord & rec = Sources[handle - 1];
	if (!rec.InUse) {
		return false;
	}
	rec = SensorSourceRecord();		// InUse false, slot free for reuse; handle never reissued
												// to a *different* registration while this one still
												// existed, since InUse gates every lookup by handle.
	return true;
}


bool WorldShroudManager::Is_Sensor_Source(ShroudSensorHandleType handle)
{
	if (!Initialized || handle <= 0 || handle > Sources.Count()) {
		return false;
	}
	return Sources[handle - 1].InUse;
}


int WorldShroudManager::Get_Sensor_Source_Count(void)
{
	int count = 0;
	for (int i = 0; i < Sources.Count(); i++) {
		if (Sources[i].InUse) {
			count++;
		}
	}
	return count;
}


/***********************************************************************************************
 * WorldShroudManager::Find_Team_Slot / Make_Team_Slot                                           *
 *=============================================================================================*/
int WorldShroudManager::Find_Team_Slot(int team)
{
	for (int i = 0; i < Teams.Count(); i++) {
		if (Teams[i].Team == team) {
			return i;
		}
	}
	return -1;
}

int WorldShroudManager::Make_Team_Slot(int team)
{
	int existing = Find_Team_Slot(team);
	if (existing >= 0) {
		return existing;
	}
	if (Teams.Count() >= WORLD_SHROUD_MAX_TEAMS) {
		RefusalCount++;
		return -1;
	}

	TeamGridClass grid;
	grid.Team = team;
	if (Partitioned && Cols > 0 && Rows > 0) {
		grid.CellCount = Cols * Rows;
		grid.Cells = new ShroudCellClass[grid.CellCount];
	}
	Teams.Add(grid);
	return Teams.Count() - 1;
}


/***********************************************************************************************
 * WorldShroudManager::World_To_Cell -- world XY to a clamped grid cell                          *
 *=============================================================================================*/
bool WorldShroudManager::World_To_Cell(float x,float y,int * col,int * row)
{
	if (!Partitioned || Cols <= 0 || Rows <= 0) {
		return false;
	}
	int c = (int)WWMath::Floor((x - WorldMin.X) / CellSize);
	int r = (int)WWMath::Floor((y - WorldMin.Y) / CellSize);
	c = (c < 0) ? 0 : ((c >= Cols) ? (Cols - 1) : c);
	r = (r < 0) ? 0 : ((r >= Rows) ? (Rows - 1) : r);
	*col = c;
	*row = r;
	return true;
}


/***********************************************************************************************
 * WorldShroudManager::Stamp_Circle -- mark every cell whose center falls inside a world circle   *
 *=============================================================================================*/
void WorldShroudManager::Stamp_Circle(bool * coverage,float cx,float cy,float radius)
{
	if (radius <= 0.0f || !Partitioned || Cols <= 0 || Rows <= 0) {
		return;
	}

	int min_col,min_row,max_col,max_row;
	World_To_Cell(cx - radius,cy - radius,&min_col,&min_row);
	World_To_Cell(cx + radius,cy + radius,&max_col,&max_row);
	if (min_col > max_col) { int t = min_col; min_col = max_col; max_col = t; }
	if (min_row > max_row) { int t = min_row; min_row = max_row; max_row = t; }

	float radius_sq = radius * radius;

	for (int row = min_row; row <= max_row; row++) {
		float cell_cy = WorldMin.Y + (row + 0.5f) * CellSize;
		for (int col = min_col; col <= max_col; col++) {
			float cell_cx = WorldMin.X + (col + 0.5f) * CellSize;
			float dx = cell_cx - cx;
			float dy = cell_cy - cy;
			if ((dx * dx + dy * dy) <= radius_sq) {
				coverage[row * Cols + col] = true;
			}
		}
	}

	//	A source always covers the cell it stands in.  The centre test above misses it whenever
	//	the radius is smaller than the distance from the source to that cell's own centre -- a
	//	source on a cell corner with a radius under half the cell's diagonal would otherwise not
	//	see where it is standing, which a coarse grid on a large map makes routine.
	int own_col,own_row;
	if (World_To_Cell(cx,cy,&own_col,&own_row)) {
		coverage[own_row * Cols + own_col] = true;
	}
}


/***********************************************************************************************
 * WorldShroudManager::Timestep -- recompute live coverage, age LAST_KNOWN retention             *
 *=============================================================================================*/
void WorldShroudManager::Timestep(float dt)
{
	if (!Initialized || !Partitioned || Cols <= 0 || Rows <= 0) {
		return;
	}

	int cell_count = Cols * Rows;
	if (ScratchCellCount < cell_count) {
		delete [] VisionScratch;
		delete [] RadarScratch;
		delete [] RetentionScratch;
		VisionScratch		= new bool[cell_count];
		RadarScratch		= new bool[cell_count];
		RetentionScratch	= new float[cell_count];
		ScratchCellCount	= cell_count;
	}

	//	Every team with at least one registered source gets a grid, even one that has never
	//	been queried yet -- so a source registered before the first query still accumulates
	//	EXPLORED memory instead of losing every tick before anyone asked.
	for (int i = 0; i < Sources.Count(); i++) {
		if (Sources[i].InUse) {
			Make_Team_Slot(Sources[i].Team);
		}
	}

	for (int t = 0; t < Teams.Count(); t++) {
		TeamGridClass & grid = Teams[t];

		//	A team created before the current partition, or before Re_Partition dropped every
		//	grid, has no cells yet; give it one sized to the current grid.
		if (grid.Cells == nullptr || grid.CellCount != cell_count) {
			delete [] grid.Cells;
			grid.CellCount = cell_count;
			grid.Cells = new ShroudCellClass[cell_count];
		}

		::memset(VisionScratch,0,cell_count * sizeof(bool));
		::memset(RadarScratch,0,cell_count * sizeof(bool));
		for (int c = 0; c < cell_count; c++) {
			RetentionScratch[c] = 0.0f;
		}

		for (int i = 0; i < Sources.Count(); i++) {
			SensorSourceRecord & rec = Sources[i];
			if (!rec.InUse || !rec.Online || rec.Team != grid.Team) {
				continue;
			}
			float retention = (rec.LastKnownRetention > 0.0f) ? rec.LastKnownRetention : DefaultRetention;

			if (rec.VisionRadius > 0.0f) {
				Stamp_Circle(VisionScratch,rec.Position.X,rec.Position.Y,rec.VisionRadius);
			}
			if (rec.RadarRadius > 0.0f) {
				Stamp_Circle(RadarScratch,rec.Position.X,rec.Position.Y,rec.RadarRadius);
			}

			//	Retention is recorded for every cell this source's radar (the larger of its two
			//	circles in the normal case) reaches, so a cell remembers the best retention that
			//	applied to it the last tick it was covered, without needing per-tick history.
			float stamp_radius = WWMath::Max(rec.VisionRadius,rec.RadarRadius);
			if (stamp_radius > 0.0f) {
				int min_col,min_row,max_col,max_row;
				World_To_Cell(rec.Position.X - stamp_radius,rec.Position.Y - stamp_radius,&min_col,&min_row);
				World_To_Cell(rec.Position.X + stamp_radius,rec.Position.Y + stamp_radius,&max_col,&max_row);
				for (int row = min_row; row <= max_row; row++) {
					for (int col = min_col; col <= max_col; col++) {
						int idx = row * Cols + col;
						if ((VisionScratch[idx] || RadarScratch[idx]) && retention > RetentionScratch[idx]) {
							RetentionScratch[idx] = retention;
						}
					}
				}
			}
		}

		for (int c = 0; c < cell_count; c++) {
			ShroudCellClass & cell = grid.Cells[c];
			bool in_vision = VisionScratch[c];
			bool in_radar	 = RadarScratch[c];

			if (in_vision) {
				cell.Explored				= true;
				cell.Transient				= (unsigned char)SHROUD_VISIBLE;
				cell.Retention				= RetentionScratch[c];
				cell.LastKnownRemaining	= 0.0f;
			} else if (in_radar) {
				cell.Explored				= true;
				cell.Transient				= (unsigned char)SHROUD_RADAR_DETECTED;
				cell.Retention				= RetentionScratch[c];
				cell.LastKnownRemaining	= 0.0f;
			} else if (cell.Transient == (unsigned char)SHROUD_VISIBLE ||
						  cell.Transient == (unsigned char)SHROUD_RADAR_DETECTED) {
				cell.Transient				= (unsigned char)SHROUD_LAST_KNOWN;
				cell.LastKnownRemaining	= (cell.Retention > 0.0f) ? cell.Retention : DefaultRetention;
			} else if (cell.Transient == (unsigned char)SHROUD_LAST_KNOWN) {
				cell.LastKnownRemaining -= dt;
				if (cell.LastKnownRemaining <= 0.0f) {
					cell.Transient = SHROUD_CELL_TRANSIENT_NONE;
				}
			}
			//	Transient already NONE: nothing changes; the cell reads as EXPLORED or UNKNOWN
			//	from its Explored bit alone.
		}
	}
}


/***********************************************************************************************
 * WorldShroudManager::Get_Shroud_State -- what a team currently knows about a world point        *
 *=============================================================================================*/
ShroudStateType WorldShroudManager::Get_Shroud_State(int team,const Vector3 & world_point)
{
	int team_slot = Find_Team_Slot(team);
	if (team_slot < 0) {
		return SHROUD_UNKNOWN;
	}
	TeamGridClass & grid = Teams[team_slot];
	int col,row;
	if (grid.Cells == nullptr || !World_To_Cell(world_point.X,world_point.Y,&col,&row)) {
		return SHROUD_UNKNOWN;
	}
	int idx = row * Cols + col;
	if (idx < 0 || idx >= grid.CellCount) {
		return SHROUD_UNKNOWN;
	}
	const ShroudCellClass & cell = grid.Cells[idx];
	if (cell.Transient != SHROUD_CELL_TRANSIENT_NONE) {
		return (ShroudStateType)cell.Transient;
	}
	return cell.Explored ? SHROUD_EXPLORED : SHROUD_UNKNOWN;
}

bool WorldShroudManager::Is_Visible(int team,const Vector3 & world_point)
{
	return Get_Shroud_State(team,world_point) == SHROUD_VISIBLE;
}

bool WorldShroudManager::Is_Radar_Detected(int team,const Vector3 & world_point)
{
	return Get_Shroud_State(team,world_point) == SHROUD_RADAR_DETECTED;
}

bool WorldShroudManager::Is_Explored(int team,const Vector3 & world_point)
{
	ShroudStateType state = Get_Shroud_State(team,world_point);
	return state != SHROUD_UNKNOWN;
}


/***********************************************************************************************
 * Debug-overlay support -- Section 33's DEBUG_LAYER_SHROUD_CELLS                               *
 *=============================================================================================*/
bool WorldShroudManager::Get_Grid_Geometry(float * cell_size,int * cols,int * rows,Vector3 * world_min)
{
	if (!Partitioned) {
		return false;
	}
	if (cell_size != nullptr) *cell_size = CellSize;
	if (cols != nullptr) *cols = Cols;
	if (rows != nullptr) *rows = Rows;
	if (world_min != nullptr) *world_min = WorldMin;
	return true;
}

ShroudStateType WorldShroudManager::Get_Cell_State_By_Index(int team,int col,int row)
{
	int team_slot = Find_Team_Slot(team);
	if (team_slot < 0) {
		return SHROUD_UNKNOWN;
	}
	TeamGridClass & grid = Teams[team_slot];
	if (grid.Cells == nullptr || col < 0 || col >= Cols || row < 0 || row >= Rows) {
		return SHROUD_UNKNOWN;
	}
	int idx = row * Cols + col;
	if (idx < 0 || idx >= grid.CellCount) {
		return SHROUD_UNKNOWN;
	}
	const ShroudCellClass & cell = grid.Cells[idx];
	if (cell.Transient != SHROUD_CELL_TRANSIENT_NONE) {
		return (ShroudStateType)cell.Transient;
	}
	return cell.Explored ? SHROUD_EXPLORED : SHROUD_UNKNOWN;
}


/***********************************************************************************************
 * Accounting                                                                                    *
 *=============================================================================================*/
int WorldShroudManager::Get_Team_Count(void)
{
	return Teams.Count();
}

int WorldShroudManager::Get_Cell_Count(void)
{
	return Cols * Rows;
}

int WorldShroudManager::Get_Visible_Cell_Count(int team)
{
	int team_slot = Find_Team_Slot(team);
	if (team_slot < 0) {
		return 0;
	}
	TeamGridClass & grid = Teams[team_slot];
	if (grid.Cells == nullptr) {
		return 0;
	}
	int count = 0;
	for (int c = 0; c < grid.CellCount; c++) {
		if (grid.Cells[c].Transient == (unsigned char)SHROUD_VISIBLE) {
			count++;
		}
	}
	return count;
}

int WorldShroudManager::Get_Explored_Cell_Count(int team)
{
	int team_slot = Find_Team_Slot(team);
	if (team_slot < 0) {
		return 0;
	}
	TeamGridClass & grid = Teams[team_slot];
	if (grid.Cells == nullptr) {
		return 0;
	}
	int count = 0;
	for (int c = 0; c < grid.CellCount; c++) {
		if (grid.Cells[c].Explored) {
			count++;
		}
	}
	return count;
}

int WorldShroudManager::Get_Refusal_Count(void)
{
	return RefusalCount;
}
