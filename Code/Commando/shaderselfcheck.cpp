//
//	The shader and state management layer, checked from inside the binary that has to be right.
//	See shaderselfcheck.h for why the checks live here.
//

#include "shaderselfcheck.h"

#include "backgroundmgr.h"
#include "building.h"
#include "buildingdescriptormanager.h"
#include "combat.h"
#include "definitionmgr.h"
#include "commandermodecontroller.h"
#include "commandersidebar.h"
#include "commanderplacement.h"
#include "commanderproduction.h"
#include "buildingconstruction.h"
#include "rtspersistence.h"
#include "obbox.h"
#include "aabox.h"
#include "compositeattachment.h"
#include "debrisrenderer.h"
#include "debugrendersystem.h"
#include "teampowergridservice.h"
#include "guardstatecontroller.h"
#include "radar.h"
#include "rtstelemetryservice.h"
#include "shadermgr.h"
#include "statusoverlayrenderer.h"
#include "statusoverlaysystem.h"
#include "tacticalintelligence.h"
#include "tracerrenderer.h"
#include "weatherenvironment.h"
#include "worldparticlemanager.h"
#include "worldshroudmanager.h"

#include "assetmgr.h"
#include "camera.h"
#include "light.h"
#include "lightphys.h"
#include "lightsolvecontext.h"
#include "postprocess.h"
#include "dx8wrapper.h"
#include "foliagesystem.h"
#include "WeatherMgr.h"
#include "terraintexturesystem.h"
#include "worldsurfacemarkmanager.h"
#include "surfacetinteffect.h"
#include "camerashakesystem.h"
#include "worldmetrics.h"
#include "worldtraversability.h"
#include "escarpment.h"
#include "bridgesystem.h"
#include "bridgesection.h"

#include "rendobj.h"
#include "rinfo.h"
#include "vector3.h"
#include "vector4.h"
#include "worldlightmanager.h"
#include "ww3d.h"

#include "refcount.h"
#include "shader.h"
#include "vertmaterial.h"
#include "wwmath.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

int	_Failures = 0;

void	Check (bool condition, const char *format, ...)
{
	if (condition) {
		return ;
	}

	va_list args;
	va_start (args, format);
	::fprintf (stderr, "FAIL: ");
	::vfprintf (stderr, format, args);
	::fprintf (stderr, "\n");
	va_end (args);

	_Failures ++;
	return ;
}


/*
**	A program that reports what was done to it.
**
**	Counting is the only way to answer the questions that matter here.  "Was the previous
**	program reset" is not the same as "is some program reset", and "reset once" is not the
**	same as "reset", which is what a layer that leaks device state looks like from outside.
*/
class CountingProgramClass : public MaterialProgramClass
{
public:
	CountingProgramClass (const char *name, int passes, ShaderTierType minimum)
		:	Name (name),
			Passes (passes),
			Minimum (minimum)
	{
		Live ++;
	}

	virtual ~CountingProgramClass (void) override		{ Live --; }

	virtual const char *	Get_Name (void) const override	{ return Name; }
	virtual int				Get_Pass_Count (void) const override	{ return Passes; }

	virtual bool	Init (ShaderTierType tier) override
	{
		Inits ++;
		return (tier >= Minimum);
	}

	virtual void	Shutdown (void) override		{ Shutdowns ++; }
	virtual void	Set_Pass (int pass) override	{ Sets ++; LastPass = pass; }
	virtual void	Reset (void) override			{ Resets ++; }

	const char *	Name;
	int				Passes;
	ShaderTierType	Minimum;

	int				Inits = 0;
	int				Shutdowns = 0;
	int				Sets = 0;
	int				Resets = 0;
	int				LastPass = -1;

	//	How many of these exist.  A program the manager refuses has to be destroyed by the
	//	manager, and the only way to see that from here is to watch the population.
	static int		Live;
};

int	CountingProgramClass::Live = 0;


/***********************************************************************************************
**	programs -- registration, tiers, passes, and who owns the device state
***********************************************************************************************/

int	Run_Programs (void)
{
	//
	//	There is no device in this process, and that is the answer a dedicated server gets
	//	too.  Fixed function is what every program has to be able to fall back to, so it is
	//	the safe reading as well as the true one.
	//
	Check (ShaderManagerClass::Detect_Tier () == SHADER_TIER_FIXED_FUNCTION,
			"a process with no device detected a programmable tier");

	//
	//	Every program has a name, and no two share one.  A missing entry in the name table
	//	shows up here rather than in a log six months from now.
	//
	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {

		const char *name = ShaderManagerClass::Get_Program_Name ((MaterialProgramType)i);
		Check (name != nullptr && name[0] != 0, "program %d has no name", i);

		for (int j=i+1; j<MATERIAL_PROGRAM_COUNT; j++) {
			const char *other = ShaderManagerClass::Get_Program_Name ((MaterialProgramType)j);
			Check (::strcmp (name, other) != 0, "programs %d and %d are both called %s", i, j, name);
		}
	}

	ShaderManagerClass::Init (SHADER_TIER_FIXED_FUNCTION);

	Check (ShaderManagerClass::Is_Initialized (), "the manager did not come up");
	Check (ShaderManagerClass::Get_Tier () == SHADER_TIER_FIXED_FUNCTION,
			"the manager ignored the tier it was given");

	//
	//	Stock W3D content is a program, not an exception.  One pass, always present, and
	//	it is what a Renegade model has always been drawn with.
	//
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_LEGACY_W3D),
			"stock W3D materials have no program");
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_LEGACY_W3D) == 1,
			"stock W3D materials want %d passes, not 1",
			ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_LEGACY_W3D));

	//
	//	Debug overlays build a material in Init, which is why registering them here -- in a
	//	process with no device -- is worth checking rather than assuming.
	//
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_DEBUG_OVERLAY),
			"debug overlays have no program");
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_DEBUG_OVERLAY) == 1,
			"debug overlays want %d passes, not 1",
			ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_DEBUG_OVERLAY));

	//
	//	Exactly the pipelines whose consumers exist are registered, and no others.  Both
	//	halves matter: a pipeline missing means a system draws in whatever state it inherits,
	//	and a pipeline registered early is a claim that something draws it, which Is_Supported
	//	is how a caller asks.  An unregistered one reports no passes so that a caller looping
	//	over them draws nothing rather than drawing it wrongly.
	//
	static const MaterialProgramType	_expected[] = {
		MATERIAL_PROGRAM_LEGACY_W3D,
		MATERIAL_PROGRAM_TERRAIN,
		MATERIAL_PROGRAM_ROAD,
		MATERIAL_PROGRAM_BRIDGE,
		MATERIAL_PROGRAM_WATER,
		MATERIAL_PROGRAM_FOLIAGE,
		MATERIAL_PROGRAM_PROJECTED_SHADOW,
		MATERIAL_PROGRAM_DEBUG_OVERLAY,
	};

	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {

		bool expected = false;
		for (int e=0; e<(int)(sizeof(_expected)/sizeof(_expected[0])); e++) {
			if (_expected[e] == i) { expected = true; break; }
		}

		const char *name = ShaderManagerClass::Get_Program_Name ((MaterialProgramType)i);

		if (expected) {
			Check (ShaderManagerClass::Is_Supported ((MaterialProgramType)i),
					"%s has a consumer and no program", name);
			Check (ShaderManagerClass::Get_Pass_Count ((MaterialProgramType)i) >= 1,
					"%s is registered and reports no passes", name);
		} else {
			Check (ShaderManagerClass::Is_Supported ((MaterialProgramType)i) == false,
					"%s is registered and nothing draws it yet", name);
			Check (ShaderManagerClass::Get_Pass_Count ((MaterialProgramType)i) == 0,
					"%s reports passes without being registered", name);
		}
	}

	//
	//	A program that wants hardware this tier does not have declines, and declining is a
	//	normal answer: it is not registered, and the manager destroys it rather than leaving
	//	the caller holding something it was told to hand over.  Offered into a slot nothing
	//	claims, because registering over a registered pipeline is a caller mistake the manager
	//	asserts on rather than a case worth proving.
	//
	const int	live_before = CountingProgramClass::Live;

	CountingProgramClass *	wants_shaders = new CountingProgramClass ("wants_shaders", 1, SHADER_TIER_PROGRAMMABLE);
	const bool					kept = ShaderManagerClass::Register_Program (MATERIAL_PROGRAM_PARTICLE, wants_shaders);

	Check (kept == false, "a program that declined the tier was registered anyway");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_PARTICLE) == false,
			"a declined program is still supported");
	Check (CountingProgramClass::Live == live_before,
			"a declined program was not destroyed: %d live, expected %d",
			CountingProgramClass::Live, live_before);

	//
	//	Two programs that do fit, so that the handover between them can be watched.
	//
	CountingProgramClass *	ghost = new CountingProgramClass ("ghost", 1, SHADER_TIER_FIXED_FUNCTION);
	CountingProgramClass *	overlay = new CountingProgramClass ("marker", 2, SHADER_TIER_FIXED_FUNCTION);

	Check (ShaderManagerClass::Register_Program (MATERIAL_PROGRAM_GHOST_TINT, ghost),
			"a program that fits the tier was refused");
	Check (ShaderManagerClass::Register_Program (MATERIAL_PROGRAM_STATUS_MARKER, overlay),
			"a multipass program that fits the tier was refused");

	Check (ghost->Inits == 1, "the manager called Init %d times, not once", ghost->Inits);
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_STATUS_MARKER) == 2,
			"a two pass program reported %d passes",
			ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_STATUS_MARKER));

	//
	//	Nothing is current until something is set, and resetting nothing is harmless.
	//
	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_COUNT,
			"something was current before anything was set");
	ShaderManagerClass::Reset_Program ();
	Check (ghost->Resets == 0, "resetting nothing reset a program");

	ShaderManagerClass::Set_Program (MATERIAL_PROGRAM_GHOST_TINT, 0);

	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_GHOST_TINT,
			"the program that was set is not the current one");
	Check (ShaderManagerClass::Get_Current_Pass () == 0, "the current pass is not the one that was set");
	Check (ghost->Sets == 1, "Set_Pass ran %d times for one pass", ghost->Sets);
	Check (ghost->Resets == 0, "setting a program reset it");

	//
	//	Moving between passes of one program does not reset it.  A program setting up its
	//	own second pass knows what its first one did, and making it start from nothing
	//	every time would be the layer getting in the way of the pipeline it exists to serve.
	//
	ShaderManagerClass::Set_Program (MATERIAL_PROGRAM_STATUS_MARKER, 0);
	ShaderManagerClass::Set_Program (MATERIAL_PROGRAM_STATUS_MARKER, 1);

	Check (ghost->Resets == 1, "handing over to another program reset the old one %d times, not once",
			ghost->Resets);
	Check (overlay->Sets == 2, "two passes ran Set_Pass %d times", overlay->Sets);
	Check (overlay->Resets == 0, "moving between passes of one program reset it");
	Check (overlay->LastPass == 1, "the second pass arrived as pass %d", overlay->LastPass);
	Check (ShaderManagerClass::Get_Current_Pass () == 1, "the current pass is not the second one");

	//
	//	And giving the device back resets exactly once, however often it is asked.
	//
	ShaderManagerClass::Reset_Program ();
	ShaderManagerClass::Reset_Program ();

	Check (overlay->Resets == 1, "giving the device back reset %d times, not once", overlay->Resets);
	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_COUNT,
			"a program is still current after the device was given back");

	//
	//	Staged textures survive being staged and are given back on request.  Nothing here
	//	holds a real texture -- that needs a device -- but the stage bookkeeping does not.
	//
	Check (ShaderManagerClass::Peek_Texture (0) == nullptr, "a stage held a texture before anything staged one");
	ShaderManagerClass::Set_Texture (0, nullptr);
	Check (ShaderManagerClass::Peek_Texture (0) == nullptr, "staging nothing produced something");
	ShaderManagerClass::Reset_Textures ();

	//
	//	Bringing the manager up again tears down what was there.  Every registered program
	//	is shut down and destroyed, and what comes back is a fresh registry holding stock
	//	W3D content and nothing else.
	//
	ShaderManagerClass::Init (SHADER_TIER_PROGRAMMABLE);

	Check (CountingProgramClass::Live == live_before,
			"re-initialising left %d programs alive, expected %d",
			CountingProgramClass::Live, live_before);
	Check (ShaderManagerClass::Get_Tier () == SHADER_TIER_PROGRAMMABLE,
			"re-initialising ignored the new tier");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_LEGACY_W3D),
			"stock W3D materials lost their program on re-initialisation");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_DEBUG_OVERLAY),
			"debug overlays lost their program on re-initialisation");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_GHOST_TINT) == false,
			"a program survived re-initialisation");
	Check (ShaderManagerClass::Get_Current_Program () == MATERIAL_PROGRAM_COUNT,
			"something was current after re-initialisation");

	ShaderManagerClass::Shutdown ();

	Check (ShaderManagerClass::Is_Initialized () == false, "the manager did not go down");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_LEGACY_W3D) == false,
			"a program outlived the manager");
	Check (ShaderManagerClass::Is_Supported (MATERIAL_PROGRAM_DEBUG_OVERLAY) == false,
			"the debug overlay program outlived the manager");

	return _Failures;
}


/***********************************************************************************************
**	pipelines -- what each registered pipeline actually draws in
**
**	The pipelines are worth pinning rather than merely counting.  The whole claim of this layer
**	is that "how is a road surface drawn" has one answer; a check that only asked whether the
**	road program existed would pass just as happily if that answer quietly became foliage's.
***********************************************************************************************/

int	Run_Pipelines (void)
{
	//
	//	The manager is down when this starts, which is the state a dedicated server is in: it
	//	builds the same road, bridge, water and foliage geometry a client does and never
	//	reaches the device init that normally starts this class.  Asking for pipeline state
	//	must therefore bring it up, because nothing here is a device resource.
	//
	ShaderManagerClass::Shutdown ();
	Check (ShaderManagerClass::Is_Initialized () == false, "the manager did not go down before this check");

	ShaderClass				shader;
	VertexMaterialClass *	material = nullptr;

	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 0, &shader, &material),
			"a pipeline could not be asked for its state with the manager down");
	Check (ShaderManagerClass::Is_Initialized (),
			"asking a pipeline for its state did not bring the manager up");
	Check (material != nullptr, "the road pipeline has no material");
	REF_PTR_RELEASE (material);

	//
	//	The material comes back referenced.  A pipeline that handed out a borrowed pointer
	//	would be handing out something a re-initialisation can delete underneath a mesh that
	//	is still drawing with it.
	//
	VertexMaterialClass *	first = nullptr;
	VertexMaterialClass *	second = nullptr;

	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 0, nullptr, &first);
	Check (first != nullptr, "the road pipeline stopped having a material");

	if (first != nullptr) {
		const int	refs_before = first->Num_Refs ();
		ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 0, nullptr, &second);
		Check (second == first, "the road pipeline handed out two different materials");
		Check (first->Num_Refs () == refs_before + 1,
				"asking for pipeline state took %d references, not one",
				first->Num_Refs () - refs_before);
	}

	REF_PTR_RELEASE (second);
	REF_PTR_RELEASE (first);

	//
	//	An unregistered pipeline answers no, and says so without leaving a material behind for
	//	the caller to release.  A pass that does not exist answers no as well.
	//
	material = nullptr;
	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_GHOST_TINT, 0, &shader, &material) == false,
			"an unregistered pipeline answered with state");
	Check (material == nullptr, "an unregistered pipeline handed out a material");

	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, 1, &shader, &material) == false,
			"a one pass pipeline answered for a second pass");
	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_ROAD, -1, &shader, &material) == false,
			"a pipeline answered for a negative pass");

	//
	//	Terrain is two passes and they differ in exactly one thing.  That is how stock Renegade
	//	composites ground and it is why the two exist: the first layer with any influence on a
	//	quad draws opaque and the rest blend over it.  Both are fogged, because a hillside that
	//	stopped being fogged where its second layer started would be a visible seam.
	//
	Check (ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_TERRAIN) == 2,
			"terrain wants %d passes, not 2", ShaderManagerClass::Get_Pass_Count (MATERIAL_PROGRAM_TERRAIN));

	ShaderClass	base_shader, layer_shader;
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, &base_shader, nullptr);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_LAYER, &layer_shader, nullptr);

	Check (base_shader.Get_Fog_Func () == ShaderClass::FOG_ENABLE, "the terrain base pass is not fogged");
	Check (layer_shader.Get_Fog_Func () == ShaderClass::FOG_ENABLE, "the terrain layer pass is not fogged");
	Check (base_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ZERO,
			"the terrain base pass blends instead of drawing opaque");
	Check (layer_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA,
			"the terrain layer pass does not blend over what is under it");

	//
	//	And the base pass has two materials, because whether the ground's colour comes from the
	//	light environment or from a baked vertex is a fact about the patch and not about
	//	terrain.  The layer pass always takes it from the vertex -- that is where its blend
	//	weight lives -- so the vertex-lit base is the same material the layer uses.
	//
	VertexMaterialClass *	lit_base = nullptr;
	VertexMaterialClass *	vertex_base = nullptr;
	VertexMaterialClass *	layer = nullptr;

	ShaderManagerClass::Set_Program_Variant (MATERIAL_PROGRAM_TERRAIN, TERRAIN_VARIANT_LIT);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, nullptr, &lit_base);

	ShaderManagerClass::Set_Program_Variant (MATERIAL_PROGRAM_TERRAIN, TERRAIN_VARIANT_VERTEX_LIT);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, nullptr, &vertex_base);

	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_LAYER, nullptr, &layer);

	Check (lit_base != nullptr && vertex_base != nullptr && layer != nullptr,
			"a terrain pass has no material");
	Check (lit_base != vertex_base, "the terrain variant switch changed nothing");
	Check (vertex_base == layer, "the vertex-lit base is not the material the layer draws with");

	//	And back, so a later caller does not inherit this check's choice.
	ShaderManagerClass::Set_Program_Variant (MATERIAL_PROGRAM_TERRAIN, TERRAIN_VARIANT_LIT);

	VertexMaterialClass *	restored = nullptr;
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_TERRAIN, TERRAIN_PASS_BASE, nullptr, &restored);
	Check (restored == lit_base, "the terrain variant did not switch back");
	REF_PTR_RELEASE (restored);

	REF_PTR_RELEASE (layer);
	REF_PTR_RELEASE (vertex_base);
	REF_PTR_RELEASE (lit_base);

	//
	//	Water and foliage are the pair the design contrasts, and they must not converge.  Water
	//	is a translucent surface that blends over what is beneath it and must not write depth,
	//	or it stops the things beneath it being drawn.  Foliage is cut-out art: it has an edge
	//	to cut around rather than a surface to blend, so it alpha tests and does write depth.
	//	Both are two-sided, for different reasons -- a lake seen from below, a leaf sheet seen
	//	from behind.
	//
	ShaderClass	water_shader, foliage_shader;
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_WATER, 0, &water_shader, nullptr);
	ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_FOLIAGE, 0, &foliage_shader, nullptr);

	Check (water_shader.Get_Alpha_Test () == ShaderClass::ALPHATEST_DISABLE, "water alpha tests");
	Check (water_shader.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_DISABLE, "water writes depth");
	Check (water_shader.Get_Src_Blend_Func () == ShaderClass::SRCBLEND_SRC_ALPHA,
			"water does not blend by its own alpha");
	Check (water_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA,
			"water does not blend over what is beneath it");
	Check (water_shader.Get_Cull_Mode () == ShaderClass::CULL_MODE_DISABLE, "water is one-sided");

	Check (foliage_shader.Get_Alpha_Test () == ShaderClass::ALPHATEST_ENABLE, "foliage does not alpha test");
	Check (foliage_shader.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_ENABLE, "foliage does not write depth");
	Check (foliage_shader.Get_Cull_Mode () == ShaderClass::CULL_MODE_DISABLE, "foliage is one-sided");

	//
	//	Roads and bridges are solid ground with the light baked in.  Nothing blends, nothing is
	//	cut around, and both write depth, because a vehicle behind a bridge pier is behind it.
	//
	const MaterialProgramType	_solid[] = { MATERIAL_PROGRAM_ROAD, MATERIAL_PROGRAM_BRIDGE };

	for (int i=0; i<2; i++) {

		ShaderClass	solid;
		VertexMaterialClass *	solid_material = nullptr;
		const char *name = ShaderManagerClass::Get_Program_Name (_solid[i]);

		Check (ShaderManagerClass::Get_Pass_State (_solid[i], 0, &solid, &solid_material),
				"%s has no pass state", name);
		Check (solid.Get_Alpha_Test () == ShaderClass::ALPHATEST_DISABLE, "%s alpha tests", name);
		Check (solid.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_ENABLE, "%s does not write depth", name);
		Check (solid.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ZERO, "%s blends", name);
		Check (solid_material != nullptr, "%s has no material", name);

		REF_PTR_RELEASE (solid_material);
	}

	//
	//	The projected shadow accumulation pass builds coverage, not a picture: every triangle
	//	of a caster contributes wherever it lands, regardless of what already did, so it is
	//	additive with depth ignored in both directions and no texture.  It sets no material,
	//	which is a real answer and not an oversight -- this pass never set one, and inventing
	//	one here would be the layer changing a picture rather than owning the state behind it.
	//
	ShaderClass	shadow_shader;
	VertexMaterialClass *	shadow_material = nullptr;

	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_PROJECTED_SHADOW, 0, &shadow_shader, &shadow_material),
			"the projected shadow pipeline has no pass state");
	Check (shadow_material == nullptr, "the projected shadow pass acquired a material");
	Check (shadow_shader.Get_Src_Blend_Func () == ShaderClass::SRCBLEND_ONE, "shadow coverage is not additive");
	Check (shadow_shader.Get_Dst_Blend_Func () == ShaderClass::DSTBLEND_ONE, "shadow coverage is not additive");
	Check (shadow_shader.Get_Depth_Mask () == ShaderClass::DEPTH_WRITE_DISABLE, "shadow coverage writes depth");
	Check (shadow_shader.Get_Depth_Compare () == ShaderClass::PASS_ALWAYS, "shadow coverage tests depth");
	Check (shadow_shader.Get_Fog_Func () == ShaderClass::FOG_DISABLE, "shadow coverage is fogged");
	Check (shadow_shader.Get_Texturing () == ShaderClass::TEXTURING_DISABLE, "shadow coverage is textured");
	Check (shadow_shader.Get_Cull_Mode () == ShaderClass::CULL_MODE_DISABLE, "shadow coverage is one-sided");

	//
	//	Stock W3D content is the one registered program with nothing to hand out.  A Renegade
	//	material carries its own shader and its own vertex material and the renderer applies
	//	them as it draws, so a pipeline state for it would be this layer inventing an opinion
	//	about a model exported in 2002.
	//
	material = nullptr;
	Check (ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_LEGACY_W3D, 0, &shader, &material) == false,
			"stock W3D content was given a pipeline state of its own");
	Check (material == nullptr, "stock W3D content handed out a material");

	//
	//	Every pipeline answers for every pass it claims, and for none beyond them.
	//
	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {

		const MaterialProgramType	type = (MaterialProgramType)i;
		const int						passes = ShaderManagerClass::Get_Pass_Count (type);

		if ((passes == 0) || (type == MATERIAL_PROGRAM_LEGACY_W3D) || (type == MATERIAL_PROGRAM_DEBUG_OVERLAY)) {
			continue;
		}

		for (int pass=0; pass<passes; pass++) {
			VertexMaterialClass *	any = nullptr;
			Check (ShaderManagerClass::Get_Pass_State (type, pass, nullptr, &any),
					"%s could not answer for pass %d of %d",
					ShaderManagerClass::Get_Program_Name (type), pass, passes);
			REF_PTR_RELEASE (any);
		}

		Check (ShaderManagerClass::Get_Pass_State (type, passes, nullptr, nullptr) == false,
				"%s answered for one pass more than it claims",
				ShaderManagerClass::Get_Program_Name (type));
	}

	ShaderManagerClass::Shutdown ();
	return _Failures;
}



/***********************************************************************************************
**	particles -- the shared pools, the batch key and the budget
**
**	Section 26's acceptance is a claim about numbers -- that a large firefight does not explode
**	draw calls or allocations -- and numbers are what this checks.  It never draws anything: a
**	flush with no device does every part of its work except the draw, so the batch a submission
**	lands in, the table its particles are gathered through and the budget that paid for them are
**	all decided and counted here exactly as they would be on a screen.
***********************************************************************************************/

namespace {

enum { PARTICLE_CHECK_MAX = 4096 };

/*
**	Fixed rather than allocated, so the check owns no heap across its own exit -- a self check
**	that leaks is a self check that reports whatever the leak tracker decides to do about it
**	rather than what it was asked.
*/
Vector3			_CheckPositions[PARTICLE_CHECK_MAX];
Vector4			_CheckDiffuse[PARTICLE_CHECK_MAX];
bool				_CheckArraysBuilt = false;

void	Build_Check_Arrays (void)
{
	if (_CheckArraysBuilt) {
		return ;
	}

	for (int i = 0; i < PARTICLE_CHECK_MAX; i++) {
		_CheckPositions[i].Set ((float)i, 0.0f, 0.0f);
		_CheckDiffuse[i].Set (0.5f, 0.5f, 0.5f, 1.0f);
	}

	_CheckArraysBuilt = true;
	return ;
}

/*
**	One emitter's worth of particles, described the way a ParticleBufferClass with no property
**	arrays describes itself -- which is the common case in stock content and the case where the
**	batch key has to work hardest.
*/
ParticleSubmissionStruct	Make_Submission (int count, int budget, float distance, bool with_diffuse)
{
	ParticleSubmissionStruct	submission;

	submission.Texture = nullptr;
	submission.PointMode = 1;					// PointGroupClass::QUADS
	submission.LineMode = 0;					// LineGroupClass::TETRAHEDRON
	submission.Position = _CheckPositions;
	submission.TailPosition = _CheckPositions;
	submission.Diffuse = with_diffuse ? _CheckDiffuse : nullptr;
	submission.Count = count;
	submission.Budget = budget;
	submission.Center.Set (distance, 0.0f, 0.0f);

	return submission;
}

}	// namespace


