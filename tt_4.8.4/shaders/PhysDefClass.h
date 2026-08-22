#ifndef TT_INCLUDE__PHYSDEFCLASS_H
#define TT_INCLUDE__PHYSDEFCLASS_H

#include "engine_common.h"
#include "engine_vector.h"
#include "engine_io.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "Definition.h"

class PhysDefClass : public DefinitionClass
{
public:
	PhysDefClass(void);
	virtual bool					Save(ChunkSaveClass &csave);
	virtual bool					Load(ChunkLoadClass &cload);
	virtual const char *			Get_Type_Name(void)				{ return "PhysDef"; }
	virtual bool					Is_Type(const char *);
	virtual bool					Is_Valid_Config (StringClass &message);
	const StringClass &			Get_Model_Name()					{ return ModelName; }
	bool								Get_Is_Pre_Lit()					{ return IsPreLit; }
	DECLARE_EDITABLE(PhysDefClass,DefinitionClass);
protected:
	StringClass						ModelName;
	bool								IsPreLit;
	friend class PhysClass;
}; // 36

#endif
