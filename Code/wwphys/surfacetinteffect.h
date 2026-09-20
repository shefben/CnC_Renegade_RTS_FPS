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
 *                     $Archive:: /Commando/Code/wwphys/surfacetinteffect.h                    $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	What a surface looks like once the world has happened to it.                                *
 *	Roadmap Section 52A, Phases 59 and 61.                     *
 *                                                                                             *
 *	Today a building's whole visual response to damage is one binary material swap at zero      *
 *	health (BuildingGameObj::Update_State calls Enable_Alternate_Materials), so a structure     *
 *	one shot from collapse is pixel-identical to an untouched one.  Everything needed to fix    *
 *	that already exists: the health fraction is replicated, and PhysClass has carried a         *
 *	material-effect list since stealth and transitions were written.                            *
 *                                                                                             *
 *	Two passes over whatever the object already draws, and nothing else:                        *
 *                                                                                             *
 *	  - a multiplicative soot pass, which darkens and browns the surface as health falls;       *
 *	  - an additive ember pass, which is nothing in daylight and a dull red glow at night.      *
 *                                                                                             *
 *	Tint only.  No damage-state models, no damage textures, no second material set, nothing     *
 *	for anybody to author -- which is Phase 61's own constraint, and also the reason this       *
 *	works on stock content that shipped in 2002.                                                *
 *                                                                                             *
 *	Repair reverses it because the tint follows the health fraction rather than latching on     *
 *	damage, and it ramps rather than snapping so that a repair gun reads as healing rather      *
 *	than as a flicker.                                                                          *
 *                                                                                             *
 *	The global scale defaults to zero, which makes both passes identities and costs one         *
 *	compare per object per frame.  That is the "degrades to today's appearance" rule.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SURFACETINTEFFECT_H
#define SURFACETINTEFFECT_H

#ifndef MATERIALEFFECT_H
#include "materialeffect.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

class MaterialPassClass;
class VertexMaterialClass;


class SurfaceTintEffectClass : public MaterialEffectClass
{
public:

	SurfaceTintEffectClass(void);
	virtual ~SurfaceTintEffectClass(void);

	virtual void	Timestep(float dt) override;
	virtual void	Render_Push(RenderInfoClass & rinfo,PhysClass * obj) override;
	virtual void	Render_Pop(RenderInfoClass & rinfo) override;

	/*
	**	Two phases share one effect, and that is deliberate.  Both want the same two passes over
	**	the same object -- something multiplicative to darken it and something additive on top --
	**	so giving each its own effect would mean two entries in every object's effect list, two
	**	material passes pushed where one would do, and the same object walked twice per frame.
	**	One effect carrying both terms is the cheaper and the more honest structure.
	**
	**	1.0 is pristine, 0.0 is destroyed.  Set it every time the object's health changes; the
	**	displayed tint walks toward it rather than jumping, so repair looks like repair.
	*/
	void		Set_Health_Fraction(float fraction);
	float		Get_Health_Fraction(void) const			{ return HealthFraction; }
	float		Get_Displayed_Fraction(void) const		{ return DisplayedFraction; }

	//	How dark it is right now, for anything that wants to know without re-deriving it.
	float		Get_Soot_Amount(void) const;

	/*
	**	Phase 59's half: how wet this surface is, 0 to 1.  Set per object rather than read from
	**	the global, because shelter is per object -- a truck under a bridge does not get wet,
	**	and the caller is the one that knows whether it can see the sky.
	*/
	void		Set_Wetness(float wetness);
	float		Get_Wetness(void) const						{ return Wetness; }

	/*
	**	How much of the night the embers are allowed to show through.  1 is full night, 0 is
	**	broad daylight; the sky clock is the natural source.  Set globally because the whole
	**	world shares one sky.
	*/
	static void		Set_Night_Factor(float night);
	static float	Get_Night_Factor(void);

	/*
	**	Master scale, 0 to 1.  Zero -- the default -- is the appearance this engine had before
	**	Phase 61, exactly: Render_Push returns without pushing anything.
	*/
	static void		Set_Global_Scale(float scale);
	static float	Get_Global_Scale(void);

	/*
	**	The same thing for Phase 59's half, kept separate so the two phases can be turned on
	**	and judged independently -- they change different things about the picture and there is
	**	no reason a decision about one should be a decision about the other.
	*/
	static void		Set_Wetness_Scale(float scale);
	static float	Get_Wetness_Scale(void);

	/*
	**	The pure parts, which is where the look actually lives and the only part provable
	**	without a device.
	*/

	//	The multiplicative tint at a given health.  White at full health, so a pristine object
	//	multiplied by it is unchanged bit for bit; soot-brown and dark at zero.
	static Vector3	Compute_Soot_Tint(float health_fraction,float global_scale);

	/*
	**	Phase 59.  Wet albedo is darker albedo: water fills the surface's microscopic roughness,
	**	light that would have scattered straight back out is instead bounced around inside the
	**	film and absorbed.  That is why a wet pavement is nearly black and why this is a
	**	multiply rather than a tint -- it takes light away, it does not add a colour.
	**
	**	Slightly cooler as well as darker, because the water film is a weak blue filter.
	**	White at zero wetness, so a dry world multiplies by exactly one.
	*/
	static Vector3	Compute_Wet_Albedo(float wetness,float global_scale);

	/*
	**	And the other half of what wet looks like: it is shinier.  This is the specular term the
	**	additive pass lifts, and it is what actually reads as "wet" rather than merely "dark" --
	**	a surface that only darkened would read as being in shadow.
	*/
	static float	Compute_Wet_Specular(float wetness,float global_scale);

	//	How strongly the embers glow.  Nothing until the object is badly hurt, and nothing at
	//	all in daylight, because a glowing building at noon reads as a bug.
	static float	Compute_Ember_Intensity(float health_fraction,float night_factor,float global_scale);

	//	One step of the tint following the health.  Fast enough to keep up with a fight, slow
	//	enough that a repair gun does not strobe.
	static float	Approach(float current,float target,float dt);

private:

	bool		Build_Passes(void);
	void		Update_Passes(void);

	float		HealthFraction;
	float		DisplayedFraction;
	float		Wetness;
	bool		PassesPushed;

	MaterialPassClass *		SootPass;
	MaterialPassClass *		EmberPass;
	VertexMaterialClass *	SootMaterial;
	VertexMaterialClass *	EmberMaterial;
};

#endif	// SURFACETINTEFFECT_H