int	Run_Particles (void)
{
	Build_Check_Arrays ();

	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	WorldParticleManager::Init ();
	WorldParticleManager::Set_Enabled (true);
	WorldParticleManager::Set_Pool_Capacity (PARTICLE_POOL_SPRITE, 1024);
	WorldParticleManager::Set_Pool_Capacity (PARTICLE_POOL_STREAK, 256);

	int	capacity = WorldParticleManager::Get_Pool_Capacity (PARTICLE_POOL_SPRITE);
	Check (capacity == 1024, "the sprite pool was sized to %d and not 1024", capacity);

	/*
	**	The acceptance, as a number.  Sixty-four emitters that share a material are sixty-four
	**	draw calls in the stock engine and one here, and the ratio is the point: what a batch
	**	counts is materials, not effects.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 64; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (8, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		Check (WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission),
				"the pool refused submission %d of a material it had already taken", i);
	}

	Check (WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE) == 1,
			"sixty-four emitters sharing one material opened %d batches",
			WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE) == 64,
			"%d emitters submitted, not 64",
			WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE) == 1,
			"sixty-four emitters drew as %d batches, not one",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Drawn_Particles (PARTICLE_POOL_SPRITE) == 512,
			"%d of 512 offered particles were drawn",
			WorldParticleManager::Get_Drawn_Particles (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Thinned_Particles (PARTICLE_POOL_SPRITE) == 0,
			"a submission well inside the pool lost %d particles",
			WorldParticleManager::Get_Thinned_Particles (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE) == 0,
			"the flush left %d particles staged",
			WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE));

	/*
	**	A shader difference is a state change, so it is a second batch.  If this ever merged,
	**	one of the two groups of particles would be drawn in the other's blend mode.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 32; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (4, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		submission.Shader = ShaderClass ((unsigned int)(i & 3));
		Check (WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission),
				"the pool refused a submission with shader bits %d", i & 3);
	}

	Check (WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE) == 4,
			"thirty-two emitters using four shaders opened %d batches",
			WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE) == 4,
			"four shaders drew as %d batches",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE));

	/*
	**	The other half of the acceptance: the pool is the budget.  Offer it four times what it
	**	can hold and it holds exactly what it holds, and every particle is accounted for either
	**	as drawn or as thinned -- a pool that quietly dropped some would still look bounded.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 64; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (64, PARTICLE_BUDGET_AMBIENT, 0.0f, false);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission);
	}

	unsigned int	submitted = WorldParticleManager::Get_Submitted_Particles (PARTICLE_POOL_SPRITE);
	unsigned int	drawn = WorldParticleManager::Get_Drawn_Particles (PARTICLE_POOL_SPRITE);
	unsigned int	thinned = WorldParticleManager::Get_Thinned_Particles (PARTICLE_POOL_SPRITE);

	Check (submitted == 4096, "%u particles were offered, not 4096", submitted);
	Check (drawn <= (unsigned int)capacity, "%u particles were staged in a pool of %d", drawn, capacity);
	Check (drawn + thinned == submitted,
			"%u drawn plus %u thinned is not the %u offered", drawn, thinned, submitted);
	Check (WorldParticleManager::Get_Pool_Capacity (PARTICLE_POOL_SPRITE) == capacity,
			"the pool grew to %d under four times its capacity",
			WorldParticleManager::Get_Pool_Capacity (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);

	/*
	**	That flush saw ambient demand of four times the pool, so the next accumulation runs under
	**	caps.  Critical gameplay asks for more than its reserve and must still get its reserve --
	**	the whole reason a class that is never the loudest is worth having.
	*/
	{
		ParticleSubmissionStruct	ambient = Make_Submission (4096, PARTICLE_BUDGET_AMBIENT, 0.0f, false);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, ambient);

		int	after_ambient = WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE);
		Check (after_ambient < capacity,
				"a capped ambient submission took %d of a %d pool", after_ambient, capacity);

		ParticleSubmissionStruct	critical = Make_Submission (512, PARTICLE_BUDGET_CRITICAL_GAMEPLAY, 0.0f, false);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, critical);

		int	after_critical = WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE) - after_ambient;
		int	reserve = capacity / 4;			// the fixed critical share

		Check (after_critical >= reserve,
				"ambient at four times the pool left critical gameplay only %d of its %d reserve",
				after_critical, reserve);
	}

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Reset_Statistics ();

	/*
	**	A submission that names no budget class is placed by distance.  Same effect, same
	**	everything, twice -- once under the camera's nose and once across the map.
	*/
	WorldParticleManager::Set_Camera_Position (Vector3 (0.0f, 0.0f, 0.0f));
	WorldParticleManager::Set_Combat_Near_Distance (50.0f);

	{
		ParticleSubmissionStruct	close = Make_Submission (16, PARTICLE_BUDGET_AUTO, 10.0f, false);
		ParticleSubmissionStruct	distant = Make_Submission (32, PARTICLE_BUDGET_AUTO, 500.0f, false);

		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, close);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, distant);
	}

	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_NEAR) == 16,
			"an effect ten units away was charged %u particles to COMBAT_NEAR",
			WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_NEAR));
	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_FAR) == 32,
			"an effect five hundred units away was charged %u particles to COMBAT_FAR",
			WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_FAR));

	WorldParticleManager::Flush (rinfo);

	/*
	**	The pools are separate.  A streak is not a sprite and must not be charged to one, or a
	**	scene full of tracer trails would silently ration the smoke.
	*/
	WorldParticleManager::Reset_Statistics ();

	{
		ParticleSubmissionStruct	streak = Make_Submission (64, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, true);
		Check (WorldParticleManager::Submit (PARTICLE_POOL_STREAK, streak), "the streak pool refused a streak");
	}

	Check (WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_STREAK) == 64,
			"the streak pool staged %d of 64 streaks",
			WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_STREAK));
	Check (WorldParticleManager::Get_Pending_Particles (PARTICLE_POOL_SPRITE) == 0,
			"a streak was charged to the sprite pool");

	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_STREAK) == 1,
			"one streak emitter drew as %d batches",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_STREAK));
	Check (WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE) == 0,
			"the sprite pool drew %d batches with nothing in it",
			WorldParticleManager::Get_Batch_Count (PARTICLE_POOL_SPRITE));

	/*
	**	Disabled has to mean the caller draws itself, because that is the only thing that makes
	**	"turn it off and you have the stock image" a statement about code rather than a hope.
	*/
	WorldParticleManager::Set_Enabled (false);

	{
		ParticleSubmissionStruct	submission = Make_Submission (8, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		Check (WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission) == false,
				"a disabled manager took a submission the emitter then would not draw");
	}

	WorldParticleManager::Set_Enabled (true);

	/*
	**	And the table of batches is bounded like everything else: a pool asked for more distinct
	**	materials than it holds refuses the extras rather than growing or overrunning.
	*/
	WorldParticleManager::Reset_Statistics ();

	for (int i = 0; i < 256; i++) {
		ParticleSubmissionStruct	submission = Make_Submission (1, PARTICLE_BUDGET_COMBAT_NEAR, 0.0f, false);
		submission.Shader = ShaderClass ((unsigned int)i);
		WorldParticleManager::Submit (PARTICLE_POOL_SPRITE, submission);
	}

	Check (WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE) == 64,
			"a pool asked for 256 materials opened %d batches",
			WorldParticleManager::Get_Pending_Batches (PARTICLE_POOL_SPRITE));
	Check (WorldParticleManager::Get_Refused_Submission_Count (PARTICLE_POOL_SPRITE) == 192,
			"%u of the 192 materials past the table's end were refused",
			WorldParticleManager::Get_Refused_Submission_Count (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Shutdown ();

	return _Failures;
}


/*
**	The weather layer, checked without any weather.
**
**	Everything Section 36's acceptance is about happens before a device is needed: how far
**	apart the fall lines are put, how many particles that comes to, and what happens to that
**	number when it is larger than the frame is allowed to hold.  The renderer decides all of
**	it from the density, the mode's constants and the clock, so a check can ask it and then
**	count what it handed over.
**
**	No collision provider is installed, which is the dedicated-server case: nothing is under
**	the weather, so every column falls the full depth of its box and nothing lands.
*/
int	Run_Weather (void)
{
	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	//	The default camera sits at the origin looking down its own -Z, which in world terms is
	//	straight down -- so the lattice under it is in front of it, and the per-particle frustum
	//	rejection is exercised rather than avoided.
	WorldParticleManager::Init ();
	WorldParticleManager::Set_Enabled (true);

	WeatherEnvironmentRenderer::Set_Enabled (true);
	WeatherEnvironmentRenderer::Set_Max_Particles (4096);
	WeatherEnvironmentRenderer::Set_Max_Casts_Per_Frame (256);

	WeatherEnvironmentRenderer	rain (WEATHER_MODE_RAIN);

	Check (rain.Get_Mode() == WEATHER_MODE_RAIN, "a renderer built for rain reports mode %d",
			 (int)rain.Get_Mode());

	/*
	**	Weather falling at all.  Density is the stock quantity -- particles per square world
	**	unit -- and reaches the renderer from exactly where it reached the system it replaced.
	*/
	rain.Set_Density (1.0f);
	rain.Update (0.1f, Vector3 (0.0f, 0.0f, 0.0f));

	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);

	int	drawn_once = rain.Get_Drawn_Particles();
	float	spacing_once = rain.Get_Column_Spacing();

	Check (drawn_once > 0, "rain at a density of one drew nothing");
	Check (drawn_once <= 4096, "rain drew %d particles against a budget of 4096", drawn_once);
	Check (rain.Get_Column_Count() > 0, "rain drew %d particles from no columns at all", drawn_once);

	/*
	**	The same clock and the same density are the same weather.  This is what "no simulation"
	**	buys and it is worth pinning: nothing about a particle is carried from one frame to the
	**	next, so rendering twice without advancing anything cannot drift.
	*/
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);

	Check (rain.Get_Drawn_Particles() == drawn_once,
			 "the same weather drawn twice gave %d particles and then %d",
			 drawn_once, rain.Get_Drawn_Particles());

	/*
	**	More weather is more particles, while there is room for them.
	*/
	WeatherEnvironmentRenderer::Set_Max_Particles (8192);
	rain.Set_Density (1.0f);
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);
	int	drawn_light = rain.Get_Drawn_Particles();

	rain.Set_Density (2.0f);
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);
	int	drawn_heavy = rain.Get_Drawn_Particles();

	Check (drawn_heavy > drawn_light,
			 "doubling the density drew %d particles where half of it drew %d",
			 drawn_heavy, drawn_light);

	/*
	**	The acceptance itself: a large scene at a bounded count.  Ten times the density of heavy
	**	rain, against a budget of two hundred and fifty-six, must produce at most two hundred and
	**	fifty-six -- and must do it by putting the fall lines further apart, not by drawing part
	**	of the scene and stopping.  The spacing is how that is told apart.
	*/
	WeatherEnvironmentRenderer::Set_Max_Particles (256);
	rain.Set_Density (20.0f);
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);

	int	drawn_capped = rain.Get_Drawn_Particles();
	float	spacing_capped = rain.Get_Column_Spacing();

	Check (drawn_capped > 0, "rain under a budget of 256 drew nothing at all");
	Check (drawn_capped <= 256, "rain drew %d particles against a budget of 256", drawn_capped);
	Check (spacing_capped > spacing_once,
			 "a budget of 256 left the fall lines %.2f apart, no further than the %.2f of an "
			 "unbudgeted scene, so it truncated rather than coarsened",
			 spacing_capped, spacing_once);

	/*
	**	The budget belongs to the frame and not to any one kind of weather.  Two kinds falling
	**	at once must come to the bound between them; the stock engine's cap, by contrast, was
	**	per system and three systems could each reach it.
	*/
	WeatherEnvironmentRenderer	snow (WEATHER_MODE_SNOW);

	snow.Set_Density (20.0f);
	snow.Update (0.1f, Vector3 (0.0f, 0.0f, 0.0f));

	WeatherEnvironmentRenderer::Set_Max_Particles (1024);
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);
	snow.Render (rinfo);

	int	shared = rain.Get_Drawn_Particles() + snow.Get_Drawn_Particles();

	Check (shared <= 1024,
			 "rain and snow in one frame drew %d particles between them against a budget of 1024",
			 shared);
	Check (rain.Get_Drawn_Particles() > 0 && snow.Get_Drawn_Particles() > 0,
			 "sharing a budget left rain with %d particles and snow with %d; one of them was "
			 "starved rather than thinned",
			 rain.Get_Drawn_Particles(), snow.Get_Drawn_Particles());

	/*
	**	Which pool each mode is drawn out of.  Rain is a streak because a raindrop is longer than
	**	it is wide and a sprite has only one size; everything else is a sprite.
	*/
	WorldParticleManager::Reset_Statistics ();
	WeatherEnvironmentRenderer::Set_Max_Particles (4096);
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);

	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_STREAK) == 1,
			 "rain made %u submission(s) to the streak pool, not one",
			 WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_STREAK));
	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE) == 0,
			 "rain made %u submission(s) to the sprite pool",
			 WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE));

	WorldParticleManager::Reset_Statistics ();
	WeatherEnvironmentRenderer::Begin_Frame ();
	snow.Render (rinfo);

	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE) == 1,
			 "snow made %u submission(s) to the sprite pool, not one",
			 WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE));

	/*
	**	Weather is charged to the environment class, which is what that class was reserved for
	**	when the particle pools were built and what it has had no caller for until now.
	*/
	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_ENVIRONMENT) > 0,
			 "snow drew %u particles charged to the environment budget",
			 WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_ENVIRONMENT));
	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_NEAR) == 0,
			 "snow charged %u particles to a combat budget",
			 WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE, PARTICLE_BUDGET_COMBAT_NEAR));

	/*
	**	Wind does not blow weather out of its own bounding box.  A falling particle is upwind of
	**	where it will land by a whole fall's worth of wind, and the scene culls this object by
	**	the box, so the box has to know that.
	*/
	Vector3	still_min, still_max, windy_min, windy_max;

	snow.Set_Wind (Vector2 (0.0f, 0.0f));
	snow.Update (0.1f, Vector3 (0.0f, 0.0f, 0.0f));
	snow.Get_Bounds (still_min, still_max);

	snow.Set_Wind (Vector2 (8.0f, 0.0f));
	snow.Update (0.1f, Vector3 (0.0f, 0.0f, 0.0f));
	snow.Get_Bounds (windy_min, windy_max);

	Check (windy_max.X > still_max.X,
			 "wind of eight units left the bounding box at %.2f, no wider than the %.2f it had "
			 "in still air",
			 windy_max.X, still_max.X);

	/*
	**	Off is off.  There is no stock path left to fall back to -- the per-particle system it
	**	replaced is gone rather than switched away from -- so this is the only sense in which
	**	weather can be absent.
	*/
	WeatherEnvironmentRenderer::Set_Enabled (false);
	WeatherEnvironmentRenderer::Begin_Frame ();
	rain.Render (rinfo);

	Check (rain.Get_Drawn_Particles() == 0,
			 "a disabled weather layer drew %d particles", rain.Get_Drawn_Particles());

	WeatherEnvironmentRenderer::Set_Enabled (true);

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Shutdown ();

	return _Failures;
}


/*
**	The tracer layer, checked without a scene or a device.
**
**	A tracer carries no simulation of its own -- WeaponClass has already done the raycast
**	that decides where it ends before Add_Tracer is ever called -- so everything the
**	acceptance is about is arithmetic on a fixed pool: whether a live tracer draws, whether
**	an expired one stops, and whether ten shots fired in one frame cost one draw call or ten.
*/
int	Run_Tracers (void)
{
	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	WorldParticleManager::Init ();
	WorldParticleManager::Set_Enabled (true);

	TracerRenderer::Init ();
	TracerRenderer::Set_Enabled (true);
	TracerRenderer::Set_Max_Tracers (256);

	//	Straight down the default camera's own -Z, the same forward axis the weather check
	//	relies on to put its lattice in view without aiming anything at it explicitly.
	Vector3	muzzle (0.0f,0.0f,0.0f);
	Vector3	target (0.0f,0.0f,-10.0f);
	Vector4	red (0.8f,0.0f,0.0f,0.8f);

	/*
	**	One tracer, aged forward inside its lifetime, is still there and still drawn.
	*/
	TracerRenderer::Add_Tracer (muzzle,target,0.1f,red,0.6f);
	TracerRenderer::Age (0.1f);
	TracerRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (TracerRenderer::Get_Live_Count() == 1,
			 "one tracer a tenth of a second into a six-tenths lifetime left %d live",
			 TracerRenderer::Get_Live_Count());
	Check (TracerRenderer::Get_Drawn_Particles() == 1,
			 "one live tracer in front of the camera drew %d",
			 TracerRenderer::Get_Drawn_Particles());

	/*
	**	Render is read-only.  Calling it again against the same clock, with no Age between,
	**	submits the same tracer -- nothing about drawing decides what is still alive.
	*/
	TracerRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));
	Check (TracerRenderer::Get_Drawn_Particles() == 1,
			 "re-rendering the same clock drew %d, not the 1 it drew the first time",
			 TracerRenderer::Get_Drawn_Particles());

	/*
	**	Ageing it past its lifetime reaps it.  This is the acceptance for a tracer in
	**	particular: the flash has to actually go away, not linger the way an object once
	**	allocated for it and left on a six-tenths-of-a-second timer used to.
	*/
	TracerRenderer::Age (0.6f);
	TracerRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (TracerRenderer::Get_Live_Count() == 0,
			 "a tracer aged past its lifetime left %d live",
			 TracerRenderer::Get_Live_Count());
	Check (TracerRenderer::Get_Drawn_Particles() == 0,
			 "an expired tracer still drew %d",
			 TracerRenderer::Get_Drawn_Particles());

	TracerRenderer::Reset ();

	/*
	**	Ten shots fired in one frame are one submission, not ten -- the acceptance's own
	**	wording, "pooled/batched render modules without script-spawned effect objects for
	**	every tracer", read as an arithmetic count.
	*/
	WorldParticleManager::Reset_Statistics ();
	for (int i = 0; i < 10; i++) {
		TracerRenderer::Add_Tracer (muzzle,target,0.1f,red,0.6f);
	}
	TracerRenderer::Age (0.0f);
	TracerRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (TracerRenderer::Get_Drawn_Particles() == 10,
			 "ten shots in one frame drew %d particles",
			 TracerRenderer::Get_Drawn_Particles());
	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_STREAK) == 1,
			 "ten tracers made %u submission(s) to the streak pool, not one",
			 WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_STREAK));

	/*
	**	Charged to a combat budget, resolved from distance the way any submission that names
	**	no class is -- and never to the environment class, which stays weather's alone even
	**	though both draw through the same streak pool.
	*/
	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_STREAK,PARTICLE_BUDGET_COMBAT_NEAR) > 0,
			 "ten close tracers charged %u particles to the near-combat budget",
			 WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_STREAK,PARTICLE_BUDGET_COMBAT_NEAR));
	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_STREAK,PARTICLE_BUDGET_ENVIRONMENT) == 0,
			 "tracers charged %u particles to the environment budget, which is weather's alone",
			 WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_STREAK,PARTICLE_BUDGET_ENVIRONMENT));

	TracerRenderer::Reset ();

	/*
	**	The acceptance's other half: a pool asked to hold more than its ceiling drops the
	**	newest arrival and counts it, rather than growing without bound or evicting a tracer
	**	already on screen to make room for a newer one.
	*/
	TracerRenderer::Set_Max_Tracers (4);
	for (int i = 0; i < 6; i++) {
		TracerRenderer::Add_Tracer (muzzle,target,0.1f,red,0.6f);
	}

	Check (TracerRenderer::Get_Live_Count() == 4,
			 "a pool capped at 4 held %d tracers after 6 arrived",
			 TracerRenderer::Get_Live_Count());
	Check (TracerRenderer::Get_Dropped_Count() == 2,
			 "a pool capped at 4 reported %d dropped after 6 arrived, not 2",
			 TracerRenderer::Get_Dropped_Count());

	TracerRenderer::Set_Max_Tracers (256);
	TracerRenderer::Reset ();

	/*
	**	Off is off.  There is no per-shot scene object left to fall back to -- the object this
	**	replaced is gone rather than switched away from -- so this is the only sense in which
	**	the tracer layer can be absent.
	*/
	TracerRenderer::Set_Enabled (false);
	TracerRenderer::Add_Tracer (muzzle,target,0.1f,red,0.6f);
	TracerRenderer::Age (0.1f);
	TracerRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (TracerRenderer::Get_Live_Count() == 0,
			 "a disabled tracer layer accepted a shot anyway, leaving %d live",
			 TracerRenderer::Get_Live_Count());
	Check (TracerRenderer::Get_Drawn_Particles() == 0,
			 "a disabled tracer layer drew %d particles",
			 TracerRenderer::Get_Drawn_Particles());

	TracerRenderer::Set_Enabled (true);
	TracerRenderer::Reset ();
	TracerRenderer::Shutdown ();

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Shutdown ();

	return _Failures;
}


/*
**	Section 28's device-less half: DebrisRenderer directly, the same layer the weather and
**	tracer checks exercise their own renderer at -- not DebrisManager, whose gameplay tier
**	needs a live combat scene this binary never brings up (see
**	docs/zerohour/DebrisRenderModule.md).  What this pins: a chip inside its lifetime is alive
**	and drawn; re-rendering the same clock draws the same count; an expired chip is reaped; a
**	scatter of ten chips in one frame makes one submission to the sprite pool, not ten; those
**	ten are charged to the near-combat budget and never to the environment budget, which stays
**	weather's alone even though weather's own snow shares this pool; a pool capped at four
**	holds at most four after six arrive and reports exactly two dropped; a disabled layer
**	accepts no scatter and draws nothing.
*/
int	Run_Debris (void)
{
	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	WorldParticleManager::Init ();
	WorldParticleManager::Set_Enabled (true);

	DebrisRenderer::Init ();
	DebrisRenderer::Set_Enabled (true);
	DebrisRenderer::Set_Max_Debris (256);

	//	Straight down the default camera's own -Z, the same forward axis the tracer and
	//	weather checks rely on to put their content in view without aiming at it explicitly.
	//	A scatter radius of zero makes every chip land exactly on 'center' -- Spawn still
	//	randomises each chip's velocity, size and spin, but not where it starts, which is what
	//	a frustum-visibility assertion needs to be deterministic.
	Vector3	center (0.0f,0.0f,-10.0f);
	Vector4	grey (0.42f,0.38f,0.34f,1.0f);

	/*
	**	One chip, aged forward inside its lifetime, is still there and still drawn.
	*/
	DebrisRenderer::Spawn (center,0.0f,1,grey,3.0f);
	DebrisRenderer::Age (0.3f);
	DebrisRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (DebrisRenderer::Get_Live_Count() == 1,
			 "one chip three-tenths of a second into a three-second lifetime left %d live",
			 DebrisRenderer::Get_Live_Count());
	Check (DebrisRenderer::Get_Drawn_Particles() == 1,
			 "one live chip in front of the camera drew %d",
			 DebrisRenderer::Get_Drawn_Particles());

	/*
	**	Render is read-only.  Calling it again against the same clock, with no Age between,
	**	submits the same chip -- nothing about drawing decides what is still alive.
	*/
	DebrisRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));
	Check (DebrisRenderer::Get_Drawn_Particles() == 1,
			 "re-rendering the same clock drew %d, not the 1 it drew the first time",
			 DebrisRenderer::Get_Drawn_Particles());

	/*
	**	Ageing it past its lifetime reaps it -- the acceptance's own point: a piece of
	**	lightweight, bounded-lifetime debris has to actually go away.
	*/
	DebrisRenderer::Age (3.0f);
	DebrisRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (DebrisRenderer::Get_Live_Count() == 0,
			 "a chip aged past its lifetime left %d live",
			 DebrisRenderer::Get_Live_Count());
	Check (DebrisRenderer::Get_Drawn_Particles() == 0,
			 "an expired chip still drew %d",
			 DebrisRenderer::Get_Drawn_Particles());

	DebrisRenderer::Reset ();

	/*
	**	Ten pieces scattered in one frame are one submission, not ten -- the acceptance's own
	**	wording, "does not require every visible fragment to be a complete networked GameObj",
	**	read as an arithmetic count on the render side of it.
	*/
	WorldParticleManager::Reset_Statistics ();
	DebrisRenderer::Spawn (center,0.0f,10,grey,3.0f);
	DebrisRenderer::Age (0.0f);
	DebrisRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (DebrisRenderer::Get_Drawn_Particles() == 10,
			 "ten pieces in one scatter drew %d particles",
			 DebrisRenderer::Get_Drawn_Particles());
	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE) == 1,
			 "ten debris chips made %u submission(s) to the sprite pool, not one",
			 WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_SPRITE));

	/*
	**	Charged to a combat budget, resolved from distance the way any submission that names
	**	no class is -- and never to the environment class, which stays weather's alone even
	**	though both draw through the sprite pool (weather's snow is the other user of it).
	*/
	WorldParticleManager::Flush (rinfo);

	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE,PARTICLE_BUDGET_COMBAT_NEAR) > 0,
			 "ten close debris chips charged %u particles to the near-combat budget",
			 WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE,PARTICLE_BUDGET_COMBAT_NEAR));
	Check (WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE,PARTICLE_BUDGET_ENVIRONMENT) == 0,
			 "debris charged %u particles to the environment budget, which is weather's alone",
			 WorldParticleManager::Get_Budget_Particles (PARTICLE_POOL_SPRITE,PARTICLE_BUDGET_ENVIRONMENT));

	DebrisRenderer::Reset ();

	/*
	**	A pool asked to hold more than its ceiling drops the newest arrivals and counts them,
	**	rather than growing without bound or evicting a chip already on screen to make room.
	*/
	DebrisRenderer::Set_Max_Debris (4);
	DebrisRenderer::Spawn (center,0.0f,6,grey,3.0f);

	Check (DebrisRenderer::Get_Live_Count() == 4,
			 "a pool capped at 4 held %d chips after 6 arrived",
			 DebrisRenderer::Get_Live_Count());
	Check (DebrisRenderer::Get_Dropped_Count() == 2,
			 "a pool capped at 4 reported %d dropped after 6 arrived, not 2",
			 DebrisRenderer::Get_Dropped_Count());

	DebrisRenderer::Set_Max_Debris (256);
	DebrisRenderer::Reset ();

	/*
	**	Off is off.  There is no per-fragment scene object left to fall back to, so this is the
	**	only sense in which the visual debris layer can be absent.
	*/
	DebrisRenderer::Set_Enabled (false);
	DebrisRenderer::Spawn (center,0.0f,1,grey,3.0f);
	DebrisRenderer::Age (0.1f);
	DebrisRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (DebrisRenderer::Get_Live_Count() == 0,
			 "a disabled debris layer accepted a scatter anyway, leaving %d live",
			 DebrisRenderer::Get_Live_Count());
	Check (DebrisRenderer::Get_Drawn_Particles() == 0,
			 "a disabled debris layer drew %d particles",
			 DebrisRenderer::Get_Drawn_Particles());

	DebrisRenderer::Set_Enabled (true);
	DebrisRenderer::Reset ();
	DebrisRenderer::Shutdown ();

	WorldParticleManager::Flush (rinfo);
	WorldParticleManager::Shutdown ();

	return _Failures;
}


/*
**	A host to hang attachments off that needs no art and no device.  RenderObjClass is
**	abstract, so the attachment check cannot simply make one; this is the same trick
**	TracerSystemClass and DebrisSystemClass already use to be scene objects without being
**	models.  Its bones are whatever the base class provides, which is none -- exactly the
**	state the refusal paths below are about.
*/
class NullHostRenderObjClass : public RenderObjClass
{
	public:
		virtual RenderObjClass *	Clone (void) const override	{ return nullptr; }
		virtual void				Render (RenderInfoClass &) override	{ }
		virtual int					Class_ID (void) const override	{ return CLASSID_UNKNOWN; }
};


int	Run_Attachments (void)
{
	/*
	**	The attachment layer resolves child models through the asset manager, so the check
	**	needs one -- but only the object, not a device: the constructor registers prototype
	**	loaders and allocates a hash table and touches no DirectX, and with load-on-demand off
	**	(its own default) an unknown name is answered out of an empty prototype table.  That is
	**	the whole of what these assertions exercise.
	*/
	WW3DAssetManager	assets;

	NullHostRenderObjClass * host = NEW_REF (NullHostRenderObjClass, ());

	CompositeAttachmentClass	controller;

	Check (controller.Get_Slot_Count() == 0,
			 "a fresh attachment controller already held %d slots",
			 controller.Get_Slot_Count());
	Check (controller.Is_Slot_Attached ("upgrade") == false,
			 "a fresh attachment controller reported slot 'upgrade' attached");

	/*
	**	The missing-art rule, which is the one behaviour here worth pinning: a child model
	**	with no art yet has to refuse the attachment and leave the controller untouched, not
	**	crash and not occupy the slot with nothing in it.  No .w3d has ever been loaded in this
	**	process, so every name is a missing one.
	*/
	bool	attached = controller.Attach (host,"upgrade","TURRETBONE","ow_no_such_model_exists");

	Check (attached == false,
			 "attaching a model that does not exist reported success");
	Check (controller.Get_Slot_Count() == 0,
			 "a refused attachment still left %d slots behind",
			 controller.Get_Slot_Count());
	Check (controller.Is_Slot_Attached ("upgrade") == false,
			 "a refused attachment still reported its slot attached");

	/*
	**	Every accessor has to be safe against a slot that was never filled -- a caller acting
	**	on a module whose art is missing is the ordinary case, not an error.
	*/
	controller.Detach ("upgrade");
	controller.Set_Slot_Hidden ("upgrade",true);

	Check (controller.Set_Slot_Animation ("upgrade","anything",true) == false,
			 "animating a slot that was never attached reported success");
	Check (controller.Get_Slot_Count() == 0,
			 "operating on an unattached slot created %d slots",
			 controller.Get_Slot_Count());

	controller.Detach_All ();

	Check (controller.Get_Slot_Count() == 0,
			 "Detach_All on an empty controller left %d slots",
			 controller.Get_Slot_Count());

	host->Release_Ref ();

	return _Failures;
}


/*
**	The shared tactical intelligence model -- roadmap Section 31.
**
**	Section 31's own rule is "do not expose strategic radar-only/last-known information as
**	live FPS wallhacks", so what this pins is the separation: Build_FPS_Contacts never touches
**	WorldShroudManager and can never produce a RADAR_ONLY or LAST_KNOWN contact, no matter what
**	is registered in the shroud.  RadarManager::Init() is not called here -- it allocates a
**	Render2DClass, which wants a device -- so this exercises the model against an empty world
**	and RadarManager's marker store directly, the same device-less discipline as every other
**	check in this file.
*/
int	Run_Tactical_Intelligence (void)
{
	TacticalContactListClass	contacts;

	//
	//	Nothing registered anywhere: both queries are empty, not a crash.
	//
	TacticalIntelligenceModel::Build_FPS_Contacts (nullptr,2 /* RADAR_ALL */,nullptr,0.0f,3.0f,contacts);
	Check (contacts.Count () == 0,
			 "an empty world produced %d FPS contact(s)",contacts.Count ());
	Check (TacticalIntelligenceModel::Get_Last_FPS_Contact_Count () == 0,
			 "the FPS contact counter disagrees with the list it built");

	TacticalIntelligenceModel::Build_Strategic_Contacts (0,contacts);
	Check (contacts.Count () == 0,
			 "an empty world with no markers or objectives produced %d strategic contact(s)",
			 contacts.Count ());

	//
	//	A marker is the one contact source this check can populate without a device or a
	//	physics scene.  It has to come back as CONTACT_COMMANDER_MARKER and never be excluded
	//	by shroud -- markers, like objectives, are mission-known already.
	//
	RadarMarkerClass	marker;
	marker.ID			= 1;
	marker.Type			= 0;
	marker.Color		= 0;
	marker.Position	= Vector3 (500.0f,500.0f,0.0f);
	marker.Intensity	= 1.0f;
	RadarManager::Add_Marker (marker);

	TacticalIntelligenceModel::Build_Strategic_Contacts (0,contacts);
	Check (contacts.Count () == 1,
			 "one registered marker produced %d strategic contact(s)",contacts.Count ());
	if (contacts.Count () == 1) {
		Check (contacts[0].Type == CONTACT_COMMANDER_MARKER,
				 "a marker did not classify as CONTACT_COMMANDER_MARKER");
		Check (contacts[0].Shroud == SHROUD_VISIBLE,
				 "a marker was shroud-gated even though markers are mission-known");
		Check (contacts[0].GameObj == nullptr,
				 "a synthetic marker contact carries a GameObj pointer");
	}

	RadarManager::Clear_Markers ();
	TacticalIntelligenceModel::Build_Strategic_Contacts (0,contacts);
	Check (contacts.Count () == 0,
			 "clearing the only marker left %d contact(s) behind",contacts.Count ());

	return _Failures;
}


