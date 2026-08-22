#ifndef TT_INCLUDE__ASSETSTATUSCLASS_H
#define TT_INCLUDE__ASSETSTATUSCLASS_H



class AssetStatusClass
{

public:

	UNKB unk0000;
	bool unk0001;
	// ...

	static REF_DECL2(Instance, AssetStatusClass);

	AssetStatusClass();
	~AssetStatusClass();
	UNK Add_To_Report(int, const char*);
	UNK Report_Load_On_Demand_RObj(const char*);
	UNK Report_Load_On_Demand_HAnim(const char*);
	UNK Report_Load_On_Demand_HTree(const char*);
	UNK Report_Missing_RObj(const char*);
	UNK Report_Missing_HAnim(const char*);
	UNK Report_Missing_HTree(const char*);

}; // 0064



#endif