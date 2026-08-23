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
 *                     $Archive:: /Commando/Code/Scripts/scripts.h                      $*
 *                                                                                             *
 *                      $Author:: Byon_g                                                      $*
 *                                                                                             *
 *                     $Modtime:: 11/29/01 11:08a                                             $*
 *                                                                                             *
 *                    $Revision:: 26                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef	SCRIPTS_H
#define	SCRIPTS_H

#include "scriptevents.h"
#include "scriptcommands.h"
#include "ScriptRegistrant.h"
#include "string_ids.h"
#include "wwstring.h"
#include <cstdint>

class CombatSound;


// ScriptVariables
class ScriptVariableClass
{
public:
	ScriptVariableClass( void * data_ptr, int data_size, int id, ScriptVariableClass * next ) :
		DataPtr( data_ptr ),
		DataSize( data_size ),
		ID( id ),
		Next( next )	{}

	void * 						Get_Data_Ptr( void )		{ return DataPtr; }
	int	 						Get_Data_Size( void )	{ return DataSize; }
	int							Get_ID( void )				{ return ID; }
	ScriptVariableClass *	Get_Next( void )			{ return Next; }

private:
	void *						DataPtr;
	int							DataSize;
	int							ID;
	ScriptVariableClass *	Next;
};


class ScriptImpClass : public ScriptClass
{
public:
	ScriptImpClass();
	virtual ~ScriptImpClass();

	// Retrieve the name of the script.
	const char* Get_Name(void) override;

	// Retrieve owner object of the script.
	GameObject* Owner() override
		{return mOwner;}

	GameObject** Get_Owner_Ptr() override
		{return &mOwner;}

	// Set the scripts parameter string.
	void Set_Parameters_String(const char* params) override;

	// Retrieve the scripts parameter string.
	void Get_Parameters_String(char* buffer, unsigned int size) override;

	// Retrieve the parameter count.
	int Get_Parameter_Count(void)
		{return mArgC;}

	// Retrieve a parameter by name.
	const char* Get_Parameter(const char* name);

	// Retrieve a parameter by is ordinal position in the parameter list.
	const char* Get_Parameter(int index);

	// Get a parameter as an integer
	int Get_Int_Parameter(int index)
		{return atoi(Get_Parameter(index));}

	// Get a parameter as an integer
	int Get_Int_Parameter(const char* parameterName);

	// Get a parameter as a float
	float Get_Float_Parameter(int index)
		{return (float)atof(Get_Parameter(index));}

	// Get a parameter as a float
	float Get_Float_Parameter(const char* parameterName);

	// Get a parameter as a bool.  Anything that reads as a non-zero integer is
	// true; a level author writes 1 or 0.
	bool Get_Bool_Parameter(int index)
		{return Get_Int_Parameter(index) != 0;}

	// Get a parameter as a bool
	bool Get_Bool_Parameter(const char* parameterName)
		{return Get_Int_Parameter(parameterName) != 0;}

	// Get a parameter as a vector3
	Vector3 Get_Vector3_Parameter(int index);

	// Get a parameter as a float
	Vector3 Get_Vector3_Parameter(const char* parameterName);

	// Get a parameter as an float
	int Get_Parameter_Index(const char* parameterName);

	// Set the script factory used to create this script.
	//
	// This must only be called by the script factory upon creation of
	// the script.
	void SetFactory(ScriptFactoryClass* factory)
		{mFactory = factory;}

