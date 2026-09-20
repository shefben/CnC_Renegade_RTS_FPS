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
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : wwphys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/lightsolve.cpp                        $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 3/28/02 11:53a                                              $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "lightsolve.h"
#include "phys.h"
#include "staticphys.h"
#include "rendobj.h"
#include "mesh.h"
#include "dx8renderer.h"
#include "simplevec.h"
#include "vp.h"
#include "lightphys.h"
#include "light.h"
#include "lightsolvecontext.h"
#include "lightsolveprogress.h"
#include "renegadeterrainpatch.h"


/**
** VertexSolveClass
** This class does the job of generated a vertex solve for a mesh.
*/
class VertexSolveClass
{
public:
	VertexSolveClass(void) {}

	void Light_Mesh(LightSolveContextClass & context,MeshClass * mesh, NonRefPhysListClass & lights);
	void Light_Terrain(LightSolveContextClass & context,RenegadeTerrainPatchClass * patch, NonRefPhysListClass & lights);

protected:

	void Grow_Arrays(int new_vcount);
	void Add_Light_To_Vertex(LightSolveContextClass & context,int idx,LightClass * light_obj);
	float Compute_Sun_Visibility(LightSolveContextClass & context,int idx,LightClass * sun);
	float Compute_Sky_Occlusion(LightSolveContextClass & context,int idx);

	SimpleVecClass<Vector3>		Position;
	SimpleVecClass<Vector3>		Normal;
	SimpleVecClass<Vector4>		MeshAmbient;
	SimpleVecClass<Vector4>		MeshDiffuse;

	SimpleVecClass<Vector4>		AmbientSolve;
	SimpleVecClass<Vector4>		DiffuseSolve;
	SimpleVecClass<Vector4>		Solve;

	/*
	**	Roadmap Section 52A, Phase 49.  SunAmbientContrib/SunDiffuseContrib are a snapshot of
	**	exactly what the sun alone added into AmbientSolve/DiffuseSolve, taken right after
	**	Add_Light_To_Vertex adds it and before any other light does -- that is what lets
	**	BaseSolve be produced by subtraction instead of a second full accumulation pass.
	**	SunVis is the geometric-only companion: how visible the sun would be here regardless of
	**	its actual current colour.
	*/
	SimpleVecClass<Vector4>		SunAmbientContrib;
	SimpleVecClass<Vector4>		SunDiffuseContrib;
	SimpleVecClass<Vector4>		BaseSolve;
	SimpleVecClass<unsigned char>	SunVis;

};

static VertexSolveClass TheVertexSolver;

