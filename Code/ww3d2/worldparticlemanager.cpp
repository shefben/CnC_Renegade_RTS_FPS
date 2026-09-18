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
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/worldparticlemanager.cpp               $*
 *                                                                                             *
 *	Roadmap Section 26 -- the owner of the three shared particle pools.  The header says why.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldparticlemanager.h"

#include "camera.h"
#include "dx8wrapper.h"
#include "linegrp.h"
#include "pointgr.h"
#include "refcount.h"
#include "rinfo.h"
#include "sharebuf.h"
#include "texture.h"
#include "wwdebug.h"

#include <string.h>


/*
**	How many distinct batches one pool may hold before it stops merging and starts refusing.
**	A batch is a texture and a blend mode, so this is a count of materials on screen at once and
**	not a count of effects; a level that wanted more than this many distinct particle materials
**	visible simultaneously would be paying that many draw calls in the stock engine too.
*/
enum { PARTICLE_MAX_BATCHES = 64 };

/*
**	How many emitters may contribute to one pool in one flush.  Each one costs a run -- a range
**	of staged particles belonging to one batch -- and the runs are what a batch is gathered from.
*/
enum { PARTICLE_MAX_RUNS = 512 };

/*
**	The share of a pool that CRITICAL_GAMEPLAY keeps out of everyone else's reach.  It is fixed
**	rather than settable because a guarantee that can be tuned to nothing is not a guarantee.
*/
static const float _CriticalReserveShare = 0.25f;

static const int _DefaultPoolCapacity[PARTICLE_POOL_COUNT] =
{
	8192,		// PARTICLE_POOL_SPRITE -- quads, which is nearly all stock content
	4096,		// PARTICLE_POOL_POINT  -- tris
	4096		// PARTICLE_POOL_STREAK -- line groups, four or six vertices each
};

static const float _DefaultBudgetShare[PARTICLE_BUDGET_COUNT] =
{
	_CriticalReserveShare,		// CRITICAL_GAMEPLAY, not settable
	0.35f,							// COMBAT_NEAR
	0.15f,							// COMBAT_FAR
	0.15f,							// ENVIRONMENT
	0.10f								// AMBIENT
};

/*
**	The live shares, seeded from the defaults at Init.  Declared here rather than with the rest of
**	the module state because the pool's own cap arithmetic reads them.
*/
static float _BudgetShare[PARTICLE_BUDGET_COUNT];

static const char * _PoolNames[PARTICLE_POOL_COUNT] =
{
	"sprite","point","streak"
};

static const char * _BudgetNames[PARTICLE_BUDGET_COUNT] =
{
	"CRITICAL_GAMEPLAY","COMBAT_NEAR","COMBAT_FAR","ENVIRONMENT","AMBIENT"
};


/*
**	What makes two submissions the same draw.
**
**	Texture and shader are the state a draw call installs, so they have to match.  The mode is
**	the point shape -- three vertices or four, tetrahedron or prism -- because it decides which
**	index buffer the draw walks.  The frame row/column count is the subdivision of the texture
**	the frame index means, so two emitters using the same texture with different frame layouts
**	mean different things by the same number.
**
**	GradientDisabled is the one that is not obviously state.  PointGroupClass and LineGroupClass
**	both decide, at draw time, to turn the primary gradient off for a submission that has no
**	diffuse array, a white opaque default and a texture -- an optimisation for untinted content.
**	It is a shader bit, so merging a submission that gets it with one that does not would change
**	the image for one of them.  Keeping it in the key is what makes "disabling the manager
**	restores the stock image exactly" true rather than nearly true.
*/
struct ParticleBatchKeyStruct
{
	TextureClass *		Texture;
	unsigned int		ShaderBits;
	int					Mode;
	unsigned char		FrameRowColumnLog2;
	bool					GradientDisabled;

	bool Matches(const ParticleBatchKeyStruct & that) const
	{
		return	Texture == that.Texture &&
					ShaderBits == that.ShaderBits &&
					Mode == that.Mode &&
					FrameRowColumnLog2 == that.FrameRowColumnLog2 &&
					GradientDisabled == that.GradientDisabled;
	}
};


