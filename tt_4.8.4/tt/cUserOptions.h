#ifndef TT_INCLUDE__CUSEROPTIONS_H
#define TT_INCLUDE__CUSEROPTIONS_H



#include "cRegistryInt.h"



enum BANDWIDTH_TYPE_ENUM;



class cUserOptions
{

public:

	static REF_DECL2(ShowNamesOnSoldier, bool);
	static REF_DECL2(SkipQuitConfirmDialog, bool);
	static REF_DECL2(SkipIngameQuitConfirmDialog, bool);
	static REF_DECL2(CameraLockedToTurret, bool);
	static REF_DECL2(PermitDiagLogging, bool);
	static REF_DECL2(Sku, int);
	static REF_DECL2(BandwidthType, int);
	static REF_DECL2(BandwidthBps, int);
	static REF_DECL2(GameSpyBandwidthType, int);
	static REF_DECL2(PreferredGameSpyNic, int);
	static REF_DECL2(GameSpyQueryPort, int);
	static REF_DECL2(GameSpyGamePort, int);
	static REF_DECL2(SplashCount, int);
	static REF_DECL2(DoneClientBandwidthTest, bool);
	static REF_DECL2(PreferredLanNic, int);
	static REF_DECL2(NetUpdateRate, int);
	static REF_DECL2(ClientHintFactor, float);
	static REF_DECL2(MaxFacingPenalty, float);
	static REF_DECL2(IrrelevancePenalty, float);
	static REF_DECL2(ResultsLogNumber, cRegistryInt);

	static bool Parse_Command_Line(const char*);
	static void Set_Server_INI_File(char*);
	static void Set_Bandwidth_Type(BANDWIDTH_TYPE_ENUM);
	static BANDWIDTH_TYPE_ENUM Get_Bandwidth_Type();
	static void Set_Bandwidth_Bps(int);
	static void Reread();

};



#endif
