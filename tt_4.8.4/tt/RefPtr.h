#ifndef TT_INCLUDE__REFPTR_H
#define TT_INCLUDE__REFPTR_H



#include "RefPtrBase.h"



template<class Referent> class RefPtr
{

	RefPtrBase base;

	inline RefPtr(Referent* _referent) : base(_referent) {}
	inline const RefPtr& operator =(Referent* _referent) { base = _referent; return *this; }

public:

	static RefPtr<Referent> create() { RefPtr instance = new Referent(); return instance; }

	virtual ~RefPtr() {};

	inline RefPtr() {}
	inline RefPtr(const RefPtr& that) : base(that.base) { }
	inline const RefPtr& operator =(const RefPtr& that) { base = that.base; return *this; }

	inline operator bool() const { return base; }

	inline Referent& operator *() { return (Referent&)*base; }
	inline Referent* operator ->() { return (Referent*)&*base; }

};



#endif
