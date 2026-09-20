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
 *                     $Archive:: /Commando/Code/wwphys/surfacetinteffect.cpp                  $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "surfacetinteffect.h"

#include "matpass.h"
#include "rinfo.h"
#include "shader.h"
#include "vertmaterial.h"
#include "wwmath.h"


/*
**	The colour a completely wrecked surface is multiplied by.  Dark, and browner than it is
**	grey, because soot over paint reads warm rather than neutral.  Not black: a building that
**	multiplies to nothing is a silhouette, and a silhouette is a worse picture than the stock
**	one, which Section 52A forbids.
*/
static const Vector3 FULL_SOOT_TINT(0.38f,0.32f,0.28f);

/*
**	Where the soot starts.  A building that has taken one bullet should not be visibly charred,
**	so nothing happens in the top of the health range at all.
*/
static const float SOOT_START_HEALTH	= 0.85f;

/*
**	Where the embers start, and what colour they are.  Much later than the soot: a structure
**	is scorched long before anything inside it is actually burning.
*/
static const float EMBER_START_HEALTH	= 0.35f;
static const Vector3 EMBER_COLOR(1.00f,0.32f,0.08f);
static const float EMBER_MAX_INTENSITY	= 0.45f;

/*
**	How fast the displayed tint follows the real health, in health-fraction per second.  Fast
**	enough to keep up with a firefight, slow enough that a repair gun ticking health up reads
**	as healing rather than as a strobe.
*/
static const float APPROACH_RATE			= 0.6f;

/*
**	Roadmap Section 52A, Phase 59.  What a soaked surface multiplies down to, and how shiny it
**	gets.  The darkening is strong because wet albedo genuinely is -- a wet pavement is close to
**	half the brightness of a dry one -- and slightly cool, because the water film filters that
**	way.  Not black, for the same reason the soot tint is not: a surface multiplied to nothing
**	is a silhouette.
*/
static const Vector3 FULL_WET_ALBEDO(0.55f,0.57f,0.62f);
static const float FULL_WET_SPECULAR	= 0.55f;
static const float WET_SHININESS			= 0.85f;



static float	_NightFactor	= 0.0f;
static float	_GlobalScale	= 0.0f;
static float	_WetnessScale	= 0.0f;


/***********************************************************************************************
 * SurfaceTintEffectClass -- construction                                                       *
 *=============================================================================================*/
SurfaceTintEffectClass::SurfaceTintEffectClass(void) :
	HealthFraction(1.0f),
	DisplayedFraction(1.0f),
	Wetness(0.0f),
	PassesPushed(false),
	SootPass(nullptr),
	EmberPass(nullptr),
	SootMaterial(nullptr),
	EmberMaterial(nullptr)
{
}


SurfaceTintEffectClass::~SurfaceTintEffectClass(void)
{
	REF_PTR_RELEASE(SootPass);
	REF_PTR_RELEASE(EmberPass);
	REF_PTR_RELEASE(SootMaterial);
	REF_PTR_RELEASE(EmberMaterial);
}


void SurfaceTintEffectClass::Set_Night_Factor(float night)
{
	_NightFactor = WWMath::Clamp(night,0.0f,1.0f);
}

float SurfaceTintEffectClass::Get_Night_Factor(void)	{ return _NightFactor; }


void SurfaceTintEffectClass::Set_Global_Scale(float scale)
{
	_GlobalScale = WWMath::Clamp(scale,0.0f,1.0f);
}

float SurfaceTintEffectClass::Get_Global_Scale(void)	{ return _GlobalScale; }


void SurfaceTintEffectClass::Set_Wetness_Scale(float scale)
{
	_WetnessScale = WWMath::Clamp(scale,0.0f,1.0f);
}

float SurfaceTintEffectClass::Get_Wetness_Scale(void)	{ return _WetnessScale; }


void SurfaceTintEffectClass::Set_Health_Fraction(float fraction)
{
	HealthFraction = WWMath::Clamp(fraction,0.0f,1.0f);
}


