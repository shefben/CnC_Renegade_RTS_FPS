/*
**	OpenW3D
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
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/postprocess.cpp                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   PostProcessChainClass::Compute_Scene_Brightness -- what the eye is looking at              *
 *   PostProcessChainClass::Adapt -- one step of it                                             *
 *   PostProcessChainClass::Update -- adapt, then push the ramp                                 *
 *   PostProcessChainClass::Render_Bloom -- threshold, blur, add                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "postprocess.h"

#include "dx8wrapper.h"
#include "render2d.h"
#include "texture.h"
#include "surfaceclass.h"
#include "ww3dformat.h"
#include "wwdebug.h"
#include "wwmath.h"
#include "vector2.h"
#include "rect.h"


/*
**	Exposure limits.  A half stop down and two stops up, roughly.  The ceiling matters more
**	than the floor: an exposure that can climb without limit turns a dark corridor into a grey
**	one with no detail anywhere, which looks worse than the dark corridor did.
*/
static const float MIN_EXPOSURE			= 0.65f;
static const float MAX_EXPOSURE			= 2.20f;

/*
**	The scene brightness that wants an exposure of exactly 1.0 -- that is, the brightness at
**	which the stock image is already correctly exposed.  Anything above this darkens, anything
**	below brightens.  Set at "open daylight", because open daylight is what Renegade's levels
**	were lit and gamma-tuned for.
*/
static const float REFERENCE_BRIGHTNESS	= 1.0f;

/*
**	Default adaptation times.  Fast into the light, slow into the dark, which is the asymmetry
**	the human eye actually has and the reason stepping into a building should not be instant.
*/
static const float DEFAULT_LIGHT_SECONDS	= 0.45f;
static const float DEFAULT_DARK_SECONDS		= 2.50f;

/*
**	Bloom.  BLUR_TAPS composite passes at growing offsets stand in for a blur kernel: each
**	pass is the same texture drawn slightly larger and dimmer, and bilinear filtering spreads
**	it.  Four is where adding more stops being visible.
*/
static const int	BLUR_TAPS				= 4;
static const float	BLUR_SPREAD			= 0.012f;		// of screen width, at the widest tap
static const int	MAX_GLARE_SOURCES	= 64;

static const int	GLOW_TEXTURE_SIZE	= 32;

/*
**	Phase 55.  How many times the target is composited outward, and how far the last one grows.
**	Six is where the streak stops looking like discrete copies and starts looking continuous;
**	past about 1.9x the scaling pushes the source off the edge of the target and the streak
**	starts to thin rather than lengthen.
*/
static const int	STREAK_STEPS		= 6;
static const float	STREAK_MAX_SCALE	= 1.9f;


struct GlareSourceStruct
{
	Vector3	Position;		// normalised device coordinates, -1..1, Y up
	Vector3	Color;
	float		Intensity;
	float		Size;			// in NDC units across the target
};


static bool		_TonemapEnabled		= false;
static bool		_BloomEnabled		= false;

static float	_SceneBrightness		= REFERENCE_BRIGHTNESS;
static float	_Exposure				= 1.0f;
static float	_TargetExposure		= 1.0f;
static Vector3	_GradingTint(1.0f,1.0f,1.0f);
static float	_LightSeconds			= DEFAULT_LIGHT_SECONDS;
static float	_DarkSeconds			= DEFAULT_DARK_SECONDS;

//	True once a non-neutral ramp has actually been pushed, so that turning the feature off
//	restores the stock ramp exactly once rather than re-uploading it every frame forever.
static bool		_RampDirty				= false;

static float	_BloomIntensity		= 0.55f;
static float	_BloomThreshold		= 0.10f;

static GlareSourceStruct	_Glare[MAX_GLARE_SOURCES];
static int		_GlareCount				= 0;

static TextureClass *	_BloomTarget	= nullptr;
static TextureClass *	_GlowTexture	= nullptr;
static Render2DClass *	_GlareRenderer	= nullptr;
static Render2DClass *	_CompositeRenderer = nullptr;
static int		_TargetSize				= 0;


/***********************************************************************************************
 * PostProcessChainClass::Allocate_Targets -- the one target both phases share                 *
 *=============================================================================================*/
