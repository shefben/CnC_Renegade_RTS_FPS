#include "cNetEvent.h"
#include "bitstream.h"
#include "SysTimeClass.h"
#include "SimpleNetworkObjectFactoryClass.h"
#include "NetworkObjectClassId.h"
#include "NetworkObjectMgrClass.h"
#include "engine_vector.h"
#include "engine_io.h"
#include "HashManagerClass.h"
#include "cConnection.h"
#include "cNetwork.h"

enum FileLifetime {
	Global, //global files that get loaded once
	Map, //map files that get read every time the map is loaded
	SignedFile, //files (code and mix) that are signed
};

struct SignatureType {
	GlobalFileType globaltype;
	bool signaturevalid;
};

void CheckLoad(char* data, int dataLength, char* _fileName, GlobalFileType _type);
void CheckGlobal(char* data, int dataLength, char* _fileName, GlobalFileType _type);
void CheckMap(char* data, int dataLength, char* _fileName, MapFileType _type);
void CheckSignature(char *data, int dataLength, char* _fileName, GlobalFileType _type);