/*
**	Waypoint / status-circle renderers -- roadmap Section 32.
**
**	StatusOverlayRenderer is device-less exercisable the same way TracerRenderer is: the pool,
**	the clock and the batched streak submission all run with no real graphics device, so this
**	pins both halves of the acceptance -- "batched" as a submission count, and "respects
**	shroud/per-player visibility" as StatusOverlaySystem's ownership/shroud policy, which is a
**	pure function of a handle and a team and needs neither a scene nor a device either.
*/
int	Run_Status_Overlays (void)
{
	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	WorldParticleManager::Init ();
	WorldParticleManager::Set_Enabled (true);

	StatusOverlayRenderer::Init ();
	StatusOverlayRenderer::Set_Enabled (true);
	StatusOverlayRenderer::Set_Max_Overlays (256);

	Vector3	point (0.0f,0.0f,-10.0f);
	Vector4	green (0.0f,0.8f,0.0f,0.8f);

	/*
	**	One persistent ring (lifetime <= 0) survives ageing and stays drawn -- a selection
	**	ring has to outlast a single frame the way a tracer never does.
	*/
	StatusOverlayHandleType handle = StatusOverlayRenderer::Add_Overlay (
		OVERLAY_SELECTION,point,2.0f,green,0.0f);
	Check (handle != 0,"adding one overlay with room available was refused");
	Check (StatusOverlayRenderer::Get_Live_Count () == 1,
			 "one added overlay left %d live",StatusOverlayRenderer::Get_Live_Count ());

	StatusOverlayRenderer::Age (10.0f);
	Check (StatusOverlayRenderer::Is_Overlay (handle),
			 "a persistent (lifetime <= 0) overlay was reaped by ageing");

	StatusOverlayRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));
	Check (StatusOverlayRenderer::Get_Drawn_Segment_Count () == 12,
			 "one ring in front of the camera drew %d segments, not the 12 that approximate it",
			 StatusOverlayRenderer::Get_Drawn_Segment_Count ());

	//	Set_Visible is the one hook a viewer-team policy uses; the pool itself just obeys it.
	StatusOverlayRenderer::Set_Visible (handle,false);
	StatusOverlayRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));
	Check (StatusOverlayRenderer::Get_Drawn_Segment_Count () == 0,
			 "an overlay marked not visible still drew %d segments",
			 StatusOverlayRenderer::Get_Drawn_Segment_Count ());
	StatusOverlayRenderer::Set_Visible (handle,true);

	StatusOverlayRenderer::Remove_Overlay (handle);
	Check (!StatusOverlayRenderer::Is_Overlay (handle),"a removed overlay still reports as one");
	Check (StatusOverlayRenderer::Get_Live_Count () == 0,
			 "removing the only overlay left %d live",StatusOverlayRenderer::Get_Live_Count ());

	StatusOverlayRenderer::Reset ();

	/*
	**	Batching: twenty rings in one frame make one streak submission, not twenty -- the same
	**	arithmetic reading of "batched" the tracer check uses.
	*/
	WorldParticleManager::Reset_Statistics ();
	for (int i = 0; i < 20; i++) {
		StatusOverlayRenderer::Add_Overlay (OVERLAY_TARGET,point,1.0f,green,0.0f);
	}
	StatusOverlayRenderer::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));

	Check (StatusOverlayRenderer::Get_Drawn_Segment_Count () == 20 * 12,
			 "twenty rings drew %d segments, not 240",
			 StatusOverlayRenderer::Get_Drawn_Segment_Count ());
	Check (WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_STREAK) == 1,
			 "twenty rings made %u submission(s) to the streak pool, not one",
			 WorldParticleManager::Get_Submission_Count (PARTICLE_POOL_STREAK));

	WorldParticleManager::Flush (rinfo);
	StatusOverlayRenderer::Reset ();

	/*
	**	The pool refuses rather than grows past its bound.
	*/
	StatusOverlayRenderer::Set_Max_Overlays (4);
	int made = 0;
	for (int i = 0; i < 6; i++) {
		if (StatusOverlayRenderer::Add_Overlay (OVERLAY_TARGET,point,1.0f,green,0.0f) != 0) {
			made++;
		}
	}
	Check (made == 4,"a pool capped at 4 accepted %d overlays out of 6",made);
	Check (StatusOverlayRenderer::Get_Dropped_Count () == 2,
			 "a pool capped at 4 reported %d dropped after 6 arrived, not 2",
			 StatusOverlayRenderer::Get_Dropped_Count ());

	StatusOverlayRenderer::Set_Max_Overlays (256);
	StatusOverlayRenderer::Reset ();
	StatusOverlayRenderer::Shutdown ();
	WorldParticleManager::Shutdown ();

	/*
	**	StatusOverlaySystem's visibility policy -- Section 32's "respects shroud/per-player
	**	visibility" -- checked with no scene, no device and no COMBAT_STAR.  It cannot go
	**	through StatusOverlaySystem::Add_Overlay (that lazily touches COMBAT_SCENE), so this
	**	drives StatusOverlayRenderer directly and asks StatusOverlaySystem::Is_Overlay_Visible_To
	**	the question the same way StatusOverlaySystem::Update() would each tick.
	*/
	StatusOverlayRenderer::Init ();

	StatusOverlayHandleType unowned = StatusOverlayRenderer::Add_Overlay (
		OVERLAY_CONSTRUCTION_FOOTPRINT,point,4.0f,green,0.0f);
	Check (StatusOverlaySystem::Is_Overlay_Visible_To (unowned,0),
			 "an unowned overlay was hidden from team 0");
	Check (StatusOverlaySystem::Is_Overlay_Visible_To (unowned,1),
			 "an unowned overlay was hidden from team 1");

	//	A handle StatusOverlaySystem was never told about reads as unowned (Get_Overlay_Team
	//	returns -1 for an index it has never seen) rather than wrongly gated.
	StatusOverlayHandleType untold = StatusOverlayRenderer::Add_Overlay (
		OVERLAY_SELECTION,Vector3 (10.0f,10.0f,0.0f),2.0f,green,0.0f);
	Check (StatusOverlaySystem::Get_Overlay_Team (untold) == -1,
			 "a handle StatusOverlaySystem was never told about reported team %d, not -1",
			 StatusOverlaySystem::Get_Overlay_Team (untold));
	Check (StatusOverlaySystem::Is_Overlay_Visible_To (untold,0),
			 "a handle with no recorded owner was shroud-gated instead of treated as unowned");

	WorldShroudManager::Init ();
	WorldShroudManager::Re_Partition (Vector3 (-1000.0f,-1000.0f,0.0f),Vector3 (1000.0f,1000.0f,0.0f));

	Vector3 enemy_point (500.0f,500.0f,0.0f);
	StatusOverlayHandleType team1_ring = StatusOverlayRenderer::Add_Overlay (
		OVERLAY_SELECTION,enemy_point,2.0f,green,0.0f);
	StatusOverlaySystem::Set_Overlay_Team (team1_ring,1);

	Check (StatusOverlaySystem::Is_Overlay_Visible_To (team1_ring,1),
			 "team 1's own ring was hidden from team 1");
	Check (!StatusOverlaySystem::Is_Overlay_Visible_To (team1_ring,0),
			 "team 1's ring was visible to team 0 with no shroud coverage there at all");

	//	Give team 0 a sensor source that actually reaches the ring's position, and only then
	//	does the gate open -- the acceptance's own words, "respects shroud/per-player
	//	visibility", proved as a state change rather than asserted.
	WorldShroudManager::RegisterStrategicSensorSource (0,enemy_point,50.0f,50.0f,5.0f,true);
	WorldShroudManager::Timestep (0.1f);

	Check (StatusOverlaySystem::Is_Overlay_Visible_To (team1_ring,0),
			 "team 1's ring stayed hidden from team 0 after team 0 gained shroud coverage there");

	WorldShroudManager::Shutdown ();
	StatusOverlayRenderer::Shutdown ();

	return _Failures;
}


/*
**	Guard state / retaliation -- roadmap Section 37.
**
**	GuardStateControllerClass is a pure state machine -- no GameObj, no ActionClass, no scene
**	-- specifically so the seven scenarios the roadmap itself names can be driven exactly, with
**	synthetic damage reports and positions, with no device and no live unit anywhere in sight.
*/
int	Run_Guard_AI (void)
{
	/*
	**	guard -> retaliate -> return.
	*/
	{
		GuardStateControllerClass c;
		Check (c.Activate (Vector3 (0.0f,0.0f,0.0f),3.0f),"Activate refused with room available");
		Check (c.Get_State () == GUARD_STATE_GUARDING,"a freshly activated controller was not GUARDING");

		c.Report_Damage (100,Vector3 (5.0f,0.0f,0.0f),0.0f);
		Check (c.Get_State () == GUARD_STATE_RETALIATING,"a damage report did not start retaliation");
		Check (c.Get_Current_Target_Id () == 100,"retaliation engaged the wrong target id");

		GuardAIIntentType intent = c.Update (0.0f,0.1f,Vector3 (5.0f,0.0f,0.0f),true,Vector3 (5.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (intent == GUARD_INTENT_ATTACK,"retaliating with a live target in range did not intend ATTACK");

		c.Report_Target_Lost (0.2f);
		Check (c.Get_State () == GUARD_STATE_REACQUIRING,"losing the only known target did not enter REACQUIRING");

		intent = c.Update (0.2f,0.1f,Vector3 (5.0f,0.0f,0.0f),false,Vector3 (0.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_State () == GUARD_STATE_RETURNING,"no threat left, outside the guard radius, did not head home");
		Check (intent == GUARD_INTENT_GOTO,"heading home was not signalled as GOTO");
		Check ((c.Get_Intent_Position () - Vector3 (0.0f,0.0f,0.0f)).Length () < 0.01f,
				 "the return GOTO did not target the guard anchor");

		intent = c.Update (1.3f,0.1f,Vector3 (0.0f,0.0f,0.0f),false,Vector3 (0.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_State () == GUARD_STATE_GUARDING,"arriving back at the anchor did not return to GUARDING");
		Check (intent == GUARD_INTENT_STAND,"arriving home was not signalled as STAND");

		c.Deactivate ();
	}

	/*
	**	multiple simultaneous threats, and guard -> lose target -> reacquire.
	*/
	{
		GuardStateControllerClass c;
		c.Activate (Vector3 (0.0f,0.0f,0.0f),50.0f);

		c.Report_Damage (1,Vector3 (5.0f,0.0f,0.0f),0.0f);
		Check (c.Get_Current_Target_Id () == 1,"the first attacker was not engaged");

		c.Report_Damage (2,Vector3 (-5.0f,0.0f,0.0f),0.5f);
		Check (c.Get_Current_Target_Id () == 1,
				 "a second simultaneous attacker stole the target away from the one already engaged");

		c.Report_Target_Lost (1.0f);
		GuardAIIntentType intent = c.Update (1.0f,0.1f,Vector3 (0.0f,0.0f,0.0f),false,Vector3 (0.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_State () == GUARD_STATE_RETALIATING,
				 "losing attacker 1 with attacker 2 still fresh in memory did not reacquire");
		Check (c.Get_Current_Target_Id () == 2,
				 "reacquire picked target %d instead of the remembered second attacker",c.Get_Current_Target_Id ());
		Check (intent == GUARD_INTENT_ATTACK,"reacquiring a remembered attacker did not intend ATTACK");

		c.Deactivate ();
	}

	/*
	**	invalid/dead target transition, noticed by Update itself rather than an explicit report.
	*/
	{
		GuardStateControllerClass c;
		c.Activate (Vector3 (0.0f,0.0f,0.0f),50.0f);
		c.Report_Damage (9,Vector3 (1.0f,0.0f,0.0f),0.0f);
		Check (c.Get_Current_Target_Id () == 9,"setup for the dead-target scenario did not engage id 9");

		//	target_valid == false, with no explicit Report_Target_Lost call: Update has to
		//	notice on its own.  With no other attacker remembered and the controller already
		//	inside its guard radius, the same tick also resolves reacquire and settles back to
		//	GUARDING -- proving the drop happened is the id changing and the state no longer
		//	being RETALIATING against something that no longer exists, not a transient state
		//	this synchronous a machine has no reason to still be in one call later.
		GuardAIIntentType intent = c.Update (0.0f,0.1f,Vector3 (0.0f,0.0f,0.0f),false,Vector3 (0.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_Current_Target_Id () == 0,
				 "Update kept target id %d alive after target_valid == false",c.Get_Current_Target_Id ());
		Check (c.Get_State () != GUARD_STATE_RETALIATING,
				 "Update did not notice target_valid == false on its own for a RETALIATING controller");
		Check (intent != GUARD_INTENT_ATTACK,"an intent to attack survived an invalid target");

		c.Deactivate ();
	}

	/*
	**	guard area boundary: the leash forces a return even while the target is still alive.
	*/
	{
		GuardStateControllerClass c;
		c.Activate (Vector3 (0.0f,0.0f,0.0f),3.0f);
		c.Report_Damage (5,Vector3 (2.0f,0.0f,0.0f),0.0f);

		GuardAIIntentType intent = c.Update (0.0f,0.1f,Vector3 (2.0f,0.0f,0.0f),true,Vector3 (2.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (intent == GUARD_INTENT_ATTACK,"a target still inside the leash was not attacked");

		intent = c.Update (0.1f,0.1f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_State () == GUARD_STATE_RETURNING,
				 "chasing a live target outside the leash did not force a return");
		Check (intent == GUARD_INTENT_GOTO,"the forced return was not signalled as GOTO");

		c.Deactivate ();
	}

	/*
	**	stuck unit recovery / path blocked -> replan, budgeted.
	*/
	{
		GuardStateControllerClass c;
		c.Activate (Vector3 (0.0f,0.0f,0.0f),3.0f);
		c.Report_Damage (7,Vector3 (10.0f,0.0f,0.0f),0.0f);

		//	Immediately outside the leash: one Update both enters RETURNING and proposes the
		//	first GOTO.
		c.Update (0.0f,0.1f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_State () == GUARD_STATE_RETURNING,"setup for the stuck scenario did not reach RETURNING");

		//	The position never changes across the next two thinks -- a blocked path -- until
		//	the stuck window (two seconds of "think" time) is spent.
		c.Update (0.1f,1.0f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_Stuck_Count () == 0,"stuck was flagged before the stuck window elapsed");

		GuardAIIntentType intent = c.Update (1.1f,1.0f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);
		Check (c.Get_Stuck_Count () == 1,"a unit motionless for the whole stuck window was not flagged stuck");
		Check (c.Get_Replan_Count () == 1,"a stuck unit with replan budget available did not replan");
		Check (intent == GUARD_INTENT_GOTO,"a replan was not signalled as a new GOTO");
		Check ((c.Get_Intent_Position () - Vector3 (0.0f,0.0f,0.0f)).Length () > 0.01f,
				 "a replanned GOTO landed on exactly the same point the blocked path already failed at");
		Check (GuardStateControllerClass::Get_Replans_This_Tick () == 1,
				 "one replan left the shared per-tick counter at %d, not 1",
				 GuardStateControllerClass::Get_Replans_This_Tick ());

		GuardStateControllerClass::Begin_Tick ();
		Check (GuardStateControllerClass::Get_Replans_This_Tick () == 0,
				 "Begin_Tick did not reset the shared replan budget for the next tick");

		c.Deactivate ();
	}

	/*
	**	The same stuck scenario again, with the shared per-tick replan budget set to zero --
	**	"expensive path replans are staggered/budgeted" has to mean something can actually be
	**	refused by it, not just that the counter exists.
	*/
	{
		GuardStateControllerClass::Set_Max_Replans_Per_Tick (0);

		GuardStateControllerClass c;
		c.Activate (Vector3 (0.0f,0.0f,0.0f),3.0f);
		c.Report_Damage (8,Vector3 (10.0f,0.0f,0.0f),0.0f);
		c.Update (0.0f,0.1f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);
		c.Update (0.1f,1.0f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);
		c.Update (1.1f,1.0f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);

		Check (c.Get_Stuck_Count () == 1,"stuck detection itself depended on the replan budget");
		Check (c.Get_Replan_Count () == 0,"a replan was issued despite a zero per-tick budget");

		c.Deactivate ();
		GuardStateControllerClass::Set_Max_Replans_Per_Tick (8);
	}

	/*
	**	Human-player priority (Section 3A.4): a proposed GOTO that would land on a nearby human
	**	is suppressed for that tick rather than issued through them.
	*/
	{
		GuardStateControllerClass c;
		c.Activate (Vector3 (0.0f,0.0f,0.0f),3.0f);
		c.Report_Damage (3,Vector3 (10.0f,0.0f,0.0f),0.0f);
		c.Update (0.0f,0.1f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),nullptr,-1.0f);

		Vector3 human_position (0.0f,0.0f,0.0f);		// standing right on the guard anchor
		GuardAIIntentType intent = c.Update (0.2f,1.0f,Vector3 (10.0f,0.0f,0.0f),true,Vector3 (10.0f,0.0f,0.0f),
														  &human_position,15.0f);
		Check (intent == GUARD_INTENT_STAND,
				 "a return-to-anchor GOTO landing on a human was issued instead of suppressed");
		Check (c.Get_Yield_Count () == 1,"a yielded tick was not counted");

		c.Deactivate ();
	}

	/*
	**	Population bound: refuses past its ceiling, and a freed slot is reusable.
	*/
	{
		GuardStateControllerClass::Set_Max_Active_Controllers (2);

		GuardStateControllerClass a,b,c;
		Check (a.Activate (Vector3 (0.0f,0.0f,0.0f),5.0f),"controller 1 of a 2-controller cap was refused");
		Check (b.Activate (Vector3 (0.0f,0.0f,0.0f),5.0f),"controller 2 of a 2-controller cap was refused");

		int refusals_before = GuardStateControllerClass::Get_Refusal_Count ();
		Check (!c.Activate (Vector3 (0.0f,0.0f,0.0f),5.0f),"a controller past the population cap was accepted");
		Check (GuardStateControllerClass::Get_Refusal_Count () > refusals_before,
				 "a refused controller was not counted");

		b.Deactivate ();
		Check (c.Activate (Vector3 (0.0f,0.0f,0.0f),5.0f),"a slot freed by Deactivate was not reusable");

		a.Deactivate ();
		c.Deactivate ();
		GuardStateControllerClass::Set_Max_Active_Controllers (64);
	}

	return _Failures;
}


/*
**	RTS telemetry -- roadmap Section 38.
*/
int	Run_Telemetry (void)
{
	RTSTelemetryService::Init ();

	Check (RTSTelemetryService::Is_Enabled (),"a freshly initialised service is not enabled");
	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL) == 0,
			 "a fresh service already has %d kill event(s)",RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL));

	/*
	**	Counting and subtype narrowing.
	*/
	RTSTelemetryService::Record_Event (TELEMETRY_KILL,0 /* PLAYERTYPE_NOD */,1.0f);
	RTSTelemetryService::Record_Event (TELEMETRY_KILL,0 /* PLAYERTYPE_NOD */,1.0f);
	RTSTelemetryService::Record_Event (TELEMETRY_KILL,1 /* PLAYERTYPE_GDI */,1.0f);

	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL) == 3,
			 "three kills recorded as %d total",RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL));
	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL,0) == 2,
			 "two Nod kills narrowed to subtype 0 read back as %d",
			 RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL,0));
	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL,1) == 1,
			 "one GDI kill narrowed to subtype 1 read back as %d",
			 RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL,1));
	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_DAMAGE) == 0,
			 "recording kills also recorded %d damage event(s)",RTSTelemetryService::Get_Event_Count (TELEMETRY_DAMAGE));

	/*
	**	Running totals, for categories where a number matters more than a tally.
	*/
	RTSTelemetryService::Record_Event (TELEMETRY_DAMAGE,0,12.5f);
	RTSTelemetryService::Record_Event (TELEMETRY_DAMAGE,0,7.5f);
	Check (WWMath::Fabs (RTSTelemetryService::Get_Event_Total (TELEMETRY_DAMAGE) - 20.0f) < 0.01f,
			 "12.5 + 7.5 damage totalled %f, not 20",RTSTelemetryService::Get_Event_Total (TELEMETRY_DAMAGE));

	/*
	**	The structured snapshot the acceptance names -- walkable without knowing the category
	**	enum in advance.
	*/
	DynamicVectorClass<RTSTelemetryService::SnapshotEntryClass>	snapshot;
	int snapshot_count = RTSTelemetryService::Get_Snapshot (snapshot);
	Check (snapshot_count == 3,		// (KILL,0) (KILL,1) (DAMAGE,0)
			 "two kill subtypes and one damage subtype produced a snapshot of %d, not 3",snapshot_count);
	Check (snapshot.Count () == snapshot_count,"Get_Snapshot's return value disagreed with the list it filled");

	Check (::strcmp (RTSTelemetryService::Get_Category_Name (TELEMETRY_KILL),"kill") == 0,
			 "TELEMETRY_KILL's name is '%s', not 'kill'",RTSTelemetryService::Get_Category_Name (TELEMETRY_KILL));
	Check (::strcmp (RTSTelemetryService::Get_Category_Name (TELEMETRY_CATEGORY_COUNT),"unknown") == 0,
			 "an out-of-range category did not read back 'unknown'");

	/*
	**	Off means Record_Event does nothing at all, not a suppressed-but-counted event.
	*/
	RTSTelemetryService::Reset ();
	RTSTelemetryService::Set_Enabled (false);
	RTSTelemetryService::Record_Event (TELEMETRY_KILL,0,1.0f);
	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL) == 0,
			 "a disabled service still recorded %d event(s)",RTSTelemetryService::Get_Event_Count (TELEMETRY_KILL));
	RTSTelemetryService::Set_Enabled (true);

	/*
	**	The bound refuses rather than grows: TELEMETRY_MAX_RECORDS distinct (category, subtype)
	**	pairs is the ceiling, tested with one category and enough distinct subtypes to cross it.
	*/
	RTSTelemetryService::Reset ();
	int made = 0;
	for (int i = 0; i < TELEMETRY_MAX_RECORDS + 8; i++) {
		int before = RTSTelemetryService::Get_Refusal_Count ();
		RTSTelemetryService::Record_Event (TELEMETRY_AI_ORDER,i,1.0f);
		if (RTSTelemetryService::Get_Refusal_Count () == before) made++;
	}
	Check (made == TELEMETRY_MAX_RECORDS,
			 "the record table let in %d distinct subtypes, not the %d bound",made,(int)TELEMETRY_MAX_RECORDS);
	Check (RTSTelemetryService::Get_Refusal_Count () == 8,
			 "8 over-the-bound records produced %d refusal(s)",RTSTelemetryService::Get_Refusal_Count ());

	RTSTelemetryService::Shutdown ();
	RTSTelemetryService::Init ();
	Check (RTSTelemetryService::Get_Event_Count (TELEMETRY_AI_ORDER) == 0,"records outlived Shutdown/Init");
	RTSTelemetryService::Shutdown ();

	return _Failures;
}


/*
**	Debug rendering tools -- roadmap Section 33.
*/
int	Run_Debug_Render (void)
{
	CameraClass				camera;
	RenderInfoClass		rinfo (camera);

	DebugRenderSystem::Init ();

	Check (!DebugRenderSystem::Is_Layer_Enabled (DEBUG_LAYER_BUILDING_FOOTPRINTS),
			 "a freshly initialised service has a layer already on");
	Check (::strcmp (DebugRenderSystem::Get_Layer_Name (DEBUG_LAYER_BUILDING_FOOTPRINTS),"building_footprints") == 0,
			 "DEBUG_LAYER_BUILDING_FOOTPRINTS's name is '%s', not 'building_footprints'",
			 DebugRenderSystem::Get_Layer_Name (DEBUG_LAYER_BUILDING_FOOTPRINTS));
	Check (::strcmp (DebugRenderSystem::Get_Layer_Name (DEBUG_LAYER_COUNT),"unknown") == 0,
			 "an out-of-range layer did not read back 'unknown'");

	DebugRenderSystem::Set_Layer_Enabled (DEBUG_LAYER_BUILDING_FOOTPRINTS,true);
	Check (DebugRenderSystem::Is_Layer_Enabled (DEBUG_LAYER_BUILDING_FOOTPRINTS),
			 "Set_Layer_Enabled(true) did not stick");
	DebugRenderSystem::Disable_All_Layers ();
	Check (!DebugRenderSystem::Is_Layer_Enabled (DEBUG_LAYER_BUILDING_FOOTPRINTS),
			 "Disable_All_Layers left a layer on");

	/*
	**	Primitive decomposition: a box is twelve edges, a cross is three.
	*/
	DebugRenderSystem::Begin_Frame ();
	Check (DebugRenderSystem::Get_Line_Count () == 0,"Begin_Frame did not clear the pool");

	DebugRenderSystem::Add_Box (Vector3 (-1.0f,-1.0f,-1.0f),Vector3 (1.0f,1.0f,1.0f),Vector4 (1,1,1,1));
	Check (DebugRenderSystem::Get_Line_Count () == 12,
			 "one box queued %d line(s), not 12",DebugRenderSystem::Get_Line_Count ());

	DebugRenderSystem::Add_Cross (Vector3 (0,0,0),1.0f,Vector4 (1,0,0,1));
	Check (DebugRenderSystem::Get_Line_Count () == 15,
			 "a box plus a cross queued %d line(s), not 15",DebugRenderSystem::Get_Line_Count ());

	//	In front of the default camera, along its own -Z, the same placement the tracer and
	//	overlay checks already rely on.
	DebugRenderSystem::Render (rinfo,Vector3 (0.0f,0.0f,0.0f));
	Check (DebugRenderSystem::Get_Drawn_Segment_Count () > 0,
			 "geometry in front of the camera drew %d segments",DebugRenderSystem::Get_Drawn_Segment_Count ());

	DebugRenderSystem::Begin_Frame ();
	Check (DebugRenderSystem::Get_Line_Count () == 0,"a second Begin_Frame did not clear the previous frame's lines");

	/*
	**	The pool refuses rather than grows.
	*/
	for (int i = 0; i < (DEBUG_LINE_POOL_MAX / 12) + 4; i++) {
		DebugRenderSystem::Add_Box (Vector3 (0,0,0),Vector3 (1,1,1),Vector4 (1,1,1,1));
	}
	Check (DebugRenderSystem::Get_Line_Count () <= DEBUG_LINE_POOL_MAX,
			 "the line pool grew to %d past its %d bound",DebugRenderSystem::Get_Line_Count (),(int)DEBUG_LINE_POOL_MAX);
	Check (DebugRenderSystem::Get_Dropped_Count () > 0,
			 "queuing well past the pool's bound reported %d dropped",DebugRenderSystem::Get_Dropped_Count ());

	DebugRenderSystem::Shutdown ();
	Check (DebugRenderSystem::Get_Line_Count () == 0,"lines outlived Shutdown");
	Check (!DebugRenderSystem::Is_Layer_Enabled (DEBUG_LAYER_BUILDING_FOOTPRINTS),"layer state outlived Shutdown");

	return _Failures;
}


/*
**	Building model semantic JSON -- roadmap Section 39.
**
**	The load-bearing claim under test is the reload transaction: a valid reload atomically
**	replaces a live descriptor, an invalid one never touches it.  That needs a real file on
**	disk to exercise end to end, so this check writes and rewrites one small scratch JSON file
**	rather than only ever parsing text in memory.
*/
static const char * const	VALID_POWER_PLANT_JSON =
	"{"
	" \"model_name\": \"ow_test_powerplant.w3d\","
	" \"schema_version\": 1,"
	" \"footprint\": { \"width\": 20.0, \"depth\": 20.0 },"
	" \"power\": { \"requires_power\": false, \"generation\": 500.0, \"consumption\": 0.0, \"priority\": 50 }"
	"}";

static const char * const	VALID_COMM_CENTER_JSON =
	"{"
	" \"model_name\": \"ow_test_commcenter.w3d\","
	" \"schema_version\": 1,"
	" \"footprint\": { \"width\": 15.0, \"depth\": 15.0 },"
	" \"power\": { \"requires_power\": true, \"generation\": 0.0, \"consumption\": 100.0, \"priority\": 60 },"
	" \"capabilities\": { \"communications\": { \"enabled\": true, \"vision_radius\": 180.0,"
	"     \"radar_radius\": 300.0, \"last_known_retention_seconds\": 20.0, \"requires_building_online\": true } },"
	" \"anchors\": { \"mct_anchor\": \"MCT_BONE\", \"purchase_terminal_anchor\": \"PT_BONE\" }"
	"}";