/*
**	A contiguous range of staged particles belonging to one batch.  One submission makes one run.
*/
struct ParticleRunStruct
{
	int	Start;
	int	Count;
	int	NextRun;		// -1 terminates the batch's chain
};


struct ParticleBatchStruct
{
	ParticleBatchKeyStruct	Key;
	int							FirstRun;
	int							LastRun;
	int							ParticleCount;
};


/*
**	One shared pool.  The arrays are allocated at Init and never grow; Count is how much of the
**	pool the current flush has spoken for.
*/
class ParticlePoolClass
{
public:
	ParticlePoolClass(void);
	~ParticlePoolClass(void);

	void		Allocate(int capacity,bool streak);
	void		Free(void);
	void		Reset(void);
	void		Recompute_Caps(void);

	int		Capacity;
	int		Count;
	bool		IsStreak;

	ShareBufferClass<Vector3> *			Position;
	ShareBufferClass<Vector3> *			TailPosition;
	ShareBufferClass<Vector4> *			Diffuse;
	ShareBufferClass<Vector4> *			TailDiffuse;
	ShareBufferClass<float> *				Size;
	ShareBufferClass<unsigned char> *	Orientation;
	ShareBufferClass<unsigned char> *	Frame;
	ShareBufferClass<float> *				UCoord;
	ShareBufferClass<unsigned int> *		APT;

	ParticleBatchStruct	Batches[PARTICLE_MAX_BATCHES];
	int						BatchCount;
	ParticleRunStruct		Runs[PARTICLE_MAX_RUNS];
	int						RunCount;

	int						BudgetUsed[PARTICLE_BUDGET_COUNT];

	/*
	**	What each class asked for since the last flush, and what each class is allowed this
	**	time.  The cap is computed at the end of a flush from the demand that flush saw, which
	**	is the only way to know whether there is contention at all: a submission arriving on its
	**	own cannot be told apart from the first of five hundred, so a rule that decides at
	**	submission time has to assume the worst and ends up rationing a pool that is nearly
	**	empty.  One flush of lag after a sudden change is the price, and it buys the property
	**	that matters -- while the total fits, no class is capped and the image is the stock one.
	*/
	int						BudgetDemand[PARTICLE_BUDGET_COUNT];
	int						BudgetCap[PARTICLE_BUDGET_COUNT];

	// Statistics, reset by Reset_Statistics and not by a flush.
	unsigned int			SubmittedParticles;
	unsigned int			DrawnParticles;
	unsigned int			ThinnedParticles;
	unsigned int			BatchesDrawn;
	unsigned int			SubmissionsTaken;
	unsigned int			SubmissionsRefused;
	unsigned int			BudgetParticles[PARTICLE_BUDGET_COUNT];
};


ParticlePoolClass::ParticlePoolClass(void) :
	Capacity(0),
	Count(0),
	IsStreak(false),
	Position(nullptr),
	TailPosition(nullptr),
	Diffuse(nullptr),
	TailDiffuse(nullptr),
	Size(nullptr),
	Orientation(nullptr),
	Frame(nullptr),
	UCoord(nullptr),
	APT(nullptr),
	BatchCount(0),
	RunCount(0),
	SubmittedParticles(0),
	DrawnParticles(0),
	ThinnedParticles(0),
	BatchesDrawn(0),
	SubmissionsTaken(0),
	SubmissionsRefused(0)
{
	for (int i = 0; i < PARTICLE_BUDGET_COUNT; i++) {
		BudgetUsed[i] = 0;
		BudgetDemand[i] = 0;
		BudgetCap[i] = 0x7FFFFFFF;
		BudgetParticles[i] = 0;
	}
}


ParticlePoolClass::~ParticlePoolClass(void)
{
	Free();
}


