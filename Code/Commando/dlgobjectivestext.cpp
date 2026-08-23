/******************************************************************************
*
* FILE
*     dlgobjectivestext.cpp
*
* DESCRIPTION
*     See dlgobjectivestext.h.
*
******************************************************************************/

#include "dlgobjectivestext.h"

#include "ffactory.h"
#include "wwfile.h"
#include "listctrl.h"
#include "renegadedialog.h"
#include "widestring.h"

#include <string.h>


//
//	The file name arrives over the wire, so it is not allowed to be a path:
//	a level names a file and gets the file, not a walk up out of the data
//	directory.
//
static bool	Is_Plain_File_Name (const char *filename)
{
	if ((filename == nullptr) || (filename[0] == 0)) {
		return false;
	}

	if (strchr (filename, '/') || strchr (filename, '\\') || strchr (filename, ':')) {
		return false;
	}

	return strstr (filename, "..") == nullptr;
}


DlgObjectivesText::DlgObjectivesText (void) :
	PopupDialogClass (GetRenegadeDialog (RenegadeDialogID::IDD_OBJECTIVES_TEXT))
{
	return ;
}


DlgObjectivesText::~DlgObjectivesText (void)
{
	return ;
}


bool
DlgObjectivesText::DoDialog (const char *filename)
{
	if (!Is_Plain_File_Name (filename)) {
		return false;
	}

	DlgObjectivesText *popup = new DlgObjectivesText;
	if (popup == nullptr) {
		return false;
	}

	popup->Start_Dialog ();
	popup->Fill_From_File (filename);
	popup->Release_Ref ();

	return true;
}


void
DlgObjectivesText::On_Init_Dialog (void)
{
	ListCtrlClass *list_ctrl = (ListCtrlClass *)Get_Dlg_Item (IDC_OBJECTIVES_LIST_CTRL);
	if (list_ctrl != nullptr) {
		list_ctrl->Add_Column (L"", 1.0F, Vector3 (1, 1, 1));
	}

	PopupDialogClass::On_Init_Dialog ();
	return ;
}


void
DlgObjectivesText::Fill_From_File (const char *filename)
{
	ListCtrlClass *list_ctrl = (ListCtrlClass *)Get_Dlg_Item (IDC_OBJECTIVES_LIST_CTRL);
	if (list_ctrl == nullptr) {
		return ;
	}

	FileClass *file = _TheFileFactory->Get_File (filename);
	if (file == nullptr) {
		return ;
	}

	if (!file->Is_Available () || !file->Open (FileClass::READ)) {
		_TheFileFactory->Return_File (file);
		return ;
	}

	int size = file->Size ();
	if (size > 0) {

		char *contents = new char[size + 1];
		int read = file->Read (contents, size);
		contents[(read > 0) ? read : 0] = 0;

		//
		//	One line of the file is one row.  A file written on Windows carries
		//	a carriage return the list would otherwise draw as a box.
		//
		int	row	= 0;
		char *line	= contents;

		while (line != nullptr) {

			char *next = strchr (line, '\n');
			if (next != nullptr) {
				*next = 0;
				next ++;
			}

			int length = (int)strlen (line);
			if ((length > 0) && (line[length - 1] == '\r')) {
				line[length - 1] = 0;
			}

			WideStringClass wide_line (line, true);
			list_ctrl->Insert_Entry (row, wide_line);
			row ++;

			line = next;
		}

		delete [] contents;
	}

	file->Close ();
	_TheFileFactory->Return_File (file);
	return ;
}


void
DlgObjectivesText::On_Command (int ctrl, int message, unsigned int param)
{
	if (ctrl == IDOK) {
		End_Dialog ();
		return ;
	}

	PopupDialogClass::On_Command (ctrl, message, param);
	return ;
}