/*
**	Q-016's damage-visual table, in both of the shapes the schema allows: the three-name form
**	(intact/damaged/destroyed) and the same thing with an extra threshold state wedged in.  Both
**	must come out of the parser as one descending table ending at zero.
*/
static const char * const	DAMAGE_VISUAL_JSON =
	"{"
	" \"model_name\": \"ow_test_dv.w3d\","
	" \"schema_version\": 1,"
	" \"footprint\": { \"width\": 10.0, \"depth\": 10.0 },"
	" \"game_object_preset\": \"ow_test_preset\","
	" \"build\": { \"buildable\": true, \"cost\": 500, \"tech_level\": 1 },"
	" \"damage_visual\": {"
	"   \"intact_model\": \"ow_test_dv.w3d\","
	"   \"damaged_model\": \"ow_test_dv_d.w3d\","
	"   \"destroyed_model\": \"ow_test_dv_x.w3d\","
	"   \"damaged_health_fraction\": 0.5,"
	"   \"extra_states\": [ { \"health_fraction\": 0.8, \"model\": \"ow_test_dv_s1.w3d\" } ]"
	" }"
	"}";

//	No damage_visual block at all -- the missing-art case, which must still produce a usable
//	one-entry table rather than being refused.
static const char * const	NO_DAMAGE_VISUAL_JSON =
	"{ \"model_name\":\"ow_test_plain.w3d\",\"schema_version\":1,"
	"  \"footprint\":{\"width\":10,\"depth\":10} }";

//	Buildable, but names no preset: nothing could create the canonical building, so the
//	descriptor is refused at load rather than at placement.
static const char * const	BUILDABLE_NO_PRESET_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"build\":{\"buildable\":true,\"cost\":100} }";

//	Two states claiming the same threshold: the selection scan would be ambiguous, so refused.
static const char * const	DUPLICATE_THRESHOLD_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"damage_visual\":{\"intact_model\":\"a.w3d\",\"damaged_model\":\"b.w3d\","
	"    \"damaged_health_fraction\":0.0} }";

static const char * const	MALFORMED_JSON = "{ this is not valid json";

static const char * const	NEGATIVE_GENERATION_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":false,\"generation\":-5.0,\"consumption\":0,\"priority\":10} }";

static const char * const	INCOMPATIBLE_ROLE_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":false,\"generation\":100,\"consumption\":0,\"priority\":10},"
	"  \"capabilities\":{\"communications\":{\"enabled\":true,\"vision_radius\":10,\"radar_radius\":10}} }";

int	Run_Building_Descriptors (void)
{
	BuildingDescriptorManager::Init ();

	Check (BuildingDescriptorManager::Get_Descriptor_Count () == 0,"a fresh manager already has descriptors");
	Check (BuildingDescriptorManager::Get_Descriptor ("nothing") == nullptr,
			 "an unknown definition returned a descriptor instead of nullptr");

	/*
	**	A valid descriptor resolves, and its power/capability values are queryable -- the
	**	acceptance's own first two lines.
	*/
	StringClass error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_power_plant",VALID_POWER_PLANT_JSON,&error),
			 "a valid Power Plant descriptor was refused: %s",error.Peek_Buffer ());

	const BuildingModelDescriptor * plant = BuildingDescriptorManager::Get_Descriptor ("ow_test_power_plant");
	Check (plant != nullptr,"a loaded descriptor could not be looked back up");
	if (plant != nullptr) {
		Check (plant->Power.RequiresPower == false,"a generator's requires_power did not read back false");
		Check (WWMath::Fabs (plant->Power.Generation - 500.0f) < 0.01f,
				 "a generator's generation read back %f, not 500",plant->Power.Generation);
		Check (plant->Power.Priority == 50,"a generator's priority read back %d, not 50",plant->Power.Priority);
	}

	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_comm_center",VALID_COMM_CENTER_JSON,&error),
			 "a valid Communications Center descriptor was refused: %s",error.Peek_Buffer ());
	const BuildingModelDescriptor * comm = BuildingDescriptorManager::Get_Descriptor ("ow_test_comm_center");
	Check (comm != nullptr,"a loaded communications descriptor could not be looked back up");
	if (comm != nullptr) {
		Check (comm->Communications.Enabled,"a communications building's capability did not read back enabled");
		Check (WWMath::Fabs (comm->Communications.RadarRadius - 300.0f) < 0.01f,
				 "a communications building's radar_radius read back %f, not 300",comm->Communications.RadarRadius);
		const char * mct_bone = comm->Find_Anchor_Bone ("mct_anchor");
		Check (mct_bone != nullptr,"mct_anchor did not resolve to a bone name at all");
		Check (mct_bone != nullptr && ::strcmp (mct_bone,"MCT_BONE") == 0,
				 "mct_anchor resolved to '%s', not 'MCT_BONE'",(mct_bone != nullptr) ? mct_bone : "(null)");
		Check (comm->Find_Anchor_Bone ("no_such_role") == nullptr,
				 "an undeclared role resolved to a bone name instead of nullptr");
	}

	/*
	**	Rejection: malformed JSON, and a semantically invalid value, neither register a
	**	descriptor and both report an error.
	*/
	Check (!BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_malformed",MALFORMED_JSON,&error),
			 "malformed JSON was accepted");
	Check (error.Get_Length () > 0,"a rejected descriptor reported no error");
	Check (!BuildingDescriptorManager::Is_Loaded ("ow_test_malformed"),"a rejected descriptor is still loaded");

	Check (!BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_negative",NEGATIVE_GENERATION_JSON,&error),
			 "negative generation was accepted");
	Check (!BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_incompatible",INCOMPATIBLE_ROLE_JSON,&error),
			 "a generator declaring communications capability was accepted");

	/*
	**	Q-016's damage-visual table.  Everything here is arithmetic over data -- no scene, no
	**	model, no device -- which is why it belongs in this family rather than in a manual pass.
	*/
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_dv",DAMAGE_VISUAL_JSON,&error),
			 "a valid damage_visual descriptor was refused: %s",error.Peek_Buffer ());

	const BuildingModelDescriptor * dv = BuildingDescriptorManager::Get_Descriptor ("ow_test_dv");
	Check (dv != nullptr,"a loaded damage_visual descriptor could not be looked back up");
	if (dv != nullptr) {

		Check (dv->GameObjectPreset.Compare_No_Case ("ow_test_preset") == 0,
				 "game_object_preset read back '%s', not 'ow_test_preset'",dv->GameObjectPreset.Peek_Buffer ());

		//	Transitions 0.8 (into the extra state) and 0.5 (into damaged) become three bands:
		//	intact down to 0.8, the extra state down to 0.5, damaged down to 0.
		Check (dv->DamageVisual.States.Count () == 3,
				 "the damage table has %d states, not the three the file named",dv->DamageVisual.States.Count ());

		if (dv->DamageVisual.States.Count () == 3) {
			Check (WWMath::Fabs (dv->DamageVisual.States[0].MinHealthFraction - 0.8f) < 0.001f,
					 "the first state's threshold is %f, not the extra state's 0.8",
					 dv->DamageVisual.States[0].MinHealthFraction);
			Check (WWMath::Fabs (dv->DamageVisual.States[1].MinHealthFraction - 0.5f) < 0.001f,
					 "the second state's threshold is %f, not the intact 0.5",
					 dv->DamageVisual.States[1].MinHealthFraction);
			Check (dv->DamageVisual.States[2].MinHealthFraction == 0.0f,
					 "the last state's threshold is %f, not 0",dv->DamageVisual.States[2].MinHealthFraction);
		}

		//	Selection, at and across every boundary.  A threshold is inclusive at its own value.
		Check (dv->DamageVisual.Find_State_Index (1.0f) == 0,"full health did not select the top state");
		Check (dv->DamageVisual.Find_State_Index (0.8f) == 0,"health exactly at a threshold selected the state below it");
		Check (dv->DamageVisual.Find_State_Index (0.79f) == 1,"health just under a threshold did not drop a state");
		Check (dv->DamageVisual.Find_State_Index (0.5f) == 1,"health exactly at the damaged threshold selected the wrong state");
		Check (dv->DamageVisual.Find_State_Index (0.49f) == 2,"health under the damaged threshold did not select the damaged state");
		Check (dv->DamageVisual.Find_State_Index (0.0f) == 2,"zero health did not select the last living state");

		//	And the model each of those means, including the destroyed one, which is not a
		//	threshold and is only reached by asking for it.
		Check (::strcmp (dv->Get_Damage_Model (1.0f,false),"ow_test_dv.w3d") == 0,
				 "full health chose '%s', not the intact model",dv->Get_Damage_Model (1.0f,false));
		Check (::strcmp (dv->Get_Damage_Model (0.6f,false),"ow_test_dv_s1.w3d") == 0,
				 "mid health chose '%s', not the extra state's model",dv->Get_Damage_Model (0.6f,false));
		Check (::strcmp (dv->Get_Damage_Model (0.1f,false),"ow_test_dv_d.w3d") == 0,
				 "low health chose '%s', not the damaged model",dv->Get_Damage_Model (0.1f,false));
		Check (::strcmp (dv->Get_Damage_Model (0.0f,true),"ow_test_dv_x.w3d") == 0,
				 "a destroyed building chose '%s', not the destroyed model",dv->Get_Damage_Model (0.0f,true));

		//	Destroyed overrides the table whatever the health says, which is what makes a
		//	building killed at full health by a script still show rubble.
		Check (::strcmp (dv->Get_Damage_Model (1.0f,true),"ow_test_dv_x.w3d") == 0,
				 "destroyed at full health chose '%s', not the destroyed model",dv->Get_Damage_Model (1.0f,true));
	}

	//	Missing art: no damage_visual block at all still yields a usable one-entry table.
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_plain_dv",NO_DAMAGE_VISUAL_JSON,&error),
			 "a descriptor with no damage_visual block was refused: %s",error.Peek_Buffer ());
	const BuildingModelDescriptor * plain_dv = BuildingDescriptorManager::Get_Descriptor ("ow_test_plain_dv");
	Check (plain_dv != nullptr,"a descriptor with no damage_visual could not be looked back up");
	if (plain_dv != nullptr) {
		Check (plain_dv->DamageVisual.States.Count () == 1,
				 "an absent damage_visual produced %d states, not one",plain_dv->DamageVisual.States.Count ());
		Check (::strcmp (plain_dv->Get_Damage_Model (1.0f,false),"ow_test_plain.w3d") == 0,
				 "an absent damage_visual did not fall back to model_name");
		Check (::strcmp (plain_dv->Get_Damage_Model (0.0f,true),"ow_test_plain.w3d") == 0,
				 "an absent destroyed model did not keep the last living model");
	}

	Check (!BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_nopreset",BUILDABLE_NO_PRESET_JSON,&error),
			 "a buildable descriptor naming no game_object_preset was accepted");

	/*
	**	Q-015: Validate_Text is the editor's entry point, and its whole contract is that it
	**	reaches the same verdict as a load without having the load's side effect.  Both halves
	**	are proved: the same accept/reject answers, and nothing registered either way.
	*/
	Check (BuildingDescriptorManager::Validate_Text ("ow_test_validate_ok",DAMAGE_VISUAL_JSON,&error),
			 "validation refused a descriptor that loading accepts: %s",error.Peek_Buffer ());
	Check (!BuildingDescriptorManager::Is_Loaded ("ow_test_validate_ok"),
			 "validating a descriptor registered it -- validation must not have a side effect");

	Check (!BuildingDescriptorManager::Validate_Text ("ow_test_validate_bad",MALFORMED_JSON,&error),
			 "validation accepted malformed JSON that loading rejects");
	Check (error.Get_Length () > 0,"a failed validation reported no error");
	Check (!BuildingDescriptorManager::Is_Loaded ("ow_test_validate_bad"),
			 "a failed validation registered something");

	Check (!BuildingDescriptorManager::Validate_Text ("ow_test_validate_dup",DUPLICATE_THRESHOLD_JSON,&error),
			 "validation accepted a damage table loading rejects");

	//	A descriptor already loaded stays exactly as it was after validating different text
	//	under its own name -- the case that would bite an author validating a work in progress.
	Check (BuildingDescriptorManager::Is_Loaded ("ow_test_dv"),"the damage_visual descriptor vanished");
	BuildingDescriptorManager::Validate_Text ("ow_test_dv",NO_DAMAGE_VISUAL_JSON,&error);
	const BuildingModelDescriptor * unchanged = BuildingDescriptorManager::Get_Descriptor ("ow_test_dv");
	Check (unchanged != nullptr && unchanged->DamageVisual.States.Count () == 3,
			 "validating replacement text under a live descriptor's name modified it");

	Check (!BuildingDescriptorManager::Validate_File ("ow_no_such_descriptor_file.json",&error),
			 "validating a file that does not exist reported success");

	int checked_files = -1;
	StringClass directory_report ("");
	Check (BuildingDescriptorManager::Validate_Directory ("ow_no_such_directory",&directory_report,
																		  &checked_files) == -1,
			 "validating an unreadable directory did not report -1");
	Check (checked_files == 0,"an unreadable directory reported %d files checked, not 0",checked_files);
	Check (!BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_duptresh",DUPLICATE_THRESHOLD_JSON,&error),
			 "a damage table with two states at the same threshold was accepted");

	/*
	**	The reload transaction, end to end against a real file: valid replaces, invalid
	**	preserves the last known-good descriptor and reports the error.
	*/
	const char *	SCRATCH_PATH = "ow_selfcheck_building_descriptor.json";

	FILE * f = ::fopen (SCRATCH_PATH,"wb");
	Check (f != nullptr,"could not create the reload scratch file");
	if (f != nullptr) {
		::fwrite (VALID_POWER_PLANT_JSON,1,::strlen (VALID_POWER_PLANT_JSON),f);
		::fclose (f);

		Check (BuildingDescriptorManager::Load_Descriptor_From_File ("ow_test_reload",SCRATCH_PATH,&error),
				 "loading the reload scratch file failed: %s",error.Peek_Buffer ());

		//	Rewrite it with a different, still-valid generation value and reload.
		const char * UPDATED_JSON =
			"{ \"model_name\":\"ow_test_powerplant.w3d\",\"schema_version\":1,"
			"  \"footprint\":{\"width\":20,\"depth\":20},"
			"  \"power\":{\"requires_power\":false,\"generation\":750.0,\"consumption\":0,\"priority\":50} }";
		f = ::fopen (SCRATCH_PATH,"wb");
		::fwrite (UPDATED_JSON,1,::strlen (UPDATED_JSON),f);
		::fclose (f);

		Check (BuildingDescriptorManager::Reload_Descriptor ("ow_test_reload",&error),
				 "a valid reload was refused: %s",error.Peek_Buffer ());
		const BuildingModelDescriptor * reloaded = BuildingDescriptorManager::Get_Descriptor ("ow_test_reload");
		Check (reloaded != nullptr && WWMath::Fabs (reloaded->Power.Generation - 750.0f) < 0.01f,
				 "a valid reload did not publish the new generation value");

		//	Rewrite it with an invalid value and reload again -- this must fail and leave 750
		//	in place, not silently fall back to 0 or crash.
		f = ::fopen (SCRATCH_PATH,"wb");
		::fwrite (NEGATIVE_GENERATION_JSON,1,::strlen (NEGATIVE_GENERATION_JSON),f);
		::fclose (f);

		int failures_before = BuildingDescriptorManager::Get_Reload_Failure_Count ();
		Check (!BuildingDescriptorManager::Reload_Descriptor ("ow_test_reload",&error),
				 "an invalid reload (negative generation) was accepted");
		Check (BuildingDescriptorManager::Get_Reload_Failure_Count () > failures_before,
				 "an invalid reload was not counted as a failure");

		reloaded = BuildingDescriptorManager::Get_Descriptor ("ow_test_reload");
		Check (reloaded != nullptr && WWMath::Fabs (reloaded->Power.Generation - 750.0f) < 0.01f,
				 "an invalid reload changed the live descriptor instead of preserving the last known-good one");

		::remove (SCRATCH_PATH);
	}

	//	A descriptor loaded from text, not a file, has nothing to reload from -- refused, not a
	//	crash.
	Check (!BuildingDescriptorManager::Reload_Descriptor ("ow_test_power_plant",&error),
			 "reloading a text-loaded descriptor (no source file) was accepted");

	BuildingDescriptorManager::Unload_All ();
	Check (BuildingDescriptorManager::Get_Descriptor_Count () == 0,"Unload_All left descriptors behind");

	/*
	**	Q-017 -- Resolve_Game_Object_Presets and the Validate_Descriptor preset-class check, both
	**	of which only do anything once at least one CLASSID_BUILDINGS definition is registered.
	**	A real BuildingGameObjDef is constructed and registered by hand here, exactly the way
	**	ProjectBuildingDefs::Register does for Tiberium Silo/Advanced Comm Center in the real
	**	game -- no always.dat needed for this to be a meaningful test.
	*/
	{
		BuildingGameObjDef *	test_def = new BuildingGameObjDef;
		test_def->Set_Name ("ow_test_resolve_building");
		test_def->Set_ID (DefinitionMgrClass::Get_New_ID (test_def->Get_Class_ID ()));
		DefinitionMgrClass::Register_Definition (test_def);

		Check (test_def->Get_Descriptor_Name ().Get_Length () == 0,
				 "a freshly constructed BuildingGameObjDef already had a descriptor name");

		const char * RESOLVES_JSON =
			"{ \"model_name\":\"ow_test_resolve.w3d\",\"schema_version\":1,"
			"  \"game_object_preset\":\"ow_test_resolve_building\","
			"  \"footprint\":{\"width\":10,\"depth\":10},"
			"  \"power\":{\"requires_power\":false,\"generation\":0,\"consumption\":0,\"priority\":0},"
			"  \"build\":{\"buildable\":true,\"cost\":100,\"team\":1} }";
		Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_resolve",RESOLVES_JSON,&error),
				 "a descriptor naming a real, registered building preset was rejected: %s",error.Peek_Buffer ());

		const char * MISSING_PRESET_JSON =
			"{ \"model_name\":\"ow_test_missing.w3d\",\"schema_version\":1,"
			"  \"game_object_preset\":\"ow_test_no_such_preset_anywhere\","
			"  \"footprint\":{\"width\":10,\"depth\":10},"
			"  \"power\":{\"requires_power\":false,\"generation\":0,\"consumption\":0,\"priority\":0},"
			"  \"build\":{\"buildable\":true,\"cost\":100,\"team\":1} }";
		Check (!BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_missing_preset",MISSING_PRESET_JSON,&error),
				 "a buildable descriptor naming a preset no loaded definition provides was accepted, "
				 "now that a real building definition is registered to check against");

		int resolved = BuildingDescriptorManager::Resolve_Game_Object_Presets (nullptr);
		Check (resolved >= 1,"Resolve_Game_Object_Presets resolved nothing for a descriptor with a real preset");
		Check (test_def->Get_Descriptor_Name () == StringClass ("ow_test_resolve",true),
				 "Resolve_Game_Object_Presets did not set the definition's DescriptorName back-link "
				 "(got '%s')",test_def->Get_Descriptor_Name ().Peek_Buffer ());

		BuildingDescriptorManager::Unload_All ();
		DefinitionMgrClass::Unregister_Definition (test_def);
		delete test_def;
	}

	BuildingDescriptorManager::Shutdown ();

	return _Failures;
}


/*
**	Finite team power grid + strategic building capabilities -- roadmap Section 40.
**
**	Every member here is registered with a nullptr BuildingGameObj*, which is what makes the
**	allocation algorithm itself -- the acceptance's real substance -- checkable with no device,
**	no scene and no live GameObj.  What Enable_Power would have done to a real building is
**	Is_Member_Powered's own bookkeeping instead.
*/
static const char * const	GRID_GENERATOR_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":false,\"generation\":500.0,\"consumption\":0.0,\"priority\":50} }";

static const char * const	GRID_CONSUMER_HI_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":300.0,\"priority\":80} }";

static const char * const	GRID_CONSUMER_LO_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":300.0,\"priority\":20} }";

static const char * const	GRID_COMMS_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":50.0,\"priority\":60},"
	"  \"capabilities\":{\"communications\":{\"enabled\":true,\"vision_radius\":100.0,\"radar_radius\":200.0,"
	"      \"last_known_retention_seconds\":10.0,\"requires_building_online\":true}} }";

static const char * const	GRID_SILO_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":10.0,\"priority\":40},"
	"  \"capabilities\":{\"tiberium_silo\":{\"enabled\":true,\"wallet_capacity_bonus_per_player\":2500.0,"
	"      \"requires_building_online\":true}} }";

int	Run_Power_Grid (void)
{
	BuildingDescriptorManager::Init ();
	TeamPowerGridService::Init ();
	WorldShroudManager::Init ();
	WorldShroudManager::Re_Partition (Vector3 (-1000.0f,-1000.0f,0.0f),Vector3 (1000.0f,1000.0f,0.0f));

	StringClass error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_plant",GRID_GENERATOR_JSON,&error),"plant descriptor refused");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_hi",GRID_CONSUMER_HI_JSON,&error),"hi descriptor refused");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_lo",GRID_CONSUMER_LO_JSON,&error),"lo descriptor refused");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_comms",GRID_COMMS_JSON,&error),"comms descriptor refused");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_silo",GRID_SILO_JSON,&error),"silo descriptor refused");

	const int TEAM0 = 0;

	/*
	**	Finite generation/demand, and deterministic overload shedding by priority.
	*/
	Check (TeamPowerGridService::RegisterBuilding (nullptr,TEAM0,"ow_test_grid_plant",1),"registering member 1 was refused");
	TeamPowerGridService::RecalculateTeam (TEAM0);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Generation (TEAM0) - 500.0f) < 0.01f,
			 "one 500-generation plant reports generation %f",TeamPowerGridService::Get_Generation (TEAM0));
	Check (TeamPowerGridService::Is_Member_Powered (1),"a lone generator with nothing shedding it was not powered");

	TeamPowerGridService::RegisterBuilding (nullptr,TEAM0,"ow_test_grid_hi",2);		// priority 80, 300
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM0,"ow_test_grid_lo",3);		// priority 20, 300
	TeamPowerGridService::RecalculateTeam (TEAM0);

	Check (WWMath::Fabs (TeamPowerGridService::Get_Requested_Demand (TEAM0) - 600.0f) < 0.01f,
			 "two 300-consumption consumers requested %f, not 600",TeamPowerGridService::Get_Requested_Demand (TEAM0));
	Check (WWMath::Fabs (TeamPowerGridService::Get_Allocated_Demand (TEAM0) - 300.0f) < 0.01f,
			 "500 available against 600 requested allocated %f, not 300",TeamPowerGridService::Get_Allocated_Demand (TEAM0));
	Check (TeamPowerGridService::Is_Member_Powered (2),"the higher-priority consumer was shed instead of the lower one");
	Check (!TeamPowerGridService::Is_Member_Powered (3),"the lower-priority consumer was powered past the available budget");
	Check (TeamPowerGridService::Get_Shed_Count (TEAM0) == 1,"overload shed %d consumer(s), not 1",TeamPowerGridService::Get_Shed_Count (TEAM0));
	Check (TeamPowerGridService::Get_Powered_Count (TEAM0) == 2,
			 "%d member(s) powered, not the plant plus the one winning consumer",TeamPowerGridService::Get_Powered_Count (TEAM0));

	/*
	**	Reduced demand restores a previously shed consumer.
	*/
	TeamPowerGridService::OnBuildingDestroyed (2);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Requested_Demand (TEAM0) - 300.0f) < 0.01f,
			 "destroying the winning consumer left requested demand at %f, not 300",
			 TeamPowerGridService::Get_Requested_Demand (TEAM0));
	Check (TeamPowerGridService::Is_Member_Powered (3),
			 "reduced demand did not restore the previously shed consumer");

	/*
	**	Destroying the final generator powers down every dependent building; reviving it
	**	restores them.
	*/
	TeamPowerGridService::OnBuildingDestroyed (1);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Generation (TEAM0)) < 0.01f,
			 "the only generator destroyed still reports generation %f",TeamPowerGridService::Get_Generation (TEAM0));
	Check (!TeamPowerGridService::Is_Member_Powered (3),
			 "a consumer stayed powered after the final generator was destroyed");

	TeamPowerGridService::OnBuildingRevived (1);
	Check (TeamPowerGridService::Is_Member_Powered (3),
			 "reviving the only generator did not restore a waiting consumer");

	/*
	**	Manual on/off is authoritative and takes effect immediately.
	*/
	TeamPowerGridService::Set_Manual_Disabled (3,true);
	Check (!TeamPowerGridService::Is_Member_Powered (3),"a manually disabled consumer still reads powered");
	Check (WWMath::Fabs (TeamPowerGridService::Get_Requested_Demand (TEAM0)) < 0.01f,
			 "a manually disabled consumer still counted toward requested demand: %f",
			 TeamPowerGridService::Get_Requested_Demand (TEAM0));
	Check (TeamPowerGridService::Get_Manual_Off_Count (TEAM0) == 1,"manual-off count read %d, not 1",
			 TeamPowerGridService::Get_Manual_Off_Count (TEAM0));

	TeamPowerGridService::Set_Manual_Disabled (3,false);
	Check (TeamPowerGridService::Is_Member_Powered (3),"clearing manual-disable did not restore power immediately");

	/*
	**	Priority override changes allocation ordering immediately, and clearing it reverts to
	**	the descriptor's own priority with the deterministic MemberId tie-break.
	*/
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM0,"ow_test_grid_lo",4);		// same priority 20 as member 3
	TeamPowerGridService::RecalculateTeam (TEAM0);
	Check (TeamPowerGridService::Is_Member_Powered (3) && !TeamPowerGridService::Is_Member_Powered (4),
			 "two equal-priority consumers did not break the tie by the lower MemberId");

	TeamPowerGridService::Set_Priority_Override (4,100);
	Check (TeamPowerGridService::Is_Member_Powered (4) && !TeamPowerGridService::Is_Member_Powered (3),
			 "a priority override did not immediately change which consumer wins");

	TeamPowerGridService::Clear_Priority_Override (4);
	Check (TeamPowerGridService::Is_Member_Powered (3) && !TeamPowerGridService::Is_Member_Powered (4),
			 "clearing a priority override did not revert to the descriptor priority and MemberId tie-break");

	/*
	**	Communications coverage follows online state -- Section 30's own sensor-source API,
	**	not a second shroud implementation.
	*/
	const int TEAM1 = 1;
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM1,"ow_test_grid_plant",10);
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM1,"ow_test_grid_comms",11);
	TeamPowerGridService::RecalculateTeam (TEAM1);
	WorldShroudManager::Timestep (0.1f);

	Vector3 comms_position (0.0f,0.0f,0.0f);		// members registered with a null Building default here
	Check (WorldShroudManager::Get_Shroud_State (TEAM1,comms_position) == SHROUD_VISIBLE,
			 "an online communications building did not register live shroud coverage");

	TeamPowerGridService::OnBuildingDestroyed (11);
	WorldShroudManager::Timestep (0.1f);
	Check (WorldShroudManager::Get_Shroud_State (TEAM1,comms_position) != SHROUD_VISIBLE,
			 "coverage stayed VISIBLE after its communications building was destroyed");

	TeamPowerGridService::OnBuildingRevived (11);
	WorldShroudManager::Timestep (0.1f);
	Check (WorldShroudManager::Get_Shroud_State (TEAM1,comms_position) == SHROUD_VISIBLE,
			 "reviving the communications building did not restore live shroud coverage");

	/*
	**	Silo wallet bonuses stack, and stop contributing when destroyed, manually disabled, or
	**	powered down by the final generator's loss -- proving the bonus is a live function of
	**	grid state, not a value fixed at registration.
	*/
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM1,"ow_test_grid_silo",20);
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM1,"ow_test_grid_silo",21);
	TeamPowerGridService::RecalculateTeam (TEAM1);

	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity_Bonus (TEAM1) - 5000.0f) < 0.01f,
			 "two stacked Silos report a wallet bonus of %f, not 5000",TeamPowerGridService::Get_Wallet_Capacity_Bonus (TEAM1));

	TeamPowerGridService::Set_Manual_Disabled (20,true);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity_Bonus (TEAM1) - 2500.0f) < 0.01f,
			 "disabling one of two Silos left the bonus at %f, not 2500",TeamPowerGridService::Get_Wallet_Capacity_Bonus (TEAM1));

	TeamPowerGridService::OnBuildingDestroyed (10);		// team 1's only generator
	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity_Bonus (TEAM1)) < 0.01f,
			 "destroying the only generator left a Silo bonus of %f, not 0",TeamPowerGridService::Get_Wallet_Capacity_Bonus (TEAM1));

	/*
	**	Q-014's cap: base plus the live Silo bonus, and a base that can be retuned at runtime.
	**	The sweep that wipes banked credits needs real players and a server, neither of which a
	**	self check has, so what is proved here is the number every part of the sweep reads.
	*/
	Check (WWMath::Fabs (TeamPowerGridService::DEFAULT_BASE_WALLET_CAP - 10000.0f) < 0.01f,
			 "the default base wallet cap is %f, not 10000",TeamPowerGridService::DEFAULT_BASE_WALLET_CAP);

	TeamPowerGridService::Set_Base_Wallet_Cap (TeamPowerGridService::DEFAULT_BASE_WALLET_CAP);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity (TEAM1) - 10000.0f) < 0.01f,
			 "a team with no live Silo has an effective cap of %f, not the bare base 10000",
			 TeamPowerGridService::Get_Wallet_Capacity (TEAM1));

	//	Bring the generator back: both Silos power up again and the cap rises with them.
	TeamPowerGridService::OnBuildingRevived (10);
	TeamPowerGridService::Set_Manual_Disabled (20,false);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity (TEAM1) - 15000.0f) < 0.01f,
			 "two live Silos give an effective cap of %f, not 10000 + 5000",
			 TeamPowerGridService::Get_Wallet_Capacity (TEAM1));

	//	Losing one Silo drops the ceiling by exactly that Silo's declared bonus.  This is the
	//	state change the credit wipe hangs off, so it is the one that has to be exact.
	TeamPowerGridService::OnBuildingDestroyed (21);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity (TEAM1) - 12500.0f) < 0.01f,
			 "losing one of two Silos left the cap at %f, not 12500",
			 TeamPowerGridService::Get_Wallet_Capacity (TEAM1));

	TeamPowerGridService::Set_Base_Wallet_Cap (0.0f);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Wallet_Capacity (TEAM1) - 2500.0f) < 0.01f,
			 "with the base retuned to zero the cap is %f, not the surviving Silo's 2500",
			 TeamPowerGridService::Get_Wallet_Capacity (TEAM1));

	TeamPowerGridService::Set_Base_Wallet_Cap (-1.0f);
	Check (TeamPowerGridService::Get_Base_Wallet_Cap () == 0.0f,
			 "a negative base wallet cap was accepted (%f)",TeamPowerGridService::Get_Base_Wallet_Cap ());

	TeamPowerGridService::Set_Base_Wallet_Cap (TeamPowerGridService::DEFAULT_BASE_WALLET_CAP);
	TeamPowerGridService::OnBuildingDestroyed (10);		// restore the state the rest expects
	TeamPowerGridService::Set_Manual_Disabled (20,true);

	/*
	**	Live JSON reload recalculates correctly, through OnDescriptorReload.
	*/
	const int TEAM2 = 2;
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_reload_plant",
				"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
				"  \"power\":{\"requires_power\":false,\"generation\":200.0,\"consumption\":0.0,\"priority\":50} }",
				&error),
			 "the reload-test plant descriptor was refused");

	TeamPowerGridService::RegisterBuilding (nullptr,TEAM2,"ow_test_grid_reload_plant",30);
	TeamPowerGridService::RecalculateTeam (TEAM2);
	Check (WWMath::Fabs (TeamPowerGridService::Get_Generation (TEAM2) - 200.0f) < 0.01f,
			 "the reload-test plant reports generation %f before any reload, not 200",TeamPowerGridService::Get_Generation (TEAM2));

	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_grid_reload_plant",
				"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":10,\"depth\":10},"
				"  \"power\":{\"requires_power\":false,\"generation\":999.0,\"consumption\":0.0,\"priority\":50} }",
				&error),
			 "the edited reload-test plant descriptor was refused");
	TeamPowerGridService::OnDescriptorReload ("ow_test_grid_reload_plant");
	Check (WWMath::Fabs (TeamPowerGridService::Get_Generation (TEAM2) - 999.0f) < 0.01f,
			 "OnDescriptorReload left generation at %f, not the edited value 999",TeamPowerGridService::Get_Generation (TEAM2));

	WorldShroudManager::Shutdown ();
	TeamPowerGridService::Shutdown ();
	BuildingDescriptorManager::Shutdown ();

	return _Failures;
}


