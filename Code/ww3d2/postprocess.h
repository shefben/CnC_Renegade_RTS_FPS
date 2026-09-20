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
 *                     $Archive:: /Commando/Code/ww3d2/postprocess.h                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	One post chain, shared.  Roadmap Section 52A, Phases 56 and 57.                             *
 *                                                                                             *
 *	Phase 57 says in as many words that bloom shares the render-target owner with Phase 56      *
 *	"as one post chain rather than two", so both live here and the target is allocated once.    *
 *                                                                                             *
 *	What this pipeline can and cannot do, stated plainly because it shapes both phases:         *
 *                                                                                             *
 *	  - Render targets are real.  DX8Wrapper::Create_Render_Target works and the shadow         *
 *	    projectors have been using it all along.  They are forced square and power-of-two.      *
 *	  - There are no pixel shaders.  Nothing in this tree ever calls CreatePixelShader, and     *
 *	    no material program uses the programmable tier even where the caps report it.           *
 *                                                                                             *
 *	So a per-pixel filmic tonemap operator is not on the table, and pretending otherwise        *
 *	would mean writing a shader pipeline as a side effect of a tonemap phase.  What is on the   *
 *	table is the hardware gamma ramp -- a genuine 256-entry per-channel lookup applied to       *
 *	every pixel -- and that is where Phase 56's exposure and grading go.  This class owns the   *
 *	*adaptation*: measuring the scene, moving the exposure toward it at eye-like rates, and     *
 *	handing the result to DX8Wrapper::Set_Tone_Scale.  See docs/zerohour/VisualFidelityProgram  *
 *	for what that costs against a true screen-space operator.                                   *
 *                                                                                             *
 *	Bloom is a real threshold/blur/add.  The threshold is free: dazzle.cpp already marks the    *
 *	glare sources, so only marked sources are ever accumulated.  They are drawn as soft         *
 *	sprites into the shared target, and the target is composited back over the frame several    *
 *	times at sub-pixel offsets, which is the blur -- bilinear filtering does the work a blur    *
 *	kernel would, and on this pipeline it is the only way to get one.                           *
 *                                                                                             *
 *	Both default off, and both off is bit-for-bit today's image: no target is allocated, no     *
 *	ramp call is made, and nothing is drawn.                                                    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

class TextureClass;
class Render2DClass;


class PostProcessChainClass
{
public:

	/*
	**	The shared target.  Allocated on first use by whichever phase needs it, released on
	**	shutdown or a device reset.  Square and power-of-two, because the device demands it.
	*/
	static bool		Allocate_Targets(int size = 256);
	static void		Release_Targets(void);
	static bool		Has_Targets(void);
	static int		Get_Target_Size(void);
	static void		Shutdown(void);

	/*=========================================================================================
	**	Phase 56 -- tonemapping and eye adaptation.
	**=======================================================================================*/

	static void		Enable_Tonemap(bool enabled);
	static bool		Is_Tonemap_Enabled(void);

	/*
	**	The scene's own brightness, as the thing driving the exposure.  Not measured off the
	**	frame buffer -- reading a render target back costs a pipeline stall this engine cannot
	**	absorb -- but handed in by the one part of the codebase that already knows it: the
	**	background manager, which owns the sun, the ambient and the sky clock.  A caller that
	**	also knows how much sky the camera can see passes it, and a camera indoors then adapts
	**	the way an eye does when it steps inside.
	**
	**	sky_visibility is 0 for fully enclosed, 1 for open sky.
	*/
	static void		Set_Scene_Brightness(float sun_intensity,float ambient_intensity,float sky_visibility);
	static float	Get_Scene_Brightness(void);

	/*
	**	The grading tint, from the sky clock.  Warm near the horizon, neutral at noon, cool at
	**	night.  White is no grading at all.
	*/
	static void		Set_Grading_Tint(const Vector3 & tint);
	static const Vector3 &	Get_Grading_Tint(void);

