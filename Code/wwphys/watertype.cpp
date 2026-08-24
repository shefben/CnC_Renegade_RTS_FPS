/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/watertype.cpp                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roadmap Section 22.  See watertype.h for what each field means and watersystem.h for the    *
 *	service that places and builds these.                                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "watertype.h"

#include "aabox.h"
#include "vector2.h"
#include "wwmath.h"

#include <string.h>


const char * Water_Category_Name(int category)
{
	switch (category) {
		case WATER_OCEAN:	return "ocean";
		case WATER_COAST:	return "coast";
		case WATER_LAKE:	return "lake";
		case WATER_POND:	return "pond";
		case WATER_RIVER:	return "river";
		case WATER_STREAM:	return "stream";
	}
	return "unknown";
}


bool Water_Category_Flows(int category)
{
	return (category == WATER_RIVER) || (category == WATER_STREAM);
}


bool Water_Category_Carves_Terrain(int category)
{
	return (category == WATER_LAKE) || (category == WATER_POND) ||
			 (category == WATER_RIVER) || (category == WATER_STREAM);
}


/***********************************************************************************************
 *	WaterDefinitionClass                                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
WaterDefinitionClass::WaterDefinitionClass(void) :
	Category(WATER_LAKE),
	ScrollSpeed(0.0f),
	WaveAmplitude(0.0f),
	ShoreFeather(2.0f),
	BedDepth(2.0f)
{
}


bool WaterDefinitionClass::operator == (const WaterDefinitionClass & that) const
{
	return (Name == that.Name) && (Category == that.Category) &&
			 (SurfaceTexture == that.SurfaceTexture) && (FlowTexture == that.FlowTexture) &&
			 (ScrollSpeed == that.ScrollSpeed) && (WaveAmplitude == that.WaveAmplitude) &&
			 (ShoreFeather == that.ShoreFeather) && (BedDepth == that.BedDepth);
}


void WaterDefinitionClass::Set_Name(const char * name)
{
	Name = (name != nullptr) ? name : "";
}


void WaterDefinitionClass::Set_Surface_Texture(const char * name)
{
	SurfaceTexture = (name != nullptr) ? name : "";
}


void WaterDefinitionClass::Set_Flow_Texture(const char * name)
{
	FlowTexture = (name != nullptr) ? name : "";
}


bool WaterDefinitionClass::Names_Any_Texture(void) const
{
	return (SurfaceTexture.Get_Length() > 0) || (FlowTexture.Get_Length() > 0);
}


/***********************************************************************************************
 *	WaterAreaClass                                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
WaterAreaClass::WaterAreaClass(void) :
	ID(-1), Closed(true), Height(0.0f), Phys(nullptr), TextureMissing(false)
{
}


WaterAreaClass::WaterAreaClass(const WaterAreaClass & that) :
	Name(that.Name), Definition(that.Definition), ID(that.ID),
	Closed(that.Closed), Height(that.Height),
	Phys(that.Phys), TextureMissing(that.TextureMissing)
{
	for (int i = 0; i < that.Stations.Count(); i++) { Stations.Add(that.Stations[i]); }
}


WaterAreaClass::~WaterAreaClass(void)
{
}


WaterAreaClass & WaterAreaClass::operator = (const WaterAreaClass & that)
{
	if (this != &that) {
		Name = that.Name;
		Definition = that.Definition;
		ID = that.ID;
		Closed = that.Closed;
		Height = that.Height;
		Phys = that.Phys;
		TextureMissing = that.TextureMissing;
		Stations.Delete_All();
		for (int i = 0; i < that.Stations.Count(); i++) { Stations.Add(that.Stations[i]); }
	}
	return *this;
}


bool WaterAreaClass::operator == (const WaterAreaClass & that) const
{
	if ((Name != that.Name) || (Definition != that.Definition) || (ID != that.ID) ||
		 (Closed != that.Closed) || (Height != that.Height) ||
		 (Stations.Count() != that.Stations.Count())) {
		return false;
	}
	for (int i = 0; i < Stations.Count(); i++) {
		if (Stations[i] != that.Stations[i]) { return false; }
	}
	return true;
}


void WaterAreaClass::Reset(void)
{
	Name = "";
	Definition = "";
	ID = -1;
	Closed = true;
	Height = 0.0f;
	Phys = nullptr;
	TextureMissing = false;
	Stations.Delete_All();
}


void WaterAreaClass::Set_Name(const char * name)
{
	Name = (name != nullptr) ? name : "";
}


void WaterAreaClass::Set_Definition(const char * name)
{
	Definition = (name != nullptr) ? name : "";
}


void WaterAreaClass::Add_Station(const Vector3 & position,float width)
{
	WaterStationClass station;
	station.Position = position;
	station.Width = width;
	Stations.Add(station);
}


static WaterStationClass _EmptyStation;

const WaterStationClass & WaterAreaClass::Get_Station(int index) const
{
	if ((index < 0) || (index >= Stations.Count())) {
		return _EmptyStation;
	}
	return Stations[index];
}


float WaterAreaClass::Get_Length(void) const
{
	float length = 0.0f;
	for (int i = 0; i < Stations.Count() - 1; i++) {
		length += (Stations[i+1].Position - Stations[i].Position).Length();
	}
	return length;
}


bool WaterAreaClass::Get_Bounds(AABoxClass * bounds_out) const
{
	if ((bounds_out == nullptr) || (Stations.Count() == 0)) {
		return false;
	}

	Vector3 lo = Stations[0].Position;
	Vector3 hi = Stations[0].Position;

	for (int i = 1; i < Stations.Count(); i++) {
		const Vector3 & p = Stations[i].Position;
		if (p.X < lo.X) lo.X = p.X;		if (p.X > hi.X) hi.X = p.X;
		if (p.Y < lo.Y) lo.Y = p.Y;		if (p.Y > hi.Y) hi.Y = p.Y;
		if (p.Z < lo.Z) lo.Z = p.Z;		if (p.Z > hi.Z) hi.Z = p.Z;
	}

	//	An open area's water extends half its widest station either side of the centre line.
	if (!Closed) {
		float half_width = 0.0f;
		for (int i = 0; i < Stations.Count(); i++) {
			if (Stations[i].Width * 0.5f > half_width) { half_width = Stations[i].Width * 0.5f; }
		}
		lo.X -= half_width;	hi.X += half_width;
		lo.Y -= half_width;	hi.Y += half_width;
	} else {
		lo.Z = Height;
		hi.Z = Height;
	}

	bounds_out->Init(0.5f * (lo + hi),0.5f * (hi - lo));
	return true;
}


/***********************************************************************************************
 * WaterAreaClass::Contains_Point -- is (x,y) over this body of water                           *
 *                                                                                             *
 * A closed area is a ray-crossing test against its ring of stations, the standard point in     *
 * polygon test, correct for the convex ponds and lakes this is meant for and wrong only on a   *
 * self-intersecting boundary nobody would draw.  An open area is a capsule: the nearest point   *
 * on the nearest segment, tested against that segment's own width so a river can narrow and     *
 * widen along its own length.                                                                   *
 *=============================================================================================*/