void SurfaceTintEffectClass::Set_Wetness(float wetness)
{
	Wetness = WWMath::Clamp(wetness,0.0f,1.0f);
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Compute_Wet_Albedo -- rain takes light away                         *
 *=============================================================================================*/
Vector3 SurfaceTintEffectClass::Compute_Wet_Albedo(float wetness,float global_scale)
{
	if (global_scale <= 0.0f) {
		return Vector3(1.0f,1.0f,1.0f);
	}

	float wet = WWMath::Clamp(wetness,0.0f,1.0f) * WWMath::Clamp(global_scale,0.0f,1.0f);
	if (wet <= 0.0f) {
		return Vector3(1.0f,1.0f,1.0f);
	}

	return Vector3(1.0f + ((FULL_WET_ALBEDO.X - 1.0f) * wet),
						1.0f + ((FULL_WET_ALBEDO.Y - 1.0f) * wet),
						1.0f + ((FULL_WET_ALBEDO.Z - 1.0f) * wet));
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Compute_Wet_Specular -- and gives some back as a highlight          *
 *=============================================================================================*/
float SurfaceTintEffectClass::Compute_Wet_Specular(float wetness,float global_scale)
{
	if (global_scale <= 0.0f) {
		return 0.0f;
	}
	float wet = WWMath::Clamp(wetness,0.0f,1.0f) * WWMath::Clamp(global_scale,0.0f,1.0f);
	return wet * FULL_WET_SPECULAR;
}


float SurfaceTintEffectClass::Get_Soot_Amount(void) const
{
	Vector3 tint = Compute_Soot_Tint(DisplayedFraction,_GlobalScale);
	return 1.0f - tint.X;
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Compute_Soot_Tint -- the multiplier at a given health                *
 *                                                                                             *
 * White above SOOT_START_HEALTH, so an undamaged object multiplied by this is unchanged, and  *
 * so is every object in the world while the global scale is zero.  That is what makes the     *
 * feature disabled identical rather than approximately identical.                             *
 *=============================================================================================*/
Vector3 SurfaceTintEffectClass::Compute_Soot_Tint(float health_fraction,float global_scale)
{
	if (global_scale <= 0.0f) {
		return Vector3(1.0f,1.0f,1.0f);
	}

	health_fraction = WWMath::Clamp(health_fraction,0.0f,1.0f);

	if (health_fraction >= SOOT_START_HEALTH) {
		return Vector3(1.0f,1.0f,1.0f);
	}

	//	0 at the threshold, 1 at zero health.
	float soot = (SOOT_START_HEALTH - health_fraction) / SOOT_START_HEALTH;
	soot = WWMath::Clamp(soot,0.0f,1.0f) * WWMath::Clamp(global_scale,0.0f,1.0f);

	return Vector3(1.0f + ((FULL_SOOT_TINT.X - 1.0f) * soot),
						1.0f + ((FULL_SOOT_TINT.Y - 1.0f) * soot),
						1.0f + ((FULL_SOOT_TINT.Z - 1.0f) * soot));
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Compute_Ember_Intensity --                                           *
 *=============================================================================================*/
float SurfaceTintEffectClass::Compute_Ember_Intensity(float health_fraction,float night_factor,
																	  float global_scale)
{
	if (global_scale <= 0.0f) {
		return 0.0f;
	}

	health_fraction = WWMath::Clamp(health_fraction,0.0f,1.0f);
	night_factor = WWMath::Clamp(night_factor,0.0f,1.0f);

	if (health_fraction >= EMBER_START_HEALTH) {
		return 0.0f;
	}
	if (night_factor <= 0.0f) {
		return 0.0f;
	}

	float burn = (EMBER_START_HEALTH - health_fraction) / EMBER_START_HEALTH;
	burn = WWMath::Clamp(burn,0.0f,1.0f);

	return burn * night_factor * EMBER_MAX_INTENSITY * WWMath::Clamp(global_scale,0.0f,1.0f);
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Approach --                                                          *
 *=============================================================================================*/
float SurfaceTintEffectClass::Approach(float current,float target,float dt)
{
	if (dt <= 0.0f) {
		return current;
	}

	float step = APPROACH_RATE * dt;

	if (target > current) {
		current += step;
		if (current > target) { current = target; }
	} else {
		current -= step;
		if (current < target) { current = target; }
	}

	return WWMath::Clamp(current,0.0f,1.0f);
}


void SurfaceTintEffectClass::Timestep(float dt)
{
	DisplayedFraction = Approach(DisplayedFraction,HealthFraction,dt);
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Build_Passes -- two passes, made once                                *
 *                                                                                             *
 * The soot pass multiplies, which is the only way to darken something: an additive pass can    *
 * only ever brighten, which is why the transition and stealth effects could not have been      *
 * reused for this however similar they look.                                                   *
 *                                                                                             *
 * Both are untextured.  Their colour comes from the vertex material's emissive term with       *
 * lighting off, so the tint is exactly the number computed rather than that number lit.        *
 *=============================================================================================*/
bool SurfaceTintEffectClass::Build_Passes(void)
{
	if (SootPass != nullptr) {
		return true;
	}

	SootPass = NEW_REF(MaterialPassClass,());
	EmberPass = NEW_REF(MaterialPassClass,());
	if ((SootPass == nullptr) || (EmberPass == nullptr)) {
		return false;
	}

	//	A translucent mesh multiplied a second time goes much darker than an opaque one for the
	//	same tint, so it is left out, the same way the transition effect leaves it out.
	SootPass->Enable_On_Translucent_Meshes(false);
	EmberPass->Enable_On_Translucent_Meshes(false);

	SootMaterial = NEW_REF(VertexMaterialClass,());
	SootMaterial->Set_Ambient(0.0f,0.0f,0.0f);
	SootMaterial->Set_Diffuse(0.0f,0.0f,0.0f);
	SootMaterial->Set_Specular(0.0f,0.0f,0.0f);
	SootMaterial->Set_Emissive(1.0f,1.0f,1.0f);
	SootMaterial->Set_Opacity(1.0f);
	SootMaterial->Set_Shininess(0.0f);
	SootMaterial->Set_Lighting(false);
	SootPass->Set_Material(SootMaterial);
	SootPass->Set_Shader(ShaderClass::_PresetMultiplicativeShader);

	EmberMaterial = NEW_REF(VertexMaterialClass,());
	EmberMaterial->Set_Ambient(0.0f,0.0f,0.0f);
	EmberMaterial->Set_Diffuse(0.0f,0.0f,0.0f);
	EmberMaterial->Set_Specular(0.0f,0.0f,0.0f);
	EmberMaterial->Set_Emissive(0.0f,0.0f,0.0f);
	EmberMaterial->Set_Opacity(1.0f);
	EmberMaterial->Set_Shininess(0.0f);

	//	Lighting ON for this one, unlike the soot pass.  Ambient and diffuse are black, so the
	//	only thing lighting can contribute is the specular highlight -- which is exactly what
	//	Phase 59 wants: a view-and-light-dependent sheen that moves as the player moves, rather
	//	than a flat brightening that would just wash the object out.  The embers ride along as
	//	the emissive term of the same pass, since both are additive and neither needs its own.
	EmberMaterial->Set_Lighting(true);
	EmberPass->Set_Material(EmberMaterial);
	EmberPass->Set_Shader(ShaderClass::_PresetAdditiveShader);

	return true;
}


void SurfaceTintEffectClass::Update_Passes(void)
{
	//	Soot and wet both take light away, so they compose as a single multiply.  A charred
	//	building in the rain is darker than either alone, which is correct.
	Vector3 soot = Compute_Soot_Tint(DisplayedFraction,_GlobalScale);
	Vector3 wet = Compute_Wet_Albedo(Wetness,_WetnessScale);
	SootMaterial->Set_Emissive(soot.X * wet.X,soot.Y * wet.Y,soot.Z * wet.Z);

	float ember = Compute_Ember_Intensity(DisplayedFraction,_NightFactor,_GlobalScale);
	EmberMaterial->Set_Emissive(EMBER_COLOR.X * ember,
										 EMBER_COLOR.Y * ember,
										 EMBER_COLOR.Z * ember);

	float specular = Compute_Wet_Specular(Wetness,_WetnessScale);
	EmberMaterial->Set_Specular(specular,specular,specular);
	EmberMaterial->Set_Shininess((specular > 0.0f) ? WET_SHININESS : 0.0f);
}


/***********************************************************************************************
 * SurfaceTintEffectClass::Render_Push / Render_Pop                                             *
 *                                                                                             *
 * Nothing is pushed at all while the feature is off or the object is healthy, so an undamaged  *
 * world costs one compare per object and draws exactly what it drew before.  Render_Pop pops   *
 * only what Render_Push actually pushed, which is why the flag is kept rather than recomputed  *
 * -- the health can change between the two.                                                    *
 *=============================================================================================*/
void SurfaceTintEffectClass::Render_Push(RenderInfoClass & rinfo,PhysClass * /*obj*/)
{
	PassesPushed = false;

	bool damaged = (_GlobalScale > 0.0f) && (DisplayedFraction < SOOT_START_HEALTH);
	bool soaked = (_WetnessScale > 0.0f) && (Wetness > 0.0f);

	if (!damaged && !soaked) {
		return;
	}
	if (!Build_Passes()) {
		return;
	}

	Update_Passes();

	rinfo.Push_Material_Pass(SootPass);
	rinfo.Push_Material_Pass(EmberPass);
	PassesPushed = true;
}


void SurfaceTintEffectClass::Render_Pop(RenderInfoClass & rinfo)
{
	if (!PassesPushed) {
		return;
	}

	rinfo.Pop_Material_Pass();
	rinfo.Pop_Material_Pass();
	PassesPushed = false;
}
