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
 *                     $Archive:: /Commando/Code/Combat/WeatherMgr.cpp        $*
 *                                                                                             *
 *                       Author:: Ian Leslie		                                               *
 *                                                                                             *
 *                     $Modtime:: 1/02/02 1:26p                                     $*
 *                                                                                             *
 *                    $Revision:: 27                                                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// Includes.
#include "WeatherMgr.h"
#include "apppackettypes.h"
#include "assetmgr.h"
#include "AudibleSound.h"
#include "camera.h"
#include "chunkio.h"
#include "combat.h"
#include "gameobjmanager.h"
#include "gametype.h"
#include "light.h"
#include "dx8indexbuffer.h"
#include "dx8wrapper.h"
#include "phys.h"
#include "physcoltest.h"
#include "pscene.h"
#include "rinfo.h"
#include "scene.h"
#include "sortingrenderer.h"
#include "SoundEnvironment.h"
#include "WWAudio.h"
#include "wwmemlog.h"


// Singletons.
WeatherMgrClass _TheWeatherMgr;


// Static data.
SoundEnvironmentClass						*WeatherMgrClass::_SoundEnvironment;
WeatherParameterClass						 WeatherMgrClass::_Parameters [PARAMETER_COUNT];
bool												 WeatherMgrClass::_Imported;
unsigned											 WeatherMgrClass::_WindOverrideCount;
unsigned											 WeatherMgrClass::_PrecipitationOverrideCount;

WindClass										*WeatherMgrClass::_Wind;
WeatherSystemClass							*WeatherMgrClass::_Precipitation [PRECIPITATION_COUNT];
bool												 WeatherMgrClass::_FogEnabled;
bool												 WeatherMgrClass::_Dirty;


/***********************************************************************************************
 * WindClass::WindClass --																							  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/09/01    IML : Created.                                                                *
 *=============================================================================================*/
WindClass::WindClass (float heading, float speed, float variability, SoundEnvironmentClass *soundenvironment)
	: Velocity (0.0f, 0.0f),
	  SoundEnvironment (soundenvironment)
{
	const char *windsamplename	= "Wind01";

	Set (heading, speed, variability);

	for (unsigned octave = 0; octave < OCTAVE_COUNT; octave++) {
		Theta [octave] = 0.0l;
	}

	// Create the wind sound effect.
	Sound = WWAudioClass::Get_Instance()->Create_Sound (windsamplename, nullptr, 0, CLASSID_2D);
	if (Sound != nullptr) {
		SoundEnvironment->Add_User();
		Sound->Set_Volume (0.0f);
		Sound->Play();
	}
}


/***********************************************************************************************
 * WindClass::~WindClass --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/09/01    IML : Created.                                                                *
 *=============================================================================================*/
WindClass::~WindClass()
{
	// Remove wind sound effect.
	if (Sound != nullptr) {
		Sound->Stop();
		REF_PTR_RELEASE (Sound);
		SoundEnvironment->Remove_User();
	}
}


/***********************************************************************************************
 * WindClass::Set --																									  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/09/01    IML : Created.                                                                *
 *=============================================================================================*/
void WindClass::Set (float heading, float speed, float variability)
{
	WWASSERT (speed >= 0.0f);

	Heading		= DEG_TO_RADF (heading);
	Speed			= speed;
	Variability = variability;
}


