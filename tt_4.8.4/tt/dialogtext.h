/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dialogtext.h            $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 10/26/01 12:25p                                             $*
 *                                                                                             *
 *                    $Revision:: 9                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __DIALOG_TEXT_H
#define __DIALOG_TEXT_H

#include "dialogcontrol.h"
#include "vector3.h"
#include "render2d.h"

////////////////////////////////////////////////////////////////
//
//	DialogTextClass
//
////////////////////////////////////////////////////////////////
class DialogTextClass : public DialogControlClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	DialogTextClass (void);
	virtual ~DialogTextClass (void)	{}

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	// RTTI
	DialogTextClass *As_DialogTextClass (void)		{ return (this); }

	//
	//	From DialogControlClass
	//
	void					Render (void);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					On_Create (void);
	void					Create_Text_Renderer (void);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	Render2DSentenceClass	TextRenderer;
	Render2DSentenceClass	GlowRenderer;
	Render2DClass				ControlRenderer;
	bool							IsTitle;
};


#endif //__DIALOG_TEXT_H
