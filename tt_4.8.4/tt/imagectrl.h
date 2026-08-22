/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : wwui                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/imagectrl.h                              $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 8/30/01 9:51a                                                $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __IMAGECTRL_H
#define __IMAGECTRL_H


#include "render2d.h"
#include "dialogcontrol.h"

//////////////////////////////////////////////////////////////////////
//
//	ImageCtrlClass
//
//////////////////////////////////////////////////////////////////////
class ImageCtrlClass : public DialogControlClass
{
public:

	///////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	///////////////////////////////////////////////////////////////////
	ImageCtrlClass  (void);
	virtual ~ImageCtrlClass (void);

	///////////////////////////////////////////////////////////////////
	//	Public methods
	///////////////////////////////////////////////////////////////////

	//
	//	Inherited
	//
	void				Render (void);

	//
	//	Configuration
	//
	void				Set_Texture (const char *texture_name);
	
protected:
	
	///////////////////////////////////////////////////////////////////
	//	Protected methods
	///////////////////////////////////////////////////////////////////
	void				Create_Control_Renderer (void);
	void				Create_Texture_Renderer (void);
	
	///////////////////////////////////////////////////////////////////
	//	Protected member data
	///////////////////////////////////////////////////////////////////
	Render2DClass			ControlRenderer;
	Render2DClass			TextureRenderer;
};


#endif //__IMAGECTRL_H
