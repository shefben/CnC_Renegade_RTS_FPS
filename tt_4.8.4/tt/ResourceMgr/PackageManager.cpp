#include "General.h"
#include "PackageManager.h"

#include "Package.h"
#include "RawFileClass.h"
#include "VfsFile.h"
#include "MixFileFactoryClass.h"
#include "engine2.h"
#include "SimpleFileFactoryClass.h"
#pragma warning(disable: 6053) //Call to 'strncpy' might not zero-terminate string 'tmp'
#pragma warning(disable:6262) // Function uses '' bytes of stack: exceeds /analyze:stacksize



#define PACKAGEMANAGER_VERSION 'TTPM'
#define FILE_CLUSTER_SIZE (0x4000)
#define FILE_BUFFER_SIZE (0x4000)



PackageManager::PackageManager()
{
	filesystem = CreateNewInstance();
	if (!filesystem)
		MessageBoxW(0, L"Failed to create a VFS instance.", L"Fatal error", MB_ICONHAND);
	
	if (PathFileExistsW(L"data\\tt.vfs"))
	{
		if (!filesystem->Open(L"data\\tt.vfs", PACKAGEMANAGER_VERSION, true))
			MessageBoxW(0, L"Failed to open tt.vfs.", L"Fatal error", MB_ICONHAND);
	}
	else
	{
		if (!filesystem->Create(L"data\\tt.vfs", PACKAGEMANAGER_VERSION, FILE_CLUSTER_SIZE))
			MessageBoxW(0, L"Failed to create tt.vfs.", L"Fatal error", MB_ICONHAND);
	}
	
	load();
}



PackageManager::~PackageManager()
{
	save();
	
	for (int i = 0; i < packages.Count(); ++i)
		delete packages[i];

	delete filesystem;
}



FileClass* PackageManager::Get_File(const char* fileName)
{
	if (filesDirty)
		rebuild();

	char lowerFileName[MAX_PATH];
	strncpy(lowerFileName, fileName, MAX_PATH);
	_strlwr(lowerFileName);

	uint64* fileId = files.Get(lowerFileName);
	if (fileId)
		return new VfsFile(filesystem, *fileId);
	else
		return NULL;
}



void PackageManager::Return_File(FileClass* file)
{
	delete file;
}



void PackageManager::load()
{
	VfsFile file(filesystem, L"packages.dat");
	if (file.Open())
	{
		ChunkLoadClass fileData(&file);
		while (fileData.Open_Chunk())
		{
			TT_ASSERT(fileData.Cur_Chunk_ID() == 'PCKG');
			Package& package = *new Package(fileData);
			packages.Add(&package);
			fileData.Close_Chunk();

			package.update(*filesystem);
			activate(package);
		}
		file.Close();

		filesDirty = true;
		rebuild();
	}
	/*
	else
	{
		// TODO: TEST CODE
		Package& package = add("C&C_Hourglass", "1.0", "Westwood", Package::TYPE_NORMAL);
		importMix(package, "C&C_Hourglass.mix.IMP");
		package.update(*filesystem);
		activate(package);
		filesDirty = true;
		rebuild();
		save();
	}
	*/
}



void PackageManager::save()
{
	filesystem->Open(L"data\\tt.vfs", PACKAGEMANAGER_VERSION, false);

	VfsFile file(filesystem, L"packages.dat");
	file.Open(2);
	if (file.Is_Open())
	{
		ChunkSaveClass fileData(&file);
		for (int i = 0; i < packages.Count(); ++i)
		{
			fileData.Begin_Chunk('PCKG');
			packages[i]->save(fileData);
			fileData.End_Chunk();
		}
		file.Close();
	}

	filesystem->Open(L"data\\tt.vfs", PACKAGEMANAGER_VERSION, true);
}



