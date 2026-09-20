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

/***********************************************************************************************
 *	Q-017 -- two Commander-buildable structures with no stock Renegade equivalent.               *
 *                                                                                             *
 *	`C:\Westwood\Renegade_full\Data\Objects.DDB` was enumerated in full (every CLASSID_BUILDINGS *
 *	definition it contains, by name and class) to answer Q-017.  It names a Tiberium Silo        *
 *	exactly once -- `SP_Silo_Nod`, a single-player map's set-dressing prop, generic              *
 *	`BuildingGameObjDef` with no gameplay behaviour of its own -- and never names an Advanced     *
 *	Communications Center at all.  Neither is the multiplayer economy structure this project's   *
 *	roadmap wants, so per Q-017's own instruction ("create a project-owned definition/preset      *
 *	using the correct building class and capabilities... do not fall back to a generic or bare    *
 *	building object merely so placement succeeds") this file constructs two real ones in code,    *
 *	the same way LevelEdit's own preset editor constructs a brand new preset                      *
 *	(`DefinitionMgrClass::Register_Definition`, see `PhysObjEditTab.cpp`) -- never touching       *
 *	`always.dat`.                                                                                *
 *                                                                                             *
 *	Class choice, deliberate, not the first match:                                               *
 *	  - Tiberium Silo -> plain `BuildingGameObjDef`.  Its entire gameplay behaviour is the team   *
 *	    wallet-capacity bonus, already data-driven end to end through                             *
 *	    `BuildingModelDescriptor::TiberiumSilo` and `TeamPowerGridService` -- nothing about it     *
 *	    needs a specialised C++ class.  Real multiplayer defense structures in the stock data      *
 *	    (`mp_GDI_Advanced_Guard_Tower`, `mp_Nod_Turret`, `mp_Nod_Obelisk`, `mp_Nod_Sam_Site`) are   *
 *	    the same base class for the same reason: their behaviour lives elsewhere.                  *
 *	  - Advanced Communications Center -> `ComCenterGameObjDef`, the same class stock             *
 *	    `mp_GDI_Com_Center` / `mp_Nod_Com_Center` use.  It is architecturally the advanced tier    *
 *	    of the same building type, and the roadmap's own "tactical coverage" behaviour for both    *
 *	    tiers belongs on that one class once it is built -- not yet done, tracked separately, and  *
 *	    this preset is the correct class to receive it.  Today it inherits whatever                *
 *	    `ComCenterGameObjDef` already does (the stock harvester-unload bookkeeping, unused by a    *
 *	    Commander-built instance) rather than nothing.                                             *
 *                                                                                             *
 *	Preset names are `ow_`-prefixed per the project's asset-naming convention, and are pure        *
 *	gameplay-identity strings: the runtime construction code in `RuntimeBuildingFactory` never     *
 *	sees them by name, only by whatever `game_object_preset` the matching JSON descriptor          *
 *	names -- these two names live in `data/building_descriptors/*_tiberium_silo.json` and          *
 *	`*_advanced_communications_center.json`, not hard-coded into any construction path.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROJECTBUILDINGDEFS_H
#define PROJECTBUILDINGDEFS_H

class ProjectBuildingDefs
{
	public:

		//	Idempotent: a second call is a no-op (checked by name), so callers never have to
		//	track whether this already ran.  Registers ow_gdi_tiberium_silo, ow_nod_tiberium_silo,
		//	ow_gdi_adv_comm_center and ow_nod_adv_comm_center.
		static void	Register(void);
};

#endif // PROJECTBUILDINGDEFS_H
