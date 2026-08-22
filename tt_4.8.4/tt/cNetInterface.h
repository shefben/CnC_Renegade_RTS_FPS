#ifndef TT_INCLUDE__CNETINTERFACE_H
#define TT_INCLUDE__CNETINTERFACE_H



class WideStringClass;



class cNetInterface
{

	static REF_DECL2(mSidePreference, int);
	static REF_DECL2(Nickname, WideStringClass);

public:

	cNetInterface();
	~cNetInterface();
	static WideStringClass Get_Nickname();
	void Set_Nickname(WideStringClass&);
	void Set_Random_Nickname();
	static void Set_Side_Preference(int _sidePreference) { mSidePreference = _sidePreference; }
	static int Get_Side_Preference() { return mSidePreference; }

};



#endif