bool PostProcessChainClass::Allocate_Targets(int size)
{
	if (_BloomTarget != nullptr) {
		return true;
	}

	if (size < 64) {
		size = 64;
	}

	//	Create_Render_Target forces square and power-of-two itself, and returns null when the
	//	device cannot render to the display format.  Both are fine: a device that cannot do
	//	this simply does not get bloom, and says so through Has_Targets.
	_BloomTarget = DX8Wrapper::Create_Render_Target(size,size,WW3D_FORMAT_UNKNOWN);
	if (_BloomTarget == nullptr) {
		WWDEBUG_SAY(("PostProcessChain: no render target available; bloom is unavailable.\r\n"));
		return false;
	}

	_BloomTarget->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
	_BloomTarget->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
	_TargetSize = size;

	return true;
}


void PostProcessChainClass::Release_Targets(void)
{
	REF_PTR_RELEASE(_BloomTarget);
	_TargetSize = 0;

	if (_GlareRenderer != nullptr) {
		delete _GlareRenderer;
		_GlareRenderer = nullptr;
	}
	if (_CompositeRenderer != nullptr) {
		delete _CompositeRenderer;
		_CompositeRenderer = nullptr;
	}
}


bool PostProcessChainClass::Has_Targets(void)		{ return (_BloomTarget != nullptr); }
int PostProcessChainClass::Get_Target_Size(void)	{ return _TargetSize; }


void PostProcessChainClass::Shutdown(void)
{
	Release_Targets();
	REF_PTR_RELEASE(_GlowTexture);

	//	Leave the player's own gamma exactly as they set it.
	if (_RampDirty) {
		DX8Wrapper::Set_Tone_Scale(1.0f,Vector3(1.0f,1.0f,1.0f));
		_RampDirty = false;
	}

	_GlareCount = 0;
	_TonemapEnabled = false;
	_BloomEnabled = false;
	_Exposure = 1.0f;
	_TargetExposure = 1.0f;
	_SceneBrightness = REFERENCE_BRIGHTNESS;
	_GradingTint.Set(1.0f,1.0f,1.0f);
}


/*=================================================================================================
**	Phase 56 -- tonemapping and eye adaptation.
**===============================================================================================*/

void PostProcessChainClass::Enable_Tonemap(bool enabled)
{
	if (_TonemapEnabled == enabled) {
		return;
	}
	_TonemapEnabled = enabled;

	if (!enabled) {
		//	Straight back to the player's own ramp, now, not gradually.  "Disabled is
		//	pixel-identical to today" is an acceptance line, not an aspiration.
		_Exposure = 1.0f;
		_TargetExposure = 1.0f;
		if (_RampDirty) {
			DX8Wrapper::Set_Tone_Scale(1.0f,Vector3(1.0f,1.0f,1.0f));
			_RampDirty = false;
		}
	}
}


bool PostProcessChainClass::Is_Tonemap_Enabled(void)	{ return _TonemapEnabled; }


/***********************************************************************************************
 * PostProcessChainClass::Compute_Scene_Brightness -- what the eye is looking at                *
 *                                                                                             *
 * The sun contributes what the sky lets through; the ambient contributes everywhere.  That     *
 * second term is what stops an interior reading as pitch black and driving the exposure to     *
 * its ceiling the moment a door closes.                                                        *
 *                                                                                             *
 * Sky visibility is the whole reason this works: at noon, outdoors and indoors differ by       *
 * nothing else at all -- same sun, same ambient -- and it is the sky the walls take away.      *
 *=============================================================================================*/
float PostProcessChainClass::Compute_Scene_Brightness(float sun_intensity,float ambient_intensity,
																		float sky_visibility)
{
	if (sun_intensity < 0.0f)		{ sun_intensity = 0.0f; }
	if (ambient_intensity < 0.0f)	{ ambient_intensity = 0.0f; }
	sky_visibility = WWMath::Clamp(sky_visibility,0.0f,1.0f);

	return (sun_intensity * sky_visibility) + ambient_intensity;
}


void PostProcessChainClass::Set_Scene_Brightness(float sun_intensity,float ambient_intensity,
																 float sky_visibility)
{
	_SceneBrightness = Compute_Scene_Brightness(sun_intensity,ambient_intensity,sky_visibility);
	_TargetExposure = Compute_Target_Exposure(_SceneBrightness);
}


float PostProcessChainClass::Get_Scene_Brightness(void)	{ return _SceneBrightness; }


void PostProcessChainClass::Set_Grading_Tint(const Vector3 & tint)
{
	_GradingTint = tint;
}


