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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/particlebatchtype.cpp                 $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "particlebatchtype.h"


const char * Particle_Batch_Kind_Name(int kind)
{
	switch (kind)
	{
		case PARTICLE_BATCH_SPRITE:	return "SPRITE";
		case PARTICLE_BATCH_POINT:		return "POINT";
		case PARTICLE_BATCH_STREAK:	return "STREAK";
		default:								return "UNKNOWN";
	}
}


const char * Particle_Budget_Class_Name(int budget_class)
{
	switch (budget_class)
	{
		case PARTICLE_BUDGET_CRITICAL_GAMEPLAY:	return "CRITICAL_GAMEPLAY";
		case PARTICLE_BUDGET_COMBAT_NEAR:			return "COMBAT_NEAR";
		case PARTICLE_BUDGET_COMBAT_FAR:				return "COMBAT_FAR";
		case PARTICLE_BUDGET_ENVIRONMENT:			return "ENVIRONMENT";
		case PARTICLE_BUDGET_AMBIENT:					return "AMBIENT";
		default:												return "UNKNOWN";
	}
}


/***********************************************************************************************
 *	Particle_Budget_Class_Default_Reserve -- the share a class can always have                   *
 *                                                                                             *
 *	These add up to three quarters of the pool, deliberately.  The missing quarter is the        *
 *	headroom: it belongs to nobody, anything may use it, and a class that is inside its own      *
 *	reservation can never be evicted to make room for anything.  Reservations that added up to   *
 *	the whole pool would be a system with no give in it at all -- every class permanently at its *
 *	promise and the first unexpected explosion refused.                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int Particle_Budget_Class_Default_Reserve(int budget_class)
{
	switch (budget_class)
	{
		case PARTICLE_BUDGET_CRITICAL_GAMEPLAY:	return 512;
		case PARTICLE_BUDGET_COMBAT_NEAR:			return 1024;
		case PARTICLE_BUDGET_COMBAT_FAR:				return 768;
		case PARTICLE_BUDGET_ENVIRONMENT:			return 512;
		case PARTICLE_BUDGET_AMBIENT:					return 256;
		default:												return 0;
	}
}


/***********************************************************************************************
 *	Particle_Budget_Class_Default_Cap -- the most a class may ever hold                          *
 *                                                                                             *
 *	Critical gameplay may have the whole pool, because the alternative to drawing the plume of  *
 *	a beacon is a player who cannot see the thing that is about to kill them.  Everything else  *
 *	is capped below the pool so that one over-enthusiastic effect cannot starve the classes      *
 *	beneath it even when they happen to be empty at the time.                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int Particle_Budget_Class_Default_Cap(int budget_class)
{
	switch (budget_class)
	{
		case PARTICLE_BUDGET_CRITICAL_GAMEPLAY:	return PARTICLE_BATCH_POOL_SIZE;
		case PARTICLE_BUDGET_COMBAT_NEAR:			return 3072;
		case PARTICLE_BUDGET_COMBAT_FAR:				return 2048;
		case PARTICLE_BUDGET_ENVIRONMENT:			return 1536;
		case PARTICLE_BUDGET_AMBIENT:					return 768;
		default:												return 0;
	}
}


int Particle_Batch_Kind_Points_Per_Submission(int kind)
{
	switch (kind)
	{
		case PARTICLE_BATCH_SPRITE:	return PARTICLE_BATCH_POINTS_PER_SUBMISSION_QUAD;
		case PARTICLE_BATCH_POINT:		return PARTICLE_BATCH_POINTS_PER_SUBMISSION_TRI;

		//	A line group fills one index buffer and one vertex buffer for however many lines it
		//	was given and draws them all at once, so a streak group is one submission whatever
		//	is in it.
		case PARTICLE_BATCH_STREAK:	return PARTICLE_BATCH_MAX_PER_GROUP;

		default:								return PARTICLE_BATCH_POINTS_PER_SUBMISSION_QUAD;
	}
}


ParticleBatchDefinitionClass::ParticleBatchDefinitionClass(void) :
	Name(""),
	Texture(""),
	Kind(PARTICLE_BATCH_SPRITE),
	BudgetClass(PARTICLE_BUDGET_AMBIENT),
	Lifetime(1.5f),
	StartSize(0.35f),
	EndSize(0.75f),
	StartColor(1.0f,1.0f,1.0f),
	EndColor(1.0f,1.0f,1.0f),
	StartAlpha(1.0f),
	EndAlpha(0.0f),
	Gravity(0.0f),
	Drag(0.0f),
	SpinRate(0.0f),
	StreakSeconds(0.05f),
	Additive(true),
	FrameGridLog2(0),
	MaxParticles(PARTICLE_BATCH_MAX_PER_GROUP)
{
	//	The default budget class is the least important one on purpose.  An effect that never
	//	said what it was worth is exactly the effect that should lose its particles first.
}


bool ParticleBatchDefinitionClass::operator == (const ParticleBatchDefinitionClass & that) const
{
	return	(Name == that.Name) &&
				(Texture == that.Texture) &&
				(Kind == that.Kind) &&
				(BudgetClass == that.BudgetClass) &&
				(Lifetime == that.Lifetime) &&
				(StartSize == that.StartSize) &&
				(EndSize == that.EndSize) &&
				(StartColor == that.StartColor) &&
				(EndColor == that.EndColor) &&
				(StartAlpha == that.StartAlpha) &&
				(EndAlpha == that.EndAlpha) &&
				(Gravity == that.Gravity) &&
				(Drag == that.Drag) &&
				(SpinRate == that.SpinRate) &&
				(StreakSeconds == that.StreakSeconds) &&
				(Additive == that.Additive) &&
				(FrameGridLog2 == that.FrameGridLog2) &&
				(MaxParticles == that.MaxParticles);
}


void ParticleBatchDefinitionClass::Set_Kind(int kind)
{
	if ((kind < 0) || (kind >= PARTICLE_BATCH_KIND_COUNT)) {
		kind = PARTICLE_BATCH_SPRITE;
	}
	Kind = kind;
	return ;
}


void ParticleBatchDefinitionClass::Set_Budget_Class(int budget_class)
{
	if ((budget_class < 0) || (budget_class >= PARTICLE_BUDGET_CLASS_COUNT)) {
		budget_class = PARTICLE_BUDGET_AMBIENT;
	}
	BudgetClass = budget_class;
	return ;
}


void ParticleBatchDefinitionClass::Set_Lifetime(float seconds)
{
	//	A particle with no lifetime would never free its slot, which is the one way a fixed pool
	//	can still be exhausted for good.  Clamped rather than asserted: the number arrives from
	//	content and from the console.
	if (seconds < 0.05f) { seconds = 0.05f; }
	if (seconds > 120.0f) { seconds = 120.0f; }
	Lifetime = seconds;
	return ;
}


void ParticleBatchDefinitionClass::Set_Start_Alpha(float alpha)
{
	if (alpha < 0.0f) { alpha = 0.0f; }
	if (alpha > 1.0f) { alpha = 1.0f; }
	StartAlpha = alpha;
	return ;
}


void ParticleBatchDefinitionClass::Set_End_Alpha(float alpha)
{
	if (alpha < 0.0f) { alpha = 0.0f; }
	if (alpha > 1.0f) { alpha = 1.0f; }
	EndAlpha = alpha;
	return ;
}


void ParticleBatchDefinitionClass::Set_Drag(float per_second)
{
	//	Drag is applied as a fraction of the velocity removed each second, so one is a particle
	//	that stops dead within a second and anything above it would reverse the velocity.
	if (per_second < 0.0f) { per_second = 0.0f; }
	if (per_second > 1.0f) { per_second = 1.0f; }
	Drag = per_second;
	return ;
}


void ParticleBatchDefinitionClass::Set_Frame_Grid_Log2(uint8 log2)
{
	//	PointGroupClass precalculates UVs for 1x1 up to 16x16 frames and asserts past that.
	if (log2 > 4) { log2 = 4; }
	FrameGridLog2 = log2;
	return ;
}


void ParticleBatchDefinitionClass::Set_Max_Particles(int count)
{
	if (count < 1) { count = 1; }
	if (count > PARTICLE_BATCH_MAX_PER_GROUP) { count = PARTICLE_BATCH_MAX_PER_GROUP; }
	MaxParticles = count;
	return ;
}


void BatchParticleClass::Reset(void)
{
	InUse			= false;
	Definition	= -1;
	BudgetClass	= PARTICLE_BUDGET_AMBIENT;
	Position.Set(0.0f,0.0f,0.0f);
	Velocity.Set(0.0f,0.0f,0.0f);
	Tint.Set(1.0f,1.0f,1.0f);
	SizeScale	= 1.0f;
	Color.Set(1.0f,1.0f,1.0f);
	Alpha			= 1.0f;
	Size			= 0.0f;
	Angle			= 0.0f;
	Age			= 0.0f;
	Lifetime		= 0.0f;
	ListNext		= -1;
	ListPrev		= -1;
	return ;
}
