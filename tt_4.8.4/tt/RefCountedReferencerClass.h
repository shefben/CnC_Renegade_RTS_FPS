#ifndef TT_INCLUDE__REFCOUNTEDREFERENCERCLASS_H
#define TT_INCLUDE__REFCOUNTEDREFERENCERCLASS_H



#include "ReferencerClass.h"
#include "engine_vector.h"



class RefCountedReferencerClass :
	public ReferencerClass,
	public RefCountClass
{

public:

	inline RefCountedReferencerClass(): ReferencerClass()
	{
	}

	inline RefCountedReferencerClass(const ReferencerClass& src) : ReferencerClass()
	{
		ReferencerClass::operator=(src);
	}

}; // 0018  0030



#endif
