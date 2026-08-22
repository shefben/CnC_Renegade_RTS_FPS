
#include "StdLib.h"
#include "BaseClasses.h"
#include "ChunkClasses.h"
#include "Archive.h"
#include "Memory.h"
#include "StdLib.h"

//extern "C" {
#define ZLIB_DLL
#define ZEXPORT	__cdecl
#include "zlib.h"
//}

#define ZLIB_CHUNK (16384)

//Inflate == decompress

bool CopyDecompress(ChunkLoadClass* pCL, file f, pfnExtractCallback pCallback){
	char in[ZLIB_CHUNK];
	char out[ZLIB_CHUNK];

	size_32 o, have;
	int ret;

	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	if (inflateInit(&strm) != Z_OK) return false;

	
	
	o = 0;

	do {
		o += (strm.avail_in = (pCL->Read(in, ZLIB_CHUNK)));

		if (strm.avail_in == 0)
			break;

		strm.next_in = (Bytef *)in;

		do {
			strm.avail_out = ZLIB_CHUNK;
			strm.next_out = (Bytef *)out;

			ret = inflate(&strm, Z_NO_FLUSH);

			switch (ret){
				case Z_NEED_DICT:
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					inflateEnd(&strm);
					return false;
			}

			have = ZLIB_CHUNK - strm.avail_out;

			if (fwrite_(out, have, f) != have){
				inflateEnd(&strm);
				return false;
			}

		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	deflateEnd(&strm);

	return (ret == Z_STREAM_END) ? true : false;

}

bool CopyDecompressToStream(ChunkLoadClass* pCL, IStream* pStream){
	char in[ZLIB_CHUNK];
	char out[ZLIB_CHUNK];

	size_32 o, have;
	int ret;
	ULONG w;

	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	if (inflateInit(&strm) != Z_OK) return false;

	
	
	o = 0;

	do {
		o += (strm.avail_in = (pCL->Read(in, ZLIB_CHUNK)));

		if (strm.avail_in == 0)
			break;

		strm.next_in = (Bytef *)in;

		do {
			strm.avail_out = ZLIB_CHUNK;
			strm.next_out = (Bytef *)out;

			ret = inflate(&strm, Z_NO_FLUSH);

			switch (ret){
				case Z_NEED_DICT:
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					inflateEnd(&strm);
					return false;
			}

			have = ZLIB_CHUNK - strm.avail_out;

			if (pStream->Write(out, have, &w) != S_OK || w != have){
				inflateEnd(&strm);
				return false;
			}

		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	deflateEnd(&strm);

	return (ret == Z_STREAM_END) ? true : false;

}

//Deflate == compress

bool CopyCompress(file f, ChunkSaveClass* pCS, pfnExtractCallback pCallback){
	char in[ZLIB_CHUNK];
	char out[ZLIB_CHUNK];

	size_32 o, fsize, have;
	int flush;

	int ret;

	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK) return false;

	
	
	fsize = fsize_(f);
	fsize = o = 0;

	do {
		o += (strm.avail_in = (fread_(in, ZLIB_CHUNK, f)));

		flush = (o == fsize_(f)) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = (Bytef *)in;

		do {
			strm.avail_out = ZLIB_CHUNK;
			strm.next_out = (Bytef *)out;

			ret = deflate(&strm, flush);

			have = ZLIB_CHUNK-strm.avail_out;
			fsize += have;
			if (pCS->Write(out, have) != have){
				deflateEnd(&strm);
				return false;
			}
		} while (strm.avail_out == 0);

	} while (flush != Z_FINISH);

	ret = deflateEnd(&strm);

	return true;

}

bool ExtractVFS(const wchar_t *pArchive, pfnStreamExtractCallback pCallback, void *pCookie){
	if (!pArchive || !pCallback) return false;
	
	ChunkLoadClass cl;
	file f;
	IStream* pStream;
	wchar_t* pBuf;
	char lBuf[1024];

	size_32 totalfiles, curfile;
	size_32 r, fsize, o;
	ULONG w;
	bool gotcount;

	if ((f = fopen_W(pArchive, L"r")) == INVALID_FILE) return false;
	if (!(pBuf = (wchar_t*)g_pAllocator(64*1024*sizeof(wchar_t)))){
		fclose_(f);
		return false;
	}

	cl.SetFile(f);

	gotcount = false;
	
	while (cl.OpenChunk()){
		if (cl.CurChunkID() != CHUNK_VFS_FILES){
			cl.CloseChunk();
			continue;
		}

		totalfiles = curfile = 0;

		//Callback supplied, so get file count
		if (pCallback && !gotcount){
			while (cl.OpenChunk()){
				if (cl.CurChunkID() == CHUNK_FILE)
					totalfiles++;
				cl.CloseChunk(); //CHUNK_FILE
			}
			//Notify count:
			gotcount = true;
			//Close chunk and seek to 0 to reset to start
			cl.CloseChunk();
			fseek_(f, 0, SEEK_SET);
			continue;
		}

		while (cl.OpenChunk()){
			if (cl.CurChunkID() == CHUNK_FILE){
				cl.OpenChunk();
				if (cl.CurChunkID() != CHUNK_FILEINFO){
__do_continue:
					cl.CloseChunk(); //CHUNK_FILEINFO
					cl.CloseChunk(); //CHUNK_FILE
					continue;
				}

				cl.OpenMicroChunk();
				if (cl.CurrentMicroChunkID() != MICRO_CHUNK_FILENAME){
					cl.CloseMicroChunk();
					goto __do_continue;
				}

				cl.Read(pBuf, cl.CurrentMicroChunkLength());
				cl.CloseMicroChunk(); //MICRO_CHUNK_FILENAME
				cl.CloseChunk(); //CHUNK_FILEINFO

				cl.OpenChunk();
				if (cl.CurChunkID() != CHUNK_DATA && cl.CurChunkID() != CHUNK_ZIP_DATA)
					goto __do_continue;

				if ((pStream = pCallback(pBuf, pCookie))){
					fsize = cl.CurChunkLength();

					if (cl.CurChunkID() == CHUNK_ZIP_DATA){
						//ZIP compressed
						CopyDecompressToStream(&cl, pStream);
					} else {
						o = 0;

						while ((r = cl.Read(lBuf, sizeof(lBuf)))){
							//fwrite_(lBuf, r, f2);
							pStream->Write(lBuf, r, &w);
						}
					}
					pStream->Release();
				}
				curfile++;

				cl.CloseChunk();



			}
			cl.CloseChunk(); //CHUNK_FILE
		}


		cl.CloseChunk();
	}
	fclose_(f);

	return true;
}

bool ExtractExtern(const wchar_t *pDestDir, const wchar_t *pArchive, pfnExtractCallback pCallback, void *pCookie){
	ChunkLoadClass cl;
	file f, f2;
	wchar_t* pBuf;
	char lBuf[1024];

	size_32 totalfiles, curfile;
	size_32 r, fsize, o;
	int l;
	bool gotcount;

	if ((f = fopen_W(pArchive, L"r")) == INVALID_FILE) return false;
	if (!(pBuf = (wchar_t*)g_pAllocator(64*1024*sizeof(wchar_t)))){
		fclose_(f);
		return false;
	}

	cl.SetFile(f);

	gotcount = false;
	l = StrLenW(pDestDir);
	
	while (cl.OpenChunk()){
		if (cl.CurChunkID() != CHUNK_EXTERN_FILES){
			cl.CloseChunk();
			continue;
		}

		totalfiles = curfile = 0;

		//Callback supplied, so get file count
		if (pCallback && !gotcount){
			while (cl.OpenChunk()){
				if (cl.CurChunkID() == CHUNK_FILE)
					totalfiles++;
				cl.CloseChunk(); //CHUNK_FILE
			}
			//Notify count:
			pCallback(NULL, 0, 0, 0, totalfiles, pCookie);
			gotcount = true;
			//Close chunk and seek to 0 to reset to start
			cl.CloseChunk();
			fseek_(f, 0, SEEK_SET);
			continue;
		}
		StrCpyW(pBuf, pDestDir);
		StrCatW(pBuf, L"\\");
		l++;

		while (cl.OpenChunk()){
			if (cl.CurChunkID() == CHUNK_FILE){
				cl.OpenChunk();
				if (cl.CurChunkID() != CHUNK_FILEINFO){
__do_continue:
					cl.CloseChunk(); //CHUNK_FILEINFO
					cl.CloseChunk(); //CHUNK_FILE
					continue;
				}

				cl.OpenMicroChunk();
				if (cl.CurrentMicroChunkID() != MICRO_CHUNK_FILENAME){
					cl.CloseMicroChunk();
					goto __do_continue;
				}

				cl.Read(pBuf+l, cl.CurrentMicroChunkLength());
				cl.CloseMicroChunk(); //MICRO_CHUNK_FILENAME
				cl.CloseChunk(); //CHUNK_FILEINFO

				cl.OpenChunk();
				if (cl.CurChunkID() != CHUNK_DATA && cl.CurChunkID() != CHUNK_ZIP_DATA)
					goto __do_continue;

				if (MakeDirFromFilenameW(pBuf, NULL, NULL) && (f2 = fopen_W(pBuf, L"w")) != INVALID_FILE){
					fsize = cl.CurChunkLength();
					if (pCallback)
						pCallback(pBuf, 0, fsize, curfile, totalfiles, pCookie);

					if (cl.CurChunkID() == CHUNK_ZIP_DATA){
						//ZIP compressed
						CopyDecompress(&cl, f2, pCallback);
					} else {
						o = 0;

						while ((r = cl.Read(lBuf, sizeof(lBuf)))){
							fwrite_(lBuf, r, f2);
							o += r;
							if (pCallback)
								pCallback(pBuf, o, fsize, curfile, totalfiles, pCookie);
						}
					}
					fclose_(f2);
				}
				curfile++;

				cl.CloseChunk();



			}
			cl.CloseChunk(); //CHUNK_FILE
		}


		cl.CloseChunk();
	}
	fclose_(f);

	return true;
}

void WriteFileToArchive(const wchar_t *pFileName, const wchar_t *pFullName, ChunkSaveClass *pCS, bool zip){
	file f;
	char lBuf[1024];
	size_32 r;

	pCS->BeginChunk(CHUNK_FILE);

	pCS->BeginChunk(CHUNK_FILEINFO);
	pCS->BeginMicroChunk(MICRO_CHUNK_FILENAME);
	pCS->Write(pFileName, ((StrLenW(pFileName)+1)*sizeof(wchar_t)));
	pCS->EndMicroChunk(); //MICRO_CHUNK_FILENAME
	pCS->EndChunk(); //CHUNK_FILEINFO

	if ((f = fopen_W(pFullName, L"r"))){
		if (zip){
			pCS->BeginChunk(CHUNK_ZIP_DATA);
			/*while ((r = fread_(lBuf, 1024, f))){
				pCS->Write(lBuf, r);
			}*/
			CopyCompress(f, pCS, NULL);
		} else {
			pCS->BeginChunk(CHUNK_DATA);
			while ((r = fread_(lBuf, 1024, f))){
				pCS->Write(lBuf, r);
			}
		}
			fclose_(f);
		pCS->EndChunk(); //CHUNK_DATA || CHUNK_ZIP_DATA
	}

	pCS->EndChunk(); //CHUNK_FILE
}

void ImportRecursive(wchar_t *pBuf, wchar_t *pStart, wchar_t *pCur, ChunkSaveClass *pCS, bool zip){
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	wchar_t *pStr, *pStr2;

	hFind = FindFirstFileW(pBuf, &fd);
	*(pCur--) = 0;
	pStr = pCur;
	pStr2 = pStart+StrLenW(pStart);


	if (hFind != INVALID_HANDLE_VALUE){
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				StrCpyW(pStr2-1, fd.cFileName);
				pCur = pStr2-1+StrLenW(fd.cFileName);
				*(pCur++) = '\\';
				*(pCur++) = '*';
				*pCur = 0;
				ImportRecursive(pBuf, pStart, pCur, pCS, zip);
				*pStr = 0;
			} else {
				StrCpyW(pStr2-1, fd.cFileName);
				pCur = pStr2-1+StrLenW(fd.cFileName);
				*pCur = 0;
				WriteFileToArchive(pStart, pBuf, pCS, zip);
				*pStr = 0;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
}

void DoVFS(ChunkSaveClass *pCS, const wchar_t *pDir, wchar_t *pBuf, bool zip){
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	wchar_t *pStart, *pCur;

	StrCpyW(pBuf, pDir);
	pStart = pBuf + StrLenW(pDir);
	*(pStart++) = '\\';
	*(pStart++) = '*';
	*pStart = 0;

	hFind = FindFirstFileW(pBuf, &fd);
	pStart--;


	if (hFind != INVALID_HANDLE_VALUE){
		pCS->BeginChunk(CHUNK_VFS_FILES);
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				StrCpyW(pStart, fd.cFileName);
				pCur = pStart+StrLenW(pStart);
				*(pCur++) = '\\';
				*(pCur++) = '*';
				*pCur = 0;
				ImportRecursive(pBuf, pStart, pCur, pCS, zip);
				*pStart = 0;
			} else {
				StrCpyW(pStart, fd.cFileName);
				pCur = pStart+StrLenW(fd.cFileName);
				*pCur = 0;
				WriteFileToArchive(pStart, pBuf, pCS, zip);
				*(pStart) = 0;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
		pCS->EndChunk(); //CHUNK_VFS_FILES
	}

}

void DirsRecursive(wchar_t *pBuf, wchar_t *pStart, wchar_t *pCur, ChunkSaveClass *pCS){
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	wchar_t *pStr, *pStr2;
	int l;

	hFind = FindFirstFileW(pBuf, &fd);
	*(pCur--) = 0;
	pStr = pCur;
	pStr2 = pStart+(l = StrLenW(pStart));
	*(pStr2-1) = 0;

	pCS->BeginMicroChunk(MICRO_CHUNK_DIRNAME);
	pCS->Write(pStart, l*sizeof(wchar_t));
	pCS->EndMicroChunk(); //MICRO_CHUNK_DIRNAME

	if (hFind != INVALID_HANDLE_VALUE){
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				StrCpyW(pStr2-1, fd.cFileName);
				pCur = pStr2-1+StrLenW(fd.cFileName);
				*(pCur++) = '\\';
				*(pCur++) = '*';
				*pCur = 0;
				DirsRecursive(pBuf, pStart, pCur, pCS);
				*pStr = 0;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
}

void DoExtern(ChunkSaveClass *pCS, const wchar_t *pDir, wchar_t *pBuf, bool zip){
	HANDLE hFind;
	WIN32_FIND_DATAW fd;
	wchar_t *pStart, *pCur;

	StrCpyW(pBuf, pDir);
	pStart = pBuf + StrLenW(pDir);
	*(pStart++) = '\\';
	*(pStart++) = '*';
	*pStart = 0;

	hFind = FindFirstFileW(pBuf, &fd);
	pStart--;


	pCS->BeginChunk(CHUNK_EXTERN_FILES);
	/*if (hFind != INVALID_HANDLE_VALUE){
		pCS->BeginChunk(CHUNK_DIRS);
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				StrCpyW(pStart, fd.cFileName);
				pCur = pStart+StrLenW(pStart);
				*(pCur++) = '\\';
				*(pCur++) = '*';
				*pCur = 0;
				DirsRecursive(pBuf, pStart, pCur, pCS);
				*pStart = 0;
			}
		} while (FindNextFileW(hFind, &fd));
		pCS->EndChunk(); //CHUNK_DIRS
		FindClose(hFind);
	}//*/
	*(pStart++) = '*';
	*pStart = 0;

	hFind = FindFirstFileW(pBuf, &fd);
	pStart--;

	if (hFind != INVALID_HANDLE_VALUE){
		do {
			if (memcmp(fd.cFileName, L".", 2*sizeof(wchar_t)) == 0 || memcmp(fd.cFileName, L"..", 3*sizeof(wchar_t)) == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				StrCpyW(pStart, fd.cFileName);
				pCur = pStart+StrLenW(pStart);
				*(pCur++) = '\\';
				*(pCur++) = '*';
				*pCur = 0;
				ImportRecursive(pBuf, pStart, pCur, pCS, zip);
				*pStart = 0;
			} else {
				StrCpyW(pStart, fd.cFileName);
				pCur = pStart+StrLenW(fd.cFileName);
				*pCur = 0;
				WriteFileToArchive(pStart, pBuf, pCS, zip);
				*(pStart) = 0;
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}


	pCS->EndChunk(); //CHUNK_EXTERN_FILES
}

bool GenerateArchive(const wchar_t *pOut, const wchar_t *pVFS, const wchar_t *pExtern, bool zip){
	file f;
	ChunkSaveClass cs;
	wchar_t *pBuf;
	bool ret;
	ret = false;
	if (!pOut || !(pVFS || pExtern)) return false;

	if (!(pBuf = (wchar_t*)g_pAllocator(0xFFFF))) return false;

	if ((f = fopen_W(pOut, L"w")) == INVALID_FILE) goto __cleanup_exit;


	cs.SetFile(f);
	if (pVFS)
		DoVFS(&cs, pVFS, pBuf, zip);

	
	if (pExtern)
		DoExtern(&cs, pExtern, pBuf, zip);

	fclose_(f);


	ret = true;
__cleanup_exit:
	g_pFreeer(pBuf);
	return ret;
}
