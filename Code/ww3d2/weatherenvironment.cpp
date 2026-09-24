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

#include "weatherenvironment.h"

#include "assetmgr.h"
#include "camera.h"
#include "colmath.h"
#include "frustum.h"
#include "linegrp.h"
#include "pointgr.h"
#include "refcount.h"
#include "rinfo.h"
#include "texture.h"
#include "worldparticlemanager.h"
#include "ww3d.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <math.h>
#include <string.h>


/*
**	The mode table.
**
**	Rain, snow and ash are Renegade's own, and their numbers are the arguments the stock
**	RainSystemClass, SnowSystemClass and AshSystemClass handed to WeatherSystemClass, carried
**	over unchanged -- fall speed, emitter size and height, particles per unit length, particle
**	width and height, and the static period a landed particle lasts for.  Anything that looks
**	arbitrary here is arbitrary in the same way it was in 2002.
**
**	The frame band is the one part that is new.  The stock atlas is addressed as four pages
**	inside a horizontal strip, and the point renderer addresses frames as cells of a square
**	grid, so the strips have to line up with grid rows: at a 4x4 grid, snow's strip is row two
**	and ash's is row three, exactly.  Rain's strip is two rows tall and therefore is not a row
**	at all, which is one of two reasons rain is a streak instead (the other being that a
**	raindrop is three times longer than it is wide, and a sprite has only one size).
*/
static const WeatherModeStruct _DefaultModes[WEATHER_MODE_COUNT] =
{
	//	Rain.  A streak, untextured until ow_weather_rain.tga exists; see docs/assets/weather.md.
	{	"rain",		"ow_weather_rain.tga",	2,	0,	1,	true,	false,
		15.0f,	20.0f,	20.0f,	0.2f,	0.15f,	0.45f,
		0.1f,	0.2f,	true,	false,
		0.0f,	0.0f,
		Vector4(1.0f,1.0f,1.0f,1.0f) },

	//	Snow.  Row two of the stock atlas, four frames.
	{	"snow",		"WeatherParticles.tga",	2,	8,	4,	false,	false,
		3.5f,	40.0f,	20.0f,	0.1f,	0.32f,	0.32f,
		1.0f,	2.0f,	false,	true,
		0.75f,	0.35f,
		Vector4(1.0f,1.0f,1.0f,1.0f) },

	//	Ash.  Row three of the stock atlas, four frames.
	{	"ash",		"WeatherParticles.tga",	2,	12,	4,	false,	false,
		3.0f,	40.0f,	20.0f,	0.1f,	0.32f,	0.32f,
		1.0f,	2.0f,	false,	true,
		0.9f,	0.3f,
		Vector4(1.0f,1.0f,1.0f,1.0f) },

	/*
	**	The three modes Section 36 adds have no stock behaviour to preserve.  Dust and tiberium
	**	are not precipitation at all -- they hang in the air rather than land, which here is
	**	simply a very slow fall over a tall box and a long wander.
	*/
	{	"dust",		"ow_weather_dust.tga",	2,	0,	4,	false,	false,
		0.6f,	40.0f,	24.0f,	0.05f,	0.5f,	0.5f,
		0.0f,	0.0f,	false,	false,
		1.6f,	0.2f,
		Vector4(0.82f,0.74f,0.60f,0.45f) },

	{	"tiberium",	"ow_weather_tiberium.tga",	2,	0,	4,	false,	true,
		0.4f,	30.0f,	20.0f,	0.04f,	0.35f,	0.35f,
		0.0f,	0.0f,	false,	false,
		1.2f,	0.25f,
		Vector4(0.45f,1.0f,0.45f,0.8f) },

	{	"falling",	"ow_weather_falling.tga",	2,	0,	4,	false,	false,
		2.5f,	40.0f,	20.0f,	0.1f,	0.3f,	0.3f,
		0.5f,	1.0f,	false,	true,
		0.4f,	0.3f,
		Vector4(1.0f,1.0f,1.0f,1.0f) },
};