const Vector3 & PostProcessChainClass::Get_Grading_Tint(void)	{ return _GradingTint; }


void PostProcessChainClass::Set_Adaptation_Rates(float to_light_seconds,float to_dark_seconds)
{
	_LightSeconds = (to_light_seconds > 0.01f) ? to_light_seconds : 0.01f;
	_DarkSeconds = (to_dark_seconds > 0.01f) ? to_dark_seconds : 0.01f;
}


float PostProcessChainClass::Get_Light_Adaptation_Rate(void)	{ return _LightSeconds; }
float PostProcessChainClass::Get_Dark_Adaptation_Rate(void)		{ return _DarkSeconds; }
float PostProcessChainClass::Get_Exposure(void)						{ return _Exposure; }
float PostProcessChainClass::Get_Target_Exposure(void)				{ return _TargetExposure; }


/***********************************************************************************************
 * PostProcessChainClass::Compute_Target_Exposure --                                            *
 *                                                                                             *
 * Reciprocal: twice the light wants half the exposure.  Clamped hard at both ends, because an  *
 * auto-exposure that can run away is worse than none -- a player who walks into a cupboard      *
 * should not come out unable to see the sky for three seconds.                                 *
 *=============================================================================================*/
float PostProcessChainClass::Compute_Target_Exposure(float scene_brightness)
{
	if (scene_brightness < 0.01f) {
		scene_brightness = 0.01f;
	}

	float exposure = REFERENCE_BRIGHTNESS / scene_brightness;
	return WWMath::Clamp(exposure,MIN_EXPOSURE,MAX_EXPOSURE);
}


/***********************************************************************************************
 * PostProcessChainClass::Adapt -- one step of it                                              *
 *                                                                                             *
 * Exponential approach with a per-frame-rate-independent factor, so the adaptation takes the   *
 * same wall-clock time at 30 and at 300 frames a second.                                       *
 *=============================================================================================*/
float PostProcessChainClass::Adapt(float current,float target,float dt,float rate_seconds)
{
	if (dt <= 0.0f) {
		return current;
	}
	if (rate_seconds <= 0.01f) {
		return target;
	}

	//	Reach 63% of the way in rate_seconds, which is the usual reading of a time constant.
	float alpha = 1.0f - expf(-dt / rate_seconds);
	if (alpha > 1.0f) { alpha = 1.0f; }

	return current + ((target - current) * alpha);
}


/***********************************************************************************************
 * PostProcessChainClass::Update -- adapt, then push the ramp                                   *
 *=============================================================================================*/
void PostProcessChainClass::Update(float dt)
{
	if (!_TonemapEnabled) {
		return;
	}

	//	Going up in exposure means the scene got darker, and that is the slow direction.
	float rate = (_TargetExposure > _Exposure) ? _DarkSeconds : _LightSeconds;
	_Exposure = Adapt(_Exposure,_TargetExposure,dt,rate);

	DX8Wrapper::Set_Tone_Scale(_Exposure,_GradingTint);
	_RampDirty = true;
}


/*=================================================================================================
**	Phase 57 -- bloom on emissive and dazzle sources.
**===============================================================================================*/

void PostProcessChainClass::Enable_Bloom(bool enabled)
{
	_BloomEnabled = enabled;
	if (!enabled) {
		_GlareCount = 0;
	}
}


bool PostProcessChainClass::Is_Bloom_Enabled(void)	{ return _BloomEnabled; }


void PostProcessChainClass::Set_Bloom_Intensity(float intensity)
{
	_BloomIntensity = (intensity > 0.0f) ? intensity : 0.0f;
}

float PostProcessChainClass::Get_Bloom_Intensity(void)	{ return _BloomIntensity; }


void PostProcessChainClass::Set_Bloom_Threshold(float threshold)
{
	_BloomThreshold = (threshold > 0.0f) ? threshold : 0.0f;
}

float PostProcessChainClass::Get_Bloom_Threshold(void)	{ return _BloomThreshold; }


bool PostProcessChainClass::Passes_Threshold(float intensity,float threshold)
{
	return (intensity > threshold);
}


void PostProcessChainClass::Begin_Glare(void)
{
	_GlareCount = 0;
}


int PostProcessChainClass::Get_Glare_Count(void)	{ return _GlareCount; }


