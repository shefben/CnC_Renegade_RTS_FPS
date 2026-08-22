#include "General.h"
#include "RefCounted.h"



void RefCounted::ReleaseReference() const
{
	--referenceCount;
	if (referenceCount == 0)
		delete this;
}



void RefCounted::AddReference() const
{
	++referenceCount;
}
