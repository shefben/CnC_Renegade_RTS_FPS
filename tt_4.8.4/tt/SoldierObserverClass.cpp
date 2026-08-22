#include "General.h"
#include "SoldierObserverClass.h"
#include "wwmath.h"
SoldierObserverClass::SoldierObserverClass() : unk10(0), unk14(0), HomeFacing(9999999), ActTime(0), Cover(0), CoverPosition(0), unk54(0), Aggressiveness(0.5), TakeCoverProbability(0.5), IsStationary(false), State(0), Weapon(0), unk50(((WWMath::Random_Float() * 9.0f) + 1.0f))
{
}
SoldierObserverClass::~SoldierObserverClass()
{
	Destroyed(0);
	Reference = 0;
}
RENEGADE_FUNCTION
void SoldierObserverClass::Detach(GameObject *obj)
AT2(0x006C2390,0x006C1C30);
RENEGADE_FUNCTION
void SoldierObserverClass::Created(GameObject *obj)
AT2(0x006C23E0,0x006C1C80);
RENEGADE_FUNCTION
void SoldierObserverClass::Destroyed(GameObject *obj)
AT2(0x006C24C0,0x006C1D60);
RENEGADE_FUNCTION
void SoldierObserverClass::Damaged(GameObject *obj,GameObject *damager,float amount)
AT2(0x006C2540,0x006C1DE0);
RENEGADE_FUNCTION
void SoldierObserverClass::Sound_Heard(GameObject *obj,const CombatSound & sound)
AT2(0x006C26F0,0x006C1F90);
RENEGADE_FUNCTION
void SoldierObserverClass::Enemy_Seen(GameObject *obj,GameObject *enemy)
AT2(0x006C2830,0x006C20D0);
RENEGADE_FUNCTION
void SoldierObserverClass::Action_Complete(GameObject *obj,int action_id,ActionCompleteReason complete_reason)
AT2(0x006C2990,0x006C2230);
RENEGADE_FUNCTION
void SoldierObserverClass::Timer_Expired(GameObject *obj,int number)
AT2(0x006C24E0,0x006C1D80);
RENEGADE_FUNCTION
void SoldierObserverClass::Poked(GameObject *obj,GameObject *poker)
AT2(0x006C2F40,0x006C27E0);
RENEGADE_FUNCTION
const PersistFactoryClass &SoldierObserverClass::Get_Factory  () const
AT2(0x006C1FF0,0x006C1890);
RENEGADE_FUNCTION
bool SoldierObserverClass::Save(ChunkSaveClass &csave)
AT2(0x006C2000,0x006C18A0);
RENEGADE_FUNCTION
bool SoldierObserverClass::Load(ChunkLoadClass &cload)
AT2(0x006C2210,0x006C1AB0);