void PostProcessChainClass::Add_Glare(const Vector3 & screen_position,float intensity,
												  const Vector3 & color,float size)
{
	if (!_BloomEnabled) {
		return;
	}
	if (!Passes_Threshold(intensity,_BloomThreshold)) {
		return;
	}
	if (_GlareCount >= MAX_GLARE_SOURCES) {
		return;
	}

	//	Behind the camera, past the far plane, or off screen contributes nothing.  Dazzles
	//	off the edge of the screen do bleed in reality, but this pipeline has no way to draw
	//	the part of the sprite that is on screen without the part that is not, so they are
	//	dropped rather than clamped to the edge, which would smear a bright spot down the side.
	if ((screen_position.Z <= 0.0f) || (screen_position.Z >= 1.0f)) {
		return;
	}
	if ((WWMath::Fabs(screen_position.X) > 1.0f) || (WWMath::Fabs(screen_position.Y) > 1.0f)) {
		return;
	}

	GlareSourceStruct & glare = _Glare[_GlareCount++];
	glare.Position = screen_position;
	glare.Color = color;
	glare.Intensity = intensity;
	glare.Size = (size > 0.0f) ? size : 0.05f;
}


/***********************************************************************************************
 * PostProcessChainClass::Build_Glow_Texture -- the soft sprite every source is drawn as        *
 *                                                                                             *
 * A radial falloff, squared for a soft shoulder.  Generated rather than loaded: Section 52A's  *
 * own rule is that no phase requires new authored art, and a round white blur is exactly the   *
 * kind of thing that is cheaper to compute than to ship.  Recorded in docs/assets anyway, so   *
 * that an artist who wants to replace it knows the name and the constraints.                   *
 *=============================================================================================*/
bool PostProcessChainClass::Build_Glow_Texture(void)
{
	if (_GlowTexture != nullptr) {
		return true;
	}

	_GlowTexture = NEW_REF(TextureClass,(GLOW_TEXTURE_SIZE,GLOW_TEXTURE_SIZE,
													 WW3D_FORMAT_A8R8G8B8,TextureClass::MIP_LEVELS_1));
	if (_GlowTexture == nullptr) {
		return false;
	}

	_GlowTexture->Set_U_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
	_GlowTexture->Set_V_Addr_Mode(TextureClass::TEXTURE_ADDRESS_CLAMP);
	_GlowTexture->Set_Texture_Name("ow_bloom_glow");

	SurfaceClass * surface = _GlowTexture->Get_Surface_Level(0);
	if (surface == nullptr) {
		REF_PTR_RELEASE(_GlowTexture);
		return false;
	}

	int pitch = 0;
	unsigned char * map = (unsigned char *)surface->Lock(&pitch);
	if (map == nullptr) {
		REF_PTR_RELEASE(surface);
		REF_PTR_RELEASE(_GlowTexture);
		return false;
	}

	float center = (float)(GLOW_TEXTURE_SIZE - 1) * 0.5f;
	for (int y = 0; y < GLOW_TEXTURE_SIZE; y++) {
		for (int x = 0; x < GLOW_TEXTURE_SIZE; x++) {

			float dx = ((float)x - center) / center;
			float dy = ((float)y - center) / center;
			float dist = WWMath::Sqrt((dx * dx) + (dy * dy));
			float falloff = 1.0f - WWMath::Clamp(dist,0.0f,1.0f);
			falloff = falloff * falloff;

			unsigned char v = (unsigned char)(falloff * 255.0f);
			map[(x * 4) + 0] = v;
			map[(x * 4) + 1] = v;
			map[(x * 4) + 2] = v;
			map[(x * 4) + 3] = v;
		}
		map += pitch;
	}

	surface->Unlock();
	REF_PTR_RELEASE(surface);
	return true;
}


/***********************************************************************************************
 * PostProcessChainClass::Render_Bloom -- threshold, blur, add                                  *
 *                                                                                             *
 * Threshold happened at Add_Glare: only sources dazzle.cpp marked, and only those above the    *
 * cutoff, are in the list at all.                                                              *
 *                                                                                             *
 * Blur and add happen together.  Each source is drawn once into the shared target, and the     *
 * target is then composited back over the frame BLUR_TAPS times, each tap larger and dimmer    *
 * than the last.  The sum of those taps is a wide, soft falloff around every source -- which   *
 * is what a blur kernel would have produced, obtained on a pipeline that has no way to run     *
 * one.  Bilinear filtering on the way out of a 256-pixel target does the smoothing.            *
 *                                                                                             *
 * The target switch is bracketed the way the shadow projectors bracket theirs: set, clear,     *
 * draw, restore.  It is not wrapped in its own Begin_Render/End_Render, because unlike the     *
 * projectors this runs inside the frame that is already being rendered.                        *
 *=============================================================================================*/
