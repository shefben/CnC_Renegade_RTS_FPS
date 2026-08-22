#include "General.h"
#include "RenderObjClass.h"
#include "SceneClass.h"
#include "cameraclass.h"
#include "wwmath.h"
#include "htreeclass.h"
#include "CastResultStruct.h"
#include "LineSegClass.h"
#include "intersec.h"
#include "coltest.h"
#include "engine_io.h"
REF_DEF1(PredictiveLODOptimizerClass::TotalCost,float,0x00836910);

RENEGADE_FUNCTION
void PredictiveLODOptimizerClass::Add_Object(RenderObjClass *obj)
AT1(0x0057B630);

const float	RenderObjClass::AT_MIN_LOD = FLT_MAX;
const float	RenderObjClass::AT_MAX_LOD = -1.0f;
StringClass
Filename_From_Asset_Name (const char *asset_name)
{
	StringClass filename;
	if (asset_name != NULL) {
		::lstrcpy (filename.Get_Buffer (::lstrlen (asset_name) + 5), asset_name);
		char *suffix = (char *)::strchr (filename, '.');
		if (suffix != NULL) {
			suffix[0] = 0;
		}
		filename += ".w3d";
	}
	return filename;
}
static inline bool Check_Is_Transform_Identity(const Matrix3D& m)
{
	const float zero=0.0f;
	const float one=1.0f;

	unsigned d=
		((unsigned&)m[0][0]^(unsigned&)one) |
		((unsigned&)m[0][1]^(unsigned&)zero) |
		((unsigned&)m[0][2]^(unsigned&)zero) |
		((unsigned&)m[0][3]^(unsigned&)zero) |
		((unsigned&)m[1][0]^(unsigned&)zero) |
		((unsigned&)m[1][1]^(unsigned&)one) |
		((unsigned&)m[1][2]^(unsigned&)zero) |
		((unsigned&)m[1][3]^(unsigned&)zero) |
		((unsigned&)m[2][0]^(unsigned&)zero) |
		((unsigned&)m[2][1]^(unsigned&)zero) |
		((unsigned&)m[2][2]^(unsigned&)one) |
		((unsigned&)m[2][3]^(unsigned&)zero);
	return !d;
}
RenderObjClass::RenderObjClass(void) :
	Bits(DEFAULT_BITS),
	Transform(1),
	NativeScreenSize(1.0),
	Scene(NULL),
	Container(NULL),
	User_Data(NULL),
	CachedBoundingSphere(Vector3(0,0,0),1.0f),
	CachedBoundingBox(Vector3(0,0,0),Vector3(1,1,1)),
	IsTransformIdentity(false)
{
}
RenderObjClass::RenderObjClass(const RenderObjClass & src) :
	Bits(src.Bits),
	Transform(src.Transform),
	NativeScreenSize(src.NativeScreenSize),
	Scene(NULL),
	Container(NULL),
	User_Data(NULL),
	CachedBoundingSphere(src.CachedBoundingSphere),
	CachedBoundingBox(src.CachedBoundingBox),
	IsTransformIdentity(src.IsTransformIdentity)
{
}
RenderObjClass & RenderObjClass::operator = (const RenderObjClass & that)
{
	if (this != &that) {
		Set_Hidden(that.Is_Hidden());
		Set_Animation_Hidden(that.Is_Animation_Hidden());
		Set_Force_Visible(that.Is_Force_Visible());
		Set_Collision_Type(that.Get_Collision_Type());
		Set_Native_Screen_Size(that.Get_Native_Screen_Size());
		IsTransformIdentity=false;
	}
	return *this;
}
float RenderObjClass::Get_Screen_Size(CameraClass& camera)
{
	Vector3 cam = camera.Get_Position();
	ViewportClass viewport = camera.Get_Viewport();
	Vector2 vpr_min, vpr_max;
	camera.Get_View_Plane(vpr_min, vpr_max);
	float width_factor = viewport.Width() / (vpr_max.X - vpr_min.X);
	float height_factor = viewport.Height() / (vpr_max.Y - vpr_min.Y);
	const SphereClass & sphere = Get_Bounding_Sphere();
	float dist = (sphere.Center - cam).Length();
	float radius = 0.0f;
	if (dist) {
		radius = sphere.Radius / dist;
	}
	return WWMATH_PI * radius * radius * width_factor * height_factor;
}
SceneClass * RenderObjClass::Get_Scene(void)
{
	if (Scene != NULL) {
		Scene->Add_Ref();
	}
	return Scene;
}
void RenderObjClass::Set_Container(RenderObjClass * con)
{ 
	Container = con; 
}
RenderObjClass * RenderObjClass::Get_Container(void) const													
{ 
	return Container; 
}
void RenderObjClass::Set_Transform(const Matrix3D &m)
{
	Transform = m;
	IsTransformIdentity=Check_Is_Transform_Identity(m);
	Invalidate_Cached_Bounding_Volumes();
}
void RenderObjClass::Set_Position(const Vector3 &v)
{
	Transform.Set_Translation(v);
	IsTransformIdentity=Check_Is_Transform_Identity(Transform);
	Invalidate_Cached_Bounding_Volumes();
}
void RenderObjClass::Validate_Transform(void) const
{
	RenderObjClass * con = Get_Container();
	bool dirty=false;
	if (con != NULL) {
		dirty = con->Are_Sub_Object_Transforms_Dirty();
		while (con->Get_Container() != NULL) {
			dirty |= con->Are_Sub_Object_Transforms_Dirty();
			con = con->Get_Container();
		}
		if (dirty) {
			con->Update_Sub_Object_Transforms();
		}
	}
	if (dirty) IsTransformIdentity=Check_Is_Transform_Identity(Transform);
}
Vector3 RenderObjClass::Get_Position(void) const
{
	Validate_Transform();
	return Transform.Get_Translation();
}
RenderObjClass * RenderObjClass::Get_Sub_Object_By_Name(const char * name) const
{
	int i;
	for (i=0; i<Get_Num_Sub_Objects(); i++) {
		RenderObjClass * robj = Get_Sub_Object(i);
		if (robj) {
			if (_stricmp(robj->Get_Name(),name) == 0) {
				return robj;
			} else {
				robj->Release_Ref();
			}
		}
	}
	for (i=0; i<Get_Num_Sub_Objects(); i++) {
		RenderObjClass * robj = Get_Sub_Object(i);
		if (robj) {
			const char * subobjname = strchr(robj->Get_Name(),'.');
			if (subobjname == NULL) {
				subobjname = robj->Get_Name();	
			} else {
				subobjname = subobjname+1;
			}
			if (_stricmp(subobjname,name) == 0) {
				return robj;
			} else {
				robj->Release_Ref();
			}
		}
	}
	return NULL;
}
int RenderObjClass::Add_Sub_Object_To_Bone(RenderObjClass * subobj,const char * bname)
{
	int bindex = Get_Bone_Index(bname);
	return Add_Sub_Object_To_Bone(subobj,bindex);
}
int RenderObjClass::Remove_Sub_Objects_From_Bone(const char * bname)
{
	int boneidx = Get_Bone_Index(bname);
	int count = Get_Num_Sub_Objects_On_Bone(boneidx);
	int remove_count = 0;
	for (int i = count-1; i >= 0; i--) {
		RenderObjClass *robj = Get_Sub_Object_On_Bone(i, boneidx);
		if ( robj ) {
			remove_count += Remove_Sub_Object(robj);
			robj->Release_Ref();
		}
	}
	return remove_count;
}
void RenderObjClass::Prepare_LOD(CameraClass&)
{
	PredictiveLODOptimizerClass::Add_Cost(Get_Cost());
}
float RenderObjClass::Get_Cost(void) const
{
	int polycount = Get_Num_Polys();
	float cost = (polycount != 0)? polycount : 0.000001f;
	return cost;
}
int RenderObjClass::Calculate_Cost_Value_Arrays(float screen_area, float *values, float *costs) const
{
	values[0] = AT_MIN_LOD;
	values[1] = AT_MAX_LOD;
	costs[0] = Get_Cost();
	return 0;
}
void RenderObjClass::Update_Sub_Object_Bits(void)
{
	if (Get_Num_Sub_Objects() == 0) return;
	int coltype = 0;
	int istrans = 0;
	for (int ni = 0; ni < Get_Num_Sub_Objects(); ni++) {
		RenderObjClass * robj = Get_Sub_Object(ni);
		coltype |= robj->Get_Collision_Type();
		istrans |= robj->Is_Translucent();
		robj->Release_Ref();
	}
	Set_Collision_Type(coltype);
	Set_Translucent(istrans);	
	if (Container) {
		Container->Update_Sub_Object_Bits();
	}
}
void RenderObjClass::Update_Sub_Object_Transforms(void)
{
}
void RenderObjClass::Add(SceneClass * scene)
{
	Scene = scene;
	Scene->Add_Render_Object(this);
}
void RenderObjClass::Remove(void)
{
	if (Container == NULL) {
		if (Scene != NULL) {
			Scene->Remove_Render_Object(this);
			return;
		}
	} else {
		Container->Remove_Sub_Object(this);
		return;
	}
}
void RenderObjClass::Notify_Added(SceneClass * scene)
{
	Scene = scene;
}
void RenderObjClass::Notify_Removed(SceneClass * scene)
{
	Scene = NULL;
}
void RenderObjClass::Update_Cached_Bounding_Volumes() const
{
	Get_Obj_Space_Bounding_Box(CachedBoundingBox);
	Get_Obj_Space_Bounding_Sphere(CachedBoundingSphere);
	CachedBoundingSphere.Center = Get_Transform() * CachedBoundingSphere.Center;
	CachedBoundingBox.Transform(Get_Transform());
	Validate_Cached_Bounding_Volumes();
}
void RenderObjClass::Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const
{
	sphere.Center.Set(0,0,0);
	sphere.Radius = 1.0f;
}
void RenderObjClass::Get_Obj_Space_Bounding_Box(AABoxClass & box) const
{
	box.Center.Set(0,0,0);
	box.Extent.Set(0,0,0);
}
bool RenderObjClass::Intersect(IntersectionClass* Intersection, IntersectionResultClass* Final_Result)
{
	if (Intersect_Sphere_Quick(Intersection, Final_Result))
	{
		CastResultStruct castresult;
		LineSegClass lineseg;
		Vector3 end = *Intersection->RayLocation + *Intersection->RayDirection * Intersection->MaxDistance;
		lineseg.Set(*Intersection->RayLocation,end);
		RayCollisionTestClass ray(lineseg, &castresult);
		ray.CollisionType = COLLISION_TYPE_ALL;
		if (Cast_Ray(ray))
		{
			lineseg.Compute_Point(ray.Result->Fraction,&(Final_Result->Intersection));
			Final_Result->Intersects = true;
			Final_Result->IntersectionType = IntersectionResultClass::GENERIC;
			if (Intersection->IntersectionNormal)
			{
				*Intersection->IntersectionNormal = castresult.Normal;
			}
			Final_Result->IntersectedRenderObject = this;
			Final_Result->ModelMatrix = Transform;
			return true;
		}
	}
	Final_Result->Intersects = false;
	return false;
}

