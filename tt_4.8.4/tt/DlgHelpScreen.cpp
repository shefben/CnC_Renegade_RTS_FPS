#include "General.h"
#include "DlgHelpScreen.h"

#include "menubackdrop.h"



void DlgHelpScreen::On_Menu_Activate(bool active)
{
	if (active)
	{
		MenuBackDropClass* backDrop = new MenuBackDropClass();
		backDrop->Set_Model("IF_HELPLOAD");
		backDrop->Set_Animation("IF_HELPLOAD.IF_HELPLOAD");
		previousBackDrop = Replace_BackDrop(backDrop);
	}
	else
	{
		if (previousBackDrop)
		{
			delete Replace_BackDrop(previousBackDrop);
			previousBackDrop = NULL;
		}
	}
}



void DlgHelpScreen::On_Destroy()
{
	if (previousBackDrop)
	{
		delete Replace_BackDrop(previousBackDrop);
		previousBackDrop = NULL;
	}
}
