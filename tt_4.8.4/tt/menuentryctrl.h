/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/menuentryctrl.h         $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 9/19/01 11:12a                                              $*
 *                                                                                             *
 *                    $Revision:: 7                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __MENU_ENTRY_CTRL_H
#define __MENU_ENTRY_CTRL_H

#include "dialogcontrol.h"
#include "vector3.h"
#include "render2d.h"

////////////////////////////////////////////////////////////////
//
//	MenuEntryCtrlClass
//
////////////////////////////////////////////////////////////////
class MenuEntryCtrlClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	MenuEntryCtrlClass (void);
	virtual ~MenuEntryCtrlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	From DialogControlClass
	//
	void					Render (void);
	bool					Wants_Tooltip (void) const	{ return false; }
	void					Center_Mouse (void);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////

	//
	//	From DialogControlClass
	//
	void					On_LButton_Down (const Vector2 &mouse_pos);
	void					On_LButton_Up (const Vector2 &mouse_pos);
	void					On_Mouse_Move (const Vector2 &mouse_pos);
	void					On_Set_Cursor (const Vector2 &mouse_pos);
	void					On_Create (void);	
	void					On_Set_Focus (void);
	void					On_Kill_Focus (DialogControlClass *);
	bool					On_Key_Down (uint32 key_id, uint32 key_data);
	bool					On_Key_Up (uint32 key_id);
	void					On_Mouse_Wheel (int direction);
	void					On_Frame_Update (void);
	void					Update_Client_Rect (void);

	void					Create_Text_Renderer (void);
	void					Set_State (int new_state);
	void					Update_State (void);

	void					Create_Glow (int radiusx, int radiusy, int color);
	void					On_Pushed (void);	


	////////////////////////////////////////////////////////////////
	//	Protected constants
	////////////////////////////////////////////////////////////////
	enum
	{
		UP				= 0,
		DOWN,
		HILIGHT,
		STATE_MAX
	};

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	Render2DSentenceClass	GlowRenderer; // 0054
	Render2DSentenceClass	TextRenderer; // 0194
	RectClass				MaxRect; // 02D4
	bool						WasButtonPressedOnMe; // 02E4
	bool						IsMouseOverMe; // 02E5
	bool						TriggerCommand; // 02E6
	int						CurrState; // 02E8

	int						CurrRadiusX; // 02EC
	int						CurrRadiusY; // 02F0
	int						StartTime; // 02F4
	int						EndTime; // 02F8
	int						CurrColor; // 02FC

	//static int				MaxDefaultRedValue;
	//static int				MaxHilightRedValue;
}; // 0300


#endif //__MENU_ENTRY_CTRL_H