/*
**	Commander Mode camera / embodied player -- roadmap Section 41.
**
**	Enter_Commander_Mode never dereferences the star it is handed (see the header comment), so
**	a sentinel non-null pointer exercises embodiment/refusal logic safely with no real
**	SoldierGameObj, no device and no scene.  Pick_At_Cursor is not called here: it reaches
**	WorldSpatialIndex::Query_Ray, which this check has not verified is safe against a scene
**	that does not exist -- see the design doc's "not covered" section.
*/
int	Run_Commander_Mode (void)
{
	CommanderModeController::Init ();

	Check (!CommanderModeController::Is_Active (),"a freshly initialised controller is already active");
	Check (CommanderModeController::Get_Mode () == FPS_MODE,"a freshly initialised controller is not FPS_MODE");

	/*
	**	Embodiment: refuses a null star -- there is nothing to embody -- and never spawns
	**	anything of its own.
	*/
	Check (!CommanderModeController::Enter_Commander_Mode (nullptr),
			 "entering Commander Mode with no star to embody was accepted");
	Check (!CommanderModeController::Is_Active (),"a refused Enter_Commander_Mode left the controller active");

	SoldierGameObj * sentinel_star = reinterpret_cast<SoldierGameObj *> (1);	// never dereferenced
	Check (CommanderModeController::Enter_Commander_Mode (sentinel_star),
			 "entering Commander Mode with a living star was refused");
	Check (CommanderModeController::Is_Active (),"Enter_Commander_Mode succeeded but Is_Active is false");
	Check (CommanderModeController::Get_Mode () == COMMANDER_MODE,"Get_Mode is not COMMANDER_MODE after entering");
	Check (CommanderModeController::Get_Embodied_Star () == sentinel_star,
			 "Get_Embodied_Star did not return the same body that entered -- \"same player body\"");

	Check (!CommanderModeController::Enter_Commander_Mode (sentinel_star),
			 "entering Commander Mode a second time while already active was accepted");

	/*
	**	Bounds: pan and zoom clamp to configured ranges; rotation wraps instead.
	*/
	CommanderModeController::Set_World_Bounds (Vector3 (-100.0f,-100.0f,-100.0f),Vector3 (100.0f,100.0f,100.0f));
	CommanderModeController::Set_Focus (Vector3 (0.0f,0.0f,0.0f));
	CommanderModeController::Pan (Vector3 (10.0f,20.0f,0.0f));
	Check ((CommanderModeController::Get_Focus () - Vector3 (10.0f,20.0f,0.0f)).Length () < 0.01f,
			 "a pan inside world bounds did not move the focus by the expected amount");

	CommanderModeController::Pan (Vector3 (1000.0f,0.0f,0.0f));
	Check (WWMath::Fabs (CommanderModeController::Get_Focus ().X - 100.0f) < 0.01f,
			 "panning far past the world bounds left focus.X at %f, not clamped to 100",
			 CommanderModeController::Get_Focus ().X);

	CommanderModeController::Set_Zoom_Range (50.0f,500.0f);
	CommanderModeController::Set_Zoom (1000.0f);
	Check (WWMath::Fabs (CommanderModeController::Get_Zoom () - 500.0f) < 0.01f,
			 "zoom past its maximum read back %f, not clamped to 500",CommanderModeController::Get_Zoom ());
	CommanderModeController::Zoom (-10000.0f);
	Check (WWMath::Fabs (CommanderModeController::Get_Zoom () - 50.0f) < 0.01f,
			 "zoom past its minimum read back %f, not clamped to 50",CommanderModeController::Get_Zoom ());

	CommanderModeController::Set_Tilt_Range (10.0f,80.0f);
	CommanderModeController::Set_Tilt (200.0f);
	Check (WWMath::Fabs (CommanderModeController::Get_Tilt () - 80.0f) < 0.01f,
			 "tilt past its maximum read back %f, not clamped to 80",CommanderModeController::Get_Tilt ());

	CommanderModeController::Set_Rotation (400.0f);
	Check (WWMath::Fabs (CommanderModeController::Get_Rotation () - 40.0f) < 0.01f,
			 "rotation of 400 degrees did not wrap to 40, read back %f",CommanderModeController::Get_Rotation ());
	CommanderModeController::Set_Rotation (-10.0f);
	Check (WWMath::Fabs (CommanderModeController::Get_Rotation () - 350.0f) < 0.01f,
			 "rotation of -10 degrees did not wrap to 350, read back %f",CommanderModeController::Get_Rotation ());

	/*
	**	Compute_Camera_Transform is a pure, checkable function: the camera sits exactly Zoom
	**	world units from Focus (the orbit-camera radius), and its three axes are an orthonormal
	**	basis -- a real, valid transform, not merely "some matrix".
	*/
	CommanderModeController::Set_World_Bounds (Vector3 (-100000.0f,-100000.0f,-100000.0f),
																 Vector3 ( 100000.0f, 100000.0f, 100000.0f));
	CommanderModeController::Set_Focus (Vector3 (0.0f,0.0f,0.0f));
	CommanderModeController::Set_Zoom_Range (1.0f,1000.0f);
	CommanderModeController::Set_Zoom (100.0f);
	CommanderModeController::Set_Tilt_Range (0.0f,90.0f);
	CommanderModeController::Set_Tilt (45.0f);
	CommanderModeController::Set_Rotation (0.0f);

	Matrix3D transform = CommanderModeController::Compute_Camera_Transform ();
	Vector3 translation = transform.Get_Translation ();

	Check (WWMath::Fabs (translation.Length () - 100.0f) < 0.1f,
			 "a camera at zoom 100 sits %f world units from the focus, not 100",translation.Length ());
	Check (translation.Z > 0.0f,"a positive-tilt camera did not end up above the focus plane");

	Vector3 x_axis = transform.Get_X_Vector ();
	Vector3 y_axis = transform.Get_Y_Vector ();
	Vector3 z_axis = transform.Get_Z_Vector ();
	Check (WWMath::Fabs (x_axis.Length () - 1.0f) < 0.01f,"the transform's X axis is not unit length");
	Check (WWMath::Fabs (y_axis.Length () - 1.0f) < 0.01f,"the transform's Y axis is not unit length");
	Check (WWMath::Fabs (z_axis.Length () - 1.0f) < 0.01f,"the transform's Z axis is not unit length");
	Check (WWMath::Fabs (Vector3::Dot_Product (x_axis,y_axis)) < 0.01f,"the transform's X and Y axes are not orthogonal");
	Check (WWMath::Fabs (Vector3::Dot_Product (y_axis,z_axis)) < 0.01f,"the transform's Y and Z axes are not orthogonal");
	Check (WWMath::Fabs (Vector3::Dot_Product (x_axis,z_axis)) < 0.01f,"the transform's X and Z axes are not orthogonal");

	/*
	**	Death immediately exits Commander Mode -- star_is_alive is the one fact Update needs
	**	and does not look up itself.
	*/
	Check (CommanderModeController::Is_Active (),"setup before the death check was not left active");
	CommanderModeController::Update (0.1f,true);
	Check (CommanderModeController::Is_Active (),"Update with a living star exited Commander Mode");

	CommanderModeController::Update (0.1f,false);
	Check (!CommanderModeController::Is_Active (),
			 "Update with star_is_alive == false did not exit Commander Mode");
	Check (CommanderModeController::Get_Mode () == FPS_MODE,"death-exit did not return to FPS_MODE");
	Check (CommanderModeController::Get_Embodied_Star () == nullptr,
			 "death-exit left a stale embodied star behind");

	/*
	**	Update on an already-inactive controller is a safe no-op, not a crash -- the
	**	"safe to call when already inactive" contract Exit_Commander_Mode's own doc promises.
	*/
	CommanderModeController::Update (0.1f,true);
	Check (!CommanderModeController::Is_Active (),"Update while inactive somehow re-activated Commander Mode");

	CommanderModeController::Shutdown ();
	Check (!CommanderModeController::Is_Active (),"Commander Mode outlived Shutdown");

	return _Failures;
}


/***********************************************************************************************
 * Commander sidebar, placement, construction, production (Phases 36-39) and RTS save/load (40)  *
 *                                                                                             *
 *	All five are device-less by construction, which is the whole reason they were built as       *
 *	state-and-rules services with the drawing left outside them.  What cannot be checked here    *
 *	is named in each block rather than quietly skipped.                                          *
 *=============================================================================================*/

static const char * const	SIDEBAR_FACTORY_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":20,\"depth\":30},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":100.0,\"priority\":50},"
	"  \"production\":{\"kind\":\"ground_vehicle\",\"queue_capacity\":3,\"requires_power\":true},"
	"  \"build\":{\"buildable\":true,\"cost\":2000,\"build_time_seconds\":10.0,\"display_name\":\"Factory\"} }";

static const char * const	SIDEBAR_BARRACKS_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":15,\"depth\":15},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":50.0,\"priority\":50},"
	"  \"production\":{\"kind\":\"infantry\",\"queue_capacity\":5,\"requires_power\":true} }";

static const char * const	SIDEBAR_PLANT_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":18,\"depth\":18},"
	"  \"power\":{\"requires_power\":false,\"generation\":300.0,\"consumption\":0.0,\"priority\":0},"
	"  \"build\":{\"buildable\":true,\"cost\":1500,\"team\":1,\"display_name\":\"Power Plant\"},"
	"  \"placement_rules\":{\"max_slope_degrees\":12.0,\"exit_clearance\":0.0} }";

static const char * const	SIDEBAR_PLAIN_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":8,\"depth\":8},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":10.0,\"priority\":10} }";


int	Run_Commander_Sidebar (void)
{
	BuildingDescriptorManager::Init ();
	TeamPowerGridService::Init ();
	BuildingConstructionService::Init ();
	CommanderProductionService::Init ();
	CommanderSidebarModel::Init ();

	StringClass error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_sb_factory",SIDEBAR_FACTORY_JSON,&error),
			 "factory descriptor refused: %s",error.Peek_Buffer ());
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_sb_barracks",SIDEBAR_BARRACKS_JSON,&error),
			 "barracks descriptor refused: %s",error.Peek_Buffer ());
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_sb_plant",SIDEBAR_PLANT_JSON,&error),
			 "plant descriptor refused: %s",error.Peek_Buffer ());
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_sb_plain",SIDEBAR_PLAIN_JSON,&error),
			 "plain descriptor refused: %s",error.Peek_Buffer ());

	const int TEAM_GDI = 1;

	//	"BUILDINGS sidebar appears on Commander entry", and it is the default mode.
	Check (!CommanderSidebarModel::Is_Open (),"the sidebar was open before anything opened it");
	CommanderSidebarModel::Open (TEAM_GDI);
	Check (CommanderSidebarModel::Is_Open (),"Open did not open the sidebar");
	Check (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_BUILDINGS,
			 "the sidebar opened in %s, not BUILDINGS",
			 CommanderSidebarModel::Get_Mode_Name (CommanderSidebarModel::Get_Mode ()));

	/*
	**	Only buildable descriptors become building cards, and a card carries the descriptor's
	**	own cost and power numbers -- Section 42's "+generation / -consumption" on the card.
	*/
	CommanderSidebarModel::Refresh (100000);
	Check (CommanderSidebarModel::Get_Card_Count () == 2,
			 "%d building card(s) from two buildable descriptors and two unbuildable ones",
			 CommanderSidebarModel::Get_Card_Count ());

	bool found_factory = false, found_plant = false;
	for (int i = 0; i < CommanderSidebarModel::Get_Card_Count (); i++) {
		const CommanderSidebarCard * card = CommanderSidebarModel::Get_Card (i);
		Check (card != nullptr,"a card index inside the count returned nullptr");
		if (card == nullptr) continue;
		if (card->DescriptorName.Compare_No_Case ("ow_test_sb_factory") == 0) {
			found_factory = true;
			Check (card->Cost == 2000,"the factory card cost %d, not its descriptor's 2000",card->Cost);
			Check (WWMath::Fabs (card->PowerConsumption - 100.0f) < 0.01f,
					 "the factory card shows consumption %f, not 100",card->PowerConsumption);
			Check (card->Status == CARD_AVAILABLE,"an affordable any-faction building read '%s'",
					 CommanderSidebarModel::Get_Status_Name (card->Status));
		}
		if (card->DescriptorName.Compare_No_Case ("ow_test_sb_plant") == 0) {
			found_plant = true;
			Check (WWMath::Fabs (card->PowerGeneration - 300.0f) < 0.01f,
					 "the plant card shows generation %f, not 300",card->PowerGeneration);
		}
	}
	Check (found_factory,"the buildable factory produced no card");
	Check (found_plant,"the buildable plant produced no card");

	//	Affordability is a card status, not a missing card -- the panel has to draw the reason.
	CommanderSidebarModel::Refresh (100);
	Check (CommanderSidebarModel::Get_Card_Count () == 2,
			 "a poor Commander sees %d cards instead of the same two greyed out",
			 CommanderSidebarModel::Get_Card_Count ());
	for (int i = 0; i < CommanderSidebarModel::Get_Card_Count (); i++) {
		const CommanderSidebarCard * card = CommanderSidebarModel::Get_Card (i);
		if (card == nullptr) continue;
		Check (card->Status == CARD_INSUFFICIENT_CREDITS,
				 "a 1500-2000 credit building with 100 credits read '%s'",
				 CommanderSidebarModel::Get_Status_Name (card->Status));
	}

	//	A faction-locked descriptor on the wrong team: same card, a different reason -- which is
	//	what distinguishes "cannot afford yet" from "cannot ever".
	CommanderSidebarModel::Open (0);				// PLAYERTYPE_NOD
	CommanderSidebarModel::Refresh (100000);
	bool saw_wrong_faction = false;
	for (int i = 0; i < CommanderSidebarModel::Get_Card_Count (); i++) {
		const CommanderSidebarCard * card = CommanderSidebarModel::Get_Card (i);
		if (card == nullptr) continue;
		if (card->DescriptorName.Compare_No_Case ("ow_test_sb_plant") == 0) {
			saw_wrong_faction = (card->Status == CARD_WRONG_FACTION);
		}
	}
	Check (saw_wrong_faction,"a GDI-only building did not read WRONG_FACTION for Nod");

	//	Paging: eight visible slots, a page count that follows the content, clamped at both ends.
	CommanderSidebarModel::Open (TEAM_GDI);
	CommanderSidebarModel::Refresh (100000);
	Check (CommanderSidebarModel::PAGE_SIZE == 8,"the visible page is %d slots, not the required 2 x 4",
			 (int)CommanderSidebarModel::PAGE_SIZE);
	Check (CommanderSidebarModel::Get_Page_Count () == 1,
			 "two cards produced %d pages",CommanderSidebarModel::Get_Page_Count ());
	CommanderSidebarModel::Next_Page ();
	Check (CommanderSidebarModel::Get_Page () == 0,"Next_Page on the only page moved to page %d",
			 CommanderSidebarModel::Get_Page ());
	CommanderSidebarModel::Prev_Page ();
	Check (CommanderSidebarModel::Get_Page () == 0,"Prev_Page on page 0 moved to page %d",
			 CommanderSidebarModel::Get_Page ());

	const CommanderSidebarCard * visible [CommanderSidebarModel::PAGE_SIZE];
	int visible_count = CommanderSidebarModel::Get_Visible_Cards (visible,CommanderSidebarModel::PAGE_SIZE);
	Check (visible_count == 2,"a two-card page reported %d visible cards",visible_count);

	//	A bind that cannot resolve a descriptor changes nothing at all.
	Check (!CommanderSidebarModel::Bind_Facility (nullptr,0),
			 "binding to a null building with no descriptor was accepted");
	Check (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_BUILDINGS,
			 "a refused bind still changed the mode");

	/*
	**	Not covered here: Bind_Facility's success path needs a real BuildingGameObj, because the
	**	descriptor name it binds by is read off BuildingGameObjDef.  The rules that binding
	**	drives -- the facility gate below, and the mode each production kind maps to -- are
	**	exercised directly instead.
	*/
	CommanderSidebarModel::Set_Mode (SIDEBAR_MODE_INFANTRY);
	Check (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_INFANTRY,"Set_Mode to INFANTRY did not take");
	CommanderSidebarModel::Refresh (100000);

	/*
	**	"Never allow stale purchases from a dead/offline facility."  With nothing bound, no unit
	**	card may read AVAILABLE.  (A device-less run also has no purchase-page content, so an
	**	empty list is equally a pass; what is checked is that nothing is buyable.)
	*/
	for (int i = 0; i < CommanderSidebarModel::Get_Card_Count (); i++) {
		const CommanderSidebarCard * card = CommanderSidebarModel::Get_Card (i);
		if (card == nullptr) continue;
		Check (card->Status != CARD_AVAILABLE,"a unit card read AVAILABLE with no facility bound");
	}
	Check (!CommanderSidebarModel::Get_Facility_State ().Is_Operational (),
			 "an unbound facility state reported itself operational");

	//	Return to BUILDINGS is immediate and drops the binding.
	CommanderSidebarModel::Return_To_Buildings ();
	Check (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_BUILDINGS,"the return input did not reach BUILDINGS");
	Check (!CommanderSidebarModel::Get_Facility_State ().Bound,"the return input left a facility bound");

	/*
	**	AIR_VEHICLES exists, switches and pages, and is empty because stock Renegade ships no
	**	air purchase page.  Checked so the emptiness is a recorded fact, not a surprise.
	*/
	CommanderSidebarModel::Set_Mode (SIDEBAR_MODE_AIR_VEHICLES);
	CommanderSidebarModel::Refresh (100000);
	Check (CommanderSidebarModel::Get_Card_Count () == 0,
			 "AIR_VEHICLES produced %d cards, but no air purchase content exists in this tree",
			 CommanderSidebarModel::Get_Card_Count ());
	Check (CommanderSidebarModel::Get_Page_Count () == 1,"an empty mode reported %d pages, not one empty one",
			 CommanderSidebarModel::Get_Page_Count ());

	//	Power bars read the grid, and overload is reported separately from headroom.
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM_GDI,"ow_test_sb_plant",101);
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM_GDI,"ow_test_sb_factory",102);
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM_GDI,"ow_test_sb_barracks",103);
	TeamPowerGridService::RecalculateTeam (TEAM_GDI);

	CommanderSidebarModel::Open (TEAM_GDI);
	CommanderPowerDisplay power;
	CommanderSidebarModel::Get_Power_Display (&power);
	Check (WWMath::Fabs (power.Generation - 300.0f) < 0.01f,
			 "the power bar shows generation %f, not the grid's 300",power.Generation);
	Check (WWMath::Fabs (power.RequestedDemand - 150.0f) < 0.01f,
			 "the power bar shows requested demand %f, not 150",power.RequestedDemand);
	Check (!power.Overloaded,"300 generation against 150 demand read as overloaded");
	Check (WWMath::Fabs (power.Headroom - 150.0f) < 0.01f,"headroom read %f, not 150",power.Headroom);

	//	Overloaded, and the bar says so rather than showing negative headroom.
	TeamPowerGridService::OnBuildingDestroyed (101);
	CommanderSidebarModel::Get_Power_Display (&power);
	Check (power.Overloaded,"demand with no generation at all did not read as overloaded");
	Check (power.Headroom >= 0.0f,"headroom went negative (%f) instead of reporting an overload",power.Headroom);
	Check (power.ShedCount == 2,"%d building(s) shed with no generation, not both consumers",power.ShedCount);

	CommanderSidebarModel::Close ();
	Check (!CommanderSidebarModel::Is_Open (),"Close left the sidebar open");
	Check (CommanderSidebarModel::Get_Card_Count () == 0,"Close left cards behind");

	CommanderSidebarModel::Shutdown ();
	CommanderProductionService::Shutdown ();
	BuildingConstructionService::Shutdown ();
	TeamPowerGridService::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	return _Failures;
}


int	Run_Commander_Placement (void)
{
	BuildingDescriptorManager::Init ();
	CommanderPlacementValidator::Init ();

	StringClass error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_pl_plant",SIDEBAR_PLANT_JSON,&error),
			 "plant descriptor refused: %s",error.Peek_Buffer ());
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_pl_plain",SIDEBAR_PLAIN_JSON,&error),
			 "plain descriptor refused: %s",error.Peek_Buffer ());

	const int TEAM_GDI = 1;
	const int TEAM_NOD = 0;

	CommanderPlacementRequest request;
	request.DescriptorName	= "ow_test_pl_plant";
	request.Team				= TEAM_GDI;
	request.Position			= Vector3 (0.0f,0.0f,0.0f);
	request.SequenceId		= 7;

	/*
	**	With no physics scene and no heightfield, every world check is skipped and the request
	**	is valid.  That is both the right answer and the right baseline: it means each refusal
	**	below is caused by the one thing being tested, not by the empty world.
	*/
	CommanderPlacementResult result = CommanderPlacementValidator::Validate (request);
	Check (result.Is_Valid (),"an unobstructed placement was refused: %s",
			 CommanderPlacementValidator::Get_Result_Name (result.Result));
	Check (result.SequenceId == 7,"the result carried sequence %d, not the request's 7",result.SequenceId);

	CommanderPlacementRequest bad = request;
	bad.DescriptorName = "ow_test_pl_nosuch";
	Check (CommanderPlacementValidator::Validate (bad).Result == PLACEMENT_NO_DESCRIPTOR,
			 "an unknown descriptor was not refused as NO_DESCRIPTOR");

	bad = request;
	bad.DescriptorName = "ow_test_pl_plain";
	Check (CommanderPlacementValidator::Validate (bad).Result == PLACEMENT_NOT_BUILDABLE,
			 "a descriptor with no build block was not refused as NOT_BUILDABLE");

	bad = request;
	bad.Team = TEAM_NOD;
	Check (CommanderPlacementValidator::Validate (bad).Result == PLACEMENT_WRONG_FACTION,
			 "a GDI-only building placed by Nod was not refused as WRONG_FACTION");

	/*
	**	The footprint box is the descriptor's, rotated by the request -- and it is the same box
	**	the ghost draws, so a rotation has to change the basis, not only the number.
	*/
	OBBoxClass box;
	Check (CommanderPlacementValidator::Build_Footprint_Box (request,&box),
			 "a valid request produced no footprint box");
	Check (WWMath::Fabs (box.Extent.X - 9.0f) < 0.01f,"the footprint half-width is %f, not the descriptor's 9",box.Extent.X);
	Check (WWMath::Fabs (box.Extent.Y - 9.0f) < 0.01f,"the footprint half-depth is %f, not the descriptor's 9",box.Extent.Y);

	CommanderPlacementRequest rotated = request;
	rotated.RotationDegrees = 90.0f;
	OBBoxClass rotated_box;
	Check (CommanderPlacementValidator::Build_Footprint_Box (rotated,&rotated_box),
			 "a rotated request produced no footprint box");
	Vector3 x_axis = rotated_box.Basis.Get_X_Vector ();
	Check (WWMath::Fabs (x_axis.X) < 0.01f && WWMath::Fabs (x_axis.Y - 1.0f) < 0.01f,
			 "a 90-degree rotation left the footprint basis unrotated (X axis %f,%f)",x_axis.X,x_axis.Y);

	/*
	**	The ghost.  Section 43's "RED TRANSLUCENT" state is Is_Ghost_Valid coming back false, so
	**	what matters is that it tracks Validate without the caller having to ask.
	*/
	Check (!CommanderPlacementValidator::Is_Ghost_Active (),"a ghost existed before one was begun");
	CommanderPlacementValidator::Begin_Ghost ("ow_test_pl_plant",TEAM_GDI);
	Check (CommanderPlacementValidator::Is_Ghost_Active (),"Begin_Ghost did not start a ghost");
	Check (CommanderPlacementValidator::Is_Ghost_Valid (),"an unobstructed ghost read invalid");

	CommanderPlacementValidator::Move_Ghost (Vector3 (50.0f,25.0f,3.0f));
	Check (WWMath::Fabs (CommanderPlacementValidator::Get_Ghost ().Position.X - 50.0f) < 0.01f,
			 "Move_Ghost did not move the ghost");

	CommanderPlacementValidator::Rotate_Ghost (45.0f);
	Check (WWMath::Fabs (CommanderPlacementValidator::Get_Ghost ().RotationDegrees - 45.0f) < 0.01f,
			 "Rotate_Ghost left rotation at %f",CommanderPlacementValidator::Get_Ghost ().RotationDegrees);
	CommanderPlacementValidator::Rotate_Ghost (-90.0f);
	Check (WWMath::Fabs (CommanderPlacementValidator::Get_Ghost ().RotationDegrees - 315.0f) < 0.01f,
			 "rotating past zero gave %f, not a wrapped 315",
			 CommanderPlacementValidator::Get_Ghost ().RotationDegrees);

	//	A ghost of a definition the Commander may not build is active and permanently red.
	CommanderPlacementValidator::Begin_Ghost ("ow_test_pl_plain",TEAM_GDI);
	Check (CommanderPlacementValidator::Is_Ghost_Active (),"an unbuildable ghost did not start");
	Check (!CommanderPlacementValidator::Is_Ghost_Valid (),"an unbuildable ghost read valid");

	CommanderPlacementValidator::Cancel_Ghost ();
	Check (!CommanderPlacementValidator::Is_Ghost_Active (),"Cancel_Ghost left the ghost active");
	Check (!CommanderPlacementValidator::Is_Ghost_Valid (),"a cancelled ghost still read valid");

	/*
	**	Server authority.  The server runs the identical function, so a request the client would
	**	have drawn red is refused -- and the refusal is counted.
	*/
	int rejections_before = CommanderPlacementValidator::Get_Rejection_Count ();

	CommanderPlacementRequest server_bad = request;
	server_bad.Team			= TEAM_NOD;
	server_bad.SequenceId	= 99;
	CommanderPlacementResult server_result = CommanderPlacementValidator::Validate_Server_Request (server_bad);
	Check (!server_result.Is_Valid (),"the server accepted a placement the client's own validator refuses");
	Check (CommanderPlacementValidator::Get_Rejection_Count () == rejections_before + 1,
			 "a server rejection was not counted");
	Check (CommanderPlacementValidator::Get_Last_Accepted_Sequence () != 99,
			 "a rejected placement was recorded as the last accepted sequence");

	CommanderPlacementRequest server_good = request;
	server_good.SequenceId = 42;
	Check (CommanderPlacementValidator::Validate_Server_Request (server_good).Is_Valid (),
			 "the server refused a placement its own validator calls valid");
	Check (CommanderPlacementValidator::Get_Last_Accepted_Sequence () == 42,
			 "an accepted placement did not record its sequence id");

	/*
	**	Not covered by any automated check: the overlap, slope, world-bounds and exit-clearance
	**	branches all need a live PhysicsSceneClass or a generated WorldTerrainSystem heightfield,
	**	neither of which -shaderselfcheck can stand up.  What is proved here is that the
	**	definition-level rules hold, that the footprint box those world queries are handed is
	**	built correctly and rotates, and that client and server reach the same verdict.
	*/

	CommanderPlacementValidator::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	return _Failures;
}