void PostProcessChainClass::Render_Bloom(void)
{
	if (!_BloomEnabled || (_GlareCount < 1)) {
		return;
	}

	if (!Allocate_Targets()) {
		return;
	}
	if (!Build_Glow_Texture()) {
		return;
	}

	if (_GlareRenderer == nullptr) {
		_GlareRenderer = new Render2DClass();
		_GlareRenderer->Set_Texture(_GlowTexture);
		_GlareRenderer->Enable_Additive(true);
	}
	if (_CompositeRenderer == nullptr) {
		_CompositeRenderer = new Render2DClass();
		_CompositeRenderer->Set_Texture(_BloomTarget);
		_CompositeRenderer->Enable_Additive(true);
	}

	/*
	**	Accumulate every source into the shared target.
	*/
	float target_size = (float)_TargetSize;
	RectClass target_rect(0.0f,0.0f,target_size,target_size);

	DX8Wrapper::Set_Render_Target(_BloomTarget);
	DX8Wrapper::Clear(true,false,Vector3(0.0f,0.0f,0.0f),0.0f);

	_GlareRenderer->Set_Coordinate_Range(target_rect);
	_GlareRenderer->Reset();

	for (int i = 0; i < _GlareCount; i++) {

		const GlareSourceStruct & glare = _Glare[i];

		float px = (glare.Position.X * 0.5f + 0.5f) * target_size;
		float py = (1.0f - (glare.Position.Y * 0.5f + 0.5f)) * target_size;
		float half = glare.Size * target_size * 0.5f;

		unsigned int color = DX8Wrapper::Convert_Color(glare.Color,
																	  WWMath::Clamp(glare.Intensity,0.0f,1.0f));
		_GlareRenderer->Add_Quad(RectClass(px - half,py - half,px + half,py + half),color);
	}

	_GlareRenderer->Render();

	DX8Wrapper::Set_Render_Target((IDirect3DSurface9 *)nullptr);

	/*
	**	Composite back over the frame, widening and dimming.
	*/
	RectClass screen = Render2DClass::Get_Screen_Resolution();
	_CompositeRenderer->Set_Coordinate_Range(screen);
	_CompositeRenderer->Reset();

	float spread = screen.Width() * BLUR_SPREAD;

	for (int tap = 0; tap < BLUR_TAPS; tap++) {

		//	Tap 0 is the sharp core; each later one is wider and correspondingly weaker, so
		//	the total energy stays close to the intensity the sources asked for.
		float t = (float)tap / (float)(BLUR_TAPS - 1);
		float grow = spread * t;
		float weight = _BloomIntensity * (1.0f - (t * 0.65f)) / (float)BLUR_TAPS;

		unsigned int color = DX8Wrapper::Convert_Color(Vector3(1.0f,1.0f,1.0f),
																	  WWMath::Clamp(weight,0.0f,1.0f));

		RectClass quad(screen.Left - grow,screen.Top - grow,
							screen.Right + grow,screen.Bottom + grow);
		_CompositeRenderer->Add_Quad(quad,color);
	}

	_CompositeRenderer->Render();

	_GlareCount = 0;
}


/***********************************************************************************************
 * PostProcessChainClass::Compute_Streak_Weight --                                             *
 *                                                                                             *
 * Falls off with distance out from the source, and scales with how much of the source is       *
 * actually visible.  Fully occluded is zero at every step, which is what makes the shafts      *
 * disappear behind a wall rather than merely dim behind it.                                    *
 *=============================================================================================*/
float PostProcessChainClass::Compute_Streak_Weight(int step,int step_count,float occlusion)
{
	if ((step_count < 1) || (step < 0) || (step >= step_count)) {
		return 0.0f;
	}
	if (occlusion <= 0.0f) {
		return 0.0f;
	}

	float t = (float)step / (float)step_count;
	float falloff = 1.0f - t;

	//	Divided by the step count so the total energy does not grow with the number of taps:
	//	adding steps should make the streak smoother, not brighter.
	return falloff * WWMath::Clamp(occlusion,0.0f,1.0f) / (float)step_count;
}


