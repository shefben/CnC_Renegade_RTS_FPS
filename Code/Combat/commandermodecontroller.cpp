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
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandermodecontroller.cpp           $*
 *                                                                                             *
 *	Roadmap Section 41, Phase 35 -- Commander Mode camera / embodied player.                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandermodecontroller.h"

#include "commanderplacement.h"
#include "commandersidebar.h"
#include "soldier.h"

#include "castres.h"
#include "ccamera.h"
#include "combat.h"
#include "lineseg.h"
#include "pathfind.h"
#include "pathfindsector.h"
#include "phys.h"
#include "physcoltest.h"
#include "rendobj.h"
#include "worldboundsservice.h"
#include "baseselectionplanner.h"
#include "basecontroller.h"
#include "building.h"
#include "combatphysobserver.h"
#include "physicalgameobj.h"
#include "worldspatialindex.h"
#include "wwmath.h"
#include "commandercamera.h"
#include "strategicviewmanager.h"


static const float	COMMANDER_DEFAULT_ZOOM_MIN		= 50.0f;
static const float	COMMANDER_DEFAULT_ZOOM_MAX		= 2000.0f;
static const float	COMMANDER_DEFAULT_TILT_MIN		= 15.0f;		// degrees above the horizon
static const float	COMMANDER_DEFAULT_TILT_MAX		= 85.0f;
static const float	COMMANDER_PICK_RAY_LENGTH		= 10000.0f;


static bool							_Active			= false;
static SoldierGameObj *			_Star				= nullptr;

static Vector3						_WorldMin (-100000.0f,-100000.0f,-100000.0f);
static Vector3						_WorldMax ( 100000.0f, 100000.0f, 100000.0f);
static float						_ZoomMin			= COMMANDER_DEFAULT_ZOOM_MIN;
static float						_ZoomMax			= COMMANDER_DEFAULT_ZOOM_MAX;
static float						_TiltMin			= COMMANDER_DEFAULT_TILT_MIN;
static float						_TiltMax			= COMMANDER_DEFAULT_TILT_MAX;

static Vector3						_Focus (0.0f,0.0f,0.0f);
static bool							_FocusPlaced	= false;		// the entry anchor has been applied this match

static bool							_HavePickRay	= false;
static bool							_PickRayHit		= false;
static Vector3						_PickRayOrigin (0.0f,0.0f,0.0f);
static Vector3						_PickRayEnd (0.0f,0.0f,0.0f);
static float						_Zoom				= 400.0f;
static float						_Tilt				= 55.0f;
static float						_Rotation		= 0.0f;


/*
**	The overhead camera itself is the CnC_Renegade_new fork's CommanderCamera, driven by its
**	StrategicViewManager (pan keys, wheel zoom, smoothing, the in/out transition).  This
**	controller stays the owner of *being* in Commander Mode -- the sidebar, the embodied star,
**	picking -- and hands the camera an anchor: the entry focus, with a pan radius wide enough
**	to cover the world bounds.
*/
class CommanderViewBoundsClass : public IStrategicViewBounds
{
public:
	bool Get_Base_Center (int player_type,Vector3 & center) const override
	{
		(void)player_type;
		center = _Focus;
		return true;
	}

	float Get_Max_Pan_Distance (int player_type) const override
	{
		(void)player_type;
		const float world_extent = (Vector3 (_WorldMax.X - _WorldMin.X,_WorldMax.Y - _WorldMin.Y,0.0f)).Length ();
		return WWMath::Clamp (world_extent,100.0f,4000.0f);
	}
};

static CommanderViewBoundsClass	_ViewBounds;

static bool Overhead_Camera_Engaged(void)
{
	return (CommanderCamera::Get_Instance () != nullptr) && StrategicViewManager::Is_Overhead_View ();
}


static float	Clamp(float value,float lo,float hi)
{
	if (value < lo) return lo;
	if (value > hi) return hi;
	return value;
}


