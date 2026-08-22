#ifndef TT_INCLUDE__PINGPROFILEWAIT_H
#define TT_INCLUDE__PINGPROFILEWAIT_H



#include "WWOnline.h"
#include "SingleWait.h"
#include "notify.h"

#include "RefPtr.h"



class PingProfileWait :
	public SingleWait, // 0000
	public Observer<WWOnline::RawPing> // 001C
{

	static uint8 pings[8];
	
	RefPtr<WWOnline::Session> session; // 0030
	int pendingPings; // 0038

public:

	static bool updatePings(RefPtr<WWOnline::Session>& session);
	static uint8* getPings() { return pings; }
	
	static RefPtr<PingProfileWait> Create() { return RefPtr<PingProfileWait>::create(); } 
	
	PingProfileWait();
	void WaitBeginning();
	void HandleNotification(WWOnline::RawPing& ping);

}; // 003C



#endif