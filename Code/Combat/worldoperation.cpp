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
 *                     $Archive:: /Commando/Code/Combat/worldoperation.cpp                    $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldoperation.h"

#include "crc.h"
#include "wwmath.h"

#include <string.h>


//	Section 3B.4's own quantization, used by every checksum in this generation chain: a CRC over
//	raw floats would differ between two peers that agree about the world to the last millimetre.
static inline int Quantize(float value)		{ return (int)(value * 1000.0f); }


const char *	World_Operation_Type_Name(WorldOperationType type)
{
	switch (type) {
		case WORLD_OP_RAISE:						return "RAISE";
		case WORLD_OP_LOWER:						return "LOWER";
		case WORLD_OP_FLATTEN:					return "FLATTEN";
		case WORLD_OP_SMOOTH:					return "SMOOTH";
		case WORLD_OP_GRADE_FOUNDATION:		return "GRADE_FOUNDATION";
		case WORLD_OP_CARVE_ROAD:				return "CARVE_ROAD";
		case WORLD_OP_CARVE_CHANNEL:			return "CARVE_CHANNEL";
		case WORLD_OP_CREATE_CRATER:			return "CREATE_CRATER";
		case WORLD_OP_RECLAIM_SHALLOW_WATER:	return "RECLAIM_SHALLOW_WATER";
		case WORLD_OP_RESTORE_REGION:			return "RESTORE_REGION";
		default:										return "unknown";
	}
}


const char *	World_Operation_Result_Name(WorldOperationResultType result)
{
	switch (result) {
		case WORLD_OP_RESULT_APPLIED:					return "applied";
		case WORLD_OP_RESULT_BAD_FORMAT_VERSION:	return "unsupported format version";
		case WORLD_OP_RESULT_BAD_TYPE:				return "unknown operation type";
		case WORLD_OP_RESULT_BAD_PARAMETERS:		return "bad parameters";
		case WORLD_OP_RESULT_NO_TERRAIN:				return "no terrain";
		case WORLD_OP_RESULT_NO_JOURNAL:				return "no journal open";
		case WORLD_OP_RESULT_STALE_REVISION:		return "stale world revision";
		case WORLD_OP_RESULT_CHUNKS_RESERVED:		return "affected chunks are reserved";
		case WORLD_OP_RESULT_PROTECTED_REGION:		return "protected region";
		case WORLD_OP_RESULT_BOUNDARY_BAND:			return "boundary cliff band";
		case WORLD_OP_RESULT_CRATER_LIMIT:			return "crater exceeds its bounds";
		case WORLD_OP_RESULT_RATE_LIMITED:			return "rate limited";
		case WORLD_OP_RESULT_DISABLED:				return "operation type disabled";
		default:												return "unknown";
	}
}


