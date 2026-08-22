#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#include "../Shared/BaseTypes.h"

#define LAUNCHER_VERSION			(1)

//Comment out to enable selfupdates
#define TEST_NO_SELFUPDATE

#define LAUNCHER_CONTENT_VFS		(L"LauncherContent.vfs")
#define START_PAGE					(L"http://sktest.aruarose.com/formtest3.html")
#define LAUNCHER_VERSION_FILE_URL	(L"http://sktest.aruarose.com/Update/version.dat")
//#define LAUNCHER_PACKAGE_URL		(L"http://sktest.aruarose.com/Update/launcher.cdp")

//#define LAUNCHER_PACKAGE_URL		(L"http://sktest.aruarose.com/Update/launcher2.cdp")
#define LAUNCHER_PACKAGE_URL		(L"http://sktest.aruarose.com/Update/launcher3.cdp")

#define LAUNCHER_TEMP_PACKAGE_FILE	(L"launcher.cdp")

#define GAME_VERSION_FILE_URL		(L"http://sktest.aruarose.com/patchversions.dat")
#define GAME_VERSION_FILE			(L"version.dat")
#define GAME_PACKAGE_URL_STRING		(L"http://sktest.aruarose.com/patch_%d.cdp")

#define CLASS_NAME					(L"{AF117795-BC4C-4cec-82CD-64576A929F71}")
#define WINDOW_TITLE				(L"Launcher")

#define ARUA_VFS					(L"Arua.VFS")

#define ARUA_VFS_USER_VERSION		(_32BIT_INVERSE('ARUA'))
#define ARUA_VFS_DOMAIN				(L"AruaROSE")


#endif //_SETTINGS_H_
