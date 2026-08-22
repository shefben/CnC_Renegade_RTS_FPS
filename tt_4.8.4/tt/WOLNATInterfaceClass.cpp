#include "General.h"
#include "WOLNATInterfaceClass.h"



#include "cPacket.h"
#include "PacketManagerClass.h"
#include "SocketHandlerClass.h"
#include "IPAddressClass.h"
#include "cNetwork.h"
#include "cConnection.h"



RENEGADE_FUNCTION
void WOLNATInterfaceClass::Set_Server(bool _isServer)
AT2(0x0046BB60, 0x0046B490);



RENEGADE_FUNCTION
void WOLNATInterfaceClass::Intercept_Game_Packet(cPacket&)
AT2(0x0046CF70, 0x0046C8A0);



void WOLNATInterfaceClass::Service_Receive_Queue(SocketHandlerClass* socketHandler)
{
	WaitForSingleObject(mutex, 10000);
	if (!unk007C)
	{
		cPacket packet1;
		cPacket packet2;
		Socket socket(socketHandler->socket);

		Address address;
		int packetSize = PacketManager().Get_Packet(socket, (byte*)packet2.Get_Data(), packet2.Get_Buffer_Size(), (Ip&)address.sin_addr.s_addr, address.sin_port);
		if (packetSize > 0)
		{
			packet2.setAddress(address);
			packet2.Set_Bit_Write_Position(8 * packetSize);
			cPacket::Construct_App_Packet(packet1, packet2);
			WOLNATInterfaceClass::Intercept_Game_Packet(packet1);
		}
	}

	ReleaseMutex(mutex);
}


BOOL WOLNATInterfaceClass::Send_Game_Format_Packet_To(IPAddressClass* ipAddress, char* playerName, int, SocketHandlerClass* socketHandler)
{
	WaitForSingleObject(mutex, 10000);

	cPacket packet;
	packet.Set_Type(cPacket::TypeFirewallProbe);
	packet.Set_Id(0);
	packet.Add_Terminated_String(playerName);
	
	Address address = ipAddress->toAddress();
	
	if (unk007C)
	{
		cNetwork::PServerConnection->Send_Packet_To_Address(packet, address);
	}
	else
	{
		cPacket fullPacket;
		cPacket::Construct_Full_Packet(fullPacket, packet);
		Socket socket(socketHandler->socket);
		PacketManager().Take_Packet((byte*)fullPacket.Get_Data(), fullPacket.Get_Compressed_Size_Bytes(), (Ip&)address.sin_addr.s_addr, address.sin_port, socket);
		PacketManager().Flush(true);
	}

	ReleaseMutex(mutex);

	return true;
}




REF_DEF2(WOLNATInterface, WOLNATInterfaceClass, 0x0082C5A8, 0x0082B790);