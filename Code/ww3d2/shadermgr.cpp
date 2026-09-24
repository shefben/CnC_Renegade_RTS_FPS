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
 *                     $Archive:: /Commando/Code/ww3d2/shadermgr.cpp                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   ShaderManagerClass::Init -- detect the tier and build the programs                        *
 *   ShaderManagerClass::Shutdown -- give everything back                                      *
 *   ShaderManagerClass::Detect_Tier -- ask the device what it can do                          *
 *   ShaderManagerClass::Register_Program -- hand a program to the manager                     *
 *   ShaderManagerClass::Get_Pass_Count -- how many submissions a program needs                *
 *   ShaderManagerClass::Set_Program -- make one pass of one program current                   *
 *   ShaderManagerClass::Reset_Program -- give the device back                                 *
 *   ShaderManagerClass::Set_Texture -- stage a texture for the programs to read               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "shadermgr.h"

#include "dx8wrapper.h"
#include "dx8caps.h"
#include "shader.h"
#include "texture.h"
#include "vertmaterial.h"
#include "wwdebug.h"


MaterialProgramClass *	ShaderManagerClass::Programs[MATERIAL_PROGRAM_COUNT] = { nullptr };
TextureClass *				ShaderManagerClass::Textures[ShaderManagerClass::MAX_TEXTURE_STAGES] = { nullptr };
MaterialProgramType		ShaderManagerClass::CurrentProgram = MATERIAL_PROGRAM_COUNT;
int							ShaderManagerClass::CurrentPass = 0;
ShaderTierType				ShaderManagerClass::Tier = SHADER_TIER_FIXED_FUNCTION;
bool							ShaderManagerClass::Initialized = false;


/*
** The names, in enumeration order, for diagnostics and for the self check.  Kept next to
** the enum it mirrors rather than inside a function, so that adding a program without
** naming it is a compile error and not a silent blank.
*/
static const char * _ProgramNames[MATERIAL_PROGRAM_COUNT] =
{
	"legacy_w3d",
	"terrain",
	"terrain_detail",
	"road",
	"bridge",
	"water",
	"foliage",
	"projected_shadow",
	"particle",
	"tracer",
	"status_marker",
	"ghost_tint",
	"debug_overlay",
};


/***********************************************************************************************
**	Stock W3D content.
**
**	One pass, and that pass sets nothing.  That is not a placeholder: a W3D material already
**	carries its own ShaderClass and its own VertexMaterialClass, and the renderer applies
**	them as it draws.  What this program contributes is the guarantee that it is a program,
**	so a stock Renegade model goes through the same layer as everything else and the layer
**	has nothing to say about it.  A model exported in 2002 renders in exactly the state it
**	rendered in before this class existed.
***********************************************************************************************/
class LegacyW3DProgramClass : public MaterialProgramClass
{
public:
	virtual const char *	Get_Name(void) const override		{ return _ProgramNames[MATERIAL_PROGRAM_LEGACY_W3D]; }
	virtual bool			Init(ShaderTierType) override		{ return true; }
	virtual int				Get_Pass_Count(void) const override	{ return 1; }
	virtual void			Set_Pass(int pass) override		{ WWASSERT(pass == 0); (void)pass; }
	virtual void			Reset(void) override				{ }
};


/***********************************************************************************************
**	Debug overlays.
**
**	The boxes the engine draws over the world when a display mask is on: collision boxes,
**	bounding boxes, the vis sectors.  They were the first thing in this tree to set a
**	material, a shader and a texture stage straight on DX8Wrapper and then leave, which is
**	precisely the arrangement Section 15 exists to replace, and they are the one pipeline in
**	the list whose consumer was already here.
**
**	Emissive white with vertex alpha: the colour of a debug box is per-vertex, so one material
**	serves every box and the program can own it.
***********************************************************************************************/
class DebugOverlayProgramClass : public MaterialProgramClass
{
public:
	DebugOverlayProgramClass(void) : Material(nullptr) { }
	virtual ~DebugOverlayProgramClass(void) override	{ Shutdown(); }

