#ifndef TT_INCLUDE__DLGCNCWINSCREEN_H
#define TT_INCLUDE__DLGCNCWINSCREEN_H



#include "menudialog.h"
#include "menubackdrop.h"
#include "render2d.h"



class DlgCnCWinScreen :
	public MenuDialogClass
{

private:

	RenderObjClass* BackdropObj; // 0074  008C
	Render2DClass Renderer; // 0078  0090
	MenuBackDropClass Backdrop; // 062C  0644
	float Time; // 0648  0660
	bool IsLaddered; // 064C  0664

	void Update_List(int teamId, int listId);

public:

	DlgCnCWinScreen();
	~DlgCnCWinScreen();

	virtual void			Render (void);
	virtual void			On_Command (int ctrl_id, int mesage_id, DWORD param);
	virtual void			On_Init_Dialog (void);
	virtual void			On_Destroy (void)					{}
	virtual void			On_Frame_Update (void); //this needs to be public
	virtual void			On_Menu_Activate (bool onoff);

	static REF_DECL1(Instance, DlgCnCWinScreen *);
	static void Close_Dialog();

};



#endif
