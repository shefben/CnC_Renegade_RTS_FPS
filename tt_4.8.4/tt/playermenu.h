#ifndef TT_INCLUDE_PLAYERMENU_H
#define TT_INCLUDE_PLAYERMENU_H

class DlgCncBattleInfo : public MenuDialogClass {
public:
	DlgCncBattleInfo();
	~DlgCncBattleInfo();
	void On_Init_Dialog();
	void On_Frame_Update();
};

class DlgCncTeamInfo : public MenuDialogClass {
public:
	DlgCncTeamInfo();
	~DlgCncTeamInfo();
	void On_Init_Dialog();
	void On_Frame_Update();
};

class DlgCncServerInfo : public MenuDialogClass {
public:
	DlgCncServerInfo();
	~DlgCncServerInfo();
	void On_Init_Dialog();
	void On_Frame_Update();
};

#endif