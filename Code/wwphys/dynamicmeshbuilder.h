/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/dynamicmeshbuilder.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Counting the geometry and then writing it, from the same code.                              *
 *                                                                                             *
 *	A DynamicMeshClass is sized when it is created, so the triangles have to be counted before  *
 *	any of them can be written.  Rather than count in one place and emit in another -- where    *
 *	the two drift apart and the mesh overruns -- the emitter runs twice against this, once      *
 *	counting and once building.                                                                 *
 *                                                                                             *
 *	This began as the road system's private helper.  The bridge system generates its geometry   *
 *	the same way and for the same reason, so it lives here rather than being written twice.     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef DYNAMICMESHBUILDER_H
#define DYNAMICMESHBUILDER_H

#include "always.h"
#include "dynamesh.h"
#include "refcount.h"
#include "vector3.h"

class DynamicMeshBuilderClass
{
public:

	DynamicMeshBuilderClass(void) :
		Mesh(nullptr), Counting(true), PolyCount(0), VertCount(0), PrimitiveVerts(0) { }

	~DynamicMeshBuilderClass(void)
	{
		REF_PTR_RELEASE(Mesh);
	}

	void	Begin_Count(void)
	{
		REF_PTR_RELEASE(Mesh);
		Counting = true;
		PolyCount = 0;
		VertCount = 0;
		PrimitiveVerts = 0;
	}

	int	Get_Poly_Count(void) const		{ return PolyCount; }
	int	Get_Vert_Count(void) const		{ return VertCount; }

	bool	Begin_Build(void)
	{
		if ((PolyCount <= 0) || (VertCount <= 0)) {
			return false;
		}
		REF_PTR_RELEASE(Mesh);
		Mesh = NEW_REF(DynamicMeshClass,(PolyCount,VertCount));
		Counting = false;
		PrimitiveVerts = 0;
		return (Mesh != nullptr);
	}

	void	Begin_Strip(void)
	{
		PrimitiveVerts = 0;
		if (!Counting && (Mesh != nullptr)) { Mesh->Begin_Tri_Strip(); }
	}

	void	Begin_Fan(void)
	{
		PrimitiveVerts = 0;
		if (!Counting && (Mesh != nullptr)) { Mesh->Begin_Tri_Fan(); }
	}

	void	Vertex(const Vector3 & position,float u,float v)
	{
		if (Counting) {
			VertCount++;
			if (PrimitiveVerts >= 2) { PolyCount++; }
		} else if (Mesh != nullptr) {
			Mesh->Vertex(position.X,position.Y,position.Z,u,v);
		}
		PrimitiveVerts++;
	}

	//	Hands the mesh over with its reference; the builder no longer has one.
	DynamicMeshClass *	Detach_Mesh(void)
	{
		DynamicMeshClass * mesh = Mesh;
		Mesh = nullptr;
		return mesh;
	}

private:

	DynamicMeshBuilderClass(const DynamicMeshBuilderClass &);
	DynamicMeshBuilderClass & operator = (const DynamicMeshBuilderClass &);

	DynamicMeshClass *	Mesh;
	bool						Counting;
	int						PolyCount;
	int						VertCount;
	int						PrimitiveVerts;
};

#endif // DYNAMICMESHBUILDER_H
