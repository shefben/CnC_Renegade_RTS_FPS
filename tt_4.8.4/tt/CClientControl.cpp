#include "General.h"
#include "CClientControl.h"



#include "CombatManager.h"
#include "NetworkObjectMgrClass.h"
#include "cConnection.h"
#include "bitstream.h"
#include "SmartGameObj.h"
#include "GameObjManager.h"
#include "cNetwork.h"



REF_DEF1(PClientControl, CClientControl*, 0x0085E11C);



CClientControl::CClientControl()
{
	clientId = -1;
	soldierId = -1;
	PacketType = 35;
}



void CClientControl::Init()
{
	clientId = cNetwork::Get_My_Id();
	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);
}



void CClientControl::Set_Update_Flag(int _soldierId)
{
	soldierId = _soldierId;
	if (soldierId != -1)
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_FREQUENT, true);
}



void CClientControl::Export_Creation(BitStreamClass& bitStream)
{
	bitStream.Add(clientId);
}



void CClientControl::Import_Creation(BitStreamClass& bitStream)
{
	bitStream.Get(clientId);
}



void CClientControl::Export_Frequent(BitStreamClass& bitStream)
{
	SmartGameObj* object;
	if (soldierId != -1)
	{
		object = GameObjManager::Find_SmartGameObj(soldierId);
		if (!object)
			soldierId = -1;
	}
	else
		object = NULL;

	bitStream.Add(soldierId);

	if (object)
	{
		object->Export_Control_Cs(bitStream);
		object->Export_State_Cs(bitStream);
	}

	soldierId = -1;
}



void CClientControl::Import_Frequent(BitStreamClass& bitStream)
{
	bitStream.Get(soldierId);
	if (soldierId != -1)
	{
		SmartGameObj* object = GameObjManager::Find_SmartGameObj(soldierId);

		if (object)
		{
			object->Import_Control_Cs(bitStream);
			object->Import_State_Cs(bitStream);
		}
	}
}



void CClientControl::Delete()
{
	delete this;
}
