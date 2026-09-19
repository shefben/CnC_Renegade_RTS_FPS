#include "debrissystem.h"

#include "debrisrenderer.h"

#include "assetmgr.h"
#include "bridgesystem.h"
#include "building.h"
#include "camera.h"
#include "combat.h"
#include "crandom.h"
#include "pscene.h"
#include "rbody.h"
#include "rendobj.h"
#include "rendercomponent.h"
#include "rinfo.h"
#include "timemgr.h"
#include "wwdebug.h"

enum { GAMEPLAY_DEBRIS_MAX = 32 };

static bool		_Enabled = true;
static int		_MaxGameplay = 16;
static int		_GameplayLiveCount = 0;
static int		_GameplayDroppedCount = 0;

static RigidBodyClass *	_GameplayFragment [GAMEPLAY_DEBRIS_MAX];
static float				_GameplayRemaining [GAMEPLAY_DEBRIS_MAX];

//	Permanent -- RigidBodyClass::Init keeps a pointer to the definition it is given (see
//	PhysClass::Init), so this has to outlive every fragment built from it.  Nothing on it is
//	ever set: every field a fragment needs (model, mass, transform, velocity, collision group)
//	is set on the instance afterward, the same way BeamEffectManagerClass builds a
//	TimedDecorationPhysClass without ever touching a definition file.
static RigidBodyDefClass	_TheDebrisFragmentDef;


/**
** DebrisSystemClass
**
** One persistent scene object, created the first time a visual-tier chip is spawned and
** never removed, that draws every VISUAL_DEBRIS chip through DebrisRenderer -- the render
** half of the pool, exactly the way TracerSystemClass (Code/Combat/bullet.cpp) draws
** TracerRenderer's pool.  GAMEPLAY_DEBRIS fragments do not need one of these: each is a real
** RigidBodyClass, its own scene object already drawn by the ordinary scene walk.
*/
class DebrisSystemClass : public RenderObjClass, public IRenderComponent
{
public:
	DebrisSystemClass( void )		{ Set_Transform( Matrix3D(1) ); }

	virtual RenderObjClass *	Clone( void ) const override	{ WWASSERT(false); return (nullptr); }

	virtual void	Render( RenderInfoClass &rinfo ) override
	{
		DebrisRenderer::Render( rinfo, rinfo.Camera.Get_Position() );
	}

	//	Section 29's IRenderComponent -- same render, reached through the shared contract instead
	//	of only through the scene's RenderObjClass walk.
	virtual void	Render_Component( RenderInfoClass &rinfo ) override
	{
		Render( rinfo );
	}

	virtual void	Get_Obj_Space_Bounding_Sphere( SphereClass &sphere ) const override
	{
		Vector3 minimum, maximum;
		DebrisRenderer::Get_Bounds( minimum, maximum );
		sphere.Init( (minimum + maximum) * 0.5f, ((maximum - minimum) * 0.5f).Length() );
	}

	virtual void	Get_Obj_Space_Bounding_Box( AABoxClass &box ) const override
	{
		Vector3 minimum, maximum;
		DebrisRenderer::Get_Bounds( minimum, maximum );
		box.Init( (minimum + maximum) * 0.5f, (maximum - minimum) * 0.5f );
	}
};

static DebrisSystemClass *	_TheDebrisSystem = nullptr;

static void Ensure_Debris_System(void)
{
	if (_TheDebrisSystem == nullptr) {
		_TheDebrisSystem = NEW_REF( DebrisSystemClass, () );
		COMBAT_SCENE->Add_Render_Object( _TheDebrisSystem );
	}
}

