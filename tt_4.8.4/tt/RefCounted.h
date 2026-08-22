#ifndef TT_INCLUDE__REFCOUNTED_H
#define TT_INCLUDE__REFCOUNTED_H



class RefCounted
{

	mutable int referenceCount;

public:

	virtual void ReleaseReference() const;
	virtual ~RefCounted() {};
	virtual bool operator==(const RefCounted&) const { return false; }

	RefCounted() { referenceCount = 0; }
	void AddReference() const;

};



#endif