void ParticlePoolClass::Allocate(int capacity,bool streak)
{
	Free();

	if (capacity < 1) capacity = 1;

	Capacity = capacity;
	IsStreak = streak;

	Position		= NEW_REF( ShareBufferClass<Vector3> , (capacity) );
	Diffuse		= NEW_REF( ShareBufferClass<Vector4> , (capacity) );
	Size			= NEW_REF( ShareBufferClass<float> , (capacity) );
	APT			= NEW_REF( ShareBufferClass<unsigned int> , (capacity) );

	if (streak) {
		TailPosition	= NEW_REF( ShareBufferClass<Vector3> , (capacity) );
		TailDiffuse		= NEW_REF( ShareBufferClass<Vector4> , (capacity) );
		UCoord			= NEW_REF( ShareBufferClass<float> , (capacity) );
	} else {
		Orientation		= NEW_REF( ShareBufferClass<unsigned char> , (capacity) );
		Frame				= NEW_REF( ShareBufferClass<unsigned char> , (capacity) );
	}

	Reset();
}


void ParticlePoolClass::Free(void)
{
	REF_PTR_RELEASE(Position);
	REF_PTR_RELEASE(TailPosition);
	REF_PTR_RELEASE(Diffuse);
	REF_PTR_RELEASE(TailDiffuse);
	REF_PTR_RELEASE(Size);
	REF_PTR_RELEASE(Orientation);
	REF_PTR_RELEASE(Frame);
	REF_PTR_RELEASE(UCoord);
	REF_PTR_RELEASE(APT);

	Reset();
	Capacity = 0;
}


/*
**	Empty the pool without forgetting the statistics.  Every batch holds a reference to its
**	texture, taken when the batch was opened, because the emitter that submitted it may well be
**	released before the flush that draws it -- a particle buffer whose only reference was the
**	static sort list is released the moment it returns from Render.
*/
void ParticlePoolClass::Reset(void)
{
	for (int i = 0; i < BatchCount; i++) {
		if (Batches[i].Key.Texture != nullptr) {
			Batches[i].Key.Texture->Release_Ref();
			Batches[i].Key.Texture = nullptr;
		}
	}

	Recompute_Caps();

	Count = 0;
	BatchCount = 0;
	RunCount = 0;

	for (int b = 0; b < PARTICLE_BUDGET_COUNT; b++) {
		BudgetUsed[b] = 0;
		BudgetDemand[b] = 0;
	}
}


/*
**	Turn what was asked for into what may be had next time.
**
**	While everything fits, nothing is capped: that is the whole point, and it is why the caps are
**	an enormous number rather than a share when there is no contention.  Under contention every
**	class is first given the smaller of what it asked for and its reserve, so a class cannot be
**	starved by a louder one, and whatever is left over is then handed out in enum order -- which
**	is priority order, CRITICAL_GAMEPLAY first and AMBIENT last.
*/
void ParticlePoolClass::Recompute_Caps(void)
{
	int total = 0;
	for (int b = 0; b < PARTICLE_BUDGET_COUNT; b++) {
		total += BudgetDemand[b];
	}

	if (total <= Capacity) {
		for (int b = 0; b < PARTICLE_BUDGET_COUNT; b++) {
			BudgetCap[b] = 0x7FFFFFFF;
		}
		return;
	}

	int left = Capacity;

	for (int b = 0; b < PARTICLE_BUDGET_COUNT; b++) {

		float share = (b == PARTICLE_BUDGET_CRITICAL_GAMEPLAY) ? _CriticalReserveShare : _BudgetShare[b];
		int reserve = (int)(share * (float)Capacity);

		/*
		**	Critical gameplay holds its whole reserve whether or not it asked for anything last
		**	time, because the case it exists for is exactly the one where it was not there a
		**	moment ago: a beacon is planted, and the plume that tells everyone so must not be
		**	the thing that loses to smoke.  Every other class is granted what it actually used.
		*/
		int grant = reserve;
		if (b != PARTICLE_BUDGET_CRITICAL_GAMEPLAY && BudgetDemand[b] < reserve) {
			grant = BudgetDemand[b];
		}

		if (grant > left) grant = left;
		BudgetCap[b] = grant;
		left -= grant;
	}

	for (int b = 0; b < PARTICLE_BUDGET_COUNT && left > 0; b++) {
		int extra = BudgetDemand[b] - BudgetCap[b];
		if (extra <= 0) continue;
		if (extra > left) extra = left;
		BudgetCap[b] += extra;
		left -= extra;
	}
}


