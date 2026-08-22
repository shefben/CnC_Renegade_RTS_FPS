#ifndef TT_INCLUDE__HUMANANIMCONTROLCLASS_H
#define TT_INCLUDE__HUMANANIMCONTROLCLASS_H
#include "SimpleAnimControlClass.h"
#include "engine_vector.h"
class HAnimComboDataClass;
class HAnimComboClass {
public:
	DynamicVectorClass<HAnimComboDataClass *> HAnimComboData;
};
class HumanAnimControlClass : public AnimControlClass
{
public:
	HumanAnimControlClass( void );
	~HumanAnimControlClass( void );
	virtual	bool 	Save( ChunkSaveClass & csave );
	virtual	bool	Load( ChunkLoadClass & cload );
	virtual	void	Set_Model( RenderObjClass	*anim_model );
	virtual	void	Set_Animation( const char *name, float	blendtime = 0.0f, float start_frame = 0.0f  );
	virtual	void	Set_Animation( const HAnimClass * anim, float blendtime = 0.0f, float start_frame = 0.0f  );
	virtual	void	Set_Animation( const char *name1, const char * name2, float ratio, float blendtime = 0.0f );
	virtual	void	Set_Mode( AnimMode mode, float frame = -1 );
	virtual	AnimMode	Get_Mode( void )					{ return Channel1.Get_Mode(); }
	virtual	bool	Is_Complete( void )					{ return Channel1.Is_Complete(); }
	virtual	const char *Get_Animation_Name( void )	{ return Channel1.Get_Animation_Name(); }
	virtual	float	Get_Frame( void )						{ return Channel1.Get_Frame(); }
	virtual	float	Get_Progress( void )					{ return Channel1.Get_Progress(); }
	virtual	void	Set_Target_Frame( float frame )	{ Channel1.Set_Target_Frame( frame ); }
	virtual	float	Get_Target_Frame( void )			{ return Channel1.Get_Target_Frame(); }
	virtual	float	Get_Current_Frame( void )			{ return Channel1.Get_Frame(); }
	virtual	void	Set_Anim_Speed_Scale( float speed )		{ AnimSpeedScale = speed; }
	virtual	void	Update( float dtime );
	virtual HAnimClass *	Peek_Animation( void )		{ return Channel1.Peek_Animation(); }
	void				Get_Information( StringClass & string );
	char				Get_Skeleton( void )					{ return Skeleton; }
private:
	BlendableAnimChannelClass	Channel1;
	BlendableAnimChannelClass	Channel2;
	float								Channel2Ratio;
	float								AnimSpeedScale;
	AnimationDataList				DataList;
	HAnimComboClass				AnimCombo;
	char								Skeleton;
	void								Build_Skeleton_Anim_Name( StringClass& new_name, const char * name );
};

#endif
