#include "SaveLoadSystem.h"

void PostLoadableClass::On_Post_Load(){
}

PersistClass::~PersistClass(){
}

void *PersistClass::Get_Factory(){
	return NULL;
}
bool PersistClass::Save(ChunkSaveClass &save){
	return true;
}
bool PersistClass::Load(ChunkLoadClass &load){
	return true;
}