/*
**	Module state.
*/
static ParticlePoolClass	_Pools[PARTICLE_POOL_COUNT];
static bool						_Initialised = false;
static bool						_Enabled = true;
static float					_CombatNearDistance = 60.0f;
static int						_RequestedCapacity[PARTICLE_POOL_COUNT];

/*
**	Where the frame being drawn is being drawn from.  It is what resolves an AUTO submission into
**	COMBAT_NEAR or COMBAT_FAR, and the emitter is told to keep it current rather than being asked
**	to classify itself, because an emitter knows where it is and nothing else.
*/
static Vector3					_CameraPosition(0.0f,0.0f,0.0f);

/*
**	The two renderers every batch goes through.  One of each, shared by every batch and by both
**	of the point-shaped pools, because their own vertex arrays are sized from the staging array
**	handed to them and a second instance would be a second copy of that.
**
**	They are built on first use rather than in Init: constructing either one asks
**	VertexMaterialClass::Get_Preset for a material, and that dereferences a null until a client
**	has run VertexMaterialClass::Init.  A dedicated server never does, and neither does a self
**	check, and both of those are expected to be able to call everything else here.
*/
static PointGroupClass *	_PointRenderer = nullptr;
static LineGroupClass *		_LineRenderer = nullptr;


ParticleSubmissionStruct::ParticleSubmissionStruct(void) :
	Texture(nullptr),
	PointMode(0),
	LineMode(0),
	FrameRowColumnLog2(0),
	Position(nullptr),
	TailPosition(nullptr),
	Diffuse(nullptr),
	TailDiffuse(nullptr),
	Size(nullptr),
	Orientation(nullptr),
	Frame(nullptr),
	UCoord(nullptr),
	DefaultDiffuse(1.0f,1.0f,1.0f,1.0f),
	DefaultTailDiffuse(1.0f,1.0f,1.0f,1.0f),
	DefaultSize(1.0f),
	DefaultOrientation(0),
	DefaultFrame(0),
	DefaultUCoord(0.0f),
	APT(nullptr),
	Count(0),
	Center(0.0f,0.0f,0.0f),
	Budget(PARTICLE_BUDGET_AUTO)
{
}


void WorldParticleManager::Init(void)
{
	if (_Initialised) return;

	for (int b = 0; b < PARTICLE_BUDGET_COUNT; b++) {
		_BudgetShare[b] = _DefaultBudgetShare[b];
	}

	for (int p = 0; p < PARTICLE_POOL_COUNT; p++) {
		if (_RequestedCapacity[p] <= 0) {
			_RequestedCapacity[p] = _DefaultPoolCapacity[p];
		}
		_Pools[p].Allocate(_RequestedCapacity[p],p == PARTICLE_POOL_STREAK);
	}

	_Initialised = true;

	Reset_Statistics();
}


void WorldParticleManager::Shutdown(void)
{
	Release_Resources();

	for (int p = 0; p < PARTICLE_POOL_COUNT; p++) {
		_Pools[p].Free();
	}

	_Initialised = false;
}


void WorldParticleManager::Release_Resources(void)
{
	for (int p = 0; p < PARTICLE_POOL_COUNT; p++) {
		_Pools[p].Reset();
	}

	if (_PointRenderer != nullptr) {
		delete _PointRenderer;
		_PointRenderer = nullptr;
	}
	if (_LineRenderer != nullptr) {
		delete _LineRenderer;
		_LineRenderer = nullptr;
	}
}


void WorldParticleManager::Set_Enabled(bool onoff)
{
	if (_Enabled == onoff) return;

	/*
	**	Anything already staged was staged on the understanding that a flush would draw it.
	**	Turning the manager off has to drop it rather than leave it to be drawn later out of
	**	order, and the emitters will submit again next frame or draw themselves.
	*/
	if (!onoff) {
		for (int p = 0; p < PARTICLE_POOL_COUNT; p++) {
			_Pools[p].Reset();
		}
	}

	_Enabled = onoff;
}


bool WorldParticleManager::Is_Enabled(void)
{
	return _Enabled;
}


