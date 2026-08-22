/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/popupdialog.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/02/01 2:36p                                              $*
 *                                                                                             *
 *                    $Revision:: 7                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __POPUP_DIALOG_H
#define __POPUP_DIALOG_H


#include "dialogbase.h"
#include "RenderObjClass.h"
#include "engine_string.h"
#include "render2d.h"


////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class SimpleSceneClass;
class CameraClass;


////////////////////////////////////////////////////////////////
//
//	PopupDialogClass
//
////////////////////////////////////////////////////////////////
class PopupDialogClass : public DialogBaseClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	PopupDialogClass (int res_id);
	virtual ~PopupDialogClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	RTTI
	//
	virtual PopupDialogClass *	As_PopupDialogClass (void)	{ return this; }

	//
	//	Display methods
	//
	virtual void				Render (void);

	virtual void Set_Title(const WCHAR* title);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void				Build_Background_Renderers (void);
	
	//
	// Set whether or not background is darkened
	//
	void				Set_Background_Darkened(bool flag)		{ IsBackgroundDarkened = flag; }

	//
	//	From DialogBaseClass
	//
	void				On_Init_Dialog (void);	


	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////	
	Render2DClass BlackoutRenderer; // 0074  0088
	Render2DClass BackgroundRenderer; // 0628  063C
	Render2DSentenceClass TextRenderer; // 0BDC  0BF0
	bool IsBackgroundDarkened; // 0D1C  0D30

}; // 0D20  0D34


#endif //__POPUP_DIALOG_H

