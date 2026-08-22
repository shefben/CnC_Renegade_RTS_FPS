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

#ifndef	NATIVESCRIPTREGISTRY_H
#define	NATIVESCRIPTREGISTRY_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

class	ScriptClass;


/*
**	Where a built-in script came from.
**
**	Recorded per script so the catalog can answer, at runtime and in the
**	editor, which implementation of a name is the live one.  There is only ever
**	one: a name that both the stock catalog and the 4.8.4 package define is
**	compiled once, from the merged source, and registers as STOCK_MERGED.
*/
enum ScriptSourceEnum
{
	SCRIPT_SOURCE_STOCK,			// stock Renegade script, untouched by 4.8.4
	SCRIPT_SOURCE_STOCK_MERGED,	// stock script with the 4.8.4 changes merged in
	SCRIPT_SOURCE_TT,				// introduced by the 4.8.4 package
	SCRIPT_SOURCE_OPENW3D,		// introduced by this project
};


/*
**	The registration record for one built-in script.
**
**	Concrete factories are produced by the ScriptRegistrant template on the
**	script side, which is the only thing that knows the script class.
*/
class	ScriptFactoryClass
{
public:

	virtual ~ScriptFactoryClass (void);

	const char *		Get_Name (void) const						{ return ScriptName; }
	const char *		Get_Parameter_Description (void) const	{ return ParameterDescription; }
	ScriptSourceEnum	Get_Source (void) const					{ return Source; }

	//
	//	A script renamed between versions keeps its old name here, so a level
	//	saved against the old name still finds its script.  Null when the
	//	script has never been renamed, which is the normal case.
	//
	const char *		Get_Alias (void) const						{ return Alias; }

	virtual ScriptClass *	Create (void) = 0;

	//
	//	Registry linkage.  Public because registration happens during static
	//	initialization, before anything could grant friendship.
	//
	ScriptFactoryClass *	Get_Next (void) const					{ return NextFactory; }
	void						Set_Next (ScriptFactoryClass *next)	{ NextFactory = next; }

protected:

	ScriptFactoryClass (const char *name, const char *parameters,
			ScriptSourceEnum source = SCRIPT_SOURCE_STOCK, const char *alias = nullptr);

private:

	ScriptFactoryClass *	NextFactory;
	const char *			ScriptName;
	const char *			ParameterDescription;
	const char *			Alias;
	ScriptSourceEnum		Source;
};


/*
**	The built-in script catalog.
**
**	This is what replaced loading SCRIPTS.DLL and calling through its exported
**	Create_Script.  Every built-in script is compiled into the program and adds
**	itself here during static initialization; a name resolves to exactly one
**	implementation, and asking for a name that is not registered is an error
**	rather than a silently absent script.
**
**	Registration order is not meaningful and is not relied upon: the catalog is
**	sorted by name the first time it is used, which is also when duplicate
**	names are detected.
*/
class	NativeScriptRegistry
{
public:

	//
	//	Called from the ScriptRegistrant constructor during static
	//	initialization.  Nothing but linking happens here -- the registry is
	//	not usable until Build_Index has run.
	//
	static void		Register (ScriptFactoryClass *factory);
	static void		Unregister (ScriptFactoryClass *factory);

	//
	//	Sorts the catalog and fails on a duplicate name.  Safe to call more
	//	than once; a second call after new registrations rebuilds the index.
	//
	static void		Build_Index (void);
	static void		Shutdown (void);

	static ScriptClass *			Create (const char *name);
	static ScriptFactoryClass *	Find (const char *name);

	//
	//	Enumeration, for the editor catalog and for diagnostics.  Index order
	//	is the sorted order, so it is stable between runs.
	//
	static int							Count (void);
	static ScriptFactoryClass *	Peek (int index);
};

#endif	// NATIVESCRIPTREGISTRY_H
