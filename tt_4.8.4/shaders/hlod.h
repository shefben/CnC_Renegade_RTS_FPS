#ifndef TT_INCLUDE_HLOD_H
#define TT_INCLUDE_HLOD_H

#include "animobj.h"
#include "engine_vector.h"
class SnapPointsClass;
class ProxyArrayClass;
class ProxyClass;
class HLodClass : public Animatable3DObjClass {
public:
	class ModelNodeClass {
	public:
		RenderObjClass* Model;
		int BoneIndex;
		bool operator== (const ModelNodeClass &src) { return false; }
		bool operator!= (const ModelNodeClass &src) { return true; }
	};
	class ModelArrayClass : public DynamicVectorClass<ModelNodeClass> {
	public:
		float MaxScreenSize;
		float NonPixelCost;
		float PixelCostPerArea;
		float BenefitFactor;
	};
	int LodCount;
	int CurLod;
	ModelArrayClass* Lod;
	int BoundingBoxIndex;
	float* Cost;
	float* Value;
	ModelArrayClass AdditionalModels;
	SnapPointsClass* SnapPoints;
	ProxyArrayClass* ProxyArray;
	float LODBias;
	~HLodClass();
	RenderObjClass*      Clone                             () const;
	int                  Class_ID                          () const;
	int                  Get_Num_Polys                     () const;
	void SHADERS_API     Render                            (RenderInfoClass&);
	void                 Special_Render                    (SpecialRenderInfoClass&);
	void                 Set_Transform                     (const Matrix3D&);
	void                 Set_Position                      (const Vector3&);
	void                 Notify_Added                      (SceneClass*);
	void                 Notify_Removed                    (SceneClass*);
	int                  Get_Num_Sub_Objects               () const;
	RenderObjClass*      Get_Sub_Object                    (int index) const;
	int                  Add_Sub_Object                    (RenderObjClass*);
	int                  Remove_Sub_Object                 (RenderObjClass*);
	int                  Get_Num_Sub_Objects_On_Bone       (int boneIndex) const;
	RenderObjClass*      Get_Sub_Object_On_Bone            (int index, int boneIndex) const;
	int                  Get_Sub_Object_Bone_Index         (RenderObjClass* object) const;
	int                  Add_Sub_Object_To_Bone            (RenderObjClass*, int boneIndex);
	void                 Update_Sub_Object_Transforms      ();
	void                 Set_Animation                     ();
	void                 Set_Animation                     (HAnimClass*, float, int);
	void                 Set_Animation                     (HAnimClass*, float, HAnimClass*, float, float);
	void                 Set_Animation                     (HAnimComboClass*);
	bool                 Cast_Ray                          (RayCollisionTestClass&);
	bool                 Cast_AABox                        (AABoxCollisionTestClass&);
	bool                 Cast_OBBox                        (OBBoxCollisionTestClass&);
	bool                 Intersect_AABox                   (AABoxIntersectionTestClass&);
	bool                 Intersect_OBBox                   (OBBoxIntersectionTestClass&);
	const SphereClass&   Get_Bounding_Sphere               ();
	const AABoxClass&    Get_Bounding_Box                  ();
	void                 Get_Obj_Space_Bounding_Sphere     (SphereClass&) const;
	void                 Get_Obj_Space_Bounding_Box        (AABoxClass&) const;
	void                 Update_Obj_Space_Bounding_Volumes ();
	void                 Prepare_LOD                       (CameraClass&);
	void                 Recalculate_Static_LOD_Factors    ();
	void                 Increment_LOD                     ();
	void                 Decrement_LOD                     ();
	float                Get_Cost                          () const;
	float                Get_Value                         () const;
	float                Get_Post_Increment_Value          () const;
	void                 Set_LOD_Level                     (int);
	int                  Get_LOD_Level                     () const;
	int                  Get_LOD_Count                     () const;
	void                 Set_LOD_Bias                      (float);
	int                  Calculate_Cost_Value_Arrays       (float, float*, float*) const;
	RenderObjClass *     Get_Current_LOD                   ();
	void                 Create_Decal                      (DecalGeneratorClass*);
	void                 Delete_Decal                      (uint32);
	int                  Get_Num_Snap_Points               ();
	void                 Get_Snap_Point                    (int, Vector3*);
	void                 Scale                             (float);
	void                 Scale                             (float, float, float);
	void                 Set_Hidden                        (int hidden);
	void Set_HTree(HTreeClass* htree);
	virtual void Set_Max_Screen_Size(int lod_index,float size);
	virtual float Get_Max_Screen_Size(int lod_index);
	virtual int Get_Lod_Count();
	virtual int Get_Lod_Model_Count(int lod_index);
	virtual RenderObjClass *Peek_Lod_Model(int lod_index,int model_index);
	virtual RenderObjClass *Get_Lod_Model(int lod_index,int model_index);
	virtual int Get_Lod_Model_Bone(int lod_index,int model_index);
	virtual int Get_Additional_Model_Count();
	virtual RenderObjClass *Peek_Additional_Model(int model_index);
	virtual RenderObjClass *Get_Additional_Model(int model_index);
	virtual int Get_Additional_Model_Bone(int model_index);
	virtual bool Is_NULL_Lod_Included();
	virtual void Include_NULL_Lod(bool include);
	virtual int Get_Proxy_Count();
	virtual bool Get_Proxy(int index,ProxyClass& proxy);
};

#endif
