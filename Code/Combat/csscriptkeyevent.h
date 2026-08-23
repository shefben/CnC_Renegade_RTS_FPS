/******************************************************************************
*
* FILE
*     csscriptkeyevent.h
*
* DESCRIPTION
*     One logical key press, client to server.
*
*     The client end of the script key facility described in scriptkeys.h.  It
*     carries a name and nothing else: which player sent it is the connection
*     it arrived on, and what it means is the server's business.
*
*     Reliable, unlike cCsDamageEvent.  A dropped shot is one shot; a dropped
*     key press is a deploy that did not happen.
*
******************************************************************************/

#ifndef __CSSCRIPTKEYEVENT_H__
#define __CSSCRIPTKEYEVENT_H__

#include "networkobject.h"
#include "netclassids.h"
#include "wwstring.h"


class	cCsScriptKeyEvent : public NetworkObjectClass
{
public:
	cCsScriptKeyEvent (void);

	void						Init (const char *key_name);

	virtual void			Export_Creation (BitStreamClass &packet) override;
	virtual void			Import_Creation (BitStreamClass &packet) override;

	virtual uint32			Get_Network_Class_ID (void) const override	{ return NETCLASSID_CSSCRIPTKEYEVENT; }
	virtual void			Delete (void) override								{ delete this; }

private:

	virtual void			Act (void);

	int						SenderId;
	StringClass				KeyName;
};


#endif	// __CSSCRIPTKEYEVENT_H__
