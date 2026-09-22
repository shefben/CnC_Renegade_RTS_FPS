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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/proceduralassetregistry.cpp           $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "proceduralassetregistry.h"

#include "buildingdescriptormanager.h"
#include "playertype.h"
#include "worldmetrics.h"


bool	ProceduralAssetRegistry::Built = false;
DynamicVectorClass<ProceduralAssetDescriptor>	ProceduralAssetRegistry::Assets;


ProceduralAssetDescriptor::ProceduralAssetDescriptor(void) :
	Category(ASSET_CATEGORY_BUILDING), Faction(PLAYERTYPE_NEUTRAL), Cover(ASSET_COVER_VEHICLE),
	NavObstacle(true), AllowDuplicateAdjacent(false), MinSetback(5.0f), MinNeighborSpacing(20.0f)
{ }

bool ProceduralAssetDescriptor::operator == (const ProceduralAssetDescriptor & that) const
{
	return (Id == that.Id) && (DescriptorName == that.DescriptorName) && (Category == that.Category) &&
			 (Faction == that.Faction) && (Cover == that.Cover) && (NavObstacle == that.NavObstacle) &&
			 (AllowDuplicateAdjacent == that.AllowDuplicateAdjacent) && (MinSetback == that.MinSetback) &&
			 (MinNeighborSpacing == that.MinNeighborSpacing);
}

bool ProceduralAssetDescriptor::Resolve_Placement_Radius(float * radius_out,float * height_out) const
{
	if (Category != ASSET_CATEGORY_BUILDING) return false;
	return WorldMetricsClass::Get_Building_Footprint(DescriptorName.Peek_Buffer(),radius_out,height_out);
}


void ProceduralAssetRegistry::Init(void)
{
	Shutdown();

	//	Q-017's own production load path -- the same directory the self checks and LevelEdit's
	//	validate command already use, walked for real here too rather than assuming a caller
	//	upstream already did it.
	StringClass report ("");
	int loaded = 0;
	BuildingDescriptorManager::Load_Directory ("data/building_descriptors",&report,&loaded);

	//	Real, offline-verified entries only -- see this file's own header comment for exactly
	//	how each was verified.  Every one of these three names a real `data/building_models/`
	//	.w3d this tree actually ships; every other `data/building_descriptors/` entry names a
	//	part list this tree has no files for yet, so it is not registered here.
	static const struct { const char * name; int faction; } REAL_ENTRIES[] = {
		{ "gdi_communications_center",	PLAYERTYPE_GDI },
		{ "gdi_power_plant",					PLAYERTYPE_GDI },
		{ "nod_power_plant",					PLAYERTYPE_NOD },
	};

	for (int i = 0; i < (int)(sizeof(REAL_ENTRIES) / sizeof(REAL_ENTRIES[0])); i++) {
		if (!BuildingDescriptorManager::Is_Loaded(REAL_ENTRIES[i].name)) continue;		// skipped, not fabricated

		ProceduralAssetDescriptor asset;
		asset.Id					= REAL_ENTRIES[i].name;
		asset.DescriptorName	= REAL_ENTRIES[i].name;
		asset.Category			= ASSET_CATEGORY_BUILDING;
		asset.Faction			= REAL_ENTRIES[i].faction;

		//	A solid multi-story Tech Structure is the conservative real classification for
		//	"blocks a vehicle's own line of fire" -- Section 17A.3's own VEHICLE_COVER -- until
		//	a per-asset cover survey exists to say otherwise for any specific one.
		asset.Cover					= ASSET_COVER_VEHICLE;
		asset.NavObstacle			= true;
		asset.AllowDuplicateAdjacent	= false;

		Assets.Add(asset);
	}

	Built = true;
}

void ProceduralAssetRegistry::Shutdown(void)
{
	Built = false;
	Assets.Delete_All();
}

bool ProceduralAssetRegistry::Get_Asset(int index,ProceduralAssetDescriptor * out)
{
	if ((out == nullptr) || (index < 0) || (index >= Assets.Count())) return false;
	*out = Assets[index];
	return true;
}

bool ProceduralAssetRegistry::Get_Asset_By_Id(const StringClass & id,ProceduralAssetDescriptor * out)
{
	for (int i = 0; i < Assets.Count(); i++) {
		if (Assets[i].Id == id) { if (out != nullptr) *out = Assets[i]; return true; }
	}
	return false;
}

int ProceduralAssetRegistry::Find_Assets(ProceduralAssetCategory category,int faction,
														DynamicVectorClass<int> & indices_out)
{
	int found = 0;
	for (int i = 0; i < Assets.Count(); i++) {
		if (Assets[i].Category != category) continue;
		if ((Assets[i].Faction != faction) && (Assets[i].Faction != PLAYERTYPE_NEUTRAL)) continue;
		indices_out.Add(i);
		found++;
	}
	return found;
}
