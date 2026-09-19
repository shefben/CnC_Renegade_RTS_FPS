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

/*
**	WeatherEnvironmentRenderer -- weather as an arithmetic function of the clock.
**
**	Roadmap Section 36, Zero Hour / SAGE feature 23.  Donor concept: the Zero Hour W3DSnow
**	renderer, whose reusable idea is not snow: it is that falling weather does not need to be
**	simulated at all.  A flake's height is a function of where its column is and what time it
**	is, so an engine that can evaluate that function needs no particle to remember it.
**
**	Renegade's own weather -- WeatherMgrClass, rain, snow and ash -- already established what
**	the image has to be, and this replaces the way it was produced rather than the image.  The
**	stock system was a linked list of pooled ParticleStructs, spawned and killed at a rate
**	derived from a density, each carrying a position, a velocity, a collision time and a
**	lifetime; it drew itself once per weather system into its own dynamic vertex buffer.  Three
**	facts about it are what make this rewrite exact rather than approximate:
**
**		- a weather particle has constant velocity.  There is no gravity term and live
**		  particles are never re-steered, so the path from the emitter plane to the point the
**		  ray hit is a straight line, and a straight line at constant speed is a clock;
**		- particles on one fall line are evenly spaced.  A freshly cast ray is filled with
**		  ParticlesPerUnitLength * length particles at once, which is the same statement as
**		  "the n'th particle on this column is n/N of the way through the cycle";
**		- the ray, not the particle, is what touches the world.  Collision was already one
**		  raycast per fall line, amortised across frames -- not one per particle.
**
**	So the state that has to exist is per column, and there is a bounded number of columns.
**	What used to be a particle is now an index.
**
**	Two things follow that the stock system could not offer.  The particle count is chosen
**	before any particle exists, by picking the lattice, instead of emerging from a spawn rate
**	and being clamped at USHRT_MAX/6 across all weather at once; and every mode's particles are
**	handed to WorldParticleManager, so weather that shares a texture with other weather -- all
**	of stock Renegade's does, it is one atlas -- shares its draw call too.
**
**	Nothing here allocates per particle, per frame, or per emitter.
*/

#ifndef WEATHERENVIRONMENT_H
#define WEATHERENVIRONMENT_H

#ifndef ALWAYS_H
	#include "always.h"
#endif

#include "shader.h"
#include "vector2.h"
#include "vector3.h"
#include "vector4.h"

class RenderInfoClass;
class TextureClass;


/*
**	The modes Section 36 asks for.
**
**	The first three are Renegade's own precipitation and their defaults reproduce it: the
**	numbers in _DefaultModes are the arguments the stock RainSystemClass, SnowSystemClass and
**	AshSystemClass passed to WeatherSystemClass, carried over one for one.  The last three are
**	the section's additions and have no stock behaviour to preserve.
*/
enum WeatherModeType
{
	WEATHER_MODE_RAIN = 0,				// RAIN_LIKE_PRECIPITATION
	WEATHER_MODE_SNOW,
	WEATHER_MODE_ASH,
	WEATHER_MODE_DUST,
	WEATHER_MODE_TIBERIUM,				// TIBERIUM_ATMOSPHERIC_PARTICLES
	WEATHER_MODE_FALLING,				// GENERIC_FALLING_PARTICLES

	WEATHER_MODE_COUNT
};


/*
**	What a mode is.
**
**	Every field is a number the stock system either took as a constructor argument or hard
**	coded, which is the point: a mode is data, so a fourth kind of weather is a table entry
**	rather than a fourth subclass of a render object.
*/
struct WeatherModeStruct
{
	const char *	Name;

	/*
	**	The art.  TextureName is the shared atlas for stock precipitation; FrameFirst and
	**	FrameCount select that mode's band of it, on the 4x4 grid the point renderer addresses
	**	frames with.  A mode whose texture is missing draws untextured rather than not at all.
	*/
	const char *	TextureName;
	unsigned char	FrameRowColumnLog2;
	unsigned char	FrameFirst;
	unsigned char	FrameCount;