unsigned int	World_Operation_Dependencies(WorldOperationType type)
{
	//	Render and collision are invalidated by every type here, because every type moves the
	//	ground -- there is no operation in this list that changes only a material.  What differs is
	//	everything downstream of the ground.
	unsigned int common = WORLD_OP_DEPENDS_RENDER | WORLD_OP_DEPENDS_COLLISION;

	switch (type) {

		//	Small, local, and not deep enough to reach a road or a waterline: nav only, because a
		//	slope that was climbable may not be any more.
		case WORLD_OP_RAISE:
		case WORLD_OP_LOWER:
		case WORLD_OP_SMOOTH:
		case WORLD_OP_CREATE_CRATER:
			return common | WORLD_OP_DEPENDS_NAV;

		//	A levelled pad is where a building goes, so nav must see it; roads are re-graded
		//	because a pad next to a road pulls the ground the road sits on.
		case WORLD_OP_FLATTEN:
		case WORLD_OP_GRADE_FOUNDATION:
			return common | WORLD_OP_DEPENDS_NAV | WORLD_OP_DEPENDS_ROADS;

		//	The road's own ground moved, and the surface mask that dresses it moved with it.
		case WORLD_OP_CARVE_ROAD:
			return common | WORLD_OP_DEPENDS_NAV | WORLD_OP_DEPENDS_ROADS | WORLD_OP_DEPENDS_MATERIALS;

		//	A channel is the water system's business by definition, and a road crossing the line
		//	it was cut along has just lost its ground.
		case WORLD_OP_CARVE_CHANNEL:
			return common | WORLD_OP_DEPENDS_NAV | WORLD_OP_DEPENDS_WATER | WORLD_OP_DEPENDS_ROADS;

		//	Ground raised out of shallow water: the water is unchanged, what changed is how deep
		//	it is over this ground, which is a water-system query.
		case WORLD_OP_RECLAIM_SHALLOW_WATER:
			return common | WORLD_OP_DEPENDS_NAV | WORLD_OP_DEPENDS_WATER;

		//	Restoring ground can undo any of the above, so it invalidates everything any of them
		//	could have.  Assuming less would leave whichever system the undone operation had
		//	touched holding a view of ground that no longer exists.
		case WORLD_OP_RESTORE_REGION:
			return common | WORLD_OP_DEPENDS_NAV | WORLD_OP_DEPENDS_WATER
								| WORLD_OP_DEPENDS_ROADS | WORLD_OP_DEPENDS_MATERIALS;

		default:
			return common;
	}
}


bool	World_Operation_Ordering_Matters(WorldOperationType type)
{
	switch (type) {
		case WORLD_OP_RAISE:
		case WORLD_OP_LOWER:
		case WORLD_OP_SMOOTH:
		case WORLD_OP_CREATE_CRATER:
			return true;			// relative to the ground they find
		default:
			return false;			// absolute targets
	}
}


WorldOperationClass::WorldOperationClass(void) :
	FormatVersion(CURRENT_FORMAT_VERSION),
	OperationId(0),
	BaseRevision(0),
	Type(WORLD_OP_RAISE),
	Team(-1),
	TimeSeconds(0.0f),
	Center(0.0f,0.0f,0.0f),
	Radius(0.0f),
	Depth(0.0f),
	Width(0.0f),
	Blend(0.0f)
{
}


bool WorldOperationClass::operator == (const WorldOperationClass & that) const
{
	if ((FormatVersion != that.FormatVersion) || (OperationId != that.OperationId) ||
		 (BaseRevision != that.BaseRevision) || (Type != that.Type) || (Team != that.Team) ||
		 (TimeSeconds != that.TimeSeconds) || (Center != that.Center) ||
		 (Radius != that.Radius) || (Depth != that.Depth) || (Width != that.Width) ||
		 (Blend != that.Blend)) {
		return false;
	}
	if (Path.Count() != that.Path.Count()) return false;
	for (int i = 0; i < Path.Count(); i++) {
		if (Path[i] != that.Path[i]) return false;
	}
	return true;
}


unsigned int WorldOperationClass::Compute_Checksum(void) const
{
	CRCEngine crc;

	crc(&FormatVersion,sizeof(FormatVersion));
	crc(&OperationId,sizeof(OperationId));
	crc(&BaseRevision,sizeof(BaseRevision));

	int type = (int)Type;
	crc(&type,sizeof(type));
	crc(&Team,sizeof(Team));

	int q_time = Quantize(TimeSeconds);		crc(&q_time,sizeof(q_time));
	int q_x = Quantize(Center.X);				crc(&q_x,sizeof(q_x));
	int q_y = Quantize(Center.Y);				crc(&q_y,sizeof(q_y));
	int q_z = Quantize(Center.Z);				crc(&q_z,sizeof(q_z));
	int q_radius = Quantize(Radius);			crc(&q_radius,sizeof(q_radius));
	int q_depth = Quantize(Depth);			crc(&q_depth,sizeof(q_depth));
	int q_width = Quantize(Width);			crc(&q_width,sizeof(q_width));
	int q_blend = Quantize(Blend);			crc(&q_blend,sizeof(q_blend));

	int point_count = Path.Count();
	crc(&point_count,sizeof(point_count));
	for (int i = 0; i < point_count; i++) {
		int px = Quantize(Path[i].X);	crc(&px,sizeof(px));
		int py = Quantize(Path[i].Y);	crc(&py,sizeof(py));
		int pz = Quantize(Path[i].Z);	crc(&pz,sizeof(pz));
	}

	return (unsigned int)crc;
}


