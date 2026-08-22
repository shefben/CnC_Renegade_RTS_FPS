#include "general.h"
#include "ObjectiveManager.h"
#include "vector3.h"


REF_DEF2(ObjectiveManager::Viewer, ObjectivesViewerClass, 0x00856500, 0x008556E8);
REF_DEF1(ObjectiveManager::ObjectiveList, SimpleDynVecClass<Objective*>, 0x008564C0);
REF_DEF1(ObjectiveManager::HUDUpdate, bool, 0x0080F66C);

RENEGADE_FUNCTION
void ObjectiveManager::Update(float)
AT2(0x00675570, 0x00674E10);

RENEGADE_FUNCTION
void ObjectiveManager::Init()
AT1(0x00674380);

RENEGADE_FUNCTION
int ObjectiveManager::Get_Num_HUD_Objectives()
AT1(0x006753A0);

RENEGADE_FUNCTION
const char* ObjectiveManager::Get_HUD_Objectives_Pog_Texture_Name(int index)
AT1(0x006753E0);

RENEGADE_FUNCTION
const wchar_t* ObjectiveManager::Get_HUD_Objectives_Message(int index)
AT1(0x00675400);

RENEGADE_FUNCTION
Vector3 ObjectiveManager::Get_HUD_Objectives_Location(int index)
AT1(0x00675450);

RENEGADE_FUNCTION
float ObjectiveManager::Get_HUD_Objectives_Age(int index)
AT1(0x006754C0);
