#ifndef TT_INCLUDE__RESOURCEMGR_PACKAGEMANAGER_H
#define TT_INCLUDE__RESOURCEMGR_PACKAGEMANAGER_H



#include "Singleton.h"
#include "Package.h"
#include "engine_io.h"

#include "..\VFS\VFS\API\VFSInterface.h"



#define packageManager (Singleton<PackageManager>::getInstance())



class PackageManager :
	public FileFactoryClass
{

	IVFS* filesystem;
	SimpleDynVecClass<Package*> packages;
	HashTemplateClass<StringClass, uint64> files;
	bool filesDirty;

public:

	PackageManager();
	virtual ~PackageManager();

	virtual FileClass* Get_File(const char* fileName);
	virtual void Return_File(FileClass* file);

	void load();
	void save();

	void importMix(Package& package, const char* mixFileName);

	Package& add(ChunkLoadClass& fileData);
	Package& add(const StringClass& name, const StringClass& version, const StringClass& author, Package::Type type);
	void remove(Package& package);
	void activate(Package& package);
	void deactivate(Package& package);
	void enable(Package& package);
	void disable(Package& package);

	Package* find(uint32 id);
	const SimpleDynVecClass<Package*>& getPackages() const { return packages; }
	
	void rebuild();

	static void test();

};



#endif
