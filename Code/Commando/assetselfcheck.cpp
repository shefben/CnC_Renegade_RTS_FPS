//
//	Asset exclusion and residency, checked from inside the binary that has to be right.
//	See assetselfcheck.h for why the checks live here.
//

#include "assetselfcheck.h"

#include "assetresidency.h"
#include "w3dexclusionlist.h"
#include "vector.h"
#include "wwstring.h"

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


/***********************************************************************************************
**	exclusion -- the w3d naming conventions the keep-list matches on
**
**	These are not conventions this project invented.  They are how stock Renegade content is
**	already named, and getting one of them wrong is how a level change quietly frees a model
**	the next level is still holding, or keeps every run-time-munged object forever.
***********************************************************************************************/

int	Run_Exclusion (void)
{
	DynamicVectorClass<StringClass>	names;
	names.Add (StringClass ("c_ag_gdi"));		// a character
	names.Add (StringClass ("v_gdi_mrls"));	// a vehicle
	names.Add (StringClass ("mx0_bldg"));		// a piece of level geometry

	W3DExclusionListClass	list (names);

	Check (list.Count () == 3, "the exclusion list lost a name: %d of 3", list.Count ());

	//
	//	A named file is kept.
	//
	Check (list.Is_Excluded ("c_ag_gdi"),
			"a named model is not kept");
	Check (list.Is_Excluded_Model_Name ("v_gdi_mrls"),
			"a named vehicle is not kept");

	//
	//	Naming a file keeps its sub-objects.  A stock w3d holds many of these and freeing
	//	them while keeping the file they came from is what leaves a model half loaded.
	//
	Check (list.Is_Excluded_Model_Name ("v_gdi_mrls.turret"),
			"a sub-object of a named model is not kept");
	Check (list.Is_Excluded_Model_Name ("mx0_bldg.mesh01"),
			"a sub-object of named level geometry is not kept");

	//
	//	Names are compared without regard to case.  Stock content is not consistent about it
	//	and the hierarchy tree manager already lower-cases everything it hashes.
	//
	Check (list.Is_Excluded_Model_Name ("V_GDI_MRLS"),
			"an upper case model name is not matched");
	Check (list.Is_Excluded_Model_Name ("C_AG_GDI.Head"),
			"a mixed case sub-object name is not matched");

	//
	//	A run-time munged object belongs to whatever built it, never to the list.
	//
	Check (list.Is_Excluded_Model_Name ("v_gdi_mrls#01") == false,
			"a munged object was kept");
	Check (list.Is_Excluded_Model_Name ("c_ag_gdi.arm#02") == false,
			"a munged sub-object was kept");

	//
	//	Something nobody named is released, which is the entire point.
	//
	Check (list.Is_Excluded_Model_Name ("v_nod_buggy") == false,
			"an unnamed model was kept");
	Check (list.Is_Excluded_Model_Name ("v_nod_buggy.turret") == false,
			"a sub-object of an unnamed model was kept");

	//
	//	Animations are "<skeleton>.<file>", so it is the part after the dot that is the file.
	//	Matching the skeleton instead would keep every animation of a kept character and free
	//	the shared ones, which is exactly backwards.
	//
	Check (list.Is_Excluded_Anim_Name ("s_a_human.c_ag_gdi"),
			"an animation from a named file is not kept");
	Check (list.Is_Excluded_Anim_Name ("c_ag_gdi.h_a_a0a0") == false,
			"an animation was matched on its skeleton rather than its file");
	Check (list.Is_Excluded_Anim_Name ("c_ag_gdi") == false,
			"an animation with no file part was kept");

	//
	//	Degenerate input must not be a crash.  Names come out of files.
	//
	Check (list.Is_Excluded (static_cast<const char *> (nullptr)) == false,
			"a null name was kept");
	Check (list.Is_Excluded_Model_Name ("") == false,
			"an empty name was kept");

	::fprintf (stdout, "exclusion: %d names, w3d naming conventions honoured\n",
			list.Count ());
	return _Failures;
}


