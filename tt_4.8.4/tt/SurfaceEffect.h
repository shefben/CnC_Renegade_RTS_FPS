#ifndef TT_INCLUDE__SURFACEEFFECT_H
#define TT_INCLUDE__SURFACEEFFECT_H



#include "RandomStringClass.h"



struct SurfaceEffect
{

	RandomStringClass sounds; // 0000
	RandomStringClass emitters; // 0408
	RandomStringClass decals; // 0810
	float decalSize; // 0C18
	float decalSizeRandom; // 0C1C

}; // 0C20



#endif