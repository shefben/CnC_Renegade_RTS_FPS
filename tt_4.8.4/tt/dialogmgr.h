/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dialogmgr.h            $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/08/02 6:05p                                               $*
 *                                                                                             *
 *                    $Revision:: 22                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __DIALOG_MGR_H
#define __DIALOG_MGR_H

#include "engine_vector.h"
#include "vector3.h"
#include "wwuiinput.h"
#include "Vector2.h"

////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class DialogBaseClass;
class DialogControlClass;
class DialogTransitionClass;
class ToolTipClass;

////////////////////////////////////////////////////////////////
//	Public constants
////////////////////////////////////////////////////////////////
enum
{
	VKEY_PRESSED	= 0x80,
	VKEY_TOGGLED	= 0x01
};


////////////////////////////////////////////////////////////////
//
//	DialogMgrClass
//
////////////////////////////////////////////////////////////////
class DialogMgrClass
{
public:
	
	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Library management
	//
	static void		Initialize (const char *stylemgr_ini);
	static void		Init2();
	static void		Shutdown (void);

	//
	//	Per-frame processing
	//
	static void		Render (void);
	static void		On_Frame_Update (void);

	//
	//	Input support
	//
	static void		Install_Input (WWUIInputClass *instance)			{ REF_PTR_SET (Input, instance); }

	static IME::IMEManager* Get_IME(void)
		{return Input->GetIME();}

	static void Show_IME_Message(const wchar_t* message, uint32 duration);

	//
	//	Keyboard Input
	//
	static BYTE *	Get_Keyboard_State (void)								{ return KeyboardState; }
	static BYTE		Get_VKey_State (BYTE index)							{ return KeyboardState[index]; }

	static void Reset (void);

	//
	//	Mouse Input
	//
	//		Note X,Y are screen coordinates, while the Z component is the mouse wheel
	// position.
	//
	static const Vector3 &	Get_Mouse_Pos (void)							{ return Input->Get_Mouse_Pos (); }
	static void					Set_Mouse_Pos (const Vector3 &pos)		{ Input->Set_Mouse_Pos (pos); }
	
	static const Vector3 &	Get_Last_Mouse_Pos (void)					{ return LastMousePos; }
	static void					Set_Last_Mouse_Pos (const Vector3 &pos){ LastMousePos = pos; }

	//
	//	Mouse button input
	//
	static bool		Is_Button_Down (int vk_mouse_button_id)			{ return Input->Is_Button_Down (vk_mouse_button_id); };
	static bool		Was_Button_Down (int vk_mouse_button_id);

	//
	//	Dialog registration
	//
	static void		Register_Dialog (DialogBaseClass *dialog);
	static void		UnRegister_Dialog (DialogBaseClass *dialog);
	static void		Flush_Dialogs (void);
	static bool		Is_Flushing_Dialogs(void);

	//
	//	Timing support
	//
	static int		Get_Time (void)			{ return CurrTime; }
	static int		Get_Frame_Time (void)	{ return LastFrameTime; }

	//
	//	Active dialog support
	//
	static DialogBaseClass* Find_Dialog(int dialogID);
	static void						Set_Active_Dialog (DialogBaseClass *dialog);
	static DialogBaseClass *	Get_Active_Dialog (void)							{ return ActiveDialog; }
	static void						Rollback (DialogBaseClass *dialog);

	//
	//	Control support
	//
	static DialogControlClass *	Find_Control (const Vector2 &mouse_pos);

	//
	//	Input capture
	//
	static void							Set_Capture (DialogControlClass *control);
	static void							Release_Capture (void);
	static DialogControlClass *	Get_Capture (void)					{ return InputCapture; }

	//
	//	Focus support
	//
	static void							Set_Focus (DialogControlClass *control);
	static DialogControlClass *	Get_Focus (void)	{ return FocusControl; }

	//
	//	Transition support
	//
	static DialogBaseClass *		Peek_Transitioning_Dialog (void)	{ return TransitionDialog; }

	//
	//	Statistics
	//
	static int	Get_Dialog_Count (void)	{ return DialogList.Count (); }

private:

	////////////////////////////////////////////////////////////////
	//	Private constants
	////////////////////////////////////////////////////////////////
	enum
	{
		MB_LBUTTON	= 0,
		MB_MBUTTON,
		MB_RBUTTON,
		MB_XBUTTON1,
		MB_XBUTTON2,
		MB_COUNT,
	};

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////
	static void		On_Dialog_Added (void);
	static void		On_Dialog_Removed (void);
	static void		Update_Transition (void);
public:
	static void		Reset_Inputs (void); //this needs to be public
private:
	static void		Internal_Set_Active_Dialog (DialogBaseClass *dialog);

	//
	//	Keyboard input
	//
	static bool		On_Key_Down (uint32 key_id, uint32 key_data);
	static bool		On_Key_Up (uint32 key_id);
	static void		On_Unicode_Char(uint16 unicode);

	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////
	static DialogBaseClass **							TestArray;
	static int												TestArrayMaxCount;
	static int												TestArrayCount;
	static bool												IsFirstRender;
	static bool												IsInMenuMode;
	static REF_ARR_DECL1(KeyboardState, BYTE, 256);
	static bool												LastMouseButtonState[MB_COUNT];
	
	static Vector3											LastMousePos;

	static bool IsFlushing;

	static ToolTipClass* mIMEMessage;
	static uint32 mIMEMessageTime;
	static REF_DECL1(Input, WWUIInputClass*);
	static REF_DECL1(FocusControl, DialogControlClass*);
	static REF_DECL1(ActiveDialog, DialogBaseClass*);
	static REF_DECL1(InputCapture, DialogControlClass *);
	static REF_DECL1(Transition, DialogTransitionClass *);
	static REF_DECL1(PendingActiveDialog, DialogBaseClass *);
	static REF_DECL1(TransitionDialog, DialogBaseClass *);
	static REF_DECL1(CurrTime, uint32);
	static REF_DECL1(LastFrameTime, uint32);
	static REF_DECL1(DialogList, DynamicVectorClass<DialogBaseClass *>);

	////////////////////////////////////////////////////////////////
	//	Friend classes
	////////////////////////////////////////////////////////////////
	friend WWUIInputClass;
};

#endif //__DIALOG_MGR_H