void VertexSolveClass::Light_Mesh(LightSolveContextClass & context,MeshClass * mesh, NonRefPhysListClass & lights)
{
	int vi;
	context.Get_Progress().Set_Current_Mesh_Name(mesh->Get_Name());
	context.Get_Progress().Set_Current_Mesh_Vertex_Count(mesh->Peek_Model()->Get_Vertex_Count());

	/*
	** Get the model and initialize our arrays
	*/
	MeshModelClass * model = mesh->Peek_Model();
	int vcount = model->Get_Vertex_Count();
	Grow_Arrays(vcount);

	/*
	** Transform the positions and normals into world space
	*/
	Matrix3D tm = mesh->Get_Transform();
	const Vector3 * src_verts = model->Get_Vertex_Array();
	const Vector3 * src_norms = model->Get_Vertex_Normal_Array();

	VectorProcessorClass::Transform(&(Position[0]), src_verts, tm, vcount);
	tm.Set_Translation(Vector3(0,0,0));
	VectorProcessorClass::Transform(&(Normal[0]), src_norms, tm, vcount);

	/*
	** Fill the mesh ambient and diffuse arrays with the per vertex material
	** color (either from the material itself or the vertex color array)
	**
	** LIGHT SOLVE GENERATION RULES:
	**	- for each vertex
	**		- for ambient and diffuse
	**			- if this vertex is supposed to use a color array in any pass copy the color from the array
	**			- else copy the material setting from pass0
	**
	** LIGHT SOLVE INSTALL RULES:
	** - for each pass
	**		- for each vertex material
	**			- if this material has any emissive component, leave the vmtl alone
	**			- else, point the emissive source to the new light solve color array,
	**			  make the diffuse and ambient use the material settings.
	*/
	unsigned * dcg = nullptr;
	for (int pi=0; pi<model->Get_Pass_Count(); pi++) {
		if (model->Get_DCG_Array(pi) != nullptr) {
			dcg = model->Get_DCG_Array(pi);
		}
	}

	for (vi=0; vi<model->Get_Vertex_Count(); vi++) {

		bool use_array = false;
		for (int pi=0; pi<model->Get_Pass_Count(); pi++) {
			VertexMaterialClass * pass_mtl = model->Peek_Material(vi,pi);
			if (	(pass_mtl != nullptr) &&
					(dcg != nullptr) &&
					(pass_mtl->Get_Diffuse_Color_Source() == VertexMaterialClass::COLOR1) )
			{
				use_array = true;
			}
		}

		/*
		** Set up the ambient color for this vertex
		*/
		VertexMaterialClass * vmtl = model->Peek_Material(vi,0);
		if (use_array) {
			MeshAmbient[vi] = DX8Wrapper::Convert_Color(dcg[vi]);
		} else if (vmtl != nullptr) {
			Vector3 ambient;
			vmtl->Get_Ambient(&ambient);
			MeshAmbient[vi] = Vector4(ambient.X,ambient.Y,ambient.Z,1.0f);
		} else {
			MeshAmbient[vi].Set(1,1,1,1);
		}

		/*
		** Set up the diffuse color for this vertex
		*/
		if (use_array) {
			MeshDiffuse[vi] = DX8Wrapper::Convert_Color(dcg[vi]);
		} else if (vmtl != nullptr) {
			Vector3 diffuse;
			vmtl->Get_Diffuse(&diffuse);
			MeshDiffuse[vi] = Vector4(diffuse.X,diffuse.Y,diffuse.Z,1.0f);
		} else {
			MeshDiffuse[vi].Set(1,1,1,1);
		}
	}

	/*
	** Compute the light solve
	*/
	LightClass * sun = PhysicsSceneClass::Get_Instance()->Get_Sun_Light();
	Vector3 scene_ambient = PhysicsSceneClass::Get_Instance()->Get_Ambient_Light();
	int callback_counter = 0;

	for (vi=0; vi<model->Get_Vertex_Count(); vi++) {

		AmbientSolve[vi].X = scene_ambient.X;
		AmbientSolve[vi].Y = scene_ambient.Y;
		AmbientSolve[vi].Z = scene_ambient.Z;
		AmbientSolve[vi].W = 1.0f;

		DiffuseSolve[vi].X = 0.0f;
		DiffuseSolve[vi].Y = 0.0f;
		DiffuseSolve[vi].Z = 0.0f;
		DiffuseSolve[vi].W = MeshDiffuse[vi].W;

		/*
		** Sun
		*/
		Add_Light_To_Vertex(context,vi,sun);

		/*
		**	Phase 49: exactly what the sun alone just added, before any other light gets a
		**	chance to.  AmbientSolve/DiffuseSolve started this iteration at (scene_ambient,0),
		**	so the delta from those starting values is the sun's contribution alone.
		*/
		SunAmbientContrib[vi] = Vector4(AmbientSolve[vi].X - scene_ambient.X,
													AmbientSolve[vi].Y - scene_ambient.Y,
													AmbientSolve[vi].Z - scene_ambient.Z, 0.0f);
		SunDiffuseContrib[vi] = Vector4(DiffuseSolve[vi].X,DiffuseSolve[vi].Y,DiffuseSolve[vi].Z,0.0f);
		SunVis[vi] = (unsigned char) (WWMath::Clamp(Compute_Sun_Visibility(context,vi,sun),0.0f,1.0f) * 255.0f + 0.5f);

		/*
		** Other lights
		*/
		NonRefPhysListIterator it(&lights);
		while (!it.Is_Done()) {

			LightPhysClass * light = it.Peek_Obj()->As_LightPhysClass();
			if ((light) && (light->Peek_Model()) && (light->Peek_Model()->Class_ID() == RenderObjClass::CLASSID_LIGHT)) {

				LightClass * light_obj = (LightClass*)light->Peek_Model();

				Add_Light_To_Vertex(context,vi,light_obj);
			}
			it.Next();
		}

		context.Get_Progress().Set_Current_Vertex(vi);

		callback_counter++;
		if (callback_counter > 100) {
			callback_counter = 0;
			context.Update_Observer();
		}
	}

	VectorProcessorClass::Clamp(&(AmbientSolve[0]),&(AmbientSolve[0]), 0.0f, 1.0f,vcount);
	VectorProcessorClass::Clamp(&(DiffuseSolve[0]),&(DiffuseSolve[0]), 0.0f, 1.0f,vcount);

	/*
	** Modulate the accumulated light by the material properties
	*/
	for (vi=0; vi<vcount; vi++) {
		Solve[vi].X = AmbientSolve[vi].X * MeshAmbient[vi].X + DiffuseSolve[vi].X * MeshDiffuse[vi].X;
		Solve[vi].Y = AmbientSolve[vi].Y * MeshAmbient[vi].Y + DiffuseSolve[vi].Y * MeshDiffuse[vi].Y;
		Solve[vi].Z = AmbientSolve[vi].Z * MeshAmbient[vi].Z + DiffuseSolve[vi].Z * MeshDiffuse[vi].Z;
		Solve[vi].W = DiffuseSolve[vi].W;

		/*
		**	Phase 49: the same vertex again, minus what the sun alone contributed -- an
		**	approximation where AmbientSolve/DiffuseSolve saturated pre-clamp (multiple stacked
		**	lights over 1.0), since the subtraction happens before Solve's own clamp above,
		**	not after it.  Acceptable for a first cut of a cosmetic-only relighting term.
		*/
		BaseSolve[vi].X = (AmbientSolve[vi].X - SunAmbientContrib[vi].X) * MeshAmbient[vi].X +
								 (DiffuseSolve[vi].X - SunDiffuseContrib[vi].X) * MeshDiffuse[vi].X;
		BaseSolve[vi].Y = (AmbientSolve[vi].Y - SunAmbientContrib[vi].Y) * MeshAmbient[vi].Y +
								 (DiffuseSolve[vi].Y - SunDiffuseContrib[vi].Y) * MeshDiffuse[vi].Y;
		BaseSolve[vi].Z = (AmbientSolve[vi].Z - SunAmbientContrib[vi].Z) * MeshAmbient[vi].Z +
								 (DiffuseSolve[vi].Z - SunDiffuseContrib[vi].Z) * MeshDiffuse[vi].Z;
		BaseSolve[vi].W = Solve[vi].W;
	}
	VectorProcessorClass::Clamp(&(Solve[0]),&(Solve[0]), 0.0f, 1.0f,vcount);
	VectorProcessorClass::Clamp(&(BaseSolve[0]),&(BaseSolve[0]), 0.0f, 1.0f,vcount);

	mesh->Install_User_Lighting_Array(&(Solve[0]));

	/*
	**	Phase 49: hand off the second bake too.  Installing it changes nothing by itself -- see
	**	MeshClass::Install_Sun_Relighting_Data -- so every stock mesh getting this data at load
	**	time is not a visual change until something explicitly relights with it.
	*/
	{
		static SimpleVecClass<unsigned int> _PackedBaseSolve;
		_PackedBaseSolve.Uninitialised_Grow(vcount);
		for (vi=0; vi<vcount; vi++) {
			_PackedBaseSolve[vi] = DX8Wrapper::Convert_Color(BaseSolve[vi]);
		}
		mesh->Install_Sun_Relighting_Data(&(_PackedBaseSolve[0]),&(SunVis[0]));
	}

	context.Update_Observer();
	REF_PTR_RELEASE(sun);
}

