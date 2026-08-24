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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/wwphys.cpp                            $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 6/18/01 6:18p                                               $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "wwphys.h"
#include "physcon.h"
#include "physresourcemgr.h"
#include "surfaceribbonsystem.h"
#include "worldsurfacemarkmanager.h"


void WWPhys::Init(void)
{
	PhysicsConstants::Init();
	PhysResourceMgrClass::Init();

	//	Marks on the ground.  The pool is the only allocation the ribbon system ever makes, and
	//	the five kinds roadmap Section 23 names cost nothing until something lays one, so both
	//	happen here rather than waiting for a caller that would only ever say yes.
	SurfaceRibbonSystem::Init();
	SurfaceRibbonSystem::Define_Default_Ribbons();

	//	Marks on the ground and on the walls.  One bounded pool, one eviction policy, one place
	//	that knows how many marks the world is holding -- roadmap Section 35.
	WorldSurfaceMarkManager::Init();
	WorldSurfaceMarkManager::Define_Default_Marks();
}

void WWPhys::Shutdown(void)
{
	WorldSurfaceMarkManager::Shutdown();
	SurfaceRibbonSystem::Shutdown();
	PhysResourceMgrClass::Shutdown();
}


