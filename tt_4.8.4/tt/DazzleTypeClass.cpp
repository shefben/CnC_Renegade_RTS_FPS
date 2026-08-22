#include "General.h"
#include "DazzleTypeClass.h"



#include "engine_math.h"



void DazzleTypeClass::Calculate_Intensities(float& dazzleIntensity, float& dazzleSize, float& haloIntensity, const Vector3& a5, const Vector3& a6, float distanceSqr) const
{
	if (distanceSqr >= fadeoutEndSqr)
	{
		dazzleSize = 0;
		haloIntensity = 0;
	}
	else
	{
		dazzleIntensity = clamp((dazzleIntensity - (1 - this->unk000C.dazzleArea)) / this->unk000C.dazzleArea, 0.f, 1.f);

		if (this->unk000C.dazzleDirectionArea > 0)
		{
			float v1 = (abs(a5 * a6) - (1 - this->unk000C.dazzleDirectionArea)) / this->unk000C.dazzleDirectionArea;
			dazzleIntensity *= clamp(v1, 0.f, 1.f);
		}
		
		if (dazzleIntensity <= 0)
			dazzleIntensity = 0;
		else
		{
			dazzleSize = pow(dazzleIntensity, this->unk000C.dazzleSizePow);
			dazzleIntensity = pow(dazzleIntensity, this->unk000C.dazzleIntensityPow);
		
			dazzleIntensity *= this->unk000C.dazzleIntensity;
			haloIntensity *= this->unk000C.haloIntensity;
		
			if (distanceSqr > fadeoutStartSqr)
			{
				float distanceFactor = 1.f - (sqrt(distanceSqr) - this->unk000C.fadeoutStart) / (this->unk000C.fadeoutEnd - this->unk000C.fadeoutStart);
				
				dazzleSize *= distanceFactor;
				haloIntensity *= distanceFactor;
			}
		}
	}
}
