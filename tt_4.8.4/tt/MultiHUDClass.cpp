#include "general.h"
#include "MultiHUDClass.h"
#include "engine_3dre.h"
#include "render2d.h"
#include "GameModeManager.h"
#include "GameModeClass.h"
#include "CombatManager.h"
#include "cNetwork.h"
#include "cUserOptions.h"
#include "cPlayerManager.h"
#include "SmartGameObj.h"
#include "CCameraClass.h"
#include "SoldierGameObj.h"
#include "CastResultStruct.h"
#include "physcoltest.h"
#include "PhysicsSceneClass.h"
#include "VehicleGameObj.h"
#include "TexProjectClass.h"
#include "GameObjManager.h"
#include "cPlayer.h"
// TODO: Move
D3DCOLOR GetD3dColor(const Vector3& color, float alpha)
{
	uint8 d3dColor[4];
	
	d3dColor[0] = (uint8)(color.Z * 255.f);
	d3dColor[1] = (uint8)(color.Y * 255.f);
	d3dColor[2] = (uint8)(color.X * 255.f);
	d3dColor[3] = (uint8)(alpha * 255.f);
	
	return (D3DCOLOR&)d3dColor;
}



REF_DEF1(MultiHUDClass::NameRenderer, Render2DSentenceClass*, 0x0082A1E4);



void MultiHUDClass::Render()
{
	extern bool RenderHud;
	if (NameRenderer && RenderHud)
		NameRenderer->Render();
}



void MultiHUDClass::Show_Player_Names()
{
	if (NameRenderer &&
		!GameModeManager::Find("Menu")->Is_Active() &&
		COMBAT_CAMERA &&
		cNetwork::PClientConnection &&
		cUserOptions::ShowNamesOnSoldier)
	{
		// Disallow star collisions in first person mode
		if (CombatManager::Is_First_Person() && CombatManager::Get_The_Star())
			(CombatManager::Get_The_Star()->Peek_Physical_Object()->Flags += 0x10000000);

		for (SLNode<SmartGameObj>* iter = (SLNode<SmartGameObj>*)GameObjManager::SmartGameObjList.Head(); iter; iter = iter->Next()) // Actually GameObjManager::SmartGameObjList
		{
			SmartGameObj* gameObject = iter->Data();
			cPlayer* player = cPlayerManager::Find_Player(gameObject->Get_Control_Owner());

			if (gameObject->Is_Delete_Pending() ||
				gameObject->Get_Defense_Object()->Get_Health() <= 0.0f ||
				!gameObject->Get_Player_Data() ||
				!player ||
				(CombatManager::Is_First_Person() && gameObject == CombatManager::Get_The_Star()))
				continue;
			
			SoldierGameObj* soldierGameObject = gameObject->As_SoldierGameObj();
			if (!soldierGameObject)
				continue;

			SmartGameObj* smartGameObject = soldierGameObject->Get_Vehicle() ? (SmartGameObj*)soldierGameObject->Get_Vehicle() : (SmartGameObj*)soldierGameObject;
			if ((smartGameObject->Is_Stealthed() && (!CombatManager::Get_The_Star() || !smartGameObject->Is_Teammate(CombatManager::Get_The_Star()))))
				continue;
			
			Vector3 position;
			gameObject->Get_Position(&position);
			position.Z += 1.3f;
			
			COMBAT_CAMERA->Validate_Transform();
			
			Vector3 cameraPosition = COMBAT_CAMERA->Get_Position();
			Vector3 positionOffset = position - cameraPosition;
			float distance = positionOffset.Length();

			if (COMBAT_CAMERA->Is_Star_Sniping())
				distance /= COMBAT_CAMERA->Get_Sniper_Zoom();
			
			if (distance > 50.f)
				continue;
			
			Vector3 screenPosition;
			if (COMBAT_CAMERA->Project(screenPosition, position) != 0)
				continue;
			
			LineSegClass line(cameraPosition + Normalize(positionOffset) * (COMBAT_CAMERA->Get_ZNear() + .1f), position);
			CastResultStruct result;
			PhysRayCollisionTestClass collisionTest(line, &result, 2);
			collisionTest.CollisionGroup = 3;
			
			PhysicsSceneClass::Get_Instance()->Cast_Ray(collisionTest, false);
			
			if (collisionTest.CollidedPhysObj && result.Fraction != 1.f)
			{
				if (!collisionTest.CollidedPhysObj->Get_Observer())
					continue;

				DamageableGameObj* damageableObserver = ((CombatPhysObserverClass *)(collisionTest.CollidedPhysObj->Get_Observer()))->As_DamageableGameObj();
				SmartGameObj* smartObserver = damageableObserver->As_SmartGameObj();
				if (!smartObserver)
					continue;

				if (smartObserver->As_SoldierGameObj() && smartObserver->As_SoldierGameObj()->Get_Vehicle())
					smartObserver = smartObserver->As_SoldierGameObj()->Get_Vehicle();

				if (smartObserver != smartGameObject)
					continue;
			}

			WideStringClass nameLabel = player->getNameLabel();
			
			Vector2 textExtents = NameRenderer->Get_Text_Extents(nameLabel);
			
			screenPosition.X = .5f * ((1 + screenPosition.X) * (Render2DClass::Get_Screen_Resolution().Right - Render2DClass::Get_Screen_Resolution().Left) - textExtents.X);
			screenPosition.Y = .5f * (1 - screenPosition.Y) * (Render2DClass::Get_Screen_Resolution().Bottom - Render2DClass::Get_Screen_Resolution().Top);
			
			D3DCOLOR color = GetD3dColor(player->Get_Color(), 1 - distance / 50.f);
			
			Render_Text(nameLabel, screenPosition.X, screenPosition.Y, color);
		}

		// Reallow star collisions in first person mode
		if (CombatManager::Is_First_Person() && CombatManager::Get_The_Star())
			(CombatManager::Get_The_Star()->Peek_Physical_Object()->Flags -= 0x10000000);
	}
}



void MultiHUDClass::Render_Text(WideStringClass& text, float x, float y, D3DCOLOR color)
{
	if (!MultiHUDClass::NameRenderer)
		return;
	
	MultiHUDClass::NameRenderer->Set_Location(Vector2((float)cMathUtil::Round(x), (float)cMathUtil::Round(y)));
	MultiHUDClass::NameRenderer->Build_Sentence(text);
	MultiHUDClass::NameRenderer->Draw_Sentence(color);
}




RENEGADE_FUNCTION
void MultiHUDClass::Init()
AT2(0x00462FA0, 0x004628D0);



RENEGADE_FUNCTION
void MultiHUDClass::Shutdown()
AT2(0x00462FF0, 0x00462920);



RENEGADE_FUNCTION
UNK MultiHUDClass::Toggle()
AT2(0x004630A0, 0x004629D0);



RENEGADE_FUNCTION
bool MultiHUDClass::Is_On()
AT2(0x00463120, 0x00462A50);



RENEGADE_FUNCTION
void MultiHUDClass::Think()
AT2(0x00463BC0, 0x004634F0);



RENEGADE_FUNCTION
UNK MultiHUDClass::Next_Playerlist_Format()
AT2(0x00463C10, 0x00463540);
