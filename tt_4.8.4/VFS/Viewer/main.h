#ifndef _MAIN_H_
#define _MAIN_H_


#include <commctrl.h>
#include "../Shared/StdLib.h"
#include "../Shared/Memory.h"
#include "../VFS/API/VFSInterface.h"

#define CLASS_NAME			L"{C72F4B00-A4EE-11DD-AD8B-0800200C9A66}"
//#define WINDOW_TITLE		L"Tiberian Technologies VFS viewer"
#define WINDOW_TITLE		L"VFS viewer"

extern IVFS*	g_pVFS;

#define STATUSBAR_HEIGHT	(25)
#define TREEVIEW_STYLE		(TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|WS_BORDER|WS_TABSTOP|TVS_INFOTIP|TVS_EDITLABELS)

#define EX_ERROR_OK			0x00
#define EX_ERROR_FILE_OPEN	0x01
#define EX_ERROR_FILE_READ	0x02
#define EX_ERROR_FILE_WRITE	0x02

typedef struct _VFSITEM : AutoPoolClass<_VFSITEM, 256> {
	HTREEITEM	m_hTreeItem;
	size_64		m_TotalSize;
	int			m_FileCount;
	int			m_DirectoryCount;
	ptr			m_pFileData;
	ptr			m_pParentFileData;
	int			m_Type;
} VFSITEM, *PVFSITEM;

#endif //_MAIN_H_
