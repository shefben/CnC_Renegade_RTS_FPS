#ifndef TT_INCLUDE__STATEVECTORCLASS_H
#define TT_INCLUDE__STATEVECTORCLASS_H
#include "engine_vector.h"

class StateVectorClass : public DynamicVectorClass<float> 
{
public:
	void Reset(void) { ActiveCount = 0; }
	void Resize(int size) { if (size > VectorMax) { DynamicVectorClass<float>::Resize(size); } }
};

#endif