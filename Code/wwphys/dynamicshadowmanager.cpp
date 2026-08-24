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
 *                     $Archive:: /Commando/Code/wwphys/dynamicshadowmanager.cpp              $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 12/07/01 4:39p                                              $*
 *                                                                                             *
 *                    $Revision:: 14                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "dynamicshadowmanager.h"
#include "chunkio.h"
#include "phys.h"
#include "dyntexproject.h"
#include "pscene.h"
#include "physcoltest.h"
#include "light.h"
#include "texture.h"
#include "physresourcemgr.h"
#include "worldshadowmanager.h"



DynamicShadowManagerClass::DynamicShadowManagerClass(PhysClass & parent) :
	Parent(parent),
	Shadow(nullptr),
	ShadowNearZ(-1.0f),
	ShadowFarZ(-1.0f),
	ForceUseBlobBox(false),
	BlobBoxProjectionScale(1,1,1)
{
}

DynamicShadowManagerClass::~DynamicShadowManagerClass(void)
{
	Release_Shadow();
}


/*
**	Both of the classes that embed one of these -- MovePhysClass and DynamicAnimPhysClass --
**	call this once per post-timestep, and TT-facing code calls it directly.  The decision
**	itself belongs to WorldShadowManager, which is also what a registered world-system caster
**	goes through, so there is one implementation of "what does this object's shadow look like
**	this frame" rather than one per kind of caster.  Roadmap Section 24.
*/
void DynamicShadowManagerClass::Update_Shadow(void)
{
	WorldShadowManager::Update_Object_Shadow(Parent,&Shadow,ShadowNearZ,ShadowFarZ,
														  ForceUseBlobBox,BlobBoxProjectionScale);
}


void DynamicShadowManagerClass::Release_Shadow(void)
{
	WorldShadowManager::Release_Object_Shadow(&Shadow);
}

