/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/directinput.cpp                       $*
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/15/02 5:32p                                               $*
 *                                                                                             *
 *                    $Revision:: 25                                                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "General.h"
#include "directinput.h"
#include "TimeManager.h"

/*
**
*/
LPDIRECTINPUT			DIObject				= NULL;
LPDIRECTINPUTDEVICE	DIKeyboardDevice		= NULL;
LPDIRECTINPUTDEVICE	DIMouseDevice			= NULL;
LPDIRECTINPUTDEVICE2	DIJoystickDevice		= NULL;

DIJOYSTATE				DIJoystickState;

int PASCAL	InitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);

REF_ARR_DEF1(DirectInput::DIKeyboardButtons,char,256,0x0085DFE8);
char						DirectInput::DIMouseButtons[NUM_MOUSE_BUTTONS];
long						DirectInput::DIMouseAxis[NUM_MOUSE_AXIS];
char						DirectInput::DIJoystickButtons[NUM_MOUSE_BUTTONS];
float						DirectInput::ButtonLastHitTime[NUM_KEYBOARD_BUTTONS];
REF_DEF1(DirectInput::CursorPos,Vector3,0x0085DF78);
bool						DirectInput::EatMouseHeld = false;
bool						DirectInput::Captured = false;
void *					DirectInput::DirectInputLibrary = NULL;
REF_DEF1(DirectInput::LastKeyPressed,int,0x0085E104);

// Temp State Table (only for joystick currently)
char	Button_State_Table[4] = {	0,
								DirectInput::DI_BUTTON_HIT | DirectInput::DI_BUTTON_HELD,
								DirectInput::DI_BUTTON_RELEASED,
								DirectInput::DI_BUTTON_HELD };


// Buffered input
#define					DI_KEYBOARD_BUFFER_SIZE			20
#define					DI_MOUSE_BUFFER_SIZE				20

int PASCAL DirectInputInitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);

#define	MINIMUM_DIRECTINPUT_VERSION		0x300

// Button bits
#define		BUTTON_BIT_DOUBLE				8
#define		BUTTON_DOUBLE_THRESHHOLD	0.25f


typedef HRESULT (WINAPI *DirectInput8CreateType) (HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);
DirectInput8CreateType DirectInput8CreatePtr = NULL;


