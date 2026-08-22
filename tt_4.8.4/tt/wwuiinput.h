/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/wwuiinput.h           $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/08/02 5:32p                                               $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __WWUIINPUT_H
#define __WWUIINPUT_H

#include "vector3.h"
#include "engine_vector.h"
#include "IMEManager.h"

////////////////////////////////////////////////////////////////
//
//	WWUIInputClass
//
//		This class is used as an input communication mechanism
// between WWUI and the outside application.
//
////////////////////////////////////////////////////////////////
class WWUIInputClass :
	public RefCountClass,
	protected Observer<IME::UnicodeChar>,
	protected Observer<IME::IMEEvent>
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructor
	////////////////////////////////////////////////////////////////
	WWUIInputClass (void);
	virtual ~WWUIInputClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Mouse Input
	//
	//		Note: X,Y are screen coordinates, while the Z component is the
	// mouse wheel position.  The Set_Mouse_Position() call should
	//	reset the mouse position in the calling application.  Dialogs
	// use this to snap the mouse to the default control.
	//
	virtual const Vector3 &	Get_Mouse_Pos (void) const				= 0;
	virtual void				Set_Mouse_Pos (const Vector3 &pos)	= 0;

	//
	//	Mouse button input
	//
	virtual bool				Is_Button_Down (int vk_mouse_button_id) = 0;

	//
	//	Menu enter/exit callback
	//
	//		This is used so the controlling application can filter
	// input while in the menu system (if necessary).
	//
	virtual void				Enter_Menu_Mode (void)	{};
	virtual void				Exit_Menu_Mode (void)	{};

	bool ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result);

	void InitIME(HWND hwnd);

	IME::IMEManager* GetIME(void) const;
		
	protected:
		void HandleNotification(IME::UnicodeChar& unicode);
		void HandleNotification(IME::IMEEvent&);

	private:
		IME::IMEManager* mIMEManager;
};


#endif //__WWUIINPUT_H
