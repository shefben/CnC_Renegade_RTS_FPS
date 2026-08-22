/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dialogtransition.h        $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 6/27/01 9:56a                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __DIALOG_TRANSITION_H
#define __DIALOG_TRANSITION_H


#include "engine_vector.h"


///////////////////////////////////////////////////////////////////////////////
//
//	DialogTransitionClass
//
///////////////////////////////////////////////////////////////////////////////
class DialogTransitionClass : public RefCountClass
{
public:

	////////////////////////////////////////////////////////////////////////
	//	Public constants
	////////////////////////////////////////////////////////////////////////
	enum TYPE
	{
		SCREEN_OUT	= 0,
		SCREEN_IN
	};
	
	////////////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////////////
	DialogTransitionClass (TYPE type = SCREEN_OUT);
	virtual ~DialogTransitionClass (void);

	////////////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////////////
	
	//
	//	Configuration
	//
	virtual void	Set_Type (TYPE type)		{ Type = type; }
	TYPE				Get_Type (void) const	{ return Type; }
	
	//
	//	Frame updates
	//
	virtual void	Render (void)				{}
	virtual void	On_Frame_Update (void)	{}

	//
	//	Flow control
	//
	virtual bool	Is_Complete (void)		{ return true; }

protected:

	////////////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////////////
	//virtual void	Update_Renderer (void);

	////////////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////////////	
	//Render2DClass	Renderer;
	//Render2DClass	SwipeRenderer;
	TYPE				Type;
	//int				MaxTime;
	//int				Timer;
};


#endif //__DIALOG_TRANSITION_H

