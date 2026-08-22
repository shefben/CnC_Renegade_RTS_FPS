#ifndef TT_INCLUDE__NEWDAMAGEEVENT_H
#define TT_INCLUDE__NEWDAMAGEEVENT_H

#include "cNetEvent.h"
#include "Vector3.h"
#include "ReferencerClass.h"



class AmmoDefinitionClass;
class PhysicalGameObj;
class BulletDataClass;
class SmartGameObj;
class DamageableGameObj;
class OffenseObjectClass;


class NewDamageEvent :
	public cNetEvent
{

private:

	AmmoDefinitionClass* ammoDefinition;
	PhysicalGameObj* damager;
	PhysicalGameObj* damagee;
	Vector3 velocity; // TODO: Is it useful to send this over network?
	char collisionBone[64]; // TODO: What's the max length for a bone? Is there a better way to send the bone over network?


public:
	
	virtual unsigned int Get_Network_Class_ID() const;
	virtual void Import_Creation (BitStreamClass& stream);
	virtual void Export_Creation (BitStreamClass& stream);
	virtual void Act();
	
	void Init(AmmoDefinitionClass& _ammoDefinition, PhysicalGameObj* _damager, PhysicalGameObj& _damagee, const Vector3& _velocity, const char* _collisionBone);
	
	static void OnBulletCollisionOccurred(BulletDataClass* bulletData, PhysicalGameObj& damagee, const char* collisionBone);

};
#endif
