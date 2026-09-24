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
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/WeatherMgr.h          $*
 *                                                                                             *
 *                       Author:: Ian Leslie																	  *
 *                                                                                             *
 *                     $Modtime:: 1/15/02 7:26p       $*
 *                                                                                             *
 *                    $Revision:: 16                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WEATHERMGR_H
#define WEATHERMGR_H


// Code controlling defines.
// Includes.
#include "combatchunkid.h"
#include "weatherenvironment.h"
#include "mempool.h"
#include "networkobject.h"
#include "random.h"
#include	"rendobj.h"
#include "saveloadsubsystem.h"
#include "SoundEnvironment.h"
#include "texture.h"
#include "vector.h"
#include "vector2.h"
#include "vector3.h"
#include "wwdebug.h"


// Class declarations.
class ChunkLoadClass;
class ChunkSaveClass;
class PhysicsSceneClass;

class AudibleSoundClass;


class WindClass
{
	public:

		 WindClass (float heading, float speed, float variability, SoundEnvironmentClass *soundenvironment);
		~WindClass();

		Vector2 Get_Velocity()	{return (Velocity);}
		void	  Set (float heading, float speed, float variability);
		bool	  Update();

	protected:

		enum {
			OCTAVE_COUNT = 2
		};

		SoundEnvironmentClass *SoundEnvironment;
		float						  Heading;
		float						  Speed;
		float						  Variability;
		double					  Theta [OCTAVE_COUNT];
		Vector2					  Velocity;
		AudibleSoundClass		 *Sound;
};


/*
**	Where the ground is, answered for the weather renderer.
**
**	WeatherEnvironmentRenderer lives in ww3d2 and so cannot ask a physics scene anything; this
**	is the half of that question that knows how.  The cast is the stock one, unchanged --
**	terrain only, as a projectile -- because what counts as ground for a raindrop is a decision
**	the original made and there is no reason to make it differently now.
*/
class WeatherSceneCollisionClass : public WeatherCollisionProviderClass
{
	public:

		WeatherSceneCollisionClass (PhysicsSceneClass *scene) : Scene (scene) {}

		bool Cast_Weather_Column (const Vector3 &top, const Vector3 &bottom,
										  Vector3 &point, Vector3 &normal) override;

	private:

		PhysicsSceneClass *Scene;
};


/*
**	One kind of weather, in the scene.
**
**	This used to be the whole weather implementation: a list of rays cast into the world, a
**	linked list of pooled particles spawned along them, an index buffer, and a draw call of its
**	own.  All of that is now arithmetic in WeatherEnvironmentRenderer, and what remains here is
**	the part that has to be a RenderObjClass -- something the scene can hold, cull, and render
**	in its proper place in the frame.
**
**	It stays a render object rather than becoming a call from the game loop for one reason
**	worth stating: the stock system put itself on static sort list 31 and drew on the way back
**	through, and that is where weather belongs in the draw order.  Keeping the object keeps the
**	order.
*/
class WeatherSystemClass : public RenderObjClass
{
	public:

		 WeatherSystemClass (PhysicsSceneClass *scene, WeatherModeType mode, float density);
		~WeatherSystemClass();

		RenderObjClass *Clone() const override
		{
			WWASSERT (false);
			return (0);
		}

		void Set_Density (float density);
		void Render (RenderInfoClass &rinfo) override;
		void Get_Obj_Space_Bounding_Sphere (SphereClass &sphere) const override;
		void Get_Obj_Space_Bounding_Box (AABoxClass &box) const override;

		virtual bool Update (WindClass *wind, const Vector3 &cameraposition);

		WeatherEnvironmentRenderer &	Peek_Renderer()					{return (Renderer);}
		int									Get_Drawn_Particles() const	{return (Renderer.Get_Drawn_Particles());}

	protected:

		WeatherSceneCollisionClass	Collision;
		WeatherEnvironmentRenderer	Renderer;
		float								Density;
};


/*
**	Rain, which is a weather system plus the sound of one.
**
**	Snow and ash needed a subclass each in the stock engine only to name their texture page and
**	their fall speed; those are table entries now, so they have none.
*/
class RainSystemClass : public WeatherSystemClass
{
	public:

					RainSystemClass (PhysicsSceneClass *scene, float particledensity, SoundEnvironmentClass *soundenvironment);
		virtual ~RainSystemClass();

		bool Update (WindClass *wind, const Vector3 &cameraposition) override;

	protected:

		AudibleSoundClass		 *Sound;					// Sound effect for rainfall.
		SoundEnvironmentClass *SoundEnvironment;
};


