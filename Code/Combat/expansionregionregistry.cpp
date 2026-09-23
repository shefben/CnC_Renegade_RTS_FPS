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
 *                     $Archive:: /Commando/Code/Combat/expansionregionregistry.cpp           $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "expansionregionregistry.h"

#include "commanderplacement.h"
#include "crc.h"
#include "generatedworldplacementrules.h"
#include "navmeshgenerator.h"
#include "teampowergridservice.h"
#include "worldterrainsystem.h"
#include "wwmath.h"


ConstructionReservation::ConstructionReservation(void) :
	Id(""),DescriptorName(""),Position(0.0f,0.0f,0.0f),Radius(0.0f),
	RotationDegrees(0.0f),Team(-1),SequenceId(0),BuildingId(-1)
{
}

bool ConstructionReservation::operator == (const ConstructionReservation & that) const
{
	return (Id == that.Id) && (DescriptorName == that.DescriptorName) &&
			 (Position == that.Position) && (Radius == that.Radius) &&
			 (RotationDegrees == that.RotationDegrees) && (Team == that.Team) &&
			 (SequenceId == that.SequenceId) && (BuildingId == that.BuildingId);
}


ExpansionRegion::ExpansionRegion(void) :
	Id(""),Team(-1),Center(0.0f,0.0f,0.0f),Radius(0.0f),StructureCount(0),IsForward(false)
{
}

bool ExpansionRegion::operator == (const ExpansionRegion & that) const
{
	return (Id == that.Id) && (Team == that.Team) && (Center == that.Center) &&
			 (Radius == that.Radius) && (StructureCount == that.StructureCount) &&
			 (IsForward == that.IsForward);
}


int				ExpansionRegionRegistry::NextReservationOrdinal	= 0;
int				ExpansionRegionRegistry::NavigationRebuildCount	= 0;
int				ExpansionRegionRegistry::PowerRefreshCount			= 0;
int				ExpansionRegionRegistry::CompletionCount			= 0;
unsigned int	ExpansionRegionRegistry::Checksum						= 0;

DynamicVectorClass<ConstructionReservation>						ExpansionRegionRegistry::Reservations;
DynamicVectorClass<ExpansionRegionRegistry::CompletedStructure>	ExpansionRegionRegistry::Completed;
DynamicVectorClass<ExpansionRegion>									ExpansionRegionRegistry::Regions;


void ExpansionRegionRegistry::Init(void)		{ Clear(); }
void ExpansionRegionRegistry::Shutdown(void)	{ Clear(); }


void ExpansionRegionRegistry::Clear(void)
{
	NextReservationOrdinal	= 0;
	NavigationRebuildCount	= 0;
	PowerRefreshCount			= 0;
	CompletionCount			= 0;
	Checksum						= 0;

	Reservations.Delete_All();
	Completed.Delete_All();
	Regions.Delete_All();
}


int	ExpansionRegionRegistry::Get_Reservation_Count(void)		{ return Reservations.Count(); }
int	ExpansionRegionRegistry::Get_Navigation_Rebuild_Count(void)	{ return NavigationRebuildCount; }
int	ExpansionRegionRegistry::Get_Power_Refresh_Count(void)		{ return PowerRefreshCount; }
int	ExpansionRegionRegistry::Get_Completion_Count(void)			{ return CompletionCount; }
int	ExpansionRegionRegistry::Get_Region_Count(void)				{ return Regions.Count(); }
unsigned int	ExpansionRegionRegistry::Get_Checksum(void)		{ return Checksum; }


bool ExpansionRegionRegistry::Get_Reservation(int index,ConstructionReservation * out)
{
	if ((index < 0) || (index >= Reservations.Count()) || (out == nullptr)) return false;
	*out = Reservations[index];
	return true;
}


bool ExpansionRegionRegistry::Get_Region(int index,ExpansionRegion * out)
{
	if ((index < 0) || (index >= Regions.Count()) || (out == nullptr)) return false;
	*out = Regions[index];
	return true;
}


