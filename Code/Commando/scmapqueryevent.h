/******************************************************************************
*
* FILE
*     scmapqueryevent.h
*
* DESCRIPTION
*     Server asks one client whether it has a named map.
*
*     4.8.4 asked this by sending the client a private chat message carrying a
*     smuggled opcode, which only worked because a DLL on the far end was
*     reading other people's mail.  Directive 0.5 has no such DLL, so the
*     question is an ordinary network event and the answer comes back as
*     another one: cCsMapQueryResponseEvent.
*
*     The pair is the client-to-server reply channel the map transition and
*     download work needs.  Nothing about it is specific to the console command
*     that first asks the question.
*
******************************************************************************/

#ifndef	__SCMAPQUERYEVENT_H__
#define	__SCMAPQUERYEVENT_H__

#include "netevent.h"
#include "netclassids.h"
#include "wwstring.h"

//-----------------------------------------------------------------------------
//
//	A S->C mirrored object carrying the name of a map to look for.
//
class	cScMapQueryEvent : public cNetEvent
{
public:
	cScMapQueryEvent (void);

	void					Init (int client_id, const StringClass &map_name);

	virtual void		Export_Creation (BitStreamClass &packet) override;
	virtual void		Import_Creation (BitStreamClass &packet) override;
	virtual uint32		Get_Network_Class_ID (void) const override	{return NETCLASSID_SCMAPQUERYEVENT;}

	enum					{MAX_MAP_NAME_LENGTH = 128};

private:

	virtual void		Act (void) override;

	StringClass			MapName;
};

//-----------------------------------------------------------------------------

#endif	// __SCMAPQUERYEVENT_H__
