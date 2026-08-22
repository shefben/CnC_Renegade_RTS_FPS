#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "../VFS/VFS.h"
#include "../VFS/API/VFSBase.h"
#include "../VFS/API/VFSAdvanced.h"
#include "../VFS/API/VFSCryptography.h"

#include <mmsystem.h>
#include "STB.h"
#include "RPC VFS_h.h"
#include "../RPC VFS/RPC VFS.h"
#include "../Shared/StdLib.h"
#include "../Shared/Lists.h"
#include "../Shared/Hash.h"

#ifdef _NO_TEST_

const char *g_pLevel =
	"                                                                            " \
	"                                                                            " \
	"                                                                            " \
	"                                                                            " \
	"                                                                            " \
	"                                                                            " \
	"                                                                            ";
#define VFS_TEST
#ifdef VFS_TEST
#ifndef _USRDLL
void RecurPrint(CEntry *pEntry, int level){

	/*GenericSLNode<CEntry> *pNode;
	printf("%.*s %s %d %S\n", \
		level, g_pLevel, \
		((pEntry->GetElement()->m_Type == MFT_ELEMENT_DIRECTORY) ? "Dir " : ((pEntry->GetElement()->m_Type == MFT_ELEMENT_FILE) ? "File" : "Error")), \
		pEntry->GetElement()->m_Type, \
		pEntry->GetElement()->m_Name);
	pNode = pEntry->GetFirstChild();
	while (pNode){
		RecurPrint(pNode->m_pData, level+1);
		pNode = pNode->m_pNext;
	}//*/
}
#endif //_USRDLL

int VFS_CALL EnumCallback(PVFSFILEINFO pFileInfo, ptr wParam, ptr lParam){
	printf("%S\n", pFileInfo->m_pFullName);
	/*if (pFileInfo->m_Type == VFS_TYPE_FILE)
		printf("File: size: %lu bytes path: %S\n", pFileInfo->m_Size, pFileInfo->m_pFullName);
	else if (pFileInfo->m_Type == VFS_TYPE_DIRECTORY)
		printf("Directory path: %S\n", pFileInfo->m_pFullName);
	else if (pFileInfo->m_Type == VFS_TYPE_INTERNAL)
		printf("Internal (shouldn't show here)\n");
	else 
		printf("Error: type %d\n", pFileInfo->m_Type);//*/
	return VFS_ENUM_CONTINUE | VFS_ENUM_MY_AND_VALUE;
}

const char g_Alphabet[] = \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
	"abcdefghijklmnopqrstuvwxyz";

