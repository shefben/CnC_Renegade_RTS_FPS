#ifndef TT_INCLUDE__SIMPLEANIMCONTROLCLASS_H
#define TT_INCLUDE__SIMPLEANIMCONTROLCLASS_H

#include "AnimControlClass.h"
#include "Matrix3d.h"
#include "HAnimClass.h"
#include "scripts.h"
#include "engine_common.h"
#include "engine_vector.h"
#include "engine_io.h"
struct AnimationDataRecord {
	HAnimClass	*Animation;
	float			Frame;
	float			Weight;

	bool operator == (AnimationDataRecord const & rec) const	{ return false; }
	bool operator != (AnimationDataRecord const & rec) const	{ return true; }
};
typedef	DynamicVectorClass<AnimationDataRecord>	AnimationDataList;
class AnimChannelClass
{
public:
	AnimChannelClass( void );
	AnimChannelClass(const AnimChannelClass &);
	AnimChannelClass & operator = (const AnimChannelClass &);
	~AnimChannelClass(void);
	virtual	bool 	Save( ChunkSaveClass & csave );
	virtual	bool	Load( ChunkLoadClass & cload );
	void			Set_Animation( const char *name );
	void			Set_Animation( const HAnimClass *anim );
	HAnimClass	*Peek_Animation( void )		{ return Animation; }
	void			Set_Mode( AnimMode mode, float frame = -1 );
	AnimMode		Get_Mode( void )					{ return Mode; }
	bool			Is_Complete( void )				{ return (Animation == NULL) || ((Mode == ANIM_MODE_ONCE) && (Frame == NumFrames-1)) || ((Mode == ANIM_MODE_TARGET) && (Frame == TargetFrame)); }
	const char *Get_Animation_Name( void )		{ return Animation ? Animation->Get_Name() : ""; }
	void			Set_Frame( float frame )	{ Frame = frame; }
	float			Get_Frame( void )				{ return Frame; }
	float			Get_Progress( void )			{ return NumFrames ? Frame / NumFrames : 0; }
	void			Set_Target_Frame( float frame )	{ TargetFrame = frame; }
	float			Get_Target_Frame( void )			{ return TargetFrame; }
	void			Update( float dtime );
	void			Get_Animation_Data( AnimationDataList & list, float weight = 1.0f  );
	void			Update_Model( RenderObjClass	*anim_model );
private:
	HAnimClass *	Animation;
	float			Frame;
	float			NumFrames;
	float			TargetFrame;
	AnimMode		Mode;
};

class BlendableAnimChannelClass
{
public:
	BlendableAnimChannelClass( void );
	virtual	bool 	Save( ChunkSaveClass & csave );
	virtual	bool	Load( ChunkLoadClass & cload );
	void			Set_Animation( const char *name, float blendtime = 0.0f, float start_frame = 0.0f );
	void			Set_Animation( const HAnimClass * anim, float blendtime = 0.0f, float start_frame = 0.0f  );
	void			Set_Mode( AnimMode mode, float frame = -1 )	{ NewChannel.Set_Mode( mode, frame ); }
	AnimMode		Get_Mode( void )					{ return NewChannel.Get_Mode(); }
	bool	Is_Complete( void )						{ return NewChannel.Is_Complete(); }
	const char *Get_Animation_Name( void )		{ return NewChannel.Get_Animation_Name(); }
	void			Set_Target_Frame( float frame )	{ NewChannel.Set_Target_Frame ( frame ); }
	float			Get_Target_Frame( void )			{ return NewChannel.Get_Target_Frame(); }
	HAnimClass	*Peek_Animation( void )			{ return NewChannel.Peek_Animation (); }
	void			Update( float dtime );
	void			Get_Animation_Data( AnimationDataList & list, float weight = 1.0f  );
	void			Update_Model( RenderObjClass	*anim_model );
	float			Get_Frame( void )					{ return NewChannel.Get_Frame(); }
	float			Get_Progress( void )				{ return NewChannel.Get_Progress(); }
private:
	AnimChannelClass	NewChannel;
	AnimChannelClass	OldChannel;
	float					BlendTimer;
	float					BlendTotal;
};

class SimpleAnimControlClass : public AnimControlClass
{
public:
	SimpleAnimControlClass( void );
	~SimpleAnimControlClass( void );
	virtual	bool 	Save( ChunkSaveClass & csave );
	virtual	bool	Load( ChunkLoadClass & cload );
	virtual	void	Set_Animation( const char *name, float blendtime = 0.0f, float start_frame = 0.0f  );
	virtual	void	Set_Animation( const HAnimClass * anim, float blendtime = 0.0f, float start_frame = 0.0f  );
	virtual	void	Set_Mode( AnimMode mode, float frame = -1 )	{ Channel.Set_Mode( mode, frame ); }
	virtual	AnimMode	Get_Mode( void )									{ return Channel.Get_Mode(); }
	virtual	bool	Is_Complete( void )									{ return Channel.Is_Complete(); }
	virtual	const char *Get_Animation_Name( void )					{ return Channel.Get_Animation_Name(); }
	virtual	void	Set_Target_Frame( float frame )					{ Channel.Set_Target_Frame( frame ); }
	virtual	float	Get_Target_Frame( void )							{ return Channel.Get_Target_Frame(); }
	virtual	float	Get_Current_Frame( void )							{ return Channel.Get_Frame(); }
	virtual	void	Update( float dtime );
private:
	BlendableAnimChannelClass	Channel;
};


#endif
