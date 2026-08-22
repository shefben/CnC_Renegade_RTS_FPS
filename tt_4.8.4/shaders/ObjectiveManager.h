#ifndef TT_INCLUDE__OBJECTIVEMANAGER_H
#define TT_INCLUDE__OBJECTIVEMANAGER_H
#include "engine_string.h"


class Objective;
class ChunkLoadClass;
class ChunkSaveClass;
class Vector3;
class PhysicalGameObj;
class ObjectivesViewerClass;
template<typename T> class SimpleDynVecClass;



class ObjectiveManager
{

public:

	static REF_DECL2(Viewer, ObjectivesViewerClass);
	static REF_DECL1(HUDUpdate, bool);
	static REF_DECL1(ObjectiveList, SimpleDynVecClass<Objective*>);

	static void Update(float);
	static bool Load(ChunkLoadClass&);
	static Objective* Add_Loadable_Objective();
	static Objective* Find_Objective(int);
	static void Init();
	static void Shutdown();
	static void Reset();
	static bool Save(ChunkSaveClass&);
	static void Add_Objective(int, int, int, int, int, char*);
	static void Remove_Objective(int);
	static void Set_Objective_Status(int, int);
	static void Change_Objective_Type(int, int);
	static void Set_Objective_Radar_Blip(int, Vector3);
	static void Set_Objective_Radar_Blip(int, PhysicalGameObj*);
	static int ObjectiveSortCallback(void const*, void const*);
	static void Sort_Objectives();
	static void Set_Objective_HUD_Info(int, float, char const*, int);
	static void Set_Objective_HUD_Info(int, float, char const*, int, const Vector3&);
	static int Get_Num_HUD_Objectives();
	static const char* Get_HUD_Objectives_Pog_Texture_Name(int);
	static const wchar_t* Get_HUD_Objectives_Message(int);
	static Vector3 Get_HUD_Objectives_Location(int);
	static float Get_HUD_Objectives_Age(int);
	static int Get_Num_Objectives(int);
	static int Get_Num_Completed_Objectives(int);


};



#endif