/*
**	Module state.
**
**	The budget is static because it belongs to the frame.  Three kinds of weather falling at
**	once are one scene's worth of weather, and letting each take the whole allowance would
**	make the bound a lie by a factor of three.
*/
static bool					_Enabled = true;
static int					_MaxParticles = 4096;
static int					_MaxCastsPerFrame = 256;

/*
**	What is left of each allowance this frame, and which frame that is.  Every instance checks
**	the stamp before spending, so whichever mode renders first in a frame opens the budget and
**	the rest share what remains.
*/
static unsigned int			_BudgetFrame = 0xFFFFFFFF;
static int					_BudgetParticles = 0;
static int					_BudgetCasts = 0;

/*
**	The staging arrays, shared by every instance because Render is not re-entrant, and fixed
**	because the whole point of the exercise is that weather does not allocate.
*/
enum { WEATHER_STAGING_MAX = 8192 };

static Vector3				_StagePosition[WEATHER_STAGING_MAX];
static Vector3				_StageTailPosition[WEATHER_STAGING_MAX];
static Vector4				_StageDiffuse[WEATHER_STAGING_MAX];
static float				_StageSize[WEATHER_STAGING_MAX];
static unsigned char		_StageFrame[WEATHER_STAGING_MAX];

/*
**	The finest lattice any weather is laid out on, in world units.  Every spacing is this
**	times a power of two, which is what lets the lattice coarsen under budget pressure without
**	the surviving columns moving: at twice the spacing, the odd columns simply stop existing.
*/
static const float			_BaseLatticeUnit = 0.25f;

/*
**	How far below the ceiling a column looks for ground, as a multiple of the fall height.
**	Past this a particle is falling into a hole deep enough that nobody is watching it land.
*/
static const float			_ColumnDepthScale = 4.0f;

/*
**	The top of the fall fades in over this fraction of the emitter height, which is the stock
**	alpha ramp -- without it a particle appears out of nothing at the ceiling.
*/
static const float			_FadeBandFraction = 0.2f;


/*
**	A hash, not a random number generator.
**
**	Everything that used to be randomised per particle at spawn -- where the fall line is
**	inside its cell, how far through its cycle it is, which frame of the atlas it shows -- is
**	now randomised per column by hashing the column's coordinates.  The difference matters:
**	a generator has to be called in order and remembered, and a hash can be asked about the
**	ten thousandth column without having asked about the first.
*/
static inline unsigned int Weather_Hash(unsigned int a,unsigned int b,unsigned int salt)
{
	unsigned int h = a * 0x9E3779B1u;
	h ^= (b + 0x85EBCA6Bu + (h << 6) + (h >> 2));
	h *= 0xC2B2AE35u;
	h ^= (salt + 0x27D4EB2Fu + (h << 5) + (h >> 3));
	h ^= h >> 15;
	return h;
}

static inline float Weather_Hash_Float(unsigned int a,unsigned int b,unsigned int salt)
{
	return (float)(Weather_Hash(a,b,salt) & 0x00FFFFFFu) * (1.0f / 16777216.0f);
}


/*
**	Round an index down to a multiple of step, for negative indices too.  The lattice has to be
**	anchored in the world rather than to the camera, or every column would move when the player
**	does, so the indices are signed and the naive remainder is wrong on half the map.
*/
static inline int Weather_Align_Down(int value,int step)
{
	int remainder = value % step;
	if (remainder < 0) remainder += step;
	return value - remainder;
}


static inline float Weather_Wrap(float value,float period)
{
	if (period <= 0.0f) return 0.0f;
	float wrapped = value - ((float)floor(value / period)) * period;
	if (wrapped < 0.0f) wrapped = 0.0f;
	if (wrapped >= period) wrapped = 0.0f;
	return wrapped;
}


