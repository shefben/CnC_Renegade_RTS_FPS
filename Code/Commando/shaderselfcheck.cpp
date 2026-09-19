//
//	The shader and state management layer, checked from inside the binary that has to be right.
//	See shaderselfcheck.h for why the checks live here.
//

#include "shaderselfcheck.h"

#include "buildingdescriptormanager.h"
#include "commandermodecontroller.h"
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
#include "rendobj.h"
#include "rinfo.h"
#include "vector3.h"
#include "vector4.h"

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

	if (!ran) {
		::fprintf (stderr, "unknown self check '%s'\n", which);
		return 2;
	}

	::fflush (stdout);
	::fflush (stderr);

	return (_Failures == 0) ? 0 : 1;
}
