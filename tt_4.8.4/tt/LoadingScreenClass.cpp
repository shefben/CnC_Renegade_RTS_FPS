#include "General.h"
#include "LoadingScreenClass.h"

#include "TimeManager.h"
#include "WW3D.h"
#include "cNetwork.h"
#include "CombatManager.h"
#include "ConsoleModeClass.h"
#include "CampaignManager.h"
#include "TranslateDBClass.h"
#include "engine_string.h"
#pragma warning(disable:6031) //Return value ignored: 'sscanf'



float LoadingScreenClass::computePhaseEndTime(uint stage)
{
	switch (stage)
	{
	case 0: return 0.000000f / 0.544465f;
	case 1: return 0.000009f / 0.544465f;
	case 2: return 0.227275f / 0.544465f;
	case 3: return 0.245714f / 0.544465f;
	case 4: return 0.245716f / 0.544465f;
	case 5: return 0.245718f / 0.544465f;
	case 6: return 0.544465f / 0.544465f;
	default: return 1.f;
	}
	/*
	//The following are Westwood's time step values. StealthEye measured these to better match the current loading code.
	switch (stage)
	{
	case 0: return   1.f/180.f;
	case 1: return   2.f/180.f;
	case 2: return   3.f/180.f;
	case 3: return   7.f/180.f;
	case 4: return  38.f/180.f;
	case 5: return 155.f/180.f;
	case 6: return 176.f/180.f;
	default: return 1.f;
	}
	*/
}



LoadingScreenClass::LoadingScreenClass()
{
	elapsedTime = 0.001f; // To avoid division by 0?
	currentTime = 0;
	progress = 0;
	timeMultiplier = 0;
	
	if (!ConsoleBox.Is_Exclusive())
	{
		unk001C.Set_Texture_Size_Hint(0x100);
		unk001C.Set_Font(Fonts[11]);
	
		unk015C.Set_Texture_Size_Hint(0x100);
		unk015C.Set_Font(Fonts[12]);
	
		uint32 color = 0xFFFFFFFF;

		const RectClass& screen_res = Render2DClass::Get_Screen_Resolution();

		float dlg_width = screen_res.Width();
		float dlg_height = screen_res.Height();
		if (screen_res.Width() / screen_res.Height() <= 4.0f / 3.0f)
		{
			// smaller than 4:3, scale height
			dlg_height = screen_res.Width() * 3.0f / 4.0f;
		} 
		else
		{
			// wider than 4:3, scale width
			dlg_width = screen_res.Height() * 4.0f / 3.0f;
		}
	
		Vector2 screen_center = screen_res.Center();

		Vector2 position_bias = Vector2(screen_center.X - (dlg_width / 2), screen_center.Y - (dlg_height / 2));

	
		for (uint i = 0; i < CampaignManager::Get_Backdrop_Description_Count(); ++i )
		{
			StringClass description = CampaignManager::Get_Backdrop_Description(i);
			description.trim();
		
			if (description.startsWithI("Text2"))
			{
				description.truncateLeft(5);
				description.trimLeft();
			
				Vector2 location;
				sscanf(description, "%f,%f,", &location.X, &location.Y);
				const char* commaPos = strchr(description, ',');
				if (commaPos)
				{
					commaPos = strchr(commaPos + 1, ',');
					if (commaPos)
					{
						location.X *= dlg_width / 640.f;
						location.Y *= dlg_height / 480.f;
						location += position_bias;
						location.Floor(); // floor both components
					
						unk015C.Build_Sentence(TranslateDBClass::Get_String(commaPos + 1));
						unk015C.Set_Location(location);
						unk015C.Draw_Sentence(color);
						unk015C.Set_Wrapping_Width(0);
					
						color = 0xFFFFFFFF;
					}
				}
			}
			else if (description.startsWithI("Text"))
			{
				description.truncateLeft(4);
				description.trimLeft();
			
				Vector2 location;
				sscanf(description, "%f,%f,", &location.X, &location.Y);
				const char* commaPos = strchr(description, ',');
				if (commaPos)
				{
					commaPos = strchr(commaPos + 1, ',');
					if (commaPos)
					{
						location.X *= dlg_width / 640.f;
						location.Y *= dlg_height / 480.f;
						location += position_bias;
						location.Floor(); // floor both components
					
						unk001C.Build_Sentence(TranslateDBClass::Get_String(commaPos + 1));
						unk001C.Set_Location(location);
						unk001C.Draw_Sentence(color);
						unk001C.Set_Wrapping_Width(0);
					
						color = 0xFFFFFFFF;
					}
				}
			}
			else if (description.startsWithI("Wrap2"))
			{
				description.truncateLeft(5);
				description.trimLeft();
			
				float wrapWidth;
				sscanf(description, "%f,", &wrapWidth);
				wrapWidth *= dlg_width / 640.f;
				unk015C.Set_Wrapping_Width(wrapWidth);
			}
			else if (description.startsWithI("Wrap"))
			{
				description.truncateLeft(4);
				description.trimLeft();
			
				float wrapWidth;
				sscanf(description, "%f,", &wrapWidth);
				wrapWidth *= dlg_width / 640.f;
				unk001C.Set_Wrapping_Width(wrapWidth);
			}
			else if (description.startsWithI("Test"))
			{
				description.truncateLeft(4);
				description.trimLeft();
			
				Vector2 location;
				sscanf(description, "%f,%f,", &location.X, &location.Y);
			
				const char* commaPos = strchr(description, ',');
				if (commaPos)
				{
					commaPos = strchr(commaPos + 1, ',');
					if (commaPos)
					{
						WideStringClass string;
						string.Convert_From(commaPos + 1);
						unk001C.Build_Sentence(string);
					
						location.X *= dlg_width / 640.f;
						location.Y *= dlg_height / 480.f;
						location += position_bias;
						location.Floor(); // floor both components

						unk001C.Set_Location(location);
						unk001C.Draw_Sentence(color);
						unk001C.Set_Wrapping_Width(0);
					
						color = 0xFFFFFFFF;
					}
				}
			}
			else if (description.startsWithI("Model"))
			{
				description.truncateLeft(5);
				description.trimLeft();
			
				StringClass animation;
				animation.Format("%s.%s", description, description);
				
				unk0000.Set_Model(description);
				unk0000.Set_Animation(animation);
				unk0000.Set_Animation_Percentage(0);
			}
			else if (description.startsWithI("Color"))
			{
				description.truncateLeft(5);
				description.trimLeft();
				
				Vector3 colorVector;
				sscanf(description, "%f,%f,%f", &colorVector.X, &colorVector.Y, &colorVector.Z);
				
				color = D3DCOLOR_XRGB((uint8)colorVector.X, (uint8)colorVector.Y, (uint8)colorVector.Z);
			}
		}
	}
	//TODO: Needed? SaveLoadStatus::Reset_Status_Count();
}



