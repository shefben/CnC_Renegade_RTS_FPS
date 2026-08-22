/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/menudialog.cpp          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/02/02 11:10a                                              $*
 *                                                                                             *
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "General.h"
#include "menudialog.h"
#include "menubackdrop.h"
#include "render2d.h"
#include "stylemgr.h"
#include "dialogmgr.h"
#include "childdialog.h"
#include "dialogcontrol.h"
#include "ww3d.h"
////////////////////////////////////////////////////////////////
//	Static member initialization
////////////////////////////////////////////////////////////////
REF_DEF1(MenuDialogClass::BackDrop, MenuBackDropClass *, 0x0082FFEC);
REF_DEF1(MenuDialogClass::MenuStack, DynamicVectorClass<MenuDialogClass*>, 0x0082FFD0);
REF_DEF1(MenuDialogClass::ActiveMenu, MenuDialogClass*, 0x0082FFE8);

////////////////////////////////////////////////////////////////
//
//	MenuDialogClass
//
////////////////////////////////////////////////////////////////
MenuDialogClass::MenuDialogClass (int res_id)	:
	DialogBaseClass (res_id)
{	
	//
	//	Add ourselves to the global stack of menus
	//
	MenuStack.Add (this);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	~MenuDialogClass
//
////////////////////////////////////////////////////////////////
MenuDialogClass::~MenuDialogClass (void)
{
	if (ActiveMenu == this) {
		ActiveMenu = NULL;
	}

	//
	//	Remove ourselves from the stack
	//
	int index = MenuStack.ID (this);
	if (index != -1) {
		MenuStack.Delete (index);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Initialize
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Initialize (void)
{	
	BackDrop = new MenuBackDropClass;
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Shutdown
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Shutdown (void)
{
	if (BackDrop != NULL) {
		delete BackDrop;
		BackDrop = NULL;
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Render
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Render (void)
{
	//
	//	Don't render if we aren't the active menu
	//
	if (ActiveMenu == this || DialogMgrClass::Peek_Transitioning_Dialog () == this) {

		//
		//	Render the background scene first
		//
		BackDrop->Render();

		//
		//	Now, let the dialog subsystem render the controls and
		// such...
		//
		DialogBaseClass::Render ();
	}
	
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Init_Dialog
//
////////////////////////////////////////////////////////////////
/*void
MenuDialogClass::On_Init_Dialog (void)
{
	DialogBaseClass::Set_Default_Focus ();
	return ;
}*/


////////////////////////////////////////////////////////////////
//
//	Start_Dialog
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Start_Dialog (void)
{
	DialogBaseClass::Start_Dialog ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Activate
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::On_Activate (bool onoff)
{
	if (onoff) {

		//
		//	Notify the old menu
		//
		if (ActiveMenu != NULL) {
			ActiveMenu->On_Menu_Activate (false);
		}

		//
		//	Switch to active state
		//
		ActiveMenu = this;
		On_Menu_Activate (true);
	}

	DialogBaseClass::On_Activate (onoff);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Menu_Activate
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::On_Menu_Activate (bool onoff)
{
	return ;
}


////////////////////////////////////////////////////////////////
//
//	End_Dialog
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::End_Dialog (void)
{	
	//
	//	Is this the last menu?  If so, send a notification
	//
	if (DialogMgrClass::Is_Flushing_Dialogs () == false) {

		if (MenuStack.Count () == 1) {
			On_Last_Menu_Ending ();
		} else {

			//
			//	Play the sound effect
			//
			StyleMgrClass::Play_Sound (StyleMgrClass::EVENT_MENU_BACK);			
		}
	} 

	DialogBaseClass::End_Dialog ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Replace_BackDrop
//
////////////////////////////////////////////////////////////////
MenuBackDropClass *
MenuDialogClass::Replace_BackDrop (MenuBackDropClass *backdrop)
{
	MenuBackDropClass *retval = BackDrop;
	BackDrop = backdrop;
	return retval;
}
