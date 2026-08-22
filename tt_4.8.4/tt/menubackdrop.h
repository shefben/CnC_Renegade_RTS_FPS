/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/menubackdrop.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 11/21/01 11:30a                                             $*
 *                                                                                             *
 *                    $Revision:: 6                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __MENU_BACKDROP_H
#define __MENU_BACKDROP_H

#include "engine_string.h"
#include "RenderObjClass.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class SimpleSceneClass;
class CameraClass;


////////////////////////////////////////////////////////////////
//
//	MenuBackDropClass
//
////////////////////////////////////////////////////////////////
class MenuBackDropClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	MenuBackDropClass (void);
	virtual ~MenuBackDropClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Display methods
	//
	void					Render (void);

	//
	//	Configuration
	//
	void					Set_Model (const char *name);
	void					Remove_Model (void);
	void					Set_Animation (const char *animation_name_name);
	void					Set_Animation_Percentage( float percent );
	void					Clear_Screen (bool onoff)	{ ClearScreen = onoff; }

	//
	//	Accessors
	//
	SimpleSceneClass *	Peek_Scene (void)			{ return Scene; };
	CameraClass *			Peek_Camera (void)		{ return Camera; };
	RenderObjClass *		Peek_Model (void) const	{ return Model; }

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					Play_Animation (void);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	SimpleSceneClass *	Scene; // 0004
	CameraClass	*			Camera; // 0008
	StringClass				AnimationName; // 000C
	RenderObjClass *		Model; // 0010
	HAnimClass *			Anim; // 0014
	bool						ClearScreen; // 0018
}; // 001C


#endif //__MENU_BACKDROP_H