int ExpansionRegionRegistry::Get_Region_Count_For_Team(int team)
{
	int count = 0;
	for (int i = 0; i < Regions.Count(); i++) {
		if (Regions[i].Team == team) count++;
	}
	return count;
}


bool ExpansionRegionRegistry::Find_Region_At(const Vector3 & position,ExpansionRegion * out)
{
	for (int i = 0; i < Regions.Count(); i++) {

		const ExpansionRegion & region = Regions[i];

		float dx = position.X - region.Center.X;
		float dy = position.Y - region.Center.Y;
		if ((dx * dx + dy * dy) > (region.Radius * region.Radius)) continue;

		if (out != nullptr) *out = region;
		return true;
	}
	return false;
}


bool ExpansionRegionRegistry::Is_Reserved(const Vector3 & position,float radius)
{
	float query_radius = (radius > 0.0f) ? radius : 0.0f;

	for (int i = 0; i < Reservations.Count(); i++) {

		const ConstructionReservation & reservation = Reservations[i];

		float reach = reservation.Radius + query_radius;
		float dx = position.X - reservation.Position.X;
		float dy = position.Y - reservation.Position.Y;

		if ((dx * dx + dy * dy) < (reach * reach)) return true;
	}
	return false;
}


bool ExpansionRegionRegistry::Reserve(const char * descriptor_name,const Vector3 & position,
												  float rotation_degrees,int team,int sequence_id,
												  StringClass * reservation_id_out)
{
	if ((descriptor_name == nullptr) || (descriptor_name [0] == '\0')) return false;

	float radius = 0.0f;
	if (!CommanderPlacementValidator::Get_Footprint_Radius(descriptor_name,&radius)) return false;
	if (!(radius > 0.0f)) return false;

	//	The race this whole class exists to lose safely: the second claim on the same ground.
	if (Is_Reserved(position,radius)) return false;

	ConstructionReservation reservation;

	//	Stable id, Section 3B.5, built the same way every other record in this chain builds one.
	reservation.Id.Format("reservation_%d",NextReservationOrdinal);
	NextReservationOrdinal++;

	reservation.DescriptorName	= descriptor_name;
	reservation.Position			= position;
	reservation.Radius			= radius;
	reservation.RotationDegrees	= rotation_degrees;
	reservation.Team				= team;
	reservation.SequenceId		= sequence_id;
	reservation.BuildingId		= -1;

	Reservations.Add(reservation);

	if (reservation_id_out != nullptr) *reservation_id_out = reservation.Id;
	return true;
}


bool ExpansionRegionRegistry::Bind_Building(const char * reservation_id,int building_id)
{
	if (reservation_id == nullptr) return false;

	for (int i = 0; i < Reservations.Count(); i++) {
		if (Reservations[i].Id != reservation_id) continue;
		Reservations[i].BuildingId = building_id;
		return true;
	}
	return false;
}


bool ExpansionRegionRegistry::Release(const char * reservation_id)
{
	if (reservation_id == nullptr) return false;

	for (int i = 0; i < Reservations.Count(); i++) {
		if (Reservations[i].Id != reservation_id) continue;
		Reservations.Delete(i);
		return true;
	}
	return false;
}


bool ExpansionRegionRegistry::Release_By_Building(int building_id)
{
	if (building_id < 0) return false;

	for (int i = 0; i < Reservations.Count(); i++) {
		if (Reservations[i].BuildingId != building_id) continue;
		Reservations.Delete(i);
		return true;
	}
	return false;
}