/***********************************************************************************************
 * WindClass::Update --																								  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/09/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WindClass::Update()
{
	const double frequency [OCTAVE_COUNT] = {0.5, 0.2};
	const double twopi						  = WWMATH_PI * 2.0;

	float	h, speed;

	if (Variability > 0.0f) {

		float f = 0.0f;
		int	d;

		for (unsigned octave = 0; octave < OCTAVE_COUNT; octave++) {

			Theta [octave] += WW3D::Get_Frame_Time() * 0.001 * frequency [octave];
			d = int(WWMath::Floor (Theta [octave] / twopi));
			if (d >= 1) Theta [octave] -= d * twopi;
			f += float(WWMath::Sin (Theta [octave]));
		}
		speed = Speed - (Speed * ((f + 1.0f) * 0.5f) * Variability);
	} else {
		speed = Speed;
	}

	h = Heading + (0.5f * WWMATH_PI);
	Velocity.Set (WWMath::Cos (h) * speed, WWMath::Sin (h) * speed);

	// Update wind sound effect.
	if (Sound != nullptr) {

		const float maxvolumespeed	= 10.0f;

		float attenuation;

		// Precalculate volume attenuation based on speed.
		attenuation = (0.5f * (std::min (Speed, maxvolumespeed) + std::min (speed, maxvolumespeed))) / maxvolumespeed;
		Sound->Set_Volume (SoundEnvironment->Get_Amplitude() * attenuation);
	}

	// Is the wind essentially idle (ie. it is not contributing to the scene visually or audibly)?
	return (Speed > 0.0f);
}


/***********************************************************************************************
 * WeatherSceneCollisionClass::Cast_Weather_Column --                                          *
 *                                                                                             *
 * The stock ray, unchanged: terrain only, treated as a projectile, from the top of the fall   *
 * straight down.  What differs is how often it is asked.  The old system cast one ray per     *
 * fall line and re-cast it every time that line drifted out of the emitter -- which, because  *
 * the emitter follows the camera, is constantly.  These answers are cached against a world    *
 * lattice instead, so walking away from a column and back again costs nothing.                *
 *=============================================================================================*/
bool WeatherSceneCollisionClass::Cast_Weather_Column (const Vector3 &top, const Vector3 &bottom,
																		Vector3 &point, Vector3 &normal)
{
	if (Scene == nullptr) {
		point = bottom;
		return (false);
	}

	LineSegClass				  raycast (top, bottom);
	CastResultStruct			  rayresult;
	PhysRayCollisionTestClass raytest (raycast, &rayresult, TERRAIN_ONLY_COLLISION_GROUP, COLLISION_TYPE_PROJECTILE);

	Scene->Cast_Ray (raytest);
	raycast.Compute_Point (raytest.Result->Fraction, &point);

	if (raytest.Result->Fraction < 1.0f) {
		normal = raytest.Result->Normal;
		return (true);
	}

	// Nothing under this column but the bottom of the box: a particle reaching it has not
	// landed on anything, and so does not splash, settle, or stay.
	return (false);
}


/***********************************************************************************************
 * WeatherSystemClass::WeatherSystemClass --                                                   *
 *=============================================================================================*/
WeatherSystemClass::WeatherSystemClass (PhysicsSceneClass *scene, WeatherModeType mode, float density)
	: Collision (scene),
	  Renderer (mode),
	  Density (density)
{
	Renderer.Set_Collision_Provider (&Collision);
	Renderer.Set_Density (density);

	// The weather is drawn in world space, like the particles it replaced.
	Set_Transform (Matrix3D (1));
}


/***********************************************************************************************
 * WeatherSystemClass::~WeatherSystemClass --                                                  *
 *=============================================================================================*/
WeatherSystemClass::~WeatherSystemClass()
{
}


/***********************************************************************************************
 * WeatherSystemClass::Set_Density --                                                          *
 *                                                                                             *
 * Density means what it always meant -- particles per square world unit -- and it still       *
 * arrives here from WeatherMgrClass's ramped parameters.  It used to resize a list of rays;   *
 * now it only picks how far apart the fall lines are, which is decided afresh every frame, so *
 * this stores it and nothing else happens until something is drawn.                           *
 *=============================================================================================*/
void WeatherSystemClass::Set_Density (float density)
{
	Density = density;
	Renderer.Set_Density (density);
}


/***********************************************************************************************
 * WeatherSystemClass::Update --                                                               *
 *=============================================================================================*/
bool WeatherSystemClass::Update (WindClass *wind, const Vector3 &cameraposition)
{
	float time = WW3D::Get_Frame_Time() * 0.001f;

	Renderer.Set_Wind ((wind != nullptr) ? wind->Get_Velocity() : Vector2 (0.0f, 0.0f));
	Renderer.Update (time, cameraposition);

	// The box moved with the camera, so whatever the scene remembered about where this object
	// is no longer holds.
	Invalidate_Cached_Bounding_Volumes();

	// The stock system had to keep running after its density reached zero, to let the particles
	// already falling finish falling.  Nothing is in flight here -- a particle exists only for
	// as long as it is being looked at -- so weather ends the moment it stops.
	return (Density > 0.0f);
}


/***********************************************************************************************
 * WeatherSystemClass::Render --                                                               *
 *=============================================================================================*/
