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

#include "nativescriptregistry.h"

#include "wwdebug.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>


/*
**	Registration happens during static initialization, so the head pointer has
**	to be something the loader can zero before any constructor runs.  A raw
**	pointer and an intrusive list are: a container with its own constructor
**	would not be.
**
**	The sorted index is built later, on first use, when every translation unit
**	has finished registering.
*/
static ScriptFactoryClass *		_FactoryList			= nullptr;
static DynamicVectorClass<ScriptFactoryClass *> *	_SortedIndex	= nullptr;


////////////////////////////////////////////////////////////////
//
//	ScriptFactoryClass
//
////////////////////////////////////////////////////////////////
ScriptFactoryClass::ScriptFactoryClass
(
	const char *		name,
	const char *		parameters,
	ScriptSourceEnum	source,
	const char *		alias
)
	:	NextFactory (nullptr),
		ScriptName (name),
		ParameterDescription (parameters),
		Alias (alias),
		Source (source)
{
	WWASSERT (name != nullptr);
	WWASSERT (parameters != nullptr);

	NativeScriptRegistry::Register (this);
}


ScriptFactoryClass::~ScriptFactoryClass (void)
{
	NativeScriptRegistry::Unregister (this);

	ScriptName				= nullptr;
	ParameterDescription	= nullptr;
	Alias						= nullptr;
}


////////////////////////////////////////////////////////////////
//
//	Register
//
////////////////////////////////////////////////////////////////
void
NativeScriptRegistry::Register (ScriptFactoryClass *factory)
{
	if (factory == nullptr) {
		return ;
	}

	factory->Set_Next (_FactoryList);
	_FactoryList = factory;

	//
	//	Anything already built is now stale.  In practice this only happens
	//	during static initialization, before the index exists at all.
	//
	if (_SortedIndex != nullptr) {
		delete _SortedIndex;
		_SortedIndex = nullptr;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Unregister
//
////////////////////////////////////////////////////////////////
void
NativeScriptRegistry::Unregister (ScriptFactoryClass *factory)
{
	ScriptFactoryClass *previous	= nullptr;
	ScriptFactoryClass *current	= _FactoryList;

	while (current != nullptr) {
		ScriptFactoryClass *next = current->Get_Next ();

		if (current == factory) {
			if (previous == nullptr) {
				_FactoryList = next;
			} else {
				previous->Set_Next (next);
			}

			break;
		}

		previous	= current;
		current	= next;
	}

	if (_SortedIndex != nullptr) {
		delete _SortedIndex;
		_SortedIndex = nullptr;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Compare_Factories
//
// Script names are matched without regard to case, because level files have
// always been written with whatever case the designer typed.
//
////////////////////////////////////////////////////////////////
static int __cdecl
Compare_Factories (const void *left, const void *right)
{
	ScriptFactoryClass *factory1 = *(ScriptFactoryClass * const *)left;
	ScriptFactoryClass *factory2 = *(ScriptFactoryClass * const *)right;

	return ::_stricmp (factory1->Get_Name (), factory2->Get_Name ());
}


////////////////////////////////////////////////////////////////
//
//	Build_Index
//
////////////////////////////////////////////////////////////////
void
NativeScriptRegistry::Build_Index (void)
{
	if (_SortedIndex != nullptr) {
		delete _SortedIndex;
	}

	_SortedIndex = new DynamicVectorClass<ScriptFactoryClass *>;

	for (ScriptFactoryClass *factory = _FactoryList; factory != nullptr; factory = factory->Get_Next ()) {
		_SortedIndex->Add (factory);
	}

	int count = _SortedIndex->Count ();
	if (count > 1) {
		::qsort (&(*_SortedIndex)[0], count, sizeof (ScriptFactoryClass *), Compare_Factories);
	}

	//
	//	Two factories for one name means two implementations of the same
	//	script are compiled in, which the integration rules do not allow.  The
	//	build-time catalog check should have caught it; this catches the case
	//	where a name is produced at registration rather than written in the
	//	source.
	//
	for (int index = 1; index < count; index ++) {
		if (::_stricmp ((*_SortedIndex)[index - 1]->Get_Name (), (*_SortedIndex)[index]->Get_Name ()) == 0) {
			WWDEBUG_SAY (("Duplicate built-in script name '%s'\n", (*_SortedIndex)[index]->Get_Name ()));
			WWASSERT (!"duplicate built-in script name");
		}
	}

	WWDEBUG_SAY (("Native script registry: %d built-in scripts\n", count));
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Shutdown
//
////////////////////////////////////////////////////////////////
void
NativeScriptRegistry::Shutdown (void)
{
	//
	//	Only the index is owned here.  The factories themselves are static
	//	objects belonging to the translation units that declared them.
	//
	delete _SortedIndex;
	_SortedIndex = nullptr;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Find
//
////////////////////////////////////////////////////////////////
ScriptFactoryClass *
NativeScriptRegistry::Find (const char *name)
{
	if (name == nullptr) {
		return nullptr;
	}

	if (_SortedIndex == nullptr) {
		Build_Index ();
	}

	//
	//	Binary search over the sorted names.
	//
	int lower = 0;
	int upper = _SortedIndex->Count () - 1;

	while (lower <= upper) {
		int middle = lower + ((upper - lower) / 2);
		int result = ::_stricmp ((*_SortedIndex)[middle]->Get_Name (), name);

		if (result == 0) {
			return (*_SortedIndex)[middle];
		} else if (result < 0) {
			lower = middle + 1;
		} else {
			upper = middle - 1;
		}
	}

	//
	//	Not a current name.  A script that was renamed keeps its old name as an
	//	alias so that levels saved against it still load; aliases are rare
	//	enough that a linear sweep is the right cost.
	//
	for (int index = 0; index < _SortedIndex->Count (); index ++) {
		const char *alias = (*_SortedIndex)[index]->Get_Alias ();
		if (alias != nullptr && ::_stricmp (alias, name) == 0) {
			return (*_SortedIndex)[index];
		}
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////
//
//	Create
//
////////////////////////////////////////////////////////////////
ScriptClass *
NativeScriptRegistry::Create (const char *name)
{
	ScriptFactoryClass *factory = Find (name);

	if (factory == nullptr) {
		WWDEBUG_SAY (("Built-in script '%s' not found\n", (name != nullptr) ? name : "(null)"));
		return nullptr;
	}

	return factory->Create ();
}


////////////////////////////////////////////////////////////////
//
//	Count
//
////////////////////////////////////////////////////////////////
int
NativeScriptRegistry::Count (void)
{
	if (_SortedIndex == nullptr) {
		Build_Index ();
	}

	return _SortedIndex->Count ();
}


////////////////////////////////////////////////////////////////
//
//	Peek
//
////////////////////////////////////////////////////////////////
ScriptFactoryClass *
NativeScriptRegistry::Peek (int index)
{
	if (_SortedIndex == nullptr) {
		Build_Index ();
	}

	if (index < 0 || index >= _SortedIndex->Count ()) {
		return nullptr;
	}

	return (*_SortedIndex)[index];
}
