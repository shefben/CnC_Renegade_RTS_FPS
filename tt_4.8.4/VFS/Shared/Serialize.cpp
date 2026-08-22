#include <windows.h>
#include "Base.h"
#include "BaseClasses.h"
#include "Memory.h"
#include "Lists.h"
#include "Serialize.h"


CSerialize::CSerialize(){
	m_pData = g_pAllocator(SERIALIZE_CACHE_SIZE);
	m_Offset = 0;
}

CSerialize::~CSerialize(){
	if (m_pData)
		g_pFreeer(m_pData);

	m_pData = NULL;
	m_Offset = 0;
}

size_32 CSerialize::GetSize(){
	return m_Offset;
}

void* CSerialize::GetBuffer(){
	return m_pData;
}

void CSerialize::Reset(){
	m_Offset = 0;
}

void CSerialize::WriteSize32(size_32 val){
	memcpy(((unsigned char*)m_pData)+m_Offset, &val, sizeof(size_32));
	m_Offset += sizeof(size_32);
}

void CSerialize::WriteSize64(size_64 val){
	memcpy(((unsigned char*)m_pData)+m_Offset, &val, sizeof(size_64));
	m_Offset += sizeof(size_64);
}

void CSerialize::WriteHandle(HANDLE val){
	size_64 s64;
	s64 = PtrToUlong((void*)val);
	WriteSize64(s64);
}


void CSerialize::WritePointer(ptr val){
	size_64 s64;
	s64 = PtrToUlong(val);
	WriteSize64(s64);
}

void CSerialize::WriteRaw(void *val, size_32 size){
	memcpy(((unsigned char*)m_pData)+m_Offset, val, size);
	m_Offset += size;
}

/*
CDeSerialize
*/

CDeSerialize::CDeSerialize(){
	m_pData = g_pAllocator(SERIALIZE_CACHE_SIZE);
	m_Offset = 0;
}

CDeSerialize::~CDeSerialize(){
	if (m_pData)
		g_pFreeer(m_pData);

	m_pData = NULL;
	m_Offset = 0;
}


void* CDeSerialize::GetBuffer(){
	return m_pData;
}

void CDeSerialize::Reset(){
	m_Offset = 0;
}

size_32 CDeSerialize::GetSize32(){
	return *(size_32*)(((unsigned char*)m_pData)+(m_Offset += sizeof(size_32)));
}

size_64 CDeSerialize::GetSize64(){
	return *(size_64*)(((unsigned char*)m_pData)+(m_Offset += sizeof(size_64)));
}

HANDLE CDeSerialize::GetHandle(){
	return NULL;
}

ptr CDeSerialize::GetPointer(){
	return NULL;
}

ptr CDeSerialize::GetRaw(size_32 size){
	return (((unsigned char*)m_pData)+(m_Offset += size));
}
