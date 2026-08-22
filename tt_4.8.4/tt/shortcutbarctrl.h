/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/shortcutbarctrl.h        $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 11/13/01 9:53a                                              $*
 *                                                                                             *
 *                    $Revision:: 4                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __SHORTCUT_BAR_CTRL_H
#define __SHORTCUT_BAR_CTRL_H

#include "dialogcontrol.h"
#include "engine_vector.h"
#include "render2d.h"

////////////////////////////////////////////////////////////////
//
//	ShortcutBarCtrlClass
//
////////////////////////////////////////////////////////////////
class ShortcutBarCtrlClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ShortcutBarCtrlClass (void);
	virtual ~ShortcutBarCtrlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	RTTI
	//	
	ShortcutBarCtrlClass *		As_ShortcutBarCtrlClass (void)	{ return this; }

	//
	//	From DialogControlClass
	//	
	void					Render (void);
	const RectClass &	Get_Window_Rect (void) const;
	bool					Wants_Tooltip (void) const		{ return false; }

	//
	//	Configuration
	//
	void				Add_Button (int ctrl_id, const WCHAR *text);
	void				Remove_Button (int ctrl_id);

	void				Display (bool onoff)	{ IsHiddenState = !onoff; Set_Dirty (); }

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
	void				On_Mouse_Wheel (int direction);
	void				On_Create (void);
	void				On_Frame_Update (void);
	void				Update_Client_Rect (void);

	void				Create_Control_Renderers (void);
	void				Create_Hidden_State_Renderers (void);
	void				Create_Displayed_State_Renderers (void);
	void				Create_Text_Renderers (void);

	void				Set_Hidden_State (bool onoff);
	void				Get_Entry_Rect (int index, RectClass &rect);
	int				Entry_From_Pos (const Vector2 &mouse_pos);
	void				Set_Curr_Sel (int index, bool snap_mouse = false);
	void				Render_Strip (const RectClass &screen_rect, bool flip_uvs);
	void				Send_Command (void);
	void				Update_Pulse (void);


	////////////////////////////////////////////////////////////////
	//	Protected data types
	////////////////////////////////////////////////////////////////
	struct ShortcutInfoStruct
	{
		ShortcutInfoStruct (void) : ctrl_id (0) {}
		ShortcutInfoStruct (int _id, const WCHAR *_text) :
			ctrl_id (_id), text (_text) {}

		int					ctrl_id;
		WideStringClass	text;
		
		bool operator== (const ShortcutInfoStruct &src)	{ return false; }
		bool operator!= (const ShortcutInfoStruct &src)	{ return true; }
	};

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	DynamicVectorClass<ShortcutInfoStruct>	ShortcutList;
	Render2DSentenceClass	TextRenderer;
	Render2DSentenceClass	GlowRenderer;
	Render2DSentenceClass	HilightRenderer;
	Render2DClass				ControlRenderer;
	Render2DClass				TexturedControlRenderer;
	RectClass					FullRect;
	bool							IsHiddenState;
	int							CurrSel;
	float							BarOpacity;
	float							PulseDirection;
};


#endif //__SHORTCUT_BAR_CTRL_H