void WorldParticleManager::Set_Pool_Capacity(ParticlePoolType pool,int capacity)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return;
	if (capacity < 1) capacity = 1;

	_RequestedCapacity[pool] = capacity;

	if (_Initialised) {
		_Pools[pool].Allocate(capacity,pool == PARTICLE_POOL_STREAK);
	}
}


int WorldParticleManager::Get_Pool_Capacity(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].Capacity;
}


void WorldParticleManager::Set_Budget_Share(ParticleBudgetType budget,float share)
{
	if (budget <= PARTICLE_BUDGET_CRITICAL_GAMEPLAY || budget >= PARTICLE_BUDGET_COUNT) return;

	if (share < 0.0f) share = 0.0f;
	if (share > 1.0f) share = 1.0f;

	_BudgetShare[budget] = share;
}


float WorldParticleManager::Get_Budget_Share(ParticleBudgetType budget)
{
	if (budget < 0 || budget >= PARTICLE_BUDGET_COUNT) return 0.0f;
	return _BudgetShare[budget];
}


void WorldParticleManager::Set_Camera_Position(const Vector3 & position)
{
	_CameraPosition = position;
}


void WorldParticleManager::Set_Combat_Near_Distance(float distance)
{
	_CombatNearDistance = (distance < 0.0f) ? 0.0f : distance;
}


float WorldParticleManager::Get_Combat_Near_Distance(void)
{
	return _CombatNearDistance;
}


/*
**	Which budget pays for this submission.  Anything but AUTO is taken as given -- the caller
**	knows something the manager does not -- and AUTO is the distance question, which is the only
**	one that can be answered without being told.
*/
static ParticleBudgetType Resolve_Budget(const ParticleSubmissionStruct & submission,const Vector3 & camera_position)
{
	if (submission.Budget >= 0 && submission.Budget < PARTICLE_BUDGET_COUNT) {
		return (ParticleBudgetType)submission.Budget;
	}

	float distance2 = (submission.Center - camera_position).Length2();
	float near2 = _CombatNearDistance * _CombatNearDistance;

	return (distance2 <= near2) ? PARTICLE_BUDGET_COMBAT_NEAR : PARTICLE_BUDGET_COMBAT_FAR;
}


/*
**	How many of the particles offered may be staged: whatever is left of the pool, and whatever
**	is left of this class's cap.  The cap is the interesting half and it was decided at the end
**	of the last flush -- see Recompute_Caps -- so this is deliberately the cheap end of the
**	arithmetic, because it runs once per emitter per frame.
*/
static int Admit(const ParticlePoolClass & pool,ParticleBudgetType budget,int want)
{
	int room = pool.Capacity - pool.Count;
	if (room <= 0) return 0;

	/*
	**	A cap may raise a class above its reserve; it may never take it below one.  Without that
	**	floor a class that was absent from the previous flush would have a cap of nothing and be
	**	refused outright -- so the first frame of a firefight, arriving into a scene that had
	**	only been making smoke, would show no firefight.  The floors can sum to more than the
	**	pool, which is why the room left in the pool is still the other half of this answer.
	*/
	float share = (budget == PARTICLE_BUDGET_CRITICAL_GAMEPLAY) ? _CriticalReserveShare : _BudgetShare[budget];
	int reserve = (int)(share * (float)pool.Capacity);

	int limit = pool.BudgetCap[budget];
	if (limit < reserve) limit = reserve;

	int cap = limit - pool.BudgetUsed[budget];
	if (cap < 0) cap = 0;

	int allowed = (cap < room) ? cap : room;

	return (want < allowed) ? want : allowed;
}


/*
**	Find the batch this submission belongs in, opening one if it is the first of its kind.
**	-1 means the pool is already holding as many distinct materials as it will hold.
*/
static int Find_Batch(ParticlePoolClass & pool,const ParticleBatchKeyStruct & key)
{
	for (int i = 0; i < pool.BatchCount; i++) {
		if (pool.Batches[i].Key.Matches(key)) {
			return i;
		}
	}

	if (pool.BatchCount >= PARTICLE_MAX_BATCHES) {
		return -1;
	}

	int index = pool.BatchCount++;
	pool.Batches[index].Key = key;
	pool.Batches[index].FirstRun = -1;
	pool.Batches[index].LastRun = -1;
	pool.Batches[index].ParticleCount = 0;

	if (key.Texture != nullptr) {
		key.Texture->Add_Ref();
	}

	return index;
}


