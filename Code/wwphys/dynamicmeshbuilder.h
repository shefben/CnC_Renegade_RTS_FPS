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
	//	One triangle, as a fan of three.  Merging somebody else's mesh means walking a triangle
	//	list, and a fan of three vertices is exactly one triangle with nothing shared.
	void	Triangle(const Vector3 & p0,const Vector3 & p1,const Vector3 & p2,
						  float u0,float v0,float u1,float v1,float u2,float v2)
	{
		Begin_Fan();
		Vertex(p0,u0,v0);
		Vertex(p1,u1,v1);
		Vertex(p2,u2,v2);
	}

	//	An axis-aligned box.  X and Y are half extents about the offset; Z runs from bottom to
	//	top, which is asymmetric because the things built out of boxes here -- a bridge deck, a
	//	tree's collision post -- hang below or stand above a surface rather than straddling it.
	void	Box(float half_x,float half_y,float top,float bottom,
					 const Vector3 & offset = Vector3(0.0f,0.0f,0.0f))
	{
		const float x0 = offset.X - half_x, x1 = offset.X + half_x;
		const float y0 = offset.Y - half_y, y1 = offset.Y + half_y;
		const float z0 = offset.Z + bottom, z1 = offset.Z + top;

		struct QuadDesc { Vector3 a,b,c,d; };

		QuadDesc quads[6] = {
			//	top
			{ Vector3(x0,y0,z1), Vector3(x1,y0,z1), Vector3(x0,y1,z1), Vector3(x1,y1,z1) },
			//	bottom
			{ Vector3(x0,y1,z0), Vector3(x1,y1,z0), Vector3(x0,y0,z0), Vector3(x1,y0,z0) },
			//	left
			{ Vector3(x0,y1,z1), Vector3(x1,y1,z1), Vector3(x0,y1,z0), Vector3(x1,y1,z0) },
			//	right
			{ Vector3(x0,y0,z0), Vector3(x1,y0,z0), Vector3(x0,y0,z1), Vector3(x1,y0,z1) },
			//	start
			{ Vector3(x0,y0,z1), Vector3(x0,y1,z1), Vector3(x0,y0,z0), Vector3(x0,y1,z0) },
			//	end
			{ Vector3(x1,y1,z1), Vector3(x1,y0,z1), Vector3(x1,y1,z0), Vector3(x1,y0,z0) },
		};

		for (int q = 0; q < 6; q++) {
			Begin_Strip();
			Vertex(quads[q].a,0.0f,0.0f);
			Vertex(quads[q].b,1.0f,0.0f);
			Vertex(quads[q].c,0.0f,1.0f);
			Vertex(quads[q].d,1.0f,1.0f);
		}
	}

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
