#ifndef TT_INCLUDE__COMBATNETWORKRECEIVERINSTANCECLASS_H
#define TT_INCLUDE__COMBATNETWORKRECEIVERINSTANCECLASS_H



#include "CombatNetworkReceiver.h"



class CombatNetworkReceiverInstanceClass :
	public CombatNetworkReceiver
{

public:

	virtual bool Server_Update_Dynamic_Objects(bool isUrgent);
	virtual void Server_Send_Delete_Notifications();
	virtual bool Client_Update_Dynamic_Objects(bool isUrgent);

	void Print(const char*, ...);
	void Print(const Vector3&, const char*, ...);

};



#endif