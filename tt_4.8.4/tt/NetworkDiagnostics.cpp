#include "General.h"
#include "NetworkDiagnostics.h"



#if NET_DIAG


#include "Definition.h"
#include "ScriptableGameObj.h"
#include "NetworkObjectClass.h"
#include "cConnection.h"
#include "SysTimeClass.h"
#include "cNetwork.h"
#include "DebugManager.h"
#include <shlobj.h>


NetworkDiagnostics::NetworkDiagnostics()
{
	SetProcessAffinityMask(GetCurrentProcess(), 1);

	time_t directoryTime;
	time(&directoryTime);
	
	char path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	sprintf(directoryPath, "%s\\renegade-netdiag\\tt-%d",path, directoryTime);
	SHCreateDirectoryEx(NULL,directoryPath, NULL);

	memset(files, 0, sizeof(files));
}



NetworkDiagnostics::~NetworkDiagnostics()
{
	for (uint i = 0; i < countof(files); ++i)
	{
		if (files[i][false])
			fclose(files[i][false]);
		if (files[i][true])
			fclose(files[i][true]);
	}
}



FILE* NetworkDiagnostics::getLogFile(int clientId, bool isSender)
{
	if (!files[clientId][isSender])
	{
		time_t directoryTime;
		time(&directoryTime);

		uint myId = cNetwork::I_Am_Client() ? cNetwork::Get_My_Id() : SERVER_HOST_ID;
		
		char filePath[MAX_PATH];
		if (isSender)
			sprintf(filePath, "%s/netdiag[%d][%d]_%s.txt", directoryPath, myId, clientId, isSender ? "sent" : "recv");
		else
			sprintf(filePath, "%s/netdiag[%d][%d]_%s.txt", directoryPath, clientId, myId, isSender ? "sent" : "recv");

		files[clientId][isSender] = fopen(filePath, "ab");
	}
	return files[clientId][isSender];
}



uint64 NetworkDiagnostics::getSampleTime()
{
	return DebugTimer::getMicroTime();
}



void NetworkDiagnostics::onObjectUpdateReceived(NetworkObjectClass& object, int clientId)
{
	FILE* file = getLogFile(clientId, false);
	if (object.Get_Network_Class_ID() == 1000)
		fprintf(file, "%d\t%d\t%s\r\n", (uint32)getSampleTime(), object.Get_Network_ID(), ((BaseGameObj&)object).Get_Definition()->Get_Name());
	else
		fprintf(file, "%d\t%d\t%d\r\n", (uint32)getSampleTime(), object.Get_Network_ID(), object.Get_Network_Class_ID());
}



void NetworkDiagnostics::onObjectUpdateSent(NetworkObjectClass& object, int clientId)
{
	FILE* file = getLogFile(clientId, true);
	if (object.Get_Network_Class_ID() == 1000)
		fprintf(file, "%d\t%d\t%s\r\n", (uint32)getSampleTime(), object.Get_Network_ID(), ((BaseGameObj&)object).Get_Definition()->Get_Name());
	else
		fprintf(file, "%d\t%d\t%d\r\n", (uint32)getSampleTime(), object.Get_Network_ID(), object.Get_Network_Class_ID());
}



#endif