	/* Event Functions which will be called as events happen
	 * Scripts can choose to override any of these functions,
	 * otherwise the following empty functions will be called
	 */
	virtual	void	Created( GameObject * /*obj*/ ) override {}
	virtual	void	Destroyed( GameObject * /*obj*/ ) override {}
	virtual	void	Killed( GameObject * /*obj*/, GameObject * /*killer*/ ) override {}
	virtual	void	Damaged( GameObject * /*obj*/, GameObject * /*damager*/, float /*amount*/ ) override {}
	virtual	void	Custom( GameObject * /*obj*/, int /*type*/, intptr_t /*param*/, GameObject * /*sender*/ ) override {}
	virtual	void	Sound_Heard( GameObject * /*obj*/, const CombatSound & /*sound*/ ) override {}
	virtual	void	Enemy_Seen( GameObject * /*obj*/, GameObject * /*enemy*/ ) override {}
	virtual	void	Action_Complete( GameObject * /*obj*/, int /*action_id*/, ActionCompleteReason /*complete_reason*/ ) override	{}
	virtual	void	Timer_Expired( GameObject * /*obj*/, int /*timer_id*/ ) override {}
	virtual	void	Animation_Complete( GameObject * /*obj*/, const char * /*animation_name*/ ) override {}
	virtual	void	Poked( GameObject * /*obj*/, GameObject * /*poker*/ ) override {}
	virtual	void	Entered( GameObject * /*obj*/, GameObject * /*enterer*/ ) override {}
	virtual	void	Exited( GameObject * /*obj*/, GameObject * /*exiter*/ ) override {}

	// Save and Load specific script
	virtual void Save_Data(ScriptSaver& /*saver*/) {}
	virtual void Load_Data(ScriptLoader& /*loader*/) {}

	// Auto Variable Save and Load
	virtual	void Register_Auto_Save_Variables( void )	{}
	void	Auto_Save_Variable( void * data_ptr, int data_size, int id );

	//
	//	A script's Player_Type parameter, as the 4.8.4 library writes it:
	//	0 Nod, 1 GDI, 2 either side, and 3 meaning whichever player is
	//	nearest the object this script is on.
	//
	bool Is_Player_Type(GameObject* obj, int type);

	//
	//	A script parameter that is itself the parameter list for another
	//	script.  It cannot hold commas, so the level author picks a
	//	delimiter and names it in a second parameter; this puts the commas
	//	back.  An empty delimiter parameter leaves the list alone.
	//
	void Get_Nested_Parameters(const char* params_name, const char* delimiter_name,
			StringClass& params);

protected:
	void Destroy_Script(void);

	void Attach(GameObject* obj) override;
	void Detach(GameObject* obj) override;

	virtual void Save(ScriptSaver& saver) override;
	virtual void Load(ScriptLoader& loader) override;

private:
	void Clear_Parameters(void);
	void Set_Parameter(int index, const char* str);

	GameObject* mOwner;
	int mArgC;
	char** mArgV;

	// The factory reference is provided to the script class so that it
	// knows what it is (IE: Name, Parameter description).
	//
	// The factory reference could also be used for script cloning.
	// (The script would use this factory to create a new instance of
	// itself then copy its state to the new script instance.)
	ScriptFactoryClass* mFactory;

	// Auto Variable Save and Load
	ScriptVariableClass *	AutoVariableList;
};


class PlayerKeyEventClass;
class ObjectCreateEventClass;


/******************************************************************************
*
* CLASS
*     KeyHookScriptClass
*
* DESCRIPTION
*     Base for a script that wants to hear about a key.
*
*     A key hook is server side.  The script names a logical key and a player,
*     and Key_Hook runs when that player presses whatever they have bound to
*     that name.  Which key that is, the script never learns; that the key
*     means anything, the client never learns.  See Code/Combat/scriptkeys.h.
*
*     One subscription to GameEventBus::PlayerKey stands behind every hook in
*     the level rather than one each, so a map with fifty deployable vehicles
*     costs one dispatch per key press rather than fifty.
*
*     Cleanup is the base class's own -- the destructor and Detach both remove
*     the hook -- so a derived script does not have to remember to chain.
*
******************************************************************************/

class KeyHookScriptClass : public ScriptImpClass
{
public:
	KeyHookScriptClass(void);
	virtual ~KeyHookScriptClass(void);

	//	Called on the server when the hooked player presses the hooked key.
	virtual void Key_Hook(void) = 0;

	//
	//	`obj` is who is doing the pressing: the soldier whose player the hook
	//	follows, or a vehicle's occupant.  An object with no player behind it
	//	installs nothing.  A second call replaces the first.
	//
	void Install_Hook(const char* key_name, GameObject* obj);
	void Remove_Hook(void);

	bool Is_Hook_Installed(void) const	{ return HookPlayerID != -1; }

protected:
	void Detach(GameObject* obj) override;

private:
	static void Player_Key_Handler(PlayerKeyEventClass& event, void* data);

