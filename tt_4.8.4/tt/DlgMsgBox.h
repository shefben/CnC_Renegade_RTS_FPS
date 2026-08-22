#ifndef TT_INCLUDE__DLGMSGBOX_H
#define TT_INCLUDE__DLGMSGBOX_H



#include "notify.h"



class DlgMsgBoxEvent;



class DlgMsgBox
{

	static REF_DECL2(CurrentCount, bool);

public:

	enum Type
	{
		TYPE_0,
	};

	static void DoDialog(const wchar_t* title, const wchar_t* message, DlgMsgBox::Type type = TYPE_0, Observer<DlgMsgBoxEvent>* observer = NULL, uint32 = 0);
	static void DoDialog(int, int, DlgMsgBox::Type, Observer<DlgMsgBoxEvent>*, uint32);
	DlgMsgBox();
	~DlgMsgBox();
	UNK SetResourceType(DlgMsgBox::Type);
	UNK End_Dialog();
	UNK On_Command(int, int, uint32);
	UNK NotifyObservers(DlgMsgBoxEvent&);
	UNK AddObserver(Observer<DlgMsgBoxEvent>&);
	UNK RemoveObserver(Observer<DlgMsgBoxEvent>&);
	UNK Set_User_Data(uint32);
	UNK Get_User_Data();
	static int Get_Current_Count() { return CurrentCount; }

};


#endif
