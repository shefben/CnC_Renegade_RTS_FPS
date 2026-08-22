/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors.
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

#include "scopemgr.h"
#include "ffactory.h"
#include "wwfile.h"
#include "wwdebug.h"

#include <stdlib.h>
#include <string.h>


#define	SCOPE_CONFIG_FILENAME		"scopes.cfg"

DynamicVectorClass<ScopeMgrClass::ScopeClass>	ScopeMgrClass::ScopeList;


/*
**	Split one "a=b=c=d" line into its four fields.  Anything shorter is a
**	comment, a blank line, or a typo, and is skipped rather than diagnosed --
**	the file is server data, not something the game ships.
*/
static bool
Parse_Scope_Line (char *line, ScopeMgrClass::ScopeClass &scope)
{
	char *fields[4] = { nullptr, nullptr, nullptr, nullptr };
	int count = 0;

	char *cursor = line;
	while (count < 4) {
		fields[count++] = cursor;
		char *sep = ::strchr (cursor, '=');
		if (sep == nullptr) {
			break;
		}
		*sep	= 0;
		cursor	= sep + 1;
	}

	if (count < 4) {
		return false;
	}

	for (int index = 0; index < 4; index ++) {
		//	Trim, so a hand-edited file with spaces around the separators works
		while (*fields[index] == ' ' || *fields[index] == '\t') {
			fields[index] ++;
		}
		int len = (int)::strlen (fields[index]);
		while (len > 0 && (fields[index][len-1] == ' ' || fields[index][len-1] == '\t')) {
			fields[index][--len] = 0;
		}
	}

	if (fields[0][0] == 0 || fields[1][0] == 0) {
		return false;
	}

	scope.WeaponPreset	= fields[0];
	scope.ScopeTexture	= fields[1];
	scope.MinZoom			= (float)::atof (fields[2]);
	scope.MaxZoom			= (float)::atof (fields[3]);

	//
	//	A zoom factor of zero would divide the field of view by nothing, so a
	//	line that does not name a usable range is dropped.
	//
	if (scope.MinZoom <= 0 || scope.MaxZoom <= 0) {
		return false;
	}

	return true;
}


void
ScopeMgrClass::Init (void)
{
	Shutdown ();

	if (_TheFileFactory == nullptr) {
		return;
	}

	FileClass *file = _TheFileFactory->Get_File (SCOPE_CONFIG_FILENAME);
	if (file == nullptr) {
		return;
	}

	if (file->Is_Available () && file->Open (FileClass::READ)) {

		int size = file->Size ();
		if (size > 0) {

			char *data = new char[size + 1];
			int read = file->Read (data, size);
			data[(read > 0) ? read : 0] = 0;

			char *line = data;
			while (line != nullptr && *line != 0) {

				char *next = ::strchr (line, '\n');
				if (next != nullptr) {
					*next++ = 0;
				}

				//	Tolerate CRLF
				int len = (int)::strlen (line);
				while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
					line[--len] = 0;
				}

				ScopeClass scope;
				if (Parse_Scope_Line (line, scope)) {
					ScopeList.Add (scope);
				}

				line = next;
			}

			delete [] data;
		}

		file->Close ();
	}

	_TheFileFactory->Return_File (file);
}


void
ScopeMgrClass::Shutdown (void)
{
	ScopeList.Delete_All ();
}


const ScopeMgrClass::ScopeClass *
ScopeMgrClass::Find (const char *weapon_preset)
{
	if (weapon_preset == nullptr) {
		return nullptr;
	}

	for (int index = 0; index < ScopeList.Count (); index ++) {
		if (::_stricmp (ScopeList[index].WeaponPreset, weapon_preset) == 0) {
			return &ScopeList[index];
		}
	}

	return nullptr;
}