void WeatherSystemClass::Render (RenderInfoClass &rinfo)
{
	// Sort level 31, exactly as before: weather is transparent and belongs behind the sorted
	// geometry of its level.
	if (WW3D::Are_Static_Sort_Lists_Enabled()) {
		WW3D::Add_To_Static_Sort_List (this, 31);
		return;
	}

	Renderer.Render (rinfo);
}


/***********************************************************************************************
 * WeatherSystemClass::Get_Obj_Space_Bounding_Sphere --                                        *
 *                                                                                             *
 * Object space in name only, as it was in the stock engine: the transform is identity and     *
 * these are world coordinates.                                                                *
 *=============================================================================================*/
void WeatherSystemClass::Get_Obj_Space_Bounding_Sphere (SphereClass &sphere) const
{
	Vector3 minimum, maximum;

	Renderer.Get_Bounds (minimum, maximum);
	sphere.Init ((minimum + maximum) * 0.5f, ((maximum - minimum) * 0.5f).Length());
}


/***********************************************************************************************
 * WeatherSystemClass::Get_Obj_Space_Bounding_Box --                                           *
 *=============================================================================================*/
void WeatherSystemClass::Get_Obj_Space_Bounding_Box (AABoxClass &box) const
{
	Vector3 minimum, maximum;

	Renderer.Get_Bounds (minimum, maximum);
	box.Init ((minimum + maximum) * 0.5f, (maximum - minimum) * 0.5f);
}


/***********************************************************************************************
 * RainSystemClass::RainSystemClass --                                                         *
 *=============================================================================================*/
RainSystemClass::RainSystemClass (PhysicsSceneClass *scene, float particledensity, SoundEnvironmentClass *soundenvironment)
	: WeatherSystemClass (scene, WEATHER_MODE_RAIN, particledensity),
	  SoundEnvironment (soundenvironment)
{
	const char *rainsamplename	= "Rainfall01";

	// Create the rain sound effect.
	Sound = WWAudioClass::Get_Instance()->Create_Sound (rainsamplename, nullptr, 0, CLASSID_2D);
	if (Sound != nullptr) {
		SoundEnvironment->Add_User();
		Sound->Set_Volume (0.0f);
		Sound->Play();
 	}
}


/***********************************************************************************************
 * RainSystemClass::~RainSystemClass --                                                        *
 *=============================================================================================*/
RainSystemClass::~RainSystemClass()
{
	// Remove rain sound effect.
	if (Sound != nullptr) {
		Sound->Stop();
		REF_PTR_RELEASE (Sound);
		SoundEnvironment->Remove_User();
	}
}


/***********************************************************************************************
 * RainSystemClass::Update --                                                                  *
 *                                                                                             *
 * The rainfall loop is as loud as the rain is heavy.  Stock measured that by the number of    *
 * live particles, which is the same number this asks the renderer for -- with the difference  *
 * that it is now the number actually drawn last frame rather than the number in existence,    *
 * so rain that is entirely behind the camera is quieter.  That is arguably more correct and   *
 * certainly not louder.                                                                       *
 *=============================================================================================*/
bool RainSystemClass::Update (WindClass *wind, const Vector3 &cameraposition)
{
	if (Sound != nullptr) {

		const float maxvolume			= 4.0f;
		const float volumeperparticle	= 0.0025f;

		float attenuation;

		attenuation = MIN (Get_Drawn_Particles() * volumeperparticle, maxvolume) / maxvolume;
		Sound->Set_Volume (SoundEnvironment->Get_Amplitude() * attenuation);
	}

	// Base class update.
	return (WeatherSystemClass::Update (wind, cameraposition));
}


