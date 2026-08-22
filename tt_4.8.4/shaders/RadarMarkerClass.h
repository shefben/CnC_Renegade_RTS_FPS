#ifndef TT_INCLUDE__RADARMARKERCLASS_H
#define TT_INCLUDE__RADARMARKERCLASS_H



#include "Vector3.h"



class RadarMarkerClass
{

public:

	int markerID; //0000
	int radarBlipShape; // 0004
	int radarBlipColorType; // 0008
	Vector3 location; // 000C
	float radarBlipIntensity; // 0018
	bool operator== (const RadarMarkerClass &src)
	{
		return false;
	}
	bool operator!= (const RadarMarkerClass &src)
	{
		return true;
	}
};



#endif