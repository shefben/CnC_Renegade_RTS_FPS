#ifndef TT_INCLUDE__ENGINE2_H
#define TT_INCLUDE__ENGINE2_H



class FileFactoryClass;
class FileFactoryListClass;
class WideStringClass;
class Vector3;
class SimpleFileFactoryClass;


extern REF_DECL2(GameInFocus, bool);
extern REF_DECL2(_HiddenFrameCount, uint);
extern REF_DECL2(RenegadeFileFactory, FileFactoryListClass);
extern REF_DECL2(RenegadeBaseFileFactory, SimpleFileFactoryClass);

extern bool vehicleExitReloadContinued;
#endif
