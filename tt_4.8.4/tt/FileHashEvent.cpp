#include "General.h"
#include "FileHashEvent.h"
#include "cPacket.h"
#include "NetworkObjectMgrClass.h"
#include "CombatManager.h"
#include "cNetwork.h"

bool Connected = false;
struct FileHashData {
	char* fileName;
	unsigned long hash;
	GlobalFileType type;
};

DynamicVectorClass<FileHashData *> StoredHashes;

class FileHashEvent:
	public cNetEvent
{
public:
	int playerId;
	char* fileName;
	unsigned long hash;
	FileLifetime lifetime;
	union {
		GlobalFileType globaltype;
		MapFileType maptype;
		SignatureType signaturetype;
	};
	virtual ~FileHashEvent();
	virtual unsigned int Get_Network_Class_ID() const;
	virtual void Import_Creation(BitStreamClass &oStream);
	virtual void Export_Creation(BitStreamClass &oStream);
	virtual void Act();

	__forceinline void InitGlobal(char* data, int dataLength, char* _fileName, GlobalFileType _type)
	{
		fileName = newstr(_fileName);
		globaltype = _type;
		lifetime = Global;
		hash = CRC_Memory((unsigned char*)data, dataLength,0);
		hash ^= 0xFFFFFFFF;
		Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);

		Set_Delete_Pending();
	}

	__forceinline void InitHash(int _hash, char* _fileName, GlobalFileType _type)
	{
		fileName = newstr(_fileName);
		globaltype = _type;
		lifetime = Global;
		hash = _hash;
		Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);

		Set_Delete_Pending();
	}

	__forceinline void InitMap(char* data, int dataLength, char* _fileName, MapFileType _type)
	{
		fileName = newstr(_fileName);
		maptype = _type;
		lifetime = Map;
		hash = CRC_Memory((unsigned char*)data, dataLength,0);
		hash ^= 0xFFFFFFFF;
		Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);	

		Set_Delete_Pending();
	}

	__forceinline void InitSigned(char* _fileName, GlobalFileType _type, bool signaturevalid)
	{
		fileName = newstr(_fileName);
		signaturetype.globaltype = _type;
		signaturetype.signaturevalid = signaturevalid;
		hash = 0;
		Set_Network_ID(NetworkObjectMgrClass::Get_New_Client_ID());
		Set_Object_Dirty_Bit(SERVER_HOST_ID, DB_CREATION, true);
		Set_Delete_Pending();
	}
};

void SendStoredEvents()
{
	if (!cNetwork::I_Am_Server() && Connected)
	{
		for (int i = 0;i < StoredHashes.Count();i++)
		{
			FileHashEvent *e = new FileHashEvent();
			e->InitHash(StoredHashes[i]->hash,StoredHashes[i]->fileName,StoredHashes[i]->type);
		}
	}
}

void CheckGlobal(char* data, int dataLength, char* _fileName, GlobalFileType _type)
{
	if (!cNetwork::I_Am_Server() && Connected)
	{
		FileHashEvent *e = new FileHashEvent();
		e->InitGlobal(data,dataLength,_fileName,_type);
	}
}

void CheckMap(char* data, int dataLength, char* _fileName, MapFileType _type)
{
	if (!cNetwork::I_Am_Server() && Connected)
	{
		FileHashEvent *e = new FileHashEvent();
		e->InitMap(data,dataLength,_fileName,_type);
	}
}

void CheckLoad(char* data, int dataLength, char* _fileName, GlobalFileType _type)
{
	FileHashData *f = new FileHashData;
	f->fileName = newstr(_fileName);
	f->type = _type;
	f->hash = CRC_Memory((unsigned char*)data, dataLength,0);
	f->hash ^= 0xFFFFFFFF;
	StoredHashes.Add(f);
}

void CheckSignature(char *data, int dataLength, char* _fileName, GlobalFileType _type)
{
}

FileHashEvent::~FileHashEvent()
{
	delete[] fileName;
}

unsigned int FileHashEvent::Get_Network_Class_ID() const
{
	return NET_FileHashEvent;
}

void FileHashEvent::Import_Creation(BitStreamClass &oStream)
{
	cNetEvent::Import_Creation(oStream);

	fileName = new char[261];
	oStream.Get_Terminated_String(fileName, 261);
	oStream.Get(hash);
	oStream.Get((uint32&)lifetime);

	if (lifetime == Global)
		oStream.Get((uint32&)globaltype);
	else if (lifetime == Map)
		oStream.Get((uint32&)maptype);
	else
	{
		oStream.Get((uint32&)signaturetype.globaltype);
		oStream.Get((uint32&)signaturetype.signaturevalid);
	}
	cPacket& Packet = (cPacket&)oStream;
	playerId = Packet.Get_Sender_Id();
	
	Act();
}


void FileHashEvent::Export_Creation(BitStreamClass &oStream)
{
	cNetEvent::Export_Creation(oStream);

	oStream.Add_Terminated_String(fileName);
	oStream.Add((uint32)hash);
	oStream.Add((uint32)lifetime);
	if (lifetime == Global)
	{
		oStream.Add((uint32)globaltype);
	}
	else if (lifetime == Map)
	{
		oStream.Add((uint32)maptype);
	}
	else
	{
		oStream.Add((uint32)signaturetype.globaltype);
		oStream.Add((uint32)signaturetype.signaturevalid);
	}
	Set_Delete_Pending();
}

void FileHashEvent::Act()
{
	if(CombatManager::I_Am_Server())
	{
		hash ^= 0xFFFFFFFF;
		if (lifetime == Global)
		{
			if (!HashManager.CheckGlobalFile(playerId, fileName, hash, globaltype))
			{
				cNetwork::Server_Kill_Connection(playerId);
				cNetwork::Cleanup_After_Client(playerId);
			}
		}
		else if (lifetime == Map)
		{
			if (!HashManager.CheckMapFile(playerId, fileName, hash, maptype))
			{
				cNetwork::Server_Kill_Connection(playerId);
				cNetwork::Cleanup_After_Client(playerId);
			}
		}
		else
		{
			if (!HashManager.CheckSignedFile(playerId, fileName, hash,signaturetype.globaltype,signaturetype.signaturevalid))
			{
				cNetwork::Server_Kill_Connection(playerId);
				cNetwork::Cleanup_After_Client(playerId);
			}
		}
	}
}



SimpleNetworkObjectFactoryClass<FileHashEvent, NET_FileHashEvent> fileHashEventFactory;
