#ifndef TT_INCLUDE__NETWORKMANAGER_H
#define TT_INCLUDE__NETWORKMANAGER_H



#include "Singleton.h"
#include "cNetwork.h"



class cRemoteHost;
class cPacket;



class NetworkManager
{

protected:

	enum ScPacketType
	{
		SC_PACKET_TYPE_RESET_GAME,
		SC_PACKET_TYPE_GAME_PACKAGE,
		SC_PACKET_TYPE_GAME_PACKAGES_DONE,
		SC_PACKET_TYPE_COUNT,
	};

	enum CsPacketType
	{
		CS_PACKET_TYPE_REQUESTED_PACKAGE,
		CS_PACKET_TYPE_REQUESTED_PACKAGES_DONE,
		CS_PACKET_TYPE_PACKAGES_LOADED,
		CS_PACKET_TYPE_COUNT,
	};


public:

	virtual void onPacketReceived(cPacket& packet) = 0;

};



class ServerNetworkManager :
	public NetworkManager
{

public:

	void onConnectionAccepted(cRemoteHost& remoteHost);
	void onRequestedPackagesReceived(cRemoteHost& remoteHost);
	void onPackagesAvailable(cRemoteHost& remoteHost);
	void onGameEnd();

	virtual void onPacketReceived(cPacket& packet);

};



class ClientNetworkManager :
	public NetworkManager
{

	enum State
	{
		STATE_NEGOTIATING,
		STATE_DOWNLOADING,
		STATE_INGAME,
		STATE_COUNT,
	};

	State state;

public:

	void setState(State newState);
	void think();
	void onAcceptationReceived();
	void onGamePackagesReceived();

	virtual void onPacketReceived(cPacket& packet);

};



#define clientNetworkManager (Singleton<ClientNetworkManager>::getInstance())
#define serverNetworkManager (Singleton<ServerNetworkManager>::getInstance())

NetworkManager& Singleton<NetworkManager>::getInstance()
{
	if (cNetwork::I_Am_Server())
	{
		return serverNetworkManager;
	}
	else
	{
		TT_ASSERT(cNetwork::I_Am_Client());
		return clientNetworkManager;
	}
}

#define networkManager (Singleton<NetworkManager>::getInstance())



#endif
