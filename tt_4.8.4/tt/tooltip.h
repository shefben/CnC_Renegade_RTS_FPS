/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/tooltip.h           $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/24/02 3:43p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __TOOLTIP_H
#define __TOOLTIP_H

#include "vector3.h"
#include "render2d.h"
#include "engine_string.h"


////////////////////////////////////////////////////////////////
//
//	ToolTipClass
//
////////////////////////////////////////////////////////////////
class ToolTipClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ToolTipClass (void);
	virtual ~ToolTipClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	//	Render support
	//
	void				Render (void);

	//
	//	Content control
	//
	void				Set_Text (const WCHAR *text);
	void				Set_Position (const Vector2 &pos);

private:

	////////////////////////////////////////////////////////////////
	//	Private methods
	////////////////////////////////////////////////////////////////
	void				Create_Text_Renderer (void);
	void				Create_Background_Renderer (void);
	void				Update_Rect (void);


	////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////
	Render2DSentenceClass 	TextRenderer;
	Render2DClass			BackgroundRenderer;	
	WideStringClass		Text;
	RectClass				Rect;

	Vector3					TextColor;
	Vector3					BkColor;
};


#endif //__TOOLTIP_H