	virtual const char *	Get_Name(void) const override		{ return _ProgramNames[MATERIAL_PROGRAM_DEBUG_OVERLAY]; }
	virtual int				Get_Pass_Count(void) const override	{ return 1; }

	virtual bool	Init(ShaderTierType) override
	{
		//	Fixed function is enough, and asked for: a debug box wants to be cheap and to look
		//	the same on every machine somebody is debugging on.
		WWASSERT(Material == nullptr);
		Material = NEW_REF(VertexMaterialClass,());
		Material->Set_Ambient(0,0,0);
		Material->Set_Diffuse(0,0,0);
		Material->Set_Specular(0,0,0);
		Material->Set_Emissive(1,1,1);
		Material->Set_Opacity(1.0f);		// uses vertex alpha
		Material->Set_Shininess(0.0f);
		return true;
	}

	virtual void	Shutdown(void) override
	{
		REF_PTR_RELEASE(Material);
	}

	virtual void	Set_Pass(int pass) override
	{
		WWASSERT(pass == 0);
		(void)pass;

		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(ShaderClass::_PresetAlphaSolidShader);
		DX8Wrapper::Set_Texture(0,nullptr);
	}

	virtual void	Reset(void) override
	{
		//	Only the stage.  Material and shader are set by whatever draws next -- every draw
		//	in this renderer sets its own -- so putting a null material back would be inventing
		//	a state nobody asked for rather than restoring one.
		DX8Wrapper::Set_Texture(0,nullptr);
	}

private:
	VertexMaterialClass *	Material;
};


/***********************************************************************************************
**	A pipeline that is a shader, a material and a set of texture stages.
**
**	Every surface in this engine is drawn by fixed-function state: a ShaderClass, a
**	VertexMaterialClass and what is in the stages.  Six of Section 15's pipelines differ from
**	each other only in what those are, so they differ from each other here only in what their
**	Init sets, and the answer to "why is water not culled and foliage is" is two lines apart in
**	one file instead of two systems apart in two.
**
**	It holds the state rather than computing it at draw time, which is what lets Get_Pass_State
**	hand out the identical thing Set_Pass installs.  A road mesh and a terrain draw therefore
**	cannot disagree about what their pipeline is, because there is only one of it.
***********************************************************************************************/
class FixedFunctionProgramClass : public MaterialProgramClass
{
public:

	enum { MAX_PASSES = 2 };

	FixedFunctionProgramClass(MaterialProgramType type,int pass_count,int stage_count) :
		Type(type), PassCount(pass_count), StageCount(stage_count)
	{
		WWASSERT(pass_count >= 1 && pass_count <= MAX_PASSES);
		for (int i=0; i<MAX_PASSES; i++) {
			Materials[i] = nullptr;
		}
	}

	virtual ~FixedFunctionProgramClass(void) override		{ FixedFunctionProgramClass::Shutdown(); }

	virtual const char *	Get_Name(void) const override		{ return _ProgramNames[Type]; }
	virtual int				Get_Pass_Count(void) const override	{ return PassCount; }

	virtual void	Shutdown(void) override
	{
		for (int i=0; i<MAX_PASSES; i++) {
			REF_PTR_RELEASE(Materials[i]);
		}
	}

	virtual void	Set_Pass(int pass) override
	{
		WWASSERT(pass >= 0 && pass < PassCount);

		DX8Wrapper::Set_Shader(Shaders[pass]);

		//	A pipeline with no material of its own leaves the current one alone.  The projected
		//	shadow pass is drawn that way and always has been; inventing a material for it here
		//	would be this layer changing a picture rather than owning the state behind it.
		if (Materials[pass] != nullptr) {
			DX8Wrapper::Set_Material(Materials[pass]);
		}

		//	What the caller staged, including the stages it deliberately left empty.
		for (int stage=0; stage<StageCount; stage++) {
			DX8Wrapper::Set_Texture(stage,ShaderManagerClass::Peek_Texture(stage));
		}
	}

