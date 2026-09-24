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
 *	Q-017.  See projectbuildingdefs.h.                                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "projectbuildingdefs.h"

#include "comcentergameobj.h"
#include "constructionyardgameobj.h"
#include "definitionmgr.h"


/***********************************************************************************************
 * Register                                                                                       *
 *=============================================================================================*/
void ProjectBuildingDefs::Register(void)
{
	//
	//	No outer "only once, ever" guard: DefinitionMgrClass::Free_Definitions runs on every
	//	level load (Combat_Init, "Reload the definition databases (to support level-specific
	//	temp ddb's)") followed by a fresh SaveGameManager::Load_Definitions, so these four are
	//	gone and need re-adding every time this is called, not just the first.  The per-name
	//	Find_Named_Definition check below is what makes that safe to call unconditionally: a
	//	name already present (same load pass, or a definition file that happened to ship one)
	//	is left alone rather than double-registered.
	//
	#define REGISTER_IF_ABSENT(def_expr,preset_name,building_type)								\
		if (DefinitionMgrClass::Find_Named_Definition (preset_name,false) == nullptr) {		\
			auto *definition = def_expr;																	\
			definition->Set_Name (preset_name);														\
			definition->Set_Type (building_type);													\
			definition->Set_ID (DefinitionMgrClass::Get_New_ID (definition->Get_Class_ID ()));	\
			DefinitionMgrClass::Register_Definition (definition);								\
		}

	//
	//	Tiberium Silo -- plain BuildingGameObjDef.  See projectbuildingdefs.h for why: its whole
	//	gameplay contribution is the team wallet-capacity bonus, already carried entirely by
	//	BuildingModelDescriptor::TiberiumSilo and TeamPowerGridService, not by this class.
	//
	REGISTER_IF_ABSENT (new BuildingGameObjDef,"ow_gdi_tiberium_silo",BuildingConstants::TYPE_NONE);
	REGISTER_IF_ABSENT (new BuildingGameObjDef,"ow_nod_tiberium_silo",BuildingConstants::TYPE_NONE);

	//
	//	Advanced Communications Center -- the same ComCenterGameObjDef class stock
	//	mp_GDI_Com_Center / mp_Nod_Com_Center use; see projectbuildingdefs.h for why.
	//
	REGISTER_IF_ABSENT (new ComCenterGameObjDef,"ow_gdi_adv_comm_center",BuildingConstants::TYPE_COM_CENTER);
	REGISTER_IF_ABSENT (new ComCenterGameObjDef,"ow_nod_adv_comm_center",BuildingConstants::TYPE_COM_CENTER);

	//
	//	Construction Yard -- ConstructionYardGameObjDef, the class this engine already has for it.
	//	The retail definition database has no Construction Yard building definition: the
	//	mp_GDI_Construction_Yard / mp_Nod_Construction_Yard names in objects.ddb sit outside
	//	its building-definition chunks (docs/zerohour/BuildingPresets.md), so the preset is ours.
	//
	REGISTER_IF_ABSENT (new ConstructionYardGameObjDef,"ow_gdi_construction_yard",BuildingConstants::TYPE_CONYARD);
	REGISTER_IF_ABSENT (new ConstructionYardGameObjDef,"ow_nod_construction_yard",BuildingConstants::TYPE_CONYARD);

	#undef REGISTER_IF_ABSENT

	return ;
}
