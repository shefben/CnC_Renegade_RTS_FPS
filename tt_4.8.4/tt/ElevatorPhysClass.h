#ifndef TT_INCLUDE__ELEVATORPHYSCLASS_H
#define TT_INCLUDE__ELEVATORPHYSCLASS_H



#include "AccessiblePhysClass.h"



class ElevatorPhysClass :
	public AccessiblePhysClass
{

public:

	enum {
		STATE_DOWN						= 0,
		STATE_MOVING_UP,
		STATE_UP,
		STATE_MOVING_DOWN,
		STATE_MAX
	};

	enum {
		DOOR_STATE_NORMAL				= 0,
		DOOR_STATE_UNLOCKED,
		DOOR_STATE_ACCESS_DENIED,
		DOOR_STATE_MAX
	};
	static void Set_Precision();

};



#endif