static Vector3 _offset (0, 0, 0);

void VertexSolveClass::Light_Terrain(LightSolveContextClass & context,RenegadeTerrainPatchClass * patch, NonRefPhysListClass & lights)
{
	int vi;
	context.Get_Progress().Set_Current_Mesh_Name(patch->Get_Name());
	context.Get_Progress().Set_Current_Mesh_Vertex_Count(patch->Get_Vertex_Count());

	/*
	** Get the model and initialize our arrays
	*/
	int vcount = patch->Get_Vertex_Count();
	Grow_Arrays(vcount);

	/*
	** Transform the positions and normals into world space
	*/
	Matrix3D tm = patch->Get_Transform();
	const Vector3 * src_verts = patch->Get_Vertex_Array();
	const Vector3 * src_norms = patch->Get_Vertex_Normal_Array();

	VectorProcessorClass::Transform(&(Position[0]), src_verts, tm, vcount);
	tm.Set_Translation(Vector3(0,0,0));
	VectorProcessorClass::Transform(&(Normal[0]), src_norms, tm, vcount);

	/*
	** For heightfield terrains, the mesh ambient and diffuse colors are white
	*/
	for (int index = 0; index < vcount; index ++) {
		MeshAmbient[index].Set (1.0F, 1.0F, 1.0F, 1.0F);
		MeshDiffuse[index].Set (1.0F, 1.0F, 1.0F, 1.0F);
	}

	/*
	** Compute the light solve
	*/
	LightClass * sun = PhysicsSceneClass::Get_Instance()->Get_Sun_Light();
	Vector3 scene_ambient = PhysicsSceneClass::Get_Instance()->Get_Ambient_Light();
	int callback_counter = 0;

	for (vi = 0; vi < vcount; vi ++) {

		AmbientSolve[vi].X = scene_ambient.X;
		AmbientSolve[vi].Y = scene_ambient.Y;
		AmbientSolve[vi].Z = scene_ambient.Z;
		AmbientSolve[vi].W = 1.0f;

		DiffuseSolve[vi].X = 0.0f;
		DiffuseSolve[vi].Y = 0.0f;
		DiffuseSolve[vi].Z = 0.0f;
		DiffuseSolve[vi].W = MeshDiffuse[vi].W;

		/*
		** Sun
		*/
		Add_Light_To_Vertex(context,vi,sun);

		/*
		** Other lights
		*/
		NonRefPhysListIterator it(&lights);
		while (!it.Is_Done()) {

			LightPhysClass * light = it.Peek_Obj()->As_LightPhysClass();
			if ((light) && (light->Peek_Model()) && (light->Peek_Model()->Class_ID() == RenderObjClass::CLASSID_LIGHT)) {

				LightClass * light_obj = (LightClass*)light->Peek_Model();

				Add_Light_To_Vertex(context,vi,light_obj);
			}
			it.Next();
		}

		context.Get_Progress().Set_Current_Vertex(vi);

		callback_counter++;
		if (callback_counter > 100) {
			callback_counter = 0;
			context.Update_Observer();
		}
	}

	VectorProcessorClass::Clamp(&(AmbientSolve[0]),&(AmbientSolve[0]), 0.0f, 1.0f,vcount);
	VectorProcessorClass::Clamp(&(DiffuseSolve[0]),&(DiffuseSolve[0]), 0.0f, 1.0f,vcount);

	/*
	** Modulate the accumulated light by the material properties.  Roadmap Section 52A,
	** Phase 52: when a caller has opted in, the ambient term (not the direct sun/light terms,
	** which already have their own per-light shadow test above) is additionally scaled by
	** each vertex's sky visibility, so creases and the insides of slopes darken.
	*/
	for (vi=0; vi<vcount; vi++) {
		float sky_occlusion = context.Is_Sky_Occlusion_Enabled() ? Compute_Sky_Occlusion(context,vi) : 1.0f;
		Solve[vi].X = AmbientSolve[vi].X * sky_occlusion * MeshAmbient[vi].X + DiffuseSolve[vi].X * MeshDiffuse[vi].X;
		Solve[vi].Y = AmbientSolve[vi].Y * sky_occlusion * MeshAmbient[vi].Y + DiffuseSolve[vi].Y * MeshDiffuse[vi].Y;
		Solve[vi].Z = AmbientSolve[vi].Z * sky_occlusion * MeshAmbient[vi].Z + DiffuseSolve[vi].Z * MeshDiffuse[vi].Z;
		Solve[vi].W = DiffuseSolve[vi].W;
	}
	VectorProcessorClass::Clamp(&(Solve[0]),&(Solve[0]), 0.0f, 1.0f,vcount);

	/*
	**	Pass the resultant vertex colors onto the patch
	*/
	for (vi=0; vi<vcount; vi++) {
		patch->Set_Vertex_Color (vi, Vector3 (Solve[vi].X, Solve[vi].Y, Solve[vi].Z));
	}

	//
	//	Let the patch know that it is now prelit
	//
	patch->Set_Is_Prelit (true);
	return ;
}