void CommanderModeController::Init(void)
{
	_Active	= false;
	_Star		= nullptr;

	_WorldMin.Set (-100000.0f,-100000.0f,-100000.0f);
	_WorldMax.Set ( 100000.0f, 100000.0f, 100000.0f);
	_ZoomMin = COMMANDER_DEFAULT_ZOOM_MIN;
	_ZoomMax = COMMANDER_DEFAULT_ZOOM_MAX;
	_TiltMin = COMMANDER_DEFAULT_TILT_MIN;
	_TiltMax = COMMANDER_DEFAULT_TILT_MAX;

	_Focus.Set (0.0f,0.0f,0.0f);
	_FocusPlaced = false;
	_Zoom		= 400.0f;
	_Tilt		= 55.0f;
	_Rotation= 0.0f;
}

void CommanderModeController::Shutdown(void)
{
	_Active	= false;
	_Star		= nullptr;
}


bool CommanderModeController::Enter_Commander_Mode(SoldierGameObj * star)
{
	if (star == nullptr) return false;

	int team = star->Get_Player_Type ();
	if (!Enter_Commander_Mode (star,team)) return false;

	if (!_FocusPlaced) {
		Vector3 star_position;
		star->Get_Position (&star_position);
		Set_Focus (Resolve_Entry_Focus (team,star_position));
		_FocusPlaced = true;
	}

	if (CommanderCamera::Get_Instance () != nullptr) {
		StrategicViewManager::Set_Bounds_Provider (&_ViewBounds);
		StrategicViewManager::Set_Player_Type (team);
		StrategicViewManager::Request_View_Mode (StrategicViewManager::VIEW_OVERHEAD);
	}
	return true;
}


Vector3 CommanderModeController::Resolve_Entry_Focus(int team,const Vector3 & star_position)
{
	SelectedBaseSite site;
	if (BaseSelectionPlanner::Has_Selection () && BaseSelectionPlanner::Get_Selected_Site (team,&site)) {
		return site.Position;
	}

	BaseControllerClass * base = BaseControllerClass::Find_Base (team);
	if ((base != nullptr) && (base->Get_Building_Count () > 0)) {
		Vector3 sum (0.0f,0.0f,0.0f);
		int counted = 0;
		for (int index = 0; index < base->Get_Building_Count (); index++) {
			BuildingGameObj * building = base->Peek_Building (index);
			if (building == nullptr) continue;
			Vector3 position;
			building->Get_Position (&position);
			sum += position;
			counted++;
		}
		if (counted > 0) {
			return sum / (float)counted;
		}
	}

	return star_position;
}

bool CommanderModeController::Enter_Commander_Mode(SoldierGameObj * star,int team)
{
	if (star == nullptr) return false;
	if (_Active) return false;

	_Active	= true;
	_Star		= star;

	/*
	**	The sidebar exists in Commander Mode and nowhere else (Section 42), so entering is the
	**	one event that opens it and leaving is the one that closes it.  Opening it here rather
	**	than at the console command means every future way into Commander Mode gets the panel
	**	without having to remember to ask for it.
	**
	**	The team comes from the embodied star, which is the only team a Commander can command --
	**	Section 3A's embodiment rule, showing up as a UI fact.
	*/
	CommanderSidebarModel::Open (team);
	return true;
}

void CommanderModeController::Exit_Commander_Mode(void)
{
	if (Overhead_Camera_Engaged ()) {
		StrategicViewManager::Request_View_Mode (StrategicViewManager::VIEW_FIRST_PERSON);
	}

	_Active	= false;
	_Star		= nullptr;

	//	Closing drops the cards and any bound facility, and cancelling the ghost makes sure a
	//	half-placed building does not survive a death that dropped the player out of Commander
	//	Mode mid-placement.
	CommanderSidebarModel::Close ();
	CommanderPlacementValidator::Cancel_Ghost ();
}

bool CommanderModeController::Is_Active(void)						{ return _Active; }
CommanderCameraModeType CommanderModeController::Get_Mode(void)	{ return _Active ? COMMANDER_MODE : FPS_MODE; }
SoldierGameObj * CommanderModeController::Get_Embodied_Star(void)	{ return _Star; }