bool RenderObjClass::Intersect_Sphere(IntersectionClass *Intersection, IntersectionResultClass *Final_Result)
{
	SphereClass sphere = Get_Bounding_Sphere();
	return Intersection->Intersect_Sphere(sphere, Final_Result); 
}
bool RenderObjClass::Intersect_Sphere_Quick(IntersectionClass *Intersection, IntersectionResultClass *Final_Result)
{
	SphereClass sphere = Get_Bounding_Sphere();
	return Intersection->Intersect_Sphere_Quick(sphere, Final_Result); 
}
bool RenderObjClass::Build_Dependency_List (DynamicVectorClass<StringClass> &file_list, bool recursive)
{
	if (recursive)
	{
		int subobj_count = Get_Num_Sub_Objects ();
		for (int index = 0; index < subobj_count; index ++) {
			RenderObjClass *psub_obj = Get_Sub_Object (index);
			if (psub_obj != NULL) {
				psub_obj->Build_Dependency_List(file_list);
				psub_obj->Release_Ref ();
			}
		}
	}
	Add_Dependencies_To_List (file_list);
	return (file_list.Count () > 0);
}
bool RenderObjClass::Build_Texture_List
(
	DynamicVectorClass<StringClass> &	texture_file_list,
	bool											recursive
)
{
	if (recursive) {
		int subobj_count = Get_Num_Sub_Objects ();
		for (int index = 0; index < subobj_count; index ++) {
			RenderObjClass *sub_obj = Get_Sub_Object (index);
			if (sub_obj != NULL) {
				sub_obj->Build_Texture_List (texture_file_list);
				sub_obj->Release_Ref ();
			}
		}
	}
	Add_Dependencies_To_List (texture_file_list, true);
	return (texture_file_list.Count () > 0);
}
void RenderObjClass::Add_Dependencies_To_List
(
	DynamicVectorClass<StringClass> &file_list,
	bool										textures_only
)
{
	if (textures_only == false) {
		const char *model_name = Get_Name ();
		file_list.Add (::Filename_From_Asset_Name (model_name));
		HTreeClass *phtree = Get_HTree ();
		if (phtree != NULL) {
			char *htree_name = phtree->Get_Name ();
			if (::lstrcmpi (htree_name, model_name) != 0) {
				file_list.Add (::Filename_From_Asset_Name (htree_name));
			}
		}
		const char *base_model_name = Get_Base_Model_Name ();
		if (base_model_name != NULL) {
			file_list.Add (::Filename_From_Asset_Name (base_model_name));
		}
	}
	return;
}
RENEGADE_FUNCTION
const PersistFactoryClass& RenderObjClass::Get_Factory() const
AT1(0x00572050);
bool RenderObjClass::Save (ChunkSaveClass &csave)
{
	return true;
}
bool RenderObjClass::Load (ChunkLoadClass &cload)
{
	return true;
}
enum
{
	CHUNKID_SUBOBJ_USER_LIGHTING = 0x02191159,
	CHUNKID_SUBOBJ_NAME,
	CHUNKD_SUBOBJ_BONE_INDEX,
	CHUNKID_SUBOBJ_LIGHTING_DATA
};
void RenderObjClass::Save_User_Lighting (ChunkSaveClass & csave)
{
	if (Has_User_Lighting()) {
		for (int bi=0; bi<Get_Num_Bones(); bi++) {
			int bone_obj_count = Get_Num_Sub_Objects_On_Bone(bi);
			for (int ri=0; ri<bone_obj_count; ri++) {
				RenderObjClass * sub_obj = Get_Sub_Object_On_Bone(ri,bi);

				if (	sub_obj && 
						(sub_obj->Class_ID() == RenderObjClass::CLASSID_MESH) && 
						(sub_obj->Has_User_Lighting())) 
				{
					csave.Begin_Chunk(CHUNKID_SUBOBJ_USER_LIGHTING);
					Save_Sub_Object_User_Lighting(csave,sub_obj,bi);
					csave.End_Chunk();

					REF_PTR_RELEASE(sub_obj);
				}
			}
		}
	}
}
void RenderObjClass::Save_Sub_Object_User_Lighting(ChunkSaveClass & csave,RenderObjClass * sub_obj,int bone_index)
{
	csave.Begin_Chunk(CHUNKID_SUBOBJ_NAME);
	const char * name = sub_obj->Get_Name();
	csave.Write((void *)name,strlen(name) + 1);
	csave.End_Chunk();

	csave.Begin_Chunk(CHUNKD_SUBOBJ_BONE_INDEX);
	csave.Write(&bone_index,sizeof(int));
	csave.End_Chunk();

	csave.Begin_Chunk(CHUNKID_SUBOBJ_LIGHTING_DATA);
	sub_obj->Save_User_Lighting(csave);
	csave.End_Chunk();

	Set_Has_User_Lighting(true);
}
void RenderObjClass::Load_User_Lighting (ChunkLoadClass & cload)
{
	while (cload.Open_Chunk()) {
	
		switch (cload.Cur_Chunk_ID()) {
			case CHUNKID_SUBOBJ_USER_LIGHTING:
				Load_Sub_Object_User_Lighting(cload);
				break;
		}

		cload.Close_Chunk();
		Set_Has_User_Lighting(true);
	}
}

