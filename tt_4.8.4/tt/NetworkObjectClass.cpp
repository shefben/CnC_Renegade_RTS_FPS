#include "General.h"

#include "NetworkObjectClass.h"
#include "SysTimeClass.h"
#include "NetworkObjectMgrClass.h"



REF_DEF2(NetworkObjectClass::IsServer, bool, 0x00854C64, 0x00853E4C);



NetworkObjectClass::NetworkObjectClass() :
	NetworkID(0), ImportStateCount(0), LastClientUpdate(0), LastReset(SysTimeClass::SystemTime.Get()),
	ClientUpdateCount(0), DeletePending(false), PacketType(0), LastDamaged(-1), LastDamager(-1),
	UpdateExportSize(0), CachedPriority(0), UnreliableOverride(false)
{  
	if (IsServer)
	{
		NetworkObjectMgrClass::Unregister_Object(this);
		NetworkID = NetworkObjectMgrClass::Get_New_Dynamic_ID();
		NetworkObjectMgrClass::Register_Object(this);
	}

	memset(CachedPriority2, 0, sizeof(CachedPriority2));

	Clear_Object_Dirty_Bits();
}



NetworkObjectClass::~NetworkObjectClass()
{
	NetworkObjectMgrClass::Unregister_Object(this);
}



void NetworkObjectClass::Clear_Object_Dirty_Bits()
{
	memset(DirtyBits, 0, sizeof(DirtyBits));
	memset(UpdateData, 0, sizeof(UpdateData));

	for (int clientId = 0; clientId < 128; ++clientId)
		UpdateData[clientId].updatePeriod = 50;
}



void NetworkObjectClass::Export_Creation(BitStreamClass& stream)
{
}



void NetworkObjectClass::Export_Frequent(BitStreamClass& stream)
{
}



void NetworkObjectClass::Export_Occasional(BitStreamClass& stream)
{
}



void NetworkObjectClass::Export_Rare(BitStreamClass& stream)
{
}



float NetworkObjectClass::Get_Cached_Priority()
{
	return CachedPriority;
}



void NetworkObjectClass::Get_Description(StringClass& description)
{
}



float NetworkObjectClass::Get_Filter_Distance()
{
	return 10000.f;
}



uint NetworkObjectClass::getNetworkId() const
{
	return NetworkID;
}



unsigned int NetworkObjectClass::Get_Network_Class_ID() const
{
	return 0;
}



bool NetworkObjectClass::Get_Object_Dirty_Bit(int clientId, DIRTY_BIT level)
{
	return (DirtyBits[clientId] & level) == level;
}



unsigned char NetworkObjectClass::Get_Object_Dirty_Bits(int clientId)
{
	return DirtyBits[clientId];
}



int NetworkObjectClass::Get_Vis_ID()
{
	return -1;
}



bool NetworkObjectClass::Get_World_Position(Vector3& position)
{
	return false;
}



void NetworkObjectClass::Import_Creation(BitStreamClass& stream)
{
}



void NetworkObjectClass::Import_Frequent(BitStreamClass& stream)
{
}



void NetworkObjectClass::Import_Occasional(BitStreamClass& stream)
{
}



void NetworkObjectClass::Import_Rare(BitStreamClass& stream)
{
}



bool NetworkObjectClass::Is_Tagged()
{
	return false;
}



void NetworkObjectClass::Network_Think()
{
}



void NetworkObjectClass::Set_Delete_Pending()
{
	if (!DeletePending)
	{
		DeletePending = true;
		NetworkObjectMgrClass::Register_Object_For_Deletion(this);
	}
}

void NetworkObjectClass::Set_Object_Dirty_Bit(DIRTY_BIT level, bool set)
{
	if (IsServer)
	{
		uint8 bits = (uint8)(set ? level : ~(uint8)level);

		if (set)
			for (int clientId = 1; clientId < 128; ++clientId)
				DirtyBits[clientId] |= bits;
		else
			for (int clientId = 1; clientId < 128; ++clientId)
				DirtyBits[clientId] &= bits;
	}
}



void NetworkObjectClass::Set_Object_Dirty_Bit(int clientId, DIRTY_BIT level, bool set)
{
	if (set)
		DirtyBits[clientId] |= level;
	else
		DirtyBits[clientId] &= ~level;
}



void NetworkObjectClass::Set_Object_Dirty_Bits(int clientId, unsigned char level)
{
	DirtyBits[clientId] = level;
}

bool NetworkObjectClass::Is_Client_Dirty(int clientId)
{
	return (DirtyBits[clientId] & ~DB_NOT_CREATED) != 0;
}



void NetworkObjectClass::Set_Last_Object_Id_I_Damaged(uint lastIdDamaged)
{
	LastDamaged = lastIdDamaged;
}



void NetworkObjectClass::Set_Last_Object_Id_I_Got_Damaged_By(uint lastIdDamagedBy)
{
	LastDamager = lastIdDamagedBy;
}



void NetworkObjectClass::Set_Network_ID(uint32 networkId)
{
	if (NetworkID)
		NetworkObjectMgrClass::Unregister_Object(this);

	NetworkID = networkId;

	if (networkId)
		NetworkObjectMgrClass::Register_Object(this);
}



bool NetworkObjectClass::Belongs_To_Client(int clientId)
{
	return NetworkID >  2109900000u + 100000u * clientId &&
	       NetworkID <= 2110000000u + 100000u * clientId;
}



int NetworkObjectClass::Get_Clientside_Update_Frequency()
{
	uint32 currentTime = TIMEGETTIME();
	
	int timeDifference = currentTime - LastReset;
	if (timeDifference > 10000)
	{
		if (ClientUpdateCount != 0)
		{
			ClientUpdateFrequency = timeDifference / ClientUpdateCount;
			LastReset = currentTime;
			ClientUpdateCount = 0;
		}
		else
			ClientUpdateFrequency = 10000;
	}
	
	return ClientUpdateFrequency;
}