void VertexSolveClass::Add_Light_To_Vertex(LightSolveContextClass & context,int vi,LightClass * light_obj)
{
	if (light_obj->Is_Within_Attenuation_Radius(Position[vi])) {

		// cast ray...
		bool is_occluded = false;

// FIXME need a collision group for light occlusion here...
		if (context.Is_Occlusion_Enabled()) {
			CastResultStruct res;
			Vector3 p0 = Position[vi];
			Vector3 p1 = light_obj->Get_Position();

			if (light_obj->Get_Type() == LightClass::DIRECTIONAL) {
				Vector3 dir = -(light_obj->Get_Transform().Get_Z_Vector());
				p1 = p0 + dir * light_obj->Get_Attenuation_Range();
			}

			const float MOVE_AMOUNT = 0.25f;		// can't be occluded closer than this
			Vector3 delta = p1-p0;
			delta.Normalize();
			p0 += MOVE_AMOUNT * delta;

			p0 += _offset;
			p1 += _offset;

			LineSegClass ray(p0,p1);
			PhysRayCollisionTestClass raytest(ray,&res,0,COLLISION_TYPE_PROJECTILE);
			raytest.CheckDynamicObjs = false;
			PhysicsSceneClass::Get_Instance()->Cast_Ray(raytest);

			if (res.Fraction < 1.0f) {
				is_occluded = true;
			}
		}

		if (!is_occluded) {

			Vector3 ambient;
			Vector3 diffuse;
			light_obj->Compute_Lighting(Position[vi],Normal[vi],&ambient,&diffuse);

			AmbientSolve[vi] += Vector4(ambient.X,ambient.Y,ambient.Z,0.0f);
			DiffuseSolve[vi] += Vector4(diffuse.X,diffuse.Y,diffuse.Z,0.0f);
		}
	}
}

