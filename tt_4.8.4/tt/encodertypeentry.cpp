#include "general.h"
#include "encodertypeentry.h"
#include "cMathUtil.h"
#include "engine_math.h"
RENEGADE_FUNCTION
void cEncoderTypeEntry::Init(int bitCount)
AT2(0x00784330,0x00783BD0);
RENEGADE_FUNCTION
void cEncoderTypeEntry::Init(double min,double max,double resolution)
AT2(0x00784290,0x00783B30);
RENEGADE_FUNCTION
void cEncoderTypeEntry::Calc_Bit_Precision(double resolution)
AT2(0x00784470,0x00783D10);
RENEGADE_FUNCTION
bool cEncoderTypeEntry::Scale(double value, ULONG& compressedValue)
AT2(0x00784390,0x00783C30);
/*{
	double clampedValue = Clamp(value);
	compressedValue = cMathUtil::Round((clampedValue - Min) / Resolution);
	return clampedValue != value;
}*/
double cEncoderTypeEntry::Unscale(ULONG value)
{
   return (double)value * Resolution + Min;
}
double cEncoderTypeEntry::Clamp(double value)
{
	return clamp(value, Min, Max);
}
