#ifndef TT_INCLUDE__GAMECOMBATNETWORKHANDLERCLASS_H
#define TT_INCLUDE__GAMECOMBATNETWORKHANDLERCLASS_H



class ArmedGameObj;
class PhysicalGameObj;
class SoldierGameObj;



class GameCombatNetworkHandlerClass : public CombatNetworkHandlerClass
{

public:

	virtual bool Can_Damage(ArmedGameObj*, PhysicalGameObj*);
	virtual float Get_Damage_Factor(ArmedGameObj*, PhysicalGameObj*);
	virtual void On_Soldier_Kill(SoldierGameObj*, SoldierGameObj*);
	virtual void On_Soldier_Death(SoldierGameObj*);
	virtual bool Is_Gameplay_Permitted();

};



#endif