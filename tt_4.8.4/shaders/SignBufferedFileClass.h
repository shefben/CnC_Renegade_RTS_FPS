#ifndef TT_INCLUDE_SIGNBUFFEREDFILECLASS_H
#define TT_INCLUDE_SIGNBUFFEREDFILECLASS_H
#include "BufferedFileClass.h"
class SignBufferedFileClass : public BufferedFileClass {
public:
	bool IsSigned;
	SignBufferedFileClass() : IsSigned(false)
	{
	}
};
#endif
