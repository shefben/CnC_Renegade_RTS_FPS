#include "General.h"
#include "NetworkObjectFactoryClass.h"
#include "NetworkObjectFactoryMgrClass.h"



NetworkObjectFactoryClass::NetworkObjectFactoryClass(bool autoRegister) :
	next(0),
	prev(0)
{
	if (autoRegister)
		NetworkObjectFactoryMgrClass::Register_Factory(this);
}



NetworkObjectFactoryClass::~NetworkObjectFactoryClass()
{
	NetworkObjectFactoryMgrClass::Unregister_Factory(this);
}
