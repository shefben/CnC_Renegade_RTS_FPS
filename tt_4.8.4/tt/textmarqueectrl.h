/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : wwui                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/textmarqueectrl.h                              $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 9/18/01 3:56p                                                 $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef __TEXTMARQUEECTRL_H
#define __TEXTMARQUEECTRL_H

#include "dialogcontrol.h"
#include "render2d.h"
#include "engine_vector.h"

//////////////////////////////////////////////////////////////////////
//
//	TextMarqueeCtrlClass
//
//////////////////////////////////////////////////////////////////////
class TextMarqueeCtrlClass : public DialogControlClass
{
public:

	///////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	///////////////////////////////////////////////////////////////////
	TextMarqueeCtrlClass  (void);
	~TextMarqueeCtrlClass (void);

	///////////////////////////////////////////////////////////////////
	//	Public methods
	///////////////////////////////////////////////////////////////////

	//
	//	Inherited
	//
	void				Render (void);
	void				Set_Text (const WCHAR *title);

	//
	//	Configuration
	//
	void				Set_Scroll_Rate (float rows_per_sec)		{ ScrollRate = rows_per_sec; }
	float				Get_Scroll_Rate (void)							{ return ScrollRate; }

protected:

	///////////////////////////////////////////////////////////////////
	//	Protected classes
	///////////////////////////////////////////////////////////////////
	typedef struct _CREDIT_LINE
	{
		_CREDIT_LINE (void)	:
			FontIndex (0), Color (0), Height (0)	{}

		_CREDIT_LINE (const WCHAR *text, int font_index, int color)	:
			Text (text), FontIndex (font_index), Color (color)	{}

		bool operator== (const _CREDIT_LINE &src)	{ return false; }
		bool operator!= (const _CREDIT_LINE &src)	{ return true; }
			
		WideStringClass	Text;
		int					FontIndex;
		int					Color;
		float					Height;

	} CREDIT_LINE;
	
	///////////////////////////////////////////////////////////////////
	//	Protected methods
	///////////////////////////////////////////////////////////////////
	void				Update_Client_Rect (void);
	void				On_Frame_Update (void);

	void				Create_Control_Renderer (void);
	void				Create_Text_Renderer (void);
	void				Calculate_Row_Count (void);

	int				Read_Tag (const WCHAR *text, CREDIT_LINE &line);
	const WCHAR *	Read_Line (const WCHAR *text, CREDIT_LINE &line);
	void				Build_Credit_Lines (void);

	///////////////////////////////////////////////////////////////////
	//	Protected member data
	///////////////////////////////////////////////////////////////////
	Render2DClass				ControlRenderer;
	Render2DSentenceClass	TextRenderers[2];
	float							ScrollPos;
	int							PixelHeight;
	float							ScrollRate;
	
	DynamicVectorClass<CREDIT_LINE>	CreditLines;
};


#endif //__TEXTMARQUEECTRL_H
