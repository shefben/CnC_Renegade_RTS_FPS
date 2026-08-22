#ifndef TT_INCLUDE__DYNAMICPHYSCLASS_H
#define TT_INCLUDE__DYNAMICPHYSCLASS_H

#include "PhysClass.h"
#include "DynamicPhysDefClass.h"


class DynamicPhysClass : public PhysClass
{
public:
	DynamicPhysClass(void);
	~DynamicPhysClass(void);
	virtual DynamicPhysClass *	As_DynamicPhysClass(void)									{ return this; }
	void Init(const DynamicPhysDefClass & definition);
	virtual void								Set_Model(RenderObjClass * model);
	void											Update_Visibility_Status(void);
	virtual int									Get_Vis_Object_ID(void);
	virtual bool								Is_Simulation_Disabled(void);
	virtual bool								Is_Rendering_Disabled(void);
	virtual bool								Save (ChunkSaveClass &csave);
	virtual bool								Load (ChunkLoadClass &cload);
	virtual void								On_Post_Load(void);
protected:
	void											Internal_Update_Visibility_Status(void);
	bool											DirtyVisObjectID; // 0078  0078  0094  0090
	int											VisNodeID; // 007C  007C  0098  0094
	unsigned int								VisStatusLastUpdated; // 0080  0080  009C  0098
private:
	DynamicPhysClass(const DynamicPhysClass &);
	DynamicPhysClass & operator = (const DynamicPhysClass &);
}; // 0084  0084  00A0  009C

#endif
