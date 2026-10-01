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
 *                     $Archive:: /Commando/Code/Combat/weatherenvironment.cpp                $*
 *                                                                                             *
 *	Roadmap Section 36, Zero Hour / SAGE feature 23.  See the header for what this is and what   *
 *	Renegade's own precipitation keeps.                                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WeatherEnvironmentRenderer::Init -- one particle kind per mode, and nothing else           *
 *   WeatherEnvironmentRenderer::Compute_Targets -- how many motes the air should hold          *
 *   WeatherEnvironmentRenderer::Emit_Into -- fill a volume without a random number generator   *
 *   WeatherEnvironmentRenderer::Update -- close the gap between target and live                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "weatherenvironment.h"

#include "particlebatchtype.h"
#include "worldparticlebatchmanager.h"
#include "wwdebug.h"
#include "wwmath.h"


int			WeatherEnvironmentRenderer::Definition[WEATHER_ENVIRONMENT_MODE_COUNT]	= { -1,-1,-1 };
float			WeatherEnvironmentRenderer::Density[WEATHER_ENVIRONMENT_MODE_COUNT]		= { 0.0f,0.0f,0.0f };
int			WeatherEnvironmentRenderer::Target[WEATHER_ENVIRONMENT_MODE_COUNT]		= { 0,0,0 };
unsigned		WeatherEnvironmentRenderer::Phase[WEATHER_ENVIRONMENT_MODE_COUNT]			= { 0,0,0 };

Vector3		WeatherEnvironmentRenderer::Wind(0.0f,0.0f,0.0f);
AABoxClass	WeatherEnvironmentRenderer::Region(Vector3(0.0f,0.0f,0.0f),Vector3(0.0f,0.0f,0.0f));
bool			WeatherEnvironmentRenderer::RegionValid	= false;
AABoxClass	WeatherEnvironmentRenderer::Volume(Vector3(0.0f,0.0f,0.0f),Vector3(0.0f,0.0f,0.0f));

int			WeatherEnvironmentRenderer::Budget			= 0;
int			WeatherEnvironmentRenderer::Emitted			= 0;
int			WeatherEnvironmentRenderer::Refused			= 0;
bool			WeatherEnvironmentRenderer::Initialized	= false;

static_assert(WEATHER_ENVIRONMENT_MODE_COUNT == 3,
				  "the atmospheric mode list changed -- update the static initialisers above");


/*
**	The share of the pool's ENVIRONMENT budget class the weather may use.  Less than the class
**	holds, deliberately: the class is also where a Commander's smoke and a burning building's
**	haze will be spawned, and weather that could fill it would be weather that turns those off.
*/
enum { WEATHER_ENVIRONMENT_DEFAULT_BUDGET = 900 };

/*
**	How many motes one update may add per mode.  The steady state does not depend on this -- the
**	population converges on its target either way -- but a density that goes from nothing to
**	full should arrive over a second or so rather than in one frame, which would be both a
**	visible pop and a spike in the fill.
*/
enum { WEATHER_ENVIRONMENT_MAX_SPAWN_PER_UPDATE = 48 };

/*
**	The volume the camera carries with it, as half extents in metres.  Wide and flat: this is
**	the air the player can see motes in, and anything further than this is too small to read.
*/
static const Vector3 WEATHER_ENVIRONMENT_EXTENT(28.0f,28.0f,9.0f);


/*
**	One mode, as the numbers that make it.
**
**	Population is what full density means in particles, which is the whole of "bounded": a mode
**	is a number of motes rather than a rate, so the air holds what it was told to hold however
**	long the weather runs and however fast the frames arrive.
*/
struct WeatherEnvironmentModeStruct
{
	const char *	Name;				// what the console and the checks call it
	const char *	Kind;				// the name of the particle kind interned in the pool
	int				Population;		// particles at density one
	int				Shape;			// one of ParticleBatchKindType
	float				Lifetime;		// seconds
	float				StartSize;		// metres
	float				EndSize;
	float				StartAlpha;
	float				EndAlpha;
	float				Rise;				// metres per second, up; negative falls
	float				Drag;
	float				WindFactor;		// how much of the wind this mode takes
	float				Jitter;			// metres per second of per-mote variation, horizontally
	bool				Additive;
	Vector3			StartColor;
	Vector3			EndColor;
};