void Test(bool create){
#ifndef _USRDLL
	VFS vfs;
	VFSFileHandle hFile, hFile2;
	size_32 r;
	char lBuf[sizeof(g_Alphabet)];
	//if (create){
	if (false || vfs.Create(L"C:/VFSTest/test", 'TSET', 4)){
		printf("Created VFS!\n");
		hFile = vfs.VFSCreateFile(L"data/misc/lol.txt", FILE_CREATE_ALWAYS | FILE_OPEN_WRITE);
		if (!hFile){
			printf("Failed to create file: %d\n", vfs.GetLastError());
		} else {
			r = vfs.VFSWrite(hFile, g_Alphabet, sizeof(g_Alphabet), NULL);
			printf("Wrote %d bytes to file\n", r);
			vfs.VFSSeek(hFile, 26, SEEK_SET);
			r = vfs.VFSWrite(hFile, g_Alphabet, 26, NULL);
			printf("Wrote %d bytes to file\n", r);
			vfs.VFSSeek(hFile, -26, SEEK_CUR);
			r = vfs.VFSWrite(hFile, g_Alphabet+26, 26/2, NULL);
			printf("Wrote %d bytes to file\n", r);

			/*vfs.VFSSeek(hFile, 26, SEEK_SET);
			if (vfs.VFSSetEndOfFile(hFile))
				printf("Set end of file at %d\n", vfs.VFSSize(hFile));
			else
				printf("Failed to set end of file\n");//*/
			vfs.VFSCloseFile(hFile);
		}

		
		hFile2 = vfs.VFSCreateFile(L"data/misc2/lol2.txt", FILE_CREATE_ALWAYS | FILE_OPEN_WRITE);
		if (!hFile2){
			printf("Failed to create file: %d\n", vfs.GetLastError());
		} else {
			r = vfs.VFSWrite(hFile2, g_Alphabet+26, 26, NULL);
			printf("Wrote %d bytes to file\n", r);
			vfs.VFSCloseFile(hFile2);
		}

		/*if (hFile){
			vfs.VFSSeek(hFile, 26, SEEK_SET);
			if (vfs.VFSSetEndOfFile(hFile))
				printf("Set end of file at %d\n", vfs.VFSSize(hFile));
			else
				printf("Failed to set end of file\n");
			vfs.VFSCloseFile(hFile);
		}//*/

		
		hFile2 = vfs.VFSCreateFile(L"data/misc/lol3.txt", FILE_CREATE_ALWAYS | FILE_OPEN_WRITE);
		if (!hFile2){
			printf("Failed to create file: %d\n", vfs.GetLastError());
		} else {
			r = vfs.VFSWrite(hFile2, g_Alphabet+13, 26, NULL);
			printf("Wrote %d bytes to file\n", r);
			vfs.VFSCloseFile(hFile2);
		}

		printf("\n\n\n");
		vfs.VFSEnum(NULL, EnumCallback, NULL, NULL, VFS_ENUM_ALL);
		printf("\n\n\n");
		printf("Del dir: %d\n", vfs.VFSDeleteDirectory(L"data/misc2/"));


		RecurPrint(&vfs.m_Root_Dir, 0);

		//vfs.DumpFTInfo();
		vfs.Close();
	} else
		printf("Failed to create VFS: %d!\n", vfs.GetLastError());
	printf("\n\n");
	return;
	//}
	//if (vfs.Create(L"C:/VFSTest/test", 'TSET', 4)){
	if (vfs.Open(L"C:/VFSTest/test", 'TSET', false)){
		printf("Opened VFS!\n");
		hFile = vfs.VFSCreateFile(L"data/misc/lol.txt", FILE_OPEN_EXISTING | FILE_OPEN_READ);
		if (!hFile){
			printf("Failed to open file: %d\n", vfs.GetLastError());
		} else {
			printf("Size: %ld\n", vfs.VFSSize(hFile));
			//r = vfs.VFSRead(hFile, lBuf, vfs.VFSSize(hFile) > sizeof(lBuf) ? sizeof(lBuf) : vfs.VFSSize(hFile));
			r = vfs.VFSRead(hFile, lBuf, sizeof(lBuf), NULL);
			if (r == 0)
				printf("Failed to read from file: %d\n", vfs.GetLastError());
			else
				printf("Read %d bytes from file\n", r);
			printf("Data: %.*s\n", r, lBuf);
			vfs.VFSCloseFile(hFile);
		}
		hFile = vfs.VFSCreateFile(L"data/misc/lol2.txt", FILE_OPEN_EXISTING | FILE_OPEN_READ);
		if (!hFile){
			printf("Failed to open file: %d\n", vfs.GetLastError());
		} else {
			printf("Size: %ld\n", vfs.VFSSize(hFile));
			//r = vfs.VFSRead(hFile, lBuf, vfs.VFSSize(hFile) > sizeof(lBuf) ? sizeof(lBuf) : vfs.VFSSize(hFile));
			r = vfs.VFSRead(hFile, lBuf, sizeof(lBuf), NULL);
			if (r == 0)
				printf("Failed to read from file: %d\n", vfs.GetLastError());
			else
				printf("Read %d bytes from file\n", r);
			printf("Data: %.*s\n", r, lBuf);
			vfs.VFSCloseFile(hFile);
		}
		vfs.DumpFTInfo();
		hFile = vfs.VFSCreateFile(L"data/misc/lol3.txt", FILE_OPEN_EXISTING | FILE_OPEN_READ);
		if (!hFile){
			printf("Failed to open file: %d\n", vfs.GetLastError());
		} else {
			printf("Size: %ld\n", vfs.VFSSize(hFile));
			//r = vfs.VFSRead(hFile, lBuf, vfs.VFSSize(hFile) > sizeof(lBuf) ? sizeof(lBuf) : vfs.VFSSize(hFile));
			r = vfs.VFSRead(hFile, lBuf, sizeof(lBuf), NULL);
			if (r == 0)
				printf("Failed to read from file: %d\n", vfs.GetLastError());
			else
				printf("Read %d bytes from file\n", r);
			printf("Data: %.*s\n", r, lBuf);
			vfs.VFSCloseFile(hFile);
		}

		/*if (!vfs.Allocate_Directory(L"/test/moretest",ROOT_DIRECTORY, &UID))
			printf("Failed to alloc dir!\n");
		if (!vfs.Locate_Directory(L"/test/moretest", NULL, 0, false))
			printf("Failed to locate dir!\n");//*/

		/*if (!vfs.Locate_Directory(L"/rofl/lol/dir", NULL, 0, false, NULL))
			printf("Failed to locate dir!\n");
		else
			printf("Located dir!\n");

		
		if (!vfs.Locate_File(L"/rofl/wee/wtf/mens/lol.dat", false, NULL))
			printf("Failed to locate file!\n");
		else
			printf("Found file!\n");//*/

		RecurPrint(&vfs.m_Root_Dir, 0);

		//vfs.DumpFTInfo();
		vfs.Close();
	} else
		printf("Failed to open VFS: %d!\n", vfs.GetLastError());
#ifdef LOG_POOL_DATA
	printf("Allocated objects: %d\n", g_Allocated_Objects);
#endif //LOG_POOL_DATA
#endif //_WINDLL
}

