#ifndef TT_INCLUDE__VISTABLECLASS_H
#define TT_INCLUDE__VISTABLECLASS_H



#include "engine_vector.h"
#include "multilist.h"



class CompressedVisTableClass;



class VisTableClass :
	public RefCountClass,
	public MultiListObjectClass
{

	int bitCount; // 0010  0028
	union
	{
		byte* bytes; // 0014  002C
		uint32* longs; // 0014  002C
	};

	int visSectorId; // 0018  0030
	int timeStamp; // 001C  0034

public:

	VisTableClass(uint, int);
	VisTableClass(CompressedVisTableClass*, int, int);
	VisTableClass(const VisTableClass& that) { *this = that; }
	~VisTableClass();
	const VisTableClass& operator =(const VisTableClass& that);

	void Alloc_Buffer(int);
	byte* Get_Bytes() const { return bytes; }
	int Get_Byte_Count() const { return 4 * Get_Long_Count(); }
	uint32* Get_Longs() const { return longs; }
	int Get_Long_Count() const;
	void Reset_All();
	void Set_All();
	void Delete_Bit(int);
	void Merge(const VisTableClass&);
	void Invert();
	bool Is_Equal_To(const VisTableClass&) const;
	int Count_Differences(const VisTableClass&) const;
	int Count_True_Bits() const;
	float Match_Fraction(const VisTableClass&) const;
	int Get_Bit_Count() const { return bitCount; }
	void Set_Vis_Sector_ID(int _visSectorId) { visSectorId = _visSectorId; }
	int Get_Vis_Sector_ID() const { return visSectorId; }
	void Set_Time_Stamp(int _timeStamp) { timeStamp = _timeStamp; }
	int Get_Time_Stamp() const { return timeStamp; }
	bool Get_Bit(int) const;
	void Set_Bit(int, bool);

}; // 0020  0038



#endif