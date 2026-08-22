#ifndef TT_INCLUDE_DECALSYS_H
#define TT_INCLUDE_DECALSYS_H
#include "projector.h"
class DecalGeneratorClass;
class DecalMeshClass;
class DecalSystemClass {
public:
	DecalSystemClass() {}
	virtual ~DecalSystemClass() {};
	SHADERS_API virtual DecalGeneratorClass *Lock_Decal_Generator();
	virtual void Unlock_Decal_Generator(DecalGeneratorClass* generator);
	virtual void Decal_Mesh_Destroyed(unsigned long decal_id,DecalMeshClass* mesh);
	virtual unsigned long Generate_Decal_Id();
};
class MultiFixedPoolDecalSystemClass : public DecalSystemClass {
private:
	class LogicalDecalClass {
		MultiListClass<RenderObjClass> MeshList;
	};
	class LogicalDecalPoolClass {
		LogicalDecalClass* Array;
		unsigned long Size;
	};
	unsigned long Generator_PoolID;
	unsigned long Generator_SlotID;
	LogicalDecalPoolClass* Pools;
	unsigned long PoolCount;
public:
	virtual ~MultiFixedPoolDecalSystemClass();
	SHADERS_API virtual DecalGeneratorClass *Lock_Decal_Generator();
	virtual void Unlock_Decal_Generator(DecalGeneratorClass* generator);
	virtual void Decal_Mesh_Destroyed(unsigned long decal_id,DecalMeshClass* mesh);
	virtual unsigned long Generate_Decal_Id();
	void Clear_Decal_Slot(unsigned long pool_id,unsigned long slot_id);
};
class DecalGeneratorClass : public ProjectorClass {
public:
	uint32							Get_Decal_ID(void)								{ return DecalID; }
	DecalSystemClass *			Peek_Decal_System(void)							{ return System; }
	void								Set_Backface_Threshhold(float val)			{ BackfaceVal = val; }
	float								Get_Backface_Threshhold(void)					{ return BackfaceVal; }
	void								Apply_To_Translucent_Meshes(bool onoff)	{ ApplyToTranslucentMeshes = onoff; }
	bool								Is_Applied_To_Translucent_Meshes(void)		{ return ApplyToTranslucentMeshes; }
	MaterialPassClass *			Get_Material(void)								{ Material->Add_Ref(); return Material; }
	SHADERS_API void Set_Mesh_Transform(Matrix3D  const&transform);
protected:
	DecalGeneratorClass(uint32 id,DecalSystemClass * system);
	~DecalGeneratorClass(void);
	DecalSystemClass *			System;		
	uint32							DecalID;
	float								BackfaceVal;
	bool								ApplyToTranslucentMeshes;
	MaterialPassClass *			Material;
	MultiListClass<RenderObjClass> MeshList;
	friend class DecalSystemClass;
};
#endif