bool WorldParticleManager::Submit(ParticlePoolType pool_type,const ParticleSubmissionStruct & submission)
{
	if (!_Initialised || !_Enabled) return false;
	if (pool_type < 0 || pool_type >= PARTICLE_POOL_COUNT) return false;

	ParticlePoolClass & pool = _Pools[pool_type];
	if (pool.Capacity <= 0 || pool.Position == nullptr) return false;

	/*
	**	A submission with nothing in it is still a submission the caller must not draw itself;
	**	saying no here would cost the draw call the batching exists to avoid.
	*/
	if (submission.Count <= 0 || submission.Position == nullptr) {
		pool.SubmissionsTaken++;
		return true;
	}

	if (pool.RunCount >= PARTICLE_MAX_RUNS) {
		pool.SubmissionsRefused++;
		return false;
	}

	/*
	**	Reproduce, exactly, the condition PointGroupClass and LineGroupClass use to decide whether
	**	the primary gradient is on.  Getting this wrong does not crash anything; it changes the
	**	colour of untinted particles, which is worse.
	*/
	const float value_255 = 0.9961f;
	bool default_white_opaque =	submission.DefaultDiffuse.X > value_255 &&
											submission.DefaultDiffuse.Y > value_255 &&
											submission.DefaultDiffuse.Z > value_255 &&
											submission.DefaultDiffuse.W > value_255;

	ParticleBatchKeyStruct key;
	key.Texture					= submission.Texture;
	key.ShaderBits				= submission.Shader.Get_Bits();
	key.Mode						= pool.IsStreak ? submission.LineMode : submission.PointMode;
	key.FrameRowColumnLog2	= submission.FrameRowColumnLog2;
	key.GradientDisabled		= (submission.Diffuse == nullptr) && default_white_opaque && (submission.Texture != nullptr);

	int batch_index = Find_Batch(pool,key);
	if (batch_index < 0) {
		pool.SubmissionsRefused++;
		return false;
	}

	ParticleBudgetType budget = Resolve_Budget(submission,_CameraPosition);

	int want = submission.Count;
	int take = Admit(pool,budget,want);

	pool.SubmittedParticles += (unsigned int)want;
	pool.SubmissionsTaken++;
	pool.BudgetDemand[budget] += want;

	if (take <= 0) {
		pool.ThinnedParticles += (unsigned int)want;
		return true;
	}

	int start = pool.Count;

	Vector3 *			position		= pool.Position->Get_Array();
	Vector4 *			diffuse		= pool.Diffuse->Get_Array();
	float *				size			= pool.Size->Get_Array();
	Vector3 *			tail_pos		= pool.TailPosition ? pool.TailPosition->Get_Array() : nullptr;
	Vector4 *			tail_diff	= pool.TailDiffuse ? pool.TailDiffuse->Get_Array() : nullptr;
	float *				ucoord		= pool.UCoord ? pool.UCoord->Get_Array() : nullptr;
	unsigned char *	orientation	= pool.Orientation ? pool.Orientation->Get_Array() : nullptr;
	unsigned char *	frame			= pool.Frame ? pool.Frame->Get_Array() : nullptr;

	for (int j = 0; j < take; j++) {

		/*
		**	Spread the kept particles evenly over the offered ones.  When take == want this is
		**	the identity and nothing is lost; when it is not, an emitter thins rather than being
		**	cut off part way through, which for a plume means it gets sparser and not shorter.
		*/
		int src = (take == want) ? j : (int)(((long long)j * (long long)want) / (long long)take);
		int idx = (submission.APT != nullptr) ? (int)submission.APT[src] : src;

		int dst = start + j;

		position[dst]	= submission.Position[idx];
		diffuse[dst]	= submission.Diffuse ? submission.Diffuse[idx] : submission.DefaultDiffuse;
		size[dst]		= submission.Size ? submission.Size[idx] : submission.DefaultSize;

		if (orientation != nullptr) {
			orientation[dst] = submission.Orientation ? submission.Orientation[idx] : submission.DefaultOrientation;
		}
		if (frame != nullptr) {
			frame[dst] = submission.Frame ? submission.Frame[idx] : submission.DefaultFrame;
		}
		if (tail_pos != nullptr) {
			tail_pos[dst] = submission.TailPosition ? submission.TailPosition[idx] : submission.Position[idx];
		}
		if (tail_diff != nullptr) {
			tail_diff[dst] = submission.TailDiffuse ? submission.TailDiffuse[idx] : submission.DefaultTailDiffuse;
		}
		if (ucoord != nullptr) {
			ucoord[dst] = submission.UCoord ? submission.UCoord[idx] : submission.DefaultUCoord;
		}
	}

	int run_index = pool.RunCount++;
	pool.Runs[run_index].Start		= start;
	pool.Runs[run_index].Count		= take;
	pool.Runs[run_index].NextRun	= -1;

	ParticleBatchStruct & batch = pool.Batches[batch_index];
	if (batch.FirstRun < 0) {
		batch.FirstRun = run_index;
	} else {
		pool.Runs[batch.LastRun].NextRun = run_index;
	}
	batch.LastRun = run_index;
	batch.ParticleCount += take;

	pool.Count += take;
	pool.BudgetUsed[budget] += take;
	pool.BudgetParticles[budget] += (unsigned int)take;
	pool.DrawnParticles += (unsigned int)take;
	pool.ThinnedParticles += (unsigned int)(want - take);

	return true;
}


