/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/buttonctrl.h           $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/26/01 10:20a                                             $*
 *                                                                                             *
 *                    $Revision:: 7                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __BUTTON_CTRL_H
#define __BUTTON_CTRL_H

#include "dialogcontrol.h"
#include "vector3.h"
#include "render2d.h"


////////////////////////////////////////////////////////////////
//
//	ButtonCtrlClass
//
////////////////////////////////////////////////////////////////
class ButtonCtrlClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ButtonCtrlClass (void);
	virtual ~ButtonCtrlClass (void)	{}

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	// RTTI.
	virtual ButtonCtrlClass *As_ButtonCtrlClass (void)		{ return this; }

	//
	//	From DialogControlClass
	//
	void					Render (void);

	//
	//	Bitmap access
	//
	void					Set_Bitmap (const char *texture_up, const char *texture_dn);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					On_LButton_Down (const Vector2 &mouse_pos);
	void					On_LButton_Up (const Vector2 &mouse_pos);
	void					On_Mouse_Move (const Vector2 &mouse_pos);
	void					On_Set_Cursor (const Vector2 &mouse_pos);
	void					On_Create (void);
	void					On_Kill_Focus (DialogControlClass *focus);
	bool					On_Key_Down (uint32 key_id, uint32 key_data);
	void					On_Frame_Update (void);

	void					Create_Bitmap_Button (void);
	void					Create_Component_Button (void);
	void					Create_Component_Button2 (void);
	void					Create_Vector_Button (void);
	void					Create_Text_Renderers (void);

	void					Update_Pulse (bool is_mouse_over);


	////////////////////////////////////////////////////////////////
	//	Protected constants
	////////////////////////////////////////////////////////////////
	enum
	{
		UP				= 0,
		DOWN,
		STATE_MAX
	};

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	Render2DSentenceClass	GlowRenderer;
	Render2DSentenceClass	TextRenderer;
	Render2DClass				ButtonRenderers[STATE_MAX];
	bool							WasButtonPressedOnMe;
	bool							IsMouseOverMe;
	bool							IsBitmapMode;
	int							PulseTime;
};


#endif //__BUTTON_CTRL_H
