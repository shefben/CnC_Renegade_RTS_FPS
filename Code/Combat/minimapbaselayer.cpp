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
 *                     $Archive:: /Commando/Code/Combat/minimapbaselayer.cpp                  $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "minimapbaselayer.h"

#include "aabox.h"
#include "baseselectionplanner.h"
#include "bridgeplanner.h"
#include "crc.h"
#include "playertype.h"
#include "settlementgenerator.h"
#include "strategicplanner.h"
#include "terraintexturesystem.h"
#include "terrainmask.h"
#include "tiberiumfieldgenerator.h"
#include "watersystem.h"
#include "worldterrainsystem.h"


static const float ROAD_MASK_OCCUPIED_THRESHOLD = 0.5f;


const char * Minimap_Marker_Type_Name(MinimapMarkerType type)
{
	switch (type) {
		case MINIMAP_MARKER_BASE:					return "base";
		case MINIMAP_MARKER_SETTLEMENT:			return "settlement";
		case MINIMAP_MARKER_POI:					return "poi";
		case MINIMAP_MARKER_TIBERIUM_FIELD:		return "tiberium_field";
		case MINIMAP_MARKER_BRIDGE:				return "bridge";
		default:											return "unknown";
	}
}


MinimapMarker::MinimapMarker(void) :
	Type(MINIMAP_MARKER_BASE),
	Position(0.0f,0.0f,0.0f),
	Radius(0.0f)
{
}


bool MinimapMarker::operator == (const MinimapMarker & that) const
{
	return (Id == that.Id) && (Type == that.Type) &&
			 (Position == that.Position) && (Radius == that.Radius);
}


MinimapCellClass::MinimapCellClass(void) :
	AverageHeight(0.0f),
	IsWater(false),
	IsCliffBand(false),
	HasRoad(false)
{
}


bool				MinimapBaseLayer::Built			= false;
int				MinimapBaseLayer::CellCountX	= 0;
int				MinimapBaseLayer::CellCountY	= 0;
unsigned int	MinimapBaseLayer::Checksum		= 0;

DynamicVectorClass<MinimapCellClass>	MinimapBaseLayer::Cells;
DynamicVectorClass<MinimapMarker>		MinimapBaseLayer::Markers;


void MinimapBaseLayer::Init(void)
{
	Clear();
}


void MinimapBaseLayer::Shutdown(void)
{
	Clear();
}


void MinimapBaseLayer::Clear(void)
{
	Cells.Delete_All();
	Markers.Delete_All();
	Built				= false;
	CellCountX		= 0;
	CellCountY		= 0;
	Checksum			= 0;
}


static bool Reads_As_Occupied(TerrainMaskType mask_type,float x,float y)
{
	if (!TerrainTextureSystem::Has_Masks()) return false;
	TerrainMaskClass * mask = TerrainTextureSystem::Peek_Mask(mask_type);
	if (mask == nullptr) return false;
	float v = 0.0f;
	return mask->Sample(x,y,&v) && (v >= ROAD_MASK_OCCUPIED_THRESHOLD);
}


