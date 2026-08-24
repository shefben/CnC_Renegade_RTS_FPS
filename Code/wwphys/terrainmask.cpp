/*
**	OpenW3D
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

#include "terrainmask.h"

#include "wwmath.h"


TerrainMaskClass::TerrainMaskClass(void) :
	Values(nullptr),
	VertexCountX(0),
	VertexCountY(0),
	CellSize(1.0f),
	Origin(0.0f,0.0f,0.0f)
{
}


TerrainMaskClass::~TerrainMaskClass(void)
{
	Destroy();
}


bool TerrainMaskClass::Create(int vertex_count_x,int vertex_count_y,float cell_size,const Vector3 & origin)
{
	Destroy();

	if ((vertex_count_x < 2) || (vertex_count_y < 2) || (cell_size <= 0.0f)) {
		return false;
	}

	VertexCountX = vertex_count_x;
	VertexCountY = vertex_count_y;
	CellSize = cell_size;
	Origin = origin;

	Values = new float[VertexCountX * VertexCountY];
	Clear(0.0f);
	return true;
}


void TerrainMaskClass::Destroy(void)
{
	if (Values != nullptr) {
		delete [] Values;
		Values = nullptr;
	}
	VertexCountX = 0;
	VertexCountY = 0;
}


float TerrainMaskClass::Get(int ix,int iy) const
{
	if (Values == nullptr) {
		return 0.0f;
	}

	if (ix < 0) ix = 0;
	if (iy < 0) iy = 0;
	if (ix > VertexCountX - 1) ix = VertexCountX - 1;
	if (iy > VertexCountY - 1) iy = VertexCountY - 1;

	return Values[iy * VertexCountX + ix];
}


void TerrainMaskClass::Set(int ix,int iy,float value)
{
	if (Values == nullptr) {
		return;
	}
	if ((ix < 0) || (iy < 0) || (ix > VertexCountX - 1) || (iy > VertexCountY - 1)) {
		return;
	}
	Values[iy * VertexCountX + ix] = value;
}


void TerrainMaskClass::Clear(float value)
{
	if (Values == nullptr) {
		return;
	}
	int count = VertexCountX * VertexCountY;
	for (int index = 0; index < count; index++) {
		Values[index] = value;
	}
}


bool TerrainMaskClass::Set_Values(const float * values,int count)
{
	if ((Values == nullptr) || (values == nullptr)) {
		return false;
	}
	if (count != VertexCountX * VertexCountY) {
		return false;
	}

	for (int index = 0; index < count; index++) {
		Values[index] = values[index];
	}
	return true;
}


float TerrainMaskClass::Sample_Bilinear(float fx,float fy) const
{
	int ix = WWMath::Float_To_Int_Floor(fx);
	int iy = WWMath::Float_To_Int_Floor(fy);

	float u = fx - (float)ix;
	float v = fy - (float)iy;

	float v00 = Get(ix,  iy  );
	float v10 = Get(ix+1,iy  );
	float v01 = Get(ix,  iy+1);
	float v11 = Get(ix+1,iy+1);

	float lower = v00 + (v10 - v00) * u;
	float upper = v01 + (v11 - v01) * u;
	return lower + (upper - lower) * v;
}


bool TerrainMaskClass::Sample(float x,float y,float * value_out) const
{
	if ((Values == nullptr) || (value_out == nullptr)) {
		return false;
	}

	float fx = (x - Origin.X) / CellSize;
	float fy = (y - Origin.Y) / CellSize;

	if ((fx < 0.0f) || (fy < 0.0f) ||
		 (fx > (float)(VertexCountX - 1)) || (fy > (float)(VertexCountY - 1))) {
		return false;
	}

	*value_out = Sample_Bilinear(fx,fy);
	return true;
}


void TerrainMaskClass::Stamp_Disc(const Vector3 & center,float radius,float value,float feather)
{
	if ((Values == nullptr) || (radius <= 0.0f)) {
		return;
	}

	float outer = radius + ((feather > 0.0f) ? feather : 0.0f);

	//	Only the rows and columns the disc can reach are visited; a stamp is a local edit and
	//	sweeping the whole map for every road segment is how a road becomes a loading screen.
	int ix0 = WWMath::Float_To_Int_Floor((center.X - outer - Origin.X) / CellSize);
	int iy0 = WWMath::Float_To_Int_Floor((center.Y - outer - Origin.Y) / CellSize);
	int ix1 = WWMath::Float_To_Int_Floor((center.X + outer - Origin.X) / CellSize) + 1;
	int iy1 = WWMath::Float_To_Int_Floor((center.Y + outer - Origin.Y) / CellSize) + 1;

	if (ix0 < 0) ix0 = 0;
	if (iy0 < 0) iy0 = 0;
	if (ix1 > VertexCountX - 1) ix1 = VertexCountX - 1;
	if (iy1 > VertexCountY - 1) iy1 = VertexCountY - 1;

	for (int iy = iy0; iy <= iy1; iy++) {
		for (int ix = ix0; ix <= ix1; ix++) {

			float dx = (Origin.X + ix * CellSize) - center.X;
			float dy = (Origin.Y + iy * CellSize) - center.Y;
			float distance = WWMath::Sqrt(dx*dx + dy*dy);

			if (distance > outer) {
				continue;
			}

			float strength = 1.0f;
			if ((feather > 0.0f) && (distance > radius)) {
				float t = 1.0f - ((distance - radius) / feather);
				strength = t * t * (3.0f - 2.0f * t);			// smoothstep, so edges do not band
			}

			//	A stamp moves the value toward its own rather than replacing it.  Discs overlap
			//	at every joint of a polyline, and the feathered rim of a later one cannot undo
			//	what the centre of an earlier one wrote, because both are heading for the same
			//	value and the rim only ever gets part of the way there.
			float & existing = Values[iy * VertexCountX + ix];
			existing = existing + (value - existing) * strength;
		}
	}
}


void TerrainMaskClass::Stamp_Polyline(const Vector3 * points,int count,float width,float value,float feather)
{
	if ((points == nullptr) || (count < 2) || (width <= 0.0f)) {
		return;
	}

	float radius = width * 0.5f;

	//	Discs walked along each segment, half a radius apart, so the result is a continuous strip
	//	rather than a row of dots with gaps between them.
	float step = radius * 0.5f;
	if (step < CellSize * 0.5f) {
		step = CellSize * 0.5f;
	}

	for (int index = 0; index < count - 1; index++) {

		Vector3 from = points[index];
		Vector3 to = points[index + 1];
		Vector3 along = to - from;
		along.Z = 0.0f;

		float length = along.Length();
		if (length <= 0.0f) {
			Stamp_Disc(from,radius,value,feather);
			continue;
		}

		along /= length;

		for (float travelled = 0.0f; travelled <= length; travelled += step) {
			Stamp_Disc(from + along * travelled,radius,value,feather);
		}
		Stamp_Disc(to,radius,value,feather);
	}
}


bool TerrainMaskClass::Build_Distance_Field(const TerrainMaskClass & source,float threshold,float max_distance)
{
	if (!source.Is_Valid()) {
		return false;
	}

	if (!Is_Valid() ||
		 (VertexCountX != source.Get_Vertex_Count_X()) ||
		 (VertexCountY != source.Get_Vertex_Count_Y())) {

		if (!Create(source.Get_Vertex_Count_X(),source.Get_Vertex_Count_Y(),
						source.Get_Cell_Size(),source.Get_Origin())) {
			return false;
		}
	}

	CellSize = source.Get_Cell_Size();
	Origin = source.Get_Origin();

	float straight = CellSize;
	float diagonal = CellSize * 1.41421356f;

	//	Seed: zero where the source fires, saturated everywhere else.
	for (int iy = 0; iy < VertexCountY; iy++) {
		for (int ix = 0; ix < VertexCountX; ix++) {
			Values[iy * VertexCountX + ix] =
				(source.Get(ix,iy) >= threshold) ? 0.0f : max_distance;
		}
	}

	//	Forward sweep, then backward.  Two passes of a chamfer mask is not an exact Euclidean
	//	distance -- diagonals come out a few per cent long -- and nothing downstream of a
	//	shoreline blend can tell the difference.
	for (int iy = 0; iy < VertexCountY; iy++) {
		for (int ix = 0; ix < VertexCountX; ix++) {

			float best = Values[iy * VertexCountX + ix];

			if (ix > 0)					best = WWMath::Min(best,Values[iy*VertexCountX + ix-1] + straight);
			if (iy > 0)					best = WWMath::Min(best,Values[(iy-1)*VertexCountX + ix] + straight);
			if ((ix > 0) && (iy > 0))	best = WWMath::Min(best,Values[(iy-1)*VertexCountX + ix-1] + diagonal);
			if ((ix < VertexCountX-1) && (iy > 0)) {
				best = WWMath::Min(best,Values[(iy-1)*VertexCountX + ix+1] + diagonal);
			}

			Values[iy * VertexCountX + ix] = WWMath::Min(best,max_distance);
		}
	}

	for (int iy = VertexCountY - 1; iy >= 0; iy--) {
		for (int ix = VertexCountX - 1; ix >= 0; ix--) {

			float best = Values[iy * VertexCountX + ix];

			if (ix < VertexCountX-1)	best = WWMath::Min(best,Values[iy*VertexCountX + ix+1] + straight);
			if (iy < VertexCountY-1)	best = WWMath::Min(best,Values[(iy+1)*VertexCountX + ix] + straight);
			if ((ix < VertexCountX-1) && (iy < VertexCountY-1)) {
				best = WWMath::Min(best,Values[(iy+1)*VertexCountX + ix+1] + diagonal);
			}
			if ((ix > 0) && (iy < VertexCountY-1)) {
				best = WWMath::Min(best,Values[(iy+1)*VertexCountX + ix-1] + diagonal);
			}

			Values[iy * VertexCountX + ix] = WWMath::Min(best,max_distance);
		}
	}

	return true;
}


float TerrainMaskClass::Hash_Value(int ix,int iy,int seed)
{
	//	An integer hash, not a random number generator.  A generator carries state, and state
	//	means the answer depends on what order the map was walked in and on which machine walked
	//	it.  This depends on nothing but its arguments, which is what makes a server and a client
	//	agree about what the ground looks like without sending it.
	unsigned int value = (unsigned int)(ix * 0x1F1F1F1F) ^ (unsigned int)(iy * 0x3B9ACA07);
	value ^= (unsigned int)seed * 0x9E3779B9u;

	value ^= (value >> 15);
	value *= 0x2C1B3C6Du;
	value ^= (value >> 12);
	value *= 0x297A2D39u;
	value ^= (value >> 15);

	return (float)(value & 0x00FFFFFF) / (float)0x01000000;
}


void TerrainMaskClass::Fill_With_Noise(int seed,float frequency,float low,float high)
{
	if (Values == nullptr) {
		return;
	}

	if (frequency <= 0.0f) {
		frequency = 1.0f;
	}

	//	Value noise: hash on a coarse lattice, interpolated smoothly between.  Hashing every
	//	vertex directly would give a different value at every grid point, which is static rather
	//	than variation.
	for (int iy = 0; iy < VertexCountY; iy++) {
		for (int ix = 0; ix < VertexCountX; ix++) {

			float fx = (float)ix * frequency;
			float fy = (float)iy * frequency;

			int lx = WWMath::Float_To_Int_Floor(fx);
			int ly = WWMath::Float_To_Int_Floor(fy);

			float u = fx - (float)lx;
			float v = fy - (float)ly;
			u = u * u * (3.0f - 2.0f * u);
			v = v * v * (3.0f - 2.0f * v);

			float h00 = Hash_Value(lx,  ly,  seed);
			float h10 = Hash_Value(lx+1,ly,  seed);
			float h01 = Hash_Value(lx,  ly+1,seed);
			float h11 = Hash_Value(lx+1,ly+1,seed);

			float lower = h00 + (h10 - h00) * u;
			float upper = h01 + (h11 - h01) * u;
			float noise = lower + (upper - lower) * v;

			Values[iy * VertexCountX + ix] = low + (high - low) * noise;
		}
	}
}
