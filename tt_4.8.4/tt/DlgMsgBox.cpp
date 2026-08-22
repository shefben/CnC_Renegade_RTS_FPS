#include "General.h"
#include "DlgMsgBox.h"



REF_DEF1(DlgMsgBox::CurrentCount, bool, 0x0082F170);



RENEGADE_FUNCTION
void DlgMsgBox::DoDialog(const wchar_t* title, const wchar_t* message, DlgMsgBox::Type type, Observer<DlgMsgBoxEvent>* observer, uint32)
AT2(0x00492360, 0x00491B10);
