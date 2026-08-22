#ifndef TT_INCLUDE_INPUTCONFIGCLASS_H
#define TT_INCLUDE_INPUTCONFIGCLASS_H
#include "Engine_String.h"
class InputConfigClass : public NoEqualsClass<InputConfigClass>
{
public:
	StringClass Name;
	StringClass Filename;
	bool unk1;
	bool unk2;
};

#endif