void VertexSolveClass::Grow_Arrays(int vcount)
{
	Position.Uninitialised_Grow(vcount);
	Normal.Uninitialised_Grow(vcount);
	AmbientSolve.Uninitialised_Grow(vcount);
	DiffuseSolve.Uninitialised_Grow(vcount);
	Solve.Uninitialised_Grow(vcount);
	MeshAmbient.Uninitialised_Grow(vcount);
	MeshDiffuse.Uninitialised_Grow(vcount);
	SunAmbientContrib.Uninitialised_Grow(vcount);
	SunDiffuseContrib.Uninitialised_Grow(vcount);
	BaseSolve.Uninitialised_Grow(vcount);
	SunVis.Uninitialised_Grow(vcount);
}


float VertexSolveClass::Compute_Sun_Visibility(LightSolveContextClass & context,int vi,LightClass * sun)
{
	if (sun == nullptr) {
		return 0.0f;
	}

	//	Geometric only, independent of the sun's own colour/intensity, so the caller can
	//	recolour it with whatever the sky clock says the sun is right now.
	Vector3 sun_dir = -(sun->Get_Transform().Get_Z_Vector());
	float ndotl = Vector3::Dot_Product(Normal[vi],sun_dir);
	if (ndotl <= 0.0f) {
		return 0.0f;
	}

	if (context.Is_Occlusion_Enabled()) {

		CastResultStruct res;
		Vector3 p0 = Position[vi];
		Vector3 p1 = p0 + sun_dir * sun->Get_Attenuation_Range();

		const float MOVE_AMOUNT = 0.25f;		// can't be occluded closer than this
		Vector3 delta = p1 - p0;
		delta.Normalize();
		p0 += MOVE_AMOUNT * delta;

		LineSegClass ray(p0,p1);
		PhysRayCollisionTestClass raytest(ray,&res,0,COLLISION_TYPE_PROJECTILE);
		raytest.CheckDynamicObjs = false;
		PhysicsSceneClass::Get_Instance()->Cast_Ray(raytest);

		if (res.Fraction < 1.0f) {
			return 0.0f;
		}
	}

	return ndotl;
}