bool MinimapBaseLayer::Build(StringClass * error_out)
{
	Clear();

	if (!WorldTerrainSystem::Has_Chunk_Registry()) {
		if (error_out != nullptr) *error_out = "MinimapBaseLayer::Build -- no terrain chunk registry";
		return false;
	}

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();
	if ((count_x <= 0) || (count_y <= 0)) {
		if (error_out != nullptr) *error_out = "MinimapBaseLayer::Build -- chunk registry is empty";
		return false;
	}

	CellCountX = count_x;
	CellCountY = count_y;

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			MinimapCellClass cell;

			TerrainChunkInfoClass info;
			if (WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) {
				cell.AverageHeight	= (info.MinHeight + info.MaxHeight) * 0.5f;
				cell.IsCliffBand		= info.CliffBand;
			}

			AABoxClass bounds;
			if (WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) {
				cell.IsWater	= WaterSystem::Is_Point_In_Water(bounds.Center.X,bounds.Center.Y);
				cell.HasRoad	= Reads_As_Occupied(TERRAIN_MASK_ROAD,bounds.Center.X,bounds.Center.Y);
			}

			Cells.Add(cell);
		}
	}

	//	Bases (real, selected -- Phase 11).
	SelectedBaseSite gdi_site, nod_site;
	if (BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_GDI,&gdi_site)) {
		MinimapMarker marker;
		marker.Id			= gdi_site.Id;
		marker.Type			= MINIMAP_MARKER_BASE;
		marker.Position	= gdi_site.Position;
		marker.Radius		= gdi_site.ProtectedRadius;
		Markers.Add(marker);
	}
	if (BaseSelectionPlanner::Get_Selected_Site(PLAYERTYPE_NOD,&nod_site)) {
		MinimapMarker marker;
		marker.Id			= nod_site.Id;
		marker.Type			= MINIMAP_MARKER_BASE;
		marker.Position	= nod_site.Position;
		marker.Radius		= nod_site.ProtectedRadius;
		Markers.Add(marker);
	}

	//	Tiberium fields (real, resolved -- Phase 12).
	for (int i = 0; i < TiberiumFieldGenerator::Get_Field_Count(); i++) {
		ResolvedTiberiumField field;
		if (!TiberiumFieldGenerator::Get_Field(i,&field)) continue;
		MinimapMarker marker;
		marker.Id			= field.Id;
		marker.Type			= MINIMAP_MARKER_TIBERIUM_FIELD;
		marker.Position	= field.Position;
		marker.Radius		= field.Radius;
		Markers.Add(marker);
	}

	//	Settlements and POIs (real, resolved -- Phase 13).
	for (int i = 0; i < SettlementGenerator::Get_Settlement_Count(); i++) {
		ResolvedSettlement settlement;
		if (!SettlementGenerator::Get_Settlement(i,&settlement)) continue;
		MinimapMarker marker;
		marker.Id			= settlement.Id;
		marker.Type			= MINIMAP_MARKER_SETTLEMENT;
		marker.Position	= settlement.Position;
		marker.Radius		= settlement.Radius;
		Markers.Add(marker);
	}
	for (int i = 0; i < SettlementGenerator::Get_Poi_Count(); i++) {
		ResolvedPoi poi;
		if (!SettlementGenerator::Get_Poi(i,&poi)) continue;
		MinimapMarker marker;
		marker.Id			= poi.Id;
		marker.Type			= MINIMAP_MARKER_POI;
		marker.Position	= poi.Position;
		marker.Radius		= 0.0f;
		Markers.Add(marker);
	}

	//	Bridges (real, resolved -- Phase 9).  Position comes from the crossing node this bridge
	//	resolves -- the same real point NavMeshGenerator's own bridge portals search from.
	for (int i = 0; i < BridgePlanner::Get_Bridge_Count(); i++) {
		ResolvedBridge bridge;
		if (!BridgePlanner::Get_Bridge(i,&bridge)) continue;
		int node_index = -1;
		if (!StrategicPlanner::Find_Node_By_Id(bridge.CrossingNodeId,&node_index)) continue;
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(node_index,&node)) continue;
		MinimapMarker marker;
		marker.Id			= bridge.Id;
		marker.Type			= MINIMAP_MARKER_BRIDGE;
		marker.Position	= node.Position;
		marker.Radius		= 0.0f;
		Markers.Add(marker);
	}

	{
		CRCEngine crc;
		for (int i = 0; i < Cells.Count(); i++) {
			const MinimapCellClass & c = Cells[i];
			int q_height	= (int)(c.AverageHeight * 100.0f);
			int q_flags		= (c.IsWater ? 1 : 0) | (c.IsCliffBand ? 2 : 0) | (c.HasRoad ? 4 : 0);
			crc(&q_height,sizeof(q_height));
			crc(&q_flags,sizeof(q_flags));
		}
		for (int i = 0; i < Markers.Count(); i++) {
			const MinimapMarker & m = Markers[i];
			int q_type = (int)m.Type;
			int q_x = (int)(m.Position.X * 100.0f);
			int q_y = (int)(m.Position.Y * 100.0f);
			crc(m.Id.Peek_Buffer(),m.Id.Get_Length());
			crc(&q_type,sizeof(q_type));
			crc(&q_x,sizeof(q_x));
			crc(&q_y,sizeof(q_y));
		}
		Checksum = (unsigned int)crc;
	}

	Built = true;
	return true;
}


bool MinimapBaseLayer::Get_Cell(int px,int py,MinimapCellClass * out)
{
	if ((px < 0) || (px >= CellCountX) || (py < 0) || (py >= CellCountY) || (out == nullptr)) return false;
	int slot = px + (py * CellCountX);
	if ((slot < 0) || (slot >= Cells.Count())) return false;
	*out = Cells[slot];
	return true;
}


bool MinimapBaseLayer::Get_Marker(int index,MinimapMarker * out)
{
	if ((index < 0) || (index >= Markers.Count()) || (out == nullptr)) return false;
	*out = Markers[index];
	return true;
}
