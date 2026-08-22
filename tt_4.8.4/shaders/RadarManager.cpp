#include "general.h"
#include "RadarManager.h"
#include "engine_math.h"
#include "engine_tdb.h"
#include "engine_obj.h"
#include "Matrix3D.h"
#include "render2d.h"
#include "TimeManager.h"
#include "SmartGameObj.h"
#include "ObjectiveManager.h"
#include "Objective.h"
#include "RadarMarkerClass.h"
#include "CombatManager.h"
#include "wwmath.h"
#include "SoldierGameObj.h"
#include "slist.h"
#include "GameObjManager.h"

extern bool HideCompass;
extern bool HideRadar;



const Vector2 INFO_UV_SCALE(0.00390625,0.00390625);

const Vector2 RadarManager::RADAR_RINGS_UV_UL(95, 0);
const Vector2 RadarManager::RADAR_RINGS_UV_LR(197, 53);
const Vector2 RadarManager::RADAR_RINGS_L_OFFSET(-51, -50);
const Vector2 RadarManager::RADAR_RINGS_R_OFFSET(0, -50);
const Vector2 RadarManager::RADAR_CENTER_OFFSET(55, 78);
const PhysicalGameObj *RadarManager::BracketObj;
Vector2 RadarManager::OldRadarCenter(0, 0);
Vector2 RadarManager::RadarCenter(0, 0);
Matrix3D RadarManager::RadarTM;
REF_DEF1(RadarManager::IsHidden, bool, 0x00856FB8);
REF_DEF1(RadarManager::HiddenTimer, float, 0x00856FBC);
float RadarManager::RadarIntensity;
REF_DEF1(RadarManager::Renderer, Render2DClass*, 0x00856EA8);
uint RadarManager::CurrentCompassRendererIndex;
REF_ARR_DEF1(RadarManager::CompassRenderers, Render2DSentenceClass *, 8, 0x00856E7C);
uint RadarManager::RadarMode;
REF_DEF1(RadarManager::Markers, DynamicVectorClass<RadarMarkerClass>, 0x00856F20);
D3DCOLOR RadarManager::RadarColor;
REF_ARR_DEF1(RadarManager::BlipColors, uint, 8, 0x00856F78);
REF_ARR_DEF1(RadarManager::BlipUV, RectClass, 7, 0x00856EB0);



RENEGADE_FUNCTION
const char *RadarManager::Get_Blip_Shape_Type_Name(int)
AT2(0x0067DB70, 0x0067D410);



RENEGADE_FUNCTION
void RadarManager::Set_Hidden(bool)
AT2(0x0067DB80, 0x0067D420);



RENEGADE_FUNCTION
void RadarManager::Init()
AT2(0x0067DC40, 0x0067D4E0);



RENEGADE_FUNCTION
void RadarManager::Shutdown()
AT2(0x0067E0D0, 0x0067D970);



RENEGADE_FUNCTION
bool RadarManager::Save(ChunkSaveClass&)
AT2(0x0067E1B0, 0x0067DA50);



RENEGADE_FUNCTION
bool RadarManager::Load(ChunkLoadClass&)
AT2(0x0067E250, 0x0067DAF0);



float RadarManager::Add_Blip(const Vector3& position, int shape, int colorId, float intensity, bool isBracket, bool isSmall)
{
	const float radarScale = 84.f;
	const float radarSize = 50.4f;
	const float radius = 4.f;
	const float smallScale = 2.f/3.f;

	if (shape == 0)
		return intensity;
	
	Vector3 relativePosition = RadarTM.applyTo(position);
	relativePosition.Z = 0;

	relativePosition *= radarScale / 100.f;


	if (relativePosition.Length() >= radarSize)
	{
		if (shape == 4)
		{
			relativePosition.Normalize();
			relativePosition *= radarSize;
		}
		else
			return intensity;
	}
	
	intensity = 1;
	
	if (!Renderer)
		return intensity;
	
	float blipIntensity;
	if (shape == 4)
		blipIntensity = RadarIntensity;
	else
	{
		blipIntensity = intensity - (relativePosition.Length() - 42.f) / (radarScale / 10);
		blipIntensity = RadarIntensity * clamp(blipIntensity, 0.f, 1.f);
	}

	if (isSmall)
		blipIntensity *= smallScale;
	
	float blipRadius = isSmall ? smallScale * radius : radius;
	Vector2 blipOffset = RadarCenter + Vector2(1.f - relativePosition.X, 2.f + relativePosition.Y);
	
	RectClass rect;
	rect.Left = blipOffset.X - blipRadius;
	rect.Top = blipOffset.Y - blipRadius;
	rect.Right = blipOffset.X + blipRadius;
	rect.Bottom = blipOffset.Y + blipRadius;
	
	int blipColor = ((uint)(blipIntensity * 255.f) << 24) | (BlipColors[colorId] & 0xFFFFFF);
	Renderer->Add_Quad(rect, RadarManager::BlipUV[shape], blipColor);
	
	if (isBracket)
	{
		uint bracketColor = ((uint)(blipIntensity * 255.f) << 24) | 0x00FF00;
		Renderer->Add_Quad(rect, RadarManager::BlipUV[5], bracketColor);
	}
	
	return intensity;
}



