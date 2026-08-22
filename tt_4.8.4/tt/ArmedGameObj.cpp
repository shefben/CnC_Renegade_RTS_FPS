#include "general.h"
#include "ArmedGameObj.h"
#include "WeaponClass.h"
#include "bitstream.h"
#include "SmartGameObj.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
#include "PhysClass.h"
#include "RenderObjClass.h"



void ArmedGameObj::Export_Frequent(BitStreamClass& stream)
{
	PhysicalGameObj::Export_Frequent(stream);
	
	stream.Add(TargetVector.X,BITPACK_WORLD_POSITION_X);
	stream.Add(TargetVector.Y,BITPACK_WORLD_POSITION_Y);
	stream.Add(TargetVector.Z,BITPACK_WORLD_POSITION_Z);
}



void ArmedGameObj::Import_Frequent(BitStreamClass& stream)
{
	PhysicalGameObj::Import_Frequent(stream);
	
	Vector3 newTargetVector;
	stream.Get(newTargetVector.X,BITPACK_WORLD_POSITION_X);
	stream.Get(newTargetVector.Y,BITPACK_WORLD_POSITION_Y);
	stream.Get(newTargetVector.Z,BITPACK_WORLD_POSITION_Z);

	SmartGameObj* smart = As_SmartGameObj();
	if (!smart)
		Set_Targeting(newTargetVector, true);

	else if (!smart->Is_Controlled_By_Me())
	{
		if (smart->As_VehicleGameObj())
			TargetVector = newTargetVector;
		else
			Set_Targeting(newTargetVector, true);
	}
}



void ArmedGameObj::Export_State_Cs(BitStreamClass& stream)
{
	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		stream.Add(TargetVector.X);
		stream.Add(TargetVector.Y);
		stream.Add(TargetVector.Z);
	}
	else
	{
		Vector3 relativeTargetPosition;
		Get_Position(&relativeTargetPosition);
		relativeTargetPosition = TargetVector - relativeTargetPosition;
		
		stream.Add(relativeTargetPosition.X,BITPACK_WORLD_POSITION_X);
		stream.Add(relativeTargetPosition.Y,BITPACK_WORLD_POSITION_Y);
		stream.Add(relativeTargetPosition.Z,BITPACK_WORLD_POSITION_Z);
	}
}



void ArmedGameObj::Import_State_Cs(BitStreamClass& stream)
{
	Vector3 newTargetVector;

	if (cNetwork::Get_Server_Rhost(((cPacket&)stream).Get_Sender_Id())->getVersion() >= 4.0f)
	{
		stream.Get(newTargetVector.X);
		stream.Get(newTargetVector.Y);
		stream.Get(newTargetVector.Z);
	}
	else
	{
		Vector3 position;
		Get_Position(&position);
		
		Vector3 relativeTargetPosition;
		stream.Get(relativeTargetPosition.X,BITPACK_WORLD_POSITION_X);
		stream.Get(relativeTargetPosition.Y,BITPACK_WORLD_POSITION_Y);
		stream.Get(relativeTargetPosition.Z,BITPACK_WORLD_POSITION_Z);
		newTargetVector = position + relativeTargetPosition;
	}
	
	SmartGameObj* smart = As_SmartGameObj();
	if (!smart)
		Set_Targeting(newTargetVector, true);

	else if (!smart->Is_Controlled_By_Me())
	{
		if (smart->As_VehicleGameObj())
			TargetVector = newTargetVector;
		else
			Set_Targeting(newTargetVector, true);
	}
}



const Matrix3D& ArmedGameObj::Get_Muzzle(int muzzleId)
{
	RenderObjClass* model = Peek_Physical_Object()->Peek_Model();

	if (muzzleId == 3 && MuzzleBoneIndexes[3])
		return model->Get_Bone_Transform(MuzzleBoneIndexes[3]);
	
	if (muzzleId >= 2 && MuzzleBoneIndexes[2])
		return model->Get_Bone_Transform(MuzzleBoneIndexes[2]);
	
	if (muzzleId == 1 && MuzzleBoneIndexes[1])
		return model->Get_Bone_Transform(MuzzleBoneIndexes[1]);
	
	if (MuzzleBoneIndexes[0])
		return model->Get_Bone_Transform(MuzzleBoneIndexes[0]);
	
	return Get_Transform();
}



bool ArmedGameObj::Set_Targeting(const Vector3& target, bool b)
{
	if (&target != &TargetVector) // Don't update if the function is called with the current targeting. If we do, we trigger an assert. It would be better to make sure this function is never called in this way.
		TargetVector = target;

	return true;
}



void ArmedGameObj::Init_Muzzle_Bones()
{
	RenderObjClass* model = Peek_Physical_Object()->Peek_Model();
	
	int muzzlea0 = model->Get_Bone_Index("muzzlea0");
	int muzzlea1 = model->Get_Bone_Index("muzzlea1");
	int muzzleb0 = model->Get_Bone_Index("muzzleb0");
	int muzzleb1 = model->Get_Bone_Index("muzzleb1");
	
	MuzzleBoneIndexes[0] = muzzlea0;
	MuzzleBoneIndexes[1] = muzzlea1 ? muzzlea1 : muzzlea0;
	MuzzleBoneIndexes[2] = muzzleb0 ? muzzleb0 : muzzlea0;
	MuzzleBoneIndexes[3] = muzzleb1 ? muzzleb1 : (muzzleb0 ? muzzleb0 : muzzlea1);
	
	MuzzleRecoils[0].Init(MuzzleBoneIndexes[0]);
	MuzzleRecoils[1].Init(MuzzleBoneIndexes[1]);
	MuzzleRecoils[2].Init(MuzzleBoneIndexes[2]);
	MuzzleRecoils[3].Init(MuzzleBoneIndexes[3]);
	
	if (Get_Weapon())
		Get_Weapon()->Set_Model(model);
}



void ArmedGameObj::Post_Think()
{
	PhysicalGameObj::Post_Think();
	if (!Is_Delete_Pending())
	{
		if (Get_Weapon())
			Get_Weapon()->Update();
		
		if (Peek_Physical_Object()->Peek_Model())
			for (uint i = 0; i < 4; ++i)
				MuzzleRecoils[i].Update(Peek_Physical_Object()->Peek_Model());
	}
}

RENEGADE_FUNCTION
void ArmedGameObj::Init
   (const ArmedGameObjDef& oDefinition)
   AT2(0x0071E3C0,0x0071D980);

RENEGADE_FUNCTION
void ArmedGameObj::Re_Init
   (const ArmedGameObjDef& oDefinition)
   AT2(0x0071EEF0,0x0071E4B0);

RENEGADE_FUNCTION
bool ArmedGameObj::Load(ChunkLoadClass &cload)
AT2(0x0071F530,0x0071EAF0);
