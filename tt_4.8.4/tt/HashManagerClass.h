#ifndef TT_INCLUDE_HASHMANAGERCLASS_H
#define TT_INCLUDE_HASHMANAGERCLASS_H
#include "engine_vector.h"
#include "FileHash.h"

struct PlayerHashStatus {
	bool GlobalHashes[MaxGlobalHash];
	bool MapHashes[MaxMapHash];
};

class HashManagerClass
{
	PlayerHashStatus *HashStatus;
public:
	void Init(unsigned int playercount);
	~HashManagerClass();
	void OnPlayerJoin(int playerId);
	void OnMapLoad(int playerId);
	void OnMapLoadDone(int playerId);
	bool CheckGlobalFile(int playerId, char* fileName, unsigned long hash, GlobalFileType type);
	bool CheckMapFile(int playerId, char* fileName, unsigned long hash, MapFileType type);
	bool CheckSignedFile(int playerId, char* fileName, unsigned long hash, GlobalFileType type, bool signaturevalid);
};

extern HashManagerClass HashManager;
#endif
