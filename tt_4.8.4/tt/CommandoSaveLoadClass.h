#ifndef TT_INCLUDE__COMMANDOSAVELOADCLASS_H
#define TT_INCLUDE__COMMANDOSAVELOADCLASS_H



#include "SaveLoadSubSystemClass.h"



class CommandoSaveLoadClass :
	public SaveLoadSubSystemClass
{

public:

	CommandoSaveLoadClass();

	virtual ~CommandoSaveLoadClass();
	virtual uint32 Chunk_ID() const;
	virtual bool Save(ChunkSaveClass&);
	virtual bool Load(ChunkLoadClass&);
	virtual const char* Name() const;

};



extern REF_DECL2(CommandoSaveLoad, CommandoSaveLoadClass);



#endif