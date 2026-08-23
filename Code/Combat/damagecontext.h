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
*     damagecontext.h
*
* DESCRIPTION
*     What is being done to an object, for the duration of its being done.
*
*     An observer's Damaged and Killed are handed the victim, the responsible
*     object and how much health went, and that is all.  It is not enough for
*     a script that has to tell one kind of damage from another -- a hack that
*     only answers to a repair gun, an EMP that only answers to its own
*     warhead, a kill log that has to say whether a mine did it.  Those want
*     to know what hit them, and the answer exists a stack frame above them
*     and is thrown away before they are called.
*
*     So the damage path records it on the way past and clears it again after.
*     Both readings are only meaningful from inside a Damaged, Killed or
*     Destroyed handler; ask outside one and the warhead is NO_WARHEAD and the
*     explosion is null.
*
*     Nesting is safe.  Damage applied from inside a damage handler pushes its
*     own reading and puts the old one back, so the outer handler is not left
*     reading the inner blast.
*
******************************************************************************/

#ifndef DAMAGECONTEXT_H
#define DAMAGECONTEXT_H

#include "always.h"
#include "damage.h"

class ScriptableGameObj;


class	DamageContextClass
{
public:

	//	No damage is being applied, or the warhead was not recorded.
	static const WarheadType	NO_WARHEAD = (WarheadType)-1;

	//
	//	The warhead of the damage being applied right now.
	//
	static WarheadType	Get_Warhead (void)				{ return CurrentWarhead; }

	//
	//	The mine, C4 or beacon whose explosion is being resolved right now,
	//	or null when the damage did not come from one.  This is the device
	//	itself, not the player who placed it -- the responsible player is the
	//	damager the handler is already given.
	//
	static ScriptableGameObj *	Get_Explosion_Object (void)	{ return CurrentExplosion; }


	//
	//	Held for as long as one lot of damage is being applied.
	//
	class	WarheadScopeClass
	{
	public:
		explicit WarheadScopeClass (WarheadType warhead)
			:	Previous (CurrentWarhead)			{ CurrentWarhead = warhead; }
		~WarheadScopeClass (void)					{ CurrentWarhead = Previous; }

	private:
		WarheadType	Previous;

		WarheadScopeClass (const WarheadScopeClass &);
		WarheadScopeClass & operator= (const WarheadScopeClass &);
	};


	//
	//	Held for as long as one device's explosion is being resolved.
	//
	class	ExplosionScopeClass
	{
	public:
		explicit ExplosionScopeClass (ScriptableGameObj * source)
			:	Previous (CurrentExplosion)		{ CurrentExplosion = source; }
		~ExplosionScopeClass (void)				{ CurrentExplosion = Previous; }

	private:
		ScriptableGameObj *	Previous;

		ExplosionScopeClass (const ExplosionScopeClass &);
		ExplosionScopeClass & operator= (const ExplosionScopeClass &);
	};

private:

	static WarheadType			CurrentWarhead;
	static ScriptableGameObj *	CurrentExplosion;
};


#endif // DAMAGECONTEXT_H
