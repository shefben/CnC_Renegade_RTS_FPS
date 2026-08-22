#include "General.h"

#include "HashManagerClass.h"

void HashManagerClass::Init(unsigned int playercount)
{
	HashStatus = new PlayerHashStatus[playercount];
	memset(HashStatus,0,sizeof(PlayerHashStatus)*playercount);
}

HashManagerClass::~HashManagerClass()
{
	if (HashStatus)
	{
		delete[] HashStatus;
	}
}

void HashManagerClass::OnPlayerJoin(int playerId)
{
	memset(&HashStatus[playerId],0,sizeof(HashStatus[playerId]));
}

void HashManagerClass::OnMapLoad(int playerId)
{
	memset(&(HashStatus[playerId].MapHashes),0,sizeof(HashStatus[playerId].MapHashes));
}

void HashManagerClass::OnMapLoadDone(int playerId)
{
}

bool HashManagerClass::CheckSignedFile(int playerId, char* fileName, unsigned long hash, GlobalFileType type,bool signaturevalid)
{
	return true;
}

bool HashManagerClass::CheckGlobalFile(int playerId, char* fileName, unsigned long hash, GlobalFileType type)
{
	/*for(int i = 0; i < requests.Count(); i ++)
	{
		if(_stricmp(fileName, requests[i]->fileName) == 0 && requests[i]->playerId == playerId && requests[i]->type == type)
		{
			delete[] requests[i]->fileName;
			delete requests[i];

			requests.Delete(i);
			break;
		}
	}

	char* data;
	int length = 0;

	switch(type)
	{

		case Binary:
		{
			char* tempFileName = new char[strlen("data-clean\\")+strlen(fileName)+1];
			tempFileName[strlen("data-clean\\")+strlen(fileName)] = 0;
			strcpy(tempFileName, "data-clean\\");
			strcpy(tempFileName+strlen("data-clean\\"),fileName);

			FILE* handle = fopen(tempFileName, "r");

			if(!handle)
			{
				handle = fopen(fileName, "r");
			}
			length = ftell(handle);
			data = new char[length+1];
			data[length] = 0;
			if(fseek(handle, 0, SEEK_SET))
			{
				delete[] tempFileName;
				fclose(handle);
				return false;
			}
			fread(data, 1, length, handle);
			fclose(handle);
			delete[] tempFileName;
		}
		break;

		case Data:
		default:
		{
			char* tempFileName = new char[strlen("data-clean\\")+strlen(fileName)+1];
			tempFileName[strlen("data-clean\\")+strlen(fileName)] = 0;
			strcpy(tempFileName, "data-clean\\");
			strcpy(tempFileName+strlen("data-clean\\"),fileName);

			FILE* handle = fopen(tempFileName, "r");
	
			if(handle)
			{
				if(fseek(handle, 0, SEEK_END))
				{
					fclose(handle);
					delete[] tempFileName;
					return false;
				}
				length = ftell(handle);
				data = new char[length+1];
				data[length] = 0;
				if(fseek(handle, 0, SEEK_SET))
				{
					delete[] tempFileName;
					fclose(handle);
					return false;
				}
				fread(data, 1, length, handle);
				fclose(handle);
			}
			else
			{
				FileClass* file = _TheFileFactory->Get_File(fileName);
				if(!file)
				{
					delete[] tempFileName;
					return false;
				}
				length = file->Size();
				data = new char[length+1];
				data[length] = 0;
				file->Seek(0, SEEK_SET);
				file->Read(data, length);
				_TheFileFactory->Return_File(file);
			}
			delete[] tempFileName;
		}
		break;
	}

	char* hashString = new char[strlen(fileName)+length];

	memcpy(hashString, fileName, strlen(fileName));
	memcpy(hashString + strlen(fileName), data, length);

	unsigned long sHash = CRC_Memory((unsigned char*)hashString, strlen(fileName)+length, 0);

	delete[] hashString;

	hashString = new char[12];

	memcpy(hashString, (void*)&time, 4);
	memcpy(hashString + 4, (void*)&sHash, 4);
	memcpy(hashString + 8, (void*)&time, 4);

	sHash = CRC_Memory((unsigned char*)hashString, 12, sHash);

	delete[] data;

	if(sHash != hash)
	{
		FILE* fhandle = fopen("hashwhitelist.txt","r");
		char* whiteFilename = new char[256];
		whiteFilename[255] = 0;
		while(fgets(whiteFilename, 255, fhandle) != NULL)
		{
			whiteFilename[strlen(whiteFilename)-1] = 0;
			if(_stricmp(whiteFilename, fileName) == 0)
			{
				if(fgets(whiteFilename, 255,fhandle))
				{
					whiteFilename[strlen(whiteFilename)-1] = 0;
					sHash = atoi(whiteFilename);

					memcpy(hashString, (void*)&time, 4);
					memcpy(hashString + 4, (void*)&sHash, 4);
					memcpy(hashString + 8, (void*)&time, 4);

					sHash = CRC_Memory((unsigned char*)hashString, 12, sHash);
					if(sHash == hash)
					{
						return true;
					}
				}
			}
			else
			{
				fgets(whiteFilename, 255,fhandle);
			}
		}

		return false;
	}*/
	return true;
}

bool HashManagerClass::CheckMapFile(int playerId, char* fileName, unsigned long hash, MapFileType type)
{
	return true;
}

HashManagerClass HashManager;
