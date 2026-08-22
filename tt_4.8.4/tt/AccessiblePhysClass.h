#ifndef TT_INCLUDE__ACCESSIBLEPHYSCLASS_H
#define TT_INCLUDE__ACCESSIBLEPHYSCLASS_H
#include "StaticAnimPhysClass.h"
class AccessiblePhysClass : public StaticAnimPhysClass {
public:
	int LockCode;
	~AccessiblePhysClass();
	AccessiblePhysClass * As_AccessiblePhysClass() {return 0;}
	const PersistFactoryClass& Get_Factory  () const;
	bool Save(ChunkSaveClass &csave);
	bool Load(ChunkLoadClass &cload);
};

#endif