#define TEST_FILE	L"apbbetafinal.exe"

void VFS_CALL ProgressCallback(const ptr pData, int type, size_64 progress, size_64 total, ptr pCookie){
	if (type == PROGRESS_TYPE_START)
		printf("Found %lu files!\n", total);
	else if (type == PROGRESS_TYPE_NEXT)
		printf("%I64u/%I64u: %S\n", progress, total, pData);
		//printf("%I64u/%I64u: %S\n", (size_64)0, (size_64)0, pData);
}


bool VFS_CALL GetAESKey(int KeyIndex, ptr pKey, ptr pIV){
	if (KeyIndex != 0) return false;
	memset(pKey, 0, 16);
	memset(pIV, 0, 16);
	memcpy(pKey, "Rofl", 4);
	return true;
}


void EmbedProgress(const ptr pData, int type, size_64 progress, size_64 total, ptr pCookie){
	if (type == PROGRESS_TYPE_NEXT){
		wprintf(L"%s\n", pData);
	}
}
void Test2(){
#ifndef _USRDLL
	FUNC_TIME();
	char lBuf[64];
	VFS vfs;
	bool b;
	DWORD u;
	VFSFILEINFO finfo;
	PROGESSCB cb;
	cb.m_pCB = ProgressCallback;
	cb.m_pCookie = NULL;
	ptr info;
	VFSFLAGS flags;
	VFSFileHandle hFile;
	//PROGESSCB cb;
	cb.m_pCB = EmbedProgress;
	cb.m_pCookie = NULL;

	//if (vfs.Open(L"C:/VFSTest/test.vfs", 'TSET', true)){
	if (vfs.Create(L"C:/VFSTest/Arua.vfs", 'AURA', 0x4000)){
		/*vfs.SetOption(VFS_OPTION_AES_KEYCB, GetAESKey, sizeof(pfGetAESKey));
		hFile = vfs.VFSCreateFile(L"lol.txt", FILE_CREATE_ALWAYS | FILE_OPEN_WRITE | FILE_OPEN_AES);
		if (VFS_VALID_FILE(hFile)){
			vfs.VFSSetFileOption(hFile, FILE_OPTION_AES_KEY, ULongToPtr(0), sizeof(int));
			vfs.VFSWrite(hFile, g_Alphabet, 26*2, NULL);
			vfs.VFSCloseFile(hFile);
		}

		
		hFile = vfs.VFSCreateFile(L"lol.txt", FILE_OPEN_EXISTING | FILE_OPEN_READ/* | FILE_OPEN_AES);
		if (VFS_VALID_FILE(hFile)){
			vfs.VFSSetFileOption(hFile, FILE_OPTION_AES_KEY, ULongToPtr(0), sizeof(int));
			//vfs.VFSWrite(hFile, "lol", 4, NULL);
			vfs.VFSRead(hFile, lBuf, vfs.VFSSize(hFile), NULL);
			vfs.VFSCloseFile(hFile);
		}//*/
		vfs.VFSEmbedFile(L"C:/VFSTEST/SYSTEMFUNC.LUA", L"SYSTEMFUNC.LUA", true, NULL);
		printf("%d\n", (size_32)vfs.VFSFileSize(L"SYSTEMFUNC.LUA"));
	//if (vfs.Open(L"C:/VFSTest/Arua.vfs", 'TSET', true)){
		/*u = timeGetTime();
		res = vfs.VFSEmbedFile(L"E:/Laptop/" TEST_FILE, L"TEST/test.bin", false);
		printf("VFSEmbedFile took %.3f seconds\n", ((float)(timeGetTime()-u))/1000);

				
		u = timeGetTime();
		res = vfs.VFSExtractFile(L"TEST/test.bin", L"E:/Laptop/2" TEST_FILE);
		printf("VFSExtractFile took %.3f seconds\n", ((float)(timeGetTime()-u))/1000);

		
		printf("VFS::VFSDeleteFile() %s\n", vfs.VFSDeleteFile(L"TEST/test.bin") != VFS_DELETE_FAILED ? "succeeded" : "failed");

		u = timeGetTime();
		res = vfs.VFSEmbedFile(L"E:/Laptop/" TEST_FILE, L"TEST/test2.bin", false);
		printf("VFSEmbedFile took %.3f seconds\n", ((float)(timeGetTime()-u))/1000);

				
		u = timeGetTime();
		res = vfs.VFSExtractFile(L"TEST/test2.bin", L"E:/Laptop/3" TEST_FILE);		
		printf("VFSExtractFile took %.3f seconds\n", ((float)(timeGetTime()-u))/1000);//*/
		//vfs.VFSEmbedDir(L"C:\\Users\\Admin\\Downloads\\ssb\\ssb\\Super Smash Bros. Brawl Themes", L"BrawlTest", false, NULL);
		//u = timeGetTime();

		//vfs.VFSEmbedDir(L"E:\\Renegade", L"Renegade", false, &cb);
		//vfs.VFSExtractDir(L"Renegade", L"E:\\Renegade2", true, &cb);
		//vfs.VFSExtractFile(L"Renegade\\Data\\always.dat", L"E:\\Renegade2\\always.dat", NULL);
		//b = vfs.VFSEmbedDir(L"E:\\AR files\\ROSE files", L"", false, NULL);

		//b = vfs.VFSEmbedDir(L"E:\\AR files\\ROSE files\\Shaders", L"Shaders", false, &cb);
		//b = vfs.VFSEmbedDir(L"E:\\AR files\\ROSE files", L"", false, &cb);

		//vfs.VFSEmbedDir(L"D:\\Old\\H\\Neuer Ordner", L"Renegade", false, &cb);
		//vfs.VFSEmbedDir(L"D:\\Old\\H\\Neuer Ordner", L"Renegade", false, &cb);
		///fs.VFSEmbedDir(L"E:\\Renegade\\Data", L"Data", false, NULL);
		//printf("VFSEmbedDir took %.3f seconds\n", ((float)(timeGetTime()-u))/1000);
		//if ((info = vfs.VFSGetFileData(L"Shaders")) && vfs.VFSQueryFileInfo(&finfo, info, NULL, 0)){
			//wprintf(L"Got info: %s\n", finfo.m_pFullName);
			//flags.m_Flags = finfo.m_Flags;
			//vfs.VFSSetNoEnum(info, MFT_ELEMENT_NO_ENUM);
		//}
		//if ((info = vfs.VFSGetFileData(L"Shaders")) && vfs.VFSQueryFileInfo(&finfo, info, NULL, 0)){
			
			//flags.m_Flags = finfo.m_Flags;
			//wprintf(L"Enum values for %s: %d\n", finfo.m_pFullName, flags.m_No_Enum);
		//}
		//vfs.VFSEnum(NULL, EnumCallback, NULL, VFS_ENUM_ALL);

		vfs.Close();
	}
#endif //_USRDLL
}

