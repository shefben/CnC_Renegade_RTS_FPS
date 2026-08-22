#pragma once

#include "childdialog.h"

class ModPackageClass;



class MapSelectDialog :
	public ChildDialogClass
{

	DynamicVectorClass<WideStringClass> MapList; // 0078  008C
	DynamicVectorClass<WideStringClass> SelectedMapList; // 0078  008C

public:
	MapSelectDialog();
	void buildDefaultMapList();
	void buildMapList(ModPackageClass* modPackage);

};
