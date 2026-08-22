#ifndef TT_INCLUDE__DLGHELPSCREEN_H
#define TT_INCLUDE__DLGHELPSCREEN_H



#include "menudialog.h"



class DlgHelpScreen :
	public MenuDialogClass
{

private:

	MenuBackDropClass* previousBackDrop; // 0074
	// ...

public:

	virtual void On_Menu_Activate(bool active);
	virtual void On_Destroy();

#pragma warning(suppress: 4510 4610) // Not being able to generate a default constructor is not a problem.
};



#endif