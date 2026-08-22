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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : LevelEdit                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Tools/LevelEdit/ScriptMgr.cpp                $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 4/27/00 8:53a                                               $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "StdAfx.h"
#include "pathutil.h"
#include "scriptmgr.h"
#include "nativescriptregistry.h"
#include "scriptevents.H"
#include "EditScript.h"
#include "Utils.h"
#include "FileMgr.h"
#include "FileLocations.h"


//////////////////////////////////////////////////////////////////////////
//	Static member initialization
//////////////////////////////////////////////////////////////////////////
SCRIPT_LIST ScriptMgrClass::_ScriptList;


//////////////////////////////////////////////////////////////////////////
//
//	~ScriptMgrClass
//
//////////////////////////////////////////////////////////////////////////
ScriptMgrClass::~ScriptMgrClass (void)
{
	Shutdown ();
	return ;
}


//////////////////////////////////////////////////////////////////////////
//
//	Initialize
//
//////////////////////////////////////////////////////////////////////////
void
ScriptMgrClass::Initialize (void)
{
	//
	//	The built-in scripts are compiled into this program, the same catalog
	//	the game runs, so there is no DLL to enumerate.  Registration has
	//	already happened; indexing it is what puts it in name order and rejects
	//	a duplicate name.
	//
	NativeScriptRegistry::Build_Index ();

	for (int index = 0; index < NativeScriptRegistry::Count (); index ++) {
		ScriptFactoryClass *factory = NativeScriptRegistry::Peek (index);
		if (factory == nullptr) {
			continue;
		}

		EditScriptClass *script = new EditScriptClass;
		script->Set_Name (factory->Get_Name ());
		script->Set_Param_Desc (factory->Get_Parameter_Description ());

		_ScriptList.Add (script);
	}

	return ;
}


//////////////////////////////////////////////////////////////////////////
//
//	Shutdown
//
//////////////////////////////////////////////////////////////////////////
void
ScriptMgrClass::Shutdown (void)
{
	for (int index = 0; index < _ScriptList.Count (); index++) {
		EditScriptClass *script = _ScriptList[index];
		SAFE_DELETE (script);
	}

	_ScriptList.Delete_All ();
	return ;
}


//////////////////////////////////////////////////////////////////////////
//
//	Find_Script
//
//////////////////////////////////////////////////////////////////////////
EditScriptClass *
ScriptMgrClass::Find_Script (LPCTSTR name)
{
	EditScriptClass *script = nullptr;

	for (int index = 0; (index < _ScriptList.Count ()) && (script == nullptr); index++) {
		EditScriptClass *curr_script = _ScriptList[index];

		//
		//	Is this the script we are looking for?
		//
		if (::lstrcmpi (curr_script->Get_Name (), name) == 0) {
			script = curr_script;
		}
	}

	return script;
}
