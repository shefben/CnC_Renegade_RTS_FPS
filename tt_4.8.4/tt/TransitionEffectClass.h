#ifndef TT_INCLUDE_TRANSITIONEFFECTCLASS_H
#define TT_INCLUDE_TRANSITIONEFFECTCLASS_H
#include "MaterialEffectClass.h"
#include "Vector2.h"
#include "mapper.h"
#include "MaterialPassClass.h"

class TransitionEffectClass : public MaterialEffectClass
{
public:
	TransitionEffectClass(void);
	~TransitionEffectClass(void);
	virtual void		Timestep(float dt);
	virtual void		Render_Push(RenderInfoClass & rinfo,PhysClass * obj);
	virtual void		Render_Pop(RenderInfoClass & rinfo);
	void					Set_Parameter(float param)							{ CurrentParameter = param; }
	void					Set_Target_Parameter(float param)				{ TargetParameter = param; }
	void					Set_Texture(TextureClass * texture)
	{
		if (MaterialPass)
		{
			MaterialPass->Set_Texture(texture,0);
		}
	}
	void					Set_Start_Delay(float seconds)					{ StartDelay = seconds; }
	void					Set_Transition_Time(float seconds)				{ if (seconds > 0.0f) ParameterVelocity = 1.0f / seconds; }
	void					Set_Min_UV_Velocity(const Vector2 & uv_vel)	{ MinUVRate = uv_vel; }
	void					Set_Max_UV_Velocity(const Vector2 & uv_vel)	{ MaxUVRate = uv_vel; }
	void					Set_Max_Intensity(float max_inten)				{ MaxIntensity = max_inten; }
	void					Enable_Remove_On_Complete(bool onoff)			{ RemoveOnComplete = onoff; }
	float					Get_Parameter(void) const							{ return CurrentParameter; }
	float					Get_Target_Parameter(void) const					{ return TargetParameter; }
	float					Get_Max_Intensity(void) const						{ return MaxIntensity; }
	float					Get_Start_Delay(void) const						{ return StartDelay; }
	bool					Is_Remove_On_Complete_Enabled(void) const		{ return RemoveOnComplete; }
	bool					Save(ChunkSaveClass & csave);
	bool					Load(ChunkLoadClass & cload);
protected:
	float					CurrentParameter; //20
	float					TargetParameter; //24
	float					ParameterVelocity; //28
	Vector2				MinUVRate; //32
	Vector2				MaxUVRate; //40
	Vector2				UVRate; //48
	float					MaxIntensity; //56
	float					StartDelay; //60
	bool					RemoveOnComplete; //64
	bool					RenderBaseMaterial; //65
	bool					RenderTransitionMaterial; //66
	float					IntensityScale; //68
	Vector2				UVOffset; //72
	MatrixMapperClass *		Mapper; //80
	MaterialPassClass *		MaterialPass; //84
};

#endif
