#ifndef TT_INCLUDE__CSERVERFPS_H
#define TT_INCLUDE__CSERVERFPS_H


#include "NetworkObjectClass.h"

class cServerFps :
	public NetworkObjectClass
{

private:
	
	static REF_DECL2(TheInstance, cServerFps*);

	int fps; // 06B4

public:

	static void Create_Instance();
	static void Destroy_Instance();
	static cServerFps* Get_Instance() { return TheInstance; }

	cServerFps();
	void Set_Fps(int _fps) { fps = _fps; Set_Object_Dirty_Bit(DB_FREQUENT,true); }
	int Get_Fps() const { return fps; }

	virtual ~cServerFps() {}
	virtual void Export_Frequent(BitStreamClass& bitStream);
	virtual void Import_Frequent(BitStreamClass& bitStream);
	virtual void Delete() {}
	virtual void Set_Delete_Pending() {}

};



#endif