#ifndef TT_INCLUDE__BITCOUNTERCLASS_H
#define TT_INCLUDE__BITCOUNTERCLASS_H



class BitCounterClass
{

	int cache[256];

public:

	BitCounterClass();
	int count(byte data);

};



extern BitCounterClass TheBitCounter;



#endif