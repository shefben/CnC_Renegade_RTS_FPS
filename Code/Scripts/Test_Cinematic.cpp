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

/******************************************************************************
*
* FILE
*
* DESCRIPTION
*
* PROGRAMMER
*     Byon Garrabrant
*
* VERSION INFO
*     $Author: Greg_h $
*     $Revision: 51 $
*     $Modtime: 12/13/01 10:37a $
*     $Archive: /Commando/Code/Scripts/Test_Cinematic.cpp $
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"
#include <stdio.h>
#include <string.h>
#include "wwlib/wwstring.h"
#include <limits>
#include <stdlib.h>

#define		LAST_VALID_TIMESTAMP		999000.0f

//
//	4.8.4 shipped a byte-for-byte copy of this under its own name.  One
//	implementation, two names.
//
DECLARE_SCRIPT_MERGED_ALIAS (M00_Cinematic_Attack_Command_DLS, "AttackDuration=1.0:float", "JFW_Cinematic_Attack_Command")
{

	void Created( GameObject * obj ) override
	{
		float facing = ScriptEngine::Get_Facing(Owner());
		Vector3 target = ScriptEngine::Get_Position(Owner());
		target.X += WWMath::Cos(DEG_TO_RADF(facing)) * 10.0f;
		target.Y += WWMath::Sin(DEG_TO_RADF(facing)) * 10.0f;
		target.Z += 2.0f;

		ActionParamsStruct params;
		params.Set_Basic( this, 99, 1 );
		params.Set_Attack( target, 100.0f, 0.0f, true );
		params.AttackForceFire = true;

		ScriptEngine::Action_Attack( obj, params );
		ScriptEngine::Send_Custom_Event( obj, obj, 1, 1, Get_Float_Parameter("AttackDuration") );
	}

	void Custom( GameObject * obj, int /*type*/, intptr_t /*param*/, GameObject * /*sender*/ ) override
	{
		ScriptEngine::Action_Reset( obj, 100 );

	}

};

DECLARE_SCRIPT_MERGED_ALIAS(Test_Cinematic_Primary_Killed, "CallbackID=:int", "JFW_Cinematic_Primary_Killed")
{

	bool custom_sent;

	// Register variables to be Auto-Saved
	// All variables must have a unique ID, less than 256, that never changes
	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE( custom_sent, 1 );
	}

	virtual	void Created (GameObject * /* game_obj */) override
	{
		custom_sent = false;
	}

	virtual	void	Killed( GameObject * obj, GameObject * /* killer */ ) override
	{
		if (!custom_sent)
		{
			custom_sent = true;
			ScriptEngine::Debug_Message( "Cinematic Primary Killed\n" );
			int id = Get_Int_Parameter( "CallbackID" );

			GameObject * callback = ScriptEngine::Find_Object( id );
			if ( callback ) {
				ScriptEngine::Send_Custom_Event( obj, callback, M00_CUSTOM_CINEMATIC_PRIMARY_KILLED, 0, 0.0f );
			}
		}
	}

	virtual	void	Destroyed( GameObject * obj ) override
	{
		if (!custom_sent)
		{
			custom_sent = true;
			ScriptEngine::Debug_Message( "Cinematic Primary Destroyed\n" );
			int id = Get_Int_Parameter( "CallbackID" );

			GameObject * callback = ScriptEngine::Find_Object( id );
			if ( callback ) {
				ScriptEngine::Send_Custom_Event( obj, callback, M00_CUSTOM_CINEMATIC_PRIMARY_KILLED, 0, 0.0f );
			}
		}
	}

};