/*
**
*/
REF_DEF1(MainWindow, HWND, 0x0083DBB8);
extern HINSTANCE ProgramInstance;
void DirectInput::Init( void )
{

	HRESULT        hr;

	DirectInputLibrary = LoadLibrary("DINPUT8.DLL");

	if (DirectInputLibrary != NULL) {
		DirectInput8CreatePtr = (DirectInput8CreateType) GetProcAddress((HINSTANCE)DirectInputLibrary, "DirectInput8Create");

		if (DirectInput8CreatePtr) {

			// Create the DirectInput Object
			hr = DirectInput8CreatePtr( ProgramInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&DIObject, NULL);
			if FAILED(hr)
			{
				hr = DirectInput8CreatePtr( ProgramInstance, MINIMUM_DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&DIObject, NULL);
				if FAILED(hr)
				{
					FreeLibrary((HINSTANCE)DirectInputLibrary);
					DirectInputLibrary = NULL;
					return;
				}
			}
		}
	}

	// Create the Keyboard Object
	hr = DIObject->CreateDevice( GUID_SysKeyboard , &DIKeyboardDevice, NULL);

	if ( DIKeyboardDevice != NULL ) {

		// Set the keyboard's data format
		hr = DIKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);

		// Set the keyboard's cooperative level
		// First we try for "exclusive" access (mainly so debugging works well) if that fails
		// then we'll take non-exclusive access.
			DIKeyboardDevice->SetCooperativeLevel( MainWindow, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

		// Set Keyboard Buffer Size
		DIPROPDWORD dipdw;
		dipdw.diph.dwSize       = sizeof(dipdw);
		dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
		dipdw.diph.dwObj        = 0;
		dipdw.diph.dwHow        = DIPH_DEVICE;
		dipdw.dwData            = DI_KEYBOARD_BUFFER_SIZE;
		hr = DIKeyboardDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

		// Aquire the keyboard
		hr = DIKeyboardDevice->Acquire();
		if ( FAILED(hr) ) {
		}

	}

	// Create the Mouse Object
	hr = DIObject->CreateDevice( GUID_SysMouse, &DIMouseDevice, NULL );

	if ( DIMouseDevice != NULL ) {

		// Set the mouse's data format
		hr = DIMouseDevice->SetDataFormat(&c_dfDIMouse2);

		/**/
		// Set the mouse's cooperative level
		hr = DIMouseDevice->SetCooperativeLevel( MainWindow,
						DISCL_EXCLUSIVE | DISCL_FOREGROUND);
		/**/

		// Set Mouse Buffer Size
		DIPROPDWORD dipdw;
		dipdw.diph.dwSize       = sizeof(dipdw);
		dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
		dipdw.diph.dwObj        = 0;
		dipdw.diph.dwHow        = DIPH_DEVICE;
		dipdw.dwData            = DI_MOUSE_BUFFER_SIZE;
		hr = DIMouseDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

		// Aquire the mouse
		hr = DIMouseDevice->Acquire();
		if ( FAILED(hr) ) {
		}

	}

	// Enumerate the Joysticks
	DIObject->EnumDevices(DI8DEVCLASS_GAMECTRL, InitJoystick, DIObject, DIEDFL_ATTACHEDONLY );

	if ( DIJoystickDevice != NULL ) {

		// Set the joystick's data format
		hr = DIJoystickDevice->SetDataFormat( &c_dfDIJoystick );

		// Set the joystick's cooperative level
		hr = DIJoystickDevice->SetCooperativeLevel( MainWindow, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

		DIPROPRANGE diprg;
		diprg.diph.dwSize       = sizeof(diprg);
		diprg.diph.dwHeaderSize = sizeof(diprg.diph);

		// Set X Range
		diprg.diph.dwObj        = DIJOFS_X;
		diprg.diph.dwHow        = DIPH_BYOFFSET;
		diprg.lMin              = -1000;
 		diprg.lMax              = +1000;
		hr = DIJoystickDevice->SetProperty(DIPROP_RANGE, &diprg.diph);

		// Set Y Range
		diprg.diph.dwObj        = DIJOFS_Y;
		hr = DIJoystickDevice->SetProperty(DIPROP_RANGE, &diprg.diph);

		DIPROPDWORD dipdw;
		dipdw.diph.dwSize       = sizeof(dipdw);
		dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);

	   // set X axis dead zone to 20% (to avoid accidental turning)
		dipdw.diph.dwObj        = DIJOFS_X;
		dipdw.diph.dwHow        = DIPH_BYOFFSET;
		dipdw.dwData            = 200;
		hr = DIJoystickDevice->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

	   // set Y axis dead zone to 20% (to avoid accidental turning)
		dipdw.diph.dwObj        = DIJOFS_Y;
		hr = DIJoystickDevice->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

		// Aquire the mouse
		hr = DIJoystickDevice->Acquire();
		if ( FAILED(hr) ) {
		}
	}

	Captured = true;

	Flush();

	//
	//	Reset the double-click array entries
	//
	for ( int index = 0; index < NUM_KEYBOARD_BUTTONS; index++ ) {
		ButtonLastHitTime[index] = 1000;
	}

	return ;
}

/*
**
*/
void DirectInput::Shutdown( void )
{

	if ( DIKeyboardDevice ) {
		DIKeyboardDevice->Unacquire();
		DIKeyboardDevice->Release();
		DIKeyboardDevice = NULL;
	}

	if ( DIMouseDevice ) {
		DIMouseDevice->Unacquire();
		DIMouseDevice->Release();
		DIMouseDevice = NULL;
	}

	if ( DIJoystickDevice ) {
		DIJoystickDevice->Unacquire();
		DIJoystickDevice->Release();
		DIJoystickDevice = NULL;
	}

	if ( DIObject ) {
		DIObject->Release();
		DIObject = NULL;
		if (DirectInputLibrary) {
			FreeLibrary((HINSTANCE)DirectInputLibrary);
		}
	}
}

/*
**
*/
void DirectInput::Flush( void )
{
	memset( DIKeyboardButtons, 0, NUM_KEYBOARD_BUTTONS );
	memset( DIMouseButtons, 0, sizeof(DIMouseButtons) );
	memset( DIMouseAxis, 0, sizeof(DIMouseAxis) );
	memset( DIJoystickButtons, 0, sizeof(DIJoystickButtons) );
}


/*
** Acquire access to input devices
*/
void DirectInput::Acquire(void)
{

	if (Captured == false) {
		Flush();

		if (DIKeyboardDevice) {
			DIKeyboardDevice->Acquire();
		}

		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(MainWindow, &cursorPos);

		CursorPos.X = (float)cursorPos.x;
		CursorPos.Y = (float)cursorPos.y;

		if (DIMouseDevice) {
			DIMouseDevice->Acquire();
		}

		Captured = true;
	}
}


