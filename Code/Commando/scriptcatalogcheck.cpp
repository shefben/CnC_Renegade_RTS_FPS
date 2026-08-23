//
//	The built-in script catalog, checked against what the source says it is.
//	See scriptcatalogcheck.h for why the check lives inside this program.
//

#include "scriptcatalogcheck.h"

#include "nativescriptregistry.h"
#include "scriptevents.h"
#include "scriptman.h"

#include "../Scripts/tests/ScriptCatalogManifest.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

int	_Failures = 0;

void	Fail (const char *format, ...)
{
	va_list args;
	va_start (args, format);
	::fprintf (stderr, "FAIL: ");
	::vfprintf (stderr, format, args);
	::fprintf (stderr, "\n");
	va_end (args);

	_Failures ++;
	return ;
}

//
//	A failure per missing script would bury the useful line under two thousand
//	others, so only the first few of any one kind are named.
//
const int	MAX_REPORTED	= 10;

//
//	`Find` is case-insensitive and follows aliases, which is what a level and a
//	save file both rely on.
//
ScriptFactoryClass *	Lookup (const char *name)
{
	return NativeScriptRegistry::Find (name);
}

int	Check_Present (const char * const *names, int count, const char *what,
		bool require_tt)
{
	int missing	= 0;
	int mistagged	= 0;

	for (int index = 0; index < count; index ++) {
		ScriptFactoryClass *factory = Lookup (names[index]);

		if (factory == nullptr) {
			if (missing < MAX_REPORTED) {
				Fail ("%s script '%s' is declared in the source but is not in the registry",
						what, names[index]);
			}
			missing ++;
			continue;
		}

		const bool is_tt = (factory->Get_Source () == SCRIPT_SOURCE_TT);

		if (require_tt != is_tt) {
			if (mistagged < MAX_REPORTED) {
				Fail ("script '%s' is registered with the wrong source tag (%d)",
						names[index], (int)factory->Get_Source ());
			}
			mistagged ++;
		}
	}

	if (missing > MAX_REPORTED) {
		Fail ("... and %d more %s scripts are missing", missing - MAX_REPORTED, what);
	}
	if (mistagged > MAX_REPORTED) {
		Fail ("... and %d more %s scripts are tagged wrongly", mistagged - MAX_REPORTED, what);
	}

	return count - missing;
}


int	Run_Stock (void)
{
	const int found = Check_Present (ScriptCatalogManifest::StockNames,
			ScriptCatalogManifest::StockCount, "stock", false);

	//
	//	A merged script is a stock script carrying the 4.8.4 corrections.  It
	//	keeps its stock name and must not be reported as a 4.8.4 script, or a
	//	level that names it stops finding it.
	//
	for (int index = 0; index < ScriptCatalogManifest::MergedCount; index ++) {
		const char *name = ScriptCatalogManifest::MergedNames[index];
		ScriptFactoryClass *factory = Lookup (name);

		if (factory == nullptr) {
			continue;		//	already reported above
		}

		if (factory->Get_Source () != SCRIPT_SOURCE_STOCK_MERGED) {
			Fail ("merged script '%s' is not tagged SCRIPT_SOURCE_STOCK_MERGED", name);
		}
	}

	::printf ("stock: %d of %d present, %d of them merged\n", found,
			ScriptCatalogManifest::StockCount, ScriptCatalogManifest::MergedCount);
	return _Failures;
}


int	Run_TT (void)
{
	const int found = Check_Present (ScriptCatalogManifest::TTNames,
			ScriptCatalogManifest::TTCount, "4.8.4", true);

	//
	//	The count matters as much as the names: a 4.8.4 script that arrived
	//	without being written into the manifest is just as much a surprise as
	//	one that went missing.
	//
	int registered = 0;
	const int total = NativeScriptRegistry::Count ();
	for (int index = 0; index < total; index ++) {
		ScriptFactoryClass *factory = NativeScriptRegistry::Peek (index);
		if ((factory != nullptr) && (factory->Get_Source () == SCRIPT_SOURCE_TT)) {
			registered ++;
		}
	}

	if (registered != ScriptCatalogManifest::TTCount) {
		Fail ("the registry holds %d 4.8.4 scripts, the source declares %d",
				registered, ScriptCatalogManifest::TTCount);
	}

	::printf ("4.8.4: %d of %d present, %d registered\n", found,
			ScriptCatalogManifest::TTCount, registered);
	return _Failures;
}


