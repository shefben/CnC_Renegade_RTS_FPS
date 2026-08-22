#include "General.h"
#include "PingProfileWait.h"

#include "TranslateDBClass.h"
#include "string_ids.h"



uint8 PingProfileWait::pings[8];



bool PingProfileWait::updatePings(RefPtr<WWOnline::Session>& session)
{
	memset(pings, 0xFF, 8);
	
	if (!session || session->pingServers.empty())
		return false;
	
	int pingIndex = 0;
	for (RefPtr<WWOnline::PingServerData>* pingServer = session->pingServers.begin(); pingServer != session->pingServers.end(); ++pingServer)
	{
		int ping = (*pingServer)->ping;
		if (ping < 0)
			return false;
		
		pings[++pingIndex] = (uint8)min(ping * 255 / 1000, 255);
	}
	
	return true;
}



PingProfileWait::PingProfileWait() :
	SingleWait(TRANSLATE(IDS_WOL_PINGPROFILEREQUEST), 30000)
{
	session = WWOnline::Session::GetInstance(false);
	pendingPings = 0;
}



void PingProfileWait::WaitBeginning()
{
	if (!session)
		EndWait(WAIT_RESULT_ERROR, TRANSLATE(IDS_WOL_NOTINITIALIZED));
	else
	{
		if (session->pingServers.size() == 0)
			EndWait(WAIT_RESULT_ERROR, TRANSLATE(IDS_WOL_NOPINGSERVER));
		else
		{
			NotifyMe(*session);
			
			pendingPings = 0;
			for (RefPtr<WWOnline::PingServerData>* pingServer = session->pingServers.begin(); pingServer != session->pingServers.end(); ++pingServer)
			{
				if ((*pingServer)->ping == -1)
				{
					++pendingPings;
					session->RequestPing((*pingServer)->ipString, 1000);
				}
			}
			
			if (pendingPings == 0)
			{
				updatePings(session);
				EndWait(WAIT_RESULT_FINISHED, TRANSLATE(IDS_WOL_PINGPROFILERECEIVED));
			}
		}
	}
}



void PingProfileWait::HandleNotification(WWOnline::RawPing& ping)
{
	if (result == WAIT_RESULT_PENDING)
	{
		TT_ASSERT(pendingPings > 0);

		for (RefPtr<WWOnline::PingServerData>* pingServer = session->pingServers.begin(); pingServer != session->pingServers.end(); ++pingServer)
		{
			if (_stricmp(ping.ipString, (*pingServer)->ipString) == 0)
			{
				--pendingPings;
				break;
			}
		}
		
		if (pendingPings == 0)
		{
			updatePings(session);
			EndWait(WAIT_RESULT_FINISHED, TRANSLATE(IDS_WOL_PINGPROFILERECEIVED));
		}
	}
}
