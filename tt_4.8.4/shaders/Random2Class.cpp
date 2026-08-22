#include "General.h"
#include "Random2Class.h"

#include "Random3Class.h"



REF_DEF2(FreeRandom, Random2Class, 0x0085CB40, 0x0085BD28);



Random2Class::Random2Class(uint arg)
{
	unk0000 = 0;
	unk0004 = 103;
	Random3Class random3(arg, 0);
	
	for (uint i = 0; i < 250; ++i)
		unk0008[i] = random3();
}



uint32 Random2Class::operator()()
{
	unk0008[unk0000] ^= unk0008[unk0004];
	uint32 value = unk0008[unk0000];
	
	if (++unk0000 >= 250)
		unk0000 = 0;
	if (++unk0004 >= 250)
		unk0004 = 0;

	return value;
}

float Random2Class::Get_Float()
{
	return operator()() % 4096 / 4096.f;
}

RENEGADE_FUNCTION
int Random2Class::operator ()(int, int)
AT1(0x005E67D0);