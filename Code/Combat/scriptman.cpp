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
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/scripts.cpp                           $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/09/02 9:19a                                               $*
 *                                                                                             *
 *                    $Revision:: 53                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "scriptman.h"
#include "debug.h"
#include "nativescriptregistry.h"
#include "scriptcommands.h"
#include "physicalgameobj.h"
#include "wwstring.h"
#include "combat.h"
#include "wwprofile.h"
#include "ffactorylist.h"
#include "rawfile.h"
#include "gametype.h"
#include "soutil.h"
#include <stdio.h>
#include <win.h>

#if 1
#define	SCRIPT_PROFILE_START( x )	WWProfileManager::Profile_Start( "Scripts" );
#define	SCRIPT_PROFILE_STOP( x )	WWProfileManager::Profile_Stop( );
#else
#define	SCRIPT_PROFILE_START( x )
#define	SCRIPT_PROFILE_STOP( x )
#endif

/*
**
*/
SimpleDynVecClass<ScriptClass *> ScriptManager::ActiveScriptList;
SimpleDynVecClass<ScriptClass *> ScriptManager::PendingDestroyList;
bool	ScriptManager::EnableScriptCreation = true;



/*
**
*/
void ScriptManager::Init(void)
{
	//
	//	Every built-in script is compiled into this program and has already
	//	added itself to the registry during static initialization.  All that is
	//	left is to index the catalog, which is also where a duplicate name is
	//	caught.
	//
	NativeScriptRegistry::Build_Index();
}

void ScriptManager::Shutdown(void)
{
	while (ActiveScriptList.Count()) {
		ScriptClass* script = ActiveScriptList[0];
		assert(script != nullptr);

		delete script;

		ActiveScriptList.Delete(0);
	}

	NativeScriptRegistry::Shutdown();
}

void ScriptManager::Destroy_Pending(void)
{
	while (PendingDestroyList.Count()) {
		ScriptClass* script = PendingDestroyList[0];
		assert(script != nullptr);

		ScriptableGameObj* object = script->Owner();

		if (object != nullptr) {
			object->Remove_Observer(script);
		}

		delete script;
		PendingDestroyList.Delete(0);
	}
}

ScriptClass* ScriptManager::Create_Script(const char* script_name)
{
	ScriptClass* script = nullptr;

	if (EnableScriptCreation) {
		script = NativeScriptRegistry::Create(script_name);

		if (script != nullptr) {
			script->Set_ID( GameObjObserverManager::Get_Next_Observer_ID() );
			ActiveScriptList.Add(script);
		}
	}

	return script;
}

void ScriptManager::Request_Destroy_Script(ScriptClass* script)
{
	ActiveScriptList.Delete(script);

	// Do not add the script to the destroy list if it is already there.
	for (int index = 0; index < PendingDestroyList.Count(); index++) {
		if (PendingDestroyList[index] == script) {
			return;
		}
	}

	PendingDestroyList.Add(script);
}


/*
** Script Manager Save and Load
*/
enum	{
	CHUNKID_SCRIPT_ENTRY					=	131001134,
	CHUNKID_SCRIPT_HEADER,
	CHUNKID_SCRIPT_DATA,

	MICROCHUNKID_NAME						= 1,

// Denzil 3/31/00 - This information is now saved by the script.
#if(0)
	MICROCHUNKID_PARAM_COUNT,
#endif
	MICROCHUNKID_PARAM,
	MICROCHUNKID_GAME_OBJ_OBSERVER_PTR,
	MICROCHUNKID_OWNER_PTR,
	MICROCHUNKID_ID,
};


