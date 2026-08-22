#ifndef TT_INCLUDE__RADARMANAGER_H
#define TT_INCLUDE__RADARMANAGER_H



class Matrix3D;
class Vector2;
class Vector3;
class Render2DClass;
class Render2DSentenceClass;
class RadarMarkerClass;
class RectClass;
class ChunkSaveClass;
class ChunkLoadClass;
class PhysicalGameObj;
template<typename T> class DynamicVectorClass;



class RadarManager
{
public:
	static float RadarIntensity;
	static REF_DECL1(Markers, DynamicVectorClass<RadarMarkerClass>);
private:

	static Vector2 OldRadarCenter;
	static Vector2 RadarCenter;
	static Matrix3D RadarTM;
	static D3DCOLOR RadarColor;
	static uint RadarMode;

	static REF_DECL1(IsHidden, bool);
	static REF_DECL1(HiddenTimer, float);
	static REF_DECL1(Renderer, Render2DClass*);
	static REF_ARR_DECL1(BlipColors, uint, 8);
	static REF_ARR_DECL1(BlipUV, RectClass, 7);
	static REF_ARR_DECL1(CompassRenderers, Render2DSentenceClass *, 8);

public:

	static const PhysicalGameObj *BracketObj;
	static uint CurrentCompassRendererIndex;
	static const Vector2 RADAR_RINGS_UV_UL;
	static const Vector2 RADAR_RINGS_UV_LR;
	static const Vector2 RADAR_RINGS_L_OFFSET;
	static const Vector2 RADAR_RINGS_R_OFFSET;
	static const Vector2 RADAR_CENTER_OFFSET;


	static const char *Get_Blip_Shape_Type_Name(int);
	SHADERS_API static void Set_Hidden(bool);
	SHADERS_API static void Init();
	static void Shutdown();
	static bool Save(ChunkSaveClass&);
	static bool Load(ChunkLoadClass&);
	static float Add_Blip(const Vector3&, int, int, float, bool, bool);
	static void Update(const Matrix3D& _radarTranslation, const Vector2& _radarCenter);
	static void Render();
	static void Clear_Marker(int);
	static void Change_Marker_Color(int, int);

	SHADERS_API static void Set_Radar_Mode(uint radarMode) { RadarMode = radarMode; }
	static	void	Set_Bracket_Object( const PhysicalGameObj * obj )		{ BracketObj = obj; }

};



#endif