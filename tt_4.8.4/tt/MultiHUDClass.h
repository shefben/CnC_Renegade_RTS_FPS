#ifndef TT_INCLUDE__MULTIHUDCLASS_H
#define TT_INCLUDE__MULTIHUDCLASS_H



class Render2DSentenceClass;
class WideStringClass;



class MultiHUDClass
{

private:
	
	static REF_DECL1(MAX_OVERLAY_DISTANCE_M, UNK);
	static REF_DECL1(Y_INCREMENT_FACTOR, UNK);
	static REF_DECL1(NameRenderer, Render2DSentenceClass*);
	static REF_DECL1(BottomTextYPos, UNK);
	static REF_DECL1(PlayerlistFormat, UNK);
	static REF_DECL1(IsOn, UNK);

public:

	static void Init();
	static void Shutdown();
	static void Render_Text(WideStringClass& text, float x, float y, D3DCOLOR color);
	static UNK Toggle();
	static bool Is_On();
	static void Render();
	static void Show_Player_Names();
	static void Think();
	static UNK Next_Playerlist_Format();


};


#endif