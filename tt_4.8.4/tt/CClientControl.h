#ifndef TT_INCLUDE__CCLIENTCONTROL_H
#define TT_INCLUDE__CCLIENTCONTROL_H



#include "NetworkObjectClass.h"
#include "NetworkObjectClassId.h"



class CClientControl :
	public NetworkObjectClass
{

public:

   int clientId; // 06B4
   int soldierId; // 06B8


	CClientControl();

	virtual void   Delete               ();
	virtual void   Export_Creation      (BitStreamClass& oStream);
	virtual void   Export_Frequent      (BitStreamClass& oStream);
	virtual unsigned int Get_Network_Class_ID () const { return NET_CClientControl; }
	virtual void   Import_Creation      (BitStreamClass& oStream);
	virtual void   Import_Frequent      (BitStreamClass& oStream);

	void Init                ();
	void Set_Update_Flag     (sint32 sSoldierID);

};


extern REF_DECL1(PClientControl, CClientControl*);


#endif