WeatherEnvironmentRenderer::WeatherEnvironmentRenderer(WeatherModeType mode) :
	Mode(mode),
	Density(0.0f),
	Wind(0.0f,0.0f),
	CameraPosition(0.0f,0.0f,0.0f),
	CameraVelocity(0.0f,0.0f,0.0f),
	HasCameraSample(false),
	BoundsMin(0.0f,0.0f,0.0f),
	BoundsMax(0.0f,0.0f,0.0f),
	Phase(0.0f),
	Collision(nullptr),
	Columns(nullptr),
	Texture(nullptr),
	TextureChecked(false),
	DrawnParticles(0),
	ColumnCount(0),
	ColumnSpacing(0.0f),
	DeferredColumns(0),
	Casts(0),
	CacheHits(0)
{
	WWASSERT(mode >= 0 && mode < WEATHER_MODE_COUNT);
	Parms = _DefaultModes[mode];

	Columns = new ColumnStruct[COLUMN_CACHE_SIZE];
	Reset();
}


WeatherEnvironmentRenderer::~WeatherEnvironmentRenderer(void)
{
	delete [] Columns;
	Columns = nullptr;

	REF_PTR_RELEASE(Texture);
}


/*
**	The mode's art, or nothing at all.
**
**	A mode names the texture it wants and is drawn whether or not that file exists, which is
**	the rule for every asset this engine has not been given yet.  The care taken here is about
**	*how* it is drawn without one: the asset manager answers a missing name with a placeholder
**	image rather than a null, and a placeholder is the wrong answer ten thousand times over --
**	it is opaque and it is obviously not weather.  An untextured particle is a plain coloured
**	quad, which for rain is very nearly right and for everything else is at least quiet.
*/
TextureClass * WeatherEnvironmentRenderer::Peek_Texture(void)
{
	if (TextureChecked) return Texture;

	if (Parms.TextureName == nullptr || WW3DAssetManager::Get_Instance() == nullptr) {
		TextureChecked = true;
		return nullptr;
	}

	if (Texture == nullptr) {
		Texture = WW3DAssetManager::Get_Instance()->Get_Texture(Parms.TextureName,TextureClass::MIP_LEVELS_5);
		if (Texture != nullptr) {
			Texture->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
			Texture->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
		}
	}

	/*
	**	Whether the file was there is not known until the texture has been initialised, which
	**	happens on the first frame it is drawn, so the question is asked again each frame until
	**	it can be answered.
	*/
	if (Texture != nullptr && Texture->Is_Initialized()) {
		TextureChecked = true;
		if (Texture->Is_Missing_Texture()) {
			REF_PTR_RELEASE(Texture);
		}
	}

	return Texture;
}


void WeatherEnvironmentRenderer::Reset(void)
{
	for (int i = 0; i < COLUMN_CACHE_SIZE; i++) {
		Columns[i].Key = 0;
		Columns[i].FloorZ = 0.0f;
		Columns[i].Normal.Set(0.0f,0.0f,1.0f);
		Columns[i].Landed = false;
		Columns[i].Valid = false;
		Columns[i].Stamp = 0;
	}

	Phase = 0.0f;
}


void WeatherEnvironmentRenderer::Set_Density(float density)
{
	if (density < 0.0f) density = 0.0f;
	Density = density;
}


void WeatherEnvironmentRenderer::Set_Wind(const Vector2 & velocity)
{
	Wind = velocity;
}


void WeatherEnvironmentRenderer::Set_Collision_Provider(WeatherCollisionProviderClass * provider)
{
	if (provider != Collision) {
		Collision = provider;

		//	A cached floor was answered by the old provider, about a world the new one may not
		//	agree with.
		Reset();
	}
}


