#include "General.h"
#include "NetworkManager.h"

#include "cNetwork.h"
#include "cConnection.h"
#include "cRemoteHost.h"
#include "cPacket.h"
#include "DlgMPConnecting.h"
#include "dialogmgr.h"
#include "resource.h"
#include "engine_game.h"
#include "engine_diagnostics.h"
#include "cGameType.h"
#include "DownloadManager.h"
#include "PackageDownloadTask.h"
#include "Package.h"
#include "PackageManager.h"
#include "cGameOptionsEvent.h"



/*

Definitions

Requested package: Package the clients wants to use, but is possibly not allowed to.
Game package: Package used during one game, a server package or allowed requested package.
Packages ready: The client already has all packages and sends this notification directly, or it has finished downloading.

*/



void ServerNetworkManager::onConnectionAccepted(cRemoteHost& remoteHost)
{
	if (remoteHost.getVersion() < 4.0f)
		remoteHost.setResourceManagerState(cRemoteHost::RESOURCE_MANAGER_STATE_INGAME);
	else
	{
		remoteHost.setResourceManagerState(cRemoteHost::RESOURCE_MANAGER_STATE_NEGOTIATING);
		
		cPacket packet(cPacket::TypeResourceManager);
		packet.Add(SC_PACKET_TYPE_RESET_GAME);
		packet.Add_Terminated_String(The_Game()->MapName);
		cNetwork::PServerConnection->sendPacket(packet, remoteHost.getId(), true);
	}
}



void ServerNetworkManager::onRequestedPackagesReceived(cRemoteHost& remoteHost)
{
	// Send game packages
	const SimpleDynVecClass<Package*>& packages = packageManager.getPackages();
	for (int index = 0; index < packages.Count(); ++index)
	{
		const Package& package = *packages[index];
		if (package.getIsActive())
		{
			cPacket packet(cPacket::TypeResourceManager);
			packet.Add(SC_PACKET_TYPE_GAME_PACKAGE);
			packet.Add(package.getId());
			packet.Add_Terminated_String(package.getDownloadUrl());
			cNetwork::PServerConnection->sendPacket(packet, remoteHost.getId(), true);
		}
	}

	cPacket packet(cPacket::TypeResourceManager);
	packet.Add(SC_PACKET_TYPE_GAME_PACKAGES_DONE);
	cNetwork::PServerConnection->sendPacket(packet, remoteHost, true);
}



void ServerNetworkManager::onGameEnd()
{
	cPacket packet(cPacket::TypeResourceManager);
	packet.Add(SC_PACKET_TYPE_RESET_GAME);
	
	cConnection* connection = cNetwork::PServerConnection;
	for (int remoteHostId = connection->Get_Min_RHost(); remoteHostId <= connection->Get_Max_RHost(); ++remoteHostId)
	{
		cRemoteHost* remoteHost = connection->Get_Remote_Host(remoteHostId);
		if (remoteHost && remoteHost->getVersion() >= 4.0f)
		{
			connection->sendPacket(packet, *connection->Get_Remote_Host(remoteHostId), true);
			remoteHost->setResourceManagerState(cRemoteHost::RESOURCE_MANAGER_STATE_NEGOTIATING);
		}
	}

	// TODO: Load new packages here
}



void ServerNetworkManager::onPacketReceived(cPacket& packet)
{
	TT_ASSERT(packet.Get_Type() == cPacket::TypeResourceManager);

	int packetType;
	packet.Get(packetType);

	cRemoteHost* remoteHost = cNetwork::Get_Server_Rhost(packet.Get_Sender_Id());
	if (!remoteHost)
	{
		return;
	}

	switch (packetType)
	{

	case CS_PACKET_TYPE_REQUESTED_PACKAGE:
		// TODO
		break;

	case CS_PACKET_TYPE_REQUESTED_PACKAGES_DONE:
		TT_ASSERT(remoteHost->getResourceManagerState() == cRemoteHost::RESOURCE_MANAGER_STATE_NEGOTIATING);
		remoteHost->setResourceManagerState(cRemoteHost::RESOURCE_MANAGER_STATE_ACQUIRING_PACKAGES);
		onRequestedPackagesReceived(*remoteHost);
		break;

	case CS_PACKET_TYPE_PACKAGES_LOADED:
		TT_ASSERT(remoteHost->getResourceManagerState() == cRemoteHost::RESOURCE_MANAGER_STATE_ACQUIRING_PACKAGES);
		remoteHost->setResourceManagerState(cRemoteHost::RESOURCE_MANAGER_STATE_INGAME);
		break;

	default:
		TT_UNREACHABLE;

	}
}



void ClientNetworkManager::onAcceptationReceived()
{
	if (IS_MULTIPLAY)
	{
		if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f && !cNetwork::I_Am_Server())
			setState(STATE_NEGOTIATING); // TODO: Actually pre-negotiating/idle.
		else
			setState(STATE_INGAME);
	}
}



void ClientNetworkManager::onGamePackagesReceived()
{
	setState(STATE_DOWNLOADING);
}



void ClientNetworkManager::setState(State _state)
{
	state = _state;
	
	DebugOutputString(__FUNCTION__ ": %d\n", state);
}



void ClientNetworkManager::think()
{
	if (state == STATE_DOWNLOADING && downloadManager.hasFinished())
	{
		setState(STATE_INGAME);

		DlgMPConnecting* dialog = (DlgMPConnecting*)DialogMgrClass::Find_Dialog(IDD_MULTIPLAY_CONNECTING);
		if (dialog)
			dialog->setGameData(The_Game());
		
		cPacket packet(cPacket::TypeResourceManager);
		packet.Add(CS_PACKET_TYPE_PACKAGES_LOADED);
		cNetwork::PClientConnection->sendPacket(packet, SERVER_HOST_ID, true);
	}
}



void ClientNetworkManager::onPacketReceived(cPacket& packet)
{
	TT_ASSERT(packet.Get_Type() == cPacket::TypeResourceManager);

	int packetType;
	packet.Get(packetType);

	switch (packetType)
	{

	case SC_PACKET_TYPE_RESET_GAME:
		{
			packet.Get_Terminated_String(The_Game()->MapName.Get_Buffer(256), 256);
			The_Game()->ModName = "";

			// TODO: Send requested packages
			
			cPacket packet2(cPacket::TypeResourceManager);
			packet2.Add(CS_PACKET_TYPE_REQUESTED_PACKAGES_DONE);
			cNetwork::PClientConnection->sendPacket(packet2, SERVER_HOST_ID, true);

			// TODO: downloadManager.removePending();
			setState(STATE_NEGOTIATING);
			break;
		}

	case SC_PACKET_TYPE_GAME_PACKAGE:
		{
			uint32 packageId;
			StringClass packageUrl;
			packet.Get(packageId);
			packet.Get_Terminated_String(packageUrl.Get_Buffer(256), 256);

			Package* package = packageManager.find(packageId);
			if (package)
				packageManager.activate(*package);
			else
				downloadManager.addTask(*new PackageDownloadTask(packageId, packageUrl));

			break;
		}

	case SC_PACKET_TYPE_GAME_PACKAGES_DONE:
		onGamePackagesReceived();
		break;

	default:
		TT_UNREACHABLE;

	}
}
