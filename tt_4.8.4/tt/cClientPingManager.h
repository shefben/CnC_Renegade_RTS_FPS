#ifndef TT_INCLUDE__CCLIENTPINGMANAGER_H
#define TT_INCLUDE__CCLIENTPINGMANAGER_H



class cClientPingManager
{

public:

	static REF_DECL2(PingNumber, int);
	static REF_DECL2(TimeSentMs, int);
	static REF_DECL2(LastRoundTripPingMs, int);
	static REF_DECL2(AvgRoundTripPingMs, int);
	static REF_DECL2(IsAwaitingResponse, bool);
	static REF_DECL2(RoundTripPingSamplesMs, int);
	
	static void Init();
	static void Think();
	static int Get_Last_Round_Trip_Ping_Ms();
	static int Get_Avg_Round_Trip_Ping_Ms();
	static void Compute_Average_Round_Trip_Ping_Ms();
	static void Response_Received(int);

};



#endif