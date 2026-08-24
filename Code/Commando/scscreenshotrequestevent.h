/******************************************************************************
*
* FILE
*     scscreenshotrequestevent.h
*
* DESCRIPTION
*     Server asking one client for a picture of its game window.
*
*     The URL travels with the request rather than being configured on the
*     client, so a client never holds a destination it was not just told about
*     and there is nothing to leave behind when the player leaves the server.
*
*     See remotescreenshot.h for what the client does with it, and why it tells
*     the player.
*
******************************************************************************/

#ifndef	__SCSCREENSHOTREQUESTEVENT_H__
#define	__SCSCREENSHOTREQUESTEVENT_H__

#include "netevent.h"
#include "netclassids.h"
#include "wwstring.h"

//-----------------------------------------------------------------------------
class	cScScreenshotRequestEvent : public cNetEvent
{
public:
	cScScreenshotRequestEvent (void);

	void					Init (int client_id, const StringClass &url);

	virtual void		Export_Creation (BitStreamClass &packet) override;
	virtual void		Import_Creation (BitStreamClass &packet) override;
	virtual uint32		Get_Network_Class_ID (void) const override	{return NETCLASSID_SCSCREENSHOTREQUESTEVENT;}

	enum					{MAX_URL_LENGTH = 512};

private:

	virtual void		Act (void) override;

	StringClass			Url;
};

//-----------------------------------------------------------------------------

#endif	// __SCSCREENSHOTREQUESTEVENT_H__
