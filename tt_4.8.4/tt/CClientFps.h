#ifndef TT_INCLUDE__CCLIENTFPS_H
#define TT_INCLUDE__CCLIENTFPS_H



#include "cNetEvent.h"
#include "NetworkObjectClassId.h"



class CClientFps :
	public cNetEvent
{

private:

	int clientId; // 06B4
	uint8 fps; // 06B8

public:

	CClientFps();
	virtual void Init();
	virtual void Act();
	virtual void Export_Creation(BitStreamClass& bitStream);
	virtual void Import_Creation(BitStreamClass& bitStream);
	virtual void Export_Frequent(BitStreamClass& bitStream);
	virtual void Import_Frequent(BitStreamClass& bitStream);
	virtual unsigned int Get_Network_Class_ID() const { return NET_CClientFps ; }
	virtual void Delete();
	
	void Set_Fps(int _fps);

};


extern REF_DECL2(PClientFps, CClientFps*);



#endif