static const char * const	CONSTRUCTION_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":20,\"depth\":20},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":50.0,\"priority\":50},"
	"  \"build\":{\"buildable\":true,\"cost\":1000,\"build_time_seconds\":20.0},"
	"  \"construction_visual\":{\"duration_seconds\":15.0,\"block_width\":2.0,\"block_depth\":2.0,"
	"     \"row_height\":1.5,\"source_reveal_start\":0.75,\"block_fade_start\":0.9,\"max_blocks\":2048} }";

//	Absurdly small blocks against a small cap, so the coarsening path is exercised, not assumed.
static const char * const	CONSTRUCTION_TINY_JSON =
	"{ \"model_name\":\"x.w3d\",\"schema_version\":1,\"footprint\":{\"width\":20,\"depth\":20},"
	"  \"power\":{\"requires_power\":true,\"generation\":0.0,\"consumption\":50.0,\"priority\":50},"
	"  \"build\":{\"buildable\":true,\"cost\":1000},"
	"  \"construction_visual\":{\"duration_seconds\":15.0,\"block_width\":0.1,\"block_depth\":0.1,"
	"     \"row_height\":0.1,\"source_reveal_start\":0.75,\"block_fade_start\":0.9,\"max_blocks\":64} }";


int	Run_Building_Construction (void)
{
	BuildingDescriptorManager::Init ();
	BuildingConstructionService::Init ();

	StringClass error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_con",CONSTRUCTION_JSON,&error),
			 "construction descriptor refused: %s",error.Peek_Buffer ());
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_con_tiny",CONSTRUCTION_TINY_JSON,&error),
			 "tiny-block descriptor refused: %s",error.Peek_Buffer ());

	//	build.build_time_seconds wins over construction_visual.duration_seconds when both exist.
	const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor ("ow_test_con");
	Check (descriptor != nullptr,"the construction descriptor did not come back");
	if (descriptor != nullptr) {
		Check (WWMath::Fabs (descriptor->Get_Build_Time_Seconds () - 20.0f) < 0.01f,
				 "build time resolved to %f, not the build block's 20",descriptor->Get_Build_Time_Seconds ());
	}

	AABoxClass bounds;
	bounds.Center = Vector3 (0.0f,0.0f,5.0f);
	bounds.Extent = Vector3 (10.0f,10.0f,5.0f);		// a 20 x 20 x 10 building

	/*
	**	Determinism.  The same (bounds, seed) generates the identical layout -- this is what a
	**	client, a late joiner and a load all rest on, and it is the one property of the
	**	generator that actually has to hold.
	*/
	DynamicVectorClass<BuildingConstructionBlock>	first;
	DynamicVectorClass<BuildingConstructionBlock>	second;
	int count_a = BuildingConstructionService::Generate_Blocks ("ow_test_con",bounds,12345,first);
	int count_b = BuildingConstructionService::Generate_Blocks ("ow_test_con",bounds,12345,second);

	Check (count_a > 0,"a 20 x 20 x 10 building generated no blocks");
	Check (count_a == count_b,"the same seed generated %d blocks then %d",count_a,count_b);

	bool identical = true;
	for (int i = 0; i < count_a && i < count_b; i++) {
		if (first [i].Row != second [i].Row) identical = false;
		if (first [i].Order != second [i].Order) identical = false;
		if (WWMath::Fabs (first [i].LocalCenter.X - second [i].LocalCenter.X) > 0.0001f) identical = false;
		if (WWMath::Fabs (first [i].LocalCenter.Z - second [i].LocalCenter.Z) > 0.0001f) identical = false;
		if (WWMath::Fabs (first [i].ProgressStart - second [i].ProgressStart) > 0.0001f) identical = false;
	}
	Check (identical,"the same seed produced two different layouts");

	//	A different seed reorders the blocks but must not change the geometry: the seed is a
	//	shuffle, not a shape.
	DynamicVectorClass<BuildingConstructionBlock>	other_seed;
	int count_c = BuildingConstructionService::Generate_Blocks ("ow_test_con",bounds,999,other_seed);
	Check (count_c == count_a,"a different seed changed the block count from %d to %d",count_a,count_c);

	bool order_differs = false;
	for (int i = 0; i < count_a && i < count_c; i++) {
		if (first [i].Order != other_seed [i].Order) { order_differs = true; break; }
	}
	Check (order_differs,"two different seeds produced the same within-row ordering");

	/*
	**	Rows are bottom-to-top and no block escapes its row's slice of the timeline, which is
	**	what makes "rows activate strictly bottom-to-top" true whatever the shuffle did.
	*/
	int rows_seen = 0;
	bool rows_ordered = true;
	bool windows_sane = true;
	bool row_separation = true;
	for (int i = 0; i < count_a; i++) {
		if (first [i].Row + 1 > rows_seen) rows_seen = first [i].Row + 1;
		if (i > 0 && first [i].Row < first [i - 1].Row) rows_ordered = false;
		if (first [i].ProgressStart < BuildingConstructionService::FOUNDATION_END - 0.0001f) windows_sane = false;
		if (first [i].ProgressEnd > BuildingConstructionService::ROWS_END + 0.0001f) windows_sane = false;
		if (first [i].ProgressEnd <= first [i].ProgressStart) windows_sane = false;
		if (i > 0 && first [i].Row > first [i - 1].Row &&
			 first [i].ProgressStart < first [i - 1].ProgressStart) row_separation = false;
	}
	Check (rows_ordered,"the generated block list is not ordered bottom-to-top by row");
	Check (windows_sane,"a block's progress window falls outside the foundation/rows staging band");
	Check (rows_seen > 1,"a 10-unit-tall building with 1.5-unit rows generated only %d row(s)",rows_seen);
	Check (row_separation,"a higher row began before a lower one");

	//	max_blocks is respected by coarsening, not by truncation: the count fits *and* the
	//	blocks got bigger, rather than the top of the building going missing.
	DynamicVectorClass<BuildingConstructionBlock>	capped;
	int capped_count = BuildingConstructionService::Generate_Blocks ("ow_test_con_tiny",bounds,1,capped);
	Check (capped_count > 0,"the capped descriptor generated nothing at all");
	Check (capped_count <= 64,"max_blocks 64 produced %d blocks",capped_count);
	if (capped_count > 0) {
		Check (capped [0].HalfExtent.X > 0.05f,
				 "the capped layout kept 0.1-wide blocks (half extent %f) instead of coarsening",
				 capped [0].HalfExtent.X);
	}

	/*
	**	Lifecycle and the gate.  A building is under construction until its duration has elapsed
	**	on the *server clock*, not after some number of Update calls.
	*/
	Matrix3D transform (1);
	transform.Set_Translation (Vector3 (100.0f,200.0f,0.0f));

	Check (!BuildingConstructionService::Is_Under_Construction (555),
			 "a building nobody ever started read as under construction");
	Check (BuildingConstructionService::Begin_Construction (555,"ow_test_con",transform,bounds,1000.0f,12345),
			 "Begin_Construction refused a valid start");
	Check (BuildingConstructionService::Is_Under_Construction (555),"a just-started building is not under construction");
	Check (BuildingConstructionService::Get_Block_Count (555) == count_a,
			 "the started construction holds %d blocks, not the %d its layout generates",
			 BuildingConstructionService::Get_Block_Count (555),count_a);

	float progress = -1.0f;
	Check (BuildingConstructionService::Get_Progress (555,&progress),"a live construction reported no progress");
	Check (WWMath::Fabs (progress) < 0.0001f,"a construction started at progress %f, not 0",progress);
	Check (BuildingConstructionService::Get_Stage (555) == CONSTRUCTION_STAGE_FOUNDATION,
			 "a new construction is in stage '%s', not foundation",
			 BuildingConstructionService::Get_Stage_Name (BuildingConstructionService::Get_Stage (555)));

	BuildingConstructionService::Update (1010.0f);
	BuildingConstructionService::Get_Progress (555,&progress);
	Check (WWMath::Fabs (progress - 0.5f) < 0.0001f,
			 "ten seconds into a twenty-second build reads progress %f, not 0.5",progress);
	Check (BuildingConstructionService::Get_Stage (555) == CONSTRUCTION_STAGE_BLOCKS,
			 "midway through the block rows the stage is '%s'",
			 BuildingConstructionService::Get_Stage_Name (BuildingConstructionService::Get_Stage (555)));
	Check (WWMath::Fabs (BuildingConstructionService::Get_Source_Reveal_Alpha (555)) < 0.0001f,
			 "the real model was already revealing at half progress");

	//	A frame-rate hitch changes nothing: server time is the only input.
	BuildingConstructionService::Update (1010.0f);
	BuildingConstructionService::Get_Progress (555,&progress);
	Check (WWMath::Fabs (progress - 0.5f) < 0.0001f,
			 "a second Update at the same server time moved progress to %f",progress);

	BuildingConstructionService::Update (1018.0f);		// 0.90
	Check (BuildingConstructionService::Get_Stage (555) == CONSTRUCTION_STAGE_REVEAL,
			 "past source_reveal_start the stage is '%s', not reveal",
			 BuildingConstructionService::Get_Stage_Name (BuildingConstructionService::Get_Stage (555)));
	Check (BuildingConstructionService::Get_Source_Reveal_Alpha (555) > 0.0f,
			 "the real model had not begun revealing past source_reveal_start");

	//	A visible block is somewhere real; a block whose row has not started reports invisible
	//	rather than being drawn at the origin.
	BuildingConstructionBlockRenderState render;
	Check (BuildingConstructionService::Get_Block_Render_State (555,0,&render),
			 "block 0 of a live construction has no render state");
	if (render.Visible) {
		Vector3 position = render.Transform.Get_Translation ();
		Check (WWMath::Fabs (position.X - 100.0f) < 60.0f && WWMath::Fabs (position.Y - 200.0f) < 60.0f,
				 "a block landed at %f,%f, nowhere near the building's 100,200",position.X,position.Y);
		Check (render.Alpha > 0.0f && render.Alpha <= 1.0f,"a visible block has alpha %f",render.Alpha);
		Check (render.HalfExtent.Z > 0.0f,"a visible block has no height");
	}

	BuildingConstructionService::Update (1020.0f);
	Check (BuildingConstructionService::Get_Completed_Count () == 1,
			 "%d completion(s) reported when one building finished",
			 BuildingConstructionService::Get_Completed_Count ());
	Check (BuildingConstructionService::Get_Completed_Building (0) == 555,
			 "the completion named building %d, not 555",BuildingConstructionService::Get_Completed_Building (0));
	Check (!BuildingConstructionService::Is_Under_Construction (555),
			 "a finished building still reads as under construction");
	Check (WWMath::Fabs (BuildingConstructionService::Get_Source_Reveal_Alpha (555) - 1.0f) < 0.0001f,
			 "a finished building's real model is not fully revealed");

	BuildingConstructionService::Clear_Completed ();
	Check (BuildingConstructionService::Get_Completed_Count () == 0,"Clear_Completed left completions behind");
	Check (BuildingConstructionService::Get_Block_Count (555) == 0,
			 "a consumed completion left %d temporary block(s) behind",
			 BuildingConstructionService::Get_Block_Count (555));

	/*
	**	Cancellation leaves nothing: "cancellation/destruction leaves no temporary state", and
	**	"no half-built object may provide power/radar/production/defense/wallet capacity" --
	**	which is these records being the only place that state lives.
	*/
	Check (BuildingConstructionService::Begin_Construction (777,"ow_test_con",transform,bounds,2000.0f,7),
			 "a second construction was refused");
	BuildingConstructionService::Update (2005.0f);
	Check (BuildingConstructionService::Get_Active_Count () == 1,"%d active construction(s), not 1",
			 BuildingConstructionService::Get_Active_Count ());

	BuildingConstructionService::Cancel_Construction (777);
	Check (!BuildingConstructionService::Is_Under_Construction (777),"a cancelled construction persists");
	Check (BuildingConstructionService::Get_Block_Count (777) == 0,"a cancelled construction kept its blocks");
	Check (BuildingConstructionService::Get_Active_Count () == 0,"a cancelled construction is still counted active");
	Check (BuildingConstructionService::Get_Completed_Count () == 0,
			 "a cancelled construction reported a completion");

	//	A late joiner rebuilds the identical layout from the replicated numbers alone.
	BuildingConstructionService::Begin_Construction (888,"ow_test_con",transform,bounds,3000.0f,4242);
	BuildingConstructionReplicationState state;
	Check (BuildingConstructionService::Get_Replication_State (888,&state),"no replication state for a live build");
	Check (state.Seed == 4242,"the replication state carries seed %u, not 4242",state.Seed);

	int original_blocks = BuildingConstructionService::Get_Block_Count (888);
	BuildingConstructionService::Cancel_Construction (888);
	Check (BuildingConstructionService::Begin_Construction_From_State (state,bounds),
			 "a late joiner could not rebuild from the replicated state");
	Check (BuildingConstructionService::Get_Block_Count (888) == original_blocks,
			 "a late joiner rebuilt %d blocks, not the original %d",
			 BuildingConstructionService::Get_Block_Count (888),original_blocks);

	Check (!BuildingConstructionService::Begin_Construction (999,"ow_test_nosuch",transform,bounds,0.0f,1),
			 "a construction of an unknown descriptor was accepted");

	//	Get_Active_Row is the input a batching renderer uses to split settled rows from the
	//	animating one.  The batcher itself is not written yet.
	BuildingConstructionService::Update (3000.0f);
	Check (BuildingConstructionService::Get_Active_Row (888) == -1,
			 "a construction still in its foundation stage named an active row");
	BuildingConstructionService::Update (3010.0f);
	int active_row = BuildingConstructionService::Get_Active_Row (888);
	Check (active_row >= 0 && active_row < BuildingConstructionService::Get_Row_Count (888),
			 "the active row %d is outside the building's %d rows",active_row,
			 BuildingConstructionService::Get_Row_Count (888));

	/*
	**	Not covered by any automated check: nothing here draws a block, so the batching itself
	**	-- one mesh for the settled rows plus one for the animating row -- is unverified, as is
	**	how the reveal actually looks.  See docs/zerohour/BuildingConstruction.md.
	*/

	BuildingConstructionService::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	return _Failures;
}


int	Run_Commander_Production (void)
{
	CommanderProductionService::Init ();

	const int TEAM_GDI	= 1;
	const int COMMANDER	= 4242;
	const int IMPOSTOR	= 77;
	const int FACTORY		= 900;
	const int BARRACKS	= 901;

	CommanderFacilityContext facility;
	facility.FacilityId				= FACTORY;
	facility.Team						= TEAM_GDI;
	facility.Alive						= true;
	facility.ConstructionComplete	= true;
	facility.Powered					= true;
	facility.Kind						= BUILDING_PRODUCTION_GROUND_VEHICLE;
	facility.QueueCapacity			= 3;
	facility.ExitClear				= true;

	int ticket = 0;

	/*
	**	Commander authority.  A team with no Commander produces nothing, and a player who is not
	**	the Commander is refused even once one exists.
	*/
	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_NO_AUTHORITY,"a team with no Commander accepted a purchase");

	CommanderProductionService::Set_Team_Commander (TEAM_GDI,COMMANDER);
	Check (CommanderProductionService::Request_Production (IMPOSTOR,facility,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_NO_AUTHORITY,"a player who is not the Commander was allowed to purchase");

	/*
	**	"Dead/offline facilities cannot produce invalid units."  Each state refuses with its own
	**	reason, because the sidebar has to be able to draw the reason.
	*/
	CommanderFacilityContext dead = facility;
	dead.Alive = false;
	Check (CommanderProductionService::Request_Production (COMMANDER,dead,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_FACILITY_DEAD,"a destroyed facility accepted a purchase");

	CommanderFacilityContext building_up = facility;
	building_up.ConstructionComplete = false;
	Check (CommanderProductionService::Request_Production (COMMANDER,building_up,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_FACILITY_CONSTRUCTING,"a facility still under construction accepted a purchase");

	CommanderFacilityContext dark = facility;
	dark.Powered = false;
	Check (CommanderProductionService::Request_Production (COMMANDER,dark,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_FACILITY_UNPOWERED,"an unpowered facility accepted a purchase");

	CommanderFacilityContext nothing = facility;
	nothing.Kind = BUILDING_PRODUCTION_NONE;
	Check (CommanderProductionService::Request_Production (COMMANDER,nothing,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_WRONG_FACILITY_KIND,"a building that produces nothing accepted a purchase");

	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,50,true,&ticket)
				== PRODUCTION_INSUFFICIENT_CREDITS,"a 100-credit unit was sold for 50 credits");

	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,10000,false,&ticket)
				== PRODUCTION_UNAVAILABLE,"a disabled purchase entry was accepted");

	//	The accepted path, and the per-facility queue cap.
	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_ACCEPTED,"a fully valid purchase was refused");
	Check (ticket != 0,"an accepted purchase returned no ticket");
	Check (CommanderProductionService::Get_Queue_Length (FACTORY) == 1,
			 "the queue holds %d item(s) after one purchase",CommanderProductionService::Get_Queue_Length (FACTORY));
	Check (CommanderProductionService::Get_Queued_Count (FACTORY,10) == 1,
			 "the queue reports %d of definition 10",CommanderProductionService::Get_Queued_Count (FACTORY,10));

	CommanderProductionService::Request_Production (COMMANDER,facility,11,100,5.0f,10000,true,&ticket);
	CommanderProductionService::Request_Production (COMMANDER,facility,12,100,5.0f,10000,true,&ticket);
	Check (CommanderProductionService::Get_Queue_Length (FACTORY) == 3,"three purchases did not make a queue of three");
	Check (CommanderProductionService::Is_Queue_Full (FACTORY,3),"a full queue did not read as full");
	Check (CommanderProductionService::Request_Production (COMMANDER,facility,13,100,5.0f,10000,true,&ticket)
				== PRODUCTION_QUEUE_FULL,"a fourth purchase got past a capacity of three");

	//	A second facility has its own queue -- Section 45's "do not silently redirect".
	CommanderFacilityContext barracks = facility;
	barracks.FacilityId		= BARRACKS;
	barracks.Kind				= BUILDING_PRODUCTION_INFANTRY;
	barracks.QueueCapacity	= 5;
	Check (CommanderProductionService::Request_Production (COMMANDER,barracks,20,50,2.0f,10000,true,&ticket)
				== PRODUCTION_ACCEPTED,"a second facility's first purchase was refused");
	Check (CommanderProductionService::Get_Queue_Length (BARRACKS) == 1,"the second facility's queue is not its own");
	Check (CommanderProductionService::Get_Queue_Length (FACTORY) == 3,
			 "purchasing at one facility changed another's queue length");

	//	One item at a time per facility, and the head item is the one that advances.
	CommanderProductionService::Update (3.0f);
	CommanderProductionItem item;
	Check (CommanderProductionService::Get_Queue_Item (FACTORY,0,&item),"the factory queue head is unreadable");
	Check (item.State == PRODUCTION_STATE_PRODUCING,"the head item reads '%s', not producing",
			 CommanderProductionService::Get_Item_State_Name (item.State));
	Check (CommanderProductionService::Get_Queue_Item (FACTORY,1,&item),"the second queue item is unreadable");
	Check (item.State == PRODUCTION_STATE_QUEUED,"the second item reads '%s' instead of waiting its turn",
			 CommanderProductionService::Get_Item_State_Name (item.State));

	CommanderProductionService::Update (3.0f);		// the head is now past its five seconds
	Check (CommanderProductionService::Get_Completed_Count () >= 1,"a finished item produced no completion");
	Check (CommanderProductionService::Get_Queue_Length (FACTORY) == 2,
			 "completing the head left a queue of %d",CommanderProductionService::Get_Queue_Length (FACTORY));

	CommanderProductionItem completed;
	Check (CommanderProductionService::Get_Completed_Item (0,&completed),"the completion is unreadable");
	Check (completed.DefinitionId == 10,"the completion named definition %d, not the first purchase's 10",
			 completed.DefinitionId);
	CommanderProductionService::Clear_Completed ();

	//	Power loss pauses; it does not cancel.  The queue survives and resumes where it was.
	CommanderProductionService::Set_Facility_Runtime_State (FACTORY,true,true,false,true);
	CommanderProductionService::Update (10.0f);
	Check (CommanderProductionService::Get_Queue_Length (FACTORY) == 2,
			 "a power cut cancelled the queue instead of pausing it");
	Check (CommanderProductionService::Get_Queue_Item (FACTORY,0,&item) &&
			 item.State == PRODUCTION_STATE_WAITING_FOR_POWER,
			 "an unpowered facility's head item does not read 'waiting for power'");
	Check (CommanderProductionService::Get_Completed_Count () == 0,
			 "an unpowered facility completed an item anyway");

	CommanderProductionService::Set_Facility_Runtime_State (FACTORY,true,true,true,true);
	CommanderProductionService::Update (5.0f);
	Check (CommanderProductionService::Get_Completed_Count () >= 1,"restoring power did not resume production");
	CommanderProductionService::Clear_Completed ();

	//	A blocked exit holds the finished unit, and the line behind it does not pass it.
	CommanderProductionService::Set_Facility_Runtime_State (FACTORY,true,true,true,false);
	CommanderProductionService::Update (60.0f);
	Check (CommanderProductionService::Get_Completed_Count () == 0,"a unit was released through a blocked exit");
	Check (CommanderProductionService::Get_Queue_Item (FACTORY,0,&item) &&
			 item.State == PRODUCTION_STATE_BLOCKED_EXIT,
			 "a finished item at a blocked exit does not read 'blocked exit'");

	CommanderProductionService::Set_Facility_Runtime_State (FACTORY,true,true,true,true);
	CommanderProductionService::Update (0.1f);
	Check (CommanderProductionService::Get_Completed_Count () == 1,"clearing the exit did not release the unit");
	CommanderProductionService::Clear_Completed ();

	//	Destruction refunds the queue and leaves nothing that could complete invisibly.
	CommanderProductionService::Request_Production (COMMANDER,barracks,21,50,2.0f,10000,true,&ticket);
	int queued_value = 0;
	for (int i = 0; i < CommanderProductionService::Get_Queue_Length (BARRACKS); i++) {
		CommanderProductionItem queued;
		if (CommanderProductionService::Get_Queue_Item (BARRACKS,i,&queued)) queued_value += queued.Cost;
	}
	int refund = CommanderProductionService::On_Facility_Destroyed (BARRACKS);
	Check (refund == queued_value,"destroying a facility refunded %d against a queued value of %d",
			 refund,queued_value);
	Check (CommanderProductionService::Get_Queue_Length (BARRACKS) == 0,"a destroyed facility kept its queue");

	/*
	**	Population caps count the queue as well as the living, so a Commander cannot out-queue
	**	the cap and receive them all afterwards.
	*/
	CommanderProductionService::Init ();
	CommanderProductionService::Set_Team_Commander (TEAM_GDI,COMMANDER);
	CommanderProductionService::Set_Population_Cap (BUILDING_PRODUCTION_GROUND_VEHICLE,2);
	CommanderProductionService::Set_Population (TEAM_GDI,BUILDING_PRODUCTION_GROUND_VEHICLE,2);
	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_POPULATION_CAP,"a purchase got past a reached population cap");

	CommanderProductionService::Set_Population (TEAM_GDI,BUILDING_PRODUCTION_GROUND_VEHICLE,1);
	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_ACCEPTED,"a purchase inside the cap was refused");
	Check (CommanderProductionService::Request_Production (COMMANDER,facility,10,100,5.0f,10000,true,&ticket)
				== PRODUCTION_POPULATION_CAP,"one live unit plus one queued got past a cap of two");

	//	Selection, and orders a non-Commander or a human-driven unit must refuse.
	CommanderProductionService::Clear_Selection ();
	Check (CommanderProductionService::Get_Selection_Count () == 0,"a cleared selection is not empty");
	Check (CommanderProductionService::Add_To_Selection (5001),"a unit could not be selected");
	Check (CommanderProductionService::Add_To_Selection (5001),"re-selecting a selected unit failed");
	Check (CommanderProductionService::Get_Selection_Count () == 1,"selecting the same unit twice made %d entries",
			 CommanderProductionService::Get_Selection_Count ());
	Check (CommanderProductionService::Is_Selected (5001),"a selected unit does not read as selected");
	Check (CommanderProductionService::Remove_From_Selection (5001),"a selected unit could not be deselected");
	Check (!CommanderProductionService::Is_Selected (5001),"a deselected unit still reads as selected");

	Vector3 destination (10.0f,20.0f,0.0f);
	Check (!CommanderProductionService::Issue_Order (COMMANDER,5001,TEAM_GDI,false,ORDER_MOVE,destination,0),
			 "the Commander was allowed to order a human-controlled unit");
	Check (!CommanderProductionService::Issue_Order (IMPOSTOR,5001,TEAM_GDI,true,ORDER_MOVE,destination,0),
			 "a non-Commander was allowed to order AI");
	Check (!CommanderProductionService::Issue_Order (COMMANDER,5001,0,true,ORDER_MOVE,destination,0),
			 "the GDI Commander was allowed to order a Nod unit");

	Check (CommanderProductionService::Get_Order (5001) == ORDER_HOLD_POSITION,
			 "an unordered unit defaults to '%s', not hold position",
			 CommanderProductionService::Get_Order_Name (CommanderProductionService::Get_Order (5001)));

	Check (CommanderProductionService::Issue_Order (COMMANDER,5001,TEAM_GDI,true,ORDER_MOVE,destination,0),
			 "the Commander could not order their own AI");
	Check (CommanderProductionService::Get_Order (5001) == ORDER_MOVE,"the issued order did not stick");

	Vector3 read_back (0.0f,0.0f,0.0f);
	int target_id = -1;
	Check (CommanderProductionService::Get_Order_Target (5001,&read_back,&target_id),"the order has no target");
	Check (WWMath::Fabs (read_back.X - 10.0f) < 0.01f,"the order target read back as %f, not 10",read_back.X);

	//	A second order replaces the first rather than stacking.
	CommanderProductionService::Issue_Order (COMMANDER,5001,TEAM_GDI,true,ORDER_GUARD,destination,0);
	Check (CommanderProductionService::Get_Order (5001) == ORDER_GUARD,"a re-order did not replace the previous one");
	Check (CommanderProductionService::Get_Order_Count () == 1,"re-ordering one unit created %d order records",
			 CommanderProductionService::Get_Order_Count ());

	CommanderProductionService::Forget_Unit (5001);
	Check (CommanderProductionService::Get_Order_Count () == 0,"a forgotten unit kept its order");
	Check (!CommanderProductionService::Is_Selected (5001),"a forgotten unit stayed selected");

	/*
	**	Not covered by any automated check: nothing here creates a SoldierGameObj or a
	**	VehicleGameObj, by design -- this service reports completions and never spawns.  That a
	**	completion really does become a canonical AI unit which walks out of a door and holds
	**	needs a live match, and is named in WIP_features.md.
	*/

	CommanderProductionService::Shutdown ();
	return _Failures;
}


