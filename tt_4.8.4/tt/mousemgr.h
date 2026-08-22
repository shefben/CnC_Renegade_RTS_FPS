/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/mousemgr.h              $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/22/01 4:36p                                             $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __MOUSE_MGR_H
#define __MOUSE_MGR_H


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class TextureClass;
class ScreenCursorClass;


////////////////////////////////////////////////////////////////
//
//	MouseMgrClass
//
////////////////////////////////////////////////////////////////
class MouseMgrClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constants
	////////////////////////////////////////////////////////////////
	typedef enum
	{
		CURSOR_ARROW	= 0,
		CURSOR_TEXT,
		CURSOR_ACTION,
		CURSOR_BUSY,
		CURSOR_PAN_UP,
		CURSOR_ROTATE,
		CURSOR_COUNT
	} CURSOR_TYPE;

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Initialization
	//
	static void				Initialize (void);
	static void				Shutdown (void);

	//
	//	Display methods
	//
	static void				Show_Cursor (bool onoff);
	static void				Set_Cursor (CURSOR_TYPE type);
	static void				Begin_Wait_Cursor(void);
	static void				End_Wait_Cursor(void);

	//
	//	Render methods
	//
	static void				Render (void);

private:

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////	
	static TextureClass *			Textures[CURSOR_COUNT];
	static ScreenCursorClass *		MouseCursor;
	static CURSOR_TYPE				CursorType;

	static unsigned int CursorWaitCount;
};

#endif //__MOUSE_MGR_H

