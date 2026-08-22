#ifndef TT_INCLUDE_PHYSDECALSYSCLASS_H
#define TT_INCLUDE_PHYSDECALSYSCLASS_H
#include "DecalSys.h"
#include "engine_vector.h"
#include "MeshClass.h"
#include "PhysicsSceneClass.h"
class PhysDecalSysClass : public DecalSystemClass
{
public:
	static int decalpoolsize;
	class LogicalDecalClass : public MultiListObjectClass
	{
	public:
		int id;
		SimpleDynVecClass<MeshClass *> meshes;
		bool operator == (LogicalDecalClass const & rec) const	{ return false; }
		bool operator != (LogicalDecalClass const & rec) const	{ return true; }
		LogicalDecalClass() : id(-1)
		{
		}
		void Reset()
		{
			for (int i = 0;i < meshes.Count();i++)
			{
				meshes[i]->Delete_Decal(id);
			}
			meshes.Delete_All();
			id = -1;
		}
		~LogicalDecalClass()
		{
			Reset();
		}
	};
	PhysicsSceneClass *scene;
	bool is_permanent;
	int count;
	VectorClass<LogicalDecalClass> logicaldecals;
	MultiListClass<LogicalDecalClass> logicaldecallist;
	VertexMaterialClass *vertmat;
	ShaderClass shader;
	int unknown;
	void allocate_resources();
	void release_resources();
	void Set_Temporary_Decal_Pool_Size(int size);
	PhysDecalSysClass(PhysicsSceneClass *pscene) : scene(pscene), is_permanent(false), count(0), vertmat(0), shader(0)
	{
		allocate_resources();
		Set_Temporary_Decal_Pool_Size(decalpoolsize);
	}
	virtual ~PhysDecalSysClass()
	{
		release_resources();
	}
	virtual void Unlock_Decal_Generator(DecalGeneratorClass* generator);
	virtual void Decal_Mesh_Destroyed(unsigned long decal_id,DecalMeshClass* mesh);
	virtual unsigned long Generate_Decal_Id();
};

#endif