#ifndef _SOUNDANDSETTINGS_H_
#define _SOUNDANDSETTINGS_H_
#include <windows.h>
#include "..\..\sound_preset\sndapi\interfaces.h"
void Load_Settings();
void Save_Settings();

extern ISoundDatabase *g_iSoundDatabase;
extern HKEY g_hKey;

#endif //_SOUNDANDSETTINGS_H_