/*
**
*/
bool ScriptManager::Save(ChunkSaveClass& csave)
{
	for (int index = 0; index < ActiveScriptList.Count(); index++) {
		ScriptClass* script = ActiveScriptList[ index ];

		csave.Begin_Chunk( CHUNKID_SCRIPT_ENTRY );
		csave.Begin_Chunk( CHUNKID_SCRIPT_HEADER );

		StringClass name = script->Get_Name();
//		Debug_Say(("Saving script '%s'\n", name));
		WRITE_MICRO_CHUNK_WWSTRING( csave, MICROCHUNKID_NAME, name );

		char paramString[256];
		script->Get_Parameters_String(paramString, sizeof(paramString));
//		Debug_Say(("\tParameters: '%s'\n", paramString));
		WRITE_MICRO_CHUNK_STRING(csave, MICROCHUNKID_PARAM, paramString);

		GameObjObserverClass* game_obj_observer_ptr = (GameObjObserverClass*)script;
		WRITE_MICRO_CHUNK_PTR( csave, MICROCHUNKID_GAME_OBJ_OBSERVER_PTR, game_obj_observer_ptr );

		ScriptableGameObj* owner_ptr = *(script->Get_Owner_Ptr());
//		Debug_Say(("\tObjectPtr: '%p'\n", *owner_ptr));
		WRITE_MICRO_CHUNK_PTR( csave, MICROCHUNKID_OWNER_PTR, owner_ptr );

		int id = script->Get_ID();
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_ID, id );
//		Debug_Say(( "Saved Script ID %d\n", id ));

		csave.End_Chunk();

		// If data is not saved, script will be re-created
		if (CombatManager::Are_Observers_Active()) {
			csave.Begin_Chunk(CHUNKID_SCRIPT_DATA);
			ScriptSaver	saver(csave);
			script->Save(saver);
			csave.End_Chunk();
		}

		csave.End_Chunk();
	}
	return true;
}


bool	ScriptManager::Load( ChunkLoadClass & cload )
{
	WWASSERT( ActiveScriptList.Count() == 0 );

	while (cload.Open_Chunk()) {

		GameObjObserverClass * game_obj_observer_ptr = nullptr;
		PhysicalGameObj * owner_ptr = nullptr;

		WWASSERT( cload.Cur_Chunk_ID() == CHUNKID_SCRIPT_ENTRY );

		ScriptClass *script = nullptr;

		// Load header
		cload.Open_Chunk();
		WWASSERT( cload.Cur_Chunk_ID() == CHUNKID_SCRIPT_HEADER );

		int obs_id = -1;

//		int param_index = 0;
		while (cload.Open_Micro_Chunk()) {
			int id = cload.Cur_Micro_Chunk_ID();
			switch( id ) {
				case MICROCHUNKID_NAME:
				{
					StringClass	name;
					LOAD_MICRO_CHUNK_WWSTRING( cload, name );
					WWASSERT( script == nullptr );
					script = Create_Script( name );
					if ( script == nullptr ) {
						Debug_Say(( "Script %s not found \n", name.Peek_Buffer() ));
					}

					// A Missing script is not fatal
//					WWASSERT( script != nullptr );
					break;
				}

				case MICROCHUNKID_PARAM:
				{
					if ( script != nullptr ) {
						StringClass	param;
						LOAD_MICRO_CHUNK_WWSTRING( cload, param );
						script->Set_Parameters_String(param);
					}
					break;
				}

				READ_MICRO_CHUNK_PTR( cload, MICROCHUNKID_GAME_OBJ_OBSERVER_PTR, game_obj_observer_ptr );
				READ_MICRO_CHUNK_PTR( cload, MICROCHUNKID_OWNER_PTR, owner_ptr );

				READ_MICRO_CHUNK( cload, MICROCHUNKID_ID, obs_id );

				default:
					Debug_Say(( "Unrecognized ScriptCollection Header chunkID\n" ));
					break;
			}
			cload.Close_Micro_Chunk();
		}
		cload.Close_Chunk();

		if ( script != nullptr ) {

			if ( obs_id != -1 ) {
				script->Set_ID( obs_id );
//				Debug_Say(( "Loaded Script ID %d\n", obs_id ));
			}

			// If there is data, load
			if ( cload.Open_Chunk() ) {
				WWASSERT( cload.Cur_Chunk_ID() == CHUNKID_SCRIPT_DATA );
				ScriptLoader loader( cload );
				script->Load( loader );
				cload.Close_Chunk();
			}

			WWASSERT( game_obj_observer_ptr != nullptr );
			if ( game_obj_observer_ptr != nullptr ) {
				SaveLoadSystemClass::Register_Pointer(game_obj_observer_ptr, (GameObjObserverClass *)script);
			}

			// set the owner, and request remap
			*(script->Get_Owner_Ptr()) = owner_ptr;
			REQUEST_POINTER_REMAP( (void **)script->Get_Owner_Ptr() );
		} else {
			SaveLoadSystemClass::Register_Pointer(game_obj_observer_ptr, (GameObjObserverClass *)nullptr);
		}


		cload.Close_Chunk();
	}
	return true;
}



