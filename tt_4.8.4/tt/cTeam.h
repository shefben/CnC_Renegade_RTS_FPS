#ifndef TT_INCLUDE__CTEAM_H
#define TT_INCLUDE__CTEAM_H



#include "NetworkObjectClass.h"
#include "engine_string.h"
#include "NetworkObjectClassId.h"



#define MAX_TEAMS 2



class cTeam :
	public NetworkObjectClass
{

private:

	WideStringClass name; // 06B4
	int kills; // 06B8
	int deaths; // 06BC
	float score; // 06C0
	int unk06C4; // 06C4; Not sure about type, but this variable never seems to be used.
	int id; // 06C8

public:

	cTeam();
	~cTeam();
	void Init(int _id);
	void Init_Team_Name();
	void Reset();
	void Set_Kills(int _kills);
	void Set_Deaths(int _deaths);
	void Set_Score(float _score);
	int Tally_Size();
	void Increment_Kills();
	void Increment_Deaths();
	void Increment_Score(float amount);
	float Get_Kill_To_Death_Ratio();
	void Get_Team_String(int, WideStringClass&);
	Vector3 Get_Color();
	int Tally_Money();
	void Export_Creation(BitStreamClass& bitStream);
	void Import_Creation(BitStreamClass& bitStream);
	void Export_Rare(BitStreamClass& bitStream);
	void Import_Rare(BitStreamClass& bitStream);
	void Export_Occasional(BitStreamClass& bitStream);
	void Import_Occasional(BitStreamClass& bitStream);

	unsigned int Get_Network_Class_ID() const { return NET_cTeam; }
	void Delete() { delete this; }
	WideStringClass Get_Name() const { return name; }
	int Get_Id() const { return id; }
	int Get_Kills() const { return kills; }
	int Get_Deaths() const { return deaths; }
	float Get_Score() const { return score; }

}; // 06CC



#endif