	StringClass	HookKeyName;
	int			HookPlayerID;

	KeyHookScriptClass*	NextHook;

	static KeyHookScriptClass*	HookList;
	static int					HookToken;
};


// Declare script definition
#define	DECLARE_SCRIPT(x, d) \
	REGISTER_SCRIPT(x, d) \
	class x : public ScriptImpClass

// Same, for a stock script carrying the 4.8.4 corrections.
#define	DECLARE_SCRIPT_MERGED(x, d) \
	REGISTER_SCRIPT_MERGED(x, d) \
	class x : public ScriptImpClass

// Same, for a script the 4.8.4 library brought with it.
#define	DECLARE_SCRIPT_TT(x, d) \
	REGISTER_SCRIPT_TT(x, d) \
	class x : public ScriptImpClass

// Same, for a 4.8.4 script whose registered name is not its class name.
#define	DECLARE_SCRIPT_TT_NAMED(x, n, d) \
	REGISTER_SCRIPT_TT_NAMED(x, n, d) \
	class x : public ScriptImpClass

// Same, for a merged script that also answers to a second, 4.8.4 name.
#define	DECLARE_SCRIPT_MERGED_ALIAS(x, d, a) \
	REGISTER_SCRIPT_MERGED_ALIAS(x, d, a) \
	class x : public ScriptImpClass

// Load / Save Macros
#define SAVE_BEGIN()
#define SAVE_DATA(id, var) ScriptEngine::Save_Data(saver, id, sizeof(var), &var)
#define SAVE_STRING(id, string) ScriptEngine::Save_Data(saver, id, (int)strlen(string), string)
#define SAVE_END()


#define LOAD_BEGIN() \
{ \
	int id; \
	while (ScriptEngine::Load_Begin(loader, &id)) { \
		switch (id) {

#define LOAD_DATA(id, var) \
			case id: \
				ScriptEngine::Load_Data(loader, sizeof(var), &var); \
			break;

#define LOAD_END() \
			default: \
				break; \
		} \
		ScriptEngine::Load_End(loader); \
	} \
}

#define LOAD_STRING(id, var) LOAD_DATA(id, var)

// Auto Variable Save/Load
#define REGISTER_VARIABLES()			public: void Register_Auto_Save_Variables( void ) override
#define SAVE_VARIABLE( x, id )		Auto_Save_Variable( &x, sizeof( x ), id )

// Array Macros
#define		ARRAY_ELEMENT_COUNT( x )	( sizeof( x ) / sizeof( x[0] ) )
#define		RANDOM_ARRAY_ELEMENT( x )	( x[ScriptEngine::Get_Random_Int( 0, ARRAY_ELEMENT_COUNT( x ) )] )

/******************************************************************************
*
* CLASS
*     ObjectCreateHookScriptClass
*
* DESCRIPTION
*     Base for a script that wants to hear about everything that is created.
*
*     A level uses one of these to say "whatever else turns up in this map,
*     put this script on it if it is one of these" -- a rule about the map
*     rather than a rule about an object.  4.8.4 reached it by patching the
*     engine's object list; here it is GameEventBus::ObjectCreate.
*
*     Shaped like KeyHookScriptClass and for the same reason: one subscription
*     stands behind every hook, the list is collected before any hook runs, and
*     cleanup is the base class's own.
*
******************************************************************************/

class ObjectCreateHookScriptClass : public ScriptImpClass
{
public:
	ObjectCreateHookScriptClass(void);
	virtual ~ObjectCreateHookScriptClass(void);

	//	Called for every object created while the hook is installed.
	virtual void Object_Created(GameObject* obj) = 0;

	void Install_Create_Hook(void);
	void Remove_Create_Hook(void);

protected:
	void Created(GameObject* obj) override;
	void Detach(GameObject* obj) override;

private:
	static void Object_Create_Handler(ObjectCreateEventClass& event, void* data);

	bool	CreateHookInstalled;

	ObjectCreateHookScriptClass*	NextCreateHook;

	static ObjectCreateHookScriptClass*	CreateHookList;
	static int							CreateHookToken;
};


#endif // SCRIPTS_H