//	Tries one real-collision fragment.  False means the caller's remaining count -- this piece
//	and everything after it it chooses not to keep trying for -- belongs to the visual tier
//	instead: either the model has no art yet (WW3DAssetManager returned nothing, the sane
//	default the missing-art rule asks for) or the gameplay pool is already at its cap.
static bool Try_Spawn_Gameplay_Fragment(const Vector3 & position,const char * model_name,float lifetime)
{
	if (_GameplayLiveCount >= _MaxGameplay) {
		_GameplayDroppedCount++;
		return (false);
	}

	RenderObjClass * model = WW3DAssetManager::Get_Instance()->Create_Render_Obj( model_name );
	if (model == nullptr) {
		return (false);
	}

	RigidBodyClass * frag = NEW_REF( RigidBodyClass, () );
	frag->Init( _TheDebrisFragmentDef );

	//	Set_Model builds the fragment's collision box and inertia from the model itself
	//	(RigidBodyClass::Update_Cached_Model_Parameters); Set_Mass has to come after, so the
	//	inertia it recomputes has a real box to measure instead of the identity box a model-less
	//	fragment would leave behind.
	frag->Set_Model( model );
	model->Release_Ref();
	frag->Set_Mass( 30.0f );
	frag->Set_Collision_Group( DEFAULT_COLLISION_GROUP );

	Matrix3D tm(1);
	tm.Set_Translation( position );
	frag->Set_Transform( tm );

	Vector3 kick( FreeRandom.Get_Float(-4.0f,4.0f), FreeRandom.Get_Float(-4.0f,4.0f), FreeRandom.Get_Float(3.0f,7.0f) );
	frag->Set_Velocity( kick );
	frag->Set_Angular_Velocity( Vector3( FreeRandom.Get_Float(-4.0f,4.0f), FreeRandom.Get_Float(-4.0f,4.0f), FreeRandom.Get_Float(-4.0f,4.0f) ) );

	//	One reference, kept for as long as the fragment is alive; released in
	//	Update_Gameplay_Fragments or Reset, the same single-owner convention
	//	BulletManager keeps for _TheTracerSystem.
	COMBAT_SCENE->Add_Dynamic_Object( frag );

	int slot = _GameplayLiveCount++;
	_GameplayFragment [slot] = frag;
	_GameplayRemaining [slot] = lifetime;

	return (true);
}

static void Update_Gameplay_Fragments(float seconds)
{
	int i = 0;
	while (i < _GameplayLiveCount) {
		_GameplayRemaining [i] -= seconds;
		if (_GameplayRemaining [i] <= 0.0f) {

			RigidBodyClass * frag = _GameplayFragment [i];
			COMBAT_SCENE->Remove_Object( frag );
			REF_PTR_RELEASE( frag );

			int last = _GameplayLiveCount - 1;
			_GameplayFragment [i] = _GameplayFragment [last];
			_GameplayRemaining [i] = _GameplayRemaining [last];
			_GameplayLiveCount--;

		} else {
			i++;
		}
	}
}

//	Everything BridgeSystem has recorded since the last drain -- see
//	Code/wwphys/bridgesystem.h's BridgeDebrisClass and docs/assets/BridgeSections.md.  Each
//	entry names how many pieces came off, where, and (for the three default bridge kinds) what
//	model represents them; Spawn resolves the model itself, so an undefined kind or a kind that
//	names no debris model still degrades to the visual tier rather than being skipped.
static void Drain_Bridge_Debris(void)
{
	int count = BridgeSystem::Get_Pending_Debris_Count();
	if (count <= 0) return;

	static const Vector4 RUBBLE_COLOR (0.42f,0.38f,0.34f,1.0f);

	for (int i = 0; i < count; i++) {
		const BridgeDebrisClass * debris = BridgeSystem::Peek_Pending_Debris(i);
		if ((debris == nullptr) || (debris->Count <= 0)) continue;

		const char * model_name = (debris->Model.Get_Length() > 0) ? debris->Model.Peek_Buffer() : nullptr;

		DebrisManager::Spawn( debris->Position,3.0f,debris->Count,debris->Count,
							   model_name,RUBBLE_COLOR,4.0f );
	}

	BridgeSystem::Clear_Pending_Debris();
}

void DebrisManager::Init(void)
{
	DebrisRenderer::Init();
}

void DebrisManager::Shutdown(void)
{
	if (_TheDebrisSystem != nullptr) {
		_TheDebrisSystem->Remove();
		REF_PTR_RELEASE( _TheDebrisSystem );
	}
	DebrisRenderer::Shutdown();

	Reset();
}

