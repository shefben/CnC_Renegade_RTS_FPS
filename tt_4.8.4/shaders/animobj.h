#ifndef TT_INCLUDE__ANIMOBJ_H
#define TT_INCLUDE__ANIMOBJ_H
#include "composite.h"
#include "Matrix3D.h"
class RenderInfoClass;
class SpecialRenderInfoClass;
class HAnimClass;
class HAnimComboClass;
class HTreeClass;
class Animatable3DObjClass : public CompositeRenderObjClass {
public:
	~Animatable3DObjClass();
	void Render(RenderInfoClass& rinfo);
	void Special_Render(SpecialRenderInfoClass& rinfo);
	void Set_Transform(const Matrix3D &m);
	void Set_Position(const Vector3 &v);
	void Set_Animation();
	void Set_Animation(HAnimClass* motion,float frame,int anim_mode);
	void Set_Animation(HAnimClass* motion0,float frame0,HAnimClass* motion1,float frame1,float percentage);
	void Set_Animation(HAnimComboClass* anim_combo);
	HAnimClass *Peek_Animation();
	virtual bool Is_Animation_Complete() const;
	int Get_Num_Bones();
	const char *Get_Bone_Name(int bone_index);
	int Get_Bone_Index(const char* bonename);
	const Matrix3D& Get_Bone_Transform(const char* bonename);
	const Matrix3D& Get_Bone_Transform(int boneindex);
	void Capture_Bone(int boneindex);
	void Release_Bone(int boneindex);
	bool Is_Bone_Captured(int boneindex) const;
	void Control_Bone(int bindex,const Matrix3D& objtm,bool world_space_translation);
	HTreeClass *Get_HTree() const;
	virtual bool Simple_Evaluate_Bone(int boneindex,Matrix3D *tm) const;
	virtual bool Simple_Evaluate_Bone(int boneindex,float frame,Matrix3D *tm) const;
	virtual void Set_HTree(HTreeClass* htree);
protected:
	void Update_Sub_Object_Transforms();
	enum {
		NONE = 0,
		BASE_POSE,
		SINGLE_ANIM,
		DOUBLE_ANIM,
		MULTIPLE_ANIM,
	};
	bool IsTreeValid;
	HTreeClass *HTree;
	int CurMotionMode;
	union {
		struct {
			HAnimClass* Motion;
			float Frame;
			float PrevFrame;
			int AnimMode;
			int LastSyncTime;
		} ModeAnim;
		struct {
			HAnimClass* Motion0;
			HAnimClass* Motion1;
			float Frame0;
			float Frame1;
			float PrevFrame0;
			float PrevFrame1;
			float Percentage;
		} ModeInterp;
		struct {
			HAnimComboClass* AnimCombo;
		} ModeCombo;
	};
};
#endif
