#ifndef TT_INCLUDE__COMBATNETWORKRECEIVER_H
#define TT_INCLUDE__COMBATNETWORKRECEIVER_H



class Vector3;



class CombatNetworkReceiver
{

public:

	virtual bool Server_Update_Dynamic_Objects(bool isUrgent) = 0;
	virtual void Server_Send_Delete_Notifications() = 0;
	virtual bool Client_Update_Dynamic_Objects(bool isUrgent) = 0;

	void Print(const char*, ...);
	void Print(const Vector3&, const char*, ...);

};



#endif