int	Run_Duplicates (void)
{
	const int total = NativeScriptRegistry::Count ();

	if (total != ScriptCatalogManifest::ScriptCount) {
		Fail ("the registry holds %d scripts, the source declares %d",
				total, ScriptCatalogManifest::ScriptCount);
	}

	//
	//	The index is sorted case-insensitively, so a duplicate name is a pair of
	//	neighbours.  This is the collision that compiles cleanly and then races
	//	during static initialisation, which is why it is looked for here rather
	//	than being left to whichever registrant happens to run last.
	//
	int duplicates = 0;
	for (int index = 0; index < total; index ++) {
		ScriptFactoryClass *factory = NativeScriptRegistry::Peek (index);

		if (factory == nullptr) {
			Fail ("the registry has a null entry at %d", index);
			continue;
		}

		if ((factory->Get_Name () == nullptr) || (factory->Get_Name ()[0] == 0)) {
			Fail ("the registry has an unnamed entry at %d", index);
			continue;
		}

		if (factory->Get_Parameter_Description () == nullptr) {
			Fail ("script '%s' has no parameter description", factory->Get_Name ());
		}

		if (index == 0) {
			continue;
		}

		ScriptFactoryClass *previous = NativeScriptRegistry::Peek (index - 1);
		if ((previous == nullptr) || (previous->Get_Name () == nullptr)) {
			continue;
		}

		const int order = ::_stricmp (previous->Get_Name (), factory->Get_Name ());

		if (order == 0) {
			if (duplicates < MAX_REPORTED) {
				Fail ("two built-in scripts answer to the name '%s'", factory->Get_Name ());
			}
			duplicates ++;
		} else if (order > 0) {
			Fail ("the registry index is not sorted at %d ('%s' before '%s')",
					index, previous->Get_Name (), factory->Get_Name ());
		}
	}

	if (duplicates > MAX_REPORTED) {
		Fail ("... and %d more duplicate names", duplicates - MAX_REPORTED);
	}

	//
	//	An alias is the second name a merged script answers to.  It has no
	//	factory of its own, so it must resolve, must not resolve to itself, and
	//	must not collide with a real script name.
	//
	for (int index = 0; index < ScriptCatalogManifest::AliasCount; index ++) {
		const char *alias = ScriptCatalogManifest::AliasNames[index];

		ScriptFactoryClass *factory = Lookup (alias);

		if (factory == nullptr) {
			Fail ("the alias '%s' resolves to nothing", alias);
			continue;
		}

		if (::_stricmp (factory->Get_Name (), alias) == 0) {
			Fail ("the alias '%s' is registered as a script in its own right", alias);
			continue;
		}

		if (!NativeScriptRegistry::Alias_Matches (factory->Get_Alias (), alias)) {
			Fail ("'%s' resolved to '%s', which does not list it as an alias",
					alias, factory->Get_Name ());
		}
	}

	::printf ("catalog: %d scripts, %d aliases, no duplicate names\n", total,
			ScriptCatalogManifest::AliasCount);
	return _Failures;
}


int	Run_Recreate (void)
{
	//
	//	What a load does: read a name and a parameter string out of the save
	//	file, ask the manager for a script of that name, and hand the parameters
	//	back.  Every entry in the catalog has to survive that, or a saved game
	//	naming it comes back without it.
	//
	ScriptManager::Init ();

	const int total = NativeScriptRegistry::Count ();
	int created = 0;
	int failures_before = _Failures;
	int truncated = 0;

	for (int index = 0; index < total; index ++) {
		ScriptFactoryClass *factory = NativeScriptRegistry::Peek (index);
		if (factory == nullptr) {
			continue;
		}

		//	the name is copied: creating a script rebuilds the index, which
		//	invalidates the factory pointer but never the string it named.
		char name[128];
		::strncpy (name, factory->Get_Name (), sizeof (name) - 1);
		name[sizeof (name) - 1] = 0;

		ScriptClass *script = ScriptManager::Create_Script (name);

		if (script == nullptr) {
			if ((_Failures - failures_before) < MAX_REPORTED) {
				Fail ("script '%s' is in the catalog but could not be created", name);
			}
			continue;
		}

		created ++;

		if (::_stricmp (script->Get_Name (), name) != 0) {
			Fail ("a script created as '%s' calls itself '%s'", name, script->Get_Name ());
		}

		//
		//	The save file carries the parameter string through a 256 byte
		//	buffer, so a script whose parameters do not fit comes back with
		//	them cut short.  Counted rather than failed: it is a property of
		//	the save format, and the count is what says whether it has grown.
		//
		char parameters[256];
		parameters[0] = 0;
		script->Get_Parameters_String (parameters, sizeof (parameters));

		if (::strlen (parameters) == (sizeof (parameters) - 1)) {
			truncated ++;
		} else {
			script->Set_Parameters_String (parameters);

			char again[256];
			again[0] = 0;
			script->Get_Parameters_String (again, sizeof (again));

			if (::strcmp (parameters, again) != 0) {
				Fail ("script '%s' does not round-trip its parameter string", name);
			}
		}

		ScriptManager::Request_Destroy_Script (script);
		ScriptManager::Destroy_Pending ();
	}

	if (created != total) {
		Fail ("%d of %d catalog entries could be created", created, total);
	}

	::printf ("recreate: %d of %d scripts rebuilt by name, %d parameter strings"
			" longer than the save buffer\n", created, total, truncated);

	ScriptManager::Shutdown ();
	return _Failures;
}

}	// namespace


int	ScriptCatalogCheck::Run (const char *which)
{
	if (which == nullptr) {
		which = "duplicates";
	}

	//	the registry is filled by static initialisation, so it is already there
	NativeScriptRegistry::Build_Index ();

	if (::strcmp (which, "stock") == 0) {
		Run_Stock ();
	} else if (::strcmp (which, "tt") == 0) {
		Run_TT ();
	} else if (::strcmp (which, "duplicates") == 0) {
		Run_Duplicates ();
	} else if (::strcmp (which, "recreate") == 0) {
		Run_Recreate ();
	} else {
		::fprintf (stderr, "unknown script catalog check '%s'\n", which);
		return 2;
	}

	::fflush (stdout);
	::fflush (stderr);

	return (_Failures == 0) ? 0 : 1;
}
