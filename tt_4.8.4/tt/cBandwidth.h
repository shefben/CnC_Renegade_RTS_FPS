#ifndef TT_INCLUDE__CBANDWIDTH_H
#define TT_INCLUDE__CBANDWIDTH_H



enum BANDWIDTH_TYPE_ENUM
{
	BANDWIDTH_MODEM_288 = 100,
	BANDWIDTH_MODEM_336 = 101,
	BANDWIDTH_MODEM_56  = 102,
	BANDWIDTH_ISDN      = 103,
	BANDWIDTH_CABLE     = 104,
	BANDWIDTH_LANT1     = 105,
	BANDWIDTH_AUTO      = 106,
	BANDWIDTH_CUSTOM    = 107, // default
};



class cBandwidth
{

public:

	static uint Get_Bandwidth_Bps_From_Type(BANDWIDTH_TYPE_ENUM bandwidthType);
	static const wchar_t* Get_Bandwidth_String_From_Type(BANDWIDTH_TYPE_ENUM bandwidthType);
	static BANDWIDTH_TYPE_ENUM Get_Bandwidth_Type_From_String(const char* bandwidthString);

};



#endif