/******************************************************************************
*
* FILE
*     csscreenshotresultevent.h
*
* DESCRIPTION
*     A client telling the server what became of a screenshot request.
*
*     Every request gets an answer, including refusals and failures.  An
*     operator who cannot tell a client that could not comply from one that did
*     not has been given a tool that lies to them.
*
******************************************************************************/

#ifndef	__CSSCREENSHOTRESULTEVENT_H__
#define	__CSSCREENSHOTRESULTEVENT_H__

#include "netevent.h"
#include "netclassids.h"

//-----------------------------------------------------------------------------
class	cCsScreenshotResultEvent : public cNetEvent
{
public:
	cCsScreenshotResultEvent (void);

	void					Init (int result);

	virtual void		Export_Creation (BitStreamClass &packet) override;
	virtual void		Import_Creation (BitStreamClass &packet) override;
	virtual uint32		Get_Network_Class_ID (void) const override	{return NETCLASSID_CSSCREENSHOTRESULTEVENT;}

private:

	virtual void		Act (void) override;

	int					ClientId;
	int					Result;
};

//-----------------------------------------------------------------------------

#endif	// __CSSCREENSHOTRESULTEVENT_H__
