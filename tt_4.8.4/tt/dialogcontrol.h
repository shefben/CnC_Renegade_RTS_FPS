/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dialogcontrol.h           $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/09/02 4:06p                                               $*
 *                                                                                             *
 *                    $Revision:: 23                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __DIALOG_CONTROL_H
#define __DIALOG_CONTROL_H

#include "vector3.h"
#include "engine_string.h"
#include "controladvisesink.h"
#include "rect.h"

////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class DialogBaseClass;
class ButtonCtrlClass;
class CheckBoxCtrlClass;
class EditCtrlClass;
class ListCtrlClass;
class TabCtrlClass;
class DialogTextClass;
class InputCtrlClass;
class ButtonCtrlClass;
class MenuEntryCtrlClass;
class ComboBoxCtrlClass;
class ScrollBarCtrlClass;
class SliderCtrlClass;
class ViewerCtrlClass;
class ShortcutBarCtrlClass;


////////////////////////////////////////////////////////////////
//	Useful macros
////////////////////////////////////////////////////////////////
#define ADVISE_NOTIFY(fn)									\
		if (Parent != NULL)		{ Parent->fn; }		\
		if (AdviseSink != NULL)	{ AdviseSink->fn;	}
	

////////////////////////////////////////////////////////////////
//
//	DialogControlClass
//
////////////////////////////////////////////////////////////////
class DialogControlClass : public ControlAdviseSinkClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public friends
	////////////////////////////////////////////////////////////////
	friend class DialogBaseClass;
	friend class DialogMgrClass;

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	DialogControlClass (void);
	virtual ~DialogControlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	RTTI
	//
	virtual CheckBoxCtrlClass *		As_CheckBoxCtrlClass (void)	{ return NULL; }
	virtual EditCtrlClass *				As_EditCtrlClass (void)			{ return NULL; }
	virtual ListCtrlClass *				As_ListCtrlClass (void)			{ return NULL; }
	virtual TabCtrlClass *				As_TabCtrlClass (void)			{ return NULL; }
	virtual DialogTextClass *			As_DialogTextClass (void)		{ return NULL; }
	virtual InputCtrlClass *			As_InputCtrlClass (void)		{ return NULL; }
	virtual ButtonCtrlClass *			As_ButtonCtrlClass (void)		{ return NULL; }
	virtual MenuEntryCtrlClass *		As_MenuEntryCtrlClass (void)	{ return NULL; }
	virtual ComboBoxCtrlClass *		As_ComboBoxCtrlClass (void)	{ return NULL; }
	virtual ScrollBarCtrlClass *		As_ScrollBarCtrlClass (void)	{ return NULL; }
	virtual SliderCtrlClass *			As_SliderCtrlClass (void)		{ return NULL; }
	virtual ViewerCtrlClass *			As_ViewerCtrlClass (void)		{ return NULL; }	
	virtual ShortcutBarCtrlClass *	As_ShortcutBarCtrlClass (void){ return NULL; }
	virtual TreeCtrlClass *				As_TreeCtrlClass (void)			{ return NULL; }

	//
	//	Simple property access
	//
	virtual const WCHAR *		Get_Text (void) const							{ return Title; }
	virtual const RectClass &	Get_Window_Rect (void) const		{ return Rect; }
	Vector2				Get_Window_Pos (void) const					{ return Rect.Upper_Left (); }
	const RectClass &	Get_Client_Rect (void) const					{ return ClientRect; }
	DWORD					Get_Style (void) const							{ return Style; }
	int					Get_ID (void) const								{ return ID; }

	virtual void	Set_Text (const WCHAR *title)					{ Title = title; Set_Dirty ();  }
	void					Set_Window_Rect (const RectClass &rect)	{ Rect = rect; Update_Client_Rect (); }
	virtual void	Set_Window_Pos (const Vector2 &pos);
	void					Set_Client_Rect (const RectClass &rect)	{ ClientRect = rect; }
	virtual void	Set_Style (DWORD style)							{ Style = style; }
	void					Set_ID (int id)									{ ID = id; }
	
	//
	//	Parent access
	//
	DialogBaseClass *	Peek_Parent (void) const					{ return Parent; }
	void					Set_Parent (DialogBaseClass *parent)	{ Parent = parent; }

	//
	//	Advise sink access
	//
	ControlAdviseSinkClass *	Get_Advise_Sink (void) const							{ return AdviseSink; }
	void								Set_Advise_Sink (ControlAdviseSinkClass *sink)	{ AdviseSink = sink; }

	//
	//	Focus support
	//
	bool					Has_Focus (void) const			{ return HasFocus; }
	void					Set_Focus (void);
	bool					Wants_Focus (void) const		{ return WantsFocus; }
	void					Set_Wants_Focus (bool onoff)	{ WantsFocus = onoff; }

	//
	//	Enabled support
	//
	bool					Is_Enabled (void) const			{ return IsEnabled; }
	void					Enable (bool onoff);

	//
	//	Visibility
	//
	void					Show (bool onoff)			{ IsVisible = onoff; }
	bool					Is_Visible (void) const	{ return IsVisible; }

	//
	//	Embedded support
	//
	void					Set_Is_Embedded (bool onoff)	{ IsEmbedded = onoff; }
	bool					Is_Embedded (void) const		{ return IsEmbedded; }

	//
	//	Input capture support
	//
	void					Set_Capture (void);
	void					Release_Capture (void);

	//
	//	Tooltip support
	//
	virtual bool		Wants_Tooltip (void) const						{ return (ToolTipText.Get_Length () > 0); }
	virtual void		Get_Tooltip_Text (WideStringClass &text)	{ text = ToolTipText; }
	virtual void		Set_Tooltip_Text (const WCHAR *text)		{ ToolTipText = text; }

	//
	//	Appearance control
	//
	const Vector3 &	Get_Text_Color (void) const				{ return TextColor; }
	void					Set_Text_Color (const Vector3 &color)	{ TextColor = color; IsTextColorOverridden = true; Set_Dirty (); }

	//
	//	Display methods
	//
	virtual void		Render (void)								{ IsDirty = false; }
	virtual void		Center_Mouse (void);

	//
	//	Dirty support
	//
	virtual void		Set_Dirty (bool onoff = true)			{ IsDirty = onoff; }
	virtual bool		Is_Dirty (void) const					{ return IsDirty; }

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	virtual void			On_LButton_Down (const Vector2 &mouse_pos)	{}
	virtual void			On_LButton_DblClk (const Vector2 &mouse_pos)	{}
	virtual void			On_LButton_Up (const Vector2 &mouse_pos)		{}
	virtual void			On_RButton_Down (const Vector2 &mouse_pos)	{}
	virtual void			On_RButton_Up (const Vector2 &mouse_pos)		{}
	virtual void			On_MButton_Down (const Vector2 &mouse_pos)	{}
	virtual void			On_MButton_Up (const Vector2 &mouse_pos)		{}
	virtual void			On_Mouse_Move (const Vector2 &mouse_pos)		{}
	virtual bool			On_Key_Down (uint32 key_id, uint32 key_data)	{return false;}
	virtual bool			On_Key_Up (uint32 key_id)							{return false;}
	virtual void			On_Unicode_Char(WCHAR unicode) {}
	virtual void			On_Mouse_Wheel (int direction)					{}
	virtual void			On_Set_Cursor (const Vector2 &mouse_pos);
	virtual void			On_Create (void)										{}
	virtual void			On_Destroy (void)										{}
	virtual void			On_Add_To_Dialog (void)								{}
	virtual void			On_Remove_From_Dialog (void)						{}
	virtual void			On_Frame_Update (void)								{}
	virtual void			On_Set_Focus (void)									{ HasFocus = true; }
	virtual void			On_Kill_Focus (DialogControlClass *)			{ HasFocus = false; }
	virtual void			Update_Client_Rect (void)							{ ClientRect = Rect; Set_Dirty (); }

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	WideStringClass		Title; // 0004
	WideStringClass		ToolTipText; // 0008
	RectClass				Rect; // 000C
	RectClass				ClientRect; // 001C
	DWORD						Style; // 002C
	int						ID; // 0030
	DialogBaseClass *		Parent; // 0034
	bool						HasFocus; // 0038
	bool						IsDirty; // 0039
	bool						WantsFocus; // 003A
	bool						IsEnabled; // 003B
	bool						IsVisible; // 003C
	bool						IsEmbedded; // 003D
	Vector3					TextColor; // 0040
	bool						IsTextColorOverridden; // 004C

	ControlAdviseSinkClass *	AdviseSink; // 0050
}; // 0054


#endif //__DIALOG_CONTROL_H
