#include "general.h"
#include "cMathUtil.h"



int cMathUtil::Round(double number)
{
	// See http://ldesoras.free.fr/doc/articles/rounding_en.pdf

	int result;
	static const float half = .5f;
	__asm
	{
		fld number
		fadd st, st
		fadd half
		fistp result
		sar result, 1
	}

	TT_ASSERT(result == (int)(number + (number > 0 ? .5 : -.5)));
	return result;
}