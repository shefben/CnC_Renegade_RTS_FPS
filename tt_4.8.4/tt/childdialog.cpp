/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/childdialog.cpp        $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 4/16/01 8:20a                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "General.h"
#include "childdialog.h"
#include "stylemgr.h"


////////////////////////////////////////////////////////////////
//
//	ChildDialogClass
//
////////////////////////////////////////////////////////////////
ChildDialogClass::ChildDialogClass (int res_id)	:
	ParentDialog (NULL),
	DialogBaseClass (res_id)
{	
	return ;
}


////////////////////////////////////////////////////////////////
//
//	~ChildDialogClass
//
////////////////////////////////////////////////////////////////
ChildDialogClass::~ChildDialogClass (void)
{	
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Init_Dialog
//
////////////////////////////////////////////////////////////////
void
ChildDialogClass::On_Init_Dialog (void)
{
	DialogBaseClass::On_Init_Dialog ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Render
//
////////////////////////////////////////////////////////////////
void
ChildDialogClass::Render (void)
{
	//
	//	Only render the controls... don't render any
	// backdrop or border.
	//
	DialogBaseClass::Render ();
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Is_Active
//
////////////////////////////////////////////////////////////////
bool
ChildDialogClass::Is_Active (void)
{
	bool retval = false;

	//
	//	We are automatically active if our parent is active
	//
	if (IsVisible && ParentDialog != NULL && ParentDialog->Is_Active ()) {
		//retval = true;
	}

	return retval;
}
