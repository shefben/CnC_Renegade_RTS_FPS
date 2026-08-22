#include "General.h"
#include "cTeamManager.h"
#include "render2d.h"
#include "MultiHUDClass.h"
#include "Vector3.h"
#include "cUserOptions.h"
#include "cTeam.h"

REF_DEF2(cTeamManager::TeamList, SList<cTeam>, 0x0081DD00, 0x0081CEE0);
REF_ARR_DEF2(cTeamManager::Team_Array,cTeam *,2,0x0081DCF8,0x0081CED8);

RENEGADE_FUNCTION
void cTeamManager::Onetime_Init()
AT2(0x0041D8C0, 0x0041D8C0);



RENEGADE_FUNCTION
void cTeamManager::Onetime_Shutdown()
AT2(0x0041D980, 0x0041D980);



void cTeamManager::Think()
{
	if (PTextRenderer)
	{
		if (MultiHUDClass::Is_On())
		{
			Render_Team_List();
		}
		else
		{
			PTextRenderer->Reset();
		}
	}
}


REF_DEF1(cTeamManager::PTextRenderer, Render2DClass *, 0x0081DDB0);
extern bool RenderHud;
extern bool HidePlayerList;
void cTeamManager::Render()
{
	if (RenderHud && PTextRenderer && !HidePlayerList)
	{
		PTextRenderer->Render();
	}
}



RENEGADE_FUNCTION
cTeam* cTeamManager::Find_Team(int)
AT2(0x0041DA10, 0x0041DA10);



RENEGADE_FUNCTION
cTeam* cTeamManager::Find_Empty_Team()
AT2(0x0041DA40, 0x0041DA40);



RENEGADE_FUNCTION
void cTeamManager::Add(cTeam*)
AT2(0x0041DA70, 0x0041DA70);



RENEGADE_FUNCTION
void cTeamManager::Remove(cTeam*)
AT2(0x0041DB60, 0x0041DB60);



RENEGADE_FUNCTION
void cTeamManager::Remove(int)
AT2(0x0041DC10, 0x0041DC10);



RENEGADE_FUNCTION
void cTeamManager::Remove_All()
AT2(0x0041DCE0, 0x0041DCE0);



RENEGADE_FUNCTION
int cTeamManager::Compute_Team_List_Height()
AT2(0x0041DDA0, 0x0041DDA0);



void cTeamManager::Log_Team_List()
{
	StringClass filename;
	filename.Format("results%d.txt",cUserOptions::ResultsLogNumber.Data);
	FILE *f = fopen(filename,"at");
	if (f)
	{
		char temp[2000];
		memset(temp,0,sizeof(temp));
		WideStringClass str(0,true);
		cTeamManager::Construct_Heading(str);
		StringClass str2;
		str2.Copy_Wide(str);
		sprintf(temp,"%s\n",(const char *)str2);
		fwrite(temp,1,strlen(temp),f);
		memset(temp,0,sizeof(temp));
		for (int i = 0;i < 2;i++)
		{
			if (Team_Array[i])
			{
				Team_Array[i]->Get_Team_String(i+1,str);
				str2.Copy_Wide(str);
				sprintf(temp,"%s\n",(const char *)str2);
				fwrite(temp,1,strlen(temp),f);
				memset(temp,0,sizeof(temp));
			}
		}
		sprintf(temp, "\n");
		fwrite(temp,1,strlen(temp),f);
		fclose(f);
	}
}

RENEGADE_FUNCTION
WideStringClass cTeamManager::Get_Team_Name(int)
AT2(0x0041DF90, 0x0041DF90);



RENEGADE_FUNCTION
void cTeamManager::Reset_Teams()
AT2(0x0041E0C0, 0x0041E0C0);



RENEGADE_FUNCTION
int cTeamManager::Get_Non_Empty_Team_Count()
AT2(0x0041E0E0, 0x0041E0E0);



RENEGADE_FUNCTION
void cTeamManager::Sort_Teams()
AT2(0x0041E110, 0x0041E110);



RENEGADE_FUNCTION
int cTeamManager::Get_Leaders_Id()
AT2(0x0041E160, 0x0041E160);



RENEGADE_FUNCTION
int cTeamManager::Sort_Score_Ktd_Kills(cTeam*, cTeam*)
AT2(0x0041E170, 0x0041E170);



RENEGADE_FUNCTION
int cTeamManager::Team_Compare(const void*, const void*)
AT2(0x0041E1D0, 0x0041E1D0);



RENEGADE_FUNCTION
void cTeamManager::List_Print(WideStringClass&, Vector3)
AT2(0x0041E230, 0x0041E230);



RENEGADE_FUNCTION
void cTeamManager::Construct_Heading(WideStringClass&)
AT2(0x0041E320, 0x0041E320);



RENEGADE_FUNCTION
void cTeamManager::Render_Team_List()
AT2(0x0041E6D0, 0x0041E6D0);
