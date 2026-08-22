#ifndef TT_INCLUDE__SERVERSETTINGSCLASS_H
#define TT_INCLUDE__SERVERSETTINGSCLASS_H



class StringClass;



class ServerSettingsClass
{

public:

	static REF_DECL2(IsActive, bool);
	static REF_DECL2(GameMode, int);
	static REF_DECL2(MasterBandwidth, int);
	static REF_DECL2(DiskLogSize, int);

	void Set_Settings_File_Name(char*);
	bool Parse(bool);
	void Encrypt_Serial(StringClass, StringClass&, bool);
	void Decrypt_Serial(StringClass, StringClass&);
	bool Check_Game_Settings_File(char*);

	static int Get_Master_Bandwidth() { return MasterBandwidth; }

};



#endif