/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/tooltipmgr.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 4/09/01 12:08p                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __TOOLTIP_MGR_H
#define __TOOLTIP_MGR_H

#include "vector2.h"

////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class ToolTipClass;
class DialogControlClass;


////////////////////////////////////////////////////////////////
//
//	ToolTipMgrClass
//
////////////////////////////////////////////////////////////////
class ToolTipMgrClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Library management
	//
	static void		Initialize (void);
	static void		Shutdown (void);

	//
	//	Rendering
	//
	static void		Render (void);

	//
	//	Tooltip control
	//
	static void		Reset (void);
	static void		Update (const Vector2 &mouse_pos);

private:

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////
	static ToolTipClass *		ToolTip;
	static int						ToolTipDelay;
	static int						DefaultToolTipDelay;
	static bool						ToolTipDisplayed;
	static Vector2					LastMousePos;
	static int						PauseTime;
	static DialogControlClass *CurrentControl;
};


#endif //__TOOLTIP_MGR_H
