#ifndef TT_INCLUDE__DLGMPCONNECTIONREFUSED_H
#define TT_INCLUDE__DLGMPCONNECTIONREFUSED_H



#include "popupdialog.h"



class DlgMPConnectionRefused :
	public PopupDialogClass
{

public:

	DlgMPConnectionRefused();

	static bool DoDialog(const wchar_t* message, bool arg2);

};



#endif