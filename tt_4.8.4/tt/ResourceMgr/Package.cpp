#include "General.h"
#include "Package.h"

#include "RawFileClass.h"
#include "MixFileFactoryClass.h"
#include "engine2.h"
#include "engine_io.h"

#include "..\VFS\VFS\API\VFSBase.h"



Package::Package(const StringClass& _name, const StringClass& _version, const StringClass& _author, Type _type) :
	name(_name),
	version(_version),
	author(_author),
	type(_type)
{
	state.isActive = false;
	state.isEnabled = false;
	state.isComplete = true;
	state.isIsCompleteDirty = false;
}



Package::Package(ChunkLoadClass& fileData)
{
	load(fileData);
}



Package::~Package()
{
}



void Package::load(ChunkLoadClass& fileData)
{
	while (fileData.Open_Chunk())
	{
		switch (fileData.Cur_Chunk_ID())
		{
			case 'DATA':
				fileData.Read(name);
				fileData.Read(version);
				fileData.Read(author);
				fileData.SimpleRead(type);
				fileData.SimpleRead(state);
				break;

			case 'FILE':
			{
				File file;
				file.id = (uint64)VFS_INVALID_FILE;
				fileData.SimpleRead(file.hash);
				fileData.Read(file.name);
				files.Add(file);
				break;
			}

			/*
			default:
				TT_UNREACHABLE;
			*/
		}
		fileData.Close_Chunk();
	}

	state.isActive = false;
	setIsIsCompleteDirty();
}



void Package::save(ChunkSaveClass& fileData)
{
	fileData.Begin_Chunk('DATA');
	fileData.Write(name);
	fileData.Write(version);
	fileData.Write(author);
	fileData.SimpleWrite(type);
	fileData.SimpleWrite(state);
	fileData.End_Chunk();

	for (int i = 0; i < files.Count(); ++i)
	{
		fileData.Begin_Chunk('FILE');
		fileData.SimpleWrite(files[i].hash);
		fileData.Write(files[i].name);
		fileData.End_Chunk();
	}
}



// Case sensitive
Package::File* Package::findFile(const StringClass& fileName)
{
	for (int i = 0; i < files.Count(); ++i)
		if (files[i].name == fileName)
			return &files[i];
	return NULL;
}



void Package::addFile(uint64 fileId, const StringClass& fileName, uint32 fileHash)
{
	TT_ASSERT(!findFile(fileName));
	TT_ASSERT(!state.isActive);

	File file;
	file.id = fileId;
	file.hash = fileHash;
	file.name = fileName;
	files.Add(file);

	state.isComplete = state.isComplete && file.id != VFS_INVALID_UID;
}



void Package::removeFile(File& file)
{
	TT_ASSERT(!state.isActive);

	if (!state.isComplete && file.id == VFS_INVALID_UID)
		setIsIsCompleteDirty();

	int index = (&file - &files[0]) / sizeof(File);
	TT_ASSERT(index >= 0 && index < files.Count());
	files.Delete(index);
}



void Package::update(IVFS& filesystem)
{
	TT_ASSERT(!state.isActive);
	TT_ASSERT(state.isIsCompleteDirty);

	state.isComplete = true;

	for (int i = 0; i < files.Count(); ++i)
	{
		File& file = files[i];
		
		wchar_t vfsFileName[MAX_PATH];
		swprintf(vfsFileName, MAX_PATH, L"files/%08X.%S", file.hash, file.name);

		if (file.id == (uint64)VFS_INVALID_FILE)
		{
			file.id = filesystem.VFSGetFileUID(vfsFileName);

			if (file.id == (uint64)VFS_INVALID_FILE)
				state.isComplete = false;
		}
	}

	state.isIsCompleteDirty = false;
}



void Package::collectFileMappings(HashTemplateClass<StringClass, uint64>& mappings) const
{
	for (int fileIndex = 0; fileIndex < files.Count(); ++fileIndex)
	{
		const File& file = files[fileIndex];
		TT_ASSERT(file.id != (uint64)VFS_INVALID_FILE);
		mappings.Insert(file.name, file.id);
	}
}