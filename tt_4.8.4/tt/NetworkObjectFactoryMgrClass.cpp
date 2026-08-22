#include "General.h"
#include "NetworkObjectFactoryMgrClass.h"

#include "NetworkObjectFactoryClass.h"


class NetworkObjectClass;
class cPacket;


#pragma warning(disable: 4073) //warning C4073: initializers put in library initialization area - That's EXACTLY why I put that pragma in...
#pragma init_seg(lib) // Move this files static initializers up a level
#pragma warning(default: 4073)

REF_DEF2(NetworkObjectFactoryMgrClass::_FactoryListHead, NetworkObjectFactoryClass*, 0x00854FAC, 0x00854194);



// 0x00620830,,,
NetworkObjectFactoryClass* NetworkObjectFactoryMgrClass::Find_Factory(uint32 uClassID)
{
	for (NetworkObjectFactoryClass* oFactory = _FactoryListHead; oFactory; oFactory = oFactory->next)
		if (oFactory->Get_Class_ID() == uClassID)
			return oFactory;

	return 0;
}



// 0x00620860,,,
NetworkObjectFactoryClass* NetworkObjectFactoryMgrClass::Get_First()
{
	return _FactoryListHead;
}



// 0x00620870,,,
NetworkObjectFactoryClass* NetworkObjectFactoryMgrClass::Get_Next(NetworkObjectFactoryClass* oFactory)
{
	return oFactory ? oFactory->next : 0;
}



// 0x006208E0,,,
void NetworkObjectFactoryMgrClass::Link_Factory(NetworkObjectFactoryClass* oFactory)
{
	NetworkObjectFactoryMgrClass::Register_Factory (oFactory);
}



// 0x00620880,,,
void NetworkObjectFactoryMgrClass::Register_Factory(NetworkObjectFactoryClass* oFactory)
{
	TT_ASSERT(Exe != 6); // If we hit this, we had better be init'd already or bad things will happen.

	oFactory->next = _FactoryListHead;
	if (oFactory->next)
		oFactory->next->prev = oFactory;

	_FactoryListHead = oFactory;
}



void NetworkObjectFactoryMgrClass::Unregister(uint32 classId)
{
	NetworkObjectFactoryMgrClass::Unregister_Factory(NetworkObjectFactoryMgrClass::Find_Factory(classId));
}



// 0x006208A0,,,
void NetworkObjectFactoryMgrClass::Unregister_Factory(NetworkObjectFactoryClass* oFactory)
{
	if (!oFactory)
		return;

	if (!oFactory->prev)
		_FactoryListHead = oFactory->next;
	else
		oFactory->prev->next = oFactory->next;

	if (oFactory->next)
		oFactory->next->prev = oFactory->prev;

	oFactory->next = 0;
	oFactory->prev = 0;
}



// 0x00620900,,,
void NetworkObjectFactoryMgrClass::Unlink_Factory(NetworkObjectFactoryClass* oFactory)
{
	NetworkObjectFactoryMgrClass::Unregister_Factory(oFactory);
}