void CommanderModeController::Set_World_Bounds(const Vector3 & minimum,const Vector3 & maximum)
{
	_WorldMin = minimum;
	_WorldMax = maximum;
	Set_Focus (_Focus);		// re-clamp whatever focus is already set
}

void CommanderModeController::Set_Zoom_Range(float minimum,float maximum)
{
	if (minimum <= 0.0f || maximum < minimum) return;
	_ZoomMin = minimum;
	_ZoomMax = maximum;
	Set_Zoom (_Zoom);
}

void CommanderModeController::Set_Tilt_Range(float minimum_degrees,float maximum_degrees)
{
	if (maximum_degrees < minimum_degrees) return;
	_TiltMin = minimum_degrees;
	_TiltMax = maximum_degrees;
	Set_Tilt (_Tilt);
}


void CommanderModeController::Set_Focus(const Vector3 & world_point)
{
	_Focus.X = Clamp (world_point.X,_WorldMin.X,_WorldMax.X);
	_Focus.Y = Clamp (world_point.Y,_WorldMin.Y,_WorldMax.Y);
	_Focus.Z = Clamp (world_point.Z,_WorldMin.Z,_WorldMax.Z);

	/*
	**	V2 roadmap Section 34 Phase 2 step 11 / Section 5.8 -- the strategic camera obeys the
	**	same authoritative boundary every actor does, so it cannot pan outside the cliff ring
	**	and look at the terrain from behind the world.
	**
	**	Applied after the existing box clamp rather than instead of it: the box is what a
	**	hand-authored map configures through Set_World_Bounds and it keeps working exactly as
	**	before, while a procedural world adds the real polygon on top.  Clamp_Commander_Camera
	**	returns its argument untouched when no boundary is configured, so this is a no-op on
	**	every legacy map (directive 1.10).
	*/
	_Focus = WorldBoundsService::Clamp_Commander_Camera (_Focus);

	if (Overhead_Camera_Engaged ()) {
		CommanderCamera::Get_Instance ()->Focus_On (_Focus);
	}
}

void CommanderModeController::Pan(const Vector3 & delta)
{
	Set_Focus (_Focus + delta);
}

Vector3 CommanderModeController::Get_Focus(void)	{ return _Focus; }


void CommanderModeController::Set_Zoom(float height)		{ _Zoom = Clamp (height,_ZoomMin,_ZoomMax); }
void CommanderModeController::Zoom(float delta)				{ Set_Zoom (_Zoom + delta); }
float CommanderModeController::Get_Zoom(void)					{ return _Zoom; }

void CommanderModeController::Set_Tilt(float degrees)		{ _Tilt = Clamp (degrees,_TiltMin,_TiltMax); }
float CommanderModeController::Get_Tilt(void)					{ return _Tilt; }

void CommanderModeController::Set_Rotation(float degrees)
{
	_Rotation = WWMath::Wrap (degrees,0.0f,360.0f);
}
float CommanderModeController::Get_Rotation(void)				{ return _Rotation; }


Matrix3D CommanderModeController::Compute_Camera_Transform(void)
{
	float tilt_rad		= DEG_TO_RADF (_Tilt);
	float rotation_rad	= DEG_TO_RADF (_Rotation);

	//	Standard orbit-camera offset from the focus point: horizontal distance shrinks as tilt
	//	steepens toward looking straight down, height grows the same way.  Z is up, matching
	//	every other world-space convention in this project (WorldTerrainSystem::Sample_Height,
	//	Section 30's shroud grid).
	float horizontal_distance	= _Zoom * WWMath::Cos (tilt_rad);
	float height					= _Zoom * WWMath::Sin (tilt_rad);

	Vector3 camera_position (
		_Focus.X + horizontal_distance * WWMath::Sin (rotation_rad),
		_Focus.Y - horizontal_distance * WWMath::Cos (rotation_rad),
		_Focus.Z + height);

	Vector3 forward = _Focus - camera_position;
	if (forward.Length () < 0.001f) {
		forward.Set (0.0f,1.0f,0.0f);		// directly overhead; pick an arbitrary forward
	} else {
		forward.Normalize ();
	}

	Vector3 world_up (0.0f,0.0f,1.0f);
	Vector3 right = Vector3::Cross_Product (forward,world_up);
	if (right.Length () < 0.001f) {
		right.Set (1.0f,0.0f,0.0f);			// forward parallel to world up; pick an arbitrary right
	} else {
		right.Normalize ();
	}
	Vector3 up = Vector3::Cross_Product (right,forward);
	up.Normalize ();

	Matrix3D transform;
	transform.Set (forward,right,up,camera_position);
	return transform;
}


