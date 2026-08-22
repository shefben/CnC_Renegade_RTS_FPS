#ifndef TT_INCLUDE__CGAMESPYQNR_H
#define TT_INCLUDE__CGAMESPYQNR_H



class StringClass;
class WideStringClass;



class CGameSpyQnR
{

private:

	static REF_DECL2(prodid, UNK);
	static REF_DECL2(cdkey_id, UNK);
	static REF_DECL2(bname, UNK);
	static REF_DECL2(gamename, UNK);
	static REF_DECL2(default_heartbeat_list, UNK);

	UNKB unk0004; // 0004
	UNKB unk0005; // 0005
	UNKB unk0006; // 0006
	UNKB unk0007; // 0007
	UNKB unk0008; // 0008
	UNKB unk0009; // 0009
	UNKB unk000A; // 000A
	UNK unk000C; // 000C
	UNK unk0010; // 0010
	UNK unk0014; // 0014
	UNK unk0018; // 0018
	UNK unk001C; // 001C; Time

public:

	CGameSpyQnR();
	virtual ~CGameSpyQnR();

	UNK LaunchArcade();
	UNK Shutdown();
	UNK TrackUsage();
	void Init();
	UNK DoGameStuff();
	UNK Think();
	UNK basic_callback(char*, int);
	UNK info_callback(char*, int);
	UNK rules_callback(char*, int);
	UNK Parse_HeartBeat_List(const char*);
	UNK Append_InfoKey_Pair(char*, int, const char*, const char*);
	UNK Append_InfoKey_Pair(char*, int, const char*, const WideStringClass&);
	UNK Append_InfoKey_Pair(char*, int, const char*, const StringClass&);
	UNK players_callback(char*, int);
	UNK Get_GameSpy_GameName();
	UNK Get_Default_HeartBeat_List();
	UNK Enable_Reporting(int);
	UNK IsEnabled();

}; // 0020


extern REF_DECL2(GameSpyQnR, CGameSpyQnR);


#endif