#ifndef TT_INCLUDE__RANDOMSTRINGCLASS_H
#define TT_INCLUDE__RANDOMSTRINGCLASS_H



#include "Random2Class.h"
#include "engine_string.h"
#include "engine_vector.h"



class RandomStringClass
{

public:

	DynamicVectorClass<StringClass> strings; // 0000
	Random2Class random; // 0018

	RandomStringClass();
	void Add_String(const char* string);
	const StringClass* Get_String();

}; // 0408



#endif