	/*
	**	Streak means the mode is drawn by the streak pool, as a quad stretched along the
	**	direction the particle appears to be travelling.  It is how rain gets to be three times
	**	longer than it is wide: a sprite has one size, and a raindrop has two.
	*/
	bool			Streak;
	bool			Additive;

	float			FallSpeed;				// world units per second, down the fall line
	float			EmitterSize;			// half-width of the square the columns cover
	float			EmitterHeight;			// how far above the camera the fall starts
	float			ParticlesPerUnitLength;	// how densely one column is filled
	float			ParticleWidth;
	float			ParticleHeight;

	/*
	**	What happens where the fall line ends.  A particle that reaches the ground stays there
	**	for StaticSeconds, and then its column reuses it -- which is the stock static phase,
	**	where rain shows a splash frame and snow settles and shrinks.
	*/
	float			MinStaticSeconds;
	float			MaxStaticSeconds;
	bool			SplashFrame;			// the last frame of the band is the landed one
	bool			DecayAfterLanding;		// shrink to nothing over the static period

	/*
	**	Lateral wander, which is what makes a flake fall unlike a stone.  Zero for rain.
	*/
	float			WanderAmplitude;
	float			WanderFrequency;

	Vector4			Color;
};


/*
**	Where the ground is.
**
**	The renderer lives in ww3d2 and the ground is a physics question, so it asks rather than
**	knows.  An implementation is handed a vertical segment and answers where it first meets
**	the world; Combat supplies one over PhysicsSceneClass, and a dedicated server or a
**	device-less check supplies none at all, in which case weather falls the full height of its
**	box and nothing is occluded.
*/
class WeatherCollisionProviderClass
{
public:
	virtual ~WeatherCollisionProviderClass(void) {}

	/*
	**	True when the segment met real geometry, with the point and the surface normal; false
	**	when it reached the bottom without meeting anything, in which case only the point is
	**	set and a particle landing there is not drawn as a splash.
	*/
	virtual bool	Cast_Weather_Column(const Vector3 & top,const Vector3 & bottom,
										Vector3 & point,Vector3 & normal) = 0;
};


/*
**	One kind of weather, falling over one camera.
**
**	An instance is cheap and owns one fixed column cache; WeatherMgrClass makes one per
**	precipitation type.  The budget, by contrast, is static, because it is a property of the
**	frame and not of any one kind of weather -- three modes at once must not each be allowed
**	the whole of it.
*/
class WeatherEnvironmentRenderer
{
public:

	WeatherEnvironmentRenderer(WeatherModeType mode);
	~WeatherEnvironmentRenderer(void);

	/*
	**	The mode's tunables, live.  Returning the struct by reference is deliberate: these are
	**	the numbers a person changes at a console while looking at the weather, and every one
	**	of them is read afresh each frame.
	*/
	WeatherModeType			Get_Mode(void) const			{ return Mode; }
	WeatherModeStruct &		Parameters(void)				{ return Parms; }
	const WeatherModeStruct & Parameters(void) const		{ return Parms; }

	/*
	**	Particles per square world unit, which is the stock density and reaches this class
	**	from exactly where it reached the stock one: WeatherMgrClass's ramped parameters, set
	**	by script, by the level, or by the server.  Zero means this mode is not falling.
	*/
	void					Set_Density(float density);
	float					Get_Density(void) const			{ return Density; }

	void					Set_Wind(const Vector2 & velocity);
	const Vector2 &			Get_Wind(void) const			{ return Wind; }

	void					Set_Collision_Provider(WeatherCollisionProviderClass * provider);

	/*
	**	Advance the clock and place the box.  Cheap, and the only thing that casts rays.
	*/
	void					Update(float seconds,const Vector3 & camera_position);

	/*
	**	Evaluate the lattice and hand it to WorldParticleManager.  Draws nothing itself.
	*/
	void					Render(RenderInfoClass & rinfo);

	/*
	**	The box the weather occupies, for the scene to cull the whole thing at once.
	*/
	void					Get_Bounds(Vector3 & minimum,Vector3 & maximum) const;

