#include "General.h"
#include "DlgCnCReference.h"

#include "menubackdrop.h"



void DlgCnCReference::On_Menu_Activate(bool active)
{
	if (active)
	{
		MenuBackDropClass* backDrop = new MenuBackDropClass();
		backDrop->Set_Model("IF_LVL80LOAD");
		backDrop->Set_Animation("IF_LVL80LOAD.IF_LVL80LOAD");
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



void DlgCnCReference::On_Destroy()
{
	if (previousBackDrop)
	{
		delete Replace_BackDrop(previousBackDrop);
		previousBackDrop = NULL;
	}
}
