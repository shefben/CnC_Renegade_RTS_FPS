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
*     dlggamehint.h
*
* DESCRIPTION
*     The popup a level uses to explain itself: a title, three lines of text
*     and an optional picture.  See gamehint.h in Combat for who asks for it
*     and how a player only ever sees each one once.
*
******************************************************************************/

#ifndef	__DLGGAMEHINT_H
#define	__DLGGAMEHINT_H

#include <wwui/popupdialog.h>
#include "gamehint.h"


class	DlgGameHint : public PopupDialogClass
{
public:

	static bool	DoDialog (int title_id, int text_id, int text_id2, int text_id3,
			const char *texture_name);

protected:

	DlgGameHint (void);
	virtual ~DlgGameHint (void);

	void	On_Command (int ctrl, int message, unsigned int param) override;

private:

	DlgGameHint (const DlgGameHint &);
	const DlgGameHint & operator= (const DlgGameHint &);
};


//
//	The game's answer to Combat's hint interface.  Registered once at startup.
//
class	GameHintDialogClass : public GameHintInterfaceClass
{
public:

	void	Show_Hint (int title_id, int text_id, int text_id2, int text_id3,
			const char *texture_name) override;
};


#endif	//__DLGGAMEHINT_H