//	Roadmap Section 52A, Phase 52.  Terrain and contact ambient occlusion.  Casts the same
//	kind of ray Compute_Sun_Visibility already casts, in a small fixed set of directions spread
//	cosine-weighted over the hemisphere above the vertex normal, and returns the unoccluded
//	fraction -- 1.0 fully open sky, dropping toward 0.0 inside a crease or against a slope's
//	own interior. Only ever called from Light_Terrain, and only when a caller has opted in via
//	LightSolveContextClass::Enable_Sky_Occlusion, so it costs nothing on an ordinary solve.
float VertexSolveClass::Compute_Sky_Occlusion(LightSolveContextClass & context,int vi)
{
	//	A local basis around the vertex normal: pick whichever world axis is least parallel to
	//	it as the seed so the cross products stay well conditioned at any slope.
	const Vector3 & n = Normal[vi];
	Vector3 seed = (WWMath::Fabs(n.Z) < 0.9f) ? Vector3(0.0f,0.0f,1.0f) : Vector3(1.0f,0.0f,0.0f);
	Vector3 tangent = Vector3::Cross_Product(seed,n);
	tangent.Normalize();
	Vector3 bitangent = Vector3::Cross_Product(n,tangent);

	//	Six samples: straight up the normal plus five spread around it at a fixed cone angle.
	//	Cheap and directionally representative -- this never runs at draw time, only once per
	//	bake, so it does not need to be a high-sample-count offline-quality AO solve.
	static const float AZIMUTHS[5] = { 0.0f, 72.0f, 144.0f, 216.0f, 288.0f };
	const float CONE_DEG = 50.0f;
	const float AO_TEST_DISTANCE = 6.0f;		// local-scale only; not meant to see distant terrain
	const float MOVE_AMOUNT = 0.25f;

	Vector3 directions[6];
	directions[0] = n;
	for (int i = 0; i < 5; i++) {
		float az = DEG_TO_RADF(AZIMUTHS[i]);
		float cone = DEG_TO_RADF(CONE_DEG);
		float s = WWMath::Sin(cone);
		directions[i+1] = (n * WWMath::Cos(cone)) + (tangent * (s * WWMath::Cos(az))) + (bitangent * (s * WWMath::Sin(az)));
		directions[i+1].Normalize();
	}

	float open_weight = 0.0f;
	float total_weight = 0.0f;
	for (int i = 0; i < 6; i++) {
		float ndotd = Vector3::Dot_Product(n,directions[i]);
		if (ndotd <= 0.0f) continue;			// below the local horizon, never contributes
		total_weight += ndotd;

		Vector3 p0 = Position[vi] + directions[i] * MOVE_AMOUNT;
		Vector3 p1 = Position[vi] + directions[i] * AO_TEST_DISTANCE;
		LineSegClass ray(p0,p1);
		CastResultStruct res;
		PhysRayCollisionTestClass raytest(ray,&res,0,COLLISION_TYPE_PROJECTILE);
		raytest.CheckDynamicObjs = false;
		PhysicsSceneClass::Get_Instance()->Cast_Ray(raytest);

		if (res.Fraction >= 1.0f) {
			open_weight += ndotd;
		}
	}

	if (total_weight <= 0.0f) return 1.0f;
	return open_weight / total_weight;
}




/*****************************************************************************
**
** LightSolveClass Implementation
**
*****************************************************************************/

/*
  Global Solve:
  - start with StaticObjList
  - pass list to cull function
  - cull function gens new list with objects that will not be lit removed
  - pass to solve function which accepts a list
  - iterate list, pass each to solve function for an object

  List Solve:
  - pass list to cull function
  - cull function gens new list with objects that will not be lit removed
  - pass to solve function which accepts a list
  - iterate list, pass each to solve function for an object
*/


void LightSolveClass::Generate_Static_Light_Solve(LightSolveContextClass & context)
{
	RefPhysListIterator it = PhysicsSceneClass::Get_Instance()->Get_Static_Object_Iterator();

	/*
	** Build a list of the objects to be solved
	*/
	RefPhysListClass solve_list;
	while (!it.Is_Done()) {
		StaticPhysClass * obj = it.Peek_Obj()->As_StaticPhysClass();
		if ((obj != nullptr) && (Does_Obj_Get_Static_Light_Solve(obj))) {
			solve_list.Add(obj);
		}
		it.Next();
	}


	/*
	** Pass the list to the solve function
	*/
	Compute_Solve(context,solve_list);
}

void LightSolveClass::Generate_Static_Light_Solve(LightSolveContextClass & context,RefPhysListClass & input_list)
{
	RefPhysListClass solve_list;

	/*
	** Cull out objects which should not get a lighting solve
	*/
	RefPhysListIterator it(&input_list);
	while (!it.Is_Done()) {
		StaticPhysClass * obj = it.Peek_Obj()->As_StaticPhysClass();
		if ((obj != nullptr) && (Does_Obj_Get_Static_Light_Solve(obj))) {
			solve_list.Add(obj);
		}
		it.Next();
	}

	/*
	** Pass the list to the solve function
	*/
	Compute_Solve(context,solve_list);
}


