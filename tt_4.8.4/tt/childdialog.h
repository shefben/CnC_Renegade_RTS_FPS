/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/childdialog.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 7/17/01 5:37p                                               $*
 *                                                                                             *
 *                    $Revision:: 4                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __CHILD_DIALOG_H
#define __CHILD_DIALOG_H


#include "dialogbase.h"
#include "engine_string.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
//
//	ChildDialogClass
//
////////////////////////////////////////////////////////////////
class ChildDialogClass : public DialogBaseClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ChildDialogClass (int res_id);
	virtual ~ChildDialogClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	RTTI
	//
	virtual ChildDialogClass *	As_ChildDialogClass (void)	{ return this; }

	//
	//	From DialogBaseClass
	//
	virtual void			Render (void);
	virtual bool			Is_Active (void);
	virtual bool			Wants_Activation (void)	{ return false; }

	//
	//	Parent dialog access
	//
	void						Set_Parent_Dialog (DialogBaseClass *dialog)	{ ParentDialog = dialog; }
	DialogBaseClass *		Get_Parent_Dialog (void) const					{ return ParentDialog; }

	//
	//	Data management
	//
	//		Note: Returning false from either method will keep
	// the dialog from advancing.
	//
	virtual bool			On_Apply (void)	{ return true; }
	virtual bool			On_Discard (void)	{ return true; }

	virtual void			On_Reload (void)	{ }

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////

	//
	//	From DialogBaseClass
	//
	virtual void			On_Init_Dialog (void);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////	
	DialogBaseClass *		ParentDialog;
};


#endif //__CHILD_DIALOG_H

