/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderwaypointrenderer.cpp         $*
 *                                                                                             *
 *	Roadmap Sections 3A and 45.  See commanderwaypointrenderer.h.                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderwaypointrenderer.h"

#include "aabox.h"
#include "camera.h"
#include "ccamera.h"
#include "combat.h"
#include "commanderwaypoint.h"
#include "linegrp.h"
#include "pscene.h"
#include "rendercomponent.h"
#include "rendobj.h"
#include "rinfo.h"
#include "shader.h"
#include "soldier.h"
#include "sphere.h"
#include "worldparticlemanager.h"
#include "ww3d.h"
#include "wwmath.h"


/*
**	Geometry budget.  Each marker is one ring plus one column: RING_SEGMENTS segments for the
**	ring and one for the column.  Four teams times four waypoints is the system's own cap, but
**	only one team's are ever drawn at once, so the pool only has to hold one team's worth.
*/
static const int		RING_SEGMENTS		= 16;
static const int		SEGMENTS_PER_MARKER	= RING_SEGMENTS + 1;
static const int		MAX_MARKERS			= CommanderWaypointSystem::MAX_PER_TEAM;
static const int		MAX_SEGMENTS		= MAX_MARKERS * SEGMENTS_PER_MARKER;

static const float	RING_RADIUS			= 2.2f;
static const float	COLUMN_HEIGHT		= 3.4f;
static const float	SEGMENT_THICKNESS	= 0.09f;

//	Translucent yellow, as asked for, and the same amber family the Commander sidebar reads in
//	so the whole layer is one colour.  Alpha is low enough to see the ground through it and high
//	enough to read against bright terrain.
static const Vector4	COLOR_MOVE		(1.00f,0.80f,0.25f,0.55f);
static const Vector4	COLOR_ATTACK	(1.00f,0.45f,0.20f,0.60f);
static const Vector4	COLOR_DEFEND	(0.55f,0.80f,1.00f,0.55f);
static const Vector4	COLOR_RALLY		(1.00f,0.92f,0.55f,0.50f);


static bool		_Available		= false;
static int		_DrawnMarkers	= 0;
static float	_PulsePhase		= 0.0f;

//	Rebuilt every Update, read inside Render on the same frame.  File-static rather than on the
//	scene object because the submission structure takes bare pointers into them.
static Vector3	_Head    [MAX_SEGMENTS];
static Vector3	_Tail    [MAX_SEGMENTS];
static Vector4	_Diffuse [MAX_SEGMENTS];
static float		_Size    [MAX_SEGMENTS];
static int		_SegmentCount	= 0;
static Vector3	_BatchCenter (0.0f,0.0f,0.0f);


static const Vector4 &	Color_For_Kind(CommanderWaypointKindType kind)
{
	switch (kind) {
		case WAYPOINT_ATTACK:	return COLOR_ATTACK;
		case WAYPOINT_DEFEND:	return COLOR_DEFEND;
		case WAYPOINT_RALLY:		return COLOR_RALLY;
		default:						return COLOR_MOVE;
	}
}


static void	Add_Segment(const Vector3 & head,const Vector3 & tail,const Vector4 & color)
{
	if (_SegmentCount >= MAX_SEGMENTS) return;

	_Head    [_SegmentCount]	= head;
	_Tail    [_SegmentCount]	= tail;
	_Diffuse [_SegmentCount]	= color;
	_Size    [_SegmentCount]	= SEGMENT_THICKNESS;
	_SegmentCount++;
}


/***********************************************************************************************
 * CommanderWaypointSceneObjectClass -- the marker's place in the world                          *
 *                                                                                             *
 *	A world render object rather than a HUD pass, so the markers are depth-sorted against the    *
 *	terrain and buildings they sit among instead of being painted over them.  Modelled on         *
 *	`DebugDisplaySceneObjectClass` (`Code/Combat/worlddebugdisplay.cpp`), which is this tree's     *
 *	established way to put procedural geometry into the combat scene.                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
class CommanderWaypointSceneObjectClass : public RenderObjClass, public IRenderComponent
{
	public:
		CommanderWaypointSceneObjectClass(void)	{ Set_Transform (Matrix3D (1)); }

		virtual RenderObjClass *	Clone(void) const override	{ WWASSERT (false); return nullptr; }

		virtual void	Render(RenderInfoClass & rinfo) override
		{
			if (_SegmentCount <= 0) return;

			/*
			**	Alpha blended, both faces, fogged: a marker is a translucent object in the world
			**	and has to be lit and faded by the same distance fog everything else is, or it
			**	reads as a UI element that happens to be in 3D.  Depth write stays off so two
			**	overlapping markers blend rather than cutting into each other.
			*/
			ShaderClass shader = ShaderClass::_PresetAlphaShader;
			shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
			shader.Set_Cull_Mode (ShaderClass::CULL_MODE_DISABLE);
			shader.Enable_Fog ("CommanderWaypointRenderer");

			ParticleSubmissionStruct submission;
			submission.Texture			= nullptr;
			submission.Shader				= shader;
			submission.LineMode			= LineGroupClass::PRISM;
			submission.Position			= _Head;
			submission.TailPosition		= _Tail;
			submission.Diffuse			= _Diffuse;
			submission.TailDiffuse		= _Diffuse;
			submission.Size				= _Size;
			submission.Count				= _SegmentCount;
			submission.Center				= _BatchCenter;

			//	Critical gameplay rather than ambient: this is an order being communicated, and a
			//	marker dropped under particle load is a player who never saw where to go.
			submission.Budget				= PARTICLE_BUDGET_CRITICAL_GAMEPLAY;

			WorldParticleManager::Submit (PARTICLE_POOL_STREAK,submission);
		}

		virtual void	Render_Component(RenderInfoClass & rinfo) override		{ Render (rinfo); }

		virtual void	Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const override
		{
			sphere.Init (Vector3 (0,0,0),100000.0f);		// markers can be anywhere in the level
		}

		virtual void	Get_Obj_Space_Bounding_Box(AABoxClass & box) const override
		{
			box.Init (Vector3 (0,0,0),Vector3 (100000.0f,100000.0f,100000.0f));
		}
};