void LightSolveClass::Compute_Solve(LightSolveContextClass & context,RefPhysListClass & obj_list)
{
	RefPhysListIterator it(&obj_list);

	/*
	** Count the objects for the progress display
	*/
	int count = 0;
	while (!it.Is_Done()) {
		count++;
		it.Next();
	}
	context.Get_Progress().Set_Object_Count(count);

	/*
	** Generate a light solve for each static object
	*/
	it.First();
	while (!it.Is_Done() && !context.Get_Progress().Is_Cancel_Requested()) {
		StaticPhysClass * obj = it.Peek_Obj()->As_StaticPhysClass();
		if (obj != nullptr) {
			Compute_Solve(context,obj);
			context.Get_Progress().Increment_Processed_Object_Count();
		}
		it.Next();
	}

	/*
	** Re-gen all VB's, IB's, etc
	*/
	TheDX8MeshRenderer.Invalidate();
}

void LightSolveClass::Compute_Solve(LightSolveContextClass & context,StaticPhysClass * obj)
{
	WWASSERT(obj != nullptr);
	if (context.Get_Progress().Get_Object_Count() == 0) {
		context.Get_Progress().Set_Object_Count(1);
	}

	/*
	** Generate a light solve for each mesh in this object
	*/
	if (obj->Is_Model_Pre_Lit() == false) {
		NonRefPhysListClass light_list;
		PhysicsSceneClass::Get_Instance()->Collect_Lights(obj->Get_Cull_Box(),true,false,&light_list);
		Compute_Solve(context,obj->Peek_Model(),light_list);
	}
}

void LightSolveClass::Compute_Solve(LightSolveContextClass & context,RenderObjClass * obj,NonRefPhysListClass & light_list)
{
	/*
	** Mark this render object as containing a static lighting solve
	*/
	obj->Set_Has_User_Lighting(true);

	/*
	** Recurse through all sub-objects
	*/
	WWASSERT(obj != nullptr);
	for (int i=0; i<obj->Get_Num_Sub_Objects(); i++) {
		RenderObjClass * sub_obj = obj->Get_Sub_Object(i);
		if (sub_obj != nullptr) {
			Compute_Solve(context,sub_obj,light_list);
			REF_PTR_RELEASE(sub_obj);
		}
	}

	/*
	** For all mesh objects, generate a vertex lighting solution.
	*/
	if (obj->Class_ID() == RenderObjClass::CLASSID_MESH) {
		MeshClass * mesh = (MeshClass*)obj;
		WWDEBUG_SAY(("Generating Solve for Mesh: %s\r\n",mesh->Get_Name()));
		TheVertexSolver.Light_Mesh(context,mesh,light_list);
	} else if (obj->Class_ID() == RenderObjClass::CLASSID_RENEGADE_TERRAIN) {
		TheVertexSolver.Light_Terrain(context,(RenegadeTerrainPatchClass *)obj,light_list);
	}
}

bool LightSolveClass::Does_Obj_Get_Static_Light_Solve(StaticPhysClass * obj)
{
	if (obj->Is_Model_Pre_Lit()) {
		return false;
	}

	return Does_Model_Get_Static_Light_Solve(obj->Peek_Model());
}

bool LightSolveClass::Does_Model_Get_Static_Light_Solve(RenderObjClass * model)
{
	if (model == nullptr) {
		return false;
	}

	if (!model->Is_Not_Hidden_At_All()) {
		return false;
	}

	if (model->Get_Num_Sub_Objects() > 0) {
		bool any_sub_objs_visible = false;
		for (int i=0; i<model->Get_Num_Sub_Objects(); i++) {
			RenderObjClass * sub_obj = model->Get_Sub_Object(i);
			if (sub_obj != nullptr) {
				if (sub_obj->Is_Not_Hidden_At_All()) {
					any_sub_objs_visible = true;
				}
				REF_PTR_RELEASE(sub_obj);
			}
		}

		if (any_sub_objs_visible == false) {
			return false;
		}
	}

	// TODO: check if the entire model is emissive???

	return true;
}





