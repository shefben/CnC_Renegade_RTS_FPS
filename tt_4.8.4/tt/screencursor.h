/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : W3DView                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/screencursor.h                                                                                                                                                                                                                                                                                                                                        $Modtime::                                                             $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __SCREENCURSOR_H
#define __SCREENCURSOR_H

#include "Vector2.h"
#include "render2d.h"

///////////////////////////////////////////////////////////////////////////////
//
//	ScreenCursorClass
//
///////////////////////////////////////////////////////////////////////////////
class ScreenCursorClass
{
public:
	
	////////////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////////////
	ScreenCursorClass (void);
	~ScreenCursorClass (void);

	////////////////////////////////////////////////////////////////////////
	//	Public operators
	////////////////////////////////////////////////////////////////////////
	const ScreenCursorClass &operator= (const ScreenCursorClass &src);

	////////////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////////////
	void						Set_Texture (TextureClass *texture);
	void						Render (void);

	void						Set_Hotspot (const Vector2 &pos)	{ Hotspot = pos; }
	const Vector2 &		Get_Hotspot (void) const			{ return Hotspot; }

private:

	////////////////////////////////////////////////////////////////////////
	//	Private member data
	////////////////////////////////////////////////////////////////////////	
	Vector2					Hotspot;
	int 						Width;
	int						Height;
	Render2DClass			Renderer;
	TextureClass *			Texture;
};


#endif //__SCREENCURSOR_H