void DebrisManager::Reset(void)
{
	DebrisRenderer::Reset();

	for (int i = 0; i < _GameplayLiveCount; i++) {
		RigidBodyClass * frag = _GameplayFragment [i];
		COMBAT_SCENE->Remove_Object( frag );
		REF_PTR_RELEASE( frag );
	}
	_GameplayLiveCount = 0;
	_GameplayDroppedCount = 0;
}

void DebrisManager::Set_Enabled(bool onoff)
{
	_Enabled = onoff;
	DebrisRenderer::Set_Enabled( onoff );
}

bool DebrisManager::Is_Enabled(void)
{
	return (_Enabled);
}

void DebrisManager::Update(void)
{
	float seconds = TimeManager::Get_Frame_Seconds();

	//	Both tiers age every simulation frame regardless of whether the layer is on screen
	//	right now -- see DebrisRenderer::Age for why that has to be separate from the draw call.
	DebrisRenderer::Age( seconds );
	Update_Gameplay_Fragments( seconds );

	Drain_Bridge_Debris();
}

void DebrisManager::Spawn(const Vector3 & center,float radius,int total_count,int gameplay_count,
								  const char * model_name,const Vector4 & color,float lifetime)
{
	if (!_Enabled) return;
	if (total_count <= 0) return;
	if (radius < 0.0f) radius = 0.0f;

	int gameplay_spawned = 0;

	if ((model_name != nullptr) && (model_name [0] != 0)) {

		int wanted = (gameplay_count < total_count) ? gameplay_count : total_count;

		for (int n = 0; n < wanted; n++) {
			Vector3 offset( FreeRandom.Get_Float(-radius,radius), FreeRandom.Get_Float(-radius,radius),
							 FreeRandom.Get_Float(0.0f,radius) );

			if (!Try_Spawn_Gameplay_Fragment( center + offset,model_name,lifetime )) {
				//	A missing model will not resolve on a second try either, and a full pool
				//	will not have room on a second try either -- either way, everything from
				//	here on belongs to the visual tier.
				break;
			}
			gameplay_spawned++;
		}
	}

	int visual_count = total_count - gameplay_spawned;
	if (visual_count > 0) {
		Ensure_Debris_System();
		DebrisRenderer::Spawn( center,radius,visual_count,color,lifetime );
	}
}

void DebrisManager::Spawn_Building_Debris(BuildingGameObj * building)
{
	if (!_Enabled) return;
	if (building == nullptr) return;

	Vector3 position (0.0f,0.0f,0.0f);
	building->Get_Position( &position );

	//	A building is an aggregate of static meshes with no model of its own (see
	//	BuildingGameObj::Get_Bounds), so its collected bounding box is the radius to scatter
	//	rubble across instead of a single model's bounding box.
	const AABoxClass & box = building->Get_Bounds();
	float radius = (box.Extent.X + box.Extent.Y) * 0.5f;
	if (radius < 2.0f) radius = 2.0f;

	static const Vector4 RUBBLE_COLOR (0.42f,0.38f,0.34f,1.0f);

	//	Buildings name no debris model of their own the way BridgeDebrisClass does for a
	//	bridge, so every piece goes to the pooled visual tier -- gameplay_count of 0.
	Spawn( position,radius,14,0,nullptr,RUBBLE_COLOR,3.5f );
}

void DebrisManager::Set_Max_Gameplay(int count)
{
	if (count < 0) count = 0;
	if (count > GAMEPLAY_DEBRIS_MAX) count = GAMEPLAY_DEBRIS_MAX;
	_MaxGameplay = count;
}

int DebrisManager::Get_Max_Gameplay(void)
{
	return (_MaxGameplay);
}

int DebrisManager::Get_Live_Gameplay_Count(void)
{
	return (_GameplayLiveCount);
}

int DebrisManager::Get_Dropped_Gameplay_Count(void)
{
	return (_GameplayDroppedCount);
}