void WeatherEnvironmentRenderer::Update(float seconds,const Vector3 & camera_position)
{
	/*
	**	D-001 P23: the camera's own velocity, from where it was last frame, so rain can be slanted
	**	by it the way stock rain was.  A jump faster than anything that moves (a respawn, a
	**	camera cut) is a teleport, not a velocity, and leaves the streaks upright.
	*/
	const float TELEPORT_SPEED = 200.0f;
	CameraVelocity.Set(0.0f,0.0f,0.0f);
	if (HasCameraSample && (seconds > 0.0f)) {
		Vector3 velocity = (camera_position - CameraPosition) / seconds;
		if (velocity.Length() < TELEPORT_SPEED) {
			CameraVelocity = velocity;
		}
	}
	HasCameraSample = true;

	CameraPosition = camera_position;
	Phase += seconds;

	/*
	**	The clock is only ever read modulo a column's cycle, but it is read as a float, so it
	**	has to be kept small or the columns lose their phase to rounding after an hour.
	*/
	if (Phase > 3600.0f) {
		Phase -= 3600.0f;
	}

	/*
	**	The box has to contain where the particles are, not where their columns are.  A falling
	**	particle is upwind of its landing point by as much as a whole fall's worth of wind, and
	**	a wandering one is a wander's amplitude to the side of that, so both are added -- the
	**	scene culls this object as a whole, and a box that is too small would cull away weather
	**	that is on screen.
	*/
	float fall_seconds = (Parms.FallSpeed > 0.0f) ? (Parms.EmitterHeight / Parms.FallSpeed) : 0.0f;
	float drift = (WWMath::Fabs(Wind.X) + WWMath::Fabs(Wind.Y)) * fall_seconds;

	float reach = Parms.EmitterSize + drift + Parms.WanderAmplitude
						+ Parms.ParticleWidth + Parms.ParticleHeight;
	float ceiling = CameraPosition.Z + Parms.EmitterHeight;
	float bottom = ceiling - Parms.EmitterHeight * _ColumnDepthScale;

	BoundsMin.Set(CameraPosition.X - reach,CameraPosition.Y - reach,bottom);
	BoundsMax.Set(CameraPosition.X + reach,CameraPosition.Y + reach,ceiling);
}


void WeatherEnvironmentRenderer::Get_Bounds(Vector3 & minimum,Vector3 & maximum) const
{
	minimum = BoundsMin;
	maximum = BoundsMax;
}


/*
**	Where the ground is under one column, from the cache or from the world.
**
**	The cache is keyed on the lattice index rather than on anything about the camera, so an
**	entry stays true as the player walks away and is still true when they walk back.  That is
**	only possible because the cast is vertical, which in turn is only possible because the
**	lattice is anchored at the point a particle *lands* rather than the point it starts from:
**	wind slides the start of a fall upwind and leaves the landing where it is, so the answer
**	does not go stale every time the wind breathes.
*/
WeatherEnvironmentRenderer::ColumnStruct * WeatherEnvironmentRenderer::Find_Column(
	int ix,int iy,float ceiling,float bottom,bool & ready)
{
	ready = false;

	unsigned int key = (((unsigned int)ix & 0xFFFFu) << 16) | ((unsigned int)iy & 0xFFFFu);
	unsigned int slot = Weather_Hash((unsigned int)ix,(unsigned int)iy,0x5715u) & (COLUMN_CACHE_SIZE - 1);
	unsigned int frame = WW3D::Get_Frame_Count();

	ColumnStruct * victim = nullptr;

	for (int probe = 0; probe < 4; probe++) {

		ColumnStruct * entry = &Columns[(slot + probe) & (COLUMN_CACHE_SIZE - 1)];

		if (entry->Valid && entry->Key == key) {
			entry->Stamp = frame;
			CacheHits++;
			ready = true;
			return entry;
		}

		if (!entry->Valid) {
			victim = entry;
			break;
		}

		//	Evict whichever of the probed entries nobody has stood near for longest.
		if (victim == nullptr || entry->Stamp < victim->Stamp) {
			victim = entry;
		}
	}

	WWASSERT(victim != nullptr);

	float x = (float)ix * _BaseLatticeUnit;
	float y = (float)iy * _BaseLatticeUnit;

	/*
	**	With nobody to ask, weather falls through the world.  That is the dedicated server and
	**	the device-less check, where there is no image to be wrong, and it is also what a
	**	client does for the frame or two before the cast lands.
	*/
	if (Collision == nullptr) {
		victim->Key = key;
		victim->FloorZ = bottom;
		victim->Normal.Set(0.0f,0.0f,1.0f);
		victim->Landed = false;
		victim->Valid = true;
		victim->Stamp = frame;
		ready = true;
		return victim;
	}

	/*
	**	Casting is the only thing weather asks of the physics scene, so it is rationed.  A
	**	column that cannot be cast this frame is left out of this frame entirely rather than
	**	guessed at: a guessed floor is rain falling through a roof, and a missing column is
	**	one fall line's worth of thinner rain for a frame or two.
	*/
	if (_BudgetCasts <= 0) {
		DeferredColumns++;
		return nullptr;
	}

	_BudgetCasts--;
	Casts++;

	Vector3 top(x,y,ceiling);
	Vector3 floor_point(x,y,bottom);
	Vector3 normal(0.0f,0.0f,1.0f);
	bool landed = Collision->Cast_Weather_Column(top,Vector3(x,y,bottom),floor_point,normal);

	victim->Key = key;
	victim->FloorZ = floor_point.Z;
	victim->Normal = normal;
	victim->Landed = landed;
	victim->Valid = true;
	victim->Stamp = frame;

	ready = true;
	return victim;
}


