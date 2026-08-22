#ifndef TT_INCLUDE__NETWORKDIAGNOSTICS_H
#define TT_INCLUDE__NETWORKDIAGNOSTICS_H



#include "Singleton.h"



class NetworkObjectClass;



#define NET_DIAG 0



class NetworkDiagnostics
{

private:

	friend Singleton<NetworkDiagnostics>;

	char directoryPath[MAX_PATH];
	FILE* files[126][2];

	static uint64 getSampleTime();

	NetworkDiagnostics();
	~NetworkDiagnostics();
	FILE* getLogFile(int clientId, bool isSender);

public:

	void onObjectUpdateSent(NetworkObjectClass& object, int clientId);
	void onObjectUpdateReceived(NetworkObjectClass& object, int clientId);

};



#define networkDiagnostics (Singleton<NetworkDiagnostics>::getInstance())



#endif