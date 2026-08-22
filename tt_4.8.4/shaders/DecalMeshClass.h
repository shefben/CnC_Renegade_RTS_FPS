#ifndef TT_INCLUDE__DECALMESHCLASS_H
#define TT_INCLUDE__DECALMESHCLASS_H
class MeshClass;
class DecalSystemClass;
class DecalGeneratorClass;
class DecalMeshClass : public RefCountClass {
private:
	MeshClass* Parent;
	DecalSystemClass* DecalSystem;
	DecalMeshClass* NextVisible;
public:
	MeshClass* Peek_Parent()
	{
		return Parent;
	}
	DecalSystemClass* Peek_System()
	{
		return DecalSystem;
	}
	DecalMeshClass* Peek_Next_Visible()
	{
		return NextVisible;
	}
	virtual ~DecalMeshClass();
	virtual void Render() = 0;
	virtual bool Create_Decal(DecalGeneratorClass*,OBBoxClass&,SimpleDynVecClass<unsigned long>&,DynamicVectorClass<Vector3>*) = 0;
	virtual bool Delete_Decal(unsigned long) = 0;
	virtual int Decal_Count() = 0;
	virtual unsigned long Get_Decal_ID(int) = 0;

	void setNextVisible(DecalMeshClass* nextVisible)
	{
		NextVisible = nextVisible;
	}
};
#endif