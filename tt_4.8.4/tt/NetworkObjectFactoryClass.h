#ifndef TT_INCLUDE__NETWORKOBJECTFACTORYCLASS_H
#define TT_INCLUDE__NETWORKOBJECTFACTORYCLASS_H
class NetworkObjectClass;
#include "cPacket.h"


class NetworkObjectFactoryClass
{

public:

	NetworkObjectFactoryClass* next; // 0004  0000
	NetworkObjectFactoryClass* prev; // 0008  0004

	NetworkObjectFactoryClass(bool autoRegister = true);
	virtual ~NetworkObjectFactoryClass();

	virtual NetworkObjectClass* Create(cPacket& packet) const = 0;
	virtual void Prep_Packet(NetworkObjectClass* object, cPacket& packet) {}
	virtual uint32 Get_Class_ID() const = 0;

}; // 000C


#endif
