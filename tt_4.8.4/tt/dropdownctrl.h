/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dropdownctrl.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/16/01 10:27p                                             $*
 *                                                                                             *
 *                    $Revision:: 10                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __DROPDOWN_CTRL_H
#define __DROPDOWN_CTRL_H


#include "dialogcontrol.h"
#include "engine_vector.h"
#include "render2d.h"
#include "scrollbarctrl.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class ComboBoxCtrlClass;


////////////////////////////////////////////////////////////////
//
//	DropDownCtrlClass
//
////////////////////////////////////////////////////////////////
class DropDownCtrlClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	DropDownCtrlClass (void);
	virtual ~DropDownCtrlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	From DialogControlClass
	//
	void					Render (void);

	//
	//	Content control
	//
	int					Add_String (const WCHAR* string);
	void					Delete_String (int index);
	int					Find_String (const WCHAR* string);
	int					Find_Closest_String(const WCHAR* string);
	void					Set_Item_Data (int index, uint32 data);
	uint32				Get_Item_Data (int index);
	void					Reset_Content (void);
	
	//
	//	Information accessors
	//
	bool					Get_String (int index, WideStringClass &string) const;
	const WCHAR *		Get_String (int index) const;
	int					Get_Count (void)							{ return EntryList.Count (); }
	
	//
	//	Selection management
	//
	void					Set_Curr_Sel (int index);
	int					Get_Curr_Sel (void) const				{ return CurrSel; }

	//
	//	Combobox access
	//
	ComboBoxCtrlClass *	Get_Combo_Box (void) const						{ return ComboBox; }
	void						Set_Combo_Box (ComboBoxCtrlClass *ctrl)	{ ComboBox = ctrl; }

	void						Set_Full_Rect (const RectClass &rect)		{ FullRect = rect; }

	//
	//	Advise-sink callbacks
	//
	void					On_VScroll (ScrollBarCtrlClass *scrollbar, int ctrl_id, int new_position);


protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					On_LButton_Down (const Vector2 &mouse_pos);
	void					On_LButton_Up (const Vector2 &mouse_pos);
	void					On_Mouse_Move (const Vector2 &mouse_pos);
	void					On_Set_Cursor (const Vector2 &mouse_pos);
	void					On_Kill_Focus (DialogControlClass *focus);
	bool					On_Key_Down (uint32 key_id, uint32 key_data);
	void					On_Create (void);
	void					On_Add_To_Dialog (void);
	void					On_Remove_From_Dialog (void);
	void					Update_Client_Rect (void);

	void					Create_Control_Renderer (void);
	void					Create_Text_Renderer (void);

	int					Entry_From_Pos (const Vector2 &mouse_pos);
	void					Update_Scroll_Pos (void);


	////////////////////////////////////////////////////////////////
	//	Protected data types
	////////////////////////////////////////////////////////////////
	typedef struct _ENTRY
	{
		WideStringClass	text;
		uint32				user_data;

		bool operator== (const _ENTRY &src)	{ return false; }
		bool operator!= (const _ENTRY &src)	{ return true; }

		_ENTRY (void)	:
			user_data (0) {}

		_ENTRY (const WCHAR *_text, uint32 _data)	:
			text (_text), user_data (_data) {}

	} ENTRY;

	typedef DynamicVectorClass<ENTRY>	ENTRY_LIST;


	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	Render2DSentenceClass	TextRenderer;
	Render2DClass				ControlRenderer;
	Render2DClass				HilightRenderer;
	ENTRY_LIST					EntryList;
	ComboBoxCtrlClass *		ComboBox;
	Vector2						CellSize;
	ScrollBarCtrlClass		ScrollBarCtrl;
	RectClass					FullRect;
	RectClass					ScrollBarRect;
	int							CurrSel;
	int							ScrollPos;
	int							CountPerPage;
	bool							DisplayScrollBar;
	bool							ButtonClickedOnMe;
	int							DisplayTime;
};


#endif //__DROPDOWN_CTRL_H
