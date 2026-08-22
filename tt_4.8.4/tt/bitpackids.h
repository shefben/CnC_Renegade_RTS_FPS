/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwbitpack/bitpackids.h                       $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 10/14/01 11:15a                                             $*
 *                                                                                             *
 *                    $Revision:: 6                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef	BITPACKIDS_H
#define	BITPACKIDS_H

enum {
	/*
	//
	// 08/30/00 Use of pathfind extents for bitcompression decommissioned.
	// Flying vehicles and sea vehicles would have to use world extents
	// anyway.
	//

	//
	// Use these for objects restricted to pathfindable areas
	//
	BITPACK_PATHFIND_POSITION_X,
	BITPACK_PATHFIND_POSITION_Y,
	BITPACK_PATHFIND_POSITION_Z,
	*/

	//
	// Use these for objects restricted to world extents
	//
	BITPACK_WORLD_POSITION_X, // 0
	BITPACK_WORLD_POSITION_Y, // 1
	BITPACK_WORLD_POSITION_Z, // 2

	BITPACK_ONE_TIME_BOOLEAN_BITS, // 3
	BITPACK_CONTINUOUS_BOOLEAN_BITS, // 4
	BITPACK_ANALOG_VALUES, // 5

	BITPACK_HEALTH, // 6
	BITPACK_SHIELD_STRENGTH, // 7
	BITPACK_SHIELD_TYPE, // 8

	BITPACK_CTF_TEAM_FLAG, // 9

	BITPACK_HUMAN_STATE, // 10
	BITPACK_HUMAN_SUB_STATE, // 11

	BITPACK_VEHICLE_VELOCITY, // 12
	BITPACK_VEHICLE_ANGULAR_VELOCITY, // 13
	BITPACK_VEHICLE_QUATERNION, // 14
	BITPACK_VEHICLE_LOCK_TIMER, // 15

	BITPACK_DOOR_STATE, // 16
	BITPACK_ELEVATOR_STATE, // 17
	BITPACK_ELEVATOR_TOP_DOOR_STATE, // 18
	BITPACK_ELEVATOR_BOTTOM_DOOR_STATE, // 19

	BITPACK_BUILDING_RADIUS, // 20

	BITPACK_BUILDING_STATE, // 21

	BITPACK_CONTROL_MOVES_CS, // 22
	BITPACK_CONTROL_MOVES_SC, // 23

	BITPACK_PACKET_TYPE, // 24
	BITPACK_PACKET_ID, // 25

};

#endif	// BITPACKIDS_H