void StressTest(){
#ifndef _USRDLL
	VFS vfs;
	short buf[0x1000];
	short buf2[0x1000];
	int i;
	VFSFileHandle hFile;
	size_32 offset;

	if (vfs.Create(L"C:/VFSTest/Stress.vfs", 'AURA', 0x4000)){
		for (i = 0; i < 0x1000; i++)
			buf[i] = i;

		hFile = vfs.VFSCreateFile(L"lol.txt", FILE_CREATE_ALWAYS | FILE_OPEN_WRITE);
		if (VFS_VALID_FILE(hFile)){
			vfs.VFSWrite(hFile, buf, sizeof(buf), NULL);
			vfs.VFSCloseFile(hFile);
		}

		hFile = vfs.VFSCreateFile(L"lol.txt", FILE_OPEN_EXISTING | FILE_OPEN_READ);
		if (VFS_VALID_FILE(hFile)){
			vfs.VFSRead(hFile, buf2, sizeof(buf2), NULL);
			for (i = 0; i < 0x1000; i++){
				offset = ((i*0x1000)^(-i))%0x1000;
				vfs.VFSSeek(hFile, (ssize_64)offset, SEEK_SET);
				vfs.VFSRead(hFile, &buf2[offset/sizeof(short)], 2, NULL);
			}
			vfs.VFSCloseFile(hFile);
		}

		printf("%d\n", memcmp(buf, buf2, sizeof(buf)));
		vfs.Close();
	}
#endif //_USRDLL
}


