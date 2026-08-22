#include "General.h"
#include "_globals.h"
#include "RegistryClass.h"
#include "SimpleFileFactoryClass.h"
#include "FileFactoryListClass.h"
#include "LoggingFileFactoryClass.h"
#include "StrippingFileFactoryClass.h"
#include "Random2Class.h"
#include "WWAudioClass.h"
#include "WW3DAssetManager.h"
RENEGADE_FUNCTION
void Construct_Directory_Structure()
AT2(0x00438020,0x004380C0);
class DebugManager {
public:
	void Init();
	void Load_Registry_Settings(const char *);
};
RENEGADE_FUNCTION
void DebugManager::Init()
AT2(0x00671DE0,0x00671680);
RENEGADE_FUNCTION
void DebugManager::Load_Registry_Settings(const char *)
AT2(0x00671F10,0x006717B0);
RENEGADE_FUNCTION
void Get_Version_Number(unsigned long *,unsigned long *)
AT2(0x00438C80,0x00438D20);
REF_DEF2(RenegadeWritingFileFactory, SimpleFileFactoryClass, 0x0081E878, 0x0081DA60);
REF_DEF2(TheWritingFileFactory, FileFactoryClass *,0x00809E7C, 0x00809054);
extern REF_DECL2(RenegadeBaseFileFactory, SimpleFileFactoryClass);
REF_DEF2(TheSimpleFileFactory, FileFactoryClass *,0x00809E78, 0x00809050);
extern REF_DECL2(RenegadeFileFactory, FileFactoryListClass);
REF_DEF2(TheFileFactory, FileFactoryClass *, 0x00809E74, 0x0080904C);
REF_DEF2(LoggingFileFactory, LoggingFileFactoryClass, 0x0081ED10, 0x0081DEF8);
REF_DEF2(StrippingFileFactory, StrippingFileFactoryClass, 0x0081E8F0, 0x0081DAD8);

class MPSettingsMgrClass
{
public:
	static void Load_Settings();
};

class PathMgrClass {
public:
	static void Initialize();
};

RENEGADE_FUNCTION
void MPSettingsMgrClass::Load_Settings()
AT2(0x004401E0,0x004402A0);
RENEGADE_FUNCTION
void PathMgrClass::Initialize()
AT2(0x0062E7A0,0x0062E040);
bool Game_Init()
{
	return false;
}
