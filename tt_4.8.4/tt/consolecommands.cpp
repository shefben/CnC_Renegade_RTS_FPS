/*	Renegade tt.dll
	Console commands implementation
	Copyright 2009 Jonathan Wilson

	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"

#include "consolecommands.h"
#include "tt.h"
#include "SoldierGameObj.h"
#include "cPlayerManager.h"
#include "cRemoteHost.h"
#include "cNetwork.h"
#include "CombatManager.h"
#include "cconnection.h"
#include "cGameType.h"
#include "viewerdlg.h"
#include "OffenseObjectClass.h"
#include "Engine_Game.h"
#include "GameObjManager.h"
#include "Engine_Weap.h"
#include "Engine_Obj.h"
#include "Engine_Obj2.h"
#include "Engine_dmg.h"
#include "GameInitMgrClass.h"
#define MSG_MAXLEN 512
extern int MineLimit;
extern sockaddr_in rlmonaddr;
extern "C" int Get_Bandwidth(int PlayerID);
extern "C" int Get_Ping(int PlayerID);
extern "C" const char *Get_IP_Port(int PlayerID);
extern "C" unsigned long Get_Kbits(int PlayerID);
REF_DEF2(ConsoleFunctionList,DynamicVectorClass<ConsoleFunctionClass *>,0x0081DEB8,0x0081D098);
void AddToConsoleFunctionList(ConsoleFunctionClass *cmd)
{
	ConsoleFunctionList.Add(cmd);
}


void IntializeShaderDevConsoleCommands();



#ifdef DEBUG
#include "ObjectLibraryManager.h"
#include "BuildingGameObj.h"



class CommandDebug :
	public ConsoleFunctionClass
{

public:

	char* Get_Name() { return "debug"; }
	char* Get_Help() { return ""; }
	void Activate(char* commandLine)
	{
		PersistClass* persistObject = ObjectLibraryManager::Create_Object("CnC_GDI_Orca");
		if (persistObject)
		{
			Vector3 position;
			if (CombatManager::Get_The_Star())
			{
				CombatManager::Get_The_Star()->Get_Position(&position);
				position.Z += 2.f;
			}
			else
				position = Vector3(0, 0, 10);
			
			((PhysicalGameObj*)persistObject)->Set_Position(position);
			((DamageableGameObj*)persistObject)->Get_Defense_Object()->Set_Health_Max(2000);
			((DamageableGameObj*)persistObject)->Get_Defense_Object()->Set_Shield_Strength_Max(2000);
			((DamageableGameObj*)persistObject)->Get_Defense_Object()->Set_Health(2000);
			((DamageableGameObj*)persistObject)->Get_Defense_Object()->Set_Shield_Strength(2000);
		}
	}

};



class CommandTest :
	public ConsoleFunctionClass
{

public:

	char* Get_Name() { return "test"; }
	char* Get_Help() { return ""; }
	void Activate(char* commandLine)
	{
		OffenseObjectClass offense(99999.f, 0, NULL);
		if (Find_Harvester(0)) Commands->Apply_Damage(Find_Harvester(0), 99999.f, "None", NULL);
		if (Find_Harvester(1)) Commands->Apply_Damage(Find_Harvester(1), 99999.f, "None", NULL);
		if (BaseControllerClass::Find_Base(0)->Find_Building(3)) BaseControllerClass::Find_Base(0)->Find_Building(3)->As_BuildingGameObj()->Apply_Damage(offense, 1.f, 0);
		if (BaseControllerClass::Find_Base(1)->Find_Building(3)) BaseControllerClass::Find_Base(1)->Find_Building(3)->As_BuildingGameObj()->Apply_Damage(offense, 1.f, 0);
	}

};



static void initDebugCommands()
{
	AddToConsoleFunctionList(new CommandDebug);
	AddToConsoleFunctionList(new CommandTest);
}
#endif


class ConsoleFunctionManager
{
public:
	static void Sort_Function_List();
	static void Verbose_Help_File();
};

RENEGADE_FUNCTION
void ConsoleFunctionManager::Sort_Function_List()
AT2(0x004282F0,0x00428470);
RENEGADE_FUNCTION
void ConsoleFunctionManager::Verbose_Help_File()
AT2(0x004284F0,0x00428670);
void InitConsole(void)
{
	ConsoleFunctionList.Delete(8);
	AddToConsoleFunctionList(new CommandID);
	AddToConsoleFunctionList(new CommandPAMSG);
	AddToConsoleFunctionList(new CommandSNDP);
	AddToConsoleFunctionList(new CommandSNDT);
	AddToConsoleFunctionList(new CommandSNDA);
	AddToConsoleFunctionList(new CommandMUSICP);
	AddToConsoleFunctionList(new CommandMUSICA);
	AddToConsoleFunctionList(new CommandNOMUSICP);
	AddToConsoleFunctionList(new CommandNOMUSICA);
	AddToConsoleFunctionList(new CommandSND3DP);
	AddToConsoleFunctionList(new CommandSND3DA);
	AddToConsoleFunctionList(new CommandPPAGE);
	AddToConsoleFunctionList(new CommandTPAGE);
	AddToConsoleFunctionList(new CommandMSG);
	AddToConsoleFunctionList(new CommandTEAM);
	AddToConsoleFunctionList(new CommandTEAM2);
	AddToConsoleFunctionList(new CommandDONATE);
	AddToConsoleFunctionList(new CommandMLIMIT);
	AddToConsoleFunctionList(new CommandVERSION);
	AddToConsoleFunctionList(new CommandSVERSION);
	AddToConsoleFunctionList(new CommandRADAR);
	AddToConsoleFunctionList(new CommandMLIST);
	AddToConsoleFunctionList(new CommandMLISTC);
	AddToConsoleFunctionList(new CommandMAP);
	AddToConsoleFunctionList(new CommandMOD);
	AddToConsoleFunctionList(new CommandMAPNUM);
	AddToConsoleFunctionList(new CommandMLIMITD);
	AddToConsoleFunctionList(new CommandMINED);
	AddToConsoleFunctionList(new CommandEJECT);
	AddToConsoleFunctionList(new CommandSND3DT);
	AddToConsoleFunctionList(new CommandICON);
	AddToConsoleFunctionList(new CommandICON2);
	AddToConsoleFunctionList(new CommandSONG);
	AddToConsoleFunctionList(new CommandWIN);
	AddToConsoleFunctionList(new CommandTMSG);
	AddToConsoleFunctionList(new CommandGETBW);
	AddToConsoleFunctionList(new CommandSETBW);
	AddToConsoleFunctionList(new CommandPINFO);
	AddToConsoleFunctionList(new CommandVLIMIT);
	AddToConsoleFunctionList(new CommandVLIMITD);
	AddToConsoleFunctionList(new CommandCMSG);
	AddToConsoleFunctionList(new CommandCMSGP);
	AddToConsoleFunctionList(new CommandCMSGT);
	AddToConsoleFunctionList(new CommandDISARM);
	AddToConsoleFunctionList(new CommandDISARMP);
	AddToConsoleFunctionList(new CommandDISARMB);
	AddToConsoleFunctionList(new CommandRLMON);
	AddToConsoleFunctionList(new CommandRLMONOFF);
	AddToConsoleFunctionList(new CommandPLIMITD);
	AddToConsoleFunctionList(new CommandPLIMIT);
	AddToConsoleFunctionList(new CommandTIME);
	AddToConsoleFunctionList(new CommandTIMED);
	AddToConsoleFunctionList(new CommandTIMEL);
	AddToConsoleFunctionList(new CommandTIMELD);
	AddToConsoleFunctionList(new CommandMAPCH);
	AddToConsoleFunctionList(new CommandSSURL);
	AddToConsoleFunctionList(new CommandSSHOT);
	AddToConsoleFunctionList(new CommandTAG);
	AddToConsoleFunctionList(new CommandSERIAL);
	AddToConsoleFunctionList(new CommandKICK2);
	if (!Exe)
	{
		AddToConsoleFunctionList(new CommandLOG);
		AddToConsoleFunctionList(new CommandLOGP);
		AddToConsoleFunctionList(new CommandEXIT);
		AddToConsoleFunctionList(new CommandVIEW);
		AddToConsoleFunctionList(new CommandHUD);
		IntializeShaderDevConsoleCommands();
	}
#ifdef DEBUG
	initDebugCommands();
#endif
	ConsoleFunctionManager::Sort_Function_List();
	ConsoleFunctionManager::Verbose_Help_File();
	if (Exe)
	{
		FILE *f = fopen("rlmon.cfg","rt");
		if (f)
		{
			unsigned long a;
			unsigned long b;
			unsigned long c;
			unsigned long d;
			unsigned short port;
			unsigned long IP;
#pragma warning(suppress: 6031)
			fscanf(f,"%d.%d.%d.%d:%d",&a,&b,&c,&d,&port);
			if ((a < 255) && (b < 255) && (c < 255) && (d < 255))
			{
				IP = a + (b << 8) + (c << 16) + (d << 24);
				rlmonaddr.sin_addr.s_addr = IP;
				rlmonaddr.sin_port = htons(port);
				rlmonaddr.sin_family = AF_INET;
			}
			fclose(f);
		}
	}
}

void CommandID::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	const char *m_pName;
	if (strlen(m_pArgs) > MSG_MAXLEN)
	{
		return;
	}
	for (SLNode<cPlayer>* PlayerIter = PlayerList.Head(); (PlayerIter != NULL); PlayerIter = PlayerIter->Next())
	{
		if (!(PlayerIter->Data())->IsActive)
		{
			continue;
		}
		m_pName = Get_Player_Name_By_Data(PlayerIter->Data());
		if (m_pArgs[0] && !m_pName)
		{
			continue;
		}
		if (m_pArgs[0] && !stristr(m_pName,m_pArgs))
		{
			delete[] m_pName;
			continue;
		}
		Console_Output("   %u: %s\n",Get_Player_ID_By_Data(PlayerIter->Data()),m_pName);
		if (m_pName)
		{
			delete[] m_pName;
		}
	}
}

void CommandPAMSG::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	SendTextSc(m_pMsg+1,PrivateMessage,true,-1,m_Id);
}

void CommandSNDP::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id) || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DWAVSOUND,m_pMsg);
	SendTextSc(buf,PrivateMessage,true,-2,m_Id);
	if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
	{
		Create_2D_WAV_Sound(m_pMsg);
	}
}

void CommandSNDT::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	int team = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject *o = x->Data();
		if (o)
		{
			if ((Get_Object_Type(o) == team) || (team == 2))
			{
				long m_Id = Get_Player_ID(o);
				if ((m_Id) && (m_Id != -1))
				{
					char buf[512];
					sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DWAVSOUND,m_pMsg);
					SendTextSc(buf,PrivateMessage,false,-2,m_Id);
					if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
					{
						Create_2D_WAV_Sound(m_pMsg);
					}
				}
			}
		}
		x = x->Next();
	}
}

void CommandSNDA::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_CREATE2DWAVSOUND,m_pArgs);
	SendTextSc(buf,PublicMessage,true,-2,-1);
	if (!Exe)
	{
		Create_2D_WAV_Sound(m_pArgs);
	}
}

void CommandMUSICP::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id) || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_SETBGM,m_pMsg);
	SendTextSc(buf,PrivateMessage,true,-2,m_Id);
	if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
	{
		Set_Background_Music(m_pMsg);
	}
}

void CommandMUSICA::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	if (CurrentSong)
	{
		delete[] CurrentSong;
	}
	CurrentSong = newstr(m_pArgs);
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n",SS_SETBGM,m_pArgs);
	SendTextSc(buf,PublicMessage,true,-2,-1);
	if (!Exe)
	{
		Set_Background_Music(m_pArgs);
	}
}

void CommandNOMUSICP::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (!m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_STOPBGM,0);
	SendTextSc(buf,PrivateMessage,true,-2,m_Id);
	if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
	{
		Stop_Background_Music();
	}
}

void CommandNOMUSICA::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	CurrentSong = 0;
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_STOPBGM,0);
	SendTextSc(buf,PublicMessage,true,-2,-1);
	if (!Exe)
	{
		Stop_Background_Music();
	}
}

void CommandSND3DP::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id) || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DWAVSOUND,m_pMsg,Commands->Get_ID(Get_GameObj(m_Id)),"ROOTTRANSFORM");
	SendTextSc(buf,PrivateMessage,true,-2,m_Id);
	if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
	{
		Create_3D_WAV_Sound_At_Bone(m_pMsg,Get_GameObj(m_Id),"ROOTTRANSFORM");
	}
}

void CommandSND3DA::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id) || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	char buf[512];
	sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DWAVSOUND,m_pMsg,Commands->Get_ID(Get_GameObj(m_Id)),"ROOTTRANSFORM");
	SendTextSc(buf,PublicMessage,0,-2,-1);
	Create_3D_WAV_Sound_At_Bone(m_pMsg,Get_GameObj(m_Id),"ROOTTRANSFORM");
}

void CommandPPAGE::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	bool send = true;
	if (Host_Hook)
	{
		send = Host_Hook(m_Id,PrivateMessage,m_pMsg);
	}
	if (send)
	{
		SendTextSc(m_pMsg+1,PrivateMessage,false,-1,m_Id);
	}
}

void CommandTPAGE::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long team = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg)
	{
		return;
	}
	bool send = true;
	if (Host_Hook)
	{
		send = Host_Hook(team,TeamMessage,m_pMsg);
	}
	if (send)
	{
		SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
		while (x)
		{
			GameObject *o = x->Data();
			if (o)
			{
				if ((Get_Object_Type(o) == team) || (team == 2))
				{
					long m_Id = Get_Player_ID(o);
					if ((m_Id) && (m_Id != -1))
					{
						SendTextSc(m_pMsg+1,PrivateMessage,false,-1,m_Id);
					}
				}
			}
			x = x->Next();
		}
	}
}

void CommandMSG::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	bool send = true;
	if (Host_Hook)
	{
		send = Host_Hook(-1,PublicMessage,m_pArgs);
	}
	if (send)
	{
		SendTextSc(m_pArgs,PublicMessage,false,-1,-1);
	}
}

void CommandTEAM::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	long m_TeamId = 0;
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	m_TeamId = atoi(m_pMsg+1);
	m_TeamId = (m_TeamId > 2 ? 2 : m_TeamId);
	GameObject *m_pGameObject = Get_GameObj(m_Id);
	if (m_TeamId == Commands->Get_Player_Type(m_pGameObject))
	{
		return;
	}
	Commands->Give_Points(m_pGameObject,-Commands->Get_Points(m_pGameObject),false);
	Commands->Give_Money(m_pGameObject,-Commands->Get_Money(m_pGameObject),false);
	Disarm_All_C4(m_Id);
	Change_Team_By_ID(m_Id,m_TeamId);
	Commands->Destroy_Object(Get_GameObj(m_Id));
}

void CommandTEAM2::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	long m_TeamId = 0;
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	m_TeamId = atoi(m_pMsg+1);
	m_TeamId = (m_TeamId > 2 ? 2 : m_TeamId);
	GameObject *m_pGameObject = Get_GameObj(m_Id);
	if (m_TeamId == Commands->Get_Player_Type(m_pGameObject))
	{
		return;
	}
	Disarm_All_C4(m_Id);
	Change_Team_By_ID(m_Id,m_TeamId);
	Commands->Destroy_Object(Get_GameObj(m_Id));
}

void CommandDONATE::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	m_pMsg++;
	long m_Id2 = atoi(m_pMsg);
	char *m_pMsg2 = strchr(m_pMsg,' ');
	if (!m_pMsg2 || !m_Id2 || !FindPlayer(m_Id2))
	{
		return;
	}
	m_pMsg2++;
	float m_Amount = (float)atof(m_pMsg2);
	if (m_Amount < 0)
	{
		return;
	}
	GameObject *m_pGameObject = Get_GameObj(m_Id);
	GameObject *m_pGameObject2 = Get_GameObj(m_Id2);
	if ((!m_pGameObject) || (!m_pGameObject2))
	{
		return;
	}
	float m_Cash = Commands->Get_Money(m_pGameObject);
	if (m_Cash < m_Amount || Commands->Get_Player_Type(m_pGameObject) != Commands->Get_Player_Type(m_pGameObject2))
	{
		return;
	}
	Commands->Give_Money(m_pGameObject,-m_Amount,false);
	Commands->Give_Money(m_pGameObject2,m_Amount,false);
}

void CommandEXIT::Activate(char *m_pArgs)
{
	if (!Exe)
	{
		GameInitMgrClass::NeedsGameExitAll = true;
	}
}

void CommandMLIMIT::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long limit = atoi(m_pArgs);
	MineLimit = limit;
}

void CommandVERSION::Activate(char *m_pArgs)
{
	if (!CombatManager::I_Am_Server() || cGameType::GameType == 1)
		return;
	
	int playerId = atoi(m_pArgs);
	
	cRemoteHost* remoteHost = cNetwork::Get_Server_Rhost(playerId);
	if (!remoteHost)
		return;
	
	Console_Output("The version of player %d is %f\n", playerId, remoteHost->getVersion());
}

void CommandSVERSION::Activate(char *m_pArgs)
{
	Console_Output("The version of tt.dll on this machine is %f\n", TT_VERSION);
}

void CommandRADAR::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	Console_Output("The Current Radar Mode is %d\n",The_Game()->RadarMode);
}

void CommandMLIST::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (m_Id > 99)
	{
		return;
	}
	if (*(The_Game()->MapList[m_Id]))
	{
		Console_Output("The map in position %d of the map list is %s\n",m_Id,The_Game()->MapList[m_Id]);
	}
	else
	{
		Console_Output("There is no map in position %d of the map list\n",m_Id);
	}
}

void CommandMLISTC::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || m_Id > 99 || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	char *map = new char[(strlen(m_pMsg)+1)];
	strcpy(map,m_pMsg);
	The_Game()->MapList[m_Id] = map;
}

void CommandMAP::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	Console_Output("The Current Map is %s\n",The_Game()->MapName);
}

void CommandMOD::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	if (*(The_Game()->ModName))
	{
		Console_Output("The Current Mod Package is %s\n",The_Game()->ModName);
	}
	else
	{
		Console_Output("There is no mod loaded\n");
	}
}

void CommandMAPNUM::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	Console_Output("The Current Map Number is %d\n",The_Game()->MapNumber);
}

void CommandMLIMITD::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	Console_Output("The Current Mine Limit is %d\n",MineLimit);
}

void CommandMINED::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	int team = atoi(m_pArgs);
	Console_Output("The Current Mine count is %d\n",Get_C4_Count(team));
}

void CommandEJECT::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (!m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	if (Get_Vehicle(Get_GameObj(m_Id)))
	{
		Check_Transitions(Get_GameObj(m_Id),true);
	}
}

void CommandSND3DT::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id2 = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	long m_TeamId = 0;
	if (!m_pMsg || !m_Id2 || !FindPlayer(m_Id2))
	{
		return;
	}
	m_pMsg++;
	m_TeamId = atoi(m_pMsg);
	m_TeamId = (m_TeamId > 2 ? 2 : m_TeamId);
	char *m_pMsg2 = strchr(m_pMsg,' ');
	m_pMsg2++;
	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject *o = x->Data();
		if (o)
		{
			if ((Get_Object_Type(o) == m_TeamId) || (m_TeamId == 2))
			{
				long m_Id = Get_Player_ID(o);
				if ((m_Id) && (m_Id != -1))
				{
					char buf[512];
					sprintf(buf,"j\n%d\n%s\n%d\n%s\n",SS_CREATE3DWAVSOUND,m_pMsg2,Commands->Get_ID(Get_GameObj(m_Id2)),"ROOTTRANSFORM");
					SendTextSc(buf,PrivateMessage,false,-2,m_Id);
					if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
					{
						Create_3D_WAV_Sound_At_Bone(m_pMsg2,Get_GameObj(m_Id2),"ROOTTRANSFORM");
					}
				}
			}
		}
		x = x->Next();
	}
}

void CommandICON::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_pId = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_pId || !FindPlayer(m_pId) || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	int team = Commands->Get_Player_Type(Get_GameObj(m_pId));
	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject *o = x->Data();
		if (o)
		{
			if ((Get_Object_Type(o) == team) || (team == 2))
			{
				long m_Id = Get_Player_ID(o);
				if ((m_Id) && (m_Id != -1))
				{
					char buf[512];
					sprintf(buf,"j\n%d\n%d\n%s\n",SS_ICONS,m_pId,m_pMsg);
					SendTextSc(buf,PrivateMessage,false,-2,m_Id);
					if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
					{
						((SoldierGameObj *)As_SoldierGameObj(Get_GameObj(m_pId)))->Set_Emot_Icon(m_pMsg,2.0);
					}
				}
			}
		}
		x = x->Next();
	}
}

void CommandICON2::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_pId = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_pId || !FindPlayer(m_pId) || (strlen(m_pMsg) > MSG_MAXLEN))
	{
		return;
	}
	m_pMsg++;
	int team = Commands->Get_Player_Type(Get_GameObj(m_pId));
	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject *o = x->Data();
		if (o)
		{
			if ((Get_Object_Type(o) != team))
			{
				long m_Id = Get_Player_ID(o);
				if ((m_Id) && (m_Id != -1))
				{
					char buf[512];
					sprintf(buf,"j\n%d\n%d\n%s\n",SS_ICONS,m_pId,m_pMsg);
					SendTextSc(buf,PrivateMessage,false,-2,m_Id);
					if ((!Exe) && (cNetwork::PClientConnection->Get_Local_Id() == m_Id))
					{
						((SoldierGameObj *)As_SoldierGameObj(Get_GameObj(m_pId)))->Set_Emot_Icon(m_pMsg,2.0);
					}
				}
			}
		}
		x = x->Next();
	}
}

void CommandSONG::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	if (CurrentSong)
	{
		Console_Output("The current song is %s\n",CurrentSong);
	}
	else
	{
		Console_Output("There is no song playing\n");
	}
}

void CommandWIN::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long team = atoi(m_pArgs);
	if (!team)
	{
		Kill_All_Buildings_By_Team(1);
	}
	else
	{
		Kill_All_Buildings_By_Team(0);
	}
}

void CommandLOG::Activate(char *m_pArgs)
{
	ClientChatLog = atoi(m_pArgs);
	if (ClientChatLog == false)
	{
		Console_Output("Client Chat Log is disabled\n");
	}
	else
	{
		Console_Output("Client Chat Log is enabled\n");
	}
	HKEY key;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,(char *)0x007F5274,0,KEY_ALL_ACCESS,&key);
	unsigned long l = ClientChatLog;
	RegSetValueEx(key,"ClientChatLog",0,REG_DWORD,(BYTE *)&l,4);
	RegCloseKey(key);
}

void CommandLOGP::Activate(char *m_pArgs)
{
	if (ClientChatLog == false)
	{
		Console_Output("Client Chat Log is disabled\n");
	}
	else
	{
		Console_Output("Client Chat Log is enabled\n");
	}
}

void CommandVIEW::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	char *m_pMsg = newstr(m_pArgs);
	if (!m_pMsg)
	{
		return;
	}
	m_pMsg[strlen(m_pMsg)] = 0;
	char *m_pMsg2 = strchr(m_pMsg,' ');
	if (m_pMsg2)
	{
		m_pMsg2[0] = 0;
		m_pMsg2++;
	}
	ModelViewerDlg::Do_Dialog(m_pMsg2,m_pMsg);
	delete[] m_pMsg;
}

extern bool RenderHud;
void CommandHUD::Activate(char *m_pArgs)
{
	RenderHud = RenderHud ? false: true;
	Commands->Enable_HUD(RenderHud);
}

void CommandTMSG::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	bool send = true;
	if (Host_Hook)
	{
		send = Host_Hook(m_Id,TMSGCommand,m_pMsg);
	}
	if (send)
	{
		SendTextSc(m_pMsg+1,PrivateMessage,false,m_Id,-1);
	}
}
void CommandGETBW::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (!m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	const char *name = Get_Player_Name_By_ID(m_Id);
	if (name)
	{
		Console_Output("Current Bandwidth for player %s is %d\n",name,Get_Bandwidth(m_Id));
		delete[] name;
	}
}

void CommandSETBW::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	long m_BW = 0;
	m_BW = atoi(m_pMsg+1);
	Set_Bandwidth(m_Id,m_BW);
}

void CommandPINFO::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	const char *m_pName;
	Console_Output("Start PInfo output\n");
	for (SLNode<cPlayer>* PlayerIter = PlayerList.Head(); (PlayerIter != NULL); PlayerIter = PlayerIter->Next())
	{
		if (!(PlayerIter->Data())->IsActive)
		{
			continue;
		}
		unsigned int i = Get_Player_ID_By_Data(PlayerIter->Data());
		m_pName = Get_Player_Name_By_Data(PlayerIter->Data());
		if (!m_pName)
		{
			continue;
		}
		long m_Score = (long)Get_Score(i);
		long m_Team = Get_Team(i);
		unsigned long m_Ping = Get_Ping(i);
		const char *m_IP = Get_IP_Port(i);
		unsigned long m_KB = Get_Kbits(i);
		long m_Rank = Get_Rank(i);
		long m_Kills = Get_Kills(i);
		long m_Deaths = Get_Deaths(i);
		long m_Money = (long)Get_Money(i);
		float m_KD = Get_Kill_To_Death_Ratio(i);
		Console_Output("%d,%s,%d,%d,%d,%s,%d,%d,%d,%d,%d,%f\n",i,m_pName,m_Score,m_Team,m_Ping,m_IP,m_KB,m_Rank,m_Kills,m_Deaths,m_Money,m_KD);
		delete[] m_pName;
		delete[] m_IP;
	}
	Console_Output("End PInfo output\n");
}

REF_DEF2(VehicleCount,int,0x00813620,0x008127F8);
void CommandVLIMIT::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Limit = atoi(m_pArgs);
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n",SS_VEHICLELIMIT,m_Limit);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	VehicleCount = m_Limit;
}

void CommandVLIMITD::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	Console_Output("Current Vehicle Limit is %d\n",VehicleCount);
}

void CommandCMSG::Activate(char *m_pArgs)
{
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	char *m_pMsg = newstr(m_pArgs);
	if (!m_pMsg)
	{
		return;
	}
	char *m_pMsg2 = strchr(m_pMsg,' ');
	m_pMsg2[0] = 0;
	m_pMsg2++;
	if (!m_pMsg2)
	{
		return;
	}
	if (!_stricmp(m_pMsg,"private"))
	{
		Get_Private_Message_Color(&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"public"))
	{
		Get_Public_Message_Color(&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"gdi"))
	{
		Get_Team_Color(1,&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"nod"))
	{
		Get_Team_Color(0,&red,&green,&blue);
	}
	else
	{
		if (!sscanf(m_pMsg,"%d,%d,%d",&red,&green,&blue))
		{
			red = 0;
			green = 0;
			blue = 0;
		}
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%s\n",SS_ADDMESSAGE,red,green,blue,m_pMsg2);
	SendTextSc(buf,PublicMessage,0,-2,-1);
	AddCombatMessage(m_pMsg2,red,green,blue);
	delete[] m_pMsg;
}

void CommandCMSGP::Activate(char *m_pArgs)
{
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	int m_Id = atoi(m_pArgs);
	char *m_pMsg = newstr(&strchr(m_pArgs,' ')[1]);
	if (!m_pMsg)
	{
		return;
	}
	char *m_pMsg2 = strchr(m_pMsg,' ');
	m_pMsg2[0] = 0;
	m_pMsg2++;
	if (!m_pMsg2)
	{
		return;
	}
	if (!_stricmp(m_pMsg,"private"))
	{
		Get_Private_Message_Color(&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"public"))
	{
		Get_Public_Message_Color(&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"gdi"))
	{
		Get_Team_Color(1,&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"nod"))
	{
		Get_Team_Color(0,&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"player"))
	{
		Get_Player_Color(m_Id,&red,&green,&blue);
	}
	else
	{
		if (!sscanf(m_pMsg,"%d,%d,%d",&red,&green,&blue))
		{
			red = 0;
			green = 0;
			blue = 0;
		}
	}
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%d\n%d\n%s\n",SS_ADDMESSAGE,red,green,blue,m_pMsg2);
	SendTextSc(buf,PrivateMessage,0,-2,m_Id);
	if ((!Exe) && (m_Id == cNetwork::PClientConnection->Get_Local_Id()))
	{
		AddCombatMessage(m_pMsg2,red,green,blue);
	}
	delete[] m_pMsg;
}
void CommandCMSGT::Activate(char *m_pArgs)
{
	unsigned int red;
	unsigned int green;
	unsigned int blue;
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long team = atoi(m_pArgs);
	char *m_pMsg = newstr(&strchr(m_pArgs,' ')[1]);
	if (!m_pMsg)
	{
		return;
	}
	char *m_pMsg2 = strchr(m_pMsg,' ');
	m_pMsg2[0] = 0;
	m_pMsg2++;
	if (!m_pMsg2)
	{
		return;
	}
	if (!_stricmp(m_pMsg,"private"))
	{
		Get_Private_Message_Color(&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"public"))
	{
		Get_Public_Message_Color(&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"gdi"))
	{
		Get_Team_Color(1,&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"nod"))
	{
		Get_Team_Color(0,&red,&green,&blue);
	}
	else if (!_stricmp(m_pMsg,"team"))
	{
		Get_Team_Color(team,&red,&green,&blue);
	}
	else
	{
		if (!sscanf(m_pMsg,"%d,%d,%d",&red,&green,&blue))
		{
			red = 0;
			green = 0;
			blue = 0;
		}
	}
	SLNode<GameObject> *x = GameObjManager::SmartGameObjList.Head();
	while (x)
	{
		GameObject *o = x->Data();
		if (o)
		{
			if ((Get_Object_Type(o) == team) || (team == 2))
			{
				long m_Id = Get_Player_ID(o);
				if ((m_Id) && (m_Id != -1))
				{
					char buf[512];
					sprintf(buf,"j\n%d\n%d\n%d\n%d\n%s\n",SS_ADDMESSAGE,red,green,blue,m_pMsg2);
					SendTextSc(buf,PrivateMessage,0,-2,m_Id);
					if ((!Exe) && m_Id == (cNetwork::PClientConnection->Get_Local_Id()))
					{
						AddCombatMessage(m_pMsg2,red,green,blue);
					}
				}
			}
		}
		x = x->Next();
	}
	delete[] m_pMsg;
}

void CommandDISARM::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (!m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	Disarm_All_C4(m_Id);
}

void CommandDISARMP::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (!m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	Disarm_All_Proxy_C4(m_Id);
}

void CommandDISARMB::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	if (!m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	Disarm_Beacons(m_Id);
}

void CommandRLMON::Activate(char *m_pArgs)
{
	if (Exe)
	{
		unsigned long a;
		unsigned long b;
		unsigned long c;
		unsigned long d;
		unsigned short port;
		unsigned long IP;
		if (!sscanf(m_pArgs,"%d.%d.%d.%d:%d",&a,&b,&c,&d,&port))
		{
			a = 0;
			b = 0;
			c = 0;
			d = 0;
			port = 0;
		}
		if ((a < 255) && (b < 255) && (c < 255) && (d < 255))
		{
			Console_Output("RenLogMon is on, sending to %d.%d.%d.%d:%d\n",a,b,c,d,port);
			IP = a + (b << 8) + (c << 16) + (d << 24);
			rlmonaddr.sin_addr.s_addr = IP;
			rlmonaddr.sin_port = htons(port);
			rlmonaddr.sin_family = AF_INET;
		}
		else
		{
			Console_Output("Invalid input to RenLogMon\n");
		}
	}
	else
	{
		Console_Output("RLMON is for Dedicated Servers only\n");
	}
}

void CommandRLMONOFF::Activate(char *m_pArgs)
{
	if (The_Game()->IsDedicated)
	{
		rlmonaddr.sin_addr.s_addr = 0;
		rlmonaddr.sin_port = 0;
		Console_Output("RenLogMon is off\n");
	}
	else
	{
		Console_Output("RLMONOFF is for Dedicated Servers only\n");
	}
}

void CommandPLIMIT::Activate(char* m_pArgs)
{
	cGameData* gameData = The_Game();
	if (!gameData)
	{
		Console_Output("Error: game is not active.\n");
		return;
	}
	
	unsigned int maxPlayers = atoi(m_pArgs);
	gameData->Set_Max_Players(maxPlayers);
	Console_Output("New player limit is %d\n", gameData->MaxPlayers);
}

void CommandPLIMITD::Activate(char *m_pArgs)
{
	cGameData* gameData = The_Game();
	if (!gameData)
	{
		Console_Output("Error: game is not active.\n");
		return;
	}

	Console_Output("Current player limit is %d\n", gameData->MaxPlayers);
}

void CommandTIME::Activate(char *m_pArgs)
{
	float time = (float)atof(m_pArgs);
	Change_Time_Remaining(time);
}

void CommandTIMEL::Activate(char *m_pArgs)
{
	int time = atoi(m_pArgs);
	Change_Time_Limit(time);
}

void CommandTIMED::Activate(char *m_pArgs)
{
	Console_Output("Time Remaining is %f\n",The_Game()->TimeRemaining_Seconds);
}

void CommandTIMELD::Activate(char *m_pArgs)
{
	Console_Output("Time Limit is %d\n",The_Game()->TimeLimit_Minutes);
}

void CommandMAPCH::Activate(char *m_pArgs)
{
	if ((!CombatManager::I_Am_Server()) || (cGameType::GameType == 1))
	{
		return;
	}
	long m_Id = atoi(m_pArgs);
	char *m_pMsg = strchr(m_pArgs,' ');
	if (!m_pMsg || !m_Id || !FindPlayer(m_Id))
	{
		return;
	}
	m_pMsg++;
	char buf[512];
	sprintf(buf,"j\n%d\n%d\n%s\n",SS_MAPCHECK,m_Id,m_pMsg);
	SendTextSc(buf,PrivateMessage,true,-2,m_Id);
}

void CommandSSURL::Activate(char *m_pArgs)
{
	if (!CombatManager::I_Am_Server() || cGameType::GameType == 1)
		return;
	
	ssurl = newstr(m_pArgs);
}

void CommandSSHOT::Activate(char *m_pArgs)
{
	if (!CombatManager::I_Am_Server() || cGameType::GameType == 1)
		return;
	
	if (!ssurl)
		return;
	
	long m_Id = atoi(m_pArgs);
	if (m_Id <= 0)
		return;
	
	char buf[512];
	sprintf(buf, "j\n%d\n%s\n", SS_SCREENSHOT, ssurl);
	SendTextSc(buf, PrivateMessage, true, -2, m_Id);
}



void CommandTAG::Activate(char* arguments)
{
	if (!CombatManager::I_Am_Server() || cGameType::GameType == 1)
		return;
	
	char* separatorPtr = strchr(arguments, ' ');
	if (!separatorPtr)
		return;
	
	*separatorPtr = '\0';
	int playerId = atol(arguments);
	const char* message = separatorPtr + 1;
	
	cPlayer* player = cPlayerManager::Find_Player(playerId);
	if (!player)
		return;
	
	player->customTag.Convert_From(message);
	player->Set_Object_Dirty_Bit(DB_RARE, true);
}



void CommandSERIAL::Activate(char* arguments)
{
	if (!CombatManager::I_Am_Server() || cGameType::GameType == 1)
		return;
	
	int playerId = atol(arguments);
	
	cRemoteHost* host = cNetwork::Get_Server_Rhost(playerId);
	if (!host)
		return;
	
	Console_Output("The serial hash of player %d is %32s\n", playerId, host->getSerial());
}




void CommandKICK2::Activate(char* arguments)
{
	if (cNetwork::I_Am_Server() && arguments && arguments[0])
	{
		int clientId = atoi(arguments);
		cRemoteHost* remoteHost = cNetwork::PServerConnection->Get_Remote_Host(clientId);

		if (remoteHost)
		{
			cNetwork::Server_Kill_Connection(clientId);
			cNetwork::Cleanup_After_Client(clientId);
		}
		else
			Console_Output("Player with id %d not found.\n", clientId);
	}
}
