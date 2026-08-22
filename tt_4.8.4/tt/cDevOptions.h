#ifndef TT_INCLUDE__CDEVOPTIONS_H
#define TT_INCLUDE__CDEVOPTIONS_H
#include "cRegistryBool.h"


class cDevOptions
{

public:

	static REF_DECL2(QuickFullExit, bool);
	static REF_DECL2(ExitThreadOnAssert, cRegistryBool);
	static REF_DECL2(CompareExeVersionOnNetwork, cRegistryBool);
	static REF_DECL2(UseNewTCADO, cRegistryBool);
	static REF_DECL2(ShowFps, cRegistryBool);

};



#endif