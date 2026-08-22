#ifndef TT_INCLUDE__BULLETDATACLASS_H
#define TT_INCLUDE__BULLETDATACLASS_H

#include "ReferencerClass.h"
#include "Vector3.h"



class AmmoDefinitionClass;
class CollisionEventClass;



class BulletDataClass
{

public:

	AmmoDefinitionClass* ammoDefinition;    //  0
	ReferencerClass      owner;             //  4
	Vector3              position;          // 20
	Vector3              velocity;          // 32
	sint32               softPierceCount;   // 44
	bool                 b48;               // 48 - got something to do with expired bullets
	bool                 b49;               // 49 - got something to do with expired bullets / shattering
	uint                 lastColideeId;     // 52 - networkId of last collided object; used for soft piercing
	sint32               grenadeSafetyTime; // 56


	uint32 Bullet_Collision_Occurred (const CollisionEventClass& collisionEvent);
	uint32 Bullet_Expired            ();

}; // 60
#endif