void STBTest(){
#ifndef _USRDLL
	char lBuf[8];
	IVFS *pVFS = new VFS;
	classSTB stb(pVFS);
	printf("lol");
	VFSFileHandle hFile;

	//if (vfs.Open(L"F:/AruaROSE/Arua.vfs", 'AURA', false)){
	//if (pVFS->Create(L"F:/AruaROSE/Arua2.vfs", 'AURA', 0x4000)){
	if (pVFS->Open(L"F:/AruaROSE/Arua.vfs", 'AURA', true)){
		hFile = pVFS->VFSCreateFile(L"\\//3DDATA\\MOTION/\\AVATAR\\s_spinattack01_m1.ZMO", FILE_OPEN_EXISTING | FILE_OPEN_READ | FILE_OPEN_MEMORY);
		if (VFS_VALID_FILE(hFile)){
			printf("lol");
			pVFS->VFSCloseFile(hFile);
		}
		pVFS->Close();
	}
	delete pVFS;
#endif //_USRDLL
}

void RPC_Test(){
	RPC_WSTR pStringBinding;
	RPC_STATUS status;
	RV_VFSFileHandle hFile;
	RV_VFSFILEINFO info;
	wchar_t lBuf[32*1024] = L"LOL";

	pStringBinding = NULL;

	hFile = NULL;

	status = RpcStringBindingComposeW(NULL, (RPC_WSTR)L"ncalrpc", NULL, (RPC_WSTR)L"lolwtf", NULL, &pStringBinding);

	if (status != RPC_S_OK){
		printf("Failed to make binding string!\n");
		goto __exit;
	}

	status = RpcBindingFromStringBinding(pStringBinding, &RPC_VFS_v1_0_c_ifspec);

	if (status != RPC_S_OK){
		printf("Failed to make binding handle!\n");
		goto __exit;
	}
	//VFS_RPC_Open(BindingHandle, L"Rofls", 12345, (RV_BOOL)TRUE);
	memset(&info, 0, sizeof(info));
	info.m_RefCount = 123;
	VFS_RPC_VFSQueryFileInfo(RPC_VFS_v1_0_c_ifspec, &info, 12345, lBuf, 32*1024);
	//VFS_RPC_VFSQueryFileInfo(RPC_VFS_v1_0_c_ifspec, (RV_FileData)ULongToPtr(0x12345678), lBuf, 32*1024);
	printf("%d\n", (int)info.m_RefCount);
	VFS_RPC_VFSCreateFile(RPC_VFS_v1_0_c_ifspec, L"vaginas", 100, &hFile);
	RV_size_32 cs;
	VFS_RPC_VFSGetClusterSize(RPC_VFS_v1_0_c_ifspec, &cs);
	printf("%u", cs);

__exit:
	if (pStringBinding) RpcStringFreeW(&pStringBinding);
	RpcBindingFree(&RPC_VFS_v1_0_c_ifspec);
}

void RPCTest2(){
#ifndef _USRDLL
	FUNC_TIME();
	RPCVFS vfs;
	//VFS vfs;
	IVFS* pVFS = &vfs;
	VFSFileHandle hFile;
	unsigned long u;
	char lBuf[16] = "rofl fag omglol";
	//if (pVFS->Create(L"C:\\VFSTest\\OMG.VFS", 'VFSS', 16*1024)){
	if (pVFS->Open(L"lolwtf", 'VFSS', false)){
		printf("Opened VFS!\n");
		{
			hFile = pVFS->VFSCreateFile(L"ROFL.txt", FILE_CREATE_ALWAYS | FILE_OPEN_READ_WRITE);
			CFunctionTime wt("Writing time");
			if (VFS_VALID_FILE(hFile)){
				printf("Made file!\n");
				for (u = 0; u < 100000; u++){
					pVFS->VFSWrite(hFile, lBuf, sizeof(lBuf), NULL);
				}
				//pVFS->VFSWrite(hFile, L"LMAO", 10, NULL);
				pVFS->VFSCloseFile(hFile);
			}
		}

		{
			CFunctionTime wt("Reading time");
			hFile = pVFS->VFSCreateFile(L"ROFL.txt", FILE_OPEN_EXISTING | FILE_OPEN_READ_WRITE);
			if (VFS_VALID_FILE(hFile)){
				printf("Opened file!\n");
				//pVFS->VFSWrite(hFile, L"LMAO", 5, NULL);
				//pVFS->VFSRead(hFile, lBuf, 16, NULL);
				
				for (u = 0; u < 100000; u++){
					pVFS->VFSRead(hFile, lBuf, sizeof(lBuf), NULL);
				}
				printf("-> %s\n", lBuf);
				pVFS->VFSCloseFile(hFile);
			}
		}
		pVFS->Close();
	}
#endif //!_USRDLL
}