/***********************************************************************************************
**	residency -- scope ordering, dependency closure and the diagnostics
***********************************************************************************************/

bool	List_Contains (const DynamicVectorClass<StringClass> &names, const char *wanted)
{
	StringClass	target = AssetResidencyManagerClass::Normalize_Name (wanted);
	for (int index = 0; index < names.Count (); index ++) {
		if (AssetResidencyManagerClass::Normalize_Name (names[index]) == target) {
			return true;
		}
	}
	return false;
}

int	Run_Residency (void)
{
	//
	//	A local manager, not the engine's.  Nothing is loaded in this process, so releasing
	//	a scope here exercises the arithmetic and finds no asset manager to call.
	//
	AssetResidencyManagerClass	residency;

	residency.Register_Asset ("hud_reticle",	ASSET_KIND_TEXTURE,			ASSET_SCOPE_PERMANENT,	4096);
	residency.Register_Asset ("c_ag_gdi",		ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_GAME_MODE,	16384);
	residency.Register_Asset ("mx0_bldg",		ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_WORLD,		65536);
	residency.Register_Asset ("mx0_tree01",	ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_SECTOR,		8192);
	residency.Register_Asset ("expl_flash",	ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_TRANSIENT,	1024);
	residency.Register_Asset ("s_a_human",		ASSET_KIND_HIERARCHY_TREE,	ASSET_SCOPE_TRANSIENT,	2048);

	Check (residency.Get_Asset_Count () == 6,
			"6 registrations became %d records", residency.Get_Asset_Count ());

	//
	//	Registering a name twice keeps the longer-lived scope, whichever order it arrives in.
	//
	residency.Register_Asset ("c_ag_gdi", ASSET_KIND_PROTOTYPE, ASSET_SCOPE_WORLD);
	Check (residency.Get_Asset_Scope ("c_ag_gdi") == ASSET_SCOPE_GAME_MODE,
			"a second shorter-lived registration shortened an asset's life");
	residency.Register_Asset ("mx0_bldg", ASSET_KIND_PROTOTYPE, ASSET_SCOPE_PERMANENT);
	Check (residency.Get_Asset_Scope ("mx0_bldg") == ASSET_SCOPE_PERMANENT,
			"a second longer-lived registration did not extend an asset's life");
	residency.Register_Asset ("mx0_bldg", ASSET_KIND_PROTOTYPE, ASSET_SCOPE_WORLD);
	Check (residency.Get_Asset_Count () == 6,
			"re-registering a name added a record: %d", residency.Get_Asset_Count ());

	//	Put it back where the rest of this check expects it.
	AssetResidencyManagerClass	scoped;
	scoped.Register_Asset ("hud_reticle",	ASSET_KIND_TEXTURE,			ASSET_SCOPE_PERMANENT,	4096);
	scoped.Register_Asset ("c_ag_gdi",		ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_GAME_MODE,	16384);
	scoped.Register_Asset ("mx0_bldg",		ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_WORLD,		65536);
	scoped.Register_Asset ("mx0_tree01",	ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_SECTOR,		8192);
	scoped.Register_Asset ("expl_flash",	ASSET_KIND_PROTOTYPE,		ASSET_SCOPE_TRANSIENT,	1024);
	scoped.Register_Asset ("s_a_human",		ASSET_KIND_HIERARCHY_TREE,	ASSET_SCOPE_TRANSIENT,	2048);

	//
	//	Names are matched the way the exclusion list matches them.
	//
	Check (scoped.Is_Registered ("C_AG_GDI"),
			"an upper case name did not find its record");

	//
	//	Releasing WORLD keeps PERMANENT and GAME_MODE and nothing else, before dependencies.
	//
	DynamicVectorClass<StringClass>	retained;
	scoped.Build_Retained_List (ASSET_SCOPE_WORLD, retained);

	Check (List_Contains (retained, "hud_reticle"), "releasing WORLD dropped a PERMANENT asset");
	Check (List_Contains (retained, "c_ag_gdi"),    "releasing WORLD dropped a GAME_MODE asset");
	Check (List_Contains (retained, "mx0_bldg") == false,   "releasing WORLD kept the WORLD asset");
	Check (List_Contains (retained, "mx0_tree01") == false, "releasing WORLD kept a SECTOR asset");
	Check (List_Contains (retained, "expl_flash") == false, "releasing WORLD kept a TRANSIENT asset");
	Check (retained.Count () == 2, "releasing WORLD retained %d names, not 2", retained.Count ());

	//
	//	Releasing SECTOR keeps everything longer-lived than a sector, including the world.
	//
	DynamicVectorClass<StringClass>	sector_retained;
	scoped.Build_Retained_List (ASSET_SCOPE_SECTOR, sector_retained);
	Check (List_Contains (sector_retained, "mx0_bldg"),
			"releasing SECTOR dropped the WORLD asset");
	Check (List_Contains (sector_retained, "mx0_tree01") == false,
			"releasing SECTOR kept the SECTOR asset");
	Check (sector_retained.Count () == 3,
			"releasing SECTOR retained %d names, not 3", sector_retained.Count ());

	//
	//	A dependency drags a shorter-lived asset up with its owner.  A character kept for the
	//	game mode needs its skeleton, whatever scope the skeleton was first seen in.
	//
	scoped.Add_Dependency ("c_ag_gdi", "s_a_human");
	scoped.Add_Dependency ("c_ag_gdi", "s_a_human");	// twice is once
	Check (scoped.Get_Dependency_Count () == 1,
			"a repeated dependency was stored twice: %d", scoped.Get_Dependency_Count ());

	DynamicVectorClass<StringClass>	with_dependency;
	scoped.Build_Retained_List (ASSET_SCOPE_WORLD, with_dependency);
	Check (List_Contains (with_dependency, "s_a_human"),
			"a TRANSIENT asset a kept asset depends on was released");

	//
	//	Closure is transitive, and a cycle terminates rather than recursing forever.
	//
	scoped.Register_Asset ("s_a_human_bones", ASSET_KIND_ANIMATION, ASSET_SCOPE_TRANSIENT, 512);
	scoped.Add_Dependency ("s_a_human", "s_a_human_bones");
	scoped.Add_Dependency ("s_a_human_bones", "c_ag_gdi");	// back to the top

	DynamicVectorClass<StringClass>	transitive;
	scoped.Build_Retained_List (ASSET_SCOPE_WORLD, transitive);
	Check (List_Contains (transitive, "s_a_human_bones"),
			"the dependency closure stopped after one step");
	Check (transitive.Count () == 4,
			"the closure retained %d names, not 4", transitive.Count ());

	//
	//	An asset depending on itself is not a dependency.
	//
	int	before = scoped.Get_Dependency_Count ();
	scoped.Add_Dependency ("mx0_bldg", "mx0_bldg");
	Check (scoped.Get_Dependency_Count () == before,
			"an asset was recorded as depending on itself");

	//
	//	Per-scope memory is per scope, and the total is the sum of the scopes.
	//
	Check (scoped.Get_Scope_Memory (ASSET_SCOPE_PERMANENT) == 4096,
			"PERMANENT reports %u bytes, not 4096",
			scoped.Get_Scope_Memory (ASSET_SCOPE_PERMANENT));
	Check (scoped.Get_Scope_Memory (ASSET_SCOPE_TRANSIENT) == (1024 + 2048 + 512),
			"TRANSIENT reports %u bytes, not %u",
			scoped.Get_Scope_Memory (ASSET_SCOPE_TRANSIENT), 1024 + 2048 + 512);

	unsigned	summed = 0;
	for (int scope = 0; scope < ASSET_SCOPE_COUNT; scope ++) {
		summed += scoped.Get_Scope_Memory (static_cast<AssetScopeType> (scope));
	}
	Check (summed == scoped.Get_Total_Memory (),
			"the scopes sum to %u bytes but the total says %u",
			summed, scoped.Get_Total_Memory ());

	//
	//	Kind counts are what the report prints.
	//
	Check (scoped.Get_Kind_Asset_Count (ASSET_KIND_PROTOTYPE) == 4,
			"%d prototypes counted, not 4",
			scoped.Get_Kind_Asset_Count (ASSET_KIND_PROTOTYPE));
	Check (scoped.Get_Kind_Asset_Count (ASSET_KIND_TEXTURE) == 1,
			"%d textures counted, not 1",
			scoped.Get_Kind_Asset_Count (ASSET_KIND_TEXTURE));

	//
	//	Releasing a scope forgets the records it released and keeps the rest, and the
	//	dependency edges of released assets go with them.  Nothing is loaded in this
	//	process, so no asset manager is asked to free anything.
	//
	scoped.Release_Scope (ASSET_SCOPE_WORLD);
	Check (scoped.Get_Asset_Count () == 2,
			"after releasing WORLD, %d records remain, not 2", scoped.Get_Asset_Count ());
	Check (scoped.Is_Registered ("hud_reticle"), "releasing WORLD forgot a PERMANENT record");
	Check (scoped.Is_Registered ("c_ag_gdi"),    "releasing WORLD forgot a GAME_MODE record");
	Check (scoped.Is_Registered ("mx0_bldg") == false,
			"releasing WORLD kept the WORLD record");
	Check (scoped.Get_Dependency_Count () == 1,
			"%d dependency edges survived their owners, not 1",
			scoped.Get_Dependency_Count ());

	//
	//	The name index has to survive that rebuild, or the next registration collides.
	//
	scoped.Register_Asset ("mx1_bldg", ASSET_KIND_PROTOTYPE, ASSET_SCOPE_WORLD, 32768);
	Check (scoped.Is_Registered ("mx1_bldg"),
			"a registration after a release did not take");
	Check (scoped.Get_Asset_Scope ("hud_reticle") == ASSET_SCOPE_PERMANENT,
			"the name index lost track of a record when the vector was rebuilt");

	//
	//	Repeating the whole load/release cycle must not accumulate anything.  This is the
	//	acceptance condition of Section 14 stated as arithmetic.
	//
	AssetResidencyManagerClass	cycled;
	cycled.Register_Asset ("hud_reticle", ASSET_KIND_TEXTURE, ASSET_SCOPE_PERMANENT, 4096);

	for (int pass = 0; pass < 16; pass ++) {
		cycled.Register_Asset ("mx0_bldg",  ASSET_KIND_PROTOTYPE, ASSET_SCOPE_WORLD, 65536);
		cycled.Register_Asset ("mx0_tree01", ASSET_KIND_PROTOTYPE, ASSET_SCOPE_SECTOR, 8192);
		cycled.Add_Dependency ("mx0_bldg", "mx0_tree01");
		cycled.Release_Scope (ASSET_SCOPE_WORLD);
	}

	Check (cycled.Get_Asset_Count () == 1,
			"16 load/release cycles left %d records, not 1", cycled.Get_Asset_Count ());
	Check (cycled.Get_Dependency_Count () == 0,
			"16 load/release cycles left %d dependency edges, not 0",
			cycled.Get_Dependency_Count ());
	Check (cycled.Is_Registered ("hud_reticle"),
			"16 load/release cycles invalidated the retained PERMANENT asset");
	Check (cycled.Get_Total_Memory () == 4096,
			"16 load/release cycles left %u bytes accounted, not 4096",
			cycled.Get_Total_Memory ());

	::fprintf (stdout, "residency: 5 scopes, closure over %d edges, 16 cycles leak nothing\n",
			scoped.Get_Dependency_Count ());
	return _Failures;
}

}	// namespace


int	AssetSelfCheck::Run (const char *which)
{
	if (which == nullptr) {
		which = "residency";
	}

	if (::strcmp (which, "exclusion") == 0) {
		Run_Exclusion ();
	} else if (::strcmp (which, "residency") == 0) {
		Run_Residency ();
	} else {
		::fprintf (stderr, "unknown self check '%s'\n", which);
		return 2;
	}

	::fflush (stdout);
	::fflush (stderr);

	return (_Failures == 0) ? 0 : 1;
}
