/******************************************************************************
*
* FILE
*     csmapqueryresponseevent.h
*
* DESCRIPTION
*     A client answering cScMapQueryEvent.
*
*     Carries the map that was asked about as well as the answer, because the
*     server may have asked several clients about several maps and a bare yes
*     answers none of them.
*
******************************************************************************/

#ifndef	__CSMAPQUERYRESPONSEEVENT_H__
#define	__CSMAPQUERYRESPONSEEVENT_H__

#include "netevent.h"
#include "netclassids.h"
#include "wwstring.h"

//-----------------------------------------------------------------------------
//
//	A C->S mirrored object carrying whether the client has a named map.
//
class	cCsMapQueryResponseEvent : public cNetEvent
{
public:
	cCsMapQueryResponseEvent (void);

	void					Init (const StringClass &map_name, bool has_map);

	virtual void		Export_Creation (BitStreamClass &packet) override;
	virtual void		Import_Creation (BitStreamClass &packet) override;
	virtual uint32		Get_Network_Class_ID (void) const override	{return NETCLASSID_CSMAPQUERYRESPONSEEVENT;}

	enum					{MAX_MAP_NAME_LENGTH = 128};

private:

	virtual void		Act (void) override;

	int					ClientId;
	StringClass			MapName;
	bool					HasMap;
};

//-----------------------------------------------------------------------------

#endif	// __CSMAPQUERYRESPONSEEVENT_H__
