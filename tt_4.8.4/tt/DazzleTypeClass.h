#ifndef TT_INCLUDE__DAZZLETYPECLASS_H
#define TT_INCLUDE__DAZZLETYPECLASS_H



#include "engine_string.h"
#include "Vector3.h"
#include "Vector2.h"


class DazzleTypeClass;



class DazzleInitClass
{

private:

	friend DazzleTypeClass;

	uint32 id; // 0000
	bool useCameraTranslation; // 0004
	StringClass dazzleTextureName; // 0008
	StringClass haloTextureName; // 000C
	StringClass lensflareName; // 0010
	float haloIntensity; // 0014
	Vector2 haloScale; // 0018
	float dazzleSizePow; // 0020
	float dazzleIntensityPow; // 0024
	float dazzleIntensity; // 0028
	float dazzleArea; // 002C
	float dazzleDirectionArea; // 0030
	Vector3 dazzleDirection; // 0034
	Vector3 dazzleTestColor; // 0040
	Vector3 dazzleColor; // 004C
	Vector3 haloColor; // 0058
	Vector2 dazzleScale; // 0064
	float fadeoutStart; // 006C
	float fadeoutEnd; // 0070
	float sizeOptimizationLimit; // 0074
	float historyWeight; // 0078
	float radius; // 007C
	uint32 blinkPeriod; // 0080
	uint32 blinkOnTime; // 0084

public:

}; // 0088



class DazzleTypeClass
{

private:

	uint32 unk0004; // 0004
	uint32 unk0008; // 0008
	DazzleInitClass unk000C; // 000C
	float fadeoutEndSqr; // 0094
	float fadeoutStartSqr; // 0098
	StringClass unk009C; // 009C
	D3DCOLOR unk00A0; // 00A0; Perhaps type is incorrect
	D3DCOLOR unk00A4; // 00A4; Perhaps type is incorrect

public:

	virtual ~DazzleTypeClass();
	virtual void Calculate_Intensities(float& a2, float& a3, float& a4, const Vector3& a5, const Vector3& a6, float a7) const;

};



#endif