DECLARE_SCRIPT_MERGED_ALIAS(Test_Cinematic, "ControlFilename=:string,StartPaused=0:int", "JFW_Cinematic")
{
public:
	/*
	** Object Slots
	*/
	#define	NUM_SLOTS	40
	int	ObjectSlots[ NUM_SLOTS ];

	int	MyID;  // doesn't need to be saved

	/*
	** Timing
	*/
	unsigned int	LastSyncTime;
	float				Time;
	float				FrameSync;
	bool				PrimaryKilled;

	bool				IsCameraCinematic;

	//
	//	4.8.4's second parameter: a cinematic that does not start itself, so
	//	a level can fill its object slots before the first command runs.
	//	Custom M00_CUSTOM_CINEMATIC_START releases it.
	//
	bool				StartPaused;


	/*
	** Parameters
	*/
	char * NextParameter;

	/*
	** Control Lines
	*/
	struct ControlLine {
		float				Time;
		char *			Command;
		ControlLine *	Next;
	};

	ControlLine * Controls;

	void	Add_Control_Line( float time, const char * command )
	{
		ControlLine * control = new ControlLine;
		control->Time = time;
		control->Command = strdup( command );
		control->Next = nullptr;

		if ( Controls == nullptr || Controls->Time > time ) {
			control->Next = Controls;
			Controls = control;
		} else {
			ControlLine * node = Controls;
			while ( (node->Next != nullptr) && (node->Next->Time <= time) ) {
				node = node->Next;
			}
			control->Next = node->Next;
			node->Next = control;
		}
	}

	void	Remove_Head_Control_Line( void )
	{
		if ( Controls != nullptr ) {
			ControlLine * control = Controls;
			Controls = Controls->Next;

			free( control->Command );
			delete control;
		}
	}

	void	Display_Control_Lines( void )
	{
		for ( ControlLine * node = Controls; node != nullptr; node = node->Next ) {
//			ScriptEngine::Debug_Message( "%3.2f \"%s\"\n", node->Time, node->Command );
		}
	}

	/*
	** Loading the control file, line-by-line
	*/
	void	Load_Control_File( const char * filename )
	{
		ScriptEngine::Debug_Message( "Loading Control File %s\n", filename );

		char full_filename[80];
		snprintf( full_filename, sizeof(full_filename), "DATA/%s", filename );
//		FILE * in = fopen( full_filename, "rt" );
		void *handle = ScriptEngine::Text_File_Open( filename );
		if ( handle == nullptr ) {
			ScriptEngine::Debug_Message( "Failed to open %s\n", full_filename );
			return;
		}

		char *line,line_data[200];
		while ( ScriptEngine::Text_File_Get_String( handle, line_data, sizeof( line_data )-1 ) ) {
			line = line_data;
			// Convert tabs to spaces
			for ( char * l = line; *l; l++ ) {
				if ( *l == 9 ) *l = ' ';
			}

			// Remove leading and trailing white space
			while ( *line && *line <= ' ' )	line++;
			size_t length = ::strlen( line );
			while ( (length != 0) && (line[length-1] <= ' ') )	{
				line[--length] = 0;
			}

			if ( *line && *line != ';' ) {	// ignore comments
				float	time;
				time = strtof( line , nullptr);

				if ( time < 0 ) {	// handle frame numbers
					time = -time / 30.f;
				}

				while ( *line && *line > ' ' ) line++;
				while ( *line && *line <= ' ' ) line++;
				if ( *line ) {
					Add_Control_Line( time, line );
				}
			}
		}

//		fclose( in );
		ScriptEngine::Text_File_Close( handle );
	}

	/*
	**
	*/
	const char* Get_Command_Parameter( char * string = nullptr )
	{
		if ( string != nullptr ) {
			NextParameter = string;
		}

		if ( NextParameter == nullptr || *NextParameter == 0 ) return "";

		// Start at this parameter;
		char * parameter = NextParameter;

		// Skip leading whitespace
		while ( *parameter != 0 && *parameter <= ' ' ) parameter++;

		// if starting with quote, find end quote
		if ( *parameter == '\"' ) {
			parameter++;
			NextParameter = parameter;
			while ( *NextParameter && *NextParameter != '\"' ) {
				NextParameter++;
			}
		}

		// Find next parameter end
		while ( *NextParameter && *NextParameter != ',' ) {
			NextParameter++;
		}

		// Terminate
		if ( *NextParameter ) {
			*NextParameter++ = 0;
		}

		// Remove trailing whitespace
		size_t length = ::strlen( parameter );
		while ( length && parameter[ length-1 ] <= ' ' ) {
			parameter[ --length ] = 0;
		}

		// Remove end quote
		if ( length && parameter[ length-1 ] == '\"' ) {
			parameter[ --length ] = 0;
		}

		return parameter;
	}


#define	CHUNKID_VARIABLES								1000
#define	CHUNKID_OBJECT_SLOTS							1001
#define	CHUNKID_CONTROL_LINES						1002

#define	CHUNKID_VARIABLE_SYNC_DIFF					1
#define	CHUNKID_VARIABLE_TIME						2

#define	CHUNKID_CONTROL_TIME							4
#define	CHUNKID_CONTROL_COMMAND_SIZE				5
#define	CHUNKID_CONTROL_COMMAND						6
#define	CHUNKID_PRIMARY_KILLED						7
#define	CHUNKID_START_PAUSED						8

	/*
	**
	*/
	void Save(ScriptSaver& saver) override
	{
//		ScriptEngine::Debug_Message( "Cinematic Saving\n" );
//		ScriptEngine::Debug_Message("Sync time is %d.\n", ScriptEngine::Get_Sync_Time());

		// Save the Variables
		ScriptEngine::Begin_Chunk(saver, CHUNKID_VARIABLES);
			int sync_diff = ScriptEngine::Get_Sync_Time() - LastSyncTime;
			ScriptEngine::Save_Data(saver, CHUNKID_VARIABLE_SYNC_DIFF, sizeof( sync_diff ), &sync_diff );
//ScriptEngine::Debug_Message( "Saving sync_diff %d\n", sync_diff );
			ScriptEngine::Save_Data(saver, CHUNKID_VARIABLE_TIME, sizeof( Time ), &Time );
//ScriptEngine::Debug_Message( "Saving Time %f\n", Time );
			ScriptEngine::Save_Data(saver, CHUNKID_PRIMARY_KILLED, sizeof( PrimaryKilled ), &PrimaryKilled );
			ScriptEngine::Save_Data(saver, CHUNKID_START_PAUSED, sizeof( StartPaused ), &StartPaused );
		ScriptEngine::End_Chunk(saver);

		// Save the Object Slots
		ScriptEngine::Begin_Chunk(saver, CHUNKID_OBJECT_SLOTS);
		for ( int i = 0; i < NUM_SLOTS; i++ ) {
			if ( ObjectSlots[i] != 0 ) {
				ScriptEngine::Save_Data(saver, i, sizeof( ObjectSlots[i] ), &ObjectSlots[ i ] );
//ScriptEngine::Debug_Message( "Saving Slot %d as %d\n", i, ObjectSlots[ i ] );
			}
		}
		ScriptEngine::End_Chunk(saver);

		// Save Control Lines
		ScriptEngine::Begin_Chunk(saver, CHUNKID_CONTROL_LINES);
		ControlLine * controls = Controls;
		while ( controls != nullptr ) {
			ScriptEngine::Save_Data(saver, CHUNKID_CONTROL_TIME, sizeof( controls->Time ), &controls->Time );
			const size_t command_len = ::strlen( controls->Command ) + 1;
			assert(command_len <= static_cast<size_t>(std::numeric_limits<int>::max()));
			int len = static_cast<int>(command_len);
			ScriptEngine::Save_Data(saver, CHUNKID_CONTROL_COMMAND_SIZE, sizeof( len ), &len );
			ScriptEngine::Save_Data(saver, CHUNKID_CONTROL_COMMAND, len, controls->Command );
//ScriptEngine::Debug_Message( "Saving Command %f %s\n", controls->Time, controls->Command );
			controls = controls->Next;
		}
		ScriptEngine::End_Chunk(saver);

	}

	void Load(ScriptLoader& loader) override
	{
		Controls = nullptr;
		NextParameter = nullptr;

		for ( int i = 0; i < NUM_SLOTS; i++ ) {
			ObjectSlots[i] =0;
		}

//		ScriptEngine::Debug_Message( "Cinematic Loading\n" );
//		ScriptEngine::Debug_Message("Sync time is %d.\n", ScriptEngine::Get_Sync_Time());

		unsigned int chunkID;

		while (ScriptEngine::Open_Chunk(loader, &chunkID)) {

			switch (chunkID) {

				case CHUNKID_VARIABLES:
				{
					int id;
					int sync_diff;
					while (ScriptEngine::Load_Begin(loader, &id)) {
						switch ( id ) {
							case CHUNKID_VARIABLE_SYNC_DIFF:
								ScriptEngine::Load_Data(loader, sizeof( sync_diff ), &sync_diff );
//ScriptEngine::Debug_Message( "Loading sync_diff %d\n", sync_diff );
								LastSyncTime = ScriptEngine::Get_Sync_Time() - sync_diff;
		  						break;

							case CHUNKID_VARIABLE_TIME:
								ScriptEngine::Load_Data(loader, sizeof( Time ), &Time );
//ScriptEngine::Debug_Message( "Loading Time %f\n", Time );
								break;

							case CHUNKID_PRIMARY_KILLED:
								ScriptEngine::Load_Data(loader, sizeof( PrimaryKilled ), &PrimaryKilled );
//ScriptEngine::Debug_Message( "Loading Time %f\n", Time );
								break;

							case CHUNKID_START_PAUSED:
								ScriptEngine::Load_Data(loader, sizeof( StartPaused ), &StartPaused );
								break;

						}
						ScriptEngine::Load_End(loader);
					}
				}
				break;

				case CHUNKID_OBJECT_SLOTS:
				{
					int id;
					while (ScriptEngine::Load_Begin(loader, &id)) {
						if ( id >= 0 && id < NUM_SLOTS ) {
							ScriptEngine::Load_Data(loader, sizeof( ObjectSlots[id] ), &ObjectSlots[id] );
//ScriptEngine::Debug_Message( "Loading Slot %d as %d\n", id, ObjectSlots[ id ] );
						}
						ScriptEngine::Load_End(loader);
					}
				}
				break;


				case CHUNKID_CONTROL_LINES:
				{
					int id;
					float time = 0;
					int len = 0;

					while (ScriptEngine::Load_Begin(loader, &id)) {
						if ( id == CHUNKID_CONTROL_TIME ) {
							ScriptEngine::Load_Data(loader, sizeof( time ), &time );
						}
						if ( id == CHUNKID_CONTROL_COMMAND_SIZE ) {
							ScriptEngine::Load_Data(loader, sizeof( len ), &len );
						}
						if ( id == CHUNKID_CONTROL_COMMAND ) {
							#define	MAX_COMMAND_LOAD_SIZE 200
							char load_command[MAX_COMMAND_LOAD_SIZE+1];
							if ( len < MAX_COMMAND_LOAD_SIZE ) {
								ScriptEngine::Load_Data(loader, len, &load_command[0] );
								Add_Control_Line( time, load_command );
//ScriptEngine::Debug_Message( "Loading Command %f %s\n", time, load_command );
							}
						}
						ScriptEngine::Load_End(loader);
					}
				}
				break;

				default:
					DebugPrint("***** Cinematic Script::Load() - Unrecognized chunk %0x!\n", chunkID);
				break;
			}

			ScriptEngine::Close_Chunk(loader);
		}
	}

	const char * Get_First_Parameter( char * str ) { return Get_Command_Parameter( str ); }
	const char * Get_Next_Parameter( void ) { return Get_Command_Parameter(); }

	/*
	**
	*/
	void	Command_Create_Object( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Creating Object %s\n", (int)params );
		int slot = atoi( Get_First_Parameter( params ) );
		const char * model_name = Get_Next_Parameter();

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Slot Number %d\n", slot );
			slot = -1;
		}

		if ( slot != -1 && ObjectSlots[ slot ] != 0 ) {
//			ScriptEngine::Debug_Message( "Used Slot number %d\n", slot );
//			ScriptEngine::Debug_Message( "Slot used by %d\n", ObjectSlots[ slot ] );
		}

		// Create a decoration cinematic object, then set it's model
		GameObject * obj = ScriptEngine::Create_Object( "Generic_Cinematic", ScriptEngine::Get_Position( Owner() ) );
		ScriptEngine::Enable_Hibernation( obj, false );

		if ( obj ) {
			ScriptEngine::Add_To_Dirty_Cull_List(obj);
			ScriptEngine::Set_Model( obj, model_name );
			ScriptEngine::Set_Facing( obj, ScriptEngine::Get_Facing( Owner() ) );
			if ( slot != -1 ) {
				ObjectSlots[ slot ] = ScriptEngine::Get_ID( obj );
			}

			if ( IsCameraCinematic ) {
				ScriptEngine::Enable_Cinematic_Freeze( obj, false );
				ScriptEngine::Enable_Hibernation(obj, false);
			}

		} else {
//			ScriptEngine::Debug_Message( "Failed to create %s\n", (int)model_name );
		}
	}

	/*
	**
	*/
	void	Command_Create_Real_Object( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Creating Real Object %s\n", (int)params );
		int slot = atoi( Get_First_Parameter( params ) );
		const char * preset_name = Get_Next_Parameter();
		const char * host_slot_name = Get_Next_Parameter();
		const char * host_bone_name = Get_Next_Parameter();

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Slot Number %d\n", slot );
			slot = -1;
		}

		if ( slot != -1 && ObjectSlots[ slot ] != 0 ) {
//			ScriptEngine::Debug_Message( "Used Slot number %d\n", slot );
//			ScriptEngine::Debug_Message( "Slot used by %d\n", ObjectSlots[ slot ] );
		}

		GameObject * obj = nullptr;
		if (( host_slot_name != nullptr ) && ( *host_slot_name != 0 ) ) {
			int host_slot = atoi( host_slot_name );
			GameObject * host_obj = ScriptEngine::Find_Object( ObjectSlots[ host_slot ] );
			obj = ScriptEngine::Create_Object_At_Bone( host_obj, preset_name, host_bone_name );
		} else {
			obj = ScriptEngine::Create_Object( preset_name, ScriptEngine::Get_Position( Owner() ) );
			ScriptEngine::Set_Facing( obj, ScriptEngine::Get_Facing( Owner() ) );
		}

		if ( obj ) {
			ScriptEngine::Enable_Engine( obj, true );
			ScriptEngine::Add_To_Dirty_Cull_List(obj);
//			ScriptEngine::Enable_Hibernation( obj, false );
			if ( slot != -1 ) {
				ObjectSlots[ slot ] = ScriptEngine::Get_ID( obj );
			}

			if ( IsCameraCinematic ) {
				ScriptEngine::Enable_Cinematic_Freeze( obj, false );
				ScriptEngine::Enable_Hibernation(obj, false);
			}

		} else {
//			ScriptEngine::Debug_Message( "Failed to create real %s\n", (int)preset_name );
		}
	}

	/*
	**
	*/
	void	Command_Create_Explosion( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Creating Explosion %s\n", (int)params );
		const char * preset_name = Get_First_Parameter( params );
		int host_slot = atoi( Get_Next_Parameter() );
		const char * host_bone_name = Get_Next_Parameter();

		GameObject * host_obj = ScriptEngine::Find_Object( ObjectSlots[ host_slot ] );
		ScriptEngine::Create_Explosion_At_Bone( preset_name, host_obj, host_bone_name, nullptr );
	}

	void	Command_Destroy_Object( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Destroying Object %s\n", (int)params );
		int slot = atoi( Get_First_Parameter( params ) );

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Slot Number %d\n", slot );
			slot = -1;
		}

		if ( slot != -1 ) {
			if ( ObjectSlots[ slot ] == 0 ) {
//				ScriptEngine::Debug_Message( "Empty Slot number %d\n", slot );
			} else {
				int id = ObjectSlots[ slot ];
				GameObject * obj = ScriptEngine::Find_Object( id );
				if ( obj ) {
					ScriptEngine::Destroy_Object( obj );
				}
			}
		}
	}

	//
	//	Take an object that is already in the level into a slot, found by preset
	//	and nearest to the cinematic.  Create_Object makes a new one; this drives
	//	scenery the designer placed by hand.
	//
	void	Command_Add_Object( char * params )
	{
		int slot = atoi( Get_First_Parameter( params ) );
		const char * preset_name = Get_Next_Parameter();

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
			return;
		}

		GameObject * obj = ScriptEngine::Find_Nearest_Preset(
				ScriptEngine::Get_Position( Owner() ), preset_name );
		if ( obj != nullptr ) {
			ObjectSlots[ slot ] = ScriptEngine::Get_ID( obj );
		}
	}

	//
	//	Play an animation through once, from its first frame.  Play_Animation
	//	starts partway in to stay in step with the cinematic's clock, which is
	//	wrong when what is wanted is the whole thing.
	//
	void	Command_Play_Complete_Animation( char * params )
	{
		int slot = atoi( Get_First_Parameter( params ) );
		const char * name = Get_Next_Parameter();

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
			return;
		}

		GameObject * obj = ScriptEngine::Find_Object( ObjectSlots[ slot ] );
		if ( obj != nullptr ) {
			ScriptEngine::Set_Animation( obj, name, false, nullptr, 0.0F, -1.0F, false );

			if ( IsCameraCinematic ) {
				ScriptEngine::Innate_Disable( obj );
			}
		}
	}

	//	Stop whatever the slot is playing and leave it where it stands.
	void	Command_Stop_Animation( char * params )
	{
		int slot = atoi( Get_First_Parameter( params ) );

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
			return;
		}

		GameObject * obj = ScriptEngine::Find_Object( ObjectSlots[ slot ] );
		if ( obj != nullptr ) {
			ScriptEngine::Set_Animation( obj, "", false );

			if ( IsCameraCinematic ) {
				ScriptEngine::Innate_Disable( obj );
			}
		}
	}

	//	Put a line of text in front of the players watching.  Team 2 -- the
	//	default when the parameter is left off -- means everyone.  A colour
	//	that is not fully given falls back to white rather than to black.
	void	Command_Show_Message( char * params )
	{
		const char * message	= Get_First_Parameter( params );
		const char * team_str	= Get_Next_Parameter();
		const char * red_str		= Get_Next_Parameter();
		const char * green_str	= Get_Next_Parameter();
		const char * blue_str	= Get_Next_Parameter();

		if ( (message == nullptr) || (message[0] == 0) ) {
			return;
		}

		int team = ( (team_str != nullptr) && (team_str[0] != 0) ) ? atoi( team_str ) : 2;

		int red		= 255;
		int green	= 255;
		int blue		= 255;

		if ( (red_str != nullptr) && (red_str[0] != 0) &&
			  (green_str != nullptr) && (green_str[0] != 0) &&
			  (blue_str != nullptr) && (blue_str[0] != 0) ) {
			red	= atoi( red_str );
			green	= atoi( green_str );
			blue	= atoi( blue_str );
		}

		if ( (team != SCRIPT_PLAYERTYPE_NOD) && (team != SCRIPT_PLAYERTYPE_GDI) ) {
			ScriptEngine::Send_Message( red, green, blue, message );
		} else {
			ScriptEngine::Send_Message_Team( team, red, green, blue, message );
		}
	}

	void	Command_Play_Animation( char * params )
	{
//		ScriptEngine::Debug_Message( "Playing Animation %s\n", (int)params );
		int slot = atoi( Get_First_Parameter( params ) );
		const char * name = Get_Next_Parameter();
		bool looping = atoi( Get_Next_Parameter() ) != 0;
		const char * sub_obj_name = Get_Next_Parameter();
		bool is_blended = atoi( Get_Next_Parameter() ) == 1;

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Slot Number %d\n", slot );
			slot = -1;
		}

		if ( slot != -1 ) {
			int id = ObjectSlots[ slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
				ScriptEngine::Update_Network_Object( obj );
				ScriptEngine::Set_Subobject_Animation( obj, name, looping, sub_obj_name, FrameSync, -1.0F, is_blended );

				// Anyone that plays an animation in a camera cine, doesn't get innate
				if ( IsCameraCinematic ) {
					ScriptEngine::Innate_Disable( obj );
				}


			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", slot );
			}
		}

	}

	void	Command_Play_Audio( char * params )
	{
//		ScriptEngine::Debug_Message( "Playing Audio %s\n", (int)params );
		const char * preset_name = Get_First_Parameter( params );
		const char * slot_name = Get_Next_Parameter();
		const char * bone_name = Get_Next_Parameter();

		int slot = -1;
		if (( slot_name != nullptr ) && ( *slot_name != 0 ) ) {
			slot = atoi( slot_name );
		}

		if ( (slot < 0) || ( slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Slot Number %d\n", slot );
			slot = -1;
		}

		if ( slot != -1 ) {
			int id = ObjectSlots[ slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
				ScriptEngine::Create_3D_Sound_At_Bone( preset_name, obj, bone_name );
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", slot );
			}
		} else {
			ScriptEngine::Create_2D_Sound( preset_name );
		}
	}

	void	Command_Control_Camera( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Controlling the camera %s\n", (int)params );
		int slot = atoi( Get_First_Parameter( params ) );

		if ( (slot < -1) || ( slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Slot Number %d\n", slot );
			slot = -1;
		}

		if ( slot != -1 ) {
			int id = ObjectSlots[ slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
				ScriptEngine::Update_Network_Object( obj );
				ScriptEngine::Set_Camera_Host_Network( obj );
				ScriptEngine::Control_Enable( ScriptEngine::Get_The_Star(), false );
				ScriptEngine::Enable_HUD(0);
				IsCameraCinematic = true;
				ScriptEngine::Enable_Cinematic_Freeze( obj, false );
				ScriptEngine::Enable_Hibernation(obj, false);
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", slot );
			}
		} else {
			ScriptEngine::Set_Camera_Host_Network( nullptr );
			ScriptEngine::Control_Enable( ScriptEngine::Get_The_Star(), true );
			ScriptEngine::Enable_HUD(1);
		}
	}

	void	Command_Send_Custom( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Send Custom %s\n", (int)params );
		const char * to_id_name = Get_First_Parameter( params );
		int type = atoi( Get_Next_Parameter() );
		const char * parameter_name = Get_Next_Parameter();

		int to_id = -1;
		if ( ::strchr( to_id_name, '#' ) != nullptr ) {
			int to_slot = atoi( ::strchr( to_id_name, '#' ) + 1 );
			if ( (to_slot < 0) || ( to_slot >= NUM_SLOTS ) ) {
//				ScriptEngine::Debug_Message( "Bad Slot Number %d\n", to_slot );
			} else {
				to_id = ObjectSlots[ to_slot ];
			}
		} else {
			to_id = atoi( to_id_name );
		}

		int parameter = 0;
		if ( ::strchr( parameter_name, '#' ) != nullptr ) {
			int parameter_slot = atoi( ::strchr( parameter_name, '#' ) + 1 );
			if ( (parameter_slot < 0) || ( parameter_slot >= NUM_SLOTS ) ) {
//				ScriptEngine::Debug_Message( "Bad Slot Number %d\n", parameter_slot );
			} else {
				parameter = ObjectSlots[ parameter_slot ];
			}
		} else {
			parameter = atoi( parameter_name );
		}


		GameObject * to = ScriptEngine::Find_Object( to_id );
		if ( to ) {
			ScriptEngine::Send_Custom_Event( Owner(), to, type, parameter, 0.0f );
		} else {
//			ScriptEngine::Debug_Message( "Send Custom Target not found %d\n", to_id );
		}
	}

	void	Command_Attach_To_Bone( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Attach_To_Bone %s\n", (int)params );
		int obj_slot = atoi( Get_First_Parameter( params ) );
		int host_slot = atoi( Get_Next_Parameter() );
		const char * bone_name = Get_Next_Parameter();

		if ( (obj_slot < 0) || ( obj_slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Obj Slot Number %d\n", obj_slot );
			obj_slot = -1;
		}

		if ( (host_slot < 0) || ( host_slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Host Slot Number %d\n", host_slot );
			host_slot = -1;
		}

		if ( obj_slot != -1 ) {
			int id = ObjectSlots[ obj_slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {

				if ( host_slot != -1 ) {
					id = ObjectSlots[ host_slot ];
					GameObject * host = ScriptEngine::Find_Object( id );
					if	( host ) {
						ScriptEngine::Attach_To_Object_Bone( obj, host, bone_name );
					} else {
//						ScriptEngine::Debug_Message( "Host Object not found %d\n", host_slot );
					}
				} else {
					ScriptEngine::Attach_To_Object_Bone( obj, nullptr, nullptr );
				}
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", obj_slot );
			}
		}
	}

	void	Command_Attach_Script( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Attach_Script %s\n", (int)params );
		int obj_slot = atoi( Get_First_Parameter( params ) );
		const char * script_name = Get_Next_Parameter();
		const char * script_parameters = Get_Next_Parameter();

		if ( (obj_slot < 0) || ( obj_slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Obj Slot Number %d\n", obj_slot );
			obj_slot = -1;
		}

		if ( obj_slot != -1 ) {
			int id = ObjectSlots[ obj_slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
//				ScriptEngine::Debug_Message( "Attaching Script %s \"%s\"\n", script_name, script_parameters );
				ScriptEngine::Attach_Script( obj, script_name, script_parameters );
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", obj_slot );
			}
		}
	}

	void	Command_Set_Primary( char * params )
	{
//		ScriptEngine::Debug_Message_2( "Set_Primary %s\n", (int)params );
		int obj_slot = atoi( Get_First_Parameter( params ) );

		if ( (obj_slot < 0) || ( obj_slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Obj Slot Number %d\n", obj_slot );
			obj_slot = -1;
		}

		if ( obj_slot != -1 ) {
			int id = ObjectSlots[ obj_slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
				ScriptEngine::Enable_Hibernation( obj, false );
				char id_str[12];
				sprintf( id_str, "%d", MyID );
				ScriptEngine::Attach_Script( obj, "Test_Cinematic_Primary_Killed", id_str );
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", obj_slot );
			}
		}
	}

	void	Command_Move_Slot( char * params )
	{
		int new_slot = atoi( Get_First_Parameter( params ) );
		int old_slot = atoi( Get_Next_Parameter() );

		if ( (new_slot < 0) || ( new_slot >= NUM_SLOTS ) ) {
			new_slot = -1;
		}

		if ( (old_slot < 0) || ( old_slot >= NUM_SLOTS ) ) {
			old_slot = -1;
		}

		if ( new_slot != -1 && old_slot != -1 && new_slot != old_slot ) {
			ObjectSlots[ new_slot ] = ObjectSlots[ old_slot ];
			ObjectSlots[ old_slot ] = 0;
		}
	}

	void	Command_Sniper_Control( char * params )
	{
		int enabled = atoi( Get_First_Parameter( params ) );
		float zoom = strtof( Get_Next_Parameter() , nullptr);

		ScriptEngine::Cinematic_Sniper_Control( enabled != 0, zoom );
	}

	void	Command_Shake_Camera( char * params )
	{
		int obj_slot = atoi( Get_First_Parameter( params ) );
		float intensity = strtof( Get_Next_Parameter() , nullptr);
		float duration = strtof( Get_Next_Parameter() , nullptr);

		if ( (obj_slot < 0) || ( obj_slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Obj Slot Number %d\n", obj_slot );
			obj_slot = -1;
		}

		if ( obj_slot != -1 ) {
			int id = ObjectSlots[ obj_slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
				Vector3 pos = ScriptEngine::Get_Bone_Position( obj, "Camera" );
				ScriptEngine::Shake_Camera( pos, 100, intensity, duration );
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", obj_slot );
			}
		}
	}

	void	Command_Enable_Shadow( char * params )
	{
		int obj_slot = atoi( Get_First_Parameter( params ) );
		int onoff = atoi( Get_Next_Parameter() );

		if ( (obj_slot < 0) || ( obj_slot >= NUM_SLOTS ) ) {
//			ScriptEngine::Debug_Message( "Bad Obj Slot Number %d\n", obj_slot );
			obj_slot = -1;
		}

		if ( obj_slot != -1 ) {
			int id = ObjectSlots[ obj_slot ];
			GameObject * obj = ScriptEngine::Find_Object( id );
			if ( obj ) {
				ScriptEngine::Enable_Shadow(obj,(onoff != 0));
			} else {
//				ScriptEngine::Debug_Message( "Slot Object not found %d\n", obj_slot );
			}
		}
	}

	void	Command_Enable_Letterbox( char * params )
	{
		int onoff = atoi( Get_First_Parameter( params ) );
		float time = strtof( Get_Next_Parameter() , nullptr);

		ScriptEngine::Enable_Letterbox(!!onoff,time);
	}

	void Command_Set_Screen_Fade_Color( char * params )
	{
		float r = strtof( Get_First_Parameter( params ) , nullptr);
		float g = strtof( Get_Next_Parameter() , nullptr);
		float b = strtof( Get_Next_Parameter() , nullptr);
		float time = strtof( Get_Next_Parameter() , nullptr);

		ScriptEngine::Set_Screen_Fade_Color(r,g,b,time);
	}

	void Command_Set_Screen_Fade_Opacity( char * params )
	{
		float opacity = strtof( Get_First_Parameter( params ) , nullptr);
		float time = strtof( Get_Next_Parameter() , nullptr);

		ScriptEngine::Set_Screen_Fade_Opacity(opacity,time);
	}

	/*
	**
	*/
	bool	Title_Match( char ** command, const char * title )
	{
        if ( strnicmp( *command, title, strlen( title ) ) == 0 ) {
			*command += strlen( title );

			while ( **command && **command != ',' ) {
				(*command)++;
			}
			if ( **command == ',' ) {
				(*command)++;
			}
			return true;
		}
		return false;
	}

	void	Parse_Command( char *command )
	{
//		ScriptEngine::Debug_Message( "Parse %s\n", (int)command );

				if ( Title_Match( &command, "Create_Object" ) )			Command_Create_Object( command );
		else	if ( Title_Match( &command, "Create_Real_Object" ) )	Command_Create_Real_Object( command );
		else	if ( Title_Match( &command, "Create_Explosion" ) )		Command_Create_Explosion( command );
		else	if ( Title_Match( &command, "Destroy_Object" ) )		Command_Destroy_Object( command );
		else	if ( Title_Match( &command, "Add_Object" ) )			Command_Add_Object( command );
		else	if ( Title_Match( &command, "Play_Complete_Animation" ) )	Command_Play_Complete_Animation( command );
		else	if ( Title_Match( &command, "Stop_Animation" ) )		Command_Stop_Animation( command );
		else	if ( Title_Match( &command, "Play_Animation" ) )		Command_Play_Animation( command );
		else	if ( Title_Match( &command, "Play_Audio" ) )				Command_Play_Audio( command );
		else	if ( Title_Match( &command, "Show_Message" ) )			Command_Show_Message( command );
		else	if ( Title_Match( &command, "Control_Camera" ) )		Command_Control_Camera( command );
		else	if ( Title_Match( &command, "Send_Custom" ) )			Command_Send_Custom( command );
		else	if ( Title_Match( &command, "Attach_To_Bone" ) )		Command_Attach_To_Bone( command );
		else	if ( Title_Match( &command, "Attach_Script" ) )			Command_Attach_Script( command );
		else	if ( Title_Match( &command, "Set_Primary" ) )			Command_Set_Primary( command );
		else	if ( Title_Match( &command, "Move_Slot" ) )				Command_Move_Slot( command );
		else	if ( Title_Match( &command, "Sniper_Control" ) ) 		Command_Sniper_Control( command );
		else	if ( Title_Match( &command, "Shake_Camera" ) ) 			Command_Shake_Camera( command );
		else	if	( Title_Match( &command, "Enable_Shadow" ) )			Command_Enable_Shadow( command );
		else	if ( Title_Match( &command, "Enable_Letterbox" ) )		Command_Enable_Letterbox( command );
		else	if ( Title_Match( &command, "Set_Screen_Fade_Color" ) )	Command_Set_Screen_Fade_Color( command );
		else	if ( Title_Match( &command, "Set_Screen_Fade_Opacity" ) )	Command_Set_Screen_Fade_Opacity( command );
		else {
//			ScriptEngine::Debug_Message( "Failed to parse %s\n", (int)command );
		}
	}

	void	Parse_Commands( GameObject* obj ) {

		unsigned int sync_diff = ScriptEngine::Get_Sync_Time() - LastSyncTime;
		LastSyncTime += sync_diff;

		float bump_time = ((float)sync_diff) / 1000.0f;
		Time += bump_time;

		MyID = ScriptEngine::Get_ID( obj );

//		ScriptEngine::Debug_Message( "Cinematic Time %1.3f Frame %1.3f Bump Time %1.3f\n", Time, Time * 30.0f, bump_time );

		// If Primary Destroyed,
		if ( PrimaryKilled ) {
			// skip all timestamps < LAST_VALID_TIMESTAMP
			while ( Controls != nullptr && Controls->Time <= LAST_VALID_TIMESTAMP ) {
				Remove_Head_Control_Line();
			}

			// Run all remaining commands
			while ( Controls != nullptr ) {
				Parse_Command( Controls->Command );
				Remove_Head_Control_Line();
			}
		}

		while ( Controls != nullptr && Controls->Time <= Time ) {
			FrameSync = (Time - Controls->Time) * 30.0f;
			Parse_Command( Controls->Command );
			Remove_Head_Control_Line();
		}

		if ( Controls != nullptr ) {
			// if the next command is > than the LAST_VALID_TIMESTAMP, we are done
			if ( Controls->Time >= LAST_VALID_TIMESTAMP ) {
				ScriptEngine::Destroy_Object( obj );
			} else {
				float time_diff = Controls->Time - Time;
				ScriptEngine::Start_Timer( obj, this, time_diff, 0 );
			}
		} else {
			ScriptEngine::Destroy_Object( obj );
		}

		// Make sure we don't sleep
		if ( IsCameraCinematic ) {
			ScriptEngine::Enable_Cinematic_Freeze( obj, false );
		}

	}

	/*
	**
	*/
	void Created(GameObject* obj) override
	{
		Controls = nullptr;
		for ( int i = 0; i < NUM_SLOTS; i++ ) {
			ObjectSlots[ i ] = 0;
		}

		Load_Control_File( Get_Parameter( "ControlFilename" ) );

		//	A level written before the parameter existed does not carry it.
		StartPaused = ( Get_Parameter_Count() >= 2 ) && Get_Bool_Parameter( 1 );

		if ( !StartPaused ) {
			Start_Cinematic( obj );
		}
	}

	void Start_Cinematic( GameObject* obj )
	{
		ScriptEngine::Enable_Hibernation( obj, false );

		LastSyncTime = ScriptEngine::Get_Sync_Time();
		Time = 0;
		PrimaryKilled = false;
		IsCameraCinematic = false;

		Parse_Commands( obj );
	}

	void Timer_Expired (GameObject* obj, int /*timer_id*/) override
	{
//		ScriptEngine::Debug_Message("In Timer_Expired Get_Sync_Time is %d.\n", ScriptEngine::Get_Sync_Time());
		Parse_Commands(obj);
	}

	void	Custom( GameObject * obj, int type, intptr_t param, GameObject * /*sender*/ ) override
	{
		if ( type == M00_CUSTOM_CINEMATIC_START ) {
			if ( StartPaused ) {
				StartPaused = false;
				Start_Cinematic( obj );
			}
			return;
		}

		if ( type == M00_CUSTOM_CINEMATIC_PRIMARY_KILLED ) {
			if ( !PrimaryKilled ) {		// Prevent loops
				ScriptEngine::Debug_Message("Cinematic:Primary Killed\n");
				PrimaryKilled = true;
				Parse_Commands( obj );
			}
		}
		if ( type >= M00_CUSTOM_CINEMATIC_SET_SLOT ) {
			int slot = type - M00_CUSTOM_CINEMATIC_SET_SLOT;
			if ( slot < NUM_SLOTS ) {
				if ( ObjectSlots[ slot ] != 0 ) {
//					ScriptEngine::Debug_Message( "Slot number %d used by %d\n", slot, ObjectSlots[ slot ] );
				} else {
					ObjectSlots[ slot ] = param;
//					ScriptEngine::Debug_Message( "Slot number %d set to %d\n", slot, ObjectSlots[ slot ] );
				}
			}
		}
	}

};

#if 0
;_________________________________________
;
; Available Cinematic Script Commands
;
; time/frame	Create_Object,	id (slot), preset_name, x, y, z, facing, animation
; id can be -1 to mean do not store this object, and do not destroy
; 0 Create_Object, 0, .44 Magnum, 0, 0, 0, 180, "Human.jump"
;
; time/frame	Destroy_Object, id (slot)
; 0 Destroy_Object, 0
;
; time/frame	Play_Animation, id (slot), animation_name, looping, sub_obj_name
; 0 Play_Animation, 0, "Human.Jump", false
;
; time/frame	Control_Camera,	id ( slot )
; use id -1 for disabling control;
; note this will also disable star control and disbale the hud
; 0 Control_Camera,	0
;

TIME	Create_Object					SLOT	MODEL_NAME
TIME	Create_Real_Object			SLOT	PRESET_NAME	HOST_SLOT	HOST_BONE_NAME
TIME	Destroy_Object					SLOT
TIME	Play_Animation					SLOT	ANIMATION_NAME	LOOPING	SUB_OBJ_NAME	IS_BLENDED
TIME	Play_Audio						PRESET_NAME	HOST_SLOT	HOST_BONE_NAME
TIME	Control_Camera					SLOT
TIME	Send_Custon						[TO_ID/#TO_SLOT]	TYPE	[PARAM/#SLOT_PARAM]
TIME	Attach_To_Bone					SLOT	HOST_SLOT	HOST_BONE_NAME
TIME	Attach_Script					SLOT	SCRIPT_NAME	"SCRIPT_PARAMETERS"
TIME	Move_Slot						New_Slot, Old_Slot
TIME	Sniper_Control					ENABLED, ZOOM
TIME	Shake_Camera					SLOT, INTENSITY, DURATION
TIME	Enable_Shadow					SLOT, [1/0]
TIME	Enable_Letterbox				[1/0], TIME_TO_ANIMATE_IN
TIME	Set_Screen_Fade_Color		RED,GREEN,BLUE,TIME_TO_FADE (all colors floating point 0.0-1.0)
TIME	Set_Screen_Fade_Opacity		OPACITY,TIME_TO_FADE


To fill a slot, Send_Custom to the controller with
type = M00_CUSTOM_CINEMATIC_SET_SLOT + SLOT_NUMBER
parameter = OBJECT_ID

;_________________________________________
#endif

