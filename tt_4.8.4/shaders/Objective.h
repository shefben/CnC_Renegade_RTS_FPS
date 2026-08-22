#ifndef TT_INCLUDE__OBJECTIVE_H
#define TT_INCLUDE__OBJECTIVE_H



#include "Vector3.h"
#include "engine_common.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "ReferencerClass.h"

class Objective
{

public:

	typedef enum
	{
		Pending,
		Accomplished,
		Failed,
		Hidden // ?
	} Status;


	uint objectiveID; //0
	uint type; //4
	Status status; //8
	uint descriptionStringId; //C
	uint titleStringId; //10
	StringClass sound; //14
	StringClass texture; //18
	uint hudTitleStringId; //1C
	float priority; //20
	bool showOnRadar; //24
	Vector3 location; //28
	float radarBlipIntensity; //34
	ReferencerClass objectiveObject; //38
	float age; //48
	Vector3 Type_To_Color();
	int Radar_Blip_Color_Type() const
	{
		return type + 4;
	}
};



#endif