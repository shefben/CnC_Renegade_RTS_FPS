#ifndef TT_INCLUDE__REFPTRBASE_H
#define TT_INCLUDE__REFPTRBASE_H



#include "RefCounted.h"



class RefPtrBase
{

	RefCounted* referent;

	void destruct() { if (referent) referent->ReleaseReference(); }
	void construct(RefCounted* _referent) { referent = _referent; if (referent) referent->AddReference(); }

public:

	inline RefPtrBase(RefCounted* _referent) { construct(_referent); }
	const RefPtrBase& operator =(RefCounted* _referent) { if (referent != _referent) { destruct(); construct(_referent); } return *this; }


	~RefPtrBase() { destruct(); }

	inline RefPtrBase() { referent = NULL; }

	inline RefPtrBase(const RefPtrBase& that) { construct(that.referent); }
	inline const RefPtrBase& operator =(const RefPtrBase& that) { *this = that.referent; return *this; }

	inline operator bool() const { return referent != NULL; }

	inline RefCounted& operator *() { TT_ASSERT(referent); return *referent; }

};



#endif