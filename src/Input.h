#pragma once

#include <Windows.h>
#include <dinput.h>

namespace t3d {

// these read the keyboard asynchronously
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code)   ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

class Input
{
public:
	Input();

	int Init(HINSTANCE instance);
	void Shutdown();

	int InitJoystick(HWND handle, int min_x=-256, int max_x=256, 
		int min_y=-256, int max_y=256, int dead_band = 10);
	int InitMouse(HWND handle);
	int InitKeyboard(HWND handle);

	int ReadJoystick();
	int ReadMouse();
	int ReadKeyboard();

	void ReleaseJoystick();
	void ReleaseMouse();
	void ReleaseKeyboard();

	const unsigned char* KeyboardState() const { 
		return keyboard_state; }
	DIMOUSESTATE& MouseState() {
		return mouse_state;
	}

private:
	LPDIRECTINPUT8       lpdi;				// dinput object
	LPDIRECTINPUTDEVICE8 lpdikey;			// dinput keyboard
	LPDIRECTINPUTDEVICE8 lpdimouse;			// dinput mouse
	LPDIRECTINPUTDEVICE8 lpdijoy;			// dinput joystick
	GUID                 joystickGUID;		// guid for main joystick
//	char                 joyname[80];		// name of joystick

	// these contain the target records for all di input packets
	unsigned char keyboard_state[256];	// contains keyboard state table
	DIMOUSESTATE mouse_state;	// contains state of mouse
	DIJOYSTATE joy_state;		// contains state of joystick
	int joystick_found;			// tracks if joystick was found and inited

}; // Input

}