	virtual void	Reset(void) override
	{
		//	Only the stages, for the reason the debug overlay program gives: every draw in this
		//	renderer sets its own material and shader, so what the next pipeline needs back is a
		//	stage it did not inherit.
		for (int stage=0; stage<StageCount; stage++) {
			DX8Wrapper::Set_Texture(stage,nullptr);
		}

		//	And the staging area with them, so the next pipeline has to say what it draws with
		//	rather than inheriting what the last one happened to leave -- and so a texture is
		//	not held alive here by a system that has since been torn down.
		ShaderManagerClass::Reset_Textures();
	}

	virtual bool	Get_Pass_State(int pass,ShaderClass * shader_out,VertexMaterialClass ** material_out) const override
	{
		if ((pass < 0) || (pass >= PassCount)) {
			return false;
		}

		if (shader_out != nullptr) {
			*shader_out = Shaders[pass];
		}

		if (material_out != nullptr) {
			*material_out = Materials[pass];
			if (Materials[pass] != nullptr) {
				Materials[pass]->Add_Ref();
			}
		}

		return true;
	}

protected:

	void	Set_Pass_Shader(int pass,const ShaderClass & shader)
	{
		WWASSERT(pass >= 0 && pass < PassCount);
		Shaders[pass] = shader;
	}

	void	Set_Pass_Material(int pass,VertexMaterialClass * material)
	{
		WWASSERT(pass >= 0 && pass < PassCount);
		REF_PTR_SET(Materials[pass],material);
	}

	MaterialProgramType		Type;
	int							PassCount;
	int							StageCount;
	ShaderClass					Shaders[MAX_PASSES];
	VertexMaterialClass *	Materials[MAX_PASSES];
};


/*
**	The material the generated world's surfaces are lit by.  Ambient and diffuse full, nothing
**	specular or emissive, lighting on: the light environment decides what the ground looks like.
*/
static VertexMaterialClass *	Make_World_Surface_Material(bool colour_from_vertex)
{
	VertexMaterialClass * material = NEW_REF(VertexMaterialClass,());
	material->Set_Ambient(1.0f,1.0f,1.0f);
	material->Set_Diffuse(1.0f,1.0f,1.0f);
	material->Set_Specular(0,0,0);
	material->Set_Emissive(0.0f,0.0f,0.0f);
	material->Set_Opacity(1.0f);
	material->Set_Shininess(0.0f);
	material->Set_Lighting(true);

	if (colour_from_vertex) {
		material->Set_Ambient_Color_Source(VertexMaterialClass::COLOR1);
		material->Set_Diffuse_Color_Source(VertexMaterialClass::COLOR1);
	}

	return material;
}


/*
**	Geometry whose lighting is already in its vertices: roads, bridges, water, foliage.
**
**	This is VertexMaterialClass::PRELIT_DIFFUSE, built rather than fetched.  The presets are
**	created by VertexMaterialClass::Init, which a dedicated server never reaches -- Get_Preset
**	dereferences a null pointer there -- and this class has to answer on a server, because the
**	FDS builds the same road and water geometry a client does.  Building it also means no
**	pipeline can reach through Get_Preset and change a material every mesh in the engine shares.
*/
static VertexMaterialClass *	Make_Prelit_Diffuse_Material(void)
{
	VertexMaterialClass * material = NEW_REF(VertexMaterialClass,());
	material->Set_Diffuse_Color_Source(VertexMaterialClass::COLOR1);
	material->Set_Lighting(false);
	return material;
}


