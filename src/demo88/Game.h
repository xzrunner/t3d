#include <windows.h>
#include <stdio.h>

#include "RenderObject.h"

namespace t3d {

class RenderList;
class Camera;

class Game
{
public:
	static const int WINDOW_WIDTH = 800;
	static const int WINDOW_HEIGHT = 640;
	static const int WINDOW_BPP = 32; // bitdepth of window (8,16,24 etc.)
	// note: if windowed and not
	// fullscreen then bitdepth must
	// be same as system bitdepth
	// also if 8-bit the a pallete
	// is created and attached
	static const int WINDOWED_APP = 1; // 0 not windowed, 1 windowed

public:
	Game(HWND handle, HINSTANCE instance);
	~Game();

	void Init();
	void Shutdown();
	void Step();

private:
	// create some constants for ease of access
	static const int AMBIENT_LIGHT_INDEX	= 0; // ambient light index
	static const int INFINITE_LIGHT_INDEX	= 1; // infinite light index
	static const int POINT_LIGHT_INDEX		= 2; // point light index
	static const int SPOT_LIGHT_INDEX		= 3; // spot light index

private:
	Camera* _cam;
	RenderObject _obj_player;
	RenderList* _list;

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[256];        // used to print text

}; // Game

}