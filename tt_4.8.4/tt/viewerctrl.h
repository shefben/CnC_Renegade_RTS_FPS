/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/viewerctrl.h          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 11/06/01 4:24p                                              $*
 *                                                                                             *
 *                    $Revision:: 6                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __VIEWER_CTRL_H
#define __VIEWER_CTRL_H

#include "dialogcontrol.h"
#include "vector3.h"
#include "render2d.h"
#include "AABoxClass.h"
#include "engine_math.h"

////////////////////////////////////////////////////////////////
//	Forward declarations
////////////////////////////////////////////////////////////////
class SimpleSceneClass;
class CameraClass;
class RenderObjClass;
class LightClass;


////////////////////////////////////////////////////////////////
//
//	ViewerCtrlClass
//
////////////////////////////////////////////////////////////////
class ViewerCtrlClass : public DialogControlClass
{
public:

	enum InterfaceModeEnum {
		Z_ROTATION,				// Automatic rotation of camera around Z-axis.
		VIRTUAL_TRACKBALL		// Camera rotation with virtual trackball under user control.
	};
	
	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ViewerCtrlClass (void);
	virtual ~ViewerCtrlClass (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	// RTTI.
	ViewerCtrlClass *As_ViewerCtrlClass (void)		{ return this; }	

	//
	//	From DialogControlClass
	//
	void				Render (void);

	//
	// Configuation
	//
	void				Set_Background_Visible (bool isvisible)		{IsBackgroundVisible = isvisible;}
	void				Set_Rotation_Rate (float r)						{RotationRate = (float)DEG2RAD(r);}
	void				Set_Interface_Mode (InterfaceModeEnum mode, float rotationrate = 0.0f);
	void				Set_Camera_Min_Dist (float dist)				{ MinCameraDist = dist; }

	//
	//	Selection management
	//
	SimpleSceneClass *Peek_Scene()									{ return (Scene); }
	CameraClass		  *Peek_Camera()									{ return (Camera); }	
	void					Set_Model (const char *model_name);
	void					Set_Model (RenderObjClass *new_model);
	RenderObjClass	  *Peek_Model()									{ return (Model); }
	void					Set_Animation (const char *animation_name_name);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void				On_LButton_Down (const Vector2 &mouse_pos);
	void				On_LButton_Up (const Vector2 &mouse_pos);
	void				On_Set_Cursor (const Vector2 &mouse_pos);
	void				On_Frame_Update (void);
	void				Update_Client_Rect (void);

	void				Create_Control_Renderer (void);
	void				Free_Model (void);
	void				Get_Visible_Bounding_Box (AABoxClass *box, RenderObjClass *render_obj, bool &is_first);
	void				Calculate_Camera_Position (void);


	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	SimpleSceneClass *	Scene; // 0054
	CameraClass	*			Camera; // 0058
	LightClass *			Light; // 005C
	RenderObjClass *		Model; // 0060
	Render2DClass			ControlRenderer; // 0064
	AABoxClass				BoundingBox; // 0618
	float						Distance; // 0630
	float						ZRotation; // 0634
	float						RotationRate; // 0638
	float						MinCameraDist; // 063C
	bool						IsCameraDirty; // 0640
	bool						IsBackgroundVisible; // 0641
	InterfaceModeEnum		InterfaceMode; // 0644
	Vector3					LastMousePosition; // 0648
}; // 0654


#endif //__VIEWER_CTRL_H
