#ifndef TT_INCLUDE__MODPACKAGECLASS_H
#define TT_INCLUDE__MODPACKAGECLASS_H

#include "engine_vector.h"



class ModPackageClass :
	public NoEqualsClass<ModPackageClass>
{

	StringClass baseName; // 0004
	StringClass fileName; // 0008
	uint32 crc; // 000C

public:

	ModPackageClass();
	virtual ~ModPackageClass();
	void Set_Package_Filename(const char* _fileName);
	void Build_Level_List(DynamicVectorClass<StringClass>& levelList);
	bool Find_Map_From_CRC(uint32 mapCrc, StringClass* mapName);
	void Compute_CRC();
	uint32 Get_CRC();
	int Get_Map_Index(const char* mapName);
	const StringClass& Get_Name() { return baseName; }
	void Set_Name(const char* _baseName) { baseName = _baseName; }
	const StringClass& Get_Package_Filename() { return fileName; }

}; // 0010



#endif
