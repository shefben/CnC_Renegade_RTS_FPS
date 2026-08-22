#ifndef TT_INCLUDE__VISTABLEMGRCLASS_H
#define TT_INCLUDE__VISTABLEMGRCLASS_H

#include "engine_common.h"
#include "engine_vector.h"
class CompressedVisTableClass;
class VisTableMgrClass
{

private:

   SimpleDynVecClass<CompressedVisTableClass*> tables; // 0
   sint32                                      s16;
   sint32                                      s20;
   sint32                                      s24;
   sint32                                      s28;

}; // 32

#endif
