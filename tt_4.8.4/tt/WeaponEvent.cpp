#include "General.h"
#include "WeaponEvent.h"
#include "scripts.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "ArmedGameObj.h"
#include "WeaponBagClass.h"
#include "WeaponClass.h"
#include "GameObjManager.h"
WeaponEvent::WeaponEvent()
{
	Clip = 0;
	Inventory = 0;
	OwnerId = -1;
	Index = 0;
}

WeaponEvent::~WeaponEvent()
{
}

unsigned int WeaponEvent::Get_Network_Class_ID() const
{
	return NET_WeaponEvent;
}

void WeaponEvent::Init(int clip,int inventory,WeaponClass *weap)
{
	cNetEvent::Init();
	Clip = clip;
	Inventory = inventory;
	ArmedGameObj *obj = weap->Get_Owner();
	OwnerId = obj->Get_Network_ID();
	for (int i = 0;i < obj->Get_Weapon_Bag()->Get_Count();i++)
	{
		if (obj->Get_Weapon_Bag()->Peek_Weapon(i) == weap)
		{
			Index = i;
		}
	}
	Set_Object_Dirty_Bit(DB_CREATION,true);
}

void WeaponEvent::Import_Creation(BitStreamClass &oStream)
{
	cNetEvent::Import_Creation(oStream);
	oStream.Get(Clip);
	oStream.Get(Inventory);
	oStream.Get(OwnerId);
	oStream.Get(Index);
	if (!cNetwork::I_Am_Server())
	{
		Act();
	}
}

void WeaponEvent::Export_Creation(BitStreamClass &oStream)
{
	cNetEvent::Export_Creation(oStream);
	oStream.Add(Clip);
	oStream.Add(Inventory);
	oStream.Add(OwnerId);
	oStream.Add(Index);
	Set_Delete_Pending();
}

void WeaponEvent::Act()
{
	ArmedGameObj *obj = GameObjManager::Find_PhysicalGameObj(OwnerId)->As_ArmedGameObj();
	if (obj)
	{
		WeaponClass *weap = obj->Get_Weapon_Bag()->Peek_Weapon(Index);
		weap->Set_Clip_Rounds(Clip);
		weap->Set_Inventory_Rounds(Inventory);
	}
}

SimpleNetworkObjectFactoryClass<WeaponEvent, NET_WeaponEvent> weaponEventFactory;
