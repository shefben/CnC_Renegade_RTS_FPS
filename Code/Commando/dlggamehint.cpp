/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/******************************************************************************
*
* FILE
*     dlggamehint.cpp
*
* DESCRIPTION
*     See dlggamehint.h.
*
******************************************************************************/

#include "dlggamehint.h"

#include "imagectrl.h"
#include "renegadedialog.h"
#include "translatedb.h"


DlgGameHint::DlgGameHint (void) :
	PopupDialogClass (GetRenegadeDialog (RenegadeDialogID::IDD_GAME_HINT))
{
	return ;
}


DlgGameHint::~DlgGameHint (void)
{
	return ;
}


////////////////////////////////////////////////////////////////
//
//	DoDialog
//
////////////////////////////////////////////////////////////////
bool
DlgGameHint::DoDialog (int title_id, int text_id, int text_id2, int text_id3,
		const char *texture_name)
{
	DlgGameHint *popup = new DlgGameHint;
	if (popup == nullptr) {
		return false;
	}

	popup->Start_Dialog ();

	popup->Set_Title (TranslateDBClass::Get_String (title_id));

	//
	//	A line with no string ID is left blank rather than showing whatever
	//	string zero happens to be.
	//
	static const int	TEXT_CTRLS[] = { IDC_MESSAGE, IDC_MESSAGE2, IDC_MESSAGE3 };
	const int			text_ids[]	= { text_id, text_id2, text_id3 };

	for (int index = 0; index < 3; index ++) {
		popup->Set_Dlg_Item_Text (TEXT_CTRLS[index],
				(text_ids[index] != 0) ? TranslateDBClass::Get_String (text_ids[index]) : L"");
	}

	//
	//	The picture is optional; without one the control simply has no
	//	texture and draws nothing.
	//
	ImageCtrlClass *image = (ImageCtrlClass *)popup->Get_Dlg_Item (IDC_IMAGE);
	if ((image != nullptr) && (texture_name != nullptr)) {
		image->Set_Texture (texture_name);
	}

	popup->Release_Ref ();
	return true;
}


////////////////////////////////////////////////////////////////
//
//	On_Command
//
////////////////////////////////////////////////////////////////
void
DlgGameHint::On_Command (int ctrl, int message, unsigned int param)
{
	if (ctrl == IDOK) {
		End_Dialog ();
		return ;
	}

	PopupDialogClass::On_Command (ctrl, message, param);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	GameHintDialogClass::Show_Hint
//
////////////////////////////////////////////////////////////////
void
GameHintDialogClass::Show_Hint (int title_id, int text_id, int text_id2, int text_id3,
		const char *texture_name)
{
	DlgGameHint::DoDialog (title_id, text_id, text_id2, text_id3, texture_name);
	return ;
}
