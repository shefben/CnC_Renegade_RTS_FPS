#include "General.h"
#include "NetworkObjectMgrClass.h"



REF_DEF2(NetworkObjectMgrClass::_DeletePendingList, DynamicVectorClass<NetworkObjectClass*>, 0x00854658, 0x00853840);
REF_DEF2(NetworkObjectMgrClass::_ObjectList, DynamicVectorClass<NetworkObjectClass*>, 0x00854640, 0x00853828);
REF_DEF2(NetworkObjectMgrClass::_NewDynamicID, uint, 0x0080EB88, 0x0080DD60);
REF_DEF2(NetworkObjectMgrClass::_NewClientID, uint, 0x00854674, 0x0085385C);
REF_DEF2(NetworkObjectMgrClass::_IsLevelLoading, bool, 0x00854678, 0x00853860);



void NetworkObjectMgrClass::Unregister_Object(NetworkObjectClass* object)
{
	uint32 maxIndex;
	if (!object->Get_Network_ID() || !Find_Object(object->Get_Network_ID(), maxIndex))
		return;

	_ObjectList.Delete(maxIndex);
}



void NetworkObjectMgrClass::Register_Object_For_Deletion(NetworkObjectClass* object)
{
	if (_DeletePendingList.ID(object) != -1)
		return;

	_DeletePendingList.Add(object);
}



void NetworkObjectMgrClass::Register_Object(NetworkObjectClass* object)
{
	if (!object->Get_Network_ID())
		return;
	uint32 maxIndex;
	if (!Find_Object(object->Get_Network_ID(), maxIndex))
		_ObjectList.Insert(maxIndex, object);
}



int NetworkObjectMgrClass::Get_New_Client_ID()
{
	return _NewClientID++;
}



int NetworkObjectMgrClass::Get_New_Dynamic_ID()
{
	while (Find_Object(_NewDynamicID))
		++_NewDynamicID;
	return _NewDynamicID++;
}



NetworkObjectClass* NetworkObjectMgrClass::Find_Object(uint networkId)
{
	uint maxIndex;
	if (!Find_Object(networkId, maxIndex))
		return 0;

	return _ObjectList[maxIndex];
}



bool NetworkObjectMgrClass::Find_Object(uint networkId, uint& maxIndex)
{
	maxIndex = _ObjectList.Count();
	for (uint i = 0; i < maxIndex; ++i)
	{
		uint id = _ObjectList[i]->Get_Network_ID();

		if (id >= networkId)
		{
			maxIndex = i;
			return id == networkId;
		}
	}

	return false;
}



void NetworkObjectMgrClass::Init_New_Client_ID(int arg)
{
	_NewClientID = 2109900001 + 100000 * arg;
}



RENEGADE_FUNCTION
void NetworkObjectMgrClass::Delete_Pending()
AT2(0x00617E10, 0x006176B0);



void NetworkObjectMgrClass::Delete_Client_Objects(int clientId)
{
	for (int i = 0; i < _ObjectList.Count(); ++i)
	{
		NetworkObjectClass& object = *_ObjectList[i];
		if (object.Belongs_To_Client(clientId))
			object.Set_Delete_Pending();
	}
}




void NetworkObjectMgrClass::Restore_Dirty_Bits(int clientId)
{
	for (int i = 0; i < _ObjectList.Count(); ++i)
	{
		NetworkObjectClass& object = *_ObjectList[i];
		object.Set_Object_Dirty_Bits(clientId, object.Get_Object_Dirty_Bits(127));
	}
}