int	Run_RTS_Persistence (void)
{
	BuildingDescriptorManager::Init ();
	TeamPowerGridService::Init ();
	BuildingConstructionService::Init ();
	CommanderProductionService::Init ();

	StringClass error ("");
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_ps_plant",SIDEBAR_PLANT_JSON,&error),
			 "plant descriptor refused: %s",error.Peek_Buffer ());
	Check (BuildingDescriptorManager::Load_Descriptor_From_Text ("ow_test_ps_factory",SIDEBAR_FACTORY_JSON,&error),
			 "factory descriptor refused: %s",error.Peek_Buffer ());

	const int TEAM_GDI = 1;

	/*
	**	Contains_Data is what keeps an ordinary FPS save file unchanged by this phase: with
	**	nothing registered anywhere, the subsystem writes no chunk at all.
	*/
	Check (!RTSPersistenceClass::Has_Data (),
			 "an empty RTS layer still claims to have save data");

	TeamPowerGridService::RegisterBuilding (nullptr,TEAM_GDI,"ow_test_ps_plant",201);
	TeamPowerGridService::RegisterBuilding (nullptr,TEAM_GDI,"ow_test_ps_factory",202);
	TeamPowerGridService::RecalculateTeam (TEAM_GDI);
	Check (RTSPersistenceClass::Has_Data (),
			 "a populated power grid reports nothing worth saving");

	/*
	**	The two facts Section 46 says persist from the grid are readable back through the
	**	accessors the save pass uses -- and the derived allocation is *not* among them.
	*/
	Check (TeamPowerGridService::Set_Manual_Disabled (202,true),"a manual disable was refused");
	Check (TeamPowerGridService::Set_Priority_Override (202,77),"a priority override was refused");

	PowerGridMemberIdType read_id = 0;
	int read_team = 0;
	bool found_202 = false;
	for (int i = 0; i < TeamPowerGridService::Get_Member_Count (); i++) {
		if (!TeamPowerGridService::Get_Member_By_Index (i,&read_id,&read_team)) continue;
		if (read_id == 202) {
			found_202 = true;
			Check (read_team == TEAM_GDI,"member 202 enumerated on team %d, not GDI",read_team);
		}
	}
	Check (found_202,"a registered member did not appear in the save enumeration");

	int read_priority = 0;
	Check (TeamPowerGridService::Get_Priority_Override (202,&read_priority),
			 "a set priority override did not read back");
	Check (read_priority == 77,"the priority override read back as %d, not 77",read_priority);
	Check (!TeamPowerGridService::Get_Priority_Override (201,&read_priority),
			 "a member with no override reported one");
	Check (TeamPowerGridService::Is_Manual_Disabled (202),"a manual disable did not read back");

	/*
	**	Rebuild_Derived_State is what On_Post_Load calls.  After a load the allocation must be
	**	recomputed from the surviving members rather than restored -- so running it must leave
	**	the grid consistent with the manual state above, not with whatever it was before.
	*/
	RTSPersistenceClass::Rebuild_Derived_State ();
	Check (!TeamPowerGridService::Is_Member_Powered (202),
			 "a manually disabled building came back powered after the post-load recalculation");
	Check (WWMath::Fabs (TeamPowerGridService::Get_Requested_Demand (TEAM_GDI)) < 0.01f,
			 "a manually disabled consumer still requested %f after the recalculation",
			 TeamPowerGridService::Get_Requested_Demand (TEAM_GDI));
	Check (WWMath::Fabs (TeamPowerGridService::Get_Generation (TEAM_GDI) - 300.0f) < 0.01f,
			 "the post-load recalculation produced generation %f, not the plant's 300",
			 TeamPowerGridService::Get_Generation (TEAM_GDI));

	/*
	**	A production queue restored out of a save keeps its identity and its elapsed time, and
	**	is restored as QUEUED -- PRODUCING and the two blocked states are conclusions about a
	**	world that has not been rebuilt yet.
	*/
	CommanderProductionItem saved;
	saved.FacilityId			= 900;
	saved.DefinitionId		= 10;
	saved.Team					= TEAM_GDI;
	saved.Cost					= 100;
	saved.BuildTimeSeconds	= 5.0f;
	saved.ElapsedSeconds		= 3.0f;
	saved.Kind					= BUILDING_PRODUCTION_GROUND_VEHICLE;
	saved.TicketId				= 4096;
	saved.State					= PRODUCTION_STATE_QUEUED;

	CommanderProductionService::Restore_Item (saved);
	Check (CommanderProductionService::Get_Queue_Length (900) == 1,"a restored item did not reach the queue");

	CommanderProductionItem restored;
	Check (CommanderProductionService::Get_Queue_Item (900,0,&restored),"the restored item is unreadable");
	Check (restored.TicketId == 4096,"the restored item's ticket is %d, not 4096",restored.TicketId);
	Check (WWMath::Fabs (restored.ElapsedSeconds - 3.0f) < 0.0001f,
			 "the restored item lost its elapsed time (%f)",restored.ElapsedSeconds);

	/*
	**	A ticket issued after the load must not collide with a restored one -- the counter is
	**	pushed past everything restored.
	*/
	CommanderProductionService::Set_Team_Commander (TEAM_GDI,4242);
	CommanderFacilityContext facility;
	facility.FacilityId				= 900;
	facility.Team						= TEAM_GDI;
	facility.Alive						= true;
	facility.ConstructionComplete	= true;
	facility.Powered					= true;
	facility.Kind						= BUILDING_PRODUCTION_GROUND_VEHICLE;
	facility.QueueCapacity			= 5;
	facility.ExitClear				= true;

	int new_ticket = 0;
	Check (CommanderProductionService::Request_Production (4242,facility,11,100,5.0f,10000,true,&new_ticket)
				== PRODUCTION_ACCEPTED,"a purchase after a restore was refused");
	Check (new_ticket > 4096,"a post-load ticket (%d) collided with a restored one (4096)",new_ticket);

	//	A restored queue resumes rather than restarting: two more seconds finishes a five-second
	//	item that was three seconds in when it was saved.
	CommanderProductionService::Set_Facility_Runtime_State (900,true,true,true,true);
	CommanderProductionService::Update (2.1f);
	Check (CommanderProductionService::Get_Completed_Count () == 1,
			 "a restored item three seconds into a five-second build did not finish after two more");

	//	Durable orders survive, and an out-of-range order value is clamped rather than trusted.
	CommanderProductionService::Restore_Order (6001,ORDER_PATROL,Vector3 (5.0f,6.0f,7.0f),0);
	Check (CommanderProductionService::Get_Order (6001) == ORDER_PATROL,"a restored order did not read back");
	Check (CommanderProductionService::Get_Order_Count () == 1,"a restored order made %d records",
			 CommanderProductionService::Get_Order_Count ());
	CommanderProductionService::Restore_Order (0,ORDER_MOVE,Vector3 (0.0f,0.0f,0.0f),0);
	Check (CommanderProductionService::Get_Order_Count () == 1,"a restore of unit id 0 created a record");

	/*
	**	Not covered by any automated check: nothing here writes or reads an actual chunk, because
	**	ChunkSaveClass wants a file and SaveLoadSystemClass wants a running game.  What is proved
	**	is every rule the Save and Load bodies depend on -- the enumeration, the restore paths,
	**	the ticket collision, and that the derived power state is recomputed rather than restored.
	**	A real round trip through a save file needs a live match; see WIP_features.md.
	*/

	CommanderProductionService::Shutdown ();
	BuildingConstructionService::Shutdown ();
	TeamPowerGridService::Shutdown ();
	BuildingDescriptorManager::Shutdown ();
	return _Failures;
}


/***********************************************************************************************
 * Run_Visual_Settings -- roadmap Section 52A, Phases 46 + 47.                                 *
 *   Pure static state, no device: the sampler/filtering/antialias defaults and the LOD-        *
 *   dissolve switch, and that every one of them clamps instead of trusting its caller.         *
 *=============================================================================================*/
int	Run_Visual_Settings (void)
{
	Check (WW3D::Get_Anisotropic_Level () == 2,
			 "anisotropic level defaulted to %d, not the 2 TextureClass::_Init_Filters always hardcoded",
			 WW3D::Get_Anisotropic_Level ());
	Check (WW3D::Get_Texture_LOD_Bias () == 0.0f,"texture LOD bias defaulted to %f, not 0",
			 WW3D::Get_Texture_LOD_Bias ());
	Check (WW3D::Get_Antialias_Quality () == 0,"antialias quality defaulted to %d, not 0 (off)",
			 WW3D::Get_Antialias_Quality ());
	Check (!WW3D::Get_LOD_Dissolve_Enabled (),"LOD dissolve defaulted to on");

	WW3D::Set_Anisotropic_Level (99);
	Check (WW3D::Get_Anisotropic_Level () == 16,"anisotropic level 99 was not clamped to 16 (got %d)",
			 WW3D::Get_Anisotropic_Level ());
	WW3D::Set_Anisotropic_Level (0);
	Check (WW3D::Get_Anisotropic_Level () == 1,"anisotropic level 0 was not clamped to 1 (got %d)",
			 WW3D::Get_Anisotropic_Level ());

	WW3D::Set_Texture_LOD_Bias (-1.5f);
	Check (WW3D::Get_Texture_LOD_Bias () == -1.5f,"texture LOD bias did not round-trip (got %f)",
			 WW3D::Get_Texture_LOD_Bias ());

	WW3D::Set_Antialias_Quality (5);
	Check (WW3D::Get_Antialias_Quality () == 2,"antialias quality 5 was not clamped to 2 (got %d)",
			 WW3D::Get_Antialias_Quality ());
	WW3D::Set_Antialias_Quality (-1);
	Check (WW3D::Get_Antialias_Quality () == 0,"antialias quality -1 was not clamped to 0 (got %d)",
			 WW3D::Get_Antialias_Quality ());

	WW3D::Set_LOD_Dissolve_Enabled (true);
	Check (WW3D::Get_LOD_Dissolve_Enabled (),"LOD dissolve did not turn on");
	WW3D::Set_LOD_Dissolve_Enabled (false);
	Check (!WW3D::Get_LOD_Dissolve_Enabled (),"LOD dissolve did not turn back off");

	/*
	**	Not covered here: DistLODClass's, HLodClass's and WorldTerrainSystem's actual
	**	overlap-window behaviour, all three of which need real .w3d content and a scene this
	**	binary does not have.  What is proved is the one switch all three owners read
	**	(WW3D::Get_LOD_Dissolve_Enabled) and that it defaults off.
	*/

	//	Restore defaults so a later family in the same "all" run sees stock settings.
	WW3D::Set_Anisotropic_Level (2);
	WW3D::Set_Texture_LOD_Bias (0.0f);
	WW3D::Set_Antialias_Quality (0);

	return _Failures;
}


/***********************************************************************************************
 * Run_Sky_Clock -- roadmap Section 52A, Phase 48.                                             *
 *=============================================================================================*/
int	Run_Sky_Clock (void)
{
	bool was_server = CombatManager::I_Am_Server ();
	CombatManager::Set_I_Am_Server (true);

	Check (!BackgroundMgrClass::Is_Sky_Clock_Active (),
			 "the sky clock defaulted to active before Set_Sky_Clock_Rate was ever called");

	unsigned start_hours = 0, start_minutes = 0;
	BackgroundMgrClass::Get_Time_Of_Day (start_hours,start_minutes);

	BackgroundMgrClass::Set_Sky_Clock_Rate (0.0f);
	Check (BackgroundMgrClass::Is_Sky_Clock_Active (),"Set_Sky_Clock_Rate did not turn the clock on");
	Check (BackgroundMgrClass::Get_Sky_Clock_Rate () == 0.0f,"a rate of zero did not read back as zero");

	unsigned hours = 0, minutes = 0;
	BackgroundMgrClass::Get_Time_Of_Day (hours,minutes);
	Check ((hours == start_hours) && (minutes == start_minutes),
			 "turning the clock on at a rate of zero changed the hour (%u:%u -> %u:%u)",
			 start_hours,start_minutes,hours,minutes);

	BackgroundMgrClass::Set_Sky_Clock_Rate (1.5f);
	Check (BackgroundMgrClass::Get_Sky_Clock_Rate () == 1.5f,
			 "a rate of 1.5 did not read back (got %f)",BackgroundMgrClass::Get_Sky_Clock_Rate ());

	/*
	**	Not covered here, the same way Run_RTS_Persistence's own save/load is not: the
	**	BitStreamClass wire round trip and the per-frame Update() clock advance both need a
	**	running scene/network layer this binary does not have.  What is proved is the state the
	**	rest of the phase is built on -- the switch is off until asked, a zero rate leaves the
	**	hour exactly alone, and a set rate reads back.  See WIP_features.md.
	*/

	CombatManager::Set_I_Am_Server (was_server);
	return _Failures;
}


/***********************************************************************************************
 * Run_Light_Curves -- roadmap Section 52A, Phase 50.                                          *
 *=============================================================================================*/
int	Run_Light_Curves (void)
{
	LightClass * light = NEW_REF (LightClass,());
	light->Set_Intensity (2.0f);

	Check (light->Get_Light_Curve () == LightClass::LIGHT_CURVE_STEADY,
			 "a fresh light did not default to LIGHT_CURVE_STEADY");

	//	STEADY is a no-op: Update_Light_Curve must never move a STEADY light's intensity.
	light->Update_Light_Curve (1.0f);
	Check (light->Get_Intensity () == 2.0f,
			 "Update_Light_Curve moved a STEADY light's intensity to %f",light->Get_Intensity ());

	//	FAIL ramps down to (and stays at) zero.
	light->Set_Light_Curve (LightClass::LIGHT_CURVE_FAIL);
	light->Update_Light_Curve (10.0f);		// far past the ramp's own duration
	Check (light->Get_Intensity () == 0.0f,
			 "a FAIL curve did not reach zero after ten seconds (got %f)",light->Get_Intensity ());

	//	STEADY restores the ORIGINAL intensity, not whatever FAIL left it at -- the exact bug
	//	Set_Light_Curve's own "restore, don't resnapshot" comment exists to prevent.
	light->Set_Light_Curve (LightClass::LIGHT_CURVE_STEADY);
	Check (light->Get_Intensity () == 2.0f,
			 "restoring STEADY after a FAIL ramp gave %f, not the original 2.0",
			 light->Get_Intensity ());

	//	Setting the SAME curve twice is a no-op -- BuildingGameObj::Update_Group_Lighting calls
	//	this on every Enable_Power, changed or not, so a redundant call must never re-snapshot.
	light->Set_Intensity (5.0f);
	light->Set_Light_Curve (LightClass::LIGHT_CURVE_STEADY);
	Check (light->Get_Intensity () == 5.0f,
			 "a redundant Set_Light_Curve(STEADY) touched Intensity (got %f)",light->Get_Intensity ());

	//	PULSE stays within [0, base].
	light->Set_Intensity (4.0f);
	light->Set_Light_Curve (LightClass::LIGHT_CURVE_PULSE);
	for (float t = 0.0f; t < 5.0f; t += 0.37f) {
		light->Update_Light_Curve (0.37f);
		Check ((light->Get_Intensity () >= 0.0f) && (light->Get_Intensity () <= 4.0f),
				 "a PULSE curve produced %f, outside [0,4]",light->Get_Intensity ());
	}

	//	WARMUP ramps 0 -> base once, then holds.
	light->Set_Intensity (3.0f);
	light->Set_Light_Curve (LightClass::LIGHT_CURVE_WARMUP);
	light->Update_Light_Curve (0.01f);
	Check (light->Get_Intensity () < 3.0f,
			 "a WARMUP curve reached full intensity on its very first tick");
	light->Update_Light_Curve (10.0f);
	Check (light->Get_Intensity () == 3.0f,
			 "a WARMUP curve did not reach full intensity after ten seconds (got %f)",
			 light->Get_Intensity ());

	light->Release_Ref ();

	//	WorldLightManager: the group-id association Section 40 power/damage drives (the other
	//	half of lightphys.h's own long-standing "the building code uses the Group ID" comment).
	LightPhysClass * fixture = NEW_REF (LightPhysClass,(true));
	fixture->Set_Group_ID (777);

	WorldLightManager::Set_Group_Curve (777,LightClass::LIGHT_CURVE_FAIL);
	LightClass * fixture_light = (LightClass *) fixture->Peek_Model ();
	Check ((fixture_light != nullptr) && (fixture_light->Get_Light_Curve () == LightClass::LIGHT_CURVE_FAIL),
			 "Set_Group_Curve did not reach a light registered under its group id");

	WorldLightManager::Set_Group_Curve (778,LightClass::LIGHT_CURVE_PULSE);
	Check ((fixture_light != nullptr) && (fixture_light->Get_Light_Curve () == LightClass::LIGHT_CURVE_FAIL),
			 "Set_Group_Curve touched a light registered under a different group id");

	WorldLightManager::Set_Group_Curve (0,LightClass::LIGHT_CURVE_STEADY);
	Check ((fixture_light != nullptr) && (fixture_light->Get_Light_Curve () == LightClass::LIGHT_CURVE_FAIL),
			 "Set_Group_Curve(0, ...) touched a real light -- group id 0 must match nothing");

	fixture->Release_Ref ();

	/*
	**	Not covered here: BuildingGameObj::Update_Group_Lighting itself needs a real
	**	BuildingGameObj, and the level-authoring convention this whole phase depends on -- a
	**	fixture's Group ID actually being set to its building's Get_ID() in LevelEdit or at
	**	spawn time -- is data, not code (see the docs/assets/ pattern this project already uses
	**	for exactly this kind of gap).
	*/

	return _Failures;
}


/***********************************************************************************************
 * Run_Height_Fog -- roadmap Section 52A, Phase 51.                                            *
 *   WW3D::Compute_Height_Fog_Scale is a pure function of the current settings and a viewer     *
 *   height; no device, no scene, and it is exactly what DX8Wrapper::Set_Fog applies, so         *
 *   exercising it here proves the term without needing a render target.                        *
 *=============================================================================================*/
int	Run_Height_Fog (void)
{
	Check (!WW3D::Get_Height_Fog_Enabled (),"height fog defaulted to on");
	Check (WW3D::Compute_Height_Fog_Scale (-1000.0f) == 1.0f,
			 "disabled height fog did not return scale 1.0 regardless of height (got %f)",
			 WW3D::Compute_Height_Fog_Scale (-1000.0f));
	Check (WW3D::Compute_Height_Fog_Scale (1000.0f) == 1.0f,
			 "disabled height fog did not return scale 1.0 at a high viewer position (got %f)",
			 WW3D::Compute_Height_Fog_Scale (1000.0f));

	WW3D::Set_Height_Fog_Reference (0.0f,100.0f);
	WW3D::Set_Height_Fog_Enabled (true);

	Check (WW3D::Compute_Height_Fog_Scale (-100.0f) == 1.0f,
			 "at/below the falloff band's bottom, scale was not 1.0 (got %f)",
			 WW3D::Compute_Height_Fog_Scale (-100.0f));
	Check (WW3D::Compute_Height_Fog_Scale (0.0f) == 0.0f,
			 "at the ceiling, scale was not 0.0 (got %f)",WW3D::Compute_Height_Fog_Scale (0.0f));
	Check (WW3D::Compute_Height_Fog_Scale (500.0f) == 0.0f,
			 "well above the ceiling, scale was not clamped to 0.0 (got %f)",
			 WW3D::Compute_Height_Fog_Scale (500.0f));

	float mid = WW3D::Compute_Height_Fog_Scale (-50.0f);
	Check ((mid > 0.0f) && (mid < 1.0f),"midway through the falloff band, scale was not between 0 and 1 (got %f)",mid);

	WW3D::Set_Height_Fog_Reference (0.0f,-5.0f);
	Check (WW3D::Get_Height_Fog_Falloff () > 0.0f,
			 "a non-positive falloff was not guarded against (got %f)",WW3D::Get_Height_Fog_Falloff ());

	/*
	**	Not covered here: DX8Wrapper::Set_Fog's own application of this scale to the device's
	**	FOGSTART/FOGEND render states needs a real device. What is proved is the term every
	**	frame's one Set_Fog call reads -- off by default, and clamped both directions once on.
	*/

	WW3D::Set_Height_Fog_Enabled (false);
	WW3D::Set_Height_Fog_Reference (0.0f,500.0f);
	return _Failures;
}


/***********************************************************************************************
 * Run_Sky_Occlusion -- roadmap Section 52A, Phase 52.                                         *
 *   The switch Light_Terrain reads before spending a raycast per vertex per sample direction   *
 *   -- default off, so an ordinary solve costs nothing new and reproduces today's terrain      *
 *   lighting.  The raycasts themselves need a real PhysicsSceneClass and are not covered here.  *
 *=============================================================================================*/
int	Run_Sky_Occlusion (void)
{
	LightSolveContextClass context;

	Check (context.Is_Occlusion_Enabled (),
			 "the pre-existing direct-light occlusion test changed its long-standing default");
	Check (!context.Is_Sky_Occlusion_Enabled (),
			 "the new ambient sky-occlusion term defaulted to on");

	context.Enable_Sky_Occlusion (true);
	Check (context.Is_Sky_Occlusion_Enabled (),"Enable_Sky_Occlusion(true) did not take");
	Check (context.Is_Occlusion_Enabled (),
			 "turning on sky occlusion touched the separate, pre-existing direct-light switch");

	context.Enable_Sky_Occlusion (false);
	Check (!context.Is_Sky_Occlusion_Enabled (),"Enable_Sky_Occlusion(false) did not take");

	return _Failures;
}


/***********************************************************************************************
 * Run_Sky_Driven_Toggles -- roadmap Section 52A, Phases 53-55.                                *
 *   Cloud ground-shadows, metal-map reflection refresh and sun shafts all gate their real work *
 *   -- a CloudLayerClass ground projector, MetalMapManagerClass's texture regeneration, and a  *
 *   Render2DClass draw -- behind one static bool each, and every one of those real paths needs *
 *   a live scene, an asset manager and a device this binary does not have.  What is proved here *
 *   is the one thing common to all three: off by default, and the setter round-trips.           *
 *=============================================================================================*/
int	Run_Sky_Driven_Toggles (void)
{
	Check (!BackgroundMgrClass::Get_Cloud_Shadows_Enabled (),"cloud shadows defaulted to on");
	BackgroundMgrClass::Set_Cloud_Shadows_Enabled (true);
	Check (BackgroundMgrClass::Get_Cloud_Shadows_Enabled (),"cloud shadows did not turn on");
	BackgroundMgrClass::Set_Cloud_Shadows_Enabled (false);
	Check (!BackgroundMgrClass::Get_Cloud_Shadows_Enabled (),"cloud shadows did not turn back off");

	Check (!BackgroundMgrClass::Get_Metal_Reflections_Enabled (),"metal reflections defaulted to on");
	BackgroundMgrClass::Set_Metal_Reflections_Enabled (true);
	Check (BackgroundMgrClass::Get_Metal_Reflections_Enabled (),"metal reflections did not turn on");
	BackgroundMgrClass::Set_Metal_Reflections_Enabled (false);
	Check (!BackgroundMgrClass::Get_Metal_Reflections_Enabled (),"metal reflections did not turn back off");

	Check (!BackgroundMgrClass::Get_Sun_Shafts_Enabled (),"sun shafts defaulted to on");
	BackgroundMgrClass::Set_Sun_Shafts_Enabled (true);
	Check (BackgroundMgrClass::Get_Sun_Shafts_Enabled (),"sun shafts did not turn on");
	BackgroundMgrClass::Set_Sun_Shafts_Enabled (false);
	Check (!BackgroundMgrClass::Get_Sun_Shafts_Enabled (),"sun shafts did not turn back off");

	return _Failures;
}




/***********************************************************************************************
 * Run_Tonemap -- roadmap Section 52A, Phase 56.                                               *
 *   The adaptation is arithmetic and the ramp upload is a device call, so the arithmetic is    *
 *   what gets proved: brightness composition, the exposure it asks for, and that the approach  *
 *   converges rather than overshooting.  That the ramp reaches the hardware, and that an       *
 *   unlit interior looks right once it does, both need a screen.                               *
 *=============================================================================================*/
int	Run_Tonemap (void)
{
	Check (!PostProcessChainClass::Is_Tonemap_Enabled (),"tonemap defaulted to on");

	//	Sky visibility is the whole interior/exterior distinction.
	float outdoors = PostProcessChainClass::Compute_Scene_Brightness (1.0f,0.2f,1.0f);
	float indoors  = PostProcessChainClass::Compute_Scene_Brightness (1.0f,0.2f,0.0f);
	Check (outdoors > indoors,"an enclosed camera did not read darker than an open one");
	Check (indoors > 0.0f,"a fully enclosed camera read as absolutely black");

	//	Brighter scene, lower exposure.
	float bright_exposure = PostProcessChainClass::Compute_Target_Exposure (outdoors);
	float dark_exposure   = PostProcessChainClass::Compute_Target_Exposure (indoors);
	Check (dark_exposure > bright_exposure,"a darker scene did not ask for more exposure");

	//	Clamped at both ends, so neither a cupboard nor a white sky can run it away.
	Check (PostProcessChainClass::Compute_Target_Exposure (0.0f) <= 2.21f,
			 "a black scene drove the exposure past its ceiling");
	Check (PostProcessChainClass::Compute_Target_Exposure (1000.0f) >= 0.64f,
			 "a blinding scene drove the exposure below its floor");

	//	Adaptation converges and never overshoots.
	float value = 1.0f;
	for (int i = 0; i < 400; i++) {
		value = PostProcessChainClass::Adapt (value,2.0f,0.05f,0.5f);
		Check (value <= 2.0f,"adaptation overshot its target");
	}
	Check (value > 1.98f,"adaptation did not converge on its target");

	//	A zero-length frame must not move anything, or a paused game drifts the exposure.
	Check (PostProcessChainClass::Adapt (1.0f,2.0f,0.0f,0.5f) == 1.0f,
			 "a zero-length frame still adapted");

	PostProcessChainClass::Enable_Tonemap (true);
	Check (PostProcessChainClass::Is_Tonemap_Enabled (),"tonemap did not turn on");
	PostProcessChainClass::Enable_Tonemap (false);
	Check (!PostProcessChainClass::Is_Tonemap_Enabled (),"tonemap did not turn back off");

	//	The shoulder: off by default, so the ramp clamps exactly as it always did.
	Check (DX8Wrapper::Get_Tone_Shoulder () == 0.0f,"the tone shoulder defaulted to on");

	return _Failures;
}


/***********************************************************************************************
 * Run_Bloom -- roadmap Section 52A, Phase 57.                                                 *
 *   The threshold is the only part of the chain that is not a draw call.  Accumulation is      *
 *   proved inert while the feature is off, which is the property that makes "bloom disabled    *
 *   leaves the image unchanged" true rather than merely intended.                              *
 *=============================================================================================*/
int	Run_Bloom (void)
{
	Check (!PostProcessChainClass::Is_Bloom_Enabled (),"bloom defaulted to on");

	Check (PostProcessChainClass::Passes_Threshold (0.5f,0.1f),
			 "a bright source was rejected by the threshold");
	Check (!PostProcessChainClass::Passes_Threshold (0.05f,0.1f),
			 "a dim source passed the threshold");
	Check (!PostProcessChainClass::Passes_Threshold (0.1f,0.1f),
			 "a source exactly at the threshold passed it");

	//	Off: nothing accumulates, so no target is allocated and no draw ever happens.
	PostProcessChainClass::Begin_Glare ();
	PostProcessChainClass::Add_Glare (Vector3 (0.0f,0.0f,0.5f),1.0f,Vector3 (1.0f,1.0f,1.0f),0.1f);
	Check (PostProcessChainClass::Get_Glare_Count () == 0,
			 "a glare source was accumulated while bloom was off");

	PostProcessChainClass::Enable_Bloom (true);
	Check (PostProcessChainClass::Is_Bloom_Enabled (),"bloom did not turn on");

	PostProcessChainClass::Begin_Glare ();
	PostProcessChainClass::Add_Glare (Vector3 (0.0f,0.0f,0.5f),1.0f,Vector3 (1.0f,1.0f,1.0f),0.1f);
	Check (PostProcessChainClass::Get_Glare_Count () == 1,"a visible source was not accumulated");

	//	Off screen and behind the camera are both rejected.
	PostProcessChainClass::Add_Glare (Vector3 (3.0f,0.0f,0.5f),1.0f,Vector3 (1.0f,1.0f,1.0f),0.1f);
	Check (PostProcessChainClass::Get_Glare_Count () == 1,"an off-screen source was accumulated");
	PostProcessChainClass::Add_Glare (Vector3 (0.0f,0.0f,-1.0f),1.0f,Vector3 (1.0f,1.0f,1.0f),0.1f);
	Check (PostProcessChainClass::Get_Glare_Count () == 1,
			 "a source behind the camera was accumulated");

	PostProcessChainClass::Begin_Glare ();
	Check (PostProcessChainClass::Get_Glare_Count () == 0,"Begin_Glare did not clear the list");

	PostProcessChainClass::Enable_Bloom (false);
	Check (!PostProcessChainClass::Is_Bloom_Enabled (),"bloom did not turn back off");

	/*
	**	Phase 55's radial streaks, which now share this chain instead of being the streak-sprite
	**	approximation they started as.  The weights are the part that is arithmetic.
	*/
	Check (PostProcessChainClass::Compute_Streak_Weight (0,6,0.0f) == 0.0f,
			 "a fully occluded source still cast shafts");
	Check (PostProcessChainClass::Compute_Streak_Weight (0,6,1.0f) > 0.0f,
			 "a visible source cast no shafts");
	Check (PostProcessChainClass::Compute_Streak_Weight (5,6,1.0f) <
			 PostProcessChainClass::Compute_Streak_Weight (0,6,1.0f),
			 "the streak did not fall off with distance from the source");
	Check (PostProcessChainClass::Compute_Streak_Weight (6,6,1.0f) == 0.0f,
			 "a step past the end of the streak still contributed");
	Check (PostProcessChainClass::Compute_Streak_Weight (0,6,0.5f) <
			 PostProcessChainClass::Compute_Streak_Weight (0,6,1.0f),
			 "partial occlusion did not weaken the shafts");

	//	Total energy must not grow with the tap count, or adding steps would brighten the sun
	//	rather than smoothing it.
	float few = 0.0f, many = 0.0f;
	for (int i = 0; i < 4; i++) { few += PostProcessChainClass::Compute_Streak_Weight (i,4,1.0f); }
	for (int i = 0; i < 16; i++) { many += PostProcessChainClass::Compute_Streak_Weight (i,16,1.0f); }
	Check (WWMath::Fabs (few - many) < 0.1f,
			 "the streak got brighter rather than smoother as taps were added");

	return _Failures;
}


