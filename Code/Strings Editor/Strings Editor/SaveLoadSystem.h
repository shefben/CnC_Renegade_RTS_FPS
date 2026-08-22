#ifndef _SAVELOADSYSTEM_H_
#define _SAVELOADSYSTEM_H_
#include "Chunkclasses.h"

class PostLoadableClass {
public:
	bool m_Do_Post_Load;
	virtual void On_Post_Load();
};


class PersistClass : public PostLoadableClass{
public:
	virtual ~PersistClass();
	virtual void *Get_Factory();
	virtual bool Save(ChunkSaveClass &save);
	virtual bool Load(ChunkLoadClass &load);
};


#endif //_SAVELOADSYSTEM_H_