/***********************************************************************************************
 * WeatherParameterClass::Initialize --																		  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/24/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherParameterClass::Initialize()
{
	CurrentValue	  = 0.0f;
	NormalTarget	  = 0.0f;
	NormalDuration	  = 0.0f;
	OverrideTarget	  = 0.0f;
	OverrideDuration = 0.0f;
}


/***********************************************************************************************
 * WeatherParameterClass::Set --																					  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/24/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherParameterClass::Set (float target, float ramptime, bool override)
{
	if (!override) {
		NormalTarget	  = target;
		NormalDuration	  = ramptime;
	} else {
		OverrideTarget	  = target;
		OverrideDuration = ramptime;
	}
}


/***********************************************************************************************
 * WeatherParameterClass::Update --																				  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/24/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherParameterClass::Update (float time, bool override)
{
	const float previouscurrentvalue = CurrentValue;

	// Update the normal parameters.
	Update (NormalValue, NormalTarget, NormalDuration, time);

	// Update the override parameters?
	if (override) {
		Update (CurrentValue, OverrideTarget, OverrideDuration, time);
	} else {
		if (OverrideDuration > 0.0f) {
			Update (CurrentValue, NormalValue, OverrideDuration, time);
		} else {
			CurrentValue = NormalValue;
		}
	}

	return (CurrentValue != previouscurrentvalue);
}


/***********************************************************************************************
 * WeatherParameterClass::Update --																				  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/24/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherParameterClass::Update (float &value, float &target, float &duration, float time)
{
	if (value == target) {
		duration = 0.0f;
	} else {
		duration -= time;
		if (duration > 0.0f) {

			bool sign0, sign1;

			sign0 = value < target;
			value += ((target - value) * (time / duration));
			if (value == target) {
				duration = 0.0f;
			} else {
				sign1 = value < target;

				// If the value has 'blown past' the target value...
				if (sign0 ^ sign1) {
					duration = 0.0f;
					value		= target;
				}
			}
		} else {
			duration = 0.0f;
			value	   = target;
		}
	}
}


/***********************************************************************************************
 * WeatherMgrClass::WeatherMgrClass --																			  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
WeatherMgrClass::WeatherMgrClass()
{
	Set_Network_ID (NETID_SERVER_WEATHER);
	Set_App_Packet_Type (APPPACKETTYPE_NETWEATHER);
}


/***********************************************************************************************
 * WeatherMgrClass::Init --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Init (SoundEnvironmentClass *soundenvironment)
{
	WWASSERT (soundenvironment != nullptr);

	REF_PTR_SET (_SoundEnvironment, soundenvironment);

	_Wind = nullptr;
	for (int p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++) {
		_Precipitation [p] = nullptr;
	}

	Reset();
}


/***********************************************************************************************
 * WeatherMgrClass::Reset --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Reset()
{
	int p;

	// Iterate and initialize the parameters.
	for (p = 0; p < PARAMETER_COUNT; p++) {
		_Parameters [p].Initialize();
	}

	if (_Wind != nullptr) {
		delete _Wind;
		_Wind = nullptr;
	}

	//	Restore the settings to default.
	Set_Wind (0.0f, 0.0f, 0.0f, 0.0f, false);
	for (p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++) {
		if (_Precipitation [p] != nullptr) {
			_Precipitation [p]->Remove();
  			REF_PTR_RELEASE (_Precipitation [p]);
			Set_Precipitation ((PrecipitationEnum) p, 0.0f, 0.0f, false);
		}
	}
	Set_Fog_Enable (false);
	Set_Fog_Range (200.0f, 300.0f);

	_Imported = false;
	_WindOverrideCount = 0;
	_PrecipitationOverrideCount = 0;

	Set_Dirty();
}


/***********************************************************************************************
 * WeatherMgrClass::Shutdown --																					  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Shutdown()
{
	Reset();
	REF_PTR_RELEASE (_SoundEnvironment);
}


/***********************************************************************************************
 * WeatherMgrClass::Set_Wind --																					  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Set_Wind (float heading, float speed, float variability, float ramptime)
{
	if (CombatManager::I_Am_Server()) {
		if (Set_Wind (heading, speed, variability, ramptime, false)) {
			_TheWeatherMgr.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
			return (true);
		}
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Override_Wind --																			  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Override_Wind (float heading, float speed, float variability, float ramptime)
{
	if (CombatManager::I_Am_Server()) {
		_WindOverrideCount++;
		if (Set_Wind (heading, speed, variability, ramptime, true)) {
			_TheWeatherMgr.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
			return (true);
		}
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Set_Wind --																					  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Set_Wind (float heading, float speed, float variability, float ramptime, bool override)
{
	if ((heading >= 0.0f) && (heading <= 360.0f) && (speed >= 0.0f) && (variability >= 0.0f) &&
		(variability <= 1.0f) && (ramptime >= 0.0f)) {

		bool o;

		o = (_WindOverrideCount > 0) && override;
		_Parameters [PARAMETER_WIND_HEADING].Set (heading, ramptime, o);
		_Parameters [PARAMETER_WIND_SPEED].Set (speed, ramptime, o);
		_Parameters [PARAMETER_WIND_VARIABILITY].Set (variability, ramptime, o);
		return (true);
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Get_Wind --																					  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Get_Wind (float &heading, float &speed, float &variability)
{
	heading		= _Parameters [PARAMETER_WIND_HEADING].Value();
	speed			= _Parameters [PARAMETER_WIND_SPEED].Value();
	variability = _Parameters [PARAMETER_WIND_VARIABILITY].Value();
}


/***********************************************************************************************
 * WeatherMgrClass::Restore_Wind --																				  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Restore_Wind (float ramptime)
{
	if (CombatManager::I_Am_Server()) {
		_Parameters [PARAMETER_WIND_HEADING].Set (ramptime);
		_Parameters [PARAMETER_WIND_SPEED].Set (ramptime);
		_Parameters [PARAMETER_WIND_VARIABILITY].Set (ramptime);
		_WindOverrideCount--;
		_TheWeatherMgr.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	}
}


/***********************************************************************************************
 * WeatherMgrClass::Set_Precipitation --																		  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Set_Precipitation (PrecipitationEnum precipitation, float density, float ramptime)
{
	if (CombatManager::I_Am_Server()) {
		if (Set_Precipitation (precipitation, density, ramptime, false)) {
			_TheWeatherMgr.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
			return (true);
		}
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Override_Precipitation --																  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Override_Precipitation (PrecipitationEnum precipitation, float density, float ramptime)
{
	if (CombatManager::I_Am_Server()) {

		bool success;

		_PrecipitationOverrideCount++;

		// Override requested precipitation but also override and ramp down any other types of precipitation that may exist.
		success = true;
		for (int p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++) {
			if (p != precipitation) {
				success &= Set_Precipitation ((PrecipitationEnum) p, 0.0f, ramptime, true);
			} else {
				success &= Set_Precipitation ((PrecipitationEnum) p, density, ramptime, true);
			}
		}
		_TheWeatherMgr.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
		return (success);
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Set_Precipitation --																		  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Set_Precipitation (PrecipitationEnum precipitation, float density, float ramptime, bool override)
{
	if ((density >= 0.0f) && (ramptime >= 0.0f)) {

		const bool o = (_PrecipitationOverrideCount > 0) && override;

		switch (precipitation) {

			case PRECIPITATION_RAIN:
				_Parameters [PARAMETER_RAIN_DENSITY].Set (density, ramptime, o);
				break;

			case PRECIPITATION_SNOW:
				_Parameters [PARAMETER_SNOW_DENSITY].Set (density, ramptime, o);
				break;

			case PRECIPITATION_ASH:
				_Parameters [PARAMETER_ASH_DENSITY].Set (density, ramptime, o);
				break;

			default:
				WWASSERT (false);
				break;
		}
		return (true);
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Get_Precipitation --																		  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Get_Precipitation (PrecipitationEnum precipitation, float &density)
{
	switch (precipitation) {

		case PRECIPITATION_RAIN:
			density = _Parameters [PARAMETER_RAIN_DENSITY].Value();
			break;

		case PRECIPITATION_SNOW:
			density = _Parameters [PARAMETER_SNOW_DENSITY].Value();
			break;

		case PRECIPITATION_ASH:
			density = _Parameters [PARAMETER_ASH_DENSITY].Value();
			break;

		default:
			WWASSERT (false);
			break;
	}
}


/***********************************************************************************************
 * WeatherMgrClass::Restore_Precipitation --																	  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Restore_Precipitation (float ramptime)
{
	if (CombatManager::I_Am_Server()) {
		_Parameters [PARAMETER_RAIN_DENSITY].Set (ramptime);
		_Parameters [PARAMETER_SNOW_DENSITY].Set (ramptime);
		_Parameters [PARAMETER_ASH_DENSITY].Set (ramptime);
		_PrecipitationOverrideCount--;
		_TheWeatherMgr.Set_Object_Dirty_Bit (NetworkObjectClass::BIT_RARE, true);
	}
}


/***********************************************************************************************
 * WeatherMgrClass::Set_Fog_Range --																			  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/12/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Set_Fog_Range (float startdistance, float enddistance, float ramptime)
{
	if ((startdistance >= 0.0f) && (enddistance >= startdistance)) {
		_Parameters [PARAMETER_FOG_START_DISTANCE].Set (startdistance, ramptime, false);
		_Parameters [PARAMETER_FOG_END_DISTANCE].Set (enddistance, ramptime, false);
		return (true);
	}
	return (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Get_Fog_Range --																			  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/12/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Get_Fog_Range (float &startdistance, float &enddistance)
{
	startdistance = _Parameters [PARAMETER_FOG_START_DISTANCE].Value();
	enddistance	  = _Parameters [PARAMETER_FOG_END_DISTANCE].Value();
}


/***********************************************************************************************
 * WeatherMgrClass::Update --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/06/01    IML : Created.                                                                *
 *=============================================================================================*/
