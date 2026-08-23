/******************************************************************************
*
* FILE
*     dlgobjectivestext.h
*
* DESCRIPTION
*     A scrollable page of plain text, named by file, opened on one player's
*     screen because a script on the server asked for it.
*
*     4.8.4 called this the objectives dialog.  A level ships a text file
*     alongside its own data and hands the name to JFW_Custom_Objectives_Dlg;
*     the file is read where it is displayed, on the client, so nothing but
*     the name travels.  One line of the file is one row of the list.
*
*     See playerterminal.h in Combat for who asks for it, and
*     scscriptcommandevent.h for how the request reaches this machine.
*
******************************************************************************/

#ifndef	__DLGOBJECTIVESTEXT_H
#define	__DLGOBJECTIVESTEXT_H

#include <wwui/popupdialog.h>

class	DlgObjectivesText : public PopupDialogClass
{
public:

	static bool	DoDialog (const char *filename);

protected:

	DlgObjectivesText (void);
	virtual ~DlgObjectivesText (void);

	void	On_Init_Dialog (void) override;
	void	On_Command (int ctrl, int message, unsigned int param) override;

	void	Fill_From_File (const char *filename);

private:

	DlgObjectivesText (const DlgObjectivesText &);
	const DlgObjectivesText & operator= (const DlgObjectivesText &);
};

#endif	//__DLGOBJECTIVESTEXT_H