	/*
	**	How long the eye takes.  Two rates, because they are not the same: walking into the
	**	dark takes far longer to adjust to than walking back out into the light.
	*/
	static void		Set_Adaptation_Rates(float to_light_seconds,float to_dark_seconds);
	static float	Get_Light_Adaptation_Rate(void);
	static float	Get_Dark_Adaptation_Rate(void);

	static float	Get_Exposure(void);
	static float	Get_Target_Exposure(void);

	/*
	**	Move the exposure toward its target and push the ramp.  Called once a frame.  With the
	**	tonemap disabled this restores the neutral ramp once and then does nothing.
	*/
	static void		Update(float dt);

	/*
	**	The pure parts, exposed because they are what can be proved without a device.
	*/

	//	What the scene's brightness asks the exposure to be.  Brighter scene, lower exposure:
	//	this is the reciprocal relationship an auto-exposure has, clamped so neither a black
	//	room nor a white sky can drive it somewhere it cannot come back from.
	static float	Compute_Target_Exposure(float scene_brightness);

	//	One step of adaptation.  Exponential approach, so it never overshoots and never quite
	//	arrives, which is also how the eye behaves.
	static float	Adapt(float current,float target,float dt,float rate_seconds);

	//	Scene brightness from its three inputs.  Sky visibility is the term that makes an
	//	interior dark even at noon.
	static float	Compute_Scene_Brightness(float sun_intensity,float ambient_intensity,float sky_visibility);

	/*=========================================================================================
	**	Phase 57 -- bloom on emissive and dazzle sources.
	**=======================================================================================*/

	static void		Enable_Bloom(bool enabled);
	static bool		Is_Bloom_Enabled(void);

	static void		Set_Bloom_Intensity(float intensity);
	static float	Get_Bloom_Intensity(void);
	static void		Set_Bloom_Threshold(float threshold);
	static float	Get_Bloom_Threshold(void);

	/*
	**	One frame's worth of glare.  Begin, add every marked source, render.  Sources below
	**	the threshold are dropped at Add time, which is the threshold pass: it costs nothing
	**	because dazzle.cpp has already computed each source's visibility for its own draw.
	**
	**	screen_position is the same normalised device coordinate DazzleRenderObjClass keeps
	**	in current_vloc: -1..1 across the screen, Y up.
	*/
	static void		Begin_Glare(void);
	static void		Add_Glare(const Vector3 & screen_position,float intensity,const Vector3 & color,float size);
	static int		Get_Glare_Count(void);
	static void		Render_Bloom(void);

	//	Whether a source of this intensity contributes at all.  Pure, and the whole of the
	//	threshold stage.
	static bool		Passes_Threshold(float intensity,float threshold);

	/*=========================================================================================
	**	Phase 55 -- sun shafts, reimplemented on this chain.
	**=======================================================================================*/

	/*
	**	A real radial blur about a point on screen, which is what Phase 55's acceptance text
	**	asked for and what its first implementation could not give: at the time there was no
	**	render target to blur into, so it drew a row of streak sprites instead and said so.
	**
	**	This composites the shared target back over the frame several times, each pass scaled
	**	up about the source's own screen position.  Successive scaling about a point *is* a
	**	radial blur -- every bright pixel smears along the line joining it to that point, which
	**	is exactly what light scattering around an occluder does.
	**
	**	occlusion is the sun dazzle's own visibility, so the shafts are hidden by whatever hides
	**	the sun, using the depth test that already exists rather than a second one.
	*/
	static void		Render_Radial_Streaks(const Vector3 & screen_position,float occlusion,
													  const Vector3 & color,float size);

	//	How strong a streak pass is at a given step out from the source.  Pure.
	static float	Compute_Streak_Weight(int step,int step_count,float occlusion);

private:

	static bool		Build_Glow_Texture(void);
};

#endif	// POSTPROCESS_H