void WeatherMgrClass::Update (PhysicsSceneClass *scene, CameraClass *camera)
{
	float	time;
	bool	windmodified, fogmodified;

	time = WW3D::Get_Frame_Time() * 0.001f;

	// Update wind.
	windmodified  = _Parameters [PARAMETER_WIND_HEADING].Update (time, _WindOverrideCount > 0);
	windmodified |= _Parameters [PARAMETER_WIND_SPEED].Update (time, _WindOverrideCount > 0);
	windmodified |= _Parameters [PARAMETER_WIND_VARIABILITY].Update (time, _WindOverrideCount > 0);
	if (_Wind != nullptr) {
		if (windmodified) {
			_Wind->Set (_Parameters [PARAMETER_WIND_HEADING].Value(), _Parameters [PARAMETER_WIND_SPEED].Value(), _Parameters [PARAMETER_WIND_VARIABILITY].Value());
		}

		// Optimization: if there is nothing to update, can safely remove the wind.
		if (!_Wind->Update()) {
			delete _Wind;
			_Wind = nullptr;
		}

	} else {

	  	// Optimization: only create wind if speed is non-zero.
		if (_Parameters [PARAMETER_WIND_SPEED].Value() > 0.0f) {
			_Wind = new WindClass (_Parameters [PARAMETER_WIND_HEADING].Value(), _Parameters [PARAMETER_WIND_SPEED].Value(), _Parameters [PARAMETER_WIND_VARIABILITY].Value(), _SoundEnvironment);
		}
	}

	for (int p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++) {

		WeatherParameterClass *parameterptr = nullptr;
		bool						  modified;

		switch (p) {

		  	case PRECIPITATION_RAIN:
				parameterptr = &_Parameters [PARAMETER_RAIN_DENSITY];
				break;

		  	case PRECIPITATION_SNOW:
				parameterptr = &_Parameters [PARAMETER_SNOW_DENSITY];
				break;

		  	case PRECIPITATION_ASH:
				parameterptr = &_Parameters [PARAMETER_ASH_DENSITY];
				break;

			default:
				WWASSERT (false);
				break;
		}

		modified = parameterptr->Update (time, _PrecipitationOverrideCount > 0);
	  	if (_Precipitation [p] != nullptr) {
	  		if (modified) {
	  			_Precipitation [p]->Set_Density (parameterptr->Value());
	  		}

	  		// Optimization: if there is nothing to update, can safely remove the weather system.
	  		if (!_Precipitation [p]->Update (_Wind, camera->Get_Position())) {
				_Precipitation [p]->Remove();
	  			REF_PTR_RELEASE (_Precipitation [p]);
	  		}

	  	} else {

			// Optimization: only create weather if density is non-zero.
	  	  	if (parameterptr->Value() > 0.0f) {

				// Don't bother on dedicated servers.
				if (!CombatManager::I_Am_Only_Server()) {

					switch (p) {

	  					case PRECIPITATION_RAIN:
	  						_Precipitation [p] = NEW_REF (RainSystemClass, (scene, parameterptr->Value(), _SoundEnvironment));
	  						break;

	  					case PRECIPITATION_SNOW:
	  						_Precipitation [p] = NEW_REF (WeatherSystemClass, (scene, WEATHER_MODE_SNOW, parameterptr->Value()));
	  						break;

	  					case PRECIPITATION_ASH:
	  						_Precipitation [p] = NEW_REF (WeatherSystemClass, (scene, WEATHER_MODE_ASH, parameterptr->Value()));
	  						break;

						default:
							WWASSERT (false);
							break;
					}
					scene->Add_Render_Object (_Precipitation [p]);
				}
			}
		}
	}

	fogmodified  = _Parameters [PARAMETER_FOG_START_DISTANCE].Update (time, false);
	fogmodified |= _Parameters [PARAMETER_FOG_END_DISTANCE].Update (time, false);
	if (Is_Dirty() || fogmodified) {
		scene->Set_Fog_Enable (_FogEnabled);
		scene->Set_Fog_Range (_Parameters [PARAMETER_FOG_START_DISTANCE].Value(), _Parameters [PARAMETER_FOG_END_DISTANCE].Value());
	}

	// Everything necessary has been updated. Clear the dirty flag.
	Set_Dirty (false);
}