/*
** Release accesss to input devices.
*/
void DirectInput::Unacquire(void)
{

	if (Captured) {
		if (DIMouseDevice) {
			DIMouseDevice->Unacquire();
		}

		if (DIKeyboardDevice) {
			DIKeyboardDevice->Unacquire();
		}

		POINT cursorPos;
		cursorPos.x = (LONG)CursorPos.X;
		cursorPos.y = (LONG)CursorPos.Y;
		ClientToScreen(MainWindow, &cursorPos);
		SetCursorPos(cursorPos.x, cursorPos.y);

		Captured = false;
	}
}


/*
**
*/
int PASCAL InitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
   LPDIRECTINPUT pdi = (LPDIRECTINPUT)pvRef;

	if ( DIJoystickDevice == NULL ) {

		LPDIRECTINPUTDEVICE temp;
		HRESULT hr;
		hr = pdi->CreateDevice(pdinst->guidInstance, &temp, NULL);

		if ( !FAILED(hr)) {
			hr = temp->QueryInterface(IID_IDirectInputDevice2,
                                    (LPVOID *)&DIJoystickDevice);
			IDirectInputDevice_Release(temp);
		}
		if ( FAILED(hr) ) {
		} else {
			return DIENUM_STOP;		// we got one
		}
	}

	return DIENUM_CONTINUE;
}


/*
**
*/
void DirectInput::ReadKeyboard( void )
{
	if ( DIKeyboardDevice == NULL ) return;

	for (int i = 0; i < NUM_KEYBOARD_BUTTONS; i++ ) {
		DIKeyboardButtons[i] &= DI_BUTTON_HELD;	// make off all but the STATE
	}

	DWORD buffer_size = 1;
	DIDEVICEOBJECTDATA	input_buffer;

	bool done = false;
	while( !done ) {
		// Jani: Try to acquire first (Acquire doesn't increase ref count).
		HRESULT hr = DIKeyboardDevice->Acquire();
		if ( FAILED( hr) ) {
			return;
		}

		hr = DIKeyboardDevice->GetDeviceData(
			sizeof(DIDEVICEOBJECTDATA), &input_buffer, &buffer_size, 0 );

		if FAILED(hr) {

			if ( (hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED) ) {

				if (hr == DIERR_INPUTLOST) {
				}

				// Try to re-aquire
				hr = DIKeyboardDevice->Acquire();
				if ( FAILED( hr) ) {
					return;
				}
				continue;

			} else {
				return;
			}
		}

		if ( buffer_size == 0 ) {
			done = true;
		} else {

			if ( input_buffer.dwData & 0x80 ) {
				DIKeyboardButtons[ input_buffer.dwOfs ] |= DI_BUTTON_HIT;
				DIKeyboardButtons[ input_buffer.dwOfs ] |= DI_BUTTON_HELD;
				LastKeyPressed = input_buffer.dwOfs;
			} else {
				DIKeyboardButtons[ input_buffer.dwOfs ] |= DI_BUTTON_RELEASED;
				DIKeyboardButtons[ input_buffer.dwOfs ] &= ~DI_BUTTON_HELD;
			}
		}
	}

	// Set Dupe Keys
	DIKeyboardButtons[ DIK_CONTROL ]	= DIKeyboardButtons[ DIK_LCONTROL ]	| DIKeyboardButtons[ DIK_RCONTROL ] ;
	DIKeyboardButtons[ DIK_SHIFT ]	= DIKeyboardButtons[ DIK_LSHIFT ]	| DIKeyboardButtons[ DIK_RSHIFT ] ;
	DIKeyboardButtons[ DIK_ALT ]		= DIKeyboardButtons[ DIK_LALT ]		| DIKeyboardButtons[ DIK_RALT ];
	DIKeyboardButtons[ DIK_WIN ]		= DIKeyboardButtons[ DIK_LWIN ]		| DIKeyboardButtons[ DIK_RWIN ];

}


