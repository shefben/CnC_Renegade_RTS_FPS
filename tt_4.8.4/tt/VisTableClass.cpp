#include "General.h"
#include "VisTableClass.h"

#include "BitCounterClass.h"



VisTableClass::VisTableClass(uint _bitCount, int _visSectorId)
{
	bitCount = _bitCount;
	longs = NULL;
	visSectorId = _visSectorId;
	timeStamp = 0;

	Alloc_Buffer(bitCount);
}


/*
VisTableClass::VisTableClass(CompressedVisTableClass* compressedVisTable, int _bitCount, int _visSectorId)
{
	bitCount = _bitCount;
	longs = NULL;
	visSectorId = _visSectorId;
	timeStamp = 0;
	
	Alloc_Buffer(bitCount);
	return compressedVisTable->Decompress(Get_Bytes(), Get_Byte_Count());
}
*/


VisTableClass::~VisTableClass()
{
	delete longs;
}



const VisTableClass& VisTableClass::operator =(const VisTableClass& that)
{
	Alloc_Buffer(that.bitCount);
	memcpy(bytes, that.bytes, Get_Byte_Count());
	
	visSectorId = that.visSectorId;
	timeStamp = that.timeStamp;
	
	return *this;
}



void VisTableClass::Alloc_Buffer(int _bitCount)
{
	bitCount = _bitCount;
	delete[] longs;
	longs = new uint32[Get_Long_Count()];
	memset(bytes, 0, Get_Byte_Count());
}



int VisTableClass::Get_Long_Count() const
{
	int bits = bitCount + 31;
	if (bits < 0)
		bits += 31;
	return bits / 32;
}



void VisTableClass::Reset_All()
{
	TT_UNTESTED;

	if (bytes)
		memset(bytes, 0, Get_Byte_Count());
}



void VisTableClass::Set_All()
{
	TT_UNTESTED;

	if (bytes)
		memset(bytes, 0xFF, Get_Byte_Count());
	
	// This unsets the last few bits, quite sloppy.
	for (int bitIndex = (uint)bitCount; bitIndex < 32 * Get_Long_Count(); ++bitIndex)
		longs[bitIndex / 32u] &= ~(0x80000000u >> (bitIndex % 32u));
}



void VisTableClass::Delete_Bit(int bitIndex)
{
	TT_UNTESTED;

	int v18 = min(bitCount - 1, 32 * ((bitIndex >> 5) + 1));
	
	for (int i = bitIndex; i < v18; ++i)
	{
		if (longs[(i + 1) / 32u] & (0x80000000u >> ((i + 1) % 32u)))
			longs[i / 32u] |= 0x80000000u >> (i % 32u);
		else
			longs[i / 32u] &= ~(0x80000000u >> (i % 32u));
		
	}

	int longCount = Get_Long_Count();
	if (bitIndex / 32u == longCount - 1u)
		longs[bitIndex / 32u] &= ~(0x80000000u >> ((bitCount - 1) % 32u));
	else
	{
		for (int i = bitIndex / 32u + 1; i < longCount - 1; ++i)
			longs[i] = ((longs[i+1] & 0x80000000u) >> 31u) | (longs[i] << 1);
		longs[longCount - 1] <<= 1u;
	}
	
	--bitCount;
}




void VisTableClass::Merge(const VisTableClass& that)
{
	if (bitCount == that.bitCount)
		for (int longIndex = 0; longIndex < Get_Long_Count(); ++longIndex)
			longs[longIndex] |= that.longs[longIndex];
}



void VisTableClass::Invert()
{
	TT_UNTESTED;

	for (int bitIndex = 0; bitIndex < Get_Long_Count(); ++bitIndex)
		longs[bitIndex] = ~longs[bitIndex];
}



bool VisTableClass::Is_Equal_To(const VisTableClass& that) const
{
	TT_UNTESTED;

	if (bitCount == that.bitCount)
		return memcmp(bytes, that.bytes, Get_Byte_Count()) == 0;
	else
		return false;
}



int VisTableClass::Count_Differences(const VisTableClass& that) const
{
	TT_UNTESTED;

	int differences = 0;
	if (bitCount == that.bitCount)
	{
		const int byteCount = Get_Byte_Count();
		for (int byteIndex = 0; byteIndex < byteCount; ++byteIndex)
			differences += TheBitCounter.count(bytes[byteIndex] ^ that.bytes[byteIndex]);
	}
	return differences;
}



int VisTableClass::Count_True_Bits() const
{
	TT_UNTESTED;

	int trueBitCount = 0;
	const int byteCount = Get_Byte_Count();
	for (int byteIndex = 0; byteIndex < byteCount; ++byteIndex)
		trueBitCount += TheBitCounter.count(bytes[byteIndex]);
	return trueBitCount;
}



float VisTableClass::Match_Fraction(const VisTableClass& that) const
{
	TT_UNTESTED;

	if (bitCount == that.bitCount)
	{
		int differentBits = 0;
		int trueBits = 0;

		int byteCount = Get_Byte_Count();
		for (int byteIndex = 0; byteIndex < byteCount; ++byteIndex)
		{
			differentBits += TheBitCounter.count(bytes[byteIndex] ^ that.bytes[byteIndex]);
			trueBits += TheBitCounter.count(bytes[byteIndex] | that.bytes[byteIndex]);
		}

		if (trueBits == 0)
			return 1.f;
		else
			return 1.f - differentBits / trueBits;
	}
	else
		return 0.f;
}



bool VisTableClass::Get_Bit(int bitIndex) const
{
	int mask = 0x80000000u >> (bitIndex % 32u);
	return (longs[bitIndex / 32u] & mask) != 0;
}



void VisTableClass::Set_Bit(int bitIndex, bool value)
{
	TT_UNTESTED;

	int mask = 0x80000000u >> (bitIndex % 32u);
	if (value)
		bytes[bitIndex / 32u] |= mask;
	else
		bytes[bitIndex / 32u] &= ~mask;
}