/***********************************************************************************************
 * Run_Wind_Field -- roadmap Section 52A, Phase 58.                                            *
 *   Sway is baked into geometry that needs an asset manager to build, so what is proved is the *
 *   offset function deciding where each vertex goes -- above all that a vertex at the base does *
 *   not move, which is what keeps foliage standing on the ground it grows out of.               *
 *=============================================================================================*/
int	Run_Wind_Field (void)
{
	Vector3 wind (3.0f,1.0f,0.0f);

	//	Zero scale is the pre-phase appearance, exactly.
	Check (FoliageSystem::Compute_Sway_Offset (wind,2.0f,2.0f,0.0f).Length () == 0.0f,
			 "a zero sway scale still moved a vertex");

	//	The base is pinned.
	Check (FoliageSystem::Compute_Sway_Offset (wind,0.0f,2.0f,1.0f).Length () == 0.0f,
			 "a vertex at the base of a plant moved with the wind");

	//	Higher moves further, and faster than linearly.
	float half = FoliageSystem::Compute_Sway_Offset (wind,1.0f,2.0f,1.0f).Length ();
	float top  = FoliageSystem::Compute_Sway_Offset (wind,2.0f,2.0f,1.0f).Length ();
	Check (top > half,"the top of a plant did not move further than its middle");
	Check (top > (half * 2.0f),"sway was linear in height rather than weighted toward the top");

	//	A vertex above the stated height is clamped, not extrapolated.
	float over = FoliageSystem::Compute_Sway_Offset (wind,10.0f,2.0f,1.0f).Length ();
	Check (over <= (top + 0.001f),"a vertex above the model height extrapolated past full sway");

	//	Sway follows the wind, and a horizontal wind does not lift anything.
	Vector3 offset = FoliageSystem::Compute_Sway_Offset (wind,2.0f,2.0f,1.0f);
	Check (offset.Z == 0.0f,"a horizontal wind pushed a vertex vertically");
	Check ((offset.X > 0.0f) && (offset.Y > 0.0f),"sway did not follow the wind direction");

	//	All three scales default to zero, which is what makes zero wind the current appearance.
	Check (WeatherMgrClass::Get_Foliage_Sway_Scale () == 0.0f,"foliage sway defaulted to non-zero");
	Check (WeatherMgrClass::Get_Particle_Wind_Scale () == 0.0f,"particle wind defaulted to non-zero");
	Check (WeatherMgrClass::Get_Cloud_Wind_Scale () == 0.0f,"cloud wind defaulted to non-zero");

	WeatherMgrClass::Set_Particle_Wind_Scale (0.5f);
	Check (WeatherMgrClass::Get_Particle_Wind_Scale () == 0.5f,"particle wind scale did not take");
	WeatherMgrClass::Set_Particle_Wind_Scale (-1.0f);
	Check (WeatherMgrClass::Get_Particle_Wind_Scale () == 0.0f,
			 "a negative particle wind scale was kept");
	WeatherMgrClass::Set_Particle_Wind_Scale (0.0f);

	return _Failures;
}


/***********************************************************************************************
 * Run_Wetness -- roadmap Section 52A, Phases 59 and 60.                                       *
 *   The scalar's integration and both things it drives -- the terrain layer bias and the mark   *
 *   fade rate -- are arithmetic, so all three are proved.  What is not proved is that a wet    *
 *   ground texture exists to bias toward, which is art, or that it reads, which needs a screen. *
 *=============================================================================================*/
int	Run_Wetness (void)
{
	//	Soaking, then drying, and drying is much the slower of the two.
	float wet = 0.0f;
	for (int i = 0; i < 200; i++) {
		wet = WeatherMgrClass::Integrate_Wetness (wet,1.0f,0.25f);
	}
	Check (wet > 0.99f,"a sustained downpour did not soak the world");

	float soaked = wet;
	float dried_one_second = soaked - WeatherMgrClass::Integrate_Wetness (soaked,0.0f,1.0f);
	float wetted_one_second = WeatherMgrClass::Integrate_Wetness (0.0f,1.0f,1.0f);
	Check (dried_one_second < wetted_one_second,
			 "the world dried at least as fast as it wetted");

	for (int i = 0; i < 500; i++) {
		wet = WeatherMgrClass::Integrate_Wetness (wet,0.0f,0.25f);
	}
	Check (wet == 0.0f,"the world never finished drying out");

	//	Bounded, and a zero-length frame moves nothing.
	Check (WeatherMgrClass::Integrate_Wetness (0.5f,1.0f,0.0f) == 0.5f,
			 "a zero-length frame still changed the wetness");
	Check (WeatherMgrClass::Integrate_Wetness (0.0f,5.0f,1000.0f) <= 1.0f,"wetness exceeded one");

	//	Phase 59's terrain bias: full on the flat, nothing on a cliff.
	Check (TerrainTextureSystem::Compute_Wetness_Bias (0.0f,0.0f) == 0.0f,
			 "a dry world still biased the layer table");
	float flat = TerrainTextureSystem::Compute_Wetness_Bias (1.0f,0.0f);
	float leaning = TerrainTextureSystem::Compute_Wetness_Bias (1.0f,DEG_TO_RADF (10.0f));
	Check (flat > leaning,"sloped ground held as much water as flat ground");
	Check (TerrainTextureSystem::Compute_Wetness_Bias (1.0f,DEG_TO_RADF (45.0f)) == 0.0f,
			 "water pooled on a cliff face");
	Check (flat < 1.0f,"a soaked field turned entirely into the wet layer");

	//	Phase 60's fade rate: exactly one when dry, so dry-weather timing is unchanged.
	Check (WorldSurfaceMarkManager::Compute_Weather_Fade_Rate (0.0f) == 1.0f,
			 "dry weather did not reproduce the current mark fade timing");
	Check (WorldSurfaceMarkManager::Compute_Weather_Fade_Rate (1.0f) > 1.0f,
			 "rain did not clear marks any faster");
	Check (WorldSurfaceMarkManager::Compute_Weather_Fade_Rate (0.5f) <
			 WorldSurfaceMarkManager::Compute_Weather_Fade_Rate (1.0f),
			 "the fade rate did not increase with wetness");

	Check (WeatherMgrClass::Get_Wetness_Response_Scale () == 0.0f,
			 "the wetness response defaulted to on");

	/*
	**	Phase 59's object half: wet albedo is darker and cooler, and wet surfaces are shinier.
	**	Both ride on the same effect Phase 61 uses, so both are off by the same default.
	*/
	Check (SurfaceTintEffectClass::Get_Wetness_Scale () == 0.0f,
			 "the object wetness response defaulted to on");

	Vector3 dry = SurfaceTintEffectClass::Compute_Wet_Albedo (0.0f,1.0f);
	Check ((dry.X == 1.0f) && (dry.Y == 1.0f) && (dry.Z == 1.0f),
			 "a dry surface was not multiplied by exactly white");
	Check (SurfaceTintEffectClass::Compute_Wet_Albedo (1.0f,0.0f).X == 1.0f,
			 "the disabled object wetness still darkened a surface");

	Vector3 soaked_albedo = SurfaceTintEffectClass::Compute_Wet_Albedo (1.0f,1.0f);
	Check (soaked_albedo.X < 1.0f,"rain did not darken a surface");
	Check (soaked_albedo.X > 0.0f,"a soaked surface multiplied down to a silhouette");
	Check (soaked_albedo.Z > soaked_albedo.X,"the wet tint was not cooler than neutral");
	Check (SurfaceTintEffectClass::Compute_Wet_Albedo (0.5f,1.0f).X > soaked_albedo.X,
			 "a damp surface was as dark as a soaked one");

	Check (SurfaceTintEffectClass::Compute_Wet_Specular (0.0f,1.0f) == 0.0f,
			 "a dry surface had a wet sheen");
	Check (SurfaceTintEffectClass::Compute_Wet_Specular (1.0f,0.0f) == 0.0f,
			 "the disabled object wetness still added a sheen");
	Check (SurfaceTintEffectClass::Compute_Wet_Specular (1.0f,1.0f) > 0.0f,
			 "a soaked surface had no sheen, so it would read as shadow rather than as wet");

	//	Shelter.  This is the acceptance line's "sheltered surfaces stay dry".
	WeatherMgrClass::Set_Wetness (1.0f);
	WeatherMgrClass::Set_Wetness_Response_Scale (1.0f);
	Check (WeatherMgrClass::Get_Sheltered_Wetness (1.0f) > 0.0f,
			 "a surface under open sky did not get wet");
	Check (WeatherMgrClass::Get_Sheltered_Wetness (0.0f) == 0.0f,
			 "a fully sheltered surface got wet");
	Check (WeatherMgrClass::Get_Sheltered_Wetness (0.5f) <
			 WeatherMgrClass::Get_Sheltered_Wetness (1.0f),
			 "partial shelter did not keep a surface drier");

	//	Put the world back the way the next family expects to find it.
	WeatherMgrClass::Set_Wetness_Response_Scale (0.0f);
	WeatherMgrClass::Set_Wetness (0.0f);

	return _Failures;
}


/***********************************************************************************************
 * Run_Damage_Tint -- roadmap Section 52A, Phase 61.                                           *
 *   The tint and ember ramps decide the whole look and are pure, so they are what is proved.   *
 *   Whether a multiplicative material pass composites the way it should needs a device.        *
 *=============================================================================================*/
int	Run_Damage_Tint (void)
{
	Check (SurfaceTintEffectClass::Get_Global_Scale () == 0.0f,"the damage tint defaulted to on");

	//	Off: the tint is exactly white, so a multiply by it is the identity.
	Vector3 off = SurfaceTintEffectClass::Compute_Soot_Tint (0.0f,0.0f);
	Check ((off.X == 1.0f) && (off.Y == 1.0f) && (off.Z == 1.0f),
			 "the disabled damage tint was not exactly white");

	//	Healthy objects are untinted even with the feature on.
	Vector3 healthy = SurfaceTintEffectClass::Compute_Soot_Tint (1.0f,1.0f);
	Check (healthy.X == 1.0f,"an undamaged object was tinted");

	//	And it darkens monotonically as health falls.
	Vector3 hurt = SurfaceTintEffectClass::Compute_Soot_Tint (0.5f,1.0f);
	Vector3 wrecked = SurfaceTintEffectClass::Compute_Soot_Tint (0.0f,1.0f);
	Check (hurt.X < healthy.X,"damage did not darken the surface");
	Check (wrecked.X < hurt.X,"more damage did not darken it further");
	Check (wrecked.X > 0.0f,"a wrecked object multiplied down to a silhouette");

	//	Soot is warm: it takes more blue than red.
	Check (wrecked.Z < wrecked.X,"the soot tint was not warmer than neutral");

	//	Embers: nothing in daylight, nothing on a healthy object, something on a burning one.
	Check (SurfaceTintEffectClass::Compute_Ember_Intensity (0.1f,0.0f,1.0f) == 0.0f,
			 "a burning object glowed in broad daylight");
	Check (SurfaceTintEffectClass::Compute_Ember_Intensity (1.0f,1.0f,1.0f) == 0.0f,
			 "an undamaged object glowed at night");
	Check (SurfaceTintEffectClass::Compute_Ember_Intensity (0.1f,1.0f,1.0f) > 0.0f,
			 "a nearly destroyed object did not glow at night");
	Check (SurfaceTintEffectClass::Compute_Ember_Intensity (0.1f,1.0f,0.0f) == 0.0f,
			 "embers ignored the master scale");

	//	Repair reverses it: the approach walks both ways and settles exactly.
	float shown = 0.2f;
	for (int i = 0; i < 100; i++) {
		shown = SurfaceTintEffectClass::Approach (shown,1.0f,0.1f);
	}
	Check (shown == 1.0f,"repair did not return the tint to pristine");
	for (int i = 0; i < 100; i++) {
		shown = SurfaceTintEffectClass::Approach (shown,0.0f,0.1f);
	}
	Check (shown == 0.0f,"damage did not drive the tint fully down");

	return _Failures;
}


/***********************************************************************************************
 * Run_Weapon_Kick -- roadmap Section 52A, Phase 62.                                           *
 *   The recovery curve is the phase.  That it stays cosmetic is structural rather than          *
 *   testable: the shake system only ever writes the camera transform, and it does so after the  *
 *   shot has already been resolved by the server.                                               *
 *=============================================================================================*/
int	Run_Weapon_Kick (void)
{
	Check (CameraShakeSystemClass::Get_Weapon_Response_Scale () == 0.0f,
			 "the weapon camera response defaulted to on");

	Check (CameraShakeSystemClass::Compute_Recovery (0.0f,0.2f) == 1.0f,
			 "the kick was not at full strength at the instant of the shot");
	Check (CameraShakeSystemClass::Compute_Recovery (0.2f,0.2f) == 0.0f,
			 "the kick had not recovered by the end of its duration");
	Check (CameraShakeSystemClass::Compute_Recovery (1.0f,0.2f) == 0.0f,
			 "the kick came back after recovering");
	Check (CameraShakeSystemClass::Compute_Recovery (0.1f,0.2f) < 0.5f,
			 "the recovery was not front loaded, so the kick would read as a drag");

	//	Monotonic, so the view never jerks back out on the way in.
	float previous = 1.0f;
	for (int i = 1; i <= 20; i++) {
		float now = CameraShakeSystemClass::Compute_Recovery ((float)i * 0.01f,0.2f);
		Check (now <= previous,"the recovery curve went back up part way through");
		previous = now;
	}

	//	A zero duration is a shot that never happened, not a divide by zero.
	Check (CameraShakeSystemClass::Compute_Recovery (0.0f,0.0f) == 0.0f,
			 "a zero-duration kick was not inert");

	CameraShakeSystemClass::Set_Weapon_Response_Scale (0.5f);
	Check (CameraShakeSystemClass::Get_Weapon_Response_Scale () == 0.5f,
			 "the weapon response scale did not take");
	CameraShakeSystemClass::Set_Weapon_Response_Scale (0.0f);
	Check (CameraShakeSystemClass::Get_Weapon_Response_Scale () == 0.0f,
			 "the weapon response scale did not turn back off");

	return _Failures;
}


/***********************************************************************************************
 * Run_World_Metrics -- roadmap Section 52B, Phase 63.                                         *
 *   Reading a definition needs a definition database and reading an envelope needs an asset     *
 *   manager, so what is proved is the derivations: slope, turning radius, corridor, clearance.  *
 *   Those are exactly the numbers a generator would otherwise invent, which is why the service  *
 *   exists at all.                                                                              *
 *=============================================================================================*/
int	Run_World_Metrics (void)
{
	//	Max climbable slope is the slide angle, clamped into a real quadrant.
	Check (WorldMetricsClass::Compute_Max_Climbable_Slope (DEG_TO_RADF (45.0f)) ==
			 DEG_TO_RADF (45.0f),"the slide angle was not reported as the climbable slope");
	Check (WorldMetricsClass::Compute_Max_Climbable_Slope (0.0f) == 0.0f,
			 "a zero slide angle produced a climbable slope");
	Check (WorldMetricsClass::Compute_Max_Climbable_Slope (DEG_TO_RADF (180.0f)) <
			 DEG_TO_RADF (90.0f),"a nonsense slide angle produced a slope past vertical");

	//	Turning radius: the bicycle model, with both degenerate cases distinguishable.
	float radius = WorldMetricsClass::Compute_Turning_Radius (4.0f,DEG_TO_RADF (30.0f));
	Check (radius > 0.0f,"a steerable vehicle reported no turning radius");
	Check (radius > 4.0f,"a thirty-degree steering limit produced an impossibly tight turn");

	float tighter = WorldMetricsClass::Compute_Turning_Radius (4.0f,DEG_TO_RADF (45.0f));
	Check (tighter < radius,"more steering angle did not tighten the turn");

	float longer = WorldMetricsClass::Compute_Turning_Radius (8.0f,DEG_TO_RADF (30.0f));
	Check (longer > radius,"a longer wheelbase did not widen the turn");

	Check (WorldMetricsClass::Compute_Turning_Radius (4.0f,0.0f) < 0.0f,
			 "a vehicle that cannot steer reported a finite turning radius");
	Check (WorldMetricsClass::Compute_Turning_Radius (4.0f,DEG_TO_RADF (90.0f)) == 0.0f,
			 "a vehicle steering at a right angle did not report turning on the spot");

	//	Corridor and clearance both leave margin: a gap a body exactly fits is a gap it grinds
	//	through.
	Check (WorldMetricsClass::Compute_Corridor_Width (1.0f) > 2.0f,
			 "the corridor width left no margin around the body");
	Check (WorldMetricsClass::Compute_Corridor_Width (0.0f) == 0.0f,
			 "a body with no radius still needed a corridor");

	Check (WorldMetricsClass::Compute_Overhead_Clearance (2.0f,0.5f) > 2.0f,
			 "the overhead clearance did not allow for the step being ridden up");
	Check (WorldMetricsClass::Compute_Overhead_Clearance (0.0f,0.5f) == 0.0f,
			 "a body with no height still needed headroom");

	//	Postures, including the one this engine does not have.
	Check (WorldMetricsClass::Is_Posture_Supported (POSTURE_STAND),"standing was unsupported");
	Check (WorldMetricsClass::Is_Posture_Supported (POSTURE_CROUCH),"crouching was unsupported");
	Check (!WorldMetricsClass::Is_Posture_Supported (POSTURE_PRONE),
			 "prone was reported as a posture this engine has");

	//	The soldier constants are the engine's own, moved rather than invented.
	Check (WorldMetricsClass::Get_Soldier_Eye_Height () > 1.6f,"the soldier eye height changed");
	Check (WorldMetricsClass::Get_Soldier_Crouch_Drop () > 0.0f,"the crouch drop was zero");
	Check (WorldMetricsClass::Get_Soldier_Crouch_Drop () <
			 WorldMetricsClass::Get_Soldier_Eye_Height (),
			 "crouching put the soldier's eyes below the ground");

	return _Failures;
}


/***********************************************************************************************
 * Run_Escarpment -- roadmap Section 52B, Phase 64.                                            *
 *   Geometry and collision need a physics scene; the polyline queries that decide where a face  *
 *   is and whether something crosses it do not, and they are what the traversability graph and  *
 *   any later generator actually consume.                                                       *
 *=============================================================================================*/
int	Run_Escarpment (void)
{
	EscarpmentClass cliff;
	cliff.Set_Name ("check");
	cliff.Add_Point (Vector3 (0.0f,0.0f,0.0f),10.0f,4.0f);
	cliff.Add_Point (Vector3 (100.0f,0.0f,0.0f),10.0f,4.0f);

	Check (cliff.Get_Point_Count () == 2,"the polyline did not take its points");
	Check (cliff.Get_Length () > 99.0f,"the polyline length was wrong");
	Check (cliff.Get_Max_Face_Height () == 6.0f,"the face height was not upper minus lower");

	//	A point beside the face samples the benches there.
	float upper = 0.0f, lower = 0.0f, distance = 0.0f;
	Check (cliff.Sample (50.0f,5.0f,&upper,&lower,&distance),
			 "a point beside the cliff did not sample");
	Check (upper == 10.0f,"the upper bench sampled wrong");
	Check (lower == 4.0f,"the lower bench sampled wrong");
	Check ((distance > 4.9f) && (distance < 5.1f),"the perpendicular distance was wrong");

	//	A line crossing the face is blocked; one running along it is not.
	float face = 0.0f;
	Check (cliff.Does_Segment_Cross (Vector3 (50.0f,-5.0f,0.0f),Vector3 (50.0f,5.0f,0.0f),&face),
			 "a segment straight across the cliff did not register as crossing it");
	Check (face == 6.0f,"the crossed face height was wrong");
	Check (!cliff.Does_Segment_Cross (Vector3 (10.0f,5.0f,0.0f),Vector3 (90.0f,5.0f,0.0f),&face),
			 "a segment running alongside the cliff registered as crossing it");
	Check (!cliff.Does_Segment_Cross (Vector3 (200.0f,-5.0f,0.0f),Vector3 (200.0f,5.0f,0.0f),&face),
			 "a segment past the end of the cliff registered as crossing it");

	//	A one-point polyline has no face and must not pretend to.
	EscarpmentClass stub;
	stub.Add_Point (Vector3 (0.0f,0.0f,0.0f),10.0f,4.0f);
	Check (!stub.Sample (0.0f,0.0f,&upper,&lower,&distance),
			 "a single-point escarpment sampled a face");
	Check (!stub.Does_Segment_Cross (Vector3 (-1.0f,0.0f,0.0f),Vector3 (1.0f,0.0f,0.0f),&face),
			 "a single-point escarpment blocked a segment");

	//	Phase 64's bridge half: a crossing states a clearance, or states nothing.
	BridgeDefinitionClass definition;
	Check (!definition.States_Clearance (),
			 "a fresh bridge definition claimed to state a clearance");
	definition.Set_Required_Clearance (5.0f);
	Check (definition.States_Clearance (),"a stated clearance did not take");
	Check (definition.Get_Required_Clearance () == 5.0f,"the stated clearance was wrong");
	definition.Set_Required_Clearance (-1.0f);
	Check (!definition.States_Clearance (),"a negative clearance was kept");

	BridgeClass bridge;
	Check (!bridge.States_Deck_Height (),"a fresh bridge claimed a deck height");
	Check (!bridge.Has_Abutment_Heights (),"a fresh bridge claimed conformed abutments");
	bridge.Set_Deck_Height (8.0f);
	Check (bridge.States_Deck_Height (),"the deck height did not take");
	Check (bridge.Get_Deck_Height () == 8.0f,"the deck height was wrong");

	return _Failures;
}


/***********************************************************************************************
 * Run_Traversability -- roadmap Section 52B, Phase 65.                                        *
 *   Building the graph needs a heightfield; the rules deciding whether an edge exists do not,   *
 *   and that is where the acceptance line lives -- a crossing a tank cannot use is absent from  *
 *   the tank's graph because Crossing_Carries says so.                                          *
 *=============================================================================================*/
int	Run_Traversability (void)
{
	//	A small step is walkable whatever slope it implies.
	Check (WorldTraversabilityGraph::Can_Step (0.2f,2.0f,0.25f,DEG_TO_RADF (45.0f)),
			 "a kerb inside the step height was not walkable");
	//	A big rise over a long run is a hill, and hills are walkable.
	Check (WorldTraversabilityGraph::Can_Step (1.0f,4.0f,0.25f,DEG_TO_RADF (45.0f)),
			 "a gentle hillside was not walkable");
	//	The same rise over no run at all is a wall.
	Check (!WorldTraversabilityGraph::Can_Step (4.0f,1.0f,0.25f,DEG_TO_RADF (45.0f)),
			 "a wall was reported as walkable");
	//	Down is the same as up, since falling off a cliff is not a route.
	Check (!WorldTraversabilityGraph::Can_Step (-4.0f,1.0f,0.25f,DEG_TO_RADF (45.0f)),
			 "a drop was treated differently from a rise");
	Check (!WorldTraversabilityGraph::Can_Step (1.0f,4.0f,0.25f,0.0f),
			 "something that cannot climb at all climbed a slope");

	//	Crossings.  This is the acceptance line.
	WorldMetricsStruct infantry;
	infantry.CorridorWidth = 1.2f;
	infantry.OverheadClearance = 2.0f;
	infantry.StandHeight = 1.8f;
	infantry.Valid = true;

	WorldMetricsStruct tank;
	tank.CorridorWidth = 5.0f;
	tank.OverheadClearance = 3.5f;
	tank.StandHeight = 3.0f;
	tank.Valid = true;

	//	A narrow footbridge: infantry only.
	Check (WorldTraversabilityGraph::Crossing_Carries (2.0f,0.0f,infantry),
			 "a footbridge refused infantry");
	Check (!WorldTraversabilityGraph::Crossing_Carries (2.0f,0.0f,tank),
			 "a footbridge carried a tank");

	//	A low underpass: infantry only, for the other reason.
	Check (WorldTraversabilityGraph::Crossing_Carries (10.0f,2.5f,infantry),
			 "a low underpass refused infantry");
	Check (!WorldTraversabilityGraph::Crossing_Carries (10.0f,2.5f,tank),
			 "a tank fitted under a span lower than it is tall");

	//	A crossing that states nothing carries everything, so nothing predating Phase 64 starts
	//	refusing traffic it always carried.
	Check (WorldTraversabilityGraph::Crossing_Carries (0.0f,0.0f,tank),
			 "a crossing that states nothing refused a tank");

	//	Wading: infantry do, ground vehicles do not, aircraft are not in the water.
	Check (WorldTraversabilityGraph::Compute_Wade_Depth (LOCOMOTION_INFANTRY,infantry) > 0.0f,
			 "infantry could not wade at all");
	Check (WorldTraversabilityGraph::Compute_Wade_Depth (LOCOMOTION_INFANTRY,infantry) <
			 infantry.StandHeight,"infantry waded over their own heads");
	Check (WorldTraversabilityGraph::Compute_Wade_Depth (LOCOMOTION_TRACKED,tank) == 0.0f,
			 "a tank forded a river");
	Check (WorldTraversabilityGraph::Compute_Wade_Depth (LOCOMOTION_FLYING,tank) > 100.0f,
			 "an aircraft was stopped by water");

	//	Nothing is built, so every query must answer safely rather than crash.
	Check (!WorldTraversabilityGraph::Is_Built (),"a graph existed before one was built");
	Check (WorldTraversabilityGraph::Find_Node (Vector3 (0.0f,0.0f,0.0f)) == -1,
			 "an unbuilt graph found a node");
	Check (!WorldTraversabilityGraph::Is_Reachable_Nodes (0,1),
			 "an unbuilt graph reported two nodes as connected");

	return _Failures;
}


}	// namespace


int	ShaderSelfCheck::Run (const char *which)
{
	if (which == nullptr) {
		which = "programs";
	}

	bool	ran = false;

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "programs") == 0)) {
		Run_Programs ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "pipelines") == 0)) {
		Run_Pipelines ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "particles") == 0)) {
		Run_Particles ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "weather") == 0)) {
		Run_Weather ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "tracers") == 0)) {
		Run_Tracers ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "debris") == 0)) {
		Run_Debris ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "attachments") == 0)) {
		Run_Attachments ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "radar") == 0)) {
		Run_Tactical_Intelligence ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "overlays") == 0)) {
		Run_Status_Overlays ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "guardai") == 0)) {
		Run_Guard_AI ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "telemetry") == 0)) {
		Run_Telemetry ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "debugrender") == 0)) {
		Run_Debug_Render ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "buildingjson") == 0)) {
		Run_Building_Descriptors ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "powergrid") == 0)) {
		Run_Power_Grid ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "commandermode") == 0)) {
		Run_Commander_Mode ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "sidebar") == 0)) {
		Run_Commander_Sidebar ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "placement") == 0)) {
		Run_Commander_Placement ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "construction") == 0)) {
		Run_Building_Construction ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "production") == 0)) {
		Run_Commander_Production ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "rtssaveload") == 0)) {
		Run_RTS_Persistence ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "visualsettings") == 0)) {
		Run_Visual_Settings ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "skyclock") == 0)) {
		Run_Sky_Clock ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "lightcurves") == 0)) {
		Run_Light_Curves ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "heightfog") == 0)) {
		Run_Height_Fog ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "skyocclusion") == 0)) {
		Run_Sky_Occlusion ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "skydriventoggles") == 0)) {
		Run_Sky_Driven_Toggles ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "tonemap") == 0)) {
		Run_Tonemap ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "bloom") == 0)) {
		Run_Bloom ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "windfield") == 0)) {
		Run_Wind_Field ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "wetness") == 0)) {
		Run_Wetness ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "damagetint") == 0)) {
		Run_Damage_Tint ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "weaponkick") == 0)) {
		Run_Weapon_Kick ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "worldmetrics") == 0)) {
		Run_World_Metrics ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "escarpment") == 0)) {
		Run_Escarpment ();
		ran = true;
	}

	if ((::strcmp (which, "all") == 0) || (::strcmp (which, "traversability") == 0)) {
		Run_Traversability ();
		ran = true;
	}


	if (!ran) {
		::fprintf (stderr, "unknown self check '%s'\n", which);
		return 2;
	}

	::fflush (stdout);
	::fflush (stderr);

	return (_Failures == 0) ? 0 : 1;
}