/*
**
*/
void DirectInput::ReadMouse( void )
{
	if ( DIMouseDevice == NULL ) return;

	for (int i = 0; i < sizeof( DIMouseButtons ); i++ ) {
		DIMouseButtons[i] &= DI_BUTTON_HELD;	// make off all but the STATE
	}

	for (int i = 0; i < (sizeof( DIMouseAxis )/sizeof( DIMouseAxis[0] ) ); i++ ) {
		DIMouseAxis[i] = 0;
	}

	DWORD						buffer_size = 1;
	DIDEVICEOBJECTDATA	input_buffer;

	bool done = false;
	while( !done ) {
		// Try to aquire first
		HRESULT hr = DIMouseDevice->Acquire();
		if ( FAILED( hr) ) {
			return;
		}
		hr = DIMouseDevice->GetDeviceData(
			sizeof(DIDEVICEOBJECTDATA), &input_buffer, &buffer_size, 0 );

		if FAILED(hr) {

			if ( (hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED) ) {

				if (hr == DIERR_INPUTLOST) {
				}

				// Try to re-aquire
				hr = DIMouseDevice->Acquire();
				if ( FAILED( hr) ) {
					return;
				}
				continue;

			} else {
				return;
			}
		}

		if ( buffer_size == 0 ) {
			done = true;
		} else {
			int	index = 0;

			switch( input_buffer.dwOfs ) {

				case	DIMOFS_Z:	index++;
				case	DIMOFS_Y:	index++;
				case	DIMOFS_X:
							DIMouseAxis[index]	+= input_buffer.dwData;
							CursorPos[index]		+= ((int)input_buffer.dwData) * 2;
			   			break;

				case	DIMOFS_BUTTON4:	index++;
				case	DIMOFS_BUTTON3:	index++;
				case	DIMOFS_BUTTON2:	index++;
				case	DIMOFS_BUTTON1:	index++;
				case	DIMOFS_BUTTON0:
							if ( input_buffer.dwData & 0x80 ) {
								DIMouseButtons[ index ] |= DI_BUTTON_HIT;
								DIMouseButtons[ index ] |= DI_BUTTON_HELD;
							} else {
								DIMouseButtons[ index ] |= DI_BUTTON_RELEASED;
								DIMouseButtons[ index ] &= ~DI_BUTTON_HELD;
								EatMouseHeld = false;
							}
							break;
			}
		}
	}

	//
	//	"Eat" the left mouse button as necessary
	//
	if ( EatMouseHeld ) {
		DIMouseButtons[ BUTTON_MOUSE_LEFT & 0xFF ] &= ~DI_BUTTON_HELD;
		DIMouseButtons[ BUTTON_MOUSE_LEFT & 0xFF ] &= ~DI_BUTTON_HIT;
		DIMouseButtons[ BUTTON_MOUSE_LEFT & 0xFF ] |= DI_BUTTON_RELEASED;
	}


}


/*
**
*/
void DirectInput::ReadJoystick( void )
{
	if ( DIJoystickDevice == NULL ) return;

    // poll the joystick to read the current state
retry_joystick:

	HRESULT  hr = DIJoystickDevice->Poll();

	if (FAILED(hr)) {

		if ( (hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED) ) {

			if (hr == DIERR_INPUTLOST) {
			}

			// Try to re-aquire
			hr = DIJoystickDevice->Acquire();
			if ( FAILED( hr) ) {
				return;
			}
			goto	retry_joystick;
		}
	}

	hr = DIJoystickDevice->GetDeviceState( sizeof(DIJoystickState), (LPVOID)&DIJoystickState );
	if FAILED(hr) {
		return;
	}

	DIJoystickButtons[ 0 ]	= Button_State_Table[ ((DIJoystickButtons[ 0 ]&1) << 1) + ((DIJoystickState.rgbButtons[ 0 ] & 0x80 )?1:0) ];
	DIJoystickButtons[ 1 ]	= Button_State_Table[ ((DIJoystickButtons[ 1 ]&1) << 1) + ((DIJoystickState.rgbButtons[ 1 ] & 0x80 )?1:0) ];

}


/*
**
*/
void DirectInput::Read( void )
{
	if (Captured) {
		ReadKeyboard();
		ReadMouse();
		ReadJoystick();

		Update_Double_Clicks();
	}

	return ;
}


/*
**
*/
void DirectInput::Eat_Mouse_Held_States (void)
{
	if (	(DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] & DI_BUTTON_HELD) ||
			(DIMouseButtons[BUTTON_MOUSE_LEFT & 0xFF] & DI_BUTTON_HIT))
	{
		EatMouseHeld = true;
	}

	return ;
}


/*
**
*/
long	DirectInput::Get_Joystick_Axis_State( JoystickAxis axis )
{
	return ((long*)&DIJoystickState.lX)[axis];
}


/*
**
*/
void	DirectInput::Update_Double_Clicks (void)
{
	float time_delta = TimeManager::RealFrameSeconds;
	for ( int index = 0; index < NUM_KEYBOARD_BUTTONS; index++ ) {

		//
		// Bump time since last
		//
		ButtonLastHitTime[index] += time_delta;

		//
		// If the button is hit, check for double and reset time
		//
		if ( DIKeyboardButtons[index] & DI_BUTTON_HIT ) {
			if ( ButtonLastHitTime[index] <= BUTTON_DOUBLE_THRESHHOLD ) {
				DIKeyboardButtons[index] |= BUTTON_BIT_DOUBLE;
			}
			ButtonLastHitTime[index] = 0;
		}
	}

	return ;
}