#include "General.h"
#include "cSbboManager.h"

#include "cNetwork.h"
#include "cConnection.h"




float cSbboManager::AccumTimeSNetUpdate = 0.f;
float cSbboManager::AccumTimeSCombatThink = 0.f;
float cSbboManager::NetToCombatRatio = 0.f;
int cSbboManager::PoorRatios = 0;
int cSbboManager::SlowSamples = 0;
bool cSbboManager::IsEnabled = true;



void cSbboManager::Reset()
{
	AccumTimeSNetUpdate = 0.f;
	AccumTimeSCombatThink = 0.f;
	NetToCombatRatio = 0.f;
	PoorRatios = 0;
	SlowSamples = 0;
}



void cSbboManager::Think()
{
	if (IsEnabled)
	{
		if (AccumTimeSCombatThink > 0)
		{
			if (AccumTimeSNetUpdate + AccumTimeSCombatThink > .5f)
			{
				NetToCombatRatio = AccumTimeSNetUpdate / AccumTimeSCombatThink;
				AccumTimeSNetUpdate = 0;
				AccumTimeSCombatThink = 0;
				
				if (NetToCombatRatio > 5.f)
					++PoorRatios;
				else
					PoorRatios = 0;
				//TT_ASSERT(PoorRatios == 0); // Tell StealthEye if you hit this; you can remove and continue safely.
				
				if (cNetwork::Fps < 20)
					++SlowSamples;
				else
					SlowSamples = 0;
				
				if (SlowSamples >= 10 && PoorRatios >= 10)
				{
					if (cNetwork::PServerConnection->Get_Bandwidth_Budget_Out() >= 64000)
					{
						// StealthEye thinks this method of limiting bandwidth is pointless, don't want to rely on time measurements of functions really. This is just here to see whether it could have had a bad effect or whether it's just hardly reachable.
						TT_INTERRUPT;

						cNetwork::PServerConnection->Set_Bandwidth_Budget_Out(cNetwork::PServerConnection->Get_Bandwidth_Budget_Out() * 9 / 10);
						SlowSamples = 0;
						PoorRatios = 0;
					}
				}
			}
		}
	}
}



void cSbboManager::Increment_Accum_Time_S_Net_Update(float amount)
{
	AccumTimeSNetUpdate += amount;
}



void cSbboManager::Increment_Accum_Time_S_Combat_Think(float amount)
{
	AccumTimeSCombatThink += amount;
}



bool cSbboManager::Toggle_Is_Enabled()
{
	IsEnabled = !IsEnabled;
	Reset();
	return IsEnabled;
}