/*
**	Gather one batch's runs into the active point table and draw it.
*/
static void Flush_Batch(ParticlePoolClass & pool,ParticleBatchStruct & batch,RenderInfoClass & rinfo,bool has_device)
{
	if (batch.ParticleCount <= 0) return;

	pool.BatchesDrawn++;

	/*
	**	One run covering the whole pool needs no table at all, which is the common case when a
	**	scene has one particle material in it: PointGroupClass then reads the staging arrays
	**	directly instead of gathering them.
	*/
	bool whole_pool =	(batch.FirstRun >= 0) &&
							(pool.Runs[batch.FirstRun].NextRun < 0) &&
							(pool.Runs[batch.FirstRun].Start == 0) &&
							(pool.Runs[batch.FirstRun].Count == pool.Count);

	ShareBufferClass<unsigned int> * apt = nullptr;

	if (!whole_pool) {
		unsigned int * table = pool.APT->Get_Array();
		int n = 0;
		for (int run = batch.FirstRun; run >= 0; run = pool.Runs[run].NextRun) {
			int end = pool.Runs[run].Start + pool.Runs[run].Count;
			for (int i = pool.Runs[run].Start; i < end; i++) {
				table[n++] = (unsigned int)i;
			}
		}
		WWASSERT(n == batch.ParticleCount);
		apt = pool.APT;
	}

	/*
	**	Everything above this line is the arithmetic -- which batch, which particles, in what
	**	order -- and it runs whether or not there is anything to draw on, so a check with no
	**	device exercises all of it.  Below it is the draw.
	*/
	if (!has_device) return;

	ShaderClass shader(batch.Key.ShaderBits);

	if (pool.IsStreak) {

		if (_LineRenderer == nullptr) {
			_LineRenderer = new LineGroupClass();
			_LineRenderer->Set_Flag(LineGroupClass::TRANSFORM,true);
		}

		_LineRenderer->Set_Texture(batch.Key.Texture);
		_LineRenderer->Set_Shader(shader);
		_LineRenderer->Set_Line_Mode((LineGroupClass::LineModeType)batch.Key.Mode);

		if (batch.Key.GradientDisabled) {
			_LineRenderer->Set_Line_Color(Vector3(1.0f,1.0f,1.0f));
			_LineRenderer->Set_Line_Alpha(1.0f);
			_LineRenderer->Set_Arrays(pool.Position,pool.TailPosition,nullptr,pool.TailDiffuse,
				apt,pool.Size,pool.UCoord,batch.ParticleCount);
		} else {
			_LineRenderer->Set_Arrays(pool.Position,pool.TailPosition,pool.Diffuse,pool.TailDiffuse,
				apt,pool.Size,pool.UCoord,batch.ParticleCount);
		}

		_LineRenderer->Render(rinfo);

	} else {

		if (_PointRenderer == nullptr) {
			_PointRenderer = new PointGroupClass();
			_PointRenderer->Set_Flag(PointGroupClass::TRANSFORM,true);
		}

		_PointRenderer->Set_Texture(batch.Key.Texture);
		_PointRenderer->Set_Shader(shader);
		_PointRenderer->Set_Point_Mode((PointGroupClass::PointModeEnum)batch.Key.Mode);
		_PointRenderer->Set_Frame_Row_Column_Count_Log2(batch.Key.FrameRowColumnLog2);

		if (batch.Key.GradientDisabled) {
			_PointRenderer->Set_Point_Color(Vector3(1.0f,1.0f,1.0f));
			_PointRenderer->Set_Point_Alpha(1.0f);
			_PointRenderer->Set_Arrays(pool.Position,nullptr,apt,pool.Size,
				pool.Orientation,pool.Frame,batch.ParticleCount);
		} else {
			_PointRenderer->Set_Arrays(pool.Position,pool.Diffuse,apt,pool.Size,
				pool.Orientation,pool.Frame,batch.ParticleCount);
		}

		_PointRenderer->Render(rinfo);
	}
}