/***********************************************************************************************
 * WeatherMgrClass::Save --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
 #define WRITE_PARAMETER(varname) \
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _CURRENT_VALUE, _Parameters [PARAMETER_ ## varname].CurrentValue); \
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _NORMAL_VALUE, _Parameters [PARAMETER_ ## varname].NormalValue);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _NORMAL_TARGET, _Parameters [PARAMETER_ ## varname].NormalTarget);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _NORMAL_DURATION, _Parameters [PARAMETER_ ## varname].NormalDuration);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _OVERRIDE_TARGET, _Parameters [PARAMETER_ ## varname].OverrideTarget);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _OVERRIDE_DURATION, _Parameters [PARAMETER_ ## varname].OverrideDuration)

bool WeatherMgrClass::Save (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_MICRO_CHUNKS);
	csave.End_Chunk ();

	Save_Dynamic (csave);
	return (true);
}


/***********************************************************************************************
 * WeatherMgrClass::Save_Dynamic --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Save_Dynamic (ChunkSaveClass &csave)
{
	csave.Begin_Chunk (CHUNKID_DYNAMIC_MICRO_CHUNKS);
	WRITE_PARAMETER (WIND_HEADING);
	WRITE_PARAMETER (WIND_SPEED);
	WRITE_PARAMETER (WIND_VARIABILITY);
	WRITE_PARAMETER (RAIN_DENSITY);
	WRITE_PARAMETER (SNOW_DENSITY);
	WRITE_PARAMETER (ASH_DENSITY);
	WRITE_MICRO_CHUNK (csave, VARID_WIND_OVERRIDE_COUNT, _WindOverrideCount);
	WRITE_MICRO_CHUNK (csave, VARID_PRECIPITATION_OVERRIDE_COUNT, _PrecipitationOverrideCount);
	WRITE_MICRO_CHUNK (csave, VARID_FOG_ENABLED, _FogEnabled);
	WRITE_PARAMETER (FOG_START_DISTANCE);
	WRITE_PARAMETER (FOG_END_DISTANCE);
	csave.End_Chunk ();
	return (true);
}


/***********************************************************************************************
 * WeatherMgrClass::Load --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Load (ChunkLoadClass &cload)
{
	WWMEMLOG (MEM_GAMEDATA);

	bool retval = true;

	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_MICRO_CHUNKS:
				retval &= Load_Micro_Chunks (cload);
				break;

			case CHUNKID_DYNAMIC_MICRO_CHUNKS:
				retval &= Load_Dynamic_Micro_Chunks (cload);
				break;
		}
		cload.Close_Chunk ();
	}
	return (retval);
}


/***********************************************************************************************
 * WeatherMgrClass::Load_Micro_Chunks --																		  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
#define READ_PARAMETER(varname) \
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _CURRENT_VALUE, _Parameters [PARAMETER_ ## varname].CurrentValue); \
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _NORMAL_VALUE, _Parameters [PARAMETER_ ## varname].NormalValue);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _NORMAL_TARGET, _Parameters [PARAMETER_ ## varname].NormalTarget);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _NORMAL_DURATION, _Parameters [PARAMETER_ ## varname].NormalDuration);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _OVERRIDE_TARGET, _Parameters [PARAMETER_ ## varname].OverrideTarget);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _OVERRIDE_DURATION, _Parameters [PARAMETER_ ## varname].OverrideDuration)

bool WeatherMgrClass::Load_Micro_Chunks (ChunkLoadClass &cload)
{
	// Read weather micro chunk.
	while (cload.Open_Micro_Chunk ()) {
		cload.Close_Micro_Chunk ();
	}

	return (true);
}


/***********************************************************************************************
 * WeatherMgrClass::Load_Dynamic --																						  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Load_Dynamic (ChunkLoadClass &cload)
{
	WWMEMLOG (MEM_GAMEDATA);

	bool retval = true;

	while (cload.Open_Chunk ()) {
		switch (cload.Cur_Chunk_ID ()) {

			case CHUNKID_DYNAMIC_MICRO_CHUNKS:
				retval &= Load_Dynamic_Micro_Chunks (cload);
				break;
		}
		cload.Close_Chunk ();
	}
	return (retval);
}


/***********************************************************************************************
 * WeatherMgrClass::Load_Dynamic_Micro_Chunks --																		  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
bool WeatherMgrClass::Load_Dynamic_Micro_Chunks (ChunkLoadClass &cload)
{
	// Read weather micro chunk.
	while (cload.Open_Micro_Chunk ()) {
		switch (cload.Cur_Micro_Chunk_ID ()) {
			READ_PARAMETER (WIND_HEADING);
			READ_PARAMETER (WIND_SPEED);
			READ_PARAMETER (WIND_VARIABILITY);
			READ_PARAMETER (RAIN_DENSITY);
			READ_PARAMETER (SNOW_DENSITY);
			READ_PARAMETER (ASH_DENSITY);
			READ_MICRO_CHUNK (cload, VARID_WIND_OVERRIDE_COUNT, _WindOverrideCount);
			READ_MICRO_CHUNK (cload, VARID_PRECIPITATION_OVERRIDE_COUNT, _PrecipitationOverrideCount);
			READ_MICRO_CHUNK (cload, VARID_FOG_ENABLED, _FogEnabled);
			READ_PARAMETER (FOG_START_DISTANCE);
			READ_PARAMETER (FOG_END_DISTANCE);
		}
		cload.Close_Micro_Chunk ();
	}

	return (true);
}


/***********************************************************************************************
 * WeatherMgrClass::Export_Rare --																				  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
#define EXPORT_PARAMETER(object, varname) \
object.Add (_Parameters [PARAMETER_ ## varname].NormalTarget); \
object.Add (_Parameters [PARAMETER_ ## varname].NormalDuration); \
object.Add (_Parameters [PARAMETER_ ## varname].OverrideTarget); \
object.Add (_Parameters [PARAMETER_ ## varname].OverrideDuration)

void WeatherMgrClass::Export_Rare (BitStreamClass &packet)
{
	WWASSERT (CombatManager::I_Am_Server());

	EXPORT_PARAMETER (packet, WIND_HEADING);
	EXPORT_PARAMETER (packet, WIND_SPEED);
	EXPORT_PARAMETER (packet, WIND_VARIABILITY);
	EXPORT_PARAMETER (packet, RAIN_DENSITY);
	EXPORT_PARAMETER (packet, SNOW_DENSITY);
	EXPORT_PARAMETER (packet, ASH_DENSITY);
	packet.Add (_WindOverrideCount);
	packet.Add (_PrecipitationOverrideCount);
}


/***********************************************************************************************
 * WeatherMgrClass::Import_Rare --																				  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/02/01    IML : Created.                                                                *
 *=============================================================================================*/
#define IMPORT_PARAMETER(object, varname) \
object.Get (_Parameters [PARAMETER_ ## varname].NormalTarget); \
object.Get (_Parameters [PARAMETER_ ## varname].NormalDuration); \
object.Get (_Parameters [PARAMETER_ ## varname].OverrideTarget); \
object.Get (_Parameters [PARAMETER_ ## varname].OverrideDuration)

void WeatherMgrClass::Import_Rare (BitStreamClass &packet)
{
	WWASSERT (CombatManager::I_Am_Client());

	IMPORT_PARAMETER (packet, WIND_HEADING);
	IMPORT_PARAMETER (packet, WIND_SPEED);
	IMPORT_PARAMETER (packet, WIND_VARIABILITY);
	IMPORT_PARAMETER (packet, RAIN_DENSITY);
	IMPORT_PARAMETER (packet, SNOW_DENSITY);
	IMPORT_PARAMETER (packet, ASH_DENSITY);
	packet.Get (_WindOverrideCount);
	packet.Get (_PrecipitationOverrideCount);

	// Flag that weather data has been imported.
	_Imported = true;
}


#undef EXPORT_PARAMETER
#undef IMPORT_PARAMETER
#undef READ_PARAMETER
#undef WRITE_PARAMETER