/***********************************************************************************************
**	Terrain.
**
**	Two passes, because that is how the ground is composited and always has been: the first
**	layer with any influence on a quad draws opaque, and the rest blend over it.  Pass zero is
**	that base and pass one is a layer, which is why they differ in exactly one thing -- opaque
**	against alpha -- and agree about everything else including fog.
**
**	The base pass has two materials and the caller picks between them, because whether the
**	ground's colour comes from the light environment or from the vertex is a fact about the
**	patch, not about terrain.  The layer pass always takes it from the vertex: that is where
**	the layer's own blend weight lives.
**
**	This state used to be four members on every RenegadeTerrainPatchClass, rebuilt per patch and
**	mutated during Render when the patch turned out to be prelit.  A thousand patches meant a
**	thousand copies of two materials that were always identical, and a draw-time mutation that
**	nothing put back.
***********************************************************************************************/
class TerrainProgramClass : public FixedFunctionProgramClass
{
public:

	enum { PASS_BASE = TERRAIN_PASS_BASE, PASS_LAYER = TERRAIN_PASS_LAYER, STAGE_COUNT = 2 };

	TerrainProgramClass(MaterialProgramType type = MATERIAL_PROGRAM_TERRAIN,bool detail = false) :
		FixedFunctionProgramClass(type,2,STAGE_COUNT),
		Detail(detail), LitMaterial(nullptr), VertexLitMaterial(nullptr) { }

	virtual ~TerrainProgramClass(void) override	{ TerrainProgramClass::Shutdown(); }

	virtual bool	Init(ShaderTierType) override
	{
		LitMaterial			= Make_World_Surface_Material(false);
		VertexLitMaterial	= Make_World_Surface_Material(true);

		ShaderClass base = ShaderClass::_PresetOpaqueShader;
		base.Set_Fog_Func(ShaderClass::FOG_ENABLE);

		ShaderClass layer = ShaderClass::_PresetAlphaShader;
		layer.Set_Fog_Func(ShaderClass::FOG_ENABLE);

		//	D-001 P09 terrain detail: the same two passes with a second texture multiplied over
		//	the first, read through the vertex's second UV set -- the detail rate, which repeats
		//	far more often than the layer's own.  Alpha stays the first stage's, so a layer's
		//	blend weight is untouched by the grain laid over it.
		if (Detail) {
			LitMaterial->Set_UV_Source(1,1);
			VertexLitMaterial->Set_UV_Source(1,1);
			base.Set_Post_Detail_Color_Func(ShaderClass::DETAILCOLOR_SCALE);
			layer.Set_Post_Detail_Color_Func(ShaderClass::DETAILCOLOR_SCALE);
		}

		Set_Pass_Material(PASS_LAYER,VertexLitMaterial);

		Set_Pass_Shader(PASS_BASE,base);
		Set_Pass_Shader(PASS_LAYER,layer);

		Set_Variant(TERRAIN_VARIANT_LIT);
		return true;
	}

	virtual void	Shutdown(void) override
	{
		REF_PTR_RELEASE(LitMaterial);
		REF_PTR_RELEASE(VertexLitMaterial);
		FixedFunctionProgramClass::Shutdown();
	}

	virtual void	Set_Variant(int variant) override
	{
		Set_Pass_Material(PASS_BASE,
			(variant == TERRAIN_VARIANT_VERTEX_LIT) ? VertexLitMaterial : LitMaterial);
	}

private:

	bool							Detail;
	VertexMaterialClass *	LitMaterial;
	VertexMaterialClass *	VertexLitMaterial;
};


/***********************************************************************************************
**	Roads and bridges.
**
**	Opaque, lit off the vertex.  They are two programs rather than one because they are two
**	entries in Section 15's list and because the moment either grows a wet-road sheen or a
**	damaged-span decal it stops matching the other -- and the point of the layer is that such a
**	change happens in one place and is visible next to what it is no longer the same as.
***********************************************************************************************/
class OpaqueVertexLitProgramClass : public FixedFunctionProgramClass
{
public:

	OpaqueVertexLitProgramClass(MaterialProgramType type) :
		FixedFunctionProgramClass(type,1,1) { }