void PackageManager::importMix(Package& package, const char* mixFileName)
{
	filesystem->Open(L"data\\tt.vfs", PACKAGEMANAGER_VERSION, false);

	MixFileFactoryClass mixFile(mixFileName, RenegadeBaseFileFactory);

	DynamicVectorClass<StringClass> fileNames;
	mixFile.Build_Filename_List(fileNames);

	for (int i = 0; i < fileNames.Count(); ++i)
	{
		const char* fileName = fileNames[i];

		FileClass* sourceFile = mixFile.Get_File(fileName);
		sourceFile->Open();
		TT_ASSERT(sourceFile->Is_Open());

		uint32 crc = 0x00000000;
		byte fileBuffer[FILE_BUFFER_SIZE];
		int fileBufferLength;
		while ((fileBufferLength = sourceFile->Read(fileBuffer, FILE_BUFFER_SIZE)) > 0)
			crc = CRC_Memory(fileBuffer, fileBufferLength, crc);
		
		wchar_t vfsFileName[MAX_PATH];
		swprintf(vfsFileName, MAX_PATH, L"files/%08X.%S", crc, fileName);
		
		uint64 fileId = filesystem->VFSGetFileUID(vfsFileName);
		if (fileId == VFS_INVALID_UID)
		{
			VFSFileHandle destinationFile = filesystem->VFSCreateFile(vfsFileName, FILE_CREATE_ALWAYS | FILE_OPEN_WRITE);
			TT_ASSERT(VFS_VALID_FILE(destinationFile));

			fileId = filesystem->VFSGetUIDFromFileHandle(destinationFile);
			
			sourceFile->Seek(0, SEEK_SET);
			int fileBufferLength2;
			while ((fileBufferLength2 = sourceFile->Read(fileBuffer, FILE_BUFFER_SIZE)) > 0)
				filesystem->VFSWrite(destinationFile, fileBuffer, fileBufferLength2, NULL);

			filesystem->VFSCloseFile(destinationFile);
			sourceFile->Close();
			mixFile.Return_File(sourceFile);
		}
		
		char lowerFileName[MAX_PATH];
		strncpy(lowerFileName, fileName, MAX_PATH);
		_strlwr(lowerFileName);

		package.addFile(fileId, lowerFileName, crc);
	}

	package.setIsIsCompleteDirty();
	
	filesystem->Open(L"data\\tt.vfs", PACKAGEMANAGER_VERSION, true);
}



Package& PackageManager::add(ChunkLoadClass& fileData)
{
	Package* package = new Package(fileData);
	packages.Add(package);
	return *package;
}



Package& PackageManager::add(const StringClass& name, const StringClass& version, const StringClass& author, Package::Type type)
{
	Package* package = new Package(name, version, author, type);
	packages.Add(package);
	return *package;
}



void PackageManager::remove(Package& package)
{
	TT_ASSERT(packages.Find_Index(&package) != -1);
	if (package.getIsActive())
		filesDirty = true;
	packages.Delete(&package);
}



void PackageManager::activate(Package& package)
{
	TT_ASSERT(packages.Find_Index(&package) != -1);
	package.setIsActive(true);
	filesDirty = true;
}



void PackageManager::deactivate(Package& package)
{
	TT_ASSERT(packages.Find_Index(&package) != -1);
	package.setIsActive(false);
	filesDirty = true;
}



void PackageManager::enable(Package& package)
{
	TT_ASSERT(packages.Find_Index(&package) != -1);
	package.setIsEnabled(true);
}



void PackageManager::disable(Package& package)
{
	TT_ASSERT(packages.Find_Index(&package) != -1);
	package.setIsEnabled(false);
}



Package* PackageManager::find(uint32 id)
{
	for (int packageIndex = 0; packageIndex < packages.Count(); ++packageIndex)
	{
		Package& package = *packages[packageIndex];
		if (package.getId() == id)
			return &package;
	}

	return NULL;
}



void PackageManager::rebuild()
{
	TT_ASSERT(filesDirty);
	files.Remove_All();
	for (int packageIndex = 0; packageIndex < packages.Count(); ++packageIndex)
	{
		Package& package = *packages[packageIndex];
		if (package.getIsActive())
		{
			TT_ASSERT(package.getIsComplete());
			package.collectFileMappings(files);
		}
	}

	filesDirty = false;
}
