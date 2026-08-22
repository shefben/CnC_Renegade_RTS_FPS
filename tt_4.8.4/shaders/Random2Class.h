#ifndef TT_INCLUDE__RANDOM2CLASS_H
#define TT_INCLUDE__RANDOM2CLASS_H



class Random2Class
{

	int unk0000; // 0000
	int unk0004; // 0004
	int unk0008[250]; // 0008

public:

	Random2Class(uint arg);
	uint32 operator()();
	int operator()(int, int);
	float Get_Float();

}; // 03F0



extern REF_DECL2(FreeRandom, Random2Class);



#endif
