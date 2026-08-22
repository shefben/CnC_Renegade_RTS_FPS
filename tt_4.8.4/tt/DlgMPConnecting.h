#ifndef TT_INCLUDE__DLGMPCONNECTING_H
#define TT_INCLUDE__DLGMPCONNECTING_H



#include "popupdialog.h"



class cGameData;



class DlgMPConnecting :
	public PopupDialogClass
{

private:

	int teamId; // 0D20  0D34
	uint32 localPort; // 0D24  0D38
	cGameData* gameData; // 0D28  0D3C
	bool isRefused; // 0D2C  0D40


public:

	static bool Create(int teamId, uint32 localPort);

	DlgMPConnecting(int _teamId, uint32 _localPort);
	virtual void On_Command(int controlId, int messageId, DWORD param);
	virtual void On_Periodic();
	
	void setGameData(cGameData* _gameData) { gameData = _gameData; }
	void handleRefusal() { isRefused = true; }

	int getTeamId() const { return teamId; }
	uint32 getLocalPort() const { return localPort; }
	cGameData* getGameData() const { return gameData; }

}; // 0D30  0D44



#endif