void RadarManager::Update(const Matrix3D& _radarTranslation, const Vector2& _radarCenter)
{
	OldRadarCenter = RadarCenter;
	RadarCenter = _radarCenter;
	
	RadarTM = _radarTranslation;
	RadarTM.rotateZ(-WWMATH_PI/2);
    
	if (IsHidden)
		HiddenTimer += TimeManager::FrameSeconds;
	else
		HiddenTimer -= TimeManager::FrameSeconds;
	
	HiddenTimer = clamp(HiddenTimer, 0.f, 1.f);
	
	
	RadarIntensity = 1 - HiddenTimer;
	
	RadarColor = D3DCOLOR_ARGB((int)(RadarIntensity * 255.f),  255, 255, 255);
	Renderer->Reset();
	
	if (RadarIntensity == 0)
		return;
	
	RectClass rect;
	rect.Left = RADAR_RINGS_UV_UL.X * INFO_UV_SCALE.X;
	rect.Top = RADAR_RINGS_UV_UL.Y * INFO_UV_SCALE.Y;
	rect.Right = RADAR_RINGS_UV_LR.X * INFO_UV_SCALE.X;
	rect.Bottom = RADAR_RINGS_UV_LR.Y * INFO_UV_SCALE.Y;
	
	Vector2 c1 = RadarCenter + RADAR_RINGS_L_OFFSET;
	
	Vector2 b1;
	b1.X = c1.X + RADAR_RINGS_UV_LR.Y - RADAR_RINGS_UV_UL.Y;
	b1.Y = c1.Y + RADAR_RINGS_UV_LR.X - RADAR_RINGS_UV_UL.X;


	Vector2 d1(b1.X, c1.Y);
	Vector2 a1(c1.X, b1.Y);

	Renderer->Add_Quad(a1, b1, c1, d1, rect, RadarColor);

	Vector2 a2 = b1 - RADAR_RINGS_L_OFFSET + RADAR_RINGS_R_OFFSET;
	Vector2 d2 = RadarCenter + RADAR_RINGS_R_OFFSET;
	Vector2 c2(a2.X, d2.Y);
	Vector2 b2(d2.X, a2.Y);
	
	Renderer->Add_Quad_Backfaced(a2, b2, c2, d2, rect, RadarColor);

	CurrentCompassRendererIndex = (uint)(_radarTranslation.getRotationZ() / (WWMATH_PI*2) * 8 + 10.5) % 8; // 8+2+.5: 8 to get rid of negative numbers, 2 because the labels are different from the angle, .5 for rounding
	Render2DSentenceClass* compassRenderer = CompassRenderers[CurrentCompassRendererIndex];
	
	if (!compassRenderer)
	{
		compassRenderer = new Render2DSentenceClass;
		CompassRenderers[CurrentCompassRendererIndex] = compassRenderer;

		compassRenderer->Set_Font(Fonts[11]); // StyleMgrClass::Fonts
		
		const wchar_t* compassLabel = Get_Wide_Translated_String(0x3160 + CurrentCompassRendererIndex); // This was previously done by an array
		compassRenderer->Build_Sentence(compassLabel);
		
		Vector2 textExtents = compassRenderer->Get_Text_Extents(compassLabel);
		
		Vector2 location = _radarCenter + Vector2(2.f, -73.f) - textExtents * .5f;
		location.X = floor(location.X);
		location.Y = floor(location.Y);
		
		compassRenderer->Set_Location(location);
		compassRenderer->Draw_Sentence(0xFFFFFFFF);
	}
	else if (RadarCenter != OldRadarCenter)
	{
		compassRenderer->Reset();
		
		const wchar_t* compassLabel = Get_Wide_Translated_String(0x3160 + CurrentCompassRendererIndex); // This was previously done by an array
		compassRenderer->Build_Sentence(compassLabel);
		Vector2 textExtents = compassRenderer->Get_Text_Extents(compassLabel);
		
		Vector2 location = RadarCenter + Vector2(2.f, -73.f) - textExtents * .5f;
		location.X = floor(location.X);
		location.Y = floor(location.Y);
		
		compassRenderer->Set_Location(location);
		compassRenderer->Draw_Sentence(0xFFFFFFFF);
	}
	
	Vector3 starPosition;
	int starPlayerType = 0;
	if (CombatManager::Get_The_Star())
	{
		CombatManager::Get_The_Star()->Get_Position(&starPosition);
		starPlayerType = CombatManager::Get_The_Star()->Get_Player_Type();
	}
	
	for (SLNode<BaseGameObj>* node = (SLNode<BaseGameObj>*)GameObjManager::GameObjList.Head(); node; node = node->Next())
	{
		PhysicalGameObj* gameObject = node->Data()->As_PhysicalGameObj();
		if (gameObject &&
			gameObject != CombatManager::Get_The_Star() &&
			gameObject->Peek_Physical_Object() &&
			(!gameObject->As_SoldierGameObj() || gameObject->Get_Defense_Object()->Get_Health() > 0.0f) &&
			(!CombatManager::Get_The_Star() || RadarMode == 2 || (RadarMode != 0 && starPlayerType == gameObject->Get_Player_Type() && (uint)gameObject->Get_Player_Type() < 2)))
		{
			SmartGameObj* smartGameObject = gameObject->As_SmartGameObj();
			if (!smartGameObject ||
				!smartGameObject->Is_Stealthed() ||
				starPlayerType == smartGameObject->Get_Player_Type())
			{
				Vector3 gameObjectPosition;
				gameObject->Get_Position(&gameObjectPosition);
				
				float radarBlipIntensity = Add_Blip(gameObjectPosition, gameObject->Get_Radar_Blip_Shape_Type(), gameObject->Get_Radar_Blip_Color_Type(), gameObject->Get_Radar_Blip_Intensity(), gameObject == RadarManager::BracketObj, abs(starPosition.Z - gameObjectPosition.Z) > 3.f);
				
				gameObject->Set_Radar_Blip_Intensity(clamp(radarBlipIntensity, 0.f, 1.f));
			}
		}
	}
	for (int i = 0; i < ObjectiveManager::ObjectiveList.Count(); i++)
	{
		Objective* objective = ObjectiveManager::ObjectiveList[i];
		if (objective->showOnRadar &&
			objective->status == Objective::Pending)
		{
			float radarBlipIntensity = Add_Blip(objective->location, 4, objective->Radar_Blip_Color_Type(), objective->radarBlipIntensity, 0, 0);
			objective->radarBlipIntensity = clamp(radarBlipIntensity, 0.f, 1.f);
		}
	}
	
	for (int i = 0; i < Markers.Count(); i++)
	{
		RadarMarkerClass* marker = &Markers[i];
		
		float radarBlipIntensity = Add_Blip(marker->location, marker->radarBlipShape, marker->radarBlipColorType, marker->radarBlipIntensity, false, false);
		marker->radarBlipIntensity = clamp(radarBlipIntensity, 0.f, 1.f);
	}
}



void RadarManager::Render()
{
	if (HiddenTimer < 1)
	{
		if (!HideRadar)
			Renderer->Render();

		if (!HideCompass)
			CompassRenderers[CurrentCompassRendererIndex]->Render();
	}
}



RENEGADE_FUNCTION
void RadarManager::Clear_Marker(int)
AT2(0x0067F200, 0x0067EAA0);



RENEGADE_FUNCTION
void RadarManager::Change_Marker_Color(int, int)
AT2(0x0067F290, 0x0067EB30);
