#ifndef TT_INCLUDE__ITERATOR_TT_H
#define TT_INCLUDE__ITERATOR_TT_H



#include "Iterator.h"
#include "cConnection.h"



template<> class Iterator<cConnection>
{

	cRemoteHost** current;
	cRemoteHost** end;

	inline void seekNext() { while (current != end && !*current) { ++current; } }

public:

	inline Iterator(cConnection& connection) { current = &connection.remoteHosts[connection.minRemoteHostId]; end = &connection.remoteHosts[connection.maxRemoteHostId+1]; seekNext(); }
	inline operator bool() { return current != end; }
	inline operator cRemoteHost*() { return *current; }
	inline cRemoteHost* operator ->() { return (cRemoteHost*)*this; }
	inline Iterator& operator ++() { ++current; seekNext(); return *this; }
};



#endif
