#ifndef __SIMPLENETWORKOBJECTFACTORYCLASS_H__
#define __SIMPLENETWORKOBJECTFACTORYCLASS_H__



#include "NetworkObjectFactoryClass.h"
#include "NetworkObjectFactoryMgrClass.h"



template<class OBJECT_CLASS, int OBJECT_CLASS_ID>
class SimpleNetworkObjectFactoryClass :
	public NetworkObjectFactoryClass
{

public:

	SimpleNetworkObjectFactoryClass() :
		NetworkObjectFactoryClass(false)
	{
		NetworkObjectFactoryMgrClass::Unregister(OBJECT_CLASS_ID);
		NetworkObjectFactoryMgrClass::Register_Factory(this);
	}

	virtual NetworkObjectClass* Create(cPacket&) const { return new OBJECT_CLASS; }
	virtual uint32 Get_Class_ID() const { return OBJECT_CLASS_ID; }

};


#endif