/***********************************************************************************************
 * Is_Commandable_Surface -- is this struck point somewhere a unit could actually be sent?       *
 *                                                                                             *
 *	The answer comes from the one authority on where this engine's AI can walk: the pathfind     *
 *	sectors.  On a map-authored level they were built by the editor; on a generated world        *
 *	`NavMeshGenerator` fills the same `PathfindClass` with sectors and portals.  Either way, a   *
 *	point no sector contains is a point `Action_Goto` will refuse, so picking it is picking a    *
 *	destination the order layer is guaranteed to throw away.                                     *
 *                                                                                             *
 *	A level with no pathfind data at all answers "yes" to everything -- there is nothing to       *
 *	consult, and on such a level `GotoActionCodeClass::Pathfind` beelines rather than refusing,   *
 *	so the first surface struck is still the honest answer.                                      *
 *                                                                                             *
 *	The fudge matches `HumanStateClass`'s own sector lookups (2.0): a point sampled on a floor    *
 *	triangle sits on the boundary of the sector resting on it, and an exact containment test     *
 *	would reject half the ground in the world to floating-point noise.                           *
 *=============================================================================================*/
static bool	Is_Commandable_Surface(const Vector3 & point)
{
	PathfindClass * pathfind = PathfindClass::Get_Instance ();
	if (pathfind == nullptr) return true;
	if (!pathfind->Does_Pathfind_Data_Exist ()) return true;

	return (pathfind->Find_Sector (point,2.0f) != nullptr);
}


bool CommanderModeController::Pick_At_Cursor(const Vector3 & ray_origin,const Vector3 & ray_direction,
															 Vector3 * out_world_point)
{
	if (out_world_point == nullptr) return false;

	float length = ray_direction.Length ();
	if (length < 0.0001f) return false;
	Vector3 direction = ray_direction * (1.0f / length);

	Vector3 far_point = ray_origin + direction * COMMANDER_PICK_RAY_LENGTH;

	_HavePickRay	= true;
	_PickRayOrigin	= ray_origin;

	/*
	**	The cursor ray keeps going past geometry it cannot command onto.  A strategic camera
	**	looking down at an interior -- the cavern C&C_Under is almost entirely made of, a tunnel,
	**	the inside of a structure -- strikes the roof over that space long before it reaches the
	**	floor the units are standing on, and taking that first hit is what put every waypoint in
	**	the ceiling.  Each miss restarts the cast just past the surface it struck, so the ray
	**	walks down through the roof, and the first point a unit could actually be ordered to is
	**	the one the cursor means.
	**
	**	Bounded on purpose: eight surfaces is deeper than any stacked interior in this game's
	**	content, and a bound is what keeps a cursor that happens to graze a long run of thin
	**	geometry from casting all day on the frame the player moved the mouse.  Exhausting the
	**	budget falls back to the first surface struck -- the old behaviour, which is wrong in a
	**	cave but right everywhere there is nothing better to choose.
	*/
	const int	MAX_PICK_SURFACES	= 8;

	//	Past the surface just struck, so the next cast cannot re-hit the triangle it started on.
	const float	SURFACE_ADVANCE	= 0.25f;

	Vector3	first_hit (0.0f,0.0f,0.0f);
	bool		have_first	= false;
	float		travelled	= 0.0f;

	for (int surface = 0; surface < MAX_PICK_SURFACES; surface++) {

		float remaining = COMMANDER_PICK_RAY_LENGTH - travelled;
		if (remaining <= 0.0f) break;

		Vector3 segment_origin	= ray_origin + direction * travelled;
		Vector3 segment_end		= segment_origin + direction * remaining;

		LineSegClass ray (segment_origin,segment_end);
		CastResultStruct result;

		//	Collision group 0: a Commander cursor pick is asking "what is the ground/geometry
		//	here", not making a gameplay/team-filtered damage query, so no group restricts it.
		PhysRayCollisionTestClass raytest (ray,&result,0);

		if (!WorldSpatialIndex::Query_Ray (raytest)) break;

		float		hit_distance	= travelled + (result.Fraction * remaining);
		Vector3	hit				= ray_origin + direction * hit_distance;

		if (!have_first) {
			first_hit	= hit;
			have_first	= true;
		}

		if (Is_Commandable_Surface (hit)) {
			*out_world_point	= hit;
			_PickRayHit			= true;
			_PickRayEnd			= hit;
			return true;
		}

		travelled = hit_distance + SURFACE_ADVANCE;
	}

	if (have_first) {
		*out_world_point	= first_hit;
		_PickRayHit			= true;
		_PickRayEnd			= first_hit;
		return true;
	}

	_PickRayHit	= false;
	_PickRayEnd	= far_point;
	return false;
}


