/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/sliderctrl.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 9/07/01 10:50a                                              $*
 *                                                                                             *
 *                    $Revision:: 6                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __SLIDER_CTRL_H
#define __SLIDER_CTRL_H

#include "dialogcontrol.h"
#include "vector3.h"
#include "render2d.h"

////////////////////////////////////////////////////////////////
//
//	SliderCtrlClass
//
////////////////////////////////////////////////////////////////
class SliderCtrlClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	SliderCtrlClass (void);
	virtual ~SliderCtrlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	From DialogControlClass
	//
	void				Render (void);

	//
	//	Selection management
	//
	void				Set_Range (int range_min, int range_max);
	void				Set_Pos (int pos, bool send_notification = true);
	int				Get_Pos (void) const								{ return CurrPos; }
	virtual SliderCtrlClass *			As_SliderCtrlClass (void)		{ return this; }

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void				On_LButton_Down (const Vector2 &mouse_pos);
	void				On_LButton_Up (const Vector2 &mouse_pos);
	void				On_Mouse_Move (const Vector2 &mouse_pos);
	void				On_Set_Cursor (const Vector2 &mouse_pos);
	void				On_Set_Focus (void);
	void				On_Kill_Focus (DialogControlClass *focus);
	bool				On_Key_Down (uint32 key_id, uint32 key_data);
	void				On_Create (void);
	void				Update_Client_Rect (void);

	void				Create_Control_Renderer (void);
	int				Slider_Pos_From_Mouse_Pos (const Vector2 &mouse_pos);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	Render2DClass			ControlRenderer;
	bool						IsDragging;
	int						MinPos;
	int						MaxPos;
	int						CurrPos;
};


#endif //__SLIDER_CTRL_H