void LoadingScreenClass::Render(bool a2)
{
	if (!ConsoleBox.Is_Exclusive())
	{
		TimeManager::Update_Frame_Time();
		elapsedTime += TimeManager::FrameSeconds;

		WW3D::Begin_Render(true, true, Vector3(0, 0, 0), a2 ? cNetwork::Update : NULL);

		unk0000.Render();
		unk001C.Render();
		unk015C.Render();
	
		static int lastPhase = 0;
		if (lastPhase != CombatManager::Get_Load_Progress())
		{
			lastPhase = CombatManager::Get_Load_Progress();
		
			currentTime = computePhaseEndTime(lastPhase);
			phaseEndTime = computePhaseEndTime(lastPhase + 1);
			timeMultiplier = currentTime / elapsedTime;
		}
	
		currentTime += TimeManager::FrameSeconds * timeMultiplier;
		currentTime = clamp(currentTime, 0.f, phaseEndTime);
		//progress += (currentTime - progress) * .1f; // Westwood's method to cope with small differences.
		progress = currentTime;

		unk0000.Set_Animation_Percentage(progress);
		if (ConsoleBox.Is_Exclusive())
		{
			int percentage = cMathUtil::Round(progress * 100.f);
			static int lastPercentage = -1;
			if (percentage != lastPercentage)
			{
				lastPercentage = percentage;
				ConsoleBox.Print("Load %d%% complete\r", percentage);
			}
		}
	
		WW3D::End_Render(true);
	}
}