/***********************************************************************************************
 * PostProcessChainClass::Render_Radial_Streaks -- Phase 55, properly this time                *
 *                                                                                             *
 * Accumulate the source into the shared target, then composite it outward from the source's    *
 * own screen position, growing each pass.  Because each pass is anchored at that point, a      *
 * bright pixel lands a little further from it every time, and the sum is a streak pointing     *
 * away from the source -- a radial blur, built out of the one operation this pipeline has.     *
 *                                                                                             *
 * It shares the target with bloom rather than allocating a second, which is the same rule      *
 * Phase 57 was given: one post chain, not two.                                                 *
 *=============================================================================================*/
void PostProcessChainClass::Render_Radial_Streaks(const Vector3 & screen_position,float occlusion,
																  const Vector3 & color,float size)
{
	if (occlusion <= 0.0f) {
		return;
	}
	//	Behind the camera or off screen: the streak would have no anchor to point away from.
	if ((screen_position.Z <= 0.0f) || (screen_position.Z >= 1.0f)) {
		return;
	}
	if ((WWMath::Fabs(screen_position.X) > 1.0f) || (WWMath::Fabs(screen_position.Y) > 1.0f)) {
		return;
	}

	if (!Allocate_Targets()) {
		return;
	}
	if (!Build_Glow_Texture()) {
		return;
	}

	if (_GlareRenderer == nullptr) {
		_GlareRenderer = new Render2DClass();
		_GlareRenderer->Set_Texture(_GlowTexture);
		_GlareRenderer->Enable_Additive(true);
	}
	if (_CompositeRenderer == nullptr) {
		_CompositeRenderer = new Render2DClass();
		_CompositeRenderer->Set_Texture(_BloomTarget);
		_CompositeRenderer->Enable_Additive(true);
	}

	/*
	**	The source, into the shared target.
	*/
	float target_size = (float)_TargetSize;
	RectClass target_rect(0.0f,0.0f,target_size,target_size);

	float sx = (screen_position.X * 0.5f + 0.5f) * target_size;
	float sy = (1.0f - (screen_position.Y * 0.5f + 0.5f)) * target_size;
	float half = ((size > 0.0f) ? size : 0.08f) * target_size * 0.5f;

	DX8Wrapper::Set_Render_Target(_BloomTarget);
	DX8Wrapper::Clear(true,false,Vector3(0.0f,0.0f,0.0f),0.0f);

	_GlareRenderer->Set_Coordinate_Range(target_rect);
	_GlareRenderer->Reset();
	_GlareRenderer->Add_Quad(RectClass(sx - half,sy - half,sx + half,sy + half),
									 DX8Wrapper::Convert_Color(color,1.0f));
	_GlareRenderer->Render();

	DX8Wrapper::Set_Render_Target((IDirect3DSurface9 *)nullptr);

	/*
	**	And outward, anchored at the source.
	*/
	RectClass screen = Render2DClass::Get_Screen_Resolution();
	_CompositeRenderer->Set_Coordinate_Range(screen);
	_CompositeRenderer->Reset();

	//	The anchor in screen pixels: the point every pass scales about.
	float ax = (screen_position.X * 0.5f + 0.5f) * screen.Width();
	float ay = (1.0f - (screen_position.Y * 0.5f + 0.5f)) * screen.Height();

	for (int step = 0; step < STREAK_STEPS; step++) {

		float weight = Compute_Streak_Weight(step,STREAK_STEPS,occlusion);
		if (weight <= 0.0f) {
			continue;
		}

		float scale = 1.0f + ((STREAK_MAX_SCALE - 1.0f) * ((float)step / (float)STREAK_STEPS));

		//	Scale the full-screen quad about the anchor rather than about its own centre.  That
		//	is the whole trick: the anchor is a fixed point of the transform, so everything else
		//	moves away from it.
		RectClass quad(ax + ((screen.Left   - ax) * scale),
							ay + ((screen.Top    - ay) * scale),
							ax + ((screen.Right  - ax) * scale),
							ay + ((screen.Bottom - ay) * scale));

		_CompositeRenderer->Add_Quad(quad,
			DX8Wrapper::Convert_Color(Vector3(1.0f,1.0f,1.0f),WWMath::Clamp(weight,0.0f,1.0f)));
	}

	_CompositeRenderer->Render();
}
