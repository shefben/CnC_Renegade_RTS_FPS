/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/checkboxctrl.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 12/05/01 4:20p                                              $*
 *                                                                                             *
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __CHECKBOX_CTRL_H
#define __CHECKBOX_CTRL_H

#include "dialogcontrol.h"
#include "vector3.h"
#include "render2d.h"

////////////////////////////////////////////////////////////////
//
//	CheckBoxCtrlClass
//
////////////////////////////////////////////////////////////////
class CheckBoxCtrlClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	CheckBoxCtrlClass (void);
	virtual ~CheckBoxCtrlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	RTTI
	//	
	CheckBoxCtrlClass *		As_CheckBoxCtrlClass (void)	{ return this; }

	//
	//	From DialogControlClass
	//	
	void				Render (void);

	//
	//	Selection management
	//
	void				Set_Check (bool onoff);
	bool				Get_Check (void) const		{ return IsChecked; }

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void				Internal_Set_Check(bool onoff, bool notify);

	void				On_LButton_Down (const Vector2 &mouse_pos);
	void				On_LButton_Up (const Vector2 &mouse_pos);
	void				On_Set_Cursor (const Vector2 &mouse_pos);
	void				On_Set_Focus (void);
	void				On_Kill_Focus (DialogControlClass *focus);
	bool				On_Key_Down (uint32 key_id, uint32 key_data);
	void				On_Create (void);
	void				Update_Client_Rect (void);

	void				Create_Control_Renderers (void);
	void				Create_Text_Renderers (void);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	Render2DSentenceClass	TextRenderer;
	Render2DClass				ControlRenderer;
	Render2DClass				HilightRenderer;
	bool							IsChecked;
	RectClass					ButtonRect;
	RectClass					TextRect;
	RectClass					MaxRect;
};


#endif //__CHECKBOX_CTRL_H
