#include "General.h"
#include "PlayerDataClass.h"
#include "BitStream.h"
#include "CombatManager.h"

PlayerDataClass::PlayerDataClass() :
	Score(0), Money(0), WeaponsFired(), WeaponsFired2()
{
	PunishTimer = 0;
	Stats_Reset();
}

void PlayerDataClass::Export_Occasional(BitStreamClass& stream)
{
	stream.Add(Score);
	stream.Add(Money);
}

void PlayerDataClass::Stats_Reset()
{
	GameTime = 0;
	SessionTime = 0;
	EnemiesKilled = 0;
	AlliesKilled = 0;
	ShotsFired = 0;
	HeadShots = 0;
	TorsoShots = 0;
	ArmShots = 0;
	LegShots = 0;
	CrotchShots = 0;
	FinalHealth = 0;
	PowerupsCollected = 0;
	VehiclesDestroyed = 0;
	VehicleTime = 0;
	KillsFromVehicle = 0;
	Squishes = 0;
	CreditGrant = 0;
	BuildingDestroyed = 0;
	HeadHit = 0;
	TorsoHit = 0;
	ArmHit = 0;
	LegHit = 0;
	CrotchHit = 0;
	WeaponFired = -1;
	WeaponsFired.Delete_All();
	WeaponsFired2.Delete_All();
}

bool PlayerDataClass::Purchase_Item(int money)
{
	if (money >= 0)
	{
		float m = Money;
		if (money < m)
		{
			Increment_Money((float)-money);
			return true;
		}
	}
	return false;
}

void PlayerDataClass::Apply_Damage_Points(float points, DamageableGameObj* obj)
{
	Increment_Score(points);
}

void PlayerDataClass::Apply_Death_Points(float points, DamageableGameObj* obj)
{
	Increment_Score(points);
}

PlayerDataClass::~PlayerDataClass()
{
}

void PlayerDataClass::Reset_Player()
{
	Stats_Reset();
	Set_Score (0);
	Set_Money (0);
}

void PlayerDataClass::Set_Money(float money)
{
	Money = money;
}

void PlayerDataClass::Set_Score(float score)
{
	Score = score;
}

void PlayerDataClass::Increment_Money (float fMoney)
{
	if (!CombatManager::Is_Gameplay_Permitted())
		return;

	Set_Money (Money + fMoney);

	if (fMoney > 0)
		CreditGrant += fMoney;
}

void PlayerDataClass::Increment_Score(float fScore)
{
	if (!CombatManager::Is_Gameplay_Permitted())
		return;

	Set_Score(Score + fScore);
	if (fScore > 0)
		Increment_Money(fScore);
}

void PlayerDataClass::Import_Occasional(BitStreamClass& stream)
{
	float temp;
	stream.Get(temp);
	Score = temp;
	stream.Get(temp);
	Money = temp;
}

RENEGADE_FUNCTION
void PlayerDataClass::Stats_Add_Weapon_Fired(int weapon)
AT2(0x006D3AB0,0x006D3350);