WorldOperationResultType WorldOperationClass::Validate_Parameters(void) const
{
	if (FormatVersion != (unsigned int)CURRENT_FORMAT_VERSION) {
		return WORLD_OP_RESULT_BAD_FORMAT_VERSION;
	}
	int type_index = (int)Type;
	if ((type_index < 0) || (type_index >= WORLD_OPERATION_TYPE_COUNT)) {
		return WORLD_OP_RESULT_BAD_TYPE;
	}

	//	Every number that travels has to be a number.  A NaN radius would pass every comparison
	//	below and then reach the heightfield, where it would quietly poison whatever it touched.
	if (!WWMath::Is_Valid_Float(Radius) || !WWMath::Is_Valid_Float(Depth) ||
		 !WWMath::Is_Valid_Float(Width) || !WWMath::Is_Valid_Float(Blend) ||
		 !WWMath::Is_Valid_Float(TimeSeconds) ||
		 !WWMath::Is_Valid_Float(Center.X) || !WWMath::Is_Valid_Float(Center.Y) ||
		 !WWMath::Is_Valid_Float(Center.Z)) {
		return WORLD_OP_RESULT_BAD_PARAMETERS;
	}

	if (Path.Count() > MAX_PATH_POINTS) {
		return WORLD_OP_RESULT_BAD_PARAMETERS;
	}
	for (int i = 0; i < Path.Count(); i++) {
		if (!WWMath::Is_Valid_Float(Path[i].X) || !WWMath::Is_Valid_Float(Path[i].Y) ||
			 !WWMath::Is_Valid_Float(Path[i].Z)) {
			return WORLD_OP_RESULT_BAD_PARAMETERS;
		}
	}

	switch (Type) {

		case WORLD_OP_RAISE:
			if (!(Radius > 0.0f))	return WORLD_OP_RESULT_BAD_PARAMETERS;
			if (!(Depth > 0.0f))		return WORLD_OP_RESULT_BAD_PARAMETERS;	// raising by nothing, or by a negative
			break;

		case WORLD_OP_LOWER:
		case WORLD_OP_CREATE_CRATER:
			if (!(Radius > 0.0f))	return WORLD_OP_RESULT_BAD_PARAMETERS;
			if (!(Depth > 0.0f))		return WORLD_OP_RESULT_BAD_PARAMETERS;	// depth is stated positive; the sign is the type's
			break;

		case WORLD_OP_FLATTEN:
			if (!(Radius > 0.0f))						return WORLD_OP_RESULT_BAD_PARAMETERS;
			if ((Blend <= 0.0f) || (Blend > 1.0f))	return WORLD_OP_RESULT_BAD_PARAMETERS;
			break;

		case WORLD_OP_SMOOTH:
			if (!(Radius > 0.0f))						return WORLD_OP_RESULT_BAD_PARAMETERS;
			if ((Blend <= 0.0f) || (Blend > 1.0f))	return WORLD_OP_RESULT_BAD_PARAMETERS;
			break;

		case WORLD_OP_GRADE_FOUNDATION:
			//	Blend carries the apron radius here, and an apron inside the pad is not an apron.
			if (!(Radius > 0.0f))		return WORLD_OP_RESULT_BAD_PARAMETERS;
			if (!(Blend >= Radius))		return WORLD_OP_RESULT_BAD_PARAMETERS;
			break;

		case WORLD_OP_CARVE_ROAD:
			if (Path.Count() < 2)	return WORLD_OP_RESULT_BAD_PARAMETERS;
			if (!(Width > 0.0f))		return WORLD_OP_RESULT_BAD_PARAMETERS;
			if ((Blend <= 0.0f) || (Blend > 1.0f))	return WORLD_OP_RESULT_BAD_PARAMETERS;
			break;

		case WORLD_OP_CARVE_CHANNEL:
			if (Path.Count() < 2)	return WORLD_OP_RESULT_BAD_PARAMETERS;
			if (!(Width > 0.0f))		return WORLD_OP_RESULT_BAD_PARAMETERS;
			if (!(Depth > 0.0f))		return WORLD_OP_RESULT_BAD_PARAMETERS;
			break;

		case WORLD_OP_RECLAIM_SHALLOW_WATER:
		case WORLD_OP_RESTORE_REGION:
			if (!(Radius > 0.0f))	return WORLD_OP_RESULT_BAD_PARAMETERS;
			break;

		default:
			return WORLD_OP_RESULT_BAD_TYPE;
	}

	return WORLD_OP_RESULT_APPLIED;
}


