#ifndef TT_INCLUDE__DLGCNCREFERENCE_H
#define TT_INCLUDE__DLGCNCREFERENCE_H



#include "menudialog.h"



class DlgCnCReference :
	public MenuDialogClass
{

private:

	UNK unk0074; // 0074
	UNK unk0078; // 0078
	UNK unk007C; // 007C
	UNK unk0080; // 0080
	UNK unk0084; // 0084
	MenuBackDropClass* previousBackDrop; // 0088
	// ...

public:

	virtual void On_Menu_Activate(bool active);
	virtual void On_Destroy();

#pragma warning(suppress: 4510 4610) // Not being able to generate a default constructor is not a problem.
};



#endif