class WeatherParameterClass
{
	public:

		void Initialize();
		void Set (float targetvalue, float ramptime, bool override);
		void Set (float overrideramptime) {OverrideDuration = overrideramptime;}

		float Value() {return (CurrentValue);}

		bool Update (float time, bool override);
		void Update (float &value, float &target, float &duration, float time);

	private:

		float CurrentValue;
		float NormalValue;
		float NormalTarget;
		float	NormalDuration;
		float OverrideTarget;
		float OverrideDuration;

		friend class WeatherMgrClass;
};



class	WeatherMgrClass : public SaveLoadSubSystemClass, public NetworkObjectClass
{
	public:

		enum PrecipitationEnum {
			PRECIPITATION_FIRST,
			PRECIPITATION_RAIN = PRECIPITATION_FIRST,
			PRECIPITATION_SNOW,
			PRECIPITATION_ASH,
			PRECIPITATION_COUNT
		};

		 WeatherMgrClass();
		~WeatherMgrClass() {}

		uint32		Chunk_ID() const override	{return (CHUNKID_WEATHER_MGR);}
		const char *Name() const override		{return ("WeatherMgrClass");}
		void Delete (void) override				{}
		virtual void Set_Delete_Pending (void) override {};

		bool Save (ChunkSaveClass &csave) override;
		bool Load (ChunkLoadClass &cload) override;
		bool Load_Micro_Chunks (ChunkLoadClass &cload);
		void Export_Rare (BitStreamClass &packet) override;
		void Import_Rare (BitStreamClass &packet) override;

		/*
		**	D-001 P23: the three environment modes WeatherMgrClass had no caller for.  Tiberium haze
		**	and dust come from the generated world (its fields and its profile); falling particles
		**	are scripted (console `weather_falling`, or Set_Falling_Density from code).  All three are
		**	derived per client from state every client already has, so none of them is saved or
		**	sent: the generated world regenerates them, and a script re-asks for its own.
		*/
		static void		Set_Falling_Density (float density);
		static float	Get_Environment_Density (int mode);
		static float	Compute_Tiberium_Haze (const Vector3 &position, float base_density);

		static void Init (SoundEnvironmentClass *soundenvironment);
		static void	Reset();
		static void Shutdown();

		static bool Save_Dynamic (ChunkSaveClass &csave);
		static bool Load_Dynamic (ChunkLoadClass &cload);
		static bool Load_Dynamic_Micro_Chunks (ChunkLoadClass &cload);

		static bool Set_Wind (float heading, float speed, float variability, float ramptime = 0.0f);
		static bool Override_Wind (float heading, float speed, float variability, float ramptime = 0.0f);
		static void Get_Wind (float &heading, float &speed, float &variability);

		/*
		**	The live gusting vector, and the scales that decide how hard it pushes each of
		**	the three things Phase 58 drives.  All three default to zero, so the wind field
		**	exists and moves nothing until somebody turns it up -- Section 52A's "zero wind
		**	is the current static appearance" made the default rather than a special case.
		*/
		static Vector2	Get_Wind_Velocity (void);
		static void		Update_Wind_Consumers (void);

		static void		Set_Foliage_Sway_Scale (float scale);
		static float	Get_Foliage_Sway_Scale (void);
		static void		Set_Particle_Wind_Scale (float scale);
		static float	Get_Particle_Wind_Scale (void);
		static void		Set_Cloud_Wind_Scale (float scale);
		static float	Get_Cloud_Wind_Scale (void);

		/*
		**	Wetness.  Roadmap Section 52A, Phase 59.
		**
		**	One scalar, 0 dry to 1 soaked, rising while precipitation falls and drying over
		**	about a minute after it stops.  It is not replicated as its own value and does not
		**	need to be: the rain density it integrates *is* replicated (Export_Rare), and both
		**	ends run the same integrator over the same input, so they agree without a byte.
		**
		**	Section 36 owns the falling particles; this is the world's response to them, and
		**	the two are deliberately separate -- turning the rain off does not instantly dry
		**	the ground, which is the entire point.
		**
		**	Shelter: a surface that cannot see the sky does not get wet.  Callers that know
		**	their own sky view (the terrain solve does; BackgroundMgrClass::Compute_Sky_View
		**	answers for the camera) ask for the sheltered value instead of the open one.
		*/
		static float	Get_Wetness (void);
		static float	Get_Sheltered_Wetness (float sky_view);
		static void		Set_Wetness (float wetness);