bool WaterAreaClass::Contains_Point(float x,float y) const
{
	int count = Stations.Count();
	if (count < 2) { return false; }

	if (Closed) {

		bool inside = false;
		for (int i = 0, j = count - 1; i < count; j = i++) {
			const Vector3 & pi = Stations[i].Position;
			const Vector3 & pj = Stations[j].Position;
			bool crosses = ((pi.Y > y) != (pj.Y > y));
			if (crosses) {
				float x_at_y = pi.X + (y - pi.Y) * (pj.X - pi.X) / (pj.Y - pi.Y);
				if (x < x_at_y) { inside = !inside; }
			}
		}
		return inside;
	}

	for (int i = 0; i < count - 1; i++) {

		const Vector3 & a = Stations[i].Position;
		const Vector3 & b = Stations[i+1].Position;

		Vector2 span(b.X - a.X,b.Y - a.Y);
		float length2 = span.X * span.X + span.Y * span.Y;
		float t = 0.0f;
		if (length2 > WWMATH_EPSILON) {
			t = ((x - a.X) * span.X + (y - a.Y) * span.Y) / length2;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
		}

		float nx = a.X + t * span.X;
		float ny = a.Y + t * span.Y;
		float dx = x - nx, dy = y - ny;
		float half_width = 0.5f * WWMath::Lerp(Stations[i].Width,Stations[i+1].Width,t);

		if ((dx * dx + dy * dy) <= (half_width * half_width)) {
			return true;
		}
	}
	return false;
}


