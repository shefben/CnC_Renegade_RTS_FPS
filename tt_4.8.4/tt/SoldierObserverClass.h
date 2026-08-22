#ifndef TT_INCLUDE__SOLDIEROBSERVERCLASS_H
#define TT_INCLUDE__SOLDIEROBSERVERCLASS_H



#include "PersistentGameObjObserverClass.h"
#include "ReferencerClass.h"
#include "engine_string.h"
class CoverEntryClass;
class SoldierObserverClass :
	public PersistentGameObjObserverClass
{
	int unk10; //10
	float unk14; //14
	Vector3 HomeLocation; //18 1C 20
	float HomeFacing; //24
	Vector3 unk28; //28 2C 30
	float ActTime; //34
	ReferencerClass Reference; //38 3C 40 44
	bool Cover; //48
	CoverEntryClass *CoverPosition; //4C
	float unk50; //50
	bool unk54; //54
	float Aggressiveness; //58
	float TakeCoverProbability; //5C
	bool IsStationary; //60
	StringClass StateStr; //64
	int State; //68
	int Weapon; //6C
public:
	SoldierObserverClass();
	~SoldierObserverClass();
	const PersistFactoryClass& Get_Factory  () const;
	bool Save(ChunkSaveClass &csave);
	bool Load(ChunkLoadClass &cload);
	virtual const char *Get_Name()
	{
		return "Innate Soldier";
	}
	virtual void Attach(GameObject *newobj)
	{
	}
	virtual void Detach(GameObject *obj);
	virtual void Created(GameObject *obj);
	virtual void Destroyed(GameObject *obj);
	virtual void Killed(GameObject *obj,GameObject *killer)
	{
	}
	virtual void Damaged(GameObject *obj,GameObject *damager,float amount);
	virtual void Custom(GameObject *obj,int type,int param,GameObject *sender)
	{
	}
	virtual void Sound_Heard(GameObject *obj,const CombatSound & sound);
	virtual void Enemy_Seen(GameObject *obj,GameObject *enemy);
	virtual void Action_Complete(GameObject *obj,int action_id,ActionCompleteReason complete_reason);
	virtual void Timer_Expired(GameObject *obj,int number);
	virtual void Animation_Complete(GameObject *obj,const char *animation_name)
	{
	}
	virtual void Poked(GameObject *obj,GameObject *poker);
	virtual void Entered(GameObject *obj,GameObject *enterer)
	{
	}
	virtual void Exited(GameObject *obj,GameObject *exiter)
	{
	}
}; // 0070

#endif