static CommanderWaypointSceneObjectClass *	_TheSceneObject = nullptr;


void CommanderWaypointRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available		= render_available;
	_SegmentCount	= 0;
	_DrawnMarkers	= 0;
	_PulsePhase		= 0.0f;
}


void CommanderWaypointRenderer::Shutdown(void)
{
	if (_TheSceneObject != nullptr) {
		if (COMBAT_SCENE != nullptr) {
			COMBAT_SCENE->Remove_Render_Object (_TheSceneObject);
		}
		REF_PTR_RELEASE (_TheSceneObject);
	}

	_Available		= false;
	_SegmentCount	= 0;
	_DrawnMarkers	= 0;
}


void CommanderWaypointRenderer::Update(float seconds)
{
	if (!_Available) return;

	_SegmentCount	= 0;
	_DrawnMarkers	= 0;

	if (COMBAT_SCENE == nullptr) return;

	/*
	**	Only the local player's own team.  A waypoint is an order; one the enemy could read would
	**	hand them the Commander's intentions, which is the same intelligence rule Section 30
	**	applies to the strategic camera.
	*/
	if (COMBAT_STAR == nullptr) return;
	int team = COMBAT_STAR->Get_Player_Type ();

	//	A slow pulse so a marker reads as live rather than as scenery, and so a stack of them is
	//	still legible.  Wrapped rather than left to grow, since it feeds a sine.
	_PulsePhase += seconds;
	if (_PulsePhase > 1000.0f) _PulsePhase -= 1000.0f;

	float pulse = 0.82f + 0.18f * WWMath::Sin (_PulsePhase * 3.0f);

	Vector3 center_sum (0.0f,0.0f,0.0f);
	int total = CommanderWaypointSystem::MAX_TEAMS * CommanderWaypointSystem::MAX_PER_TEAM;

	for (int i = 0; i < total; i++) {

		const CommanderWaypointStruct & waypoint = CommanderWaypointSystem::Get_Waypoint (i);
		if (waypoint.Team != team) continue;
		if (_DrawnMarkers >= MAX_MARKERS) break;

		Vector4 color = Color_For_Kind (waypoint.Kind);

		//	Fade out over the last second of life rather than vanishing, so a marker expiring
		//	reads as "that order is over" instead of as a dropped frame.
		float fade = (waypoint.SecondsLeft < 1.0f) ? waypoint.SecondsLeft : 1.0f;
		if (fade < 0.0f) fade = 0.0f;
		color.W *= fade * pulse;

		const Vector3 & base = waypoint.Position;

		//	The ring, on the ground.
		float previous_angle = 0.0f;
		for (int segment = 0; segment < RING_SEGMENTS; segment++) {

			float next_angle = (float)(segment + 1) * (2.0f * WWMATH_PI / (float)RING_SEGMENTS);

			Vector3 head (base.X + RING_RADIUS * WWMath::Cos (previous_angle),
							  base.Y + RING_RADIUS * WWMath::Sin (previous_angle),
							  base.Z);
			Vector3 tail (base.X + RING_RADIUS * WWMath::Cos (next_angle),
							  base.Y + RING_RADIUS * WWMath::Sin (next_angle),
							  base.Z);

			Add_Segment (head,tail,color);
			previous_angle = next_angle;
		}

		//	The column, so the marker is visible from ground level where a flat ring would be
		//	edge-on and effectively invisible -- which is exactly the view an infantry player has.
		Add_Segment (base,Vector3 (base.X,base.Y,base.Z + COLUMN_HEIGHT),color);

		center_sum += base;
		_DrawnMarkers++;
	}

	if (_SegmentCount <= 0) return;

	_BatchCenter = center_sum * (1.0f / (float)_DrawnMarkers);

	//	Attached lazily, the same way the debug display attaches itself: COMBAT_SCENE does not
	//	exist when Init runs, and a marker is rare enough that paying for the scene object only
	//	once anything is actually drawn is worth the branch.
	if (_TheSceneObject == nullptr) {
		_TheSceneObject = NEW_REF (CommanderWaypointSceneObjectClass,());
		COMBAT_SCENE->Add_Render_Object (_TheSceneObject);
	}
}


int CommanderWaypointRenderer::Get_Drawn_Marker_Count(void)	{ return _DrawnMarkers; }