void WeatherEnvironmentRenderer::Render(RenderInfoClass & rinfo)
{
	DrawnParticles = 0;
	ColumnCount = 0;
	ColumnSpacing = 0.0f;
	DeferredColumns = 0;
	Casts = 0;
	CacheHits = 0;

	if (!_Enabled) return;
	if (Density <= 0.0f) return;
	if (Parms.FallSpeed <= 0.0f) return;
	if (Parms.ParticlesPerUnitLength <= 0.0f) return;

	/*
	**	Open the frame's allowances if this is the first weather to render in it.
	*/
	unsigned int frame = WW3D::Get_Frame_Count();
	if (frame != _BudgetFrame) {
		_BudgetFrame = frame;
		Begin_Frame();
	}

	if (_BudgetParticles <= 0) return;

	/*
	**	How far apart the fall lines are.
	**
	**	The stock system derived a ray count from the density -- rays = density * size^2 /
	**	(particles-per-unit-length * speed) -- and scattered that many rays at random over the
	**	emitter.  Dividing by the emitter's area turns that into a statement that does not
	**	mention the emitter at all: columns per square unit is density / (4 * ppul * speed).
	**	The lattice is then just the spacing that gives that many, which is why changing the
	**	emitter size changes how much weather there is and not how thick it is.
	*/
	float columns_per_area = Density / (4.0f * Parms.ParticlesPerUnitLength * Parms.FallSpeed);
	if (columns_per_area <= 0.0f) return;

	float wanted_spacing = WWMath::Sqrt(1.0f / columns_per_area);

	/*
	**	Quantise down to the lattice, then take the fraction that overshoots out in columns:
	**	the spacing is a power of two so the lattice never slides, and the leftover density is
	**	made up by keeping only some of the columns, chosen by hash so the choice is stable
	**	from frame to frame.
	*/
	int step = 1;
	while (((float)step * _BaseLatticeUnit) * 2.0f <= wanted_spacing && step < (1 << 14)) {
		step *= 2;
	}

	float nominal_particles_per_column = Parms.ParticlesPerUnitLength * Parms.EmitterHeight;
	if (nominal_particles_per_column < 1.0f) nominal_particles_per_column = 1.0f;

	/*
	**	Coarsen until the estimate fits what is left of the frame.  This is the acceptance: the
	**	count is decided here, before a single particle is evaluated, so weather cannot grow
	**	with the size of the scene or the length of the storm.
	*/
	float span = Parms.EmitterSize * 2.0f;
	for (int guard = 0; guard < 16; guard++) {

		float spacing = (float)step * _BaseLatticeUnit;
		float across = span / spacing + 1.0f;
		float estimate = across * across * nominal_particles_per_column;

		if (estimate <= (float)_BudgetParticles) break;
		if (step >= (1 << 14)) break;
		step *= 2;
	}

	float spacing = (float)step * _BaseLatticeUnit;
	ColumnSpacing = spacing;

	//	What fraction of the lattice's columns are actually wanted at this density.
	float keep = (spacing * spacing) * columns_per_area;
	if (keep > 1.0f) keep = 1.0f;

	float ceiling = CameraPosition.Z + Parms.EmitterHeight;
	float bottom = ceiling - Parms.EmitterHeight * _ColumnDepthScale;

	/*
	**	The region of lattice to walk.  It is centred on the camera and widened by how far a
	**	particle can be displaced from its column -- upwind while it is still falling, and
	**	sideways by the wander -- so that weather which drifts into view is evaluated rather
	**	than appearing at the edge of the box.
	*/
	float fall_seconds = Parms.EmitterHeight / Parms.FallSpeed;
	float drift = (WWMath::Fabs(Wind.X) + WWMath::Fabs(Wind.Y)) * fall_seconds;
	float reach = Parms.EmitterSize + drift + Parms.WanderAmplitude;

	int i0 = Weather_Align_Down((int)floor((CameraPosition.X - reach) / _BaseLatticeUnit),step);
	int j0 = Weather_Align_Down((int)floor((CameraPosition.Y - reach) / _BaseLatticeUnit),step);
	int i1 = (int)floor((CameraPosition.X + reach) / _BaseLatticeUnit);
	int j1 = (int)floor((CameraPosition.Y + reach) / _BaseLatticeUnit);

	const FrustumClass & frustum = rinfo.Camera.Get_Frustum();

	/*
	**	Which way a particle appears to be going, for the streak modes: its velocity minus the
	**	camera's own, as stock rain did, so that running sideways through rain slants it
	**	(D-001 P23).  The camera's velocity is measured in Update.
	*/
	Vector3 fall_velocity(Wind.X,Wind.Y,-Parms.FallSpeed);
	Vector3 fall_direction = fall_velocity - CameraVelocity;
	if (fall_direction.Length2() < 0.0001f) {
		fall_direction = fall_velocity;
	}
	fall_direction.Normalize();

	float half_width = Parms.ParticleWidth * 0.5f;
	float half_height = Parms.ParticleHeight * 0.5f;
	float fade_band = Parms.EmitterHeight * _FadeBandFraction;
	float fade_start = ceiling - fade_band;

	int staged = 0;
	int room = _BudgetParticles;
	if (room > WEATHER_STAGING_MAX) room = WEATHER_STAGING_MAX;

	for (int iy = j0; iy <= j1 && staged < room; iy += step) {
		for (int ix = i0; ix <= i1 && staged < room; ix += step) {

			//	Is this one of the columns this density wants at all?
			if (keep < 1.0f) {
				if (Weather_Hash_Float((unsigned int)ix,(unsigned int)iy,0x1B3Fu) >= keep) continue;
			}

			bool ready = false;
			ColumnStruct * column = Find_Column(ix,iy,ceiling,bottom,ready);
			if (!ready || column == nullptr) continue;

			ColumnCount++;

			float landing_x = (float)ix * _BaseLatticeUnit
									+ Weather_Hash_Float((unsigned int)ix,(unsigned int)iy,0x2C91u) * spacing;
			float landing_y = (float)iy * _BaseLatticeUnit
									+ Weather_Hash_Float((unsigned int)ix,(unsigned int)iy,0x77A5u) * spacing;

			float floor_z = column->FloorZ;
			if (floor_z > ceiling) continue;		// standing under an overhang: nothing falls here

			float fall_length = ceiling - floor_z;
			float fall_time = fall_length / Parms.FallSpeed;

			/*
			**	How long a landed particle lingers, and therefore how long the whole cycle is.
			**	A column with nothing under it but the bottom of the box has no landing at all.
			*/
			float static_seconds = 0.0f;
			if (column->Landed && Parms.MaxStaticSeconds > 0.0f) {
				float t = Weather_Hash_Float((unsigned int)ix,(unsigned int)iy,0x3E07u);
				static_seconds = Parms.MinStaticSeconds + t * (Parms.MaxStaticSeconds - Parms.MinStaticSeconds);
			}

			float cycle = fall_time + static_seconds;
			if (cycle <= 0.0f) continue;

			//	How many particles this column carries, which is the stock spacing along a fall line.
			int count = (int)(Parms.ParticlesPerUnitLength * fall_length);
			if (count < 1) count = 1;

			float column_phase = Weather_Hash_Float((unsigned int)ix,(unsigned int)iy,0x64C1u);

			for (int n = 0; n < count && staged < room; n++) {

				float t = Weather_Wrap(Phase + (column_phase + (float)n / (float)count) * cycle,cycle);

				Vector3 position;
				float size = 1.0f;
				float alpha = 1.0f;
				unsigned char frame_index = Parms.FrameFirst;

				if (Parms.FrameCount > 1) {
					unsigned int f = Weather_Hash((unsigned int)ix,(unsigned int)iy,0x5A11u + (unsigned int)n);
					unsigned char span_frames = (unsigned char)(Parms.SplashFrame ? (Parms.FrameCount - 1) : Parms.FrameCount);
					if (span_frames < 1) span_frames = 1;
					frame_index = (unsigned char)(Parms.FrameFirst + (f % (unsigned int)span_frames));
				}

				if (t < fall_time) {

					/*
					**	Falling.  The particle is placed backwards from where it will land: at
					**	t seconds into the fall it is (fall_time - t) seconds of wind upwind of
					**	its landing point, which is what makes the landing exact no matter what
					**	the wind has been doing.
					*/
					float remaining = fall_time - t;

					position.X = landing_x - Wind.X * remaining;
					position.Y = landing_y - Wind.Y * remaining;
					position.Z = ceiling - Parms.FallSpeed * t;

					if (Parms.WanderAmplitude > 0.0f) {
						position.X += Parms.WanderAmplitude
											* WWMath::Fast_Sin(position.Z * Parms.WanderFrequency + (float)ix);
						position.Y += Parms.WanderAmplitude
											* WWMath::Fast_Sin(position.Z * Parms.WanderFrequency + (float)iy);
					}

					//	The stock fade-in, so that nothing appears out of nothing at the ceiling.
					if (position.Z > fade_start && fade_band > 0.0f) {
						alpha = 1.0f - (position.Z - fade_start) / fade_band;
						if (alpha < 0.0f) alpha = 0.0f;
					}

				} else {

					//	Landed, and lasting out its static period where it fell.
					position.X = landing_x;
					position.Y = landing_y;
					position.Z = floor_z;

					float elapsed = t - fall_time;
					float remaining = (static_seconds > 0.0f) ? (1.0f - elapsed / static_seconds) : 0.0f;
					if (remaining < 0.0f) remaining = 0.0f;

					if (Parms.DecayAfterLanding) {
						size = remaining;
					}

					if (Parms.SplashFrame) {
						frame_index = (unsigned char)(Parms.FrameFirst + Parms.FrameCount - 1);
					}

					if (size <= 0.0f) continue;
				}

				//	Exactly the stock per-particle rejection: a weather particle outside the
				//	frustum costs nothing but the arithmetic that got us here.
				if (CollisionMath::Overlap_Test(frustum,position) == CollisionMath::OUTSIDE) continue;

				_StagePosition[staged] = position;
				_StageDiffuse[staged].Set(Parms.Color.X,Parms.Color.Y,Parms.Color.Z,Parms.Color.W * alpha);
				_StageFrame[staged] = frame_index;

				if (Parms.Streak) {
					_StageSize[staged] = half_width * size;
					_StageTailPosition[staged] = position + fall_direction * (Parms.ParticleHeight * size);
				} else {
					_StageSize[staged] = half_height * 2.0f * size;
					_StageTailPosition[staged] = position;
				}

				staged++;
			}
		}
	}

	if (staged <= 0) return;

	_BudgetParticles -= staged;
	DrawnParticles = staged;

	/*
	**	Hand it over.  Everything past here belongs to WorldParticleManager: which draw call
	**	this shares, whether the pool has room for it, and what gets thinned if it has not.
	*/
	ShaderClass shader = Parms.Additive ? ShaderClass::_PresetAdditiveShader : ShaderClass::_PresetAlphaShader;
	shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	shader.Enable_Fog("WeatherEnvironmentRenderer");

	ParticleSubmissionStruct submission;
	submission.Texture = Peek_Texture();
	submission.Shader = shader;
	submission.FrameRowColumnLog2 = Parms.FrameRowColumnLog2;
	submission.Position = _StagePosition;
	submission.Diffuse = _StageDiffuse;
	submission.Size = _StageSize;
	submission.Count = staged;
	submission.Center = CameraPosition;
	submission.Budget = PARTICLE_BUDGET_ENVIRONMENT;

	if (Parms.Streak) {
		submission.LineMode = LineGroupClass::PRISM;
		submission.TailPosition = _StageTailPosition;
		submission.TailDiffuse = _StageDiffuse;
		WorldParticleManager::Submit(PARTICLE_POOL_STREAK,submission);
	} else {
		submission.PointMode = PointGroupClass::QUADS;
		submission.Frame = _StageFrame;
		WorldParticleManager::Submit(PARTICLE_POOL_SPRITE,submission);
	}
}