static const WeatherEnvironmentModeStruct _Modes[WEATHER_ENVIRONMENT_MODE_COUNT] =
{
	//	Dust: hanging, almost still, going nowhere slowly.  The mode a hot dry map wears.
	{	"dust",		"ow_weather_dust",		500,	PARTICLE_BATCH_SPRITE,
		6.0f,	0.35f,	0.55f,	0.35f,	0.0f,	-0.15f,	0.25f,	1.00f,	0.30f,	false,
		Vector3(0.78f,0.72f,0.60f),	Vector3(0.78f,0.72f,0.60f)	},

	//	Tiberium haze: motes that rise and glow.  Additive, because the thing it is made of is
	//	light rather than matter, and slow to take the wind because it is heavy.
	{	"tiberium",	"ow_weather_tiberium",	300,	PARTICLE_BATCH_POINT,
		4.0f,	0.10f,	0.04f,	0.75f,	0.0f,	0.35f,	0.15f,	0.25f,	0.15f,	true,
		Vector3(0.45f,1.00f,0.40f),	Vector3(0.15f,0.55f,0.15f)	},

	//	Generic falling particles: whatever a level needs to fall slowly near the player.  Not
	//	precipitation -- it does not land, because nothing here casts a ray to find out where
	//	the ground is -- so it is given a short life and a slow fall and lives in the air.
	{	"falling",	"ow_weather_falling",	700,	PARTICLE_BATCH_SPRITE,
		3.5f,	0.12f,	0.10f,	0.80f,	0.0f,	-1.60f,	0.10f,	0.60f,	0.20f,	false,
		Vector3(0.85f,0.85f,0.88f),	Vector3(0.85f,0.85f,0.88f)	},
};


/*
**	A low-discrepancy sequence, three dimensions of it, as additive recurrences of the
**	generalised golden ratios.  This is what fills the volume: it covers it more evenly than a
**	random number generator of the same count does, and it is the same air every time, so a
**	recorded game and a check both get back what they saw.
*/
static const float WEATHER_R3_X = 0.8191725133961645f;
static const float WEATHER_R3_Y = 0.6710436067037893f;
static const float WEATHER_R3_Z = 0.5497004779019702f;

static inline float Weather_Fraction(unsigned index,float alpha)
{
	float value = (float)index * alpha;
	value -= (float)((int)value);
	if (value < 0.0f) { value += 1.0f; }
	return value;
}


