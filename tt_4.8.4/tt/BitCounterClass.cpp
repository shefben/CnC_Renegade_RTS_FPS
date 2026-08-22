#include "General.h"
#include "BitCounterClass.h"



BitCounterClass::BitCounterClass()
{
	cache[0] = 0;
	for (uint cacheIndex = 1; cacheIndex < 256; ++cacheIndex)
		cache[cacheIndex] += cache[cacheIndex >> 1] + (cacheIndex & 1);
}



int BitCounterClass::count(byte data)
{
	return cache[data];
}



BitCounterClass TheBitCounter;