		static void		Set_Wetness_Response_Scale (float scale);
		static float	Get_Wetness_Response_Scale (void);

		//	The pure integrator, exposed because it is the part a device-less check can prove.
		static float	Integrate_Wetness (float current, float precipitation_density, float dt);
		static void Restore_Wind (float ramptime);

		static bool Set_Precipitation (PrecipitationEnum precipitation, float density, float ramptime = 0.0f);
		static bool Override_Precipitation (PrecipitationEnum precipitation, float density, float ramptime = 0.0f);
		static void Get_Precipitation (PrecipitationEnum precipitation, float &density);
		static void Restore_Precipitation (float ramptime);

		static void Set_Fog_Enable (bool enabled)
		{
			_FogEnabled = enabled;
			Set_Dirty();
		}
		static bool Get_Fog_Enable()
		{
			return (_FogEnabled);
		}
		static bool Set_Fog_Range (float startdistance, float enddistance, float ramptime = 0.0f);
		static void Get_Fog_Range (float &startdistance, float &enddistance);

		/*
		**	The live weather of one kind, or null when that kind is not falling.  For the
		**	console, which reports what the weather layer cost this frame.
		*/
		static WeatherSystemClass *Peek_Precipitation (PrecipitationEnum precipitation)
		{
			return (_Precipitation [precipitation]);
		}

		static void Update (PhysicsSceneClass *scene, CameraClass *camera);
		static void Render (const CameraClass *camera);

	private:

		#define VARID_PARAMETER(varname) \
			VARID_ ## varname ## _CURRENT_VALUE, \
			VARID_ ## varname ## _NORMAL_VALUE,	\
			VARID_ ## varname ## _NORMAL_TARGET, \
			VARID_ ## varname ## _NORMAL_DURATION, \
			VARID_ ## varname ## _OVERRIDE_TARGET,	\
			VARID_ ## varname ## _OVERRIDE_DURATION

		// Constants.
		enum {
			CHUNKID_MICRO_CHUNKS			  = 0x03020113,
			CHUNKID_DYNAMIC_MICRO_CHUNKS = 0x11020245
		};

		enum {
			VARID_DUMMY = 0x09,

			VARID_PARAMETER (WIND_HEADING),
			VARID_PARAMETER (WIND_SPEED),
			VARID_PARAMETER (WIND_VARIABILITY),
			VARID_PARAMETER (RAIN_DENSITY),
			VARID_PARAMETER (SNOW_DENSITY),
			VARID_PARAMETER (ASH_DENSITY),

			VARID_WIND_OVERRIDE_COUNT,
			VARID_PRECIPITATION_OVERRIDE_COUNT,

			VARID_FOG_ENABLED,
			VARID_PARAMETER (FOG_START_DISTANCE),
			VARID_PARAMETER (FOG_END_DISTANCE)
		};

		#undef VARID_PARAMETER

		enum {
			PARAMETER_WIND_HEADING,
			PARAMETER_WIND_SPEED,
			PARAMETER_WIND_VARIABILITY,
			PARAMETER_RAIN_DENSITY,
			PARAMETER_SNOW_DENSITY,
			PARAMETER_ASH_DENSITY,
			PARAMETER_FOG_START_DISTANCE,
			PARAMETER_FOG_END_DISTANCE,
			PARAMETER_COUNT
		};

		static bool Set_Wind (float heading, float speed, float variability, float ramptime, bool override);
		static bool Set_Precipitation (PrecipitationEnum precipitation, float density, float ramptime, bool override);

		static bool Is_Dirty()							{return (_Dirty);}
		static void Set_Dirty (bool dirty = true)	{_Dirty = dirty;}

		static SoundEnvironmentClass *_SoundEnvironment;
		static WeatherParameterClass	_Parameters [PARAMETER_COUNT];
		static bool							_Imported;
		static unsigned					_WindOverrideCount;
		static unsigned					_PrecipitationOverrideCount;

		static WindClass				  *_Wind;
		static WeatherSystemClass	  *_Precipitation [PRECIPITATION_COUNT];
		static WeatherSystemClass	  *_Environment [3];			// dust, Tiberium, falling (D-001 P23)
		static float					_EnvironmentDensity [3];
		static float					_FallingDensity;
		static float						_Wetness;
		static float						_WetnessResponseScale;
		static float						_ParticleWindScale;
		static float						_CloudWindScale;
		static bool							_FogEnabled;
		static bool							_Dirty;
};


// Externals.
extern WeatherMgrClass _TheWeatherMgr;


#endif // WEATHERMGR_H
