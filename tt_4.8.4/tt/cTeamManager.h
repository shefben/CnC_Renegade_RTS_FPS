#ifndef TT_INCLUDE__CTEAMMANAGER_H
#define TT_INCLUDE__CTEAMMANAGER_H



class cTeam;
class WideStringClass;
class Vector3;
template<typename T> class SList;


class Font3DInstanceClass;
class Render2DClass;
class cTeamManager
{

private:

	static REF_DECL2(Y_INCREMENT_FACTOR, float);
	static REF_DECL2(TeamListHeight, int);
	static REF_DECL1(PTextRenderer, Render2DClass*);
	static REF_DECL2(PFont, Font3DInstanceClass*);
	static REF_DECL2(XPos, int);
	static REF_DECL2(YPos, int);
	static REF_DECL2(TeamList, SList<cTeam>);
	static REF_ARR_DECL2(Team_Array,cTeam *,2);

public:

	static void Onetime_Init();
	static void Onetime_Shutdown();
	static void Think();
	static void Render();
	static cTeam *Find_Team(int);
	static cTeam *Find_Empty_Team();
	static void Add(cTeam*);
	static void Remove(cTeam*);
	static void Remove(int);
	static void Remove_All();
	static int Compute_Team_List_Height();
	static void Log_Team_List();
	static WideStringClass Get_Team_Name(int);
	static void Reset_Teams();
	static int Get_Non_Empty_Team_Count();
	static void Sort_Teams();
	static int Get_Leaders_Id();
	static int Sort_Score_Ktd_Kills(cTeam*, cTeam*);
	static int Team_Compare(const void*, const void*);
	static void List_Print(WideStringClass&, Vector3);
	static void Construct_Heading(WideStringClass&);
	static void Render_Team_List();

	static SList<cTeam>& Get_Team_Object_List() { return TeamList; }
};



#endif