/***********************************************************************************************
**	Step 12 -- what a completed construction changes.
*/
bool ExpansionRegionRegistry::Note_Construction_Complete(int building_id,const char * descriptor_name,
																			const Vector3 & position,int team,
																			StringClass * report_out)
{
	if ((descriptor_name == nullptr) || (descriptor_name [0] == '\0')) {
		if (report_out) *report_out = "no descriptor named";
		return false;
	}

	float radius = 0.0f;
	if (!CommanderPlacementValidator::Get_Footprint_Radius(descriptor_name,&radius)) {
		if (report_out) *report_out = "descriptor is not loaded";
		return false;
	}

	CompletedStructure structure;
	structure.DescriptorName	= descriptor_name;
	structure.Position			= position;
	structure.Radius				= radius;
	structure.Team					= team;
	structure.BuildingId			= building_id;
	Completed.Add(structure);

	CompletionCount++;

	StringClass report ("");

	/*
	**	Navigation.  A new structure is a new obstacle and the ground under it has just been
	**	graded, so the sector graph built from that ground is out of date.
	**
	**	A whole rebuild rather than a local patch, honestly: there is no incremental nav rebuild
	**	anywhere in this tree (Phase 16's own disclosed gap), and a stale graph that says a unit
	**	can walk through a Power Plant is worse than the cost of rebuilding.  Only attempted when
	**	navigation was built in the first place -- a hand-authored level has none and needs none.
	*/
	if (NavMeshGenerator::Has_Navigation()) {

		NavMeshParams params;
		StringClass nav_error ("");

		if (NavMeshGenerator::Build(params,&nav_error)) {
			NavigationRebuildCount++;
			report += "nav rebuilt; ";

			//	The chokepoint set is a property of the graph that was just replaced.
			StringClass choke_error ("");
			if (GeneratedWorldPlacementRules::Build_Chokepoints(&choke_error)) {
				report += "chokepoints rebuilt; ";
			} else {
				report += "chokepoints NOT rebuilt (";
				report += choke_error;
				report += "); ";
			}
		} else {
			report += "nav NOT rebuilt (";
			report += nav_error;
			report += "); ";
		}
	} else {
		report += "no navigation to rebuild; ";
	}

	/*
	**	Power and strategic capability.  Asked of the service that already owns the answer --
	**	`TeamPowerGridService` recomputes generation, demand and which members are powered for a
	**	team in one call, and a new building is exactly the event that invalidates all three.
	**	Nothing about power is recomputed here; this file only knows when to ask.
	*/
	if (team >= 0) {
		TeamPowerGridService::RecalculateTeam(team);
		PowerRefreshCount++;
		report += "power recalculated; ";
	} else {
		report += "no team to recalculate power for; ";
	}

	/*
	**	Spatial registration.  Not performed here on purpose: a completed building is wrapped in
	**	a StaticPhysClass and handed to PhysicsSceneClass by the construction path that created
	**	it (BuildingGameObj::Build_Runtime_Components), which is the one canonical registration.
	**	Doing it again here would put two collision objects in the scene for one building.
	*/
	report += "spatial registration owned by the construction path; ";

	//	Last, so nothing can claim this ground between the building appearing and the world
	//	agreeing that it has.
	if (Release_By_Building(building_id)) {
		report += "reservation released";
	} else {
		report += "no reservation to release";
	}

	if (report_out != nullptr) *report_out = report;
	return true;
}


