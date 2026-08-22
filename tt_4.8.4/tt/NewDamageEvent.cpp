#include "General.h"
#include "NewDamageEvent.h"
#include "cPacket.h"
#include "NetworkObjectClassId.h"
#include "DefinitionMgrClass.h"
#include "GameObjManager.h"
#include "SmartGameObj.h"
#include "NetworkObjectMgrClass.h"
#include "BulletDataClass.h"
#include "tt.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "CombatManager.h"
#include "cNetwork.h"
#include "cConnection.h"
#include "cRemoteHost.h"
#include "WeaponMgr.h"
#include "OffenseObjectClass.h"
#include "C4GameObj.h"

uint NewDamageEvent::Get_Network_Class_ID() const
{
	return NET_NewDamageEvent;
}



void NewDamageEvent::Import_Creation(BitStreamClass& stream)
{
	cNetEvent::Import_Creation(stream);

	uint32 ammoDefinitionId = 0;
	uint32 damagerId = 0;
	uint32 damageeId = 0;

	stream.Get(ammoDefinitionId);
	stream.Get(damagerId);
	stream.Get(damageeId);
	//velocity = stream.readVector3();
	stream.Get_Terminated_String(collisionBone, sizeof(collisionBone));

	ammoDefinition = (AmmoDefinitionClass*)DefinitionMgrClass::Find_Definition(ammoDefinitionId, true);
	damager = GameObjManager::Find_PhysicalGameObj(damagerId);
	damagee = GameObjManager::Find_PhysicalGameObj(damageeId);

	if (ammoDefinition && damager && damagee)
	{
		SmartGameObj* smartDamager = damager ? damager->As_SmartGameObj() : NULL;
		if (smartDamager &&
			smartDamager->Get_Control_Owner() == ((cPacket&)stream).Get_Sender_Id() &&
			damagee->Get_Defense_Object()->canTrustClientDamage(smartDamager))

			Act();
	}
}



void NewDamageEvent::Export_Creation(BitStreamClass& stream)
{
	cNetEvent::Export_Creation(stream);
	
	stream.Add(ammoDefinition->Get_ID());
	stream.Add(damager ? damager->Get_Network_ID() : NULL);
	stream.Add(damagee ? damagee->Get_Network_ID() : NULL);
	//stream.write(velocity);
	stream.Add_Terminated_String(collisionBone);
}



void NewDamageEvent::Act()
{
	OffenseObjectClass offense(ammoDefinition->Damage, ammoDefinition->Warhead, damager->As_ArmedGameObj());
	offense.ForceServerDamage = true;
	
	damagee->Apply_Damage_Extended(offense, 1.f, velocity, collisionBone);
	if (damagee->As_C4GameObj())
	{
		ScriptableGameObj* attached = damagee->As_C4GameObj()->Get_Stuck_Object();
		
		if (attached)
		{
			PhysicalGameObj* attachedPhysical = attached->As_PhysicalGameObj();
			if (attachedPhysical)
				attachedPhysical->As_PhysicalGameObj()->Apply_Damage_Extended(offense, 1.f, velocity, NULL);
		}
	}
}



void NewDamageEvent::Init(AmmoDefinitionClass& _ammoDefinition, PhysicalGameObj* _damager, PhysicalGameObj& _damagee, const Vector3& _velocity, const char* _collisionBone)
{
	ammoDefinition = &_ammoDefinition;
	damager = _damager;
	damagee = &_damagee;
	velocity = _velocity;
	strncpy(collisionBone, _collisionBone, sizeof(collisionBone)-1);
	collisionBone[sizeof(collisionBone)-1] = '\0';

	Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
	Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);

	Set_Delete_Pending();
}



void NewDamageEvent::OnBulletCollisionOccurred(BulletDataClass* bulletData, PhysicalGameObj& damagee, const char* collisionBone)
{
	TT_ASSERT(cNetwork::I_Am_Server() || cNetwork::I_Am_Client());
		
	if (!cNetwork::I_Am_Server() && cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		if (bulletData->owner.Get_Ptr())
		{
			SmartGameObj* smartDamager = bulletData->owner.Get_Ptr()->As_SmartGameObj();
			if (smartDamager && smartDamager->Get_Control_Owner() == cNetwork::PClientConnection->Get_Local_Id() && damagee.Get_Defense_Object()->canTrustClientDamage(smartDamager))
				(new NewDamageEvent())->Init(*bulletData->ammoDefinition, smartDamager, damagee, bulletData->velocity, collisionBone);
		}
	}
}



SimpleNetworkObjectFactoryClass<NewDamageEvent, NET_NewDamageEvent> newDamageEventFactory;
