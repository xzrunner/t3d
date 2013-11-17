#include <windows.h>
#include <stdio.h>

#include "RenderObject.h"

namespace t3d {

class RenderList;
class Camera;

class Game
{
public:
	static const int WINDOW_WIDTH = 400;
	static const int WINDOW_HEIGHT = 400;
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
	// object defines
	static const int NUM_OBJECTS = 2;			// number of objects on a row
	static const int OBJECT_SPACING = 250;		// spacing between objects

private:
	Camera* _cam;

	RenderObject _obj;
	RenderList* _list;

	HWND main_window_handle; // save the window handle
	HINSTANCE main_instance; // save the instance
	char buffer[1024];        // used to print text

}; // Game

}