/***********************************************************************************************
 *	WeatherEnvironmentRenderer::Init -- one particle kind per mode, and nothing else            *
 *                                                                                             *
 *	The kinds are defined in the batched pool, which is where every allocation in this feature   *
 *	happens.  None of them names a texture, so none of them draws anything today and none of     *
 *	them costs a buffer until something emits -- see docs/assets/WeatherSprites.md.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WeatherEnvironmentRenderer::Init(void)
{
	if (Initialized) {
		return ;
	}

	Budget	= WEATHER_ENVIRONMENT_DEFAULT_BUDGET;
	Emitted	= 0;
	Refused	= 0;

	Wind.Set(0.0f,0.0f,0.0f);
	RegionValid = false;
	Volume.Center.Set(0.0f,0.0f,0.0f);
	Volume.Extent = WEATHER_ENVIRONMENT_EXTENT;

	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {

		const WeatherEnvironmentModeStruct & mode = _Modes[m];

		Density[m]	= 0.0f;
		Target[m]	= 0;
		Phase[m]		= 0;

		ParticleBatchDefinitionClass def;
		def.Set_Name(mode.Kind);
		def.Set_Kind(mode.Shape);

		//	Every mode is in the one budget class the pool keeps for the air of a place.  That is
		//	what makes weather the first thing a busy frame stops drawing, which is correct: a
		//	player who cannot see the dust has lost nothing they needed.
		def.Set_Budget_Class(PARTICLE_BUDGET_ENVIRONMENT);

		def.Set_Lifetime(mode.Lifetime);
		def.Set_Start_Size(mode.StartSize);
		def.Set_End_Size(mode.EndSize);
		def.Set_Start_Alpha(mode.StartAlpha);
		def.Set_End_Alpha(mode.EndAlpha);
		def.Set_Start_Color(mode.StartColor);
		def.Set_End_Color(mode.EndColor);

		//	Rise is stated the way a reader thinks about it -- up is positive -- and the pool
		//	states gravity the way physics does, so the sign turns over here.
		def.Set_Gravity(-mode.Rise);
		def.Set_Drag(mode.Drag);
		def.Set_Additive(mode.Additive);
		def.Set_Spin_Rate((mode.Shape == PARTICLE_BATCH_SPRITE) ? 0.25f : 0.0f);

		//	A mode may hold its whole population in one group, and never more, so the group's
		//	buffers are sized once at exactly what this layer can ask of them.
		int ceiling = mode.Population * 2;
		if (ceiling > PARTICLE_BATCH_MAX_PER_GROUP) { ceiling = PARTICLE_BATCH_MAX_PER_GROUP; }
		def.Set_Max_Particles(ceiling);

		Definition[m] = WorldParticleBatchManager::Define_Definition(def);
	}

	Initialized = true;
	return ;
}


void WeatherEnvironmentRenderer::Shutdown(void)
{
	Reset();

	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {
		Definition[m] = -1;
	}

	Budget		= 0;
	Emitted		= 0;
	Refused		= 0;
	RegionValid	= false;
	Initialized	= false;
	return ;
}


/***********************************************************************************************
 *	WeatherEnvironmentRenderer::Reset -- a different place has different air                     *
 *                                                                                             *
 *	The particle kinds stay, because they are tuning rather than content; the motes go, because  *
 *	they were hanging in a level that is not loaded any more.  The pool would drop them at the   *
 *	scene change by itself; doing it here as well means a level that reloads into the same scene *
 *	does not inherit the last one's weather for a few seconds.                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WeatherEnvironmentRenderer::Reset(void)
{
	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {
		Density[m]	= 0.0f;
		Target[m]	= 0;
		if (Definition[m] >= 0) {
			WorldParticleBatchManager::Clear_Group(Definition[m]);
		}
	}

	Wind.Set(0.0f,0.0f,0.0f);
	RegionValid = false;
	return ;
}


void WeatherEnvironmentRenderer::Set_Density(int mode,float density)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT)) {
		return ;
	}

	//	Clamped rather than asserted: the number arrives from a script, a level's weather
	//	settings or the console, and content is allowed to be wrong without taking the game
	//	down.  Four times full density is already more motes than the budget will allow.
	if (density < 0.0f) { density = 0.0f; }
	if (density > 4.0f) { density = 4.0f; }

	Density[mode] = density;
	return ;
}


float WeatherEnvironmentRenderer::Get_Density(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT)) {
		return 0.0f;
	}
	return Density[mode];
}


void WeatherEnvironmentRenderer::Set_Wind(const Vector3 & velocity)
{
	Wind = velocity;
	return ;
}


const Vector3 & WeatherEnvironmentRenderer::Get_Wind(void)
{
	return Wind;
}


void WeatherEnvironmentRenderer::Set_Region(const AABoxClass & region)
{
	Region		= region;
	RegionValid	= true;
	return ;
}


void WeatherEnvironmentRenderer::Clear_Region(void)
{
	RegionValid = false;
	return ;
}


bool WeatherEnvironmentRenderer::Has_Region(void)
{
	return RegionValid;
}


const AABoxClass & WeatherEnvironmentRenderer::Get_Volume(void)
{
	return Volume;
}


void WeatherEnvironmentRenderer::Set_Budget(int particles)
{
	if (particles < 0) { particles = 0; }

	//	The pool's own ceiling for the class is the hard one; asking for more than it would ever
	//	hand out is not an error, it is just a number that cannot be reached, so it is clamped
	//	to the truth instead of being believed.
	const int class_cap = WorldParticleBatchManager::Get_Budget_Cap(PARTICLE_BUDGET_ENVIRONMENT);
	if ((class_cap > 0) && (particles > class_cap)) {
		particles = class_cap;
	}

	Budget = particles;
	return ;
}


int WeatherEnvironmentRenderer::Get_Budget(void)
{
	return Budget;
}


int WeatherEnvironmentRenderer::Mode_Full_Population(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT)) {
		return 0;
	}
	return _Modes[mode].Population;
}


/***********************************************************************************************
 *	WeatherEnvironmentRenderer::Compute_Targets -- how many motes the air should hold            *
 *                                                                                             *
 *	Density times the mode's full population, and then, if the modes between them want more than *
 *	the layer's budget, every one of them is scaled by the same fraction.  Scaling rather than   *
 *	first-come-first-served, because a budget that the first mode to be asked for empties is a   *
 *	budget that makes dust turn tiberium haze off.                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WeatherEnvironmentRenderer::Compute_Targets(int * targets)
{
	int wanted[WEATHER_ENVIRONMENT_MODE_COUNT];
	int total = 0;

	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {
		wanted[m] = (int)((Density[m] * (float)_Modes[m].Population) + 0.5f);
		if (wanted[m] < 0) { wanted[m] = 0; }
		total += wanted[m];
	}

	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {

		int target = wanted[m];

		if ((total > Budget) && (total > 0)) {
			//	Integer arithmetic, so that the same densities give the same answer on every
			//	machine rather than one that depends on how the compiler rounded.
			target = (int)(((long long)wanted[m] * (long long)Budget) / (long long)total);
		}

		//	No mode may ask for more than its group can hold, whatever the budget says.
		if (Definition[m] >= 0) {
			const int ceiling = WorldParticleBatchManager::Peek_Definition(Definition[m]).Get_Max_Particles();
			if (target > ceiling) { target = ceiling; }
		}

		targets[m] = target;
	}
	return ;
}


/***********************************************************************************************
 *	WeatherEnvironmentRenderer::Emit_Into -- fill a volume without a random number generator     *
 *                                                                                             *
 *	Each mote's place in the volume is the next point of a three dimensional low-discrepancy     *
 *	sequence, and each mote's drift is its own place scattered a little.  Nothing here consumes  *
 *	randomness, so two machines watching the same weather see the same air, which a random       *
 *	number generator in a client-side effect would quietly take away.                           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WeatherEnvironmentRenderer::Emit_Into(int mode,int count,const AABoxClass & volume)
{
	if ((count <= 0) || (Definition[mode] < 0)) {
		return 0;
	}

	const WeatherEnvironmentModeStruct & settings = _Modes[mode];
	const Vector3 wind = Wind * settings.WindFactor;

	int made = 0;
	for (int i = 0; i < count; i++) {

		const unsigned index = ++Phase[mode];

		const float fx = Weather_Fraction(index,WEATHER_R3_X);
		const float fy = Weather_Fraction(index,WEATHER_R3_Y);
		const float fz = Weather_Fraction(index,WEATHER_R3_Z);

		Vector3 position(volume.Center.X + (((fx * 2.0f) - 1.0f) * volume.Extent.X),
								volume.Center.Y + (((fy * 2.0f) - 1.0f) * volume.Extent.Y),
								volume.Center.Z + (((fz * 2.0f) - 1.0f) * volume.Extent.Z));

		Vector3 velocity(wind.X + (((fy * 2.0f) - 1.0f) * settings.Jitter),
								wind.Y + (((fz * 2.0f) - 1.0f) * settings.Jitter),
								wind.Z + settings.Rise);

		if (WorldParticleBatchManager::Emit(Definition[mode],position,velocity)) {
			made++;
			Emitted++;
		} else {
			//	The pool is full, or the ENVIRONMENT class is at its own cap.  Counted and
			//	dropped: the population simply stays where the pool can afford it, which is the
			//	budget doing its job rather than a failure.
			Refused++;
		}
	}
	return made;
}


/***********************************************************************************************
 *	WeatherEnvironmentRenderer::Update -- close the gap between target and live                  *
 *                                                                                             *
 *	The pool knows exactly how many of each kind are alive, so there is no bookkeeping here to   *
 *	drift out of step with it: each step asks for the difference, bounded, and a mote that died  *
 *	of old age is replaced by the next step.  Nothing is ever removed on purpose -- a density    *
 *	that falls is a population that thins out over the mode's lifetime, which is how weather     *
 *	stops looking like a switch.                                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WeatherEnvironmentRenderer::Update(const Vector3 & camera_position)
{
	if (!Initialized) {
		return ;
	}

	//	Where the air is.  A region pins it; otherwise it travels with the camera, which is what
	//	makes a bounded number of motes cover an unbounded outdoor scene.
	if (RegionValid) {
		Volume = Region;
	} else {
		Volume.Center = camera_position;
		Volume.Extent = WEATHER_ENVIRONMENT_EXTENT;
	}

	Compute_Targets(Target);

	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {

		if (Definition[m] < 0) {
			continue;
		}

		const int live = WorldParticleBatchManager::Get_Group_Particle_Count(Definition[m]);
		int wanted = Target[m] - live;

		if (wanted <= 0) {
			continue;
		}
		if (wanted > WEATHER_ENVIRONMENT_MAX_SPAWN_PER_UPDATE) {
			wanted = WEATHER_ENVIRONMENT_MAX_SPAWN_PER_UPDATE;
		}

		Emit_Into(m,wanted,Volume);
	}
	return ;
}


int WeatherEnvironmentRenderer::Get_Live_Count(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT) || (Definition[mode] < 0)) {
		return 0;
	}
	return WorldParticleBatchManager::Get_Group_Particle_Count(Definition[mode]);
}


int WeatherEnvironmentRenderer::Get_Live_Count(void)
{
	int live = 0;
	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {
		live += Get_Live_Count(m);
	}
	return live;
}


int WeatherEnvironmentRenderer::Get_Target_Count(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT)) {
		return 0;
	}
	return Target[mode];
}


int WeatherEnvironmentRenderer::Get_Target_Count(void)
{
	int target = 0;
	for (int m = 0; m < WEATHER_ENVIRONMENT_MODE_COUNT; m++) {
		target += Target[m];
	}
	return target;
}


int WeatherEnvironmentRenderer::Get_Emitted_Count(void)
{
	return Emitted;
}


int WeatherEnvironmentRenderer::Get_Refused_Count(void)
{
	return Refused;
}


const char * WeatherEnvironmentRenderer::Get_Mode_Name(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT)) {
		return "unknown";
	}
	return _Modes[mode].Name;
}


int WeatherEnvironmentRenderer::Get_Definition_Index(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT)) {
		return -1;
	}
	return Definition[mode];
}


bool WeatherEnvironmentRenderer::Names_A_Texture(int mode)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT) || (Definition[mode] < 0)) {
		return false;
	}
	return WorldParticleBatchManager::Peek_Definition(Definition[mode]).Names_A_Texture();
}


bool WeatherEnvironmentRenderer::Set_Texture(int mode,const char * texture_name)
{
	if ((mode < 0) || (mode >= WEATHER_ENVIRONMENT_MODE_COUNT) || (Definition[mode] < 0)) {
		return false;
	}
	return WorldParticleBatchManager::Set_Definition_Texture(Definition[mode],texture_name);
}
