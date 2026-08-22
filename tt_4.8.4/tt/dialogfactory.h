/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/dialogfactory.h         $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 4/26/01 10:37a                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __DIALOG_FACTORY_H
#define __DIALOG_FACTORY_H

#include "dialogbase.h"


////////////////////////////////////////////////////////////////
//
//	DialogFactoryBaseClass
//
////////////////////////////////////////////////////////////////
class DialogFactoryBaseClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////
	virtual void	Do_Dialog (void)	= 0;
};


////////////////////////////////////////////////////////////////
//
//	DialogFactoryClass
//
////////////////////////////////////////////////////////////////
template<class  T>
class DialogFactoryClass : public DialogFactoryBaseClass
{
public:

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////
	void	Do_Dialog (void)	{ T *dialog = new T; dialog->Start_Dialog (); REF_PTR_RELEASE (dialog); }
};


#endif //__DIALOG_FACTORY_H