/***********************************************************************************************
 * WaterAreaClass::Get_Surface_Height -- what height the water sits at over (x,y)               *
 *                                                                                             *
 * A closed body has one flat height.  An open body's height comes from the nearest station on  *
 * the nearest segment, which is what lets a river's surface drop along with its stations while *
 * still answering sensibly for a point off to the side of the line rather than exactly on it.   *
 *=============================================================================================*/
bool WaterAreaClass::Get_Surface_Height(float x,float y,float * height_out) const
{
	if (height_out == nullptr) { return false; }

	if (Closed) {
		*height_out = Height;
		return true;
	}

	int count = Stations.Count();
	if (count < 2) { return false; }

	float best_distance2 = -1.0f;
	float best_height = Stations[0].Position.Z;

	for (int i = 0; i < count - 1; i++) {

		const Vector3 & a = Stations[i].Position;
		const Vector3 & b = Stations[i+1].Position;

		Vector2 span(b.X - a.X,b.Y - a.Y);
		float length2 = span.X * span.X + span.Y * span.Y;
		float t = 0.0f;
		if (length2 > WWMATH_EPSILON) {
			t = ((x - a.X) * span.X + (y - a.Y) * span.Y) / length2;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
		}

		float nx = a.X + t * span.X;
		float ny = a.Y + t * span.Y;
		float dx = x - nx, dy = y - ny;
		float distance2 = dx * dx + dy * dy;

		if ((best_distance2 < 0.0f) || (distance2 < best_distance2)) {
			best_distance2 = distance2;
			best_height = WWMath::Lerp(a.Z,b.Z,t);
		}
	}

	*height_out = best_height;
	return true;
}


/***********************************************************************************************
 * WaterAreaClass::Get_Flow_Direction -- which way the water is going at (x,y)                  *
 *=============================================================================================*/
bool WaterAreaClass::Get_Flow_Direction(float x,float y,Vector3 * direction_out) const
{
	if (direction_out == nullptr) { return false; }

	*direction_out = Vector3(0.0f,0.0f,0.0f);

	if (Closed) { return true; }		// still water: no flow, and that is a real answer

	int count = Stations.Count();
	if (count < 2) { return false; }

	float best_distance2 = -1.0f;
	int best_segment = 0;

	for (int i = 0; i < count - 1; i++) {

		const Vector3 & a = Stations[i].Position;
		const Vector3 & b = Stations[i+1].Position;

		Vector2 span(b.X - a.X,b.Y - a.Y);
		float length2 = span.X * span.X + span.Y * span.Y;
		float t = 0.0f;
		if (length2 > WWMATH_EPSILON) {
			t = ((x - a.X) * span.X + (y - a.Y) * span.Y) / length2;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;
		}

		float nx = a.X + t * span.X;
		float ny = a.Y + t * span.Y;
		float dx = x - nx, dy = y - ny;
		float distance2 = dx * dx + dy * dy;

		if ((best_distance2 < 0.0f) || (distance2 < best_distance2)) {
			best_distance2 = distance2;
			best_segment = i;
		}
	}

	Vector3 span = Stations[best_segment + 1].Position - Stations[best_segment].Position;
	span.Z = 0.0f;
	if (span.Length2() > WWMATH_EPSILON) {
		span.Normalize();
	}
	*direction_out = span;
	return true;
}