bool WorldOperationClass::Get_Affected_Bounds(Vector3 * center_out,float * radius_out) const
{
	if ((Type == WORLD_OP_CARVE_ROAD) || (Type == WORLD_OP_CARVE_CHANNEL)) {

		if (Path.Count() < 1) return false;

		//	The polyline's own bounding circle, widened by half the strip.  A circle rather than a
		//	box because everything downstream -- chunk reservation, the protected-region sweep --
		//	asks in terms of a centre and a radius, and one shape that is never optimistic beats
		//	two that have to agree.
		Vector3 lo = Path[0];
		Vector3 hi = Path[0];
		for (int i = 1; i < Path.Count(); i++) {
			lo.X = WWMath::Min(lo.X,Path[i].X);
			lo.Y = WWMath::Min(lo.Y,Path[i].Y);
			lo.Z = WWMath::Min(lo.Z,Path[i].Z);
			hi.X = WWMath::Max(hi.X,Path[i].X);
			hi.Y = WWMath::Max(hi.Y,Path[i].Y);
			hi.Z = WWMath::Max(hi.Z,Path[i].Z);
		}

		Vector3 center = (lo + hi) * 0.5f;
		float half_x = (hi.X - lo.X) * 0.5f;
		float half_y = (hi.Y - lo.Y) * 0.5f;
		float radius = WWMath::Sqrt((half_x * half_x) + (half_y * half_y)) + (Width * 0.5f);

		if (center_out != nullptr) *center_out = center;
		if (radius_out != nullptr) *radius_out = radius;
		return true;
	}

	//	GRADE_FOUNDATION reaches to its apron, not its pad.
	float radius = (Type == WORLD_OP_GRADE_FOUNDATION) ? WWMath::Max(Radius,Blend) : Radius;
	if (!(radius > 0.0f)) return false;

	if (center_out != nullptr) *center_out = Center;
	if (radius_out != nullptr) *radius_out = radius;
	return true;
}


/***********************************************************************************************
**	Step 14's compact form.
**
**	Written field by field rather than by copying the struct: this is a wire format, and a
**	memcpy of a class would tie it to one compiler's padding and one machine's idea of an enum.
*/

//	32-bit fields, in declaration order, then the path.  Fourteen: the eleven scalars, the team,
//	the format version's companions, and the point count that ends the header.
enum { WORLD_OPERATION_HEADER_FIELDS = 14 };

static void Write_UInt(unsigned char * at,unsigned int value)
{
	at[0] = (unsigned char)( value        & 0xFF);
	at[1] = (unsigned char)((value >>  8) & 0xFF);
	at[2] = (unsigned char)((value >> 16) & 0xFF);
	at[3] = (unsigned char)((value >> 24) & 0xFF);
}

