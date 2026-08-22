#include <windows.h>
#include "VFS.h"
#include "VFSStructures.h"
#include "../Shared/Memory.h"
#include "../Shared/Lists.h"
#include "../Shared/BaseTypes.h"
#include "../Shared/StdLib.h"
#include "../Shared/MD5.h"
#include "API/VFSErrorcodes.h"
#include "API/VFSVersion.h"

#ifdef VFS_ENABLE_AES
#include "../Shared/AES/aes.h"
#endif //VFS_ENABLE_AES

#include <stdio.h>

/*VFS2::VFS2(){
}

VFS2::~VFS2(){
}

int VFS2::GetVersion(){
	return VFS_INTERFACE_FLAT_V2;
}//*/


