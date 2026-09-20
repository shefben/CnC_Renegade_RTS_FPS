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
 *                 Project Name : WWSaveLoad                                                   *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwsaveload/definitionmgr.h                   $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 8/24/01 5:13p                                               $*
 *                                                                                             *
 *                    $Revision:: 24                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once
#endif


#ifndef __DEFINITION_MGR_H
#define __DEFINITION_MGR_H

#include "always.h"
#include "saveload.h"
#include "saveloadsubsystem.h"
#include "saveloadids.h"
#include "wwdebug.h"
#include "wwstring.h"
#include "hashtemplate.h"
#include "vector.h"


// Forward declarations
class DefinitionClass;

//////////////////////////////////////////////////////////////////////////////////
//	Global declarations
//////////////////////////////////////////////////////////////////////////////////
extern class DefinitionMgrClass	_TheDefinitionMgr;

//////////////////////////////////////////////////////////////////////////////////
//
//	DefinitionMgrClass
//
//////////////////////////////////////////////////////////////////////////////////
class DefinitionMgrClass : public SaveLoadSubSystemClass
{
public:

	/////////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	/////////////////////////////////////////////////////////////////////
	DefinitionMgrClass (void);
	~DefinitionMgrClass (void);

	/////////////////////////////////////////////////////////////////////
	//	Public methods
	/////////////////////////////////////////////////////////////////////

	// From SaveLoadSubSystemClass
	virtual uint32					Chunk_ID (void) const override;

	// Type identification
	static DefinitionClass *	Find_Definition (uint32 id, bool twiddle = true);
	static DefinitionClass *	Find_Named_Definition (const char *name, bool twiddle = true);
	static DefinitionClass *	Find_Typed_Definition (const char *name, uint32 class_id, bool twiddle = true);
   static void                List_Available_Definitions (void);
   static void                List_Available_Definitions (int superclass_id);
	static uint32					Get_New_ID (uint32 class_id);

	// Definition registration
	static void						Register_Definition (DefinitionClass *definition);
	static void						Unregister_Definition (DefinitionClass *definition);

	//
	// Definition enumeration
	//
	typedef enum
	{
		ID_CLASS			= 1,
		ID_SUPERCLASS,
	} ID_TYPE;

	static DefinitionClass *	Get_First (void);
	static DefinitionClass *	Get_First (uint32 id, ID_TYPE type = ID_CLASS);
	static DefinitionClass *	Get_Next (DefinitionClass *curr_def);
	static DefinitionClass *	Get_Next (DefinitionClass *curr_def, uint32 id, ID_TYPE type = ID_CLASS);

	static void						Free_Definitions (void);

	/*
	**	Opt-in, off by default (zero cost when unset beyond one bool check per definition).  When
	**	set, Load_Objects WWDEBUG_SAY's each definition's chunk id, index and (once constructed)
	**	name and class id as it loads.  Objects.DDB is the whole game's definition set loaded as
	**	one undifferentiated stream; a crash partway through it is otherwise invisible as to which
	**	definition it was on.  A caller wanting the trace also needs a message handler installed
	**	(WWDebug_Install_Message_Handler) -- this flag only controls whether Load_Objects emits it.
	*/
	static bool						Verbose_Load_Diagnostics;

	/*
	**	Opt-in, off by default (Load_Chunk_Range_End == 0 means unrestricted -- identical to
	**	today's behavior).  When End is non-zero, Load_Objects skips any definition sub-chunk
	**	whose id falls outside [Begin, End) without even looking up its persist factory, let
	**	alone constructing or loading it.  Objects.DDB is the whole game's definition set, and a
	**	caller that only wants one library's definitions (say, Combat's, at CHUNKID_COMBAT_BEGIN)
	**	should not have to bring up every other library those other chunks assume is already
	**	running (WWAudio, WW3D, ...) just to skip past them.
	*/
	static uint32					Load_Chunk_Range_Begin;
	static uint32					Load_Chunk_Range_End;

protected:

	/////////////////////////////////////////////////////////////////////
	//	Protected methods
	/////////////////////////////////////////////////////////////////////

	// From SaveLoadSubSystemClass
	virtual bool					Contains_Data (void) const override;
	virtual bool					Save (ChunkSaveClass &csave) override;
	virtual bool					Load (ChunkLoadClass &cload) override;
	virtual const char*			Name (void) const override						{ return "DefinitionMgrClass"; }

	// Persistence methods
	bool								Save_Objects (ChunkSaveClass &csave);
	bool								Load_Objects (ChunkLoadClass &cload);
	bool								Save_Variables (ChunkSaveClass &csave);
	bool								Load_Variables (ChunkLoadClass &cload);

private:
	static HashTemplateClass<StringClass, DynamicVectorClass<DefinitionClass*>*>* DefinitionHash;

	/////////////////////////////////////////////////////////////////////
	//	Private methods
	/////////////////////////////////////////////////////////////////////
	static void						Prepare_Definition_Array (void);
	static int fnCompareDefinitionsCallback (const void *elem1, const void *elem2);

	/////////////////////////////////////////////////////////////////////
	//	Static member data
	/////////////////////////////////////////////////////////////////////
	static DefinitionClass **	_SortedDefinitionArray;
	static int						_MaxDefinitionCount;
	static int						_DefinitionCount;

	/////////////////////////////////////////////////////////////////////
	//	Friend classes
	/////////////////////////////////////////////////////////////////////
	friend class DefinitionClass;
};

/////////////////////////////////////////////////////////////////////
//	Chunk_ID
/////////////////////////////////////////////////////////////////////
inline uint32
DefinitionMgrClass::Chunk_ID (void) const
{
	return CHUNKID_SAVELOAD_DEFMGR;
}

/////////////////////////////////////////////////////////////////////
//	Contains_Data
/////////////////////////////////////////////////////////////////////
inline bool
DefinitionMgrClass::Contains_Data (void) const
{
	return true;  // TODO: check if we have any definitions...
}

/////////////////////////////////////////////////////////////////////
//	Get_First_Definition
/////////////////////////////////////////////////////////////////////
inline DefinitionClass *
DefinitionMgrClass::Get_First (void)
{
	DefinitionClass *definition = nullptr;
	if (_DefinitionCount > 0) {
		definition = _SortedDefinitionArray[0];
	}

	return definition;
}


#endif //__DEFINITION_MGR_H