void RenderObjClass::Load_Sub_Object_User_Lighting(ChunkLoadClass & cload)
{
	const int BUFFER_SIZE = 256;
	char tmp_string[BUFFER_SIZE];
	int bone_index;
	cload.Open_Chunk();
	cload.Read(tmp_string,cload.Cur_Chunk_Length());
	tmp_string[BUFFER_SIZE-1] = 0;
	cload.Close_Chunk();
	cload.Open_Chunk();
	cload.Read(&bone_index,sizeof(int));
	cload.Close_Chunk();
	if ((bone_index < 0) || (bone_index >= Get_Num_Bones())) {
		return;
	}
	cload.Open_Chunk();
	RenderObjClass * obj = NULL;
	int bone_obj_count = Get_Num_Sub_Objects_On_Bone(bone_index);
	for (int obj_index=0; (obj_index<bone_obj_count) && (obj == NULL); obj_index++) {
		RenderObjClass * sub_obj = Get_Sub_Object_On_Bone(obj_index,bone_index);
		if (_stricmp(sub_obj->Get_Name(), tmp_string) == 0) {
			obj = sub_obj;
		}
		REF_PTR_RELEASE(sub_obj);
	}
	if (obj != NULL) {
		obj->Load_User_Lighting(cload);
	}
	cload.Close_Chunk();
}