	virtual bool	Init(ShaderTierType) override
	{
		VertexMaterialClass * material = Make_Prelit_Diffuse_Material();
		Set_Pass_Material(0,material);
		REF_PTR_RELEASE(material);

		Set_Pass_Shader(0,ShaderClass::_PresetOpaqueShader);
		return true;
	}
};


/***********************************************************************************************
**	Tracers and status markers (D-001 P09).
**
**	Both are line strips submitted to the pooled particle batching, alpha-blended, modulated by
**	their vertex colour and drawn from both sides -- exactly the state each renderer built for
**	itself before this program existed, so moving onto the layer changes no picture.  No
**	material: the particle path carries colour per vertex.  Fog is per renderer and is still
**	switched on by the caller, because the fog name is the caller's own.
***********************************************************************************************/
class AlphaLineProgramClass : public FixedFunctionProgramClass
{
public:

	AlphaLineProgramClass(MaterialProgramType type) : FixedFunctionProgramClass(type,1,1) { }

	virtual bool	Init(ShaderTierType) override
	{
		ShaderClass shader = ShaderClass::_PresetAlphaShader;
		shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
		shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
		Set_Pass_Shader(0,shader);
		return true;
	}
};


/***********************************************************************************************
**	The Commander's ghost building (D-001 P09).
**
**	Alpha-blended with no depth write, the state CommanderGhostRenderer built before.  The
**	emissive tint and opacity change with the placement verdict, so the material stays the
**	caller's; this program owns the blend state it is drawn with.
***********************************************************************************************/
class GhostTintProgramClass : public FixedFunctionProgramClass
{
public:

	GhostTintProgramClass(void) : FixedFunctionProgramClass(MATERIAL_PROGRAM_GHOST_TINT,1,1) { }

	virtual bool	Init(ShaderTierType) override
	{
		ShaderClass shader = ShaderClass::_PresetAlphaShader;
		shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
		Set_Pass_Shader(0,shader);
		return true;
	}
};


/***********************************************************************************************
**	Water.
**
**	Translucent and two-sided, and no depth write, because a surface that blends over what is
**	beneath it must not stop the things beneath it from being drawn.  That is a different thing
**	from foliage's alpha-tested cutout: there is no edge to cut around here, only a surface.
***********************************************************************************************/
class WaterProgramClass : public FixedFunctionProgramClass
{
public:

	WaterProgramClass(void) : FixedFunctionProgramClass(MATERIAL_PROGRAM_WATER,1,1) { }

	virtual bool	Init(ShaderTierType) override
	{
		VertexMaterialClass * material = Make_Prelit_Diffuse_Material();
		Set_Pass_Material(0,material);
		REF_PTR_RELEASE(material);

		ShaderClass shader = ShaderClass::_PresetOpaqueShader;
		shader.Set_Alpha_Test(ShaderClass::ALPHATEST_DISABLE);
		shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
		shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_SRC_ALPHA);
		shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA);
		shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);

		Set_Pass_Shader(0,shader);
		return true;
	}
};


/***********************************************************************************************
**	Foliage.
**
**	Cut-out art: a leaf sheet is mostly transparent and both of its faces are seen, so alpha
**	tested and not culled.  Not culling is also what makes the far level's crossed quads work
**	from every side without emitting each of them twice.
***********************************************************************************************/
class FoliageProgramClass : public FixedFunctionProgramClass
{
public:

	FoliageProgramClass(void) : FixedFunctionProgramClass(MATERIAL_PROGRAM_FOLIAGE,1,1) { }

	virtual bool	Init(ShaderTierType) override
	{
		VertexMaterialClass * material = Make_Prelit_Diffuse_Material();
		Set_Pass_Material(0,material);
		REF_PTR_RELEASE(material);

		ShaderClass shader = ShaderClass::_PresetOpaqueShader;
		shader.Set_Alpha_Test(ShaderClass::ALPHATEST_ENABLE);
		shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);

