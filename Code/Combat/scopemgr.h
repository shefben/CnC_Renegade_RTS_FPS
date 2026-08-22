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

#ifndef	SCOPEMGR_H
#define	SCOPEMGR_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

#include "wwstring.h"
#include "vector.h"

/*
**	A scope is a per-weapon override of the sniper overlay and of how far the
**	sniper camera may zoom.  Without one, every scoped weapon in the game shares
**	the one procedural reticle and the one 1x-to-40x range.
**
**	The table is read from "scopes.cfg", one weapon per line, in the format a
**	server already ships:
**
**		<weapon preset>=<scope texture>=<min zoom>=<max zoom>
**
**	Zoom is a magnification factor, so 1 is unzoomed and 40 is what the stock
**	reticle reaches.
*/
class ScopeMgrClass
{
public:

	struct ScopeClass
	{
		StringClass		WeaponPreset;
		StringClass		ScopeTexture;
		float				MinZoom;
		float				MaxZoom;

		//	DynamicVectorClass wants these; nothing looks a scope up by value.
		bool operator == (const ScopeClass &other) const	{ return this == &other; }
		bool operator != (const ScopeClass &other) const	{ return this != &other; }
	};

	static void						Init (void);
	static void						Shutdown (void);

	//
	//	nullptr when this weapon has no scope of its own, which is the normal case.
	//
	static const ScopeClass *	Find (const char *weapon_preset);

private:

	static DynamicVectorClass<ScopeClass>	ScopeList;
};

#endif	// SCOPEMGR_H
