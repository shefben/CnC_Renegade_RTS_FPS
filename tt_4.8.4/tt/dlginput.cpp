#include "General.h"
#include "dlginput.h"
#include "Input.h"

bool RenegadeUIInputClass::Is_Button_Down(int vk_mouse_button_id)
{
	bool retval = false;
	switch (vk_mouse_button_id)
	{
		case VK_LBUTTON:
			retval = Input::Is_Button_Down (0x100);
			break;
		case VK_MBUTTON:
			retval = Input::Is_Button_Down (0x102);
			break;
		case VK_RBUTTON:
			retval = Input::Is_Button_Down (0x101);
			break;
		case VK_XBUTTON1:
			retval = Input::Is_Button_Down (0x103);
			break;
		case VK_XBUTTON2:
			retval = Input::Is_Button_Down (0x104);
			break;
	}
	return retval;
}

IME::IMEManager* WWUIInputClass::GetIME(void) const
{
	if (mIMEManager) {
		mIMEManager->Add_Ref();
	}

	return mIMEManager;
}