		Set_Pass_Shader(0,shader);
		return true;
	}
};


/***********************************************************************************************
**	Projected shadows.
**
**	The pass that accumulates a caster into a shadow render target, not the pass that projects
**	the result onto the world.  Additive, untextured, depth ignored in both directions: what is
**	being built is coverage, and every triangle of the caster contributes to it wherever it
**	lands regardless of what else already did.
**
**	No material.  This pass never set one, so this program does not set one either -- see the
**	note in FixedFunctionProgramClass::Set_Pass.
***********************************************************************************************/
class ProjectedShadowProgramClass : public FixedFunctionProgramClass
{
public:

	ProjectedShadowProgramClass(void) : FixedFunctionProgramClass(MATERIAL_PROGRAM_PROJECTED_SHADOW,1,1) { }

	virtual bool	Init(ShaderTierType) override
	{
		ShaderClass shader;
		shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
		shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
		shader.Set_Depth_Compare(ShaderClass::PASS_ALWAYS);
		shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ONE);
		shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_ONE);
		shader.Set_Fog_Func(ShaderClass::FOG_DISABLE);
		shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
		shader.Set_Texturing(ShaderClass::TEXTURING_DISABLE);

		Set_Pass_Shader(0,shader);
		return true;
	}
};


/***********************************************************************************************
 * ShaderManagerClass::Detect_Tier -- ask the device what it can do                            *
 *                                                                                             *
 * There is no device on a dedicated server and none before the renderer comes up, and         *
 * fixed function is the right answer in both cases: it is what every program has to be        *
 * able to fall back to anyway.                                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
ShaderTierType	ShaderManagerClass::Detect_Tier(void)
{
	if (DX8Wrapper::_Get_D3D_Device8() == nullptr) {
		return SHADER_TIER_FIXED_FUNCTION;
	}

	const DX8Caps * caps = DX8Wrapper::Get_Current_Caps();
	if (caps == nullptr) {
		return SHADER_TIER_FIXED_FUNCTION;
	}

	//	Both halves have to be there.  A vertex shader with no pixel shader to receive
	//	its output is not a tier any program here wants to write against.
	if ((caps->Get_Vertex_Shader_Majon_Version() >= 1)
		&& (caps->Get_Pixel_Shader_Majon_Version() >= 1)) {
		return SHADER_TIER_PROGRAMMABLE;
	}

	return SHADER_TIER_FIXED_FUNCTION;
}


/***********************************************************************************************
 * ShaderManagerClass::Init -- detect the tier and build the programs                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Init(void)
{
	Init(Detect_Tier());
}


void	ShaderManagerClass::Init(ShaderTierType tier)
{
	WWASSERT(tier >= 0 && tier < SHADER_TIER_COUNT);

	if (Initialized) {
		Shutdown();
	}

	Tier = tier;
	Initialized = true;

	CurrentProgram = MATERIAL_PROGRAM_COUNT;
	CurrentPass = 0;

	//	Stock content first and always.
	Register_Program(MATERIAL_PROGRAM_LEGACY_W3D,new LegacyW3DProgramClass);

	//	The pipelines whose consumers exist.  They are built here, in one file, rather than by
	//	the systems that draw them: a pipeline is a description of device state in this
	//	library's own vocabulary, its lifetime is this class's, and having the six of them
	//	side by side is the whole point -- a road and a bridge that draw the same way say so by
	//	sitting next to each other, and water and foliage that differ show what they differ in.
	//	The dependency runs the way Section 15 asks: the systems know about the layer and the
	//	layer knows nothing about the systems.
	Register_Program(MATERIAL_PROGRAM_TERRAIN,new TerrainProgramClass);
	Register_Program(MATERIAL_PROGRAM_TERRAIN_DETAIL,new TerrainProgramClass(MATERIAL_PROGRAM_TERRAIN_DETAIL,true));
	Register_Program(MATERIAL_PROGRAM_ROAD,new OpaqueVertexLitProgramClass(MATERIAL_PROGRAM_ROAD));
	Register_Program(MATERIAL_PROGRAM_BRIDGE,new OpaqueVertexLitProgramClass(MATERIAL_PROGRAM_BRIDGE));
	Register_Program(MATERIAL_PROGRAM_WATER,new WaterProgramClass);
	Register_Program(MATERIAL_PROGRAM_FOLIAGE,new FoliageProgramClass);
	Register_Program(MATERIAL_PROGRAM_PROJECTED_SHADOW,new ProjectedShadowProgramClass);
	Register_Program(MATERIAL_PROGRAM_DEBUG_OVERLAY,new DebugOverlayProgramClass);

	//	D-001 P09: the three later renderers that draw with one fixed state.
	Register_Program(MATERIAL_PROGRAM_TRACER,new AlphaLineProgramClass(MATERIAL_PROGRAM_TRACER));
	Register_Program(MATERIAL_PROGRAM_STATUS_MARKER,new AlphaLineProgramClass(MATERIAL_PROGRAM_STATUS_MARKER));
	Register_Program(MATERIAL_PROGRAM_GHOST_TINT,new GhostTintProgramClass);

	//	Deliberately not registered:
	//	  - particles: every emitter carries the shader its W3D definition authored (additive,
	//	    alpha, screen...), so no single program can describe them; they keep the stock
	//	    per-emitter path, and MATERIAL_PROGRAM_PARTICLE is retired rather than faked.
	//	  - terrain detail: it needs a second UV rate on a single layer, which the terrain pass
	//	    cannot carry, and it has no layer table entry to draw.  A registered program is a
	//	    claim that a pipeline exists -- Is_Supported is how a caller asks.
}


/***********************************************************************************************
 * ShaderManagerClass::Shutdown -- give everything back                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Shutdown(void)
{
	Reset_Program();
	Reset_Textures();

	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {
		if (Programs[i] != nullptr) {
			Programs[i]->Shutdown();
			delete Programs[i];
			Programs[i] = nullptr;
		}
	}

	Tier = SHADER_TIER_FIXED_FUNCTION;
	Initialized = false;
}


bool	ShaderManagerClass::Is_Initialized(void)
{
	return Initialized;
}


ShaderTierType	ShaderManagerClass::Get_Tier(void)
{
	return Tier;
}


/***********************************************************************************************
 * ShaderManagerClass::Register_Program -- hand a program to the manager                       *
 *                                                                                             *
 * The manager owns the program from the moment it is offered, including when it declines      *
 * the tier.  A caller that had to remember to delete a program the manager refused would      *
 * be a caller that eventually forgets.                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool	ShaderManagerClass::Register_Program(MaterialProgramType type, MaterialProgramClass * program)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	if (program == nullptr) {
		return false;
	}

	//	Replacing a registered program is a mistake rather than an override: two systems
	//	claiming one pipeline is exactly the duplicate path this layer exists to prevent.
	WWASSERT(Programs[type] == nullptr);
	if (Programs[type] != nullptr) {
		delete program;
		return false;
	}

	if (!program->Init(Tier)) {
		delete program;
		return false;
	}

	Programs[type] = program;
	return true;
}


bool	ShaderManagerClass::Is_Supported(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);
	return (Programs[type] != nullptr);
}


MaterialProgramClass *	ShaderManagerClass::Peek_Program(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);
	return Programs[type];
}


const char *	ShaderManagerClass::Get_Program_Name(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);
	return _ProgramNames[type];
}


/***********************************************************************************************
 * ShaderManagerClass::Get_Pass_Count -- how many submissions a program needs                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int	ShaderManagerClass::Get_Pass_Count(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	if (Programs[type] == nullptr) {
		return 0;
	}

	return Programs[type]->Get_Pass_Count();
}


/***********************************************************************************************
 * ShaderManagerClass::Set_Program -- make one pass of one program current                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Set_Program(MaterialProgramType type, int pass)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	MaterialProgramClass * program = Programs[type];
	if (program == nullptr) {
		//	Asking for a pipeline the build does not have is a caller mistake, but it must
		//	not leave the previous program current and half-owned.
		WWASSERT(0);
		Reset_Program();
		return ;
	}

	WWASSERT(pass >= 0 && pass < program->Get_Pass_Count());

	//	Anything that was current gives the device back first, so state never leaks from
	//	one pipeline into the next.  Passes within one program do not: a program setting
	//	up its own second pass knows what its first one did.
	if ((CurrentProgram != type) && (CurrentProgram != MATERIAL_PROGRAM_COUNT)) {
		Reset_Program();
	}

	CurrentProgram = type;
	CurrentPass = pass;

	program->Set_Pass(pass);
}


/***********************************************************************************************
 * ShaderManagerClass::Reset_Program -- give the device back                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Reset_Program(void)
{
	if (CurrentProgram == MATERIAL_PROGRAM_COUNT) {
		return ;
	}

	MaterialProgramClass * program = Programs[CurrentProgram];

	CurrentProgram = MATERIAL_PROGRAM_COUNT;
	CurrentPass = 0;

	if (program != nullptr) {
		program->Reset();
	}
}


MaterialProgramType	ShaderManagerClass::Get_Current_Program(void)
{
	return CurrentProgram;
}


int	ShaderManagerClass::Get_Current_Pass(void)
{
	return CurrentPass;
}


/***********************************************************************************************
 * ShaderManagerClass::Get_Pass_State -- what state a pipeline draws in                        *
 *                                                                                             *
 * Brings the manager up if it is not up, which is what makes this usable from a dedicated     *
 * server: the FDS builds the same road, bridge, water and foliage geometry a client does --    *
 * it is the same code and the same collision -- and it never reaches the device init that      *
 * normally starts this class.  Nothing here is a device resource, and Detect_Tier answers      *
 * fixed function with no device, so coming up early is correct rather than merely safe.        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool	ShaderManagerClass::Get_Pass_State(MaterialProgramType type,int pass,ShaderClass * shader_out,VertexMaterialClass ** material_out)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	if (material_out != nullptr) {
		*material_out = nullptr;
	}

	if (!Initialized) {
		Init();
	}

	MaterialProgramClass * program = Programs[type];
	if (program == nullptr) {
		return false;
	}

	return program->Get_Pass_State(pass,shader_out,material_out);
}


/***********************************************************************************************
 * ShaderManagerClass::Set_Program_Variant -- choose which way a pipeline draws                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Set_Program_Variant(MaterialProgramType type,int variant)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	MaterialProgramClass * program = Programs[type];
	if (program != nullptr) {
		program->Set_Variant(variant);
	}
}


/***********************************************************************************************
 * ShaderManagerClass::Set_Texture -- stage a texture for the programs to read                 *
 *                                                                                             *
 * A reference is held for as long as the stage holds the texture.  The donor stored raw       *
 * pointers here and relied on the caller outliving the draw, which is true right up until      *
 * a level release happens between staging and drawing.                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Set_Texture(int stage, TextureClass * texture)
{
	WWASSERT(stage >= 0 && stage < MAX_TEXTURE_STAGES);

	REF_PTR_SET(Textures[stage],texture);
}


TextureClass *	ShaderManagerClass::Peek_Texture(int stage)
{
	WWASSERT(stage >= 0 && stage < MAX_TEXTURE_STAGES);
	return Textures[stage];
}


void	ShaderManagerClass::Reset_Textures(void)
{
	for (int i=0; i<MAX_TEXTURE_STAGES; i++) {
		REF_PTR_RELEASE(Textures[i]);
	}
}