void WorldParticleManager::Flush(RenderInfoClass & rinfo)
{
	if (!_Initialised) return;

	_CameraPosition = rinfo.Camera.Get_Position();

	bool has_device = (DX8Wrapper::_Get_D3D_Device8() != nullptr);

	for (int p = 0; p < PARTICLE_POOL_COUNT; p++) {

		ParticlePoolClass & pool = _Pools[p];
		if (pool.Count <= 0) {
			pool.Reset();
			continue;
		}

		for (int b = 0; b < pool.BatchCount; b++) {
			Flush_Batch(pool,pool.Batches[b],rinfo,has_device);
		}

		pool.Reset();
	}
}


void WorldParticleManager::Reset_Statistics(void)
{
	for (int p = 0; p < PARTICLE_POOL_COUNT; p++) {
		ParticlePoolClass & pool = _Pools[p];
		pool.SubmittedParticles	= 0;
		pool.DrawnParticles		= 0;
		pool.ThinnedParticles	= 0;
		pool.BatchesDrawn			= 0;
		pool.SubmissionsTaken	= 0;
		pool.SubmissionsRefused	= 0;
		for (int b = 0; b < PARTICLE_BUDGET_COUNT; b++) {
			pool.BudgetParticles[b] = 0;
		}
	}
}


unsigned int WorldParticleManager::Get_Submitted_Particles(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].SubmittedParticles;
}


unsigned int WorldParticleManager::Get_Drawn_Particles(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].DrawnParticles;
}


unsigned int WorldParticleManager::Get_Thinned_Particles(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].ThinnedParticles;
}


unsigned int WorldParticleManager::Get_Batch_Count(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].BatchesDrawn;
}


unsigned int WorldParticleManager::Get_Submission_Count(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].SubmissionsTaken;
}


unsigned int WorldParticleManager::Get_Refused_Submission_Count(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].SubmissionsRefused;
}


unsigned int WorldParticleManager::Get_Budget_Particles(ParticlePoolType pool,ParticleBudgetType budget)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	if (budget < 0 || budget >= PARTICLE_BUDGET_COUNT) return 0;
	return _Pools[pool].BudgetParticles[budget];
}


int WorldParticleManager::Get_Pending_Particles(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].Count;
}


int WorldParticleManager::Get_Pending_Batches(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return 0;
	return _Pools[pool].BatchCount;
}


const char * WorldParticleManager::Get_Pool_Name(ParticlePoolType pool)
{
	if (pool < 0 || pool >= PARTICLE_POOL_COUNT) return "?";
	return _PoolNames[pool];
}


const char * WorldParticleManager::Get_Budget_Name(ParticleBudgetType budget)
{
	if (budget < 0 || budget >= PARTICLE_BUDGET_COUNT) return "?";
	return _BudgetNames[budget];
}
