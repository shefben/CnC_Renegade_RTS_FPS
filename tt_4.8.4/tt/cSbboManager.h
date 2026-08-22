#ifndef TT_INCLUDE__CSBBOMANAGER_H
#define TT_INCLUDE__CSBBOMANAGER_H



class cSbboManager
{

private:
	
	static float AccumTimeSNetUpdate;
	static float AccumTimeSCombatThink;
	static float NetToCombatRatio;
	static int PoorRatios;
	static int SlowSamples;
	static bool IsEnabled;

public:

	static void Reset();
	static void Think();
	static void Increment_Accum_Time_S_Net_Update(float amount);
	static void Increment_Accum_Time_S_Combat_Think(float amount);
	static float Get_Net_To_Combat_Ratio() { return NetToCombatRatio; }
	static bool Toggle_Is_Enabled();

};



#endif