/***********************************************************************************************
**	Step 13 -- forward-base/expansion region metadata.
**
**	Single-pass clustering in completion order: each structure joins the first region of its own
**	team whose centre is within `cluster_radius`, or starts one.  Order-dependent by design --
**	the order is the deterministic server-side completion order, so every peer replaying the same
**	sequence builds the same regions with the same ids.
*/
bool ExpansionRegionRegistry::Rebuild_Regions(float cluster_radius,StringClass * error_out)
{
	Regions.Delete_All();
	Checksum = 0;

	if (!(cluster_radius > 0.0f)) {
		if (error_out) *error_out = "cluster radius must be greater than zero";
		return false;
	}

	//	Parallel to Regions: the furthest any member sits from its region's running centre, so a
	//	region's radius contains every structure in it rather than only its seed.
	DynamicVectorClass<int> member_count;

	for (int i = 0; i < Completed.Count(); i++) {

		const CompletedStructure & structure = Completed[i];

		int target = -1;
		for (int r = 0; r < Regions.Count(); r++) {

			if (Regions[r].Team != structure.Team) continue;

			float dx = structure.Position.X - Regions[r].Center.X;
			float dy = structure.Position.Y - Regions[r].Center.Y;
			if ((dx * dx + dy * dy) <= (cluster_radius * cluster_radius)) { target = r; break; }
		}

		if (target < 0) {

			ExpansionRegion region;

			region.Id.Format("expansion_%d_%d",structure.Team,Get_Region_Count_For_Team(structure.Team));
			region.Team					= structure.Team;
			region.Center				= structure.Position;
			region.Radius				= structure.Radius;
			region.StructureCount	= 1;
			region.IsForward			= false;

			Regions.Add(region);
			member_count.Add(1);

		} else {

			ExpansionRegion & region = Regions[target];

			//	Running mean, so a region's centre is the structures in it rather than whichever
			//	one happened to be built first.
			int count = member_count[target];
			float weight = 1.0f / (float)(count + 1);

			region.Center = (region.Center * (1.0f - weight)) + (structure.Position * weight);
			region.StructureCount++;
			member_count[target] = count + 1;
		}
	}

	//	Radii, measured after every centre has settled.
	for (int r = 0; r < Regions.Count(); r++) {

		ExpansionRegion & region = Regions[r];
		float furthest = 0.0f;

		for (int i = 0; i < Completed.Count(); i++) {

			const CompletedStructure & structure = Completed[i];
			if (structure.Team != region.Team) continue;

			float dx = structure.Position.X - region.Center.X;
			float dy = structure.Position.Y - region.Center.Y;
			float distance = WWMath::Sqrt((dx * dx) + (dy * dy));
			if (distance > cluster_radius) continue;

			furthest = WWMath::Max(furthest,distance + structure.Radius);
		}

		region.Radius = WWMath::Max(furthest,region.Radius);
	}

	/*
	**	Which of them are forward positions.  The first region a team ever formed is that team's
	**	home by definition -- it is where its first completed structure went -- and everything
	**	beyond one cluster radius of it is forward.  Not a guess about intent: it is the only
	**	real home signal available without a base-ownership concept this tree does not have.
	*/
	for (int r = 0; r < Regions.Count(); r++) {

		int home = -1;
		for (int h = 0; h < Regions.Count(); h++) {
			if (Regions[h].Team == Regions[r].Team) { home = h; break; }
		}
		if (home < 0) continue;

		if (home == r) {
			Regions[r].IsForward = false;
			continue;
		}

		float dx = Regions[r].Center.X - Regions[home].Center.X;
		float dy = Regions[r].Center.Y - Regions[home].Center.Y;
		Regions[r].IsForward = ((dx * dx + dy * dy) > (cluster_radius * cluster_radius));
	}

	{
		CRCEngine crc;
		int region_count = Regions.Count();
		crc(&region_count,sizeof(region_count));

		for (int r = 0; r < Regions.Count(); r++) {

			const ExpansionRegion & region = Regions[r];

			crc(region.Id.Peek_Buffer(),region.Id.Get_Length());
			crc(&region.Team,sizeof(region.Team));
			crc(&region.StructureCount,sizeof(region.StructureCount));

			//	Section 3B.4 quantization -- a checksum over raw floats would differ between two
			//	peers that agree about the world to the last millimetre.
			int qx = (int)(region.Center.X * 1000.0f);
			int qy = (int)(region.Center.Y * 1000.0f);
			int qr = (int)(region.Radius * 1000.0f);
			int qf = region.IsForward ? 1 : 0;
			crc(&qx,sizeof(qx));
			crc(&qy,sizeof(qy));
			crc(&qr,sizeof(qr));
			crc(&qf,sizeof(qf));
		}

		Checksum = (unsigned int)crc;
	}

	return true;
}
