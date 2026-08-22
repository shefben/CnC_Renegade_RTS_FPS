#ifndef TT_INCLUDE__DYNAMICPHYSDEFCLASS_H
#define TT_INCLUDE__DYNAMICPHYSDEFCLASS_H

#include "PhysDefClass.h"


class DynamicPhysDefClass : public PhysDefClass
{
public:
	DynamicPhysDefClass(void);
	virtual bool								Save(ChunkSaveClass &csave);
	virtual bool								Load(ChunkLoadClass &cload);
	virtual const char *						Get_Type_Name(void)			{ return "DynamicPhysDef"; }
	virtual bool								Is_Type(const char *);
	virtual bool								Is_Valid_Config (StringClass &message);
	DECLARE_EDITABLE(DynamicPhysDefClass,PhysDefClass);
protected:
	friend class DynamicPhysClass;
};
#endif