	/*
	**	Forget every cached column.  Called when the level changes, because a cached floor
	**	height is a fact about a world that no longer exists.
	*/
	void					Reset(void);

	//	Per-instance statistics, all measured over the last Render.
	int						Get_Drawn_Particles(void) const		{ return DrawnParticles; }
	int						Get_Column_Count(void) const		{ return ColumnCount; }
	float					Get_Column_Spacing(void) const		{ return ColumnSpacing; }
	int						Get_Deferred_Columns(void) const	{ return DeferredColumns; }
	int						Get_Casts(void) const				{ return Casts; }
	int						Get_Cache_Hits(void) const			{ return CacheHits; }

	/*
	**	Off means no weather is evaluated or submitted at all.  It is the A/B switch, and the
	**	thing it is checked against is the stock image.
	*/
	static void				Set_Enabled(bool onoff);
	static bool				Is_Enabled(void);

	/*
	**	How many weather particles may exist in a frame, across every mode at once.  This is
	**	the bound the acceptance is about, and it is spent in mode order, so a mode that starts
	**	falling into a frame that is already full gets what is left rather than nothing: the
	**	lattice coarsens instead of the particles disappearing.
	*/
	static void				Set_Max_Particles(int count);
	static int				Get_Max_Particles(void);

	/*
	**	Hand the frame its allowances back.  Render does this by itself when the frame counter
	**	moves, which is every case the game has; it is public because a check that renders
	**	several times without a frame ever advancing needs to say so explicitly, and because a
	**	second render of the same frame -- a reflection, a second viewport -- genuinely should
	**	not be given the budget twice.
	*/
	static void				Begin_Frame(void);

	/*
	**	How many columns may be cast per frame.  Casting is the only part of weather that
	**	touches the physics scene, so this is the number that decides what weather costs the
	**	world, and it is a number rather than a fraction of anything.
	*/
	static void				Set_Max_Casts_Per_Frame(int count);
	static int				Get_Max_Casts_Per_Frame(void);

	static const char *		Get_Mode_Name(WeatherModeType mode);
	static bool				Find_Mode(const char * name,WeatherModeType & mode);

	/*
	**	The mode table's shipped values, for a console command that wants to put one back.
	*/
	static const WeatherModeStruct & Get_Default_Parameters(WeatherModeType mode);

private:

	/*
	**	One column: where a particle that falls down it lands, and whether that was the world
	**	or merely the bottom of the box.  Key is the lattice index pair, packed; Stamp is the
	**	frame the entry was last wanted, so the cache evicts what nobody is standing near.
	*/
	struct ColumnStruct
	{
		unsigned int	Key;
		float			FloorZ;
		Vector3			Normal;
		bool			Landed;
		bool			Valid;
		unsigned int	Stamp;
	};

	enum { COLUMN_CACHE_SIZE = 4096 };

	ColumnStruct *			Find_Column(int ix,int iy,float ceiling,float bottom,bool & ready);

	/*
	**	The mode's texture, loaded the first time it is drawn and dropped again if the asset
	**	manager reports that it does not exist.  Dropping it matters: the manager answers a
	**	missing file with a placeholder, and a placeholder drawn ten thousand times is worse
	**	than no texture at all, whereas untextured weather is a plain coloured particle.
	*/
	TextureClass *			Peek_Texture(void);

	WeatherModeType			Mode;
	WeatherModeStruct		Parms;

	float					Density;
	Vector2					Wind;
	Vector3					CameraPosition;
	Vector3					BoundsMin;
	Vector3					BoundsMax;
	float					Phase;					// seconds since it started falling

	WeatherCollisionProviderClass *	Collision;

	ColumnStruct *			Columns;
	TextureClass *			Texture;
	bool					TextureChecked;

	//	Statistics, all for the last Render.
	int						DrawnParticles;
	int						ColumnCount;
	float					ColumnSpacing;
	int						DeferredColumns;
	int						Casts;
	int						CacheHits;
};

#endif	// WEATHERENVIRONMENT_H
