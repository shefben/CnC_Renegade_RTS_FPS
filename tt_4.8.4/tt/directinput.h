/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/directinput.h                         $*
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/15/02 5:31p                                               $*
 *                                                                                             *
 *                    $Revision:: 11                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef	DIRECTINPUT_H
#define	DIRECTINPUT_H

#include "vector3.h"
#include "vector2.h"

typedef enum {
	DIK_SHIFT = 0xF0,
	DIK_CONTROL,
	DIK_ALT,
	DIK_WIN,
} DupeKeys;

/*
**
*/
class	DirectInput {

public:
	typedef enum {
		NUM_KEYBOARD_BUTTONS	= 256,
		NUM_MOUSE_BUTTONS		= 5,
		NUM_JOYSTICK_BUTTONS	= 2
	} ButtonCounts;

	typedef enum {
		MOUSE_X_AXIS,
		MOUSE_Y_AXIS,
		MOUSE_Z_AXIS,
		NUM_MOUSE_AXIS,
	} MouseAxis;

	typedef enum {
		JOYSTICK_X_AXIS,
		JOYSTICK_Y_AXIS,
	} JoystickAxis;

	typedef enum {
		DI_BUTTON_HELD = 1,
		DI_BUTTON_HIT = 2,
		DI_BUTTON_RELEASED = 4
	} ButtonStateBitMask;

	/*
	** Buttons include all keyboard keys, plus the buttons
	** on the mouse and joysticks
	*/
	enum {
		BUTTON_KEYBOARD_FIRST	= 0,
		BUTTON_MOUSE_FIRST		= 256,
		BUTTON_MOUSE_LEFT			= BUTTON_MOUSE_FIRST,
		BUTTON_MOUSE_RIGHT,
		BUTTON_MOUSE_CENTER,
		BUTTON_JOYSTICK_FIRST,
		BUTTON_JOYSTICK_A			= BUTTON_JOYSTICK_FIRST,
		BUTTON_JOYSTICK_B,
		BUTTON_MAX
		//NUM_BUTTONS,
	};

	/*
	**
	*/
	static void Init( void );
	static void Shutdown( void );
	static void Read( void );
	static void Flush( void );

	static void Acquire(void);
	static void Unacquire(void);

	/*
	**
	*/
	static	int	Get_Keyboard_Button(	int button )	{	return DIKeyboardButtons[ button & 0xFF ]; }
	static	int	Get_Mouse_Button(	int button )		{	return DIMouseButtons[ button & 0xFF ]; }
	static	int	Get_Joystick_Button(	int button )	{	return DIJoystickButtons[ button & 0xFF ]; }
	static	long	Get_Mouse_Axis( MouseAxis axis )		{	return DIMouseAxis[axis]; }

	/*
	**
	*/
	static	void	Eat_Mouse_Held_States (void);

	// Still non-buffered
	static	long	Get_Joystick_Axis_State( JoystickAxis axis );

	//
	//	Cursor support
	//
	static void		Reset_Cursor_Pos (const Vector2 &pos)	{ CursorPos.X = pos.X; CursorPos.Y = pos.Y; }
	static void		Get_Cursor_Pos (Vector3 *pos)				{ *pos = CursorPos; }

	//
	//	Button support
	//
	static char		Get_Button_Value (int button_id);

	//
	//	Return the DIK ID of the last keyboard key pressed
	//
	static int		Get_Last_Key_Pressed (void)	{ return LastKeyPressed; }

private:

	//
	//	Internal methods
	//
	static void		Update_Double_Clicks (void);

	//
	//	Private member data
	//
	static	REF_ARR_DECL1(DIKeyboardButtons,char,256);
	static	char						DIMouseButtons[NUM_MOUSE_BUTTONS];
	static	long						DIMouseAxis[NUM_MOUSE_AXIS];
	static	char						DIJoystickButtons[NUM_MOUSE_BUTTONS];
	static	float						ButtonLastHitTime[NUM_KEYBOARD_BUTTONS];

	static REF_DECL1(CursorPos,Vector3);
	static	bool						EatMouseHeld;

	static	void *					DirectInputLibrary;

	static REF_DECL1(LastKeyPressed,int);

	static bool Captured;

	static	void ReadKeyboard( void );
	static	void ReadMouse( void );
	static	void ReadJoystick( void );

};

///////////////////////////////////////////////////////////
//	Get_Button_Value
///////////////////////////////////////////////////////////
TT_INLINE char
DirectInput::Get_Button_Value (int button_id)
{
	char retval = 0;

	if (button_id < BUTTON_MOUSE_FIRST) {
		retval = DIKeyboardButtons[button_id];
	} else if (button_id < BUTTON_JOYSTICK_FIRST) {
		retval = DIMouseButtons[button_id - BUTTON_MOUSE_FIRST];
	} else if (button_id < BUTTON_MAX) {
		retval = DIJoystickButtons[button_id - BUTTON_JOYSTICK_FIRST];
	}

	return retval;
}

#endif