void WeatherEnvironmentRenderer::Begin_Frame(void)
{
	_BudgetParticles = _MaxParticles;
	_BudgetCasts = _MaxCastsPerFrame;
}


void WeatherEnvironmentRenderer::Set_Enabled(bool onoff)
{
	_Enabled = onoff;
}


bool WeatherEnvironmentRenderer::Is_Enabled(void)
{
	return _Enabled;
}


void WeatherEnvironmentRenderer::Set_Max_Particles(int count)
{
	if (count < 0) count = 0;
	if (count > WEATHER_STAGING_MAX) count = WEATHER_STAGING_MAX;
	_MaxParticles = count;
}


int WeatherEnvironmentRenderer::Get_Max_Particles(void)
{
	return _MaxParticles;
}


void WeatherEnvironmentRenderer::Set_Max_Casts_Per_Frame(int count)
{
	if (count < 0) count = 0;
	_MaxCastsPerFrame = count;
}


int WeatherEnvironmentRenderer::Get_Max_Casts_Per_Frame(void)
{
	return _MaxCastsPerFrame;
}


const char * WeatherEnvironmentRenderer::Get_Mode_Name(WeatherModeType mode)
{
	if (mode < 0 || mode >= WEATHER_MODE_COUNT) return "?";
	return _DefaultModes[mode].Name;
}


bool WeatherEnvironmentRenderer::Find_Mode(const char * name,WeatherModeType & mode)
{
	if (name == nullptr) return false;

	for (int i = 0; i < WEATHER_MODE_COUNT; i++) {
		if (::stricmp(name,_DefaultModes[i].Name) == 0) {
			mode = (WeatherModeType)i;
			return true;
		}
	}

	return false;
}


const WeatherModeStruct & WeatherEnvironmentRenderer::Get_Default_Parameters(WeatherModeType mode)
{
	WWASSERT(mode >= 0 && mode < WEATHER_MODE_COUNT);
	return _DefaultModes[mode];
}