static unsigned int Read_UInt(const unsigned char * at)
{
	return	((unsigned int)at[0])			|
				(((unsigned int)at[1]) <<  8)	|
				(((unsigned int)at[2]) << 16)	|
				(((unsigned int)at[3]) << 24);
}

static void Write_Float(unsigned char * at,float value)
{
	unsigned int bits = 0;
	::memcpy(&bits,&value,sizeof(bits));
	Write_UInt(at,bits);
}

static float Read_Float(const unsigned char * at)
{
	unsigned int bits = Read_UInt(at);
	float value = 0.0f;
	::memcpy(&value,&bits,sizeof(value));
	return value;
}


int WorldOperationClass::Get_Block_Size(void) const
{
	return (WORLD_OPERATION_HEADER_FIELDS * 4) + (Path.Count() * 3 * 4);
}


bool WorldOperationClass::Write_Block(unsigned char * buffer,int max_bytes,int * used_out) const
{
	int needed = Get_Block_Size();
	if ((buffer == nullptr) || (max_bytes < needed)) return false;

	unsigned char * at = buffer;

	Write_UInt (at,FormatVersion);				at += 4;
	Write_UInt (at,OperationId);					at += 4;
	Write_UInt (at,BaseRevision);					at += 4;
	Write_UInt (at,(unsigned int)Type);			at += 4;
	Write_UInt (at,(unsigned int)Team);			at += 4;
	Write_Float(at,TimeSeconds);					at += 4;
	Write_Float(at,Center.X);						at += 4;
	Write_Float(at,Center.Y);						at += 4;
	Write_Float(at,Center.Z);						at += 4;
	Write_Float(at,Radius);							at += 4;
	Write_Float(at,Depth);							at += 4;
	Write_Float(at,Width);							at += 4;
	Write_Float(at,Blend);							at += 4;
	Write_UInt (at,(unsigned int)Path.Count());	at += 4;

	for (int i = 0; i < Path.Count(); i++) {
		Write_Float(at,Path[i].X);	at += 4;
		Write_Float(at,Path[i].Y);	at += 4;
		Write_Float(at,Path[i].Z);	at += 4;
	}

	if (used_out != nullptr) *used_out = (int)(at - buffer);
	return true;
}


bool WorldOperationClass::Read_Block(const unsigned char * buffer,int bytes,int * consumed_out)
{
	if ((buffer == nullptr) || (bytes < (WORLD_OPERATION_HEADER_FIELDS * 4))) return false;

	const unsigned char * at = buffer;

	FormatVersion	= Read_UInt (at);	at += 4;
	OperationId		= Read_UInt (at);	at += 4;
	BaseRevision	= Read_UInt (at);	at += 4;

	unsigned int type = Read_UInt(at);	at += 4;
	if (type >= (unsigned int)WORLD_OPERATION_TYPE_COUNT) return false;
	Type = (WorldOperationType)type;

	Team				= (int)Read_UInt(at);	at += 4;
	TimeSeconds		= Read_Float(at);			at += 4;
	Center.X			= Read_Float(at);			at += 4;
	Center.Y			= Read_Float(at);			at += 4;
	Center.Z			= Read_Float(at);			at += 4;
	Radius			= Read_Float(at);			at += 4;
	Depth				= Read_Float(at);			at += 4;
	Width				= Read_Float(at);			at += 4;
	Blend				= Read_Float(at);			at += 4;

	unsigned int point_count = Read_UInt(at);	at += 4;
	if (point_count > (unsigned int)MAX_PATH_POINTS) return false;

	int remaining = bytes - (int)(at - buffer);
	if (remaining < (int)(point_count * 3 * 4)) return false;

	Path.Delete_All();
	for (unsigned int i = 0; i < point_count; i++) {
		float x = Read_Float(at);	at += 4;
		float y = Read_Float(at);	at += 4;
		float z = Read_Float(at);	at += 4;
		Path.Add(Vector3(x,y,z));
	}

	if (consumed_out != nullptr) *consumed_out = (int)(at - buffer);
	return true;
}
