#ifndef TT_INCLUDE__DAMAGEABLESTATICPHYSCLASS_H
#define TT_INCLUDE__DAMAGEABLESTATICPHYSCLASS_H



#include "StaticPhysClass.h"



class DamageableStaticPhysDefClass;
class OffenseObjectClass;



class DamageableStaticPhysClass :
	public StaticPhysClass
{

public:

	virtual ~DamageableStaticPhysClass();
	virtual void Timestep(float);
	virtual DamageableStaticPhysClass* As_DamageableStaticPhysClass() {return this;}
	virtual bool Save(ChunkSaveClass&);
	virtual bool Load(ChunkLoadClass&);
	virtual PersistFactoryClass& Get_Factory() const;

	DamageableStaticPhysClass();
	UNK Init(const DamageableStaticPhysDefClass&);
	void Apply_Damage_Static(const OffenseObjectClass&);
	UNK Reset_Health();
	UNK Start_Loop();
	UNK Play_Twitch();
	UNK Play_Death_Transition();
	UNK Get_DamageableStaticPhysDef();

};



#endif