struct LITObject {
	char		m_Name[512];
	int			m_NameOffset;
	int			m_X;
	int			m_Y;
	int			m_ObjIdx;
	int			m_Size;
	int			m_Model;
	int			m_SubObj;
	LITObject*	m_pNext;
	int			m_Count;
};

LITObject* FindByObj(HashList<LITObject*, int> *pList, int Obj){
	HashNode<LITObject*, int> *pNode;
	if (!(pNode = pList->Search(Obj))) return false;
	do {
		if (pNode->m_Data->m_ObjIdx == Obj)
			return pNode->m_Data;
	} while ((pNode = pList->SearchContinue(pNode, Obj)));
	return NULL;
}

void ProcFile(const char *pFullName, int offset, HashList<LITObject*, int> *pCnstList, HashList<LITObject*, int> *pDecoList, int *pCnstCount, int *pDecoCount){
	char type[6];
	LITObject *pObj, *pObj2;
	int x, y, idx, model, sub, size, i;

	HashList<LITObject*, int> *pList;

	//32_32__DECO__101_188_0_64.dds
	if (sscanf(pFullName+offset, "%d_%d__%04s__%d_%d_%d_%d.dds",
		&x, &y, type, &idx, &model, &sub, &size) != 7) return;

	if (strcmp(type, "DECO") != 0 && strcmp(type, "CNST") != 0) return;
	pList = strcmp(type, "DECO") == 0 ? pDecoList : pCnstList;
	pObj2 = FindByObj(pList, idx);
	pObj = new LITObject;

	pObj->m_NameOffset = offset;
	StrCpy(pObj->m_Name, m_Name);
	pObj->m_X = x;
	pObj->m_Y = y;
	pObj->m_ObjIdx = idx;
	pObj->m_Model = model;
	pObj->m_Size = size;
	pObj->m_SubObj = sub;
	pObj->m_Count = 1;

}

void TestCompile(){
	HashList<LITObject*, int> list(0x80);
	char lBuf[1024];
	int len;
	HANDLE hSearch;
	WIN32_FIND_DATAA fd;

	if ((hSearch = FindFirstFileA("C:\\Users\\Admin\\Documents\\3dsmax\\Lightmaps\\*.dds", &fd)) == INVALID_HANDLE_VALUE) return;
	StrCpy(lBuf, "C:\\Users\\Admin\\Documents\\3dsmax\\Lightmaps\\");
	len = StrLen(lBuf);

	do{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			StrCat(lBuf+len, fd.cFileName);
			//printf("%s\n", lBuf);
			ProcFile(fd.cFileName, lBuf);

			lBuf[len] = 0;
		}
	} while (FindNextFileA(hSearch, &fd));
	FindClose(hSearch);
}

int __cdecl main(int argc, char **argv) {
	SetDefaultMemoryFunctions();
	//Test(false);
#ifdef _USRDLL
	printf("DLL\n");
#else _USRDLL
	printf("NOT DLL\n");
#endif //_USRDLL
	
	//Test2();
	//StressTest();
	//STBTest();
	//RPC_Test();
	//RPCTest2();
	TestCompile();
	getch();
	return 0;
}
#endif //VFS_TEST
#else //_NO_TEST_

int __cdecl main(int argc, char **argv) {
	SetDefaultMemoryFunctions();
	getch();
	return 0;
}
#endif //!_NO_TEST_

void __RPC_FAR * __RPC_API midl_user_allocate(size_t nLen){
	return (g_pAllocator(nLen));
}

void __RPC_API midl_user_free(void __RPC_FAR * lpvPointer){
    if(NULL != lpvPointer)
		g_pFreeer(lpvPointer);
}