bool CommanderModeController::Get_Last_Pick_Ray(Vector3 * origin_out,Vector3 * end_out,bool * hit_out)
{
	if (!_HavePickRay) return false;
	if (origin_out != nullptr)	*origin_out = _PickRayOrigin;
	if (end_out != nullptr)		*end_out = _PickRayEnd;
	if (hit_out != nullptr)		*hit_out = _PickRayHit;
	return true;
}


int CommanderModeController::Pick_Object_At_Cursor(const Vector3 & ray_origin,const Vector3 & ray_direction,
																	bool * out_is_building)
{
	if (out_is_building != nullptr) *out_is_building = false;

	float length = ray_direction.Length ();
	if (length < 0.0001f) return 0;
	Vector3 direction = ray_direction * (1.0f / length);

	LineSegClass ray (ray_origin,ray_origin + direction * COMMANDER_PICK_RAY_LENGTH);
	CastResultStruct result;
	PhysRayCollisionTestClass raytest (ray,&result,0);

	if (!WorldSpatialIndex::Query_Ray (raytest)) return 0;
	if ((raytest.CollidedPhysObj == nullptr) || (raytest.CollidedPhysObj->Get_Observer () == nullptr)) return 0;

	//	The same observer cast the bullet and camera code use to name what a ray struck.  A
	//	building's meshes observe the building itself, so a click anywhere on its shell resolves.
	CombatPhysObserverClass * observer = (CombatPhysObserverClass *)raytest.CollidedPhysObj->Get_Observer ();

	BuildingGameObj * building = observer->As_BuildingGameObj ();
	if (building != nullptr) {
		if (out_is_building != nullptr) *out_is_building = true;
		return building->Get_ID ();
	}

	PhysicalGameObj * physical = observer->As_PhysicalGameObj ();
	return (physical != nullptr) ? physical->Get_ID () : 0;
}


void CommanderModeController::Update(float seconds,bool star_is_alive)
{
	(void)seconds;

	if (!_Active) return;

	if (!star_is_alive) {
		Exit_Commander_Mode ();		// "death immediately exits Commander Mode"
		return;
	}

	// The CommanderCamera moves itself in its own Update; mirror where it is looking so picking,
	// placement and the minimap read the camera's real focus.
	if (Overhead_Camera_Engaged ()) {
		_Focus = CommanderCamera::Get_Instance ()->Get_Look_At_Position ();
		return;
	}

	if (COMBAT_CAMERA != nullptr) {
		COMBAT_CAMERA->Set_Transform (Compute_Camera_Transform ());
	}
}
