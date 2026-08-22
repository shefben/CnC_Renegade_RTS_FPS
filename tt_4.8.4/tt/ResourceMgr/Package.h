#ifndef TT_INCLUDE__RESOURCEMGR_PACKAGE_H
#define TT_INCLUDE__RESOURCEMGR_PACKAGE_H



#include "HashTemplateClass.h"
#include "engine_vector.h"
#include "engine_string.h"

#include "..\VFS\VFS\API\VFSInterface.h"



class ChunkLoadClass;
class ChunkSaveClass;

unsigned long CRC_Stringi(char  const*, unsigned long = 0); // TODO: Move



class Package :
	public NoEqualsClass<Package>
{

public:

	enum Type
	{
		TYPE_SECRET, // Packages that are always enabled and never displayed.
		TYPE_HIDDEN, // Packages that are hidden but can be shown.
		TYPE_NORMAL, // Packages that are always shown.
	};

private:

	struct File :
		public NoEqualsClass<Package::File>
	{
		uint64 id;
		uint32 hash;
		StringClass name;
	};

	StringClass name;
	StringClass version;
	StringClass author;
	Type type;

	struct
	{
		bool isActive : 1; // True if this package is loaded (enabled and allowed or forced by the server)
		bool isEnabled : 1; // True if this package is enabled in the package manager.
		bool isComplete : 1; // True if there are no missing files.
		bool isIsCompleteDirty : 1; // isComplete
	} state;
	
	DynamicVectorClass<File> files;

public:

	Package(const StringClass& _name, const StringClass& _version, const StringClass& _author, Type _type);
	Package(ChunkLoadClass& fileData);
	~Package();

	void load(ChunkLoadClass& fileData);
	void save(ChunkSaveClass& fileData);
	void loadFileIds(const IVFS& filesystem);

	File* findFile(const StringClass& fileName);
	void addFile(uint64 fileId, const StringClass& fileName, uint32 fileHash);
	void removeFile(File& file);
	void update(IVFS& filesystem);
	void collectFileMappings(HashTemplateClass<StringClass, uint64>& mappings) const;

	uint32 getId() const { return CRC_Stringi(name); }
	StringClass getDownloadUrl() const { StringClass downloadUrl; downloadUrl = "http://www.blackintel.org/resources/"; downloadUrl += name; return downloadUrl; }
	bool getIsEnabled() const { return state.isEnabled; }
	void setIsEnabled(bool _isEnabled) { state.isEnabled = _isEnabled; }
	bool getIsActive() const { return state.isActive; }
	void setIsActive(bool _isActive) { TT_ASSERT(state.isComplete); state.isActive = _isActive; }
	bool getIsComplete() const { TT_ASSERT(!(state.isComplete && state.isIsCompleteDirty)); return state.isComplete; }
	//void setIsComplete(bool _isComplete) { state.isComplete = _isComplete; state.isIsCompleteDirty = false; }
	bool getIsIsCompleteDirty() const { return state.isIsCompleteDirty; }
	void setIsIsCompleteDirty() { state.isComplete = false; state.isIsCompleteDirty = true; }

};


#endif
