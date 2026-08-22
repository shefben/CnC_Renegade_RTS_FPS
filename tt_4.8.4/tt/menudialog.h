/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/menudialog.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/25/01 5:15p                                              $*
 *                                                                                             *
 *                    $Revision:: 8                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __MENU_DIALOG_H
#define __MENU_DIALOG_H


#include "dialogbase.h"
#include "engine_string.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class MenuBackDropClass;


////////////////////////////////////////////////////////////////
//
//	MenuDialogClass
//
////////////////////////////////////////////////////////////////
class MenuDialogClass : public DialogBaseClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	MenuDialogClass (int res_id);
	virtual ~MenuDialogClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Initialization
	//
	static void						Initialize (void);
	static void						Shutdown (void);

	//
	//	RTTI
	//
	virtual MenuDialogClass *	As_MenuDialogClass (void)	{ return this; }

	//
	//	Configuration methods
	//
	static MenuBackDropClass *	Get_BackDrop (void)		{ return BackDrop; }
	static MenuBackDropClass *	Replace_BackDrop (MenuBackDropClass *backdrop);

	//
	//	Informational
	//
	bool						Is_Active_Menu (void) const	{ return (ActiveMenu == this); }

	//
	//	From DialogBaseClass
	//
	virtual void			Start_Dialog (void);
	virtual void			Render (void);
	virtual void			End_Dialog (void);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////

	//
	//	From DialogBaseClass
	//
	virtual void			On_Activate (bool onoff);

	//
	//	New notifications
	//
	virtual void			On_Menu_Activate (bool onoff);
	virtual void			On_Last_Menu_Ending (void) {}


	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////	
	static REF_DECL1(MenuStack, DynamicVectorClass<MenuDialogClass*>);
	static REF_DECL1(ActiveMenu, MenuDialogClass*);
	static REF_DECL1(BackDrop, MenuBackDropClass*);
}; // 0074  008C


#endif //__MENU_DIALOG_H

