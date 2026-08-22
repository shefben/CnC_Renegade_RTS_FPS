#ifndef TT_INCLUDE__SCREENFADEMANAGER_H
#define TT_INCLUDE__SCREENFADEMANAGER_H



class ChunkLoadClass;
class ChunkSaveClass;
class Vector3;



class ScreenFadeManager
{

public:

	static void Think();
	static bool Load(ChunkLoadClass&);
	static void Init();
	static void Shutdown();
	static void Render();
	static void Enable_Letterbox(bool, float);
	static void Set_Screen_Overlay_Color(Vector3 const&, float);
	static void Set_Screen_Overlay_Color(float, float, float, float);
	static void Set_Screen_Overlay_Opacity(float, float);
	static bool Save(ChunkSaveClass&);

};



#endif